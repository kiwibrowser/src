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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/


#include "lp_bld_type.h"
#include "lp_bld_arit.h"
#include "lp_bld_const.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_quad.h"


static const unsigned char
swizzle_left[4] = {
   LP_BLD_QUAD_TOP_LEFT,     LP_BLD_QUAD_TOP_LEFT,
   LP_BLD_QUAD_BOTTOM_LEFT,  LP_BLD_QUAD_BOTTOM_LEFT
};

static const unsigned char
swizzle_right[4] = {
   LP_BLD_QUAD_TOP_RIGHT,    LP_BLD_QUAD_TOP_RIGHT,
   LP_BLD_QUAD_BOTTOM_RIGHT, LP_BLD_QUAD_BOTTOM_RIGHT
};

static const unsigned char
swizzle_top[4] = {
   LP_BLD_QUAD_TOP_LEFT,     LP_BLD_QUAD_TOP_RIGHT,
   LP_BLD_QUAD_TOP_LEFT,     LP_BLD_QUAD_TOP_RIGHT
};

static const unsigned char
swizzle_bottom[4] = {
   LP_BLD_QUAD_BOTTOM_LEFT,  LP_BLD_QUAD_BOTTOM_RIGHT,
   LP_BLD_QUAD_BOTTOM_LEFT,  LP_BLD_QUAD_BOTTOM_RIGHT
};


LLVMValueRef
lp_build_ddx(struct lp_build_context *bld,
             LLVMValueRef a)
{
   LLVMValueRef a_left  = lp_build_swizzle_aos(bld, a, swizzle_left);
   LLVMValueRef a_right = lp_build_swizzle_aos(bld, a, swizzle_right);
   return lp_build_sub(bld, a_right, a_left);
}


LLVMValueRef
lp_build_ddy(struct lp_build_context *bld,
             LLVMValueRef a)
{
   LLVMValueRef a_top    = lp_build_swizzle_aos(bld, a, swizzle_top);
   LLVMValueRef a_bottom = lp_build_swizzle_aos(bld, a, swizzle_bottom);
   return lp_build_sub(bld, a_bottom, a_top);
}

/*
 * To be able to handle multiple quads at once in texture sampling and
 * do lod calculations per quad, it is necessary to get the per-quad
 * derivatives into the lp_build_rho function.
 * For 8-wide vectors the packed derivative values for 3 coords would
 * look like this, this scales to a arbitrary (multiple of 4) vector size:
 * ds1dx ds1dy dt1dx dt1dy ds2dx ds2dy dt2dx dt2dy
 * dr1dx dr1dy _____ _____ dr2dx dr2dy _____ _____
 * The second vector will be unused for 1d and 2d textures.
 */
LLVMValueRef
lp_build_packed_ddx_ddy_onecoord(struct lp_build_context *bld,
                                 LLVMValueRef a)
{
   struct gallivm_state *gallivm = bld->gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef vec1, vec2;

   /* same packing as _twocoord, but can use aos swizzle helper */

   /*
    * XXX could make swizzle1 a noop swizzle by using right top/bottom
    * pair for ddy
    */
   static const unsigned char swizzle1[] = {
      LP_BLD_QUAD_TOP_LEFT, LP_BLD_QUAD_TOP_LEFT,
      LP_BLD_SWIZZLE_DONTCARE, LP_BLD_SWIZZLE_DONTCARE
   };
   static const unsigned char swizzle2[] = {
      LP_BLD_QUAD_TOP_RIGHT, LP_BLD_QUAD_BOTTOM_LEFT,
      LP_BLD_SWIZZLE_DONTCARE, LP_BLD_SWIZZLE_DONTCARE
   };

   vec1 = lp_build_swizzle_aos(bld, a, swizzle1);
   vec2 = lp_build_swizzle_aos(bld, a, swizzle2);

   if (bld->type.floating)
      return LLVMBuildFSub(builder, vec2, vec1, "ddxddy");
   else
      return LLVMBuildSub(builder, vec2, vec1, "ddxddy");
}


LLVMValueRef
lp_build_packed_ddx_ddy_twocoord(struct lp_build_context *bld,
                                 LLVMValueRef a, LLVMValueRef b)
{
   struct gallivm_state *gallivm = bld->gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef shuffles1[LP_MAX_VECTOR_LENGTH/4];
   LLVMValueRef shuffles2[LP_MAX_VECTOR_LENGTH/4];
   LLVMValueRef vec1, vec2;
   unsigned length, num_quads, i;

   /* XXX: do hsub version */
   length = bld->type.length;
   num_quads = length / 4;
   for (i = 0; i < num_quads; i++) {
      unsigned s1 = 4 * i;
      unsigned s2 = 4 * i + length;
      shuffles1[4*i + 0] = lp_build_const_int32(gallivm, LP_BLD_QUAD_TOP_LEFT + s1);
      shuffles1[4*i + 1] = lp_build_const_int32(gallivm, LP_BLD_QUAD_TOP_LEFT + s1);
      shuffles1[4*i + 2] = lp_build_const_int32(gallivm, LP_BLD_QUAD_TOP_LEFT + s2);
      shuffles1[4*i + 3] = lp_build_const_int32(gallivm, LP_BLD_QUAD_TOP_LEFT + s2);
      shuffles2[4*i + 0] = lp_build_const_int32(gallivm, LP_BLD_QUAD_TOP_RIGHT + s1);
      shuffles2[4*i + 1] = lp_build_const_int32(gallivm, LP_BLD_QUAD_BOTTOM_LEFT + s1);
      shuffles2[4*i + 2] = lp_build_const_int32(gallivm, LP_BLD_QUAD_TOP_RIGHT + s2);
      shuffles2[4*i + 3] = lp_build_const_int32(gallivm, LP_BLD_QUAD_BOTTOM_LEFT + s2);
   }
   vec1 = LLVMBuildShuffleVector(builder, a, b,
                                 LLVMConstVector(shuffles1, length), "");
   vec2 = LLVMBuildShuffleVector(builder, a, b,
                                 LLVMConstVector(shuffles2, length), "");
   if (bld->type.floating)
      return LLVMBuildFSub(builder, vec2, vec1, "ddxddyddxddy");
   else
      return LLVMBuildSub(builder, vec2, vec1, "ddxddyddxddy");
}

