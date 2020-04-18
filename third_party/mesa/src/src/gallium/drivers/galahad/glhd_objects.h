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

#ifndef GLHD_OBJECTS_H
#define GLHD_OBJECTS_H


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"

#include "glhd_screen.h"

struct galahad_context;


struct galahad_resource
{
   struct pipe_resource base;

   struct pipe_resource *resource;

   int map_count;
};


struct galahad_sampler_view
{
   struct pipe_sampler_view base;

   struct pipe_sampler_view *sampler_view;
};


struct galahad_surface
{
   struct pipe_surface base;

   struct pipe_surface *surface;
};


struct galahad_transfer
{
   struct pipe_transfer base;

   struct pipe_transfer *transfer;
};


static INLINE struct galahad_resource *
galahad_resource(struct pipe_resource *_resource)
{
   if(!_resource)
      return NULL;
   (void)galahad_screen(_resource->screen);
   return (struct galahad_resource *)_resource;
}

static INLINE struct galahad_sampler_view *
galahad_sampler_view(struct pipe_sampler_view *_sampler_view)
{
   if (!_sampler_view) {
      return NULL;
   }
   return (struct galahad_sampler_view *)_sampler_view;
}

static INLINE struct galahad_surface *
galahad_surface(struct pipe_surface *_surface)
{
   if(!_surface)
      return NULL;
   (void)galahad_resource(_surface->texture);
   return (struct galahad_surface *)_surface;
}

static INLINE struct galahad_transfer *
galahad_transfer(struct pipe_transfer *_transfer)
{
   if(!_transfer)
      return NULL;
   (void)galahad_resource(_transfer->resource);
   return (struct galahad_transfer *)_transfer;
}

static INLINE struct pipe_resource *
galahad_resource_unwrap(struct pipe_resource *_resource)
{
   if(!_resource)
      return NULL;
   return galahad_resource(_resource)->resource;
}

static INLINE struct pipe_sampler_view *
galahad_sampler_view_unwrap(struct pipe_sampler_view *_sampler_view)
{
   if (!_sampler_view) {
      return NULL;
   }
   return galahad_sampler_view(_sampler_view)->sampler_view;
}

static INLINE struct pipe_surface *
galahad_surface_unwrap(struct pipe_surface *_surface)
{
   if(!_surface)
      return NULL;
   return galahad_surface(_surface)->surface;
}

static INLINE struct pipe_transfer *
galahad_transfer_unwrap(struct pipe_transfer *_transfer)
{
   if(!_transfer)
      return NULL;
   return galahad_transfer(_transfer)->transfer;
}


struct pipe_resource *
galahad_resource_create(struct galahad_screen *glhd_screen,
                         struct pipe_resource *resource);

void
galahad_resource_destroy(struct galahad_resource *glhd_resource);

struct pipe_surface *
galahad_surface_create(struct galahad_context *glhd_context,
                       struct galahad_resource *glhd_resource,
                        struct pipe_surface *surface);

void
galahad_surface_destroy(struct galahad_context *glhd_context,
                         struct galahad_surface *glhd_surface);

struct pipe_sampler_view *
galahad_sampler_view_create(struct galahad_context *glhd_context,
                             struct galahad_resource *glhd_resource,
                             struct pipe_sampler_view *view);

void
galahad_sampler_view_destroy(struct galahad_context *glhd_context,
                              struct galahad_sampler_view *glhd_sampler_view);

struct pipe_transfer *
galahad_transfer_create(struct galahad_context *glhd_context,
                         struct galahad_resource *glhd_resource,
                         struct pipe_transfer *transfer);

void
galahad_transfer_destroy(struct galahad_context *glhd_context,
                          struct galahad_transfer *glhd_transfer);


#endif /* GLHD_OBJECTS_H */
