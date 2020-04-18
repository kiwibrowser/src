/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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


/**
 * Small/misc EGL functions
 */


#include <assert.h>
#include <string.h>
#include "eglcurrent.h"
#include "eglmisc.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglstring.h"


/**
 * Copy the extension into the string and update the string pointer.
 */
static EGLint
_eglAppendExtension(char **str, const char *ext)
{
   char *s = *str;
   size_t len = strlen(ext);

   if (s) {
      memcpy(s, ext, len);
      s[len++] = ' ';
      s[len] = '\0';

      *str += len;
   }
   else {
      len++;
   }

   return (EGLint) len;
}


/**
 * Examine the individual extension enable/disable flags and recompute
 * the driver's Extensions string.
 */
static void
_eglUpdateExtensionsString(_EGLDisplay *dpy)
{
#define _EGL_CHECK_EXTENSION(ext)                                          \
   do {                                                                    \
      if (dpy->Extensions.ext) {                                           \
         _eglAppendExtension(&exts, "EGL_" #ext);                          \
         assert(exts <= dpy->ExtensionsString + _EGL_MAX_EXTENSIONS_LEN);  \
      }                                                                    \
   } while (0)

   char *exts = dpy->ExtensionsString;

   if (exts[0])
      return;

   _EGL_CHECK_EXTENSION(MESA_screen_surface);
   _EGL_CHECK_EXTENSION(MESA_copy_context);
   _EGL_CHECK_EXTENSION(MESA_drm_display);
   _EGL_CHECK_EXTENSION(MESA_drm_image);

   _EGL_CHECK_EXTENSION(WL_bind_wayland_display);

   _EGL_CHECK_EXTENSION(KHR_image_base);
   _EGL_CHECK_EXTENSION(KHR_image_pixmap);
   if (dpy->Extensions.KHR_image_base && dpy->Extensions.KHR_image_pixmap)
      _eglAppendExtension(&exts, "EGL_KHR_image");

   _EGL_CHECK_EXTENSION(KHR_vg_parent_image);
   _EGL_CHECK_EXTENSION(KHR_gl_texture_2D_image);
   _EGL_CHECK_EXTENSION(KHR_gl_texture_cubemap_image);
   _EGL_CHECK_EXTENSION(KHR_gl_texture_3D_image);
   _EGL_CHECK_EXTENSION(KHR_gl_renderbuffer_image);

   _EGL_CHECK_EXTENSION(KHR_reusable_sync);
   _EGL_CHECK_EXTENSION(KHR_fence_sync);

   _EGL_CHECK_EXTENSION(KHR_surfaceless_context);
   _EGL_CHECK_EXTENSION(KHR_create_context);

   _EGL_CHECK_EXTENSION(NOK_swap_region);
   _EGL_CHECK_EXTENSION(NOK_texture_from_pixmap);

   _EGL_CHECK_EXTENSION(ANDROID_image_native_buffer);

   _EGL_CHECK_EXTENSION(EXT_create_context_robustness);

   _EGL_CHECK_EXTENSION(NV_post_sub_buffer);
#undef _EGL_CHECK_EXTENSION
}


static void
_eglUpdateAPIsString(_EGLDisplay *dpy)
{
   char *apis = dpy->ClientAPIsString;

   if (apis[0] || !dpy->ClientAPIs)
      return;

   if (dpy->ClientAPIs & EGL_OPENGL_BIT)
      strcat(apis, "OpenGL ");

   if (dpy->ClientAPIs & EGL_OPENGL_ES_BIT)
      strcat(apis, "OpenGL_ES ");

   if (dpy->ClientAPIs & EGL_OPENGL_ES2_BIT)
      strcat(apis, "OpenGL_ES2 ");

   if (dpy->ClientAPIs & EGL_OPENVG_BIT)
      strcat(apis, "OpenVG ");

   assert(strlen(apis) < sizeof(dpy->ClientAPIsString));
}


const char *
_eglQueryString(_EGLDriver *drv, _EGLDisplay *dpy, EGLint name)
{
   (void) drv;

   switch (name) {
   case EGL_VENDOR:
      return _EGL_VENDOR_STRING;
   case EGL_VERSION:
      _eglsnprintf(dpy->VersionString, sizeof(dpy->VersionString),
              "%d.%d (%s)", dpy->VersionMajor, dpy->VersionMinor,
              dpy->Driver->Name);
      return dpy->VersionString;
   case EGL_EXTENSIONS:
      _eglUpdateExtensionsString(dpy);
      return dpy->ExtensionsString;
   case EGL_CLIENT_APIS:
      _eglUpdateAPIsString(dpy);
      return dpy->ClientAPIsString;
   default:
      _eglError(EGL_BAD_PARAMETER, "eglQueryString");
      return NULL;
   }
}
