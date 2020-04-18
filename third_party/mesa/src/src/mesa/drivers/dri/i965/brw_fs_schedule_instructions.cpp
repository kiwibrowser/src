/*
 * Copyright © 2010 Intel Corporation
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
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "brw_fs.h"
#include "glsl/glsl_types.h"
#include "glsl/ir_optimization.h"
#include "glsl/ir_print_visitor.h"

/** @file brw_fs_schedule_instructions.cpp
 *
 * List scheduling of FS instructions.
 *
 * The basic model of the list scheduler is to take a basic block,
 * compute a DAG of the dependencies (RAW ordering with latency, WAW
 * ordering, WAR ordering), and make a list of the DAG heads.
 * Heuristically pick a DAG head, then put all the children that are
 * now DAG heads into the list of things to schedule.
 *
 * The heuristic is the important part.  We're trying to be cheap,
 * since actually computing the optimal scheduling is NP complete.
 * What we do is track a "current clock".  When we schedule a node, we
 * update the earliest-unblocked clock time of its children, and
 * increment the clock.  Then, when trying to schedule, we just pick
 * the earliest-unblocked instruction to schedule.
 *
 * Note that often there will be many things which could execute
 * immediately, and there are a range of heuristic options to choose
 * from in picking among those.
 */

class schedule_node : public exec_node
{
public:
   schedule_node(fs_inst *inst)
   {
      this->inst = inst;
      this->child_array_size = 0;
      this->children = NULL;
      this->child_latency = NULL;
      this->child_count = 0;
      this->parent_count = 0;
      this->unblocked_time = 0;

      int chans = 8;
      int math_latency = 22;

      switch (inst->opcode) {
      case SHADER_OPCODE_RCP:
	 this->latency = 1 * chans * math_latency;
	 break;
      case SHADER_OPCODE_RSQ:
	 this->latency = 2 * chans * math_latency;
	 break;
      case SHADER_OPCODE_INT_QUOTIENT:
      case SHADER_OPCODE_SQRT:
      case SHADER_OPCODE_LOG2:
	 /* full precision log.  partial is 2. */
	 this->latency = 3 * chans * math_latency;
	 break;
      case SHADER_OPCODE_INT_REMAINDER:
      case SHADER_OPCODE_EXP2:
	 /* full precision.  partial is 3, same throughput. */
	 this->latency = 4 * chans * math_latency;
	 break;
      case SHADER_OPCODE_POW:
	 this->latency = 8 * chans * math_latency;
	 break;
      case SHADER_OPCODE_SIN:
      case SHADER_OPCODE_COS:
	 /* minimum latency, max is 12 rounds. */
	 this->latency = 5 * chans * math_latency;
	 break;
      default:
	 this->latency = 2;
	 break;
      }
   }

   fs_inst *inst;
   schedule_node **children;
   int *child_latency;
   int child_count;
   int parent_count;
   int child_array_size;
   int unblocked_time;
   int latency;
};

class instruction_scheduler {
public:
   instruction_scheduler(fs_visitor *v, void *mem_ctx, int virtual_grf_count)
   {
      this->v = v;
      this->mem_ctx = ralloc_context(mem_ctx);
      this->virtual_grf_count = virtual_grf_count;
      this->instructions.make_empty();
      this->instructions_to_schedule = 0;
   }

   ~instruction_scheduler()
   {
      ralloc_free(this->mem_ctx);
   }
   void add_barrier_deps(schedule_node *n);
   void add_dep(schedule_node *before, schedule_node *after, int latency);
   void add_dep(schedule_node *before, schedule_node *after);

   void add_inst(fs_inst *inst);
   void calculate_deps();
   void schedule_instructions(fs_inst *next_block_header);

   bool is_compressed(fs_inst *inst);

   void *mem_ctx;

   int instructions_to_schedule;
   int virtual_grf_count;
   exec_list instructions;
   fs_visitor *v;
};

void
instruction_scheduler::add_inst(fs_inst *inst)
{
   schedule_node *n = new(mem_ctx) schedule_node(inst);

   assert(!inst->is_head_sentinel());
   assert(!inst->is_tail_sentinel());

   this->instructions_to_schedule++;

   inst->remove();
   instructions.push_tail(n);
}

/**
 * Add a dependency between two instruction nodes.
 *
 * The @after node will be scheduled after @before.  We will try to
 * schedule it @latency cycles after @before, but no guarantees there.
 */
void
instruction_scheduler::add_dep(schedule_node *before, schedule_node *after,
			       int latency)
{
   if (!before || !after)
      return;

   assert(before != after);

   for (int i = 0; i < before->child_count; i++) {
      if (before->children[i] == after) {
	 before->child_latency[i] = MAX2(before->child_latency[i], latency);
	 return;
      }
   }

   if (before->child_array_size <= before->child_count) {
      if (before->child_array_size < 16)
	 before->child_array_size = 16;
      else
	 before->child_array_size *= 2;

      before->children = reralloc(mem_ctx, before->children,
				  schedule_node *,
				  before->child_array_size);
      before->child_latency = reralloc(mem_ctx, before->child_latency,
				       int, before->child_array_size);
   }

   before->children[before->child_count] = after;
   before->child_latency[before->child_count] = latency;
   before->child_count++;
   after->parent_count++;
}

void
instruction_scheduler::add_dep(schedule_node *before, schedule_node *after)
{
   if (!before)
      return;

   add_dep(before, after, before->latency);
}

/**
 * Sometimes we really want this node to execute after everything that
 * was before it and before everything that followed it.  This adds
 * the deps to do so.
 */
void
instruction_scheduler::add_barrier_deps(schedule_node *n)
{
   schedule_node *prev = (schedule_node *)n->prev;
   schedule_node *next = (schedule_node *)n->next;

   if (prev) {
      while (!prev->is_head_sentinel()) {
	 add_dep(prev, n, 0);
	 prev = (schedule_node *)prev->prev;
      }
   }

   if (next) {
      while (!next->is_tail_sentinel()) {
	 add_dep(n, next, 0);
	 next = (schedule_node *)next->next;
      }
   }
}

/* instruction scheduling needs to be aware of when an MRF write
 * actually writes 2 MRFs.
 */
bool
instruction_scheduler::is_compressed(fs_inst *inst)
{
   return (v->c->dispatch_width == 16 &&
	   !inst->force_uncompressed &&
	   !inst->force_sechalf);
}

void
instruction_scheduler::calculate_deps()
{
   schedule_node *last_grf_write[virtual_grf_count];
   schedule_node *last_mrf_write[BRW_MAX_MRF];
   schedule_node *last_conditional_mod = NULL;
   /* Fixed HW registers are assumed to be separate from the virtual
    * GRFs, so they can be tracked separately.  We don't really write
    * to fixed GRFs much, so don't bother tracking them on a more
    * granular level.
    */
   schedule_node *last_fixed_grf_write = NULL;

   /* The last instruction always needs to still be the last
    * instruction.  Either it's flow control (IF, ELSE, ENDIF, DO,
    * WHILE) and scheduling other things after it would disturb the
    * basic block, or it's FB_WRITE and we should do a better job at
    * dead code elimination anyway.
    */
   schedule_node *last = (schedule_node *)instructions.get_tail();
   add_barrier_deps(last);

   memset(last_grf_write, 0, sizeof(last_grf_write));
   memset(last_mrf_write, 0, sizeof(last_mrf_write));

   /* top-to-bottom dependencies: RAW and WAW. */
   foreach_list(node, &instructions) {
      schedule_node *n = (schedule_node *)node;
      fs_inst *inst = n->inst;

      /* read-after-write deps. */
      for (int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF) {
	    add_dep(last_grf_write[inst->src[i].reg], n);
	 } else if (inst->src[i].file == FIXED_HW_REG &&
		    (inst->src[i].fixed_hw_reg.file ==
		     BRW_GENERAL_REGISTER_FILE)) {
	    add_dep(last_fixed_grf_write, n);
	 } else if (inst->src[i].file != BAD_FILE &&
		    inst->src[i].file != IMM &&
		    inst->src[i].file != UNIFORM) {
	    assert(inst->src[i].file != MRF);
	    add_barrier_deps(n);
	 }
      }

      for (int i = 0; i < inst->mlen; i++) {
	 /* It looks like the MRF regs are released in the send
	  * instruction once it's sent, not when the result comes
	  * back.
	  */
	 add_dep(last_mrf_write[inst->base_mrf + i], n);
      }

      if (inst->predicated) {
	 assert(last_conditional_mod);
	 add_dep(last_conditional_mod, n);
      }

      /* write-after-write deps. */
      if (inst->dst.file == GRF) {
	 add_dep(last_grf_write[inst->dst.reg], n);
	 last_grf_write[inst->dst.reg] = n;
      } else if (inst->dst.file == MRF) {
	 int reg = inst->dst.reg & ~BRW_MRF_COMPR4;

	 add_dep(last_mrf_write[reg], n);
	 last_mrf_write[reg] = n;
	 if (is_compressed(inst)) {
	    if (inst->dst.reg & BRW_MRF_COMPR4)
	       reg += 4;
	    else
	       reg++;
	    add_dep(last_mrf_write[reg], n);
	    last_mrf_write[reg] = n;
	 }
      } else if (inst->dst.file == FIXED_HW_REG &&
		 inst->dst.fixed_hw_reg.file == BRW_GENERAL_REGISTER_FILE) {
	 last_fixed_grf_write = n;
      } else if (inst->dst.file != BAD_FILE) {
	 add_barrier_deps(n);
      }

      if (inst->mlen > 0) {
	 for (int i = 0; i < v->implied_mrf_writes(inst); i++) {
	    add_dep(last_mrf_write[inst->base_mrf + i], n);
	    last_mrf_write[inst->base_mrf + i] = n;
	 }
      }

      /* Treat FS_OPCODE_MOV_DISPATCH_TO_FLAGS as though it had a
       * conditional_mod, because it sets the flag register.
       */
      if (inst->conditional_mod ||
          inst->opcode == FS_OPCODE_MOV_DISPATCH_TO_FLAGS) {
	 add_dep(last_conditional_mod, n, 0);
	 last_conditional_mod = n;
      }
   }

   /* bottom-to-top dependencies: WAR */
   memset(last_grf_write, 0, sizeof(last_grf_write));
   memset(last_mrf_write, 0, sizeof(last_mrf_write));
   last_conditional_mod = NULL;
   last_fixed_grf_write = NULL;

   exec_node *node;
   exec_node *prev;
   for (node = instructions.get_tail(), prev = node->prev;
	!node->is_head_sentinel();
	node = prev, prev = node->prev) {
      schedule_node *n = (schedule_node *)node;
      fs_inst *inst = n->inst;

      /* write-after-read deps. */
      for (int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF) {
	    add_dep(n, last_grf_write[inst->src[i].reg]);
	 } else if (inst->src[i].file == FIXED_HW_REG &&
		    (inst->src[i].fixed_hw_reg.file ==
		     BRW_GENERAL_REGISTER_FILE)) {
	    add_dep(n, last_fixed_grf_write);
	 } else if (inst->src[i].file != BAD_FILE &&
		    inst->src[i].file != IMM &&
		    inst->src[i].file != UNIFORM) {
	    assert(inst->src[i].file != MRF);
	    add_barrier_deps(n);
	 }
      }

      for (int i = 0; i < inst->mlen; i++) {
	 /* It looks like the MRF regs are released in the send
	  * instruction once it's sent, not when the result comes
	  * back.
	  */
	 add_dep(n, last_mrf_write[inst->base_mrf + i], 2);
      }

      if (inst->predicated) {
	 add_dep(n, last_conditional_mod);
      }

      /* Update the things this instruction wrote, so earlier reads
       * can mark this as WAR dependency.
       */
      if (inst->dst.file == GRF) {
	 last_grf_write[inst->dst.reg] = n;
      } else if (inst->dst.file == MRF) {
	 int reg = inst->dst.reg & ~BRW_MRF_COMPR4;

	 last_mrf_write[reg] = n;

	 if (is_compressed(inst)) {
	    if (inst->dst.reg & BRW_MRF_COMPR4)
	       reg += 4;
	    else
	       reg++;

	    last_mrf_write[reg] = n;
	 }
      } else if (inst->dst.file == FIXED_HW_REG &&
		 inst->dst.fixed_hw_reg.file == BRW_GENERAL_REGISTER_FILE) {
	 last_fixed_grf_write = n;
      } else if (inst->dst.file != BAD_FILE) {
	 add_barrier_deps(n);
      }

      if (inst->mlen > 0) {
	 for (int i = 0; i < v->implied_mrf_writes(inst); i++) {
	    last_mrf_write[inst->base_mrf + i] = n;
	 }
      }

      /* Treat FS_OPCODE_MOV_DISPATCH_TO_FLAGS as though it had a
       * conditional_mod, because it sets the flag register.
       */
      if (inst->conditional_mod ||
          inst->opcode == FS_OPCODE_MOV_DISPATCH_TO_FLAGS) {
	 last_conditional_mod = n;
      }
   }
}

void
instruction_scheduler::schedule_instructions(fs_inst *next_block_header)
{
   int time = 0;

   /* Remove non-DAG heads from the list. */
   foreach_list_safe(node, &instructions) {
      schedule_node *n = (schedule_node *)node;
      if (n->parent_count != 0)
	 n->remove();
   }

   while (!instructions.is_empty()) {
      schedule_node *chosen = NULL;
      int chosen_time = 0;

      foreach_list(node, &instructions) {
	 schedule_node *n = (schedule_node *)node;

	 if (!chosen || n->unblocked_time < chosen_time) {
	    chosen = n;
	    chosen_time = n->unblocked_time;
	 }
      }

      /* Schedule this instruction. */
      assert(chosen);
      chosen->remove();
      next_block_header->insert_before(chosen->inst);
      instructions_to_schedule--;

      /* Bump the clock.  If we expected a delay for scheduling, then
       * bump the clock to reflect that.
       */
      time = MAX2(time + 1, chosen_time);

      /* Now that we've scheduled a new instruction, some of its
       * children can be promoted to the list of instructions ready to
       * be scheduled.  Update the children's unblocked time for this
       * DAG edge as we do so.
       */
      for (int i = 0; i < chosen->child_count; i++) {
	 schedule_node *child = chosen->children[i];

	 child->unblocked_time = MAX2(child->unblocked_time,
				      time + chosen->child_latency[i]);

	 child->parent_count--;
	 if (child->parent_count == 0) {
	    instructions.push_tail(child);
	 }
      }

      /* Shared resource: the mathbox.  There's one per EU (on later
       * generations, it's even more limited pre-gen6), so if we send
       * something off to it then the next math isn't going to make
       * progress until the first is done.
       */
      if (chosen->inst->is_math()) {
	 foreach_list(node, &instructions) {
	    schedule_node *n = (schedule_node *)node;

	    if (n->inst->is_math())
	       n->unblocked_time = MAX2(n->unblocked_time,
					time + chosen->latency);
	 }
      }
   }

   assert(instructions_to_schedule == 0);
}

void
fs_visitor::schedule_instructions()
{
   fs_inst *next_block_header = (fs_inst *)instructions.head;
   instruction_scheduler sched(this, mem_ctx, this->virtual_grf_count);

   while (!next_block_header->is_tail_sentinel()) {
      /* Add things to be scheduled until we get to a new BB. */
      while (!next_block_header->is_tail_sentinel()) {
	 fs_inst *inst = next_block_header;
	 next_block_header = (fs_inst *)next_block_header->next;

	 sched.add_inst(inst);
	 if (inst->opcode == BRW_OPCODE_IF ||
	     inst->opcode == BRW_OPCODE_ELSE ||
	     inst->opcode == BRW_OPCODE_ENDIF ||
	     inst->opcode == BRW_OPCODE_DO ||
	     inst->opcode == BRW_OPCODE_WHILE ||
	     inst->opcode == BRW_OPCODE_BREAK ||
	     inst->opcode == BRW_OPCODE_CONTINUE) {
	    break;
	 }
      }
      sched.calculate_deps();
      sched.schedule_instructions(next_block_header);
   }

   this->live_intervals_valid = false;
}
