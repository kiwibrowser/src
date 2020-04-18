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

#include "xm_api.h"
#include "xm_st.h"

#include "util/u_inlines.h"
#include "util/u_atomic.h"

struct xmesa_st_framebuffer {
   XMesaDisplay display;
   XMesaBuffer buffer;
   struct pipe_screen *screen;

   struct st_visual stvis;
   enum pipe_texture_target target;

   unsigned texture_width, texture_height, texture_mask;
   struct pipe_resource *textures[ST_ATTACHMENT_COUNT];

   struct pipe_resource *display_resource;
};

static INLINE struct xmesa_st_framebuffer *
xmesa_st_framebuffer(struct st_framebuffer_iface *stfbi)
{
   return (struct xmesa_st_framebuffer *) stfbi->st_manager_private;
}

/**
 * Display an attachment to the xlib_drawable of the framebuffer.
 */
static boolean
xmesa_st_framebuffer_display(struct st_framebuffer_iface *stfbi,
                             enum st_attachment_type statt)
{
   struct xmesa_st_framebuffer *xstfb = xmesa_st_framebuffer(stfbi);
   struct pipe_resource *ptex = xstfb->textures[statt];
   struct pipe_resource *pres;

   if (!ptex)
      return TRUE;

   pres = xstfb->display_resource;
   /* (re)allocate the surface for the texture to be displayed */
   if (!pres || pres != ptex) {
      pipe_resource_reference(&xstfb->display_resource, ptex);
      pres = xstfb->display_resource;
   }

   xstfb->screen->flush_frontbuffer(xstfb->screen, pres, 0, 0, &xstfb->buffer->ws);

   return TRUE;
}

/**
 * Copy the contents between the attachments.
 */
static void
xmesa_st_framebuffer_copy_textures(struct st_framebuffer_iface *stfbi,
                                   enum st_attachment_type src_statt,
                                   enum st_attachment_type dst_statt,
                                   unsigned x, unsigned y,
                                   unsigned width, unsigned height)
{
   struct xmesa_st_framebuffer *xstfb = xmesa_st_framebuffer(stfbi);
   struct pipe_resource *src_ptex = xstfb->textures[src_statt];
   struct pipe_resource *dst_ptex = xstfb->textures[dst_statt];
   struct pipe_box src_box;
   struct pipe_context *pipe;

   if (!src_ptex || !dst_ptex)
      return;

   pipe = xmesa_get_context(stfbi);

   u_box_2d(x, y, width, height, &src_box);

   if (src_ptex && dst_ptex)
      pipe->resource_copy_region(pipe, dst_ptex, 0, x, y, 0,
                                 src_ptex, 0, &src_box);
}

/**
 * Remove outdated textures and create the requested ones.
 * This is a helper used during framebuffer validation.
 */
boolean
xmesa_st_framebuffer_validate_textures(struct st_framebuffer_iface *stfbi,
                                       unsigned width, unsigned height,
                                       unsigned mask)
{
   struct xmesa_st_framebuffer *xstfb = xmesa_st_framebuffer(stfbi);
   struct pipe_resource templ;
   enum st_attachment_type i;

   /* remove outdated textures */
   if (xstfb->texture_width != width || xstfb->texture_height != height) {
      for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
         pipe_resource_reference(&xstfb->textures[i], NULL);
   }

   memset(&templ, 0, sizeof(templ));
   templ.target = xstfb->target;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.last_level = 0;

   for (i = 0; i < ST_ATTACHMENT_COUNT; i++) {
      enum pipe_format format;
      unsigned bind;

      /* the texture already exists or not requested */
      if (xstfb->textures[i] || !(mask & (1 << i))) {
         /* remember the texture */
         if (xstfb->textures[i])
            mask |= (1 << i);
         continue;
      }

      switch (i) {
      case ST_ATTACHMENT_FRONT_LEFT:
      case ST_ATTACHMENT_BACK_LEFT:
      case ST_ATTACHMENT_FRONT_RIGHT:
      case ST_ATTACHMENT_BACK_RIGHT:
         format = xstfb->stvis.color_format;
         bind = PIPE_BIND_DISPLAY_TARGET |
                     PIPE_BIND_RENDER_TARGET;
         break;
      case ST_ATTACHMENT_DEPTH_STENCIL:
         format = xstfb->stvis.depth_stencil_format;
         bind = PIPE_BIND_DEPTH_STENCIL;
         break;
      default:
         format = PIPE_FORMAT_NONE;
         break;
      }

      if (format != PIPE_FORMAT_NONE) {
         templ.format = format;
         templ.bind = bind;

         xstfb->textures[i] =
            xstfb->screen->resource_create(xstfb->screen, &templ);
         if (!xstfb->textures[i])
            return FALSE;
      }
   }

   xstfb->texture_width = width;
   xstfb->texture_height = height;
   xstfb->texture_mask = mask;

   return TRUE;
}


/**
 * Check that a framebuffer's attachments match the window's size.
 *
 * Called via st_framebuffer_iface::validate()
 *
 * \param statts  array of framebuffer attachments
 * \param count  number of framebuffer attachments in statts[]
 * \param out  returns resources for each of the attachments
 */
static boolean 
xmesa_st_framebuffer_validate(struct st_framebuffer_iface *stfbi,
                              const enum st_attachment_type *statts,
                              unsigned count,
                              struct pipe_resource **out)
{
   struct xmesa_st_framebuffer *xstfb = xmesa_st_framebuffer(stfbi);
   unsigned statt_mask, new_mask, i;
   boolean resized;
   boolean ret;

   /* build mask of ST_ATTACHMENT bits */
   statt_mask = 0x0;
   for (i = 0; i < count; i++)
      statt_mask |= 1 << statts[i];

   /* record newly allocated textures */
   new_mask = statt_mask & ~xstfb->texture_mask;

   /* If xmesa_strict_invalidate is not set, we will not yet have
    * called XGetGeometry().  Do so here:
    */
   if (!xmesa_strict_invalidate)
      xmesa_check_buffer_size(xstfb->buffer);

   resized = (xstfb->buffer->width != xstfb->texture_width ||
              xstfb->buffer->height != xstfb->texture_height);

   /* revalidate textures */
   if (resized || new_mask) {
      ret = xmesa_st_framebuffer_validate_textures(stfbi,
                  xstfb->buffer->width, xstfb->buffer->height, statt_mask);
      if (!ret)
         return ret;

      if (!resized) {
         enum st_attachment_type back, front;

         back = ST_ATTACHMENT_BACK_LEFT;
         front = ST_ATTACHMENT_FRONT_LEFT;
         /* copy the contents if front is newly allocated and back is not */
         if ((statt_mask & (1 << back)) &&
             (new_mask & (1 << front)) &&
             !(new_mask & (1 << back))) {
            xmesa_st_framebuffer_copy_textures(stfbi, back, front,
                  0, 0, xstfb->texture_width, xstfb->texture_height);
         }
      }
   }

   for (i = 0; i < count; i++) {
      out[i] = NULL;
      pipe_resource_reference(&out[i], xstfb->textures[statts[i]]);
   }

   return TRUE;
}

/**
 * Called via st_framebuffer_iface::flush_front()
 */
static boolean
xmesa_st_framebuffer_flush_front(struct st_framebuffer_iface *stfbi,
                                 enum st_attachment_type statt)
{
   struct xmesa_st_framebuffer *xstfb = xmesa_st_framebuffer(stfbi);
   boolean ret;

   ret = xmesa_st_framebuffer_display(stfbi, statt);

   if (ret && xmesa_strict_invalidate)
      xmesa_check_buffer_size(xstfb->buffer);

   return ret;
}

struct st_framebuffer_iface *
xmesa_create_st_framebuffer(XMesaDisplay xmdpy, XMesaBuffer b)
{
   struct st_framebuffer_iface *stfbi;
   struct xmesa_st_framebuffer *xstfb;

   assert(xmdpy->display == b->xm_visual->display);

   stfbi = CALLOC_STRUCT(st_framebuffer_iface);
   xstfb = CALLOC_STRUCT(xmesa_st_framebuffer);
   if (!stfbi || !xstfb) {
      if (stfbi)
         FREE(stfbi);
      if (xstfb)
         FREE(xstfb);
      return NULL;
   }

   xstfb->display = xmdpy;
   xstfb->buffer = b;
   xstfb->screen = xmdpy->screen;
   xstfb->stvis = b->xm_visual->stvis;
   if(xstfb->screen->get_param(xstfb->screen, PIPE_CAP_NPOT_TEXTURES))
      xstfb->target = PIPE_TEXTURE_2D;
   else
      xstfb->target = PIPE_TEXTURE_RECT;

   stfbi->visual = &xstfb->stvis;
   stfbi->flush_front = xmesa_st_framebuffer_flush_front;
   stfbi->validate = xmesa_st_framebuffer_validate;
   p_atomic_set(&stfbi->stamp, 1);
   stfbi->st_manager_private = (void *) xstfb;

   return stfbi;
}

void
xmesa_destroy_st_framebuffer(struct st_framebuffer_iface *stfbi)
{
   struct xmesa_st_framebuffer *xstfb = xmesa_st_framebuffer(stfbi);
   int i;

   pipe_resource_reference(&xstfb->display_resource, NULL);

   for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
      pipe_resource_reference(&xstfb->textures[i], NULL);

   FREE(xstfb);
   FREE(stfbi);
}

void
xmesa_swap_st_framebuffer(struct st_framebuffer_iface *stfbi)
{
   struct xmesa_st_framebuffer *xstfb = xmesa_st_framebuffer(stfbi);
   boolean ret;

   ret = xmesa_st_framebuffer_display(stfbi, ST_ATTACHMENT_BACK_LEFT);
   if (ret) {
      struct pipe_resource **front, **back, *tmp;

      front = &xstfb->textures[ST_ATTACHMENT_FRONT_LEFT];
      back = &xstfb->textures[ST_ATTACHMENT_BACK_LEFT];
      /* swap textures only if the front texture has been allocated */
      if (*front) {
         tmp = *front;
         *front = *back;
         *back = tmp;

         /* the current context should validate the buffer after swapping */
         if (!xmesa_strict_invalidate)
            xmesa_notify_invalid_buffer(xstfb->buffer);
      }

      if (xmesa_strict_invalidate)
	 xmesa_check_buffer_size(xstfb->buffer);
   }
}

void
xmesa_copy_st_framebuffer(struct st_framebuffer_iface *stfbi,
                          enum st_attachment_type src,
                          enum st_attachment_type dst,
                          int x, int y, int w, int h)
{
   xmesa_st_framebuffer_copy_textures(stfbi, src, dst, x, y, w, h);
   if (dst == ST_ATTACHMENT_FRONT_LEFT)
      xmesa_st_framebuffer_display(stfbi, dst);
}

struct pipe_resource*
xmesa_get_attachment(struct st_framebuffer_iface *stfbi,
                     enum st_attachment_type st_attachment)
{
   struct xmesa_st_framebuffer *xstfb = xmesa_st_framebuffer(stfbi);
   struct pipe_resource* res;

   res = xstfb->textures[st_attachment];
   return res;
}

struct pipe_context*
xmesa_get_context(struct st_framebuffer_iface* stfbi)
{
   struct pipe_context *pipe;
   struct xmesa_st_framebuffer *xstfb = xmesa_st_framebuffer(stfbi);

   pipe = xstfb->display->pipe;
   if (!pipe) {
      pipe = xstfb->screen->context_create(xstfb->screen, NULL);
      if (!pipe)
         return NULL;
      xstfb->display->pipe = pipe;
   }
   return pipe;
}

