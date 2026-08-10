/*
 * i6c_snr.h -- MI_SNR bindings, Infinity6C
 *
 * Vendored from OpenIPC divinus, src/hal/star/i6c_snr.h (MIT), which is why this
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

#ifndef SIGMASTAR_I6C_SNR_H
#define SIGMASTAR_I6C_SNR_H

#include "i6c_common.h"

typedef enum {
    I6C_SNR_HWHDR_NONE,
    I6C_SNR_HWHDR_SONY_DOL,
    I6C_SNR_HWHDR_DCG,
    I6C_SNR_HWHDR_EMBED_RAW8,
    I6C_SNR_HWHDR_EMBED_RAW10,
    I6C_SNR_HWHDR_EMBED_RAW12,
    I6C_SNR_HWHDR_EMBED_RAW14,
    I6C_SNR_HWHDR_EMBED_RAW16
} i6c_snr_hwhdr;

typedef struct {
    unsigned int laneCnt;
    unsigned int rgbFmtOn;
    unsigned int hsyncMode;
    unsigned int sampDelay;
    i6c_snr_hwhdr hwHdr;
    unsigned int virtChn;
    unsigned int packType[2];
} i6c_snr_mipi;

typedef struct {
    int vsyncInv;
    int hsyncInv;
    int pixclkInv;
    unsigned int vsyncDelay;
    unsigned int hsyncDelay;
    unsigned int pixclkDelay;
} i6c_snr_sync;

typedef struct {
    unsigned int multplxNum;
    i6c_snr_sync sync;
    i6c_common_edge edge;
    int bitswap;
} i6c_snr_bt656;

typedef struct {
    i6c_snr_sync sync;
} i6c_snr_par;

typedef union {
    i6c_snr_par parallel;
    i6c_snr_mipi mipi;
    i6c_snr_bt656 bt656;
} i6c_snr_intfattr;

typedef struct {
    unsigned int planeCnt;
    i6c_common_intf intf;
    i6c_common_hdr hdr;
    i6c_snr_intfattr intfAttr;
    char earlyInit;
} i6c_snr_pad;

typedef struct {
    unsigned int planeId;
    char sensName[32];
    i6c_common_rect capt;
    i6c_common_bayer bayer;
    i6c_common_prec precision;
    int hdrSrc;
    // Value in microseconds
    unsigned int shutter;
    // Value multiplied by 1024
    unsigned int sensGain;
    unsigned int compGain;
    i6c_common_pixfmt pixFmt;
} i6c_snr_plane;

typedef struct {
    i6c_common_rect crop;
    i6c_common_dim output;
    unsigned int maxFps;
    unsigned int minFps;
    char desc[32];
} __attribute__((packed, aligned(4))) i6c_snr_res;

#endif /* SIGMASTAR_I6C_SNR_H */
