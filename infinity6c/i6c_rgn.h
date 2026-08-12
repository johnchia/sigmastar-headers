/*
 * i6c_rgn.h -- MI_RGN (region/overlay) ABI for Infinity6C (MI 3.0)
 *
 * Counterpart to infinity6e/i6_rgn.h. Transcribed from the vendor
 * mi_rgn.h/mi_rgn_datatype.h (RGN major version 3) rather than adapted from the
 * MI 2.x header, because the display-attr struct is not the same shape and a
 * near-copy would have been wrong in a way that composites nothing rather than
 * failing a call.
 *
 * TWO DIFFERENCES FROM MI 2.x THAT MATTER.
 *
 * 1. Every entry point leads with a SoC id (MI_U16). MI_RGN and MI_SYS are the
 *    two modules that take it as a distinct argument rather than folding it into
 *    the high halfword of a device index; see infinity6c_state.h. So the loader's
 *    function pointers all gain a leading `unsigned short soc`.
 *
 * 2. The attach target is the standard MI module enum, not a private RGN one.
 *    MI 2.x's MI_RGN_AttachToChn took an id from RGN's own enum (VPE 0, VENC 2),
 *    which is i6_rgn.h's TRAP 1. MI 3.0's MI_RGN_ChnPort_t.eModId is a plain
 *    MI_ModuleId_e, and i6c_sys_mod is transcribed to those same values
 *    (VENC 2, RGN 3, SCL 34...), so an i6c_sys_bind IS an MI_RGN_ChnPort_t field
 *    for field and is reused as the attach target. hal_osd.c attaches to the
 *    VENC channel (I6C_SYS_MOD_VENC), which is what the working i6c reference
 *    does and what gives a main and its cascaded sub independent overlays --
 *    attaching at the shared SCL port would give them one.
 *
 * 3. The OSD display attr dropped i6_rgn.h's `invert` block and expresses the
 *    alpha choice as an enum + union rather than a bool + two overlaid arrays.
 *    Pixel-alpha (per-pixel, what antialiased text needs) is mode 0; the bg/fg
 *    pair lives in the union's ARGB1555 arm. See i6_rgn.h's ALPHA note in
 *    hal_osd.c for why const-alpha is the wrong mapping of a "global alpha".
 *
 * The pixel-format and region-type enum orderings are unchanged from MI 2.x, so
 * they read the same here; kept spelled out so the ordering stays checkable
 * against the vendor header.
 *
 * Copyright (C) 2026 Thingino Project
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SIGMASTAR_I6C_RGN_H
#define SIGMASTAR_I6C_RGN_H

#include "i6c_common.h"
#include "i6c_sys.h"

/* MI_RGN_HANDLE is MI_U32; a region is addressed by a bare handle, not a
 * device/channel pair. */

typedef enum {
    I6C_RGN_PIXFMT_ARGB1555,
    I6C_RGN_PIXFMT_ARGB4444,
    I6C_RGN_PIXFMT_I2,
    I6C_RGN_PIXFMT_I4,
    I6C_RGN_PIXFMT_I8,
    I6C_RGN_PIXFMT_RGB565,
    I6C_RGN_PIXFMT_ARGB8888,
    I6C_RGN_PIXFMT_END
} i6c_rgn_pixfmt;

typedef enum {
    I6C_RGN_TYPE_OSD,
    I6C_RGN_TYPE_COVER,
    I6C_RGN_TYPE_END
} i6c_rgn_type;

typedef struct {
    unsigned int width;
    unsigned int height;
} i6c_rgn_size;

/* MI_RGN_Attr_t = { type, { pixFmt, size } }. The nested OSD-init pair is
 * flattened here as it was in i6_rgn.h -- the layout is identical either way. */
typedef struct {
    i6c_rgn_type type;
    i6c_rgn_pixfmt pixFmt;
    i6c_rgn_size size;
} i6c_rgn_cnf;

typedef struct {
    i6c_rgn_pixfmt pixFmt;
    i6c_rgn_size size;
    void *data;
} i6c_rgn_bmp;

typedef struct {
    unsigned int x;
    unsigned int y;
} i6c_rgn_pnt;

/* MI_RGN_AlphaMode_e: 0 keeps the bitmap's per-pixel alpha, 1 replaces it with
 * one constant. Text wants 0. */
typedef enum {
    I6C_RGN_ALPHA_PIXEL = 0,
    I6C_RGN_ALPHA_CONST,
} i6c_rgn_alphamode;

typedef struct {
    unsigned char bgAlpha;
    unsigned char fgAlpha;
} i6c_rgn_bgfg;

typedef union {
    i6c_rgn_bgfg bgFgAlpha;
    unsigned char constAlpha;
} i6c_rgn_alphapara;

typedef struct {
    i6c_rgn_alphamode alphaMode;
    i6c_rgn_alphapara alphaPara;
} i6c_rgn_osdalpha;

typedef struct {
    unsigned int layer;
    i6c_rgn_osdalpha alpha;
} i6c_rgn_osd;

typedef struct {
    unsigned int layer;
    i6c_rgn_size size;
    unsigned int color;
} i6c_rgn_cov;

typedef struct {
    int show;
    i6c_rgn_pnt point;
    union {
        i6c_rgn_cov cover;
        i6c_rgn_osd osd;
    };
} i6c_rgn_chn;

typedef struct {
    unsigned char alpha;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} i6c_rgn_pale;

typedef struct {
    i6c_rgn_pale element[256];
} i6c_rgn_pal;

_Static_assert(sizeof(i6c_rgn_cnf) == 16, "i6c_rgn_cnf must be type + pixFmt + 2x u32 size");
_Static_assert(sizeof(i6c_rgn_bmp) == 16, "i6c_rgn_bmp must be pixFmt + 2x u32 size + pointer");
_Static_assert(sizeof(i6c_rgn_chn) == 28,
               "i6c_rgn_chn must match MI_RGN_ChnPortParam_t: show + point + 16-byte union");
_Static_assert(sizeof(i6c_rgn_pal) == 1024, "i6c_rgn_pal is 256 x 4-byte entries");

#endif /* SIGMASTAR_I6C_RGN_H */
