/*
 * Copyright © 2011 Intel Corporation
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

#include "brw_vec4.h"
extern "C" {
#include "main/macros.h"
#include "program/prog_parameter.h"
#include "program/sampler.h"
}

namespace brw {

vec4_instruction::vec4_instruction(vec4_visitor *v,
				   enum opcode opcode, dst_reg dst,
				   src_reg src0, src_reg src1, src_reg src2)
{
   this->opcode = opcode;
   this->dst = dst;
   this->src[0] = src0;
   this->src[1] = src1;
   this->src[2] = src2;
   this->ir = v->base_ir;
   this->annotation = v->current_annotation;
}

vec4_instruction *
vec4_visitor::emit(vec4_instruction *inst)
{
   this->instructions.push_tail(inst);

   return inst;
}

vec4_instruction *
vec4_visitor::emit_before(vec4_instruction *inst, vec4_instruction *new_inst)
{
   new_inst->ir = inst->ir;
   new_inst->annotation = inst->annotation;

   inst->insert_before(new_inst);

   return inst;
}

vec4_instruction *
vec4_visitor::emit(enum opcode opcode, dst_reg dst,
		   src_reg src0, src_reg src1, src_reg src2)
{
   return emit(new(mem_ctx) vec4_instruction(this, opcode, dst,
					     src0, src1, src2));
}


vec4_instruction *
vec4_visitor::emit(enum opcode opcode, dst_reg dst, src_reg src0, src_reg src1)
{
   return emit(new(mem_ctx) vec4_instruction(this, opcode, dst, src0, src1));
}

vec4_instruction *
vec4_visitor::emit(enum opcode opcode, dst_reg dst, src_reg src0)
{
   return emit(new(mem_ctx) vec4_instruction(this, opcode, dst, src0));
}

vec4_instruction *
vec4_visitor::emit(enum opcode opcode)
{
   return emit(new(mem_ctx) vec4_instruction(this, opcode, dst_reg()));
}

#define ALU1(op)							\
   vec4_instruction *							\
   vec4_visitor::op(dst_reg dst, src_reg src0)				\
   {									\
      return new(mem_ctx) vec4_instruction(this, BRW_OPCODE_##op, dst,	\
					   src0);			\
   }

#define ALU2(op)							\
   vec4_instruction *							\
   vec4_visitor::op(dst_reg dst, src_reg src0, src_reg src1)		\
   {									\
      return new(mem_ctx) vec4_instruction(this, BRW_OPCODE_##op, dst,	\
					   src0, src1);			\
   }

ALU1(NOT)
ALU1(MOV)
ALU1(FRC)
ALU1(RNDD)
ALU1(RNDE)
ALU1(RNDZ)
ALU2(ADD)
ALU2(MUL)
ALU2(MACH)
ALU2(AND)
ALU2(OR)
ALU2(XOR)
ALU2(DP3)
ALU2(DP4)

/** Gen4 predicated IF. */
vec4_instruction *
vec4_visitor::IF(uint32_t predicate)
{
   vec4_instruction *inst;

   inst = new(mem_ctx) vec4_instruction(this, BRW_OPCODE_IF);
   inst->predicate = predicate;

   return inst;
}

/** Gen6+ IF with embedded comparison. */
vec4_instruction *
vec4_visitor::IF(src_reg src0, src_reg src1, uint32_t condition)
{
   assert(intel->gen >= 6);

   vec4_instruction *inst;

   resolve_ud_negate(&src0);
   resolve_ud_negate(&src1);

   inst = new(mem_ctx) vec4_instruction(this, BRW_OPCODE_IF, dst_null_d(),
					src0, src1);
   inst->conditional_mod = condition;

   return inst;
}

/**
 * CMP: Sets the low bit of the destination channels with the result
 * of the comparison, while the upper bits are undefined, and updates
 * the flag register with the packed 16 bits of the result.
 */
vec4_instruction *
vec4_visitor::CMP(dst_reg dst, src_reg src0, src_reg src1, uint32_t condition)
{
   vec4_instruction *inst;

   /* original gen4 does type conversion to the destination type
    * before before comparison, producing garbage results for floating
    * point comparisons.
    */
   if (intel->gen == 4) {
      dst.type = src0.type;
      if (dst.file == HW_REG)
	 dst.fixed_hw_reg.type = dst.type;
   }

   resolve_ud_negate(&src0);
   resolve_ud_negate(&src1);

   inst = new(mem_ctx) vec4_instruction(this, BRW_OPCODE_CMP, dst, src0, src1);
   inst->conditional_mod = condition;

   return inst;
}

vec4_instruction *
vec4_visitor::SCRATCH_READ(dst_reg dst, src_reg index)
{
   vec4_instruction *inst;

   inst = new(mem_ctx) vec4_instruction(this, VS_OPCODE_SCRATCH_READ,
					dst, index);
   inst->base_mrf = 14;
   inst->mlen = 1;

   return inst;
}

vec4_instruction *
vec4_visitor::SCRATCH_WRITE(dst_reg dst, src_reg src, src_reg index)
{
   vec4_instruction *inst;

   inst = new(mem_ctx) vec4_instruction(this, VS_OPCODE_SCRATCH_WRITE,
					dst, src, index);
   inst->base_mrf = 13;
   inst->mlen = 2;

   return inst;
}

void
vec4_visitor::emit_dp(dst_reg dst, src_reg src0, src_reg src1, unsigned elements)
{
   static enum opcode dot_opcodes[] = {
      BRW_OPCODE_DP2, BRW_OPCODE_DP3, BRW_OPCODE_DP4
   };

   emit(dot_opcodes[elements - 2], dst, src0, src1);
}

void
vec4_visitor::emit_math1_gen6(enum opcode opcode, dst_reg dst, src_reg src)
{
   /* The gen6 math instruction ignores the source modifiers --
    * swizzle, abs, negate, and at least some parts of the register
    * region description.
    *
    * While it would seem that this MOV could be avoided at this point
    * in the case that the swizzle is matched up with the destination
    * writemask, note that uniform packing and register allocation
    * could rearrange our swizzle, so let's leave this matter up to
    * copy propagation later.
    */
   src_reg temp_src = src_reg(this, glsl_type::vec4_type);
   emit(MOV(dst_reg(temp_src), src));

   if (dst.writemask != WRITEMASK_XYZW) {
      /* The gen6 math instruction must be align1, so we can't do
       * writemasks.
       */
      dst_reg temp_dst = dst_reg(this, glsl_type::vec4_type);

      emit(opcode, temp_dst, temp_src);

      emit(MOV(dst, src_reg(temp_dst)));
   } else {
      emit(opcode, dst, temp_src);
   }
}

void
vec4_visitor::emit_math1_gen4(enum opcode opcode, dst_reg dst, src_reg src)
{
   vec4_instruction *inst = emit(opcode, dst, src);
   inst->base_mrf = 1;
   inst->mlen = 1;
}

void
vec4_visitor::emit_math(opcode opcode, dst_reg dst, src_reg src)
{
   switch (opcode) {
   case SHADER_OPCODE_RCP:
   case SHADER_OPCODE_RSQ:
   case SHADER_OPCODE_SQRT:
   case SHADER_OPCODE_EXP2:
   case SHADER_OPCODE_LOG2:
   case SHADER_OPCODE_SIN:
   case SHADER_OPCODE_COS:
      break;
   default:
      assert(!"not reached: bad math opcode");
      return;
   }

   if (intel->gen >= 7) {
      emit(opcode, dst, src);
   } else if (intel->gen == 6) {
      return emit_math1_gen6(opcode, dst, src);
   } else {
      return emit_math1_gen4(opcode, dst, src);
   }
}

void
vec4_visitor::emit_math2_gen6(enum opcode opcode,
			      dst_reg dst, src_reg src0, src_reg src1)
{
   src_reg expanded;

   /* The gen6 math instruction ignores the source modifiers --
    * swizzle, abs, negate, and at least some parts of the register
    * region description.  Move the sources to temporaries to make it
    * generally work.
    */

   expanded = src_reg(this, glsl_type::vec4_type);
   expanded.type = src0.type;
   emit(MOV(dst_reg(expanded), src0));
   src0 = expanded;

   expanded = src_reg(this, glsl_type::vec4_type);
   expanded.type = src1.type;
   emit(MOV(dst_reg(expanded), src1));
   src1 = expanded;

   if (dst.writemask != WRITEMASK_XYZW) {
      /* The gen6 math instruction must be align1, so we can't do
       * writemasks.
       */
      dst_reg temp_dst = dst_reg(this, glsl_type::vec4_type);
      temp_dst.type = dst.type;

      emit(opcode, temp_dst, src0, src1);

      emit(MOV(dst, src_reg(temp_dst)));
   } else {
      emit(opcode, dst, src0, src1);
   }
}

void
vec4_visitor::emit_math2_gen4(enum opcode opcode,
			      dst_reg dst, src_reg src0, src_reg src1)
{
   vec4_instruction *inst = emit(opcode, dst, src0, src1);
   inst->base_mrf = 1;
   inst->mlen = 2;
}

void
vec4_visitor::emit_math(enum opcode opcode,
			dst_reg dst, src_reg src0, src_reg src1)
{
   switch (opcode) {
   case SHADER_OPCODE_POW:
   case SHADER_OPCODE_INT_QUOTIENT:
   case SHADER_OPCODE_INT_REMAINDER:
      break;
   default:
      assert(!"not reached: unsupported binary math opcode");
      return;
   }

   if (intel->gen >= 7) {
      emit(opcode, dst, src0, src1);
   } else if (intel->gen == 6) {
      return emit_math2_gen6(opcode, dst, src0, src1);
   } else {
      return emit_math2_gen4(opcode, dst, src0, src1);
   }
}

void
vec4_visitor::visit_instructions(const exec_list *list)
{
   foreach_list(node, list) {
      ir_instruction *ir = (ir_instruction *)node;

      base_ir = ir;
      ir->accept(this);
   }
}


static int
type_size(const struct glsl_type *type)
{
   unsigned int i;
   int size;

   switch (type->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_BOOL:
      if (type->is_matrix()) {
	 return type->matrix_columns;
      } else {
	 /* Regardless of size of vector, it gets a vec4. This is bad
	  * packing for things like floats, but otherwise arrays become a
	  * mess.  Hopefully a later pass over the code can pack scalars
	  * down if appropriate.
	  */
	 return 1;
      }
   case GLSL_TYPE_ARRAY:
      assert(type->length > 0);
      return type_size(type->fields.array) * type->length;
   case GLSL_TYPE_STRUCT:
      size = 0;
      for (i = 0; i < type->length; i++) {
	 size += type_size(type->fields.structure[i].type);
      }
      return size;
   case GLSL_TYPE_SAMPLER:
      /* Samplers take up one slot in UNIFORMS[], but they're baked in
       * at link time.
       */
      return 1;
   default:
      assert(0);
      return 0;
   }
}

int
vec4_visitor::virtual_grf_alloc(int size)
{
   if (virtual_grf_array_size <= virtual_grf_count) {
      if (virtual_grf_array_size == 0)
	 virtual_grf_array_size = 16;
      else
	 virtual_grf_array_size *= 2;
      virtual_grf_sizes = reralloc(mem_ctx, virtual_grf_sizes, int,
				   virtual_grf_array_size);
      virtual_grf_reg_map = reralloc(mem_ctx, virtual_grf_reg_map, int,
				     virtual_grf_array_size);
   }
   virtual_grf_reg_map[virtual_grf_count] = virtual_grf_reg_count;
   virtual_grf_reg_count += size;
   virtual_grf_sizes[virtual_grf_count] = size;
   return virtual_grf_count++;
}

src_reg::src_reg(class vec4_visitor *v, const struct glsl_type *type)
{
   init();

   this->file = GRF;
   this->reg = v->virtual_grf_alloc(type_size(type));

   if (type->is_array() || type->is_record()) {
      this->swizzle = BRW_SWIZZLE_NOOP;
   } else {
      this->swizzle = swizzle_for_size(type->vector_elements);
   }

   this->type = brw_type_for_base_type(type);
}

dst_reg::dst_reg(class vec4_visitor *v, const struct glsl_type *type)
{
   init();

   this->file = GRF;
   this->reg = v->virtual_grf_alloc(type_size(type));

   if (type->is_array() || type->is_record()) {
      this->writemask = WRITEMASK_XYZW;
   } else {
      this->writemask = (1 << type->vector_elements) - 1;
   }

   this->type = brw_type_for_base_type(type);
}

/* Our support for uniforms is piggy-backed on the struct
 * gl_fragment_program, because that's where the values actually
 * get stored, rather than in some global gl_shader_program uniform
 * store.
 */
int
vec4_visitor::setup_uniform_values(int loc, const glsl_type *type)
{
   unsigned int offset = 0;
   float *values = &this->vp->Base.Parameters->ParameterValues[loc][0].f;

   if (type->is_matrix()) {
      const glsl_type *column = type->column_type();

      for (unsigned int i = 0; i < type->matrix_columns; i++) {
	 offset += setup_uniform_values(loc + offset, column);
      }

      return offset;
   }

   switch (type->base_type) {
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_BOOL:
      for (unsigned int i = 0; i < type->vector_elements; i++) {
	 c->prog_data.param[this->uniforms * 4 + i] = &values[i];
      }

      /* Set up pad elements to get things aligned to a vec4 boundary. */
      for (unsigned int i = type->vector_elements; i < 4; i++) {
	 static float zero = 0;

	 c->prog_data.param[this->uniforms * 4 + i] = &zero;
      }

      /* Track the size of this uniform vector, for future packing of
       * uniforms.
       */
      this->uniform_vector_size[this->uniforms] = type->vector_elements;
      this->uniforms++;

      return 1;

   case GLSL_TYPE_STRUCT:
      for (unsigned int i = 0; i < type->length; i++) {
	 offset += setup_uniform_values(loc + offset,
					type->fields.structure[i].type);
      }
      return offset;

   case GLSL_TYPE_ARRAY:
      for (unsigned int i = 0; i < type->length; i++) {
	 offset += setup_uniform_values(loc + offset, type->fields.array);
      }
      return offset;

   case GLSL_TYPE_SAMPLER:
      /* The sampler takes up a slot, but we don't use any values from it. */
      return 1;

   default:
      assert(!"not reached");
      return 0;
   }
}

void
vec4_visitor::setup_uniform_clipplane_values()
{
   gl_clip_plane *clip_planes = brw_select_clip_planes(ctx);

   /* Pre-Gen6, we compact clip planes.  For example, if the user
    * enables just clip planes 0, 1, and 3, we will enable clip planes
    * 0, 1, and 2 in the hardware, and we'll move clip plane 3 to clip
    * plane 2.  This simplifies the implementation of the Gen6 clip
    * thread.
    *
    * In Gen6 and later, we don't compact clip planes, because this
    * simplifies the implementation of gl_ClipDistance.
    */
   int compacted_clipplane_index = 0;
   for (int i = 0; i < c->key.nr_userclip_plane_consts; ++i) {
      if (intel->gen < 6 &&
          !(c->key.userclip_planes_enabled_gen_4_5 & (1 << i))) {
         continue;
      }
      this->uniform_vector_size[this->uniforms] = 4;
      this->userplane[compacted_clipplane_index] = dst_reg(UNIFORM, this->uniforms);
      this->userplane[compacted_clipplane_index].type = BRW_REGISTER_TYPE_F;
      for (int j = 0; j < 4; ++j) {
         c->prog_data.param[this->uniforms * 4 + j] = &clip_planes[i][j];
      }
      ++compacted_clipplane_index;
      ++this->uniforms;
   }
}

/* Our support for builtin uniforms is even scarier than non-builtin.
 * It sits on top of the PROG_STATE_VAR parameters that are
 * automatically updated from GL context state.
 */
void
vec4_visitor::setup_builtin_uniform_values(ir_variable *ir)
{
   const ir_state_slot *const slots = ir->state_slots;
   assert(ir->state_slots != NULL);

   for (unsigned int i = 0; i < ir->num_state_slots; i++) {
      /* This state reference has already been setup by ir_to_mesa,
       * but we'll get the same index back here.  We can reference
       * ParameterValues directly, since unlike brw_fs.cpp, we never
       * add new state references during compile.
       */
      int index = _mesa_add_state_reference(this->vp->Base.Parameters,
					    (gl_state_index *)slots[i].tokens);
      float *values = &this->vp->Base.Parameters->ParameterValues[index][0].f;

      this->uniform_vector_size[this->uniforms] = 0;
      /* Add each of the unique swizzled channels of the element.
       * This will end up matching the size of the glsl_type of this field.
       */
      int last_swiz = -1;
      for (unsigned int j = 0; j < 4; j++) {
	 int swiz = GET_SWZ(slots[i].swizzle, j);
	 last_swiz = swiz;

	 c->prog_data.param[this->uniforms * 4 + j] = &values[swiz];
	 if (swiz <= last_swiz)
	    this->uniform_vector_size[this->uniforms]++;
      }
      this->uniforms++;
   }
}

dst_reg *
vec4_visitor::variable_storage(ir_variable *var)
{
   return (dst_reg *)hash_table_find(this->variable_ht, var);
}

void
vec4_visitor::emit_bool_to_cond_code(ir_rvalue *ir, uint32_t *predicate)
{
   ir_expression *expr = ir->as_expression();

   *predicate = BRW_PREDICATE_NORMAL;

   if (expr) {
      src_reg op[2];
      vec4_instruction *inst;

      assert(expr->get_num_operands() <= 2);
      for (unsigned int i = 0; i < expr->get_num_operands(); i++) {
	 expr->operands[i]->accept(this);
	 op[i] = this->result;

	 resolve_ud_negate(&op[i]);
      }

      switch (expr->operation) {
      case ir_unop_logic_not:
	 inst = emit(AND(dst_null_d(), op[0], src_reg(1)));
	 inst->conditional_mod = BRW_CONDITIONAL_Z;
	 break;

      case ir_binop_logic_xor:
	 inst = emit(XOR(dst_null_d(), op[0], op[1]));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_binop_logic_or:
	 inst = emit(OR(dst_null_d(), op[0], op[1]));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_binop_logic_and:
	 inst = emit(AND(dst_null_d(), op[0], op[1]));
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;

      case ir_unop_f2b:
	 if (intel->gen >= 6) {
	    emit(CMP(dst_null_d(), op[0], src_reg(0.0f), BRW_CONDITIONAL_NZ));
	 } else {
	    inst = emit(MOV(dst_null_f(), op[0]));
	    inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 }
	 break;

      case ir_unop_i2b:
	 if (intel->gen >= 6) {
	    emit(CMP(dst_null_d(), op[0], src_reg(0), BRW_CONDITIONAL_NZ));
	 } else {
	    inst = emit(MOV(dst_null_d(), op[0]));
	    inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 }
	 break;

      case ir_binop_all_equal:
	 inst = emit(CMP(dst_null_d(), op[0], op[1], BRW_CONDITIONAL_Z));
	 *predicate = BRW_PREDICATE_ALIGN16_ALL4H;
	 break;

      case ir_binop_any_nequal:
	 inst = emit(CMP(dst_null_d(), op[0], op[1], BRW_CONDITIONAL_NZ));
	 *predicate = BRW_PREDICATE_ALIGN16_ANY4H;
	 break;

      case ir_unop_any:
	 inst = emit(CMP(dst_null_d(), op[0], src_reg(0), BRW_CONDITIONAL_NZ));
	 *predicate = BRW_PREDICATE_ALIGN16_ANY4H;
	 break;

      case ir_binop_greater:
      case ir_binop_gequal:
      case ir_binop_less:
      case ir_binop_lequal:
      case ir_binop_equal:
      case ir_binop_nequal:
	 emit(CMP(dst_null_d(), op[0], op[1],
		  brw_conditional_for_comparison(expr->operation)));
	 break;

      default:
	 assert(!"not reached");
	 break;
      }
      return;
   }

   ir->accept(this);

   resolve_ud_negate(&this->result);

   if (intel->gen >= 6) {
      vec4_instruction *inst = emit(AND(dst_null_d(),
					this->result, src_reg(1)));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
   } else {
      vec4_instruction *inst = emit(MOV(dst_null_d(), this->result));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
   }
}

/**
 * Emit a gen6 IF statement with the comparison folded into the IF
 * instruction.
 */
void
vec4_visitor::emit_if_gen6(ir_if *ir)
{
   ir_expression *expr = ir->condition->as_expression();

   if (expr) {
      src_reg op[2];
      dst_reg temp;

      assert(expr->get_num_operands() <= 2);
      for (unsigned int i = 0; i < expr->get_num_operands(); i++) {
	 expr->operands[i]->accept(this);
	 op[i] = this->result;
      }

      switch (expr->operation) {
      case ir_unop_logic_not:
	 emit(IF(op[0], src_reg(0), BRW_CONDITIONAL_Z));
	 return;

      case ir_binop_logic_xor:
	 emit(IF(op[0], op[1], BRW_CONDITIONAL_NZ));
	 return;

      case ir_binop_logic_or:
	 temp = dst_reg(this, glsl_type::bool_type);
	 emit(OR(temp, op[0], op[1]));
	 emit(IF(src_reg(temp), src_reg(0), BRW_CONDITIONAL_NZ));
	 return;

      case ir_binop_logic_and:
	 temp = dst_reg(this, glsl_type::bool_type);
	 emit(AND(temp, op[0], op[1]));
	 emit(IF(src_reg(temp), src_reg(0), BRW_CONDITIONAL_NZ));
	 return;

      case ir_unop_f2b:
	 emit(IF(op[0], src_reg(0), BRW_CONDITIONAL_NZ));
	 return;

      case ir_unop_i2b:
	 emit(IF(op[0], src_reg(0), BRW_CONDITIONAL_NZ));
	 return;

      case ir_binop_greater:
      case ir_binop_gequal:
      case ir_binop_less:
      case ir_binop_lequal:
      case ir_binop_equal:
      case ir_binop_nequal:
	 emit(IF(op[0], op[1],
		 brw_conditional_for_comparison(expr->operation)));
	 return;

      case ir_binop_all_equal:
	 emit(CMP(dst_null_d(), op[0], op[1], BRW_CONDITIONAL_Z));
	 emit(IF(BRW_PREDICATE_ALIGN16_ALL4H));
	 return;

      case ir_binop_any_nequal:
	 emit(CMP(dst_null_d(), op[0], op[1], BRW_CONDITIONAL_NZ));
	 emit(IF(BRW_PREDICATE_ALIGN16_ANY4H));
	 return;

      case ir_unop_any:
	 emit(CMP(dst_null_d(), op[0], src_reg(0), BRW_CONDITIONAL_NZ));
	 emit(IF(BRW_PREDICATE_ALIGN16_ANY4H));
	 return;

      default:
	 assert(!"not reached");
	 emit(IF(op[0], src_reg(0), BRW_CONDITIONAL_NZ));
	 return;
      }
      return;
   }

   ir->condition->accept(this);

   emit(IF(this->result, src_reg(0), BRW_CONDITIONAL_NZ));
}

void
vec4_visitor::visit(ir_variable *ir)
{
   dst_reg *reg = NULL;

   if (variable_storage(ir))
      return;

   switch (ir->mode) {
   case ir_var_in:
      reg = new(mem_ctx) dst_reg(ATTR, ir->location);

      /* Do GL_FIXED rescaling for GLES2.0.  Our GL_FIXED attributes
       * come in as floating point conversions of the integer values.
       */
      for (int i = ir->location; i < ir->location + type_size(ir->type); i++) {
	 if (!c->key.gl_fixed_input_size[i])
	    continue;

	 dst_reg dst = *reg;
         dst.type = brw_type_for_base_type(ir->type);
	 dst.writemask = (1 << c->key.gl_fixed_input_size[i]) - 1;
	 emit(MUL(dst, src_reg(dst), src_reg(1.0f / 65536.0f)));
      }
      break;

   case ir_var_out:
      reg = new(mem_ctx) dst_reg(this, ir->type);

      for (int i = 0; i < type_size(ir->type); i++) {
	 output_reg[ir->location + i] = *reg;
	 output_reg[ir->location + i].reg_offset = i;
	 output_reg[ir->location + i].type =
            brw_type_for_base_type(ir->type->get_scalar_type());
	 output_reg_annotation[ir->location + i] = ir->name;
      }
      break;

   case ir_var_auto:
   case ir_var_temporary:
      reg = new(mem_ctx) dst_reg(this, ir->type);
      break;

   case ir_var_uniform:
      reg = new(this->mem_ctx) dst_reg(UNIFORM, this->uniforms);

      /* Thanks to the lower_ubo_reference pass, we will see only
       * ir_binop_ubo_load expressions and not ir_dereference_variable for UBO
       * variables, so no need for them to be in variable_ht.
       */
      if (ir->uniform_block != -1)
         return;

      /* Track how big the whole uniform variable is, in case we need to put a
       * copy of its data into pull constants for array access.
       */
      this->uniform_size[this->uniforms] = type_size(ir->type);

      if (!strncmp(ir->name, "gl_", 3)) {
	 setup_builtin_uniform_values(ir);
      } else {
	 setup_uniform_values(ir->location, ir->type);
      }
      break;

   case ir_var_system_value:
      /* VertexID is stored by the VF as the last vertex element, but
       * we don't represent it with a flag in inputs_read, so we call
       * it VERT_ATTRIB_MAX, which setup_attributes() picks up on.
       */
      reg = new(mem_ctx) dst_reg(ATTR, VERT_ATTRIB_MAX);
      prog_data->uses_vertexid = true;

      switch (ir->location) {
      case SYSTEM_VALUE_VERTEX_ID:
	 reg->writemask = WRITEMASK_X;
	 break;
      case SYSTEM_VALUE_INSTANCE_ID:
	 reg->writemask = WRITEMASK_Y;
	 break;
      default:
	 assert(!"not reached");
	 break;
      }
      break;

   default:
      assert(!"not reached");
   }

   reg->type = brw_type_for_base_type(ir->type);
   hash_table_insert(this->variable_ht, reg, ir);
}

void
vec4_visitor::visit(ir_loop *ir)
{
   dst_reg counter;

   /* We don't want debugging output to print the whole body of the
    * loop as the annotation.
    */
   this->base_ir = NULL;

   if (ir->counter != NULL) {
      this->base_ir = ir->counter;
      ir->counter->accept(this);
      counter = *(variable_storage(ir->counter));

      if (ir->from != NULL) {
	 this->base_ir = ir->from;
	 ir->from->accept(this);

	 emit(MOV(counter, this->result));
      }
   }

   emit(BRW_OPCODE_DO);

   if (ir->to) {
      this->base_ir = ir->to;
      ir->to->accept(this);

      emit(CMP(dst_null_d(), src_reg(counter), this->result,
	       brw_conditional_for_comparison(ir->cmp)));

      vec4_instruction *inst = emit(BRW_OPCODE_BREAK);
      inst->predicate = BRW_PREDICATE_NORMAL;
   }

   visit_instructions(&ir->body_instructions);


   if (ir->increment) {
      this->base_ir = ir->increment;
      ir->increment->accept(this);
      emit(ADD(counter, src_reg(counter), this->result));
   }

   emit(BRW_OPCODE_WHILE);
}

void
vec4_visitor::visit(ir_loop_jump *ir)
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
vec4_visitor::visit(ir_function_signature *ir)
{
   assert(0);
   (void)ir;
}

void
vec4_visitor::visit(ir_function *ir)
{
   /* Ignore function bodies other than main() -- we shouldn't see calls to
    * them since they should all be inlined.
    */
   if (strcmp(ir->name, "main") == 0) {
      const ir_function_signature *sig;
      exec_list empty;

      sig = ir->matching_signature(&empty);

      assert(sig);

      visit_instructions(&sig->body);
   }
}

bool
vec4_visitor::try_emit_sat(ir_expression *ir)
{
   ir_rvalue *sat_src = ir->as_rvalue_to_saturate();
   if (!sat_src)
      return false;

   sat_src->accept(this);
   src_reg src = this->result;

   this->result = src_reg(this, ir->type);
   vec4_instruction *inst;
   inst = emit(MOV(dst_reg(this->result), src));
   inst->saturate = true;

   return true;
}

void
vec4_visitor::emit_bool_comparison(unsigned int op,
				 dst_reg dst, src_reg src0, src_reg src1)
{
   /* original gen4 does destination conversion before comparison. */
   if (intel->gen < 5)
      dst.type = src0.type;

   emit(CMP(dst, src0, src1, brw_conditional_for_comparison(op)));

   dst.type = BRW_REGISTER_TYPE_D;
   emit(AND(dst, src_reg(dst), src_reg(0x1)));
}

void
vec4_visitor::visit(ir_expression *ir)
{
   unsigned int operand;
   src_reg op[Elements(ir->operands)];
   src_reg result_src;
   dst_reg result_dst;
   vec4_instruction *inst;

   if (try_emit_sat(ir))
      return;

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      this->result.file = BAD_FILE;
      ir->operands[operand]->accept(this);
      if (this->result.file == BAD_FILE) {
	 printf("Failed to get tree for expression operand:\n");
	 ir->operands[operand]->print();
	 exit(1);
      }
      op[operand] = this->result;

      /* Matrix expression operands should have been broken down to vector
       * operations already.
       */
      assert(!ir->operands[operand]->type->is_matrix());
   }

   int vector_elements = ir->operands[0]->type->vector_elements;
   if (ir->operands[1]) {
      vector_elements = MAX2(vector_elements,
			     ir->operands[1]->type->vector_elements);
   }

   this->result.file = BAD_FILE;

   /* Storage for our result.  Ideally for an assignment we'd be using
    * the actual storage for the result here, instead.
    */
   result_src = src_reg(this, ir->type);
   /* convenience for the emit functions below. */
   result_dst = dst_reg(result_src);
   /* If nothing special happens, this is the result. */
   this->result = result_src;
   /* Limit writes to the channels that will be used by result_src later.
    * This does limit this temp's use as a temporary for multi-instruction
    * sequences.
    */
   result_dst.writemask = (1 << ir->type->vector_elements) - 1;

   switch (ir->operation) {
   case ir_unop_logic_not:
      /* Note that BRW_OPCODE_NOT is not appropriate here, since it is
       * ones complement of the whole register, not just bit 0.
       */
      emit(XOR(result_dst, op[0], src_reg(1)));
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
      emit(MOV(result_dst, src_reg(0.0f)));

      emit(CMP(dst_null_d(), op[0], src_reg(0.0f), BRW_CONDITIONAL_G));
      inst = emit(MOV(result_dst, src_reg(1.0f)));
      inst->predicate = BRW_PREDICATE_NORMAL;

      emit(CMP(dst_null_d(), op[0], src_reg(0.0f), BRW_CONDITIONAL_L));
      inst = emit(MOV(result_dst, src_reg(-1.0f)));
      inst->predicate = BRW_PREDICATE_NORMAL;

      break;

   case ir_unop_rcp:
      emit_math(SHADER_OPCODE_RCP, result_dst, op[0]);
      break;

   case ir_unop_exp2:
      emit_math(SHADER_OPCODE_EXP2, result_dst, op[0]);
      break;
   case ir_unop_log2:
      emit_math(SHADER_OPCODE_LOG2, result_dst, op[0]);
      break;
   case ir_unop_exp:
   case ir_unop_log:
      assert(!"not reached: should be handled by ir_explog_to_explog2");
      break;
   case ir_unop_sin:
   case ir_unop_sin_reduced:
      emit_math(SHADER_OPCODE_SIN, result_dst, op[0]);
      break;
   case ir_unop_cos:
   case ir_unop_cos_reduced:
      emit_math(SHADER_OPCODE_COS, result_dst, op[0]);
      break;

   case ir_unop_dFdx:
   case ir_unop_dFdy:
      assert(!"derivatives not valid in vertex shader");
      break;

   case ir_unop_noise:
      assert(!"not reached: should be handled by lower_noise");
      break;

   case ir_binop_add:
      emit(ADD(result_dst, op[0], op[1]));
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
	 struct brw_reg acc = retype(brw_acc_reg(), BRW_REGISTER_TYPE_D);

	 emit(MUL(acc, op[0], op[1]));
	 emit(MACH(dst_null_d(), op[0], op[1]));
	 emit(MOV(result_dst, src_reg(acc)));
      } else {
	 emit(MUL(result_dst, op[0], op[1]));
      }
      break;
   case ir_binop_div:
      /* Floating point should be lowered by DIV_TO_MUL_RCP in the compiler. */
      assert(ir->type->is_integer());
      emit_math(SHADER_OPCODE_INT_QUOTIENT, result_dst, op[0], op[1]);
      break;
   case ir_binop_mod:
      /* Floating point should be lowered by MOD_TO_FRACT in the compiler. */
      assert(ir->type->is_integer());
      emit_math(SHADER_OPCODE_INT_REMAINDER, result_dst, op[0], op[1]);
      break;

   case ir_binop_less:
   case ir_binop_greater:
   case ir_binop_lequal:
   case ir_binop_gequal:
   case ir_binop_equal:
   case ir_binop_nequal: {
      emit(CMP(result_dst, op[0], op[1],
	       brw_conditional_for_comparison(ir->operation)));
      emit(AND(result_dst, result_src, src_reg(0x1)));
      break;
   }

   case ir_binop_all_equal:
      /* "==" operator producing a scalar boolean. */
      if (ir->operands[0]->type->is_vector() ||
	  ir->operands[1]->type->is_vector()) {
	 emit(CMP(dst_null_d(), op[0], op[1], BRW_CONDITIONAL_Z));
	 emit(MOV(result_dst, src_reg(0)));
	 inst = emit(MOV(result_dst, src_reg(1)));
	 inst->predicate = BRW_PREDICATE_ALIGN16_ALL4H;
      } else {
	 emit(CMP(result_dst, op[0], op[1], BRW_CONDITIONAL_Z));
	 emit(AND(result_dst, result_src, src_reg(0x1)));
      }
      break;
   case ir_binop_any_nequal:
      /* "!=" operator producing a scalar boolean. */
      if (ir->operands[0]->type->is_vector() ||
	  ir->operands[1]->type->is_vector()) {
	 emit(CMP(dst_null_d(), op[0], op[1], BRW_CONDITIONAL_NZ));

	 emit(MOV(result_dst, src_reg(0)));
	 inst = emit(MOV(result_dst, src_reg(1)));
	 inst->predicate = BRW_PREDICATE_ALIGN16_ANY4H;
      } else {
	 emit(CMP(result_dst, op[0], op[1], BRW_CONDITIONAL_NZ));
	 emit(AND(result_dst, result_src, src_reg(0x1)));
      }
      break;

   case ir_unop_any:
      emit(CMP(dst_null_d(), op[0], src_reg(0), BRW_CONDITIONAL_NZ));
      emit(MOV(result_dst, src_reg(0)));

      inst = emit(MOV(result_dst, src_reg(1)));
      inst->predicate = BRW_PREDICATE_ALIGN16_ANY4H;
      break;

   case ir_binop_logic_xor:
      emit(XOR(result_dst, op[0], op[1]));
      break;

   case ir_binop_logic_or:
      emit(OR(result_dst, op[0], op[1]));
      break;

   case ir_binop_logic_and:
      emit(AND(result_dst, op[0], op[1]));
      break;

   case ir_binop_dot:
      assert(ir->operands[0]->type->is_vector());
      assert(ir->operands[0]->type == ir->operands[1]->type);
      emit_dp(result_dst, op[0], op[1], ir->operands[0]->type->vector_elements);
      break;

   case ir_unop_sqrt:
      emit_math(SHADER_OPCODE_SQRT, result_dst, op[0]);
      break;
   case ir_unop_rsq:
      emit_math(SHADER_OPCODE_RSQ, result_dst, op[0]);
      break;

   case ir_unop_bitcast_i2f:
   case ir_unop_bitcast_u2f:
      this->result = op[0];
      this->result.type = BRW_REGISTER_TYPE_F;
      break;

   case ir_unop_bitcast_f2i:
      this->result = op[0];
      this->result.type = BRW_REGISTER_TYPE_D;
      break;

   case ir_unop_bitcast_f2u:
      this->result = op[0];
      this->result.type = BRW_REGISTER_TYPE_UD;
      break;

   case ir_unop_i2f:
   case ir_unop_i2u:
   case ir_unop_u2i:
   case ir_unop_u2f:
   case ir_unop_b2f:
   case ir_unop_b2i:
   case ir_unop_f2i:
   case ir_unop_f2u:
      emit(MOV(result_dst, op[0]));
      break;
   case ir_unop_f2b:
   case ir_unop_i2b: {
      emit(CMP(result_dst, op[0], src_reg(0.0f), BRW_CONDITIONAL_NZ));
      emit(AND(result_dst, result_src, src_reg(1)));
      break;
   }

   case ir_unop_trunc:
      emit(RNDZ(result_dst, op[0]));
      break;
   case ir_unop_ceil:
      op[0].negate = !op[0].negate;
      inst = emit(RNDD(result_dst, op[0]));
      this->result.negate = true;
      break;
   case ir_unop_floor:
      inst = emit(RNDD(result_dst, op[0]));
      break;
   case ir_unop_fract:
      inst = emit(FRC(result_dst, op[0]));
      break;
   case ir_unop_round_even:
      emit(RNDE(result_dst, op[0]));
      break;

   case ir_binop_min:
      if (intel->gen >= 6) {
	 inst = emit(BRW_OPCODE_SEL, result_dst, op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_L;
      } else {
	 emit(CMP(result_dst, op[0], op[1], BRW_CONDITIONAL_L));

	 inst = emit(BRW_OPCODE_SEL, result_dst, op[0], op[1]);
	 inst->predicate = BRW_PREDICATE_NORMAL;
      }
      break;
   case ir_binop_max:
      if (intel->gen >= 6) {
	 inst = emit(BRW_OPCODE_SEL, result_dst, op[0], op[1]);
	 inst->conditional_mod = BRW_CONDITIONAL_G;
      } else {
	 emit(CMP(result_dst, op[0], op[1], BRW_CONDITIONAL_G));

	 inst = emit(BRW_OPCODE_SEL, result_dst, op[0], op[1]);
	 inst->predicate = BRW_PREDICATE_NORMAL;
      }
      break;

   case ir_binop_pow:
      emit_math(SHADER_OPCODE_POW, result_dst, op[0], op[1]);
      break;

   case ir_unop_bit_not:
      inst = emit(NOT(result_dst, op[0]));
      break;
   case ir_binop_bit_and:
      inst = emit(AND(result_dst, op[0], op[1]));
      break;
   case ir_binop_bit_xor:
      inst = emit(XOR(result_dst, op[0], op[1]));
      break;
   case ir_binop_bit_or:
      inst = emit(OR(result_dst, op[0], op[1]));
      break;

   case ir_binop_lshift:
      inst = emit(BRW_OPCODE_SHL, result_dst, op[0], op[1]);
      break;

   case ir_binop_rshift:
      if (ir->type->base_type == GLSL_TYPE_INT)
	 inst = emit(BRW_OPCODE_ASR, result_dst, op[0], op[1]);
      else
	 inst = emit(BRW_OPCODE_SHR, result_dst, op[0], op[1]);
      break;

   case ir_binop_ubo_load: {
      ir_constant *uniform_block = ir->operands[0]->as_constant();
      ir_constant *const_offset_ir = ir->operands[1]->as_constant();
      unsigned const_offset = const_offset_ir ? const_offset_ir->value.u[0] : 0;
      src_reg offset = op[1];

      /* Now, load the vector from that offset. */
      assert(ir->type->is_vector() || ir->type->is_scalar());

      src_reg packed_consts = src_reg(this, glsl_type::vec4_type);
      packed_consts.type = result.type;
      src_reg surf_index =
         src_reg(SURF_INDEX_VS_UBO(uniform_block->value.u[0]));
      if (const_offset_ir) {
         offset = src_reg(const_offset / 16);
      } else {
         emit(BRW_OPCODE_SHR, dst_reg(offset), offset, src_reg(4));
      }

      vec4_instruction *pull =
         emit(new(mem_ctx) vec4_instruction(this,
                                            VS_OPCODE_PULL_CONSTANT_LOAD,
                                            dst_reg(packed_consts),
                                            surf_index,
                                            offset));
      pull->base_mrf = 14;
      pull->mlen = 1;

      packed_consts.swizzle = swizzle_for_size(ir->type->vector_elements);
      packed_consts.swizzle += BRW_SWIZZLE4(const_offset % 16 / 4,
                                            const_offset % 16 / 4,
                                            const_offset % 16 / 4,
                                            const_offset % 16 / 4);

      /* UBO bools are any nonzero int.  We store bools as either 0 or 1. */
      if (ir->type->base_type == GLSL_TYPE_BOOL) {
         emit(CMP(result_dst, packed_consts, src_reg(0u),
                  BRW_CONDITIONAL_NZ));
         emit(AND(result_dst, result, src_reg(0x1)));
      } else {
         emit(MOV(result_dst, packed_consts));
      }
      break;
   }

   case ir_quadop_vector:
      assert(!"not reached: should be handled by lower_quadop_vector");
      break;
   }
}


void
vec4_visitor::visit(ir_swizzle *ir)
{
   src_reg src;
   int i = 0;
   int swizzle[4];

   /* Note that this is only swizzles in expressions, not those on the left
    * hand side of an assignment, which do write masking.  See ir_assignment
    * for that.
    */

   ir->val->accept(this);
   src = this->result;
   assert(src.file != BAD_FILE);

   for (i = 0; i < ir->type->vector_elements; i++) {
      switch (i) {
      case 0:
	 swizzle[i] = BRW_GET_SWZ(src.swizzle, ir->mask.x);
	 break;
      case 1:
	 swizzle[i] = BRW_GET_SWZ(src.swizzle, ir->mask.y);
	 break;
      case 2:
	 swizzle[i] = BRW_GET_SWZ(src.swizzle, ir->mask.z);
	 break;
      case 3:
	 swizzle[i] = BRW_GET_SWZ(src.swizzle, ir->mask.w);
	    break;
      }
   }
   for (; i < 4; i++) {
      /* Replicate the last channel out. */
      swizzle[i] = swizzle[ir->type->vector_elements - 1];
   }

   src.swizzle = BRW_SWIZZLE4(swizzle[0], swizzle[1], swizzle[2], swizzle[3]);

   this->result = src;
}

void
vec4_visitor::visit(ir_dereference_variable *ir)
{
   const struct glsl_type *type = ir->type;
   dst_reg *reg = variable_storage(ir->var);

   if (!reg) {
      fail("Failed to find variable storage for %s\n", ir->var->name);
      this->result = src_reg(brw_null_reg());
      return;
   }

   this->result = src_reg(*reg);

   /* System values get their swizzle from the dst_reg writemask */
   if (ir->var->mode == ir_var_system_value)
      return;

   if (type->is_scalar() || type->is_vector() || type->is_matrix())
      this->result.swizzle = swizzle_for_size(type->vector_elements);
}

void
vec4_visitor::visit(ir_dereference_array *ir)
{
   ir_constant *constant_index;
   src_reg src;
   int element_size = type_size(ir->type);

   constant_index = ir->array_index->constant_expression_value();

   ir->array->accept(this);
   src = this->result;

   if (constant_index) {
      src.reg_offset += constant_index->value.i[0] * element_size;
   } else {
      /* Variable index array dereference.  It eats the "vec4" of the
       * base of the array and an index that offsets the Mesa register
       * index.
       */
      ir->array_index->accept(this);

      src_reg index_reg;

      if (element_size == 1) {
	 index_reg = this->result;
      } else {
	 index_reg = src_reg(this, glsl_type::int_type);

	 emit(MUL(dst_reg(index_reg), this->result, src_reg(element_size)));
      }

      if (src.reladdr) {
	 src_reg temp = src_reg(this, glsl_type::int_type);

	 emit(ADD(dst_reg(temp), *src.reladdr, index_reg));

	 index_reg = temp;
      }

      src.reladdr = ralloc(mem_ctx, src_reg);
      memcpy(src.reladdr, &index_reg, sizeof(index_reg));
   }

   /* If the type is smaller than a vec4, replicate the last channel out. */
   if (ir->type->is_scalar() || ir->type->is_vector() || ir->type->is_matrix())
      src.swizzle = swizzle_for_size(ir->type->vector_elements);
   else
      src.swizzle = BRW_SWIZZLE_NOOP;
   src.type = brw_type_for_base_type(ir->type);

   this->result = src;
}

void
vec4_visitor::visit(ir_dereference_record *ir)
{
   unsigned int i;
   const glsl_type *struct_type = ir->record->type;
   int offset = 0;

   ir->record->accept(this);

   for (i = 0; i < struct_type->length; i++) {
      if (strcmp(struct_type->fields.structure[i].name, ir->field) == 0)
	 break;
      offset += type_size(struct_type->fields.structure[i].type);
   }

   /* If the type is smaller than a vec4, replicate the last channel out. */
   if (ir->type->is_scalar() || ir->type->is_vector() || ir->type->is_matrix())
      this->result.swizzle = swizzle_for_size(ir->type->vector_elements);
   else
      this->result.swizzle = BRW_SWIZZLE_NOOP;
   this->result.type = brw_type_for_base_type(ir->type);

   this->result.reg_offset += offset;
}

/**
 * We want to be careful in assignment setup to hit the actual storage
 * instead of potentially using a temporary like we might with the
 * ir_dereference handler.
 */
static dst_reg
get_assignment_lhs(ir_dereference *ir, vec4_visitor *v)
{
   /* The LHS must be a dereference.  If the LHS is a variable indexed array
    * access of a vector, it must be separated into a series conditional moves
    * before reaching this point (see ir_vec_index_to_cond_assign).
    */
   assert(ir->as_dereference());
   ir_dereference_array *deref_array = ir->as_dereference_array();
   if (deref_array) {
      assert(!deref_array->array->type->is_vector());
   }

   /* Use the rvalue deref handler for the most part.  We'll ignore
    * swizzles in it and write swizzles using writemask, though.
    */
   ir->accept(v);
   return dst_reg(v->result);
}

void
vec4_visitor::emit_block_move(dst_reg *dst, src_reg *src,
			      const struct glsl_type *type, uint32_t predicate)
{
   if (type->base_type == GLSL_TYPE_STRUCT) {
      for (unsigned int i = 0; i < type->length; i++) {
	 emit_block_move(dst, src, type->fields.structure[i].type, predicate);
      }
      return;
   }

   if (type->is_array()) {
      for (unsigned int i = 0; i < type->length; i++) {
	 emit_block_move(dst, src, type->fields.array, predicate);
      }
      return;
   }

   if (type->is_matrix()) {
      const struct glsl_type *vec_type;

      vec_type = glsl_type::get_instance(GLSL_TYPE_FLOAT,
					 type->vector_elements, 1);

      for (int i = 0; i < type->matrix_columns; i++) {
	 emit_block_move(dst, src, vec_type, predicate);
      }
      return;
   }

   assert(type->is_scalar() || type->is_vector());

   dst->type = brw_type_for_base_type(type);
   src->type = dst->type;

   dst->writemask = (1 << type->vector_elements) - 1;

   src->swizzle = swizzle_for_size(type->vector_elements);

   vec4_instruction *inst = emit(MOV(*dst, *src));
   inst->predicate = predicate;

   dst->reg_offset++;
   src->reg_offset++;
}


/* If the RHS processing resulted in an instruction generating a
 * temporary value, and it would be easy to rewrite the instruction to
 * generate its result right into the LHS instead, do so.  This ends
 * up reliably removing instructions where it can be tricky to do so
 * later without real UD chain information.
 */
bool
vec4_visitor::try_rewrite_rhs_to_dst(ir_assignment *ir,
				     dst_reg dst,
				     src_reg src,
				     vec4_instruction *pre_rhs_inst,
				     vec4_instruction *last_rhs_inst)
{
   /* This could be supported, but it would take more smarts. */
   if (ir->condition)
      return false;

   if (pre_rhs_inst == last_rhs_inst)
      return false; /* No instructions generated to work with. */

   /* Make sure the last instruction generated our source reg. */
   if (src.file != GRF ||
       src.file != last_rhs_inst->dst.file ||
       src.reg != last_rhs_inst->dst.reg ||
       src.reg_offset != last_rhs_inst->dst.reg_offset ||
       src.reladdr ||
       src.abs ||
       src.negate ||
       last_rhs_inst->predicate != BRW_PREDICATE_NONE)
      return false;

   /* Check that that last instruction fully initialized the channels
    * we want to use, in the order we want to use them.  We could
    * potentially reswizzle the operands of many instructions so that
    * we could handle out of order channels, but don't yet.
    */

   for (unsigned i = 0; i < 4; i++) {
      if (dst.writemask & (1 << i)) {
	 if (!(last_rhs_inst->dst.writemask & (1 << i)))
	    return false;

	 if (BRW_GET_SWZ(src.swizzle, i) != i)
	    return false;
      }
   }

   /* Success!  Rewrite the instruction. */
   last_rhs_inst->dst.file = dst.file;
   last_rhs_inst->dst.reg = dst.reg;
   last_rhs_inst->dst.reg_offset = dst.reg_offset;
   last_rhs_inst->dst.reladdr = dst.reladdr;
   last_rhs_inst->dst.writemask &= dst.writemask;

   return true;
}

void
vec4_visitor::visit(ir_assignment *ir)
{
   dst_reg dst = get_assignment_lhs(ir->lhs, this);
   uint32_t predicate = BRW_PREDICATE_NONE;

   if (!ir->lhs->type->is_scalar() &&
       !ir->lhs->type->is_vector()) {
      ir->rhs->accept(this);
      src_reg src = this->result;

      if (ir->condition) {
	 emit_bool_to_cond_code(ir->condition, &predicate);
      }

      /* emit_block_move doesn't account for swizzles in the source register.
       * This should be ok, since the source register is a structure or an
       * array, and those can't be swizzled.  But double-check to be sure.
       */
      assert(src.swizzle ==
             (ir->rhs->type->is_matrix()
              ? swizzle_for_size(ir->rhs->type->vector_elements)
              : BRW_SWIZZLE_NOOP));

      emit_block_move(&dst, &src, ir->rhs->type, predicate);
      return;
   }

   /* Now we're down to just a scalar/vector with writemasks. */
   int i;

   vec4_instruction *pre_rhs_inst, *last_rhs_inst;
   pre_rhs_inst = (vec4_instruction *)this->instructions.get_tail();

   ir->rhs->accept(this);

   last_rhs_inst = (vec4_instruction *)this->instructions.get_tail();

   src_reg src = this->result;

   int swizzles[4];
   int first_enabled_chan = 0;
   int src_chan = 0;

   assert(ir->lhs->type->is_vector() ||
	  ir->lhs->type->is_scalar());
   dst.writemask = ir->write_mask;

   for (int i = 0; i < 4; i++) {
      if (dst.writemask & (1 << i)) {
	 first_enabled_chan = BRW_GET_SWZ(src.swizzle, i);
	 break;
      }
   }

   /* Swizzle a small RHS vector into the channels being written.
    *
    * glsl ir treats write_mask as dictating how many channels are
    * present on the RHS while in our instructions we need to make
    * those channels appear in the slots of the vec4 they're written to.
    */
   for (int i = 0; i < 4; i++) {
      if (dst.writemask & (1 << i))
	 swizzles[i] = BRW_GET_SWZ(src.swizzle, src_chan++);
      else
	 swizzles[i] = first_enabled_chan;
   }
   src.swizzle = BRW_SWIZZLE4(swizzles[0], swizzles[1],
			      swizzles[2], swizzles[3]);

   if (try_rewrite_rhs_to_dst(ir, dst, src, pre_rhs_inst, last_rhs_inst)) {
      return;
   }

   if (ir->condition) {
      emit_bool_to_cond_code(ir->condition, &predicate);
   }

   for (i = 0; i < type_size(ir->lhs->type); i++) {
      vec4_instruction *inst = emit(MOV(dst, src));
      inst->predicate = predicate;

      dst.reg_offset++;
      src.reg_offset++;
   }
}

void
vec4_visitor::emit_constant_values(dst_reg *dst, ir_constant *ir)
{
   if (ir->type->base_type == GLSL_TYPE_STRUCT) {
      foreach_list(node, &ir->components) {
	 ir_constant *field_value = (ir_constant *)node;

	 emit_constant_values(dst, field_value);
      }
      return;
   }

   if (ir->type->is_array()) {
      for (unsigned int i = 0; i < ir->type->length; i++) {
	 emit_constant_values(dst, ir->array_elements[i]);
      }
      return;
   }

   if (ir->type->is_matrix()) {
      for (int i = 0; i < ir->type->matrix_columns; i++) {
	 float *vec = &ir->value.f[i * ir->type->vector_elements];

	 for (int j = 0; j < ir->type->vector_elements; j++) {
	    dst->writemask = 1 << j;
	    dst->type = BRW_REGISTER_TYPE_F;

	    emit(MOV(*dst, src_reg(vec[j])));
	 }
	 dst->reg_offset++;
      }
      return;
   }

   int remaining_writemask = (1 << ir->type->vector_elements) - 1;

   for (int i = 0; i < ir->type->vector_elements; i++) {
      if (!(remaining_writemask & (1 << i)))
	 continue;

      dst->writemask = 1 << i;
      dst->type = brw_type_for_base_type(ir->type);

      /* Find other components that match the one we're about to
       * write.  Emits fewer instructions for things like vec4(0.5,
       * 1.5, 1.5, 1.5).
       */
      for (int j = i + 1; j < ir->type->vector_elements; j++) {
	 if (ir->type->base_type == GLSL_TYPE_BOOL) {
	    if (ir->value.b[i] == ir->value.b[j])
	       dst->writemask |= (1 << j);
	 } else {
	    /* u, i, and f storage all line up, so no need for a
	     * switch case for comparing each type.
	     */
	    if (ir->value.u[i] == ir->value.u[j])
	       dst->writemask |= (1 << j);
	 }
      }

      switch (ir->type->base_type) {
      case GLSL_TYPE_FLOAT:
	 emit(MOV(*dst, src_reg(ir->value.f[i])));
	 break;
      case GLSL_TYPE_INT:
	 emit(MOV(*dst, src_reg(ir->value.i[i])));
	 break;
      case GLSL_TYPE_UINT:
	 emit(MOV(*dst, src_reg(ir->value.u[i])));
	 break;
      case GLSL_TYPE_BOOL:
	 emit(MOV(*dst, src_reg(ir->value.b[i])));
	 break;
      default:
	 assert(!"Non-float/uint/int/bool constant");
	 break;
      }

      remaining_writemask &= ~dst->writemask;
   }
   dst->reg_offset++;
}

void
vec4_visitor::visit(ir_constant *ir)
{
   dst_reg dst = dst_reg(this, ir->type);
   this->result = src_reg(dst);

   emit_constant_values(&dst, ir);
}

void
vec4_visitor::visit(ir_call *ir)
{
   assert(!"not reached");
}

void
vec4_visitor::visit(ir_texture *ir)
{
   int sampler = _mesa_get_sampler_uniform_value(ir->sampler, prog, &vp->Base);

   /* Should be lowered by do_lower_texture_projection */
   assert(!ir->projector);

   /* Generate code to compute all the subexpression trees.  This has to be
    * done before loading any values into MRFs for the sampler message since
    * generating these values may involve SEND messages that need the MRFs.
    */
   src_reg coordinate;
   if (ir->coordinate) {
      ir->coordinate->accept(this);
      coordinate = this->result;
   }

   src_reg shadow_comparitor;
   if (ir->shadow_comparitor) {
      ir->shadow_comparitor->accept(this);
      shadow_comparitor = this->result;
   }

   const glsl_type *lod_type;
   src_reg lod, dPdx, dPdy;
   switch (ir->op) {
   case ir_tex:
      lod = src_reg(0.0f);
      lod_type = glsl_type::float_type;
      break;
   case ir_txf:
   case ir_txl:
   case ir_txs:
      ir->lod_info.lod->accept(this);
      lod = this->result;
      lod_type = ir->lod_info.lod->type;
      break;
   case ir_txd:
      ir->lod_info.grad.dPdx->accept(this);
      dPdx = this->result;

      ir->lod_info.grad.dPdy->accept(this);
      dPdy = this->result;

      lod_type = ir->lod_info.grad.dPdx->type;
      break;
   case ir_txb:
      break;
   }

   vec4_instruction *inst = NULL;
   switch (ir->op) {
   case ir_tex:
   case ir_txl:
      inst = new(mem_ctx) vec4_instruction(this, SHADER_OPCODE_TXL);
      break;
   case ir_txd:
      inst = new(mem_ctx) vec4_instruction(this, SHADER_OPCODE_TXD);
      break;
   case ir_txf:
      inst = new(mem_ctx) vec4_instruction(this, SHADER_OPCODE_TXF);
      break;
   case ir_txs:
      inst = new(mem_ctx) vec4_instruction(this, SHADER_OPCODE_TXS);
      break;
   case ir_txb:
      assert(!"TXB is not valid for vertex shaders.");
   }

   /* Texel offsets go in the message header; Gen4 also requires headers. */
   inst->header_present = ir->offset || intel->gen < 5;
   inst->base_mrf = 2;
   inst->mlen = inst->header_present + 1; /* always at least one */
   inst->sampler = sampler;
   inst->dst = dst_reg(this, ir->type);
   inst->dst.writemask = WRITEMASK_XYZW;
   inst->shadow_compare = ir->shadow_comparitor != NULL;

   if (ir->offset != NULL && ir->op != ir_txf)
      inst->texture_offset = brw_texture_offset(ir->offset->as_constant());

   /* MRF for the first parameter */
   int param_base = inst->base_mrf + inst->header_present;

   if (ir->op == ir_txs) {
      int writemask = intel->gen == 4 ? WRITEMASK_W : WRITEMASK_X;
      emit(MOV(dst_reg(MRF, param_base, lod_type, writemask), lod));
   } else {
      int i, coord_mask = 0, zero_mask = 0;
      /* Load the coordinate */
      /* FINISHME: gl_clamp_mask and saturate */
      for (i = 0; i < ir->coordinate->type->vector_elements; i++)
	 coord_mask |= (1 << i);
      for (; i < 4; i++)
	 zero_mask |= (1 << i);

      if (ir->offset && ir->op == ir_txf) {
	 /* It appears that the ld instruction used for txf does its
	  * address bounds check before adding in the offset.  To work
	  * around this, just add the integer offset to the integer
	  * texel coordinate, and don't put the offset in the header.
	  */
	 ir_constant *offset = ir->offset->as_constant();
	 assert(offset);

	 for (int j = 0; j < ir->coordinate->type->vector_elements; j++) {
	    src_reg src = coordinate;
	    src.swizzle = BRW_SWIZZLE4(BRW_GET_SWZ(src.swizzle, j),
				       BRW_GET_SWZ(src.swizzle, j),
				       BRW_GET_SWZ(src.swizzle, j),
				       BRW_GET_SWZ(src.swizzle, j));
	    emit(ADD(dst_reg(MRF, param_base, ir->coordinate->type, 1 << j),
		     src, offset->value.i[j]));
	 }
      } else {
	 emit(MOV(dst_reg(MRF, param_base, ir->coordinate->type, coord_mask),
		  coordinate));
      }
      emit(MOV(dst_reg(MRF, param_base, ir->coordinate->type, zero_mask),
	       src_reg(0)));
      /* Load the shadow comparitor */
      if (ir->shadow_comparitor) {
	 emit(MOV(dst_reg(MRF, param_base + 1, ir->shadow_comparitor->type,
			  WRITEMASK_X),
		  shadow_comparitor));
	 inst->mlen++;
      }

      /* Load the LOD info */
      if (ir->op == ir_tex || ir->op == ir_txl) {
	 int mrf, writemask;
	 if (intel->gen >= 5) {
	    mrf = param_base + 1;
	    if (ir->shadow_comparitor) {
	       writemask = WRITEMASK_Y;
	       /* mlen already incremented */
	    } else {
	       writemask = WRITEMASK_X;
	       inst->mlen++;
	    }
	 } else /* intel->gen == 4 */ {
	    mrf = param_base;
	    writemask = WRITEMASK_Z;
	 }
	 emit(MOV(dst_reg(MRF, mrf, lod_type, writemask), lod));
      } else if (ir->op == ir_txf) {
	 emit(MOV(dst_reg(MRF, param_base, lod_type, WRITEMASK_W),
		  lod));
      } else if (ir->op == ir_txd) {
	 const glsl_type *type = lod_type;

	 if (intel->gen >= 5) {
	    dPdx.swizzle = BRW_SWIZZLE4(SWIZZLE_X,SWIZZLE_X,SWIZZLE_Y,SWIZZLE_Y);
	    dPdy.swizzle = BRW_SWIZZLE4(SWIZZLE_X,SWIZZLE_X,SWIZZLE_Y,SWIZZLE_Y);
	    emit(MOV(dst_reg(MRF, param_base + 1, type, WRITEMASK_XZ), dPdx));
	    emit(MOV(dst_reg(MRF, param_base + 1, type, WRITEMASK_YW), dPdy));
	    inst->mlen++;

	    if (ir->type->vector_elements == 3) {
	       dPdx.swizzle = BRW_SWIZZLE_ZZZZ;
	       dPdy.swizzle = BRW_SWIZZLE_ZZZZ;
	       emit(MOV(dst_reg(MRF, param_base + 2, type, WRITEMASK_X), dPdx));
	       emit(MOV(dst_reg(MRF, param_base + 2, type, WRITEMASK_Y), dPdy));
	       inst->mlen++;
	    }
	 } else /* intel->gen == 4 */ {
	    emit(MOV(dst_reg(MRF, param_base + 1, type, WRITEMASK_XYZ), dPdx));
	    emit(MOV(dst_reg(MRF, param_base + 2, type, WRITEMASK_XYZ), dPdy));
	    inst->mlen += 2;
	 }
      }
   }

   emit(inst);

   swizzle_result(ir, src_reg(inst->dst), sampler);
}

void
vec4_visitor::swizzle_result(ir_texture *ir, src_reg orig_val, int sampler)
{
   int s = c->key.tex.swizzles[sampler];

   this->result = src_reg(this, ir->type);
   dst_reg swizzled_result(this->result);

   if (ir->op == ir_txs || ir->type == glsl_type::float_type
			|| s == SWIZZLE_NOOP) {
      emit(MOV(swizzled_result, orig_val));
      return;
   }

   int zero_mask = 0, one_mask = 0, copy_mask = 0;
   int swizzle[4];

   for (int i = 0; i < 4; i++) {
      switch (GET_SWZ(s, i)) {
      case SWIZZLE_ZERO:
	 zero_mask |= (1 << i);
	 break;
      case SWIZZLE_ONE:
	 one_mask |= (1 << i);
	 break;
      default:
	 copy_mask |= (1 << i);
	 swizzle[i] = GET_SWZ(s, i);
	 break;
      }
   }

   if (copy_mask) {
      orig_val.swizzle = BRW_SWIZZLE4(swizzle[0], swizzle[1], swizzle[2], swizzle[3]);
      swizzled_result.writemask = copy_mask;
      emit(MOV(swizzled_result, orig_val));
   }

   if (zero_mask) {
      swizzled_result.writemask = zero_mask;
      emit(MOV(swizzled_result, src_reg(0.0f)));
   }

   if (one_mask) {
      swizzled_result.writemask = one_mask;
      emit(MOV(swizzled_result, src_reg(1.0f)));
   }
}

void
vec4_visitor::visit(ir_return *ir)
{
   assert(!"not reached");
}

void
vec4_visitor::visit(ir_discard *ir)
{
   assert(!"not reached");
}

void
vec4_visitor::visit(ir_if *ir)
{
   /* Don't point the annotation at the if statement, because then it plus
    * the then and else blocks get printed.
    */
   this->base_ir = ir->condition;

   if (intel->gen == 6) {
      emit_if_gen6(ir);
   } else {
      uint32_t predicate;
      emit_bool_to_cond_code(ir->condition, &predicate);
      emit(IF(predicate));
   }

   visit_instructions(&ir->then_instructions);

   if (!ir->else_instructions.is_empty()) {
      this->base_ir = ir->condition;
      emit(BRW_OPCODE_ELSE);

      visit_instructions(&ir->else_instructions);
   }

   this->base_ir = ir->condition;
   emit(BRW_OPCODE_ENDIF);
}

void
vec4_visitor::emit_ndc_computation()
{
   /* Get the position */
   src_reg pos = src_reg(output_reg[VERT_RESULT_HPOS]);

   /* Build ndc coords, which are (x/w, y/w, z/w, 1/w) */
   dst_reg ndc = dst_reg(this, glsl_type::vec4_type);
   output_reg[BRW_VERT_RESULT_NDC] = ndc;

   current_annotation = "NDC";
   dst_reg ndc_w = ndc;
   ndc_w.writemask = WRITEMASK_W;
   src_reg pos_w = pos;
   pos_w.swizzle = BRW_SWIZZLE4(SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W);
   emit_math(SHADER_OPCODE_RCP, ndc_w, pos_w);

   dst_reg ndc_xyz = ndc;
   ndc_xyz.writemask = WRITEMASK_XYZ;

   emit(MUL(ndc_xyz, pos, src_reg(ndc_w)));
}

void
vec4_visitor::emit_psiz_and_flags(struct brw_reg reg)
{
   if (intel->gen < 6 &&
       ((c->prog_data.outputs_written & BITFIELD64_BIT(VERT_RESULT_PSIZ)) ||
        c->key.userclip_active || brw->has_negative_rhw_bug)) {
      dst_reg header1 = dst_reg(this, glsl_type::uvec4_type);
      dst_reg header1_w = header1;
      header1_w.writemask = WRITEMASK_W;
      GLuint i;

      emit(MOV(header1, 0u));

      if (c->prog_data.outputs_written & BITFIELD64_BIT(VERT_RESULT_PSIZ)) {
	 src_reg psiz = src_reg(output_reg[VERT_RESULT_PSIZ]);

	 current_annotation = "Point size";
	 emit(MUL(header1_w, psiz, src_reg((float)(1 << 11))));
	 emit(AND(header1_w, src_reg(header1_w), 0x7ff << 8));
      }

      current_annotation = "Clipping flags";
      for (i = 0; i < c->key.nr_userclip_plane_consts; i++) {
	 vec4_instruction *inst;

	 inst = emit(DP4(dst_null_f(), src_reg(output_reg[VERT_RESULT_HPOS]),
                         src_reg(this->userplane[i])));
	 inst->conditional_mod = BRW_CONDITIONAL_L;

	 inst = emit(OR(header1_w, src_reg(header1_w), 1u << i));
	 inst->predicate = BRW_PREDICATE_NORMAL;
      }

      /* i965 clipping workaround:
       * 1) Test for -ve rhw
       * 2) If set,
       *      set ndc = (0,0,0,0)
       *      set ucp[6] = 1
       *
       * Later, clipping will detect ucp[6] and ensure the primitive is
       * clipped against all fixed planes.
       */
      if (brw->has_negative_rhw_bug) {
#if 0
	 /* FINISHME */
	 brw_CMP(p,
		 vec8(brw_null_reg()),
		 BRW_CONDITIONAL_L,
		 brw_swizzle1(output_reg[BRW_VERT_RESULT_NDC], 3),
		 brw_imm_f(0));

	 brw_OR(p, brw_writemask(header1, WRITEMASK_W), header1, brw_imm_ud(1<<6));
	 brw_MOV(p, output_reg[BRW_VERT_RESULT_NDC], brw_imm_f(0));
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
#endif
      }

      emit(MOV(retype(reg, BRW_REGISTER_TYPE_UD), src_reg(header1)));
   } else if (intel->gen < 6) {
      emit(MOV(retype(reg, BRW_REGISTER_TYPE_UD), 0u));
   } else {
      emit(MOV(retype(reg, BRW_REGISTER_TYPE_D), src_reg(0)));
      if (c->prog_data.outputs_written & BITFIELD64_BIT(VERT_RESULT_PSIZ)) {
         emit(MOV(brw_writemask(reg, WRITEMASK_W),
                  src_reg(output_reg[VERT_RESULT_PSIZ])));
      }
   }
}

void
vec4_visitor::emit_clip_distances(struct brw_reg reg, int offset)
{
   if (intel->gen < 6) {
      /* Clip distance slots are set aside in gen5, but they are not used.  It
       * is not clear whether we actually need to set aside space for them,
       * but the performance cost is negligible.
       */
      return;
   }

   /* From the GLSL 1.30 spec, section 7.1 (Vertex Shader Special Variables):
    *
    *     "If a linked set of shaders forming the vertex stage contains no
    *     static write to gl_ClipVertex or gl_ClipDistance, but the
    *     application has requested clipping against user clip planes through
    *     the API, then the coordinate written to gl_Position is used for
    *     comparison against the user clip planes."
    *
    * This function is only called if the shader didn't write to
    * gl_ClipDistance.  Accordingly, we use gl_ClipVertex to perform clipping
    * if the user wrote to it; otherwise we use gl_Position.
    */
   gl_vert_result clip_vertex = VERT_RESULT_CLIP_VERTEX;
   if (!(c->prog_data.outputs_written
         & BITFIELD64_BIT(VERT_RESULT_CLIP_VERTEX))) {
      clip_vertex = VERT_RESULT_HPOS;
   }

   for (int i = 0; i + offset < c->key.nr_userclip_plane_consts && i < 4;
        ++i) {
      emit(DP4(dst_reg(brw_writemask(reg, 1 << i)),
               src_reg(output_reg[clip_vertex]),
               src_reg(this->userplane[i + offset])));
   }
}

void
vec4_visitor::emit_generic_urb_slot(dst_reg reg, int vert_result)
{
   assert (vert_result < VERT_RESULT_MAX);
   reg.type = output_reg[vert_result].type;
   current_annotation = output_reg_annotation[vert_result];
   /* Copy the register, saturating if necessary */
   vec4_instruction *inst = emit(MOV(reg,
                                     src_reg(output_reg[vert_result])));
   if ((vert_result == VERT_RESULT_COL0 ||
        vert_result == VERT_RESULT_COL1 ||
        vert_result == VERT_RESULT_BFC0 ||
        vert_result == VERT_RESULT_BFC1) &&
       c->key.clamp_vertex_color) {
      inst->saturate = true;
   }
}

void
vec4_visitor::emit_urb_slot(int mrf, int vert_result)
{
   struct brw_reg hw_reg = brw_message_reg(mrf);
   dst_reg reg = dst_reg(MRF, mrf);
   reg.type = BRW_REGISTER_TYPE_F;

   switch (vert_result) {
   case VERT_RESULT_PSIZ:
      /* PSIZ is always in slot 0, and is coupled with other flags. */
      current_annotation = "indices, point width, clip flags";
      emit_psiz_and_flags(hw_reg);
      break;
   case BRW_VERT_RESULT_NDC:
      current_annotation = "NDC";
      emit(MOV(reg, src_reg(output_reg[BRW_VERT_RESULT_NDC])));
      break;
   case BRW_VERT_RESULT_HPOS_DUPLICATE:
   case VERT_RESULT_HPOS:
      current_annotation = "gl_Position";
      emit(MOV(reg, src_reg(output_reg[VERT_RESULT_HPOS])));
      break;
   case VERT_RESULT_CLIP_DIST0:
   case VERT_RESULT_CLIP_DIST1:
      if (this->c->key.uses_clip_distance) {
         emit_generic_urb_slot(reg, vert_result);
      } else {
         current_annotation = "user clip distances";
         emit_clip_distances(hw_reg, (vert_result - VERT_RESULT_CLIP_DIST0) * 4);
      }
      break;
   case VERT_RESULT_EDGE:
      /* This is present when doing unfilled polygons.  We're supposed to copy
       * the edge flag from the user-provided vertex array
       * (glEdgeFlagPointer), or otherwise we'll copy from the current value
       * of that attribute (starts as 1.0f).  This is then used in clipping to
       * determine which edges should be drawn as wireframe.
       */
      current_annotation = "edge flag";
      emit(MOV(reg, src_reg(dst_reg(ATTR, VERT_ATTRIB_EDGEFLAG,
                                    glsl_type::float_type, WRITEMASK_XYZW))));
      break;
   case BRW_VERT_RESULT_PAD:
      /* No need to write to this slot */
      break;
   default:
      emit_generic_urb_slot(reg, vert_result);
      break;
   }
}

static int
align_interleaved_urb_mlen(struct brw_context *brw, int mlen)
{
   struct intel_context *intel = &brw->intel;

   if (intel->gen >= 6) {
      /* URB data written (does not include the message header reg) must
       * be a multiple of 256 bits, or 2 VS registers.  See vol5c.5,
       * section 5.4.3.2.2: URB_INTERLEAVED.
       *
       * URB entries are allocated on a multiple of 1024 bits, so an
       * extra 128 bits written here to make the end align to 256 is
       * no problem.
       */
      if ((mlen % 2) != 1)
	 mlen++;
   }

   return mlen;
}

/**
 * Generates the VUE payload plus the 1 or 2 URB write instructions to
 * complete the VS thread.
 *
 * The VUE layout is documented in Volume 2a.
 */
void
vec4_visitor::emit_urb_writes()
{
   /* MRF 0 is reserved for the debugger, so start with message header
    * in MRF 1.
    */
   int base_mrf = 1;
   int mrf = base_mrf;
   /* In the process of generating our URB write message contents, we
    * may need to unspill a register or load from an array.  Those
    * reads would use MRFs 14-15.
    */
   int max_usable_mrf = 13;

   /* The following assertion verifies that max_usable_mrf causes an
    * even-numbered amount of URB write data, which will meet gen6's
    * requirements for length alignment.
    */
   assert ((max_usable_mrf - base_mrf) % 2 == 0);

   /* First mrf is the g0-based message header containing URB handles and such,
    * which is implied in VS_OPCODE_URB_WRITE.
    */
   mrf++;

   if (intel->gen < 6) {
      emit_ndc_computation();
   }

   /* Set up the VUE data for the first URB write */
   int slot;
   for (slot = 0; slot < c->prog_data.vue_map.num_slots; ++slot) {
      emit_urb_slot(mrf++, c->prog_data.vue_map.slot_to_vert_result[slot]);

      /* If this was max_usable_mrf, we can't fit anything more into this URB
       * WRITE.
       */
      if (mrf > max_usable_mrf) {
	 slot++;
	 break;
      }
   }

   current_annotation = "URB write";
   vec4_instruction *inst = emit(VS_OPCODE_URB_WRITE);
   inst->base_mrf = base_mrf;
   inst->mlen = align_interleaved_urb_mlen(brw, mrf - base_mrf);
   inst->eot = (slot >= c->prog_data.vue_map.num_slots);

   /* Optional second URB write */
   if (!inst->eot) {
      mrf = base_mrf + 1;

      for (; slot < c->prog_data.vue_map.num_slots; ++slot) {
	 assert(mrf < max_usable_mrf);

         emit_urb_slot(mrf++, c->prog_data.vue_map.slot_to_vert_result[slot]);
      }

      current_annotation = "URB write";
      inst = emit(VS_OPCODE_URB_WRITE);
      inst->base_mrf = base_mrf;
      inst->mlen = align_interleaved_urb_mlen(brw, mrf - base_mrf);
      inst->eot = true;
      /* URB destination offset.  In the previous write, we got MRFs
       * 2-13 minus the one header MRF, so 12 regs.  URB offset is in
       * URB row increments, and each of our MRFs is half of one of
       * those, since we're doing interleaved writes.
       */
      inst->offset = (max_usable_mrf - base_mrf) / 2;
   }
}

src_reg
vec4_visitor::get_scratch_offset(vec4_instruction *inst,
				 src_reg *reladdr, int reg_offset)
{
   /* Because we store the values to scratch interleaved like our
    * vertex data, we need to scale the vec4 index by 2.
    */
   int message_header_scale = 2;

   /* Pre-gen6, the message header uses byte offsets instead of vec4
    * (16-byte) offset units.
    */
   if (intel->gen < 6)
      message_header_scale *= 16;

   if (reladdr) {
      src_reg index = src_reg(this, glsl_type::int_type);

      emit_before(inst, ADD(dst_reg(index), *reladdr, src_reg(reg_offset)));
      emit_before(inst, MUL(dst_reg(index),
			    index, src_reg(message_header_scale)));

      return index;
   } else {
      return src_reg(reg_offset * message_header_scale);
   }
}

src_reg
vec4_visitor::get_pull_constant_offset(vec4_instruction *inst,
				       src_reg *reladdr, int reg_offset)
{
   if (reladdr) {
      src_reg index = src_reg(this, glsl_type::int_type);

      emit_before(inst, ADD(dst_reg(index), *reladdr, src_reg(reg_offset)));

      /* Pre-gen6, the message header uses byte offsets instead of vec4
       * (16-byte) offset units.
       */
      if (intel->gen < 6) {
	 emit_before(inst, MUL(dst_reg(index), index, src_reg(16)));
      }

      return index;
   } else {
      int message_header_scale = intel->gen < 6 ? 16 : 1;
      return src_reg(reg_offset * message_header_scale);
   }
}

/**
 * Emits an instruction before @inst to load the value named by @orig_src
 * from scratch space at @base_offset to @temp.
 *
 * @base_offset is measured in 32-byte units (the size of a register).
 */
void
vec4_visitor::emit_scratch_read(vec4_instruction *inst,
				dst_reg temp, src_reg orig_src,
				int base_offset)
{
   int reg_offset = base_offset + orig_src.reg_offset;
   src_reg index = get_scratch_offset(inst, orig_src.reladdr, reg_offset);

   emit_before(inst, SCRATCH_READ(temp, index));
}

/**
 * Emits an instruction after @inst to store the value to be written
 * to @orig_dst to scratch space at @base_offset, from @temp.
 *
 * @base_offset is measured in 32-byte units (the size of a register).
 */
void
vec4_visitor::emit_scratch_write(vec4_instruction *inst,
				 src_reg temp, dst_reg orig_dst,
				 int base_offset)
{
   int reg_offset = base_offset + orig_dst.reg_offset;
   src_reg index = get_scratch_offset(inst, orig_dst.reladdr, reg_offset);

   dst_reg dst = dst_reg(brw_writemask(brw_vec8_grf(0, 0),
				       orig_dst.writemask));
   vec4_instruction *write = SCRATCH_WRITE(dst, temp, index);
   write->predicate = inst->predicate;
   write->ir = inst->ir;
   write->annotation = inst->annotation;
   inst->insert_after(write);
}

/**
 * We can't generally support array access in GRF space, because a
 * single instruction's destination can only span 2 contiguous
 * registers.  So, we send all GRF arrays that get variable index
 * access to scratch space.
 */
void
vec4_visitor::move_grf_array_access_to_scratch()
{
   int scratch_loc[this->virtual_grf_count];

   for (int i = 0; i < this->virtual_grf_count; i++) {
      scratch_loc[i] = -1;
   }

   /* First, calculate the set of virtual GRFs that need to be punted
    * to scratch due to having any array access on them, and where in
    * scratch.
    */
   foreach_list(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      if (inst->dst.file == GRF && inst->dst.reladdr &&
	  scratch_loc[inst->dst.reg] == -1) {
	 scratch_loc[inst->dst.reg] = c->last_scratch;
	 c->last_scratch += this->virtual_grf_sizes[inst->dst.reg];
      }

      for (int i = 0 ; i < 3; i++) {
	 src_reg *src = &inst->src[i];

	 if (src->file == GRF && src->reladdr &&
	     scratch_loc[src->reg] == -1) {
	    scratch_loc[src->reg] = c->last_scratch;
	    c->last_scratch += this->virtual_grf_sizes[src->reg];
	 }
      }
   }

   /* Now, for anything that will be accessed through scratch, rewrite
    * it to load/store.  Note that this is a _safe list walk, because
    * we may generate a new scratch_write instruction after the one
    * we're processing.
    */
   foreach_list_safe(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      /* Set up the annotation tracking for new generated instructions. */
      base_ir = inst->ir;
      current_annotation = inst->annotation;

      if (inst->dst.file == GRF && scratch_loc[inst->dst.reg] != -1) {
	 src_reg temp = src_reg(this, glsl_type::vec4_type);

	 emit_scratch_write(inst, temp, inst->dst, scratch_loc[inst->dst.reg]);

	 inst->dst.file = temp.file;
	 inst->dst.reg = temp.reg;
	 inst->dst.reg_offset = temp.reg_offset;
	 inst->dst.reladdr = NULL;
      }

      for (int i = 0 ; i < 3; i++) {
	 if (inst->src[i].file != GRF || scratch_loc[inst->src[i].reg] == -1)
	    continue;

	 dst_reg temp = dst_reg(this, glsl_type::vec4_type);

	 emit_scratch_read(inst, temp, inst->src[i],
			   scratch_loc[inst->src[i].reg]);

	 inst->src[i].file = temp.file;
	 inst->src[i].reg = temp.reg;
	 inst->src[i].reg_offset = temp.reg_offset;
	 inst->src[i].reladdr = NULL;
      }
   }
}

/**
 * Emits an instruction before @inst to load the value named by @orig_src
 * from the pull constant buffer (surface) at @base_offset to @temp.
 */
void
vec4_visitor::emit_pull_constant_load(vec4_instruction *inst,
				      dst_reg temp, src_reg orig_src,
				      int base_offset)
{
   int reg_offset = base_offset + orig_src.reg_offset;
   src_reg index = src_reg((unsigned)SURF_INDEX_VERT_CONST_BUFFER);
   src_reg offset = get_pull_constant_offset(inst, orig_src.reladdr, reg_offset);
   vec4_instruction *load;

   load = new(mem_ctx) vec4_instruction(this, VS_OPCODE_PULL_CONSTANT_LOAD,
					temp, index, offset);
   load->base_mrf = 14;
   load->mlen = 1;
   emit_before(inst, load);
}

/**
 * Implements array access of uniforms by inserting a
 * PULL_CONSTANT_LOAD instruction.
 *
 * Unlike temporary GRF array access (where we don't support it due to
 * the difficulty of doing relative addressing on instruction
 * destinations), we could potentially do array access of uniforms
 * that were loaded in GRF space as push constants.  In real-world
 * usage we've seen, though, the arrays being used are always larger
 * than we could load as push constants, so just always move all
 * uniform array access out to a pull constant buffer.
 */
void
vec4_visitor::move_uniform_array_access_to_pull_constants()
{
   int pull_constant_loc[this->uniforms];

   for (int i = 0; i < this->uniforms; i++) {
      pull_constant_loc[i] = -1;
   }

   /* Walk through and find array access of uniforms.  Put a copy of that
    * uniform in the pull constant buffer.
    *
    * Note that we don't move constant-indexed accesses to arrays.  No
    * testing has been done of the performance impact of this choice.
    */
   foreach_list_safe(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      for (int i = 0 ; i < 3; i++) {
	 if (inst->src[i].file != UNIFORM || !inst->src[i].reladdr)
	    continue;

	 int uniform = inst->src[i].reg;

	 /* If this array isn't already present in the pull constant buffer,
	  * add it.
	  */
	 if (pull_constant_loc[uniform] == -1) {
	    const float **values = &prog_data->param[uniform * 4];

	    pull_constant_loc[uniform] = prog_data->nr_pull_params / 4;

	    for (int j = 0; j < uniform_size[uniform] * 4; j++) {
	       prog_data->pull_param[prog_data->nr_pull_params++] = values[j];
	    }
	 }

	 /* Set up the annotation tracking for new generated instructions. */
	 base_ir = inst->ir;
	 current_annotation = inst->annotation;

	 dst_reg temp = dst_reg(this, glsl_type::vec4_type);

	 emit_pull_constant_load(inst, temp, inst->src[i],
				 pull_constant_loc[uniform]);

	 inst->src[i].file = temp.file;
	 inst->src[i].reg = temp.reg;
	 inst->src[i].reg_offset = temp.reg_offset;
	 inst->src[i].reladdr = NULL;
      }
   }

   /* Now there are no accesses of the UNIFORM file with a reladdr, so
    * no need to track them as larger-than-vec4 objects.  This will be
    * relied on in cutting out unused uniform vectors from push
    * constants.
    */
   split_uniform_registers();
}

void
vec4_visitor::resolve_ud_negate(src_reg *reg)
{
   if (reg->type != BRW_REGISTER_TYPE_UD ||
       !reg->negate)
      return;

   src_reg temp = src_reg(this, glsl_type::uvec4_type);
   emit(BRW_OPCODE_MOV, dst_reg(temp), *reg);
   *reg = temp;
}

vec4_visitor::vec4_visitor(struct brw_vs_compile *c,
			   struct gl_shader_program *prog,
			   struct brw_shader *shader)
{
   this->c = c;
   this->p = &c->func;
   this->brw = p->brw;
   this->intel = &brw->intel;
   this->ctx = &intel->ctx;
   this->prog = prog;
   this->shader = shader;

   this->mem_ctx = ralloc_context(NULL);
   this->failed = false;

   this->base_ir = NULL;
   this->current_annotation = NULL;

   this->c = c;
   this->vp = (struct gl_vertex_program *)
     prog->_LinkedShaders[MESA_SHADER_VERTEX]->Program;
   this->prog_data = &c->prog_data;

   this->variable_ht = hash_table_ctor(0,
				       hash_table_pointer_hash,
				       hash_table_pointer_compare);

   this->virtual_grf_def = NULL;
   this->virtual_grf_use = NULL;
   this->virtual_grf_sizes = NULL;
   this->virtual_grf_count = 0;
   this->virtual_grf_reg_map = NULL;
   this->virtual_grf_reg_count = 0;
   this->virtual_grf_array_size = 0;
   this->live_intervals_valid = false;

   this->max_grf = intel->gen >= 7 ? GEN7_MRF_HACK_START : BRW_MAX_GRF;

   this->uniforms = 0;
}

vec4_visitor::~vec4_visitor()
{
   ralloc_free(this->mem_ctx);
   hash_table_dtor(this->variable_ht);
}


void
vec4_visitor::fail(const char *format, ...)
{
   va_list va;
   char *msg;

   if (failed)
      return;

   failed = true;

   va_start(va, format);
   msg = ralloc_vasprintf(mem_ctx, format, va);
   va_end(va);
   msg = ralloc_asprintf(mem_ctx, "VS compile failed: %s\n", msg);

   this->fail_msg = msg;

   if (INTEL_DEBUG & DEBUG_VS) {
      fprintf(stderr, "%s",  msg);
   }
}

} /* namespace brw */
