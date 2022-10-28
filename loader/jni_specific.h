#ifndef SOLOADER_JNI_SPECIFIC_H
#define SOLOADER_JNI_SPECIFIC_H

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wwritable-strings"

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include "jni_fake.h"
#include "android/java.io.InputStream.h"

#include "android/EAAudioCore.h"

typedef enum FIELD_TYPE {
    FIELD_TYPE_UNKNOWN   = 0,
    FIELD_TYPE_STRING    = 1,
    FIELD_TYPE_BOOLEAN   = 2,
    FIELD_TYPE_INT       = 3,
    FIELD_TYPE_INT_ARRAY = 4
} FIELD_TYPE;

typedef struct {
    int id;
    char *name;
    FIELD_TYPE f;
} NameToFieldID;

typedef struct {
    int id;
    jboolean value;
} FieldsBoolean;

typedef struct {
    int id;
    int value;
} FieldsInt;

typedef struct {
    int id;
    char *value;
} FieldsString;

typedef struct {
    int id;
    const int* value;
    jsize length;
} FieldsIntArray;

typedef struct {
    char *type_name;
    FIELD_TYPE f;
} FieldTypeMap;

typedef enum METHOD_TYPE {
    METHOD_TYPE_UNKNOWN   = 0,
    METHOD_TYPE_VOID      = 1,
    METHOD_TYPE_INT       = 2,
    METHOD_TYPE_FLOAT     = 3,
    METHOD_TYPE_LONG      = 4,
    METHOD_TYPE_BOOLEAN   = 5,
    METHOD_TYPE_OBJECT    = 6,
    METHOD_TYPE_INT_ARRAY = 7,
} METHOD_TYPE;

typedef struct {
    int id;
    char *name;
    METHOD_TYPE f;
} NameToMethodID;

typedef struct {
    int id;
    jobject (*Method)(int id, va_list args);
} MethodsObject;

typedef struct {
    int id;
    jint (*Method)(int id, va_list args);
} MethodsInt;

typedef struct {
    int id;
    jfloat (*Method)(int id, va_list args);
} MethodsFloat;

typedef struct {
    int id;
    void (*Method)(int id, va_list args);
} MethodsVoid;

typedef struct {
    int id;
    jboolean (*Method)(int id, va_list args);
} MethodsBoolean;

typedef struct {
    int id;
    jlong (*Method)(int id, va_list args);
} MethodsLong;

NameToMethodID nameToMethodId[] = {
    { 219, "Startup", METHOD_TYPE_VOID },
    { 220, "GetInstance", METHOD_TYPE_OBJECT },
    { 221, "getAssets", METHOD_TYPE_OBJECT },
    { 300, "GetAccelerometerCount", METHOD_TYPE_OBJECT },
    { 301, "IsBatteryStateAvailable", METHOD_TYPE_OBJECT },
    { 302, "GetCameraCount", METHOD_TYPE_OBJECT },
    { 303, "GetChipset", METHOD_TYPE_OBJECT },
    { 304, "GetCompassCount", METHOD_TYPE_OBJECT },
    { 305, "GetManufacturer", METHOD_TYPE_OBJECT },
    { 306, "GetDeviceModel", METHOD_TYPE_OBJECT },
    { 307, "GetDeviceName", METHOD_TYPE_OBJECT },
    { 308, "GetPhoneNumber", METHOD_TYPE_OBJECT },
    { 309, "GetDeviceSubscriberID", METHOD_TYPE_OBJECT },
    { 310, "GetDeviceUniqueId", METHOD_TYPE_OBJECT },
    { 311, "GetDisplayCount", METHOD_TYPE_OBJECT },
    { 312, "GetGyroscopeCount", METHOD_TYPE_OBJECT },
    { 313, "GetLocationAvailable", METHOD_TYPE_OBJECT },
    { 314, "GetMicrophoneCount", METHOD_TYPE_OBJECT },
    { 315, "GetApiLevel", METHOD_TYPE_OBJECT },
    { 316, "GetPlatformRawName", METHOD_TYPE_OBJECT },
    { 317, "GetPlatformStdName", METHOD_TYPE_OBJECT },
    { 318, "GetPlatformVersion", METHOD_TYPE_OBJECT },
    { 319, "GetPhysicalKeyboardCount", METHOD_TYPE_OBJECT },
    { 320, "GetProcessorArchitecture", METHOD_TYPE_OBJECT },
    { 321, "GetLanguage", METHOD_TYPE_OBJECT },
    { 322, "GetLocale", METHOD_TYPE_OBJECT },
    { 323, "GetTotalRAM", METHOD_TYPE_OBJECT },
    { 324, "GetTouchPadCount", METHOD_TYPE_OBJECT },
    { 325, "GetTouchScreenCount", METHOD_TYPE_OBJECT },
    { 326, "GetTrackBallCount", METHOD_TYPE_OBJECT },
    { 327, "GetVibratorCount", METHOD_TYPE_OBJECT },
    { 328, "GetVirtualKeyboardCount", METHOD_TYPE_OBJECT },

    // Class initializers
    { 401, "com/ea/blast/SystemAndroidDelegate/<init>", METHOD_TYPE_OBJECT },
    { 402, "com/ea/blast/PowerManagerAndroid/<init>", METHOD_TYPE_OBJECT },
    { 403, "com/ea/blast/DisplayAndroidDelegate/<init>", METHOD_TYPE_OBJECT },
    { 404, "com/ea/blast/DeviceOrientationHandlerAndroidDelegate/<init>", METHOD_TYPE_OBJECT },
    { 405, "com/ea/blast/GetAppDataDirectoryDelegate/<init>", METHOD_TYPE_OBJECT },
    { 405, "com/ea/blast/AccelerometerAndroidDelegate/<init>", METHOD_TYPE_OBJECT },

    // AssetManager and java.io.InputStream
    { 600, "read", METHOD_TYPE_INT },
    { 601, "close", METHOD_TYPE_VOID },
    { 602, "skip", METHOD_TYPE_OBJECT },
    { 603, "open", METHOD_TYPE_OBJECT },
    { 604, "openFd", METHOD_TYPE_OBJECT },
    { 605, "list", METHOD_TYPE_OBJECT },
    { 606, "getLength", METHOD_TYPE_LONG },

    { 100, "isContentReady", METHOD_TYPE_BOOLEAN },
    { 101, "ApplyKeepAwake", METHOD_TYPE_VOID },
    { 102, "GetStdOrientation", METHOD_TYPE_INT },
    { 103, "GetDefaultWidth", METHOD_TYPE_INT },
    { 104, "GetDefaultHeight", METHOD_TYPE_INT },
    { 105, "GetDpiX", METHOD_TYPE_FLOAT },
    { 106, "GetDpiY", METHOD_TYPE_FLOAT },
    { 107, "GetAppDataDirectory", METHOD_TYPE_OBJECT},
    { 108, "GetExternalStorageDirectory", METHOD_TYPE_OBJECT},

    { 109, "SetStdOrientation", METHOD_TYPE_VOID },
    { 110, "OnLifeCycleFocusGained", METHOD_TYPE_VOID },
    { 111, "SetEnabled", METHOD_TYPE_VOID },
    { 112, "IsTouchScreenMultiTouch", METHOD_TYPE_BOOLEAN },
    { 113, "getVersion", METHOD_TYPE_OBJECT },
    { 114, "getTotalMemory", METHOD_TYPE_LONG },
    { 115, "SetUpdateFrequency", METHOD_TYPE_VOID },

    // EAAudioCore
    { 116, "play", METHOD_TYPE_VOID },
    { 117, "stop", METHOD_TYPE_VOID },
    { 118, "write", METHOD_TYPE_INT },
};

/*
 *
[JNI] GetStaticMethodID(env, clazz, "GetInstance", "()Lcom/ea/blast/MainActivity;"): unknown method name
CallStaticObjectMethodV 0
[JNI] GetMethodID(env, clazz, "getAssets", "()Landroid/content/res/AssetManager;"): unknown method name

 */

extern void (*Java_com_ea_EAIO_EAIO_Startup)(JNIEnv*, void*, jobject);
// com/ea/EAIO/EAIO/Startup
void ea_EAIO_Startup(int id, va_list args) {
    void* assetManager = va_arg(args, void*);
    debugPrintf("JNI: Method Call: com/ea/EAIO/EAIO/Startup(AssetManager: 0x%x) / id: %i\n", (int)assetManager, id);

    Java_com_ea_EAIO_EAIO_Startup(&jni, NULL, assetManager);
}

// com/ea/blast/MainActivity/GetInstance
jobject ea_blast_MainActivity_GetInstance(int id, va_list args) {
    void* assetManager = va_arg(args, void*);
    debugPrintf("JNI: Method Call: com/ea/blast/MainActivity/GetInstance() / id: %i\n", id);
    return strdup("MainActivityInstance");
}

// 	com/android/content/Context/getAssets
jobject android_content_Context_getAssets(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/android/content/Context/getAssets() / id: %i\n", id);
    return strdup("getAssetsInstance");
}

const char* _string_empty = "";
const char* _string_0 = "0";
const char* _string_1 = "1";
const char* _string_2 = "2";
const char* _string_19 = "19";
const char* _string_minus1 = "-1";
const char* _string_manufacturer = "sony";
const char* _string_devicename = "R800";
const char* _string_devicemodel = "Vita";
const char* _string_true = "true";
const char* _string_android = "Android";
const char* _string_release = "4.4.4";
const char* _string_cpuarch = "armeabi-v7a";
const char* _string_en = "en";


// 	com/ea/blast/SystemAndroidDelegate/GetAccelerometerCount
jobject ea_blast_SystemAndroidDelegate_GetAccelerometerCount(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetAccelerometerCount() / id: %i\n", id);

    return (jobject)_string_1;
}

// 	com/ea/blast/SystemAndroidDelegate/IsBatteryStateAvailable
jobject ea_blast_SystemAndroidDelegate_IsBatteryStateAvailable(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/IsBatteryStateAvailable() / id: %i\n", id);

    return (jobject)_string_0;
}

// 	com/ea/blast/SystemAndroidDelegate/GetCameraCount
jobject ea_blast_SystemAndroidDelegate_GetCameraCount(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetCameraCount() / id: %i\n", id);

    return (jobject)_string_0;
}

// 	com/ea/blast/SystemAndroidDelegate/GetChipset
jobject ea_blast_SystemAndroidDelegate_GetChipset(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetChipset() / id: %i\n", id);

    return (jobject)_string_0;
}

// 	com/ea/blast/SystemAndroidDelegate/GetCompassCount
jobject ea_blast_SystemAndroidDelegate_GetCompassCount(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetCompassCount() / id: %i\n", id);

    return (jobject)_string_0;
}

// 	com/ea/blast/SystemAndroidDelegate/GetManufacturer
jobject ea_blast_SystemAndroidDelegate_GetManufacturer(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetManufacturer() / id: %i\n", id);

    return (jobject)_string_manufacturer;
}

// 	com/ea/blast/SystemAndroidDelegate/GetDeviceModel
jobject ea_blast_SystemAndroidDelegate_GetDeviceModel(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetDeviceModel() / id: %i\n", id);

    return (jobject)_string_devicemodel;
}

// 	com/ea/blast/SystemAndroidDelegate/GetDeviceName
jobject ea_blast_SystemAndroidDelegate_GetDeviceName(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetDeviceName() / id: %i\n", id);

    return (jobject)_string_devicename;
}

// 	com/ea/blast/SystemAndroidDelegate/GetPhoneNumber
jobject ea_blast_SystemAndroidDelegate_GetPhoneNumber(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetPhoneNumber() / id: %i\n", id);

    return (jobject)_string_0;
}

// 	com/ea/blast/SystemAndroidDelegate/GetDeviceSubscriberID
jobject ea_blast_SystemAndroidDelegate_GetDeviceSubscriberID(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetDeviceSubscriberID() / id: %i\n", id);

    return (jobject)_string_0;
}

// 	com/ea/blast/SystemAndroidDelegate/GetDeviceUniqueId
jobject ea_blast_SystemAndroidDelegate_GetDeviceUniqueId(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetDeviceUniqueId() / id: %i\n", id);

    return (jobject)_string_minus1;
}

// 	com/ea/blast/SystemAndroidDelegate/GetDisplayCount
jobject ea_blast_SystemAndroidDelegate_GetDisplayCount(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetDisplayCount() / id: %i\n", id);

    return (jobject)_string_1;
}

// 	com/ea/blast/SystemAndroidDelegate/GetGyroscopeCount
jobject ea_blast_SystemAndroidDelegate_GetGyroscopeCount(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetGyroscopeCount() / id: %i\n", id);

    return (jobject)_string_1;
}

// 	com/ea/blast/SystemAndroidDelegate/GetLocationAvailable
jobject ea_blast_SystemAndroidDelegate_GetLocationAvailable(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetLocationAvailable() / id: %i\n", id);

    return (jobject)_string_true;
}

// 	com/ea/blast/SystemAndroidDelegate/GetMicrophoneCount
jobject ea_blast_SystemAndroidDelegate_GetMicrophoneCount(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetMicrophoneCount() / id: %i\n", id);

    return (jobject)_string_1;
}

// 	com/ea/blast/SystemAndroidDelegate/GetApiLevel
jobject ea_blast_SystemAndroidDelegate_GetApiLevel(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetApiLevel() / id: %i\n", id);

    return (jobject)_string_19;
}

// 	com/ea/blast/SystemAndroidDelegate/GetPlatformRawName
jobject ea_blast_SystemAndroidDelegate_GetPlatformRawName(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetPlatformRawName() / id: %i\n", id);

    return (jobject)_string_android;
}

// 	com/ea/blast/SystemAndroidDelegate/GetPlatformStdName
jobject ea_blast_SystemAndroidDelegate_GetPlatformStdName(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetPlatformStdName() / id: %i\n", id);

    return (jobject)_string_android;
}

// 	com/ea/blast/SystemAndroidDelegate/GetPlatformVersion
jobject ea_blast_SystemAndroidDelegate_GetPlatformVersion(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetPlatformVersion() / id: %i\n", id);

    return (jobject)_string_release;
}

// 	com/ea/blast/SystemAndroidDelegate/GetPhysicalKeyboardCount
jobject ea_blast_SystemAndroidDelegate_GetPhysicalKeyboardCount(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetPhysicalKeyboardCount() / id: %i\n", id);

    return (jobject)_string_1;
}

// 	com/ea/blast/SystemAndroidDelegate/GetProcessorArchitecture
jobject ea_blast_SystemAndroidDelegate_GetProcessorArchitecture(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetProcessorArchitecture() / id: %i\n", id);

    return (jobject)_string_cpuarch;
}

// 	com/ea/blast/SystemAndroidDelegate/GetLanguage
jobject ea_blast_SystemAndroidDelegate_GetLanguage(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetLanguage() / id: %i\n", id);
    // TODO: Check for system language
    return (jobject)_string_en;
}

// 	com/ea/blast/SystemAndroidDelegate/GetLocale
jobject ea_blast_SystemAndroidDelegate_GetLocale(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetLocale() / id: %i\n", id);
    // TODO: Check for system language
    return (jobject)_string_en;
}

// 	com/ea/blast/SystemAndroidDelegate/GetTotalRAM
jobject ea_blast_SystemAndroidDelegate_GetTotalRAM(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetTotalRAM() / id: %i\n", id);

    return (jobject)_string_minus1;
}

// 	com/ea/blast/SystemAndroidDelegate/GetTouchPadCount
jobject ea_blast_SystemAndroidDelegate_GetTouchPadCount(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetTouchPadCount() / id: %i\n", id);

    return (jobject)_string_1;
}

// 	com/ea/blast/SystemAndroidDelegate/GetTouchScreenCount
jobject ea_blast_SystemAndroidDelegate_GetTouchScreenCount(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetTouchScreenCount() / id: %i\n", id);

    return (jobject)_string_1;
}

// 	com/ea/blast/SystemAndroidDelegate/GetTrackBallCount
jobject ea_blast_SystemAndroidDelegate_GetTrackBallCount(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetTrackBallCount() / id: %i\n", id);

    return (jobject)_string_0;
}

// 	com/ea/blast/SystemAndroidDelegate/GetVibratorCount
jobject ea_blast_SystemAndroidDelegate_GetVibratorCount(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetVibratorCount() / id: %i\n", id);
    // TODO: Support for DualShock vibrator?
    return (jobject)_string_0;
}

// 	com/ea/blast/SystemAndroidDelegate/GetVirtualKeyboardCount
jobject ea_blast_SystemAndroidDelegate_GetVirtualKeyboardCount(int id, va_list args) {
    debugPrintf("JNI: Method Call: com/ea/blast/SystemAndroidDelegate/GetVirtualKeyboardCount() / id: %i\n", id);
    // TODO: Support for DualShock vibrator?
    return (jobject)_string_1;
}

// 	com/ea/blast/GetAppDataDirectoryDelegate/GetAppDataDirectory
jobject GetAppDataDirectory(int id, va_list args) {
    debugPrintf("JNI: Method Call: GetAppDataDirectory() / id: %i\n", id);
    char * dir = DATA_PATH;
    return (jobject) strdup(dir);
}

// 	com/ea/blast/GetAppDataDirectoryDelegate/GetExternalStorageDirectory
jobject GetExternalStorageDirectory(int id, va_list args) {
    debugPrintf("JNI: Method Call: GetExternalStorageDirectory() / id: %i\n", id);
    char * dir = DATA_PATH;
    return (jobject) strdup(dir);
}

jobject dummyConstructor(int id, va_list args) {
    debugPrintf("JNI: Method Call: <init>() / id: %i\n", id);
    // There might be an attempt to free this object; anyway, we can safely
    // leak a few bytes here.
    char * dummy = malloc(4);
    strcpy(dummy, "nop");
    return (jobject)dummy;
}

jboolean isContentReady(int id, va_list args) {
    debugPrintf("JNI: Method Call: isContentReady() / id: %i\n", id);
    return JNI_TRUE;
}

jboolean IsTouchScreenMultiTouch(int id, va_list args) {
    debugPrintf("JNI: Method Call: IsTouchScreenMultiTouch() / id: %i\n", id);
    return JNI_TRUE;
}

// com/eamobile/Query/getVersion
jobject getVersion(int id, va_list args) {
    debugPrintf("JNI: Method Call: getVersion() / id: %i\n", id);
    return (jobject) strdup("1.0.1");
}

// com/ea/blast/PowerManagerAndroid/ApplyKeepAwake
void ApplyKeepAwake(int id, va_list args) {
    debugPrintf("JNI: Method Call: ApplyKeepAwake() / id: %i\n", id);
    // TODO: Maybe it really is needed to keep the device awake here?
}

// com/ea/blast/DisplayAndroidDelegate.java
int GetStdOrientation(int id, va_list args) {
    debugPrintf("JNI: Method Call: GetStdOrientation() / id: %i\n", id);
    // TODO: Maybe other values is needed? 0-3
    return 0;
}

int GetDefaultWidth(int id, va_list args) {
    debugPrintf("JNI: Method Call: GetDefaultWidth() / id: %i\n", id);
    return 544;
}

int GetDefaultHeight(int id, va_list args) {
    debugPrintf("JNI: Method Call: GetDefaultHeight() / id: %i\n", id);

    return 960;
}

float GetDpiX(int id, va_list args) {
    debugPrintf("JNI: Method Call: GetDpiX() / id: %i\n", id);
    return 200.0f;
}

float GetDpiY(int id, va_list args) {
    debugPrintf("JNI: Method Call: GetDpiY() / id: %i\n", id);
    return 200.0f;
}

// com/ea/blast/DeviceOrientationHandlerAndroidDelegate/SetStdOrientation
void SetStdOrientation(int id, va_list args) {
    debugPrintf("JNI: Method Call: SetStdOrientation() / id: %i\n", id);
    // We don't support changing orientation, ignore.
}

// com/ea/blast/DeviceOrientationHandlerAndroidDelegate/OnLifeCycleFocusGained
void OnLifeCycleFocusGained(int id, va_list args) {
    debugPrintf("JNI: Method Call: OnLifeCycleFocusGained() / id: %i\n", id);
    // We don't support changing orientation, ignore.
}

// com/ea/blast/AccelerometerAndroidDelegate/SetEnabled
// com/ea/blast/DeviceOrientationHandlerAndroidDelegate/SetEnabled
void SetEnabled(int id, va_list args) {
    // We deal with acceletometer stuff other way, ignore
    // We don't support changing orientation, ignore.
}

// com/ea/blast/AccelerometerAndroidDelegate/SetUpdateFrequency
void SetUpdateFrequency(int id, va_list args) {
    // We deal with acceletometer stuff other way, ignore
}

// com/eamobile/Query/getTotalMemory
jlong getTotalMemory(int id, va_list args) {
    return 256;
}

MethodsObject methodsObject[] = {
    { 220, ea_blast_MainActivity_GetInstance },
    { 221, android_content_Context_getAssets },
    { 300, ea_blast_SystemAndroidDelegate_GetAccelerometerCount },
    { 301, ea_blast_SystemAndroidDelegate_IsBatteryStateAvailable },
    { 302, ea_blast_SystemAndroidDelegate_GetCameraCount },
    { 303, ea_blast_SystemAndroidDelegate_GetChipset },
    { 304, ea_blast_SystemAndroidDelegate_GetCompassCount },
    { 305, ea_blast_SystemAndroidDelegate_GetManufacturer },
    { 306, ea_blast_SystemAndroidDelegate_GetDeviceModel },
    { 307, ea_blast_SystemAndroidDelegate_GetDeviceName },
    { 308, ea_blast_SystemAndroidDelegate_GetPhoneNumber },
    { 309, ea_blast_SystemAndroidDelegate_GetDeviceSubscriberID },
    { 310, ea_blast_SystemAndroidDelegate_GetDeviceUniqueId },
    { 311, ea_blast_SystemAndroidDelegate_GetDisplayCount },
    { 312, ea_blast_SystemAndroidDelegate_GetGyroscopeCount },
    { 313, ea_blast_SystemAndroidDelegate_GetLocationAvailable },
    { 314, ea_blast_SystemAndroidDelegate_GetMicrophoneCount },
    { 315, ea_blast_SystemAndroidDelegate_GetApiLevel },
    { 316, ea_blast_SystemAndroidDelegate_GetPlatformRawName },
    { 317, ea_blast_SystemAndroidDelegate_GetPlatformStdName },
    { 318, ea_blast_SystemAndroidDelegate_GetPlatformVersion },
    { 319, ea_blast_SystemAndroidDelegate_GetPhysicalKeyboardCount },
    { 320, ea_blast_SystemAndroidDelegate_GetProcessorArchitecture },
    { 321, ea_blast_SystemAndroidDelegate_GetLanguage },
    { 322, ea_blast_SystemAndroidDelegate_GetLocale },
    { 323, ea_blast_SystemAndroidDelegate_GetTotalRAM },
    { 324, ea_blast_SystemAndroidDelegate_GetTouchPadCount },
    { 325, ea_blast_SystemAndroidDelegate_GetTouchScreenCount },
    { 326, ea_blast_SystemAndroidDelegate_GetTrackBallCount },
    { 327, ea_blast_SystemAndroidDelegate_GetVibratorCount },
    { 328, ea_blast_SystemAndroidDelegate_GetVirtualKeyboardCount },
    // dummy constructor
    { 401, dummyConstructor },
    { 402, dummyConstructor },
    { 403, dummyConstructor },
    { 404, dummyConstructor },
    { 405, dummyConstructor },
    // fdf

    { 602, InputStream_skip },
    { 603, InputStream_open },
    { 604, InputStream_openFd },
    { 605, InputStream_list },
    // x
    { 107, GetAppDataDirectory },
    { 108, GetExternalStorageDirectory },
    { 113, getVersion }
};

MethodsLong methodsLong[] = {
    { 606, InputStream_getLength },
    { 114, getTotalMemory }
};

MethodsInt methodsInt[] = {
    { 102, GetStdOrientation },
    { 103, GetDefaultWidth },
    { 104, GetDefaultHeight },
    { 600, InputStream_read },
    { 118, EAAudioCore_AudioTrack_write }
};

MethodsFloat methodsFloat[] = {
    { 105, GetDpiX },
    { 106, GetDpiY },
};

MethodsVoid methodsVoid[] = {
    { 219, ea_EAIO_Startup },
    { 101, ApplyKeepAwake },
    { 109, SetStdOrientation },
    { 110, OnLifeCycleFocusGained },
    { 111, SetEnabled },
    { 601, InputStream_close },
    { 115, SetUpdateFrequency },
    { 116, EAAudioCore_AudioTrack_play },
    { 117, EAAudioCore_AudioTrack_stop },
};

MethodsBoolean methodsBoolean[] = {
    { 100, isContentReady },
    { 112, IsTouchScreenMultiTouch }
};

FieldTypeMap fieldTypeMap[] = {
    { "Ljava/lang/String;", FIELD_TYPE_STRING },
    { "[I", FIELD_TYPE_INT_ARRAY },
    { "I", FIELD_TYPE_INT },
    { "Z", FIELD_TYPE_BOOLEAN }
};

NameToFieldID nameToFieldId[] = {
    { 1, "WINDOW_SERVICE",         FIELD_TYPE_STRING },
    { 2, "gamepadAxisIndices",     FIELD_TYPE_INT_ARRAY },
    { 3, "gamepadAxisMinVals",     FIELD_TYPE_INT_ARRAY },
    { 4, "gamepadAxisMaxVals",     FIELD_TYPE_INT_ARRAY },
    { 5, "gamepadButtonIndices",   FIELD_TYPE_INT_ARRAY },
    { 6, "main_obb_mounted_path",  FIELD_TYPE_STRING },
    { 7, "patch_obb_mounted_path", FIELD_TYPE_STRING },
    { 8, "screenWidth", FIELD_TYPE_INT },
    { 9, "screenHeight", FIELD_TYPE_INT },
    { 10, "is_licensed", FIELD_TYPE_BOOLEAN }
};

FieldsBoolean fieldsBoolean[] = {
    { 10, JNI_TRUE }
};

FieldsInt fieldsInt[] = {
    { 8, 960 },
    { 9, 544 },
};

FieldsString fieldsString[] = {
    { 1, "window_service_field_val" },
    { 6, "ux0:/data/warband/main_unpacked" }, // no trailing slash!
    { 7, "ux0:/data/warband/patch_unpacked" }, // no trailing slash!
};

int _fieldIntArray2_value[] = {0, 1, 2, 3, 4, 5 };
int _fieldIntArray3_value[] = {0, 0, 0, 0, 0, 0 };
int _fieldIntArray4_value[] = {127, 127, 127, 127, 127, 127 };
int _fieldIntArray5_value[] = {188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 96, 97, 98, 99, 100, 101, 102, 104, 103, 105, 108, 109, 110, 3, 19, 20, 21, 22, 23};
FieldsIntArray fieldsIntArray[] = {
    { 2, _fieldIntArray2_value, sizeof(_fieldIntArray2_value) / sizeof (int) },
    { 3, _fieldIntArray3_value, sizeof(_fieldIntArray3_value) / sizeof (int) },
    { 4, _fieldIntArray4_value, sizeof(_fieldIntArray4_value) / sizeof (int) },
    { 5, _fieldIntArray5_value, sizeof(_fieldIntArray5_value) / sizeof (int) },
};

const char* fieldStringGet(int id) {
    for (int i = 0; i < sizeof(fieldsString) / sizeof(FieldsString); i++) {
        if (fieldsString[i].id == id) {
            return fieldsString[i].value;
        }
    }
    return NULL;
}

const int* fieldIntGet(int id) {
    for (int i = 0; i < sizeof(fieldsInt) / sizeof(FieldsInt); i++) {
        if (fieldsInt[i].id == id) {
            return &fieldsInt[i].value;
        }
    }
    return NULL;
}

const jboolean * fieldBoolGet(int id) {
    for (int i = 0; i < sizeof(fieldsBoolean) / sizeof(FieldsBoolean); i++) {
        if (fieldsBoolean[i].id == id) {
            return &fieldsBoolean[i].value;
        }
    }
    return NULL;
}

const int* fieldIntArrayGet(int id) {
    for (int i = 0; i < sizeof(fieldsIntArray) / sizeof(FieldsIntArray); i++) {
        if (fieldsIntArray[i].id == id) {
            return fieldsIntArray[i].value;
        }
    }
    return NULL;
}

jsize* fieldIntArrayGetLengthByPtr(const int * arr) {
    for (int i = 0; i < sizeof(fieldsIntArray) / sizeof(FieldsIntArray); i++) {
        if (fieldsIntArray[i].value == arr) {
            fprintf(stderr, "found array: id #%i\n", fieldsIntArray[i].id);
            return &fieldsIntArray[i].length;
        }
    }
    //fprintf(stderr, "not found int array\n");
    return NULL;
}


int getFieldIdByName(const char* name) {
    for (int i = 0; i < sizeof(nameToFieldId) / sizeof(NameToFieldID); i++) {
        if (strcmp(name, nameToFieldId[i].name) == 0) {
            debugPrintf("resolved to id %i\n", nameToFieldId[i].id);
            return nameToFieldId[i].id;
        }
    }

    debugPrintf("unknown field name\n");
    return 0;
}

jobject getObjectFieldValueById(int id) {
    for (int i = 0; i < sizeof(nameToFieldId) / sizeof(NameToFieldID); i++) {
        if (nameToFieldId[i].id == id) {
            switch (nameToFieldId[i].f) {
                case FIELD_TYPE_STRING: {
                    const char *ret = fieldStringGet((int)id);
                    if (ret) {
                        debugPrintf("\"%s\"\n", ret);
                        return (jobject)ret;
                    }
                    debugPrintf("String value for \"%s\" not found!\n", nameToFieldId[i].name);
                    return NULL;
                }
                case FIELD_TYPE_BOOLEAN: {
                    const jboolean *ret = fieldBoolGet((int)id);
                    if (ret) {
                        if (*ret == JNI_TRUE) {
                            debugPrintf("(bool)true\n");
                        } else {
                            debugPrintf("(bool)false\n");
                        }

                        return (jobject)ret;
                    }
                    debugPrintf("Boolean value for \"%s\" not found!\n", nameToFieldId[i].name);
                    return NULL;
                }
                case FIELD_TYPE_INT: {
                    const int *ret = fieldIntGet((int)id);
                    if (ret) {
                        debugPrintf("(int)%i\n", *ret);
                        return (jobject)ret;
                    }
                    debugPrintf("Int value for \"%s\" not found!\n", nameToFieldId[i].name);
                    return NULL;
                }
                case FIELD_TYPE_INT_ARRAY: {
                    const int *ret = fieldIntArrayGet((int)id);
                    if (ret) {
                        debugPrintf("int[\"%i\"]\n", sizeof(ret));
                        return (jobject)ret;
                    }
                    debugPrintf("Int array value for \"%s\" not found!\n", nameToFieldId[i].name);
                    return NULL;
                }
                case FIELD_TYPE_UNKNOWN:
                default:
                    debugPrintf("Unknown field type for \"%s\"!\n", nameToFieldId[i].name);
                    return NULL;
            }
        }
    }

    debugPrintf("unknown field id!\n");
    return NULL;
}


jint getIntFieldValueById(int id) {
    for (int i = 0; i < sizeof(nameToFieldId) / sizeof(NameToFieldID); i++) {
        if (nameToFieldId[i].id == id) {
            switch (nameToFieldId[i].f) {
                case FIELD_TYPE_INT: {
                    const int * x = fieldIntGet(id);
                    if (!x) {
                        debugPrintf("Int field not found!\n");
                        return 0;
                    } else {
                        debugPrintf("\"%i\"\n", *x);
                        return *x;
                    }
                }
                default:
                    debugPrintf("Unknown field type for \"%s\"!\n", nameToFieldId[i].name);
                    return 0;
            }
        }
    }

    debugPrintf("unknown field id!\n");
    return 0;
}

jboolean getBooleanFieldValueById(int id) {
    for (int i = 0; i < sizeof(nameToFieldId) / sizeof(NameToFieldID); i++) {
        if (nameToFieldId[i].id == id) {
            switch (nameToFieldId[i].f) {
                case FIELD_TYPE_BOOLEAN: {
                    const jboolean *ret = fieldBoolGet((int)id);
                    if (ret) {
                        if (*ret == JNI_TRUE) {
                            debugPrintf("(bool)true\n");
                        } else {
                            debugPrintf("(bool)false\n");
                        }

                        return (jboolean)*ret;
                    }
                    debugPrintf("Boolean value for \"%s\" not found!\n", nameToFieldId[i].name);
                    return 0;
                }
                default:
                    debugPrintf("Unknown field type for \"%s\"!\n", nameToFieldId[i].name);
                    return 0;
            }
        }
    }

    debugPrintf("unknown field id!\n");
    return 0;
}

int getMethodIdByName(const char* name) {
    for (int i = 0; i < sizeof(nameToMethodId) / sizeof(NameToMethodID); i++) {
        if (strcmp(name, nameToMethodId[i].name) == 0) {
            debugPrintf("resolved to id %i\n", nameToMethodId[i].id);
            return nameToMethodId[i].id;
        }
    }

    debugPrintf("unknown method name\n");
    return 0;
}

jobject methodObjectCall(int id, va_list args) {
    for (int i = 0; i < sizeof(methodsObject) / sizeof(MethodsObject); i++) {
        if (methodsObject[i].id == id) {
            debugPrintf("resolved : ");
            jobject ret = methodsObject[i].Method(id, args);
            debugPrintf("0x%x\n", (int)ret);
            return ret;
        }
    }

    for (int i = 0; i < sizeof(methodsBoolean) / sizeof(MethodsBoolean); i++) {
        if (methodsBoolean[i].id == id) {
            debugPrintf("resolved : ");
            jobject ret = (jobject)(int)methodsBoolean[i].Method(id, args);
            debugPrintf("0x%x\n", (int)ret);
            return ret;
        }
    }

    debugPrintf("method ID not found!\n");
    return NULL;
}

void methodVoidCall(int id, va_list args) {
    for (int i = 0; i < sizeof(methodsVoid) / sizeof(MethodsVoid); i++) {
        if (methodsVoid[i].id == id) {
            debugPrintf("resolved.\n");
            return methodsVoid[i].Method(id, args);
        }
    }

    debugPrintf("method ID not found!\n");
}

jboolean methodBooleanCall(int id, va_list args) {
    for (int i = 0; i < sizeof(methodsBoolean) / sizeof(MethodsBoolean); i++) {
        if (methodsBoolean[i].id == id) {
            debugPrintf("resolved.\n");
            return methodsBoolean[i].Method(id, args);
        }
    }

    debugPrintf("not found!\n");
    return JNI_FALSE;
}

jlong methodLongCall(int id, va_list args) {
    for (int i = 0; i < sizeof(methodsLong) / sizeof(MethodsLong); i++) {
        if (methodsLong[i].id == id) {
            debugPrintf("resolved.\n");
            return methodsLong[i].Method(id, args);
        }
    }

    debugPrintf("not found!\n");
    return -1;
}

jint methodIntCall(int id, va_list args) {
    for (int i = 0; i < sizeof(methodsInt) / sizeof(MethodsInt); i++) {
        if (methodsInt[i].id == id) {
            //debugPrintf("resolved.\n");
            return methodsInt[i].Method(id, args);
        }
    }

    //debugPrintf("not found!\n");
    return -1;
}

jfloat methodFloatCall(int id, va_list args) {
    for (int i = 0; i < sizeof(methodsFloat) / sizeof(MethodsFloat); i++) {
        if (methodsFloat[i].id == id) {
            debugPrintf("resolved.\n");
            return methodsFloat[i].Method(id, args);
        }
    }

    debugPrintf("not found!\n");
    return -1;
}

#ifdef __cplusplus
};
#endif

#endif // SOLOADER_JNI_SPECIFIC_H
