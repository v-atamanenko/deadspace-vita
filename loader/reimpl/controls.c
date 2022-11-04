#include <stdio.h>
#include <psp2/kernel/threadmgr.h>
#include "main.h"
#include "controls.h"

int lastX[SCE_TOUCH_MAX_REPORT] = {-1, -1, -1, -1, -1, -1, -1, -1};
int lastY[SCE_TOUCH_MAX_REPORT] = {-1, -1, -1, -1, -1, -1, -1, -1};

void (*NativeOnPointerEvent)(JNIEnv *env, jobject obj, int rawEvent, int moduleId, int eventPointerId, float eventX, float eventY);
void (*NativeOnKeyDown)(JNIEnv *env, jobject obj, int moduleId, int androidKey, int altPressed);
void (*NativeOnKeyUp)(JNIEnv *env, jobject obj, int moduleId, int androidKey, int altPressed);
void (*NativeOnAcceleration)(JNIEnv *env, jobject obj, float f1, float f2, float f3);

void controls_init() {
    // Enable analog sticks and touchscreen
    sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);

    NativeOnPointerEvent = (void*)so_symbol(&so_mod,"Java_com_ea_blast_TouchSurfaceAndroid_NativeOnPointerEvent");
    NativeOnKeyDown = (void*)so_symbol(&so_mod,"Java_com_ea_blast_KeyboardAndroid_NativeOnKeyDown");
    NativeOnKeyUp = (void*)so_symbol(&so_mod,"Java_com_ea_blast_KeyboardAndroid_NativeOnKeyUp");
    NativeOnAcceleration = (void*)so_symbol(&so_mod,"Java_com_ea_blast_AccelerometerAndroidDelegate_NativeOnAcceleration");

    sceMotionStartSampling();
}

void controls_poll() {
    pollTouch();
    pollPad();
    pollAccel();
}

void pollTouch() {
    SceTouchData touch;
    sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
    for (int i = 0; i < SCE_TOUCH_MAX_REPORT; i++) {
        if (i < touch.reportNum) {
            int x, y;
            x = (int)((float)touch.report[i].x * 960.f / 1920.0f);
            y = (int)((float)touch.report[i].y * 544.f / 1088.0f);

            if (lastX[i] == -1 || lastY[i] == -1) {
                NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerDown, kModuleTypeIdTouchScreen, i, (float)x, (float)y);
            }
            else if (lastX[i] != x || lastY[i] != y) {
                NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerMove, kModuleTypeIdTouchScreen, i, (float)x, (float)y);
            }

            lastX[i] = x;
            lastY[i] = y;
        } else {
            if (lastX[i] != -1 || lastY[i] != -1) {
                NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerUp, kModuleTypeIdTouchScreen, i, (float)lastX[i], (float)lastY[i]);
                lastX[i] = -1;
                lastY[i] = -1;
            }
        }
    }
}

static ButtonMapping mapping[] = {
        { SCE_CTRL_UP,        AKEYCODE_DPAD_UP },
        { SCE_CTRL_DOWN,      AKEYCODE_DPAD_DOWN },
        { SCE_CTRL_LEFT,      AKEYCODE_DPAD_LEFT },
        { SCE_CTRL_RIGHT,     AKEYCODE_DPAD_RIGHT },
        { SCE_CTRL_CROSS,     AKEYCODE_DPAD_CENTER },
        { SCE_CTRL_CIRCLE,    AKEYCODE_BACK },
        { SCE_CTRL_SQUARE,    AKEYCODE_BUTTON_X },
        { SCE_CTRL_TRIANGLE,  AKEYCODE_BUTTON_Y },
        { SCE_CTRL_L1,        AKEYCODE_BUTTON_L1 },
        { SCE_CTRL_R1,        AKEYCODE_BUTTON_R1 },
        { SCE_CTRL_START,     AKEYCODE_BUTTON_START },
        { SCE_CTRL_SELECT,    AKEYCODE_BUTTON_SELECT },
};

uint32_t old_buttons = 0, current_buttons = 0, pressed_buttons = 0, released_buttons = 0;

float lastLx = 0.0f, lastLy = 0.0f, lastRx = 0.0f, lastRy = 0.0f;
float lx = 0.0f, ly = 0.0f, rx = 0.0f, ry = 0.0f;

int lActive = 0, rActive = 0, lastLActive = 0, lastRActive = 0;
int fingerIdL = 0, fingerIdR = 1;

int rDown = 0;

float touchRx_last, touchRy_last, touchLx_last, touchLy_last;

int pollPad() {
    if (rDown == 1) {
        NativeOnPointerEvent(&jni, (void *) 0x42424242, kIdRawPointerUp, kModuleTypeIdTouchPad, fingerIdR+1, touchRx_last, touchRy_last);
        rDown = 0;
    }

    SceCtrlData pad;
    sceCtrlPeekBufferPositiveExt2(0, &pad, 1);

    { // Gamepad buttons
        old_buttons = current_buttons;
        current_buttons = pad.buttons;
        pressed_buttons = current_buttons & ~old_buttons;
        released_buttons = ~current_buttons & old_buttons;

        for (int i = 0; i < sizeof(mapping) / sizeof(ButtonMapping); i++) {
            if (pressed_buttons & mapping[i].sce_button) {
                //debugPrintf("NativeOnKeyDown %i\n", mapping[i].android_button);
                NativeOnKeyDown(&jni, (void *) 0x42424242, 600, mapping[i].android_button, 1);
            }
            if (released_buttons & mapping[i].sce_button) {
                //debugPrintf("NativeOnKeyUp %i\n", mapping[i].android_button);
                NativeOnKeyUp(&jni, (void *) 0x42424242, 600, mapping[i].android_button, 1);
            }
        }
    }

    // Analog sticks
    // Here we are trying to use real analog sticks to emulate Xperia Play's virtual analog sticks
    // that are trying to emulate real analog sticks :)

    // If you read Xperia Play's developers manual, its touchpad is basically a second touch screen sized 966x360.
    // I assumed that a 360x360 on each side would be the working surface for our "analog sticks" and mapped
    // the values accordingly. There is one caveat though: the camera movement is rather gesture-based, so we can't just
    // keep sending PointerMove events when stick is pushed completely to one side. That's why you can see a hack below
    // where `if (fabsf(rx) > 0.88f || fabsf(ry) > 0.38f)` we send touch up, touch down at (virtual) 0;0,
    // and only then move.

    lx = ((float)pad.lx - 128.0f) / 128.0f;
    ly = ((float)pad.ly - 128.0f) / 128.0f;
    rx = ((float)pad.rx - 128.0f) / 128.0f;
    ry = ((float)pad.ry - 128.0f) / 128.0f;

    if (fabsf(lx) < 0.10f)
        lx = 0.0f;
    if (fabsf(ly) < 0.10f)
        ly = 0.0f;
    if (fabsf(rx) < 0.10f)
        rx = 0.0f;
    if (fabsf(ry) < 0.10f)
        ry = 0.0f;

    float touchLx_radius = 99;
    float touchLy_radius = 99;
    float touchRx_radius = 150;
    float touchRy_radius = 100;

    float touchLx_base = 100;
    float touchLy_base = 100;
    float touchRx_base = 786;
    float touchRy_base = 180;

    float touchLx = touchLx_base + (touchLx_radius * lx);
    float touchLy = touchLy_base + (touchLy_radius * (ly));
    float touchRx = touchRx_base + (touchRx_radius * rx);
    float touchRy = touchRy_base + (touchRy_radius * (ry * -1));

    if (fabsf(rx) > 0.f || fabsf(ry) > 0.f) {
        rActive = 1;
    } else {
        rActive = 0;
    }

    if (fabsf(lx) > 0.f || fabsf(ly) > 0.f) {
        lActive = 1;
    } else if (lActive) {
        NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerUp, kModuleTypeIdTouchScreen, fingerIdL+1, touchLx_last, touchLy_last);
        lActive = 0;
    }

    if ((fabsf(rx) > 0.f || fabsf(ry) > 0.f) && !rDown) {
        NativeOnPointerEvent(&jni, (void *) 0x42424242, kIdRawPointerDown, kModuleTypeIdTouchPad, fingerIdR+1, touchRx_base, touchRy_base);
        NativeOnPointerEvent(&jni, (void *) 0x42424242, kIdRawPointerMove, kModuleTypeIdTouchPad, fingerIdR+1, touchRx, touchRy);
        rDown = 1;
    }

    if (fabsf(lx) > 0.f || fabsf(ly) > 0.f) {
        if (!lastLActive) {
            NativeOnPointerEvent(&jni, (void *) 0x42424242, kIdRawPointerDown, kModuleTypeIdTouchScreen, fingerIdL+1, touchLx_base, touchLy_base);
        }

        NativeOnPointerEvent(&jni, (void *) 0x42424242, kIdRawPointerMove, kModuleTypeIdTouchScreen, fingerIdL+1, touchLx, touchLy);
    }

    lastLActive = lActive;
    lastRActive = rActive;

    lastLx = lx;
    lastLy = ly;
    lastRx = rx;
    lastRy = ry;

    touchRx_last = touchRx_base + touchRx_radius * lastRx;
    touchRy_last = touchRy_base + touchRy_radius * (lastRy * (-1));

    touchLx_last = touchLx_base + touchLx_radius * lastLx;
    touchLy_last = touchLy_base + touchLy_radius * (lastLy);

    return 1;
}

uint64_t lastUpdate = 0;
float accel_last_x;
float accel_last_y;
float accel_last_z;

#define SHAKE_THRESHOLD 300

void pollAccel() {
    uint64_t curTime = currenttime_ms();

    if (lastUpdate == 0) {
        lastUpdate = curTime;

        SceMotionSensorState sensor;
        sceMotionGetSensorState(&sensor, 1);
        accel_last_x = sensor.accelerometer.x*GRAVITY_CONSTANT;
        accel_last_y = sensor.accelerometer.y*GRAVITY_CONSTANT;
        accel_last_z = sensor.accelerometer.z*GRAVITY_CONSTANT;
        return;
    }

    if ((curTime - lastUpdate) > 75) {
        float diffTime = (float)(curTime - lastUpdate);
        lastUpdate = curTime;

        SceMotionSensorState sensor;
        sceMotionGetSensorState(&sensor, 1);

        float x = sensor.accelerometer.x*GRAVITY_CONSTANT;
        float y = (sensor.accelerometer.y + 1.0f)*GRAVITY_CONSTANT;
        float z = sensor.accelerometer.z*GRAVITY_CONSTANT;

        float speed = fabsf(x + y + z - accel_last_x - accel_last_y - accel_last_z) / diffTime * 10000;

        if (speed > SHAKE_THRESHOLD) {
            NativeOnAcceleration(&jni, (void*)0x42424242, x, y, z);
        } else {
            NativeOnAcceleration(&jni, (void*)0x42424242, accel_last_x, accel_last_y, accel_last_z);
        }

        accel_last_x = x;
        accel_last_y = y;
        accel_last_z = z;
    }
}
