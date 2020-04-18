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


#include <string.h>

#include "eglsync.h"
#include "eglcurrent.h"
#include "egllog.h"


/**
 * Parse the list of sync attributes and return the proper error code.
 */
static EGLint
_eglParseSyncAttribList(_EGLSync *sync, const EGLint *attrib_list)
{
   EGLint i, err = EGL_SUCCESS;

   if (!attrib_list)
      return EGL_SUCCESS;

   for (i = 0; attrib_list[i] != EGL_NONE; i++) {
      EGLint attr = attrib_list[i++];
      EGLint val = attrib_list[i];

      switch (attr) {
      default:
         (void) val;
         err = EGL_BAD_ATTRIBUTE;
         break;
      }

      if (err != EGL_SUCCESS) {
         _eglLog(_EGL_DEBUG, "bad sync attribute 0x%04x", attr);
         break;
      }
   }

   return err;
}


EGLBoolean
_eglInitSync(_EGLSync *sync, _EGLDisplay *dpy, EGLenum type,
             const EGLint *attrib_list)
{
   EGLint err;

   if (!(type == EGL_SYNC_REUSABLE_KHR && dpy->Extensions.KHR_reusable_sync) &&
       !(type == EGL_SYNC_FENCE_KHR && dpy->Extensions.KHR_fence_sync))
      return _eglError(EGL_BAD_ATTRIBUTE, "eglCreateSyncKHR");

   _eglInitResource(&sync->Resource, sizeof(*sync), dpy);
   sync->Type = type;
   sync->SyncStatus = EGL_UNSIGNALED_KHR;
   sync->SyncCondition = EGL_SYNC_PRIOR_COMMANDS_COMPLETE_KHR;

   err = _eglParseSyncAttribList(sync, attrib_list);
   if (err != EGL_SUCCESS)
      return _eglError(err, "eglCreateSyncKHR");

   return EGL_TRUE;
}


EGLBoolean
_eglGetSyncAttribKHR(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync,
                     EGLint attribute, EGLint *value)
{
   if (!value)
      return _eglError(EGL_BAD_PARAMETER, "eglGetConfigs");

   switch (attribute) {
   case EGL_SYNC_TYPE_KHR:
      *value = sync->Type;
      break;
   case EGL_SYNC_STATUS_KHR:
      *value = sync->SyncStatus;
      break;
   case EGL_SYNC_CONDITION_KHR:
      if (sync->Type != EGL_SYNC_FENCE_KHR)
         return _eglError(EGL_BAD_ATTRIBUTE, "eglGetSyncAttribKHR");
      *value = sync->SyncCondition;
      break;
   default:
      return _eglError(EGL_BAD_ATTRIBUTE, "eglGetSyncAttribKHR");
      break;
   }

   return EGL_TRUE;
}
