/*
 * Mesa 3-D graphics library
 * Version:  7.12
 *
 * Copyright (C) 2010-2011 Chia-I Wu <olvaffe@gmail.com>
 * Copyright (C) 2010-2011 LunarG Inc.
 *
 * Based on platform_x11, which has
 *
 * Copyright © 2011 Intel Corporation
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

#include <errno.h>
#include <dlfcn.h>

/* for droid_get_pci_id */
#include <xf86drm.h>
#include <i915_drm.h>
#include <radeon_drm.h>

#include "egl_dri2.h"

static int
get_format_bpp(int native)
{
   int bpp;

   switch (native) {
   case HAL_PIXEL_FORMAT_RGBA_8888:
   case HAL_PIXEL_FORMAT_RGBX_8888:
   case HAL_PIXEL_FORMAT_BGRA_8888:
      bpp = 4;
      break;
   case HAL_PIXEL_FORMAT_RGB_888:
      bpp = 3;
      break;
   case HAL_PIXEL_FORMAT_RGB_565:
   case HAL_PIXEL_FORMAT_RGBA_5551:
   case HAL_PIXEL_FORMAT_RGBA_4444:
      bpp = 2;
      break;
   default:
      bpp = 0;
      break;
   }

   return bpp;
}

static int
get_native_buffer_name(struct ANativeWindowBuffer *buf)
{
   struct gralloc_drm_handle_t *handle;

   /* check that the buffer is allocated by drm_gralloc and cast */
   handle = gralloc_drm_handle(buf->handle);

   return (handle) ? handle->name : 0;
}

static EGLBoolean
droid_window_dequeue_buffer(struct dri2_egl_surface *dri2_surf)
{
   if (dri2_surf->window->dequeueBuffer(dri2_surf->window, &dri2_surf->buffer))
      return EGL_FALSE;

   dri2_surf->buffer->common.incRef(&dri2_surf->buffer->common);
   dri2_surf->window->lockBuffer(dri2_surf->window, dri2_surf->buffer);

   return EGL_TRUE;
}

static EGLBoolean
droid_window_enqueue_buffer(struct dri2_egl_surface *dri2_surf)
{
   dri2_surf->window->queueBuffer(dri2_surf->window, dri2_surf->buffer);

   dri2_surf->buffer->common.decRef(&dri2_surf->buffer->common);
   dri2_surf->buffer = NULL;

   return EGL_TRUE;
}

static void
droid_window_cancel_buffer(struct dri2_egl_surface *dri2_surf)
{
   /* no cancel buffer? */
   droid_window_enqueue_buffer(dri2_surf);
}

static __DRIbuffer *
droid_alloc_local_buffer(struct dri2_egl_surface *dri2_surf,
                         unsigned int att, unsigned int format)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);

   if (att >= ARRAY_SIZE(dri2_surf->local_buffers))
      return NULL;

   if (!dri2_surf->local_buffers[att]) {
      dri2_surf->local_buffers[att] =
         dri2_dpy->dri2->allocateBuffer(dri2_dpy->dri_screen, att, format,
               dri2_surf->base.Width, dri2_surf->base.Height);
   }

   return dri2_surf->local_buffers[att];
}

static void
droid_free_local_buffers(struct dri2_egl_surface *dri2_surf)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   int i;

   for (i = 0; i < ARRAY_SIZE(dri2_surf->local_buffers); i++) {
      if (dri2_surf->local_buffers[i]) {
         dri2_dpy->dri2->releaseBuffer(dri2_dpy->dri_screen,
               dri2_surf->local_buffers[i]);
         dri2_surf->local_buffers[i] = NULL;
      }
   }
}

static _EGLSurface *
droid_create_surface(_EGLDriver *drv, _EGLDisplay *disp, EGLint type,
		    _EGLConfig *conf, EGLNativeWindowType window,
		    const EGLint *attrib_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_config *dri2_conf = dri2_egl_config(conf);
   struct dri2_egl_surface *dri2_surf;
   dri2_surf = calloc(1, sizeof *dri2_surf);
   if (!dri2_surf) {
      _eglError(EGL_BAD_ALLOC, "droid_create_surface");
      return NULL;
   }

   if (!_eglInitSurface(&dri2_surf->base, disp, type, conf, attrib_list))
      goto cleanup_surface;

   if (type == EGL_WINDOW_BIT) {
      int format;

      if (!window || window->common.magic != ANDROID_NATIVE_WINDOW_MAGIC) {
         _eglError(EGL_BAD_NATIVE_WINDOW, "droid_create_surface");
         goto cleanup_surface;
      }
      if (window->query(window, NATIVE_WINDOW_FORMAT, &format)) {
         _eglError(EGL_BAD_NATIVE_WINDOW, "droid_create_surface");
         goto cleanup_surface;
      }

      if (format != dri2_conf->base.NativeVisualID) {
         _eglLog(_EGL_WARNING, "Native format mismatch: 0x%x != 0x%x",
               format, dri2_conf->base.NativeVisualID);
      }

      window->query(window, NATIVE_WINDOW_WIDTH, &dri2_surf->base.Width);
      window->query(window, NATIVE_WINDOW_HEIGHT, &dri2_surf->base.Height);
   }

   dri2_surf->dri_drawable =
      (*dri2_dpy->dri2->createNewDrawable)(dri2_dpy->dri_screen,
					   dri2_conf->dri_double_config,
                                           dri2_surf);
   if (dri2_surf->dri_drawable == NULL) {
      _eglError(EGL_BAD_ALLOC, "dri2->createNewDrawable");
      goto cleanup_surface;
   }

   if (window) {
      window->common.incRef(&window->common);
      dri2_surf->window = window;
   }

   return &dri2_surf->base;

cleanup_surface:
   free(dri2_surf);

   return NULL;
}

static _EGLSurface *
droid_create_window_surface(_EGLDriver *drv, _EGLDisplay *disp,
			   _EGLConfig *conf, EGLNativeWindowType window,
			   const EGLint *attrib_list)
{
   return droid_create_surface(drv, disp, EGL_WINDOW_BIT, conf,
			      window, attrib_list);
}

static _EGLSurface *
droid_create_pixmap_surface(_EGLDriver *drv, _EGLDisplay *disp,
			   _EGLConfig *conf, EGLNativePixmapType pixmap,
			   const EGLint *attrib_list)
{
   return NULL;
}

static _EGLSurface *
droid_create_pbuffer_surface(_EGLDriver *drv, _EGLDisplay *disp,
			    _EGLConfig *conf, const EGLint *attrib_list)
{
   return droid_create_surface(drv, disp, EGL_PBUFFER_BIT, conf,
			      NULL, attrib_list);
}

static EGLBoolean
droid_destroy_surface(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surf)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);

   if (!_eglPutSurface(surf))
      return EGL_TRUE;

   droid_free_local_buffers(dri2_surf);

   if (dri2_surf->base.Type == EGL_WINDOW_BIT) {
      if (dri2_surf->buffer)
         droid_window_cancel_buffer(dri2_surf);

      dri2_surf->window->common.decRef(&dri2_surf->window->common);
   }

   (*dri2_dpy->core->destroyDrawable)(dri2_surf->dri_drawable);

   free(dri2_surf);

   return EGL_TRUE;
}

static EGLBoolean
droid_swap_buffers(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *draw)
{
   struct dri2_egl_driver *dri2_drv = dri2_egl_driver(drv);
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);
   _EGLContext *ctx;

   if (dri2_surf->base.Type != EGL_WINDOW_BIT)
      return EGL_TRUE;

   if (dri2_drv->glFlush) {
      ctx = _eglGetCurrentContext();
      if (ctx && ctx->DrawSurface == &dri2_surf->base)
         dri2_drv->glFlush();
   }

   (*dri2_dpy->flush->flush)(dri2_surf->dri_drawable);

   if (dri2_surf->buffer)
      droid_window_enqueue_buffer(dri2_surf);

   (*dri2_dpy->flush->invalidate)(dri2_surf->dri_drawable);

   return EGL_TRUE;
}

static _EGLImage *
dri2_create_image_android_native_buffer(_EGLDisplay *disp,
                                        struct ANativeWindowBuffer *buf)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img;
   int name;
   EGLint format;

   if (!buf || buf->common.magic != ANDROID_NATIVE_BUFFER_MAGIC ||
       buf->common.version != sizeof(*buf)) {
      _eglError(EGL_BAD_PARAMETER, "eglCreateEGLImageKHR");
      return NULL;
   }

   name = get_native_buffer_name(buf);
   if (!name) {
      _eglError(EGL_BAD_PARAMETER, "eglCreateEGLImageKHR");
      return NULL;
   }

   /* see the table in droid_add_configs_for_visuals */
   switch (buf->format) {
   case HAL_PIXEL_FORMAT_BGRA_8888:
      format = __DRI_IMAGE_FORMAT_ARGB8888;
      break;
   case HAL_PIXEL_FORMAT_RGB_565:
      format = __DRI_IMAGE_FORMAT_RGB565;
      break;
   case HAL_PIXEL_FORMAT_RGBA_8888:
      format = __DRI_IMAGE_FORMAT_ABGR8888;
      break;
   case HAL_PIXEL_FORMAT_RGBX_8888:
      format = __DRI_IMAGE_FORMAT_XBGR8888;
      break;
   case HAL_PIXEL_FORMAT_RGB_888:
   case HAL_PIXEL_FORMAT_RGBA_5551:
   case HAL_PIXEL_FORMAT_RGBA_4444:
      /* unsupported */
   default:
      _eglLog(_EGL_WARNING, "unsupported native buffer format 0x%x", buf->format);
      return NULL;
      break;
   }

   dri2_img = calloc(1, sizeof(*dri2_img));
   if (!dri2_img) {
      _eglError(EGL_BAD_ALLOC, "droid_create_image_mesa_drm");
      return NULL;
   }

   if (!_eglInitImage(&dri2_img->base, disp)) {
      free(dri2_img);
      return NULL;
   }

   dri2_img->dri_image =
      dri2_dpy->image->createImageFromName(dri2_dpy->dri_screen,
					   buf->width,
					   buf->height,
					   format,
					   name,
					   buf->stride,
					   dri2_img);
   if (!dri2_img->dri_image) {
      free(dri2_img);
      _eglError(EGL_BAD_ALLOC, "droid_create_image_mesa_drm");
      return NULL;
   }

   return &dri2_img->base;
}

static _EGLImage *
droid_create_image_khr(_EGLDriver *drv, _EGLDisplay *disp,
		       _EGLContext *ctx, EGLenum target,
		       EGLClientBuffer buffer, const EGLint *attr_list)
{
   switch (target) {
   case EGL_NATIVE_BUFFER_ANDROID:
      return dri2_create_image_android_native_buffer(disp,
            (struct ANativeWindowBuffer *) buffer);
   default:
      return dri2_create_image_khr(drv, disp, ctx, target, buffer, attr_list);
   }
}

static void
droid_init_driver_functions(_EGLDriver *drv)
{
   drv->API.CreateWindowSurface = droid_create_window_surface;
   drv->API.CreatePixmapSurface = droid_create_pixmap_surface;
   drv->API.CreatePbufferSurface = droid_create_pbuffer_surface;
   drv->API.DestroySurface = droid_destroy_surface;
   drv->API.SwapBuffers = droid_swap_buffers;

   drv->API.CreateImageKHR = droid_create_image_khr;
}

static void
droid_flush_front_buffer(__DRIdrawable * driDrawable, void *loaderPrivate)
{
}

static int
droid_get_buffers_parse_attachments(struct dri2_egl_surface *dri2_surf,
                                    unsigned int *attachments, int count)
{
   int num_buffers = 0, i;

   /* fill dri2_surf->buffers */
   for (i = 0; i < count * 2; i += 2) {
      __DRIbuffer *buf, *local;

      assert(num_buffers < ARRAY_SIZE(dri2_surf->buffers));
      buf = &dri2_surf->buffers[num_buffers];

      switch (attachments[i]) {
      case __DRI_BUFFER_BACK_LEFT:
         if (dri2_surf->base.Type == EGL_WINDOW_BIT) {
            buf->attachment = attachments[i];
            buf->name = get_native_buffer_name(dri2_surf->buffer);
            buf->cpp = get_format_bpp(dri2_surf->buffer->format);
            buf->pitch = dri2_surf->buffer->stride * buf->cpp;
            buf->flags = 0;

            if (buf->name)
               num_buffers++;

            break;
         }
         /* fall through for pbuffers */
      case __DRI_BUFFER_DEPTH:
      case __DRI_BUFFER_STENCIL:
      case __DRI_BUFFER_ACCUM:
      case __DRI_BUFFER_DEPTH_STENCIL:
      case __DRI_BUFFER_HIZ:
         local = droid_alloc_local_buffer(dri2_surf,
               attachments[i], attachments[i + 1]);

         if (local) {
            *buf = *local;
            num_buffers++;
         }
         break;
      case __DRI_BUFFER_FRONT_LEFT:
      case __DRI_BUFFER_FRONT_RIGHT:
      case __DRI_BUFFER_FAKE_FRONT_LEFT:
      case __DRI_BUFFER_FAKE_FRONT_RIGHT:
      case __DRI_BUFFER_BACK_RIGHT:
      default:
         /* no front or right buffers */
         break;
      }
   }

   return num_buffers;
}

static __DRIbuffer *
droid_get_buffers_with_format(__DRIdrawable * driDrawable,
			     int *width, int *height,
			     unsigned int *attachments, int count,
			     int *out_count, void *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   int i;

   if (dri2_surf->base.Type == EGL_WINDOW_BIT) {
      /* try to dequeue the next back buffer */
      if (!dri2_surf->buffer && !droid_window_dequeue_buffer(dri2_surf))
         return NULL;

      /* free outdated buffers and update the surface size */
      if (dri2_surf->base.Width != dri2_surf->buffer->width ||
          dri2_surf->base.Height != dri2_surf->buffer->height) {
         droid_free_local_buffers(dri2_surf);
         dri2_surf->base.Width = dri2_surf->buffer->width;
         dri2_surf->base.Height = dri2_surf->buffer->height;
      }
   }

   dri2_surf->buffer_count =
      droid_get_buffers_parse_attachments(dri2_surf, attachments, count);

   if (width)
      *width = dri2_surf->base.Width;
   if (height)
      *height = dri2_surf->base.Height;

   *out_count = dri2_surf->buffer_count;;

   return dri2_surf->buffers;
}

static EGLBoolean
droid_add_configs_for_visuals(_EGLDriver *drv, _EGLDisplay *dpy)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(dpy);
   const struct {
      int format;
      int size;
      unsigned int rgba_masks[4];
   } visuals[] = {
      { HAL_PIXEL_FORMAT_RGBA_8888, 32, { 0xff, 0xff00, 0xff0000, 0xff000000 } },
      { HAL_PIXEL_FORMAT_RGBX_8888, 32, { 0xff, 0xff00, 0xff0000, 0x0 } },
      { HAL_PIXEL_FORMAT_RGB_888,   24, { 0xff, 0xff00, 0xff0000, 0x0 } },
      { HAL_PIXEL_FORMAT_RGB_565,   16, { 0xf800, 0x7e0, 0x1f, 0x0 } },
      { HAL_PIXEL_FORMAT_BGRA_8888, 32, { 0xff0000, 0xff00, 0xff, 0xff000000 } },
      { 0, 0, { 0, 0, 0, 0 } }
   };
   int count, i, j;

   count = 0;
   for (i = 0; visuals[i].format; i++) {
      int format_count = 0;

      for (j = 0; dri2_dpy->driver_configs[j]; j++) {
         const EGLint surface_type = EGL_WINDOW_BIT | EGL_PBUFFER_BIT;
         struct dri2_egl_config *dri2_conf;
         unsigned int double_buffered = 0;

         dri2_dpy->core->getConfigAttrib(dri2_dpy->driver_configs[j],
            __DRI_ATTRIB_DOUBLE_BUFFER, &double_buffered);

         /* support only double buffered configs */
         if (!double_buffered)
            continue;

         dri2_conf = dri2_add_config(dpy, dri2_dpy->driver_configs[j],
               count + 1, visuals[i].size, surface_type, NULL,
               visuals[i].rgba_masks);
         if (dri2_conf) {
            dri2_conf->base.NativeVisualID = visuals[i].format;
            dri2_conf->base.NativeVisualType = visuals[i].format;
            count++;
            format_count++;
         }
      }

      if (!format_count) {
         _eglLog(_EGL_DEBUG, "No DRI config supports native format 0x%x",
               visuals[i].format);
      }
   }

   /* post-process configs */
   for (i = 0; i < dpy->Configs->Size; i++) {
      struct dri2_egl_config *dri2_conf = dri2_egl_config(dpy->Configs->Elements[i]);

      /* there is no front buffer so no OpenGL */
      dri2_conf->base.RenderableType &= ~EGL_OPENGL_BIT;
      dri2_conf->base.Conformant &= ~EGL_OPENGL_BIT;
   }

   return (count != 0);
}

static EGLBoolean
droid_get_pci_id(int fd, int *vendor_id, int *chip_id)
{
   drmVersionPtr version;

   *chip_id = -1;

   version = drmGetVersion(fd);
   if (!version) {
      _eglLog(_EGL_WARNING, "invalid drm fd");
      return EGL_FALSE;
   }
   if (!version->name) {
      _eglLog(_EGL_WARNING, "unable to determine the driver name");
      drmFreeVersion(version);
      return EGL_FALSE;
   }

   if (strcmp(version->name, "i915") == 0) {
      struct drm_i915_getparam gp;
      int ret;

      *vendor_id = 0x8086;

      memset(&gp, 0, sizeof(gp));
      gp.param = I915_PARAM_CHIPSET_ID;
      gp.value = chip_id;
      ret = drmCommandWriteRead(fd, DRM_I915_GETPARAM, &gp, sizeof(gp));
      if (ret) {
         _eglLog(_EGL_WARNING, "failed to get param for i915");
	 *chip_id = -1;
      }
   }
   else if (strcmp(version->name, "radeon") == 0) {
      struct drm_radeon_info info;
      int ret;

      *vendor_id = 0x1002;

      memset(&info, 0, sizeof(info));
      info.request = RADEON_INFO_DEVICE_ID;
      info.value = (unsigned long) chip_id;
      ret = drmCommandWriteRead(fd, DRM_RADEON_INFO, &info, sizeof(info));
      if (ret) {
         _eglLog(_EGL_WARNING, "failed to get info for radeon");
	 *chip_id = -1;
      }
   }
   else if (strcmp(version->name, "nouveau") == 0) {
      *vendor_id = 0x10de;
      /* not used */
      *chip_id = 0;
   }
   else if (strcmp(version->name, "vmwgfx") == 0) {
      *vendor_id = 0x15ad;
      /* assume SVGA II */
      *chip_id = 0x0405;
   }

   drmFreeVersion(version);

   return (*chip_id >= 0);
}

#define DRIVER_MAP_DRI2_ONLY
#include "pci_ids/pci_id_driver_map.h"
static const char *
droid_get_driver_name(int fd)
{
   int vendor_id = -1, chip_id = -1;
   int idx, i;
   char *name;

   if (!droid_get_pci_id(fd, &vendor_id, &chip_id))
      return NULL;

   for (idx = 0; driver_map[idx].driver; idx++) {
      if (vendor_id != driver_map[idx].vendor_id)
         continue;

      if (driver_map[idx].num_chips_ids == -1)
         break;

      for (i = 0; i < driver_map[idx].num_chips_ids; i++) {
         if (driver_map[idx].chip_ids[i] == chip_id)
            break;
      }
      if (i < driver_map[idx].num_chips_ids)
	      break;
   }

   _eglLog(_EGL_INFO, "pci id for fd %d: %04x:%04x, driver %s",
         fd, vendor_id, chip_id, driver_map[idx].driver);

   return driver_map[idx].driver;
}

static int
droid_open_device(void)
{
   const hw_module_t *mod;
   int fd = -1, err;

   err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &mod);
   if (!err) {
      const gralloc_module_t *gr = (gralloc_module_t *) mod;

      err = -EINVAL;
      if (gr->perform)
         err = gr->perform(gr, GRALLOC_MODULE_PERFORM_GET_DRM_FD, &fd);
   }
   if (err || fd < 0) {
      _eglLog(_EGL_WARNING, "fail to get drm fd");
      fd = -1;
   }

   return (fd >= 0) ? dup(fd) : -1;
}

/* support versions < JellyBean */
#ifndef ALOGW
#define ALOGW LOGW
#endif
#ifndef ALOGD
#define ALOGD LOGD
#endif
#ifndef ALOGI
#define ALOGI LOGI
#endif

static void
droid_log(EGLint level, const char *msg)
{
   switch (level) {
   case _EGL_DEBUG:
      ALOGD("%s", msg);
      break;
   case _EGL_INFO:
      ALOGI("%s", msg);
      break;
   case _EGL_WARNING:
      ALOGW("%s", msg);
      break;
   case _EGL_FATAL:
      LOG_FATAL("%s", msg);
      break;
   default:
      break;
   }
}

EGLBoolean
dri2_initialize_android(_EGLDriver *drv, _EGLDisplay *dpy)
{
   struct dri2_egl_display *dri2_dpy;
   const char *err;

   _eglSetLogProc(droid_log);

   dri2_dpy = calloc(1, sizeof(*dri2_dpy));
   if (!dri2_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   dpy->DriverData = (void *) dri2_dpy;

   dri2_dpy->fd = droid_open_device();
   if (dri2_dpy->fd < 0) {
      err = "DRI2: failed to open device";
      goto cleanup_display;
   }

   dri2_dpy->driver_name = (char *) droid_get_driver_name(dri2_dpy->fd);
   if (dri2_dpy->driver_name == NULL) {
      err = "DRI2: failed to get driver name";
      goto cleanup_device;
   }

   if (!dri2_load_driver(dpy)) {
      err = "DRI2: failed to load driver";
      goto cleanup_device;
   }

   dri2_dpy->dri2_loader_extension.base.name = __DRI_DRI2_LOADER;
   dri2_dpy->dri2_loader_extension.base.version = 3;
   dri2_dpy->dri2_loader_extension.getBuffers = NULL;
   dri2_dpy->dri2_loader_extension.flushFrontBuffer = droid_flush_front_buffer;
   dri2_dpy->dri2_loader_extension.getBuffersWithFormat =
      droid_get_buffers_with_format;

   dri2_dpy->extensions[0] = &dri2_dpy->dri2_loader_extension.base;
   dri2_dpy->extensions[1] = &image_lookup_extension.base;
   dri2_dpy->extensions[2] = &use_invalidate.base;
   dri2_dpy->extensions[3] = NULL;

   if (!dri2_create_screen(dpy)) {
      err = "DRI2: failed to create screen";
      goto cleanup_driver;
   }

   if (!droid_add_configs_for_visuals(drv, dpy)) {
      err = "DRI2: failed to add configs";
      goto cleanup_screen;
   }

   dpy->Extensions.ANDROID_image_native_buffer = EGL_TRUE;
   dpy->Extensions.KHR_image_base = EGL_TRUE;

   /* we're supporting EGL 1.4 */
   dpy->VersionMajor = 1;
   dpy->VersionMinor = 4;

   droid_init_driver_functions(drv);

   return EGL_TRUE;

cleanup_screen:
   dri2_dpy->core->destroyScreen(dri2_dpy->dri_screen);
cleanup_driver:
   dlclose(dri2_dpy->driver);
cleanup_device:
   close(dri2_dpy->fd);
cleanup_display:
   free(dri2_dpy);

   return _eglError(EGL_NOT_INITIALIZED, err);
}
