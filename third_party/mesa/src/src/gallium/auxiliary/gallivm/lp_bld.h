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
 * Wrapper for LLVM header file #includes.
 */


#ifndef LP_BLD_H
#define LP_BLD_H


/**
 * @file
 * LLVM IR building helpers interfaces.
 *
 * We use LLVM-C bindings for now. They are not documented, but follow the C++
 * interfaces very closely, and appear to be complete enough for code
 * genration. See
 * http://npcontemplation.blogspot.com/2008/06/secret-of-llvm-c-bindings.html
 * for a standalone example.
 */

#include <llvm-c/Core.h>  


/** Ensure HAVE_LLVM is set to avoid #ifdef HAVE_LLVM everywhere */
#ifndef HAVE_LLVM
#error "HAVE_LLVM should be set with LLVM's version number, e.g. (0x0207 for 2.7)"
#endif


/**
 * Redefine these LLVM entrypoints as invalid macros to make sure we
 * don't accidentally use them.  We need to use the functions which
 * take an explicit LLVMContextRef parameter.
 */
#define LLVMInt1Type ILLEGAL_LLVM_FUNCTION
#define LLVMInt8Type ILLEGAL_LLVM_FUNCTION
#define LLVMInt16Type ILLEGAL_LLVM_FUNCTION
#define LLVMInt32Type ILLEGAL_LLVM_FUNCTION
#define LLVMInt64Type ILLEGAL_LLVM_FUNCTION
#define LLVMIntType ILLEGAL_LLVM_FUNCTION
#define LLVMFloatType ILLEGAL_LLVM_FUNCTION
#define LLVMDoubleType ILLEGAL_LLVM_FUNCTION
#define LLVMX86FP80Type ILLEGAL_LLVM_FUNCTION
#define LLVMFP128Type ILLEGAL_LLVM_FUNCTION
#define LLVMPPCFP128Type ILLEGAL_LLVM_FUNCTION
#define LLVMStructType ILLEGAL_LLVM_FUNCTION
#define LLVMVoidType ILLEGAL_LLVM_FUNCTION
#define LLVMLabelType ILLEGAL_LLVM_FUNCTION
#define LLVMOpaqueType ILLEGAL_LLVM_FUNCTION
#define LLVMUnionType ILLEGAL_LLVM_FUNCTION
#define LLVMMDString ILLEGAL_LLVM_FUNCTION
#define LLVMMDNode ILLEGAL_LLVM_FUNCTION
#define LLVMConstString ILLEGAL_LLVM_FUNCTION
#define LLVMConstStruct ILLEGAL_LLVM_FUNCTION
#define LLVMAppendBasicBlock ILLEGAL_LLVM_FUNCTION
#define LLVMInsertBasicBlock ILLEGAL_LLVM_FUNCTION
#define LLVMCreateBuilder ILLEGAL_LLVM_FUNCTION

#endif /* LP_BLD_H */
