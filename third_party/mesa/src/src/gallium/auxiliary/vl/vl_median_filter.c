/**************************************************************************
 *
 * Copyright 2012 Christian König.
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

#include "pipe/p_context.h"

#include "tgsi/tgsi_ureg.h"

#include "util/u_draw.h"
#include "util/u_memory.h"
#include "util/u_math.h"

#include "vl_types.h"
#include "vl_vertex_buffers.h"
#include "vl_median_filter.h"

enum VS_OUTPUT
{
   VS_O_VPOS = 0,
   VS_O_VTEX = 0
};

static void *
create_vert_shader(struct vl_median_filter *filter)
{
   struct ureg_program *shader;
   struct ureg_src i_vpos;
   struct ureg_dst o_vpos, o_vtex;

   shader = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!shader)
      return NULL;

   i_vpos = ureg_DECL_vs_input(shader, 0);
   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS);
   o_vtex = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTEX);

   ureg_MOV(shader, o_vpos, i_vpos);
   ureg_MOV(shader, o_vtex, i_vpos);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, filter->pipe);
}

static inline bool
is_vec_zero(struct vertex2f v)
{
   return v.x == 0.0f && v.y == 0.0f;
}

static void *
create_frag_shader(struct vl_median_filter *filter,
                   struct vertex2f *offsets,
                   unsigned num_offsets)
{
   struct pipe_screen *screen = filter->pipe->screen;
   struct ureg_program *shader;
   struct ureg_src i_vtex;
   struct ureg_src sampler;
   struct ureg_dst *t_array = MALLOC(sizeof(struct ureg_dst) * num_offsets);
   struct ureg_dst o_fragment;
   const unsigned median = num_offsets >> 1;
   int i, j;

   assert(num_offsets & 1); /* we need an odd number of offsets */
   if (!(num_offsets & 1)) { /* yeah, we REALLY need an odd number of offsets!!! */
      FREE(t_array);
      return NULL;
   }

   if (num_offsets > screen->get_shader_param(
      screen, TGSI_PROCESSOR_FRAGMENT, PIPE_SHADER_CAP_MAX_TEMPS)) {

      FREE(t_array);
      return NULL;
   }

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader) {
      FREE(t_array);
      return NULL;
   }

   i_vtex = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTEX, TGSI_INTERPOLATE_LINEAR);
   sampler = ureg_DECL_sampler(shader, 0);

   for (i = 0; i < num_offsets; ++i)
      t_array[i] = ureg_DECL_temporary(shader);
   o_fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   /*
    * t_array[0..*] = vtex + offset[0..*]
    * t_array[0..*] = tex(t_array[0..*], sampler)
    * result = partial_bubblesort(t_array)[mid]
    */

   for (i = 0; i < num_offsets; ++i) {
      if (!is_vec_zero(offsets[i])) {
         ureg_ADD(shader, ureg_writemask(t_array[i], TGSI_WRITEMASK_XY),
                  i_vtex, ureg_imm2f(shader, offsets[i].x, offsets[i].y));
         ureg_MOV(shader, ureg_writemask(t_array[i], TGSI_WRITEMASK_ZW),
                  ureg_imm1f(shader, 0.0f));
      }
   }

   for (i = 0; i < num_offsets; ++i) {
      struct ureg_src src = is_vec_zero(offsets[i]) ? i_vtex : ureg_src(t_array[i]);
      ureg_TEX(shader, t_array[i], TGSI_TEXTURE_2D, src, sampler);
   }

   // TODO: Couldn't this be improved even more?
   for (i = 0; i <= median; ++i) {
      for (j = 1; j < (num_offsets - i - 1); ++j) {
         struct ureg_dst tmp = ureg_DECL_temporary(shader);
         ureg_MOV(shader, tmp, ureg_src(t_array[j]));
         ureg_MAX(shader, t_array[j], ureg_src(t_array[j]), ureg_src(t_array[j - 1]));
         ureg_MIN(shader, t_array[j - 1], ureg_src(tmp), ureg_src(t_array[j - 1]));
         ureg_release_temporary(shader, tmp);
      }
      if (i == median)
         ureg_MAX(shader, t_array[j], ureg_src(t_array[j]), ureg_src(t_array[j - 1]));
      else
         ureg_MIN(shader, t_array[j - 1], ureg_src(t_array[j]), ureg_src(t_array[j - 1]));
   }
   ureg_MOV(shader, o_fragment, ureg_src(t_array[median]));

   ureg_END(shader);

   FREE(t_array);
   return ureg_create_shader_and_destroy(shader, filter->pipe);
}

static void
generate_offsets(enum vl_median_filter_shape shape, unsigned size,
                 struct vertex2f **offsets, unsigned *num_offsets)
{
   int i = 0, half_size;
   struct vertex2f v;

   assert(offsets && num_offsets);

   /* size needs to be odd */
   size = align(size + 1, 2) - 1;
   half_size = size >> 1;

   switch(shape) {
   case VL_MEDIAN_FILTER_BOX:
      *num_offsets = size*size;
      break;

   case VL_MEDIAN_FILTER_CROSS:
   case VL_MEDIAN_FILTER_X:
      *num_offsets = size + size - 1;
      break;

   case VL_MEDIAN_FILTER_HORIZONTAL:
   case VL_MEDIAN_FILTER_VERTICAL:
      *num_offsets = size;
      break;

   default:
      *num_offsets = 0;
      return;
   }

   *offsets = MALLOC(sizeof(struct vertex2f) * *num_offsets);
   if (!*offsets)
      return;

   switch(shape) {
   case VL_MEDIAN_FILTER_BOX:
      for (v.x = -half_size; v.x <= half_size; ++v.x)
         for (v.y = -half_size; v.y <= half_size; ++v.y)
            (*offsets)[i++] = v;
      break;

   case VL_MEDIAN_FILTER_CROSS:
      v.y = 0.0f;
      for (v.x = -half_size; v.x <= half_size; ++v.x)
         (*offsets)[i++] = v;

      v.x = 0.0f;
      for (v.y = -half_size; v.y <= half_size; ++v.y)
         if (v.y != 0.0f)
            (*offsets)[i++] = v;
      break;

   case VL_MEDIAN_FILTER_X:
      for (v.x = v.y = -half_size; v.x <= half_size; ++v.x, ++v.y)
         (*offsets)[i++] = v;

      for (v.x = -half_size, v.y = half_size; v.x <= half_size; ++v.x, --v.y)
         if (v.y != 0.0f)
            (*offsets)[i++] = v;
      break;

   case VL_MEDIAN_FILTER_HORIZONTAL:
      v.y = 0.0f;
      for (v.x = -half_size; v.x <= half_size; ++v.x)
         (*offsets)[i++] = v;
      break;

   case VL_MEDIAN_FILTER_VERTICAL:
      v.x = 0.0f;
      for (v.y = -half_size; v.y <= half_size; ++v.y)
         (*offsets)[i++] = v;
      break;
   }

   assert(i == *num_offsets);
}

bool
vl_median_filter_init(struct vl_median_filter *filter, struct pipe_context *pipe,
                      unsigned width, unsigned height, unsigned size,
                      enum vl_median_filter_shape shape)
{
   struct pipe_rasterizer_state rs_state;
   struct pipe_blend_state blend;
   struct pipe_sampler_state sampler;
   struct vertex2f *offsets = NULL;
   struct pipe_vertex_element ve;
   unsigned i, num_offsets = 0;

   assert(filter && pipe);
   assert(width && height);
   assert(size > 1 && size < 20);

   memset(filter, 0, sizeof(*filter));
   filter->pipe = pipe;

   memset(&rs_state, 0, sizeof(rs_state));
   rs_state.gl_rasterization_rules = true;
   rs_state.depth_clip = 1;
   filter->rs_state = pipe->create_rasterizer_state(pipe, &rs_state);
   if (!filter->rs_state)
      goto error_rs_state;

   memset(&blend, 0, sizeof blend);
   blend.rt[0].rgb_func = PIPE_BLEND_ADD;
   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_func = PIPE_BLEND_ADD;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ONE;
   blend.logicop_func = PIPE_LOGICOP_CLEAR;
   blend.rt[0].colormask = PIPE_MASK_RGBA;
   filter->blend = pipe->create_blend_state(pipe, &blend);
   if (!filter->blend)
      goto error_blend;

   memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
   sampler.compare_func = PIPE_FUNC_ALWAYS;
   sampler.normalized_coords = 1;
   filter->sampler = pipe->create_sampler_state(pipe, &sampler);
   if (!filter->sampler)
      goto error_sampler;

   filter->quad = vl_vb_upload_quads(pipe);
   if(!filter->quad.buffer)
      goto error_quad;

   memset(&ve, 0, sizeof(ve));
   ve.src_offset = 0;
   ve.instance_divisor = 0;
   ve.vertex_buffer_index = 0;
   ve.src_format = PIPE_FORMAT_R32G32_FLOAT;
   filter->ves = pipe->create_vertex_elements_state(pipe, 1, &ve);
   if (!filter->ves)
      goto error_ves;

   generate_offsets(shape, size, &offsets, &num_offsets);
   if (!offsets)
      goto error_offsets;

   for (i = 0; i < num_offsets; ++i) {
      offsets[i].x /= width;
      offsets[i].y /= height;
   }

   filter->vs = create_vert_shader(filter);
   if (!filter->vs)
      goto error_vs;

   filter->fs = create_frag_shader(filter, offsets, num_offsets);
   if (!filter->fs)
      goto error_fs;

   FREE(offsets);
   return true;

error_fs:
   pipe->delete_vs_state(pipe, filter->vs);

error_vs:
   FREE(offsets);

error_offsets:
   pipe->delete_vertex_elements_state(pipe, filter->ves);

error_ves:
   pipe_resource_reference(&filter->quad.buffer, NULL);

error_quad:
   pipe->delete_sampler_state(pipe, filter->sampler);

error_sampler:
   pipe->delete_blend_state(pipe, filter->blend);

error_blend:
   pipe->delete_rasterizer_state(pipe, filter->rs_state);

error_rs_state:
   return false;
}

void
vl_median_filter_cleanup(struct vl_median_filter *filter)
{
   assert(filter);

   filter->pipe->delete_sampler_state(filter->pipe, filter->sampler);
   filter->pipe->delete_blend_state(filter->pipe, filter->blend);
   filter->pipe->delete_rasterizer_state(filter->pipe, filter->rs_state);
   filter->pipe->delete_vertex_elements_state(filter->pipe, filter->ves);
   pipe_resource_reference(&filter->quad.buffer, NULL);

   filter->pipe->delete_vs_state(filter->pipe, filter->vs);
   filter->pipe->delete_fs_state(filter->pipe, filter->fs);
}

void
vl_median_filter_render(struct vl_median_filter *filter,
                        struct pipe_sampler_view *src,
                        struct pipe_surface *dst)
{
   struct pipe_viewport_state viewport;
   struct pipe_framebuffer_state fb_state;

   assert(filter && src && dst);

   memset(&viewport, 0, sizeof(viewport));
   viewport.scale[0] = dst->width;
   viewport.scale[1] = dst->height;
   viewport.scale[2] = 1;
   viewport.scale[3] = 1;

   memset(&fb_state, 0, sizeof(fb_state));
   fb_state.width = dst->width;
   fb_state.height = dst->height;
   fb_state.nr_cbufs = 1;
   fb_state.cbufs[0] = dst;

   filter->pipe->bind_rasterizer_state(filter->pipe, filter->rs_state);
   filter->pipe->bind_blend_state(filter->pipe, filter->blend);
   filter->pipe->bind_fragment_sampler_states(filter->pipe, 1, &filter->sampler);
   filter->pipe->set_fragment_sampler_views(filter->pipe, 1, &src);
   filter->pipe->bind_vs_state(filter->pipe, filter->vs);
   filter->pipe->bind_fs_state(filter->pipe, filter->fs);
   filter->pipe->set_framebuffer_state(filter->pipe, &fb_state);
   filter->pipe->set_viewport_state(filter->pipe, &viewport);
   filter->pipe->set_vertex_buffers(filter->pipe, 1, &filter->quad);
   filter->pipe->bind_vertex_elements_state(filter->pipe, filter->ves);

   util_draw_arrays(filter->pipe, PIPE_PRIM_QUADS, 0, 4);
}
