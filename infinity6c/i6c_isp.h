/*
 * i6c_isp.h -- MI_ISP bindings, Infinity6C
 *
 * Vendored from OpenIPC divinus, src/hal/star/i6c_isp.h (MIT), which is why this
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
 * On this generation the ISP is a pipeline stage of its own -- device, channel
 * and output ports, sitting between VIF and SCL -- rather than the set of tuning
 * calls folded into VPE that MI 2.x has. That is why this file has no MI 2.x
 * counterpart to be diffed against.
 *
 * Keep the i6c_* names and field order as upstream so fixes stay diffable
 * against divinus.
 *
 * Copyright (c) 2024 OpenIPC
 * SPDX-License-Identifier: MIT
 */

#ifndef SIGMASTAR_I6C_ISP_H
#define SIGMASTAR_I6C_ISP_H

#include "i6c_common.h"

typedef struct {
    unsigned int rev;
    unsigned int size;
    unsigned char data[64];
} i6c_isp_iqver;

typedef struct {
    unsigned int sensorId;
    i6c_isp_iqver iqVer;
    unsigned int sync3A;
} i6c_isp_chn;

typedef struct {
    i6c_common_hdr hdr;
    /* Accepts values from 0-7 */
    int level3DNR;
    char mirror;
    char flip;
    /* Represents 90-degree arcs */
    int rotate;
    char yuv2BayerOn;
} i6c_isp_para;

typedef struct {
    i6c_common_rect crop;
    i6c_common_pixfmt pixFmt;
    i6c_common_compr compress;
    char multiPlanes;
} i6c_isp_port;

/*
 * Sizes read off the ioctl payloads, with the id words taken from the module's
 * own thunks rather than counted:
 *
 *   i6c_isp_chn   CreateChannel        88, two ids   (ldrd [r4], struct at +8)
 *   i6c_isp_para  SetChnParam          28, two ids   (ldrd [r4], struct at +8)
 *   i6c_isp_port  SetOutputPortParam   32, three ids (ldr [r4], ldrd [r4, #4],
 *                                                    struct at +12)
 *
 * All three had their field order checked as well, which a size cannot do, and
 * every member below is named by something in mi_isp.ko rather than inferred.
 *
 * i6c_isp_chn: MI_ISP_IMPL_CreateChannel prints "sensorbindid %d, sync3a %d,
 * ispinit version %d, size %d" from offsets 0, 76, 4 and 8 in that argument
 * order. So the sensor id leads, the version block follows at 4 as a revision
 * then a length, and sync3A is the last word -- which is what fixes the version
 * block's payload at 64 bytes, since 4 + 8 + 64 lands exactly on 76.
 *
 * i6c_isp_para: MI_ISP_CHECK_ChnParamValid bounds each field and then hands it
 * to the platform predicate that names it. Offset 0 is a word bounded at or
 * below 4 and goes to MI_ISP_PLATFORM_SupportHdr, which matches the five values
 * i6c_common_hdr has; offset 4 is a word bounded at or below 7 and goes to
 * Support3DNR, matching upstream's "values from 0-7" exactly; offsets 8 and 9
 * are read with ldrb and bounded at or below 1, going to SupportMirror and
 * SupportFlip; offset 12 is a word bounded at or below 3 and goes to SupportRot,
 * which is the four quadrants. A last call passes offsets 4, 9 and 12 together
 * to RotFlipRelyOn3Dnr, naming three of them at once.
 *
 * i6c_isp_port: MI_ISP_IMPL_SetOutputPortParam prints "croprect(%d,%d,%d,%d),
 * pixel %d Compress %d, layout %d" from four halfwords at 0, 2, 4 and 6 and
 * words at 8, 12 and 16.
 *
 * One caveat on that last member. The only read of offset 16 found anywhere is
 * that print, and it is a full word (ldr) against upstream's char -- so whether
 * the field is one byte or four is unresolved. It does not move the size either
 * way, since a char there is followed by three bytes of tail padding, and the
 * name is kept as upstream. It does mean a caller must clear the struct before
 * filling it: write only the low byte and the driver may read three bytes of
 * whatever was on the stack.
 *
 * See DERIVED.md.
 */
_Static_assert(sizeof(i6c_isp_chn) == 80, "MI_ISP_CreateChannel marshals 88 less two ids");
_Static_assert(sizeof(i6c_isp_para) == 20, "MI_ISP_SetChnParam marshals 28 less two ids");
_Static_assert(sizeof(i6c_isp_port) == 20, "MI_ISP_SetOutputPortParam marshals 32 less three ids");

_Static_assert(sizeof(i6c_isp_iqver) == 72, "sync3A at 76 leaves exactly this much from offset 4");
_Static_assert(offsetof(i6c_isp_chn, iqVer) == 4, "printed as the version, after the sensor id");
_Static_assert(offsetof(i6c_isp_chn, sync3A) == 76, "the last word, printed second of the four");
_Static_assert(offsetof(i6c_isp_iqver, size) == 4, "printed as the size, one word past the revision");

_Static_assert(offsetof(i6c_isp_para, level3DNR) == 4, "the word Support3DNR bounds at 7");
_Static_assert(offsetof(i6c_isp_para, mirror) == 8, "read with ldrb, handed to SupportMirror");
_Static_assert(offsetof(i6c_isp_para, flip) == 9, "read with ldrb, handed to SupportFlip");
_Static_assert(offsetof(i6c_isp_para, rotate) == 12, "the word SupportRot bounds at 3");
_Static_assert(offsetof(i6c_isp_para, yuv2BayerOn) == 16, "the only member left, and the struct is 20");

_Static_assert(offsetof(i6c_isp_port, pixFmt) == 8, "printed as the pixel format, after the crop");
_Static_assert(offsetof(i6c_isp_port, compress) == 12, "printed as Compress");
_Static_assert(offsetof(i6c_isp_port, multiPlanes) == 16, "printed as the layout");

/*
 * ================================================================
 * THE TUNING API: PAYLOAD SIZES AND MANUAL-BLOCK OFFSETS
 *
 * MI_ISP_IQ_* and MI_ISP_AE_* are not typed ioctls the way the pipeline
 * calls above are. Each is a thin wrapper that stacks a 24-byte descriptor
 * of {descriptor length, payload length, api id, channel, device, 0} and
 * hands it to MI_ISP_GENERAL_{Set,Get}IspApiData with the caller's buffer.
 * So one function pointer shape serves every module and the only per-module
 * facts are the two numbers below -- which is why hal_isp.c drives them from
 * a table rather than from a struct apiece.
 *
 * PAYLOAD is the length in that descriptor: what the library copies, so what
 * the staging buffer must be able to hold. Read out of each wrapper's own
 * literal pool, where it sits as the second word of the pair loaded into
 * d16 (the first is always 0x18, the descriptor's own length).
 *
 * MANUAL is offsetof(stManual) for a module whose payload is
 * { bEnable, enOpType, stAuto[16], stManual }. Writing a level anywhere else
 * lands in the per-ISO auto array: it does not take effect, and it corrupts
 * one auto entry on the way past.
 *
 * Every number here is derived twice and the two agree. The maruko headers
 * give the struct layout (MI_ISP_AUTO_NUM is 16; SaturationParam is 24 bytes
 * from SAT_LUT_X_NUM 5 and SAT_LUT_Y_NUM 6; DefogParam is one byte), and
 * the shipped blob's descriptor gives the length independently. Where the
 * derivations could disagree they do not: 8 + 16*24 + 24 lands exactly on
 * the 416 the wrapper declares.
 *
 * Eight of the ten modules hal_isp drives on Infinity6E carry the same
 * layout here, which is worth stating because MI 3.0 shares no layout with
 * MI 2.x anywhere else in this directory. The ones that differ are those
 * with the large per-band tables -- sharpness is 6264 bytes against
 * Infinity6E's 1268, 3DNR 1912 against 1776 -- so a level cannot simply be
 * poked at Infinity6E's offset.
 *
 * Two of those three now have a STRENGTH offset below as well, because a
 * per-band table does have an honest scalar in it: not one field, but one
 * contiguous run of same-width fields that all mean strength, which a knob
 * scales together about whatever the tuning left there. hal_isp.c calls that
 * shape IQ_VECTOR, and STRENGTH_NUM/STRENGTH_MAX are the run's length and
 * the vendor's per-field bound. Everything else in the module -- the
 * thresholds, the kernels, the by-luma and by-motion curves -- stays as the
 * tuning wrote it.
 *
 * 3DNR still has none. Its interior is thresholds and per-band weights with
 * no run that means "how much", and Infinity6C reaches it through
 * i6c_isp_para.level3DNR instead, which is one field the driver itself
 * bounds at 7.
 *
 * DEFOG is the one module whose vendor struct differs from Infinity6E's in
 * kind rather than size. There it is a bare 4-byte enable; on maruko it is
 * a full auto/manual module carrying a strength byte, so it has a manual
 * offset here where Infinity6E's table needs none.
 *
 * tests/abi_iq_i6c.c asserts all of this against the vendor headers.
 * ================================================================
 */
typedef int (*i6c_isp_cmd_fn)(unsigned int device, unsigned int channel, void *payload);

#define I6C_ISP_IQ_BRIGHTNESS_PAYLOAD 76
#define I6C_ISP_IQ_BRIGHTNESS_MANUAL  72
#define I6C_ISP_IQ_CONTRAST_PAYLOAD   76
#define I6C_ISP_IQ_CONTRAST_MANUAL    72
#define I6C_ISP_IQ_SATURATION_PAYLOAD 416
#define I6C_ISP_IQ_SATURATION_MANUAL  392
#define I6C_ISP_IQ_DEFOG_PAYLOAD      28
#define I6C_ISP_IQ_DEFOG_MANUAL       24

/*
 * Sharpness. MI_ISP_IQ_{Get,Set}Sharpness, api id 0x1012, payload 6264 --
 * 8 + 16 * 368 + 368, from MI_ISP_IQ_SharpnessParam_t at 368 bytes.
 *
 * The run is u8SharpnessUD[3] immediately followed by u8SharpnessD[3], both
 * at the front of the parameter block: the per-frequency gains for the
 * undirectional and directional sharpeners, three bands each, every one of
 * them 0..127. They are one six-byte run only because the vendor happens to
 * declare them adjacent, which abi_iq_i6c.c asserts rather than assumes --
 * a header drop that separates them would otherwise have a knob writing
 * u8PreCorUD as if it were sharpening strength.
 */
#define I6C_ISP_IQ_SHARPNESS_PAYLOAD      6264
#define I6C_ISP_IQ_SHARPNESS_MANUAL       5896
#define I6C_ISP_IQ_SHARPNESS_STRENGTH     0
#define I6C_ISP_IQ_SHARPNESS_STRENGTH_NUM 6
#define I6C_ISP_IQ_SHARPNESS_STRENGTH_MAX 127

/*
 * Advanced spatial luma denoise, which is what raptor calls sinter.
 * MI_ISP_IQ_{Get,Set}NrLumaAdv, api id 0x102d, payload 3544 --
 * 8 + 16 * 208 + 208, from MI_ISP_IQ_NrLumaAdvParam_t at 208 bytes.
 *
 * The run is u16Strength[2], which the vendor header itself labels
 * "blending (global strength)": how much of the filtered luma is mixed back
 * over the original, per level. 0..256 and not 0..255 -- 256 is unity, a
 * full swap for the filtered plane, so the bound is one past what a byte
 * would hold and the field is sixteen bits for that reason.
 *
 * Note there is a plain MI_ISP_IQ_NrLuma module as well. It is the older,
 * emptier one; NrLumaAdv is what the maruko tuning binaries carry.
 */
#define I6C_ISP_IQ_NRLUMAADV_PAYLOAD      3544
#define I6C_ISP_IQ_NRLUMAADV_MANUAL       3336
#define I6C_ISP_IQ_NRLUMAADV_STRENGTH     180
#define I6C_ISP_IQ_NRLUMAADV_STRENGTH_NUM 2
#define I6C_ISP_IQ_NRLUMAADV_STRENGTH_MAX 256

#define I6C_ISP_IQ_GRAY_PAYLOAD       4
#define I6C_ISP_AE_EVCOMP_PAYLOAD     8
#define I6C_ISP_AE_FLICKER_PAYLOAD    4
#define I6C_ISP_AE_EXPOLIMIT_PAYLOAD  32

/*
 * AE exposure and gain limits. MI_ISP_AE_{Get,Set}ExposureLimit, api id 0x1409,
 * payload 32 -- and byte-for-byte the Infinity6E i6_isp_exp, which is worth
 * stating because MI 3.0 shares no layout with MI 2.x anywhere else here.
 *
 * Both wrappers declare 32 and both carry that api id, read off their own
 * literal pools by the method above --
 *
 *   MI_ISP_AE_SetExposureLimit:  18000000 20000000  ->  0x20 = 32
 *   MI_ISP_AE_GetExposureLimit:  18000000 20000000
 *
 * cross-checked on the same library against two sizes already in the table
 * (brightness 0x4c = 76, saturation 0x1a0 = 416).
 *
 * The field order is the vendor's, from MI_ISP_AE_ExpoLimitType_t in
 * isp/mi_isp_ae_datatype.h: the two gain minima sit together ahead of the two
 * maxima, which is not the min/max pairing the shutter and aperture fields use.
 * tests/abi_iq_i6c.c asserts every offset against that header, because eight
 * same-width words in the wrong order are still 32 bytes.
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
} i6c_isp_exp;

_Static_assert(sizeof(i6c_isp_exp) == I6C_ISP_AE_EXPOLIMIT_PAYLOAD,
               "MI_ISP_AE_ExpoLimitType_t, and the wrapper declares 32");

/* X(row, vendor type) -- payload is { bEnable, enOpType, stAuto[16], stManual }. */
#define I6C_ISP_IQ_AUTOMAN_ROWS(X)                \
    X(IQ_BRIGHTNESS, MI_ISP_IQ_BrightnessType_t)  \
    X(IQ_CONTRAST,   MI_ISP_IQ_ContrastType_t)    \
    X(IQ_SATURATION, MI_ISP_IQ_SaturationType_t)  \
    X(IQ_DEFOG,      MI_ISP_IQ_DefogType_t)

/* X(row, vendor type) -- no auto/manual split; the field written is at offset 0. */
#define I6C_ISP_IQ_FLAT_ROWS(X)                   \
    X(IQ_GRAY,    MI_ISP_IQ_ColorToGrayType_t)    \
    X(AE_EVCOMP,  MI_ISP_AE_EvCompType_t)         \
    X(AE_FLICKER, MI_ISP_AE_FlickerType_e)

/*
 * X(row, vendor type, vendor param type, first field of the strength run,
 * field width) -- the same { bEnable, enOpType, stAuto[16], stManual } payload
 * as the auto/manual rows, but the value is a run of STRENGTH_NUM fields
 * starting at STRENGTH inside stManual rather than a single level.
 */
#define I6C_ISP_IQ_VECTOR_ROWS(X)                                                \
    X(IQ_SHARPNESS,  MI_ISP_IQ_SharpnessType_t,  MI_ISP_IQ_SharpnessParam_t,     \
      u8SharpnessUD, MI_U8)                                                      \
    X(IQ_NRLUMAADV,  MI_ISP_IQ_NrLumaAdvType_t,  MI_ISP_IQ_NrLumaAdvParam_t,     \
      u16Strength,   MI_U16)

#endif /* SIGMASTAR_I6C_ISP_H */
