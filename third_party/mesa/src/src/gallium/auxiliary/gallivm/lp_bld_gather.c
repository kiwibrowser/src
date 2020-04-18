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


#include "util/u_debug.h"
#include "lp_bld_debug.h"
#include "lp_bld_const.h"
#include "lp_bld_format.h"
#include "lp_bld_gather.h"
#include "lp_bld_init.h"


/**
 * Get the pointer to one element from scatter positions in memory.
 *
 * @sa lp_build_gather()
 */
LLVMValueRef
lp_build_gather_elem_ptr(struct gallivm_state *gallivm,
                         unsigned length,
                         LLVMValueRef base_ptr,
                         LLVMValueRef offsets,
                         unsigned i)
{
   LLVMValueRef offset;
   LLVMValueRef ptr;

   assert(LLVMTypeOf(base_ptr) == LLVMPointerType(LLVMInt8TypeInContext(gallivm->context), 0));

   if (length == 1) {
      assert(i == 0);
      offset = offsets;
   } else {
      LLVMValueRef index = lp_build_const_int32(gallivm, i);
      offset = LLVMBuildExtractElement(gallivm->builder, offsets, index, "");
   }

   ptr = LLVMBuildGEP(gallivm->builder, base_ptr, &offset, 1, "");

   return ptr;
}


/**
 * Gather one element from scatter positions in memory.
 *
 * @sa lp_build_gather()
 */
LLVMValueRef
lp_build_gather_elem(struct gallivm_state *gallivm,
                     unsigned length,
                     unsigned src_width,
                     unsigned dst_width,
                     LLVMValueRef base_ptr,
                     LLVMValueRef offsets,
                     unsigned i)
{
   LLVMTypeRef src_type = LLVMIntTypeInContext(gallivm->context, src_width);
   LLVMTypeRef src_ptr_type = LLVMPointerType(src_type, 0);
   LLVMTypeRef dst_elem_type = LLVMIntTypeInContext(gallivm->context, dst_width);
   LLVMValueRef ptr;
   LLVMValueRef res;

   assert(LLVMTypeOf(base_ptr) == LLVMPointerType(LLVMInt8TypeInContext(gallivm->context), 0));

   ptr = lp_build_gather_elem_ptr(gallivm, length, base_ptr, offsets, i);
   ptr = LLVMBuildBitCast(gallivm->builder, ptr, src_ptr_type, "");
   res = LLVMBuildLoad(gallivm->builder, ptr, "");

   assert(src_width <= dst_width);
   if (src_width > dst_width)
      res = LLVMBuildTrunc(gallivm->builder, res, dst_elem_type, "");
   if (src_width < dst_width)
      res = LLVMBuildZExt(gallivm->builder, res, dst_elem_type, "");

   return res;
}


/**
 * Gather elements from scatter positions in memory into a single vector.
 * Use for fetching texels from a texture.
 * For SSE, typical values are length=4, src_width=32, dst_width=32.
 *
 * @param length length of the offsets
 * @param src_width src element width in bits
 * @param dst_width result element width in bits (src will be expanded to fit)
 * @param base_ptr base pointer, should be a i8 pointer type.
 * @param offsets vector with offsets
 */
LLVMValueRef
lp_build_gather(struct gallivm_state *gallivm,
                unsigned length,
                unsigned src_width,
                unsigned dst_width,
                LLVMValueRef base_ptr,
                LLVMValueRef offsets)
{
   LLVMValueRef res;

   if (length == 1) {
      /* Scalar */
      return lp_build_gather_elem(gallivm, length,
                                  src_width, dst_width,
                                  base_ptr, offsets, 0);
   } else {
      /* Vector */

      LLVMTypeRef dst_elem_type = LLVMIntTypeInContext(gallivm->context, dst_width);
      LLVMTypeRef dst_vec_type = LLVMVectorType(dst_elem_type, length);
      unsigned i;

      res = LLVMGetUndef(dst_vec_type);
      for (i = 0; i < length; ++i) {
         LLVMValueRef index = lp_build_const_int32(gallivm, i);
         LLVMValueRef elem;
         elem = lp_build_gather_elem(gallivm, length,
                                     src_width, dst_width,
                                     base_ptr, offsets, i);
         res = LLVMBuildInsertElement(gallivm->builder, res, elem, index, "");
      }
   }

   return res;
}

LLVMValueRef
lp_build_gather_values(struct gallivm_state * gallivm,
                       LLVMValueRef * values,
                       unsigned value_count)
{
   LLVMTypeRef vec_type = LLVMVectorType(LLVMTypeOf(values[0]), value_count);
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef vec = LLVMGetUndef(vec_type);
   unsigned i;

   for (i = 0; i < value_count; i++) {
      LLVMValueRef index = lp_build_const_int32(gallivm, i);
      vec = LLVMBuildInsertElement(builder, vec, values[i], index, "");
   }
   return vec;
}
