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


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "eglcurrent.h"
#include "eglsurface.h"
#include "egllog.h"


/**
 * Return the API bit (one of EGL_xxx_BIT) of the context.
 */
static EGLint
_eglGetContextAPIBit(_EGLContext *ctx)
{
   EGLint bit = 0;

   switch (ctx->ClientAPI) {
   case EGL_OPENGL_ES_API:
      switch (ctx->ClientMajorVersion) {
      case 1:
         bit = EGL_OPENGL_ES_BIT;
         break;
      case 2:
      case 3:
         bit = EGL_OPENGL_ES2_BIT;
         break;
      default:
         break;
      }
      break;
   case EGL_OPENVG_API:
      bit = EGL_OPENVG_BIT;
      break;
   case EGL_OPENGL_API:
      bit = EGL_OPENGL_BIT;
      break;
   default:
      break;
   }

   return bit;
}


/**
 * Parse the list of context attributes and return the proper error code.
 */
static EGLint
_eglParseContextAttribList(_EGLContext *ctx, _EGLDisplay *dpy,
                           const EGLint *attrib_list)
{
   EGLenum api = ctx->ClientAPI;
   EGLint i, err = EGL_SUCCESS;

   if (!attrib_list)
      return EGL_SUCCESS;

   if (api == EGL_OPENVG_API && attrib_list[0] != EGL_NONE) {
      _eglLog(_EGL_DEBUG, "bad context attribute 0x%04x", attrib_list[0]);
      return EGL_BAD_ATTRIBUTE;
   }

   for (i = 0; attrib_list[i] != EGL_NONE; i++) {
      EGLint attr = attrib_list[i++];
      EGLint val = attrib_list[i];

      switch (attr) {
      case EGL_CONTEXT_CLIENT_VERSION:
         ctx->ClientMajorVersion = val;
         break;

      case EGL_CONTEXT_MINOR_VERSION_KHR:
         if (!dpy->Extensions.KHR_create_context) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }

         ctx->ClientMinorVersion = val;
         break;

      case EGL_CONTEXT_FLAGS_KHR:
         if (!dpy->Extensions.KHR_create_context) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }

         /* The EGL_KHR_create_context spec says:
          *
          *     "Flags are only defined for OpenGL context creation, and
          *     specifying a flags value other than zero for other types of
          *     contexts, including OpenGL ES contexts, will generate an
          *     error."
          */
         if (api != EGL_OPENGL_API && val != 0) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }

         ctx->Flags = val;
         break;

      case EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR:
         if (!dpy->Extensions.KHR_create_context) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }

         /* The EGL_KHR_create_context spec says:
          *
          *     "[EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR] is only meaningful for
          *     OpenGL contexts, and specifying it for other types of
          *     contexts, including OpenGL ES contexts, will generate an
          *     error."
          */
         if (api != EGL_OPENGL_API) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }

         ctx->Profile = val;
         break;

      case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR:
         /* The EGL_KHR_create_context spec says:
          *
          *     "[EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR] is only
          *     meaningful for OpenGL contexts, and specifying it for other
          *     types of contexts, including OpenGL ES contexts, will generate
          *     an error."
          */
           if (!dpy->Extensions.KHR_create_context
               || api != EGL_OPENGL_API) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }

         ctx->ResetNotificationStrategy = val;
         break;

      case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT:
         /* The EGL_EXT_create_context_robustness spec says:
          *
          *     "[EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT] is only
          *     meaningful for OpenGL ES contexts, and specifying it for other
          *     types of contexts will generate an EGL_BAD_ATTRIBUTE error."
          */
         if (!dpy->Extensions.EXT_create_context_robustness
             || api != EGL_OPENGL_ES_API) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }

         ctx->ResetNotificationStrategy = val;
         break;

      case EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT:
         if (!dpy->Extensions.EXT_create_context_robustness) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }

         ctx->Flags = EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR;
         break;

      default:
         err = EGL_BAD_ATTRIBUTE;
         break;
      }

      if (err != EGL_SUCCESS) {
         _eglLog(_EGL_DEBUG, "bad context attribute 0x%04x", attr);
         break;
      }
   }

   if (api == EGL_OPENGL_API) {
      /* The EGL_KHR_create_context spec says:
       *
       *     "If the requested OpenGL version is less than 3.2,
       *     EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR is ignored and the
       *     functionality of the context is determined solely by the
       *     requested version."
       *
       * Since the value is ignored, only validate the setting if the version
       * is >= 3.2.
       */
      if (ctx->ClientMajorVersion >= 4
          || (ctx->ClientMajorVersion == 3 && ctx->ClientMinorVersion >= 2)) {
         switch (ctx->Profile) {
         case EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR:
         case EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR:
            break;

         default:
            /* The EGL_KHR_create_context spec says:
             *
             *     "* If an OpenGL context is requested, the requested version
             *        is greater than 3.2, and the value for attribute
             *        EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR has no bits set; has
             *        any bits set other than EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR
             *        and EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR; has
             *        more than one of these bits set; or if the implementation does
             *        not support the requested profile, then an EGL_BAD_MATCH error
             *        is generated."
             */
            err = EGL_BAD_MATCH;
            break;
         }
      }

      /* The EGL_KHR_create_context spec says:
       *
       *     "* If an OpenGL context is requested and the values for
       *        attributes EGL_CONTEXT_MAJOR_VERSION_KHR and
       *        EGL_CONTEXT_MINOR_VERSION_KHR, when considered together with
       *        the value for attribute
       *        EGL_CONTEXT_FORWARD_COMPATIBLE_BIT_KHR, specify an OpenGL
       *        version and feature set that are not defined, than an
       *        EGL_BAD_MATCH error is generated.
       *
       *        ... Thus, examples of invalid combinations of attributes
       *        include:
       *
       *          - Major version < 1 or > 4
       *          - Major version == 1 and minor version < 0 or > 5
       *          - Major version == 2 and minor version < 0 or > 1
       *          - Major version == 3 and minor version < 0 or > 2
       *          - Major version == 4 and minor version < 0 or > 2
       *          - Forward-compatible flag set and major version < 3"
       */
      if (ctx->ClientMajorVersion < 1 || ctx->ClientMinorVersion < 0)
         err = EGL_BAD_MATCH;

      switch (ctx->ClientMajorVersion) {
      case 1:
         if (ctx->ClientMinorVersion > 5
             || (ctx->Flags & EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR) != 0)
            err = EGL_BAD_MATCH;
         break;

      case 2:
         if (ctx->ClientMinorVersion > 1
             || (ctx->Flags & EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR) != 0)
            err = EGL_BAD_MATCH;
         break;

      case 3:
         /* Note: The text above is incorrect.  There *is* an OpenGL 3.3!
          */
         if (ctx->ClientMinorVersion > 3)
            err = EGL_BAD_MATCH;
         break;

      case 4:
      default:
         /* Don't put additional version checks here.  We don't know that
          * there won't be versions > 4.2.
          */
         break;
      }
   } else if (api == EGL_OPENGL_ES_API) {
      /* The EGL_KHR_create_context spec says:
       *
       *     "* If an OpenGL ES context is requested and the values for
       *        attributes EGL_CONTEXT_MAJOR_VERSION_KHR and
       *        EGL_CONTEXT_MINOR_VERSION_KHR specify an OpenGL ES version that
       *        is not defined, than an EGL_BAD_MATCH error is generated.
       *
       *        ... Examples of invalid combinations of attributes include:
       *
       *          - Major version < 1 or > 2
       *          - Major version == 1 and minor version < 0 or > 1
       *          - Major version == 2 and minor version != 0
       */
      if (ctx->ClientMajorVersion < 1 || ctx->ClientMinorVersion < 0)
         err = EGL_BAD_MATCH;

      switch (ctx->ClientMajorVersion) {
      case 1:
         if (ctx->ClientMinorVersion > 1)
            err = EGL_BAD_MATCH;
         break;

      case 2:
         if (ctx->ClientMinorVersion > 0)
            err = EGL_BAD_MATCH;
         break;

      case 3:
      default:
         /* Don't put additional version checks here.  We don't know that
          * there won't be versions > 3.0.
          */
         break;
      }
   }

   switch (ctx->ResetNotificationStrategy) {
   case EGL_NO_RESET_NOTIFICATION_KHR:
   case EGL_LOSE_CONTEXT_ON_RESET_KHR:
      break;

   default:
      err = EGL_BAD_ATTRIBUTE;
      break;
   }

   if ((ctx->Flags & ~(EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR
                      | EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR
                      | EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR)) != 0) {
      err = EGL_BAD_ATTRIBUTE;
   }

   return err;
}


/**
 * Initialize the given _EGLContext object to defaults and/or the values
 * in the attrib_list.
 */
EGLBoolean
_eglInitContext(_EGLContext *ctx, _EGLDisplay *dpy, _EGLConfig *conf,
                const EGLint *attrib_list)
{
   const EGLenum api = eglQueryAPI();
   EGLint err;

   if (api == EGL_NONE) {
      _eglError(EGL_BAD_MATCH, "eglCreateContext(no client API)");
      return EGL_FALSE;
   }

   _eglInitResource(&ctx->Resource, sizeof(*ctx), dpy);
   ctx->ClientAPI = api;
   ctx->Config = conf;
   ctx->WindowRenderBuffer = EGL_NONE;
   ctx->Profile = EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR;

   ctx->ClientMajorVersion = 1; /* the default, per EGL spec */
   ctx->ClientMinorVersion = 0;
   ctx->Flags = 0;
   ctx->Profile = EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR;
   ctx->ResetNotificationStrategy = EGL_NO_RESET_NOTIFICATION_KHR;

   err = _eglParseContextAttribList(ctx, dpy, attrib_list);
   if (err == EGL_SUCCESS && ctx->Config) {
      EGLint api_bit;

      api_bit = _eglGetContextAPIBit(ctx);
      if (!(ctx->Config->RenderableType & api_bit)) {
         _eglLog(_EGL_DEBUG, "context api is 0x%x while config supports 0x%x",
               api_bit, ctx->Config->RenderableType);
         err = EGL_BAD_CONFIG;
      }
   }
   if (err != EGL_SUCCESS)
      return _eglError(err, "eglCreateContext");

   return EGL_TRUE;
}


static EGLint
_eglQueryContextRenderBuffer(_EGLContext *ctx)
{
   _EGLSurface *surf = ctx->DrawSurface;
   EGLint rb;

   if (!surf)
      return EGL_NONE;
   if (surf->Type == EGL_WINDOW_BIT && ctx->WindowRenderBuffer != EGL_NONE)
      rb = ctx->WindowRenderBuffer;
   else
      rb = surf->RenderBuffer;
   return rb;
}


EGLBoolean
_eglQueryContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *c,
                 EGLint attribute, EGLint *value)
{
   (void) drv;
   (void) dpy;

   if (!value)
      return _eglError(EGL_BAD_PARAMETER, "eglQueryContext");

   switch (attribute) {
   case EGL_CONFIG_ID:
      if (!c->Config)
         return _eglError(EGL_BAD_ATTRIBUTE, "eglQueryContext");
      *value = c->Config->ConfigID;
      break;
   case EGL_CONTEXT_CLIENT_VERSION:
      *value = c->ClientMajorVersion;
      break;
   case EGL_CONTEXT_CLIENT_TYPE:
      *value = c->ClientAPI;
      break;
   case EGL_RENDER_BUFFER:
      *value = _eglQueryContextRenderBuffer(c);
      break;
   default:
      return _eglError(EGL_BAD_ATTRIBUTE, "eglQueryContext");
   }

   return EGL_TRUE;
}


/**
 * Bind the context to the thread and return the previous context.
 *
 * Note that the context may be NULL.
 */
static _EGLContext *
_eglBindContextToThread(_EGLContext *ctx, _EGLThreadInfo *t)
{
   EGLint apiIndex;
   _EGLContext *oldCtx;

   apiIndex = (ctx) ?
      _eglConvertApiToIndex(ctx->ClientAPI) : t->CurrentAPIIndex;

   oldCtx = t->CurrentContexts[apiIndex];
   if (ctx != oldCtx) {
      if (oldCtx)
         oldCtx->Binding = NULL;
      if (ctx)
         ctx->Binding = t;

      t->CurrentContexts[apiIndex] = ctx;
   }

   return oldCtx;
}


/**
 * Return true if the given context and surfaces can be made current.
 */
static EGLBoolean
_eglCheckMakeCurrent(_EGLContext *ctx, _EGLSurface *draw, _EGLSurface *read)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   _EGLDisplay *dpy;
   EGLint conflict_api;

   if (_eglIsCurrentThreadDummy())
      return _eglError(EGL_BAD_ALLOC, "eglMakeCurrent");

   /* this is easy */
   if (!ctx) {
      if (draw || read)
         return _eglError(EGL_BAD_MATCH, "eglMakeCurrent");
      return EGL_TRUE;
   }

   dpy = ctx->Resource.Display;
   if (!dpy->Extensions.KHR_surfaceless_context
       && (draw == NULL || read == NULL))
      return _eglError(EGL_BAD_MATCH, "eglMakeCurrent");

   /*
    * The spec says
    *
    * "If ctx is current to some other thread, or if either draw or read are
    * bound to contexts in another thread, an EGL_BAD_ACCESS error is
    * generated."
    *
    * and
    *
    * "at most one context may be bound to a particular surface at a given
    * time"
    */
   if (ctx->Binding && ctx->Binding != t)
      return _eglError(EGL_BAD_ACCESS, "eglMakeCurrent");
   if (draw && draw->CurrentContext && draw->CurrentContext != ctx) {
      if (draw->CurrentContext->Binding != t ||
          draw->CurrentContext->ClientAPI != ctx->ClientAPI)
         return _eglError(EGL_BAD_ACCESS, "eglMakeCurrent");
   }
   if (read && read->CurrentContext && read->CurrentContext != ctx) {
      if (read->CurrentContext->Binding != t ||
          read->CurrentContext->ClientAPI != ctx->ClientAPI)
         return _eglError(EGL_BAD_ACCESS, "eglMakeCurrent");
   }

   /* simply require the configs to be equal */
   if ((draw && draw->Config != ctx->Config) ||
       (read && read->Config != ctx->Config))
      return _eglError(EGL_BAD_MATCH, "eglMakeCurrent");

   switch (ctx->ClientAPI) {
   /* OpenGL and OpenGL ES are conflicting */
   case EGL_OPENGL_ES_API:
      conflict_api = EGL_OPENGL_API;
      break;
   case EGL_OPENGL_API:
      conflict_api = EGL_OPENGL_ES_API;
      break;
   default:
      conflict_api = -1;
      break;
   }

   if (conflict_api >= 0 && _eglGetAPIContext(conflict_api))
      return _eglError(EGL_BAD_ACCESS, "eglMakeCurrent");

   return EGL_TRUE;
}


/**
 * Bind the context to the current thread and given surfaces.  Return the
 * previous bound context and surfaces.  The caller should unreference the
 * returned context and surfaces.
 *
 * Making a second call with the resources returned by the first call
 * unsurprisingly undoes the first call, except for the resouce reference
 * counts.
 */
EGLBoolean
_eglBindContext(_EGLContext *ctx, _EGLSurface *draw, _EGLSurface *read,
                _EGLContext **old_ctx,
                _EGLSurface **old_draw, _EGLSurface **old_read)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   _EGLContext *prev_ctx;
   _EGLSurface *prev_draw, *prev_read;

   if (!_eglCheckMakeCurrent(ctx, draw, read))
      return EGL_FALSE;

   /* increment refcounts before binding */
   _eglGetContext(ctx);
   _eglGetSurface(draw);
   _eglGetSurface(read);

   /* bind the new context */
   prev_ctx = _eglBindContextToThread(ctx, t);

   /* break previous bindings */
   if (prev_ctx) {
      prev_draw = prev_ctx->DrawSurface;
      prev_read = prev_ctx->ReadSurface;

      if (prev_draw)
         prev_draw->CurrentContext = NULL;
      if (prev_read)
         prev_read->CurrentContext = NULL;

      prev_ctx->DrawSurface = NULL;
      prev_ctx->ReadSurface = NULL;
   }
   else {
      prev_draw = prev_read = NULL;
   }

   /* establish new bindings */
   if (ctx) {
      if (draw)
         draw->CurrentContext = ctx;
      if (read)
         read->CurrentContext = ctx;

      ctx->DrawSurface = draw;
      ctx->ReadSurface = read;
   }

   assert(old_ctx && old_draw && old_read);
   *old_ctx = prev_ctx;
   *old_draw = prev_draw;
   *old_read = prev_read;

   return EGL_TRUE;
}
