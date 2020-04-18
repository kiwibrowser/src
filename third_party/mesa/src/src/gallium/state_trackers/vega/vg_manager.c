/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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

#include "state_tracker/st_api.h"

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_box.h"
#include "util/u_surface.h"

#include "vg_api.h"
#include "vg_manager.h"
#include "vg_context.h"
#include "api.h"
#include "handle.h"

static boolean
vg_context_update_color_rb(struct vg_context *ctx, struct pipe_resource *pt)
{
   struct st_renderbuffer *strb = ctx->draw_buffer->strb;
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_surface surf_tmpl;

   if (strb->texture == pt) {
      pipe_resource_reference(&pt, NULL);
      return FALSE;
   }

   /* unreference existing ones */
   pipe_surface_reference(&strb->surface, NULL);
   pipe_resource_reference(&strb->texture, NULL);
   strb->width = strb->height = 0;

   strb->texture = pt;

   memset(&surf_tmpl, 0, sizeof(surf_tmpl));
   u_surface_default_template(&surf_tmpl, strb->texture,
                              PIPE_BIND_RENDER_TARGET);
   strb->surface = pipe->create_surface(pipe, strb->texture, &surf_tmpl);

   if (!strb->surface) {
      pipe_resource_reference(&strb->texture, NULL);
      return TRUE;
   }

   strb->width = pt->width0;
   strb->height = pt->height0;

   return TRUE;
}

/**
 * Flush the front buffer if the current context renders to the front buffer.
 */
void
vg_manager_flush_frontbuffer(struct vg_context *ctx)
{
   struct st_framebuffer *stfb = ctx->draw_buffer;

   if (!stfb)
      return;

   switch (stfb->strb_att) {
   case ST_ATTACHMENT_FRONT_LEFT:
   case ST_ATTACHMENT_FRONT_RIGHT:
      stfb->iface->flush_front(stfb->iface, stfb->strb_att);
      break;
   default:
      break;
   }
}

/**
 * Re-validate the framebuffer.
 */
void
vg_manager_validate_framebuffer(struct vg_context *ctx)
{
   struct st_framebuffer *stfb = ctx->draw_buffer;
   struct pipe_resource *pt;
   int32_t new_stamp;

   /* no binding surface */
   if (!stfb)
      return;

   new_stamp = p_atomic_read(&stfb->iface->stamp);
   if (stfb->iface_stamp != new_stamp) {
      do {
	 /* validate the fb */
	 if (!stfb->iface->validate(stfb->iface, &stfb->strb_att,
				    1, &pt) || !pt)
	    return;

	 stfb->iface_stamp = new_stamp;
	 new_stamp = p_atomic_read(&stfb->iface->stamp);

      } while (stfb->iface_stamp != new_stamp);

      if (vg_context_update_color_rb(ctx, pt) ||
          stfb->width != pt->width0 ||
          stfb->height != pt->height0)
         ++stfb->stamp;

      stfb->width = pt->width0;
      stfb->height = pt->height0;
   }

   if (ctx->draw_stamp != stfb->stamp) {
      ctx->state.dirty |= FRAMEBUFFER_DIRTY;
      ctx->draw_stamp = stfb->stamp;
   }
}

static void
vg_context_flush(struct st_context_iface *stctxi, unsigned flags,
                 struct pipe_fence_handle **fence)
{
   struct vg_context *ctx = (struct vg_context *) stctxi;
   ctx->pipe->flush(ctx->pipe, fence);
   if (flags & ST_FLUSH_FRONT)
      vg_manager_flush_frontbuffer(ctx);
}

static void
vg_context_destroy(struct st_context_iface *stctxi)
{
   struct vg_context *ctx = (struct vg_context *) stctxi;
   struct pipe_context *pipe = ctx->pipe;

   vg_destroy_context(ctx);
   pipe->destroy(pipe);
}

static struct st_context_iface *
vg_api_create_context(struct st_api *stapi, struct st_manager *smapi,
                      const struct st_context_attribs *attribs,
                      enum st_context_error *error,
                      struct st_context_iface *shared_stctxi)
{
   struct vg_context *shared_ctx = (struct vg_context *) shared_stctxi;
   struct vg_context *ctx;
   struct pipe_context *pipe;

   if (!(stapi->profile_mask & (1 << attribs->profile))) {
      *error = ST_CONTEXT_ERROR_BAD_API;
      return NULL;
   }

   /* only 1.0 is supported */
   if (attribs->major > 1 || (attribs->major == 1 && attribs->minor > 0)) {
      *error = ST_CONTEXT_ERROR_BAD_VERSION;
      return NULL;
   }

   /* for VGHandle / pointer lookups */
   init_handles();

   pipe = smapi->screen->context_create(smapi->screen, NULL);
   if (!pipe) {
      *error = ST_CONTEXT_ERROR_NO_MEMORY;
      return NULL;
   }
   ctx = vg_create_context(pipe, NULL, shared_ctx);
   if (!ctx) {
      pipe->destroy(pipe);
      *error = ST_CONTEXT_ERROR_NO_MEMORY;
      return NULL;
   }

   ctx->iface.destroy = vg_context_destroy;

   ctx->iface.flush = vg_context_flush;

   ctx->iface.teximage = NULL;
   ctx->iface.copy = NULL;

   ctx->iface.st_context_private = (void *) smapi;

   return &ctx->iface;
}

static struct st_renderbuffer *
create_renderbuffer(enum pipe_format format)
{
   struct st_renderbuffer *strb;

   strb = CALLOC_STRUCT(st_renderbuffer);
   if (strb)
      strb->format = format;

   return strb;
}

static void
destroy_renderbuffer(struct st_renderbuffer *strb)
{
   pipe_surface_reference(&strb->surface, NULL);
   pipe_resource_reference(&strb->texture, NULL);
   FREE(strb);
}

/**
 * Decide the buffer to render to.
 */
static enum st_attachment_type
choose_attachment(struct st_framebuffer_iface *stfbi)
{
   enum st_attachment_type statt;

   statt = stfbi->visual->render_buffer;
   if (statt != ST_ATTACHMENT_INVALID) {
      /* use the buffer given by the visual, unless it is unavailable */
      if (!st_visual_have_buffers(stfbi->visual, 1 << statt)) {
         switch (statt) {
         case ST_ATTACHMENT_BACK_LEFT:
            statt = ST_ATTACHMENT_FRONT_LEFT;
            break;
         case ST_ATTACHMENT_BACK_RIGHT:
            statt = ST_ATTACHMENT_FRONT_RIGHT;
            break;
         default:
            break;
         }

         if (!st_visual_have_buffers(stfbi->visual, 1 << statt))
            statt = ST_ATTACHMENT_INVALID;
      }
   }

   return statt;
}

/**
 * Bind the context to the given framebuffers.
 */
static boolean
vg_context_bind_framebuffers(struct st_context_iface *stctxi,
                             struct st_framebuffer_iface *stdrawi,
                             struct st_framebuffer_iface *streadi)
{
   struct vg_context *ctx = (struct vg_context *) stctxi;
   struct st_framebuffer *stfb;
   enum st_attachment_type strb_att;

   /* the draw and read framebuffers must be the same */
   if (stdrawi != streadi)
      return FALSE;

   strb_att = (stdrawi) ? choose_attachment(stdrawi) : ST_ATTACHMENT_INVALID;

   if (ctx->draw_buffer) {
      stfb = ctx->draw_buffer;

      /* free the existing fb */
      if (!stdrawi ||
          stfb->strb_att != strb_att ||
          stfb->strb->format != stdrawi->visual->color_format) {
         destroy_renderbuffer(stfb->strb);
         destroy_renderbuffer(stfb->dsrb);
         FREE(stfb);

         ctx->draw_buffer = NULL;
      }
   }

   if (!stdrawi)
      return TRUE;

   if (strb_att == ST_ATTACHMENT_INVALID)
      return FALSE;

   /* create a new fb */
   if (!ctx->draw_buffer) {
      stfb = CALLOC_STRUCT(st_framebuffer);
      if (!stfb)
         return FALSE;

      stfb->strb = create_renderbuffer(stdrawi->visual->color_format);
      if (!stfb->strb) {
         FREE(stfb);
         return FALSE;
      }

      stfb->dsrb = create_renderbuffer(ctx->ds_format);
      if (!stfb->dsrb) {
         FREE(stfb->strb);
         FREE(stfb);
         return FALSE;
      }

      stfb->width = 0;
      stfb->height = 0;
      stfb->strb_att = strb_att;
      stfb->stamp = 1;
      stfb->iface_stamp = p_atomic_read(&stdrawi->stamp) - 1;

      ctx->draw_buffer = stfb;
   }

   ctx->draw_buffer->iface = stdrawi;
   ctx->draw_stamp = ctx->draw_buffer->stamp - 1;

   return TRUE;
}

static boolean
vg_api_make_current(struct st_api *stapi, struct st_context_iface *stctxi,
                    struct st_framebuffer_iface *stdrawi,
                    struct st_framebuffer_iface *streadi)
{
   struct vg_context *ctx = (struct vg_context *) stctxi;

   if (stctxi)
      vg_context_bind_framebuffers(stctxi, stdrawi, streadi);
   vg_set_current_context(ctx);

   return TRUE;
}

static struct st_context_iface *
vg_api_get_current(struct st_api *stapi)
{
   struct vg_context *ctx = vg_current_context();

   return (ctx) ? &ctx->iface : NULL;
}

static st_proc_t
vg_api_get_proc_address(struct st_api *stapi, const char *procname)
{
   return api_get_proc_address(procname);
}

static void
vg_api_destroy(struct st_api *stapi)
{
}

static const struct st_api vg_api = {
   "Vega " VEGA_VERSION_STRING,
   ST_API_OPENVG,
   ST_PROFILE_DEFAULT_MASK,
   0,
   vg_api_destroy,
   vg_api_get_proc_address,
   vg_api_create_context,
   vg_api_make_current,
   vg_api_get_current,
};

const struct st_api *
vg_api_get(void)
{
   return &vg_api;
}
