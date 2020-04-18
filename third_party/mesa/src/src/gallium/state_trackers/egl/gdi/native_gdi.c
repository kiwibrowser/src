/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
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
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include <windows.h>

#include "pipe/p_compiler.h"
#include "util/u_memory.h"
#include "util/u_format.h"
#include "util/u_inlines.h"
#include "gdi/gdi_sw_winsys.h"

#include "common/native_helper.h"
#include "common/native.h"

struct gdi_display {
   struct native_display base;

   HDC hDC;
   const struct native_event_handler *event_handler;

   struct native_config *configs;
   int num_configs;
};

struct gdi_surface {
   struct native_surface base;

   HWND hWnd;
   enum pipe_format color_format;

   struct gdi_display *gdpy;

   unsigned int server_stamp;
   unsigned int client_stamp;

   struct resource_surface *rsurf;
};

static INLINE struct gdi_display *
gdi_display(const struct native_display *ndpy)
{
   return (struct gdi_display *) ndpy;
}

static INLINE struct gdi_surface *
gdi_surface(const struct native_surface *nsurf)
{
   return (struct gdi_surface *) nsurf;
}

/**
 * Update the geometry of the surface.  This is a slow functions.
 */
static void
gdi_surface_update_geometry(struct native_surface *nsurf)
{
   struct gdi_surface *gsurf = gdi_surface(nsurf);
   RECT rect;
   uint w, h;

   GetClientRect(gsurf->hWnd, &rect);
   w = rect.right - rect.left;
   h = rect.bottom - rect.top;

   if (resource_surface_set_size(gsurf->rsurf, w, h))
      gsurf->server_stamp++;
}

/**
 * Update the buffers of the surface.
 */
static boolean
gdi_surface_update_buffers(struct native_surface *nsurf, uint buffer_mask)
{
   struct gdi_surface *gsurf = gdi_surface(nsurf);

   if (gsurf->client_stamp != gsurf->server_stamp) {
      gdi_surface_update_geometry(&gsurf->base);
      gsurf->client_stamp = gsurf->server_stamp;
   }

   return resource_surface_add_resources(gsurf->rsurf, buffer_mask);
}

/**
 * Emulate an invalidate event.
 */
static void
gdi_surface_invalidate(struct native_surface *nsurf)
{
   struct gdi_surface *gsurf = gdi_surface(nsurf);
   struct gdi_display *gdpy = gsurf->gdpy;

   gsurf->server_stamp++;
   gdpy->event_handler->invalid_surface(&gdpy->base,
         &gsurf->base, gsurf->server_stamp);
}

static boolean
gdi_surface_flush_frontbuffer(struct native_surface *nsurf)
{
   struct gdi_surface *gsurf = gdi_surface(nsurf);
   HDC hDC;
   boolean ret;

   hDC = GetDC(gsurf->hWnd);
   ret = resource_surface_present(gsurf->rsurf,
         NATIVE_ATTACHMENT_FRONT_LEFT, (void *) hDC);
   ReleaseDC(gsurf->hWnd, hDC);

   /* force buffers to be updated in next validation call */
   gdi_surface_invalidate(&gsurf->base);

   return ret;
}

static boolean
gdi_surface_swap_buffers(struct native_surface *nsurf)
{
   struct gdi_surface *gsurf = gdi_surface(nsurf);
   HDC hDC;
   boolean ret;

   hDC = GetDC(gsurf->hWnd);
   ret = resource_surface_present(gsurf->rsurf,
         NATIVE_ATTACHMENT_BACK_LEFT, (void *) hDC);
   ReleaseDC(gsurf->hWnd, hDC);

   resource_surface_swap_buffers(gsurf->rsurf,
         NATIVE_ATTACHMENT_FRONT_LEFT, NATIVE_ATTACHMENT_BACK_LEFT, TRUE);
   /* the front/back buffers have been swapped */
   gdi_surface_invalidate(&gsurf->base);

   return ret;
}

static boolean
gdi_surface_present(struct native_surface *nsurf,
                    const struct native_present_control *ctrl)
{
   boolean ret;

   if (ctrl->preserve || ctrl->swap_interval)
      return FALSE;

   switch (ctrl->natt) {
   case NATIVE_ATTACHMENT_FRONT_LEFT:
      ret = gdi_surface_flush_frontbuffer(nsurf);
      break;
   case NATIVE_ATTACHMENT_BACK_LEFT:
      ret = gdi_surface_swap_buffers(nsurf);
      break;
   default:
      ret = FALSE;
      break;
   }

   return ret;
}

static boolean
gdi_surface_validate(struct native_surface *nsurf, uint attachment_mask,
                        unsigned int *seq_num, struct pipe_resource **textures,
                        int *width, int *height)
{
   struct gdi_surface *gsurf = gdi_surface(nsurf);
   uint w, h;

   if (!gdi_surface_update_buffers(&gsurf->base, attachment_mask))
      return FALSE;

   if (seq_num)
      *seq_num = gsurf->client_stamp;

   if (textures)
      resource_surface_get_resources(gsurf->rsurf, textures, attachment_mask);

   resource_surface_get_size(gsurf->rsurf, &w, &h);
   if (width)
      *width = w;
   if (height)
      *height = h;

   return TRUE;
}

static void
gdi_surface_wait(struct native_surface *nsurf)
{
   /* no-op */
}

static void
gdi_surface_destroy(struct native_surface *nsurf)
{
   struct gdi_surface *gsurf = gdi_surface(nsurf);

   resource_surface_destroy(gsurf->rsurf);
   FREE(gsurf);
}

static struct native_surface *
gdi_display_create_window_surface(struct native_display *ndpy,
                                  EGLNativeWindowType win,
                                  const struct native_config *nconf)
{
   struct gdi_display *gdpy = gdi_display(ndpy);
   struct gdi_surface *gsurf;

   gsurf = CALLOC_STRUCT(gdi_surface);
   if (!gsurf)
      return NULL;

   gsurf->gdpy = gdpy;
   gsurf->color_format = nconf->color_format;
   gsurf->hWnd = (HWND) win;

   gsurf->rsurf = resource_surface_create(gdpy->base.screen,
         gsurf->color_format,
         PIPE_BIND_RENDER_TARGET |
         PIPE_BIND_SAMPLER_VIEW |
         PIPE_BIND_DISPLAY_TARGET |
         PIPE_BIND_SCANOUT);
   if (!gsurf->rsurf) {
      FREE(gsurf);
      return NULL;
   }

   /* initialize the geometry */
   gdi_surface_update_geometry(&gsurf->base);

   gsurf->base.destroy = gdi_surface_destroy;
   gsurf->base.present = gdi_surface_present;
   gsurf->base.validate = gdi_surface_validate;
   gsurf->base.wait = gdi_surface_wait;

   return &gsurf->base;
}

static int
fill_color_formats(struct native_display *ndpy, enum pipe_format formats[8])
{
   struct pipe_screen *screen = ndpy->screen;
   int i, count = 0;

   enum pipe_format candidates[] = {
      /* 32-bit */
      PIPE_FORMAT_B8G8R8A8_UNORM,
      PIPE_FORMAT_A8R8G8B8_UNORM,
      /* 24-bit */
      PIPE_FORMAT_B8G8R8X8_UNORM,
      PIPE_FORMAT_X8R8G8B8_UNORM,
      /* 16-bit */
      PIPE_FORMAT_B5G6R5_UNORM
   };

   assert(Elements(candidates) <= 8);

   for (i = 0; i < Elements(candidates); i++) {
      if (screen->is_format_supported(screen, candidates[i],
               PIPE_TEXTURE_2D, 0, PIPE_BIND_RENDER_TARGET))
         formats[count++] = candidates[i];
   }

   return count;
}

static const struct native_config **
gdi_display_get_configs(struct native_display *ndpy, int *num_configs)
{
   struct gdi_display *gdpy = gdi_display(ndpy);
   const struct native_config **configs;
   int i;

   /* first time */
   if (!gdpy->configs) {
      enum pipe_format formats[8];
      int i, count;

      count = fill_color_formats(&gdpy->base, formats);

      gdpy->configs = CALLOC(count, sizeof(*gdpy->configs));
      if (!gdpy->configs)
         return NULL;

      for (i = 0; i < count; i++) {
         struct native_config *nconf = &gdpy->configs[i];

         nconf->buffer_mask =
            (1 << NATIVE_ATTACHMENT_FRONT_LEFT) |
            (1 << NATIVE_ATTACHMENT_BACK_LEFT);
         nconf->color_format = formats[i];

         nconf->window_bit = TRUE;
      }

      gdpy->num_configs = count;
   }

   configs = MALLOC(gdpy->num_configs * sizeof(*configs));
   if (configs) {
      for (i = 0; i < gdpy->num_configs; i++)
         configs[i] = (const struct native_config *) &gdpy->configs[i];
      if (num_configs)
         *num_configs = gdpy->num_configs;
   }
   return configs;
}

static int
gdi_display_get_param(struct native_display *ndpy,
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
gdi_display_destroy(struct native_display *ndpy)
{
   struct gdi_display *gdpy = gdi_display(ndpy);

   if (gdpy->configs)
      FREE(gdpy->configs);

   ndpy_uninit(ndpy);

   FREE(gdpy);
}

static boolean
gdi_display_init_screen(struct native_display *ndpy)
{
   struct gdi_display *gdpy = gdi_display(ndpy);
   struct sw_winsys *winsys;

   winsys = gdi_create_sw_winsys();
   if (!winsys)
      return FALSE;

   gdpy->base.screen = gdpy->event_handler->new_sw_screen(&gdpy->base, winsys);
   if (!gdpy->base.screen) {
      if (winsys->destroy)
         winsys->destroy(winsys);
      return FALSE;
   }

   return TRUE;
}

static struct native_display *
gdi_create_display(HDC hDC, const struct native_event_handler *event_handler)
{
   struct gdi_display *gdpy;

   gdpy = CALLOC_STRUCT(gdi_display);
   if (!gdpy)
      return NULL;

   gdpy->hDC = hDC;
   gdpy->event_handler = event_handler;

   gdpy->base.init_screen = gdi_display_init_screen;
   gdpy->base.destroy = gdi_display_destroy;
   gdpy->base.get_param = gdi_display_get_param;

   gdpy->base.get_configs = gdi_display_get_configs;
   gdpy->base.create_window_surface = gdi_display_create_window_surface;

   return &gdpy->base;
}

static const struct native_event_handler *gdi_event_handler;

static struct native_display *
native_create_display(void *dpy, boolean use_sw)
{
   return gdi_create_display((HDC) dpy, gdi_event_handler);
}

static const struct native_platform gdi_platform = {
   "GDI", /* name */
   native_create_display
};

const struct native_platform *
native_get_gdi_platform(const struct native_event_handler *event_handler)
{
   gdi_event_handler = event_handler;
   return &gdi_platform;
}
