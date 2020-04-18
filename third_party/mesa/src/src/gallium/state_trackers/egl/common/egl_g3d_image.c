/*
 * Mesa 3-D graphics library
 * Version:  7.8
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

#include "pipe/p_screen.h"
#include "util/u_memory.h"
#include "util/u_rect.h"
#include "util/u_inlines.h"
#include "eglcurrent.h"
#include "egllog.h"

#include "native.h"
#include "egl_g3d.h"
#include "egl_g3d_image.h"

/**
 * Reference and return the front left buffer of the native pixmap.
 */
static struct pipe_resource *
egl_g3d_reference_native_pixmap(_EGLDisplay *dpy, EGLNativePixmapType pix)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct native_surface *nsurf;
   struct pipe_resource *textures[NUM_NATIVE_ATTACHMENTS];
   enum native_attachment natt;

   nsurf = gdpy->native->create_pixmap_surface(gdpy->native, pix, NULL);
   if (!nsurf)
      return NULL;

   natt = NATIVE_ATTACHMENT_FRONT_LEFT;
   if (!nsurf->validate(nsurf, 1 << natt, NULL, textures, NULL, NULL))
      textures[natt] = NULL;

   nsurf->destroy(nsurf);

   return textures[natt];
}

#ifdef EGL_MESA_drm_image

static struct pipe_resource *
egl_g3d_create_drm_buffer(_EGLDisplay *dpy, _EGLImage *img,
                          const EGLint *attribs)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct pipe_screen *screen = gdpy->native->screen;
   struct pipe_resource templ;
   _EGLImageAttribs attrs;
   EGLint format, valid_use;

   if (_eglParseImageAttribList(&attrs, dpy, attribs) != EGL_SUCCESS)
      return NULL;

   if (attrs.Width <= 0 || attrs.Height <= 0) {
      _eglLog(_EGL_DEBUG, "bad width or height (%dx%d)",
            attrs.Width, attrs.Height);
      return NULL;
   }

   switch (attrs.DRMBufferFormatMESA) {
   case EGL_DRM_BUFFER_FORMAT_ARGB32_MESA:
      format = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   default:
      _eglLog(_EGL_DEBUG, "bad image format value 0x%04x",
            attrs.DRMBufferFormatMESA);
      return NULL;
      break;
   }

   valid_use = EGL_DRM_BUFFER_USE_SCANOUT_MESA |
               EGL_DRM_BUFFER_USE_SHARE_MESA |
               EGL_DRM_BUFFER_USE_CURSOR_MESA;
   if (attrs.DRMBufferUseMESA & ~valid_use) {
      _eglLog(_EGL_DEBUG, "bad image use bit 0x%04x",
            attrs.DRMBufferUseMESA);
      return NULL;
   }

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = format;
   templ.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
   templ.width0 = attrs.Width;
   templ.height0 = attrs.Height;
   templ.depth0 = 1;
   templ.array_size = 1;

   /*
    * XXX fix apps (e.g. wayland) and pipe drivers (e.g. i915) and remove the
    * size check
    */
   if ((attrs.DRMBufferUseMESA & EGL_DRM_BUFFER_USE_SCANOUT_MESA) &&
       attrs.Width >= 640 && attrs.Height >= 480)
      templ.bind |= PIPE_BIND_SCANOUT;
   if (attrs.DRMBufferUseMESA & EGL_DRM_BUFFER_USE_SHARE_MESA)
      templ.bind |= PIPE_BIND_SHARED;
   if (attrs.DRMBufferUseMESA & EGL_DRM_BUFFER_USE_CURSOR_MESA) {
      if (attrs.Width != 64 || attrs.Height != 64)
         return NULL;
      templ.bind |= PIPE_BIND_CURSOR;
   }

   return screen->resource_create(screen, &templ);
}

static struct pipe_resource *
egl_g3d_reference_drm_buffer(_EGLDisplay *dpy, EGLint name,
                             _EGLImage *img, const EGLint *attribs)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   _EGLImageAttribs attrs;
   EGLint format;
   struct native_buffer nbuf;

   if (!dpy->Extensions.MESA_drm_image)
      return NULL;

   if (_eglParseImageAttribList(&attrs, dpy, attribs) != EGL_SUCCESS)
      return NULL;

   if (attrs.Width <= 0 || attrs.Height <= 0 ||
       attrs.DRMBufferStrideMESA <= 0) {
      _eglLog(_EGL_DEBUG, "bad width, height, or stride (%dx%dx%d)",
            attrs.Width, attrs.Height, attrs.DRMBufferStrideMESA);
      return NULL;
   }

   switch (attrs.DRMBufferFormatMESA) {
   case EGL_DRM_BUFFER_FORMAT_ARGB32_MESA:
      format = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   default:
      _eglLog(_EGL_DEBUG, "bad image format value 0x%04x",
            attrs.DRMBufferFormatMESA);
      return NULL;
      break;
   }

   memset(&nbuf, 0, sizeof(nbuf));
   nbuf.type = NATIVE_BUFFER_DRM;
   nbuf.u.drm.templ.target = PIPE_TEXTURE_2D;
   nbuf.u.drm.templ.format = format;
   nbuf.u.drm.templ.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
   nbuf.u.drm.templ.width0 = attrs.Width;
   nbuf.u.drm.templ.height0 = attrs.Height;
   nbuf.u.drm.templ.depth0 = 1;
   nbuf.u.drm.templ.array_size = 1;

   nbuf.u.drm.name = name;
   nbuf.u.drm.stride =
      attrs.DRMBufferStrideMESA * util_format_get_blocksize(format);

   return gdpy->native->buffer->import_buffer(gdpy->native, &nbuf);
}

#endif /* EGL_MESA_drm_image */

#ifdef EGL_WL_bind_wayland_display

static struct pipe_resource *
egl_g3d_reference_wl_buffer(_EGLDisplay *dpy, struct wl_buffer *buffer,
                            _EGLImage *img, const EGLint *attribs)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct pipe_resource *resource = NULL, *tmp = NULL;

   if (!gdpy->native->wayland_bufmgr)
      return NULL;

   tmp = gdpy->native->wayland_bufmgr->buffer_get_resource(gdpy->native, buffer);

   pipe_resource_reference(&resource, tmp);

   return resource;
}

#endif /* EGL_WL_bind_wayland_display */

#ifdef EGL_ANDROID_image_native_buffer

static struct pipe_resource *
egl_g3d_reference_android_native_buffer(_EGLDisplay *dpy,
                                        struct ANativeWindowBuffer *buf)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct native_buffer nbuf;

   memset(&nbuf, 0, sizeof(nbuf));
   nbuf.type = NATIVE_BUFFER_ANDROID;
   nbuf.u.android = buf;
    
   return gdpy->native->buffer->import_buffer(gdpy->native, &nbuf);
}

#endif /* EGL_ANDROID_image_native_buffer */

_EGLImage *
egl_g3d_create_image(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx,
                     EGLenum target, EGLClientBuffer buffer,
                     const EGLint *attribs)
{
   struct pipe_resource *ptex;
   struct egl_g3d_image *gimg;
   unsigned level = 0, layer = 0;

   gimg = CALLOC_STRUCT(egl_g3d_image);
   if (!gimg) {
      _eglError(EGL_BAD_ALLOC, "eglCreateEGLImageKHR");
      return NULL;
   }

   if (!_eglInitImage(&gimg->base, dpy)) {
      FREE(gimg);
      return NULL;
   }

   switch (target) {
   case EGL_NATIVE_PIXMAP_KHR:
      ptex = egl_g3d_reference_native_pixmap(dpy,
            (EGLNativePixmapType) buffer);
      break;
#ifdef EGL_MESA_drm_image
   case EGL_DRM_BUFFER_MESA:
      ptex = egl_g3d_reference_drm_buffer(dpy,
            (EGLint) pointer_to_intptr(buffer), &gimg->base, attribs);
      break;
#endif
#ifdef EGL_WL_bind_wayland_display
   case EGL_WAYLAND_BUFFER_WL:
      ptex = egl_g3d_reference_wl_buffer(dpy,
            (struct wl_buffer *) buffer, &gimg->base, attribs);
      break;
#endif
#ifdef EGL_ANDROID_image_native_buffer
   case EGL_NATIVE_BUFFER_ANDROID:
      ptex = egl_g3d_reference_android_native_buffer(dpy,
            (struct ANativeWindowBuffer *) buffer);
      break;
#endif
   default:
      ptex = NULL;
      break;
   }

   if (!ptex) {
      FREE(gimg);
      return NULL;
   }

   if (level > ptex->last_level) {
      _eglError(EGL_BAD_MATCH, "eglCreateEGLImageKHR");
      pipe_resource_reference(&gimg->texture, NULL);
      FREE(gimg);
      return NULL;
   }
   if (layer >= (u_minify(ptex->depth0, level) + ptex->array_size - 1)) {
      _eglError(EGL_BAD_PARAMETER, "eglCreateEGLImageKHR");
      pipe_resource_reference(&gimg->texture, NULL);
      FREE(gimg);
      return NULL;
   }

   /* transfer the ownership to the image */
   gimg->texture = ptex;
   gimg->level = level;
   gimg->layer = layer;

   return &gimg->base;
}

EGLBoolean
egl_g3d_destroy_image(_EGLDriver *drv, _EGLDisplay *dpy, _EGLImage *img)
{
   struct egl_g3d_image *gimg = egl_g3d_image(img);

   pipe_resource_reference(&gimg->texture, NULL);
   FREE(gimg);

   return EGL_TRUE;
}

_EGLImage *
egl_g3d_create_drm_image(_EGLDriver *drv, _EGLDisplay *dpy,
                         const EGLint *attribs)
{
   struct egl_g3d_image *gimg;
   struct pipe_resource *ptex;

   gimg = CALLOC_STRUCT(egl_g3d_image);
   if (!gimg) {
      _eglError(EGL_BAD_ALLOC, "eglCreateDRMImageKHR");
      return NULL;
   }

   if (!_eglInitImage(&gimg->base, dpy)) {
      FREE(gimg);
      return NULL;
   }

#ifdef EGL_MESA_drm_image
   ptex = egl_g3d_create_drm_buffer(dpy, &gimg->base, attribs);
#else
   ptex = NULL;
#endif
   if (!ptex) {
      FREE(gimg);
      return NULL;
   }

   /* transfer the ownership to the image */
   gimg->texture = ptex;
   gimg->level = 0;
   gimg->layer = 0;

   return &gimg->base;
}

EGLBoolean
egl_g3d_export_drm_image(_EGLDriver *drv, _EGLDisplay *dpy, _EGLImage *img,
			 EGLint *name, EGLint *handle, EGLint *stride)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_image *gimg = egl_g3d_image(img);
   struct native_buffer nbuf;

   if (!dpy->Extensions.MESA_drm_image)
      return EGL_FALSE;

   memset(&nbuf, 0, sizeof(nbuf));
   nbuf.type = NATIVE_BUFFER_DRM;
   if (name)
      nbuf.u.drm.templ.bind |= PIPE_BIND_SHARED;

   if (!gdpy->native->buffer->export_buffer(gdpy->native,
                                            gimg->texture, &nbuf))
      return EGL_FALSE;

   if (name)
      *name = nbuf.u.drm.name;
   if (handle)
      *handle = nbuf.u.drm.handle;
   if (stride)
      *stride = nbuf.u.drm.stride;

   return EGL_TRUE;
}
