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
#include <stdlib.h>
#include <pthread.h>
#include <sys/dirent.h>
#include <dirent.h>

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
    char string[0x8000];

    va_start(list, text);
    vsnprintf(string, 0x8000, text, list);
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

char* strremove(char *str, const char *sub) {
    char *p, *q, *r;
    if (*sub && (q = r = strstr(str, sub)) != NULL) {
        size_t len = strlen(sub);
        while ((r = strstr(p = r + len, sub)) != NULL) {
            while (p < r)
                *q++ = *p++;
        }
        while ((*q++ = *p++) != '\0')
            continue;
    }
    return str;
}

char* strreplace(char *target, const char *needle, const char *replacement) {
    char buffer[1024] = { 0 };
    char *insert_point = &buffer[0];
    const char *tmp = target;
    size_t needle_len = strlen(needle);
    size_t repl_len = strlen(replacement);

    while (1) {
        const char *p = strstr(tmp, needle);

        // walked past last occurrence of needle; copy remaining part
        if (p == NULL) {
            strcpy(insert_point, tmp);
            break;
        }

        // copy part before needle
        memcpy(insert_point, tmp, p - tmp);
        insert_point += p - tmp;

        // copy replacement string
        memcpy(insert_point, replacement, repl_len);
        insert_point += repl_len;

        // adjust pointers, move on
        tmp = p + needle_len;
    }

    // write altered string back to target
    strcpy(target, buffer);
    return target;
}

void check_init_mutex(pthread_mutex_t* mut) {
    if (!mut) {
        fprintf(stderr, "MUTEX INIT!!!\n");
        pthread_mutex_t initTmpNormal;
        fprintf(stderr, "MUTEX INIT2!!!\n");
        mut = calloc(1, sizeof(pthread_mutex_t));
        fprintf(stderr, "MUTEX INIT3!!!\n");
        memcpy(mut, &initTmpNormal, sizeof(pthread_mutex_t));
        fprintf(stderr, "MUTEX INIT4!!!\n");
        pthread_mutex_init(mut, NULL);
    }
}

inline int8_t is_dir(char* p) {
    DIR* filetest = opendir(p);
    if (filetest != NULL) {
        closedir(filetest);
        return 1;
    }
    return 0;
}
