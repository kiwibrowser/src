/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2009-2010 Chia-I Wu <olv@0xlab.org>
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
 */

#include "egldriver.h"
#include "eglcurrent.h"
#include "egllog.h"

#include "pipe/p_screen.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_box.h"

#include "egl_g3d.h"
#include "egl_g3d_api.h"
#include "egl_g3d_image.h"
#include "egl_g3d_sync.h"
#include "egl_g3d_st.h"
#include "native.h"

/**
 * Return the state tracker for the given context.
 */
static struct st_api *
egl_g3d_choose_st(_EGLDriver *drv, _EGLContext *ctx,
                  enum st_profile_type *profile)
{
   struct st_api *stapi;
   EGLint api = -1;

   *profile = ST_PROFILE_DEFAULT;

   switch (ctx->ClientAPI) {
   case EGL_OPENGL_ES_API:
      switch (ctx->ClientMajorVersion) {
      case 1:
         api = ST_API_OPENGL;
         *profile = ST_PROFILE_OPENGL_ES1;
         break;
      case 2:
         api = ST_API_OPENGL;
         *profile = ST_PROFILE_OPENGL_ES2;
         break;
      default:
         _eglLog(_EGL_WARNING, "unknown client major version %d",
               ctx->ClientMajorVersion);
         break;
      }
      break;
   case EGL_OPENVG_API:
      api = ST_API_OPENVG;
      break;
   case EGL_OPENGL_API:
      api = ST_API_OPENGL;
      break;
   default:
      _eglLog(_EGL_WARNING, "unknown client API 0x%04x", ctx->ClientAPI);
      break;
   }

   stapi = egl_g3d_get_st_api(drv, api);
   if (stapi && !(stapi->profile_mask & (1 << *profile)))
      stapi = NULL;

   return stapi;
}

struct egl_g3d_choose_config_data {
   _EGLConfig criteria;
   enum pipe_format format;
};

static int
egl_g3d_compare_config(const _EGLConfig *conf1, const _EGLConfig *conf2,
                       void *priv_data)
{
   struct egl_g3d_choose_config_data *data =
      (struct egl_g3d_choose_config_data *) priv_data;
   const _EGLConfig *criteria = &data->criteria;;

   /* EGL_NATIVE_VISUAL_TYPE ignored? */
   return _eglCompareConfigs(conf1, conf2, criteria, EGL_TRUE);
}

static EGLBoolean
egl_g3d_match_config(const _EGLConfig *conf, void *priv_data)
{
   struct egl_g3d_choose_config_data *data =
      (struct egl_g3d_choose_config_data *) priv_data;
   struct egl_g3d_config *gconf = egl_g3d_config(conf);

   if (data->format != PIPE_FORMAT_NONE &&
       data->format != gconf->native->color_format)
      return EGL_FALSE;

   return _eglMatchConfig(conf, &data->criteria);
}

static EGLBoolean
egl_g3d_choose_config(_EGLDriver *drv, _EGLDisplay *dpy, const EGLint *attribs,
                      EGLConfig *configs, EGLint size, EGLint *num_configs)
{
   struct egl_g3d_choose_config_data data;

   if (!_eglParseConfigAttribList(&data.criteria, dpy, attribs))
      return _eglError(EGL_BAD_ATTRIBUTE, "eglChooseConfig");

   data.format = PIPE_FORMAT_NONE;
   if (data.criteria.MatchNativePixmap != EGL_NONE &&
       data.criteria.MatchNativePixmap != EGL_DONT_CARE) {
      struct egl_g3d_display *gdpy = egl_g3d_display(dpy);

      if (!gdpy->native->get_pixmap_format(gdpy->native,
               (EGLNativePixmapType) data.criteria.MatchNativePixmap,
               &data.format))
         return _eglError(EGL_BAD_NATIVE_PIXMAP, "eglChooseConfig");
   }

   return _eglFilterConfigArray(dpy->Configs, configs, size, num_configs,
         egl_g3d_match_config, egl_g3d_compare_config, &data);
}

static _EGLContext *
egl_g3d_create_context(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                       _EGLContext *share, const EGLint *attribs)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_context *gshare = egl_g3d_context(share);
   struct egl_g3d_config *gconf = egl_g3d_config(conf);
   struct egl_g3d_context *gctx;
   struct st_context_attribs stattribs;
   enum st_context_error ctx_err = 0;

   gctx = CALLOC_STRUCT(egl_g3d_context);
   if (!gctx) {
      _eglError(EGL_BAD_ALLOC, "eglCreateContext");
      return NULL;
   }

   if (!_eglInitContext(&gctx->base, dpy, conf, attribs)) {
      FREE(gctx);
      return NULL;
   }

   memset(&stattribs, 0, sizeof(stattribs));
   if (gconf)
      stattribs.visual = gconf->stvis;

   gctx->stapi = egl_g3d_choose_st(drv, &gctx->base, &stattribs.profile);
   if (!gctx->stapi) {
      FREE(gctx);
      return NULL;
   }

   gctx->stctxi = gctx->stapi->create_context(gctx->stapi, gdpy->smapi, 
         &stattribs, &ctx_err, (gshare) ? gshare->stctxi : NULL);
   if (!gctx->stctxi) {
      FREE(gctx);
      return NULL;
   }

   gctx->stctxi->st_manager_private = (void *) &gctx->base;

   return &gctx->base;
}

/**
 * Destroy a context.
 */
static void
destroy_context(_EGLDisplay *dpy, _EGLContext *ctx)
{
   struct egl_g3d_context *gctx = egl_g3d_context(ctx);

   /* FIXME a context might live longer than its display */
   if (!dpy->Initialized)
      _eglLog(_EGL_FATAL, "destroy a context with an unitialized display");

   gctx->stctxi->destroy(gctx->stctxi);

   FREE(gctx);
}

static EGLBoolean
egl_g3d_destroy_context(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx)
{
   if (_eglPutContext(ctx))
      destroy_context(dpy, ctx);
   return EGL_TRUE;
}

struct egl_g3d_create_surface_arg {
   EGLint type;
   union {
      EGLNativeWindowType win;
      EGLNativePixmapType pix;
   } u;
};

static _EGLSurface *
egl_g3d_create_surface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                       struct egl_g3d_create_surface_arg *arg,
                       const EGLint *attribs)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_config *gconf = egl_g3d_config(conf);
   struct egl_g3d_surface *gsurf;
   struct native_surface *nsurf;
   const char *err;

   switch (arg->type) {
   case EGL_WINDOW_BIT:
      err = "eglCreateWindowSurface";
      break;
   case EGL_PIXMAP_BIT:
      err = "eglCreatePixmapSurface";
      break;
#ifdef EGL_MESA_screen_surface
   case EGL_SCREEN_BIT_MESA:
      err = "eglCreateScreenSurface";
      break;
#endif
   default:
      err = "eglCreateUnknownSurface";
      break;
   }

   gsurf = CALLOC_STRUCT(egl_g3d_surface);
   if (!gsurf) {
      _eglError(EGL_BAD_ALLOC, err);
      return NULL;
   }

   if (!_eglInitSurface(&gsurf->base, dpy, arg->type, conf, attribs)) {
      FREE(gsurf);
      return NULL;
   }

   /* create the native surface */
   switch (arg->type) {
   case EGL_WINDOW_BIT:
      nsurf = gdpy->native->create_window_surface(gdpy->native,
            arg->u.win, gconf->native);
      break;
   case EGL_PIXMAP_BIT:
      nsurf = gdpy->native->create_pixmap_surface(gdpy->native,
            arg->u.pix, gconf->native);
      break;
#ifdef EGL_MESA_screen_surface
   case EGL_SCREEN_BIT_MESA:
      /* prefer back buffer (move to _eglInitSurface?) */
      gsurf->base.RenderBuffer = EGL_BACK_BUFFER;
      nsurf = gdpy->native->modeset->create_scanout_surface(gdpy->native,
            gconf->native, gsurf->base.Width, gsurf->base.Height);
      break;
#endif
   default:
      nsurf = NULL;
      break;
   }

   if (!nsurf) {
      FREE(gsurf);
      return NULL;
   }
   /* initialize the geometry */
   if (!nsurf->validate(nsurf, 0x0, &gsurf->sequence_number, NULL,
            &gsurf->base.Width, &gsurf->base.Height)) {
      nsurf->destroy(nsurf);
      FREE(gsurf);
      return NULL;
   }

   gsurf->stvis = gconf->stvis;
   if (gsurf->base.RenderBuffer == EGL_SINGLE_BUFFER &&
       gconf->stvis.buffer_mask & ST_ATTACHMENT_FRONT_LEFT_MASK)
      gsurf->stvis.render_buffer = ST_ATTACHMENT_FRONT_LEFT;

   /* surfaces can always be posted when the display supports it */
   if (dpy->Extensions.NV_post_sub_buffer)
      gsurf->base.PostSubBufferSupportedNV = EGL_TRUE;

   gsurf->stfbi = egl_g3d_create_st_framebuffer(&gsurf->base);
   if (!gsurf->stfbi) {
      nsurf->destroy(nsurf);
      FREE(gsurf);
      return NULL;
   }

   nsurf->user_data = &gsurf->base;
   gsurf->native = nsurf;

   return &gsurf->base;
}

static _EGLSurface *
egl_g3d_create_window_surface(_EGLDriver *drv, _EGLDisplay *dpy,
                              _EGLConfig *conf, EGLNativeWindowType win,
                              const EGLint *attribs)
{
   struct egl_g3d_create_surface_arg arg;

   memset(&arg, 0, sizeof(arg));
   arg.type = EGL_WINDOW_BIT;
   arg.u.win = win;

   return egl_g3d_create_surface(drv, dpy, conf, &arg, attribs);
}

static _EGLSurface *
egl_g3d_create_pixmap_surface(_EGLDriver *drv, _EGLDisplay *dpy,
                              _EGLConfig *conf, EGLNativePixmapType pix,
                              const EGLint *attribs)
{
   struct egl_g3d_create_surface_arg arg;

   memset(&arg, 0, sizeof(arg));
   arg.type = EGL_PIXMAP_BIT;
   arg.u.pix = pix;

   return egl_g3d_create_surface(drv, dpy, conf, &arg, attribs);
}

static struct egl_g3d_surface *
create_pbuffer_surface(_EGLDisplay *dpy, _EGLConfig *conf,
                       const EGLint *attribs, const char *func)
{
   struct egl_g3d_config *gconf = egl_g3d_config(conf);
   struct egl_g3d_surface *gsurf;

   gsurf = CALLOC_STRUCT(egl_g3d_surface);
   if (!gsurf) {
      _eglError(EGL_BAD_ALLOC, func);
      return NULL;
   }

   if (!_eglInitSurface(&gsurf->base, dpy, EGL_PBUFFER_BIT, conf, attribs)) {
      FREE(gsurf);
      return NULL;
   }

   gsurf->stvis = gconf->stvis;

   gsurf->stfbi = egl_g3d_create_st_framebuffer(&gsurf->base);
   if (!gsurf->stfbi) {
      FREE(gsurf);
      return NULL;
   }

   return gsurf;
}

static _EGLSurface *
egl_g3d_create_pbuffer_surface(_EGLDriver *drv, _EGLDisplay *dpy,
                               _EGLConfig *conf, const EGLint *attribs)
{
   struct egl_g3d_surface *gsurf;

   gsurf = create_pbuffer_surface(dpy, conf, attribs,
         "eglCreatePbufferSurface");
   if (!gsurf)
      return NULL;

   gsurf->client_buffer_type = EGL_NONE;

   return &gsurf->base;
}

static _EGLSurface *
egl_g3d_create_pbuffer_from_client_buffer(_EGLDriver *drv, _EGLDisplay *dpy,
                                          EGLenum buftype,
                                          EGLClientBuffer buffer,
                                          _EGLConfig *conf,
                                          const EGLint *attribs)
{
   struct egl_g3d_surface *gsurf;
   struct pipe_resource *ptex = NULL;
   EGLint pbuffer_attribs[32];
   EGLint count, i;

   switch (buftype) {
   case EGL_OPENVG_IMAGE:
      break;
   default:
      _eglError(EGL_BAD_PARAMETER, "eglCreatePbufferFromClientBuffer");
      return NULL;
      break;
   }

   /* parse the attributes first */
   count = 0;
   for (i = 0; attribs && attribs[i] != EGL_NONE; i++) {
      EGLint attr = attribs[i++];
      EGLint val = attribs[i];
      EGLint err = EGL_SUCCESS;

      switch (attr) {
      case EGL_TEXTURE_FORMAT:
      case EGL_TEXTURE_TARGET:
      case EGL_MIPMAP_TEXTURE:
         pbuffer_attribs[count++] = attr;
         pbuffer_attribs[count++] = val;
         break;
      default:
         err = EGL_BAD_ATTRIBUTE;
         break;
      }
      /* bail out */
      if (err != EGL_SUCCESS) {
         _eglError(err, "eglCreatePbufferFromClientBuffer");
         return NULL;
      }
   }

   pbuffer_attribs[count++] = EGL_NONE;

   gsurf = create_pbuffer_surface(dpy, conf, pbuffer_attribs,
         "eglCreatePbufferFromClientBuffer");
   if (!gsurf)
      return NULL;

   gsurf->client_buffer_type = buftype;
   gsurf->client_buffer = buffer;

   /* validate now so that it fails if the client buffer is invalid */
   if (!gsurf->stfbi->validate(gsurf->stfbi,
            &gsurf->stvis.render_buffer, 1, &ptex)) {
      egl_g3d_destroy_st_framebuffer(gsurf->stfbi);
      FREE(gsurf);
      return NULL;
   }
   pipe_resource_reference(&ptex, NULL);

   return &gsurf->base;
}

/**
 * Destroy a surface.
 */
static void
destroy_surface(_EGLDisplay *dpy, _EGLSurface *surf)
{
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);

   /* FIXME a surface might live longer than its display */
   if (!dpy->Initialized)
      _eglLog(_EGL_FATAL, "destroy a surface with an unitialized display");

   pipe_resource_reference(&gsurf->render_texture, NULL);
   egl_g3d_destroy_st_framebuffer(gsurf->stfbi);
   if (gsurf->native)
      gsurf->native->destroy(gsurf->native);
   FREE(gsurf);
}

static EGLBoolean
egl_g3d_destroy_surface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf)
{
   if (_eglPutSurface(surf))
      destroy_surface(dpy, surf);
   return EGL_TRUE;
}

static EGLBoolean
egl_g3d_make_current(_EGLDriver *drv, _EGLDisplay *dpy,
                     _EGLSurface *draw, _EGLSurface *read, _EGLContext *ctx)
{
   struct egl_g3d_context *gctx = egl_g3d_context(ctx);
   struct egl_g3d_surface *gdraw = egl_g3d_surface(draw);
   struct egl_g3d_surface *gread = egl_g3d_surface(read);
   struct egl_g3d_context *old_gctx;
   _EGLContext *old_ctx;
   _EGLSurface *old_draw, *old_read;
   EGLBoolean ok = EGL_TRUE;

   /* make new bindings */
   if (!_eglBindContext(ctx, draw, read, &old_ctx, &old_draw, &old_read))
      return EGL_FALSE;

   old_gctx = egl_g3d_context(old_ctx);
   if (old_gctx) {
      /* flush old context */
      old_gctx->stctxi->flush(old_gctx->stctxi, ST_FLUSH_FRONT, NULL);
   }

   if (gctx) {
      ok = gctx->stapi->make_current(gctx->stapi, gctx->stctxi,
            (gdraw) ? gdraw->stfbi : NULL, (gread) ? gread->stfbi : NULL);
      if (ok) {
         if (gdraw) {
            if (gdraw->base.Type == EGL_WINDOW_BIT) {
               gctx->base.WindowRenderBuffer =
                  (gdraw->stvis.render_buffer == ST_ATTACHMENT_FRONT_LEFT) ?
                  EGL_SINGLE_BUFFER : EGL_BACK_BUFFER;
            }
         }
      }
   }
   else if (old_gctx) {
      ok = old_gctx->stapi->make_current(old_gctx->stapi, NULL, NULL, NULL);
      if (ok)
         old_gctx->base.WindowRenderBuffer = EGL_NONE;
   }

   if (ok) {
      if (_eglPutContext(old_ctx))
         destroy_context(dpy, old_ctx);
      if (_eglPutSurface(old_draw))
         destroy_surface(dpy, old_draw);
      if (_eglPutSurface(old_read))
         destroy_surface(dpy, old_read);
   }
   else {
      /* undo the previous _eglBindContext */
      _eglBindContext(old_ctx, old_draw, old_read, &ctx, &draw, &read);
      assert(&gctx->base == ctx &&
             &gdraw->base == draw &&
             &gread->base == read);

      _eglPutSurface(draw);
      _eglPutSurface(read);
      _eglPutContext(ctx);

      _eglPutSurface(old_draw);
      _eglPutSurface(old_read);
      _eglPutContext(old_ctx);
   }

   return ok;
}

static EGLBoolean
swap_buffers(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf,
             EGLint num_rects, const EGLint *rects, EGLBoolean preserve)
{
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);
   _EGLContext *ctx = _eglGetCurrentContext();
   struct egl_g3d_context *gctx = NULL;
   struct native_present_control ctrl;

   /* no-op for pixmap or pbuffer surface */
   if (gsurf->base.Type == EGL_PIXMAP_BIT ||
       gsurf->base.Type == EGL_PBUFFER_BIT)
      return EGL_TRUE;

   /* or when the surface is single-buffered */
   if (gsurf->stvis.render_buffer == ST_ATTACHMENT_FRONT_LEFT)
      return EGL_TRUE;

   if (ctx && ctx->DrawSurface == surf)
      gctx = egl_g3d_context(ctx);

   /* flush if the surface is current */
   if (gctx) {
      gctx->stctxi->flush(gctx->stctxi, ST_FLUSH_FRONT, NULL);
   }

   memset(&ctrl, 0, sizeof(ctrl));
   ctrl.natt = NATIVE_ATTACHMENT_BACK_LEFT;
   ctrl.preserve = preserve;
   ctrl.swap_interval = gsurf->base.SwapInterval;
   ctrl.premultiplied_alpha = (gsurf->base.VGAlphaFormat == EGL_VG_ALPHA_FORMAT_PRE);
   ctrl.num_rects = num_rects;
   ctrl.rects = rects;

   return gsurf->native->present(gsurf->native, &ctrl);
}

static EGLBoolean
egl_g3d_swap_buffers(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf)
{
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);

   return swap_buffers(drv, dpy, surf, 0, NULL,
                       (gsurf->base.SwapBehavior == EGL_BUFFER_PRESERVED));
}

#ifdef EGL_NOK_swap_region
static EGLBoolean
egl_g3d_swap_buffers_region(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf,
                            EGLint num_rects, const EGLint *rects)
{
   /* Note: y=0=top */
   return swap_buffers(drv, dpy, surf, num_rects, rects, EGL_TRUE);
}
#endif /* EGL_NOK_swap_region */

static EGLBoolean
egl_g3d_post_sub_buffer(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf,
                        EGLint x, EGLint y, EGLint width, EGLint height)
{
   EGLint rect[4];

   if (x < 0 || y < 0 || width < 0 || height < 0)
      return _eglError(EGL_BAD_PARAMETER, "eglPostSubBufferNV");

   /* clamp */
   if (x + width > surf->Width)
      width = surf->Width - x;
   if (y + height > surf->Height)
      height = surf->Height - y;

   if (width <= 0 || height <= 0)
      return EGL_TRUE;

   rect[0] = x;
   /* Note: y=0=bottom */
   rect[1] = surf->Height - y - height;
   rect[2] = width;
   rect[3] = height;

   return swap_buffers(drv, dpy, surf, 1, rect, EGL_TRUE);
}

static EGLBoolean
egl_g3d_copy_buffers(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf,
                     EGLNativePixmapType target)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);
   _EGLContext *ctx = _eglGetCurrentContext();

   if (!gsurf->render_texture)
      return EGL_TRUE;

   /* flush if the surface is current */
   if (ctx && ctx->DrawSurface == &gsurf->base) {
      struct egl_g3d_context *gctx = egl_g3d_context(ctx);
      gctx->stctxi->flush(gctx->stctxi, ST_FLUSH_FRONT, NULL);
   }

   return gdpy->native->copy_to_pixmap(gdpy->native,
         target, gsurf->render_texture);
}

static EGLBoolean
egl_g3d_wait_client(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_context *gctx = egl_g3d_context(ctx);
   struct pipe_screen *screen = gdpy->native->screen;
   struct pipe_fence_handle *fence = NULL;

   gctx->stctxi->flush(gctx->stctxi, ST_FLUSH_FRONT, &fence);
   if (fence) {
      screen->fence_finish(screen, fence, PIPE_TIMEOUT_INFINITE);
      screen->fence_reference(screen, &fence, NULL);
   }

   return EGL_TRUE;
}

static EGLBoolean
egl_g3d_wait_native(_EGLDriver *drv, _EGLDisplay *dpy, EGLint engine)
{
   _EGLContext *ctx = _eglGetCurrentContext();

   if (engine != EGL_CORE_NATIVE_ENGINE)
      return _eglError(EGL_BAD_PARAMETER, "eglWaitNative");

   if (ctx && ctx->DrawSurface) {
      struct egl_g3d_surface *gsurf = egl_g3d_surface(ctx->DrawSurface);

      if (gsurf->native)
         gsurf->native->wait(gsurf->native);
   }

   return EGL_TRUE;
}

static EGLBoolean
egl_g3d_bind_tex_image(_EGLDriver *drv, _EGLDisplay *dpy,
                       _EGLSurface *surf, EGLint buffer)
{
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);
   _EGLContext *es1 = _eglGetAPIContext(EGL_OPENGL_ES_API);
   struct egl_g3d_context *gctx;
   enum pipe_format internal_format;
   enum st_texture_type target;

   if (!gsurf || gsurf->base.Type != EGL_PBUFFER_BIT)
      return _eglError(EGL_BAD_SURFACE, "eglBindTexImage");
   if (buffer != EGL_BACK_BUFFER)
      return _eglError(EGL_BAD_PARAMETER, "eglBindTexImage");
   if (gsurf->base.BoundToTexture)
      return _eglError(EGL_BAD_ACCESS, "eglBindTexImage");

   switch (gsurf->base.TextureFormat) {
   case EGL_TEXTURE_RGB:
      internal_format = PIPE_FORMAT_R8G8B8_UNORM;
      break;
   case EGL_TEXTURE_RGBA:
      internal_format = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   default:
      return _eglError(EGL_BAD_MATCH, "eglBindTexImage");
   }

   switch (gsurf->base.TextureTarget) {
   case EGL_TEXTURE_2D:
      target = ST_TEXTURE_2D;
      break;
   default:
      return _eglError(EGL_BAD_MATCH, "eglBindTexImage");
   }

   if (!es1)
      return EGL_TRUE;
   if (!gsurf->render_texture)
      return EGL_FALSE;

   /* flush properly if the surface is bound */
   if (gsurf->base.CurrentContext) {
      gctx = egl_g3d_context(gsurf->base.CurrentContext);
      gctx->stctxi->flush(gctx->stctxi, ST_FLUSH_FRONT, NULL);
   }

   gctx = egl_g3d_context(es1);
   if (gctx->stctxi->teximage) {
      if (!gctx->stctxi->teximage(gctx->stctxi, target,
               gsurf->base.MipmapLevel, internal_format,
               gsurf->render_texture, gsurf->base.MipmapTexture))
         return EGL_FALSE;
      gsurf->base.BoundToTexture = EGL_TRUE;
   }

   return EGL_TRUE;
}

static EGLBoolean
egl_g3d_release_tex_image(_EGLDriver *drv, _EGLDisplay *dpy,
                          _EGLSurface *surf, EGLint buffer)
{
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);

   if (!gsurf || gsurf->base.Type != EGL_PBUFFER_BIT ||
       !gsurf->base.BoundToTexture)
      return _eglError(EGL_BAD_SURFACE, "eglReleaseTexImage");
   if (buffer != EGL_BACK_BUFFER)
      return _eglError(EGL_BAD_PARAMETER, "eglReleaseTexImage");

   if (gsurf->render_texture) {
      _EGLContext *ctx = _eglGetAPIContext(EGL_OPENGL_ES_API);
      struct egl_g3d_context *gctx = egl_g3d_context(ctx);

      /* what if the context the surface binds to is no longer current? */
      if (gctx) {
         gctx->stctxi->teximage(gctx->stctxi, ST_TEXTURE_2D,
               gsurf->base.MipmapLevel, PIPE_FORMAT_NONE, NULL, FALSE);
      }
   }

   gsurf->base.BoundToTexture = EGL_FALSE;

   return EGL_TRUE;
}

#ifdef EGL_MESA_screen_surface

static _EGLSurface *
egl_g3d_create_screen_surface(_EGLDriver *drv, _EGLDisplay *dpy,
                              _EGLConfig *conf, const EGLint *attribs)
{
   struct egl_g3d_create_surface_arg arg;

   memset(&arg, 0, sizeof(arg));
   arg.type = EGL_SCREEN_BIT_MESA;

   return egl_g3d_create_surface(drv, dpy, conf, &arg, attribs);
}

static EGLBoolean
egl_g3d_show_screen_surface(_EGLDriver *drv, _EGLDisplay *dpy,
                            _EGLScreen *scr, _EGLSurface *surf,
                            _EGLMode *mode)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_screen *gscr = egl_g3d_screen(scr);
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);
   struct native_surface *nsurf;
   const struct native_mode *nmode;
   EGLBoolean changed;

   if (gsurf) {
      EGLint idx;

      if (!mode)
         return _eglError(EGL_BAD_MATCH, "eglShowSurfaceMESA");
      if (gsurf->base.Type != EGL_SCREEN_BIT_MESA)
         return _eglError(EGL_BAD_SURFACE, "eglShowScreenSurfaceMESA");
      if (gsurf->base.Width < mode->Width || gsurf->base.Height < mode->Height)
         return _eglError(EGL_BAD_MATCH,
               "eglShowSurfaceMESA(surface smaller than mode size)");

      /* find the index of the mode */
      for (idx = 0; idx < gscr->base.NumModes; idx++)
         if (mode == &gscr->base.Modes[idx])
            break;
      if (idx >= gscr->base.NumModes) {
         return _eglError(EGL_BAD_MODE_MESA,
               "eglShowSurfaceMESA(unknown mode)");
      }

      nsurf = gsurf->native;
      nmode = gscr->native_modes[idx];
   }
   else {
      if (mode)
         return _eglError(EGL_BAD_MATCH, "eglShowSurfaceMESA");

      /* disable the screen */
      nsurf = NULL;
      nmode = NULL;
   }

   /* TODO surface panning by CRTC choosing */
   changed = gdpy->native->modeset->program(gdpy->native, 0, nsurf,
         gscr->base.OriginX, gscr->base.OriginY, &gscr->native, 1, nmode);
   if (changed) {
      gscr->base.CurrentSurface = &gsurf->base;
      gscr->base.CurrentMode = mode;
   }

   return changed;
}

#endif /* EGL_MESA_screen_surface */

#ifdef EGL_WL_bind_wayland_display

static EGLBoolean
egl_g3d_bind_wayland_display_wl(_EGLDriver *drv, _EGLDisplay *dpy,
                                struct wl_display *wl_dpy)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);

   if (!gdpy->native->wayland_bufmgr)
      return EGL_FALSE;

   return gdpy->native->wayland_bufmgr->bind_display(gdpy->native, wl_dpy);
}

static EGLBoolean
egl_g3d_unbind_wayland_display_wl(_EGLDriver *drv, _EGLDisplay *dpy,
                                  struct wl_display *wl_dpy)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);

   if (!gdpy->native->wayland_bufmgr)
      return EGL_FALSE;

   return gdpy->native->wayland_bufmgr->unbind_display(gdpy->native, wl_dpy);
}

static EGLBoolean
egl_g3d_query_wayland_buffer_wl(_EGLDriver *drv, _EGLDisplay *dpy,
                                struct wl_buffer *buffer,
                                EGLint attribute, EGLint *value)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);

   if (!gdpy->native->wayland_bufmgr)
      return EGL_FALSE;

   return gdpy->native->wayland_bufmgr->query_buffer(gdpy->native,
                                                     buffer, attribute, value);
}
#endif /* EGL_WL_bind_wayland_display */

void
egl_g3d_init_driver_api(_EGLDriver *drv)
{
   _eglInitDriverFallbacks(drv);

   drv->API.ChooseConfig = egl_g3d_choose_config;

   drv->API.CreateContext = egl_g3d_create_context;
   drv->API.DestroyContext = egl_g3d_destroy_context;
   drv->API.CreateWindowSurface = egl_g3d_create_window_surface;
   drv->API.CreatePixmapSurface = egl_g3d_create_pixmap_surface;
   drv->API.CreatePbufferSurface = egl_g3d_create_pbuffer_surface;
   drv->API.CreatePbufferFromClientBuffer = egl_g3d_create_pbuffer_from_client_buffer;
   drv->API.DestroySurface = egl_g3d_destroy_surface;
   drv->API.MakeCurrent = egl_g3d_make_current;
   drv->API.SwapBuffers = egl_g3d_swap_buffers;
   drv->API.CopyBuffers = egl_g3d_copy_buffers;
   drv->API.WaitClient = egl_g3d_wait_client;
   drv->API.WaitNative = egl_g3d_wait_native;

   drv->API.BindTexImage = egl_g3d_bind_tex_image;
   drv->API.ReleaseTexImage = egl_g3d_release_tex_image;

   drv->API.CreateImageKHR = egl_g3d_create_image;
   drv->API.DestroyImageKHR = egl_g3d_destroy_image;
#ifdef EGL_MESA_drm_image
   drv->API.CreateDRMImageMESA = egl_g3d_create_drm_image;
   drv->API.ExportDRMImageMESA = egl_g3d_export_drm_image;
#endif
#ifdef EGL_WL_bind_wayland_display
   drv->API.BindWaylandDisplayWL = egl_g3d_bind_wayland_display_wl;
   drv->API.UnbindWaylandDisplayWL = egl_g3d_unbind_wayland_display_wl;
   drv->API.QueryWaylandBufferWL = egl_g3d_query_wayland_buffer_wl;
#endif

   drv->API.CreateSyncKHR = egl_g3d_create_sync;
   drv->API.DestroySyncKHR = egl_g3d_destroy_sync;
   drv->API.ClientWaitSyncKHR = egl_g3d_client_wait_sync;
   drv->API.SignalSyncKHR = egl_g3d_signal_sync;

#ifdef EGL_MESA_screen_surface
   drv->API.CreateScreenSurfaceMESA = egl_g3d_create_screen_surface;
   drv->API.ShowScreenSurfaceMESA = egl_g3d_show_screen_surface;
#endif

#ifdef EGL_NOK_swap_region
   drv->API.SwapBuffersRegionNOK = egl_g3d_swap_buffers_region;
#endif

   drv->API.PostSubBufferNV = egl_g3d_post_sub_buffer;
}
