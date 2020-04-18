/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "main/glheader.h"
#include "main/macros.h"
#include "main/enums.h"

#include "program/prog_instruction.h"
#include "program/prog_parameter.h"
#include "program/program.h"
#include "program/programopt.h"
#include "program/prog_print.h"

#include "tnl/tnl.h"
#include "tnl/t_context.h"

#include "intel_batchbuffer.h"

#include "i915_reg.h"
#include "i915_context.h"
#include "i915_program.h"

static const GLfloat sin_quad_constants[2][4] = {
   {
      2.0,
      -1.0,
      .5,
      .75
   },
   {
      4.0,
      -4.0,
      1.0 / (2.0 * M_PI),
      .2225
   }
};

static const GLfloat sin_constants[4] = { 1.0,
   -1.0 / (3 * 2 * 1),
   1.0 / (5 * 4 * 3 * 2 * 1),
   -1.0 / (7 * 6 * 5 * 4 * 3 * 2 * 1)
};

/* 1, -1/2!, 1/4!, -1/6! */
static const GLfloat cos_constants[4] = { 1.0,
   -1.0 / (2 * 1),
   1.0 / (4 * 3 * 2 * 1),
   -1.0 / (6 * 5 * 4 * 3 * 2 * 1)
};

/**
 * Retrieve a ureg for the given source register.  Will emit
 * constants, apply swizzling and negation as needed.
 */
static GLuint
src_vector(struct i915_fragment_program *p,
           const struct prog_src_register *source,
           const struct gl_fragment_program *program)
{
   GLuint src;

   switch (source->File) {

      /* Registers:
       */
   case PROGRAM_TEMPORARY:
      if (source->Index >= I915_MAX_TEMPORARY) {
         i915_program_error(p, "Exceeded max temporary reg: %d/%d",
			    source->Index, I915_MAX_TEMPORARY);
         return 0;
      }
      src = UREG(REG_TYPE_R, source->Index);
      break;
   case PROGRAM_INPUT:
      switch (source->Index) {
      case FRAG_ATTRIB_WPOS:
         src = i915_emit_decl(p, REG_TYPE_T, p->wpos_tex, D0_CHANNEL_ALL);
         break;
      case FRAG_ATTRIB_COL0:
         src = i915_emit_decl(p, REG_TYPE_T, T_DIFFUSE, D0_CHANNEL_ALL);
         break;
      case FRAG_ATTRIB_COL1:
         src = i915_emit_decl(p, REG_TYPE_T, T_SPECULAR, D0_CHANNEL_XYZ);
         src = swizzle(src, X, Y, Z, ONE);
         break;
      case FRAG_ATTRIB_FOGC:
         src = i915_emit_decl(p, REG_TYPE_T, T_FOG_W, D0_CHANNEL_W);
         src = swizzle(src, W, ZERO, ZERO, ONE);
         break;
      case FRAG_ATTRIB_TEX0:
      case FRAG_ATTRIB_TEX1:
      case FRAG_ATTRIB_TEX2:
      case FRAG_ATTRIB_TEX3:
      case FRAG_ATTRIB_TEX4:
      case FRAG_ATTRIB_TEX5:
      case FRAG_ATTRIB_TEX6:
      case FRAG_ATTRIB_TEX7:
         src = i915_emit_decl(p, REG_TYPE_T,
                              T_TEX0 + (source->Index - FRAG_ATTRIB_TEX0),
                              D0_CHANNEL_ALL);
	 break;

      case FRAG_ATTRIB_VAR0:
      case FRAG_ATTRIB_VAR0 + 1:
      case FRAG_ATTRIB_VAR0 + 2:
      case FRAG_ATTRIB_VAR0 + 3:
      case FRAG_ATTRIB_VAR0 + 4:
      case FRAG_ATTRIB_VAR0 + 5:
      case FRAG_ATTRIB_VAR0 + 6:
      case FRAG_ATTRIB_VAR0 + 7:
         src = i915_emit_decl(p, REG_TYPE_T,
                              T_TEX0 + (source->Index - FRAG_ATTRIB_VAR0),
                              D0_CHANNEL_ALL);
         break;

      default:
         i915_program_error(p, "Bad source->Index: %d", source->Index);
         return 0;
      }
      break;

   case PROGRAM_OUTPUT:
      switch (source->Index) {
      case FRAG_RESULT_COLOR:
	 src = UREG(REG_TYPE_OC, 0);
	 break;
      case FRAG_RESULT_DEPTH:
	 src = UREG(REG_TYPE_OD, 0);
	 break;
      default:
	 i915_program_error(p, "Bad source->Index: %d", source->Index);
	 return 0;
      }
      break;

      /* Various paramters and env values.  All emitted to
       * hardware as program constants.
       */
   case PROGRAM_LOCAL_PARAM:
      src = i915_emit_param4fv(p, program->Base.LocalParams[source->Index]);
      break;

   case PROGRAM_ENV_PARAM:
      src =
         i915_emit_param4fv(p,
                            p->ctx->FragmentProgram.Parameters[source->
                                                               Index]);
      break;

   case PROGRAM_CONSTANT:
   case PROGRAM_STATE_VAR:
   case PROGRAM_NAMED_PARAM:
   case PROGRAM_UNIFORM:
      src = i915_emit_param4fv(p,
	 &program->Base.Parameters->ParameterValues[source->Index][0].f);
      break;

   default:
      i915_program_error(p, "Bad source->File: %d", source->File);
      return 0;
   }

   src = swizzle(src,
                 GET_SWZ(source->Swizzle, 0),
                 GET_SWZ(source->Swizzle, 1),
                 GET_SWZ(source->Swizzle, 2), GET_SWZ(source->Swizzle, 3));

   if (source->Negate)
      src = negate(src,
                   GET_BIT(source->Negate, 0),
                   GET_BIT(source->Negate, 1),
                   GET_BIT(source->Negate, 2),
                   GET_BIT(source->Negate, 3));

   return src;
}


static GLuint
get_result_vector(struct i915_fragment_program *p,
                  const struct prog_instruction *inst)
{
   switch (inst->DstReg.File) {
   case PROGRAM_OUTPUT:
      switch (inst->DstReg.Index) {
      case FRAG_RESULT_COLOR:
      case FRAG_RESULT_DATA0:
         return UREG(REG_TYPE_OC, 0);
      case FRAG_RESULT_DEPTH:
         p->depth_written = 1;
         return UREG(REG_TYPE_OD, 0);
      default:
         i915_program_error(p, "Bad inst->DstReg.Index: %d",
			    inst->DstReg.Index);
         return 0;
      }
   case PROGRAM_TEMPORARY:
      return UREG(REG_TYPE_R, inst->DstReg.Index);
   default:
      i915_program_error(p, "Bad inst->DstReg.File: %d", inst->DstReg.File);
      return 0;
   }
}

static GLuint
get_result_flags(const struct prog_instruction *inst)
{
   GLuint flags = 0;

   if (inst->SaturateMode == SATURATE_ZERO_ONE)
      flags |= A0_DEST_SATURATE;
   if (inst->DstReg.WriteMask & WRITEMASK_X)
      flags |= A0_DEST_CHANNEL_X;
   if (inst->DstReg.WriteMask & WRITEMASK_Y)
      flags |= A0_DEST_CHANNEL_Y;
   if (inst->DstReg.WriteMask & WRITEMASK_Z)
      flags |= A0_DEST_CHANNEL_Z;
   if (inst->DstReg.WriteMask & WRITEMASK_W)
      flags |= A0_DEST_CHANNEL_W;

   return flags;
}

static GLuint
translate_tex_src_target(struct i915_fragment_program *p, GLubyte bit)
{
   switch (bit) {
   case TEXTURE_1D_INDEX:
      return D0_SAMPLE_TYPE_2D;
   case TEXTURE_2D_INDEX:
      return D0_SAMPLE_TYPE_2D;
   case TEXTURE_RECT_INDEX:
      return D0_SAMPLE_TYPE_2D;
   case TEXTURE_3D_INDEX:
      return D0_SAMPLE_TYPE_VOLUME;
   case TEXTURE_CUBE_INDEX:
      return D0_SAMPLE_TYPE_CUBE;
   default:
      i915_program_error(p, "TexSrcBit: %d", bit);
      return 0;
   }
}

#define EMIT_TEX( OP )						\
do {								\
   GLuint dim = translate_tex_src_target( p, inst->TexSrcTarget );	\
   const struct gl_fragment_program *program = &p->FragProg;	\
   GLuint unit = program->Base.SamplerUnits[inst->TexSrcUnit];	\
   GLuint sampler = i915_emit_decl(p, REG_TYPE_S,		\
				   unit, dim);			\
   GLuint coord = src_vector( p, &inst->SrcReg[0], program);	\
   /* Texel lookup */						\
								\
   i915_emit_texld( p, get_live_regs(p, inst),						\
	       get_result_vector( p, inst ),			\
	       get_result_flags( inst ),			\
	       sampler,						\
	       coord,						\
	       OP);						\
} while (0)

#define EMIT_ARITH( OP, N )						\
do {									\
   i915_emit_arith( p,							\
	       OP,							\
	       get_result_vector( p, inst ), 				\
	       get_result_flags( inst ), 0,			\
	       (N<1)?0:src_vector( p, &inst->SrcReg[0], program),	\
	       (N<2)?0:src_vector( p, &inst->SrcReg[1], program),	\
	       (N<3)?0:src_vector( p, &inst->SrcReg[2], program));	\
} while (0)

#define EMIT_1ARG_ARITH( OP ) EMIT_ARITH( OP, 1 )
#define EMIT_2ARG_ARITH( OP ) EMIT_ARITH( OP, 2 )
#define EMIT_3ARG_ARITH( OP ) EMIT_ARITH( OP, 3 )

/* 
 * TODO: consider moving this into core 
 */
static bool calc_live_regs( struct i915_fragment_program *p )
{
    const struct gl_fragment_program *program = &p->FragProg;
    GLuint regsUsed = ~((1 << I915_MAX_TEMPORARY) - 1);
    uint8_t live_components[I915_MAX_TEMPORARY] = { 0, };
    GLint i;
   
    for (i = program->Base.NumInstructions - 1; i >= 0; i--) {
        struct prog_instruction *inst = &program->Base.Instructions[i];
        int opArgs = _mesa_num_inst_src_regs(inst->Opcode);
        int a;

        /* Register is written to: unmark as live for this and preceeding ops */ 
        if (inst->DstReg.File == PROGRAM_TEMPORARY) {
	    if (inst->DstReg.Index >= I915_MAX_TEMPORARY)
	       return false;

            live_components[inst->DstReg.Index] &= ~inst->DstReg.WriteMask;
            if (live_components[inst->DstReg.Index] == 0)
                regsUsed &= ~(1 << inst->DstReg.Index);
        }

        for (a = 0; a < opArgs; a++) {
            /* Register is read from: mark as live for this and preceeding ops */ 
            if (inst->SrcReg[a].File == PROGRAM_TEMPORARY) {
                unsigned c;

		if (inst->SrcReg[a].Index >= I915_MAX_TEMPORARY)
		   return false;

                regsUsed |= 1 << inst->SrcReg[a].Index;

                for (c = 0; c < 4; c++) {
                    const unsigned field = GET_SWZ(inst->SrcReg[a].Swizzle, c);

                    if (field <= SWIZZLE_W)
                        live_components[inst->SrcReg[a].Index] |= (1U << field);
                }
            }
        }

        p->usedRegs[i] = regsUsed;
    }

    return true;
}

static GLuint get_live_regs( struct i915_fragment_program *p, 
                             const struct prog_instruction *inst )
{
    const struct gl_fragment_program *program = &p->FragProg;
    GLuint nr = inst - program->Base.Instructions;

    return p->usedRegs[nr];
}
 

/* Possible concerns:
 *
 * SIN, COS -- could use another taylor step?
 * LIT      -- results seem a little different to sw mesa
 * LOG      -- different to mesa on negative numbers, but this is conformant.
 * 
 * Parse failures -- Mesa doesn't currently give a good indication
 * internally whether a particular program string parsed or not.  This
 * can lead to confusion -- hopefully we cope with it ok now.
 *
 */
static void
upload_program(struct i915_fragment_program *p)
{
   const struct gl_fragment_program *program = &p->FragProg;
   const struct prog_instruction *inst = program->Base.Instructions;

   if (INTEL_DEBUG & DEBUG_WM)
      _mesa_print_program(&program->Base);

   /* Is this a parse-failed program?  Ensure a valid program is
    * loaded, as the flagging of an error isn't sufficient to stop
    * this being uploaded to hardware.
    */
   if (inst[0].Opcode == OPCODE_END) {
      GLuint tmp = i915_get_utemp(p);
      i915_emit_arith(p,
                      A0_MOV,
                      UREG(REG_TYPE_OC, 0),
                      A0_DEST_CHANNEL_ALL, 0,
                      swizzle(tmp, ONE, ZERO, ONE, ONE), 0, 0);
      return;
   }

   if (program->Base.NumInstructions > I915_MAX_INSN) {
      i915_program_error(p, "Exceeded max instructions (%d out of %d)",
			 program->Base.NumInstructions, I915_MAX_INSN);
      return;
   }

   /* Not always needed:
    */
   if (!calc_live_regs(p)) {
      i915_program_error(p, "Could not allocate registers");
      return;
   }

   while (1) {
      GLuint src0, src1, src2, flags;
      GLuint tmp = 0, dst, consts0 = 0, consts1 = 0;

      switch (inst->Opcode) {
      case OPCODE_ABS:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         i915_emit_arith(p,
                         A0_MAX,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         src0, negate(src0, 1, 1, 1, 1), 0);
         break;

      case OPCODE_ADD:
         EMIT_2ARG_ARITH(A0_ADD);
         break;

      case OPCODE_CMP:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);
         src2 = src_vector(p, &inst->SrcReg[2], program);
         i915_emit_arith(p, A0_CMP, get_result_vector(p, inst), get_result_flags(inst), 0, src0, src2, src1);   /* NOTE: order of src2, src1 */
         break;

      case OPCODE_COS:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         tmp = i915_get_utemp(p);
	 consts0 = i915_emit_const4fv(p, sin_quad_constants[0]);
	 consts1 = i915_emit_const4fv(p, sin_quad_constants[1]);

	 /* Reduce range from repeating about [-pi,pi] to [-1,1] */
         i915_emit_arith(p,
                         A0_MAD,
                         tmp, A0_DEST_CHANNEL_X, 0,
                         src0,
			 swizzle(consts1, Z, ZERO, ZERO, ZERO), /* 1/(2pi) */
			 swizzle(consts0, W, ZERO, ZERO, ZERO)); /* .75 */

         i915_emit_arith(p, A0_FRC, tmp, A0_DEST_CHANNEL_X, 0, tmp, 0, 0);

	 i915_emit_arith(p,
			 A0_MAD,
			 tmp, A0_DEST_CHANNEL_X, 0,
			 tmp,
			 swizzle(consts0, X, ZERO, ZERO, ZERO), /* 2 */
			 swizzle(consts0, Y, ZERO, ZERO, ZERO)); /* -1 */

	 /* Compute COS with the same calculation used for SIN, but a
	  * different source range has been mapped to [-1,1] this time.
	  */

	 /* tmp.y = abs(tmp.x); {x, abs(x), 0, 0} */
	 i915_emit_arith(p,
                         A0_MAX,
			 tmp, A0_DEST_CHANNEL_Y, 0,
			 swizzle(tmp, ZERO, X, ZERO, ZERO),
			 negate(swizzle(tmp, ZERO, X, ZERO, ZERO), 0, 1, 0, 0),
			 0);

	 /* tmp.y = tmp.y * tmp.x; {x, x * abs(x), 0, 0} */
	 i915_emit_arith(p,
			 A0_MUL,
			 tmp, A0_DEST_CHANNEL_Y, 0,
			 swizzle(tmp, ZERO, X, ZERO, ZERO),
			 tmp,
			 0);

	 /* tmp.x = tmp.xy DP sin_quad_constants[2].xy */
         i915_emit_arith(p,
                         A0_DP3,
                         tmp, A0_DEST_CHANNEL_X, 0,
			 tmp,
                         swizzle(consts1, X, Y, ZERO, ZERO),
			 0);

	 /* tmp.x now contains a first approximation (y).  Now, weight it
	  * against tmp.y**2 to get closer.
	  */
	 i915_emit_arith(p,
                         A0_MAX,
			 tmp, A0_DEST_CHANNEL_Y, 0,
			 swizzle(tmp, ZERO, X, ZERO, ZERO),
			 negate(swizzle(tmp, ZERO, X, ZERO, ZERO), 0, 1, 0, 0),
			 0);

	 /* tmp.y = tmp.x * tmp.y - tmp.x; {y, y * abs(y) - y, 0, 0} */
	 i915_emit_arith(p,
			 A0_MAD,
			 tmp, A0_DEST_CHANNEL_Y, 0,
			 swizzle(tmp, ZERO, X, ZERO, ZERO),
			 swizzle(tmp, ZERO, Y, ZERO, ZERO),
			 negate(swizzle(tmp, ZERO, X, ZERO, ZERO), 0, 1, 0, 0));

	 /* result = .2225 * tmp.y + tmp.x =.2225(y * abs(y) - y) + y= */
	 i915_emit_arith(p,
			 A0_MAD,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
			 swizzle(consts1, W, W, W, W),
			 swizzle(tmp, Y, Y, Y, Y),
			 swizzle(tmp, X, X, X, X));
         break;

      case OPCODE_DP2:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);
	 i915_emit_arith(p,
			 A0_DP3,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
			 swizzle(src0, X, Y, ZERO, ZERO),
			 swizzle(src1, X, Y, ZERO, ZERO),
			 0);
         break;

      case OPCODE_DP3:
         EMIT_2ARG_ARITH(A0_DP3);
         break;

      case OPCODE_DP4:
         EMIT_2ARG_ARITH(A0_DP4);
         break;

      case OPCODE_DPH:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);

         i915_emit_arith(p,
                         A0_DP4,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         swizzle(src0, X, Y, Z, ONE), src1, 0);
         break;

      case OPCODE_DST:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);

         /* result[0] = 1    * 1;
          * result[1] = a[1] * b[1];
          * result[2] = a[2] * 1;
          * result[3] = 1    * b[3];
          */
         i915_emit_arith(p,
                         A0_MUL,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         swizzle(src0, ONE, Y, Z, ONE),
                         swizzle(src1, ONE, Y, ONE, W), 0);
         break;

      case OPCODE_EX2:
         src0 = src_vector(p, &inst->SrcReg[0], program);

         i915_emit_arith(p,
                         A0_EXP,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         swizzle(src0, X, X, X, X), 0, 0);
         break;

      case OPCODE_FLR:
         EMIT_1ARG_ARITH(A0_FLR);
         break;

      case OPCODE_TRUNC:
	 EMIT_1ARG_ARITH(A0_TRC);
	 break;

      case OPCODE_FRC:
         EMIT_1ARG_ARITH(A0_FRC);
         break;

      case OPCODE_KIL:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         tmp = i915_get_utemp(p);

         i915_emit_texld(p, get_live_regs(p, inst),
                         tmp, A0_DEST_CHANNEL_ALL,   /* use a dummy dest reg */
                         0, src0, T0_TEXKILL);
         break;

      case OPCODE_KIL_NV:
	 if (inst->DstReg.CondMask == COND_TR) {
	    tmp = i915_get_utemp(p);

	    /* The KIL instruction discards the fragment if any component of
	     * the source is < 0.  Emit an immediate operand of {-1}.xywz.
	     */
	    i915_emit_texld(p, get_live_regs(p, inst),
			    tmp, A0_DEST_CHANNEL_ALL,
			    0, /* use a dummy dest reg */
			    negate(swizzle(tmp, ONE, ONE, ONE, ONE),
				   1, 1, 1, 1),
			    T0_TEXKILL);
	 } else {
	    p->error = 1;
	    i915_program_error(p, "Unsupported KIL_NV condition code: %d",
			       inst->DstReg.CondMask);
	 }
	 break;

      case OPCODE_LG2:
         src0 = src_vector(p, &inst->SrcReg[0], program);

         i915_emit_arith(p,
                         A0_LOG,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         swizzle(src0, X, X, X, X), 0, 0);
         break;

      case OPCODE_LIT:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         tmp = i915_get_utemp(p);

         /* tmp = max( a.xyzw, a.00zw )
          * XXX: Clamp tmp.w to -128..128
          * tmp.y = log(tmp.y)
          * tmp.y = tmp.w * tmp.y
          * tmp.y = exp(tmp.y)
          * result = cmp (a.11-x1, a.1x01, a.1xy1 )
          */
         i915_emit_arith(p, A0_MAX, tmp, A0_DEST_CHANNEL_ALL, 0,
                         src0, swizzle(src0, ZERO, ZERO, Z, W), 0);

         i915_emit_arith(p, A0_LOG, tmp, A0_DEST_CHANNEL_Y, 0,
                         swizzle(tmp, Y, Y, Y, Y), 0, 0);

         i915_emit_arith(p, A0_MUL, tmp, A0_DEST_CHANNEL_Y, 0,
                         swizzle(tmp, ZERO, Y, ZERO, ZERO),
                         swizzle(tmp, ZERO, W, ZERO, ZERO), 0);

         i915_emit_arith(p, A0_EXP, tmp, A0_DEST_CHANNEL_Y, 0,
                         swizzle(tmp, Y, Y, Y, Y), 0, 0);

         i915_emit_arith(p, A0_CMP,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         negate(swizzle(tmp, ONE, ONE, X, ONE), 0, 0, 1, 0),
                         swizzle(tmp, ONE, X, ZERO, ONE),
                         swizzle(tmp, ONE, X, Y, ONE));

         break;

      case OPCODE_LRP:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);
         src2 = src_vector(p, &inst->SrcReg[2], program);
         flags = get_result_flags(inst);
         tmp = i915_get_utemp(p);

         /* b*a + c*(1-a)
          *
          * b*a + c - ca 
          *
          * tmp = b*a + c, 
          * result = (-c)*a + tmp 
          */
         i915_emit_arith(p, A0_MAD, tmp,
                         flags & A0_DEST_CHANNEL_ALL, 0, src1, src0, src2);

         i915_emit_arith(p, A0_MAD,
                         get_result_vector(p, inst),
                         flags, 0, negate(src2, 1, 1, 1, 1), src0, tmp);
         break;

      case OPCODE_MAD:
         EMIT_3ARG_ARITH(A0_MAD);
         break;

      case OPCODE_MAX:
         EMIT_2ARG_ARITH(A0_MAX);
         break;

      case OPCODE_MIN:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);
         tmp = i915_get_utemp(p);
         flags = get_result_flags(inst);

         i915_emit_arith(p,
                         A0_MAX,
                         tmp, flags & A0_DEST_CHANNEL_ALL, 0,
                         negate(src0, 1, 1, 1, 1),
                         negate(src1, 1, 1, 1, 1), 0);

         i915_emit_arith(p,
                         A0_MOV,
                         get_result_vector(p, inst),
                         flags, 0, negate(tmp, 1, 1, 1, 1), 0, 0);
         break;

      case OPCODE_MOV:
         EMIT_1ARG_ARITH(A0_MOV);
         break;

      case OPCODE_MUL:
         EMIT_2ARG_ARITH(A0_MUL);
         break;

      case OPCODE_POW:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);
         tmp = i915_get_utemp(p);
         flags = get_result_flags(inst);

         /* XXX: masking on intermediate values, here and elsewhere.
          */
         i915_emit_arith(p,
                         A0_LOG,
                         tmp, A0_DEST_CHANNEL_X, 0,
                         swizzle(src0, X, X, X, X), 0, 0);

         i915_emit_arith(p, A0_MUL, tmp, A0_DEST_CHANNEL_X, 0, tmp, src1, 0);


         i915_emit_arith(p,
                         A0_EXP,
                         get_result_vector(p, inst),
                         flags, 0, swizzle(tmp, X, X, X, X), 0, 0);

         break;

      case OPCODE_RCP:
         src0 = src_vector(p, &inst->SrcReg[0], program);

         i915_emit_arith(p,
                         A0_RCP,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         swizzle(src0, X, X, X, X), 0, 0);
         break;

      case OPCODE_RSQ:

         src0 = src_vector(p, &inst->SrcReg[0], program);

         i915_emit_arith(p,
                         A0_RSQ,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         swizzle(src0, X, X, X, X), 0, 0);
         break;

      case OPCODE_SCS:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         tmp = i915_get_utemp(p);

         /* 
          * t0.xy = MUL x.xx11, x.x1111  ; x^2, x, 1, 1
          * t0 = MUL t0.xyxy t0.xx11 ; x^4, x^3, x^2, x
          * t1 = MUL t0.xyyw t0.yz11    ; x^7 x^5 x^3 x
          * scs.x = DP4 t1, sin_constants
          * t1 = MUL t0.xxz1 t0.z111    ; x^6 x^4 x^2 1
          * scs.y = DP4 t1, cos_constants
          */
         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_XY, 0,
                         swizzle(src0, X, X, ONE, ONE),
                         swizzle(src0, X, ONE, ONE, ONE), 0);

         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_ALL, 0,
                         swizzle(tmp, X, Y, X, Y),
                         swizzle(tmp, X, X, ONE, ONE), 0);

         if (inst->DstReg.WriteMask & WRITEMASK_Y) {
            GLuint tmp1;

            if (inst->DstReg.WriteMask & WRITEMASK_X)
               tmp1 = i915_get_utemp(p);
            else
               tmp1 = tmp;

            i915_emit_arith(p,
                            A0_MUL,
                            tmp1, A0_DEST_CHANNEL_ALL, 0,
                            swizzle(tmp, X, Y, Y, W),
                            swizzle(tmp, X, Z, ONE, ONE), 0);

            i915_emit_arith(p,
                            A0_DP4,
                            get_result_vector(p, inst),
                            A0_DEST_CHANNEL_Y, 0,
                            swizzle(tmp1, W, Z, Y, X),
                            i915_emit_const4fv(p, sin_constants), 0);
         }

         if (inst->DstReg.WriteMask & WRITEMASK_X) {
            i915_emit_arith(p,
                            A0_MUL,
                            tmp, A0_DEST_CHANNEL_XYZ, 0,
                            swizzle(tmp, X, X, Z, ONE),
                            swizzle(tmp, Z, ONE, ONE, ONE), 0);

            i915_emit_arith(p,
                            A0_DP4,
                            get_result_vector(p, inst),
                            A0_DEST_CHANNEL_X, 0,
                            swizzle(tmp, ONE, Z, Y, X),
                            i915_emit_const4fv(p, cos_constants), 0);
         }
         break;

      case OPCODE_SEQ:
	 tmp = i915_get_utemp(p);
	 flags = get_result_flags(inst);
	 dst = get_result_vector(p, inst);

	 /* tmp = src1 >= src2 */
	 i915_emit_arith(p,
			 A0_SGE,
			 tmp,
			 flags, 0,
			 src_vector(p, &inst->SrcReg[0], program),
			 src_vector(p, &inst->SrcReg[1], program),
			 0);
	 /* dst = src1 <= src2 */
	 i915_emit_arith(p,
			 A0_SGE,
			 dst,
			 flags, 0,
			 negate(src_vector(p, &inst->SrcReg[0], program),
				1, 1, 1, 1),
			 negate(src_vector(p, &inst->SrcReg[1], program),
				1, 1, 1, 1),
			 0);
	 /* dst = tmp && dst */
	 i915_emit_arith(p,
			 A0_MUL,
			 dst,
			 flags, 0,
			 dst,
			 tmp,
			 0);
	 break;

      case OPCODE_SIN:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         tmp = i915_get_utemp(p);
	 consts0 = i915_emit_const4fv(p, sin_quad_constants[0]);
	 consts1 = i915_emit_const4fv(p, sin_quad_constants[1]);

	 /* Reduce range from repeating about [-pi,pi] to [-1,1] */
         i915_emit_arith(p,
                         A0_MAD,
                         tmp, A0_DEST_CHANNEL_X, 0,
                         src0,
			 swizzle(consts1, Z, ZERO, ZERO, ZERO), /* 1/(2pi) */
			 swizzle(consts0, Z, ZERO, ZERO, ZERO)); /* .5 */

         i915_emit_arith(p, A0_FRC, tmp, A0_DEST_CHANNEL_X, 0, tmp, 0, 0);

	 i915_emit_arith(p,
			 A0_MAD,
			 tmp, A0_DEST_CHANNEL_X, 0,
			 tmp,
			 swizzle(consts0, X, ZERO, ZERO, ZERO), /* 2 */
			 swizzle(consts0, Y, ZERO, ZERO, ZERO)); /* -1 */

	 /* Compute sin using a quadratic and quartic.  It gives continuity
	  * that repeating the Taylor series lacks every 2*pi, and has
	  * reduced error.
	  *
	  * The idea was described at:
	  * http://www.devmaster.net/forums/showthread.php?t=5784
	  */

	 /* tmp.y = abs(tmp.x); {x, abs(x), 0, 0} */
	 i915_emit_arith(p,
                         A0_MAX,
			 tmp, A0_DEST_CHANNEL_Y, 0,
			 swizzle(tmp, ZERO, X, ZERO, ZERO),
			 negate(swizzle(tmp, ZERO, X, ZERO, ZERO), 0, 1, 0, 0),
			 0);

	 /* tmp.y = tmp.y * tmp.x; {x, x * abs(x), 0, 0} */
	 i915_emit_arith(p,
			 A0_MUL,
			 tmp, A0_DEST_CHANNEL_Y, 0,
			 swizzle(tmp, ZERO, X, ZERO, ZERO),
			 tmp,
			 0);

	 /* tmp.x = tmp.xy DP sin_quad_constants[2].xy */
         i915_emit_arith(p,
                         A0_DP3,
                         tmp, A0_DEST_CHANNEL_X, 0,
			 tmp,
                         swizzle(consts1, X, Y, ZERO, ZERO),
			 0);

	 /* tmp.x now contains a first approximation (y).  Now, weight it
	  * against tmp.y**2 to get closer.
	  */
	 i915_emit_arith(p,
                         A0_MAX,
			 tmp, A0_DEST_CHANNEL_Y, 0,
			 swizzle(tmp, ZERO, X, ZERO, ZERO),
			 negate(swizzle(tmp, ZERO, X, ZERO, ZERO), 0, 1, 0, 0),
			 0);

	 /* tmp.y = tmp.x * tmp.y - tmp.x; {y, y * abs(y) - y, 0, 0} */
	 i915_emit_arith(p,
			 A0_MAD,
			 tmp, A0_DEST_CHANNEL_Y, 0,
			 swizzle(tmp, ZERO, X, ZERO, ZERO),
			 swizzle(tmp, ZERO, Y, ZERO, ZERO),
			 negate(swizzle(tmp, ZERO, X, ZERO, ZERO), 0, 1, 0, 0));

	 /* result = .2225 * tmp.y + tmp.x =.2225(y * abs(y) - y) + y= */
	 i915_emit_arith(p,
			 A0_MAD,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
			 swizzle(consts1, W, W, W, W),
			 swizzle(tmp, Y, Y, Y, Y),
			 swizzle(tmp, X, X, X, X));

         break;

      case OPCODE_SGE:
	 EMIT_2ARG_ARITH(A0_SGE);
	 break;

      case OPCODE_SGT:
	 i915_emit_arith(p,
			 A0_SLT,
			 get_result_vector( p, inst ),
			 get_result_flags( inst ), 0,
			 negate(src_vector( p, &inst->SrcReg[0], program),
				1, 1, 1, 1),
			 negate(src_vector( p, &inst->SrcReg[1], program),
				1, 1, 1, 1),
			 0);
         break;

      case OPCODE_SLE:
	 i915_emit_arith(p,
			 A0_SGE,
			 get_result_vector( p, inst ),
			 get_result_flags( inst ), 0,
			 negate(src_vector( p, &inst->SrcReg[0], program),
				1, 1, 1, 1),
			 negate(src_vector( p, &inst->SrcReg[1], program),
				1, 1, 1, 1),
			 0);
         break;

      case OPCODE_SLT:
         EMIT_2ARG_ARITH(A0_SLT);
         break;

      case OPCODE_SNE:
	 tmp = i915_get_utemp(p);
	 flags = get_result_flags(inst);
	 dst = get_result_vector(p, inst);

	 /* tmp = src1 < src2 */
	 i915_emit_arith(p,
			 A0_SLT,
			 tmp,
			 flags, 0,
			 src_vector(p, &inst->SrcReg[0], program),
			 src_vector(p, &inst->SrcReg[1], program),
			 0);
	 /* dst = src1 > src2 */
	 i915_emit_arith(p,
			 A0_SLT,
			 dst,
			 flags, 0,
			 negate(src_vector(p, &inst->SrcReg[0], program),
				1, 1, 1, 1),
			 negate(src_vector(p, &inst->SrcReg[1], program),
				1, 1, 1, 1),
			 0);
	 /* dst = tmp || dst */
	 i915_emit_arith(p,
			 A0_ADD,
			 dst,
			 flags | A0_DEST_SATURATE, 0,
			 dst,
			 tmp,
			 0);
         break;

      case OPCODE_SSG:
	 dst = get_result_vector(p, inst);
	 flags = get_result_flags(inst);
         src0 = src_vector(p, &inst->SrcReg[0], program);
	 tmp = i915_get_utemp(p);

	 /* tmp = (src < 0.0) */
	 i915_emit_arith(p,
			 A0_SLT,
			 tmp,
			 flags, 0,
			 src0,
			 swizzle(src0, ZERO, ZERO, ZERO, ZERO),
			 0);

	 /* dst = (0.0 < src) */
	 i915_emit_arith(p,
			 A0_SLT,
			 dst,
			 flags, 0,
			 swizzle(src0, ZERO, ZERO, ZERO, ZERO),
			 src0,
			 0);

	 /* dst = (src > 0.0) - (src < 0.0) */
	 i915_emit_arith(p,
			 A0_ADD,
			 dst,
			 flags, 0,
			 dst,
			 negate(tmp, 1, 1, 1, 1),
			 0);

         break;

      case OPCODE_SUB:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);

         i915_emit_arith(p,
                         A0_ADD,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         src0, negate(src1, 1, 1, 1, 1), 0);
         break;

      case OPCODE_SWZ:
         EMIT_1ARG_ARITH(A0_MOV);       /* extended swizzle handled natively */
         break;

      case OPCODE_TEX:
         EMIT_TEX(T0_TEXLD);
         break;

      case OPCODE_TXB:
         EMIT_TEX(T0_TEXLDB);
         break;

      case OPCODE_TXP:
         EMIT_TEX(T0_TEXLDP);
         break;

      case OPCODE_XPD:
         /* Cross product:
          *      result.x = src0.y * src1.z - src0.z * src1.y;
          *      result.y = src0.z * src1.x - src0.x * src1.z;
          *      result.z = src0.x * src1.y - src0.y * src1.x;
          *      result.w = undef;
          */
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);
         tmp = i915_get_utemp(p);

         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_ALL, 0,
                         swizzle(src0, Z, X, Y, ONE),
                         swizzle(src1, Y, Z, X, ONE), 0);

         i915_emit_arith(p,
                         A0_MAD,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         swizzle(src0, Y, Z, X, ONE),
                         swizzle(src1, Z, X, Y, ONE),
                         negate(tmp, 1, 1, 1, 0));
         break;

      case OPCODE_END:
         return;

      case OPCODE_BGNLOOP:
      case OPCODE_BGNSUB:
      case OPCODE_BRA:
      case OPCODE_BRK:
      case OPCODE_CAL:
      case OPCODE_CONT:
      case OPCODE_DDX:
      case OPCODE_DDY:
      case OPCODE_ELSE:
      case OPCODE_ENDIF:
      case OPCODE_ENDLOOP:
      case OPCODE_ENDSUB:
      case OPCODE_IF:
      case OPCODE_RET:
	 p->error = 1;
	 i915_program_error(p, "Unsupported opcode: %s",
			    _mesa_opcode_string(inst->Opcode));
	 return;

      case OPCODE_EXP:
      case OPCODE_LOG:
	 /* These opcodes are claimed as GLSL, NV_vp, and ARB_vp in
	  * prog_instruction.h, but apparently GLSL doesn't ever emit them.
	  * Instead, it translates to EX2 or LG2.
	  */
      case OPCODE_TXD:
      case OPCODE_TXL:
	 /* These opcodes are claimed by GLSL in prog_instruction.h, but
	  * only NV_vp/fp appears to emit them.
	  */
      default:
         i915_program_error(p, "bad opcode: %s",
			    _mesa_opcode_string(inst->Opcode));
         return;
      }

      inst++;
      i915_release_utemps(p);
   }
}

/* Rather than trying to intercept and jiggle depth writes during
 * emit, just move the value into its correct position at the end of
 * the program:
 */
static void
fixup_depth_write(struct i915_fragment_program *p)
{
   if (p->depth_written) {
      GLuint depth = UREG(REG_TYPE_OD, 0);

      i915_emit_arith(p,
                      A0_MOV,
                      depth, A0_DEST_CHANNEL_W, 0,
                      swizzle(depth, X, Y, Z, Z), 0, 0);
   }
}


static void
check_wpos(struct i915_fragment_program *p)
{
   GLbitfield64 inputs = p->FragProg.Base.InputsRead;
   GLint i;

   p->wpos_tex = -1;

   for (i = 0; i < p->ctx->Const.MaxTextureCoordUnits; i++) {
      if (inputs & (FRAG_BIT_TEX(i) | FRAG_BIT_VAR(i)))
         continue;
      else if (inputs & FRAG_BIT_WPOS) {
         p->wpos_tex = i;
         inputs &= ~FRAG_BIT_WPOS;
      }
   }

   if (inputs & FRAG_BIT_WPOS) {
      i915_program_error(p, "No free texcoord for wpos value");
   }
}


static void
translate_program(struct i915_fragment_program *p)
{
   struct i915_context *i915 = I915_CONTEXT(p->ctx);

   if (INTEL_DEBUG & DEBUG_WM) {
      printf("fp:\n");
      _mesa_print_program(&p->FragProg.Base);
      printf("\n");
   }

   i915_init_program(i915, p);
   check_wpos(p);
   upload_program(p);
   fixup_depth_write(p);
   i915_fini_program(p);

   p->translated = 1;
}


static void
track_params(struct i915_fragment_program *p)
{
   GLint i;

   if (p->nr_params)
      _mesa_load_state_parameters(p->ctx, p->FragProg.Base.Parameters);

   for (i = 0; i < p->nr_params; i++) {
      GLint reg = p->param[i].reg;
      COPY_4V(p->constant[reg], p->param[i].values);
   }

   p->params_uptodate = 1;
   p->on_hardware = 0;          /* overkill */
}


static void
i915BindProgram(struct gl_context * ctx, GLenum target, struct gl_program *prog)
{
   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      struct i915_context *i915 = I915_CONTEXT(ctx);
      struct i915_fragment_program *p = (struct i915_fragment_program *) prog;

      if (i915->current_program == p)
         return;

      if (i915->current_program) {
         i915->current_program->on_hardware = 0;
         i915->current_program->params_uptodate = 0;
      }

      i915->current_program = p;

      assert(p->on_hardware == 0);
      assert(p->params_uptodate == 0);

   }
}

static struct gl_program *
i915NewProgram(struct gl_context * ctx, GLenum target, GLuint id)
{
   switch (target) {
   case GL_VERTEX_PROGRAM_ARB:
      return _mesa_init_vertex_program(ctx, CALLOC_STRUCT(gl_vertex_program),
                                       target, id);

   case GL_FRAGMENT_PROGRAM_ARB:{
         struct i915_fragment_program *prog =
            CALLOC_STRUCT(i915_fragment_program);
         if (prog) {
            i915_init_program(I915_CONTEXT(ctx), prog);

            return _mesa_init_fragment_program(ctx, &prog->FragProg,
                                               target, id);
         }
         else
            return NULL;
      }

   default:
      /* Just fallback:
       */
      return _mesa_new_program(ctx, target, id);
   }
}

static void
i915DeleteProgram(struct gl_context * ctx, struct gl_program *prog)
{
   if (prog->Target == GL_FRAGMENT_PROGRAM_ARB) {
      struct i915_context *i915 = I915_CONTEXT(ctx);
      struct i915_fragment_program *p = (struct i915_fragment_program *) prog;

      if (i915->current_program == p)
         i915->current_program = 0;
   }

   _mesa_delete_program(ctx, prog);
}


static GLboolean
i915IsProgramNative(struct gl_context * ctx, GLenum target, struct gl_program *prog)
{
   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      struct i915_fragment_program *p = (struct i915_fragment_program *) prog;

      if (!p->translated)
         translate_program(p);

      return !p->error;
   }
   else
      return true;
}

static GLboolean
i915ProgramStringNotify(struct gl_context * ctx,
                        GLenum target, struct gl_program *prog)
{
   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      struct i915_fragment_program *p = (struct i915_fragment_program *) prog;
      p->translated = 0;
   }

   (void) _tnl_program_string(ctx, target, prog);

   /* XXX check if program is legal, within limits */
   return true;
}

static void
i915SamplerUniformChange(struct gl_context *ctx,
                         GLenum target, struct gl_program *prog)
{
   i915ProgramStringNotify(ctx, target, prog);
}

void
i915_update_program(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);
   struct i915_context *i915 = i915_context(&intel->ctx);
   struct i915_fragment_program *fp =
      (struct i915_fragment_program *) ctx->FragmentProgram._Current;

   if (i915->current_program != fp) {
      if (i915->current_program) {
         i915->current_program->on_hardware = 0;
         i915->current_program->params_uptodate = 0;
      }

      i915->current_program = fp;
   }

   if (!fp->translated)
      translate_program(fp);

   FALLBACK(&i915->intel, I915_FALLBACK_PROGRAM, fp->error);
}

void
i915ValidateFragmentProgram(struct i915_context *i915)
{
   struct gl_context *ctx = &i915->intel.ctx;
   struct intel_context *intel = intel_context(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;

   struct i915_fragment_program *p =
      (struct i915_fragment_program *) ctx->FragmentProgram._Current;

   const GLbitfield64 inputsRead = p->FragProg.Base.InputsRead;
   GLuint s4 = i915->state.Ctx[I915_CTXREG_LIS4] & ~S4_VFMT_MASK;
   GLuint s2 = S2_TEXCOORD_NONE;
   int i, offset = 0;

   /* Important:
    */
   VB->AttribPtr[VERT_ATTRIB_POS] = VB->NdcPtr;

   if (!p->translated)
      translate_program(p);

   intel->vertex_attr_count = 0;
   intel->wpos_offset = 0;
   intel->coloroffset = 0;
   intel->specoffset = 0;

   if (inputsRead & FRAG_BITS_TEX_ANY || p->wpos_tex != -1) {
      EMIT_ATTR(_TNL_ATTRIB_POS, EMIT_4F_VIEWPORT, S4_VFMT_XYZW, 16);
   }
   else {
      EMIT_ATTR(_TNL_ATTRIB_POS, EMIT_3F_VIEWPORT, S4_VFMT_XYZ, 12);
   }

   /* Handle gl_PointSize builtin var here */
   if (ctx->Point._Attenuated || ctx->VertexProgram.PointSizeEnabled)
      EMIT_ATTR(_TNL_ATTRIB_POINTSIZE, EMIT_1F, S4_VFMT_POINT_WIDTH, 4);

   if (inputsRead & FRAG_BIT_COL0) {
      intel->coloroffset = offset / 4;
      EMIT_ATTR(_TNL_ATTRIB_COLOR0, EMIT_4UB_4F_BGRA, S4_VFMT_COLOR, 4);
   }

   if (inputsRead & FRAG_BIT_COL1) {
       intel->specoffset = offset / 4;
       EMIT_ATTR(_TNL_ATTRIB_COLOR1, EMIT_4UB_4F_BGRA, S4_VFMT_SPEC_FOG, 4);
   }

   if ((inputsRead & FRAG_BIT_FOGC)) {
      EMIT_ATTR(_TNL_ATTRIB_FOG, EMIT_1F, S4_VFMT_FOG_PARAM, 4);
   }

   for (i = 0; i < p->ctx->Const.MaxTextureCoordUnits; i++) {
      if (inputsRead & FRAG_BIT_TEX(i)) {
         int sz = VB->AttribPtr[_TNL_ATTRIB_TEX0 + i]->size;

         s2 &= ~S2_TEXCOORD_FMT(i, S2_TEXCOORD_FMT0_MASK);
         s2 |= S2_TEXCOORD_FMT(i, SZ_TO_HW(sz));

         EMIT_ATTR(_TNL_ATTRIB_TEX0 + i, EMIT_SZ(sz), 0, sz * 4);
      }
      else if (inputsRead & FRAG_BIT_VAR(i)) {
         int sz = VB->AttribPtr[_TNL_ATTRIB_GENERIC0 + i]->size;

         s2 &= ~S2_TEXCOORD_FMT(i, S2_TEXCOORD_FMT0_MASK);
         s2 |= S2_TEXCOORD_FMT(i, SZ_TO_HW(sz));

         EMIT_ATTR(_TNL_ATTRIB_GENERIC0 + i, EMIT_SZ(sz), 0, sz * 4);
      }
      else if (i == p->wpos_tex) {
	 int wpos_size = 4 * sizeof(float);
         /* If WPOS is required, duplicate the XYZ position data in an
          * unused texture coordinate:
          */
         s2 &= ~S2_TEXCOORD_FMT(i, S2_TEXCOORD_FMT0_MASK);
         s2 |= S2_TEXCOORD_FMT(i, SZ_TO_HW(wpos_size));

         intel->wpos_offset = offset;
         EMIT_PAD(wpos_size);
      }
   }

   if (s2 != i915->state.Ctx[I915_CTXREG_LIS2] ||
       s4 != i915->state.Ctx[I915_CTXREG_LIS4]) {
      int k;

      I915_STATECHANGE(i915, I915_UPLOAD_CTX);

      /* Must do this *after* statechange, so as not to affect
       * buffered vertices reliant on the old state:
       */
      intel->vertex_size = _tnl_install_attrs(&intel->ctx,
                                              intel->vertex_attrs,
                                              intel->vertex_attr_count,
                                              intel->ViewportMatrix.m, 0);

      assert(intel->prim.current_offset == intel->prim.start_offset);
      intel->prim.start_offset = (intel->prim.current_offset + intel->vertex_size-1) / intel->vertex_size * intel->vertex_size;
      intel->prim.current_offset = intel->prim.start_offset;

      intel->vertex_size >>= 2;

      i915->state.Ctx[I915_CTXREG_LIS2] = s2;
      i915->state.Ctx[I915_CTXREG_LIS4] = s4;

      k = intel->vtbl.check_vertex_size(intel, intel->vertex_size);
      assert(k);
   }

   if (!p->params_uptodate)
      track_params(p);

   if (!p->on_hardware)
      i915_upload_program(i915, p);

   if (INTEL_DEBUG & DEBUG_WM) {
      printf("i915:\n");
      i915_disassemble_program(i915->state.Program, i915->state.ProgramSize);
   }
}

void
i915InitFragProgFuncs(struct dd_function_table *functions)
{
   functions->BindProgram = i915BindProgram;
   functions->NewProgram = i915NewProgram;
   functions->DeleteProgram = i915DeleteProgram;
   functions->IsProgramNative = i915IsProgramNative;
   functions->ProgramStringNotify = i915ProgramStringNotify;
   functions->SamplerUniformChange = i915SamplerUniformChange;
}
