/*
 * i6c_sys.h -- MI_SYS bindings, Infinity6C
 *
 * Vendored from OpenIPC divinus, src/hal/star/i6c_sys.h (MIT), which is why this
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

#ifndef SIGMASTAR_I6C_SYS_H
#define SIGMASTAR_I6C_SYS_H

#include "i6c_common.h"

#define I6C_SYS_API "1.0"

typedef enum {
    I6C_SYS_LINK_FRAMEBASE = 0x1,
    I6C_SYS_LINK_LOWLATENCY = 0x2,
    I6C_SYS_LINK_REALTIME = 0x4,
    I6C_SYS_LINK_AUTOSYNC = 0x8,
    I6C_SYS_LINK_RING = 0x10
} i6c_sys_link;

typedef enum {
    I6C_SYS_MOD_IVE,
    I6C_SYS_MOD_VDF,
    I6C_SYS_MOD_VENC,
    I6C_SYS_MOD_RGN,
    I6C_SYS_MOD_AI,
    I6C_SYS_MOD_AO,
    I6C_SYS_MOD_VIF,
    I6C_SYS_MOD_VPE,
    I6C_SYS_MOD_VDEC,
    I6C_SYS_MOD_SYS,
    I6C_SYS_MOD_FB,
    I6C_SYS_MOD_HDMI,
    I6C_SYS_MOD_DIVP,
    I6C_SYS_MOD_GFX,
    I6C_SYS_MOD_VDISP,
    I6C_SYS_MOD_DISP,
    I6C_SYS_MOD_OS,
    I6C_SYS_MOD_IAE,
    I6C_SYS_MOD_MD,
    I6C_SYS_MOD_OD,
    I6C_SYS_MOD_SHADOW,
    I6C_SYS_MOD_WARP,
    I6C_SYS_MOD_UAC,
    I6C_SYS_MOD_LDC,
    I6C_SYS_MOD_SD,
    I6C_SYS_MOD_PANEL,
    I6C_SYS_MOD_CIPHER,
    I6C_SYS_MOD_SNR,
    I6C_SYS_MOD_WLAN,
    I6C_SYS_MOD_IPU,
    I6C_SYS_MOD_MIPITX,
    I6C_SYS_MOD_GYRO,
    I6C_SYS_MOD_JPD,
    I6C_SYS_MOD_ISP,
    I6C_SYS_MOD_SCL,
    I6C_SYS_MOD_WBC,
    I6C_SYS_MOD_DSP,
    I6C_SYS_MOD_PCIE,
    I6C_SYS_MOD_DUMMY,
    I6C_SYS_MOD_NIR,
    I6C_SYS_MOD_DPU,
    I6C_SYS_MOD_END,
} i6c_sys_mod;

typedef enum {
    I6C_SYS_POOL_ENCODER_RING,
    I6C_SYS_POOL_CHANNEL,
    I6C_SYS_POOL_DEVICE,
    I6C_SYS_POOL_OUTPUT,
    I6C_SYS_POOL_DEVICE_RING
} i6c_sys_pooltype;

typedef struct {
    i6c_sys_mod module;
    unsigned int device;
    unsigned int channel;
    unsigned int port;
} i6c_sys_bind;

typedef struct {
    i6c_sys_mod module;
    unsigned int device;
    unsigned int channel;
    unsigned char heapName[32];
    unsigned int heapSize;
} i6c_sys_poolchn;

typedef struct {
    i6c_sys_mod module;
    unsigned int device;
    unsigned int reserved;
    unsigned char heapName[32];
    unsigned int heapSize;
} i6c_sys_pooldev;

typedef struct {
    unsigned int ringSize;
    unsigned char heapName[32];
} i6c_sys_poolenc;

typedef struct {
    i6c_sys_mod module;
    unsigned int device;
    unsigned int channel;
    unsigned int port;
    unsigned char heapName[32];
    unsigned int heapSize;
} i6c_sys_poolout;

typedef struct {
    i6c_sys_mod module;
    unsigned int device;
    unsigned short maxWidth;
    unsigned short maxHeight;
    unsigned short ringLine;
    unsigned char heapName[32];
} i6c_sys_poolring;

typedef struct {
    i6c_sys_pooltype type;
    char create;
    union {
        i6c_sys_poolchn channel;
        i6c_sys_pooldev device;
        i6c_sys_poolenc encode;
        i6c_sys_poolout output;
        i6c_sys_poolring ring;
    } config;
} i6c_sys_pool;

typedef struct {
    unsigned char version[128];
} i6c_sys_ver;

/*
 * Checked against the shipped libraries rather than taken on trust, per
 * DERIVED.md:
 *
 *   i6c_sys_ver     MI_SYS_GetVersion loads #128 into the size slot of the
 *                   block it hands to ioctl, so the buffer is 128 bytes.
 *   i6c_sys_bind    MI_SYS_BindChnPort2 marshals 56 bytes, which is two of
 *                   these plus the source and destination rates.
 *   i6c_sys_mod     ISP at 33 and SCL at 34 match the module ids the
 *                   libraries use.
 *
 * The SoC id every MI_SYS entry point leads with is 16 bits, and that is not an
 * assumption either: MI_SYS_GetVersion stores its first argument with strh.w, a
 * halfword store. MI 2.x has no equivalent argument, and dlsym resolves by name,
 * so a table built for that generation binds against these libraries and then
 * calls with a stray value where the id belongs, reporting nothing.
 */
_Static_assert(sizeof(i6c_sys_ver) == 128, "MI_SYS_GetVersion fills 128 bytes");
_Static_assert(sizeof(i6c_sys_bind) == 16, "two of these plus two rates is the 56 BindChnPort2 marshals");

/*
 * The pool descriptors are pinned by offset rather than by size, because
 * MI_SYS_ConfigPrivateMMAPool does not marshal this struct wholesale. It reads
 * named fields out of it and composes a differently shaped 68-byte block, so the
 * payload size says nothing about sizeof() here -- only the reads do:
 *
 *   ldr  r2, [r1]        the type, a word at 0
 *   ldrb r1, [r1, #4]    the create flag, a byte at 4
 *   ldrd r0, r1, [r4, #8]    the union at 8: module, then device
 *   ldr  r2, [r4, #16]       union + 8
 *   ldrh r3, [r4, #20]       union + 12, a halfword
 *   add.w r6, r4, #22 ; strlen   union + 14, the heap name
 *
 * That last offset is what settles the ring descriptor's shape. A name starting
 * at 14 leaves exactly three halfwords between the device and it, which is the
 * only arrangement that fits -- and the halfword load at union + 12 confirms one
 * of the three directly. Which of the two at union + 8 is the width and which the
 * height is not derived: a single word load covers both, and only the pair's
 * position is observable.
 *
 * The union's own size is not reachable this way either, since nothing copies it
 * whole. The arms are upstream's.
 */
_Static_assert(offsetof(i6c_sys_pool, create) == 4, "read with ldrb, one word past the type");
_Static_assert(offsetof(i6c_sys_pool, config) == 8, "every arm is read relative to here");
_Static_assert(offsetof(i6c_sys_poolring, device) == 4, "the second word of the ldrd at union + 8");
_Static_assert(offsetof(i6c_sys_poolring, maxWidth) == 8, "the word read at union + 8");
_Static_assert(offsetof(i6c_sys_poolring, ringLine) == 12, "the halfword read at union + 12");
_Static_assert(offsetof(i6c_sys_poolring, heapName) == 14, "where strlen is pointed, at union + 14");

#endif /* SIGMASTAR_I6C_SYS_H */
