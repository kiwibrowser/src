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
#include "brw_context.h"
#include "brw_vs.h"

/* Component is active if it may diverge from [0,0,0,1].  Undef values
 * are promoted to [0,0,0,1] for the purposes of this analysis.
 */
struct tracker {
   bool twoside;
   GLubyte active[PROGRAM_OUTPUT+1][MAX_PROGRAM_TEMPS];
   GLbitfield size_masks[4];  /**< one bit per fragment program input attrib */
};


static void set_active_component( struct tracker *t,
				  GLuint file,
				  GLuint index,
				  GLubyte active )
{
   switch (file) {
   case PROGRAM_TEMPORARY:
   case PROGRAM_INPUT:
   case PROGRAM_OUTPUT:
      assert(file < PROGRAM_OUTPUT + 1);
      assert(index < Elements(t->active[0]));
      t->active[file][index] |= active;
      break;
   default:
      break;
   }
}

static void set_active( struct tracker *t,
			struct prog_dst_register dst,
			GLuint active )
{
   set_active_component( t, dst.File, dst.Index, active & dst.WriteMask );
}


static GLubyte get_active_component( struct tracker *t,
				     GLuint file,
				     GLuint index,
				     GLuint component,
				     GLubyte swz )
{
   switch (swz) {
   case SWIZZLE_ZERO:
      return component < 3 ? 0 : (1<<component);
   case SWIZZLE_ONE:
      return component == 3 ? 0 : (1<<component);
   default:
      switch (file) {
      case PROGRAM_TEMPORARY:
      case PROGRAM_INPUT:
      case PROGRAM_OUTPUT:
	 return t->active[file][index] & (1<<component);
      default:
	 return 1 << component;
      }
   }
}


static GLubyte get_active( struct tracker *t,
			   struct prog_src_register src )
{
   GLuint i;
   GLubyte active = src.Negate; /* NOTE! */

   if (src.RelAddr)
      return 0xf;

   for (i = 0; i < 4; i++) 
      active |= get_active_component(t, src.File, src.Index, i,
				     GET_SWZ(src.Swizzle, i));

   return active;
}

/**
 * Return the size (1,2,3 or 4) of the output/result for VERT_RESULT_idx.
 */
static GLubyte get_output_size( struct tracker *t,
				GLuint idx )
{
   GLubyte active;
   assert(idx < VERT_RESULT_MAX);
   active = t->active[PROGRAM_OUTPUT][idx];
   if (active & (1<<3)) return 4;
   if (active & (1<<2)) return 3;
   if (active & (1<<1)) return 2;
   if (active & (1<<0)) return 1;
   return 0;
}

/* Note the potential copying that occurs in the setup program:
 */
static void calc_sizes( struct tracker *t )
{
   GLint vertRes;

   if (t->twoside) {
      t->active[PROGRAM_OUTPUT][VERT_RESULT_COL0] |= 
	 t->active[PROGRAM_OUTPUT][VERT_RESULT_BFC0];

      t->active[PROGRAM_OUTPUT][VERT_RESULT_COL1] |= 
	 t->active[PROGRAM_OUTPUT][VERT_RESULT_BFC1];
   }

   /* Examine vertex program output sizes to set the size_masks[] info
    * which describes the fragment program input sizes.
    */
   for (vertRes = 0; vertRes < VERT_RESULT_MAX; vertRes++) {

      /* map vertex program output index to fragment program input index */
      GLint fragAttrib = _mesa_vert_result_to_frag_attrib(vertRes);
      if (fragAttrib < 0)
         continue;

      switch (get_output_size(t, vertRes)) {
      case 4: t->size_masks[4-1] |= 1 << fragAttrib;
      case 3: t->size_masks[3-1] |= 1 << fragAttrib;
      case 2: t->size_masks[2-1] |= 1 << fragAttrib;
      case 1: t->size_masks[1-1] |= 1 << fragAttrib;
	 break;
      }
   }
}

static GLubyte szflag[4+1] = {
   0,
   0x1,
   0x3,
   0x7,
   0xf
};

/* Pull a size out of the packed array:
 */
static GLuint get_input_size(struct brw_context *brw,
			     GLuint attr)
{
   GLuint sizes_dword = brw->vb.info.sizes[attr/16];
   GLuint sizes_bits = (sizes_dword>>((attr%16)*2)) & 0x3;
   return sizes_bits + 1;
/*    return brw->vb.inputs[attr].glarray->Size; */
}

/* Calculate sizes of vertex program outputs.  Size is the largest
 * component index which might vary from [0,0,0,1]
 */
static void calc_wm_input_sizes( struct brw_context *brw )
{
   struct gl_context *ctx = &brw->intel.ctx;
   /* BRW_NEW_VERTEX_PROGRAM */
   const struct brw_vertex_program *vp =
      brw_vertex_program_const(brw->vertex_program);
   /* BRW_NEW_INPUT_DIMENSIONS */
   struct tracker t;
   GLuint insn;
   GLuint i;

   /* Mesa IR is not generated for GLSL vertex shaders.  If there's no Mesa
    * IR, the code below cannot determine which output components are
    * written.  So, skip it and assume everything is written.  This
    * circumvents some optimizations in the fragment shader, but it guarantees
    * that correct code is generated.
    */
   if (vp->program.Base.NumInstructions == 0) {
      brw->wm.input_size_masks[0] = ~0;
      brw->wm.input_size_masks[1] = ~0;
      brw->wm.input_size_masks[2] = ~0;
      brw->wm.input_size_masks[3] = ~0;
      return;
   }


   memset(&t, 0, sizeof(t));

   /* _NEW_LIGHT | _NEW_PROGRAM */
   if (ctx->VertexProgram._TwoSideEnabled)
      t.twoside = 1;

   for (i = 0; i < VERT_ATTRIB_MAX; i++) 
      if (vp->program.Base.InputsRead & BITFIELD64_BIT(i))
	 set_active_component(&t, PROGRAM_INPUT, i, 
			      szflag[get_input_size(brw, i)]);
      
   for (insn = 0; insn < vp->program.Base.NumInstructions; insn++) {
      struct prog_instruction *inst = &vp->program.Base.Instructions[insn];
      
      switch (inst->Opcode) {
      case OPCODE_ARL:
	 break;

      case OPCODE_MOV:
	 set_active(&t, inst->DstReg, get_active(&t, inst->SrcReg[0]));
	 break;

      default:
	 set_active(&t, inst->DstReg, 0xf);
	 break;
      }
   }

   calc_sizes(&t);

   if (memcmp(brw->wm.input_size_masks, t.size_masks, sizeof(t.size_masks)) != 0) {
      memcpy(brw->wm.input_size_masks, t.size_masks, sizeof(t.size_masks));
      brw->state.dirty.brw |= BRW_NEW_WM_INPUT_DIMENSIONS;
   }
}

const struct brw_tracked_state brw_wm_input_sizes = {
   .dirty = {
      .mesa  = _NEW_LIGHT | _NEW_PROGRAM,
      .brw   = BRW_NEW_VERTEX_PROGRAM | BRW_NEW_INPUT_DIMENSIONS,
      .cache = 0
   },
   .emit = calc_wm_input_sizes
};

