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

/*
 * MI_VENC_ChnAttr_t is checked against mi_venc.ko rather than taken on trust,
 * per DERIVED.md, and both halves come out as divinus has them.
 *
 * MI_VENC_IMPL_SetChnAttr copies the incoming attribute in two pieces and
 * memcmps each against the channel context's own copy, which fixes both halves'
 * sizes and the boundary between them: 40 bytes from offset 0, then 36 from
 * offset 40. _MI_VENC_IMPL_ConfigRcAttr agrees, reaching the rate half by adding
 * 40 to the whole attribute it is handed. The 76 that makes is what
 * MI_VENC_CreateChn marshals.
 *
 * The interior offsets fall out of the same comparison. A change to the codec
 * half is refused unless it is confined to resolution and profile, which the
 * driver overwrites with the channel's current values before comparing -- at
 * offsets 16, 24 and 28 for H264 and H265, and at 20 and 24 for MJPG. That
 * reproduces both union arms, MJPG included: it has no profile field, so its
 * width and height sit four bytes earlier than H26x's.
 *
 * The rest of the module is checked the same way, and also comes out as divinus
 * has it. The ioctl thunks in mi_venc.ko are the shortcut: each one splits the
 * marshalled payload itself, so the id words a call prepends can be read off
 * rather than counted. MI_VENC_IOCTL_InitDev takes one id and puts the struct at
 * payload+4 of 12; the GetStream, GetChnAttr, SetChnAttr and Query thunks take
 * two and put it at payload+8; and MI_VENC_IOCTL_GetStream fetches its timeout
 * from payload+80, which brackets MI_VENC_Stream_t between 8 and 80.
 *
 * MI_VENC_Pack_t is never marshalled -- the stream struct points at an array the
 * caller owns -- so its size comes from the stride the library indexes that array
 * with, 184, and the offsets from the fields it writes. i6c_venc_pack measuring
 * 180 of payload in a 184-byte footprint is not slack: the u64 members align it
 * to 8, and 184 is what the stride confirms.
 *
 * The three stream-info arms are each confirmed at one interior offset, and the
 * offsets are the load-bearing part. The same 3-bit reference type is written to
 * strm+60 for H264 and strm+52 for H265, which is refType's slot in each arm and
 * differs by exactly the eight bytes the H264 arm is longer; MJPG's quality lands
 * at strm+24, its arm's offset 8. Only the H264 arm's *size* is pinned, as the
 * largest and therefore the union's own size -- a shorter arm's size is not
 * observable, so h265 at 48 and mjpg at 12 remain bounded rather than fixed.
 *
 * Unchecked: every enumeration except i6c_venc_codec, whose 2..4 the driver
 * bounds explicitly.
 *
 * These sizes are the 32-bit target ABI, and i6c_venc_rate and i6c_venc_pack
 * contain pointers, so a host compiler measures them over and none of the totals
 * here hold. Compile the asserts with the cross compiler; see the README.
 */
_Static_assert(sizeof(i6c_venc_attrib) == 40, "MI_VENC_Attr_t is the 40 bytes SetChnAttr compares");
_Static_assert(sizeof(i6c_venc_rate) == 36, "MI_VENC_RcAttr_t is the 36 bytes SetChnAttr compares");
_Static_assert(sizeof(i6c_venc_chn) == 76, "MI_VENC_ChnAttr_t is 76 bytes");
_Static_assert(offsetof(i6c_venc_chn, rate) == 40, "the rate half starts where ConfigRcAttr looks for it");
_Static_assert(offsetof(i6c_venc_attrib, h264.profile) == 16, "H26x profile is one of the masked fields");
_Static_assert(offsetof(i6c_venc_attrib, h264.width) == 24, "H26x width is one of the masked fields");
_Static_assert(offsetof(i6c_venc_attrib, h264.height) == 28, "H26x height is one of the masked fields");
_Static_assert(offsetof(i6c_venc_attrib, mjpg.width) == 20, "MJPG width is one of the masked fields");
_Static_assert(offsetof(i6c_venc_attrib, mjpg.height) == 24, "MJPG height is one of the masked fields");

_Static_assert(sizeof(i6c_venc_init) == 8, "MI_VENC_InitParam_t is InitDev's payload less one id");
_Static_assert(sizeof(i6c_venc_jpg) == 136, "MI_VENC_ParamJpeg_t is what Set/GetJpegParam memcpy");
_Static_assert(offsetof(i6c_venc_jpg, qtLuma) == 4, "SetJpegParam reads quality, then bytes from 4");
_Static_assert(offsetof(i6c_venc_jpg, qtChroma) == 68, "SetJpegParam takes the second table from +68");
_Static_assert(offsetof(i6c_venc_jpg, mcuPerEcs) == 132, "the tables leave one word at the end");

_Static_assert(sizeof(i6c_venc_stat) == 40, "MI_VENC_ChnStat_t is Query's payload less two ids");
_Static_assert(offsetof(i6c_venc_stat, bitrate) == 36, "IMPL_Query sweeps all ten words, 36 down to 0");

_Static_assert(sizeof(i6c_venc_strm) == 72, "MI_VENC_Stream_t spans payload 8 to 80 in GetStream");
_Static_assert(offsetof(i6c_venc_strm, count) == 4, "GetStream's pack loop bounds on this");
_Static_assert(offsetof(i6c_venc_strm, h264Info) == 16, "the stream-info union starts here");
_Static_assert(sizeof(i6c_venc_strminfo_h264) == 56, "the largest arm, so the union's own size");
_Static_assert(offsetof(i6c_venc_strminfo_h264, refType) == 44, "written as strm+60");
_Static_assert(offsetof(i6c_venc_strminfo_h265, refType) == 36, "written as strm+52");
_Static_assert(offsetof(i6c_venc_strminfo_mjpg, quality) == 8, "written as strm+24");

/*
 * i6c_venc_pack's stride is what the library steps the caller's array by, so an
 * error here walks off the array rather than corrupting one field.
 */
_Static_assert(sizeof(i6c_venc_pack) == 184, "GetStream indexes the pack array with mul #184");
_Static_assert(offsetof(i6c_venc_pack, data) == 8, "GetStream writes addr and data as one strd");
_Static_assert(offsetof(i6c_venc_pack, timestamp) == 16, "written as an 8-byte vstr");
_Static_assert(offsetof(i6c_venc_pack, endFrame) == 24, "the only byte store in the prefix");
_Static_assert(offsetof(i6c_venc_pack, packNum) == 36, "the NAL scanner's final count");
_Static_assert(offsetof(i6c_venc_pack, frameQual) == 40, "byte-stored from the same source as mjpg quality");
_Static_assert(offsetof(i6c_venc_pack, picOrder) == 44, "written with gradient as one strd");
_Static_assert(offsetof(i6c_venc_pack, packetInfo) == 52, "the NAL scanner's base, indexed by lsl #4");
_Static_assert(sizeof(i6c_venc_packinfo) == 16, "that lsl #4 is the stride");
_Static_assert(offsetof(i6c_venc_packinfo, length) == 8, "back-filled as this offset less the previous");
_Static_assert(offsetof(i6c_venc_packinfo, sliceId) == 12, "the remaining word the scanner writes");

#endif /* SIGMASTAR_I6C_VENC_H */
