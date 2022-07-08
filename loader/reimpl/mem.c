/*
 * mem.c
 *
 * Implementations and wrappers for memory-related functions.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2022 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "mem.h"

void *sceClibMemclr(void *dst, SceSize len) {
    return sceClibMemset(dst, 0, len);
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offs) {
    return malloc(length);
}

int munmap(void *addr, size_t length) {
    free(addr);
    return 0;
}
