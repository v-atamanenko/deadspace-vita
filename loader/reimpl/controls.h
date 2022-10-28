#ifndef SOLOADER_CONTROLS_H
#define SOLOADER_CONTROLS_H

#include <psp2/touch.h>
#include <psp2/ctrl.h>
#include <psp2/motion.h>

#include <math.h>

#include "jni_fake.h"

#define GRAVITY_CONSTANT 0.726f

#define kIdRawPointerCancel 0xc
#define kIdRawPointerDown 0x6000c
#define kIdRawPointerMove 0x4000c
#define kIdRawPointerUp 0x8000c
#define kIdUndefined 0

#define kModuleTypeIdTouchScreen 1000
#define kModuleTypeIdTouchPad 1100

enum {
    ACTION_DOWN = 1,
    ACTION_UP   = 2,
    ACTION_MOVE = 3,
};

enum {
    AKEYCODE_BACK = 4,
    AKEYCODE_DPAD_UP = 19,
    AKEYCODE_DPAD_DOWN = 20,
    AKEYCODE_DPAD_LEFT = 21,
    AKEYCODE_DPAD_RIGHT = 22,
    AKEYCODE_DPAD_CENTER = 23,
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

void controls_init();
void controls_poll();
void pollTouch();
void pollPad();
void pollAccel();

#endif // SOLOADER_CONTROLS_H
