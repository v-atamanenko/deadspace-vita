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

void pollPad() {
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
    rx = (((float)pad.rx - 128.0f) / 128.0f) * 0.9f;
    ry = (((float)pad.ry - 128.0f) / 128.0f) * 0.9f;

    if (fabsf(lx) < 0.14f)
        lx = 0.0f;
    if (fabsf(ly) < 0.14f)
        ly = 0.0f;
    if (fabsf(rx) < 0.14f)
        rx = 0.0f;
    if (fabsf(ry) < 0.14f)
        ry = 0.0f;

    float touchX_radius = 180;
    float touchY_radius = 110;

    float touchLx_base = 180;
    float touchLy_base = 180;
    float touchRx_base = 720;
    float touchRy_base = 180;

    float touchLx = touchLx_base + touchX_radius * lx;
    float touchLy = touchLy_base + touchY_radius * (ly * -1);
    float touchRx = touchRx_base + touchX_radius * rx;
    float touchRy = touchRy_base + touchY_radius * (ry * -1);

    float touchLx_last = touchLx_base + touchX_radius * lastLx;
    float touchLy_last = touchLy_base + touchY_radius * (lastLy * -1);
    float touchRx_last = touchRx_base + touchX_radius * lastRx;
    float touchRy_last = touchRy_base + touchY_radius * (lastRy * -1);

    if ((lastLx == 0.f && lastLy == 0.f) && (lx != 0.f || ly != 0.f)) {
        // Left stick was still before and moved => touch down
        NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerDown, kModuleTypeIdTouchPad, 0, touchLx_base, touchLy_base);
        //NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerMove, kModuleTypeIdTouchPad, 0, touchLx, touchLy);
    }
    if ((lastRx == 0.f && lastRy == 0.f) && (rx != 0.f || ry != 0.f)) {
        // Right stick was still before and moved => touch down
        NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerDown, kModuleTypeIdTouchPad, 1, touchRx_base, touchRy_base);
        NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerMove, kModuleTypeIdTouchPad, 1, touchRx, touchRy);
    }

    if ((lastLx != 0.f || lastLy != 0.f) && (lx != 0.f || ly != 0.f)) {
        // Left stick continues movement
        NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerMove, kModuleTypeIdTouchPad, 0, touchLx, touchLy);
    }
    if ((lastRx != 0.f || lastRy != 0.f) && (rx != 0.f || ry != 0.f)) {
        // Right stick continues movement
        if (fabsf(rx) > 0.68f || fabsf(ry) > 0.68f) {
            NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerUp, kModuleTypeIdTouchPad, 1, touchRx_last, touchRy_last);
            NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerDown, kModuleTypeIdTouchPad, 1, touchRx_base, touchRy_base);
            NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerMove, kModuleTypeIdTouchPad, 1, touchRx, touchRy);
        } else {
            NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerMove, kModuleTypeIdTouchPad, 1, touchRx, touchRy);
        }
    }

    if ((lastLx != 0.f || lastLy != 0.f) && (lx == 0.f && ly == 0.f)) {
        // Left stick is back to 0:0 => touch up
        NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerUp, kModuleTypeIdTouchPad, 0, touchLx_last, touchLy_last);
    }
    if ((lastRx != 0.f || lastRy != 0.f) && (rx == 0.f && ry == 0.f)) {
        // Right stick is back to 0:0 => touch up
        NativeOnPointerEvent(&jni, (void*)0x42424242, kIdRawPointerUp, kModuleTypeIdTouchPad, 1, touchRx_last, touchRy_last);
    }

    lastLx = lx;
    lastLy = ly;
    lastRx = rx;
    lastRy = ry;
}

void pollAccel() {
    SceMotionSensorState sensor;
    sceMotionGetSensorState(&sensor, 1);

    float x = sensor.accelerometer.x*GRAVITY_CONSTANT;
    float y = sensor.accelerometer.y*GRAVITY_CONSTANT;
    float z = sensor.accelerometer.z*GRAVITY_CONSTANT;
    NativeOnAcceleration(&jni, (void*)0x42424242, x, y, z);
}
