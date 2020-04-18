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

#ifndef _NATIVE_WAYLAND_H_
#define _NATIVE_WAYLAND_H_

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"

#include "common/native.h"
#include "common/native_helper.h"

#include "wayland-egl-priv.h"

struct wayland_surface;

enum wayland_format_flag {
   HAS_ARGB8888        = (1 << 0),
   HAS_XRGB8888        = (1 << 1)
};

struct wayland_display {
   struct native_display base;

   struct wl_display *dpy;
   struct wl_event_queue *queue;
   struct wl_registry *registry;
   boolean own_dpy;
   /* supported formats */
   uint32_t formats;

   struct wayland_config *configs;
   int num_configs;

   struct wl_buffer *(*create_buffer)(struct wayland_display *display,
                                      struct wayland_surface *surface,
                                      enum native_attachment attachment);
};

enum wayland_buffer_type {
   WL_BUFFER_FRONT,
   WL_BUFFER_BACK,
   WL_BUFFER_COUNT
};

enum wayland_surface_type {
   WL_WINDOW_SURFACE,
   WL_PBUFFER_SURFACE
};

struct wayland_surface {
   struct native_surface base;
   struct wayland_display *display;

   struct wl_egl_window *win;
   enum wayland_surface_type type;
   int dx, dy;
   struct resource_surface *rsurf;
   struct pipe_resource *pending_resource;
   enum pipe_format color_format;

   unsigned int sequence_number;
   struct wl_buffer *buffer[WL_BUFFER_COUNT];
   unsigned int attachment_mask;

   struct wl_callback *frame_callback;
   boolean premultiplied_alpha;
};

struct wayland_config {
   struct native_config base;
};

static INLINE struct wayland_display *
wayland_display(const struct native_display *ndpy)
{
   return (struct wayland_display *) ndpy;
}

static INLINE struct wayland_surface *
wayland_surface(const struct native_surface *nsurf)
{
   return (struct wayland_surface *) nsurf;
}

static INLINE struct wayland_config *
wayland_config(const struct native_config *nconf)
{
   return (struct wayland_config *) nconf;
}

struct wayland_display *
wayland_create_shm_display(struct wl_display *display,
                           const struct native_event_handler *event_handler);

struct wayland_display *
wayland_create_drm_display(struct wl_display *display,
                           const struct native_event_handler *event_handler);

int
wayland_roundtrip(struct wayland_display *drmdpy);

#endif /* _NATIVE_WAYLAND_H_ */
