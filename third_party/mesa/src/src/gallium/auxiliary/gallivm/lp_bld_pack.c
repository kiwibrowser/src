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
 * Helper functions for packing/unpacking.
 *
 * Pack/unpacking is necessary for conversion between types of different
 * bit width.
 *
 * They are also commonly used when an computation needs higher
 * precision for the intermediate values. For example, if one needs the
 * function:
 *
 *   c = compute(a, b);
 *
 * to use more precision for intermediate results then one should implement it
 * as:
 *
 *   LLVMValueRef
 *   compute(LLVMBuilderRef builder struct lp_type type, LLVMValueRef a, LLVMValueRef b)
 *   {
 *      struct lp_type wide_type = lp_wider_type(type);
 *      LLVMValueRef al, ah, bl, bh, cl, ch, c;
 *
 *      lp_build_unpack2(builder, type, wide_type, a, &al, &ah);
 *      lp_build_unpack2(builder, type, wide_type, b, &bl, &bh);
 *
 *      cl = compute_half(al, bl);
 *      ch = compute_half(ah, bh);
 *
 *      c = lp_build_pack2(bld->builder, wide_type, type, cl, ch);
 *
 *      return c;
 *   }
 *
 * where compute_half() would do the computation for half the elements with
 * twice the precision.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_debug.h"
#include "util/u_math.h"
#include "util/u_cpu_detect.h"
#include "util/u_memory.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_init.h"
#include "lp_bld_intr.h"
#include "lp_bld_arit.h"
#include "lp_bld_pack.h"
#include "lp_bld_swizzle.h"


/**
 * Build shuffle vectors that match PUNPCKLxx and PUNPCKHxx instructions.
 */
static LLVMValueRef
lp_build_const_unpack_shuffle(struct gallivm_state *gallivm,
                              unsigned n, unsigned lo_hi)
{
   LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   unsigned i, j;

   assert(n <= LP_MAX_VECTOR_LENGTH);
   assert(lo_hi < 2);

   /* TODO: cache results in a static table */

   for(i = 0, j = lo_hi*n/2; i < n; i += 2, ++j) {
      elems[i + 0] = lp_build_const_int32(gallivm, 0 + j);
      elems[i + 1] = lp_build_const_int32(gallivm, n + j);
   }

   return LLVMConstVector(elems, n);
}

/**
 * Similar to lp_build_const_unpack_shuffle but for special AVX 256bit unpack.
 * See comment above lp_build_interleave2_half for more details.
 */
static LLVMValueRef
lp_build_const_unpack_shuffle_half(struct gallivm_state *gallivm,
                                   unsigned n, unsigned lo_hi)
{
   LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   unsigned i, j;

   assert(n <= LP_MAX_VECTOR_LENGTH);
   assert(lo_hi < 2);

   for (i = 0, j = lo_hi*(n/4); i < n; i += 2, ++j) {
      if (i == (n / 2))
         j += n / 4;

      elems[i + 0] = lp_build_const_int32(gallivm, 0 + j);
      elems[i + 1] = lp_build_const_int32(gallivm, n + j);
   }

   return LLVMConstVector(elems, n);
}

/**
 * Build shuffle vectors that match PACKxx instructions.
 */
static LLVMValueRef
lp_build_const_pack_shuffle(struct gallivm_state *gallivm, unsigned n)
{
   LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   unsigned i;

   assert(n <= LP_MAX_VECTOR_LENGTH);

   for(i = 0; i < n; ++i)
      elems[i] = lp_build_const_int32(gallivm, 2*i);

   return LLVMConstVector(elems, n);
}

/**
 * Return a vector with elements src[start:start+size]
 * Most useful for getting half the values out of a 256bit sized vector,
 * otherwise may cause data rearrangement to happen.
 */
LLVMValueRef
lp_build_extract_range(struct gallivm_state *gallivm,
                       LLVMValueRef src,
                       unsigned start,
                       unsigned size)
{
   LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   unsigned i;

   assert(size <= Elements(elems));

   for (i = 0; i < size; ++i)
      elems[i] = lp_build_const_int32(gallivm, i + start);

   if (size == 1) {
      return LLVMBuildExtractElement(gallivm->builder, src, elems[0], "");
   }
   else {
      return LLVMBuildShuffleVector(gallivm->builder, src, src,
                                    LLVMConstVector(elems, size), "");
   }
}

/**
 * Concatenates several (must be a power of 2) vectors (of same type)
 * into a larger one.
 * Most useful for building up a 256bit sized vector out of two 128bit ones.
 */
LLVMValueRef
lp_build_concat(struct gallivm_state *gallivm,
                LLVMValueRef src[],
                struct lp_type src_type,
                unsigned num_vectors)
{
   unsigned new_length, i;
   LLVMValueRef tmp[LP_MAX_VECTOR_LENGTH/2];
   LLVMValueRef shuffles[LP_MAX_VECTOR_LENGTH];

   assert(src_type.length * num_vectors <= Elements(shuffles));
   assert(util_is_power_of_two(num_vectors));

   new_length = src_type.length;

   for (i = 0; i < num_vectors; i++)
      tmp[i] = src[i];

   while (num_vectors > 1) {
      num_vectors >>= 1;
      new_length <<= 1;
      for (i = 0; i < new_length; i++) {
         shuffles[i] = lp_build_const_int32(gallivm, i);
      }
      for (i = 0; i < num_vectors; i++) {
         tmp[i] = LLVMBuildShuffleVector(gallivm->builder, tmp[i*2], tmp[i*2 + 1],
                                         LLVMConstVector(shuffles, new_length), "");
      }
   }

   return tmp[0];
}

/**
 * Interleave vector elements.
 *
 * Matches the PUNPCKLxx and PUNPCKHxx SSE instructions.
 */
LLVMValueRef
lp_build_interleave2(struct gallivm_state *gallivm,
                     struct lp_type type,
                     LLVMValueRef a,
                     LLVMValueRef b,
                     unsigned lo_hi)
{
   LLVMValueRef shuffle;

   shuffle = lp_build_const_unpack_shuffle(gallivm, type.length, lo_hi);

   return LLVMBuildShuffleVector(gallivm->builder, a, b, shuffle, "");
}

/**
 * Interleave vector elements but with 256 bit,
 * treats it as interleave with 2 concatenated 128 bit vectors.
 *
 * This differs to lp_build_interleave2 as that function would do the following (for lo):
 * a0 b0 a1 b1 a2 b2 a3 b3, and this does not compile into an AVX unpack instruction.
 *
 *
 * An example interleave 8x float with 8x float on AVX 256bit unpack:
 *   a0 a1 a2 a3 a4 a5 a6 a7 <-> b0 b1 b2 b3 b4 b5 b6 b7
 *
 * Equivalent to interleaving 2x 128 bit vectors
 *   a0 a1 a2 a3 <-> b0 b1 b2 b3 concatenated with a4 a5 a6 a7 <-> b4 b5 b6 b7
 *
 * So interleave-lo would result in:
 *   a0 b0 a1 b1 a4 b4 a5 b5
 *
 * And interleave-hi would result in:
 *   a2 b2 a3 b3 a6 b6 a7 b7
 */
LLVMValueRef
lp_build_interleave2_half(struct gallivm_state *gallivm,
                     struct lp_type type,
                     LLVMValueRef a,
                     LLVMValueRef b,
                     unsigned lo_hi)
{
   if (type.length * type.width == 256) {
      LLVMValueRef shuffle = lp_build_const_unpack_shuffle_half(gallivm, type.length, lo_hi);
      return LLVMBuildShuffleVector(gallivm->builder, a, b, shuffle, "");
   } else {
      return lp_build_interleave2(gallivm, type, a, b, lo_hi);
   }
}

/**
 * Double the bit width.
 *
 * This will only change the number of bits the values are represented, not the
 * values themselves.
 */
void
lp_build_unpack2(struct gallivm_state *gallivm,
                 struct lp_type src_type,
                 struct lp_type dst_type,
                 LLVMValueRef src,
                 LLVMValueRef *dst_lo,
                 LLVMValueRef *dst_hi)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef msb;
   LLVMTypeRef dst_vec_type;

   assert(!src_type.floating);
   assert(!dst_type.floating);
   assert(dst_type.width == src_type.width * 2);
   assert(dst_type.length * 2 == src_type.length);

   if(dst_type.sign && src_type.sign) {
      /* Replicate the sign bit in the most significant bits */
      msb = LLVMBuildAShr(builder, src, lp_build_const_int_vec(gallivm, src_type, src_type.width - 1), "");
   }
   else
      /* Most significant bits always zero */
      msb = lp_build_zero(gallivm, src_type);

   /* Interleave bits */
#ifdef PIPE_ARCH_LITTLE_ENDIAN
   *dst_lo = lp_build_interleave2(gallivm, src_type, src, msb, 0);
   *dst_hi = lp_build_interleave2(gallivm, src_type, src, msb, 1);
#else
   *dst_lo = lp_build_interleave2(gallivm, src_type, msb, src, 0);
   *dst_hi = lp_build_interleave2(gallivm, src_type, msb, src, 1);
#endif

   /* Cast the result into the new type (twice as wide) */

   dst_vec_type = lp_build_vec_type(gallivm, dst_type);

   *dst_lo = LLVMBuildBitCast(builder, *dst_lo, dst_vec_type, "");
   *dst_hi = LLVMBuildBitCast(builder, *dst_hi, dst_vec_type, "");
}


/**
 * Expand the bit width.
 *
 * This will only change the number of bits the values are represented, not the
 * values themselves.
 */
void
lp_build_unpack(struct gallivm_state *gallivm,
                struct lp_type src_type,
                struct lp_type dst_type,
                LLVMValueRef src,
                LLVMValueRef *dst, unsigned num_dsts)
{
   unsigned num_tmps;
   unsigned i;

   /* Register width must remain constant */
   assert(src_type.width * src_type.length == dst_type.width * dst_type.length);

   /* We must not loose or gain channels. Only precision */
   assert(src_type.length == dst_type.length * num_dsts);

   num_tmps = 1;
   dst[0] = src;

   while(src_type.width < dst_type.width) {
      struct lp_type tmp_type = src_type;

      tmp_type.width *= 2;
      tmp_type.length /= 2;

      for(i = num_tmps; i--; ) {
         lp_build_unpack2(gallivm, src_type, tmp_type, dst[i], &dst[2*i + 0], &dst[2*i + 1]);
      }

      src_type = tmp_type;

      num_tmps *= 2;
   }

   assert(num_tmps == num_dsts);
}


/**
 * Non-interleaved pack.
 *
 * This will move values as
 *         (LSB)                     (MSB)
 *   lo =   l0 __ l1 __ l2 __..  __ ln __
 *   hi =   h0 __ h1 __ h2 __..  __ hn __
 *   res =  l0 l1 l2 .. ln h0 h1 h2 .. hn
 *
 * This will only change the number of bits the values are represented, not the
 * values themselves.
 *
 * It is assumed the values are already clamped into the destination type range.
 * Values outside that range will produce undefined results. Use
 * lp_build_packs2 instead.
 */
LLVMValueRef
lp_build_pack2(struct gallivm_state *gallivm,
               struct lp_type src_type,
               struct lp_type dst_type,
               LLVMValueRef lo,
               LLVMValueRef hi)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef dst_vec_type = lp_build_vec_type(gallivm, dst_type);
   LLVMValueRef shuffle;
   LLVMValueRef res = NULL;
   struct lp_type intr_type = dst_type;

#if HAVE_LLVM < 0x0207
   intr_type = src_type;
#endif

   assert(!src_type.floating);
   assert(!dst_type.floating);
   assert(src_type.width == dst_type.width * 2);
   assert(src_type.length * 2 == dst_type.length);

   /* Check for special cases first */
   if(util_cpu_caps.has_sse2 && src_type.width * src_type.length >= 128) {
      const char *intrinsic = NULL;

      switch(src_type.width) {
      case 32:
         if(dst_type.sign) {
            intrinsic = "llvm.x86.sse2.packssdw.128";
         }
         else {
            if (util_cpu_caps.has_sse4_1) {
               intrinsic = "llvm.x86.sse41.packusdw";
#if HAVE_LLVM < 0x0207
               /* llvm < 2.7 has inconsistent signatures except for packusdw */
               intr_type = dst_type;
#endif
            }
         }
         break;
      case 16:
         if (dst_type.sign) {
            intrinsic = "llvm.x86.sse2.packsswb.128";
         }
         else {
            intrinsic = "llvm.x86.sse2.packuswb.128";
         }
         break;
      /* default uses generic shuffle below */
      }
      if (intrinsic) {
         if (src_type.width * src_type.length == 128) {
            LLVMTypeRef intr_vec_type = lp_build_vec_type(gallivm, intr_type);
            res = lp_build_intrinsic_binary(builder, intrinsic, intr_vec_type, lo, hi);
            if (dst_vec_type != intr_vec_type) {
               res = LLVMBuildBitCast(builder, res, dst_vec_type, "");
            }
         }
         else {
            int num_split = src_type.width * src_type.length / 128;
            int i;
            int nlen = 128 / src_type.width;
            struct lp_type ndst_type = lp_type_unorm(dst_type.width, 128);
            struct lp_type nintr_type = lp_type_unorm(intr_type.width, 128);
            LLVMValueRef tmpres[LP_MAX_VECTOR_WIDTH / 128];
            LLVMValueRef tmplo, tmphi;
            LLVMTypeRef ndst_vec_type = lp_build_vec_type(gallivm, ndst_type);
            LLVMTypeRef nintr_vec_type = lp_build_vec_type(gallivm, nintr_type);

            assert(num_split <= LP_MAX_VECTOR_WIDTH / 128);

            for (i = 0; i < num_split / 2; i++) {
               tmplo = lp_build_extract_range(gallivm,
                                              lo, i*nlen*2, nlen);
               tmphi = lp_build_extract_range(gallivm,
                                              lo, i*nlen*2 + nlen, nlen);
               tmpres[i] = lp_build_intrinsic_binary(builder, intrinsic,
                                                     nintr_vec_type, tmplo, tmphi);
               if (ndst_vec_type != nintr_vec_type) {
                  tmpres[i] = LLVMBuildBitCast(builder, tmpres[i], ndst_vec_type, "");
               }
            }
            for (i = 0; i < num_split / 2; i++) {
               tmplo = lp_build_extract_range(gallivm,
                                              hi, i*nlen*2, nlen);
               tmphi = lp_build_extract_range(gallivm,
                                              hi, i*nlen*2 + nlen, nlen);
               tmpres[i+num_split/2] = lp_build_intrinsic_binary(builder, intrinsic,
                                                                 nintr_vec_type,
                                                                 tmplo, tmphi);
               if (ndst_vec_type != nintr_vec_type) {
                  tmpres[i+num_split/2] = LLVMBuildBitCast(builder, tmpres[i+num_split/2],
                                                           ndst_vec_type, "");
               }
            }
            res = lp_build_concat(gallivm, tmpres, ndst_type, num_split);
         }
         return res;
      }
   }

   /* generic shuffle */
   lo = LLVMBuildBitCast(builder, lo, dst_vec_type, "");
   hi = LLVMBuildBitCast(builder, hi, dst_vec_type, "");

   shuffle = lp_build_const_pack_shuffle(gallivm, dst_type.length);

   res = LLVMBuildShuffleVector(builder, lo, hi, shuffle, "");

   return res;
}



/**
 * Non-interleaved pack and saturate.
 *
 * Same as lp_build_pack2 but will saturate values so that they fit into the
 * destination type.
 */
LLVMValueRef
lp_build_packs2(struct gallivm_state *gallivm,
                struct lp_type src_type,
                struct lp_type dst_type,
                LLVMValueRef lo,
                LLVMValueRef hi)
{
   boolean clamp;

   assert(!src_type.floating);
   assert(!dst_type.floating);
   assert(src_type.sign == dst_type.sign);
   assert(src_type.width == dst_type.width * 2);
   assert(src_type.length * 2 == dst_type.length);

   clamp = TRUE;

   /* All X86 SSE non-interleaved pack instructions take signed inputs and
    * saturate them, so no need to clamp for those cases. */
   if(util_cpu_caps.has_sse2 &&
      src_type.width * src_type.length >= 128 &&
      src_type.sign &&
      (src_type.width == 32 || src_type.width == 16))
      clamp = FALSE;

   if(clamp) {
      struct lp_build_context bld;
      unsigned dst_bits = dst_type.sign ? dst_type.width - 1 : dst_type.width;
      LLVMValueRef dst_max = lp_build_const_int_vec(gallivm, src_type, ((unsigned long long)1 << dst_bits) - 1);
      lp_build_context_init(&bld, gallivm, src_type);
      lo = lp_build_min(&bld, lo, dst_max);
      hi = lp_build_min(&bld, hi, dst_max);
      /* FIXME: What about lower bound? */
   }

   return lp_build_pack2(gallivm, src_type, dst_type, lo, hi);
}


/**
 * Truncate the bit width.
 *
 * TODO: Handle saturation consistently.
 */
LLVMValueRef
lp_build_pack(struct gallivm_state *gallivm,
              struct lp_type src_type,
              struct lp_type dst_type,
              boolean clamped,
              const LLVMValueRef *src, unsigned num_srcs)
{
   LLVMValueRef (*pack2)(struct gallivm_state *gallivm,
                         struct lp_type src_type,
                         struct lp_type dst_type,
                         LLVMValueRef lo,
                         LLVMValueRef hi);
   LLVMValueRef tmp[LP_MAX_VECTOR_LENGTH];
   unsigned i;

   /* Register width must remain constant */
   assert(src_type.width * src_type.length == dst_type.width * dst_type.length);

   /* We must not loose or gain channels. Only precision */
   assert(src_type.length * num_srcs == dst_type.length);

   if(clamped)
      pack2 = &lp_build_pack2;
   else
      pack2 = &lp_build_packs2;

   for(i = 0; i < num_srcs; ++i)
      tmp[i] = src[i];

   while(src_type.width > dst_type.width) {
      struct lp_type tmp_type = src_type;

      tmp_type.width /= 2;
      tmp_type.length *= 2;

      /* Take in consideration the sign changes only in the last step */
      if(tmp_type.width == dst_type.width)
         tmp_type.sign = dst_type.sign;

      num_srcs /= 2;

      for(i = 0; i < num_srcs; ++i)
         tmp[i] = pack2(gallivm, src_type, tmp_type,
                        tmp[2*i + 0], tmp[2*i + 1]);

      src_type = tmp_type;
   }

   assert(num_srcs == 1);

   return tmp[0];
}


/**
 * Truncate or expand the bitwidth.
 *
 * NOTE: Getting the right sign flags is crucial here, as we employ some
 * intrinsics that do saturation.
 */
void
lp_build_resize(struct gallivm_state *gallivm,
                struct lp_type src_type,
                struct lp_type dst_type,
                const LLVMValueRef *src, unsigned num_srcs,
                LLVMValueRef *dst, unsigned num_dsts)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef tmp[LP_MAX_VECTOR_LENGTH];
   unsigned i;

   /*
    * We don't support float <-> int conversion here. That must be done
    * before/after calling this function.
    */
   assert(src_type.floating == dst_type.floating);

   /*
    * We don't support double <-> float conversion yet, although it could be
    * added with little effort.
    */
   assert((!src_type.floating && !dst_type.floating) ||
          src_type.width == dst_type.width);

   /* We must not loose or gain channels. Only precision */
   assert(src_type.length * num_srcs == dst_type.length * num_dsts);

   /* We don't support M:N conversion, only 1:N, M:1, or 1:1 */
   assert(num_srcs == 1 || num_dsts == 1);

   assert(src_type.length <= LP_MAX_VECTOR_LENGTH);
   assert(dst_type.length <= LP_MAX_VECTOR_LENGTH);
   assert(num_srcs <= LP_MAX_VECTOR_LENGTH);
   assert(num_dsts <= LP_MAX_VECTOR_LENGTH);

   if (src_type.width > dst_type.width) {
      /*
       * Truncate bit width.
       */

      assert(num_dsts == 1);

      if (src_type.width * src_type.length == dst_type.width * dst_type.length) {
        /*
         * Register width remains constant -- use vector packing intrinsics
         */
         tmp[0] = lp_build_pack(gallivm, src_type, dst_type, TRUE, src, num_srcs);
      }
      else {
         if (src_type.width / dst_type.width > num_srcs) {
            /*
            * First change src vectors size (with shuffle) so they have the
            * same size as the destination vector, then pack normally.
            * Note: cannot use cast/extract because llvm generates atrocious code.
            */
            unsigned size_ratio = (src_type.width * src_type.length) /
                                  (dst_type.length * dst_type.width);
            unsigned new_length = src_type.length / size_ratio;

            for (i = 0; i < size_ratio * num_srcs; i++) {
               unsigned start_index = (i % size_ratio) * new_length;
               tmp[i] = lp_build_extract_range(gallivm, src[i / size_ratio],
                                               start_index, new_length);
            }
            num_srcs *= size_ratio;
            src_type.length = new_length;
            tmp[0] = lp_build_pack(gallivm, src_type, dst_type, TRUE, tmp, num_srcs);
         }
         else {
            /*
             * Truncate bit width but expand vector size - first pack
             * then expand simply because this should be more AVX-friendly
             * for the cases we probably hit.
             */
            unsigned size_ratio = (dst_type.width * dst_type.length) /
                                  (src_type.length * src_type.width);
            unsigned num_pack_srcs = num_srcs / size_ratio;
            dst_type.length = dst_type.length / size_ratio;

            for (i = 0; i < size_ratio; i++) {
               tmp[i] = lp_build_pack(gallivm, src_type, dst_type, TRUE,
                                      &src[i*num_pack_srcs], num_pack_srcs);
            }
            tmp[0] = lp_build_concat(gallivm, tmp, dst_type, size_ratio);
         }
      }
   }
   else if (src_type.width < dst_type.width) {
      /*
       * Expand bit width.
       */

      assert(num_srcs == 1);

      if (src_type.width * src_type.length == dst_type.width * dst_type.length) {
         /*
          * Register width remains constant -- use vector unpack intrinsics
          */
         lp_build_unpack(gallivm, src_type, dst_type, src[0], tmp, num_dsts);
      }
      else {
         /*
          * Do it element-wise.
          */
         assert(src_type.length * num_srcs == dst_type.length * num_dsts);

         for (i = 0; i < num_dsts; i++) {
            tmp[i] = lp_build_undef(gallivm, dst_type);
         }

         for (i = 0; i < src_type.length; ++i) {
            unsigned j = i / dst_type.length;
            LLVMValueRef srcindex = lp_build_const_int32(gallivm, i);
            LLVMValueRef dstindex = lp_build_const_int32(gallivm, i % dst_type.length);
            LLVMValueRef val = LLVMBuildExtractElement(builder, src[0], srcindex, "");

            if (src_type.sign && dst_type.sign) {
               val = LLVMBuildSExt(builder, val, lp_build_elem_type(gallivm, dst_type), "");
            } else {
               val = LLVMBuildZExt(builder, val, lp_build_elem_type(gallivm, dst_type), "");
            }
            tmp[j] = LLVMBuildInsertElement(builder, tmp[j], val, dstindex, "");
         }
      }
   }
   else {
      /*
       * No-op
       */

      assert(num_srcs == 1);
      assert(num_dsts == 1);

      tmp[0] = src[0];
   }

   for(i = 0; i < num_dsts; ++i)
      dst[i] = tmp[i];
}


/**
 * Expands src vector from src.length to dst_length
 */
LLVMValueRef
lp_build_pad_vector(struct gallivm_state *gallivm,
                       LLVMValueRef src,
                       struct lp_type src_type,
                       unsigned dst_length)
{
   LLVMValueRef undef = LLVMGetUndef(lp_build_vec_type(gallivm, src_type));
   LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   unsigned i;

   assert(dst_length <= Elements(elems));
   assert(dst_length > src_type.length);

   if (src_type.length == dst_length)
      return src;

   /* If its a single scalar type, no need to reinvent the wheel */
   if (src_type.length == 1) {
      return lp_build_broadcast(gallivm, LLVMVectorType(lp_build_elem_type(gallivm, src_type), dst_length), src);
   }

   /* All elements from src vector */
   for (i = 0; i < src_type.length; ++i)
      elems[i] = lp_build_const_int32(gallivm, i);

   /* Undef fill remaining space */
   for (i = src_type.length; i < dst_length; ++i)
      elems[i] = lp_build_const_int32(gallivm, src_type.length);

   /* Combine the two vectors */
   return LLVMBuildShuffleVector(gallivm->builder, src, undef, LLVMConstVector(elems, dst_length), "");
}
