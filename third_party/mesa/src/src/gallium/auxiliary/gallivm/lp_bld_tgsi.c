/**************************************************************************
 *
 * Copyright 2011-2012 Advanced Micro Devices, Inc.
 * Copyright 2010 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "gallivm/lp_bld_tgsi.h"

#include "gallivm/lp_bld_arit.h"
#include "gallivm/lp_bld_gather.h"
#include "gallivm/lp_bld_init.h"
#include "gallivm/lp_bld_intr.h"
#include "tgsi/tgsi_info.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "util/u_memory.h"

/* The user is responsible for freeing list->instructions */
unsigned lp_bld_tgsi_list_init(struct lp_build_tgsi_context * bld_base)
{
   bld_base->instructions = (struct tgsi_full_instruction *)
         MALLOC( LP_MAX_INSTRUCTIONS * sizeof(struct tgsi_full_instruction) );
   if (!bld_base->instructions) {
      return 0;
   }
   bld_base->max_instructions = LP_MAX_INSTRUCTIONS;
   return 1;
}


unsigned lp_bld_tgsi_add_instruction(
   struct lp_build_tgsi_context * bld_base,
   struct tgsi_full_instruction *inst_to_add)
{

   if (bld_base->num_instructions == bld_base->max_instructions) {
      struct tgsi_full_instruction *instructions;
      instructions = REALLOC(bld_base->instructions, bld_base->max_instructions
                                      * sizeof(struct tgsi_full_instruction),
                                      (bld_base->max_instructions + LP_MAX_INSTRUCTIONS)
                                      * sizeof(struct tgsi_full_instruction));
      if (!instructions) {
         return 0;
      }
      bld_base->instructions = instructions;
      bld_base->max_instructions += LP_MAX_INSTRUCTIONS;
   }
   memcpy(bld_base->instructions + bld_base->num_instructions, inst_to_add,
          sizeof(bld_base->instructions[0]));

   bld_base->num_instructions++;

   return 1;
}


/**
 * This function assumes that all the args in emit_data have been set.
 */
static void
lp_build_action_set_dst_type(
   struct lp_build_emit_data * emit_data,
   struct lp_build_tgsi_context *bld_base,
   unsigned tgsi_opcode)
{
   if (emit_data->arg_count == 0) {
      emit_data->dst_type = LLVMVoidTypeInContext(bld_base->base.gallivm->context);
   } else {
      /* XXX: Not all opcodes have the same src and dst types. */
      emit_data->dst_type = LLVMTypeOf(emit_data->args[0]);
   }
}

void
lp_build_tgsi_intrinsic(
 const struct lp_build_tgsi_action * action,
 struct lp_build_tgsi_context * bld_base,
 struct lp_build_emit_data * emit_data)
{
   struct lp_build_context * base = &bld_base->base;
   emit_data->output[emit_data->chan] = lp_build_intrinsic(
               base->gallivm->builder, action->intr_name,
               emit_data->dst_type, emit_data->args, emit_data->arg_count);
}

LLVMValueRef
lp_build_emit_llvm(
   struct lp_build_tgsi_context *bld_base,
   unsigned tgsi_opcode,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_action * action = &bld_base->op_actions[tgsi_opcode];
   /* XXX: Assert that this is a componentwise or replicate instruction */

   lp_build_action_set_dst_type(emit_data, bld_base, tgsi_opcode);
   emit_data->chan = 0;
   assert(action->emit);
   action->emit(action, bld_base, emit_data);
   return emit_data->output[0];
}

LLVMValueRef
lp_build_emit_llvm_unary(
   struct lp_build_tgsi_context *bld_base,
   unsigned tgsi_opcode,
   LLVMValueRef arg0)
{
   struct lp_build_emit_data emit_data;
   emit_data.arg_count = 1;
   emit_data.args[0] = arg0;
   return lp_build_emit_llvm(bld_base, tgsi_opcode, &emit_data);
}

LLVMValueRef
lp_build_emit_llvm_binary(
   struct lp_build_tgsi_context *bld_base,
   unsigned tgsi_opcode,
   LLVMValueRef arg0,
   LLVMValueRef arg1)
{
   struct lp_build_emit_data emit_data;
   emit_data.arg_count = 2;
   emit_data.args[0] = arg0;
   emit_data.args[1] = arg1;
   return lp_build_emit_llvm(bld_base, tgsi_opcode, &emit_data);
}

LLVMValueRef
lp_build_emit_llvm_ternary(
   struct lp_build_tgsi_context *bld_base,
   unsigned tgsi_opcode,
   LLVMValueRef arg0,
   LLVMValueRef arg1,
   LLVMValueRef arg2)
{
   struct lp_build_emit_data emit_data;
   emit_data.arg_count = 3;
   emit_data.args[0] = arg0;
   emit_data.args[1] = arg1;
   emit_data.args[2] = arg2;
   return lp_build_emit_llvm(bld_base, tgsi_opcode, &emit_data);
}

/**
 * The default fetch implementation.
 */
void lp_build_fetch_args(
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   unsigned src;
   for (src = 0; src < emit_data->info->num_src; src++) {
      emit_data->args[src] = lp_build_emit_fetch(bld_base, emit_data->inst, src,
                                               emit_data->chan);
   }
   emit_data->arg_count = emit_data->info->num_src;
   lp_build_action_set_dst_type(emit_data, bld_base,
		emit_data->inst->Instruction.Opcode);
}

/* XXX: COMMENT
 * It should be assumed that this function ignores writemasks
 */
boolean
lp_build_tgsi_inst_llvm(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_full_instruction * inst)
{
   unsigned tgsi_opcode = inst->Instruction.Opcode;
   const struct tgsi_opcode_info * info = tgsi_get_opcode_info(tgsi_opcode);
   const struct lp_build_tgsi_action * action =
                                         &bld_base->op_actions[tgsi_opcode];
   struct lp_build_emit_data emit_data;
   unsigned chan_index;
   LLVMValueRef val;

   bld_base->pc++;

   /* Ignore deprecated instructions */
   switch (inst->Instruction.Opcode) {

   case TGSI_OPCODE_RCC:
   case TGSI_OPCODE_UP2H:
   case TGSI_OPCODE_UP2US:
   case TGSI_OPCODE_UP4B:
   case TGSI_OPCODE_UP4UB:
   case TGSI_OPCODE_X2D:
   case TGSI_OPCODE_ARA:
   case TGSI_OPCODE_BRA:
   case TGSI_OPCODE_DIV:
   case TGSI_OPCODE_PUSHA:
   case TGSI_OPCODE_POPA:
   case TGSI_OPCODE_SAD:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;
   }

   /* Check if the opcode has been implemented */
   if (!action->emit) {
      return FALSE;
   }

   memset(&emit_data, 0, sizeof(emit_data));

   assert(info->num_dst <= 1);
   if (info->num_dst) {
      TGSI_FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         emit_data.output[chan_index] = bld_base->base.undef;
      }
   }

   emit_data.inst = inst;
   emit_data.info = info;

   /* Emit the instructions */
   if (info->output_mode == TGSI_OUTPUT_COMPONENTWISE && bld_base->soa) {
      TGSI_FOR_EACH_DST0_ENABLED_CHANNEL(inst, chan_index) {
         emit_data.chan = chan_index;
         if (!action->fetch_args) {
            lp_build_fetch_args(bld_base, &emit_data);
         } else {
             action->fetch_args(bld_base, &emit_data);
         }
         action->emit(action, bld_base, &emit_data);
      }
   } else {
      emit_data.chan = LP_CHAN_ALL;
      if (action->fetch_args) {
         action->fetch_args(bld_base, &emit_data);
      }
      /* Make sure the output value is stored in emit_data.output[0], unless
       * the opcode is channel dependent */
      if (info->output_mode != TGSI_OUTPUT_CHAN_DEPENDENT) {
         emit_data.chan = 0;
      }
      action->emit(action, bld_base, &emit_data);

      /* Replicate the output values */
      if (info->output_mode == TGSI_OUTPUT_REPLICATE && bld_base->soa) {
         val = emit_data.output[0];
         memset(emit_data.output, 0, sizeof(emit_data.output));
         TGSI_FOR_EACH_DST0_ENABLED_CHANNEL(inst, chan_index) {
            emit_data.output[chan_index] = val;
         }
      }
   }

   if (info->num_dst > 0) {
      bld_base->emit_store(bld_base, inst, info, emit_data.output);
   }
   return TRUE;
}


LLVMValueRef
lp_build_emit_fetch(
   struct lp_build_tgsi_context *bld_base,
   const struct tgsi_full_instruction *inst,
   unsigned src_op,
   const unsigned chan_index)
{
   const struct tgsi_full_src_register *reg = &inst->Src[src_op];
   unsigned swizzle;
   LLVMValueRef res;
   enum tgsi_opcode_type stype = tgsi_opcode_infer_src_type(inst->Instruction.Opcode);

   if (chan_index == LP_CHAN_ALL) {
      swizzle = ~0;
   } else {
      swizzle = tgsi_util_get_full_src_register_swizzle(reg, chan_index);
      if (swizzle > 3) {
         assert(0 && "invalid swizzle in emit_fetch()");
         return bld_base->base.undef;
      }
   }

   assert(reg->Register.Index <= bld_base->info->file_max[reg->Register.File]);

   if (bld_base->emit_fetch_funcs[reg->Register.File]) {
      res = bld_base->emit_fetch_funcs[reg->Register.File](bld_base, reg, stype,
                                                           swizzle);
   } else {
      assert(0 && "invalid src register in emit_fetch()");
      return bld_base->base.undef;
   }

   if (reg->Register.Absolute) {
      res = lp_build_emit_llvm_unary(bld_base, TGSI_OPCODE_ABS, res);
   }

   if (reg->Register.Negate) {
      res = lp_build_negate( &bld_base->base, res );
   }

   /*
    * Swizzle the argument
    */

   if (swizzle == ~0) {
      res = bld_base->emit_swizzle(bld_base, res,
                     reg->Register.SwizzleX,
                     reg->Register.SwizzleY,
                     reg->Register.SwizzleZ,
                     reg->Register.SwizzleW);
   }

   return res;

}

boolean
lp_build_tgsi_llvm(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_token *tokens)
{
   struct tgsi_parse_context parse;

   if (bld_base->emit_prologue) {
      bld_base->emit_prologue(bld_base);
   }

   if (!lp_bld_tgsi_list_init(bld_base)) {
      return FALSE;
   }

   tgsi_parse_init( &parse, tokens );

   while( !tgsi_parse_end_of_tokens( &parse ) ) {
      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         /* Inputs already interpolated */
         bld_base->emit_declaration(bld_base, &parse.FullToken.FullDeclaration);
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         lp_bld_tgsi_add_instruction(bld_base, &parse.FullToken.FullInstruction);
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         bld_base->emit_immediate(bld_base, &parse.FullToken.FullImmediate);
         break;

      case TGSI_TOKEN_TYPE_PROPERTY:
         break;

      default:
         assert( 0 );
      }
   }

   while (bld_base->pc != -1) {
      struct tgsi_full_instruction *instr = bld_base->instructions +
							bld_base->pc;
      const struct tgsi_opcode_info *opcode_info =
         tgsi_get_opcode_info(instr->Instruction.Opcode);
      if (!lp_build_tgsi_inst_llvm(bld_base, instr)) {
         _debug_printf("warning: failed to translate tgsi opcode %s to LLVM\n",
                       opcode_info->mnemonic);
         return FALSE;
      }
   }

   tgsi_parse_free(&parse);

   FREE(bld_base->instructions);

   if (bld_base->emit_epilogue) {
      bld_base->emit_epilogue(bld_base);
   }

   return TRUE;
}
