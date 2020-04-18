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

#include "native_wayland.h"

static void
sync_callback(void *data, struct wl_callback *callback, uint32_t serial)
{
   int *done = data;

   *done = 1;
   wl_callback_destroy(callback);
}

static const struct wl_callback_listener sync_listener = {
   sync_callback
};

int
wayland_roundtrip(struct wayland_display *display)
{
   struct wl_callback *callback;
   int done = 0, ret = 0;

   callback = wl_display_sync(display->dpy);
   wl_callback_add_listener(callback, &sync_listener, &done);
   wl_proxy_set_queue((struct wl_proxy *) callback, display->queue);
   while (ret != -1 && !done)
      ret = wl_display_dispatch_queue(display->dpy, display->queue);

   if (!done)
      wl_callback_destroy(callback);

   return ret;
}

static const struct native_event_handler *wayland_event_handler;

const static struct {
   enum pipe_format format;
   enum wayland_format_flag flag;
} wayland_formats[] = {
   { PIPE_FORMAT_B8G8R8A8_UNORM, HAS_ARGB8888 },
   { PIPE_FORMAT_B8G8R8X8_UNORM, HAS_XRGB8888 },
};

static const struct native_config **
wayland_display_get_configs(struct native_display *ndpy, int *num_configs)
{
   struct wayland_display *display = wayland_display(ndpy);
   const struct native_config **configs;
   int i;

   if (!display->configs) {
      struct native_config *nconf;

      display->num_configs = 0;
      display->configs = CALLOC(Elements(wayland_formats),
                                sizeof(*display->configs));
      if (!display->configs)
         return NULL;

      for (i = 0; i < Elements(wayland_formats); ++i) {
         if (!(display->formats & wayland_formats[i].flag))
            continue;

         nconf = &display->configs[display->num_configs].base;
         nconf->buffer_mask =
            (1 << NATIVE_ATTACHMENT_FRONT_LEFT) |
            (1 << NATIVE_ATTACHMENT_BACK_LEFT);
         
         nconf->window_bit = TRUE;
         
         nconf->color_format = wayland_formats[i].format;
         display->num_configs++;
      }
   }

   configs = MALLOC(display->num_configs * sizeof(*configs));
   if (configs) {
      for (i = 0; i < display->num_configs; ++i)
         configs[i] = &display->configs[i].base;
      if (num_configs)
         *num_configs = display->num_configs;
   }

   return configs;
}

static int
wayland_display_get_param(struct native_display *ndpy,
                          enum native_param_type param)
{
   int val;

   switch (param) {
   case NATIVE_PARAM_PREMULTIPLIED_ALPHA:
      val = 1;
      break;
   case NATIVE_PARAM_USE_NATIVE_BUFFER:
   case NATIVE_PARAM_PRESERVE_BUFFER:
   case NATIVE_PARAM_MAX_SWAP_INTERVAL:
   default:
      val = 0;
      break;
   }

   return val;
}

static void
wayland_release_pending_resource(void *data,
                                 struct wl_callback *callback,
                                 uint32_t time)
{
   struct wayland_surface *surface = data;

   wl_callback_destroy(callback);

   /* FIXME: print internal error */
   if (!surface->pending_resource)
      return;

   pipe_resource_reference(&surface->pending_resource, NULL);
}

static const struct wl_callback_listener release_buffer_listener = {
   wayland_release_pending_resource
};

static void
wayland_window_surface_handle_resize(struct wayland_surface *surface)
{
   struct wayland_display *display = surface->display;
   struct pipe_resource *front_resource;
   const enum native_attachment front_natt = NATIVE_ATTACHMENT_FRONT_LEFT;
   int i;

   front_resource = resource_surface_get_single_resource(surface->rsurf,
                                                         front_natt);
   if (resource_surface_set_size(surface->rsurf,
                                 surface->win->width, surface->win->height)) {

      if (surface->pending_resource)
         wayland_roundtrip(display);

      if (front_resource) {
         struct wl_callback *callback;

         surface->pending_resource = front_resource;
         front_resource = NULL;

         callback = wl_display_sync(display->dpy);
         wl_callback_add_listener(callback, &release_buffer_listener, surface);
         wl_proxy_set_queue((struct wl_proxy *) callback, display->queue);
      }

      for (i = 0; i < WL_BUFFER_COUNT; ++i) {
         if (surface->buffer[i])
            wl_buffer_destroy(surface->buffer[i]);
         surface->buffer[i] = NULL;
      }

      surface->dx = surface->win->dx;
      surface->dy = surface->win->dy;
   }
   pipe_resource_reference(&front_resource, NULL);
}

static boolean
wayland_surface_validate(struct native_surface *nsurf, uint attachment_mask,
                         unsigned int *seq_num, struct pipe_resource **textures,
                         int *width, int *height)
{
   struct wayland_surface *surface = wayland_surface(nsurf);

   if (surface->type == WL_WINDOW_SURFACE)
      wayland_window_surface_handle_resize(surface);

   if (!resource_surface_add_resources(surface->rsurf, attachment_mask |
                                       surface->attachment_mask))
      return FALSE;

   if (textures)
      resource_surface_get_resources(surface->rsurf, textures, attachment_mask);

   if (seq_num)
      *seq_num = surface->sequence_number;

   resource_surface_get_size(surface->rsurf, (uint *) width, (uint *) height);

   return TRUE;
}

static void
wayland_frame_callback(void *data, struct wl_callback *callback, uint32_t time)
{
   struct wayland_surface *surface = data;

   surface->frame_callback = NULL;

   wl_callback_destroy(callback);
}

static const struct wl_callback_listener frame_listener = {
   wayland_frame_callback
};

static INLINE void
wayland_buffers_swap(struct wl_buffer **buffer,
                     enum wayland_buffer_type buf1,
                     enum wayland_buffer_type buf2)
{
   struct wl_buffer *tmp = buffer[buf1];
   buffer[buf1] = buffer[buf2];
   buffer[buf2] = tmp;
}

static boolean
wayland_surface_swap_buffers(struct native_surface *nsurf)
{
   struct wayland_surface *surface = wayland_surface(nsurf);
   struct wayland_display *display = surface->display;
   int ret = 0;

   while (surface->frame_callback && ret != -1)
      ret = wl_display_dispatch_queue(display->dpy, display->queue);
   if (ret == -1)
      return EGL_FALSE;

   surface->frame_callback = wl_surface_frame(surface->win->surface);
   wl_callback_add_listener(surface->frame_callback, &frame_listener, surface);
   wl_proxy_set_queue((struct wl_proxy *) surface->frame_callback,
                      display->queue);

   if (surface->type == WL_WINDOW_SURFACE) {
      resource_surface_swap_buffers(surface->rsurf,
                                    NATIVE_ATTACHMENT_FRONT_LEFT,
                                    NATIVE_ATTACHMENT_BACK_LEFT, FALSE);

      wayland_buffers_swap(surface->buffer, WL_BUFFER_FRONT, WL_BUFFER_BACK);

      if (surface->buffer[WL_BUFFER_FRONT] == NULL)
         surface->buffer[WL_BUFFER_FRONT] =
            display->create_buffer(display, surface,
                                   NATIVE_ATTACHMENT_FRONT_LEFT);

      wl_surface_attach(surface->win->surface, surface->buffer[WL_BUFFER_FRONT],
                        surface->dx, surface->dy);

      resource_surface_get_size(surface->rsurf,
                                (uint *) &surface->win->attached_width,
                                (uint *) &surface->win->attached_height);
      surface->dx = 0;
      surface->dy = 0;
   }

   surface->sequence_number++;
   wayland_event_handler->invalid_surface(&display->base,
                                          &surface->base,
                                          surface->sequence_number);

   return TRUE;
}

static boolean
wayland_surface_present(struct native_surface *nsurf,
                        const struct native_present_control *ctrl)
{
   struct wayland_surface *surface = wayland_surface(nsurf);
   uint width, height;
   boolean ret;

   if (ctrl->preserve || ctrl->swap_interval)
      return FALSE;

   /* force buffers to be re-created if they will be presented differently */
   if (surface->premultiplied_alpha != ctrl->premultiplied_alpha) {
      enum wayland_buffer_type buffer;

      for (buffer = 0; buffer < WL_BUFFER_COUNT; ++buffer) {
         if (surface->buffer[buffer]) {
            wl_buffer_destroy(surface->buffer[buffer]);
            surface->buffer[buffer] = NULL;
         }
      }

      surface->premultiplied_alpha = ctrl->premultiplied_alpha;
   }

   switch (ctrl->natt) {
   case NATIVE_ATTACHMENT_FRONT_LEFT:
      ret = TRUE;
      break;
   case NATIVE_ATTACHMENT_BACK_LEFT:
      ret = wayland_surface_swap_buffers(nsurf);
      break;
   default:
      ret = FALSE;
      break;
   }

   if (surface->type == WL_WINDOW_SURFACE) {
      resource_surface_get_size(surface->rsurf, &width, &height);
      wl_surface_damage(surface->win->surface, 0, 0, width, height);
      wl_surface_commit(surface->win->surface);
   }

   return ret;
}

static void
wayland_surface_wait(struct native_surface *nsurf)
{
   /* no-op */
}

static void
wayland_surface_destroy(struct native_surface *nsurf)
{
   struct wayland_surface *surface = wayland_surface(nsurf);
   enum wayland_buffer_type buffer;

   for (buffer = 0; buffer < WL_BUFFER_COUNT; ++buffer) {
      if (surface->buffer[buffer])
         wl_buffer_destroy(surface->buffer[buffer]);
   }

   if (surface->frame_callback)
      wl_callback_destroy(surface->frame_callback);

   resource_surface_destroy(surface->rsurf);
   FREE(surface);
}


static struct native_surface *
wayland_create_window_surface(struct native_display *ndpy,
                              EGLNativeWindowType win,
                              const struct native_config *nconf)
{
   struct wayland_display *display = wayland_display(ndpy);
   struct wayland_config *config = wayland_config(nconf);
   struct wayland_surface *surface;
   uint bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW |
      PIPE_BIND_DISPLAY_TARGET | PIPE_BIND_SCANOUT;

   surface = CALLOC_STRUCT(wayland_surface);
   if (!surface)
      return NULL;

   surface->display = display;
   surface->color_format = config->base.color_format;

   surface->win = (struct wl_egl_window *) win;

   surface->pending_resource = NULL;
   surface->frame_callback = NULL;
   surface->type = WL_WINDOW_SURFACE;

   surface->buffer[WL_BUFFER_FRONT] = NULL;
   surface->buffer[WL_BUFFER_BACK] = NULL;
   surface->attachment_mask = (1 << NATIVE_ATTACHMENT_FRONT_LEFT) |
      (1 << NATIVE_ATTACHMENT_BACK_LEFT);

   surface->rsurf = resource_surface_create(display->base.screen,
                                            surface->color_format, bind);

   if (!surface->rsurf) {
      FREE(surface);
      return NULL;
   }

   surface->base.destroy = wayland_surface_destroy;
   surface->base.present = wayland_surface_present;
   surface->base.validate = wayland_surface_validate;
   surface->base.wait = wayland_surface_wait;

   return &surface->base;
}

static struct native_display *
native_create_display(void *dpy, boolean use_sw)
{
   struct wayland_display *display = NULL;
   boolean own_dpy = FALSE;

   use_sw = use_sw || debug_get_bool_option("EGL_SOFTWARE", FALSE);

   if (dpy == NULL) {
      dpy = wl_display_connect(NULL);
      if (dpy == NULL)
         return NULL;
      own_dpy = TRUE;
   }

   if (use_sw) {
      _eglLog(_EGL_INFO, "use software fallback");
      display = wayland_create_shm_display((struct wl_display *) dpy,
                                           wayland_event_handler);
   } else {
      display = wayland_create_drm_display((struct wl_display *) dpy,
                                           wayland_event_handler);
   }

   if (!display)
      return NULL;

   display->base.get_param = wayland_display_get_param;
   display->base.get_configs = wayland_display_get_configs;
   display->base.create_window_surface = wayland_create_window_surface;

   display->own_dpy = own_dpy;

   return &display->base;
}

static const struct native_platform wayland_platform = {
   "wayland", /* name */
   native_create_display
};

const struct native_platform *
native_get_wayland_platform(const struct native_event_handler *event_handler)
{
   wayland_event_handler = event_handler;
   return &wayland_platform;
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
