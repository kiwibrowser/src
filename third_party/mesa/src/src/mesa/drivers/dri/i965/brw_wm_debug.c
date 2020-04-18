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


void brw_wm_print_value( struct brw_wm_compile *c,
		       struct brw_wm_value *value )
{
   assert(value);
   if (c->state >= PASS2_DONE) 
      brw_print_reg(value->hw_reg);
   else if( value == &c->undef_value )
      printf("undef");
   else if( value - c->vreg >= 0 &&
	    value - c->vreg < BRW_WM_MAX_VREG)
      printf("r%ld", (long) (value - c->vreg));
   else if (value - c->creg >= 0 &&
	    value - c->creg < BRW_WM_MAX_PARAM)
      printf("c%ld", (long) (value - c->creg));
   else if (value - c->payload.input_interp >= 0 &&
	    value - c->payload.input_interp < FRAG_ATTRIB_MAX)
      printf("i%ld", (long) (value - c->payload.input_interp));
   else if (value - c->payload.depth >= 0 &&
	    value - c->payload.depth < FRAG_ATTRIB_MAX)
      printf("d%ld", (long) (value - c->payload.depth));
   else 
      printf("?");
}

void brw_wm_print_ref( struct brw_wm_compile *c,
		       struct brw_wm_ref *ref )
{
   struct brw_reg hw_reg = ref->hw_reg;

   if (ref->unspill_reg)
      printf("UNSPILL(%x)/", ref->value->spill_slot);

   if (c->state >= PASS2_DONE)
      brw_print_reg(ref->hw_reg);
   else {
      printf("%s", hw_reg.negate ? "-" : "");
      printf("%s", hw_reg.abs ? "abs/" : "");
      brw_wm_print_value(c, ref->value);
      if ((hw_reg.nr&1) || hw_reg.subnr) {
	 printf("->%d.%d", (hw_reg.nr&1), hw_reg.subnr);
      }
   }
}

void brw_wm_print_insn( struct brw_wm_compile *c,
			struct brw_wm_instruction *inst )
{
   GLuint i, arg;
   GLuint nr_args = brw_wm_nr_args(inst->opcode);

   printf("[");
   for (i = 0; i < 4; i++) {
      if (inst->dst[i]) {
	 brw_wm_print_value(c, inst->dst[i]);
	 if (inst->dst[i]->spill_slot)
	    printf("/SPILL(%x)",inst->dst[i]->spill_slot);
      }
      else
	 printf("#");
      if (i < 3)      
	 printf(",");
   }
   printf("]");

   if (inst->writemask != WRITEMASK_XYZW)
      printf(".%s%s%s%s", 
		   GET_BIT(inst->writemask, 0) ? "x" : "",
		   GET_BIT(inst->writemask, 1) ? "y" : "",
		   GET_BIT(inst->writemask, 2) ? "z" : "",
		   GET_BIT(inst->writemask, 3) ? "w" : "");

   switch (inst->opcode) {
   case WM_PIXELXY:
      printf(" = PIXELXY");
      break;
   case WM_DELTAXY:
      printf(" = DELTAXY");
      break;
   case WM_PIXELW:
      printf(" = PIXELW");
      break;
   case WM_WPOSXY:
      printf(" = WPOSXY");
      break;
   case WM_PINTERP:
      printf(" = PINTERP");
      break;
   case WM_LINTERP:
      printf(" = LINTERP");
      break;
   case WM_CINTERP:
      printf(" = CINTERP");
      break;
   case WM_FB_WRITE:
      printf(" = FB_WRITE");
      break;
   case WM_FRONTFACING:
      printf(" = FRONTFACING");
      break;
   default:
      printf(" = %s", _mesa_opcode_string(inst->opcode));
      break;
   }

   if (inst->saturate)
      printf("_SAT");

   for (arg = 0; arg < nr_args; arg++) {

      printf(" [");

      for (i = 0; i < 4; i++) {
	 if (inst->src[arg][i]) {
	    brw_wm_print_ref(c, inst->src[arg][i]);
	 }
	 else
	    printf("%%");

	 if (i < 3) 
	    printf(",");
	 else
	    printf("]");
      }
   }
   printf("\n");
}

void brw_wm_print_program( struct brw_wm_compile *c,
			   const char *stage )
{
   GLuint insn;

   printf("%s:\n", stage);
   for (insn = 0; insn < c->nr_insns; insn++)
      brw_wm_print_insn(c, &c->instruction[insn]);
   printf("\n");
}

