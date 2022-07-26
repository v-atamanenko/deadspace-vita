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

//this
void glTexEnvf_sfp(GLenum target, GLenum pname, int param)
{
	float fa1;

	fa1 = *(float *)(&param);
	glTexEnvf(target, pname, fa1);
}

//this
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

//this
void glClearDepthf_sfp(int depth)
{
	float fa1;

	fa1 = *(float *)(&depth);
	glClearDepthf(fa1);
}

//this
void glDepthRangef_sfp(int fZNear, int fZFar)
{
	float fa1, fa2;

	fa1 = *(float *)(&fZNear);
	fa2 = *(float *)(&fZFar);
	glDepthRangef(fa1, fa2);
}

void glColor4f_sfp(int red, int green, int blue, int alpha) {
    float fa1, fa2, fa3, fa4;

    fa1 = *(float *)(&red);
    fa2 = *(float *)(&green);
    fa3 = *(float *)(&blue);
    fa4 = *(float *)(&alpha);

    glColor4f(fa1, fa2, fa3, fa4);
}

void glFrustumf_sfp(int left, int right, int bottom, int top, int zNear, int zFar) {
    float fa1, fa2, fa3, fa4, fa5, fa6;

    fa1 = *(float *)(&left);
    fa2 = *(float *)(&right);
    fa3 = *(float *)(&bottom);
    fa4 = *(float *)(&top);
    fa5 = *(float *)(&zNear);
    fa6 = *(float *)(&zFar);
    glFrustumf(fa1, fa2, fa3, fa4, fa5, fa6);
}

void glNormal3f_sfp(int nx, int ny, int nz) {
    float fa1;
    float fa2;
    float fa3;

    fa1 = *(float *)(&nx);
    fa2 = *(float *)(&ny);
    fa3 = *(float *)(&nz);
    glNormal3f(fa1,fa2,fa3);
}

void glOrthof_sfp(int left, int right, int bottom, int top, int zNear, int zFar) {
    float fa1, fa2, fa3, fa4, fa5, fa6;

    fa1 = *(float *)(&left);
    fa2 = *(float *)(&right);
    fa3 = *(float *)(&bottom);
    fa4 = *(float *)(&top);
    fa5 = *(float *)(&zNear);
    fa6 = *(float *)(&zFar);

    glOrthof(fa1, fa2, fa3, fa4, fa5, fa6);
}

void glRotatef_sfp(int angle, int x, int y, int z) {
    float fa1, fa2, fa3, fa4;

    fa1 = *(float *)(&angle);
    fa2 = *(float *)(&x);
    fa3 = *(float *)(&y);
    fa4 = *(float *)(&z);
    glRotatef(fa1, fa2, fa3, fa4);
}

void glScalef_sfp(int x, int y, int z) {
    float fa1, fa2, fa3;

    fa1 = *(float *)(&x);
    fa2 = *(float *)(&y);
    fa3 = *(float *)(&z);
    glScalef(fa1, fa2, fa3);
}

void glTexParameterf_sfp(GLenum target, GLenum pname, int param) {
    float fa1;

    fa1 = *(float *)(&param);
    glTexParameterf(target, pname, fa1);
}

void glTranslatef_sfp(int x, int y, int z) {
    float fa1, fa2, fa3;

    fa1 = *(float *)(&x);
    fa2 = *(float *)(&y);
    fa3 = *(float *)(&z);
    glTranslatef(fa1, fa2, fa3);
}

void glMaterialf_sfp(GLenum face, GLenum pname, int param) {
    float fa1;
    fa1 = *(float *)(&param);
    glMaterialf(face, pname, fa1);
}

void glDrawTexfOES_sfp(int x, int y, int z, int width, int height) {
    float fa1, fa2, fa3, fa4, fa5;
    fa1 = *(float *)(&x);
    fa2 = *(float *)(&y);
    fa3 = *(float *)(&z);
    fa4 = *(float *)(&width);
    fa5 = *(float *)(&height);
    glDrawTexfOES(fa1, fa2, fa3, fa4, fa5);
}

void glLightModelf_sfp(GLenum pname, int param) {
    float fa1;
    fa1 = *(float *)(&param);
    glLightModelf(pname, fa1);
}

void glLightf_sfp(GLenum light, GLenum pname, int param) {
    float fa1;
    fa1 = *(float *)(&param);
    glLightf(light, pname, fa1);
}

void glMultiTexCoord4f_sfp(GLenum target, int s, int t, int r, int q) {
    float fa1, fa2,fa3,fa4;
    fa1 = *(float *)(&s);
    fa2 = *(float *)(&t);
    fa3 = *(float *)(&r);
    fa4 = *(float *)(&q);
    glMultiTexCoord4f(target, fa1, fa2, fa3, fa4);
}

void glPointParameterf_sfp(GLenum pname, int param) {
    float fa1;
    fa1 = *(float *)(&param);
    glPointParameterf(pname, fa1);
}

void glTexGenfOES_sfp(GLenum coord, GLenum pname, int param) {
    float fa1;
    fa1 = *(float *)(&param);
    glTexGenfOES(coord, pname, fa1);
}

void glClearColor_sfp(int red, int green, int blue, int alpha) {
    float fa1, fa2, fa3, fa4;
    fa1 = *(float *)(&red);
    fa2 = *(float *)(&green);
    fa3 = *(float *)(&blue);
    fa4 = *(float *)(&alpha);
    glClearColor(fa1,fa2,fa3,fa4);
}

void glPolygonOffset_sfp(int factor, int units) {
    float fa1, fa2;
    fa1 = *(float *)(&factor);
    fa2 = *(float *)(&units);
    glPolygonOffset(fa1, fa2);
}

void glPointSize_sfp(int size) {
    float fa1;
    fa1 = *(float *)(&size);
    glPointSize(fa1);
}

void glLineWidth_sfp(int width) {
    float fa1;
    fa1 = *(float *)(&width);
    glLineWidth(fa1);
}

#endif // SOLOADER_SFP2HFP_H
