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

#ifndef ID_OBJECTS_H
#define ID_OBJECTS_H


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"

#include "id_screen.h"

struct identity_context;


struct identity_resource
{
   struct pipe_resource base;

   struct pipe_resource *resource;
};


struct identity_sampler_view
{
   struct pipe_sampler_view base;

   struct pipe_sampler_view *sampler_view;
};


struct identity_surface
{
   struct pipe_surface base;

   struct pipe_surface *surface;
};


struct identity_transfer
{
   struct pipe_transfer base;

   struct pipe_transfer *transfer;
};


static INLINE struct identity_resource *
identity_resource(struct pipe_resource *_resource)
{
   if(!_resource)
      return NULL;
   (void)identity_screen(_resource->screen);
   return (struct identity_resource *)_resource;
}

static INLINE struct identity_sampler_view *
identity_sampler_view(struct pipe_sampler_view *_sampler_view)
{
   if (!_sampler_view) {
      return NULL;
   }
   return (struct identity_sampler_view *)_sampler_view;
}

static INLINE struct identity_surface *
identity_surface(struct pipe_surface *_surface)
{
   if(!_surface)
      return NULL;
   (void)identity_resource(_surface->texture);
   return (struct identity_surface *)_surface;
}

static INLINE struct identity_transfer *
identity_transfer(struct pipe_transfer *_transfer)
{
   if(!_transfer)
      return NULL;
   (void)identity_resource(_transfer->resource);
   return (struct identity_transfer *)_transfer;
}

static INLINE struct pipe_resource *
identity_resource_unwrap(struct pipe_resource *_resource)
{
   if(!_resource)
      return NULL;
   return identity_resource(_resource)->resource;
}

static INLINE struct pipe_sampler_view *
identity_sampler_view_unwrap(struct pipe_sampler_view *_sampler_view)
{
   if (!_sampler_view) {
      return NULL;
   }
   return identity_sampler_view(_sampler_view)->sampler_view;
}

static INLINE struct pipe_surface *
identity_surface_unwrap(struct pipe_surface *_surface)
{
   if(!_surface)
      return NULL;
   return identity_surface(_surface)->surface;
}

static INLINE struct pipe_transfer *
identity_transfer_unwrap(struct pipe_transfer *_transfer)
{
   if(!_transfer)
      return NULL;
   return identity_transfer(_transfer)->transfer;
}


struct pipe_resource *
identity_resource_create(struct identity_screen *id_screen,
                         struct pipe_resource *resource);

void
identity_resource_destroy(struct identity_resource *id_resource);

struct pipe_surface *
identity_surface_create(struct identity_context *id_context,
                        struct identity_resource *id_resource,
                        struct pipe_surface *surface);

void
identity_surface_destroy(struct identity_context *id_context,
                         struct identity_surface *id_surface);

struct pipe_sampler_view *
identity_sampler_view_create(struct identity_context *id_context,
                             struct identity_resource *id_resource,
                             struct pipe_sampler_view *view);

void
identity_sampler_view_destroy(struct identity_context *id_context,
                              struct identity_sampler_view *id_sampler_view);

struct pipe_transfer *
identity_transfer_create(struct identity_context *id_context,
                         struct identity_resource *id_resource,
                         struct pipe_transfer *transfer);

void
identity_transfer_destroy(struct identity_context *id_context,
                          struct identity_transfer *id_transfer);


#endif /* ID_OBJECTS_H */
