/**************************************************************************
 *
 * Copyright 2009-2010 VMware, Inc.
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
 * Depth/stencil testing to LLVM IR translation.
 *
 * To be done accurately/efficiently the depth/stencil test must be done with
 * the same type/format of the depth/stencil buffer, which implies massaging
 * the incoming depths to fit into place. Using a more straightforward
 * type/format for depth/stencil values internally and only convert when
 * flushing would avoid this, but it would most likely result in depth fighting
 * artifacts.
 *
 * We are free to use a different pixel layout though. Since our basic
 * processing unit is a quad (2x2 pixel block) we store the depth/stencil
 * values tiled, a quad at time. That is, a depth buffer containing 
 *
 *  Z11 Z12 Z13 Z14 ...
 *  Z21 Z22 Z23 Z24 ...
 *  Z31 Z32 Z33 Z34 ...
 *  Z41 Z42 Z43 Z44 ...
 *  ... ... ... ... ...
 *
 * will actually be stored in memory as
 *
 *  Z11 Z12 Z21 Z22 Z13 Z14 Z23 Z24 ...
 *  Z31 Z32 Z41 Z42 Z33 Z34 Z43 Z44 ...
 *  ... ... ... ... ... ... ... ... ...
 *
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 * @author Brian Paul <jfonseca@vmware.com>
 */

#include "pipe/p_state.h"
#include "util/u_format.h"
#include "util/u_cpu_detect.h"

#include "gallivm/lp_bld_type.h"
#include "gallivm/lp_bld_arit.h"
#include "gallivm/lp_bld_bitarit.h"
#include "gallivm/lp_bld_const.h"
#include "gallivm/lp_bld_conv.h"
#include "gallivm/lp_bld_logic.h"
#include "gallivm/lp_bld_flow.h"
#include "gallivm/lp_bld_intr.h"
#include "gallivm/lp_bld_debug.h"
#include "gallivm/lp_bld_swizzle.h"

#include "lp_bld_depth.h"


/** Used to select fields from pipe_stencil_state */
enum stencil_op {
   S_FAIL_OP,
   Z_FAIL_OP,
   Z_PASS_OP
};



/**
 * Do the stencil test comparison (compare FB stencil values against ref value).
 * This will be used twice when generating two-sided stencil code.
 * \param stencil  the front/back stencil state
 * \param stencilRef  the stencil reference value, replicated as a vector
 * \param stencilVals  vector of stencil values from framebuffer
 * \return vector mask of pass/fail values (~0 or 0)
 */
static LLVMValueRef
lp_build_stencil_test_single(struct lp_build_context *bld,
                             const struct pipe_stencil_state *stencil,
                             LLVMValueRef stencilRef,
                             LLVMValueRef stencilVals)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const unsigned stencilMax = 255; /* XXX fix */
   struct lp_type type = bld->type;
   LLVMValueRef res;

   /*
    * SSE2 has intrinsics for signed comparisons, but not unsigned ones. Values
    * are between 0..255 so ensure we generate the fastest comparisons for
    * wider elements.
    */
   if (type.width <= 8) {
      assert(!type.sign);
   } else {
      assert(type.sign);
   }

   assert(stencil->enabled);

   if (stencil->valuemask != stencilMax) {
      /* compute stencilRef = stencilRef & valuemask */
      LLVMValueRef valuemask = lp_build_const_int_vec(bld->gallivm, type, stencil->valuemask);
      stencilRef = LLVMBuildAnd(builder, stencilRef, valuemask, "");
      /* compute stencilVals = stencilVals & valuemask */
      stencilVals = LLVMBuildAnd(builder, stencilVals, valuemask, "");
   }

   res = lp_build_cmp(bld, stencil->func, stencilRef, stencilVals);

   return res;
}


/**
 * Do the one or two-sided stencil test comparison.
 * \sa lp_build_stencil_test_single
 * \param front_facing  an integer vector mask, indicating front (~0) or back
 *                      (0) facing polygon. If NULL, assume front-facing.
 */
static LLVMValueRef
lp_build_stencil_test(struct lp_build_context *bld,
                      const struct pipe_stencil_state stencil[2],
                      LLVMValueRef stencilRefs[2],
                      LLVMValueRef stencilVals,
                      LLVMValueRef front_facing)
{
   LLVMValueRef res;

   assert(stencil[0].enabled);

   /* do front face test */
   res = lp_build_stencil_test_single(bld, &stencil[0],
                                      stencilRefs[0], stencilVals);

   if (stencil[1].enabled && front_facing != NULL) {
      /* do back face test */
      LLVMValueRef back_res;

      back_res = lp_build_stencil_test_single(bld, &stencil[1],
                                              stencilRefs[1], stencilVals);

      res = lp_build_select(bld, front_facing, res, back_res);
   }

   return res;
}


/**
 * Apply the stencil operator (add/sub/keep/etc) to the given vector
 * of stencil values.
 * \return  new stencil values vector
 */
static LLVMValueRef
lp_build_stencil_op_single(struct lp_build_context *bld,
                           const struct pipe_stencil_state *stencil,
                           enum stencil_op op,
                           LLVMValueRef stencilRef,
                           LLVMValueRef stencilVals)

{
   LLVMBuilderRef builder = bld->gallivm->builder;
   struct lp_type type = bld->type;
   LLVMValueRef res;
   LLVMValueRef max = lp_build_const_int_vec(bld->gallivm, type, 0xff);
   unsigned stencil_op;

   assert(type.sign);

   switch (op) {
   case S_FAIL_OP:
      stencil_op = stencil->fail_op;
      break;
   case Z_FAIL_OP:
      stencil_op = stencil->zfail_op;
      break;
   case Z_PASS_OP:
      stencil_op = stencil->zpass_op;
      break;
   default:
      assert(0 && "Invalid stencil_op mode");
      stencil_op = PIPE_STENCIL_OP_KEEP;
   }

   switch (stencil_op) {
   case PIPE_STENCIL_OP_KEEP:
      res = stencilVals;
      /* we can return early for this case */
      return res;
   case PIPE_STENCIL_OP_ZERO:
      res = bld->zero;
      break;
   case PIPE_STENCIL_OP_REPLACE:
      res = stencilRef;
      break;
   case PIPE_STENCIL_OP_INCR:
      res = lp_build_add(bld, stencilVals, bld->one);
      res = lp_build_min(bld, res, max);
      break;
   case PIPE_STENCIL_OP_DECR:
      res = lp_build_sub(bld, stencilVals, bld->one);
      res = lp_build_max(bld, res, bld->zero);
      break;
   case PIPE_STENCIL_OP_INCR_WRAP:
      res = lp_build_add(bld, stencilVals, bld->one);
      res = LLVMBuildAnd(builder, res, max, "");
      break;
   case PIPE_STENCIL_OP_DECR_WRAP:
      res = lp_build_sub(bld, stencilVals, bld->one);
      res = LLVMBuildAnd(builder, res, max, "");
      break;
   case PIPE_STENCIL_OP_INVERT:
      res = LLVMBuildNot(builder, stencilVals, "");
      res = LLVMBuildAnd(builder, res, max, "");
      break;
   default:
      assert(0 && "bad stencil op mode");
      res = bld->undef;
   }

   return res;
}


/**
 * Do the one or two-sided stencil test op/update.
 */
static LLVMValueRef
lp_build_stencil_op(struct lp_build_context *bld,
                    const struct pipe_stencil_state stencil[2],
                    enum stencil_op op,
                    LLVMValueRef stencilRefs[2],
                    LLVMValueRef stencilVals,
                    LLVMValueRef mask,
                    LLVMValueRef front_facing)

{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef res;

   assert(stencil[0].enabled);

   /* do front face op */
   res = lp_build_stencil_op_single(bld, &stencil[0], op,
                                     stencilRefs[0], stencilVals);

   if (stencil[1].enabled && front_facing != NULL) {
      /* do back face op */
      LLVMValueRef back_res;

      back_res = lp_build_stencil_op_single(bld, &stencil[1], op,
                                            stencilRefs[1], stencilVals);

      res = lp_build_select(bld, front_facing, res, back_res);
   }

   if (stencil[0].writemask != 0xff ||
       (stencil[1].enabled && front_facing != NULL && stencil[1].writemask != 0xff)) {
      /* mask &= stencil[0].writemask */
      LLVMValueRef writemask = lp_build_const_int_vec(bld->gallivm, bld->type,
                                                      stencil[0].writemask);
      if (stencil[1].enabled && stencil[1].writemask != stencil[0].writemask && front_facing != NULL) {
         LLVMValueRef back_writemask = lp_build_const_int_vec(bld->gallivm, bld->type,
                                                         stencil[1].writemask);
         writemask = lp_build_select(bld, front_facing, writemask, back_writemask);
      }

      mask = LLVMBuildAnd(builder, mask, writemask, "");
      /* res = (res & mask) | (stencilVals & ~mask) */
      res = lp_build_select_bitwise(bld, mask, res, stencilVals);
   }
   else {
      /* res = mask ? res : stencilVals */
      res = lp_build_select(bld, mask, res, stencilVals);
   }

   return res;
}



/**
 * Return a type appropriate for depth/stencil testing.
 */
struct lp_type
lp_depth_type(const struct util_format_description *format_desc,
              unsigned length)
{
   struct lp_type type;
   unsigned swizzle;

   assert(format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS);
   assert(format_desc->block.width == 1);
   assert(format_desc->block.height == 1);

   swizzle = format_desc->swizzle[0];
   assert(swizzle < 4);

   memset(&type, 0, sizeof type);
   type.width = format_desc->block.bits;

   if(format_desc->channel[swizzle].type == UTIL_FORMAT_TYPE_FLOAT) {
      type.floating = TRUE;
      assert(swizzle == 0);
      assert(format_desc->channel[swizzle].size == format_desc->block.bits);
   }
   else if(format_desc->channel[swizzle].type == UTIL_FORMAT_TYPE_UNSIGNED) {
      assert(format_desc->block.bits <= 32);
      assert(format_desc->channel[swizzle].normalized);
      if (format_desc->channel[swizzle].size < format_desc->block.bits) {
         /* Prefer signed integers when possible, as SSE has less support
          * for unsigned comparison;
          */
         type.sign = TRUE;
      }
   }
   else
      assert(0);

   assert(type.width <= length);
   type.length = length / type.width;

   return type;
}


/**
 * Compute bitmask and bit shift to apply to the incoming fragment Z values
 * and the Z buffer values needed before doing the Z comparison.
 *
 * Note that we leave the Z bits in the position that we find them
 * in the Z buffer (typically 0xffffff00 or 0x00ffffff).  That lets us
 * get by with fewer bit twiddling steps.
 */
static boolean
get_z_shift_and_mask(const struct util_format_description *format_desc,
                     unsigned *shift, unsigned *width, unsigned *mask)
{
   const unsigned total_bits = format_desc->block.bits;
   unsigned z_swizzle;
   unsigned chan;
   unsigned padding_left, padding_right;
   
   assert(format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS);
   assert(format_desc->block.width == 1);
   assert(format_desc->block.height == 1);

   z_swizzle = format_desc->swizzle[0];

   if (z_swizzle == UTIL_FORMAT_SWIZZLE_NONE)
      return FALSE;

   *width = format_desc->channel[z_swizzle].size;

   padding_right = 0;
   for (chan = 0; chan < z_swizzle; ++chan)
      padding_right += format_desc->channel[chan].size;

   padding_left =
      total_bits - (padding_right + *width);

   if (padding_left || padding_right) {
      unsigned long long mask_left = (1ULL << (total_bits - padding_left)) - 1;
      unsigned long long mask_right = (1ULL << (padding_right)) - 1;
      *mask = mask_left ^ mask_right;
   }
   else {
      *mask = 0xffffffff;
   }

   *shift = padding_right;

   return TRUE;
}


/**
 * Compute bitmask and bit shift to apply to the framebuffer pixel values
 * to put the stencil bits in the least significant position.
 * (i.e. 0x000000ff)
 */
static boolean
get_s_shift_and_mask(const struct util_format_description *format_desc,
                     unsigned *shift, unsigned *mask)
{
   unsigned s_swizzle;
   unsigned chan, sz;

   s_swizzle = format_desc->swizzle[1];

   if (s_swizzle == UTIL_FORMAT_SWIZZLE_NONE)
      return FALSE;

   *shift = 0;
   for (chan = 0; chan < s_swizzle; chan++)
      *shift += format_desc->channel[chan].size;

   sz = format_desc->channel[s_swizzle].size;
   *mask = (1U << sz) - 1U;

   return TRUE;
}


/**
 * Perform the occlusion test and increase the counter.
 * Test the depth mask. Add the number of channel which has none zero mask
 * into the occlusion counter. e.g. maskvalue is {-1, -1, -1, -1}.
 * The counter will add 4.
 *
 * \param type holds element type of the mask vector.
 * \param maskvalue is the depth test mask.
 * \param counter is a pointer of the uint32 counter.
 */
void
lp_build_occlusion_count(struct gallivm_state *gallivm,
                         struct lp_type type,
                         LLVMValueRef maskvalue,
                         LLVMValueRef counter)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMContextRef context = gallivm->context;
   LLVMValueRef countmask = lp_build_const_int_vec(gallivm, type, 1);
   LLVMValueRef count, newcount;

   assert(type.length <= 16);
   assert(type.floating);

   if(util_cpu_caps.has_sse && type.length == 4) {
      const char *movmskintr = "llvm.x86.sse.movmsk.ps";
      const char *popcntintr = "llvm.ctpop.i32";
      LLVMValueRef bits = LLVMBuildBitCast(builder, maskvalue,
                                           lp_build_vec_type(gallivm, type), "");
      bits = lp_build_intrinsic_unary(builder, movmskintr,
                                      LLVMInt32TypeInContext(context), bits);
      count = lp_build_intrinsic_unary(builder, popcntintr,
                                       LLVMInt32TypeInContext(context), bits);
   }
   else if(util_cpu_caps.has_avx && type.length == 8) {
      const char *movmskintr = "llvm.x86.avx.movmsk.ps.256";
      const char *popcntintr = "llvm.ctpop.i32";
      LLVMValueRef bits = LLVMBuildBitCast(builder, maskvalue,
                                           lp_build_vec_type(gallivm, type), "");
      bits = lp_build_intrinsic_unary(builder, movmskintr,
                                      LLVMInt32TypeInContext(context), bits);
      count = lp_build_intrinsic_unary(builder, popcntintr,
                                       LLVMInt32TypeInContext(context), bits);
   }
   else {
      unsigned i;
      LLVMValueRef countv = LLVMBuildAnd(builder, maskvalue, countmask, "countv");
      LLVMTypeRef counttype = LLVMIntTypeInContext(context, type.length * 8);
      LLVMTypeRef i8vntype = LLVMVectorType(LLVMInt8TypeInContext(context), type.length * 4);
      LLVMValueRef shufflev, countd;
      LLVMValueRef shuffles[16];
      const char *popcntintr = NULL;

      countv = LLVMBuildBitCast(builder, countv, i8vntype, "");

       for (i = 0; i < type.length; i++) {
          shuffles[i] = lp_build_const_int32(gallivm, 4*i);
       }

       shufflev = LLVMConstVector(shuffles, type.length);
       countd = LLVMBuildShuffleVector(builder, countv, LLVMGetUndef(i8vntype), shufflev, "");
       countd = LLVMBuildBitCast(builder, countd, counttype, "countd");

       /*
        * XXX FIXME
        * this is bad on cpus without popcount (on x86 supported by intel
        * nehalem, amd barcelona, and up - not tied to sse42).
        * Would be much faster to just sum the 4 elements of the vector with
        * some horizontal add (shuffle/add/shuffle/add after the initial and).
        */
       switch (type.length) {
       case 4:
          popcntintr = "llvm.ctpop.i32";
          break;
       case 8:
          popcntintr = "llvm.ctpop.i64";
          break;
       case 16:
          popcntintr = "llvm.ctpop.i128";
          break;
       default:
          assert(0);
       }
       count = lp_build_intrinsic_unary(builder, popcntintr, counttype, countd);

       if (type.length > 4) {
          count = LLVMBuildTrunc(builder, count, LLVMIntTypeInContext(context, 32), "");
       }
   }
   newcount = LLVMBuildLoad(builder, counter, "origcount");
   newcount = LLVMBuildAdd(builder, newcount, count, "newcount");
   LLVMBuildStore(builder, newcount, counter);
}



/**
 * Generate code for performing depth and/or stencil tests.
 * We operate on a vector of values (typically n 2x2 quads).
 *
 * \param depth  the depth test state
 * \param stencil  the front/back stencil state
 * \param type  the data type of the fragment depth/stencil values
 * \param format_desc  description of the depth/stencil surface
 * \param mask  the alive/dead pixel mask for the quad (vector)
 * \param stencil_refs  the front/back stencil ref values (scalar)
 * \param z_src  the incoming depth/stencil values (n 2x2 quad values, float32)
 * \param zs_dst_ptr  pointer to depth/stencil values in framebuffer
 * \param face  contains boolean value indicating front/back facing polygon
 */
void
lp_build_depth_stencil_test(struct gallivm_state *gallivm,
                            const struct pipe_depth_state *depth,
                            const struct pipe_stencil_state stencil[2],
                            struct lp_type z_src_type,
                            const struct util_format_description *format_desc,
                            struct lp_build_mask_context *mask,
                            LLVMValueRef stencil_refs[2],
                            LLVMValueRef z_src,
                            LLVMValueRef zs_dst_ptr,
                            LLVMValueRef face,
                            LLVMValueRef *zs_value,
                            boolean do_branch)
{
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_type z_type;
   struct lp_build_context z_bld;
   struct lp_build_context s_bld;
   struct lp_type s_type;
   unsigned z_shift = 0, z_width = 0, z_mask = 0;
   LLVMValueRef zs_dst, z_dst = NULL;
   LLVMValueRef stencil_vals = NULL;
   LLVMValueRef z_bitmask = NULL, stencil_shift = NULL;
   LLVMValueRef z_pass = NULL, s_pass_mask = NULL;
   LLVMValueRef orig_mask = lp_build_mask_value(mask);
   LLVMValueRef front_facing = NULL;


   /*
    * Depths are expected to be between 0 and 1, even if they are stored in
    * floats. Setting these bits here will ensure that the lp_build_conv() call
    * below won't try to unnecessarily clamp the incoming values.
    */
   if(z_src_type.floating) {
      z_src_type.sign = FALSE;
      z_src_type.norm = TRUE;
   }
   else {
      assert(!z_src_type.sign);
      assert(z_src_type.norm);
   }

   /* Pick the depth type. */
   z_type = lp_depth_type(format_desc, z_src_type.width*z_src_type.length);

   /* FIXME: Cope with a depth test type with a different bit width. */
   assert(z_type.width == z_src_type.width);
   assert(z_type.length == z_src_type.length);

   /* FIXME: for non-float depth/stencil might generate better code
    * if we'd always split it up to use 128bit operations.
    * For stencil we'd almost certainly want to pack to 8xi16 values,
    * for z just run twice.
    */

   /* Sanity checking */
   {
      const unsigned z_swizzle = format_desc->swizzle[0];
      const unsigned s_swizzle = format_desc->swizzle[1];

      assert(z_swizzle != UTIL_FORMAT_SWIZZLE_NONE ||
             s_swizzle != UTIL_FORMAT_SWIZZLE_NONE);

      assert(depth->enabled || stencil[0].enabled);

      assert(format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS);
      assert(format_desc->block.width == 1);
      assert(format_desc->block.height == 1);

      if (stencil[0].enabled) {
         assert(format_desc->format == PIPE_FORMAT_Z24_UNORM_S8_UINT ||
                format_desc->format == PIPE_FORMAT_S8_UINT_Z24_UNORM);
      }

      assert(z_swizzle < 4);
      assert(format_desc->block.bits == z_type.width);
      if (z_type.floating) {
         assert(z_swizzle == 0);
         assert(format_desc->channel[z_swizzle].type ==
                UTIL_FORMAT_TYPE_FLOAT);
         assert(format_desc->channel[z_swizzle].size ==
                format_desc->block.bits);
      }
      else {
         assert(format_desc->channel[z_swizzle].type ==
                UTIL_FORMAT_TYPE_UNSIGNED);
         assert(format_desc->channel[z_swizzle].normalized);
         assert(!z_type.fixed);
      }
   }


   /* Setup build context for Z vals */
   lp_build_context_init(&z_bld, gallivm, z_type);

   /* Setup build context for stencil vals */
   s_type = lp_int_type(z_type);
   lp_build_context_init(&s_bld, gallivm, s_type);

   /* Load current z/stencil value from z/stencil buffer */
   zs_dst_ptr = LLVMBuildBitCast(builder,
                                 zs_dst_ptr,
                                 LLVMPointerType(z_bld.vec_type, 0), "");
   zs_dst = LLVMBuildLoad(builder, zs_dst_ptr, "");

   lp_build_name(zs_dst, "zs_dst");


   /* Compute and apply the Z/stencil bitmasks and shifts.
    */
   {
      unsigned s_shift, s_mask;

      if (get_z_shift_and_mask(format_desc, &z_shift, &z_width, &z_mask)) {
         if (z_mask != 0xffffffff) {
            z_bitmask = lp_build_const_int_vec(gallivm, z_type, z_mask);
         }

         /*
          * Align the framebuffer Z 's LSB to the right.
          */
         if (z_shift) {
            LLVMValueRef shift = lp_build_const_int_vec(gallivm, z_type, z_shift);
            z_dst = LLVMBuildLShr(builder, zs_dst, shift, "z_dst");
         } else if (z_bitmask) {
	    /* TODO: Instead of loading a mask from memory and ANDing, it's
	     * probably faster to just shake the bits with two shifts. */
            z_dst = LLVMBuildAnd(builder, zs_dst, z_bitmask, "z_dst");
         } else {
            z_dst = zs_dst;
            lp_build_name(z_dst, "z_dst");
         }
      }

      if (get_s_shift_and_mask(format_desc, &s_shift, &s_mask)) {
         if (s_shift) {
            LLVMValueRef shift = lp_build_const_int_vec(gallivm, s_type, s_shift);
            stencil_vals = LLVMBuildLShr(builder, zs_dst, shift, "");
            stencil_shift = shift;  /* used below */
         }
         else {
            stencil_vals = zs_dst;
         }

         if (s_mask != 0xffffffff) {
            LLVMValueRef mask = lp_build_const_int_vec(gallivm, s_type, s_mask);
            stencil_vals = LLVMBuildAnd(builder, stencil_vals, mask, "");
         }

         lp_build_name(stencil_vals, "s_dst");
      }
   }

   if (stencil[0].enabled) {

      if (face) {
         LLVMValueRef zero = lp_build_const_int32(gallivm, 0);

         /* front_facing = face != 0 ? ~0 : 0 */
         front_facing = LLVMBuildICmp(builder, LLVMIntNE, face, zero, "");
         front_facing = LLVMBuildSExt(builder, front_facing,
                                      LLVMIntTypeInContext(gallivm->context,
                                             s_bld.type.length*s_bld.type.width),
                                      "");
         front_facing = LLVMBuildBitCast(builder, front_facing,
                                         s_bld.int_vec_type, "");
      }

      /* convert scalar stencil refs into vectors */
      stencil_refs[0] = lp_build_broadcast_scalar(&s_bld, stencil_refs[0]);
      stencil_refs[1] = lp_build_broadcast_scalar(&s_bld, stencil_refs[1]);

      s_pass_mask = lp_build_stencil_test(&s_bld, stencil,
                                          stencil_refs, stencil_vals,
                                          front_facing);

      /* apply stencil-fail operator */
      {
         LLVMValueRef s_fail_mask = lp_build_andnot(&s_bld, orig_mask, s_pass_mask);
         stencil_vals = lp_build_stencil_op(&s_bld, stencil, S_FAIL_OP,
                                            stencil_refs, stencil_vals,
                                            s_fail_mask, front_facing);
      }
   }

   if (depth->enabled) {
      /*
       * Convert fragment Z to the desired type, aligning the LSB to the right.
       */

      assert(z_type.width == z_src_type.width);
      assert(z_type.length == z_src_type.length);
      assert(lp_check_value(z_src_type, z_src));
      if (z_src_type.floating) {
         /*
          * Convert from floating point values
          */

         if (!z_type.floating) {
            z_src = lp_build_clamped_float_to_unsigned_norm(gallivm,
                                                            z_src_type,
                                                            z_width,
                                                            z_src);
         }
      } else {
         /*
          * Convert from unsigned normalized values.
          */

         assert(!z_src_type.sign);
         assert(!z_src_type.fixed);
         assert(z_src_type.norm);
         assert(!z_type.floating);
         if (z_src_type.width > z_width) {
            LLVMValueRef shift = lp_build_const_int_vec(gallivm, z_src_type,
                                                        z_src_type.width - z_width);
            z_src = LLVMBuildLShr(builder, z_src, shift, "");
         }
      }
      assert(lp_check_value(z_type, z_src));

      lp_build_name(z_src, "z_src");

      /* compare src Z to dst Z, returning 'pass' mask */
      z_pass = lp_build_cmp(&z_bld, depth->func, z_src, z_dst);

      if (!stencil[0].enabled) {
         /* We can potentially skip all remaining operations here, but only
          * if stencil is disabled because we still need to update the stencil
          * buffer values.  Don't need to update Z buffer values.
          */
         lp_build_mask_update(mask, z_pass);

         if (do_branch) {
            lp_build_mask_check(mask);
            do_branch = FALSE;
         }
      }

      if (depth->writemask) {
         LLVMValueRef zselectmask;

         /* mask off bits that failed Z test */
         zselectmask = LLVMBuildAnd(builder, orig_mask, z_pass, "");

         /* mask off bits that failed stencil test */
         if (s_pass_mask) {
            zselectmask = LLVMBuildAnd(builder, zselectmask, s_pass_mask, "");
         }

         /* Mix the old and new Z buffer values.
          * z_dst[i] = zselectmask[i] ? z_src[i] : z_dst[i]
          */
         z_dst = lp_build_select(&z_bld, zselectmask, z_src, z_dst);
      }

      if (stencil[0].enabled) {
         /* update stencil buffer values according to z pass/fail result */
         LLVMValueRef z_fail_mask, z_pass_mask;

         /* apply Z-fail operator */
         z_fail_mask = lp_build_andnot(&z_bld, orig_mask, z_pass);
         stencil_vals = lp_build_stencil_op(&s_bld, stencil, Z_FAIL_OP,
                                            stencil_refs, stencil_vals,
                                            z_fail_mask, front_facing);

         /* apply Z-pass operator */
         z_pass_mask = LLVMBuildAnd(builder, orig_mask, z_pass, "");
         stencil_vals = lp_build_stencil_op(&s_bld, stencil, Z_PASS_OP,
                                            stencil_refs, stencil_vals,
                                            z_pass_mask, front_facing);
      }
   }
   else {
      /* No depth test: apply Z-pass operator to stencil buffer values which
       * passed the stencil test.
       */
      s_pass_mask = LLVMBuildAnd(builder, orig_mask, s_pass_mask, "");
      stencil_vals = lp_build_stencil_op(&s_bld, stencil, Z_PASS_OP,
                                         stencil_refs, stencil_vals,
                                         s_pass_mask, front_facing);
   }

   /* Put Z and ztencil bits in the right place */
   if (z_dst && z_shift) {
      LLVMValueRef shift = lp_build_const_int_vec(gallivm, z_type, z_shift);
      z_dst = LLVMBuildShl(builder, z_dst, shift, "");
   }
   if (stencil_vals && stencil_shift)
      stencil_vals = LLVMBuildShl(builder, stencil_vals,
                                  stencil_shift, "");

   /* Finally, merge/store the z/stencil values */
   if ((depth->enabled && depth->writemask) ||
       (stencil[0].enabled && stencil[0].writemask)) {

      if (z_dst && stencil_vals)
         zs_dst = LLVMBuildOr(builder, z_dst, stencil_vals, "");
      else if (z_dst)
         zs_dst = z_dst;
      else
         zs_dst = stencil_vals;

      *zs_value = zs_dst;
   }

   if (s_pass_mask)
      lp_build_mask_update(mask, s_pass_mask);

   if (depth->enabled && stencil[0].enabled)
      lp_build_mask_update(mask, z_pass);

   if (do_branch)
      lp_build_mask_check(mask);

}


void
lp_build_depth_write(LLVMBuilderRef builder,
                     const struct util_format_description *format_desc,
                     LLVMValueRef zs_dst_ptr,
                     LLVMValueRef zs_value)
{
   zs_dst_ptr = LLVMBuildBitCast(builder, zs_dst_ptr,
                                 LLVMPointerType(LLVMTypeOf(zs_value), 0), "");

   LLVMBuildStore(builder, zs_value, zs_dst_ptr);
}


void
lp_build_deferred_depth_write(struct gallivm_state *gallivm,
                              struct lp_type z_src_type,
                              const struct util_format_description *format_desc,
                              struct lp_build_mask_context *mask,
                              LLVMValueRef zs_dst_ptr,
                              LLVMValueRef zs_value)
{
   struct lp_type z_type;
   struct lp_build_context z_bld;
   LLVMValueRef z_dst;
   LLVMBuilderRef builder = gallivm->builder;

   /* XXX: pointlessly redo type logic:
    */
   z_type = lp_depth_type(format_desc, z_src_type.width*z_src_type.length);
   lp_build_context_init(&z_bld, gallivm, z_type);

   zs_dst_ptr = LLVMBuildBitCast(builder, zs_dst_ptr,
                                 LLVMPointerType(z_bld.vec_type, 0), "");

   z_dst = LLVMBuildLoad(builder, zs_dst_ptr, "zsbufval");
   z_dst = lp_build_select(&z_bld, lp_build_mask_value(mask), zs_value, z_dst);

   LLVMBuildStore(builder, z_dst, zs_dst_ptr);
}
