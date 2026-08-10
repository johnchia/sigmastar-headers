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

All five have now been checked against divinus's vendored layouts, compiled for
the 32-bit ARM target, and **all five agree** — `i6c_sys_ver` 128,
`i6c_sys_bind` 16 (two of which plus two rates is BindChnPort2's 56),
`i6c_vif_grp` 28, `i6c_vif_dev` 20, `i6c_snr_plane` 72, `i6c_scl_port` 24. The
assertions live in the headers themselves, so the check runs on every build
rather than once.

Worth being exact about what that proves. A matching size says the field *set* is
right and nothing is missing or spurious; it does not say the fields are in the
right order, since permuting same-width members preserves the total. Only
`i6c_vif_grp` has had its order checked, via the bounds in
`MI_VIF_CHECK_GroupAttr`. For the others, size is necessary and not sufficient.

Not yet decomposed: `MI_VIF_OutputPortAttr_t` (payload 32, id words not yet
counted) and `MI_SCL_ChnParam_t` (`MI_SCL_SetChnParam` has no matching size slot,
so it is shaped differently).

## VENC disagrees, and divinus is the one that is wrong

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

divinus's `i6c_venc_chn` is **80**. It is `{ i6c_venc_attrib, i6c_venc_rate }`,
which measure 40 and 40, and one of them is four bytes too large.

**No assertion is added for this yet, deliberately** — a failing `_Static_assert`
would break every consumer's build over a defect in one struct nobody has
finished diagnosing. It is written down here instead, and `i6c_venc.h` carries the
warning at the declaration.

Which of the two is wrong is not yet established. `i6c_venc_attrib` is a codec
enum plus a union whose largest arm (`attr_h26x`) is 36, giving 40 with no slack.
`i6c_venc_rate` is a mode enum plus a union whose largest arm (`h26xvbr`) is 28,
which should give 32 rather than 40 — so `i6c_venc_rate` is the one to look at
first, and its union arms are where the extra words most likely are.

### The four bytes are not localized yet

`MI_VENC_SetRcParam` looked like the way to isolate the rate half, and is not: its
payload is 64, so the struct it carries is 56, and that is `MI_VENC_RcParam_t` --
a separate and larger thing than the rate attributes embedded in `ChnAttr`. No
wrapper marshals either half of `ChnAttr` on its own, so no size read can
separate them.

Localizing it needs `mi_venc.ko`, and **not** `mi_vcodec.ko`: that one is the
driver and MHAL layer and contains zero `MI_VENC_*` symbols. `mi_venc.ko` is the
MI layer, unstripped, 1816 symbols.

`MI_VENC_CHECK_ChnAttr` does not exist. VENC does not follow VIF's
`CHECK_`/`DEBUG_` naming, so that technique does not transfer -- it has
`MI_VENC_IOCTL_*` thunks over `MI_VENC_IMPL_*` bodies instead, plus a private
`_MI_VENC_IMPL_ConfigRcAttr`.

The lead to follow is narrower than a whole validator. The driver keeps the rate
attributes at **offset 252 of its internal channel context**, which is where both
`MI_VENC_IMPL_SetChnAttr` (0x12074) and the sole caller of `ConfigRcAttr`
(0x1b724) point with `add.w rN, r4, #252`. Immediately after computing it,
`MI_VENC_IMPL_SetChnAttr` calls `memcmp` against it (0x1207c) to decide whether
the rate changed:

    12074:  add.w r5, r4, #252    @ context's rate slot
    1207a:  mov   r1, r5
    1207c:  bl    memcmp

**That `memcmp` settles both open questions at once.** Its length argument is the
rate structure's true size, and its other pointer is the rate sub-struct inside
the caller's `ChnAttr`, so the offset used to form it is where the rate half
begins -- 36 means `i6c_venc_attrib` is four bytes too large, 40 means
`i6c_venc_rate` is. Read the instructions before 0x1207c that set up r0 and r2.

This is the validation earning its keep. VENC is the largest surface in the
family, and the first number checked against it did not match.

`MI_SYS_BindChnPort2` reports 56 and is not one struct: it packs two channel
ports plus the source and destination rates.

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

Field order and offsets are **not** derivable from these libraries. Every setter
and getter above copies the caller's struct wholesale — `ldmia`/`stmia` or
`memcpy` — and never touches a field, because the decoding happens on the other
side of the ioctl. The sizes are real; the interiors are opaque here.

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

`MI_VIF_DevAttr_t`, 20 bytes, from `MI_VIF_CHECK_DevAttr` (0x3788):

| offset | width | bound |
| --- | --- | --- |
| 0 | 32 | non-zero |
| 4 | — | not read here |
| 8 | 16 | — |
| 10 | 16 | — |
| 12 | 32 | < 3 |
| 16 | 8 | < 1, so boolean |
| — | | 3 bytes tail padding to 20 |

Offsets 8 and 10 being adjacent halfwords, with 4 and 6 untouched, fits an
8-byte rectangle at offset 4 whose width and height are validated and whose
origin is not — `MI_SYS_WindowRect_t` is four halfwords. Stated as the reading
it is, not as a fact: nothing here has confirmed 4 and 6 exist as separate
fields.

**These are offsets, not names.** Assigning names means correlating each failing
branch with the format string it prints, which the module makes possible — it
complains in terms of `IntfMode`, `HdrType`, `workmode` and `Clkedge` — but that
correlation has not been done, so no struct is declared from this yet.

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
