/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/** @file brw_fs_visitor.cpp
 *
 * This file supports generating the FS LIR from the GLSL IR.  The LIR
 * makes it easier to do backend-specific optimizations than doing so
 * in the GLSL IR or in the native code.
 */
extern "C" {

#include <sys/types.h>

#include "main/macros.h"
#include "main/shaderobj.h"
#include "main/uniforms.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"
#include "program/prog_optimize.h"
#include "program/register_allocate.h"
#include "program/sampler.h"
#include "program/hash_table.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_wm.h"
}
#include "brw_shader.h"
#include "brw_fs.h"
#include "glsl/glsl_types.h"
#include "glsl/ir_optimization.h"
#include "glsl/ir_print_visitor.h"

void
fs_visitor::visit(ir_variable *ir)
{
   fs_reg *reg = NULL;

   if (variable_storage(ir))
      return;

   if (ir->mode == ir_var_in) {
      if (!strcmp(ir->name, "gl_FragCoord")) {
	 reg = emit_fragcoord_interpolation(ir);
      } else if (!strcmp(ir->name, "gl_FrontFacing")) {
	 reg = emit_frontfacing_interpolation(ir);
      } else {
	 reg = emit_general_interpolation(ir);
      }
      assert(reg);
      hash_table_insert(this->variable_ht, reg, ir);
      return;
   } else if (ir->mode == ir_var_out) {
      reg = new(this->mem_ctx) fs_reg(this, ir->type);

      if (ir->index > 0) {
	 assert(ir->location == FRAG_RESULT_DATA0);
	 assert(ir->index == 1);
	 this->dual_src_output = *reg;
      } else if (ir->location == FRAG_RESULT_COLOR) {
	 /* Writing gl_FragColor outputs to all color regions. */
	 for (unsigned int i = 0; i < MAX2(c->key.nr_color_regions, 1); i++) {
	    this->outputs[i] = *reg;
	    this->output_components[i] = 4;
	 }
      } else if (ir->location == FRAG_RESULT_DEPTH) {
	 this->frag_depth = ir;
      } else {
	 /* gl_FragData or a user-defined FS output */
	 assert(ir->location >= FRAG_RESULT_DATA0 &&
		ir->location < FRAG_RESULT_DATA0 + BRW_MAX_DRAW_BUFFERS);

	 int vector_elements =
	    ir->type->is_array() ? ir->type->fields.array->vector_elements
				 : ir->type->vector_elements;

	 /* General color output. */
	 for (unsigned int i = 0; i < MAX2(1, ir->type->length); i++) {
	    int output = ir->location - FRAG_RESULT_DATA0 + i;
	    this->outputs[output] = *reg;
	    this->outputs[output].reg_offset += vector_elements * i;
	    this->output_components[output] = vector_elements;
	 }
      }
   } else if (ir->mode == ir_var_uniform) {
      int param_index = c->prog_data.nr_params;

      /* Thanks to the lower_ubo_reference pass, we will see only
       * ir_binop_ubo_load expressions and not ir_dereference_variable for UBO
       * variables, so no need for them to be in variable_ht.
       */
      if (ir->uniform_block != -1)
         return;

      if (c->dispatch_width == 16) {
	 if (!variable_storage(ir)) {
	    fail("Failed to find uniform '%s' in 16-wide\n", ir->name);
	 }
	 return;
      }

      if (!strncmp(ir->name, "gl_", 3)) {
	 setup_builtin_uniform_values(ir);
      } else {
	 setup_uniform_values(ir->location, ir->type);
      }

      reg = new(this->mem_ctx) fs_reg(UNIFORM, param_index);
      reg->type = brw_type_for_base_type(ir->type);
   }

   if (!reg)
      reg = new(this->mem_ctx) fs_reg(this, ir->type);

   hash_table_insert(this->variable_ht, reg, ir);
}

void
fs_visitor::visit(ir_dereference_variable *ir)
{
   fs_reg *reg = variable_storage(ir->var);
   this->result = *reg;
}

void
fs_visitor::visit(ir_dereference_record *ir)
{
   const glsl_type *struct_type = ir->record->type;

   ir->record->accept(this);

   unsigned int offset = 0;
   for (unsigned int i = 0; i < struct_type->length; i++) {
      if (strcmp(struct_type->fields.structure[i].name, ir->field) == 0)
	 break;
      offset += type_size(struct_type->fields.structure[i].type);
   }
   this->result.reg_offset += offset;
   this->result.type = brw_type_for_base_type(ir->type);
}

void
fs_visitor::visit(ir_dereference_array *ir)
{
   ir_constant *index;
   int element_size;

   ir->array->accept(this);
   index = ir->array_index->as_constant();

   element_size = type_size(ir->type);
   this->result.type = brw_type_for_base_type(ir->type);

   if (index) {
      assert(this->result.file == UNIFORM || this->result.file == GRF);
      this->result.reg_offset += index->value.i[0] * element_size;
   } else {
      assert(!"FINISHME: non-constant array element");
   }
}

/* Instruction selection: Produce a MOV.sat instead of
 * MIN(MAX(val, 0), 1) when possible.
 */
bool
fs_visitor::try_emit_saturate(ir_expression *ir)
{
   ir_rvalue *sat_val = ir->as_rvalue_to_saturate();

   if (!sat_val)
      return false;

   fs_inst *pre_inst = (fs_inst *) this->instructions.get_tail();

   sat_val->accept(this);
   fs_reg src = this->result;

   fs_inst *last_inst = (fs_inst *) this->instructions.get_tail();

   /* If the last instruction from our accept() didn't generate our
    * src, generate a saturated MOV
    */
   fs_inst *modify = get_instruction_generating_reg(pre_inst, last_inst, src);
   if (!modify || modify->regs_written() != 1) {
      this->result = fs_reg(this, ir->type);
      fs_inst *inst = emit(BRW_OPCODE_MOV, this->result, src);
      inst->saturate = true;
   } else {
      modify->saturate = true;
      this->result = src;
   }


   return true;
}

bool
fs_visitor::try_emit_mad(ir_expression *ir, int mul_arg)
{
   /* 3-src instructions were introduced in gen6. */
   if (intel->gen < 6)
      return false;

   /* MAD can only handle floating-point data. */
   if (ir->type != glsl_type::float_type)
      return false;

   ir_rvalue *nonmul = ir->operands[1 - mul_arg];
   ir_expression *mul = ir->operands[mul_arg]->as_expression();

   if (!mul || mul->operation != ir_binop_mul)
      return false;

   if (nonmul->as_constant() ||
       mul->operands[0]->as_constant() ||
       mul->operands[1]->as_constant())
      return false;

   nonmul->accept(this);
   fs_reg src0 = this->result;

   mul->operands[0]->accept(this);
   fs_reg src1 = this->result;

   mul->operands[1]->accept(this);
   fs_reg src2 = this->result;

   this->result = fs_reg(this, ir->type);
   emit(BRW_OPCODE_MAD, this->result, src0, src1, src2);

   return true;
}

void
fs_visitor::visit(ir_expression *ir)
{
   unsigned int operand;
   fs_reg op[2], temp;
   fs_inst *inst;

   assert(ir->get_num_operands() <= 2);

   if (try_emit_saturate(ir))
      return;
   if (ir->operation == ir_binop_add) {
      if (try_emit_mad(ir, 0) || try_emit_mad(ir, 1))
	 return;
   }

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      ir->operands[operand]->accept(this);
      if (this->result.file == BAD_FILE) {
	 ir_print_visitor v;
	 fail("Failed to get tree for expression operand:\n");
	 ir->operands[operand]->accept(&v);
      }
      op[operand] = this->result;

      /* Matrix expression operands should have been broken down to vector
       * operations already.
       */
      assert(!ir->operands[operand]->type->is_matrix());
      /* And then those vector operands should have been broken down to scalar.
       */
      assert(!ir->operands[operand]->type->is_vector());
   }

   /* Storage for our result.  If our result goes into an assignment, it will
    * just get copy-propagated out, so no worries.
    */
   this->result = fs_reg(this, ir->type);

   switch (ir->operation) {
   case ir_unop_logic_not:
      /* Note that BRW_OPCODE_NOT is not appropriate here, since it is
       * ones complement of the whole register, not just bit 0.
       */
      emit(BRW_OPCODE_XOR, this->result, op[0], fs_reg(1));
      break;
   case ir_unop_neg:
      op[0].negate = !op[0].negate;
      this->result = op[0];
      break;
   case ir_unop_abs:
      op[0].abs = true;
      op[0].negate = false;
      this->result = op[0];
      break;
   case ir_unop_sign:
      temp = fs_reg(this, ir->type);

      emit(BRW_OPCODE_MOV, this->result, fs_reg(0.0f));

      inst = emit(BRW_OPCODE_CMP, reg_null_f, op[0], fs_reg(0.0f));
      inst->conditional_mod = BRW_CONDITIONAL_G;
      inst = emit(BRW_OPCODE_MOV, this->result, fs_reg(1.0f));
      inst->predicated = true;

      inst = emit(BRW_OPCODE_CMP, reg_null_f, op[0], fs_reg(0.0f));
      inst->conditional_mod = BRW_CONDITIONAL_L;
      inst = emit(BRW_OPCODE_MOV, this->result, fs_reg(-1.0f));
      inst->predicated = true;

      break;
   case ir_unop_rcp:
      emit_math(SHADER_OPCODE_RCP, this->result, op[0]);
      break;

   case ir_unop_exp2:
      emit_math(SHADER_OPCODE_EXP2, this->result, op[0]);
      break;
   case ir_unop_log2:
      emit_math(SHADER_OPCODE_LOG2, this->result, op[0]);
      break;
   case ir_unop_exp:
   case ir_unop_log:
      assert(!"not reached: should be handled by ir_explog_to_explog2");
      break;
   case ir_unop_sin:
   case ir_unop_sin_reduced:
      emit_math(SHADER_OPCODE_SIN, this->result, op[0]);
      break;
   case ir_unop_cos:
   case ir_unop_cos_reduced:
      emit_math(SHADER_OPCODE_COS, this->result, op[0]);
      break;

   case ir_unop_dFdx:
      emit(FS_OPCODE_DDX, this->result, op[0]);
      break;
   case ir_unop_dFdy:
      emit(FS_OPCODE_DDY, this->result, op[0]);
      break;

   case ir_binop_add:
      emit(BRW_OPCODE_ADD, this->result, op[0], op[1]);
      break;
   case ir_binop_sub:
      assert(!"not reached: should be handled by ir_sub_to_add_neg");
      break;

   case ir_binop_mul:
      if (ir->type->is_integer()) {
	 /* For integer multiplication, the MUL uses the low 16 bits
	  * of one of the operands (src0 on gen6, src1 on gen7).  The
	  * MACH accumulates in the contribution of the upper 16 bits
	  * of that operand.
	  *
	  * FINISHME: Emit just the MUL if we know an operand is small
	  * enough.
	  */
	 if (intel->gen >= 7 && c->dispatch_width == 16)
	    fail("16-wide explicit accumulator operands unsupported\n");

	 struct brw_reg acc = retype(brw_acc_reg(), BRW_REGISTER_TYPE_D);

	 emit(BRW_OPCODE_MUL, acc, op[0], op[1]);
	 emit(BRW_OPCODE_MACH, reg_null_d, op[0], op[1]);
	 emit(BRW_OPCODE_MOV, this->result, fs_reg(acc));
      } else {
	 emit(BRW_OPCODE_MUL, this->result, op[0], op[1]);
      }
      break;
   case ir_binop_div:
      if (intel->gen >= 7 && c->dispatch_width == 16)
	 fail("16-wide INTDIV unsupported\n");

      /* Floating point should be lowered by DIV_TO_MUL_RCP in the compiler. */
      assert(ir->type->is_integer());
      emit_math(SHADER_OPCODE_INT_QUOTIENT, this->result, op[0], op[1]);
      break;
   case ir_binop_mod:
      if (intel->gen >= 7 && c->dispatch_width == 16)
	 fail("16-wide INTDIV unsupported\n");

      /* Floating point should be lowered by MOD_TO_FRACT in the compiler. */
      assert(ir->type->is_integer());
      emit_math(SHADER_OPCODE_INT_REMAINDER, this->result, op[0], op[1]);
      break;

   case ir_binop_less:
   case ir_binop_greater:
   case ir_binop_lequal:
   case ir_binop_gequal:
   case ir_binop_equal:
   case ir_binop_all_equal:
   case ir_binop_nequal:
   case ir_binop_any_nequal:
      temp = this->result;
      /* original gen4 does implicit conversion before comparison. */
      if (intel->gen < 5)
	 temp.type = op[0].type;

      resolve_ud_negate(&op[0]);
      resolve_ud_negate(&op[1]);

      resolve_bool_comparison(ir->operands[0], &op[0]);
      resolve_bool_comparison(ir->operands[1], &op[1]);

      inst = emit(BRW_OPCODE_CMP, temp, op[0], op[1]);
      inst->conditional_mod = brw_conditional_for_comparison(ir->operation);
      break;

   case ir_binop_logic_xor:
      emit(BRW_OPCODE_XOR, this->result, op[0], op[1]);
      break;

   case ir_binop_logic_or:
      emit(BRW_OPCODE_OR, this->result, op[0], op[1]);
      break;

   case ir_binop_logic_and:
      emit(BRW_OPCODE_AND, this->result, op[0], op[1]);
      break;

   case ir_binop_dot:
   case ir_unop_any:
      assert(!"not reached: should be handled by brw_fs_channel_expressions");
      break;

   case ir_unop_noise:
      assert(!"not reached: should be handled by lower_noise");
      break;

   case ir_quadop_vector:
      assert(!"not reached: should be handled by lower_quadop_vector");
      break;

   case ir_unop_sqrt:
      emit_math(SHADER_OPCODE_SQRT, this->result, op[0]);
      break;

   case ir_unop_rsq:
      emit_math(SHADER_OPCODE_RSQ, this->result, op[0]);
      break;

   case ir_unop_bitcast_i2f:
   case ir_unop_bitcast_u2f:
      op[0].type = BRW_REGISTER_TYPE_F;
      this->result = op[0];
      break;
   case ir_unop_i2u:
   case ir_unop_bitcast_f2u:
      op[0].type = BRW_REGISTER_TYPE_UD;
      this->result = op[0];
      break;
   case ir_unop_u2i:
   case ir_unop_bitcast_f2i:
      op[0].type = BRW_REGISTER_TYPE_D;
      this->result = op[0];
      break;
   case ir_unop_i2f:
   case ir_unop_u2f:
   case ir_unop_f2i:
   case ir_unop_f2u:
      emit(BRW_OPCODE_MOV, this->result, op[0]);
      break;

   case ir_unop_b2i:
      inst = emit(BRW_OPCODE_AND, this->result, op[0], fs_reg(1));
      break;
   case ir_unop_b2f:
      temp = fs_reg(this, glsl_type::int_type);
      emit(BRW_OPCODE_AND, temp, op[0], fs_reg(1));
      emit(BRW_OPCODE_MOV, this->result, temp);
      break;

   case ir_unop_f2b:
      inst = emit(BRW_OPCODE_CMP, this->result, op[0], fs_reg(0.0f));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
      emit(BRW_OPCODE_AND, this->result, this->result, fs_reg(1));
      break;
   case ir_unop_i2b:
      assert(op[0].type == BRW_REGISTER_TYPE_D);

      inst = emit(BRW_OPCODE_CMP, this->result, op[0], fs_reg(0));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
      emit(BRW_OPCODE_AND, this->result, this->result, fs_reg(1));
      break;

   case ir_unop_trunc:
      emit(BRW_OPCODE_RNDZ, this->result, op[0]);
      break;
   case ir_unop_ceil:
      op[0].negate = !op[0].negate;
      inst = emit(BRW_OPCODE_RNDD, this->result, op[0]);
      this->result.negate = true;
      break;
   case ir_unop_floor:
      inst = emit(BRW_OPCODE_RNDD, this->result, op[0]);
      break;
   case ir_unop_fract:
      inst = emit(BRW_OPCODE_FRC, this->result, op[0]);
      break;
   case ir_unop_round_even:
      emit(BRW_OPCODE_RNDE, this->result, op[0]);
      break;

   case ir_binop_min:
      resolve_ud_negate(&op[0]);
      resolve_ud_negate(&op[1]);

      if (intel->gen >= 6) {
	 inst = emit(BRW_OPCODE_SEL, this->result, op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_L;
      } else {
	 /* Unalias the destination */
	 this->result = fs_reg(this, ir->type);

	 inst = emit(BRW_OPCODE_CMP, this->result, op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_L;

	 inst = emit(BRW_OPCODE_SEL, this->result, op[0], op[1]);
	 inst->predicated = true;
      }
      break;
   case ir_binop_max:
      resolve_ud_negate(&op[0]);
      resolve_ud_negate(&op[1]);

      if (intel->gen >= 6) {
	 inst = emit(BRW_OPCODE_SEL, this->result, op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_GE;
      } else {
	 /* Unalias the destination */
	 this->result = fs_reg(this, ir->type);

	 inst = emit(BRW_OPCODE_CMP, this->result, op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_G;

	 inst = emit(BRW_OPCODE_SEL, this->result, op[0], op[1]);
	 inst->predicated = true;
      }
      break;

   case ir_binop_pow:
      emit_math(SHADER_OPCODE_POW, this->result, op[0], op[1]);
      break;

   case ir_unop_bit_not:
      inst = emit(BRW_OPCODE_NOT, this->result, op[0]);
      break;
   case ir_binop_bit_and:
      inst = emit(BRW_OPCODE_AND, this->result, op[0], op[1]);
      break;
   case ir_binop_bit_xor:
      inst = emit(BRW_OPCODE_XOR, this->result, op[0], op[1]);
      break;
   case ir_binop_bit_or:
      inst = emit(BRW_OPCODE_OR, this->result, op[0], op[1]);
      break;

   case ir_binop_lshift:
      inst = emit(BRW_OPCODE_SHL, this->result, op[0], op[1]);
      break;

   case ir_binop_rshift:
      if (ir->type->base_type == GLSL_TYPE_INT)
	 inst = emit(BRW_OPCODE_ASR, this->result, op[0], op[1]);
      else
	 inst = emit(BRW_OPCODE_SHR, this->result, op[0], op[1]);
      break;

   case ir_binop_ubo_load:
      ir_constant *uniform_block = ir->operands[0]->as_constant();
      ir_constant *offset = ir->operands[1]->as_constant();

      fs_reg packed_consts = fs_reg(this, glsl_type::float_type);
      packed_consts.type = result.type;
      fs_reg surf_index = fs_reg((unsigned)SURF_INDEX_WM_UBO(uniform_block->value.u[0]));
      fs_inst *pull = emit(fs_inst(FS_OPCODE_PULL_CONSTANT_LOAD,
                                   packed_consts,
                                   surf_index,
                                   fs_reg(offset->value.u[0])));
      pull->base_mrf = 14;
      pull->mlen = 1;

      packed_consts.smear = offset->value.u[0] % 16 / 4;
      for (int i = 0; i < ir->type->vector_elements; i++) {
         /* UBO bools are any nonzero value.  We consider bools to be
          * values with the low bit set to 1.  Convert them using CMP.
          */
         if (ir->type->base_type == GLSL_TYPE_BOOL) {
            fs_inst *inst = emit(fs_inst(BRW_OPCODE_CMP, result,
                                         packed_consts, fs_reg(0u)));
            inst->conditional_mod = BRW_CONDITIONAL_NZ;
         } else {
            emit(fs_inst(BRW_OPCODE_MOV, result, packed_consts));
         }

         packed_consts.smear++;
         result.reg_offset++;

         /* The std140 packing rules don't allow vectors to cross 16-byte
          * boundaries, and a reg is 32 bytes.
          */
         assert(packed_consts.smear < 8);
      }
      result.reg_offset = 0;
      break;
   }
}

void
fs_visitor::emit_assignment_writes(fs_reg &l, fs_reg &r,
				   const glsl_type *type, bool predicated)
{
   switch (type->base_type) {
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_BOOL:
      for (unsigned int i = 0; i < type->components(); i++) {
	 l.type = brw_type_for_base_type(type);
	 r.type = brw_type_for_base_type(type);

	 if (predicated || !l.equals(r)) {
	    fs_inst *inst = emit(BRW_OPCODE_MOV, l, r);
	    inst->predicated = predicated;
	 }

	 l.reg_offset++;
	 r.reg_offset++;
      }
      break;
   case GLSL_TYPE_ARRAY:
      for (unsigned int i = 0; i < type->length; i++) {
	 emit_assignment_writes(l, r, type->fields.array, predicated);
      }
      break;

   case GLSL_TYPE_STRUCT:
      for (unsigned int i = 0; i < type->length; i++) {
	 emit_assignment_writes(l, r, type->fields.structure[i].type,
				predicated);
      }
      break;

   case GLSL_TYPE_SAMPLER:
      break;

   default:
      assert(!"not reached");
      break;
   }
}

/* If the RHS processing resulted in an instruction generating a
 * temporary value, and it would be easy to rewrite the instruction to
 * generate its result right into the LHS instead, do so.  This ends
 * up reliably removing instructions where it can be tricky to do so
 * later without real UD chain information.
 */
bool
fs_visitor::try_rewrite_rhs_to_dst(ir_assignment *ir,
                                   fs_reg dst,
                                   fs_reg src,
                                   fs_inst *pre_rhs_inst,
                                   fs_inst *last_rhs_inst)
{
   /* Only attempt if we're doing a direct assignment. */
   if (ir->condition ||
       !(ir->lhs->type->is_scalar() ||
        (ir->lhs->type->is_vector() &&
         ir->write_mask == (1 << ir->lhs->type->vector_elements) - 1)))
      return false;

   /* Make sure the last instruction generated our source reg. */
   fs_inst *modify = get_instruction_generating_reg(pre_rhs_inst,
						    last_rhs_inst,
						    src);
   if (!modify)
      return false;

   /* If last_rhs_inst wrote a different number of components than our LHS,
    * we can't safely rewrite it.
    */
   if (ir->lhs->type->vector_elements != modify->regs_written())
      return false;

   /* Success!  Rewrite the instruction. */
   modify->dst = dst;

   return true;
}

void
fs_visitor::visit(ir_assignment *ir)
{
   fs_reg l, r;
   fs_inst *inst;

   /* FINISHME: arrays on the lhs */
   ir->lhs->accept(this);
   l = this->result;

   fs_inst *pre_rhs_inst = (fs_inst *) this->instructions.get_tail();

   ir->rhs->accept(this);
   r = this->result;

   fs_inst *last_rhs_inst = (fs_inst *) this->instructions.get_tail();

   assert(l.file != BAD_FILE);
   assert(r.file != BAD_FILE);

   if (try_rewrite_rhs_to_dst(ir, l, r, pre_rhs_inst, last_rhs_inst))
      return;

   if (ir->condition) {
      emit_bool_to_cond_code(ir->condition);
   }

   if (ir->lhs->type->is_scalar() ||
       ir->lhs->type->is_vector()) {
      for (int i = 0; i < ir->lhs->type->vector_elements; i++) {
	 if (ir->write_mask & (1 << i)) {
	    inst = emit(BRW_OPCODE_MOV, l, r);
	    if (ir->condition)
	       inst->predicated = true;
	    r.reg_offset++;
	 }
	 l.reg_offset++;
      }
   } else {
      emit_assignment_writes(l, r, ir->lhs->type, ir->condition != NULL);
   }
}

fs_inst *
fs_visitor::emit_texture_gen4(ir_texture *ir, fs_reg dst, fs_reg coordinate,
			      fs_reg shadow_c, fs_reg lod, fs_reg dPdy)
{
   int mlen;
   int base_mrf = 1;
   bool simd16 = false;
   fs_reg orig_dst;

   /* g0 header. */
   mlen = 1;

   if (ir->shadow_comparitor) {
      for (int i = 0; i < ir->coordinate->type->vector_elements; i++) {
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen + i), coordinate);
	 coordinate.reg_offset++;
      }
      /* gen4's SIMD8 sampler always has the slots for u,v,r present. */
      mlen += 3;

      if (ir->op == ir_tex) {
	 /* There's no plain shadow compare message, so we use shadow
	  * compare with a bias of 0.0.
	  */
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), fs_reg(0.0f));
	 mlen++;
      } else if (ir->op == ir_txb || ir->op == ir_txl) {
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), lod);
	 mlen++;
      } else {
         assert(!"Should not get here.");
      }

      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), shadow_c);
      mlen++;
   } else if (ir->op == ir_tex) {
      for (int i = 0; i < ir->coordinate->type->vector_elements; i++) {
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen + i), coordinate);
	 coordinate.reg_offset++;
      }
      /* gen4's SIMD8 sampler always has the slots for u,v,r present. */
      mlen += 3;
   } else if (ir->op == ir_txd) {
      fs_reg &dPdx = lod;

      for (int i = 0; i < ir->coordinate->type->vector_elements; i++) {
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen + i), coordinate);
	 coordinate.reg_offset++;
      }
      /* the slots for u and v are always present, but r is optional */
      mlen += MAX2(ir->coordinate->type->vector_elements, 2);

      /*  P   = u, v, r
       * dPdx = dudx, dvdx, drdx
       * dPdy = dudy, dvdy, drdy
       *
       * 1-arg: Does not exist.
       *
       * 2-arg: dudx   dvdx   dudy   dvdy
       *        dPdx.x dPdx.y dPdy.x dPdy.y
       *        m4     m5     m6     m7
       *
       * 3-arg: dudx   dvdx   drdx   dudy   dvdy   drdy
       *        dPdx.x dPdx.y dPdx.z dPdy.x dPdy.y dPdy.z
       *        m5     m6     m7     m8     m9     m10
       */
      for (int i = 0; i < ir->lod_info.grad.dPdx->type->vector_elements; i++) {
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), dPdx);
	 dPdx.reg_offset++;
      }
      mlen += MAX2(ir->lod_info.grad.dPdx->type->vector_elements, 2);

      for (int i = 0; i < ir->lod_info.grad.dPdy->type->vector_elements; i++) {
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), dPdy);
	 dPdy.reg_offset++;
      }
      mlen += MAX2(ir->lod_info.grad.dPdy->type->vector_elements, 2);
   } else if (ir->op == ir_txs) {
      /* There's no SIMD8 resinfo message on Gen4.  Use SIMD16 instead. */
      simd16 = true;
      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen, BRW_REGISTER_TYPE_UD), lod);
      mlen += 2;
   } else {
      /* Oh joy.  gen4 doesn't have SIMD8 non-shadow-compare bias/lod
       * instructions.  We'll need to do SIMD16 here.
       */
      simd16 = true;
      assert(ir->op == ir_txb || ir->op == ir_txl || ir->op == ir_txf);

      for (int i = 0; i < ir->coordinate->type->vector_elements; i++) {
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen + i * 2, coordinate.type),
	      coordinate);
	 coordinate.reg_offset++;
      }

      /* Initialize the rest of u/v/r with 0.0.  Empirically, this seems to
       * be necessary for TXF (ld), but seems wise to do for all messages.
       */
      for (int i = ir->coordinate->type->vector_elements; i < 3; i++) {
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen + i * 2), fs_reg(0.0f));
      }

      /* lod/bias appears after u/v/r. */
      mlen += 6;

      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen, lod.type), lod);
      mlen++;

      /* The unused upper half. */
      mlen++;
   }

   if (simd16) {
      /* Now, since we're doing simd16, the return is 2 interleaved
       * vec4s where the odd-indexed ones are junk. We'll need to move
       * this weirdness around to the expected layout.
       */
      orig_dst = dst;
      const glsl_type *vec_type =
	 glsl_type::get_instance(ir->type->base_type, 4, 1);
      dst = fs_reg(this, glsl_type::get_array_instance(vec_type, 2));
      dst.type = intel->is_g4x ? brw_type_for_base_type(ir->type)
			       : BRW_REGISTER_TYPE_F;
   }

   fs_inst *inst = NULL;
   switch (ir->op) {
   case ir_tex:
      inst = emit(SHADER_OPCODE_TEX, dst);
      break;
   case ir_txb:
      inst = emit(FS_OPCODE_TXB, dst);
      break;
   case ir_txl:
      inst = emit(SHADER_OPCODE_TXL, dst);
      break;
   case ir_txd:
      inst = emit(SHADER_OPCODE_TXD, dst);
      break;
   case ir_txs:
      inst = emit(SHADER_OPCODE_TXS, dst);
      break;
   case ir_txf:
      inst = emit(SHADER_OPCODE_TXF, dst);
      break;
   }
   inst->base_mrf = base_mrf;
   inst->mlen = mlen;
   inst->header_present = true;

   if (simd16) {
      for (int i = 0; i < 4; i++) {
	 emit(BRW_OPCODE_MOV, orig_dst, dst);
	 orig_dst.reg_offset++;
	 dst.reg_offset += 2;
      }
   }

   return inst;
}

/* gen5's sampler has slots for u, v, r, array index, then optional
 * parameters like shadow comparitor or LOD bias.  If optional
 * parameters aren't present, those base slots are optional and don't
 * need to be included in the message.
 *
 * We don't fill in the unnecessary slots regardless, which may look
 * surprising in the disassembly.
 */
fs_inst *
fs_visitor::emit_texture_gen5(ir_texture *ir, fs_reg dst, fs_reg coordinate,
			      fs_reg shadow_c, fs_reg lod, fs_reg lod2)
{
   int mlen = 0;
   int base_mrf = 2;
   int reg_width = c->dispatch_width / 8;
   bool header_present = false;
   const int vector_elements =
      ir->coordinate ? ir->coordinate->type->vector_elements : 0;

   if (ir->offset != NULL && ir->op == ir_txf) {
      /* It appears that the ld instruction used for txf does its
       * address bounds check before adding in the offset.  To work
       * around this, just add the integer offset to the integer texel
       * coordinate, and don't put the offset in the header.
       */
      ir_constant *offset = ir->offset->as_constant();
      for (int i = 0; i < vector_elements; i++) {
	 emit(BRW_OPCODE_ADD,
	      fs_reg(MRF, base_mrf + mlen + i * reg_width, coordinate.type),
	      coordinate,
	      offset->value.i[i]);
	 coordinate.reg_offset++;
      }
   } else {
      if (ir->offset) {
	 /* The offsets set up by the ir_texture visitor are in the
	  * m1 header, so we can't go headerless.
	  */
	 header_present = true;
	 mlen++;
	 base_mrf--;
      }

      for (int i = 0; i < vector_elements; i++) {
	 emit(BRW_OPCODE_MOV,
	      fs_reg(MRF, base_mrf + mlen + i * reg_width, coordinate.type),
	      coordinate);
	 coordinate.reg_offset++;
      }
   }
   mlen += vector_elements * reg_width;

   if (ir->shadow_comparitor) {
      mlen = MAX2(mlen, header_present + 4 * reg_width);

      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), shadow_c);
      mlen += reg_width;
   }

   fs_inst *inst = NULL;
   switch (ir->op) {
   case ir_tex:
      inst = emit(SHADER_OPCODE_TEX, dst);
      break;
   case ir_txb:
      mlen = MAX2(mlen, header_present + 4 * reg_width);
      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), lod);
      mlen += reg_width;

      inst = emit(FS_OPCODE_TXB, dst);
      break;
   case ir_txl:
      mlen = MAX2(mlen, header_present + 4 * reg_width);
      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), lod);
      mlen += reg_width;

      inst = emit(SHADER_OPCODE_TXL, dst);
      break;
   case ir_txd: {
      mlen = MAX2(mlen, header_present + 4 * reg_width); /* skip over 'ai' */

      /**
       *  P   =  u,    v,    r
       * dPdx = dudx, dvdx, drdx
       * dPdy = dudy, dvdy, drdy
       *
       * Load up these values:
       * - dudx   dudy   dvdx   dvdy   drdx   drdy
       * - dPdx.x dPdy.x dPdx.y dPdy.y dPdx.z dPdy.z
       */
      for (int i = 0; i < ir->lod_info.grad.dPdx->type->vector_elements; i++) {
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), lod);
	 lod.reg_offset++;
	 mlen += reg_width;

	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), lod2);
	 lod2.reg_offset++;
	 mlen += reg_width;
      }

      inst = emit(SHADER_OPCODE_TXD, dst);
      break;
   }
   case ir_txs:
      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen, BRW_REGISTER_TYPE_UD), lod);
      mlen += reg_width;
      inst = emit(SHADER_OPCODE_TXS, dst);
      break;
   case ir_txf:
      mlen = header_present + 4 * reg_width;

      emit(BRW_OPCODE_MOV,
	   fs_reg(MRF, base_mrf + mlen - reg_width, BRW_REGISTER_TYPE_UD),
	   lod);
      inst = emit(SHADER_OPCODE_TXF, dst);
      break;
   }
   inst->base_mrf = base_mrf;
   inst->mlen = mlen;
   inst->header_present = header_present;

   if (mlen > 11) {
      fail("Message length >11 disallowed by hardware\n");
   }

   return inst;
}

fs_inst *
fs_visitor::emit_texture_gen7(ir_texture *ir, fs_reg dst, fs_reg coordinate,
			      fs_reg shadow_c, fs_reg lod, fs_reg lod2)
{
   int mlen = 0;
   int base_mrf = 2;
   int reg_width = c->dispatch_width / 8;
   bool header_present = false;
   int offsets[3];

   if (ir->offset && ir->op != ir_txf) {
      /* The offsets set up by the ir_texture visitor are in the
       * m1 header, so we can't go headerless.
       */
      header_present = true;
      mlen++;
      base_mrf--;
   }

   if (ir->shadow_comparitor) {
      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), shadow_c);
      mlen += reg_width;
   }

   /* Set up the LOD info */
   switch (ir->op) {
   case ir_tex:
      break;
   case ir_txb:
      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), lod);
      mlen += reg_width;
      break;
   case ir_txl:
      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), lod);
      mlen += reg_width;
      break;
   case ir_txd: {
      if (c->dispatch_width == 16)
	 fail("Gen7 does not support sample_d/sample_d_c in SIMD16 mode.");

      /* Load dPdx and the coordinate together:
       * [hdr], [ref], x, dPdx.x, dPdy.x, y, dPdx.y, dPdy.y, z, dPdx.z, dPdy.z
       */
      for (int i = 0; i < ir->coordinate->type->vector_elements; i++) {
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), coordinate);
	 coordinate.reg_offset++;
	 mlen += reg_width;

	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), lod);
	 lod.reg_offset++;
	 mlen += reg_width;

	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), lod2);
	 lod2.reg_offset++;
	 mlen += reg_width;
      }
      break;
   }
   case ir_txs:
      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen, BRW_REGISTER_TYPE_UD), lod);
      mlen += reg_width;
      break;
   case ir_txf:
      /* It appears that the ld instruction used for txf does its
       * address bounds check before adding in the offset.  To work
       * around this, just add the integer offset to the integer texel
       * coordinate, and don't put the offset in the header.
       */
      if (ir->offset) {
	 ir_constant *offset = ir->offset->as_constant();
	 offsets[0] = offset->value.i[0];
	 offsets[1] = offset->value.i[1];
	 offsets[2] = offset->value.i[2];
      } else {
	 memset(offsets, 0, sizeof(offsets));
      }

      /* Unfortunately, the parameters for LD are intermixed: u, lod, v, r. */
      emit(BRW_OPCODE_ADD,
	   fs_reg(MRF, base_mrf + mlen, BRW_REGISTER_TYPE_D), coordinate, offsets[0]);
      coordinate.reg_offset++;
      mlen += reg_width;

      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen, BRW_REGISTER_TYPE_D), lod);
      mlen += reg_width;

      for (int i = 1; i < ir->coordinate->type->vector_elements; i++) {
	 emit(BRW_OPCODE_ADD,
	      fs_reg(MRF, base_mrf + mlen, BRW_REGISTER_TYPE_D), coordinate, offsets[i]);
	 coordinate.reg_offset++;
	 mlen += reg_width;
      }
      break;
   }

   /* Set up the coordinate (except for cases where it was done above) */
   if (ir->op != ir_txd && ir->op != ir_txs && ir->op != ir_txf) {
      for (int i = 0; i < ir->coordinate->type->vector_elements; i++) {
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), coordinate);
	 coordinate.reg_offset++;
	 mlen += reg_width;
      }
   }

   /* Generate the SEND */
   fs_inst *inst = NULL;
   switch (ir->op) {
   case ir_tex: inst = emit(SHADER_OPCODE_TEX, dst); break;
   case ir_txb: inst = emit(FS_OPCODE_TXB, dst); break;
   case ir_txl: inst = emit(SHADER_OPCODE_TXL, dst); break;
   case ir_txd: inst = emit(SHADER_OPCODE_TXD, dst); break;
   case ir_txf: inst = emit(SHADER_OPCODE_TXF, dst); break;
   case ir_txs: inst = emit(SHADER_OPCODE_TXS, dst); break;
   }
   inst->base_mrf = base_mrf;
   inst->mlen = mlen;
   inst->header_present = header_present;

   if (mlen > 11) {
      fail("Message length >11 disallowed by hardware\n");
   }

   return inst;
}

/**
 * Emit code to produce the coordinates for a texture lookup.
 *
 * Returns the fs_reg containing the texture coordinate (as opposed to
 * setting this->result).
 */
fs_reg
fs_visitor::emit_texcoord(ir_texture *ir, int sampler, int texunit)
{
   fs_inst *inst = NULL;

   if (!ir->coordinate)
      return fs_reg(); /* Return the default BAD_FILE register. */

   ir->coordinate->accept(this);
   fs_reg coordinate = this->result;

   bool needs_gl_clamp = true;

   fs_reg scale_x, scale_y;

   /* The 965 requires the EU to do the normalization of GL rectangle
    * texture coordinates.  We use the program parameter state
    * tracking to get the scaling factor.
    */
   if (ir->sampler->type->sampler_dimensionality == GLSL_SAMPLER_DIM_RECT &&
       (intel->gen < 6 ||
	(intel->gen >= 6 && (c->key.tex.gl_clamp_mask[0] & (1 << sampler) ||
			     c->key.tex.gl_clamp_mask[1] & (1 << sampler))))) {
      struct gl_program_parameter_list *params = c->fp->program.Base.Parameters;
      int tokens[STATE_LENGTH] = {
	 STATE_INTERNAL,
	 STATE_TEXRECT_SCALE,
	 texunit,
	 0,
	 0
      };

      if (c->dispatch_width == 16) {
	 fail("rectangle scale uniform setup not supported on 16-wide\n");
	 return fs_reg(this, ir->type);
      }

      scale_x = fs_reg(UNIFORM, c->prog_data.nr_params);
      scale_y = fs_reg(UNIFORM, c->prog_data.nr_params + 1);

      GLuint index = _mesa_add_state_reference(params,
					       (gl_state_index *)tokens);

      this->param_index[c->prog_data.nr_params] = index;
      this->param_offset[c->prog_data.nr_params] = 0;
      c->prog_data.nr_params++;
      this->param_index[c->prog_data.nr_params] = index;
      this->param_offset[c->prog_data.nr_params] = 1;
      c->prog_data.nr_params++;
   }

   /* The 965 requires the EU to do the normalization of GL rectangle
    * texture coordinates.  We use the program parameter state
    * tracking to get the scaling factor.
    */
   if (intel->gen < 6 &&
       ir->sampler->type->sampler_dimensionality == GLSL_SAMPLER_DIM_RECT) {
      fs_reg dst = fs_reg(this, ir->coordinate->type);
      fs_reg src = coordinate;
      coordinate = dst;

      emit(BRW_OPCODE_MUL, dst, src, scale_x);
      dst.reg_offset++;
      src.reg_offset++;
      emit(BRW_OPCODE_MUL, dst, src, scale_y);
   } else if (ir->sampler->type->sampler_dimensionality == GLSL_SAMPLER_DIM_RECT) {
      /* On gen6+, the sampler handles the rectangle coordinates
       * natively, without needing rescaling.  But that means we have
       * to do GL_CLAMP clamping at the [0, width], [0, height] scale,
       * not [0, 1] like the default case below.
       */
      needs_gl_clamp = false;

      for (int i = 0; i < 2; i++) {
	 if (c->key.tex.gl_clamp_mask[i] & (1 << sampler)) {
	    fs_reg chan = coordinate;
	    chan.reg_offset += i;

	    inst = emit(BRW_OPCODE_SEL, chan, chan, brw_imm_f(0.0));
	    inst->conditional_mod = BRW_CONDITIONAL_G;

	    /* Our parameter comes in as 1.0/width or 1.0/height,
	     * because that's what people normally want for doing
	     * texture rectangle handling.  We need width or height
	     * for clamping, but we don't care enough to make a new
	     * parameter type, so just invert back.
	     */
	    fs_reg limit = fs_reg(this, glsl_type::float_type);
	    emit(BRW_OPCODE_MOV, limit, i == 0 ? scale_x : scale_y);
	    emit(SHADER_OPCODE_RCP, limit, limit);

	    inst = emit(BRW_OPCODE_SEL, chan, chan, limit);
	    inst->conditional_mod = BRW_CONDITIONAL_L;
	 }
      }
   }

   if (ir->coordinate && needs_gl_clamp) {
      for (unsigned int i = 0;
	   i < MIN2(ir->coordinate->type->vector_elements, 3); i++) {
	 if (c->key.tex.gl_clamp_mask[i] & (1 << sampler)) {
	    fs_reg chan = coordinate;
	    chan.reg_offset += i;

	    fs_inst *inst = emit(BRW_OPCODE_MOV, chan, chan);
	    inst->saturate = true;
	 }
      }
   }
   return coordinate;
}

void
fs_visitor::visit(ir_texture *ir)
{
   fs_inst *inst = NULL;

   int sampler = _mesa_get_sampler_uniform_value(ir->sampler, prog, &fp->Base);
   int texunit = fp->Base.SamplerUnits[sampler];

   /* Should be lowered by do_lower_texture_projection */
   assert(!ir->projector);

   /* Generate code to compute all the subexpression trees.  This has to be
    * done before loading any values into MRFs for the sampler message since
    * generating these values may involve SEND messages that need the MRFs.
    */
   fs_reg coordinate = emit_texcoord(ir, sampler, texunit);

   fs_reg shadow_comparitor;
   if (ir->shadow_comparitor) {
      ir->shadow_comparitor->accept(this);
      shadow_comparitor = this->result;
   }

   fs_reg lod, lod2;
   switch (ir->op) {
   case ir_tex:
      break;
   case ir_txb:
      ir->lod_info.bias->accept(this);
      lod = this->result;
      break;
   case ir_txd:
      ir->lod_info.grad.dPdx->accept(this);
      lod = this->result;

      ir->lod_info.grad.dPdy->accept(this);
      lod2 = this->result;
      break;
   case ir_txf:
   case ir_txl:
   case ir_txs:
      ir->lod_info.lod->accept(this);
      lod = this->result;
      break;
   };

   /* Writemasking doesn't eliminate channels on SIMD8 texture
    * samples, so don't worry about them.
    */
   fs_reg dst = fs_reg(this, glsl_type::get_instance(ir->type->base_type, 4, 1));

   if (intel->gen >= 7) {
      inst = emit_texture_gen7(ir, dst, coordinate, shadow_comparitor,
                               lod, lod2);
   } else if (intel->gen >= 5) {
      inst = emit_texture_gen5(ir, dst, coordinate, shadow_comparitor,
                               lod, lod2);
   } else {
      inst = emit_texture_gen4(ir, dst, coordinate, shadow_comparitor,
                               lod, lod2);
   }

   /* The header is set up by generate_tex() when necessary. */
   inst->src[0] = reg_undef;

   if (ir->offset != NULL && ir->op != ir_txf)
      inst->texture_offset = brw_texture_offset(ir->offset->as_constant());

   inst->sampler = sampler;

   if (ir->shadow_comparitor)
      inst->shadow_compare = true;

   swizzle_result(ir, dst, sampler);
}

/**
 * Swizzle the result of a texture result.  This is necessary for
 * EXT_texture_swizzle as well as DEPTH_TEXTURE_MODE for shadow comparisons.
 */
void
fs_visitor::swizzle_result(ir_texture *ir, fs_reg orig_val, int sampler)
{
   this->result = orig_val;

   if (ir->op == ir_txs)
      return;

   if (ir->type == glsl_type::float_type) {
      /* Ignore DEPTH_TEXTURE_MODE swizzling. */
      assert(ir->sampler->type->sampler_shadow);
   } else if (c->key.tex.swizzles[sampler] != SWIZZLE_NOOP) {
      fs_reg swizzled_result = fs_reg(this, glsl_type::vec4_type);

      for (int i = 0; i < 4; i++) {
	 int swiz = GET_SWZ(c->key.tex.swizzles[sampler], i);
	 fs_reg l = swizzled_result;
	 l.reg_offset += i;

	 if (swiz == SWIZZLE_ZERO) {
	    emit(BRW_OPCODE_MOV, l, fs_reg(0.0f));
	 } else if (swiz == SWIZZLE_ONE) {
	    emit(BRW_OPCODE_MOV, l, fs_reg(1.0f));
	 } else {
	    fs_reg r = orig_val;
	    r.reg_offset += GET_SWZ(c->key.tex.swizzles[sampler], i);
	    emit(BRW_OPCODE_MOV, l, r);
	 }
      }
      this->result = swizzled_result;
   }
}

void
fs_visitor::visit(ir_swizzle *ir)
{
   ir->val->accept(this);
   fs_reg val = this->result;

   if (ir->type->vector_elements == 1) {
      this->result.reg_offset += ir->mask.x;
      return;
   }

   fs_reg result = fs_reg(this, ir->type);
   this->result = result;

   for (unsigned int i = 0; i < ir->type->vector_elements; i++) {
      fs_reg channel = val;
      int swiz = 0;

      switch (i) {
      case 0:
	 swiz = ir->mask.x;
	 break;
      case 1:
	 swiz = ir->mask.y;
	 break;
      case 2:
	 swiz = ir->mask.z;
	 break;
      case 3:
	 swiz = ir->mask.w;
	 break;
      }

      channel.reg_offset += swiz;
      emit(BRW_OPCODE_MOV, result, channel);
      result.reg_offset++;
   }
}

void
fs_visitor::visit(ir_discard *ir)
{
   assert(ir->condition == NULL); /* FINISHME */

   emit(FS_OPCODE_DISCARD);
}

void
fs_visitor::visit(ir_constant *ir)
{
   /* Set this->result to reg at the bottom of the function because some code
    * paths will cause this visitor to be applied to other fields.  This will
    * cause the value stored in this->result to be modified.
    *
    * Make reg constant so that it doesn't get accidentally modified along the
    * way.  Yes, I actually had this problem. :(
    */
   const fs_reg reg(this, ir->type);
   fs_reg dst_reg = reg;

   if (ir->type->is_array()) {
      const unsigned size = type_size(ir->type->fields.array);

      for (unsigned i = 0; i < ir->type->length; i++) {
	 ir->array_elements[i]->accept(this);
	 fs_reg src_reg = this->result;

	 dst_reg.type = src_reg.type;
	 for (unsigned j = 0; j < size; j++) {
	    emit(BRW_OPCODE_MOV, dst_reg, src_reg);
	    src_reg.reg_offset++;
	    dst_reg.reg_offset++;
	 }
      }
   } else if (ir->type->is_record()) {
      foreach_list(node, &ir->components) {
	 ir_constant *const field = (ir_constant *) node;
	 const unsigned size = type_size(field->type);

	 field->accept(this);
	 fs_reg src_reg = this->result;

	 dst_reg.type = src_reg.type;
	 for (unsigned j = 0; j < size; j++) {
	    emit(BRW_OPCODE_MOV, dst_reg, src_reg);
	    src_reg.reg_offset++;
	    dst_reg.reg_offset++;
	 }
      }
   } else {
      const unsigned size = type_size(ir->type);

      for (unsigned i = 0; i < size; i++) {
	 switch (ir->type->base_type) {
	 case GLSL_TYPE_FLOAT:
	    emit(BRW_OPCODE_MOV, dst_reg, fs_reg(ir->value.f[i]));
	    break;
	 case GLSL_TYPE_UINT:
	    emit(BRW_OPCODE_MOV, dst_reg, fs_reg(ir->value.u[i]));
	    break;
	 case GLSL_TYPE_INT:
	    emit(BRW_OPCODE_MOV, dst_reg, fs_reg(ir->value.i[i]));
	    break;
	 case GLSL_TYPE_BOOL:
	    emit(BRW_OPCODE_MOV, dst_reg, fs_reg((int)ir->value.b[i]));
	    break;
	 default:
	    assert(!"Non-float/uint/int/bool constant");
	 }
	 dst_reg.reg_offset++;
      }
   }

   this->result = reg;
}

void
fs_visitor::emit_bool_to_cond_code(ir_rvalue *ir)
{
   ir_expression *expr = ir->as_expression();

   if (expr) {
      fs_reg op[2];
      fs_inst *inst;

      assert(expr->get_num_operands() <= 2);
      for (unsigned int i = 0; i < expr->get_num_operands(); i++) {
	 assert(expr->operands[i]->type->is_scalar());

	 expr->operands[i]->accept(this);
	 op[i] = this->result;

	 resolve_ud_negate(&op[i]);
      }

      switch (expr->operation) {
      case ir_unop_logic_not:
	 inst = emit(BRW_OPCODE_AND, reg_null_d, op[0], fs_reg(1));
	 inst->conditional_mod = BRW_CONDITIONAL_Z;
	 break;

      case ir_binop_logic_xor:
      case ir_binop_logic_or:
      case ir_binop_logic_and:
	 goto out;

      case ir_unop_f2b:
	 if (intel->gen >= 6) {
	    inst = emit(BRW_OPCODE_CMP, reg_null_d, op[0], fs_reg(0.0f));
	 } else {
	    inst = emit(BRW_OPCODE_MOV, reg_null_f, op[0]);
	 }
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_unop_i2b:
	 if (intel->gen >= 6) {
	    inst = emit(BRW_OPCODE_CMP, reg_null_d, op[0], fs_reg(0));
	 } else {
	    inst = emit(BRW_OPCODE_MOV, reg_null_d, op[0]);
	 }
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_binop_greater:
      case ir_binop_gequal:
      case ir_binop_less:
      case ir_binop_lequal:
      case ir_binop_equal:
      case ir_binop_all_equal:
      case ir_binop_nequal:
      case ir_binop_any_nequal:
	 resolve_bool_comparison(expr->operands[0], &op[0]);
	 resolve_bool_comparison(expr->operands[1], &op[1]);

	 inst = emit(BRW_OPCODE_CMP, reg_null_cmp, op[0], op[1]);
	 inst->conditional_mod =
	    brw_conditional_for_comparison(expr->operation);
	 break;

      default:
	 assert(!"not reached");
	 fail("bad cond code\n");
	 break;
      }
      return;
   }

out:
   ir->accept(this);

   fs_inst *inst = emit(BRW_OPCODE_AND, reg_null_d, this->result, fs_reg(1));
   inst->conditional_mod = BRW_CONDITIONAL_NZ;
}

/**
 * Emit a gen6 IF statement with the comparison folded into the IF
 * instruction.
 */
void
fs_visitor::emit_if_gen6(ir_if *ir)
{
   ir_expression *expr = ir->condition->as_expression();

   if (expr) {
      fs_reg op[2];
      fs_inst *inst;
      fs_reg temp;

      assert(expr->get_num_operands() <= 2);
      for (unsigned int i = 0; i < expr->get_num_operands(); i++) {
	 assert(expr->operands[i]->type->is_scalar());

	 expr->operands[i]->accept(this);
	 op[i] = this->result;
      }

      switch (expr->operation) {
      case ir_unop_logic_not:
      case ir_binop_logic_xor:
      case ir_binop_logic_or:
      case ir_binop_logic_and:
         /* For operations on bool arguments, only the low bit of the bool is
          * valid, and the others are undefined.  Fall back to the condition
          * code path.
          */
         break;

      case ir_unop_f2b:
	 inst = emit(BRW_OPCODE_IF, reg_null_f, op[0], fs_reg(0));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 return;

      case ir_unop_i2b:
	 inst = emit(BRW_OPCODE_IF, reg_null_d, op[0], fs_reg(0));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 return;

      case ir_binop_greater:
      case ir_binop_gequal:
      case ir_binop_less:
      case ir_binop_lequal:
      case ir_binop_equal:
      case ir_binop_all_equal:
      case ir_binop_nequal:
      case ir_binop_any_nequal:
	 resolve_bool_comparison(expr->operands[0], &op[0]);
	 resolve_bool_comparison(expr->operands[1], &op[1]);

	 inst = emit(BRW_OPCODE_IF, reg_null_d, op[0], op[1]);
	 inst->conditional_mod =
	    brw_conditional_for_comparison(expr->operation);
	 return;
      default:
	 assert(!"not reached");
	 inst = emit(BRW_OPCODE_IF, reg_null_d, op[0], fs_reg(0));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 fail("bad condition\n");
	 return;
      }
   }

   emit_bool_to_cond_code(ir->condition);
   fs_inst *inst = emit(BRW_OPCODE_IF);
   inst->predicated = true;
}

void
fs_visitor::visit(ir_if *ir)
{
   fs_inst *inst;

   if (intel->gen < 6 && c->dispatch_width == 16) {
      fail("Can't support (non-uniform) control flow on 16-wide\n");
   }

   /* Don't point the annotation at the if statement, because then it plus
    * the then and else blocks get printed.
    */
   this->base_ir = ir->condition;

   if (intel->gen == 6) {
      emit_if_gen6(ir);
   } else {
      emit_bool_to_cond_code(ir->condition);

      inst = emit(BRW_OPCODE_IF);
      inst->predicated = true;
   }

   foreach_list(node, &ir->then_instructions) {
      ir_instruction *ir = (ir_instruction *)node;
      this->base_ir = ir;

      ir->accept(this);
   }

   if (!ir->else_instructions.is_empty()) {
      emit(BRW_OPCODE_ELSE);

      foreach_list(node, &ir->else_instructions) {
	 ir_instruction *ir = (ir_instruction *)node;
	 this->base_ir = ir;

	 ir->accept(this);
      }
   }

   emit(BRW_OPCODE_ENDIF);
}

void
fs_visitor::visit(ir_loop *ir)
{
   fs_reg counter = reg_undef;

   if (intel->gen < 6 && c->dispatch_width == 16) {
      fail("Can't support (non-uniform) control flow on 16-wide\n");
   }

   if (ir->counter) {
      this->base_ir = ir->counter;
      ir->counter->accept(this);
      counter = *(variable_storage(ir->counter));

      if (ir->from) {
	 this->base_ir = ir->from;
	 ir->from->accept(this);

	 emit(BRW_OPCODE_MOV, counter, this->result);
      }
   }

   this->base_ir = NULL;
   emit(BRW_OPCODE_DO);

   if (ir->to) {
      this->base_ir = ir->to;
      ir->to->accept(this);

      fs_inst *inst = emit(BRW_OPCODE_CMP, reg_null_cmp, counter, this->result);
      inst->conditional_mod = brw_conditional_for_comparison(ir->cmp);

      inst = emit(BRW_OPCODE_BREAK);
      inst->predicated = true;
   }

   foreach_list(node, &ir->body_instructions) {
      ir_instruction *ir = (ir_instruction *)node;

      this->base_ir = ir;
      ir->accept(this);
   }

   if (ir->increment) {
      this->base_ir = ir->increment;
      ir->increment->accept(this);
      emit(BRW_OPCODE_ADD, counter, counter, this->result);
   }

   this->base_ir = NULL;
   emit(BRW_OPCODE_WHILE);
}

void
fs_visitor::visit(ir_loop_jump *ir)
{
   switch (ir->mode) {
   case ir_loop_jump::jump_break:
      emit(BRW_OPCODE_BREAK);
      break;
   case ir_loop_jump::jump_continue:
      emit(BRW_OPCODE_CONTINUE);
      break;
   }
}

void
fs_visitor::visit(ir_call *ir)
{
   assert(!"FINISHME");
}

void
fs_visitor::visit(ir_return *ir)
{
   assert(!"FINISHME");
}

void
fs_visitor::visit(ir_function *ir)
{
   /* Ignore function bodies other than main() -- we shouldn't see calls to
    * them since they should all be inlined before we get to ir_to_mesa.
    */
   if (strcmp(ir->name, "main") == 0) {
      const ir_function_signature *sig;
      exec_list empty;

      sig = ir->matching_signature(&empty);

      assert(sig);

      foreach_list(node, &sig->body) {
	 ir_instruction *ir = (ir_instruction *)node;
	 this->base_ir = ir;

	 ir->accept(this);
      }
   }
}

void
fs_visitor::visit(ir_function_signature *ir)
{
   assert(!"not reached");
   (void)ir;
}

fs_inst *
fs_visitor::emit(fs_inst inst)
{
   fs_inst *list_inst = new(mem_ctx) fs_inst;
   *list_inst = inst;

   if (force_uncompressed_stack > 0)
      list_inst->force_uncompressed = true;
   else if (force_sechalf_stack > 0)
      list_inst->force_sechalf = true;

   list_inst->annotation = this->current_annotation;
   list_inst->ir = this->base_ir;

   this->instructions.push_tail(list_inst);

   return list_inst;
}

/** Emits a dummy fragment shader consisting of magenta for bringup purposes. */
void
fs_visitor::emit_dummy_fs()
{
   int reg_width = c->dispatch_width / 8;

   /* Everyone's favorite color. */
   emit(BRW_OPCODE_MOV, fs_reg(MRF, 2 + 0 * reg_width), fs_reg(1.0f));
   emit(BRW_OPCODE_MOV, fs_reg(MRF, 2 + 1 * reg_width), fs_reg(0.0f));
   emit(BRW_OPCODE_MOV, fs_reg(MRF, 2 + 2 * reg_width), fs_reg(1.0f));
   emit(BRW_OPCODE_MOV, fs_reg(MRF, 2 + 3 * reg_width), fs_reg(0.0f));

   fs_inst *write;
   write = emit(FS_OPCODE_FB_WRITE, fs_reg(0), fs_reg(0));
   write->base_mrf = 2;
   write->mlen = 4 * reg_width;
   write->eot = true;
}

/* The register location here is relative to the start of the URB
 * data.  It will get adjusted to be a real location before
 * generate_code() time.
 */
struct brw_reg
fs_visitor::interp_reg(int location, int channel)
{
   int regnr = urb_setup[location] * 2 + channel / 2;
   int stride = (channel & 1) * 4;

   assert(urb_setup[location] != -1);

   return brw_vec1_grf(regnr, stride);
}

/** Emits the interpolation for the varying inputs. */
void
fs_visitor::emit_interpolation_setup_gen4()
{
   this->current_annotation = "compute pixel centers";
   this->pixel_x = fs_reg(this, glsl_type::uint_type);
   this->pixel_y = fs_reg(this, glsl_type::uint_type);
   this->pixel_x.type = BRW_REGISTER_TYPE_UW;
   this->pixel_y.type = BRW_REGISTER_TYPE_UW;

   emit(FS_OPCODE_PIXEL_X, this->pixel_x);
   emit(FS_OPCODE_PIXEL_Y, this->pixel_y);

   this->current_annotation = "compute pixel deltas from v0";
   if (brw->has_pln) {
      this->delta_x[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC] =
         fs_reg(this, glsl_type::vec2_type);
      this->delta_y[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC] =
         this->delta_x[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC];
      this->delta_y[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC].reg_offset++;
   } else {
      this->delta_x[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC] =
         fs_reg(this, glsl_type::float_type);
      this->delta_y[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC] =
         fs_reg(this, glsl_type::float_type);
   }
   emit(BRW_OPCODE_ADD, this->delta_x[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC],
	this->pixel_x, fs_reg(negate(brw_vec1_grf(1, 0))));
   emit(BRW_OPCODE_ADD, this->delta_y[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC],
	this->pixel_y, fs_reg(negate(brw_vec1_grf(1, 1))));

   this->current_annotation = "compute pos.w and 1/pos.w";
   /* Compute wpos.w.  It's always in our setup, since it's needed to
    * interpolate the other attributes.
    */
   this->wpos_w = fs_reg(this, glsl_type::float_type);
   emit(FS_OPCODE_LINTERP, wpos_w,
        this->delta_x[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC],
        this->delta_y[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC],
	interp_reg(FRAG_ATTRIB_WPOS, 3));
   /* Compute the pixel 1/W value from wpos.w. */
   this->pixel_w = fs_reg(this, glsl_type::float_type);
   emit_math(SHADER_OPCODE_RCP, this->pixel_w, wpos_w);
   this->current_annotation = NULL;
}

/** Emits the interpolation for the varying inputs. */
void
fs_visitor::emit_interpolation_setup_gen6()
{
   struct brw_reg g1_uw = retype(brw_vec1_grf(1, 0), BRW_REGISTER_TYPE_UW);

   /* If the pixel centers end up used, the setup is the same as for gen4. */
   this->current_annotation = "compute pixel centers";
   fs_reg int_pixel_x = fs_reg(this, glsl_type::uint_type);
   fs_reg int_pixel_y = fs_reg(this, glsl_type::uint_type);
   int_pixel_x.type = BRW_REGISTER_TYPE_UW;
   int_pixel_y.type = BRW_REGISTER_TYPE_UW;
   emit(BRW_OPCODE_ADD,
	int_pixel_x,
	fs_reg(stride(suboffset(g1_uw, 4), 2, 4, 0)),
	fs_reg(brw_imm_v(0x10101010)));
   emit(BRW_OPCODE_ADD,
	int_pixel_y,
	fs_reg(stride(suboffset(g1_uw, 5), 2, 4, 0)),
	fs_reg(brw_imm_v(0x11001100)));

   /* As of gen6, we can no longer mix float and int sources.  We have
    * to turn the integer pixel centers into floats for their actual
    * use.
    */
   this->pixel_x = fs_reg(this, glsl_type::float_type);
   this->pixel_y = fs_reg(this, glsl_type::float_type);
   emit(BRW_OPCODE_MOV, this->pixel_x, int_pixel_x);
   emit(BRW_OPCODE_MOV, this->pixel_y, int_pixel_y);

   this->current_annotation = "compute pos.w";
   this->pixel_w = fs_reg(brw_vec8_grf(c->source_w_reg, 0));
   this->wpos_w = fs_reg(this, glsl_type::float_type);
   emit_math(SHADER_OPCODE_RCP, this->wpos_w, this->pixel_w);

   for (int i = 0; i < BRW_WM_BARYCENTRIC_INTERP_MODE_COUNT; ++i) {
      uint8_t reg = c->barycentric_coord_reg[i];
      this->delta_x[i] = fs_reg(brw_vec8_grf(reg, 0));
      this->delta_y[i] = fs_reg(brw_vec8_grf(reg + 1, 0));
   }

   this->current_annotation = NULL;
}

void
fs_visitor::emit_color_write(int target, int index, int first_color_mrf)
{
   int reg_width = c->dispatch_width / 8;
   fs_inst *inst;
   fs_reg color = outputs[target];
   fs_reg mrf;

   /* If there's no color data to be written, skip it. */
   if (color.file == BAD_FILE)
      return;

   color.reg_offset += index;

   if (c->dispatch_width == 8 || intel->gen >= 6) {
      /* SIMD8 write looks like:
       * m + 0: r0
       * m + 1: r1
       * m + 2: g0
       * m + 3: g1
       *
       * gen6 SIMD16 DP write looks like:
       * m + 0: r0
       * m + 1: r1
       * m + 2: g0
       * m + 3: g1
       * m + 4: b0
       * m + 5: b1
       * m + 6: a0
       * m + 7: a1
       */
      inst = emit(BRW_OPCODE_MOV,
		  fs_reg(MRF, first_color_mrf + index * reg_width, color.type),
		  color);
      inst->saturate = c->key.clamp_fragment_color;
   } else {
      /* pre-gen6 SIMD16 single source DP write looks like:
       * m + 0: r0
       * m + 1: g0
       * m + 2: b0
       * m + 3: a0
       * m + 4: r1
       * m + 5: g1
       * m + 6: b1
       * m + 7: a1
       */
      if (brw->has_compr4) {
	 /* By setting the high bit of the MRF register number, we
	  * indicate that we want COMPR4 mode - instead of doing the
	  * usual destination + 1 for the second half we get
	  * destination + 4.
	  */
	 inst = emit(BRW_OPCODE_MOV,
		     fs_reg(MRF, BRW_MRF_COMPR4 + first_color_mrf + index,
			    color.type),
		     color);
	 inst->saturate = c->key.clamp_fragment_color;
      } else {
	 push_force_uncompressed();
	 inst = emit(BRW_OPCODE_MOV, fs_reg(MRF, first_color_mrf + index,
					    color.type),
		     color);
	 inst->saturate = c->key.clamp_fragment_color;
	 pop_force_uncompressed();

	 push_force_sechalf();
	 color.sechalf = true;
	 inst = emit(BRW_OPCODE_MOV, fs_reg(MRF, first_color_mrf + index + 4,
					    color.type),
		     color);
	 inst->saturate = c->key.clamp_fragment_color;
	 pop_force_sechalf();
	 color.sechalf = false;
      }
   }
}

void
fs_visitor::emit_fb_writes()
{
   this->current_annotation = "FB write header";
   bool header_present = true;
   /* We can potentially have a message length of up to 15, so we have to set
    * base_mrf to either 0 or 1 in order to fit in m0..m15.
    */
   int base_mrf = 1;
   int nr = base_mrf;
   int reg_width = c->dispatch_width / 8;
   bool do_dual_src = this->dual_src_output.file != BAD_FILE;
   bool src0_alpha_to_render_target = false;

   if (c->dispatch_width == 16 && do_dual_src) {
      fail("GL_ARB_blend_func_extended not yet supported in 16-wide.");
      do_dual_src = false;
   }

   /* From the Sandy Bridge PRM, volume 4, page 198:
    *
    *     "Dispatched Pixel Enables. One bit per pixel indicating
    *      which pixels were originally enabled when the thread was
    *      dispatched. This field is only required for the end-of-
    *      thread message and on all dual-source messages."
    */
   if (intel->gen >= 6 &&
       !this->fp->UsesKill &&
       !do_dual_src &&
       c->key.nr_color_regions == 1) {
      header_present = false;
   }

   if (header_present) {
      src0_alpha_to_render_target = intel->gen >= 6 &&
				    !do_dual_src &&
				    c->key.nr_color_regions > 1 &&
				    c->key.sample_alpha_to_coverage;
      /* m2, m3 header */
      nr += 2;
   }

   if (c->aa_dest_stencil_reg) {
      push_force_uncompressed();
      emit(BRW_OPCODE_MOV, fs_reg(MRF, nr++),
	   fs_reg(brw_vec8_grf(c->aa_dest_stencil_reg, 0)));
      pop_force_uncompressed();
   }

   /* Reserve space for color. It'll be filled in per MRT below. */
   int color_mrf = nr;
   nr += 4 * reg_width;
   if (do_dual_src)
      nr += 4;
   if (src0_alpha_to_render_target)
      nr += reg_width;

   if (c->source_depth_to_render_target) {
      if (intel->gen == 6 && c->dispatch_width == 16) {
	 /* For outputting oDepth on gen6, SIMD8 writes have to be
	  * used.  This would require 8-wide moves of each half to
	  * message regs, kind of like pre-gen5 SIMD16 FB writes.
	  * Just bail on doing so for now.
	  */
	 fail("Missing support for simd16 depth writes on gen6\n");
      }

      if (c->computes_depth) {
	 /* Hand over gl_FragDepth. */
	 assert(this->frag_depth);
	 fs_reg depth = *(variable_storage(this->frag_depth));

	 emit(BRW_OPCODE_MOV, fs_reg(MRF, nr), depth);
      } else {
	 /* Pass through the payload depth. */
	 emit(BRW_OPCODE_MOV, fs_reg(MRF, nr),
	      fs_reg(brw_vec8_grf(c->source_depth_reg, 0)));
      }
      nr += reg_width;
   }

   if (c->dest_depth_reg) {
      emit(BRW_OPCODE_MOV, fs_reg(MRF, nr),
	   fs_reg(brw_vec8_grf(c->dest_depth_reg, 0)));
      nr += reg_width;
   }

   if (do_dual_src) {
      fs_reg src0 = this->outputs[0];
      fs_reg src1 = this->dual_src_output;

      this->current_annotation = ralloc_asprintf(this->mem_ctx,
						 "FB write src0");
      for (int i = 0; i < 4; i++) {
	 fs_inst *inst = emit(BRW_OPCODE_MOV,
			      fs_reg(MRF, color_mrf + i, src0.type),
			      src0);
	 src0.reg_offset++;
	 inst->saturate = c->key.clamp_fragment_color;
      }

      this->current_annotation = ralloc_asprintf(this->mem_ctx,
						 "FB write src1");
      for (int i = 0; i < 4; i++) {
	 fs_inst *inst = emit(BRW_OPCODE_MOV,
			      fs_reg(MRF, color_mrf + 4 + i, src1.type),
			      src1);
	 src1.reg_offset++;
	 inst->saturate = c->key.clamp_fragment_color;
      }

      fs_inst *inst = emit(FS_OPCODE_FB_WRITE);
      inst->target = 0;
      inst->base_mrf = base_mrf;
      inst->mlen = nr - base_mrf;
      inst->eot = true;
      inst->header_present = header_present;

      c->prog_data.dual_src_blend = true;
      this->current_annotation = NULL;
      return;
   }

   for (int target = 0; target < c->key.nr_color_regions; target++) {
      this->current_annotation = ralloc_asprintf(this->mem_ctx,
						 "FB write target %d",
						 target);
      /* If src0_alpha_to_render_target is true, include source zero alpha
       * data in RenderTargetWrite message for targets > 0.
       */
      int write_color_mrf = color_mrf;
      if (src0_alpha_to_render_target && target != 0) {
         fs_inst *inst;
         fs_reg color = outputs[0];
         color.reg_offset += 3;

         inst = emit(BRW_OPCODE_MOV,
		     fs_reg(MRF, write_color_mrf, color.type),
		     color);
         inst->saturate = c->key.clamp_fragment_color;
         write_color_mrf = color_mrf + reg_width;
      }

      for (unsigned i = 0; i < this->output_components[target]; i++)
         emit_color_write(target, i, write_color_mrf);

      fs_inst *inst = emit(FS_OPCODE_FB_WRITE);
      inst->target = target;
      inst->base_mrf = base_mrf;
      if (src0_alpha_to_render_target && target == 0)
         inst->mlen = nr - base_mrf - reg_width;
      else
         inst->mlen = nr - base_mrf;
      if (target == c->key.nr_color_regions - 1)
	 inst->eot = true;
      inst->header_present = header_present;
   }

   if (c->key.nr_color_regions == 0) {
      /* Even if there's no color buffers enabled, we still need to send
       * alpha out the pipeline to our null renderbuffer to support
       * alpha-testing, alpha-to-coverage, and so on.
       */
      emit_color_write(0, 3, color_mrf);

      fs_inst *inst = emit(FS_OPCODE_FB_WRITE);
      inst->base_mrf = base_mrf;
      inst->mlen = nr - base_mrf;
      inst->eot = true;
      inst->header_present = header_present;
   }

   this->current_annotation = NULL;
}

void
fs_visitor::resolve_ud_negate(fs_reg *reg)
{
   if (reg->type != BRW_REGISTER_TYPE_UD ||
       !reg->negate)
      return;

   fs_reg temp = fs_reg(this, glsl_type::uint_type);
   emit(BRW_OPCODE_MOV, temp, *reg);
   *reg = temp;
}

void
fs_visitor::resolve_bool_comparison(ir_rvalue *rvalue, fs_reg *reg)
{
   if (rvalue->type != glsl_type::bool_type)
      return;

   fs_reg temp = fs_reg(this, glsl_type::bool_type);
   emit(BRW_OPCODE_AND, temp, *reg, fs_reg(1));
   *reg = temp;
}

fs_visitor::fs_visitor(struct brw_wm_compile *c, struct gl_shader_program *prog,
                       struct brw_shader *shader)
{
   this->c = c;
   this->p = &c->func;
   this->brw = p->brw;
   this->fp = (struct gl_fragment_program *)
      prog->_LinkedShaders[MESA_SHADER_FRAGMENT]->Program;
   this->prog = prog;
   this->intel = &brw->intel;
   this->ctx = &intel->ctx;
   this->mem_ctx = ralloc_context(NULL);
   this->shader = shader;
   this->failed = false;
   this->variable_ht = hash_table_ctor(0,
                                       hash_table_pointer_hash,
                                       hash_table_pointer_compare);

   /* There's a question that appears to be left open in the spec:
    * How do implicit dst conversions interact with the CMP
    * instruction or conditional mods?  On gen6, the instruction:
    *
    * CMP null<d> src0<f> src1<f>
    *
    * will do src1 - src0 and compare that result as if it was an
    * integer.  On gen4, it will do src1 - src0 as float, convert
    * the result to int, and compare as int.  In between, it
    * appears that it does src1 - src0 and does the compare in the
    * execution type so dst type doesn't matter.
    */
   if (this->intel->gen > 4)
      this->reg_null_cmp = reg_null_d;
   else
      this->reg_null_cmp = reg_null_f;

   this->frag_depth = NULL;
   memset(this->outputs, 0, sizeof(this->outputs));
   memset(this->output_components, 0, sizeof(this->output_components));
   this->first_non_payload_grf = 0;
   this->max_grf = intel->gen >= 7 ? GEN7_MRF_HACK_START : BRW_MAX_GRF;

   this->current_annotation = NULL;
   this->base_ir = NULL;

   this->virtual_grf_sizes = NULL;
   this->virtual_grf_count = 0;
   this->virtual_grf_array_size = 0;
   this->virtual_grf_def = NULL;
   this->virtual_grf_use = NULL;
   this->live_intervals_valid = false;

   this->force_uncompressed_stack = 0;
   this->force_sechalf_stack = 0;
}

fs_visitor::~fs_visitor()
{
   ralloc_free(this->mem_ctx);
   hash_table_dtor(this->variable_ht);
}
