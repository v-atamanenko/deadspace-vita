/*
 * io.c
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

#include "io.h"

#include <string.h>
#include <libc_bridge.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <malloc.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <pthread.h>
#include <psp2/kernel/threadmgr.h>

#include "utils/utils.h"
#include "utils/dialog.h"

char* fix_path(const char * orig_path) {
    char* path_temp = malloc(PATH_MAX * sizeof(char));
    strcpy(path_temp, orig_path);

    if (strstr(path_temp, "appbundle:/")) {
        path_temp = strremove(path_temp, "appbundle:/");
        strprepend(path_temp, DATA_PATH_INT);
        return path_temp;
    }

    path_temp = strremove(path_temp, "Android/data/com.ea.deadspace/files/");
    path_temp = strreplace(path_temp, "deadspace/published", "deadspace/assets/published");
    return path_temp;
}

dirent64_bionic * dirent_newlib_to_dirent_bionic(struct dirent* dirent_newlib) {
    dirent64_bionic * ret = malloc(sizeof(dirent64_bionic));
    strcpy(ret->d_name, dirent_newlib->d_name);
    ret->d_off = 0;
    ret->d_reclen = 0;
    ret->d_type = SCE_S_ISDIR(dirent_newlib->d_stat.st_mode) ? DT_DIR : DT_REG;
    return ret;
}

struct dirent * readdir_soloader(DIR * dir) {
    struct dirent* ret = readdir(dir);
    //debugPrintf("[io] readdir()\n");
    return ret;
}

int readdir_r_soloader(DIR *dirp, dirent64_bionic *entry, dirent64_bionic **result) {
    struct dirent dirent_tmp;
    struct dirent* pdirent_tmp;

    int ret = readdir_r(dirp, &dirent_tmp, &pdirent_tmp);

    if (ret == 0) {
        dirent64_bionic* entry_tmp = dirent_newlib_to_dirent_bionic(&dirent_tmp);
        memcpy(entry, entry_tmp, sizeof(dirent64_bionic));
        *result = (pdirent_tmp != NULL) ? entry : NULL;
        free(entry_tmp);
    }

    //debugPrintf("[io] readdir_r()\n");
    return ret;
}

FILE *fopen_soloader(char *fname, char *mode) {
    FILE* ret =  fopen(fname, mode);
    debugPrintf("[io] fopen(%s): 0x%x\n", fname, ret);
    return ret;
}

int open_soloader(char *_fname, int flags) {
    char* fname = fix_path(_fname);

    int ret = open(fname, flags);
    debugPrintf("[io] open(%s, %x): %i\n", fname, flags, ret);

    if (ret > 0 && flags == 0x242 && (strstr(fname, "CheckpointSave") || strstr(fname, "LevelSave")
                                        || strstr(fname, "settings.sb") || strstr(fname, "AchievementsSave.sb")) ) {
        close_soloader(ret);
        fclose(fopen(fname, "w"));
        debugPrintf("[io] open(): killed the contents of %s.\n", fname);
        ret = open(fname, flags);
    }

    free(fname);
    return ret;
}

int read_soloader(int __fd, void *__buf, size_t __nbyte) {
    int ret = read(__fd, __buf, __nbyte);
    //debugPrintf("[io] read(fd#%i, %x, %i): %i\n", __fd, (int)__buf, __nbyte, ret);
    return ret;
}

DIR* opendir_soloader(char* _pathname) {
    char * pathname = fix_path(_pathname);
    DIR* ret = opendir(pathname);
    debugPrintf("[io] opendir(\"%s\"): 0x%x\n", pathname, ret);
    free(pathname);
    return ret;
}

int fstat_soloader(int fd, void *statbuf) {
    struct stat st;
    int res = fstat(fd, &st);
    if (res == 0)
        *(uint64_t *)(statbuf + 0x30) = st.st_size;

    debugPrintf("[io] fstat(fd#%i): %i\n", fd, res);
    return res;
}

int write_soloader(int fd, const void *buf, int count) {
    int ret = write(fd, buf, count);
    //debugPrintf("[io] write(fd#%i, 0x%x, %i): %i\n", fd, buf, count, ret);
    return ret;
}

int fcntl_soloader(int fd, int cmd, ... /* arg */ ) {
    debugPrintf("[io] fcntl(fd#%i, cmd#%i)\n", fd, cmd);
    return 0;
}

int fsync_soloader(int fd) {
    int ret = fsync(fd);
    debugPrintf("[io] fsync(fd#%i): %i\n", fd, ret);
    return fsync(fd);
}

off_t lseek_soloader(int fildes, off_t offset, int whence) {
    off_t ret = lseek(fildes, offset, whence);
    //debugPrintf("[io] lseek(fd#i, %i, %i): %i\n", fildes, offset, whence, ret);
    return ret;
}

int close_soloader(int fd) {
    int ret = close(fd);
    //debugPrintf("[io] close(fd#%i): %i\n", fd, ret);
    return ret;
}

int closedir_soloader(DIR* dir) {
    int ret = closedir(dir);
    //debugPrintf("[io] closedir(0x%x): %i\n", dir, ret);
    return ret;
}

int stat_soloader(char *_pathname, stat64_bionic *statbuf) {
    char* pathname = fix_path(_pathname);

    struct stat st;
    int res = stat(pathname, &st);

    if (res == 0) {
        if (!statbuf) {
            statbuf = malloc(sizeof(stat64_bionic));
        }
        statbuf->st_dev = st.st_dev;
        statbuf->st_ino = st.st_ino;
        statbuf->st_mode = st.st_mode;
        statbuf->st_nlink = st.st_nlink;
        statbuf->st_uid = st.st_uid;
        statbuf->st_gid = st.st_gid;
        statbuf->st_rdev = st.st_rdev;
        statbuf->st_size = st.st_size;
        statbuf->st_blksize = st.st_blksize;
        statbuf->st_blocks = st.st_blocks;
        statbuf->st_atime = st.st_atime;
        statbuf->st_atime_nsec = 0;
        statbuf->st_mtime = st.st_mtime;
        statbuf->st_mtime_nsec = 0;
        statbuf->st_ctime = st.st_ctime;
        statbuf->st_ctime_nsec = 0;
    }

    //debugPrintf("[io] stat(%s): %i", pathname, res);
    free(pathname);
    return res;
}

int fseeko_soloader(FILE * a, off_t b, int c) {
    int ret = fseeko(a,b,c);
    debugPrintf("[io] fseeko(0x%x, %i, %i): %i\n", a,b,c,ret);
    return ret;
}

off_t ftello_soloader(FILE * a) {
    off_t ret = ftello(a);
    debugPrintf("[io] ftello(0x%x): %i\n", a, ret);
    return ret;
}

int ffullread(FILE *f, void **dataptr, size_t *sizeptr, size_t chunk) {
    char *data = NULL, *temp;
    size_t size = 0;
    size_t used = 0;
    size_t n;

    if (f == NULL || dataptr == NULL || sizeptr == NULL)
        return FFULLREAD_INVALID;

    if (sceLibcBridge_ferror(f))
        return FFULLREAD_ERROR;

    while (1) {
        if (used + chunk + 1 > size) {
            size = used + chunk + 1;

            // Overflow check
            if (size <= used) {
                free(data);
                return FFULLREAD_TOOMUCH;
            }

            temp = realloc(data, size);
            if (temp == NULL) {
                free(data);
                return FFULLREAD_NOMEM;
            }
            data = temp;
        }

        n = sceLibcBridge_fread(data + used, 1, chunk, f);
        if (n == 0)
            break;

        used += n;
    }

    if (sceLibcBridge_ferror(f)) {
        free(data);
        return FFULLREAD_ERROR;
    }

    temp = realloc(data, used + 1);
    if (temp == NULL) {
        free(data);
        return FFULLREAD_NOMEM;
    }
    data = temp;
    data[used] = '\0';

    *dataptr = data;
    *sizeptr = used;

    return FFULLREAD_OK;
}
