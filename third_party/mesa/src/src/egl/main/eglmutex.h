/**************************************************************************
 *
 * Copyright 2009 Chia-I Wu <olvaffe@gmail.com>
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


#ifndef EGLMUTEX_INCLUDED
#define EGLMUTEX_INCLUDED

#include "eglcompiler.h"

#ifdef HAVE_PTHREAD
#include <pthread.h>

typedef pthread_mutex_t _EGLMutex;

static INLINE void _eglInitMutex(_EGLMutex *m)
{
   pthread_mutex_init(m, NULL);
}

static INLINE void
_eglDestroyMutex(_EGLMutex *m)
{
   pthread_mutex_destroy(m);
}

static INLINE void
_eglLockMutex(_EGLMutex *m)
{
   pthread_mutex_lock(m);
}

static INLINE void
_eglUnlockMutex(_EGLMutex *m)
{
   pthread_mutex_unlock(m);
}

#define _EGL_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define _EGL_DECLARE_MUTEX(m) \
   _EGLMutex m = _EGL_MUTEX_INITIALIZER

#else

typedef int _EGLMutex;
static INLINE void _eglInitMutex(_EGLMutex *m) { (void) m; }
static INLINE void _eglDestroyMutex(_EGLMutex *m) { (void) m; }
static INLINE void _eglLockMutex(_EGLMutex *m) { (void) m; }
static INLINE void _eglUnlockMutex(_EGLMutex *m) { (void) m; }

#define _EGL_MUTEX_INITIALIZER 0
#define _EGL_DECLARE_MUTEX(m) \
   _EGLMutex m = _EGL_MUTEX_INITIALIZER

#endif

#endif /* EGLMUTEX_INCLUDED */
