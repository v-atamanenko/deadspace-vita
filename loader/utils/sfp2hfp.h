/*
 * sfp2hfp.h
 *
 * Wrapper for PVR_PSP2 functions that use floats, translating softfp to hardfp.
 *
 * Copyright (C) 2022 GrapheneCt
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef SOLOADER_SFP2HFP_H
#define SOLOADER_SFP2HFP_H

#include <stdint.h>
#include <math.h>

#include "GLES2/gl2.h"
#include "GLES2/gl2ext.h"

#ifdef GL_VERSION_ES_CM_1_0

void glTexEnvf_sfp(GLenum target, GLenum pname, int param)
{
	float fa1;

	fa1 = *(float *)(&param);
	glTexEnvf(target, pname, fa1);
}

void glFogf_sfp(GLenum pname, int param)
{
	float fa1;

	fa1 = *(float *)(&param);
	glFogf(pname, fa1);
}

void glAlphaFunc_sfp(GLenum func, int ref)
{
	float fa1;

	fa1 = *(float *)(&ref);
	glAlphaFunc(func, fa1);
}

#endif

void glClearDepthf_sfp(int depth)
{
	float fa1;

	fa1 = *(float *)(&depth);
	glClearDepthf(fa1);
}

void glDepthRangef_sfp(int fZNear, int fZFar)
{
	float fa1, fa2;

	fa1 = *(float *)(&fZNear);
	fa2 = *(float *)(&fZFar);
	glDepthRangef(fa1, fa2);
}

void glUniform1f_sfp(GLint location, int x)
{
    float fa1;

    fa1 = *(float *)(&x);
    glUniform1f(location, fa1);
}

void glUniform2f_sfp(GLint location, int x, int y)
{
    float fa1, fa2;

    fa1 = *(float *)(&x);
    fa2 = *(float *)(&y);
    glUniform2f(location, fa1, fa2);
}

void glUniform4f_sfp(GLint location, int x, int y, int z, int w)
{
    float fa1, fa2, fa3, fa4;

    fa1 = *(float *)(&x);
    fa2 = *(float *)(&y);
    fa3 = *(float *)(&z);
    fa4 = *(float *)(&w);
    glUniform4f(location, fa1, fa2, fa3, fa4);
}

#endif // SOLOADER_SFP2HFP_H
