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
                 

#include "brw_context.h"
#include "brw_wm.h"
#include "program/prog_parameter.h"



/***********************************************************************
 */

static struct brw_wm_ref *get_ref( struct brw_wm_compile *c )
{
   assert(c->nr_refs < BRW_WM_MAX_REF);
   memset(&c->refs[c->nr_refs], 0, sizeof(*c->refs));
   return &c->refs[c->nr_refs++];
}

static struct brw_wm_value *get_value( struct brw_wm_compile *c)
{
   assert(c->nr_refs < BRW_WM_MAX_VREG);
   memset(&c->vreg[c->nr_vreg], 0, sizeof(*c->vreg));
   return &c->vreg[c->nr_vreg++];
}

/** return pointer to a newly allocated instruction */
static struct brw_wm_instruction *get_instruction( struct brw_wm_compile *c )
{
   assert(c->nr_insns < BRW_WM_MAX_INSN);
   memset(&c->instruction[c->nr_insns], 0, sizeof(*c->instruction));
   return &c->instruction[c->nr_insns++];
}

/***********************************************************************
 */

/** Init the "undef" register */
static void pass0_init_undef( struct brw_wm_compile *c)
{
   struct brw_wm_ref *ref = &c->undef_ref;
   ref->value = &c->undef_value;
   ref->hw_reg = brw_vec8_grf(0, 0);
   ref->insn = 0;
   ref->prevuse = NULL;
}

/** Set a FP register to a value */
static void pass0_set_fpreg_value( struct brw_wm_compile *c,
				   GLuint file,
				   GLuint idx,
				   GLuint component,
				   struct brw_wm_value *value )
{
   struct brw_wm_ref *ref = get_ref(c);
   ref->value = value;
   ref->hw_reg = brw_vec8_grf(0, 0);
   ref->insn = 0;
   ref->prevuse = NULL;
   c->pass0_fp_reg[file][idx][component] = ref;
}

/** Set a FP register to a ref */
static void pass0_set_fpreg_ref( struct brw_wm_compile *c,
				 GLuint file,
				 GLuint idx,
				 GLuint component,
				 const struct brw_wm_ref *src_ref )
{
   c->pass0_fp_reg[file][idx][component] = src_ref;
}

static const struct brw_wm_ref *get_param_ref( struct brw_wm_compile *c, 
					       const GLfloat *param_ptr )
{
   GLuint i = c->prog_data.nr_params++;
   
   if (i >= BRW_WM_MAX_PARAM) {
      printf("%s: out of params\n", __FUNCTION__);
      c->prog_data.error = 1;
      return NULL;
   }
   else {
      struct brw_wm_ref *ref = get_ref(c);

      c->prog_data.param[i] = param_ptr;
      c->nr_creg = (i+16)/16;

      /* Push the offsets into hw_reg.  These will be added to the
       * real register numbers once one is allocated in pass2.
       */
      ref->hw_reg = brw_vec1_grf((i&8)?1:0, i%8);
      ref->value = &c->creg[i/16];
      ref->insn = 0;
      ref->prevuse = NULL;

      return ref;
   }
}


/** Return a ref to a constant/literal value */
static const struct brw_wm_ref *get_const_ref( struct brw_wm_compile *c,
					       const GLfloat *constval )
{
   GLuint i;

   /* Search for an existing const value matching the request:
    */
   for (i = 0; i < c->nr_constrefs; i++) {
      if (c->constref[i].constval == *constval) 
	 return c->constref[i].ref;
   }

   /* Else try to add a new one:
    */
   if (c->nr_constrefs < BRW_WM_MAX_CONST) {
      GLuint i = c->nr_constrefs++;

      /* A constant is a special type of parameter:
       */
      c->constref[i].constval = *constval;
      c->constref[i].ref = get_param_ref(c, constval);

      return c->constref[i].ref;
   }
   else {
      printf("%s: out of constrefs\n", __FUNCTION__);
      c->prog_data.error = 1;
      return NULL;
   }
}


/* Lookup our internal registers
 */
static const struct brw_wm_ref *pass0_get_reg( struct brw_wm_compile *c,
					       GLuint file,
					       GLuint idx,
					       GLuint component )
{
   const struct brw_wm_ref *ref = c->pass0_fp_reg[file][idx][component];

   if (!ref) {
      switch (file) {
      case PROGRAM_INPUT:
      case PROGRAM_PAYLOAD:
      case PROGRAM_TEMPORARY:
      case PROGRAM_OUTPUT:
      case PROGRAM_VARYING:
	 break;

      case PROGRAM_LOCAL_PARAM:
	 ref = get_param_ref(c, &c->fp->program.Base.LocalParams[idx][component]);
	 break;

      case PROGRAM_ENV_PARAM:
	 ref = get_param_ref(c, &c->env_param[idx][component]);
	 break;

      case PROGRAM_STATE_VAR:
      case PROGRAM_UNIFORM:
      case PROGRAM_CONSTANT:
      case PROGRAM_NAMED_PARAM: {
	 struct gl_program_parameter_list *plist = c->fp->program.Base.Parameters;
	 
	 /* There's something really hokey about parameters parsed in
	  * arb programs - they all end up in here, whether they be
	  * state values, parameters or constants.  This duplicates the
	  * structure above & also seems to subvert the limits set for
	  * each type of constant/param.
	  */ 
	 switch (plist->Parameters[idx].Type) {
	 case PROGRAM_NAMED_PARAM:
	 case PROGRAM_CONSTANT:
	    /* These are invariant:
	     */
	    ref = get_const_ref(c, &plist->ParameterValues[idx][component].f);
	    break;

	 case PROGRAM_STATE_VAR:
	 case PROGRAM_UNIFORM:
	    /* These may change from run to run:
	     */
	    ref = get_param_ref(c, &plist->ParameterValues[idx][component].f );
	    break;

	 default:
	    assert(0);
	    break;
	 }
	 break;
      }

      default:
	 assert(0);
	 break;
      }

      c->pass0_fp_reg[file][idx][component] = ref;
   }

   if (!ref)
      ref = &c->undef_ref;

   return ref;
}



/***********************************************************************
 * Straight translation to internal instruction format
 */

static void pass0_set_dst( struct brw_wm_compile *c,
			   struct brw_wm_instruction *out,
			   const struct prog_instruction *inst,
			   GLuint writemask )
{
   const struct prog_dst_register *dst = &inst->DstReg;
   GLuint i;

   for (i = 0; i < 4; i++) {
      if (writemask & (1<<i)) {
	 out->dst[i] = get_value(c);
	 pass0_set_fpreg_value(c, dst->File, dst->Index, i, out->dst[i]);
      }
   }

   out->writemask = writemask;
}


static const struct brw_wm_ref *get_fp_src_reg_ref( struct brw_wm_compile *c,
						    struct prog_src_register src,
						    GLuint i )
{
   GLuint component = GET_SWZ(src.Swizzle,i);
   const struct brw_wm_ref *src_ref;
   static const GLfloat const_zero = 0.0;
   static const GLfloat const_one = 1.0;

   if (component == SWIZZLE_ZERO) 
      src_ref = get_const_ref(c, &const_zero);
   else if (component == SWIZZLE_ONE) 
      src_ref = get_const_ref(c, &const_one);
   else 
      src_ref = pass0_get_reg(c, src.File, src.Index, component);

   return src_ref;
}


static struct brw_wm_ref *get_new_ref( struct brw_wm_compile *c,
				       struct prog_src_register src,
				       GLuint i,
				       struct brw_wm_instruction *insn)
{
   const struct brw_wm_ref *ref = get_fp_src_reg_ref(c, src, i);
   struct brw_wm_ref *newref = get_ref(c);

   newref->value = ref->value;
   newref->hw_reg = ref->hw_reg;

   if (insn) {
      newref->insn = insn - c->instruction;
      newref->prevuse = newref->value->lastuse;
      newref->value->lastuse = newref;
   }

   if (src.Negate & (1 << i))
      newref->hw_reg.negate ^= 1;

   if (src.Abs) {
      newref->hw_reg.negate = 0;
      newref->hw_reg.abs = 1;
   }

   return newref;
}


static void
translate_insn(struct brw_wm_compile *c,
               const struct prog_instruction *inst)
{
   struct brw_wm_instruction *out = get_instruction(c);
   GLuint writemask = inst->DstReg.WriteMask;
   GLuint nr_args = brw_wm_nr_args(inst->Opcode);
   GLuint i, j;

   /* Copy some data out of the instruction
    */
   out->opcode = inst->Opcode;
   out->saturate = (inst->SaturateMode != SATURATE_OFF);
   out->tex_unit = inst->TexSrcUnit;
   out->tex_idx = inst->TexSrcTarget;
   out->tex_shadow = inst->TexShadow;
   out->eot = inst->Aux & INST_AUX_EOT;
   out->target = INST_AUX_GET_TARGET(inst->Aux);

   /* Args:
    */
   for (i = 0; i < nr_args; i++) {
      for (j = 0; j < 4; j++) {
	 out->src[i][j] = get_new_ref(c, inst->SrcReg[i], j, out);
      }
   }

   /* Dst:
    */
   pass0_set_dst(c, out, inst, writemask);
}



/***********************************************************************
 * Optimize moves and swizzles away:
 */ 
static void pass0_precalc_mov( struct brw_wm_compile *c,
			       const struct prog_instruction *inst )
{
   const struct prog_dst_register *dst = &inst->DstReg;
   GLuint writemask = inst->DstReg.WriteMask;
   struct brw_wm_ref *refs[4];
   GLuint i;

   /* Get the effect of a MOV by manipulating our register table:
    * First get all refs, then assign refs.  This ensures that "in-place"
    * swizzles such as:
    *   MOV t, t.xxyx
    * are handled correctly.  Previously, these two steps were done in
    * one loop and the above case was incorrectly handled.
    */
   for (i = 0; i < 4; i++) {
      refs[i] = get_new_ref(c, inst->SrcReg[0], i, NULL);
   }
   for (i = 0; i < 4; i++) {
      if (writemask & (1 << i)) {	    
         pass0_set_fpreg_ref( c, dst->File, dst->Index, i, refs[i]);
      }
   }
}


/* Initialize payload "registers".
 */
static void pass0_init_payload( struct brw_wm_compile *c )
{
   GLuint i;

   for (i = 0; i < 4; i++) {
      GLuint j = i >= (c->nr_payload_regs + 1) / 2 ? 0 : i;
      pass0_set_fpreg_value( c, PROGRAM_PAYLOAD, PAYLOAD_DEPTH, i, 
			     &c->payload.depth[j] );
   }

#if 0
   /* This seems to be an alternative to the INTERP_WPOS stuff I do
    * elsewhere:
    */
   if (c->key.source_depth_reg)
      pass0_set_fpreg_value(c, PROGRAM_INPUT, FRAG_ATTRIB_WPOS, 2,
			    &c->payload.depth[c->key.source_depth_reg/2]);
#endif
   
   for (i = 0; i < FRAG_ATTRIB_MAX; i++)
      pass0_set_fpreg_value( c, PROGRAM_PAYLOAD, i, 0, 
			     &c->payload.input_interp[i] );      
}


/***********************************************************************
 * PASS 0
 *
 * Work forwards to give each calculated value a unique number.  Where
 * an instruction produces duplicate values (eg DP3), all are given
 * the same number.
 *
 * Translate away swizzling and eliminate non-saturating moves.
 */
void brw_wm_pass0( struct brw_wm_compile *c )
{
   GLuint insn;

   c->nr_vreg = 0;
   c->nr_insns = 0;

   pass0_init_undef(c);
   pass0_init_payload(c);

   for (insn = 0; insn < c->nr_fp_insns; insn++) {
      const struct prog_instruction *inst = &c->prog_instructions[insn];

      /* Optimize away moves, otherwise emit translated instruction:
       */      
      switch (inst->Opcode) {
      case OPCODE_MOV: 
      case OPCODE_SWZ: 
	 if (!inst->SaturateMode) {
	    pass0_precalc_mov(c, inst);
	 }
	 else {
	    translate_insn(c, inst);
	 }
	 break;
      default:
	 translate_insn(c, inst);
	 break;
      }
   }
 
   if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
      brw_wm_print_program(c, "pass0");
   }
}
