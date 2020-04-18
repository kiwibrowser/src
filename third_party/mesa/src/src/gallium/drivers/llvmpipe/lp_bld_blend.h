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

#ifndef LP_BLD_BLEND_H
#define LP_BLD_BLEND_H


#include "gallivm/lp_bld.h"
#include "gallivm/lp_bld_init.h"
 
#include "pipe/p_format.h"


struct pipe_blend_state;
struct lp_type;
struct lp_build_context;


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
               boolean optimise_only);


LLVMValueRef
lp_build_blend_aos(struct gallivm_state *gallivm,
                   const struct pipe_blend_state *blend,
                   const enum pipe_format *cbuf_format,
                   struct lp_type type,
                   unsigned rt,
                   LLVMValueRef src,
                   LLVMValueRef dst,
                   LLVMValueRef mask,
                   LLVMValueRef const_,
                   const unsigned char swizzle[4]);


void
lp_build_blend_soa(struct gallivm_state *gallivm,
                   const struct pipe_blend_state *blend,
                   struct lp_type type,
                   unsigned rt,
                   LLVMValueRef src[4],
                   LLVMValueRef dst[4],
                   LLVMValueRef const_[4],
                   LLVMValueRef res[4]);


/**
 * Apply a logic op.
 *
 * src/dst parameters are packed values. It should work regardless the inputs
 * are scalars, or a vector.
 */
LLVMValueRef
lp_build_logicop(LLVMBuilderRef builder,
                 unsigned logicop_func,
                 LLVMValueRef src,
                 LLVMValueRef dst);


LLVMValueRef
lp_build_blend_func(struct lp_build_context *bld,
                    unsigned func,
                    LLVMValueRef term1,
                    LLVMValueRef term2);


boolean
lp_build_blend_func_reverse(unsigned rgb_func,
                            unsigned alpha_func);


boolean
lp_build_blend_func_commutative(unsigned func);


#endif /* !LP_BLD_BLEND_H */
