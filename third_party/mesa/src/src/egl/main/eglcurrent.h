/**************************************************************************
 *
 * Copyright 2009-2010 Chia-I Wu <olvaffe@gmail.com>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#ifndef EGLCURRENT_INCLUDED
#define EGLCURRENT_INCLUDED


#include "egltypedefs.h"


#define _EGL_API_ALL_BITS \
   (EGL_OPENGL_ES_BIT   | \
    EGL_OPENVG_BIT      | \
    EGL_OPENGL_ES2_BIT  | \
    EGL_OPENGL_BIT)


#define _EGL_API_FIRST_API EGL_OPENGL_ES_API
#define _EGL_API_LAST_API EGL_OPENGL_API
#define _EGL_API_NUM_APIS (_EGL_API_LAST_API - _EGL_API_FIRST_API + 1)


/**
 * Per-thread info
 */
struct _egl_thread_info
{
   EGLint LastError;
   _EGLContext *CurrentContexts[_EGL_API_NUM_APIS];
   /* use index for fast access to current context */
   EGLint CurrentAPIIndex;
};


/**
 * Return true if a client API enum is recognized.
 */
static INLINE EGLBoolean
_eglIsApiValid(EGLenum api)
{
   return (api >= _EGL_API_FIRST_API && api <= _EGL_API_LAST_API);
}


/**
 * Convert a client API enum to an index, for use by thread info.
 * The client API enum is assumed to be valid.
 */
static INLINE EGLint
_eglConvertApiToIndex(EGLenum api)
{
   return api - _EGL_API_FIRST_API;
}


/**
 * Convert an index, used by thread info, to a client API enum.
 * The index is assumed to be valid.
 */
static INLINE EGLenum
_eglConvertApiFromIndex(EGLint idx)
{
   return _EGL_API_FIRST_API + idx;
}


PUBLIC _EGLThreadInfo *
_eglGetCurrentThread(void);


extern void
_eglDestroyCurrentThread(void);


extern EGLBoolean
_eglIsCurrentThreadDummy(void);


PUBLIC _EGLContext *
_eglGetAPIContext(EGLenum api);


PUBLIC _EGLContext *
_eglGetCurrentContext(void);


PUBLIC EGLBoolean
_eglError(EGLint errCode, const char *msg);


#endif /* EGLCURRENT_INCLUDED */
