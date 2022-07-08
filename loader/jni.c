/*
 * jni.c
 *
 * Fake Java Native Interface, providing JVM and Env objects.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "jni.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <psp2/apputil.h>
#include <psp2/system_param.h>

#pragma ide diagnostic ignored "UnusedParameter"

char fake_vm[0x1000];
char fake_env[0x1000];

static NameToMethodID name_to_method_ids[] = {
    { "<init>", INIT },
    { "getApkPath", GET_APK_PATH },
    { "getExternalStoragePath", GET_EXTERNAL_STORAGE_PATH },
    { "getInternalStoragePath", GET_INTERNAL_STORAGE_PATH },
    { "playSound", PLAY_SOUND },
    { "loadSound", LOAD_SOUND },
    { "stopSound", STOP_SOUND },
    { "stopAllSounds", STOP_ALL_SOUNDS },
    { "unloadSound", UNLOAD_SOUND },
    { "isPlaying", IS_PLAYING },
    { "isVBOSupported", IS_VBO_SUPPORTED },
    { "getLanguage", GET_LANGUAGE },
    { "getContext", GET_CONTEXT },
    { "getAssets", GET_ASSETS }
};

// VM functions:

int GetEnv(void *vm, void **env, int r2) {
    debugPrintf("GetEnv()\n");
    *env = fake_env;
    return 0;
}

// ENV functions:

void *FindClass(void) {
    return (void *)0x41414141;
}

void *NewGlobalRef(void *env, char *str) {
    debugPrintf("New global ref: %s\n", str);
    if (!str)
        return (void *)0x42424242;

    if (str[0] != 'r') {
        return str;
    } else {
        void *r = malloc(strlen(str) + 14);
        sprintf(r, "ux0:data/goo/%s", str);
        return r;
    }
}

void DeleteGlobalRef(void *env, char *str) {
    if (str && str[0] == 'u')
        free(str);
}

void *NewObjectV(void *env, void *clazz, int methodID, uintptr_t args) {
    return (void *)0x43434343;
}

void *GetObjectClass(void *env, void *obj) {
    return (void *)0x44444444;
}

int GetMethodID(void *env, void *class, const char *name, const char *sig) {
    debugPrintf("GetMethodID %s\n", name);

    for (int i=0; i < sizeof(name_to_method_ids)/sizeof(NameToMethodID); i++) {
        if (strcmp(name, name_to_method_ids[i].name) == 0) {
            return name_to_method_ids[i].id;
        }
    }

    return UNKNOWN;
}

void *CallObjectMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
    int lang = -1;
    debugPrintf("CallObjectMethodV %i\n", methodID);
    switch (methodID) {
        case GET_APK_PATH:
        case GET_EXTERNAL_STORAGE_PATH:
        case GET_INTERNAL_STORAGE_PATH:
        case GET_ASSETS:
            return DATA_PATH;
        case PLAY_SOUND:
            return NULL;
        case GET_LANGUAGE:
            sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &lang);

            switch (lang) {
                case SCE_SYSTEM_PARAM_LANG_ITALIAN:
                    return "it";
                case SCE_SYSTEM_PARAM_LANG_GERMAN:
                    return "de";
                case SCE_SYSTEM_PARAM_LANG_KOREAN:
                    return "ko";
                case SCE_SYSTEM_PARAM_LANG_CHINESE_S:
                case SCE_SYSTEM_PARAM_LANG_CHINESE_T:
                    return "ch";
                case SCE_SYSTEM_PARAM_LANG_SPANISH:
                    return "es";
                case SCE_SYSTEM_PARAM_LANG_FRENCH:
                    return "fr";
                case SCE_SYSTEM_PARAM_LANG_POLISH:
                    return "pl";
                case SCE_SYSTEM_PARAM_LANG_DUTCH:
                    return "nl";
                default:
                    return NULL;
            }
        default:
            return NULL;
    }
}

int CallBooleanMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
    debugPrintf("CallBooleanMethodV %i\n", methodID);
    switch (methodID) {
        case IS_PLAYING:
            //return (args[0] > 1) ? audio_player_is_playing(args[0]) : 0;
            return 0;
        case IS_VBO_SUPPORTED:
            return 1;
        default:
            return 0;
    }
}

uint64_t CallLongMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
    printf("CallLongMethodV %i\n", methodID);
    return -1;
}

void CallVoidMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
    debugPrintf("CallVoidMethodV %i\n", methodID);
    switch (methodID) {
        case STOP_SOUND:
            debugPrintf("CallVoidMethodV: STOP_SOUND\n", methodID);
            break;
        case STOP_ALL_SOUNDS:
            debugPrintf("CallVoidMethodV: STOP_ALL_SOUNDS\n", methodID);
            break;
        default:
            break;
    }
}

int GetFieldID(void *env, void *clazz, const char *name, const char *sig) {
    return 0;
}

int GetBooleanField(void *env, void *obj, int fieldID) {
    return 0;
}

int GetIntField(void *env, void *obj, int fieldID) {
    return 0;
}

int GetStaticMethodID(void *env, void *c, const char *name, const char *s) {
    debugPrintf("GetStaticMethodID(%s)\n", name);
    for (int i=0; i<sizeof(name_to_method_ids) / sizeof(NameToMethodID); i++) {
        if (strcmp(name, name_to_method_ids[i].name) == 0)
            return name_to_method_ids[i].id;
    }

    return UNKNOWN;
}

void* CallStaticObjectMethod(void *env, void *obj, int methodID, ...) {
    debugPrintf("CallStaticObjectMethod(%i/0x%x)\n", methodID, methodID);
    return NULL;
}

void* CallStaticObjectMethodV(void *e, void *o, int methodID, uintptr_t *args) {
    debugPrintf("CallStaticObjectMethodV(%i/0x%x)\n", methodID, methodID);
    switch (methodID) {
        case GET_LANGUAGE:
            return "en";
        case GET_CONTEXT:
            return "context_main";
        default:
            debugPrintf("Unexpected StaticObjectMethodV %i\n", methodID);
            return NULL;
    }
}

void* CallStaticObjectMethodA(void *e, void *o, int methodID, uintptr_t *args) {
    debugPrintf("CallStaticObjectMethodA(%i/0x%x)\n", methodID, methodID);
    return NULL;
}

int CallStaticBooleanMethodV(void *e, void *o, int methodID, uintptr_t *args) {
    debugPrintf("CallStaticBooleanMethodV(%i/0x%x)\n", methodID, methodID);
    return 0;
}

void CallStaticVoidMethodV(void *e, void *o, int methodID, uintptr_t *args) {
    debugPrintf("CallStaticVoidMethodV(%i/0x%x)\n", methodID, methodID);
}

char *NewStringUTF(void *env, char *bytes) {
    return bytes;
}

char *GetStringUTFChars(void *env, char *string, int *isCopy) {
    return string;
}

int GetJavaVM(void *env, void **vm) {
    *vm = fake_vm;
    return 0;
}

int RegisterNatives(void* e, void* c, void* methods, int method_count) {
    debugPrintf("RegisterNatives( for %i methods )\n", method_count);
    return 0;
}

void init_jni() {
    memset(fake_vm, 'A', sizeof(fake_vm));
    *(uintptr_t *)(fake_vm + 0x00) = (uintptr_t)fake_vm;
    *(uintptr_t *)(fake_vm + 0x10) = (uintptr_t)GetEnv;
    *(uintptr_t *)(fake_vm + 0x14) = (uintptr_t)ret0;
    *(uintptr_t *)(fake_vm + 0x18) = (uintptr_t)GetEnv;

    memset(fake_env, 'A', sizeof(fake_env));
    *(uintptr_t *)(fake_env + 0x00) = (uintptr_t)fake_env;
    *(uintptr_t *)(fake_env + 0x18) = (uintptr_t)FindClass;
    *(uintptr_t *)(fake_env + 0x54) = (uintptr_t)NewGlobalRef;
    *(uintptr_t *)(fake_env + 0x58) = (uintptr_t)DeleteGlobalRef;
    *(uintptr_t *)(fake_env + 0x5C) = (uintptr_t)ret0; // DeleteLocalRef
    *(uintptr_t *)(fake_env + 0x74) = (uintptr_t)NewObjectV;
    *(uintptr_t *)(fake_env + 0x4c) = (uintptr_t)ret0; // PushLocalFrame
    *(uintptr_t *)(fake_env + 0x50) = (uintptr_t)ret0; // PopLocalFrame
    *(uintptr_t *)(fake_env + 0x7C) = (uintptr_t)GetObjectClass;
    *(uintptr_t *)(fake_env + 0x84) = (uintptr_t)GetMethodID;
    *(uintptr_t *)(fake_env + 0x8C) = (uintptr_t)CallObjectMethodV;
    *(uintptr_t *)(fake_env + 0x98) = (uintptr_t)CallBooleanMethodV;
    *(uintptr_t *)(fake_env + 0xD4) = (uintptr_t)CallLongMethodV;
    *(uintptr_t *)(fake_env + 0xF8) = (uintptr_t)CallVoidMethodV;
    *(uintptr_t *)(fake_env + 0x178) = (uintptr_t)GetFieldID;
    *(uintptr_t *)(fake_env + 0x17C) = (uintptr_t)GetBooleanField;
    *(uintptr_t *)(fake_env + 0x190) = (uintptr_t)GetIntField;
    *(uintptr_t *)(fake_env + 0x1C4) = (uintptr_t)GetStaticMethodID;
    *(uintptr_t *)(fake_env + 0x1c8) = (uintptr_t)CallStaticObjectMethod;
    *(uintptr_t *)(fake_env + 0x1cc) = (uintptr_t)CallStaticObjectMethodV;
    *(uintptr_t *)(fake_env + 0x1d0) = (uintptr_t)CallStaticObjectMethodA;
    *(uintptr_t *)(fake_env + 0x1D8) = (uintptr_t)CallStaticBooleanMethodV;
    *(uintptr_t *)(fake_env + 0x238) = (uintptr_t)CallStaticVoidMethodV;
    *(uintptr_t *)(fake_env + 0x29C) = (uintptr_t)NewStringUTF;
    *(uintptr_t *)(fake_env + 0x2A4) = (uintptr_t)GetStringUTFChars;
    *(uintptr_t *)(fake_env + 0x2A8) = (uintptr_t)ret0;
    *(uintptr_t *)(fake_env + 0x35c) = (uintptr_t)RegisterNatives;
    *(uintptr_t *)(fake_env + 0x36C) = (uintptr_t)GetJavaVM;
}
