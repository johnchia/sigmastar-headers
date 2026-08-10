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

**The immediate is not always a size.** `MI_VENC_CreateChn` yields 295, which is
not 4-aligned and cannot be a struct length. Confirm every number against the
copy in the same function before believing it.

## Sizes established this way

| struct | payload | id words | struct |
| --- | --- | --- | --- |
| `MI_SYS_Version_t` | 128 | 0 | **128** |
| `MI_VIF_GroupAttr_t` | 32 | 4 (group) | **28** |
| `MI_VIF_DevAttr_t` | 24 | 4 (dev) | **20** |
| `MI_SNR_PlaneInfo_t` | 80 | 8 (pad, plane) | **72** |
| `MI_SCL_OutputPortParam_t` | 36 | 12 (dev, chn, port) | **24** |

Not yet decomposed: `MI_VIF_OutputPortAttr_t` (payload 32, id words not yet
counted), `MI_SCL_ChnParam_t` (`MI_SCL_SetChnParam` has no matching size slot,
so it is shaped differently), and everything in VENC.

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
