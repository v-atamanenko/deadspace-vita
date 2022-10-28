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
#include "android/jni.h"

extern JavaVM jvm;
extern JNIEnv jni;

typedef struct FakeJavaClass {
    const char* name;
} FakeJavaClass;

extern jint GetEnv(JavaVM *vm, void **env, jint r2);

void jni_init();


/// DYNAMICALLY ALLOCATED ARRAYS

typedef struct {
    int id;
    const void* arr;
    jsize length;
    uint8_t freed;
} DynamicallyAllocatedArrays;

jboolean tryFreeDynamicallyAllocatedArray(const void * arr);
void saveDynamicallyAllocatedArrayPointer(const void * arr, jsize sz);
jsize* findDynamicallyAllocatedArrayLength(const void* arr);

#endif // SOLOADER_JNI_H
