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

#include "VG/openvg.h"

#include "vg_context.h"
#include "paint.h"
#include "path.h"
#include "handle.h"
#include "image.h"
#include "text.h"
#include "matrix.h"
#include "api_consts.h"
#include "api.h"

#include "pipe/p_compiler.h"
#include "util/u_pointer.h"
#include "util/u_math.h"

#include <math.h>

static INLINE struct vg_state *current_state()
{
   struct vg_context *ctx = vg_current_context();
   if (!ctx)
      return 0;
   else
      return &ctx->state.vg;
}

static INLINE VGboolean count_in_bounds(VGParamType type, VGint count)
{
   if (count < 0)
      return VG_FALSE;

   if (type == VG_SCISSOR_RECTS)
      return (!(count % 4) && (count >= 0 || count <= VEGA_MAX_SCISSOR_RECTS * 4));
   else if (type == VG_STROKE_DASH_PATTERN) {
      return count <= VEGA_MAX_DASH_COUNT;
   } else {
      VGint real_count = vegaGetVectorSize(type);
      return count == real_count;
   }
}

void vegaSetf (VGParamType type, VGfloat value)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_state *state = current_state();
   VGErrorCode error = VG_NO_ERROR;

   switch(type) {
   case VG_MATRIX_MODE:
   case VG_FILL_RULE:
   case VG_IMAGE_QUALITY:
   case VG_RENDERING_QUALITY:
   case VG_BLEND_MODE:
   case VG_IMAGE_MODE:
#ifdef OPENVG_VERSION_1_1
   case VG_COLOR_TRANSFORM:
#endif
   case VG_STROKE_CAP_STYLE:
   case VG_STROKE_JOIN_STYLE:
   case VG_STROKE_DASH_PHASE_RESET:
   case VG_MASKING:
   case VG_SCISSORING:
   case VG_PIXEL_LAYOUT:
   case VG_SCREEN_LAYOUT:
   case VG_FILTER_FORMAT_LINEAR:
   case VG_FILTER_FORMAT_PREMULTIPLIED:
   case VG_FILTER_CHANNEL_MASK:

   case VG_MAX_SCISSOR_RECTS:
   case VG_MAX_DASH_COUNT:
   case VG_MAX_KERNEL_SIZE:
   case VG_MAX_SEPARABLE_KERNEL_SIZE:
   case VG_MAX_COLOR_RAMP_STOPS:
   case VG_MAX_IMAGE_WIDTH:
   case VG_MAX_IMAGE_HEIGHT:
   case VG_MAX_IMAGE_PIXELS:
   case VG_MAX_IMAGE_BYTES:
   case VG_MAX_GAUSSIAN_STD_DEVIATION:
   case VG_MAX_FLOAT:
      vegaSeti(type, floor(value));
      return;
      break;
   case VG_STROKE_LINE_WIDTH:
      state->stroke.line_width.f = value;
      state->stroke.line_width.i = float_to_int_floor(*((VGuint*)(&value)));
      break;
   case VG_STROKE_MITER_LIMIT:
      state->stroke.miter_limit.f = value;
      state->stroke.miter_limit.i = float_to_int_floor(*((VGuint*)(&value)));
      break;
   case VG_STROKE_DASH_PHASE:
      state->stroke.dash_phase.f = value;
      state->stroke.dash_phase.i = float_to_int_floor(*((VGuint*)(&value)));
      break;
   default:
      error = VG_ILLEGAL_ARGUMENT_ERROR;
      break;
   }
   vg_set_error(ctx, error);
}

void vegaSeti (VGParamType type, VGint value)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_state *state = current_state();
   VGErrorCode error = VG_NO_ERROR;

   switch(type) {
   case VG_MATRIX_MODE:
      if (value < VG_MATRIX_PATH_USER_TO_SURFACE ||
#ifdef OPENVG_VERSION_1_1
          value > VG_MATRIX_GLYPH_USER_TO_SURFACE)
#else
          value > VG_MATRIX_STROKE_PAINT_TO_USER)
#endif
         error = VG_ILLEGAL_ARGUMENT_ERROR;
      else
         state->matrix_mode = value;
      break;
   case VG_FILL_RULE:
      if (value < VG_EVEN_ODD ||
          value > VG_NON_ZERO)
         error = VG_ILLEGAL_ARGUMENT_ERROR;
      else
         state->fill_rule = value;
      break;
   case VG_IMAGE_QUALITY:
      state->image_quality = value;
      break;
   case VG_RENDERING_QUALITY:
      if (value < VG_RENDERING_QUALITY_NONANTIALIASED ||
          value > VG_RENDERING_QUALITY_BETTER)
         error = VG_ILLEGAL_ARGUMENT_ERROR;
      else
         state->rendering_quality = value;
      break;
   case VG_BLEND_MODE:
      if (value < VG_BLEND_SRC ||
          value > VG_BLEND_ADDITIVE)
         error = VG_ILLEGAL_ARGUMENT_ERROR;
      else {
         ctx->state.dirty |= BLEND_DIRTY;
         state->blend_mode = value;
      }
      break;
   case VG_IMAGE_MODE:
      if (value < VG_DRAW_IMAGE_NORMAL ||
          value > VG_DRAW_IMAGE_STENCIL)
         error = VG_ILLEGAL_ARGUMENT_ERROR;
      else
         state->image_mode = value;
      break;
#ifdef OPENVG_VERSION_1_1
   case VG_COLOR_TRANSFORM:
      state->color_transform = value;
#endif
      break;
   case VG_STROKE_LINE_WIDTH:
      state->stroke.line_width.f = value;
      state->stroke.line_width.i = value;
      break;
   case VG_STROKE_CAP_STYLE:
      if (value < VG_CAP_BUTT ||
          value > VG_CAP_SQUARE)
         error = VG_ILLEGAL_ARGUMENT_ERROR;
      else
         state->stroke.cap_style = value;
      break;
   case VG_STROKE_JOIN_STYLE:
      if (value < VG_JOIN_MITER ||
          value > VG_JOIN_BEVEL)
         error = VG_ILLEGAL_ARGUMENT_ERROR;
      else
         state->stroke.join_style = value;
      break;
   case VG_STROKE_MITER_LIMIT:
      state->stroke.miter_limit.f = value;
      state->stroke.miter_limit.i = value;
      break;
   case VG_STROKE_DASH_PHASE:
      state->stroke.dash_phase.f = value;
      state->stroke.dash_phase.i = value;
      break;
   case VG_STROKE_DASH_PHASE_RESET:
      state->stroke.dash_phase_reset = value;
      break;
   case VG_MASKING:
      state->masking = value;
      break;
   case VG_SCISSORING:
      state->scissoring = value;
      ctx->state.dirty |= DEPTH_STENCIL_DIRTY;
      break;
   case VG_PIXEL_LAYOUT:
      if (value < VG_PIXEL_LAYOUT_UNKNOWN ||
          value > VG_PIXEL_LAYOUT_BGR_HORIZONTAL)
         error = VG_ILLEGAL_ARGUMENT_ERROR;
      else
         state->pixel_layout = value;
      break;
   case VG_SCREEN_LAYOUT:
      /* read only ignore */
      break;
   case VG_FILTER_FORMAT_LINEAR:
      state->filter_format_linear = value;
      break;
   case VG_FILTER_FORMAT_PREMULTIPLIED:
      state->filter_format_premultiplied = value;
      break;
   case VG_FILTER_CHANNEL_MASK:
      state->filter_channel_mask = value;
      break;

   case VG_MAX_SCISSOR_RECTS:
   case VG_MAX_DASH_COUNT:
   case VG_MAX_KERNEL_SIZE:
   case VG_MAX_SEPARABLE_KERNEL_SIZE:
   case VG_MAX_COLOR_RAMP_STOPS:
   case VG_MAX_IMAGE_WIDTH:
   case VG_MAX_IMAGE_HEIGHT:
   case VG_MAX_IMAGE_PIXELS:
   case VG_MAX_IMAGE_BYTES:
   case VG_MAX_GAUSSIAN_STD_DEVIATION:
   case VG_MAX_FLOAT:
      /* read only ignore */
      break;
   default:
      error = VG_ILLEGAL_ARGUMENT_ERROR;
      break;
   }
   vg_set_error(ctx, error);
}

void vegaSetfv(VGParamType type, VGint count,
               const VGfloat * values)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_state *state = current_state();
   VGErrorCode error = VG_NO_ERROR;

   if ((count && !values) || !count_in_bounds(type, count) || !is_aligned(values)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   switch(type) {
   case VG_MATRIX_MODE:
   case VG_FILL_RULE:
   case VG_IMAGE_QUALITY:
   case VG_RENDERING_QUALITY:
   case VG_BLEND_MODE:
   case VG_IMAGE_MODE:
#ifdef OPENVG_VERSION_1_1
   case VG_COLOR_TRANSFORM:
#endif
   case VG_STROKE_CAP_STYLE:
   case VG_STROKE_JOIN_STYLE:
   case VG_STROKE_DASH_PHASE_RESET:
   case VG_MASKING:
   case VG_SCISSORING:
   case VG_PIXEL_LAYOUT:
   case VG_SCREEN_LAYOUT:
   case VG_FILTER_FORMAT_LINEAR:
   case VG_FILTER_FORMAT_PREMULTIPLIED:
   case VG_FILTER_CHANNEL_MASK:
      vegaSeti(type, floor(values[0]));
      return;
      break;
   case VG_SCISSOR_RECTS: {
      VGint i;
      VGuint *x = (VGuint*)values;
      for (i = 0; i < count; ++i) {
         state->scissor_rects[i].f =  values[i];
         state->scissor_rects[i].i =  float_to_int_floor(x[i]);
      }
      state->scissor_rects_num = count / 4;
      ctx->state.dirty |= DEPTH_STENCIL_DIRTY;
   }
      break;
#ifdef OPENVG_VERSION_1_1
   case VG_COLOR_TRANSFORM_VALUES: {
      VGint i;
      for (i = 0; i < count; ++i) {
         state->color_transform_values[i] =  values[i];
      }
   }
      break;
#endif
   case VG_STROKE_LINE_WIDTH:
      state->stroke.line_width.f = values[0];
      state->stroke.line_width.i = float_to_int_floor(*((VGuint*)(values)));
      break;
   case VG_STROKE_MITER_LIMIT:
      state->stroke.miter_limit.f = values[0];
      state->stroke.miter_limit.i = float_to_int_floor(*((VGuint*)(values)));
      break;
   case VG_STROKE_DASH_PATTERN: {
      int i;
      for (i = 0; i < count; ++i) {
         state->stroke.dash_pattern[i].f = values[i];
         state->stroke.dash_pattern[i].i =
            float_to_int_floor(*((VGuint*)(values + i)));
      }
      state->stroke.dash_pattern_num = count;
   }
      break;
   case VG_STROKE_DASH_PHASE:
      state->stroke.dash_phase.f = values[0];
      state->stroke.dash_phase.i = float_to_int_floor(*((VGuint*)(values)));
      break;
   case VG_TILE_FILL_COLOR:
      state->tile_fill_color[0] = values[0];
      state->tile_fill_color[1] = values[1];
      state->tile_fill_color[2] = values[2];
      state->tile_fill_color[3] = values[3];

      state->tile_fill_colori[0] = float_to_int_floor(*((VGuint*)(values + 0)));
      state->tile_fill_colori[1] = float_to_int_floor(*((VGuint*)(values + 1)));
      state->tile_fill_colori[2] = float_to_int_floor(*((VGuint*)(values + 2)));
      state->tile_fill_colori[3] = float_to_int_floor(*((VGuint*)(values + 3)));
      break;
   case VG_CLEAR_COLOR:
      state->clear_color[0] = values[0];
      state->clear_color[1] = values[1];
      state->clear_color[2] = values[2];
      state->clear_color[3] = values[3];

      state->clear_colori[0] = float_to_int_floor(*((VGuint*)(values + 0)));
      state->clear_colori[1] = float_to_int_floor(*((VGuint*)(values + 1)));
      state->clear_colori[2] = float_to_int_floor(*((VGuint*)(values + 2)));
      state->clear_colori[3] = float_to_int_floor(*((VGuint*)(values + 3)));
      break;
#ifdef OPENVG_VERSION_1_1
   case VG_GLYPH_ORIGIN:
      state->glyph_origin[0].f = values[0];
      state->glyph_origin[1].f = values[1];

      state->glyph_origin[0].i = float_to_int_floor(*((VGuint*)(values + 0)));
      state->glyph_origin[1].i = float_to_int_floor(*((VGuint*)(values + 1)));
      break;
#endif

   case VG_MAX_SCISSOR_RECTS:
   case VG_MAX_DASH_COUNT:
   case VG_MAX_KERNEL_SIZE:
   case VG_MAX_SEPARABLE_KERNEL_SIZE:
   case VG_MAX_COLOR_RAMP_STOPS:
   case VG_MAX_IMAGE_WIDTH:
   case VG_MAX_IMAGE_HEIGHT:
   case VG_MAX_IMAGE_PIXELS:
   case VG_MAX_IMAGE_BYTES:
   case VG_MAX_GAUSSIAN_STD_DEVIATION:
   case VG_MAX_FLOAT:
      break;
   default:
      error = VG_ILLEGAL_ARGUMENT_ERROR;
      break;
   }
   vg_set_error(ctx, error);
}

void vegaSetiv(VGParamType type, VGint count,
               const VGint * values)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_state *state = current_state();

   if ((count && !values) || !count_in_bounds(type, count) || !is_aligned(values)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   switch(type) {
   case VG_MATRIX_MODE:
   case VG_FILL_RULE:
   case VG_IMAGE_QUALITY:
   case VG_RENDERING_QUALITY:
   case VG_BLEND_MODE:
   case VG_IMAGE_MODE:
#ifdef OPENVG_VERSION_1_1
   case VG_COLOR_TRANSFORM:
#endif
   case VG_STROKE_CAP_STYLE:
   case VG_STROKE_JOIN_STYLE:
   case VG_STROKE_DASH_PHASE_RESET:
   case VG_MASKING:
   case VG_SCISSORING:
   case VG_PIXEL_LAYOUT:
   case VG_SCREEN_LAYOUT:
   case VG_FILTER_FORMAT_LINEAR:
   case VG_FILTER_FORMAT_PREMULTIPLIED:
   case VG_FILTER_CHANNEL_MASK:
      vegaSeti(type, values[0]);
      return;
      break;
   case VG_SCISSOR_RECTS: {
      VGint i;
      for (i = 0; i < count; ++i) {
         state->scissor_rects[i].i =  values[i];
         state->scissor_rects[i].f =  values[i];
      }
      state->scissor_rects_num = count / 4;
      ctx->state.dirty |= DEPTH_STENCIL_DIRTY;
   }
      break;
#ifdef OPENVG_VERSION_1_1
   case VG_COLOR_TRANSFORM_VALUES: {
      VGint i;
      for (i = 0; i < count; ++i) {
         state->color_transform_values[i] =  values[i];
      }
   }
      break;
#endif
   case VG_STROKE_LINE_WIDTH:
      state->stroke.line_width.f = values[0];
      state->stroke.line_width.i = values[0];
      break;
   case VG_STROKE_MITER_LIMIT:
      state->stroke.miter_limit.f = values[0];
      state->stroke.miter_limit.i = values[0];
      break;
   case VG_STROKE_DASH_PATTERN: {
      int i;
      for (i = 0; i < count; ++i) {
         state->stroke.dash_pattern[i].f = values[i];
         state->stroke.dash_pattern[i].i = values[i];
      }
      state->stroke.dash_pattern_num = count;
   }
      break;
   case VG_STROKE_DASH_PHASE:
      state->stroke.dash_phase.f = values[0];
      state->stroke.dash_phase.i = values[0];
      break;
   case VG_TILE_FILL_COLOR:
      state->tile_fill_color[0] = values[0];
      state->tile_fill_color[1] = values[1];
      state->tile_fill_color[2] = values[2];
      state->tile_fill_color[3] = values[3];

      state->tile_fill_colori[0] = values[0];
      state->tile_fill_colori[1] = values[1];
      state->tile_fill_colori[2] = values[2];
      state->tile_fill_colori[3] = values[3];
      break;
   case VG_CLEAR_COLOR:
      state->clear_color[0] = values[0];
      state->clear_color[1] = values[1];
      state->clear_color[2] = values[2];
      state->clear_color[3] = values[3];

      state->clear_colori[0] = values[0];
      state->clear_colori[1] = values[1];
      state->clear_colori[2] = values[2];
      state->clear_colori[3] = values[3];
      break;
#ifdef OPENVG_VERSION_1_1
   case VG_GLYPH_ORIGIN:
      state->glyph_origin[0].f = values[0];
      state->glyph_origin[1].f = values[1];
      state->glyph_origin[0].i = values[0];
      state->glyph_origin[1].i = values[1];
      break;
#endif

   case VG_MAX_SCISSOR_RECTS:
   case VG_MAX_DASH_COUNT:
   case VG_MAX_KERNEL_SIZE:
   case VG_MAX_SEPARABLE_KERNEL_SIZE:
   case VG_MAX_COLOR_RAMP_STOPS:
   case VG_MAX_IMAGE_WIDTH:
   case VG_MAX_IMAGE_HEIGHT:
   case VG_MAX_IMAGE_PIXELS:
   case VG_MAX_IMAGE_BYTES:
   case VG_MAX_GAUSSIAN_STD_DEVIATION:
   case VG_MAX_FLOAT:
      break;

   default:
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      break;
   }
}

VGfloat vegaGetf(VGParamType type)
{
   struct vg_context *ctx = vg_current_context();
   const struct vg_state *state = current_state();
   VGErrorCode error = VG_NO_ERROR;
   VGfloat value = 0.0f;

   switch(type) {
   case VG_MATRIX_MODE:
   case VG_FILL_RULE:
   case VG_IMAGE_QUALITY:
   case VG_RENDERING_QUALITY:
   case VG_BLEND_MODE:
   case VG_IMAGE_MODE:
#ifdef OPENVG_VERSION_1_1
   case VG_COLOR_TRANSFORM:
#endif
   case VG_STROKE_CAP_STYLE:
   case VG_STROKE_JOIN_STYLE:
   case VG_STROKE_DASH_PHASE_RESET:
   case VG_MASKING:
   case VG_SCISSORING:
   case VG_PIXEL_LAYOUT:
   case VG_SCREEN_LAYOUT:
   case VG_FILTER_FORMAT_LINEAR:
   case VG_FILTER_FORMAT_PREMULTIPLIED:
   case VG_FILTER_CHANNEL_MASK:
      return vegaGeti(type);
      break;
   case VG_STROKE_LINE_WIDTH:
      value = state->stroke.line_width.f;
      break;
   case VG_STROKE_MITER_LIMIT:
      value = state->stroke.miter_limit.f;
      break;
   case VG_STROKE_DASH_PHASE:
      value = state->stroke.dash_phase.f;
      break;

   case VG_MAX_SCISSOR_RECTS:
   case VG_MAX_DASH_COUNT:
   case VG_MAX_KERNEL_SIZE:
   case VG_MAX_SEPARABLE_KERNEL_SIZE:
   case VG_MAX_COLOR_RAMP_STOPS:
   case VG_MAX_IMAGE_WIDTH:
   case VG_MAX_IMAGE_HEIGHT:
   case VG_MAX_IMAGE_PIXELS:
   case VG_MAX_IMAGE_BYTES:
   case VG_MAX_GAUSSIAN_STD_DEVIATION:
      return vegaGeti(type);
      break;
   case VG_MAX_FLOAT:
      value = 1e+10;/*must be at least 1e+10*/
      break;
   default:
      error = VG_ILLEGAL_ARGUMENT_ERROR;
      break;
   }
   vg_set_error(ctx, error);
   return value;
}

VGint vegaGeti(VGParamType type)
{
   const struct vg_state *state = current_state();
   struct vg_context *ctx = vg_current_context();
   VGErrorCode error = VG_NO_ERROR;
   VGint value = 0;

   switch(type) {
   case VG_MATRIX_MODE:
      value = state->matrix_mode;
      break;
   case VG_FILL_RULE:
      value = state->fill_rule;
      break;
   case VG_IMAGE_QUALITY:
      value = state->image_quality;
      break;
   case VG_RENDERING_QUALITY:
      value = state->rendering_quality;
      break;
   case VG_BLEND_MODE:
      value = state->blend_mode;
      break;
   case VG_IMAGE_MODE:
      value = state->image_mode;
      break;
#ifdef OPENVG_VERSION_1_1
   case VG_COLOR_TRANSFORM:
      value = state->color_transform;
      break;
#endif
   case VG_STROKE_LINE_WIDTH:
      value = state->stroke.line_width.i;
      break;
   case VG_STROKE_CAP_STYLE:
      value = state->stroke.cap_style;
      break;
   case VG_STROKE_JOIN_STYLE:
      value = state->stroke.join_style;
      break;
   case VG_STROKE_MITER_LIMIT:
      value = state->stroke.miter_limit.i;
      break;
   case VG_STROKE_DASH_PHASE:
      value = state->stroke.dash_phase.i;
      break;
   case VG_STROKE_DASH_PHASE_RESET:
      value = state->stroke.dash_phase_reset;
      break;
   case VG_MASKING:
      value = state->masking;
      break;
   case VG_SCISSORING:
      value = state->scissoring;
      break;
   case VG_PIXEL_LAYOUT:
      value = state->pixel_layout;
      break;
   case VG_SCREEN_LAYOUT:
      value = state->screen_layout;
      break;
   case VG_FILTER_FORMAT_LINEAR:
      value = state->filter_format_linear;
      break;
   case VG_FILTER_FORMAT_PREMULTIPLIED:
      value = state->filter_format_premultiplied;
      break;
   case VG_FILTER_CHANNEL_MASK:
      value = state->filter_channel_mask;
      break;

   case VG_MAX_SCISSOR_RECTS:
      value = 32; /*must be at least 32*/
      break;
   case VG_MAX_DASH_COUNT:
      value = 16; /*must be at least 16*/
      break;
   case VG_MAX_KERNEL_SIZE:
      value = 7; /*must be at least 7*/
      break;
   case VG_MAX_SEPARABLE_KERNEL_SIZE:
      value = 15; /*must be at least 15*/
      break;
   case VG_MAX_COLOR_RAMP_STOPS:
      value = 256; /*must be at least 32*/
      break;
   case VG_MAX_IMAGE_WIDTH:
      value = 2048;
      break;
   case VG_MAX_IMAGE_HEIGHT:
      value = 2048;
      break;
   case VG_MAX_IMAGE_PIXELS:
      value = 2048*2048;
      break;
   case VG_MAX_IMAGE_BYTES:
      value = 2048*2048 * 4;
      break;
   case VG_MAX_GAUSSIAN_STD_DEVIATION:
      value = 128; /*must be at least 128*/
      break;

   case VG_MAX_FLOAT: {
      VGfloat val = vegaGetf(type);
      value = float_to_int_floor(*((VGuint*)&val));
   }
      break;
   default:
      error = VG_ILLEGAL_ARGUMENT_ERROR;
      break;
   }
   vg_set_error(ctx, error);
   return value;
}

VGint vegaGetVectorSize(VGParamType type)
{
   struct vg_context *ctx = vg_current_context();
   const struct vg_state *state = current_state();
   switch(type) {
   case VG_MATRIX_MODE:
   case VG_FILL_RULE:
   case VG_IMAGE_QUALITY:
   case VG_RENDERING_QUALITY:
   case VG_BLEND_MODE:
   case VG_IMAGE_MODE:
      return 1;
   case VG_SCISSOR_RECTS:
      return state->scissor_rects_num * 4;
#ifdef OPENVG_VERSION_1_1
   case VG_COLOR_TRANSFORM:
      return 1;
   case VG_COLOR_TRANSFORM_VALUES:
      return 8;
#endif
   case VG_STROKE_LINE_WIDTH:
   case VG_STROKE_CAP_STYLE:
   case VG_STROKE_JOIN_STYLE:
   case VG_STROKE_MITER_LIMIT:
      return 1;
   case VG_STROKE_DASH_PATTERN:
      return state->stroke.dash_pattern_num;
   case VG_STROKE_DASH_PHASE:
      return 1;
   case VG_STROKE_DASH_PHASE_RESET:
      return 1;
   case VG_TILE_FILL_COLOR:
      return 4;
   case VG_CLEAR_COLOR:
      return 4;
#ifdef OPENVG_VERSION_1_1
   case VG_GLYPH_ORIGIN:
      return 2;
#endif
   case VG_MASKING:
      return 1;
   case VG_SCISSORING:
      return 1;
   case VG_PIXEL_LAYOUT:
      return 1;
   case VG_SCREEN_LAYOUT:
      return 1;
   case VG_FILTER_FORMAT_LINEAR:
      return 1;
   case VG_FILTER_FORMAT_PREMULTIPLIED:
      return 1;
   case VG_FILTER_CHANNEL_MASK:
      return 1;

   case VG_MAX_COLOR_RAMP_STOPS:
      return 1;
   case VG_MAX_SCISSOR_RECTS:
   case VG_MAX_DASH_COUNT:
   case VG_MAX_KERNEL_SIZE:
   case VG_MAX_SEPARABLE_KERNEL_SIZE:
   case VG_MAX_IMAGE_WIDTH:
   case VG_MAX_IMAGE_HEIGHT:
   case VG_MAX_IMAGE_PIXELS:
   case VG_MAX_IMAGE_BYTES:
   case VG_MAX_FLOAT:
   case VG_MAX_GAUSSIAN_STD_DEVIATION:
      return 1;
   default:
      if (ctx)
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return 0;
   }
}

void vegaGetfv(VGParamType type, VGint count,
               VGfloat * values)
{
   const struct vg_state *state = current_state();
   struct vg_context *ctx = vg_current_context();
   VGint real_count = vegaGetVectorSize(type);

   if (!values || count <= 0 || count > real_count || !is_aligned(values)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   switch(type) {
   case VG_MATRIX_MODE:
   case VG_FILL_RULE:
   case VG_IMAGE_QUALITY:
   case VG_RENDERING_QUALITY:
   case VG_BLEND_MODE:
   case VG_IMAGE_MODE:
#ifdef OPENVG_VERSION_1_1
   case VG_COLOR_TRANSFORM:
#endif
   case VG_STROKE_CAP_STYLE:
   case VG_STROKE_JOIN_STYLE:
   case VG_STROKE_DASH_PHASE_RESET:
   case VG_MASKING:
   case VG_SCISSORING:
   case VG_PIXEL_LAYOUT:
   case VG_SCREEN_LAYOUT:
   case VG_FILTER_FORMAT_LINEAR:
   case VG_FILTER_FORMAT_PREMULTIPLIED:
   case VG_FILTER_CHANNEL_MASK:
   case VG_MAX_SCISSOR_RECTS:
   case VG_MAX_DASH_COUNT:
   case VG_MAX_KERNEL_SIZE:
   case VG_MAX_SEPARABLE_KERNEL_SIZE:
   case VG_MAX_COLOR_RAMP_STOPS:
   case VG_MAX_IMAGE_WIDTH:
   case VG_MAX_IMAGE_HEIGHT:
   case VG_MAX_IMAGE_PIXELS:
   case VG_MAX_IMAGE_BYTES:
   case VG_MAX_GAUSSIAN_STD_DEVIATION:
      values[0] = vegaGeti(type);
      break;
   case VG_MAX_FLOAT:
      values[0] = vegaGetf(type);
      break;
   case VG_SCISSOR_RECTS: {
      VGint i;
      for (i = 0; i < count; ++i) {
         values[i] = state->scissor_rects[i].f;
      }
   }
      break;
#ifdef OPENVG_VERSION_1_1
   case VG_COLOR_TRANSFORM_VALUES: {
      memcpy(values, state->color_transform_values,
             sizeof(VGfloat) * count);
   }
      break;
#endif
   case VG_STROKE_LINE_WIDTH:
      values[0] = state->stroke.line_width.f;
      break;
   case VG_STROKE_MITER_LIMIT:
      values[0] = state->stroke.miter_limit.f;
      break;
   case VG_STROKE_DASH_PATTERN: {
      VGint i;
      for (i = 0; i < count; ++i) {
         values[i] = state->stroke.dash_pattern[i].f;
      }
   }
      break;
   case VG_STROKE_DASH_PHASE:
      values[0] = state->stroke.dash_phase.f;
      break;
   case VG_TILE_FILL_COLOR:
      values[0] = state->tile_fill_color[0];
      values[1] = state->tile_fill_color[1];
      values[2] = state->tile_fill_color[2];
      values[3] = state->tile_fill_color[3];
      break;
   case VG_CLEAR_COLOR:
      values[0] = state->clear_color[0];
      values[1] = state->clear_color[1];
      values[2] = state->clear_color[2];
      values[3] = state->clear_color[3];
      break;
#ifdef OPENVG_VERSION_1_1
   case VG_GLYPH_ORIGIN:
      values[0] = state->glyph_origin[0].f;
      values[1] = state->glyph_origin[1].f;
      break;
#endif
   default:
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      break;
   }
}

void vegaGetiv(VGParamType type, VGint count,
               VGint * values)
{
   const struct vg_state *state = current_state();
   struct vg_context *ctx = vg_current_context();
   VGint real_count = vegaGetVectorSize(type);

   if (!values || count <= 0 || count > real_count || !is_aligned(values)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   switch(type) {
   case VG_MATRIX_MODE:
   case VG_FILL_RULE:
   case VG_IMAGE_QUALITY:
   case VG_RENDERING_QUALITY:
   case VG_BLEND_MODE:
   case VG_IMAGE_MODE:
#ifdef OPENVG_VERSION_1_1
   case VG_COLOR_TRANSFORM:
#endif
   case VG_STROKE_CAP_STYLE:
   case VG_STROKE_JOIN_STYLE:
   case VG_STROKE_DASH_PHASE_RESET:
   case VG_MASKING:
   case VG_SCISSORING:
   case VG_PIXEL_LAYOUT:
   case VG_SCREEN_LAYOUT:
   case VG_FILTER_FORMAT_LINEAR:
   case VG_FILTER_FORMAT_PREMULTIPLIED:
   case VG_FILTER_CHANNEL_MASK:
   case VG_MAX_SCISSOR_RECTS:
   case VG_MAX_DASH_COUNT:
   case VG_MAX_KERNEL_SIZE:
   case VG_MAX_SEPARABLE_KERNEL_SIZE:
   case VG_MAX_COLOR_RAMP_STOPS:
   case VG_MAX_IMAGE_WIDTH:
   case VG_MAX_IMAGE_HEIGHT:
   case VG_MAX_IMAGE_PIXELS:
   case VG_MAX_IMAGE_BYTES:
   case VG_MAX_GAUSSIAN_STD_DEVIATION:
      values[0] = vegaGeti(type);
      break;
   case VG_MAX_FLOAT: {
      VGfloat val = vegaGetf(type);
      values[0] = float_to_int_floor(*((VGuint*)&val));
   }
      break;
   case VG_SCISSOR_RECTS: {
      VGint i;
      for (i = 0; i < count; ++i) {
         values[i] = state->scissor_rects[i].i;
      }
   }
      break;
#ifdef OPENVG_VERSION_1_1
   case VG_COLOR_TRANSFORM_VALUES: {
      VGint i;
      VGuint *x = (VGuint*)state->color_transform_values;
      for (i = 0; i < count; ++i) {
         values[i] = float_to_int_floor(x[i]);
      }
   }
      break;
#endif
   case VG_STROKE_LINE_WIDTH:
      values[0] = state->stroke.line_width.i;
      break;
   case VG_STROKE_MITER_LIMIT:
      values[0] = state->stroke.miter_limit.i;
      break;
   case VG_STROKE_DASH_PATTERN: {
      VGint i;
      for (i = 0; i < count; ++i) {
         values[i] = state->stroke.dash_pattern[i].i;
      }
   }
      break;
   case VG_STROKE_DASH_PHASE:
      values[0] = state->stroke.dash_phase.i;
      break;
   case VG_TILE_FILL_COLOR:
      values[0] = state->tile_fill_colori[0];
      values[1] = state->tile_fill_colori[1];
      values[2] = state->tile_fill_colori[2];
      values[3] = state->tile_fill_colori[3];
      break;
   case VG_CLEAR_COLOR:
      values[0] = state->clear_colori[0];
      values[1] = state->clear_colori[1];
      values[2] = state->clear_colori[2];
      values[3] = state->clear_colori[3];
      break;
#ifdef OPENVG_VERSION_1_1
   case VG_GLYPH_ORIGIN:
      values[0] = state->glyph_origin[0].i;
      values[1] = state->glyph_origin[1].i;
      break;
#endif
   default:
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      break;
   }
}

void vegaSetParameterf(VGHandle object,
                       VGint paramType,
                       VGfloat value)
{
   struct vg_context *ctx = vg_current_context();
   void *ptr = handle_to_pointer(object);

   if (object == VG_INVALID_HANDLE || !is_aligned(ptr)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   switch(paramType) {
   case VG_PAINT_TYPE:
   case VG_PAINT_COLOR_RAMP_SPREAD_MODE:
   case VG_PAINT_PATTERN_TILING_MODE:
      vegaSetParameteri(object, paramType, floor(value));
      return;
      break;
   case VG_PAINT_COLOR:
   case VG_PAINT_COLOR_RAMP_STOPS:
   case VG_PAINT_LINEAR_GRADIENT:
   case VG_PAINT_RADIAL_GRADIENT:
      /* it's an error if paramType refers to a vector parameter */
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      break;
   case VG_PAINT_COLOR_RAMP_PREMULTIPLIED: {
      struct vg_paint *p = handle_to_paint(object);
      paint_set_color_ramp_premultiplied(p, value);
   }
      break;

   case VG_PATH_DATATYPE:
   case VG_PATH_FORMAT:
   case VG_PATH_SCALE:
   case VG_PATH_BIAS:
   case VG_PATH_NUM_SEGMENTS:
   case VG_PATH_NUM_COORDS:

   case VG_IMAGE_FORMAT:
   case VG_IMAGE_WIDTH:
   case VG_IMAGE_HEIGHT:

#ifdef OPENVG_VERSION_1_1
   case VG_FONT_NUM_GLYPHS:
      /* read only don't produce an error */
      break;
#endif
   default:
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      break;
   }
}

void vegaSetParameteri(VGHandle object,
                       VGint paramType,
                       VGint value)
{
   struct vg_context *ctx = vg_current_context();
   void *ptr = handle_to_pointer(object);

   if (object == VG_INVALID_HANDLE || !is_aligned(ptr)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   switch(paramType) {
   case VG_PAINT_TYPE:
      if (value < VG_PAINT_TYPE_COLOR ||
          value > VG_PAINT_TYPE_PATTERN)
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      else {
         struct vg_paint *paint = handle_to_paint(object);
         paint_set_type(paint, value);
      }
      break;
   case VG_PAINT_COLOR:
   case VG_PAINT_COLOR_RAMP_STOPS:
   case VG_PAINT_LINEAR_GRADIENT:
   case VG_PAINT_RADIAL_GRADIENT:
      /* it's an error if paramType refers to a vector parameter */
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      break;
   case VG_PAINT_COLOR_RAMP_SPREAD_MODE:
      if (value < VG_COLOR_RAMP_SPREAD_PAD ||
          value > VG_COLOR_RAMP_SPREAD_REFLECT)
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      else {
         struct vg_paint *paint = handle_to_paint(object);
         paint_set_spread_mode(paint, value);
      }
      break;
   case VG_PAINT_COLOR_RAMP_PREMULTIPLIED: {
      struct vg_paint *p = handle_to_paint(object);
      paint_set_color_ramp_premultiplied(p, value);
   }
      break;
   case VG_PAINT_PATTERN_TILING_MODE:
      if (value < VG_TILE_FILL ||
          value > VG_TILE_REFLECT)
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      else {
         struct vg_paint *paint = handle_to_paint(object);
         paint_set_pattern_tiling(paint, value);
      }
      break;

   case VG_PATH_DATATYPE:
   case VG_PATH_FORMAT:
   case VG_PATH_SCALE:
   case VG_PATH_BIAS:
   case VG_PATH_NUM_SEGMENTS:
   case VG_PATH_NUM_COORDS:

   case VG_IMAGE_FORMAT:
   case VG_IMAGE_WIDTH:
   case VG_IMAGE_HEIGHT:

#ifdef OPENVG_VERSION_1_1
   case VG_FONT_NUM_GLYPHS:
      /* read only don't produce an error */
      break;
#endif
   default:
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
}

void vegaSetParameterfv(VGHandle object,
                        VGint paramType,
                        VGint count,
                        const VGfloat * values)
{
   struct vg_context *ctx = vg_current_context();
   void *ptr = handle_to_pointer(object);
   VGint real_count = vegaGetParameterVectorSize(object, paramType);

   if (object == VG_INVALID_HANDLE || !is_aligned(ptr)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (count < 0 || count < real_count ||
       (values == NULL && count != 0) ||
       !is_aligned(values)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   switch(paramType) {
   case VG_PAINT_TYPE:
   case VG_PAINT_COLOR_RAMP_SPREAD_MODE:
   case VG_PAINT_COLOR_RAMP_PREMULTIPLIED:
   case VG_PAINT_PATTERN_TILING_MODE:
      if (count != 1)
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      else
         vegaSetParameterf(object, paramType, values[0]);
      return;
      break;
   case VG_PAINT_COLOR: {
      if (count != 4)
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      else {
         struct vg_paint *paint = handle_to_paint(object);
         paint_set_color(paint, values);
         if (ctx->state.vg.fill_paint == paint ||
             ctx->state.vg.stroke_paint == paint)
            ctx->state.dirty |= PAINT_DIRTY;
      }
   }
      break;
   case VG_PAINT_COLOR_RAMP_STOPS: {
      if (count && count < 4)
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      else {
         struct vg_paint *paint = handle_to_paint(object);
         count = MIN2(count, VEGA_MAX_COLOR_RAMP_STOPS);
         paint_set_ramp_stops(paint, values, count);
         {
            VGint stopsi[VEGA_MAX_COLOR_RAMP_STOPS];
            int i = 0;
            for (i = 0; i < count; ++i) {
               stopsi[i] = float_to_int_floor(*((VGuint*)(values + i)));
            }
            paint_set_ramp_stopsi(paint, stopsi, count);
         }
      }
   }
      break;
   case VG_PAINT_LINEAR_GRADIENT: {
      if (count != 4)
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      else {
         struct vg_paint *paint = handle_to_paint(object);
         paint_set_linear_gradient(paint, values);
         {
            VGint vals[4];
            vals[0] = FLT_TO_INT(values[0]);
            vals[1] = FLT_TO_INT(values[1]);
            vals[2] = FLT_TO_INT(values[2]);
            vals[3] = FLT_TO_INT(values[3]);
            paint_set_linear_gradienti(paint, vals);
         }
      }
   }
      break;
   case VG_PAINT_RADIAL_GRADIENT: {
      if (count != 5)
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      else {
         struct vg_paint *paint = handle_to_paint(object);
         paint_set_radial_gradient(paint, values);
         {
            VGint vals[5];
            vals[0] = FLT_TO_INT(values[0]);
            vals[1] = FLT_TO_INT(values[1]);
            vals[2] = FLT_TO_INT(values[2]);
            vals[3] = FLT_TO_INT(values[3]);
            vals[4] = FLT_TO_INT(values[4]);
            paint_set_radial_gradienti(paint, vals);
         }
      }
   }
      break;

   case VG_PATH_DATATYPE:
   case VG_PATH_FORMAT:
   case VG_PATH_SCALE:
   case VG_PATH_BIAS:
   case VG_PATH_NUM_SEGMENTS:
   case VG_PATH_NUM_COORDS:

#ifdef OPENVG_VERSION_1_1
   case VG_FONT_NUM_GLYPHS:
      /* read only don't produce an error */
      break;
#endif
   default:
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
}

void vegaSetParameteriv(VGHandle object,
                        VGint paramType,
                        VGint count,
                        const VGint * values)
{
   struct vg_context *ctx = vg_current_context();
   void *ptr = handle_to_pointer(object);
   VGint real_count = vegaGetParameterVectorSize(object, paramType);

   if (object == VG_INVALID_HANDLE || !is_aligned(ptr)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (count < 0 || count < real_count ||
       (values == NULL && count != 0) ||
       !is_aligned(values)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   switch(paramType) {
   case VG_PAINT_TYPE:
   case VG_PAINT_COLOR_RAMP_SPREAD_MODE:
   case VG_PAINT_COLOR_RAMP_PREMULTIPLIED:
   case VG_PAINT_PATTERN_TILING_MODE:
      if (count != 1)
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      else
         vegaSetParameteri(object, paramType, values[0]);
      return;
      break;
   case VG_PAINT_COLOR: {
      if (count != 4)
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      else {
         struct vg_paint *paint = handle_to_paint(object);
         paint_set_coloriv(paint, values);
         if (ctx->state.vg.fill_paint == paint ||
             ctx->state.vg.stroke_paint == paint)
            ctx->state.dirty |= PAINT_DIRTY;
      }
   }
      break;
   case VG_PAINT_COLOR_RAMP_STOPS: {
      if ((count % 5))
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      else {
         VGfloat *vals = 0;
         int i;
         struct vg_paint *paint = handle_to_paint(object);
         if (count) {
            vals = malloc(sizeof(VGfloat)*count);
            for (i = 0; i < count; ++i)
               vals[i] = values[i];
         }

         paint_set_ramp_stopsi(paint, values, count);
         paint_set_ramp_stops(paint, vals, count);
         free(vals);
      }
   }
      break;
   case VG_PAINT_LINEAR_GRADIENT: {
      if (count != 4)
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      else {
         VGfloat vals[4];
         struct vg_paint *paint = handle_to_paint(object);
         vals[0] = values[0];
         vals[1] = values[1];
         vals[2] = values[2];
         vals[3] = values[3];
         paint_set_linear_gradient(paint, vals);
         paint_set_linear_gradienti(paint, values);
      }
   }
      break;
   case VG_PAINT_RADIAL_GRADIENT: {
      if (count != 5)
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      else {
         VGfloat vals[5];
         struct vg_paint *paint = handle_to_paint(object);
         vals[0] = values[0];
         vals[1] = values[1];
         vals[2] = values[2];
         vals[3] = values[3];
         vals[4] = values[4];
         paint_set_radial_gradient(paint, vals);
         paint_set_radial_gradienti(paint, values);
      }
   }
      break;
   case VG_PATH_DATATYPE:
   case VG_PATH_FORMAT:
   case VG_PATH_SCALE:
   case VG_PATH_BIAS:
   case VG_PATH_NUM_SEGMENTS:
   case VG_PATH_NUM_COORDS:
      /* read only don't produce an error */
      break;
   default:
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
}

VGint vegaGetParameterVectorSize(VGHandle object,
                                 VGint paramType)
{
   struct vg_context *ctx = vg_current_context();

   if (object == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return 0;
   }

   switch(paramType) {
   case VG_PAINT_TYPE:
   case VG_PAINT_COLOR_RAMP_SPREAD_MODE:
   case VG_PAINT_COLOR_RAMP_PREMULTIPLIED:
   case VG_PAINT_PATTERN_TILING_MODE:
      return 1;
   case VG_PAINT_COLOR:
      return 4;
   case VG_PAINT_COLOR_RAMP_STOPS: {
      struct vg_paint *p = handle_to_paint(object);
      return paint_num_ramp_stops(p);
   }
      break;
   case VG_PAINT_LINEAR_GRADIENT:
      return 4;
   case VG_PAINT_RADIAL_GRADIENT:
      return 5;


   case VG_PATH_FORMAT:
   case VG_PATH_DATATYPE:
   case VG_PATH_SCALE:
   case VG_PATH_BIAS:
   case VG_PATH_NUM_SEGMENTS:
   case VG_PATH_NUM_COORDS:
      return 1;

   case VG_IMAGE_FORMAT:
   case VG_IMAGE_WIDTH:
   case VG_IMAGE_HEIGHT:
      return 1;

#ifdef OPENVG_VERSION_1_1
   case VG_FONT_NUM_GLYPHS:
      return 1;
#endif

   default:
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      break;
   }
   return 0;
}


VGfloat vegaGetParameterf(VGHandle object,
                          VGint paramType)
{
   struct vg_context *ctx = vg_current_context();

   if (object == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return 0;
   }

   switch(paramType) {
   case VG_PAINT_TYPE:
   case VG_PAINT_COLOR_RAMP_SPREAD_MODE:
   case VG_PAINT_COLOR_RAMP_PREMULTIPLIED:
   case VG_PAINT_PATTERN_TILING_MODE:
      return vegaGetParameteri(object, paramType);
      break;
   case VG_PAINT_COLOR:
   case VG_PAINT_COLOR_RAMP_STOPS:
   case VG_PAINT_LINEAR_GRADIENT:
   case VG_PAINT_RADIAL_GRADIENT:
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      break;

   case VG_PATH_FORMAT:
      return VG_PATH_FORMAT_STANDARD;
   case VG_PATH_SCALE: {
      struct path *p = handle_to_path(object);
      return path_scale(p);
   }
   case VG_PATH_BIAS: {
      struct path *p = handle_to_path(object);
      return path_bias(p);
   }
   case VG_PATH_DATATYPE:
   case VG_PATH_NUM_SEGMENTS:
   case VG_PATH_NUM_COORDS:
      return vegaGetParameteri(object, paramType);
      break;

   case VG_IMAGE_FORMAT:
   case VG_IMAGE_WIDTH:
   case VG_IMAGE_HEIGHT:
#ifdef OPENVG_VERSION_1_1
   case VG_FONT_NUM_GLYPHS: 
      return vegaGetParameteri(object, paramType);
      break;
#endif

   default:
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      break;
   }
   return 0;
}

VGint vegaGetParameteri(VGHandle object,
                        VGint paramType)
{
   struct vg_context *ctx = vg_current_context();

   if (object == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return 0;
   }

   switch(paramType) {
   case VG_PAINT_TYPE: {
         struct vg_paint *paint = handle_to_paint(object);
         return paint_type(paint);
   }
      break;
   case VG_PAINT_COLOR_RAMP_SPREAD_MODE: {
      struct vg_paint *p = handle_to_paint(object);
      return paint_spread_mode(p);
   }
   case VG_PAINT_COLOR_RAMP_PREMULTIPLIED: {
      struct vg_paint *p = handle_to_paint(object);
      return paint_color_ramp_premultiplied(p);
   }
      break;
   case VG_PAINT_PATTERN_TILING_MODE: {
      struct vg_paint *p = handle_to_paint(object);
      return paint_pattern_tiling(p);
   }
      break;
   case VG_PAINT_COLOR:
   case VG_PAINT_COLOR_RAMP_STOPS:
   case VG_PAINT_LINEAR_GRADIENT:
   case VG_PAINT_RADIAL_GRADIENT:
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      break;

   case VG_PATH_FORMAT:
      return VG_PATH_FORMAT_STANDARD;
   case VG_PATH_SCALE:
   case VG_PATH_BIAS:
      return vegaGetParameterf(object, paramType);
   case VG_PATH_DATATYPE: {
      struct path *p = handle_to_path(object);
      return path_datatype(p);
   }
   case VG_PATH_NUM_SEGMENTS: {
      struct path *p = handle_to_path(object);
      return path_num_segments(p);
   }
   case VG_PATH_NUM_COORDS: {
      struct path *p = handle_to_path(object);
      return path_num_coords(p);
   }
      break;

   case VG_IMAGE_FORMAT: {
      struct vg_image *img = handle_to_image(object);
      return img->format;
   }
      break;
   case VG_IMAGE_WIDTH: {
      struct vg_image *img = handle_to_image(object);
      return img->width;
   }
      break;
   case VG_IMAGE_HEIGHT: {
      struct vg_image *img = handle_to_image(object);
      return img->height;
   }
      break;

#ifdef OPENVG_VERSION_1_1
   case VG_FONT_NUM_GLYPHS: {
      struct vg_font *font = handle_to_font(object);
      return font_num_glyphs(font);
   }
      break;
#endif

   default:
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      break;
   }
   return 0;
}

void vegaGetParameterfv(VGHandle object,
                        VGint paramType,
                        VGint count,
                        VGfloat * values)
{
   struct vg_context *ctx = vg_current_context();
   VGint real_count = vegaGetParameterVectorSize(object, paramType);

   if (object == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (!values || count <= 0 || count > real_count ||
       !is_aligned(values)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   switch(paramType) {
   case VG_PAINT_TYPE: {
      struct vg_paint *p = handle_to_paint(object);
      values[0] = paint_type(p);
   }
      break;
   case VG_PAINT_COLOR_RAMP_SPREAD_MODE: {
      struct vg_paint *p = handle_to_paint(object);
      values[0] = paint_spread_mode(p);
   }
      break;
   case VG_PAINT_COLOR_RAMP_PREMULTIPLIED: {
      struct vg_paint *p = handle_to_paint(object);
      values[0] = paint_color_ramp_premultiplied(p);
   }
      break;
   case VG_PAINT_PATTERN_TILING_MODE: {
      values[0] = vegaGetParameterf(object, paramType);
   }
      break;
   case VG_PAINT_COLOR: {
      struct vg_paint *paint = handle_to_paint(object);
      paint_get_color(paint, values);
   }
      break;
   case VG_PAINT_COLOR_RAMP_STOPS: {
      struct vg_paint *paint = handle_to_paint(object);
      paint_ramp_stops(paint, values, count);
   }
      break;
   case VG_PAINT_LINEAR_GRADIENT: {
      struct vg_paint *paint = handle_to_paint(object);
      paint_linear_gradient(paint, values);
   }
      break;
   case VG_PAINT_RADIAL_GRADIENT: {
      struct vg_paint *paint = handle_to_paint(object);
      paint_radial_gradient(paint, values);
   }
      break;

   case VG_PATH_FORMAT:
   case VG_PATH_DATATYPE:
   case VG_PATH_NUM_SEGMENTS:
   case VG_PATH_NUM_COORDS:
      values[0] = vegaGetParameteri(object, paramType);
      break;
   case VG_PATH_SCALE:
   case VG_PATH_BIAS:
      values[0] = vegaGetParameterf(object, paramType);
      break;

   case VG_IMAGE_FORMAT:
   case VG_IMAGE_WIDTH:
   case VG_IMAGE_HEIGHT:
#ifdef OPENVG_VERSION_1_1
   case VG_FONT_NUM_GLYPHS:
      values[0] = vegaGetParameteri(object, paramType);
      break;
#endif

   default:
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      break;
   }
}

void vegaGetParameteriv(VGHandle object,
                        VGint paramType,
                        VGint count,
                        VGint * values)
{
   struct vg_context *ctx = vg_current_context();
   VGint real_count = vegaGetParameterVectorSize(object, paramType);

   if (object || object == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (!values || count <= 0 || count > real_count ||
       !is_aligned(values)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   switch(paramType) {
   case VG_PAINT_TYPE:
   case VG_PAINT_COLOR_RAMP_SPREAD_MODE:
   case VG_PAINT_COLOR_RAMP_PREMULTIPLIED:
   case VG_PAINT_PATTERN_TILING_MODE:
#ifdef OPENVG_VERSION_1_1
   case VG_FONT_NUM_GLYPHS:
      values[0] = vegaGetParameteri(object, paramType);
      break;
#endif
   case VG_PAINT_COLOR: {
      struct vg_paint *paint = handle_to_paint(object);
      paint_get_coloriv(paint, values);
   }
      break;
   case VG_PAINT_COLOR_RAMP_STOPS: {
      struct vg_paint *paint = handle_to_paint(object);
      paint_ramp_stopsi(paint, values, count);
   }
      break;
   case VG_PAINT_LINEAR_GRADIENT: {
      struct vg_paint *paint = handle_to_paint(object);
      paint_linear_gradienti(paint, values);
   }
      break;
   case VG_PAINT_RADIAL_GRADIENT: {
      struct vg_paint *paint = handle_to_paint(object);
      paint_radial_gradienti(paint, values);
   }
      break;

   case VG_PATH_SCALE:
   case VG_PATH_BIAS:
      values[0] = vegaGetParameterf(object, paramType);
      break;
   case VG_PATH_FORMAT:
   case VG_PATH_DATATYPE:
   case VG_PATH_NUM_SEGMENTS:
   case VG_PATH_NUM_COORDS:
      values[0] = vegaGetParameteri(object, paramType);
      break;

   case VG_IMAGE_FORMAT:
   case VG_IMAGE_WIDTH:
   case VG_IMAGE_HEIGHT:
      values[0] = vegaGetParameteri(object, paramType);
      break;

   default:
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      break;
   }
}
