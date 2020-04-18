/*
 * Copyright © 2011 Intel Corporation
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

/**
 * @file brw_vec4_copy_propagation.cpp
 *
 * Implements tracking of values copied between registers, and
 * optimizations based on that: copy propagation and constant
 * propagation.
 */

#include "brw_vec4.h"
extern "C" {
#include "main/macros.h"
}

namespace brw {

static bool
is_direct_copy(vec4_instruction *inst)
{
   return (inst->opcode == BRW_OPCODE_MOV &&
	   !inst->predicate &&
	   inst->dst.file == GRF &&
	   !inst->saturate &&
	   !inst->dst.reladdr &&
	   !inst->src[0].reladdr &&
	   inst->dst.type == inst->src[0].type);
}

static bool
is_dominated_by_previous_instruction(vec4_instruction *inst)
{
   return (inst->opcode != BRW_OPCODE_DO &&
	   inst->opcode != BRW_OPCODE_WHILE &&
	   inst->opcode != BRW_OPCODE_ELSE &&
	   inst->opcode != BRW_OPCODE_ENDIF);
}

static bool
try_constant_propagation(vec4_instruction *inst, int arg, src_reg *values[4])
{
   /* For constant propagation, we only handle the same constant
    * across all 4 channels.  Some day, we should handle the 8-bit
    * float vector format, which would let us constant propagate
    * vectors better.
    */
   src_reg value = *values[0];
   for (int i = 1; i < 4; i++) {
      if (!value.equals(values[i]))
	 return false;
   }

   if (value.file != IMM)
      return false;

   if (inst->src[arg].abs) {
      if (value.type == BRW_REGISTER_TYPE_F) {
	 value.imm.f = fabs(value.imm.f);
      } else if (value.type == BRW_REGISTER_TYPE_D) {
	 if (value.imm.i < 0)
	    value.imm.i = -value.imm.i;
      }
   }

   if (inst->src[arg].negate) {
      if (value.type == BRW_REGISTER_TYPE_F)
	 value.imm.f = -value.imm.f;
      else
	 value.imm.u = -value.imm.u;
   }

   switch (inst->opcode) {
   case BRW_OPCODE_MOV:
      inst->src[arg] = value;
      return true;

   case BRW_OPCODE_MUL:
   case BRW_OPCODE_ADD:
      if (arg == 1) {
	 inst->src[arg] = value;
	 return true;
      } else if (arg == 0 && inst->src[1].file != IMM) {
	 /* Fit this constant in by commuting the operands.  Exception: we
	  * can't do this for 32-bit integer MUL because it's asymmetric.
	  */
	 if (inst->opcode == BRW_OPCODE_MUL &&
	     (inst->src[1].type == BRW_REGISTER_TYPE_D ||
	      inst->src[1].type == BRW_REGISTER_TYPE_UD))
	    break;
	 inst->src[0] = inst->src[1];
	 inst->src[1] = value;
	 return true;
      }
      break;

   case BRW_OPCODE_CMP:
      if (arg == 1) {
	 inst->src[arg] = value;
	 return true;
      } else if (arg == 0 && inst->src[1].file != IMM) {
	 uint32_t new_cmod;

	 new_cmod = brw_swap_cmod(inst->conditional_mod);
	 if (new_cmod != ~0u) {
	    /* Fit this constant in by swapping the operands and
	     * flipping the test.
	     */
	    inst->src[0] = inst->src[1];
	    inst->src[1] = value;
	    inst->conditional_mod = new_cmod;
	    return true;
	 }
      }
      break;

   case BRW_OPCODE_SEL:
      if (arg == 1) {
	 inst->src[arg] = value;
	 return true;
      } else if (arg == 0 && inst->src[1].file != IMM) {
	 inst->src[0] = inst->src[1];
	 inst->src[1] = value;

	 /* If this was predicated, flipping operands means
	  * we also need to flip the predicate.
	  */
	 if (inst->conditional_mod == BRW_CONDITIONAL_NONE) {
	    inst->predicate_inverse = !inst->predicate_inverse;
	 }
	 return true;
      }
      break;

   default:
      break;
   }

   return false;
}

static bool
try_copy_propagation(struct intel_context *intel,
		     vec4_instruction *inst, int arg, src_reg *values[4])
{
   /* For constant propagation, we only handle the same constant
    * across all 4 channels.  Some day, we should handle the 8-bit
    * float vector format, which would let us constant propagate
    * vectors better.
    */
   src_reg value = *values[0];
   for (int i = 1; i < 4; i++) {
      /* This is equals() except we don't care about the swizzle. */
      if (value.file != values[i]->file ||
	  value.reg != values[i]->reg ||
	  value.reg_offset != values[i]->reg_offset ||
	  value.type != values[i]->type ||
	  value.negate != values[i]->negate ||
	  value.abs != values[i]->abs) {
	 return false;
      }
   }

   /* Compute the swizzle of the original register by swizzling the
    * component loaded from each value according to the swizzle of
    * operand we're going to change.
    */
   int s[4];
   for (int i = 0; i < 4; i++) {
      s[i] = BRW_GET_SWZ(values[i]->swizzle,
			 BRW_GET_SWZ(inst->src[arg].swizzle, i));
   }
   value.swizzle = BRW_SWIZZLE4(s[0], s[1], s[2], s[3]);

   if (value.file != UNIFORM &&
       value.file != GRF &&
       value.file != ATTR)
      return false;

   if (inst->src[arg].abs) {
      value.negate = false;
      value.abs = true;
   }
   if (inst->src[arg].negate)
      value.negate = !value.negate;

   /* FINISHME: We can't copy-propagate things that aren't normal
    * vec8s into gen6 math instructions, because of the weird src
    * handling for those instructions.  Just ignore them for now.
    */
   if (intel->gen >= 6 && inst->is_math())
      return false;

   /* We can't copy-propagate a UD negation into a condmod
    * instruction, because the condmod ends up looking at the 33-bit
    * signed accumulator value instead of the 32-bit value we wanted
    */
   if (inst->conditional_mod &&
       value.negate &&
       value.type == BRW_REGISTER_TYPE_UD)
      return false;

   /* Don't report progress if this is a noop. */
   if (value.equals(&inst->src[arg]))
      return false;

   value.type = inst->src[arg].type;
   inst->src[arg] = value;
   return true;
}

bool
vec4_visitor::opt_copy_propagation()
{
   bool progress = false;
   src_reg *cur_value[virtual_grf_reg_count][4];

   memset(&cur_value, 0, sizeof(cur_value));

   foreach_list(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      /* This pass only works on basic blocks.  If there's flow
       * control, throw out all our information and start from
       * scratch.
       *
       * This should really be fixed by using a structure like in
       * src/glsl/opt_copy_propagation.cpp to track available copies.
       */
      if (!is_dominated_by_previous_instruction(inst)) {
	 memset(cur_value, 0, sizeof(cur_value));
	 continue;
      }

      /* For each source arg, see if each component comes from a copy
       * from the same type file (IMM, GRF, UNIFORM), and try
       * optimizing out access to the copy result
       */
      for (int i = 2; i >= 0; i--) {
	 /* Copied values end up in GRFs, and we don't track reladdr
	  * accesses.
	  */
	 if (inst->src[i].file != GRF ||
	     inst->src[i].reladdr)
	    continue;

	 int reg = (virtual_grf_reg_map[inst->src[i].reg] +
		    inst->src[i].reg_offset);

	 /* Find the regs that each swizzle component came from.
	  */
	 src_reg *values[4];
	 int c;
	 for (c = 0; c < 4; c++) {
	    values[c] = cur_value[reg][BRW_GET_SWZ(inst->src[i].swizzle, c)];

	    /* If there's no available copy for this channel, bail.
	     * We could be more aggressive here -- some channels might
	     * not get used based on the destination writemask.
	     */
	    if (!values[c])
	       break;

	    /* We'll only be able to copy propagate if the sources are
	     * all from the same file -- there's no ability to swizzle
	     * 0 or 1 constants in with source registers like in i915.
	     */
	    if (c > 0 && values[c - 1]->file != values[c]->file)
	       break;
	 }

	 if (c != 4)
	    continue;

	 if (try_constant_propagation(inst, i, values) ||
	     try_copy_propagation(intel, inst, i, values))
	    progress = true;
      }

      /* Track available source registers. */
      if (inst->dst.file == GRF) {
	 const int reg =
	    virtual_grf_reg_map[inst->dst.reg] + inst->dst.reg_offset;

	 /* Update our destination's current channel values.  For a direct copy,
	  * the value is the newly propagated source.  Otherwise, we don't know
	  * the new value, so clear it.
	  */
	 bool direct_copy = is_direct_copy(inst);
	 for (int i = 0; i < 4; i++) {
	    if (inst->dst.writemask & (1 << i)) {
	       cur_value[reg][i] = direct_copy ? &inst->src[0] : NULL;
	    }
	 }

	 /* Clear the records for any registers whose current value came from
	  * our destination's updated channels, as the two are no longer equal.
	  */
	 if (inst->dst.reladdr)
	    memset(cur_value, 0, sizeof(cur_value));
	 else {
	    for (int i = 0; i < virtual_grf_reg_count; i++) {
	       for (int j = 0; j < 4; j++) {
		  if (inst->dst.writemask & (1 << j) &&
		      cur_value[i][j] &&
		      cur_value[i][j]->file == GRF &&
		      cur_value[i][j]->reg == inst->dst.reg &&
		      cur_value[i][j]->reg_offset == inst->dst.reg_offset) {
		     cur_value[i][j] = NULL;
		  }
	       }
	    }
	 }
      }
   }

   if (progress)
      live_intervals_valid = false;

   return progress;
}

} /* namespace brw */
