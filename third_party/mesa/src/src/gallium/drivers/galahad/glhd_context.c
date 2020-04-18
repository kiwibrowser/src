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

#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"

#include "glhd_context.h"
#include "glhd_objects.h"


static void
galahad_context_destroy(struct pipe_context *_pipe)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->destroy(pipe);

   FREE(glhd_pipe);
}

static void
galahad_context_draw_vbo(struct pipe_context *_pipe,
                 const struct pipe_draw_info *info)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   /* XXX we should check that all bound resources are unmapped
    * before drawing.
    */

   pipe->draw_vbo(pipe, info);
}

static struct pipe_query *
galahad_context_create_query(struct pipe_context *_pipe,
                      unsigned query_type)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   if (query_type == PIPE_QUERY_OCCLUSION_COUNTER &&
      !pipe->screen->get_param(pipe->screen, PIPE_CAP_OCCLUSION_QUERY)) {
      glhd_error("Occlusion query requested but not supported");
   }

   if (query_type == PIPE_QUERY_TIME_ELAPSED &&
      !pipe->screen->get_param(pipe->screen, PIPE_CAP_TIMER_QUERY)) {
      glhd_error("Timer query requested but not supported");
   }

   return pipe->create_query(pipe,
                             query_type);
}

static void
galahad_context_destroy_query(struct pipe_context *_pipe,
                       struct pipe_query *query)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->destroy_query(pipe,
                       query);
}

static void
galahad_context_begin_query(struct pipe_context *_pipe,
                     struct pipe_query *query)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->begin_query(pipe,
                     query);
}

static void
galahad_context_end_query(struct pipe_context *_pipe,
                   struct pipe_query *query)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->end_query(pipe,
                   query);
}

static boolean
galahad_context_get_query_result(struct pipe_context *_pipe,
                          struct pipe_query *query,
                          boolean wait,
                          union pipe_query_result *result)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   return pipe->get_query_result(pipe,
                                 query,
                                 wait,
                                 result);
}

static void *
galahad_context_create_blend_state(struct pipe_context *_pipe,
                            const struct pipe_blend_state *blend)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   if (blend->logicop_enable) {
      if (blend->rt[0].blend_enable) {
         glhd_warn("Blending enabled for render target 0, but logicops "
            "are enabled");
      }
   }

   return pipe->create_blend_state(pipe,
                                   blend);
}

static void
galahad_context_bind_blend_state(struct pipe_context *_pipe,
                          void *blend)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->bind_blend_state(pipe,
                              blend);
}

static void
galahad_context_delete_blend_state(struct pipe_context *_pipe,
                            void *blend)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->delete_blend_state(pipe,
                            blend);
}

static void *
galahad_context_create_sampler_state(struct pipe_context *_pipe,
                              const struct pipe_sampler_state *sampler)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   return pipe->create_sampler_state(pipe,
                                     sampler);
}

static void
galahad_context_bind_sampler_states(struct pipe_context *_pipe,
                                    unsigned shader,
                                    unsigned start,
                                    unsigned num_samplers,
                                    void **samplers)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   if (num_samplers > PIPE_MAX_SAMPLERS) {
      glhd_error("%u samplers requested, "
         "but only %u are permitted by API",
         num_samplers, PIPE_MAX_SAMPLERS);
   }

   switch (shader) {
   case PIPE_SHADER_VERTEX:
      pipe->bind_vertex_sampler_states(pipe, num_samplers, samplers);
      break;
   case PIPE_SHADER_FRAGMENT:
      pipe->bind_fragment_sampler_states(pipe, num_samplers, samplers);
      break;
   default:
      assert(0);
   }
}

static void
galahad_context_bind_vertex_sampler_states(struct pipe_context *_pipe,
                                           unsigned num_samplers,
                                           void **samplers)
{
   galahad_context_bind_sampler_states(_pipe, PIPE_SHADER_VERTEX,
                                       0, num_samplers, samplers);
}

static void
galahad_context_bind_fragment_sampler_states(struct pipe_context *_pipe,
                                             unsigned num_samplers,
                                             void **samplers)
{
   galahad_context_bind_sampler_states(_pipe, PIPE_SHADER_FRAGMENT,
                                       0, num_samplers, samplers);
}


static void
galahad_context_delete_sampler_state(struct pipe_context *_pipe,
                              void *sampler)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->delete_sampler_state(pipe,
                              sampler);
}

static void *
galahad_context_create_rasterizer_state(struct pipe_context *_pipe,
                                 const struct pipe_rasterizer_state *rasterizer)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   if (rasterizer->point_quad_rasterization) {
       if (rasterizer->point_smooth) {
           glhd_warn("Point smoothing requested but ignored");
       }
   } else {
       if (rasterizer->sprite_coord_enable) {
           glhd_warn("Point sprites requested but ignored");
       }
   }

   return pipe->create_rasterizer_state(pipe,
                                        rasterizer);
}

static void
galahad_context_bind_rasterizer_state(struct pipe_context *_pipe,
                               void *rasterizer)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->bind_rasterizer_state(pipe,
                               rasterizer);
}

static void
galahad_context_delete_rasterizer_state(struct pipe_context *_pipe,
                                 void *rasterizer)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->delete_rasterizer_state(pipe,
                                 rasterizer);
}

static void *
galahad_context_create_depth_stencil_alpha_state(struct pipe_context *_pipe,
                                          const struct pipe_depth_stencil_alpha_state *depth_stencil_alpha)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   return pipe->create_depth_stencil_alpha_state(pipe,
                                                 depth_stencil_alpha);
}

static void
galahad_context_bind_depth_stencil_alpha_state(struct pipe_context *_pipe,
                                        void *depth_stencil_alpha)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->bind_depth_stencil_alpha_state(pipe,
                                        depth_stencil_alpha);
}

static void
galahad_context_delete_depth_stencil_alpha_state(struct pipe_context *_pipe,
                                          void *depth_stencil_alpha)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->delete_depth_stencil_alpha_state(pipe,
                                          depth_stencil_alpha);
}

static void *
galahad_context_create_fs_state(struct pipe_context *_pipe,
                         const struct pipe_shader_state *fs)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   return pipe->create_fs_state(pipe,
                                fs);
}

static void
galahad_context_bind_fs_state(struct pipe_context *_pipe,
                       void *fs)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->bind_fs_state(pipe,
                       fs);
}

static void
galahad_context_delete_fs_state(struct pipe_context *_pipe,
                         void *fs)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->delete_fs_state(pipe,
                         fs);
}

static void *
galahad_context_create_vs_state(struct pipe_context *_pipe,
                         const struct pipe_shader_state *vs)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   return pipe->create_vs_state(pipe,
                                vs);
}

static void
galahad_context_bind_vs_state(struct pipe_context *_pipe,
                       void *vs)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->bind_vs_state(pipe,
                       vs);
}

static void
galahad_context_delete_vs_state(struct pipe_context *_pipe,
                         void *vs)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->delete_vs_state(pipe,
                         vs);
}


static void *
galahad_context_create_vertex_elements_state(struct pipe_context *_pipe,
                                      unsigned num_elements,
                                      const struct pipe_vertex_element *vertex_elements)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   /* XXX check if stride lines up with element size, at least for floats */

   return pipe->create_vertex_elements_state(pipe,
                                             num_elements,
                                             vertex_elements);
}

static void
galahad_context_bind_vertex_elements_state(struct pipe_context *_pipe,
                                    void *velems)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->bind_vertex_elements_state(pipe,
                                    velems);
}

static void
galahad_context_delete_vertex_elements_state(struct pipe_context *_pipe,
                                      void *velems)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->delete_vertex_elements_state(pipe,
                                      velems);
}

static void
galahad_context_set_blend_color(struct pipe_context *_pipe,
                         const struct pipe_blend_color *blend_color)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->set_blend_color(pipe,
                         blend_color);
}

static void
galahad_context_set_stencil_ref(struct pipe_context *_pipe,
                         const struct pipe_stencil_ref *stencil_ref)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->set_stencil_ref(pipe,
                         stencil_ref);
}

static void
galahad_context_set_clip_state(struct pipe_context *_pipe,
                        const struct pipe_clip_state *clip)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->set_clip_state(pipe,
                        clip);
}

static void
galahad_context_set_sample_mask(struct pipe_context *_pipe,
                         unsigned sample_mask)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->set_sample_mask(pipe,
                         sample_mask);
}

static void
galahad_context_set_constant_buffer(struct pipe_context *_pipe,
                             uint shader,
                             uint index,
                             struct pipe_constant_buffer *_cb)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;
   struct pipe_constant_buffer cb;

   if (shader >= PIPE_SHADER_TYPES) {
      glhd_error("Unknown shader type %u", shader);
   }

   if (index &&
      index >=
         pipe->screen->get_shader_param(pipe->screen, shader, PIPE_SHADER_CAP_MAX_CONST_BUFFERS)) {
      glhd_error("Access to constant buffer %u requested, "
         "but only %d are supported",
         index,
         pipe->screen->get_shader_param(pipe->screen, shader, PIPE_SHADER_CAP_MAX_CONST_BUFFERS));
   }

   /* XXX hmm? unwrap the input state */
   if (_cb) {
      cb = *_cb;
      cb.buffer = galahad_resource_unwrap(_cb->buffer);
   }

   pipe->set_constant_buffer(pipe,
                             shader,
                             index,
                             _cb ? &cb : NULL);
}

static void
galahad_context_set_framebuffer_state(struct pipe_context *_pipe,
                               const struct pipe_framebuffer_state *_state)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;
   struct pipe_framebuffer_state unwrapped_state;
   struct pipe_framebuffer_state *state = NULL;
   unsigned i;

   if (_state->nr_cbufs > PIPE_MAX_COLOR_BUFS) {
      glhd_error("%d render targets bound, but only %d are permitted by API",
         _state->nr_cbufs, PIPE_MAX_COLOR_BUFS);
   } else if (_state->nr_cbufs >
      pipe->screen->get_param(pipe->screen, PIPE_CAP_MAX_RENDER_TARGETS)) {
      glhd_warn("%d render targets bound, but only %d are supported",
         _state->nr_cbufs,
         pipe->screen->get_param(pipe->screen, PIPE_CAP_MAX_RENDER_TARGETS));
   }

   /* unwrap the input state */
   if (_state) {
      memcpy(&unwrapped_state, _state, sizeof(unwrapped_state));
      for(i = 0; i < _state->nr_cbufs; i++)
         unwrapped_state.cbufs[i] = galahad_surface_unwrap(_state->cbufs[i]);
      for (; i < PIPE_MAX_COLOR_BUFS; i++)
         unwrapped_state.cbufs[i] = NULL;
      unwrapped_state.zsbuf = galahad_surface_unwrap(_state->zsbuf);
      state = &unwrapped_state;
   }

   pipe->set_framebuffer_state(pipe,
                               state);
}

static void
galahad_context_set_polygon_stipple(struct pipe_context *_pipe,
                             const struct pipe_poly_stipple *poly_stipple)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->set_polygon_stipple(pipe,
                             poly_stipple);
}

static void
galahad_context_set_scissor_state(struct pipe_context *_pipe,
                           const struct pipe_scissor_state *scissor)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->set_scissor_state(pipe,
                           scissor);
}

static void
galahad_context_set_viewport_state(struct pipe_context *_pipe,
                            const struct pipe_viewport_state *viewport)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->set_viewport_state(pipe,
                            viewport);
}

static void
galahad_context_set_sampler_views(struct pipe_context *_pipe,
                                  unsigned shader,
                                  unsigned start,
                                  unsigned num,
                                  struct pipe_sampler_view **_views)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;
   struct pipe_sampler_view *unwrapped_views[PIPE_MAX_SAMPLERS];
   struct pipe_sampler_view **views = NULL;
   unsigned i;

   if (_views) {
      for (i = 0; i < num; i++)
         unwrapped_views[i] = galahad_sampler_view_unwrap(_views[i]);
      for (; i < PIPE_MAX_SAMPLERS; i++)
         unwrapped_views[i] = NULL;

      views = unwrapped_views;
   }

   switch (shader) {
   case PIPE_SHADER_VERTEX:
      pipe->set_vertex_sampler_views(pipe, num, views);
      break;
   case PIPE_SHADER_FRAGMENT:
      pipe->set_fragment_sampler_views(pipe, num, views);
      break;
   default:
      assert(0);
   }
}

static void
galahad_context_set_vertex_sampler_views(struct pipe_context *_pipe,
                                         unsigned num,
                                         struct pipe_sampler_view **_views)
{
   galahad_context_set_sampler_views(_pipe, PIPE_SHADER_VERTEX,
                                     0, num, _views);
}

static void
galahad_context_set_fragment_sampler_views(struct pipe_context *_pipe,
                                         unsigned num,
                                         struct pipe_sampler_view **_views)
{
   galahad_context_set_sampler_views(_pipe, PIPE_SHADER_FRAGMENT,
                                     0, num, _views);
}


static void
galahad_context_set_vertex_buffers(struct pipe_context *_pipe,
                            unsigned num_buffers,
                            const struct pipe_vertex_buffer *_buffers)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;
   struct pipe_vertex_buffer unwrapped_buffers[PIPE_MAX_SHADER_INPUTS];
   struct pipe_vertex_buffer *buffers = NULL;
   unsigned i;

   if (num_buffers) {
      memcpy(unwrapped_buffers, _buffers, num_buffers * sizeof(*_buffers));
      for (i = 0; i < num_buffers; i++)
         unwrapped_buffers[i].buffer = galahad_resource_unwrap(_buffers[i].buffer);
      buffers = unwrapped_buffers;
   }

   pipe->set_vertex_buffers(pipe,
                            num_buffers,
                            buffers);
}

static void
galahad_context_set_index_buffer(struct pipe_context *_pipe,
                         const struct pipe_index_buffer *_ib)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;
   struct pipe_index_buffer unwrapped_ib, *ib = NULL;

   if (_ib) {
      if (_ib->buffer || _ib->user_buffer) {
         switch (_ib->index_size) {
         case 1:
         case 2:
         case 4:
            break;
         default:
            glhd_warn("unrecognized index size %d", _ib->index_size);
            break;
         }
      }
      else if (_ib->offset || _ib->index_size) {
         glhd_warn("non-indexed state with index offset %d and index size %d",
               _ib->offset, _ib->index_size);
      }

      unwrapped_ib = *_ib;
      unwrapped_ib.buffer = galahad_resource_unwrap(_ib->buffer);
      ib = &unwrapped_ib;
   }

   pipe->set_index_buffer(pipe, ib);
}

static void
galahad_context_resource_copy_region(struct pipe_context *_pipe,
                              struct pipe_resource *_dst,
                              unsigned dst_level,
                              unsigned dstx,
                              unsigned dsty,
                              unsigned dstz,
                              struct pipe_resource *_src,
                              unsigned src_level,
                              const struct pipe_box *src_box)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct galahad_resource *glhd_resource_dst = galahad_resource(_dst);
   struct galahad_resource *glhd_resource_src = galahad_resource(_src);
   struct pipe_context *pipe = glhd_pipe->pipe;
   struct pipe_resource *dst = glhd_resource_dst->resource;
   struct pipe_resource *src = glhd_resource_src->resource;

   if (_dst->format != _src->format) {
      const struct util_format_description *src_desc =
         util_format_description(_src->format);
      const struct util_format_description *dst_desc =
         util_format_description(_dst->format);
      if (!util_is_format_compatible(src_desc, dst_desc))
         glhd_warn("Format mismatch: Source is %s, destination is %s",
            src_desc->short_name,
            dst_desc->short_name);
   }

   if ((_src->target == PIPE_BUFFER && _dst->target != PIPE_BUFFER) ||
       (_src->target != PIPE_BUFFER && _dst->target == PIPE_BUFFER)) {
      glhd_warn("Resource target mismatch: Source is %i, destination is %i",
                _src->target, _dst->target);
   }

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
galahad_context_clear(struct pipe_context *_pipe,
               unsigned buffers,
               const union pipe_color_union *color,
               double depth,
               unsigned stencil)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->clear(pipe,
               buffers,
               color,
               depth,
               stencil);
}

static void
galahad_context_clear_render_target(struct pipe_context *_pipe,
                             struct pipe_surface *_dst,
                             const union pipe_color_union *color,
                             unsigned dstx, unsigned dsty,
                             unsigned width, unsigned height)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct galahad_surface *glhd_surface_dst = galahad_surface(_dst);
   struct pipe_context *pipe = glhd_pipe->pipe;
   struct pipe_surface *dst = glhd_surface_dst->surface;

   pipe->clear_render_target(pipe,
                             dst,
                             color,
                             dstx,
                             dsty,
                             width,
                             height);
}
static void
galahad_context_clear_depth_stencil(struct pipe_context *_pipe,
                             struct pipe_surface *_dst,
                             unsigned clear_flags,
                             double depth,
                             unsigned stencil,
                             unsigned dstx, unsigned dsty,
                             unsigned width, unsigned height)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct galahad_surface *glhd_surface_dst = galahad_surface(_dst);
   struct pipe_context *pipe = glhd_pipe->pipe;
   struct pipe_surface *dst = glhd_surface_dst->surface;

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
galahad_context_flush(struct pipe_context *_pipe,
               struct pipe_fence_handle **fence)
{
   struct galahad_context *glhd_pipe = galahad_context(_pipe);
   struct pipe_context *pipe = glhd_pipe->pipe;

   pipe->flush(pipe,
               fence);
}

static struct pipe_sampler_view *
galahad_context_create_sampler_view(struct pipe_context *_pipe,
                                     struct pipe_resource *_resource,
                                     const struct pipe_sampler_view *templ)
{
   struct galahad_context *glhd_context = galahad_context(_pipe);
   struct galahad_resource *glhd_resource = galahad_resource(_resource);
   struct pipe_context *pipe = glhd_context->pipe;
   struct pipe_resource *resource = glhd_resource->resource;
   struct pipe_sampler_view *result;

   result = pipe->create_sampler_view(pipe,
                                      resource,
                                      templ);

   if (result)
      return galahad_sampler_view_create(glhd_context, glhd_resource, result);
   return NULL;
}

static void
galahad_context_sampler_view_destroy(struct pipe_context *_pipe,
                                      struct pipe_sampler_view *_view)
{
   galahad_sampler_view_destroy(galahad_context(_pipe),
                                 galahad_sampler_view(_view));
}

static struct pipe_surface *
galahad_context_create_surface(struct pipe_context *_pipe,
                                struct pipe_resource *_resource,
                                const struct pipe_surface *templ)
{
   struct galahad_context *glhd_context = galahad_context(_pipe);
   struct galahad_resource *glhd_resource = galahad_resource(_resource);
   struct pipe_context *pipe = glhd_context->pipe;
   struct pipe_resource *resource = glhd_resource->resource;
   struct pipe_surface *result;

   result = pipe->create_surface(pipe,
                                 resource,
                                 templ);

   if (result)
      return galahad_surface_create(glhd_context, glhd_resource, result);
   return NULL;
}

static void
galahad_context_surface_destroy(struct pipe_context *_pipe,
                                struct pipe_surface *_surface)
{
   galahad_surface_destroy(galahad_context(_pipe),
                           galahad_surface(_surface));
}



static struct pipe_transfer *
galahad_context_get_transfer(struct pipe_context *_context,
                              struct pipe_resource *_resource,
                              unsigned level,
                              unsigned usage,
                              const struct pipe_box *box)
{
   struct galahad_context *glhd_context = galahad_context(_context);
   struct galahad_resource *glhd_resource = galahad_resource(_resource);
   struct pipe_context *context = glhd_context->pipe;
   struct pipe_resource *resource = glhd_resource->resource;
   struct pipe_transfer *result;

   result = context->get_transfer(context,
                                  resource,
                                  level,
                                  usage,
                                  box);

   if (result)
      return galahad_transfer_create(glhd_context, glhd_resource, result);
   return NULL;
}

static void
galahad_context_transfer_destroy(struct pipe_context *_pipe,
                                  struct pipe_transfer *_transfer)
{
   galahad_transfer_destroy(galahad_context(_pipe),
                             galahad_transfer(_transfer));
}

static void *
galahad_context_transfer_map(struct pipe_context *_context,
                              struct pipe_transfer *_transfer)
{
   struct galahad_context *glhd_context = galahad_context(_context);
   struct galahad_transfer *glhd_transfer = galahad_transfer(_transfer);
   struct pipe_context *context = glhd_context->pipe;
   struct pipe_transfer *transfer = glhd_transfer->transfer;

   struct galahad_resource *glhd_resource = galahad_resource(_transfer->resource);

   glhd_resource->map_count++;

   return context->transfer_map(context,
                                transfer);
}



static void
galahad_context_transfer_flush_region(struct pipe_context *_context,
                                       struct pipe_transfer *_transfer,
                                       const struct pipe_box *box)
{
   struct galahad_context *glhd_context = galahad_context(_context);
   struct galahad_transfer *glhd_transfer = galahad_transfer(_transfer);
   struct pipe_context *context = glhd_context->pipe;
   struct pipe_transfer *transfer = glhd_transfer->transfer;

   context->transfer_flush_region(context,
                                  transfer,
                                  box);
}


static void
galahad_context_transfer_unmap(struct pipe_context *_context,
                                struct pipe_transfer *_transfer)
{
   struct galahad_context *glhd_context = galahad_context(_context);
   struct galahad_transfer *glhd_transfer = galahad_transfer(_transfer);
   struct pipe_context *context = glhd_context->pipe;
   struct pipe_transfer *transfer = glhd_transfer->transfer;
   struct galahad_resource *glhd_resource = galahad_resource(_transfer->resource);

   if (glhd_resource->map_count < 1) {
      glhd_warn("context::transfer_unmap() called too many times"
                " (count = %d)\n", glhd_resource->map_count);      
   }

   glhd_resource->map_count--;

   context->transfer_unmap(context,
                           transfer);
}


static void
galahad_context_transfer_inline_write(struct pipe_context *_context,
                                       struct pipe_resource *_resource,
                                       unsigned level,
                                       unsigned usage,
                                       const struct pipe_box *box,
                                       const void *data,
                                       unsigned stride,
                                       unsigned slice_stride)
{
   struct galahad_context *glhd_context = galahad_context(_context);
   struct galahad_resource *glhd_resource = galahad_resource(_resource);
   struct pipe_context *context = glhd_context->pipe;
   struct pipe_resource *resource = glhd_resource->resource;

   context->transfer_inline_write(context,
                                  resource,
                                  level,
                                  usage,
                                  box,
                                  data,
                                  stride,
                                  slice_stride);
}


static void
galahad_context_render_condition(struct pipe_context *_context,
                                 struct pipe_query *query,
                                 uint mode)
{
   struct galahad_context *glhd_context = galahad_context(_context);
   struct pipe_context *context = glhd_context->pipe;

   context->render_condition(context, query, mode);
}


struct pipe_context *
galahad_context_create(struct pipe_screen *_screen, struct pipe_context *pipe)
{
   struct galahad_context *glhd_pipe;
   (void)galahad_screen(_screen);

   glhd_pipe = CALLOC_STRUCT(galahad_context);
   if (!glhd_pipe) {
      return NULL;
   }

   glhd_pipe->base.screen = _screen;
   glhd_pipe->base.priv = pipe->priv; /* expose wrapped data */
   glhd_pipe->base.draw = NULL;

   glhd_pipe->base.destroy = galahad_context_destroy;

#define GLHD_PIPE_INIT(_member) \
   glhd_pipe->base . _member = pipe -> _member ? galahad_context_ ## _member : NULL

   GLHD_PIPE_INIT(draw_vbo);
   GLHD_PIPE_INIT(render_condition);
   GLHD_PIPE_INIT(create_query);
   GLHD_PIPE_INIT(destroy_query);
   GLHD_PIPE_INIT(begin_query);
   GLHD_PIPE_INIT(end_query);
   GLHD_PIPE_INIT(get_query_result);
   GLHD_PIPE_INIT(create_blend_state);
   GLHD_PIPE_INIT(bind_blend_state);
   GLHD_PIPE_INIT(delete_blend_state);
   GLHD_PIPE_INIT(create_sampler_state);
   GLHD_PIPE_INIT(bind_fragment_sampler_states);
   GLHD_PIPE_INIT(bind_vertex_sampler_states);
   //GLHD_PIPE_INIT(bind_geometry_sampler_states);
   //GLHD_PIPE_INIT(bind_compute_sampler_states);
   GLHD_PIPE_INIT(delete_sampler_state);
   GLHD_PIPE_INIT(create_rasterizer_state);
   GLHD_PIPE_INIT(bind_rasterizer_state);
   GLHD_PIPE_INIT(delete_rasterizer_state);
   GLHD_PIPE_INIT(create_depth_stencil_alpha_state);
   GLHD_PIPE_INIT(bind_depth_stencil_alpha_state);
   GLHD_PIPE_INIT(delete_depth_stencil_alpha_state);
   GLHD_PIPE_INIT(create_fs_state);
   GLHD_PIPE_INIT(bind_fs_state);
   GLHD_PIPE_INIT(delete_fs_state);
   GLHD_PIPE_INIT(create_vs_state);
   GLHD_PIPE_INIT(bind_vs_state);
   GLHD_PIPE_INIT(delete_vs_state);
   //GLHD_PIPE_INIT(create_gs_state);
   //GLHD_PIPE_INIT(bind_gs_state);
   //GLHD_PIPE_INIT(delete_gs_state);
   GLHD_PIPE_INIT(create_vertex_elements_state);
   GLHD_PIPE_INIT(bind_vertex_elements_state);
   GLHD_PIPE_INIT(delete_vertex_elements_state);
   GLHD_PIPE_INIT(set_blend_color);
   GLHD_PIPE_INIT(set_stencil_ref);
   GLHD_PIPE_INIT(set_sample_mask);
   GLHD_PIPE_INIT(set_clip_state);
   GLHD_PIPE_INIT(set_constant_buffer);
   GLHD_PIPE_INIT(set_framebuffer_state);
   GLHD_PIPE_INIT(set_polygon_stipple);
   GLHD_PIPE_INIT(set_scissor_state);
   GLHD_PIPE_INIT(set_viewport_state);
   GLHD_PIPE_INIT(set_fragment_sampler_views);
   GLHD_PIPE_INIT(set_vertex_sampler_views);
   //GLHD_PIPE_INIT(set_geometry_sampler_views);
   //GLHD_PIPE_INIT(set_compute_sampler_views);
   //GLHD_PIPE_INIT(set_shader_resources);
   GLHD_PIPE_INIT(set_vertex_buffers);
   GLHD_PIPE_INIT(set_index_buffer);
   //GLHD_PIPE_INIT(create_stream_output_target);
   //GLHD_PIPE_INIT(stream_output_target_destroy);
   //GLHD_PIPE_INIT(set_stream_output_targets);
   GLHD_PIPE_INIT(resource_copy_region);
   //GLHD_PIPE_INIT(resource_resolve);
   GLHD_PIPE_INIT(clear);
   GLHD_PIPE_INIT(clear_render_target);
   GLHD_PIPE_INIT(clear_depth_stencil);
   GLHD_PIPE_INIT(flush);
   GLHD_PIPE_INIT(create_sampler_view);
   GLHD_PIPE_INIT(sampler_view_destroy);
   GLHD_PIPE_INIT(create_surface);
   GLHD_PIPE_INIT(surface_destroy);
   GLHD_PIPE_INIT(get_transfer);
   GLHD_PIPE_INIT(transfer_destroy);
   GLHD_PIPE_INIT(transfer_map);
   GLHD_PIPE_INIT(transfer_flush_region);
   GLHD_PIPE_INIT(transfer_unmap);
   GLHD_PIPE_INIT(transfer_inline_write);
   //GLHD_PIPE_INIT(texture_barrier);
   //GLHD_PIPE_INIT(create_video_decoder);
   //GLHD_PIPE_INIT(create_video_buffer);
   //GLHD_PIPE_INIT(create_compute_state);
   //GLHD_PIPE_INIT(bind_compute_state);
   //GLHD_PIPE_INIT(delete_compute_state);
   //GLHD_PIPE_INIT(set_compute_resources);
   //GLHD_PIPE_INIT(set_global_binding);
   //GLHD_PIPE_INIT(launch_grid);

#undef GLHD_PIPE_INIT

   glhd_pipe->pipe = pipe;

   return &glhd_pipe->base;
}
