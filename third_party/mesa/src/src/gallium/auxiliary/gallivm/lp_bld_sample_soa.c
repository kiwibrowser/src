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

/**
 * @file
 * Texture sampling -- SoA.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 * @author Brian Paul <brianp@vmware.com>
 */

#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_debug.h"
#include "util/u_dump.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "util/u_cpu_detect.h"
#include "lp_bld_debug.h"
#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_conv.h"
#include "lp_bld_arit.h"
#include "lp_bld_bitarit.h"
#include "lp_bld_logic.h"
#include "lp_bld_printf.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_flow.h"
#include "lp_bld_gather.h"
#include "lp_bld_format.h"
#include "lp_bld_sample.h"
#include "lp_bld_sample_aos.h"
#include "lp_bld_struct.h"
#include "lp_bld_quad.h"
#include "lp_bld_pack.h"


/**
 * Generate code to fetch a texel from a texture at int coords (x, y, z).
 * The computation depends on whether the texture is 1D, 2D or 3D.
 * The result, texel, will be float vectors:
 *   texel[0] = red values
 *   texel[1] = green values
 *   texel[2] = blue values
 *   texel[3] = alpha values
 */
static void
lp_build_sample_texel_soa(struct lp_build_sample_context *bld,
                          unsigned unit,
                          LLVMValueRef width,
                          LLVMValueRef height,
                          LLVMValueRef depth,
                          LLVMValueRef x,
                          LLVMValueRef y,
                          LLVMValueRef z,
                          LLVMValueRef y_stride,
                          LLVMValueRef z_stride,
                          LLVMValueRef data_ptr,
                          LLVMValueRef texel_out[4])
{
   const struct lp_sampler_static_state *static_state = bld->static_state;
   const unsigned dims = bld->dims;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef offset;
   LLVMValueRef i, j;
   LLVMValueRef use_border = NULL;

   /* use_border = x < 0 || x >= width || y < 0 || y >= height */
   if (lp_sampler_wrap_mode_uses_border_color(static_state->wrap_s,
                                              static_state->min_img_filter,
                                              static_state->mag_img_filter)) {
      LLVMValueRef b1, b2;
      b1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, x, int_coord_bld->zero);
      b2 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, x, width);
      use_border = LLVMBuildOr(builder, b1, b2, "b1_or_b2");
   }

   if (dims >= 2 &&
       lp_sampler_wrap_mode_uses_border_color(static_state->wrap_t,
                                              static_state->min_img_filter,
                                              static_state->mag_img_filter)) {
      LLVMValueRef b1, b2;
      b1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, y, int_coord_bld->zero);
      b2 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, y, height);
      if (use_border) {
         use_border = LLVMBuildOr(builder, use_border, b1, "ub_or_b1");
         use_border = LLVMBuildOr(builder, use_border, b2, "ub_or_b2");
      }
      else {
         use_border = LLVMBuildOr(builder, b1, b2, "b1_or_b2");
      }
   }

   if (dims == 3 &&
       lp_sampler_wrap_mode_uses_border_color(static_state->wrap_r,
                                              static_state->min_img_filter,
                                              static_state->mag_img_filter)) {
      LLVMValueRef b1, b2;
      b1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, z, int_coord_bld->zero);
      b2 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, z, depth);
      if (use_border) {
         use_border = LLVMBuildOr(builder, use_border, b1, "ub_or_b1");
         use_border = LLVMBuildOr(builder, use_border, b2, "ub_or_b2");
      }
      else {
         use_border = LLVMBuildOr(builder, b1, b2, "b1_or_b2");
      }
   }

   /* convert x,y,z coords to linear offset from start of texture, in bytes */
   lp_build_sample_offset(&bld->int_coord_bld,
                          bld->format_desc,
                          x, y, z, y_stride, z_stride,
                          &offset, &i, &j);

   if (use_border) {
      /* If we can sample the border color, it means that texcoords may
       * lie outside the bounds of the texture image.  We need to do
       * something to prevent reading out of bounds and causing a segfault.
       *
       * Simply AND the texture coords with !use_border.  This will cause
       * coords which are out of bounds to become zero.  Zero's guaranteed
       * to be inside the texture image.
       */
      offset = lp_build_andnot(&bld->int_coord_bld, offset, use_border);
   }

   lp_build_fetch_rgba_soa(bld->gallivm,
                           bld->format_desc,
                           bld->texel_type,
                           data_ptr, offset,
                           i, j,
                           texel_out);

   /*
    * Note: if we find an app which frequently samples the texture border
    * we might want to implement a true conditional here to avoid sampling
    * the texture whenever possible (since that's quite a bit of code).
    * Ex:
    *   if (use_border) {
    *      texel = border_color;
    *   }
    *   else {
    *      texel = sample_texture(coord);
    *   }
    * As it is now, we always sample the texture, then selectively replace
    * the texel color results with the border color.
    */

   if (use_border) {
      /* select texel color or border color depending on use_border */
      LLVMValueRef border_color_ptr = 
         bld->dynamic_state->border_color(bld->dynamic_state,
                                          bld->gallivm, unit);
      int chan;
      for (chan = 0; chan < 4; chan++) {
         LLVMValueRef border_chan =
            lp_build_array_get(bld->gallivm, border_color_ptr,
                               lp_build_const_int32(bld->gallivm, chan));
         LLVMValueRef border_chan_vec =
            lp_build_broadcast_scalar(&bld->float_vec_bld, border_chan);
         texel_out[chan] = lp_build_select(&bld->texel_bld, use_border,
                                           border_chan_vec, texel_out[chan]);
      }
   }
}


/**
 * Helper to compute the mirror function for the PIPE_WRAP_MIRROR modes.
 */
static LLVMValueRef
lp_build_coord_mirror(struct lp_build_sample_context *bld,
                      LLVMValueRef coord)
{
   struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMValueRef fract, flr, isOdd;

   lp_build_ifloor_fract(coord_bld, coord, &flr, &fract);

   /* isOdd = flr & 1 */
   isOdd = LLVMBuildAnd(bld->gallivm->builder, flr, int_coord_bld->one, "");

   /* make coord positive or negative depending on isOdd */
   coord = lp_build_set_sign(coord_bld, fract, isOdd);

   /* convert isOdd to float */
   isOdd = lp_build_int_to_float(coord_bld, isOdd);

   /* add isOdd to coord */
   coord = lp_build_add(coord_bld, coord, isOdd);

   return coord;
}


/**
 * Helper to compute the first coord and the weight for
 * linear wrap repeat npot textures
 */
void
lp_build_coord_repeat_npot_linear(struct lp_build_sample_context *bld,
                                  LLVMValueRef coord_f,
                                  LLVMValueRef length_i,
                                  LLVMValueRef length_f,
                                  LLVMValueRef *coord0_i,
                                  LLVMValueRef *weight_f)
{
   struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMValueRef half = lp_build_const_vec(bld->gallivm, coord_bld->type, 0.5);
   LLVMValueRef length_minus_one = lp_build_sub(int_coord_bld, length_i,
                                                int_coord_bld->one);
   LLVMValueRef mask;
   /* wrap with normalized floats is just fract */
   coord_f = lp_build_fract(coord_bld, coord_f);
   /* mul by size and subtract 0.5 */
   coord_f = lp_build_mul(coord_bld, coord_f, length_f);
   coord_f = lp_build_sub(coord_bld, coord_f, half);
   /*
    * we avoided the 0.5/length division before the repeat wrap,
    * now need to fix up edge cases with selects
    */
   /* convert to int, compute lerp weight */
   lp_build_ifloor_fract(coord_bld, coord_f, coord0_i, weight_f);
   mask = lp_build_compare(int_coord_bld->gallivm, int_coord_bld->type,
                           PIPE_FUNC_LESS, *coord0_i, int_coord_bld->zero);
   *coord0_i = lp_build_select(int_coord_bld, mask, length_minus_one, *coord0_i);
}


/**
 * Build LLVM code for texture wrap mode for linear filtering.
 * \param x0_out  returns first integer texcoord
 * \param x1_out  returns second integer texcoord
 * \param weight_out  returns linear interpolation weight
 */
static void
lp_build_sample_wrap_linear(struct lp_build_sample_context *bld,
                            LLVMValueRef coord,
                            LLVMValueRef length,
                            LLVMValueRef length_f,
                            boolean is_pot,
                            unsigned wrap_mode,
                            LLVMValueRef *x0_out,
                            LLVMValueRef *x1_out,
                            LLVMValueRef *weight_out)
{
   struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef half = lp_build_const_vec(bld->gallivm, coord_bld->type, 0.5);
   LLVMValueRef length_minus_one = lp_build_sub(int_coord_bld, length, int_coord_bld->one);
   LLVMValueRef coord0, coord1, weight;

   switch(wrap_mode) {
   case PIPE_TEX_WRAP_REPEAT:
      if (is_pot) {
         /* mul by size and subtract 0.5 */
         coord = lp_build_mul(coord_bld, coord, length_f);
         coord = lp_build_sub(coord_bld, coord, half);
         /* convert to int, compute lerp weight */
         lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
         coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
         /* repeat wrap */
         coord0 = LLVMBuildAnd(builder, coord0, length_minus_one, "");
         coord1 = LLVMBuildAnd(builder, coord1, length_minus_one, "");
      }
      else {
         LLVMValueRef mask;
         lp_build_coord_repeat_npot_linear(bld, coord,
                                           length, length_f,
                                           &coord0, &weight);
         mask = lp_build_compare(int_coord_bld->gallivm, int_coord_bld->type,
                                 PIPE_FUNC_NOTEQUAL, coord0, length_minus_one);
         coord1 = LLVMBuildAnd(builder,
                               lp_build_add(int_coord_bld, coord0, int_coord_bld->one),
                               mask, "");
      }
      break;

   case PIPE_TEX_WRAP_CLAMP:
      if (bld->static_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }

      /* clamp to [0, length] */
      coord = lp_build_clamp(coord_bld, coord, coord_bld->zero, length_f);

      coord = lp_build_sub(coord_bld, coord, half);

      /* convert to int, compute lerp weight */
      lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
      coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
      break;

   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      {
         struct lp_build_context abs_coord_bld = bld->coord_bld;
         abs_coord_bld.type.sign = FALSE;

         if (bld->static_state->normalized_coords) {
            /* mul by tex size */
            coord = lp_build_mul(coord_bld, coord, length_f);
         }
         /* clamp to length max */
         coord = lp_build_min(coord_bld, coord, length_f);
         /* subtract 0.5 */
         coord = lp_build_sub(coord_bld, coord, half);
         /* clamp to [0, length - 0.5] */
         coord = lp_build_max(coord_bld, coord, coord_bld->zero);
         /* convert to int, compute lerp weight */
         lp_build_ifloor_fract(&abs_coord_bld, coord, &coord0, &weight);
         coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
         /* coord1 = min(coord1, length-1) */
         coord1 = lp_build_min(int_coord_bld, coord1, length_minus_one);
         break;
      }

   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      {
         LLVMValueRef min;
         if (bld->static_state->normalized_coords) {
            /* scale coord to length */
            coord = lp_build_mul(coord_bld, coord, length_f);
         }
         /* was: clamp to [-0.5, length + 0.5], then sub 0.5 */
         coord = lp_build_sub(coord_bld, coord, half);
         min = lp_build_const_vec(bld->gallivm, coord_bld->type, -1.0F);
         coord = lp_build_clamp(coord_bld, coord, min, length_f);
         /* convert to int, compute lerp weight */
         lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
         coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
      }
      break;

   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      /* compute mirror function */
      coord = lp_build_coord_mirror(bld, coord);

      /* scale coord to length */
      coord = lp_build_mul(coord_bld, coord, length_f);
      coord = lp_build_sub(coord_bld, coord, half);

      /* convert to int, compute lerp weight */
      lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
      coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);

      /* coord0 = max(coord0, 0) */
      coord0 = lp_build_max(int_coord_bld, coord0, int_coord_bld->zero);
      /* coord1 = min(coord1, length-1) */
      coord1 = lp_build_min(int_coord_bld, coord1, length_minus_one);
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP:
      coord = lp_build_abs(coord_bld, coord);

      if (bld->static_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }

      /* clamp to [0, length] */
      coord = lp_build_min(coord_bld, coord, length_f);

      coord = lp_build_sub(coord_bld, coord, half);

      /* convert to int, compute lerp weight */
      lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
      coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      {
         LLVMValueRef min, max;
         struct lp_build_context abs_coord_bld = bld->coord_bld;
         abs_coord_bld.type.sign = FALSE;
         coord = lp_build_abs(coord_bld, coord);

         if (bld->static_state->normalized_coords) {
            /* scale coord to length */
            coord = lp_build_mul(coord_bld, coord, length_f);
         }

         /* clamp to [0.5, length - 0.5] */
         min = half;
         max = lp_build_sub(coord_bld, length_f, min);
         coord = lp_build_clamp(coord_bld, coord, min, max);

         coord = lp_build_sub(coord_bld, coord, half);

         /* convert to int, compute lerp weight */
         lp_build_ifloor_fract(&abs_coord_bld, coord, &coord0, &weight);
         coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
      }
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      {
         coord = lp_build_abs(coord_bld, coord);

         if (bld->static_state->normalized_coords) {
            /* scale coord to length */
            coord = lp_build_mul(coord_bld, coord, length_f);
         }

         /* was: clamp to [-0.5, length + 0.5] then sub 0.5 */
         /* skip -0.5 clamp (always positive), do sub first */
         coord = lp_build_sub(coord_bld, coord, half);
         coord = lp_build_min(coord_bld, coord, length_f);

         /* convert to int, compute lerp weight */
         lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
         coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
      }
      break;

   default:
      assert(0);
      coord0 = NULL;
      coord1 = NULL;
      weight = NULL;
   }

   *x0_out = coord0;
   *x1_out = coord1;
   *weight_out = weight;
}


/**
 * Build LLVM code for texture wrap mode for nearest filtering.
 * \param coord  the incoming texcoord (nominally in [0,1])
 * \param length  the texture size along one dimension, as int vector
 * \param is_pot  if TRUE, length is a power of two
 * \param wrap_mode  one of PIPE_TEX_WRAP_x
 */
static LLVMValueRef
lp_build_sample_wrap_nearest(struct lp_build_sample_context *bld,
                             LLVMValueRef coord,
                             LLVMValueRef length,
                             LLVMValueRef length_f,
                             boolean is_pot,
                             unsigned wrap_mode)
{
   struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef length_minus_one = lp_build_sub(int_coord_bld, length, int_coord_bld->one);
   LLVMValueRef icoord;
   
   switch(wrap_mode) {
   case PIPE_TEX_WRAP_REPEAT:
      if (is_pot) {
         coord = lp_build_mul(coord_bld, coord, length_f);
         icoord = lp_build_ifloor(coord_bld, coord);
         icoord = LLVMBuildAnd(builder, icoord, length_minus_one, "");
      }
      else {
          /* take fraction, unnormalize */
          coord = lp_build_fract_safe(coord_bld, coord);
          coord = lp_build_mul(coord_bld, coord, length_f);
          icoord = lp_build_itrunc(coord_bld, coord);
      }
      break;

   case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      if (bld->static_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }

      /* floor */
      /* use itrunc instead since we clamp to 0 anyway */
      icoord = lp_build_itrunc(coord_bld, coord);

      /* clamp to [0, length - 1]. */
      icoord = lp_build_clamp(int_coord_bld, icoord, int_coord_bld->zero,
                              length_minus_one);
      break;

   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      /* Note: this is the same as CLAMP_TO_EDGE, except min = -1 */
      {
         LLVMValueRef min, max;

         if (bld->static_state->normalized_coords) {
            /* scale coord to length */
            coord = lp_build_mul(coord_bld, coord, length_f);
         }

         icoord = lp_build_ifloor(coord_bld, coord);

         /* clamp to [-1, length] */
         min = lp_build_negate(int_coord_bld, int_coord_bld->one);
         max = length;
         icoord = lp_build_clamp(int_coord_bld, icoord, min, max);
      }
      break;

   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      /* compute mirror function */
      coord = lp_build_coord_mirror(bld, coord);

      /* scale coord to length */
      assert(bld->static_state->normalized_coords);
      coord = lp_build_mul(coord_bld, coord, length_f);

      /* itrunc == ifloor here */
      icoord = lp_build_itrunc(coord_bld, coord);

      /* clamp to [0, length - 1] */
      icoord = lp_build_min(int_coord_bld, icoord, length_minus_one);
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      coord = lp_build_abs(coord_bld, coord);

      if (bld->static_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }

      /* itrunc == ifloor here */
      icoord = lp_build_itrunc(coord_bld, coord);

      /* clamp to [0, length - 1] */
      icoord = lp_build_min(int_coord_bld, icoord, length_minus_one);
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      coord = lp_build_abs(coord_bld, coord);

      if (bld->static_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }

      /* itrunc == ifloor here */
      icoord = lp_build_itrunc(coord_bld, coord);

      /* clamp to [0, length] */
      icoord = lp_build_min(int_coord_bld, icoord, length);
      break;

   default:
      assert(0);
      icoord = NULL;
   }

   return icoord;
}


/**
 * Generate code to sample a mipmap level with nearest filtering.
 * If sampling a cube texture, r = cube face in [0,5].
 */
static void
lp_build_sample_image_nearest(struct lp_build_sample_context *bld,
                              unsigned unit,
                              LLVMValueRef size,
                              LLVMValueRef row_stride_vec,
                              LLVMValueRef img_stride_vec,
                              LLVMValueRef data_ptr,
                              LLVMValueRef s,
                              LLVMValueRef t,
                              LLVMValueRef r,
                              LLVMValueRef colors_out[4])
{
   const unsigned dims = bld->dims;
   LLVMValueRef width_vec;
   LLVMValueRef height_vec;
   LLVMValueRef depth_vec;
   LLVMValueRef flt_size;
   LLVMValueRef flt_width_vec;
   LLVMValueRef flt_height_vec;
   LLVMValueRef flt_depth_vec;
   LLVMValueRef x, y, z;

   lp_build_extract_image_sizes(bld,
                                bld->int_size_type,
                                bld->int_coord_type,
                                size,
                                &width_vec, &height_vec, &depth_vec);

   flt_size = lp_build_int_to_float(&bld->float_size_bld, size);

   lp_build_extract_image_sizes(bld,
                                bld->float_size_type,
                                bld->coord_type,
                                flt_size,
                                &flt_width_vec, &flt_height_vec, &flt_depth_vec);

   /*
    * Compute integer texcoords.
    */
   x = lp_build_sample_wrap_nearest(bld, s, width_vec, flt_width_vec,
                                    bld->static_state->pot_width,
                                    bld->static_state->wrap_s);
   lp_build_name(x, "tex.x.wrapped");

   if (dims >= 2) {
      y = lp_build_sample_wrap_nearest(bld, t, height_vec, flt_height_vec,
                                       bld->static_state->pot_height,
                                       bld->static_state->wrap_t);
      lp_build_name(y, "tex.y.wrapped");

      if (dims == 3) {
         z = lp_build_sample_wrap_nearest(bld, r, depth_vec, flt_depth_vec,
                                          bld->static_state->pot_depth,
                                          bld->static_state->wrap_r);
         lp_build_name(z, "tex.z.wrapped");
      }
      else if (bld->static_state->target == PIPE_TEXTURE_CUBE) {
         z = r;
      }
      else {
         z = NULL;
      }
   }
   else {
      y = z = NULL;
   }

   /*
    * Get texture colors.
    */
   lp_build_sample_texel_soa(bld, unit,
                             width_vec, height_vec, depth_vec,
                             x, y, z,
                             row_stride_vec, img_stride_vec,
                             data_ptr, colors_out);
}


/**
 * Generate code to sample a mipmap level with linear filtering.
 * If sampling a cube texture, r = cube face in [0,5].
 */
static void
lp_build_sample_image_linear(struct lp_build_sample_context *bld,
                             unsigned unit,
                             LLVMValueRef size,
                             LLVMValueRef row_stride_vec,
                             LLVMValueRef img_stride_vec,
                             LLVMValueRef data_ptr,
                             LLVMValueRef s,
                             LLVMValueRef t,
                             LLVMValueRef r,
                             LLVMValueRef colors_out[4])
{
   const unsigned dims = bld->dims;
   LLVMValueRef width_vec;
   LLVMValueRef height_vec;
   LLVMValueRef depth_vec;
   LLVMValueRef flt_size;
   LLVMValueRef flt_width_vec;
   LLVMValueRef flt_height_vec;
   LLVMValueRef flt_depth_vec;
   LLVMValueRef x0, y0, z0, x1, y1, z1;
   LLVMValueRef s_fpart, t_fpart, r_fpart;
   LLVMValueRef neighbors[2][2][4];
   int chan;

   lp_build_extract_image_sizes(bld,
                                bld->int_size_type,
                                bld->int_coord_type,
                                size,
                                &width_vec, &height_vec, &depth_vec);

   flt_size = lp_build_int_to_float(&bld->float_size_bld, size);

   lp_build_extract_image_sizes(bld,
                                bld->float_size_type,
                                bld->coord_type,
                                flt_size,
                                &flt_width_vec, &flt_height_vec, &flt_depth_vec);

   /*
    * Compute integer texcoords.
    */
   lp_build_sample_wrap_linear(bld, s, width_vec, flt_width_vec,
                               bld->static_state->pot_width,
                               bld->static_state->wrap_s,
                               &x0, &x1, &s_fpart);
   lp_build_name(x0, "tex.x0.wrapped");
   lp_build_name(x1, "tex.x1.wrapped");

   if (dims >= 2) {
      lp_build_sample_wrap_linear(bld, t, height_vec, flt_height_vec,
                                  bld->static_state->pot_height,
                                  bld->static_state->wrap_t,
                                  &y0, &y1, &t_fpart);
      lp_build_name(y0, "tex.y0.wrapped");
      lp_build_name(y1, "tex.y1.wrapped");

      if (dims == 3) {
         lp_build_sample_wrap_linear(bld, r, depth_vec, flt_depth_vec,
                                     bld->static_state->pot_depth,
                                     bld->static_state->wrap_r,
                                     &z0, &z1, &r_fpart);
         lp_build_name(z0, "tex.z0.wrapped");
         lp_build_name(z1, "tex.z1.wrapped");
      }
      else if (bld->static_state->target == PIPE_TEXTURE_CUBE) {
         z0 = z1 = r;  /* cube face */
         r_fpart = NULL;
      }
      else {
         z0 = z1 = NULL;
         r_fpart = NULL;
      }
   }
   else {
      y0 = y1 = t_fpart = NULL;
      z0 = z1 = r_fpart = NULL;
   }

   /*
    * Get texture colors.
    */
   /* get x0/x1 texels */
   lp_build_sample_texel_soa(bld, unit,
                             width_vec, height_vec, depth_vec,
                             x0, y0, z0,
                             row_stride_vec, img_stride_vec,
                             data_ptr, neighbors[0][0]);
   lp_build_sample_texel_soa(bld, unit,
                             width_vec, height_vec, depth_vec,
                             x1, y0, z0,
                             row_stride_vec, img_stride_vec,
                             data_ptr, neighbors[0][1]);

   if (dims == 1) {
      /* Interpolate two samples from 1D image to produce one color */
      for (chan = 0; chan < 4; chan++) {
         colors_out[chan] = lp_build_lerp(&bld->texel_bld, s_fpart,
                                          neighbors[0][0][chan],
                                          neighbors[0][1][chan]);
      }
   }
   else {
      /* 2D/3D texture */
      LLVMValueRef colors0[4];

      /* get x0/x1 texels at y1 */
      lp_build_sample_texel_soa(bld, unit,
                                width_vec, height_vec, depth_vec,
                                x0, y1, z0,
                                row_stride_vec, img_stride_vec,
                                data_ptr, neighbors[1][0]);
      lp_build_sample_texel_soa(bld, unit,
                                width_vec, height_vec, depth_vec,
                                x1, y1, z0,
                                row_stride_vec, img_stride_vec,
                                data_ptr, neighbors[1][1]);

      /* Bilinear interpolate the four samples from the 2D image / 3D slice */
      for (chan = 0; chan < 4; chan++) {
         colors0[chan] = lp_build_lerp_2d(&bld->texel_bld,
                                          s_fpart, t_fpart,
                                          neighbors[0][0][chan],
                                          neighbors[0][1][chan],
                                          neighbors[1][0][chan],
                                          neighbors[1][1][chan]);
      }

      if (dims == 3) {
         LLVMValueRef neighbors1[2][2][4];
         LLVMValueRef colors1[4];

         /* get x0/x1/y0/y1 texels at z1 */
         lp_build_sample_texel_soa(bld, unit,
                                   width_vec, height_vec, depth_vec,
                                   x0, y0, z1,
                                   row_stride_vec, img_stride_vec,
                                   data_ptr, neighbors1[0][0]);
         lp_build_sample_texel_soa(bld, unit,
                                   width_vec, height_vec, depth_vec,
                                   x1, y0, z1,
                                   row_stride_vec, img_stride_vec,
                                   data_ptr, neighbors1[0][1]);
         lp_build_sample_texel_soa(bld, unit,
                                   width_vec, height_vec, depth_vec,
                                   x0, y1, z1,
                                   row_stride_vec, img_stride_vec,
                                   data_ptr, neighbors1[1][0]);
         lp_build_sample_texel_soa(bld, unit,
                                   width_vec, height_vec, depth_vec,
                                   x1, y1, z1,
                                   row_stride_vec, img_stride_vec,
                                   data_ptr, neighbors1[1][1]);

         /* Bilinear interpolate the four samples from the second Z slice */
         for (chan = 0; chan < 4; chan++) {
            colors1[chan] = lp_build_lerp_2d(&bld->texel_bld,
                                             s_fpart, t_fpart,
                                             neighbors1[0][0][chan],
                                             neighbors1[0][1][chan],
                                             neighbors1[1][0][chan],
                                             neighbors1[1][1][chan]);
         }

         /* Linearly interpolate the two samples from the two 3D slices */
         for (chan = 0; chan < 4; chan++) {
            colors_out[chan] = lp_build_lerp(&bld->texel_bld,
                                             r_fpart,
                                             colors0[chan], colors1[chan]);
         }
      }
      else {
         /* 2D tex */
         for (chan = 0; chan < 4; chan++) {
            colors_out[chan] = colors0[chan];
         }
      }
   }
}


/**
 * Sample the texture/mipmap using given image filter and mip filter.
 * data0_ptr and data1_ptr point to the two mipmap levels to sample
 * from.  width0/1_vec, height0/1_vec, depth0/1_vec indicate their sizes.
 * If we're using nearest miplevel sampling the '1' values will be null/unused.
 */
static void
lp_build_sample_mipmap(struct lp_build_sample_context *bld,
                       unsigned unit,
                       unsigned img_filter,
                       unsigned mip_filter,
                       LLVMValueRef s,
                       LLVMValueRef t,
                       LLVMValueRef r,
                       LLVMValueRef ilevel0,
                       LLVMValueRef ilevel1,
                       LLVMValueRef lod_fpart,
                       LLVMValueRef *colors_out)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef size0 = NULL;
   LLVMValueRef size1 = NULL;
   LLVMValueRef row_stride0_vec = NULL;
   LLVMValueRef row_stride1_vec = NULL;
   LLVMValueRef img_stride0_vec = NULL;
   LLVMValueRef img_stride1_vec = NULL;
   LLVMValueRef data_ptr0 = NULL;
   LLVMValueRef data_ptr1 = NULL;
   LLVMValueRef colors0[4], colors1[4];
   unsigned chan;

   /* sample the first mipmap level */
   lp_build_mipmap_level_sizes(bld, ilevel0,
                               &size0,
                               &row_stride0_vec, &img_stride0_vec);
   data_ptr0 = lp_build_get_mipmap_level(bld, ilevel0);
   if (img_filter == PIPE_TEX_FILTER_NEAREST) {
      lp_build_sample_image_nearest(bld, unit,
                                    size0,
                                    row_stride0_vec, img_stride0_vec,
                                    data_ptr0, s, t, r,
                                    colors0);
   }
   else {
      assert(img_filter == PIPE_TEX_FILTER_LINEAR);
      lp_build_sample_image_linear(bld, unit,
                                   size0,
                                   row_stride0_vec, img_stride0_vec,
                                   data_ptr0, s, t, r,
                                   colors0);
   }

   /* Store the first level's colors in the output variables */
   for (chan = 0; chan < 4; chan++) {
       LLVMBuildStore(builder, colors0[chan], colors_out[chan]);
   }

   if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
      struct lp_build_if_state if_ctx;
      LLVMValueRef need_lerp;
      unsigned num_quads = bld->coord_bld.type.length / 4;

      /* need_lerp = lod_fpart > 0 */
      if (num_quads == 1) {
         need_lerp = LLVMBuildFCmp(builder, LLVMRealUGT,
                                   lod_fpart, bld->perquadf_bld.zero,
                                   "need_lerp");
      }
      else {
         /*
          * We'll do mip filtering if any of the quads need it.
          * It might be better to split the vectors here and only fetch/filter
          * quads which need it.
          */
         /*
          * We unfortunately need to clamp lod_fpart here since we can get
          * negative values which would screw up filtering if not all
          * lod_fpart values have same sign.
          */
         lod_fpart = lp_build_max(&bld->perquadf_bld, lod_fpart,
                                  bld->perquadf_bld.zero);
         need_lerp = lp_build_compare(bld->gallivm, bld->perquadf_bld.type,
                                      PIPE_FUNC_GREATER,
                                      lod_fpart, bld->perquadf_bld.zero);
         need_lerp = lp_build_any_true_range(&bld->perquadi_bld, num_quads, need_lerp);
     }

      lp_build_if(&if_ctx, bld->gallivm, need_lerp);
      {
         /* sample the second mipmap level */
         lp_build_mipmap_level_sizes(bld, ilevel1,
                                     &size1,
                                     &row_stride1_vec, &img_stride1_vec);
         data_ptr1 = lp_build_get_mipmap_level(bld, ilevel1);
         if (img_filter == PIPE_TEX_FILTER_NEAREST) {
            lp_build_sample_image_nearest(bld, unit,
                                          size1,
                                          row_stride1_vec, img_stride1_vec,
                                          data_ptr1, s, t, r,
                                          colors1);
         }
         else {
            lp_build_sample_image_linear(bld, unit,
                                         size1,
                                         row_stride1_vec, img_stride1_vec,
                                         data_ptr1, s, t, r,
                                         colors1);
         }

         /* interpolate samples from the two mipmap levels */

         lod_fpart = lp_build_unpack_broadcast_aos_scalars(bld->gallivm,
                                                           bld->perquadf_bld.type,
                                                           bld->texel_bld.type,
                                                           lod_fpart);

         for (chan = 0; chan < 4; chan++) {
            colors0[chan] = lp_build_lerp(&bld->texel_bld, lod_fpart,
                                          colors0[chan], colors1[chan]);
            LLVMBuildStore(builder, colors0[chan], colors_out[chan]);
         }
      }
      lp_build_endif(&if_ctx);
   }
}

/**
 * Calculate cube face, lod, mip levels.
 */
static void
lp_build_sample_common(struct lp_build_sample_context *bld,
                       unsigned unit,
                       LLVMValueRef *s,
                       LLVMValueRef *t,
                       LLVMValueRef *r,
                       const struct lp_derivatives *derivs,
                       LLVMValueRef lod_bias, /* optional */
                       LLVMValueRef explicit_lod, /* optional */
                       LLVMValueRef *lod_ipart,
                       LLVMValueRef *lod_fpart,
                       LLVMValueRef *ilevel0,
                       LLVMValueRef *ilevel1)
{
   const unsigned mip_filter = bld->static_state->min_mip_filter;
   const unsigned min_filter = bld->static_state->min_img_filter;
   const unsigned mag_filter = bld->static_state->mag_img_filter;
   LLVMValueRef first_level;
   struct lp_derivatives face_derivs;

   /*
   printf("%s mip %d  min %d  mag %d\n", __FUNCTION__,
          mip_filter, min_filter, mag_filter);
   */

   /*
    * Choose cube face, recompute texcoords and derivatives for the chosen face.
    */
   if (bld->static_state->target == PIPE_TEXTURE_CUBE) {
      LLVMValueRef face, face_s, face_t;
      lp_build_cube_lookup(bld, *s, *t, *r, &face, &face_s, &face_t);
      *s = face_s; /* vec */
      *t = face_t; /* vec */
      /* use 'r' to indicate cube face */
      *r = face; /* vec */

      /* recompute ddx, ddy using the new (s,t) face texcoords */
      face_derivs.ddx_ddy[0] = lp_build_packed_ddx_ddy_twocoord(&bld->coord_bld, *s, *t);
      face_derivs.ddx_ddy[1] = NULL;
      derivs = &face_derivs;
   }

   /*
    * Compute the level of detail (float).
    */
   if (min_filter != mag_filter ||
       mip_filter != PIPE_TEX_MIPFILTER_NONE) {
      /* Need to compute lod either to choose mipmap levels or to
       * distinguish between minification/magnification with one mipmap level.
       */
      lp_build_lod_selector(bld, unit, derivs,
                            lod_bias, explicit_lod,
                            mip_filter,
                            lod_ipart, lod_fpart);
   } else {
      *lod_ipart = bld->perquadi_bld.zero;
   }

   /*
    * Compute integer mipmap level(s) to fetch texels from: ilevel0, ilevel1
    */
   switch (mip_filter) {
   default:
      assert(0 && "bad mip_filter value in lp_build_sample_soa()");
      /* fall-through */
   case PIPE_TEX_MIPFILTER_NONE:
      /* always use mip level 0 */
      if (bld->static_state->target == PIPE_TEXTURE_CUBE) {
         /* XXX this is a work-around for an apparent bug in LLVM 2.7.
          * We should be able to set ilevel0 = const(0) but that causes
          * bad x86 code to be emitted.
          * XXX should probably disable that on other llvm versions.
          */
         assert(*lod_ipart);
         lp_build_nearest_mip_level(bld, unit, *lod_ipart, ilevel0);
      }
      else {
         first_level = bld->dynamic_state->first_level(bld->dynamic_state,
                                                       bld->gallivm, unit);
         first_level = lp_build_broadcast_scalar(&bld->perquadi_bld, first_level);
         *ilevel0 = first_level;
      }
      break;
   case PIPE_TEX_MIPFILTER_NEAREST:
      assert(*lod_ipart);
      lp_build_nearest_mip_level(bld, unit, *lod_ipart, ilevel0);
      break;
   case PIPE_TEX_MIPFILTER_LINEAR:
      assert(*lod_ipart);
      assert(*lod_fpart);
      lp_build_linear_mip_levels(bld, unit,
                                 *lod_ipart, lod_fpart,
                                 ilevel0, ilevel1);
      break;
   }
}

/**
 * General texture sampling codegen.
 * This function handles texture sampling for all texture targets (1D,
 * 2D, 3D, cube) and all filtering modes.
 */
static void
lp_build_sample_general(struct lp_build_sample_context *bld,
                        unsigned unit,
                        LLVMValueRef s,
                        LLVMValueRef t,
                        LLVMValueRef r,
                        LLVMValueRef lod_ipart,
                        LLVMValueRef lod_fpart,
                        LLVMValueRef ilevel0,
                        LLVMValueRef ilevel1,
                        LLVMValueRef *colors_out)
{
   struct lp_build_context *int_bld = &bld->int_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   const unsigned mip_filter = bld->static_state->min_mip_filter;
   const unsigned min_filter = bld->static_state->min_img_filter;
   const unsigned mag_filter = bld->static_state->mag_img_filter;
   LLVMValueRef texels[4];
   unsigned chan;

   /*
    * Get/interpolate texture colors.
    */

   for (chan = 0; chan < 4; ++chan) {
     texels[chan] = lp_build_alloca(bld->gallivm, bld->texel_bld.vec_type, "");
     lp_build_name(texels[chan], "sampler%u_texel_%c_var", unit, "xyzw"[chan]);
   }

   if (min_filter == mag_filter) {
      /* no need to distinguish between minification and magnification */
      lp_build_sample_mipmap(bld, unit,
                             min_filter, mip_filter,
                             s, t, r,
                             ilevel0, ilevel1, lod_fpart,
                             texels);
   }
   else {
      /* Emit conditional to choose min image filter or mag image filter
       * depending on the lod being > 0 or <= 0, respectively.
       */
      struct lp_build_if_state if_ctx;
      LLVMValueRef minify;

      /* minify = lod >= 0.0 */
      minify = LLVMBuildICmp(builder, LLVMIntSGE,
                             lod_ipart, int_bld->zero, "");

      lp_build_if(&if_ctx, bld->gallivm, minify);
      {
         /* Use the minification filter */
         lp_build_sample_mipmap(bld, unit,
                                min_filter, mip_filter,
                                s, t, r,
                                ilevel0, ilevel1, lod_fpart,
                                texels);
      }
      lp_build_else(&if_ctx);
      {
         /* Use the magnification filter */
         lp_build_sample_mipmap(bld, unit,
                                mag_filter, PIPE_TEX_MIPFILTER_NONE,
                                s, t, r,
                                ilevel0, NULL, NULL,
                                texels);
      }
      lp_build_endif(&if_ctx);
   }

   for (chan = 0; chan < 4; ++chan) {
     colors_out[chan] = LLVMBuildLoad(builder, texels[chan], "");
     lp_build_name(colors_out[chan], "sampler%u_texel_%c", unit, "xyzw"[chan]);
   }
}


/**
 * Do shadow test/comparison.
 * \param p  the texcoord Z (aka R, aka P) component
 * \param texel  the texel to compare against (use the X channel)
 */
static void
lp_build_sample_compare(struct lp_build_sample_context *bld,
                        LLVMValueRef p,
                        LLVMValueRef texel[4])
{
   struct lp_build_context *texel_bld = &bld->texel_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef res;
   const unsigned chan = 0;

   if (bld->static_state->compare_mode == PIPE_TEX_COMPARE_NONE)
      return;

   /* debug code */
   if (0) {
      LLVMValueRef indx = lp_build_const_int32(bld->gallivm, 0);
      LLVMValueRef coord = LLVMBuildExtractElement(builder, p, indx, "");
      LLVMValueRef tex = LLVMBuildExtractElement(builder, texel[chan], indx, "");
      lp_build_printf(bld->gallivm, "shadow compare coord %f to texture %f\n",
                      coord, tex);
   }

   /* Clamp p coords to [0,1] */
   p = lp_build_clamp(&bld->coord_bld, p,
                      bld->coord_bld.zero,
                      bld->coord_bld.one);

   /* result = (p FUNC texel) ? 1 : 0 */
   res = lp_build_cmp(texel_bld, bld->static_state->compare_func,
                      p, texel[chan]);
   res = lp_build_select(texel_bld, res, texel_bld->one, texel_bld->zero);

   /* XXX returning result for default GL_DEPTH_TEXTURE_MODE = GL_LUMINANCE */
   texel[0] =
   texel[1] =
   texel[2] = res;
   texel[3] = texel_bld->one;
}


/**
 * Just set texels to white instead of actually sampling the texture.
 * For debugging.
 */
void
lp_build_sample_nop(struct gallivm_state *gallivm,
                    struct lp_type type,
                    unsigned num_coords,
                    const LLVMValueRef *coords,
                    LLVMValueRef texel_out[4])
{
   LLVMValueRef one = lp_build_one(gallivm, type);
   unsigned chan;

   for (chan = 0; chan < 4; chan++) {
      texel_out[chan] = one;
   }  
}


/**
 * Build texture sampling code.
 * 'texel' will return a vector of four LLVMValueRefs corresponding to
 * R, G, B, A.
 * \param type  vector float type to use for coords, etc.
 * \param derivs  partial derivatives of (s,t,r,q) with respect to x and y
 */
void
lp_build_sample_soa(struct gallivm_state *gallivm,
                    const struct lp_sampler_static_state *static_state,
                    struct lp_sampler_dynamic_state *dynamic_state,
                    struct lp_type type,
                    unsigned unit,
                    unsigned num_coords,
                    const LLVMValueRef *coords,
                    const struct lp_derivatives *derivs,
                    LLVMValueRef lod_bias, /* optional */
                    LLVMValueRef explicit_lod, /* optional */
                    LLVMValueRef texel_out[4])
{
   unsigned dims = texture_dims(static_state->target);
   struct lp_build_sample_context bld;
   LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef tex_width, tex_height, tex_depth;
   LLVMValueRef s;
   LLVMValueRef t;
   LLVMValueRef r;

   if (0) {
      enum pipe_format fmt = static_state->format;
      debug_printf("Sample from %s\n", util_format_name(fmt));
   }

   assert(type.floating);

   /* Setup our build context */
   memset(&bld, 0, sizeof bld);
   bld.gallivm = gallivm;
   bld.static_state = static_state;
   bld.dynamic_state = dynamic_state;
   bld.format_desc = util_format_description(static_state->format);
   bld.dims = dims;

   bld.vector_width = lp_type_width(type);

   bld.float_type = lp_type_float(32);
   bld.int_type = lp_type_int(32);
   bld.coord_type = type;
   bld.int_coord_type = lp_int_type(type);
   bld.float_size_type = lp_type_float(32);
   bld.float_size_type.length = dims > 1 ? 4 : 1;
   bld.int_size_type = lp_int_type(bld.float_size_type);
   bld.texel_type = type;
   bld.perquadf_type = type;
   /* we want native vector size to be able to use our intrinsics */
   bld.perquadf_type.length = type.length > 4 ? ((type.length + 15) / 16) * 4 : 1;
   bld.perquadi_type = lp_int_type(bld.perquadf_type);

   lp_build_context_init(&bld.float_bld, gallivm, bld.float_type);
   lp_build_context_init(&bld.float_vec_bld, gallivm, type);
   lp_build_context_init(&bld.int_bld, gallivm, bld.int_type);
   lp_build_context_init(&bld.coord_bld, gallivm, bld.coord_type);
   lp_build_context_init(&bld.int_coord_bld, gallivm, bld.int_coord_type);
   lp_build_context_init(&bld.int_size_bld, gallivm, bld.int_size_type);
   lp_build_context_init(&bld.float_size_bld, gallivm, bld.float_size_type);
   lp_build_context_init(&bld.texel_bld, gallivm, bld.texel_type);
   lp_build_context_init(&bld.perquadf_bld, gallivm, bld.perquadf_type);
   lp_build_context_init(&bld.perquadi_bld, gallivm, bld.perquadi_type);

   /* Get the dynamic state */
   tex_width = dynamic_state->width(dynamic_state, gallivm, unit);
   tex_height = dynamic_state->height(dynamic_state, gallivm, unit);
   tex_depth = dynamic_state->depth(dynamic_state, gallivm, unit);
   bld.row_stride_array = dynamic_state->row_stride(dynamic_state, gallivm, unit);
   bld.img_stride_array = dynamic_state->img_stride(dynamic_state, gallivm, unit);
   bld.data_array = dynamic_state->data_ptr(dynamic_state, gallivm, unit);
   /* Note that data_array is an array[level] of pointers to texture images */

   s = coords[0];
   t = coords[1];
   r = coords[2];

   /* width, height, depth as single int vector */
   if (dims <= 1) {
      bld.int_size = tex_width;
   }
   else {
      bld.int_size = LLVMBuildInsertElement(builder, bld.int_size_bld.undef,
                                            tex_width, LLVMConstInt(i32t, 0, 0), "");
      if (dims >= 2) {
         bld.int_size = LLVMBuildInsertElement(builder, bld.int_size,
                                               tex_height, LLVMConstInt(i32t, 1, 0), "");
         if (dims >= 3) {
            bld.int_size = LLVMBuildInsertElement(builder, bld.int_size,
                                                  tex_depth, LLVMConstInt(i32t, 2, 0), "");
         }
      }
   }

   if (0) {
      /* For debug: no-op texture sampling */
      lp_build_sample_nop(gallivm,
                          bld.texel_type,
                          num_coords,
                          coords,
                          texel_out);
   }
   else {
      LLVMValueRef lod_ipart = NULL, lod_fpart = NULL;
      LLVMValueRef ilevel0 = NULL, ilevel1 = NULL;
      unsigned num_quads = type.length / 4;
      const unsigned mip_filter = bld.static_state->min_mip_filter;
      boolean use_aos = util_format_fits_8unorm(bld.format_desc) &&
                        lp_is_simple_wrap_mode(static_state->wrap_s) &&
                        lp_is_simple_wrap_mode(static_state->wrap_t);

      if ((gallivm_debug & GALLIVM_DEBUG_PERF) &&
          !use_aos && util_format_fits_8unorm(bld.format_desc)) {
         debug_printf("%s: using floating point linear filtering for %s\n",
                      __FUNCTION__, bld.format_desc->short_name);
         debug_printf("  min_img %d  mag_img %d  mip %d  wraps %d  wrapt %d\n",
                      static_state->min_img_filter,
                      static_state->mag_img_filter,
                      static_state->min_mip_filter,
                      static_state->wrap_s,
                      static_state->wrap_t);
      }

      lp_build_sample_common(&bld, unit,
                             &s, &t, &r,
                             derivs, lod_bias, explicit_lod,
                             &lod_ipart, &lod_fpart,
                             &ilevel0, &ilevel1);

      /*
       * we only try 8-wide sampling with soa as it appears to
       * be a loss with aos with AVX.
       */
      if (num_quads == 1 || (mip_filter == PIPE_TEX_MIPFILTER_NONE &&
                             !use_aos)) {

         if (num_quads > 1) {
            LLVMValueRef index0 = lp_build_const_int32(gallivm, 0);
            /* These parameters are the same for all quads */
            lod_ipart = LLVMBuildExtractElement(builder, lod_ipart, index0, "");
            ilevel0 = LLVMBuildExtractElement(builder, ilevel0, index0, "");
         }
         if (use_aos) {
            /* do sampling/filtering with fixed pt arithmetic */
            lp_build_sample_aos(&bld, unit,
                                s, t, r,
                                lod_ipart, lod_fpart,
                                ilevel0, ilevel1,
                                texel_out);
         }

         else {
            lp_build_sample_general(&bld, unit,
                                    s, t, r,
                                    lod_ipart, lod_fpart,
                                    ilevel0, ilevel1,
                                    texel_out);
         }
      }
      else {
         struct lp_build_if_state if_ctx;
         LLVMValueRef notsame_levels, notsame;
         LLVMValueRef index0 = lp_build_const_int32(gallivm, 0);
         LLVMValueRef texels[4];
         LLVMValueRef texelout[4];
         unsigned j;

         texels[0] = lp_build_alloca(gallivm, bld.texel_bld.vec_type, "texr");
         texels[1] = lp_build_alloca(gallivm, bld.texel_bld.vec_type, "texg");
         texels[2] = lp_build_alloca(gallivm, bld.texel_bld.vec_type, "texb");
         texels[3] = lp_build_alloca(gallivm, bld.texel_bld.vec_type, "texa");

         /* only build the if if we MAY split, otherwise always split */
         if (!use_aos) {
            notsame = lp_build_extract_broadcast(gallivm,
                                                 bld.perquadi_bld.type,
                                                 bld.perquadi_bld.type,
                                                 ilevel0, index0);
            notsame = lp_build_sub(&bld.perquadi_bld, ilevel0, notsame);
            notsame_levels = lp_build_any_true_range(&bld.perquadi_bld, num_quads,
                                                     notsame);
            if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
               notsame = lp_build_extract_broadcast(gallivm,
                                                    bld.perquadi_bld.type,
                                                    bld.perquadi_bld.type,
                                                    ilevel1, index0);
               notsame = lp_build_sub(&bld.perquadi_bld, ilevel1, notsame);
               notsame = lp_build_any_true_range(&bld.perquadi_bld, num_quads, notsame);
               notsame_levels = LLVMBuildOr(builder, notsame_levels, notsame, "");
            }
            lp_build_if(&if_ctx, gallivm, notsame_levels);
         }

         {
            struct lp_build_sample_context bld4;
            struct lp_type type4 = type;
            unsigned i;
            LLVMValueRef texelout4[4];
            LLVMValueRef texelouttmp[4][LP_MAX_VECTOR_LENGTH/16];

            type4.length = 4;

            /* Setup our build context */
            memset(&bld4, 0, sizeof bld4);
            bld4.gallivm = bld.gallivm;
            bld4.static_state = bld.static_state;
            bld4.dynamic_state = bld.dynamic_state;
            bld4.format_desc = bld.format_desc;
            bld4.dims = bld.dims;
            bld4.row_stride_array = bld.row_stride_array;
            bld4.img_stride_array = bld.img_stride_array;
            bld4.data_array = bld.data_array;
            bld4.int_size = bld.int_size;

            bld4.vector_width = lp_type_width(type4);

            bld4.float_type = lp_type_float(32);
            bld4.int_type = lp_type_int(32);
            bld4.coord_type = type4;
            bld4.int_coord_type = lp_int_type(type4);
            bld4.float_size_type = lp_type_float(32);
            bld4.float_size_type.length = dims > 1 ? 4 : 1;
            bld4.int_size_type = lp_int_type(bld4.float_size_type);
            bld4.texel_type = type4;
            bld4.perquadf_type = type4;
            /* we want native vector size to be able to use our intrinsics */
            bld4.perquadf_type.length = 1;
            bld4.perquadi_type = lp_int_type(bld4.perquadf_type);

            lp_build_context_init(&bld4.float_bld, gallivm, bld4.float_type);
            lp_build_context_init(&bld4.float_vec_bld, gallivm, type4);
            lp_build_context_init(&bld4.int_bld, gallivm, bld4.int_type);
            lp_build_context_init(&bld4.coord_bld, gallivm, bld4.coord_type);
            lp_build_context_init(&bld4.int_coord_bld, gallivm, bld4.int_coord_type);
            lp_build_context_init(&bld4.int_size_bld, gallivm, bld4.int_size_type);
            lp_build_context_init(&bld4.float_size_bld, gallivm, bld4.float_size_type);
            lp_build_context_init(&bld4.texel_bld, gallivm, bld4.texel_type);
            lp_build_context_init(&bld4.perquadf_bld, gallivm, bld4.perquadf_type);
            lp_build_context_init(&bld4.perquadi_bld, gallivm, bld4.perquadi_type);

            for (i = 0; i < num_quads; i++) {
               LLVMValueRef s4, t4, r4;
               LLVMValueRef lod_iparts, lod_fparts = NULL;
               LLVMValueRef ilevel0s, ilevel1s = NULL;
               LLVMValueRef indexi = lp_build_const_int32(gallivm, i);

               s4 = lp_build_extract_range(gallivm, s, 4*i, 4);
               t4 = lp_build_extract_range(gallivm, t, 4*i, 4);
               r4 = lp_build_extract_range(gallivm, r, 4*i, 4);
               lod_iparts = LLVMBuildExtractElement(builder, lod_ipart, indexi, "");
               ilevel0s = LLVMBuildExtractElement(builder, ilevel0, indexi, "");
               if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
                  ilevel1s = LLVMBuildExtractElement(builder, ilevel1, indexi, "");
                  lod_fparts = LLVMBuildExtractElement(builder, lod_fpart, indexi, "");
               }

               if (use_aos) {
                  /* do sampling/filtering with fixed pt arithmetic */
                  lp_build_sample_aos(&bld4, unit,
                                      s4, t4, r4,
                                      lod_iparts, lod_fparts,
                                      ilevel0s, ilevel1s,
                                      texelout4);
               }

               else {
                  lp_build_sample_general(&bld4, unit,
                                          s4, t4, r4,
                                          lod_iparts, lod_fparts,
                                          ilevel0s, ilevel1s,
                                          texelout4);
               }
               for (j = 0; j < 4; j++) {
                  texelouttmp[j][i] = texelout4[j];
               }
            }
            for (j = 0; j < 4; j++) {
               texelout[j] = lp_build_concat(gallivm, texelouttmp[j], type4, num_quads);
               LLVMBuildStore(builder, texelout[j], texels[j]);
            }
         }
         if (!use_aos) {
            LLVMValueRef ilevel0s, lod_iparts, ilevel1s = NULL;

            lp_build_else(&if_ctx);

            /* These parameters are the same for all quads */
            lod_iparts = LLVMBuildExtractElement(builder, lod_ipart, index0, "");
            ilevel0s = LLVMBuildExtractElement(builder, ilevel0, index0, "");
            if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
               ilevel1s = LLVMBuildExtractElement(builder, ilevel1, index0, "");
            }

            if (use_aos) {
               /* do sampling/filtering with fixed pt arithmetic */
               lp_build_sample_aos(&bld, unit,
                                   s, t, r,
                                   lod_iparts, lod_fpart,
                                   ilevel0s, ilevel1s,
                                   texelout);
            }

            else {
               lp_build_sample_general(&bld, unit,
                                       s, t, r,
                                       lod_iparts, lod_fpart,
                                       ilevel0s, ilevel1s,
                                       texelout);
            }
            for (j = 0; j < 4; j++) {
               LLVMBuildStore(builder, texelout[j], texels[j]);
            }

            lp_build_endif(&if_ctx);
         }

         for (j = 0; j < 4; j++) {
            texel_out[j] = LLVMBuildLoad(builder, texels[j], "");
         }
      }
   }

   lp_build_sample_compare(&bld, r, texel_out);

   apply_sampler_swizzle(&bld, texel_out);
}

void
lp_build_size_query_soa(struct gallivm_state *gallivm,
                        const struct lp_sampler_static_state *static_state,
                        struct lp_sampler_dynamic_state *dynamic_state,
                        struct lp_type int_type,
                        unsigned unit,
                        LLVMValueRef explicit_lod,
                        LLVMValueRef *sizes_out)
{
   LLVMValueRef lod;
   LLVMValueRef size;
   int dims, i;
   struct lp_build_context bld_int_vec;

   switch (static_state->target) {
   case PIPE_TEXTURE_1D:
   case PIPE_BUFFER:
      dims = 1;
      break;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_RECT:
      dims = 2;
      break;
   case PIPE_TEXTURE_3D:
      dims = 3;
      break;

   default:
      assert(0);
      return;
   }

   assert(!int_type.floating);

   lp_build_context_init(&bld_int_vec, gallivm, lp_type_int_vec(32, 128));

   if (explicit_lod) {
      LLVMValueRef first_level;
      lod = LLVMBuildExtractElement(gallivm->builder, explicit_lod, lp_build_const_int32(gallivm, 0), "");
      first_level = dynamic_state->first_level(dynamic_state, gallivm, unit);
      lod = lp_build_broadcast_scalar(&bld_int_vec,
                                      LLVMBuildAdd(gallivm->builder, lod, first_level, "lod"));

   } else {
      lod = bld_int_vec.zero;
   }

   size = bld_int_vec.undef;

   size = LLVMBuildInsertElement(gallivm->builder, size,
                                 dynamic_state->width(dynamic_state, gallivm, unit),
                                 lp_build_const_int32(gallivm, 0), "");

   if (dims >= 2) {
      size = LLVMBuildInsertElement(gallivm->builder, size,
                                    dynamic_state->height(dynamic_state, gallivm, unit),
                                    lp_build_const_int32(gallivm, 1), "");
   }

   if (dims >= 3) {
      size = LLVMBuildInsertElement(gallivm->builder, size,
                                    dynamic_state->depth(dynamic_state, gallivm, unit),
                                    lp_build_const_int32(gallivm, 2), "");
   }

   size = lp_build_minify(&bld_int_vec, size, lod);

   for (i=0; i < dims; i++) {
      sizes_out[i] = lp_build_extract_broadcast(gallivm, bld_int_vec.type, int_type,
                                                size,
                                                lp_build_const_int32(gallivm, i));
   }
}
