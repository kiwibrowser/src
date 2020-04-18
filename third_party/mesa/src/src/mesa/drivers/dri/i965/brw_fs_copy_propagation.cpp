/*
 * Copyright © 2012 Intel Corporation
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

#include "brw_fs.h"
#include "brw_fs_cfg.h"

namespace { /* avoid conflict with opt_copy_propagation_elements */
struct acp_entry : public exec_node {
   fs_reg dst;
   fs_reg src;
};
}

bool
fs_visitor::try_copy_propagate(fs_inst *inst, int arg, acp_entry *entry)
{
   if (inst->src[arg].file != entry->dst.file ||
       inst->src[arg].reg != entry->dst.reg ||
       inst->src[arg].reg_offset != entry->dst.reg_offset) {
      return false;
   }

   /* See resolve_ud_negate() and comment in brw_fs_emit.cpp. */
   if (inst->conditional_mod &&
       inst->src[arg].type == BRW_REGISTER_TYPE_UD &&
       entry->src.negate)
      return false;

   bool has_source_modifiers = entry->src.abs || entry->src.negate;

   if (intel->gen == 6 && inst->is_math() &&
       (has_source_modifiers || entry->src.file == UNIFORM))
      return false;

   inst->src[arg].file = entry->src.file;
   inst->src[arg].reg = entry->src.reg;
   inst->src[arg].reg_offset = entry->src.reg_offset;

   if (!inst->src[arg].abs) {
      inst->src[arg].abs = entry->src.abs;
      inst->src[arg].negate ^= entry->src.negate;
   }

   return true;
}

/** @file brw_fs_copy_propagation.cpp
 *
 * Support for local copy propagation by walking the list of instructions
 * and maintaining the ACP table of available copies for propagation.
 *
 * See Muchnik's Advanced Compiler Design and Implementation, section
 * 12.5 (p356).
 */

/* Walks a basic block and does copy propagation on it using the acp
 * list.
 */
bool
fs_visitor::opt_copy_propagate_local(void *mem_ctx,
				     fs_bblock *block, exec_list *acp)
{
   bool progress = false;

   for (fs_inst *inst = block->start;
	inst != block->end->next;
	inst = (fs_inst *)inst->next) {

      /* Try propagating into this instruction. */
      foreach_list(entry_node, acp) {
	 acp_entry *entry = (acp_entry *)entry_node;

	 for (int i = 0; i < 3; i++) {
	    if (try_copy_propagate(inst, i, entry))
	       progress = true;
	 }
      }

      /* kill the destination from the ACP */
      if (inst->dst.file == GRF) {
	 foreach_list_safe(entry_node, acp) {
	    acp_entry *entry = (acp_entry *)entry_node;

	    if (inst->overwrites_reg(entry->dst) ||
                inst->overwrites_reg(entry->src)) {
	       entry->remove();
	    }
	 }
      }

      /* If this instruction is a raw copy, add it to the ACP. */
      if (inst->opcode == BRW_OPCODE_MOV &&
	  inst->dst.file == GRF &&
	  ((inst->src[0].file == GRF &&
	    (inst->src[0].reg != inst->dst.reg ||
	     inst->src[0].reg_offset != inst->dst.reg_offset)) ||
	   inst->src[0].file == UNIFORM) &&
	  inst->src[0].type == inst->dst.type &&
	  !inst->saturate &&
	  !inst->predicated &&
	  !inst->force_uncompressed &&
	  !inst->force_sechalf &&
	  inst->src[0].smear == -1) {
	 acp_entry *entry = ralloc(mem_ctx, acp_entry);
	 entry->dst = inst->dst;
	 entry->src = inst->src[0];
	 acp->push_tail(entry);
      }
   }

   return progress;
}

bool
fs_visitor::opt_copy_propagate()
{
   bool progress = false;
   void *mem_ctx = ralloc_context(this->mem_ctx);

   fs_cfg cfg(this);

   for (int b = 0; b < cfg.num_blocks; b++) {
      fs_bblock *block = cfg.blocks[b];
      exec_list acp;

      progress = opt_copy_propagate_local(mem_ctx, block, &acp) || progress;
   }

   ralloc_free(mem_ctx);

   if (progress)
      live_intervals_valid = false;

   return progress;
}
