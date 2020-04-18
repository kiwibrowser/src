/*
 * Mesa 3-D graphics library
 * Version:  7.8
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

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_hash_table.h"
#include "pipe/p_compiler.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "state_tracker/drm_driver.h"
#include "egllog.h"

#include "native_x11.h"
#include "x11_screen.h"

#include "common/native_helper.h"
#ifdef HAVE_WAYLAND_BACKEND
#include "common/native_wayland_drm_bufmgr_helper.h"
#endif

#ifdef GLX_DIRECT_RENDERING

struct dri2_display {
   struct native_display base;
   Display *dpy;
   boolean own_dpy;

   const struct native_event_handler *event_handler;

   struct x11_screen *xscr;
   int xscr_number;
   const char *dri_driver;
   int dri_major, dri_minor;

   struct dri2_config *configs;
   int num_configs;

   struct util_hash_table *surfaces;
#ifdef HAVE_WAYLAND_BACKEND
   struct wl_drm *wl_server_drm; /* for EGL_WL_bind_wayland_display */
#endif
};

struct dri2_surface {
   struct native_surface base;
   Drawable drawable;
   enum pipe_format color_format;
   struct dri2_display *dri2dpy;

   unsigned int server_stamp;
   unsigned int client_stamp;
   int width, height;
   struct pipe_resource *textures[NUM_NATIVE_ATTACHMENTS];
   uint valid_mask;

   boolean have_back, have_fake;

   struct x11_drawable_buffer *last_xbufs;
   int last_num_xbufs;
};

struct dri2_config {
   struct native_config base;
};

static INLINE struct dri2_display *
dri2_display(const struct native_display *ndpy)
{
   return (struct dri2_display *) ndpy;
}

static INLINE struct dri2_surface *
dri2_surface(const struct native_surface *nsurf)
{
   return (struct dri2_surface *) nsurf;
}

static INLINE struct dri2_config *
dri2_config(const struct native_config *nconf)
{
   return (struct dri2_config *) nconf;
}

/**
 * Process the buffers returned by the server.
 */
static void
dri2_surface_process_drawable_buffers(struct native_surface *nsurf,
                                      struct x11_drawable_buffer *xbufs,
                                      int num_xbufs)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);
   struct dri2_display *dri2dpy = dri2surf->dri2dpy;
   struct pipe_resource templ;
   struct winsys_handle whandle;
   uint valid_mask;
   int i;

   /* free the old textures */
   for (i = 0; i < NUM_NATIVE_ATTACHMENTS; i++)
      pipe_resource_reference(&dri2surf->textures[i], NULL);
   dri2surf->valid_mask = 0x0;

   dri2surf->have_back = FALSE;
   dri2surf->have_fake = FALSE;

   if (!xbufs)
      return;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.last_level = 0;
   templ.width0 = dri2surf->width;
   templ.height0 = dri2surf->height;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.format = dri2surf->color_format;
   templ.bind = PIPE_BIND_RENDER_TARGET;

   valid_mask = 0x0;
   for (i = 0; i < num_xbufs; i++) {
      struct x11_drawable_buffer *xbuf = &xbufs[i];
      const char *desc;
      enum native_attachment natt;

      switch (xbuf->attachment) {
      case DRI2BufferFrontLeft:
         natt = NATIVE_ATTACHMENT_FRONT_LEFT;
         desc = "DRI2 Front Buffer";
         break;
      case DRI2BufferFakeFrontLeft:
         natt = NATIVE_ATTACHMENT_FRONT_LEFT;
         desc = "DRI2 Fake Front Buffer";
         dri2surf->have_fake = TRUE;
         break;
      case DRI2BufferBackLeft:
         natt = NATIVE_ATTACHMENT_BACK_LEFT;
         desc = "DRI2 Back Buffer";
         dri2surf->have_back = TRUE;
         break;
      default:
         desc = NULL;
         break;
      }

      if (!desc || dri2surf->textures[natt]) {
         if (!desc)
            _eglLog(_EGL_WARNING, "unknown buffer %d", xbuf->attachment);
         else
            _eglLog(_EGL_WARNING, "both real and fake front buffers are listed");
         continue;
      }

      memset(&whandle, 0, sizeof(whandle));
      whandle.stride = xbuf->pitch;
      whandle.handle = xbuf->name;
      dri2surf->textures[natt] = dri2dpy->base.screen->resource_from_handle(
         dri2dpy->base.screen, &templ, &whandle);
      if (dri2surf->textures[natt])
         valid_mask |= 1 << natt;
   }

   dri2surf->valid_mask = valid_mask;
}

/**
 * Get the buffers from the server.
 */
static void
dri2_surface_get_buffers(struct native_surface *nsurf, uint buffer_mask)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);
   struct dri2_display *dri2dpy = dri2surf->dri2dpy;
   unsigned int dri2atts[NUM_NATIVE_ATTACHMENTS * 2];
   int num_ins, num_outs, att;
   struct x11_drawable_buffer *xbufs;
   uint bpp = util_format_get_blocksizebits(dri2surf->color_format);
   boolean with_format = FALSE; /* never ask for depth/stencil */

   /* We must get the front on servers which doesn't support with format
    * due to a silly bug in core dri2. You can't copy to/from a buffer
    * that you haven't requested and you recive BadValue errors */
   if (dri2surf->dri2dpy->dri_minor < 1) {
      with_format = FALSE;
      buffer_mask |= (1 << NATIVE_ATTACHMENT_FRONT_LEFT);
   }

   /* prepare the attachments */
   num_ins = 0;
   for (att = 0; att < NUM_NATIVE_ATTACHMENTS; att++) {
      if (native_attachment_mask_test(buffer_mask, att)) {
         unsigned int dri2att;

         switch (att) {
         case NATIVE_ATTACHMENT_FRONT_LEFT:
            dri2att = DRI2BufferFrontLeft;
            break;
         case NATIVE_ATTACHMENT_BACK_LEFT:
            dri2att = DRI2BufferBackLeft;
            break;
         case NATIVE_ATTACHMENT_FRONT_RIGHT:
            dri2att = DRI2BufferFrontRight;
            break;
         case NATIVE_ATTACHMENT_BACK_RIGHT:
            dri2att = DRI2BufferBackRight;
            break;
         default:
            assert(0);
            dri2att = 0;
            break;
         }

         dri2atts[num_ins++] = dri2att;
         if (with_format)
            dri2atts[num_ins++] = bpp;
      }
   }
   if (with_format)
      num_ins /= 2;

   xbufs = x11_drawable_get_buffers(dri2dpy->xscr, dri2surf->drawable,
                                    &dri2surf->width, &dri2surf->height,
                                    dri2atts, with_format, num_ins, &num_outs);

   /* we should be able to do better... */
   if (xbufs && dri2surf->last_num_xbufs == num_outs &&
       memcmp(dri2surf->last_xbufs, xbufs, sizeof(*xbufs) * num_outs) == 0) {
      FREE(xbufs);
      dri2surf->client_stamp = dri2surf->server_stamp;
      return;
   }

   dri2_surface_process_drawable_buffers(&dri2surf->base, xbufs, num_outs);

   dri2surf->server_stamp++;
   dri2surf->client_stamp = dri2surf->server_stamp;

   if (dri2surf->last_xbufs)
      FREE(dri2surf->last_xbufs);
   dri2surf->last_xbufs = xbufs;
   dri2surf->last_num_xbufs = num_outs;
}

/**
 * Update the buffers of the surface.  This is a slow function due to the
 * round-trip to the server.
 */
static boolean
dri2_surface_update_buffers(struct native_surface *nsurf, uint buffer_mask)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);

   dri2_surface_get_buffers(&dri2surf->base, buffer_mask);

   return ((dri2surf->valid_mask & buffer_mask) == buffer_mask);
}

/**
 * Return TRUE if the surface receives DRI2_InvalidateBuffers events.
 */
static INLINE boolean
dri2_surface_receive_events(struct native_surface *nsurf)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);
   return (dri2surf->dri2dpy->dri_minor >= 3);
}

static boolean
dri2_surface_flush_frontbuffer(struct native_surface *nsurf)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);
   struct dri2_display *dri2dpy = dri2surf->dri2dpy;

   /* copy to real front buffer */
   if (dri2surf->have_fake)
      x11_drawable_copy_buffers(dri2dpy->xscr, dri2surf->drawable,
            0, 0, dri2surf->width, dri2surf->height,
            DRI2BufferFakeFrontLeft, DRI2BufferFrontLeft);

   /* force buffers to be updated in next validation call */
   if (!dri2_surface_receive_events(&dri2surf->base)) {
      dri2surf->server_stamp++;
      dri2dpy->event_handler->invalid_surface(&dri2dpy->base,
            &dri2surf->base, dri2surf->server_stamp);
   }

   return TRUE;
}

static boolean
dri2_surface_swap_buffers(struct native_surface *nsurf, int num_rects,
                          const int *rects)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);
   struct dri2_display *dri2dpy = dri2surf->dri2dpy;

   /* copy to front buffer */
   if (dri2surf->have_back) {
      if (num_rects > 0)
         x11_drawable_copy_buffers_region(dri2dpy->xscr, dri2surf->drawable,
               num_rects, rects,
               DRI2BufferBackLeft, DRI2BufferFrontLeft);
      else
         x11_drawable_copy_buffers(dri2dpy->xscr, dri2surf->drawable,
               0, 0, dri2surf->width, dri2surf->height,
               DRI2BufferBackLeft, DRI2BufferFrontLeft);
   }

   /* and update fake front buffer */
   if (dri2surf->have_fake) {
      if (num_rects > 0)
         x11_drawable_copy_buffers_region(dri2dpy->xscr, dri2surf->drawable,
               num_rects, rects,
               DRI2BufferFrontLeft, DRI2BufferFakeFrontLeft);
      else
         x11_drawable_copy_buffers(dri2dpy->xscr, dri2surf->drawable,
               0, 0, dri2surf->width, dri2surf->height,
               DRI2BufferFrontLeft, DRI2BufferFakeFrontLeft);
   }

   /* force buffers to be updated in next validation call */
   if (!dri2_surface_receive_events(&dri2surf->base)) {
      dri2surf->server_stamp++;
      dri2dpy->event_handler->invalid_surface(&dri2dpy->base,
            &dri2surf->base, dri2surf->server_stamp);
   }

   return TRUE;
}

static boolean
dri2_surface_present(struct native_surface *nsurf,
                     const struct native_present_control *ctrl)
{
   boolean ret;

   if (ctrl->swap_interval)
      return FALSE;

   switch (ctrl->natt) {
   case NATIVE_ATTACHMENT_FRONT_LEFT:
      ret = dri2_surface_flush_frontbuffer(nsurf);
      break;
   case NATIVE_ATTACHMENT_BACK_LEFT:
      ret = dri2_surface_swap_buffers(nsurf, ctrl->num_rects, ctrl->rects);
      break;
   default:
      ret = FALSE;
      break;
   }

   return ret;
}

static boolean
dri2_surface_validate(struct native_surface *nsurf, uint attachment_mask,
                      unsigned int *seq_num, struct pipe_resource **textures,
                      int *width, int *height)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);

   if (dri2surf->server_stamp != dri2surf->client_stamp ||
       (dri2surf->valid_mask & attachment_mask) != attachment_mask) {
      if (!dri2_surface_update_buffers(&dri2surf->base, attachment_mask))
         return FALSE;
   }

   if (seq_num)
      *seq_num = dri2surf->client_stamp;

   if (textures) {
      int att;
      for (att = 0; att < NUM_NATIVE_ATTACHMENTS; att++) {
         if (native_attachment_mask_test(attachment_mask, att)) {
            struct pipe_resource *ptex = dri2surf->textures[att];

            textures[att] = NULL;
            pipe_resource_reference(&textures[att], ptex);
         }
      }
   }

   if (width)
      *width = dri2surf->width;
   if (height)
      *height = dri2surf->height;

   return TRUE;
}

static void
dri2_surface_wait(struct native_surface *nsurf)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);
   struct dri2_display *dri2dpy = dri2surf->dri2dpy;

   if (dri2surf->have_fake) {
      x11_drawable_copy_buffers(dri2dpy->xscr, dri2surf->drawable,
            0, 0, dri2surf->width, dri2surf->height,
            DRI2BufferFrontLeft, DRI2BufferFakeFrontLeft);
   }
}

static void
dri2_surface_destroy(struct native_surface *nsurf)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);
   int i;

   if (dri2surf->last_xbufs)
      FREE(dri2surf->last_xbufs);

   for (i = 0; i < NUM_NATIVE_ATTACHMENTS; i++) {
      struct pipe_resource *ptex = dri2surf->textures[i];
      pipe_resource_reference(&ptex, NULL);
   }

   if (dri2surf->drawable) {
      x11_drawable_enable_dri2(dri2surf->dri2dpy->xscr,
            dri2surf->drawable, FALSE);

      util_hash_table_remove(dri2surf->dri2dpy->surfaces,
            (void *) dri2surf->drawable);
   }
   FREE(dri2surf);
}

static struct dri2_surface *
dri2_display_create_surface(struct native_display *ndpy,
                            Drawable drawable,
                            enum pipe_format color_format)
{
   struct dri2_display *dri2dpy = dri2_display(ndpy);
   struct dri2_surface *dri2surf;

   dri2surf = CALLOC_STRUCT(dri2_surface);
   if (!dri2surf)
      return NULL;

   dri2surf->dri2dpy = dri2dpy;
   dri2surf->drawable = drawable;
   dri2surf->color_format = color_format;

   dri2surf->base.destroy = dri2_surface_destroy;
   dri2surf->base.present = dri2_surface_present;
   dri2surf->base.validate = dri2_surface_validate;
   dri2surf->base.wait = dri2_surface_wait;

   if (drawable) {
      x11_drawable_enable_dri2(dri2dpy->xscr, drawable, TRUE);
      /* initialize the geometry */
      dri2_surface_update_buffers(&dri2surf->base, 0x0);

      util_hash_table_set(dri2surf->dri2dpy->surfaces,
            (void *) dri2surf->drawable, (void *) &dri2surf->base);
   }

   return dri2surf;
}

static struct native_surface *
dri2_display_create_window_surface(struct native_display *ndpy,
                                   EGLNativeWindowType win,
                                   const struct native_config *nconf)
{
   struct dri2_surface *dri2surf;

   dri2surf = dri2_display_create_surface(ndpy,
         (Drawable) win, nconf->color_format);
   return (dri2surf) ? &dri2surf->base : NULL;
}

static struct native_surface *
dri2_display_create_pixmap_surface(struct native_display *ndpy,
                                   EGLNativePixmapType pix,
                                   const struct native_config *nconf)
{
   struct dri2_surface *dri2surf;

   if (!nconf) {
      struct dri2_display *dri2dpy = dri2_display(ndpy);
      uint depth, nconf_depth;
      int i;

      depth = x11_drawable_get_depth(dri2dpy->xscr, (Drawable) pix);
      for (i = 0; i < dri2dpy->num_configs; i++) {
         nconf_depth = util_format_get_blocksizebits(
               dri2dpy->configs[i].base.color_format);
         /* simple depth match for now */
         if (depth == nconf_depth ||
             (depth == 24 && depth + 8 == nconf_depth)) {
            nconf = &dri2dpy->configs[i].base;
            break;
         }
      }

      if (!nconf)
         return NULL;
   }

   dri2surf = dri2_display_create_surface(ndpy,
         (Drawable) pix, nconf->color_format);
   return (dri2surf) ? &dri2surf->base : NULL;
}

static int
choose_color_format(const __GLcontextModes *mode, enum pipe_format formats[32])
{
   int count = 0;

   switch (mode->rgbBits) {
   case 32:
      formats[count++] = PIPE_FORMAT_B8G8R8A8_UNORM;
      formats[count++] = PIPE_FORMAT_A8R8G8B8_UNORM;
      break;
   case 24:
      formats[count++] = PIPE_FORMAT_B8G8R8X8_UNORM;
      formats[count++] = PIPE_FORMAT_X8R8G8B8_UNORM;
      formats[count++] = PIPE_FORMAT_B8G8R8A8_UNORM;
      formats[count++] = PIPE_FORMAT_A8R8G8B8_UNORM;
      break;
   case 16:
      formats[count++] = PIPE_FORMAT_B5G6R5_UNORM;
      break;
   default:
      break;
   }

   return count;
}

static boolean
is_format_supported(struct pipe_screen *screen,
                    enum pipe_format fmt, unsigned sample_count, boolean is_color)
{
   return screen->is_format_supported(screen, fmt, PIPE_TEXTURE_2D, sample_count,
         (is_color) ? PIPE_BIND_RENDER_TARGET :
         PIPE_BIND_DEPTH_STENCIL);
}

static boolean
dri2_display_convert_config(struct native_display *ndpy,
                            const __GLcontextModes *mode,
                            struct native_config *nconf)
{
   enum pipe_format formats[32];
   int num_formats, i;
   int sample_count = 0;

   if (!(mode->renderType & GLX_RGBA_BIT) || !mode->rgbMode)
      return FALSE;

   /* only interested in native renderable configs */
   if (!mode->xRenderable || !mode->drawableType)
      return FALSE;

   /* fast/slow configs are probably not relevant */
   if (mode->visualRating == GLX_SLOW_CONFIG)
      return FALSE;

   nconf->buffer_mask = 1 << NATIVE_ATTACHMENT_FRONT_LEFT;
   if (mode->doubleBufferMode)
      nconf->buffer_mask |= 1 << NATIVE_ATTACHMENT_BACK_LEFT;
   if (mode->stereoMode) {
      nconf->buffer_mask |= 1 << NATIVE_ATTACHMENT_FRONT_RIGHT;
      if (mode->doubleBufferMode)
         nconf->buffer_mask |= 1 << NATIVE_ATTACHMENT_BACK_RIGHT;
   }

   /* choose color format */
   num_formats = choose_color_format(mode, formats);
   for (i = 0; i < num_formats; i++) {
      if (is_format_supported(ndpy->screen, formats[i], sample_count, TRUE)) {
         nconf->color_format = formats[i];
         break;
      }
   }
   if (nconf->color_format == PIPE_FORMAT_NONE)
      return FALSE;

   if ((mode->drawableType & GLX_WINDOW_BIT) && mode->visualID)
      nconf->window_bit = TRUE;
   if (mode->drawableType & GLX_PIXMAP_BIT)
      nconf->pixmap_bit = TRUE;

   nconf->native_visual_id = mode->visualID;
   switch (mode->visualType) {
   case GLX_TRUE_COLOR:
      nconf->native_visual_type = TrueColor;
      break;
   case GLX_DIRECT_COLOR:
      nconf->native_visual_type = DirectColor;
      break;
   case GLX_PSEUDO_COLOR:
      nconf->native_visual_type = PseudoColor;
      break;
   case GLX_STATIC_COLOR:
      nconf->native_visual_type = StaticColor;
      break;
   case GLX_GRAY_SCALE:
      nconf->native_visual_type = GrayScale;
      break;
   case GLX_STATIC_GRAY:
      nconf->native_visual_type = StaticGray;
      break;
   }
   nconf->level = mode->level;

   if (mode->transparentPixel == GLX_TRANSPARENT_RGB) {
      nconf->transparent_rgb = TRUE;
      nconf->transparent_rgb_values[0] = mode->transparentRed;
      nconf->transparent_rgb_values[1] = mode->transparentGreen;
      nconf->transparent_rgb_values[2] = mode->transparentBlue;
   }

   return TRUE;
}

static const struct native_config **
dri2_display_get_configs(struct native_display *ndpy, int *num_configs)
{
   struct dri2_display *dri2dpy = dri2_display(ndpy);
   const struct native_config **configs;
   int i;

   /* first time */
   if (!dri2dpy->configs) {
      const __GLcontextModes *modes;
      int num_modes, count;

      modes = x11_screen_get_glx_configs(dri2dpy->xscr);
      if (!modes)
         return NULL;
      num_modes = x11_context_modes_count(modes);

      dri2dpy->configs = CALLOC(num_modes, sizeof(*dri2dpy->configs));
      if (!dri2dpy->configs)
         return NULL;

      count = 0;
      for (i = 0; i < num_modes; i++) {
         struct native_config *nconf = &dri2dpy->configs[count].base;

         if (dri2_display_convert_config(&dri2dpy->base, modes, nconf)) {
            int j;
            /* look for duplicates */
            for (j = 0; j < count; j++) {
               if (memcmp(&dri2dpy->configs[j], nconf, sizeof(*nconf)) == 0)
                  break;
            }
            if (j == count)
               count++;
         }
         modes = modes->next;
      }

      dri2dpy->num_configs = count;
   }

   configs = MALLOC(dri2dpy->num_configs * sizeof(*configs));
   if (configs) {
      for (i = 0; i < dri2dpy->num_configs; i++)
         configs[i] = (const struct native_config *) &dri2dpy->configs[i];
      if (num_configs)
         *num_configs = dri2dpy->num_configs;
   }

   return configs;
}

static boolean
dri2_display_get_pixmap_format(struct native_display *ndpy,
                               EGLNativePixmapType pix,
                               enum pipe_format *format)
{
   struct dri2_display *dri2dpy = dri2_display(ndpy);
   boolean ret = EGL_TRUE;
   uint depth;

   depth = x11_drawable_get_depth(dri2dpy->xscr, (Drawable) pix);
   switch (depth) {
   case 32:
   case 24:
      *format = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   case 16:
      *format = PIPE_FORMAT_B5G6R5_UNORM;
      break;
   default:
      *format = PIPE_FORMAT_NONE;
      ret = EGL_FALSE;
      break;
   }

   return ret;
}

static int
dri2_display_get_param(struct native_display *ndpy,
                       enum native_param_type param)
{
   int val;

   switch (param) {
   case NATIVE_PARAM_USE_NATIVE_BUFFER:
      /* DRI2GetBuffers uses the native buffers */
      val = TRUE;
      break;
   case NATIVE_PARAM_PRESERVE_BUFFER:
      /* DRI2CopyRegion is used */
      val = TRUE;
      break;
   case NATIVE_PARAM_PRESENT_REGION:
      val = TRUE;
      break;
   case NATIVE_PARAM_MAX_SWAP_INTERVAL:
   default:
      val = 0;
      break;
   }

   return val;
}

static void
dri2_display_destroy(struct native_display *ndpy)
{
   struct dri2_display *dri2dpy = dri2_display(ndpy);

   if (dri2dpy->configs)
      FREE(dri2dpy->configs);

   if (dri2dpy->base.screen)
      dri2dpy->base.screen->destroy(dri2dpy->base.screen);

   if (dri2dpy->surfaces)
      util_hash_table_destroy(dri2dpy->surfaces);

   if (dri2dpy->xscr)
      x11_screen_destroy(dri2dpy->xscr);
   if (dri2dpy->own_dpy)
      XCloseDisplay(dri2dpy->dpy);
   FREE(dri2dpy);
}

static void
dri2_display_invalidate_buffers(struct x11_screen *xscr, Drawable drawable,
                                void *user_data)
{
   struct native_display *ndpy = (struct native_display* ) user_data;
   struct dri2_display *dri2dpy = dri2_display(ndpy);
   struct native_surface *nsurf;
   struct dri2_surface *dri2surf;

   nsurf = (struct native_surface *)
      util_hash_table_get(dri2dpy->surfaces, (void *) drawable);
   if (!nsurf)
      return;

   dri2surf = dri2_surface(nsurf);

   dri2surf->server_stamp++;
   dri2dpy->event_handler->invalid_surface(&dri2dpy->base,
         &dri2surf->base, dri2surf->server_stamp);
}

/**
 * Initialize DRI2 and pipe screen.
 */
static boolean
dri2_display_init_screen(struct native_display *ndpy)
{
   struct dri2_display *dri2dpy = dri2_display(ndpy);
   int fd;

   if (!x11_screen_support(dri2dpy->xscr, X11_SCREEN_EXTENSION_DRI2) ||
       !x11_screen_support(dri2dpy->xscr, X11_SCREEN_EXTENSION_GLX)) {
      _eglLog(_EGL_WARNING, "GLX/DRI2 is not supported");
      return FALSE;
   }

   dri2dpy->dri_driver = x11_screen_probe_dri2(dri2dpy->xscr,
         &dri2dpy->dri_major, &dri2dpy->dri_minor);

   fd = x11_screen_enable_dri2(dri2dpy->xscr,
         dri2_display_invalidate_buffers, &dri2dpy->base);
   if (fd < 0)
      return FALSE;

   dri2dpy->base.screen =
      dri2dpy->event_handler->new_drm_screen(&dri2dpy->base,
            dri2dpy->dri_driver, fd);
   if (!dri2dpy->base.screen) {
      _eglLog(_EGL_DEBUG, "failed to create DRM screen");
      return FALSE;
   }

   return TRUE;
}

static unsigned
dri2_display_hash_table_hash(void *key)
{
   XID drawable = pointer_to_uintptr(key);
   return (unsigned) drawable;
}

static int
dri2_display_hash_table_compare(void *key1, void *key2)
{
   return ((char *) key1 - (char *) key2);
}

#ifdef HAVE_WAYLAND_BACKEND

static int
dri2_display_authenticate(void *user_data, uint32_t magic)
{
   struct native_display *ndpy = user_data;
   struct dri2_display *dri2dpy = dri2_display(ndpy);

   return x11_screen_authenticate(dri2dpy->xscr, magic);
}

static struct wayland_drm_callbacks wl_drm_callbacks = {
   dri2_display_authenticate,
   egl_g3d_wl_drm_helper_reference_buffer,
   egl_g3d_wl_drm_helper_unreference_buffer
};

static boolean
dri2_display_bind_wayland_display(struct native_display *ndpy,
                                  struct wl_display *wl_dpy)
{
   struct dri2_display *dri2dpy = dri2_display(ndpy);

   if (dri2dpy->wl_server_drm)
      return FALSE;

   dri2dpy->wl_server_drm = wayland_drm_init(wl_dpy,
         x11_screen_get_device_name(dri2dpy->xscr),
         &wl_drm_callbacks, ndpy);

   if (!dri2dpy->wl_server_drm)
      return FALSE;
   
   return TRUE;
}

static boolean
dri2_display_unbind_wayland_display(struct native_display *ndpy,
                                    struct wl_display *wl_dpy)
{
   struct dri2_display *dri2dpy = dri2_display(ndpy);

   if (!dri2dpy->wl_server_drm)
      return FALSE;

   wayland_drm_uninit(dri2dpy->wl_server_drm);
   dri2dpy->wl_server_drm = NULL;

   return TRUE;
}

static struct native_display_wayland_bufmgr dri2_display_wayland_bufmgr = {
   dri2_display_bind_wayland_display,
   dri2_display_unbind_wayland_display,
   egl_g3d_wl_drm_common_wl_buffer_get_resource,
   egl_g3d_wl_drm_common_query_buffer
};

#endif /* HAVE_WAYLAND_BACKEND */

struct native_display *
x11_create_dri2_display(Display *dpy,
                        const struct native_event_handler *event_handler)
{
   struct dri2_display *dri2dpy;

   dri2dpy = CALLOC_STRUCT(dri2_display);
   if (!dri2dpy)
      return NULL;

   dri2dpy->event_handler = event_handler;

   dri2dpy->dpy = dpy;
   if (!dri2dpy->dpy) {
      dri2dpy->dpy = XOpenDisplay(NULL);
      if (!dri2dpy->dpy) {
         dri2_display_destroy(&dri2dpy->base);
         return NULL;
      }
      dri2dpy->own_dpy = TRUE;
   }

   dri2dpy->xscr_number = DefaultScreen(dri2dpy->dpy);
   dri2dpy->xscr = x11_screen_create(dri2dpy->dpy, dri2dpy->xscr_number);
   if (!dri2dpy->xscr) {
      dri2_display_destroy(&dri2dpy->base);
      return NULL;
   }

   dri2dpy->surfaces = util_hash_table_create(dri2_display_hash_table_hash,
         dri2_display_hash_table_compare);
   if (!dri2dpy->surfaces) {
      dri2_display_destroy(&dri2dpy->base);
      return NULL;
   }

   dri2dpy->base.init_screen = dri2_display_init_screen;
   dri2dpy->base.destroy = dri2_display_destroy;
   dri2dpy->base.get_param = dri2_display_get_param;
   dri2dpy->base.get_configs = dri2_display_get_configs;
   dri2dpy->base.get_pixmap_format = dri2_display_get_pixmap_format;
   dri2dpy->base.copy_to_pixmap = native_display_copy_to_pixmap;
   dri2dpy->base.create_window_surface = dri2_display_create_window_surface;
   dri2dpy->base.create_pixmap_surface = dri2_display_create_pixmap_surface;
#ifdef HAVE_WAYLAND_BACKEND
   dri2dpy->base.wayland_bufmgr = &dri2_display_wayland_bufmgr;
#endif

   return &dri2dpy->base;
}

#else /* GLX_DIRECT_RENDERING */

struct native_display *
x11_create_dri2_display(Display *dpy,
                        const struct native_event_handler *event_handler)
{
   return NULL;
}

#endif /* GLX_DIRECT_RENDERING */
