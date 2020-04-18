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
#include "state_tracker/drm_driver.h"

#include "egllog.h"
#include <errno.h>

#include "native_wayland.h"

#include <wayland-client.h>
#include "wayland-drm-client-protocol.h"
#include "wayland-egl-priv.h"

#include "common/native_wayland_drm_bufmgr_helper.h"

#include <xf86drm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct wayland_drm_display {
   struct wayland_display base;

   const struct native_event_handler *event_handler;

   struct wl_drm *wl_drm;
   struct wl_drm *wl_server_drm; /* for EGL_WL_bind_wayland_display */
   int fd;
   char *device_name;
   boolean authenticated;
};

static INLINE struct wayland_drm_display *
wayland_drm_display(const struct native_display *ndpy)
{
   return (struct wayland_drm_display *) ndpy;
}

static void 
wayland_drm_display_destroy(struct native_display *ndpy)
{
   struct wayland_drm_display *drmdpy = wayland_drm_display(ndpy);

   if (drmdpy->wl_drm)
      wl_drm_destroy(drmdpy->wl_drm);
   if (drmdpy->device_name)
      FREE(drmdpy->device_name);
   if (drmdpy->base.configs)
      FREE(drmdpy->base.configs);
   if (drmdpy->base.own_dpy)
      wl_display_disconnect(drmdpy->base.dpy);

   ndpy_uninit(ndpy);

   if (drmdpy->fd)
      close(drmdpy->fd);

   FREE(drmdpy);
}

static struct wl_buffer *
wayland_create_drm_buffer(struct wayland_display *display,
                          struct wayland_surface *surface,
                          enum native_attachment attachment)
{
   struct wayland_drm_display *drmdpy = (struct wayland_drm_display *) display;
   struct pipe_screen *screen = drmdpy->base.base.screen;
   struct pipe_resource *resource;
   struct winsys_handle wsh;
   uint width, height;
   enum wl_drm_format format;

   resource = resource_surface_get_single_resource(surface->rsurf, attachment);
   resource_surface_get_size(surface->rsurf, &width, &height);

   wsh.type = DRM_API_HANDLE_TYPE_SHARED;
   screen->resource_get_handle(screen, resource, &wsh);

   pipe_resource_reference(&resource, NULL);

   switch (surface->color_format) {
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      format = WL_DRM_FORMAT_ARGB8888;
      break;
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      format = WL_DRM_FORMAT_XRGB8888;
      break;
   default:
      return NULL;
      break;
   }

   return wl_drm_create_buffer(drmdpy->wl_drm, wsh.handle,
                               width, height, wsh.stride, format);
}

static void
drm_handle_device(void *data, struct wl_drm *drm, const char *device)
{
   struct wayland_drm_display *drmdpy = data;
   drm_magic_t magic;

   drmdpy->device_name = strdup(device);
   if (!drmdpy->device_name)
      return;

#ifdef O_CLOEXEC
   drmdpy->fd = open(drmdpy->device_name, O_RDWR | O_CLOEXEC);
   if (drmdpy->fd == -1 && errno == EINVAL)
#endif
   {
      drmdpy->fd = open(drmdpy->device_name, O_RDWR);
      if (drmdpy->fd != -1)
         fcntl(drmdpy->fd, F_SETFD, fcntl(drmdpy->fd, F_GETFD) | FD_CLOEXEC);
   }
   if (drmdpy->fd == -1) {
      _eglLog(_EGL_WARNING, "wayland-egl: could not open %s (%s)",
              drmdpy->device_name, strerror(errno));
      return;
   }

   drmGetMagic(drmdpy->fd, &magic);
   wl_drm_authenticate(drmdpy->wl_drm, magic);
}

static void
drm_handle_format(void *data, struct wl_drm *drm, uint32_t format)
{
   struct wayland_drm_display *drmdpy = data;

   switch (format) {
   case WL_DRM_FORMAT_ARGB8888:
      drmdpy->base.formats |= HAS_ARGB8888;
      break;
   case WL_DRM_FORMAT_XRGB8888:
      drmdpy->base.formats |= HAS_XRGB8888;
      break;
   }
}

static void
drm_handle_authenticated(void *data, struct wl_drm *drm)
{
   struct wayland_drm_display *drmdpy = data;

   drmdpy->authenticated = true;
}

static const struct wl_drm_listener drm_listener = {
   drm_handle_device,
   drm_handle_format,
   drm_handle_authenticated
};

static void
registry_handle_global(void *data, struct wl_registry *registry, uint32_t name,
                       const char *interface, uint32_t version)
{
   struct wayland_drm_display *drmdpy = data;

   if (strcmp(interface, "wl_drm") == 0) {
      drmdpy->wl_drm = wl_registry_bind(registry, name, &wl_drm_interface, 1);
      wl_drm_add_listener(drmdpy->wl_drm, &drm_listener, drmdpy);
   }
}

static const struct wl_registry_listener registry_listener = {
       registry_handle_global
};

static boolean
wayland_drm_display_init_screen(struct native_display *ndpy)
{
   struct wayland_drm_display *drmdpy = wayland_drm_display(ndpy);

   drmdpy->base.queue = wl_display_create_queue(drmdpy->base.dpy);
   drmdpy->base.registry = wl_display_get_registry(drmdpy->base.dpy);
   wl_proxy_set_queue((struct wl_proxy *) drmdpy->base.registry,
                      drmdpy->base.queue);
   wl_registry_add_listener(drmdpy->base.registry, &registry_listener, drmdpy);
   if (wayland_roundtrip(&drmdpy->base) < 0 || drmdpy->wl_drm == NULL)
      return FALSE;

   wl_drm_add_listener(drmdpy->wl_drm, &drm_listener, drmdpy);
   if (wayland_roundtrip(&drmdpy->base) < 0 || drmdpy->fd == -1)
      return FALSE;

   if (wayland_roundtrip(&drmdpy->base) < 0 || !drmdpy->authenticated)
      return FALSE;

   if (drmdpy->base.formats == 0)
      return FALSE;

   drmdpy->base.base.screen =
      drmdpy->event_handler->new_drm_screen(&drmdpy->base.base,
                                            NULL, drmdpy->fd);
   if (!drmdpy->base.base.screen) {
      _eglLog(_EGL_WARNING, "failed to create DRM screen");
      return FALSE;
   }

   return TRUE;
}

static struct native_display_buffer wayland_drm_display_buffer = {
   /* use the helpers */
   drm_display_import_native_buffer,
   drm_display_export_native_buffer
};

static int
wayland_drm_display_authenticate(void *user_data, uint32_t magic)
{
   struct native_display *ndpy = user_data;
   struct wayland_drm_display *drmdpy = wayland_drm_display(ndpy);
   boolean current_authenticate, authenticated;

   current_authenticate = drmdpy->authenticated;

   wl_drm_authenticate(drmdpy->wl_drm, magic);
   wl_display_roundtrip(drmdpy->base.dpy);
   authenticated = drmdpy->authenticated;

   drmdpy->authenticated = current_authenticate;

   return authenticated ? 0 : -1;
}

static struct wayland_drm_callbacks wl_drm_callbacks = {
   wayland_drm_display_authenticate,
   egl_g3d_wl_drm_helper_reference_buffer,
   egl_g3d_wl_drm_helper_unreference_buffer
};

static boolean
wayland_drm_display_bind_wayland_display(struct native_display *ndpy,
                                         struct wl_display *wl_dpy)
{
   struct wayland_drm_display *drmdpy = wayland_drm_display(ndpy);

   if (drmdpy->wl_server_drm)
      return FALSE;

   drmdpy->wl_server_drm =
      wayland_drm_init(wl_dpy, drmdpy->device_name,
                       &wl_drm_callbacks, ndpy);

   if (!drmdpy->wl_server_drm)
      return FALSE;
   
   return TRUE;
}

static boolean
wayland_drm_display_unbind_wayland_display(struct native_display *ndpy,
                                           struct wl_display *wl_dpy)
{
   struct wayland_drm_display *drmdpy = wayland_drm_display(ndpy);

   if (!drmdpy->wl_server_drm)
      return FALSE;

   wayland_drm_uninit(drmdpy->wl_server_drm);
   drmdpy->wl_server_drm = NULL;

   return TRUE;
}

static struct native_display_wayland_bufmgr wayland_drm_display_wayland_bufmgr = {
   wayland_drm_display_bind_wayland_display,
   wayland_drm_display_unbind_wayland_display,
   egl_g3d_wl_drm_common_wl_buffer_get_resource,
   egl_g3d_wl_drm_common_query_buffer
};


struct wayland_display *
wayland_create_drm_display(struct wl_display *dpy,
                           const struct native_event_handler *event_handler)
{
   struct wayland_drm_display *drmdpy;

   drmdpy = CALLOC_STRUCT(wayland_drm_display);
   if (!drmdpy)
      return NULL;

   drmdpy->event_handler = event_handler;

   drmdpy->base.dpy = dpy;
   if (!drmdpy->base.dpy) {
      wayland_drm_display_destroy(&drmdpy->base.base);
      return NULL;
   }

   drmdpy->base.base.init_screen = wayland_drm_display_init_screen;
   drmdpy->base.base.destroy = wayland_drm_display_destroy;
   drmdpy->base.base.buffer = &wayland_drm_display_buffer;
   drmdpy->base.base.wayland_bufmgr = &wayland_drm_display_wayland_bufmgr;

   drmdpy->base.create_buffer = wayland_create_drm_buffer;

   return &drmdpy->base;
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
