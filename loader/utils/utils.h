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

#endif // SOLOADER_UTILS_H
