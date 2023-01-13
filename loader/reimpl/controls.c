#include <stdio.h>
#include <psp2/kernel/threadmgr.h>
#include "main.h"
#include "controls.h"
#include "utils/settings.h"

int lastX[SCE_TOUCH_MAX_REPORT] = {-1, -1, -1, -1, -1, -1, -1, -1};
int lastY[SCE_TOUCH_MAX_REPORT] = {-1, -1, -1, -1, -1, -1, -1, -1};


float lerp(float x1, float y1, float x3, float y3, float x2) {
    return ((x2-x1)*(y3-y1) / (x3-x1)) + y1;
}

float coord_normalize(float val, float deadzone, float max) {
    float sign = 1.0f;
    if (val < 0) sign = -1.0f;

    if (fabsf(val) < deadzone) return 0.f;
    return lerp(0.f, deadzone * sign, 1.0f*sign, max*sign, val);
}

typedef struct measurings {
    float x;
    float y;
    float z;
    uint64_t time;
} measurings;

measurings fakeAccel[] = {
        { -0.998903f, -0.244644f, -1.292267f, 75 },
        { -0.954221f, -0.367070f, -1.234914f, 75 },
        { -1.029063f, -0.042471f, -1.256281f, 75 },
        { -0.983264f, -0.268230f, -1.131455f, 75 },
        { -1.049170f, -0.140187f, -1.296765f },
        { -1.113959f, -0.304173f, -1.339498f, 75 },
        { -1.655727f, 2.366763f, -2.261636f, 75 },
        { 4.224865f, -2.941413f, -0.447724f, 75 },
        { 2.926493f, -1.684568f, -1.615015f, 75 },
        { -1.694824f, 1.514265f, -1.591399f, 75 },
        { -2.287976f, 0.611223f, -1.149448f, 75 },
        { -1.193269f, -0.221057f, -0.920038f, 75 },
        { -1.024595f, -0.370441f, -1.369861f, 75 },
        { -1.054755f, -0.157035f, -1.234914f, 75 },
        { -1.159758f, -0.240151f, -1.291142f, 75 },
        { -0.938582f, -0.179499f, -1.225918f, 75 },
        { -1.046936f, -0.228920f, -1.210174f, 75 },
        { -1.001137f, -0.234535f, -1.270900f, 75 },
        { -1.055872f, -0.152542f, -1.367612f, 75 },
        { -0.945284f, -0.057073f, -1.139327f, 75 },
        { -1.002254f, -0.198594f, -1.224793f, 75 },
        { -0.747567f, -0.235659f, -1.256281f, 75 },
        { -0.699534f, -0.044716f, -1.474445f, 75 },
        { -0.711821f, 0.117022f, -1.409221f, 75 },
        { -1.023478f, 0.222602f, -1.628510f, 75 },
        { -0.848101f, -0.006528f, -2.083956f, 75 },
        { -0.272822f, -0.489498f, -3.042079f, 75 },
        { -0.527508f, -0.292941f, -3.173653f, 75 },
        { -1.081564f, 0.283254f, -3.189397f, 75 },
        { -1.405508f, 0.348398f, -2.807047f, 75 },
        { -1.814348f, 0.492166f, -2.401081f, 75 },
        { -1.502691f, 0.095681f, -2.793552f, 75 },
        { -1.847859f, 0.522491f, -3.261368f, 75 },

        // for zero g jump
        { -1.761846f, -0.322144f, -1.657748f, 250 },
        { -1.188801f, -0.022254f, -1.695983f, 75 },
        { -0.794483f, -0.263738f, -2.931873f, 75 },
        { -0.745333f, -0.150296f, -2.825040f, 75 },
        { -0.873793f, -0.444571f, -1.743215f, 75 },
        { -0.804536f, -0.127832f, -3.795533f, 75 },
        { -1.742857f, 0.038399f, -1.233790f, 75 },
        { -0.812356f, 0.224848f, -2.738449f, 75 },
        { 0.863030f, 3.779731f, -9.721955f, 75 },
        { -4.183607f, 3.622485f, 8.522037f, 75 },
        { -6.169717f, 7.627765f, 23.052782f, 75 },
        { -0.738630f, 6.358565f, 14.906132f, 75 },
        { 0.151609f, 2.630712f, 9.449697f, 75 },
        { 0.831056f, 3.416942f, 6.462183f, 75 },
        { -2.905704f, 5.808204f, -9.787180f, 75 },
        { -2.807403f, 1.275026f, -4.152019f, 75 },
        { -2.541546f, -0.363701f, 0.349680f, 75 },
        { -2.520322f, 0.236080f, -2.166049f, 75 },
        { -1.934989f, -0.196346f, -0.763724f, 75 },
        { -2.559419f, -0.453556f, 0.108265f, 75 },
        { -1.904829f, -0.004282f, -1.038117f, 75 },
        { -2.014299f, -0.291818f, -0.051879f, 75 },
        { -2.021002f, -0.269354f, -1.045988f, 75 },
        { -1.804294f, -0.230042f, -1.018999f, 75 },
        { -2.016533f, -0.192978f, -0.518571f, 75 },
        { -1.768549f, -0.101999f, -1.323754f, 75 },
        { -1.541788f, -0.406383f, -1.025746f, 75 },
};


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

volatile int fakeAccelRunning = 0;

int fakeAccelThread(SceSize args, void* argp) {
    int count = sizeof(fakeAccel) / sizeof(measurings);
    printf("sizeof fakeAccel %i\n", count);
    for (int i = 0; i < count; ++i) {
        sceKernelDelayThread(fakeAccel[i].time * 100);
        NativeOnAcceleration(&jni, (void*)0x42424242, fakeAccel[i].x, fakeAccel[i].y, fakeAccel[i].z);
    }
    fakeAccelRunning = 0;
    printf("1=>0\n");
    return sceKernelExitDeleteThread(0);
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
                if (mapping[i].sce_button == SCE_CTRL_UP && fakeAccel_enabled) {
                    if (fakeAccelRunning == 0) {
                        fakeAccelRunning = 1;
                        SceUID fake_accel_thread = sceKernelCreateThread("fakeaccel", fakeAccelThread, 0x40, 0x1000, 0, 0, NULL);
                        sceKernelStartThread(fake_accel_thread, 0, NULL);
                    }
                } else {
                    NativeOnKeyDown(&jni, (void *) 0x42424242, 600, mapping[i].android_button, 1);
                }

            }
            if (released_buttons & mapping[i].sce_button) {
                //debugPrintf("NativeOnKeyUp %i\n", mapping[i].android_button);
                if (mapping[i].sce_button != SCE_CTRL_UP || !fakeAccel_enabled) {
                    NativeOnKeyUp(&jni, (void *) 0x42424242, 600, mapping[i].android_button, 1);
                }

            }
        }
    }

    // Analog sticks
    lx = coord_normalize(((float)pad.lx - 128.0f) / 128.0f, leftStickDeadZone, 0.76f);
    ly = coord_normalize(((float)pad.ly - 128.0f) / 128.0f, leftStickDeadZone, 0.76f);
    rx = coord_normalize(((float)pad.rx - 128.0f) / 128.0f, rightStickDeadZone, 1.0f);
    ry = coord_normalize(((float)pad.ry - 128.0f) / 128.0f, rightStickDeadZone, 1.0f);

    float touchLx_radius = 75;
    float touchLy_radius = 75;
    float touchRx_radius = 155;
    float touchRy_radius = 105;

    float touchLx_base = 192;
    float touchLy_base = 75;
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

    touchRx_last = touchRx;
    touchRy_last = touchRy;
    touchLx_last = touchLx;
    touchLy_last = touchLy;

    return 1;
}

uint64_t lastUpdate = 0;
float accel_last_x;
float accel_last_y;
float accel_last_z;

#define SHAKE_THRESHOLD 300

void pollAccel() {
    if (fakeAccel_enabled) return;

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
