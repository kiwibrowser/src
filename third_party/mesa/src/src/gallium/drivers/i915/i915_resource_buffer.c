/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Michel Dänzer <michel@tungstengraphics.com>
  */

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "i915_context.h"
#include "i915_resource.h"



static boolean
i915_buffer_get_handle(struct pipe_screen *screen,
		       struct pipe_resource *resource,
		       struct winsys_handle *handle)
{
   return FALSE;
}

static void
i915_buffer_destroy(struct pipe_screen *screen,
		    struct pipe_resource *resource)
{
   struct i915_buffer *buffer = i915_buffer(resource);
   if (buffer->free_on_destroy)
      align_free(buffer->data);
   FREE(buffer);
}


static struct pipe_transfer *
i915_get_transfer(struct pipe_context *pipe,
                  struct pipe_resource *resource,
                  unsigned level,
                  unsigned usage,
                  const struct pipe_box *box)
{
   struct i915_context *i915 = i915_context(pipe);
   struct pipe_transfer *transfer = util_slab_alloc(&i915->transfer_pool);

   if (transfer == NULL)
      return NULL;

   transfer->resource = resource;
   transfer->level = level;
   transfer->usage = usage;
   transfer->box = *box;

   /* Note strides are zero, this is ok for buffers, but not for
    * textures 2d & higher at least. 
    */
   return transfer;
}

static void
i915_transfer_destroy(struct pipe_context *pipe,
                      struct pipe_transfer *transfer)
{
   struct i915_context *i915 = i915_context(pipe);
   util_slab_free(&i915->transfer_pool, transfer);
}

static void *
i915_buffer_transfer_map( struct pipe_context *pipe,
                          struct pipe_transfer *transfer )
{
   struct i915_buffer *buffer = i915_buffer(transfer->resource);
   return buffer->data + transfer->box.x;
}


static void
i915_buffer_transfer_inline_write( struct pipe_context *rm_ctx,
                                   struct pipe_resource *resource,
                                   unsigned level,
                                   unsigned usage,
                                   const struct pipe_box *box,
                                   const void *data,
                                   unsigned stride,
                                   unsigned layer_stride)
{
   struct i915_buffer *buffer = i915_buffer(resource);

   memcpy(buffer->data + box->x,
          data,
          box->width);
}


struct u_resource_vtbl i915_buffer_vtbl = 
{
   i915_buffer_get_handle,	     /* get_handle */
   i915_buffer_destroy,		     /* resource_destroy */
   i915_get_transfer,		     /* get_transfer */
   i915_transfer_destroy,	     /* transfer_destroy */
   i915_buffer_transfer_map,	     /* transfer_map */
   u_default_transfer_flush_region,  /* transfer_flush_region */
   u_default_transfer_unmap,	     /* transfer_unmap */
   i915_buffer_transfer_inline_write /* transfer_inline_write */
};



struct pipe_resource *
i915_buffer_create(struct pipe_screen *screen,
                    const struct pipe_resource *template)
{
   struct i915_buffer *buf = CALLOC_STRUCT(i915_buffer);

   if (!buf)
      return NULL;

   buf->b.b = *template;
   buf->b.vtbl = &i915_buffer_vtbl;
   pipe_reference_init(&buf->b.b.reference, 1);
   buf->b.b.screen = screen;
   buf->data = align_malloc(template->width0, 16);
   buf->free_on_destroy = TRUE;

   if (!buf->data)
      goto err;

   return &buf->b.b;

err:
   FREE(buf);
   return NULL;
}



struct pipe_resource *
i915_user_buffer_create(struct pipe_screen *screen,
                        void *ptr,
                        unsigned bytes,
                        unsigned bind)
{
   struct i915_buffer *buf = CALLOC_STRUCT(i915_buffer);

   if (!buf)
      return NULL;

   pipe_reference_init(&buf->b.b.reference, 1);
   buf->b.vtbl = &i915_buffer_vtbl;
   buf->b.b.screen = screen;
   buf->b.b.format = PIPE_FORMAT_R8_UNORM; /* ?? */
   buf->b.b.usage = PIPE_USAGE_IMMUTABLE;
   buf->b.b.bind = bind;
   buf->b.b.flags = 0;
   buf->b.b.width0 = bytes;
   buf->b.b.height0 = 1;
   buf->b.b.depth0 = 1;
   buf->b.b.array_size = 1;

   buf->data = ptr;
   buf->free_on_destroy = FALSE;

   return &buf->b.b;
}
