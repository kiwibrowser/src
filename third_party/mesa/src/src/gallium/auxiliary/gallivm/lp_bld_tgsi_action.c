/**************************************************************************
 * 
 * Copyright 2011-2012 Advanced Micro Devices, Inc.
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
 * TGSI to LLVM IR translation.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 * @author Tom Stellard <thomas.stellard@amd.com>
 *
 * Based on tgsi_sse2.c code written by Michal Krol, Keith Whitwell,
 * Brian Paul, and others.
 */


#include "lp_bld_tgsi_action.h"

#include "lp_bld_tgsi.h"
#include "lp_bld_arit.h"
#include "lp_bld_bitarit.h"
#include "lp_bld_const.h"
#include "lp_bld_gather.h"
#include "lp_bld_logic.h"

#include "tgsi/tgsi_exec.h"

/* XXX: The CPU only defaults should be repaced by generic ones.  In most
 * cases, the CPU defaults are just wrappers around a function in
 * lp_build_arit.c and these functions should be inlined here and the CPU
 * generic code should be removed and placed elsewhere.
 */

/* Default actions */

/* Generic fetch_arg functions */

static void scalar_unary_fetch_args(
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   /* src0.x */
   emit_data->args[0] = lp_build_emit_fetch(bld_base, emit_data->inst, 0, 0);
   emit_data->arg_count = 1;
   emit_data->dst_type = LLVMTypeOf(emit_data->args[0]);
}

static void scalar_binary_fetch_args(
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   /* src0.x */
   emit_data->args[0] = lp_build_emit_fetch(bld_base, emit_data->inst,
                                            0, TGSI_CHAN_X);
   /* src1.x */
   emit_data->args[1] = lp_build_emit_fetch(bld_base, emit_data->inst,
                                            1, TGSI_CHAN_X);
   emit_data->arg_count = 2;
   emit_data->dst_type = LLVMTypeOf(emit_data->args[0]);
}

/* TGSI_OPCODE_ADD */
static void
add_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = LLVMBuildFAdd(
                                bld_base->base.gallivm->builder,
                                emit_data->args[0], emit_data->args[1], "");
}

/* TGSI_OPCODE_ARR */
static void
arr_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp = lp_build_emit_llvm_unary(bld_base, TGSI_OPCODE_ROUND, emit_data->args[0]);
   emit_data->output[emit_data->chan] = LLVMBuildFPToSI(bld_base->base.gallivm->builder, tmp,
							bld_base->uint_bld.vec_type, "");
}

/* TGSI_OPCODE_CLAMP */
static void
clamp_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp;
   tmp = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_MAX,
                                   emit_data->args[0],
                                   emit_data->args[1]);
   emit_data->output[emit_data->chan] = lp_build_emit_llvm_binary(bld_base,
                                       TGSI_OPCODE_MIN, tmp, emit_data->args[2]);
}

/* DP* Helper */

static void
dp_fetch_args(
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data,
   unsigned dp_components)
{
   unsigned chan, src;
   for (src = 0; src < 2; src++) {
      for (chan = 0; chan < dp_components; chan++) {
         emit_data->args[(src * dp_components) + chan] =
                     lp_build_emit_fetch(bld_base, emit_data->inst, src, chan);
      }
   }
   emit_data->dst_type = bld_base->base.elem_type;
}

/* TGSI_OPCODE_DP2 */
static void
dp2_fetch_args(
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   dp_fetch_args(bld_base, emit_data, 2);
}

static void
dp2_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp0, tmp1;
   tmp0 = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_MUL,
                                    emit_data->args[0] /* src0.x */,
                                    emit_data->args[2] /* src1.x */);
   tmp1 = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_MUL,
                                    emit_data->args[1] /* src0.y */,
                                    emit_data->args[3] /* src1.y */);
   emit_data->output[emit_data->chan] = lp_build_emit_llvm_binary(bld_base,
                                                    TGSI_OPCODE_ADD, tmp0, tmp1);
}

static struct lp_build_tgsi_action dp2_action = {
   dp2_fetch_args,	 /* fetch_args */
   dp2_emit	 /* emit */
};

/* TGSI_OPCODE_DP2A */
static void
dp2a_fetch_args(
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   dp_fetch_args(bld_base, emit_data, 2);
   emit_data->args[5] = lp_build_emit_fetch(bld_base, emit_data->inst,
                                            2, TGSI_CHAN_X);
}

static void
dp2a_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp;
   tmp = lp_build_emit_llvm(bld_base, TGSI_OPCODE_DP2, emit_data);
   emit_data->output[emit_data->chan] = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_ADD,
                                    emit_data->args[5], tmp);
}

static struct lp_build_tgsi_action dp2a_action = {
   dp2a_fetch_args,	 /* fetch_args */
   dp2a_emit	 /* emit */
};

/* TGSI_OPCODE_DP3 */
static void
dp3_fetch_args(
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   dp_fetch_args(bld_base, emit_data, 3);
}

static void
dp3_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp0, tmp1;
   tmp0 = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_MUL,
                                    emit_data->args[0] /* src0.x */,
                                    emit_data->args[3] /* src1.x */);
   tmp1 = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_MUL,
                                    emit_data->args[1] /* src0.y */,
                                    emit_data->args[4] /* src1.y */);
   tmp0 = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_ADD, tmp1, tmp0);
   tmp1 = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_MUL,
                                    emit_data->args[2] /* src0.z */,
                                    emit_data->args[5] /* src1.z */);
   emit_data->output[emit_data->chan] = lp_build_emit_llvm_binary(bld_base,
                                                    TGSI_OPCODE_ADD, tmp0, tmp1);
}

static struct lp_build_tgsi_action dp3_action = {
   dp3_fetch_args,	 /* fetch_args */
   dp3_emit	 /* emit */
};

/* TGSI_OPCODDE_DP4 */

static void
dp4_fetch_args(
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   dp_fetch_args(bld_base, emit_data, 4);
}

static void
dp4_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp0, tmp1;
   tmp0 = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_MUL,
                                    emit_data->args[0] /* src0.x */,
                                    emit_data->args[4] /* src1.x */);
   tmp1 = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_MUL,
                                    emit_data->args[1] /* src0.y */,
                                    emit_data->args[5] /* src1.y */);
   tmp0 = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_ADD, tmp0, tmp1);
   tmp1 = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_MUL,
                                    emit_data->args[2] /* src0.z */,
                                    emit_data->args[6] /* src1.z */);
   tmp0 = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_ADD, tmp0, tmp1);
   tmp1 = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_MUL,
                                    emit_data->args[3] /* src0.w */,
                                    emit_data->args[7] /* src1.w */);
   emit_data->output[emit_data->chan] = lp_build_emit_llvm_binary(bld_base,
                                                    TGSI_OPCODE_ADD, tmp0, tmp1);
}

static struct lp_build_tgsi_action dp4_action = {
   dp4_fetch_args,	 /* fetch_args */
   dp4_emit	 /* emit */
};

/* TGSI_OPCODE_DPH */
static void
dph_fetch_args(
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   dp_fetch_args(bld_base, emit_data, 4);
   /* src0.w */
   emit_data->args[3] = bld_base->base.one;
}

const struct lp_build_tgsi_action dph_action = {
   dph_fetch_args,	 /* fetch_args */
   dp4_emit	 /* emit */
};

/* TGSI_OPCODE_DST */
static void
dst_fetch_args(
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   /* src0.y */
   emit_data->args[0] = lp_build_emit_fetch(bld_base, emit_data->inst,
                                            0, TGSI_CHAN_Y);
   /* src0.z */
   emit_data->args[1] = lp_build_emit_fetch(bld_base, emit_data->inst,
                                            0, TGSI_CHAN_Z);
   /* src1.y */
   emit_data->args[2] = lp_build_emit_fetch(bld_base, emit_data->inst,
                                            1, TGSI_CHAN_Y);
   /* src1.w */
   emit_data->args[3] = lp_build_emit_fetch(bld_base, emit_data->inst,
                                            1, TGSI_CHAN_W);
}

static void
dst_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   /* dst.x */
   emit_data->output[TGSI_CHAN_X] = bld_base->base.one;

   /* dst.y */
   emit_data->output[TGSI_CHAN_Y] = lp_build_emit_llvm_binary(bld_base,
                                          TGSI_OPCODE_MUL,
                                          emit_data->args[0] /* src0.y */,
                                          emit_data->args[2] /* src1.y */);
   /* dst.z */
   emit_data->output[TGSI_CHAN_Z] = emit_data->args[1]; /* src0.z */

   /* dst.w */
   emit_data->output[TGSI_CHAN_W] = emit_data->args[3]; /* src1.w */
}

static struct lp_build_tgsi_action dst_action = {
   dst_fetch_args,	 /* fetch_args */
   dst_emit	 /* emit */
};

/* TGSI_OPCODE_END */
static void
end_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   bld_base->pc = -1;
}

/* TGSI_OPCODE_EXP */

static void
exp_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef floor_x;

   /* floor( src0.x ) */
   floor_x = lp_build_emit_llvm_unary(bld_base, TGSI_OPCODE_FLR,
                                      emit_data->args[0]);

   /* 2 ^ floor( src0.x ) */
   emit_data->output[TGSI_CHAN_X] = lp_build_emit_llvm_unary(bld_base,
                                       TGSI_OPCODE_EX2, floor_x);

   /* src0.x - floor( src0.x ) */
   emit_data->output[TGSI_CHAN_Y] = lp_build_emit_llvm_binary(bld_base,
                   TGSI_OPCODE_SUB,  emit_data->args[0] /* src0.x */, floor_x);

   /* 2 ^ src0.x */
   emit_data->output[TGSI_CHAN_Z] = lp_build_emit_llvm_unary(bld_base,
                             TGSI_OPCODE_EX2, emit_data->args[0] /* src0.x */);

   emit_data->output[TGSI_CHAN_W] = bld_base->base.one;
}

const struct lp_build_tgsi_action exp_action = {
   scalar_unary_fetch_args,	 /* fetch_args */
   exp_emit	 /* emit */
};

/* TGSI_OPCODE_FRC */

static void
frc_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp;
   tmp = lp_build_emit_llvm_unary(bld_base, TGSI_OPCODE_FLR,
                                  emit_data->args[0]);
   emit_data->output[emit_data->chan] = lp_build_emit_llvm_binary(bld_base,
                                       TGSI_OPCODE_SUB, emit_data->args[0], tmp);
}

/* TGSI_OPCODE_KIL */

static void
kil_fetch_args(
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   /* src0.x */
   emit_data->args[0] = lp_build_emit_fetch(bld_base, emit_data->inst,
                                            0, TGSI_CHAN_X);
   /* src0.y */
   emit_data->args[1] = lp_build_emit_fetch(bld_base, emit_data->inst,
                                            0, TGSI_CHAN_Y);
   /* src0.z */
   emit_data->args[2] = lp_build_emit_fetch(bld_base, emit_data->inst,
                                            0, TGSI_CHAN_Z);
   /* src0.w */
   emit_data->args[3] = lp_build_emit_fetch(bld_base, emit_data->inst,
                                            0, TGSI_CHAN_W);
   emit_data->arg_count = 4;
   emit_data->dst_type = LLVMVoidTypeInContext(bld_base->base.gallivm->context);
}

/* TGSI_OPCODE_KILP */

static void
kilp_fetch_args(
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->dst_type = LLVMVoidTypeInContext(bld_base->base.gallivm->context);
}

/* TGSI_OPCODE_LIT */

static void
lit_fetch_args(
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   /* src0.x */
   emit_data->args[0] = lp_build_emit_fetch(bld_base, emit_data->inst, 0, TGSI_CHAN_X);
   /* src0.y */
   emit_data->args[1] = lp_build_emit_fetch(bld_base, emit_data->inst, 0, TGSI_CHAN_Y);
   /* src0.w */
   emit_data->args[2] = lp_build_emit_fetch(bld_base, emit_data->inst, 0, TGSI_CHAN_W);
   emit_data->arg_count = 3;
}

static void
lit_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp0, tmp1, tmp2;

   /* dst.x */
   emit_data->output[TGSI_CHAN_X] = bld_base->base.one;

   /* dst. y */
   emit_data->output[TGSI_CHAN_Y] = lp_build_emit_llvm_binary(bld_base,
                                               TGSI_OPCODE_MAX,
                                               emit_data->args[0] /* src0.x */,
                                               bld_base->base.zero);

   /* dst.z */
   /* XMM[1] = SrcReg[0].yyyy */
   tmp1 = emit_data->args[1];
   /* XMM[1] = max(XMM[1], 0) */
   tmp1 = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_MAX,
                                    tmp1, bld_base->base.zero);
   /* XMM[2] = SrcReg[0].wwww */
   tmp2 = emit_data->args[2];
   tmp1 = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_POW,
                                    tmp1, tmp2);
   tmp0 = emit_data->args[0];
   emit_data->output[TGSI_CHAN_Z] = lp_build_emit_llvm_ternary(bld_base,
                                             TGSI_OPCODE_CMP,
                                             tmp0, bld_base->base.zero, tmp1);
   /* dst.w */
   emit_data->output[TGSI_CHAN_W] = bld_base->base.one;
}

static struct lp_build_tgsi_action lit_action = {
   lit_fetch_args,	 /* fetch_args */
   lit_emit	 /* emit */
};

/* TGSI_OPCODE_LOG */

static void
log_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{

   LLVMValueRef abs_x, log_abs_x, flr_log_abs_x, ex2_flr_log_abs_x;

   /* abs( src0.x) */
   abs_x = lp_build_emit_llvm_unary(bld_base, TGSI_OPCODE_ABS,
                                    emit_data->args[0] /* src0.x */);

   /* log( abs( src0.x ) ) */
   log_abs_x = lp_build_emit_llvm_unary(bld_base, TGSI_OPCODE_LG2,
                                        abs_x);

   /* floor( log( abs( src0.x ) ) ) */
   flr_log_abs_x = lp_build_emit_llvm_unary(bld_base, TGSI_OPCODE_FLR,
                                            log_abs_x);
   /* dst.x */
   emit_data->output[TGSI_CHAN_X] = flr_log_abs_x;

   /* dst.y */
   ex2_flr_log_abs_x = lp_build_emit_llvm_unary(bld_base, TGSI_OPCODE_EX2,
                                                flr_log_abs_x);

   /* abs( src0.x ) / 2^( floor( lg2( abs( src0.x ) ) ) ) */
   emit_data->output[TGSI_CHAN_Y] = lp_build_emit_llvm_binary(bld_base,
                                    TGSI_OPCODE_DIV, abs_x, ex2_flr_log_abs_x);

   /* dst.x */
   emit_data->output[TGSI_CHAN_Z] = log_abs_x;

   /* dst.w */
   emit_data->output[TGSI_CHAN_W] = bld_base->base.one;
}

static struct lp_build_tgsi_action log_action = {
   scalar_unary_fetch_args,	 /* fetch_args */
   log_emit	 /* emit */
};

/* TGSI_OPCODE_LRP */

static void
lrp_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp;
   tmp = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_SUB,
                                   emit_data->args[1],
                                   emit_data->args[2]);
   emit_data->output[emit_data->chan] = lp_build_emit_llvm_ternary(bld_base,
                    TGSI_OPCODE_MAD, emit_data->args[0], tmp, emit_data->args[2]);
}

/* TGSI_OPCODE_MAD */

static void
mad_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp;
   tmp = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_MUL,
                                   emit_data->args[0],
                                   emit_data->args[1]);
   emit_data->output[emit_data->chan] = lp_build_emit_llvm_binary(bld_base,
                                       TGSI_OPCODE_ADD, tmp, emit_data->args[2]);
}

/* TGSI_OPCODE_MOV */

static void
mov_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = emit_data->args[0];
}

/* TGSI_OPCODE_MUL */
static void
mul_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_mul(&bld_base->base,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_POW */

static void
pow_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_pow(&bld_base->base,
                                   emit_data->args[0], emit_data->args[1]);
}

static struct lp_build_tgsi_action pow_action = {
   scalar_binary_fetch_args,	 /* fetch_args */
   pow_emit	 /* emit */
};

/* TGSI_OPCODE_RSQ */

static void
rsq_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->args[0] = lp_build_emit_llvm_unary(bld_base, TGSI_OPCODE_ABS,
                                               emit_data->args[0]);
   if (bld_base->rsq_action.emit) {
      bld_base->rsq_action.emit(&bld_base->rsq_action, bld_base, emit_data);
   } else {
      emit_data->output[emit_data->chan] = bld_base->base.undef;
   }
}

const struct lp_build_tgsi_action rsq_action = {
   scalar_unary_fetch_args,	 /* fetch_args */
   rsq_emit	 /* emit */

};

/* TGSI_OPCODE_SCS */
static void
scs_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   /* dst.x */
   emit_data->output[TGSI_CHAN_X] = lp_build_emit_llvm_unary(bld_base,
                                           TGSI_OPCODE_COS, emit_data->args[0]);
   /* dst.y */
   emit_data->output[TGSI_CHAN_Y] = lp_build_emit_llvm_unary(bld_base,
                                           TGSI_OPCODE_SIN, emit_data->args[0]);
   /* dst.z */
   emit_data->output[TGSI_CHAN_Z] = bld_base->base.zero;

   /* dst.w */
   emit_data->output[TGSI_CHAN_W] = bld_base->base.one;
}

const struct lp_build_tgsi_action scs_action = {
   scalar_unary_fetch_args,	 /* fetch_args */
   scs_emit	 /* emit */
};

/* TGSI_OPCODE_SFL */

static void
sfl_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = bld_base->base.zero;
}

/* TGSI_OPCODE_STR */

static void
str_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = bld_base->base.one;
}

/* TGSI_OPCODE_SUB */
static void
sub_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
	emit_data->output[emit_data->chan] = LLVMBuildFSub(
				bld_base->base.gallivm->builder,
				emit_data->args[0],
				emit_data->args[1], "");
}

/* TGSI_OPCODE_U2F */
static void
u2f_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = LLVMBuildUIToFP(bld_base->base.gallivm->builder,
							emit_data->args[0],
							bld_base->base.vec_type, "");
}

static void
umad_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp;
   tmp = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_UMUL,
                                   emit_data->args[0],
                                   emit_data->args[1]);
   emit_data->output[emit_data->chan] = lp_build_emit_llvm_binary(bld_base,
                                       TGSI_OPCODE_UADD, tmp, emit_data->args[2]);
}

/* TGSI_OPCODE_UMUL */
static void
umul_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_mul(&bld_base->uint_bld,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_XPD */

static void
xpd_fetch_args(
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   dp_fetch_args(bld_base, emit_data, 3);
}

/**
 * (a * b) - (c * d)
 */
static LLVMValueRef
xpd_helper(
  struct lp_build_tgsi_context * bld_base,
  LLVMValueRef a,
  LLVMValueRef b,
  LLVMValueRef c,
  LLVMValueRef d)
{
   LLVMValueRef tmp0, tmp1;

   tmp0 = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_MUL, a, b);
   tmp1 = lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_MUL, c, d);

   return lp_build_emit_llvm_binary(bld_base, TGSI_OPCODE_SUB, tmp0, tmp1);
}

static void
xpd_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[TGSI_CHAN_X] = xpd_helper(bld_base,
              emit_data->args[1] /* src0.y */, emit_data->args[5] /* src1.z */,
              emit_data->args[4] /* src1.y */, emit_data->args[2] /* src0.z */);

   emit_data->output[TGSI_CHAN_Y] = xpd_helper(bld_base,
              emit_data->args[2] /* src0.z */, emit_data->args[3] /* src1.x */,
              emit_data->args[5] /* src1.z */, emit_data->args[0] /* src0.x */);

   emit_data->output[TGSI_CHAN_Z] = xpd_helper(bld_base,
              emit_data->args[0] /* src0.x */, emit_data->args[4] /* src1.y */,
              emit_data->args[3] /* src1.x */, emit_data->args[1] /* src0.y */);

   emit_data->output[TGSI_CHAN_W] = bld_base->base.one;
}

const struct lp_build_tgsi_action xpd_action = {
   xpd_fetch_args,	 /* fetch_args */
   xpd_emit	 /* emit */
};

void
lp_set_default_actions(struct lp_build_tgsi_context * bld_base)
{
   bld_base->op_actions[TGSI_OPCODE_DP2] = dp2_action;
   bld_base->op_actions[TGSI_OPCODE_DP3] = dp3_action;
   bld_base->op_actions[TGSI_OPCODE_DP4] = dp4_action;
   bld_base->op_actions[TGSI_OPCODE_DP2A] = dp2a_action;
   bld_base->op_actions[TGSI_OPCODE_DPH] = dph_action;
   bld_base->op_actions[TGSI_OPCODE_DST] = dst_action;
   bld_base->op_actions[TGSI_OPCODE_EXP] = exp_action;
   bld_base->op_actions[TGSI_OPCODE_LIT] = lit_action;
   bld_base->op_actions[TGSI_OPCODE_LOG] = log_action;
   bld_base->op_actions[TGSI_OPCODE_RSQ] = rsq_action;
   bld_base->op_actions[TGSI_OPCODE_POW] = pow_action;
   bld_base->op_actions[TGSI_OPCODE_SCS] = scs_action;
   bld_base->op_actions[TGSI_OPCODE_XPD] = xpd_action;

   bld_base->op_actions[TGSI_OPCODE_COS].fetch_args = scalar_unary_fetch_args;
   bld_base->op_actions[TGSI_OPCODE_EX2].fetch_args = scalar_unary_fetch_args;
   bld_base->op_actions[TGSI_OPCODE_IF].fetch_args = scalar_unary_fetch_args;
   bld_base->op_actions[TGSI_OPCODE_KIL].fetch_args = kil_fetch_args;
   bld_base->op_actions[TGSI_OPCODE_KILP].fetch_args = kilp_fetch_args;
   bld_base->op_actions[TGSI_OPCODE_RCP].fetch_args = scalar_unary_fetch_args;
   bld_base->op_actions[TGSI_OPCODE_SIN].fetch_args = scalar_unary_fetch_args;
   bld_base->op_actions[TGSI_OPCODE_LG2].fetch_args = scalar_unary_fetch_args;

   bld_base->op_actions[TGSI_OPCODE_ADD].emit = add_emit;
   bld_base->op_actions[TGSI_OPCODE_ARR].emit = arr_emit;
   bld_base->op_actions[TGSI_OPCODE_CLAMP].emit = clamp_emit;
   bld_base->op_actions[TGSI_OPCODE_END].emit = end_emit;
   bld_base->op_actions[TGSI_OPCODE_FRC].emit = frc_emit;
   bld_base->op_actions[TGSI_OPCODE_LRP].emit = lrp_emit;
   bld_base->op_actions[TGSI_OPCODE_MAD].emit = mad_emit;
   bld_base->op_actions[TGSI_OPCODE_MOV].emit = mov_emit;
   bld_base->op_actions[TGSI_OPCODE_MUL].emit = mul_emit;
   bld_base->op_actions[TGSI_OPCODE_SFL].emit = sfl_emit;
   bld_base->op_actions[TGSI_OPCODE_STR].emit = str_emit;
   bld_base->op_actions[TGSI_OPCODE_SUB].emit = sub_emit;

   bld_base->op_actions[TGSI_OPCODE_UARL].emit = mov_emit;
   bld_base->op_actions[TGSI_OPCODE_U2F].emit = u2f_emit;
   bld_base->op_actions[TGSI_OPCODE_UMAD].emit = umad_emit;
   bld_base->op_actions[TGSI_OPCODE_UMUL].emit = umul_emit;
}

/* CPU Only default actions */

/* These actions are CPU only, because they could potentially output SSE
 * intrinsics.
 */

/* TGSI_OPCODE_ABS (CPU Only)*/

static void
abs_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_abs(&bld_base->base,
                                                       emit_data->args[0]);
}

/* TGSI_OPCODE_ADD (CPU Only) */
static void
add_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_add(&bld_base->base,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_AND (CPU Only) */
static void
and_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_and(&bld_base->uint_bld,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_ARL (CPU Only) */
static void
arl_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp;
   tmp = lp_build_floor(&bld_base->base,
			emit_data->args[0]);
   emit_data->output[emit_data->chan] = LLVMBuildFPToSI(bld_base->base.gallivm->builder, tmp,
							bld_base->uint_bld.vec_type, "");
}

/* TGSI_OPCODE_ARR (CPU Only) */
static void
arr_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_iround(&bld_base->base, emit_data->args[0]);
}

/* TGSI_OPCODE_CEIL (CPU Only) */
static void
ceil_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_ceil(&bld_base->base,
                                                      emit_data->args[0]);
}

/* TGSI_OPCODE_CMP (CPU Only) */
static void
cmp_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef cond = lp_build_cmp(&bld_base->base, PIPE_FUNC_LESS,
                                   emit_data->args[0], bld_base->base.zero);
   emit_data->output[emit_data->chan] = lp_build_select(&bld_base->base,
                                cond, emit_data->args[1], emit_data->args[2]);
}

/* TGSI_OPCODE_CND (CPU Only) */
static void
cnd_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef half, tmp;
   half = lp_build_const_vec(bld_base->base.gallivm, bld_base->base.type, 0.5);
   tmp = lp_build_cmp(&bld_base->base, PIPE_FUNC_GREATER,
                      emit_data->args[2], half);
   emit_data->output[emit_data->chan] = lp_build_select(&bld_base->base,
                                          tmp,
                                          emit_data->args[0],
                                          emit_data->args[1]);
}

/* TGSI_OPCODE_COS (CPU Only) */
static void
cos_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_cos(&bld_base->base,
                                                       emit_data->args[0]);
}

/* TGSI_OPCODE_DIV (CPU Only) */
static void
div_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_div(&bld_base->base,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_EX2 (CPU Only) */
static void
ex2_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_exp2(&bld_base->base,
                                                        emit_data->args[0]);
}

/* TGSI_OPCODE_EXP (CPU Only) */
static void
exp_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   lp_build_exp2_approx(&bld_base->base, emit_data->args[0],
                        &emit_data->output[TGSI_CHAN_X],
                        &emit_data->output[TGSI_CHAN_Y],
                        &emit_data->output[TGSI_CHAN_Z]);
   emit_data->output[TGSI_CHAN_W] = bld_base->base.one;
}

/* TGSI_OPCODE_F2I (CPU Only) */
static void
f2i_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_itrunc(&bld_base->base,
                                                        emit_data->args[0]);
}

/* TGSI_OPCODE_F2U (CPU Only) */
static void
f2u_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   /* FIXME: implement and use lp_build_utrunc() */
   emit_data->output[emit_data->chan] = lp_build_itrunc(&bld_base->base,
                                                        emit_data->args[0]);
}

/* TGSI_OPCODE_FLR (CPU Only) */

static void
flr_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_floor(&bld_base->base,
                                                         emit_data->args[0]);
}

/* TGSI_OPCODE_I2F (CPU Only) */
static void
i2f_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_int_to_float(&bld_base->base,
                                                              emit_data->args[0]);
}

/* TGSI_OPCODE_IABS (CPU Only) */
static void
iabs_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_abs(&bld_base->int_bld,
                                                       emit_data->args[0]);
}

/* TGSI_OPCODE_IDIV (CPU Only) */
static void
idiv_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_div(&bld_base->int_bld,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_INEG (CPU Only) */
static void
ineg_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_sub(&bld_base->int_bld,
                                                     bld_base->int_bld.zero,
                                                     emit_data->args[0]);
}

/* TGSI_OPCODE_ISET Helper (CPU Only) */
static void
iset_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data,
   unsigned pipe_func)
{
   LLVMValueRef nz = lp_build_const_vec(bld_base->base.gallivm,
					bld_base->int_bld.type, ~0U);
   LLVMValueRef cond = lp_build_cmp(&bld_base->int_bld, pipe_func,
                                    emit_data->args[0], emit_data->args[1]);
   emit_data->output[emit_data->chan] = lp_build_select(&bld_base->int_bld,
                                          cond,
                                          nz,
                                          bld_base->int_bld.zero);
}

/* TGSI_OPCODE_IMAX (CPU Only) */
static void
imax_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_max(&bld_base->int_bld,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_IMIN (CPU Only) */
static void
imin_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_min(&bld_base->int_bld,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_ISGE (CPU Only) */
static void
isge_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   iset_emit_cpu(action, bld_base, emit_data, PIPE_FUNC_GEQUAL);
}

/* TGSI_OPCODE_ISHR (CPU Only) */
static void
ishr_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_shr(&bld_base->int_bld,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_ISLT (CPU Only) */
static void
islt_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   iset_emit_cpu(action, bld_base, emit_data, PIPE_FUNC_LESS);
}


/* TGSI_OPCODE_ISSG (CPU Only) */
static void
issg_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_sgn(&bld_base->int_bld,
                                                       emit_data->args[0]);
}

/* TGSI_OPCODE_LG2 (CPU Only) */
static void
lg2_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_log2(&bld_base->base,
                                                        emit_data->args[0]);
}

/* TGSI_OPCODE_LOG (CPU Only) */
static void
log_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef p_floor_log2;
   LLVMValueRef p_exp;
   LLVMValueRef p_log2;
   LLVMValueRef src0 = emit_data->args[0];

   lp_build_log2_approx(&bld_base->base, src0,
                        &p_exp, &p_floor_log2, &p_log2);

   emit_data->output[TGSI_CHAN_X] = p_floor_log2;

   emit_data->output[TGSI_CHAN_Y] = lp_build_emit_llvm_binary(bld_base,
                                             TGSI_OPCODE_DIV,
                                             src0, p_exp);
   emit_data->output[TGSI_CHAN_Z] = p_log2;

   emit_data->output[TGSI_CHAN_W] = bld_base->base.one;

}

/* TGSI_OPCODE_MAX (CPU Only) */

static void
max_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_max(&bld_base->base,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_MIN (CPU Only) */
static void
min_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_min(&bld_base->base,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_MOD (CPU Only) */
static void
mod_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_mod(&bld_base->int_bld,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_NOT */
static void
not_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_not(&bld_base->base,
                                                     emit_data->args[0]);
}

/* TGSI_OPCODE_OR (CPU Only) */
static void
or_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_or(&bld_base->uint_bld,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_POW (CPU Only) */
static void
pow_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_pow(&bld_base->base,
                                   emit_data->args[0], emit_data->args[1]);
}


/* TGSI_OPCODE_RCP (CPU Only) */

static void
rcp_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_rcp(&bld_base->base,
                                                       emit_data->args[0]);
}

/* Reciprical squareroot (CPU Only) */

/* This is not the same as TGSI_OPCODE_RSQ, which requres the argument to be
 * greater than or equal to 0 */
static void
recip_sqrt_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_rsqrt(&bld_base->base,
                                                         emit_data->args[0]);
}

/* TGSI_OPCODE_ROUND (CPU Only) */
static void
round_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_round(&bld_base->base,
                                                         emit_data->args[0]);
}

/* TGSI_OPCODE_SET Helper (CPU Only) */

static void
set_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data,
   unsigned pipe_func)
{
   LLVMValueRef cond = lp_build_cmp(&bld_base->base, pipe_func,
                                    emit_data->args[0], emit_data->args[1]);
   emit_data->output[emit_data->chan] = lp_build_select(&bld_base->base,
                                          cond,
                                          bld_base->base.one,
                                          bld_base->base.zero);
}

/* TGSI_OPCODE_SEQ (CPU Only) */

static void
seq_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   set_emit_cpu(action, bld_base, emit_data, PIPE_FUNC_EQUAL);
}

/* TGSI_OPCODE_SGE (CPU Only) */
static void
sge_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   set_emit_cpu(action, bld_base, emit_data, PIPE_FUNC_GEQUAL);
}

/* TGSI_OPCODE_SGT (CPU Only)*/

static void
sgt_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   set_emit_cpu(action, bld_base, emit_data, PIPE_FUNC_GREATER);
}

/* TGSI_OPCODE_SHL (CPU Only) */
static void
shl_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_shl(&bld_base->uint_bld,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_SIN (CPU Only) */
static void
sin_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_sin(&bld_base->base,
                                                       emit_data->args[0]);
}

/* TGSI_OPCODE_SLE (CPU Only) */
static void
sle_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   set_emit_cpu(action, bld_base, emit_data, PIPE_FUNC_LEQUAL);
}

/* TGSI_OPCODE_SLT (CPU Only) */
static void
slt_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   set_emit_cpu(action, bld_base, emit_data, PIPE_FUNC_LESS);
}

/* TGSI_OPCODE_SNE (CPU Only) */

static void
sne_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   set_emit_cpu(action, bld_base, emit_data, PIPE_FUNC_NOTEQUAL);
}

/* TGSI_OPCODE_SSG (CPU Only) */

static void
ssg_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_sgn(&bld_base->base,
                                                       emit_data->args[0]);
}

/* TGSI_OPCODE_SUB (CPU Only) */

static void
sub_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_sub(&bld_base->base,
                                                        emit_data->args[0],
                                                        emit_data->args[1]);
}

/* TGSI_OPCODE_TRUNC (CPU Only) */

static void
trunc_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_trunc(&bld_base->base,
                                                         emit_data->args[0]);
}

/* TGSI_OPCODE_UADD (CPU Only) */
static void
uadd_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_add(&bld_base->uint_bld,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_UDIV (CPU Only) */
static void
udiv_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_div(&bld_base->uint_bld,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_UMAX (CPU Only) */
static void
umax_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_max(&bld_base->uint_bld,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_UMIN (CPU Only) */
static void
umin_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_min(&bld_base->uint_bld,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_UMOD (CPU Only) */
static void
umod_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_mod(&bld_base->uint_bld,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_USET Helper (CPU Only) */
static void
uset_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data,
   unsigned pipe_func)
{
   LLVMValueRef nz = lp_build_const_vec(bld_base->base.gallivm,
					bld_base->uint_bld.type, ~0U);
   LLVMValueRef cond = lp_build_cmp(&bld_base->uint_bld, pipe_func,
                                    emit_data->args[0], emit_data->args[1]);
   emit_data->output[emit_data->chan] = lp_build_select(&bld_base->uint_bld,
                                          cond,
					  nz,
                                          bld_base->uint_bld.zero);
}


/* TGSI_OPCODE_USEQ (CPU Only) */
static void
useq_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   uset_emit_cpu(action, bld_base, emit_data, PIPE_FUNC_EQUAL);
}

/* TGSI_OPCODE_ISGE (CPU Only) */
static void
usge_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   uset_emit_cpu(action, bld_base, emit_data, PIPE_FUNC_GEQUAL);
}

/* TGSI_OPCODE_USHR (CPU Only) */
static void
ushr_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_shr(&bld_base->uint_bld,
                                   emit_data->args[0], emit_data->args[1]);
}

/* TGSI_OPCODE_ISLT (CPU Only) */
static void
uslt_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   uset_emit_cpu(action, bld_base, emit_data, PIPE_FUNC_LESS);
}

/* TGSI_OPCODE_USNE (CPU Only) */

static void
usne_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   uset_emit_cpu(action, bld_base, emit_data, PIPE_FUNC_NOTEQUAL);
}

/* TGSI_OPCODE_XOR */
static void
xor_emit_cpu(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   emit_data->output[emit_data->chan] = lp_build_xor(&bld_base->uint_bld,
                                                     emit_data->args[0],
                                                     emit_data->args[1]);
}

void
lp_set_default_actions_cpu(
   struct lp_build_tgsi_context * bld_base)
{
   lp_set_default_actions(bld_base);
   bld_base->op_actions[TGSI_OPCODE_ABS].emit = abs_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_ADD].emit = add_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_AND].emit = and_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_ARL].emit = arl_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_ARR].emit = arr_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_CEIL].emit = ceil_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_CND].emit = cnd_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_COS].emit = cos_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_CMP].emit = cmp_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_DIV].emit = div_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_EX2].emit = ex2_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_EXP].emit = exp_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_F2I].emit = f2i_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_F2U].emit = f2u_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_FLR].emit = flr_emit_cpu;

   bld_base->op_actions[TGSI_OPCODE_I2F].emit = i2f_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_IABS].emit = iabs_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_IDIV].emit = idiv_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_INEG].emit = ineg_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_IMAX].emit = imax_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_IMIN].emit = imin_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_ISGE].emit = isge_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_ISHR].emit = ishr_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_ISLT].emit = islt_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_ISSG].emit = issg_emit_cpu;

   bld_base->op_actions[TGSI_OPCODE_LG2].emit = lg2_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_LOG].emit = log_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_MAX].emit = max_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_MIN].emit = min_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_MOD].emit = mod_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_NOT].emit = not_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_OR].emit = or_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_POW].emit = pow_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_RCP].emit = rcp_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_ROUND].emit = round_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_SEQ].emit = seq_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_SGE].emit = sge_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_SGT].emit = sgt_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_SIN].emit = sin_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_SHL].emit = shl_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_SLE].emit = sle_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_SLT].emit = slt_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_SNE].emit = sne_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_SSG].emit = ssg_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_SUB].emit = sub_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_TRUNC].emit = trunc_emit_cpu;

   bld_base->rsq_action.emit = recip_sqrt_emit_cpu;

   bld_base->op_actions[TGSI_OPCODE_UADD].emit = uadd_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_UDIV].emit = udiv_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_UMAX].emit = umax_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_UMIN].emit = umin_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_UMOD].emit = umod_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_USEQ].emit = useq_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_USGE].emit = usge_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_USHR].emit = ushr_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_USLT].emit = uslt_emit_cpu;
   bld_base->op_actions[TGSI_OPCODE_USNE].emit = usne_emit_cpu;

   bld_base->op_actions[TGSI_OPCODE_XOR].emit = xor_emit_cpu;

}
