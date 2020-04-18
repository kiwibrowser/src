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


#ifndef EGLCONTEXT_INCLUDED
#define EGLCONTEXT_INCLUDED


#include "egltypedefs.h"
#include "egldisplay.h"


/**
 * "Base" class for device driver contexts.
 */
struct _egl_context
{
   /* A context is a display resource */
   _EGLResource Resource;

   /* The bound status of the context */
   _EGLThreadInfo *Binding;
   _EGLSurface *DrawSurface;
   _EGLSurface *ReadSurface;

   _EGLConfig *Config;

   EGLint ClientAPI; /**< EGL_OPENGL_ES_API, EGL_OPENGL_API, EGL_OPENVG_API */
   EGLint ClientMajorVersion;
   EGLint ClientMinorVersion;
   EGLint Flags;
   EGLint Profile;
   EGLint ResetNotificationStrategy;

   /* The real render buffer when a window surface is bound */
   EGLint WindowRenderBuffer;
};


PUBLIC EGLBoolean
_eglInitContext(_EGLContext *ctx, _EGLDisplay *dpy,
                _EGLConfig *config, const EGLint *attrib_list);


extern EGLBoolean
_eglQueryContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx, EGLint attribute, EGLint *value);


PUBLIC EGLBoolean
_eglBindContext(_EGLContext *ctx, _EGLSurface *draw, _EGLSurface *read,
                _EGLContext **old_ctx,
                _EGLSurface **old_draw, _EGLSurface **old_read);


/**
 * Increment reference count for the context.
 */
static INLINE _EGLContext *
_eglGetContext(_EGLContext *ctx)
{
   if (ctx)
      _eglGetResource(&ctx->Resource);
   return ctx;
}


/**
 * Decrement reference count for the context.
 */
static INLINE EGLBoolean
_eglPutContext(_EGLContext *ctx)
{
   return (ctx) ? _eglPutResource(&ctx->Resource) : EGL_FALSE;
}


/**
 * Link a context to its display and return the handle of the link.
 * The handle can be passed to client directly.
 */
static INLINE EGLContext
_eglLinkContext(_EGLContext *ctx)
{
   _eglLinkResource(&ctx->Resource, _EGL_RESOURCE_CONTEXT);
   return (EGLContext) ctx;
}


/**
 * Unlink a linked context from its display.
 * Accessing an unlinked context should generate EGL_BAD_CONTEXT error.
 */
static INLINE void
_eglUnlinkContext(_EGLContext *ctx)
{
   _eglUnlinkResource(&ctx->Resource, _EGL_RESOURCE_CONTEXT);
}


/**
 * Lookup a handle to find the linked context.
 * Return NULL if the handle has no corresponding linked context.
 */
static INLINE _EGLContext *
_eglLookupContext(EGLContext context, _EGLDisplay *dpy)
{
   _EGLContext *ctx = (_EGLContext *) context;
   if (!dpy || !_eglCheckResource((void *) ctx, _EGL_RESOURCE_CONTEXT, dpy))
      ctx = NULL;
   return ctx;
}


/**
 * Return the handle of a linked context, or EGL_NO_CONTEXT.
 */
static INLINE EGLContext
_eglGetContextHandle(_EGLContext *ctx)
{
   _EGLResource *res = (_EGLResource *) ctx;
   return (res && _eglIsResourceLinked(res)) ?
      (EGLContext) ctx : EGL_NO_CONTEXT;
}


#endif /* EGLCONTEXT_INCLUDED */
