# infinity6c — what has been derived, and how

Working notes for the family. Almost everything here came out of the prebuilt MI
libraries and kernel objects the target ships, with no vendor source involved.
Kept because each number cost a disassembly and the next pass should not repeat
it.

The exception is `i6c_aud.h`, which is transcribed from vendor headers that turned
out to be available, and then checked against the shipped library by the methods
below rather than taken on trust. Its section says so and shows the check. Where a
vendor header and a shipped blob disagree, the blob wins — that has already cost
this project an audio regression once.

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
| `MI_ISP_ChannelAttr_t` | 88 | 8 (dev, chn) | **80** |
| `MI_ISP_ChnParam_t` | 28 | 8 (dev, chn) | **20** |
| `MI_ISP_OutputPortParam_t` | 32 | 12 (dev, chn, port) | **20** |

All twelve have been checked against divinus's vendored layouts, compiled for the
32-bit ARM target, and **all twelve agree** — `i6c_sys_ver` 128, `i6c_vif_grp` 28,
`i6c_vif_dev` 20, `i6c_vif_port` 24, `i6c_snr_plane` 72, `i6c_scl_port` 24,
`i6c_venc_chn` 76, `i6c_venc_init` 8, `i6c_venc_jpg` 136, `i6c_venc_stat` 40,
`i6c_venc_strm` 72, `i6c_scl_chn` 4. The last of those agrees in a form worth
spelling out, since an earlier revision here recorded it as having no counterpart
at all: divinus passes a bare `int *` rather than a struct, so it corroborates the
one-word payload without naming a type. Two more that no payload reaches,
`i6c_venc_pack` 184 and `i6c_venc_packinfo` 16, come from array strides instead,
below. The three ISP layouts are in their own section further down. The assertions
live in the headers themselves, so the check runs on every build rather than once.

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

## MI_AI, the one family member that is a transcription

Audio input is the exception to everything above: the vendor's `mi_ai.h`,
`mi_ai_datatype.h` and `mi_aio_datatype.h` are available, and all three ssc377
drops (0602, 0712, 0907) ship them **byte-identical** at AI version 3.53. So
`i6c_aud.h` is transcribed rather than reconstructed.

It was checked against the shipped `libmi_ai.so` anyway, because a vendor header
that disagrees with the blob it describes has already cost this project an audio
regression. Both layouts agree, and both are pinned twice over.

| struct | payload | id words | struct |
| --- | --- | --- | --- |
| `MI_AI_Attr_t` | 24 | 4 (dev halfword + pad) | **20** |
| `MI_AI_Data_t` | — | — | **80** |

`MI_AI_Open` gives the first one three ways at once:

```
1d8c:  lsrs   r7, r0, #16     @ SoC id: the high halfword of AiDevId
1d90:  uxtb   r6, r0          @ device: its low byte
1da4:  strh.w r6, [sp, #16]   @ payload+0, as a halfword
1da8:  strh.w ip, [sp, #18]   @ payload+2, a zero halfword
1d9c:  ldmia  r4!, {r0,r1,r2,r3}  @ 16 bytes of the caller's struct ...
1db0:  ldr    r4, [r4, #0]        @ ... and a fifth word
1db8:  movs   r1, #24         @ payload size
1dce:  movt   r1, #0x4018     @ _IOC size field: 0x18 = 24
```

24 less the 4 id bytes is 20, and the five whole words the wrapper reads are the
five members. Note the fifth: `bInterleaved` is `MI_BOOL`, i.e. one byte, and the
library marshals it as a word — so the three padding bytes after it cross the
ioctl boundary. **Clear the struct before filling it**, the same rule the ISP
section ends on.

`MI_AI_Data_t` never crosses the boundary at all. `MI_AI_Read`'s two ioctls carry
4- and 12-byte payloads, and the library fills the caller's descriptor in
userspace from a mapped buffer — the `MI_VENC_Pack_t` exception. That makes its
interior readable rather than opaque, and it gives up a size and two offsets:

```
2ae2:  ldr  r3, [sp, #16]         @ the caller's pstData
2aee:  movs r2, #80               @ ... memset 80 bytes of it
2b04:  vstr d16, [r7, #64]        @ u64Pts  at +64
2b08:  strd r0, r1, [r7, #72]     @ u64Seq  at +72
```

`void *apvBuffer[8]` then `MI_U32 u32Byte[8]` fills 0..63, which puts the two
64-bit members exactly where the stores land. The second call site at 0x2bd8 does
the same for the echo-reference descriptor, so both outputs share the layout.

### What the exports say about the module

Only 22 functions, and **four of them are `movs r0, #0; bx lr`** — stubs that
report success and do nothing:

    MI_AI_OpenWithCfgFile   MI_AI_DupChnGroup   MI_AI_SetIfMute   MI_AI_GetIfMute

Worth more than a footnote, because a stub that returns `MI_SUCCESS` is worse than
a missing symbol: `dlsym` finds it and the caller cannot tell. `i6c_aud_load.h`
therefore binds neither mute-by-interface entry, and `hal_audio.c` mutes through
`MI_AI_SetMute` (the DPGA's, which is real).

Nothing in the export table matches `vqe`, `aenc`, `aed`, `iaa` or `src`. The
sound-quality algorithms, the audio encoder and the resampler are not absent
packs behind present entry points, as they are on MI 2.x — they are not in the
module.

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

divinus does reach this entry point, which an earlier revision of these notes
denied. It binds it as `fnAdjustChannelRotation` and declares the payload not as a
struct but as a bare `int *rotate`, which is why a search for a `i6c_scl_chn`
equivalent found nothing. Read for what it is, it corroborates the derivation
exactly: one word, and that word is the rotation. Wrapping it in a struct is this
repository's choice, made so the family reads uniformly and so a later member can
be appended without changing the call.

## ISP, from `mi_isp.ko` — the most forthcoming module of the set

The ISP is where this generation diverges hardest from MI 2.x: a stage with its own
device, channel and output ports, sitting between VIF and SCL, where MI 2.x folds
the same work into VPE as tuning calls. So none of these three layouts has an MI 2.x
counterpart to be sanity-checked against, and all three were derived rather than
adapted.

They also turned out to be the easiest in the family, because the module names its
own fields. Every member below is anchored to a string or a platform predicate in
`mi_isp.ko`, not inferred from a gap.

**`MI_ISP_ChannelAttr_t`, 80 bytes.** `MI_ISP_IMPL_CreateChannel` prints

    Dev%d, chn%d, sensorbindid %d, sync3a %d, ispinit version %d, size %d

and marshals those four struct arguments, in that order, from offsets **0, 76, 4
and 8**. That settles more than the field names. The version block starts at 4 as a
revision followed by a length, and `sync3a` sitting at 76 is what fixes the block's
payload at 64 bytes — 4 + 4 + 4 + 64 lands exactly on 76, with the 80 the ioctl
reports leaving room for nothing else. A nested array's length is normally the
hardest thing to reach from outside; here a neighbour's offset gives it away.

**`MI_ISP_ChnParam_t`, 20 bytes.** `MI_ISP_CHECK_ChnParamValid` is the pattern this
document keeps hoping for: it bounds each field and then hands it to the platform
predicate that names it.

| offset | width | bound | predicate |
| --- | --- | --- | --- |
| 0 | word | ≤ 4 | `MI_ISP_PLATFORM_SupportHdr` |
| 4 | word | ≤ 7 | `MI_ISP_PLATFORM_Support3DNR` |
| 8 | byte | ≤ 1 | `MI_ISP_PLATFORM_SupportMirror` |
| 9 | byte | ≤ 1 | `MI_ISP_PLATFORM_SupportFlip` |
| 12 | word | ≤ 3 | `MI_ISP_PLATFORM_SupportRot` |

Two of those bounds are independent confirmations rather than new facts. The ≤ 4 at
offset 0 is exactly the five values `i6c_common_hdr` declares, and the ≤ 7 at offset
4 reproduces upstream's bare comment "Accepts values from 0-7" — which divinus states
with no derivation behind it, and which the module now backs. A final call passes
offsets 4, 9 and 12 together into `RotFlipRelyOn3Dnr`, naming three at once, and
offset 16 is the only member left in a 20-byte struct.

**`MI_ISP_OutputPortParam_t`, 20 bytes.** `MI_ISP_IMPL_SetOutputPortParam` prints

    dev %d, chn %d, port%d, croprect(%d,%d,%d,%d), pixel %d Compress %d, layout %d

from four halfwords at 0, 2, 4 and 6 and words at 8, 12 and **16**.

That last one is the one loose end in the family. It is read with `ldr`, a full
word, against upstream's `char multiPlanes` — and the print is the only read of
offset 16 anywhere in the module, so there is nothing to break the tie. It does not
move the size: a `char` there carries three bytes of tail padding either way. The
name is kept as upstream and the ambiguity is recorded at the declaration.

The practical consequence is a rule rather than a question. **Clear these structs
before filling them.** If the driver reads a word where a caller wrote a byte, the
upper three bytes are whatever the stack held — and unlike a size error, that
misbehaves intermittently. divinus `memset`s this struct before use, which is
probably why the ambiguity never surfaced there.

### Where the ISP entry points hide the SoC id

Worth recording next to the layouts, because it is an arity trap of the kind this
family specialises in. `MI_ISP_EnableOutputPort` opens with

    mov.w ip, r0, lsr #16
    strh.w ip, [sp, #16]

so the SoC id is not a separate argument here as it is on MI_SYS and MI_RGN — it
rides in the **high halfword of the device argument** and the wrapper shifts it out.
On a single-die part both halves are zero and the distinction is invisible, which is
exactly what makes it worth writing down.

### The tuning API declares its own payload sizes

`MI_ISP_IQ_*` and `MI_ISP_AE_*` are not typed ioctls like the three layouts above.
Every one of them is the same wrapper: stack a 24-byte descriptor, fill in a payload
length and an api id, and call `MI_ISP_GENERAL_SetIspApiData` with the caller's
buffer. `MI_ISP_IQ_SetContrast` in full —

    push    {r4, lr}
    sub     sp, #24
    vldr    d16, [pc, #40]      @ the pair {0x18, 0x4c}
    str     r3, [sp, #12]       @ channel
    movw    r2, #0x1005         @ api id
    str     r4, [sp, #16]       @ device
    vstr    d16, [sp]           @ descriptor length, payload length
    str     r2, [sp, #8]
    blx     MI_ISP_GENERAL_SetIspApiData

The literal pool is the whole point. The first word is always `0x18`, the
descriptor's own length; the second is the **payload length** — what the library
copies into and out of the caller's buffer. So a module's size is stated outright by
the binary that will do the copying, with no measuring involved, and the `*Call`
variant states it a second time as the argument to its `calloc`.

That makes this the one corner of the family where the header and the blob can be
checked against each other cheaply, and for the modules `hal_isp.c` drives they
agree:

| module | api id | payload | vendor struct | manual offset |
| --- | --- | --- | --- | --- |
| ColorToGray | 0x1004 | 4 | 4 | — |
| Contrast | 0x1005 | 76 | 8 + 16×4 + 4 | 72 |
| Brightness | 0x1006 | 76 | 8 + 16×4 + 4 | 72 |
| Saturation | 0x100a | 416 | 8 + 16×24 + 24 | 392 |
| Defog | 0x100b | 28 | 8 + 16×1 + 1, padded | 24 |
| AE EvComp | 0x1403 | 8 | `{ MI_S32, MI_U32 }` | — |
| AE Flicker | 0x140e | 4 | bare enum | — |

Saturation is the row worth dwelling on, because the two derivations are genuinely
independent and land on the same number. From the headers, `SAT_LUT_X_NUM` is 5 and
`SAT_LUT_Y_NUM` is 6, so `MI_ISP_IQ_SaturationParam_t` is 1 + 5 + 6 + 5 + 6 + 1 = 24
bytes; with `MI_ISP_AUTO_NUM` at 16 that puts `stManual` at 8 + 384 = 392 and the
whole struct at 416. The wrapper says 416 without being asked.

**Eight of the ten modules carry Infinity6E's layout unchanged**, which is worth
stating precisely because nothing else in this directory does. The exceptions are the
two with large per-frequency tables: sharpness is 6264 bytes here against Infinity6E's
1268, and 3DNR 1912 against 1776. Both are excluded from the table rather than
extrapolated — their manual blocks are per-band arrays rather than a level, so there
is no single offset a scalar belongs at. Infinity6C reaches 3DNR through
`i6c_isp_para.level3DNR` instead, which the driver bounds at 7 and names itself.

Defog differs in kind rather than in size. On Infinity6E it is a bare 4-byte enable
with no auto/manual split; on maruko it is a full module carrying a strength byte, so
it has a manual offset here where Infinity6E's table needs none. The payload is 28
either way, which is how the difference stayed invisible until the interior mattered.
