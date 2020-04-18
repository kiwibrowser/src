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
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <xf86drm.h>

#include "egl_dri2.h"

#include <wayland-client.h>
#include "wayland-drm-client-protocol.h"

enum wl_drm_format_flags {
   HAS_ARGB8888 = 1,
   HAS_XRGB8888 = 2
};

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

static int
roundtrip(struct dri2_egl_display *dri2_dpy)
{
   struct wl_callback *callback;
   int done = 0, ret = 0;

   callback = wl_display_sync(dri2_dpy->wl_dpy);
   wl_callback_add_listener(callback, &sync_listener, &done);
   wl_proxy_set_queue((struct wl_proxy *) callback, dri2_dpy->wl_queue);
   while (ret != -1 && !done)
      ret = wl_display_dispatch_queue(dri2_dpy->wl_dpy, dri2_dpy->wl_queue);

   if (!done)
      wl_callback_destroy(callback);

   return ret;
}

static void
wl_buffer_release(void *data, struct wl_buffer *buffer)
{
   struct dri2_egl_surface *dri2_surf = data;
   int i;

   for (i = 0; i < WL_BUFFER_COUNT; ++i)
      if (dri2_surf->wl_drm_buffer[i] == buffer)
         break;

   assert(i <= WL_BUFFER_COUNT);

   /* not found? */
   if (i == WL_BUFFER_COUNT)
      return;

   dri2_surf->wl_buffer_lock[i] = 0;

}

static struct wl_buffer_listener wl_buffer_listener = {
   wl_buffer_release
};

static void
resize_callback(struct wl_egl_window *wl_win, void *data)
{
   struct dri2_egl_surface *dri2_surf = data;
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);

   (*dri2_dpy->flush->invalidate)(dri2_surf->dri_drawable);
}

/**
 * Called via eglCreateWindowSurface(), drv->API.CreateWindowSurface().
 */
static _EGLSurface *
dri2_create_surface(_EGLDriver *drv, _EGLDisplay *disp, EGLint type,
		    _EGLConfig *conf, EGLNativeWindowType window,
		    const EGLint *attrib_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_config *dri2_conf = dri2_egl_config(conf);
   struct dri2_egl_surface *dri2_surf;
   int i;

   (void) drv;

   dri2_surf = malloc(sizeof *dri2_surf);
   if (!dri2_surf) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
      return NULL;
   }
   
   if (!_eglInitSurface(&dri2_surf->base, disp, type, conf, attrib_list))
      goto cleanup_surf;

   for (i = 0; i < WL_BUFFER_COUNT; ++i) {
      dri2_surf->wl_drm_buffer[i] = NULL;
      dri2_surf->wl_buffer_lock[i] = 0;
   }

   for (i = 0; i < __DRI_BUFFER_COUNT; ++i)
      dri2_surf->dri_buffers[i] = NULL;

   dri2_surf->pending_buffer = NULL;
   dri2_surf->third_buffer = NULL;
   dri2_surf->frame_callback = NULL;
   dri2_surf->pending_buffer_callback = NULL;

   if (conf->AlphaSize == 0)
      dri2_surf->format = WL_DRM_FORMAT_XRGB8888;
   else
      dri2_surf->format = WL_DRM_FORMAT_ARGB8888;

   switch (type) {
   case EGL_WINDOW_BIT:
      dri2_surf->wl_win = (struct wl_egl_window *) window;

      dri2_surf->wl_win->private = dri2_surf;
      dri2_surf->wl_win->resize_callback = resize_callback;

      dri2_surf->base.Width =  -1;
      dri2_surf->base.Height = -1;
      break;
   default: 
      goto cleanup_surf;
   }

   dri2_surf->dri_drawable = 
      (*dri2_dpy->dri2->createNewDrawable) (dri2_dpy->dri_screen,
					    type == EGL_WINDOW_BIT ?
					    dri2_conf->dri_double_config : 
					    dri2_conf->dri_single_config,
					    dri2_surf);
   if (dri2_surf->dri_drawable == NULL) {
      _eglError(EGL_BAD_ALLOC, "dri2->createNewDrawable");
      goto cleanup_dri_drawable;
   }

   return &dri2_surf->base;

 cleanup_dri_drawable:
   dri2_dpy->core->destroyDrawable(dri2_surf->dri_drawable);
 cleanup_surf:
   free(dri2_surf);

   return NULL;
}

/**
 * Called via eglCreateWindowSurface(), drv->API.CreateWindowSurface().
 */
static _EGLSurface *
dri2_create_window_surface(_EGLDriver *drv, _EGLDisplay *disp,
			   _EGLConfig *conf, EGLNativeWindowType window,
			   const EGLint *attrib_list)
{
   return dri2_create_surface(drv, disp, EGL_WINDOW_BIT, conf,
			      window, attrib_list);
}

/**
 * Called via eglDestroySurface(), drv->API.DestroySurface().
 */
static EGLBoolean
dri2_destroy_surface(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surf)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
   int i;

   (void) drv;

   if (!_eglPutSurface(surf))
      return EGL_TRUE;

   (*dri2_dpy->core->destroyDrawable)(dri2_surf->dri_drawable);

   for (i = 0; i < WL_BUFFER_COUNT; ++i)
      if (dri2_surf->wl_drm_buffer[i])
         wl_buffer_destroy(dri2_surf->wl_drm_buffer[i]);

   for (i = 0; i < __DRI_BUFFER_COUNT; ++i)
      if (dri2_surf->dri_buffers[i])
         dri2_dpy->dri2->releaseBuffer(dri2_dpy->dri_screen,
                                       dri2_surf->dri_buffers[i]);

   if (dri2_surf->third_buffer) {
      dri2_dpy->dri2->releaseBuffer(dri2_dpy->dri_screen,
                                    dri2_surf->third_buffer);
   }

   if (dri2_surf->frame_callback)
      wl_callback_destroy(dri2_surf->frame_callback);

   if (dri2_surf->pending_buffer_callback)
      wl_callback_destroy(dri2_surf->pending_buffer_callback);


   if (dri2_surf->base.Type == EGL_WINDOW_BIT) {
      dri2_surf->wl_win->private = NULL;
      dri2_surf->wl_win->resize_callback = NULL;
   }

   free(surf);

   return EGL_TRUE;
}

static struct wl_buffer *
wayland_create_buffer(struct dri2_egl_surface *dri2_surf,
                      __DRIbuffer *buffer)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   struct wl_buffer *buf;

   buf = wl_drm_create_buffer(dri2_dpy->wl_drm, buffer->name,
                              dri2_surf->base.Width, dri2_surf->base.Height,
                              buffer->pitch, dri2_surf->format);
   wl_buffer_add_listener(buf, &wl_buffer_listener, dri2_surf);

   return buf;
}

static void
dri2_process_back_buffer(struct dri2_egl_surface *dri2_surf, unsigned format)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);

   (void) format;

   switch (dri2_surf->base.Type) {
   case EGL_WINDOW_BIT:
      /* allocate a front buffer for our double-buffered window*/
      if (dri2_surf->dri_buffers[__DRI_BUFFER_FRONT_LEFT] != NULL)
         break;
      dri2_surf->dri_buffers[__DRI_BUFFER_FRONT_LEFT] = 
         dri2_dpy->dri2->allocateBuffer(dri2_dpy->dri_screen,
               __DRI_BUFFER_FRONT_LEFT, format,
               dri2_surf->base.Width, dri2_surf->base.Height);
      break;
   default:
      break;
   }
}

static void
dri2_release_pending_buffer(void *data,
			    struct wl_callback *callback, uint32_t time)
{
   struct dri2_egl_surface *dri2_surf = data;
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);

   /* FIXME: print internal error */
   if (!dri2_surf->pending_buffer)
      return;
   
   dri2_dpy->dri2->releaseBuffer(dri2_dpy->dri_screen,
                                 dri2_surf->pending_buffer);
   dri2_surf->pending_buffer = NULL;

   wl_callback_destroy(callback);
   dri2_surf->pending_buffer_callback = NULL;
}

static const struct wl_callback_listener release_buffer_listener = {
   dri2_release_pending_buffer
};

static void
dri2_release_buffers(struct dri2_egl_surface *dri2_surf)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   struct wl_callback *callback;
   int i;

   if (dri2_surf->third_buffer) {
      dri2_dpy->dri2->releaseBuffer(dri2_dpy->dri_screen,
                                    dri2_surf->third_buffer);
      dri2_surf->third_buffer = NULL;
   }

   for (i = 0; i < __DRI_BUFFER_COUNT; ++i) {
      if (dri2_surf->dri_buffers[i]) {
         switch (i) {
         case __DRI_BUFFER_FRONT_LEFT:
            if (dri2_surf->pending_buffer)
               roundtrip(dri2_dpy);
            dri2_surf->pending_buffer = dri2_surf->dri_buffers[i];
            callback = wl_display_sync(dri2_dpy->wl_dpy);
	    wl_callback_add_listener(callback,
				     &release_buffer_listener, dri2_surf);
            wl_proxy_set_queue((struct wl_proxy *) callback,
                               dri2_dpy->wl_queue);
            dri2_surf->pending_buffer_callback = callback;
            break;
         default:
            dri2_dpy->dri2->releaseBuffer(dri2_dpy->dri_screen,
                                          dri2_surf->dri_buffers[i]);
            break;
         }
         dri2_surf->dri_buffers[i] = NULL;
      }
   }
}

static inline void
pointer_swap(const void **p1, const void **p2)
{
   const void *tmp = *p1;
   *p1 = *p2;
   *p2 = tmp;
}

static void
destroy_third_buffer(struct dri2_egl_surface *dri2_surf)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);

   if (dri2_surf->third_buffer == NULL)
      return;

   dri2_dpy->dri2->releaseBuffer(dri2_dpy->dri_screen,
                                 dri2_surf->third_buffer);
   dri2_surf->third_buffer = NULL;

   if (dri2_surf->wl_drm_buffer[WL_BUFFER_THIRD])
      wl_buffer_destroy(dri2_surf->wl_drm_buffer[WL_BUFFER_THIRD]);
   dri2_surf->wl_drm_buffer[WL_BUFFER_THIRD] = NULL;
   dri2_surf->wl_buffer_lock[WL_BUFFER_THIRD] = 0;
}

static void
swap_wl_buffers(struct dri2_egl_surface *dri2_surf,
                enum wayland_buffer_type a, enum wayland_buffer_type b)
{
   int tmp;

   tmp = dri2_surf->wl_buffer_lock[a];
   dri2_surf->wl_buffer_lock[a] = dri2_surf->wl_buffer_lock[b];
   dri2_surf->wl_buffer_lock[b] = tmp;
      
   pointer_swap((const void **) &dri2_surf->wl_drm_buffer[a],
                (const void **) &dri2_surf->wl_drm_buffer[b]);
}

static void
swap_back_and_third(struct dri2_egl_surface *dri2_surf)
{
   if (dri2_surf->wl_buffer_lock[WL_BUFFER_THIRD])
      destroy_third_buffer(dri2_surf);

   pointer_swap((const void **) &dri2_surf->dri_buffers[__DRI_BUFFER_BACK_LEFT],
                (const void **) &dri2_surf->third_buffer);

   swap_wl_buffers(dri2_surf, WL_BUFFER_BACK, WL_BUFFER_THIRD);
}

static void
dri2_prior_buffer_creation(struct dri2_egl_surface *dri2_surf,
                           unsigned int type)
{
   switch (type) {
   case __DRI_BUFFER_BACK_LEFT:
         if (dri2_surf->wl_buffer_lock[WL_BUFFER_BACK])
            swap_back_and_third(dri2_surf);
         else if (dri2_surf->third_buffer)
            destroy_third_buffer(dri2_surf);
         break;
   default:
         break;

   }
}

static __DRIbuffer *
dri2_get_buffers_with_format(__DRIdrawable * driDrawable,
			     int *width, int *height,
			     unsigned int *attachments, int count,
			     int *out_count, void *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   int i;

   /* There might be a buffer release already queued that wasn't processed */
   wl_display_dispatch_queue_pending(dri2_dpy->wl_dpy, dri2_dpy->wl_queue);

   if (dri2_surf->base.Type == EGL_WINDOW_BIT &&
       (dri2_surf->base.Width != dri2_surf->wl_win->width || 
        dri2_surf->base.Height != dri2_surf->wl_win->height)) {

      dri2_release_buffers(dri2_surf);

      dri2_surf->base.Width  = dri2_surf->wl_win->width;
      dri2_surf->base.Height = dri2_surf->wl_win->height;
      dri2_surf->dx = dri2_surf->wl_win->dx;
      dri2_surf->dy = dri2_surf->wl_win->dy;

      for (i = 0; i < WL_BUFFER_COUNT; ++i) {
         if (dri2_surf->wl_drm_buffer[i])
            wl_buffer_destroy(dri2_surf->wl_drm_buffer[i]);
         dri2_surf->wl_drm_buffer[i]  = NULL;
         dri2_surf->wl_buffer_lock[i] = 0;
      }
   }

   dri2_surf->buffer_count = 0;
   for (i = 0; i < 2*count; i+=2) {
      assert(attachments[i] < __DRI_BUFFER_COUNT);
      assert(dri2_surf->buffer_count < 5);

      dri2_prior_buffer_creation(dri2_surf, attachments[i]);

      if (dri2_surf->dri_buffers[attachments[i]] == NULL) {

         dri2_surf->dri_buffers[attachments[i]] =
            dri2_dpy->dri2->allocateBuffer(dri2_dpy->dri_screen,
                  attachments[i], attachments[i+1],
                  dri2_surf->base.Width, dri2_surf->base.Height);

         if (!dri2_surf->dri_buffers[attachments[i]]) 
            continue;

         if (attachments[i] == __DRI_BUFFER_BACK_LEFT)
            dri2_process_back_buffer(dri2_surf, attachments[i+1]);
      }

      memcpy(&dri2_surf->buffers[dri2_surf->buffer_count],
             dri2_surf->dri_buffers[attachments[i]],
             sizeof(__DRIbuffer));

      dri2_surf->buffer_count++;
   }

   assert(dri2_surf->dri_buffers[__DRI_BUFFER_BACK_LEFT]);

   *out_count = dri2_surf->buffer_count;
   if (dri2_surf->buffer_count == 0)
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

   /* FIXME: Does EGL support front buffer rendering at all? */

#if 0
   struct dri2_egl_surface *dri2_surf = loaderPrivate;

   dri2WaitGL(dri2_surf);
#else
   (void) loaderPrivate;
#endif
}

static void
wayland_frame_callback(void *data, struct wl_callback *callback, uint32_t time)
{
   struct dri2_egl_surface *dri2_surf = data;

   dri2_surf->frame_callback = NULL;
   wl_callback_destroy(callback);
}

static const struct wl_callback_listener frame_listener = {
	wayland_frame_callback
};

/**
 * Called via eglSwapBuffers(), drv->API.SwapBuffers().
 */
static EGLBoolean
dri2_swap_buffers(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *draw)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);
   struct dri2_egl_driver *dri2_drv = dri2_egl_driver(drv);
   int ret = 0;

   while (dri2_surf->frame_callback && ret != -1)
      ret = wl_display_dispatch_queue(dri2_dpy->wl_dpy, dri2_dpy->wl_queue);
   if (ret < 0)
      return EGL_FALSE;

   dri2_surf->frame_callback = wl_surface_frame(dri2_surf->wl_win->surface);
   wl_callback_add_listener(dri2_surf->frame_callback,
                            &frame_listener, dri2_surf);
   wl_proxy_set_queue((struct wl_proxy *) dri2_surf->frame_callback,
                      dri2_dpy->wl_queue);

   if (dri2_surf->base.Type == EGL_WINDOW_BIT) {
      pointer_swap(
	    (const void **) &dri2_surf->dri_buffers[__DRI_BUFFER_FRONT_LEFT],
	    (const void **) &dri2_surf->dri_buffers[__DRI_BUFFER_BACK_LEFT]);

      dri2_surf->dri_buffers[__DRI_BUFFER_FRONT_LEFT]->attachment = 
	 __DRI_BUFFER_FRONT_LEFT;
      dri2_surf->dri_buffers[__DRI_BUFFER_BACK_LEFT]->attachment = 
	 __DRI_BUFFER_BACK_LEFT;

      swap_wl_buffers(dri2_surf, WL_BUFFER_FRONT, WL_BUFFER_BACK);

      if (!dri2_surf->wl_drm_buffer[WL_BUFFER_FRONT])
	 dri2_surf->wl_drm_buffer[WL_BUFFER_FRONT] =
	    wayland_create_buffer(dri2_surf,
		  dri2_surf->dri_buffers[__DRI_BUFFER_FRONT_LEFT]);

      wl_surface_attach(dri2_surf->wl_win->surface,
	    dri2_surf->wl_drm_buffer[WL_BUFFER_FRONT],
	    dri2_surf->dx, dri2_surf->dy);
      dri2_surf->wl_buffer_lock[WL_BUFFER_FRONT] = 1;

      dri2_surf->wl_win->attached_width  = dri2_surf->base.Width;
      dri2_surf->wl_win->attached_height = dri2_surf->base.Height;
      /* reset resize growing parameters */
      dri2_surf->dx = 0;
      dri2_surf->dy = 0;

      wl_surface_damage(dri2_surf->wl_win->surface, 0, 0,
	    dri2_surf->base.Width, dri2_surf->base.Height);

      wl_surface_commit(dri2_surf->wl_win->surface);
   }

   _EGLContext *ctx;
   if (dri2_drv->glFlush) {
      ctx = _eglGetCurrentContext();
      if (ctx && ctx->DrawSurface == &dri2_surf->base)
         dri2_drv->glFlush();
   }

   (*dri2_dpy->flush->flush)(dri2_surf->dri_drawable);
   (*dri2_dpy->flush->invalidate)(dri2_surf->dri_drawable);

   return EGL_TRUE;
}

static int
dri2_wayland_authenticate(_EGLDisplay *disp, uint32_t id)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   int ret = 0;

   dri2_dpy->authenticated = 0;

   wl_drm_authenticate(dri2_dpy->wl_drm, id);
   if (roundtrip(dri2_dpy) < 0)
      ret = -1;

   if (!dri2_dpy->authenticated)
      ret = -1;

   /* reset authenticated */
   dri2_dpy->authenticated = 1;

   return ret;
}

/**
 * Called via eglTerminate(), drv->API.Terminate().
 */
static EGLBoolean
dri2_terminate(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);

   _eglReleaseDisplayResources(drv, disp);
   _eglCleanupDisplay(disp);

   dri2_dpy->core->destroyScreen(dri2_dpy->dri_screen);
   close(dri2_dpy->fd);
   dlclose(dri2_dpy->driver);
   free(dri2_dpy->driver_name);
   free(dri2_dpy->device_name);
   wl_drm_destroy(dri2_dpy->wl_drm);
   if (dri2_dpy->own_device)
      wl_display_disconnect(dri2_dpy->wl_dpy);
   free(dri2_dpy);
   disp->DriverData = NULL;

   return EGL_TRUE;
}

static void
drm_handle_device(void *data, struct wl_drm *drm, const char *device)
{
   struct dri2_egl_display *dri2_dpy = data;
   drm_magic_t magic;

   dri2_dpy->device_name = strdup(device);
   if (!dri2_dpy->device_name)
      return;

#ifdef O_CLOEXEC
   dri2_dpy->fd = open(dri2_dpy->device_name, O_RDWR | O_CLOEXEC);
   if (dri2_dpy->fd == -1 && errno == EINVAL)
#endif
   {
      dri2_dpy->fd = open(dri2_dpy->device_name, O_RDWR);
      if (dri2_dpy->fd != -1)
         fcntl(dri2_dpy->fd, F_SETFD, fcntl(dri2_dpy->fd, F_GETFD) |
            FD_CLOEXEC);
   }
   if (dri2_dpy->fd == -1) {
      _eglLog(_EGL_WARNING, "wayland-egl: could not open %s (%s)",
	      dri2_dpy->device_name, strerror(errno));
      return;
   }

   drmGetMagic(dri2_dpy->fd, &magic);
   wl_drm_authenticate(dri2_dpy->wl_drm, magic);
}

static void
drm_handle_format(void *data, struct wl_drm *drm, uint32_t format)
{
   struct dri2_egl_display *dri2_dpy = data;

   switch (format) {
   case WL_DRM_FORMAT_ARGB8888:
      dri2_dpy->formats |= HAS_ARGB8888;
      break;
   case WL_DRM_FORMAT_XRGB8888:
      dri2_dpy->formats |= HAS_XRGB8888;
      break;
   }
}

static void
drm_handle_authenticated(void *data, struct wl_drm *drm)
{
   struct dri2_egl_display *dri2_dpy = data;

   dri2_dpy->authenticated = 1;
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
   struct dri2_egl_display *dri2_dpy = data;

   if (strcmp(interface, "wl_drm") == 0) {
      dri2_dpy->wl_drm =
         wl_registry_bind(registry, name, &wl_drm_interface, 1);
      wl_drm_add_listener(dri2_dpy->wl_drm, &drm_listener, dri2_dpy);
   }
}

static const struct wl_registry_listener registry_listener = {
	registry_handle_global
};

EGLBoolean
dri2_initialize_wayland(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy;
   const __DRIconfig *config;
   uint32_t types;
   int i;
   static const unsigned int argb_masks[4] =
      { 0xff0000, 0xff00, 0xff, 0xff000000 };
   static const unsigned int rgb_masks[4] = { 0xff0000, 0xff00, 0xff, 0 };

   drv->API.CreateWindowSurface = dri2_create_window_surface;
   drv->API.DestroySurface = dri2_destroy_surface;
   drv->API.SwapBuffers = dri2_swap_buffers;
   drv->API.Terminate = dri2_terminate;

   dri2_dpy = malloc(sizeof *dri2_dpy);
   if (!dri2_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   memset(dri2_dpy, 0, sizeof *dri2_dpy);

   disp->DriverData = (void *) dri2_dpy;
   if (disp->PlatformDisplay == NULL) {
      dri2_dpy->wl_dpy = wl_display_connect(NULL);
      if (dri2_dpy->wl_dpy == NULL)
         goto cleanup_dpy;
      dri2_dpy->own_device = 1;
   } else {
      dri2_dpy->wl_dpy = disp->PlatformDisplay;
   }

   dri2_dpy->wl_queue = wl_display_create_queue(dri2_dpy->wl_dpy);
   dri2_dpy->wl_registry = wl_display_get_registry(dri2_dpy->wl_dpy);
   wl_proxy_set_queue((struct wl_proxy *) dri2_dpy->wl_registry,
                      dri2_dpy->wl_queue);
   wl_registry_add_listener(dri2_dpy->wl_registry,
                            &registry_listener, dri2_dpy);
   if (roundtrip(dri2_dpy) < 0 || dri2_dpy->wl_drm == NULL)
      goto cleanup_dpy;

   if (roundtrip(dri2_dpy) < 0 || dri2_dpy->fd == -1)
      goto cleanup_drm;

   if (roundtrip(dri2_dpy) < 0 || !dri2_dpy->authenticated)
      goto cleanup_fd;

   dri2_dpy->driver_name = dri2_get_driver_for_fd(dri2_dpy->fd);
   if (dri2_dpy->driver_name == NULL) {
      _eglError(EGL_BAD_ALLOC, "DRI2: failed to get driver name");
      goto cleanup_fd;
   }

   if (!dri2_load_driver(disp))
      goto cleanup_driver_name;

   dri2_dpy->dri2_loader_extension.base.name = __DRI_DRI2_LOADER;
   dri2_dpy->dri2_loader_extension.base.version = 3;
   dri2_dpy->dri2_loader_extension.getBuffers = dri2_get_buffers;
   dri2_dpy->dri2_loader_extension.flushFrontBuffer = dri2_flush_front_buffer;
   dri2_dpy->dri2_loader_extension.getBuffersWithFormat =
      dri2_get_buffers_with_format;
      
   dri2_dpy->extensions[0] = &dri2_dpy->dri2_loader_extension.base;
   dri2_dpy->extensions[1] = &image_lookup_extension.base;
   dri2_dpy->extensions[2] = &use_invalidate.base;
   dri2_dpy->extensions[3] = NULL;

   if (!dri2_create_screen(disp))
      goto cleanup_driver;

   types = EGL_WINDOW_BIT;
   for (i = 0; dri2_dpy->driver_configs[i]; i++) {
      config = dri2_dpy->driver_configs[i];
      if (dri2_dpy->formats & HAS_XRGB8888)
	 dri2_add_config(disp, config, i + 1, 0, types, NULL, rgb_masks);
      if (dri2_dpy->formats & HAS_ARGB8888)
	 dri2_add_config(disp, config, i + 1, 0, types, NULL, argb_masks);
   }

   disp->Extensions.WL_bind_wayland_display = EGL_TRUE;
   dri2_dpy->authenticate = dri2_wayland_authenticate;

   /* we're supporting EGL 1.4 */
   disp->VersionMajor = 1;
   disp->VersionMinor = 4;

   return EGL_TRUE;

 cleanup_driver:
   dlclose(dri2_dpy->driver);
 cleanup_driver_name:
   free(dri2_dpy->driver_name);
 cleanup_fd:
   close(dri2_dpy->fd);
 cleanup_drm:
   free(dri2_dpy->device_name);
   wl_drm_destroy(dri2_dpy->wl_drm);
 cleanup_dpy:
   free(dri2_dpy);
   
   return EGL_FALSE;
}
