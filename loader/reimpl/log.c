/*
 * log.c
 *
 * Implementations for different Android logging functions.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2022 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "log.h"

#include <stdio.h>
#include "utils/utils.h"

int android_log_write(int prio, const char *tag, const char *msg) {
    debugPrintf("[LOG][%i] %s: %s\n", prio, tag, msg);
    return 0;
}

int android_log_print(int prio, const char *tag, const char *fmt, ...) {
    va_list list;
    static char string[0x8000];

    va_start(list, fmt);
    vsprintf(string, fmt, list);
    va_end(list);

    debugPrintf("[LOG][%i] %s: %s\n", prio, tag, string);
    return 0;
}

int android_log_vprint(int pri, const char *tag, const char *fmt, va_list lst) {
    static char string[0x8000];

    vsprintf(string, fmt, lst);
    va_end(lst);

    debugPrintf("[LOGV][%i] %s: %s\n", pri, tag, string);
    return 0;
}

void mbedtls_debug_print_msg(const void *ssl, int level, const char *file,
                             int line, const char *format, ...) {
    va_list argp;
    char str[1024];
    int ret;

    va_start( argp, format );
    ret = vsnprintf( str, 1024, format, argp );
    va_end( argp );

    if( ret >= 0 && ret < 1024 - 1 ) {
        str[ret]     = '\n';
        str[ret + 1] = '\0';
    }

    debugPrintf("[mbed][%i/%i][%s:%i]: %s\n", (int)ssl, level, file, line, str);
}
