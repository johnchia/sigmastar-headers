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
 * MI_SCL_SetOutputPortParam marshals 36 bytes, less three 4-byte ids for the
 * device, channel and port. Size only -- field order unchecked. See DERIVED.md.
 */
_Static_assert(sizeof(i6c_scl_port) == 24, "MI_SCL_SetOutputPortParam marshals 36 less three ids");

#endif /* SIGMASTAR_I6C_SCL_H */
