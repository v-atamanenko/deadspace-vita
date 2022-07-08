/*
 * pthr.c
 *
 * Wrapper for vitasdk/newlib pthread functions to work with
 * Android's pthread struct which is different
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2022 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 * Copyright (C) 2022 GrapheneCt
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "pthr.h"
#include <stdlib.h>
#include <stdio.h>
#include <psp2/kernel/clib.h>
#include <string.h>

#define  MUTEX_TYPE_NORMAL     0x0000
#define  MUTEX_TYPE_RECURSIVE  0x4000
#define  MUTEX_TYPE_ERRORCHECK 0x8000

static pthread_t s_pthreadSelfRet;

static void init_static_mutex(pthread_mutex_t **mutex)
{
    pthread_mutex_t *mtxMem = NULL;

    switch ((int)*mutex) {
        case MUTEX_TYPE_NORMAL:;
            pthread_mutex_t initTmpNormal = PTHREAD_MUTEX_INITIALIZER;
            mtxMem = calloc(1, sizeof(pthread_mutex_t));
            sceClibMemcpy(mtxMem, &initTmpNormal, sizeof(pthread_mutex_t));
            *mutex = mtxMem;
            break;
        case MUTEX_TYPE_RECURSIVE:;
            pthread_mutex_t initTmpRec = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
            mtxMem = calloc(1, sizeof(pthread_mutex_t));
            sceClibMemcpy(mtxMem, &initTmpRec, sizeof(pthread_mutex_t));
            *mutex = mtxMem;
            break;
        case MUTEX_TYPE_ERRORCHECK:;
            pthread_mutex_t initTmpErr = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER;
            mtxMem = calloc(1, sizeof(pthread_mutex_t));
            sceClibMemcpy(mtxMem, &initTmpErr, sizeof(pthread_mutex_t));
            *mutex = mtxMem;
            break;
        default:
            break;
    }
}

static void init_static_cond(pthread_cond_t **cond)
{
    if (*cond == NULL) {
        pthread_cond_t initTmp = PTHREAD_COND_INITIALIZER;
        pthread_cond_t *condMem = calloc(1, sizeof(pthread_cond_t));
        sceClibMemcpy(condMem, &initTmp, sizeof(pthread_cond_t));
        *cond = condMem;
    }
}

int pthread_attr_destroy_soloader(pthread_attr_t **attr)
{
    int ret = 0;
    ret = pthread_attr_destroy(*attr);
    free(*attr);
    return ret;
}

__attribute__((unused)) int pthread_condattr_init_soloader(pthread_condattr_t **attr)
{
    *attr = calloc(1, sizeof(pthread_condattr_t));

    return pthread_condattr_init(*attr);
}

__attribute__((unused)) int pthread_condattr_destroy_soloader(pthread_condattr_t **attr)
{
    int ret = 0;
    ret = pthread_condattr_destroy(*attr);
    free(*attr);
    return ret;
}

int pthread_cond_init_soloader(pthread_cond_t **cond,
                               const pthread_condattr_t **attr)
{
    *cond = calloc(1, sizeof(pthread_cond_t));

    if (attr != NULL)
        return pthread_cond_init(*cond, *attr);
    else
        return pthread_cond_init(*cond, NULL);
}

int pthread_cond_destroy_soloader(pthread_cond_t **cond)
{
    int ret = 0;
    ret = pthread_cond_destroy(*cond);
    free(*cond);
    return ret;
}

int pthread_cond_signal_soloader(pthread_cond_t **cond)
{
    init_static_cond(cond);
    return pthread_cond_signal(*cond);
}

int pthread_cond_timedwait_soloader(pthread_cond_t **cond,
                                    pthread_mutex_t **mutex,
                                    struct timespec *abstime)
{
    init_static_cond(cond);
    init_static_mutex(mutex);
    return pthread_cond_timedwait(*cond, *mutex, abstime);
}

int pthread_create_soloader(pthread_t **thread,
                            const pthread_attr_t **attr,
                            void *(*start)(void *),
                            void *param)
{
    *thread = calloc(1, sizeof(pthread_t));

    if (attr != NULL)
        return pthread_create(*thread, *attr, start, param);
    else
        return pthread_create(*thread, NULL, start, param);
}

int pthread_mutexattr_init_soloader(pthread_mutexattr_t **attr)
{
    *attr = calloc(1, sizeof(pthread_mutexattr_t));

    return pthread_mutexattr_init(*attr);
}

int pthread_mutexattr_settype_soloader(pthread_mutexattr_t **attr, int type)
{
    return pthread_mutexattr_settype(*attr, type);
}

int pthread_mutexattr_destroy_soloader(pthread_mutexattr_t **attr)
{
    int ret = 0;
    ret = pthread_mutexattr_destroy(*attr);
    free(*attr);
    return ret;
}

int pthread_mutex_destroy_soloader(pthread_mutex_t **mutex)
{
    int ret = 0;
    ret = pthread_mutex_destroy(*mutex);
    free(*mutex);
    return ret;
}

int pthread_mutex_init_soloader(pthread_mutex_t **mutex,
                                const pthread_mutexattr_t **attr)
{
    *mutex = calloc(1, sizeof(pthread_mutex_t));

    if (attr != NULL)
        return pthread_mutex_init(*mutex, *attr);
    else
        return pthread_mutex_init(*mutex, NULL);
}

int pthread_mutex_lock_soloader(pthread_mutex_t **mutex)
{
    init_static_mutex(mutex);
    return pthread_mutex_lock(*mutex);
}

int pthread_mutex_trylock_soloader(pthread_mutex_t **mutex)
{
    init_static_mutex(mutex);
    return pthread_mutex_trylock(*mutex);
}

int pthread_mutex_unlock_soloader(pthread_mutex_t **mutex)
{
    return pthread_mutex_unlock(*mutex);
}

int pthread_join_soloader(pthread_t *thread, void **value_ptr)
{
    return pthread_join(*thread, value_ptr);
}

int pthread_cond_wait_soloader(pthread_cond_t **cond, pthread_mutex_t **mutex)
{
    return pthread_cond_wait(*cond, *mutex);
}

int pthread_cond_broadcast_soloader(pthread_cond_t **cond)
{
    return pthread_cond_broadcast(*cond);
}

int pthread_attr_init_soloader(pthread_attr_t **attr)
{
    *attr = calloc(1, sizeof(pthread_attr_t));

    return pthread_attr_init(*attr);
}

int pthread_attr_setdetachstate_soloader(pthread_attr_t **attr, int state)
{
    return pthread_attr_setdetachstate(*attr, state);
}

int pthread_attr_setstacksize_soloader(pthread_attr_t **attr, size_t stacksize)
{
    return pthread_attr_setstacksize(*attr, stacksize);
}

int pthread_setschedparam_soloader(pthread_t *thread, int policy,
                                   const struct sched_param *param)
{
    return pthread_setschedparam(*thread, policy, param);
}

int pthread_getschedparam_soloader(pthread_t *thread, int *policy,
                                   struct sched_param *param)
{
    return pthread_getschedparam(*thread, policy, param);
}

int pthread_detach_soloader(pthread_t *thread)
{
    return pthread_detach(*thread);
}

int pthread_equal_soloader(pthread_t *t1, pthread_t *t2)
{
    return pthread_equal(*t1, *t2);
}

pthread_t *pthread_self_soloader()
{
    s_pthreadSelfRet = pthread_self();
    return &s_pthreadSelfRet;
}

#ifndef MAX_TASK_COMM_LEN
#define MAX_TASK_COMM_LEN 16
#endif

int pthread_setname_np_soloader(pthread_t *thread, const char* thread_name) {
    if (thread == 0 || thread_name == NULL) {
        return EINVAL;
    }
    size_t thread_name_len = strlen(thread_name);
    if (thread_name_len >= MAX_TASK_COMM_LEN) {
        return ERANGE;
    }

    // TODO: Implement the actual name setting if possible
    fprintf(stderr, "PTHR: pthread_setname_np with name %s\n", thread_name);

    return 0;
}

int sem_destroy_soloader(sem_t * sem) {
    fprintf(stderr, "SEMA: CALLED sem_destroy_soloader\n");
    return sem_destroy(sem);
}

int sem_getvalue_soloader (sem_t * sem, int * sval) {
    fprintf(stderr, "SEMA: CALLED sem_getvalue_soloader\n");
    return sem_getvalue(sem, sval);
}

int sem_init_soloader (sem_t * sem, int pshared, unsigned int value) {
    fprintf(stderr, "SEMA: CALLED sem_init_soloader\n");
    return sem_init(sem, pshared, value);
}

int sem_post_soloader (sem_t * sem) {
    fprintf(stderr, "SEMA: CALLED sem_post_soloader\n");
    return sem_post(sem);
}

int sem_timedwait_soloader (sem_t * sem, const struct timespec * abstime) {
    fprintf(stderr, "SEMA: CALLED sem_timedwait_soloader\n");
    return sem_timedwait(sem, abstime);
}

int sem_trywait_soloader (sem_t * sem) {
    fprintf(stderr, "SEMA: CALLED sem_trywait_soloader\n");
    return sem_trywait(sem);
}

int sem_wait_soloader (sem_t * sem) {
    fprintf(stderr, "SEMA: CALLED sem_wait_soloader\n");
    return sem_wait(sem);
}
