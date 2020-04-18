/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
 * Copyright (C) 2011 VMware Inc. All rights reserved.
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
 *    Thomas Hellstrom <thellstrom@vmware.com>
 */

#include "native.h"

struct resource_surface;
struct sw_winsys;

struct resource_surface *
resource_surface_create(struct pipe_screen *screen,
                        enum pipe_format format, uint bind);

void
resource_surface_destroy(struct resource_surface *rsurf);

boolean
resource_surface_set_size(struct resource_surface *rsurf,
                          uint width, uint height);

void
resource_surface_get_size(struct resource_surface *rsurf,
                          uint *width, uint *height);

boolean
resource_surface_add_resources(struct resource_surface *rsurf,
                               uint resource_mask);

void
resource_surface_import_resource(struct resource_surface *rsurf,
                                 enum native_attachment which,
                                 struct pipe_resource *pres);

void
resource_surface_get_resources(struct resource_surface *rsurf,
                               struct pipe_resource **resources,
                               uint resource_mask);

struct pipe_resource *
resource_surface_get_single_resource(struct resource_surface *rsurf,
                                     enum native_attachment which);

void
resource_surface_swap_buffers(struct resource_surface *rsurf,
                              enum native_attachment buf1,
                              enum native_attachment buf2,
                              boolean only_if_exist);

boolean
resource_surface_present(struct resource_surface *rsurf,
                         enum native_attachment which,
                         void *winsys_drawable_handle);

/**
 * Perform a gallium copy blit between the back left and front left
 * surfaces. Needs to be followed by a call to resource_surface_flush.
 */
boolean
resource_surface_copy_swap(struct resource_surface *rsurf,
			   struct native_display *ndpy);

/**
 * Throttle on outstanding rendering using the copy context. For example
 * copy swaps.
 */
boolean
resource_surface_throttle(struct resource_surface *rsurf);

/**
 * Flush pending rendering using the copy context. This function saves a
 * marker for upcoming throttles.
 */
boolean
resource_surface_flush(struct resource_surface *rsurf,
		       struct native_display *ndpy);
/**
 * Wait for all rendering using the copy context to be complete. Frees all
 * throttle markers saved using resource_surface_flush.
 */
void
resource_surface_wait(struct resource_surface *rsurf);

boolean
native_display_copy_to_pixmap(struct native_display *ndpy,
                              EGLNativePixmapType pix,
                              struct pipe_resource *src);

struct pipe_resource *
drm_display_import_native_buffer(struct native_display *ndpy,
                                 struct native_buffer *nbuf);

boolean
drm_display_export_native_buffer(struct native_display *ndpy,
                                 struct pipe_resource *res,
                                 struct native_buffer *nbuf);
