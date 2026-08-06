/*
 * i6_snr.h -- MI_SNR bindings, Infinity6E
 *
 * Vendored from OpenIPC divinus, src/hal/star/i6_snr.h (MIT).
 * See i6_common.h for why these declarations are vendored rather than derived.
 *
 * Note that MI_SNR is entirely index-based: resolutions are queried by count
 * and index, and no call anywhere in this API names a sensor. The sensor
 * identity is fixed when sensor_<name>_mipi.ko is insmod'd, so raptor's
 * sensor "discovery" is a module check plus the geometry queries below --
 * not the probing the Ingenic backend does.
 *
 * Copyright (c) 2024 OpenIPC
 * SPDX-License-Identifier: MIT
 */

#ifndef SIGMASTAR_I6_SNR_H
#define SIGMASTAR_I6_SNR_H

#include "i6_common.h"

typedef enum {
    I6_SNR_HWHDR_NONE,
    I6_SNR_HWHDR_SONY_DOL,
    I6_SNR_HWHDR_DCG,
    I6_SNR_HWHDR_EMBED_RAW8,
    I6_SNR_HWHDR_EMBED_RAW10,
    I6_SNR_HWHDR_EMBED_RAW12,
    I6_SNR_HWHDR_EMBED_RAW16
} i6_snr_hwhdr;

typedef struct {
    unsigned int laneCnt;
    unsigned int rgbFmtOn;
    i6_common_input input;
    unsigned int hsyncMode;
    unsigned int sampDelay;
    i6_snr_hwhdr hwHdr;
    unsigned int virtChn;
    unsigned int packType[2];
} i6_snr_mipi;

typedef struct {
    unsigned int multplxNum;
    i6_common_sync sync;
    i6_common_edge edge;
    int bitswap;
} i6_snr_bt656;

typedef struct {
    i6_common_sync sync;
} i6_snr_par;

typedef union {
    i6_snr_par parallel;
    i6_snr_mipi mipi;
    i6_snr_bt656 bt656;
} i6_snr_intfattr;

typedef struct {
    unsigned int planeCnt;
    i6_common_intf intf;
    i6_common_hdr hdr;
    i6_snr_intfattr intfAttr;
    char earlyInit;
} i6_snr_pad;

typedef struct {
    unsigned int planeId;
    char sensName[32];
    i6_common_rect capt;
    i6_common_bayer bayer;
    i6_common_prec precision;
    int hdrSrc;
    // Value in microseconds
    unsigned int shutter;
    // Value multiplied by 1024
    unsigned int sensGain;
    unsigned int compGain;
    i6_common_pixfmt pixFmt;
} i6_snr_plane;

typedef struct {
    i6_common_rect crop;
    i6_common_dim output;
    unsigned int maxFps;
    unsigned int minFps;
    char desc[32];
} __attribute__((packed, aligned(4))) i6_snr_res;

#endif /* SIGMASTAR_I6_SNR_H */
