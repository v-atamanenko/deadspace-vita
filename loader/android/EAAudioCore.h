#ifndef SOLOADER_EAAUDIOCORE_H
#define SOLOADER_EAAUDIOCORE_H

#include "android/jni.h"

struct _AudioTrack {
    int gap[256];
    void (*flush)(void);
    void (*release)(void);
};

typedef struct _AudioTrack * AudioTrack;


void EAAudioCore__Startup();
void EAAudioCore__Shutdown();

int EAAudioCore_AudioTrack_write(int id, va_list args);
void EAAudioCore_AudioTrack_play(int id, va_list args);
void EAAudioCore_AudioTrack_stop(int id, va_list args);

#endif // SOLOADER_EAAUDIOCORE_H