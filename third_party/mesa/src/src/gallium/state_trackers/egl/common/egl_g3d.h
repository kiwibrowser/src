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

#ifndef _EGL_G3D_H_
#define _EGL_G3D_H_

#include "pipe/p_compiler.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_format.h"
#include "os/os_thread.h"
#include "egldriver.h"
#include "egldisplay.h"
#include "eglcontext.h"
#include "eglsurface.h"
#include "eglconfig.h"
#include "eglimage.h"
#include "eglsync.h"
#include "eglscreen.h"
#include "eglmode.h"

#include "native.h"
#include "egl_g3d_st.h"
#include "egl_g3d_loader.h"

struct egl_g3d_driver {
   _EGLDriver base;
   const struct egl_g3d_loader *loader;
   const struct native_platform *platforms[_EGL_NUM_PLATFORMS];
};

struct egl_g3d_display {
   struct native_display *native;

   const struct egl_g3d_loader *loader;
   struct st_manager *smapi;
};

struct egl_g3d_context {
   _EGLContext base;

   struct st_api *stapi;

   struct st_context_iface *stctxi;
};

struct egl_g3d_surface {
   _EGLSurface base;

   struct st_visual stvis;
   struct st_framebuffer_iface *stfbi;

   /* the native surface;  NULL for pbuffers */
   struct native_surface *native;
   struct pipe_resource *render_texture;

   EGLenum client_buffer_type;
   EGLClientBuffer client_buffer;

   unsigned int sequence_number;
};

struct egl_g3d_config {
   _EGLConfig base;
   const struct native_config *native;
   struct st_visual stvis;
};

struct egl_g3d_image {
   _EGLImage base;
   struct pipe_resource *texture;
   unsigned level;
   unsigned layer;
};

/* standard typecasts */
_EGL_DRIVER_STANDARD_TYPECASTS(egl_g3d)
_EGL_DRIVER_TYPECAST(egl_g3d_image, _EGLImage, obj)

struct egl_g3d_sync {
   _EGLSync base;

   /* the mutex protects only the condvar, not the struct */
   pipe_mutex mutex;
   pipe_condvar condvar;

   /* for fence sync */
   struct pipe_fence_handle *fence;
};
_EGL_DRIVER_TYPECAST(egl_g3d_sync, _EGLSync, obj)

#ifdef EGL_MESA_screen_surface

struct egl_g3d_screen {
   _EGLScreen base;
   const struct native_connector *native;
   const struct native_mode **native_modes;
};
_EGL_DRIVER_TYPECAST(egl_g3d_screen, _EGLScreen, obj)

#endif /* EGL_MESA_screen_surface */

static INLINE struct st_api *
egl_g3d_get_st_api(_EGLDriver *drv, enum st_api_type api)
{
   struct egl_g3d_driver *gdrv = egl_g3d_driver(drv);

   return gdrv->loader->get_st_api(api);
}

#endif /* _EGL_G3D_H_ */
