/*
 * i6c_isp.h -- MI_ISP bindings, Infinity6C
 *
 * Vendored from OpenIPC divinus, src/hal/star/i6c_isp.h (MIT), which is why this
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
 * On this generation the ISP is a pipeline stage of its own -- device, channel
 * and output ports, sitting between VIF and SCL -- rather than the set of tuning
 * calls folded into VPE that MI 2.x has. That is why this file has no MI 2.x
 * counterpart to be diffed against.
 *
 * Keep the i6c_* names and field order as upstream so fixes stay diffable
 * against divinus.
 *
 * Copyright (c) 2024 OpenIPC
 * SPDX-License-Identifier: MIT
 */

#ifndef SIGMASTAR_I6C_ISP_H
#define SIGMASTAR_I6C_ISP_H

#include "i6c_common.h"

typedef struct {
    unsigned int rev;
    unsigned int size;
    unsigned char data[64];
} i6c_isp_iqver;

typedef struct {
    unsigned int sensorId;
    i6c_isp_iqver iqVer;
    unsigned int sync3A;
} i6c_isp_chn;

typedef struct {
    i6c_common_hdr hdr;
    /* Accepts values from 0-7 */
    int level3DNR;
    char mirror;
    char flip;
    /* Represents 90-degree arcs */
    int rotate;
    char yuv2BayerOn;
} i6c_isp_para;

typedef struct {
    i6c_common_rect crop;
    i6c_common_pixfmt pixFmt;
    i6c_common_compr compress;
    char multiPlanes;
} i6c_isp_port;

/*
 * Sizes read off the ioctl payloads, with the id words taken from the module's
 * own thunks rather than counted:
 *
 *   i6c_isp_chn   CreateChannel        88, two ids   (ldrd [r4], struct at +8)
 *   i6c_isp_para  SetChnParam          28, two ids   (ldrd [r4], struct at +8)
 *   i6c_isp_port  SetOutputPortParam   32, three ids (ldr [r4], ldrd [r4, #4],
 *                                                    struct at +12)
 *
 * All three had their field order checked as well, which a size cannot do, and
 * every member below is named by something in mi_isp.ko rather than inferred.
 *
 * i6c_isp_chn: MI_ISP_IMPL_CreateChannel prints "sensorbindid %d, sync3a %d,
 * ispinit version %d, size %d" from offsets 0, 76, 4 and 8 in that argument
 * order. So the sensor id leads, the version block follows at 4 as a revision
 * then a length, and sync3A is the last word -- which is what fixes the version
 * block's payload at 64 bytes, since 4 + 8 + 64 lands exactly on 76.
 *
 * i6c_isp_para: MI_ISP_CHECK_ChnParamValid bounds each field and then hands it
 * to the platform predicate that names it. Offset 0 is a word bounded at or
 * below 4 and goes to MI_ISP_PLATFORM_SupportHdr, which matches the five values
 * i6c_common_hdr has; offset 4 is a word bounded at or below 7 and goes to
 * Support3DNR, matching upstream's "values from 0-7" exactly; offsets 8 and 9
 * are read with ldrb and bounded at or below 1, going to SupportMirror and
 * SupportFlip; offset 12 is a word bounded at or below 3 and goes to SupportRot,
 * which is the four quadrants. A last call passes offsets 4, 9 and 12 together
 * to RotFlipRelyOn3Dnr, naming three of them at once.
 *
 * i6c_isp_port: MI_ISP_IMPL_SetOutputPortParam prints "croprect(%d,%d,%d,%d),
 * pixel %d Compress %d, layout %d" from four halfwords at 0, 2, 4 and 6 and
 * words at 8, 12 and 16.
 *
 * One caveat on that last member. The only read of offset 16 found anywhere is
 * that print, and it is a full word (ldr) against upstream's char -- so whether
 * the field is one byte or four is unresolved. It does not move the size either
 * way, since a char there is followed by three bytes of tail padding, and the
 * name is kept as upstream. It does mean a caller must clear the struct before
 * filling it: write only the low byte and the driver may read three bytes of
 * whatever was on the stack.
 *
 * See DERIVED.md.
 */
_Static_assert(sizeof(i6c_isp_chn) == 80, "MI_ISP_CreateChannel marshals 88 less two ids");
_Static_assert(sizeof(i6c_isp_para) == 20, "MI_ISP_SetChnParam marshals 28 less two ids");
_Static_assert(sizeof(i6c_isp_port) == 20, "MI_ISP_SetOutputPortParam marshals 32 less three ids");

_Static_assert(sizeof(i6c_isp_iqver) == 72, "sync3A at 76 leaves exactly this much from offset 4");
_Static_assert(offsetof(i6c_isp_chn, iqVer) == 4, "printed as the version, after the sensor id");
_Static_assert(offsetof(i6c_isp_chn, sync3A) == 76, "the last word, printed second of the four");
_Static_assert(offsetof(i6c_isp_iqver, size) == 4, "printed as the size, one word past the revision");

_Static_assert(offsetof(i6c_isp_para, level3DNR) == 4, "the word Support3DNR bounds at 7");
_Static_assert(offsetof(i6c_isp_para, mirror) == 8, "read with ldrb, handed to SupportMirror");
_Static_assert(offsetof(i6c_isp_para, flip) == 9, "read with ldrb, handed to SupportFlip");
_Static_assert(offsetof(i6c_isp_para, rotate) == 12, "the word SupportRot bounds at 3");
_Static_assert(offsetof(i6c_isp_para, yuv2BayerOn) == 16, "the only member left, and the struct is 20");

_Static_assert(offsetof(i6c_isp_port, pixFmt) == 8, "printed as the pixel format, after the crop");
_Static_assert(offsetof(i6c_isp_port, compress) == 12, "printed as Compress");
_Static_assert(offsetof(i6c_isp_port, multiPlanes) == 16, "printed as the layout");

#endif /* SIGMASTAR_I6C_ISP_H */
