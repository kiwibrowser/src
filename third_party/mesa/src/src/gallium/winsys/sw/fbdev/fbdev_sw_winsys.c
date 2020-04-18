/*
 * Mesa 3-D graphics library
 * Version:  7.9
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

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include "pipe/p_compiler.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "state_tracker/sw_winsys.h"

#include "fbdev_sw_winsys.h"

struct fbdev_sw_displaytarget
{
   enum pipe_format format;
   unsigned width;
   unsigned height;
   unsigned stride;

   void *data;
   void *mapped;
};

struct fbdev_sw_winsys
{
   struct sw_winsys base;

   int fd;

   struct fb_fix_screeninfo finfo;
   unsigned rows;
   unsigned stride;
};

static INLINE struct fbdev_sw_displaytarget *
fbdev_sw_displaytarget(struct sw_displaytarget *dt)
{
   return (struct fbdev_sw_displaytarget *) dt;
}

static INLINE struct fbdev_sw_winsys *
fbdev_sw_winsys(struct sw_winsys *ws)
{
   return (struct fbdev_sw_winsys *) ws;
}

static void
fbdev_displaytarget_display(struct sw_winsys *ws,
                            struct sw_displaytarget *dt,
                            void *winsys_private)
{
   struct fbdev_sw_winsys *fbdev = fbdev_sw_winsys(ws);
   struct fbdev_sw_displaytarget *src = fbdev_sw_displaytarget(dt);
   const struct fbdev_sw_drawable *dst =
      (const struct fbdev_sw_drawable *) winsys_private;
   unsigned height, row_offset, row_len, i;
   void *fbmem;

   /* FIXME format conversion */
   if (dst->format != src->format) {
      assert(0);
      return;
   }

   height = dst->height;
   if (dst->y + dst->height > fbdev->rows) {
      /* nothing to copy */
      if (dst->y >= fbdev->rows)
         return;

      height = fbdev->rows - dst->y;
   }

   row_offset = util_format_get_stride(dst->format, dst->x);
   row_len = util_format_get_stride(dst->format, dst->width);
   if (row_offset + row_len > fbdev->stride) {
      /* nothing to copy */
      if (row_offset >= fbdev->stride)
         return;

      row_len = fbdev->stride - row_offset;
   }

   fbmem = mmap(0, fbdev->finfo.smem_len,
         PROT_WRITE, MAP_SHARED, fbdev->fd, 0);
   if (fbmem == MAP_FAILED)
      return;

   for (i = 0; i < height; i++) {
      char *from = (char *) src->data + src->stride * i;
      char *to = (char *) fbmem + fbdev->stride * (dst->y + i) + row_offset;

      memcpy(to, from, row_len);
   }

   munmap(fbmem, fbdev->finfo.smem_len);
}

static void
fbdev_displaytarget_unmap(struct sw_winsys *ws,
                           struct sw_displaytarget *dt)
{
   struct fbdev_sw_displaytarget *fbdt = fbdev_sw_displaytarget(dt);
   fbdt->mapped = NULL;
}

static void *
fbdev_displaytarget_map(struct sw_winsys *ws,
                        struct sw_displaytarget *dt,
                        unsigned flags)
{
   struct fbdev_sw_displaytarget *fbdt = fbdev_sw_displaytarget(dt);
   fbdt->mapped = fbdt->data;
   return fbdt->mapped;
}

static void
fbdev_displaytarget_destroy(struct sw_winsys *ws,
                            struct sw_displaytarget *dt)
{
   struct fbdev_sw_displaytarget *fbdt = fbdev_sw_displaytarget(dt);

   if (fbdt->data)
      align_free(fbdt->data);

   FREE(fbdt);
}

static struct sw_displaytarget *
fbdev_displaytarget_create(struct sw_winsys *ws,
                           unsigned tex_usage,
                           enum pipe_format format,
                           unsigned width, unsigned height,
                           unsigned alignment,
                           unsigned *stride)
{
   struct fbdev_sw_displaytarget *fbdt;
   unsigned nblocksy, size, format_stride;

   fbdt = CALLOC_STRUCT(fbdev_sw_displaytarget);
   if (!fbdt)
      return NULL;

   fbdt->format = format;
   fbdt->width = width;
   fbdt->height = height;

   format_stride = util_format_get_stride(format, width);
   fbdt->stride = align(format_stride, alignment);

   nblocksy = util_format_get_nblocksy(format, height);
   size = fbdt->stride * nblocksy;

   fbdt->data = align_malloc(size, alignment);
   if (!fbdt->data) {
      FREE(fbdt);
      return NULL;
   }

   *stride = fbdt->stride;

   return (struct sw_displaytarget *) fbdt;
}

static boolean
fbdev_is_displaytarget_format_supported(struct sw_winsys *ws,
                                        unsigned tex_usage,
                                        enum pipe_format format)
{
   return TRUE;
}

static void
fbdev_destroy(struct sw_winsys *ws)
{
   struct fbdev_sw_winsys *fbdev = fbdev_sw_winsys(ws);

   FREE(fbdev);
}

struct sw_winsys *
fbdev_create_sw_winsys(int fd)
{
   struct fbdev_sw_winsys *fbdev;

   fbdev = CALLOC_STRUCT(fbdev_sw_winsys);
   if (!fbdev)
      return NULL;

   fbdev->fd = fd;
   if (ioctl(fbdev->fd, FBIOGET_FSCREENINFO, &fbdev->finfo)) {
      FREE(fbdev);
      return NULL;
   }

   fbdev->rows = fbdev->finfo.smem_len / fbdev->finfo.line_length;
   fbdev->stride = fbdev->finfo.line_length;

   fbdev->base.destroy = fbdev_destroy;
   fbdev->base.is_displaytarget_format_supported =
      fbdev_is_displaytarget_format_supported;

   fbdev->base.displaytarget_create = fbdev_displaytarget_create;
   fbdev->base.displaytarget_destroy = fbdev_displaytarget_destroy;
   fbdev->base.displaytarget_map = fbdev_displaytarget_map;
   fbdev->base.displaytarget_unmap = fbdev_displaytarget_unmap;

   fbdev->base.displaytarget_display = fbdev_displaytarget_display;

   return &fbdev->base;
}
