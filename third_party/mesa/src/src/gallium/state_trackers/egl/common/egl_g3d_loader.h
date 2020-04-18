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

#ifndef _EGL_G3D_LOADER_H_
#define _EGL_G3D_LOADER_H_

#include "pipe/p_compiler.h"
#include "state_tracker/st_api.h"
#include "egltypedefs.h"

struct pipe_screen;
struct sw_winsys;

struct egl_g3d_loader {
   uint profile_masks[ST_API_COUNT];
   struct st_api *(*get_st_api)(enum st_api_type api);

   struct pipe_screen *(*create_drm_screen)(const char *name, int fd);
   struct pipe_screen *(*create_sw_screen)(struct sw_winsys *ws);
};

_EGLDriver *
egl_g3d_create_driver(const struct egl_g3d_loader *loader);

void
egl_g3d_destroy_driver(_EGLDriver *drv);

#endif /* _EGL_G3D_LOADER_H_ */
