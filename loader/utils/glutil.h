/*
 * glutil.h
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

#ifndef SOLOADER_GLUTIL_H
#define SOLOADER_GLUTIL_H

#include "config.h"
#include "utils/utils.h"

#if GRAPHICS_API == GRAPHICS_API_PVR
#include <psp2/power.h>
#include <psp2/appmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/vshbridge.h>

#include "gpu_es4/psp2_pvr_hint.h"

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/egl1_4.h>
#include <EGL/eglext.h>
#else
#include <vitaGL.h>
#endif

void gl_preload();
void gl_init();
void gl_swap();

#if GRAPHICS_API == GRAPHICS_API_PVR

int eglInit(EGLNativeDisplayType eglDisplay, EGLNativeWindowType eglWindow);

#elif GRAPHICS_API == GRAPHICS_API_VITAGL

void glShaderSourceHook(GLuint shader, GLsizei count,
                        const GLchar **string, const GLint *length);

#endif

#endif // SOLOADER_GLUTIL_H
