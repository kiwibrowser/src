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


#ifndef LP_BLD_MISC_H
#define LP_BLD_MISC_H


#include "lp_bld.h"
#include <llvm-c/ExecutionEngine.h>


#ifdef __cplusplus
extern "C" {
#endif



extern void
lp_register_oprofile_jit_event_listener(LLVMExecutionEngineRef EE);

extern void
lp_set_target_options(void);


extern void
lp_func_delete_body(LLVMValueRef func);


extern LLVMValueRef
lp_build_load_volatile(LLVMBuilderRef B, LLVMValueRef PointerVal,
                       const char *Name);

extern int
lp_build_create_mcjit_compiler_for_module(LLVMExecutionEngineRef *OutJIT,
                                          LLVMModuleRef M,
                                          unsigned OptLevel,
                                          char **OutError);


#ifdef __cplusplus
}
#endif


#endif /* !LP_BLD_MISC_H */
