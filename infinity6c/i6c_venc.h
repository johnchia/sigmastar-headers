/*
 * i6c_venc.h -- MI_VENC bindings, Infinity6C
 *
 * Vendored from OpenIPC divinus, src/hal/star/i6c_venc.h (MIT), which is why this
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

#ifndef SIGMASTAR_I6C_VENC_H
#define SIGMASTAR_I6C_VENC_H

#include "i6c_common.h"

#define I6C_VENC_CHN_NUM 12
#define I6C_VENC_DEV_H26X_0 0
#define I6C_VENC_DEV_MJPG_0 8

typedef enum {
    I6C_VENC_CODEC_H264 = 2,
    I6C_VENC_CODEC_H265,
    I6C_VENC_CODEC_MJPG,
    I6C_VENC_CODEC_END
} i6c_venc_codec;

typedef enum {
    I6C_VENC_NALU_H264_PSLICE = 1,
    I6C_VENC_NALU_H264_ISLICE = 5,
    I6C_VENC_NALU_H264_SEI,
    I6C_VENC_NALU_H264_SPS,
    I6C_VENC_NALU_H264_PPS,
    I6C_VENC_NALU_H264_IPSLICE,
    I6C_VENC_NALU_H264_PREFIX = 14,
    I6C_VENC_NALU_H264_END
} i6c_venc_nalu_h264;

typedef enum {
    I6C_VENC_NALU_H265_PSLICE = 1,
    I6C_VENC_NALU_H265_ISLICE = 19,
    I6C_VENC_NALU_H265_VPS = 32,
    I6C_VENC_NALU_H265_SPS,
    I6C_VENC_NALU_H265_PPS,
    I6C_VENC_NALU_H265_SEI = 39,
    I6C_VENC_NALU_H265_END
} i6c_venc_nalu_h265;

typedef enum {
    I6C_VENC_NALU_MJPG_ECS = 5,
    I6C_VENC_NALU_MJPG_APP,
    I6C_VENC_NALU_MJPG_VDO,
    I6C_VENC_NALU_MJPG_PIC,
    I6C_VENC_NALU_MJPG_END
} i6c_venc_nalu_mjpg;

typedef enum {
    I6C_VENC_SRC_CONF_NORMAL,
    I6C_VENC_SRC_CONF_RING_ONE,
    I6C_VENC_SRC_CONF_RING_HALF,
    I6C_VENC_SRC_CONF_HW_SYNC,
    I6C_VENC_SRC_CONF_RING_DMA,
    I6C_VENC_SRC_CONF_END
} i6c_venc_src_conf;

typedef enum {
    I6C_VENC_RATEMODE_H264CBR = 1,
    I6C_VENC_RATEMODE_H264VBR,
    I6C_VENC_RATEMODE_H264ABR,
    I6C_VENC_RATEMODE_H264QP,
    I6C_VENC_RATEMODE_H264AVBR,
    I6C_VENC_RATEMODE_MJPGCBR,
    I6C_VENC_RATEMODE_MJPGVBR,
    I6C_VENC_RATEMODE_MJPGQP,
    I6C_VENC_RATEMODE_H265CBR,
    I6C_VENC_RATEMODE_H265VBR,
    I6C_VENC_RATEMODE_H265QP,
    I6C_VENC_RATEMODE_H265AVBR,
    I6C_VENC_RATEMODE_END
} i6c_venc_ratemode;

typedef enum {
    I6C_VENC_RATEMODE_UBR_H264CBR = 1,
    I6C_VENC_RATEMODE_UBR_H264VBR,
    I6C_VENC_RATEMODE_UBR_H264UBR,
    I6C_VENC_RATEMODE_UBR_H264ABR,
    I6C_VENC_RATEMODE_UBR_H264QP,
    I6C_VENC_RATEMODE_UBR_H264AVBR,
    I6C_VENC_RATEMODE_UBR_MJPGCBR,
    I6C_VENC_RATEMODE_UBR_MJPGVBR,
    I6C_VENC_RATEMODE_UBR_MJPGQP,
    I6C_VENC_RATEMODE_UBR_H265CBR,
    I6C_VENC_RATEMODE_UBR_H265VBR,
    I6C_VENC_RATEMODE_UBR_H265UBR,
    I6C_VENC_RATEMODE_UBR_H265QP,
    I6C_VENC_RATEMODE_UBR_H265AVBR,
    I6C_VENC_RATEMODE_UBR_END
} i6c_venc_ratemode_ubr;

typedef struct {
    unsigned int maxWidth;
    unsigned int maxHeight;
    unsigned int bufSize;
    unsigned int profile;
    char byFrame;
    unsigned int width;
    unsigned int height;
    unsigned int bFrameNum;
    unsigned int refNum;
} i6c_venc_attr_h26x;

typedef struct {
    unsigned int maxWidth;
    unsigned int maxHeight;
    unsigned int bufSize;
    char byFrame;
    unsigned int width;
    unsigned int height;
    char dcfThumbs;
    unsigned int markPerRow;
} i6c_venc_attr_mjpg;

typedef struct {
    i6c_venc_codec codec;
    union {
        i6c_venc_attr_h26x h264;
        i6c_venc_attr_mjpg mjpg;
        i6c_venc_attr_h26x h265;
    };
} i6c_venc_attrib;

typedef struct {
    unsigned int gop;
    unsigned int statTime;
    unsigned int fpsNum;
    unsigned int fpsDen;
    unsigned int bitrate;
    unsigned int avgLvl;
} i6c_venc_rate_h26xcbr;

typedef struct {
    unsigned int gop;
    unsigned int statTime;
    unsigned int fpsNum;
    unsigned int fpsDen;
    unsigned int maxBitrate;
    unsigned int maxQual;
    unsigned int minQual;
} i6c_venc_rate_h26xvbr;

typedef struct {
    unsigned int gop;
    unsigned int fpsNum;
    unsigned int fpsDen;
    unsigned int interQual;
    unsigned int predQual;
} i6c_venc_rate_h26xqp;

typedef struct {
    unsigned int gop;
    unsigned int statTime;
    unsigned int fpsNum;
    unsigned int fpsDen;
    unsigned int avgBitrate;
    unsigned int maxBitrate;
} i6c_venc_rate_h26xabr;

typedef struct {
    unsigned int bitrate;
    unsigned int fpsNum;
    unsigned int fpsDen;
} i6c_venc_rate_mjpgbr;

typedef struct {
    unsigned int fpsNum;
    unsigned int fpsDen;
    unsigned int quality;
} i6c_venc_rate_mjpgqp;

typedef struct {
    int mode;
    union {
        i6c_venc_rate_h26xcbr h264Cbr;
        i6c_venc_rate_h26xvbr h264Vbr;
        i6c_venc_rate_h26xqp h264Qp;
        i6c_venc_rate_h26xabr h264Abr;
        i6c_venc_rate_h26xvbr h264Avbr;
        i6c_venc_rate_mjpgbr mjpgCbr;
        i6c_venc_rate_mjpgbr mjpgVbr;
        i6c_venc_rate_mjpgqp mjpgQp;
        i6c_venc_rate_h26xcbr h265Cbr;
        i6c_venc_rate_h26xvbr h265Vbr;
        i6c_venc_rate_h26xqp h265Qp;
        i6c_venc_rate_h26xvbr h265Avbr;
    };
    void *extend;
} i6c_venc_rate;

typedef struct {
    i6c_venc_attrib attrib;
    i6c_venc_rate rate;
} i6c_venc_chn;

typedef struct {
    unsigned int maxWidth;
    unsigned int maxHeight;
} i6c_venc_init;

typedef struct {
    unsigned int quality;
    unsigned char qtLuma[64];
    unsigned char qtChroma[64];
    unsigned int mcuPerEcs;
} i6c_venc_jpg;

typedef union {
    i6c_venc_nalu_h264 h264Nalu;
    i6c_venc_nalu_mjpg mjpgNalu;
    i6c_venc_nalu_h265 h265Nalu;
} i6c_venc_nalu;

typedef struct {
    i6c_venc_nalu packType;
    unsigned int offset;
    unsigned int length;
    unsigned int sliceId;
} i6c_venc_packinfo;

typedef struct {
    unsigned long long addr;
    unsigned char *data;
    unsigned int length;
    unsigned long long timestamp;
    char endFrame;
    i6c_venc_nalu naluType;
    unsigned int offset;
    unsigned int packNum;
    unsigned char frameQual;
    int picOrder;
    unsigned int gradient;
    i6c_venc_packinfo packetInfo[8];
} i6c_venc_pack;

typedef struct {
    unsigned int leftPics;
    unsigned int leftBytes;
    unsigned int leftFrames;
    unsigned int leftMillis;
    unsigned int curPacks;
    unsigned int leftRecvPics;
    unsigned int leftEncPics;
    unsigned int fpsNum;
    unsigned int fpsDen;
    unsigned int bitrate;
} i6c_venc_stat;

typedef struct {
    unsigned int size;
    unsigned int skipMb;
    unsigned int ipcmMb;
    unsigned int iMb16x8;
    unsigned int iMb16x16;
    unsigned int iMb8x16;
    unsigned int iMb8x8;
    unsigned int pMb16;
    unsigned int pMb8;
    unsigned int pMb4;
    unsigned int refSliceType;
    unsigned int refType;
    unsigned int updAttrCnt;
    unsigned int startQual;
} i6c_venc_strminfo_h264;

typedef struct {
    unsigned int size;
    unsigned int iCu64x64;
    unsigned int iCu32x32;
    unsigned int iCu16x16;
    unsigned int iCu8x8;
    unsigned int pCu32x32;
    unsigned int pCu16x16;
    unsigned int pCu8x8;
    unsigned int pCu4x4;
    unsigned int refType;
    unsigned int updAttrCnt;
    unsigned int startQual;  
} i6c_venc_strminfo_h265;

typedef struct {
    unsigned int size;
    unsigned int updAttrCnt;
    unsigned int quality;
} i6c_venc_strminfo_mjpg;

typedef struct {
    i6c_venc_pack *packet;
    unsigned int count;
    unsigned int sequence;
    unsigned long handle;
    union
    {
        i6c_venc_strminfo_h264 h264Info;
        i6c_venc_strminfo_mjpg mjpgInfo;
        i6c_venc_strminfo_h265 h265Info;
    };
} i6c_venc_strm;

#endif /* SIGMASTAR_I6C_VENC_H */
