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
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <xf86drm.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "egl_dri2.h"

static void
swrastCreateDrawable(struct dri2_egl_display * dri2_dpy,
                     struct dri2_egl_surface * dri2_surf,
                     int depth)
{
   uint32_t           mask;
   const uint32_t     function = GXcopy;
   uint32_t           valgc[2];
   
   /* create GC's */
   dri2_surf->gc = xcb_generate_id(dri2_dpy->conn);
   mask = XCB_GC_FUNCTION;
   xcb_create_gc(dri2_dpy->conn, dri2_surf->gc, dri2_surf->drawable, mask, &function);

   dri2_surf->swapgc = xcb_generate_id(dri2_dpy->conn);
   mask = XCB_GC_FUNCTION | XCB_GC_GRAPHICS_EXPOSURES;
   valgc[0] = function;
   valgc[1] = False;
   xcb_create_gc(dri2_dpy->conn, dri2_surf->swapgc, dri2_surf->drawable, mask, valgc);
   dri2_surf->depth = depth;
   switch (depth) {
      case 32:
      case 24:
         dri2_surf->bytes_per_pixel = 4;
         break;
      case 16:
         dri2_surf->bytes_per_pixel = 2;
         break;
      case 8:
         dri2_surf->bytes_per_pixel = 1;
         break;
      case 0:
         dri2_surf->bytes_per_pixel = 0;
         break;
      default:
         _eglLog(_EGL_WARNING, "unsupported depth %d", depth);
   }
}

static void
swrastDestroyDrawable(struct dri2_egl_display * dri2_dpy,
                      struct dri2_egl_surface * dri2_surf)
{
   xcb_free_gc(dri2_dpy->conn, dri2_surf->gc);
   xcb_free_gc(dri2_dpy->conn, dri2_surf->swapgc);
}

static void
swrastGetDrawableInfo(__DRIdrawable * draw,
                      int *x, int *y, int *w, int *h,
                      void *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(dri2_surf->base.Resource.Display);

   xcb_get_geometry_cookie_t cookie;
   xcb_get_geometry_reply_t *reply;
   xcb_generic_error_t *error;

   *w = *h = 0;
   cookie = xcb_get_geometry (dri2_dpy->conn, dri2_surf->drawable);
   reply = xcb_get_geometry_reply (dri2_dpy->conn, cookie, &error);
   if (reply == NULL)
      return;

   if (error != NULL) {
      _eglLog(_EGL_WARNING, "error in xcb_get_geometry");
      free(error);
   } else {
      *w = reply->width;
      *h = reply->height;
   }
   free(reply);
}

static void
swrastPutImage(__DRIdrawable * draw, int op,
               int x, int y, int w, int h,
               char *data, void *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(dri2_surf->base.Resource.Display);

   xcb_gcontext_t gc;

   switch (op) {
   case __DRI_SWRAST_IMAGE_OP_DRAW:
      gc = dri2_surf->gc;
      break;
   case __DRI_SWRAST_IMAGE_OP_SWAP:
      gc = dri2_surf->swapgc;
      break;
   default:
      return;
   }

   xcb_put_image(dri2_dpy->conn, XCB_IMAGE_FORMAT_Z_PIXMAP, dri2_surf->drawable,
                 gc, w, h, x, y, 0, dri2_surf->depth,
                 w*h*dri2_surf->bytes_per_pixel, (const uint8_t *)data);
}

static void
swrastGetImage(__DRIdrawable * read,
               int x, int y, int w, int h,
               char *data, void *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(dri2_surf->base.Resource.Display);

   xcb_get_image_cookie_t cookie;
   xcb_get_image_reply_t *reply;
   xcb_generic_error_t *error;

   cookie = xcb_get_image (dri2_dpy->conn, XCB_IMAGE_FORMAT_Z_PIXMAP,
                           dri2_surf->drawable, x, y, w, h, ~0);
   reply = xcb_get_image_reply (dri2_dpy->conn, cookie, &error);
   if (reply == NULL)
      return;

   if (error != NULL) {
      _eglLog(_EGL_WARNING, "error in xcb_get_image");
      free(error);
   } else {
      uint32_t bytes = xcb_get_image_data_length(reply);
      uint8_t *idata = xcb_get_image_data(reply);
      memcpy(data, idata, bytes);
   }
   free(reply);
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
   xcb_get_geometry_cookie_t cookie;
   xcb_get_geometry_reply_t *reply;
   xcb_screen_iterator_t s;
   xcb_generic_error_t *error;

   (void) drv;

   dri2_surf = malloc(sizeof *dri2_surf);
   if (!dri2_surf) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
      return NULL;
   }
   
   if (!_eglInitSurface(&dri2_surf->base, disp, type, conf, attrib_list))
      goto cleanup_surf;

   dri2_surf->region = XCB_NONE;
   if (type == EGL_PBUFFER_BIT) {
      dri2_surf->drawable = xcb_generate_id(dri2_dpy->conn);
      s = xcb_setup_roots_iterator(xcb_get_setup(dri2_dpy->conn));
      xcb_create_pixmap(dri2_dpy->conn, conf->BufferSize,
			dri2_surf->drawable, s.data->root,
			dri2_surf->base.Width, dri2_surf->base.Height);
   } else {
      dri2_surf->drawable = window;
   }

   if (dri2_dpy->dri2) {
      dri2_surf->dri_drawable = 
	 (*dri2_dpy->dri2->createNewDrawable) (dri2_dpy->dri_screen,
					       type == EGL_WINDOW_BIT ?
					       dri2_conf->dri_double_config : 
					       dri2_conf->dri_single_config,
					       dri2_surf);
   } else {
      assert(dri2_dpy->swrast);
      dri2_surf->dri_drawable = 
	 (*dri2_dpy->swrast->createNewDrawable) (dri2_dpy->dri_screen,
						 dri2_conf->dri_double_config,
						 dri2_surf);
   }

   if (dri2_surf->dri_drawable == NULL) {
      _eglError(EGL_BAD_ALLOC, "dri2->createNewDrawable");
      goto cleanup_pixmap;
   }
   
   if (dri2_dpy->dri2) {
      xcb_dri2_create_drawable (dri2_dpy->conn, dri2_surf->drawable);
   } else {
      swrastCreateDrawable(dri2_dpy, dri2_surf, _eglGetConfigKey(conf, EGL_BUFFER_SIZE));
   }

   if (type != EGL_PBUFFER_BIT) {
      cookie = xcb_get_geometry (dri2_dpy->conn, dri2_surf->drawable);
      reply = xcb_get_geometry_reply (dri2_dpy->conn, cookie, &error);
      if (reply == NULL || error != NULL) {
	 _eglError(EGL_BAD_ALLOC, "xcb_get_geometry");
	 free(error);
	 goto cleanup_dri_drawable;
      }

      dri2_surf->base.Width = reply->width;
      dri2_surf->base.Height = reply->height;
      free(reply);
   }

   /* we always copy the back buffer to front */
   dri2_surf->base.PostSubBufferSupportedNV = EGL_TRUE;

   return &dri2_surf->base;

 cleanup_dri_drawable:
   dri2_dpy->core->destroyDrawable(dri2_surf->dri_drawable);
 cleanup_pixmap:
   if (type == EGL_PBUFFER_BIT)
      xcb_free_pixmap(dri2_dpy->conn, dri2_surf->drawable);
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

static _EGLSurface *
dri2_create_pixmap_surface(_EGLDriver *drv, _EGLDisplay *disp,
			   _EGLConfig *conf, EGLNativePixmapType pixmap,
			   const EGLint *attrib_list)
{
   return dri2_create_surface(drv, disp, EGL_PIXMAP_BIT, conf,
			      pixmap, attrib_list);
}

static _EGLSurface *
dri2_create_pbuffer_surface(_EGLDriver *drv, _EGLDisplay *disp,
			    _EGLConfig *conf, const EGLint *attrib_list)
{
   return dri2_create_surface(drv, disp, EGL_PBUFFER_BIT, conf,
			      XCB_WINDOW_NONE, attrib_list);
}

static EGLBoolean
dri2_destroy_surface(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surf)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);

   (void) drv;

   if (!_eglPutSurface(surf))
      return EGL_TRUE;

   (*dri2_dpy->core->destroyDrawable)(dri2_surf->dri_drawable);
   
   if (dri2_dpy->dri2) {
      xcb_dri2_destroy_drawable (dri2_dpy->conn, dri2_surf->drawable);
   } else {
      assert(dri2_dpy->swrast);
      swrastDestroyDrawable(dri2_dpy, dri2_surf);
   }

   if (surf->Type == EGL_PBUFFER_BIT)
      xcb_free_pixmap (dri2_dpy->conn, dri2_surf->drawable);

   free(surf);

   return EGL_TRUE;
}

/**
 * Process list of buffer received from the server
 *
 * Processes the list of buffers received in a reply from the server to either
 * \c DRI2GetBuffers or \c DRI2GetBuffersWithFormat.
 */
static void
dri2_process_buffers(struct dri2_egl_surface *dri2_surf,
		     xcb_dri2_dri2_buffer_t *buffers, unsigned count)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   xcb_rectangle_t rectangle;
   unsigned i;

   dri2_surf->buffer_count = count;
   dri2_surf->have_fake_front = 0;

   /* This assumes the DRI2 buffer attachment tokens matches the
    * __DRIbuffer tokens. */
   for (i = 0; i < count; i++) {
      dri2_surf->buffers[i].attachment = buffers[i].attachment;
      dri2_surf->buffers[i].name = buffers[i].name;
      dri2_surf->buffers[i].pitch = buffers[i].pitch;
      dri2_surf->buffers[i].cpp = buffers[i].cpp;
      dri2_surf->buffers[i].flags = buffers[i].flags;

      /* We only use the DRI drivers single buffer configs.  This
       * means that if we try to render to a window, DRI2 will give us
       * the fake front buffer, which we'll use as a back buffer.
       * Note that EGL doesn't require that several clients rendering
       * to the same window must see the same aux buffers. */
      if (dri2_surf->buffers[i].attachment == __DRI_BUFFER_FAKE_FRONT_LEFT)
         dri2_surf->have_fake_front = 1;
   }

   if (dri2_surf->region != XCB_NONE)
      xcb_xfixes_destroy_region(dri2_dpy->conn, dri2_surf->region);

   rectangle.x = 0;
   rectangle.y = 0;
   rectangle.width = dri2_surf->base.Width;
   rectangle.height = dri2_surf->base.Height;
   dri2_surf->region = xcb_generate_id(dri2_dpy->conn);
   xcb_xfixes_create_region(dri2_dpy->conn, dri2_surf->region, 1, &rectangle);
}

static __DRIbuffer *
dri2_get_buffers(__DRIdrawable * driDrawable,
		int *width, int *height,
		unsigned int *attachments, int count,
		int *out_count, void *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   xcb_dri2_dri2_buffer_t *buffers;
   xcb_dri2_get_buffers_reply_t *reply;
   xcb_dri2_get_buffers_cookie_t cookie;

   (void) driDrawable;

   cookie = xcb_dri2_get_buffers_unchecked (dri2_dpy->conn,
					    dri2_surf->drawable,
					    count, count, attachments);
   reply = xcb_dri2_get_buffers_reply (dri2_dpy->conn, cookie, NULL);
   buffers = xcb_dri2_get_buffers_buffers (reply);
   if (buffers == NULL)
      return NULL;

   *out_count = reply->count;
   dri2_surf->base.Width = *width = reply->width;
   dri2_surf->base.Height = *height = reply->height;
   dri2_process_buffers(dri2_surf, buffers, *out_count);

   free(reply);

   return dri2_surf->buffers;
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
   xcb_dri2_dri2_buffer_t *buffers;
   xcb_dri2_get_buffers_with_format_reply_t *reply;
   xcb_dri2_get_buffers_with_format_cookie_t cookie;
   xcb_dri2_attach_format_t *format_attachments;

   (void) driDrawable;

   format_attachments = (xcb_dri2_attach_format_t *) attachments;
   cookie = xcb_dri2_get_buffers_with_format_unchecked (dri2_dpy->conn,
							dri2_surf->drawable,
							count, count,
							format_attachments);

   reply = xcb_dri2_get_buffers_with_format_reply (dri2_dpy->conn,
						   cookie, NULL);
   if (reply == NULL)
      return NULL;

   buffers = xcb_dri2_get_buffers_with_format_buffers (reply);
   dri2_surf->base.Width = *width = reply->width;
   dri2_surf->base.Height = *height = reply->height;
   *out_count = reply->count;
   dri2_process_buffers(dri2_surf, buffers, *out_count);

   free(reply);

   return dri2_surf->buffers;
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

static char *
dri2_strndup(const char *s, int length)
{
   char *d;

   d = malloc(length + 1);
   if (d == NULL)
      return NULL;

   memcpy(d, s, length);
   d[length] = '\0';

   return d;
}

static EGLBoolean
dri2_connect(struct dri2_egl_display *dri2_dpy)
{
   xcb_xfixes_query_version_reply_t *xfixes_query;
   xcb_xfixes_query_version_cookie_t xfixes_query_cookie;
   xcb_dri2_query_version_reply_t *dri2_query;
   xcb_dri2_query_version_cookie_t dri2_query_cookie;
   xcb_dri2_connect_reply_t *connect;
   xcb_dri2_connect_cookie_t connect_cookie;
   xcb_generic_error_t *error;
   xcb_screen_iterator_t s;
   char *driver_name, *device_name;
   const xcb_query_extension_reply_t *extension;

   xcb_prefetch_extension_data (dri2_dpy->conn, &xcb_xfixes_id);
   xcb_prefetch_extension_data (dri2_dpy->conn, &xcb_dri2_id);

   extension = xcb_get_extension_data(dri2_dpy->conn, &xcb_xfixes_id);
   if (!(extension && extension->present))
      return EGL_FALSE;

   extension = xcb_get_extension_data(dri2_dpy->conn, &xcb_dri2_id);
   if (!(extension && extension->present))
      return EGL_FALSE;

   xfixes_query_cookie = xcb_xfixes_query_version(dri2_dpy->conn,
						  XCB_XFIXES_MAJOR_VERSION,
						  XCB_XFIXES_MINOR_VERSION);

   dri2_query_cookie = xcb_dri2_query_version (dri2_dpy->conn,
					       XCB_DRI2_MAJOR_VERSION,
					       XCB_DRI2_MINOR_VERSION);

   s = xcb_setup_roots_iterator(xcb_get_setup(dri2_dpy->conn));
   connect_cookie = xcb_dri2_connect_unchecked (dri2_dpy->conn,
						s.data->root,
						XCB_DRI2_DRIVER_TYPE_DRI);

   xfixes_query =
      xcb_xfixes_query_version_reply (dri2_dpy->conn,
				      xfixes_query_cookie, &error);
   if (xfixes_query == NULL ||
       error != NULL || xfixes_query->major_version < 2) {
      _eglLog(_EGL_WARNING, "DRI2: failed to query xfixes version");
      free(error);
      return EGL_FALSE;
   }
   free(xfixes_query);

   dri2_query =
      xcb_dri2_query_version_reply (dri2_dpy->conn, dri2_query_cookie, &error);
   if (dri2_query == NULL || error != NULL) {
      _eglLog(_EGL_WARNING, "DRI2: failed to query version");
      free(error);
      return EGL_FALSE;
   }
   dri2_dpy->dri2_major = dri2_query->major_version;
   dri2_dpy->dri2_minor = dri2_query->minor_version;
   free(dri2_query);

   connect = xcb_dri2_connect_reply (dri2_dpy->conn, connect_cookie, NULL);
   if (connect == NULL ||
       connect->driver_name_length + connect->device_name_length == 0) {
      _eglLog(_EGL_WARNING, "DRI2: failed to authenticate");
      return EGL_FALSE;
   }

   driver_name = xcb_dri2_connect_driver_name (connect);
   dri2_dpy->driver_name =
      dri2_strndup(driver_name,
		   xcb_dri2_connect_driver_name_length (connect));

#if XCB_DRI2_CONNECT_DEVICE_NAME_BROKEN
   device_name = driver_name + ((connect->driver_name_length + 3) & ~3);
#else
   device_name = xcb_dri2_connect_device_name (connect);
#endif

   dri2_dpy->device_name =
      dri2_strndup(device_name,
		   xcb_dri2_connect_device_name_length (connect));

   if (dri2_dpy->device_name == NULL || dri2_dpy->driver_name == NULL) {
      free(dri2_dpy->device_name);
      free(dri2_dpy->driver_name);
      free(connect);
      return EGL_FALSE;
   }
   free(connect);

   return EGL_TRUE;
}

static int
dri2_x11_authenticate(_EGLDisplay *disp, uint32_t id)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   xcb_dri2_authenticate_reply_t *authenticate;
   xcb_dri2_authenticate_cookie_t authenticate_cookie;
   xcb_screen_iterator_t s;
   int ret = 0;

   s = xcb_setup_roots_iterator(xcb_get_setup(dri2_dpy->conn));
   authenticate_cookie =
      xcb_dri2_authenticate_unchecked(dri2_dpy->conn, s.data->root, id);
   authenticate =
      xcb_dri2_authenticate_reply(dri2_dpy->conn, authenticate_cookie, NULL);

   if (authenticate == NULL || !authenticate->authenticated)
      ret = -1;

   if (authenticate)
      free(authenticate);
   
   return ret;
}

static EGLBoolean
dri2_authenticate(_EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   drm_magic_t magic;

   if (drmGetMagic(dri2_dpy->fd, &magic)) {
      _eglLog(_EGL_WARNING, "DRI2: failed to get drm magic");
      return EGL_FALSE;
   }
   
   if (dri2_x11_authenticate(disp, magic) < 0) {
      _eglLog(_EGL_WARNING, "DRI2: failed to authenticate");
      return EGL_FALSE;
   }

   return EGL_TRUE;
}

static EGLBoolean
dri2_add_configs_for_visuals(struct dri2_egl_display *dri2_dpy,
			     _EGLDisplay *disp)
{
   xcb_screen_iterator_t s;
   xcb_depth_iterator_t d;
   xcb_visualtype_t *visuals;
   int i, j, id;
   EGLint surface_type;
   EGLint config_attrs[] = {
	   EGL_NATIVE_VISUAL_ID,   0,
	   EGL_NATIVE_VISUAL_TYPE, 0,
	   EGL_NONE
   };

   s = xcb_setup_roots_iterator(xcb_get_setup(dri2_dpy->conn));
   d = xcb_screen_allowed_depths_iterator(s.data);
   id = 1;

   surface_type =
      EGL_WINDOW_BIT |
      EGL_PIXMAP_BIT |
      EGL_PBUFFER_BIT |
      EGL_SWAP_BEHAVIOR_PRESERVED_BIT;

   while (d.rem > 0) {
      EGLBoolean class_added[6] = { 0, };

      visuals = xcb_depth_visuals(d.data);
      for (i = 0; i < xcb_depth_visuals_length(d.data); i++) {
	 if (class_added[visuals[i]._class])
	    continue;

	 class_added[visuals[i]._class] = EGL_TRUE;
	 for (j = 0; dri2_dpy->driver_configs[j]; j++) {
            config_attrs[1] = visuals[i].visual_id;
            config_attrs[3] = visuals[i]._class;

	    dri2_add_config(disp, dri2_dpy->driver_configs[j], id++,
			    d.data->depth, surface_type, config_attrs, NULL);
	 }
      }

      xcb_depth_next(&d);
   }

   if (!_eglGetArraySize(disp->Configs)) {
      _eglLog(_EGL_WARNING, "DRI2: failed to create any config");
      return EGL_FALSE;
   }

   return EGL_TRUE;
}

static EGLBoolean
dri2_copy_region(_EGLDriver *drv, _EGLDisplay *disp,
		 _EGLSurface *draw, xcb_xfixes_region_t region)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);
   enum xcb_dri2_attachment_t render_attachment;
   xcb_dri2_copy_region_cookie_t cookie;

   /* No-op for a pixmap or pbuffer surface */
   if (draw->Type == EGL_PIXMAP_BIT || draw->Type == EGL_PBUFFER_BIT)
      return EGL_TRUE;

#ifdef __DRI2_FLUSH
   if (dri2_dpy->flush)
      (*dri2_dpy->flush->flush)(dri2_surf->dri_drawable);
#endif

   if (dri2_surf->have_fake_front)
      render_attachment = XCB_DRI2_ATTACHMENT_BUFFER_FAKE_FRONT_LEFT;
   else
      render_attachment = XCB_DRI2_ATTACHMENT_BUFFER_BACK_LEFT;

   cookie = xcb_dri2_copy_region_unchecked(dri2_dpy->conn,
					   dri2_surf->drawable,
					   region,
					   XCB_DRI2_ATTACHMENT_BUFFER_FRONT_LEFT,
					   render_attachment);
   free(xcb_dri2_copy_region_reply(dri2_dpy->conn, cookie, NULL));

   return EGL_TRUE;
}

static int64_t
dri2_swap_buffers_msc(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *draw,
                      int64_t msc, int64_t divisor, int64_t remainder)
{
#if XCB_DRI2_MINOR_VERSION >= 3
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);
   uint32_t msc_hi = msc >> 32;
   uint32_t msc_lo = msc & 0xffffffff;
   uint32_t divisor_hi = divisor >> 32;
   uint32_t divisor_lo = divisor & 0xffffffff;
   uint32_t remainder_hi = remainder >> 32;
   uint32_t remainder_lo = remainder & 0xffffffff;
   xcb_dri2_swap_buffers_cookie_t cookie;
   xcb_dri2_swap_buffers_reply_t *reply;
   int64_t swap_count = -1;

   /* No-op for a pixmap or pbuffer surface */
   if (draw->Type == EGL_PIXMAP_BIT || draw->Type == EGL_PBUFFER_BIT)
      return 0;

   if (draw->SwapBehavior == EGL_BUFFER_PRESERVED || !dri2_dpy->swap_available)
      return dri2_copy_region(drv, disp, draw, dri2_surf->region) ? 0 : -1;

#ifdef __DRI2_FLUSH
   if (dri2_dpy->flush)
      (*dri2_dpy->flush->flush)(dri2_surf->dri_drawable);
#endif

   cookie = xcb_dri2_swap_buffers_unchecked(dri2_dpy->conn, dri2_surf->drawable,
                  msc_hi, msc_lo, divisor_hi, divisor_lo, remainder_hi, remainder_lo);

   reply = xcb_dri2_swap_buffers_reply(dri2_dpy->conn, cookie, NULL);

   if (reply) {
      swap_count = (((int64_t)reply->swap_hi) << 32) | reply->swap_lo;
      free(reply);
   }

#if __DRI2_FLUSH_VERSION >= 3
   /* If the server doesn't send invalidate events */
   if (dri2_dpy->invalidate_available && dri2_dpy->flush &&
       dri2_dpy->flush->base.version >= 3 && dri2_dpy->flush->invalidate)
      (*dri2_dpy->flush->invalidate)(dri2_surf->dri_drawable);
#endif

   return swap_count;
#else
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);

   return dri2_copy_region(drv, disp, draw, dri2_surf->region) ? 0 : -1;
#endif /* XCB_DRI2_MINOR_VERSION >= 3 */

}

static EGLBoolean
dri2_swap_buffers(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *draw)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);

   if (dri2_dpy->dri2) {
      return dri2_swap_buffers_msc(drv, disp, draw, 0, 0, 0) != -1;
   } else {
      assert(dri2_dpy->swrast);

      dri2_dpy->core->swapBuffers(dri2_surf->dri_drawable);
      return EGL_TRUE;
   }
}

static EGLBoolean
dri2_swap_buffers_region(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *draw,
			 EGLint numRects, const EGLint *rects)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);
   EGLBoolean ret;
   xcb_xfixes_region_t region;
   xcb_rectangle_t rectangles[16];
   int i;

   if (numRects > (int)ARRAY_SIZE(rectangles))
      return dri2_copy_region(drv, disp, draw, dri2_surf->region);

   for (i = 0; i < numRects; i++) {
      rectangles[i].x = rects[i * 4];
      rectangles[i].y = dri2_surf->base.Height - rects[i * 4 + 1] - rects[i * 4 + 3];
      rectangles[i].width = rects[i * 4 + 2];
      rectangles[i].height = rects[i * 4 + 3];
   }

   region = xcb_generate_id(dri2_dpy->conn);
   xcb_xfixes_create_region(dri2_dpy->conn, region, numRects, rectangles);
   ret = dri2_copy_region(drv, disp, draw, region);
   xcb_xfixes_destroy_region(dri2_dpy->conn, region);

   return ret;
}

static EGLBoolean
dri2_post_sub_buffer(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *draw,
		     EGLint x, EGLint y, EGLint width, EGLint height)
{
   const EGLint rect[4] = { x, y, width, height };

   if (x < 0 || y < 0 || width < 0 || height < 0)
      _eglError(EGL_BAD_PARAMETER, "eglPostSubBufferNV");

   return dri2_swap_buffers_region(drv, disp, draw, 1, rect);
}

static EGLBoolean
dri2_swap_interval(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surf, EGLint interval)
{
#if XCB_DRI2_MINOR_VERSION >= 3
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
#endif

   /* XXX Check vblank_mode here? */

   if (interval > surf->Config->MaxSwapInterval)
      interval = surf->Config->MaxSwapInterval;
   else if (interval < surf->Config->MinSwapInterval)
      interval = surf->Config->MinSwapInterval;

#if XCB_DRI2_MINOR_VERSION >= 3
   if (interval != surf->SwapInterval && dri2_dpy->swap_available)
      xcb_dri2_swap_interval(dri2_dpy->conn, dri2_surf->drawable, interval);
#endif

   surf->SwapInterval = interval;

   return EGL_TRUE;
}

static EGLBoolean
dri2_copy_buffers(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surf,
		  EGLNativePixmapType target)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
   xcb_gcontext_t gc;

   (void) drv;

   (*dri2_dpy->flush->flush)(dri2_surf->dri_drawable);

   gc = xcb_generate_id(dri2_dpy->conn);
   xcb_create_gc(dri2_dpy->conn, gc, target, 0, NULL);
   xcb_copy_area(dri2_dpy->conn,
		  dri2_surf->drawable,
		  target,
		  gc,
		  0, 0,
		  0, 0,
		  dri2_surf->base.Width,
		  dri2_surf->base.Height);
   xcb_free_gc(dri2_dpy->conn, gc);

   return EGL_TRUE;
}

static _EGLImage *
dri2_create_image_khr_pixmap(_EGLDisplay *disp, _EGLContext *ctx,
			     EGLClientBuffer buffer, const EGLint *attr_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img;
   unsigned int attachments[1];
   xcb_drawable_t drawable;
   xcb_dri2_get_buffers_cookie_t buffers_cookie;
   xcb_dri2_get_buffers_reply_t *buffers_reply;
   xcb_dri2_dri2_buffer_t *buffers;
   xcb_get_geometry_cookie_t geometry_cookie;
   xcb_get_geometry_reply_t *geometry_reply;
   xcb_generic_error_t *error;
   int stride, format;

   (void) ctx;

   drawable = (xcb_drawable_t) (uintptr_t) buffer;
   xcb_dri2_create_drawable (dri2_dpy->conn, drawable);
   attachments[0] = XCB_DRI2_ATTACHMENT_BUFFER_FRONT_LEFT;
   buffers_cookie =
      xcb_dri2_get_buffers_unchecked (dri2_dpy->conn,
				      drawable, 1, 1, attachments);
   geometry_cookie = xcb_get_geometry (dri2_dpy->conn, drawable);
   buffers_reply = xcb_dri2_get_buffers_reply (dri2_dpy->conn,
					       buffers_cookie, NULL);
   buffers = xcb_dri2_get_buffers_buffers (buffers_reply);
   if (buffers == NULL) {
      return NULL;
   }

   geometry_reply = xcb_get_geometry_reply (dri2_dpy->conn,
					    geometry_cookie, &error);
   if (geometry_reply == NULL || error != NULL) {
      _eglError(EGL_BAD_ALLOC, "xcb_get_geometry");
      free(error);
      free(buffers_reply);
      return NULL;
   }

   switch (geometry_reply->depth) {
   case 16:
      format = __DRI_IMAGE_FORMAT_RGB565;
      break;
   case 24:
      format = __DRI_IMAGE_FORMAT_XRGB8888;
      break;
   case 32:
      format = __DRI_IMAGE_FORMAT_ARGB8888;
      break;
   default:
      _eglError(EGL_BAD_PARAMETER,
		"dri2_create_image_khr: unsupported pixmap depth");
      free(buffers_reply);
      free(geometry_reply);
      return NULL;
   }

   dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      free(buffers_reply);
      free(geometry_reply);
      _eglError(EGL_BAD_ALLOC, "dri2_create_image_khr");
      return EGL_NO_IMAGE_KHR;
   }

   if (!_eglInitImage(&dri2_img->base, disp)) {
      free(buffers_reply);
      free(geometry_reply);
      free(dri2_img);
      return EGL_NO_IMAGE_KHR;
   }

   stride = buffers[0].pitch / buffers[0].cpp;
   dri2_img->dri_image =
      dri2_dpy->image->createImageFromName(dri2_dpy->dri_screen,
					   buffers_reply->width,
					   buffers_reply->height,
					   format,
					   buffers[0].name,
					   stride,
					   dri2_img);

   free(buffers_reply);
   free(geometry_reply);

   return &dri2_img->base;
}

static _EGLImage *
dri2_x11_create_image_khr(_EGLDriver *drv, _EGLDisplay *disp,
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

static EGLBoolean
dri2_initialize_x11_swrast(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy;

   drv->API.CreateWindowSurface = dri2_create_window_surface;
   drv->API.CreatePixmapSurface = dri2_create_pixmap_surface;
   drv->API.CreatePbufferSurface = dri2_create_pbuffer_surface;
   drv->API.DestroySurface = dri2_destroy_surface;
   drv->API.SwapBuffers = dri2_swap_buffers;
   drv->API.CopyBuffers = dri2_copy_buffers;

   drv->API.SwapBuffersRegionNOK = NULL;
   drv->API.CreateImageKHR  = NULL;
   drv->API.DestroyImageKHR = NULL;
   drv->API.CreateDRMImageMESA = NULL;
   drv->API.ExportDRMImageMESA = NULL;

   dri2_dpy = malloc(sizeof *dri2_dpy);
   if (!dri2_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   memset(dri2_dpy, 0, sizeof *dri2_dpy);

   disp->DriverData = (void *) dri2_dpy;
   if (disp->PlatformDisplay == NULL) {
      dri2_dpy->conn = xcb_connect(0, 0);
   } else {
      dri2_dpy->conn = XGetXCBConnection((Display *) disp->PlatformDisplay);
   }

   if (xcb_connection_has_error(dri2_dpy->conn)) {
      _eglLog(_EGL_WARNING, "DRI2: xcb_connect failed");
      goto cleanup_dpy;
   }

   if (!dri2_load_driver_swrast(disp))
      goto cleanup_conn;

   dri2_dpy->swrast_loader_extension.base.name = __DRI_SWRAST_LOADER;
   dri2_dpy->swrast_loader_extension.base.version = __DRI_SWRAST_LOADER_VERSION;
   dri2_dpy->swrast_loader_extension.getDrawableInfo = swrastGetDrawableInfo;
   dri2_dpy->swrast_loader_extension.putImage = swrastPutImage;
   dri2_dpy->swrast_loader_extension.getImage = swrastGetImage;

   dri2_dpy->extensions[0] = &dri2_dpy->swrast_loader_extension.base;
   dri2_dpy->extensions[1] = NULL;
   dri2_dpy->extensions[2] = NULL;

   if (!dri2_create_screen(disp))
      goto cleanup_driver;

   if (dri2_dpy->conn) {
      if (!dri2_add_configs_for_visuals(dri2_dpy, disp))
         goto cleanup_configs;
   }

   /* we're supporting EGL 1.4 */
   disp->VersionMajor = 1;
   disp->VersionMinor = 4;

   return EGL_TRUE;

 cleanup_configs:
   _eglCleanupDisplay(disp);
   dri2_dpy->core->destroyScreen(dri2_dpy->dri_screen);
 cleanup_driver:
   dlclose(dri2_dpy->driver);
 cleanup_conn:
   if (disp->PlatformDisplay == NULL)
      xcb_disconnect(dri2_dpy->conn);
 cleanup_dpy:
   free(dri2_dpy);

   return EGL_FALSE;
}


static EGLBoolean
dri2_initialize_x11_dri2(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy;

   drv->API.CreateWindowSurface = dri2_create_window_surface;
   drv->API.CreatePixmapSurface = dri2_create_pixmap_surface;
   drv->API.CreatePbufferSurface = dri2_create_pbuffer_surface;
   drv->API.DestroySurface = dri2_destroy_surface;
   drv->API.SwapBuffers = dri2_swap_buffers;
   drv->API.CopyBuffers = dri2_copy_buffers;
   drv->API.CreateImageKHR = dri2_x11_create_image_khr;
   drv->API.SwapBuffersRegionNOK = dri2_swap_buffers_region;
   drv->API.PostSubBufferNV = dri2_post_sub_buffer;
   drv->API.SwapInterval = dri2_swap_interval;

   dri2_dpy = malloc(sizeof *dri2_dpy);
   if (!dri2_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   memset(dri2_dpy, 0, sizeof *dri2_dpy);

   disp->DriverData = (void *) dri2_dpy;
   if (disp->PlatformDisplay == NULL) {
      dri2_dpy->conn = xcb_connect(0, 0);
   } else {
      dri2_dpy->conn = XGetXCBConnection((Display *) disp->PlatformDisplay);
   }

   if (xcb_connection_has_error(dri2_dpy->conn)) {
      _eglLog(_EGL_WARNING, "DRI2: xcb_connect failed");
      goto cleanup_dpy;
   }

   if (dri2_dpy->conn) {
      if (!dri2_connect(dri2_dpy))
	 goto cleanup_conn;
   }

   if (!dri2_load_driver(disp))
      goto cleanup_conn;

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
      _eglLog(_EGL_WARNING,
	      "DRI2: could not open %s (%s)", dri2_dpy->device_name,
              strerror(errno));
      goto cleanup_driver;
   }

   if (dri2_dpy->conn) {
      if (!dri2_authenticate(disp))
	 goto cleanup_fd;
   }

   if (dri2_dpy->dri2_minor >= 1) {
      dri2_dpy->dri2_loader_extension.base.name = __DRI_DRI2_LOADER;
      dri2_dpy->dri2_loader_extension.base.version = 3;
      dri2_dpy->dri2_loader_extension.getBuffers = dri2_get_buffers;
      dri2_dpy->dri2_loader_extension.flushFrontBuffer = dri2_flush_front_buffer;
      dri2_dpy->dri2_loader_extension.getBuffersWithFormat =
	 dri2_get_buffers_with_format;
   } else {
      dri2_dpy->dri2_loader_extension.base.name = __DRI_DRI2_LOADER;
      dri2_dpy->dri2_loader_extension.base.version = 2;
      dri2_dpy->dri2_loader_extension.getBuffers = dri2_get_buffers;
      dri2_dpy->dri2_loader_extension.flushFrontBuffer = dri2_flush_front_buffer;
      dri2_dpy->dri2_loader_extension.getBuffersWithFormat = NULL;
   }
      
   dri2_dpy->extensions[0] = &dri2_dpy->dri2_loader_extension.base;
   dri2_dpy->extensions[1] = &image_lookup_extension.base;
   dri2_dpy->extensions[2] = NULL;

#if XCB_DRI2_MINOR_VERSION >= 3
   dri2_dpy->swap_available = (dri2_dpy->dri2_minor >= 2);
   dri2_dpy->invalidate_available = (dri2_dpy->dri2_minor >= 3);
#endif

   if (!dri2_create_screen(disp))
      goto cleanup_fd;

   if (dri2_dpy->conn) {
      if (!dri2_add_configs_for_visuals(dri2_dpy, disp))
	 goto cleanup_configs;
   }

   disp->Extensions.KHR_image_pixmap = EGL_TRUE;
   disp->Extensions.NOK_swap_region = EGL_TRUE;
   disp->Extensions.NOK_texture_from_pixmap = EGL_TRUE;
   disp->Extensions.NV_post_sub_buffer = EGL_TRUE;

#ifdef HAVE_WAYLAND_PLATFORM
   disp->Extensions.WL_bind_wayland_display = EGL_TRUE;
#endif
   dri2_dpy->authenticate = dri2_x11_authenticate;

   /* we're supporting EGL 1.4 */
   disp->VersionMajor = 1;
   disp->VersionMinor = 4;

   return EGL_TRUE;

 cleanup_configs:
   _eglCleanupDisplay(disp);
   dri2_dpy->core->destroyScreen(dri2_dpy->dri_screen);
 cleanup_fd:
   close(dri2_dpy->fd);
 cleanup_driver:
   dlclose(dri2_dpy->driver);
 cleanup_conn:
   if (disp->PlatformDisplay == NULL)
      xcb_disconnect(dri2_dpy->conn);
 cleanup_dpy:
   free(dri2_dpy);

   return EGL_FALSE;
}

EGLBoolean
dri2_initialize_x11(_EGLDriver *drv, _EGLDisplay *disp)
{
   EGLBoolean initialized = EGL_TRUE;

   int x11_dri2_accel = (getenv("LIBGL_ALWAYS_SOFTWARE") == NULL);

   if (x11_dri2_accel) {
      if (!dri2_initialize_x11_dri2(drv, disp)) {
         initialized = dri2_initialize_x11_swrast(drv, disp);
      }
   } else {
      initialized = dri2_initialize_x11_swrast(drv, disp);
   }

   return initialized;
}

