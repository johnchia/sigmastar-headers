/*
 * i6c_aud.h -- MI_AI (audio input) ABI for Infinity6C (MI 3.0)
 *
 * Counterpart to infinity6e/i6_aud.h, and the one header in this family that is
 * a transcription rather than a reconstruction: the vendor's mi_ai.h,
 * mi_ai_datatype.h and mi_aio_datatype.h are available, and all three ssc377
 * SDK drops (0602, 0712, 0907) ship them byte-identical at AI version 3.53. So
 * the layouts below are the vendor's own, and both were then checked against the
 * shipped libmi_ai.so anyway -- see DERIVED.md's MI_AI section, and
 * shipped-blob-outranks-vendor-header for why that check is not optional here.
 *
 * MI 3.0 AUDIO IS NOT MI 2.x AUDIO WITH A SoC ID ADDED.
 *
 * The whole shape of the module changed, so nothing here is a near-copy of
 * i6_aud.h and a reader coming from that file should expect no correspondence:
 *
 * 1. Device attributes lost the interface. MI 2.x's MI_AI_SetPubAttr carried the
 *    input interface, the I2S mode, the DMA ring depth and the channel count in
 *    one struct. MI 3.0's MI_AI_Open takes the *data* shape only -- format,
 *    sound mode, rate, period size, interleaving -- and the physical input is
 *    chosen separately by MI_AI_AttachIf. That separation is the module's
 *    organising idea, not a detail: a device is a DMA writer, and attaching is
 *    what points its multiplexer at ADC or DMIC pins.
 *
 * 2. Channels became channel groups. A group is a set of physical channels held
 *    together so their samples stay aligned, and how many groups a device has is
 *    derived rather than configured:
 *
 *        groups = (physical channels attached - echo channels) / sound mode
 *
 *    One interface attaches two physical channels, so one ADC interface in MONO
 *    is two groups and in STEREO is one. Every per-channel entry point takes a
 *    group index, so `chn` on this generation is a group and not a track.
 *
 * 3. There is no device-side ring depth. MI 2.x's frmNum is gone and the
 *    MI_SYS output-port queue is the only queue there is, which makes
 *    MI_SYS_SetChnOutputPortDepth load-bearing rather than tuning. hal_audio.c
 *    sets it right after the group is enabled.
 *
 * 4. The algorithms are gone, not merely absent. MI 2.x's libmi_ai.so exported
 *    the VQE entry points over weak-undefined algorithm packs, so calling one
 *    without the packs jumped to address 0. This generation's library exports
 *    22 symbols and not one of them matches vqe, aenc, aed or src -- noise
 *    reduction, AGC, HPF, echo cancellation and resampling are simply not in
 *    the module. That is a cleaner contract: the ops are unimplementable rather
 *    than dangerous, and hal_audio.c leaves them NULL for a stated reason.
 *
 * 5. There are two gain stages and they are separate calls. MI_AI_SetIfGain is
 *    the interface's own analog gain (the mic preamp) and MI_AI_SetGain is the
 *    DPGA's digital gain. MI 2.x had one reachable knob, which is why
 *    star/hal_audio.c wires volume and gain to the same op and documents the
 *    collision; here they are genuinely independent and both are implemented.
 *
 * THE SoC ID RIDES IN THE DEVICE ARGUMENT.
 *
 * MI_AI follows VIF, SNR, ISP and SCL rather than MI_SYS and MI_RGN: there is no
 * separate SoC id parameter, and the wrapper takes it from the high halfword of
 * the device index. MI_AI_Open opens with
 *
 *     lsrs r7, r0, #16     @ SoC id
 *     uxtb r6, r0          @ device
 *
 * so the device is the low *byte* and the id the high halfword. Compose the
 * argument with I6C_DEV_ID() as the rest of the backend does.
 *
 * Copyright (C) 2026 Thingino Project
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SIGMASTAR_I6C_AUD_H
#define SIGMASTAR_I6C_AUD_H

#include "i6c_common.h"

/*
 * Physical channels a device can carry, and therefore the length of the
 * per-channel arrays in a frame descriptor. Eight is the vendor's
 * MI_AI_MAX_CHN_NUM, annotated "depend on multi-channel hardware ability" --
 * Maruko reaches it only through DMIC (6 channels) or TDM, never through the
 * two-channel ADC path this backend uses.
 */
#define I6C_AUD_MAX_CHN 8

/*
 * Sample format. Only S16_LE exists: the vendor header carries ten more arms
 * commented out, and the doc states outright that "currently, only S16_LE format
 * (PCM Linear 16bit (Little Endian)) is supported". INVALID is -1, which is what
 * makes this enum signed and keeps it 4 bytes wide.
 */
typedef enum {
    I6C_AUD_FMT_INVALID = -1,
    I6C_AUD_FMT_PCM_S16_LE = 0,
    I6C_AUD_FMT_END
} i6c_aud_fmt;

/*
 * How many physical channels one channel group holds. The value *is* the count,
 * so this doubles as the divisor in the group-count formula above. 4CH, 6CH and
 * 8CH are annotated "AI only" by the vendor -- capture can group more channels
 * than playback can.
 */
typedef enum {
    I6C_AUD_SND_MONO = 1,
    I6C_AUD_SND_STEREO = 2,
    I6C_AUD_SND_4CH = 4,
    I6C_AUD_SND_6CH = 6,
    I6C_AUD_SND_8CH = 8
} i6c_aud_snd;

/*
 * Sample rates, where the enumerator value is the rate in Hz rather than an
 * index -- so a rate from configuration can be assigned straight in, and an
 * unsupported one is caught by comparing against this list rather than by a
 * bounds check.
 *
 * Five of these are annotated "AO only" in the vendor header: 11025, 12000,
 * 22050, 24000 and 192000 play but do not capture. They are kept here so the
 * ordering stays checkable against the vendor header, and hal_audio.c refuses
 * them by name.
 */
typedef enum {
    I6C_AUD_RATE_8000 = 8000,
    I6C_AUD_RATE_11025 = 11025, /* AO only */
    I6C_AUD_RATE_12000 = 12000, /* AO only */
    I6C_AUD_RATE_16000 = 16000,
    I6C_AUD_RATE_22050 = 22050, /* AO only */
    I6C_AUD_RATE_24000 = 24000, /* AO only */
    I6C_AUD_RATE_32000 = 32000,
    I6C_AUD_RATE_44100 = 44100, /* AO only */
    I6C_AUD_RATE_48000 = 48000,
    I6C_AUD_RATE_96000 = 96000,
    I6C_AUD_RATE_192000 = 192000 /* AO only */
} i6c_aud_rate;

/*
 * Audio input interfaces -- the peripherals a device's multiplexer can select.
 * One enumerator is a *pair* of adjacent physical channels, which is why
 * MI_AI_AttachIf's array is bounded by MI_AI_MAX_CHN_NUM/2 and why attaching one
 * interface yields two physical channels.
 *
 * Spelled out in full because the values are positional and an attach with the
 * wrong one silently records silence from pins nothing is wired to. The vendor
 * header's own warning applies: "the order above is not allowed to change".
 *
 * What Maruko actually has (doc section 1.2.3): one DMIC interface carrying up
 * to 6 channels, two 2-channel ADCs, two 2-slot I2S RX -- and the two I2S RX
 * cannot be used at the same time. Everything else below exists in the enum
 * because the enum is shared across the Muffin and Mochi families.
 */
typedef enum {
    I6C_AUD_IF_NONE = 0,
    I6C_AUD_IF_ADC_AB = 1, /* ADC A + B, channel 0 of each -- the analog mic path */
    I6C_AUD_IF_ADC_CD = 2,
    I6C_AUD_IF_DMIC_A_01 = 3, /* DMIC channels 0 and 1 */
    I6C_AUD_IF_DMIC_A_23 = 4,
    I6C_AUD_IF_I2S_A_01 = 5,
    I6C_AUD_IF_I2S_A_23 = 6,
    I6C_AUD_IF_I2S_A_45 = 7,
    I6C_AUD_IF_I2S_A_67 = 8,
    I6C_AUD_IF_I2S_A_89 = 9,
    I6C_AUD_IF_I2S_A_AB = 10,
    I6C_AUD_IF_I2S_A_CD = 11,
    I6C_AUD_IF_I2S_A_EF = 12,
    I6C_AUD_IF_I2S_B_01 = 13,
    I6C_AUD_IF_I2S_B_23 = 14,
    I6C_AUD_IF_I2S_B_45 = 15,
    I6C_AUD_IF_I2S_B_67 = 16,
    I6C_AUD_IF_I2S_B_89 = 17,
    I6C_AUD_IF_I2S_B_AB = 18,
    I6C_AUD_IF_I2S_B_CD = 19,
    I6C_AUD_IF_I2S_B_EF = 20,
    I6C_AUD_IF_I2S_C_01 = 21,
    I6C_AUD_IF_I2S_C_23 = 22,
    I6C_AUD_IF_I2S_C_45 = 23,
    I6C_AUD_IF_I2S_C_67 = 24,
    I6C_AUD_IF_I2S_C_89 = 25,
    I6C_AUD_IF_I2S_C_AB = 26,
    I6C_AUD_IF_I2S_C_CD = 27,
    I6C_AUD_IF_I2S_C_EF = 28,
    I6C_AUD_IF_I2S_D_01 = 29,
    I6C_AUD_IF_I2S_D_23 = 30,
    I6C_AUD_IF_I2S_D_45 = 31,
    I6C_AUD_IF_I2S_D_67 = 32,
    I6C_AUD_IF_I2S_D_89 = 33,
    I6C_AUD_IF_I2S_D_AB = 34,
    I6C_AUD_IF_I2S_D_CD = 35,
    I6C_AUD_IF_I2S_D_EF = 36,
    /*
     * AEC reference: the resampled copy of what AO is playing. Must be attached
     * *after* at least one non-echo interface, and its channels do not count
     * toward the group total.
     */
    I6C_AUD_IF_ECHO_A = 37,
    I6C_AUD_IF_HDMI_A = 38,
    I6C_AUD_IF_DMIC_A_45 = 39,
    I6C_AUD_IF_END
} i6c_aud_if;

/*
 * MI_AI_Attr_t -- what MI_AI_Open takes. The data shape only; the input
 * interface is MI_AI_AttachIf's business.
 *
 * `interleaved` decides how a group's channels are laid out in the buffer. On
 * with a stereo group gives L R L R in one buffer; off gives each channel its
 * own buffer, which is what the per-channel arrays in i6c_aud_frm are for.
 *
 * MI_BOOL is `unsigned char`, so `interleaved` is one byte followed by three of
 * padding -- and MI_AI_Open reads the struct as five whole words
 * (`ldmia {r0,r1,r2,r3}` plus one `ldr`). Those three bytes therefore cross the
 * ioctl boundary. Clear this struct before filling it; the same trap as
 * MI_ISP_OutputPortParam_t in DERIVED.md, and the reason that section ends in a
 * rule rather than a question.
 */
typedef struct {
    i6c_aud_fmt format;
    i6c_aud_snd sound;
    i6c_aud_rate rate;
    unsigned int periodSize; /* samples per period, per physical channel */
    unsigned char interleaved;
} i6c_aud_cnf;

_Static_assert(sizeof(i6c_aud_cnf) == 20, "MI_AI_Open marshals 24 less a halfword dev and its pad");
_Static_assert(offsetof(i6c_aud_cnf, periodSize) == 12, "periodSize is the fourth word");

/*
 * MI_AI_Data_t -- one captured period, handed back by MI_AI_Read.
 *
 * Never crosses the ioctl boundary: MI_AI_Read's two commands carry 12- and
 * 4-byte payloads, and the library fills this struct in userspace from a mapped
 * buffer. That makes its interior observable in the shipped library rather than
 * opaque -- the same exception DERIVED.md describes for MI_VENC_Pack_t -- so
 * unusually for this family the size *and* two interior offsets are pinned:
 *
 *     memset(pstData, 0, 80)          @ the size
 *     vstr d16, [r7, #64]             @ u64Pts
 *     strd r0, r1, [r7, #72]          @ u64Seq
 *
 * With interleaved on, only addr[0] and length[0] are used.
 *
 * `length` is in *bytes*, not samples -- the vendor calls it u32Byte -- so a
 * 20 ms mono period at 16 kHz is 320 samples and 640 bytes.
 */
typedef struct {
    void *addr[I6C_AUD_MAX_CHN];
    unsigned int length[I6C_AUD_MAX_CHN];
    unsigned long long timestamp; /* microseconds */
    unsigned long long sequence;
} i6c_aud_frm;

_Static_assert(sizeof(i6c_aud_frm) == 80, "MI_AI_Read memsets 80 bytes of the caller's struct");
_Static_assert(offsetof(i6c_aud_frm, timestamp) == 64, "MI_AI_Read stores Pts at +64");
_Static_assert(offsetof(i6c_aud_frm, sequence) == 72, "MI_AI_Read stores Seq at +72");

#endif /* SIGMASTAR_I6C_AUD_H */
