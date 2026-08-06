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

## Declarations only

MI is reached by `dlopen`/`dlsym` rather than by linking `-lmi_*`, so a consumer
binds to whatever libraries the device itself carries. Nothing here includes
`dlfcn.h` or depends on a logger or an error-code set; the loaders that do that
belong to the consumer. Every header compiles standalone under
`-std=c11 -Wall -Wextra -Werror`.

Where a comment refers to `hal_isp.c`, `hal_caps.c` or similar, it means the
consumer these were extracted from — [raptor-hal](https://github.com/gtxaspec/raptor-hal)'s
SigmaStar backend.

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
