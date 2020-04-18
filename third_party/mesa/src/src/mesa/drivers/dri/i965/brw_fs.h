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
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#pragma once

#include "brw_shader.h"

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
#include "glsl/glsl_types.h"
#include "glsl/ir.h"

class fs_bblock;
namespace {
   class acp_entry;
}

enum register_file {
   BAD_FILE,
   ARF,
   GRF,
   MRF,
   IMM,
   FIXED_HW_REG, /* a struct brw_reg */
   UNIFORM, /* prog_data->params[reg] */
};

class fs_reg {
public:
   /* Callers of this ralloc-based new need not call delete. It's
    * easier to just ralloc_free 'ctx' (or any of its ancestors). */
   static void* operator new(size_t size, void *ctx)
   {
      void *node;

      node = ralloc_size(ctx, size);
      assert(node != NULL);

      return node;
   }

   void init();

   fs_reg();
   fs_reg(float f);
   fs_reg(int32_t i);
   fs_reg(uint32_t u);
   fs_reg(struct brw_reg fixed_hw_reg);
   fs_reg(enum register_file file, int reg);
   fs_reg(enum register_file file, int reg, uint32_t type);
   fs_reg(class fs_visitor *v, const struct glsl_type *type);

   bool equals(const fs_reg &r) const;

   /** Register file: ARF, GRF, MRF, IMM. */
   enum register_file file;
   /**
    * Register number.  For ARF/MRF, it's the hardware register.  For
    * GRF, it's a virtual register number until register allocation
    */
   int reg;
   /**
    * For virtual registers, this is a hardware register offset from
    * the start of the register block (for example, a constant index
    * in an array access).
    */
   int reg_offset;
   /** Register type.  BRW_REGISTER_TYPE_* */
   int type;
   bool negate;
   bool abs;
   bool sechalf;
   struct brw_reg fixed_hw_reg;
   int smear; /* -1, or a channel of the reg to smear to all channels. */

   /** Value for file == IMM */
   union {
      int32_t i;
      uint32_t u;
      float f;
   } imm;
};

static const fs_reg reg_undef;
static const fs_reg reg_null_f(ARF, BRW_ARF_NULL, BRW_REGISTER_TYPE_F);
static const fs_reg reg_null_d(ARF, BRW_ARF_NULL, BRW_REGISTER_TYPE_D);

class fs_inst : public exec_node {
public:
   /* Callers of this ralloc-based new need not call delete. It's
    * easier to just ralloc_free 'ctx' (or any of its ancestors). */
   static void* operator new(size_t size, void *ctx)
   {
      void *node;

      node = rzalloc_size(ctx, size);
      assert(node != NULL);

      return node;
   }

   void init();

   fs_inst();
   fs_inst(enum opcode opcode);
   fs_inst(enum opcode opcode, fs_reg dst);
   fs_inst(enum opcode opcode, fs_reg dst, fs_reg src0);
   fs_inst(enum opcode opcode, fs_reg dst, fs_reg src0, fs_reg src1);
   fs_inst(enum opcode opcode, fs_reg dst,
           fs_reg src0, fs_reg src1,fs_reg src2);

   bool equals(fs_inst *inst);
   int regs_written();
   bool overwrites_reg(const fs_reg &reg);
   bool is_tex();
   bool is_math();

   enum opcode opcode; /* BRW_OPCODE_* or FS_OPCODE_* */
   fs_reg dst;
   fs_reg src[3];
   bool saturate;
   bool predicated;
   bool predicate_inverse;
   int conditional_mod; /**< BRW_CONDITIONAL_* */

   int mlen; /**< SEND message length */
   int base_mrf; /**< First MRF in the SEND message, if mlen is nonzero. */
   uint32_t texture_offset; /**< Texture offset bitfield */
   int sampler;
   int target; /**< MRT target. */
   bool eot;
   bool header_present;
   bool shadow_compare;
   bool force_uncompressed;
   bool force_sechalf;
   uint32_t offset; /* spill/unspill offset */

   /** @{
    * Annotation for the generated IR.  One of the two can be set.
    */
   ir_instruction *ir;
   const char *annotation;
   /** @} */
};

class fs_visitor : public ir_visitor
{
public:

   fs_visitor(struct brw_wm_compile *c, struct gl_shader_program *prog,
              struct brw_shader *shader);
   ~fs_visitor();

   fs_reg *variable_storage(ir_variable *var);
   int virtual_grf_alloc(int size);
   void import_uniforms(fs_visitor *v);

   void visit(ir_variable *ir);
   void visit(ir_assignment *ir);
   void visit(ir_dereference_variable *ir);
   void visit(ir_dereference_record *ir);
   void visit(ir_dereference_array *ir);
   void visit(ir_expression *ir);
   void visit(ir_texture *ir);
   void visit(ir_if *ir);
   void visit(ir_constant *ir);
   void visit(ir_swizzle *ir);
   void visit(ir_return *ir);
   void visit(ir_loop *ir);
   void visit(ir_loop_jump *ir);
   void visit(ir_discard *ir);
   void visit(ir_call *ir);
   void visit(ir_function *ir);
   void visit(ir_function_signature *ir);

   void swizzle_result(ir_texture *ir, fs_reg orig_val, int sampler);

   fs_inst *emit(fs_inst inst);

   fs_inst *emit(enum opcode opcode);
   fs_inst *emit(enum opcode opcode, fs_reg dst);
   fs_inst *emit(enum opcode opcode, fs_reg dst, fs_reg src0);
   fs_inst *emit(enum opcode opcode, fs_reg dst, fs_reg src0, fs_reg src1);
   fs_inst *emit(enum opcode opcode, fs_reg dst,
                 fs_reg src0, fs_reg src1, fs_reg src2);

   int type_size(const struct glsl_type *type);
   fs_inst *get_instruction_generating_reg(fs_inst *start,
					   fs_inst *end,
					   fs_reg reg);

   bool run();
   void setup_paramvalues_refs();
   void assign_curb_setup();
   void calculate_urb_setup();
   void assign_urb_setup();
   bool assign_regs();
   void assign_regs_trivial();
   int choose_spill_reg(struct ra_graph *g);
   void spill_reg(int spill_reg);
   void split_virtual_grfs();
   void setup_pull_constants();
   void calculate_live_intervals();
   bool propagate_constants();
   bool opt_algebraic();
   bool opt_cse();
   bool opt_cse_local(fs_bblock *block, exec_list *aeb);
   bool opt_copy_propagate();
   bool try_copy_propagate(fs_inst *inst, int arg, acp_entry *entry);
   bool opt_copy_propagate_local(void *mem_ctx, fs_bblock *block,
				 exec_list *acp);
   bool register_coalesce();
   bool register_coalesce_2();
   bool compute_to_mrf();
   bool dead_code_eliminate();
   bool remove_dead_constants();
   bool remove_duplicate_mrf_writes();
   bool virtual_grf_interferes(int a, int b);
   void schedule_instructions();
   void fail(const char *msg, ...);

   void push_force_uncompressed();
   void pop_force_uncompressed();
   void push_force_sechalf();
   void pop_force_sechalf();

   void generate_code();
   void generate_fb_write(fs_inst *inst);
   void generate_pixel_xy(struct brw_reg dst, bool is_x);
   void generate_linterp(fs_inst *inst, struct brw_reg dst,
			 struct brw_reg *src);
   void generate_tex(fs_inst *inst, struct brw_reg dst, struct brw_reg src);
   void generate_math1_gen7(fs_inst *inst,
			    struct brw_reg dst,
			    struct brw_reg src);
   void generate_math2_gen7(fs_inst *inst,
			    struct brw_reg dst,
			    struct brw_reg src0,
			    struct brw_reg src1);
   void generate_math1_gen6(fs_inst *inst,
			    struct brw_reg dst,
			    struct brw_reg src);
   void generate_math2_gen6(fs_inst *inst,
			    struct brw_reg dst,
			    struct brw_reg src0,
			    struct brw_reg src1);
   void generate_math_gen4(fs_inst *inst,
			   struct brw_reg dst,
			   struct brw_reg src);
   void generate_discard(fs_inst *inst);
   void generate_ddx(fs_inst *inst, struct brw_reg dst, struct brw_reg src);
   void generate_ddy(fs_inst *inst, struct brw_reg dst, struct brw_reg src,
                     bool negate_value);
   void generate_spill(fs_inst *inst, struct brw_reg src);
   void generate_unspill(fs_inst *inst, struct brw_reg dst);
   void generate_pull_constant_load(fs_inst *inst, struct brw_reg dst,
				    struct brw_reg index,
				    struct brw_reg offset);
   void generate_mov_dispatch_to_flags();

   void emit_dummy_fs();
   fs_reg *emit_fragcoord_interpolation(ir_variable *ir);
   fs_inst *emit_linterp(const fs_reg &attr, const fs_reg &interp,
                         glsl_interp_qualifier interpolation_mode,
                         bool is_centroid);
   fs_reg *emit_frontfacing_interpolation(ir_variable *ir);
   fs_reg *emit_general_interpolation(ir_variable *ir);
   void emit_interpolation_setup_gen4();
   void emit_interpolation_setup_gen6();
   fs_reg emit_texcoord(ir_texture *ir, int sampler, int texunit);
   fs_inst *emit_texture_gen4(ir_texture *ir, fs_reg dst, fs_reg coordinate,
			      fs_reg shadow_comp, fs_reg lod, fs_reg lod2);
   fs_inst *emit_texture_gen5(ir_texture *ir, fs_reg dst, fs_reg coordinate,
			      fs_reg shadow_comp, fs_reg lod, fs_reg lod2);
   fs_inst *emit_texture_gen7(ir_texture *ir, fs_reg dst, fs_reg coordinate,
			      fs_reg shadow_comp, fs_reg lod, fs_reg lod2);
   fs_inst *emit_math(enum opcode op, fs_reg dst, fs_reg src0);
   fs_inst *emit_math(enum opcode op, fs_reg dst, fs_reg src0, fs_reg src1);
   bool try_emit_saturate(ir_expression *ir);
   bool try_emit_mad(ir_expression *ir, int mul_arg);
   void emit_bool_to_cond_code(ir_rvalue *condition);
   void emit_if_gen6(ir_if *ir);
   void emit_unspill(fs_inst *inst, fs_reg reg, uint32_t spill_offset);

   void emit_color_write(int target, int index, int first_color_mrf);
   void emit_fb_writes();
   bool try_rewrite_rhs_to_dst(ir_assignment *ir,
			       fs_reg dst,
			       fs_reg src,
			       fs_inst *pre_rhs_inst,
			       fs_inst *last_rhs_inst);
   void emit_assignment_writes(fs_reg &l, fs_reg &r,
			       const glsl_type *type, bool predicated);
   void resolve_ud_negate(fs_reg *reg);
   void resolve_bool_comparison(ir_rvalue *rvalue, fs_reg *reg);

   struct brw_reg interp_reg(int location, int channel);
   int setup_uniform_values(int loc, const glsl_type *type);
   void setup_builtin_uniform_values(ir_variable *ir);
   int implied_mrf_writes(fs_inst *inst);

   struct brw_context *brw;
   const struct gl_fragment_program *fp;
   struct intel_context *intel;
   struct gl_context *ctx;
   struct brw_wm_compile *c;
   struct brw_compile *p;
   struct brw_shader *shader;
   struct gl_shader_program *prog;
   void *mem_ctx;
   exec_list instructions;

   /* Delayed setup of c->prog_data.params[] due to realloc of
    * ParamValues[] during compile.
    */
   int param_index[MAX_UNIFORMS * 4];
   int param_offset[MAX_UNIFORMS * 4];

   int *virtual_grf_sizes;
   int virtual_grf_count;
   int virtual_grf_array_size;
   int *virtual_grf_def;
   int *virtual_grf_use;
   bool live_intervals_valid;

   /* This is the map from UNIFORM hw_reg + reg_offset as generated by
    * the visitor to the packed uniform number after
    * remove_dead_constants() that represents the actual uploaded
    * uniform index.
    */
   int *params_remap;

   struct hash_table *variable_ht;
   ir_variable *frag_depth;
   fs_reg outputs[BRW_MAX_DRAW_BUFFERS];
   unsigned output_components[BRW_MAX_DRAW_BUFFERS];
   fs_reg dual_src_output;
   int first_non_payload_grf;
   int max_grf;
   int urb_setup[FRAG_ATTRIB_MAX];

   /** @{ debug annotation info */
   const char *current_annotation;
   ir_instruction *base_ir;
   /** @} */

   bool failed;
   char *fail_msg;

   /* Result of last visit() method. */
   fs_reg result;

   fs_reg pixel_x;
   fs_reg pixel_y;
   fs_reg wpos_w;
   fs_reg pixel_w;
   fs_reg delta_x[BRW_WM_BARYCENTRIC_INTERP_MODE_COUNT];
   fs_reg delta_y[BRW_WM_BARYCENTRIC_INTERP_MODE_COUNT];
   fs_reg reg_null_cmp;

   int grf_used;

   int force_uncompressed_stack;
   int force_sechalf_stack;

   class fs_bblock *bblock;
};

bool brw_do_channel_expressions(struct exec_list *instructions);
bool brw_do_vector_splitting(struct exec_list *instructions);
bool brw_fs_precompile(struct gl_context *ctx, struct gl_shader_program *prog);
