# sigmastar-headers

## Overview

Programming headers for the SigmaStar MI (Multimedia Interface) libraries found
on Infinity-series camera SoCs. Companion to
[ingenic-headers](https://github.com/gtxaspec/ingenic-headers), same purpose and
the same layout convention.

## Families supported

| directory | chips | MI series |
| --- | --- | --- |
| `infinity6e` | SSC30KQ, SSC336Q, SSC338Q, SSC339G | 0xF1 |
| `infinity6c` | SSC377, SSC378, SSC379 | MI 3.0 |

`infinity6b0` (SSC333/335/337) has no directory of its own: it is the same MI
generation as `infinity6e` and consumers point it at that family.

`infinity6c` is a different generation and shares no layout with either, even
where a function signature is unchanged. Its whole module surface leads with
arguments MI 2.x does not have — a SoC id on MI_SYS and MI_RGN, a device on
MI_VENC — and `dlsym` resolves by name, so a consumer that reuses MI 2.x
declarations here links cleanly and then calls with the wrong argument list.
Nothing reports it.

## These are reconstructions, not vendor headers

SigmaStar ships no redistributable headers. What is here was reconstructed from
the ABI of prebuilt vendor `.so` files, vendored from
[OpenIPC divinus](https://github.com/OpenIPC/divinus) (`src/hal/star/i6_*.h`,
MIT) and corroborated field by field against `waybeam_venc`'s independently
maintained `sigmastar_types.h`, then validated struct by struct against a real
SigmaStar Alkaid SDK's `mi_*_datatype.h`.

That last step matters and is worth repeating for anything added here: a wrong
layout does not fail to compile, it corrupts memory. Several structs that two
transcriptions agreed on were still wrong, because both descend from
multi-chip HALs that carry MI 3.0 fields into MI 2.x structs — agreement
between them is a hypothesis, not evidence.

Names and field order follow divinus so fixes stay diffable against upstream.

### `infinity6c` is vendored the same way, and validated harder

Same method as the family beside it: divinus's `src/hal/star/i6c_*.h` vendored
under its MIT licence, reworked to stand alone, and then checked struct by struct
against the binaries rather than assumed. There is no waybeam transcription for
this generation to corroborate against, so the validation leans entirely on the
shipped artifacts — which turns out to be a stronger check than a second
reconstruction. Every layout checked so far has held.

MI marshals through an ioctl, and that makes the artifacts unusually
forthcoming. Each userspace wrapper writes the size of the payload it is about to
hand over into its argument block as a literal, so a struct size can be read off
the disassembly:

```
$ arm-linux-*-objdump -d --disassemble=MI_SYS_GetVersion libmi_sys.so
  movs r3, #128        @ payload size
  str  r3, [sp, #4]    @ into the size slot of the ioctl block
```

Better still, the per-module kernel objects are **unstripped**. `mi_vif.ko`
exports 2574 symbols, and `MI_VIF_CHECK_*` validates fields one at a time — so
its loads give offsets, the load instruction gives each field's width, and the
immediate it compares against gives the valid range.

`infinity6c/DERIVED.md` records what has been through that, what it cost, and the
ways the method misleads. Two worth knowing before trusting a number. The size
slot describes the *marshalled block* rather than the struct, so the device and
channel ids a call prepends have to be subtracted — `MI_VIF_SetDevAttr` reports 24
and copies 20. And the slot is not at a fixed offset in the frame: it is at
`[sp, #4]` for SYS, VIF, SNR and SCL but `[sp, #28]` for `MI_VENC_CreateChn`, so a
script that pattern-matches the offset reports whatever else lives there. Read the
size out of the ioctl command word's `_IOC` field instead, which needs no
assumption about the frame.

Two layouts have had their field *order* checked and not merely their size, and
between them they make the case for checking rather than trusting either source.
The VIF group attribute's size and four field bounds came out of the binaries,
divinus supplied the order, and divinus's ordering reproduced bounds it had no
access to — where an earlier guess from the bounds alone had two of those fields
the wrong way round. The VENC channel attribute went further: the driver rewrites
just the fields a running channel may change before re-comparing, and the offsets
it rewrites land on the right fields in *both* union arms, including the MJPG arm
where the absence of a profile field shifts width and height four bytes earlier.
Sizes cannot show that, since permuting same-width members preserves a total.

## The assertions are 32-bit facts

Compile these headers with the cross compiler, including when all you want is to
check them. The `_Static_assert`s pin the ABI of a 32-bit ARM target, so any struct
containing a pointer or a `long` measures differently on an LP64 host: four of the
nine `infinity6e` headers do not compile for x86-64, and neither does
`infinity6c/i6c_venc.h`, whose channel attribute measures 80 there against 76 on
the target.

That last one is worth singling out, because a host `sizeof` once produced a
four-byte disagreement with the libraries that did not exist, and nothing about
the symptom suggests a cross-compilation question. It is the one trap in this
repository that lives in the measuring rather than in the artifact being measured.

## Layout, and why there is no SDK version in the path

ingenic-headers keys on `<PLATFORM>/<VERSION>/<LANG>` because it archives real
vendor drops and one checkout must serve several platforms at once, each wanting
a different SDK. Neither applies here: these are reconstructions rather than
drops, and a consumer pins this repository by commit, which is the version axis.

That is not to say SDK skew does not exist. It does, and it bites — between the
2021-09-02 and 2022-06-01 Alkaid releases `MI_VPE_SensorChannel_e` turned from a
sequential enum into a bitmask, moving `SENSOR2` from 3 to 4 and `SENSOR3` from
4 to 8. Everything else added between those releases was appended before the
`_MAX`, so a header captured against the older drop is otherwise usable against
the newer libraries, but not the reverse. If one family ever needs two
incompatible generations side by side, that is the point to add a version
segment under the family directory.

## Fixes

Headers here have been corrected where the reconstruction was wrong. The commit
history is the record; each fix names the vendor struct it was checked against.

## Contributing

Corrections welcome, particularly for other Infinity families. Please say what a
change was verified against — a vendor header, a disassembly, or hardware — since
a plausible-looking layout is the failure mode this repository exists to avoid.

## Disclaimer

These declarations describe the interface of proprietary libraries so that
independent software can call them. They contain no vendor code. Be mindful of
any licensing restrictions that apply to the libraries themselves.
