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


#include "pipe/p_defines.h"

#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_string.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_conv.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_gather.h"
#include "lp_bld_debug.h"
#include "lp_bld_format.h"


void
lp_build_format_swizzle_soa(const struct util_format_description *format_desc,
                            struct lp_build_context *bld,
                            const LLVMValueRef *unswizzled,
                            LLVMValueRef swizzled_out[4])
{
   assert(UTIL_FORMAT_SWIZZLE_0 == PIPE_SWIZZLE_ZERO);
   assert(UTIL_FORMAT_SWIZZLE_1 == PIPE_SWIZZLE_ONE);

   if (format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) {
      /*
       * Return zzz1 for depth-stencil formats.
       *
       * XXX: Allow to control the depth swizzle with an additional parameter,
       * as the caller may wish another depth swizzle, or retain the stencil
       * value.
       */
      enum util_format_swizzle swizzle = format_desc->swizzle[0];
      LLVMValueRef depth = lp_build_swizzle_soa_channel(bld, unswizzled, swizzle);
      swizzled_out[2] = swizzled_out[1] = swizzled_out[0] = depth;
      swizzled_out[3] = bld->one;
   }
   else {
      unsigned chan;
      for (chan = 0; chan < 4; ++chan) {
         enum util_format_swizzle swizzle = format_desc->swizzle[chan];
         swizzled_out[chan] = lp_build_swizzle_soa_channel(bld, unswizzled, swizzle);
      }
   }
}


/**
 * Unpack several pixels in SoA.
 *
 * It takes a vector of packed pixels:
 *
 *   packed = {P0, P1, P2, P3, ..., Pn}
 *
 * And will produce four vectors:
 *
 *   red    = {R0, R1, R2, R3, ..., Rn}
 *   green  = {G0, G1, G2, G3, ..., Gn}
 *   blue   = {B0, B1, B2, B3, ..., Bn}
 *   alpha  = {A0, A1, A2, A3, ..., An}
 *
 * It requires that a packed pixel fits into an element of the output
 * channels. The common case is when converting pixel with a depth of 32 bit or
 * less into floats.
 *
 * \param format_desc  the format of the 'packed' incoming pixel vector
 * \param type  the desired type for rgba_out (type.length = n, above)
 * \param packed  the incoming vector of packed pixels
 * \param rgba_out  returns the SoA R,G,B,A vectors
 */
void
lp_build_unpack_rgba_soa(struct gallivm_state *gallivm,
                         const struct util_format_description *format_desc,
                         struct lp_type type,
                         LLVMValueRef packed,
                         LLVMValueRef rgba_out[4])
{
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context bld;
   LLVMValueRef inputs[4];
   unsigned start;
   unsigned chan;

   assert(format_desc->layout == UTIL_FORMAT_LAYOUT_PLAIN);
   assert(format_desc->block.width == 1);
   assert(format_desc->block.height == 1);
   assert(format_desc->block.bits <= type.width);
   /* FIXME: Support more output types */
   assert(type.floating);
   assert(type.width == 32);

   lp_build_context_init(&bld, gallivm, type);

   /* Decode the input vector components */
   start = 0;
   for (chan = 0; chan < format_desc->nr_channels; ++chan) {
      const unsigned width = format_desc->channel[chan].size;
      const unsigned stop = start + width;
      LLVMValueRef input;

      input = packed;

      switch(format_desc->channel[chan].type) {
      case UTIL_FORMAT_TYPE_VOID:
         input = lp_build_undef(gallivm, type);
         break;

      case UTIL_FORMAT_TYPE_UNSIGNED:
         /*
          * Align the LSB
          */

         if (start) {
            input = LLVMBuildLShr(builder, input, lp_build_const_int_vec(gallivm, type, start), "");
         }

         /*
          * Zero the MSBs
          */

         if (stop < format_desc->block.bits) {
            unsigned mask = ((unsigned long long)1 << width) - 1;
            input = LLVMBuildAnd(builder, input, lp_build_const_int_vec(gallivm, type, mask), "");
         }

         /*
          * Type conversion
          */

         if (type.floating) {
            if(format_desc->channel[chan].normalized)
               input = lp_build_unsigned_norm_to_float(gallivm, width, type, input);
            else
               input = LLVMBuildSIToFP(builder, input,
                                       lp_build_vec_type(gallivm, type), "");
         }
         else {
            /* FIXME */
            assert(0);
            input = lp_build_undef(gallivm, type);
         }

         break;

      case UTIL_FORMAT_TYPE_SIGNED:
         /*
          * Align the sign bit first.
          */

         if (stop < type.width) {
            unsigned bits = type.width - stop;
            LLVMValueRef bits_val = lp_build_const_int_vec(gallivm, type, bits);
            input = LLVMBuildShl(builder, input, bits_val, "");
         }

         /*
          * Align the LSB (with an arithmetic shift to preserve the sign)
          */

         if (format_desc->channel[chan].size < type.width) {
            unsigned bits = type.width - format_desc->channel[chan].size;
            LLVMValueRef bits_val = lp_build_const_int_vec(gallivm, type, bits);
            input = LLVMBuildAShr(builder, input, bits_val, "");
         }

         /*
          * Type conversion
          */

         if (type.floating) {
            input = LLVMBuildSIToFP(builder, input, lp_build_vec_type(gallivm, type), "");
            if (format_desc->channel[chan].normalized) {
               double scale = 1.0 / ((1 << (format_desc->channel[chan].size - 1)) - 1);
               LLVMValueRef scale_val = lp_build_const_vec(gallivm, type, scale);
               input = LLVMBuildFMul(builder, input, scale_val, "");
            }
         }
         else {
            /* FIXME */
            assert(0);
            input = lp_build_undef(gallivm, type);
         }

         break;

      case UTIL_FORMAT_TYPE_FLOAT:
         if (type.floating) {
            assert(start == 0);
            assert(stop == 32);
            assert(type.width == 32);
            input = LLVMBuildBitCast(builder, input, lp_build_vec_type(gallivm, type), "");
         }
         else {
            /* FIXME */
            assert(0);
            input = lp_build_undef(gallivm, type);
         }
         break;

      case UTIL_FORMAT_TYPE_FIXED:
         if (type.floating) {
            double scale = 1.0 / ((1 << (format_desc->channel[chan].size/2)) - 1);
            LLVMValueRef scale_val = lp_build_const_vec(gallivm, type, scale);
            input = LLVMBuildSIToFP(builder, input, lp_build_vec_type(gallivm, type), "");
            input = LLVMBuildFMul(builder, input, scale_val, "");
         }
         else {
            /* FIXME */
            assert(0);
            input = lp_build_undef(gallivm, type);
         }
         break;

      default:
         assert(0);
         input = lp_build_undef(gallivm, type);
         break;
      }

      inputs[chan] = input;

      start = stop;
   }

   lp_build_format_swizzle_soa(format_desc, &bld, inputs, rgba_out);
}


void
lp_build_rgba8_to_f32_soa(struct gallivm_state *gallivm,
                          struct lp_type dst_type,
                          LLVMValueRef packed,
                          LLVMValueRef *rgba)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef mask = lp_build_const_int_vec(gallivm, dst_type, 0xff);
   unsigned chan;

   packed = LLVMBuildBitCast(builder, packed,
                             lp_build_int_vec_type(gallivm, dst_type), "");

   /* Decode the input vector components */
   for (chan = 0; chan < 4; ++chan) {
      unsigned start = chan*8;
      unsigned stop = start + 8;
      LLVMValueRef input;

      input = packed;

      if (start)
         input = LLVMBuildLShr(builder, input,
                               lp_build_const_int_vec(gallivm, dst_type, start), "");

      if (stop < 32)
         input = LLVMBuildAnd(builder, input, mask, "");

      input = lp_build_unsigned_norm_to_float(gallivm, 8, dst_type, input);

      rgba[chan] = input;
   }
}



/**
 * Fetch a texels from a texture, returning them in SoA layout.
 *
 * \param type  the desired return type for 'rgba'.  The vector length
 *              is the number of texels to fetch
 *
 * \param base_ptr  points to start of the texture image block.  For non-
 *                  compressed formats, this simply points to the texel.
 *                  For compressed formats, it points to the start of the
 *                  compressed data block.
 *
 * \param i, j  the sub-block pixel coordinates.  For non-compressed formats
 *              these will always be (0,0).  For compressed formats, i will
 *              be in [0, block_width-1] and j will be in [0, block_height-1].
 */
void
lp_build_fetch_rgba_soa(struct gallivm_state *gallivm,
                        const struct util_format_description *format_desc,
                        struct lp_type type,
                        LLVMValueRef base_ptr,
                        LLVMValueRef offset,
                        LLVMValueRef i,
                        LLVMValueRef j,
                        LLVMValueRef rgba_out[4])
{
   LLVMBuilderRef builder = gallivm->builder;

   if (format_desc->layout == UTIL_FORMAT_LAYOUT_PLAIN &&
       (format_desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB ||
        format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) &&
       format_desc->block.width == 1 &&
       format_desc->block.height == 1 &&
       format_desc->block.bits <= type.width &&
       (format_desc->channel[0].type != UTIL_FORMAT_TYPE_FLOAT ||
        format_desc->channel[0].size == 32))
   {
      /*
       * The packed pixel fits into an element of the destination format. Put
       * the packed pixels into a vector and extract each component for all
       * vector elements in parallel.
       */

      LLVMValueRef packed;

      /*
       * gather the texels from the texture
       * Ex: packed = {BGRA, BGRA, BGRA, BGRA}.
       */
      packed = lp_build_gather(gallivm,
                               type.length,
                               format_desc->block.bits,
                               type.width,
                               base_ptr, offset);

      /*
       * convert texels to float rgba
       */
      lp_build_unpack_rgba_soa(gallivm,
                               format_desc,
                               type,
                               packed, rgba_out);
      return;
   }

   /*
    * Try calling lp_build_fetch_rgba_aos for all pixels.
    */

   if (util_format_fits_8unorm(format_desc) &&
       type.floating && type.width == 32 &&
       (type.length == 1 || (type.length % 4 == 0))) {
      struct lp_type tmp_type;
      LLVMValueRef tmp;

      memset(&tmp_type, 0, sizeof tmp_type);
      tmp_type.width = 8;
      tmp_type.length = type.length * 4;
      tmp_type.norm = TRUE;

      tmp = lp_build_fetch_rgba_aos(gallivm, format_desc, tmp_type,
                                    base_ptr, offset, i, j);

      lp_build_rgba8_to_f32_soa(gallivm,
                                type,
                                tmp,
                                rgba_out);

      return;
   }

   /*
    * Fallback to calling lp_build_fetch_rgba_aos for each pixel.
    *
    * This is not the most efficient way of fetching pixels, as we
    * miss some opportunities to do vectorization, but this is
    * convenient for formats or scenarios for which there was no
    * opportunity or incentive to optimize.
    */

   {
      unsigned k, chan;
      struct lp_type tmp_type;

      if (gallivm_debug & GALLIVM_DEBUG_PERF) {
         debug_printf("%s: scalar unpacking of %s\n",
                      __FUNCTION__, format_desc->short_name);
      }

      tmp_type = type;
      tmp_type.length = 4;

      for (chan = 0; chan < 4; ++chan) {
         rgba_out[chan] = lp_build_undef(gallivm, type);
      }

      /* loop over number of pixels */
      for(k = 0; k < type.length; ++k) {
         LLVMValueRef index = lp_build_const_int32(gallivm, k);
         LLVMValueRef offset_elem;
         LLVMValueRef i_elem, j_elem;
         LLVMValueRef tmp;

         offset_elem = LLVMBuildExtractElement(builder, offset,
                                               index, "");

         i_elem = LLVMBuildExtractElement(builder, i, index, "");
         j_elem = LLVMBuildExtractElement(builder, j, index, "");

         /* Get a single float[4]={R,G,B,A} pixel */
         tmp = lp_build_fetch_rgba_aos(gallivm, format_desc, tmp_type,
                                       base_ptr, offset_elem,
                                       i_elem, j_elem);

         /*
          * Insert the AoS tmp value channels into the SoA result vectors at
          * position = 'index'.
          */
         for (chan = 0; chan < 4; ++chan) {
            LLVMValueRef chan_val = lp_build_const_int32(gallivm, chan),
            tmp_chan = LLVMBuildExtractElement(builder, tmp, chan_val, "");
            rgba_out[chan] = LLVMBuildInsertElement(builder, rgba_out[chan],
                                                    tmp_chan, index, "");
         }
      }
   }
}
