/*
 * i6c_scl.h -- MI_SCL bindings, Infinity6C
 *
 * Vendored from OpenIPC divinus, src/hal/star/i6c_scl.h (MIT), which is why this
 * file carries divinus's licence. Reworked to stand alone: divinus's internal
 * includes are dropped, an include guard replaces #pragma once, and the
 * dlopen/dlsym function table stays with the consumer, since anything naming a
 * logger or an error-code set belongs on that side of the boundary.
 *
 * These layouts describe the ABI of prebuilt vendor .so files. divinus's are
 * exercised on this silicon, which is worth more than freshly derived ones --
 * but it is a reconstruction from a multi-chip HAL, so treat it as a hypothesis
 * and check it. DERIVED.md carries the sizes and field bounds read out of the
 * shipped libraries and kernel objects for exactly that, and records which
 * structs have been through it.
 *
 * Keep the i6c_* names and field order as upstream so fixes stay diffable
 * against divinus.
 *
 * Copyright (c) 2024 OpenIPC
 * SPDX-License-Identifier: MIT
 */

#ifndef SIGMASTAR_I6C_SCL_H
#define SIGMASTAR_I6C_SCL_H

#include "i6c_common.h"

typedef enum {
    I6C_SCL_BIND_INVALID,
    I6C_SCL_BIND_ID0 = 0x1,
    I6C_SCL_BIND_ID1 = 0x2,
    I6C_SCL_BIND_ID2 = 0x4,
    I6C_SCL_BIND_ID3 = 0x8,
    I6C_SCL_BIND_ID4 = 0x10,
    I6C_SCL_BIND_ID5 = 0x20,
    I6C_SCL_BIND_ID6 = 0x40,
    I6C_SCL_BIND_ID7 = 0x80,
    I6C_SCL_BIND_ID8 = 0x100,
    I6C_SCL_BIND_END = 0xffff,
} i6c_scl_bind;

typedef struct {
    i6c_common_rect crop;
    i6c_common_dim output;
    char mirror;
    char flip;
    i6c_common_pixfmt pixFmt;
    i6c_common_compr compress;
} i6c_scl_port;

/*
 * Rotation. divinus reaches this entry point too, but declares the payload as a
 * bare int * rather than as a struct, so there was no type to vendor -- only a
 * corroboration that the payload is a single word, and that the word is the
 * rotation. The struct is this repository's own, for uniformity with the rest of
 * the family.
 *
 * What the binaries fix is the count and the zero case, not the angles.
 * MI_SCL_CHECK_ChnParam accepts zero immediately and rejects anything above 3, so
 * there are four values and the first is "no rotation"; the module calls the field
 * rot. The order of the remaining three is the vendor's usual ascending one and is
 * *not* derived -- getting it wrong turns a picture, which is visible and
 * harmless, unlike a layout error.
 */
typedef enum {
    I6C_SCL_ROTATE_NONE,
    I6C_SCL_ROTATE_90,
    I6C_SCL_ROTATE_180,
    I6C_SCL_ROTATE_270,
    I6C_SCL_ROTATE_END
} i6c_scl_rotate;

typedef struct {
    i6c_scl_rotate rotate;
} i6c_scl_chn;

/*
 * Sizes read off the ioctl payloads, with the id words taken from the module's own
 * thunks rather than counted:
 *
 *   i6c_scl_port  SetOutputPortParam  36, three ids (ldr [r4], ldrd [r4, #4],
 *                                                   struct at +12)
 *   i6c_scl_chn   SetChnParam         12, two ids   (ldrd [r4], struct at +8)
 *
 * Neither struct's field order is checked. The port's four bytes of scalars
 * between two shaped members make it the more likely of the two to be wrong, and
 * the channel param is a single word, so there is nothing to permute.
 *
 * See DERIVED.md.
 */
_Static_assert(sizeof(i6c_scl_port) == 24, "MI_SCL_SetOutputPortParam marshals 36 less three ids");
_Static_assert(sizeof(i6c_scl_chn) == 4, "MI_SCL_SetChnParam marshals 12 less two ids");

#endif /* SIGMASTAR_I6C_SCL_H */
