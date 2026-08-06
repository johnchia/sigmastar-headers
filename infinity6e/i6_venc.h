/*
 * i6_venc.h -- MI_VENC bindings, Infinity6E
 *
 * Vendored from OpenIPC divinus, src/hal/star/i6_venc.h (MIT).
 * See i6_common.h for why these declarations are vendored rather than derived.
 *
 * Rate-control mode numbering differs between the original Infinity6
 * (series 0xEF, I6OG_VENC_RATEMODE_*) and everything later, because 0xEF
 * has no UBR modes: its H264UBR slot does not exist, so everything from
 * MJPEGCBR onward sits one lower than on 0xF1. Infinity6E is 0xF1, so use
 * the plain I6_VENC_RATEMODE_* set; the I6OG_ enum is kept only to stay
 * diffable against upstream, which supports both.
 *
 * I6_VENC_CHN_NUM below is where hal_caps.c's INFINITY6E max_enc_channels
 * comes from -- 9 addressable channels, capped to RSS_MAX_ENC_CHANNELS.
 *
 * Copyright (c) 2024 OpenIPC
 * SPDX-License-Identifier: MIT
 */

#ifndef SIGMASTAR_I6_VENC_H
#define SIGMASTAR_I6_VENC_H

#include "i6_common.h"

#define I6_VENC_CHN_NUM 9

typedef enum {
    I6_VENC_CODEC_H264 = 2,
    I6_VENC_CODEC_H265,
    I6_VENC_CODEC_MJPG,
    I6_VENC_CODEC_END
} i6_venc_codec;

typedef enum {
    I6_VENC_NALU_H264_PSLICE = 1,
    I6_VENC_NALU_H264_ISLICE = 5,
    I6_VENC_NALU_H264_SEI,
    I6_VENC_NALU_H264_SPS,
    I6_VENC_NALU_H264_PPS,
    I6_VENC_NALU_H264_IPSLICE,
    I6_VENC_NALU_H264_PREFIX = 14,
    I6_VENC_NALU_H264_END
} i6_venc_nalu_h264;

typedef enum {
    I6_VENC_NALU_H265_PSLICE = 1,
    I6_VENC_NALU_H265_ISLICE = 19,
    I6_VENC_NALU_H265_VPS = 32,
    I6_VENC_NALU_H265_SPS,
    I6_VENC_NALU_H265_PPS,
    I6_VENC_NALU_H265_SEI = 39,
    I6_VENC_NALU_H265_END
} i6_venc_nalu_h265;

typedef enum {
    I6_VENC_NALU_MJPG_ECS = 5,
    I6_VENC_NALU_MJPG_APP,
    I6_VENC_NALU_MJPG_VDO,
    I6_VENC_NALU_MJPG_PIC,
    I6_VENC_NALU_MJPG_END
} i6_venc_nalu_mjpg;

typedef enum {
    I6_VENC_SRC_CONF_NORMAL,
    I6_VENC_SRC_CONF_RING_ONE,
    I6_VENC_SRC_CONF_RING_HALF,
    I6_VENC_SRC_CONF_END
} i6_venc_src_conf;

typedef enum {
    I6_VENC_RATEMODE_H264CBR = 1,
    I6_VENC_RATEMODE_H264VBR,
    I6_VENC_RATEMODE_H264FIXQP,
    I6_VENC_RATEMODE_H264AVBR,
    I6_VENC_RATEMODE_H264UBR,
    I6_VENC_RATEMODE_MJPGCBR,
    I6_VENC_RATEMODE_MJPGFIXQP,
    I6_VENC_RATEMODE_H265CBR,
    I6_VENC_RATEMODE_H265VBR,
    I6_VENC_RATEMODE_H265FIXQP,
    I6_VENC_RATEMODE_H265AVBR,
    I6_VENC_RATEMODE_H265UBR,
    I6_VENC_RATEMODE_END
} i6_venc_ratemode;

typedef enum {
    I6OG_VENC_RATEMODE_H264CBR = 1,
    I6OG_VENC_RATEMODE_H264VBR,
    I6OG_VENC_RATEMODE_H264QP,
    I6OG_VENC_RATEMODE_H264AVBR,
    I6OG_VENC_RATEMODE_MJPGCBR,
    I6OG_VENC_RATEMODE_MJPGQP,
    I6OG_VENC_RATEMODE_H265CBR,
    I6OG_VENC_RATEMODE_H265VBR,
    I6OG_VENC_RATEMODE_H265QP,
    I6OG_VENC_RATEMODE_H265AVBR,
    I6OG_VENC_RATEMODE_END
} i6og_venc_ratemode;

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
} i6_venc_attr_h26x;

typedef struct {
    unsigned int maxWidth;
    unsigned int maxHeight;
    unsigned int bufSize;
    char byFrame;
    unsigned int width;
    unsigned int height;
    char dcfThumbs;
    unsigned int markPerRow;
} i6_venc_attr_mjpg;

typedef struct {
    i6_venc_codec codec;
    union {
        i6_venc_attr_h26x h264;
        i6_venc_attr_mjpg mjpg;
        i6_venc_attr_h26x h265;
    };
} i6_venc_attrib;

typedef struct {
    unsigned int gop;
    unsigned int statTime;
    unsigned int fpsNum;
    unsigned int fpsDen;
    unsigned int bitrate;
    unsigned int avgLvl;
} i6_venc_rate_h26xcbr;

typedef struct {
    unsigned int gop;
    unsigned int statTime;
    unsigned int fpsNum;
    unsigned int fpsDen;
    unsigned int maxBitrate;
    unsigned int maxQual;
    unsigned int minQual;
} i6_venc_rate_h26xvbr;

typedef struct {
    unsigned int gop;
    unsigned int fpsNum;
    unsigned int fpsDen;
    unsigned int interQual;
    unsigned int predQual;
} i6_venc_rate_h26xqp;

typedef struct {
    unsigned int bitrate;
    unsigned int fpsNum;
    unsigned int fpsDen;
} i6_venc_rate_mjpgcbr;

typedef struct {
    unsigned int fpsNum;
    unsigned int fpsDen;
    unsigned int quality;
} i6_venc_rate_mjpgqp;

typedef struct {
    i6_venc_ratemode mode;
    union {
        i6_venc_rate_h26xcbr h264Cbr;
        i6_venc_rate_h26xvbr h264Vbr;
        i6_venc_rate_h26xqp h264Qp;
        i6_venc_rate_h26xvbr h264Avbr;
        i6_venc_rate_mjpgcbr mjpgCbr;
        i6_venc_rate_mjpgqp mjpgQp;
        i6_venc_rate_h26xcbr h265Cbr;
        i6_venc_rate_h26xvbr h265Vbr;
        i6_venc_rate_h26xqp h265Qp;
        i6_venc_rate_h26xvbr h265Avbr;
    };
    void *extend;
} i6_venc_rate;

typedef struct {
    i6_venc_attrib attrib;
    i6_venc_rate rate;
} i6_venc_chn;

typedef struct {
    unsigned int quality;
    unsigned char qtLuma[64];
    unsigned char qtChroma[64];
    unsigned int mcuPerEcs;
} i6_venc_jpg;

typedef union {
    i6_venc_nalu_h264 h264Nalu;
    i6_venc_nalu_mjpg mjpgNalu;
    i6_venc_nalu_h265 h265Nalu;
} i6_venc_nalu;

typedef struct {
    i6_venc_nalu packType;
    unsigned int offset;
    unsigned int length;
} i6_venc_packinfo;

/* A trailing u32SliceId here is MI 3.0 (it appears in SSC377's
 * mi_venc_datatype.h and in neither Infinity6E drop). Both divinus and
 * waybeam carry it, because both are multi-chip; that is why their
 * agreement is not evidence for this struct. At 16 bytes rather than 12 a
 * consumer walks packetInfo[] at the wrong stride and reads a neighbouring
 * offset or length as a NAL type, and i6_venc_pack inherits the error. */
_Static_assert(sizeof(i6_venc_packinfo) == 12,
               "i6_venc_packinfo is 12 bytes; a u32SliceId here is MI 3.0");

typedef struct {
    unsigned long long addr;
    unsigned char *data;
    unsigned int length;
    unsigned long long timestamp;
    char endFrame;
    i6_venc_nalu naluType;
    unsigned int offset;
    unsigned int packNum;
    i6_venc_packinfo packetInfo[8];
} i6_venc_pack;

_Static_assert(sizeof(i6_venc_pack) == 136, "i6_venc_pack must match MI_VENC_Stream_t's 136 bytes");

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
} i6_venc_stat;

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
} i6_venc_strminfo_h264;

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
} i6_venc_strminfo_h265;

typedef struct {
    unsigned int size;
    unsigned int updAttrCnt;
    unsigned int quality;
} i6_venc_strminfo_mjpg;

typedef struct {
    i6_venc_pack *packet;
    unsigned int count;
    unsigned int sequence;
    int handle;
    union {
        i6_venc_strminfo_h264 h264Info;
        i6_venc_strminfo_mjpg mjpgInfo;
        i6_venc_strminfo_h265 h265Info;
    };
} i6_venc_strm;

#endif /* SIGMASTAR_I6_VENC_H */
