/**************************************************************************
 *
 * Copyright 2012 VMware, Inc.
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

#include "pipe/p_state.h"
#include "util/u_debug.h"

#include "gallivm/lp_bld_type.h"
#include "gallivm/lp_bld_arit.h"

#include "lp_bld_blend.h"

/**
 * Is (a OP b) == (b OP a)?
 */
boolean
lp_build_blend_func_commutative(unsigned func)
{
   switch (func) {
   case PIPE_BLEND_ADD:
   case PIPE_BLEND_MIN:
   case PIPE_BLEND_MAX:
      return TRUE;
   case PIPE_BLEND_SUBTRACT:
   case PIPE_BLEND_REVERSE_SUBTRACT:
      return FALSE;
   default:
      assert(0);
      return TRUE;
   }
}


/**
 * Whether the blending functions are the reverse of each other.
 */
boolean
lp_build_blend_func_reverse(unsigned rgb_func, unsigned alpha_func)
{
   if(rgb_func == alpha_func)
      return FALSE;
   if(rgb_func == PIPE_BLEND_SUBTRACT && alpha_func == PIPE_BLEND_REVERSE_SUBTRACT)
      return TRUE;
   if(rgb_func == PIPE_BLEND_REVERSE_SUBTRACT && alpha_func == PIPE_BLEND_SUBTRACT)
      return TRUE;
   return FALSE;
}


/**
 * Whether the blending factors are complementary of each other.
 */
static INLINE boolean
lp_build_blend_factor_complementary(unsigned src_factor, unsigned dst_factor)
{
   return dst_factor == (src_factor ^ 0x10);
}


/**
 * @sa http://www.opengl.org/sdk/docs/man/xhtml/glBlendEquationSeparate.xml
 */
LLVMValueRef
lp_build_blend_func(struct lp_build_context *bld,
                    unsigned func,
                    LLVMValueRef term1,
                    LLVMValueRef term2)
{
   switch (func) {
   case PIPE_BLEND_ADD:
      return lp_build_add(bld, term1, term2);
   case PIPE_BLEND_SUBTRACT:
      return lp_build_sub(bld, term1, term2);
   case PIPE_BLEND_REVERSE_SUBTRACT:
      return lp_build_sub(bld, term2, term1);
   case PIPE_BLEND_MIN:
      return lp_build_min(bld, term1, term2);
   case PIPE_BLEND_MAX:
      return lp_build_max(bld, term1, term2);
   default:
      assert(0);
      return bld->zero;
   }
}


/**
 * Performs optimisations and blending independent of SoA/AoS
 *
 * @param func                   the blend function
 * @param factor_src             PIPE_BLENDFACTOR_xxx
 * @param factor_dst             PIPE_BLENDFACTOR_xxx
 * @param src                    source rgba
 * @param dst                    dest rgba
 * @param src_factor             src factor computed value
 * @param dst_factor             dst factor computed value
 * @param not_alpha_dependent    same factors accross all channels of src/dst
 *
 * not_alpha_dependent should be:
 *  SoA: always true as it is only one channel at a time
 *  AoS: rgb_src_factor == alpha_src_factor && rgb_dst_factor == alpha_dst_factor
 *
 * Note that pretty much every possible optimisation can only be done on non-unorm targets
 * due to unorm values not going above 1.0 meaning factorisation can change results.
 * e.g. (0.9 * 0.9) + (0.9 * 0.9) != 0.9 * (0.9 + 0.9) as result of + is always <= 1.
 */
LLVMValueRef
lp_build_blend(struct lp_build_context *bld,
               unsigned func,
               unsigned factor_src,
               unsigned factor_dst,
               LLVMValueRef src,
               LLVMValueRef dst,
               LLVMValueRef src_factor,
               LLVMValueRef dst_factor,
               boolean not_alpha_dependent,
               boolean optimise_only)
{
   LLVMValueRef result, src_term, dst_term;

   /* If we are not alpha dependent we can mess with the src/dst factors */
   if (not_alpha_dependent) {
      if (lp_build_blend_factor_complementary(factor_src, factor_dst)) {
         if (func == PIPE_BLEND_ADD) {
            if (factor_src < factor_dst) {
               return lp_build_lerp(bld, src_factor, dst, src);
            } else {
               return lp_build_lerp(bld, dst_factor, src, dst);
            }
         } else if(bld->type.floating && func == PIPE_BLEND_SUBTRACT) {
            result = lp_build_add(bld, src, dst);

            if (factor_src < factor_dst) {
               result = lp_build_mul(bld, result, src_factor);
               return lp_build_sub(bld, result, dst);
            } else {
               result = lp_build_mul(bld, result, dst_factor);
               return lp_build_sub(bld, src, result);
            }
         } else if(bld->type.floating && func == PIPE_BLEND_REVERSE_SUBTRACT) {
            result = lp_build_add(bld, src, dst);

            if (factor_src < factor_dst) {
               result = lp_build_mul(bld, result, src_factor);
               return lp_build_sub(bld, dst, result);
            } else {
               result = lp_build_mul(bld, result, dst_factor);
               return lp_build_sub(bld, result, src);
            }
         }
      }

      if (bld->type.floating && factor_src == factor_dst) {
         if (func == PIPE_BLEND_ADD ||
             func == PIPE_BLEND_SUBTRACT ||
             func == PIPE_BLEND_REVERSE_SUBTRACT) {
            LLVMValueRef result;
            result = lp_build_blend_func(bld, func, src, dst);
            return lp_build_mul(bld, result, src_factor);
         }
      }
   }

   if (optimise_only)
      return NULL;

   src_term = lp_build_mul(bld, src, src_factor);
   dst_term = lp_build_mul(bld, dst, dst_factor);
   return lp_build_blend_func(bld, func, src_term, dst_term);
}
