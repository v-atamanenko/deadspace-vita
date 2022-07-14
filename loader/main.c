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
#include <sys/unistd.h>

#include "default_dynlib.h"
#include "fios.h"
#include "glutil.h"
#include "jni_fake.h"
#include "patch_game.h"
#include "reimpl/io.h"
#include "utils/dialog.h"

// Disable IDE complaints about _identifiers and unused variables
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "bugprone-reserved-identifier"

unsigned int sceLibcHeapSize = 1 * 1024 * 1024;
int _newlib_heap_size_user = 250 * 1024 * 1024;
unsigned int _pthread_stack_default_user = 512 * 1024;

unsigned int sceUserMainThreadStackSize = 5 * 1024 * 1024;

so_module so_mod;

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

    //if (fios_init() < 0)
//        fatal_error("Fatal error: Could not initialize FIOS.");
  //  debugPrintf("fios_init() passed.\n");

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

    vglInitExtended(0, 960, 544, 12 * 1024 * 1024, SCE_GXM_MULTISAMPLE_4X);

    baba_main();
    return 0;
    // Running the .so in a thread with enlarged stack size.
    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0xA00000);
    pthread_create(&t, &attr, baba_main, NULL);
    pthread_join(t, NULL);
    debugPrintf("AAA.\n");

    return 0;
}

void* (*IO__GetAllocator)();
void (*Java_com_ea_EAIO_EAIO_Startup)(JNIEnv*, void*, jobject);
void (*eastl_strappend)(void*, char*, char*);

_Noreturn void *baba_main() {

    Java_com_ea_EAIO_EAIO_Startup = (void*)so_symbol(&so_mod,"Java_com_ea_EAIO_EAIO_Startup");
    IO__GetAllocator = (void*)((uintptr_t)so_mod.text_base + 0x00271914);
    eastl_strappend = (void*)((uintptr_t)so_mod.text_base + 0x001ca8b0);

    // JNI_OnLoad is responsible for providing jvm/env for the whole app and is
    // normally called by OS. See SDL2/src/core/android/SDL_android.c#L87.

    int (*JNI_OnLoad)(JavaVM* jvm) = (void*)so_symbol(&so_mod,"JNI_OnLoad");
    JNI_OnLoad(&jvm);

    debugPrintf("JNI_OnLoad() passed.\n");

    void (*Java_com_ea_blast_MainActivity_NativeOnCreate)(void) = (void*)so_symbol(&so_mod,"Java_com_ea_blast_MainActivity_NativeOnCreate");
    Java_com_ea_blast_MainActivity_NativeOnCreate();
    debugPrintf("Java_com_ea_blast_MainActivity_NativeOnCreate() passed.\n");

    // The game inits GL from inside Java, so we'll do it here.

    //vglSwapBuffers(GL_FALSE); vglSwapBuffers(GL_FALSE);

    void (*Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceCreated)(void) = (void*)so_symbol(&so_mod,"Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceCreated");
    void (*Java_com_ea_blast_AndroidRenderer_NativeOnDrawFrame)(void) = (void*)so_symbol(&so_mod,"Java_com_ea_blast_AndroidRenderer_NativeOnDrawFrame");


    Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceCreated();

    uint64_t i = 0;
    while (1) {
        i++;
        if (i == 2) {
            // For whatever ungodly reason, you can't init GL before second frame.

            //vglSwapBuffers(GL_FALSE);vglSwapBuffers(GL_FALSE);vglSwapBuffers(GL_FALSE);
        }
        Java_com_ea_blast_AndroidRenderer_NativeOnDrawFrame();
        vglSwapBuffers(GL_FALSE);
    }

    while (1) {usleep(10000000);}
    return NULL;
}
