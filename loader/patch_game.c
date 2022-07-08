/*
 * patch_game.c
 *
 * Patching some of the .so internal functions or bridging them to native for
 * better compatibility.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 * Copyright (C) 2022 psykana
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "patch_game.h"

#if GRAPHICS_API == GRAPHICS_API_PVR
#include <GLES2/gl2.h>
#else
#include <vitaGL.h>
#endif

#include <kubridge.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

#include "main.h"
#include "utils/utils.h"
#include "jni.h"

#include "reimpl/log.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#include "stb_vorbis.c"
#include "utils/loading_screen.h"
#include "libc_bridge.h"

#define SDL_ANDROID_EXTERNAL_STORAGE_READ   0x01
#define SDL_ANDROID_EXTERNAL_STORAGE_WRITE  0x02

#pragma ide diagnostic ignored "UnusedParameter"
#pragma ide diagnostic ignored "OCDFAInspection"

extern void *__cxa_guard_abort; // NOLINT(bugprone-reserved-identifier)
extern void *__cxa_guard_acquire; // NOLINT(bugprone-reserved-identifier)
extern void *__cxa_guard_release; // NOLINT(bugprone-reserved-identifier)

SDL_bool SDL_AndroidRequestPermission(const char *permission) {
    debugPrintf("[LOG][SDL] Requested permission: %s\n", permission);
    return SDL_TRUE;
}

SDL_bool SDL_IsAndroidTV(void) {
    return SDL_FALSE;
}

SDL_bool SDL_IsChromebook(void) {
    return SDL_FALSE;
}

SDL_bool SDL_IsDeXMode(void) {
    return SDL_FALSE;
}

int SDL_GetAndroidSDKVersion(void) {
    return 10;
}

void SDL_AndroidBackButton(void) {
    debugPrintf("SDL_AndroidBackButton\n");
}

void * SDL_AndroidGetActivity(void) {
    debugPrintf("SDL_AndroidGetActivity\n");
    return NULL;
}

char *SDL_AndroidGetExternalStoragePath() {
    debugPrintf("SDL_AndroidGetExternalStoragePath\n");
    return DATA_PATH_INT;
}

char *SDL_AndroidGetInternalStoragePath() {
    debugPrintf("SDL_AndroidGetInternalStoragePath\n");
    return DATA_PATH_INT;
}

int SDL_AndroidGetExternalStorageState(void) {
    debugPrintf("SDL_AndroidGetExternalStorageState\n");
    return SDL_ANDROID_EXTERNAL_STORAGE_READ &
           SDL_ANDROID_EXTERNAL_STORAGE_WRITE;
}

int SDL_AndroidShowToast(const char* message, int d, int g, int x, int y) {
    debugPrintf("[LOG][SDL_TOAST] %s\n", message);
    return 0;
}

void * SDL_AndroidGetJNIEnv(void) {
    return fake_env;
}

SDL_Window * SDL_CreateWindow_fake(const char *title,
                                   int x, int y, int w,
                                   int h, Uint32 flags) {
    loading_screen_quit();
    return SDL_CreateWindow(title, x, y, w, h, flags);
}

int SDL_Init_fake(Uint32 flags) {
    debugPrintf("SDL_Init called\n");
    SDL_setenv("VITA_DISABLE_TOUCH_BACK", "1", 1);

    int r = SDL_Init(flags);
    SDL_GameControllerAddMapping(
            "50535669746120436f6e74726f6c6c65,PSVita Controller,a:b2,b:b0,back:b4,dpdown:b6,dpleft:b7,dpright:b9,dpup:b8,leftshoulder:b10,leftstick:b14,lefttrigger:a4,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b15,righttrigger:a5,rightx:a2,righty:a3,start:b11,x:b3,y:b1,");
    return r;
}

const char * platform_get_language(void) {
    debugPrintf("requested platform_get_language\n");
    return "English";
}

void bind_tex(uint Tex) {
    glBindTexture(0xde1, Tex);
}

static uint last_tex;

void set_tex(uint Tex)
{
    if (last_tex != Tex) {
        gl_flush_cache();
        fprintf(stderr, "[vgldbg] glBindTexture(%x, %x)\n", 0xde1, Tex);
        glBindTexture(0xde1, Tex);
        last_tex = Tex;
    }
}

SDL_RWops* SDL_RWFromFile_fake(const char *file, const char *mode) {
    if (string_ends_with(file, "gamecontrollerdb.txt") && !file_exists(file)) {
        debugPrintf("Detected gamecontrollerdb.txt, redirecting\n");
        char path_real[256];
        snprintf(path_real, 256, "%s/Data/gamecontrollerdb.txt", DATA_PATH_INT);
        return SDL_RWFromFile(path_real, mode);
    }
    return SDL_RWFromFile(file, mode);
}

FILE* fopen_nx(const char* fname) {
    debugPrintf("fopen nx: %s\n", fname);
    char path_real[256];
    snprintf(path_real, 256, "%s/%s", DATA_PATH_INT, fname);
    return fopen(path_real, "r");
}

void patch_game(void) {
    

    // game-specific
    {
        // Nuke side menu
        hook_addr(so_symbol(&so_mod, "_Z13baba_get_viewPiS_S_S_ii"), (uintptr_t)&do_nothing);

#if GRAPHICS_API==GRAPHICS_API_PVR
        hook_addr(so_symbol(&goo_mod, "_Z8bind_texj"), (uintptr_t)&bind_tex);
        hook_addr(so_symbol(&goo_mod, "_Z7set_texj"), (uintptr_t)&set_tex);
#endif
        hook_addr(so_symbol(&so_mod, "__cxa_guard_abort"), (uintptr_t)&__cxa_guard_abort);
        hook_addr(so_symbol(&so_mod, "__cxa_guard_acquire"), (uintptr_t)&__cxa_guard_acquire);
        hook_addr(so_symbol(&so_mod, "__cxa_guard_release"), (uintptr_t)&__cxa_guard_release);
        hook_addr(so_symbol(&so_mod, "fopen_nx"), (uintptr_t)&fopen_nx);
        hook_addr(so_symbol(&so_mod, "mbedtls_debug_print_msg"), (uintptr_t)&mbedtls_debug_print_msg);
        hook_addr(so_symbol(&so_mod, "platform_get_language"), (uintptr_t)&platform_get_language);
    }

    // stb_image
    {
        hook_addr(so_symbol(&so_mod, "stbi_convert_iphone_png_to_rgb"), (uintptr_t)&stbi_convert_iphone_png_to_rgb);
        hook_addr(so_symbol(&so_mod, "stbi_failure_reason"), (uintptr_t)&stbi_failure_reason);
        hook_addr(so_symbol(&so_mod, "stbi_hdr_to_ldr_gamma"), (uintptr_t)&stbi_hdr_to_ldr_gamma);
        hook_addr(so_symbol(&so_mod, "stbi_hdr_to_ldr_scale"), (uintptr_t)&stbi_hdr_to_ldr_scale);
        hook_addr(so_symbol(&so_mod, "stbi_image_free"), (uintptr_t)&stbi_image_free);
        hook_addr(so_symbol(&so_mod, "stbi_info_from_callbacks"), (uintptr_t)&stbi_info_from_callbacks);
        hook_addr(so_symbol(&so_mod, "stbi_info_from_memory"), (uintptr_t)&stbi_info_from_memory);
        hook_addr(so_symbol(&so_mod, "stbi_is_16_bit_from_callbacks"), (uintptr_t)&stbi_is_16_bit_from_callbacks);
        hook_addr(so_symbol(&so_mod, "stbi_is_16_bit_from_memory"), (uintptr_t)&stbi_is_16_bit_from_memory);
        hook_addr(so_symbol(&so_mod, "stbi_is_hdr_from_callbacks"), (uintptr_t)&stbi_is_hdr_from_callbacks);
        hook_addr(so_symbol(&so_mod, "stbi_is_hdr_from_memory"), (uintptr_t)&stbi_is_hdr_from_memory);
        hook_addr(so_symbol(&so_mod, "stbi_ldr_to_hdr_gamma"), (uintptr_t)&stbi_ldr_to_hdr_gamma);
        hook_addr(so_symbol(&so_mod, "stbi_ldr_to_hdr_scale"), (uintptr_t)&stbi_ldr_to_hdr_scale);
        hook_addr(so_symbol(&so_mod, "stbi_load_16_from_callbacks"), (uintptr_t)&stbi_load_16_from_callbacks);
        hook_addr(so_symbol(&so_mod, "stbi_load_16_from_memory"), (uintptr_t)&stbi_load_16_from_memory);
        hook_addr(so_symbol(&so_mod, "stbi_load_from_callbacks"), (uintptr_t)&stbi_load_from_callbacks);
        hook_addr(so_symbol(&so_mod, "stbi_load_from_memory"), (uintptr_t)&stbi_load_from_memory);
        hook_addr(so_symbol(&so_mod, "stbi_load_gif_from_memory"), (uintptr_t)&stbi_load_gif_from_memory);
        hook_addr(so_symbol(&so_mod, "stbi_loadf_from_callbacks"), (uintptr_t)&stbi_loadf_from_callbacks);
        hook_addr(so_symbol(&so_mod, "stbi_loadf_from_memory"), (uintptr_t)&stbi_loadf_from_memory);
        hook_addr(so_symbol(&so_mod, "stbi_set_flip_vertically_on_load"), (uintptr_t)&stbi_set_flip_vertically_on_load);
        hook_addr(so_symbol(&so_mod, "stbi_set_unpremultiply_on_load"), (uintptr_t)&stbi_set_unpremultiply_on_load);
        hook_addr(so_symbol(&so_mod, "stbi_write_png_to_mem"), (uintptr_t)&stbi_write_png_to_mem);
        hook_addr(so_symbol(&so_mod, "stbi_zlib_compress"), (uintptr_t)&stbi_zlib_compress);
        hook_addr(so_symbol(&so_mod, "stbi_zlib_decode_buffer"), (uintptr_t)&stbi_zlib_decode_buffer);
        hook_addr(so_symbol(&so_mod, "stbi_zlib_decode_malloc"), (uintptr_t)&stbi_zlib_decode_malloc);
        hook_addr(so_symbol(&so_mod, "stbi_zlib_decode_malloc_guesssize"), (uintptr_t)&stbi_zlib_decode_malloc_guesssize);
        hook_addr(so_symbol(&so_mod, "stbi_zlib_decode_malloc_guesssize_headerflag"), (uintptr_t)&stbi_zlib_decode_malloc_guesssize_headerflag);
        hook_addr(so_symbol(&so_mod, "stbi_zlib_decode_noheader_buffer"), (uintptr_t)&stbi_zlib_decode_noheader_buffer);
        hook_addr(so_symbol(&so_mod, "stbi_zlib_decode_noheader_malloc"), (uintptr_t)&stbi_zlib_decode_noheader_malloc);
        hook_addr(so_symbol(&so_mod, "_Z18stbi_zlib_compressPhiPii"), (uintptr_t)&stbi_zlib_compress);
        hook_addr(so_symbol(&so_mod, "_Z21stbi_hdr_to_ldr_gammaf"), (uintptr_t)&stbi_hdr_to_ldr_gamma);
        hook_addr(so_symbol(&so_mod, "_Z21stbi_hdr_to_ldr_scalef"), (uintptr_t)&stbi_hdr_to_ldr_scale);
        hook_addr(so_symbol(&so_mod, "_Z21stbi_write_png_to_memPhiiiiPi"), (uintptr_t)&stbi_write_png_to_mem);
        hook_addr(so_symbol(&so_mod, "_Z21stbiw__write_run_dataP19stbi__write_contextih"), (uintptr_t)&stbiw__write_run_data);
        hook_addr(so_symbol(&so_mod, "_Z22stbiw__write_dump_dataP19stbi__write_contextiPh"), (uintptr_t)&stbiw__write_dump_data);
        hook_addr(so_symbol(&so_mod, "_Z25stbiw__write_hdr_scanlineP19stbi__write_contextiiPhPf"), (uintptr_t)&stbiw__write_hdr_scanline);
    }

    // stb_vorbis
    {
        hook_addr(so_symbol(&so_mod, "stb_vorbis_close"), (uintptr_t)&stb_vorbis_close);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_decode_filename"), (uintptr_t)&stb_vorbis_decode_filename);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_decode_memory"), (uintptr_t)&stb_vorbis_decode_memory);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_get_comment"), (uintptr_t)&stb_vorbis_get_comment);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_get_error"), (uintptr_t)&stb_vorbis_get_error);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_get_file_offset"), (uintptr_t)&stb_vorbis_get_file_offset);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_get_frame_float"), (uintptr_t)&stb_vorbis_get_frame_float);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_get_frame_short"), (uintptr_t)&stb_vorbis_get_frame_short);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_get_frame_short_interleaved"), (uintptr_t)&stb_vorbis_get_frame_short_interleaved);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_get_info"), (uintptr_t)&stb_vorbis_get_info);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_get_sample_offset"), (uintptr_t)&stb_vorbis_get_sample_offset);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_get_samples_float"), (uintptr_t)&stb_vorbis_get_samples_float);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_get_samples_float_interleaved"), (uintptr_t)&stb_vorbis_get_samples_float_interleaved);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_get_samples_short"), (uintptr_t)&stb_vorbis_get_samples_short);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_get_samples_short_interleaved"), (uintptr_t)&stb_vorbis_get_samples_short_interleaved);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_open_file"), (uintptr_t)&stb_vorbis_open_file);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_open_file_section"), (uintptr_t)&stb_vorbis_open_file_section);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_open_filename"), (uintptr_t)&stb_vorbis_open_filename);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_open_memory"), (uintptr_t)&stb_vorbis_open_memory);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_seek"), (uintptr_t)&stb_vorbis_seek);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_seek_frame"), (uintptr_t)&stb_vorbis_seek_frame);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_seek_start"), (uintptr_t)&stb_vorbis_seek_start);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_stream_length_in_samples"), (uintptr_t)&stb_vorbis_stream_length_in_samples);
        hook_addr(so_symbol(&so_mod, "stb_vorbis_stream_length_in_seconds"), (uintptr_t)&stb_vorbis_stream_length_in_seconds);
    }

    // SDL2
    {
        hook_addr(so_symbol(&so_mod, "SDL_abs"), (uintptr_t)&SDL_abs);
        hook_addr(so_symbol(&so_mod, "SDL_acos"), (uintptr_t)&SDL_acos);
        hook_addr(so_symbol(&so_mod, "SDL_acosf"), (uintptr_t)&SDL_acosf);
        hook_addr(so_symbol(&so_mod, "SDL_AddEventWatch"), (uintptr_t)&SDL_AddEventWatch);
        hook_addr(so_symbol(&so_mod, "SDL_AddHintCallback"), (uintptr_t)&SDL_AddHintCallback);
        hook_addr(so_symbol(&so_mod, "SDL_AddTimer"), (uintptr_t)&SDL_AddTimer);
        hook_addr(so_symbol(&so_mod, "SDL_AllocFormat"), (uintptr_t)&SDL_AllocFormat);
        hook_addr(so_symbol(&so_mod, "SDL_AllocPalette"), (uintptr_t)&SDL_AllocPalette);
        hook_addr(so_symbol(&so_mod, "SDL_AllocRW"), (uintptr_t)&SDL_AllocRW);
        hook_addr(so_symbol(&so_mod, "SDL_AndroidBackButton"), (uintptr_t)&SDL_AndroidBackButton);
        hook_addr(so_symbol(&so_mod, "SDL_AndroidGetActivity"), (uintptr_t)&SDL_AndroidGetActivity);
        hook_addr(so_symbol(&so_mod, "SDL_AndroidGetExternalStoragePath"), (uintptr_t)&SDL_AndroidGetExternalStoragePath);
        hook_addr(so_symbol(&so_mod, "SDL_AndroidGetExternalStorageState"), (uintptr_t)&SDL_AndroidGetExternalStorageState);
        hook_addr(so_symbol(&so_mod, "SDL_AndroidGetInternalStoragePath"), (uintptr_t)&SDL_AndroidGetInternalStoragePath);
        hook_addr(so_symbol(&so_mod, "SDL_AndroidGetJNIEnv"), (uintptr_t)&SDL_AndroidGetJNIEnv);
        hook_addr(so_symbol(&so_mod, "SDL_AndroidRequestPermission"), (uintptr_t)&SDL_AndroidRequestPermission);
        hook_addr(so_symbol(&so_mod, "SDL_AndroidShowToast"), (uintptr_t)&SDL_AndroidShowToast);
        hook_addr(so_symbol(&so_mod, "SDL_asin"), (uintptr_t)&SDL_asin);
        hook_addr(so_symbol(&so_mod, "SDL_asinf"), (uintptr_t)&SDL_asinf);
        hook_addr(so_symbol(&so_mod, "SDL_atan"), (uintptr_t)&SDL_atan);
        hook_addr(so_symbol(&so_mod, "SDL_atan2"), (uintptr_t)&SDL_atan2);
        hook_addr(so_symbol(&so_mod, "SDL_atan2f"), (uintptr_t)&SDL_atan2f);
        hook_addr(so_symbol(&so_mod, "SDL_atanf"), (uintptr_t)&SDL_atanf);
        hook_addr(so_symbol(&so_mod, "SDL_atof"), (uintptr_t)&SDL_atof);
        hook_addr(so_symbol(&so_mod, "SDL_atoi"), (uintptr_t)&SDL_atoi);
        hook_addr(so_symbol(&so_mod, "SDL_AtomicAdd"), (uintptr_t)&SDL_AtomicAdd);
        hook_addr(so_symbol(&so_mod, "SDL_AtomicCAS"), (uintptr_t)&SDL_AtomicCAS);
        hook_addr(so_symbol(&so_mod, "SDL_AtomicCASPtr"), (uintptr_t)&SDL_AtomicCASPtr);
        hook_addr(so_symbol(&so_mod, "SDL_AtomicGet"), (uintptr_t)&SDL_AtomicGet);
        hook_addr(so_symbol(&so_mod, "SDL_AtomicGetPtr"), (uintptr_t)&SDL_AtomicGetPtr);
        hook_addr(so_symbol(&so_mod, "SDL_AtomicLock"), (uintptr_t)&SDL_AtomicLock);
        hook_addr(so_symbol(&so_mod, "SDL_AtomicSet"), (uintptr_t)&SDL_AtomicSet);
        hook_addr(so_symbol(&so_mod, "SDL_AtomicSetPtr"), (uintptr_t)&SDL_AtomicSetPtr);
        hook_addr(so_symbol(&so_mod, "SDL_AtomicTryLock"), (uintptr_t)&SDL_AtomicTryLock);
        hook_addr(so_symbol(&so_mod, "SDL_AtomicUnlock"), (uintptr_t)&SDL_AtomicUnlock);
        hook_addr(so_symbol(&so_mod, "SDL_AudioInit"), (uintptr_t)&SDL_AudioInit);
        hook_addr(so_symbol(&so_mod, "SDL_AudioQuit"), (uintptr_t)&SDL_AudioQuit);
        hook_addr(so_symbol(&so_mod, "SDL_AudioStreamAvailable"), (uintptr_t)&SDL_AudioStreamAvailable);
        hook_addr(so_symbol(&so_mod, "SDL_AudioStreamClear"), (uintptr_t)&SDL_AudioStreamClear);
        hook_addr(so_symbol(&so_mod, "SDL_AudioStreamFlush"), (uintptr_t)&SDL_AudioStreamFlush);
        hook_addr(so_symbol(&so_mod, "SDL_AudioStreamGet"), (uintptr_t)&SDL_AudioStreamGet);
        hook_addr(so_symbol(&so_mod, "SDL_AudioStreamPut"), (uintptr_t)&SDL_AudioStreamPut);
        hook_addr(so_symbol(&so_mod, "SDL_BuildAudioCVT"), (uintptr_t)&SDL_BuildAudioCVT);
        hook_addr(so_symbol(&so_mod, "SDL_CalculateGammaRamp"), (uintptr_t)&SDL_CalculateGammaRamp);
        hook_addr(so_symbol(&so_mod, "SDL_calloc"), (uintptr_t)&SDL_calloc);
        hook_addr(so_symbol(&so_mod, "SDL_CaptureMouse"), (uintptr_t)&SDL_CaptureMouse);
        hook_addr(so_symbol(&so_mod, "SDL_ceil"), (uintptr_t)&SDL_ceil);
        hook_addr(so_symbol(&so_mod, "SDL_ceilf"), (uintptr_t)&SDL_ceilf);
        hook_addr(so_symbol(&so_mod, "SDL_ClearError"), (uintptr_t)&SDL_ClearError);
        hook_addr(so_symbol(&so_mod, "SDL_ClearHints"), (uintptr_t)&SDL_ClearHints);
        hook_addr(so_symbol(&so_mod, "SDL_ClearQueuedAudio"), (uintptr_t)&SDL_ClearQueuedAudio);
        hook_addr(so_symbol(&so_mod, "SDL_CloseAudio"), (uintptr_t)&SDL_CloseAudio);
        hook_addr(so_symbol(&so_mod, "SDL_CloseAudioDevice"), (uintptr_t)&SDL_CloseAudioDevice);
        hook_addr(so_symbol(&so_mod, "SDL_ComposeCustomBlendMode"), (uintptr_t)&SDL_ComposeCustomBlendMode);
        hook_addr(so_symbol(&so_mod, "SDL_CondBroadcast"), (uintptr_t)&SDL_CondBroadcast);
        hook_addr(so_symbol(&so_mod, "SDL_CondSignal"), (uintptr_t)&SDL_CondSignal);
        hook_addr(so_symbol(&so_mod, "SDL_CondWait"), (uintptr_t)&SDL_CondWait);
        hook_addr(so_symbol(&so_mod, "SDL_CondWaitTimeout"), (uintptr_t)&SDL_CondWaitTimeout);
        /*hook_addr(so_symbol(&goo_mod, "SDL_Convert_F32_to_S16"), (uintptr_t)&SDL_Convert_F32_to_S16);
        hook_addr(so_symbol(&goo_mod, "SDL_Convert_F32_to_S32"), (uintptr_t)&SDL_Convert_F32_to_S32);
        hook_addr(so_symbol(&goo_mod, "SDL_Convert_F32_to_S8"), (uintptr_t)&SDL_Convert_F32_to_S8);
        hook_addr(so_symbol(&goo_mod, "SDL_Convert_F32_to_U16"), (uintptr_t)&SDL_Convert_F32_to_U16);
        hook_addr(so_symbol(&goo_mod, "SDL_Convert_F32_to_U8"), (uintptr_t)&SDL_Convert_F32_to_U8);
        hook_addr(so_symbol(&goo_mod, "SDL_Convert_S16_to_F32"), (uintptr_t)&SDL_Convert_S16_to_F32);
        hook_addr(so_symbol(&goo_mod, "SDL_Convert_S32_to_F32"), (uintptr_t)&SDL_Convert_S32_to_F32);
        hook_addr(so_symbol(&goo_mod, "SDL_Convert_S8_to_F32"), (uintptr_t)&SDL_Convert_S8_to_F32);
        hook_addr(so_symbol(&goo_mod, "SDL_Convert_U16_to_F32"), (uintptr_t)&SDL_Convert_U16_to_F32);
        hook_addr(so_symbol(&goo_mod, "SDL_Convert_U8_to_F32"), (uintptr_t)&SDL_Convert_U8_to_F32);*/
        hook_addr(so_symbol(&so_mod, "SDL_ConvertAudio"), (uintptr_t)&SDL_ConvertAudio);
        hook_addr(so_symbol(&so_mod, "SDL_ConvertPixels"), (uintptr_t)&SDL_ConvertPixels);
        hook_addr(so_symbol(&so_mod, "SDL_ConvertSurface"), (uintptr_t)&SDL_ConvertSurface);
        hook_addr(so_symbol(&so_mod, "SDL_ConvertSurfaceFormat"), (uintptr_t)&SDL_ConvertSurfaceFormat);
        hook_addr(so_symbol(&so_mod, "SDL_copysign"), (uintptr_t)&SDL_copysign);
        hook_addr(so_symbol(&so_mod, "SDL_copysignf"), (uintptr_t)&SDL_copysignf);
        hook_addr(so_symbol(&so_mod, "SDL_cos"), (uintptr_t)&SDL_cos);
        hook_addr(so_symbol(&so_mod, "SDL_cosf"), (uintptr_t)&SDL_cosf);
        hook_addr(so_symbol(&so_mod, "SDL_crc32"), (uintptr_t)&SDL_crc32);
        hook_addr(so_symbol(&so_mod, "SDL_CreateColorCursor"), (uintptr_t)&SDL_CreateColorCursor);
        hook_addr(so_symbol(&so_mod, "SDL_CreateCond"), (uintptr_t)&SDL_CreateCond);
        hook_addr(so_symbol(&so_mod, "SDL_CreateCursor"), (uintptr_t)&SDL_CreateCursor);
        hook_addr(so_symbol(&so_mod, "SDL_CreateMutex"), (uintptr_t)&SDL_CreateMutex);
        hook_addr(so_symbol(&so_mod, "SDL_CreateRenderer"), (uintptr_t)&SDL_CreateRenderer);
        hook_addr(so_symbol(&so_mod, "SDL_CreateRGBSurface"), (uintptr_t)&SDL_CreateRGBSurface);
        hook_addr(so_symbol(&so_mod, "SDL_CreateRGBSurfaceFrom"), (uintptr_t)&SDL_CreateRGBSurfaceFrom);
        hook_addr(so_symbol(&so_mod, "SDL_CreateRGBSurfaceWithFormat"), (uintptr_t)&SDL_CreateRGBSurfaceWithFormat);
        hook_addr(so_symbol(&so_mod, "SDL_CreateRGBSurfaceWithFormatFrom"), (uintptr_t)&SDL_CreateRGBSurfaceWithFormatFrom);
        hook_addr(so_symbol(&so_mod, "SDL_CreateSemaphore"), (uintptr_t)&SDL_CreateSemaphore);
        hook_addr(so_symbol(&so_mod, "SDL_CreateShapedWindow"), (uintptr_t)&SDL_CreateShapedWindow);
        hook_addr(so_symbol(&so_mod, "SDL_CreateSoftwareRenderer"), (uintptr_t)&SDL_CreateSoftwareRenderer);
        hook_addr(so_symbol(&so_mod, "SDL_CreateSystemCursor"), (uintptr_t)&SDL_CreateSystemCursor);
        hook_addr(so_symbol(&so_mod, "SDL_CreateTexture"), (uintptr_t)&SDL_CreateTexture);
        hook_addr(so_symbol(&so_mod, "SDL_CreateTextureFromSurface"), (uintptr_t)&SDL_CreateTextureFromSurface);
        hook_addr(so_symbol(&so_mod, "SDL_CreateThread"), (uintptr_t)&SDL_CreateThread);
        hook_addr(so_symbol(&so_mod, "SDL_CreateThreadWithStackSize"), (uintptr_t)&SDL_CreateThreadWithStackSize);
        hook_addr(so_symbol(&so_mod, "SDL_CreateWindow"), (uintptr_t)&SDL_CreateWindow_fake);
        hook_addr(so_symbol(&so_mod, "SDL_CreateWindowAndRenderer"), (uintptr_t)&SDL_CreateWindowAndRenderer);
        hook_addr(so_symbol(&so_mod, "SDL_CreateWindowFrom"), (uintptr_t)&SDL_CreateWindowFrom);
        hook_addr(so_symbol(&so_mod, "SDL_Delay"), (uintptr_t)&SDL_Delay);
        hook_addr(so_symbol(&so_mod, "SDL_DelEventWatch"), (uintptr_t)&SDL_DelEventWatch);
        hook_addr(so_symbol(&so_mod, "SDL_DelHintCallback"), (uintptr_t)&SDL_DelHintCallback);
        hook_addr(so_symbol(&so_mod, "SDL_DequeueAudio"), (uintptr_t)&SDL_DequeueAudio);
        hook_addr(so_symbol(&so_mod, "SDL_DestroyCond"), (uintptr_t)&SDL_DestroyCond);
        hook_addr(so_symbol(&so_mod, "SDL_DestroyMutex"), (uintptr_t)&SDL_DestroyMutex);
        hook_addr(so_symbol(&so_mod, "SDL_DestroyRenderer"), (uintptr_t)&SDL_DestroyRenderer);
        hook_addr(so_symbol(&so_mod, "SDL_DestroySemaphore"), (uintptr_t)&SDL_DestroySemaphore);
        hook_addr(so_symbol(&so_mod, "SDL_DestroyTexture"), (uintptr_t)&SDL_DestroyTexture);
        hook_addr(so_symbol(&so_mod, "SDL_DestroyWindow"), (uintptr_t)&SDL_DestroyWindow);
        hook_addr(so_symbol(&so_mod, "SDL_DetachThread"), (uintptr_t)&SDL_DetachThread);
        hook_addr(so_symbol(&so_mod, "SDL_DisableScreenSaver"), (uintptr_t)&SDL_DisableScreenSaver);
        hook_addr(so_symbol(&so_mod, "SDL_DuplicateSurface"), (uintptr_t)&SDL_DuplicateSurface);
        //hook_addr(so_symbol(&goo_mod, "SDL_DYNAPI_entry"), (uintptr_t)&SDL_DYNAPI_entry);
        hook_addr(so_symbol(&so_mod, "SDL_EnableScreenSaver"), (uintptr_t)&SDL_EnableScreenSaver);
        hook_addr(so_symbol(&so_mod, "SDL_EnclosePoints"), (uintptr_t)&SDL_EnclosePoints);
        hook_addr(so_symbol(&so_mod, "SDL_Error"), (uintptr_t)&SDL_Error);
        //hook_addr(so_symbol(&goo_mod, "sdl_event"), (uintptr_t)&sdl_event);
        hook_addr(so_symbol(&so_mod, "SDL_EventState"), (uintptr_t)&SDL_EventState);
        hook_addr(so_symbol(&so_mod, "SDL_exp"), (uintptr_t)&SDL_exp);
        hook_addr(so_symbol(&so_mod, "SDL_expf"), (uintptr_t)&SDL_expf);
        hook_addr(so_symbol(&so_mod, "SDL_fabs"), (uintptr_t)&SDL_fabs);
        hook_addr(so_symbol(&so_mod, "SDL_fabsf"), (uintptr_t)&SDL_fabsf);
        hook_addr(so_symbol(&so_mod, "SDL_FillRect"), (uintptr_t)&SDL_FillRect);
        hook_addr(so_symbol(&so_mod, "SDL_FillRects"), (uintptr_t)&SDL_FillRects);
        hook_addr(so_symbol(&so_mod, "SDL_FilterEvents"), (uintptr_t)&SDL_FilterEvents);
        hook_addr(so_symbol(&so_mod, "SDL_floor"), (uintptr_t)&SDL_floor);
        hook_addr(so_symbol(&so_mod, "SDL_floorf"), (uintptr_t)&SDL_floorf);
        hook_addr(so_symbol(&so_mod, "SDL_FlushEvent"), (uintptr_t)&SDL_FlushEvent);
        hook_addr(so_symbol(&so_mod, "SDL_FlushEvents"), (uintptr_t)&SDL_FlushEvents);
        hook_addr(so_symbol(&so_mod, "SDL_fmod"), (uintptr_t)&SDL_fmod);
        hook_addr(so_symbol(&so_mod, "SDL_fmodf"), (uintptr_t)&SDL_fmodf);
        hook_addr(so_symbol(&so_mod, "SDL_free"), (uintptr_t)&SDL_free);
        hook_addr(so_symbol(&so_mod, "SDL_FreeAudioStream"), (uintptr_t)&SDL_FreeAudioStream);
        hook_addr(so_symbol(&so_mod, "SDL_FreeCursor"), (uintptr_t)&SDL_FreeCursor);
        hook_addr(so_symbol(&so_mod, "SDL_FreeFormat"), (uintptr_t)&SDL_FreeFormat);
        hook_addr(so_symbol(&so_mod, "SDL_FreePalette"), (uintptr_t)&SDL_FreePalette);
        hook_addr(so_symbol(&so_mod, "SDL_FreeRW"), (uintptr_t)&SDL_FreeRW);
        hook_addr(so_symbol(&so_mod, "SDL_FreeSurface"), (uintptr_t)&SDL_FreeSurface);
        hook_addr(so_symbol(&so_mod, "SDL_FreeWAV"), (uintptr_t)&SDL_FreeWAV);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerAddMapping"), (uintptr_t)&SDL_GameControllerAddMapping);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerAddMappingsFromRW"), (uintptr_t)&SDL_GameControllerAddMappingsFromRW);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerClose"), (uintptr_t)&SDL_GameControllerClose);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerEventState"), (uintptr_t)&SDL_GameControllerEventState);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerFromInstanceID"), (uintptr_t)&SDL_GameControllerFromInstanceID);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerFromPlayerIndex"), (uintptr_t)&SDL_GameControllerFromPlayerIndex);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetAttached"), (uintptr_t)&SDL_GameControllerGetAttached);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetAxis"), (uintptr_t)&SDL_GameControllerGetAxis);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetAxisFromString"), (uintptr_t)&SDL_GameControllerGetAxisFromString);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetBindForAxis"), (uintptr_t)&SDL_GameControllerGetBindForAxis);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetBindForButton"), (uintptr_t)&SDL_GameControllerGetBindForButton);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetButton"), (uintptr_t)&SDL_GameControllerGetButton);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetButtonFromString"), (uintptr_t)&SDL_GameControllerGetButtonFromString);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetJoystick"), (uintptr_t)&SDL_GameControllerGetJoystick);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetNumTouchpadFingers"), (uintptr_t)&SDL_GameControllerGetNumTouchpadFingers);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetNumTouchpads"), (uintptr_t)&SDL_GameControllerGetNumTouchpads);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetPlayerIndex"), (uintptr_t)&SDL_GameControllerGetPlayerIndex);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetProduct"), (uintptr_t)&SDL_GameControllerGetProduct);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetProductVersion"), (uintptr_t)&SDL_GameControllerGetProductVersion);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetSensorData"), (uintptr_t)&SDL_GameControllerGetSensorData);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetSerial"), (uintptr_t)&SDL_GameControllerGetSerial);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetStringForAxis"), (uintptr_t)&SDL_GameControllerGetStringForAxis);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetStringForButton"), (uintptr_t)&SDL_GameControllerGetStringForButton);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetTouchpadFinger"), (uintptr_t)&SDL_GameControllerGetTouchpadFinger);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetType"), (uintptr_t)&SDL_GameControllerGetType);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerGetVendor"), (uintptr_t)&SDL_GameControllerGetVendor);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerHasAxis"), (uintptr_t)&SDL_GameControllerHasAxis);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerHasButton"), (uintptr_t)&SDL_GameControllerHasButton);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerHasLED"), (uintptr_t)&SDL_GameControllerHasLED);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerHasSensor"), (uintptr_t)&SDL_GameControllerHasSensor);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerIsSensorEnabled"), (uintptr_t)&SDL_GameControllerIsSensorEnabled);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerMapping"), (uintptr_t)&SDL_GameControllerMapping);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerMappingForDeviceIndex"), (uintptr_t)&SDL_GameControllerMappingForDeviceIndex);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerMappingForGUID"), (uintptr_t)&SDL_GameControllerMappingForGUID);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerMappingForIndex"), (uintptr_t)&SDL_GameControllerMappingForIndex);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerName"), (uintptr_t)&SDL_GameControllerName);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerNameForIndex"), (uintptr_t)&SDL_GameControllerNameForIndex);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerNumMappings"), (uintptr_t)&SDL_GameControllerNumMappings);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerOpen"), (uintptr_t)&SDL_GameControllerOpen);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerRumble"), (uintptr_t)&SDL_GameControllerRumble);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerRumbleTriggers"), (uintptr_t)&SDL_GameControllerRumbleTriggers);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerSetLED"), (uintptr_t)&SDL_GameControllerSetLED);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerSetPlayerIndex"), (uintptr_t)&SDL_GameControllerSetPlayerIndex);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerSetSensorEnabled"), (uintptr_t)&SDL_GameControllerSetSensorEnabled);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerTypeForIndex"), (uintptr_t)&SDL_GameControllerTypeForIndex);
        hook_addr(so_symbol(&so_mod, "SDL_GameControllerUpdate"), (uintptr_t)&SDL_GameControllerUpdate);
        hook_addr(so_symbol(&so_mod, "SDL_GetAndroidSDKVersion"), (uintptr_t)&SDL_GetAndroidSDKVersion);
        hook_addr(so_symbol(&so_mod, "SDL_GetAssertionHandler"), (uintptr_t)&SDL_GetAssertionHandler);
        hook_addr(so_symbol(&so_mod, "SDL_GetAssertionReport"), (uintptr_t)&SDL_GetAssertionReport);
        hook_addr(so_symbol(&so_mod, "SDL_GetAudioDeviceName"), (uintptr_t)&SDL_GetAudioDeviceName);
        hook_addr(so_symbol(&so_mod, "SDL_GetAudioDeviceSpec"), (uintptr_t)&SDL_GetAudioDeviceSpec);
        hook_addr(so_symbol(&so_mod, "SDL_GetAudioDeviceStatus"), (uintptr_t)&SDL_GetAudioDeviceStatus);
        hook_addr(so_symbol(&so_mod, "SDL_GetAudioDriver"), (uintptr_t)&SDL_GetAudioDriver);
        hook_addr(so_symbol(&so_mod, "SDL_GetAudioStatus"), (uintptr_t)&SDL_GetAudioStatus);
        hook_addr(so_symbol(&so_mod, "SDL_GetBasePath"), (uintptr_t)&SDL_GetBasePath);
        hook_addr(so_symbol(&so_mod, "SDL_GetClipboardText"), (uintptr_t)&SDL_GetClipboardText);
        hook_addr(so_symbol(&so_mod, "SDL_GetClipRect"), (uintptr_t)&SDL_GetClipRect);
        hook_addr(so_symbol(&so_mod, "SDL_GetClosestDisplayMode"), (uintptr_t)&SDL_GetClosestDisplayMode);
        hook_addr(so_symbol(&so_mod, "SDL_GetColorKey"), (uintptr_t)&SDL_GetColorKey);
        hook_addr(so_symbol(&so_mod, "SDL_GetCPUCacheLineSize"), (uintptr_t)&SDL_GetCPUCacheLineSize);
        hook_addr(so_symbol(&so_mod, "SDL_GetCPUCount"), (uintptr_t)&SDL_GetCPUCount);
        hook_addr(so_symbol(&so_mod, "SDL_GetCurrentAudioDriver"), (uintptr_t)&SDL_GetCurrentAudioDriver);
        hook_addr(so_symbol(&so_mod, "SDL_GetCurrentDisplayMode"), (uintptr_t)&SDL_GetCurrentDisplayMode);
        hook_addr(so_symbol(&so_mod, "SDL_GetCurrentVideoDriver"), (uintptr_t)&SDL_GetCurrentVideoDriver);
        hook_addr(so_symbol(&so_mod, "SDL_GetCursor"), (uintptr_t)&SDL_GetCursor);
        hook_addr(so_symbol(&so_mod, "SDL_GetDefaultAssertionHandler"), (uintptr_t)&SDL_GetDefaultAssertionHandler);
        hook_addr(so_symbol(&so_mod, "SDL_GetDefaultCursor"), (uintptr_t)&SDL_GetDefaultCursor);
        hook_addr(so_symbol(&so_mod, "SDL_GetDesktopDisplayMode"), (uintptr_t)&SDL_GetDesktopDisplayMode);
        hook_addr(so_symbol(&so_mod, "SDL_GetDisplayBounds"), (uintptr_t)&SDL_GetDisplayBounds);
        hook_addr(so_symbol(&so_mod, "SDL_GetDisplayDPI"), (uintptr_t)&SDL_GetDisplayDPI);
        hook_addr(so_symbol(&so_mod, "SDL_GetDisplayMode"), (uintptr_t)&SDL_GetDisplayMode);
        hook_addr(so_symbol(&so_mod, "SDL_GetDisplayName"), (uintptr_t)&SDL_GetDisplayName);
        hook_addr(so_symbol(&so_mod, "SDL_GetDisplayOrientation"), (uintptr_t)&SDL_GetDisplayOrientation);
        hook_addr(so_symbol(&so_mod, "SDL_GetDisplayUsableBounds"), (uintptr_t)&SDL_GetDisplayUsableBounds);
        hook_addr(so_symbol(&so_mod, "SDL_getenv"), (uintptr_t)&SDL_getenv);
        hook_addr(so_symbol(&so_mod, "SDL_GetError"), (uintptr_t)&SDL_GetError);
        hook_addr(so_symbol(&so_mod, "SDL_GetErrorMsg"), (uintptr_t)&SDL_GetErrorMsg);
        hook_addr(so_symbol(&so_mod, "SDL_GetEventFilter"), (uintptr_t)&SDL_GetEventFilter);
        hook_addr(so_symbol(&so_mod, "SDL_GetGlobalMouseState"), (uintptr_t)&SDL_GetGlobalMouseState);
        hook_addr(so_symbol(&so_mod, "SDL_GetGrabbedWindow"), (uintptr_t)&SDL_GetGrabbedWindow);
        hook_addr(so_symbol(&so_mod, "SDL_GetHint"), (uintptr_t)&SDL_GetHint);
        hook_addr(so_symbol(&so_mod, "SDL_GetHintBoolean"), (uintptr_t)&SDL_GetHintBoolean);
        hook_addr(so_symbol(&so_mod, "SDL_GetKeyboardFocus"), (uintptr_t)&SDL_GetKeyboardFocus);
        hook_addr(so_symbol(&so_mod, "SDL_GetKeyboardState"), (uintptr_t)&SDL_GetKeyboardState);
        hook_addr(so_symbol(&so_mod, "SDL_GetKeyFromName"), (uintptr_t)&SDL_GetKeyFromName);
        hook_addr(so_symbol(&so_mod, "SDL_GetKeyFromScancode"), (uintptr_t)&SDL_GetKeyFromScancode);
        hook_addr(so_symbol(&so_mod, "SDL_GetKeyName"), (uintptr_t)&SDL_GetKeyName);
        hook_addr(so_symbol(&so_mod, "SDL_GetMemoryFunctions"), (uintptr_t)&SDL_GetMemoryFunctions);
        hook_addr(so_symbol(&so_mod, "SDL_GetModState"), (uintptr_t)&SDL_GetModState);
        hook_addr(so_symbol(&so_mod, "SDL_GetMouseFocus"), (uintptr_t)&SDL_GetMouseFocus);
        hook_addr(so_symbol(&so_mod, "SDL_GetMouseState"), (uintptr_t)&SDL_GetMouseState);
        hook_addr(so_symbol(&so_mod, "SDL_GetNumAllocations"), (uintptr_t)&SDL_GetNumAllocations);
        hook_addr(so_symbol(&so_mod, "SDL_GetNumAudioDevices"), (uintptr_t)&SDL_GetNumAudioDevices);
        hook_addr(so_symbol(&so_mod, "SDL_GetNumAudioDrivers"), (uintptr_t)&SDL_GetNumAudioDrivers);
        hook_addr(so_symbol(&so_mod, "SDL_GetNumDisplayModes"), (uintptr_t)&SDL_GetNumDisplayModes);
        hook_addr(so_symbol(&so_mod, "SDL_GetNumRenderDrivers"), (uintptr_t)&SDL_GetNumRenderDrivers);
        hook_addr(so_symbol(&so_mod, "SDL_GetNumTouchDevices"), (uintptr_t)&SDL_GetNumTouchDevices);
        hook_addr(so_symbol(&so_mod, "SDL_GetNumTouchFingers"), (uintptr_t)&SDL_GetNumTouchFingers);
        hook_addr(so_symbol(&so_mod, "SDL_GetNumVideoDisplays"), (uintptr_t)&SDL_GetNumVideoDisplays);
        hook_addr(so_symbol(&so_mod, "SDL_GetNumVideoDrivers"), (uintptr_t)&SDL_GetNumVideoDrivers);
        hook_addr(so_symbol(&so_mod, "SDL_GetPerformanceCounter"), (uintptr_t)&SDL_GetPerformanceCounter);
        hook_addr(so_symbol(&so_mod, "SDL_GetPerformanceFrequency"), (uintptr_t)&SDL_GetPerformanceFrequency);
        hook_addr(so_symbol(&so_mod, "SDL_GetPixelFormatName"), (uintptr_t)&SDL_GetPixelFormatName);
        hook_addr(so_symbol(&so_mod, "SDL_GetPlatform"), (uintptr_t)&SDL_GetPlatform);
        hook_addr(so_symbol(&so_mod, "SDL_GetPowerInfo"), (uintptr_t)&SDL_GetPowerInfo);
        hook_addr(so_symbol(&so_mod, "SDL_GetPreferredLocales"), (uintptr_t)&SDL_GetPreferredLocales);
        hook_addr(so_symbol(&so_mod, "SDL_GetPrefPath"), (uintptr_t)&SDL_GetPrefPath);
        hook_addr(so_symbol(&so_mod, "SDL_GetQueuedAudioSize"), (uintptr_t)&SDL_GetQueuedAudioSize);
        hook_addr(so_symbol(&so_mod, "SDL_GetRelativeMouseMode"), (uintptr_t)&SDL_GetRelativeMouseMode);
        hook_addr(so_symbol(&so_mod, "SDL_GetRelativeMouseState"), (uintptr_t)&SDL_GetRelativeMouseState);
        hook_addr(so_symbol(&so_mod, "SDL_GetRenderDrawBlendMode"), (uintptr_t)&SDL_GetRenderDrawBlendMode);
        hook_addr(so_symbol(&so_mod, "SDL_GetRenderDrawColor"), (uintptr_t)&SDL_GetRenderDrawColor);
        hook_addr(so_symbol(&so_mod, "SDL_GetRenderDriverInfo"), (uintptr_t)&SDL_GetRenderDriverInfo);
        hook_addr(so_symbol(&so_mod, "SDL_GetRenderer"), (uintptr_t)&SDL_GetRenderer);
        hook_addr(so_symbol(&so_mod, "SDL_GetRendererInfo"), (uintptr_t)&SDL_GetRendererInfo);
        hook_addr(so_symbol(&so_mod, "SDL_GetRendererOutputSize"), (uintptr_t)&SDL_GetRendererOutputSize);
        hook_addr(so_symbol(&so_mod, "SDL_GetRenderTarget"), (uintptr_t)&SDL_GetRenderTarget);
        hook_addr(so_symbol(&so_mod, "SDL_GetRevision"), (uintptr_t)&SDL_GetRevision);
        hook_addr(so_symbol(&so_mod, "SDL_GetRGB"), (uintptr_t)&SDL_GetRGB);
        hook_addr(so_symbol(&so_mod, "SDL_GetRGBA"), (uintptr_t)&SDL_GetRGBA);
        hook_addr(so_symbol(&so_mod, "SDL_GetScancodeFromKey"), (uintptr_t)&SDL_GetScancodeFromKey);
        hook_addr(so_symbol(&so_mod, "SDL_GetScancodeFromName"), (uintptr_t)&SDL_GetScancodeFromName);
        hook_addr(so_symbol(&so_mod, "SDL_GetScancodeName"), (uintptr_t)&SDL_GetScancodeName);
        hook_addr(so_symbol(&so_mod, "SDL_GetShapedWindowMode"), (uintptr_t)&SDL_GetShapedWindowMode);
        hook_addr(so_symbol(&so_mod, "SDL_GetSurfaceAlphaMod"), (uintptr_t)&SDL_GetSurfaceAlphaMod);
        hook_addr(so_symbol(&so_mod, "SDL_GetSurfaceBlendMode"), (uintptr_t)&SDL_GetSurfaceBlendMode);
        hook_addr(so_symbol(&so_mod, "SDL_GetSurfaceColorMod"), (uintptr_t)&SDL_GetSurfaceColorMod);
        hook_addr(so_symbol(&so_mod, "SDL_GetSystemRAM"), (uintptr_t)&SDL_GetSystemRAM);
        hook_addr(so_symbol(&so_mod, "SDL_GetTextureAlphaMod"), (uintptr_t)&SDL_GetTextureAlphaMod);
        hook_addr(so_symbol(&so_mod, "SDL_GetTextureBlendMode"), (uintptr_t)&SDL_GetTextureBlendMode);
        hook_addr(so_symbol(&so_mod, "SDL_GetTextureColorMod"), (uintptr_t)&SDL_GetTextureColorMod);
        hook_addr(so_symbol(&so_mod, "SDL_GetTextureScaleMode"), (uintptr_t)&SDL_GetTextureScaleMode);
        hook_addr(so_symbol(&so_mod, "SDL_GetThreadID"), (uintptr_t)&SDL_GetThreadID);
        hook_addr(so_symbol(&so_mod, "SDL_GetThreadName"), (uintptr_t)&SDL_GetThreadName);
        hook_addr(so_symbol(&so_mod, "SDL_GetTicks"), (uintptr_t)&SDL_GetTicks);
        hook_addr(so_symbol(&so_mod, "SDL_GetTouchDevice"), (uintptr_t)&SDL_GetTouchDevice);
        hook_addr(so_symbol(&so_mod, "SDL_GetTouchDeviceType"), (uintptr_t)&SDL_GetTouchDeviceType);
        hook_addr(so_symbol(&so_mod, "SDL_GetTouchFinger"), (uintptr_t)&SDL_GetTouchFinger);
        hook_addr(so_symbol(&so_mod, "SDL_GetVersion"), (uintptr_t)&SDL_GetVersion);
        hook_addr(so_symbol(&so_mod, "SDL_GetVideoDriver"), (uintptr_t)&SDL_GetVideoDriver);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowBordersSize"), (uintptr_t)&SDL_GetWindowBordersSize);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowBrightness"), (uintptr_t)&SDL_GetWindowBrightness);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowData"), (uintptr_t)&SDL_GetWindowData);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowDisplayIndex"), (uintptr_t)&SDL_GetWindowDisplayIndex);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowDisplayMode"), (uintptr_t)&SDL_GetWindowDisplayMode);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowFlags"), (uintptr_t)&SDL_GetWindowFlags);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowFromID"), (uintptr_t)&SDL_GetWindowFromID);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowGammaRamp"), (uintptr_t)&SDL_GetWindowGammaRamp);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowGrab"), (uintptr_t)&SDL_GetWindowGrab);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowID"), (uintptr_t)&SDL_GetWindowID);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowKeyboardGrab"), (uintptr_t)&SDL_GetWindowKeyboardGrab);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowMaximumSize"), (uintptr_t)&SDL_GetWindowMaximumSize);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowMinimumSize"), (uintptr_t)&SDL_GetWindowMinimumSize);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowMouseGrab"), (uintptr_t)&SDL_GetWindowMouseGrab);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowOpacity"), (uintptr_t)&SDL_GetWindowOpacity);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowPixelFormat"), (uintptr_t)&SDL_GetWindowPixelFormat);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowPosition"), (uintptr_t)&SDL_GetWindowPosition);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowSize"), (uintptr_t)&SDL_GetWindowSize);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowSurface"), (uintptr_t)&SDL_GetWindowSurface);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowTitle"), (uintptr_t)&SDL_GetWindowTitle);
        hook_addr(so_symbol(&so_mod, "SDL_GetWindowWMInfo"), (uintptr_t)&SDL_GetWindowWMInfo);
        hook_addr(so_symbol(&so_mod, "SDL_GetYUVConversionMode"), (uintptr_t)&SDL_GetYUVConversionMode);
        hook_addr(so_symbol(&so_mod, "SDL_GetYUVConversionModeForResolution"), (uintptr_t)&SDL_GetYUVConversionModeForResolution);
        hook_addr(so_symbol(&so_mod, "SDL_GL_BindTexture"), (uintptr_t)&SDL_GL_BindTexture);
        hook_addr(so_symbol(&so_mod, "SDL_GL_CreateContext"), (uintptr_t)&SDL_GL_CreateContext);
        hook_addr(so_symbol(&so_mod, "SDL_GL_DeleteContext"), (uintptr_t)&SDL_GL_DeleteContext);
        hook_addr(so_symbol(&so_mod, "SDL_GL_ExtensionSupported"), (uintptr_t)&SDL_GL_ExtensionSupported);
        hook_addr(so_symbol(&so_mod, "SDL_GL_GetAttribute"), (uintptr_t)&SDL_GL_GetAttribute);
        hook_addr(so_symbol(&so_mod, "SDL_GL_GetCurrentContext"), (uintptr_t)&SDL_GL_GetCurrentContext);
        hook_addr(so_symbol(&so_mod, "SDL_GL_GetCurrentWindow"), (uintptr_t)&SDL_GL_GetCurrentWindow);
        hook_addr(so_symbol(&so_mod, "SDL_GL_GetDrawableSize"), (uintptr_t)&SDL_GL_GetDrawableSize);
        hook_addr(so_symbol(&so_mod, "SDL_GL_GetProcAddress"), (uintptr_t)&SDL_GL_GetProcAddress);
        hook_addr(so_symbol(&so_mod, "SDL_GL_GetSwapInterval"), (uintptr_t)&SDL_GL_GetSwapInterval);
        hook_addr(so_symbol(&so_mod, "SDL_GL_LoadLibrary"), (uintptr_t)&SDL_GL_LoadLibrary);
        hook_addr(so_symbol(&so_mod, "SDL_GL_MakeCurrent"), (uintptr_t)&SDL_GL_MakeCurrent);
        hook_addr(so_symbol(&so_mod, "SDL_GL_ResetAttributes"), (uintptr_t)&SDL_GL_ResetAttributes);
        hook_addr(so_symbol(&so_mod, "SDL_GL_SetAttribute"), (uintptr_t)&SDL_GL_SetAttribute);
        hook_addr(so_symbol(&so_mod, "SDL_GL_SetSwapInterval"), (uintptr_t)&SDL_GL_SetSwapInterval);
        hook_addr(so_symbol(&so_mod, "SDL_GL_SwapWindow"), (uintptr_t)&SDL_GL_SwapWindow);
        hook_addr(so_symbol(&so_mod, "SDL_GL_UnbindTexture"), (uintptr_t)&SDL_GL_UnbindTexture);
        hook_addr(so_symbol(&so_mod, "SDL_GL_UnloadLibrary"), (uintptr_t)&SDL_GL_UnloadLibrary);
        hook_addr(so_symbol(&so_mod, "SDL_HapticClose"), (uintptr_t)&SDL_HapticClose);
        hook_addr(so_symbol(&so_mod, "SDL_HapticDestroyEffect"), (uintptr_t)&SDL_HapticDestroyEffect);
        hook_addr(so_symbol(&so_mod, "SDL_HapticEffectSupported"), (uintptr_t)&SDL_HapticEffectSupported);
        hook_addr(so_symbol(&so_mod, "SDL_HapticGetEffectStatus"), (uintptr_t)&SDL_HapticGetEffectStatus);
        hook_addr(so_symbol(&so_mod, "SDL_HapticIndex"), (uintptr_t)&SDL_HapticIndex);
        hook_addr(so_symbol(&so_mod, "SDL_HapticName"), (uintptr_t)&SDL_HapticName);
        hook_addr(so_symbol(&so_mod, "SDL_HapticNewEffect"), (uintptr_t)&SDL_HapticNewEffect);
        hook_addr(so_symbol(&so_mod, "SDL_HapticNumAxes"), (uintptr_t)&SDL_HapticNumAxes);
        hook_addr(so_symbol(&so_mod, "SDL_HapticNumEffects"), (uintptr_t)&SDL_HapticNumEffects);
        hook_addr(so_symbol(&so_mod, "SDL_HapticNumEffectsPlaying"), (uintptr_t)&SDL_HapticNumEffectsPlaying);
        hook_addr(so_symbol(&so_mod, "SDL_HapticOpen"), (uintptr_t)&SDL_HapticOpen);
        hook_addr(so_symbol(&so_mod, "SDL_HapticOpened"), (uintptr_t)&SDL_HapticOpened);
        hook_addr(so_symbol(&so_mod, "SDL_HapticOpenFromJoystick"), (uintptr_t)&SDL_HapticOpenFromJoystick);
        hook_addr(so_symbol(&so_mod, "SDL_HapticOpenFromMouse"), (uintptr_t)&SDL_HapticOpenFromMouse);
        hook_addr(so_symbol(&so_mod, "SDL_HapticPause"), (uintptr_t)&SDL_HapticPause);
        hook_addr(so_symbol(&so_mod, "SDL_HapticQuery"), (uintptr_t)&SDL_HapticQuery);
        hook_addr(so_symbol(&so_mod, "SDL_HapticRumbleInit"), (uintptr_t)&SDL_HapticRumbleInit);
        hook_addr(so_symbol(&so_mod, "SDL_HapticRumblePlay"), (uintptr_t)&SDL_HapticRumblePlay);
        hook_addr(so_symbol(&so_mod, "SDL_HapticRumbleStop"), (uintptr_t)&SDL_HapticRumbleStop);
        hook_addr(so_symbol(&so_mod, "SDL_HapticRumbleSupported"), (uintptr_t)&SDL_HapticRumbleSupported);
        hook_addr(so_symbol(&so_mod, "SDL_HapticRunEffect"), (uintptr_t)&SDL_HapticRunEffect);
        hook_addr(so_symbol(&so_mod, "SDL_HapticSetAutocenter"), (uintptr_t)&SDL_HapticSetAutocenter);
        hook_addr(so_symbol(&so_mod, "SDL_HapticSetGain"), (uintptr_t)&SDL_HapticSetGain);
        hook_addr(so_symbol(&so_mod, "SDL_HapticStopAll"), (uintptr_t)&SDL_HapticStopAll);
        hook_addr(so_symbol(&so_mod, "SDL_HapticStopEffect"), (uintptr_t)&SDL_HapticStopEffect);
        hook_addr(so_symbol(&so_mod, "SDL_HapticUnpause"), (uintptr_t)&SDL_HapticUnpause);
        hook_addr(so_symbol(&so_mod, "SDL_HapticUpdateEffect"), (uintptr_t)&SDL_HapticUpdateEffect);
        hook_addr(so_symbol(&so_mod, "SDL_Has3DNow"), (uintptr_t)&SDL_Has3DNow);
        hook_addr(so_symbol(&so_mod, "SDL_HasAltiVec"), (uintptr_t)&SDL_HasAltiVec);
        hook_addr(so_symbol(&so_mod, "SDL_HasARMSIMD"), (uintptr_t)&SDL_HasARMSIMD);
        hook_addr(so_symbol(&so_mod, "SDL_HasAVX"), (uintptr_t)&SDL_HasAVX);
        hook_addr(so_symbol(&so_mod, "SDL_HasAVX2"), (uintptr_t)&SDL_HasAVX2);
        hook_addr(so_symbol(&so_mod, "SDL_HasAVX512F"), (uintptr_t)&SDL_HasAVX512F);
        hook_addr(so_symbol(&so_mod, "SDL_HasClipboardText"), (uintptr_t)&SDL_HasClipboardText);
        hook_addr(so_symbol(&so_mod, "SDL_HasColorKey"), (uintptr_t)&SDL_HasColorKey);
        hook_addr(so_symbol(&so_mod, "SDL_HasEvent"), (uintptr_t)&SDL_HasEvent);
        hook_addr(so_symbol(&so_mod, "SDL_HasEvents"), (uintptr_t)&SDL_HasEvents);
        hook_addr(so_symbol(&so_mod, "SDL_HasIntersection"), (uintptr_t)&SDL_HasIntersection);
        hook_addr(so_symbol(&so_mod, "SDL_HasMMX"), (uintptr_t)&SDL_HasMMX);
        hook_addr(so_symbol(&so_mod, "SDL_HasNEON"), (uintptr_t)&SDL_HasNEON);
        hook_addr(so_symbol(&so_mod, "SDL_HasRDTSC"), (uintptr_t)&SDL_HasRDTSC);
        hook_addr(so_symbol(&so_mod, "SDL_HasScreenKeyboardSupport"), (uintptr_t)&SDL_HasScreenKeyboardSupport);
        hook_addr(so_symbol(&so_mod, "SDL_HasSSE"), (uintptr_t)&SDL_HasSSE);
        hook_addr(so_symbol(&so_mod, "SDL_HasSSE2"), (uintptr_t)&SDL_HasSSE2);
        hook_addr(so_symbol(&so_mod, "SDL_HasSSE3"), (uintptr_t)&SDL_HasSSE3);
        hook_addr(so_symbol(&so_mod, "SDL_HasSSE41"), (uintptr_t)&SDL_HasSSE41);
        hook_addr(so_symbol(&so_mod, "SDL_HasSSE42"), (uintptr_t)&SDL_HasSSE42);
        hook_addr(so_symbol(&so_mod, "SDL_HasSurfaceRLE"), (uintptr_t)&SDL_HasSurfaceRLE);
        hook_addr(so_symbol(&so_mod, "SDL_HideWindow"), (uintptr_t)&SDL_HideWindow);
        hook_addr(so_symbol(&so_mod, "SDL_iconv"), (uintptr_t)&SDL_iconv);
        hook_addr(so_symbol(&so_mod, "SDL_iconv_close"), (uintptr_t)&SDL_iconv_close);
        hook_addr(so_symbol(&so_mod, "SDL_iconv_open"), (uintptr_t)&SDL_iconv_open);
        hook_addr(so_symbol(&so_mod, "SDL_iconv_string"), (uintptr_t)&SDL_iconv_string);
        hook_addr(so_symbol(&so_mod, "SDL_Init"), (uintptr_t)&SDL_Init_fake);
        hook_addr(so_symbol(&so_mod, "SDL_InitSubSystem"), (uintptr_t)&SDL_InitSubSystem);
        hook_addr(so_symbol(&so_mod, "SDL_IntersectRect"), (uintptr_t)&SDL_IntersectRect);
        hook_addr(so_symbol(&so_mod, "SDL_IntersectRectAndLine"), (uintptr_t)&SDL_IntersectRectAndLine);
        hook_addr(so_symbol(&so_mod, "SDL_isalnum"), (uintptr_t)&SDL_isalnum);
        hook_addr(so_symbol(&so_mod, "SDL_isalpha"), (uintptr_t)&SDL_isalpha);
        hook_addr(so_symbol(&so_mod, "SDL_IsAndroidTV"), (uintptr_t)&SDL_IsAndroidTV);
        hook_addr(so_symbol(&so_mod, "SDL_isblank"), (uintptr_t)&SDL_isblank);
        hook_addr(so_symbol(&so_mod, "SDL_IsChromebook"), (uintptr_t)&SDL_IsChromebook);
        hook_addr(so_symbol(&so_mod, "SDL_iscntrl"), (uintptr_t)&SDL_iscntrl);
        hook_addr(so_symbol(&so_mod, "SDL_IsDeXMode"), (uintptr_t)&SDL_IsDeXMode);
        hook_addr(so_symbol(&so_mod, "SDL_isdigit"), (uintptr_t)&SDL_isdigit);
        hook_addr(so_symbol(&so_mod, "SDL_IsGameController"), (uintptr_t)&SDL_IsGameController);
        hook_addr(so_symbol(&so_mod, "SDL_isgraph"), (uintptr_t)&SDL_isgraph);
        hook_addr(so_symbol(&so_mod, "SDL_islower"), (uintptr_t)&SDL_islower);
        hook_addr(so_symbol(&so_mod, "SDL_isprint"), (uintptr_t)&SDL_isprint);
        hook_addr(so_symbol(&so_mod, "SDL_ispunct"), (uintptr_t)&SDL_ispunct);
        hook_addr(so_symbol(&so_mod, "SDL_IsScreenKeyboardShown"), (uintptr_t)&SDL_IsScreenKeyboardShown);
        hook_addr(so_symbol(&so_mod, "SDL_IsScreenSaverEnabled"), (uintptr_t)&SDL_IsScreenSaverEnabled);
        hook_addr(so_symbol(&so_mod, "SDL_IsShapedWindow"), (uintptr_t)&SDL_IsShapedWindow);
        hook_addr(so_symbol(&so_mod, "SDL_isspace"), (uintptr_t)&SDL_isspace);
        hook_addr(so_symbol(&so_mod, "SDL_IsTablet"), (uintptr_t)&SDL_IsTablet);
        hook_addr(so_symbol(&so_mod, "SDL_IsTextInputActive"), (uintptr_t)&SDL_IsTextInputActive);
        hook_addr(so_symbol(&so_mod, "SDL_isupper"), (uintptr_t)&SDL_isupper);
        hook_addr(so_symbol(&so_mod, "SDL_isxdigit"), (uintptr_t)&SDL_isxdigit);
        hook_addr(so_symbol(&so_mod, "SDL_itoa"), (uintptr_t)&SDL_itoa);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickAttachVirtual"), (uintptr_t)&SDL_JoystickAttachVirtual);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickClose"), (uintptr_t)&SDL_JoystickClose);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickCurrentPowerLevel"), (uintptr_t)&SDL_JoystickCurrentPowerLevel);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickDetachVirtual"), (uintptr_t)&SDL_JoystickDetachVirtual);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickEventState"), (uintptr_t)&SDL_JoystickEventState);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickFromInstanceID"), (uintptr_t)&SDL_JoystickFromInstanceID);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickFromPlayerIndex"), (uintptr_t)&SDL_JoystickFromPlayerIndex);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetAttached"), (uintptr_t)&SDL_JoystickGetAttached);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetAxis"), (uintptr_t)&SDL_JoystickGetAxis);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetAxisInitialState"), (uintptr_t)&SDL_JoystickGetAxisInitialState);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetBall"), (uintptr_t)&SDL_JoystickGetBall);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetButton"), (uintptr_t)&SDL_JoystickGetButton);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetDeviceGUID"), (uintptr_t)&SDL_JoystickGetDeviceGUID);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetDeviceInstanceID"), (uintptr_t)&SDL_JoystickGetDeviceInstanceID);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetDevicePlayerIndex"), (uintptr_t)&SDL_JoystickGetDevicePlayerIndex);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetDeviceProduct"), (uintptr_t)&SDL_JoystickGetDeviceProduct);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetDeviceProductVersion"), (uintptr_t)&SDL_JoystickGetDeviceProductVersion);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetDeviceType"), (uintptr_t)&SDL_JoystickGetDeviceType);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetDeviceVendor"), (uintptr_t)&SDL_JoystickGetDeviceVendor);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetGUID"), (uintptr_t)&SDL_JoystickGetGUID);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetGUIDFromString"), (uintptr_t)&SDL_JoystickGetGUIDFromString);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetGUIDString"), (uintptr_t)&SDL_JoystickGetGUIDString);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetHat"), (uintptr_t)&SDL_JoystickGetHat);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetPlayerIndex"), (uintptr_t)&SDL_JoystickGetPlayerIndex);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetProduct"), (uintptr_t)&SDL_JoystickGetProduct);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetProductVersion"), (uintptr_t)&SDL_JoystickGetProductVersion);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetSerial"), (uintptr_t)&SDL_JoystickGetSerial);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetType"), (uintptr_t)&SDL_JoystickGetType);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickGetVendor"), (uintptr_t)&SDL_JoystickGetVendor);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickHasLED"), (uintptr_t)&SDL_JoystickHasLED);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickInstanceID"), (uintptr_t)&SDL_JoystickInstanceID);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickIsHaptic"), (uintptr_t)&SDL_JoystickIsHaptic);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickIsVirtual"), (uintptr_t)&SDL_JoystickIsVirtual);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickName"), (uintptr_t)&SDL_JoystickName);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickNameForIndex"), (uintptr_t)&SDL_JoystickNameForIndex);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickNumAxes"), (uintptr_t)&SDL_JoystickNumAxes);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickNumBalls"), (uintptr_t)&SDL_JoystickNumBalls);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickNumButtons"), (uintptr_t)&SDL_JoystickNumButtons);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickNumHats"), (uintptr_t)&SDL_JoystickNumHats);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickOpen"), (uintptr_t)&SDL_JoystickOpen);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickRumble"), (uintptr_t)&SDL_JoystickRumble);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickRumbleTriggers"), (uintptr_t)&SDL_JoystickRumbleTriggers);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickSetLED"), (uintptr_t)&SDL_JoystickSetLED);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickSetPlayerIndex"), (uintptr_t)&SDL_JoystickSetPlayerIndex);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickSetVirtualAxis"), (uintptr_t)&SDL_JoystickSetVirtualAxis);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickSetVirtualButton"), (uintptr_t)&SDL_JoystickSetVirtualButton);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickSetVirtualHat"), (uintptr_t)&SDL_JoystickSetVirtualHat);
        hook_addr(so_symbol(&so_mod, "SDL_JoystickUpdate"), (uintptr_t)&SDL_JoystickUpdate);
        hook_addr(so_symbol(&so_mod, "SDL_lltoa"), (uintptr_t)&SDL_lltoa);
        hook_addr(so_symbol(&so_mod, "SDL_LoadBMP_RW"), (uintptr_t)&SDL_LoadBMP_RW);
        hook_addr(so_symbol(&so_mod, "SDL_LoadDollarTemplates"), (uintptr_t)&SDL_LoadDollarTemplates);
        hook_addr(so_symbol(&so_mod, "SDL_LoadFile"), (uintptr_t)&SDL_LoadFile);
        hook_addr(so_symbol(&so_mod, "SDL_LoadFile_RW"), (uintptr_t)&SDL_LoadFile_RW);
        hook_addr(so_symbol(&so_mod, "SDL_LoadFunction"), (uintptr_t)&SDL_LoadFunction);
        hook_addr(so_symbol(&so_mod, "SDL_LoadObject"), (uintptr_t)&SDL_LoadObject);
        hook_addr(so_symbol(&so_mod, "SDL_LoadWAV_RW"), (uintptr_t)&SDL_LoadWAV_RW);
        hook_addr(so_symbol(&so_mod, "SDL_LockAudio"), (uintptr_t)&SDL_LockAudio);
        hook_addr(so_symbol(&so_mod, "SDL_LockAudioDevice"), (uintptr_t)&SDL_LockAudioDevice);
        hook_addr(so_symbol(&so_mod, "SDL_LockJoysticks"), (uintptr_t)&SDL_LockJoysticks);
        hook_addr(so_symbol(&so_mod, "SDL_LockMutex"), (uintptr_t)&SDL_LockMutex);
        hook_addr(so_symbol(&so_mod, "SDL_LockSensors"), (uintptr_t)&SDL_LockSensors);
        hook_addr(so_symbol(&so_mod, "SDL_LockSurface"), (uintptr_t)&SDL_LockSurface);
        hook_addr(so_symbol(&so_mod, "SDL_LockTexture"), (uintptr_t)&SDL_LockTexture);
        hook_addr(so_symbol(&so_mod, "SDL_LockTextureToSurface"), (uintptr_t)&SDL_LockTextureToSurface);
        hook_addr(so_symbol(&so_mod, "SDL_log"), (uintptr_t)&SDL_log);
        hook_addr(so_symbol(&so_mod, "SDL_Log"), (uintptr_t)&SDL_Log);
        hook_addr(so_symbol(&so_mod, "SDL_log10"), (uintptr_t)&SDL_log10);
        hook_addr(so_symbol(&so_mod, "SDL_log10f"), (uintptr_t)&SDL_log10f);
        hook_addr(so_symbol(&so_mod, "SDL_LogCritical"), (uintptr_t)&SDL_LogCritical);
        hook_addr(so_symbol(&so_mod, "SDL_LogDebug"), (uintptr_t)&SDL_LogDebug);
        hook_addr(so_symbol(&so_mod, "SDL_LogError"), (uintptr_t)&SDL_LogError);
        hook_addr(so_symbol(&so_mod, "SDL_logf"), (uintptr_t)&SDL_logf);
        hook_addr(so_symbol(&so_mod, "SDL_LogGetOutputFunction"), (uintptr_t)&SDL_LogGetOutputFunction);
        hook_addr(so_symbol(&so_mod, "SDL_LogGetPriority"), (uintptr_t)&SDL_LogGetPriority);
        hook_addr(so_symbol(&so_mod, "SDL_LogInfo"), (uintptr_t)&SDL_LogInfo);
        hook_addr(so_symbol(&so_mod, "SDL_LogMessage"), (uintptr_t)&SDL_LogMessage);
        hook_addr(so_symbol(&so_mod, "SDL_LogMessageV"), (uintptr_t)&SDL_LogMessageV);
        hook_addr(so_symbol(&so_mod, "SDL_LogResetPriorities"), (uintptr_t)&SDL_LogResetPriorities);
        hook_addr(so_symbol(&so_mod, "SDL_LogSetAllPriority"), (uintptr_t)&SDL_LogSetAllPriority);
        hook_addr(so_symbol(&so_mod, "SDL_LogSetOutputFunction"), (uintptr_t)&SDL_LogSetOutputFunction);
        hook_addr(so_symbol(&so_mod, "SDL_LogSetPriority"), (uintptr_t)&SDL_LogSetPriority);
        hook_addr(so_symbol(&so_mod, "SDL_LogVerbose"), (uintptr_t)&SDL_LogVerbose);
        hook_addr(so_symbol(&so_mod, "SDL_LogWarn"), (uintptr_t)&SDL_LogWarn);
        hook_addr(so_symbol(&so_mod, "SDL_LowerBlit"), (uintptr_t)&SDL_LowerBlit);
        hook_addr(so_symbol(&so_mod, "SDL_LowerBlitScaled"), (uintptr_t)&SDL_LowerBlitScaled);
        hook_addr(so_symbol(&so_mod, "SDL_lround"), (uintptr_t)&SDL_lround);
        hook_addr(so_symbol(&so_mod, "SDL_lroundf"), (uintptr_t)&SDL_lroundf);
        hook_addr(so_symbol(&so_mod, "SDL_ltoa"), (uintptr_t)&SDL_ltoa);
        hook_addr(so_symbol(&so_mod, "SDL_malloc"), (uintptr_t)&SDL_malloc);
        hook_addr(so_symbol(&so_mod, "SDL_MapRGB"), (uintptr_t)&SDL_MapRGB);
        hook_addr(so_symbol(&so_mod, "SDL_MapRGBA"), (uintptr_t)&SDL_MapRGBA);
        hook_addr(so_symbol(&so_mod, "SDL_MasksToPixelFormatEnum"), (uintptr_t)&SDL_MasksToPixelFormatEnum);
        hook_addr(so_symbol(&so_mod, "SDL_MaximizeWindow"), (uintptr_t)&SDL_MaximizeWindow);
        hook_addr(so_symbol(&so_mod, "SDL_memcmp"), (uintptr_t)&SDL_memcmp);
        hook_addr(so_symbol(&so_mod, "SDL_memcpy"), (uintptr_t)&SDL_memcpy);
        hook_addr(so_symbol(&so_mod, "SDL_memmove"), (uintptr_t)&SDL_memmove);
        hook_addr(so_symbol(&so_mod, "SDL_MemoryBarrierAcquireFunction"), (uintptr_t)&SDL_MemoryBarrierAcquireFunction);
        hook_addr(so_symbol(&so_mod, "SDL_MemoryBarrierReleaseFunction"), (uintptr_t)&SDL_MemoryBarrierReleaseFunction);
        hook_addr(so_symbol(&so_mod, "SDL_memset"), (uintptr_t)&SDL_memset);
        hook_addr(so_symbol(&so_mod, "SDL_Metal_CreateView"), (uintptr_t)&SDL_Metal_CreateView);
        hook_addr(so_symbol(&so_mod, "SDL_Metal_DestroyView"), (uintptr_t)&SDL_Metal_DestroyView);
        hook_addr(so_symbol(&so_mod, "SDL_Metal_GetDrawableSize"), (uintptr_t)&SDL_Metal_GetDrawableSize);
        hook_addr(so_symbol(&so_mod, "SDL_Metal_GetLayer"), (uintptr_t)&SDL_Metal_GetLayer);
        hook_addr(so_symbol(&so_mod, "SDL_MinimizeWindow"), (uintptr_t)&SDL_MinimizeWindow);
        hook_addr(so_symbol(&so_mod, "SDL_MixAudio"), (uintptr_t)&SDL_MixAudio);
        hook_addr(so_symbol(&so_mod, "SDL_MixAudioFormat"), (uintptr_t)&SDL_MixAudioFormat);
        hook_addr(so_symbol(&so_mod, "SDL_MouseIsHaptic"), (uintptr_t)&SDL_MouseIsHaptic);
        hook_addr(so_symbol(&so_mod, "SDL_NewAudioStream"), (uintptr_t)&SDL_NewAudioStream);
        hook_addr(so_symbol(&so_mod, "SDL_NumHaptics"), (uintptr_t)&SDL_NumHaptics);
        hook_addr(so_symbol(&so_mod, "SDL_NumJoysticks"), (uintptr_t)&SDL_NumJoysticks);
        hook_addr(so_symbol(&so_mod, "SDL_NumSensors"), (uintptr_t)&SDL_NumSensors);
        hook_addr(so_symbol(&so_mod, "SDL_OnApplicationDidBecomeActive"), (uintptr_t)&SDL_OnApplicationDidBecomeActive);
        hook_addr(so_symbol(&so_mod, "SDL_OnApplicationDidEnterBackground"), (uintptr_t)&SDL_OnApplicationDidEnterBackground);
        hook_addr(so_symbol(&so_mod, "SDL_OnApplicationDidReceiveMemoryWarning"), (uintptr_t)&SDL_OnApplicationDidReceiveMemoryWarning);
        hook_addr(so_symbol(&so_mod, "SDL_OnApplicationWillEnterForeground"), (uintptr_t)&SDL_OnApplicationWillEnterForeground);
        hook_addr(so_symbol(&so_mod, "SDL_OnApplicationWillResignActive"), (uintptr_t)&SDL_OnApplicationWillResignActive);
        hook_addr(so_symbol(&so_mod, "SDL_OnApplicationWillTerminate"), (uintptr_t)&SDL_OnApplicationWillTerminate);
        hook_addr(so_symbol(&so_mod, "SDL_OpenAudio"), (uintptr_t)&SDL_OpenAudio);
        hook_addr(so_symbol(&so_mod, "SDL_OpenAudioDevice"), (uintptr_t)&SDL_OpenAudioDevice);
        hook_addr(so_symbol(&so_mod, "SDL_OpenURL"), (uintptr_t)&SDL_OpenURL);
        hook_addr(so_symbol(&so_mod, "SDL_PauseAudio"), (uintptr_t)&SDL_PauseAudio);
        hook_addr(so_symbol(&so_mod, "SDL_PauseAudioDevice"), (uintptr_t)&SDL_PauseAudioDevice);
        hook_addr(so_symbol(&so_mod, "SDL_PeepEvents"), (uintptr_t)&SDL_PeepEvents);
        hook_addr(so_symbol(&so_mod, "SDL_PixelFormatEnumToMasks"), (uintptr_t)&SDL_PixelFormatEnumToMasks);
        hook_addr(so_symbol(&so_mod, "SDL_PollEvent"), (uintptr_t)&SDL_PollEvent);
        hook_addr(so_symbol(&so_mod, "SDL_pow"), (uintptr_t)&SDL_pow);
        hook_addr(so_symbol(&so_mod, "SDL_powf"), (uintptr_t)&SDL_powf);
        hook_addr(so_symbol(&so_mod, "SDL_PumpEvents"), (uintptr_t)&SDL_PumpEvents);
        hook_addr(so_symbol(&so_mod, "SDL_PushEvent"), (uintptr_t)&SDL_PushEvent);
        hook_addr(so_symbol(&so_mod, "SDL_qsort"), (uintptr_t)&SDL_qsort);
        hook_addr(so_symbol(&so_mod, "SDL_QueryTexture"), (uintptr_t)&SDL_QueryTexture);
        hook_addr(so_symbol(&so_mod, "SDL_QueueAudio"), (uintptr_t)&SDL_QueueAudio);
        hook_addr(so_symbol(&so_mod, "SDL_Quit"), (uintptr_t)&SDL_Quit);
        hook_addr(so_symbol(&so_mod, "SDL_QuitSubSystem"), (uintptr_t)&SDL_QuitSubSystem);
        hook_addr(so_symbol(&so_mod, "SDL_RaiseWindow"), (uintptr_t)&SDL_RaiseWindow);
        hook_addr(so_symbol(&so_mod, "SDL_ReadBE16"), (uintptr_t)&SDL_ReadBE16);
        hook_addr(so_symbol(&so_mod, "SDL_ReadBE32"), (uintptr_t)&SDL_ReadBE32);
        hook_addr(so_symbol(&so_mod, "SDL_ReadBE64"), (uintptr_t)&SDL_ReadBE64);
        hook_addr(so_symbol(&so_mod, "SDL_ReadLE16"), (uintptr_t)&SDL_ReadLE16);
        hook_addr(so_symbol(&so_mod, "SDL_ReadLE32"), (uintptr_t)&SDL_ReadLE32);
        hook_addr(so_symbol(&so_mod, "SDL_ReadLE64"), (uintptr_t)&SDL_ReadLE64);
        hook_addr(so_symbol(&so_mod, "SDL_ReadU8"), (uintptr_t)&SDL_ReadU8);
        hook_addr(so_symbol(&so_mod, "SDL_realloc"), (uintptr_t)&SDL_realloc);
        hook_addr(so_symbol(&so_mod, "SDL_RecordGesture"), (uintptr_t)&SDL_RecordGesture);
        hook_addr(so_symbol(&so_mod, "SDL_RegisterEvents"), (uintptr_t)&SDL_RegisterEvents);
        hook_addr(so_symbol(&so_mod, "SDL_RemoveTimer"), (uintptr_t)&SDL_RemoveTimer);
        hook_addr(so_symbol(&so_mod, "SDL_RenderClear"), (uintptr_t)&SDL_RenderClear);
        hook_addr(so_symbol(&so_mod, "SDL_RenderCopy"), (uintptr_t)&SDL_RenderCopy);
        hook_addr(so_symbol(&so_mod, "SDL_RenderCopyEx"), (uintptr_t)&SDL_RenderCopyEx);
        hook_addr(so_symbol(&so_mod, "SDL_RenderCopyExF"), (uintptr_t)&SDL_RenderCopyExF);
        hook_addr(so_symbol(&so_mod, "SDL_RenderCopyF"), (uintptr_t)&SDL_RenderCopyF);
        hook_addr(so_symbol(&so_mod, "SDL_RenderDrawLine"), (uintptr_t)&SDL_RenderDrawLine);
        hook_addr(so_symbol(&so_mod, "SDL_RenderDrawLineF"), (uintptr_t)&SDL_RenderDrawLineF);
        hook_addr(so_symbol(&so_mod, "SDL_RenderDrawLines"), (uintptr_t)&SDL_RenderDrawLines);
        hook_addr(so_symbol(&so_mod, "SDL_RenderDrawLinesF"), (uintptr_t)&SDL_RenderDrawLinesF);
        hook_addr(so_symbol(&so_mod, "SDL_RenderDrawPoint"), (uintptr_t)&SDL_RenderDrawPoint);
        hook_addr(so_symbol(&so_mod, "SDL_RenderDrawPointF"), (uintptr_t)&SDL_RenderDrawPointF);
        hook_addr(so_symbol(&so_mod, "SDL_RenderDrawPoints"), (uintptr_t)&SDL_RenderDrawPoints);
        hook_addr(so_symbol(&so_mod, "SDL_RenderDrawPointsF"), (uintptr_t)&SDL_RenderDrawPointsF);
        hook_addr(so_symbol(&so_mod, "SDL_RenderDrawRect"), (uintptr_t)&SDL_RenderDrawRect);
        hook_addr(so_symbol(&so_mod, "SDL_RenderDrawRectF"), (uintptr_t)&SDL_RenderDrawRectF);
        hook_addr(so_symbol(&so_mod, "SDL_RenderDrawRects"), (uintptr_t)&SDL_RenderDrawRects);
        hook_addr(so_symbol(&so_mod, "SDL_RenderDrawRectsF"), (uintptr_t)&SDL_RenderDrawRectsF);
        hook_addr(so_symbol(&so_mod, "SDL_RenderFillRect"), (uintptr_t)&SDL_RenderFillRect);
        hook_addr(so_symbol(&so_mod, "SDL_RenderFillRectF"), (uintptr_t)&SDL_RenderFillRectF);
        hook_addr(so_symbol(&so_mod, "SDL_RenderFillRects"), (uintptr_t)&SDL_RenderFillRects);
        hook_addr(so_symbol(&so_mod, "SDL_RenderFillRectsF"), (uintptr_t)&SDL_RenderFillRectsF);
        hook_addr(so_symbol(&so_mod, "SDL_RenderFlush"), (uintptr_t)&SDL_RenderFlush);
        hook_addr(so_symbol(&so_mod, "SDL_RenderGetClipRect"), (uintptr_t)&SDL_RenderGetClipRect);
        hook_addr(so_symbol(&so_mod, "SDL_RenderGetIntegerScale"), (uintptr_t)&SDL_RenderGetIntegerScale);
        hook_addr(so_symbol(&so_mod, "SDL_RenderGetLogicalSize"), (uintptr_t)&SDL_RenderGetLogicalSize);
        hook_addr(so_symbol(&so_mod, "SDL_RenderGetMetalCommandEncoder"), (uintptr_t)&SDL_RenderGetMetalCommandEncoder);
        hook_addr(so_symbol(&so_mod, "SDL_RenderGetMetalLayer"), (uintptr_t)&SDL_RenderGetMetalLayer);
        hook_addr(so_symbol(&so_mod, "SDL_RenderGetScale"), (uintptr_t)&SDL_RenderGetScale);
        hook_addr(so_symbol(&so_mod, "SDL_RenderGetViewport"), (uintptr_t)&SDL_RenderGetViewport);
        hook_addr(so_symbol(&so_mod, "SDL_RenderIsClipEnabled"), (uintptr_t)&SDL_RenderIsClipEnabled);
        hook_addr(so_symbol(&so_mod, "SDL_RenderPresent"), (uintptr_t)&SDL_RenderPresent);
        hook_addr(so_symbol(&so_mod, "SDL_RenderReadPixels"), (uintptr_t)&SDL_RenderReadPixels);
        hook_addr(so_symbol(&so_mod, "SDL_RenderSetClipRect"), (uintptr_t)&SDL_RenderSetClipRect);
        hook_addr(so_symbol(&so_mod, "SDL_RenderSetIntegerScale"), (uintptr_t)&SDL_RenderSetIntegerScale);
        hook_addr(so_symbol(&so_mod, "SDL_RenderSetLogicalSize"), (uintptr_t)&SDL_RenderSetLogicalSize);
        hook_addr(so_symbol(&so_mod, "SDL_RenderSetScale"), (uintptr_t)&SDL_RenderSetScale);
        hook_addr(so_symbol(&so_mod, "SDL_RenderSetViewport"), (uintptr_t)&SDL_RenderSetViewport);
        hook_addr(so_symbol(&so_mod, "SDL_RenderTargetSupported"), (uintptr_t)&SDL_RenderTargetSupported);
        hook_addr(so_symbol(&so_mod, "SDL_ReportAssertion"), (uintptr_t)&SDL_ReportAssertion);
        hook_addr(so_symbol(&so_mod, "SDL_ResetAssertionReport"), (uintptr_t)&SDL_ResetAssertionReport);
        hook_addr(so_symbol(&so_mod, "SDL_RestoreWindow"), (uintptr_t)&SDL_RestoreWindow);
        hook_addr(so_symbol(&so_mod, "SDL_round"), (uintptr_t)&SDL_round);
        hook_addr(so_symbol(&so_mod, "SDL_roundf"), (uintptr_t)&SDL_roundf);
        hook_addr(so_symbol(&so_mod, "SDL_RWclose"), (uintptr_t)&SDL_RWclose);
        hook_addr(so_symbol(&so_mod, "SDL_RWFromConstMem"), (uintptr_t)&SDL_RWFromConstMem);
        hook_addr(so_symbol(&so_mod, "SDL_RWFromFile"), (uintptr_t)&SDL_RWFromFile_fake);
        hook_addr(so_symbol(&so_mod, "SDL_RWFromFP"), (uintptr_t)&SDL_RWFromFP);
        hook_addr(so_symbol(&so_mod, "SDL_RWFromMem"), (uintptr_t)&SDL_RWFromMem);
        hook_addr(so_symbol(&so_mod, "SDL_RWread"), (uintptr_t)&SDL_RWread);
        hook_addr(so_symbol(&so_mod, "SDL_RWseek"), (uintptr_t)&SDL_RWseek);
        hook_addr(so_symbol(&so_mod, "SDL_RWsize"), (uintptr_t)&SDL_RWsize);
        hook_addr(so_symbol(&so_mod, "SDL_RWtell"), (uintptr_t)&SDL_RWtell);
        hook_addr(so_symbol(&so_mod, "SDL_RWwrite"), (uintptr_t)&SDL_RWwrite);
        hook_addr(so_symbol(&so_mod, "SDL_SaveAllDollarTemplates"), (uintptr_t)&SDL_SaveAllDollarTemplates);
        hook_addr(so_symbol(&so_mod, "SDL_SaveBMP_RW"), (uintptr_t)&SDL_SaveBMP_RW);
        hook_addr(so_symbol(&so_mod, "SDL_SaveDollarTemplate"), (uintptr_t)&SDL_SaveDollarTemplate);
        hook_addr(so_symbol(&so_mod, "SDL_scalbn"), (uintptr_t)&SDL_scalbn);
        hook_addr(so_symbol(&so_mod, "SDL_scalbnf"), (uintptr_t)&SDL_scalbnf);
        hook_addr(so_symbol(&so_mod, "SDL_SemPost"), (uintptr_t)&SDL_SemPost);
        hook_addr(so_symbol(&so_mod, "SDL_SemTryWait"), (uintptr_t)&SDL_SemTryWait);
        hook_addr(so_symbol(&so_mod, "SDL_SemValue"), (uintptr_t)&SDL_SemValue);
        hook_addr(so_symbol(&so_mod, "SDL_SemWait"), (uintptr_t)&SDL_SemWait);
        hook_addr(so_symbol(&so_mod, "SDL_SemWaitTimeout"), (uintptr_t)&SDL_SemWaitTimeout);
        hook_addr(so_symbol(&so_mod, "SDL_SensorClose"), (uintptr_t)&SDL_SensorClose);
        hook_addr(so_symbol(&so_mod, "SDL_SensorFromInstanceID"), (uintptr_t)&SDL_SensorFromInstanceID);
        hook_addr(so_symbol(&so_mod, "SDL_SensorGetData"), (uintptr_t)&SDL_SensorGetData);
        hook_addr(so_symbol(&so_mod, "SDL_SensorGetDeviceInstanceID"), (uintptr_t)&SDL_SensorGetDeviceInstanceID);
        hook_addr(so_symbol(&so_mod, "SDL_SensorGetDeviceName"), (uintptr_t)&SDL_SensorGetDeviceName);
        hook_addr(so_symbol(&so_mod, "SDL_SensorGetDeviceNonPortableType"), (uintptr_t)&SDL_SensorGetDeviceNonPortableType);
        hook_addr(so_symbol(&so_mod, "SDL_SensorGetDeviceType"), (uintptr_t)&SDL_SensorGetDeviceType);
        hook_addr(so_symbol(&so_mod, "SDL_SensorGetInstanceID"), (uintptr_t)&SDL_SensorGetInstanceID);
        hook_addr(so_symbol(&so_mod, "SDL_SensorGetName"), (uintptr_t)&SDL_SensorGetName);
        hook_addr(so_symbol(&so_mod, "SDL_SensorGetNonPortableType"), (uintptr_t)&SDL_SensorGetNonPortableType);
        hook_addr(so_symbol(&so_mod, "SDL_SensorGetType"), (uintptr_t)&SDL_SensorGetType);
        hook_addr(so_symbol(&so_mod, "SDL_SensorOpen"), (uintptr_t)&SDL_SensorOpen);
        hook_addr(so_symbol(&so_mod, "SDL_SensorUpdate"), (uintptr_t)&SDL_SensorUpdate);
        hook_addr(so_symbol(&so_mod, "SDL_SetAssertionHandler"), (uintptr_t)&SDL_SetAssertionHandler);
        hook_addr(so_symbol(&so_mod, "SDL_SetClipboardText"), (uintptr_t)&SDL_SetClipboardText);
        hook_addr(so_symbol(&so_mod, "SDL_SetClipRect"), (uintptr_t)&SDL_SetClipRect);
        hook_addr(so_symbol(&so_mod, "SDL_SetColorKey"), (uintptr_t)&SDL_SetColorKey);
        hook_addr(so_symbol(&so_mod, "SDL_SetCursor"), (uintptr_t)&SDL_SetCursor);
        hook_addr(so_symbol(&so_mod, "SDL_setenv"), (uintptr_t)&SDL_setenv);
        hook_addr(so_symbol(&so_mod, "SDL_SetError"), (uintptr_t)&SDL_SetError);
        hook_addr(so_symbol(&so_mod, "SDL_SetEventFilter"), (uintptr_t)&SDL_SetEventFilter);
        hook_addr(so_symbol(&so_mod, "SDL_SetHint"), (uintptr_t)&SDL_SetHint);
        hook_addr(so_symbol(&so_mod, "SDL_SetHintWithPriority"), (uintptr_t)&SDL_SetHintWithPriority);
        hook_addr(so_symbol(&so_mod, "SDL_SetMainReady"), (uintptr_t)&SDL_SetMainReady);
        hook_addr(so_symbol(&so_mod, "SDL_SetMemoryFunctions"), (uintptr_t)&SDL_SetMemoryFunctions);
        hook_addr(so_symbol(&so_mod, "SDL_SetModState"), (uintptr_t)&SDL_SetModState);
        hook_addr(so_symbol(&so_mod, "SDL_SetPaletteColors"), (uintptr_t)&SDL_SetPaletteColors);
        hook_addr(so_symbol(&so_mod, "SDL_SetPixelFormatPalette"), (uintptr_t)&SDL_SetPixelFormatPalette);
        hook_addr(so_symbol(&so_mod, "SDL_SetRelativeMouseMode"), (uintptr_t)&SDL_SetRelativeMouseMode);
        hook_addr(so_symbol(&so_mod, "SDL_SetRenderDrawBlendMode"), (uintptr_t)&SDL_SetRenderDrawBlendMode);
        hook_addr(so_symbol(&so_mod, "SDL_SetRenderDrawColor"), (uintptr_t)&SDL_SetRenderDrawColor);
        hook_addr(so_symbol(&so_mod, "SDL_SetRenderTarget"), (uintptr_t)&SDL_SetRenderTarget);
        hook_addr(so_symbol(&so_mod, "SDL_SetSurfaceAlphaMod"), (uintptr_t)&SDL_SetSurfaceAlphaMod);
        hook_addr(so_symbol(&so_mod, "SDL_SetSurfaceBlendMode"), (uintptr_t)&SDL_SetSurfaceBlendMode);
        hook_addr(so_symbol(&so_mod, "SDL_SetSurfaceColorMod"), (uintptr_t)&SDL_SetSurfaceColorMod);
        hook_addr(so_symbol(&so_mod, "SDL_SetSurfacePalette"), (uintptr_t)&SDL_SetSurfacePalette);
        hook_addr(so_symbol(&so_mod, "SDL_SetSurfaceRLE"), (uintptr_t)&SDL_SetSurfaceRLE);
        hook_addr(so_symbol(&so_mod, "SDL_SetTextInputRect"), (uintptr_t)&SDL_SetTextInputRect);
        hook_addr(so_symbol(&so_mod, "SDL_SetTextureAlphaMod"), (uintptr_t)&SDL_SetTextureAlphaMod);
        hook_addr(so_symbol(&so_mod, "SDL_SetTextureBlendMode"), (uintptr_t)&SDL_SetTextureBlendMode);
        hook_addr(so_symbol(&so_mod, "SDL_SetTextureColorMod"), (uintptr_t)&SDL_SetTextureColorMod);
        hook_addr(so_symbol(&so_mod, "SDL_SetTextureScaleMode"), (uintptr_t)&SDL_SetTextureScaleMode);
        hook_addr(so_symbol(&so_mod, "SDL_SetThreadPriority"), (uintptr_t)&SDL_SetThreadPriority);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowAlwaysOnTop"), (uintptr_t)&SDL_SetWindowAlwaysOnTop);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowBordered"), (uintptr_t)&SDL_SetWindowBordered);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowBrightness"), (uintptr_t)&SDL_SetWindowBrightness);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowData"), (uintptr_t)&SDL_SetWindowData);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowDisplayMode"), (uintptr_t)&SDL_SetWindowDisplayMode);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowFullscreen"), (uintptr_t)&SDL_SetWindowFullscreen);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowGammaRamp"), (uintptr_t)&SDL_SetWindowGammaRamp);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowGrab"), (uintptr_t)&SDL_SetWindowGrab);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowHitTest"), (uintptr_t)&SDL_SetWindowHitTest);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowIcon"), (uintptr_t)&SDL_SetWindowIcon);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowInputFocus"), (uintptr_t)&SDL_SetWindowInputFocus);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowKeyboardGrab"), (uintptr_t)&SDL_SetWindowKeyboardGrab);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowMaximumSize"), (uintptr_t)&SDL_SetWindowMaximumSize);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowMinimumSize"), (uintptr_t)&SDL_SetWindowMinimumSize);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowModalFor"), (uintptr_t)&SDL_SetWindowModalFor);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowMouseGrab"), (uintptr_t)&SDL_SetWindowMouseGrab);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowOpacity"), (uintptr_t)&SDL_SetWindowOpacity);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowPosition"), (uintptr_t)&SDL_SetWindowPosition);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowResizable"), (uintptr_t)&SDL_SetWindowResizable);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowShape"), (uintptr_t)&SDL_SetWindowShape);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowSize"), (uintptr_t)&SDL_SetWindowSize);
        hook_addr(so_symbol(&so_mod, "SDL_SetWindowTitle"), (uintptr_t)&SDL_SetWindowTitle);
        hook_addr(so_symbol(&so_mod, "SDL_SetYUVConversionMode"), (uintptr_t)&SDL_SetYUVConversionMode);
        hook_addr(so_symbol(&so_mod, "SDL_ShowCursor"), (uintptr_t)&SDL_ShowCursor);
        hook_addr(so_symbol(&so_mod, "SDL_ShowMessageBox"), (uintptr_t)&SDL_ShowMessageBox);
        hook_addr(so_symbol(&so_mod, "SDL_ShowSimpleMessageBox"), (uintptr_t)&SDL_ShowSimpleMessageBox);
        hook_addr(so_symbol(&so_mod, "SDL_ShowWindow"), (uintptr_t)&SDL_ShowWindow);
        hook_addr(so_symbol(&so_mod, "SDL_SIMDAlloc"), (uintptr_t)&SDL_SIMDAlloc);
        hook_addr(so_symbol(&so_mod, "SDL_SIMDFree"), (uintptr_t)&SDL_SIMDFree);
        hook_addr(so_symbol(&so_mod, "SDL_SIMDGetAlignment"), (uintptr_t)&SDL_SIMDGetAlignment);
        hook_addr(so_symbol(&so_mod, "SDL_SIMDRealloc"), (uintptr_t)&SDL_SIMDRealloc);
        hook_addr(so_symbol(&so_mod, "SDL_sin"), (uintptr_t)&SDL_sin);
        hook_addr(so_symbol(&so_mod, "SDL_sinf"), (uintptr_t)&SDL_sinf);
        hook_addr(so_symbol(&so_mod, "SDL_snprintf"), (uintptr_t)&SDL_snprintf);
        hook_addr(so_symbol(&so_mod, "SDL_SoftStretch"), (uintptr_t)&SDL_SoftStretch);
        hook_addr(so_symbol(&so_mod, "SDL_SoftStretchLinear"), (uintptr_t)&SDL_SoftStretchLinear);
        hook_addr(so_symbol(&so_mod, "SDL_sqrt"), (uintptr_t)&SDL_sqrt);
        hook_addr(so_symbol(&so_mod, "SDL_sqrtf"), (uintptr_t)&SDL_sqrtf);
        hook_addr(so_symbol(&so_mod, "SDL_sscanf"), (uintptr_t)&SDL_sscanf);
        hook_addr(so_symbol(&so_mod, "SDL_StartTextInput"), (uintptr_t)&SDL_StartTextInput);
        hook_addr(so_symbol(&so_mod, "SDL_StopTextInput"), (uintptr_t)&SDL_StopTextInput);
        hook_addr(so_symbol(&so_mod, "SDL_strcasecmp"), (uintptr_t)&SDL_strcasecmp);
        hook_addr(so_symbol(&so_mod, "SDL_strchr"), (uintptr_t)&SDL_strchr);
        hook_addr(so_symbol(&so_mod, "SDL_strcmp"), (uintptr_t)&SDL_strcmp);
        hook_addr(so_symbol(&so_mod, "SDL_strdup"), (uintptr_t)&SDL_strdup);
        hook_addr(so_symbol(&so_mod, "SDL_strlcat"), (uintptr_t)&SDL_strlcat);
        hook_addr(so_symbol(&so_mod, "SDL_strlcpy"), (uintptr_t)&SDL_strlcpy);
        hook_addr(so_symbol(&so_mod, "SDL_strlen"), (uintptr_t)&SDL_strlen);
        hook_addr(so_symbol(&so_mod, "SDL_strlwr"), (uintptr_t)&SDL_strlwr);
        hook_addr(so_symbol(&so_mod, "SDL_strncasecmp"), (uintptr_t)&SDL_strncasecmp);
        hook_addr(so_symbol(&so_mod, "SDL_strncmp"), (uintptr_t)&SDL_strncmp);
        hook_addr(so_symbol(&so_mod, "SDL_strrchr"), (uintptr_t)&SDL_strrchr);
        hook_addr(so_symbol(&so_mod, "SDL_strrev"), (uintptr_t)&SDL_strrev);
        hook_addr(so_symbol(&so_mod, "SDL_strstr"), (uintptr_t)&SDL_strstr);
        hook_addr(so_symbol(&so_mod, "SDL_strtod"), (uintptr_t)&SDL_strtod);
        hook_addr(so_symbol(&so_mod, "SDL_strtokr"), (uintptr_t)&SDL_strtokr);
        hook_addr(so_symbol(&so_mod, "SDL_strtol"), (uintptr_t)&SDL_strtol);
        hook_addr(so_symbol(&so_mod, "SDL_strtoll"), (uintptr_t)&SDL_strtoll);
        hook_addr(so_symbol(&so_mod, "SDL_strtoul"), (uintptr_t)&SDL_strtoul);
        hook_addr(so_symbol(&so_mod, "SDL_strtoull"), (uintptr_t)&SDL_strtoull);
        hook_addr(so_symbol(&so_mod, "SDL_strupr"), (uintptr_t)&SDL_strupr);
        hook_addr(so_symbol(&so_mod, "SDL_tan"), (uintptr_t)&SDL_tan);
        hook_addr(so_symbol(&so_mod, "SDL_tanf"), (uintptr_t)&SDL_tanf);
        hook_addr(so_symbol(&so_mod, "SDL_ThreadID"), (uintptr_t)&SDL_ThreadID);
        hook_addr(so_symbol(&so_mod, "SDL_TLSCleanup"), (uintptr_t)&SDL_TLSCleanup);
        hook_addr(so_symbol(&so_mod, "SDL_TLSCreate"), (uintptr_t)&SDL_TLSCreate);
        hook_addr(so_symbol(&so_mod, "SDL_TLSGet"), (uintptr_t)&SDL_TLSGet);
        hook_addr(so_symbol(&so_mod, "SDL_TLSSet"), (uintptr_t)&SDL_TLSSet);
        hook_addr(so_symbol(&so_mod, "SDL_tolower"), (uintptr_t)&SDL_tolower);
        hook_addr(so_symbol(&so_mod, "SDL_toupper"), (uintptr_t)&SDL_toupper);
        hook_addr(so_symbol(&so_mod, "SDL_trunc"), (uintptr_t)&SDL_trunc);
        hook_addr(so_symbol(&so_mod, "SDL_truncf"), (uintptr_t)&SDL_truncf);
        hook_addr(so_symbol(&so_mod, "SDL_TryLockMutex"), (uintptr_t)&SDL_TryLockMutex);
        hook_addr(so_symbol(&so_mod, "SDL_uitoa"), (uintptr_t)&SDL_uitoa);
        hook_addr(so_symbol(&so_mod, "SDL_ulltoa"), (uintptr_t)&SDL_ulltoa);
        hook_addr(so_symbol(&so_mod, "SDL_ultoa"), (uintptr_t)&SDL_ultoa);
        hook_addr(so_symbol(&so_mod, "SDL_UnionRect"), (uintptr_t)&SDL_UnionRect);
        hook_addr(so_symbol(&so_mod, "SDL_UnloadObject"), (uintptr_t)&SDL_UnloadObject);
        hook_addr(so_symbol(&so_mod, "SDL_UnlockAudio"), (uintptr_t)&SDL_UnlockAudio);
        hook_addr(so_symbol(&so_mod, "SDL_UnlockAudioDevice"), (uintptr_t)&SDL_UnlockAudioDevice);
        hook_addr(so_symbol(&so_mod, "SDL_UnlockJoysticks"), (uintptr_t)&SDL_UnlockJoysticks);
        hook_addr(so_symbol(&so_mod, "SDL_UnlockMutex"), (uintptr_t)&SDL_UnlockMutex);
        hook_addr(so_symbol(&so_mod, "SDL_UnlockSensors"), (uintptr_t)&SDL_UnlockSensors);
        hook_addr(so_symbol(&so_mod, "SDL_UnlockSurface"), (uintptr_t)&SDL_UnlockSurface);
        hook_addr(so_symbol(&so_mod, "SDL_UnlockTexture"), (uintptr_t)&SDL_UnlockTexture);
        hook_addr(so_symbol(&so_mod, "SDL_UpdateNVTexture"), (uintptr_t)&SDL_UpdateNVTexture);
        hook_addr(so_symbol(&so_mod, "SDL_UpdateTexture"), (uintptr_t)&SDL_UpdateTexture);
        hook_addr(so_symbol(&so_mod, "SDL_UpdateWindowSurface"), (uintptr_t)&SDL_UpdateWindowSurface);
        hook_addr(so_symbol(&so_mod, "SDL_UpdateWindowSurfaceRects"), (uintptr_t)&SDL_UpdateWindowSurfaceRects);
        hook_addr(so_symbol(&so_mod, "SDL_UpdateYUVTexture"), (uintptr_t)&SDL_UpdateYUVTexture);
        hook_addr(so_symbol(&so_mod, "SDL_UpperBlit"), (uintptr_t)&SDL_UpperBlit);
        hook_addr(so_symbol(&so_mod, "SDL_UpperBlitScaled"), (uintptr_t)&SDL_UpperBlitScaled);
        hook_addr(so_symbol(&so_mod, "SDL_utf8strlcpy"), (uintptr_t)&SDL_utf8strlcpy);
        hook_addr(so_symbol(&so_mod, "SDL_utf8strlen"), (uintptr_t)&SDL_utf8strlen);
        hook_addr(so_symbol(&so_mod, "SDL_VideoInit"), (uintptr_t)&SDL_VideoInit);
        hook_addr(so_symbol(&so_mod, "SDL_VideoQuit"), (uintptr_t)&SDL_VideoQuit);
        hook_addr(so_symbol(&so_mod, "SDL_vsnprintf"), (uintptr_t)&SDL_vsnprintf);
        hook_addr(so_symbol(&so_mod, "SDL_vsscanf"), (uintptr_t)&SDL_vsscanf);
        hook_addr(so_symbol(&so_mod, "SDL_Vulkan_GetInstanceExtensions"), (uintptr_t)&ret0);
        hook_addr(so_symbol(&so_mod, "SDL_Vulkan_GetVkGetInstanceProcAddr"), (uintptr_t)&ret0);
        hook_addr(so_symbol(&so_mod, "SDL_Vulkan_LoadLibrary"), (uintptr_t)&retminus1);
        hook_addr(so_symbol(&so_mod, "SDL_Vulkan_UnloadLibrary"), (uintptr_t)&do_nothing);
        hook_addr(so_symbol(&so_mod, "SDL_WaitEvent"), (uintptr_t)&SDL_WaitEvent);
        hook_addr(so_symbol(&so_mod, "SDL_WaitEventTimeout"), (uintptr_t)&SDL_WaitEventTimeout);
        hook_addr(so_symbol(&so_mod, "SDL_WaitThread"), (uintptr_t)&SDL_WaitThread);
        hook_addr(so_symbol(&so_mod, "SDL_WarpMouseGlobal"), (uintptr_t)&SDL_WarpMouseGlobal);
        hook_addr(so_symbol(&so_mod, "SDL_WarpMouseInWindow"), (uintptr_t)&SDL_WarpMouseInWindow);
        hook_addr(so_symbol(&so_mod, "SDL_WasInit"), (uintptr_t)&SDL_WasInit);
        hook_addr(so_symbol(&so_mod, "SDL_wcscasecmp"), (uintptr_t)&SDL_wcscasecmp);
        hook_addr(so_symbol(&so_mod, "SDL_wcscmp"), (uintptr_t)&SDL_wcscmp);
        hook_addr(so_symbol(&so_mod, "SDL_wcsdup"), (uintptr_t)&SDL_wcsdup);
        hook_addr(so_symbol(&so_mod, "SDL_wcslcat"), (uintptr_t)&SDL_wcslcat);
        hook_addr(so_symbol(&so_mod, "SDL_wcslcpy"), (uintptr_t)&SDL_wcslcpy);
        hook_addr(so_symbol(&so_mod, "SDL_wcslen"), (uintptr_t)&SDL_wcslen);
        hook_addr(so_symbol(&so_mod, "SDL_wcsncasecmp"), (uintptr_t)&SDL_wcsncasecmp);
        hook_addr(so_symbol(&so_mod, "SDL_wcsncmp"), (uintptr_t)&SDL_wcsncmp);
        hook_addr(so_symbol(&so_mod, "SDL_wcsstr"), (uintptr_t)&SDL_wcsstr);
        hook_addr(so_symbol(&so_mod, "SDL_WriteBE16"), (uintptr_t)&SDL_WriteBE16);
        hook_addr(so_symbol(&so_mod, "SDL_WriteBE32"), (uintptr_t)&SDL_WriteBE32);
        hook_addr(so_symbol(&so_mod, "SDL_WriteBE64"), (uintptr_t)&SDL_WriteBE64);
        hook_addr(so_symbol(&so_mod, "SDL_WriteLE16"), (uintptr_t)&SDL_WriteLE16);
        hook_addr(so_symbol(&so_mod, "SDL_WriteLE32"), (uintptr_t)&SDL_WriteLE32);
        hook_addr(so_symbol(&so_mod, "SDL_WriteLE64"), (uintptr_t)&SDL_WriteLE64);
        hook_addr(so_symbol(&so_mod, "SDL_WriteU8"), (uintptr_t)&SDL_WriteU8);
    }
}
