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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <xf86drm.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "egl_dri2.h"

static struct gbm_bo *
lock_front_buffer(struct gbm_surface *_surf)
{
   struct gbm_dri_surface *surf = (struct gbm_dri_surface *) _surf;
   struct dri2_egl_surface *dri2_surf = surf->dri_private;
   struct gbm_bo *bo;

   if (dri2_surf->current == NULL) {
      _eglError(EGL_BAD_SURFACE, "no front buffer");
      return NULL;
   }

   bo = dri2_surf->current->bo;
   dri2_surf->current->locked = 1;
   dri2_surf->current = NULL;

   return bo;
}

static void
release_buffer(struct gbm_surface *_surf, struct gbm_bo *bo)
{
   struct gbm_dri_surface *surf = (struct gbm_dri_surface *) _surf;
   struct dri2_egl_surface *dri2_surf = surf->dri_private;
   int i;

   for (i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (dri2_surf->color_buffers[i].bo == bo) {
	 dri2_surf->color_buffers[i].locked = 0;
      }
   }
}

static int
has_free_buffers(struct gbm_surface *_surf)
{
   struct gbm_dri_surface *surf = (struct gbm_dri_surface *) _surf;
   struct dri2_egl_surface *dri2_surf = surf->dri_private;
   int i;

   for (i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++)
      if (!dri2_surf->color_buffers[i].locked)
	 return 1;

   return 0;
}

static _EGLSurface *
dri2_create_surface(_EGLDriver *drv, _EGLDisplay *disp, EGLint type,
		    _EGLConfig *conf, EGLNativeWindowType window,
		    const EGLint *attrib_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_config *dri2_conf = dri2_egl_config(conf);
   struct dri2_egl_surface *dri2_surf;
   struct gbm_dri_surface *surf;

   (void) drv;

   dri2_surf = malloc(sizeof *dri2_surf);
   if (!dri2_surf) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
      return NULL;
   }

   memset(dri2_surf, 0, sizeof *dri2_surf);
   if (!_eglInitSurface(&dri2_surf->base, disp, type, conf, attrib_list))
      goto cleanup_surf;

   switch (type) {
   case EGL_WINDOW_BIT:
      if (!window)
         return NULL;
      surf = gbm_dri_surface((struct gbm_surface *) window);
      dri2_surf->gbm_surf = surf;
      dri2_surf->base.Width =  surf->base.width;
      dri2_surf->base.Height = surf->base.height;
      surf->dri_private = dri2_surf;
      break;
   default:
      goto cleanup_surf;
   }

   dri2_surf->dri_drawable =
      (*dri2_dpy->dri2->createNewDrawable) (dri2_dpy->dri_screen,
					    dri2_conf->dri_double_config,
					    dri2_surf->gbm_surf);

   if (dri2_surf->dri_drawable == NULL) {
      _eglError(EGL_BAD_ALLOC, "dri2->createNewDrawable");
      goto cleanup_surf;
   }

   return &dri2_surf->base;

 cleanup_surf:
   free(dri2_surf);

   return NULL;
}

static _EGLSurface *
dri2_create_window_surface(_EGLDriver *drv, _EGLDisplay *disp,
			   _EGLConfig *conf, EGLNativeWindowType window,
			   const EGLint *attrib_list)
{
   return dri2_create_surface(drv, disp, EGL_WINDOW_BIT, conf,
			      window, attrib_list);
}

static EGLBoolean
dri2_destroy_surface(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surf)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
   int i;

   if (!_eglPutSurface(surf))
      return EGL_TRUE;

   (*dri2_dpy->core->destroyDrawable)(dri2_surf->dri_drawable);

   for (i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (dri2_surf->color_buffers[i].bo)
	 gbm_bo_destroy(dri2_surf->color_buffers[i].bo);
   }

   for (i = 0; i < __DRI_BUFFER_COUNT; i++) {
      if (dri2_surf->dri_buffers[i])
         dri2_dpy->dri2->releaseBuffer(dri2_dpy->dri_screen,
                                       dri2_surf->dri_buffers[i]);
   }

   free(surf);

   return EGL_TRUE;
}

static int
get_back_bo(struct dri2_egl_surface *dri2_surf, __DRIbuffer *buffer)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   struct gbm_dri_bo *bo;
   struct gbm_dri_surface *surf = dri2_surf->gbm_surf;
   int i, name, pitch;

   if (dri2_surf->back == NULL) {
      for (i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
	 if (!dri2_surf->color_buffers[i].locked) {
	    dri2_surf->back = &dri2_surf->color_buffers[i];
	    break;
	 }
      }
   }

   if (dri2_surf->back == NULL)
      return -1;
   if (dri2_surf->back->bo == NULL)
      dri2_surf->back->bo = gbm_bo_create(&dri2_dpy->gbm_dri->base.base,
					  surf->base.width, surf->base.height,
					  surf->base.format, surf->base.flags);
   if (dri2_surf->back->bo == NULL)
      return -1;

   bo = (struct gbm_dri_bo *) dri2_surf->back->bo;

   dri2_dpy->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_NAME, &name);
   dri2_dpy->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_STRIDE, &pitch);

   buffer->attachment = __DRI_BUFFER_BACK_LEFT;
   buffer->name = name;
   buffer->pitch = pitch;
   buffer->cpp = 4;
   buffer->flags = 0;

   return 0;
}

static int
get_aux_bo(struct dri2_egl_surface *dri2_surf,
	   unsigned int attachment, unsigned int format, __DRIbuffer *buffer)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   __DRIbuffer *b = dri2_surf->dri_buffers[attachment];

   if (b == NULL) {
      b = dri2_dpy->dri2->allocateBuffer(dri2_dpy->dri_screen,
					 attachment, format,
					 dri2_surf->base.Width,
					 dri2_surf->base.Height);
      dri2_surf->dri_buffers[attachment] = b;
   }
   if (b == NULL)
      return -1;

   memcpy(buffer, b, sizeof *buffer);

   return 0;
}

static __DRIbuffer *
dri2_get_buffers_with_format(__DRIdrawable *driDrawable,
			     int *width, int *height,
			     unsigned int *attachments, int count,
			     int *out_count, void *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   int i, j;

   dri2_surf->buffer_count = 0;
   for (i = 0, j = 0; i < 2 * count; i += 2, j++) {
      assert(attachments[i] < __DRI_BUFFER_COUNT);
      assert(dri2_surf->buffer_count < 5);

      switch (attachments[i]) {
      case __DRI_BUFFER_BACK_LEFT:
	 if (get_back_bo(dri2_surf, &dri2_surf->buffers[j]) < 0) {
	    _eglError(EGL_BAD_ALLOC, "failed to allocate color buffer");
	    return NULL;
	 }
	 break;
      default:
	 if (get_aux_bo(dri2_surf, attachments[i], attachments[i + 1],
			&dri2_surf->buffers[j]) < 0) {
	    _eglError(EGL_BAD_ALLOC, "failed to allocate aux buffer");
	    return NULL;
	 }
	 break;
      }
   }

   *out_count = j;
   if (j == 0)
      return NULL;

   *width = dri2_surf->base.Width;
   *height = dri2_surf->base.Height;

   return dri2_surf->buffers;
}

static __DRIbuffer *
dri2_get_buffers(__DRIdrawable * driDrawable,
		 int *width, int *height,
		 unsigned int *attachments, int count,
		 int *out_count, void *loaderPrivate)
{
   unsigned int *attachments_with_format;
   __DRIbuffer *buffer;
   const unsigned int format = 32;
   int i;

   attachments_with_format = calloc(count * 2, sizeof(unsigned int));
   if (!attachments_with_format) {
      *out_count = 0;
      return NULL;
   }

   for (i = 0; i < count; ++i) {
      attachments_with_format[2*i] = attachments[i];
      attachments_with_format[2*i + 1] = format;
   }

   buffer =
      dri2_get_buffers_with_format(driDrawable,
				   width, height,
				   attachments_with_format, count,
				   out_count, loaderPrivate);

   free(attachments_with_format);

   return buffer;
}

static void
dri2_flush_front_buffer(__DRIdrawable * driDrawable, void *loaderPrivate)
{
   (void) driDrawable;
   (void) loaderPrivate;
}

static EGLBoolean
dri2_swap_buffers(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *draw)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);

   if (dri2_surf->base.Type == EGL_WINDOW_BIT) {
      if (dri2_surf->current)
	 _eglError(EGL_BAD_SURFACE, "dri2_swap_buffers");
      dri2_surf->current = dri2_surf->back;
      dri2_surf->back = NULL;
   }

   (*dri2_dpy->flush->flush)(dri2_surf->dri_drawable);
   (*dri2_dpy->flush->invalidate)(dri2_surf->dri_drawable);

   return EGL_TRUE;
}

static _EGLImage *
dri2_create_image_khr_pixmap(_EGLDisplay *disp, _EGLContext *ctx,
			     EGLClientBuffer buffer, const EGLint *attr_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct gbm_dri_bo *dri_bo = gbm_dri_bo((struct gbm_bo *) buffer);
   struct dri2_egl_image *dri2_img;

   dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_image_khr_pixmap");
      return NULL;
   }

   if (!_eglInitImage(&dri2_img->base, disp)) {
      free(dri2_img);
      return NULL;
   }

   dri2_img->dri_image = dri2_dpy->image->dupImage(dri_bo->image, dri2_img);
   if (dri2_img->dri_image == NULL) {
      free(dri2_img);
      _eglError(EGL_BAD_ALLOC, "dri2_create_image_khr_pixmap");
      return NULL;
   }

   return &dri2_img->base;
}

static _EGLImage *
dri2_drm_create_image_khr(_EGLDriver *drv, _EGLDisplay *disp,
                          _EGLContext *ctx, EGLenum target,
                          EGLClientBuffer buffer, const EGLint *attr_list)
{
   (void) drv;

   switch (target) {
   case EGL_NATIVE_PIXMAP_KHR:
      return dri2_create_image_khr_pixmap(disp, ctx, buffer, attr_list);
   default:
      return dri2_create_image_khr(drv, disp, ctx, target, buffer, attr_list);
   }
}

static int
dri2_drm_authenticate(_EGLDisplay *disp, uint32_t id)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);

   return drmAuthMagic(dri2_dpy->fd, id);
}

EGLBoolean
dri2_initialize_drm(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy;
   struct gbm_device *gbm;
   int fd = -1;
   int i;

   dri2_dpy = malloc(sizeof *dri2_dpy);
   if (!dri2_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   memset(dri2_dpy, 0, sizeof *dri2_dpy);

   disp->DriverData = (void *) dri2_dpy;

   gbm = disp->PlatformDisplay;
   if (gbm == NULL) {
      fd = open("/dev/dri/card0", O_RDWR);
      dri2_dpy->own_device = 1;
      gbm = gbm_create_device(fd);
      if (gbm == NULL)
         return EGL_FALSE;
   }

   if (strcmp(gbm_device_get_backend_name(gbm), "drm") != 0) {
      free(dri2_dpy);
      return EGL_FALSE;
   }

   dri2_dpy->gbm_dri = gbm_dri_device(gbm);
   if (dri2_dpy->gbm_dri->base.type != GBM_DRM_DRIVER_TYPE_DRI) {
      free(dri2_dpy);
      return EGL_FALSE;
   }

   if (fd < 0) {
      fd = dup(gbm_device_get_fd(gbm));
      if (fd < 0) {
         free(dri2_dpy);
         return EGL_FALSE;
      }
   }

   dri2_dpy->fd = fd;
   dri2_dpy->device_name = dri2_get_device_name_for_fd(dri2_dpy->fd);
   dri2_dpy->driver_name = dri2_dpy->gbm_dri->base.driver_name;

   dri2_dpy->dri_screen = dri2_dpy->gbm_dri->screen;
   dri2_dpy->core = dri2_dpy->gbm_dri->core;
   dri2_dpy->dri2 = dri2_dpy->gbm_dri->dri2;
   dri2_dpy->image = dri2_dpy->gbm_dri->image;
   dri2_dpy->flush = dri2_dpy->gbm_dri->flush;
   dri2_dpy->driver_configs = dri2_dpy->gbm_dri->driver_configs;

   dri2_dpy->gbm_dri->lookup_image = dri2_lookup_egl_image;
   dri2_dpy->gbm_dri->lookup_user_data = disp;

   dri2_dpy->gbm_dri->get_buffers = dri2_get_buffers;
   dri2_dpy->gbm_dri->flush_front_buffer = dri2_flush_front_buffer;
   dri2_dpy->gbm_dri->get_buffers_with_format = dri2_get_buffers_with_format;

   dri2_dpy->gbm_dri->base.base.surface_lock_front_buffer = lock_front_buffer;
   dri2_dpy->gbm_dri->base.base.surface_release_buffer = release_buffer;
   dri2_dpy->gbm_dri->base.base.surface_has_free_buffers = has_free_buffers;

   dri2_setup_screen(disp);

   for (i = 0; dri2_dpy->driver_configs[i]; i++)
      dri2_add_config(disp, dri2_dpy->driver_configs[i],
                      i + 1, 0, EGL_WINDOW_BIT, NULL, NULL);

   drv->API.CreateWindowSurface = dri2_create_window_surface;
   drv->API.DestroySurface = dri2_destroy_surface;
   drv->API.SwapBuffers = dri2_swap_buffers;
   drv->API.CreateImageKHR = dri2_drm_create_image_khr;

#ifdef HAVE_WAYLAND_PLATFORM
   disp->Extensions.WL_bind_wayland_display = EGL_TRUE;
#endif
   dri2_dpy->authenticate = dri2_drm_authenticate;

   /* we're supporting EGL 1.4 */
   disp->VersionMajor = 1;
   disp->VersionMinor = 4;

   return EGL_TRUE;
}
