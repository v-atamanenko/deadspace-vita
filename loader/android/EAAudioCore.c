#include <stdio.h>
#include <malloc.h>
#include <pthread.h>
#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>
#include <string.h>
#include "EAAudioCore.h"
#include "stdbool.h"
#include "android/jni.h"
#include "jni_fake.h"

#define EAAudioCore_nChannels 2
#define EAAudioCore_NativeOutputSampleRate 48000 //Hz
#define EAAudioCore_NativeOutputSamplesPerBuf 8192

// Random values:
#define SAMPLE_RATE_HZ_MIN 1
#define SAMPLE_RATE_HZ_MAX 50000

static bool sInit = false;
static AudioTrack sAudioTrack = NULL;

#define AudioFormat_CHANNEL_OUT_MONO 0x4
#define AudioFormat_CHANNEL_CONFIGURATION_MONO 2

#define AudioFormat_CHANNEL_OUT_STEREO (0x4 | 0x8)
#define AudioFormat_CHANNEL_CONFIGURATION_STEREO 3

/** Audio data format: PCM 16 bit per sample. Guaranteed to be supported by devices. */
#define ENCODING_PCM_16BIT 2
/** Audio data format: PCM 8 bit per sample. Not guaranteed to be supported by devices. */
#define ENCODING_PCM_8BIT 3
/** Audio data format: single-precision floating-point per sample */
#define ENCODING_PCM_FLOAT 4
/** Audio data format: AC-3 compressed, also known as Dolby Digital */
#define ENCODING_AC3 5
/** Audio data format: E-AC-3 compressed, also known as Dolby Digital Plus or DD+ */
#define ENCODING_E_AC3 6
/** Audio data format: DTS compressed */
#define ENCODING_DTS 7
/** Audio data format: DTS HD compressed */
#define ENCODING_DTS_HD 8
/** Audio data format: MP3 compressed */
#define ENCODING_MP3 9
/** Audio data format: AAC LC compressed */
#define ENCODING_AAC_LC 10
/** Audio data format: AAC HE V1 compressed */
#define ENCODING_AAC_HE_V1 11
/** Audio data format: AAC HE V2 compressed */
#define ENCODING_AAC_HE_V2 12

/** Audio data format: compressed audio wrapped in PCM for HDMI
     * or S/PDIF passthrough.
     * For devices whose SDK version is less than {@link android.os.Build.VERSION_CODES#S}, the
     * channel mask of IEC61937 track must be {@link #CHANNEL_OUT_STEREO}.
     * Data should be written to the stream in a short[] array.
     * If the data is written in a byte[] array then there may be endian problems
     * on some platforms when converting to short internally.
     */
#define ENCODING_IEC61937 13
/** Audio data format: DOLBY TRUEHD compressed
 **/
#define ENCODING_DOLBY_TRUEHD 14
/** Audio data format: AAC ELD compressed */
#define ENCODING_AAC_ELD 15
/** Audio data format: AAC xHE compressed */
#define ENCODING_AAC_XHE 16
/** Audio data format: AC-4 sync frame transport format */
#define ENCODING_AC4 17
/** Audio data format: E-AC-3-JOC compressed
 * E-AC-3-JOC streams can be decoded by downstream devices supporting {@link #ENCODING_E_AC3}.
 * Use {@link #ENCODING_E_AC3} as the AudioTrack encoding when the downstream device
 * supports {@link #ENCODING_E_AC3} but not {@link #ENCODING_E_AC3_JOC}.
 **/
#define ENCODING_E_AC3_JOC 18
/** Audio data format: Dolby MAT (Metadata-enhanced Audio Transmission)
 * Dolby MAT bitstreams are used to transmit Dolby TrueHD, channel-based PCM, or PCM with
 * metadata (object audio) over HDMI (e.g. Dolby Atmos content).
 **/
#define ENCODING_DOLBY_MAT 19
/** Audio data format: OPUS compressed. */
#define ENCODING_OPUS 20

/** @hide
 * We do not permit legacy short array reads or writes for encodings
 * introduced after this threshold.
 */
#define ENCODING_LEGACY_SHORT_ARRAY_THRESHOLD = ENCODING_OPUS;

/** Audio data format: PCM 24 bit per sample packed as 3 bytes.
 *
 * The bytes are in little-endian order, so the least significant byte
 * comes first in the byte array.
 *
 * Not guaranteed to be supported by devices, may be emulated if not supported. */
#define ENCODING_PCM_24BIT_PACKED 21
/** Audio data format: PCM 32 bit per sample.
 * Not guaranteed to be supported by devices, may be emulated if not supported. */
#define ENCODING_PCM_32BIT 22

/** Audio data format: MPEG-H baseline profile, level 3 */
#define ENCODING_MPEGH_BL_L3 23
/** Audio data format: MPEG-H baseline profile, level 4 */
#define ENCODING_MPEGH_BL_L4 24
/** Audio data format: MPEG-H low complexity profile, level 3 */
#define ENCODING_MPEGH_LC_L3 25
/** Audio data format: MPEG-H low complexity profile, level 4 */
#define ENCODING_MPEGH_LC_L4 26
/** Audio data format: DTS UHD compressed */
#define ENCODING_DTS_UHD 27
/** Audio data format: DRA compressed */
#define ENCODING_DRA 28

extern void (*Java_com_ea_EAAudioCore_AndroidEAAudioCore_Init)(JNIEnv* env, jobject* obj, AudioTrack audioTrack, int i, int i2, int i3);
extern void (*Java_com_ea_EAAudioCore_AndroidEAAudioCore_Release)(JNIEnv* env);

bool AudioFormat__isPublicEncoding(int audioFormat)
{
    switch (audioFormat) {
        case ENCODING_PCM_16BIT:
        case ENCODING_PCM_8BIT:
        case ENCODING_PCM_FLOAT:
        case ENCODING_AC3:
        case ENCODING_E_AC3:
        case ENCODING_DTS:
        case ENCODING_DTS_HD:
        case ENCODING_MP3:
        case ENCODING_AAC_LC:
        case ENCODING_AAC_HE_V1:
        case ENCODING_AAC_HE_V2:
        case ENCODING_IEC61937:
        case ENCODING_DOLBY_TRUEHD:
        case ENCODING_AAC_ELD:
        case ENCODING_AAC_XHE:
        case ENCODING_AC4:
        case ENCODING_E_AC3_JOC:
        case ENCODING_DOLBY_MAT:
        case ENCODING_OPUS:
        case ENCODING_PCM_24BIT_PACKED:
        case ENCODING_PCM_32BIT:
        case ENCODING_MPEGH_BL_L3:
        case ENCODING_MPEGH_BL_L4:
        case ENCODING_MPEGH_LC_L3:
        case ENCODING_MPEGH_LC_L4:
        case ENCODING_DTS_UHD:
        case ENCODING_DRA:
            return true;
        default:
            return false;
    }
}


int AudioTrack__getMinBufferSize(int sampleRateInHz, int channelConfig, int audioFormat) {
    int channelCount = 0;
    switch(channelConfig) {
        case AudioFormat_CHANNEL_OUT_MONO:
        case AudioFormat_CHANNEL_CONFIGURATION_MONO:
            channelCount = 1;
            break;
        case AudioFormat_CHANNEL_OUT_STEREO:
        case AudioFormat_CHANNEL_CONFIGURATION_STEREO:
            channelCount = 2;
            break;
        default:
            fprintf(stderr, "Error: getMinBufferSize: Unsupported channel config.\n");
            return -1;
    }

    if (!AudioFormat__isPublicEncoding(audioFormat)) {
        fprintf(stderr, "Error: getMinBufferSize(): Invalid audio format.");
        return -1;
    }

    // sample rate, note these values are subject to change
    // Note: AudioFormat.SAMPLE_RATE_UNSPECIFIED is not allowed
    if ( (sampleRateInHz < SAMPLE_RATE_HZ_MIN) ||
         (sampleRateInHz > SAMPLE_RATE_HZ_MAX) ) {
        fprintf(stderr, "Error: getMinBufferSize(): %i Hz is not a supported sample rate.\n", sampleRateInHz);
        return -1;
    }

    //FIXME: properly calculate?
    //int size = native_get_min_buff_size(sampleRateInHz, channelCount, audioFormat);
    int size = channelCount * 32 * 4;
    if (size <= 0) {
        fprintf(stderr, "Error: getMinBufferSize(): error querying hardware");
        return -1;
    }
    else {
        return size;
    }
}


void _AudioTrack_flush() {
    printf("_AudioTrack_flush\n");
}

void _AudioTrack_release() {
    printf("_AudioTrack_release\n");
}

//int audio_port = 0;

int EAAudioCore_AudioTrack_write(int id, va_list args) {
    //printf("AudioTrack_write\n");
    // args: short* audioData, int offsetInShorts, int sizeInShorts
    // ignore

    /*jshort* buf = va_arg(args, jshort*);
    int32_t offs = va_arg(args, int32_t);
    int32_t len = va_arg(args, int32_t);

    printf("AudioTrack_write %i / %i\n", offs, len);

    return len;*/
    return 1024;
}

void EAAudioCore_AudioTrack_play(int id, va_list args) {
    //audio_port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, 512, 44100, SCE_AUDIO_OUT_MODE_STEREO);
    //printf("audio_port %x\n", audio_port);

    printf("AudioTrack_play\n");
    // ignore
}

void EAAudioCore_AudioTrack_stop(int id, va_list args) {
    printf("AudioTrack_stop\n");
    // ignore
}


void EAAudioCore__Startup() {
    Java_com_ea_EAAudioCore_AndroidEAAudioCore_Init(&jni, (void*)0x42424242, (void*)0x69696969, 65536, 2, 44100);
}

void EAAudioCore__Shutdown() {
    Java_com_ea_EAAudioCore_AndroidEAAudioCore_Release(&jni);
}
