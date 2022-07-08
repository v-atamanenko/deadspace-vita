/*
 * sys.h
 *
 * Implementations and wrappers for system-related functions.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2022 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef SOLOADER_SYS_H
#define SOLOADER_SYS_H

#include <sys/time.h>

int clock_gettime_soloader(__attribute__((unused)) int c, struct timespec *t);

int nanosleep_soloader(const struct timespec *rqtp,
        __attribute__((unused)) struct timespec *rmtp);

int system_property_get(const char *name, char *value);

void assert2(const char* f, int l, const char* func, const char* msg);

void syscall(int c);

#endif // SOLOADER_SYS_H
