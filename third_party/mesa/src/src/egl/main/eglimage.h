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


#ifndef EGLIMAGE_INCLUDED
#define EGLIMAGE_INCLUDED


#include "egltypedefs.h"
#include "egldisplay.h"


struct _egl_image_attribs
{
   /* EGL_KHR_image_base */
   EGLBoolean ImagePreserved;

   /* EGL_KHR_gl_image */
   EGLint GLTextureLevel;
   EGLint GLTextureZOffset;

   /* EGL_MESA_drm_image */
   EGLint Width;
   EGLint Height;
   EGLint DRMBufferFormatMESA;
   EGLint DRMBufferUseMESA;
   EGLint DRMBufferStrideMESA;

   /* EGL_WL_bind_wayland_display */
   EGLint PlaneWL;
};

/**
 * "Base" class for device driver images.
 */
struct _egl_image
{
   /* An image is a display resource */
   _EGLResource Resource;
};


PUBLIC EGLint
_eglParseImageAttribList(_EGLImageAttribs *attrs, _EGLDisplay *dpy,
                         const EGLint *attrib_list);


PUBLIC EGLBoolean
_eglInitImage(_EGLImage *img, _EGLDisplay *dpy);


/**
 * Increment reference count for the image.
 */
static INLINE _EGLImage *
_eglGetImage(_EGLImage *img)
{
   if (img)
      _eglGetResource(&img->Resource);
   return img;
}


/**
 * Decrement reference count for the image.
 */
static INLINE EGLBoolean
_eglPutImage(_EGLImage *img)
{
   return (img) ? _eglPutResource(&img->Resource) : EGL_FALSE;
}


/**
 * Link an image to its display and return the handle of the link.
 * The handle can be passed to client directly.
 */
static INLINE EGLImageKHR
_eglLinkImage(_EGLImage *img)
{
   _eglLinkResource(&img->Resource, _EGL_RESOURCE_IMAGE);
   return (EGLImageKHR) img;
}


/**
 * Unlink a linked image from its display.
 * Accessing an unlinked image should generate EGL_BAD_PARAMETER error.
 */
static INLINE void
_eglUnlinkImage(_EGLImage *img)
{
   _eglUnlinkResource(&img->Resource, _EGL_RESOURCE_IMAGE);
}


/**
 * Lookup a handle to find the linked image.
 * Return NULL if the handle has no corresponding linked image.
 */
static INLINE _EGLImage *
_eglLookupImage(EGLImageKHR image, _EGLDisplay *dpy)
{
   _EGLImage *img = (_EGLImage *) image;
   if (!dpy || !_eglCheckResource((void *) img, _EGL_RESOURCE_IMAGE, dpy))
      img = NULL;
   return img;
}


/**
 * Return the handle of a linked image, or EGL_NO_IMAGE_KHR.
 */
static INLINE EGLImageKHR
_eglGetImageHandle(_EGLImage *img)
{
   _EGLResource *res = (_EGLResource *) img;
   return (res && _eglIsResourceLinked(res)) ?
      (EGLImageKHR) img : EGL_NO_IMAGE_KHR;
}


#endif /* EGLIMAGE_INCLUDED */
