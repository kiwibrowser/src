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

/** @file brw_fs_cse.cpp
 *
 * Support for local common subexpression elimination.
 *
 * See Muchnik's Advanced Compiler Design and Implementation, section
 * 13.1 (p378).
 */

namespace {
struct aeb_entry : public exec_node {
   /** The instruction that generates the expression value. */
   fs_inst *generator;

   /** The temporary where the value is stored. */
   fs_reg tmp;
};
}

static bool
is_expression(const fs_inst *const inst)
{
   switch (inst->opcode) {
   case BRW_OPCODE_SEL:
   case BRW_OPCODE_NOT:
   case BRW_OPCODE_AND:
   case BRW_OPCODE_OR:
   case BRW_OPCODE_XOR:
   case BRW_OPCODE_SHR:
   case BRW_OPCODE_SHL:
   case BRW_OPCODE_RSR:
   case BRW_OPCODE_RSL:
   case BRW_OPCODE_ASR:
   case BRW_OPCODE_ADD:
   case BRW_OPCODE_MUL:
   case BRW_OPCODE_FRC:
   case BRW_OPCODE_RNDU:
   case BRW_OPCODE_RNDD:
   case BRW_OPCODE_RNDE:
   case BRW_OPCODE_RNDZ:
   case BRW_OPCODE_LINE:
   case BRW_OPCODE_PLN:
   case BRW_OPCODE_MAD:
   case FS_OPCODE_CINTERP:
   case FS_OPCODE_LINTERP:
      return true;
   default:
      return false;
   }
}

static bool
operands_match(fs_reg *xs, fs_reg *ys)
{
   return xs[0].equals(ys[0]) && xs[1].equals(ys[1]) && xs[2].equals(ys[2]);
}

bool
fs_visitor::opt_cse_local(fs_bblock *block, exec_list *aeb)
{
   bool progress = false;

   void *mem_ctx = ralloc_context(this->mem_ctx);

   for (fs_inst *inst = block->start;
	inst != block->end->next;
	inst = (fs_inst *) inst->next) {

      /* Skip some cases. */
      if (is_expression(inst) && !inst->predicated && inst->mlen == 0 &&
          !inst->force_uncompressed && !inst->force_sechalf &&
          !inst->conditional_mod)
      {
	 bool found = false;

	 aeb_entry *entry;
	 foreach_list(entry_node, aeb) {
	    entry = (aeb_entry *) entry_node;

	    /* Match current instruction's expression against those in AEB. */
	    if (inst->opcode == entry->generator->opcode &&
		inst->saturate == entry->generator->saturate &&
		operands_match(entry->generator->src, inst->src)) {

	       found = true;
	       progress = true;
	       break;
	    }
	 }

	 if (!found) {
	    /* Our first sighting of this expression.  Create an entry. */
	    aeb_entry *entry = ralloc(mem_ctx, aeb_entry);
	    entry->tmp = reg_undef;
	    entry->generator = inst;
	    aeb->push_tail(entry);
	 } else {
	    /* This is at least our second sighting of this expression.
	     * If we don't have a temporary already, make one.
	     */
	    bool no_existing_temp = entry->tmp.file == BAD_FILE;
	    if (no_existing_temp) {
	       entry->tmp = fs_reg(this, glsl_type::float_type);
	       entry->tmp.type = inst->dst.type;

	       fs_inst *copy = new(ralloc_parent(inst))
		  fs_inst(BRW_OPCODE_MOV, entry->generator->dst, entry->tmp);
	       entry->generator->insert_after(copy);
	       entry->generator->dst = entry->tmp;
	    }

	    /* dest <- temp */
	    fs_inst *copy = new(ralloc_parent(inst))
	       fs_inst(BRW_OPCODE_MOV, inst->dst, entry->tmp);
	    inst->replace_with(copy);

	    /* Appending an instruction may have changed our bblock end. */
	    if (inst == block->end) {
	       block->end = copy;
	    }

	    /* Continue iteration with copy->next */
	    inst = copy;
	 }
      }

      /* Kill all AEB entries that use the destination. */
      foreach_list_safe(entry_node, aeb) {
	 aeb_entry *entry = (aeb_entry *)entry_node;

	 for (int i = 0; i < 3; i++) {
            if (inst->overwrites_reg(entry->generator->src[i])) {
	       entry->remove();
	       ralloc_free(entry);
	       break;
	    }
	 }
      }
   }

   ralloc_free(mem_ctx);

   if (progress)
      this->live_intervals_valid = false;

   return progress;
}

bool
fs_visitor::opt_cse()
{
   bool progress = false;

   fs_cfg cfg(this);

   for (int b = 0; b < cfg.num_blocks; b++) {
      fs_bblock *block = cfg.blocks[b];
      exec_list aeb;

      progress = opt_cse_local(block, &aeb) || progress;
   }

   return progress;
}
