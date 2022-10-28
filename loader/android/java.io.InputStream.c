#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/dirent.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <sys/unistd.h>
#include "utils/utils.h"

#include "java.io.InputStream.h"
#include "android/jni.h"
#include "jni_fake.h"

FILE* f = NULL;
int fd = -1;
pthread_mutex_t mut;

size_t listSize = 0;
size_t listAllocedSize = 0;
char** list = NULL;
int calls_count = 0;

const char* assetsPathPrefix = DATA_PATH_INT;

// public int read(byte[] b, int off, int len)
// https://docs.oracle.com/javase/7/docs/api/java/io/InputStream.html#read()
jint InputStream_read(int id, va_list args) {
    debugPrintf("JNI: Method Call: InputStream_read() / id: %i\n", id);
    // Imporant note: there are also versions of read() with two and one arg;
    // here we assume that only the full version with 3 args is used, which is
    // dangerous.

    char* b = va_arg(args, char*);
    int off = va_arg(args, int);
    int len = va_arg(args, int);

    if (!f) {
        if (fd == -1) {
            debugPrintf("[java.io.InputStream.read()] File descriptor is NULL.\n");
            return -1;
        }
    }

    if (len <= 0) {
        return 0;
    }

    size_t bytes_read;
    if (!f) {
        bytes_read = read(fd, b+off, len);
    } else {
        bytes_read = fread(b+off, 1, len, f);
    }

    if (bytes_read <= 0) {
        return -1;
    }

    return (int)bytes_read;
}

// public void close ()
// https://developer.android.com/reference/android/content/res/AssetManager#close()
void InputStream_close(int id, va_list args) {
    debugPrintf("JNI: Method Call: InputStream_close() / id: %i\n", id);
    //pthread_mutex_destroy(&mut);
    if (f) {
        fclose(f);
        f = NULL;
    }
    if (fd > -1) {
        close(fd);
        fd = -1;
    }
}

// public long skip(long n)
// https://docs.oracle.com/javase/7/docs/api/java/io/InputStream.html#skip(long)
jlong InputStream_skip(int id, va_list args) {
    debugPrintf("JNI: Method Call: InputStream_skip() / id: %i\n", id);
    int64_t off = va_arg(args, int64_t);

    if (!f) {
        debugPrintf("[java.io.InputStream.skip()] File descriptor is NULL.\n");
        return -1;
    }

    if (fseek(f, off, SEEK_CUR) < 0 ) {
        return -1;
    }

    // NOTE: According to Java docs, this function must return the actual
    // number of bytes skipped. For efficiency, let's not count that.
    return 1;
}

// public InputStream open (String fileName)
// https://developer.android.com/reference/android/content/res/AssetManager#open(java.lang.String)
jobject InputStream_open(int id, va_list args) {
    // This method is supposed to initialize the asset to be opened, but
    // since we don't need that, let's just return a dummy string that can
    // be freed later.
    debugPrintf("JNI: Method Call: InputStream_open(%) / id: %i\n", id);
    //pthread_mutex_init(&mut, NULL);
    return strdup("nop");
}

// public AssetFileDescriptor openFd (String fileName)
// https://developer.android.com/reference/android/content/res/AssetManager#openFd(java.lang.String)
jint InputStream_openFd(int id, va_list args) {
    debugPrintf("JNI: Method Call: InputStream_openFd() / id: %i\n", id);
    const char* fileName = va_arg(args, const char*);

    char temp[1024];
    sprintf(temp, "%s%s", assetsPathPrefix, fileName);

    debugPrintf("[java.io.InputStream] InputStream_openFd(\"%s\")\n", temp);
    fd = open(temp, O_RDONLY);
    return fd;
}



// public String[] list (String path)
// https://developer.android.com/reference/android/content/res/AssetManager#list(java.lang.String)
jobject InputStream_list(int id, va_list args) {
    const char* path_tmp = va_arg(args, const char*);

    debugPrintf("JNI: Method Call: InputStream_list() / id: %i / path: \"%s\" (0x%x)\n", id, path_tmp, path_tmp);

    char* path = malloc(512 * sizeof(char)); // freed in _internal()
    snprintf(path, 512, "%s%s", assetsPathPrefix, path_tmp);

    //pthread_mutex_lock(&mut);

    char** paths_stack = malloc(sizeof(char*) * 1024);

    int i = 0;
    paths_stack[i++] = path;

    int first_run = 1;

    while (i > 0) {
        path = paths_stack[--i];

        DIR *dir;
        struct dirent *entry;

        calls_count++;

        if (!(dir = opendir(path))) {
            free(path);
            continue;
        }

        while ((entry = readdir(dir)) != NULL) {
            char* p = malloc(1000 * sizeof(char));

            if (first_run) {
                snprintf(p, 999, "%s%s", path, entry->d_name);
            } else {
                snprintf(p, 999, "%s/%s", path, entry->d_name);
            }

            if (!is_dir(p)) {
                if (listSize == 0) {
                    list = malloc(sizeof(char*) * 1734);
                    listAllocedSize = 1733;
                } else {
                    if (listSize >= listAllocedSize) {
                        list = realloc(list, (sizeof(char*) * (listSize+100)));
                        listAllocedSize = listSize + 99;
                    }
                }
                list[listSize] = strdup(p);
                listSize++;
                free(p); // crash here
            } else {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;
                paths_stack[i++] = p;
            }
        }
        closedir(dir);
        free(path);

        if (first_run) {
            first_run = 0;
        }
    }

    char** list_ret = malloc(sizeof(char*) * listSize);

    for (int u = 0; u < listSize; ++u) {
        list_ret[u] = strdup(list[u]);
        free(list[u]);
    }

    debugPrintf("OL1\n");
    saveDynamicallyAllocatedArrayPointer(list_ret, (jsize)listSize);
    debugPrintf("OL2\n");
    free(list);
    debugPrintf("OL3\n");
    listSize = 0;
    debugPrintf("OL4\n");

    //pthread_mutex_unlock(&mut);
    debugPrintf("OL5\n");
    return (jobject)list_ret;
}

// public long getLength ()
// https://developer.android.com/reference/android/content/res/AssetFileDescriptor#getLength()
jlong InputStream_getLength(int id, va_list args) {
    debugPrintf("JNI: Method Call: InputStream_getLength() / id: %i / fd: %i\n", id, fd);

    if (fd == -1) {
        debugPrintf("[java.io.InputStream.getLength()] Error: fd is invalid\n");
        return -1;
    }

    size_t sz = lseek(fd, 0L, SEEK_END);
    lseek(fd, 0L, SEEK_SET);

    return (jlong)sz;
}
