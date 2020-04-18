/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2009-2010 Chia-I Wu <olvaffe@gmail.com>
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


#ifndef EGLSURFACE_INCLUDED
#define EGLSURFACE_INCLUDED


#include "egltypedefs.h"
#include "egldisplay.h"


/**
 * "Base" class for device driver surfaces.
 */
struct _egl_surface
{
   /* A surface is a display resource */
   _EGLResource Resource;

   /* The context that is currently bound to the surface */
   _EGLContext *CurrentContext;

   _EGLConfig *Config;

   EGLint Type; /* one of EGL_WINDOW_BIT, EGL_PIXMAP_BIT or EGL_PBUFFER_BIT */

   /* attributes set by attribute list */
   EGLint Width, Height;
   EGLenum TextureFormat;
   EGLenum TextureTarget;
   EGLBoolean MipmapTexture;
   EGLBoolean LargestPbuffer;
   EGLenum RenderBuffer;
   EGLenum VGAlphaFormat;
   EGLenum VGColorspace;

   /* attributes set by eglSurfaceAttrib */
   EGLint MipmapLevel;
   EGLenum MultisampleResolve;
   EGLenum SwapBehavior;

   EGLint HorizontalResolution, VerticalResolution;
   EGLint AspectRatio;

   EGLint SwapInterval;

   /* True if the surface is bound to an OpenGL ES texture */
   EGLBoolean BoundToTexture;

   EGLBoolean PostSubBufferSupportedNV;
};


PUBLIC EGLBoolean
_eglInitSurface(_EGLSurface *surf, _EGLDisplay *dpy, EGLint type,
                _EGLConfig *config, const EGLint *attrib_list);


extern EGLBoolean
_eglQuerySurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, EGLint attribute, EGLint *value);


extern EGLBoolean
_eglSurfaceAttrib(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, EGLint attribute, EGLint value);


PUBLIC extern EGLBoolean
_eglBindTexImage(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, EGLint buffer);


extern EGLBoolean
_eglSwapInterval(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, EGLint interval);


/**
 * Increment reference count for the surface.
 */
static INLINE _EGLSurface *
_eglGetSurface(_EGLSurface *surf)
{
   if (surf)
      _eglGetResource(&surf->Resource);
   return surf;
}


/**
 * Decrement reference count for the surface.
 */
static INLINE EGLBoolean
_eglPutSurface(_EGLSurface *surf)
{
   return (surf) ? _eglPutResource(&surf->Resource) : EGL_FALSE;
}


/**
 * Link a surface to its display and return the handle of the link.
 * The handle can be passed to client directly.
 */
static INLINE EGLSurface
_eglLinkSurface(_EGLSurface *surf)
{
   _eglLinkResource(&surf->Resource, _EGL_RESOURCE_SURFACE);
   return (EGLSurface) surf;
}


/**
 * Unlink a linked surface from its display.
 * Accessing an unlinked surface should generate EGL_BAD_SURFACE error.
 */
static INLINE void
_eglUnlinkSurface(_EGLSurface *surf)
{
   _eglUnlinkResource(&surf->Resource, _EGL_RESOURCE_SURFACE);
}


/**
 * Lookup a handle to find the linked surface.
 * Return NULL if the handle has no corresponding linked surface.
 */
static INLINE _EGLSurface *
_eglLookupSurface(EGLSurface surface, _EGLDisplay *dpy)
{
   _EGLSurface *surf = (_EGLSurface *) surface;
   if (!dpy || !_eglCheckResource((void *) surf, _EGL_RESOURCE_SURFACE, dpy))
      surf = NULL;
   return surf;
}


/**
 * Return the handle of a linked surface, or EGL_NO_SURFACE.
 */
static INLINE EGLSurface
_eglGetSurfaceHandle(_EGLSurface *surf)
{
   _EGLResource *res = (_EGLResource *) surf;
   return (res && _eglIsResourceLinked(res)) ?
      (EGLSurface) surf : EGL_NO_SURFACE;
}


#endif /* EGLSURFACE_INCLUDED */
