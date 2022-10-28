/*
 * default_dynlib.c
 *
 * Resolving dynamic imports of the .so.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

// Disable IDE complaints about _identifiers and global interfaces
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma ide diagnostic ignored "cppcoreguidelines-interfaces-global-init"

#include "default_dynlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <netdb.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <zlib.h>

#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <psp2/kernel/clib.h>
#include <dirent.h>
#include <utime.h>

#include "utils/glutil.h"
#include "libc_bridge.h"

#include "reimpl/env.h"
#include "reimpl/io.h"
#include "reimpl/log.h"
#include "reimpl/mem.h"
#include "reimpl/sys.h"
#include "reimpl/pthr.h"

#define PAGE_SIZE 4096
unsigned int __page_size = PAGE_SIZE;

extern void * _ZNSt9exceptionD2Ev;
extern void * _ZSt17__throw_bad_allocv;
extern void * _ZSt9terminatev;
extern void * _ZdaPv;
extern void * _ZdlPv;
extern void * _Znaj;
extern void * __cxa_allocate_exception;
extern void * __cxa_begin_catch;
extern void * __cxa_end_catch;
extern void * __cxa_free_exception;
extern void * __cxa_rethrow;
extern void * __cxa_throw;
extern void * __gxx_personality_v0;
extern void *_ZNSt8bad_castD1Ev;
extern void *_ZTISt8bad_cast;
extern void *_ZTISt9exception;
extern void *_ZTVN10__cxxabiv117__class_type_infoE;
extern void *_ZTVN10__cxxabiv120__si_class_type_infoE;
extern void *_ZTVN10__cxxabiv121__vmi_class_type_infoE;
extern void *_Znwj;
extern void *__aeabi_atexit;
extern void *__aeabi_memclr;
extern void *__aeabi_memcpy;
extern void *__aeabi_memmove;
extern void *__aeabi_memset;

extern void *__cxa_atexit;
extern void *__cxa_finalize;
extern void *__cxa_pure_virtual;
extern void *__cxa_call_unexpected;
extern void *__cxa_guard_acquire;
extern void *__cxa_guard_release;
extern void *__gnu_unwind_frame;
extern void *__stack_chk_fail;
extern void *__stack_chk_guard;
extern void *__dso_handle;

static FILE __sF_fake[0x100][3];

extern const char *BIONIC_ctype_;
extern const short *BIONIC_tolower_tab_;
extern const short *BIONIC_toupper_tab_;

#define __ATOMIC_INLINE__ static __inline__ __attribute__((always_inline))

__ATOMIC_INLINE__ int __atomic_swap(int new_value, volatile int *ptr)
{
    int old_value;
    do {
        old_value = *ptr;
    } while (__sync_val_compare_and_swap(ptr, old_value, new_value) != old_value);
    return old_value;
}

__ATOMIC_INLINE__ int __atomic_inc(volatile int *ptr)
{
    return __sync_fetch_and_add (ptr, 1);
}

__ATOMIC_INLINE__ int __atomic_dec(volatile int *ptr)
{
    return __sync_fetch_and_sub (ptr, 1);
}

__ATOMIC_INLINE__ int __atomic_cmpxchg(int old_value, int new_value, volatile int* ptr)
{
    /* We must return 0 on success */
    return __sync_val_compare_and_swap(ptr, old_value, new_value) != old_value;
}

int __gnu_Unwind_Find_exidx_tempwrap() { fprintf(stderr, "ret0d call!!! __gnu_Unwind_Find_exidx_tempwrap\n"); return 0; }
int poll_tempwrap() { fprintf(stderr, "ret0d call!!! poll_tempwrap\n"); return 0; }
int __cxa_type_match_tempwrap() { fprintf(stderr, "ret0d call!!! __cxa_type_match_tempwrap\n"); return 0; }
int __cxa_begin_cleanup_tempwrap() { fprintf(stderr, "ret0d call!!! __cxa_begin_cleanup_tempwrap\n"); return 0; }
int __isinf_tempwrap() { fprintf(stderr, "ret0d call!!! __isinf_tempwrap\n"); return 0; }
int prctl_tempwrap() { fprintf(stderr, "ret0d call!!! prctl_tempwrap\n"); return 0; }
int sysconf_tempwrap() { fprintf(stderr, "ret0d call!!! sysconf_tempwrap\n"); return 0; }
int setenv_ret0() { fprintf(stderr, "ret0d call!!! setenv_ret0\n"); return 0; }
int waitpid_tempwrap() { fprintf(stderr, "ret0d call!!! waitpid_tempwrap\n"); return 0; }
int execv_tempwrap() { fprintf(stderr, "ret0d call!!! execv_tempwrap\n"); return 0; }

#if GRAPHICS_API == GRAPHICS_API_VITAGL

int glDrawTexfOES_tempwrap() { fprintf(stderr, "ret0d call!!! glDrawTexfOES_tempwrap\n"); return 0; }
int glDrawTexsvOES_tempwrap() { fprintf(stderr, "ret0d call!!! glDrawTexsvOES_tempwrap\n"); return 0; }
int glIsRenderbufferOES_tempwrap() { fprintf(stderr, "ret0d call!!! glIsRenderbufferOES_tempwrap\n"); return 0; }
int glDrawTexxvOES_tempwrap() { fprintf(stderr, "ret0d call!!! glDrawTexxvOES_tempwrap\n"); return 0; }
int glTexGenxvOES_tempwrap() { fprintf(stderr, "ret0d call!!! glTexGenxvOES_tempwrap\n"); return 0; }
int glDrawTexivOES_tempwrap() { fprintf(stderr, "ret0d call!!! glDrawTexivOES_tempwrap\n"); return 0; }
int glTexGenfOES_tempwrap() { fprintf(stderr, "ret0d call!!! glTexGenfOES_tempwrap\n"); return 0; }
int glGetTexGenxvOES_tempwrap() { fprintf(stderr, "ret0d call!!! glGetTexGenxvOES_tempwrap\n"); return 0; }
int glGetTexGenivOES_tempwrap() { fprintf(stderr, "ret0d call!!! glGetTexGenivOES_tempwrap\n"); return 0; }
int glMatrixIndexPointerOES_tempwrap() { fprintf(stderr, "ret0d call!!! glMatrixIndexPointerOES_tempwrap\n"); return 0; }
int glTexGenfvOES_tempwrap() { fprintf(stderr, "ret0d call!!! glTexGenfvOES_tempwrap\n"); return 0; }
int glWeightPointerOES_tempwrap() { fprintf(stderr, "ret0d call!!! glWeightPointerOES_tempwrap\n"); return 0; }
int glCurrentPaletteMatrixOES_tempwrap() { fprintf(stderr, "ret0d call!!! glCurrentPaletteMatrixOES_tempwrap\n"); return 0; }
int glGetBufferPointervOES_tempwrap() { fprintf(stderr, "ret0d call!!! glGetBufferPointervOES_tempwrap\n"); return 0; }
int glEGLImageTargetRenderbufferStorageOES_tempwrap() { fprintf(stderr, "ret0d call!!! glEGLImageTargetRenderbufferStorageOES_tempwrap\n"); return 0; }
int glLoadPaletteFromModelViewMatrixOES_tempwrap() { fprintf(stderr, "ret0d call!!! glLoadPaletteFromModelViewMatrixOES_tempwrap\n"); return 0; }
int glDrawTexsOES_tempwrap() { fprintf(stderr, "ret0d call!!! glDrawTexsOES_tempwrap\n"); return 0; }
int glGetTexGenfvOES_tempwrap() { fprintf(stderr, "ret0d call!!! glGetTexGenfvOES_tempwrap\n"); return 0; }
int glDrawTexiOES_tempwrap() { fprintf(stderr, "ret0d call!!! glDrawTexiOES_tempwrap\n"); return 0; }
int glEGLImageTargetTexture2DOES_tempwrap() { fprintf(stderr, "ret0d call!!! glEGLImageTargetTexture2DOES_tempwrap\n"); return 0; }
int glQueryMatrixxOES_tempwrap() { fprintf(stderr, "ret0d call!!! glQueryMatrixxOES_tempwrap\n"); return 0; }
int glTexGenxOES_tempwrap() { fprintf(stderr, "ret0d call!!! glTexGenxOES_tempwrap\n"); return 0; }
int glDrawTexxOES_tempwrap() { fprintf(stderr, "ret0d call!!! glDrawTexxOES_tempwrap\n"); return 0; }
int glTexGenivOES_tempwrap() { fprintf(stderr, "ret0d call!!! glTexGenivOES_tempwrap\n"); return 0; }
int glDrawTexfvOES_tempwrap() { fprintf(stderr, "ret0d call!!! glDrawTexfvOES_tempwrap\n"); return 0; }
int glTexGeniOES_tempwrap() { fprintf(stderr, "ret0d call!!! glTexGeniOES_tempwrap\n"); return 0; }
int glGetPointerv_tempwrap() { fprintf(stderr, "ret0d call!!! glGetPointerv_tempwrap\n"); return 0; }
int glCompressedTexSubImage2D_tempwrap() { fprintf(stderr, "ret0d call!!! glCompressedTexSubImage2D_tempwrap\n"); return 0; }
int glSampleCoverage_tempwrap() { fprintf(stderr, "ret0d call!!! glSampleCoverage_tempwrap\n"); return 0; }
int glPointSizePointerOES_tempwrap() { fprintf(stderr, "ret0d call!!! glPointSizePointerOES_tempwrap\n"); return 0; }
int glLightf_tempwrap() { fprintf(stderr, "ret0d call!!! glLightf_tempwrap\n"); return 0; }
int glGetLightfv_tempwrap() { fprintf(stderr, "ret0d call!!! glGetLightfv_tempwrap\n"); return 0; }
int glPointParameterf_tempwrap() { fprintf(stderr, "ret0d call!!! glPointParameterf_tempwrap\n"); return 0; }
int glPointParameterxv_tempwrap() { fprintf(stderr, "ret0d call!!! glPointParameterxv_tempwrap\n"); return 0; }
int glMultiTexCoord4x_tempwrap() { fprintf(stderr, "ret0d call!!! glMultiTexCoord4x_tempwrap\n"); return 0; }
int glTexEnviv_tempwrap() { fprintf(stderr, "ret0d call!!! glTexEnviv_tempwrap\n"); return 0; }
int glGetLightxv_tempwrap() { fprintf(stderr, "ret0d call!!! glGetLightxv_tempwrap\n"); return 0; }
int glSampleCoveragex_tempwrap() { fprintf(stderr, "ret0d call!!! glSampleCoveragex_tempwrap\n"); return 0; }
int glMaterialx_tempwrap() { fprintf(stderr, "ret0d call!!! glMaterialx_tempwrap\n"); return 0; }
int glPointParameterfv_tempwrap() { fprintf(stderr, "ret0d call!!! glPointParameterfv_tempwrap\n"); return 0; }
int glTexParameterxv_tempwrap() { fprintf(stderr, "ret0d call!!! glTexParameterxv_tempwrap\n"); return 0; }
int glGetTexParameterfv_tempwrap() { fprintf(stderr, "ret0d call!!! glGetTexParameterfv_tempwrap\n"); return 0; }
int glCopyTexImage2D_tempwrap() { fprintf(stderr, "ret0d call!!! glCopyTexImage2D_tempwrap\n"); return 0; }
int glMaterialf_tempwrap() { fprintf(stderr, "ret0d call!!! glMaterialf_tempwrap\n"); return 0; }
int glGetTexEnvxv_tempwrap() { fprintf(stderr, "ret0d call!!! glGetTexEnvxv_tempwrap\n"); return 0; }
int glGetTexParameterxv_tempwrap() { fprintf(stderr, "ret0d call!!! glGetTexParameterxv_tempwrap\n"); return 0; }
int glGetMaterialxv_tempwrap() { fprintf(stderr, "ret0d call!!! glGetMaterialxv_tempwrap\n"); return 0; }
int glGetClipPlanef_tempwrap() { fprintf(stderr, "ret0d call!!! glGetClipPlanef_tempwrap\n"); return 0; }
int glGetFixedv_tempwrap() { fprintf(stderr, "ret0d call!!! glGetFixedv_tempwrap\n"); return 0; }
int glGetMaterialfv_tempwrap() { fprintf(stderr, "ret0d call!!! glGetMaterialfv_tempwrap\n"); return 0; }
int glNormal3x_tempwrap() { fprintf(stderr, "ret0d call!!! glNormal3x_tempwrap\n"); return 0; }
int glGetTexParameteriv_tempwrap() { fprintf(stderr, "ret0d call!!! glGetTexParameteriv_tempwrap\n"); return 0; }
int glLightModelf_tempwrap() { fprintf(stderr, "ret0d call!!! glLightModelf_tempwrap\n"); return 0; }
int glTexParameterfv_tempwrap() { fprintf(stderr, "ret0d call!!! glTexParameterfv_tempwrap\n"); return 0; }
int glIsBuffer_tempwrap() { fprintf(stderr, "ret0d call!!! glIsBuffer_tempwrap\n"); return 0; }
int glGetTexEnviv_tempwrap() { fprintf(stderr, "ret0d call!!! glGetTexEnviv_tempwrap\n"); return 0; }
int glGetClipPlanex_tempwrap() { fprintf(stderr, "ret0d call!!! glGetClipPlanex_tempwrap\n"); return 0; }
int glLightx_tempwrap() { fprintf(stderr, "ret0d call!!! glLightx_tempwrap\n"); return 0; }
int glMultiTexCoord4f_tempwrap() { fprintf(stderr, "ret0d call!!! glMultiTexCoord4f_tempwrap\n"); return 0; }
int glPointParameterx_tempwrap() { fprintf(stderr, "ret0d call!!! glPointParameterx_tempwrap\n"); return 0; }
int glGetTexEnvfv_tempwrap() { fprintf(stderr, "ret0d call!!! glGetTexEnvfv_tempwrap\n"); return 0; }
int glLightModelx_tempwrap() { fprintf(stderr, "ret0d call!!! glLightModelx_tempwrap\n"); return 0; }
int glLogicOp_tempwrap() { fprintf(stderr, "ret0d call!!! glLogicOp_tempwrap\n"); return 0; }
int glPixelStorei_tempwrap() { /*fprintf(stderr, "ret0d call!!! glPixelStorei_tempwrap\n");*/ return 0; }
int glCopyTexSubImage2D_tempwrap() { fprintf(stderr, "ret0d call!!! glCopyTexSubImage2D_tempwrap\n"); return 0; }
int glGetRenderbufferParameterivOES_tempwrap() { fprintf(stderr, "ret0d call!!! glGetRenderbufferParameterivOES_tempwrap\n"); return 0; }

#endif

char * strstr_soloader(char *__haystack,char *__needle) {
    //fprintf(stderr, "strstr(\"%s\", \"%s\")\n", __haystack, __needle);

    if (strcmp("appbundle:/", __needle) == 0) {
        //fprintf(stderr, "strstr: force NULL\n");
        return NULL;
    }

    return strstr(__haystack, __needle);
}


#ifdef GL_DEBUG
void glActiveTexture_dbg(GLenum texture) {
    fprintf(stderr, "[GL][DBG] glActiveTexture(0x%x)\n", (int)texture);
    return glActiveTexture(texture);
}

void glAlphaFunc_dbg(GLenum func, GLfloat ref) {
    fprintf(stderr, "[GL][DBG] glAlphaFunc(0x%x, 0x%x)\n", func, (int)ref);
    return glAlphaFunc(func, ref);
}

void glAlphaFuncx_dbg(GLenum func, GLfixed ref) {
    fprintf(stderr, "[GL][DBG] glAlphaFuncx(0x%x, 0x%x)\n", func, (int)ref);
    return glAlphaFuncx(func, ref);
}

void glAttachShader_dbg(GLuint prog, GLuint shad) {
    fprintf(stderr, "[GL][DBG] glAttachShader(0x%x, 0x%x)\n", (int)prog, (int)shad);
    return glAttachShader(prog, shad);
}

void glBindAttribLocation_dbg(GLuint program, GLuint index, const GLchar *name) {
    fprintf(stderr, "[GL][DBG] glBindAttribLocation(0x%x, 0x%x, 0x%x)\n", (int)program, (int)index, (int)name);
    return glBindAttribLocation(program, index, name);
}

void glBindBuffer_dbg(GLenum target, GLuint buffer) {
    fprintf(stderr, "[GL][DBG] glBindBuffer(0x%x, 0x%x)\n", (int)target, (int)buffer);
    return glBindBuffer(target, buffer);
}

void glBindFramebuffer_dbg(GLenum target, GLuint framebuffer) {
    fprintf(stderr, "[GL][DBG] glBindFramebuffer(0x%x, 0x%x)\n", (int)target, (int)framebuffer);
    return glBindFramebuffer(target, framebuffer);
}

void glBindRenderbuffer_dbg(GLenum target, GLuint renderbuffer) {
    fprintf(stderr, "[GL][DBG] glBindRenderbuffer(0x%x, 0x%x)\n", (int)target, (int)renderbuffer);
    return glBindRenderbuffer(target, renderbuffer);
}

void glBindTexture_dbg(GLenum target, GLuint texture) {
    fprintf(stderr, "[GL][DBG] glBindTexture(0x%x, 0x%x)\n", target, texture);
    return glBindTexture(target, texture);
}

void glBlendEquation_dbg(GLenum mode) {
    fprintf(stderr, "[GL][DBG] glBlendEquation(0x%x)\n", mode);
    return glBlendEquation(mode);
}

void glBlendEquationSeparate_dbg(GLenum modeRGB, GLenum modeAlpha) {
    fprintf(stderr, "[GL][DBG] glBlendEquationSeparate(0x%x, 0x%x)\n", modeRGB, modeAlpha);
    return glBlendEquationSeparate(modeRGB, modeAlpha);
}

void glBlendFunc_dbg(GLenum sfactor, GLenum dfactor) {
    fprintf(stderr, "[GL][DBG] glBlendFunc(0x%x, 0x%x)\n", sfactor, dfactor);
    return glBlendFunc(sfactor, dfactor);
}

void glBlendFuncSeparate_dbg(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    fprintf(stderr, "[GL][DBG] glBlendFuncSeparate(0x%x, 0x%x, 0x%x, 0x%x)\n", srcRGB, dstRGB, srcAlpha, dstAlpha);
    return glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void glBufferData_dbg(GLenum target, GLsizei size, const GLvoid *data, GLenum usage) {
    fprintf(stderr, "[GL][DBG] glBufferData(0x%x, 0x%x, 0x%x, 0x%x)\n", target, size, (int)data, usage);
    return glBufferData(target, size, data, usage);
}

void glBufferSubData_dbg(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
    fprintf(stderr, "[GL][DBG] glBufferSubData(0x%x, 0x%x, 0x%x, 0x%x)\n", target, offset, size, (int)data);
    return glBufferSubData(target, offset, size, data);
}

GLenum glCheckFramebufferStatus_dbg(GLenum target) {
    fprintf(stderr, "[GL][DBG] glCheckFramebufferStatus(0x%x)\n", target);
    return glCheckFramebufferStatus(target);
}

void glClear_dbg(GLbitfield mask) {
    fprintf(stderr, "[GL][DBG] glClear(0x%x)\n", mask);
    return glClear(mask);
}

void glClearColor_dbg(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    fprintf(stderr, "[GL][DBG] glClearColor(%f,%f,%f,%f)\n", red, green, blue, alpha);
    return glClearColor(red, green, blue, alpha);
}

void glClearColorx_dbg(GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha) {
    fprintf(stderr, "[GL][DBG] glClearColorx(0x%x,0x%x,0x%x,0x%x)\n", red, green, blue, alpha);
    return glClearColorx(red,green,blue,alpha);
}

void glClearDepthf_dbg(GLclampf depth) {
    fprintf(stderr, "[GL][DBG] glClearDepthf(%f)\n", depth);
    return glClearDepthf(depth);
}

void glClearDepthx_dbg(GLclampx depth) {
    fprintf(stderr, "[GL][DBG] glClearDepthx(0x%x)\n", depth);
    return glClearDepthx(depth);
}

void glClearStencil_dbg(GLint s) {
    fprintf(stderr, "[GL][DBG] glClearStencil(0x%x)\n", s);
    return glClearStencil(s);
}

void glClientActiveTexture_dbg(GLenum texture) {
    fprintf(stderr, "[GL][DBG] glClientActiveTexture(0x%x)\n", texture);
    return glClientActiveTexture(texture);
}

void glColor4f_dbg(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    fprintf(stderr, "[GL][DBG] glColor4f(%f, %f, %f, %f)\n",red,green,blue,alpha);
    return glColor4f(red,green,blue,alpha);
}

void glColor4ub_dbg(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
    fprintf(stderr, "[GL][DBG] glColor4ub(0x%x,0x%x,0x%x,0x%x)\n",red,green,blue,alpha);
    return glColor4ub(red,green,blue,alpha);
}

void glColor4x_dbg(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha) {
    fprintf(stderr, "[GL][DBG] glColor4x(0x%x,0x%x,0x%x,0x%x)\n",red,green,blue,alpha);
    return glColor4x(red,green,blue,alpha);
}

void glColorMask_dbg(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    fprintf(stderr, "[GL][DBG] glColorMask(0x%x,0x%x,0x%x,0x%x)\n",red,green,blue,alpha);
    return glColorMask(red,green,blue,alpha);
}

void glColorPointer_dbg(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    fprintf(stderr, "[GL][DBG] glColorPointer(0x%x,0x%x,0x%x,0x%x)\n",size,type,stride,(int)pointer);
    return glColorPointer(size,type,stride,pointer);
}

void glCompressedTexImage2D_dbg(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) {
    fprintf(stderr, "[GL][DBG] glCompressedTexImage2D(0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)\n",target, level, internalformat, width, height, border, imageSize, (int)data);
    return glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}

GLuint glCreateProgram_dbg(void) {
    fprintf(stderr, "[GL][DBG] glCreateProgram()\n");
    return glCreateProgram();
}

GLuint glCreateShader_dbg(GLenum shaderType) {
    fprintf(stderr, "[GL][DBG] glCreateShader(0x%x)\n", shaderType);
    return glCreateShader(shaderType);
}

void glCullFace_dbg(GLenum mode) {
    fprintf(stderr, "[GL][DBG] glCullFace(0x%x)\n", (int)mode);
    return glCullFace(mode);
}

void glDeleteBuffers_dbg(GLsizei n, const GLuint *gl_buffers) {
    fprintf(stderr, "[GL][DBG] glDeleteBuffers(0x%x, 0x%x)\n", n, (int)gl_buffers);
    return glDeleteBuffers(n, gl_buffers);
}

void glDeleteFramebuffers_dbg(GLsizei n, const GLuint *framebuffers) {
    fprintf(stderr, "[GL][DBG] glDeleteFramebuffers(0x%x, 0x%x)\n", n, (int)framebuffers);
    return glDeleteFramebuffers(n, framebuffers);
}


void glDeleteRenderbuffers_dbg(GLsizei n, const GLuint *renderbuffers) {
    fprintf(stderr, "[GL][DBG] glDeleteRenderbuffers(0x%x, 0x%x)\n", n, (int)renderbuffers);
    return glDeleteRenderbuffers(n, renderbuffers);
}

void glDeleteTextures_dbg(GLsizei n, const GLuint *textures) {
    fprintf(stderr, "[GL][DBG] glDeleteTextures(0x%x, 0x%x)\n", n, (int)textures);
    return glDeleteTextures(n, textures);
}

void glDepthFunc_dbg(GLenum func) {
    fprintf(stderr, "[GL][DBG] glDepthFunc(0x%x)\n", func);
    return glDepthFunc(func);
}

void glDepthMask_dbg(GLboolean flag) {
    fprintf(stderr, "[GL][DBG] glDepthMask(0x%x)\n", flag);
    return glDepthMask(flag);
}

void glDepthRangef_dbg(GLfloat nearVal, GLfloat farVal) {
    fprintf(stderr, "[GL][DBG] glDepthRangef(%f, %f)\n", nearVal, farVal);
    return glDepthRangef(nearVal, farVal);
}

void glDisable_dbg(GLenum cap) {
    fprintf(stderr, "[GL][DBG] glDisable(0x%x)\n", cap);
    return glDisable(cap);
}

void glDisableClientState_dbg(GLenum array) {
    fprintf(stderr, "[GL][DBG] glDisableClientState(0x%x)\n", array);
    return glDisableClientState(array);
}

void glDrawArrays_dbg(GLenum mode, GLint first, GLsizei count) {
    fprintf(stderr, "[GL][DBG] glDrawArrays(0x%x,0x%x,0x%x)\n", mode, first, count);
    return glDrawArrays(mode, first, count);
}

void glDrawElements_dbg(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
    fprintf(stderr, "[GL][DBG] glDrawElements(0x%x,0x%x,0x%x,0x%x)\n", mode, count, type, (int)indices);
    return glDrawElements(mode, count, type, indices);
}

void glEnable_dbg(GLenum cap) {
    fprintf(stderr, "[GL][DBG] glEnable(0x%x)\n", cap);
    return glEnable(cap);
}

void glEnableClientState_dbg(GLenum array) {
    fprintf(stderr, "[GL][DBG] glEnableClientState(0x%x)\n", array);
    return glEnableClientState(array);
}

void glEnableVertexAttribArray_dbg(GLuint index) {
    fprintf(stderr, "[GL][DBG] glEnableVertexAttribArray(0x%x)\n", index);
    return glEnableVertexAttribArray(index);
}

void glFinish_dbg(void) {
    fprintf(stderr, "[GL][DBG] glFinish()\n");
    return glFinish();
}

void glFlush_dbg(void) {
    fprintf(stderr, "[GL][DBG] glFlush()\n");
    return glFlush();
}

void glFogf_dbg(GLenum pname, GLfloat param) {
    fprintf(stderr, "[GL][DBG] glFogf(0x%x,%f)\n", pname, param);
    return glFogf(pname, param);
}

void glFogfv_dbg(GLenum pname, const GLfloat *params) {
    fprintf(stderr, "[GL][DBG] glFogfv(0x%x,0x%x)\n", pname, (int)params);
    return glFogfv(pname, params);
}

void glFramebufferRenderbuffer_dbg(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    fprintf(stderr, "[GL][DBG] glFramebufferRenderbuffer(0x%x,0x%x,0x%x,0x%x)\n", target, attachment, renderbuffertarget, renderbuffer);
    return glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

void glFramebufferTexture2D_dbg(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    fprintf(stderr, "[GL][DBG] glFramebufferTexture2D(0x%x,0x%x,0x%x,0x%x,0x%x)\n", target, attachment, textarget, texture, level);
    return glFramebufferTexture2D(target, attachment, textarget, texture, level);
}


void glFrontFace_dbg(GLenum mode) {
    fprintf(stderr, "[GL][DBG] glFrontFace(0x%x)\n", mode);
    return glFrontFace(mode);
}

void glFrustumf_dbg(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal) {
    fprintf(stderr, "[GL][DBG] glFrustumf(%f,%f,%f,%f,%f,%f)\n", left, right, bottom, top, nearVal, farVal);
    return glFrustumf(left, right, bottom, top, nearVal, farVal);
}

void glFrustumx_dbg(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed nearVal, GLfixed farVal) {
    fprintf(stderr, "[GL][DBG] glFrustumx(0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)\n", left, right, bottom, top, nearVal, farVal);
    return glFrustumx(left, right, bottom, top, nearVal, farVal);
}

void glGenBuffers_dbg(GLsizei n, GLuint *buffers) {
    fprintf(stderr, "[GL][DBG] glGenBuffers(0x%x,0x%x)\n", n, (int)buffers);
    return glGenBuffers(n, buffers);
}

void glGenFramebuffers_dbg(GLsizei n, GLuint *framebuffers) {
    fprintf(stderr, "[GL][DBG] glGenFramebuffers(0x%x,0x%x)\n", n, (int)framebuffers);
    return glGenFramebuffers(n, framebuffers);
}

void glGenRenderbuffers_dbg(GLsizei n, GLuint *renderbuffers) {
    fprintf(stderr, "[GL][DBG] glGenRenderbuffers(0x%x,0x%x)\n", n, (int)renderbuffers);
    return glGenRenderbuffers(n, renderbuffers);
}

void glGenTextures_dbg(GLsizei n, GLuint *textures) {
    fprintf(stderr, "[GL][DBG] glGenTextures(0x%x,0x%x)\n", n, (int)textures);
    return glGenTextures(n, textures);
}

void glGenerateMipmap_dbg(GLenum target) {
    fprintf(stderr, "[GL][DBG] glGenerateMipmap(0x%x)\n", target);
    return glGenerateMipmap(target);
}

void glGetBooleanv_dbg(GLenum pname, GLboolean *params) {
    fprintf(stderr, "[GL][DBG] glGetBooleanv(0x%x,0x%x)\n", pname, (int)params);
    return glGetBooleanv(pname, params);
}

void glGetBufferParameteriv_dbg(GLenum target, GLenum pname, GLint *params) {
    fprintf(stderr, "[GL][DBG] glGetBufferParameteriv(0x%x,0x%x,0x%x)\n", target, pname, (int)params);
    return glGetBufferParameteriv(target, pname, params);
}

GLenum glGetError_dbg(void) {
    fprintf(stderr, "[GL][DBG] glGetError()\n");
    return glGetError();
}

void glGetFloatv_dbg(GLenum pname, GLfloat *data) {
    fprintf(stderr, "[GL][DBG] glGetFloatv(0x%x,0x%x)\n", pname, (int)data);
    return glGetFloatv(pname, data);
}

void glGetFramebufferAttachmentParameteriv_dbg(GLenum target, GLenum attachment, GLenum pname, GLint *params) {
    fprintf(stderr, "[GL][DBG] glGetFramebufferAttachmentParameteriv(0x%x,0x%x,0x%x,0x%x)\n", target, attachment, pname, (int)params);
    return glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
}

void glGetIntegerv_dbg(GLenum pname, GLint *data) {
    fprintf(stderr, "[GL][DBG] glGetIntegerv(0x%x,0x%x)\n", pname, (int)data);
    return glGetIntegerv(pname, data);
}

void glGetProgramInfoLog_dbg(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog) {
    fprintf(stderr, "[GL][DBG] glGetProgramInfoLog(0x%x,0x%x,0x%x,0x%x)\n", program, maxLength, (int)length, (int)infoLog);
    return glGetProgramInfoLog(program, maxLength, length, infoLog);
}

void glGetProgramiv_dbg(GLuint program, GLenum pname, GLint *params) {
    fprintf(stderr, "[GL][DBG] glGetProgramiv(0x%x,0x%x,0x%x)\n", program, pname, (int)params);
    return glGetProgramiv(program, pname, params);
}

void glGetShaderInfoLog_dbg(GLuint handle, GLsizei maxLength, GLsizei *length, GLchar *infoLog) {
    fprintf(stderr, "[GL][DBG] glGetShaderInfoLog(0x%x,0x%x,0x%x,0x%x)\n", handle, maxLength, (int)length, (int)infoLog);
    return glGetShaderInfoLog(handle, maxLength, length, infoLog);
}

void glGetShaderiv_dbg(GLuint handle, GLenum pname, GLint *params) {
    fprintf(stderr, "[GL][DBG] glGetShaderiv(0x%x,0x%x,0x%x)\n", handle, pname, (int)params);
    return glGetShaderiv(handle, pname, params);
}

const GLubyte *glGetString_dbg(GLenum name) {
    fprintf(stderr, "[GL][DBG] glGetString(0x%x)\n", name);
    return glGetString(name);
}

GLint glGetUniformLocation_dbg(GLuint prog, const GLchar *name) {
    fprintf(stderr, "[GL][DBG] glGetUniformLocation(0x%x,0x%x)\n", prog, (int)name);
    return glGetUniformLocation(prog, name);
}

void glHint_dbg(GLenum target, GLenum mode) {
    fprintf(stderr, "[GL][DBG] glHint(0x%x,0x%x)\n", target, mode);
    return glHint(target, mode);
}

GLboolean glIsEnabled_dbg(GLenum cap) {
    fprintf(stderr, "[GL][DBG] glIsEnabled(0x%x)\n", cap);
    return glIsEnabled(cap);
}

GLboolean glIsFramebuffer_dbg(GLuint fb) {
    fprintf(stderr, "[GL][DBG] glIsFramebuffer(0x%x)\n", fb);
    return glIsFramebuffer(fb);
}

GLboolean glIsTexture_dbg(GLuint texture) {
    fprintf(stderr, "[GL][DBG] glIsTexture(0x%x)\n", texture);
    return glIsTexture(texture);
}

void glLightModelfv_dbg(GLenum pname, const GLfloat *params) {
    fprintf(stderr, "[GL][DBG] glLightModelfv(0x%x,0x%x)\n", pname, (int)params);
    return glLightModelfv(pname, params);
}

void glLightModelxv_dbg(GLenum pname, const GLfixed *params) {
    fprintf(stderr, "[GL][DBG] glLightModelxv(0x%x,0x%x)\n", pname, (int)params);
    return glLightModelxv(pname, params);
}

void glLightfv_dbg(GLenum light, GLenum pname, const GLfloat *params) {
    fprintf(stderr, "[GL][DBG] glLightfv(0x%x,0x%x,0x%x)\n", light, pname, (int)params);
    return glLightfv(light, pname, params);
}

void glLightxv_dbg(GLenum light, GLenum pname, const GLfixed *params) {
    fprintf(stderr, "[GL][DBG] glLightxv(0x%x,0x%x,0x%x)\n", light, pname, (int)params);
    return glLightxv(light, pname, params);
}

void glLineWidth_dbg(GLfloat width) {
    fprintf(stderr, "[GL][DBG] glLineWidth(%f)\n", width);
    return glLineWidth(width);
}

void glLinkProgram_dbg(GLuint progr) {
    fprintf(stderr, "[GL][DBG] glLinkProgram(0x%x)\n", progr);
    return glLinkProgram(progr);
}

void glLoadIdentity_dbg(void) {
    fprintf(stderr, "[GL][DBG] glLoadIdentity()\n");
    return glLoadIdentity();
}

void glLoadMatrixf_dbg(const GLfloat *m) {
    fprintf(stderr, "[GL][DBG] glLoadMatrixf(0x%x)\n",(int)m);
    return glLoadMatrixf(m);
}

void *glMapBuffer_dbg(GLenum target, GLbitfield access) {
    fprintf(stderr, "[GL][DBG] glMapBuffer(0x%x,0x%x)\n", target, access);
    return glMapBuffer(target, access);
}

void glMaterialfv_dbg(GLenum face, GLenum pname, const GLfloat *params) {
    fprintf(stderr, "[GL][DBG] glMaterialfv(0x%x,0x%x,0x%x)\n", face, pname, (int)params);
    return glMaterialfv(face, pname, params);
}

void glMaterialxv_dbg(GLenum face, GLenum pname, const GLfixed *params) {
    fprintf(stderr, "[GL][DBG] glMaterialxv(0x%x,0x%x,0x%x)\n", face, pname, (int)params);
    return glMaterialxv(face, pname, params);
}

void glMatrixMode_dbg(GLenum mode) {
    fprintf(stderr, "[GL][DBG] glMatrixMode(0x%x)\n", mode);
    return glMatrixMode(mode);
}

void glMultMatrixf_dbg(const GLfloat *m) {
    fprintf(stderr, "[GL][DBG] glMultMatrixf(0x%x)\n", (int)m);
    return glMultMatrixf(m);
}

void glMultMatrixx_dbg(const GLfixed *m) {
    fprintf(stderr, "[GL][DBG] glMultMatrixx(0x%x)\n", (int)m);
    return glMultMatrixx(m);
}

void glNormal3f_dbg(GLfloat x, GLfloat y, GLfloat z) {
    fprintf(stderr, "[GL][DBG] glNormal3f(%f,%f,%f)\n",x,y,z);
    return glNormal3f(x,y,z);
}

void glNormalPointer_dbg(GLenum type, GLsizei stride, const void *pointer) {
    fprintf(stderr, "[GL][DBG] glNormalPointer(0x%x,0x%x,0x%x)\n", type, stride, (int)pointer);
    return glNormalPointer(type, stride, pointer);
}

void glOrthof_dbg(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal) {
    fprintf(stderr, "[GL][DBG] glOrthof(%f, %f, %f, %f, %f, %f)\n", left, right, bottom, top, nearVal, farVal);
    return glOrthof(left, right, bottom, top, nearVal, farVal);
}

void glPointSize_dbg(GLfloat size) {
    fprintf(stderr, "[GL][DBG] glPointSize(%f)\n", size);
    return glPointSize(size);
}

void glPolygonOffset_dbg(GLfloat factor, GLfloat units) {
    fprintf(stderr, "[GL][DBG] glPolygonOffset(%f,%f)\n", factor, units);
    return glPolygonOffset(factor, units);
}

void glPopMatrix_dbg() {
    fprintf(stderr, "[GL][DBG] glPopMatrix()\n");
    return glPopMatrix();
}

void glPushMatrix_dbg() {
    fprintf(stderr, "[GL][DBG] glPushMatrix()\n");
    return glPushMatrix();
}

void glReadPixels_dbg(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *data) {
    fprintf(stderr, "[GL][DBG] glReadPixels(0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)\n", x, y, width, height, format, type, (int)data);
    return glReadPixels(x, y, width, height, format, type, data);
}

void glRenderbufferStorage_dbg(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    fprintf(stderr, "[GL][DBG] glRenderbufferStorage(0x%x,0x%x,0x%x,0x%x)\n", target, internalformat, width, height);
    return glRenderbufferStorage(target, internalformat, width, height);
}

void glRotatef_dbg(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    fprintf(stderr, "[GL][DBG] glRotatef(%f,%f,%f,%f)\n", angle, x, y, z);
    return glRotatef(angle, x, y, z);
}

void glRotatex_dbg(GLfixed angle, GLfixed x, GLfixed y, GLfixed z) {
    fprintf(stderr, "[GL][DBG] glRotatex(0x%x,0x%x,0x%x,0x%x)\n", angle, x, y, z);
    return glRotatex(angle, x, y, z);
}

void glScalef_dbg(GLfloat x, GLfloat y, GLfloat z) {
    fprintf(stderr, "[GL][DBG] glScalef(%f,%f,%f)\n", x, y, z);
    return glScalef(x, y, z);
}

void glScalex_dbg(GLfixed x, GLfixed y, GLfixed z) {
    fprintf(stderr, "[GL][DBG] glScalex(0x%x,0x%x,0x%x)\n", x, y, z);
    return glScalex(x, y, z);
}

void glScissor_dbg(GLint x, GLint y, GLsizei width, GLsizei height) {
    fprintf(stderr, "[GL][DBG] glScissor(0x%x,0x%x,0x%x,0x%x)\n", x, y, width, height);
    return glScissor(x, y, width, height);
}

void glShadeModel_dbg(GLenum mode) {
    fprintf(stderr, "[GL][DBG] glShadeModel(0x%x)\n", mode);
    return glShadeModel(mode);
}

void glShaderSourceHook_dbg(GLuint shader, GLsizei count, const GLchar **string,
                            const GLint *length) {
    fprintf(stderr, "[GL][DBG] glShaderSourceHook(0x%x,0x%x,0x%x,0x%x)\n", shader, count, (int)string, (int)length);
    return glShaderSourceHook(shader, count, string, length);
}

void glShaderSource_dbg(GLuint handle, GLsizei count, const GLchar *const *string, const GLint *length) {
    fprintf(stderr, "[GL][DBG] glShaderSource(0x%x,0x%x,0x%x,0x%x)\n", handle, count, (int)string, (int)length );
    return glShaderSource(handle, count, string, length);
}

void glStencilFunc_dbg(GLenum func, GLint ref, GLuint mask) {
    fprintf(stderr, "[GL][DBG] glStencilFunc(0x%x,0x%x,0x%x)\n", func, ref, mask);
    return glStencilFunc(func, ref, mask);
}

void glStencilMask_dbg(GLuint mask) {
    fprintf(stderr, "[GL][DBG] glStencilMask(0x%x)\n", mask);
    return glStencilMask(mask);
}

void glStencilOp_dbg(GLenum sfail, GLenum dpfail, GLenum dppass) {
    fprintf(stderr, "[GL][DBG] glStencilOp(0x%x,0x%x,0x%x)\n", sfail, dpfail, dppass);
    return glStencilOp(sfail, dpfail, dppass);
}

void glTexCoordPointer_dbg(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    fprintf(stderr, "[GL][DBG] glTexCoordPointer(0x%x,0x%x,0x%x,0x%x)\n", size, type, stride, (int)pointer);
    return glTexCoordPointer(size, type, stride, pointer);
}

void glTexEnvf_dbg(GLenum target, GLenum pname, GLfloat param) {
    fprintf(stderr, "[GL][DBG] glTexEnvf(0x%x,0x%x,0x%x)\n", target, pname, (int)param);
    return glTexEnvf(target, pname, param);
}

void glTexEnvfv_dbg(GLenum target, GLenum pname, GLfloat *param) {
    fprintf(stderr, "[GL][DBG] glTexEnvfv(0x%x,0x%x,0x%x)\n", target, pname, (int)param);
    return glTexEnvfv(target, pname, param);
}

void glTexEnvi_dbg(GLenum target, GLenum pname, GLint param) {
    fprintf(stderr, "[GL][DBG] glTexEnvi(0x%x,0x%x,0x%x)\n", target, pname, param);
    return glTexEnvi(target, pname, param);
}

void glTexEnvx_dbg(GLenum target, GLenum pname, GLfixed param) {
    fprintf(stderr, "[GL][DBG] glTexEnvx(0x%x,0x%x,0x%x)\n", target, pname, param);
    return glTexEnvx(target, pname, param);
}

void glTexEnvxv_dbg(GLenum target, GLenum pname, GLfixed *param) {
    fprintf(stderr, "[GL][DBG] glTexEnvxv(0x%x,0x%x,0x%x)\n", target, pname, (int)param);
    return glTexEnvxv(target, pname, param);
}

void glTexImage2D_dbg(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *data) {
    fprintf(stderr, "[GL][DBG] glTexImage2D(0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)\n", target, level, internalFormat, width, height, border, format, type, (int)data);
    return glTexImage2D(target, level, internalFormat, width, height, border, format, type, data);
}

void glTexParameterf_dbg(GLenum target, GLenum pname, GLfloat param) {
    fprintf(stderr, "[GL][DBG] glTexParameterf(0x%x, 0x%x, %f)\n", target, pname, param);
    return glTexParameterf(target, pname, param);
}

void glTexParameteri_dbg(GLenum target, GLenum pname, GLint param) {
    fprintf(stderr, "[GL][DBG] glTexParameteri(0x%x, 0x%x, 0x%x)\n", target, pname, param);
    return glTexParameteri(target, pname, param);
}

void glTexSubImage2D_dbg(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
    fprintf(stderr, "[GL][DBG] glTexSubImage2D(0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)\n", target, level, xoffset, yoffset, width, height, format, type, (int)pixels);
    return glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void glTranslatef_dbg(GLfloat x, GLfloat y, GLfloat z) {
    fprintf(stderr, "[GL][DBG] glTranslatef(%f,%f,%f)\n", x, y, z);
    return glTranslatef(x, y, z);
}

void glTranslatex_dbg(GLfixed x, GLfixed y, GLfixed z) {
    fprintf(stderr, "[GL][DBG] glTranslatex(0x%x,0x%x,0x%x)\n", x, y, z);
    return glTranslatex(x, y, z);
}

void glUniform1f_dbg(GLint location, GLfloat v0) {
    fprintf(stderr, "[GL][DBG] glUniform1f(0x%x, %f)\n", location, v0);
    return glUniform1f(location, v0);
}

void glUniform1i_dbg(GLint location, GLint v0) {
    fprintf(stderr, "[GL][DBG] glUniform1i(0x%x, 0x%x)\n", location, v0);
    return glUniform1i(location, v0);
}

void glUniform2f_dbg(GLint location, GLfloat v0, GLfloat v1) {
    fprintf(stderr, "[GL][DBG] glUniform2f(0x%x, %f, %f)\n", location, v0, v1);
    return glUniform2f(location, v0, v1);
}

void glUniform4f_dbg(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    fprintf(stderr, "[GL][DBG] glUniform4f(0x%x, %f, %f, %f, %f)\n", location, v0, v1, v2, v3);
    return glUniform4f(location, v0, v1, v2, v3);
}

GLboolean glUnmapBuffer_dbg(GLenum target) {
    fprintf(stderr, "[GL][DBG] glUnmapBuffer(0x%x)\n", target);
    return glUnmapBuffer(target);
}

void glUseProgram_dbg(GLuint program) {
    fprintf(stderr, "[GL][DBG] glUseProgram(0x%x)\n", program);
    return glUseProgram(program);
}

void glVertexAttribPointer_dbg(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
    fprintf(stderr, "[GL][DBG] glVertexAttribPointer(0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)\n", index, size, type, normalized, stride, (int)pointer);
    return glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

void glVertexPointer_dbg(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    fprintf(stderr, "[GL][DBG] glVertexPointer(0x%x,0x%x,0x%x,0x%x)\n", size, type, stride, (int)pointer);
    return glVertexPointer(size, type, stride, pointer);
}

void glViewport_dbg(GLint x, GLint y, GLsizei width, GLsizei height) {
    fprintf(stderr, "[GL][DBG] glViewport(0x%x,0x%x,0x%x,0x%x)\n", x, y, width, height);
    return glViewport(x, y, width, height);
}

#endif

#if GRAPHICS_API == GRAPHICS_API_PVR
#include "utils/sfp2hfp.h"
#endif

// NOLINT(cppcoreguidelines-interfaces-global-init)
so_default_dynlib default_dynlib[] = {
        { "__aeabi_atexit", (uintptr_t)&__aeabi_atexit },
        { "__android_log_print", (uintptr_t)&android_log_print },
        { "__android_log_vprint", (uintptr_t)&android_log_vprint },
        { "__android_log_write", (uintptr_t)&android_log_write },
        { "__atomic_cmpxchg", (uintptr_t)&__atomic_cmpxchg },
        { "__atomic_dec", (uintptr_t)&__atomic_dec },
        { "__atomic_inc", (uintptr_t)&__atomic_inc},
        { "__atomic_swap", (uintptr_t)&__atomic_swap},
        { "__cxa_begin_cleanup", (uintptr_t)&__cxa_begin_cleanup_tempwrap },
        { "__cxa_call_unexpected", (uintptr_t)&__cxa_call_unexpected},
        { "__cxa_guard_acquire", (uintptr_t)&__cxa_guard_acquire},
        { "__cxa_guard_release", (uintptr_t)&__cxa_guard_release},
        { "__cxa_pure_virtual", (uintptr_t)&__cxa_pure_virtual },
        { "__cxa_type_match", (uintptr_t)&__cxa_type_match_tempwrap },
        { "__errno", (uintptr_t)&__errno },
        { "__gnu_Unwind_Find_exidx", (uintptr_t)&__gnu_Unwind_Find_exidx_tempwrap },
        { "__isinf", (uintptr_t)&__isinf_tempwrap },
        { "__stack_chk_fail", (uintptr_t)&__stack_chk_fail },
        { "__stack_chk_guard", (uintptr_t)&__stack_chk_guard },
        { "__page_size", (uintptr_t)&__page_size },
        { "_ctype_", (uintptr_t)&BIONIC_ctype_ },
        { "_toupper_tab_", (uintptr_t)&BIONIC_toupper_tab_ },
        { "_tolower_tab_", (uintptr_t)&BIONIC_tolower_tab_ },
        { "abort", (uintptr_t)&abort },
        { "accept", (uintptr_t)&accept },
        { "acos", (uintptr_t)&acos },
        { "acosf", (uintptr_t)&acosf },
        { "asinf", (uintptr_t)&asinf },
        { "atan", (uintptr_t)&atan },
        { "atan2", (uintptr_t)&atan2 },
        { "atan2f", (uintptr_t)&atan2f },
        { "atoi", (uintptr_t)&atoi },
        { "bind", (uintptr_t)&bind},
        { "calloc", (uintptr_t)&calloc },
        { "ceil", (uintptr_t)&ceil },
        { "ceilf", (uintptr_t)&ceilf },
        { "chdir", (uintptr_t)&chdir },
        { "chmod", (uintptr_t)&chmod },
        { "clock", (uintptr_t)&clock},
        { "clock_gettime", (uintptr_t)&clock_gettime_soloader },
        { "close", (uintptr_t)&close_soloader },
        { "closedir", (uintptr_t)&closedir_soloader},
        { "connect", (uintptr_t)&connect},
        { "cos", (uintptr_t)&cos },
        { "cosf", (uintptr_t)&cosf },
        { "execv", (uintptr_t)&execv_tempwrap },
        { "exit", (uintptr_t)&exit },
        { "exp", (uintptr_t)&exp },
        { "expf", (uintptr_t)&expf },
        { "fclose", (uintptr_t)&fclose },
        { "fcntl", (uintptr_t)&fcntl_soloader },
        { "fflush", (uintptr_t)&fflush },
        { "fgetc", (uintptr_t)&fgetc },
        { "fgets", (uintptr_t)&fgets },
        { "floor", (uintptr_t)&floor },
        { "floorf", (uintptr_t)&floorf },
        { "fmodf", (uintptr_t)&fmodf },
        { "fopen", (uintptr_t)&fopen_soloader },
        { "fork", (uintptr_t)&fork },
        { "fprintf", (uintptr_t)&fprintf },
        { "fputs", (uintptr_t)&fputs },
        { "fread", (uintptr_t)&fread },
        { "free", (uintptr_t)&free },
        { "freeaddrinfo", (uintptr_t)&freeaddrinfo},
        { "fseek", (uintptr_t)&fseek },
        { "fstat", (uintptr_t)&fstat_soloader },
        { "fsync", (uintptr_t)&fsync_soloader},
        { "ftell", (uintptr_t)&ftell },
        { "ftruncate", (uintptr_t)&ftruncate},
        { "fwide", (uintptr_t)&fwide},
        { "fwrite", (uintptr_t)&fwrite },
        { "getaddrinfo", (uintptr_t)&getaddrinfo },
        { "getcwd", (uintptr_t)&getcwd },
        { "getenv", (uintptr_t)&getenv_soloader },
        { "getpeername", (uintptr_t)&getpeername},
        { "getsockname", (uintptr_t)&getsockname},
        { "getsockopt", (uintptr_t)&getsockopt},
        { "gettimeofday", (uintptr_t)&gettimeofday },

#if GRAPHICS_API == GRAPHICS_API_PVR
        { "glActiveTexture", (uintptr_t)&glActiveTexture},
        { "glAlphaFunc", (uintptr_t)&glAlphaFunc_sfp},
        { "glAlphaFuncx", (uintptr_t)&glAlphaFuncx},
        { "glBindBuffer", (uintptr_t)&glBindBuffer },
        { "glBindFramebufferOES", (uintptr_t)&glBindFramebufferOES },
        { "glBindRenderbufferOES", (uintptr_t)&glBindRenderbufferOES },
        { "glBindTexture", (uintptr_t)&glBindTexture },
        { "glBlendEquationOES", (uintptr_t)&glBlendEquationOES },
        { "glBlendEquationSeparateOES", (uintptr_t)&glBlendEquationSeparateOES },
        { "glBlendFunc", (uintptr_t)&glBlendFunc },
        { "glBlendFuncSeparateOES", (uintptr_t)&glBlendFuncSeparateOES },
        { "glBufferData", (uintptr_t)&glBufferData },
        { "glBufferSubData", (uintptr_t)&glBufferSubData},
        { "glCheckFramebufferStatusOES", (uintptr_t)&glCheckFramebufferStatusOES },
        { "glClear", (uintptr_t)&glClear },
        { "glClearColor", (uintptr_t)&glClearColor_sfp },
        { "glClearColorx", (uintptr_t)&glClearColorx},
        { "glClearDepthf", (uintptr_t)&glClearDepthf_sfp },
        { "glClearDepthx", (uintptr_t)&glClearDepthx},
        { "glClearStencil", (uintptr_t)&glClearStencil},
        { "glClientActiveTexture", (uintptr_t)&glClientActiveTexture},
        { "glClipPlanef", (uintptr_t)&glClipPlanef },
        { "glClipPlanex", (uintptr_t)&glClipPlanex },
        { "glColor4f", (uintptr_t)&glColor4f_sfp },
        { "glColor4ub", (uintptr_t)&glColor4ub},
        { "glColor4x", (uintptr_t)&glColor4x},
        { "glColorMask", (uintptr_t)&glColorMask},
        { "glColorPointer", (uintptr_t)&glColorPointer},
        { "glCompressedTexImage2D", (uintptr_t)&glCompressedTexImage2D },
        { "glCompressedTexSubImage2D", (uintptr_t)&glCompressedTexSubImage2D },
        { "glCopyTexImage2D", (uintptr_t)&glCopyTexImage2D },
        { "glCopyTexSubImage2D", (uintptr_t)&glCopyTexSubImage2D },
        { "glCullFace", (uintptr_t)&glCullFace},
        { "glCurrentPaletteMatrixOES", (uintptr_t)&glCurrentPaletteMatrixOES },
        { "glDeleteBuffers", (uintptr_t)&glDeleteBuffers },
        { "glDeleteFramebuffersOES", (uintptr_t)&glDeleteFramebuffersOES },
        { "glDeleteRenderbuffersOES", (uintptr_t)&glDeleteRenderbuffersOES },
        { "glDeleteTextures", (uintptr_t)&glDeleteTextures },
        { "glDepthFunc", (uintptr_t)&glDepthFunc },
        { "glDepthMask", (uintptr_t)&glDepthMask },
        { "glDepthRangef", (uintptr_t) &glDepthRangef_sfp },
        { "glDepthRangex", (uintptr_t)&glDepthRangex },
        { "glDisable", (uintptr_t)&glDisable },
        { "glDisableClientState", (uintptr_t)&glDisableClientState},
        { "glDrawArrays", (uintptr_t)&glDrawArrays},
        { "glDrawElements", (uintptr_t)&glDrawElements },
        { "glDrawTexfOES", (uintptr_t)&glDrawTexfOES_sfp },
        { "glDrawTexfvOES", (uintptr_t)&glDrawTexfvOES },
        { "glDrawTexiOES", (uintptr_t)&glDrawTexiOES },
        { "glDrawTexivOES", (uintptr_t)&glDrawTexivOES },
        { "glDrawTexsOES", (uintptr_t)&glDrawTexsOES },
        { "glDrawTexsvOES", (uintptr_t)&glDrawTexsvOES },
        { "glDrawTexxOES", (uintptr_t)&glDrawTexxOES },
        { "glDrawTexxvOES", (uintptr_t)&glDrawTexxvOES },
        { "glEGLImageTargetRenderbufferStorageOES", (uintptr_t)&glEGLImageTargetRenderbufferStorageOES },
        { "glEGLImageTargetTexture2DOES", (uintptr_t)&glEGLImageTargetTexture2DOES },
        { "glEnable", (uintptr_t)&glEnable },
        { "glEnableClientState", (uintptr_t)&glEnableClientState},
        { "glFinish", (uintptr_t)&glFinish},
        { "glFlush", (uintptr_t)&glFlush},
        { "glFogf", (uintptr_t)&glFogf_sfp},
        { "glFogfv", (uintptr_t)&glFogfv},
        { "glFogx", (uintptr_t)&glFogx },
        { "glFogxv", (uintptr_t)&glFogxv },
        { "glFramebufferRenderbufferOES", (uintptr_t)&glFramebufferRenderbufferOES },
        { "glFramebufferTexture2DOES", (uintptr_t)&glFramebufferTexture2DOES },
        { "glFrontFace", (uintptr_t)&glFrontFace},
        { "glFrustumf", (uintptr_t)&glFrustumf_sfp },
        { "glFrustumx", (uintptr_t)&glFrustumx},
        { "glGenBuffers", (uintptr_t)&glGenBuffers },
        { "glGenFramebuffersOES", (uintptr_t)&glGenFramebuffersOES },
        { "glGenRenderbuffersOES", (uintptr_t)&glGenRenderbuffersOES },
        { "glGenTextures", (uintptr_t)&glGenTextures },
        { "glGenerateMipmapOES", (uintptr_t)&glGenerateMipmapOES },
        { "glGetBooleanv", (uintptr_t)&glGetBooleanv},
        { "glGetBufferParameteriv", (uintptr_t)&glGetBufferParameteriv},
        { "glGetBufferPointervOES", (uintptr_t)&glGetBufferPointervOES },
        { "glGetClipPlanef", (uintptr_t)&glGetClipPlanef },
        { "glGetClipPlanex", (uintptr_t)&glGetClipPlanex },
        { "glGetError", (uintptr_t)&glGetError },
        { "glGetFixedv", (uintptr_t)&glGetFixedv },
        { "glGetFloatv", (uintptr_t)&glGetFloatv},
        { "glGetFramebufferAttachmentParameterivOES", (uintptr_t)&glGetFramebufferAttachmentParameterivOES },
        { "glGetIntegerv", (uintptr_t)&glGetIntegerv},
        { "glGetLightfv", (uintptr_t)&glGetLightfv },
        { "glGetLightxv", (uintptr_t)&glGetLightxv },
        { "glGetMaterialfv", (uintptr_t)&glGetMaterialfv },
        { "glGetMaterialxv", (uintptr_t)&glGetMaterialxv },
        { "glGetPointerv", (uintptr_t)&glGetPointerv },
        { "glGetRenderbufferParameterivOES", (uintptr_t)&glGetRenderbufferParameterivOES },
        { "glGetString", (uintptr_t)&glGetString},
        { "glGetTexEnvfv", (uintptr_t)&glGetTexEnvfv },
        { "glGetTexEnviv", (uintptr_t)&glGetTexEnviv },
        { "glGetTexEnvxv", (uintptr_t)&glGetTexEnvxv },
        { "glGetTexGenfvOES", (uintptr_t)&glGetTexGenfvOES },
        { "glGetTexGenivOES", (uintptr_t)&glGetTexGenivOES },
        { "glGetTexGenxvOES", (uintptr_t)&glGetTexGenxvOES },
        { "glGetTexParameterfv", (uintptr_t)&glGetTexParameterfv },
        { "glGetTexParameteriv", (uintptr_t)&glGetTexParameteriv },
        { "glGetTexParameterxv", (uintptr_t)&glGetTexParameterxv },
        { "glHint", (uintptr_t)&glHint},
        { "glIsBuffer", (uintptr_t)&glIsBuffer },
        { "glIsEnabled", (uintptr_t)&glIsEnabled},
        { "glIsFramebufferOES", (uintptr_t)&glIsFramebufferOES },
        { "glIsRenderbufferOES", (uintptr_t)&glIsRenderbufferOES },
        { "glIsTexture", (uintptr_t)&glIsTexture},
        { "glLightModelf", (uintptr_t)&glLightModelf_sfp },
        { "glLightModelfv", (uintptr_t)&glLightModelfv},
        { "glLightModelx", (uintptr_t)&glLightModelx },
        { "glLightModelxv", (uintptr_t)&glLightModelxv},
        { "glLightf", (uintptr_t)&glLightf_sfp },
        { "glLightfv", (uintptr_t)&glLightfv},
        { "glLightx", (uintptr_t)&glLightx },
        { "glLightxv", (uintptr_t)&glLightxv},
        { "glLineWidth", (uintptr_t)&glLineWidth_sfp},
        { "glLineWidthx", (uintptr_t)&glLineWidthx },
        { "glLoadIdentity", (uintptr_t)&glLoadIdentity},
        { "glLoadMatrixf", (uintptr_t)&glLoadMatrixf},
        { "glLoadMatrixx", (uintptr_t)&glLoadMatrixx },
        { "glLoadPaletteFromModelViewMatrixOES", (uintptr_t)&glLoadPaletteFromModelViewMatrixOES },
        { "glLogicOp", (uintptr_t)&glLogicOp },
        { "glMapBufferOES", (uintptr_t)&glMapBufferOES },
        { "glMaterialf", (uintptr_t)&glMaterialf_sfp },
        { "glMaterialfv", (uintptr_t)&glMaterialfv},
        { "glMaterialx", (uintptr_t)&glMaterialx },
        { "glMaterialxv", (uintptr_t)&glMaterialxv},
        { "glMatrixIndexPointerOES", (uintptr_t)&glMatrixIndexPointerOES },
        { "glMatrixMode", (uintptr_t)&glMatrixMode},
        { "glMultMatrixf", (uintptr_t)&glMultMatrixf},
        { "glMultMatrixx", (uintptr_t)&glMultMatrixx},
        { "glMultiTexCoord4f", (uintptr_t)&glMultiTexCoord4f_sfp },
        { "glMultiTexCoord4x", (uintptr_t)&glMultiTexCoord4x },
        { "glNormal3f", (uintptr_t)&glNormal3f_sfp },
        { "glNormal3x", (uintptr_t)&glNormal3x },
        { "glNormalPointer", (uintptr_t)&glNormalPointer},
        { "glOrthof", (uintptr_t)&glOrthof_sfp },
        { "glOrthox", (uintptr_t)&glOrthox },
        { "glPixelStorei", (uintptr_t)&glPixelStorei},
        { "glPointParameterf", (uintptr_t)&glPointParameterf_sfp },
        { "glPointParameterfv", (uintptr_t)&glPointParameterfv },
        { "glPointParameterx", (uintptr_t)&glPointParameterx },
        { "glPointParameterxv", (uintptr_t)&glPointParameterxv },
        { "glPointSize", (uintptr_t)&glPointSize_sfp},
        { "glPointSizePointerOES", (uintptr_t)&glPointSizePointerOES },
        { "glPointSizex", (uintptr_t)&glPointSizex },
        { "glPolygonOffset", (uintptr_t)&glPolygonOffset_sfp},
        { "glPolygonOffsetx", (uintptr_t)&glPolygonOffsetx },
        { "glPopMatrix", (uintptr_t)&glPopMatrix},
        { "glPushMatrix", (uintptr_t)&glPushMatrix},
        { "glQueryMatrixxOES", (uintptr_t)&glQueryMatrixxOES },
        { "glReadPixels", (uintptr_t)&glReadPixels},
        { "glRenderbufferStorageOES", (uintptr_t)&glRenderbufferStorageOES },
        { "glRotatef", (uintptr_t)&glRotatef_sfp },
        { "glRotatex", (uintptr_t)&glRotatex},
        { "glSampleCoverage", (uintptr_t)&glSampleCoverage },
        { "glSampleCoveragex", (uintptr_t)&glSampleCoveragex },
        { "glScalef", (uintptr_t)&glScalef_sfp },
        { "glScalex", (uintptr_t)&glScalex},
        { "glScissor", (uintptr_t)&glScissor},
        { "glShadeModel", (uintptr_t)&glShadeModel},
        { "glStencilFunc", (uintptr_t)&glStencilFunc},
        { "glStencilMask", (uintptr_t)&glStencilMask},
        { "glStencilOp", (uintptr_t)&glStencilOp},
        { "glTexCoordPointer", (uintptr_t)&glTexCoordPointer},
        { "glTexEnvf", (uintptr_t)&glTexEnvf_sfp},
        { "glTexEnvfv", (uintptr_t)&glTexEnvfv},
        { "glTexEnvi", (uintptr_t)&glTexEnvi},
        { "glTexEnviv", (uintptr_t)&glTexEnviv },
        { "glTexEnvx", (uintptr_t)&glTexEnvx},
        { "glTexEnvxv", (uintptr_t)&glTexEnvxv},
        { "glTexGenfOES", (uintptr_t)&glTexGenfOES_sfp },
        { "glTexGenfvOES", (uintptr_t)&glTexGenfvOES },
        { "glTexGeniOES", (uintptr_t)&glTexGeniOES },
        { "glTexGenivOES", (uintptr_t)&glTexGenivOES },
        { "glTexGenxOES", (uintptr_t)&glTexGenxOES },
        { "glTexGenxvOES", (uintptr_t)&glTexGenxvOES },
        { "glTexImage2D", (uintptr_t)&glTexImage2D },
        { "glTexParameterf", (uintptr_t)&glTexParameterf_sfp },
        { "glTexParameterfv", (uintptr_t)&glTexParameterfv },
        { "glTexParameteri", (uintptr_t)&glTexParameteri },
        { "glTexParameteriv", (uintptr_t)&glTexParameteriv },
        { "glTexParameterx", (uintptr_t)&glTexParameterx },
        { "glTexParameterxv", (uintptr_t)&glTexParameterxv },
        { "glTexSubImage2D", (uintptr_t)&glTexSubImage2D },
        { "glTranslatef", (uintptr_t)&glTranslatef_sfp },
        { "glTranslatex", (uintptr_t)&glTranslatex},
        { "glUnmapBufferOES", (uintptr_t)&glUnmapBufferOES },
        { "glVertexPointer", (uintptr_t)&glVertexPointer},
        { "glViewport", (uintptr_t)&glViewport },
        { "glWeightPointerOES", (uintptr_t)&glWeightPointerOES },
#elif GRAPHICS_API == GRAPHICS_API_VITAGL
        { "glActiveTexture", (uintptr_t)&glActiveTexture},
        { "glAlphaFunc", (uintptr_t)&glAlphaFunc},
        { "glAlphaFuncx", (uintptr_t)&glAlphaFuncx},
        { "glBindBuffer", (uintptr_t)&glBindBuffer },
        { "glBindFramebufferOES", (uintptr_t)&glBindFramebuffer },
        { "glBindRenderbufferOES", (uintptr_t)&glBindRenderbuffer },
        { "glBindTexture", (uintptr_t)&glBindTexture },
        { "glBlendEquationOES", (uintptr_t)&glBlendEquation },
        { "glBlendEquationSeparateOES", (uintptr_t)&glBlendEquationSeparate },
        { "glBlendFunc", (uintptr_t)&glBlendFunc },
        { "glBlendFuncSeparateOES", (uintptr_t)&glBlendFuncSeparate },
        { "glBufferData", (uintptr_t)&glBufferData },
        { "glBufferSubData", (uintptr_t)&glBufferSubData},
        { "glCheckFramebufferStatusOES", (uintptr_t)&glCheckFramebufferStatus },
        { "glClear", (uintptr_t)&glClear },
        { "glClearColor", (uintptr_t)&glClearColor },
        { "glClearColorx", (uintptr_t)&glClearColorx},
        { "glClearDepthf", (uintptr_t)&glClearDepthf },
        { "glClearDepthx", (uintptr_t)&glClearDepthx},
        { "glClearStencil", (uintptr_t)&glClearStencil},
        { "glClientActiveTexture", (uintptr_t)&glClientActiveTexture},
        { "glClipPlanef", (uintptr_t)&glClipPlanef },
        { "glClipPlanex", (uintptr_t)&glClipPlanex },
        { "glColor4f", (uintptr_t)&glColor4f },
        { "glColor4ub", (uintptr_t)&glColor4ub},
        { "glColor4x", (uintptr_t)&glColor4x},
        { "glColorMask", (uintptr_t)&glColorMask},
        { "glColorPointer", (uintptr_t)&glColorPointer},
        { "glCompressedTexImage2D", (uintptr_t)&glCompressedTexImage2D },
        { "glCompressedTexSubImage2D", (uintptr_t)&glCompressedTexSubImage2D_tempwrap },
        { "glCopyTexImage2D", (uintptr_t)&glCopyTexImage2D_tempwrap },
        { "glCopyTexSubImage2D", (uintptr_t)&glCopyTexSubImage2D_tempwrap },
        { "glCullFace", (uintptr_t)&glCullFace},
        { "glCurrentPaletteMatrixOES", (uintptr_t)&glCurrentPaletteMatrixOES_tempwrap },
        { "glDeleteBuffers", (uintptr_t)&glDeleteBuffers },
        { "glDeleteFramebuffersOES", (uintptr_t)&glDeleteFramebuffers },
        { "glDeleteRenderbuffersOES", (uintptr_t)&glDeleteRenderbuffers },
        { "glDeleteTextures", (uintptr_t)&glDeleteTextures },
        { "glDepthFunc", (uintptr_t)&glDepthFunc },
        { "glDepthMask", (uintptr_t)&glDepthMask },
        { "glDepthRangef", (uintptr_t) &glDepthRangef },
        { "glDepthRangex", (uintptr_t)&glDepthRangex },
        { "glDisable", (uintptr_t)&glDisable },
        { "glDisableClientState", (uintptr_t)&glDisableClientState},
        { "glDrawArrays", (uintptr_t)&glDrawArrays},
        { "glDrawElements", (uintptr_t)&glDrawElements },
        { "glDrawTexfOES", (uintptr_t)&glDrawTexfOES_tempwrap },
        { "glDrawTexfvOES", (uintptr_t)&glDrawTexfvOES_tempwrap },
        { "glDrawTexiOES", (uintptr_t)&glDrawTexiOES_tempwrap },
        { "glDrawTexivOES", (uintptr_t)&glDrawTexivOES_tempwrap },
        { "glDrawTexsOES", (uintptr_t)&glDrawTexsOES_tempwrap },
        { "glDrawTexsvOES", (uintptr_t)&glDrawTexsvOES_tempwrap },
        { "glDrawTexxOES", (uintptr_t)&glDrawTexxOES_tempwrap },
        { "glDrawTexxvOES", (uintptr_t)&glDrawTexxvOES_tempwrap },
        { "glEGLImageTargetRenderbufferStorageOES", (uintptr_t)&glEGLImageTargetRenderbufferStorageOES_tempwrap },
        { "glEGLImageTargetTexture2DOES", (uintptr_t)&glEGLImageTargetTexture2DOES_tempwrap },
        { "glEnable", (uintptr_t)&glEnable },
        { "glEnableClientState", (uintptr_t)&glEnableClientState},
        { "glFinish", (uintptr_t)&glFinish},
        { "glFlush", (uintptr_t)&glFlush},
        { "glFogf", (uintptr_t)&glFogf},
        { "glFogfv", (uintptr_t)&glFogfv},
        { "glFogx", (uintptr_t)&glFogx },
        { "glFogxv", (uintptr_t)&glFogxv },
        { "glFramebufferRenderbufferOES", (uintptr_t)&glFramebufferRenderbuffer },
        { "glFramebufferTexture2DOES", (uintptr_t)&glFramebufferTexture2D },
        { "glFrontFace", (uintptr_t)&glFrontFace},
        { "glFrustumf", (uintptr_t)&glFrustumf },
        { "glFrustumx", (uintptr_t)&glFrustumx},
        { "glGenBuffers", (uintptr_t)&glGenBuffers },
        { "glGenFramebuffersOES", (uintptr_t)&glGenFramebuffers },
        { "glGenRenderbuffersOES", (uintptr_t)&glGenRenderbuffers },
        { "glGenTextures", (uintptr_t)&glGenTextures },
        { "glGenerateMipmapOES", (uintptr_t)&glGenerateMipmap },
        { "glGetBooleanv", (uintptr_t)&glGetBooleanv},
        { "glGetBufferParameteriv", (uintptr_t)&glGetBufferParameteriv},
        { "glGetBufferPointervOES", (uintptr_t)&glGetBufferPointervOES_tempwrap },
        { "glGetClipPlanef", (uintptr_t)&glGetClipPlanef_tempwrap },
        { "glGetClipPlanex", (uintptr_t)&glGetClipPlanex_tempwrap },
        { "glGetError", (uintptr_t)&glGetError },
        { "glGetFixedv", (uintptr_t)&glGetFixedv_tempwrap },
        { "glGetFloatv", (uintptr_t)&glGetFloatv},
        { "glGetFramebufferAttachmentParameterivOES", (uintptr_t)&glGetFramebufferAttachmentParameteriv },
        { "glGetIntegerv", (uintptr_t)&glGetIntegerv},
        { "glGetLightfv", (uintptr_t)&glGetLightfv_tempwrap },
        { "glGetLightxv", (uintptr_t)&glGetLightxv_tempwrap },
        { "glGetMaterialfv", (uintptr_t)&glGetMaterialfv_tempwrap },
        { "glGetMaterialxv", (uintptr_t)&glGetMaterialxv_tempwrap },
        { "glGetPointerv", (uintptr_t)&glGetPointerv_tempwrap },
        { "glGetRenderbufferParameterivOES", (uintptr_t)&glGetRenderbufferParameterivOES_tempwrap },
        { "glGetString", (uintptr_t)&glGetString},
        { "glGetTexEnvfv", (uintptr_t)&glGetTexEnvfv_tempwrap },
        { "glGetTexEnviv", (uintptr_t)&glGetTexEnviv_tempwrap },
        { "glGetTexEnvxv", (uintptr_t)&glGetTexEnvxv_tempwrap },
        { "glGetTexGenfvOES", (uintptr_t)&glGetTexGenfvOES_tempwrap },
        { "glGetTexGenivOES", (uintptr_t)&glGetTexGenivOES_tempwrap },
        { "glGetTexGenxvOES", (uintptr_t)&glGetTexGenxvOES_tempwrap },
        { "glGetTexParameterfv", (uintptr_t)&glGetTexParameterfv_tempwrap },
        { "glGetTexParameteriv", (uintptr_t)&glGetTexParameteriv_tempwrap },
        { "glGetTexParameterxv", (uintptr_t)&glGetTexParameterxv_tempwrap },
        { "glHint", (uintptr_t)&glHint},
        { "glIsBuffer", (uintptr_t)&glIsBuffer_tempwrap },
        { "glIsEnabled", (uintptr_t)&glIsEnabled},
        { "glIsFramebufferOES", (uintptr_t)&glIsFramebuffer },
        { "glIsRenderbufferOES", (uintptr_t)&glIsRenderbufferOES_tempwrap },
        { "glIsTexture", (uintptr_t)&glIsTexture},
        { "glLightModelf", (uintptr_t)&glLightModelf_tempwrap },
        { "glLightModelfv", (uintptr_t)&glLightModelfv},
        { "glLightModelx", (uintptr_t)&glLightModelx_tempwrap },
        { "glLightModelxv", (uintptr_t)&glLightModelxv},
        { "glLightf", (uintptr_t)&glLightf_tempwrap },
        { "glLightfv", (uintptr_t)&glLightfv},
        { "glLightx", (uintptr_t)&glLightx_tempwrap },
        { "glLightxv", (uintptr_t)&glLightxv},
        { "glLineWidth", (uintptr_t)&glLineWidth },
        { "glLineWidthx", (uintptr_t)&glLineWidthx },
        { "glLoadIdentity", (uintptr_t)&glLoadIdentity},
        { "glLoadMatrixf", (uintptr_t)&glLoadMatrixf},
        { "glLoadMatrixx", (uintptr_t)&glLoadMatrixx },
        { "glLoadPaletteFromModelViewMatrixOES", (uintptr_t)&glLoadPaletteFromModelViewMatrixOES_tempwrap },
        { "glLogicOp", (uintptr_t)&glLogicOp_tempwrap },
        { "glMapBufferOES", (uintptr_t)&glMapBuffer },
        { "glMaterialf", (uintptr_t)&glMaterialf_tempwrap },
        { "glMaterialfv", (uintptr_t)&glMaterialfv},
        { "glMaterialx", (uintptr_t)&glMaterialx_tempwrap },
        { "glMaterialxv", (uintptr_t)&glMaterialxv},
        { "glMatrixIndexPointerOES", (uintptr_t)&glMatrixIndexPointerOES_tempwrap },
        { "glMatrixMode", (uintptr_t)&glMatrixMode},
        { "glMultMatrixf", (uintptr_t)&glMultMatrixf},
        { "glMultMatrixx", (uintptr_t)&glMultMatrixx},
        { "glMultiTexCoord4f", (uintptr_t)&glMultiTexCoord4f_tempwrap },
        { "glMultiTexCoord4x", (uintptr_t)&glMultiTexCoord4x_tempwrap },
        { "glNormal3f", (uintptr_t)&glNormal3f },
        { "glNormal3x", (uintptr_t)&glNormal3x_tempwrap },
        { "glNormalPointer", (uintptr_t)&glNormalPointer},
        { "glOrthof", (uintptr_t)&glOrthof },
        { "glOrthox", (uintptr_t)&glOrthox },
        { "glPixelStorei", (uintptr_t)&glPixelStorei_tempwrap},
        { "glPointParameterf", (uintptr_t)&glPointParameterf_tempwrap },
        { "glPointParameterfv", (uintptr_t)&glPointParameterfv_tempwrap },
        { "glPointParameterx", (uintptr_t)&glPointParameterx_tempwrap },
        { "glPointParameterxv", (uintptr_t)&glPointParameterxv_tempwrap },
        { "glPointSize", (uintptr_t)&glPointSize},
        { "glPointSizePointerOES", (uintptr_t)&glPointSizePointerOES_tempwrap },
        { "glPointSizex", (uintptr_t)&glPointSizex },
        { "glPolygonOffset", (uintptr_t)&glPolygonOffset},
        { "glPolygonOffsetx", (uintptr_t)&glPolygonOffsetx },
        { "glPopMatrix", (uintptr_t)&glPopMatrix},
        { "glPushMatrix", (uintptr_t)&glPushMatrix},
        { "glQueryMatrixxOES", (uintptr_t)&glQueryMatrixxOES_tempwrap },
        { "glReadPixels", (uintptr_t)&glReadPixels},
        { "glRenderbufferStorageOES", (uintptr_t)&glRenderbufferStorage },
        { "glRotatef", (uintptr_t)&glRotatef },
        { "glRotatex", (uintptr_t)&glRotatex},
        { "glSampleCoverage", (uintptr_t)&glSampleCoverage_tempwrap },
        { "glSampleCoveragex", (uintptr_t)&glSampleCoveragex_tempwrap },
        { "glScalef", (uintptr_t)&glScalef },
        { "glScalex", (uintptr_t)&glScalex},
        { "glScissor", (uintptr_t)&glScissor},
        { "glShadeModel", (uintptr_t)&glShadeModel},
        { "glStencilFunc", (uintptr_t)&glStencilFunc},
        { "glStencilMask", (uintptr_t)&glStencilMask},
        { "glStencilOp", (uintptr_t)&glStencilOp},
        { "glTexCoordPointer", (uintptr_t)&glTexCoordPointer},
        { "glTexEnvf", (uintptr_t)&glTexEnvf},
        { "glTexEnvfv", (uintptr_t)&glTexEnvfv},
        { "glTexEnvi", (uintptr_t)&glTexEnvi},
        { "glTexEnviv", (uintptr_t)&glTexEnviv_tempwrap },
        { "glTexEnvx", (uintptr_t)&glTexEnvx },
        { "glTexEnvxv", (uintptr_t)&glTexEnvxv},
        { "glTexGenfOES", (uintptr_t)&glTexGenfOES_tempwrap },
        { "glTexGenfvOES", (uintptr_t)&glTexGenfvOES_tempwrap },
        { "glTexGeniOES", (uintptr_t)&glTexGeniOES_tempwrap },
        { "glTexGenivOES", (uintptr_t)&glTexGenivOES_tempwrap },
        { "glTexGenxOES", (uintptr_t)&glTexGenxOES_tempwrap },
        { "glTexGenxvOES", (uintptr_t)&glTexGenxvOES_tempwrap },
        { "glTexImage2D", (uintptr_t)&glTexImage2D },
        { "glTexParameterf", (uintptr_t)&glTexParameterf },
        { "glTexParameterfv", (uintptr_t)&glTexParameterfv_tempwrap },
        { "glTexParameteri", (uintptr_t)&glTexParameteri },
        { "glTexParameteriv", (uintptr_t)&glTexParameteriv },
        { "glTexParameterx", (uintptr_t)&glTexParameterx },
        { "glTexParameterxv", (uintptr_t)&glTexParameterxv_tempwrap },
        { "glTexSubImage2D", (uintptr_t)&glTexSubImage2D },
        { "glTranslatef", (uintptr_t)&glTranslatef },
        { "glTranslatex", (uintptr_t)&glTranslatex},
        { "glUnmapBufferOES", (uintptr_t)&glUnmapBuffer },
        { "glVertexPointer", (uintptr_t)&glVertexPointer},
        { "glViewport", (uintptr_t)&glViewport },
        { "glWeightPointerOES", (uintptr_t)&glWeightPointerOES_tempwrap },
#endif
        { "gmtime", (uintptr_t)&gmtime},
        { "ldexp", (uintptr_t)&ldexp },
        { "listen", (uintptr_t)&listen},
        { "localtime", (uintptr_t)&localtime},
        { "log", (uintptr_t)&log },
        { "log10", (uintptr_t)&log10 },
        { "log10f", (uintptr_t)&log10f},
        { "logf", (uintptr_t)&logf},
        { "longjmp", (uintptr_t)&longjmp },
        { "lrand48", (uintptr_t)&lrand48 },
        { "lseek", (uintptr_t)&lseek_soloader },
        { "malloc", (uintptr_t)&malloc },
        { "memcmp", (uintptr_t)&memcmp },
        { "memcpy", (uintptr_t)&memcpy },
        { "memmove", (uintptr_t)&memmove },
        { "memset", (uintptr_t)&memset },
        { "mkdir", (uintptr_t)&mkdir },
        { "mktime", (uintptr_t)&mktime},
        { "mmap", (uintptr_t)&mmap },
        { "modf", (uintptr_t)&modf },
        { "munmap", (uintptr_t)&munmap },
        { "nanosleep", (uintptr_t)&nanosleep_soloader },
        { "open", (uintptr_t)&open_soloader },
        { "opendir", (uintptr_t)&opendir_soloader},
        { "poll", (uintptr_t)&poll_tempwrap },
        { "pow", (uintptr_t)&pow },
        { "powf", (uintptr_t)&powf },
        { "prctl", (uintptr_t)&prctl_tempwrap },
        { "pthread_attr_destroy", (uintptr_t)&pthread_attr_destroy_soloader },
        { "pthread_attr_getstack", (uintptr_t)&pthread_attr_getstack_soloader },
        { "pthread_attr_init", (uintptr_t) &pthread_attr_init_soloader },
        { "pthread_attr_setdetachstate", (uintptr_t) &pthread_attr_setdetachstate_soloader },
        { "pthread_attr_setschedparam", (uintptr_t)&pthread_attr_setschedparam_soloader },
        { "pthread_attr_setstack", (uintptr_t)&pthread_attr_setstack_soloader },
        { "pthread_attr_setstacksize", (uintptr_t) &pthread_attr_setstacksize_soloader },
        { "pthread_cond_broadcast", (uintptr_t) &pthread_cond_broadcast_soloader },
        { "pthread_cond_destroy", (uintptr_t) &pthread_cond_destroy_soloader },
        { "pthread_cond_init", (uintptr_t) &pthread_cond_init_soloader },
        { "pthread_cond_signal", (uintptr_t) &pthread_cond_signal_soloader },
        { "pthread_cond_timedwait", (uintptr_t) &pthread_cond_timedwait_soloader },
        { "pthread_cond_wait", (uintptr_t) &pthread_cond_wait_soloader },
        { "pthread_create", (uintptr_t) &pthread_create_soloader },
        { "pthread_exit", (uintptr_t) &pthread_exit },
        { "pthread_getattr_np", (uintptr_t) &pthread_getattr_np_soloader },
        { "pthread_getschedparam", (uintptr_t) &pthread_getschedparam_soloader },
        { "pthread_getspecific", (uintptr_t)&pthread_getspecific },
        { "pthread_key_create", (uintptr_t)&pthread_key_create },
        { "pthread_key_delete", (uintptr_t)&pthread_key_delete },
        { "pthread_mutex_destroy", (uintptr_t) &pthread_mutex_destroy_soloader },
        { "pthread_mutex_init", (uintptr_t) &pthread_mutex_init_soloader },
        { "pthread_mutex_lock", (uintptr_t) &pthread_mutex_lock_soloader },
        { "pthread_mutex_trylock", (uintptr_t) &pthread_mutex_trylock_soloader},
        { "pthread_mutex_unlock", (uintptr_t) &pthread_mutex_unlock_soloader },
        { "pthread_mutexattr_destroy", (uintptr_t) &pthread_mutexattr_destroy_soloader},
        { "pthread_mutexattr_init", (uintptr_t) &pthread_mutexattr_init_soloader},
        { "pthread_mutexattr_setpshared", (uintptr_t) &pthread_mutexattr_setpshared_soloader},
        { "pthread_mutexattr_settype", (uintptr_t) &pthread_mutexattr_settype_soloader},
        { "pthread_once", (uintptr_t)&pthread_once },
        { "pthread_self", (uintptr_t) &pthread_self_soloader },
        { "pthread_setschedparam", (uintptr_t) &pthread_setschedparam_soloader },
        { "pthread_setspecific", (uintptr_t)&pthread_setspecific },
        { "qsort", (uintptr_t)&qsort },
        { "raise", (uintptr_t)&raise},
        { "read", (uintptr_t)&read_soloader },
        { "readdir", (uintptr_t)&readdir_soloader},
        { "readdir_r", (uintptr_t)&readdir_r_soloader},
        { "realloc", (uintptr_t)&realloc },
        { "recv", (uintptr_t)&recv},
        { "recvfrom", (uintptr_t)&recvfrom},
        { "remove", (uintptr_t)&remove},
        { "rename", (uintptr_t)&rename},
        { "rmdir", (uintptr_t)&rmdir},
        { "sbrk", (uintptr_t)&sbrk},
        { "sched_yield", (uintptr_t)&sched_yield},
        { "sem_destroy", (uintptr_t) &sem_destroy_soloader},
        { "sem_getvalue", (uintptr_t) &sem_getvalue_soloader},
        { "sem_init", (uintptr_t) &sem_init_soloader},
        { "sem_post", (uintptr_t) &sem_post_soloader},
        { "sem_timedwait", (uintptr_t) &sem_timedwait_soloader},
        { "sem_trywait", (uintptr_t) &sem_trywait_soloader},
        { "sem_wait", (uintptr_t) &sem_wait_soloader},
        { "send", (uintptr_t)&send},
        { "sendto", (uintptr_t)&sendto},
        { "setenv", (uintptr_t)&setenv_ret0 },
        { "setjmp", (uintptr_t)&setjmp },
        { "setsockopt", (uintptr_t)&setsockopt},
        { "shutdown", (uintptr_t)&shutdown },
        { "sin", (uintptr_t)&sin },
        { "sinf", (uintptr_t)&sinf },
        { "snprintf", (uintptr_t)&snprintf },
        { "socket", (uintptr_t)&socket},
        { "sprintf", (uintptr_t)&sprintf },
        { "sqrt", (uintptr_t)&sqrt },
        { "sqrtf", (uintptr_t)&sqrtf },
        { "sscanf", (uintptr_t)&sscanf },
        { "stat", (uintptr_t)&stat_soloader },
        { "strcat", (uintptr_t)&strcat },
        { "strchr", (uintptr_t)&strchr },
        { "strcmp", (uintptr_t)&strcmp },
        { "strcpy", (uintptr_t)&strcpy },
        { "strdup", (uintptr_t)&strdup},
        { "strlen", (uintptr_t)&strlen },
        { "strncmp", (uintptr_t)&strncmp },
        { "strncpy", (uintptr_t)&strncpy },
        { "strrchr", (uintptr_t)&strrchr },
        { "strstr", (uintptr_t)&strstr_soloader },
        { "strtod", (uintptr_t)&strtod },
        { "strtok", (uintptr_t)&strtok},
        { "strtol", (uintptr_t)&strtol },
        { "strtoul", (uintptr_t)&strtoul },
        { "sysconf", (uintptr_t)&sysconf_tempwrap },
        { "system", (uintptr_t)&system},
        { "tan", (uintptr_t)&tan },
        { "tanf", (uintptr_t)&tanf },
        { "time", (uintptr_t)&time },
        { "ungetc", (uintptr_t)&ungetc },
        { "unlink", (uintptr_t)&unlink},
        { "usleep", (uintptr_t)&usleep },
        { "utime", (uintptr_t)&utime },
        { "vsnprintf", (uintptr_t)&vsnprintf },
        { "vsprintf", (uintptr_t)&vsprintf },
        { "vsscanf", (uintptr_t)&vsscanf},
        { "waitpid", (uintptr_t)&waitpid_tempwrap },
        { "wcslen", (uintptr_t)&wcslen },
        { "write", (uintptr_t)&write_soloader },
};

void resolve_imports(so_module* mod) {
    printf("sz: %i\n", sizeof(default_dynlib) / sizeof(default_dynlib[0]));
    so_resolve(mod, default_dynlib, sizeof(default_dynlib), 0);
}
