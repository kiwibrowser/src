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


#include <stdlib.h>
#include <string.h>
#include "egllog.h"
#include "eglmutex.h"
#include "eglcurrent.h"
#include "eglglobals.h"


/* This should be kept in sync with _eglInitThreadInfo() */
#define _EGL_THREAD_INFO_INITIALIZER \
   { EGL_SUCCESS, { NULL }, 0 }

/* a fallback thread info to guarantee that every thread always has one */
static _EGLThreadInfo dummy_thread = _EGL_THREAD_INFO_INITIALIZER;


#if HAVE_PTHREAD
#include <pthread.h>

static _EGL_DECLARE_MUTEX(_egl_TSDMutex);
static EGLBoolean _egl_TSDInitialized;
static pthread_key_t _egl_TSD;
static void (*_egl_FreeTSD)(_EGLThreadInfo *);

#ifdef GLX_USE_TLS
static __thread const _EGLThreadInfo *_egl_TLS
   __attribute__ ((tls_model("initial-exec")));
#endif

static INLINE void _eglSetTSD(const _EGLThreadInfo *t)
{
   pthread_setspecific(_egl_TSD, (const void *) t);
#ifdef GLX_USE_TLS
   _egl_TLS = t;
#endif
}

static INLINE _EGLThreadInfo *_eglGetTSD(void)
{
#ifdef GLX_USE_TLS
   return (_EGLThreadInfo *) _egl_TLS;
#else
   return (_EGLThreadInfo *) pthread_getspecific(_egl_TSD);
#endif
}

static INLINE void _eglFiniTSD(void)
{
   _eglLockMutex(&_egl_TSDMutex);
   if (_egl_TSDInitialized) {
      _EGLThreadInfo *t = _eglGetTSD();

      _egl_TSDInitialized = EGL_FALSE;
      if (t && _egl_FreeTSD)
         _egl_FreeTSD((void *) t);
      pthread_key_delete(_egl_TSD);
   }
   _eglUnlockMutex(&_egl_TSDMutex);
}

static INLINE EGLBoolean _eglInitTSD(void (*dtor)(_EGLThreadInfo *))
{
   if (!_egl_TSDInitialized) {
      _eglLockMutex(&_egl_TSDMutex);

      /* check again after acquiring lock */
      if (!_egl_TSDInitialized) {
         if (pthread_key_create(&_egl_TSD, (void (*)(void *)) dtor) != 0) {
            _eglUnlockMutex(&_egl_TSDMutex);
            return EGL_FALSE;
         }
         _egl_FreeTSD = dtor;
         _eglAddAtExitCall(_eglFiniTSD);
         _egl_TSDInitialized = EGL_TRUE;
      }

      _eglUnlockMutex(&_egl_TSDMutex);
   }

   return EGL_TRUE;
}

#else /* HAVE_PTHREAD */
static const _EGLThreadInfo *_egl_TSD;
static void (*_egl_FreeTSD)(_EGLThreadInfo *);

static INLINE void _eglSetTSD(const _EGLThreadInfo *t)
{
   _egl_TSD = t;
}

static INLINE _EGLThreadInfo *_eglGetTSD(void)
{
   return (_EGLThreadInfo *) _egl_TSD;
}

static INLINE void _eglFiniTSD(void)
{
   if (_egl_FreeTSD && _egl_TSD)
      _egl_FreeTSD((_EGLThreadInfo *) _egl_TSD);
}

static INLINE EGLBoolean _eglInitTSD(void (*dtor)(_EGLThreadInfo *))
{
   if (!_egl_FreeTSD && dtor) {
      _egl_FreeTSD = dtor;
      _eglAddAtExitCall(_eglFiniTSD);
   }
   return EGL_TRUE;
}

#endif /* !HAVE_PTHREAD */


static void
_eglInitThreadInfo(_EGLThreadInfo *t)
{
   memset(t, 0, sizeof(*t));
   t->LastError = EGL_SUCCESS;
   /* default, per EGL spec */
   t->CurrentAPIIndex = _eglConvertApiToIndex(EGL_OPENGL_ES_API);
}


/**
 * Allocate and init a new _EGLThreadInfo object.
 */
static _EGLThreadInfo *
_eglCreateThreadInfo(void)
{
   _EGLThreadInfo *t = (_EGLThreadInfo *) calloc(1, sizeof(_EGLThreadInfo));
   if (t)
      _eglInitThreadInfo(t);
   else
      t = &dummy_thread;
   return t;
}


/**
 * Delete/free a _EGLThreadInfo object.
 */
static void
_eglDestroyThreadInfo(_EGLThreadInfo *t)
{
   if (t != &dummy_thread)
      free(t);
}


/**
 * Make sure TSD is initialized and return current value.
 */
static INLINE _EGLThreadInfo *
_eglCheckedGetTSD(void)
{
   if (_eglInitTSD(&_eglDestroyThreadInfo) != EGL_TRUE) {
      _eglLog(_EGL_FATAL, "failed to initialize \"current\" system");
      return NULL;
   }

   return _eglGetTSD();
}


/**
 * Return the calling thread's thread info.
 * If the calling thread nevers calls this function before, or if its thread
 * info was destroyed, a new one is created.  This function never returns NULL.
 * In the case allocation fails, a dummy one is returned.  See also
 * _eglIsCurrentThreadDummy.
 */
_EGLThreadInfo *
_eglGetCurrentThread(void)
{
   _EGLThreadInfo *t = _eglCheckedGetTSD();
   if (!t) {
      t = _eglCreateThreadInfo();
      _eglSetTSD(t);
   }

   return t;
}


/**
 * Destroy the calling thread's thread info.
 */
void
_eglDestroyCurrentThread(void)
{
   _EGLThreadInfo *t = _eglCheckedGetTSD();
   if (t) {
      _eglDestroyThreadInfo(t);
      _eglSetTSD(NULL);
   }
}


/**
 * Return true if the calling thread's thread info is dummy.
 * A dummy thread info is shared by all threads and should not be modified.
 * Functions like eglBindAPI or eglMakeCurrent should check for dummy-ness
 * before updating the thread info.
 */
EGLBoolean
_eglIsCurrentThreadDummy(void)
{
   _EGLThreadInfo *t = _eglCheckedGetTSD();
   return (!t || t == &dummy_thread);
}


/**
 * Return the currently bound context of the given API, or NULL.
 */
PUBLIC _EGLContext *
_eglGetAPIContext(EGLenum api)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   return t->CurrentContexts[_eglConvertApiToIndex(api)];
}


/**
 * Return the currently bound context of the current API, or NULL.
 */
_EGLContext *
_eglGetCurrentContext(void)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   return t->CurrentContexts[t->CurrentAPIIndex];
}


/**
 * Record EGL error code and return EGL_FALSE.
 */
EGLBoolean
_eglError(EGLint errCode, const char *msg)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();

   if (t == &dummy_thread)
      return EGL_FALSE;

   t->LastError = errCode;

   if (errCode != EGL_SUCCESS) {
      const char *s;

      switch (errCode) {
      case EGL_BAD_ACCESS:
         s = "EGL_BAD_ACCESS";
         break;
      case EGL_BAD_ALLOC:
         s = "EGL_BAD_ALLOC";
         break;
      case EGL_BAD_ATTRIBUTE:
         s = "EGL_BAD_ATTRIBUTE";
         break;
      case EGL_BAD_CONFIG:
         s = "EGL_BAD_CONFIG";
         break;
      case EGL_BAD_CONTEXT:
         s = "EGL_BAD_CONTEXT";
         break;
      case EGL_BAD_CURRENT_SURFACE:
         s = "EGL_BAD_CURRENT_SURFACE";
         break;
      case EGL_BAD_DISPLAY:
         s = "EGL_BAD_DISPLAY";
         break;
      case EGL_BAD_MATCH:
         s = "EGL_BAD_MATCH";
         break;
      case EGL_BAD_NATIVE_PIXMAP:
         s = "EGL_BAD_NATIVE_PIXMAP";
         break;
      case EGL_BAD_NATIVE_WINDOW:
         s = "EGL_BAD_NATIVE_WINDOW";
         break;
      case EGL_BAD_PARAMETER:
         s = "EGL_BAD_PARAMETER";
         break;
      case EGL_BAD_SURFACE:
         s = "EGL_BAD_SURFACE";
         break;
      case EGL_NOT_INITIALIZED:
         s = "EGL_NOT_INITIALIZED";
         break;
#ifdef EGL_MESA_screen_surface
      case EGL_BAD_SCREEN_MESA:
         s = "EGL_BAD_SCREEN_MESA";
         break;
      case EGL_BAD_MODE_MESA:
         s = "EGL_BAD_MODE_MESA";
         break;
#endif
      default:
         s = "other EGL error";
      }
      _eglLog(_EGL_DEBUG, "EGL user error 0x%x (%s) in %s\n", errCode, s, msg);
   }

   return EGL_FALSE;
}
