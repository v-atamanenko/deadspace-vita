/*
 * main.c
 *
 * ARMv7 Shared Libraries loader. Baba Is You edition.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2022 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "main.h"

#include <pthread.h>
#include <string.h>

#include <psp2/apputil.h>
#include <psp2/power.h>

#include "default_dynlib.h"
#include "fios.h"
#include "glutil.h"
#include "jni.h"
#include "patch_game.h"
#include "reimpl/io.h"
#include "utils/dialog.h"

// Disable IDE complaints about _identifiers and unused variables
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "bugprone-reserved-identifier"

unsigned int sceLibcHeapSize = MEMORY_LIBC_MB * 1024 * 1024;
int _newlib_heap_size_user = MEMORY_NEWLIB_MB * 1024 * 1024;
unsigned int _pthread_stack_default_user = STACK_PTHREAD_MB * 1024 * 1024;

so_module so_mod;

// This symbol is required for set_tex() reimplementation needed when using
// PVR_PSP2 graphics driver.
void (* gl_flush_cache)(void); // _Z14gl_flush_cachev

int main() {
    SceAppUtilInitParam init_param;
    SceAppUtilBootParam boot_param;
    memset(&init_param, 0, sizeof(SceAppUtilInitParam));
    memset(&boot_param, 0, sizeof(SceAppUtilBootParam));
    sceAppUtilInit(&init_param, &boot_param);

    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    if (fios_init() < 0)
        fatal_error("Fatal error: Could not initialize FIOS.");
    debugPrintf("fios_init() passed.\n");

    // Baba-specific hack. Preloading sprites into RAM to reduce loading times.
    preload();
    debugPrintf("preload() passed.\n");

    if (check_kubridge() < 0)
        fatal_error("Error kubridge.skprx is not installed.");
    debugPrintf("check_kubridge() passed.\n");

    if (so_file_load(&so_mod, SO_PATH, LOAD_ADDRESS) < 0)
        fatal_error("Error could not load %s.", SO_PATH);
    debugPrintf("so_file_load() passed.\n");

    so_relocate(&so_mod);
    debugPrintf("so_relocate() passed.\n");

    so_resolve(&so_mod, default_dynlib, SO_DYNLIB_SIZE, 0);
    debugPrintf("so_resolve() passed.\n");

    patch_game();
    debugPrintf("patch_game() passed.\n");

    so_flush_caches(&so_mod);
    debugPrintf("so_flush_caches() passed.\n");

    gl_init();
    debugPrintf("gl_init() passed.\n");

    so_initialize(&so_mod);
    debugPrintf("so_initialize() passed.\n");

    init_jni();
    debugPrintf("init_jni() passed.\n");

    // This symbol is required for set_tex() reimplementation needed when using
    // PVR_PSP2 graphics driver.
    gl_flush_cache = (void *) so_symbol(&so_mod, "_Z14gl_flush_cachev");

    // Running the .so in a thread with enlarged stack size.
    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0xA00000);
    pthread_create(&t, &attr, baba_main, NULL);
    pthread_join(t, NULL);

    return 0;
}

void *baba_main() {
    // JNI_OnLoad is responsible for providing jvm/env for the whole app and is
    // normally called by OS. See SDL2/src/core/android/SDL_android.c#L87.
    int (*JNI_OnLoad)(void *vm, void*) = (void*)so_symbol(&so_mod,"JNI_OnLoad");
    JNI_OnLoad(fake_vm, NULL);
    debugPrintf("JNI_OnLoad() passed.\n");

    int (* SDL_Main)(void) = (void *) so_symbol(&so_mod, "SDL_main");
    SDL_Main();
    debugPrintf("SDL_Main() passed.\n");

    return NULL;
}
