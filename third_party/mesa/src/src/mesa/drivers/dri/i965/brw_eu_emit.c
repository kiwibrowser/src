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
#include "brw_defines.h"
#include "brw_eu.h"

#include "glsl/ralloc.h"

/***********************************************************************
 * Internal helper for constructing instructions
 */

static void guess_execution_size(struct brw_compile *p,
				 struct brw_instruction *insn,
				 struct brw_reg reg)
{
   if (reg.width == BRW_WIDTH_8 && p->compressed)
      insn->header.execution_size = BRW_EXECUTE_16;
   else
      insn->header.execution_size = reg.width;	/* note - definitions are compatible */
}


/**
 * Prior to Sandybridge, the SEND instruction accepted non-MRF source
 * registers, implicitly moving the operand to a message register.
 *
 * On Sandybridge, this is no longer the case.  This function performs the
 * explicit move; it should be called before emitting a SEND instruction.
 */
void
gen6_resolve_implied_move(struct brw_compile *p,
			  struct brw_reg *src,
			  GLuint msg_reg_nr)
{
   struct intel_context *intel = &p->brw->intel;
   if (intel->gen < 6)
      return;

   if (src->file == BRW_MESSAGE_REGISTER_FILE)
      return;

   if (src->file != BRW_ARCHITECTURE_REGISTER_FILE || src->nr != BRW_ARF_NULL) {
      brw_push_insn_state(p);
      brw_set_mask_control(p, BRW_MASK_DISABLE);
      brw_set_compression_control(p, BRW_COMPRESSION_NONE);
      brw_MOV(p, retype(brw_message_reg(msg_reg_nr), BRW_REGISTER_TYPE_UD),
	      retype(*src, BRW_REGISTER_TYPE_UD));
      brw_pop_insn_state(p);
   }
   *src = brw_message_reg(msg_reg_nr);
}

static void
gen7_convert_mrf_to_grf(struct brw_compile *p, struct brw_reg *reg)
{
   /* From the BSpec / ISA Reference / send - [DevIVB+]:
    * "The send with EOT should use register space R112-R127 for <src>. This is
    *  to enable loading of a new thread into the same slot while the message
    *  with EOT for current thread is pending dispatch."
    *
    * Since we're pretending to have 16 MRFs anyway, we may as well use the
    * registers required for messages with EOT.
    */
   struct intel_context *intel = &p->brw->intel;
   if (intel->gen == 7 && reg->file == BRW_MESSAGE_REGISTER_FILE) {
      reg->file = BRW_GENERAL_REGISTER_FILE;
      reg->nr += GEN7_MRF_HACK_START;
   }
}


void
brw_set_dest(struct brw_compile *p, struct brw_instruction *insn,
	     struct brw_reg dest)
{
   if (dest.file != BRW_ARCHITECTURE_REGISTER_FILE &&
       dest.file != BRW_MESSAGE_REGISTER_FILE)
      assert(dest.nr < 128);

   gen7_convert_mrf_to_grf(p, &dest);

   insn->bits1.da1.dest_reg_file = dest.file;
   insn->bits1.da1.dest_reg_type = dest.type;
   insn->bits1.da1.dest_address_mode = dest.address_mode;

   if (dest.address_mode == BRW_ADDRESS_DIRECT) {   
      insn->bits1.da1.dest_reg_nr = dest.nr;

      if (insn->header.access_mode == BRW_ALIGN_1) {
	 insn->bits1.da1.dest_subreg_nr = dest.subnr;
	 if (dest.hstride == BRW_HORIZONTAL_STRIDE_0)
	    dest.hstride = BRW_HORIZONTAL_STRIDE_1;
	 insn->bits1.da1.dest_horiz_stride = dest.hstride;
      }
      else {
	 insn->bits1.da16.dest_subreg_nr = dest.subnr / 16;
	 insn->bits1.da16.dest_writemask = dest.dw1.bits.writemask;
	 /* even ignored in da16, still need to set as '01' */
	 insn->bits1.da16.dest_horiz_stride = 1;
      }
   }
   else {
      insn->bits1.ia1.dest_subreg_nr = dest.subnr;

      /* These are different sizes in align1 vs align16:
       */
      if (insn->header.access_mode == BRW_ALIGN_1) {
	 insn->bits1.ia1.dest_indirect_offset = dest.dw1.bits.indirect_offset;
	 if (dest.hstride == BRW_HORIZONTAL_STRIDE_0)
	    dest.hstride = BRW_HORIZONTAL_STRIDE_1;
	 insn->bits1.ia1.dest_horiz_stride = dest.hstride;
      }
      else {
	 insn->bits1.ia16.dest_indirect_offset = dest.dw1.bits.indirect_offset;
	 /* even ignored in da16, still need to set as '01' */
	 insn->bits1.ia16.dest_horiz_stride = 1;
      }
   }

   /* NEW: Set the execution size based on dest.width and
    * insn->compression_control:
    */
   guess_execution_size(p, insn, dest);
}

extern int reg_type_size[];

static void
validate_reg(struct brw_instruction *insn, struct brw_reg reg)
{
   int hstride_for_reg[] = {0, 1, 2, 4};
   int vstride_for_reg[] = {0, 1, 2, 4, 8, 16, 32, 64, 128, 256};
   int width_for_reg[] = {1, 2, 4, 8, 16};
   int execsize_for_reg[] = {1, 2, 4, 8, 16};
   int width, hstride, vstride, execsize;

   if (reg.file == BRW_IMMEDIATE_VALUE) {
      /* 3.3.6: Region Parameters.  Restriction: Immediate vectors
       * mean the destination has to be 128-bit aligned and the
       * destination horiz stride has to be a word.
       */
      if (reg.type == BRW_REGISTER_TYPE_V) {
	 assert(hstride_for_reg[insn->bits1.da1.dest_horiz_stride] *
		reg_type_size[insn->bits1.da1.dest_reg_type] == 2);
      }

      return;
   }

   if (reg.file == BRW_ARCHITECTURE_REGISTER_FILE &&
       reg.file == BRW_ARF_NULL)
      return;

   assert(reg.hstride >= 0 && reg.hstride < Elements(hstride_for_reg));
   hstride = hstride_for_reg[reg.hstride];

   if (reg.vstride == 0xf) {
      vstride = -1;
   } else {
      assert(reg.vstride >= 0 && reg.vstride < Elements(vstride_for_reg));
      vstride = vstride_for_reg[reg.vstride];
   }

   assert(reg.width >= 0 && reg.width < Elements(width_for_reg));
   width = width_for_reg[reg.width];

   assert(insn->header.execution_size >= 0 &&
	  insn->header.execution_size < Elements(execsize_for_reg));
   execsize = execsize_for_reg[insn->header.execution_size];

   /* Restrictions from 3.3.10: Register Region Restrictions. */
   /* 3. */
   assert(execsize >= width);

   /* 4. */
   if (execsize == width && hstride != 0) {
      assert(vstride == -1 || vstride == width * hstride);
   }

   /* 5. */
   if (execsize == width && hstride == 0) {
      /* no restriction on vstride. */
   }

   /* 6. */
   if (width == 1) {
      assert(hstride == 0);
   }

   /* 7. */
   if (execsize == 1 && width == 1) {
      assert(hstride == 0);
      assert(vstride == 0);
   }

   /* 8. */
   if (vstride == 0 && hstride == 0) {
      assert(width == 1);
   }

   /* 10. Check destination issues. */
}

void
brw_set_src0(struct brw_compile *p, struct brw_instruction *insn,
	     struct brw_reg reg)
{
   if (reg.type != BRW_ARCHITECTURE_REGISTER_FILE)
      assert(reg.nr < 128);

   gen7_convert_mrf_to_grf(p, &reg);

   validate_reg(insn, reg);

   insn->bits1.da1.src0_reg_file = reg.file;
   insn->bits1.da1.src0_reg_type = reg.type;
   insn->bits2.da1.src0_abs = reg.abs;
   insn->bits2.da1.src0_negate = reg.negate;
   insn->bits2.da1.src0_address_mode = reg.address_mode;

   if (reg.file == BRW_IMMEDIATE_VALUE) {
      insn->bits3.ud = reg.dw1.ud;
   
      /* Required to set some fields in src1 as well:
       */
      insn->bits1.da1.src1_reg_file = 0; /* arf */
      insn->bits1.da1.src1_reg_type = reg.type;
   }
   else 
   {
      if (reg.address_mode == BRW_ADDRESS_DIRECT) {
	 if (insn->header.access_mode == BRW_ALIGN_1) {
	    insn->bits2.da1.src0_subreg_nr = reg.subnr;
	    insn->bits2.da1.src0_reg_nr = reg.nr;
	 }
	 else {
	    insn->bits2.da16.src0_subreg_nr = reg.subnr / 16;
	    insn->bits2.da16.src0_reg_nr = reg.nr;
	 }
      }
      else {
	 insn->bits2.ia1.src0_subreg_nr = reg.subnr;

	 if (insn->header.access_mode == BRW_ALIGN_1) {
	    insn->bits2.ia1.src0_indirect_offset = reg.dw1.bits.indirect_offset; 
	 }
	 else {
	    insn->bits2.ia16.src0_subreg_nr = reg.dw1.bits.indirect_offset;
	 }
      }

      if (insn->header.access_mode == BRW_ALIGN_1) {
	 if (reg.width == BRW_WIDTH_1 && 
	     insn->header.execution_size == BRW_EXECUTE_1) {
	    insn->bits2.da1.src0_horiz_stride = BRW_HORIZONTAL_STRIDE_0;
	    insn->bits2.da1.src0_width = BRW_WIDTH_1;
	    insn->bits2.da1.src0_vert_stride = BRW_VERTICAL_STRIDE_0;
	 }
	 else {
	    insn->bits2.da1.src0_horiz_stride = reg.hstride;
	    insn->bits2.da1.src0_width = reg.width;
	    insn->bits2.da1.src0_vert_stride = reg.vstride;
	 }
      }
      else {
	 insn->bits2.da16.src0_swz_x = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_X);
	 insn->bits2.da16.src0_swz_y = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_Y);
	 insn->bits2.da16.src0_swz_z = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_Z);
	 insn->bits2.da16.src0_swz_w = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_W);

	 /* This is an oddity of the fact we're using the same
	  * descriptions for registers in align_16 as align_1:
	  */
	 if (reg.vstride == BRW_VERTICAL_STRIDE_8)
	    insn->bits2.da16.src0_vert_stride = BRW_VERTICAL_STRIDE_4;
	 else
	    insn->bits2.da16.src0_vert_stride = reg.vstride;
      }
   }
}


void brw_set_src1(struct brw_compile *p,
		  struct brw_instruction *insn,
		  struct brw_reg reg)
{
   assert(reg.file != BRW_MESSAGE_REGISTER_FILE);

   assert(reg.nr < 128);

   gen7_convert_mrf_to_grf(p, &reg);

   validate_reg(insn, reg);

   insn->bits1.da1.src1_reg_file = reg.file;
   insn->bits1.da1.src1_reg_type = reg.type;
   insn->bits3.da1.src1_abs = reg.abs;
   insn->bits3.da1.src1_negate = reg.negate;

   /* Only src1 can be immediate in two-argument instructions.
    */
   assert(insn->bits1.da1.src0_reg_file != BRW_IMMEDIATE_VALUE);

   if (reg.file == BRW_IMMEDIATE_VALUE) {
      insn->bits3.ud = reg.dw1.ud;
   }
   else {
      /* This is a hardware restriction, which may or may not be lifted
       * in the future:
       */
      assert (reg.address_mode == BRW_ADDRESS_DIRECT);
      /* assert (reg.file == BRW_GENERAL_REGISTER_FILE); */

      if (insn->header.access_mode == BRW_ALIGN_1) {
	 insn->bits3.da1.src1_subreg_nr = reg.subnr;
	 insn->bits3.da1.src1_reg_nr = reg.nr;
      }
      else {
	 insn->bits3.da16.src1_subreg_nr = reg.subnr / 16;
	 insn->bits3.da16.src1_reg_nr = reg.nr;
      }

      if (insn->header.access_mode == BRW_ALIGN_1) {
	 if (reg.width == BRW_WIDTH_1 && 
	     insn->header.execution_size == BRW_EXECUTE_1) {
	    insn->bits3.da1.src1_horiz_stride = BRW_HORIZONTAL_STRIDE_0;
	    insn->bits3.da1.src1_width = BRW_WIDTH_1;
	    insn->bits3.da1.src1_vert_stride = BRW_VERTICAL_STRIDE_0;
	 }
	 else {
	    insn->bits3.da1.src1_horiz_stride = reg.hstride;
	    insn->bits3.da1.src1_width = reg.width;
	    insn->bits3.da1.src1_vert_stride = reg.vstride;
	 }
      }
      else {
	 insn->bits3.da16.src1_swz_x = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_X);
	 insn->bits3.da16.src1_swz_y = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_Y);
	 insn->bits3.da16.src1_swz_z = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_Z);
	 insn->bits3.da16.src1_swz_w = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_W);

	 /* This is an oddity of the fact we're using the same
	  * descriptions for registers in align_16 as align_1:
	  */
	 if (reg.vstride == BRW_VERTICAL_STRIDE_8)
	    insn->bits3.da16.src1_vert_stride = BRW_VERTICAL_STRIDE_4;
	 else
	    insn->bits3.da16.src1_vert_stride = reg.vstride;
      }
   }
}

/**
 * Set the Message Descriptor and Extended Message Descriptor fields
 * for SEND messages.
 *
 * \note This zeroes out the Function Control bits, so it must be called
 *       \b before filling out any message-specific data.  Callers can
 *       choose not to fill in irrelevant bits; they will be zero.
 */
static void
brw_set_message_descriptor(struct brw_compile *p,
			   struct brw_instruction *inst,
			   enum brw_message_target sfid,
			   unsigned msg_length,
			   unsigned response_length,
			   bool header_present,
			   bool end_of_thread)
{
   struct intel_context *intel = &p->brw->intel;

   brw_set_src1(p, inst, brw_imm_d(0));

   if (intel->gen >= 5) {
      inst->bits3.generic_gen5.header_present = header_present;
      inst->bits3.generic_gen5.response_length = response_length;
      inst->bits3.generic_gen5.msg_length = msg_length;
      inst->bits3.generic_gen5.end_of_thread = end_of_thread;

      if (intel->gen >= 6) {
	 /* On Gen6+ Message target/SFID goes in bits 27:24 of the header */
	 inst->header.destreg__conditionalmod = sfid;
      } else {
	 /* Set Extended Message Descriptor (ex_desc) */
	 inst->bits2.send_gen5.sfid = sfid;
	 inst->bits2.send_gen5.end_of_thread = end_of_thread;
      }
   } else {
      inst->bits3.generic.response_length = response_length;
      inst->bits3.generic.msg_length = msg_length;
      inst->bits3.generic.msg_target = sfid;
      inst->bits3.generic.end_of_thread = end_of_thread;
   }
}

static void brw_set_math_message( struct brw_compile *p,
				  struct brw_instruction *insn,
				  GLuint function,
				  GLuint integer_type,
				  bool low_precision,
				  GLuint dataType )
{
   struct brw_context *brw = p->brw;
   struct intel_context *intel = &brw->intel;
   unsigned msg_length;
   unsigned response_length;

   /* Infer message length from the function */
   switch (function) {
   case BRW_MATH_FUNCTION_POW:
   case BRW_MATH_FUNCTION_INT_DIV_QUOTIENT:
   case BRW_MATH_FUNCTION_INT_DIV_REMAINDER:
   case BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER:
      msg_length = 2;
      break;
   default:
      msg_length = 1;
      break;
   }

   /* Infer response length from the function */
   switch (function) {
   case BRW_MATH_FUNCTION_SINCOS:
   case BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER:
      response_length = 2;
      break;
   default:
      response_length = 1;
      break;
   }


   brw_set_message_descriptor(p, insn, BRW_SFID_MATH,
			      msg_length, response_length, false, false);
   if (intel->gen == 5) {
      insn->bits3.math_gen5.function = function;
      insn->bits3.math_gen5.int_type = integer_type;
      insn->bits3.math_gen5.precision = low_precision;
      insn->bits3.math_gen5.saturate = insn->header.saturate;
      insn->bits3.math_gen5.data_type = dataType;
      insn->bits3.math_gen5.snapshot = 0;
   } else {
      insn->bits3.math.function = function;
      insn->bits3.math.int_type = integer_type;
      insn->bits3.math.precision = low_precision;
      insn->bits3.math.saturate = insn->header.saturate;
      insn->bits3.math.data_type = dataType;
   }
   insn->header.saturate = 0;
}


static void brw_set_ff_sync_message(struct brw_compile *p,
				    struct brw_instruction *insn,
				    bool allocate,
				    GLuint response_length,
				    bool end_of_thread)
{
   brw_set_message_descriptor(p, insn, BRW_SFID_URB,
			      1, response_length, true, end_of_thread);
   insn->bits3.urb_gen5.opcode = 1; /* FF_SYNC */
   insn->bits3.urb_gen5.offset = 0; /* Not used by FF_SYNC */
   insn->bits3.urb_gen5.swizzle_control = 0; /* Not used by FF_SYNC */
   insn->bits3.urb_gen5.allocate = allocate;
   insn->bits3.urb_gen5.used = 0; /* Not used by FF_SYNC */
   insn->bits3.urb_gen5.complete = 0; /* Not used by FF_SYNC */
}

static void brw_set_urb_message( struct brw_compile *p,
				 struct brw_instruction *insn,
				 bool allocate,
				 bool used,
				 GLuint msg_length,
				 GLuint response_length,
				 bool end_of_thread,
				 bool complete,
				 GLuint offset,
				 GLuint swizzle_control )
{
   struct brw_context *brw = p->brw;
   struct intel_context *intel = &brw->intel;

   brw_set_message_descriptor(p, insn, BRW_SFID_URB,
			      msg_length, response_length, true, end_of_thread);
   if (intel->gen == 7) {
      insn->bits3.urb_gen7.opcode = 0;	/* URB_WRITE_HWORD */
      insn->bits3.urb_gen7.offset = offset;
      assert(swizzle_control != BRW_URB_SWIZZLE_TRANSPOSE);
      insn->bits3.urb_gen7.swizzle_control = swizzle_control;
      /* per_slot_offset = 0 makes it ignore offsets in message header */
      insn->bits3.urb_gen7.per_slot_offset = 0;
      insn->bits3.urb_gen7.complete = complete;
   } else if (intel->gen >= 5) {
      insn->bits3.urb_gen5.opcode = 0;	/* URB_WRITE */
      insn->bits3.urb_gen5.offset = offset;
      insn->bits3.urb_gen5.swizzle_control = swizzle_control;
      insn->bits3.urb_gen5.allocate = allocate;
      insn->bits3.urb_gen5.used = used;	/* ? */
      insn->bits3.urb_gen5.complete = complete;
   } else {
      insn->bits3.urb.opcode = 0;	/* ? */
      insn->bits3.urb.offset = offset;
      insn->bits3.urb.swizzle_control = swizzle_control;
      insn->bits3.urb.allocate = allocate;
      insn->bits3.urb.used = used;	/* ? */
      insn->bits3.urb.complete = complete;
   }
}

void
brw_set_dp_write_message(struct brw_compile *p,
			 struct brw_instruction *insn,
			 GLuint binding_table_index,
			 GLuint msg_control,
			 GLuint msg_type,
			 GLuint msg_length,
			 bool header_present,
			 GLuint last_render_target,
			 GLuint response_length,
			 GLuint end_of_thread,
			 GLuint send_commit_msg)
{
   struct brw_context *brw = p->brw;
   struct intel_context *intel = &brw->intel;
   unsigned sfid;

   if (intel->gen >= 7) {
      /* Use the Render Cache for RT writes; otherwise use the Data Cache */
      if (msg_type == GEN6_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE)
	 sfid = GEN6_SFID_DATAPORT_RENDER_CACHE;
      else
	 sfid = GEN7_SFID_DATAPORT_DATA_CACHE;
   } else if (intel->gen == 6) {
      /* Use the render cache for all write messages. */
      sfid = GEN6_SFID_DATAPORT_RENDER_CACHE;
   } else {
      sfid = BRW_SFID_DATAPORT_WRITE;
   }

   brw_set_message_descriptor(p, insn, sfid, msg_length, response_length,
			      header_present, end_of_thread);

   if (intel->gen >= 7) {
      insn->bits3.gen7_dp.binding_table_index = binding_table_index;
      insn->bits3.gen7_dp.msg_control = msg_control;
      insn->bits3.gen7_dp.last_render_target = last_render_target;
      insn->bits3.gen7_dp.msg_type = msg_type;
   } else if (intel->gen == 6) {
      insn->bits3.gen6_dp.binding_table_index = binding_table_index;
      insn->bits3.gen6_dp.msg_control = msg_control;
      insn->bits3.gen6_dp.last_render_target = last_render_target;
      insn->bits3.gen6_dp.msg_type = msg_type;
      insn->bits3.gen6_dp.send_commit_msg = send_commit_msg;
   } else if (intel->gen == 5) {
      insn->bits3.dp_write_gen5.binding_table_index = binding_table_index;
      insn->bits3.dp_write_gen5.msg_control = msg_control;
      insn->bits3.dp_write_gen5.last_render_target = last_render_target;
      insn->bits3.dp_write_gen5.msg_type = msg_type;
      insn->bits3.dp_write_gen5.send_commit_msg = send_commit_msg;
   } else {
      insn->bits3.dp_write.binding_table_index = binding_table_index;
      insn->bits3.dp_write.msg_control = msg_control;
      insn->bits3.dp_write.last_render_target = last_render_target;
      insn->bits3.dp_write.msg_type = msg_type;
      insn->bits3.dp_write.send_commit_msg = send_commit_msg;
   }
}

void
brw_set_dp_read_message(struct brw_compile *p,
			struct brw_instruction *insn,
			GLuint binding_table_index,
			GLuint msg_control,
			GLuint msg_type,
			GLuint target_cache,
			GLuint msg_length,
			GLuint response_length)
{
   struct brw_context *brw = p->brw;
   struct intel_context *intel = &brw->intel;
   unsigned sfid;

   if (intel->gen >= 7) {
      sfid = GEN7_SFID_DATAPORT_DATA_CACHE;
   } else if (intel->gen == 6) {
      if (target_cache == BRW_DATAPORT_READ_TARGET_RENDER_CACHE)
	 sfid = GEN6_SFID_DATAPORT_RENDER_CACHE;
      else
	 sfid = GEN6_SFID_DATAPORT_SAMPLER_CACHE;
   } else {
      sfid = BRW_SFID_DATAPORT_READ;
   }

   brw_set_message_descriptor(p, insn, sfid, msg_length, response_length,
			      true, false);

   if (intel->gen >= 7) {
      insn->bits3.gen7_dp.binding_table_index = binding_table_index;
      insn->bits3.gen7_dp.msg_control = msg_control;
      insn->bits3.gen7_dp.last_render_target = 0;
      insn->bits3.gen7_dp.msg_type = msg_type;
   } else if (intel->gen == 6) {
      insn->bits3.gen6_dp.binding_table_index = binding_table_index;
      insn->bits3.gen6_dp.msg_control = msg_control;
      insn->bits3.gen6_dp.last_render_target = 0;
      insn->bits3.gen6_dp.msg_type = msg_type;
      insn->bits3.gen6_dp.send_commit_msg = 0;
   } else if (intel->gen == 5) {
      insn->bits3.dp_read_gen5.binding_table_index = binding_table_index;
      insn->bits3.dp_read_gen5.msg_control = msg_control;
      insn->bits3.dp_read_gen5.msg_type = msg_type;
      insn->bits3.dp_read_gen5.target_cache = target_cache;
   } else if (intel->is_g4x) {
      insn->bits3.dp_read_g4x.binding_table_index = binding_table_index; /*0:7*/
      insn->bits3.dp_read_g4x.msg_control = msg_control;  /*8:10*/
      insn->bits3.dp_read_g4x.msg_type = msg_type;  /*11:13*/
      insn->bits3.dp_read_g4x.target_cache = target_cache;  /*14:15*/
   } else {
      insn->bits3.dp_read.binding_table_index = binding_table_index; /*0:7*/
      insn->bits3.dp_read.msg_control = msg_control;  /*8:11*/
      insn->bits3.dp_read.msg_type = msg_type;  /*12:13*/
      insn->bits3.dp_read.target_cache = target_cache;  /*14:15*/
   }
}

void
brw_set_sampler_message(struct brw_compile *p,
                        struct brw_instruction *insn,
                        GLuint binding_table_index,
                        GLuint sampler,
                        GLuint msg_type,
                        GLuint response_length,
                        GLuint msg_length,
                        GLuint header_present,
                        GLuint simd_mode,
                        GLuint return_format)
{
   struct brw_context *brw = p->brw;
   struct intel_context *intel = &brw->intel;

   brw_set_message_descriptor(p, insn, BRW_SFID_SAMPLER, msg_length,
			      response_length, header_present, false);

   if (intel->gen >= 7) {
      insn->bits3.sampler_gen7.binding_table_index = binding_table_index;
      insn->bits3.sampler_gen7.sampler = sampler;
      insn->bits3.sampler_gen7.msg_type = msg_type;
      insn->bits3.sampler_gen7.simd_mode = simd_mode;
   } else if (intel->gen >= 5) {
      insn->bits3.sampler_gen5.binding_table_index = binding_table_index;
      insn->bits3.sampler_gen5.sampler = sampler;
      insn->bits3.sampler_gen5.msg_type = msg_type;
      insn->bits3.sampler_gen5.simd_mode = simd_mode;
   } else if (intel->is_g4x) {
      insn->bits3.sampler_g4x.binding_table_index = binding_table_index;
      insn->bits3.sampler_g4x.sampler = sampler;
      insn->bits3.sampler_g4x.msg_type = msg_type;
   } else {
      insn->bits3.sampler.binding_table_index = binding_table_index;
      insn->bits3.sampler.sampler = sampler;
      insn->bits3.sampler.msg_type = msg_type;
      insn->bits3.sampler.return_format = return_format;
   }
}


#define next_insn brw_next_insn
struct brw_instruction *
brw_next_insn(struct brw_compile *p, GLuint opcode)
{
   struct brw_instruction *insn;

   if (p->nr_insn + 1 > p->store_size) {
      if (0)
         printf("incresing the store size to %d\n", p->store_size << 1);
      p->store_size <<= 1;
      p->store = reralloc(p->mem_ctx, p->store,
                          struct brw_instruction, p->store_size);
      if (!p->store)
         assert(!"realloc eu store memeory failed");
   }

   insn = &p->store[p->nr_insn++];
   memcpy(insn, p->current, sizeof(*insn));

   /* Reset this one-shot flag: 
    */

   if (p->current->header.destreg__conditionalmod) {
      p->current->header.destreg__conditionalmod = 0;
      p->current->header.predicate_control = BRW_PREDICATE_NORMAL;
   }

   insn->header.opcode = opcode;
   return insn;
}

static struct brw_instruction *brw_alu1( struct brw_compile *p,
					 GLuint opcode,
					 struct brw_reg dest,
					 struct brw_reg src )
{
   struct brw_instruction *insn = next_insn(p, opcode);
   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src);
   return insn;
}

static struct brw_instruction *brw_alu2(struct brw_compile *p,
					GLuint opcode,
					struct brw_reg dest,
					struct brw_reg src0,
					struct brw_reg src1 )
{
   struct brw_instruction *insn = next_insn(p, opcode);   
   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_src1(p, insn, src1);
   return insn;
}

static int
get_3src_subreg_nr(struct brw_reg reg)
{
   if (reg.vstride == BRW_VERTICAL_STRIDE_0) {
      assert(brw_is_single_value_swizzle(reg.dw1.bits.swizzle));
      return reg.subnr / 4 + BRW_GET_SWZ(reg.dw1.bits.swizzle, 0);
   } else {
      return reg.subnr / 4;
   }
}

static struct brw_instruction *brw_alu3(struct brw_compile *p,
					GLuint opcode,
					struct brw_reg dest,
					struct brw_reg src0,
					struct brw_reg src1,
					struct brw_reg src2)
{
   struct brw_instruction *insn = next_insn(p, opcode);

   gen7_convert_mrf_to_grf(p, &dest);

   assert(insn->header.access_mode == BRW_ALIGN_16);

   assert(dest.file == BRW_GENERAL_REGISTER_FILE ||
	  dest.file == BRW_MESSAGE_REGISTER_FILE);
   assert(dest.nr < 128);
   assert(dest.address_mode == BRW_ADDRESS_DIRECT);
   assert(dest.type = BRW_REGISTER_TYPE_F);
   insn->bits1.da3src.dest_reg_file = (dest.file == BRW_MESSAGE_REGISTER_FILE);
   insn->bits1.da3src.dest_reg_nr = dest.nr;
   insn->bits1.da3src.dest_subreg_nr = dest.subnr / 16;
   insn->bits1.da3src.dest_writemask = dest.dw1.bits.writemask;
   guess_execution_size(p, insn, dest);

   assert(src0.file == BRW_GENERAL_REGISTER_FILE);
   assert(src0.address_mode == BRW_ADDRESS_DIRECT);
   assert(src0.nr < 128);
   assert(src0.type == BRW_REGISTER_TYPE_F);
   insn->bits2.da3src.src0_swizzle = src0.dw1.bits.swizzle;
   insn->bits2.da3src.src0_subreg_nr = get_3src_subreg_nr(src0);
   insn->bits2.da3src.src0_reg_nr = src0.nr;
   insn->bits1.da3src.src0_abs = src0.abs;
   insn->bits1.da3src.src0_negate = src0.negate;
   insn->bits2.da3src.src0_rep_ctrl = src0.vstride == BRW_VERTICAL_STRIDE_0;

   assert(src1.file == BRW_GENERAL_REGISTER_FILE);
   assert(src1.address_mode == BRW_ADDRESS_DIRECT);
   assert(src1.nr < 128);
   assert(src1.type == BRW_REGISTER_TYPE_F);
   insn->bits2.da3src.src1_swizzle = src1.dw1.bits.swizzle;
   insn->bits2.da3src.src1_subreg_nr_low = get_3src_subreg_nr(src1) & 0x3;
   insn->bits3.da3src.src1_subreg_nr_high = get_3src_subreg_nr(src1) >> 2;
   insn->bits2.da3src.src1_rep_ctrl = src1.vstride == BRW_VERTICAL_STRIDE_0;
   insn->bits3.da3src.src1_reg_nr = src1.nr;
   insn->bits1.da3src.src1_abs = src1.abs;
   insn->bits1.da3src.src1_negate = src1.negate;

   assert(src2.file == BRW_GENERAL_REGISTER_FILE);
   assert(src2.address_mode == BRW_ADDRESS_DIRECT);
   assert(src2.nr < 128);
   assert(src2.type == BRW_REGISTER_TYPE_F);
   insn->bits3.da3src.src2_swizzle = src2.dw1.bits.swizzle;
   insn->bits3.da3src.src2_subreg_nr = get_3src_subreg_nr(src2);
   insn->bits3.da3src.src2_rep_ctrl = src2.vstride == BRW_VERTICAL_STRIDE_0;
   insn->bits3.da3src.src2_reg_nr = src2.nr;
   insn->bits1.da3src.src2_abs = src2.abs;
   insn->bits1.da3src.src2_negate = src2.negate;

   return insn;
}


/***********************************************************************
 * Convenience routines.
 */
#define ALU1(OP)					\
struct brw_instruction *brw_##OP(struct brw_compile *p,	\
	      struct brw_reg dest,			\
	      struct brw_reg src0)   			\
{							\
   return brw_alu1(p, BRW_OPCODE_##OP, dest, src0);    	\
}

#define ALU2(OP)					\
struct brw_instruction *brw_##OP(struct brw_compile *p,	\
	      struct brw_reg dest,			\
	      struct brw_reg src0,			\
	      struct brw_reg src1)   			\
{							\
   return brw_alu2(p, BRW_OPCODE_##OP, dest, src0, src1);	\
}

#define ALU3(OP)					\
struct brw_instruction *brw_##OP(struct brw_compile *p,	\
	      struct brw_reg dest,			\
	      struct brw_reg src0,			\
	      struct brw_reg src1,			\
	      struct brw_reg src2)   			\
{							\
   return brw_alu3(p, BRW_OPCODE_##OP, dest, src0, src1, src2);	\
}

/* Rounding operations (other than RNDD) require two instructions - the first
 * stores a rounded value (possibly the wrong way) in the dest register, but
 * also sets a per-channel "increment bit" in the flag register.  A predicated
 * add of 1.0 fixes dest to contain the desired result.
 *
 * Sandybridge and later appear to round correctly without an ADD.
 */
#define ROUND(OP)							      \
void brw_##OP(struct brw_compile *p,					      \
	      struct brw_reg dest,					      \
	      struct brw_reg src)					      \
{									      \
   struct brw_instruction *rnd, *add;					      \
   rnd = next_insn(p, BRW_OPCODE_##OP);					      \
   brw_set_dest(p, rnd, dest);						      \
   brw_set_src0(p, rnd, src);						      \
									      \
   if (p->brw->intel.gen < 6) {						      \
      /* turn on round-increments */					      \
      rnd->header.destreg__conditionalmod = BRW_CONDITIONAL_R;		      \
      add = brw_ADD(p, dest, dest, brw_imm_f(1.0f));			      \
      add->header.predicate_control = BRW_PREDICATE_NORMAL;		      \
   }									      \
}


ALU1(MOV)
ALU2(SEL)
ALU1(NOT)
ALU2(AND)
ALU2(OR)
ALU2(XOR)
ALU2(SHR)
ALU2(SHL)
ALU2(RSR)
ALU2(RSL)
ALU2(ASR)
ALU1(FRC)
ALU1(RNDD)
ALU2(MAC)
ALU2(MACH)
ALU1(LZD)
ALU2(DP4)
ALU2(DPH)
ALU2(DP3)
ALU2(DP2)
ALU2(LINE)
ALU2(PLN)
ALU3(MAD)

ROUND(RNDZ)
ROUND(RNDE)


struct brw_instruction *brw_ADD(struct brw_compile *p,
				struct brw_reg dest,
				struct brw_reg src0,
				struct brw_reg src1)
{
   /* 6.2.2: add */
   if (src0.type == BRW_REGISTER_TYPE_F ||
       (src0.file == BRW_IMMEDIATE_VALUE &&
	src0.type == BRW_REGISTER_TYPE_VF)) {
      assert(src1.type != BRW_REGISTER_TYPE_UD);
      assert(src1.type != BRW_REGISTER_TYPE_D);
   }

   if (src1.type == BRW_REGISTER_TYPE_F ||
       (src1.file == BRW_IMMEDIATE_VALUE &&
	src1.type == BRW_REGISTER_TYPE_VF)) {
      assert(src0.type != BRW_REGISTER_TYPE_UD);
      assert(src0.type != BRW_REGISTER_TYPE_D);
   }

   return brw_alu2(p, BRW_OPCODE_ADD, dest, src0, src1);
}

struct brw_instruction *brw_AVG(struct brw_compile *p,
                                struct brw_reg dest,
                                struct brw_reg src0,
                                struct brw_reg src1)
{
   assert(dest.type == src0.type);
   assert(src0.type == src1.type);
   switch (src0.type) {
   case BRW_REGISTER_TYPE_B:
   case BRW_REGISTER_TYPE_UB:
   case BRW_REGISTER_TYPE_W:
   case BRW_REGISTER_TYPE_UW:
   case BRW_REGISTER_TYPE_D:
   case BRW_REGISTER_TYPE_UD:
      break;
   default:
      assert(!"Bad type for brw_AVG");
   }

   return brw_alu2(p, BRW_OPCODE_AVG, dest, src0, src1);
}

struct brw_instruction *brw_MUL(struct brw_compile *p,
				struct brw_reg dest,
				struct brw_reg src0,
				struct brw_reg src1)
{
   /* 6.32.38: mul */
   if (src0.type == BRW_REGISTER_TYPE_D ||
       src0.type == BRW_REGISTER_TYPE_UD ||
       src1.type == BRW_REGISTER_TYPE_D ||
       src1.type == BRW_REGISTER_TYPE_UD) {
      assert(dest.type != BRW_REGISTER_TYPE_F);
   }

   if (src0.type == BRW_REGISTER_TYPE_F ||
       (src0.file == BRW_IMMEDIATE_VALUE &&
	src0.type == BRW_REGISTER_TYPE_VF)) {
      assert(src1.type != BRW_REGISTER_TYPE_UD);
      assert(src1.type != BRW_REGISTER_TYPE_D);
   }

   if (src1.type == BRW_REGISTER_TYPE_F ||
       (src1.file == BRW_IMMEDIATE_VALUE &&
	src1.type == BRW_REGISTER_TYPE_VF)) {
      assert(src0.type != BRW_REGISTER_TYPE_UD);
      assert(src0.type != BRW_REGISTER_TYPE_D);
   }

   assert(src0.file != BRW_ARCHITECTURE_REGISTER_FILE ||
	  src0.nr != BRW_ARF_ACCUMULATOR);
   assert(src1.file != BRW_ARCHITECTURE_REGISTER_FILE ||
	  src1.nr != BRW_ARF_ACCUMULATOR);

   return brw_alu2(p, BRW_OPCODE_MUL, dest, src0, src1);
}


void brw_NOP(struct brw_compile *p)
{
   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_NOP);   
   brw_set_dest(p, insn, retype(brw_vec4_grf(0,0), BRW_REGISTER_TYPE_UD));
   brw_set_src0(p, insn, retype(brw_vec4_grf(0,0), BRW_REGISTER_TYPE_UD));
   brw_set_src1(p, insn, brw_imm_ud(0x0));
}





/***********************************************************************
 * Comparisons, if/else/endif
 */

struct brw_instruction *brw_JMPI(struct brw_compile *p, 
                                 struct brw_reg dest,
                                 struct brw_reg src0,
                                 struct brw_reg src1)
{
   struct brw_instruction *insn = brw_alu2(p, BRW_OPCODE_JMPI, dest, src0, src1);

   insn->header.execution_size = 1;
   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.mask_control = BRW_MASK_DISABLE;

   p->current->header.predicate_control = BRW_PREDICATE_NONE;

   return insn;
}

static void
push_if_stack(struct brw_compile *p, struct brw_instruction *inst)
{
   p->if_stack[p->if_stack_depth] = inst - p->store;

   p->if_stack_depth++;
   if (p->if_stack_array_size <= p->if_stack_depth) {
      p->if_stack_array_size *= 2;
      p->if_stack = reralloc(p->mem_ctx, p->if_stack, int,
			     p->if_stack_array_size);
   }
}

static struct brw_instruction *
pop_if_stack(struct brw_compile *p)
{
   p->if_stack_depth--;
   return &p->store[p->if_stack[p->if_stack_depth]];
}

static void
push_loop_stack(struct brw_compile *p, struct brw_instruction *inst)
{
   if (p->loop_stack_array_size < p->loop_stack_depth) {
      p->loop_stack_array_size *= 2;
      p->loop_stack = reralloc(p->mem_ctx, p->loop_stack, int,
			       p->loop_stack_array_size);
      p->if_depth_in_loop = reralloc(p->mem_ctx, p->if_depth_in_loop, int,
				     p->loop_stack_array_size);
   }

   p->loop_stack[p->loop_stack_depth] = inst - p->store;
   p->loop_stack_depth++;
   p->if_depth_in_loop[p->loop_stack_depth] = 0;
}

static struct brw_instruction *
get_inner_do_insn(struct brw_compile *p)
{
   return &p->store[p->loop_stack[p->loop_stack_depth - 1]];
}

/* EU takes the value from the flag register and pushes it onto some
 * sort of a stack (presumably merging with any flag value already on
 * the stack).  Within an if block, the flags at the top of the stack
 * control execution on each channel of the unit, eg. on each of the
 * 16 pixel values in our wm programs.
 *
 * When the matching 'else' instruction is reached (presumably by
 * countdown of the instruction count patched in by our ELSE/ENDIF
 * functions), the relevent flags are inverted.
 *
 * When the matching 'endif' instruction is reached, the flags are
 * popped off.  If the stack is now empty, normal execution resumes.
 */
struct brw_instruction *
brw_IF(struct brw_compile *p, GLuint execute_size)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;

   insn = next_insn(p, BRW_OPCODE_IF);

   /* Override the defaults for this instruction:
    */
   if (intel->gen < 6) {
      brw_set_dest(p, insn, brw_ip_reg());
      brw_set_src0(p, insn, brw_ip_reg());
      brw_set_src1(p, insn, brw_imm_d(0x0));
   } else if (intel->gen == 6) {
      brw_set_dest(p, insn, brw_imm_w(0));
      insn->bits1.branch_gen6.jump_count = 0;
      brw_set_src0(p, insn, vec1(retype(brw_null_reg(), BRW_REGISTER_TYPE_D)));
      brw_set_src1(p, insn, vec1(retype(brw_null_reg(), BRW_REGISTER_TYPE_D)));
   } else {
      brw_set_dest(p, insn, vec1(retype(brw_null_reg(), BRW_REGISTER_TYPE_D)));
      brw_set_src0(p, insn, vec1(retype(brw_null_reg(), BRW_REGISTER_TYPE_D)));
      brw_set_src1(p, insn, brw_imm_ud(0));
      insn->bits3.break_cont.jip = 0;
      insn->bits3.break_cont.uip = 0;
   }

   insn->header.execution_size = execute_size;
   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.predicate_control = BRW_PREDICATE_NORMAL;
   insn->header.mask_control = BRW_MASK_ENABLE;
   if (!p->single_program_flow)
      insn->header.thread_control = BRW_THREAD_SWITCH;

   p->current->header.predicate_control = BRW_PREDICATE_NONE;

   push_if_stack(p, insn);
   p->if_depth_in_loop[p->loop_stack_depth]++;
   return insn;
}

/* This function is only used for gen6-style IF instructions with an
 * embedded comparison (conditional modifier).  It is not used on gen7.
 */
struct brw_instruction *
gen6_IF(struct brw_compile *p, uint32_t conditional,
	struct brw_reg src0, struct brw_reg src1)
{
   struct brw_instruction *insn;

   insn = next_insn(p, BRW_OPCODE_IF);

   brw_set_dest(p, insn, brw_imm_w(0));
   if (p->compressed) {
      insn->header.execution_size = BRW_EXECUTE_16;
   } else {
      insn->header.execution_size = BRW_EXECUTE_8;
   }
   insn->bits1.branch_gen6.jump_count = 0;
   brw_set_src0(p, insn, src0);
   brw_set_src1(p, insn, src1);

   assert(insn->header.compression_control == BRW_COMPRESSION_NONE);
   assert(insn->header.predicate_control == BRW_PREDICATE_NONE);
   insn->header.destreg__conditionalmod = conditional;

   if (!p->single_program_flow)
      insn->header.thread_control = BRW_THREAD_SWITCH;

   push_if_stack(p, insn);
   return insn;
}

/**
 * In single-program-flow (SPF) mode, convert IF and ELSE into ADDs.
 */
static void
convert_IF_ELSE_to_ADD(struct brw_compile *p,
		       struct brw_instruction *if_inst,
		       struct brw_instruction *else_inst)
{
   /* The next instruction (where the ENDIF would be, if it existed) */
   struct brw_instruction *next_inst = &p->store[p->nr_insn];

   assert(p->single_program_flow);
   assert(if_inst != NULL && if_inst->header.opcode == BRW_OPCODE_IF);
   assert(else_inst == NULL || else_inst->header.opcode == BRW_OPCODE_ELSE);
   assert(if_inst->header.execution_size == BRW_EXECUTE_1);

   /* Convert IF to an ADD instruction that moves the instruction pointer
    * to the first instruction of the ELSE block.  If there is no ELSE
    * block, point to where ENDIF would be.  Reverse the predicate.
    *
    * There's no need to execute an ENDIF since we don't need to do any
    * stack operations, and if we're currently executing, we just want to
    * continue normally.
    */
   if_inst->header.opcode = BRW_OPCODE_ADD;
   if_inst->header.predicate_inverse = 1;

   if (else_inst != NULL) {
      /* Convert ELSE to an ADD instruction that points where the ENDIF
       * would be.
       */
      else_inst->header.opcode = BRW_OPCODE_ADD;

      if_inst->bits3.ud = (else_inst - if_inst + 1) * 16;
      else_inst->bits3.ud = (next_inst - else_inst) * 16;
   } else {
      if_inst->bits3.ud = (next_inst - if_inst) * 16;
   }
}

/**
 * Patch IF and ELSE instructions with appropriate jump targets.
 */
static void
patch_IF_ELSE(struct brw_compile *p,
	      struct brw_instruction *if_inst,
	      struct brw_instruction *else_inst,
	      struct brw_instruction *endif_inst)
{
   struct intel_context *intel = &p->brw->intel;

   /* We shouldn't be patching IF and ELSE instructions in single program flow
    * mode when gen < 6, because in single program flow mode on those
    * platforms, we convert flow control instructions to conditional ADDs that
    * operate on IP (see brw_ENDIF).
    *
    * However, on Gen6, writing to IP doesn't work in single program flow mode
    * (see the SandyBridge PRM, Volume 4 part 2, p79: "When SPF is ON, IP may
    * not be updated by non-flow control instructions.").  And on later
    * platforms, there is no significant benefit to converting control flow
    * instructions to conditional ADDs.  So we do patch IF and ELSE
    * instructions in single program flow mode on those platforms.
    */
   if (intel->gen < 6)
      assert(!p->single_program_flow);

   assert(if_inst != NULL && if_inst->header.opcode == BRW_OPCODE_IF);
   assert(endif_inst != NULL);
   assert(else_inst == NULL || else_inst->header.opcode == BRW_OPCODE_ELSE);

   unsigned br = 1;
   /* Jump count is for 64bit data chunk each, so one 128bit instruction
    * requires 2 chunks.
    */
   if (intel->gen >= 5)
      br = 2;

   assert(endif_inst->header.opcode == BRW_OPCODE_ENDIF);
   endif_inst->header.execution_size = if_inst->header.execution_size;

   if (else_inst == NULL) {
      /* Patch IF -> ENDIF */
      if (intel->gen < 6) {
	 /* Turn it into an IFF, which means no mask stack operations for
	  * all-false and jumping past the ENDIF.
	  */
	 if_inst->header.opcode = BRW_OPCODE_IFF;
	 if_inst->bits3.if_else.jump_count = br * (endif_inst - if_inst + 1);
	 if_inst->bits3.if_else.pop_count = 0;
	 if_inst->bits3.if_else.pad0 = 0;
      } else if (intel->gen == 6) {
	 /* As of gen6, there is no IFF and IF must point to the ENDIF. */
	 if_inst->bits1.branch_gen6.jump_count = br * (endif_inst - if_inst);
      } else {
	 if_inst->bits3.break_cont.uip = br * (endif_inst - if_inst);
	 if_inst->bits3.break_cont.jip = br * (endif_inst - if_inst);
      }
   } else {
      else_inst->header.execution_size = if_inst->header.execution_size;

      /* Patch IF -> ELSE */
      if (intel->gen < 6) {
	 if_inst->bits3.if_else.jump_count = br * (else_inst - if_inst);
	 if_inst->bits3.if_else.pop_count = 0;
	 if_inst->bits3.if_else.pad0 = 0;
      } else if (intel->gen == 6) {
	 if_inst->bits1.branch_gen6.jump_count = br * (else_inst - if_inst + 1);
      }

      /* Patch ELSE -> ENDIF */
      if (intel->gen < 6) {
	 /* BRW_OPCODE_ELSE pre-gen6 should point just past the
	  * matching ENDIF.
	  */
	 else_inst->bits3.if_else.jump_count = br*(endif_inst - else_inst + 1);
	 else_inst->bits3.if_else.pop_count = 1;
	 else_inst->bits3.if_else.pad0 = 0;
      } else if (intel->gen == 6) {
	 /* BRW_OPCODE_ELSE on gen6 should point to the matching ENDIF. */
	 else_inst->bits1.branch_gen6.jump_count = br*(endif_inst - else_inst);
      } else {
	 /* The IF instruction's JIP should point just past the ELSE */
	 if_inst->bits3.break_cont.jip = br * (else_inst - if_inst + 1);
	 /* The IF instruction's UIP and ELSE's JIP should point to ENDIF */
	 if_inst->bits3.break_cont.uip = br * (endif_inst - if_inst);
	 else_inst->bits3.break_cont.jip = br * (endif_inst - else_inst);
      }
   }
}

void
brw_ELSE(struct brw_compile *p)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;

   insn = next_insn(p, BRW_OPCODE_ELSE);

   if (intel->gen < 6) {
      brw_set_dest(p, insn, brw_ip_reg());
      brw_set_src0(p, insn, brw_ip_reg());
      brw_set_src1(p, insn, brw_imm_d(0x0));
   } else if (intel->gen == 6) {
      brw_set_dest(p, insn, brw_imm_w(0));
      insn->bits1.branch_gen6.jump_count = 0;
      brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   } else {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(p, insn, brw_imm_ud(0));
      insn->bits3.break_cont.jip = 0;
      insn->bits3.break_cont.uip = 0;
   }

   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.mask_control = BRW_MASK_ENABLE;
   if (!p->single_program_flow)
      insn->header.thread_control = BRW_THREAD_SWITCH;

   push_if_stack(p, insn);
}

void
brw_ENDIF(struct brw_compile *p)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn = NULL;
   struct brw_instruction *else_inst = NULL;
   struct brw_instruction *if_inst = NULL;
   struct brw_instruction *tmp;
   bool emit_endif = true;

   /* In single program flow mode, we can express IF and ELSE instructions
    * equivalently as ADD instructions that operate on IP.  On platforms prior
    * to Gen6, flow control instructions cause an implied thread switch, so
    * this is a significant savings.
    *
    * However, on Gen6, writing to IP doesn't work in single program flow mode
    * (see the SandyBridge PRM, Volume 4 part 2, p79: "When SPF is ON, IP may
    * not be updated by non-flow control instructions.").  And on later
    * platforms, there is no significant benefit to converting control flow
    * instructions to conditional ADDs.  So we only do this trick on Gen4 and
    * Gen5.
    */
   if (intel->gen < 6 && p->single_program_flow)
      emit_endif = false;

   /*
    * A single next_insn() may change the base adress of instruction store
    * memory(p->store), so call it first before referencing the instruction
    * store pointer from an index
    */
   if (emit_endif)
      insn = next_insn(p, BRW_OPCODE_ENDIF);

   /* Pop the IF and (optional) ELSE instructions from the stack */
   p->if_depth_in_loop[p->loop_stack_depth]--;
   tmp = pop_if_stack(p);
   if (tmp->header.opcode == BRW_OPCODE_ELSE) {
      else_inst = tmp;
      tmp = pop_if_stack(p);
   }
   if_inst = tmp;

   if (!emit_endif) {
      /* ENDIF is useless; don't bother emitting it. */
      convert_IF_ELSE_to_ADD(p, if_inst, else_inst);
      return;
   }

   if (intel->gen < 6) {
      brw_set_dest(p, insn, retype(brw_vec4_grf(0,0), BRW_REGISTER_TYPE_UD));
      brw_set_src0(p, insn, retype(brw_vec4_grf(0,0), BRW_REGISTER_TYPE_UD));
      brw_set_src1(p, insn, brw_imm_d(0x0));
   } else if (intel->gen == 6) {
      brw_set_dest(p, insn, brw_imm_w(0));
      brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   } else {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(p, insn, brw_imm_ud(0));
   }

   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.mask_control = BRW_MASK_ENABLE;
   insn->header.thread_control = BRW_THREAD_SWITCH;

   /* Also pop item off the stack in the endif instruction: */
   if (intel->gen < 6) {
      insn->bits3.if_else.jump_count = 0;
      insn->bits3.if_else.pop_count = 1;
      insn->bits3.if_else.pad0 = 0;
   } else if (intel->gen == 6) {
      insn->bits1.branch_gen6.jump_count = 2;
   } else {
      insn->bits3.break_cont.jip = 2;
   }
   patch_IF_ELSE(p, if_inst, else_inst, insn);
}

struct brw_instruction *brw_BREAK(struct brw_compile *p)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;

   insn = next_insn(p, BRW_OPCODE_BREAK);
   if (intel->gen >= 6) {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(p, insn, brw_imm_d(0x0));
   } else {
      brw_set_dest(p, insn, brw_ip_reg());
      brw_set_src0(p, insn, brw_ip_reg());
      brw_set_src1(p, insn, brw_imm_d(0x0));
      insn->bits3.if_else.pad0 = 0;
      insn->bits3.if_else.pop_count = p->if_depth_in_loop[p->loop_stack_depth];
   }
   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.execution_size = BRW_EXECUTE_8;

   return insn;
}

struct brw_instruction *gen6_CONT(struct brw_compile *p)
{
   struct brw_instruction *insn;

   insn = next_insn(p, BRW_OPCODE_CONTINUE);
   brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   brw_set_dest(p, insn, brw_ip_reg());
   brw_set_src0(p, insn, brw_ip_reg());
   brw_set_src1(p, insn, brw_imm_d(0x0));

   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.execution_size = BRW_EXECUTE_8;
   return insn;
}

struct brw_instruction *brw_CONT(struct brw_compile *p)
{
   struct brw_instruction *insn;
   insn = next_insn(p, BRW_OPCODE_CONTINUE);
   brw_set_dest(p, insn, brw_ip_reg());
   brw_set_src0(p, insn, brw_ip_reg());
   brw_set_src1(p, insn, brw_imm_d(0x0));
   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.execution_size = BRW_EXECUTE_8;
   /* insn->header.mask_control = BRW_MASK_DISABLE; */
   insn->bits3.if_else.pad0 = 0;
   insn->bits3.if_else.pop_count = p->if_depth_in_loop[p->loop_stack_depth];
   return insn;
}

/* DO/WHILE loop:
 *
 * The DO/WHILE is just an unterminated loop -- break or continue are
 * used for control within the loop.  We have a few ways they can be
 * done.
 *
 * For uniform control flow, the WHILE is just a jump, so ADD ip, ip,
 * jip and no DO instruction.
 *
 * For non-uniform control flow pre-gen6, there's a DO instruction to
 * push the mask, and a WHILE to jump back, and BREAK to get out and
 * pop the mask.
 *
 * For gen6, there's no more mask stack, so no need for DO.  WHILE
 * just points back to the first instruction of the loop.
 */
struct brw_instruction *brw_DO(struct brw_compile *p, GLuint execute_size)
{
   struct intel_context *intel = &p->brw->intel;

   if (intel->gen >= 6 || p->single_program_flow) {
      push_loop_stack(p, &p->store[p->nr_insn]);
      return &p->store[p->nr_insn];
   } else {
      struct brw_instruction *insn = next_insn(p, BRW_OPCODE_DO);

      push_loop_stack(p, insn);

      /* Override the defaults for this instruction:
       */
      brw_set_dest(p, insn, brw_null_reg());
      brw_set_src0(p, insn, brw_null_reg());
      brw_set_src1(p, insn, brw_null_reg());

      insn->header.compression_control = BRW_COMPRESSION_NONE;
      insn->header.execution_size = execute_size;
      insn->header.predicate_control = BRW_PREDICATE_NONE;
      /* insn->header.mask_control = BRW_MASK_ENABLE; */
      /* insn->header.mask_control = BRW_MASK_DISABLE; */

      return insn;
   }
}

/**
 * For pre-gen6, we patch BREAK/CONT instructions to point at the WHILE
 * instruction here.
 *
 * For gen6+, see brw_set_uip_jip(), which doesn't care so much about the loop
 * nesting, since it can always just point to the end of the block/current loop.
 */
static void
brw_patch_break_cont(struct brw_compile *p, struct brw_instruction *while_inst)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *do_inst = get_inner_do_insn(p);
   struct brw_instruction *inst;
   int br = (intel->gen == 5) ? 2 : 1;

   for (inst = while_inst - 1; inst != do_inst; inst--) {
      /* If the jump count is != 0, that means that this instruction has already
       * been patched because it's part of a loop inside of the one we're
       * patching.
       */
      if (inst->header.opcode == BRW_OPCODE_BREAK &&
	  inst->bits3.if_else.jump_count == 0) {
	 inst->bits3.if_else.jump_count = br * ((while_inst - inst) + 1);
      } else if (inst->header.opcode == BRW_OPCODE_CONTINUE &&
		 inst->bits3.if_else.jump_count == 0) {
	 inst->bits3.if_else.jump_count = br * (while_inst - inst);
      }
   }
}

struct brw_instruction *brw_WHILE(struct brw_compile *p)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn, *do_insn;
   GLuint br = 1;

   if (intel->gen >= 5)
      br = 2;

   if (intel->gen >= 7) {
      insn = next_insn(p, BRW_OPCODE_WHILE);
      do_insn = get_inner_do_insn(p);

      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(p, insn, brw_imm_ud(0));
      insn->bits3.break_cont.jip = br * (do_insn - insn);

      insn->header.execution_size = BRW_EXECUTE_8;
   } else if (intel->gen == 6) {
      insn = next_insn(p, BRW_OPCODE_WHILE);
      do_insn = get_inner_do_insn(p);

      brw_set_dest(p, insn, brw_imm_w(0));
      insn->bits1.branch_gen6.jump_count = br * (do_insn - insn);
      brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));

      insn->header.execution_size = BRW_EXECUTE_8;
   } else {
      if (p->single_program_flow) {
	 insn = next_insn(p, BRW_OPCODE_ADD);
         do_insn = get_inner_do_insn(p);

	 brw_set_dest(p, insn, brw_ip_reg());
	 brw_set_src0(p, insn, brw_ip_reg());
	 brw_set_src1(p, insn, brw_imm_d((do_insn - insn) * 16));
	 insn->header.execution_size = BRW_EXECUTE_1;
      } else {
	 insn = next_insn(p, BRW_OPCODE_WHILE);
         do_insn = get_inner_do_insn(p);

	 assert(do_insn->header.opcode == BRW_OPCODE_DO);

	 brw_set_dest(p, insn, brw_ip_reg());
	 brw_set_src0(p, insn, brw_ip_reg());
	 brw_set_src1(p, insn, brw_imm_d(0));

	 insn->header.execution_size = do_insn->header.execution_size;
	 insn->bits3.if_else.jump_count = br * (do_insn - insn + 1);
	 insn->bits3.if_else.pop_count = 0;
	 insn->bits3.if_else.pad0 = 0;

	 brw_patch_break_cont(p, insn);
      }
   }
   insn->header.compression_control = BRW_COMPRESSION_NONE;
   p->current->header.predicate_control = BRW_PREDICATE_NONE;

   p->loop_stack_depth--;

   return insn;
}


/* FORWARD JUMPS:
 */
void brw_land_fwd_jump(struct brw_compile *p, int jmp_insn_idx)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *jmp_insn = &p->store[jmp_insn_idx];
   GLuint jmpi = 1;

   if (intel->gen >= 5)
      jmpi = 2;

   assert(jmp_insn->header.opcode == BRW_OPCODE_JMPI);
   assert(jmp_insn->bits1.da1.src1_reg_file == BRW_IMMEDIATE_VALUE);

   jmp_insn->bits3.ud = jmpi * (p->nr_insn - jmp_insn_idx - 1);
}



/* To integrate with the above, it makes sense that the comparison
 * instruction should populate the flag register.  It might be simpler
 * just to use the flag reg for most WM tasks?
 */
void brw_CMP(struct brw_compile *p,
	     struct brw_reg dest,
	     GLuint conditional,
	     struct brw_reg src0,
	     struct brw_reg src1)
{
   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_CMP);

   insn->header.destreg__conditionalmod = conditional;
   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_src1(p, insn, src1);

/*    guess_execution_size(insn, src0); */


   /* Make it so that future instructions will use the computed flag
    * value until brw_set_predicate_control_flag_value() is called
    * again.  
    */
   if (dest.file == BRW_ARCHITECTURE_REGISTER_FILE &&
       dest.nr == 0) {
      p->current->header.predicate_control = BRW_PREDICATE_NORMAL;
      p->flag_value = 0xff;
   }
}

/* Issue 'wait' instruction for n1, host could program MMIO
   to wake up thread. */
void brw_WAIT (struct brw_compile *p)
{
   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_WAIT);
   struct brw_reg src = brw_notification_1_reg();

   brw_set_dest(p, insn, src);
   brw_set_src0(p, insn, src);
   brw_set_src1(p, insn, brw_null_reg());
   insn->header.execution_size = 0; /* must */
   insn->header.predicate_control = 0;
   insn->header.compression_control = 0;
}


/***********************************************************************
 * Helpers for the various SEND message types:
 */

/** Extended math function, float[8].
 */
void brw_math( struct brw_compile *p,
	       struct brw_reg dest,
	       GLuint function,
	       GLuint msg_reg_nr,
	       struct brw_reg src,
	       GLuint data_type,
	       GLuint precision )
{
   struct intel_context *intel = &p->brw->intel;

   if (intel->gen >= 6) {
      struct brw_instruction *insn = next_insn(p, BRW_OPCODE_MATH);

      assert(dest.file == BRW_GENERAL_REGISTER_FILE);
      assert(src.file == BRW_GENERAL_REGISTER_FILE);

      assert(dest.hstride == BRW_HORIZONTAL_STRIDE_1);
      if (intel->gen == 6)
	 assert(src.hstride == BRW_HORIZONTAL_STRIDE_1);

      /* Source modifiers are ignored for extended math instructions on Gen6. */
      if (intel->gen == 6) {
	 assert(!src.negate);
	 assert(!src.abs);
      }

      if (function == BRW_MATH_FUNCTION_INT_DIV_QUOTIENT ||
	  function == BRW_MATH_FUNCTION_INT_DIV_REMAINDER ||
	  function == BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER) {
	 assert(src.type != BRW_REGISTER_TYPE_F);
      } else {
	 assert(src.type == BRW_REGISTER_TYPE_F);
      }

      /* Math is the same ISA format as other opcodes, except that CondModifier
       * becomes FC[3:0] and ThreadCtrl becomes FC[5:4].
       */
      insn->header.destreg__conditionalmod = function;

      brw_set_dest(p, insn, dest);
      brw_set_src0(p, insn, src);
      brw_set_src1(p, insn, brw_null_reg());
   } else {
      struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);

      /* Example code doesn't set predicate_control for send
       * instructions.
       */
      insn->header.predicate_control = 0;
      insn->header.destreg__conditionalmod = msg_reg_nr;

      brw_set_dest(p, insn, dest);
      brw_set_src0(p, insn, src);
      brw_set_math_message(p,
			   insn,
			   function,
			   src.type == BRW_REGISTER_TYPE_D,
			   precision,
			   data_type);
   }
}

/** Extended math function, float[8].
 */
void brw_math2(struct brw_compile *p,
	       struct brw_reg dest,
	       GLuint function,
	       struct brw_reg src0,
	       struct brw_reg src1)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_MATH);

   assert(intel->gen >= 6);
   (void) intel;


   assert(dest.file == BRW_GENERAL_REGISTER_FILE);
   assert(src0.file == BRW_GENERAL_REGISTER_FILE);
   assert(src1.file == BRW_GENERAL_REGISTER_FILE);

   assert(dest.hstride == BRW_HORIZONTAL_STRIDE_1);
   if (intel->gen == 6) {
      assert(src0.hstride == BRW_HORIZONTAL_STRIDE_1);
      assert(src1.hstride == BRW_HORIZONTAL_STRIDE_1);
   }

   if (function == BRW_MATH_FUNCTION_INT_DIV_QUOTIENT ||
       function == BRW_MATH_FUNCTION_INT_DIV_REMAINDER ||
       function == BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER) {
      assert(src0.type != BRW_REGISTER_TYPE_F);
      assert(src1.type != BRW_REGISTER_TYPE_F);
   } else {
      assert(src0.type == BRW_REGISTER_TYPE_F);
      assert(src1.type == BRW_REGISTER_TYPE_F);
   }

   /* Source modifiers are ignored for extended math instructions on Gen6. */
   if (intel->gen == 6) {
      assert(!src0.negate);
      assert(!src0.abs);
      assert(!src1.negate);
      assert(!src1.abs);
   }

   /* Math is the same ISA format as other opcodes, except that CondModifier
    * becomes FC[3:0] and ThreadCtrl becomes FC[5:4].
    */
   insn->header.destreg__conditionalmod = function;

   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_src1(p, insn, src1);
}

/**
 * Extended math function, float[16].
 * Use 2 send instructions.
 */
void brw_math_16( struct brw_compile *p,
		  struct brw_reg dest,
		  GLuint function,
		  GLuint msg_reg_nr,
		  struct brw_reg src,
		  GLuint precision )
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;

   if (intel->gen >= 6) {
      insn = next_insn(p, BRW_OPCODE_MATH);

      /* Math is the same ISA format as other opcodes, except that CondModifier
       * becomes FC[3:0] and ThreadCtrl becomes FC[5:4].
       */
      insn->header.destreg__conditionalmod = function;

      /* Source modifiers are ignored for extended math instructions. */
      assert(!src.negate);
      assert(!src.abs);

      brw_set_dest(p, insn, dest);
      brw_set_src0(p, insn, src);
      brw_set_src1(p, insn, brw_null_reg());
      return;
   }

   /* First instruction:
    */
   brw_push_insn_state(p);
   brw_set_predicate_control_flag_value(p, 0xff);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);

   insn = next_insn(p, BRW_OPCODE_SEND);
   insn->header.destreg__conditionalmod = msg_reg_nr;

   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src);
   brw_set_math_message(p,
			insn, 
			function,
			BRW_MATH_INTEGER_UNSIGNED,
			precision,
			BRW_MATH_DATA_VECTOR);

   /* Second instruction:
    */
   insn = next_insn(p, BRW_OPCODE_SEND);
   insn->header.compression_control = BRW_COMPRESSION_2NDHALF;
   insn->header.destreg__conditionalmod = msg_reg_nr+1;

   brw_set_dest(p, insn, offset(dest,1));
   brw_set_src0(p, insn, src);
   brw_set_math_message(p, 
			insn, 
			function,
			BRW_MATH_INTEGER_UNSIGNED,
			precision,
			BRW_MATH_DATA_VECTOR);

   brw_pop_insn_state(p);
}


/**
 * Write a block of OWORDs (half a GRF each) from the scratch buffer,
 * using a constant offset per channel.
 *
 * The offset must be aligned to oword size (16 bytes).  Used for
 * register spilling.
 */
void brw_oword_block_write_scratch(struct brw_compile *p,
				   struct brw_reg mrf,
				   int num_regs,
				   GLuint offset)
{
   struct intel_context *intel = &p->brw->intel;
   uint32_t msg_control, msg_type;
   int mlen;

   if (intel->gen >= 6)
      offset /= 16;

   mrf = retype(mrf, BRW_REGISTER_TYPE_UD);

   if (num_regs == 1) {
      msg_control = BRW_DATAPORT_OWORD_BLOCK_2_OWORDS;
      mlen = 2;
   } else {
      msg_control = BRW_DATAPORT_OWORD_BLOCK_4_OWORDS;
      mlen = 3;
   }

   /* Set up the message header.  This is g0, with g0.2 filled with
    * the offset.  We don't want to leave our offset around in g0 or
    * it'll screw up texture samples, so set it up inside the message
    * reg.
    */
   {
      brw_push_insn_state(p);
      brw_set_mask_control(p, BRW_MASK_DISABLE);
      brw_set_compression_control(p, BRW_COMPRESSION_NONE);

      brw_MOV(p, mrf, retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));

      /* set message header global offset field (reg 0, element 2) */
      brw_MOV(p,
	      retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE,
				  mrf.nr,
				  2), BRW_REGISTER_TYPE_UD),
	      brw_imm_ud(offset));

      brw_pop_insn_state(p);
   }

   {
      struct brw_reg dest;
      struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);
      int send_commit_msg;
      struct brw_reg src_header = retype(brw_vec8_grf(0, 0),
					 BRW_REGISTER_TYPE_UW);

      if (insn->header.compression_control != BRW_COMPRESSION_NONE) {
	 insn->header.compression_control = BRW_COMPRESSION_NONE;
	 src_header = vec16(src_header);
      }
      assert(insn->header.predicate_control == BRW_PREDICATE_NONE);
      insn->header.destreg__conditionalmod = mrf.nr;

      /* Until gen6, writes followed by reads from the same location
       * are not guaranteed to be ordered unless write_commit is set.
       * If set, then a no-op write is issued to the destination
       * register to set a dependency, and a read from the destination
       * can be used to ensure the ordering.
       *
       * For gen6, only writes between different threads need ordering
       * protection.  Our use of DP writes is all about register
       * spilling within a thread.
       */
      if (intel->gen >= 6) {
	 dest = retype(vec16(brw_null_reg()), BRW_REGISTER_TYPE_UW);
	 send_commit_msg = 0;
      } else {
	 dest = src_header;
	 send_commit_msg = 1;
      }

      brw_set_dest(p, insn, dest);
      if (intel->gen >= 6) {
	 brw_set_src0(p, insn, mrf);
      } else {
	 brw_set_src0(p, insn, brw_null_reg());
      }

      if (intel->gen >= 6)
	 msg_type = GEN6_DATAPORT_WRITE_MESSAGE_OWORD_BLOCK_WRITE;
      else
	 msg_type = BRW_DATAPORT_WRITE_MESSAGE_OWORD_BLOCK_WRITE;

      brw_set_dp_write_message(p,
			       insn,
			       255, /* binding table index (255=stateless) */
			       msg_control,
			       msg_type,
			       mlen,
			       true, /* header_present */
			       0, /* not a render target */
			       send_commit_msg, /* response_length */
			       0, /* eot */
			       send_commit_msg);
   }
}


/**
 * Read a block of owords (half a GRF each) from the scratch buffer
 * using a constant index per channel.
 *
 * Offset must be aligned to oword size (16 bytes).  Used for register
 * spilling.
 */
void
brw_oword_block_read_scratch(struct brw_compile *p,
			     struct brw_reg dest,
			     struct brw_reg mrf,
			     int num_regs,
			     GLuint offset)
{
   struct intel_context *intel = &p->brw->intel;
   uint32_t msg_control;
   int rlen;

   if (intel->gen >= 6)
      offset /= 16;

   mrf = retype(mrf, BRW_REGISTER_TYPE_UD);
   dest = retype(dest, BRW_REGISTER_TYPE_UW);

   if (num_regs == 1) {
      msg_control = BRW_DATAPORT_OWORD_BLOCK_2_OWORDS;
      rlen = 1;
   } else {
      msg_control = BRW_DATAPORT_OWORD_BLOCK_4_OWORDS;
      rlen = 2;
   }

   {
      brw_push_insn_state(p);
      brw_set_compression_control(p, BRW_COMPRESSION_NONE);
      brw_set_mask_control(p, BRW_MASK_DISABLE);

      brw_MOV(p, mrf, retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));

      /* set message header global offset field (reg 0, element 2) */
      brw_MOV(p,
	      retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE,
				  mrf.nr,
				  2), BRW_REGISTER_TYPE_UD),
	      brw_imm_ud(offset));

      brw_pop_insn_state(p);
   }

   {
      struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);

      assert(insn->header.predicate_control == 0);
      insn->header.compression_control = BRW_COMPRESSION_NONE;
      insn->header.destreg__conditionalmod = mrf.nr;

      brw_set_dest(p, insn, dest);	/* UW? */
      if (intel->gen >= 6) {
	 brw_set_src0(p, insn, mrf);
      } else {
	 brw_set_src0(p, insn, brw_null_reg());
      }

      brw_set_dp_read_message(p,
			      insn,
			      255, /* binding table index (255=stateless) */
			      msg_control,
			      BRW_DATAPORT_READ_MESSAGE_OWORD_BLOCK_READ, /* msg_type */
			      BRW_DATAPORT_READ_TARGET_RENDER_CACHE,
			      1, /* msg_length */
			      rlen);
   }
}

/**
 * Read a float[4] vector from the data port Data Cache (const buffer).
 * Location (in buffer) should be a multiple of 16.
 * Used for fetching shader constants.
 */
void brw_oword_block_read(struct brw_compile *p,
			  struct brw_reg dest,
			  struct brw_reg mrf,
			  uint32_t offset,
			  uint32_t bind_table_index)
{
   struct intel_context *intel = &p->brw->intel;

   /* On newer hardware, offset is in units of owords. */
   if (intel->gen >= 6)
      offset /= 16;

   mrf = retype(mrf, BRW_REGISTER_TYPE_UD);

   brw_push_insn_state(p);
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_set_mask_control(p, BRW_MASK_DISABLE);

   brw_MOV(p, mrf, retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));

   /* set message header global offset field (reg 0, element 2) */
   brw_MOV(p,
	   retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE,
			       mrf.nr,
			       2), BRW_REGISTER_TYPE_UD),
	   brw_imm_ud(offset));

   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);
   insn->header.destreg__conditionalmod = mrf.nr;

   /* cast dest to a uword[8] vector */
   dest = retype(vec8(dest), BRW_REGISTER_TYPE_UW);

   brw_set_dest(p, insn, dest);
   if (intel->gen >= 6) {
      brw_set_src0(p, insn, mrf);
   } else {
      brw_set_src0(p, insn, brw_null_reg());
   }

   brw_set_dp_read_message(p,
			   insn,
			   bind_table_index,
			   BRW_DATAPORT_OWORD_BLOCK_1_OWORDLOW,
			   BRW_DATAPORT_READ_MESSAGE_OWORD_BLOCK_READ,
			   BRW_DATAPORT_READ_TARGET_DATA_CACHE,
			   1, /* msg_length */
			   1); /* response_length (1 reg, 2 owords!) */

   brw_pop_insn_state(p);
}

/**
 * Read a set of dwords from the data port Data Cache (const buffer).
 *
 * Location (in buffer) appears as UD offsets in the register after
 * the provided mrf header reg.
 */
void brw_dword_scattered_read(struct brw_compile *p,
			      struct brw_reg dest,
			      struct brw_reg mrf,
			      uint32_t bind_table_index)
{
   mrf = retype(mrf, BRW_REGISTER_TYPE_UD);

   brw_push_insn_state(p);
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_MOV(p, mrf, retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));
   brw_pop_insn_state(p);

   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);
   insn->header.destreg__conditionalmod = mrf.nr;

   /* cast dest to a uword[8] vector */
   dest = retype(vec8(dest), BRW_REGISTER_TYPE_UW);

   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, brw_null_reg());

   brw_set_dp_read_message(p,
			   insn,
			   bind_table_index,
			   BRW_DATAPORT_DWORD_SCATTERED_BLOCK_8DWORDS,
			   BRW_DATAPORT_READ_MESSAGE_DWORD_SCATTERED_READ,
			   BRW_DATAPORT_READ_TARGET_DATA_CACHE,
			   2, /* msg_length */
			   1); /* response_length */
}



/**
 * Read float[4] constant(s) from VS constant buffer.
 * For relative addressing, two float[4] constants will be read into 'dest'.
 * Otherwise, one float[4] constant will be read into the lower half of 'dest'.
 */
void brw_dp_READ_4_vs(struct brw_compile *p,
                      struct brw_reg dest,
                      GLuint location,
                      GLuint bind_table_index)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;
   GLuint msg_reg_nr = 1;

   if (intel->gen >= 6)
      location /= 16;

   /* Setup MRF[1] with location/offset into const buffer */
   brw_push_insn_state(p);
   brw_set_access_mode(p, BRW_ALIGN_1);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);
   brw_MOV(p, retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE, msg_reg_nr, 2),
		     BRW_REGISTER_TYPE_UD),
	   brw_imm_ud(location));
   brw_pop_insn_state(p);

   insn = next_insn(p, BRW_OPCODE_SEND);

   insn->header.predicate_control = BRW_PREDICATE_NONE;
   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.destreg__conditionalmod = msg_reg_nr;
   insn->header.mask_control = BRW_MASK_DISABLE;

   brw_set_dest(p, insn, dest);
   if (intel->gen >= 6) {
      brw_set_src0(p, insn, brw_message_reg(msg_reg_nr));
   } else {
      brw_set_src0(p, insn, brw_null_reg());
   }

   brw_set_dp_read_message(p,
			   insn,
			   bind_table_index,
			   0,
			   BRW_DATAPORT_READ_MESSAGE_OWORD_BLOCK_READ, /* msg_type */
			   BRW_DATAPORT_READ_TARGET_DATA_CACHE,
			   1, /* msg_length */
			   1); /* response_length (1 Oword) */
}

/**
 * Read a float[4] constant per vertex from VS constant buffer, with
 * relative addressing.
 */
void brw_dp_READ_4_vs_relative(struct brw_compile *p,
			       struct brw_reg dest,
			       struct brw_reg addr_reg,
			       GLuint offset,
			       GLuint bind_table_index)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_reg src = brw_vec8_grf(0, 0);
   int msg_type;

   /* Setup MRF[1] with offset into const buffer */
   brw_push_insn_state(p);
   brw_set_access_mode(p, BRW_ALIGN_1);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);

   /* M1.0 is block offset 0, M1.4 is block offset 1, all other
    * fields ignored.
    */
   brw_ADD(p, retype(brw_message_reg(1), BRW_REGISTER_TYPE_D),
	   addr_reg, brw_imm_d(offset));
   brw_pop_insn_state(p);

   gen6_resolve_implied_move(p, &src, 0);
   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);

   insn->header.predicate_control = BRW_PREDICATE_NONE;
   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.destreg__conditionalmod = 0;
   insn->header.mask_control = BRW_MASK_DISABLE;

   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src);

   if (intel->gen >= 6)
      msg_type = GEN6_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;
   else if (intel->gen == 5 || intel->is_g4x)
      msg_type = G45_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;
   else
      msg_type = BRW_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;

   brw_set_dp_read_message(p,
			   insn,
			   bind_table_index,
			   BRW_DATAPORT_OWORD_DUAL_BLOCK_1OWORD,
			   msg_type,
			   BRW_DATAPORT_READ_TARGET_DATA_CACHE,
			   2, /* msg_length */
			   1); /* response_length */
}



void brw_fb_WRITE(struct brw_compile *p,
		  int dispatch_width,
                  GLuint msg_reg_nr,
                  struct brw_reg src0,
                  GLuint msg_control,
                  GLuint binding_table_index,
                  GLuint msg_length,
                  GLuint response_length,
                  bool eot,
                  bool header_present)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;
   GLuint msg_type;
   struct brw_reg dest;

   if (dispatch_width == 16)
      dest = retype(vec16(brw_null_reg()), BRW_REGISTER_TYPE_UW);
   else
      dest = retype(vec8(brw_null_reg()), BRW_REGISTER_TYPE_UW);

   if (intel->gen >= 6) {
      insn = next_insn(p, BRW_OPCODE_SENDC);
   } else {
      insn = next_insn(p, BRW_OPCODE_SEND);
   }
   /* The execution mask is ignored for render target writes. */
   insn->header.predicate_control = 0;
   insn->header.compression_control = BRW_COMPRESSION_NONE;

   if (intel->gen >= 6) {
      /* headerless version, just submit color payload */
      src0 = brw_message_reg(msg_reg_nr);

      msg_type = GEN6_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE;
   } else {
      insn->header.destreg__conditionalmod = msg_reg_nr;

      msg_type = BRW_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE;
   }

   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_dp_write_message(p,
			    insn,
			    binding_table_index,
			    msg_control,
			    msg_type,
			    msg_length,
			    header_present,
			    eot, /* last render target write */
			    response_length,
			    eot,
			    0 /* send_commit_msg */);
}


/**
 * Texture sample instruction.
 * Note: the msg_type plus msg_length values determine exactly what kind
 * of sampling operation is performed.  See volume 4, page 161 of docs.
 */
void brw_SAMPLE(struct brw_compile *p,
		struct brw_reg dest,
		GLuint msg_reg_nr,
		struct brw_reg src0,
		GLuint binding_table_index,
		GLuint sampler,
		GLuint writemask,
		GLuint msg_type,
		GLuint response_length,
		GLuint msg_length,
		GLuint header_present,
		GLuint simd_mode,
		GLuint return_format)
{
   struct intel_context *intel = &p->brw->intel;
   bool need_stall = 0;

   if (writemask == 0) {
      /*printf("%s: zero writemask??\n", __FUNCTION__); */
      return;
   }
   
   /* Hardware doesn't do destination dependency checking on send
    * instructions properly.  Add a workaround which generates the
    * dependency by other means.  In practice it seems like this bug
    * only crops up for texture samples, and only where registers are
    * written by the send and then written again later without being
    * read in between.  Luckily for us, we already track that
    * information and use it to modify the writemask for the
    * instruction, so that is a guide for whether a workaround is
    * needed.
    */
   if (writemask != WRITEMASK_XYZW) {
      GLuint dst_offset = 0;
      GLuint i, newmask = 0, len = 0;

      for (i = 0; i < 4; i++) {
	 if (writemask & (1<<i))
	    break;
	 dst_offset += 2;
      }
      for (; i < 4; i++) {
	 if (!(writemask & (1<<i)))
	    break;
	 newmask |= 1<<i;
	 len++;
      }

      if (newmask != writemask) {
	 need_stall = 1;
         /* printf("need stall %x %x\n", newmask , writemask); */
      }
      else {
	 bool dispatch_16 = false;

	 struct brw_reg m1 = brw_message_reg(msg_reg_nr);

	 guess_execution_size(p, p->current, dest);
	 if (p->current->header.execution_size == BRW_EXECUTE_16)
	    dispatch_16 = true;

	 newmask = ~newmask & WRITEMASK_XYZW;

	 brw_push_insn_state(p);

	 brw_set_compression_control(p, BRW_COMPRESSION_NONE);
	 brw_set_mask_control(p, BRW_MASK_DISABLE);

	 brw_MOV(p, retype(m1, BRW_REGISTER_TYPE_UD),
		 retype(brw_vec8_grf(0,0), BRW_REGISTER_TYPE_UD));
  	 brw_MOV(p, get_element_ud(m1, 2), brw_imm_ud(newmask << 12)); 

	 brw_pop_insn_state(p);

  	 src0 = retype(brw_null_reg(), BRW_REGISTER_TYPE_UW); 
	 dest = offset(dest, dst_offset);

	 /* For 16-wide dispatch, masked channels are skipped in the
	  * response.  For 8-wide, masked channels still take up slots,
	  * and are just not written to.
	  */
	 if (dispatch_16)
	    response_length = len * 2;
      }
   }

   {
      struct brw_instruction *insn;
   
      gen6_resolve_implied_move(p, &src0, msg_reg_nr);

      insn = next_insn(p, BRW_OPCODE_SEND);
      insn->header.predicate_control = 0; /* XXX */
      insn->header.compression_control = BRW_COMPRESSION_NONE;
      if (intel->gen < 6)
	  insn->header.destreg__conditionalmod = msg_reg_nr;

      brw_set_dest(p, insn, dest);
      brw_set_src0(p, insn, src0);
      brw_set_sampler_message(p, insn,
			      binding_table_index,
			      sampler,
			      msg_type,
			      response_length, 
			      msg_length,
			      header_present,
			      simd_mode,
			      return_format);
   }

   if (need_stall) {
      struct brw_reg reg = vec8(offset(dest, response_length-1));

      /*  mov (8) r9.0<1>:f    r9.0<8;8,1>:f    { Align1 }
       */
      brw_push_insn_state(p);
      brw_set_compression_control(p, BRW_COMPRESSION_NONE);
      brw_MOV(p, retype(reg, BRW_REGISTER_TYPE_UD),
	      retype(reg, BRW_REGISTER_TYPE_UD));
      brw_pop_insn_state(p);
   }

}

/* All these variables are pretty confusing - we might be better off
 * using bitmasks and macros for this, in the old style.  Or perhaps
 * just having the caller instantiate the fields in dword3 itself.
 */
void brw_urb_WRITE(struct brw_compile *p,
		   struct brw_reg dest,
		   GLuint msg_reg_nr,
		   struct brw_reg src0,
		   bool allocate,
		   bool used,
		   GLuint msg_length,
		   GLuint response_length,
		   bool eot,
		   bool writes_complete,
		   GLuint offset,
		   GLuint swizzle)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;

   gen6_resolve_implied_move(p, &src0, msg_reg_nr);

   if (intel->gen == 7) {
      /* Enable Channel Masks in the URB_WRITE_HWORD message header */
      brw_push_insn_state(p);
      brw_set_access_mode(p, BRW_ALIGN_1);
      brw_OR(p, retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE, msg_reg_nr, 5),
		       BRW_REGISTER_TYPE_UD),
	        retype(brw_vec1_grf(0, 5), BRW_REGISTER_TYPE_UD),
		brw_imm_ud(0xff00));
      brw_pop_insn_state(p);
   }

   insn = next_insn(p, BRW_OPCODE_SEND);

   assert(msg_length < BRW_MAX_MRF);

   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_src1(p, insn, brw_imm_d(0));

   if (intel->gen < 6)
      insn->header.destreg__conditionalmod = msg_reg_nr;

   brw_set_urb_message(p,
		       insn,
		       allocate,
		       used,
		       msg_length,
		       response_length, 
		       eot, 
		       writes_complete, 
		       offset,
		       swizzle);
}

static int
brw_find_next_block_end(struct brw_compile *p, int start)
{
   int ip;

   for (ip = start + 1; ip < p->nr_insn; ip++) {
      struct brw_instruction *insn = &p->store[ip];

      switch (insn->header.opcode) {
      case BRW_OPCODE_ENDIF:
      case BRW_OPCODE_ELSE:
      case BRW_OPCODE_WHILE:
	 return ip;
      }
   }
   assert(!"not reached");
   return start + 1;
}

/* There is no DO instruction on gen6, so to find the end of the loop
 * we have to see if the loop is jumping back before our start
 * instruction.
 */
static int
brw_find_loop_end(struct brw_compile *p, int start)
{
   struct intel_context *intel = &p->brw->intel;
   int ip;
   int br = 2;

   for (ip = start + 1; ip < p->nr_insn; ip++) {
      struct brw_instruction *insn = &p->store[ip];

      if (insn->header.opcode == BRW_OPCODE_WHILE) {
	 int jip = intel->gen == 6 ? insn->bits1.branch_gen6.jump_count
				   : insn->bits3.break_cont.jip;
	 if (ip + jip / br <= start)
	    return ip;
      }
   }
   assert(!"not reached");
   return start + 1;
}

/* After program generation, go back and update the UIP and JIP of
 * BREAK and CONT instructions to their correct locations.
 */
void
brw_set_uip_jip(struct brw_compile *p)
{
   struct intel_context *intel = &p->brw->intel;
   int ip;
   int br = 2;

   if (intel->gen < 6)
      return;

   for (ip = 0; ip < p->nr_insn; ip++) {
      struct brw_instruction *insn = &p->store[ip];

      switch (insn->header.opcode) {
      case BRW_OPCODE_BREAK:
	 insn->bits3.break_cont.jip = br * (brw_find_next_block_end(p, ip) - ip);
	 /* Gen7 UIP points to WHILE; Gen6 points just after it */
	 insn->bits3.break_cont.uip =
	    br * (brw_find_loop_end(p, ip) - ip + (intel->gen == 6 ? 1 : 0));
	 break;
      case BRW_OPCODE_CONTINUE:
	 insn->bits3.break_cont.jip = br * (brw_find_next_block_end(p, ip) - ip);
	 insn->bits3.break_cont.uip = br * (brw_find_loop_end(p, ip) - ip);

	 assert(insn->bits3.break_cont.uip != 0);
	 assert(insn->bits3.break_cont.jip != 0);
	 break;
      }
   }
}

void brw_ff_sync(struct brw_compile *p,
		   struct brw_reg dest,
		   GLuint msg_reg_nr,
		   struct brw_reg src0,
		   bool allocate,
		   GLuint response_length,
		   bool eot)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;

   gen6_resolve_implied_move(p, &src0, msg_reg_nr);

   insn = next_insn(p, BRW_OPCODE_SEND);
   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_src1(p, insn, brw_imm_d(0));

   if (intel->gen < 6)
      insn->header.destreg__conditionalmod = msg_reg_nr;

   brw_set_ff_sync_message(p,
			   insn,
			   allocate,
			   response_length,
			   eot);
}

/**
 * Emit the SEND instruction necessary to generate stream output data on Gen6
 * (for transform feedback).
 *
 * If send_commit_msg is true, this is the last piece of stream output data
 * from this thread, so send the data as a committed write.  According to the
 * Sandy Bridge PRM (volume 2 part 1, section 4.5.1):
 *
 *   "Prior to End of Thread with a URB_WRITE, the kernel must ensure all
 *   writes are complete by sending the final write as a committed write."
 */
void
brw_svb_write(struct brw_compile *p,
              struct brw_reg dest,
              GLuint msg_reg_nr,
              struct brw_reg src0,
              GLuint binding_table_index,
              bool   send_commit_msg)
{
   struct brw_instruction *insn;

   gen6_resolve_implied_move(p, &src0, msg_reg_nr);

   insn = next_insn(p, BRW_OPCODE_SEND);
   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_src1(p, insn, brw_imm_d(0));
   brw_set_dp_write_message(p, insn,
                            binding_table_index,
                            0, /* msg_control: ignored */
                            GEN6_DATAPORT_WRITE_MESSAGE_STREAMED_VB_WRITE,
                            1, /* msg_length */
                            true, /* header_present */
                            0, /* last_render_target: ignored */
                            send_commit_msg, /* response_length */
                            0, /* end_of_thread */
                            send_commit_msg); /* send_commit_msg */
}
