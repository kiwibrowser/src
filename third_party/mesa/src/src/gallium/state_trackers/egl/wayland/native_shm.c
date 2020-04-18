/*
 * Mesa 3-D graphics library
 * Version:  7.11
 *
 * Copyright (C) 2011 Benjamin Franzke <benjaminfranzke@googlemail.com>
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
#include "util/u_inlines.h"

#include "pipe/p_compiler.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "sw/wayland/wayland_sw_winsys.h"

#include "egllog.h"

#include "native_wayland.h"

#include <wayland-client.h>
#include "wayland-egl-priv.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct wayland_shm_display {
   struct wayland_display base;

   const struct native_event_handler *event_handler;
   struct wl_shm *wl_shm;
};

static INLINE struct wayland_shm_display *
wayland_shm_display(const struct native_display *ndpy)
{
   return (struct wayland_shm_display *) ndpy;
}


static void 
wayland_shm_display_destroy(struct native_display *ndpy)
{
   struct wayland_shm_display *shmdpy = wayland_shm_display(ndpy);

   if (shmdpy->base.configs)
      FREE(shmdpy->base.configs);
   if (shmdpy->base.own_dpy)
      wl_display_disconnect(shmdpy->base.dpy);

   ndpy_uninit(ndpy);

   FREE(shmdpy);
}

static struct wl_buffer *
wayland_create_shm_buffer(struct wayland_display *display,
                          struct wayland_surface *surface,
                          enum native_attachment attachment)
{
   struct wayland_shm_display *shmdpy = (struct wayland_shm_display *) display;
   struct pipe_screen *screen = shmdpy->base.base.screen;
   struct pipe_resource *resource;
   struct winsys_handle wsh;
   uint width, height;
   enum wl_shm_format format;
   struct wl_buffer *buffer;
   struct wl_shm_pool *pool;

   resource = resource_surface_get_single_resource(surface->rsurf, attachment);
   resource_surface_get_size(surface->rsurf, &width, &height);

   screen->resource_get_handle(screen, resource, &wsh);

   pipe_resource_reference(&resource, NULL);

   switch (surface->color_format) {
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      format = WL_SHM_FORMAT_ARGB8888;
      break;
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      format = WL_SHM_FORMAT_XRGB8888;
      break;
   default:
      return NULL;
      break;
   }

   pool = wl_shm_create_pool(shmdpy->wl_shm, wsh.fd, wsh.size);
   buffer = wl_shm_pool_create_buffer(pool, 0, width, height,
                                      wsh.stride, format);
   wl_shm_pool_destroy(pool);

   return buffer;
}

static void
shm_handle_format(void *data, struct wl_shm *shm, uint32_t format)
{
   struct wayland_shm_display *shmdpy = data;

   switch (format) {
   case WL_SHM_FORMAT_ARGB8888:
      shmdpy->base.formats |= HAS_ARGB8888;
      break;
   case WL_SHM_FORMAT_XRGB8888:
      shmdpy->base.formats |= HAS_XRGB8888;
      break;
   }
}

static const struct wl_shm_listener shm_listener = {
   shm_handle_format
};

static void
registry_handle_global(void *data, struct wl_registry *registry, uint32_t name,
                       const char *interface, uint32_t version)
{
   struct wayland_shm_display *shmdpy = data;

   if (strcmp(interface, "wl_shm") == 0) {
      shmdpy->wl_shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
      wl_shm_add_listener(shmdpy->wl_shm, &shm_listener, shmdpy);
   }
}

static const struct wl_registry_listener registry_listener = {
       registry_handle_global
};

static boolean
wayland_shm_display_init_screen(struct native_display *ndpy)
{
   struct wayland_shm_display *shmdpy = wayland_shm_display(ndpy);
   struct sw_winsys *winsys = NULL;

   shmdpy->base.queue = wl_display_create_queue(shmdpy->base.dpy);
   shmdpy->base.registry = wl_display_get_registry(shmdpy->base.dpy);
   wl_proxy_set_queue((struct wl_proxy *) shmdpy->base.registry,
                      shmdpy->base.queue);
   wl_registry_add_listener(shmdpy->base.registry, &registry_listener, shmdpy);
   if (wayland_roundtrip(&shmdpy->base) < 0 || shmdpy->wl_shm == NULL)
      return FALSE;

   if (shmdpy->base.formats == 0)
      wl_display_roundtrip(shmdpy->base.dpy);
   if (shmdpy->base.formats == 0)
      return FALSE;

   winsys = wayland_create_sw_winsys(shmdpy->base.dpy);
   if (!winsys)
      return FALSE;

   shmdpy->base.base.screen =
      shmdpy->event_handler->new_sw_screen(&shmdpy->base.base, winsys);

   if (!shmdpy->base.base.screen) {
      _eglLog(_EGL_WARNING, "failed to create shm screen");
      return FALSE;
   }

   return TRUE;
}

struct wayland_display *
wayland_create_shm_display(struct wl_display *dpy,
                           const struct native_event_handler *event_handler)
{
   struct wayland_shm_display *shmdpy;

   shmdpy = CALLOC_STRUCT(wayland_shm_display);
   if (!shmdpy)
      return NULL;

   shmdpy->event_handler = event_handler;

   shmdpy->base.dpy = dpy;
   if (!shmdpy->base.dpy) {
      wayland_shm_display_destroy(&shmdpy->base.base);
      return NULL;
   }

   shmdpy->base.base.init_screen = wayland_shm_display_init_screen;
   shmdpy->base.base.destroy = wayland_shm_display_destroy;
   shmdpy->base.create_buffer = wayland_create_shm_buffer;

   return &shmdpy->base;
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
