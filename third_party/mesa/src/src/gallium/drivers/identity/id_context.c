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


#include "pipe/p_context.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"

#include "id_context.h"
#include "id_objects.h"


static void
identity_destroy(struct pipe_context *_pipe)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->destroy(pipe);

   FREE(id_pipe);
}

static void
identity_draw_vbo(struct pipe_context *_pipe,
                  const struct pipe_draw_info *info)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->draw_vbo(pipe, info);
}

static struct pipe_query *
identity_create_query(struct pipe_context *_pipe,
                      unsigned query_type)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->create_query(pipe,
                             query_type);
}

static void
identity_destroy_query(struct pipe_context *_pipe,
                       struct pipe_query *query)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->destroy_query(pipe,
                       query);
}

static void
identity_begin_query(struct pipe_context *_pipe,
                     struct pipe_query *query)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->begin_query(pipe,
                     query);
}

static void
identity_end_query(struct pipe_context *_pipe,
                   struct pipe_query *query)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->end_query(pipe,
                   query);
}

static boolean
identity_get_query_result(struct pipe_context *_pipe,
                          struct pipe_query *query,
                          boolean wait,
                          union pipe_query_result *result)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->get_query_result(pipe,
                                 query,
                                 wait,
                                 result);
}

static void *
identity_create_blend_state(struct pipe_context *_pipe,
                            const struct pipe_blend_state *blend)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->create_blend_state(pipe,
                                   blend);
}

static void
identity_bind_blend_state(struct pipe_context *_pipe,
                          void *blend)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->bind_blend_state(pipe,
                              blend);
}

static void
identity_delete_blend_state(struct pipe_context *_pipe,
                            void *blend)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->delete_blend_state(pipe,
                            blend);
}

static void *
identity_create_sampler_state(struct pipe_context *_pipe,
                              const struct pipe_sampler_state *sampler)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->create_sampler_state(pipe,
                                     sampler);
}

static void
identity_bind_sampler_states(struct pipe_context *_pipe,
                             unsigned shader,
                             unsigned start,
                             unsigned num_samplers,
                             void **samplers)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   /* remove this when we have pipe->bind_sampler_states(..., start, ...) */
   assert(start == 0);

   switch (shader) {
   case PIPE_SHADER_VERTEX:
      pipe->bind_vertex_sampler_states(pipe, num_samplers, samplers);
      break;
   case PIPE_SHADER_GEOMETRY:
      pipe->bind_geometry_sampler_states(pipe, num_samplers, samplers);
      break;
   case PIPE_SHADER_FRAGMENT:
      pipe->bind_fragment_sampler_states(pipe, num_samplers, samplers);
      break;
   default:
      debug_error("Unexpected shader in identity_bind_sampler_states()");
   }
}

static void
identity_bind_fragment_sampler_states(struct pipe_context *_pipe,
                                      unsigned num_samplers,
                                      void **samplers)
{
   identity_bind_sampler_states(_pipe, PIPE_SHADER_FRAGMENT,
                                0, num_samplers, samplers);
}

static void
identity_bind_vertex_sampler_states(struct pipe_context *_pipe,
                                    unsigned num_samplers,
                                    void **samplers)
{
   identity_bind_sampler_states(_pipe, PIPE_SHADER_VERTEX,
                                0, num_samplers, samplers);
}

static void
identity_delete_sampler_state(struct pipe_context *_pipe,
                              void *sampler)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->delete_sampler_state(pipe,
                              sampler);
}

static void *
identity_create_rasterizer_state(struct pipe_context *_pipe,
                                 const struct pipe_rasterizer_state *rasterizer)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->create_rasterizer_state(pipe,
                                        rasterizer);
}

static void
identity_bind_rasterizer_state(struct pipe_context *_pipe,
                               void *rasterizer)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->bind_rasterizer_state(pipe,
                               rasterizer);
}

static void
identity_delete_rasterizer_state(struct pipe_context *_pipe,
                                 void *rasterizer)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->delete_rasterizer_state(pipe,
                                 rasterizer);
}

static void *
identity_create_depth_stencil_alpha_state(struct pipe_context *_pipe,
                                          const struct pipe_depth_stencil_alpha_state *depth_stencil_alpha)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->create_depth_stencil_alpha_state(pipe,
                                                 depth_stencil_alpha);
}

static void
identity_bind_depth_stencil_alpha_state(struct pipe_context *_pipe,
                                        void *depth_stencil_alpha)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->bind_depth_stencil_alpha_state(pipe,
                                        depth_stencil_alpha);
}

static void
identity_delete_depth_stencil_alpha_state(struct pipe_context *_pipe,
                                          void *depth_stencil_alpha)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->delete_depth_stencil_alpha_state(pipe,
                                          depth_stencil_alpha);
}

static void *
identity_create_fs_state(struct pipe_context *_pipe,
                         const struct pipe_shader_state *fs)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->create_fs_state(pipe,
                                fs);
}

static void
identity_bind_fs_state(struct pipe_context *_pipe,
                       void *fs)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->bind_fs_state(pipe,
                       fs);
}

static void
identity_delete_fs_state(struct pipe_context *_pipe,
                         void *fs)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->delete_fs_state(pipe,
                         fs);
}

static void *
identity_create_vs_state(struct pipe_context *_pipe,
                         const struct pipe_shader_state *vs)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->create_vs_state(pipe,
                                vs);
}

static void
identity_bind_vs_state(struct pipe_context *_pipe,
                       void *vs)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->bind_vs_state(pipe,
                       vs);
}

static void
identity_delete_vs_state(struct pipe_context *_pipe,
                         void *vs)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->delete_vs_state(pipe,
                         vs);
}


static void *
identity_create_vertex_elements_state(struct pipe_context *_pipe,
                                      unsigned num_elements,
                                      const struct pipe_vertex_element *vertex_elements)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->create_vertex_elements_state(pipe,
                                             num_elements,
                                             vertex_elements);
}

static void
identity_bind_vertex_elements_state(struct pipe_context *_pipe,
                                    void *velems)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->bind_vertex_elements_state(pipe,
                                    velems);
}

static void
identity_delete_vertex_elements_state(struct pipe_context *_pipe,
                                      void *velems)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->delete_vertex_elements_state(pipe,
                                      velems);
}

static void
identity_set_blend_color(struct pipe_context *_pipe,
                         const struct pipe_blend_color *blend_color)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->set_blend_color(pipe,
                         blend_color);
}

static void
identity_set_stencil_ref(struct pipe_context *_pipe,
                         const struct pipe_stencil_ref *stencil_ref)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->set_stencil_ref(pipe,
                         stencil_ref);
}

static void
identity_set_clip_state(struct pipe_context *_pipe,
                        const struct pipe_clip_state *clip)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->set_clip_state(pipe,
                        clip);
}

static void
identity_set_sample_mask(struct pipe_context *_pipe,
                         unsigned sample_mask)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->set_sample_mask(pipe,
                         sample_mask);
}

static void
identity_set_constant_buffer(struct pipe_context *_pipe,
                             uint shader,
                             uint index,
                             struct pipe_constant_buffer *_cb)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_constant_buffer cb;

   /* XXX hmm? unwrap the input state */
   if (_cb) {
      cb = *_cb;
      cb.buffer = identity_resource_unwrap(_cb->buffer);
   }

   pipe->set_constant_buffer(pipe,
                             shader,
                             index,
                             _cb ? &cb : NULL);
}

static void
identity_set_framebuffer_state(struct pipe_context *_pipe,
                               const struct pipe_framebuffer_state *_state)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_framebuffer_state unwrapped_state;
   struct pipe_framebuffer_state *state = NULL;
   unsigned i;

   /* unwrap the input state */
   if (_state) {
      memcpy(&unwrapped_state, _state, sizeof(unwrapped_state));
      for(i = 0; i < _state->nr_cbufs; i++)
         unwrapped_state.cbufs[i] = identity_surface_unwrap(_state->cbufs[i]);
      for (; i < PIPE_MAX_COLOR_BUFS; i++)
         unwrapped_state.cbufs[i] = NULL;
      unwrapped_state.zsbuf = identity_surface_unwrap(_state->zsbuf);
      state = &unwrapped_state;
   }

   pipe->set_framebuffer_state(pipe,
                               state);
}

static void
identity_set_polygon_stipple(struct pipe_context *_pipe,
                             const struct pipe_poly_stipple *poly_stipple)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->set_polygon_stipple(pipe,
                             poly_stipple);
}

static void
identity_set_scissor_state(struct pipe_context *_pipe,
                           const struct pipe_scissor_state *scissor)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->set_scissor_state(pipe,
                           scissor);
}

static void
identity_set_viewport_state(struct pipe_context *_pipe,
                            const struct pipe_viewport_state *viewport)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->set_viewport_state(pipe,
                            viewport);
}

static void
identity_set_sampler_views(struct pipe_context *_pipe,
                           unsigned shader,
                           unsigned start,
                           unsigned num,
                           struct pipe_sampler_view **_views)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_sampler_view *unwrapped_views[PIPE_MAX_SAMPLERS];
   struct pipe_sampler_view **views = NULL;
   unsigned i;

   /* remove this when we have pipe->set_sampler_views(..., start, ...) */
   assert(start == 0);

   if (_views) {
      for (i = 0; i < num; i++)
         unwrapped_views[i] = identity_sampler_view_unwrap(_views[i]);
      for (; i < PIPE_MAX_SAMPLERS; i++)
         unwrapped_views[i] = NULL;

      views = unwrapped_views;
   }

   switch (shader) {
   case PIPE_SHADER_VERTEX:
      pipe->set_vertex_sampler_views(pipe, num, views);
      break;
   case PIPE_SHADER_GEOMETRY:
      pipe->set_geometry_sampler_views(pipe, num, views);
      break;
   case PIPE_SHADER_FRAGMENT:
      pipe->set_fragment_sampler_views(pipe, num, views);
      break;
   default:
      debug_error("Unexpected shader in identity_set_sampler_views()");
   }
}

static void
identity_set_fragment_sampler_views(struct pipe_context *_pipe,
                                    unsigned num,
                                    struct pipe_sampler_view **_views)
{
   identity_set_sampler_views(_pipe, PIPE_SHADER_FRAGMENT, 0, num, _views);
}

static void
identity_set_vertex_sampler_views(struct pipe_context *_pipe,
                                  unsigned num,
                                  struct pipe_sampler_view **_views)
{
   identity_set_sampler_views(_pipe, PIPE_SHADER_VERTEX, 0, num, _views);
}

static void
identity_set_vertex_buffers(struct pipe_context *_pipe,
                            unsigned num_buffers,
                            const struct pipe_vertex_buffer *_buffers)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_vertex_buffer unwrapped_buffers[PIPE_MAX_SHADER_INPUTS];
   struct pipe_vertex_buffer *buffers = NULL;
   unsigned i;

   if (num_buffers) {
      memcpy(unwrapped_buffers, _buffers, num_buffers * sizeof(*_buffers));
      for (i = 0; i < num_buffers; i++)
         unwrapped_buffers[i].buffer = identity_resource_unwrap(_buffers[i].buffer);
      buffers = unwrapped_buffers;
   }

   pipe->set_vertex_buffers(pipe,
                            num_buffers,
                            buffers);
}

static void
identity_set_index_buffer(struct pipe_context *_pipe,
                          const struct pipe_index_buffer *_ib)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_index_buffer unwrapped_ib, *ib = NULL;

   if (_ib) {
      unwrapped_ib = *_ib;
      unwrapped_ib.buffer = identity_resource_unwrap(_ib->buffer);
      ib = &unwrapped_ib;
   }

   pipe->set_index_buffer(pipe, ib);
}

static void
identity_resource_copy_region(struct pipe_context *_pipe,
                              struct pipe_resource *_dst,
                              unsigned dst_level,
                              unsigned dstx,
                              unsigned dsty,
                              unsigned dstz,
                              struct pipe_resource *_src,
                              unsigned src_level,
                              const struct pipe_box *src_box)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct identity_resource *id_resource_dst = identity_resource(_dst);
   struct identity_resource *id_resource_src = identity_resource(_src);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_resource *dst = id_resource_dst->resource;
   struct pipe_resource *src = id_resource_src->resource;

   pipe->resource_copy_region(pipe,
                              dst,
                              dst_level,
                              dstx,
                              dsty,
                              dstz,
                              src,
                              src_level,
                              src_box);
}

static void
identity_clear(struct pipe_context *_pipe,
               unsigned buffers,
               const union pipe_color_union *color,
               double depth,
               unsigned stencil)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->clear(pipe,
               buffers,
               color,
               depth,
               stencil);
}

static void
identity_clear_render_target(struct pipe_context *_pipe,
                             struct pipe_surface *_dst,
                             const union pipe_color_union *color,
                             unsigned dstx, unsigned dsty,
                             unsigned width, unsigned height)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct identity_surface *id_surface_dst = identity_surface(_dst);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_surface *dst = id_surface_dst->surface;

   pipe->clear_render_target(pipe,
                             dst,
                             color,
                             dstx,
                             dsty,
                             width,
                             height);
}
static void
identity_clear_depth_stencil(struct pipe_context *_pipe,
                             struct pipe_surface *_dst,
                             unsigned clear_flags,
                             double depth,
                             unsigned stencil,
                             unsigned dstx, unsigned dsty,
                             unsigned width, unsigned height)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct identity_surface *id_surface_dst = identity_surface(_dst);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_surface *dst = id_surface_dst->surface;

   pipe->clear_depth_stencil(pipe,
                             dst,
                             clear_flags,
                             depth,
                             stencil,
                             dstx,
                             dsty,
                             width,
                             height);

}

static void
identity_flush(struct pipe_context *_pipe,
               struct pipe_fence_handle **fence)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->flush(pipe,
               fence);
}

static struct pipe_sampler_view *
identity_context_create_sampler_view(struct pipe_context *_pipe,
                                     struct pipe_resource *_resource,
                                     const struct pipe_sampler_view *templ)
{
   struct identity_context *id_context = identity_context(_pipe);
   struct identity_resource *id_resource = identity_resource(_resource);
   struct pipe_context *pipe = id_context->pipe;
   struct pipe_resource *resource = id_resource->resource;
   struct pipe_sampler_view *result;

   result = pipe->create_sampler_view(pipe,
                                      resource,
                                      templ);

   if (result)
      return identity_sampler_view_create(id_context, id_resource, result);
   return NULL;
}

static void
identity_context_sampler_view_destroy(struct pipe_context *_pipe,
                                      struct pipe_sampler_view *_view)
{
   identity_sampler_view_destroy(identity_context(_pipe),
                                 identity_sampler_view(_view));
}

static struct pipe_surface *
identity_context_create_surface(struct pipe_context *_pipe,
                                struct pipe_resource *_resource,
                                const struct pipe_surface *templ)
{
   struct identity_context *id_context = identity_context(_pipe);
   struct identity_resource *id_resource = identity_resource(_resource);
   struct pipe_context *pipe = id_context->pipe;
   struct pipe_resource *resource = id_resource->resource;
   struct pipe_surface *result;

   result = pipe->create_surface(pipe,
                                 resource,
                                 templ);

   if (result)
      return identity_surface_create(id_context, id_resource, result);
   return NULL;
}

static void
identity_context_surface_destroy(struct pipe_context *_pipe,
                                 struct pipe_surface *_surf)
{
   identity_surface_destroy(identity_context(_pipe),
                            identity_surface(_surf));
}

static struct pipe_transfer *
identity_context_get_transfer(struct pipe_context *_context,
                              struct pipe_resource *_resource,
                              unsigned level,
                              unsigned usage,
                              const struct pipe_box *box)
{
   struct identity_context *id_context = identity_context(_context);
   struct identity_resource *id_resource = identity_resource(_resource);
   struct pipe_context *context = id_context->pipe;
   struct pipe_resource *resource = id_resource->resource;
   struct pipe_transfer *result;

   result = context->get_transfer(context,
                                  resource,
                                  level,
                                  usage,
                                  box);

   if (result)
      return identity_transfer_create(id_context, id_resource, result);
   return NULL;
}

static void
identity_context_transfer_destroy(struct pipe_context *_pipe,
                                  struct pipe_transfer *_transfer)
{
   identity_transfer_destroy(identity_context(_pipe),
                             identity_transfer(_transfer));
}

static void *
identity_context_transfer_map(struct pipe_context *_context,
                              struct pipe_transfer *_transfer)
{
   struct identity_context *id_context = identity_context(_context);
   struct identity_transfer *id_transfer = identity_transfer(_transfer);
   struct pipe_context *context = id_context->pipe;
   struct pipe_transfer *transfer = id_transfer->transfer;

   return context->transfer_map(context,
                                transfer);
}



static void
identity_context_transfer_flush_region(struct pipe_context *_context,
                                       struct pipe_transfer *_transfer,
                                       const struct pipe_box *box)
{
   struct identity_context *id_context = identity_context(_context);
   struct identity_transfer *id_transfer = identity_transfer(_transfer);
   struct pipe_context *context = id_context->pipe;
   struct pipe_transfer *transfer = id_transfer->transfer;

   context->transfer_flush_region(context,
                                  transfer,
                                  box);
}


static void
identity_context_transfer_unmap(struct pipe_context *_context,
                                struct pipe_transfer *_transfer)
{
   struct identity_context *id_context = identity_context(_context);
   struct identity_transfer *id_transfer = identity_transfer(_transfer);
   struct pipe_context *context = id_context->pipe;
   struct pipe_transfer *transfer = id_transfer->transfer;

   context->transfer_unmap(context,
                           transfer);
}


static void 
identity_context_transfer_inline_write(struct pipe_context *_context,
                                       struct pipe_resource *_resource,
                                       unsigned level,
                                       unsigned usage,
                                       const struct pipe_box *box,
                                       const void *data,
                                       unsigned stride,
                                       unsigned layer_stride)
{
   struct identity_context *id_context = identity_context(_context);
   struct identity_resource *id_resource = identity_resource(_resource);
   struct pipe_context *context = id_context->pipe;
   struct pipe_resource *resource = id_resource->resource;

   context->transfer_inline_write(context,
                                  resource,
                                  level,
                                  usage,
                                  box,
                                  data,
                                  stride,
                                  layer_stride);
}


struct pipe_context *
identity_context_create(struct pipe_screen *_screen, struct pipe_context *pipe)
{
   struct identity_context *id_pipe;
   (void)identity_screen(_screen);

   id_pipe = CALLOC_STRUCT(identity_context);
   if (!id_pipe) {
      return NULL;
   }

   id_pipe->base.screen = _screen;
   id_pipe->base.priv = pipe->priv; /* expose wrapped data */
   id_pipe->base.draw = NULL;

   id_pipe->base.destroy = identity_destroy;
   id_pipe->base.draw_vbo = identity_draw_vbo;
   id_pipe->base.create_query = identity_create_query;
   id_pipe->base.destroy_query = identity_destroy_query;
   id_pipe->base.begin_query = identity_begin_query;
   id_pipe->base.end_query = identity_end_query;
   id_pipe->base.get_query_result = identity_get_query_result;
   id_pipe->base.create_blend_state = identity_create_blend_state;
   id_pipe->base.bind_blend_state = identity_bind_blend_state;
   id_pipe->base.delete_blend_state = identity_delete_blend_state;
   id_pipe->base.create_sampler_state = identity_create_sampler_state;
   id_pipe->base.bind_fragment_sampler_states = identity_bind_fragment_sampler_states;
   id_pipe->base.bind_vertex_sampler_states = identity_bind_vertex_sampler_states;
   id_pipe->base.delete_sampler_state = identity_delete_sampler_state;
   id_pipe->base.create_rasterizer_state = identity_create_rasterizer_state;
   id_pipe->base.bind_rasterizer_state = identity_bind_rasterizer_state;
   id_pipe->base.delete_rasterizer_state = identity_delete_rasterizer_state;
   id_pipe->base.create_depth_stencil_alpha_state = identity_create_depth_stencil_alpha_state;
   id_pipe->base.bind_depth_stencil_alpha_state = identity_bind_depth_stencil_alpha_state;
   id_pipe->base.delete_depth_stencil_alpha_state = identity_delete_depth_stencil_alpha_state;
   id_pipe->base.create_fs_state = identity_create_fs_state;
   id_pipe->base.bind_fs_state = identity_bind_fs_state;
   id_pipe->base.delete_fs_state = identity_delete_fs_state;
   id_pipe->base.create_vs_state = identity_create_vs_state;
   id_pipe->base.bind_vs_state = identity_bind_vs_state;
   id_pipe->base.delete_vs_state = identity_delete_vs_state;
   id_pipe->base.create_vertex_elements_state = identity_create_vertex_elements_state;
   id_pipe->base.bind_vertex_elements_state = identity_bind_vertex_elements_state;
   id_pipe->base.delete_vertex_elements_state = identity_delete_vertex_elements_state;
   id_pipe->base.set_blend_color = identity_set_blend_color;
   id_pipe->base.set_stencil_ref = identity_set_stencil_ref;
   id_pipe->base.set_clip_state = identity_set_clip_state;
   id_pipe->base.set_sample_mask = identity_set_sample_mask;
   id_pipe->base.set_constant_buffer = identity_set_constant_buffer;
   id_pipe->base.set_framebuffer_state = identity_set_framebuffer_state;
   id_pipe->base.set_polygon_stipple = identity_set_polygon_stipple;
   id_pipe->base.set_scissor_state = identity_set_scissor_state;
   id_pipe->base.set_viewport_state = identity_set_viewport_state;
   id_pipe->base.set_fragment_sampler_views = identity_set_fragment_sampler_views;
   id_pipe->base.set_vertex_sampler_views = identity_set_vertex_sampler_views;
   id_pipe->base.set_vertex_buffers = identity_set_vertex_buffers;
   id_pipe->base.set_index_buffer = identity_set_index_buffer;
   id_pipe->base.resource_copy_region = identity_resource_copy_region;
   id_pipe->base.clear = identity_clear;
   id_pipe->base.clear_render_target = identity_clear_render_target;
   id_pipe->base.clear_depth_stencil = identity_clear_depth_stencil;
   id_pipe->base.flush = identity_flush;
   id_pipe->base.create_surface = identity_context_create_surface;
   id_pipe->base.surface_destroy = identity_context_surface_destroy;
   id_pipe->base.create_sampler_view = identity_context_create_sampler_view;
   id_pipe->base.sampler_view_destroy = identity_context_sampler_view_destroy;
   id_pipe->base.get_transfer = identity_context_get_transfer;
   id_pipe->base.transfer_destroy = identity_context_transfer_destroy;
   id_pipe->base.transfer_map = identity_context_transfer_map;
   id_pipe->base.transfer_unmap = identity_context_transfer_unmap;
   id_pipe->base.transfer_flush_region = identity_context_transfer_flush_region;
   id_pipe->base.transfer_inline_write = identity_context_transfer_inline_write;

   id_pipe->pipe = pipe;

   return &id_pipe->base;
}
