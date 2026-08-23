/*
 * i6_vpe.h -- MI_VPE bindings, Infinity6E
 *
 * Vendored from OpenIPC divinus, src/hal/star/i6_vpe.h (MIT).
 * See i6_common.h for why these declarations are vendored rather than derived.
 *
 * IMPORTANT -- two channel/param layouts, and the family picks. divinus is
 * right about this (i6_hal.c:302-345, `if (series == 0xF1)`): Infinity6E takes
 * the longer i6e_ structs, Infinity6B0 the shorter i6_ ones, for BOTH
 * MI_VPE_CreateChannel and MI_VPE_SetChannelParam.
 *
 * Do not settle this from the vendor SDK headers. ssc335 (Ispahan/6B0),
 * ssc336q (Pudding/6E) and ssc377 (Maruko/6C) all declare the SHORT
 * MI_VPE_ChannelPara_t, with eHDRType straight after stPqParam -- and that is
 * simply not what the shipped libmi_vpe.so does on 6E. The library is the
 * ground truth because it never interprets the struct: MI_VPE_SetChannelParam
 * memcpys it whole into an ioctl buffer, so the copy length IS the ABI.
 *
 *   6E   (ssc30kq, libmi_vpe.so dc127daf)  memset 104, memcpy 100
 *                                          = sizeof(i6e_vpe_para), 16+72+4+4+4
 *   6B0  (ssc333,  libmi_vpe.so 970fcb10)  memset  32, memcpy  28
 *                                          = sizeof(i6_vpe_para),  16+4+4+4
 *
 * So e3DNRLevel is at +92 on 6E and +20 on 6B0, and a backend serving both
 * must choose per platform -- star_state.h does, via star_vpe_para.
 *
 * What handing 6B0 the i6e_ form cost, measured on an SSC333 at 2304x1296:
 * its libmi_vpe.so copied the first 28 bytes of a 100-byte struct, so
 * e3DNRLevel came out of the zeroed LDC block, MhalCameraOpen logged
 * "3DNR = 0", no DNR reference frame was allocated, and every NR3D value in
 * the tuning binary was inert because the engine never ran. On 6E the same
 * code has always been correct -- .229 shows DNR_INFO0 at 0x5a0000, the
 * level-2 signature at 1440 rows.
 *
 * Copyright (c) 2024 OpenIPC
 * SPDX-License-Identifier: MIT
 */

#ifndef SIGMASTAR_I6_VPE_H
#define SIGMASTAR_I6_VPE_H

#include "i6_common.h"

typedef enum {
    I6_VPE_MODE_INVALID,
    I6_VPE_MODE_DVR = 0x1,
    I6_VPE_MODE_CAM_TOP = 0x2,
    I6_VPE_MODE_CAM_BOTTOM = 0x4,
    I6_VPE_MODE_CAM = I6_VPE_MODE_CAM_TOP | I6_VPE_MODE_CAM_BOTTOM,
    I6_VPE_MODE_REALTIME_TOP = 0x8,
    I6_VPE_MODE_REALTIME_BOTTOM = 0x10,
    I6_VPE_MODE_REALTIME = I6_VPE_MODE_REALTIME_TOP | I6_VPE_MODE_REALTIME_BOTTOM,
    I6_VPE_MODE_END
} i6_vpe_mode;

/*
 * MI_VPE_SensorChannel_e. A bitmask, not an index: the 2021-09-02 drop
 * numbered these sequentially and the 2022-06-01 drop made them bits to
 * support multi-sensor stitch, moving ID2 from 3 to 4 and ID3 from 4 to 8.
 * ID0 and ID1 happen to be the same either way.
 */
typedef enum {
    I6_VPE_SENS_INVALID = 0,
    I6_VPE_SENS_ID0 = 0x1,
    I6_VPE_SENS_ID1 = 0x2,
    I6_VPE_SENS_ID2 = 0x4,
    I6_VPE_SENS_ID3 = 0x8,
    I6_VPE_SENS_ID4 = 0x10,
    I6_VPE_SENS_ID5 = 0x20,
    I6_VPE_SENS_ID6 = 0x40,
    I6_VPE_SENS_ID7 = 0x80,
    I6_VPE_SENS_END,
} i6_vpe_sens;

typedef struct {
    unsigned int rev;
    unsigned int size;
    unsigned char data[64];
} i6_vpe_iqver;

typedef struct {
    int mode;
    char bypassOn;
    char proj3x3On;
    int proj3x3[9];
    unsigned short userSliceNum;
    unsigned int focalLengthX;
    unsigned int focalLengthY;
    void *configAddr;
    unsigned int configSize;
    int mapType;
    union {
        struct {
            void *xMapAddr, *yMapAddr;
            unsigned int xMapSize, yMapSize;
        } dispInfo;
        struct {
            void *calibPolyBinAddr;
            unsigned int calibPolyBinSize;
        } calibInfo;
    };
} i6e_vpe_ildc;

/* MI_VPE_LdcInitPara_t is 84 bytes and has no trailing bEnLdc of its own --
 * i6e_vpe_chn carries that, immediately after this struct. A duplicate here
 * pushes the channel attr's own bEnLdc and u32ChnPortMode 4 bytes late, so
 * MI_VPE_CreateChannel reads bEnLdc out of this struct's tail and never sees
 * chnPort at all. Zero-initialised callers do not notice; enabling LDC or
 * setting a channel port mode would look like the SDK ignoring the request. */
_Static_assert(sizeof(i6e_vpe_ildc) == 84, "MI_VPE_LdcInitPara_t is 84 bytes");

typedef struct {
    char bypassOn;
    char proj3x3On;
    int proj3x3[9];
    unsigned int focalLengthX;
    unsigned int focalLengthY;
    void *configAddr;
    unsigned int configSize;
    union {
        struct {
            void *xMapAddr, *yMapAddr;
            unsigned int xMapSize, yMapSize;
        } dispInfo;
        struct {
            void *calibPolyBinAddr;
            unsigned int calibPolyBinSize;
        } calibInfo;
    };
} i6e_vpe_ldc;

typedef struct {
    i6_common_dim capt;
    i6_common_pixfmt pixFmt;
    i6_common_hdr hdr;
    i6_vpe_sens sensor;
    char noiseRedOn;
    char edgeOn;
    char edgeSmoothOn;
    char contrastOn;
    char invertOn;
    char rotateOn;
    i6_vpe_mode mode;
    /*
     * MI_VPE_ChannelAttr_t.tIspInitPara, not a version string: a 64-byte
     * blob VPE forwards to the ISP as early-init sensor state -- first
     * frames' fps, shutter, gains and AWB gains. Zeroed here, which is the
     * "no opinion" value.
     */
    i6_vpe_iqver iqparam;
    i6e_vpe_ildc lensInit;
    char lensAdjOn;
    unsigned int chnPort;
} i6e_vpe_chn;

_Static_assert(sizeof(i6e_vpe_chn) == 192, "MI_VPE_ChannelAttr_t is 192 bytes");
_Static_assert(offsetof(i6e_vpe_chn, lensAdjOn) == 184, "bEnLdc sits at +184");
_Static_assert(offsetof(i6e_vpe_chn, chnPort) == 188, "u32ChnPortMode sits at +188");

typedef struct {
    i6_common_dim capt;
    i6_common_pixfmt pixFmt;
    i6_common_hdr hdr;
    i6_vpe_sens sensor;
    char noiseRedOn;
    char edgeOn;
    char edgeSmoothOn;
    char contrastOn;
    char invertOn;
    char rotateOn;
    i6_vpe_mode mode;
    i6_vpe_iqver iqparam;
    char lensAdjOn;
    unsigned int chnPort;
} i6_vpe_chn;

/* MI_VPE_ChannelPara_t as Infinity6E's libmi_vpe.so reads it: 100 bytes,
 * level3DNR at +92. Infinity6B0 wants i6_vpe_para instead -- see above. */
typedef struct {
    char reserved[16];
    i6e_vpe_ldc lensAdj;
    i6_common_hdr hdr;
    // Accepts values from 0-7
    int level3DNR;
    char mirror;
    char flip;
    char reserved2;
    char lensAdjOn;
} i6e_vpe_para;

typedef struct {
    char reserved[16];
    i6_common_hdr hdr;
    // Accepts values from 0-7
    int level3DNR;
    char mirror;
    char flip;
    char reserved2;
    char lensAdjOn;
} i6_vpe_para;

/*
 * reserved[16] is MI_VPE_PqParam_t (nine NR strengths, six edge gains and a
 * contrast, all MI_U8); reserved2 and lensAdjOn are bWdrEn and bEnLdc.
 *
 * The sizes are asserted because each one is the exact number of bytes its
 * family's libmi_vpe.so memcpys out of this struct, and a mismatch is silent:
 * the ioctl still succeeds, it just carries the wrong fields.
 */
_Static_assert(sizeof(i6_vpe_para) == 28, "Infinity6B0 libmi_vpe.so copies 28");
_Static_assert(offsetof(i6_vpe_para, hdr) == 16, "eHDRType sits at +16");
_Static_assert(offsetof(i6_vpe_para, level3DNR) == 20, "e3DNRLevel sits at +20");
_Static_assert(sizeof(i6e_vpe_para) == 100, "Infinity6E libmi_vpe.so copies 100");
_Static_assert(offsetof(i6e_vpe_para, level3DNR) == 92, "6E e3DNRLevel at +92");

typedef struct {
    i6_common_dim output;
    char mirror;
    char flip;
    i6_common_pixfmt pixFmt;
    i6_common_compr compress;
} i6_vpe_port;

#endif /* SIGMASTAR_I6_VPE_H */
