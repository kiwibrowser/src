/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * @file
 * TGSI to LLVM IR translation -- SoA.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 *
 * Based on tgsi_sse2.c code written by Michal Krol, Keith Whitwell,
 * Brian Paul, and others.
 */

#include "pipe/p_config.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_debug.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_exec.h"
#include "tgsi/tgsi_info.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_scan.h"
#include "lp_bld_tgsi_action.h"
#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_arit.h"
#include "lp_bld_bitarit.h"
#include "lp_bld_gather.h"
#include "lp_bld_init.h"
#include "lp_bld_logic.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_flow.h"
#include "lp_bld_quad.h"
#include "lp_bld_tgsi.h"
#include "lp_bld_limits.h"
#include "lp_bld_debug.h"
#include "lp_bld_printf.h"
#include "lp_bld_sample.h"


static void lp_exec_mask_init(struct lp_exec_mask *mask, struct lp_build_context *bld)
{
   LLVMTypeRef int_type = LLVMInt32TypeInContext(bld->gallivm->context);
   LLVMBuilderRef builder = bld->gallivm->builder;

   mask->bld = bld;
   mask->has_mask = FALSE;
   mask->cond_stack_size = 0;
   mask->loop_stack_size = 0;
   mask->call_stack_size = 0;

   mask->int_vec_type = lp_build_int_vec_type(bld->gallivm, mask->bld->type);
   mask->exec_mask = mask->ret_mask = mask->break_mask = mask->cont_mask = mask->cond_mask =
         LLVMConstAllOnes(mask->int_vec_type);

   mask->loop_limiter = lp_build_alloca(bld->gallivm, int_type, "looplimiter");

   LLVMBuildStore(
      builder,
      LLVMConstInt(int_type, LP_MAX_TGSI_LOOP_ITERATIONS, false),
      mask->loop_limiter);
}

static void lp_exec_mask_update(struct lp_exec_mask *mask)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;

   if (mask->loop_stack_size) {
      /*for loops we need to update the entire mask at runtime */
      LLVMValueRef tmp;
      assert(mask->break_mask);
      tmp = LLVMBuildAnd(builder,
                         mask->cont_mask,
                         mask->break_mask,
                         "maskcb");
      mask->exec_mask = LLVMBuildAnd(builder,
                                     mask->cond_mask,
                                     tmp,
                                     "maskfull");
   } else
      mask->exec_mask = mask->cond_mask;

   if (mask->call_stack_size) {
      mask->exec_mask = LLVMBuildAnd(builder,
                                     mask->exec_mask,
                                     mask->ret_mask,
                                     "callmask");
   }

   mask->has_mask = (mask->cond_stack_size > 0 ||
                     mask->loop_stack_size > 0 ||
                     mask->call_stack_size > 0);
}

static void lp_exec_mask_cond_push(struct lp_exec_mask *mask,
                                   LLVMValueRef val)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;

   assert(mask->cond_stack_size < LP_MAX_TGSI_NESTING);
   if (mask->cond_stack_size == 0) {
      assert(mask->cond_mask == LLVMConstAllOnes(mask->int_vec_type));
   }
   mask->cond_stack[mask->cond_stack_size++] = mask->cond_mask;
   assert(LLVMTypeOf(val) == mask->int_vec_type);
   mask->cond_mask = LLVMBuildAnd(builder,
                                  mask->cond_mask,
                                  val,
                                  "");
   lp_exec_mask_update(mask);
}

static void lp_exec_mask_cond_invert(struct lp_exec_mask *mask)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   LLVMValueRef prev_mask;
   LLVMValueRef inv_mask;

   assert(mask->cond_stack_size);
   prev_mask = mask->cond_stack[mask->cond_stack_size - 1];
   if (mask->cond_stack_size == 1) {
      assert(prev_mask == LLVMConstAllOnes(mask->int_vec_type));
   }

   inv_mask = LLVMBuildNot(builder, mask->cond_mask, "");

   mask->cond_mask = LLVMBuildAnd(builder,
                                  inv_mask,
                                  prev_mask, "");
   lp_exec_mask_update(mask);
}

static void lp_exec_mask_cond_pop(struct lp_exec_mask *mask)
{
   assert(mask->cond_stack_size);
   mask->cond_mask = mask->cond_stack[--mask->cond_stack_size];
   lp_exec_mask_update(mask);
}

static void lp_exec_bgnloop(struct lp_exec_mask *mask)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;

   if (mask->loop_stack_size == 0) {
      assert(mask->loop_block == NULL);
      assert(mask->cont_mask == LLVMConstAllOnes(mask->int_vec_type));
      assert(mask->break_mask == LLVMConstAllOnes(mask->int_vec_type));
      assert(mask->break_var == NULL);
   }

   assert(mask->loop_stack_size < LP_MAX_TGSI_NESTING);

   mask->loop_stack[mask->loop_stack_size].loop_block = mask->loop_block;
   mask->loop_stack[mask->loop_stack_size].cont_mask = mask->cont_mask;
   mask->loop_stack[mask->loop_stack_size].break_mask = mask->break_mask;
   mask->loop_stack[mask->loop_stack_size].break_var = mask->break_var;
   ++mask->loop_stack_size;

   mask->break_var = lp_build_alloca(mask->bld->gallivm, mask->int_vec_type, "");
   LLVMBuildStore(builder, mask->break_mask, mask->break_var);

   mask->loop_block = lp_build_insert_new_block(mask->bld->gallivm, "bgnloop");

   LLVMBuildBr(builder, mask->loop_block);
   LLVMPositionBuilderAtEnd(builder, mask->loop_block);

   mask->break_mask = LLVMBuildLoad(builder, mask->break_var, "");

   lp_exec_mask_update(mask);
}

static void lp_exec_break(struct lp_exec_mask *mask)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   LLVMValueRef exec_mask = LLVMBuildNot(builder,
                                         mask->exec_mask,
                                         "break");

   mask->break_mask = LLVMBuildAnd(builder,
                                   mask->break_mask,
                                   exec_mask, "break_full");

   lp_exec_mask_update(mask);
}

static void lp_exec_continue(struct lp_exec_mask *mask)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   LLVMValueRef exec_mask = LLVMBuildNot(builder,
                                         mask->exec_mask,
                                         "");

   mask->cont_mask = LLVMBuildAnd(builder,
                                  mask->cont_mask,
                                  exec_mask, "");

   lp_exec_mask_update(mask);
}


static void lp_exec_endloop(struct gallivm_state *gallivm,
                            struct lp_exec_mask *mask)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   LLVMBasicBlockRef endloop;
   LLVMTypeRef int_type = LLVMInt32TypeInContext(mask->bld->gallivm->context);
   LLVMTypeRef reg_type = LLVMIntTypeInContext(gallivm->context,
                                               mask->bld->type.width *
                                               mask->bld->type.length);
   LLVMValueRef i1cond, i2cond, icond, limiter;

   assert(mask->break_mask);

   /*
    * Restore the cont_mask, but don't pop
    */
   assert(mask->loop_stack_size);
   mask->cont_mask = mask->loop_stack[mask->loop_stack_size - 1].cont_mask;
   lp_exec_mask_update(mask);

   /*
    * Unlike the continue mask, the break_mask must be preserved across loop
    * iterations
    */
   LLVMBuildStore(builder, mask->break_mask, mask->break_var);

   /* Decrement the loop limiter */
   limiter = LLVMBuildLoad(builder, mask->loop_limiter, "");

   limiter = LLVMBuildSub(
      builder,
      limiter,
      LLVMConstInt(int_type, 1, false),
      "");

   LLVMBuildStore(builder, limiter, mask->loop_limiter);

   /* i1cond = (mask != 0) */
   i1cond = LLVMBuildICmp(
      builder,
      LLVMIntNE,
      LLVMBuildBitCast(builder, mask->exec_mask, reg_type, ""),
      LLVMConstNull(reg_type), "");

   /* i2cond = (looplimiter > 0) */
   i2cond = LLVMBuildICmp(
      builder,
      LLVMIntSGT,
      limiter,
      LLVMConstNull(int_type), "");

   /* if( i1cond && i2cond ) */
   icond = LLVMBuildAnd(builder, i1cond, i2cond, "");

   endloop = lp_build_insert_new_block(mask->bld->gallivm, "endloop");

   LLVMBuildCondBr(builder,
                   icond, mask->loop_block, endloop);

   LLVMPositionBuilderAtEnd(builder, endloop);

   assert(mask->loop_stack_size);
   --mask->loop_stack_size;
   mask->loop_block = mask->loop_stack[mask->loop_stack_size].loop_block;
   mask->cont_mask = mask->loop_stack[mask->loop_stack_size].cont_mask;
   mask->break_mask = mask->loop_stack[mask->loop_stack_size].break_mask;
   mask->break_var = mask->loop_stack[mask->loop_stack_size].break_var;

   lp_exec_mask_update(mask);
}

/* stores val into an address pointed to by dst.
 * mask->exec_mask is used to figure out which bits of val
 * should be stored into the address
 * (0 means don't store this bit, 1 means do store).
 */
static void lp_exec_mask_store(struct lp_exec_mask *mask,
                               struct lp_build_context *bld_store,
                               LLVMValueRef pred,
                               LLVMValueRef val,
                               LLVMValueRef dst)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;

   /* Mix the predicate and execution mask */
   if (mask->has_mask) {
      if (pred) {
         pred = LLVMBuildAnd(builder, pred, mask->exec_mask, "");
      } else {
         pred = mask->exec_mask;
      }
   }

   if (pred) {
      LLVMValueRef real_val, dst_val;

      dst_val = LLVMBuildLoad(builder, dst, "");
      real_val = lp_build_select(bld_store,
                                 pred,
                                 val, dst_val);

      LLVMBuildStore(builder, real_val, dst);
   } else
      LLVMBuildStore(builder, val, dst);
}

static void lp_exec_mask_call(struct lp_exec_mask *mask,
                              int func,
                              int *pc)
{
   assert(mask->call_stack_size < LP_MAX_TGSI_NESTING);
   mask->call_stack[mask->call_stack_size].pc = *pc;
   mask->call_stack[mask->call_stack_size].ret_mask = mask->ret_mask;
   mask->call_stack_size++;
   *pc = func;
}

static void lp_exec_mask_ret(struct lp_exec_mask *mask, int *pc)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   LLVMValueRef exec_mask;

   if (mask->call_stack_size == 0) {
      /* returning from main() */
      *pc = -1;
      return;
   }
   exec_mask = LLVMBuildNot(builder,
                            mask->exec_mask,
                            "ret");

   mask->ret_mask = LLVMBuildAnd(builder,
                                 mask->ret_mask,
                                 exec_mask, "ret_full");

   lp_exec_mask_update(mask);
}

static void lp_exec_mask_bgnsub(struct lp_exec_mask *mask)
{
}

static void lp_exec_mask_endsub(struct lp_exec_mask *mask, int *pc)
{
   assert(mask->call_stack_size);
   mask->call_stack_size--;
   *pc = mask->call_stack[mask->call_stack_size].pc;
   mask->ret_mask = mask->call_stack[mask->call_stack_size].ret_mask;
   lp_exec_mask_update(mask);
}


/**
 * Return pointer to a temporary register channel (src or dest).
 * Note that indirect addressing cannot be handled here.
 * \param index  which temporary register
 * \param chan  which channel of the temp register.
 */
LLVMValueRef
lp_get_temp_ptr_soa(struct lp_build_tgsi_soa_context *bld,
             unsigned index,
             unsigned chan)
{
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   assert(chan < 4);
   if (bld->indirect_files & (1 << TGSI_FILE_TEMPORARY)) {
      LLVMValueRef lindex = lp_build_const_int32(bld->bld_base.base.gallivm, index * 4 + chan);
      return LLVMBuildGEP(builder, bld->temps_array, &lindex, 1, "");
   }
   else {
      return bld->temps[index][chan];
   }
}

/**
 * Return pointer to a output register channel (src or dest).
 * Note that indirect addressing cannot be handled here.
 * \param index  which output register
 * \param chan  which channel of the output register.
 */
LLVMValueRef
lp_get_output_ptr(struct lp_build_tgsi_soa_context *bld,
               unsigned index,
               unsigned chan)
{
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   assert(chan < 4);
   if (bld->indirect_files & (1 << TGSI_FILE_OUTPUT)) {
      LLVMValueRef lindex = lp_build_const_int32(bld->bld_base.base.gallivm,
                                                 index * 4 + chan);
      return LLVMBuildGEP(builder, bld->outputs_array, &lindex, 1, "");
   }
   else {
      return bld->outputs[index][chan];
   }
}

/**
 * Gather vector.
 * XXX the lp_build_gather() function should be capable of doing this
 * with a little work.
 */
static LLVMValueRef
build_gather(struct lp_build_context *bld,
             LLVMValueRef base_ptr,
             LLVMValueRef indexes)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef res = bld->undef;
   unsigned i;

   /*
    * Loop over elements of index_vec, load scalar value, insert it into 'res'.
    */
   for (i = 0; i < bld->type.length; i++) {
      LLVMValueRef ii = lp_build_const_int32(bld->gallivm, i);
      LLVMValueRef index = LLVMBuildExtractElement(builder,
                                                   indexes, ii, "");
      LLVMValueRef scalar_ptr = LLVMBuildGEP(builder, base_ptr,
                                             &index, 1, "gather_ptr");
      LLVMValueRef scalar = LLVMBuildLoad(builder, scalar_ptr, "");

      res = LLVMBuildInsertElement(builder, res, scalar, ii, "");
   }

   return res;
}


/**
 * Scatter/store vector.
 */
static void
emit_mask_scatter(struct lp_build_tgsi_soa_context *bld,
                  LLVMValueRef base_ptr,
                  LLVMValueRef indexes,
                  LLVMValueRef values,
                  struct lp_exec_mask *mask,
                  LLVMValueRef pred)
{
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   unsigned i;

   /* Mix the predicate and execution mask */
   if (mask->has_mask) {
      if (pred) {
         pred = LLVMBuildAnd(builder, pred, mask->exec_mask, "");
      }
      else {
         pred = mask->exec_mask;
      }
   }

   /*
    * Loop over elements of index_vec, store scalar value.
    */
   for (i = 0; i < bld->bld_base.base.type.length; i++) {
      LLVMValueRef ii = lp_build_const_int32(gallivm, i);
      LLVMValueRef index = LLVMBuildExtractElement(builder, indexes, ii, "");
      LLVMValueRef scalar_ptr = LLVMBuildGEP(builder, base_ptr, &index, 1, "scatter_ptr");
      LLVMValueRef val = LLVMBuildExtractElement(builder, values, ii, "scatter_val");
      LLVMValueRef scalar_pred = pred ?
         LLVMBuildExtractElement(builder, pred, ii, "scatter_pred") : NULL;

      if (0)
         lp_build_printf(gallivm, "scatter %d: val %f at %d %p\n",
                         ii, val, index, scalar_ptr);

      if (scalar_pred) {
         LLVMValueRef real_val, dst_val;
         dst_val = LLVMBuildLoad(builder, scalar_ptr, "");
         real_val = lp_build_select(&bld->elem_bld, scalar_pred, val, dst_val);
         LLVMBuildStore(builder, real_val, scalar_ptr);
      }
      else {
         LLVMBuildStore(builder, val, scalar_ptr);
      }
   }
}


/**
 * Read the current value of the ADDR register, convert the floats to
 * ints, add the base index and return the vector of offsets.
 * The offsets will be used to index into the constant buffer or
 * temporary register file.
 */
static LLVMValueRef
get_indirect_index(struct lp_build_tgsi_soa_context *bld,
                   unsigned reg_file, unsigned reg_index,
                   const struct tgsi_src_register *indirect_reg)
{
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   struct lp_build_context *uint_bld = &bld->bld_base.uint_bld;
   /* always use X component of address register */
   unsigned swizzle = indirect_reg->SwizzleX;
   LLVMValueRef base;
   LLVMValueRef rel;
   LLVMValueRef max_index;
   LLVMValueRef index;

   assert(bld->indirect_files & (1 << reg_file));

   base = lp_build_const_int_vec(bld->bld_base.base.gallivm, uint_bld->type, reg_index);

   assert(swizzle < 4);
   rel = LLVMBuildLoad(builder,
                        bld->addr[indirect_reg->Index][swizzle],
                        "load addr reg");

   index = lp_build_add(uint_bld, base, rel);

   max_index = lp_build_const_int_vec(bld->bld_base.base.gallivm,
                                      uint_bld->type,
                                      bld->bld_base.info->file_max[reg_file]);

   assert(!uint_bld->type.sign);
   index = lp_build_min(uint_bld, index, max_index);

   return index;
}

static struct lp_build_context *
stype_to_fetch(struct lp_build_tgsi_context * bld_base,
	       enum tgsi_opcode_type stype)
{
   struct lp_build_context *bld_fetch;

   switch (stype) {
   case TGSI_TYPE_FLOAT:
   case TGSI_TYPE_UNTYPED:
      bld_fetch = &bld_base->base;
      break;
   case TGSI_TYPE_UNSIGNED:
      bld_fetch = &bld_base->uint_bld;
      break;
   case TGSI_TYPE_SIGNED:
      bld_fetch = &bld_base->int_bld;
      break;
   case TGSI_TYPE_VOID:
   case TGSI_TYPE_DOUBLE:
   default:
      assert(0);
      bld_fetch = NULL;
      break;
   }
   return bld_fetch;
}

static LLVMValueRef
emit_fetch_constant(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_full_src_register * reg,
   enum tgsi_opcode_type stype,
   unsigned swizzle)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   LLVMValueRef indirect_index = NULL;
   struct lp_build_context *bld_fetch = stype_to_fetch(bld_base, stype);
   
   /* XXX: Handle fetching xyzw components as a vector */
   assert(swizzle != ~0);

   if (reg->Register.Indirect) {
      indirect_index = get_indirect_index(bld,
                                          reg->Register.File,
                                          reg->Register.Index,
                                          &reg->Indirect);
   }

   if (reg->Register.Indirect) {
      LLVMValueRef swizzle_vec =
         lp_build_const_int_vec(bld->bld_base.base.gallivm, uint_bld->type, swizzle);
      LLVMValueRef index_vec;  /* index into the const buffer */

      /* index_vec = indirect_index * 4 + swizzle */
      index_vec = lp_build_shl_imm(uint_bld, indirect_index, 2);
      index_vec = lp_build_add(uint_bld, index_vec, swizzle_vec);

      /* Gather values from the constant buffer */
      return build_gather(bld_fetch, bld->consts_ptr, index_vec);
   }
   else {
      LLVMValueRef index;  /* index into the const buffer */
      LLVMValueRef scalar, scalar_ptr;

      index = lp_build_const_int32(gallivm, reg->Register.Index*4 + swizzle);

      scalar_ptr = LLVMBuildGEP(builder, bld->consts_ptr,
                                   &index, 1, "");

      if (stype != TGSI_TYPE_FLOAT && stype != TGSI_TYPE_UNTYPED) {
         LLVMTypeRef ivtype = LLVMPointerType(LLVMInt32TypeInContext(gallivm->context), 0);
         LLVMValueRef temp_ptr;
         temp_ptr = LLVMBuildBitCast(builder, scalar_ptr, ivtype, "");
         scalar = LLVMBuildLoad(builder, temp_ptr, "");
      } else
         scalar = LLVMBuildLoad(builder, scalar_ptr, "");

      return lp_build_broadcast_scalar(bld_fetch, scalar);
   }
}

static LLVMValueRef
emit_fetch_immediate(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_full_src_register * reg,
   enum tgsi_opcode_type stype,
   unsigned swizzle)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   LLVMValueRef res = bld->immediates[reg->Register.Index][swizzle];
   assert(res);

   if (stype == TGSI_TYPE_UNSIGNED) {
      res = LLVMConstBitCast(res, bld_base->uint_bld.vec_type);
   } else if (stype == TGSI_TYPE_SIGNED) {
      res = LLVMConstBitCast(res, bld_base->int_bld.vec_type);
   }
   return res;
}

static LLVMValueRef
emit_fetch_input(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_full_src_register * reg,
   enum tgsi_opcode_type stype,
   unsigned swizzle)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   LLVMValueRef indirect_index = NULL;
   LLVMValueRef res;

   if (reg->Register.Indirect) {
      indirect_index = get_indirect_index(bld,
                                          reg->Register.File,
                                          reg->Register.Index,
                                          &reg->Indirect);
   }

   if (reg->Register.Indirect) {
      LLVMValueRef swizzle_vec =
         lp_build_const_int_vec(gallivm, uint_bld->type, swizzle);
      LLVMValueRef length_vec =
         lp_build_const_int_vec(gallivm, uint_bld->type, bld->bld_base.base.type.length);
      LLVMValueRef index_vec;  /* index into the const buffer */
      LLVMValueRef inputs_array;
      LLVMTypeRef float4_ptr_type;

      /* index_vec = (indirect_index * 4 + swizzle) * length */
      index_vec = lp_build_shl_imm(uint_bld, indirect_index, 2);
      index_vec = lp_build_add(uint_bld, index_vec, swizzle_vec);
      index_vec = lp_build_mul(uint_bld, index_vec, length_vec);

      /* cast inputs_array pointer to float* */
      float4_ptr_type = LLVMPointerType(LLVMFloatTypeInContext(gallivm->context), 0);
      inputs_array = LLVMBuildBitCast(builder, bld->inputs_array,
                                         float4_ptr_type, "");

      /* Gather values from the temporary register array */
      res = build_gather(&bld_base->base, inputs_array, index_vec);
   } else {
      if (bld->indirect_files & (1 << TGSI_FILE_INPUT)) {
         LLVMValueRef lindex = lp_build_const_int32(gallivm,
                                        reg->Register.Index * 4 + swizzle);
         LLVMValueRef input_ptr =  LLVMBuildGEP(builder,
                                                bld->inputs_array, &lindex, 1, "");
         res = LLVMBuildLoad(builder, input_ptr, "");
      }
      else {
         res = bld->inputs[reg->Register.Index][swizzle];
      }
   }

   assert(res);

   if (stype == TGSI_TYPE_UNSIGNED) {
      res = LLVMBuildBitCast(builder, res, bld_base->uint_bld.vec_type, "");
   } else if (stype == TGSI_TYPE_SIGNED) {
      res = LLVMBuildBitCast(builder, res, bld_base->int_bld.vec_type, "");
   }

   return res;
}

static LLVMValueRef
emit_fetch_temporary(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_full_src_register * reg,
   enum tgsi_opcode_type stype,
   unsigned swizzle)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   LLVMValueRef indirect_index = NULL;
   LLVMValueRef res;

   if (reg->Register.Indirect) {
      indirect_index = get_indirect_index(bld,
                                          reg->Register.File,
                                          reg->Register.Index,
                                          &reg->Indirect);
   }

   if (reg->Register.Indirect) {
      LLVMValueRef swizzle_vec =
         lp_build_const_int_vec(bld->bld_base.base.gallivm, uint_bld->type, swizzle);
      LLVMValueRef length_vec =
         lp_build_const_int_vec(bld->bld_base.base.gallivm, uint_bld->type,
                                bld->bld_base.base.type.length);
      LLVMValueRef index_vec;  /* index into the const buffer */
      LLVMValueRef temps_array;
      LLVMTypeRef float4_ptr_type;

      /* index_vec = (indirect_index * 4 + swizzle) * length */
      index_vec = lp_build_shl_imm(uint_bld, indirect_index, 2);
      index_vec = lp_build_add(uint_bld, index_vec, swizzle_vec);
      index_vec = lp_build_mul(uint_bld, index_vec, length_vec);

      /* cast temps_array pointer to float* */
      float4_ptr_type = LLVMPointerType(LLVMFloatTypeInContext(bld->bld_base.base.gallivm->context), 0);
      temps_array = LLVMBuildBitCast(builder, bld->temps_array,
                                     float4_ptr_type, "");

      /* Gather values from the temporary register array */
      res = build_gather(&bld_base->base, temps_array, index_vec);
   }
   else {
      LLVMValueRef temp_ptr;
      if (stype != TGSI_TYPE_FLOAT && stype != TGSI_TYPE_UNTYPED) {
         LLVMTypeRef itype = LLVMPointerType(bld->bld_base.int_bld.vec_type, 0);
         LLVMValueRef tint_ptr = lp_get_temp_ptr_soa(bld, reg->Register.Index,
                                                     swizzle);
         temp_ptr = LLVMBuildBitCast(builder, tint_ptr, itype, "");
      } else
         temp_ptr = lp_get_temp_ptr_soa(bld, reg->Register.Index, swizzle);
      res = LLVMBuildLoad(builder, temp_ptr, "");
      if (!res)
         return bld->bld_base.base.undef;
   }

   return res;
}

static LLVMValueRef
emit_fetch_system_value(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_full_src_register * reg,
   enum tgsi_opcode_type stype,
   unsigned swizzle)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   const struct tgsi_shader_info *info = bld->bld_base.info;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef res;
   enum tgsi_opcode_type atype; // Actual type of the value

   assert(!reg->Register.Indirect);

   switch (info->system_value_semantic_name[reg->Register.Index]) {
   case TGSI_SEMANTIC_INSTANCEID:
      res = lp_build_broadcast_scalar(&bld_base->uint_bld, bld->system_values.instance_id);
      atype = TGSI_TYPE_UNSIGNED;
      break;

   case TGSI_SEMANTIC_VERTEXID:
      res = bld->system_values.vertex_id;
      atype = TGSI_TYPE_UNSIGNED;
      break;

   default:
      assert(!"unexpected semantic in emit_fetch_system_value");
      res = bld_base->base.zero;
      atype = TGSI_TYPE_FLOAT;
      break;
   }

   if (atype != stype) {
      if (stype == TGSI_TYPE_FLOAT) {
         res = LLVMBuildBitCast(builder, res, bld_base->base.vec_type, "");
      } else if (stype == TGSI_TYPE_UNSIGNED) {
         res = LLVMBuildBitCast(builder, res, bld_base->uint_bld.vec_type, "");
      } else if (stype == TGSI_TYPE_SIGNED) {
         res = LLVMBuildBitCast(builder, res, bld_base->int_bld.vec_type, "");
      }
   }

   return res;
}

/**
 * Register fetch with derivatives.
 */
static void
emit_fetch_deriv(
   struct lp_build_tgsi_soa_context *bld,
   LLVMValueRef src,
   LLVMValueRef *res,
   LLVMValueRef *ddx,
   LLVMValueRef *ddy)
{
   if(res)
      *res = src;

   /* TODO: use interpolation coeffs for inputs */

   if(ddx)
      *ddx = lp_build_ddx(&bld->bld_base.base, src);

   if(ddy)
      *ddy = lp_build_ddy(&bld->bld_base.base, src);
}


/**
 * Predicate.
 */
static void
emit_fetch_predicate(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_instruction *inst,
   LLVMValueRef *pred)
{
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   unsigned index;
   unsigned char swizzles[4];
   LLVMValueRef unswizzled[4] = {NULL, NULL, NULL, NULL};
   LLVMValueRef value;
   unsigned chan;

   if (!inst->Instruction.Predicate) {
      TGSI_FOR_EACH_CHANNEL( chan ) {
         pred[chan] = NULL;
      }
      return;
   }

   swizzles[0] = inst->Predicate.SwizzleX;
   swizzles[1] = inst->Predicate.SwizzleY;
   swizzles[2] = inst->Predicate.SwizzleZ;
   swizzles[3] = inst->Predicate.SwizzleW;

   index = inst->Predicate.Index;
   assert(index < LP_MAX_TGSI_PREDS);

   TGSI_FOR_EACH_CHANNEL( chan ) {
      unsigned swizzle = swizzles[chan];

      /*
       * Only fetch the predicate register channels that are actually listed
       * in the swizzles
       */
      if (!unswizzled[swizzle]) {
         value = LLVMBuildLoad(builder,
                               bld->preds[index][swizzle], "");

         /*
          * Convert the value to an integer mask.
          *
          * TODO: Short-circuit this comparison -- a D3D setp_xx instructions
          * is needlessly causing two comparisons due to storing the intermediate
          * result as float vector instead of an integer mask vector.
          */
         value = lp_build_compare(bld->bld_base.base.gallivm,
                                  bld->bld_base.base.type,
                                  PIPE_FUNC_NOTEQUAL,
                                  value,
                                  bld->bld_base.base.zero);
         if (inst->Predicate.Negate) {
            value = LLVMBuildNot(builder, value, "");
         }

         unswizzled[swizzle] = value;
      } else {
         value = unswizzled[swizzle];
      }

      pred[chan] = value;
   }
}

/**
 * Register store.
 */
static void
emit_store_chan(
   struct lp_build_tgsi_context *bld_base,
   const struct tgsi_full_instruction *inst,
   unsigned index,
   unsigned chan_index,
   LLVMValueRef pred,
   LLVMValueRef value)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   const struct tgsi_full_dst_register *reg = &inst->Dst[index];
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   LLVMValueRef indirect_index = NULL;
   struct lp_build_context *bld_store;
   enum tgsi_opcode_type dtype = tgsi_opcode_infer_dst_type(inst->Instruction.Opcode);

   switch (dtype) {
   default:
   case TGSI_TYPE_FLOAT:
   case TGSI_TYPE_UNTYPED:
      bld_store = &bld_base->base;
      break;
   case TGSI_TYPE_UNSIGNED:
      bld_store = &bld_base->uint_bld;
      break;
   case TGSI_TYPE_SIGNED:
      bld_store = &bld_base->int_bld;
      break;
   case TGSI_TYPE_DOUBLE:
   case TGSI_TYPE_VOID:
      assert(0);
      bld_store = NULL;
      break;
   }

   switch( inst->Instruction.Saturate ) {
   case TGSI_SAT_NONE:
      break;

   case TGSI_SAT_ZERO_ONE:
      value = lp_build_max(&bld->bld_base.base, value, bld->bld_base.base.zero);
      value = lp_build_min(&bld->bld_base.base, value, bld->bld_base.base.one);
      break;

   case TGSI_SAT_MINUS_PLUS_ONE:
      value = lp_build_max(&bld->bld_base.base, value, lp_build_const_vec(bld->bld_base.base.gallivm, bld->bld_base.base.type, -1.0));
      value = lp_build_min(&bld->bld_base.base, value, bld->bld_base.base.one);
      break;

   default:
      assert(0);
   }

   if (reg->Register.Indirect) {
      indirect_index = get_indirect_index(bld,
                                          reg->Register.File,
                                          reg->Register.Index,
                                          &reg->Indirect);
   } else {
      assert(reg->Register.Index <=
                             bld->bld_base.info->file_max[reg->Register.File]);
   }

   switch( reg->Register.File ) {
   case TGSI_FILE_OUTPUT:
      if (reg->Register.Indirect) {
         LLVMValueRef chan_vec =
            lp_build_const_int_vec(gallivm, uint_bld->type, chan_index);
         LLVMValueRef length_vec =
            lp_build_const_int_vec(gallivm, uint_bld->type, bld->bld_base.base.type.length);
         LLVMValueRef index_vec;  /* indexes into the temp registers */
         LLVMValueRef outputs_array;
         LLVMValueRef pixel_offsets;
         LLVMTypeRef float_ptr_type;
         int i;

         /* build pixel offset vector: {0, 1, 2, 3, ...} */
         pixel_offsets = uint_bld->undef;
         for (i = 0; i < bld->bld_base.base.type.length; i++) {
            LLVMValueRef ii = lp_build_const_int32(gallivm, i);
            pixel_offsets = LLVMBuildInsertElement(builder, pixel_offsets,
                                                   ii, ii, "");
         }

         /* index_vec = (indirect_index * 4 + chan_index) * length + offsets */
         index_vec = lp_build_shl_imm(uint_bld, indirect_index, 2);
         index_vec = lp_build_add(uint_bld, index_vec, chan_vec);
         index_vec = lp_build_mul(uint_bld, index_vec, length_vec);
         index_vec = lp_build_add(uint_bld, index_vec, pixel_offsets);

         float_ptr_type =
            LLVMPointerType(LLVMFloatTypeInContext(gallivm->context), 0);
         outputs_array = LLVMBuildBitCast(builder, bld->outputs_array,
                                          float_ptr_type, "");

         /* Scatter store values into temp registers */
         emit_mask_scatter(bld, outputs_array, index_vec, value,
                           &bld->exec_mask, pred);
      }
      else {
         LLVMValueRef out_ptr = lp_get_output_ptr(bld, reg->Register.Index,
                                               chan_index);
         lp_exec_mask_store(&bld->exec_mask, bld_store, pred, value, out_ptr);
      }
      break;

   case TGSI_FILE_TEMPORARY:
      if (reg->Register.Indirect) {
         LLVMValueRef chan_vec =
            lp_build_const_int_vec(gallivm, uint_bld->type, chan_index);
         LLVMValueRef length_vec =
            lp_build_const_int_vec(gallivm, uint_bld->type,
                                   bld->bld_base.base.type.length);
         LLVMValueRef index_vec;  /* indexes into the temp registers */
         LLVMValueRef temps_array;
         LLVMValueRef pixel_offsets;
         LLVMTypeRef float_ptr_type;
         int i;

         /* build pixel offset vector: {0, 1, 2, 3, ...} */
         pixel_offsets = uint_bld->undef; 
         for (i = 0; i < bld->bld_base.base.type.length; i++) {
            LLVMValueRef ii = lp_build_const_int32(gallivm, i);
            pixel_offsets = LLVMBuildInsertElement(builder, pixel_offsets,
                                                   ii, ii, "");
         }

         /* index_vec = (indirect_index * 4 + chan_index) * length + offsets */
         index_vec = lp_build_shl_imm(uint_bld, indirect_index, 2);
         index_vec = lp_build_add(uint_bld, index_vec, chan_vec);
         index_vec = lp_build_mul(uint_bld, index_vec, length_vec);
         index_vec = lp_build_add(uint_bld, index_vec, pixel_offsets);

         float_ptr_type =
            LLVMPointerType(LLVMFloatTypeInContext(gallivm->context), 0);
         temps_array = LLVMBuildBitCast(builder, bld->temps_array,
                                        float_ptr_type, "");

         /* Scatter store values into temp registers */
         emit_mask_scatter(bld, temps_array, index_vec, value,
                           &bld->exec_mask, pred);
      }
      else {
         LLVMValueRef temp_ptr;

         switch (dtype) {
         case TGSI_TYPE_UNSIGNED:
         case TGSI_TYPE_SIGNED: {
            LLVMTypeRef itype = bld_base->int_bld.vec_type;
            LLVMTypeRef ivtype = LLVMPointerType(itype, 0);
            LLVMValueRef tint_ptr = lp_get_temp_ptr_soa(bld, reg->Register.Index,
                                                        chan_index);
            LLVMValueRef temp_value_ptr;

            temp_ptr = LLVMBuildBitCast(builder, tint_ptr, ivtype, "");
            temp_value_ptr = LLVMBuildBitCast(builder, value, itype, "");
            value = temp_value_ptr;
            break;
         }
         default:
         case TGSI_TYPE_FLOAT:
         case TGSI_TYPE_UNTYPED:
            temp_ptr = lp_get_temp_ptr_soa(bld, reg->Register.Index,
                                           chan_index);
            break;
         }

         lp_exec_mask_store(&bld->exec_mask, bld_store, pred, value, temp_ptr);
      }
      break;

   case TGSI_FILE_ADDRESS:
      assert(dtype == TGSI_TYPE_SIGNED);
      assert(LLVMTypeOf(value) == bld_base->base.int_vec_type);
      lp_exec_mask_store(&bld->exec_mask, bld_store, pred, value,
                         bld->addr[reg->Register.Index][chan_index]);
      break;

   case TGSI_FILE_PREDICATE:
      lp_exec_mask_store(&bld->exec_mask, bld_store, pred, value,
                         bld->preds[reg->Register.Index][chan_index]);
      break;

   default:
      assert( 0 );
   }
}

static void
emit_store(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_full_instruction * inst,
   const struct tgsi_opcode_info * info,
   LLVMValueRef dst[4])

{
   unsigned chan_index;
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   if(info->num_dst) {
      LLVMValueRef pred[TGSI_NUM_CHANNELS];

      emit_fetch_predicate( bld, inst, pred );

      TGSI_FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         emit_store_chan(bld_base, inst, 0, chan_index, pred[chan_index], dst[chan_index]);
      }
   }
}

/**
 * High-level instruction translators.
 */

static void
emit_tex( struct lp_build_tgsi_soa_context *bld,
          const struct tgsi_full_instruction *inst,
          enum lp_build_tex_modifier modifier,
          LLVMValueRef *texel)
{
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   unsigned unit;
   LLVMValueRef lod_bias, explicit_lod;
   LLVMValueRef oow = NULL;
   LLVMValueRef coords[3];
   struct lp_derivatives derivs;
   unsigned num_coords;
   unsigned dims;
   unsigned i;

   if (!bld->sampler) {
      _debug_printf("warning: found texture instruction but no sampler generator supplied\n");
      for (i = 0; i < 4; i++) {
         texel[i] = bld->bld_base.base.undef;
      }
      return;
   }

   derivs.ddx_ddy[0] = bld->bld_base.base.undef;
   derivs.ddx_ddy[1] = bld->bld_base.base.undef;

   switch (inst->Texture.Texture) {
   case TGSI_TEXTURE_1D:
      num_coords = 1;
      dims = 1;
      break;
   case TGSI_TEXTURE_1D_ARRAY:
      num_coords = 2;
      dims = 1;
      break;
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
      num_coords = 2;
      dims = 2;
      break;
   case TGSI_TEXTURE_SHADOW1D:
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
      num_coords = 3;
      dims = 1;
      break;
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
   case TGSI_TEXTURE_2D_ARRAY:
   case TGSI_TEXTURE_CUBE:
      num_coords = 3;
      dims = 2;
      break;
   case TGSI_TEXTURE_3D:
      num_coords = 3;
      dims = 3;
      break;
   case TGSI_TEXTURE_SHADOW2D_ARRAY:
      num_coords = 4;
      dims = 2;
      break;
   default:
      assert(0);
      return;
   }

   if (modifier == LP_BLD_TEX_MODIFIER_LOD_BIAS) {
      lod_bias = lp_build_emit_fetch( &bld->bld_base, inst, 0, 3 );
      explicit_lod = NULL;
   }
   else if (modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_LOD) {
      lod_bias = NULL;
      explicit_lod = lp_build_emit_fetch( &bld->bld_base, inst, 0, 3 );
   }
   else {
      lod_bias = NULL;
      explicit_lod = NULL;
   }

   if (modifier == LP_BLD_TEX_MODIFIER_PROJECTED) {
      oow = lp_build_emit_fetch( &bld->bld_base, inst, 0, 3 );
      oow = lp_build_rcp(&bld->bld_base.base, oow);
   }

   for (i = 0; i < num_coords; i++) {
      coords[i] = lp_build_emit_fetch( &bld->bld_base, inst, 0, i );
      if (modifier == LP_BLD_TEX_MODIFIER_PROJECTED)
         coords[i] = lp_build_mul(&bld->bld_base.base, coords[i], oow);
   }
   for (i = num_coords; i < 3; i++) {
      coords[i] = bld->bld_base.base.undef;
   }

   if (modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_DERIV) {
      LLVMValueRef i32undef = LLVMGetUndef(LLVMInt32TypeInContext(gallivm->context));
      LLVMValueRef shuffles[LP_MAX_VECTOR_LENGTH];
      LLVMValueRef ddxdyonec[3];
      unsigned length = bld->bld_base.base.type.length;
      unsigned num_quads = length / 4;
      unsigned dim;
      unsigned quad;

      for (dim = 0; dim < dims; ++dim) {
         LLVMValueRef srcx = lp_build_emit_fetch( &bld->bld_base, inst, 1, dim );
         LLVMValueRef srcy = lp_build_emit_fetch( &bld->bld_base, inst, 2, dim );
         for (quad = 0; quad < num_quads; ++quad) {
            unsigned s1 = 4*quad;
            unsigned s2 = 4*quad + length;
            shuffles[4*quad + 0] = lp_build_const_int32(gallivm, s1);
            shuffles[4*quad + 1] = lp_build_const_int32(gallivm, s2);
            shuffles[4*quad + 2] = i32undef;
            shuffles[4*quad + 3] = i32undef;
         }
         ddxdyonec[dim] = LLVMBuildShuffleVector(builder, srcx, srcy,
                                               LLVMConstVector(shuffles, length), "");
      }
      if (dims == 1) {
         derivs.ddx_ddy[0] = ddxdyonec[0];
      }
      else if (dims >= 2) {
         for (quad = 0; quad < num_quads; ++quad) {
            unsigned s1 = 4*quad;
            unsigned s2 = 4*quad + length;
            shuffles[4*quad + 0] = lp_build_const_int32(gallivm, s1);
            shuffles[4*quad + 1] = lp_build_const_int32(gallivm, s1 + 1);
            shuffles[4*quad + 2] = lp_build_const_int32(gallivm, s2);
            shuffles[4*quad + 3] = lp_build_const_int32(gallivm, s2 + 1);
         }
         derivs.ddx_ddy[0] = LLVMBuildShuffleVector(builder, ddxdyonec[0], ddxdyonec[1],
                                                  LLVMConstVector(shuffles, length), "");
         if (dims == 3) {
            derivs.ddx_ddy[1] = ddxdyonec[2];
         }
      }
      unit = inst->Src[3].Register.Index;
   }  else {
      if (dims == 1) {
         derivs.ddx_ddy[0] = lp_build_packed_ddx_ddy_onecoord(&bld->bld_base.base, coords[0]);
      }
      else if (dims >= 2) {
         derivs.ddx_ddy[0] = lp_build_packed_ddx_ddy_twocoord(&bld->bld_base.base,
                                                            coords[0], coords[1]);
         if (dims == 3) {
            derivs.ddx_ddy[1] = lp_build_packed_ddx_ddy_onecoord(&bld->bld_base.base, coords[2]);
         }
      }
      unit = inst->Src[1].Register.Index;
   }

   bld->sampler->emit_fetch_texel(bld->sampler,
                                  bld->bld_base.base.gallivm,
                                  bld->bld_base.base.type,
                                  unit, num_coords, coords,
                                  &derivs,
                                  lod_bias, explicit_lod,
                                  texel);
}

static void
emit_txq( struct lp_build_tgsi_soa_context *bld,
          const struct tgsi_full_instruction *inst,
          LLVMValueRef *sizes_out)
{
   LLVMValueRef explicit_lod;
   unsigned num_coords, has_lod;
   unsigned i;

   switch (inst->Texture.Texture) {
   case TGSI_TEXTURE_1D:
   case TGSI_TEXTURE_SHADOW1D:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWCUBE:
      num_coords = 1;
      has_lod = 1;
      break;
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_CUBE:
   case TGSI_TEXTURE_1D_ARRAY:
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
      num_coords = 2;
      has_lod = 1;
      break;
   case TGSI_TEXTURE_3D:
// case TGSI_TEXTURE_CUBE_ARRAY:
// case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
   case TGSI_TEXTURE_2D_ARRAY:
   case TGSI_TEXTURE_SHADOW2D_ARRAY:
      num_coords = 3;
      has_lod = 1;
      break;

   case TGSI_TEXTURE_BUFFER:
      num_coords = 1;
      has_lod = 0;
      break;

   case TGSI_TEXTURE_RECT:
   case TGSI_TEXTURE_SHADOWRECT:
// case TGSI_TEXTURE_2D_MS:
      num_coords = 2;
      has_lod = 0;
      break;

// case TGSI_TEXTURE_2D_MS_ARRAY:
//    num_coords = 3;
//    has_lod = 0;
//    break;

   default:
      assert(0);
      return;
   }

   if (!bld->sampler) {
      _debug_printf("warning: found texture query instruction but no sampler generator supplied\n");
      for (i = 0; i < num_coords; i++)
         sizes_out[i] = bld->bld_base.base.undef;
      return;
   }

   if (has_lod)
      explicit_lod = lp_build_emit_fetch( &bld->bld_base, inst, 0, 2 );
   else
      explicit_lod = NULL;

   bld->sampler->emit_size_query(bld->sampler,
                                 bld->bld_base.base.gallivm,
                                 bld->bld_base.int_bld.type,
                                 inst->Src[1].Register.Index,
                                 explicit_lod,
                                 sizes_out);
}

static boolean
near_end_of_shader(struct lp_build_tgsi_soa_context *bld,
		   int pc)
{
   int i;

   for (i = 0; i < 5; i++) {
      unsigned opcode;

      if (pc + i >= bld->bld_base.info->num_instructions)
	 return TRUE;

      opcode = bld->bld_base.instructions[pc + i].Instruction.Opcode;

      if (opcode == TGSI_OPCODE_END)
	 return TRUE;

      if (opcode == TGSI_OPCODE_TEX ||
	  opcode == TGSI_OPCODE_TXP ||
	  opcode == TGSI_OPCODE_TXD ||
	  opcode == TGSI_OPCODE_TXB ||
	  opcode == TGSI_OPCODE_TXL ||
	  opcode == TGSI_OPCODE_TXF ||
	  opcode == TGSI_OPCODE_TXQ ||
	  opcode == TGSI_OPCODE_CAL ||
	  opcode == TGSI_OPCODE_CALLNZ ||
	  opcode == TGSI_OPCODE_IF ||
	  opcode == TGSI_OPCODE_IFC ||
	  opcode == TGSI_OPCODE_BGNLOOP ||
	  opcode == TGSI_OPCODE_SWITCH)
	 return FALSE;
   }

   return TRUE;
}



/**
 * Kill fragment if any of the src register values are negative.
 */
static void
emit_kil(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_instruction *inst,
   int pc)
{
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   const struct tgsi_full_src_register *reg = &inst->Src[0];
   LLVMValueRef terms[TGSI_NUM_CHANNELS];
   LLVMValueRef mask;
   unsigned chan_index;

   memset(&terms, 0, sizeof terms);

   TGSI_FOR_EACH_CHANNEL( chan_index ) {
      unsigned swizzle;

      /* Unswizzle channel */
      swizzle = tgsi_util_get_full_src_register_swizzle( reg, chan_index );

      /* Check if the component has not been already tested. */
      assert(swizzle < TGSI_NUM_CHANNELS);
      if( !terms[swizzle] )
         /* TODO: change the comparison operator instead of setting the sign */
         terms[swizzle] =  lp_build_emit_fetch(&bld->bld_base, inst, 0, chan_index );
   }

   mask = NULL;
   TGSI_FOR_EACH_CHANNEL( chan_index ) {
      if(terms[chan_index]) {
         LLVMValueRef chan_mask;

         /*
          * If term < 0 then mask = 0 else mask = ~0.
          */
         chan_mask = lp_build_cmp(&bld->bld_base.base, PIPE_FUNC_GEQUAL, terms[chan_index], bld->bld_base.base.zero);

         if(mask)
            mask = LLVMBuildAnd(builder, mask, chan_mask, "");
         else
            mask = chan_mask;
      }
   }

   if(mask) {
      lp_build_mask_update(bld->mask, mask);

      if (!near_end_of_shader(bld, pc))
	 lp_build_mask_check(bld->mask);
   }
}


/**
 * Predicated fragment kill.
 * XXX Actually, we do an unconditional kill (as in tgsi_exec.c).
 * The only predication is the execution mask which will apply if
 * we're inside a loop or conditional.
 */
static void
emit_kilp(struct lp_build_tgsi_soa_context *bld,
          int pc)
{
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   LLVMValueRef mask;

   /* For those channels which are "alive", disable fragment shader
    * execution.
    */
   if (bld->exec_mask.has_mask) {
      mask = LLVMBuildNot(builder, bld->exec_mask.exec_mask, "kilp");
   }
   else {
      LLVMValueRef zero = LLVMConstNull(bld->bld_base.base.int_vec_type);
      mask = zero;
   }

   lp_build_mask_update(bld->mask, mask);

   if (!near_end_of_shader(bld, pc))
      lp_build_mask_check(bld->mask);
}


/**
 * Emit code which will dump the value of all the temporary registers
 * to stdout.
 */
static void
emit_dump_temps(struct lp_build_tgsi_soa_context *bld)
{
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef temp_ptr;
   LLVMValueRef i0 = lp_build_const_int32(gallivm, 0);
   LLVMValueRef i1 = lp_build_const_int32(gallivm, 1);
   LLVMValueRef i2 = lp_build_const_int32(gallivm, 2);
   LLVMValueRef i3 = lp_build_const_int32(gallivm, 3);
   int index;
   int n = bld->bld_base.info->file_max[TGSI_FILE_TEMPORARY];

   for (index = 0; index < n; index++) {
      LLVMValueRef idx = lp_build_const_int32(gallivm, index);
      LLVMValueRef v[4][4], res;
      int chan;

      lp_build_printf(gallivm, "TEMP[%d]:\n", idx);

      for (chan = 0; chan < 4; chan++) {
         temp_ptr = lp_get_temp_ptr_soa(bld, index, chan);
         res = LLVMBuildLoad(builder, temp_ptr, "");
         v[chan][0] = LLVMBuildExtractElement(builder, res, i0, "");
         v[chan][1] = LLVMBuildExtractElement(builder, res, i1, "");
         v[chan][2] = LLVMBuildExtractElement(builder, res, i2, "");
         v[chan][3] = LLVMBuildExtractElement(builder, res, i3, "");
      }

      lp_build_printf(gallivm, "  X: %f %f %f %f\n",
                      v[0][0], v[0][1], v[0][2], v[0][3]);
      lp_build_printf(gallivm, "  Y: %f %f %f %f\n",
                      v[1][0], v[1][1], v[1][2], v[1][3]);
      lp_build_printf(gallivm, "  Z: %f %f %f %f\n",
                      v[2][0], v[2][1], v[2][2], v[2][3]);
      lp_build_printf(gallivm, "  W: %f %f %f %f\n",
                      v[3][0], v[3][1], v[3][2], v[3][3]);
   }
}



void
lp_emit_declaration_soa(
   struct lp_build_tgsi_context *bld_base,
   const struct tgsi_full_declaration *decl)
{
   struct lp_build_tgsi_soa_context *bld = lp_soa_context(bld_base);
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   LLVMTypeRef vec_type = bld->bld_base.base.vec_type;
   const unsigned first = decl->Range.First;
   const unsigned last = decl->Range.Last;
   unsigned idx, i;

   for (idx = first; idx <= last; ++idx) {
      assert(last <= bld->bld_base.info->file_max[decl->Declaration.File]);
      switch (decl->Declaration.File) {
      case TGSI_FILE_TEMPORARY:
         assert(idx < LP_MAX_TGSI_TEMPS);
         if (!(bld->indirect_files & (1 << TGSI_FILE_TEMPORARY))) {
            for (i = 0; i < TGSI_NUM_CHANNELS; i++)
               bld->temps[idx][i] = lp_build_alloca(gallivm, vec_type, "temp");
         }
         break;

      case TGSI_FILE_OUTPUT:
         if (!(bld->indirect_files & (1 << TGSI_FILE_OUTPUT))) {
            for (i = 0; i < TGSI_NUM_CHANNELS; i++)
               bld->outputs[idx][i] = lp_build_alloca(gallivm,
                                                      vec_type, "output");
         }
         break;

      case TGSI_FILE_ADDRESS:
	 /* ADDR registers are the only allocated with an integer LLVM IR type,
	  * as they are guaranteed to always have integers.
	  * XXX: Not sure if this exception is worthwhile (or the whole idea of
	  * an ADDR register for that matter).
	  */
         assert(idx < LP_MAX_TGSI_ADDRS);
         for (i = 0; i < TGSI_NUM_CHANNELS; i++)
            bld->addr[idx][i] = lp_build_alloca(gallivm, bld_base->base.int_vec_type, "addr");
         break;

      case TGSI_FILE_PREDICATE:
         assert(idx < LP_MAX_TGSI_PREDS);
         for (i = 0; i < TGSI_NUM_CHANNELS; i++)
            bld->preds[idx][i] = lp_build_alloca(gallivm, vec_type,
                                                 "predicate");
         break;

      default:
         /* don't need to declare other vars */
         break;
      }
   }
}


void lp_emit_immediate_soa(
   struct lp_build_tgsi_context *bld_base,
   const struct tgsi_full_immediate *imm)
{
   struct lp_build_tgsi_soa_context *bld = lp_soa_context(bld_base);
   struct gallivm_state * gallivm = bld_base->base.gallivm;

   /* simply copy the immediate values into the next immediates[] slot */
   unsigned i;
   const uint size = imm->Immediate.NrTokens - 1;
   assert(size <= 4);
   assert(bld->num_immediates < LP_MAX_TGSI_IMMEDIATES);
   switch (imm->Immediate.DataType) {
   case TGSI_IMM_FLOAT32:
      for( i = 0; i < size; ++i )
         bld->immediates[bld->num_immediates][i] =
            lp_build_const_vec(gallivm, bld_base->base.type, imm->u[i].Float);

      break;
   case TGSI_IMM_UINT32:
      for( i = 0; i < size; ++i ) {
         LLVMValueRef tmp = lp_build_const_vec(gallivm, bld_base->uint_bld.type, imm->u[i].Uint);
         bld->immediates[bld->num_immediates][i] =
            LLVMConstBitCast(tmp, bld_base->base.vec_type);
      }

      break;
   case TGSI_IMM_INT32:
      for( i = 0; i < size; ++i ) {
         LLVMValueRef tmp = lp_build_const_vec(gallivm, bld_base->int_bld.type, imm->u[i].Int);
         bld->immediates[bld->num_immediates][i] =
            LLVMConstBitCast(tmp, bld_base->base.vec_type);
      }
            
      break;
   }
   for( i = size; i < 4; ++i )
      bld->immediates[bld->num_immediates][i] = bld_base->base.undef;

   bld->num_immediates++;
}

static void
ddx_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_fetch_deriv(bld, emit_data->args[0], NULL,
                    &emit_data->output[emit_data->chan], NULL);
}

static void
ddy_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_fetch_deriv(bld, emit_data->args[0], NULL, NULL,
                    &emit_data->output[emit_data->chan]);
}

static void
kilp_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_kilp(bld, bld_base->pc - 1);
}

static void
kil_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_kil(bld, emit_data->inst, bld_base->pc - 1);
}

static void
tex_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_tex(bld, emit_data->inst, LP_BLD_TEX_MODIFIER_NONE, emit_data->output);
}

static void
txb_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_tex(bld, emit_data->inst, LP_BLD_TEX_MODIFIER_LOD_BIAS,
            emit_data->output);
}

static void
txd_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_tex(bld, emit_data->inst, LP_BLD_TEX_MODIFIER_EXPLICIT_DERIV,
            emit_data->output);
}

static void
txl_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_tex(bld, emit_data->inst, LP_BLD_TEX_MODIFIER_EXPLICIT_LOD,
            emit_data->output);
}

static void
txp_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_tex(bld, emit_data->inst, LP_BLD_TEX_MODIFIER_PROJECTED,
            emit_data->output);
}

static void
txq_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_txq(bld, emit_data->inst, emit_data->output);
}

static void
cal_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_mask_call(&bld->exec_mask, emit_data->inst->Label.Label,
                     &bld_base->pc);
}

static void
ret_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_mask_ret(&bld->exec_mask, &bld_base->pc);
}

static void
brk_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_break(&bld->exec_mask);
}

static void
if_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp;
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   tmp = lp_build_cmp(&bld_base->base, PIPE_FUNC_NOTEQUAL,
                      emit_data->args[0], bld->bld_base.base.zero);
   lp_exec_mask_cond_push(&bld->exec_mask, tmp);
}

static void
bgnloop_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_bgnloop(&bld->exec_mask);
}

static void
bgnsub_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_mask_bgnsub(&bld->exec_mask);
}

static void
else_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_mask_cond_invert(&bld->exec_mask);
}

static void
endif_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_mask_cond_pop(&bld->exec_mask);
}

static void
endloop_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_endloop(bld_base->base.gallivm, &bld->exec_mask);
}

static void
endsub_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_mask_endsub(&bld->exec_mask, &bld_base->pc);
}

static void
cont_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_continue(&bld->exec_mask);
}

/* XXX: Refactor and move it to lp_bld_tgsi_action.c
 *
 * XXX: What do the comments about xmm registers mean?  Maybe they are left over
 * from old code, but there is no garauntee that LLVM will use those registers
 * for this code.
 *
 * XXX: There should be no calls to lp_build_emit_fetch in this function.  This
 * should be handled by the emit_data->fetch_args function. */
static void
nrm_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp0, tmp1;
   LLVMValueRef tmp4 = NULL;
   LLVMValueRef tmp5 = NULL;
   LLVMValueRef tmp6 = NULL;
   LLVMValueRef tmp7 = NULL;
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   uint dims = (emit_data->inst->Instruction.Opcode == TGSI_OPCODE_NRM) ? 3 : 4;

  if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_X) ||
      TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_Y) ||
      TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_Z) ||
      (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_W) && dims == 4)) {

      /* NOTE: Cannot use xmm regs 2/3 here (see emit_rsqrt() above). */

      /* xmm4 = src.x */
      /* xmm0 = src.x * src.x */
      tmp0 = lp_build_emit_fetch(&bld->bld_base, emit_data->inst, 0, TGSI_CHAN_X);
      if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_X)) {
         tmp4 = tmp0;
      }
      tmp0 = lp_build_mul( &bld->bld_base.base, tmp0, tmp0);

      /* xmm5 = src.y */
      /* xmm0 = xmm0 + src.y * src.y */
      tmp1 = lp_build_emit_fetch(&bld->bld_base, emit_data->inst, 0, TGSI_CHAN_Y);
      if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_Y)) {
         tmp5 = tmp1;
      }
      tmp1 = lp_build_mul( &bld->bld_base.base, tmp1, tmp1);
      tmp0 = lp_build_add( &bld->bld_base.base, tmp0, tmp1);

      /* xmm6 = src.z */
      /* xmm0 = xmm0 + src.z * src.z */
      tmp1 = lp_build_emit_fetch(&bld->bld_base, emit_data->inst, 0, TGSI_CHAN_Z);
      if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_Z)) {
         tmp6 = tmp1;
      }
      tmp1 = lp_build_mul( &bld->bld_base.base, tmp1, tmp1);
      tmp0 = lp_build_add( &bld->bld_base.base, tmp0, tmp1);

      if (dims == 4) {
         /* xmm7 = src.w */
         /* xmm0 = xmm0 + src.w * src.w */
         tmp1 = lp_build_emit_fetch(&bld->bld_base, emit_data->inst, 0, TGSI_CHAN_W);
         if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_W)) {
            tmp7 = tmp1;
         }
         tmp1 = lp_build_mul( &bld->bld_base.base, tmp1, tmp1);
         tmp0 = lp_build_add( &bld->bld_base.base, tmp0, tmp1);
      }
      /* xmm1 = 1 / sqrt(xmm0) */
      tmp1 = lp_build_rsqrt( &bld->bld_base.base, tmp0);
       /* dst.x = xmm1 * src.x */
      if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_X)) {
         emit_data->output[TGSI_CHAN_X] = lp_build_mul( &bld->bld_base.base, tmp4, tmp1);
      }
      /* dst.y = xmm1 * src.y */
      if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_Y)) {
         emit_data->output[TGSI_CHAN_Y] = lp_build_mul( &bld->bld_base.base, tmp5, tmp1);
      }

      /* dst.z = xmm1 * src.z */
      if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_Z)) {
         emit_data->output[TGSI_CHAN_Z] = lp_build_mul( &bld->bld_base.base, tmp6, tmp1);
      }
      /* dst.w = xmm1 * src.w */
      if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_X) && dims == 4) {
         emit_data->output[TGSI_CHAN_W] = lp_build_mul( &bld->bld_base.base, tmp7, tmp1);
      }
   }

   /* dst.w = 1.0 */
   if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_W) && dims == 3) {
       emit_data->output[TGSI_CHAN_W] = bld->bld_base.base.one;
   }
}

static void emit_prologue(struct lp_build_tgsi_context * bld_base)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   struct gallivm_state * gallivm = bld_base->base.gallivm;

   if (bld->indirect_files & (1 << TGSI_FILE_TEMPORARY)) {
      LLVMValueRef array_size =
         lp_build_const_int32(gallivm,
                         bld_base->info->file_max[TGSI_FILE_TEMPORARY] * 4 + 4);
      bld->temps_array = lp_build_array_alloca(gallivm,
                                              bld_base->base.vec_type, array_size,
                                              "temp_array");
   }

   if (bld->indirect_files & (1 << TGSI_FILE_OUTPUT)) {
      LLVMValueRef array_size =
         lp_build_const_int32(gallivm,
                            bld_base->info->file_max[TGSI_FILE_OUTPUT] * 4 + 4);
      bld->outputs_array = lp_build_array_alloca(gallivm,
                                                bld_base->base.vec_type, array_size,
                                                "output_array");
   }

   /* If we have indirect addressing in inputs we need to copy them into
    * our alloca array to be able to iterate over them */
   if (bld->indirect_files & (1 << TGSI_FILE_INPUT)) {
      unsigned index, chan;
      LLVMTypeRef vec_type = bld_base->base.vec_type;
      LLVMValueRef array_size = lp_build_const_int32(gallivm,
            bld_base->info->file_max[TGSI_FILE_INPUT]*4 + 4);
      bld->inputs_array = lp_build_array_alloca(gallivm,
                                               vec_type, array_size,
                                               "input_array");

      assert(bld_base->info->num_inputs
                        <= bld_base->info->file_max[TGSI_FILE_INPUT] + 1);

      for (index = 0; index < bld_base->info->num_inputs; ++index) {
         for (chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
            LLVMValueRef lindex =
               lp_build_const_int32(gallivm, index * 4 + chan);
            LLVMValueRef input_ptr =
               LLVMBuildGEP(gallivm->builder, bld->inputs_array,
                            &lindex, 1, "");
            LLVMValueRef value = bld->inputs[index][chan];
            if (value)
               LLVMBuildStore(gallivm->builder, value, input_ptr);
         }
      }
   }
}

static void emit_epilogue(struct lp_build_tgsi_context * bld_base)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   if (0) {
      /* for debugging */
      emit_dump_temps(bld);
   }

   /* If we have indirect addressing in outputs we need to copy our alloca array
    * to the outputs slots specified by the called */
   if (bld->indirect_files & (1 << TGSI_FILE_OUTPUT)) {
      unsigned index, chan;
      assert(bld_base->info->num_outputs <=
                        bld_base->info->file_max[TGSI_FILE_OUTPUT] + 1);
      for (index = 0; index < bld_base->info->num_outputs; ++index) {
         for (chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
            bld->outputs[index][chan] = lp_get_output_ptr(bld, index, chan);
         }
      }
   }
}

void
lp_build_tgsi_soa(struct gallivm_state *gallivm,
                  const struct tgsi_token *tokens,
                  struct lp_type type,
                  struct lp_build_mask_context *mask,
                  LLVMValueRef consts_ptr,
                  const struct lp_bld_tgsi_system_values *system_values,
                  const LLVMValueRef *pos,
                  const LLVMValueRef (*inputs)[TGSI_NUM_CHANNELS],
                  LLVMValueRef (*outputs)[TGSI_NUM_CHANNELS],
                  struct lp_build_sampler_soa *sampler,
                  const struct tgsi_shader_info *info)
{
   struct lp_build_tgsi_soa_context bld;

   struct lp_type res_type;

   assert(type.length <= LP_MAX_VECTOR_LENGTH);
   memset(&res_type, 0, sizeof res_type);
   res_type.width = type.width;
   res_type.length = type.length;
   res_type.sign = 1;

   /* Setup build context */
   memset(&bld, 0, sizeof bld);
   lp_build_context_init(&bld.bld_base.base, gallivm, type);
   lp_build_context_init(&bld.bld_base.uint_bld, gallivm, lp_uint_type(type));
   lp_build_context_init(&bld.bld_base.int_bld, gallivm, lp_int_type(type));
   lp_build_context_init(&bld.elem_bld, gallivm, lp_elem_type(type));
   bld.mask = mask;
   bld.pos = pos;
   bld.inputs = inputs;
   bld.outputs = outputs;
   bld.consts_ptr = consts_ptr;
   bld.sampler = sampler;
   bld.bld_base.info = info;
   bld.indirect_files = info->indirect_files;

   bld.bld_base.soa = TRUE;
   bld.bld_base.emit_fetch_funcs[TGSI_FILE_CONSTANT] = emit_fetch_constant;
   bld.bld_base.emit_fetch_funcs[TGSI_FILE_IMMEDIATE] = emit_fetch_immediate;
   bld.bld_base.emit_fetch_funcs[TGSI_FILE_INPUT] = emit_fetch_input;
   bld.bld_base.emit_fetch_funcs[TGSI_FILE_TEMPORARY] = emit_fetch_temporary;
   bld.bld_base.emit_fetch_funcs[TGSI_FILE_SYSTEM_VALUE] = emit_fetch_system_value;
   bld.bld_base.emit_store = emit_store;

   bld.bld_base.emit_declaration = lp_emit_declaration_soa;
   bld.bld_base.emit_immediate = lp_emit_immediate_soa;

   bld.bld_base.emit_prologue = emit_prologue;
   bld.bld_base.emit_epilogue = emit_epilogue;

   /* Set opcode actions */
   lp_set_default_actions_cpu(&bld.bld_base);

   bld.bld_base.op_actions[TGSI_OPCODE_BGNLOOP].emit = bgnloop_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_BGNSUB].emit = bgnsub_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_BRK].emit = brk_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_CAL].emit = cal_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_CONT].emit = cont_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_DDX].emit = ddx_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_DDY].emit = ddy_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_ELSE].emit = else_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_ENDIF].emit = endif_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_ENDLOOP].emit = endloop_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_ENDSUB].emit = endsub_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_IF].emit = if_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_KIL].emit = kil_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_KILP].emit = kilp_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_NRM].emit = nrm_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_NRM4].emit = nrm_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_RET].emit = ret_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_TEX].emit = tex_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_TXB].emit = txb_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_TXD].emit = txd_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_TXL].emit = txl_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_TXP].emit = txp_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_TXQ].emit = txq_emit;

   lp_exec_mask_init(&bld.exec_mask, &bld.bld_base.base);

   bld.system_values = *system_values;

   lp_build_tgsi_llvm(&bld.bld_base, tokens);

   if (0) {
      LLVMBasicBlockRef block = LLVMGetInsertBlock(gallivm->builder);
      LLVMValueRef function = LLVMGetBasicBlockParent(block);
      debug_printf("11111111111111111111111111111 \n");
      tgsi_dump(tokens, 0);
      lp_debug_dump_value(function);
      debug_printf("2222222222222222222222222222 \n");
   }

   if (0) {
      LLVMModuleRef module = LLVMGetGlobalParent(
         LLVMGetBasicBlockParent(LLVMGetInsertBlock(gallivm->builder)));
      LLVMDumpModule(module);

   }
}
