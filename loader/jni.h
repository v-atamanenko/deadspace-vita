#ifndef SOLOADER_JNI_H
#define SOLOADER_JNI_H
/*
 * jni.h
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

#include "config.h"
#include "utils/utils.h"

extern char fake_vm[];
extern char fake_env[];

typedef enum MethodIDs {
    UNKNOWN = 0,
    INIT,
    GET_APK_PATH,
    GET_EXTERNAL_STORAGE_PATH,
    GET_INTERNAL_STORAGE_PATH,
    PLAY_SOUND,
    LOAD_SOUND,
    STOP_SOUND,
    STOP_ALL_SOUNDS,
    UNLOAD_SOUND,
    IS_PLAYING,
    IS_VBO_SUPPORTED,
    GET_LANGUAGE,
    GET_CONTEXT,
    GET_ASSETS
} MethodIDs;

typedef struct {
    char *name;
    enum MethodIDs id;
} NameToMethodID;

extern int GetEnv(void *vm, void **env, int r2);

void init_jni();

#endif // SOLOADER_JNI_H
