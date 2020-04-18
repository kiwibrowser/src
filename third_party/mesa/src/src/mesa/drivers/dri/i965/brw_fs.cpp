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

/** @file brw_fs.cpp
 *
 * This file drives the GLSL IR -> LIR translation, contains the
 * optimizations on the LIR, and drives the generation of native code
 * from the LIR.
 */

extern "C" {

#include <sys/types.h>

#include "main/macros.h"
#include "main/shaderobj.h"
#include "main/uniforms.h"
#include "main/fbobject.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"
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
#include "glsl/ir_print_visitor.h"

void
fs_inst::init()
{
   memset(this, 0, sizeof(*this));
   this->opcode = BRW_OPCODE_NOP;
   this->conditional_mod = BRW_CONDITIONAL_NONE;

   this->dst = reg_undef;
   this->src[0] = reg_undef;
   this->src[1] = reg_undef;
   this->src[2] = reg_undef;
}

fs_inst::fs_inst()
{
   init();
}

fs_inst::fs_inst(enum opcode opcode)
{
   init();
   this->opcode = opcode;
}

fs_inst::fs_inst(enum opcode opcode, fs_reg dst)
{
   init();
   this->opcode = opcode;
   this->dst = dst;

   if (dst.file == GRF)
      assert(dst.reg_offset >= 0);
}

fs_inst::fs_inst(enum opcode opcode, fs_reg dst, fs_reg src0)
{
   init();
   this->opcode = opcode;
   this->dst = dst;
   this->src[0] = src0;

   if (dst.file == GRF)
      assert(dst.reg_offset >= 0);
   if (src[0].file == GRF)
      assert(src[0].reg_offset >= 0);
}

fs_inst::fs_inst(enum opcode opcode, fs_reg dst, fs_reg src0, fs_reg src1)
{
   init();
   this->opcode = opcode;
   this->dst = dst;
   this->src[0] = src0;
   this->src[1] = src1;

   if (dst.file == GRF)
      assert(dst.reg_offset >= 0);
   if (src[0].file == GRF)
      assert(src[0].reg_offset >= 0);
   if (src[1].file == GRF)
      assert(src[1].reg_offset >= 0);
}

fs_inst::fs_inst(enum opcode opcode, fs_reg dst,
		 fs_reg src0, fs_reg src1, fs_reg src2)
{
   init();
   this->opcode = opcode;
   this->dst = dst;
   this->src[0] = src0;
   this->src[1] = src1;
   this->src[2] = src2;

   if (dst.file == GRF)
      assert(dst.reg_offset >= 0);
   if (src[0].file == GRF)
      assert(src[0].reg_offset >= 0);
   if (src[1].file == GRF)
      assert(src[1].reg_offset >= 0);
   if (src[2].file == GRF)
      assert(src[2].reg_offset >= 0);
}

bool
fs_inst::equals(fs_inst *inst)
{
   return (opcode == inst->opcode &&
           dst.equals(inst->dst) &&
           src[0].equals(inst->src[0]) &&
           src[1].equals(inst->src[1]) &&
           src[2].equals(inst->src[2]) &&
           saturate == inst->saturate &&
           predicated == inst->predicated &&
           conditional_mod == inst->conditional_mod &&
           mlen == inst->mlen &&
           base_mrf == inst->base_mrf &&
           sampler == inst->sampler &&
           target == inst->target &&
           eot == inst->eot &&
           header_present == inst->header_present &&
           shadow_compare == inst->shadow_compare &&
           offset == inst->offset);
}

int
fs_inst::regs_written()
{
   if (is_tex())
      return 4;

   /* The SINCOS and INT_DIV_QUOTIENT_AND_REMAINDER math functions return 2,
    * but we don't currently use them...nor do we have an opcode for them.
    */

   return 1;
}

bool
fs_inst::overwrites_reg(const fs_reg &reg)
{
   return (reg.file == dst.file &&
           reg.reg == dst.reg &&
           reg.reg_offset >= dst.reg_offset  &&
           reg.reg_offset < dst.reg_offset + regs_written());
}

bool
fs_inst::is_tex()
{
   return (opcode == SHADER_OPCODE_TEX ||
           opcode == FS_OPCODE_TXB ||
           opcode == SHADER_OPCODE_TXD ||
           opcode == SHADER_OPCODE_TXF ||
           opcode == SHADER_OPCODE_TXL ||
           opcode == SHADER_OPCODE_TXS);
}

bool
fs_inst::is_math()
{
   return (opcode == SHADER_OPCODE_RCP ||
           opcode == SHADER_OPCODE_RSQ ||
           opcode == SHADER_OPCODE_SQRT ||
           opcode == SHADER_OPCODE_EXP2 ||
           opcode == SHADER_OPCODE_LOG2 ||
           opcode == SHADER_OPCODE_SIN ||
           opcode == SHADER_OPCODE_COS ||
           opcode == SHADER_OPCODE_INT_QUOTIENT ||
           opcode == SHADER_OPCODE_INT_REMAINDER ||
           opcode == SHADER_OPCODE_POW);
}

void
fs_reg::init()
{
   memset(this, 0, sizeof(*this));
   this->smear = -1;
}

/** Generic unset register constructor. */
fs_reg::fs_reg()
{
   init();
   this->file = BAD_FILE;
}

/** Immediate value constructor. */
fs_reg::fs_reg(float f)
{
   init();
   this->file = IMM;
   this->type = BRW_REGISTER_TYPE_F;
   this->imm.f = f;
}

/** Immediate value constructor. */
fs_reg::fs_reg(int32_t i)
{
   init();
   this->file = IMM;
   this->type = BRW_REGISTER_TYPE_D;
   this->imm.i = i;
}

/** Immediate value constructor. */
fs_reg::fs_reg(uint32_t u)
{
   init();
   this->file = IMM;
   this->type = BRW_REGISTER_TYPE_UD;
   this->imm.u = u;
}

/** Fixed brw_reg Immediate value constructor. */
fs_reg::fs_reg(struct brw_reg fixed_hw_reg)
{
   init();
   this->file = FIXED_HW_REG;
   this->fixed_hw_reg = fixed_hw_reg;
   this->type = fixed_hw_reg.type;
}

bool
fs_reg::equals(const fs_reg &r) const
{
   return (file == r.file &&
           reg == r.reg &&
           reg_offset == r.reg_offset &&
           type == r.type &&
           negate == r.negate &&
           abs == r.abs &&
           memcmp(&fixed_hw_reg, &r.fixed_hw_reg,
                  sizeof(fixed_hw_reg)) == 0 &&
           smear == r.smear &&
           imm.u == r.imm.u);
}

int
fs_visitor::type_size(const struct glsl_type *type)
{
   unsigned int size, i;

   switch (type->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_BOOL:
      return type->components();
   case GLSL_TYPE_ARRAY:
      return type_size(type->fields.array) * type->length;
   case GLSL_TYPE_STRUCT:
      size = 0;
      for (i = 0; i < type->length; i++) {
	 size += type_size(type->fields.structure[i].type);
      }
      return size;
   case GLSL_TYPE_SAMPLER:
      /* Samplers take up no register space, since they're baked in at
       * link time.
       */
      return 0;
   default:
      assert(!"not reached");
      return 0;
   }
}

void
fs_visitor::fail(const char *format, ...)
{
   va_list va;
   char *msg;

   if (failed)
      return;

   failed = true;

   va_start(va, format);
   msg = ralloc_vasprintf(mem_ctx, format, va);
   va_end(va);
   msg = ralloc_asprintf(mem_ctx, "FS compile failed: %s\n", msg);

   this->fail_msg = msg;

   if (INTEL_DEBUG & DEBUG_WM) {
      fprintf(stderr, "%s",  msg);
   }
}

fs_inst *
fs_visitor::emit(enum opcode opcode)
{
   return emit(fs_inst(opcode));
}

fs_inst *
fs_visitor::emit(enum opcode opcode, fs_reg dst)
{
   return emit(fs_inst(opcode, dst));
}

fs_inst *
fs_visitor::emit(enum opcode opcode, fs_reg dst, fs_reg src0)
{
   return emit(fs_inst(opcode, dst, src0));
}

fs_inst *
fs_visitor::emit(enum opcode opcode, fs_reg dst, fs_reg src0, fs_reg src1)
{
   return emit(fs_inst(opcode, dst, src0, src1));
}

fs_inst *
fs_visitor::emit(enum opcode opcode, fs_reg dst,
                 fs_reg src0, fs_reg src1, fs_reg src2)
{
   return emit(fs_inst(opcode, dst, src0, src1, src2));
}

void
fs_visitor::push_force_uncompressed()
{
   force_uncompressed_stack++;
}

void
fs_visitor::pop_force_uncompressed()
{
   force_uncompressed_stack--;
   assert(force_uncompressed_stack >= 0);
}

void
fs_visitor::push_force_sechalf()
{
   force_sechalf_stack++;
}

void
fs_visitor::pop_force_sechalf()
{
   force_sechalf_stack--;
   assert(force_sechalf_stack >= 0);
}

/**
 * Returns how many MRFs an FS opcode will write over.
 *
 * Note that this is not the 0 or 1 implied writes in an actual gen
 * instruction -- the FS opcodes often generate MOVs in addition.
 */
int
fs_visitor::implied_mrf_writes(fs_inst *inst)
{
   if (inst->mlen == 0)
      return 0;

   switch (inst->opcode) {
   case SHADER_OPCODE_RCP:
   case SHADER_OPCODE_RSQ:
   case SHADER_OPCODE_SQRT:
   case SHADER_OPCODE_EXP2:
   case SHADER_OPCODE_LOG2:
   case SHADER_OPCODE_SIN:
   case SHADER_OPCODE_COS:
      return 1 * c->dispatch_width / 8;
   case SHADER_OPCODE_POW:
   case SHADER_OPCODE_INT_QUOTIENT:
   case SHADER_OPCODE_INT_REMAINDER:
      return 2 * c->dispatch_width / 8;
   case SHADER_OPCODE_TEX:
   case FS_OPCODE_TXB:
   case SHADER_OPCODE_TXD:
   case SHADER_OPCODE_TXF:
   case SHADER_OPCODE_TXL:
   case SHADER_OPCODE_TXS:
      return 1;
   case FS_OPCODE_FB_WRITE:
      return 2;
   case FS_OPCODE_PULL_CONSTANT_LOAD:
   case FS_OPCODE_UNSPILL:
      return 1;
   case FS_OPCODE_SPILL:
      return 2;
   default:
      assert(!"not reached");
      return inst->mlen;
   }
}

int
fs_visitor::virtual_grf_alloc(int size)
{
   if (virtual_grf_array_size <= virtual_grf_count) {
      if (virtual_grf_array_size == 0)
	 virtual_grf_array_size = 16;
      else
	 virtual_grf_array_size *= 2;
      virtual_grf_sizes = reralloc(mem_ctx, virtual_grf_sizes, int,
				   virtual_grf_array_size);
   }
   virtual_grf_sizes[virtual_grf_count] = size;
   return virtual_grf_count++;
}

/** Fixed HW reg constructor. */
fs_reg::fs_reg(enum register_file file, int reg)
{
   init();
   this->file = file;
   this->reg = reg;
   this->type = BRW_REGISTER_TYPE_F;
}

/** Fixed HW reg constructor. */
fs_reg::fs_reg(enum register_file file, int reg, uint32_t type)
{
   init();
   this->file = file;
   this->reg = reg;
   this->type = type;
}

/** Automatic reg constructor. */
fs_reg::fs_reg(class fs_visitor *v, const struct glsl_type *type)
{
   init();

   this->file = GRF;
   this->reg = v->virtual_grf_alloc(v->type_size(type));
   this->reg_offset = 0;
   this->type = brw_type_for_base_type(type);
}

fs_reg *
fs_visitor::variable_storage(ir_variable *var)
{
   return (fs_reg *)hash_table_find(this->variable_ht, var);
}

void
import_uniforms_callback(const void *key,
			 void *data,
			 void *closure)
{
   struct hash_table *dst_ht = (struct hash_table *)closure;
   const fs_reg *reg = (const fs_reg *)data;

   if (reg->file != UNIFORM)
      return;

   hash_table_insert(dst_ht, data, key);
}

/* For 16-wide, we need to follow from the uniform setup of 8-wide dispatch.
 * This brings in those uniform definitions
 */
void
fs_visitor::import_uniforms(fs_visitor *v)
{
   hash_table_call_foreach(v->variable_ht,
			   import_uniforms_callback,
			   variable_ht);
   this->params_remap = v->params_remap;
}

/* Our support for uniforms is piggy-backed on the struct
 * gl_fragment_program, because that's where the values actually
 * get stored, rather than in some global gl_shader_program uniform
 * store.
 */
int
fs_visitor::setup_uniform_values(int loc, const glsl_type *type)
{
   unsigned int offset = 0;

   if (type->is_matrix()) {
      const glsl_type *column = glsl_type::get_instance(GLSL_TYPE_FLOAT,
							type->vector_elements,
							1);

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
	 unsigned int param = c->prog_data.nr_params++;

	 assert(param < ARRAY_SIZE(c->prog_data.param));

	 this->param_index[param] = loc;
	 this->param_offset[param] = i;
      }
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


/* Our support for builtin uniforms is even scarier than non-builtin.
 * It sits on top of the PROG_STATE_VAR parameters that are
 * automatically updated from GL context state.
 */
void
fs_visitor::setup_builtin_uniform_values(ir_variable *ir)
{
   const ir_state_slot *const slots = ir->state_slots;
   assert(ir->state_slots != NULL);

   for (unsigned int i = 0; i < ir->num_state_slots; i++) {
      /* This state reference has already been setup by ir_to_mesa, but we'll
       * get the same index back here.
       */
      int index = _mesa_add_state_reference(this->fp->Base.Parameters,
					    (gl_state_index *)slots[i].tokens);

      /* Add each of the unique swizzles of the element as a parameter.
       * This'll end up matching the expected layout of the
       * array/matrix/structure we're trying to fill in.
       */
      int last_swiz = -1;
      for (unsigned int j = 0; j < 4; j++) {
	 int swiz = GET_SWZ(slots[i].swizzle, j);
	 if (swiz == last_swiz)
	    break;
	 last_swiz = swiz;

	 this->param_index[c->prog_data.nr_params] = index;
	 this->param_offset[c->prog_data.nr_params] = swiz;
	 c->prog_data.nr_params++;
      }
   }
}

fs_reg *
fs_visitor::emit_fragcoord_interpolation(ir_variable *ir)
{
   fs_reg *reg = new(this->mem_ctx) fs_reg(this, ir->type);
   fs_reg wpos = *reg;
   bool flip = !ir->origin_upper_left ^ c->key.render_to_fbo;

   /* gl_FragCoord.x */
   if (ir->pixel_center_integer) {
      emit(BRW_OPCODE_MOV, wpos, this->pixel_x);
   } else {
      emit(BRW_OPCODE_ADD, wpos, this->pixel_x, fs_reg(0.5f));
   }
   wpos.reg_offset++;

   /* gl_FragCoord.y */
   if (!flip && ir->pixel_center_integer) {
      emit(BRW_OPCODE_MOV, wpos, this->pixel_y);
   } else {
      fs_reg pixel_y = this->pixel_y;
      float offset = (ir->pixel_center_integer ? 0.0 : 0.5);

      if (flip) {
	 pixel_y.negate = true;
	 offset += c->key.drawable_height - 1.0;
      }

      emit(BRW_OPCODE_ADD, wpos, pixel_y, fs_reg(offset));
   }
   wpos.reg_offset++;

   /* gl_FragCoord.z */
   if (intel->gen >= 6) {
      emit(BRW_OPCODE_MOV, wpos,
	   fs_reg(brw_vec8_grf(c->source_depth_reg, 0)));
   } else {
      emit(FS_OPCODE_LINTERP, wpos,
           this->delta_x[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC],
           this->delta_y[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC],
           interp_reg(FRAG_ATTRIB_WPOS, 2));
   }
   wpos.reg_offset++;

   /* gl_FragCoord.w: Already set up in emit_interpolation */
   emit(BRW_OPCODE_MOV, wpos, this->wpos_w);

   return reg;
}

fs_inst *
fs_visitor::emit_linterp(const fs_reg &attr, const fs_reg &interp,
                         glsl_interp_qualifier interpolation_mode,
                         bool is_centroid)
{
   brw_wm_barycentric_interp_mode barycoord_mode;
   if (is_centroid) {
      if (interpolation_mode == INTERP_QUALIFIER_SMOOTH)
         barycoord_mode = BRW_WM_PERSPECTIVE_CENTROID_BARYCENTRIC;
      else
         barycoord_mode = BRW_WM_NONPERSPECTIVE_CENTROID_BARYCENTRIC;
   } else {
      if (interpolation_mode == INTERP_QUALIFIER_SMOOTH)
         barycoord_mode = BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC;
      else
         barycoord_mode = BRW_WM_NONPERSPECTIVE_PIXEL_BARYCENTRIC;
   }
   return emit(FS_OPCODE_LINTERP, attr,
               this->delta_x[barycoord_mode],
               this->delta_y[barycoord_mode], interp);
}

fs_reg *
fs_visitor::emit_general_interpolation(ir_variable *ir)
{
   fs_reg *reg = new(this->mem_ctx) fs_reg(this, ir->type);
   reg->type = brw_type_for_base_type(ir->type->get_scalar_type());
   fs_reg attr = *reg;

   unsigned int array_elements;
   const glsl_type *type;

   if (ir->type->is_array()) {
      array_elements = ir->type->length;
      if (array_elements == 0) {
	 fail("dereferenced array '%s' has length 0\n", ir->name);
      }
      type = ir->type->fields.array;
   } else {
      array_elements = 1;
      type = ir->type;
   }

   glsl_interp_qualifier interpolation_mode =
      ir->determine_interpolation_mode(c->key.flat_shade);

   int location = ir->location;
   for (unsigned int i = 0; i < array_elements; i++) {
      for (unsigned int j = 0; j < type->matrix_columns; j++) {
	 if (urb_setup[location] == -1) {
	    /* If there's no incoming setup data for this slot, don't
	     * emit interpolation for it.
	     */
	    attr.reg_offset += type->vector_elements;
	    location++;
	    continue;
	 }

	 if (interpolation_mode == INTERP_QUALIFIER_FLAT) {
	    /* Constant interpolation (flat shading) case. The SF has
	     * handed us defined values in only the constant offset
	     * field of the setup reg.
	     */
	    for (unsigned int k = 0; k < type->vector_elements; k++) {
	       struct brw_reg interp = interp_reg(location, k);
	       interp = suboffset(interp, 3);
               interp.type = reg->type;
	       emit(FS_OPCODE_CINTERP, attr, fs_reg(interp));
	       attr.reg_offset++;
	    }
	 } else {
	    /* Smooth/noperspective interpolation case. */
	    for (unsigned int k = 0; k < type->vector_elements; k++) {
	       /* FINISHME: At some point we probably want to push
		* this farther by giving similar treatment to the
		* other potentially constant components of the
		* attribute, as well as making brw_vs_constval.c
		* handle varyings other than gl_TexCoord.
		*/
	       if (location >= FRAG_ATTRIB_TEX0 &&
		   location <= FRAG_ATTRIB_TEX7 &&
		   k == 3 && !(c->key.proj_attrib_mask & (1 << location))) {
		  emit(BRW_OPCODE_MOV, attr, fs_reg(1.0f));
	       } else {
		  struct brw_reg interp = interp_reg(location, k);
                  emit_linterp(attr, fs_reg(interp), interpolation_mode,
                               ir->centroid);
                  if (brw->needs_unlit_centroid_workaround && ir->centroid) {
                     /* Get the pixel/sample mask into f0 so that we know
                      * which pixels are lit.  Then, for each channel that is
                      * unlit, replace the centroid data with non-centroid
                      * data.
                      */
                     emit(FS_OPCODE_MOV_DISPATCH_TO_FLAGS, attr);
                     fs_inst *inst = emit_linterp(attr, fs_reg(interp),
                                                  interpolation_mode, false);
                     inst->predicated = true;
                     inst->predicate_inverse = true;
                  }
		  if (intel->gen < 6) {
		     emit(BRW_OPCODE_MUL, attr, attr, this->pixel_w);
		  }
	       }
	       attr.reg_offset++;
	    }

	 }
	 location++;
      }
   }

   return reg;
}

fs_reg *
fs_visitor::emit_frontfacing_interpolation(ir_variable *ir)
{
   fs_reg *reg = new(this->mem_ctx) fs_reg(this, ir->type);

   /* The frontfacing comes in as a bit in the thread payload. */
   if (intel->gen >= 6) {
      emit(BRW_OPCODE_ASR, *reg,
	   fs_reg(retype(brw_vec1_grf(0, 0), BRW_REGISTER_TYPE_D)),
	   fs_reg(15));
      emit(BRW_OPCODE_NOT, *reg, *reg);
      emit(BRW_OPCODE_AND, *reg, *reg, fs_reg(1));
   } else {
      struct brw_reg r1_6ud = retype(brw_vec1_grf(1, 6), BRW_REGISTER_TYPE_UD);
      /* bit 31 is "primitive is back face", so checking < (1 << 31) gives
       * us front face
       */
      fs_inst *inst = emit(BRW_OPCODE_CMP, *reg,
			   fs_reg(r1_6ud),
			   fs_reg(1u << 31));
      inst->conditional_mod = BRW_CONDITIONAL_L;
      emit(BRW_OPCODE_AND, *reg, *reg, fs_reg(1u));
   }

   return reg;
}

fs_inst *
fs_visitor::emit_math(enum opcode opcode, fs_reg dst, fs_reg src)
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
      return NULL;
   }

   /* Can't do hstride == 0 args to gen6 math, so expand it out.  We
    * might be able to do better by doing execsize = 1 math and then
    * expanding that result out, but we would need to be careful with
    * masking.
    *
    * Gen 6 hardware ignores source modifiers (negate and abs) on math
    * instructions, so we also move to a temp to set those up.
    */
   if (intel->gen == 6 && (src.file == UNIFORM ||
			   src.abs ||
			   src.negate)) {
      fs_reg expanded = fs_reg(this, glsl_type::float_type);
      emit(BRW_OPCODE_MOV, expanded, src);
      src = expanded;
   }

   fs_inst *inst = emit(opcode, dst, src);

   if (intel->gen < 6) {
      inst->base_mrf = 2;
      inst->mlen = c->dispatch_width / 8;
   }

   return inst;
}

fs_inst *
fs_visitor::emit_math(enum opcode opcode, fs_reg dst, fs_reg src0, fs_reg src1)
{
   int base_mrf = 2;
   fs_inst *inst;

   switch (opcode) {
   case SHADER_OPCODE_POW:
   case SHADER_OPCODE_INT_QUOTIENT:
   case SHADER_OPCODE_INT_REMAINDER:
      break;
   default:
      assert(!"not reached: unsupported binary math opcode.");
      return NULL;
   }

   if (intel->gen >= 7) {
      inst = emit(opcode, dst, src0, src1);
   } else if (intel->gen == 6) {
      /* Can't do hstride == 0 args to gen6 math, so expand it out.
       *
       * The hardware ignores source modifiers (negate and abs) on math
       * instructions, so we also move to a temp to set those up.
       */
      if (src0.file == UNIFORM || src0.abs || src0.negate) {
	 fs_reg expanded = fs_reg(this, glsl_type::float_type);
	 expanded.type = src0.type;
	 emit(BRW_OPCODE_MOV, expanded, src0);
	 src0 = expanded;
      }

      if (src1.file == UNIFORM || src1.abs || src1.negate) {
	 fs_reg expanded = fs_reg(this, glsl_type::float_type);
	 expanded.type = src1.type;
	 emit(BRW_OPCODE_MOV, expanded, src1);
	 src1 = expanded;
      }

      inst = emit(opcode, dst, src0, src1);
   } else {
      /* From the Ironlake PRM, Volume 4, Part 1, Section 6.1.13
       * "Message Payload":
       *
       * "Operand0[7].  For the INT DIV functions, this operand is the
       *  denominator."
       *  ...
       * "Operand1[7].  For the INT DIV functions, this operand is the
       *  numerator."
       */
      bool is_int_div = opcode != SHADER_OPCODE_POW;
      fs_reg &op0 = is_int_div ? src1 : src0;
      fs_reg &op1 = is_int_div ? src0 : src1;

      emit(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + 1, op1.type), op1);
      inst = emit(opcode, dst, op0, reg_null_f);

      inst->base_mrf = base_mrf;
      inst->mlen = 2 * c->dispatch_width / 8;
   }
   return inst;
}

/**
 * To be called after the last _mesa_add_state_reference() call, to
 * set up prog_data.param[] for assign_curb_setup() and
 * setup_pull_constants().
 */
void
fs_visitor::setup_paramvalues_refs()
{
   if (c->dispatch_width != 8)
      return;

   /* Set up the pointers to ParamValues now that that array is finalized. */
   for (unsigned int i = 0; i < c->prog_data.nr_params; i++) {
      c->prog_data.param[i] =
	 (const float *)fp->Base.Parameters->ParameterValues[this->param_index[i]] +
	 this->param_offset[i];
   }
}

void
fs_visitor::assign_curb_setup()
{
   c->prog_data.curb_read_length = ALIGN(c->prog_data.nr_params, 8) / 8;
   if (c->dispatch_width == 8) {
      c->prog_data.first_curbe_grf = c->nr_payload_regs;
   } else {
      c->prog_data.first_curbe_grf_16 = c->nr_payload_regs;
   }

   /* Map the offsets in the UNIFORM file to fixed HW regs. */
   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      for (unsigned int i = 0; i < 3; i++) {
	 if (inst->src[i].file == UNIFORM) {
	    int constant_nr = inst->src[i].reg + inst->src[i].reg_offset;
	    struct brw_reg brw_reg = brw_vec1_grf(c->nr_payload_regs +
						  constant_nr / 8,
						  constant_nr % 8);

	    inst->src[i].file = FIXED_HW_REG;
	    inst->src[i].fixed_hw_reg = retype(brw_reg, inst->src[i].type);
	 }
      }
   }
}

void
fs_visitor::calculate_urb_setup()
{
   for (unsigned int i = 0; i < FRAG_ATTRIB_MAX; i++) {
      urb_setup[i] = -1;
   }

   int urb_next = 0;
   /* Figure out where each of the incoming setup attributes lands. */
   if (intel->gen >= 6) {
      for (unsigned int i = 0; i < FRAG_ATTRIB_MAX; i++) {
	 if (fp->Base.InputsRead & BITFIELD64_BIT(i)) {
	    urb_setup[i] = urb_next++;
	 }
      }
   } else {
      /* FINISHME: The sf doesn't map VS->FS inputs for us very well. */
      for (unsigned int i = 0; i < VERT_RESULT_MAX; i++) {
         /* Point size is packed into the header, not as a general attribute */
         if (i == VERT_RESULT_PSIZ)
            continue;

	 if (c->key.vp_outputs_written & BITFIELD64_BIT(i)) {
	    int fp_index = _mesa_vert_result_to_frag_attrib((gl_vert_result) i);

	    /* The back color slot is skipped when the front color is
	     * also written to.  In addition, some slots can be
	     * written in the vertex shader and not read in the
	     * fragment shader.  So the register number must always be
	     * incremented, mapped or not.
	     */
	    if (fp_index >= 0)
	       urb_setup[fp_index] = urb_next;
            urb_next++;
	 }
      }

      /*
       * It's a FS only attribute, and we did interpolation for this attribute
       * in SF thread. So, count it here, too.
       *
       * See compile_sf_prog() for more info.
       */
      if (fp->Base.InputsRead & BITFIELD64_BIT(FRAG_ATTRIB_PNTC))
         urb_setup[FRAG_ATTRIB_PNTC] = urb_next++;
   }

   /* Each attribute is 4 setup channels, each of which is half a reg. */
   c->prog_data.urb_read_length = urb_next * 2;
}

void
fs_visitor::assign_urb_setup()
{
   int urb_start = c->nr_payload_regs + c->prog_data.curb_read_length;

   /* Offset all the urb_setup[] index by the actual position of the
    * setup regs, now that the location of the constants has been chosen.
    */
   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      if (inst->opcode == FS_OPCODE_LINTERP) {
	 assert(inst->src[2].file == FIXED_HW_REG);
	 inst->src[2].fixed_hw_reg.nr += urb_start;
      }

      if (inst->opcode == FS_OPCODE_CINTERP) {
	 assert(inst->src[0].file == FIXED_HW_REG);
	 inst->src[0].fixed_hw_reg.nr += urb_start;
      }
   }

   this->first_non_payload_grf = urb_start + c->prog_data.urb_read_length;
}

/**
 * Split large virtual GRFs into separate components if we can.
 *
 * This is mostly duplicated with what brw_fs_vector_splitting does,
 * but that's really conservative because it's afraid of doing
 * splitting that doesn't result in real progress after the rest of
 * the optimization phases, which would cause infinite looping in
 * optimization.  We can do it once here, safely.  This also has the
 * opportunity to split interpolated values, or maybe even uniforms,
 * which we don't have at the IR level.
 *
 * We want to split, because virtual GRFs are what we register
 * allocate and spill (due to contiguousness requirements for some
 * instructions), and they're what we naturally generate in the
 * codegen process, but most virtual GRFs don't actually need to be
 * contiguous sets of GRFs.  If we split, we'll end up with reduced
 * live intervals and better dead code elimination and coalescing.
 */
void
fs_visitor::split_virtual_grfs()
{
   int num_vars = this->virtual_grf_count;
   bool split_grf[num_vars];
   int new_virtual_grf[num_vars];

   /* Try to split anything > 0 sized. */
   for (int i = 0; i < num_vars; i++) {
      if (this->virtual_grf_sizes[i] != 1)
	 split_grf[i] = true;
      else
	 split_grf[i] = false;
   }

   if (brw->has_pln &&
       this->delta_x[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC].file == GRF) {
      /* PLN opcodes rely on the delta_xy being contiguous.  We only have to
       * check this for BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC, because prior to
       * Gen6, that was the only supported interpolation mode, and since Gen6,
       * delta_x and delta_y are in fixed hardware registers.
       */
      split_grf[this->delta_x[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC].reg] =
         false;
   }

   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      /* If there's a SEND message that requires contiguous destination
       * registers, no splitting is allowed.
       */
      if (inst->regs_written() > 1) {
	 split_grf[inst->dst.reg] = false;
      }
   }

   /* Allocate new space for split regs.  Note that the virtual
    * numbers will be contiguous.
    */
   for (int i = 0; i < num_vars; i++) {
      if (split_grf[i]) {
	 new_virtual_grf[i] = virtual_grf_alloc(1);
	 for (int j = 2; j < this->virtual_grf_sizes[i]; j++) {
	    int reg = virtual_grf_alloc(1);
	    assert(reg == new_virtual_grf[i] + j - 1);
	    (void) reg;
	 }
	 this->virtual_grf_sizes[i] = 1;
      }
   }

   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      if (inst->dst.file == GRF &&
	  split_grf[inst->dst.reg] &&
	  inst->dst.reg_offset != 0) {
	 inst->dst.reg = (new_virtual_grf[inst->dst.reg] +
			  inst->dst.reg_offset - 1);
	 inst->dst.reg_offset = 0;
      }
      for (int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF &&
	     split_grf[inst->src[i].reg] &&
	     inst->src[i].reg_offset != 0) {
	    inst->src[i].reg = (new_virtual_grf[inst->src[i].reg] +
				inst->src[i].reg_offset - 1);
	    inst->src[i].reg_offset = 0;
	 }
      }
   }
   this->live_intervals_valid = false;
}

bool
fs_visitor::remove_dead_constants()
{
   if (c->dispatch_width == 8) {
      this->params_remap = ralloc_array(mem_ctx, int, c->prog_data.nr_params);

      for (unsigned int i = 0; i < c->prog_data.nr_params; i++)
	 this->params_remap[i] = -1;

      /* Find which params are still in use. */
      foreach_list(node, &this->instructions) {
	 fs_inst *inst = (fs_inst *)node;

	 for (int i = 0; i < 3; i++) {
	    int constant_nr = inst->src[i].reg + inst->src[i].reg_offset;

	    if (inst->src[i].file != UNIFORM)
	       continue;

	    assert(constant_nr < (int)c->prog_data.nr_params);

	    /* For now, set this to non-negative.  We'll give it the
	     * actual new number in a moment, in order to keep the
	     * register numbers nicely ordered.
	     */
	    this->params_remap[constant_nr] = 0;
	 }
      }

      /* Figure out what the new numbers for the params will be.  At some
       * point when we're doing uniform array access, we're going to want
       * to keep the distinction between .reg and .reg_offset, but for
       * now we don't care.
       */
      unsigned int new_nr_params = 0;
      for (unsigned int i = 0; i < c->prog_data.nr_params; i++) {
	 if (this->params_remap[i] != -1) {
	    this->params_remap[i] = new_nr_params++;
	 }
      }

      /* Update the list of params to be uploaded to match our new numbering. */
      for (unsigned int i = 0; i < c->prog_data.nr_params; i++) {
	 int remapped = this->params_remap[i];

	 if (remapped == -1)
	    continue;

	 /* We've already done setup_paramvalues_refs() so no need to worry
	  * about param_index and param_offset.
	  */
	 c->prog_data.param[remapped] = c->prog_data.param[i];
      }

      c->prog_data.nr_params = new_nr_params;
   } else {
      /* This should have been generated in the 8-wide pass already. */
      assert(this->params_remap);
   }

   /* Now do the renumbering of the shader to remove unused params. */
   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      for (int i = 0; i < 3; i++) {
	 int constant_nr = inst->src[i].reg + inst->src[i].reg_offset;

	 if (inst->src[i].file != UNIFORM)
	    continue;

	 assert(this->params_remap[constant_nr] != -1);
	 inst->src[i].reg = this->params_remap[constant_nr];
	 inst->src[i].reg_offset = 0;
      }
   }

   return true;
}

/**
 * Choose accesses from the UNIFORM file to demote to using the pull
 * constant buffer.
 *
 * We allow a fragment shader to have more than the specified minimum
 * maximum number of fragment shader uniform components (64).  If
 * there are too many of these, they'd fill up all of register space.
 * So, this will push some of them out to the pull constant buffer and
 * update the program to load them.
 */
void
fs_visitor::setup_pull_constants()
{
   /* Only allow 16 registers (128 uniform components) as push constants. */
   unsigned int max_uniform_components = 16 * 8;
   if (c->prog_data.nr_params <= max_uniform_components)
      return;

   if (c->dispatch_width == 16) {
      fail("Pull constants not supported in 16-wide\n");
      return;
   }

   /* Just demote the end of the list.  We could probably do better
    * here, demoting things that are rarely used in the program first.
    */
   int pull_uniform_base = max_uniform_components;
   int pull_uniform_count = c->prog_data.nr_params - pull_uniform_base;

   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      for (int i = 0; i < 3; i++) {
	 if (inst->src[i].file != UNIFORM)
	    continue;

	 int uniform_nr = inst->src[i].reg + inst->src[i].reg_offset;
	 if (uniform_nr < pull_uniform_base)
	    continue;

	 fs_reg dst = fs_reg(this, glsl_type::float_type);
	 fs_reg index = fs_reg((unsigned)SURF_INDEX_FRAG_CONST_BUFFER);
	 fs_reg offset = fs_reg((unsigned)(((uniform_nr -
					     pull_uniform_base) * 4) & ~15));
	 fs_inst *pull = new(mem_ctx) fs_inst(FS_OPCODE_PULL_CONSTANT_LOAD,
					      dst, index, offset);
	 pull->ir = inst->ir;
	 pull->annotation = inst->annotation;
	 pull->base_mrf = 14;
	 pull->mlen = 1;

	 inst->insert_before(pull);

	 inst->src[i].file = GRF;
	 inst->src[i].reg = dst.reg;
	 inst->src[i].reg_offset = 0;
	 inst->src[i].smear = (uniform_nr - pull_uniform_base) & 3;
      }
   }

   for (int i = 0; i < pull_uniform_count; i++) {
      c->prog_data.pull_param[i] = c->prog_data.param[pull_uniform_base + i];
   }
   c->prog_data.nr_params -= pull_uniform_count;
   c->prog_data.nr_pull_params = pull_uniform_count;
}

/**
 * Attempts to move immediate constants into the immediate
 * constant slot of following instructions.
 *
 * Immediate constants are a bit tricky -- they have to be in the last
 * operand slot, you can't do abs/negate on them,
 */

bool
fs_visitor::propagate_constants()
{
   bool progress = false;

   calculate_live_intervals();

   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      if (inst->opcode != BRW_OPCODE_MOV ||
	  inst->predicated ||
	  inst->dst.file != GRF || inst->src[0].file != IMM ||
	  inst->dst.type != inst->src[0].type ||
	  (c->dispatch_width == 16 &&
	   (inst->force_uncompressed || inst->force_sechalf)))
	 continue;

      /* Don't bother with cases where we should have had the
       * operation on the constant folded in GLSL already.
       */
      if (inst->saturate)
	 continue;

      /* Found a move of a constant to a GRF.  Find anything else using the GRF
       * before it's written, and replace it with the constant if we can.
       */
      for (fs_inst *scan_inst = (fs_inst *)inst->next;
	   !scan_inst->is_tail_sentinel();
	   scan_inst = (fs_inst *)scan_inst->next) {
	 if (scan_inst->opcode == BRW_OPCODE_DO ||
	     scan_inst->opcode == BRW_OPCODE_WHILE ||
	     scan_inst->opcode == BRW_OPCODE_ELSE ||
	     scan_inst->opcode == BRW_OPCODE_ENDIF) {
	    break;
	 }

	 for (int i = 2; i >= 0; i--) {
	    if (scan_inst->src[i].file != GRF ||
		scan_inst->src[i].reg != inst->dst.reg ||
		scan_inst->src[i].reg_offset != inst->dst.reg_offset)
	       continue;

	    /* Don't bother with cases where we should have had the
	     * operation on the constant folded in GLSL already.
	     */
	    if (scan_inst->src[i].negate || scan_inst->src[i].abs)
	       continue;

	    switch (scan_inst->opcode) {
	    case BRW_OPCODE_MOV:
	       scan_inst->src[i] = inst->src[0];
	       progress = true;
	       break;

	    case BRW_OPCODE_MUL:
	    case BRW_OPCODE_ADD:
	       if (i == 1) {
		  scan_inst->src[i] = inst->src[0];
		  progress = true;
	       } else if (i == 0 && scan_inst->src[1].file != IMM) {
		  /* Fit this constant in by commuting the operands.
		   * Exception: we can't do this for 32-bit integer MUL
		   * because it's asymmetric.
		   */
		  if (scan_inst->opcode == BRW_OPCODE_MUL &&
		      (scan_inst->src[1].type == BRW_REGISTER_TYPE_D ||
		       scan_inst->src[1].type == BRW_REGISTER_TYPE_UD))
		     break;
		  scan_inst->src[0] = scan_inst->src[1];
		  scan_inst->src[1] = inst->src[0];
		  progress = true;
	       }
	       break;

	    case BRW_OPCODE_CMP:
	    case BRW_OPCODE_IF:
	       if (i == 1) {
		  scan_inst->src[i] = inst->src[0];
		  progress = true;
	       } else if (i == 0 && scan_inst->src[1].file != IMM) {
		  uint32_t new_cmod;

		  new_cmod = brw_swap_cmod(scan_inst->conditional_mod);
		  if (new_cmod != ~0u) {
		     /* Fit this constant in by swapping the operands and
		      * flipping the test
		      */
		     scan_inst->src[0] = scan_inst->src[1];
		     scan_inst->src[1] = inst->src[0];
		     scan_inst->conditional_mod = new_cmod;
		     progress = true;
		  }
	       }
	       break;

	    case BRW_OPCODE_SEL:
	       if (i == 1) {
		  scan_inst->src[i] = inst->src[0];
		  progress = true;
	       } else if (i == 0 && scan_inst->src[1].file != IMM) {
		  scan_inst->src[0] = scan_inst->src[1];
		  scan_inst->src[1] = inst->src[0];

		  /* If this was predicated, flipping operands means
		   * we also need to flip the predicate.
		   */
		  if (scan_inst->conditional_mod == BRW_CONDITIONAL_NONE) {
		     scan_inst->predicate_inverse =
			!scan_inst->predicate_inverse;
		  }
		  progress = true;
	       }
	       break;

	    case SHADER_OPCODE_RCP:
	       /* The hardware doesn't do math on immediate values
		* (because why are you doing that, seriously?), but
		* the correct answer is to just constant fold it
		* anyway.
		*/
	       assert(i == 0);
	       if (inst->src[0].imm.f != 0.0f) {
		  scan_inst->opcode = BRW_OPCODE_MOV;
		  scan_inst->src[0] = inst->src[0];
		  scan_inst->src[0].imm.f = 1.0f / scan_inst->src[0].imm.f;
		  progress = true;
	       }
	       break;

            case FS_OPCODE_PULL_CONSTANT_LOAD:
	       scan_inst->src[i] = inst->src[0];
	       progress = true;
	       break;

	    default:
	       break;
	    }
	 }

	 if (scan_inst->dst.file == GRF &&
             scan_inst->overwrites_reg(inst->dst)) {
	    break;
	 }
      }
   }

   if (progress)
       this->live_intervals_valid = false;

   return progress;
}


/**
 * Attempts to move immediate constants into the immediate
 * constant slot of following instructions.
 *
 * Immediate constants are a bit tricky -- they have to be in the last
 * operand slot, you can't do abs/negate on them,
 */

bool
fs_visitor::opt_algebraic()
{
   bool progress = false;

   calculate_live_intervals();

   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      switch (inst->opcode) {
      case BRW_OPCODE_MUL:
	 if (inst->src[1].file != IMM)
	    continue;

	 /* a * 1.0 = a */
	 if (inst->src[1].type == BRW_REGISTER_TYPE_F &&
	     inst->src[1].imm.f == 1.0) {
	    inst->opcode = BRW_OPCODE_MOV;
	    inst->src[1] = reg_undef;
	    progress = true;
	    break;
	 }

	 break;
      default:
	 break;
      }
   }

   return progress;
}

/**
 * Must be called after calculate_live_intervales() to remove unused
 * writes to registers -- register allocation will fail otherwise
 * because something deffed but not used won't be considered to
 * interfere with other regs.
 */
bool
fs_visitor::dead_code_eliminate()
{
   bool progress = false;
   int pc = 0;

   calculate_live_intervals();

   foreach_list_safe(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      if (inst->dst.file == GRF && this->virtual_grf_use[inst->dst.reg] <= pc) {
	 inst->remove();
	 progress = true;
      }

      pc++;
   }

   if (progress)
      live_intervals_valid = false;

   return progress;
}

/**
 * Implements a second type of register coalescing: This one checks if
 * the two regs involved in a raw move don't interfere, in which case
 * they can both by stored in the same place and the MOV removed.
 */
bool
fs_visitor::register_coalesce_2()
{
   bool progress = false;

   calculate_live_intervals();

   foreach_list_safe(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      if (inst->opcode != BRW_OPCODE_MOV ||
	  inst->predicated ||
	  inst->saturate ||
	  inst->src[0].file != GRF ||
	  inst->src[0].negate ||
	  inst->src[0].abs ||
	  inst->src[0].smear != -1 ||
	  inst->dst.file != GRF ||
	  inst->dst.type != inst->src[0].type ||
	  virtual_grf_sizes[inst->src[0].reg] != 1 ||
	  virtual_grf_interferes(inst->dst.reg, inst->src[0].reg)) {
	 continue;
      }

      int reg_from = inst->src[0].reg;
      assert(inst->src[0].reg_offset == 0);
      int reg_to = inst->dst.reg;
      int reg_to_offset = inst->dst.reg_offset;

      foreach_list_safe(node, &this->instructions) {
	 fs_inst *scan_inst = (fs_inst *)node;

	 if (scan_inst->dst.file == GRF &&
	     scan_inst->dst.reg == reg_from) {
	    scan_inst->dst.reg = reg_to;
	    scan_inst->dst.reg_offset = reg_to_offset;
	 }
	 for (int i = 0; i < 3; i++) {
	    if (scan_inst->src[i].file == GRF &&
		scan_inst->src[i].reg == reg_from) {
	       scan_inst->src[i].reg = reg_to;
	       scan_inst->src[i].reg_offset = reg_to_offset;
	    }
	 }
      }

      inst->remove();
      live_intervals_valid = false;
      progress = true;
      continue;
   }

   return progress;
}

bool
fs_visitor::register_coalesce()
{
   bool progress = false;
   int if_depth = 0;
   int loop_depth = 0;

   foreach_list_safe(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      /* Make sure that we dominate the instructions we're going to
       * scan for interfering with our coalescing, or we won't have
       * scanned enough to see if anything interferes with our
       * coalescing.  We don't dominate the following instructions if
       * we're in a loop or an if block.
       */
      switch (inst->opcode) {
      case BRW_OPCODE_DO:
	 loop_depth++;
	 break;
      case BRW_OPCODE_WHILE:
	 loop_depth--;
	 break;
      case BRW_OPCODE_IF:
	 if_depth++;
	 break;
      case BRW_OPCODE_ENDIF:
	 if_depth--;
	 break;
      default:
	 break;
      }
      if (loop_depth || if_depth)
	 continue;

      if (inst->opcode != BRW_OPCODE_MOV ||
	  inst->predicated ||
	  inst->saturate ||
	  inst->dst.file != GRF || (inst->src[0].file != GRF &&
				    inst->src[0].file != UNIFORM)||
	  inst->dst.type != inst->src[0].type)
	 continue;

      bool has_source_modifiers = inst->src[0].abs || inst->src[0].negate;

      /* Found a move of a GRF to a GRF.  Let's see if we can coalesce
       * them: check for no writes to either one until the exit of the
       * program.
       */
      bool interfered = false;

      for (fs_inst *scan_inst = (fs_inst *)inst->next;
	   !scan_inst->is_tail_sentinel();
	   scan_inst = (fs_inst *)scan_inst->next) {
	 if (scan_inst->dst.file == GRF) {
	    if (scan_inst->overwrites_reg(inst->dst) ||
                scan_inst->overwrites_reg(inst->src[0])) {
	       interfered = true;
	       break;
	    }
	 }

	 /* The gen6 MATH instruction can't handle source modifiers or
	  * unusual register regions, so avoid coalescing those for
	  * now.  We should do something more specific.
	  */
	 if (intel->gen >= 6 &&
	     scan_inst->is_math() &&
	     (has_source_modifiers || inst->src[0].file == UNIFORM)) {
	    interfered = true;
	    break;
	 }

	 /* The accumulator result appears to get used for the
	  * conditional modifier generation.  When negating a UD
	  * value, there is a 33rd bit generated for the sign in the
	  * accumulator value, so now you can't check, for example,
	  * equality with a 32-bit value.  See piglit fs-op-neg-uint.
	  */
	 if (scan_inst->conditional_mod &&
	     inst->src[0].negate &&
	     inst->src[0].type == BRW_REGISTER_TYPE_UD) {
	    interfered = true;
	    break;
	 }
      }
      if (interfered) {
	 continue;
      }

      /* Rewrite the later usage to point at the source of the move to
       * be removed.
       */
      for (fs_inst *scan_inst = inst;
	   !scan_inst->is_tail_sentinel();
	   scan_inst = (fs_inst *)scan_inst->next) {
	 for (int i = 0; i < 3; i++) {
	    if (scan_inst->src[i].file == GRF &&
		scan_inst->src[i].reg == inst->dst.reg &&
		scan_inst->src[i].reg_offset == inst->dst.reg_offset) {
	       fs_reg new_src = inst->src[0];
               if (scan_inst->src[i].abs) {
                  new_src.negate = 0;
                  new_src.abs = 1;
               }
	       new_src.negate ^= scan_inst->src[i].negate;
	       scan_inst->src[i] = new_src;
	    }
	 }
      }

      inst->remove();
      progress = true;
   }

   if (progress)
      live_intervals_valid = false;

   return progress;
}


bool
fs_visitor::compute_to_mrf()
{
   bool progress = false;
   int next_ip = 0;

   calculate_live_intervals();

   foreach_list_safe(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      int ip = next_ip;
      next_ip++;

      if (inst->opcode != BRW_OPCODE_MOV ||
	  inst->predicated ||
	  inst->dst.file != MRF || inst->src[0].file != GRF ||
	  inst->dst.type != inst->src[0].type ||
	  inst->src[0].abs || inst->src[0].negate || inst->src[0].smear != -1)
	 continue;

      /* Work out which hardware MRF registers are written by this
       * instruction.
       */
      int mrf_low = inst->dst.reg & ~BRW_MRF_COMPR4;
      int mrf_high;
      if (inst->dst.reg & BRW_MRF_COMPR4) {
	 mrf_high = mrf_low + 4;
      } else if (c->dispatch_width == 16 &&
		 (!inst->force_uncompressed && !inst->force_sechalf)) {
	 mrf_high = mrf_low + 1;
      } else {
	 mrf_high = mrf_low;
      }

      /* Can't compute-to-MRF this GRF if someone else was going to
       * read it later.
       */
      if (this->virtual_grf_use[inst->src[0].reg] > ip)
	 continue;

      /* Found a move of a GRF to a MRF.  Let's see if we can go
       * rewrite the thing that made this GRF to write into the MRF.
       */
      fs_inst *scan_inst;
      for (scan_inst = (fs_inst *)inst->prev;
	   scan_inst->prev != NULL;
	   scan_inst = (fs_inst *)scan_inst->prev) {
	 if (scan_inst->dst.file == GRF &&
	     scan_inst->dst.reg == inst->src[0].reg) {
	    /* Found the last thing to write our reg we want to turn
	     * into a compute-to-MRF.
	     */

            /* SENDs can only write to GRFs, so no compute-to-MRF. */
	    if (scan_inst->mlen) {
	       break;
	    }

	    /* If it's predicated, it (probably) didn't populate all
	     * the channels.  We might be able to rewrite everything
	     * that writes that reg, but it would require smarter
	     * tracking to delay the rewriting until complete success.
	     */
	    if (scan_inst->predicated)
	       break;

	    /* If it's half of register setup and not the same half as
	     * our MOV we're trying to remove, bail for now.
	     */
	    if (scan_inst->force_uncompressed != inst->force_uncompressed ||
		scan_inst->force_sechalf != inst->force_sechalf) {
	       break;
	    }

	    /* SEND instructions can't have MRF as a destination. */
	    if (scan_inst->mlen)
	       break;

	    if (intel->gen >= 6) {
	       /* gen6 math instructions must have the destination be
		* GRF, so no compute-to-MRF for them.
		*/
	       if (scan_inst->is_math()) {
		  break;
	       }
	    }

	    if (scan_inst->dst.reg_offset == inst->src[0].reg_offset) {
	       /* Found the creator of our MRF's source value. */
	       scan_inst->dst.file = MRF;
	       scan_inst->dst.reg = inst->dst.reg;
	       scan_inst->saturate |= inst->saturate;
	       inst->remove();
	       progress = true;
	    }
	    break;
	 }

	 /* We don't handle flow control here.  Most computation of
	  * values that end up in MRFs are shortly before the MRF
	  * write anyway.
	  */
	 if (scan_inst->opcode == BRW_OPCODE_DO ||
	     scan_inst->opcode == BRW_OPCODE_WHILE ||
	     scan_inst->opcode == BRW_OPCODE_ELSE ||
	     scan_inst->opcode == BRW_OPCODE_ENDIF) {
	    break;
	 }

	 /* You can't read from an MRF, so if someone else reads our
	  * MRF's source GRF that we wanted to rewrite, that stops us.
	  */
	 bool interfered = false;
	 for (int i = 0; i < 3; i++) {
	    if (scan_inst->src[i].file == GRF &&
		scan_inst->src[i].reg == inst->src[0].reg &&
		scan_inst->src[i].reg_offset == inst->src[0].reg_offset) {
	       interfered = true;
	    }
	 }
	 if (interfered)
	    break;

	 if (scan_inst->dst.file == MRF) {
	    /* If somebody else writes our MRF here, we can't
	     * compute-to-MRF before that.
	     */
	    int scan_mrf_low = scan_inst->dst.reg & ~BRW_MRF_COMPR4;
	    int scan_mrf_high;

	    if (scan_inst->dst.reg & BRW_MRF_COMPR4) {
	       scan_mrf_high = scan_mrf_low + 4;
	    } else if (c->dispatch_width == 16 &&
		       (!scan_inst->force_uncompressed &&
			!scan_inst->force_sechalf)) {
	       scan_mrf_high = scan_mrf_low + 1;
	    } else {
	       scan_mrf_high = scan_mrf_low;
	    }

	    if (mrf_low == scan_mrf_low ||
		mrf_low == scan_mrf_high ||
		mrf_high == scan_mrf_low ||
		mrf_high == scan_mrf_high) {
	       break;
	    }
	 }

	 if (scan_inst->mlen > 0) {
	    /* Found a SEND instruction, which means that there are
	     * live values in MRFs from base_mrf to base_mrf +
	     * scan_inst->mlen - 1.  Don't go pushing our MRF write up
	     * above it.
	     */
	    if (mrf_low >= scan_inst->base_mrf &&
		mrf_low < scan_inst->base_mrf + scan_inst->mlen) {
	       break;
	    }
	    if (mrf_high >= scan_inst->base_mrf &&
		mrf_high < scan_inst->base_mrf + scan_inst->mlen) {
	       break;
	    }
	 }
      }
   }

   if (progress)
      live_intervals_valid = false;

   return progress;
}

/**
 * Walks through basic blocks, looking for repeated MRF writes and
 * removing the later ones.
 */
bool
fs_visitor::remove_duplicate_mrf_writes()
{
   fs_inst *last_mrf_move[16];
   bool progress = false;

   /* Need to update the MRF tracking for compressed instructions. */
   if (c->dispatch_width == 16)
      return false;

   memset(last_mrf_move, 0, sizeof(last_mrf_move));

   foreach_list_safe(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      switch (inst->opcode) {
      case BRW_OPCODE_DO:
      case BRW_OPCODE_WHILE:
      case BRW_OPCODE_IF:
      case BRW_OPCODE_ELSE:
      case BRW_OPCODE_ENDIF:
	 memset(last_mrf_move, 0, sizeof(last_mrf_move));
	 continue;
      default:
	 break;
      }

      if (inst->opcode == BRW_OPCODE_MOV &&
	  inst->dst.file == MRF) {
	 fs_inst *prev_inst = last_mrf_move[inst->dst.reg];
	 if (prev_inst && inst->equals(prev_inst)) {
	    inst->remove();
	    progress = true;
	    continue;
	 }
      }

      /* Clear out the last-write records for MRFs that were overwritten. */
      if (inst->dst.file == MRF) {
	 last_mrf_move[inst->dst.reg] = NULL;
      }

      if (inst->mlen > 0) {
	 /* Found a SEND instruction, which will include two or fewer
	  * implied MRF writes.  We could do better here.
	  */
	 for (int i = 0; i < implied_mrf_writes(inst); i++) {
	    last_mrf_move[inst->base_mrf + i] = NULL;
	 }
      }

      /* Clear out any MRF move records whose sources got overwritten. */
      if (inst->dst.file == GRF) {
	 for (unsigned int i = 0; i < Elements(last_mrf_move); i++) {
	    if (last_mrf_move[i] &&
		last_mrf_move[i]->src[0].reg == inst->dst.reg) {
	       last_mrf_move[i] = NULL;
	    }
	 }
      }

      if (inst->opcode == BRW_OPCODE_MOV &&
	  inst->dst.file == MRF &&
	  inst->src[0].file == GRF &&
	  !inst->predicated) {
	 last_mrf_move[inst->dst.reg] = inst;
      }
   }

   if (progress)
      live_intervals_valid = false;

   return progress;
}

/**
 * Possibly returns an instruction that set up @param reg.
 *
 * Sometimes we want to take the result of some expression/variable
 * dereference tree and rewrite the instruction generating the result
 * of the tree.  When processing the tree, we know that the
 * instructions generated are all writing temporaries that are dead
 * outside of this tree.  So, if we have some instructions that write
 * a temporary, we're free to point that temp write somewhere else.
 *
 * Note that this doesn't guarantee that the instruction generated
 * only reg -- it might be the size=4 destination of a texture instruction.
 */
fs_inst *
fs_visitor::get_instruction_generating_reg(fs_inst *start,
					   fs_inst *end,
					   fs_reg reg)
{
   if (end == start ||
       end->predicated ||
       end->force_uncompressed ||
       end->force_sechalf ||
       !reg.equals(end->dst)) {
      return NULL;
   } else {
      return end;
   }
}

bool
fs_visitor::run()
{
   uint32_t prog_offset_16 = 0;
   uint32_t orig_nr_params = c->prog_data.nr_params;

   brw_wm_payload_setup(brw, c);

   if (c->dispatch_width == 16) {
      /* align to 64 byte boundary. */
      while ((c->func.nr_insn * sizeof(struct brw_instruction)) % 64) {
	 brw_NOP(p);
      }

      /* Save off the start of this 16-wide program in case we succeed. */
      prog_offset_16 = c->func.nr_insn * sizeof(struct brw_instruction);

      brw_set_compression_control(p, BRW_COMPRESSION_COMPRESSED);
   }

   if (0) {
      emit_dummy_fs();
   } else {
      calculate_urb_setup();
      if (intel->gen < 6)
	 emit_interpolation_setup_gen4();
      else
	 emit_interpolation_setup_gen6();

      /* Generate FS IR for main().  (the visitor only descends into
       * functions called "main").
       */
      foreach_list(node, &*shader->ir) {
	 ir_instruction *ir = (ir_instruction *)node;
	 base_ir = ir;
	 this->result = reg_undef;
	 ir->accept(this);
      }
      if (failed)
	 return false;

      emit_fb_writes();

      split_virtual_grfs();

      setup_paramvalues_refs();
      setup_pull_constants();

      bool progress;
      do {
	 progress = false;

	 progress = remove_duplicate_mrf_writes() || progress;

	 progress = propagate_constants() || progress;
	 progress = opt_algebraic() || progress;
	 progress = opt_cse() || progress;
	 progress = opt_copy_propagate() || progress;
	 progress = register_coalesce() || progress;
	 progress = register_coalesce_2() || progress;
	 progress = compute_to_mrf() || progress;
	 progress = dead_code_eliminate() || progress;
      } while (progress);

      remove_dead_constants();

      schedule_instructions();

      assign_curb_setup();
      assign_urb_setup();

      if (0) {
	 /* Debug of register spilling: Go spill everything. */
	 for (int i = 0; i < virtual_grf_count; i++) {
	    spill_reg(i);
	 }
      }

      if (0)
	 assign_regs_trivial();
      else {
	 while (!assign_regs()) {
	    if (failed)
	       break;
	 }
      }
   }
   assert(force_uncompressed_stack == 0);
   assert(force_sechalf_stack == 0);

   if (failed)
      return false;

   generate_code();

   if (c->dispatch_width == 8) {
      c->prog_data.reg_blocks = brw_register_blocks(grf_used);
   } else {
      c->prog_data.reg_blocks_16 = brw_register_blocks(grf_used);
      c->prog_data.prog_offset_16 = prog_offset_16;

      /* Make sure we didn't try to sneak in an extra uniform */
      assert(orig_nr_params == c->prog_data.nr_params);
      (void) orig_nr_params;
   }

   return !failed;
}

bool
brw_wm_fs_emit(struct brw_context *brw, struct brw_wm_compile *c,
	       struct gl_shader_program *prog)
{
   struct intel_context *intel = &brw->intel;
   bool start_busy = false;
   float start_time = 0;

   if (!prog)
      return false;

   if (unlikely(INTEL_DEBUG & DEBUG_PERF)) {
      start_busy = (intel->batch.last_bo &&
                    drm_intel_bo_busy(intel->batch.last_bo));
      start_time = get_time();
   }

   struct brw_shader *shader =
     (brw_shader *) prog->_LinkedShaders[MESA_SHADER_FRAGMENT];
   if (!shader)
      return false;

   if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
      printf("GLSL IR for native fragment shader %d:\n", prog->Name);
      _mesa_print_ir(shader->ir, NULL);
      printf("\n\n");
   }

   /* Now the main event: Visit the shader IR and generate our FS IR for it.
    */
   c->dispatch_width = 8;

   fs_visitor v(c, prog, shader);
   if (!v.run()) {
      prog->LinkStatus = false;
      ralloc_strcat(&prog->InfoLog, v.fail_msg);

      _mesa_problem(NULL, "Failed to compile fragment shader: %s\n",
		    v.fail_msg);

      return false;
   }

   if (intel->gen >= 5 && c->prog_data.nr_pull_params == 0) {
      c->dispatch_width = 16;
      fs_visitor v2(c, prog, shader);
      v2.import_uniforms(&v);
      if (!v2.run()) {
         perf_debug("16-wide shader failed to compile, falling back to "
                    "8-wide at a 10-20%% performance cost: %s", v2.fail_msg);
      }
   }

   c->prog_data.dispatch_width = 8;

   if (unlikely(INTEL_DEBUG & DEBUG_PERF)) {
      if (shader->compiled_once)
         brw_wm_debug_recompile(brw, prog, &c->key);
      shader->compiled_once = true;

      if (start_busy && !drm_intel_bo_busy(intel->batch.last_bo)) {
         perf_debug("FS compile took %.03f ms and stalled the GPU\n",
                    (get_time() - start_time) * 1000);
      }
   }

   return true;
}

bool
brw_fs_precompile(struct gl_context *ctx, struct gl_shader_program *prog)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_context *intel = &brw->intel;
   struct brw_wm_prog_key key;

   if (!prog->_LinkedShaders[MESA_SHADER_FRAGMENT])
      return true;

   struct gl_fragment_program *fp = (struct gl_fragment_program *)
      prog->_LinkedShaders[MESA_SHADER_FRAGMENT]->Program;
   struct brw_fragment_program *bfp = brw_fragment_program(fp);
   bool program_uses_dfdy = fp->UsesDFdy;

   memset(&key, 0, sizeof(key));

   if (intel->gen < 6) {
      if (fp->UsesKill)
         key.iz_lookup |= IZ_PS_KILL_ALPHATEST_BIT;

      if (fp->Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH))
         key.iz_lookup |= IZ_PS_COMPUTES_DEPTH_BIT;

      /* Just assume depth testing. */
      key.iz_lookup |= IZ_DEPTH_TEST_ENABLE_BIT;
      key.iz_lookup |= IZ_DEPTH_WRITE_ENABLE_BIT;
   }

   if (prog->Name != 0)
      key.proj_attrib_mask = 0xffffffff;

   if (intel->gen < 6)
      key.vp_outputs_written |= BITFIELD64_BIT(FRAG_ATTRIB_WPOS);

   for (int i = 0; i < FRAG_ATTRIB_MAX; i++) {
      if (!(fp->Base.InputsRead & BITFIELD64_BIT(i)))
	 continue;

      if (prog->Name == 0)
         key.proj_attrib_mask |= 1 << i;

      if (intel->gen < 6) {
         int vp_index = _mesa_vert_result_to_frag_attrib((gl_vert_result) i);

         if (vp_index >= 0)
            key.vp_outputs_written |= BITFIELD64_BIT(vp_index);
      }
   }

   key.clamp_fragment_color = true;

   for (int i = 0; i < MAX_SAMPLERS; i++) {
      if (fp->Base.ShadowSamplers & (1 << i)) {
         /* Assume DEPTH_TEXTURE_MODE is the default: X, X, X, 1 */
         key.tex.swizzles[i] =
            MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_ONE);
      } else {
         /* Color sampler: assume no swizzling. */
         key.tex.swizzles[i] = SWIZZLE_XYZW;
      }
   }

   if (fp->Base.InputsRead & FRAG_BIT_WPOS) {
      key.drawable_height = ctx->DrawBuffer->Height;
   }

   if ((fp->Base.InputsRead & FRAG_BIT_WPOS) || program_uses_dfdy) {
      key.render_to_fbo = _mesa_is_user_fbo(ctx->DrawBuffer);
   }

   key.nr_color_regions = 1;

   key.program_string_id = bfp->id;

   uint32_t old_prog_offset = brw->wm.prog_offset;
   struct brw_wm_prog_data *old_prog_data = brw->wm.prog_data;

   bool success = do_wm_prog(brw, prog, bfp, &key);

   brw->wm.prog_offset = old_prog_offset;
   brw->wm.prog_data = old_prog_data;

   return success;
}
