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

extern "C" {
#include "main/macros.h"
#include "brw_context.h"
#include "brw_vs.h"
}
#include "brw_fs.h"
#include "glsl/ir_optimization.h"
#include "glsl/ir_print_visitor.h"

struct gl_shader *
brw_new_shader(struct gl_context *ctx, GLuint name, GLuint type)
{
   struct brw_shader *shader;

   shader = rzalloc(NULL, struct brw_shader);
   if (shader) {
      shader->base.Type = type;
      shader->base.Name = name;
      _mesa_init_shader(ctx, &shader->base);
   }

   return &shader->base;
}

struct gl_shader_program *
brw_new_shader_program(struct gl_context *ctx, GLuint name)
{
   struct brw_shader_program *prog;
   prog = rzalloc(NULL, struct brw_shader_program);
   if (prog) {
      prog->base.Name = name;
      _mesa_init_shader_program(ctx, &prog->base);
   }
   return &prog->base;
}

/**
 * Performs a compile of the shader stages even when we don't know
 * what non-orthogonal state will be set, in the hope that it reflects
 * the eventual NOS used, and thus allows us to produce link failures.
 */
bool
brw_shader_precompile(struct gl_context *ctx, struct gl_shader_program *prog)
{
   struct brw_context *brw = brw_context(ctx);

   if (brw->precompile && !brw_fs_precompile(ctx, prog))
      return false;

   if (brw->precompile && !brw_vs_precompile(ctx, prog))
      return false;

   return true;
}

GLboolean
brw_link_shader(struct gl_context *ctx, struct gl_shader_program *shProg)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_context *intel = &brw->intel;
   unsigned int stage;

   for (stage = 0; stage < ARRAY_SIZE(shProg->_LinkedShaders); stage++) {
      struct brw_shader *shader =
	 (struct brw_shader *)shProg->_LinkedShaders[stage];
      static const GLenum targets[] = {
	 GL_VERTEX_PROGRAM_ARB,
	 GL_FRAGMENT_PROGRAM_ARB,
	 GL_GEOMETRY_PROGRAM_NV
      };

      if (!shader)
	 continue;

      struct gl_program *prog =
	 ctx->Driver.NewProgram(ctx, targets[stage], shader->base.Name);
      if (!prog)
	return false;
      prog->Parameters = _mesa_new_parameter_list();

      _mesa_generate_parameters_list_for_uniforms(shProg, &shader->base,
						  prog->Parameters);

      if (stage == 0) {
	 struct gl_vertex_program *vp = (struct gl_vertex_program *) prog;
	 vp->UsesClipDistance = shProg->Vert.UsesClipDistance;
      }

      void *mem_ctx = ralloc_context(NULL);
      bool progress;

      if (shader->ir)
	 ralloc_free(shader->ir);
      shader->ir = new(shader) exec_list;
      clone_ir_list(mem_ctx, shader->ir, shader->base.ir);

      do_mat_op_to_vec(shader->ir);
      lower_instructions(shader->ir,
			 MOD_TO_FRACT |
			 DIV_TO_MUL_RCP |
			 SUB_TO_ADD_NEG |
			 EXP_TO_EXP2 |
			 LOG_TO_LOG2);

      /* Pre-gen6 HW can only nest if-statements 16 deep.  Beyond this,
       * if-statements need to be flattened.
       */
      if (intel->gen < 6)
	 lower_if_to_cond_assign(shader->ir, 16);

      do_lower_texture_projection(shader->ir);
      if (intel->gen < 8 && !intel->is_haswell)
         brw_lower_texture_gradients(shader->ir);
      do_vec_index_to_cond_assign(shader->ir);
      brw_do_cubemap_normalize(shader->ir);
      lower_noise(shader->ir);
      lower_quadop_vector(shader->ir, false);

      bool input = true;
      bool output = stage == MESA_SHADER_FRAGMENT;
      bool temp = stage == MESA_SHADER_FRAGMENT;
      bool uniform = stage == MESA_SHADER_FRAGMENT;

      lower_variable_index_to_cond_assign(shader->ir,
					  input, output, temp, uniform);

      /* FINISHME: Do this before the variable index lowering. */
      lower_ubo_reference(&shader->base, shader->ir);

      do {
	 progress = false;

	 if (stage == MESA_SHADER_FRAGMENT) {
	    brw_do_channel_expressions(shader->ir);
	    brw_do_vector_splitting(shader->ir);
	 }

	 progress = do_lower_jumps(shader->ir, true, true,
				   true, /* main return */
				   false, /* continue */
				   false /* loops */
				   ) || progress;

	 progress = do_common_optimization(shader->ir, true, true, 32)
	   || progress;
      } while (progress);

      /* Make a pass over the IR to add state references for any built-in
       * uniforms that are used.  This has to be done now (during linking).
       * Code generation doesn't happen until the first time this shader is
       * used for rendering.  Waiting until then to generate the parameters is
       * too late.  At that point, the values for the built-in informs won't
       * get sent to the shader.
       */
      foreach_list(node, shader->ir) {
	 ir_variable *var = ((ir_instruction *) node)->as_variable();

	 if ((var == NULL) || (var->mode != ir_var_uniform)
	     || (strncmp(var->name, "gl_", 3) != 0))
	    continue;

	 const ir_state_slot *const slots = var->state_slots;
	 assert(var->state_slots != NULL);

	 for (unsigned int i = 0; i < var->num_state_slots; i++) {
	    _mesa_add_state_reference(prog->Parameters,
				      (gl_state_index *) slots[i].tokens);
	 }
      }

      validate_ir_tree(shader->ir);

      reparent_ir(shader->ir, shader->ir);
      ralloc_free(mem_ctx);

      do_set_program_inouts(shader->ir, prog,
			    shader->base.Type == GL_FRAGMENT_SHADER);

      prog->SamplersUsed = shader->base.active_samplers;
      _mesa_update_shader_textures_used(shProg, prog);

      _mesa_reference_program(ctx, &shader->base.Program, prog);

      /* This has to be done last.  Any operation that can cause
       * prog->ParameterValues to get reallocated (e.g., anything that adds a
       * program constant) has to happen before creating this linkage.
       */
      _mesa_associate_uniform_storage(ctx, shProg, prog->Parameters);

      _mesa_reference_program(ctx, &prog, NULL);
   }

   if (!brw_shader_precompile(ctx, shProg))
      return false;

   return true;
}


int
brw_type_for_base_type(const struct glsl_type *type)
{
   switch (type->base_type) {
   case GLSL_TYPE_FLOAT:
      return BRW_REGISTER_TYPE_F;
   case GLSL_TYPE_INT:
   case GLSL_TYPE_BOOL:
      return BRW_REGISTER_TYPE_D;
   case GLSL_TYPE_UINT:
      return BRW_REGISTER_TYPE_UD;
   case GLSL_TYPE_ARRAY:
      return brw_type_for_base_type(type->fields.array);
   case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_SAMPLER:
      /* These should be overridden with the type of the member when
       * dereferenced into.  BRW_REGISTER_TYPE_UD seems like a likely
       * way to trip up if we don't.
       */
      return BRW_REGISTER_TYPE_UD;
   default:
      assert(!"not reached");
      return BRW_REGISTER_TYPE_F;
   }
}

uint32_t
brw_conditional_for_comparison(unsigned int op)
{
   switch (op) {
   case ir_binop_less:
      return BRW_CONDITIONAL_L;
   case ir_binop_greater:
      return BRW_CONDITIONAL_G;
   case ir_binop_lequal:
      return BRW_CONDITIONAL_LE;
   case ir_binop_gequal:
      return BRW_CONDITIONAL_GE;
   case ir_binop_equal:
   case ir_binop_all_equal: /* same as equal for scalars */
      return BRW_CONDITIONAL_Z;
   case ir_binop_nequal:
   case ir_binop_any_nequal: /* same as nequal for scalars */
      return BRW_CONDITIONAL_NZ;
   default:
      assert(!"not reached: bad operation for comparison");
      return BRW_CONDITIONAL_NZ;
   }
}

uint32_t
brw_math_function(enum opcode op)
{
   switch (op) {
   case SHADER_OPCODE_RCP:
      return BRW_MATH_FUNCTION_INV;
   case SHADER_OPCODE_RSQ:
      return BRW_MATH_FUNCTION_RSQ;
   case SHADER_OPCODE_SQRT:
      return BRW_MATH_FUNCTION_SQRT;
   case SHADER_OPCODE_EXP2:
      return BRW_MATH_FUNCTION_EXP;
   case SHADER_OPCODE_LOG2:
      return BRW_MATH_FUNCTION_LOG;
   case SHADER_OPCODE_POW:
      return BRW_MATH_FUNCTION_POW;
   case SHADER_OPCODE_SIN:
      return BRW_MATH_FUNCTION_SIN;
   case SHADER_OPCODE_COS:
      return BRW_MATH_FUNCTION_COS;
   case SHADER_OPCODE_INT_QUOTIENT:
      return BRW_MATH_FUNCTION_INT_DIV_QUOTIENT;
   case SHADER_OPCODE_INT_REMAINDER:
      return BRW_MATH_FUNCTION_INT_DIV_REMAINDER;
   default:
      assert(!"not reached: unknown math function");
      return 0;
   }
}

uint32_t
brw_texture_offset(ir_constant *offset)
{
   assert(offset != NULL);

   signed char offsets[3];
   for (unsigned i = 0; i < offset->type->vector_elements; i++)
      offsets[i] = (signed char) offset->value.i[i];

   /* Combine all three offsets into a single unsigned dword:
    *
    *    bits 11:8 - U Offset (X component)
    *    bits  7:4 - V Offset (Y component)
    *    bits  3:0 - R Offset (Z component)
    */
   unsigned offset_bits = 0;
   for (unsigned i = 0; i < offset->type->vector_elements; i++) {
      const unsigned shift = 4 * (2 - i);
      offset_bits |= (offsets[i] << shift) & (0xF << shift);
   }
   return offset_bits;
}
