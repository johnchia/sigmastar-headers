/*
 * i6_aud.h -- MI_AI (audio input) ABI for SigmaStar Infinity6E
 *
 * Layouts and symbol names come from divinus's src/hal/star/i6_aud.h
 * (MIT), independently corroborated by waybeam's star6e_audio.c, which
 * declares the same device-config and frame structs field for field.
 *
 * That corroboration matters because the vendored SigmaStar MI_AI
 * reference (ref/sigmastar-docs, MI AI API 2.19) documents
 * MI_AUDIO_Attr_t with a *different field order* -- eBitwidth first,
 * then eSamplerate, eSoundmode, eWorkmode, u32PtNumPerFrm, u32ChnCnt.
 * Those docs are for SSD20X, a different chip generation, so the doc is
 * the authority on **semantics** (what the fields mean, what values are
 * legal, what the calls guarantee) and the two references are the
 * authority on **layout** for this SoC. Do not "fix" the field order to
 * match the documentation.
 *
 * CAPTURE ONLY. There is deliberately no MI_AO here and no AI encoder:
 *
 *   - Nothing in scope plays audio out, so libmi_ao is never loaded.
 *   - MI's own encoder (MI_AI_SetAencAttr / EnableAenc / DisableAenc) is
 *     unused. divinus dlsym's all three and has zero call sites for any
 *     of them; waybeam does not load them at all. Both capture raw PCM
 *     and encode in userspace, and so does raptor -- rad has its own
 *     G.711/L16/Opus/AAC encoders. Omitting them keeps the symbol set to
 *     what is actually called.
 *   - The VQE / AEC / ANR / AGC / EQ / AED / SSL / BF entry points exist
 *     in libmi_ai.so but their implementations do not: the algorithm
 *     packs (IaaXxx_*, MI_AED_*, the SDK-side G711 and G726 helpers) are
 *     declared **weak undefined**, so they resolve to NULL rather than
 *     failing the link.
 *     Calling into them would jump to address 0. See hal_audio.c's OP
 *     COVERAGE comment.
 *
 * Copyright (C) 2026 Thingino Project
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SIGMASTAR_I6_AUD_H
#define SIGMASTAR_I6_AUD_H

#include "i6_common.h"

/* Tracks per AI channel. divinus's I6_AUD_CHN_NUM; the frame struct
 * carries one address per track, so the array bound is part of the ABI
 * and must not be trimmed even though this backend uses one or two. */
#define I6_AUD_CHN_NUM 16

typedef enum {
    I6_AUD_CLK_OFF,
    I6_AUD_CLK_12_288M,
    I6_AUD_CLK_16_384M,
    I6_AUD_CLK_18_432M,
    I6_AUD_CLK_24_576M,
    I6_AUD_CLK_24M,
    I6_AUD_CLK_48M
} i6_aud_clk;

typedef enum {
    I6_AUD_INTF_I2S_MASTER,
    I6_AUD_INTF_I2S_SLAVE,
    I6_AUD_INTF_TDM_MASTER,
    I6_AUD_INTF_TDM_SLAVE,
    I6_AUD_INTF_END
} i6_aud_intf;

typedef enum {
    I6_AUD_SND_MONO,
    I6_AUD_SND_STEREO,
    /* Several physical channels carried in one AI channel. Unused here:
     * per the MI docs every channel then shares one set of algorithm
     * parameters, and it only makes sense for a mic array. */
    I6_AUD_SND_QUEUE,
    I6_AUD_SND_END
} i6_aud_snd;

/*
 * MI_AUDIO_I2sConfig_t. 20 bytes here against the 0601 header's 12, and
 * the extra two fields are REQUIRED -- do not trim them to match the
 * header.
 *
 * They were trimmed once, on the reasoning that the header is the vendor's
 * own word and that the union is the last member of MI_AUDIO_Attr_t so the
 * fields ahead of it stay put. Both halves of that are true and the
 * conclusion is still wrong: MI_AI_SetPubAttr then fails with illegal
 * parameter and audio capture does not start at all. Measured by A/B on the
 * board, two builds seconds apart against the same device state.
 *
 * That A/B has since been confirmed by disassembly, which is the account to
 * trust. An earlier revision of this comment quoted the failure as
 * 0xa0058003, which is not an MI_AI code at all: MI_DEF_ERR packs MOD_ID
 * into bits 16-23, so that value is module 5 (AO) at an undefined level 8,
 * whereas AI/ERROR/ILLEGAL_PARAM is 0xA0042003. The disassembly does not
 * depend on remembering a code correctly -- MI_AI_SetPubAttr is a
 * validating wrapper around a single ioctl, and the length it copies into
 * the ioctl buffer IS the ABI:
 *
 *     board  /usr/lib/libmi_ai.so     memset 56, memcpy 52  -> union is 20
 *     SDK    release_0601 i6e build   memset 48, memcpy 44  -> union is 12
 *
 * Both builds validate the same field offsets (+0 rate, +4 bitwidth, +8
 * workmode, +12 soundmode, +20 packNumPerFrm), so the extra eight bytes are
 * genuinely on the end and every field ahead of the union is unmoved. The
 * SDK drop is simply older than the firmware, and the ioctl request number
 * encodes the size change too: 0x40306900 -> 0x40386900.
 *
 * So the board's libmi_ai.so reads 20 bytes where the SDK drop's header
 * declares 12, exactly as libmi_isp.so declares 28 for a 4-byte defog
 * struct. The rule both cases establish: where the shipped library and the
 * header disagree about a *size*, the library wins, because the library is
 * what does the reading. The header is still the better witness for field
 * order and enumerator values.
 *
 * tdmSlotNum and bit24On are SSC377-shaped names for whatever the extra
 * eight bytes are here; hal_audio.c leaves them zero and only sets the
 * three fields the header does declare.
 */
typedef struct {
    int leftJustOn;
    i6_aud_clk clock;
    char syncRxClkOn;
    unsigned int tdmSlotNum;
    int bit24On;
} i6_aud_i2s;

_Static_assert(sizeof(i6_aud_i2s) == 20,
               "MI_AI_SetPubAttr rejects a 12-byte I2S config -- see the comment above");

typedef struct {
    /* MI accepts 8/16/32/48 kHz only -- the docs say so explicitly, and
     * the resampler that would cover anything else (IaaSrc_*) is one of
     * the weak-undefined algorithm entry points. hal_audio.c rejects
     * other rates rather than letting MI resample into a NULL call. */
    int rate;
    int bit24On;
    i6_aud_intf intf;
    i6_aud_snd sound;
    /* DMA ring depth in frames, and samples per frame per channel. */
    unsigned int frmNum;
    unsigned int packNumPerFrm;
    /* Zero: no SDK codec channel. Both references set this to 0. */
    unsigned int codecChnNum;
    unsigned int chnNum;
    i6_aud_i2s i2s;
} i6_aud_cnf;

/*
 * MI_AI_ChnParam_t, and the two gain stages it reaches.
 *
 * Unlike everything VQE, this one is real: MI_AI_SetChnParam is exported by
 * the board's libmi_ai.so and is not a stub -- it validates, then does
 * memcpy(buf + 8, param, 12) into a 20-byte ioctl block (4 device + 4
 * channel + 12 payload). 12 is exactly what the SDK header declares for
 * MI_AI_ChnParam_t, so here the header and the library agree and the layout
 * below is simply the header's.
 *
 * The two gains are *different stages*, which is the whole reason this
 * struct is worth binding:
 *
 *   front  the analog front-end step, and the SAME table MI_AI_SetVqeVolume
 *          indexes. The library's range check is per device and matches the
 *          MI_AI reference's published table exactly -- dev 0 (Amic) and
 *          dev 4 accept [0,21], dev 3 (Line in) [0,7], dev 2 is refused.
 *          So writing this field fights MI_AI_SetVqeVolume; hal_audio.c
 *          preserves whatever is already there rather than setting it.
 *
 *   rear   a digital trim in whole dB, range [-60,+30], reachable ONLY
 *          through this call. That range is not a guess: it is what the
 *          library compares against, and it is the same span the Infinity6C
 *          backend documents for its DPGA.
 *
 * bEnableGainSet gates both -- the library skips the range checks entirely
 * when it is zero, so a cleared struct is "leave the gains alone".
 */
typedef struct {
    unsigned char gainOn;
    short front;
    short rear;
} i6_aud_chn_gain;

typedef struct {
    i6_aud_chn_gain gain;
    unsigned int reserved;
} i6_aud_chn_para;

_Static_assert(sizeof(i6_aud_chn_para) == 12, "MI_AI_SetChnParam copies 12");
_Static_assert(offsetof(i6_aud_chn_para, gain.front) == 2, "s16FrontGain sits at +2");
_Static_assert(offsetof(i6_aud_chn_para, gain.rear) == 4, "s16RearGain sits at +4");
_Static_assert(offsetof(i6_aud_chn_para, reserved) == 8, "u32Reserved sits at +8");

typedef struct {
    int bit24On;
    i6_aud_snd sound;
    unsigned char *addr[I6_AUD_CHN_NUM];
    unsigned long long timestamp;
    unsigned int sequence;
    unsigned int length;
    unsigned int poolId[2];
    unsigned char *pcmAddr[I6_AUD_CHN_NUM];
    unsigned int pcmLength;
} i6_aud_frm;

/* The AEC reference frame. Passed as NULL by both references and by this
 * backend -- echo cancellation is one of the absent algorithms -- but it
 * is part of the GetFrame/ReleaseFrame signatures, so the type has to
 * exist. */
typedef struct {
    i6_aud_frm frame;
    char isValid;
} i6_aud_efrm;

#endif /* SIGMASTAR_I6_AUD_H */
