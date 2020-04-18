/*
 * Mesa 3-D graphics library
 * Version:  7.12
 *
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
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "util/u_memory.h"
#include "util/u_format.h"
#include "state_tracker/sw_winsys.h"

#include <hardware/gralloc.h>
#include <utils/Errors.h>

#if ANDROID_VERSION < 0x0300
#include <private/ui/sw_gralloc_handle.h>
#endif

#include "android_sw_winsys.h"

struct android_sw_winsys
{
   struct sw_winsys base;

   const gralloc_module_t *grmod;
};

struct android_sw_displaytarget
{
   buffer_handle_t handle;
   int stride;
   int width, height;
   int usage; /* gralloc usage */

   void *mapped;
};

static INLINE struct android_sw_winsys *
android_sw_winsys(struct sw_winsys *ws)
{
   return (struct android_sw_winsys *) ws;
}

static INLINE struct android_sw_displaytarget *
android_sw_displaytarget(struct sw_displaytarget *dt)
{
   return (struct android_sw_displaytarget *) dt;
}

namespace android {

static void
android_displaytarget_display(struct sw_winsys *ws,
                              struct sw_displaytarget *dt,
                              void *context_private)
{
}

static struct sw_displaytarget *
android_displaytarget_create(struct sw_winsys *ws,
                             unsigned tex_usage,
                             enum pipe_format format,
                             unsigned width, unsigned height,
                             unsigned alignment,
                             unsigned *stride)
{
   return NULL;
}

static void
android_displaytarget_destroy(struct sw_winsys *ws,
                              struct sw_displaytarget *dt)
{
   struct android_sw_displaytarget *adt = android_sw_displaytarget(dt);

   assert(!adt->mapped);
   FREE(adt);
}

static void
android_displaytarget_unmap(struct sw_winsys *ws,
                            struct sw_displaytarget *dt)
{
   struct android_sw_winsys *droid = android_sw_winsys(ws);
   struct android_sw_displaytarget *adt = android_sw_displaytarget(dt);

#if ANDROID_VERSION < 0x0300
   /* try sw_gralloc first */
   if (adt->mapped && sw_gralloc_handle_t::validate(adt->handle) >= 0) {
      adt->mapped = NULL;
      return;
   }
#endif

   if (adt->mapped) {
      droid->grmod->unlock(droid->grmod, adt->handle);
      adt->mapped = NULL;
   }
}

static void *
android_displaytarget_map(struct sw_winsys *ws,
                          struct sw_displaytarget *dt,
                          unsigned flags)
{
   struct android_sw_winsys *droid = android_sw_winsys(ws);
   struct android_sw_displaytarget *adt = android_sw_displaytarget(dt);

#if ANDROID_VERSION < 0x0300
   /* try sw_gralloc first */
   if (sw_gralloc_handle_t::validate(adt->handle) >= 0) {
      const sw_gralloc_handle_t *swhandle =
         reinterpret_cast<const sw_gralloc_handle_t *>(adt->handle);
      adt->mapped = reinterpret_cast<void *>(swhandle->base);

      return adt->mapped;
   }
#endif

   if (!adt->mapped) {
      /* lock the buffer for CPU access */
      droid->grmod->lock(droid->grmod, adt->handle,
            adt->usage, 0, 0, adt->width, adt->height, &adt->mapped);
   }

   return adt->mapped;
}

static struct sw_displaytarget *
android_displaytarget_from_handle(struct sw_winsys *ws,
                                  const struct pipe_resource *templ,
                                  struct winsys_handle *whandle,
                                  unsigned *stride)
{
   struct android_winsys_handle *ahandle =
      (struct android_winsys_handle *) whandle;
   struct android_sw_displaytarget *adt;

   adt = CALLOC_STRUCT(android_sw_displaytarget);
   if (!adt)
      return NULL;

   adt->handle = ahandle->handle;
   adt->stride = ahandle->stride;
   adt->width = templ->width0;
   adt->height = templ->height0;

   if (templ->bind & (PIPE_BIND_RENDER_TARGET | PIPE_BIND_TRANSFER_WRITE))
      adt->usage |= GRALLOC_USAGE_SW_WRITE_OFTEN;
   if (templ->bind & (PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_TRANSFER_READ))
      adt->usage |= GRALLOC_USAGE_SW_READ_OFTEN;

   if (stride)
      *stride = adt->stride;

   return reinterpret_cast<struct sw_displaytarget *>(adt);
}

static boolean
android_displaytarget_get_handle(struct sw_winsys *ws,
                                 struct sw_displaytarget *dt,
                                 struct winsys_handle *whandle)
{
   return FALSE;
}

static boolean
android_is_displaytarget_format_supported(struct sw_winsys *ws,
                                          unsigned tex_usage,
                                          enum pipe_format format)
{
   struct android_sw_winsys *droid = android_sw_winsys(ws);
   int fmt = -1;

   switch (format) {
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      fmt = HAL_PIXEL_FORMAT_RGBA_8888;
      break;
   case PIPE_FORMAT_R8G8B8X8_UNORM:
      fmt = HAL_PIXEL_FORMAT_RGBX_8888;
      break;
   case PIPE_FORMAT_R8G8B8_UNORM:
      fmt = HAL_PIXEL_FORMAT_RGB_888;
      break;
   case PIPE_FORMAT_B5G6R5_UNORM:
      fmt = HAL_PIXEL_FORMAT_RGB_565;
      break;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      fmt = HAL_PIXEL_FORMAT_BGRA_8888;
      break;
   default:
      break;
   }

   return (fmt != -1);
}

static void
android_destroy(struct sw_winsys *ws)
{
   struct android_sw_winsys *droid = android_sw_winsys(ws);

   FREE(droid);
}

}; /* namespace android */

using namespace android;

struct sw_winsys *
android_create_sw_winsys(void)
{
   struct android_sw_winsys *droid;
   const hw_module_t *mod;

   droid = CALLOC_STRUCT(android_sw_winsys);
   if (!droid)
      return NULL;

   if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &mod)) {
      FREE(droid);
      return NULL;
   }

   droid->grmod = (const gralloc_module_t *) mod;

   droid->base.destroy = android_destroy;
   droid->base.is_displaytarget_format_supported =
      android_is_displaytarget_format_supported;

   droid->base.displaytarget_create = android_displaytarget_create;
   droid->base.displaytarget_destroy = android_displaytarget_destroy;
   droid->base.displaytarget_from_handle = android_displaytarget_from_handle;
   droid->base.displaytarget_get_handle = android_displaytarget_get_handle;

   droid->base.displaytarget_map = android_displaytarget_map;
   droid->base.displaytarget_unmap = android_displaytarget_unmap;
   droid->base.displaytarget_display = android_displaytarget_display;

   return &droid->base;
}
