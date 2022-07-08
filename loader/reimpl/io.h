/*
 * io.h
 *
 * Wrappers and implementations for some of IO functions for optimization
 * and bridging to SceLibc.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2022 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef SOLOADER_IO_H
#define SOLOADER_IO_H

#include <stdio.h>

// vitasdk lacks proper definition of open() although it has its implementation
int open(const char *pathname, int flags);

FILE *fopen_soloader(char *fname, char *mode);

int open_soloader(const char *fname, int flags);

int fstat_soloader(int fd, void *statbuf);

int write_soloader(int fd, const void *buf, int count);

int close_soloader(int fd);

int stat_soloader(const char *pathname, void *statbuf);

void *AAssetManager_open(void *mgr, const char *filename, int mode);

int fseeko_soloader(FILE * a, off_t b, int c);

off_t ftello_soloader(FILE * a);

/*
 * Stuff related to in-memory assets preloading.
 */

typedef struct inmemfile {
    void * buf;
    size_t size;
} inmemfile;

void preload();

#define FFULLREAD_OK      (0)
#define FFULLREAD_INVALID (-1) // Invalid params
#define FFULLREAD_ERROR   (-2) // File stream error
#define FFULLREAD_TOOMUCH (-3) // Too much input
#define FFULLREAD_NOMEM   (-4)

int ffullread(FILE *f, void **dataptr, size_t *sizeptr, size_t chunk);

#endif // SOLOADER_IO_H
