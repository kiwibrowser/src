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

/**
 * @file
 * Texture sampling -- AoS.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 * @author Brian Paul <brianp@vmware.com>
 */

#include "pipe/p_defines.h"
#include "pipe/p_state.h"
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
#include "lp_bld_swizzle.h"
#include "lp_bld_pack.h"
#include "lp_bld_flow.h"
#include "lp_bld_gather.h"
#include "lp_bld_format.h"
#include "lp_bld_init.h"
#include "lp_bld_sample.h"
#include "lp_bld_sample_aos.h"
#include "lp_bld_quad.h"


/**
 * Build LLVM code for texture coord wrapping, for nearest filtering,
 * for scaled integer texcoords.
 * \param block_length  is the length of the pixel block along the
 *                      coordinate axis
 * \param coord  the incoming texcoord (s,t,r or q) scaled to the texture size
 * \param length  the texture size along one dimension
 * \param stride  pixel stride along the coordinate axis (in bytes)
 * \param is_pot  if TRUE, length is a power of two
 * \param wrap_mode  one of PIPE_TEX_WRAP_x
 * \param out_offset  byte offset for the wrapped coordinate
 * \param out_i  resulting sub-block pixel coordinate for coord0
 */
static void
lp_build_sample_wrap_nearest_int(struct lp_build_sample_context *bld,
                                 unsigned block_length,
                                 LLVMValueRef coord,
                                 LLVMValueRef coord_f,
                                 LLVMValueRef length,
                                 LLVMValueRef stride,
                                 boolean is_pot,
                                 unsigned wrap_mode,
                                 LLVMValueRef *out_offset,
                                 LLVMValueRef *out_i)
{
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef length_minus_one;

   length_minus_one = lp_build_sub(int_coord_bld, length, int_coord_bld->one);

   switch(wrap_mode) {
   case PIPE_TEX_WRAP_REPEAT:
      if(is_pot)
         coord = LLVMBuildAnd(builder, coord, length_minus_one, "");
      else {
         struct lp_build_context *coord_bld = &bld->coord_bld;
         LLVMValueRef length_f = lp_build_int_to_float(coord_bld, length);
         coord = lp_build_fract_safe(coord_bld, coord_f);
         coord = lp_build_mul(coord_bld, coord, length_f);
         coord = lp_build_itrunc(coord_bld, coord);
      }
      break;

   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      coord = lp_build_max(int_coord_bld, coord, int_coord_bld->zero);
      coord = lp_build_min(int_coord_bld, coord, length_minus_one);
      break;

   case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
   default:
      assert(0);
   }

   lp_build_sample_partial_offset(int_coord_bld, block_length, coord, stride,
                                  out_offset, out_i);
}


/**
 * Build LLVM code for texture coord wrapping, for nearest filtering,
 * for float texcoords.
 * \param coord  the incoming texcoord (s,t,r or q)
 * \param length  the texture size along one dimension
 * \param is_pot  if TRUE, length is a power of two
 * \param wrap_mode  one of PIPE_TEX_WRAP_x
 * \param icoord  the texcoord after wrapping, as int
 */
static void
lp_build_sample_wrap_nearest_float(struct lp_build_sample_context *bld,
                                   LLVMValueRef coord,
                                   LLVMValueRef length,
                                   boolean is_pot,
                                   unsigned wrap_mode,
                                   LLVMValueRef *icoord)
{
   struct lp_build_context *coord_bld = &bld->coord_bld;
   LLVMValueRef length_minus_one;

   switch(wrap_mode) {
   case PIPE_TEX_WRAP_REPEAT:
      /* take fraction, unnormalize */
      coord = lp_build_fract_safe(coord_bld, coord);
      coord = lp_build_mul(coord_bld, coord, length);
      *icoord = lp_build_itrunc(coord_bld, coord);
      break;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      length_minus_one = lp_build_sub(coord_bld, length, coord_bld->one);
      if (bld->static_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length);
      }
      coord = lp_build_clamp(coord_bld, coord, coord_bld->zero,
                             length_minus_one);
      *icoord = lp_build_itrunc(coord_bld, coord);
      break;

   case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
   default:
      assert(0);
   }
}


/**
 * Build LLVM code for texture coord wrapping, for linear filtering,
 * for scaled integer texcoords.
 * \param block_length  is the length of the pixel block along the
 *                      coordinate axis
 * \param coord0  the incoming texcoord (s,t,r or q) scaled to the texture size
 * \param length  the texture size along one dimension
 * \param stride  pixel stride along the coordinate axis (in bytes)
 * \param is_pot  if TRUE, length is a power of two
 * \param wrap_mode  one of PIPE_TEX_WRAP_x
 * \param offset0  resulting relative offset for coord0
 * \param offset1  resulting relative offset for coord0 + 1
 * \param i0  resulting sub-block pixel coordinate for coord0
 * \param i1  resulting sub-block pixel coordinate for coord0 + 1
 */
static void
lp_build_sample_wrap_linear_int(struct lp_build_sample_context *bld,
                                unsigned block_length,
                                LLVMValueRef coord0,
                                LLVMValueRef *weight_i,
                                LLVMValueRef coord_f,
                                LLVMValueRef length,
                                LLVMValueRef stride,
                                boolean is_pot,
                                unsigned wrap_mode,
                                LLVMValueRef *offset0,
                                LLVMValueRef *offset1,
                                LLVMValueRef *i0,
                                LLVMValueRef *i1)
{
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef length_minus_one;
   LLVMValueRef lmask, umask, mask;

   /*
    * If the pixel block covers more than one pixel then there is no easy
    * way to calculate offset1 relative to offset0. Instead, compute them
    * independently. Otherwise, try to compute offset0 and offset1 with
    * a single stride multiplication.
    */

   length_minus_one = lp_build_sub(int_coord_bld, length, int_coord_bld->one);

   if (block_length != 1) {
      LLVMValueRef coord1;
      switch(wrap_mode) {
      case PIPE_TEX_WRAP_REPEAT:
         if (is_pot) {
            coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
            coord0 = LLVMBuildAnd(builder, coord0, length_minus_one, "");
            coord1 = LLVMBuildAnd(builder, coord1, length_minus_one, "");
         }
         else {
            LLVMValueRef mask;
            LLVMValueRef weight;
            LLVMValueRef length_f = lp_build_int_to_float(&bld->coord_bld, length);
            lp_build_coord_repeat_npot_linear(bld, coord_f,
                                              length, length_f,
                                              &coord0, &weight);
            mask = lp_build_compare(bld->gallivm, int_coord_bld->type,
                                    PIPE_FUNC_NOTEQUAL, coord0, length_minus_one);
            coord1 = LLVMBuildAnd(builder,
                                  lp_build_add(int_coord_bld, coord0,
                                               int_coord_bld->one),
                                  mask, "");
            weight = lp_build_mul_imm(&bld->coord_bld, weight, 256);
            *weight_i = lp_build_itrunc(&bld->coord_bld, weight);
         }
         break;

      case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
         coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
         coord0 = lp_build_clamp(int_coord_bld, coord0, int_coord_bld->zero,
                                length_minus_one);
         coord1 = lp_build_clamp(int_coord_bld, coord1, int_coord_bld->zero,
                                length_minus_one);
         break;

      case PIPE_TEX_WRAP_CLAMP:
      case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      case PIPE_TEX_WRAP_MIRROR_REPEAT:
      case PIPE_TEX_WRAP_MIRROR_CLAMP:
      case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      default:
         assert(0);
         coord0 = int_coord_bld->zero;
         coord1 = int_coord_bld->zero;
         break;
      }
      lp_build_sample_partial_offset(int_coord_bld, block_length, coord0, stride,
                                     offset0, i0);
      lp_build_sample_partial_offset(int_coord_bld, block_length, coord1, stride,
                                     offset1, i1);
      return;
   }

   *i0 = int_coord_bld->zero;
   *i1 = int_coord_bld->zero;

   switch(wrap_mode) {
   case PIPE_TEX_WRAP_REPEAT:
      if (is_pot) {
         coord0 = LLVMBuildAnd(builder, coord0, length_minus_one, "");
      }
      else {
         LLVMValueRef weight;
         LLVMValueRef length_f = lp_build_int_to_float(&bld->coord_bld, length);
         lp_build_coord_repeat_npot_linear(bld, coord_f,
                                           length, length_f,
                                           &coord0, &weight);
         weight = lp_build_mul_imm(&bld->coord_bld, weight, 256);
         *weight_i = lp_build_itrunc(&bld->coord_bld, weight);
      }

      mask = lp_build_compare(bld->gallivm, int_coord_bld->type,
                              PIPE_FUNC_NOTEQUAL, coord0, length_minus_one);

      *offset0 = lp_build_mul(int_coord_bld, coord0, stride);
      *offset1 = LLVMBuildAnd(builder,
                              lp_build_add(int_coord_bld, *offset0, stride),
                              mask, "");
      break;

   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      /* XXX this might be slower than the separate path
       * on some newer cpus. With sse41 this is 8 instructions vs. 7
       * - at least on SNB this is almost certainly slower since
       * min/max are cheaper than selects, and the muls aren't bad.
       */
      lmask = lp_build_compare(int_coord_bld->gallivm, int_coord_bld->type,
                               PIPE_FUNC_GEQUAL, coord0, int_coord_bld->zero);
      umask = lp_build_compare(int_coord_bld->gallivm, int_coord_bld->type,
                               PIPE_FUNC_LESS, coord0, length_minus_one);

      coord0 = lp_build_select(int_coord_bld, lmask, coord0, int_coord_bld->zero);
      coord0 = lp_build_select(int_coord_bld, umask, coord0, length_minus_one);

      mask = LLVMBuildAnd(builder, lmask, umask, "");

      *offset0 = lp_build_mul(int_coord_bld, coord0, stride);
      *offset1 = lp_build_add(int_coord_bld,
                              *offset0,
                              LLVMBuildAnd(builder, stride, mask, ""));
      break;

   case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
   default:
      assert(0);
      *offset0 = int_coord_bld->zero;
      *offset1 = int_coord_bld->zero;
      break;
   }
}


/**
 * Build LLVM code for texture coord wrapping, for linear filtering,
 * for float texcoords.
 * \param block_length  is the length of the pixel block along the
 *                      coordinate axis
 * \param coord  the incoming texcoord (s,t,r or q)
 * \param length  the texture size along one dimension
 * \param is_pot  if TRUE, length is a power of two
 * \param wrap_mode  one of PIPE_TEX_WRAP_x
 * \param coord0  the first texcoord after wrapping, as int
 * \param coord1  the second texcoord after wrapping, as int
 * \param weight  the filter weight as int (0-255)
 * \param force_nearest  if this coord actually uses nearest filtering
 */
static void
lp_build_sample_wrap_linear_float(struct lp_build_sample_context *bld,
                                  unsigned block_length,
                                  LLVMValueRef coord,
                                  LLVMValueRef length,
                                  boolean is_pot,
                                  unsigned wrap_mode,
                                  LLVMValueRef *coord0,
                                  LLVMValueRef *coord1,
                                  LLVMValueRef *weight,
                                  unsigned force_nearest)
{
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   struct lp_build_context *coord_bld = &bld->coord_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef half = lp_build_const_vec(bld->gallivm, coord_bld->type, 0.5);
   LLVMValueRef length_minus_one = lp_build_sub(coord_bld, length, coord_bld->one);

   switch(wrap_mode) {
   case PIPE_TEX_WRAP_REPEAT:
      if (is_pot) {
         /* mul by size and subtract 0.5 */
         coord = lp_build_mul(coord_bld, coord, length);
         if (!force_nearest)
            coord = lp_build_sub(coord_bld, coord, half);
         *coord1 = lp_build_add(coord_bld, coord, coord_bld->one);
         /* convert to int, compute lerp weight */
         lp_build_ifloor_fract(coord_bld, coord, coord0, weight);
         *coord1 = lp_build_ifloor(coord_bld, *coord1);
         /* repeat wrap */
         length_minus_one = lp_build_itrunc(coord_bld, length_minus_one);
         *coord0 = LLVMBuildAnd(builder, *coord0, length_minus_one, "");
         *coord1 = LLVMBuildAnd(builder, *coord1, length_minus_one, "");
      }
      else {
         LLVMValueRef mask;
         /* wrap with normalized floats is just fract */
         coord = lp_build_fract(coord_bld, coord);
         /* unnormalize */
         coord = lp_build_mul(coord_bld, coord, length);
         /*
          * we avoided the 0.5/length division, have to fix up wrong
          * edge cases with selects
          */
         *coord1 = lp_build_add(coord_bld, coord, half);
         coord = lp_build_sub(coord_bld, coord, half);
         *weight = lp_build_fract(coord_bld, coord);
         mask = lp_build_compare(coord_bld->gallivm, coord_bld->type,
                                 PIPE_FUNC_LESS, coord, coord_bld->zero);
         *coord0 = lp_build_select(coord_bld, mask, length_minus_one, coord);
         *coord0 = lp_build_itrunc(coord_bld, *coord0);
         mask = lp_build_compare(coord_bld->gallivm, coord_bld->type,
                                 PIPE_FUNC_LESS, *coord1, length);
         *coord1 = lp_build_select(coord_bld, mask, *coord1, coord_bld->zero);
         *coord1 = lp_build_itrunc(coord_bld, *coord1);
      }
      break;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      if (bld->static_state->normalized_coords) {
         /* mul by tex size */
         coord = lp_build_mul(coord_bld, coord, length);
      }
      /* subtract 0.5 */
      if (!force_nearest) {
         coord = lp_build_sub(coord_bld, coord, half);
      }
      /* clamp to [0, length - 1] */
      coord = lp_build_min(coord_bld, coord, length_minus_one);
      coord = lp_build_max(coord_bld, coord, coord_bld->zero);
      *coord1 = lp_build_add(coord_bld, coord, coord_bld->one);
      /* convert to int, compute lerp weight */
      lp_build_ifloor_fract(coord_bld, coord, coord0, weight);
      /* coord1 = min(coord1, length-1) */
      *coord1 = lp_build_min(coord_bld, *coord1, length_minus_one);
      *coord1 = lp_build_itrunc(coord_bld, *coord1);
      break;
   default:
      assert(0);
      *coord0 = int_coord_bld->zero;
      *coord1 = int_coord_bld->zero;
      *weight = coord_bld->zero;
      break;
   }
   *weight = lp_build_mul_imm(coord_bld, *weight, 256);
   *weight = lp_build_itrunc(coord_bld, *weight);
   return;
}


/**
 * Fetch texels for image with nearest sampling.
 * Return filtered color as two vectors of 16-bit fixed point values.
 */
static void
lp_build_sample_fetch_image_nearest(struct lp_build_sample_context *bld,
                                    LLVMValueRef data_ptr,
                                    LLVMValueRef offset,
                                    LLVMValueRef x_subcoord,
                                    LLVMValueRef y_subcoord,
                                    LLVMValueRef *colors_lo,
                                    LLVMValueRef *colors_hi)
{
   /*
    * Fetch the pixels as 4 x 32bit (rgba order might differ):
    *
    *   rgba0 rgba1 rgba2 rgba3
    *
    * bit cast them into 16 x u8
    *
    *   r0 g0 b0 a0 r1 g1 b1 a1 r2 g2 b2 a2 r3 g3 b3 a3
    *
    * unpack them into two 8 x i16:
    *
    *   r0 g0 b0 a0 r1 g1 b1 a1
    *   r2 g2 b2 a2 r3 g3 b3 a3
    *
    * The higher 8 bits of the resulting elements will be zero.
    */
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef rgba8;
   struct lp_build_context h16, u8n;
   LLVMTypeRef u8n_vec_type;

   lp_build_context_init(&h16, bld->gallivm, lp_type_ufixed(16, bld->vector_width));
   lp_build_context_init(&u8n, bld->gallivm, lp_type_unorm(8, bld->vector_width));
   u8n_vec_type = lp_build_vec_type(bld->gallivm, u8n.type);

   if (util_format_is_rgba8_variant(bld->format_desc)) {
      /*
       * Given the format is a rgba8, just read the pixels as is,
       * without any swizzling. Swizzling will be done later.
       */
      rgba8 = lp_build_gather(bld->gallivm,
                              bld->texel_type.length,
                              bld->format_desc->block.bits,
                              bld->texel_type.width,
                              data_ptr, offset);

      rgba8 = LLVMBuildBitCast(builder, rgba8, u8n_vec_type, "");
   }
   else {
      rgba8 = lp_build_fetch_rgba_aos(bld->gallivm,
                                      bld->format_desc,
                                      u8n.type,
                                      data_ptr, offset,
                                      x_subcoord,
                                      y_subcoord);
   }

   /* Expand one 4*rgba8 to two 2*rgba16 */
   lp_build_unpack2(bld->gallivm, u8n.type, h16.type,
                    rgba8,
                    colors_lo, colors_hi);
}


/**
 * Sample a single texture image with nearest sampling.
 * If sampling a cube texture, r = cube face in [0,5].
 * Return filtered color as two vectors of 16-bit fixed point values.
 */
static void
lp_build_sample_image_nearest(struct lp_build_sample_context *bld,
                              LLVMValueRef int_size,
                              LLVMValueRef row_stride_vec,
                              LLVMValueRef img_stride_vec,
                              LLVMValueRef data_ptr,
                              LLVMValueRef s,
                              LLVMValueRef t,
                              LLVMValueRef r,
                              LLVMValueRef *colors_lo,
                              LLVMValueRef *colors_hi)
{
   const unsigned dims = bld->dims;
   LLVMBuilderRef builder = bld->gallivm->builder;
   struct lp_build_context i32;
   LLVMTypeRef i32_vec_type;
   LLVMValueRef i32_c8;
   LLVMValueRef width_vec, height_vec, depth_vec;
   LLVMValueRef s_ipart, t_ipart = NULL, r_ipart = NULL;
   LLVMValueRef s_float, t_float = NULL, r_float = NULL;
   LLVMValueRef x_stride;
   LLVMValueRef x_offset, offset;
   LLVMValueRef x_subcoord, y_subcoord, z_subcoord;

   lp_build_context_init(&i32, bld->gallivm, lp_type_int_vec(32, bld->vector_width));

   i32_vec_type = lp_build_vec_type(bld->gallivm, i32.type);

   lp_build_extract_image_sizes(bld,
                                bld->int_size_type,
                                bld->int_coord_type,
                                int_size,
                                &width_vec,
                                &height_vec,
                                &depth_vec);

   s_float = s; t_float = t; r_float = r;

   if (bld->static_state->normalized_coords) {
      LLVMValueRef scaled_size;
      LLVMValueRef flt_size;

      /* scale size by 256 (8 fractional bits) */
      scaled_size = lp_build_shl_imm(&bld->int_size_bld, int_size, 8);

      flt_size = lp_build_int_to_float(&bld->float_size_bld, scaled_size);

      lp_build_unnormalized_coords(bld, flt_size, &s, &t, &r);
   }
   else {
      /* scale coords by 256 (8 fractional bits) */
      s = lp_build_mul_imm(&bld->coord_bld, s, 256);
      if (dims >= 2)
         t = lp_build_mul_imm(&bld->coord_bld, t, 256);
      if (dims >= 3)
         r = lp_build_mul_imm(&bld->coord_bld, r, 256);
   }

   /* convert float to int */
   s = LLVMBuildFPToSI(builder, s, i32_vec_type, "");
   if (dims >= 2)
      t = LLVMBuildFPToSI(builder, t, i32_vec_type, "");
   if (dims >= 3)
      r = LLVMBuildFPToSI(builder, r, i32_vec_type, "");

   /* compute floor (shift right 8) */
   i32_c8 = lp_build_const_int_vec(bld->gallivm, i32.type, 8);
   s_ipart = LLVMBuildAShr(builder, s, i32_c8, "");
   if (dims >= 2)
      t_ipart = LLVMBuildAShr(builder, t, i32_c8, "");
   if (dims >= 3)
      r_ipart = LLVMBuildAShr(builder, r, i32_c8, "");

   /* get pixel, row, image strides */
   x_stride = lp_build_const_vec(bld->gallivm,
                                 bld->int_coord_bld.type,
                                 bld->format_desc->block.bits/8);

   /* Do texcoord wrapping, compute texel offset */
   lp_build_sample_wrap_nearest_int(bld,
                                    bld->format_desc->block.width,
                                    s_ipart, s_float,
                                    width_vec, x_stride,
                                    bld->static_state->pot_width,
                                    bld->static_state->wrap_s,
                                    &x_offset, &x_subcoord);
   offset = x_offset;
   if (dims >= 2) {
      LLVMValueRef y_offset;
      lp_build_sample_wrap_nearest_int(bld,
                                       bld->format_desc->block.height,
                                       t_ipart, t_float,
                                       height_vec, row_stride_vec,
                                       bld->static_state->pot_height,
                                       bld->static_state->wrap_t,
                                       &y_offset, &y_subcoord);
      offset = lp_build_add(&bld->int_coord_bld, offset, y_offset);
      if (dims >= 3) {
         LLVMValueRef z_offset;
         lp_build_sample_wrap_nearest_int(bld,
                                          1, /* block length (depth) */
                                          r_ipart, r_float,
                                          depth_vec, img_stride_vec,
                                          bld->static_state->pot_depth,
                                          bld->static_state->wrap_r,
                                          &z_offset, &z_subcoord);
         offset = lp_build_add(&bld->int_coord_bld, offset, z_offset);
      }
      else if (bld->static_state->target == PIPE_TEXTURE_CUBE) {
         LLVMValueRef z_offset;
         /* The r coord is the cube face in [0,5] */
         z_offset = lp_build_mul(&bld->int_coord_bld, r, img_stride_vec);
         offset = lp_build_add(&bld->int_coord_bld, offset, z_offset);
      }
   }

   lp_build_sample_fetch_image_nearest(bld, data_ptr, offset,
                                       x_subcoord, y_subcoord,
                                       colors_lo, colors_hi);
}


/**
 * Sample a single texture image with nearest sampling.
 * If sampling a cube texture, r = cube face in [0,5].
 * Return filtered color as two vectors of 16-bit fixed point values.
 * Does address calcs (except offsets) with floats.
 * Useful for AVX which has support for 8x32 floats but not 8x32 ints.
 */
static void
lp_build_sample_image_nearest_afloat(struct lp_build_sample_context *bld,
                                     LLVMValueRef int_size,
                                     LLVMValueRef row_stride_vec,
                                     LLVMValueRef img_stride_vec,
                                     LLVMValueRef data_ptr,
                                     LLVMValueRef s,
                                     LLVMValueRef t,
                                     LLVMValueRef r,
                                     LLVMValueRef *colors_lo,
                                     LLVMValueRef *colors_hi)
   {
   const unsigned dims = bld->dims;
   LLVMValueRef width_vec, height_vec, depth_vec;
   LLVMValueRef offset;
   LLVMValueRef x_subcoord, y_subcoord;
   LLVMValueRef x_icoord = NULL, y_icoord = NULL, z_icoord = NULL;
   LLVMValueRef flt_size;

   flt_size = lp_build_int_to_float(&bld->float_size_bld, int_size);

   lp_build_extract_image_sizes(bld,
                                bld->float_size_type,
                                bld->coord_type,
                                flt_size,
                                &width_vec,
                                &height_vec,
                                &depth_vec);

   /* Do texcoord wrapping */
   lp_build_sample_wrap_nearest_float(bld,
                                      s, width_vec,
                                      bld->static_state->pot_width,
                                      bld->static_state->wrap_s,
                                      &x_icoord);

   if (dims >= 2) {
      lp_build_sample_wrap_nearest_float(bld,
                                         t, height_vec,
                                         bld->static_state->pot_height,
                                         bld->static_state->wrap_t,
                                         &y_icoord);

      if (dims >= 3) {
         lp_build_sample_wrap_nearest_float(bld,
                                            r, depth_vec,
                                            bld->static_state->pot_depth,
                                            bld->static_state->wrap_r,
                                            &z_icoord);
      }
      else if (bld->static_state->target == PIPE_TEXTURE_CUBE) {
         z_icoord = r;
      }
   }

   /*
    * From here on we deal with ints, and we should split up the 256bit
    * vectors manually for better generated code.
    */

   /*
    * compute texel offsets -
    * cannot do offset calc with floats, difficult for block-based formats,
    * and not enough precision anyway.
    */
   lp_build_sample_offset(&bld->int_coord_bld,
                          bld->format_desc,
                          x_icoord, y_icoord,
                          z_icoord,
                          row_stride_vec, img_stride_vec,
                          &offset,
                          &x_subcoord, &y_subcoord);

   lp_build_sample_fetch_image_nearest(bld, data_ptr, offset,
                                       x_subcoord, y_subcoord,
                                       colors_lo, colors_hi);
}


/**
 * Fetch texels for image with linear sampling.
 * Return filtered color as two vectors of 16-bit fixed point values.
 */
static void
lp_build_sample_fetch_image_linear(struct lp_build_sample_context *bld,
                                   LLVMValueRef data_ptr,
                                   LLVMValueRef offset[2][2][2],
                                   LLVMValueRef x_subcoord[2],
                                   LLVMValueRef y_subcoord[2],
                                   LLVMValueRef s_fpart,
                                   LLVMValueRef t_fpart,
                                   LLVMValueRef r_fpart,
                                   LLVMValueRef *colors_lo,
                                   LLVMValueRef *colors_hi)
{
   const unsigned dims = bld->dims;
   LLVMBuilderRef builder = bld->gallivm->builder;
   struct lp_build_context h16, u8n;
   LLVMTypeRef h16_vec_type, u8n_vec_type;
   LLVMTypeRef elem_type = LLVMInt32TypeInContext(bld->gallivm->context);
   LLVMValueRef shuffles_lo[LP_MAX_VECTOR_LENGTH];
   LLVMValueRef shuffles_hi[LP_MAX_VECTOR_LENGTH];
   LLVMValueRef shuffle_lo, shuffle_hi;
   LLVMValueRef s_fpart_lo, s_fpart_hi;
   LLVMValueRef t_fpart_lo = NULL, t_fpart_hi = NULL;
   LLVMValueRef r_fpart_lo = NULL, r_fpart_hi = NULL;
   LLVMValueRef neighbors_lo[2][2][2]; /* [z][y][x] */
   LLVMValueRef neighbors_hi[2][2][2]; /* [z][y][x] */
   LLVMValueRef packed_lo, packed_hi;
   unsigned i, j, k;
   unsigned numj, numk;

   lp_build_context_init(&h16, bld->gallivm, lp_type_ufixed(16, bld->vector_width));
   lp_build_context_init(&u8n, bld->gallivm, lp_type_unorm(8, bld->vector_width));
   h16_vec_type = lp_build_vec_type(bld->gallivm, h16.type);
   u8n_vec_type = lp_build_vec_type(bld->gallivm, u8n.type);

   /*
    * Transform 4 x i32 in
    *
    *   s_fpart = {s0, s1, s2, s3}
    *
    * into 8 x i16
    *
    *   s_fpart = {00, s0, 00, s1, 00, s2, 00, s3}
    *
    * into two 8 x i16
    *
    *   s_fpart_lo = {s0, s0, s0, s0, s1, s1, s1, s1}
    *   s_fpart_hi = {s2, s2, s2, s2, s3, s3, s3, s3}
    *
    * and likewise for t_fpart. There is no risk of loosing precision here
    * since the fractional parts only use the lower 8bits.
    */
   s_fpart = LLVMBuildBitCast(builder, s_fpart, h16_vec_type, "");
   if (dims >= 2)
      t_fpart = LLVMBuildBitCast(builder, t_fpart, h16_vec_type, "");
   if (dims >= 3)
      r_fpart = LLVMBuildBitCast(builder, r_fpart, h16_vec_type, "");

   for (j = 0; j < h16.type.length; j += 4) {
#ifdef PIPE_ARCH_LITTLE_ENDIAN
      unsigned subindex = 0;
#else
      unsigned subindex = 1;
#endif
      LLVMValueRef index;

      index = LLVMConstInt(elem_type, j/2 + subindex, 0);
      for (i = 0; i < 4; ++i)
         shuffles_lo[j + i] = index;

      index = LLVMConstInt(elem_type, h16.type.length/2 + j/2 + subindex, 0);
      for (i = 0; i < 4; ++i)
         shuffles_hi[j + i] = index;
   }

   shuffle_lo = LLVMConstVector(shuffles_lo, h16.type.length);
   shuffle_hi = LLVMConstVector(shuffles_hi, h16.type.length);

   s_fpart_lo = LLVMBuildShuffleVector(builder, s_fpart, h16.undef,
                                       shuffle_lo, "");
   s_fpart_hi = LLVMBuildShuffleVector(builder, s_fpart, h16.undef,
                                       shuffle_hi, "");
   if (dims >= 2) {
      t_fpart_lo = LLVMBuildShuffleVector(builder, t_fpart, h16.undef,
                                          shuffle_lo, "");
      t_fpart_hi = LLVMBuildShuffleVector(builder, t_fpart, h16.undef,
                                          shuffle_hi, "");
   }
   if (dims >= 3) {
      r_fpart_lo = LLVMBuildShuffleVector(builder, r_fpart, h16.undef,
                                          shuffle_lo, "");
      r_fpart_hi = LLVMBuildShuffleVector(builder, r_fpart, h16.undef,
                                          shuffle_hi, "");
   }

   /*
    * Fetch the pixels as 4 x 32bit (rgba order might differ):
    *
    *   rgba0 rgba1 rgba2 rgba3
    *
    * bit cast them into 16 x u8
    *
    *   r0 g0 b0 a0 r1 g1 b1 a1 r2 g2 b2 a2 r3 g3 b3 a3
    *
    * unpack them into two 8 x i16:
    *
    *   r0 g0 b0 a0 r1 g1 b1 a1
    *   r2 g2 b2 a2 r3 g3 b3 a3
    *
    * The higher 8 bits of the resulting elements will be zero.
    */
   numj = 1 + (dims >= 2);
   numk = 1 + (dims >= 3);

   for (k = 0; k < numk; k++) {
      for (j = 0; j < numj; j++) {
         for (i = 0; i < 2; i++) {
            LLVMValueRef rgba8;

            if (util_format_is_rgba8_variant(bld->format_desc)) {
               /*
                * Given the format is a rgba8, just read the pixels as is,
                * without any swizzling. Swizzling will be done later.
                */
               rgba8 = lp_build_gather(bld->gallivm,
                                       bld->texel_type.length,
                                       bld->format_desc->block.bits,
                                       bld->texel_type.width,
                                       data_ptr, offset[k][j][i]);

               rgba8 = LLVMBuildBitCast(builder, rgba8, u8n_vec_type, "");
            }
            else {
               rgba8 = lp_build_fetch_rgba_aos(bld->gallivm,
                                               bld->format_desc,
                                               u8n.type,
                                               data_ptr, offset[k][j][i],
                                               x_subcoord[i],
                                               y_subcoord[j]);
            }

            /* Expand one 4*rgba8 to two 2*rgba16 */
            lp_build_unpack2(bld->gallivm, u8n.type, h16.type,
                             rgba8,
                             &neighbors_lo[k][j][i], &neighbors_hi[k][j][i]);
         }
      }
   }

   /*
    * Linear interpolation with 8.8 fixed point.
    */
   if (bld->static_state->force_nearest_s) {
      /* special case 1-D lerp */
      packed_lo = lp_build_lerp(&h16,
                                t_fpart_lo,
                                neighbors_lo[0][0][0],
                                neighbors_lo[0][0][1]);

      packed_hi = lp_build_lerp(&h16,
                                t_fpart_hi,
                                neighbors_hi[0][1][0],
                                neighbors_hi[0][1][0]);
   }
   else if (bld->static_state->force_nearest_t) {
      /* special case 1-D lerp */
      packed_lo = lp_build_lerp(&h16,
                                s_fpart_lo,
                                neighbors_lo[0][0][0],
                                neighbors_lo[0][0][1]);

      packed_hi = lp_build_lerp(&h16,
                                s_fpart_hi,
                                neighbors_hi[0][0][0],
                                neighbors_hi[0][0][1]);
   }
   else {
      /* general 1/2/3-D lerping */
      if (dims == 1) {
         packed_lo = lp_build_lerp(&h16,
                                   s_fpart_lo,
                                   neighbors_lo[0][0][0],
                                   neighbors_lo[0][0][1]);

         packed_hi = lp_build_lerp(&h16,
                                   s_fpart_hi,
                                   neighbors_hi[0][0][0],
                                   neighbors_hi[0][0][1]);
      }
      else {
         /* 2-D lerp */
         packed_lo = lp_build_lerp_2d(&h16,
                                      s_fpart_lo, t_fpart_lo,
                                      neighbors_lo[0][0][0],
                                      neighbors_lo[0][0][1],
                                      neighbors_lo[0][1][0],
                                      neighbors_lo[0][1][1]);

         packed_hi = lp_build_lerp_2d(&h16,
                                      s_fpart_hi, t_fpart_hi,
                                      neighbors_hi[0][0][0],
                                      neighbors_hi[0][0][1],
                                      neighbors_hi[0][1][0],
                                      neighbors_hi[0][1][1]);

         if (dims >= 3) {
            LLVMValueRef packed_lo2, packed_hi2;

            /* lerp in the second z slice */
            packed_lo2 = lp_build_lerp_2d(&h16,
                                          s_fpart_lo, t_fpart_lo,
                                          neighbors_lo[1][0][0],
                                          neighbors_lo[1][0][1],
                                          neighbors_lo[1][1][0],
                                          neighbors_lo[1][1][1]);

            packed_hi2 = lp_build_lerp_2d(&h16,
                                          s_fpart_hi, t_fpart_hi,
                                          neighbors_hi[1][0][0],
                                          neighbors_hi[1][0][1],
                                          neighbors_hi[1][1][0],
                                          neighbors_hi[1][1][1]);
            /* interp between two z slices */
            packed_lo = lp_build_lerp(&h16, r_fpart_lo,
                                      packed_lo, packed_lo2);
            packed_hi = lp_build_lerp(&h16, r_fpart_hi,
                                      packed_hi, packed_hi2);
         }
      }
   }

   *colors_lo = packed_lo;
   *colors_hi = packed_hi;
}

/**
 * Sample a single texture image with (bi-)(tri-)linear sampling.
 * Return filtered color as two vectors of 16-bit fixed point values.
 */
static void
lp_build_sample_image_linear(struct lp_build_sample_context *bld,
                             LLVMValueRef int_size,
                             LLVMValueRef row_stride_vec,
                             LLVMValueRef img_stride_vec,
                             LLVMValueRef data_ptr,
                             LLVMValueRef s,
                             LLVMValueRef t,
                             LLVMValueRef r,
                             LLVMValueRef *colors_lo,
                             LLVMValueRef *colors_hi)
{
   const unsigned dims = bld->dims;
   LLVMBuilderRef builder = bld->gallivm->builder;
   struct lp_build_context i32;
   LLVMTypeRef i32_vec_type;
   LLVMValueRef i32_c8, i32_c128, i32_c255;
   LLVMValueRef width_vec, height_vec, depth_vec;
   LLVMValueRef s_ipart, s_fpart, s_float;
   LLVMValueRef t_ipart = NULL, t_fpart = NULL, t_float = NULL;
   LLVMValueRef r_ipart = NULL, r_fpart = NULL, r_float = NULL;
   LLVMValueRef x_stride, y_stride, z_stride;
   LLVMValueRef x_offset0, x_offset1;
   LLVMValueRef y_offset0, y_offset1;
   LLVMValueRef z_offset0, z_offset1;
   LLVMValueRef offset[2][2][2]; /* [z][y][x] */
   LLVMValueRef x_subcoord[2], y_subcoord[2], z_subcoord[2];
   unsigned x, y, z;

   lp_build_context_init(&i32, bld->gallivm, lp_type_int_vec(32, bld->vector_width));

   i32_vec_type = lp_build_vec_type(bld->gallivm, i32.type);

   lp_build_extract_image_sizes(bld,
                                bld->int_size_type,
                                bld->int_coord_type,
                                int_size,
                                &width_vec,
                                &height_vec,
                                &depth_vec);

   s_float = s; t_float = t; r_float = r;

   if (bld->static_state->normalized_coords) {
      LLVMValueRef scaled_size;
      LLVMValueRef flt_size;

      /* scale size by 256 (8 fractional bits) */
      scaled_size = lp_build_shl_imm(&bld->int_size_bld, int_size, 8);

      flt_size = lp_build_int_to_float(&bld->float_size_bld, scaled_size);

      lp_build_unnormalized_coords(bld, flt_size, &s, &t, &r);
   }
   else {
      /* scale coords by 256 (8 fractional bits) */
      s = lp_build_mul_imm(&bld->coord_bld, s, 256);
      if (dims >= 2)
         t = lp_build_mul_imm(&bld->coord_bld, t, 256);
      if (dims >= 3)
         r = lp_build_mul_imm(&bld->coord_bld, r, 256);
   }

   /* convert float to int */
   s = LLVMBuildFPToSI(builder, s, i32_vec_type, "");
   if (dims >= 2)
      t = LLVMBuildFPToSI(builder, t, i32_vec_type, "");
   if (dims >= 3)
      r = LLVMBuildFPToSI(builder, r, i32_vec_type, "");

   /* subtract 0.5 (add -128) */
   i32_c128 = lp_build_const_int_vec(bld->gallivm, i32.type, -128);
   if (!bld->static_state->force_nearest_s) {
      s = LLVMBuildAdd(builder, s, i32_c128, "");
   }
   if (dims >= 2 && !bld->static_state->force_nearest_t) {
      t = LLVMBuildAdd(builder, t, i32_c128, "");
   }
   if (dims >= 3) {
      r = LLVMBuildAdd(builder, r, i32_c128, "");
   }

   /* compute floor (shift right 8) */
   i32_c8 = lp_build_const_int_vec(bld->gallivm, i32.type, 8);
   s_ipart = LLVMBuildAShr(builder, s, i32_c8, "");
   if (dims >= 2)
      t_ipart = LLVMBuildAShr(builder, t, i32_c8, "");
   if (dims >= 3)
      r_ipart = LLVMBuildAShr(builder, r, i32_c8, "");

   /* compute fractional part (AND with 0xff) */
   i32_c255 = lp_build_const_int_vec(bld->gallivm, i32.type, 255);
   s_fpart = LLVMBuildAnd(builder, s, i32_c255, "");
   if (dims >= 2)
      t_fpart = LLVMBuildAnd(builder, t, i32_c255, "");
   if (dims >= 3)
      r_fpart = LLVMBuildAnd(builder, r, i32_c255, "");

   /* get pixel, row and image strides */
   x_stride = lp_build_const_vec(bld->gallivm, bld->int_coord_bld.type,
                                 bld->format_desc->block.bits/8);
   y_stride = row_stride_vec;
   z_stride = img_stride_vec;

   /* do texcoord wrapping and compute texel offsets */
   lp_build_sample_wrap_linear_int(bld,
                                   bld->format_desc->block.width,
                                   s_ipart, &s_fpart, s_float,
                                   width_vec, x_stride,
                                   bld->static_state->pot_width,
                                   bld->static_state->wrap_s,
                                   &x_offset0, &x_offset1,
                                   &x_subcoord[0], &x_subcoord[1]);
   for (z = 0; z < 2; z++) {
      for (y = 0; y < 2; y++) {
         offset[z][y][0] = x_offset0;
         offset[z][y][1] = x_offset1;
      }
   }

   if (dims >= 2) {
      lp_build_sample_wrap_linear_int(bld,
                                      bld->format_desc->block.height,
                                      t_ipart, &t_fpart, t_float,
                                      height_vec, y_stride,
                                      bld->static_state->pot_height,
                                      bld->static_state->wrap_t,
                                      &y_offset0, &y_offset1,
                                      &y_subcoord[0], &y_subcoord[1]);

      for (z = 0; z < 2; z++) {
         for (x = 0; x < 2; x++) {
            offset[z][0][x] = lp_build_add(&bld->int_coord_bld,
                                           offset[z][0][x], y_offset0);
            offset[z][1][x] = lp_build_add(&bld->int_coord_bld,
                                           offset[z][1][x], y_offset1);
         }
      }
   }

   if (dims >= 3) {
      lp_build_sample_wrap_linear_int(bld,
                                      bld->format_desc->block.height,
                                      r_ipart, &r_fpart, r_float,
                                      depth_vec, z_stride,
                                      bld->static_state->pot_depth,
                                      bld->static_state->wrap_r,
                                      &z_offset0, &z_offset1,
                                      &z_subcoord[0], &z_subcoord[1]);
      for (y = 0; y < 2; y++) {
         for (x = 0; x < 2; x++) {
            offset[0][y][x] = lp_build_add(&bld->int_coord_bld,
                                           offset[0][y][x], z_offset0);
            offset[1][y][x] = lp_build_add(&bld->int_coord_bld,
                                           offset[1][y][x], z_offset1);
         }
      }
   }
   else if (bld->static_state->target == PIPE_TEXTURE_CUBE) {
      LLVMValueRef z_offset;
      z_offset = lp_build_mul(&bld->int_coord_bld, r, img_stride_vec);
      for (y = 0; y < 2; y++) {
         for (x = 0; x < 2; x++) {
            /* The r coord is the cube face in [0,5] */
            offset[0][y][x] = lp_build_add(&bld->int_coord_bld,
                                           offset[0][y][x], z_offset);
         }
      }
   }

   lp_build_sample_fetch_image_linear(bld, data_ptr, offset,
                                      x_subcoord, y_subcoord,
                                      s_fpart, t_fpart, r_fpart,
                                      colors_lo, colors_hi);
}


/**
 * Sample a single texture image with (bi-)(tri-)linear sampling.
 * Return filtered color as two vectors of 16-bit fixed point values.
 * Does address calcs (except offsets) with floats.
 * Useful for AVX which has support for 8x32 floats but not 8x32 ints.
 */
static void
lp_build_sample_image_linear_afloat(struct lp_build_sample_context *bld,
                                    LLVMValueRef int_size,
                                    LLVMValueRef row_stride_vec,
                                    LLVMValueRef img_stride_vec,
                                    LLVMValueRef data_ptr,
                                    LLVMValueRef s,
                                    LLVMValueRef t,
                                    LLVMValueRef r,
                                    LLVMValueRef *colors_lo,
                                    LLVMValueRef *colors_hi)
{
   const unsigned dims = bld->dims;
   LLVMValueRef width_vec, height_vec, depth_vec;
   LLVMValueRef s_fpart;
   LLVMValueRef t_fpart = NULL;
   LLVMValueRef r_fpart = NULL;
   LLVMValueRef x_stride, y_stride, z_stride;
   LLVMValueRef x_offset0, x_offset1;
   LLVMValueRef y_offset0, y_offset1;
   LLVMValueRef z_offset0, z_offset1;
   LLVMValueRef offset[2][2][2]; /* [z][y][x] */
   LLVMValueRef x_subcoord[2], y_subcoord[2];
   LLVMValueRef flt_size;
   LLVMValueRef x_icoord0, x_icoord1;
   LLVMValueRef y_icoord0, y_icoord1;
   LLVMValueRef z_icoord0, z_icoord1;
   unsigned x, y, z;

   flt_size = lp_build_int_to_float(&bld->float_size_bld, int_size);

   lp_build_extract_image_sizes(bld,
                                bld->float_size_type,
                                bld->coord_type,
                                flt_size,
                                &width_vec,
                                &height_vec,
                                &depth_vec);

   /* do texcoord wrapping and compute texel offsets */
   lp_build_sample_wrap_linear_float(bld,
                                     bld->format_desc->block.width,
                                     s, width_vec,
                                     bld->static_state->pot_width,
                                     bld->static_state->wrap_s,
                                     &x_icoord0, &x_icoord1,
                                     &s_fpart,
                                     bld->static_state->force_nearest_s);

   if (dims >= 2) {
      lp_build_sample_wrap_linear_float(bld,
                                        bld->format_desc->block.height,
                                        t, height_vec,
                                        bld->static_state->pot_height,
                                        bld->static_state->wrap_t,
                                        &y_icoord0, &y_icoord1,
                                        &t_fpart,
                                        bld->static_state->force_nearest_t);

      if (dims >= 3) {
         lp_build_sample_wrap_linear_float(bld,
                                           bld->format_desc->block.height,
                                           r, depth_vec,
                                           bld->static_state->pot_depth,
                                           bld->static_state->wrap_r,
                                           &z_icoord0, &z_icoord1,
                                           &r_fpart, 0);
      }
   }

   /*
    * From here on we deal with ints, and we should split up the 256bit
    * vectors manually for better generated code.
    */

   /* get pixel, row and image strides */
   x_stride = lp_build_const_vec(bld->gallivm,
                                 bld->int_coord_bld.type,
                                 bld->format_desc->block.bits/8);
   y_stride = row_stride_vec;
   z_stride = img_stride_vec;

   /*
    * compute texel offset -
    * cannot do offset calc with floats, difficult for block-based formats,
    * and not enough precision anyway.
    */
   lp_build_sample_partial_offset(&bld->int_coord_bld,
                                  bld->format_desc->block.width,
                                  x_icoord0, x_stride,
                                  &x_offset0, &x_subcoord[0]);
   lp_build_sample_partial_offset(&bld->int_coord_bld,
                                  bld->format_desc->block.width,
                                  x_icoord1, x_stride,
                                  &x_offset1, &x_subcoord[1]);
   for (z = 0; z < 2; z++) {
      for (y = 0; y < 2; y++) {
         offset[z][y][0] = x_offset0;
         offset[z][y][1] = x_offset1;
      }
   }

   if (dims >= 2) {
      lp_build_sample_partial_offset(&bld->int_coord_bld,
                                     bld->format_desc->block.height,
                                     y_icoord0, y_stride,
                                     &y_offset0, &y_subcoord[0]);
      lp_build_sample_partial_offset(&bld->int_coord_bld,
                                     bld->format_desc->block.height,
                                     y_icoord1, y_stride,
                                     &y_offset1, &y_subcoord[1]);
      for (z = 0; z < 2; z++) {
         for (x = 0; x < 2; x++) {
            offset[z][0][x] = lp_build_add(&bld->int_coord_bld,
                                           offset[z][0][x], y_offset0);
            offset[z][1][x] = lp_build_add(&bld->int_coord_bld,
                                           offset[z][1][x], y_offset1);
         }
      }
   }

   if (dims >= 3) {
      LLVMValueRef z_subcoord[2];
      lp_build_sample_partial_offset(&bld->int_coord_bld,
                                     1,
                                     z_icoord0, z_stride,
                                     &z_offset0, &z_subcoord[0]);
      lp_build_sample_partial_offset(&bld->int_coord_bld,
                                     1,
                                     z_icoord1, z_stride,
                                     &z_offset1, &z_subcoord[1]);
      for (y = 0; y < 2; y++) {
         for (x = 0; x < 2; x++) {
            offset[0][y][x] = lp_build_add(&bld->int_coord_bld,
                                           offset[0][y][x], z_offset0);
            offset[1][y][x] = lp_build_add(&bld->int_coord_bld,
                                           offset[1][y][x], z_offset1);
         }
      }
   }
   else if (bld->static_state->target == PIPE_TEXTURE_CUBE) {
      LLVMValueRef z_offset;
      z_offset = lp_build_mul(&bld->int_coord_bld, r, img_stride_vec);
      for (y = 0; y < 2; y++) {
         for (x = 0; x < 2; x++) {
            /* The r coord is the cube face in [0,5] */
            offset[0][y][x] = lp_build_add(&bld->int_coord_bld,
                                           offset[0][y][x], z_offset);
         }
      }
   }

   lp_build_sample_fetch_image_linear(bld, data_ptr, offset,
                                      x_subcoord, y_subcoord,
                                      s_fpart, t_fpart, r_fpart,
                                      colors_lo, colors_hi);
}


/**
 * Sample the texture/mipmap using given image filter and mip filter.
 * data0_ptr and data1_ptr point to the two mipmap levels to sample
 * from.  width0/1_vec, height0/1_vec, depth0/1_vec indicate their sizes.
 * If we're using nearest miplevel sampling the '1' values will be null/unused.
 */
static void
lp_build_sample_mipmap(struct lp_build_sample_context *bld,
                       unsigned img_filter,
                       unsigned mip_filter,
                       LLVMValueRef s,
                       LLVMValueRef t,
                       LLVMValueRef r,
                       LLVMValueRef ilevel0,
                       LLVMValueRef ilevel1,
                       LLVMValueRef lod_fpart,
                       LLVMValueRef colors_lo_var,
                       LLVMValueRef colors_hi_var)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef size0;
   LLVMValueRef size1;
   LLVMValueRef row_stride0_vec = NULL;
   LLVMValueRef row_stride1_vec = NULL;
   LLVMValueRef img_stride0_vec = NULL;
   LLVMValueRef img_stride1_vec = NULL;
   LLVMValueRef data_ptr0;
   LLVMValueRef data_ptr1;
   LLVMValueRef colors0_lo, colors0_hi;
   LLVMValueRef colors1_lo, colors1_hi;

   /* sample the first mipmap level */
   lp_build_mipmap_level_sizes(bld, ilevel0,
                               &size0,
                               &row_stride0_vec, &img_stride0_vec);
   data_ptr0 = lp_build_get_mipmap_level(bld, ilevel0);
   if (util_cpu_caps.has_avx && bld->coord_type.length > 4) {
      if (img_filter == PIPE_TEX_FILTER_NEAREST) {
         lp_build_sample_image_nearest_afloat(bld,
                                              size0,
                                              row_stride0_vec, img_stride0_vec,
                                              data_ptr0, s, t, r,
                                              &colors0_lo, &colors0_hi);
      }
      else {
         assert(img_filter == PIPE_TEX_FILTER_LINEAR);
         lp_build_sample_image_linear_afloat(bld,
                                             size0,
                                             row_stride0_vec, img_stride0_vec,
                                             data_ptr0, s, t, r,
                                             &colors0_lo, &colors0_hi);
      }
   }
   else {
      if (img_filter == PIPE_TEX_FILTER_NEAREST) {
         lp_build_sample_image_nearest(bld,
                                       size0,
                                       row_stride0_vec, img_stride0_vec,
                                       data_ptr0, s, t, r,
                                       &colors0_lo, &colors0_hi);
      }
      else {
         assert(img_filter == PIPE_TEX_FILTER_LINEAR);
         lp_build_sample_image_linear(bld,
                                      size0,
                                      row_stride0_vec, img_stride0_vec,
                                      data_ptr0, s, t, r,
                                      &colors0_lo, &colors0_hi);
      }
   }

   /* Store the first level's colors in the output variables */
   LLVMBuildStore(builder, colors0_lo, colors_lo_var);
   LLVMBuildStore(builder, colors0_hi, colors_hi_var);

   if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
      LLVMValueRef h16vec_scale = lp_build_const_vec(bld->gallivm,
                                                     bld->perquadf_bld.type, 256.0);
      LLVMTypeRef i32vec_type = lp_build_vec_type(bld->gallivm, bld->perquadi_bld.type);
      struct lp_build_if_state if_ctx;
      LLVMValueRef need_lerp;
      unsigned num_quads = bld->coord_bld.type.length / 4;
      unsigned i;

      lod_fpart = LLVMBuildFMul(builder, lod_fpart, h16vec_scale, "");
      lod_fpart = LLVMBuildFPToSI(builder, lod_fpart, i32vec_type, "lod_fpart.fixed16");

      /* need_lerp = lod_fpart > 0 */
      if (num_quads == 1) {
         need_lerp = LLVMBuildICmp(builder, LLVMIntSGT,
                                   lod_fpart, bld->perquadi_bld.zero,
                                   "need_lerp");
      }
      else {
         /*
          * We'll do mip filtering if any of the quads need it.
          * It might be better to split the vectors here and only fetch/filter
          * quads which need it.
          */
         /*
          * We need to clamp lod_fpart here since we can get negative
          * values which would screw up filtering if not all
          * lod_fpart values have same sign.
          * We can however then skip the greater than comparison.
          */
         lod_fpart = lp_build_max(&bld->perquadi_bld, lod_fpart,
                                  bld->perquadi_bld.zero);
         need_lerp = lp_build_any_true_range(&bld->perquadi_bld, num_quads, lod_fpart);
      }

      lp_build_if(&if_ctx, bld->gallivm, need_lerp);
      {
         struct lp_build_context h16_bld;

         lp_build_context_init(&h16_bld, bld->gallivm, lp_type_ufixed(16, bld->vector_width));

         /* sample the second mipmap level */
         lp_build_mipmap_level_sizes(bld, ilevel1,
                                     &size1,
                                     &row_stride1_vec, &img_stride1_vec);
         data_ptr1 = lp_build_get_mipmap_level(bld, ilevel1);

         if (util_cpu_caps.has_avx && bld->coord_type.length > 4) {
            if (img_filter == PIPE_TEX_FILTER_NEAREST) {
               lp_build_sample_image_nearest_afloat(bld,
                                                    size1,
                                                    row_stride1_vec, img_stride1_vec,
                                                    data_ptr1, s, t, r,
                                                    &colors1_lo, &colors1_hi);
            }
            else {
               lp_build_sample_image_linear_afloat(bld,
                                                   size1,
                                                   row_stride1_vec, img_stride1_vec,
                                                   data_ptr1, s, t, r,
                                                   &colors1_lo, &colors1_hi);
            }
         }
         else {
            if (img_filter == PIPE_TEX_FILTER_NEAREST) {
               lp_build_sample_image_nearest(bld,
                                             size1,
                                             row_stride1_vec, img_stride1_vec,
                                             data_ptr1, s, t, r,
                                             &colors1_lo, &colors1_hi);
            }
            else {
               lp_build_sample_image_linear(bld,
                                            size1,
                                            row_stride1_vec, img_stride1_vec,
                                            data_ptr1, s, t, r,
                                            &colors1_lo, &colors1_hi);
            }
         }

         /* interpolate samples from the two mipmap levels */

         if (num_quads == 1) {
            lod_fpart = LLVMBuildTrunc(builder, lod_fpart, h16_bld.elem_type, "");
            lod_fpart = lp_build_broadcast_scalar(&h16_bld, lod_fpart);

#if HAVE_LLVM == 0x208
            /* This is a work-around for a bug in LLVM 2.8.
             * Evidently, something goes wrong in the construction of the
             * lod_fpart short[8] vector.  Adding this no-effect shuffle seems
             * to force the vector to be properly constructed.
             * Tested with mesa-demos/src/tests/mipmap_limits.c (press t, f).
             */
            {
               LLVMValueRef shuffles[8], shuffle;
               assert(h16_bld.type.length <= Elements(shuffles));
               for (i = 0; i < h16_bld.type.length; i++)
                  shuffles[i] = lp_build_const_int32(bld->gallivm, 2 * (i & 1));
               shuffle = LLVMConstVector(shuffles, h16_bld.type.length);
               lod_fpart = LLVMBuildShuffleVector(builder,
                                                  lod_fpart, lod_fpart,
                                                  shuffle, "");
            }
#endif

            colors0_lo = lp_build_lerp(&h16_bld, lod_fpart,
                                       colors0_lo, colors1_lo);
            colors0_hi = lp_build_lerp(&h16_bld, lod_fpart,
                                       colors0_hi, colors1_hi);
         }
         else {
            LLVMValueRef lod_parts[LP_MAX_VECTOR_LENGTH/16];
            struct lp_type perquadi16_type = bld->perquadi_bld.type;
            perquadi16_type.width /= 2;
            perquadi16_type.length *= 2;
            lod_fpart = LLVMBuildBitCast(builder, lod_fpart,
                                         lp_build_vec_type(bld->gallivm,
                                                           perquadi16_type), "");
            /* XXX this only works for exactly 2 quads. More quads need shuffle */
            assert(num_quads == 2);
            for (i = 0; i < num_quads; i++) {
               LLVMValueRef indexi2 = lp_build_const_int32(bld->gallivm, i*2);
               lod_parts[i] = lp_build_extract_broadcast(bld->gallivm,
                                                         perquadi16_type,
                                                         h16_bld.type,
                                                         lod_fpart,
                                                         indexi2);
            }
            colors0_lo = lp_build_lerp(&h16_bld, lod_parts[0],
                                       colors0_lo, colors1_lo);
            colors0_hi = lp_build_lerp(&h16_bld, lod_parts[1],
                                       colors0_hi, colors1_hi);
         }

         LLVMBuildStore(builder, colors0_lo, colors_lo_var);
         LLVMBuildStore(builder, colors0_hi, colors_hi_var);
      }
      lp_build_endif(&if_ctx);
   }
}



/**
 * Texture sampling in AoS format.  Used when sampling common 32-bit/texel
 * formats.  1D/2D/3D/cube texture supported.  All mipmap sampling modes
 * but only limited texture coord wrap modes.
 */
void
lp_build_sample_aos(struct lp_build_sample_context *bld,
                    unsigned unit,
                    LLVMValueRef s,
                    LLVMValueRef t,
                    LLVMValueRef r,
                    LLVMValueRef lod_ipart,
                    LLVMValueRef lod_fpart,
                    LLVMValueRef ilevel0,
                    LLVMValueRef ilevel1,
                    LLVMValueRef texel_out[4])
{
   struct lp_build_context *int_bld = &bld->int_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   const unsigned mip_filter = bld->static_state->min_mip_filter;
   const unsigned min_filter = bld->static_state->min_img_filter;
   const unsigned mag_filter = bld->static_state->mag_img_filter;
   const unsigned dims = bld->dims;
   LLVMValueRef packed, packed_lo, packed_hi;
   LLVMValueRef unswizzled[4];
   struct lp_build_context h16_bld;

   /* we only support the common/simple wrap modes at this time */
   assert(lp_is_simple_wrap_mode(bld->static_state->wrap_s));
   if (dims >= 2)
      assert(lp_is_simple_wrap_mode(bld->static_state->wrap_t));
   if (dims >= 3)
      assert(lp_is_simple_wrap_mode(bld->static_state->wrap_r));


   /* make 16-bit fixed-pt builder context */
   lp_build_context_init(&h16_bld, bld->gallivm, lp_type_ufixed(16, bld->vector_width));

   /*
    * Get/interpolate texture colors.
    */

   packed_lo = lp_build_alloca(bld->gallivm, h16_bld.vec_type, "packed_lo");
   packed_hi = lp_build_alloca(bld->gallivm, h16_bld.vec_type, "packed_hi");

   if (min_filter == mag_filter) {
      /* no need to distinguish between minification and magnification */
      lp_build_sample_mipmap(bld,
                             min_filter, mip_filter,
                             s, t, r,
                             ilevel0, ilevel1, lod_fpart,
                             packed_lo, packed_hi);
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
         lp_build_sample_mipmap(bld,
                                min_filter, mip_filter,
                                s, t, r,
                                ilevel0, ilevel1, lod_fpart,
                                packed_lo, packed_hi);
      }
      lp_build_else(&if_ctx);
      {
         /* Use the magnification filter */
         lp_build_sample_mipmap(bld, 
                                mag_filter, PIPE_TEX_MIPFILTER_NONE,
                                s, t, r,
                                ilevel0, NULL, NULL,
                                packed_lo, packed_hi);
      }
      lp_build_endif(&if_ctx);
   }

   /*
    * combine the values stored in 'packed_lo' and 'packed_hi' variables
    * into 'packed'
    */
   packed = lp_build_pack2(bld->gallivm,
                           h16_bld.type, lp_type_unorm(8, bld->vector_width),
                           LLVMBuildLoad(builder, packed_lo, ""),
                           LLVMBuildLoad(builder, packed_hi, ""));

   /*
    * Convert to SoA and swizzle.
    */
   lp_build_rgba8_to_f32_soa(bld->gallivm,
                             bld->texel_type,
                             packed, unswizzled);

   if (util_format_is_rgba8_variant(bld->format_desc)) {
      lp_build_format_swizzle_soa(bld->format_desc,
                                  &bld->texel_bld,
                                  unswizzled, texel_out);
   }
   else {
      texel_out[0] = unswizzled[0];
      texel_out[1] = unswizzled[1];
      texel_out[2] = unswizzled[2];
      texel_out[3] = unswizzled[3];
   }
}
