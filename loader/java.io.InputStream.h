#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include "android/jni.h"

// public int read(byte[] b, int off, int len)
// https://docs.oracle.com/javase/7/docs/api/java/io/InputStream.html#read()
jint InputStream_read(int id, va_list args);

// public long skip(long n)
// https://docs.oracle.com/javase/7/docs/api/java/io/InputStream.html#skip(long)
jlong InputStream_skip(int id, va_list args);

// public void close ()
// https://developer.android.com/reference/android/content/res/AssetManager#close()
void InputStream_close(int id, va_list args);

// public InputStream open (String fileName)
// https://developer.android.com/reference/android/content/res/AssetManager#open(java.lang.String)
jobject InputStream_open(int id, va_list args);

// public AssetFileDescriptor openFd (String fileName)
// https://developer.android.com/reference/android/content/res/AssetManager#openFd(java.lang.String)
jint InputStream_openFd(int id, va_list args);

// public String[] list (String path)
// https://developer.android.com/reference/android/content/res/AssetManager#list(java.lang.String)
jobject InputStream_list(int id, va_list args);

// public long getLength ()
// https://developer.android.com/reference/android/content/res/AssetFileDescriptor#getLength()
jlong InputStream_getLength(int id, va_list args);
