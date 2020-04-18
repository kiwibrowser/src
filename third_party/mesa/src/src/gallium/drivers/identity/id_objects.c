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

#include "id_screen.h"
#include "id_objects.h"
#include "id_context.h"



struct pipe_resource *
identity_resource_create(struct identity_screen *id_screen,
                        struct pipe_resource *resource)
{
   struct identity_resource *id_resource;

   if(!resource)
      goto error;

   assert(resource->screen == id_screen->screen);

   id_resource = CALLOC_STRUCT(identity_resource);
   if(!id_resource)
      goto error;

   memcpy(&id_resource->base, resource, sizeof(struct pipe_resource));

   pipe_reference_init(&id_resource->base.reference, 1);
   id_resource->base.screen = &id_screen->base;
   id_resource->resource = resource;

   return &id_resource->base;

error:
   pipe_resource_reference(&resource, NULL);
   return NULL;
}

void
identity_resource_destroy(struct identity_resource *id_resource)
{
   pipe_resource_reference(&id_resource->resource, NULL);
   FREE(id_resource);
}


struct pipe_surface *
identity_surface_create(struct identity_context *id_context,
                        struct identity_resource *id_resource,
                        struct pipe_surface *surface)
{
   struct identity_surface *id_surface;

   if(!surface)
      goto error;

   assert(surface->texture == id_resource->resource);

   id_surface = CALLOC_STRUCT(identity_surface);
   if(!id_surface)
      goto error;

   memcpy(&id_surface->base, surface, sizeof(struct pipe_surface));

   pipe_reference_init(&id_surface->base.reference, 1);
   id_surface->base.texture = NULL;
   pipe_resource_reference(&id_surface->base.texture, &id_resource->base);
   id_surface->surface = surface;

   return &id_surface->base;

error:
   pipe_surface_reference(&surface, NULL);
   return NULL;
}

void
identity_surface_destroy(struct identity_context *id_context,
                         struct identity_surface *id_surface)
{
   pipe_resource_reference(&id_surface->base.texture, NULL);
   id_context->pipe->surface_destroy(id_context->pipe,
                                     id_surface->surface);
   FREE(id_surface);
}


struct pipe_sampler_view *
identity_sampler_view_create(struct identity_context *id_context,
                             struct identity_resource *id_resource,
                             struct pipe_sampler_view *view)
{
   struct identity_sampler_view *id_view;

   if (!view)
      goto error;

   assert(view->texture == id_resource->resource);

   id_view = CALLOC_STRUCT(identity_sampler_view);

   id_view->base = *view;
   id_view->base.reference.count = 1;
   id_view->base.texture = NULL;
   pipe_resource_reference(&id_view->base.texture, id_resource->resource);
   id_view->base.context = id_context->pipe;
   id_view->sampler_view = view;

   return &id_view->base;
error:
   return NULL;
}

void
identity_sampler_view_destroy(struct identity_context *id_context,
                              struct identity_sampler_view *id_view)
{
   pipe_resource_reference(&id_view->base.texture, NULL);
   id_context->pipe->sampler_view_destroy(id_context->pipe,
                                          id_view->sampler_view);
   FREE(id_view);
}


struct pipe_transfer *
identity_transfer_create(struct identity_context *id_context,
                         struct identity_resource *id_resource,
                         struct pipe_transfer *transfer)
{
   struct identity_transfer *id_transfer;

   if(!transfer)
      goto error;

   assert(transfer->resource == id_resource->resource);

   id_transfer = CALLOC_STRUCT(identity_transfer);
   if(!id_transfer)
      goto error;

   memcpy(&id_transfer->base, transfer, sizeof(struct pipe_transfer));

   id_transfer->base.resource = NULL;
   id_transfer->transfer = transfer;

   pipe_resource_reference(&id_transfer->base.resource, &id_resource->base);
   assert(id_transfer->base.resource == &id_resource->base);

   return &id_transfer->base;

error:
   id_context->pipe->transfer_destroy(id_context->pipe, transfer);
   return NULL;
}

void
identity_transfer_destroy(struct identity_context *id_context,
                          struct identity_transfer *id_transfer)
{
   pipe_resource_reference(&id_transfer->base.resource, NULL);
   id_context->pipe->transfer_destroy(id_context->pipe,
                                      id_transfer->transfer);
   FREE(id_transfer);
}

