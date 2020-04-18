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

#include "intel_batchbuffer.h"

#include "brw_defines.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_util.h"
#include "brw_sf.h"


/**
 * Determine the vert_result corresponding to the given half of the given
 * register.  half=0 means the first half of a register, half=1 means the
 * second half.
 */
static inline int vert_reg_to_vert_result(struct brw_sf_compile *c, GLuint reg,
                                          int half)
{
   int vue_slot = (reg + c->urb_entry_read_offset) * 2 + half;
   return c->vue_map.slot_to_vert_result[vue_slot];
}

/**
 * Determine the register corresponding to the given vert_result.
 */
static struct brw_reg get_vert_result(struct brw_sf_compile *c,
                                      struct brw_reg vert,
                                      GLuint vert_result)
{
   int vue_slot = c->vue_map.vert_result_to_slot[vert_result];
   assert (vue_slot >= c->urb_entry_read_offset);
   GLuint off = vue_slot / 2 - c->urb_entry_read_offset;
   GLuint sub = vue_slot % 2;

   return brw_vec4_grf(vert.nr + off, sub * 4);
}

static bool
have_attr(struct brw_sf_compile *c, GLuint attr)
{
   return (c->key.attrs & BITFIELD64_BIT(attr)) ? 1 : 0;
}

/*********************************************************************** 
 * Twoside lighting
 */
static void copy_bfc( struct brw_sf_compile *c,
		      struct brw_reg vert )
{
   struct brw_compile *p = &c->func;
   GLuint i;

   for (i = 0; i < 2; i++) {
      if (have_attr(c, VERT_RESULT_COL0+i) &&
	  have_attr(c, VERT_RESULT_BFC0+i))
	 brw_MOV(p, 
		 get_vert_result(c, vert, VERT_RESULT_COL0+i),
		 get_vert_result(c, vert, VERT_RESULT_BFC0+i));
   }
}


static void do_twoside_color( struct brw_sf_compile *c )
{
   struct brw_compile *p = &c->func;
   GLuint backface_conditional = c->key.frontface_ccw ? BRW_CONDITIONAL_G : BRW_CONDITIONAL_L;

   /* Already done in clip program:
    */
   if (c->key.primitive == SF_UNFILLED_TRIS)
      return;

   /* XXX: What happens if BFC isn't present?  This could only happen
    * for user-supplied vertex programs, as t_vp_build.c always does
    * the right thing.
    */
   if (!(have_attr(c, VERT_RESULT_COL0) && have_attr(c, VERT_RESULT_BFC0)) &&
       !(have_attr(c, VERT_RESULT_COL1) && have_attr(c, VERT_RESULT_BFC1)))
      return;
   
   /* Need to use BRW_EXECUTE_4 and also do an 4-wide compare in order
    * to get all channels active inside the IF.  In the clipping code
    * we run with NoMask, so it's not an option and we can use
    * BRW_EXECUTE_1 for all comparisions.
    */
   brw_push_insn_state(p);
   brw_CMP(p, vec4(brw_null_reg()), backface_conditional, c->det, brw_imm_f(0));
   brw_IF(p, BRW_EXECUTE_4);
   {
      switch (c->nr_verts) {
      case 3: copy_bfc(c, c->vert[2]);
      case 2: copy_bfc(c, c->vert[1]);
      case 1: copy_bfc(c, c->vert[0]);
      }
   }
   brw_ENDIF(p);
   brw_pop_insn_state(p);
}



/***********************************************************************
 * Flat shading
 */

#define VERT_RESULT_COLOR_BITS (BITFIELD64_BIT(VERT_RESULT_COL0) | \
				BITFIELD64_BIT(VERT_RESULT_COL1))

static void copy_colors( struct brw_sf_compile *c,
		     struct brw_reg dst,
		     struct brw_reg src)
{
   struct brw_compile *p = &c->func;
   GLuint i;

   for (i = VERT_RESULT_COL0; i <= VERT_RESULT_COL1; i++) {
      if (have_attr(c,i))
	 brw_MOV(p, 
		 get_vert_result(c, dst, i),
		 get_vert_result(c, src, i));
   }
}



/* Need to use a computed jump to copy flatshaded attributes as the
 * vertices are ordered according to y-coordinate before reaching this
 * point, so the PV could be anywhere.
 */
static void do_flatshade_triangle( struct brw_sf_compile *c )
{
   struct brw_compile *p = &c->func;
   struct intel_context *intel = &p->brw->intel;
   struct brw_reg ip = brw_ip_reg();
   GLuint nr = _mesa_bitcount_64(c->key.attrs & VERT_RESULT_COLOR_BITS);
   GLuint jmpi = 1;

   if (!nr)
      return;

   /* Already done in clip program:
    */
   if (c->key.primitive == SF_UNFILLED_TRIS)
      return;

   if (intel->gen == 5)
       jmpi = 2;

   brw_push_insn_state(p);
   
   brw_MUL(p, c->pv, c->pv, brw_imm_d(jmpi*(nr*2+1)));
   brw_JMPI(p, ip, ip, c->pv);

   copy_colors(c, c->vert[1], c->vert[0]);
   copy_colors(c, c->vert[2], c->vert[0]);
   brw_JMPI(p, ip, ip, brw_imm_d(jmpi*(nr*4+1)));

   copy_colors(c, c->vert[0], c->vert[1]);
   copy_colors(c, c->vert[2], c->vert[1]);
   brw_JMPI(p, ip, ip, brw_imm_d(jmpi*nr*2));

   copy_colors(c, c->vert[0], c->vert[2]);
   copy_colors(c, c->vert[1], c->vert[2]);

   brw_pop_insn_state(p);
}
	

static void do_flatshade_line( struct brw_sf_compile *c )
{
   struct brw_compile *p = &c->func;
   struct intel_context *intel = &p->brw->intel;
   struct brw_reg ip = brw_ip_reg();
   GLuint nr = _mesa_bitcount_64(c->key.attrs & VERT_RESULT_COLOR_BITS);
   GLuint jmpi = 1;

   if (!nr)
      return;

   /* Already done in clip program: 
    */
   if (c->key.primitive == SF_UNFILLED_TRIS)
      return;

   if (intel->gen == 5)
       jmpi = 2;

   brw_push_insn_state(p);
   
   brw_MUL(p, c->pv, c->pv, brw_imm_d(jmpi*(nr+1)));
   brw_JMPI(p, ip, ip, c->pv);
   copy_colors(c, c->vert[1], c->vert[0]);

   brw_JMPI(p, ip, ip, brw_imm_ud(jmpi*nr));
   copy_colors(c, c->vert[0], c->vert[1]);

   brw_pop_insn_state(p);
}

	

/***********************************************************************
 * Triangle setup.
 */


static void alloc_regs( struct brw_sf_compile *c )
{
   GLuint reg, i;

   /* Values computed by fixed function unit:
    */
   c->pv  = retype(brw_vec1_grf(1, 1), BRW_REGISTER_TYPE_D);
   c->det = brw_vec1_grf(1, 2);
   c->dx0 = brw_vec1_grf(1, 3);
   c->dx2 = brw_vec1_grf(1, 4);
   c->dy0 = brw_vec1_grf(1, 5);
   c->dy2 = brw_vec1_grf(1, 6);

   /* z and 1/w passed in seperately:
    */
   c->z[0]     = brw_vec1_grf(2, 0);
   c->inv_w[0] = brw_vec1_grf(2, 1);
   c->z[1]     = brw_vec1_grf(2, 2);
   c->inv_w[1] = brw_vec1_grf(2, 3);
   c->z[2]     = brw_vec1_grf(2, 4);
   c->inv_w[2] = brw_vec1_grf(2, 5);
   
   /* The vertices:
    */
   reg = 3;
   for (i = 0; i < c->nr_verts; i++) {
      c->vert[i] = brw_vec8_grf(reg, 0);
      reg += c->nr_attr_regs;
   }

   /* Temporaries, allocated after last vertex reg.
    */
   c->inv_det = brw_vec1_grf(reg, 0);  reg++;
   c->a1_sub_a0 = brw_vec8_grf(reg, 0);  reg++;
   c->a2_sub_a0 = brw_vec8_grf(reg, 0);  reg++;
   c->tmp = brw_vec8_grf(reg, 0);  reg++;

   /* Note grf allocation:
    */
   c->prog_data.total_grf = reg;
   

   /* Outputs of this program - interpolation coefficients for
    * rasterization:
    */
   c->m1Cx = brw_vec8_reg(BRW_MESSAGE_REGISTER_FILE, 1, 0);
   c->m2Cy = brw_vec8_reg(BRW_MESSAGE_REGISTER_FILE, 2, 0);
   c->m3C0 = brw_vec8_reg(BRW_MESSAGE_REGISTER_FILE, 3, 0);
}


static void copy_z_inv_w( struct brw_sf_compile *c )
{
   struct brw_compile *p = &c->func;
   GLuint i;

   brw_push_insn_state(p);
	
   /* Copy both scalars with a single MOV:
    */
   for (i = 0; i < c->nr_verts; i++)
      brw_MOV(p, vec2(suboffset(c->vert[i], 2)), vec2(c->z[i]));
	 
   brw_pop_insn_state(p);
}


static void invert_det( struct brw_sf_compile *c)
{
   /* Looks like we invert all 8 elements just to get 1/det in
    * position 2 !?!
    */
   brw_math(&c->func, 
	    c->inv_det, 
	    BRW_MATH_FUNCTION_INV,
	    0, 
	    c->det,
	    BRW_MATH_DATA_SCALAR,
	    BRW_MATH_PRECISION_FULL);

}


static bool
calculate_masks(struct brw_sf_compile *c,
	        GLuint reg,
		GLushort *pc,
		GLushort *pc_persp,
		GLushort *pc_linear)
{
   bool is_last_attr = (reg == c->nr_setup_regs - 1);
   GLbitfield64 persp_mask;
   GLbitfield64 linear_mask;

   if (c->key.do_flat_shading)
      persp_mask = c->key.attrs & ~(BITFIELD64_BIT(VERT_RESULT_HPOS) |
                                    BITFIELD64_BIT(VERT_RESULT_COL0) |
                                    BITFIELD64_BIT(VERT_RESULT_COL1));
   else
      persp_mask = c->key.attrs & ~(BITFIELD64_BIT(VERT_RESULT_HPOS));

   if (c->key.do_flat_shading)
      linear_mask = c->key.attrs & ~(BITFIELD64_BIT(VERT_RESULT_COL0) |
                                     BITFIELD64_BIT(VERT_RESULT_COL1));
   else
      linear_mask = c->key.attrs;

   *pc_persp = 0;
   *pc_linear = 0;
   *pc = 0xf;
      
   if (persp_mask & BITFIELD64_BIT(vert_reg_to_vert_result(c, reg, 0)))
      *pc_persp = 0xf;

   if (linear_mask & BITFIELD64_BIT(vert_reg_to_vert_result(c, reg, 0)))
      *pc_linear = 0xf;

   /* Maybe only processs one attribute on the final round:
    */
   if (vert_reg_to_vert_result(c, reg, 1) != BRW_VERT_RESULT_MAX) {
      *pc |= 0xf0;

      if (persp_mask & BITFIELD64_BIT(vert_reg_to_vert_result(c, reg, 1)))
	 *pc_persp |= 0xf0;

      if (linear_mask & BITFIELD64_BIT(vert_reg_to_vert_result(c, reg, 1)))
	 *pc_linear |= 0xf0;
   }

   return is_last_attr;
}

/* Calculates the predicate control for which channels of a reg
 * (containing 2 attrs) to do point sprite coordinate replacement on.
 */
static uint16_t
calculate_point_sprite_mask(struct brw_sf_compile *c, GLuint reg)
{
   int vert_result1, vert_result2;
   uint16_t pc = 0;

   vert_result1 = vert_reg_to_vert_result(c, reg, 0);
   if (vert_result1 >= VERT_RESULT_TEX0 && vert_result1 <= VERT_RESULT_TEX7) {
      if (c->key.point_sprite_coord_replace & (1 << (vert_result1 - VERT_RESULT_TEX0)))
	 pc |= 0x0f;
   }
   if (vert_result1 == BRW_VERT_RESULT_PNTC)
      pc |= 0x0f;

   vert_result2 = vert_reg_to_vert_result(c, reg, 1);
   if (vert_result2 >= VERT_RESULT_TEX0 && vert_result2 <= VERT_RESULT_TEX7) {
      if (c->key.point_sprite_coord_replace & (1 << (vert_result2 -
                                                     VERT_RESULT_TEX0)))
         pc |= 0xf0;
   }
   if (vert_result2 == BRW_VERT_RESULT_PNTC)
      pc |= 0xf0;

   return pc;
}



void brw_emit_tri_setup(struct brw_sf_compile *c, bool allocate)
{
   struct brw_compile *p = &c->func;
   GLuint i;

   c->nr_verts = 3;

   if (allocate)
      alloc_regs(c);

   invert_det(c);
   copy_z_inv_w(c);

   if (c->key.do_twoside_color) 
      do_twoside_color(c);

   if (c->key.do_flat_shading)
      do_flatshade_triangle(c);
      
   
   for (i = 0; i < c->nr_setup_regs; i++)
   {
      /* Pair of incoming attributes:
       */
      struct brw_reg a0 = offset(c->vert[0], i);
      struct brw_reg a1 = offset(c->vert[1], i);
      struct brw_reg a2 = offset(c->vert[2], i);
      GLushort pc, pc_persp, pc_linear;
      bool last = calculate_masks(c, i, &pc, &pc_persp, &pc_linear);

      if (pc_persp)
      {
	 brw_set_predicate_control_flag_value(p, pc_persp);
	 brw_MUL(p, a0, a0, c->inv_w[0]);
	 brw_MUL(p, a1, a1, c->inv_w[1]);
	 brw_MUL(p, a2, a2, c->inv_w[2]);
      }
      
      
      /* Calculate coefficients for interpolated values:
       */      
      if (pc_linear)
      {
	 brw_set_predicate_control_flag_value(p, pc_linear);

	 brw_ADD(p, c->a1_sub_a0, a1, negate(a0));
	 brw_ADD(p, c->a2_sub_a0, a2, negate(a0));

	 /* calculate dA/dx
	  */
	 brw_MUL(p, brw_null_reg(), c->a1_sub_a0, c->dy2);
	 brw_MAC(p, c->tmp, c->a2_sub_a0, negate(c->dy0));
	 brw_MUL(p, c->m1Cx, c->tmp, c->inv_det);
		
	 /* calculate dA/dy
	  */
	 brw_MUL(p, brw_null_reg(), c->a2_sub_a0, c->dx0);
	 brw_MAC(p, c->tmp, c->a1_sub_a0, negate(c->dx2));
	 brw_MUL(p, c->m2Cy, c->tmp, c->inv_det);
      }

      {
	 brw_set_predicate_control_flag_value(p, pc); 
	 /* start point for interpolation
	  */
	 brw_MOV(p, c->m3C0, a0);
      
	 /* Copy m0..m3 to URB.  m0 is implicitly copied from r0 in
	  * the send instruction:
	  */	 
	 brw_urb_WRITE(p, 
		       brw_null_reg(),
		       0,
		       brw_vec8_grf(0, 0), /* r0, will be copied to m0 */
		       0, 	/* allocate */
		       1,	/* used */
		       4, 	/* msg len */
		       0,	/* response len */
		       last,	/* eot */
		       last, 	/* writes complete */
		       i*4,	/* offset */
		       BRW_URB_SWIZZLE_TRANSPOSE); /* XXX: Swizzle control "SF to windower" */
      }
   }
}



void brw_emit_line_setup(struct brw_sf_compile *c, bool allocate)
{
   struct brw_compile *p = &c->func;
   GLuint i;


   c->nr_verts = 2;

   if (allocate)
      alloc_regs(c);

   invert_det(c);
   copy_z_inv_w(c);

   if (c->key.do_flat_shading)
      do_flatshade_line(c);

   for (i = 0; i < c->nr_setup_regs; i++)
   {
      /* Pair of incoming attributes:
       */
      struct brw_reg a0 = offset(c->vert[0], i);
      struct brw_reg a1 = offset(c->vert[1], i);
      GLushort pc, pc_persp, pc_linear;
      bool last = calculate_masks(c, i, &pc, &pc_persp, &pc_linear);

      if (pc_persp)
      {
	 brw_set_predicate_control_flag_value(p, pc_persp);
	 brw_MUL(p, a0, a0, c->inv_w[0]);
	 brw_MUL(p, a1, a1, c->inv_w[1]);
      }

      /* Calculate coefficients for position, color:
       */
      if (pc_linear) {
	 brw_set_predicate_control_flag_value(p, pc_linear); 

	 brw_ADD(p, c->a1_sub_a0, a1, negate(a0));

 	 brw_MUL(p, c->tmp, c->a1_sub_a0, c->dx0); 
	 brw_MUL(p, c->m1Cx, c->tmp, c->inv_det);
		
	 brw_MUL(p, c->tmp, c->a1_sub_a0, c->dy0);
	 brw_MUL(p, c->m2Cy, c->tmp, c->inv_det);
      }

      {
	 brw_set_predicate_control_flag_value(p, pc); 

	 /* start point for interpolation
	  */
	 brw_MOV(p, c->m3C0, a0);

	 /* Copy m0..m3 to URB. 
	  */
	 brw_urb_WRITE(p, 
		       brw_null_reg(),
		       0,
		       brw_vec8_grf(0, 0),
		       0, 	/* allocate */
		       1, 	/* used */
		       4, 	/* msg len */
		       0,	/* response len */
		       last, 	/* eot */
		       last, 	/* writes complete */
		       i*4,	/* urb destination offset */
		       BRW_URB_SWIZZLE_TRANSPOSE); 
      }
   } 
}

void brw_emit_point_sprite_setup(struct brw_sf_compile *c, bool allocate)
{
   struct brw_compile *p = &c->func;
   GLuint i;

   c->nr_verts = 1;

   if (allocate)
      alloc_regs(c);

   copy_z_inv_w(c);
   for (i = 0; i < c->nr_setup_regs; i++)
   {
      struct brw_reg a0 = offset(c->vert[0], i);
      GLushort pc, pc_persp, pc_linear, pc_coord_replace;
      bool last = calculate_masks(c, i, &pc, &pc_persp, &pc_linear);

      pc_coord_replace = calculate_point_sprite_mask(c, i);
      pc_persp &= ~pc_coord_replace;

      if (pc_persp) {
	 brw_set_predicate_control_flag_value(p, pc_persp);
	 brw_MUL(p, a0, a0, c->inv_w[0]);
      }

      /* Point sprite coordinate replacement: A texcoord with this
       * enabled gets replaced with the value (x, y, 0, 1) where x and
       * y vary from 0 to 1 across the horizontal and vertical of the
       * point.
       */
      if (pc_coord_replace) {
	 brw_set_predicate_control_flag_value(p, pc_coord_replace);
	 /* Caculate 1.0/PointWidth */
	 brw_math(&c->func,
		  c->tmp,
		  BRW_MATH_FUNCTION_INV,
		  0,
		  c->dx0,
		  BRW_MATH_DATA_SCALAR,
		  BRW_MATH_PRECISION_FULL);

	 brw_set_access_mode(p, BRW_ALIGN_16);

	 /* dA/dx, dA/dy */
	 brw_MOV(p, c->m1Cx, brw_imm_f(0.0));
	 brw_MOV(p, c->m2Cy, brw_imm_f(0.0));
	 brw_MOV(p, brw_writemask(c->m1Cx, WRITEMASK_X), c->tmp);
	 if (c->key.sprite_origin_lower_left) {
	    brw_MOV(p, brw_writemask(c->m2Cy, WRITEMASK_Y), negate(c->tmp));
	 } else {
	    brw_MOV(p, brw_writemask(c->m2Cy, WRITEMASK_Y), c->tmp);
	 }

	 /* attribute constant offset */
	 brw_MOV(p, c->m3C0, brw_imm_f(0.0));
	 if (c->key.sprite_origin_lower_left) {
	    brw_MOV(p, brw_writemask(c->m3C0, WRITEMASK_YW), brw_imm_f(1.0));
	 } else {
	    brw_MOV(p, brw_writemask(c->m3C0, WRITEMASK_W), brw_imm_f(1.0));
	 }

	 brw_set_access_mode(p, BRW_ALIGN_1);
      }

      if (pc & ~pc_coord_replace) {
	 brw_set_predicate_control_flag_value(p, pc & ~pc_coord_replace);
	 brw_MOV(p, c->m1Cx, brw_imm_ud(0));
	 brw_MOV(p, c->m2Cy, brw_imm_ud(0));
	 brw_MOV(p, c->m3C0, a0); /* constant value */
      }


      brw_set_predicate_control_flag_value(p, pc);
      /* Copy m0..m3 to URB. */
      brw_urb_WRITE(p,
		    brw_null_reg(),
		    0,
		    brw_vec8_grf(0, 0),
		    0, 	/* allocate */
		    1,	/* used */
		    4, 	/* msg len */
		    0,	/* response len */
		    last, 	/* eot */
		    last, 	/* writes complete */
		    i*4,	/* urb destination offset */
		    BRW_URB_SWIZZLE_TRANSPOSE);
   }
}

/* Points setup - several simplifications as all attributes are
 * constant across the face of the point (point sprites excluded!)
 */
void brw_emit_point_setup(struct brw_sf_compile *c, bool allocate)
{
   struct brw_compile *p = &c->func;
   GLuint i;

   c->nr_verts = 1;
   
   if (allocate)
      alloc_regs(c);

   copy_z_inv_w(c);

   brw_MOV(p, c->m1Cx, brw_imm_ud(0)); /* zero - move out of loop */
   brw_MOV(p, c->m2Cy, brw_imm_ud(0)); /* zero - move out of loop */

   for (i = 0; i < c->nr_setup_regs; i++)
   {
      struct brw_reg a0 = offset(c->vert[0], i);
      GLushort pc, pc_persp, pc_linear;
      bool last = calculate_masks(c, i, &pc, &pc_persp, &pc_linear);
            
      if (pc_persp)
      {				
	 /* This seems odd as the values are all constant, but the
	  * fragment shader will be expecting it:
	  */
	 brw_set_predicate_control_flag_value(p, pc_persp);
	 brw_MUL(p, a0, a0, c->inv_w[0]);
      }


      /* The delta values are always zero, just send the starting
       * coordinate.  Again, this is to fit in with the interpolation
       * code in the fragment shader.
       */
      {
	 brw_set_predicate_control_flag_value(p, pc); 

	 brw_MOV(p, c->m3C0, a0); /* constant value */

	 /* Copy m0..m3 to URB. 
	  */
	 brw_urb_WRITE(p, 
		       brw_null_reg(),
		       0,
		       brw_vec8_grf(0, 0),
		       0, 	/* allocate */
		       1,	/* used */
		       4, 	/* msg len */
		       0,	/* response len */
		       last, 	/* eot */
		       last, 	/* writes complete */
		       i*4,	/* urb destination offset */
		       BRW_URB_SWIZZLE_TRANSPOSE);
      }
   }
}

void brw_emit_anyprim_setup( struct brw_sf_compile *c )
{
   struct brw_compile *p = &c->func;
   struct brw_reg ip = brw_ip_reg();
   struct brw_reg payload_prim = brw_uw1_reg(BRW_GENERAL_REGISTER_FILE, 1, 0);
   struct brw_reg payload_attr = get_element_ud(brw_vec1_reg(BRW_GENERAL_REGISTER_FILE, 1, 0), 0); 
   struct brw_reg primmask;
   int jmp;
   struct brw_reg v1_null_ud = vec1(retype(brw_null_reg(), BRW_REGISTER_TYPE_UD));
   
   GLuint saveflag;

   c->nr_verts = 3;
   alloc_regs(c);

   primmask = retype(get_element(c->tmp, 0), BRW_REGISTER_TYPE_UD);

   brw_MOV(p, primmask, brw_imm_ud(1));
   brw_SHL(p, primmask, primmask, payload_prim);

   brw_set_conditionalmod(p, BRW_CONDITIONAL_Z);
   brw_AND(p, v1_null_ud, primmask, brw_imm_ud((1<<_3DPRIM_TRILIST) |
					       (1<<_3DPRIM_TRISTRIP) |
					       (1<<_3DPRIM_TRIFAN) |
					       (1<<_3DPRIM_TRISTRIP_REVERSE) |
					       (1<<_3DPRIM_POLYGON) |
					       (1<<_3DPRIM_RECTLIST) |
					       (1<<_3DPRIM_TRIFAN_NOSTIPPLE)));
   jmp = brw_JMPI(p, ip, ip, brw_imm_d(0)) - p->store;
   {
      saveflag = p->flag_value;
      brw_push_insn_state(p); 
      brw_emit_tri_setup( c, false );
      brw_pop_insn_state(p);
      p->flag_value = saveflag;
      /* note - thread killed in subroutine, so must
       * restore the flag which is changed when building
       * the subroutine. fix #13240
       */
   }
   brw_land_fwd_jump(p, jmp);

   brw_set_conditionalmod(p, BRW_CONDITIONAL_Z);
   brw_AND(p, v1_null_ud, primmask, brw_imm_ud((1<<_3DPRIM_LINELIST) |
					       (1<<_3DPRIM_LINESTRIP) |
					       (1<<_3DPRIM_LINELOOP) |
					       (1<<_3DPRIM_LINESTRIP_CONT) |
					       (1<<_3DPRIM_LINESTRIP_BF) |
					       (1<<_3DPRIM_LINESTRIP_CONT_BF)));
   jmp = brw_JMPI(p, ip, ip, brw_imm_d(0)) - p->store;
   {
      saveflag = p->flag_value;
      brw_push_insn_state(p); 
      brw_emit_line_setup( c, false );
      brw_pop_insn_state(p);
      p->flag_value = saveflag;
      /* note - thread killed in subroutine */
   }
   brw_land_fwd_jump(p, jmp); 

   brw_set_conditionalmod(p, BRW_CONDITIONAL_Z);
   brw_AND(p, v1_null_ud, payload_attr, brw_imm_ud(1<<BRW_SPRITE_POINT_ENABLE));
   jmp = brw_JMPI(p, ip, ip, brw_imm_d(0)) - p->store;
   {
      saveflag = p->flag_value;
      brw_push_insn_state(p); 
      brw_emit_point_sprite_setup( c, false );
      brw_pop_insn_state(p);
      p->flag_value = saveflag;
   }
   brw_land_fwd_jump(p, jmp); 

   brw_emit_point_setup( c, false );
}




