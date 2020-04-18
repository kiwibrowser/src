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
 * Helper functions for calling intrinsics.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#ifndef LP_BLD_INTR_H
#define LP_BLD_INTR_H


#include "gallivm/lp_bld.h"
#include "gallivm/lp_bld_init.h"


/**
 * Max number of arguments in an intrinsic.
 */
#define LP_MAX_FUNC_ARGS 32


LLVMValueRef
lp_declare_intrinsic(LLVMModuleRef module,
                     const char *name,
                     LLVMTypeRef ret_type,
                     LLVMTypeRef *arg_types,
                     unsigned num_args);

LLVMValueRef
lp_build_intrinsic(LLVMBuilderRef builder,
                   const char *name,
                   LLVMTypeRef ret_type,
                   LLVMValueRef *args,
                   unsigned num_args);


LLVMValueRef
lp_build_intrinsic_unary(LLVMBuilderRef builder,
                         const char *name,
                         LLVMTypeRef ret_type,
                         LLVMValueRef a);


LLVMValueRef
lp_build_intrinsic_binary(LLVMBuilderRef builder,
                          const char *name,
                          LLVMTypeRef ret_type,
                          LLVMValueRef a,
                          LLVMValueRef b);


LLVMValueRef
lp_build_intrinsic_binary_anylength(struct gallivm_state *gallivm,
                                    const char *name,
                                    struct lp_type src_type,
                                    unsigned intr_size,
                                    LLVMValueRef a,
                                    LLVMValueRef b);


LLVMValueRef
lp_build_intrinsic_map(struct gallivm_state *gallivm,
                       const char *name,
                       LLVMTypeRef ret_type,
                       LLVMValueRef *args,
                       unsigned num_args);


LLVMValueRef
lp_build_intrinsic_map_unary(struct gallivm_state *gallivm,
                             const char *name,
                             LLVMTypeRef ret_type,
                             LLVMValueRef a);


LLVMValueRef
lp_build_intrinsic_map_binary(struct gallivm_state *gallivm,
                              const char *name,
                              LLVMTypeRef ret_type,
                              LLVMValueRef a,
                              LLVMValueRef b);


#endif /* !LP_BLD_INTR_H */
