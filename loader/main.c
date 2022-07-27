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
#include <psp2/motion.h>

#include "default_dynlib.h"
#include "fios.h"
#include "glutil.h"
#include "jni_fake.h"
#include "patch_game.h"
#include "reimpl/io.h"
#include "utils/dialog.h"

#include "../lib/kubridge/kubridge.h"
#include "EAAudioCore.h"

// Disable IDE complaints about _identifiers and unused variables
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "bugprone-reserved-identifier"

#define GRAVITY_CONSTANT 9.807f

unsigned int sceLibcHeapSize = 6 * 1024 * 1024;
int _newlib_heap_size_user = 250 * 1024 * 1024;
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
    debugPrintf("gl_init() passed.\n");

    baba_main();
}

void (*Java_com_ea_EAIO_EAIO_Startup)(JNIEnv*, void*, jobject);

int lastX[SCE_TOUCH_MAX_REPORT] = {-1, -1, -1, -1, -1, -1, -1, -1};
int lastY[SCE_TOUCH_MAX_REPORT] = {-1, -1, -1, -1, -1, -1, -1, -1};

void (*NativeOnPointerEvent)(JNIEnv *env, jobject obj, int rawEvent, int moduleId, int eventPointerId, float eventX, float eventY);
void (*NativeOnKeyDown)(JNIEnv *env, jobject obj, int moduleId, int androidKey, int altPressed);
void (*NativeOnKeyUp)(JNIEnv *env, jobject obj, int moduleId, int androidKey, int altPressed);
void (*NativeOnAcceleration)(JNIEnv *env, jobject obj, float f1, float f2, float f3);

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

enum {
    ACTION_DOWN = 1,
    ACTION_UP   = 2,
    ACTION_MOVE = 3,
};

enum {
    AKEYCODE_DPAD_UP = 19,
    AKEYCODE_DPAD_DOWN = 20,
    AKEYCODE_DPAD_LEFT = 21,
    AKEYCODE_DPAD_RIGHT = 22,
    AKEYCODE_A = 29,
    AKEYCODE_B = 30,
    AKEYCODE_BUTTON_X = 99,
    AKEYCODE_BUTTON_Y = 100,
    AKEYCODE_BUTTON_L1 = 102,
    AKEYCODE_BUTTON_R1 = 103,
    AKEYCODE_BUTTON_START = 108,
    AKEYCODE_BUTTON_SELECT = 109,
};

typedef struct {
    uint32_t sce_button;
    uint32_t android_button;
} ButtonMapping;

static ButtonMapping mapping[] = {
        { SCE_CTRL_UP,        AKEYCODE_DPAD_UP },
        { SCE_CTRL_DOWN,      AKEYCODE_DPAD_DOWN },
        { SCE_CTRL_LEFT,      AKEYCODE_DPAD_LEFT },
        { SCE_CTRL_RIGHT,     AKEYCODE_DPAD_RIGHT },
        { SCE_CTRL_CROSS,     AKEYCODE_A },
        { SCE_CTRL_CIRCLE,    AKEYCODE_B },
        { SCE_CTRL_SQUARE,    AKEYCODE_BUTTON_X },
        { SCE_CTRL_TRIANGLE,  AKEYCODE_BUTTON_Y },
        { SCE_CTRL_L1,        AKEYCODE_BUTTON_L1 },
        { SCE_CTRL_R1,        AKEYCODE_BUTTON_R1 },
        { SCE_CTRL_START,     AKEYCODE_BUTTON_START },
        { SCE_CTRL_SELECT,    AKEYCODE_BUTTON_SELECT },
};

uint32_t old_buttons = 0, current_buttons = 0, pressed_buttons = 0, released_buttons = 0;

void pollPad() {
    SceCtrlData pad;
    sceCtrlPeekBufferPositiveExt2(0, &pad, 1);

    old_buttons = current_buttons;
    current_buttons = pad.buttons;
    pressed_buttons = current_buttons & ~old_buttons;
    released_buttons = ~current_buttons & old_buttons;

    for (int i = 0; i < sizeof(mapping) / sizeof(ButtonMapping); i++) {
        if (pressed_buttons & mapping[i].sce_button) {
            NativeOnKeyDown(&jni, (void *) 0x42424242, 600, mapping[i].android_button, 0);
        }
        if (released_buttons & mapping[i].sce_button)
            NativeOnKeyUp(&jni, (void*)0x42424242, 600, mapping[i].android_button, 0);
    }
}

void pollAccel() {
    SceMotionSensorState sensor;
    sceMotionGetSensorState(&sensor, 1);

    float val1 = sensor.accelerometer.x*GRAVITY_CONSTANT;
    float val2 = sensor.accelerometer.y*GRAVITY_CONSTANT;
    float val3 = sensor.accelerometer.z*GRAVITY_CONSTANT;
    NativeOnAcceleration(&jni, (void*)0x42424242, val1, val2, val3);
}

void (*Java_com_ea_EAAudioCore_AndroidEAAudioCore_Init)(JNIEnv* env, jobject* obj, AudioTrack audioTrack, int i, int i2, int i3);
void (*Java_com_ea_EAAudioCore_AndroidEAAudioCore_Release)(JNIEnv* env);


_Noreturn void *baba_main() {
    Java_com_ea_EAIO_EAIO_Startup = (void*)so_symbol(&so_mod,"Java_com_ea_EAIO_EAIO_Startup");
    int (*JNI_OnLoad)(JavaVM* jvm) = (void*)so_symbol(&so_mod,"JNI_OnLoad");
    void (*Java_com_ea_blast_MainActivity_NativeOnCreate)(void) = (void*)so_symbol(&so_mod,"Java_com_ea_blast_MainActivity_NativeOnCreate");
    void (*Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceCreated)(void) = (void*)so_symbol(&so_mod,"Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceCreated");
    void (*Java_com_ea_blast_AndroidRenderer_NativeOnDrawFrame)(void) = (void*)so_symbol(&so_mod,"Java_com_ea_blast_AndroidRenderer_NativeOnDrawFrame");
    NativeOnPointerEvent = (void*)so_symbol(&so_mod,"Java_com_ea_blast_TouchSurfaceAndroid_NativeOnPointerEvent");
    NativeOnKeyDown = (void*)so_symbol(&so_mod,"Java_com_ea_blast_KeyboardAndroid_NativeOnKeyDown");
    NativeOnKeyUp = (void*)so_symbol(&so_mod,"Java_com_ea_blast_KeyboardAndroid_NativeOnKeyUp");
    NativeOnAcceleration = (void*)so_symbol(&so_mod,"Java_com_ea_blast_AccelerometerAndroidDelegate_NativeOnAcceleration");
    Java_com_ea_EAAudioCore_AndroidEAAudioCore_Init = (void*)so_symbol(&so_mod,"Java_com_ea_EAAudioCore_AndroidEAAudioCore_Init");
    Java_com_ea_EAAudioCore_AndroidEAAudioCore_Release = (void*)so_symbol(&so_mod,"Java_com_ea_EAAudioCore_AndroidEAAudioCore_Release");

    JNI_OnLoad(&jvm);
    debugPrintf("JNI_OnLoad() passed.\n");

    EAAudioCore__Startup();
    debugPrintf("EAAudioCore__Startup() passed.\n");

    Java_com_ea_blast_MainActivity_NativeOnCreate();
    debugPrintf("Java_com_ea_blast_MainActivity_NativeOnCreate() passed.\n");

    sceMotionStartSampling();

    Java_com_ea_blast_AndroidRenderer_NativeOnSurfaceCreated();

    void (*NativeOnVisibilityChanged)(JNIEnv* jniEnv, jobject obj, int moduleId, int hardKeyboardHidden) = (void*)so_symbol(&so_mod,"Java_com_ea_blast_KeyboardAndroid_NativeOnVisibilityChanged");
    NativeOnVisibilityChanged(&jni, (void*)0x42424242, 600, 1);

    while (1) {
        Java_com_ea_blast_AndroidRenderer_NativeOnDrawFrame();
        gl_swap();
        pollTouch();
        //pollPad();
        pollAccel();
    }
}
