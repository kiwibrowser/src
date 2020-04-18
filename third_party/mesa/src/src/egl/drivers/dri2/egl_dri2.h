/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Kristian Høgsberg <krh@bitplanet.net>
 */

#ifndef EGL_DRI2_INCLUDED
#define EGL_DRI2_INCLUDED

#ifdef HAVE_X11_PLATFORM
#include <xcb/xcb.h>
#include <xcb/dri2.h>
#include <xcb/xfixes.h>
#include <X11/Xlib-xcb.h>
#endif

#ifdef HAVE_WAYLAND_PLATFORM
#include <wayland-client.h>
#include "wayland-drm.h"
#include "wayland-egl-priv.h"
#endif

#include <GL/gl.h>
#include <GL/internal/dri_interface.h>

#ifdef HAVE_DRM_PLATFORM
#include <gbm_driint.h>
#endif

#ifdef HAVE_ANDROID_PLATFORM
#define LOG_TAG "EGL-DRI2"

#if ANDROID_VERSION >= 0x0400
#  include <system/window.h>
#else
#  define android_native_buffer_t ANativeWindowBuffer
#  include <ui/egl/android_natives.h>
#  include <ui/android_native_buffer.h>
#endif

#include <hardware/gralloc.h>
#include <gralloc_drm_handle.h>
#include <cutils/log.h>

#endif /* HAVE_ANDROID_PLATFORM */

#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglcurrent.h"
#include "egllog.h"
#include "eglsurface.h"
#include "eglimage.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

struct dri2_egl_driver
{
   _EGLDriver base;

   void *handle;
   _EGLProc (*get_proc_address)(const char *procname);
   void (*glFlush)(void);
};

struct dri2_egl_display
{
   int                       dri2_major;
   int                       dri2_minor;
   __DRIscreen              *dri_screen;
   int                       own_dri_screen;
   const __DRIconfig       **driver_configs;
   void                     *driver;
   __DRIcoreExtension       *core;
   __DRIdri2Extension       *dri2;
   __DRIswrastExtension     *swrast;
   __DRI2flushExtension     *flush;
   __DRItexBufferExtension  *tex_buffer;
   __DRIimageExtension      *image;
   __DRIrobustnessExtension *robustness;
   int                       fd;

   int                       own_device;
   int                       swap_available;
   int                       invalidate_available;
#ifdef HAVE_DRM_PLATFORM
   struct gbm_dri_device    *gbm_dri;
#endif

   char                     *device_name;
   char                     *driver_name;

   __DRIdri2LoaderExtension    dri2_loader_extension;
   __DRIswrastLoaderExtension  swrast_loader_extension;
   const __DRIextension     *extensions[4];

#ifdef HAVE_X11_PLATFORM
   xcb_connection_t         *conn;
#endif

#ifdef HAVE_WAYLAND_PLATFORM
   struct wl_display        *wl_dpy;
   struct wl_registry       *wl_registry;
   struct wl_drm            *wl_server_drm;
   struct wl_drm            *wl_drm;
   struct wl_event_queue    *wl_queue;
   int			     authenticated;
   int			     formats;
#endif

   int (*authenticate) (_EGLDisplay *disp, uint32_t id);
};

struct dri2_egl_context
{
   _EGLContext   base;
   __DRIcontext *dri_context;
};

#ifdef HAVE_WAYLAND_PLATFORM
enum wayland_buffer_type {
   WL_BUFFER_FRONT,
   WL_BUFFER_BACK,
   WL_BUFFER_THIRD,
   WL_BUFFER_COUNT
};
#endif

struct dri2_egl_surface
{
   _EGLSurface          base;
   __DRIdrawable       *dri_drawable;
   __DRIbuffer          buffers[5];
   int                  buffer_count;
   int                  have_fake_front;
   int                  swap_interval;

#ifdef HAVE_X11_PLATFORM
   xcb_drawable_t       drawable;
   xcb_xfixes_region_t  region;
   int                  depth;
   int                  bytes_per_pixel;
   xcb_gcontext_t       gc;
   xcb_gcontext_t       swapgc;
#endif

#ifdef HAVE_WAYLAND_PLATFORM
   struct wl_egl_window  *wl_win;
   struct wl_egl_pixmap  *wl_pix;
   struct wl_buffer      *wl_drm_buffer[WL_BUFFER_COUNT];
   int                    wl_buffer_lock[WL_BUFFER_COUNT];
   int                    dx;
   int                    dy;
   __DRIbuffer           *dri_buffers[__DRI_BUFFER_COUNT];
   __DRIbuffer           *third_buffer;
   __DRIbuffer           *pending_buffer;
   struct wl_callback    *frame_callback;
   struct wl_callback    *pending_buffer_callback;
   int			  format;
#endif

#ifdef HAVE_DRM_PLATFORM
   struct gbm_dri_surface *gbm_surf;
   struct {
      struct gbm_bo       *bo;
      int                  locked;
   } color_buffers[3], *back, *current;
#ifndef HAVE_WAYLAND_PLATFORM
   __DRIbuffer           *dri_buffers[__DRI_BUFFER_COUNT];
#endif
#endif

#ifdef HAVE_ANDROID_PLATFORM
   struct ANativeWindow *window;
   struct ANativeWindowBuffer *buffer;

   /* EGL-owned buffers */
   __DRIbuffer           *local_buffers[__DRI_BUFFER_COUNT];
#endif
};

struct dri2_egl_buffer {
   __DRIbuffer *dri_buffer;
   struct dri2_egl_display *dri2_dpy;
};


struct dri2_egl_config
{
   _EGLConfig         base;
   const __DRIconfig *dri_single_config;
   const __DRIconfig *dri_double_config;
};

struct dri2_egl_image
{
   _EGLImage   base;
   __DRIimage *dri_image;
};

/* standard typecasts */
_EGL_DRIVER_STANDARD_TYPECASTS(dri2_egl)
_EGL_DRIVER_TYPECAST(dri2_egl_image, _EGLImage, obj)

extern const __DRIimageLookupExtension image_lookup_extension;
extern const __DRIuseInvalidateExtension use_invalidate;

EGLBoolean
dri2_load_driver(_EGLDisplay *disp);

/* Helper for platforms not using dri2_create_screen */
void
dri2_setup_screen(_EGLDisplay *disp);

EGLBoolean
dri2_load_driver_swrast(_EGLDisplay *disp);

EGLBoolean
dri2_create_screen(_EGLDisplay *disp);

__DRIimage *
dri2_lookup_egl_image(__DRIscreen *screen, void *image, void *data);

struct dri2_egl_config *
dri2_add_config(_EGLDisplay *disp, const __DRIconfig *dri_config, int id,
		int depth, EGLint surface_type, const EGLint *attr_list,
		const unsigned int *rgba_masks);

_EGLImage *
dri2_create_image_khr(_EGLDriver *drv, _EGLDisplay *disp,
		      _EGLContext *ctx, EGLenum target,
		      EGLClientBuffer buffer, const EGLint *attr_list);

EGLBoolean
dri2_initialize_x11(_EGLDriver *drv, _EGLDisplay *disp);

EGLBoolean
dri2_initialize_drm(_EGLDriver *drv, _EGLDisplay *disp);

EGLBoolean
dri2_initialize_wayland(_EGLDriver *drv, _EGLDisplay *disp);

EGLBoolean
dri2_initialize_android(_EGLDriver *drv, _EGLDisplay *disp);

char *
dri2_get_driver_for_fd(int fd);
char *
dri2_get_device_name_for_fd(int fd);

#endif /* EGL_DRI2_INCLUDED */
