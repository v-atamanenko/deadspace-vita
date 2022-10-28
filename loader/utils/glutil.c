/*
 * glutil.c
 *
 * OpenGL API initializer, related functions.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils/glutil.h"
#include "utils/dialog.h"
#include "sha1.h"

#include <stdio.h>
#include <malloc.h>
#include <psp2/kernel/sysmem.h>

#if GRAPHICS_API == GRAPHICS_API_PVR

static EGLDisplay dpy;
static EGLSurface surface;

#endif

void gl_preload() {
#if GRAPHICS_API == GRAPHICS_API_PVR
    PVRSRV_PSP2_APPHINT hint;
    int ret;
    ret = sceKernelLoadStartModule("vs0:sys/external/libfios2.suprx", 0, NULL, 0, NULL, NULL);
    debugPrintf("libfios2.suprx ret %#010x\n", ret);

    ret = sceKernelLoadStartModule("vs0:sys/external/libc.suprx", 0, NULL, 0, NULL, NULL);
    debugPrintf("libc.suprx ret %#010x\n", ret);

    ret = sceKernelLoadStartModule("app0:/module/libgpu_es4_ext.suprx", 0, NULL, 0, NULL, NULL);
    debugPrintf("libgpu_es4_ext.suprx ret %#010x\n", ret);

    ret = sceKernelLoadStartModule("app0:/module/libIMGEGL.suprx", 0, NULL, 0, NULL, NULL);
    debugPrintf("libIMGEGL.suprx ret %#010x\n", ret);

    PVRSRVInitializeAppHint(&hint);
    hint.bDisableAsyncTextureOp = 1;
    PVRSRVCreateVirtualAppHint(&hint);
#else
    if (!file_exists("ur0:/data/libshacccg.suprx")
        && !file_exists("ur0:/data/external/libshacccg.suprx")) {
        fatal_error("Error: libshacccg.suprx is not installed. Google \"ShaRKBR33D\" for quick installation.");
    }
#endif
}

void gl_init() {
#if GRAPHICS_API == GRAPHICS_API_PVR
    eglInit(EGL_DEFAULT_DISPLAY, 0);
#elif GRAPHICS_API == GRAPHICS_API_VITAGL
    vglInitExtended(0, 960, 544, 12 * 1024 * 1024, SCE_GXM_MULTISAMPLE_4X);
#endif
}

void gl_swap() {
#if GRAPHICS_API == GRAPHICS_API_PVR
    eglSwapBuffers(dpy, surface);
#elif GRAPHICS_API == GRAPHICS_API_VITAGL
    vglSwapBuffers(GL_FALSE);
#endif
}

#if GRAPHICS_API == GRAPHICS_API_VITAGL
void glShaderSourceHook(GLuint shader, GLsizei count, const GLchar **string,
                        const GLint *length) {
    uint32_t sha1[5];
    SHA1_CTX ctx;

    sha1_init(&ctx);
    sha1_update(&ctx, (uint8_t *)*string, *length);
    sha1_final(&ctx, (uint8_t *)sha1);

    char sha_name[64];
    snprintf(sha_name, sizeof(sha_name), "%08x%08x%08x%08x%08x",
             sha1[0], sha1[1], sha1[2], sha1[3], sha1[4]);

    char gxp_path[128];
    snprintf(gxp_path, sizeof(gxp_path), "%s/%s.gxp", GXP_PATH, sha_name);

    FILE *file = fopen(gxp_path, "rb");
    if (!file) {
        debugPrintf("[%i] Could not find %s\n", count, gxp_path);

        char glsl_path[128];
        snprintf(glsl_path, sizeof(glsl_path), "%s/%s.glsl",
                 GLSL_PATH, sha_name);

        file = fopen(glsl_path, "w");
        if (file) {
            fwrite(*string, 1, *length, file);
            fclose(file);
        }

        snprintf(gxp_path, sizeof(gxp_path), "%s/%s.gxp",
                 GXP_PATH, "9349e41c5fad90529f8aa627f5ad9ceeb0b75c7c");
        file = fopen(gxp_path, "rb");
    }

    if (file) {
        size_t shaderSize;
        void *shaderBuf;

        fseek(file, 0, SEEK_END);
        shaderSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        shaderBuf = malloc(shaderSize);
        fread(shaderBuf, 1, shaderSize, file);
        fclose(file);

        glShaderBinary(1, &shader, 0, shaderBuf, (int32_t)shaderSize);

        free(shaderBuf);
    }
}
#endif

#if GRAPHICS_API == GRAPHICS_API_PVR

int eglInit(EGLNativeDisplayType eglDisplay, EGLNativeWindowType eglWindow)
{
    EGLContext context;
    EGLConfig configs[2];
    EGLBoolean eRetStatus;
    EGLint major, minor;
    EGLint config_count;

    EGLint cfg_attribs[] = { EGL_BUFFER_SIZE,    EGL_DONT_CARE,
                             EGL_RED_SIZE,       8,
                             EGL_GREEN_SIZE,     8,
                             EGL_BLUE_SIZE,      8,
                             EGL_ALPHA_SIZE,		8,
                             EGL_DEPTH_SIZE,		16,
                             EGL_SAMPLES,		4,
                             EGL_NONE };

    dpy = eglGetDisplay(eglDisplay);

    eRetStatus = eglInitialize(dpy, &major, &minor);

    if (eRetStatus != EGL_TRUE)
    {
        printf("Error: eglInitialize\n");
    }

    eRetStatus = eglGetConfigs(dpy, configs, 2, &config_count);

    if (eRetStatus != EGL_TRUE)
    {
        printf("Error: eglGetConfigs\n");
    }

    eRetStatus = eglChooseConfig(dpy, cfg_attribs, configs, 2, &config_count);

    if (eRetStatus != EGL_TRUE)
    {
        printf("Error: eglChooseConfig\n");
    }

    Psp2NativeWindow win;
    win.type = PSP2_DRAWABLE_TYPE_WINDOW;
    if (sceKernelGetModel() == SCE_KERNEL_MODEL_VITATV) {
        win.windowSize = PSP2_WINDOW_1920X1088;
        scePowerSetGpuClockFrequency(222);
    }
    else {
        win.windowSize = PSP2_WINDOW_960X544;
    }
    win.numFlipBuffers = 2;
    win.flipChainThrdAffinity = SCE_KERNEL_CPU_MASK_USER_1;

    surface = eglCreateWindowSurface(dpy, configs[0], &win, NULL);

    if (surface == EGL_NO_SURFACE)
    {
        printf("Error: eglCreateWindowSurface\n");
    }

    context = eglCreateContext(dpy, configs[0], EGL_NO_CONTEXT, NULL);

    if (context == EGL_NO_CONTEXT)
    {
        printf("Error: eglCreateContext\n");
    }

    eRetStatus = eglMakeCurrent(dpy, surface, surface, context);

    if (eRetStatus != EGL_TRUE)
    {
        printf("Error: eglMakeCurrent\n");
    }

    return 0;
}

#endif
