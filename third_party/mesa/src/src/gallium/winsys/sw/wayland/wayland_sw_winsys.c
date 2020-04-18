/*
 * Mesa 3-D graphics library
 * Version:  7.11
 *
 * Copyright (C) 2011 Benjamin Franzke <benjaminfranzke@googlemail.com>
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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include "pipe/p_compiler.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "state_tracker/sw_winsys.h"

#include <wayland-client.h>
#include "wayland_sw_winsys.h"

struct wayland_sw_displaytarget
{
   int fd;
   unsigned size;

   unsigned width;
   unsigned height;
   unsigned stride;

   enum pipe_format format;

   void *map;
   unsigned map_count;
};

struct wayland_sw_winsys
{
   struct sw_winsys base;

   struct wl_display *display;
};

static INLINE struct wayland_sw_displaytarget *
wayland_sw_displaytarget(struct sw_displaytarget *dt)
{
   return (struct wayland_sw_displaytarget *) dt;
}

static INLINE struct wayland_sw_winsys *
wayland_sw_winsys(struct sw_winsys *ws)
{
   return (struct wayland_sw_winsys *) ws;
}

static void
wayland_displaytarget_display(struct sw_winsys *ws,
                              struct sw_displaytarget *dt,
                              void *context_private)
{
}

static void
wayland_displaytarget_unmap(struct sw_winsys *ws,
                            struct sw_displaytarget *dt)
{
   struct wayland_sw_displaytarget *wldt = wayland_sw_displaytarget(dt);

   wldt->map_count--;
   if (wldt->map_count > 0)
      return;

   munmap(wldt->map, wldt->size);
   wldt->map = NULL;
}

static void *
wayland_displaytarget_map(struct sw_winsys *ws,
                          struct sw_displaytarget *dt,
                          unsigned flags)
{
   struct wayland_sw_displaytarget *wldt = wayland_sw_displaytarget(dt);
   uint mmap_flags = 0;

   if (wldt->map) {
      wldt->map_count++;
      return wldt->map;
   }

   if (flags & PIPE_TRANSFER_READ)
      mmap_flags |= PROT_READ;
   if (flags & PIPE_TRANSFER_WRITE)
      mmap_flags |= PROT_WRITE;

   wldt->map = mmap(NULL, wldt->size, mmap_flags,
                    MAP_SHARED, wldt->fd, 0);

   if (wldt->map == MAP_FAILED)
      return NULL;

   wldt->map_count = 1;

   return wldt->map;
}

static void
wayland_displaytarget_destroy(struct sw_winsys *ws,
                              struct sw_displaytarget *dt)
{
   struct wayland_sw_displaytarget *wldt = wayland_sw_displaytarget(dt);

   if (wldt->map)
      wayland_displaytarget_unmap(ws, dt);

   FREE(wldt);
}

static boolean
wayland_is_displaytarget_format_supported(struct sw_winsys *ws,
                                          unsigned tex_usage,
                                          enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_B8G8R8X8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return TRUE;
   default:
      return FALSE;
   }
}

static struct sw_displaytarget *
wayland_displaytarget_create(struct sw_winsys *ws,
                             unsigned tex_usage,
                             enum pipe_format format,
                             unsigned width, unsigned height,
                             unsigned alignment,
                             unsigned *stride)
{
   struct wayland_sw_displaytarget *wldt;
   unsigned nblocksy, format_stride;
   char filename[] = "/tmp/wayland-shm-XXXXXX";

   if (!wayland_is_displaytarget_format_supported(ws, tex_usage, format))
      return NULL;

   wldt = CALLOC_STRUCT(wayland_sw_displaytarget);
   if (!wldt)
      return NULL;

   wldt->map = NULL;

   wldt->format = format;
   wldt->width = width;
   wldt->height = height;

   format_stride = util_format_get_stride(format, width);
   wldt->stride = align(format_stride, alignment);

   nblocksy = util_format_get_nblocksy(format, height);
   wldt->size = wldt->stride * nblocksy;

   wldt->fd = mkstemp(filename);
   if (wldt->fd < 0) {
      FREE(wldt);
      return NULL;
   }

   if (ftruncate(wldt->fd, wldt->size) < 0) {
      unlink(filename);
      close(wldt->fd);
      FREE(wldt);
      return NULL;
   }

   unlink(filename);

   *stride = wldt->stride;

   return (struct sw_displaytarget *) wldt;
}

static struct sw_displaytarget *
wayland_displaytarget_from_handle(struct sw_winsys *ws,
                                  const struct pipe_resource *templet,
                                  struct winsys_handle *whandle,
                                  unsigned *stride)
{
   struct wayland_sw_displaytarget *wldt;
   unsigned nblocksy;

   if (!wayland_is_displaytarget_format_supported(ws, 0, templet->format))
      return NULL;

   wldt = CALLOC_STRUCT(wayland_sw_displaytarget);
   if (!wldt)
      return NULL;
 
   wldt->fd = whandle->fd;
   wldt->stride = whandle->stride;
   wldt->width = templet->width0;
   wldt->height = templet->height0;
   wldt->format = templet->format;

   nblocksy = util_format_get_nblocksy(wldt->format, wldt->height);

   wldt->size = wldt->stride * nblocksy;

   wldt->map = NULL;

   *stride = wldt->stride;

   return (struct sw_displaytarget *) wldt;
}


static boolean
wayland_displaytarget_get_handle(struct sw_winsys *ws,
                                 struct sw_displaytarget *dt,
                                 struct winsys_handle *whandle)
{
   struct wayland_sw_displaytarget *wldt = wayland_sw_displaytarget(dt);

   whandle->fd = wldt->fd;
   whandle->stride = wldt->stride;
   whandle->size = wldt->size;

   return TRUE;
}

static void
wayland_destroy(struct sw_winsys *ws)
{
   struct wayland_sw_winsys *wayland = wayland_sw_winsys(ws);

   FREE(wayland);
}

struct sw_winsys *
wayland_create_sw_winsys(struct wl_display *display)
{
   struct wayland_sw_winsys *wlws;

   wlws = CALLOC_STRUCT(wayland_sw_winsys);
   if (!wlws)
      return NULL;

   wlws->display = display;

   wlws->base.destroy = wayland_destroy;
   wlws->base.is_displaytarget_format_supported =
      wayland_is_displaytarget_format_supported;

   wlws->base.displaytarget_create = wayland_displaytarget_create;
   wlws->base.displaytarget_from_handle = wayland_displaytarget_from_handle;
   wlws->base.displaytarget_get_handle = wayland_displaytarget_get_handle;
   wlws->base.displaytarget_destroy = wayland_displaytarget_destroy;
   wlws->base.displaytarget_map = wayland_displaytarget_map;
   wlws->base.displaytarget_unmap = wayland_displaytarget_unmap;

   wlws->base.displaytarget_display = wayland_displaytarget_display;

   return &wlws->base;
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
