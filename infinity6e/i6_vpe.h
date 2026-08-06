/*
 * i6_vpe.h -- MI_VPE bindings, Infinity6E
 *
 * Vendored from OpenIPC divinus, src/hal/star/i6_vpe.h (MIT).
 * See i6_common.h for why these declarations are vendored rather than derived.
 *
 * IMPORTANT -- two channel/param layouts, and Infinity6E uses the i6e_ ones.
 * MI_VPE_CreateChannel and MI_VPE_SetChannelParam take a longer struct on
 * Infinity6E (i6e_vpe_chn / i6e_vpe_para, with the lens-distortion-correction
 * members) than on the original Infinity6 (i6_vpe_chn / i6_vpe_para).
 * divinus picks by SoC series at runtime and casts to the shorter type, since
 * that is what the function pointers are declared with -- see i6_hal.c:302-345,
 * `if (series == 0xF1)`. Our target is 0xF1 only, so the backend always
 * populates the i6e_ variants; the shorter ones are kept solely because the
 * function-pointer signatures name them and because keeping the file diffable
 * against upstream is worth more than trimming two structs.
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

typedef struct {
    i6_common_dim output;
    char mirror;
    char flip;
    i6_common_pixfmt pixFmt;
    i6_common_compr compress;
} i6_vpe_port;

#endif /* SIGMASTAR_I6_VPE_H */
