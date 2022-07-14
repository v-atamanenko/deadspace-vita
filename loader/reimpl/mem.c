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

#include <stdio.h>
#include <string.h>
#include "mem.h"

void *sceClibMemclr(void *dst, SceSize len) {
    return sceClibMemset(dst, 0, len);
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offs) {
    fprintf(stderr, "mmap(addr: 0x%x, length: %i, prot: %i, flags: %i, fd %i, offs: %i: ", (int)addr, length, prot, flags, fd, offs);
    void* ret= malloc(length);
    fprintf(stderr, "ret 0x%x\n", (int)ret);
    memset(ret, 0, length);
    return ret;
}

int munmap(void *addr, size_t length) {
    fprintf(stderr, "munmap 0x%x, length %i: ", (int)addr, length);
    free(addr);
    fprintf(stderr, "success\n");
    return 0;
}
