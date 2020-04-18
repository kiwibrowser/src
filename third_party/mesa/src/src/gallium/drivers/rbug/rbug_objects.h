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

#ifndef RBUG_OBJECTS_H
#define RBUG_OBJECTS_H


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"

#include "rbug_screen.h"

struct rbug_context;


struct rbug_resource
{
   struct pipe_resource base;

   struct pipe_resource *resource;

   struct rbug_list list;
};


enum rbug_shader_type
{
   RBUG_SHADER_GEOM,
   RBUG_SHADER_VERTEX,
   RBUG_SHADER_FRAGMENT,
};

struct rbug_shader
{
   struct rbug_list list;

   void *shader;
   void *tokens;
   void *replaced_shader;
   void *replaced_tokens;

   enum rbug_shader_type type;
   boolean disabled;
};


struct rbug_sampler_view
{
   struct pipe_sampler_view base;

   struct pipe_sampler_view *sampler_view;
};


struct rbug_surface
{
   struct pipe_surface base;

   struct pipe_surface *surface;
};


struct rbug_transfer
{
   struct pipe_transfer base;

   struct pipe_context *pipe;
   struct pipe_transfer *transfer;
};


static INLINE struct rbug_resource *
rbug_resource(struct pipe_resource *_resource)
{
   if (!_resource)
      return NULL;
   (void)rbug_screen(_resource->screen);
   return (struct rbug_resource *)_resource;
}

static INLINE struct rbug_sampler_view *
rbug_sampler_view(struct pipe_sampler_view *_sampler_view)
{
   if (!_sampler_view)
      return NULL;
   (void)rbug_resource(_sampler_view->texture);
   return (struct rbug_sampler_view *)_sampler_view;
}

static INLINE struct rbug_surface *
rbug_surface(struct pipe_surface *_surface)
{
   if (!_surface)
      return NULL;
   (void)rbug_resource(_surface->texture);
   return (struct rbug_surface *)_surface;
}

static INLINE struct rbug_transfer *
rbug_transfer(struct pipe_transfer *_transfer)
{
   if (!_transfer)
      return NULL;
   (void)rbug_resource(_transfer->resource);
   return (struct rbug_transfer *)_transfer;
}

static INLINE struct rbug_shader *
rbug_shader(void *_state)
{
   if (!_state)
      return NULL;
   return (struct rbug_shader *)_state;
}

static INLINE struct pipe_resource *
rbug_resource_unwrap(struct pipe_resource *_resource)
{
   if (!_resource)
      return NULL;
   return rbug_resource(_resource)->resource;
}

static INLINE struct pipe_sampler_view *
rbug_sampler_view_unwrap(struct pipe_sampler_view *_sampler_view)
{
   if (!_sampler_view)
      return NULL;
   return rbug_sampler_view(_sampler_view)->sampler_view;
}

static INLINE struct pipe_surface *
rbug_surface_unwrap(struct pipe_surface *_surface)
{
   if (!_surface)
      return NULL;
   return rbug_surface(_surface)->surface;
}

static INLINE struct pipe_transfer *
rbug_transfer_unwrap(struct pipe_transfer *_transfer)
{
   if (!_transfer)
      return NULL;
   return rbug_transfer(_transfer)->transfer;
}

static INLINE void *
rbug_shader_unwrap(void *_state)
{
   struct rbug_shader *shader;
   if (!_state)
      return NULL;

   shader = rbug_shader(_state);
   return shader->replaced_shader ? shader->replaced_shader : shader->shader;
}


struct pipe_resource *
rbug_resource_create(struct rbug_screen *rb_screen,
                     struct pipe_resource *resource);

void
rbug_resource_destroy(struct rbug_resource *rb_resource);

struct pipe_surface *
rbug_surface_create(struct rbug_context *rb_context,
                    struct rbug_resource *rb_resource,
                    struct pipe_surface *surface);

void
rbug_surface_destroy(struct rbug_context *rb_context,
                     struct rbug_surface *rb_surface);

struct pipe_sampler_view *
rbug_sampler_view_create(struct rbug_context *rb_context,
                         struct rbug_resource *rb_resource,
                         struct pipe_sampler_view *view);

void
rbug_sampler_view_destroy(struct rbug_context *rb_context,
                          struct rbug_sampler_view *rb_sampler_view);

struct pipe_transfer *
rbug_transfer_create(struct rbug_context *rb_context,
                     struct rbug_resource *rb_resource,
                     struct pipe_transfer *transfer);

void
rbug_transfer_destroy(struct rbug_context *rb_context,
                      struct rbug_transfer *rb_transfer);

void *
rbug_shader_create(struct rbug_context *rb_context,
                   const struct pipe_shader_state *state,
                   void *result, enum rbug_shader_type type);

void
rbug_shader_destroy(struct rbug_context *rb_context,
                    struct rbug_shader *rb_shader);


#endif /* RBUG_OBJECTS_H */
