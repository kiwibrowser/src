/*
 * Mesa 3-D graphics library
 * Version:  7.12
 *
 * Copyright (C) 2010-2011 Chia-I Wu <olvaffe@gmail.com>
 * Copyright (C) 2010-2011 LunarG Inc.
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

#define LOG_TAG "EGL-GALLIUM"

#if ANDROID_VERSION >= 0x0400
#include <stdlib.h>
#include <system/window.h>
#else
#define android_native_buffer_t ANativeWindowBuffer
#include <ui/egl/android_natives.h>
#include <ui/android_native_buffer.h>
#endif

#include <hardware/gralloc.h>
#include <cutils/properties.h>
#include <cutils/log.h>
#include <utils/Errors.h>

extern "C" {
#include "egllog.h"
}

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_box.h"
#include "common/native.h"
#include "common/native_helper.h"
#include "android/android_sw_winsys.h"
#include "state_tracker/drm_driver.h"

struct android_config;

struct android_display {
   struct native_display base;

   boolean use_drm;
   const struct native_event_handler *event_handler;
   struct android_config *configs;
   int num_configs;
};

struct android_surface {
   struct native_surface base;

   struct android_display *adpy;
   ANativeWindow *win;

   /* staging color buffer for when buffer preserving is enabled */
   struct pipe_resource *color_res;

   uint stamp;
   ANativeWindowBuffer *buf;
   struct pipe_resource *buf_res;

   /* cache the current back buffers */
   struct {
      int width;
      int height;
      int format;
   } cache_key;
   void *cache_handles[2];
   struct pipe_resource *cache_resources[2];
};

struct android_config {
   struct native_config base;
};

static INLINE struct android_display *
android_display(const struct native_display *ndpy)
{
   return (struct android_display *) ndpy;
}

static INLINE struct android_surface *
android_surface(const struct native_surface *nsurf)
{
   return (struct android_surface *) nsurf;
}

static INLINE struct android_config *
android_config(const struct native_config *nconf)
{
   return (struct android_config *) nconf;
}

namespace android {

static enum pipe_format
get_pipe_format(int native)
{
   enum pipe_format fmt;

   switch (native) {
   case HAL_PIXEL_FORMAT_RGBA_8888:
      fmt = PIPE_FORMAT_R8G8B8A8_UNORM;
      break;
   case HAL_PIXEL_FORMAT_RGBX_8888:
      fmt = PIPE_FORMAT_R8G8B8X8_UNORM;
      break;
   case HAL_PIXEL_FORMAT_RGB_888:
      fmt = PIPE_FORMAT_R8G8B8_UNORM;
      break;
   case HAL_PIXEL_FORMAT_RGB_565:
      fmt = PIPE_FORMAT_B5G6R5_UNORM;
      break;
   case HAL_PIXEL_FORMAT_BGRA_8888:
      fmt = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   case HAL_PIXEL_FORMAT_RGBA_5551:
      /* fmt = PIPE_FORMAT_A1B5G5R5_UNORM; */
   case HAL_PIXEL_FORMAT_RGBA_4444:
      /* fmt = PIPE_FORMAT_A4B4G4R4_UNORM; */
   default:
      LOGE("unsupported native format 0x%x", native);
      fmt = PIPE_FORMAT_NONE;
      break;
   }

   return fmt;
}

#ifndef ANDROID_BACKEND_NO_DRM

#include <gralloc_drm_handle.h>
static int
get_handle_name(buffer_handle_t handle)
{
   struct gralloc_drm_handle_t *dh;

   /* check that the buffer is allocated by drm_gralloc and cast */
   dh = gralloc_drm_handle(handle);

   return (dh) ? dh->name : 0;
}

#else

static int
get_handle_name(buffer_handle_t handle)
{
   return 0;
}

#endif /* ANDROID_BACKEND_NO_DRM */

/**
 * Import an ANativeWindowBuffer allocated by the server.
 */
static struct pipe_resource *
import_buffer(struct android_display *adpy, const struct pipe_resource *templ,
              ANativeWindowBuffer *abuf)
{
   struct pipe_screen *screen = adpy->base.screen;
   struct pipe_resource *res;

   if (templ->bind & PIPE_BIND_RENDER_TARGET) {
      if (!screen->is_format_supported(screen, templ->format,
               templ->target, 0, PIPE_BIND_RENDER_TARGET))
         LOGW("importing unsupported buffer as render target");
   }
   if (templ->bind & PIPE_BIND_SAMPLER_VIEW) {
      if (!screen->is_format_supported(screen, templ->format,
               templ->target, 0, PIPE_BIND_SAMPLER_VIEW))
         LOGW("importing unsupported buffer as sampler view");
   }

   if (adpy->use_drm) {
      struct winsys_handle handle;

      memset(&handle, 0, sizeof(handle));
      handle.type = DRM_API_HANDLE_TYPE_SHARED;
      /* for DRM, we need the GEM name */
      handle.handle = get_handle_name(abuf->handle);
      if (!handle.handle) {
         LOGE("unable to import invalid buffer %p", abuf);
         return NULL;
      }

      handle.stride =
         abuf->stride * util_format_get_blocksize(templ->format);

      res = screen->resource_from_handle(screen, templ, &handle);
   }
   else {
      struct android_winsys_handle handle;

      memset(&handle, 0, sizeof(handle));
      handle.handle = abuf->handle;
      handle.stride =
         abuf->stride * util_format_get_blocksize(templ->format);

      res = screen->resource_from_handle(screen,
            templ, (struct winsys_handle *) &handle);
   }

   if (!res)
      LOGE("failed to import buffer %p", abuf);

   return res;
}

static void
android_surface_clear_cache(struct native_surface *nsurf)
{
   struct android_surface *asurf = android_surface(nsurf);
   int i;

   for (i = 0; i < Elements(asurf->cache_handles); i++) {
      asurf->cache_handles[i] = NULL;
      pipe_resource_reference(&asurf->cache_resources[i], NULL);
   }

   memset(&asurf->cache_key, 0, sizeof(asurf->cache_key));
}

static struct pipe_resource *
android_surface_add_cache(struct native_surface *nsurf,
                          ANativeWindowBuffer *abuf)
{
   struct android_surface *asurf = android_surface(nsurf);
   void *handle;
   int idx;

   /* how about abuf->usage? */
   if (asurf->cache_key.width != abuf->width ||
       asurf->cache_key.height != abuf->height ||
       asurf->cache_key.format != abuf->format)
      android_surface_clear_cache(&asurf->base);

   if (asurf->adpy->use_drm)
      handle = (void *) get_handle_name(abuf->handle);
   else
      handle = (void *) abuf->handle;
   /* NULL is invalid */
   if (!handle) {
      LOGE("invalid buffer native buffer %p", abuf);
      return NULL;
   }

   /* find the slot to use */
   for (idx = 0; idx < Elements(asurf->cache_handles); idx++) {
      if (asurf->cache_handles[idx] == handle || !asurf->cache_handles[idx])
         break;
   }
   if (idx == Elements(asurf->cache_handles)) {
      LOGW("cache full: buf %p, width %d, height %d, format %d, usage 0x%x",
            abuf, abuf->width, abuf->height, abuf->format, abuf->usage);
      android_surface_clear_cache(&asurf->base);
      idx = 0;
   }

   if (idx == 0) {
      asurf->cache_key.width = abuf->width;
      asurf->cache_key.height = abuf->height;
      asurf->cache_key.format = abuf->format;
   }

   if (!asurf->cache_handles[idx]) {
      struct pipe_resource templ;

      assert(!asurf->cache_resources[idx]);

      memset(&templ, 0, sizeof(templ));
      templ.target = PIPE_TEXTURE_2D;
      templ.format = get_pipe_format(asurf->buf->format);
      templ.bind = PIPE_BIND_RENDER_TARGET;
      if (!asurf->adpy->use_drm) {
         templ.bind |= PIPE_BIND_TRANSFER_WRITE |
                       PIPE_BIND_TRANSFER_READ;
      }

      templ.width0 = asurf->buf->width;
      templ.height0 = asurf->buf->height;
      templ.depth0 = 1;
      templ.array_size = 1;

      if (templ.format != PIPE_FORMAT_NONE) {
         asurf->cache_resources[idx] =
            import_buffer(asurf->adpy, &templ, asurf->buf);
      }
      else {
         asurf->cache_resources[idx] = NULL;
      }

      asurf->cache_handles[idx] = handle;
   }

   return asurf->cache_resources[idx];
}

/**
 * Dequeue the next back buffer for rendering.
 */
static boolean
android_surface_dequeue_buffer(struct native_surface *nsurf)
{
   struct android_surface *asurf = android_surface(nsurf);
   struct pipe_resource *res;

   if (asurf->win->dequeueBuffer(asurf->win, &asurf->buf) != NO_ERROR) {
      LOGE("failed to dequeue window %p", asurf->win);
      return FALSE;
   }

   asurf->buf->common.incRef(&asurf->buf->common);
   asurf->win->lockBuffer(asurf->win, asurf->buf);

   res = android_surface_add_cache(&asurf->base, asurf->buf);
   if (!res)
      return FALSE;

   pipe_resource_reference(&asurf->buf_res, res);

   return TRUE;
}

/**
 * Enqueue the back buffer.  This will make it the next front buffer.
 */
static boolean
android_surface_enqueue_buffer(struct native_surface *nsurf)
{
   struct android_surface *asurf = android_surface(nsurf);

   pipe_resource_reference(&asurf->buf_res, NULL);

   asurf->win->queueBuffer(asurf->win, asurf->buf);

   asurf->buf->common.decRef(&asurf->buf->common);
   asurf->buf = NULL;

   return TRUE;
}

static boolean
android_surface_swap_buffers(struct native_surface *nsurf)
{
   struct android_surface *asurf = android_surface(nsurf);
   struct android_display *adpy = asurf->adpy;

   android_surface_enqueue_buffer(&asurf->base);

   asurf->stamp++;
   adpy->event_handler->invalid_surface(&adpy->base,
         &asurf->base, asurf->stamp);

   return TRUE;
}

static void
copy_resources(struct native_display *ndpy,
               struct pipe_resource *src,
               struct pipe_resource *dst)
{
   struct pipe_context *pipe;
   struct pipe_box box;

   pipe = ndpy_get_copy_context(ndpy);
   if (!pipe)
      return;

   u_box_origin_2d(src->width0, src->height0, &box);
   pipe->resource_copy_region(pipe, dst, 0, 0, 0, 0, src, 0, &box);
   pipe->flush(pipe, NULL);
}

static boolean
android_surface_present(struct native_surface *nsurf,
                        const native_present_control *ctrl)
{
   struct android_surface *asurf = android_surface(nsurf);
   struct android_display *adpy = asurf->adpy;
   boolean ret;

   if (ctrl->swap_interval || ctrl->natt != NATIVE_ATTACHMENT_BACK_LEFT)
      return FALSE;

   /* this happens when eglSwapBuffers is called more than once in a row */
   if (!asurf->buf)
      return TRUE;

   /* we always render to color_res first when it exists */
   if (asurf->color_res) {
      copy_resources(&adpy->base, asurf->color_res, asurf->buf_res);
      if (!ctrl->preserve)
         pipe_resource_reference(&asurf->color_res, NULL);
   }
   else if (ctrl->preserve) {
      struct pipe_resource templ;

      memset(&templ, 0, sizeof(templ));
      templ.target = asurf->buf_res->target;
      templ.format = asurf->buf_res->format;
      templ.bind = PIPE_BIND_RENDER_TARGET;
      templ.width0 = asurf->buf_res->width0;
      templ.height0 = asurf->buf_res->height0;
      templ.depth0 = asurf->buf_res->depth0;
      templ.array_size = asurf->buf_res->array_size;

      asurf->color_res =
         adpy->base.screen->resource_create(adpy->base.screen, &templ);
      if (!asurf->color_res)
         return FALSE;

      /* preserve the contents */
      copy_resources(&adpy->base, asurf->buf_res, asurf->color_res);
   }

   return android_surface_swap_buffers(nsurf);
}

static boolean
android_surface_validate(struct native_surface *nsurf, uint attachment_mask,
                         unsigned int *seq_num, struct pipe_resource **textures,
                         int *width, int *height)
{
   struct android_surface *asurf = android_surface(nsurf);
   struct winsys_handle handle;

   if (!asurf->buf) {
      if (!android_surface_dequeue_buffer(&asurf->base))
         return FALSE;

      /* color_res must be compatible with buf_res */
      if (asurf->color_res &&
          (asurf->color_res->format != asurf->buf_res->format ||
           asurf->color_res->width0 != asurf->buf_res->width0 ||
           asurf->color_res->height0 != asurf->buf_res->height0))
         pipe_resource_reference(&asurf->color_res, NULL);
   }

   if (textures) {
      /* we have access to only the back buffer */
      const enum native_attachment att = NATIVE_ATTACHMENT_BACK_LEFT;

      if (native_attachment_mask_test(attachment_mask, att)) {
         textures[att] = NULL;
         pipe_resource_reference(&textures[att],
               (asurf->color_res) ? asurf->color_res : asurf->buf_res);
      }
   }

   if (seq_num)
      *seq_num = asurf->stamp;
   if (width)
      *width = asurf->buf->width;
   if (height)
      *height = asurf->buf->height;

   return TRUE;
}

static void
android_surface_wait(struct native_surface *nsurf)
{
}

static void
android_surface_destroy(struct native_surface *nsurf)
{
   struct android_surface *asurf = android_surface(nsurf);
   int i;

   pipe_resource_reference(&asurf->color_res, NULL);

   if (asurf->buf)
      android_surface_enqueue_buffer(&asurf->base);

   android_surface_clear_cache(&asurf->base);

   asurf->win->common.decRef(&asurf->win->common);

   FREE(asurf);
}

static struct native_surface *
android_display_create_window_surface(struct native_display *ndpy,
                                      EGLNativeWindowType win,
                                      const struct native_config *nconf)
{
   struct android_display *adpy = android_display(ndpy);
   struct android_config *aconf = android_config(nconf);
   struct android_surface *asurf;
   enum pipe_format format;
   int val;

   if (win->common.magic != ANDROID_NATIVE_WINDOW_MAGIC) {
      LOGE("invalid native window with magic 0x%x", win->common.magic);
      return NULL;
   }
   if (win->query(win, NATIVE_WINDOW_FORMAT, &val)) {
      LOGE("failed to query native window format");
      return NULL;
   }
   format = get_pipe_format(val);
   if (format != nconf->color_format) {
      LOGW("native window format 0x%x != config format 0x%x",
            format, nconf->color_format);
      if (!adpy->base.screen->is_format_supported(adpy->base.screen,
               format, PIPE_TEXTURE_2D, 0, PIPE_BIND_RENDER_TARGET)) {
         LOGE("and the native window cannot be used as a render target");
         return NULL;
      }
   }

   asurf = CALLOC_STRUCT(android_surface);
   if (!asurf)
      return NULL;

   asurf->adpy = adpy;
   asurf->win = win;
   asurf->win->common.incRef(&asurf->win->common);

   /* request buffers that are for CPU access */
   if (!adpy->use_drm) {
      native_window_set_usage(asurf->win,
            GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN);
   }

   asurf->base.destroy = android_surface_destroy;
   asurf->base.present = android_surface_present;
   asurf->base.validate = android_surface_validate;
   asurf->base.wait = android_surface_wait;

   return &asurf->base;
}

static boolean
android_display_init_configs(struct native_display *ndpy)
{
   struct android_display *adpy = android_display(ndpy);
   const int native_formats[] = {
      HAL_PIXEL_FORMAT_RGBA_8888,
      HAL_PIXEL_FORMAT_RGBX_8888,
      HAL_PIXEL_FORMAT_RGB_888,
      HAL_PIXEL_FORMAT_RGB_565,
      HAL_PIXEL_FORMAT_BGRA_8888,
   };
   int i;

   adpy->configs = (struct android_config *)
      CALLOC(Elements(native_formats), sizeof(*adpy->configs));
   if (!adpy->configs)
      return FALSE;

   for (i = 0; i < Elements(native_formats); i++) {
      enum pipe_format color_format;
      struct android_config *aconf;

      color_format = get_pipe_format(native_formats[i]);
      if (color_format == PIPE_FORMAT_NONE ||
          !adpy->base.screen->is_format_supported(adpy->base.screen,
               color_format, PIPE_TEXTURE_2D, 0, PIPE_BIND_RENDER_TARGET)) {
         LOGI("skip unsupported native format 0x%x", native_formats[i]);
         continue;
      }

      aconf = &adpy->configs[adpy->num_configs++];
      /* only the back buffer */
      aconf->base.buffer_mask = 1 << NATIVE_ATTACHMENT_BACK_LEFT;
      aconf->base.color_format = color_format;
      aconf->base.window_bit = TRUE;

      aconf->base.native_visual_id = native_formats[i];
      aconf->base.native_visual_type = native_formats[i];
   }

   return TRUE;
}

static boolean
android_display_init_drm(struct native_display *ndpy)
{
   struct android_display *adpy = android_display(ndpy);
   const hw_module_t *mod;
   int fd, err;

#ifndef ANDROID_BACKEND_NO_DRM
   /* get the authorized fd from gralloc */
   err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &mod);
   if (!err) {
      const gralloc_module_t *gr = (gralloc_module_t *) mod;

      err = -EINVAL;
      if (gr->perform)
         err = gr->perform(gr, GRALLOC_MODULE_PERFORM_GET_DRM_FD, &fd);
   }
   if (!err && fd >= 0) {
      adpy->base.screen =
         adpy->event_handler->new_drm_screen(&adpy->base, NULL, fd);
   }
#endif

   if (adpy->base.screen) {
      LOGI("using DRM screen");
      return TRUE;
   }
   else {
      LOGW("failed to create DRM screen");
      LOGW("will fall back to other EGL drivers if any");
      return FALSE;
   }
}

static boolean
android_display_init_sw(struct native_display *ndpy)
{
   struct android_display *adpy = android_display(ndpy);
   struct sw_winsys *ws;

   ws = android_create_sw_winsys();
   if (ws) {
      adpy->base.screen =
         adpy->event_handler->new_sw_screen(&adpy->base, ws);
   }

   if (adpy->base.screen) {
      LOGI("using SW screen");
      return TRUE;
   }
   else {
      LOGE("failed to create SW screen");
      return FALSE;
   }
}

static boolean
android_display_init_screen(struct native_display *ndpy)
{
   struct android_display *adpy = android_display(ndpy);

   if (adpy->use_drm)
      android_display_init_drm(&adpy->base);
   else
      android_display_init_sw(&adpy->base);

   if (!adpy->base.screen)
      return FALSE;

   if (!android_display_init_configs(&adpy->base)) {
      adpy->base.screen->destroy(adpy->base.screen);
      adpy->base.screen = NULL;
      return FALSE;
   }

   return TRUE;
}

static void
android_display_destroy(struct native_display *ndpy)
{
   struct android_display *adpy = android_display(ndpy);

   FREE(adpy->configs);
   if (adpy->base.screen)
      adpy->base.screen->destroy(adpy->base.screen);
   FREE(adpy);
}

static const struct native_config **
android_display_get_configs(struct native_display *ndpy, int *num_configs)
{
   struct android_display *adpy = android_display(ndpy);
   const struct native_config **configs;
   int i;

   configs = (const struct native_config **)
      MALLOC(adpy->num_configs * sizeof(*configs));
   if (configs) {
      for (i = 0; i < adpy->num_configs; i++)
         configs[i] = (const struct native_config *) &adpy->configs[i];
      if (num_configs)
         *num_configs = adpy->num_configs;
   }

   return configs;
}

static int
android_display_get_param(struct native_display *ndpy,
                          enum native_param_type param)
{
   int val;

   switch (param) {
   case NATIVE_PARAM_PRESERVE_BUFFER:
      val = 1;
      break;
   default:
      val = 0;
      break;
   }

   return val;
}

static struct pipe_resource *
android_display_import_buffer(struct native_display *ndpy,
                              struct native_buffer *nbuf)
{
   struct android_display *adpy = android_display(ndpy);
   ANativeWindowBuffer *abuf;
   enum pipe_format format;
   struct pipe_resource templ;

   if (nbuf->type != NATIVE_BUFFER_ANDROID)
      return NULL;

   abuf = nbuf->u.android;

   if (!abuf || abuf->common.magic != ANDROID_NATIVE_BUFFER_MAGIC ||
       abuf->common.version != sizeof(*abuf)) {
      LOGE("invalid android native buffer");
      return NULL;
   }

   format = get_pipe_format(abuf->format);
   if (format == PIPE_FORMAT_NONE)
      return NULL;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = format;
   /* assume for texturing only */
   templ.bind = PIPE_BIND_SAMPLER_VIEW;
   templ.width0 = abuf->width;
   templ.height0 = abuf->height;
   templ.depth0 = 1;
   templ.array_size = 1;

   return import_buffer(adpy, &templ, abuf);
}

static boolean
android_display_export_buffer(struct native_display *ndpy,
                              struct pipe_resource *res,
                              struct native_buffer *nbuf)
{
   return FALSE;
}

static struct native_display_buffer android_display_buffer = {
   android_display_import_buffer,
   android_display_export_buffer
};

static struct android_display *
android_display_create(const struct native_event_handler *event_handler,
                       boolean use_sw)
{
   struct android_display *adpy;
   char value[PROPERTY_VALUE_MAX];
   boolean force_sw;

   /* check if SW renderer is forced */
   if (property_get("debug.mesa.software", value, NULL))
      force_sw = (atoi(value) != 0);
   else
      force_sw = debug_get_bool_option("EGL_SOFTWARE", FALSE);
   if (force_sw)
      use_sw = TRUE;

   adpy = CALLOC_STRUCT(android_display);
   if (!adpy)
      return NULL;

   adpy->event_handler = event_handler;
   adpy->use_drm = !use_sw;

   adpy->base.init_screen = android_display_init_screen;
   adpy->base.destroy = android_display_destroy;
   adpy->base.get_param = android_display_get_param;
   adpy->base.get_configs = android_display_get_configs;
   adpy->base.create_window_surface = android_display_create_window_surface;

   adpy->base.buffer = &android_display_buffer;

   return adpy;
}

static const struct native_event_handler *android_event_handler;

static struct native_display *
native_create_display(void *dpy, boolean use_sw)
{
   struct android_display *adpy;

   adpy = android_display_create(android_event_handler, use_sw);

   return (adpy) ? &adpy->base : NULL;
}

static const struct native_platform android_platform = {
   "Android", /* name */
   native_create_display
};

}; /* namespace android */

using namespace android;

static void
android_log(EGLint level, const char *msg)
{
   switch (level) {
   case _EGL_DEBUG:
      LOGD("%s", msg);
      break;
   case _EGL_INFO:
      LOGI("%s", msg);
      break;
   case _EGL_WARNING:
      LOGW("%s", msg);
      break;
   case _EGL_FATAL:
      LOG_FATAL("%s", msg);
      break;
   default:
      break;
   }
}

const struct native_platform *
native_get_android_platform(const struct native_event_handler *event_handler)
{
   android_event_handler = event_handler;
   /* use Android logger */
   _eglSetLogProc(android_log);

   return &android_platform;
}
