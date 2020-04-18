/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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

#include "glhd_screen.h"
#include "glhd_objects.h"
#include "glhd_context.h"



struct pipe_resource *
galahad_resource_create(struct galahad_screen *glhd_screen,
                        struct pipe_resource *resource)
{
   struct galahad_resource *glhd_resource;

   if(!resource)
      goto error;

   assert(resource->screen == glhd_screen->screen);

   glhd_resource = CALLOC_STRUCT(galahad_resource);
   if(!glhd_resource)
      goto error;

   memcpy(&glhd_resource->base, resource, sizeof(struct pipe_resource));

   pipe_reference_init(&glhd_resource->base.reference, 1);
   glhd_resource->base.screen = &glhd_screen->base;
   glhd_resource->resource = resource;

   return &glhd_resource->base;

error:
   pipe_resource_reference(&resource, NULL);
   return NULL;
}

void
galahad_resource_destroy(struct galahad_resource *glhd_resource)
{
   pipe_resource_reference(&glhd_resource->resource, NULL);
   FREE(glhd_resource);
}


struct pipe_surface *
galahad_surface_create(struct galahad_context *glhd_context,
                        struct galahad_resource *glhd_resource,
                        struct pipe_surface *surface)
{
   struct galahad_surface *glhd_surface;

   if(!surface)
      goto error;

   assert(surface->texture == glhd_resource->resource);

   glhd_surface = CALLOC_STRUCT(galahad_surface);
   if(!glhd_surface)
      goto error;

   memcpy(&glhd_surface->base, surface, sizeof(struct pipe_surface));

   pipe_reference_init(&glhd_surface->base.reference, 1);
   glhd_surface->base.texture = NULL;
   pipe_resource_reference(&glhd_surface->base.texture, &glhd_resource->base);
   glhd_surface->surface = surface;

   return &glhd_surface->base;

error:
   pipe_surface_reference(&surface, NULL);
   return NULL;
}

void
galahad_surface_destroy(struct galahad_context *glhd_context,
                         struct galahad_surface *glhd_surface)
{
   pipe_resource_reference(&glhd_surface->base.texture, NULL);
   pipe_surface_reference(&glhd_surface->surface, NULL);
   FREE(glhd_surface);
}


struct pipe_sampler_view *
galahad_sampler_view_create(struct galahad_context *glhd_context,
                             struct galahad_resource *glhd_resource,
                             struct pipe_sampler_view *view)
{
   struct galahad_sampler_view *glhd_view;

   if (!view)
      goto error;

   assert(view->texture == glhd_resource->resource);

   glhd_view = CALLOC_STRUCT(galahad_sampler_view);

   glhd_view->base = *view;
   glhd_view->base.reference.count = 1;
   glhd_view->base.texture = NULL;
   pipe_resource_reference(&glhd_view->base.texture, &glhd_resource->base);
   glhd_view->base.context = &glhd_context->base;
   glhd_view->sampler_view = view;

   return &glhd_view->base;
error:
   return NULL;
}

void
galahad_sampler_view_destroy(struct galahad_context *glhd_context,
                              struct galahad_sampler_view *glhd_view)
{
   pipe_sampler_view_reference(&glhd_view->sampler_view, NULL);
   pipe_resource_reference(&glhd_view->base.texture, NULL);
   FREE(glhd_view);
}


struct pipe_transfer *
galahad_transfer_create(struct galahad_context *glhd_context,
                         struct galahad_resource *glhd_resource,
                         struct pipe_transfer *transfer)
{
   struct galahad_transfer *glhd_transfer;

   if(!transfer)
      goto error;

   assert(transfer->resource == glhd_resource->resource);

   glhd_transfer = CALLOC_STRUCT(galahad_transfer);
   if(!glhd_transfer)
      goto error;

   memcpy(&glhd_transfer->base, transfer, sizeof(struct pipe_transfer));

   glhd_transfer->base.resource = NULL;
   glhd_transfer->transfer = transfer;

   pipe_resource_reference(&glhd_transfer->base.resource, &glhd_resource->base);
   assert(glhd_transfer->base.resource == &glhd_resource->base);

   return &glhd_transfer->base;

error:
   glhd_context->pipe->transfer_destroy(glhd_context->pipe, transfer);
   return NULL;
}

void
galahad_transfer_destroy(struct galahad_context *glhd_context,
                          struct galahad_transfer *glhd_transfer)
{
   pipe_resource_reference(&glhd_transfer->base.resource, NULL);
   glhd_context->pipe->transfer_destroy(glhd_context->pipe,
                                        glhd_transfer->transfer);
   FREE(glhd_transfer);
}
