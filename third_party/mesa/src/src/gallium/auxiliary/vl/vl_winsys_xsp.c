/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/* directly referenced from target Makefile, because of X dependencies */

#include <sys/time.h>

#include "pipe/p_state.h"

#include "util/u_memory.h"
#include "util/u_format.h"
#include "util/u_inlines.h"

#include "state_tracker/xlib_sw_winsys.h"
#include "softpipe/sp_public.h"

#include "vl/vl_compositor.h"
#include "vl/vl_winsys.h"

struct vl_xsp_screen
{
   struct vl_screen base;
   Display *display;
   int screen;
   Visual visual;
   struct xlib_drawable xdraw;
   struct pipe_resource *tex;
   struct u_rect dirty_area;
};

struct pipe_resource*
vl_screen_texture_from_drawable(struct vl_screen *vscreen, Drawable drawable)
{
   struct vl_xsp_screen *xsp_screen = (struct vl_xsp_screen*)vscreen;
   Window root;
   int x, y;
   unsigned int width, height;
   unsigned int border_width;
   unsigned int depth;
   struct pipe_resource templat;

   assert(vscreen);
   assert(drawable != None);

   if (XGetGeometry(xsp_screen->display, drawable, &root, &x, &y, &width, &height, &border_width, &depth) == BadDrawable)
      return NULL;

   xsp_screen->xdraw.drawable = drawable;

   if (xsp_screen->tex) {
      if (xsp_screen->tex->width0 == width && xsp_screen->tex->height0 == height)
         return xsp_screen->tex;
      pipe_resource_reference(&xsp_screen->tex, NULL);
      vl_compositor_reset_dirty_area(&xsp_screen->dirty_area);
   }

   memset(&templat, 0, sizeof(struct pipe_resource));
   templat.target = PIPE_TEXTURE_2D;
   /* XXX: Need to figure out drawable's format */
   templat.format = PIPE_FORMAT_B8G8R8X8_UNORM;
   templat.last_level = 0;
   templat.width0 = width;
   templat.height0 = height;
   templat.depth0 = 1;
   templat.usage = PIPE_USAGE_DEFAULT;
   templat.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_DISPLAY_TARGET;
   templat.flags = 0;

   xsp_screen->xdraw.depth = 24/*util_format_get_blocksizebits(templat.format) /
                             util_format_get_blockwidth(templat.format)*/;

   pipe_resource_reference(&xsp_screen->tex, vscreen->pscreen->resource_create(vscreen->pscreen, &templat));
   return xsp_screen->tex;
}

struct u_rect *
vl_screen_get_dirty_area(struct vl_screen *vscreen)
{
   struct vl_xsp_screen *xsp_screen = (struct vl_xsp_screen*)vscreen;
   return &xsp_screen->dirty_area;
}

uint64_t
vl_screen_get_timestamp(struct vl_screen *vscreen, Drawable drawable)
{
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return (uint64_t)tv.tv_sec * 1000000000LL + (uint64_t)tv.tv_usec * 1000LL;
}

void
vl_screen_set_next_timestamp(struct vl_screen *vscreen, uint64_t stamp)
{
   /* not supported on softpipe and so only a dummy */
}

void*
vl_screen_get_private(struct vl_screen *vscreen)
{
   struct vl_xsp_screen *xsp_screen = (struct vl_xsp_screen*)vscreen;
   return &xsp_screen->xdraw;
}

struct vl_screen*
vl_screen_create(Display *display, int screen)
{
   struct vl_xsp_screen *xsp_screen;
   struct sw_winsys *winsys;

   assert(display);

   xsp_screen = CALLOC_STRUCT(vl_xsp_screen);
   if (!xsp_screen)
      return NULL;

   winsys = xlib_create_sw_winsys(display);
   if (!winsys) {
      FREE(xsp_screen);
      return NULL;
   }

   xsp_screen->base.pscreen = softpipe_create_screen(winsys);
   if (!xsp_screen->base.pscreen) {
      winsys->destroy(winsys);
      FREE(xsp_screen);
      return NULL;
   }

   xsp_screen->display = display;
   xsp_screen->screen = screen;
   xsp_screen->xdraw.visual = XDefaultVisual(display, screen);
   vl_compositor_reset_dirty_area(&xsp_screen->dirty_area);

   return &xsp_screen->base;
}

void vl_screen_destroy(struct vl_screen *vscreen)
{
   struct vl_xsp_screen *xsp_screen = (struct vl_xsp_screen*)vscreen;

   assert(vscreen);

   pipe_resource_reference(&xsp_screen->tex, NULL);
   vscreen->pscreen->destroy(vscreen->pscreen);
   FREE(vscreen);
}
