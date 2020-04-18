/* DO NOT EDIT - This file generated automatically by glX_proto_send.py (from Mesa) script */

/*
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT, IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if !defined( _INDIRECT_H_ )
#  define _INDIRECT_H_

/**
 * \file
 * Prototypes for indirect rendering functions.
 *
 * \author Kevin E. Martin <kevin@precisioninsight.com>
 * \author Ian Romanick <idr@us.ibm.com>
 */

#  if defined(__i386__) && defined(__GNUC__) && !defined(__CYGWIN__) && !defined(__MINGW32__)
#    define FASTCALL __attribute__((fastcall))
#  else
#    define FASTCALL
#  endif
#  if defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#    define NOINLINE __attribute__((noinline))
#  else
#    define NOINLINE
#  endif

#include <X11/Xfuncproto.h>
#include "glxclient.h"

extern _X_HIDDEN NOINLINE CARD32 __glXReadReply( Display *dpy, size_t size,
    void * dest, GLboolean reply_is_always_array );

extern _X_HIDDEN NOINLINE void __glXReadPixelReply( Display *dpy,
    struct glx_context * gc, unsigned max_dim, GLint width, GLint height,
    GLint depth, GLenum format, GLenum type, void * dest,
    GLboolean dimensions_in_reply );

extern _X_HIDDEN NOINLINE FASTCALL GLubyte * __glXSetupSingleRequest(
    struct glx_context * gc, GLint sop, GLint cmdlen );

extern _X_HIDDEN NOINLINE FASTCALL GLubyte * __glXSetupVendorRequest(
    struct glx_context * gc, GLint code, GLint vop, GLint cmdlen );

extern _X_HIDDEN void __indirect_glNewList(GLuint list, GLenum mode);
extern _X_HIDDEN void __indirect_glEndList(void);
extern _X_HIDDEN void __indirect_glCallList(GLuint list);
extern _X_HIDDEN void __indirect_glCallLists(GLsizei n, GLenum type, const GLvoid * lists);
extern _X_HIDDEN void __indirect_glDeleteLists(GLuint list, GLsizei range);
extern _X_HIDDEN GLuint __indirect_glGenLists(GLsizei range);
extern _X_HIDDEN void __indirect_glListBase(GLuint base);
extern _X_HIDDEN void __indirect_glBegin(GLenum mode);
extern _X_HIDDEN void __indirect_glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte * bitmap);
extern _X_HIDDEN void __indirect_glColor3b(GLbyte red, GLbyte green, GLbyte blue);
extern _X_HIDDEN void __indirect_glColor3bv(const GLbyte * v);
extern _X_HIDDEN void __indirect_glColor3d(GLdouble red, GLdouble green, GLdouble blue);
extern _X_HIDDEN void __indirect_glColor3dv(const GLdouble * v);
extern _X_HIDDEN void __indirect_glColor3f(GLfloat red, GLfloat green, GLfloat blue);
extern _X_HIDDEN void __indirect_glColor3fv(const GLfloat * v);
extern _X_HIDDEN void __indirect_glColor3i(GLint red, GLint green, GLint blue);
extern _X_HIDDEN void __indirect_glColor3iv(const GLint * v);
extern _X_HIDDEN void __indirect_glColor3s(GLshort red, GLshort green, GLshort blue);
extern _X_HIDDEN void __indirect_glColor3sv(const GLshort * v);
extern _X_HIDDEN void __indirect_glColor3ub(GLubyte red, GLubyte green, GLubyte blue);
extern _X_HIDDEN void __indirect_glColor3ubv(const GLubyte * v);
extern _X_HIDDEN void __indirect_glColor3ui(GLuint red, GLuint green, GLuint blue);
extern _X_HIDDEN void __indirect_glColor3uiv(const GLuint * v);
extern _X_HIDDEN void __indirect_glColor3us(GLushort red, GLushort green, GLushort blue);
extern _X_HIDDEN void __indirect_glColor3usv(const GLushort * v);
extern _X_HIDDEN void __indirect_glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
extern _X_HIDDEN void __indirect_glColor4bv(const GLbyte * v);
extern _X_HIDDEN void __indirect_glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
extern _X_HIDDEN void __indirect_glColor4dv(const GLdouble * v);
extern _X_HIDDEN void __indirect_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern _X_HIDDEN void __indirect_glColor4fv(const GLfloat * v);
extern _X_HIDDEN void __indirect_glColor4i(GLint red, GLint green, GLint blue, GLint alpha);
extern _X_HIDDEN void __indirect_glColor4iv(const GLint * v);
extern _X_HIDDEN void __indirect_glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha);
extern _X_HIDDEN void __indirect_glColor4sv(const GLshort * v);
extern _X_HIDDEN void __indirect_glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
extern _X_HIDDEN void __indirect_glColor4ubv(const GLubyte * v);
extern _X_HIDDEN void __indirect_glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha);
extern _X_HIDDEN void __indirect_glColor4uiv(const GLuint * v);
extern _X_HIDDEN void __indirect_glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha);
extern _X_HIDDEN void __indirect_glColor4usv(const GLushort * v);
extern _X_HIDDEN void __indirect_glEdgeFlag(GLboolean flag);
extern _X_HIDDEN void __indirect_glEdgeFlagv(const GLboolean * flag);
extern _X_HIDDEN void __indirect_glEnd(void);
extern _X_HIDDEN void __indirect_glIndexd(GLdouble c);
extern _X_HIDDEN void __indirect_glIndexdv(const GLdouble * c);
extern _X_HIDDEN void __indirect_glIndexf(GLfloat c);
extern _X_HIDDEN void __indirect_glIndexfv(const GLfloat * c);
extern _X_HIDDEN void __indirect_glIndexi(GLint c);
extern _X_HIDDEN void __indirect_glIndexiv(const GLint * c);
extern _X_HIDDEN void __indirect_glIndexs(GLshort c);
extern _X_HIDDEN void __indirect_glIndexsv(const GLshort * c);
extern _X_HIDDEN void __indirect_glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz);
extern _X_HIDDEN void __indirect_glNormal3bv(const GLbyte * v);
extern _X_HIDDEN void __indirect_glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz);
extern _X_HIDDEN void __indirect_glNormal3dv(const GLdouble * v);
extern _X_HIDDEN void __indirect_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
extern _X_HIDDEN void __indirect_glNormal3fv(const GLfloat * v);
extern _X_HIDDEN void __indirect_glNormal3i(GLint nx, GLint ny, GLint nz);
extern _X_HIDDEN void __indirect_glNormal3iv(const GLint * v);
extern _X_HIDDEN void __indirect_glNormal3s(GLshort nx, GLshort ny, GLshort nz);
extern _X_HIDDEN void __indirect_glNormal3sv(const GLshort * v);
extern _X_HIDDEN void __indirect_glRasterPos2d(GLdouble x, GLdouble y);
extern _X_HIDDEN void __indirect_glRasterPos2dv(const GLdouble * v);
extern _X_HIDDEN void __indirect_glRasterPos2f(GLfloat x, GLfloat y);
extern _X_HIDDEN void __indirect_glRasterPos2fv(const GLfloat * v);
extern _X_HIDDEN void __indirect_glRasterPos2i(GLint x, GLint y);
extern _X_HIDDEN void __indirect_glRasterPos2iv(const GLint * v);
extern _X_HIDDEN void __indirect_glRasterPos2s(GLshort x, GLshort y);
extern _X_HIDDEN void __indirect_glRasterPos2sv(const GLshort * v);
extern _X_HIDDEN void __indirect_glRasterPos3d(GLdouble x, GLdouble y, GLdouble z);
extern _X_HIDDEN void __indirect_glRasterPos3dv(const GLdouble * v);
extern _X_HIDDEN void __indirect_glRasterPos3f(GLfloat x, GLfloat y, GLfloat z);
extern _X_HIDDEN void __indirect_glRasterPos3fv(const GLfloat * v);
extern _X_HIDDEN void __indirect_glRasterPos3i(GLint x, GLint y, GLint z);
extern _X_HIDDEN void __indirect_glRasterPos3iv(const GLint * v);
extern _X_HIDDEN void __indirect_glRasterPos3s(GLshort x, GLshort y, GLshort z);
extern _X_HIDDEN void __indirect_glRasterPos3sv(const GLshort * v);
extern _X_HIDDEN void __indirect_glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern _X_HIDDEN void __indirect_glRasterPos4dv(const GLdouble * v);
extern _X_HIDDEN void __indirect_glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern _X_HIDDEN void __indirect_glRasterPos4fv(const GLfloat * v);
extern _X_HIDDEN void __indirect_glRasterPos4i(GLint x, GLint y, GLint z, GLint w);
extern _X_HIDDEN void __indirect_glRasterPos4iv(const GLint * v);
extern _X_HIDDEN void __indirect_glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w);
extern _X_HIDDEN void __indirect_glRasterPos4sv(const GLshort * v);
extern _X_HIDDEN void __indirect_glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
extern _X_HIDDEN void __indirect_glRectdv(const GLdouble * v1, const GLdouble * v2);
extern _X_HIDDEN void __indirect_glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
extern _X_HIDDEN void __indirect_glRectfv(const GLfloat * v1, const GLfloat * v2);
extern _X_HIDDEN void __indirect_glRecti(GLint x1, GLint y1, GLint x2, GLint y2);
extern _X_HIDDEN void __indirect_glRectiv(const GLint * v1, const GLint * v2);
extern _X_HIDDEN void __indirect_glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
extern _X_HIDDEN void __indirect_glRectsv(const GLshort * v1, const GLshort * v2);
extern _X_HIDDEN void __indirect_glTexCoord1d(GLdouble s);
extern _X_HIDDEN void __indirect_glTexCoord1dv(const GLdouble * v);
extern _X_HIDDEN void __indirect_glTexCoord1f(GLfloat s);
extern _X_HIDDEN void __indirect_glTexCoord1fv(const GLfloat * v);
extern _X_HIDDEN void __indirect_glTexCoord1i(GLint s);
extern _X_HIDDEN void __indirect_glTexCoord1iv(const GLint * v);
extern _X_HIDDEN void __indirect_glTexCoord1s(GLshort s);
extern _X_HIDDEN void __indirect_glTexCoord1sv(const GLshort * v);
extern _X_HIDDEN void __indirect_glTexCoord2d(GLdouble s, GLdouble t);
extern _X_HIDDEN void __indirect_glTexCoord2dv(const GLdouble * v);
extern _X_HIDDEN void __indirect_glTexCoord2f(GLfloat s, GLfloat t);
extern _X_HIDDEN void __indirect_glTexCoord2fv(const GLfloat * v);
extern _X_HIDDEN void __indirect_glTexCoord2i(GLint s, GLint t);
extern _X_HIDDEN void __indirect_glTexCoord2iv(const GLint * v);
extern _X_HIDDEN void __indirect_glTexCoord2s(GLshort s, GLshort t);
extern _X_HIDDEN void __indirect_glTexCoord2sv(const GLshort * v);
extern _X_HIDDEN void __indirect_glTexCoord3d(GLdouble s, GLdouble t, GLdouble r);
extern _X_HIDDEN void __indirect_glTexCoord3dv(const GLdouble * v);
extern _X_HIDDEN void __indirect_glTexCoord3f(GLfloat s, GLfloat t, GLfloat r);
extern _X_HIDDEN void __indirect_glTexCoord3fv(const GLfloat * v);
extern _X_HIDDEN void __indirect_glTexCoord3i(GLint s, GLint t, GLint r);
extern _X_HIDDEN void __indirect_glTexCoord3iv(const GLint * v);
extern _X_HIDDEN void __indirect_glTexCoord3s(GLshort s, GLshort t, GLshort r);
extern _X_HIDDEN void __indirect_glTexCoord3sv(const GLshort * v);
extern _X_HIDDEN void __indirect_glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
extern _X_HIDDEN void __indirect_glTexCoord4dv(const GLdouble * v);
extern _X_HIDDEN void __indirect_glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
extern _X_HIDDEN void __indirect_glTexCoord4fv(const GLfloat * v);
extern _X_HIDDEN void __indirect_glTexCoord4i(GLint s, GLint t, GLint r, GLint q);
extern _X_HIDDEN void __indirect_glTexCoord4iv(const GLint * v);
extern _X_HIDDEN void __indirect_glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q);
extern _X_HIDDEN void __indirect_glTexCoord4sv(const GLshort * v);
extern _X_HIDDEN void __indirect_glVertex2d(GLdouble x, GLdouble y);
extern _X_HIDDEN void __indirect_glVertex2dv(const GLdouble * v);
extern _X_HIDDEN void __indirect_glVertex2f(GLfloat x, GLfloat y);
extern _X_HIDDEN void __indirect_glVertex2fv(const GLfloat * v);
extern _X_HIDDEN void __indirect_glVertex2i(GLint x, GLint y);
extern _X_HIDDEN void __indirect_glVertex2iv(const GLint * v);
extern _X_HIDDEN void __indirect_glVertex2s(GLshort x, GLshort y);
extern _X_HIDDEN void __indirect_glVertex2sv(const GLshort * v);
extern _X_HIDDEN void __indirect_glVertex3d(GLdouble x, GLdouble y, GLdouble z);
extern _X_HIDDEN void __indirect_glVertex3dv(const GLdouble * v);
extern _X_HIDDEN void __indirect_glVertex3f(GLfloat x, GLfloat y, GLfloat z);
extern _X_HIDDEN void __indirect_glVertex3fv(const GLfloat * v);
extern _X_HIDDEN void __indirect_glVertex3i(GLint x, GLint y, GLint z);
extern _X_HIDDEN void __indirect_glVertex3iv(const GLint * v);
extern _X_HIDDEN void __indirect_glVertex3s(GLshort x, GLshort y, GLshort z);
extern _X_HIDDEN void __indirect_glVertex3sv(const GLshort * v);
extern _X_HIDDEN void __indirect_glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern _X_HIDDEN void __indirect_glVertex4dv(const GLdouble * v);
extern _X_HIDDEN void __indirect_glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern _X_HIDDEN void __indirect_glVertex4fv(const GLfloat * v);
extern _X_HIDDEN void __indirect_glVertex4i(GLint x, GLint y, GLint z, GLint w);
extern _X_HIDDEN void __indirect_glVertex4iv(const GLint * v);
extern _X_HIDDEN void __indirect_glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w);
extern _X_HIDDEN void __indirect_glVertex4sv(const GLshort * v);
extern _X_HIDDEN void __indirect_glClipPlane(GLenum plane, const GLdouble * equation);
extern _X_HIDDEN void __indirect_glColorMaterial(GLenum face, GLenum mode);
extern _X_HIDDEN void __indirect_glCullFace(GLenum mode);
extern _X_HIDDEN void __indirect_glFogf(GLenum pname, GLfloat param);
extern _X_HIDDEN void __indirect_glFogfv(GLenum pname, const GLfloat * params);
extern _X_HIDDEN void __indirect_glFogi(GLenum pname, GLint param);
extern _X_HIDDEN void __indirect_glFogiv(GLenum pname, const GLint * params);
extern _X_HIDDEN void __indirect_glFrontFace(GLenum mode);
extern _X_HIDDEN void __indirect_glHint(GLenum target, GLenum mode);
extern _X_HIDDEN void __indirect_glLightf(GLenum light, GLenum pname, GLfloat param);
extern _X_HIDDEN void __indirect_glLightfv(GLenum light, GLenum pname, const GLfloat * params);
extern _X_HIDDEN void __indirect_glLighti(GLenum light, GLenum pname, GLint param);
extern _X_HIDDEN void __indirect_glLightiv(GLenum light, GLenum pname, const GLint * params);
extern _X_HIDDEN void __indirect_glLightModelf(GLenum pname, GLfloat param);
extern _X_HIDDEN void __indirect_glLightModelfv(GLenum pname, const GLfloat * params);
extern _X_HIDDEN void __indirect_glLightModeli(GLenum pname, GLint param);
extern _X_HIDDEN void __indirect_glLightModeliv(GLenum pname, const GLint * params);
extern _X_HIDDEN void __indirect_glLineStipple(GLint factor, GLushort pattern);
extern _X_HIDDEN void __indirect_glLineWidth(GLfloat width);
extern _X_HIDDEN void __indirect_glMaterialf(GLenum face, GLenum pname, GLfloat param);
extern _X_HIDDEN void __indirect_glMaterialfv(GLenum face, GLenum pname, const GLfloat * params);
extern _X_HIDDEN void __indirect_glMateriali(GLenum face, GLenum pname, GLint param);
extern _X_HIDDEN void __indirect_glMaterialiv(GLenum face, GLenum pname, const GLint * params);
extern _X_HIDDEN void __indirect_glPointSize(GLfloat size);
extern _X_HIDDEN void __indirect_glPolygonMode(GLenum face, GLenum mode);
extern _X_HIDDEN void __indirect_glPolygonStipple(const GLubyte * mask);
extern _X_HIDDEN void __indirect_glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
extern _X_HIDDEN void __indirect_glShadeModel(GLenum mode);
extern _X_HIDDEN void __indirect_glTexParameterf(GLenum target, GLenum pname, GLfloat param);
extern _X_HIDDEN void __indirect_glTexParameterfv(GLenum target, GLenum pname, const GLfloat * params);
extern _X_HIDDEN void __indirect_glTexParameteri(GLenum target, GLenum pname, GLint param);
extern _X_HIDDEN void __indirect_glTexParameteriv(GLenum target, GLenum pname, const GLint * params);
extern _X_HIDDEN void __indirect_glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid * pixels);
extern _X_HIDDEN void __indirect_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels);
extern _X_HIDDEN void __indirect_glTexEnvf(GLenum target, GLenum pname, GLfloat param);
extern _X_HIDDEN void __indirect_glTexEnvfv(GLenum target, GLenum pname, const GLfloat * params);
extern _X_HIDDEN void __indirect_glTexEnvi(GLenum target, GLenum pname, GLint param);
extern _X_HIDDEN void __indirect_glTexEnviv(GLenum target, GLenum pname, const GLint * params);
extern _X_HIDDEN void __indirect_glTexGend(GLenum coord, GLenum pname, GLdouble param);
extern _X_HIDDEN void __indirect_glTexGendv(GLenum coord, GLenum pname, const GLdouble * params);
extern _X_HIDDEN void __indirect_glTexGenf(GLenum coord, GLenum pname, GLfloat param);
extern _X_HIDDEN void __indirect_glTexGenfv(GLenum coord, GLenum pname, const GLfloat * params);
extern _X_HIDDEN void __indirect_glTexGeni(GLenum coord, GLenum pname, GLint param);
extern _X_HIDDEN void __indirect_glTexGeniv(GLenum coord, GLenum pname, const GLint * params);
extern _X_HIDDEN void __indirect_glFeedbackBuffer(GLsizei size, GLenum type, GLfloat * buffer);
extern _X_HIDDEN void __indirect_glSelectBuffer(GLsizei size, GLuint * buffer);
extern _X_HIDDEN GLint __indirect_glRenderMode(GLenum mode);
extern _X_HIDDEN void __indirect_glInitNames(void);
extern _X_HIDDEN void __indirect_glLoadName(GLuint name);
extern _X_HIDDEN void __indirect_glPassThrough(GLfloat token);
extern _X_HIDDEN void __indirect_glPopName(void);
extern _X_HIDDEN void __indirect_glPushName(GLuint name);
extern _X_HIDDEN void __indirect_glDrawBuffer(GLenum mode);
extern _X_HIDDEN void __indirect_glClear(GLbitfield mask);
extern _X_HIDDEN void __indirect_glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern _X_HIDDEN void __indirect_glClearIndex(GLfloat c);
extern _X_HIDDEN void __indirect_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern _X_HIDDEN void __indirect_glClearStencil(GLint s);
extern _X_HIDDEN void __indirect_glClearDepth(GLclampd depth);
extern _X_HIDDEN void __indirect_glStencilMask(GLuint mask);
extern _X_HIDDEN void __indirect_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
extern _X_HIDDEN void __indirect_glDepthMask(GLboolean flag);
extern _X_HIDDEN void __indirect_glIndexMask(GLuint mask);
extern _X_HIDDEN void __indirect_glAccum(GLenum op, GLfloat value);
extern _X_HIDDEN void __indirect_glDisable(GLenum cap);
extern _X_HIDDEN void __indirect_glEnable(GLenum cap);
extern _X_HIDDEN void __indirect_glFinish(void);
extern _X_HIDDEN void __indirect_glFlush(void);
extern _X_HIDDEN void __indirect_glPopAttrib(void);
extern _X_HIDDEN void __indirect_glPushAttrib(GLbitfield mask);
extern _X_HIDDEN void __indirect_glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble * points);
extern _X_HIDDEN void __indirect_glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat * points);
extern _X_HIDDEN void __indirect_glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble * points);
extern _X_HIDDEN void __indirect_glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat * points);
extern _X_HIDDEN void __indirect_glMapGrid1d(GLint un, GLdouble u1, GLdouble u2);
extern _X_HIDDEN void __indirect_glMapGrid1f(GLint un, GLfloat u1, GLfloat u2);
extern _X_HIDDEN void __indirect_glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
extern _X_HIDDEN void __indirect_glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
extern _X_HIDDEN void __indirect_glEvalCoord1d(GLdouble u);
extern _X_HIDDEN void __indirect_glEvalCoord1dv(const GLdouble * u);
extern _X_HIDDEN void __indirect_glEvalCoord1f(GLfloat u);
extern _X_HIDDEN void __indirect_glEvalCoord1fv(const GLfloat * u);
extern _X_HIDDEN void __indirect_glEvalCoord2d(GLdouble u, GLdouble v);
extern _X_HIDDEN void __indirect_glEvalCoord2dv(const GLdouble * u);
extern _X_HIDDEN void __indirect_glEvalCoord2f(GLfloat u, GLfloat v);
extern _X_HIDDEN void __indirect_glEvalCoord2fv(const GLfloat * u);
extern _X_HIDDEN void __indirect_glEvalMesh1(GLenum mode, GLint i1, GLint i2);
extern _X_HIDDEN void __indirect_glEvalPoint1(GLint i);
extern _X_HIDDEN void __indirect_glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
extern _X_HIDDEN void __indirect_glEvalPoint2(GLint i, GLint j);
extern _X_HIDDEN void __indirect_glAlphaFunc(GLenum func, GLclampf ref);
extern _X_HIDDEN void __indirect_glBlendFunc(GLenum sfactor, GLenum dfactor);
extern _X_HIDDEN void __indirect_glLogicOp(GLenum opcode);
extern _X_HIDDEN void __indirect_glStencilFunc(GLenum func, GLint ref, GLuint mask);
extern _X_HIDDEN void __indirect_glStencilOp(GLenum fail, GLenum zfail, GLenum zpass);
extern _X_HIDDEN void __indirect_glDepthFunc(GLenum func);
extern _X_HIDDEN void __indirect_glPixelZoom(GLfloat xfactor, GLfloat yfactor);
extern _X_HIDDEN void __indirect_glPixelTransferf(GLenum pname, GLfloat param);
extern _X_HIDDEN void __indirect_glPixelTransferi(GLenum pname, GLint param);
extern _X_HIDDEN void __indirect_glPixelStoref(GLenum pname, GLfloat param);
extern _X_HIDDEN void __indirect_glPixelStorei(GLenum pname, GLint param);
extern _X_HIDDEN void __indirect_glPixelMapfv(GLenum map, GLsizei mapsize, const GLfloat * values);
extern _X_HIDDEN void __indirect_glPixelMapuiv(GLenum map, GLsizei mapsize, const GLuint * values);
extern _X_HIDDEN void __indirect_glPixelMapusv(GLenum map, GLsizei mapsize, const GLushort * values);
extern _X_HIDDEN void __indirect_glReadBuffer(GLenum mode);
extern _X_HIDDEN void __indirect_glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
extern _X_HIDDEN void __indirect_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels);
extern _X_HIDDEN void __indirect_glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels);
extern _X_HIDDEN void __indirect_glGetBooleanv(GLenum pname, GLboolean * params);
extern _X_HIDDEN void __indirect_glGetClipPlane(GLenum plane, GLdouble * equation);
extern _X_HIDDEN void __indirect_glGetDoublev(GLenum pname, GLdouble * params);
extern _X_HIDDEN GLenum __indirect_glGetError(void);
extern _X_HIDDEN void __indirect_glGetFloatv(GLenum pname, GLfloat * params);
extern _X_HIDDEN void __indirect_glGetIntegerv(GLenum pname, GLint * params);
extern _X_HIDDEN void __indirect_glGetLightfv(GLenum light, GLenum pname, GLfloat * params);
extern _X_HIDDEN void __indirect_glGetLightiv(GLenum light, GLenum pname, GLint * params);
extern _X_HIDDEN void __indirect_glGetMapdv(GLenum target, GLenum query, GLdouble * v);
extern _X_HIDDEN void __indirect_glGetMapfv(GLenum target, GLenum query, GLfloat * v);
extern _X_HIDDEN void __indirect_glGetMapiv(GLenum target, GLenum query, GLint * v);
extern _X_HIDDEN void __indirect_glGetMaterialfv(GLenum face, GLenum pname, GLfloat * params);
extern _X_HIDDEN void __indirect_glGetMaterialiv(GLenum face, GLenum pname, GLint * params);
extern _X_HIDDEN void __indirect_glGetPixelMapfv(GLenum map, GLfloat * values);
extern _X_HIDDEN void __indirect_glGetPixelMapuiv(GLenum map, GLuint * values);
extern _X_HIDDEN void __indirect_glGetPixelMapusv(GLenum map, GLushort * values);
extern _X_HIDDEN void __indirect_glGetPolygonStipple(GLubyte * mask);
extern _X_HIDDEN const GLubyte * __indirect_glGetString(GLenum name);
extern _X_HIDDEN void __indirect_glGetTexEnvfv(GLenum target, GLenum pname, GLfloat * params);
extern _X_HIDDEN void __indirect_glGetTexEnviv(GLenum target, GLenum pname, GLint * params);
extern _X_HIDDEN void __indirect_glGetTexGendv(GLenum coord, GLenum pname, GLdouble * params);
extern _X_HIDDEN void __indirect_glGetTexGenfv(GLenum coord, GLenum pname, GLfloat * params);
extern _X_HIDDEN void __indirect_glGetTexGeniv(GLenum coord, GLenum pname, GLint * params);
extern _X_HIDDEN void __indirect_glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid * pixels);
extern _X_HIDDEN void __indirect_glGetTexParameterfv(GLenum target, GLenum pname, GLfloat * params);
extern _X_HIDDEN void __indirect_glGetTexParameteriv(GLenum target, GLenum pname, GLint * params);
extern _X_HIDDEN void __indirect_glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat * params);
extern _X_HIDDEN void __indirect_glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint * params);
extern _X_HIDDEN GLboolean __indirect_glIsEnabled(GLenum cap);
extern _X_HIDDEN GLboolean __indirect_glIsList(GLuint list);
extern _X_HIDDEN void __indirect_glDepthRange(GLclampd zNear, GLclampd zFar);
extern _X_HIDDEN void __indirect_glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern _X_HIDDEN void __indirect_glLoadIdentity(void);
extern _X_HIDDEN void __indirect_glLoadMatrixf(const GLfloat * m);
extern _X_HIDDEN void __indirect_glLoadMatrixd(const GLdouble * m);
extern _X_HIDDEN void __indirect_glMatrixMode(GLenum mode);
extern _X_HIDDEN void __indirect_glMultMatrixf(const GLfloat * m);
extern _X_HIDDEN void __indirect_glMultMatrixd(const GLdouble * m);
extern _X_HIDDEN void __indirect_glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern _X_HIDDEN void __indirect_glPopMatrix(void);
extern _X_HIDDEN void __indirect_glPushMatrix(void);
extern _X_HIDDEN void __indirect_glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
extern _X_HIDDEN void __indirect_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern _X_HIDDEN void __indirect_glScaled(GLdouble x, GLdouble y, GLdouble z);
extern _X_HIDDEN void __indirect_glScalef(GLfloat x, GLfloat y, GLfloat z);
extern _X_HIDDEN void __indirect_glTranslated(GLdouble x, GLdouble y, GLdouble z);
extern _X_HIDDEN void __indirect_glTranslatef(GLfloat x, GLfloat y, GLfloat z);
extern _X_HIDDEN void __indirect_glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
extern _X_HIDDEN void __indirect_glArrayElement(GLint i);
extern _X_HIDDEN void __indirect_glBindTexture(GLenum target, GLuint texture);
extern _X_HIDDEN void __indirect_glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);
extern _X_HIDDEN void __indirect_glDisableClientState(GLenum array);
extern _X_HIDDEN void __indirect_glDrawArrays(GLenum mode, GLint first, GLsizei count);
extern _X_HIDDEN void __indirect_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices);
extern _X_HIDDEN void __indirect_glEdgeFlagPointer(GLsizei stride, const GLvoid * pointer);
extern _X_HIDDEN void __indirect_glEnableClientState(GLenum array);
extern _X_HIDDEN void __indirect_glIndexPointer(GLenum type, GLsizei stride, const GLvoid * pointer);
extern _X_HIDDEN void __indirect_glIndexub(GLubyte c);
extern _X_HIDDEN void __indirect_glIndexubv(const GLubyte * c);
extern _X_HIDDEN void __indirect_glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid * pointer);
extern _X_HIDDEN void __indirect_glNormalPointer(GLenum type, GLsizei stride, const GLvoid * pointer);
extern _X_HIDDEN void __indirect_glPolygonOffset(GLfloat factor, GLfloat units);
extern _X_HIDDEN void __indirect_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);
extern _X_HIDDEN void __indirect_glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);
extern _X_HIDDEN GLboolean __indirect_glAreTexturesResident(GLsizei n, const GLuint * textures, GLboolean * residences);
GLAPI GLboolean GLAPIENTRY glAreTexturesResidentEXT(GLsizei n, const GLuint * textures, GLboolean * residences);
extern _X_HIDDEN void __indirect_glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
extern _X_HIDDEN void __indirect_glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern _X_HIDDEN void __indirect_glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
extern _X_HIDDEN void __indirect_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern _X_HIDDEN void __indirect_glDeleteTextures(GLsizei n, const GLuint * textures);
GLAPI void GLAPIENTRY glDeleteTexturesEXT(GLsizei n, const GLuint * textures);
extern _X_HIDDEN void __indirect_glGenTextures(GLsizei n, GLuint * textures);
GLAPI void GLAPIENTRY glGenTexturesEXT(GLsizei n, GLuint * textures);
extern _X_HIDDEN void __indirect_glGetPointerv(GLenum pname, GLvoid ** params);
extern _X_HIDDEN GLboolean __indirect_glIsTexture(GLuint texture);
GLAPI GLboolean GLAPIENTRY glIsTextureEXT(GLuint texture);
extern _X_HIDDEN void __indirect_glPrioritizeTextures(GLsizei n, const GLuint * textures, const GLclampf * priorities);
extern _X_HIDDEN void __indirect_glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid * pixels);
extern _X_HIDDEN void __indirect_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels);
extern _X_HIDDEN void __indirect_glPopClientAttrib(void);
extern _X_HIDDEN void __indirect_glPushClientAttrib(GLbitfield mask);
extern _X_HIDDEN void __indirect_glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern _X_HIDDEN void __indirect_glBlendEquation(GLenum mode);
extern _X_HIDDEN void __indirect_glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices);
extern _X_HIDDEN void __indirect_glColorTable(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * table);
extern _X_HIDDEN void __indirect_glColorTableParameterfv(GLenum target, GLenum pname, const GLfloat * params);
extern _X_HIDDEN void __indirect_glColorTableParameteriv(GLenum target, GLenum pname, const GLint * params);
extern _X_HIDDEN void __indirect_glCopyColorTable(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
extern _X_HIDDEN void __indirect_glGetColorTable(GLenum target, GLenum format, GLenum type, GLvoid * table);
GLAPI void GLAPIENTRY glGetColorTableEXT(GLenum target, GLenum format, GLenum type, GLvoid * table);
extern _X_HIDDEN void __indirect_glGetColorTableParameterfv(GLenum target, GLenum pname, GLfloat * params);
GLAPI void GLAPIENTRY glGetColorTableParameterfvEXT(GLenum target, GLenum pname, GLfloat * params);
extern _X_HIDDEN void __indirect_glGetColorTableParameteriv(GLenum target, GLenum pname, GLint * params);
GLAPI void GLAPIENTRY glGetColorTableParameterivEXT(GLenum target, GLenum pname, GLint * params);
extern _X_HIDDEN void __indirect_glColorSubTable(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid * data);
extern _X_HIDDEN void __indirect_glCopyColorSubTable(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);
extern _X_HIDDEN void __indirect_glConvolutionFilter1D(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * image);
extern _X_HIDDEN void __indirect_glConvolutionFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * image);
extern _X_HIDDEN void __indirect_glConvolutionParameterf(GLenum target, GLenum pname, GLfloat params);
extern _X_HIDDEN void __indirect_glConvolutionParameterfv(GLenum target, GLenum pname, const GLfloat * params);
extern _X_HIDDEN void __indirect_glConvolutionParameteri(GLenum target, GLenum pname, GLint params);
extern _X_HIDDEN void __indirect_glConvolutionParameteriv(GLenum target, GLenum pname, const GLint * params);
extern _X_HIDDEN void __indirect_glCopyConvolutionFilter1D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
extern _X_HIDDEN void __indirect_glCopyConvolutionFilter2D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
extern _X_HIDDEN void __indirect_glGetConvolutionFilter(GLenum target, GLenum format, GLenum type, GLvoid * image);
extern _X_HIDDEN void gl_dispatch_stub_356(GLenum target, GLenum format, GLenum type, GLvoid * image);
extern _X_HIDDEN void __indirect_glGetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat * params);
extern _X_HIDDEN void gl_dispatch_stub_357(GLenum target, GLenum pname, GLfloat * params);
extern _X_HIDDEN void __indirect_glGetConvolutionParameteriv(GLenum target, GLenum pname, GLint * params);
extern _X_HIDDEN void gl_dispatch_stub_358(GLenum target, GLenum pname, GLint * params);
extern _X_HIDDEN void __indirect_glGetSeparableFilter(GLenum target, GLenum format, GLenum type, GLvoid * row, GLvoid * column, GLvoid * span);
extern _X_HIDDEN void gl_dispatch_stub_359(GLenum target, GLenum format, GLenum type, GLvoid * row, GLvoid * column, GLvoid * span);
#define gl_dispatch_stub_GetSeparableFilterEXT gl_dispatch_stub_359
extern _X_HIDDEN void __indirect_glSeparableFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * row, const GLvoid * column);
extern _X_HIDDEN void __indirect_glGetHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values);
extern _X_HIDDEN void gl_dispatch_stub_361(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values);
extern _X_HIDDEN void __indirect_glGetHistogramParameterfv(GLenum target, GLenum pname, GLfloat * params);
extern _X_HIDDEN void gl_dispatch_stub_362(GLenum target, GLenum pname, GLfloat * params);
extern _X_HIDDEN void __indirect_glGetHistogramParameteriv(GLenum target, GLenum pname, GLint * params);
extern _X_HIDDEN void gl_dispatch_stub_363(GLenum target, GLenum pname, GLint * params);
extern _X_HIDDEN void __indirect_glGetMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values);
extern _X_HIDDEN void gl_dispatch_stub_364(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values);
extern _X_HIDDEN void __indirect_glGetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat * params);
extern _X_HIDDEN void gl_dispatch_stub_365(GLenum target, GLenum pname, GLfloat * params);
extern _X_HIDDEN void __indirect_glGetMinmaxParameteriv(GLenum target, GLenum pname, GLint * params);
extern _X_HIDDEN void gl_dispatch_stub_366(GLenum target, GLenum pname, GLint * params);
extern _X_HIDDEN void __indirect_glHistogram(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);
extern _X_HIDDEN void __indirect_glMinmax(GLenum target, GLenum internalformat, GLboolean sink);
extern _X_HIDDEN void __indirect_glResetHistogram(GLenum target);
extern _X_HIDDEN void __indirect_glResetMinmax(GLenum target);
extern _X_HIDDEN void __indirect_glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * pixels);
extern _X_HIDDEN void __indirect_glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * pixels);
extern _X_HIDDEN void __indirect_glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern _X_HIDDEN void __indirect_glActiveTextureARB(GLenum texture);
extern _X_HIDDEN void __indirect_glClientActiveTextureARB(GLenum texture);
extern _X_HIDDEN void __indirect_glMultiTexCoord1dARB(GLenum target, GLdouble s);
extern _X_HIDDEN void __indirect_glMultiTexCoord1dvARB(GLenum target, const GLdouble * v);
extern _X_HIDDEN void __indirect_glMultiTexCoord1fARB(GLenum target, GLfloat s);
extern _X_HIDDEN void __indirect_glMultiTexCoord1fvARB(GLenum target, const GLfloat * v);
extern _X_HIDDEN void __indirect_glMultiTexCoord1iARB(GLenum target, GLint s);
extern _X_HIDDEN void __indirect_glMultiTexCoord1ivARB(GLenum target, const GLint * v);
extern _X_HIDDEN void __indirect_glMultiTexCoord1sARB(GLenum target, GLshort s);
extern _X_HIDDEN void __indirect_glMultiTexCoord1svARB(GLenum target, const GLshort * v);
extern _X_HIDDEN void __indirect_glMultiTexCoord2dARB(GLenum target, GLdouble s, GLdouble t);
extern _X_HIDDEN void __indirect_glMultiTexCoord2dvARB(GLenum target, const GLdouble * v);
extern _X_HIDDEN void __indirect_glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t);
extern _X_HIDDEN void __indirect_glMultiTexCoord2fvARB(GLenum target, const GLfloat * v);
extern _X_HIDDEN void __indirect_glMultiTexCoord2iARB(GLenum target, GLint s, GLint t);
extern _X_HIDDEN void __indirect_glMultiTexCoord2ivARB(GLenum target, const GLint * v);
extern _X_HIDDEN void __indirect_glMultiTexCoord2sARB(GLenum target, GLshort s, GLshort t);
extern _X_HIDDEN void __indirect_glMultiTexCoord2svARB(GLenum target, const GLshort * v);
extern _X_HIDDEN void __indirect_glMultiTexCoord3dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r);
extern _X_HIDDEN void __indirect_glMultiTexCoord3dvARB(GLenum target, const GLdouble * v);
extern _X_HIDDEN void __indirect_glMultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r);
extern _X_HIDDEN void __indirect_glMultiTexCoord3fvARB(GLenum target, const GLfloat * v);
extern _X_HIDDEN void __indirect_glMultiTexCoord3iARB(GLenum target, GLint s, GLint t, GLint r);
extern _X_HIDDEN void __indirect_glMultiTexCoord3ivARB(GLenum target, const GLint * v);
extern _X_HIDDEN void __indirect_glMultiTexCoord3sARB(GLenum target, GLshort s, GLshort t, GLshort r);
extern _X_HIDDEN void __indirect_glMultiTexCoord3svARB(GLenum target, const GLshort * v);
extern _X_HIDDEN void __indirect_glMultiTexCoord4dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
extern _X_HIDDEN void __indirect_glMultiTexCoord4dvARB(GLenum target, const GLdouble * v);
extern _X_HIDDEN void __indirect_glMultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
extern _X_HIDDEN void __indirect_glMultiTexCoord4fvARB(GLenum target, const GLfloat * v);
extern _X_HIDDEN void __indirect_glMultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q);
extern _X_HIDDEN void __indirect_glMultiTexCoord4ivARB(GLenum target, const GLint * v);
extern _X_HIDDEN void __indirect_glMultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
extern _X_HIDDEN void __indirect_glMultiTexCoord4svARB(GLenum target, const GLshort * v);
extern _X_HIDDEN void __indirect_glLoadTransposeMatrixdARB(const GLdouble * m);
extern _X_HIDDEN void __indirect_glLoadTransposeMatrixfARB(const GLfloat * m);
extern _X_HIDDEN void __indirect_glMultTransposeMatrixdARB(const GLdouble * m);
extern _X_HIDDEN void __indirect_glMultTransposeMatrixfARB(const GLfloat * m);
extern _X_HIDDEN void __indirect_glSampleCoverageARB(GLclampf value, GLboolean invert);
extern _X_HIDDEN void __indirect_glCompressedTexImage1DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid * data);
extern _X_HIDDEN void __indirect_glCompressedTexImage2DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid * data);
extern _X_HIDDEN void __indirect_glCompressedTexImage3DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid * data);
extern _X_HIDDEN void __indirect_glCompressedTexSubImage1DARB(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid * data);
extern _X_HIDDEN void __indirect_glCompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid * data);
extern _X_HIDDEN void __indirect_glCompressedTexSubImage3DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid * data);
extern _X_HIDDEN void __indirect_glGetCompressedTexImageARB(GLenum target, GLint level, GLvoid * img);
extern _X_HIDDEN void __indirect_glDisableVertexAttribArrayARB(GLuint index);
extern _X_HIDDEN void __indirect_glEnableVertexAttribArrayARB(GLuint index);
extern _X_HIDDEN void __indirect_glGetProgramEnvParameterdvARB(GLenum target, GLuint index, GLdouble * params);
extern _X_HIDDEN void __indirect_glGetProgramEnvParameterfvARB(GLenum target, GLuint index, GLfloat * params);
extern _X_HIDDEN void __indirect_glGetProgramLocalParameterdvARB(GLenum target, GLuint index, GLdouble * params);
extern _X_HIDDEN void __indirect_glGetProgramLocalParameterfvARB(GLenum target, GLuint index, GLfloat * params);
extern _X_HIDDEN void __indirect_glGetProgramStringARB(GLenum target, GLenum pname, GLvoid * string);
extern _X_HIDDEN void __indirect_glGetProgramivARB(GLenum target, GLenum pname, GLint * params);
extern _X_HIDDEN void __indirect_glGetVertexAttribdvARB(GLuint index, GLenum pname, GLdouble * params);
extern _X_HIDDEN void __indirect_glGetVertexAttribfvARB(GLuint index, GLenum pname, GLfloat * params);
extern _X_HIDDEN void __indirect_glGetVertexAttribivARB(GLuint index, GLenum pname, GLint * params);
extern _X_HIDDEN void __indirect_glProgramEnvParameter4dARB(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern _X_HIDDEN void __indirect_glProgramEnvParameter4dvARB(GLenum target, GLuint index, const GLdouble * params);
extern _X_HIDDEN void __indirect_glProgramEnvParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern _X_HIDDEN void __indirect_glProgramEnvParameter4fvARB(GLenum target, GLuint index, const GLfloat * params);
extern _X_HIDDEN void __indirect_glProgramLocalParameter4dARB(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern _X_HIDDEN void __indirect_glProgramLocalParameter4dvARB(GLenum target, GLuint index, const GLdouble * params);
extern _X_HIDDEN void __indirect_glProgramLocalParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern _X_HIDDEN void __indirect_glProgramLocalParameter4fvARB(GLenum target, GLuint index, const GLfloat * params);
extern _X_HIDDEN void __indirect_glProgramStringARB(GLenum target, GLenum format, GLsizei len, const GLvoid * string);
extern _X_HIDDEN void __indirect_glVertexAttrib1dARB(GLuint index, GLdouble x);
extern _X_HIDDEN void __indirect_glVertexAttrib1dvARB(GLuint index, const GLdouble * v);
extern _X_HIDDEN void __indirect_glVertexAttrib1fARB(GLuint index, GLfloat x);
extern _X_HIDDEN void __indirect_glVertexAttrib1fvARB(GLuint index, const GLfloat * v);
extern _X_HIDDEN void __indirect_glVertexAttrib1sARB(GLuint index, GLshort x);
extern _X_HIDDEN void __indirect_glVertexAttrib1svARB(GLuint index, const GLshort * v);
extern _X_HIDDEN void __indirect_glVertexAttrib2dARB(GLuint index, GLdouble x, GLdouble y);
extern _X_HIDDEN void __indirect_glVertexAttrib2dvARB(GLuint index, const GLdouble * v);
extern _X_HIDDEN void __indirect_glVertexAttrib2fARB(GLuint index, GLfloat x, GLfloat y);
extern _X_HIDDEN void __indirect_glVertexAttrib2fvARB(GLuint index, const GLfloat * v);
extern _X_HIDDEN void __indirect_glVertexAttrib2sARB(GLuint index, GLshort x, GLshort y);
extern _X_HIDDEN void __indirect_glVertexAttrib2svARB(GLuint index, const GLshort * v);
extern _X_HIDDEN void __indirect_glVertexAttrib3dARB(GLuint index, GLdouble x, GLdouble y, GLdouble z);
extern _X_HIDDEN void __indirect_glVertexAttrib3dvARB(GLuint index, const GLdouble * v);
extern _X_HIDDEN void __indirect_glVertexAttrib3fARB(GLuint index, GLfloat x, GLfloat y, GLfloat z);
extern _X_HIDDEN void __indirect_glVertexAttrib3fvARB(GLuint index, const GLfloat * v);
extern _X_HIDDEN void __indirect_glVertexAttrib3sARB(GLuint index, GLshort x, GLshort y, GLshort z);
extern _X_HIDDEN void __indirect_glVertexAttrib3svARB(GLuint index, const GLshort * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4NbvARB(GLuint index, const GLbyte * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4NivARB(GLuint index, const GLint * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4NsvARB(GLuint index, const GLshort * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4NubARB(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
extern _X_HIDDEN void __indirect_glVertexAttrib4NubvARB(GLuint index, const GLubyte * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4NuivARB(GLuint index, const GLuint * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4NusvARB(GLuint index, const GLushort * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4bvARB(GLuint index, const GLbyte * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4dARB(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern _X_HIDDEN void __indirect_glVertexAttrib4dvARB(GLuint index, const GLdouble * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4fARB(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern _X_HIDDEN void __indirect_glVertexAttrib4fvARB(GLuint index, const GLfloat * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4ivARB(GLuint index, const GLint * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4sARB(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
extern _X_HIDDEN void __indirect_glVertexAttrib4svARB(GLuint index, const GLshort * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4ubvARB(GLuint index, const GLubyte * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4uivARB(GLuint index, const GLuint * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4usvARB(GLuint index, const GLushort * v);
extern _X_HIDDEN void __indirect_glVertexAttribPointerARB(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer);
extern _X_HIDDEN void __indirect_glBeginQueryARB(GLenum target, GLuint id);
extern _X_HIDDEN void __indirect_glDeleteQueriesARB(GLsizei n, const GLuint * ids);
extern _X_HIDDEN void __indirect_glEndQueryARB(GLenum target);
extern _X_HIDDEN void __indirect_glGenQueriesARB(GLsizei n, GLuint * ids);
extern _X_HIDDEN void __indirect_glGetQueryObjectivARB(GLuint id, GLenum pname, GLint * params);
extern _X_HIDDEN void __indirect_glGetQueryObjectuivARB(GLuint id, GLenum pname, GLuint * params);
extern _X_HIDDEN void __indirect_glGetQueryivARB(GLenum target, GLenum pname, GLint * params);
extern _X_HIDDEN GLboolean __indirect_glIsQueryARB(GLuint id);
extern _X_HIDDEN void __indirect_glDrawBuffersARB(GLsizei n, const GLenum * bufs);
extern _X_HIDDEN void __indirect_glClampColorARB(GLenum target, GLenum clamp);
extern _X_HIDDEN void __indirect_glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
extern _X_HIDDEN void __indirect_glSampleMaskSGIS(GLclampf value, GLboolean invert);
extern _X_HIDDEN void __indirect_glSamplePatternSGIS(GLenum pattern);
extern _X_HIDDEN void __indirect_glColorPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer);
extern _X_HIDDEN void __indirect_glEdgeFlagPointerEXT(GLsizei stride, GLsizei count, const GLboolean * pointer);
extern _X_HIDDEN void __indirect_glIndexPointerEXT(GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer);
extern _X_HIDDEN void __indirect_glNormalPointerEXT(GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer);
extern _X_HIDDEN void __indirect_glTexCoordPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer);
extern _X_HIDDEN void __indirect_glVertexPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer);
extern _X_HIDDEN void __indirect_glPointParameterfEXT(GLenum pname, GLfloat param);
extern _X_HIDDEN void __indirect_glPointParameterfvEXT(GLenum pname, const GLfloat * params);
extern _X_HIDDEN void __indirect_glSecondaryColor3bEXT(GLbyte red, GLbyte green, GLbyte blue);
extern _X_HIDDEN void __indirect_glSecondaryColor3bvEXT(const GLbyte * v);
extern _X_HIDDEN void __indirect_glSecondaryColor3dEXT(GLdouble red, GLdouble green, GLdouble blue);
extern _X_HIDDEN void __indirect_glSecondaryColor3dvEXT(const GLdouble * v);
extern _X_HIDDEN void __indirect_glSecondaryColor3fEXT(GLfloat red, GLfloat green, GLfloat blue);
extern _X_HIDDEN void __indirect_glSecondaryColor3fvEXT(const GLfloat * v);
extern _X_HIDDEN void __indirect_glSecondaryColor3iEXT(GLint red, GLint green, GLint blue);
extern _X_HIDDEN void __indirect_glSecondaryColor3ivEXT(const GLint * v);
extern _X_HIDDEN void __indirect_glSecondaryColor3sEXT(GLshort red, GLshort green, GLshort blue);
extern _X_HIDDEN void __indirect_glSecondaryColor3svEXT(const GLshort * v);
extern _X_HIDDEN void __indirect_glSecondaryColor3ubEXT(GLubyte red, GLubyte green, GLubyte blue);
extern _X_HIDDEN void __indirect_glSecondaryColor3ubvEXT(const GLubyte * v);
extern _X_HIDDEN void __indirect_glSecondaryColor3uiEXT(GLuint red, GLuint green, GLuint blue);
extern _X_HIDDEN void __indirect_glSecondaryColor3uivEXT(const GLuint * v);
extern _X_HIDDEN void __indirect_glSecondaryColor3usEXT(GLushort red, GLushort green, GLushort blue);
extern _X_HIDDEN void __indirect_glSecondaryColor3usvEXT(const GLushort * v);
extern _X_HIDDEN void __indirect_glSecondaryColorPointerEXT(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);
extern _X_HIDDEN void __indirect_glMultiDrawArraysEXT(GLenum mode, const GLint * first, const GLsizei * count, GLsizei primcount);
extern _X_HIDDEN void __indirect_glMultiDrawElementsEXT(GLenum mode, const GLsizei * count, GLenum type, const GLvoid ** indices, GLsizei primcount);
extern _X_HIDDEN void __indirect_glFogCoordPointerEXT(GLenum type, GLsizei stride, const GLvoid * pointer);
extern _X_HIDDEN void __indirect_glFogCoorddEXT(GLdouble coord);
extern _X_HIDDEN void __indirect_glFogCoorddvEXT(const GLdouble * coord);
extern _X_HIDDEN void __indirect_glFogCoordfEXT(GLfloat coord);
extern _X_HIDDEN void __indirect_glFogCoordfvEXT(const GLfloat * coord);
extern _X_HIDDEN void __indirect_glBlendFuncSeparateEXT(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
extern _X_HIDDEN void __indirect_glWindowPos2dMESA(GLdouble x, GLdouble y);
extern _X_HIDDEN void __indirect_glWindowPos2dvMESA(const GLdouble * v);
extern _X_HIDDEN void __indirect_glWindowPos2fMESA(GLfloat x, GLfloat y);
extern _X_HIDDEN void __indirect_glWindowPos2fvMESA(const GLfloat * v);
extern _X_HIDDEN void __indirect_glWindowPos2iMESA(GLint x, GLint y);
extern _X_HIDDEN void __indirect_glWindowPos2ivMESA(const GLint * v);
extern _X_HIDDEN void __indirect_glWindowPos2sMESA(GLshort x, GLshort y);
extern _X_HIDDEN void __indirect_glWindowPos2svMESA(const GLshort * v);
extern _X_HIDDEN void __indirect_glWindowPos3dMESA(GLdouble x, GLdouble y, GLdouble z);
extern _X_HIDDEN void __indirect_glWindowPos3dvMESA(const GLdouble * v);
extern _X_HIDDEN void __indirect_glWindowPos3fMESA(GLfloat x, GLfloat y, GLfloat z);
extern _X_HIDDEN void __indirect_glWindowPos3fvMESA(const GLfloat * v);
extern _X_HIDDEN void __indirect_glWindowPos3iMESA(GLint x, GLint y, GLint z);
extern _X_HIDDEN void __indirect_glWindowPos3ivMESA(const GLint * v);
extern _X_HIDDEN void __indirect_glWindowPos3sMESA(GLshort x, GLshort y, GLshort z);
extern _X_HIDDEN void __indirect_glWindowPos3svMESA(const GLshort * v);
extern _X_HIDDEN GLboolean __indirect_glAreProgramsResidentNV(GLsizei n, const GLuint * ids, GLboolean * residences);
extern _X_HIDDEN void __indirect_glBindProgramNV(GLenum target, GLuint program);
extern _X_HIDDEN void __indirect_glDeleteProgramsNV(GLsizei n, const GLuint * programs);
extern _X_HIDDEN void __indirect_glExecuteProgramNV(GLenum target, GLuint id, const GLfloat * params);
extern _X_HIDDEN void __indirect_glGenProgramsNV(GLsizei n, GLuint * programs);
extern _X_HIDDEN void __indirect_glGetProgramParameterdvNV(GLenum target, GLuint index, GLenum pname, GLdouble * params);
extern _X_HIDDEN void __indirect_glGetProgramParameterfvNV(GLenum target, GLuint index, GLenum pname, GLfloat * params);
extern _X_HIDDEN void __indirect_glGetProgramStringNV(GLuint id, GLenum pname, GLubyte * program);
extern _X_HIDDEN void __indirect_glGetProgramivNV(GLuint id, GLenum pname, GLint * params);
extern _X_HIDDEN void __indirect_glGetTrackMatrixivNV(GLenum target, GLuint address, GLenum pname, GLint * params);
extern _X_HIDDEN void __indirect_glGetVertexAttribPointervNV(GLuint index, GLenum pname, GLvoid ** pointer);
extern _X_HIDDEN void __indirect_glGetVertexAttribdvNV(GLuint index, GLenum pname, GLdouble * params);
extern _X_HIDDEN void __indirect_glGetVertexAttribfvNV(GLuint index, GLenum pname, GLfloat * params);
extern _X_HIDDEN void __indirect_glGetVertexAttribivNV(GLuint index, GLenum pname, GLint * params);
extern _X_HIDDEN GLboolean __indirect_glIsProgramNV(GLuint program);
extern _X_HIDDEN void __indirect_glLoadProgramNV(GLenum target, GLuint id, GLsizei len, const GLubyte * program);
extern _X_HIDDEN void __indirect_glProgramParameters4dvNV(GLenum target, GLuint index, GLsizei num, const GLdouble * params);
extern _X_HIDDEN void __indirect_glProgramParameters4fvNV(GLenum target, GLuint index, GLsizei num, const GLfloat * params);
extern _X_HIDDEN void __indirect_glRequestResidentProgramsNV(GLsizei n, const GLuint * ids);
extern _X_HIDDEN void __indirect_glTrackMatrixNV(GLenum target, GLuint address, GLenum matrix, GLenum transform);
extern _X_HIDDEN void __indirect_glVertexAttrib1dNV(GLuint index, GLdouble x);
extern _X_HIDDEN void __indirect_glVertexAttrib1dvNV(GLuint index, const GLdouble * v);
extern _X_HIDDEN void __indirect_glVertexAttrib1fNV(GLuint index, GLfloat x);
extern _X_HIDDEN void __indirect_glVertexAttrib1fvNV(GLuint index, const GLfloat * v);
extern _X_HIDDEN void __indirect_glVertexAttrib1sNV(GLuint index, GLshort x);
extern _X_HIDDEN void __indirect_glVertexAttrib1svNV(GLuint index, const GLshort * v);
extern _X_HIDDEN void __indirect_glVertexAttrib2dNV(GLuint index, GLdouble x, GLdouble y);
extern _X_HIDDEN void __indirect_glVertexAttrib2dvNV(GLuint index, const GLdouble * v);
extern _X_HIDDEN void __indirect_glVertexAttrib2fNV(GLuint index, GLfloat x, GLfloat y);
extern _X_HIDDEN void __indirect_glVertexAttrib2fvNV(GLuint index, const GLfloat * v);
extern _X_HIDDEN void __indirect_glVertexAttrib2sNV(GLuint index, GLshort x, GLshort y);
extern _X_HIDDEN void __indirect_glVertexAttrib2svNV(GLuint index, const GLshort * v);
extern _X_HIDDEN void __indirect_glVertexAttrib3dNV(GLuint index, GLdouble x, GLdouble y, GLdouble z);
extern _X_HIDDEN void __indirect_glVertexAttrib3dvNV(GLuint index, const GLdouble * v);
extern _X_HIDDEN void __indirect_glVertexAttrib3fNV(GLuint index, GLfloat x, GLfloat y, GLfloat z);
extern _X_HIDDEN void __indirect_glVertexAttrib3fvNV(GLuint index, const GLfloat * v);
extern _X_HIDDEN void __indirect_glVertexAttrib3sNV(GLuint index, GLshort x, GLshort y, GLshort z);
extern _X_HIDDEN void __indirect_glVertexAttrib3svNV(GLuint index, const GLshort * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4dNV(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern _X_HIDDEN void __indirect_glVertexAttrib4dvNV(GLuint index, const GLdouble * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4fNV(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern _X_HIDDEN void __indirect_glVertexAttrib4fvNV(GLuint index, const GLfloat * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4sNV(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
extern _X_HIDDEN void __indirect_glVertexAttrib4svNV(GLuint index, const GLshort * v);
extern _X_HIDDEN void __indirect_glVertexAttrib4ubNV(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
extern _X_HIDDEN void __indirect_glVertexAttrib4ubvNV(GLuint index, const GLubyte * v);
extern _X_HIDDEN void __indirect_glVertexAttribPointerNV(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);
extern _X_HIDDEN void __indirect_glVertexAttribs1dvNV(GLuint index, GLsizei n, const GLdouble * v);
extern _X_HIDDEN void __indirect_glVertexAttribs1fvNV(GLuint index, GLsizei n, const GLfloat * v);
extern _X_HIDDEN void __indirect_glVertexAttribs1svNV(GLuint index, GLsizei n, const GLshort * v);
extern _X_HIDDEN void __indirect_glVertexAttribs2dvNV(GLuint index, GLsizei n, const GLdouble * v);
extern _X_HIDDEN void __indirect_glVertexAttribs2fvNV(GLuint index, GLsizei n, const GLfloat * v);
extern _X_HIDDEN void __indirect_glVertexAttribs2svNV(GLuint index, GLsizei n, const GLshort * v);
extern _X_HIDDEN void __indirect_glVertexAttribs3dvNV(GLuint index, GLsizei n, const GLdouble * v);
extern _X_HIDDEN void __indirect_glVertexAttribs3fvNV(GLuint index, GLsizei n, const GLfloat * v);
extern _X_HIDDEN void __indirect_glVertexAttribs3svNV(GLuint index, GLsizei n, const GLshort * v);
extern _X_HIDDEN void __indirect_glVertexAttribs4dvNV(GLuint index, GLsizei n, const GLdouble * v);
extern _X_HIDDEN void __indirect_glVertexAttribs4fvNV(GLuint index, GLsizei n, const GLfloat * v);
extern _X_HIDDEN void __indirect_glVertexAttribs4svNV(GLuint index, GLsizei n, const GLshort * v);
extern _X_HIDDEN void __indirect_glVertexAttribs4ubvNV(GLuint index, GLsizei n, const GLubyte * v);
extern _X_HIDDEN void __indirect_glPointParameteriNV(GLenum pname, GLint param);
extern _X_HIDDEN void __indirect_glPointParameterivNV(GLenum pname, const GLint * params);
extern _X_HIDDEN void __indirect_glActiveStencilFaceEXT(GLenum face);
extern _X_HIDDEN void __indirect_glGetProgramNamedParameterdvNV(GLuint id, GLsizei len, const GLubyte * name, GLdouble * params);
extern _X_HIDDEN void __indirect_glGetProgramNamedParameterfvNV(GLuint id, GLsizei len, const GLubyte * name, GLfloat * params);
extern _X_HIDDEN void __indirect_glProgramNamedParameter4dNV(GLuint id, GLsizei len, const GLubyte * name, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern _X_HIDDEN void __indirect_glProgramNamedParameter4dvNV(GLuint id, GLsizei len, const GLubyte * name, const GLdouble * v);
extern _X_HIDDEN void __indirect_glProgramNamedParameter4fNV(GLuint id, GLsizei len, const GLubyte * name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern _X_HIDDEN void __indirect_glProgramNamedParameter4fvNV(GLuint id, GLsizei len, const GLubyte * name, const GLfloat * v);
extern _X_HIDDEN void __indirect_glBlendEquationSeparateEXT(GLenum modeRGB, GLenum modeA);
extern _X_HIDDEN void __indirect_glBindFramebufferEXT(GLenum target, GLuint framebuffer);
extern _X_HIDDEN void __indirect_glBindRenderbufferEXT(GLenum target, GLuint renderbuffer);
extern _X_HIDDEN GLenum __indirect_glCheckFramebufferStatusEXT(GLenum target);
extern _X_HIDDEN void __indirect_glDeleteFramebuffersEXT(GLsizei n, const GLuint * framebuffers);
extern _X_HIDDEN void __indirect_glDeleteRenderbuffersEXT(GLsizei n, const GLuint * renderbuffers);
extern _X_HIDDEN void __indirect_glFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
extern _X_HIDDEN void __indirect_glFramebufferTexture1DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern _X_HIDDEN void __indirect_glFramebufferTexture2DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern _X_HIDDEN void __indirect_glFramebufferTexture3DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
extern _X_HIDDEN void __indirect_glGenFramebuffersEXT(GLsizei n, GLuint * framebuffers);
extern _X_HIDDEN void __indirect_glGenRenderbuffersEXT(GLsizei n, GLuint * renderbuffers);
extern _X_HIDDEN void __indirect_glGenerateMipmapEXT(GLenum target);
extern _X_HIDDEN void __indirect_glGetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment, GLenum pname, GLint * params);
extern _X_HIDDEN void __indirect_glGetRenderbufferParameterivEXT(GLenum target, GLenum pname, GLint * params);
extern _X_HIDDEN GLboolean __indirect_glIsFramebufferEXT(GLuint framebuffer);
extern _X_HIDDEN GLboolean __indirect_glIsRenderbufferEXT(GLuint renderbuffer);
extern _X_HIDDEN void __indirect_glRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
extern _X_HIDDEN void __indirect_glBlitFramebufferEXT(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
extern _X_HIDDEN void __indirect_glFramebufferTextureLayerEXT(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);

#ifdef GLX_SHARED_GLAPI
extern _X_HIDDEN void (*__indirect_get_proc_address(const char *name))(void);
#endif

#  undef FASTCALL
#  undef NOINLINE

#endif /* !defined( _INDIRECT_H_ ) */
