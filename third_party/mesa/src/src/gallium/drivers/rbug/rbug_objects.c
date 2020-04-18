/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"

#include "tgsi/tgsi_parse.h"

#include "rbug_screen.h"
#include "rbug_objects.h"
#include "rbug_context.h"



struct pipe_resource *
rbug_resource_create(struct rbug_screen *rb_screen,
                     struct pipe_resource *resource)
{
   struct rbug_resource *rb_resource;

   if(!resource)
      goto error;

   assert(resource->screen == rb_screen->screen);

   rb_resource = CALLOC_STRUCT(rbug_resource);
   if(!rb_resource)
      goto error;

   memcpy(&rb_resource->base, resource, sizeof(struct pipe_resource));

   pipe_reference_init(&rb_resource->base.reference, 1);
   rb_resource->base.screen = &rb_screen->base;
   rb_resource->resource = resource;

   rbug_screen_add_to_list(rb_screen, resources, rb_resource);

   return &rb_resource->base;

error:
   pipe_resource_reference(&resource, NULL);
   return NULL;
}

void
rbug_resource_destroy(struct rbug_resource *rb_resource)
{
   struct rbug_screen *rb_screen = rbug_screen(rb_resource->base.screen);
   rbug_screen_remove_from_list(rb_screen, resources, rb_resource);

   pipe_resource_reference(&rb_resource->resource, NULL);
   FREE(rb_resource);
}


struct pipe_surface *
rbug_surface_create(struct rbug_context *rb_context,
                    struct rbug_resource *rb_resource,
                    struct pipe_surface *surface)
{
   struct rbug_surface *rb_surface;

   if(!surface)
      goto error;

   assert(surface->texture == rb_resource->resource);

   rb_surface = CALLOC_STRUCT(rbug_surface);
   if(!rb_surface)
      goto error;

   memcpy(&rb_surface->base, surface, sizeof(struct pipe_surface));

   pipe_reference_init(&rb_surface->base.reference, 1);
   rb_surface->base.texture = NULL;
   rb_surface->base.context = &rb_context->base;
   rb_surface->surface = surface; /* we own the surface already */
   pipe_resource_reference(&rb_surface->base.texture, &rb_resource->base);

   return &rb_surface->base;

error:
   pipe_surface_reference(&surface, NULL);
   return NULL;
}

void
rbug_surface_destroy(struct rbug_context *rb_context,
                     struct rbug_surface *rb_surface)
{
   pipe_resource_reference(&rb_surface->base.texture, NULL);
   pipe_surface_reference(&rb_surface->surface, NULL);
   FREE(rb_surface);
}


struct pipe_sampler_view *
rbug_sampler_view_create(struct rbug_context *rb_context,
                         struct rbug_resource *rb_resource,
                         struct pipe_sampler_view *view)
{
   struct rbug_sampler_view *rb_view;

   if (!view)
      goto error;

   assert(view->texture == rb_resource->resource);

   rb_view = MALLOC(sizeof(struct rbug_sampler_view));

   rb_view->base = *view;
   rb_view->base.reference.count = 1;
   rb_view->base.texture = NULL;
   pipe_resource_reference(&rb_view->base.texture, &rb_resource->base);
   rb_view->base.context = rb_context->pipe;
   rb_view->sampler_view = view;

   return &rb_view->base;
error:
   return NULL;
}

void
rbug_sampler_view_destroy(struct rbug_context *rb_context,
                          struct rbug_sampler_view *rb_view)
{
   pipe_resource_reference(&rb_view->base.texture, NULL);
   rb_context->pipe->sampler_view_destroy(rb_context->pipe,
                                          rb_view->sampler_view);
   FREE(rb_view);
}


struct pipe_transfer *
rbug_transfer_create(struct rbug_context *rb_context,
                     struct rbug_resource *rb_resource,
                     struct pipe_transfer *transfer)
{
   struct rbug_transfer *rb_transfer;

   if(!transfer)
      goto error;

   assert(transfer->resource == rb_resource->resource);

   rb_transfer = CALLOC_STRUCT(rbug_transfer);
   if(!rb_transfer)
      goto error;

   memcpy(&rb_transfer->base, transfer, sizeof(struct pipe_transfer));

   rb_transfer->base.resource = NULL;
   rb_transfer->transfer = transfer;
   rb_transfer->pipe = rb_context->pipe;

   pipe_resource_reference(&rb_transfer->base.resource, &rb_resource->base);
   assert(rb_transfer->base.resource == &rb_resource->base);

   return &rb_transfer->base;

error:
   rb_context->pipe->transfer_destroy(rb_context->pipe, transfer);
   return NULL;
}

void
rbug_transfer_destroy(struct rbug_context *rb_context,
                      struct rbug_transfer *rb_transfer)
{
   pipe_resource_reference(&rb_transfer->base.resource, NULL);
   rb_transfer->pipe->transfer_destroy(rb_context->pipe,
                                       rb_transfer->transfer);
   FREE(rb_transfer);
}

void *
rbug_shader_create(struct rbug_context *rb_context,
                   const struct pipe_shader_state *state,
                   void *result, enum rbug_shader_type type)
{
   struct rbug_shader *rb_shader = CALLOC_STRUCT(rbug_shader);

   rb_shader->type = type;
   rb_shader->shader = result;
   rb_shader->tokens = tgsi_dup_tokens(state->tokens);

   /* works on context as well since its just a macro */
   rbug_screen_add_to_list(rb_context, shaders, rb_shader);

   return rb_shader;
}

void
rbug_shader_destroy(struct rbug_context *rb_context,
                    struct rbug_shader *rb_shader)
{
   struct pipe_context *pipe = rb_context->pipe;

   /* works on context as well since its just a macro */
   rbug_screen_remove_from_list(rb_context, shaders, rb_shader);

   switch(rb_shader->type) {
   case RBUG_SHADER_FRAGMENT:
      if (rb_shader->replaced_shader)
         pipe->delete_fs_state(pipe, rb_shader->replaced_shader);
      pipe->delete_fs_state(pipe, rb_shader->shader);
      break;
   case RBUG_SHADER_VERTEX:
      if (rb_shader->replaced_shader)
         pipe->delete_vs_state(pipe, rb_shader->replaced_shader);
      pipe->delete_vs_state(pipe, rb_shader->shader);
      break;
   case RBUG_SHADER_GEOM:
      if (rb_shader->replaced_shader)
         pipe->delete_gs_state(pipe, rb_shader->replaced_shader);
      pipe->delete_gs_state(pipe, rb_shader->shader);
      break;
   default:
      assert(0);
   }

   FREE(rb_shader->replaced_tokens);
   FREE(rb_shader->tokens);
   FREE(rb_shader);
}
