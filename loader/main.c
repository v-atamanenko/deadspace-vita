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
#include <psp2/touch.h>
#include <psp2/ctrl.h>

#include "default_dynlib.h"
#include "fios.h"
#include "glutil.h"
#include "jni_fake.h"
#include "patch_game.h"
#include "reimpl/io.h"
#include "utils/dialog.h"

#include "../lib/kubridge/kubridge.h"

// Disable IDE complaints about _identifiers and unused variables
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "bugprone-reserved-identifier"

unsigned int sceLibcHeapSize = 6 * 1024 * 1024;
int _newlib_heap_size_user = 210 * 1024 * 1024;
unsigned int _pthread_stack_default_user = 1 * 1024 * 1024;

unsigned int sceUserMainThreadStackSize = 2 * 1024 * 1024;

so_module so_mod;

void abort_handler(KuKernelAbortContext *ctx) {
    printf("Crash Detected!!! (Abort Type: 0x%08X)\n", ctx->abortType);
    printf("-----------------\n");
    printf("PC: 0x%08X\n", ctx->pc);
    printf("LR: 0x%08X\n", ctx->lr);
    printf("SP: 0x%08X\n", ctx->sp);
    printf("-----------------\n");
    printf("REGISTERS:\n");
    uint32_t *registers = (uint32_t *)ctx;
    for (int i = 0; i < 13; i++) {
        printf("R%d: 0x%08X\n", i, registers[i]);
    }
    printf("-----------------\n");
    printf("VFP REGISTERS:\n");
    for (int i = 0; i < 32; i++) {
        printf("D%d: 0x%016llX\n", i, ctx->vfpRegisters[i]);
    }
    printf("-----------------\n");
    printf("SPSR: 0x%08X\n", ctx->SPSR);
    printf("FPSCR: 0x%08X\n", ctx->FPSCR);
    printf("FPEXC: 0x%08X\n", ctx->FPEXC);
    printf("FSR: 0x%08X\n", ctx->FSR);
    printf("FAR: 0x%08X\n", *(&(ctx->FSR) + 4)); // Using ctx->FAR gives an error for some weird reason
    //sceKernelExitProcess(0);
}

int main() {
    //kuKernelRegisterAbortHandler(abort_handler, NULL);

    SceAppUtilInitParam init_param;
    SceAppUtilBootParam boot_param;
    memset(&init_param, 0, sizeof(SceAppUtilInitParam));
    memset(&boot_param, 0, sizeof(SceAppUtilBootParam));
    sceAppUtilInit(&init_param, &boot_param);

    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    // Enable analog stick and touchscreen
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);

    //if (fios_init() < 0)
    //    fatal_error("Fatal error: Could not initialize FIOS.");
    //    debugPrintf("fios_init() passed.\n");

    if (check_kubridge() < 0)
        fatal_error("Error kubridge.skprx is not installed.");
    debugPrintf("check_kubridge() passed.\n");

    if (so_file_load(&so_mod, SO_PATH, LOAD_ADDRESS) < 0)
        fatal_error("Error could not load %s.", SO_PATH);
    debugPrintf("so_file_load() passed.\n");

    so_relocate(&so_mod);
    debugPrintf("so_relocate() passed.\n");

    resolve_imports(&so_mod);
    debugPrintf("so_resolve() passed.\n");

    patch_game();
    debugPrintf("patch_game() passed.\n");

    so_flush_caches(&so_mod);
    debugPrintf("so_flush_caches() passed.\n");

    gl_preload();
    debugPrintf("gl_preload() passed.\n");

    so_initialize(&so_mod);
    debugPrintf("so_initialize() passed.\n");

    init_jni();
    debugPrintf("init_jni() passed.\n");

    gl_init();

    baba_main();
}

void (*Java_com_ea_EAIO_EAIO_Startup)(JNIEnv*, void*, jobject);

int lastX[SCE_TOUCH_MAX_REPORT] = {-1, -1, -1, -1, -1, -1, -1, -1};
int lastY[SCE_TOUCH_MAX_REPORT] = {-1, -1, -1, -1, -1, -1, -1, -1};

void (*NativeOnPointerEvent)(JNIEnv *env, jobject obj, int rawEvent, int moduleId, int eventPointerId, float eventX, float eventY);

#define kIdRawPointerCancel 0xc
#define kIdRawPointerDown 0x6000c
#define kIdRawPointerMove 0x4000c
#define kIdRawPointerUp 0x8000c
#define kIdUndefined 0

void pollTouch() {
    SceTouchData touch;
    sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
    for (int i = 0; i < SCE_TOUCH_MAX_REPORT; i++) {
        if (i < touch.reportNum) {
            int x, y;
            x = (int)((float)touch.report[i].x * (float)960 / 1920.0f);
            y = (int)((float)touch.report[i].y * (float)544 / 1088.0f);

            if (lastX[i] == -1 || lastY[i] == -1) {
                //printf("send down\n");
                NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerDown, 1000, i, (float)x, (float)y);
            }
            else if (lastX[i] != x || lastY[i] != y) {
                //printf("send move\n");
                NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerMove, 1000, i, (float)x, (float)y);
            }

            lastX[i] = x;
            lastY[i] = y;
        } else {
            if (lastX[i] != -1 || lastY[i] != -1) {
                //printf("send up\n");
                NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerUp, 1000, i, (float)lastX[i], (float)lastY[i]);
                lastX[i] = -1;
                lastY[i] = -1;
            }
        }
    }
}

_Noreturn void *baba_main() {
    Java_com_ea_EAIO_EAIO_Startup = (void*)so_symbol(&so_mod,"Java_com_ea_EAIO_EAIO_Startup");
    int (*JNI_OnLoad)(JavaVM* jvm) = (void*)so_symbol(&so_mod,"JNI_OnLoad");
    void (*Java_com_ea_blast_MainActivity_NativeOnCreate)(void) = (void*)so_symbol(&so_mod,"Java_com_ea_blast_MainActivity_NativeOnCreate");
    void (*Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceCreated)(void) = (void*)so_symbol(&so_mod,"Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceCreated");
    void (*Java_com_ea_blast_AndroidRenderer_NativeOnDrawFrame)(void) = (void*)so_symbol(&so_mod,"Java_com_ea_blast_AndroidRenderer_NativeOnDrawFrame");
    NativeOnPointerEvent = (void*)so_symbol(&so_mod,"Java_com_ea_blast_TouchSurfaceAndroid_NativeOnPointerEvent");

    JNI_OnLoad(&jvm);
    debugPrintf("JNI_OnLoad() passed.\n");

    Java_com_ea_blast_MainActivity_NativeOnCreate();
    debugPrintf("Java_com_ea_blast_MainActivity_NativeOnCreate() passed.\n");

    Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceCreated();

    while (1) {
        Java_com_ea_blast_AndroidRenderer_NativeOnDrawFrame();
        gl_swap();
        pollTouch();
    }
}
