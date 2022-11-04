/*
 * utils.h
 *
 * Common helper utilities.
 *
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef SOLOADER_UTILS_H
#define SOLOADER_UTILS_H

#include <sys/types.h>
#include "config.h"

void* retNULL(void);

int ret0(void);

__attribute__((unused)) int ret1(void);

int retminus1(void);

void do_nothing(void);

int file_exists(const char *path);

int debugPrintf(char *text, ...);

int check_kubridge(void);

int string_ends_with(const char * str, const char * suffix);

void strprepend(char* s, const char* t);

__attribute__((unused)) inline int string_starts_with(const char *pre,
                                                      const char *str) {
    char cp;
    char cs;

    if (!*pre)
        return 1;

    while ((cp = *pre++) && (cs = *str++))
    {
        if (cp != cs)
            return 0;
    }

    if (!cs)
        return 0;

    return 1;
}

long long current_timestamp();

void assert(int i);

char *strremove(char *str, const char *sub);

char* strreplace(char *target, const char *needle, const char *replacement);

void check_init_mutex(pthread_mutex_t* mut);

uint64_t currenttime_ms();

int8_t is_dir(char* p);

#endif // SOLOADER_UTILS_H
