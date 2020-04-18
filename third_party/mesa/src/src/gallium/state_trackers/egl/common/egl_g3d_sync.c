/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "util/u_memory.h"
#include "util/u_atomic.h"
#include "os/os_thread.h"
#include "eglsync.h"
#include "eglcurrent.h"

#include "egl_g3d.h"
#include "egl_g3d_sync.h"

/**
 * Wait for the conditional variable.
 */
static EGLint
egl_g3d_wait_sync_condvar(struct egl_g3d_sync *gsync, EGLTimeKHR timeout)
{
   _EGLDisplay *dpy = gsync->base.Resource.Display;

   pipe_mutex_lock(gsync->mutex);

   /* unlock display lock just before waiting */
   _eglUnlockMutex(&dpy->Mutex);

   /* No timed wait.  Always treat timeout as EGL_FOREVER_KHR */
   pipe_condvar_wait(gsync->condvar, gsync->mutex);

   _eglLockMutex(&dpy->Mutex);

   pipe_mutex_unlock(gsync->mutex);

   return EGL_CONDITION_SATISFIED_KHR;
}

/**
 * Signal the conditional variable.
 */
static void
egl_g3d_signal_sync_condvar(struct egl_g3d_sync *gsync)
{
   pipe_mutex_lock(gsync->mutex);
   pipe_condvar_broadcast(gsync->condvar);
   pipe_mutex_unlock(gsync->mutex);
}

/**
 * Insert a fence command to the command stream of the current context.
 */
static EGLint
egl_g3d_insert_fence_sync(struct egl_g3d_sync *gsync)
{
   _EGLContext *ctx = _eglGetCurrentContext();
   struct egl_g3d_context *gctx = egl_g3d_context(ctx);

   /* already checked in egl_g3d_create_sync */
   assert(gctx);

   /* insert the fence command */
   gctx->stctxi->flush(gctx->stctxi, 0x0, &gsync->fence);
   if (!gsync->fence)
      gsync->base.SyncStatus = EGL_SIGNALED_KHR;

   return EGL_SUCCESS;
}

/**
 * Wait for the fence sync to be signaled.
 */
static EGLint
egl_g3d_wait_fence_sync(struct egl_g3d_sync *gsync, EGLTimeKHR timeout)
{
   EGLint ret;

   if (gsync->fence) {
      _EGLDisplay *dpy = gsync->base.Resource.Display;
      struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
      struct pipe_screen *screen = gdpy->native->screen;
      struct pipe_fence_handle *fence = gsync->fence;

      gsync->fence = NULL;

      _eglUnlockMutex(&dpy->Mutex);
      /* no timed finish? */
      screen->fence_finish(screen, fence, PIPE_TIMEOUT_INFINITE);
      ret = EGL_CONDITION_SATISFIED_KHR;
      _eglLockMutex(&dpy->Mutex);

      gsync->base.SyncStatus = EGL_SIGNALED_KHR;

      screen->fence_reference(screen, &fence, NULL);
      egl_g3d_signal_sync_condvar(gsync);
   }
   else {
      ret = egl_g3d_wait_sync_condvar(gsync, timeout);
   }

   return ret;
}

static INLINE void
egl_g3d_ref_sync(struct egl_g3d_sync *gsync)
{
   _eglGetSync(&gsync->base);
}

static INLINE void
egl_g3d_unref_sync(struct egl_g3d_sync *gsync)
{
   if (_eglPutSync(&gsync->base)) {
      pipe_condvar_destroy(gsync->condvar);
      pipe_mutex_destroy(gsync->mutex);

      if (gsync->fence) {
         struct egl_g3d_display *gdpy =
            egl_g3d_display(gsync->base.Resource.Display);
         struct pipe_screen *screen = gdpy->native->screen;

         screen->fence_reference(screen, &gsync->fence, NULL);
      }

      FREE(gsync);
   }
}

_EGLSync *
egl_g3d_create_sync(_EGLDriver *drv, _EGLDisplay *dpy,
                    EGLenum type, const EGLint *attrib_list)
{
   _EGLContext *ctx = _eglGetCurrentContext();
   struct egl_g3d_sync *gsync;
   EGLint err;

   if (!ctx || ctx->Resource.Display != dpy) {
      _eglError(EGL_BAD_MATCH, "eglCreateSyncKHR");
      return NULL;
   }

   gsync = CALLOC_STRUCT(egl_g3d_sync);
   if (!gsync) {
      _eglError(EGL_BAD_ALLOC, "eglCreateSyncKHR");
      return NULL;
   }

   if (!_eglInitSync(&gsync->base, dpy, type, attrib_list)) {
      FREE(gsync);
      return NULL;
   }

   switch (type) {
   case EGL_SYNC_REUSABLE_KHR:
      err = EGL_SUCCESS;
      break;
   case EGL_SYNC_FENCE_KHR:
      err = egl_g3d_insert_fence_sync(gsync);
      break;
   default:
      err = EGL_BAD_ATTRIBUTE;
      break;
   }

   if (err != EGL_SUCCESS) {
      _eglError(err, "eglCreateSyncKHR");
      FREE(gsync);
      return NULL;
   }

   pipe_mutex_init(gsync->mutex);
   pipe_condvar_init(gsync->condvar);

   return &gsync->base;
}

EGLBoolean
egl_g3d_destroy_sync(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync)
{
   struct egl_g3d_sync *gsync = egl_g3d_sync(sync);

   switch (gsync->base.Type) {
   case EGL_SYNC_REUSABLE_KHR:
      /* signal the waiters */
      if (gsync->base.SyncStatus != EGL_SIGNALED_KHR) {
         gsync->base.SyncStatus = EGL_SIGNALED_KHR;
         egl_g3d_signal_sync_condvar(gsync);
      }
      break;
   default:
      break;
   }

   egl_g3d_unref_sync(gsync);

   return EGL_TRUE;
}

EGLint
egl_g3d_client_wait_sync(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync,
                         EGLint flags, EGLTimeKHR timeout)
{
   struct egl_g3d_sync *gsync = egl_g3d_sync(sync);
   EGLint ret = EGL_CONDITION_SATISFIED_KHR;

   if (gsync->base.SyncStatus != EGL_SIGNALED_KHR) {
      /* flush if there is a current context */
      if (flags & EGL_SYNC_FLUSH_COMMANDS_BIT_KHR) {
         _EGLContext *ctx = _eglGetCurrentContext();
         struct egl_g3d_context *gctx = egl_g3d_context(ctx);

         if (gctx)
            gctx->stctxi->flush(gctx->stctxi, ST_FLUSH_FRONT, NULL);
      }

      if (timeout) {
         /* reference the sync object in case it is destroyed while waiting */
         egl_g3d_ref_sync(gsync);

         switch (gsync->base.Type) {
         case EGL_SYNC_REUSABLE_KHR:
            ret = egl_g3d_wait_sync_condvar(gsync, timeout);
            break;
         case EGL_SYNC_FENCE_KHR:
            ret = egl_g3d_wait_fence_sync(gsync, timeout);
         default:
            break;
         }

         egl_g3d_unref_sync(gsync);
      }
      else {
         ret = EGL_TIMEOUT_EXPIRED_KHR;
      }
   }

   return ret;
}

EGLBoolean
egl_g3d_signal_sync(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync,
                    EGLenum mode)
{
   struct egl_g3d_sync *gsync = egl_g3d_sync(sync);

   /* only for reusable sync */
   if (sync->Type != EGL_SYNC_REUSABLE_KHR)
      return _eglError(EGL_BAD_MATCH, "eglSignalSyncKHR");

   if (gsync->base.SyncStatus != mode) {
      gsync->base.SyncStatus = mode;
      if (mode == EGL_SIGNALED_KHR)
         egl_g3d_signal_sync_condvar(gsync);
   }

   return EGL_TRUE;
}
