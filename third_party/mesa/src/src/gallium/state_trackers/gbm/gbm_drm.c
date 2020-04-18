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
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
 */

#include "util/u_memory.h"
#include "util/u_inlines.h"

#include "state_tracker/drm_driver.h"

#include <unistd.h>
#include <sys/types.h>

#include "gbm_gallium_drmint.h"

/* For importing wl_buffer */
#if HAVE_WAYLAND_PLATFORM
#include "../../../egl/wayland/wayland-drm/wayland-drm.h"
#endif

static INLINE enum pipe_format
gbm_format_to_gallium(enum gbm_bo_format format)
{
   switch (format) {
   case GBM_BO_FORMAT_XRGB8888:
      return PIPE_FORMAT_B8G8R8X8_UNORM;
   case GBM_BO_FORMAT_ARGB8888:
      return PIPE_FORMAT_B8G8R8A8_UNORM;
   default:
      return PIPE_FORMAT_NONE;
   }

   return PIPE_FORMAT_NONE;
}

static INLINE uint
gbm_usage_to_gallium(uint usage)
{
   uint resource_usage = 0;

   if (usage & GBM_BO_USE_SCANOUT)
      resource_usage |= PIPE_BIND_SCANOUT;

   if (usage & GBM_BO_USE_RENDERING)
      resource_usage |= PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;

   if (usage & GBM_BO_USE_CURSOR_64X64)
      resource_usage |= PIPE_BIND_CURSOR;

   return resource_usage;
}

static int
gbm_gallium_drm_is_format_supported(struct gbm_device *gbm,
                                    enum gbm_bo_format format,
                                    uint32_t usage)
{
   struct gbm_gallium_drm_device *gdrm = gbm_gallium_drm_device(gbm);
   enum pipe_format pf;

   pf = gbm_format_to_gallium(format);
   if (pf == PIPE_FORMAT_NONE)
      return 0;

   if (!gdrm->screen->is_format_supported(gdrm->screen, PIPE_TEXTURE_2D, pf, 0,
                                          gbm_usage_to_gallium(usage)))
      return 0;

   if (usage & GBM_BO_USE_SCANOUT && format != GBM_BO_FORMAT_XRGB8888)
      return 0;

   return 1;
}

static void
gbm_gallium_drm_bo_destroy(struct gbm_bo *_bo)
{
   struct gbm_gallium_drm_bo *bo = gbm_gallium_drm_bo(_bo);

   pipe_resource_reference(&bo->resource, NULL);
   free(bo);
}

static struct gbm_bo *
gbm_gallium_drm_bo_import(struct gbm_device *gbm,
                          uint32_t type, void *buffer, uint32_t usage)
{
   struct gbm_gallium_drm_device *gdrm = gbm_gallium_drm_device(gbm);
   struct gbm_gallium_drm_bo *bo;
   struct winsys_handle whandle;
   struct pipe_resource *resource;

   switch (type) {
#if HAVE_WAYLAND_PLATFORM
   case GBM_BO_IMPORT_WL_BUFFER:
   {
      struct wl_drm_buffer *wb = (struct wl_drm_buffer *) buffer;

      resource = wb->driver_buffer;
      break;
   }
#endif

   case GBM_BO_IMPORT_EGL_IMAGE:
      if (!gdrm->lookup_egl_image)
         return NULL;

      resource = gdrm->lookup_egl_image(gdrm->lookup_egl_image_data, buffer);
      if (resource == NULL)
         return NULL;
      break;

   default:
      return NULL;
   }

   bo = CALLOC_STRUCT(gbm_gallium_drm_bo);
   if (bo == NULL)
      return NULL;

   bo->base.base.gbm = gbm;
   bo->base.base.width = resource->width0;
   bo->base.base.height = resource->height0;

   switch (resource->format) {
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      bo->base.base.format = GBM_BO_FORMAT_XRGB8888;
      break;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      bo->base.base.format = GBM_BO_FORMAT_ARGB8888;
      break;
   default:
      FREE(bo);
      return NULL;
   }

   pipe_resource_reference(&bo->resource, resource);

   memset(&whandle, 0, sizeof(whandle));
   whandle.type = DRM_API_HANDLE_TYPE_KMS;
   gdrm->screen->resource_get_handle(gdrm->screen, bo->resource, &whandle);

   bo->base.base.handle.u32 = whandle.handle;
   bo->base.base.stride      = whandle.stride;

   return &bo->base.base;
}

static struct gbm_bo *
gbm_gallium_drm_bo_create(struct gbm_device *gbm,
                          uint32_t width, uint32_t height,
                          enum gbm_bo_format format, uint32_t usage)
{
   struct gbm_gallium_drm_device *gdrm = gbm_gallium_drm_device(gbm);
   struct gbm_gallium_drm_bo *bo;
   struct pipe_resource templ;
   struct winsys_handle whandle;
   enum pipe_format pf;

   bo = CALLOC_STRUCT(gbm_gallium_drm_bo);
   if (bo == NULL)
      return NULL;

   bo->base.base.gbm = gbm;
   bo->base.base.width = width;
   bo->base.base.height = height;
   bo->base.base.format = format;

   pf = gbm_format_to_gallium(format);
   if (pf == PIPE_FORMAT_NONE)
      return NULL;

   memset(&templ, 0, sizeof(templ));
   templ.bind = gbm_usage_to_gallium(usage);
   templ.format = pf;
   templ.target = PIPE_TEXTURE_2D;
   templ.last_level = 0;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;

   bo->resource = gdrm->screen->resource_create(gdrm->screen, &templ);
   if (bo->resource == NULL) {
      FREE(bo);
      return NULL;
   }

   memset(&whandle, 0, sizeof(whandle));
   whandle.type = DRM_API_HANDLE_TYPE_KMS;
   gdrm->screen->resource_get_handle(gdrm->screen, bo->resource, &whandle);

   bo->base.base.handle.u32 = whandle.handle;
   bo->base.base.stride      = whandle.stride;

   return &bo->base.base;
}

static void
gbm_gallium_drm_destroy(struct gbm_device *gbm)
{
   struct gbm_gallium_drm_device *gdrm = gbm_gallium_drm_device(gbm);

   gallium_screen_destroy(gdrm);
   FREE(gdrm);
}

struct gbm_device *
gbm_gallium_drm_device_create(int fd)
{
   struct gbm_gallium_drm_device *gdrm;
   int ret;

   gdrm = calloc(1, sizeof *gdrm);

   gdrm->base.base.fd = fd;
   gdrm->base.base.bo_create = gbm_gallium_drm_bo_create;
   gdrm->base.base.bo_import = gbm_gallium_drm_bo_import;
   gdrm->base.base.bo_destroy = gbm_gallium_drm_bo_destroy;
   gdrm->base.base.is_format_supported = gbm_gallium_drm_is_format_supported;
   gdrm->base.base.destroy = gbm_gallium_drm_destroy;

   gdrm->base.type = GBM_DRM_DRIVER_TYPE_GALLIUM;
   gdrm->base.base.name = "drm";

   ret = gallium_screen_create(gdrm);
   if (ret) {
      free(gdrm);
      return NULL;
   }

   return &gdrm->base.base;
}
