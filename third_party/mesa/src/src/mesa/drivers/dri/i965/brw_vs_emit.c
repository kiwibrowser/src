/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
            

#include "main/macros.h"
#include "program/program.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"
#include "brw_context.h"
#include "brw_vs.h"

/* Return the SrcReg index of the channels that can be immediate float operands
 * instead of usage of PROGRAM_CONSTANT values through push/pull.
 */
static bool
brw_vs_arg_can_be_immediate(enum prog_opcode opcode, int arg)
{
   int opcode_array[] = {
      [OPCODE_MOV] = 1,
      [OPCODE_ADD] = 2,
      [OPCODE_CMP] = 3,
      [OPCODE_DP2] = 2,
      [OPCODE_DP3] = 2,
      [OPCODE_DP4] = 2,
      [OPCODE_DPH] = 2,
      [OPCODE_MAX] = 2,
      [OPCODE_MIN] = 2,
      [OPCODE_MUL] = 2,
      [OPCODE_SEQ] = 2,
      [OPCODE_SGE] = 2,
      [OPCODE_SGT] = 2,
      [OPCODE_SLE] = 2,
      [OPCODE_SLT] = 2,
      [OPCODE_SNE] = 2,
      [OPCODE_XPD] = 2,
   };

   /* These opcodes get broken down in a way that allow two
    * args to be immediates.
    */
   if (opcode == OPCODE_MAD || opcode == OPCODE_LRP) {
      if (arg == 1 || arg == 2)
	 return true;
   }

   if (opcode > ARRAY_SIZE(opcode_array))
      return false;

   return arg == opcode_array[opcode] - 1;
}

static struct brw_reg get_tmp( struct brw_vs_compile *c )
{
   struct brw_reg tmp = brw_vec8_grf(c->last_tmp, 0);

   if (++c->last_tmp > c->prog_data.total_grf)
      c->prog_data.total_grf = c->last_tmp;

   return tmp;
}

static void release_tmp( struct brw_vs_compile *c, struct brw_reg tmp )
{
   if (tmp.nr == c->last_tmp-1)
      c->last_tmp--;
}
			       
static void release_tmps( struct brw_vs_compile *c )
{
   c->last_tmp = c->first_tmp;
}

static int
get_first_reladdr_output(struct gl_vertex_program *vp)
{
   int i;
   int first_reladdr_output = VERT_RESULT_MAX;

   for (i = 0; i < vp->Base.NumInstructions; i++) {
      struct prog_instruction *inst = vp->Base.Instructions + i;

      if (inst->DstReg.File == PROGRAM_OUTPUT &&
	  inst->DstReg.RelAddr &&
	  inst->DstReg.Index < first_reladdr_output)
	 first_reladdr_output = inst->DstReg.Index;
   }

   return first_reladdr_output;
}

/* Clears the record of which vp_const_buffer elements have been
 * loaded into our constant buffer registers, for the starts of new
 * blocks after control flow.
 */
static void
clear_current_const(struct brw_vs_compile *c)
{
   unsigned int i;

   if (c->vp->use_const_buffer) {
      for (i = 0; i < 3; i++) {
         c->current_const[i].index = -1;
      }
   }
}

/* The message length for all SEND messages is restricted to [1,15].  This
 * includes 1 for the header, so anything in slots 14 and above needs to be
 * placed in a general-purpose register and emitted using a second URB write.
 */
#define MAX_SLOTS_IN_FIRST_URB_WRITE 14

/**
 * Determine whether the given vertex output can be written directly to a MRF
 * or whether it has to be stored in a general-purpose register.
 */
static inline bool can_use_direct_mrf(int vert_result,
                                      int first_reladdr_output, int slot)
{
   if (vert_result == VERT_RESULT_HPOS || vert_result == VERT_RESULT_PSIZ) {
      /* These never go straight into MRF's.  They are placed in the MRF by
       * epilog code.
       */
      return false;
   }
   if (first_reladdr_output <= vert_result && vert_result < VERT_RESULT_MAX) {
      /* Relative addressing might be used to access this vert_result, so it
       * needs to go into a general-purpose register.
       */
      return false;
   }
   if (slot >= MAX_SLOTS_IN_FIRST_URB_WRITE) {
      /* This output won't go out until the second URB write so it must be
       * stored in a general-purpose register until then.
       */
      return false;
   }
   return true;
}

/**
 * Preallocate GRF register before code emit.
 * Do things as simply as possible.  Allocate and populate all regs
 * ahead of time.
 */
static void brw_vs_alloc_regs( struct brw_vs_compile *c )
{
   struct intel_context *intel = &c->func.brw->intel;
   GLuint i, reg = 0, slot;
   int attributes_in_vue;
   int first_reladdr_output;
   int max_constant;
   int constant = 0;
   struct brw_vertex_program *vp = c->vp;
   const struct gl_program_parameter_list *params = vp->program.Base.Parameters;

   /* Determine whether to use a real constant buffer or use a block
    * of GRF registers for constants.  The later is faster but only
    * works if everything fits in the GRF.
    * XXX this heuristic/check may need some fine tuning...
    */
   if (c->vp->program.Base.Parameters->NumParameters +
       c->vp->program.Base.NumTemporaries + 20 > BRW_MAX_GRF)
      c->vp->use_const_buffer = true;
   else
      c->vp->use_const_buffer = false;

   /*printf("use_const_buffer = %d\n", c->vp->use_const_buffer);*/

   /* r0 -- reserved as usual
    */
   c->r0 = brw_vec8_grf(reg, 0);
   reg++;

   /* User clip planes from curbe: 
    */
   if (c->key.userclip_active) {
      if (intel->gen >= 6) {
	 for (i = 0; i <= c->key.nr_userclip_plane_consts; i++) {
	    c->userplane[i] = stride(brw_vec4_grf(reg + i / 2,
						  (i % 2) * 4), 0, 4, 1);
	 }
	 reg += ALIGN(c->key.nr_userclip_plane_consts, 2) / 2;
      } else {
	 for (i = 0; i < c->key.nr_userclip_plane_consts; i++) {
	    c->userplane[i] = stride(brw_vec4_grf(reg + (6 + i) / 2,
						  (i % 2) * 4), 0, 4, 1);
	 }
	 reg += (ALIGN(6 + c->key.nr_userclip_plane_consts, 4) / 4) * 2;
      }

   }

   /* Assign some (probably all) of the vertex program constants to
    * the push constant buffer/CURBE.
    *
    * There's an obvious limit to the numer of push constants equal to
    * the number of register available, and that number is smaller
    * than the minimum maximum number of vertex program parameters, so
    * support for pull constants is required if we overflow.
    * Additionally, on gen6 the number of push constants is even
    * lower.
    *
    * When there's relative addressing, we don't know what range of
    * Mesa IR registers can be accessed.  And generally, when relative
    * addressing is used we also have too many constants to load them
    * all as push constants.  So, we'll just support relative
    * addressing out of the pull constant buffers, and try to load as
    * many statically-accessed constants into the push constant buffer
    * as we can.
    */
   if (intel->gen >= 6) {
      /* We can only load 32 regs of push constants. */
      max_constant = 32 * 2 - c->key.nr_userclip_plane_consts;
   } else {
      max_constant = BRW_MAX_GRF - 20 - c->vp->program.Base.NumTemporaries;
   }

   /* constant_map maps from ParameterValues[] index to index in the
    * push constant buffer, or -1 if it's only in the pull constant
    * buffer.
    */
   memset(c->constant_map, -1, c->vp->program.Base.Parameters->NumParameters);
   for (i = 0;
	i < c->vp->program.Base.NumInstructions && constant < max_constant;
	i++) {
      struct prog_instruction *inst = &c->vp->program.Base.Instructions[i];
      int arg;

      for (arg = 0; arg < 3 && constant < max_constant; arg++) {
	 if (inst->SrcReg[arg].File != PROGRAM_STATE_VAR &&
	     inst->SrcReg[arg].File != PROGRAM_CONSTANT &&
	     inst->SrcReg[arg].File != PROGRAM_UNIFORM &&
	     inst->SrcReg[arg].File != PROGRAM_ENV_PARAM &&
	     inst->SrcReg[arg].File != PROGRAM_LOCAL_PARAM) {
	    continue;
	 }

	 if (inst->SrcReg[arg].RelAddr) {
	    c->vp->use_const_buffer = true;
	    continue;
	 }

	 if (c->constant_map[inst->SrcReg[arg].Index] == -1) {
	    c->constant_map[inst->SrcReg[arg].Index] = constant++;
	 }
      }
   }

   /* If we ran out of push constant space, then we'll also upload all
    * constants through the pull constant buffer so that they can be
    * accessed no matter what.  For relative addressing (the common
    * case) we need them all in place anyway.
    */
   if (constant == max_constant)
      c->vp->use_const_buffer = true;

   /* Set up the references to the pull parameters if present.  This backend
    * uses a 1:1 mapping from Mesa IR's index to location in the pull constant
    * buffer, while the new VS backend allocates values to the pull buffer on
    * demand.
    */
   if (c->vp->use_const_buffer) {
      for (i = 0; i < params->NumParameters * 4; i++) {
	 c->prog_data.pull_param[i] = &params->ParameterValues[i / 4][i % 4].f;
      }
      c->prog_data.nr_pull_params = i;
   }

   for (i = 0; i < constant; i++) {
      c->regs[PROGRAM_STATE_VAR][i] = stride(brw_vec4_grf(reg + i / 2,
							  (i % 2) * 4),
					     0, 4, 1);
   }
   reg += (constant + 1) / 2;
   c->prog_data.curb_read_length = reg - 1;
   c->prog_data.nr_params = constant * 4;
   /* XXX 0 causes a bug elsewhere... */
   if (intel->gen < 6 && c->prog_data.nr_params == 0)
      c->prog_data.nr_params = 4;

   /* Allocate input regs:  
    */
   c->nr_inputs = 0;
   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      if (c->prog_data.inputs_read & BITFIELD64_BIT(i)) {
	 c->nr_inputs++;
	 c->regs[PROGRAM_INPUT][i] = brw_vec8_grf(reg, 0);
	 reg++;
      }
   }
   /* If there are no inputs, we'll still be reading one attribute's worth
    * because it's required -- see urb_read_length setting.
    */
   if (c->nr_inputs == 0)
      reg++;

   /* Allocate outputs.  The non-position outputs go straight into message regs.
    */
   c->first_output = reg;

   first_reladdr_output = get_first_reladdr_output(&c->vp->program);

   for (slot = 0; slot < c->prog_data.vue_map.num_slots; slot++) {
      int vert_result = c->prog_data.vue_map.slot_to_vert_result[slot];
      assert(vert_result < Elements(c->regs[PROGRAM_OUTPUT]));
      if (can_use_direct_mrf(vert_result, first_reladdr_output, slot)) {
         c->regs[PROGRAM_OUTPUT][vert_result] = brw_message_reg(slot + 1);
      } else {
         c->regs[PROGRAM_OUTPUT][vert_result] = brw_vec8_grf(reg, 0);
         reg++;
      }
   }     

   /* Allocate program temporaries:
    */
   for (i = 0; i < c->vp->program.Base.NumTemporaries; i++) {
      c->regs[PROGRAM_TEMPORARY][i] = brw_vec8_grf(reg, 0);
      reg++;
   }

   /* Address reg(s).  Don't try to use the internal address reg until
    * deref time.
    */
   for (i = 0; i < c->vp->program.Base.NumAddressRegs; i++) {
      c->regs[PROGRAM_ADDRESS][i] =  brw_reg(BRW_GENERAL_REGISTER_FILE,
					     reg,
					     0,
					     BRW_REGISTER_TYPE_D,
					     BRW_VERTICAL_STRIDE_8,
					     BRW_WIDTH_8,
					     BRW_HORIZONTAL_STRIDE_1,
					     BRW_SWIZZLE_XXXX,
					     WRITEMASK_X);
      reg++;
   }

   if (c->vp->use_const_buffer) {
      for (i = 0; i < 3; i++) {
         c->current_const[i].reg = brw_vec8_grf(reg, 0);
         reg++;
      }
      clear_current_const(c);
   }

   for (i = 0; i < 128; i++) {
      if (c->output_regs[i].used_in_src) {
         c->output_regs[i].reg = brw_vec8_grf(reg, 0);
         reg++;
      }
   }

   /* Some opcodes need an internal temporary:
    */
   c->first_tmp = reg;
   c->last_tmp = reg;		/* for allocation purposes */

   /* Each input reg holds data from two vertices.  The
    * urb_read_length is the number of registers read from *each*
    * vertex urb, so is half the amount:
    */
   c->prog_data.urb_read_length = (c->nr_inputs + 1) / 2;
   /* Setting this field to 0 leads to undefined behavior according to the
    * the VS_STATE docs.  Our VUEs will always have at least one attribute
    * sitting in them, even if it's padding.
    */
   if (c->prog_data.urb_read_length == 0)
      c->prog_data.urb_read_length = 1;

   /* The VS VUEs are shared by VF (outputting our inputs) and VS, so size
    * them to fit the biggest thing they need to.
    */
   attributes_in_vue = MAX2(c->prog_data.vue_map.num_slots, c->nr_inputs);

   if (intel->gen == 6) {
      /* Each attribute is 32 bytes (2 vec4s), so dividing by 8 gives us the
       * number of 128-byte (1024-bit) units.
       */
      c->prog_data.urb_entry_size = ALIGN(attributes_in_vue, 8) / 8;
   } else {
      /* Each attribute is 16 bytes (1 vec4), so dividing by 4 gives us the
       * number of 64-byte (512-bit) units.
       */
      c->prog_data.urb_entry_size = ALIGN(attributes_in_vue, 4) / 4;
   }

   c->prog_data.total_grf = reg;

   if (unlikely(INTEL_DEBUG & DEBUG_VS)) {
      printf("%s NumAddrRegs %d\n", __FUNCTION__, c->vp->program.Base.NumAddressRegs);
      printf("%s NumTemps %d\n", __FUNCTION__, c->vp->program.Base.NumTemporaries);
      printf("%s reg = %d\n", __FUNCTION__, reg);
   }
}


/**
 * If an instruction uses a temp reg both as a src and the dest, we
 * sometimes need to allocate an intermediate temporary.
 */
static void unalias1( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      void (*func)( struct brw_vs_compile *,
				    struct brw_reg,
				    struct brw_reg ))
{
   if (dst.file == arg0.file && dst.nr == arg0.nr) {
      struct brw_compile *p = &c->func;
      struct brw_reg tmp = brw_writemask(get_tmp(c), dst.dw1.bits.writemask);
      func(c, tmp, arg0);
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
   else {
      func(c, dst, arg0);
   }
}

/**
 * \sa unalias2
 * Checkes if 2-operand instruction needs an intermediate temporary.
 */
static void unalias2( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1,
		      void (*func)( struct brw_vs_compile *,
				    struct brw_reg,
				    struct brw_reg,
				    struct brw_reg ))
{
   if ((dst.file == arg0.file && dst.nr == arg0.nr) ||
       (dst.file == arg1.file && dst.nr == arg1.nr)) {
      struct brw_compile *p = &c->func;
      struct brw_reg tmp = brw_writemask(get_tmp(c), dst.dw1.bits.writemask);
      func(c, tmp, arg0, arg1);
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
   else {
      func(c, dst, arg0, arg1);
   }
}

/**
 * \sa unalias2
 * Checkes if 3-operand instruction needs an intermediate temporary.
 */
static void unalias3( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1,
		      struct brw_reg arg2,
		      void (*func)( struct brw_vs_compile *,
				    struct brw_reg,
				    struct brw_reg,
				    struct brw_reg,
				    struct brw_reg ))
{
   if ((dst.file == arg0.file && dst.nr == arg0.nr) ||
       (dst.file == arg1.file && dst.nr == arg1.nr) ||
       (dst.file == arg2.file && dst.nr == arg2.nr)) {
      struct brw_compile *p = &c->func;
      struct brw_reg tmp = brw_writemask(get_tmp(c), dst.dw1.bits.writemask);
      func(c, tmp, arg0, arg1, arg2);
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
   else {
      func(c, dst, arg0, arg1, arg2);
   }
}

static void emit_sop( struct brw_vs_compile *c,
                      struct brw_reg dst,
                      struct brw_reg arg0,
                      struct brw_reg arg1, 
		      GLuint cond)
{
   struct brw_compile *p = &c->func;

   brw_MOV(p, dst, brw_imm_f(0.0f));
   brw_CMP(p, brw_null_reg(), cond, arg0, arg1);
   brw_MOV(p, dst, brw_imm_f(1.0f));
   brw_set_predicate_control_flag_value(p, 0xff);
}

static void emit_seq( struct brw_vs_compile *c,
                      struct brw_reg dst,
                      struct brw_reg arg0,
                      struct brw_reg arg1 )
{
   emit_sop(c, dst, arg0, arg1, BRW_CONDITIONAL_EQ);
}

static void emit_sne( struct brw_vs_compile *c,
                      struct brw_reg dst,
                      struct brw_reg arg0,
                      struct brw_reg arg1 )
{
   emit_sop(c, dst, arg0, arg1, BRW_CONDITIONAL_NEQ);
}
static void emit_slt( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   emit_sop(c, dst, arg0, arg1, BRW_CONDITIONAL_L);
}

static void emit_sle( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   emit_sop(c, dst, arg0, arg1, BRW_CONDITIONAL_LE);
}

static void emit_sgt( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   emit_sop(c, dst, arg0, arg1, BRW_CONDITIONAL_G);
}

static void emit_sge( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
  emit_sop(c, dst, arg0, arg1, BRW_CONDITIONAL_GE);
}

static void emit_cmp( struct brw_compile *p,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1,
		      struct brw_reg arg2 )
{
   brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_L, arg0, brw_imm_f(0));
   brw_SEL(p, dst, arg1, arg2);
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);
}

static void emit_sign(struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0)
{
   struct brw_compile *p = &c->func;

   brw_MOV(p, dst, brw_imm_f(0));

   brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_L, arg0, brw_imm_f(0));
   brw_MOV(p, dst, brw_imm_f(-1.0));
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);

   brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_G, arg0, brw_imm_f(0));
   brw_MOV(p, dst, brw_imm_f(1.0));
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);
}

static void emit_max( struct brw_compile *p, 
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   struct intel_context *intel = &p->brw->intel;

   if (intel->gen >= 6) {
      brw_set_conditionalmod(p, BRW_CONDITIONAL_GE);
      brw_SEL(p, dst, arg0, arg1);
      brw_set_conditionalmod(p, BRW_CONDITIONAL_NONE);
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);
   } else {
      brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_GE, arg0, arg1);
      brw_SEL(p, dst, arg0, arg1);
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);
   }
}

static void emit_min( struct brw_compile *p, 
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   struct intel_context *intel = &p->brw->intel;

   if (intel->gen >= 6) {
      brw_set_conditionalmod(p, BRW_CONDITIONAL_L);
      brw_SEL(p, dst, arg0, arg1);
      brw_set_conditionalmod(p, BRW_CONDITIONAL_NONE);
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);
   } else {
      brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_L, arg0, arg1);
      brw_SEL(p, dst, arg0, arg1);
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);
   }
}

static void emit_arl(struct brw_compile *p,
		     struct brw_reg dst,
		     struct brw_reg src)
{
   struct intel_context *intel = &p->brw->intel;

   if (intel->gen >= 6) {
      struct brw_reg dst_f = retype(dst, BRW_REGISTER_TYPE_F);

      brw_RNDD(p, dst_f, src);
      brw_MOV(p, dst, dst_f);
   } else {
      brw_RNDD(p, dst, src);
   }
}

static void emit_math1_gen4(struct brw_vs_compile *c,
			    GLuint function,
			    struct brw_reg dst,
			    struct brw_reg arg0,
			    GLuint precision)
{
   /* There are various odd behaviours with SEND on the simulator.  In
    * addition there are documented issues with the fact that the GEN4
    * processor doesn't do dependency control properly on SEND
    * results.  So, on balance, this kludge to get around failures
    * with writemasked math results looks like it might be necessary
    * whether that turns out to be a simulator bug or not:
    */
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = dst;
   bool need_tmp = false;

   if (dst.file != BRW_GENERAL_REGISTER_FILE ||
       dst.dw1.bits.writemask != 0xf)
      need_tmp = true;

   if (need_tmp)
      tmp = get_tmp(c);

   brw_math(p, 
	    tmp,
	    function,
	    2,
	    arg0,
	    BRW_MATH_DATA_SCALAR,
	    precision);

   if (need_tmp) {
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
}

static void
emit_math1_gen6(struct brw_vs_compile *c,
		GLuint function,
		struct brw_reg dst,
		struct brw_reg arg0,
		GLuint precision)
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp_src, tmp_dst;

   /* Something is strange on gen6 math in 16-wide mode, though the
    * docs say it's supposed to work.  Punt to using align1 mode,
    * which doesn't do writemasking and swizzles.
    */
   tmp_src = get_tmp(c);
   tmp_dst = get_tmp(c);

   brw_MOV(p, tmp_src, arg0);

   brw_set_access_mode(p, BRW_ALIGN_1);
   brw_math(p,
	    tmp_dst,
	    function,
	    2,
	    tmp_src,
	    BRW_MATH_DATA_SCALAR,
	    precision);
   brw_set_access_mode(p, BRW_ALIGN_16);

   brw_MOV(p, dst, tmp_dst);

   release_tmp(c, tmp_src);
   release_tmp(c, tmp_dst);
}

static void
emit_math1(struct brw_vs_compile *c,
	   GLuint function,
	   struct brw_reg dst,
	   struct brw_reg arg0,
	   GLuint precision)
{
   struct brw_compile *p = &c->func;
   struct intel_context *intel = &p->brw->intel;

   if (intel->gen >= 6)
      emit_math1_gen6(c, function, dst, arg0, precision);
   else
      emit_math1_gen4(c, function, dst, arg0, precision);
}

static void emit_math2_gen4( struct brw_vs_compile *c, 
			GLuint function,
			struct brw_reg dst,
			struct brw_reg arg0,
			struct brw_reg arg1,
			GLuint precision)
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = dst;
   bool need_tmp = false;

   if (dst.file != BRW_GENERAL_REGISTER_FILE ||
       dst.dw1.bits.writemask != 0xf)
      need_tmp = true;

   if (need_tmp) 
      tmp = get_tmp(c);

   brw_MOV(p, brw_message_reg(3), arg1);
   
   brw_math(p, 
	    tmp,
	    function,
	    2,
 	    arg0,
	    BRW_MATH_DATA_SCALAR,
	    precision);

   if (need_tmp) {
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
}

static void emit_math2_gen6( struct brw_vs_compile *c, 
			GLuint function,
			struct brw_reg dst,
			struct brw_reg arg0,
			struct brw_reg arg1,
			GLuint precision)
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp_src0, tmp_src1, tmp_dst;

   tmp_src0 = get_tmp(c);
   tmp_src1 = get_tmp(c);
   tmp_dst = get_tmp(c);

   brw_MOV(p, tmp_src0, arg0);
   brw_MOV(p, tmp_src1, arg1);
   
   brw_set_access_mode(p, BRW_ALIGN_1);
   brw_math2(p,
	    tmp_dst,
	    function,
	    tmp_src0,
	    tmp_src1);
   brw_set_access_mode(p, BRW_ALIGN_16);

   brw_MOV(p, dst, tmp_dst);

   release_tmp(c, tmp_src0);
   release_tmp(c, tmp_src1);
   release_tmp(c, tmp_dst);
}

static void emit_math2( struct brw_vs_compile *c, 
			GLuint function,
			struct brw_reg dst,
			struct brw_reg arg0,
			struct brw_reg arg1,
			GLuint precision)
{
   struct brw_compile *p = &c->func;
   struct intel_context *intel = &p->brw->intel;

   if (intel->gen >= 6)
      emit_math2_gen6(c, function, dst, arg0, arg1, precision);
   else
      emit_math2_gen4(c, function, dst, arg0, arg1, precision);
}

static void emit_exp_noalias( struct brw_vs_compile *c,
			      struct brw_reg dst,
			      struct brw_reg arg0 )
{
   struct brw_compile *p = &c->func;
   

   if (dst.dw1.bits.writemask & WRITEMASK_X) {
      struct brw_reg tmp = get_tmp(c);
      struct brw_reg tmp_d = retype(tmp, BRW_REGISTER_TYPE_D);

      /* tmp_d = floor(arg0.x) */
      brw_RNDD(p, tmp_d, brw_swizzle1(arg0, 0));

      /* result[0] = 2.0 ^ tmp */

      /* Adjust exponent for floating point: 
       * exp += 127 
       */
      brw_ADD(p, brw_writemask(tmp_d, WRITEMASK_X), tmp_d, brw_imm_d(127));

      /* Install exponent and sign.  
       * Excess drops off the edge: 
       */
      brw_SHL(p, brw_writemask(retype(dst, BRW_REGISTER_TYPE_D), WRITEMASK_X), 
	      tmp_d, brw_imm_d(23));

      release_tmp(c, tmp);
   }

   if (dst.dw1.bits.writemask & WRITEMASK_Y) {
      /* result[1] = arg0.x - floor(arg0.x) */
      brw_FRC(p, brw_writemask(dst, WRITEMASK_Y), brw_swizzle1(arg0, 0));
   }
   
   if (dst.dw1.bits.writemask & WRITEMASK_Z) {
      /* As with the LOG instruction, we might be better off just
       * doing a taylor expansion here, seeing as we have to do all
       * the prep work.
       *
       * If mathbox partial precision is too low, consider also:
       * result[3] = result[0] * EXP(result[1])
       */
      emit_math1(c, 
		 BRW_MATH_FUNCTION_EXP, 
		 brw_writemask(dst, WRITEMASK_Z),
		 brw_swizzle1(arg0, 0), 
		 BRW_MATH_PRECISION_FULL);
   }  

   if (dst.dw1.bits.writemask & WRITEMASK_W) {
      /* result[3] = 1.0; */
      brw_MOV(p, brw_writemask(dst, WRITEMASK_W), brw_imm_f(1));
   }
}


static void emit_log_noalias( struct brw_vs_compile *c,
			      struct brw_reg dst,
			      struct brw_reg arg0 )
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = dst;
   struct brw_reg tmp_ud = retype(tmp, BRW_REGISTER_TYPE_UD);
   struct brw_reg arg0_ud = retype(arg0, BRW_REGISTER_TYPE_UD);
   bool need_tmp = (dst.dw1.bits.writemask != 0xf ||
			 dst.file != BRW_GENERAL_REGISTER_FILE);

   if (need_tmp) {
      tmp = get_tmp(c);
      tmp_ud = retype(tmp, BRW_REGISTER_TYPE_UD);
   }
   
   /* Perform mant = frexpf(fabsf(x), &exp), adjust exp and mnt
    * according to spec:
    *
    * These almost look likey they could be joined up, but not really
    * practical:
    *
    * result[0].f = (x.i & ((1<<31)-1) >> 23) - 127
    * result[1].i = (x.i & ((1<<23)-1)        + (127<<23)
    */
   if (dst.dw1.bits.writemask & WRITEMASK_XZ) {
      brw_AND(p, 
	      brw_writemask(tmp_ud, WRITEMASK_X),
	      brw_swizzle1(arg0_ud, 0),
	      brw_imm_ud((1U<<31)-1));

      brw_SHR(p, 
	      brw_writemask(tmp_ud, WRITEMASK_X), 
	      tmp_ud,
	      brw_imm_ud(23));

      brw_ADD(p, 
	      brw_writemask(tmp, WRITEMASK_X), 
	      retype(tmp_ud, BRW_REGISTER_TYPE_D),	/* does it matter? */
	      brw_imm_d(-127));
   }

   if (dst.dw1.bits.writemask & WRITEMASK_YZ) {
      brw_AND(p, 
	      brw_writemask(tmp_ud, WRITEMASK_Y),
	      brw_swizzle1(arg0_ud, 0),
	      brw_imm_ud((1<<23)-1));

      brw_OR(p, 
	     brw_writemask(tmp_ud, WRITEMASK_Y), 
	     tmp_ud,
	     brw_imm_ud(127<<23));
   }
   
   if (dst.dw1.bits.writemask & WRITEMASK_Z) {
      /* result[2] = result[0] + LOG2(result[1]); */

      /* Why bother?  The above is just a hint how to do this with a
       * taylor series.  Maybe we *should* use a taylor series as by
       * the time all the above has been done it's almost certainly
       * quicker than calling the mathbox, even with low precision.
       * 
       * Options are:
       *    - result[0] + mathbox.LOG2(result[1])
       *    - mathbox.LOG2(arg0.x)
       *    - result[0] + inline_taylor_approx(result[1])
       */
      emit_math1(c, 
		 BRW_MATH_FUNCTION_LOG, 
		 brw_writemask(tmp, WRITEMASK_Z), 
		 brw_swizzle1(tmp, 1), 
		 BRW_MATH_PRECISION_FULL);
      
      brw_ADD(p, 
	      brw_writemask(tmp, WRITEMASK_Z), 
	      brw_swizzle1(tmp, 2), 
	      brw_swizzle1(tmp, 0));
   }  

   if (dst.dw1.bits.writemask & WRITEMASK_W) {
      /* result[3] = 1.0; */
      brw_MOV(p, brw_writemask(tmp, WRITEMASK_W), brw_imm_f(1));
   }

   if (need_tmp) {
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
}


/* Need to unalias - consider swizzles:   r0 = DST r0.xxxx r1
 */
static void emit_dst_noalias( struct brw_vs_compile *c, 
			      struct brw_reg dst,
			      struct brw_reg arg0,
			      struct brw_reg arg1)
{
   struct brw_compile *p = &c->func;

   /* There must be a better way to do this: 
    */
   if (dst.dw1.bits.writemask & WRITEMASK_X)
      brw_MOV(p, brw_writemask(dst, WRITEMASK_X), brw_imm_f(1.0));
   if (dst.dw1.bits.writemask & WRITEMASK_Y)
      brw_MUL(p, brw_writemask(dst, WRITEMASK_Y), arg0, arg1);
   if (dst.dw1.bits.writemask & WRITEMASK_Z)
      brw_MOV(p, brw_writemask(dst, WRITEMASK_Z), arg0);
   if (dst.dw1.bits.writemask & WRITEMASK_W)
      brw_MOV(p, brw_writemask(dst, WRITEMASK_W), arg1);
}


static void emit_xpd( struct brw_compile *p,
		      struct brw_reg dst,
		      struct brw_reg t,
		      struct brw_reg u)
{
   brw_MUL(p, brw_null_reg(), brw_swizzle(t, 1,2,0,3),  brw_swizzle(u,2,0,1,3));
   brw_MAC(p, dst,     negate(brw_swizzle(t, 2,0,1,3)), brw_swizzle(u,1,2,0,3));
}


static void emit_lit_noalias( struct brw_vs_compile *c, 
			      struct brw_reg dst,
			      struct brw_reg arg0 )
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = dst;
   bool need_tmp = (dst.file != BRW_GENERAL_REGISTER_FILE);

   if (need_tmp) 
      tmp = get_tmp(c);
   
   brw_MOV(p, brw_writemask(dst, WRITEMASK_YZ), brw_imm_f(0)); 
   brw_MOV(p, brw_writemask(dst, WRITEMASK_XW), brw_imm_f(1)); 

   /* Need to use BRW_EXECUTE_8 and also do an 8-wide compare in order
    * to get all channels active inside the IF.  In the clipping code
    * we run with NoMask, so it's not an option and we can use
    * BRW_EXECUTE_1 for all comparisions.
    */
   brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_G, brw_swizzle1(arg0,0), brw_imm_f(0));
   brw_IF(p, BRW_EXECUTE_8);
   {
      brw_MOV(p, brw_writemask(dst, WRITEMASK_Y), brw_swizzle1(arg0,0));

      brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_G, brw_swizzle1(arg0,1), brw_imm_f(0));
      brw_MOV(p, brw_writemask(tmp, WRITEMASK_Z),  brw_swizzle1(arg0,1));
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);

      emit_math2(c, 
		 BRW_MATH_FUNCTION_POW, 
		 brw_writemask(dst, WRITEMASK_Z),
		 brw_swizzle1(tmp, 2),
		 brw_swizzle1(arg0, 3),
		 BRW_MATH_PRECISION_PARTIAL);      
   }
   brw_ENDIF(p);

   release_tmp(c, tmp);
}

static void emit_lrp_noalias(struct brw_vs_compile *c,
			     struct brw_reg dst,
			     struct brw_reg arg0,
			     struct brw_reg arg1,
			     struct brw_reg arg2)
{
   struct brw_compile *p = &c->func;

   brw_ADD(p, dst, negate(arg0), brw_imm_f(1.0));
   brw_MUL(p, brw_null_reg(), dst, arg2);
   brw_MAC(p, dst, arg0, arg1);
}

static struct brw_reg
get_constant(struct brw_vs_compile *c,
             const struct prog_instruction *inst,
             GLuint argIndex)
{
   const struct prog_src_register *src = &inst->SrcReg[argIndex];
   struct brw_compile *p = &c->func;
   struct brw_reg const_reg = c->current_const[argIndex].reg;

   assert(argIndex < 3);

   if (c->current_const[argIndex].index != src->Index) {
      /* Keep track of the last constant loaded in this slot, for reuse. */
      c->current_const[argIndex].index = src->Index;

#if 0
      printf("  fetch const[%d] for arg %d into reg %d\n",
             src->Index, argIndex, c->current_const[argIndex].reg.nr);
#endif
      /* need to fetch the constant now */
      brw_dp_READ_4_vs(p,
                       const_reg,                     /* writeback dest */
                       16 * src->Index,               /* byte offset */
                       SURF_INDEX_VERT_CONST_BUFFER   /* binding table index */
                       );
   }

   /* replicate lower four floats into upper half (to get XYZWXYZW) */
   const_reg = stride(const_reg, 0, 4, 1);
   const_reg.subnr = 0;

   return const_reg;
}

static struct brw_reg
get_reladdr_constant(struct brw_vs_compile *c,
		     const struct prog_instruction *inst,
		     GLuint argIndex)
{
   const struct prog_src_register *src = &inst->SrcReg[argIndex];
   struct brw_compile *p = &c->func;
   struct brw_context *brw = p->brw;
   struct intel_context *intel = &brw->intel;
   struct brw_reg const_reg = c->current_const[argIndex].reg;
   struct brw_reg addr_reg = c->regs[PROGRAM_ADDRESS][0];
   uint32_t offset;

   assert(argIndex < 3);

   /* Can't reuse a reladdr constant load. */
   c->current_const[argIndex].index = -1;

 #if 0
   printf("  fetch const[a0.x+%d] for arg %d into reg %d\n",
	  src->Index, argIndex, c->current_const[argIndex].reg.nr);
#endif

   if (intel->gen >= 6) {
      offset = src->Index;
   } else {
      struct brw_reg byte_addr_reg = retype(get_tmp(c), BRW_REGISTER_TYPE_D);
      brw_MUL(p, byte_addr_reg, addr_reg, brw_imm_d(16));
      addr_reg = byte_addr_reg;
      offset = 16 * src->Index;
   }

   /* fetch the first vec4 */
   brw_dp_READ_4_vs_relative(p,
			     const_reg,
			     addr_reg,
			     offset,
			     SURF_INDEX_VERT_CONST_BUFFER);

   return const_reg;
}



/* TODO: relative addressing!
 */
static struct brw_reg get_reg( struct brw_vs_compile *c,
			       gl_register_file file,
			       GLuint index )
{
   switch (file) {
   case PROGRAM_TEMPORARY:
   case PROGRAM_INPUT:
   case PROGRAM_OUTPUT:
      assert(c->regs[file][index].nr != 0);
      return c->regs[file][index];
   case PROGRAM_STATE_VAR:
   case PROGRAM_CONSTANT:
   case PROGRAM_UNIFORM:
      assert(c->regs[PROGRAM_STATE_VAR][index].nr != 0);
      return c->regs[PROGRAM_STATE_VAR][index];
   case PROGRAM_ADDRESS:
      assert(index == 0);
      return c->regs[file][index];

   case PROGRAM_UNDEFINED:			/* undef values */
      return brw_null_reg();

   case PROGRAM_LOCAL_PARAM: 
   case PROGRAM_ENV_PARAM: 
   case PROGRAM_WRITE_ONLY:
   default:
      assert(0);
      return brw_null_reg();
   }
}


/**
 * Indirect addressing:  get reg[[arg] + offset].
 */
static struct brw_reg deref( struct brw_vs_compile *c,
			     struct brw_reg arg,
			     GLint offset,
			     GLuint reg_size )
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = get_tmp(c);
   struct brw_reg addr_reg = c->regs[PROGRAM_ADDRESS][0];
   struct brw_reg vp_address = retype(vec1(addr_reg), BRW_REGISTER_TYPE_D);
   GLuint byte_offset = arg.nr * 32 + arg.subnr + offset * reg_size;
   struct brw_reg indirect = brw_vec4_indirect(0,0);
   struct brw_reg acc = retype(vec1(get_tmp(c)), BRW_REGISTER_TYPE_UW);

   /* Set the vertical stride on the register access so that the first
    * 4 components come from a0.0 and the second 4 from a0.1.
    */
   indirect.vstride = BRW_VERTICAL_STRIDE_ONE_DIMENSIONAL;

   {
      brw_push_insn_state(p);
      brw_set_access_mode(p, BRW_ALIGN_1);

      brw_MUL(p, acc, vp_address, brw_imm_uw(reg_size));
      brw_ADD(p, brw_address_reg(0), acc, brw_imm_uw(byte_offset));

      brw_MUL(p, acc, suboffset(vp_address, 4), brw_imm_uw(reg_size));
      brw_ADD(p, brw_address_reg(1), acc, brw_imm_uw(byte_offset));

      brw_MOV(p, tmp, indirect);

      brw_pop_insn_state(p);
   }

   /* NOTE: tmp not released */
   return tmp;
}

static void
move_to_reladdr_dst(struct brw_vs_compile *c,
		    const struct prog_instruction *inst,
		    struct brw_reg val)
{
   struct brw_compile *p = &c->func;
   int reg_size = 32;
   struct brw_reg addr_reg = c->regs[PROGRAM_ADDRESS][0];
   struct brw_reg vp_address = retype(vec1(addr_reg), BRW_REGISTER_TYPE_D);
   struct brw_reg base = c->regs[inst->DstReg.File][inst->DstReg.Index];
   GLuint byte_offset = base.nr * 32 + base.subnr;
   struct brw_reg indirect = brw_vec4_indirect(0,0);
   struct brw_reg acc = retype(vec1(get_tmp(c)), BRW_REGISTER_TYPE_UW);

   /* Because destination register indirect addressing can only use
    * one index, we'll write each vertex's vec4 value separately.
    */
   val.width = BRW_WIDTH_4;
   val.vstride = BRW_VERTICAL_STRIDE_4;

   brw_push_insn_state(p);
   brw_set_access_mode(p, BRW_ALIGN_1);

   brw_MUL(p, acc, vp_address, brw_imm_uw(reg_size));
   brw_ADD(p, brw_address_reg(0), acc, brw_imm_uw(byte_offset));
   brw_MOV(p, indirect, val);

   brw_MUL(p, acc, suboffset(vp_address, 4), brw_imm_uw(reg_size));
   brw_ADD(p, brw_address_reg(0), acc,
	   brw_imm_uw(byte_offset + reg_size / 2));
   brw_MOV(p, indirect, suboffset(val, 4));

   brw_pop_insn_state(p);
}

/**
 * Get brw reg corresponding to the instruction's [argIndex] src reg.
 * TODO: relative addressing!
 */
static struct brw_reg
get_src_reg( struct brw_vs_compile *c,
             const struct prog_instruction *inst,
             GLuint argIndex )
{
   const GLuint file = inst->SrcReg[argIndex].File;
   const GLint index = inst->SrcReg[argIndex].Index;
   const bool relAddr = inst->SrcReg[argIndex].RelAddr;

   if (brw_vs_arg_can_be_immediate(inst->Opcode, argIndex)) {
      const struct prog_src_register *src = &inst->SrcReg[argIndex];

      if (src->Swizzle == MAKE_SWIZZLE4(SWIZZLE_ZERO,
					SWIZZLE_ZERO,
					SWIZZLE_ZERO,
					SWIZZLE_ZERO)) {
	  return brw_imm_f(0.0f);
      } else if (src->Swizzle == MAKE_SWIZZLE4(SWIZZLE_ONE,
					       SWIZZLE_ONE,
					       SWIZZLE_ONE,
					       SWIZZLE_ONE)) {
	 if (src->Negate)
	    return brw_imm_f(-1.0F);
	 else
	    return brw_imm_f(1.0F);
      } else if (src->File == PROGRAM_CONSTANT) {
	 const struct gl_program_parameter_list *params;
	 float f;
	 int component = -1;

	 switch (src->Swizzle) {
	 case SWIZZLE_XXXX:
	    component = 0;
	    break;
	 case SWIZZLE_YYYY:
	    component = 1;
	    break;
	 case SWIZZLE_ZZZZ:
	    component = 2;
	    break;
	 case SWIZZLE_WWWW:
	    component = 3;
	    break;
	 }

	 if (component >= 0) {
	    params = c->vp->program.Base.Parameters;
	    f = params->ParameterValues[src->Index][component].f;

	    if (src->Abs)
	       f = fabs(f);
	    if (src->Negate)
	       f = -f;
	    return brw_imm_f(f);
	 }
      }
   }

   switch (file) {
   case PROGRAM_TEMPORARY:
   case PROGRAM_INPUT:
   case PROGRAM_OUTPUT:
      if (relAddr) {
         return deref(c, c->regs[file][0], index, 32);
      }
      else {
         assert(c->regs[file][index].nr != 0);
         return c->regs[file][index];
      }

   case PROGRAM_STATE_VAR:
   case PROGRAM_CONSTANT:
   case PROGRAM_UNIFORM:
   case PROGRAM_ENV_PARAM:
   case PROGRAM_LOCAL_PARAM:
      if (!relAddr && c->constant_map[index] != -1) {
	 /* Take from the push constant buffer if possible. */
	 assert(c->regs[PROGRAM_STATE_VAR][c->constant_map[index]].nr != 0);
	 return c->regs[PROGRAM_STATE_VAR][c->constant_map[index]];
      } else {
	 /* Must be in the pull constant buffer then .*/
	 assert(c->vp->use_const_buffer);
	 if (relAddr)
	    return get_reladdr_constant(c, inst, argIndex);
	 else
	    return get_constant(c, inst, argIndex);
      }
   case PROGRAM_ADDRESS:
      assert(index == 0);
      return c->regs[file][index];

   case PROGRAM_UNDEFINED:
      /* this is a normal case since we loop over all three src args */
      return brw_null_reg();

   case PROGRAM_WRITE_ONLY:
   default:
      assert(0);
      return brw_null_reg();
   }
}

/**
 * Return the brw reg for the given instruction's src argument.
 * Will return mangled results for SWZ op.  The emit_swz() function
 * ignores this result and recalculates taking extended swizzles into
 * account.
 */
static struct brw_reg get_arg( struct brw_vs_compile *c,
                               const struct prog_instruction *inst,
                               GLuint argIndex )
{
   const struct prog_src_register *src = &inst->SrcReg[argIndex];
   struct brw_reg reg;

   if (src->File == PROGRAM_UNDEFINED)
      return brw_null_reg();

   reg = get_src_reg(c, inst, argIndex);

   /* Convert 3-bit swizzle to 2-bit.  
    */
   if (reg.file != BRW_IMMEDIATE_VALUE) {
      reg.dw1.bits.swizzle = BRW_SWIZZLE4(GET_SWZ(src->Swizzle, 0),
					  GET_SWZ(src->Swizzle, 1),
					  GET_SWZ(src->Swizzle, 2),
					  GET_SWZ(src->Swizzle, 3));

      /* Note this is ok for non-swizzle ARB_vp instructions */
      reg.negate = src->Negate ? 1 : 0;
   }

   return reg;
}


/**
 * Get brw register for the given program dest register.
 */
static struct brw_reg get_dst( struct brw_vs_compile *c,
			       struct prog_dst_register dst )
{
   struct brw_reg reg;

   switch (dst.File) {
   case PROGRAM_TEMPORARY:
   case PROGRAM_OUTPUT:
      /* register-indirect addressing is only 1x1, not VxH, for
       * destination regs.  So, for RelAddr we'll return a temporary
       * for the dest and do a move of the result to the RelAddr
       * register after the instruction emit.
       */
      if (dst.RelAddr) {
	 reg = get_tmp(c);
      } else {
	 assert(c->regs[dst.File][dst.Index].nr != 0);
	 reg = c->regs[dst.File][dst.Index];
      }
      break;
   case PROGRAM_ADDRESS:
      assert(dst.Index == 0);
      reg = c->regs[dst.File][dst.Index];
      break;
   case PROGRAM_UNDEFINED:
      /* we may hit this for OPCODE_END, OPCODE_KIL, etc */
      reg = brw_null_reg();
      break;
   default:
      assert(0);
      reg = brw_null_reg();
   }

   assert(reg.type != BRW_IMMEDIATE_VALUE);
   reg.dw1.bits.writemask = dst.WriteMask;

   return reg;
}


static void emit_swz( struct brw_vs_compile *c, 
		      struct brw_reg dst,
                      const struct prog_instruction *inst)
{
   const GLuint argIndex = 0;
   const struct prog_src_register src = inst->SrcReg[argIndex];
   struct brw_compile *p = &c->func;
   GLuint zeros_mask = 0;
   GLuint ones_mask = 0;
   GLuint src_mask = 0;
   GLubyte src_swz[4];
   bool need_tmp = (src.Negate &&
			 dst.file != BRW_GENERAL_REGISTER_FILE);
   struct brw_reg tmp = dst;
   GLuint i;

   if (need_tmp)
      tmp = get_tmp(c);

   for (i = 0; i < 4; i++) {
      if (dst.dw1.bits.writemask & (1<<i)) {
	 GLubyte s = GET_SWZ(src.Swizzle, i);
	 switch (s) {
	 case SWIZZLE_X:
	 case SWIZZLE_Y:
	 case SWIZZLE_Z:
	 case SWIZZLE_W:
	    src_mask |= 1<<i;
	    src_swz[i] = s;
	    break;
	 case SWIZZLE_ZERO:
	    zeros_mask |= 1<<i;
	    break;
	 case SWIZZLE_ONE:
	    ones_mask |= 1<<i;
	    break;
	 }
      }
   }
   
   /* Do src first, in case dst aliases src:
    */
   if (src_mask) {
      struct brw_reg arg0;

      arg0 = get_src_reg(c, inst, argIndex);

      arg0 = brw_swizzle(arg0, 
			 src_swz[0], src_swz[1], 
			 src_swz[2], src_swz[3]);

      brw_MOV(p, brw_writemask(tmp, src_mask), arg0);
   } 
   
   if (zeros_mask) 
      brw_MOV(p, brw_writemask(tmp, zeros_mask), brw_imm_f(0));

   if (ones_mask) 
      brw_MOV(p, brw_writemask(tmp, ones_mask), brw_imm_f(1));

   if (src.Negate)
      brw_MOV(p, brw_writemask(tmp, src.Negate), negate(tmp));
   
   if (need_tmp) {
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
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
 * Post-vertex-program processing.  Send the results to the URB.
 */
static void emit_vertex_write( struct brw_vs_compile *c)
{
   struct brw_compile *p = &c->func;
   struct brw_context *brw = p->brw;
   struct intel_context *intel = &brw->intel;
   struct brw_reg pos = c->regs[PROGRAM_OUTPUT][VERT_RESULT_HPOS];
   struct brw_reg ndc;
   int eot;
   GLuint len_vertex_header = 2;
   int i;
   int msg_len;
   int slot;

   if (c->key.copy_edgeflag) {
      brw_MOV(p, 
	      get_reg(c, PROGRAM_OUTPUT, VERT_RESULT_EDGE),
	      get_reg(c, PROGRAM_INPUT, VERT_ATTRIB_EDGEFLAG));
   }

   if (intel->gen < 6) {
      /* Build ndc coords */
      ndc = get_tmp(c);
      /* ndc = 1.0 / pos.w */
      emit_math1(c, BRW_MATH_FUNCTION_INV, ndc, brw_swizzle1(pos, 3), BRW_MATH_PRECISION_FULL);
      /* ndc.xyz = pos * ndc */
      brw_MUL(p, brw_writemask(ndc, WRITEMASK_XYZ), pos, ndc);
   }

   /* Update the header for point size, user clipping flags, and -ve rhw
    * workaround.
    */
   if (intel->gen >= 6) {
      struct brw_reg m1 = brw_message_reg(1);

      /* On gen6, m1 has each value in a separate dword, so we never
       * need to mess with a temporary for computing the m1 value.
       */
      brw_MOV(p, retype(m1, BRW_REGISTER_TYPE_UD), brw_imm_ud(0));
      if (c->prog_data.outputs_written & BITFIELD64_BIT(VERT_RESULT_PSIZ)) {
	 brw_MOV(p, brw_writemask(m1, WRITEMASK_W),
		 brw_swizzle1(c->regs[PROGRAM_OUTPUT][VERT_RESULT_PSIZ], 0));
      }

      /* Set the user clip distances in dword 8-15. (m3-4)*/
      if (c->key.userclip_active) {
	 for (i = 0; i < c->key.nr_userclip_plane_consts; i++) {
	    struct brw_reg m;
	    if (i < 4)
	       m = brw_message_reg(3);
	    else
	       m = brw_message_reg(4);

	    brw_DP4(p, brw_writemask(m, (1 << (i & 3))),pos, c->userplane[i]);
	 }
      }
   } else if ((c->prog_data.outputs_written &
	       BITFIELD64_BIT(VERT_RESULT_PSIZ)) ||
	      c->key.userclip_active || brw->has_negative_rhw_bug) {
      struct brw_reg header1 = retype(get_tmp(c), BRW_REGISTER_TYPE_UD);
      GLuint i;

      brw_MOV(p, header1, brw_imm_ud(0));

      brw_set_access_mode(p, BRW_ALIGN_16);	

      if (c->prog_data.outputs_written & BITFIELD64_BIT(VERT_RESULT_PSIZ)) {
	 struct brw_reg psiz = c->regs[PROGRAM_OUTPUT][VERT_RESULT_PSIZ];
	 brw_MUL(p, brw_writemask(header1, WRITEMASK_W),
		 brw_swizzle1(psiz, 0), brw_imm_f(1<<11));
	 brw_AND(p, brw_writemask(header1, WRITEMASK_W),
		 header1, brw_imm_ud(0x7ff<<8));
      }

      for (i = 0; i < c->key.nr_userclip_plane_consts; i++) {
	 brw_set_conditionalmod(p, BRW_CONDITIONAL_L);
	 brw_DP4(p, brw_null_reg(), pos, c->userplane[i]);
	 brw_OR(p, brw_writemask(header1, WRITEMASK_W), header1, brw_imm_ud(1<<i));
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
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
	 brw_CMP(p,
		 vec8(brw_null_reg()),
		 BRW_CONDITIONAL_L,
		 brw_swizzle1(ndc, 3),
		 brw_imm_f(0));
   
	 brw_OR(p, brw_writemask(header1, WRITEMASK_W), header1, brw_imm_ud(1<<6));
	 brw_MOV(p, ndc, brw_imm_f(0));
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
      }

      brw_set_access_mode(p, BRW_ALIGN_1);	/* why? */
      brw_MOV(p, retype(brw_message_reg(1), BRW_REGISTER_TYPE_UD), header1);
      brw_set_access_mode(p, BRW_ALIGN_16);

      release_tmp(c, header1);
   }
   else {
      brw_MOV(p, retype(brw_message_reg(1), BRW_REGISTER_TYPE_UD), brw_imm_ud(0));
   }

   /* Emit the (interleaved) headers for the two vertices - an 8-reg
    * of zeros followed by two sets of NDC coordinates:
    */
   brw_set_access_mode(p, BRW_ALIGN_1);
   brw_set_acc_write_control(p, 0);

   /* The VUE layout is documented in Volume 2a. */
   if (intel->gen >= 6) {
      /* There are 8 or 16 DWs (D0-D15) in VUE header on Sandybridge:
       * dword 0-3 (m1) of the header is indices, point width, clip flags.
       * dword 4-7 (m2) is the 4D space position
       * dword 8-15 (m3,m4) of the vertex header is the user clip distance if
       * enabled.
       * m3 or 5 is the first vertex element data we fill, which is
       * the vertex position.
       */
      brw_MOV(p, brw_message_reg(2), pos);
      len_vertex_header = 1;
      if (c->key.userclip_active)
	 len_vertex_header += 2;
   } else if (intel->gen == 5) {
      /* There are 20 DWs (D0-D19) in VUE header on Ironlake:
       * dword 0-3 (m1) of the header is indices, point width, clip flags.
       * dword 4-7 (m2) is the ndc position (set above)
       * dword 8-11 (m3) of the vertex header is the 4D space position
       * dword 12-19 (m4,m5) of the vertex header is the user clip distance.
       * m6 is a pad so that the vertex element data is aligned
       * m7 is the first vertex data we fill, which is the vertex position.
       */
      brw_MOV(p, brw_message_reg(2), ndc);
      brw_MOV(p, brw_message_reg(3), pos);
      brw_MOV(p, brw_message_reg(7), pos);
      len_vertex_header = 6;
   } else {
      /* There are 8 dwords in VUE header pre-Ironlake:
       * dword 0-3 (m1) is indices, point width, clip flags.
       * dword 4-7 (m2) is ndc position (set above)
       *
       * dword 8-11 (m3) is the first vertex data, which we always have be the
       * vertex position.
       */
      brw_MOV(p, brw_message_reg(2), ndc);
      brw_MOV(p, brw_message_reg(3), pos);
      len_vertex_header = 2;
   }

   /* Move variable-addressed, non-overflow outputs to their MRFs. */
   for (slot = len_vertex_header; slot < c->prog_data.vue_map.num_slots; ++slot) {
      if (slot >= MAX_SLOTS_IN_FIRST_URB_WRITE)
         break;

      int mrf = slot + 1;
      int vert_result = c->prog_data.vue_map.slot_to_vert_result[slot];
      if (c->regs[PROGRAM_OUTPUT][vert_result].file ==
          BRW_GENERAL_REGISTER_FILE) {
         brw_MOV(p, brw_message_reg(mrf),
                 c->regs[PROGRAM_OUTPUT][vert_result]);
      }
   }

   eot = (slot >= c->prog_data.vue_map.num_slots);

   /* Message header, plus the (first part of the) VUE. */
   msg_len = 1 + slot;
   msg_len = align_interleaved_urb_mlen(brw, msg_len);
   /* Any outputs beyond BRW_MAX_MRF should be in the second URB write */
   assert (msg_len <= BRW_MAX_MRF - 1);

   brw_urb_WRITE(p, 
		 brw_null_reg(), /* dest */
		 0,		/* starting mrf reg nr */
		 c->r0,		/* src */
		 0,		/* allocate */
		 1,		/* used */
		 msg_len,
		 0,		/* response len */
		 eot, 		/* eot */
		 eot, 		/* writes complete */
		 0, 		/* urb destination offset */
		 BRW_URB_SWIZZLE_INTERLEAVE);

   if (slot < c->prog_data.vue_map.num_slots) {
      /* Not all of the vertex outputs/results fit into the MRF.
       * Move the overflowed attributes from the GRF to the MRF and
       * issue another brw_urb_WRITE().
       */
      GLuint mrf = 1;
      for (; slot < c->prog_data.vue_map.num_slots; ++slot) {
         int vert_result = c->prog_data.vue_map.slot_to_vert_result[slot];
         /* move from GRF to MRF */
         brw_MOV(p, brw_message_reg(mrf),
                 c->regs[PROGRAM_OUTPUT][vert_result]);
         mrf++;
      }

      brw_urb_WRITE(p,
                    brw_null_reg(), /* dest */
                    0,              /* starting mrf reg nr */
                    c->r0,          /* src */
                    0,              /* allocate */
                    1,              /* used */
                    align_interleaved_urb_mlen(brw, mrf),
                    0,              /* response len */
                    1,              /* eot */
                    1,              /* writes complete */
                    MAX_SLOTS_IN_FIRST_URB_WRITE / 2,  /* urb destination offset */
                    BRW_URB_SWIZZLE_INTERLEAVE);
   }
}

static bool
accumulator_contains(struct brw_vs_compile *c, struct brw_reg val)
{
   struct brw_compile *p = &c->func;
   struct brw_instruction *prev_insn = &p->store[p->nr_insn - 1];

   if (p->nr_insn == 0)
      return false;

   if (val.address_mode != BRW_ADDRESS_DIRECT)
      return false;

   if (val.negate || val.abs || val.dw1.bits.swizzle != BRW_SWIZZLE_XYZW)
      return false;

   switch (prev_insn->header.opcode) {
   case BRW_OPCODE_MOV:
   case BRW_OPCODE_MAC:
   case BRW_OPCODE_MUL:
      if (prev_insn->header.access_mode == BRW_ALIGN_16 &&
	  prev_insn->header.execution_size == val.width &&
	  prev_insn->bits1.da1.dest_reg_file == val.file &&
	  prev_insn->bits1.da1.dest_reg_type == val.type &&
	  prev_insn->bits1.da1.dest_address_mode == val.address_mode &&
	  prev_insn->bits1.da1.dest_reg_nr == val.nr &&
	  prev_insn->bits1.da16.dest_subreg_nr == val.subnr / 16 &&
	  prev_insn->bits1.da16.dest_writemask == 0xf)
	 return true;
      else
	 return false;
   default:
      return false;
   }
}

static uint32_t
get_predicate(const struct prog_instruction *inst)
{
   if (inst->DstReg.CondMask == COND_TR)
      return BRW_PREDICATE_NONE;

   /* All of GLSL only produces predicates for COND_NE and one channel per
    * vector.  Fail badly if someone starts doing something else, as it might
    * mean infinite looping or something.
    *
    * We'd like to support all the condition codes, but our hardware doesn't
    * quite match the Mesa IR, which is modeled after the NV extensions.  For
    * those, the instruction may update the condition codes or not, then any
    * later instruction may use one of those condition codes.  For gen4, the
    * instruction may update the flags register based on one of the condition
    * codes output by the instruction, and then further instructions may
    * predicate on that.  We can probably support this, but it won't
    * necessarily be easy.
    */
   assert(inst->DstReg.CondMask == COND_NE);

   switch (inst->DstReg.CondSwizzle) {
   case SWIZZLE_XXXX:
      return BRW_PREDICATE_ALIGN16_REPLICATE_X;
   case SWIZZLE_YYYY:
      return BRW_PREDICATE_ALIGN16_REPLICATE_Y;
   case SWIZZLE_ZZZZ:
      return BRW_PREDICATE_ALIGN16_REPLICATE_Z;
   case SWIZZLE_WWWW:
      return BRW_PREDICATE_ALIGN16_REPLICATE_W;
   default:
      _mesa_problem(NULL, "Unexpected predicate: 0x%08x\n",
		    inst->DstReg.CondMask);
      return BRW_PREDICATE_NORMAL;
   }
}

static void
brw_vs_rescale_gl_fixed(struct brw_vs_compile *c)
{
   struct brw_compile *p = &c->func;
   int i;

   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      if (!(c->prog_data.inputs_read & BITFIELD64_BIT(i)))
	 continue;

      if (c->key.gl_fixed_input_size[i] != 0) {
	 struct brw_reg reg = c->regs[PROGRAM_INPUT][i];

	 brw_MUL(p,
		 brw_writemask(reg, (1 << c->key.gl_fixed_input_size[i]) - 1),
		 reg, brw_imm_f(1.0 / 65536.0));
      }
   }
}

/* Emit the vertex program instructions here.
 */
void brw_old_vs_emit(struct brw_vs_compile *c )
{
#define MAX_IF_DEPTH 32
#define MAX_LOOP_DEPTH 32
   struct brw_compile *p = &c->func;
   struct brw_context *brw = p->brw;
   struct intel_context *intel = &brw->intel;
   const GLuint nr_insns = c->vp->program.Base.NumInstructions;
   GLuint insn;
   GLuint index;
   GLuint file;

   if (unlikely(INTEL_DEBUG & DEBUG_VS)) {
      printf("vs-mesa:\n");
      _mesa_fprint_program_opt(stdout, &c->vp->program.Base, PROG_PRINT_DEBUG,
			       true);
      printf("\n");
   }

   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_set_access_mode(p, BRW_ALIGN_16);

   brw_set_acc_write_control(p, 1);

   for (insn = 0; insn < nr_insns; insn++) {
       GLuint i;
       struct prog_instruction *inst = &c->vp->program.Base.Instructions[insn];

       /* Message registers can't be read, so copy the output into GRF
	* register if they are used in source registers
	*/
       for (i = 0; i < 3; i++) {
	   struct prog_src_register *src = &inst->SrcReg[i];
	   GLuint index = src->Index;
	   GLuint file = src->File;	
	   if (file == PROGRAM_OUTPUT && index != VERT_RESULT_HPOS)
	       c->output_regs[index].used_in_src = true;
       }
   }

   /* Static register allocation
    */
   brw_vs_alloc_regs(c);

   brw_vs_rescale_gl_fixed(c);

   for (insn = 0; insn < nr_insns; insn++) {

      const struct prog_instruction *inst = &c->vp->program.Base.Instructions[insn];
      struct brw_reg args[3], dst;
      GLuint i;

#if 0
      printf("%d: ", insn);
      _mesa_print_instruction(inst);
#endif

      /* Get argument regs.  SWZ is special and does this itself.
       */
      if (inst->Opcode != OPCODE_SWZ)
	  for (i = 0; i < 3; i++) {
	      const struct prog_src_register *src = &inst->SrcReg[i];
	      index = src->Index;
	      file = src->File;	
	      if (file == PROGRAM_OUTPUT && c->output_regs[index].used_in_src) {
		 /* Can't just make get_arg "do the right thing" here because
		  * other callers of get_arg and get_src_reg don't expect any
		  * special behavior for the c->output_regs[index].used_in_src
		  * case.
		  */
		 args[i] = c->output_regs[index].reg;
		 args[i].dw1.bits.swizzle =
		    BRW_SWIZZLE4(GET_SWZ(src->Swizzle, 0),
				 GET_SWZ(src->Swizzle, 1),
				 GET_SWZ(src->Swizzle, 2),
				 GET_SWZ(src->Swizzle, 3));

		 /* Note this is ok for non-swizzle ARB_vp instructions */
		 args[i].negate = src->Negate ? 1 : 0;
	      } else
                  args[i] = get_arg(c, inst, i);
	  }

      /* Get dest regs.  Note that it is possible for a reg to be both
       * dst and arg, given the static allocation of registers.  So
       * care needs to be taken emitting multi-operation instructions.
       */ 
      index = inst->DstReg.Index;
      file = inst->DstReg.File;
      if (file == PROGRAM_OUTPUT && c->output_regs[index].used_in_src)
	 /* Can't just make get_dst "do the right thing" here because other
	  * callers of get_dst don't expect any special behavior for the
	  * c->output_regs[index].used_in_src case.
	  */
	 dst = brw_writemask(c->output_regs[index].reg, inst->DstReg.WriteMask);
      else
	  dst = get_dst(c, inst->DstReg);

      if (inst->SaturateMode != SATURATE_OFF) {
	 _mesa_problem(NULL, "Unsupported saturate %d in vertex shader",
                       inst->SaturateMode);
      }

      switch (inst->Opcode) {
      case OPCODE_ABS:
	 args[0].negate = false;
	 brw_MOV(p, dst, brw_abs(args[0]));
	 break;
      case OPCODE_ADD:
	 brw_ADD(p, dst, args[0], args[1]);
	 break;
      case OPCODE_COS:
	 emit_math1(c, BRW_MATH_FUNCTION_COS, dst, args[0], BRW_MATH_PRECISION_FULL);
	 break;
      case OPCODE_DP2:
	 brw_DP2(p, dst, args[0], args[1]);
	 break;
      case OPCODE_DP3:
	 brw_DP3(p, dst, args[0], args[1]);
	 break;
      case OPCODE_DP4:
	 brw_DP4(p, dst, args[0], args[1]);
	 break;
      case OPCODE_DPH:
	 brw_DPH(p, dst, args[0], args[1]);
	 break;
      case OPCODE_DST:
	 unalias2(c, dst, args[0], args[1], emit_dst_noalias); 
	 break;
      case OPCODE_EXP:
	 unalias1(c, dst, args[0], emit_exp_noalias);
	 break;
      case OPCODE_EX2:
	 emit_math1(c, BRW_MATH_FUNCTION_EXP, dst, args[0], BRW_MATH_PRECISION_FULL);
	 break;
      case OPCODE_ARL:
	 emit_arl(p, dst, args[0]);
	 break;
      case OPCODE_FLR:
	 brw_RNDD(p, dst, args[0]);
	 break;
      case OPCODE_FRC:
	 brw_FRC(p, dst, args[0]);
	 break;
      case OPCODE_LOG:
	 unalias1(c, dst, args[0], emit_log_noalias);
	 break;
      case OPCODE_LG2:
	 emit_math1(c, BRW_MATH_FUNCTION_LOG, dst, args[0], BRW_MATH_PRECISION_FULL);
	 break;
      case OPCODE_LIT:
	 unalias1(c, dst, args[0], emit_lit_noalias);
	 break;
      case OPCODE_LRP:
	 unalias3(c, dst, args[0], args[1], args[2], emit_lrp_noalias);
	 break;
      case OPCODE_MAD:
	 if (!accumulator_contains(c, args[2]))
	    brw_MOV(p, brw_acc_reg(), args[2]);
	 brw_MAC(p, dst, args[0], args[1]);
	 break;
      case OPCODE_CMP:
	 emit_cmp(p, dst, args[0], args[1], args[2]);
	 break;
      case OPCODE_MAX:
	 emit_max(p, dst, args[0], args[1]);
	 break;
      case OPCODE_MIN:
	 emit_min(p, dst, args[0], args[1]);
	 break;
      case OPCODE_MOV:
	 brw_MOV(p, dst, args[0]);
	 break;
      case OPCODE_MUL:
	 brw_MUL(p, dst, args[0], args[1]);
	 break;
      case OPCODE_POW:
	 emit_math2(c, BRW_MATH_FUNCTION_POW, dst, args[0], args[1], BRW_MATH_PRECISION_FULL); 
	 break;
      case OPCODE_RCP:
	 emit_math1(c, BRW_MATH_FUNCTION_INV, dst, args[0], BRW_MATH_PRECISION_FULL);
	 break;
      case OPCODE_RSQ:
	 emit_math1(c, BRW_MATH_FUNCTION_RSQ, dst, brw_abs(args[0]), BRW_MATH_PRECISION_FULL);
	 break;

      case OPCODE_SEQ:
         unalias2(c, dst, args[0], args[1], emit_seq);
         break;
      case OPCODE_SIN:
	 emit_math1(c, BRW_MATH_FUNCTION_SIN, dst, args[0], BRW_MATH_PRECISION_FULL);
	 break;
      case OPCODE_SNE:
         unalias2(c, dst, args[0], args[1], emit_sne);
         break;
      case OPCODE_SGE:
         unalias2(c, dst, args[0], args[1], emit_sge);
	 break;
      case OPCODE_SGT:
         unalias2(c, dst, args[0], args[1], emit_sgt);
         break;
      case OPCODE_SLT:
         unalias2(c, dst, args[0], args[1], emit_slt);
	 break;
      case OPCODE_SLE:
         unalias2(c, dst, args[0], args[1], emit_sle);
         break;
      case OPCODE_SSG:
         unalias1(c, dst, args[0], emit_sign);
         break;
      case OPCODE_SUB:
	 brw_ADD(p, dst, args[0], negate(args[1]));
	 break;
      case OPCODE_SWZ:
	 /* The args[0] value can't be used here as it won't have
	  * correctly encoded the full swizzle:
	  */
	 emit_swz(c, dst, inst);
	 break;
      case OPCODE_TRUNC:
         /* round toward zero */
	 brw_RNDZ(p, dst, args[0]);
	 break;
      case OPCODE_XPD:
	 emit_xpd(p, dst, args[0], args[1]);
	 break;
      case OPCODE_IF: {
	 struct brw_instruction *if_inst = brw_IF(p, BRW_EXECUTE_8);
	 /* Note that brw_IF smashes the predicate_control field. */
	 if_inst->header.predicate_control = get_predicate(inst);
	 break;
      }
      case OPCODE_ELSE:
	 clear_current_const(c);
	 brw_ELSE(p);
	 break;
      case OPCODE_ENDIF:
	 clear_current_const(c);
	 brw_ENDIF(p);
	 break;			
      case OPCODE_BGNLOOP:
	 clear_current_const(c);
	 brw_DO(p, BRW_EXECUTE_8);
         break;
      case OPCODE_BRK:
	 brw_set_predicate_control(p, get_predicate(inst));
	 brw_BREAK(p);
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
         break;
      case OPCODE_CONT:
	 brw_set_predicate_control(p, get_predicate(inst));
	 if (intel->gen >= 6) {
	    gen6_CONT(p);
	 } else {
	    brw_CONT(p);
	 }
         brw_set_predicate_control(p, BRW_PREDICATE_NONE);
         break;

      case OPCODE_ENDLOOP:
	 clear_current_const(c);
	 brw_WHILE(p);
         break;

      case OPCODE_BRA:
	 brw_set_predicate_control(p, get_predicate(inst));
         brw_ADD(p, brw_ip_reg(), brw_ip_reg(), brw_imm_d(1*16));
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
         break;
      case OPCODE_END:
	 emit_vertex_write(c);
         break;
      case OPCODE_PRINT:
         /* no-op */
         break;
      default:
	 _mesa_problem(NULL, "Unsupported opcode %i (%s) in vertex shader",
                       inst->Opcode, inst->Opcode < MAX_OPCODE ?
				    _mesa_opcode_string(inst->Opcode) :
				    "unknown");
      }

      /* Set the predication update on the last instruction of the native
       * instruction sequence.
       *
       * This would be problematic if it was set on a math instruction,
       * but that shouldn't be the case with the current GLSL compiler.
       */
      if (inst->CondUpdate) {
	 struct brw_instruction *hw_insn = &p->store[p->nr_insn - 1];

	 assert(hw_insn->header.destreg__conditionalmod == 0);
	 hw_insn->header.destreg__conditionalmod = BRW_CONDITIONAL_NZ;
      }

      if ((inst->DstReg.File == PROGRAM_OUTPUT)
          && (inst->DstReg.Index != VERT_RESULT_HPOS)
          && c->output_regs[inst->DstReg.Index].used_in_src) {
         brw_MOV(p, get_dst(c, inst->DstReg), dst);
      }

      /* Result color clamping.
       *
       * When destination register is an output register and
       * it's primary/secondary front/back color, we have to clamp
       * the result to [0,1]. This is done by enabling the
       * saturation bit for the last instruction.
       *
       * We don't use brw_set_saturate() as it modifies
       * p->current->header.saturate, which affects all the subsequent
       * instructions. Instead, we directly modify the header
       * of the last (already stored) instruction.
       */
      if (inst->DstReg.File == PROGRAM_OUTPUT &&
	  c->key.clamp_vertex_color) {
         if ((inst->DstReg.Index == VERT_RESULT_COL0)
             || (inst->DstReg.Index == VERT_RESULT_COL1)
             || (inst->DstReg.Index == VERT_RESULT_BFC0)
             || (inst->DstReg.Index == VERT_RESULT_BFC1)) {
            p->store[p->nr_insn-1].header.saturate = 1;
         }
      }

      if (inst->DstReg.RelAddr) {
	 assert(inst->DstReg.File == PROGRAM_TEMPORARY||
		inst->DstReg.File == PROGRAM_OUTPUT);
	 move_to_reladdr_dst(c, inst, dst);
      }

      release_tmps(c);
   }

   brw_set_uip_jip(p);

   brw_optimize(p);

   if (unlikely(INTEL_DEBUG & DEBUG_VS)) {
      int i;

      printf("vs-native:\n");
      for (i = 0; i < p->nr_insn; i++)
	 brw_disasm(stdout, &p->store[i], intel->gen);
      printf("\n");
   }
}
