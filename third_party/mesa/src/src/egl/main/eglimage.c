/**************************************************************************
 *
 * Copyright 2009-2010 Chia-I Wu <olvaffe@gmail.com>
 * Copyright 2010-2011 LunarG, Inc.
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


#include <assert.h>
#include <string.h>

#include "eglimage.h"
#include "egllog.h"


/**
 * Parse the list of image attributes and return the proper error code.
 */
EGLint
_eglParseImageAttribList(_EGLImageAttribs *attrs, _EGLDisplay *dpy,
                         const EGLint *attrib_list)
{
   EGLint i, err = EGL_SUCCESS;

   (void) dpy;

   memset(attrs, 0, sizeof(*attrs));
   attrs->ImagePreserved = EGL_FALSE;
   attrs->GLTextureLevel = 0;
   attrs->GLTextureZOffset = 0;

   if (!attrib_list)
      return err;

   for (i = 0; attrib_list[i] != EGL_NONE; i++) {
      EGLint attr = attrib_list[i++];
      EGLint val = attrib_list[i];

      switch (attr) {
      /* EGL_KHR_image_base */
      case EGL_IMAGE_PRESERVED_KHR:
         attrs->ImagePreserved = val;
         break;

      /* EGL_KHR_gl_image */
      case EGL_GL_TEXTURE_LEVEL_KHR:
         attrs->GLTextureLevel = val;
         break;
      case EGL_GL_TEXTURE_ZOFFSET_KHR:
         attrs->GLTextureZOffset = val;
         break;

      /* EGL_MESA_drm_image */
      case EGL_WIDTH:
         attrs->Width = val;
         break;
      case EGL_HEIGHT:
         attrs->Height = val;
         break;
      case EGL_DRM_BUFFER_FORMAT_MESA:
         attrs->DRMBufferFormatMESA = val;
         break;
      case EGL_DRM_BUFFER_USE_MESA:
         attrs->DRMBufferUseMESA = val;
         break;
      case EGL_DRM_BUFFER_STRIDE_MESA:
         attrs->DRMBufferStrideMESA = val;
         break;

      /* EGL_WL_bind_wayland_display */
      case EGL_WAYLAND_PLANE_WL:
         attrs->PlaneWL = val;
         break;

      default:
         /* unknown attrs are ignored */
         break;
      }

      if (err != EGL_SUCCESS) {
         _eglLog(_EGL_DEBUG, "bad image attribute 0x%04x", attr);
         break;
      }
   }

   return err;
}


EGLBoolean
_eglInitImage(_EGLImage *img, _EGLDisplay *dpy)
{
   _eglInitResource(&img->Resource, sizeof(*img), dpy);

   return EGL_TRUE;
}
