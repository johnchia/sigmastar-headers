/*
 * i6c_sys.h -- SigmaStar MI_SYS types, Infinity6C
 *
 * Written from the vendor SDK's own mi_sys.h and mi_sys_datatype.h for the
 * release the target's libraries were built from, and expressed
 * independently: the names, grouping and commentary here are this
 * repository's, and no third-party transcription was copied into them.
 *
 * That is possible for this family and was not for infinity6e. The Infinity6C
 * SDK drop is available, and its prebuilt MI libraries are byte-identical to
 * the ones the target ships -- same project, sdk and mhal commits -- so the
 * vendor interface can be read directly rather than inferred from a HAL that
 * had to work it out from a binary. Where the infinity6e headers next door
 * follow divinus's names to stay diffable against upstream, this family
 * follows the vendor's, because the vendor is what it will be checked
 * against.
 *
 * Layouts are pinned by _Static_assert rather than trusted. A wrong one
 * corrupts memory instead of failing to compile, which is the failure this
 * repository exists to make impossible.
 *
 * THE CALLING CONVENTION CHANGED, AND IT IS INVISIBLE
 *
 * Every MI_SYS entry point on this generation leads with a 16-bit SoC id that
 * MI 2.x has no equivalent of:
 *
 *     MI_SYS_Init(MI_U16 u16SocId)
 *     MI_SYS_GetVersion(MI_U16 u16SocId, MI_SYS_Version_t *)
 *     MI_SYS_BindChnPort2(MI_U16 u16SocId, src, dst, srcFps, dstFps)
 *
 * It selects a die on a multi-die part and is 0 on a single-die camera.
 * Nothing catches getting this wrong. The libraries are reached through
 * dlsym, which resolves by name, so an MI 2.x function table binds against
 * them without complaint and then calls MI_SYS_Init with whatever occupied
 * the first argument register. The argument lists themselves are declared in
 * the consumer's loader, so the warning belongs here with the types it uses.
 *
 * Declarations only. MI is reached by dlopen/dlsym rather than by linking
 * -lmi_*, and the loaders that do so are the consumer's business -- nothing
 * here includes dlfcn.h or depends on a logger or an error-code set.
 *
 * Copyright (C) 2026 Thingino Project
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SIGMASTAR_I6C_SYS_H
#define SIGMASTAR_I6C_SYS_H

/*
 * MI_SYS_Version_t.
 *
 * A fixed 128-byte field carrying the vendor's build stamp, of the form
 *
 *     Sigmastar Module mi_sys version: project_commit.<x> sdk_commit.<y>
 *     mhal_commit.<z> build_time.<yyyymmddhhmmss>
 *
 * Not guaranteed NUL-terminated, so read it bounded by the array rather than
 * with the string functions. Worth reading rather than skipping: build_time is
 * what confirms the loaded libraries are the drop a consumer's declarations
 * were written against.
 */
typedef struct {
    unsigned char version[128];
} i6c_sys_version;

_Static_assert(sizeof(i6c_sys_version) == 128,
               "i6c_sys_version is the 128-byte buffer MI_SYS_GetVersion fills");

#endif /* SIGMASTAR_I6C_SYS_H */
