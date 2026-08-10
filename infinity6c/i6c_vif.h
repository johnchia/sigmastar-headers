/*
 * i6c_vif.h -- MI_VIF bindings, Infinity6C
 *
 * Vendored from OpenIPC divinus, src/hal/star/i6c_vif.h (MIT), which is why this
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

#ifndef SIGMASTAR_I6C_VIF_H
#define SIGMASTAR_I6C_VIF_H

#include "i6c_common.h"

typedef enum {
    I6C_VIF_FRATE_FULL,
    I6C_VIF_FRATE_HALF,
    I6C_VIF_FRATE_QUART,
    I6C_VIF_FRATE_OCTANT,
    I6C_VIF_FRATE_3QUARTS,
    I6C_VIF_FRATE_END
} i6c_vif_frate;

typedef enum {
    I6C_VIF_CLK_12MHZ,
    I6C_VIF_CLK_18MHZ,
    I6C_VIF_CLK_27MHZ,
    I6C_VIF_CLK_36MHZ,
    I6C_VIF_CLK_54MHZ,
    I6C_VIF_CLK_108MHZ,
    I6C_VIF_CLK_END
} i6c_vif_clk;

typedef enum {
    I6C_VIF_WORK_1MULTIPLEX,
    I6C_VIF_WORK_2MULTIPLEX,
    I6C_VIF_WORK_4MULTIPLEX,
    I6C_VIF_WORK_END
} i6c_vif_work;

typedef struct {
    i6c_common_pixfmt pixFmt;
    i6c_common_rect crop;
    // Values 0-3 correspond to No, Top, Bottom, Both
    int field;
    char halfHScan;
} i6c_vif_dev;

typedef struct {
    i6c_common_intf intf;
    i6c_vif_work work;
    i6c_common_hdr hdr;
    i6c_common_edge edge;
    i6c_vif_clk clock;
    int interlaceOn;
    unsigned int grpStitch;
} i6c_vif_grp;

typedef struct {
    i6c_common_rect capt;
    i6c_common_dim dest;
    i6c_common_pixfmt pixFmt;
    i6c_vif_frate frate;
    i6c_common_compr compress;
} i6c_vif_port;

/*
 * Sizes checked against the shipped libraries, not taken on trust. Each
 * userspace wrapper writes its marshalled payload size into the ioctl block as a
 * literal; subtract the ids the call prepends and the remainder is the struct.
 *
 *   i6c_vif_grp   MI_VIF_CreateDevGroup marshals 32, less a 4-byte group id
 *   i6c_vif_dev   MI_VIF_SetDevAttr     marshals 24, less a 4-byte device id
 *
 * The group's field order is checked too, which size alone cannot do:
 * MI_VIF_CHECK_GroupAttr in mi_vif.ko bounds offset 0 below 5, 4 below 2, 8
 * below 4 and 12 below 2, which is the interface mode, work mode, HDR type and
 * clock edge in that order. See DERIVED.md.
 */
_Static_assert(sizeof(i6c_vif_grp) == 28, "MI_VIF_CreateDevGroup marshals 32 less a group id");
_Static_assert(sizeof(i6c_vif_dev) == 20, "MI_VIF_SetDevAttr marshals 24 less a device id");

#endif /* SIGMASTAR_I6C_VIF_H */
