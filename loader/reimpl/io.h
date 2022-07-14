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
#include <sys/dirent.h>

typedef struct __attribute__((__packed__)) stat64_bionic {
    unsigned long long st_dev;
    unsigned char __pad0[4];
    unsigned long st_ino;
    unsigned int st_mode;
    unsigned int st_nlink;
    unsigned long st_uid;
    unsigned long st_gid;
    unsigned long long st_rdev;
    unsigned char __pad3[4];
    unsigned long st_size;
    unsigned long st_blksize;
    unsigned long st_blocks;
    unsigned long st_atime;
    unsigned long st_atime_nsec;
    unsigned long st_mtime;
    unsigned long st_mtime_nsec;
    unsigned long st_ctime;
    unsigned long st_ctime_nsec;
    unsigned long long __pad4;
} stat64_bionic;

/** d_type value when the type is not known. */
#define DT_UNKNOWN 0
/** d_type value for a FIFO. */
#define DT_FIFO 1
/** d_type value for a character device. */
#define DT_CHR 2
/** d_type value for a directory. */
#define DT_DIR 4
/** d_type value for a block device. */
#define DT_BLK 6
/** d_type value for a regular file. */
#define DT_REG 8
/** d_type value for a symbolic link. */
#define DT_LNK 10
/** d_type value for a socket. */
#define DT_SOCK 12
#define DT_WHT 14

typedef struct __attribute__((__packed__)) dirent64_bionic {
    int16_t d_ino; // 2 bytes // offset 0x0
    int64_t d_off; // 8 bytes // offset 0x2
    uint64_t d_reclen; // 8 bytes // 0xA
    unsigned char d_type; // 1 byte // offset 0x12
    char d_name[256]; // 256 bytes // offset 0x13
} dirent64_bionic;


// vitasdk lacks proper definition of open() although it has its implementation
int open(const char *pathname, int flags);

int read_soloader(int __fd, void *__buf, size_t __nbyte);

int closedir_soloader(DIR* dir);
int readdir_r_soloader(DIR *dirp, dirent64_bionic *entry, dirent64_bionic **result);

DIR* opendir_soloader(char* name);

FILE *fopen_soloader(char *fname, char *mode);

int open_soloader(char *fname, int flags);

int fstat_soloader(int fd, void *statbuf);

int write_soloader(int fd, const void *buf, int count);

int close_soloader(int fd);

int stat_soloader(char *pathname, stat64_bionic *statbuf);

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
