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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <xf86drm.h>

#include <GL/gl.h> /* dri_interface needs GL types */
#include <GL/internal/dri_interface.h>

#include "gbm_driint.h"

#include "gbmint.h"

/* For importing wl_buffer */
#if HAVE_WAYLAND_PLATFORM
#include "../../../egl/wayland/wayland-drm/wayland-drm.h"
#endif

static __DRIimage *
dri_lookup_egl_image(__DRIscreen *screen, void *image, void *data)
{
   struct gbm_dri_device *dri = data;

   if (dri->lookup_image == NULL)
      return NULL;

   return dri->lookup_image(screen, image, dri->lookup_user_data);
}

static __DRIbuffer *
dri_get_buffers(__DRIdrawable * driDrawable,
		 int *width, int *height,
		 unsigned int *attachments, int count,
		 int *out_count, void *data)
{
   struct gbm_dri_surface *surf = data;
   struct gbm_dri_device *dri = gbm_dri_device(surf->base.gbm);

   if (dri->get_buffers == NULL)
      return NULL;

   return dri->get_buffers(driDrawable, width, height, attachments,
                           count, out_count, surf->dri_private);
}

static void
dri_flush_front_buffer(__DRIdrawable * driDrawable, void *data)
{
   struct gbm_dri_surface *surf = data;
   struct gbm_dri_device *dri = gbm_dri_device(surf->base.gbm);

   if (dri->flush_front_buffer != NULL)
      dri->flush_front_buffer(driDrawable, surf->dri_private);
}

static __DRIbuffer *
dri_get_buffers_with_format(__DRIdrawable * driDrawable,
                            int *width, int *height,
                            unsigned int *attachments, int count,
                            int *out_count, void *data)
{
   struct gbm_dri_surface *surf = data;
   struct gbm_dri_device *dri = gbm_dri_device(surf->base.gbm);

   if (dri->get_buffers_with_format == NULL)
      return NULL;

   return
      dri->get_buffers_with_format(driDrawable, width, height, attachments,
                                   count, out_count, surf->dri_private);
}

static const __DRIuseInvalidateExtension use_invalidate = {
   { __DRI_USE_INVALIDATE, 1 }
};

static const __DRIimageLookupExtension image_lookup_extension = {
   { __DRI_IMAGE_LOOKUP, 1 },
   dri_lookup_egl_image
};

const __DRIdri2LoaderExtension dri2_loader_extension = {
   { __DRI_DRI2_LOADER, 3 },
   dri_get_buffers,
   dri_flush_front_buffer,
   dri_get_buffers_with_format,
};

struct dri_extension_match {
   const char *name;
   int version;
   int offset;
};

static struct dri_extension_match dri_core_extensions[] = {
   { __DRI2_FLUSH, 1, offsetof(struct gbm_dri_device, flush) },
   { __DRI_IMAGE, 1, offsetof(struct gbm_dri_device, image) },
   { NULL, 0, 0 }
};

static struct dri_extension_match gbm_dri_device_extensions[] = {
   { __DRI_CORE, 1, offsetof(struct gbm_dri_device, core) },
   { __DRI_DRI2, 1, offsetof(struct gbm_dri_device, dri2) },
   { NULL, 0, 0 }
};

static int
dri_bind_extensions(struct gbm_dri_device *dri,
                    struct dri_extension_match *matches,
                    const __DRIextension **extensions)
{
   int i, j, ret = 0;
   void *field;

   for (i = 0; extensions[i]; i++) {
      for (j = 0; matches[j].name; j++) {
         if (strcmp(extensions[i]->name, matches[j].name) == 0 &&
             extensions[i]->version >= matches[j].version) {
            field = ((char *) dri + matches[j].offset);
            *(const __DRIextension **) field = extensions[i];
         }
      }
   }

   for (j = 0; matches[j].name; j++) {
      field = ((char *) dri + matches[j].offset);
      if (*(const __DRIextension **) field == NULL) {
         ret = -1;
      }
   }

   return ret;
}

static int
dri_load_driver(struct gbm_dri_device *dri)
{
   const __DRIextension **extensions;
   char path[PATH_MAX], *search_paths, *p, *next, *end;

   search_paths = NULL;
   if (geteuid() == getuid()) {
      /* don't allow setuid apps to use GBM_DRIVERS_PATH */
      search_paths = getenv("GBM_DRIVERS_PATH");
   }
   if (search_paths == NULL)
      search_paths = DEFAULT_DRIVER_DIR;

   dri->driver = NULL;
   end = search_paths + strlen(search_paths);
   for (p = search_paths; p < end && dri->driver == NULL; p = next + 1) {
      int len;
      next = strchr(p, ':');
      if (next == NULL)
         next = end;

      len = next - p;
#if GLX_USE_TLS
      snprintf(path, sizeof path,
               "%.*s/tls/%s_dri.so", len, p, dri->base.driver_name);
      dri->driver = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
#endif
      if (dri->driver == NULL) {
         snprintf(path, sizeof path,
                  "%.*s/%s_dri.so", len, p, dri->base.driver_name);
         dri->driver = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
         if (dri->driver == NULL)
            fprintf(stderr, "failed to open %s: %s\n", path, dlerror());
      }
   }

   if (dri->driver == NULL) {
      fprintf(stderr, "gbm: failed to open any driver (search paths %s)",
              search_paths);
      return -1;
   }

   extensions = dlsym(dri->driver, __DRI_DRIVER_EXTENSIONS);
   if (extensions == NULL) {
      fprintf(stderr, "gbm: driver exports no extensions (%s)", dlerror());
      dlclose(dri->driver);
      return -1;
   }


   if (dri_bind_extensions(dri, gbm_dri_device_extensions, extensions) < 0) {
      dlclose(dri->driver);
      fprintf(stderr, "failed to bind extensions\n");
      return -1;
   }

   return 0;
}

static int
dri_screen_create(struct gbm_dri_device *dri)
{
   const __DRIextension **extensions;
   int ret = 0;

   dri->base.driver_name = dri_fd_get_driver_name(dri->base.base.fd);
   if (dri->base.driver_name == NULL)
      return -1;

   ret = dri_load_driver(dri);
   if (ret) {
      fprintf(stderr, "failed to load driver: %s\n", dri->base.driver_name);
      return ret;
   };

   dri->extensions[0] = &image_lookup_extension.base;
   dri->extensions[1] = &use_invalidate.base;
   dri->extensions[2] = &dri2_loader_extension.base;
   dri->extensions[3] = NULL;

   if (dri->dri2 == NULL)
      return -1;

   dri->screen = dri->dri2->createNewScreen(0, dri->base.base.fd,
                                            dri->extensions,
                                            &dri->driver_configs, dri);
   if (dri->screen == NULL)
      return -1;

   extensions = dri->core->getExtensions(dri->screen);
   if (dri_bind_extensions(dri, dri_core_extensions, extensions) < 0) {
      ret = -1;
      goto free_screen;
   }

   dri->lookup_image = NULL;
   dri->lookup_user_data = NULL;

   return 0;

free_screen:
   dri->core->destroyScreen(dri->screen);

   return ret;
}

static int
gbm_dri_is_format_supported(struct gbm_device *gbm,
                            uint32_t format,
                            uint32_t usage)
{
   switch (format) {
   case GBM_BO_FORMAT_XRGB8888:
   case GBM_FORMAT_XRGB8888:
      break;
   case GBM_BO_FORMAT_ARGB8888:
   case GBM_FORMAT_ARGB8888:
      if (usage & GBM_BO_USE_SCANOUT)
         return 0;
      break;
   default:
      return 0;
   }

   if (usage & GBM_BO_USE_CURSOR_64X64 &&
       usage & GBM_BO_USE_RENDERING)
      return 0;

   return 1;
}

static int
gbm_dri_bo_write(struct gbm_bo *_bo, const void *buf, size_t count)
{
   struct gbm_dri_bo *bo = gbm_dri_bo(_bo);

   if (bo->image != NULL)
      return -1;

   memcpy(bo->map, buf, count);

   return 0;
}

static void
gbm_dri_bo_destroy(struct gbm_bo *_bo)
{
   struct gbm_dri_device *dri = gbm_dri_device(_bo->gbm);
   struct gbm_dri_bo *bo = gbm_dri_bo(_bo);
   struct drm_mode_destroy_dumb arg;

   if (bo->image != NULL) {
      dri->image->destroyImage(bo->image);
   } else {
      munmap(bo->map, bo->size);
      memset(&arg, 0, sizeof(arg));
      arg.handle = bo->handle;
      drmIoctl(dri->base.base.fd, DRM_IOCTL_MODE_DESTROY_DUMB, &arg);
   }

   free(bo);
}

static uint32_t
gbm_dri_to_gbm_format(uint32_t dri_format)
{
   uint32_t ret = 0;

   switch (dri_format) {
   case __DRI_IMAGE_FORMAT_RGB565:
      ret = GBM_FORMAT_RGB565;
      break;
   case __DRI_IMAGE_FORMAT_XRGB8888:
      ret = GBM_FORMAT_XRGB8888;
      break;
   case __DRI_IMAGE_FORMAT_ARGB8888:
      ret = GBM_FORMAT_ARGB8888;
      break;
   case __DRI_IMAGE_FORMAT_ABGR8888:
      ret = GBM_FORMAT_ABGR8888;
      break;
   default:
      ret = 0;
      break;
   }

   return ret;
}

static struct gbm_bo *
gbm_dri_bo_import(struct gbm_device *gbm,
                  uint32_t type, void *buffer, uint32_t usage)
{
   struct gbm_dri_device *dri = gbm_dri_device(gbm);
   struct gbm_dri_bo *bo;
   __DRIimage *image;
   unsigned dri_use = 0;
   int gbm_format;

   /* Required for query image WIDTH & HEIGHT */
   if (dri->image->base.version < 4)
      return NULL;

   switch (type) {
#if HAVE_WAYLAND_PLATFORM
   case GBM_BO_IMPORT_WL_BUFFER:
   {
      struct wl_drm_buffer *wb = (struct wl_drm_buffer *) buffer;

      if (!wayland_buffer_is_drm(buffer))
         return NULL;

      image = wb->driver_buffer;

      switch (wb->format) {
      case WL_DRM_FORMAT_XRGB8888:
         gbm_format = GBM_FORMAT_XRGB8888;
         break;
      case WL_DRM_FORMAT_ARGB8888:
         gbm_format = GBM_FORMAT_ARGB8888;
         break;
      case WL_DRM_FORMAT_YUYV:
         gbm_format = GBM_FORMAT_YUYV;
         break;
      default:
         return NULL;
      }
      break;
   }
#endif

   case GBM_BO_IMPORT_EGL_IMAGE:
   {
      int dri_format;
      if (dri->lookup_image == NULL)
         return NULL;

      image = dri->lookup_image(dri->screen, buffer, dri->lookup_user_data);
      dri->image->queryImage(image, __DRI_IMAGE_ATTRIB_FORMAT, &dri_format);
      gbm_format = gbm_dri_to_gbm_format(dri_format);
      if (gbm_format == 0)
         return NULL;
      break;
   }

   default:
      return NULL;
   }


   bo = calloc(1, sizeof *bo);
   if (bo == NULL)
      return NULL;

   bo->image = dri->image->dupImage(image, NULL);

   if (usage & GBM_BO_USE_SCANOUT)
      dri_use |= __DRI_IMAGE_USE_SCANOUT;
   if (usage & GBM_BO_USE_CURSOR_64X64)
      dri_use |= __DRI_IMAGE_USE_CURSOR;
   if (dri->image->base.version >= 2 &&
       !dri->image->validateUsage(bo->image, dri_use)) {
      free(bo);
      return NULL;
   }

   bo->base.base.gbm = gbm;
   bo->base.base.format = gbm_format;

   dri->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_WIDTH,
                          (int*)&bo->base.base.width);
   dri->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_HEIGHT,
                          (int*)&bo->base.base.height);
   dri->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_STRIDE,
                          (int*)&bo->base.base.stride);
   dri->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_HANDLE,
                          &bo->base.base.handle.s32);

   return &bo->base.base;
}

static struct gbm_bo *
create_dumb(struct gbm_device *gbm,
                  uint32_t width, uint32_t height,
                  uint32_t format, uint32_t usage)
{
   struct gbm_dri_device *dri = gbm_dri_device(gbm);
   struct drm_mode_create_dumb create_arg;
   struct drm_mode_map_dumb map_arg;
   struct gbm_dri_bo *bo;
   struct drm_mode_destroy_dumb destroy_arg;
   int ret;

   if (!(usage & GBM_BO_USE_CURSOR_64X64))
      return NULL;
   if (format != GBM_FORMAT_ARGB8888)
      return NULL;

   bo = calloc(1, sizeof *bo);
   if (bo == NULL)
      return NULL;

   create_arg.bpp = 32;
   create_arg.width = width;
   create_arg.height = height;

   ret = drmIoctl(dri->base.base.fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_arg);
   if (ret)
      goto free_bo;

   bo->base.base.gbm = gbm;
   bo->base.base.width = width;
   bo->base.base.height = height;
   bo->base.base.stride = create_arg.pitch;
   bo->base.base.handle.u32 = create_arg.handle;
   bo->handle = create_arg.handle;
   bo->size = create_arg.size;

   memset(&map_arg, 0, sizeof(map_arg));
   map_arg.handle = bo->handle;

   ret = drmIoctl(dri->base.base.fd, DRM_IOCTL_MODE_MAP_DUMB, &map_arg);
   if (ret)
      goto destroy_dumb;

   bo->map = mmap(0, bo->size, PROT_WRITE,
                  MAP_SHARED, dri->base.base.fd, map_arg.offset);
   if (bo->map == MAP_FAILED)
      goto destroy_dumb;

   return &bo->base.base;

destroy_dumb:
   memset(&destroy_arg, 0, sizeof destroy_arg);
   destroy_arg.handle = create_arg.handle;
   drmIoctl(dri->base.base.fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_arg);
free_bo:
   free(bo);

   return NULL;
}

static struct gbm_bo *
gbm_dri_bo_create(struct gbm_device *gbm,
                  uint32_t width, uint32_t height,
                  uint32_t format, uint32_t usage)
{
   struct gbm_dri_device *dri = gbm_dri_device(gbm);
   struct gbm_dri_bo *bo;
   int dri_format;
   unsigned dri_use = 0;

   if (usage & GBM_BO_USE_WRITE)
      return create_dumb(gbm, width, height, format, usage);

   bo = calloc(1, sizeof *bo);
   if (bo == NULL)
      return NULL;

   bo->base.base.gbm = gbm;
   bo->base.base.width = width;
   bo->base.base.height = height;

   switch (format) {
   case GBM_FORMAT_RGB565:
      dri_format =__DRI_IMAGE_FORMAT_RGB565;
      break;
   case GBM_FORMAT_XRGB8888:
   case GBM_BO_FORMAT_XRGB8888:
      dri_format = __DRI_IMAGE_FORMAT_XRGB8888;
      break;
   case GBM_FORMAT_ARGB8888:
   case GBM_BO_FORMAT_ARGB8888:
      dri_format = __DRI_IMAGE_FORMAT_ARGB8888;
      break;
   case GBM_FORMAT_ABGR8888:
      dri_format = __DRI_IMAGE_FORMAT_ABGR8888;
      break;
   default:
      return NULL;
   }

   if (usage & GBM_BO_USE_SCANOUT)
      dri_use |= __DRI_IMAGE_USE_SCANOUT;
   if (usage & GBM_BO_USE_CURSOR_64X64)
      dri_use |= __DRI_IMAGE_USE_CURSOR;

   /* Gallium drivers requires shared in order to get the handle/stride */
   dri_use |= __DRI_IMAGE_USE_SHARE;

   bo->image =
      dri->image->createImage(dri->screen,
                              width, height,
                              dri_format, dri_use,
                              bo);
   if (bo->image == NULL)
      return NULL;

   dri->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_HANDLE,
                          &bo->base.base.handle.s32);
   dri->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_STRIDE,
                          (int *) &bo->base.base.stride);

   return &bo->base.base;
}

static struct gbm_surface *
gbm_dri_surface_create(struct gbm_device *gbm,
                       uint32_t width, uint32_t height,
		       uint32_t format, uint32_t flags)
{
   struct gbm_dri_surface *surf;

   surf = calloc(1, sizeof *surf);
   if (surf == NULL)
      return NULL;

   surf->base.gbm = gbm;
   surf->base.width = width;
   surf->base.height = height;
   surf->base.format = format;
   surf->base.flags = flags;

   return &surf->base;
}

static void
gbm_dri_surface_destroy(struct gbm_surface *_surf)
{
   struct gbm_dri_surface *surf = gbm_dri_surface(_surf);

   free(surf);
}

static void
dri_destroy(struct gbm_device *gbm)
{
   struct gbm_dri_device *dri = gbm_dri_device(gbm);

   dri->core->destroyScreen(dri->screen);
   free(dri->driver_configs);
   dlclose(dri->driver);
   free(dri->base.driver_name);

   free(dri);
}

static struct gbm_device *
dri_device_create(int fd)
{
   struct gbm_dri_device *dri;
   int ret;

   dri = calloc(1, sizeof *dri);

   dri->base.base.fd = fd;
   dri->base.base.bo_create = gbm_dri_bo_create;
   dri->base.base.bo_import = gbm_dri_bo_import;
   dri->base.base.is_format_supported = gbm_dri_is_format_supported;
   dri->base.base.bo_write = gbm_dri_bo_write;
   dri->base.base.bo_destroy = gbm_dri_bo_destroy;
   dri->base.base.destroy = dri_destroy;
   dri->base.base.surface_create = gbm_dri_surface_create;
   dri->base.base.surface_destroy = gbm_dri_surface_destroy;

   dri->base.type = GBM_DRM_DRIVER_TYPE_DRI;
   dri->base.base.name = "drm";

   ret = dri_screen_create(dri);
   if (ret)
      goto err_dri;

   return &dri->base.base;

err_dri:
   free(dri);

   return NULL;
}

struct gbm_backend gbm_dri_backend = {
   .backend_name = "dri",
   .create_device = dri_device_create,
};
