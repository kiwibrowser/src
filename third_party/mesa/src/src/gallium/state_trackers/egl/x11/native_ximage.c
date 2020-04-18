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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "pipe/p_compiler.h"
#include "util/u_inlines.h"
#include "state_tracker/xlib_sw_winsys.h"
#include "util/u_debug.h"
#include "egllog.h"

#include "common/native_helper.h"
#include "native_x11.h"
#include "x11_screen.h"

struct ximage_display {
   struct native_display base;
   Display *dpy;
   boolean own_dpy;

   const struct native_event_handler *event_handler;

   struct x11_screen *xscr;
   int xscr_number;

   struct ximage_config *configs;
   int num_configs;
};

struct ximage_surface {
   struct native_surface base;
   Drawable drawable;
   enum pipe_format color_format;
   XVisualInfo visual;
   struct ximage_display *xdpy;

   unsigned int server_stamp;
   unsigned int client_stamp;

   struct resource_surface *rsurf;
   struct xlib_drawable xdraw;
};

struct ximage_config {
   struct native_config base;
   const XVisualInfo *visual;
};

static INLINE struct ximage_display *
ximage_display(const struct native_display *ndpy)
{
   return (struct ximage_display *) ndpy;
}

static INLINE struct ximage_surface *
ximage_surface(const struct native_surface *nsurf)
{
   return (struct ximage_surface *) nsurf;
}

static INLINE struct ximage_config *
ximage_config(const struct native_config *nconf)
{
   return (struct ximage_config *) nconf;
}

/**
 * Update the geometry of the surface.  This is a slow functions.
 */
static void
ximage_surface_update_geometry(struct native_surface *nsurf)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);
   Status ok;
   Window root;
   int x, y;
   unsigned int w, h, border, depth;

   ok = XGetGeometry(xsurf->xdpy->dpy, xsurf->drawable,
         &root, &x, &y, &w, &h, &border, &depth);
   if (ok && resource_surface_set_size(xsurf->rsurf, w, h))
      xsurf->server_stamp++;
}

/**
 * Update the buffers of the surface.
 */
static boolean
ximage_surface_update_buffers(struct native_surface *nsurf, uint buffer_mask)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);

   if (xsurf->client_stamp != xsurf->server_stamp) {
      ximage_surface_update_geometry(&xsurf->base);
      xsurf->client_stamp = xsurf->server_stamp;
   }

   return resource_surface_add_resources(xsurf->rsurf, buffer_mask);
}

/**
 * Emulate an invalidate event.
 */
static void
ximage_surface_invalidate(struct native_surface *nsurf)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);
   struct ximage_display *xdpy = xsurf->xdpy;

   xsurf->server_stamp++;
   xdpy->event_handler->invalid_surface(&xdpy->base,
         &xsurf->base, xsurf->server_stamp);
}

static boolean
ximage_surface_flush_frontbuffer(struct native_surface *nsurf)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);
   boolean ret;

   ret = resource_surface_present(xsurf->rsurf,
         NATIVE_ATTACHMENT_FRONT_LEFT, (void *) &xsurf->xdraw);
   /* force buffers to be updated in next validation call */
   ximage_surface_invalidate(&xsurf->base);

   return ret;
}

static boolean
ximage_surface_swap_buffers(struct native_surface *nsurf)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);
   boolean ret;

   ret = resource_surface_present(xsurf->rsurf,
         NATIVE_ATTACHMENT_BACK_LEFT, (void *) &xsurf->xdraw);

   resource_surface_swap_buffers(xsurf->rsurf,
         NATIVE_ATTACHMENT_FRONT_LEFT, NATIVE_ATTACHMENT_BACK_LEFT, TRUE);
   /* the front/back buffers have been swapped */
   ximage_surface_invalidate(&xsurf->base);

   return ret;
}

static boolean
ximage_surface_present(struct native_surface *nsurf,
                       const struct native_present_control *ctrl)
{
   boolean ret;

   if (ctrl->preserve || ctrl->swap_interval)
      return FALSE;

   switch (ctrl->natt) {
   case NATIVE_ATTACHMENT_FRONT_LEFT:
      ret = ximage_surface_flush_frontbuffer(nsurf);
      break;
   case NATIVE_ATTACHMENT_BACK_LEFT:
      ret = ximage_surface_swap_buffers(nsurf);
      break;
   default:
      ret = FALSE;
      break;
   }

   return ret;
}

static boolean
ximage_surface_validate(struct native_surface *nsurf, uint attachment_mask,
                        unsigned int *seq_num, struct pipe_resource **textures,
                        int *width, int *height)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);
   uint w, h;

   if (!ximage_surface_update_buffers(&xsurf->base, attachment_mask))
      return FALSE;

   if (seq_num)
      *seq_num = xsurf->client_stamp;

   if (textures)
      resource_surface_get_resources(xsurf->rsurf, textures, attachment_mask);

   resource_surface_get_size(xsurf->rsurf, &w, &h);
   if (width)
      *width = w;
   if (height)
      *height = h;

   return TRUE;
}

static void
ximage_surface_wait(struct native_surface *nsurf)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);
   XSync(xsurf->xdpy->dpy, FALSE);
   /* TODO XGetImage and update the front texture */
}

static void
ximage_surface_destroy(struct native_surface *nsurf)
{
   struct ximage_surface *xsurf = ximage_surface(nsurf);

   resource_surface_destroy(xsurf->rsurf);
   FREE(xsurf);
}

static struct ximage_surface *
ximage_display_create_surface(struct native_display *ndpy,
                              Drawable drawable,
                              const struct native_config *nconf)
{
   struct ximage_display *xdpy = ximage_display(ndpy);
   struct ximage_config *xconf = ximage_config(nconf);
   struct ximage_surface *xsurf;

   xsurf = CALLOC_STRUCT(ximage_surface);
   if (!xsurf)
      return NULL;

   xsurf->xdpy = xdpy;
   xsurf->color_format = xconf->base.color_format;
   xsurf->drawable = drawable;

   xsurf->rsurf = resource_surface_create(xdpy->base.screen,
         xsurf->color_format,
         PIPE_BIND_RENDER_TARGET |
         PIPE_BIND_SAMPLER_VIEW |
         PIPE_BIND_DISPLAY_TARGET |
         PIPE_BIND_SCANOUT);
   if (!xsurf->rsurf) {
      FREE(xsurf);
      return NULL;
   }

   xsurf->drawable = drawable;
   xsurf->visual = *xconf->visual;
   /* initialize the geometry */
   ximage_surface_update_geometry(&xsurf->base);

   xsurf->xdraw.visual = xsurf->visual.visual;
   xsurf->xdraw.depth = xsurf->visual.depth;
   xsurf->xdraw.drawable = xsurf->drawable;

   xsurf->base.destroy = ximage_surface_destroy;
   xsurf->base.present = ximage_surface_present;
   xsurf->base.validate = ximage_surface_validate;
   xsurf->base.wait = ximage_surface_wait;

   return xsurf;
}

static struct native_surface *
ximage_display_create_window_surface(struct native_display *ndpy,
                                     EGLNativeWindowType win,
                                     const struct native_config *nconf)
{
   struct ximage_surface *xsurf;

   xsurf = ximage_display_create_surface(ndpy, (Drawable) win, nconf);
   return (xsurf) ? &xsurf->base : NULL;
}

static enum pipe_format
get_pixmap_format(struct native_display *ndpy, EGLNativePixmapType pix)
{
   struct ximage_display *xdpy = ximage_display(ndpy);
   enum pipe_format fmt;
   uint depth;

   depth = x11_drawable_get_depth(xdpy->xscr, (Drawable) pix);

   switch (depth) {
   case 32:
      fmt = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   case 24:
      fmt = PIPE_FORMAT_B8G8R8X8_UNORM;
      break;
   case 16:
      fmt = PIPE_FORMAT_B5G6R5_UNORM;
      break;
   default:
      fmt = PIPE_FORMAT_NONE;
      break;
   }

   return fmt;
}

static struct native_surface *
ximage_display_create_pixmap_surface(struct native_display *ndpy,
                                     EGLNativePixmapType pix,
                                     const struct native_config *nconf)
{
   struct ximage_surface *xsurf;

   /* find the config */
   if (!nconf) {
      struct ximage_display *xdpy = ximage_display(ndpy);
      enum pipe_format fmt = get_pixmap_format(&xdpy->base, pix);
      int i;

      if (fmt != PIPE_FORMAT_NONE) {
         for (i = 0; i < xdpy->num_configs; i++) {
            if (xdpy->configs[i].base.color_format == fmt) {
               nconf = &xdpy->configs[i].base;
               break;
            }
         }
      }

      if (!nconf)
         return NULL;
   }

   xsurf = ximage_display_create_surface(ndpy, (Drawable) pix, nconf);
   return (xsurf) ? &xsurf->base : NULL;
}

static enum pipe_format
choose_format(const XVisualInfo *vinfo)
{
   enum pipe_format fmt;
   /* TODO elaborate the formats */
   switch (vinfo->depth) {
   case 32:
      fmt = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   case 24:
      fmt = PIPE_FORMAT_B8G8R8X8_UNORM;
      break;
   case 16:
      fmt = PIPE_FORMAT_B5G6R5_UNORM;
      break;
   default:
      fmt = PIPE_FORMAT_NONE;
      break;
   }

   return fmt;
}

static const struct native_config **
ximage_display_get_configs(struct native_display *ndpy, int *num_configs)
{
   struct ximage_display *xdpy = ximage_display(ndpy);
   const struct native_config **configs;
   int i;

   /* first time */
   if (!xdpy->configs) {
      const XVisualInfo *visuals;
      int num_visuals, count;

      visuals = x11_screen_get_visuals(xdpy->xscr, &num_visuals);
      if (!visuals)
         return NULL;

      /*
       * Create two configs for each visual.
       * One with depth/stencil buffer; one without
       */
      xdpy->configs = CALLOC(num_visuals * 2, sizeof(*xdpy->configs));
      if (!xdpy->configs)
         return NULL;

      count = 0;
      for (i = 0; i < num_visuals; i++) {
         struct ximage_config *xconf = &xdpy->configs[count];

         xconf->visual = &visuals[i];
         xconf->base.color_format = choose_format(xconf->visual);
         if (xconf->base.color_format == PIPE_FORMAT_NONE)
            continue;

         xconf->base.buffer_mask =
            (1 << NATIVE_ATTACHMENT_FRONT_LEFT) |
            (1 << NATIVE_ATTACHMENT_BACK_LEFT);

         xconf->base.window_bit = TRUE;
         xconf->base.pixmap_bit = TRUE;

         xconf->base.native_visual_id = xconf->visual->visualid;
#if defined(__cplusplus) || defined(c_plusplus)
         xconf->base.native_visual_type = xconf->visual->c_class;
#else
         xconf->base.native_visual_type = xconf->visual->class;
#endif

         count++;
      }

      xdpy->num_configs = count;
   }

   configs = MALLOC(xdpy->num_configs * sizeof(*configs));
   if (configs) {
      for (i = 0; i < xdpy->num_configs; i++)
         configs[i] = (const struct native_config *) &xdpy->configs[i];
      if (num_configs)
         *num_configs = xdpy->num_configs;
   }
   return configs;
}

static boolean
ximage_display_get_pixmap_format(struct native_display *ndpy,
                                 EGLNativePixmapType pix,
                                 enum pipe_format *format)
{
   struct ximage_display *xdpy = ximage_display(ndpy);

   *format = get_pixmap_format(&xdpy->base, pix);

   return (*format != PIPE_FORMAT_NONE);
}

static boolean
ximage_display_copy_to_pixmap(struct native_display *ndpy,
                              EGLNativePixmapType pix,
                              struct pipe_resource *src)
{
   /* fast path to avoid unnecessary allocation and resource_copy_region */
   if (src->bind & PIPE_BIND_DISPLAY_TARGET) {
      struct ximage_display *xdpy = ximage_display(ndpy);
      enum pipe_format fmt = get_pixmap_format(&xdpy->base, pix);
      const struct ximage_config *xconf = NULL;
      struct xlib_drawable xdraw;
      int i;

      if (fmt == PIPE_FORMAT_NONE || src->format != fmt)
         return FALSE;

      for (i = 0; i < xdpy->num_configs; i++) {
         if (xdpy->configs[i].base.color_format == fmt) {
            xconf = &xdpy->configs[i];
            break;
         }
      }
      if (!xconf)
         return FALSE;

      memset(&xdraw, 0, sizeof(xdraw));
      xdraw.visual = xconf->visual->visual;
      xdraw.depth = xconf->visual->depth;
      xdraw.drawable = (Drawable) pix;

      xdpy->base.screen->flush_frontbuffer(xdpy->base.screen,
            src, 0, 0, &xdraw);

      return TRUE;
   }

   return native_display_copy_to_pixmap(ndpy, pix, src);
}

static int
ximage_display_get_param(struct native_display *ndpy,
                         enum native_param_type param)
{
   int val;

   switch (param) {
   case NATIVE_PARAM_USE_NATIVE_BUFFER:
      /* private buffers are allocated */
      val = FALSE;
      break;
   case NATIVE_PARAM_PRESERVE_BUFFER:
   case NATIVE_PARAM_MAX_SWAP_INTERVAL:
   default:
      val = 0;
      break;
   }

   return val;
}

static void
ximage_display_destroy(struct native_display *ndpy)
{
   struct ximage_display *xdpy = ximage_display(ndpy);

   if (xdpy->configs)
      FREE(xdpy->configs);

   ndpy_uninit(ndpy);

   x11_screen_destroy(xdpy->xscr);
   if (xdpy->own_dpy)
      XCloseDisplay(xdpy->dpy);
   FREE(xdpy);
}

static boolean
ximage_display_init_screen(struct native_display *ndpy)
{
   struct ximage_display *xdpy = ximage_display(ndpy);
   struct sw_winsys *winsys;

   winsys = xlib_create_sw_winsys(xdpy->dpy);
   if (!winsys)
      return FALSE;

   xdpy->base.screen =
      xdpy->event_handler->new_sw_screen(&xdpy->base, winsys);
   if (!xdpy->base.screen) {
      if (winsys->destroy)
         winsys->destroy(winsys);
      return FALSE;
   }

   return TRUE;
}

struct native_display *
x11_create_ximage_display(Display *dpy,
                          const struct native_event_handler *event_handler)
{
   struct ximage_display *xdpy;

   xdpy = CALLOC_STRUCT(ximage_display);
   if (!xdpy)
      return NULL;

   xdpy->dpy = dpy;
   if (!xdpy->dpy) {
      xdpy->dpy = XOpenDisplay(NULL);
      if (!xdpy->dpy) {
         FREE(xdpy);
         return NULL;
      }
      xdpy->own_dpy = TRUE;
   }

   xdpy->event_handler = event_handler;

   xdpy->xscr_number = DefaultScreen(xdpy->dpy);
   xdpy->xscr = x11_screen_create(xdpy->dpy, xdpy->xscr_number);
   if (!xdpy->xscr) {
      if (xdpy->own_dpy)
         XCloseDisplay(xdpy->dpy);
      FREE(xdpy);
      return NULL;
   }

   xdpy->base.init_screen = ximage_display_init_screen;
   xdpy->base.destroy = ximage_display_destroy;
   xdpy->base.get_param = ximage_display_get_param;

   xdpy->base.get_configs = ximage_display_get_configs;
   xdpy->base.get_pixmap_format = ximage_display_get_pixmap_format;
   xdpy->base.copy_to_pixmap = ximage_display_copy_to_pixmap;
   xdpy->base.create_window_surface = ximage_display_create_window_surface;
   xdpy->base.create_pixmap_surface = ximage_display_create_pixmap_surface;

   return &xdpy->base;
}
