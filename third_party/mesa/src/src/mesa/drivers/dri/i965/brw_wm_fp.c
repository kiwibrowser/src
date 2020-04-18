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
               

#include "main/glheader.h"
#include "main/macros.h"
#include "main/enums.h"
#include "brw_context.h"
#include "brw_wm.h"
#include "brw_util.h"

#include "program/prog_parameter.h"
#include "program/prog_print.h"
#include "program/prog_statevars.h"


/** An invalid texture target */
#define TEX_TARGET_NONE NUM_TEXTURE_TARGETS

/** An invalid texture unit */
#define TEX_UNIT_NONE BRW_MAX_TEX_UNIT

#define FIRST_INTERNAL_TEMP MAX_NV_FRAGMENT_PROGRAM_TEMPS

#define X    0
#define Y    1
#define Z    2
#define W    3


static const char *wm_opcode_strings[] = {   
   "PIXELXY",
   "DELTAXY",
   "PIXELW",
   "LINTERP",
   "PINTERP",
   "CINTERP",
   "WPOSXY",
   "FB_WRITE",
   "FRONTFACING",
};

#if 0
static const char *wm_file_strings[] = {   
   "PAYLOAD"
};
#endif


/***********************************************************************
 * Source regs
 */

static struct prog_src_register src_reg(GLuint file, GLuint idx)
{
   struct prog_src_register reg;
   reg.File = file;
   reg.Index = idx;
   reg.Swizzle = SWIZZLE_NOOP;
   reg.RelAddr = 0;
   reg.Negate = NEGATE_NONE;
   reg.Abs = 0;
   reg.HasIndex2 = 0;
   reg.RelAddr2 = 0;
   reg.Index2 = 0;
   return reg;
}

static struct prog_src_register src_reg_from_dst(struct prog_dst_register dst)
{
   return src_reg(dst.File, dst.Index);
}

static struct prog_src_register src_undef( void )
{
   return src_reg(PROGRAM_UNDEFINED, 0);
}

static bool src_is_undef(struct prog_src_register src)
{
   return src.File == PROGRAM_UNDEFINED;
}

static struct prog_src_register src_swizzle( struct prog_src_register reg, int x, int y, int z, int w )
{
   reg.Swizzle = MAKE_SWIZZLE4(x,y,z,w);
   return reg;
}

static struct prog_src_register src_swizzle1( struct prog_src_register reg, int x )
{
   return src_swizzle(reg, x, x, x, x);
}

static struct prog_src_register src_swizzle4( struct prog_src_register reg, uint swizzle )
{
   reg.Swizzle = swizzle;
   return reg;
}


/***********************************************************************
 * Dest regs
 */

static struct prog_dst_register dst_reg(GLuint file, GLuint idx)
{
   struct prog_dst_register reg;
   reg.File = file;
   reg.Index = idx;
   reg.WriteMask = WRITEMASK_XYZW;
   reg.RelAddr = 0;
   reg.CondMask = COND_TR;
   reg.CondSwizzle = 0;
   reg.CondSrc = 0;
   return reg;
}

static struct prog_dst_register dst_mask( struct prog_dst_register reg, int mask )
{
   reg.WriteMask &= mask;
   return reg;
}

static struct prog_dst_register dst_undef( void )
{
   return dst_reg(PROGRAM_UNDEFINED, 0);
}



static struct prog_dst_register get_temp( struct brw_wm_compile *c )
{
   int bit = ffs( ~c->fp_temp );

   if (!bit) {
      printf("%s: out of temporaries\n", __FILE__);
      exit(1);
   }

   c->fp_temp |= 1<<(bit-1);
   return dst_reg(PROGRAM_TEMPORARY, FIRST_INTERNAL_TEMP+(bit-1));
}


static void release_temp( struct brw_wm_compile *c, struct prog_dst_register temp )
{
   c->fp_temp &= ~(1 << (temp.Index - FIRST_INTERNAL_TEMP));
}


/***********************************************************************
 * Instructions 
 */

static struct prog_instruction *get_fp_inst(struct brw_wm_compile *c)
{
   assert(c->nr_fp_insns < BRW_WM_MAX_INSN);
   memset(&c->prog_instructions[c->nr_fp_insns], 0,
	  sizeof(*c->prog_instructions));
   return &c->prog_instructions[c->nr_fp_insns++];
}

static struct prog_instruction *emit_insn(struct brw_wm_compile *c,
					const struct prog_instruction *inst0)
{
   struct prog_instruction *inst = get_fp_inst(c);
   *inst = *inst0;
   return inst;
}

static struct prog_instruction * emit_tex_op(struct brw_wm_compile *c,
				       GLuint op,
				       struct prog_dst_register dest,
				       GLuint saturate,
				       GLuint tex_src_unit,
				       GLuint tex_src_target,
				       GLuint tex_shadow,
				       struct prog_src_register src0,
				       struct prog_src_register src1,
				       struct prog_src_register src2 )
{
   struct prog_instruction *inst = get_fp_inst(c);
      
   assert(tex_src_unit < BRW_MAX_TEX_UNIT ||
          tex_src_unit == TEX_UNIT_NONE);
   assert(tex_src_target < NUM_TEXTURE_TARGETS ||
          tex_src_target == TEX_TARGET_NONE);

   memset(inst, 0, sizeof(*inst));

   inst->Opcode = op;
   inst->DstReg = dest;
   inst->SaturateMode = saturate;   
   inst->TexSrcUnit = tex_src_unit;
   inst->TexSrcTarget = tex_src_target;
   inst->TexShadow = tex_shadow;
   inst->SrcReg[0] = src0;
   inst->SrcReg[1] = src1;
   inst->SrcReg[2] = src2;
   return inst;
}
   

static struct prog_instruction * emit_op(struct brw_wm_compile *c,
				       GLuint op,
				       struct prog_dst_register dest,
				       GLuint saturate,
				       struct prog_src_register src0,
				       struct prog_src_register src1,
				       struct prog_src_register src2 )
{
   return emit_tex_op(c, op, dest, saturate,
                      TEX_UNIT_NONE, TEX_TARGET_NONE, 0,  /* unit, tgt, shadow */
                      src0, src1, src2);
}


/* Many Mesa opcodes produce the same value across all the result channels.
 * We'd rather not have to support that splatting in the opcode implementations,
 * and brw_wm_pass*.c wants to optimize them out by shuffling references around
 * anyway.  We can easily get both by emitting the opcode to one channel, and
 * then MOVing it to the others, which brw_wm_pass*.c already understands.
 */
static struct prog_instruction *emit_scalar_insn(struct brw_wm_compile *c,
						 const struct prog_instruction *inst0)
{
   struct prog_instruction *inst;
   unsigned int dst_chan;
   unsigned int other_channel_mask;

   if (inst0->DstReg.WriteMask == 0)
      return NULL;

   dst_chan = ffs(inst0->DstReg.WriteMask) - 1;
   inst = get_fp_inst(c);
   *inst = *inst0;
   inst->DstReg.WriteMask = 1 << dst_chan;

   other_channel_mask = inst0->DstReg.WriteMask & ~(1 << dst_chan);
   if (other_channel_mask != 0) {
      inst = emit_op(c,
		     OPCODE_MOV,
		     dst_mask(inst0->DstReg, other_channel_mask),
		     0,
		     src_swizzle1(src_reg_from_dst(inst0->DstReg), dst_chan),
		     src_undef(),
		     src_undef());
   }
   return inst;
}


/***********************************************************************
 * Special instructions for interpolation and other tasks
 */

static struct prog_src_register get_pixel_xy( struct brw_wm_compile *c )
{
   if (src_is_undef(c->pixel_xy)) {
      struct prog_dst_register pixel_xy = get_temp(c);
      struct prog_src_register payload_r0_depth = src_reg(PROGRAM_PAYLOAD, PAYLOAD_DEPTH);
      
      
      /* Emit the out calculations, and hold onto the results.  Use
       * two instructions as a temporary is required.
       */   
      /* pixel_xy.xy = PIXELXY payload[0];
       */
      emit_op(c,
	      WM_PIXELXY,
	      dst_mask(pixel_xy, WRITEMASK_XY),
	      0,
	      payload_r0_depth,
	      src_undef(),
	      src_undef());

      c->pixel_xy = src_reg_from_dst(pixel_xy);
   }

   return c->pixel_xy;
}

static struct prog_src_register get_delta_xy( struct brw_wm_compile *c )
{
   if (src_is_undef(c->delta_xy)) {
      struct prog_dst_register delta_xy = get_temp(c);
      struct prog_src_register pixel_xy = get_pixel_xy(c);
      struct prog_src_register payload_r0_depth = src_reg(PROGRAM_PAYLOAD, PAYLOAD_DEPTH);
      
      /* deltas.xy = DELTAXY pixel_xy, payload[0]
       */
      emit_op(c,
	      WM_DELTAXY,
	      dst_mask(delta_xy, WRITEMASK_XY),
	      0,
	      pixel_xy, 
	      payload_r0_depth,
	      src_undef());
      
      c->delta_xy = src_reg_from_dst(delta_xy);
   }

   return c->delta_xy;
}

static struct prog_src_register get_pixel_w( struct brw_wm_compile *c )
{
   /* This is called for producing 1/w in pre-gen6 interp.  for gen6,
    * the interp opcodes don't use this argument.  But to keep the
    * nr_args = 3 expectations of pinterp happy, just stuff delta_xy
    * into the slot.
    */
   if (c->func.brw->intel.gen >= 6)
      return c->delta_xy;

   if (src_is_undef(c->pixel_w)) {
      struct prog_dst_register pixel_w = get_temp(c);
      struct prog_src_register deltas = get_delta_xy(c);
      struct prog_src_register interp_wpos = src_reg(PROGRAM_PAYLOAD, FRAG_ATTRIB_WPOS);

      /* deltas.xyw = DELTAS2 deltas.xy, payload.interp_wpos.x
       */
      emit_op(c,
	      WM_PIXELW,
	      dst_mask(pixel_w, WRITEMASK_W),
	      0,
	      interp_wpos,
	      deltas, 
	      src_undef());
      

      c->pixel_w = src_reg_from_dst(pixel_w);
   }

   return c->pixel_w;
}

static void emit_interp( struct brw_wm_compile *c,
			 GLuint idx )
{
   struct prog_dst_register dst = dst_reg(PROGRAM_INPUT, idx);
   struct prog_src_register interp = src_reg(PROGRAM_PAYLOAD, idx);
   struct prog_src_register deltas;

   deltas = get_delta_xy(c);

   /* Need to use PINTERP on attributes which have been
    * multiplied by 1/W in the SF program, and LINTERP on those
    * which have not:
    */
   switch (idx) {
   case FRAG_ATTRIB_WPOS:
      /* Have to treat wpos.xy specially:
       */
      emit_op(c,
	      WM_WPOSXY,
	      dst_mask(dst, WRITEMASK_XY),
	      0,
	      get_pixel_xy(c),
	      src_undef(),
	      src_undef());
      
      dst = dst_mask(dst, WRITEMASK_ZW);

      /* PROGRAM_INPUT.attr.xyzw = INTERP payload.interp[attr].x, deltas.xyw
       */
      emit_op(c,
	      WM_LINTERP,
	      dst,
	      0,
	      interp,
	      deltas,
	      src_undef());
      break;
   case FRAG_ATTRIB_COL0:
   case FRAG_ATTRIB_COL1:
      if (c->key.flat_shade) {
	 emit_op(c,
		 WM_CINTERP,
		 dst,
		 0,
		 interp,
		 src_undef(),
		 src_undef());
      }
      else {
	 /* perspective-corrected color interpolation */
	 emit_op(c,
		 WM_PINTERP,
		 dst,
		 0,
		 interp,
		 deltas,
		 get_pixel_w(c));
      }
      break;
   case FRAG_ATTRIB_FOGC:
      /* Interpolate the fog coordinate */
      emit_op(c,
	      WM_PINTERP,
	      dst_mask(dst, WRITEMASK_X),
	      0,
	      interp,
	      deltas,
	      get_pixel_w(c));

      emit_op(c,
	      OPCODE_MOV,
	      dst_mask(dst, WRITEMASK_YZW),
	      0,
	      src_swizzle(interp,
			  SWIZZLE_ZERO,
			  SWIZZLE_ZERO,
			  SWIZZLE_ZERO,
			  SWIZZLE_ONE),
	      src_undef(),
	      src_undef());
      break;

   case FRAG_ATTRIB_FACE:
      emit_op(c,
              WM_FRONTFACING,
              dst_mask(dst, WRITEMASK_X),
              0,
              src_undef(),
              src_undef(),
              src_undef());
      break;

   case FRAG_ATTRIB_PNTC:
      /* XXX review/test this case */
      emit_op(c,
	      WM_PINTERP,
	      dst_mask(dst, WRITEMASK_XY),
	      0,
	      interp,
	      deltas,
	      get_pixel_w(c));

      emit_op(c,
	      OPCODE_MOV,
	      dst_mask(dst, WRITEMASK_ZW),
	      0,
	      src_swizzle(interp,
			  SWIZZLE_ZERO,
			  SWIZZLE_ZERO,
			  SWIZZLE_ZERO,
			  SWIZZLE_ONE),
	      src_undef(),
	      src_undef());
      break;

   default:
      emit_op(c,
	      WM_PINTERP,
	      dst,
	      0,
	      interp,
	      deltas,
	      get_pixel_w(c));
      break;
   }

   c->fp_interp_emitted |= 1<<idx;
}

/***********************************************************************
 * Hacks to extend the program parameter and constant lists.
 */

/* Add the fog parameters to the parameter list of the original
 * program, rather than creating a new list.  Doesn't really do any
 * harm and it's not as if the parameter handling isn't a big hack
 * anyway.
 */
static struct prog_src_register search_or_add_param5(struct brw_wm_compile *c, 
                                                     GLint s0,
                                                     GLint s1,
                                                     GLint s2,
                                                     GLint s3,
                                                     GLint s4)
{
   struct gl_program_parameter_list *paramList = c->fp->program.Base.Parameters;
   gl_state_index tokens[STATE_LENGTH];
   GLuint idx;
   tokens[0] = s0;
   tokens[1] = s1;
   tokens[2] = s2;
   tokens[3] = s3;
   tokens[4] = s4;

   idx = _mesa_add_state_reference( paramList, tokens );

   return src_reg(PROGRAM_STATE_VAR, idx);
}


static struct prog_src_register search_or_add_const4f( struct brw_wm_compile *c, 
						     GLfloat s0,
						     GLfloat s1,
						     GLfloat s2,
						     GLfloat s3)
{
   struct gl_program_parameter_list *paramList = c->fp->program.Base.Parameters;
   gl_constant_value values[4];
   GLuint idx;
   GLuint swizzle;
   struct prog_src_register reg;

   values[0].f = s0;
   values[1].f = s1;
   values[2].f = s2;
   values[3].f = s3;

   idx = _mesa_add_unnamed_constant( paramList, values, 4, &swizzle );
   reg = src_reg(PROGRAM_STATE_VAR, idx);
   reg.Swizzle = swizzle;

   return reg;
}



/***********************************************************************
 * Expand various instructions here to simpler forms.  
 */
static void precalc_dst( struct brw_wm_compile *c,
			       const struct prog_instruction *inst )
{
   struct prog_src_register src0 = inst->SrcReg[0];
   struct prog_src_register src1 = inst->SrcReg[1];
   struct prog_dst_register dst = inst->DstReg;
   struct prog_dst_register temp = get_temp(c);

   if (dst.WriteMask & WRITEMASK_Y) {      
      /* dst.y = mul src0.y, src1.y
       */
      emit_op(c,
	      OPCODE_MUL,
	      dst_mask(temp, WRITEMASK_Y),
	      inst->SaturateMode,
	      src0,
	      src1,
	      src_undef());
   }

   if (dst.WriteMask & WRITEMASK_XZ) {
      struct prog_instruction *swz;
      GLuint z = GET_SWZ(src0.Swizzle, Z);

      /* dst.xz = swz src0.1zzz
       */
      swz = emit_op(c,
		    OPCODE_SWZ,
		    dst_mask(temp, WRITEMASK_XZ),
		    inst->SaturateMode,
		    src_swizzle(src0, SWIZZLE_ONE, z, z, z),
		    src_undef(),
		    src_undef());
      /* Avoid letting negation flag of src0 affect our 1 constant. */
      swz->SrcReg[0].Negate &= ~NEGATE_X;
   }
   if (dst.WriteMask & WRITEMASK_W) {
      /* dst.w = mov src1.w
       */
      emit_op(c,
	      OPCODE_MOV,
	      dst_mask(temp, WRITEMASK_W),
	      inst->SaturateMode,
	      src1,
	      src_undef(),
	      src_undef());
   }

   /* This will get optimized out in general, but it ensures that we
    * don't overwrite src operands in our channel-wise splitting
    * above.  See piglit fp-dst-aliasing-[12].
    */
   emit_op(c,
	   OPCODE_MOV,
	   dst,
	   0,
	   src_reg_from_dst(temp),
	   src_undef(),
	   src_undef());

   release_temp(c, temp);
}


static void precalc_lit( struct brw_wm_compile *c,
			 const struct prog_instruction *inst )
{
   struct prog_src_register src0 = inst->SrcReg[0];
   struct prog_dst_register dst = inst->DstReg;

   if (dst.WriteMask & WRITEMASK_YZ) {
      emit_op(c,
	      OPCODE_LIT,
	      dst_mask(dst, WRITEMASK_YZ),
	      inst->SaturateMode,
	      src0,
	      src_undef(),
	      src_undef());
   }

   if (dst.WriteMask & WRITEMASK_XW) {
      struct prog_instruction *swz;

      /* dst.xw = swz src0.1111
       */
      swz = emit_op(c,
		    OPCODE_SWZ,
		    dst_mask(dst, WRITEMASK_XW),
		    0,
		    src_swizzle1(src0, SWIZZLE_ONE),
		    src_undef(),
		    src_undef());
      /* Avoid letting the negation flag of src0 affect our 1 constant. */
      swz->SrcReg[0].Negate = NEGATE_NONE;
   }
}


/**
 * Some TEX instructions require extra code, cube map coordinate
 * normalization, or coordinate scaling for RECT textures, etc.
 * This function emits those extra instructions and the TEX
 * instruction itself.
 */
static void precalc_tex( struct brw_wm_compile *c,
			 const struct prog_instruction *inst )
{
   struct brw_compile *p = &c->func;
   struct intel_context *intel = &p->brw->intel;
   struct prog_src_register coord;
   struct prog_dst_register tmpcoord = { 0 };
   const GLuint unit = c->fp->program.Base.SamplerUnits[inst->TexSrcUnit];
   struct prog_dst_register unswizzled_tmp;

   /* If we are doing EXT_texture_swizzle, we need to write our result into a
    * temporary, otherwise writemasking of the real dst could lose some of our
    * channels.
    */
   if (c->key.tex.swizzles[unit] != SWIZZLE_NOOP) {
      unswizzled_tmp = get_temp(c);
   } else {
      unswizzled_tmp = inst->DstReg;
   }

   assert(unit < BRW_MAX_TEX_UNIT);

   if (inst->TexSrcTarget == TEXTURE_CUBE_INDEX) {
       struct prog_instruction *out;
       struct prog_dst_register tmp0 = get_temp(c);
       struct prog_src_register tmp0src = src_reg_from_dst(tmp0);
       struct prog_dst_register tmp1 = get_temp(c);
       struct prog_src_register tmp1src = src_reg_from_dst(tmp1);
       struct prog_src_register src0 = inst->SrcReg[0];

       /* find longest component of coord vector and normalize it */
       tmpcoord = get_temp(c);
       coord = src_reg_from_dst(tmpcoord);

       /* tmpcoord = src0 (i.e.: coord = src0) */
       out = emit_op(c, OPCODE_MOV,
                     tmpcoord,
                     0,
                     src0,
                     src_undef(),
                     src_undef());
       out->SrcReg[0].Negate = NEGATE_NONE;
       out->SrcReg[0].Abs = 1;

       /* tmp0 = MAX(coord.X, coord.Y) */
       emit_op(c, OPCODE_MAX,
               tmp0,
               0,
               src_swizzle1(coord, X),
               src_swizzle1(coord, Y),
               src_undef());

       /* tmp1 = MAX(tmp0, coord.Z) */
       emit_op(c, OPCODE_MAX,
               tmp1,
               0,
               tmp0src,
               src_swizzle1(coord, Z),
               src_undef());

       /* tmp0 = 1 / tmp1 */
       emit_op(c, OPCODE_RCP,
               dst_mask(tmp0, WRITEMASK_X),
               0,
               tmp1src,
               src_undef(),
               src_undef());

       /* tmpCoord = src0 * tmp0 */
       emit_op(c, OPCODE_MUL,
               tmpcoord,
               0,
               src0,
               src_swizzle1(tmp0src, SWIZZLE_X),
               src_undef());

       release_temp(c, tmp0);
       release_temp(c, tmp1);
   }
   else if (intel->gen < 6 && inst->TexSrcTarget == TEXTURE_RECT_INDEX) {
      struct prog_src_register scale = 
	 search_or_add_param5( c, 
			       STATE_INTERNAL, 
			       STATE_TEXRECT_SCALE,
			       unit,
			       0,0 );

      tmpcoord = get_temp(c);

      /* coord.xy   = MUL inst->SrcReg[0], { 1/width, 1/height }
       */
      emit_op(c,
	      OPCODE_MUL,
	      tmpcoord,
	      0,
	      inst->SrcReg[0],
	      src_swizzle(scale,
			  SWIZZLE_X,
			  SWIZZLE_Y,
			  SWIZZLE_ONE,
			  SWIZZLE_ONE),
	      src_undef());

      coord = src_reg_from_dst(tmpcoord);
   }
   else {
      coord = inst->SrcReg[0];
   }

   /* Need to emit YUV texture conversions by hand.  Probably need to
    * do this here - the alternative is in brw_wm_emit.c, but the
    * conversion requires allocating a temporary variable which we
    * don't have the facility to do that late in the compilation.
    */
   if (c->key.tex.yuvtex_mask & (1 << unit)) {
      /* convert ycbcr to RGBA */
      bool swap_uv = c->key.tex.yuvtex_swap_mask & (1 << unit);

      /* 
	 CONST C0 = { -.5, -.0625,  -.5, 1.164 }
	 CONST C1 = { 1.596, -0.813, 2.018, -.391 }
	 UYV     = TEX ...
	 UYV.xyz = ADD UYV,     C0
	 UYV.y   = MUL UYV.y,   C0.w
 	 if (UV swaped)
	    RGB.xyz = MAD UYV.zzx, C1,   UYV.y
	 else
	    RGB.xyz = MAD UYV.xxz, C1,   UYV.y 
	 RGB.y   = MAD UYV.z,   C1.w, RGB.y
      */
      struct prog_dst_register tmp = get_temp(c);
      struct prog_src_register tmpsrc = src_reg_from_dst(tmp);
      struct prog_src_register C0 = search_or_add_const4f( c,  -.5, -.0625, -.5, 1.164 );
      struct prog_src_register C1 = search_or_add_const4f( c, 1.596, -0.813, 2.018, -.391 );
     
      /* tmp     = TEX ...
       */
      emit_tex_op(c, 
                  OPCODE_TEX,
                  tmp,
                  inst->SaturateMode,
                  unit,
                  inst->TexSrcTarget,
                  inst->TexShadow,
                  coord,
                  src_undef(),
                  src_undef());

      /* tmp.xyz =  ADD TMP, C0
       */
      emit_op(c,
	      OPCODE_ADD,
	      dst_mask(tmp, WRITEMASK_XYZ),
	      0,
	      tmpsrc,
	      C0,
	      src_undef());

      /* YUV.y   = MUL YUV.y, C0.w
       */

      emit_op(c,
	      OPCODE_MUL,
	      dst_mask(tmp, WRITEMASK_Y),
	      0,
	      tmpsrc,
	      src_swizzle1(C0, W),
	      src_undef());

      /* 
       * if (UV swaped)
       *     RGB.xyz = MAD YUV.zzx, C1, YUV.y
       * else
       *     RGB.xyz = MAD YUV.xxz, C1, YUV.y
       */

      emit_op(c,
	      OPCODE_MAD,
	      dst_mask(unswizzled_tmp, WRITEMASK_XYZ),
	      0,
	      swap_uv?src_swizzle(tmpsrc, Z,Z,X,X):src_swizzle(tmpsrc, X,X,Z,Z),
	      C1,
	      src_swizzle1(tmpsrc, Y));

      /*  RGB.y   = MAD YUV.z, C1.w, RGB.y
       */
      emit_op(c,
	      OPCODE_MAD,
	      dst_mask(unswizzled_tmp, WRITEMASK_Y),
	      0,
	      src_swizzle1(tmpsrc, Z),
	      src_swizzle1(C1, W),
	      src_swizzle1(src_reg_from_dst(unswizzled_tmp), Y));

      release_temp(c, tmp);
   }
   else {
      /* ordinary RGBA tex instruction */
      emit_tex_op(c, 
                  OPCODE_TEX,
                  unswizzled_tmp,
                  inst->SaturateMode,
                  unit,
                  inst->TexSrcTarget,
                  inst->TexShadow,
                  coord,
                  src_undef(),
                  src_undef());
   }

   /* For GL_EXT_texture_swizzle: */
   if (c->key.tex.swizzles[unit] != SWIZZLE_NOOP) {
      /* swizzle the result of the TEX instruction */
      struct prog_src_register tmpsrc = src_reg_from_dst(unswizzled_tmp);
      emit_op(c, OPCODE_SWZ,
              inst->DstReg,
              SATURATE_OFF, /* saturate already done above */
              src_swizzle4(tmpsrc, c->key.tex.swizzles[unit]),
              src_undef(),
              src_undef());
   }

   if ((inst->TexSrcTarget == TEXTURE_RECT_INDEX) ||
       (inst->TexSrcTarget == TEXTURE_CUBE_INDEX))
      release_temp(c, tmpcoord);
}


/**
 * Check if the given TXP instruction really needs the divide-by-W step.
 */
static bool
projtex(struct brw_wm_compile *c, const struct prog_instruction *inst)
{
   const struct prog_src_register src = inst->SrcReg[0];
   bool retVal;

   assert(inst->Opcode == OPCODE_TXP);

   /* Only try to detect the simplest cases.  Could detect (later)
    * cases where we are trying to emit code like RCP {1.0}, MUL x,
    * {1.0}, and so on.
    *
    * More complex cases than this typically only arise from
    * user-provided fragment programs anyway:
    */
   if (inst->TexSrcTarget == TEXTURE_CUBE_INDEX)
      retVal = false;  /* ut2004 gun rendering !?! */
   else if (src.File == PROGRAM_INPUT && 
	    GET_SWZ(src.Swizzle, W) == W &&
            (c->key.proj_attrib_mask & (1 << src.Index)) == 0)
      retVal = false;
   else
      retVal = true;

   return retVal;
}


/**
 * Emit code for TXP.
 */
static void precalc_txp( struct brw_wm_compile *c,
			       const struct prog_instruction *inst )
{
   struct prog_src_register src0 = inst->SrcReg[0];

   if (projtex(c, inst)) {
      struct prog_dst_register tmp = get_temp(c);
      struct prog_instruction tmp_inst;

      /* tmp0.w = RCP inst.arg[0][3]
       */
      emit_op(c,
	      OPCODE_RCP,
	      dst_mask(tmp, WRITEMASK_W),
	      0,
	      src_swizzle1(src0, GET_SWZ(src0.Swizzle, W)),
	      src_undef(),
	      src_undef());

      /* tmp0.xyz =  MUL inst.arg[0], tmp0.wwww
       */
      emit_op(c,
	      OPCODE_MUL,
	      dst_mask(tmp, WRITEMASK_XYZ),
	      0,
	      src0,
	      src_swizzle1(src_reg_from_dst(tmp), W),
	      src_undef());

      /* dst = precalc(TEX tmp0)
       */
      tmp_inst = *inst;
      tmp_inst.SrcReg[0] = src_reg_from_dst(tmp);
      precalc_tex(c, &tmp_inst);

      release_temp(c, tmp);
   }
   else
   {
      /* dst = precalc(TEX src0)
       */
      precalc_tex(c, inst);
   }
}



static void emit_render_target_writes( struct brw_wm_compile *c )
{
   struct prog_src_register payload_r0_depth = src_reg(PROGRAM_PAYLOAD, PAYLOAD_DEPTH);
   struct prog_src_register outdepth = src_reg(PROGRAM_OUTPUT, FRAG_RESULT_DEPTH);
   struct prog_src_register outcolor;
   GLuint i;

   struct prog_instruction *inst = NULL;

   /* The inst->Aux field is used for FB write target and the EOT marker */

   for (i = 0; i < c->key.nr_color_regions; i++) {
      if (c->fp->program.Base.OutputsWritten & (1 << FRAG_RESULT_COLOR)) {
	 outcolor = src_reg(PROGRAM_OUTPUT, FRAG_RESULT_COLOR);
      } else {
	 outcolor = src_reg(PROGRAM_OUTPUT, FRAG_RESULT_DATA0 + i);
      }
      inst = emit_op(c, WM_FB_WRITE, dst_mask(dst_undef(), 0),
		     0, outcolor, payload_r0_depth, outdepth);
      inst->Aux = INST_AUX_TARGET(i);
   }

   /* Mark the last FB write as final, or emit a dummy write if we had
    * no render targets bound.
    */
   if (c->key.nr_color_regions != 0) {
      inst->Aux |= INST_AUX_EOT;
   } else {
      inst = emit_op(c, WM_FB_WRITE, dst_mask(dst_undef(), 0),
		     0, src_reg(PROGRAM_OUTPUT, FRAG_RESULT_COLOR),
		     payload_r0_depth, outdepth);
      inst->Aux = INST_AUX_TARGET(0) | INST_AUX_EOT;
   }
}




/***********************************************************************
 * Emit INTERP instructions ahead of first use of each attrib.
 */

static void validate_src_regs( struct brw_wm_compile *c,
			       const struct prog_instruction *inst )
{
   GLuint nr_args = brw_wm_nr_args( inst->Opcode );
   GLuint i;

   for (i = 0; i < nr_args; i++) {
      if (inst->SrcReg[i].File == PROGRAM_INPUT) {
	 GLuint idx = inst->SrcReg[i].Index;
	 if (!(c->fp_interp_emitted & (1<<idx))) {
	    emit_interp(c, idx);
	 }
      }
   }
}

static void print_insns( const struct prog_instruction *insn,
			 GLuint nr )
{
   GLuint i;
   for (i = 0; i < nr; i++, insn++) {
      printf("%3d: ", i);
      if (insn->Opcode < MAX_OPCODE)
	 _mesa_fprint_instruction_opt(stdout, insn, 0, PROG_PRINT_DEBUG, NULL);
      else if (insn->Opcode < MAX_WM_OPCODE) {
	 GLuint idx = insn->Opcode - MAX_OPCODE;

	 _mesa_fprint_alu_instruction(stdout, insn, wm_opcode_strings[idx],
				      3, PROG_PRINT_DEBUG, NULL);
      }
      else 
	 printf("965 Opcode %d\n", insn->Opcode);
   }
}


/**
 * Initial pass for fragment program code generation.
 * This function is used by both the GLSL and non-GLSL paths.
 */
void brw_wm_pass_fp( struct brw_wm_compile *c )
{
   struct intel_context *intel = &c->func.brw->intel;
   struct brw_fragment_program *fp = c->fp;
   GLuint insn;

   if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
      printf("pre-fp:\n");
      _mesa_fprint_program_opt(stdout, &fp->program.Base, PROG_PRINT_DEBUG,
			       true);
      printf("\n");
   }

   c->pixel_xy = src_undef();
   if (intel->gen >= 6) {
      /* The interpolation deltas come in as the perspective pixel
       * location barycentric params.
       */
      c->delta_xy = src_reg(PROGRAM_PAYLOAD, PAYLOAD_DEPTH);
   } else {
      c->delta_xy = src_undef();
   }
   c->pixel_w = src_undef();
   c->nr_fp_insns = 0;

   /* Emit preamble instructions.  This is where special instructions such as
    * WM_CINTERP, WM_LINTERP, WM_PINTERP and WM_WPOSXY are emitted to
    * compute shader inputs from varying vars.
    */
   for (insn = 0; insn < fp->program.Base.NumInstructions; insn++) {
      const struct prog_instruction *inst = &fp->program.Base.Instructions[insn];
      validate_src_regs(c, inst);
   }

   /* Loop over all instructions doing assorted simplifications and
    * transformations.
    */
   for (insn = 0; insn < fp->program.Base.NumInstructions; insn++) {
      const struct prog_instruction *inst = &fp->program.Base.Instructions[insn];
      struct prog_instruction *out;

      /* Check for INPUT values, emit INTERP instructions where
       * necessary:
       */

      switch (inst->Opcode) {
      case OPCODE_SWZ: 
	 out = emit_insn(c, inst);
	 out->Opcode = OPCODE_MOV;
	 break;
	 
      case OPCODE_ABS:
	 out = emit_insn(c, inst);
	 out->Opcode = OPCODE_MOV;
	 out->SrcReg[0].Negate = NEGATE_NONE;
	 out->SrcReg[0].Abs = 1;
	 break;

      case OPCODE_SUB: 
	 out = emit_insn(c, inst);
	 out->Opcode = OPCODE_ADD;
	 out->SrcReg[1].Negate ^= NEGATE_XYZW;
	 break;

      case OPCODE_SCS: 
	 out = emit_insn(c, inst);
	 /* This should probably be done in the parser. 
	  */
	 out->DstReg.WriteMask &= WRITEMASK_XY;
	 break;
	 
      case OPCODE_DST:
	 precalc_dst(c, inst);
	 break;

      case OPCODE_LIT:
	 precalc_lit(c, inst);
	 break;

      case OPCODE_RSQ:
	 out = emit_scalar_insn(c, inst);
	 out->SrcReg[0].Abs = true;
	 break;

      case OPCODE_TEX:
	 precalc_tex(c, inst);
	 break;

      case OPCODE_TXP:
	 precalc_txp(c, inst);
	 break;

      case OPCODE_TXB:
	 out = emit_insn(c, inst);
	 out->TexSrcUnit = fp->program.Base.SamplerUnits[inst->TexSrcUnit];
         assert(out->TexSrcUnit < BRW_MAX_TEX_UNIT);
	 break;

      case OPCODE_XPD: 
	 out = emit_insn(c, inst);
	 /* This should probably be done in the parser. 
	  */
	 out->DstReg.WriteMask &= WRITEMASK_XYZ;
	 break;

      case OPCODE_KIL: 
	 out = emit_insn(c, inst);
	 /* This should probably be done in the parser. 
	  */
	 out->DstReg.WriteMask = 0;
	 break;
      case OPCODE_END:
	 emit_render_target_writes(c);
	 break;
      case OPCODE_PRINT:
	 break;
      default:
	 if (brw_wm_is_scalar_result(inst->Opcode))
	    emit_scalar_insn(c, inst);
	 else
	    emit_insn(c, inst);
	 break;
      }
   }

   if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
      printf("pass_fp:\n");
      print_insns( c->prog_instructions, c->nr_fp_insns );
      printf("\n");
   }
}

