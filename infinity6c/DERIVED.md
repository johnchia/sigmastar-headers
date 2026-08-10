# infinity6c — what has been derived, and how

Working notes for the family. Everything here came out of the prebuilt MI
libraries the target ships; nothing was taken from vendor source. Kept because
each number cost a disassembly and the next pass should not repeat it.

## The libraries are forthcoming about sizes

MI marshals across an ioctl. Each userspace wrapper builds a small block on the
stack, writes the payload size into it as a literal, and hands it to `ioctl`:

```
$ arm-linux-*-objdump -d --disassemble=MI_SYS_GetVersion libmi_sys.so
  strh.w r0, [sp]        @ SoC id, stored as a halfword -> the parameter is 16-bit
  movs   r3, #128        @ payload size
  str    r3, [sp, #4]    @ into the size slot
  strd   r2, r3, [sp, #8]@ caller's pointer, sign-extended to 64 bits
```

So a size can be read off the disassembly. Two things stop that being the whole
answer.

**The size slot describes the marshalled block, not the struct.** Most calls
prepend their device, channel and port ids to the payload, so the struct is the
remainder. Read the copy that follows — `ldmia`/`stmia` pairs, or a `memcpy`
with a literal length — and subtract.

**Read the ioctl command, not the stack frame.** The command word encodes the
payload size in its `_IOC` size field — bits 16..29 — so the size falls out of the
`movt` that builds it, with no frame to interpret:

```
MI_VENC_SetChnAttr:  movt r1, #0x4054   ->  0x4054 & 0x3fff = 84 bytes
MI_VIF_SetDevAttr:   movt r1, #0x4018   ->  0x4018 & 0x3fff = 24 bytes
```

This is the method to use. It agrees with every size in the table below, it needs
no assumption about where a function keeps its size slot, and it is what makes the
next trap harmless.

**The size slot is not always at `[sp, #4]`.** That offset holds for SYS, VIF,
SNR and SCL, but VENC builds a larger frame and puts it at `[sp, #28]`. A script
that assumes the offset reports whatever happens to live there — this is where
the "295" in an earlier revision of these notes came from. It was never a size.
Read the frame rather than pattern-matching it.

## Sizes established this way

| struct | payload | id words | struct |
| --- | --- | --- | --- |
| `MI_SYS_Version_t` | 128 | 0 | **128** |
| `MI_VIF_GroupAttr_t` | 32 | 4 (group) | **28** |
| `MI_VIF_DevAttr_t` | 24 | 4 (dev) | **20** |
| `MI_SNR_PlaneInfo_t` | 80 | 8 (pad, plane) | **72** |
| `MI_SCL_OutputPortParam_t` | 36 | 12 (dev, chn, port) | **24** |
| `MI_VENC_ChnAttr_t` | 84 | 8 (dev, chn) | **76** |
| `MI_VENC_InitParam_t` | 12 | 4 (dev) | **8** |
| `MI_VENC_ParamJpeg_t` | 144 | 8 (dev, chn) | **136** |
| `MI_VENC_ChnStat_t` | 48 | 8 (dev, chn) | **40** |
| `MI_VENC_Stream_t` | 84 | 8 (dev, chn) + 4 (timeout) | **72** |
| `MI_VIF_OutputPortAttr_t` | 32 | 8 (dev, port) | **24** |
| `MI_SCL_ChnParam_t` | 12 | 8 (dev, chn) | **4** |

Eleven of the twelve have been checked against divinus's vendored layouts,
compiled for the 32-bit ARM target, and **all eleven agree** — `i6c_sys_ver` 128,
`i6c_vif_grp` 28, `i6c_vif_dev` 20, `i6c_vif_port` 24, `i6c_snr_plane` 72,
`i6c_scl_port` 24, `i6c_venc_chn` 76, `i6c_venc_init` 8, `i6c_venc_jpg` 136,
`i6c_venc_stat` 40, `i6c_venc_strm` 72. The twelfth, `MI_SCL_ChnParam_t`, has no
divinus counterpart to check — divinus does not call `MI_SCL_SetChnParam` — so
`i6c_scl_chn` is declared from the binaries alone. Two more that no payload
reaches, `i6c_venc_pack` 184 and `i6c_venc_packinfo` 16, come from array strides
instead, below. The assertions live in the headers themselves, so the check runs
on every build rather than once.

**The id-word column used to be an argument count, and is now a reading.** Every
module's ioctl thunk splits the payload before calling the `IMPL` body, so the
boundary is in the instructions rather than in an assumption about the signature.
That retired two guesses: `MI_VIF_IOCTL_SetDevAttr` takes **one** id
(`ldr.w r7, [r4], #4`), which is what makes `MI_VIF_DevAttr_t` 20 rather than 16,
and `MI_SCL_IOCTL_SetOutputPortParam` takes **three** (`ldr [r4]`,
`ldrd [r4, #4]`, struct at `+12`), which is what makes `MI_SCL_OutputPortParam_t`
24 rather than 28. Both assertions predated the check and both survive it.

Worth being exact about what that proves. A matching size says the field *set* is
right and nothing is missing or spurious; it does not say the fields are in the
right order, since permuting same-width members preserves the total. Most have had
their order or their field widths checked as well, each from whatever in the
modules reads them: `i6c_vif_grp` from `MI_VIF_CHECK_GroupAttr`'s bounds,
`i6c_vif_dev` and `i6c_vif_port` from the validators and `MI_VIF_IMPL_SetDevAttr`,
`i6c_venc_chn` from the fields `MI_VENC_IMPL_SetChnAttr` excludes from its
comparison, and `i6c_venc_strm`, `i6c_venc_pack`, `i6c_venc_packinfo`,
`i6c_venc_jpg` and `i6c_venc_stat` from the code that fills them. That leaves
`i6c_sys_ver` (a byte array, so nothing to order), `i6c_snr_plane` and
`i6c_scl_port` on size alone.

Absent from the table because it is not one struct: `MI_SYS_BindChnPort2` reports
56, which is two channel ports plus the source and destination rates.

`MI_SCL_ChnParam_t` was recorded here as having "no matching size slot, so it is
shaped differently". It is not shaped differently — it is four bytes, and a
one-word struct is small enough that the number looked like something else.

## VENC, and the one trap that is in the measuring rather than the binary

`MI_VENC_CreateChn` marshals **84** bytes and `memcpy`s **76** of them; the
difference is the device and channel ids it prepends. So `MI_VENC_ChnAttr_t` is
**76 bytes**.

    3a3c:  movs r2, #76      @ memcpy length -- the struct
    3a44:  blx  memcpy
    3a4a:  movs r1, #84      @ payload size ...
    3a52:  str  r1, [sp, #28] @ ... into VENC's size slot

Confirmed twice over: `MI_VENC_SetChnAttr` marshals the same 84 and copies the
same 76 from a different frame layout, and both calls' `_IOC` size fields read
0x4054, i.e. 84.

**Measure the target ABI with the cross compiler.** divinus's `i6c_venc_chn`
measures 80 on an x86-64 host and 76 on the 32-bit ARM target, because
`i6c_venc_rate` ends in a `void *`: eight bytes on LP64, and the trailing padding
that pointer's alignment adds. A host `sizeof` therefore reported a four-byte
disagreement with the binaries that does not exist. This is the only trap here
that lives in the method rather than in the artifact, and it is the easiest one to
repeat, since nothing about it looks like a cross-compilation question. Four of
the nine `infinity6e` headers already fail to compile for a 64-bit host for the
same reason — their assertions are 32-bit facts.

### The driver confirms both halves and the boundary between them

`MI_VENC_IMPL_SetChnAttr` in `mi_venc.ko` compares the incoming attribute against
the channel context's copy in two pieces, which pins each half's size and where
the split falls:

    1205a:  mov   ip, r8          @ r8 = the caller's ChnAttr
    1206a:  movs  r2, #40         @ ... 40 bytes copied from offset 0 ...
    12074:  add.w r5, r4, #252    @ ... compared against context + 252
    1207c:  bl    memcmp

    1215a:  add.w ip, r8, #40     @ second piece starts at offset 40
    1216c:  add.w r1, r4, #292    @ ... against context + 292, i.e. 252 + 40
    12176:  movs  r2, #36         @ ... and runs 36 bytes
    1217a:  bl    memcmp

40 then 36 is 76, and the context's two mirrors sit 40 apart, so the halves are
adjacent and neither has slack. `_MI_VENC_IMPL_ConfigRcAttr` agrees from the other
direction: handed the whole attribute, it reads the codec from offset 0 and then
does `adds r5, #40` to reach the rate half before passing it to
`_MI_VENC_IMPL_ConvertRc` (0xea60). Its `subs r2, r3, #2` / `cmp r2, #2` bounds
that codec to 2..4, which is divinus's `i6c_venc_codec` exactly — H264 2, H265 3,
MJPG 4.

### The excluded fields give the interior offsets

When the first comparison differs, the driver does not reject the change outright.
It overwrites a few fields in its copy of the incoming attribute with the
channel's current values and compares again, and only fails if something *else*
moved. Those fields are the ones a running channel is allowed to change, and the
offsets it writes are readable:

| codec | offsets rewritten | divinus's fields at those offsets |
| --- | --- | --- |
| H264, H265 (0x120d4) | 16, 24, 28 | `profile`, `width`, `height` |
| MJPG (0x12116) | 20, 24 | `width`, `height` |

Both arms reproduce, and the MJPG arm is the stronger half of the result: it has
no `profile` field, so its `width` and `height` sit four bytes earlier than
H26x's, and the driver's offsets shift by exactly that. Size alone could not have
shown this — permuting same-width members preserves a total, and these two arms
have different shapes.

Two consequences for a consumer. `MI_VENC_SetChnAttr` will refuse any change to
the codec half outside resolution and profile, so a codec, buffer size or
reference-frame change means destroying the channel and creating it again. And the
rate half gets no such exemptions: any difference over its 36 bytes sets the
rate-dirty flag at context offset 534 and provokes a rate reconfiguration. Since
the test is a plain `memcmp`, the unused tail of the smaller union arms counts —
clear an `i6c_venc_rate` before filling it, or a stale byte outside the arm in use
reconfigures the encoder for no reason.

`MI_VENC_SetRcParam` is not the way to isolate the rate half, incidentally: its
payload is 64, so the struct it carries is 56, and that is `MI_VENC_RcParam_t` —
a separate and larger thing than the rate attributes embedded in `ChnAttr`. No
wrapper marshals either half of `ChnAttr` on its own.

Two dead ends worth not repeating. `mi_vcodec.ko` is the driver and MHAL layer and
contains zero `MI_VENC_*` symbols; the MI layer is `mi_venc.ko`, unstripped, 1816
symbols. And `MI_VENC_CHECK_ChnAttr` does not exist — VENC does not follow VIF's
`CHECK_`/`DEBUG_` naming, so that technique does not transfer. It has
`MI_VENC_IOCTL_*` thunks over `MI_VENC_IMPL_*` bodies instead, and the comparison
above is what stands in for a validator.

### The ioctl thunks hand over the id-word count

The subtraction in the table above stops being an argument count once the module
object is open. Every `MI_VENC_IOCTL_*` thunk splits the marshalled payload itself
before calling the `IMPL` body, so the boundary is in the instructions:

    MI_VENC_IOCTL_InitDev:    ldr.w r7, [r4], #4   @ one id, struct at payload+4
    MI_VENC_IOCTL_GetStream:  ldrd  r7, r8, [r1]   @ two ids ...
                              add.w r2, r4, #8     @ ... struct at payload+8
                              ldr.w r9, [r1, #80]  @ 4th argument past the struct

`GetChnAttr`, `SetChnAttr` and `Query` take the same two-id shape. `GetStream` is
the useful one: its `_IOC` size is 84 and it reads its timeout from offset 80, so
`MI_VENC_Stream_t` is bracketed between 8 and 80 without any appeal to how many
ids a call "should" have. `MI_VENC_ReleaseStream` marshals 80 — the same struct
with no timeout — which is the difference showing up as a size.

### Strides, where nothing is marshalled

`MI_VENC_Pack_t` never crosses the boundary: the stream struct points at an array
the caller owns, and the library fills it in place. Its size is therefore the
stride the library steps that array by, which is stated outright:

    2a96:  mov.w ip, #184        @ the stride
    2aba:  mul.w r1, ip, fp      @ ... times the pack index
    2ace:  add.w ip, r3, r0      @ ... off strm.packet

**184**, and `i6c_venc_pack` measures 184. Its declared members reach 180, and the
remaining four bytes are alignment rather than an omission — the two `u64` members
align the struct to 8. The offsets come from the same loop, which writes `addr` and
`data` as one `strd` at 0, `timestamp` as an 8-byte `vstr` at 16, `endFrame` as the
prefix's only byte store at 24, `offset` and `packNum` at 32, `frameQual` at 40,
and `picOrder` with `gradient` as a `strd` at 44.

`MI_VENC_PackInfo_t`'s size is a stride too, from the H264 NAL scanner:

    2d12:  lsls r0, r4, #4        @ index * 16
    2d16:  add  r0, r2            @ ... off the pack base
    2d1a:  strd r3, r1, [r0, #52] @ packType and offset, so the array starts at 52
    2d28:  cmp  r5, #7            @ ... and holds 8

`length` is back-filled one entry behind, as this entry's offset less the previous
one's (`ldr r3, [r0, #40]` against `str r3, [r4, #44]`), which places it at 8 and
leaves `sliceId` the remaining word at 12.

### Confirmed a field at a time

`MI_VENC_ChnStat_t` needs no size argument at all: `MI_VENC_IMPL_Query`'s debug
path reads the struct in one descending sweep — offsets 36, 32, 28, 24, 20, 16, 12,
8, 4, 0 and nothing above 36 — so it is exactly ten words, which is both the 40 the
payload implies and divinus's field count.

`MI_VENC_ParamJpeg_t`'s two 64-byte tables place themselves:

    274a:  add.w r1, r5, #68      @ the second table
    274e:  ldr.w r2, [r3], #4     @ quality, advancing to 4 ...
    2756:  ldrb.w r0, [r3], #1    @ ... where the first table is read as bytes

Quality at 0, `qtLuma` from 4, `qtChroma` from 68 — so `qtLuma` is 64 long — and
with the total at 136 the trailing word at 132 is `mcuPerEcs`.

The three stream-info union arms each confirm at one interior offset, and the
offsets carry more weight than a size would. `MI_VENC_GetStream` extracts the same
3-bit reference type and writes it to `strm+60` on the H264 path and `strm+52` on
the H265 path; with the union at `strm+16` those are offsets 44 and 36, which is
`refType` in each arm, and they differ by exactly the eight bytes the H264 arm is
longer. MJPG's frame quality goes to `strm+24`, its arm's offset 8. Only the H264
arm's *size* is pinned, since as the largest it is the union's own size — a shorter
arm's size is unobservable, so 48 and 12 stay bounded rather than fixed.

Still unchecked in `i6c_venc.h`: every enumeration except `i6c_venc_codec`, whose
2..4 `ConfigRcAttr` bounds explicitly with `subs r2, r3, #2` / `cmp r2, #2`.

## The SoC id is usually not a parameter

Only MI_SYS and MI_RGN take it as a distinct leading argument. VIF, SNR and SCL
**pack it into the high halfword of the device or pad argument** — the wrappers
all do

```
  str    r6, [sp, #16]   @ full argument becomes the payload's first word
  lsrs   r6, r6, #16     @ its high half ...
  strh.w r6, [sp]        @ ... becomes the SoC id in the ioctl header
```

which is why a HAL written for a single-die part never appears to pass one, and
why that works: the id is 0, so the packed argument is just the device index.
`MI_VIF_CreateDevGroup` does not even read it from the argument — it stores a
literal 0.

## Where this method stops

Field order and offsets are mostly **not** derivable from these libraries. Setters
and getters copy the caller's struct wholesale — `ldmia`/`stmia` or `memcpy` — and
never touch a field, because the decoding happens on the other side of the ioctl.
For those the sizes are real and the interiors are opaque.

The exception is worth knowing, because it is where the richest offsets in this
document came from: a call that hands back an array the *caller* owns cannot
delegate, since the array never crosses the boundary. `MI_VENC_GetStream` fills the
pack array itself, field by field, and so gives up its stride and its whole
interior in userspace. If a struct's offsets seem unreachable, check whether some
call has to touch it on this side.

The decoders are the per-module kernel objects, and they are not in
sigmastar-lib, which ships userspace libraries only. They are in
`johnchia/sigmastar-sdk` under

    infinity6c/kmod-5.10.61-0907-uclibc-9.1.0/mi_{sys,vif,scl,sensor,vcodec}.ko

matching this drop by the `0907` in the directory name. Use the uclibc set to
match what the target runs, though kernel objects should not differ.

## Those modules are unstripped, which is the real lever

`mi_vif.ko` alone exports 2574 symbols, and they are named in a pattern that
does the work for you:

| symbol | what it gives |
| --- | --- |
| `MI_VIF_CHECK_DevAttr` | validates fields individually, so its loads are the offsets |
| `MI_VIF_DEBUG_OnDumpDevAttr` | prints the struct through `MI_SYS_Proc_Print`, so format strings pair names with offsets |
| `MI_VIF_IMPL_*` | the real implementation behind each entry point |

Validation strings corroborate the field set independently — `IntfMode %d err`,
`HdrType %d err`, `group %d, workmode %d invaild`, `Clkedge`, `Compress`,
`incrop` — so a derived layout can be checked against what the module complains
about rather than against a guess.

Two practical notes. `objdump --disassemble=<sym>` misjudges function bounds on
a relocatable object and dumps a section's worth of relocations; disassemble
`.text` and cut the range instead. And read offsets from the `CHECK_` and
`DEBUG_` functions rather than the `IMPL_` ones, which mostly pass structures
onward.

None of this needs hardware. What it needs is a careful pass per struct, which
is the work the sizes above only set the boundary for.

## VIF, from `MI_VIF_CHECK_*` in mi_vif.ko

Offsets are the struct-relative loads; widths are the load instruction
(`ldr`/`ldrh`/`ldrb`); bounds are the immediate each is compared against. Both
maps close against the sizes derived from userspace, which is the useful check —
the size and the offsets come from opposite sides of the ioctl and agree.

`MI_VIF_GroupAttr_t`, 28 bytes, from `MI_VIF_CHECK_GroupAttr` (0x3490):

| offset | width | bound |
| --- | --- | --- |
| 0 | 32 | < 5 |
| 4 | 32 | < 2 |
| 8 | 32 | < 4 |
| 12 | 32 | < 2 |
| 16, 20, 24 | — | not range-checked |

The three unchecked tail words are consistent with a flag and a bitmask, which
this function would have no bound to test.

`MI_VIF_DevAttr_t`, 20 bytes. Two sources rather than one: the validator bounds
some fields, and `MI_VIF_IMPL_SetDevAttr` reads all of them, which is what settles
the widths.

| offset | width | what reads it | name |
| --- | --- | --- | --- |
| 0 | 32 | `MI_VIF_PLATFORM_DevPixelSupport` | pixel format |
| 4, 6 | 16, 16 | IMPL only | crop origin |
| 8, 10 | 16, 16 | `_MI_VIF_CHECK_RectValid(w, h)` | crop width, height |
| 12 | 32 | `_MI_VIF_CHECK_FieldValid`, bounded ≤ 3 | field |
| 16 | 8 | read as a byte | half horizontal scan |
| 17..19 | — | nothing reads past 16 | padding to 20 |

Every name here comes from something that reads the field rather than from
plausibility. The earlier revision of these notes hedged on whether 4 and 6
existed as separate fields, because the validator does not touch them;
`MI_VIF_IMPL_SetDevAttr` does, reading four consecutive halfwords at 4, 6, 8 and
10, so the crop is a rect and only its width and height are range-checked.

Two corrections to that earlier revision. Offset 0 was recorded as merely
"non-zero" — it is handed to `MI_VIF_PLATFORM_DevPixelSupport`, which names it. And
offset 12's bound was recorded as `< 3`; the instruction is `cmp r3, #3` with
`bls`, so it is **≤ 3** and the field has four values. That matters, because
divinus annotates its `field` member as "Values 0-3 correspond to No, Top, Bottom,
Both" — four values, which the corrected bound matches and the wrong one
contradicted. The module confirms the last of them in as many words, complaining
that "Field is both, not support port1 out".

`MI_VIF_OutputPortAttr_t`, 24 bytes, from `MI_VIF_CHECK_PortAttr` (0x3ca0): six
halfwords at 0, 2, 4, 6, 8 and 10, then three words at 12, 16 and 20. That is an
8-byte rect, a 4-byte dimension and three enums, and it sums to the 24 the payload
gives — the two derivations meet from opposite sides of the ioctl. The validator
names two of the three words by what it passes them to,
`MI_VIF_PLATFORM_OutputPortPixelSupport` and `MI_VIF_PLATFORM_SupportCompress`,
and its complaints distinguish the rect from the dimension: "check CapRect w/h
%dx%d invalid" against "PortId %u, check dest rect invalid".


### Naming, as far as the strings take it

`.rodata.str1.1` holds the validator's complaints in source order, and the group
block is contiguous:

```
intface %d not support
group %d, IntfMode %d invaild
eHDRType %d not support
group %d, hdr %d invaild
group %d, workmode %d invaild
group %d, ClkEdge %d invaild
```

So `GroupAttr` carries an interface mode, an HDR type, a work mode and a clock
edge, and the four bounds-checked words are those four. **Only the first is
placed.** Offset 0 is the interface mode: it is checked first and its bound of 5
is the only one that fits an interface enum.

The other three are open, and the uncertainty is wider than a first look
suggests:

| offset | bound | field |
| --- | --- | --- |
| 0 | < 5 | interface mode |
| 4 | < 2 | one of: HDR type, work mode, clock edge |
| 8 | < 4 | one of the same three |
| 12 | < 2 | one of the same three |

### Resolved, by three lines that agree

The bounds alone were misleading. A multiplex work mode *looks* like the field
with more values, which put it at offset 8 — and that was wrong. Two other lines
say otherwise and agree with each other:

- The validator checks in the order 0, 8, 4, 12, while its complaints sit in the
  string table as IntfMode, hdr, workmode, ClkEdge. Corresponding those one to
  one puts hdr at 8 and the work mode at 4.
- divinus's `i6c_vif_grp` — a reconstruction, so a hypothesis rather than
  evidence — reads `intf, work, hdr, edge, clock, interlaceOn, grpStitch`, which
  is **28 bytes**, the size derived independently from the userspace wrapper, and
  which puts work at 4 and hdr at 8.

So:

| offset | width | bound | field |
| --- | --- | --- | --- |
| 0 | 32 | < 5 | interface mode |
| 4 | 32 | < 2 | work mode |
| 8 | 32 | < 4 | HDR type |
| 12 | 32 | < 2 | clock edge |
| 16 | 32 | not checked here | clock |
| 20 | 32 | — | interlace flag |
| 24 | 32 | — | group stitch mask |

Worth being clear about what carried the weight. The offsets, widths, bounds and
the 28 came from the binaries. divinus supplied an ordering to test, and the test
is that its layout reproduces a size and four bounds it had no access to. It
broke a tie rather than serving as the source — but it did inform the result, so
the family's provenance is honestly "derived from the binaries, with a
third-party reconstruction used to choose between orderings the binaries had
narrowed to two."

An enum bounded at 4 and one bounded at 2 are both ordinary, so nothing here was
decidable by plausibility, and the original guess from plausibility was the wrong
one. That is the lesson worth keeping.

One trap costs an attempt if you do not know it. These are ARM **REL**
relocations, not RELA, so the addend is stored *in place* rather than in the
relocation entry. `objdump -r` therefore shows only
`R_ARM_ABS32  .rodata.str1.1` with no offset, and grepping for an addend finds
nothing. The string offset is the four-byte word sitting at the literal-pool
address the relocation points at, so the sequence is: find the pool entry the
`ldr rN, [pc, #...]` resolves to, read the word stored there, and use it as a
byte offset into `.rodata.str1.1`
(`objdump -s -j .rodata.str1.1`). Only then does a branch get a name.

## SCL, from `MI_SCL_CHECK_ChnParam` in mi_scl.ko

`MI_SCL_ChnParam_t` is four bytes — one word, which the validator (0x3488) accepts
at zero and rejects above 3:

    34c4:  ldr r3, [r6, #0]
    34c6:  cmp r3, #0
    34c8:  beq 34c0        @ zero returns immediately
    34ca:  cmp r3, #3
    34cc:  bls 34fa        @ 1..3 continue to the source-mask check

So four values, the first meaning "off". The module calls the field `rot`: its
second check complains "SclSrcMask 0x%x, not support rot", and elsewhere it carries
`_HalSclSetRotationCfg` and `MI_SCL_CHECK_MirrorFlipVaild` as separate things — so
this word is the rotation, not the mirror and flip pair that `i6c_scl_port` already
holds.

**The angles are not derived.** Nothing in the module prints 90, 180 or 270, so
mapping 1, 2 and 3 onto them is convention, and `i6c_scl_chn` says so at the
declaration. It is the one place in this family where a label rests on convention
rather than on a read, and it is tolerable only because the failure is visible:
a turned picture, not corrupted memory.

This is also the only struct here with no divinus counterpart — divinus never calls
`MI_SCL_SetChnParam` — so there was nothing to check against and nothing to keep
diffable.
