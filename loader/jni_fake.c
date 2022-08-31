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
 *
 * Contains parts of Dalvik implementation of JNI interfaces,
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "jni_fake.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <psp2/apputil.h>
#include <psp2/audioout.h>
#include <sys/unistd.h>
#include <psp2/kernel/threadmgr.h>
//#include "android/AAssetManager_acquirer.h"

#include "jni_specific.h"

#pragma ide diagnostic ignored "UnusedParameter"

struct JNIInvokeInterface *_jvm; // NOLINT(bugprone-reserved-identifier)
struct JNINativeInterface *_jni; // NOLINT(bugprone-reserved-identifier)

JavaVM jvm;
JNIEnv jni;

// VM functions:

jint GetEnv(JavaVM *vm, void **env, jint version) {
    //debugPrintf("[JVM] GetEnv(vm, **env, version:%i)\n", version);
    *env = &jni;
    return JNI_OK;
}

// ENV functions:



jclass FindClass (JNIEnv* env, const char* name) {
    debugPrintf("[JNI] FindClass(%s): ", name);

    // While we don't manipulate any actual classes here, the following code
    // serves two purposes:
    //   1) Being able to uniquely identify constructor methods for classes
    //      ("<init>"), since in GetMethodID we only receive class pointer.
    //   2) Providing a valid pointer to a valid object so that it behaves
    //      normally in memory and can be freed.

    FakeJavaClass* clazz = malloc(sizeof(FakeJavaClass));
    clazz->name = strdup(name);

    debugPrintf("0x%x\n", (int)clazz);

    return (jclass)clazz;
}

jobject NewGlobalRef(JNIEnv* env, jobject obj) {
    debugPrintf("[JNI] NewGlobalRef(env, 0x%x)\n", (int)obj);

    // As far as I understand, the concept of global/local references really
    // makes sense only on a real JVM. Here, since we basically operate with
    // shared global pointers everywhere, it should be safe to just return
    // obj as a "Global Ref"

    return obj;
}

int dynamicallyAllocatedArrays_length = 0;
DynamicallyAllocatedArrays* dynamicallyAllocatedArrays = NULL;
pthread_mutex_t dynamicallyAllocatedArrays_mutex;


jsize* findDynamicallyAllocatedArrayLength(const void* arr) {
    debugPrintf("[JNI] findDynamicallyAllocatedArrayLength(0x%x)\n", (int)arr);

    jsize* ret = NULL;
    pthread_mutex_lock(&dynamicallyAllocatedArrays_mutex);

    if (dynamicallyAllocatedArrays_length > 0) {
        for (int i = 0; i < dynamicallyAllocatedArrays_length; i++) {
            if (dynamicallyAllocatedArrays[i].arr == arr) {
                debugPrintf("found array: id #%i\n", dynamicallyAllocatedArrays[i].id);
                ret = &dynamicallyAllocatedArrays[i].length;
                break;
            }
        }
    }
    pthread_mutex_unlock(&dynamicallyAllocatedArrays_mutex);
    if (!ret) { debugPrintf("not found dynalloc array\n"); }
    return ret;
}

void saveDynamicallyAllocatedArrayPointer(const void * arr, jsize sz) {
    debugPrintf("[JNI] saveDynamicallyAllocatedArrayPointer(0x%x, %i)\n", (int)arr, sz);
    debugPrintf("AB1\n");
    pthread_mutex_lock(&dynamicallyAllocatedArrays_mutex);
    debugPrintf("AB2\n");
    if (dynamicallyAllocatedArrays_length == 0) {
        debugPrintf("AB3\n");
        dynamicallyAllocatedArrays = malloc(sizeof(DynamicallyAllocatedArrays));
        debugPrintf("AB4\n");
    } else {
        debugPrintf("AB5\n");
        dynamicallyAllocatedArrays = realloc(dynamicallyAllocatedArrays, (sizeof(DynamicallyAllocatedArrays) * (dynamicallyAllocatedArrays_length+1)));
        debugPrintf("AB6\n");
    }
    debugPrintf("AB7\n");
    dynamicallyAllocatedArrays[dynamicallyAllocatedArrays_length].length = sz;
    dynamicallyAllocatedArrays[dynamicallyAllocatedArrays_length].arr = arr;
    dynamicallyAllocatedArrays[dynamicallyAllocatedArrays_length].id = dynamicallyAllocatedArrays_length;
    dynamicallyAllocatedArrays[dynamicallyAllocatedArrays_length].freed = 0;
    debugPrintf("AB8\n");
    dynamicallyAllocatedArrays_length++;
    debugPrintf("AB9\n");
    pthread_mutex_unlock(&dynamicallyAllocatedArrays_mutex);
    debugPrintf("AB10\n");
}

jboolean tryFreeDynamicallyAllocatedArray(const void * arr) {
    debugPrintf("[JNI] tryFreeDynamicallyAllocatedArray(0x%x)\n", (int)arr);

    pthread_mutex_lock(&dynamicallyAllocatedArrays_mutex);

    if (dynamicallyAllocatedArrays_length <= 0) {
        pthread_mutex_unlock(&dynamicallyAllocatedArrays_mutex);
        return JNI_FALSE;
    }

    for (int i = 0; i < dynamicallyAllocatedArrays_length; i++) {
        if (dynamicallyAllocatedArrays[i].arr == arr) {
            if (dynamicallyAllocatedArrays[i].freed == 1 || dynamicallyAllocatedArrays[i].arr == NULL) {
                pthread_mutex_unlock(&dynamicallyAllocatedArrays_mutex);
                return JNI_TRUE;
            }
            free((void*)dynamicallyAllocatedArrays[i].arr);
            dynamicallyAllocatedArrays[i].freed = 1;
            pthread_mutex_unlock(&dynamicallyAllocatedArrays_mutex);
            return JNI_TRUE;
        }
    }

    pthread_mutex_unlock(&dynamicallyAllocatedArrays_mutex);
    return JNI_FALSE;
}

void DeleteGlobalRef(JNIEnv* env, jobject obj) {
    debugPrintf("[JNI] DeleteGlobalRef(env, 0x%x): ", (int)obj);

    if (tryFreeDynamicallyAllocatedArray(obj) == JNI_FALSE) {
        if (obj) free(obj);
    }

    debugPrintf("success.\n");
}

jobject NewObject(JNIEnv* env, jclass clazz, jmethodID _methodID, ...) {
    debugPrintf("[JNI] NewObject(env, 0x%x, %i): ", clazz, _methodID);

    jobject ret;
    va_list args;
    va_start(args, _methodID);
    ret = methodObjectCall((int)_methodID, args);
    va_end(args);

    return ret;
}

jobject NewObjectV(JNIEnv* env, jclass clazz, jmethodID _methodID, va_list args) {
    debugPrintf("[JNI] NewObjectV(env, 0x%x, %i): ", clazz, _methodID);
    return methodObjectCall((int)_methodID, args);
}

jobject NewObjectA(JNIEnv *env, jclass clazz, jmethodID _methodID, const jvalue *args) {
    debugPrintf("[JNI] NewObjectA(env, 0x%x, %i): not implemented\n", (int)clazz, _methodID);
    return NULL;
}

jclass GetObjectClass(JNIEnv* env, jobject obj) {
    debugPrintf("[JNI] GetObjectClass(0x%x)\n", (int)obj);
    // Due to the way we implement class methods, it's safe to not waste
    // resources on mapping objects to classess and return just a constant val.
    return (void *)0x44444444;
}

jmethodID GetMethodID(JNIEnv* env, jclass clazz, const char* name, const char* sig) {
    debugPrintf("[JNI] GetMethodID(env, 0x%x, \"%s\", \"%s\"): ", (int)clazz, name, sig);

    if (strcmp("<init>", name) == 0) {
        // Class constructor. Get class name and prepend it to `name`
        FakeJavaClass * clazz_fake = (FakeJavaClass *) clazz;
        if (!clazz_fake) {
            debugPrintf("Error! Fake class is not initialized.\n");
            return NULL;
        }
        debugPrintf("detected class constructor \"%s\" : ", clazz_fake->name);

        char* name_new = malloc(256);
        snprintf(name_new, 256, "%s/%s", clazz_fake->name, name);

        return (jmethodID)getMethodIdByName(name_new);
    }

    return (jmethodID)getMethodIdByName(name);
}

jmethodID GetStaticMethodID(JNIEnv* env, jclass clazz, const char* name, const char* sig) {
    debugPrintf("[JNI] GetStaticMethodID(env, 0x%x, \"%s\", \"%s\"): ", (int)clazz, name, sig);
    return (jmethodID)getMethodIdByName(name);
}

jobject CallObjectMethod(JNIEnv* env, jobject clazz, jmethodID id, ...) {
    debugPrintf("[JNI] CallObjectMethod(env, 0x%x, %i): ", (int)clazz, id);

    jobject ret;
    va_list args;
    va_start(args, id);
    ret = methodObjectCall((int)id, args);
    va_end(args);
    return ret;
}


void CallVoidMethodV(JNIEnv* env, jobject clazz, jmethodID _methodID, va_list args) {
    debugPrintf("[JNI] CallVoidMethodV(env, 0x%x, %i, args): ", (int)clazz, (int)_methodID);
    methodVoidCall((int)_methodID, args);
}

jobject CallObjectMethodV(JNIEnv* env, jobject obj, jmethodID _methodID, va_list args) {
    debugPrintf("[JNI] CallObjectMethodV(env, 0x%x, %i, args): ", (int)obj, (int)_methodID);
    return methodObjectCall((int)_methodID, args);
}

jboolean CallBooleanMethodV(JNIEnv* env, jobject obj, jmethodID _methodID, va_list args) {
    debugPrintf("[JNI] CallStaticBooleanMethodV(env, 0x%x, %i, args): ", (int)obj, (int)_methodID);
    return methodBooleanCall((int)_methodID, args);
}

jboolean CallStaticBooleanMethod(JNIEnv* env, jclass clazz, jmethodID _methodID, ...) {
    debugPrintf("[JNI] CallStaticBooleanMethod(env, 0x%x, %i, ...): ", (int)clazz, (int)_methodID);

    jboolean ret;
    va_list args;
    va_start(args, _methodID);
    ret = methodBooleanCall((int)_methodID, args);
    va_end(args);

    return ret;
}

jboolean CallStaticBooleanMethodV(JNIEnv* env, jclass clazz, jmethodID _methodID, va_list args) {
    debugPrintf("[JNI] CallStaticBooleanMethodV(env, 0x%x, %i, args): ", (int)clazz, (int)_methodID);
    return methodBooleanCall((int)_methodID, args);
}

jboolean CallStaticBooleanMethodA(JNIEnv* p1, jclass p2, jmethodID p3, const jvalue* p4) {
    debugPrintf("[JNI] CallStaticBooleanMethodA(): not implemented\n");
    return 0;
}

jlong CallLongMethodV(JNIEnv* env, jobject obj, jmethodID _methodID, va_list args) {
    debugPrintf("[JNI] CallLongMethodV(env, 0x%x, %i, args): ", (int)obj, (int)_methodID);
    return methodLongCall((int)_methodID, args);
}

jlong CallLongMethod(JNIEnv* env, jobject obj, jmethodID _methodID, ...) {
    debugPrintf("[JNI] CallLongMethod(env, 0x%x, %i, ...): ", (int)obj, (int)_methodID);

    jlong ret;
    va_list args;
    va_start(args, _methodID);
    ret = methodLongCall((int)_methodID, args);
    va_end(args);

    return ret;
}

jlong CallLongMethodA(JNIEnv* env, jobject obj, jmethodID _methodID, const jvalue* p4) {
    debugPrintf("[JNI] CallLongMethodA(): not implemented\n");
    return 0;
}

jlong CallStaticLongMethod(JNIEnv* env, jclass clazz, jmethodID _methodID, ...) {
    debugPrintf("[JNI] CallStaticLongMethod(env, 0x%x, %i, ...): ", (int)clazz, (int)_methodID);

    jlong ret;
    va_list args;
    va_start(args, _methodID);
    ret = methodLongCall((int)_methodID, args);
    va_end(args);

    return ret;
}

jlong CallStaticLongMethodV(JNIEnv* env, jclass clazz, jmethodID _methodID, va_list args) {
    debugPrintf("[JNI] CallStaticLongMethodV(env, 0x%x, %i, args): ", (int)clazz, (int)_methodID);
    return methodLongCall((int)_methodID, args);
}

jlong CallStaticLongMethodA(JNIEnv* env, jclass clazz, jmethodID _methodID, const jvalue* p4) {
    debugPrintf("[JNI] CallStaticLongMethodA(): not implemented\n");
    return 0;
}

jint CallIntMethod(JNIEnv* env, jobject obj, jmethodID _methodID, ...) {
    debugPrintf("[JNI] CallIntMethod(env, 0x%x, %i, ...): ", (int)obj, (int)_methodID);

    jint ret;
    va_list args;
    va_start(args, _methodID);
    ret = methodIntCall((int)_methodID, args);
    va_end(args);

    return ret;
}
jint CallIntMethodV(JNIEnv* env, jobject obj, jmethodID _methodID, va_list args) {
    //debugPrintf("[JNI] CallIntMethodV(env, 0x%x, %i, args): ", (int)obj, (int)_methodID);
    return methodIntCall((int)_methodID, args);
}
jint CallIntMethodA(JNIEnv* env, jobject obj, jmethodID _methodID, const jvalue* p4) {
    debugPrintf("[JNI] CallIntMethodA(): not implemented\n"); return 0;
}
jint CallStaticIntMethod(JNIEnv* env, jclass clazz, jmethodID _methodID, ...) {
    debugPrintf("[JNI] CallStaticIntMethod(env, 0x%x, %i, ...): ", (int)clazz, (int)_methodID);

    jint ret;
    va_list args;
    va_start(args, _methodID);
    ret = methodIntCall((int)_methodID, args);
    va_end(args);

    return ret;
}
jint CallStaticIntMethodV(JNIEnv* env, jclass clazz, jmethodID _methodID, va_list args) {
    debugPrintf("[JNI] CallStaticIntMethodV(env, 0x%x, %i, args): ", (int)clazz, (int)_methodID);
    return methodIntCall((int)_methodID, args);
}
jint CallStaticIntMethodA(JNIEnv* env, jclass clazz, jmethodID _methodID, const jvalue* p4) {
    debugPrintf("[JNI] CallStaticIntMethodA(): not implemented\n"); return 0;
}

jfloat CallFloatMethod(JNIEnv* env, jobject obj, jmethodID _methodID, ...) {
    debugPrintf("[JNI] CallFloatMethod(env, 0x%x, %i, ...): ", (int)obj, (int)_methodID);

    jfloat ret;
    va_list args;
    va_start(args, _methodID);
    ret = methodFloatCall((int)_methodID, args);
    va_end(args);

    return ret;
}
jfloat CallFloatMethodV(JNIEnv* env, jobject obj, jmethodID _methodID, va_list args) {
    debugPrintf("[JNI] CallFloatMethodV(env, 0x%x, %i, args): ", (int)obj, (int)_methodID);
    return methodFloatCall((int)_methodID, args);
}
jfloat CallFloatMethodA(JNIEnv* env, jobject obj, jmethodID _methodID, const jvalue* p4) {
    debugPrintf("[JNI] CallFloatMethodA(): not implemented\n"); return 0;
}
jfloat CallStaticFloatMethod(JNIEnv* env, jclass clazz, jmethodID _methodID, ...) {
    debugPrintf("[JNI] CallStaticFloatMethod(env, 0x%x, %i, ...): ", (int)clazz, (int)_methodID);

    jfloat ret;
    va_list args;
    va_start(args, _methodID);
    ret = methodFloatCall((int)_methodID, args);
    va_end(args);

    return ret;
}
jfloat CallStaticFloatMethodV(JNIEnv* env, jclass clazz, jmethodID _methodID, va_list args) {
    debugPrintf("[JNI] CallStaticFloatMethodV(env, 0x%x, %i, args): ", (int)clazz, (int)_methodID);
    return methodFloatCall((int)_methodID, args);
}
jfloat CallStaticFloatMethodA(JNIEnv* env, jclass obj, jmethodID _methodID, const jvalue* p4) {
    debugPrintf("[JNI] CallStaticFloatMethodA(): not implemented\n"); return 0;
}




void CallStaticVoidMethodV(JNIEnv* env, jclass clazz, jmethodID _methodID, va_list args) {
    debugPrintf("[JNI] CallStaticVoidMethodV(env, 0x%x, %i, args): ", (int)clazz, (int)_methodID);
    methodVoidCall((int)_methodID, args);
}

jobject CallStaticObjectMethodV(JNIEnv* env, jclass clazz, jmethodID _methodID, va_list args) {
    debugPrintf("[JNI] CallStaticObjectMethodV(env, 0x%x, %i, args): ", (int)clazz, (int)_methodID);
    return methodObjectCall((int)_methodID, args);
}

jstring NewStringUTF(JNIEnv* env, const char* bytes) {
    debugPrintf("[JNI] NewStringUTF(env, \"%s\")\n", bytes);

    char* newStr;
    if (bytes == NULL) {
        /* this shouldn't happen; throw NPE? */
        newStr = NULL;
    } else {
        newStr = strdup(bytes);
        if (newStr == NULL) {
            /* assume memory failure */
            fprintf(stderr, "[JNI][NewStringUTF] native heap string alloc failed! aborting.\n");
            abort();
        }
    }

    return newStr;
}

jstring NewString(JNIEnv* env, const jchar* chars, jsize char_count) {
    debugPrintf("[JNI] NewString(env, \"%s\", %i)\n", (const char*)chars, char_count);

    if (char_count < 0) {
        fprintf(stderr, "[JNI][NewString] char_count < 0: %d! aborting.\n", char_count);
        abort();
        return NULL;
    }
    if (chars == NULL && char_count > 0) {
        fprintf(stderr, "[JNI][NewString] chars == null && char_count > 0");
        abort();
        return NULL;
    }

    char* newStr = malloc(char_count+1);
    strncpy(newStr, (const char*)chars, char_count+1);
    return newStr;
}

jbyteArray NewByteArray(JNIEnv* env, jsize length) {
    debugPrintf("[JNI] NewByteArray(env, size:%i)\n", length);
    char* ret = malloc(length);
    return ret;
}

//void** alloced_by_jni_fake;
//int alloced_by_jni_fake = 0;

const char* GetStringUTFChars(JNIEnv* env, jstring string, jboolean* isCopy) {
    debugPrintf("[JNI] GetStringUTFChars(env, \"%s\", *isCopy)\n", string);

    char* newStr;
    if (string == NULL) {
        /* this shouldn't happen; throw NPE? */
        newStr = NULL;
    } else {
        if (isCopy != NULL)
            *isCopy = JNI_TRUE;
        newStr = strdup(string);
        if (newStr == NULL) {
            /* assume memory failure */
            fprintf(stderr, "[JNI][GetStringUTFChars] native heap string alloc failed! aborting.\n");
            abort();
        }
    }

    return newStr;
}

jsize GetStringUTFLength(JNIEnv* env, jstring string) {
    debugPrintf("[JNI] GetStringUTFLength(env, \"%s\")\n", string);

    if (string != NULL) {
        return (jsize)strlen(string);
    }

    return 0;
}

void ReleaseStringUTFChars(JNIEnv* env, jstring string, char* chars) {
    debugPrintf("[JNI] ReleaseStringUTFChars(env, 0x%x, \"%s\")\n", (int)string, chars);
    if (chars) {
        free(chars);
    }
}

void DeleteLocalRef(JNIEnv* env, jobject obj) {
    debugPrintf("[JNI] DeleteLocalRef(env, 0x%x)\n", (int)obj);
    if (obj) {
        //free(obj);
    }
}

jint GetJavaVM(JNIEnv* env, JavaVM** vm) {
    debugPrintf("[JNI] GetJavaVM(env, *vm)\n");
    *vm = &jvm;
    return 0;
}

jlong GetLongField(JNIEnv* p1, jobject obj, jfieldID fieldID) {
    debugPrintf("[JNI] GetLongField(env, 0x%x, 0x%x): not implemented\n", (int)obj, (int)fieldID);
    return (jlong)NULL;
}

jint GetVersion (JNIEnv * env) {
    debugPrintf("[JNI] GetVersion(env)\n");
    return JNI_VERSION_1_6;
}

jclass DefineClass (JNIEnv* env, const char* name, jobject obj, const jbyte* byt, jsize sz) {
    debugPrintf("[JNI] DefineClass(env, \"%s\", 0x%x, 0x%x, %i): not implemented\n", name, obj, byt, sz);
    return 0;
}

jthrowable ExceptionOccurred(JNIEnv* env) {
    debugPrintf("[JNI] ExceptionOccurred(env)\n");
    // We never have exceptions.
    return NULL;
}

void ExceptionDescribe(JNIEnv* env) {
    debugPrintf("[JNI] ExceptionDescribe(env)\n");
    // We never have exceptions.
}

void ExceptionClear(JNIEnv* env) {
    debugPrintf("[JNI] ExceptionClear(env)\n");
    // We never have exceptions.
}

jsize GetArrayLength(JNIEnv* env, jarray array) {
    //TODO: Other array types
    debugPrintf("[JNI] GetArrayLength(env, 0x%x)\n", (int)array);
    jsize* ret;

    ret = fieldIntArrayGetLengthByPtr(array);
    if (ret) return *ret;

    ret = findDynamicallyAllocatedArrayLength(array);
    if (ret) return * ret;

    debugPrintf("[JNI] GetArrayLength(env, 0x%x): Not Found. Unknown array type?\n", (int)array);
    return 0;
}

jobject GetObjectArrayElement(JNIEnv* env, jobjectArray java_array, jsize index) {
    debugPrintf("[JNI] GetObjectArrayElement(env, 0x%x, idx:%i)\n", java_array, index);
    jobject* arr = (jobject*)java_array;
    return arr[index];
}


void GetIntArrayRegion(JNIEnv* env, jintArray array, jsize start, jsize length, jint* buffer) {
    debugPrintf("[JNI] GetIntArrayRegion(env, 0x%x, %i, %i, buffer)\n", (int)array, start, length);

    if (!buffer)
        buffer = (jint*) malloc(length);

    memcpy(buffer, (jint*)array + start, length);
}

void GetFloatArrayRegion(JNIEnv* env, jfloatArray array, jsize start, jsize length, jfloat* buffer) {
    debugPrintf("[JNI] GetFloatArrayRegion(env, 0x%x, %i, %i, buffer)\n", (int)array, start, length);

    if (!buffer)
        buffer = (jfloat*) malloc(length);

    memcpy(buffer, (jfloat*)array + start, length);
}

void GetByteArrayRegion(JNIEnv* env, jbyteArray array, jsize start, jsize length, jbyte* buffer) {
    debugPrintf("[JNI] GetByteArrayRegion(env, 0x%x, %i, %i, buffer)\n", (int)array, start, length);

    if (!buffer)
        buffer = (jbyte*) malloc(length);

    memcpy(buffer, (jbyte*)array + start, length);
}

jint PushLocalFrame(JNIEnv* env, jint capacity) {
    debugPrintf("[JNI] PushLocalFrame(env, %i)\n", capacity);
    // Since we operate with global pointers everywhere, no need to scope them.
    return 0;
}

jobject PopLocalFrame(JNIEnv* env, jobject result) {
    debugPrintf("[JNI] PopLocalFrame(env, 0x%x)\n", (int)result);
    // Since we operate with global pointers everywhere, no need to scope them.
    return NULL;
}

jfieldID GetFieldID(JNIEnv * env, jclass clazz, const char* name, const char* t) {
    debugPrintf("[JNI] GetFieldID(env, 0x%x, \"%s\", \"%s\"): ", (int)clazz, name, t);
    return (jfieldID)getFieldIdByName(name);
}

jfieldID GetStaticFieldID(JNIEnv* env, jclass clazz, const char* name, const char* t) {
    debugPrintf("[JNI] GetStaticFieldID(env, 0x%x, \"%s\", \"%s\"): ", (int)clazz, name, t);
    return (jfieldID)getFieldIdByName(name);
}

jobject GetStaticObjectField(JNIEnv* env, jclass clazz, jfieldID id) {
    debugPrintf("[JNI] GetStaticObjectField(env, 0x%x, %i): ", (int)clazz, (int)id);
    return getObjectFieldValueById((int)id);
}

jobject GetObjectField(JNIEnv* env, jobject obj, jfieldID id) {
    debugPrintf("[JNI] GetObjectField(env, 0x%x, %i): ", (int)obj, (int)id);
    return getObjectFieldValueById((int)id);
}

jint GetIntField(JNIEnv* env, jobject obj, jfieldID id) {
    debugPrintf("[JNI] GetIntField(env, 0x%x, %i): ", (int)obj, (int)id);
    return getIntFieldValueById((int)id);
}

jboolean GetBooleanField(JNIEnv* env, jobject obj, jfieldID id) {
    debugPrintf("[JNI] GetBooleanField(env, 0x%x, %i): ", (int)obj, (int)id);
    return getBooleanFieldValueById((int)id);
}

jint AttachCurrentThread(JavaVM* vm, JNIEnv **p_env, void *thr_args) {
    debugPrintf("[JVM] AttachCurrentThread(vm, *p_env, *thr_args)\n");
    *p_env = &jni;
    return 0;
}

jsize GetStringLength(JNIEnv *env, jstring string) {
    debugPrintf("[JNI] GetStringLength(env, 0x%x/\"%s\")\n", string, string);
    return (jsize)strlen(string);
}

const jchar * GetStringChars(JNIEnv *env, jstring string, jboolean *isCopy) {
    debugPrintf("[JNI] GetStringChars(env, 0x%x/\"%s\", *isCopy)\n", string, string);

    if (!string) {
        fprintf(stderr, "[JNI][GetStringChars] Error: string is null.\n");
        return NULL;
    }

    if (isCopy != NULL) {
        *isCopy = JNI_TRUE;
    }

    return (const jchar *)strdup((const char*)string);
}

void ReleaseStringChars(JNIEnv *env, jstring string, const jchar *chars) {
    debugPrintf("[JNI] ReleaseStringChars(env, 0x%x/\"%s\", 0x%x)\n", string, string, chars);

    if (!chars) {
        fprintf(stderr, "[JNI][GetStringChars] Error: chars is null.\n");
        return;
    }

    free((char*)chars);
}

jint MonitorEnter(JNIEnv* p1, jobject p2) {
    debugPrintf("[JNI] MonitorEnter(env, 0x%x): ignored\n", (int)p2);
    return 0;
}

jint MonitorExit(JNIEnv* p1, jobject p2) {
    debugPrintf("[JNI] MonitorExit(env, 0x%x): ignored\n", (int)p2);
    return 0;
}



jbyte        GetByteField(JNIEnv* p1, jobject p2, jfieldID p3) { debugPrintf("[JNI] GetByteField(): not implemented\n"); return 0; }
jchar        GetCharField(JNIEnv* p1, jobject p2, jfieldID p3) { debugPrintf("[JNI] GetCharField(): not implemented\n"); return 0; }
jshort       GetShortField(JNIEnv* p1, jobject p2, jfieldID p3) { debugPrintf("[JNI] GetShortField(): not implemented\n"); return 0; }
jfloat       GetFloatField(JNIEnv* p1, jobject p2, jfieldID p3) { debugPrintf("[JNI] GetFloatField(): not implemented\n"); return 0; }
jdouble      GetDoubleField(JNIEnv* p1, jobject p2, jfieldID p3) { debugPrintf("[JNI] GetDoubleField(): not implemented\n"); return 0; }
void         SetObjectField(JNIEnv* p1, jobject p2, jfieldID p3, jobject p4) { debugPrintf("[JNI] SetObjectField(): not implemented\n"); }
void         SetBooleanField(JNIEnv* p1, jobject p2, jfieldID p3, jboolean p4) { debugPrintf("[JNI] SetBooleanField(): not implemented\n"); }
void         SetByteField(JNIEnv* p1, jobject p2, jfieldID p3, jbyte p4) { debugPrintf("[JNI] SetByteField(): not implemented\n"); }
void         SetCharField(JNIEnv* p1, jobject p2, jfieldID p3, jchar p4) { debugPrintf("[JNI] SetCharField(): not implemented\n"); }
void         SetShortField(JNIEnv* p1, jobject p2, jfieldID p3, jshort p4) { debugPrintf("[JNI] SetShortField(): not implemented\n"); }
void         SetIntField(JNIEnv* p1, jobject p2, jfieldID p3, jint p4) { debugPrintf("[JNI] SetIntField(): not implemented\n"); }
void         SetLongField(JNIEnv* p1, jobject p2, jfieldID p3, jlong p4) { debugPrintf("[JNI] SetLongField(): not implemented\n"); }
void         SetFloatField(JNIEnv* p1, jobject p2, jfieldID p3, jfloat p4) { debugPrintf("[JNI] SetFloatField(): not implemented\n"); }
void         SetDoubleField(JNIEnv* p1, jobject p2, jfieldID p3, jdouble p4) { debugPrintf("[JNI] SetDoubleField(): not implemented\n"); }
jobject      CallStaticObjectMethod(JNIEnv* p1, jclass p2, jmethodID p4, ...) { debugPrintf("[JNI] CallStaticObjectMethod(): not implemented\n"); return 0; }
jobject      CallStaticObjectMethodA(JNIEnv* p1, jclass p2, jmethodID p3, const jvalue* p4) { debugPrintf("[JNI] CallStaticObjectMethodA(): not implemented\n"); return 0; }
jboolean     IsSameObject(JNIEnv* p1, jobject p2, jobject p3) { debugPrintf("[JNI] IsSameObject(): not implemented\n"); return 0; }
jobject      NewLocalRef(JNIEnv* p1, jobject p2) { debugPrintf("[JNI] NewLocalRef(): not implemented\n"); return 0; }
jint         EnsureLocalCapacity(JNIEnv* p1, jint p2) { debugPrintf("[JNI] EnsureLocalCapacity(): not implemented\n"); return 0; }
jobject      AllocObject(JNIEnv* p1, jclass p2) { debugPrintf("[JNI] AllocObject(): not implemented\n"); return 0; }
jmethodID    FromReflectedMethod(JNIEnv* p1, jobject p2) { debugPrintf("[JNI] FromReflectedMethod(): not implemented\n"); return 0; }
jfieldID     FromReflectedField(JNIEnv* p1, jobject p2) { debugPrintf("[JNI] FromReflectedField(): not implemented\n"); return 0; }
jobject      ToReflectedMethod(JNIEnv* p1, jclass p2, jmethodID p3, jboolean p4) { debugPrintf("[JNI] ToReflectedMethod(): not implemented\n"); return 0; }
jclass       GetSuperclass(JNIEnv* p1, jclass p2) { debugPrintf("[JNI] GetSuperclass(): not implemented\n"); return 0; }
jboolean     IsAssignableFrom(JNIEnv* p1, jclass p2, jclass p3) { debugPrintf("[JNI] IsAssignableFrom(): not implemented\n"); return 0; }
jobject      ToReflectedField(JNIEnv* p1, jclass p2, jfieldID p3, jboolean p4) { debugPrintf("[JNI] ToReflectedField(): not implemented\n"); return 0; }
jint         Throw(JNIEnv* p1, jthrowable p2) { debugPrintf("[JNI] Throw(): not implemented\n"); return 0; }
jint         ThrowNew(JNIEnv * p1, jclass p2, const char * p3) { debugPrintf("[JNI] ThrowNew(): not implemented\n"); return 0; }
void         FatalError(JNIEnv* p1, const char* p2) { debugPrintf("[JNI] FatalError(): not implemented\n"); }
jboolean     IsInstanceOf(JNIEnv* p1, jobject p2, jclass p3) { debugPrintf("[JNI] IsInstanceOf(): not implemented\n"); return 0; }
jobject      CallObjectMethodA(JNIEnv* p1, jobject p2, jmethodID p3, const jvalue* p4) { debugPrintf("[JNI] CallObjectMethodA(): not implemented\n"); return 0; }
jboolean     CallBooleanMethod(JNIEnv* p1, jobject p2, jmethodID p3, ...) { debugPrintf("[JNI] CallBooleanMethod(): not implemented\n"); return 0; }
jboolean     CallBooleanMethodA(JNIEnv* p1, jobject p2, jmethodID p3, const jvalue* p4) { debugPrintf("[JNI] CallBooleanMethodA(): not implemented\n"); return 0; }
jbyte        CallByteMethod(JNIEnv* p1, jobject p2, jmethodID p3, ...) { debugPrintf("[JNI] CallByteMethod(): not implemented\n"); return 0; }
jbyte        CallByteMethodV(JNIEnv* p1, jobject p2, jmethodID p3, va_list p4) { debugPrintf("[JNI] CallByteMethodV(): not implemented\n"); return 0; }
jbyte        CallByteMethodA(JNIEnv* p1, jobject p2, jmethodID p3, const jvalue* p4) { debugPrintf("[JNI] CallByteMethodA(): not implemented\n"); return 0; }
jchar        CallCharMethod(JNIEnv* p1, jobject p2, jmethodID p3, ...) { debugPrintf("[JNI] CallCharMethod(): not implemented\n"); return 0; }
jchar        CallCharMethodV(JNIEnv* p1, jobject p2, jmethodID p3, va_list p4) { debugPrintf("[JNI] CallCharMethodV(): not implemented\n"); return 0; }
jchar        CallCharMethodA(JNIEnv* p1, jobject p2, jmethodID p3, const jvalue* p4) { debugPrintf("[JNI] CallCharMethodA(): not implemented\n"); return 0; }
jshort       CallShortMethod(JNIEnv* p1, jobject p2, jmethodID p3, ...) { debugPrintf("[JNI] CallShortMethod(): not implemented\n"); return 0; }
jshort       CallShortMethodV(JNIEnv* p1, jobject p2, jmethodID p3, va_list p4) { debugPrintf("[JNI] CallShortMethodV(): not implemented\n"); return 0; }
jshort       CallShortMethodA(JNIEnv* p1, jobject p2, jmethodID p3, const jvalue* p4) { debugPrintf("[JNI] CallShortMethodA(): not implemented\n"); return 0; }
jdouble      CallDoubleMethod(JNIEnv* p1, jobject p2, jmethodID p3, ...) { debugPrintf("[JNI] CallDoubleMethod(): not implemented\n"); return 0; }
jdouble      CallDoubleMethodV(JNIEnv* p1, jobject p2, jmethodID p3, va_list p4) { debugPrintf("[JNI] CallDoubleMethodV(): not implemented\n"); return 0; }
jdouble      CallDoubleMethodA(JNIEnv* p1, jobject p2, jmethodID p3, const jvalue* p4) { debugPrintf("[JNI] CallDoubleMethodA(): not implemented\n"); return 0; }
void         CallVoidMethod(JNIEnv* p1, jobject p2, jmethodID p3, ...) { debugPrintf("[JNI] CallVoidMethod(): not implemented\n"); }
void         CallVoidMethodA(JNIEnv* p1, jobject p2, jmethodID p3, const jvalue* p4) { debugPrintf("[JNI] CallVoidMethodA(): not implemented\n"); }
jobject      CallNonvirtualObjectMethod(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, ...) { debugPrintf("[JNI] CallNonvirtualObjectMethod(): not implemented\n"); return 0; }
jobject      CallNonvirtualObjectMethodV(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, va_list p5) { debugPrintf("[JNI] CallNonvirtualObjectMethodV(): not implemented\n"); return 0; }
jobject      CallNonvirtualObjectMethodA(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, const jvalue* p5) { debugPrintf("[JNI] CallNonvirtualObjectMethodA(): not implemented\n"); return 0; }
jboolean     CallNonvirtualBooleanMethod(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, ...) { debugPrintf("[JNI] CallNonvirtualBooleanMethod(): not implemented\n"); return 0; }
jboolean     CallNonvirtualBooleanMethodV(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, va_list p5) { debugPrintf("[JNI] CallNonvirtualBooleanMethodV(): not implemented\n"); return 0; }
jboolean     CallNonvirtualBooleanMethodA(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, const jvalue* p5) { debugPrintf("[JNI] CallNonvirtualBooleanMethodA(): not implemented\n"); return 0; }
jbyte        CallNonvirtualByteMethod(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, ...) { debugPrintf("[JNI] CallNonvirtualByteMethod(): not implemented\n"); return 0; }
jbyte        CallNonvirtualByteMethodV(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, va_list p5) { debugPrintf("[JNI] CallNonvirtualByteMethodV(): not implemented\n"); return 0; }
jbyte        CallNonvirtualByteMethodA(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, const jvalue* p5) { debugPrintf("[JNI] CallNonvirtualByteMethodA(): not implemented\n"); return 0; }
jchar        CallNonvirtualCharMethod(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, ...) { debugPrintf("[JNI] CallNonvirtualCharMethod(): not implemented\n"); return 0; }
jchar        CallNonvirtualCharMethodV(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, va_list p5) { debugPrintf("[JNI] CallNonvirtualCharMethodV(): not implemented\n"); return 0; }
jchar        CallNonvirtualCharMethodA(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, const jvalue* p5) { debugPrintf("[JNI] CallNonvirtualCharMethodA(): not implemented\n"); return 0; }
jshort       CallNonvirtualShortMethod(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, ...) { debugPrintf("[JNI] CallNonvirtualShortMethod(): not implemented\n"); return 0; }
jshort       CallNonvirtualShortMethodV(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, va_list p5) { debugPrintf("[JNI] CallNonvirtualShortMethodV(): not implemented\n"); return 0; }
jshort       CallNonvirtualShortMethodA(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, const jvalue* p5) { debugPrintf("[JNI] CallNonvirtualShortMethodA(): not implemented\n"); return 0; }
jint         CallNonvirtualIntMethod(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, ...) { debugPrintf("[JNI] CallNonvirtualIntMethod(): not implemented\n"); return 0; }
jint         CallNonvirtualIntMethodV(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, va_list p5) { debugPrintf("[JNI] CallNonvirtualIntMethodV(): not implemented\n"); return 0; }
jint         CallNonvirtualIntMethodA(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, const jvalue* p5) { debugPrintf("[JNI] CallNonvirtualIntMethodA(): not implemented\n"); return 0; }
jlong        CallNonvirtualLongMethod(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, ...) { debugPrintf("[JNI] CallNonvirtualLongMethod(): not implemented\n"); return 0; }
jlong        CallNonvirtualLongMethodV(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, va_list p5) { debugPrintf("[JNI] CallNonvirtualLongMethodV(): not implemented\n"); return 0; }
jlong        CallNonvirtualLongMethodA(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, const jvalue* p5) { debugPrintf("[JNI] CallNonvirtualLongMethodA(): not implemented\n"); return 0; }
jfloat       CallNonvirtualFloatMethod(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, ...) { debugPrintf("[JNI] CallNonvirtualFloatMethod(): not implemented\n"); return 0; }
jfloat       CallNonvirtualFloatMethodV(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, va_list p5) { debugPrintf("[JNI] CallNonvirtualFloatMethodV(): not implemented\n"); return 0; }
jfloat       CallNonvirtualFloatMethodA(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, const jvalue* p5) { debugPrintf("[JNI] CallNonvirtualFloatMethodA(): not implemented\n"); return 0; }
jdouble      CallNonvirtualDoubleMethod(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, ...) { debugPrintf("[JNI] CallNonvirtualDoubleMethod(): not implemented\n"); return 0; }
jdouble      CallNonvirtualDoubleMethodV(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, va_list p5) { debugPrintf("[JNI] CallNonvirtualDoubleMethodV(): not implemented\n"); return 0; }
jdouble      CallNonvirtualDoubleMethodA(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, const jvalue* p5) { debugPrintf("[JNI] CallNonvirtualDoubleMethodA(): not implemented\n"); return 0; }
void         CallNonvirtualVoidMethod(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, ...) { debugPrintf("[JNI] CallNonvirtualVoidMethod(): not implemented\n"); }
void         CallNonvirtualVoidMethodV(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, va_list p5) { debugPrintf("[JNI] CallNonvirtualVoidMethodV(): not implemented\n"); }
void         CallNonvirtualVoidMethodA(JNIEnv* p1, jobject p2, jclass p3, jmethodID p4, const jvalue* p5) { debugPrintf("[JNI] CallNonvirtualVoidMethodA(): not implemented\n"); }
jbyte        CallStaticByteMethod(JNIEnv* p1, jclass p2, jmethodID p3, ...) { debugPrintf("[JNI] CallStaticByteMethod(): not implemented\n"); return 0; }
jbyte        CallStaticByteMethodV(JNIEnv* p1, jclass p2, jmethodID p3, va_list p4) { debugPrintf("[JNI] CallStaticByteMethodV(): not implemented\n"); return 0; }
jbyte        CallStaticByteMethodA(JNIEnv* p1, jclass p2, jmethodID p3, const jvalue* p4) { debugPrintf("[JNI] CallStaticByteMethodA(): not implemented\n"); return 0; }
jchar        CallStaticCharMethod(JNIEnv* p1, jclass p2, jmethodID p3, ...) { debugPrintf("[JNI] CallStaticCharMethod(): not implemented\n"); return 0; }
jchar        CallStaticCharMethodV(JNIEnv* p1, jclass p2, jmethodID p3, va_list p4) { debugPrintf("[JNI] CallStaticCharMethodV(): not implemented\n"); return 0; }
jchar        CallStaticCharMethodA(JNIEnv* p1, jclass p2, jmethodID p3, const jvalue* p4) { debugPrintf("[JNI] CallStaticCharMethodA(): not implemented\n"); return 0; }
jshort       CallStaticShortMethod(JNIEnv* p1, jclass p2, jmethodID p3, ...) { debugPrintf("[JNI] CallStaticShortMethod(): not implemented\n"); return 0; }
jshort       CallStaticShortMethodV(JNIEnv* p1, jclass p2, jmethodID p3, va_list p4) { debugPrintf("[JNI] CallStaticShortMethodV(): not implemented\n"); return 0; }
jshort       CallStaticShortMethodA(JNIEnv* p1, jclass p2, jmethodID p3, const jvalue* p4) { debugPrintf("[JNI] CallStaticShortMethodA(): not implemented\n"); return 0; }
jdouble      CallStaticDoubleMethod(JNIEnv* p1, jclass p2, jmethodID p3, ...) { debugPrintf("[JNI] CallStaticDoubleMethod(): not implemented\n"); return 0; }
jdouble      CallStaticDoubleMethodV(JNIEnv* p1, jclass p2, jmethodID p3, va_list p4) { debugPrintf("[JNI] CallStaticDoubleMethodV(): not implemented\n"); return 0; }
jdouble      CallStaticDoubleMethodA(JNIEnv* p1, jclass p2, jmethodID p3, const jvalue* p4) { debugPrintf("[JNI] CallStaticDoubleMethodA(): not implemented\n"); return 0; }
void         CallStaticVoidMethod(JNIEnv* p1, jclass p2, jmethodID p3, ...) { debugPrintf("[JNI] CallStaticVoidMethod(): not implemented\n"); }
void         CallStaticVoidMethodA(JNIEnv* p1, jclass p2, jmethodID p3, const jvalue* p4) { debugPrintf("[JNI] CallStaticVoidMethodA(): not implemented\n"); }
jboolean     GetStaticBooleanField(JNIEnv* p1, jclass p2, jfieldID p3) { debugPrintf("[JNI] GetStaticBooleanField(): not implemented\n"); return 0; }
jbyte        GetStaticByteField(JNIEnv* p1, jclass p2, jfieldID p3) { debugPrintf("[JNI] GetStaticByteField(): not implemented\n"); return 0; }
jchar        GetStaticCharField(JNIEnv* p1, jclass p2, jfieldID p3) { debugPrintf("[JNI] GetStaticCharField(): not implemented\n"); return 0; }
jshort       GetStaticShortField(JNIEnv* p1, jclass p2, jfieldID p3) { debugPrintf("[JNI] GetStaticShortField(): not implemented\n"); return 0; }
jint         GetStaticIntField(JNIEnv* p1, jclass p2, jfieldID p3) { debugPrintf("[JNI] GetStaticIntField(): not implemented\n"); return 0; }
jlong        GetStaticLongField(JNIEnv* p1, jclass p2, jfieldID p3) { debugPrintf("[JNI] GetStaticLongField(): not implemented\n"); return 0; }
jfloat       GetStaticFloatField(JNIEnv* p1, jclass p2, jfieldID p3) { debugPrintf("[JNI] GetStaticFloatField(): not implemented\n"); return 0; }
jdouble      GetStaticDoubleField(JNIEnv* p1, jclass p2, jfieldID p3) { debugPrintf("[JNI] GetStaticDoubleField(): not implemented\n"); return 0; }
void         SetStaticObjectField(JNIEnv* p1, jclass p2, jfieldID p3, jobject p4) { debugPrintf("[JNI] SetStaticObjectField(): not implemented\n"); }
void         SetStaticBooleanField(JNIEnv* p1, jclass p2, jfieldID p3, jboolean p4) { debugPrintf("[JNI] SetStaticBooleanField(): not implemented\n"); }
void         SetStaticByteField(JNIEnv* p1, jclass p2, jfieldID p3, jbyte p4) { debugPrintf("[JNI] SetStaticByteField(): not implemented\n"); }
void         SetStaticCharField(JNIEnv* p1, jclass p2, jfieldID p3, jchar p4) { debugPrintf("[JNI] SetStaticCharField(): not implemented\n"); }
void         SetStaticShortField(JNIEnv* p1, jclass p2, jfieldID p3, jshort p4) { debugPrintf("[JNI] SetStaticShortField(): not implemented\n"); }
void         SetStaticIntField(JNIEnv* p1, jclass p2, jfieldID p3, jint p4) { debugPrintf("[JNI] SetStaticIntField(): not implemented\n"); }
void         SetStaticLongField(JNIEnv* p1, jclass p2, jfieldID p3, jlong p4) { debugPrintf("[JNI] SetStaticLongField(): not implemented\n"); }
void         SetStaticFloatField(JNIEnv* p1, jclass p2, jfieldID p3, jfloat p4) { debugPrintf("[JNI] SetStaticFloatField(): not implemented\n"); }
void         SetStaticDoubleField(JNIEnv* p1, jclass p2, jfieldID p3, jdouble p4) { debugPrintf("[JNI] SetStaticDoubleField(): not implemented\n"); }

jobjectArray  NewObjectArray(JNIEnv* p1, jsize p2, jclass p3, jobject p4) { debugPrintf("[JNI] NewObjectArray(): not implemented\n"); return 0; }
void         SetObjectArrayElement(JNIEnv* p1, jobjectArray p2, jsize p3, jobject p4) { debugPrintf("[JNI] SetObjectArrayElement()\n");  }
jbooleanArray  NewBooleanArray(JNIEnv* p1, jsize p2) { debugPrintf("[JNI] NewBooleanArray(): not implemented\n"); return 0; }
jcharArray     NewCharArray(JNIEnv* p1, jsize p2) { debugPrintf("[JNI] NewCharArray(): not implemented\n"); return 0; }
jshortArray    NewShortArray(JNIEnv* p1, jsize p2) { debugPrintf("[JNI] NewShortArray(): not implemented\n"); return 0; }
jintArray      NewIntArray(JNIEnv* p1, jsize p2) { debugPrintf("[JNI] NewIntArray(): not implemented\n"); return 0; }
jlongArray     NewLongArray(JNIEnv* p1, jsize p2) { debugPrintf("[JNI] NewLongArray(): not implemented\n"); return 0; }
jfloatArray    NewFloatArray(JNIEnv* p1, jsize p2) { debugPrintf("[JNI] NewFloatArray(): not implemented\n"); return 0; }
jdoubleArray   NewDoubleArray(JNIEnv* p1, jsize p2) { debugPrintf("[JNI] NewDoubleArray(): not implemented\n"); return 0; }
jboolean*    GetBooleanArrayElements(JNIEnv* p1, jbooleanArray p2, jboolean* p3) { debugPrintf("[JNI] GetBooleanArrayElements(): not implemented\n"); return 0; }
jbyte*       GetByteArrayElements(JNIEnv* p1, jbyteArray p2, jboolean* p3) { debugPrintf("[JNI] GetByteArrayElements(): not implemented\n"); return 0; }
jchar*       GetCharArrayElements(JNIEnv* p1, jcharArray p2, jboolean* p3) { debugPrintf("[JNI] GetCharArrayElements(): not implemented\n"); return 0; }
jshort*      GetShortArrayElements(JNIEnv* p1, jshortArray p2, jboolean* p3) { debugPrintf("[JNI] GetShortArrayElements(): not implemented\n"); return 0; }
jint*        GetIntArrayElements(JNIEnv* p1, jintArray p2, jboolean* p3) { debugPrintf("[JNI] GetIntArrayElements(): not implemented\n"); return 0; }
jlong*       GetLongArrayElements(JNIEnv* p1, jlongArray p2, jboolean* p3) { debugPrintf("[JNI] GetLongArrayElements(): not implemented\n"); return 0; }
jfloat*      GetFloatArrayElements(JNIEnv* p1, jfloatArray p2, jboolean* p3) { debugPrintf("[JNI] GetFloatArrayElements(): not implemented\n"); return 0; }
jdouble*     GetDoubleArrayElements(JNIEnv* p1, jdoubleArray p2, jboolean* p3) { debugPrintf("[JNI] GetDoubleArrayElements(): not implemented\n"); return 0; }
void         ReleaseBooleanArrayElements(JNIEnv* p1, jbooleanArray p2, jboolean* p3, jint p4) { debugPrintf("[JNI] ReleaseBooleanArrayElements(): not implemented\n"); }
void         ReleaseByteArrayElements(JNIEnv* p1, jbyteArray p2, jbyte* p3, jint p4) { debugPrintf("[JNI] ReleaseByteArrayElements(): not implemented\n"); }
void         ReleaseCharArrayElements(JNIEnv* p1, jcharArray p2, jchar* p3, jint p4) { debugPrintf("[JNI] ReleaseCharArrayElements(): not implemented\n"); }
void         ReleaseShortArrayElements(JNIEnv* p1, jshortArray p2, jshort* p3, jint p4) { debugPrintf("[JNI] ReleaseShortArrayElements(): not implemented\n"); }
void         ReleaseIntArrayElements(JNIEnv* p1, jintArray p2, jint* p3, jint p4) { debugPrintf("[JNI] ReleaseIntArrayElements(): not implemented\n"); }
void         ReleaseLongArrayElements(JNIEnv* p1, jlongArray p2, jlong* p3, jint p4) { debugPrintf("[JNI] ReleaseLongArrayElements(): not implemented\n"); }
void         ReleaseFloatArrayElements(JNIEnv* p1, jfloatArray p2, jfloat* p3, jint p4) { debugPrintf("[JNI] ReleaseFloatArrayElements(): not implemented\n"); }
void         ReleaseDoubleArrayElements(JNIEnv* p1, jdoubleArray p2, jdouble* p3, jint p4) { debugPrintf("[JNI] ReleaseDoubleArrayElements(): not implemented\n"); }
void         GetBooleanArrayRegion(JNIEnv* p1, jbooleanArray p2, jsize p3, jsize p4, jboolean* p5) { debugPrintf("[JNI] GetBooleanArrayRegion(): not implemented\n"); }
void         GetCharArrayRegion(JNIEnv* p1, jcharArray p2, jsize p3, jsize p4, jchar* p5) { debugPrintf("[JNI] GetCharArrayRegion(): not implemented\n"); }
void         GetShortArrayRegion(JNIEnv* p1, jshortArray p2, jsize p3, jsize p4, jshort* p5) { debugPrintf("[JNI] GetShortArrayRegion(): not implemented\n"); }
void         GetLongArrayRegion(JNIEnv* p1, jlongArray p2, jsize p3, jsize p4, jlong* p5) { debugPrintf("[JNI] GetLongArrayRegion(): not implemented\n"); }
void         GetDoubleArrayRegion(JNIEnv* p1, jdoubleArray p2, jsize p3, jsize p4, jdouble* p5) { debugPrintf("[JNI] GetDoubleArrayRegion(): not implemented\n"); }
void         SetBooleanArrayRegion(JNIEnv* p1, jbooleanArray p2, jsize p3, jsize p4, const jboolean* p5) { debugPrintf("[JNI] SetBooleanArrayRegion(): not implemented\n"); }
void         SetByteArrayRegion(JNIEnv* p1, jbyteArray p2, jsize p3, jsize p4, const jbyte* p5) { debugPrintf("[JNI] SetByteArrayRegion(): not implemented\n"); }
void         SetCharArrayRegion(JNIEnv* p1, jcharArray p2, jsize p3, jsize p4, const jchar* p5) { debugPrintf("[JNI] SetCharArrayRegion(): not implemented\n"); }

extern int audio_port;

void         SetShortArrayRegion(JNIEnv* p1, jshortArray p2, jsize p3, jsize p4, const jshort* buf) {
    //printf("SetShortArrayRegion %i\n", p4);
    sceAudioOutOutput(audio_port, buf);
    sceKernelDelayThread(1000);
    //SDL_QueueAudio(deviceId, buf, p4);
    //sceAudioOutOutput(audio_port, buf);
}
void         SetIntArrayRegion(JNIEnv* p1, jintArray p2, jsize p3, jsize p4, const jint* p5) { debugPrintf("[JNI] SetIntArrayRegion(): not implemented\n"); }
void         SetLongArrayRegion(JNIEnv* p1, jlongArray p2, jsize p3, jsize p4, const jlong* p5) { debugPrintf("[JNI] SetLongArrayRegion(): not implemented\n"); }
void         SetFloatArrayRegion(JNIEnv* p1, jfloatArray p2, jsize p3, jsize p4, const jfloat* p5) { debugPrintf("[JNI] SetFloatArrayRegion(): not implemented\n"); }
void         SetDoubleArrayRegion(JNIEnv* p1, jdoubleArray p2, jsize p3, jsize p4, const jdouble* p5) { debugPrintf("[JNI] SetDoubleArrayRegion(): not implemented\n"); }
jint         RegisterNatives(JNIEnv* p1, jclass p2, const JNINativeMethod* p3, jint p4) { debugPrintf("[JNI] RegisterNatives(): not implemented\n"); return 0; }
jint         UnregisterNatives(JNIEnv* p1, jclass p2) { debugPrintf("[JNI] UnregisterNatives(): not implemented\n"); return 0; }
void         GetStringRegion(JNIEnv* p1, jstring p2, jsize p3, jsize p4, jchar* p5) { debugPrintf("[JNI] GetStringRegion(): not implemented\n"); }
void         GetStringUTFRegion(JNIEnv* p1, jstring p2, jsize p3, jsize p4, char* p5) { debugPrintf("[JNI] GetStringUTFRegion(): not implemented\n"); }
void*        GetPrimitiveArrayCritical(JNIEnv* p1, jarray p2, jboolean* p3) { debugPrintf("[JNI] GetPrimitiveArrayCritical(): not implemented\n"); return 0; }
void         ReleasePrimitiveArrayCritical(JNIEnv* p1, jarray p2, void* p3, jint p4) { debugPrintf("[JNI] ReleasePrimitiveArrayCritical(): not implemented\n"); }
const jchar*  GetStringCritical(JNIEnv* p1, jstring p2, jboolean* p3) { debugPrintf("[JNI] GetStringCritical(): not implemented\n"); return 0; }
void         ReleaseStringCritical(JNIEnv* p1, jstring p2, const jchar* p3) { debugPrintf("[JNI] ReleaseStringCritical(): not implemented\n"); }
jweak        NewWeakGlobalRef(JNIEnv* p1, jobject p2) { debugPrintf("[JNI] NewWeakGlobalRef(): not implemented\n"); return 0; }
void         DeleteWeakGlobalRef(JNIEnv* p1, jweak p2) { debugPrintf("[JNI] DeleteWeakGlobalRef(): not implemented\n"); }
jboolean     ExceptionCheck(JNIEnv* p1) { debugPrintf("[JNI] ExceptionCheck(): not implemented\n"); return 0; }
jobject      NewDirectByteBuffer(JNIEnv* p1, void* p2, jlong p3) { debugPrintf("[JNI] NewDirectByteBuffer(): not implemented\n"); return 0; }
void*        GetDirectBufferAddress(JNIEnv* p1, jobject p2) { debugPrintf("[JNI] GetDirectBufferAddress(): not implemented\n"); return 0; }
jlong        GetDirectBufferCapacity(JNIEnv* p1, jobject p2) { debugPrintf("[JNI] GetDirectBufferCapacity(): not implemented\n"); return 0; }
jobjectRefType GetObjectRefType(JNIEnv* p1, jobject p2) { debugPrintf("[JNI] GetObjectRefType(): not implemented\n"); return 0; }
jint        DestroyJavaVM(JavaVM* vm) { debugPrintf("[JVM] DestroyJavaVM(): not implemented\n"); return 0; }
jint        DetachCurrentThread(JavaVM* vm) { debugPrintf("[JVM] DetachCurrentThread(): not implemented\n"); return 0; }
jint        AttachCurrentThreadAsDaemon(JavaVM* vm, JNIEnv** env, void* p3) { debugPrintf("[JVM] AttachCurrentThreadAsDaemon(): not implemented\n"); return 0; }

void init_jni() {
    _jvm = (struct JNIInvokeInterface *) malloc(sizeof(struct JNIInvokeInterface));
    _jvm->DestroyJavaVM = DestroyJavaVM;
    _jvm->AttachCurrentThread = AttachCurrentThread;
    _jvm->DetachCurrentThread = DetachCurrentThread;
    _jvm->GetEnv = GetEnv;
    _jvm->AttachCurrentThreadAsDaemon = AttachCurrentThreadAsDaemon;

    _jni = (struct JNINativeInterface *) malloc(sizeof(struct JNINativeInterface));
    _jni->GetVersion = GetVersion;
    _jni->DefineClass = DefineClass;
    _jni->FindClass = FindClass;
    _jni->FromReflectedMethod = FromReflectedMethod;
    _jni->FromReflectedField = FromReflectedField;
    _jni->ToReflectedMethod = ToReflectedMethod;
    _jni->GetSuperclass = GetSuperclass;
    _jni->IsAssignableFrom = IsAssignableFrom;
    _jni->ToReflectedField = ToReflectedField;
    _jni->Throw = Throw;
    _jni->ThrowNew = ThrowNew;
    _jni->ExceptionOccurred = ExceptionOccurred;
    _jni->ExceptionDescribe = ExceptionDescribe;
    _jni->ExceptionClear = ExceptionClear;
    _jni->FatalError = FatalError;
    _jni->PushLocalFrame = PushLocalFrame;
    _jni->PopLocalFrame = PopLocalFrame;
    _jni->NewGlobalRef = NewGlobalRef;
    _jni->DeleteGlobalRef = DeleteGlobalRef;
    _jni->DeleteLocalRef = DeleteLocalRef;
    _jni->IsSameObject = IsSameObject;
    _jni->NewLocalRef = NewLocalRef;
    _jni->EnsureLocalCapacity = EnsureLocalCapacity;
    _jni->AllocObject = AllocObject;
    _jni->NewObject = NewObject;
    _jni->NewObjectV = NewObjectV;
    _jni->NewObjectA = NewObjectA;
    _jni->GetObjectClass = GetObjectClass;
    _jni->IsInstanceOf = IsInstanceOf;
    _jni->GetMethodID = GetMethodID;
    _jni->CallObjectMethod = CallObjectMethod;
    _jni->CallObjectMethodV = CallObjectMethodV;
    _jni->CallObjectMethodA = CallObjectMethodA;
    _jni->CallBooleanMethod = CallBooleanMethod;
    _jni->CallBooleanMethodV = CallBooleanMethodV;
    _jni->CallBooleanMethodA = CallBooleanMethodA;
    _jni->CallByteMethod = CallByteMethod;
    _jni->CallByteMethodV = CallByteMethodV;
    _jni->CallByteMethodA = CallByteMethodA;
    _jni->CallCharMethod = CallCharMethod;
    _jni->CallCharMethodV = CallCharMethodV;
    _jni->CallCharMethodA = CallCharMethodA;
    _jni->CallShortMethod = CallShortMethod;
    _jni->CallShortMethodV = CallShortMethodV;
    _jni->CallShortMethodA = CallShortMethodA;
    _jni->CallIntMethod = CallIntMethod;
    _jni->CallIntMethodV = CallIntMethodV;
    _jni->CallIntMethodA = CallIntMethodA;
    _jni->CallLongMethod = CallLongMethod;
    _jni->CallLongMethodV = CallLongMethodV;
    _jni->CallLongMethodA = CallLongMethodA;
    _jni->CallFloatMethod = CallFloatMethod;
    _jni->CallFloatMethodV = CallFloatMethodV;
    _jni->CallFloatMethodA = CallFloatMethodA;
    _jni->CallDoubleMethod = CallDoubleMethod;
    _jni->CallDoubleMethodV = CallDoubleMethodV;
    _jni->CallDoubleMethodA = CallDoubleMethodA;
    _jni->CallVoidMethod = CallVoidMethod;
    _jni->CallVoidMethodV = CallVoidMethodV;
    _jni->CallVoidMethodA = CallVoidMethodA;
    _jni->CallNonvirtualObjectMethod = CallNonvirtualObjectMethod;
    _jni->CallNonvirtualObjectMethodV = CallNonvirtualObjectMethodV;
    _jni->CallNonvirtualObjectMethodA = CallNonvirtualObjectMethodA;
    _jni->CallNonvirtualBooleanMethod = CallNonvirtualBooleanMethod;
    _jni->CallNonvirtualBooleanMethodV = CallNonvirtualBooleanMethodV;
    _jni->CallNonvirtualBooleanMethodA = CallNonvirtualBooleanMethodA;
    _jni->CallNonvirtualByteMethod = CallNonvirtualByteMethod;
    _jni->CallNonvirtualByteMethodV = CallNonvirtualByteMethodV;
    _jni->CallNonvirtualByteMethodA = CallNonvirtualByteMethodA;
    _jni->CallNonvirtualCharMethod = CallNonvirtualCharMethod;
    _jni->CallNonvirtualCharMethodV = CallNonvirtualCharMethodV;
    _jni->CallNonvirtualCharMethodA = CallNonvirtualCharMethodA;
    _jni->CallNonvirtualShortMethod = CallNonvirtualShortMethod;
    _jni->CallNonvirtualShortMethodV = CallNonvirtualShortMethodV;
    _jni->CallNonvirtualShortMethodA = CallNonvirtualShortMethodA;
    _jni->CallNonvirtualIntMethod = CallNonvirtualIntMethod;
    _jni->CallNonvirtualIntMethodV = CallNonvirtualIntMethodV;
    _jni->CallNonvirtualIntMethodA = CallNonvirtualIntMethodA;
    _jni->CallNonvirtualLongMethod = CallNonvirtualLongMethod;
    _jni->CallNonvirtualLongMethodV = CallNonvirtualLongMethodV;
    _jni->CallNonvirtualLongMethodA = CallNonvirtualLongMethodA;
    _jni->CallNonvirtualFloatMethod = CallNonvirtualFloatMethod;
    _jni->CallNonvirtualFloatMethodV = CallNonvirtualFloatMethodV;
    _jni->CallNonvirtualFloatMethodA = CallNonvirtualFloatMethodA;
    _jni->CallNonvirtualDoubleMethod = CallNonvirtualDoubleMethod;
    _jni->CallNonvirtualDoubleMethodV = CallNonvirtualDoubleMethodV;
    _jni->CallNonvirtualDoubleMethodA = CallNonvirtualDoubleMethodA;
    _jni->CallNonvirtualVoidMethod = CallNonvirtualVoidMethod;
    _jni->CallNonvirtualVoidMethodV = CallNonvirtualVoidMethodV;
    _jni->CallNonvirtualVoidMethodA = CallNonvirtualVoidMethodA;
    _jni->GetFieldID = GetFieldID;
    _jni->GetObjectField = GetObjectField;
    _jni->GetBooleanField = GetBooleanField;
    _jni->GetByteField = GetByteField;
    _jni->GetCharField = GetCharField;
    _jni->GetShortField = GetShortField;
    _jni->GetIntField = GetIntField;
    _jni->GetLongField = GetLongField;
    _jni->GetFloatField = GetFloatField;
    _jni->GetDoubleField = GetDoubleField;
    _jni->SetObjectField = SetObjectField;
    _jni->SetBooleanField = SetBooleanField;
    _jni->SetByteField = SetByteField;
    _jni->SetCharField = SetCharField;
    _jni->SetShortField = SetShortField;
    _jni->SetIntField = SetIntField;
    _jni->SetLongField = SetLongField;
    _jni->SetFloatField = SetFloatField;
    _jni->SetDoubleField = SetDoubleField;
    _jni->GetStaticMethodID = GetStaticMethodID;
    _jni->CallStaticObjectMethod = CallStaticObjectMethod;
    _jni->CallStaticObjectMethodV = CallStaticObjectMethodV;
    _jni->CallStaticObjectMethodA = CallStaticObjectMethodA;
    _jni->CallStaticBooleanMethod = CallStaticBooleanMethod;
    _jni->CallStaticBooleanMethodV = CallStaticBooleanMethodV;
    _jni->CallStaticBooleanMethodA = CallStaticBooleanMethodA;
    _jni->CallStaticByteMethod = CallStaticByteMethod;
    _jni->CallStaticByteMethodV = CallStaticByteMethodV;
    _jni->CallStaticByteMethodA = CallStaticByteMethodA;
    _jni->CallStaticCharMethod = CallStaticCharMethod;
    _jni->CallStaticCharMethodV = CallStaticCharMethodV;
    _jni->CallStaticCharMethodA = CallStaticCharMethodA;
    _jni->CallStaticShortMethod = CallStaticShortMethod;
    _jni->CallStaticShortMethodV = CallStaticShortMethodV;
    _jni->CallStaticShortMethodA = CallStaticShortMethodA;
    _jni->CallStaticIntMethod = CallStaticIntMethod;
    _jni->CallStaticIntMethodV = CallStaticIntMethodV;
    _jni->CallStaticIntMethodA = CallStaticIntMethodA;
    _jni->CallStaticLongMethod = CallStaticLongMethod;
    _jni->CallStaticLongMethodV = CallStaticLongMethodV;
    _jni->CallStaticLongMethodA = CallStaticLongMethodA;
    _jni->CallStaticFloatMethod = CallStaticFloatMethod;
    _jni->CallStaticFloatMethodV = CallStaticFloatMethodV;
    _jni->CallStaticFloatMethodA = CallStaticFloatMethodA;
    _jni->CallStaticDoubleMethod = CallStaticDoubleMethod;
    _jni->CallStaticDoubleMethodV = CallStaticDoubleMethodV;
    _jni->CallStaticDoubleMethodA = CallStaticDoubleMethodA;
    _jni->CallStaticVoidMethod = CallStaticVoidMethod;
    _jni->CallStaticVoidMethodV = CallStaticVoidMethodV;
    _jni->CallStaticVoidMethodA = CallStaticVoidMethodA;
    _jni->GetStaticFieldID = GetStaticFieldID;
    _jni->GetStaticObjectField = GetStaticObjectField;
    _jni->GetStaticBooleanField = GetStaticBooleanField;
    _jni->GetStaticByteField = GetStaticByteField;
    _jni->GetStaticCharField = GetStaticCharField;
    _jni->GetStaticShortField = GetStaticShortField;
    _jni->GetStaticIntField = GetStaticIntField;
    _jni->GetStaticLongField = GetStaticLongField;
    _jni->GetStaticFloatField = GetStaticFloatField;
    _jni->GetStaticDoubleField = GetStaticDoubleField;
    _jni->SetStaticObjectField = SetStaticObjectField;
    _jni->SetStaticBooleanField = SetStaticBooleanField;
    _jni->SetStaticByteField = SetStaticByteField;
    _jni->SetStaticCharField = SetStaticCharField;
    _jni->SetStaticShortField = SetStaticShortField;
    _jni->SetStaticIntField = SetStaticIntField;
    _jni->SetStaticLongField = SetStaticLongField;
    _jni->SetStaticFloatField = SetStaticFloatField;
    _jni->SetStaticDoubleField = SetStaticDoubleField;
    _jni->NewString = NewString;
    _jni->GetStringLength = GetStringLength;
    _jni->GetStringChars = GetStringChars;
    _jni->ReleaseStringChars = ReleaseStringChars;
    _jni->NewStringUTF = NewStringUTF;
    _jni->GetStringUTFLength = GetStringUTFLength;
    _jni->GetStringUTFChars = GetStringUTFChars;
    _jni->ReleaseStringUTFChars = ReleaseStringUTFChars;
    _jni->GetArrayLength = GetArrayLength;
    _jni->NewObjectArray = NewObjectArray;
    _jni->GetObjectArrayElement = GetObjectArrayElement;
    _jni->SetObjectArrayElement = SetObjectArrayElement;
    _jni->NewBooleanArray = NewBooleanArray;
    _jni->NewByteArray = NewByteArray;
    _jni->NewCharArray = NewCharArray;
    _jni->NewShortArray = NewShortArray;
    _jni->NewIntArray = NewIntArray;
    _jni->NewLongArray = NewLongArray;
    _jni->NewFloatArray = NewFloatArray;
    _jni->NewDoubleArray = NewDoubleArray;
    _jni->GetBooleanArrayElements = GetBooleanArrayElements;
    _jni->GetByteArrayElements = GetByteArrayElements;
    _jni->GetCharArrayElements = GetCharArrayElements;
    _jni->GetShortArrayElements = GetShortArrayElements;
    _jni->GetIntArrayElements = GetIntArrayElements;
    _jni->GetLongArrayElements = GetLongArrayElements;
    _jni->GetFloatArrayElements = GetFloatArrayElements;
    _jni->GetDoubleArrayElements = GetDoubleArrayElements;
    _jni->ReleaseBooleanArrayElements = ReleaseBooleanArrayElements;
    _jni->ReleaseByteArrayElements = ReleaseByteArrayElements;
    _jni->ReleaseCharArrayElements = ReleaseCharArrayElements;
    _jni->ReleaseShortArrayElements = ReleaseShortArrayElements;
    _jni->ReleaseIntArrayElements = ReleaseIntArrayElements;
    _jni->ReleaseLongArrayElements = ReleaseLongArrayElements;
    _jni->ReleaseFloatArrayElements = ReleaseFloatArrayElements;
    _jni->ReleaseDoubleArrayElements = ReleaseDoubleArrayElements;
    _jni->GetBooleanArrayRegion = GetBooleanArrayRegion;
    _jni->GetByteArrayRegion = GetByteArrayRegion;
    _jni->GetCharArrayRegion = GetCharArrayRegion;
    _jni->GetShortArrayRegion = GetShortArrayRegion;
    _jni->GetIntArrayRegion = GetIntArrayRegion;
    _jni->GetLongArrayRegion = GetLongArrayRegion;
    _jni->GetFloatArrayRegion = GetFloatArrayRegion;
    _jni->GetDoubleArrayRegion = GetDoubleArrayRegion;
    _jni->SetBooleanArrayRegion = SetBooleanArrayRegion;
    _jni->SetByteArrayRegion = SetByteArrayRegion;
    _jni->SetCharArrayRegion = SetCharArrayRegion;
    _jni->SetShortArrayRegion = SetShortArrayRegion;
    _jni->SetIntArrayRegion = SetIntArrayRegion;
    _jni->SetLongArrayRegion = SetLongArrayRegion;
    _jni->SetFloatArrayRegion = SetFloatArrayRegion;
    _jni->SetDoubleArrayRegion = SetDoubleArrayRegion;
    _jni->RegisterNatives = RegisterNatives;
    _jni->UnregisterNatives = UnregisterNatives;
    _jni->MonitorEnter = MonitorEnter;
    _jni->MonitorExit = MonitorExit;
    _jni->GetJavaVM = GetJavaVM;
    _jni->GetStringRegion = GetStringRegion;
    _jni->GetStringUTFRegion = GetStringUTFRegion;
    _jni->GetPrimitiveArrayCritical = GetPrimitiveArrayCritical;
    _jni->ReleasePrimitiveArrayCritical = ReleasePrimitiveArrayCritical;
    _jni->GetStringCritical = GetStringCritical;
    _jni->ReleaseStringCritical = ReleaseStringCritical;
    _jni->NewWeakGlobalRef = NewWeakGlobalRef;
    _jni->DeleteWeakGlobalRef = DeleteWeakGlobalRef;
    _jni->ExceptionCheck = ExceptionCheck;
    _jni->NewDirectByteBuffer = NewDirectByteBuffer;
    _jni->GetDirectBufferAddress = GetDirectBufferAddress;
    _jni->GetDirectBufferCapacity = GetDirectBufferCapacity;
    _jni->GetObjectRefType = GetObjectRefType;

    jvm = _jvm;
    jni = _jni;

    if (pthread_mutex_init(&dynamicallyAllocatedArrays_mutex, NULL) != 0) {
        fprintf(stderr, "[ERROR] dynamicallyAllocatedArrays_mutex init failed!!!\n");
    }
}
