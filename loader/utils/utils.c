/*
 * utils.c
 *
 * Common helper utilities.
 *
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils.h"

#include <psp2/io/stat.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>

#pragma ide diagnostic ignored "bugprone-reserved-identifier"

// For check_kubridge
SceUID _vshKernelSearchModuleByName(const char *, int *);

void* retNULL(void) {
    return NULL;
}

int ret0(void) {
    return 0;
}

__attribute__((unused)) int ret1(void) {
    return 1;
}

int retminus1(void) {
    return -1;
}

void do_nothing(void) {
    // Silence is golden.
}

int file_exists(const char *path) {
    SceIoStat stat;
    return sceIoGetstat(path, &stat) >= 0;
}

// OpenSLES wants `assert()` and somehow we don't have it?
void assert(int i) {
    if (!i) {
        debugPrintf("assertion failed\n");
    }
}

int debugPrintf(char *text, ...) {
#ifdef DEBUG
    va_list list;
    static char string[0x8000];

    va_start(list, text);
    vsprintf(string, text, list);
    va_end(list);

    fprintf(stderr, "%s", string);
#endif
    return 0;
}

int check_kubridge(void) {
    int search_unk[2];
    return _vshKernelSearchModuleByName("kubridge", search_unk);
}

int string_ends_with(const char * str, const char * suffix)
{
    int str_len = (int)strlen(str);
    int suffix_len = (int)strlen(suffix);

    return
            (str_len >= suffix_len) &&
            (0 == strcmp(str + (str_len-suffix_len), suffix));
}

long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate ms
    return milliseconds;
}
