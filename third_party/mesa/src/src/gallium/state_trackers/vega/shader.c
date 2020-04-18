/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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

#include "shader.h"

#include "vg_context.h"
#include "shaders_cache.h"
#include "paint.h"
#include "mask.h"
#include "image.h"
#include "renderer.h"

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_format.h"

#define MAX_CONSTANTS 28

struct shader {
   struct vg_context *context;

   VGboolean color_transform;
   VGboolean masking;
   struct vg_paint *paint;
   struct vg_image *image;

   struct matrix modelview;
   struct matrix paint_matrix;

   VGboolean drawing_image;
   VGImageMode image_mode;

   float constants[MAX_CONSTANTS];
   struct pipe_resource *cbuf;
   struct pipe_shader_state fs_state;
   void *fs;
};

struct shader * shader_create(struct vg_context *ctx)
{
   struct shader *shader = 0;

   shader = CALLOC_STRUCT(shader);
   shader->context = ctx;

   return shader;
}

void shader_destroy(struct shader *shader)
{
   FREE(shader);
}

void shader_set_color_transform(struct shader *shader, VGboolean set)
{
   shader->color_transform = set;
}

void shader_set_masking(struct shader *shader, VGboolean set)
{
   shader->masking = set;
}

VGboolean shader_is_masking(struct shader *shader)
{
   return shader->masking;
}

void shader_set_paint(struct shader *shader, struct vg_paint *paint)
{
   shader->paint = paint;
}

struct vg_paint * shader_paint(struct shader *shader)
{
   return shader->paint;
}

static VGint setup_constant_buffer(struct shader *shader)
{
   const struct vg_state *state = &shader->context->state.vg;
   VGint param_bytes = paint_constant_buffer_size(shader->paint);
   VGint i;

   param_bytes += sizeof(VGfloat) * 8;
   assert(param_bytes <= sizeof(shader->constants));

   if (state->color_transform) {
      for (i = 0; i < 8; i++) {
         VGfloat val = (i < 4) ? 127.0f : 1.0f;
         shader->constants[i] =
            CLAMP(state->color_transform_values[i], -val, val);
      }
   }
   else {
      memset(shader->constants, 0, sizeof(VGfloat) * 8);
   }

   paint_fill_constant_buffer(shader->paint,
         &shader->paint_matrix, shader->constants + 8);

   return param_bytes;
}

static VGboolean blend_use_shader(struct shader *shader)
{
   struct vg_context *ctx = shader->context;
   VGboolean advanced_blending;

   switch (ctx->state.vg.blend_mode) {
   case VG_BLEND_DST_OVER:
   case VG_BLEND_MULTIPLY:
   case VG_BLEND_SCREEN:
   case VG_BLEND_DARKEN:
   case VG_BLEND_LIGHTEN:
   case VG_BLEND_ADDITIVE:
      advanced_blending = VG_TRUE;
      break;
   case VG_BLEND_SRC_OVER:
      if (util_format_has_alpha(ctx->draw_buffer->strb->format)) {
         /* no blending is required if the paints and the image are opaque */
         advanced_blending = !paint_is_opaque(ctx->state.vg.fill_paint) ||
                             !paint_is_opaque(ctx->state.vg.stroke_paint);
         if (!advanced_blending && shader->drawing_image) {
            advanced_blending =
               util_format_has_alpha(shader->image->sampler_view->format);
         }
         break;
      }
      /* fall through */
   default:
      advanced_blending = VG_FALSE;
      break;
   }

   return advanced_blending;
}

static VGint blend_bind_samplers(struct shader *shader,
                                 struct pipe_sampler_state **samplers,
                                 struct pipe_sampler_view **sampler_views)
{
   if (blend_use_shader(shader)) {
      struct vg_context *ctx = shader->context;

      samplers[2] = &ctx->blend_sampler;
      sampler_views[2] = vg_prepare_blend_surface(ctx);

      if (!samplers[0] || !sampler_views[0]) {
         samplers[0] = samplers[2];
         sampler_views[0] = sampler_views[2];
      }
      if (!samplers[1] || !sampler_views[1]) {
         samplers[1] = samplers[0];
         sampler_views[1] = sampler_views[0];
      }

      return 1;
   }
   return 0;
}

static VGint setup_samplers(struct shader *shader,
                            struct pipe_sampler_state **samplers,
                            struct pipe_sampler_view **sampler_views)
{
   /* a little wonky: we use the num as a boolean that just says
    * whether any sampler/textures have been set. the actual numbering
    * for samplers is always the same:
    * 0 - paint sampler/texture for gradient/pattern
    * 1 - mask sampler/texture
    * 2 - blend sampler/texture
    * 3 - image sampler/texture
    * */
   VGint num = 0;

   samplers[0] = NULL;
   samplers[1] = NULL;
   samplers[2] = NULL;
   samplers[3] = NULL;
   sampler_views[0] = NULL;
   sampler_views[1] = NULL;
   sampler_views[2] = NULL;
   sampler_views[3] = NULL;

   num += paint_bind_samplers(shader->paint, samplers, sampler_views);
   num += mask_bind_samplers(samplers, sampler_views);
   num += blend_bind_samplers(shader, samplers, sampler_views);
   if (shader->drawing_image && shader->image)
      num += image_bind_samplers(shader->image, samplers, sampler_views);

   return (num) ? 4 : 0;
}

static INLINE VGboolean is_format_bw(struct shader *shader)
{
#if 0
   struct vg_context *ctx = shader->context;
   struct st_framebuffer *stfb = ctx->draw_buffer;
#endif

   if (shader->drawing_image && shader->image) {
      if (shader->image->format == VG_BW_1)
         return VG_TRUE;
   }

   return VG_FALSE;
}

static void setup_shader_program(struct shader *shader)
{
   struct vg_context *ctx = shader->context;
   VGint shader_id = 0;
   VGBlendMode blend_mode = ctx->state.vg.blend_mode;
   VGboolean black_white = is_format_bw(shader);

   /* 1st stage: fill */
   if (!shader->drawing_image ||
       (shader->image_mode == VG_DRAW_IMAGE_MULTIPLY || shader->image_mode == VG_DRAW_IMAGE_STENCIL)) {
      switch(paint_type(shader->paint)) {
      case VG_PAINT_TYPE_COLOR:
         shader_id |= VEGA_SOLID_FILL_SHADER;
         break;
      case VG_PAINT_TYPE_LINEAR_GRADIENT:
         shader_id |= VEGA_LINEAR_GRADIENT_SHADER;
         break;
      case VG_PAINT_TYPE_RADIAL_GRADIENT:
         shader_id |= VEGA_RADIAL_GRADIENT_SHADER;
         break;
      case VG_PAINT_TYPE_PATTERN:
         shader_id |= VEGA_PATTERN_SHADER;
         break;

      default:
         abort();
      }

      if (paint_is_degenerate(shader->paint))
         shader_id = VEGA_PAINT_DEGENERATE_SHADER;
   }

   /* second stage image */
   if (shader->drawing_image) {
      switch(shader->image_mode) {
      case VG_DRAW_IMAGE_NORMAL:
         shader_id |= VEGA_IMAGE_NORMAL_SHADER;
         break;
      case VG_DRAW_IMAGE_MULTIPLY:
         shader_id |= VEGA_IMAGE_MULTIPLY_SHADER;
         break;
      case VG_DRAW_IMAGE_STENCIL:
         shader_id |= VEGA_IMAGE_STENCIL_SHADER;
         break;
      default:
         debug_printf("Unknown image mode!");
      }
   }

   if (shader->color_transform)
      shader_id |= VEGA_COLOR_TRANSFORM_SHADER;

   if (blend_use_shader(shader)) {
      if (shader->drawing_image && shader->image_mode == VG_DRAW_IMAGE_STENCIL)
         shader_id |= VEGA_ALPHA_PER_CHANNEL_SHADER;
      else
         shader_id |= VEGA_ALPHA_NORMAL_SHADER;

      switch(blend_mode) {
      case VG_BLEND_SRC:
         shader_id |= VEGA_BLEND_SRC_SHADER;
         break;
      case VG_BLEND_SRC_OVER:
         shader_id |= VEGA_BLEND_SRC_OVER_SHADER;
         break;
      case VG_BLEND_DST_OVER:
         shader_id |= VEGA_BLEND_DST_OVER_SHADER;
         break;
      case VG_BLEND_SRC_IN:
         shader_id |= VEGA_BLEND_SRC_IN_SHADER;
         break;
      case VG_BLEND_DST_IN:
         shader_id |= VEGA_BLEND_DST_IN_SHADER;
         break;
      case VG_BLEND_MULTIPLY:
         shader_id |= VEGA_BLEND_MULTIPLY_SHADER;
         break;
      case VG_BLEND_SCREEN:
         shader_id |= VEGA_BLEND_SCREEN_SHADER;
         break;
      case VG_BLEND_DARKEN:
         shader_id |= VEGA_BLEND_DARKEN_SHADER;
         break;
      case VG_BLEND_LIGHTEN:
         shader_id |= VEGA_BLEND_LIGHTEN_SHADER;
         break;
      case VG_BLEND_ADDITIVE:
         shader_id |= VEGA_BLEND_ADDITIVE_SHADER;
         break;
      default:
         assert(0);
         break;
      }
   }
   else {
      /* update alpha of the source */
      if (shader->drawing_image && shader->image_mode == VG_DRAW_IMAGE_STENCIL)
         shader_id |= VEGA_ALPHA_PER_CHANNEL_SHADER;
   }

   if (shader->masking)
      shader_id |= VEGA_MASK_SHADER;

   if (black_white)
      shader_id |= VEGA_BW_SHADER;

   shader->fs = shaders_cache_fill(ctx->sc, shader_id);
}


void shader_bind(struct shader *shader)
{
   struct vg_context *ctx = shader->context;
   struct pipe_sampler_state *samplers[PIPE_MAX_SAMPLERS];
   struct pipe_sampler_view *sampler_views[PIPE_MAX_SAMPLERS];
   VGint num_samplers, param_bytes;

   /* first resolve the real paint type */
   paint_resolve_type(shader->paint);

   num_samplers = setup_samplers(shader, samplers, sampler_views);
   param_bytes = setup_constant_buffer(shader);
   setup_shader_program(shader);

   renderer_validate_for_shader(ctx->renderer,
         (const struct pipe_sampler_state **) samplers,
         sampler_views, num_samplers,
         &shader->modelview,
         shader->fs, (const void *) shader->constants, param_bytes);
}

void shader_set_image_mode(struct shader *shader, VGImageMode image_mode)
{
   shader->image_mode = image_mode;
}

VGImageMode shader_image_mode(struct shader *shader)
{
   return shader->image_mode;
}

void shader_set_drawing_image(struct shader *shader, VGboolean drawing_image)
{
   shader->drawing_image = drawing_image;
}

VGboolean shader_drawing_image(struct shader *shader)
{
   return shader->drawing_image;
}

void shader_set_image(struct shader *shader, struct vg_image *img)
{
   shader->image = img;
}

/**
 * Set the transformation to map a vertex to the surface coordinates.
 */
void shader_set_surface_matrix(struct shader *shader,
                               const struct matrix *mat)
{
   shader->modelview = *mat;
}

/**
 * Set the transformation to map a pixel to the paint coordinates.
 */
void shader_set_paint_matrix(struct shader *shader, const struct matrix *mat)
{
   const struct st_framebuffer *stfb = shader->context->draw_buffer;
   const VGfloat px_center_offset = 0.5f;

   memcpy(&shader->paint_matrix, mat, sizeof(*mat));

   /* make it window-to-paint for the shaders */
   matrix_translate(&shader->paint_matrix, px_center_offset,
         stfb->height - 1.0f + px_center_offset);
   matrix_scale(&shader->paint_matrix, 1.0f, -1.0f);
}
