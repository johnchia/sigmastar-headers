/*
 * i6_sys.h -- MI_SYS bindings, Infinity6E
 *
 * Vendored from OpenIPC divinus, src/hal/star/i6_sys.h (MIT).
 * See i6_common.h for why these declarations are vendored rather than derived.
 *
 * libcam_os_wrapper is loaded first and RTLD_GLOBAL: every libmi_* depends
 * on it, and its absence is not fatal here on purpose -- some firmware
 * builds satisfy those symbols another way, and failing the whole load for a
 * library that may be unnecessary would be worse than letting the libmi_sys
 * dlopen report the real problem.
 *
 * The frame-buffer entry points (MI_SYS_ChnOutputPortGetBuf / PutBuf,
 * MI_SYS_GetFd / CloseFd, MI_SYS_FlushInvCache, MI_SYS_Va2Pa) have no divinus
 * counterpart -- it reads encoded streams out of VENC and never touches a raw
 * frame -- so their types and signatures come from waybeam_venc plus
 * libmi_sys.so's disassembly. See the i6_sys_bufinfo comment below.
 *
 * Copyright (c) 2024 OpenIPC
 * SPDX-License-Identifier: MIT
 */

#ifndef SIGMASTAR_I6_SYS_H
#define SIGMASTAR_I6_SYS_H

#include "i6_common.h"

/* i6_sys_bufinfo declares endOfFrame as bool */
#include <stdbool.h>

#define I6_SYS_API "1.0"

typedef enum {
    I6_SYS_LINK_FRAMEBASE = 0x1,
    I6_SYS_LINK_LOWLATENCY = 0x2,
    I6_SYS_LINK_REALTIME = 0x4,
    I6_SYS_LINK_AUTOSYNC = 0x8,
    I6_SYS_LINK_RING = 0x10
} i6_sys_link;

typedef enum {
    I6_SYS_MOD_IVE,
    I6_SYS_MOD_VDF,
    I6_SYS_MOD_VENC,
    I6_SYS_MOD_RGN,
    I6_SYS_MOD_AI,
    I6_SYS_MOD_AO,
    I6_SYS_MOD_VIF,
    I6_SYS_MOD_VPE,
    I6_SYS_MOD_VDEC,
    I6_SYS_MOD_SYS,
    I6_SYS_MOD_FB,
    I6_SYS_MOD_HDMI,
    I6_SYS_MOD_DIVP,
    I6_SYS_MOD_GFX,
    I6_SYS_MOD_VDISP,
    I6_SYS_MOD_DISP,
    I6_SYS_MOD_OS,
    I6_SYS_MOD_IAE,
    I6_SYS_MOD_MD,
    I6_SYS_MOD_OD,
    I6_SYS_MOD_SHADOW,
    I6_SYS_MOD_WARP,
    I6_SYS_MOD_UAC,
    I6_SYS_MOD_LDC,
    I6_SYS_MOD_SD,
    I6_SYS_MOD_PANEL,
    I6_SYS_MOD_CIPHER,
    I6_SYS_MOD_SNR,
    I6_SYS_MOD_WLAN,
    I6_SYS_MOD_IPU,
    I6_SYS_MOD_MIPITX,
    I6_SYS_MOD_GYRO,
    I6_SYS_MOD_JPD,
    I6_SYS_MOD_ISP,
    I6_SYS_MOD_SCL,
    I6_SYS_MOD_WBC,
    I6_SYS_MOD_DSP,
    I6_SYS_MOD_PCIE,
    I6_SYS_MOD_DUMMY,
    I6_SYS_MOD_NIR,
    I6_SYS_MOD_DPU,
    I6_SYS_MOD_END,
} i6_sys_mod;

typedef struct {
    i6_sys_mod module;
    unsigned int device;
    unsigned int channel;
    unsigned int port;
} i6_sys_bind;

typedef struct {
    unsigned char version[128];
} i6_sys_ver;

/*
 * Output-port frame buffers.
 *
 * Not from divinus -- it reads encoded streams out of VENC and never touches a
 * raw frame -- so the layout comes from waybeam_venc, which does read VIF/VPE
 * output on this exact silicon: StabSysBufInfo_t in src/star6e_framing_stab.c
 * (Infinity6E) and IyBufInfo_t in src/star6e_ipu_yolo.c (Infinity6E), plus
 * StabBufInfo_t in src/maruko_framing_stab.c (i6c). Three transcriptions that
 * agree, two of them for this SoC, all three exercised on hardware.
 *
 * libmi_sys.so's own MI_SYS_ChnOutputPortGetBuf corroborates it. The wrapper
 * memcpy's the 16-byte port descriptor into its ioctl payload, and after the
 * call copies 272 bytes back out to argument 2 and the buffer handle from
 * payload+288 to argument 3. So the kernel's view of this struct is 272 bytes,
 * meaning the vendor union is 232 -- not the 512 waybeam reserves, and not the
 * 104 that the frame-data member alone needs.
 *
 * Reserving 512 anyway is deliberate. It is what the proven code does, it
 * cannot under-allocate for a 272-byte copy-out even if another firmware
 * revision grows the union, and every field we read sits in the first 136
 * bytes regardless. The static assertions below pin the copy size and the
 * offsets the kernel writes through, so a later edit cannot quietly shrink
 * this below what MI_SYS memcpy's into it.
 *
 * The 272 figure also settles the bool width: MI_BOOL is a C99 bool (1 byte,
 * per waybeam include/star6e.h), which puts the union at offset 32 and yields
 * align8(32 + 232 + 1) == 272. Four-byte bools would give 280 and every field
 * from bEndOfStream on would be misplaced.
 */
typedef enum {
    I6_SYS_BUFDATA_RAW = 0,
    I6_SYS_BUFDATA_FRAME = 1,
    I6_SYS_BUFDATA_META = 2,
    /* Real, and the arm it selects is what makes MI_SYS_BufInfo_t as large
     * as it is -- see the 272-byte note above. Nothing on a single-plane
     * path produces one, but with _END sitting at 3 such a frame reads as
     * the terminator rather than as a type. */
    I6_SYS_BUFDATA_MULTIPLANE = 3,
    I6_SYS_BUFDATA_END
} i6_sys_bufdata;

typedef struct {
    int tileMode;
    i6_common_pixfmt pixFmt;
    i6_common_compr compress;
    int scanMode;
    int fieldType;
    int layoutType;
    unsigned short width;
    unsigned short height;
    void *virAddr[3];
    unsigned long long phyAddr[3];
    unsigned int stride[3];
    unsigned int bufSize;
    unsigned short ringStartLine;
    unsigned short ringTotalHeight;
    struct {
        int type;
        union {
            unsigned int globalGradient;
        } attr;
    } ispInfo;
    i6_common_rect crop;
} i6_sys_frame;

typedef struct {
    void *virAddr;
    unsigned long long phyAddr;
    unsigned int bufSize;
    unsigned int contentSize;
    bool endOfFrame;
    unsigned long long seqNum;
} i6_sys_raw;

typedef struct {
    void *virAddr;
    unsigned long long phyAddr;
    unsigned int size;
    unsigned int extraData;
    unsigned int fromModule;
} i6_sys_meta;

typedef struct {
    unsigned long long pts;
    unsigned long long sidebandMsg;
    i6_sys_bufdata bufType;
    bool endOfStream;
    bool usrBuf;
    unsigned int seqNum;
    bool drop;
    union {
        i6_sys_frame frame;
        i6_sys_raw raw;
        i6_sys_meta meta;
        unsigned char reserved[512];
    };
    unsigned char cusFlag;
} i6_sys_bufinfo;

/* The port descriptor MI memcpy's out of argument 1 -- 16 bytes, and the probe
 * confirmed sizeof(i6_sys_bind) == 16 on hardware. */
_Static_assert(sizeof(i6_sys_bind) == 16, "i6_sys_bind must match the 16 bytes GetBuf copies");
_Static_assert(sizeof(i6_sys_bufinfo) >= 272,
    "i6_sys_bufinfo must not be smaller than the 272 bytes MI_SYS copies into it");
_Static_assert(offsetof(i6_sys_bufinfo, seqNum) == 24, "i6_sys_bufinfo header layout drifted");
_Static_assert(offsetof(i6_sys_bufinfo, frame) == 32, "i6_sys_bufinfo union must start at 32");
_Static_assert(offsetof(i6_sys_frame, virAddr) == 28, "i6_sys_frame plane pointers drifted");
_Static_assert(offsetof(i6_sys_frame, phyAddr) == 40, "i6_sys_frame phyAddr must be 8-aligned");
_Static_assert(offsetof(i6_sys_frame, stride) == 64, "i6_sys_frame stride drifted");

#endif /* SIGMASTAR_I6_SYS_H */
