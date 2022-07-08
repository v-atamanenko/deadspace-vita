/*
 * mem.h
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

#ifndef SOLOADER_MEMORY_H
#define SOLOADER_MEMORY_H

#include <psp2/kernel/clib.h>
#include <malloc.h>
#include <sys/types.h>

void *sceClibMemclr(void *dst, SceSize len);

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offs);

int munmap(void *addr, size_t length);

#endif // SOLOADER_MEMORY_H
