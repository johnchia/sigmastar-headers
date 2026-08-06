/*
 * i6_isp.h -- MI_ISP bindings, Infinity6E
 *
 * Based on OpenIPC divinus, src/hal/star/i6_isp.h (MIT).
 * See i6_common.h for why these declarations are vendored rather than derived.
 *
 * ================================================================
 * TWO SHAPES OF MI_ISP CALL
 *
 * MI_ISP splits into a handful of *typed* lifecycle calls -- load an IQ
 * binary, enable CUS3A, read the AE exposure limits -- and roughly 340
 * MI_ISP_{IQ,AE,AWB,AF}_{Get,Set}<Module> calls that are all the same
 * shape: (int channel, void *payload), with a per-module payload whose
 * size the userspace wrapper hardcodes.
 *
 * The typed calls get real prototypes here. The per-module ones do not,
 * because writing 340 structs to poke one field each is not a sensible
 * trade -- hal_isp.c drives them from a descriptor table instead, and
 * only needs `handle` plus the i6_isp_cmd_fn signature below. The
 * layout convention that makes that possible is documented in hal_isp.c.
 * ================================================================
 *
 * Copyright (c) 2024 OpenIPC
 * SPDX-License-Identifier: MIT
 */

#ifndef SIGMASTAR_I6_ISP_H
#define SIGMASTAR_I6_ISP_H

#include "i6_common.h"

/*
 * AE exposure limits. MI_ISP_AE_{Get,Set}ExposureLimit, command 0x1409.
 *
 * The 32-byte payload size the wrapper hardcodes matches these eight
 * unsigned ints exactly (checked by disassembling the board's own
 * libmi_isp.so -- see the objdump technique in i6_sys.h). Note the field
 * order: the two gain minima precede the two maxima, which is not the
 * min/max pairing the shutter and aperture fields above them use.
 */
typedef struct {
    unsigned int minShutterUs;
    unsigned int maxShutterUs;
    unsigned int minApertX10;
    unsigned int maxApertX10;
    unsigned int minSensorGain;
    unsigned int minIspGain;
    unsigned int maxSensorGain;
    unsigned int maxIspGain;
} i6_isp_exp;

/*
 * AE status. MI_ISP_CUS3A_GetAeStatus, command 0x2e05.
 *
 * The one MI call that answers "what did the AE converge on" -- shutter
 * in microseconds and the two gains -- which is what day/night detection
 * needs and what MI_ISP_AE_GetManualExpo (the manual *setting*) and
 * GetExposureLimit (the bounds) cannot give. Field order is waybeam's
 * (star6e_cus3a.c, "verified via hex dump" on Star6E); this file adds the
 * size.
 *
 * That size is the reason not to copy waybeam's struct as it stands. The
 * wrapper declares a 65-byte payload:
 *
 *   65d4: sub  sp, #32
 *   65e4: movs r3, #65        @ 0x41   -- payload size
 *   65e8: movw r3, #11781     @ 0x2e05 -- command
 *   65fe: blx  _MI_ISP_GetIspApiData
 *
 * and _MI_ISP_GetIspApiData copies all 65 bytes into the caller's buffer.
 * waybeam declares twelve u32s, 48 bytes, on the stack -- so the vendor
 * library writes 17 bytes past it on every call. The tail below exists to
 * hold that overrun, and the assert is what keeps it holding it. Anything
 * past ispGainHdrShort is unread, not unwritten.
 */
typedef struct {
    unsigned int reserved0[3];
    unsigned int avgBlkX;
    unsigned int avgBlkY;
    unsigned int reserved1;
    unsigned int shutterUs;
    unsigned int sensorGain;
    unsigned int ispGain;
    unsigned int shutterHdrShortUs;
    unsigned int sensorGainHdrShort;
    unsigned int ispGainHdrShort;
    unsigned char tail[20];
} i6_isp_ae_status;

_Static_assert(sizeof(i6_isp_ae_status) >= 65,
               "AE status must hold the 65 bytes the wrapper copies into it");
_Static_assert(offsetof(i6_isp_ae_status, shutterUs) == 24, "AE status shutter offset");
_Static_assert(offsetof(i6_isp_ae_status, sensorGain) == 28, "AE status sensor gain offset");

/*
 * Hardware AE average statistics. MI_ISP_AE_GetAeHwAvgStats, command
 * 0x2e01, payload 46088 bytes (0xb408 at that wrapper's `movw r3`).
 *
 * 46088 = 128 * 90 * 4 + 8, and the neighbouring
 * MI_ISP_AWB_GetAwbHwAvgStats declares 34568 = 128 * 90 * 3 + 8. Two
 * calls agreeing on a 128x90 grid at one byte per channel, plus the same
 * eight spare bytes, is what fixes the cell width at four bytes here --
 * so waybeam's `short r, g, b, y` (8 bytes a cell, a 92160-byte struct
 * against a 46088-byte payload) cannot be the layout, and its avgY log
 * line is averaging two cells per sample.
 *
 * What those eight spare bytes are, and whether they lead or trail, the
 * sizes cannot say. Hence the union: hal_isp.c tries both placements
 * against the grid dimensions MI_ISP_CUS3A_GetAeStatus reports and
 * accepts the one that matches, rather than picking one and averaging
 * whatever lands at that offset. Neither matching means no luma -- not a
 * plausible-looking number derived from the wrong bytes.
 *
 * Measured on an SSC30KQ + GC4653: the grid reads back 32x32 with the
 * cells at offset 8, so the eight bytes LEAD and they are the grid
 * dimensions themselves. 128x90 is the payload's maximum,
 * not the live grid -- the buffer stays sized for the declared 46088
 * either way, and the grid actually averaged is whatever AE status
 * reports.
 *
 * The lane order is r,g,b,y, confirmed from the cells themselves: over 1022
 * scored cells lane 3 is the BT.601 sum of lanes 0..2 to within integer
 * rounding, which no other assignment of r, g and b reproduces. A frame
 * *mean* cannot settle it -- see the reasoning at the lane check in
 * hal_isp.c, which is where the two ways of getting this wrong are written
 * down.
 */
#define I6_ISP_AE_BLK_X 128
#define I6_ISP_AE_BLK_Y 90
#define I6_ISP_AE_BLK_MAX (I6_ISP_AE_BLK_X * I6_ISP_AE_BLK_Y)
#define I6_ISP_AE_CELL_SZ 4

/* Byte lane within a cell. The order is r, g, b, y -- see above. */
#define I6_ISP_AE_CELL_Y 3

typedef union {
    unsigned char raw[I6_ISP_AE_BLK_MAX * I6_ISP_AE_CELL_SZ + 8];
    struct {
        unsigned int blkX, blkY;
        unsigned char cell[I6_ISP_AE_BLK_MAX * I6_ISP_AE_CELL_SZ];
    } lead;
    struct {
        unsigned char cell[I6_ISP_AE_BLK_MAX * I6_ISP_AE_CELL_SZ];
        unsigned int blkX, blkY;
    } trail;
} i6_isp_ae_hw_stats;

_Static_assert(sizeof(i6_isp_ae_hw_stats) == 46088,
               "AE HW stats must match the 46088-byte payload the wrapper declares");

/*
 * IQ parameter-init status. MI_ISP_IQ_GetParaInitStatus, command 0x1002,
 * payload 4 bytes -- a single flag, which is why the nested-struct form
 * the vendor headers use collapses to this.
 *
 * This is the one binding neither divinus nor an ISP tuning guide would
 * suggest is necessary, and it is the difference between a reliable IQ
 * load and an intermittent one. The ISP channel initialises
 * asynchronously *after* MI_VPE_CreateChannel returns, so a bin load
 * issued immediately gets "IspApiGet channel not created" from the
 * kernel driver. divinus papers over this with sleep(1) before its load
 * (media.c:827); polling this flag is the same wait without the guess.
 */
typedef struct {
    int ready;
} i6_isp_parainit;

/*
 * Generic per-module IQ/AE/AWB command. Every
 * MI_ISP_{IQ,AE,AWB}_{Get,Set}<Module> entry point has this signature;
 * the payload is module-specific and its size is fixed in the wrapper.
 */
typedef int (*i6_isp_cmd_fn)(int channel, void *payload);

/*
 * ================================================================
 * PAYLOAD SIZES AND MANUAL-BLOCK OFFSETS FOR THE MODULES HAL_ISP DRIVES
 *
 * PAYLOAD is the size the board's libmi_isp.so wrapper puts in the
 * descriptor it hands to _MI_ISP_{Get,Set}IspApiData, which is the length
 * that gets copied into the caller's buffer -- so it sizes the staging
 * buffer, and under-declaring it means the library writes past the end.
 * Read out of the wrapper's own `mov r3, #<size>`, the same way as the
 * typed calls above.
 *
 * That is deliberately not "sizeof the vendor struct". The two agree for
 * every module here except DEFOG, where the blob declares 28 against a
 * 4-byte MI_ISP_IQ_DEFOG_TYPE_t -- so the blob wins, because it is what
 * does the copying.
 *
 * MANUAL is offsetof(stManual) for the modules whose payload is
 * { bEnable, enOpType, stAuto[16], stManual }: writing a level anywhere
 * else lands in the per-ISO auto array, which both fails to take effect
 * and corrupts one auto entry.
 *
 * The two ROWS lists name the vendor type each row addresses so
 * tests/abi_iq.c can assert all of this against mi_isp_iq_datatype.h and
 * mi_isp_3a_datatype.h at once. hal_isp.c expands only the numbers.
 * ================================================================
 */
#define I6_ISP_IQ_BRIGHTNESS_PAYLOAD 76
#define I6_ISP_IQ_BRIGHTNESS_MANUAL  72
#define I6_ISP_IQ_CONTRAST_PAYLOAD   76
#define I6_ISP_IQ_CONTRAST_MANUAL    72
#define I6_ISP_IQ_SATURATION_PAYLOAD 416
#define I6_ISP_IQ_SATURATION_MANUAL  392
#define I6_ISP_IQ_SHARPNESS_PAYLOAD  1268
#define I6_ISP_IQ_SHARPNESS_MANUAL   1192
#define I6_ISP_IQ_NRLUMA_PAYLOAD     112
#define I6_ISP_IQ_NRLUMA_MANUAL      104
#define I6_ISP_IQ_NR3D_PAYLOAD       1776
#define I6_ISP_IQ_NR3D_MANUAL        1672

#define I6_ISP_IQ_DEFOG_PAYLOAD      28
#define I6_ISP_IQ_GRAY_PAYLOAD       4
#define I6_ISP_AE_EVCOMP_PAYLOAD     8
#define I6_ISP_AE_FLICKER_PAYLOAD    4

/* X(row, vendor type) -- payload is { bEnable, enOpType, stAuto[16], stManual }. */
#define I6_ISP_IQ_AUTOMAN_ROWS(X)                 \
    X(IQ_BRIGHTNESS, MI_ISP_IQ_BRIGHTNESS_TYPE_t) \
    X(IQ_CONTRAST,   MI_ISP_IQ_CONTRAST_TYPE_t)   \
    X(IQ_SATURATION, MI_ISP_IQ_SATURATION_TYPE_t) \
    X(IQ_SHARPNESS,  MI_ISP_IQ_SHARPNESS_TYPE_t)  \
    X(IQ_NRLUMA,     MI_ISP_IQ_NRLUMA_TYPE_t)     \
    X(IQ_NR3D,       MI_ISP_IQ_NR3D_TYPE_t)

/* X(row, vendor type) -- no auto/manual split; the field written is at offset 0. */
#define I6_ISP_IQ_FLAT_ROWS(X)                  \
    X(IQ_DEFOG,   MI_ISP_IQ_DEFOG_TYPE_t)       \
    X(IQ_GRAY,    MI_ISP_IQ_COLORTOGRAY_TYPE_t) \
    X(AE_EVCOMP,  MI_ISP_AE_EV_COMP_TYPE_t)     \
    X(AE_FLICKER, MI_ISP_AE_FLICKER_TYPE_e)

#endif /* SIGMASTAR_I6_ISP_H */
