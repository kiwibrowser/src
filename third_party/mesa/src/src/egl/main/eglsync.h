/**************************************************************************
 *
 * Copyright 2010 LunarG, Inc.
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


#ifndef EGLSYNC_INCLUDED
#define EGLSYNC_INCLUDED


#include "egltypedefs.h"
#include "egldisplay.h"


/**
 * "Base" class for device driver syncs.
 */
struct _egl_sync
{
   /* A sync is a display resource */
   _EGLResource Resource;

   EGLenum Type;
   EGLenum SyncStatus;
   EGLenum SyncCondition;
};


PUBLIC EGLBoolean
_eglInitSync(_EGLSync *sync, _EGLDisplay *dpy, EGLenum type,
             const EGLint *attrib_list);


extern EGLBoolean
_eglGetSyncAttribKHR(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync,
                     EGLint attribute, EGLint *value);


/**
 * Increment reference count for the sync.
 */
static INLINE _EGLSync *
_eglGetSync(_EGLSync *sync)
{
   if (sync)
      _eglGetResource(&sync->Resource);
   return sync;
}


/**
 * Decrement reference count for the sync.
 */
static INLINE EGLBoolean
_eglPutSync(_EGLSync *sync)
{
   return (sync) ? _eglPutResource(&sync->Resource) : EGL_FALSE;
}


/**
 * Link a sync to its display and return the handle of the link.
 * The handle can be passed to client directly.
 */
static INLINE EGLSyncKHR
_eglLinkSync(_EGLSync *sync)
{
   _eglLinkResource(&sync->Resource, _EGL_RESOURCE_SYNC);
   return (EGLSyncKHR) sync;
}


/**
 * Unlink a linked sync from its display.
 */
static INLINE void
_eglUnlinkSync(_EGLSync *sync)
{
   _eglUnlinkResource(&sync->Resource, _EGL_RESOURCE_SYNC);
}


/**
 * Lookup a handle to find the linked sync.
 * Return NULL if the handle has no corresponding linked sync.
 */
static INLINE _EGLSync *
_eglLookupSync(EGLSyncKHR handle, _EGLDisplay *dpy)
{
   _EGLSync *sync = (_EGLSync *) handle;
   if (!dpy || !_eglCheckResource((void *) sync, _EGL_RESOURCE_SYNC, dpy))
      sync = NULL;
   return sync;
}


/**
 * Return the handle of a linked sync, or EGL_NO_SYNC_KHR.
 */
static INLINE EGLSyncKHR
_eglGetSyncHandle(_EGLSync *sync)
{
   _EGLResource *res = (_EGLResource *) sync;
   return (res && _eglIsResourceLinked(res)) ?
      (EGLSyncKHR) sync : EGL_NO_SYNC_KHR;
}


#endif /* EGLSYNC_INCLUDED */
