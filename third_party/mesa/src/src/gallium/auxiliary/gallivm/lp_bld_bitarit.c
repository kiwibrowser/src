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

#include "lp_bld_type.h"
#include "lp_bld_debug.h"
#include "lp_bld_const.h"
#include "lp_bld_bitarit.h"


/**
 * Return (a | b)
 */
LLVMValueRef
lp_build_or(struct lp_build_context *bld, LLVMValueRef a, LLVMValueRef b)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMValueRef res;

   assert(lp_check_value(type, a));
   assert(lp_check_value(type, b));

   /* can't do bitwise ops on floating-point values */
   if (type.floating) {
      a = LLVMBuildBitCast(builder, a, bld->int_vec_type, "");
      b = LLVMBuildBitCast(builder, b, bld->int_vec_type, "");
   }

   res = LLVMBuildOr(builder, a, b, "");

   if (type.floating) {
      res = LLVMBuildBitCast(builder, res, bld->vec_type, "");
   }

   return res;
}

/* bitwise XOR (a ^ b) */
LLVMValueRef
lp_build_xor(struct lp_build_context *bld, LLVMValueRef a, LLVMValueRef b)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMValueRef res;

   assert(lp_check_value(type, a));
   assert(lp_check_value(type, b));

   /* can't do bitwise ops on floating-point values */
   if (type.floating) {
      a = LLVMBuildBitCast(builder, a, bld->int_vec_type, "");
      b = LLVMBuildBitCast(builder, b, bld->int_vec_type, "");
   }

   res = LLVMBuildXor(builder, a, b, "");

   if (type.floating) {
      res = LLVMBuildBitCast(builder, res, bld->vec_type, "");
   }

   return res;
}

/**
 * Return (a & b)
 */
LLVMValueRef
lp_build_and(struct lp_build_context *bld, LLVMValueRef a, LLVMValueRef b)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMValueRef res;

   assert(lp_check_value(type, a));
   assert(lp_check_value(type, b));

   /* can't do bitwise ops on floating-point values */
   if (type.floating) {
      a = LLVMBuildBitCast(builder, a, bld->int_vec_type, "");
      b = LLVMBuildBitCast(builder, b, bld->int_vec_type, "");
   }

   res = LLVMBuildAnd(builder, a, b, "");

   if (type.floating) {
      res = LLVMBuildBitCast(builder, res, bld->vec_type, "");
   }

   return res;
}


/**
 * Return (a & ~b)
 */
LLVMValueRef
lp_build_andnot(struct lp_build_context *bld, LLVMValueRef a, LLVMValueRef b)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMValueRef res;

   assert(lp_check_value(type, a));
   assert(lp_check_value(type, b));

   /* can't do bitwise ops on floating-point values */
   if (type.floating) {
      a = LLVMBuildBitCast(builder, a, bld->int_vec_type, "");
      b = LLVMBuildBitCast(builder, b, bld->int_vec_type, "");
   }

   res = LLVMBuildNot(builder, b, "");
   res = LLVMBuildAnd(builder, a, res, "");

   if (type.floating) {
      res = LLVMBuildBitCast(builder, res, bld->vec_type, "");
   }

   return res;
}

/* bitwise NOT */
LLVMValueRef
lp_build_not(struct lp_build_context *bld, LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMValueRef res;

   assert(lp_check_value(type, a));

   if (type.floating) {
      a = LLVMBuildBitCast(builder, a, bld->int_vec_type, "");
   }
   res = LLVMBuildNot(builder, a, "");
   if (type.floating) {
      res = LLVMBuildBitCast(builder, res, bld->vec_type, "");
   }
   return res;
}

/**
 * Shift left.
 */
LLVMValueRef
lp_build_shl(struct lp_build_context *bld, LLVMValueRef a, LLVMValueRef b)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMValueRef res;

   assert(!type.floating);

   assert(lp_check_value(type, a));
   assert(lp_check_value(type, b));

   res = LLVMBuildShl(builder, a, b, "");

   return res;
}


/**
 * Shift right.
 */
LLVMValueRef
lp_build_shr(struct lp_build_context *bld, LLVMValueRef a, LLVMValueRef b)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMValueRef res;

   assert(!type.floating);

   assert(lp_check_value(type, a));
   assert(lp_check_value(type, b));

   if (type.sign) {
      res = LLVMBuildAShr(builder, a, b, "");
   } else {
      res = LLVMBuildLShr(builder, a, b, "");
   }

   return res;
}


/**
 * Shift left with immediate.
 */
LLVMValueRef
lp_build_shl_imm(struct lp_build_context *bld, LLVMValueRef a, unsigned imm)
{
   LLVMValueRef b = lp_build_const_int_vec(bld->gallivm, bld->type, imm);
   assert(imm <= bld->type.width);
   return lp_build_shl(bld, a, b);
}


/**
 * Shift right with immediate.
 */
LLVMValueRef
lp_build_shr_imm(struct lp_build_context *bld, LLVMValueRef a, unsigned imm)
{
   LLVMValueRef b = lp_build_const_int_vec(bld->gallivm, bld->type, imm);
   assert(imm <= bld->type.width);
   return lp_build_shr(bld, a, b);
}
