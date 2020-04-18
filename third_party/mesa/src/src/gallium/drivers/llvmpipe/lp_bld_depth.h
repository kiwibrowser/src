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
 * Depth/stencil testing to LLVM IR translation.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#ifndef LP_BLD_DEPTH_H
#define LP_BLD_DEPTH_H


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"

#include "gallivm/lp_bld.h"

 
struct pipe_depth_state;
struct gallivm_state;
struct util_format_description;
struct lp_type;
struct lp_build_mask_context;


struct lp_type
lp_depth_type(const struct util_format_description *format_desc,
              unsigned length);


void
lp_build_depth_stencil_test(struct gallivm_state *gallivm,
                            const struct pipe_depth_state *depth,
                            const struct pipe_stencil_state stencil[2],
                            struct lp_type type,
                            const struct util_format_description *format_desc,
                            struct lp_build_mask_context *mask,
                            LLVMValueRef stencil_refs[2],
                            LLVMValueRef zs_src,
                            LLVMValueRef zs_dst_ptr,
                            LLVMValueRef facing,
                            LLVMValueRef *zs_value,
                            boolean do_branch);

void
lp_build_depth_write(LLVMBuilderRef builder,
                     const struct util_format_description *format_desc,
                     LLVMValueRef zs_dst_ptr,
                     LLVMValueRef zs_value);

void
lp_build_deferred_depth_write(struct gallivm_state *gallivm,
                              struct lp_type z_src_type,
                              const struct util_format_description *format_desc,
                              struct lp_build_mask_context *mask,
                              LLVMValueRef zs_dst_ptr,
                              LLVMValueRef zs_value);

void
lp_build_occlusion_count(struct gallivm_state *gallivm,
                         struct lp_type type,
                         LLVMValueRef maskvalue,
                         LLVMValueRef counter);

#endif /* !LP_BLD_DEPTH_H */
