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


#ifndef LP_BLD_INIT_H
#define LP_BLD_INIT_H


#include "pipe/p_compiler.h"
#include "util/u_pointer.h" // for func_pointer
#include "lp_bld.h"
#include <llvm-c/ExecutionEngine.h>


struct gallivm_state
{
   LLVMModuleRef module;
   LLVMExecutionEngineRef engine;
   LLVMModuleProviderRef provider;
   LLVMTargetDataRef target;
   LLVMPassManagerRef passmgr;
   LLVMContextRef context;
   LLVMBuilderRef builder;
   unsigned compiled;
};


void
lp_build_init(void);


struct gallivm_state *
gallivm_create(void);

void
gallivm_destroy(struct gallivm_state *gallivm);


void
gallivm_verify_function(struct gallivm_state *gallivm,
                        LLVMValueRef func);

void
gallivm_compile_module(struct gallivm_state *gallivm);

func_pointer
gallivm_jit_function(struct gallivm_state *gallivm,
                     LLVMValueRef func);

void
gallivm_free_function(struct gallivm_state *gallivm,
                      LLVMValueRef func,
                      const void * code);

void
lp_set_load_alignment(LLVMValueRef Inst,
                       unsigned Align);

void
lp_set_store_alignment(LLVMValueRef Inst,
		       unsigned Align);

#endif /* !LP_BLD_INIT_H */
