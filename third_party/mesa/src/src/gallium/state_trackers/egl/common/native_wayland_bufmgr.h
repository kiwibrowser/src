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

#ifndef _NATIVE_WAYLAND_BUFMGR_H_
#define _NATIVE_WAYLAND_BUFMGR_H_

struct native_display;
struct wl_display;
struct wl_buffer;
struct pipe_resource;

struct native_display_wayland_bufmgr {
   boolean (*bind_display)(struct native_display *ndpy,
                           struct wl_display *wl_dpy);

   boolean (*unbind_display)(struct native_display *ndpy,
                             struct wl_display *wl_dpy);

   struct pipe_resource *(*buffer_get_resource)(struct native_display *ndpy,
                                                struct wl_buffer *buffer);
                             

   boolean (*query_buffer)(struct native_display *ndpy,
                           struct wl_buffer *buffer,
                           int attribute, int *value);
};

#endif /* _NATIVE_WAYLAND_BUFMGR_H_ */
