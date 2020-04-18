/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "util/u_inlines.h"
#include "util/u_hash_table.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"

#include "tr_screen.h"
#include "tr_context.h"
#include "tr_texture.h"


struct pipe_resource *
trace_resource_create(struct trace_screen *tr_scr,
                     struct pipe_resource *texture)
{
   struct trace_resource *tr_res;

   if(!texture)
      goto error;

   assert(texture->screen == tr_scr->screen);

   tr_res = CALLOC_STRUCT(trace_resource);
   if(!tr_res)
      goto error;

   memcpy(&tr_res->base, texture, sizeof(struct pipe_resource));

   pipe_reference_init(&tr_res->base.reference, 1);
   tr_res->base.screen = &tr_scr->base;
   tr_res->resource = texture;

   return &tr_res->base;

error:
   pipe_resource_reference(&texture, NULL);
   return NULL;
}


void
trace_resource_destroy(struct trace_screen *tr_scr,
		       struct trace_resource *tr_res)
{
   pipe_resource_reference(&tr_res->resource, NULL);
   FREE(tr_res);
}


struct pipe_surface *
trace_surf_create(struct trace_resource *tr_res,
                  struct pipe_surface *surface)
{
   struct trace_surface *tr_surf;

   if(!surface)
      goto error;

   assert(surface->texture == tr_res->resource);

   tr_surf = CALLOC_STRUCT(trace_surface);
   if(!tr_surf)
      goto error;

   memcpy(&tr_surf->base, surface, sizeof(struct pipe_surface));

   pipe_reference_init(&tr_surf->base.reference, 1);
   tr_surf->base.texture = NULL;
   pipe_resource_reference(&tr_surf->base.texture, &tr_res->base);
   tr_surf->surface = surface;

   return &tr_surf->base;

error:
   pipe_surface_reference(&surface, NULL);
   return NULL;
}


void
trace_surf_destroy(struct trace_surface *tr_surf)
{
   pipe_resource_reference(&tr_surf->base.texture, NULL);
   pipe_surface_reference(&tr_surf->surface, NULL);
   FREE(tr_surf);
}


struct pipe_transfer *
trace_transfer_create(struct trace_context *tr_ctx,
		      struct trace_resource *tr_res,
		      struct pipe_transfer *transfer)
{
   struct trace_transfer *tr_trans;

   if(!transfer)
      goto error;

   assert(transfer->resource == tr_res->resource);

   tr_trans = CALLOC_STRUCT(trace_transfer);
   if(!tr_trans)
      goto error;

   memcpy(&tr_trans->base, transfer, sizeof(struct pipe_transfer));

   tr_trans->base.resource = NULL;
   tr_trans->transfer = transfer;

   pipe_resource_reference(&tr_trans->base.resource, &tr_res->base);
   assert(tr_trans->base.resource == &tr_res->base);

   return &tr_trans->base;

error:
   tr_ctx->pipe->transfer_destroy(tr_ctx->pipe, transfer);
   return NULL;
}


void
trace_transfer_destroy(struct trace_context *tr_context,
                       struct trace_transfer *tr_trans)
{
   struct pipe_context *context = tr_context->pipe;
   struct pipe_transfer *transfer = tr_trans->transfer;

   pipe_resource_reference(&tr_trans->base.resource, NULL);
   context->transfer_destroy(context, transfer);
   FREE(tr_trans);
}

