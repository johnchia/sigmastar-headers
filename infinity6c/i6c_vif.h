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
 * literal, and the module's ioctl thunk splits that payload itself -- so the ids
 * a call prepends are read off rather than counted:
 *
 *   i6c_vif_grp    CreateDevGroup     32, one id  (thunk: ldr [r4], #4)
 *   i6c_vif_dev    SetDevAttr         24, one id  (thunk: ldr [r4], #4)
 *   i6c_vif_port   SetOutputPortAttr  32, two ids (thunk: ldrd [r1], add #8)
 *
 * Field order is checked too, which size alone cannot do, since permuting
 * same-width members preserves a total.
 *
 * i6c_vif_grp: MI_VIF_CHECK_GroupAttr bounds offset 0 below 5, 4 below 2, 8
 * below 4 and 12 below 2, which is the interface mode, work mode, HDR type and
 * clock edge in that order.
 *
 * i6c_vif_dev: every member is named by what reads it. Offset 0 goes to
 * MI_VIF_PLATFORM_DevPixelSupport; MI_VIF_IMPL_SetDevAttr reads four halfwords
 * at 4, 6, 8 and 10, of which the validator passes 8 and 10 to
 * _MI_VIF_CHECK_RectValid, so the crop is a rect whose width and height are
 * checked and whose origin is not; offset 12 is bounded at or below 3 and
 * belongs to _MI_VIF_CHECK_FieldValid, whose four values the module names when it
 * complains that "Field is both"; and offset 16 is read as a single byte.
 * Nothing reads past 16, which is what makes the last three bytes padding.
 *
 * i6c_vif_port: MI_VIF_CHECK_PortAttr reads six halfwords at 0, 2, 4, 6, 8, 10
 * and three words at 12, 16 and 20 -- an 8-byte rect, a 4-byte dimension and
 * three enums, totalling the 24 derived above. It hands offset 12 to
 * MI_VIF_PLATFORM_OutputPortPixelSupport and offset 20 to
 * MI_VIF_PLATFORM_SupportCompress, naming those two.
 *
 * See DERIVED.md.
 */
_Static_assert(sizeof(i6c_vif_grp) == 28, "MI_VIF_CreateDevGroup marshals 32 less a group id");
_Static_assert(sizeof(i6c_vif_dev) == 20, "MI_VIF_SetDevAttr marshals 24 less a device id");
_Static_assert(sizeof(i6c_vif_port) == 24, "MI_VIF_SetOutputPortAttr marshals 32 less two ids");

_Static_assert(offsetof(i6c_vif_dev, crop) == 4, "IMPL_SetDevAttr reads the crop as halfwords from 4");
_Static_assert(offsetof(i6c_vif_dev, field) == 12, "the word _MI_VIF_CHECK_FieldValid bounds at 3");
_Static_assert(offsetof(i6c_vif_dev, halfHScan) == 16, "the last thing read, and read as a byte");
_Static_assert(offsetof(i6c_vif_port, dest) == 8, "the rect ends here and the dimension begins");
_Static_assert(offsetof(i6c_vif_port, pixFmt) == 12, "handed to PLATFORM_OutputPortPixelSupport");
_Static_assert(offsetof(i6c_vif_port, compress) == 20, "handed to PLATFORM_SupportCompress");

#endif /* SIGMASTAR_I6C_VIF_H */
