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
include <psp2/power.h>
#include <psp2/appmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/vshbridge.h>

#include "gpu_es4/psp2_pvr_hint.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "utils/sfp2hfp.h"
#else
#include <vitaGL.h>
void glShaderSourceHook(GLuint shader, GLsizei count,
                        const GLchar **string, const GLint *length);
#endif

void gl_init();


#ifdef DEBUG_GL
void glActiveTexture_dbg(GLenum texture) {
    fprintf(stderr, "[vgldbg] glActiveTexture(%x)\n", texture);
    return glActiveTexture(texture);
}

void glAttachShader_dbg(uint32_t prog, uint32_t shad) {
    fprintf(stderr, "[vgldbg] glAttachShader(%x, %x)\n", prog, shad);
    return glAttachShader(prog, shad);
}

void glBindAttribLocation_dbg(GLuint program, GLuint index, const GLchar *name) {
    fprintf(stderr, "[vgldbg] glBindAttribLocation(%x, %x, %s)\n", program, index, name);
    return glBindAttribLocation(program, index, name);
}

void glBindBuffer_dbg(uint32_t target, uint32_t buffer) {
    fprintf(stderr, "[vgldbg] glBindBuffer(%x, %x)\n", target, buffer);
    return glBindBuffer(target, buffer);
}

void glBindFramebuffer_dbg(int32_t target, uint32_t buffer) {
    fprintf(stderr, "[vgldbg] glBindFramebuffer(%x, %x)\n", target, buffer);
    return glBindFramebuffer(target, buffer);
}

void glBindTexture_dbg(int32_t target, uint32_t texture) {
    fprintf(stderr, "[vgldbg] glBindTexture(%x, %x)\n", target, texture);
    return glBindTexture(target, texture);
}

void glBlendEquationSeparate_dbg(int32_t modeRGB, uint32_t modeAlpha) {
    fprintf(stderr, "[vgldbg] glBlendEquationSeparate(%x, %x)\n", modeRGB, modeAlpha);
    return glBlendEquationSeparate(modeRGB, modeAlpha);
}

void glBlendFunc_dbg(int32_t sfactor, uint32_t dfactor) {
    fprintf(stderr, "[vgldbg] glBlendFunc(%x, %x)\n", sfactor, dfactor);
    return glBlendFunc(sfactor, dfactor);
}

void glBlendFuncSeparate_dbg(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    fprintf(stderr, "[vgldbg] glBlendFuncSeparate(%x, %x, %x, %x)\n", srcRGB, dstRGB, srcAlpha, dstAlpha);
    return glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void glBufferData_dbg(GLenum target, GLsizei size, const GLvoid *data, GLenum usage) {
    fprintf(stderr, "[vgldbg] glBufferData(%x, %x, [data], %x)\n", target, size, usage);
    return glBufferData(target, size, data, usage);
}

void glClear_dbg(uint32_t mask) {
    fprintf(stderr, "[vgldbg] glClear(%x)\n", mask);
    return glClear(mask);
}

void glClearColor_dbg(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    fprintf(stderr, "[vgldbg] glClearColor(%f, %f, %f, %f)\n", red, green, blue, alpha);
    return glClearColor(red, green, blue, alpha);
}

void glClearDepthf_dbg(GLclampf depth) {
    fprintf(stderr, "[vgldbg] glClearDepthf(%f)\n", depth);
    return glClearDepthf(depth);
}

void glCompileShader_dbg(uint32_t shader) {
    fprintf(stderr, "[vgldbg] glCompileShader(%x) — stubbed\n", shader);
    //return glCompileShader(shader);
}

void glCompressedTexImage2D_dbg(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) {
    fprintf(stderr, "[vgldbg] glCompressedTexImage2D(%x, %x, %x, %x, %x, %x, %x, [data])\n", target, level, internalformat, width, height, border, imageSize);
    return glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}

void glCopyTexSubImage2D_dbg(void) {
    fprintf(stderr, "[vgldbg] glCopyTexSubImage2D(unk) — stubbed\n");
}

GLuint glCreateProgram_dbg(void) {
    fprintf(stderr, "[vgldbg] glCreateProgram()\n");
    GLuint ret = glCreateProgram();
    fprintf(stderr, "[vgldbg] glCreateProgram(): ret %x\n", ret);
    return ret;
}

GLuint glCreateShader_dbg(GLenum shaderType) {
    fprintf(stderr, "[vgldbg] glCreateShader(%x)\n", shaderType);
    return glCreateShader(shaderType);
}

void glDeleteBuffers_dbg(GLsizei n, const GLuint *gl_buffers) {
    fprintf(stderr, "[vgldbg] glDeleteBuffers(%x, [data])\n", n);
    return glDeleteBuffers(n, gl_buffers);
}

void glDeleteFramebuffers_dbg(GLsizei n, const GLuint *framebuffers) {
    fprintf(stderr, "[vgldbg] glDeleteFramebuffers(%x, [data])\n", n);
    return glDeleteFramebuffers(n, framebuffers);
}

void glDeleteTextures_dbg(GLsizei n, const GLuint *textures) {
    fprintf(stderr, "[vgldbg] glDeleteTextures(%x, [data])\n", n);
    return glDeleteTextures(n, textures);
}

void glDepthFunc_dbg(uint32_t func) {
    fprintf(stderr, "[vgldbg] glDepthFunc(%x)\n", func);
    return glDepthFunc(func);
}

void glDepthMask_dbg(GLboolean flag) {
    fprintf(stderr, "[vgldbg] glDepthMask(%x)\n", flag);
    return glDepthMask(flag);
}

void glDepthRangef_dbg(GLfloat nearVal, GLfloat farVal) {
    fprintf(stderr, "[vgldbg] glDepthRangef(%f, %f)\n", nearVal, farVal);
    return glDepthRangef(nearVal, farVal);
}

void glDetachShader_dbg() {
    fprintf(stderr, "[vgldbg] glDetachShader(unk) — stubbed\n");
}

void glDisable_dbg(GLenum cap) {
    fprintf(stderr, "[vgldbg] glDisable(%x)\n", cap);
    return glDisable(cap);
}

void glDrawArrays_dbg(GLenum mode, GLint first, GLsizei count) {
    fprintf(stderr, "[vgldbg] glDrawArrays(%x, %x, %x)\n", mode, first, count);
    return glDrawArrays(mode, first, count);
}

void glDrawElements_dbg(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
    fprintf(stderr, "[vgldbg] glDrawElements(%x, %x, %x, [data])\n", mode, count, type);
    return glDrawElements(mode, count, type, indices);
}

void glEnable_dbg(uint32_t cap) {
    fprintf(stderr, "[vgldbg] glEnable(%x)\n", cap);
    return glEnable(cap);
}

void glEnableVertexAttribArray_dbg(uint32_t index) {
    fprintf(stderr, "[vgldbg] glEnableVertexAttribArray(%x)\n", index);
    return glEnableVertexAttribArray(index);
}

void glFramebufferTexture2D_dbg(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    fprintf(stderr, "[vgldbg] glFramebufferTexture2D(%x, %x, %x, %x, %x)\n", target, attachment, textarget, texture, level);
    return glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

void glGenBuffers_dbg(GLsizei n, GLuint *buffers) {
    fprintf(stderr, "[vgldbg] glGenBuffers(%x, [data])\n", n);
    return glGenBuffers(n, buffers);
}

void glGenFramebuffers_dbg(GLsizei n, GLuint *framebuffers) {
    fprintf(stderr, "[vgldbg] glGenFramebuffers(%x, [data])\n", n);
    return glGenFramebuffers(n, framebuffers);
}

void glGenTextures_dbg(GLsizei n, GLuint *textures) {
    fprintf(stderr, "[vgldbg] glGenTextures(%x, [data])\n", n);
    return glGenTextures(n, textures);
}

GLenum glGetError_dbg(void) {
    fprintf(stderr, "[vgldbg] glGetError(void)\n");
    return glGetError();
}

void glGetIntegerv_dbg(GLenum pname, GLint *data) {
    fprintf(stderr, "[vgldbg] glGetIntegerv(%x, [data])\n", pname);
    return glGetIntegerv(pname, data);
}

void glGetProgramInfoLog_dbg(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog) {
    fprintf(stderr, "[vgldbg] glGetProgramInfoLog(%x, %x, [data], [data])\n", program, maxLength);
    return glGetProgramInfoLog(program, maxLength, length, infoLog);
}

void glGetProgramiv_dbg(GLuint program, GLenum pname, GLint *params) {
    fprintf(stderr, "[vgldbg] glGetProgramiv(%x, %x, [data])\n", program, pname);
    return glGetProgramiv(program, pname, params);
}

void glGetShaderInfoLog_dbg(GLuint handle, GLsizei maxLength, GLsizei *length, GLchar *infoLog) {
    fprintf(stderr, "[vgldbg] glGetShaderInfoLog(%x, %x, [data], [data])\n", handle, maxLength);
    return glGetShaderInfoLog(handle, maxLength, length, infoLog);
}

void glGetShaderiv_dbg(GLuint handle, GLenum pname, GLint *params) {
    fprintf(stderr, "[vgldbg] glGetShaderiv(%x, %x, [data])\n", handle, pname);
    return glGetShaderiv(handle, pname, params);
}

const GLubyte *glGetString_dbg(GLenum name) {
    fprintf(stderr, "[vgldbg] glGetString(%x)\n", name);
    return glGetString(name);
}

GLint glGetUniformLocation_dbg(GLuint prog, const GLchar *name) {
    fprintf(stderr, "[vgldbg] glGetUniformLocation(%x, [data])\n", prog);
    return glGetUniformLocation(prog, name);
}

void glLinkProgram_dbg(GLuint progr) {
    fprintf(stderr, "[vgldbg] glLinkProgram(%x)\n", progr);
    return glLinkProgram(progr);
}

void glPixelStorei_dbg(void) {
    fprintf(stderr, "[vgldbg] glPixelStorei(unk) — stubbed\n");
}

void glShaderSource_dbg(GLuint handle, GLsizei count, const GLchar *const *string, const GLint *length) {
    fprintf(stderr, "[vgldbg] glShaderSource(%x, %x, [data], [data])\n", handle, count);
    return glShaderSource(handle, count, string, length);
}

void glTexImage2D_dbg(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *data) {
    fprintf(stderr, "[vgldbg] glTexImage2D(%x, %x, %x, %x, %x, %x, %x, %x, [data])\n", target, level, internalFormat, width, height, border, format, type);
    return glTexImage2D(target, level, internalFormat, width, height, border, format, type, data);
}

void glTexParameteri_dbg(GLenum target, GLenum pname, GLint param) {
    fprintf(stderr, "[vgldbg] glTexParameteri(%x, %x, %x)\n", target, pname, param);
    return glTexParameteri(target, pname, param);
}

void glTexSubImage2D_dbg(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
    fprintf(stderr, "[vgldbg] glTexSubImage2D(%x, %x, %x, %x, %x, %x, %x, %x, [data])\n", target, level, xoffset, yoffset, width, height, format, type);
    return glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void glUniform1f_dbg(GLint location, GLfloat v0) {
    fprintf(stderr, "[vgldbg] glUniform1f(%x, %f)\n", location, v0);
    return glUniform1f(location, v0);
}

void glUniform2f_dbg(GLint location, GLfloat v0, GLfloat v1) {
    fprintf(stderr, "[vgldbg] glUniform2f(%x, %f, %f)\n", location, v0, v1);
    return glUniform2f(location, v0, v1);
}

void glUniform4f_dbg(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    fprintf(stderr, "[vgldbg] glUniform4f(%x, %f, %f, %f, %f)\n", location, v0, v1, v2, v3);
    return glUniform4f(location, v0, v1, v2, v3);
}

void glUniform1i_dbg(GLint location, GLint v0) {
    fprintf(stderr, "[vgldbg] glUniform1i(%x, %x)\n", location, v0);
    return glUniform1i(location, v0);
}

void glUseProgram_dbg(GLuint program) {
    fprintf(stderr, "[vgldbg] glUseProgram(%x)\n", program);
    return glUseProgram(program);
}

void glVertexAttribPointer_dbg(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
    fprintf(stderr, "[vgldbg] glVertexAttribPointer(%x, %x, %x, %x, %x, [data])\n", index, size, type, normalized, stride);
    return glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

void glViewport_dbg(GLint x, GLint y, GLsizei width, GLsizei height) {
    fprintf(stderr, "[vgldbg] glViewport(%i, %i, %i, %i)\n", x, y, width, height);
    return glViewport(x, y, width, height);
}
#endif

#endif // SOLOADER_GLUTIL_H
