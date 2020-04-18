/*
 * Copyright 2010 Tom Stellard <tstellar@gmail.com>
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * \file
 */

#include "radeon_emulate_loops.h"

#include "radeon_compiler.h"
#include "radeon_compiler_util.h"
#include "radeon_dataflow.h"

#define VERBOSE 0

#define DBG(...) do { if (VERBOSE) fprintf(stderr, __VA_ARGS__); } while(0)

struct const_value {
	struct radeon_compiler * C;
	struct rc_src_register * Src;
	float Value;
	int HasValue;
};

struct count_inst {
	struct radeon_compiler * C;
	int Index;
	rc_swizzle Swz;
	float Amount;
	int Unknown;
	unsigned BranchDepth;
};

static unsigned int loop_max_possible_iterations(struct radeon_compiler *c,
			struct loop_info * loop)
{
	unsigned int total_i = rc_recompute_ips(c);
	unsigned int loop_i = (loop->EndLoop->IP - loop->BeginLoop->IP) - 1;
	/* +1 because the program already has one iteration of the loop. */
	return 1 + ((c->max_alu_insts - total_i) / loop_i);
}

static void unroll_loop(struct radeon_compiler * c, struct loop_info * loop,
						unsigned int iterations)
{
	unsigned int i;
	struct rc_instruction * ptr;
	struct rc_instruction * first = loop->BeginLoop->Next;
	struct rc_instruction * last = loop->EndLoop->Prev;
	struct rc_instruction * append_to = last;
	rc_remove_instruction(loop->BeginLoop);
	rc_remove_instruction(loop->EndLoop);
	for( i = 1; i < iterations; i++){
		for(ptr = first; ptr != last->Next; ptr = ptr->Next){
			struct rc_instruction *new = rc_alloc_instruction(c);
			memcpy(new, ptr, sizeof(struct rc_instruction));
			rc_insert_instruction(append_to, new);
			append_to = new;
		}
	}
}


static void update_const_value(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int mask)
{
	struct const_value * value = data;
	if(value->Src->File != file ||
	   value->Src->Index != index ||
	   !(1 << GET_SWZ(value->Src->Swizzle, 0) & mask)){
		return;
	}
	switch(inst->U.I.Opcode){
	case RC_OPCODE_MOV:
		if(!rc_src_reg_is_immediate(value->C, inst->U.I.SrcReg[0].File,
						inst->U.I.SrcReg[0].Index)){
			return;
		}
		value->HasValue = 1;
		value->Value =
			rc_get_constant_value(value->C,
					      inst->U.I.SrcReg[0].Index,
					      inst->U.I.SrcReg[0].Swizzle,
					      inst->U.I.SrcReg[0].Negate, 0);
		break;
	}
}

static void get_incr_amount(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int mask)
{
	struct count_inst * count_inst = data;
	int amnt_src_index;
	const struct rc_opcode_info * opcode;
	float amount;

	if(file != RC_FILE_TEMPORARY ||
	   count_inst->Index != index ||
	   (1 << GET_SWZ(count_inst->Swz,0) != mask)){
		return;
	}

	/* XXX: Give up if the counter is modified within an IF block.  We
	 * could handle this case with better analysis. */
	if (count_inst->BranchDepth > 0) {
		count_inst->Unknown = 1;
		return;
	}

	/* Find the index of the counter register. */
	opcode = rc_get_opcode_info(inst->U.I.Opcode);
	if(opcode->NumSrcRegs != 2){
		count_inst->Unknown = 1;
		return;
	}
	if(inst->U.I.SrcReg[0].File == RC_FILE_TEMPORARY &&
	   inst->U.I.SrcReg[0].Index == count_inst->Index &&
	   inst->U.I.SrcReg[0].Swizzle == count_inst->Swz){
		amnt_src_index = 1;
	} else if( inst->U.I.SrcReg[1].File == RC_FILE_TEMPORARY &&
		   inst->U.I.SrcReg[1].Index == count_inst->Index &&
		   inst->U.I.SrcReg[1].Swizzle == count_inst->Swz){
		amnt_src_index = 0;
	}
	else{
		count_inst->Unknown = 1;
		return;
	}
	if(rc_src_reg_is_immediate(count_inst->C,
				inst->U.I.SrcReg[amnt_src_index].File,
				inst->U.I.SrcReg[amnt_src_index].Index)){
		amount = rc_get_constant_value(count_inst->C,
				inst->U.I.SrcReg[amnt_src_index].Index,
				inst->U.I.SrcReg[amnt_src_index].Swizzle,
				inst->U.I.SrcReg[amnt_src_index].Negate, 0);
	}
	else{
		count_inst->Unknown = 1 ;
		return;
	}
	switch(inst->U.I.Opcode){
	case RC_OPCODE_ADD:
		count_inst->Amount += amount;
		break;
	case RC_OPCODE_SUB:
		if(amnt_src_index == 0){
			count_inst->Unknown = 0;
			return;
		}
		count_inst->Amount -= amount;
		break;
	default:
		count_inst->Unknown = 1;
		return;
	}
}

/**
 * If c->max_alu_inst is -1, then all eligible loops will be unrolled regardless
 * of how many iterations they have.
 */
static int try_unroll_loop(struct radeon_compiler * c, struct loop_info * loop)
{
	int end_loops;
	int iterations;
	struct count_inst count_inst;
	float limit_value;
	struct rc_src_register * counter;
	struct rc_src_register * limit;
	struct const_value counter_value;
	struct rc_instruction * inst;

	/* Find the counter and the upper limit */

	if(rc_src_reg_is_immediate(c, loop->Cond->U.I.SrcReg[0].File,
					loop->Cond->U.I.SrcReg[0].Index)){
		limit = &loop->Cond->U.I.SrcReg[0];
		counter = &loop->Cond->U.I.SrcReg[1];
	}
	else if(rc_src_reg_is_immediate(c, loop->Cond->U.I.SrcReg[1].File,
					loop->Cond->U.I.SrcReg[1].Index)){
		limit = &loop->Cond->U.I.SrcReg[1];
		counter = &loop->Cond->U.I.SrcReg[0];
	}
	else{
		DBG("No constant limit.\n");
		return 0;
	}

	/* Find the initial value of the counter */
	counter_value.Src = counter;
	counter_value.Value = 0.0f;
	counter_value.HasValue = 0;
	counter_value.C = c;
	for(inst = c->Program.Instructions.Next; inst != loop->BeginLoop;
							inst = inst->Next){
		rc_for_all_writes_mask(inst, update_const_value, &counter_value);
	}
	if(!counter_value.HasValue){
		DBG("Initial counter value cannot be determined.\n");
		return 0;
	}
	DBG("Initial counter value is %f\n", counter_value.Value);
	/* Determine how the counter is modified each loop */
	count_inst.C = c;
	count_inst.Index = counter->Index;
	count_inst.Swz = counter->Swizzle;
	count_inst.Amount = 0.0f;
	count_inst.Unknown = 0;
	count_inst.BranchDepth = 0;
	end_loops = 1;
	for(inst = loop->BeginLoop->Next; end_loops > 0; inst = inst->Next){
		switch(inst->U.I.Opcode){
		/* XXX In the future we might want to try to unroll nested
		 * loops here.*/
		case RC_OPCODE_BGNLOOP:
			end_loops++;
			break;
		case RC_OPCODE_ENDLOOP:
			loop->EndLoop = inst;
			end_loops--;
			break;
		case RC_OPCODE_BRK:
			/* Don't unroll loops if it has a BRK instruction
			 * other one used when testing the main conditional
			 * of the loop. */

			/* Make sure we haven't entered a nested loops. */
			if(inst != loop->Brk && end_loops == 1) {
				return 0;
			}
			break;
		case RC_OPCODE_IF:
			count_inst.BranchDepth++;
			break;
		case RC_OPCODE_ENDIF:
			count_inst.BranchDepth--;
			break;
		default:
			rc_for_all_writes_mask(inst, get_incr_amount, &count_inst);
			if(count_inst.Unknown){
				return 0;
			}
			break;
		}
	}
	/* Infinite loop */
	if(count_inst.Amount == 0.0f){
		return 0;
	}
	DBG("Counter is increased by %f each iteration.\n", count_inst.Amount);
	/* Calculate the number of iterations of this loop.  Keeping this
	 * simple, since we only support increment and decrement loops.
	 */
	limit_value = rc_get_constant_value(c, limit->Index, limit->Swizzle,
							limit->Negate, 0);
	DBG("Limit is %f.\n", limit_value);
	/* The iteration calculations are opposite of what you would expect.
	 * In a normal loop, if the condition is met, then loop continues, but
	 * with our loops, if the condition is met, the is exited. */
	switch(loop->Cond->U.I.Opcode){
	case RC_OPCODE_SGE:
	case RC_OPCODE_SLE:
		iterations = (int) ceilf((limit_value - counter_value.Value) /
							count_inst.Amount);
		break;

	case RC_OPCODE_SGT:
	case RC_OPCODE_SLT:
		iterations = (int) floorf((limit_value - counter_value.Value) /
							count_inst.Amount) + 1;
		break;
	default:
		return 0;
	}

	if (c->max_alu_insts > 0
		&& iterations > loop_max_possible_iterations(c, loop)) {
		return 0;
	}

	DBG("Loop will have %d iterations.\n", iterations);

	/* Prepare loop for unrolling */
	rc_remove_instruction(loop->Cond);
	rc_remove_instruction(loop->If);
	rc_remove_instruction(loop->Brk);
	rc_remove_instruction(loop->EndIf);

	unroll_loop(c, loop, iterations);
	loop->EndLoop = NULL;
	return 1;
}

/**
 * @param c
 * @param loop
 * @param inst A pointer to a BGNLOOP instruction.
 * @return 1 if all of the members of loop where set.
 * @return 0 if there was an error and some members of loop are still NULL.
 */
static int build_loop_info(struct radeon_compiler * c, struct loop_info * loop,
						struct rc_instruction * inst)
{
	struct rc_instruction * ptr;

	if(inst->U.I.Opcode != RC_OPCODE_BGNLOOP){
		rc_error(c, "%s: expected BGNLOOP", __FUNCTION__);
		return 0;
	}

	memset(loop, 0, sizeof(struct loop_info));

	loop->BeginLoop = inst;

	for(ptr = loop->BeginLoop->Next; !loop->EndLoop; ptr = ptr->Next) {

		if (ptr == &c->Program.Instructions) {
			rc_error(c, "%s: BGNLOOP without an ENDLOOOP.\n",
								__FUNCTION__);
			return 0;
		}

		switch(ptr->U.I.Opcode){
		case RC_OPCODE_BGNLOOP:
		{
			/* Nested loop, skip ahead to the end. */
			unsigned int loop_depth = 1;
			for(ptr = ptr->Next; ptr != &c->Program.Instructions;
							ptr = ptr->Next){
				if (ptr->U.I.Opcode == RC_OPCODE_BGNLOOP) {
					loop_depth++;
				} else if (ptr->U.I.Opcode == RC_OPCODE_ENDLOOP) {
					if (!--loop_depth) {
						break;
					}
				}
			}
			if (ptr == &c->Program.Instructions) {
				rc_error(c, "%s: BGNLOOP without an ENDLOOOP\n",
								__FUNCTION__);
					return 0;
			}
			break;
		}
		case RC_OPCODE_BRK:
			if(ptr->Next->U.I.Opcode != RC_OPCODE_ENDIF
					|| ptr->Prev->U.I.Opcode != RC_OPCODE_IF
					|| loop->Brk){
				continue;
			}
			loop->Brk = ptr;
			loop->If = ptr->Prev;
			loop->EndIf = ptr->Next;
			switch(loop->If->Prev->U.I.Opcode){
			case RC_OPCODE_SLT:
			case RC_OPCODE_SGE:
			case RC_OPCODE_SGT:
			case RC_OPCODE_SLE:
			case RC_OPCODE_SEQ:
			case RC_OPCODE_SNE:
				break;
			default:
				return 0;
			}
			loop->Cond = loop->If->Prev;
			break;

		case RC_OPCODE_ENDLOOP:
			loop->EndLoop = ptr;
			break;
		}
	}

	if (loop->BeginLoop && loop->Brk && loop->If && loop->EndIf
					&& loop->Cond && loop->EndLoop) {
		return 1;
	}
	return 0;
}

/**
 * This function prepares a loop to be unrolled by converting it into an if
 * statement.  Here is an outline of the conversion process:
 * BGNLOOP;                         	-> BGNLOOP;
 * <Additional conditional code>	-> <Additional conditional code>
 * SGE/SLT temp[0], temp[1], temp[2];	-> SLT/SGE temp[0], temp[1], temp[2];
 * IF temp[0];                      	-> IF temp[0];
 * BRK;                             	->
 * ENDIF;                           	-> <Loop Body>
 * <Loop Body>                      	-> ENDIF;
 * ENDLOOP;                         	-> ENDLOOP
 *
 * @param inst A pointer to a BGNLOOP instruction.
 * @return 1 for success, 0 for failure
 */
static int transform_loop(struct emulate_loop_state * s,
						struct rc_instruction * inst)
{
	struct loop_info * loop;

	memory_pool_array_reserve(&s->C->Pool, struct loop_info,
			s->Loops, s->LoopCount, s->LoopReserved, 1);

	loop = &s->Loops[s->LoopCount++];

	if (!build_loop_info(s->C, loop, inst)) {
		rc_error(s->C, "Failed to build loop info\n");
		return 0;
	}

	if(try_unroll_loop(s->C, loop)){
		return 1;
	}

	/* Reverse the conditional instruction */
	switch(loop->Cond->U.I.Opcode){
	case RC_OPCODE_SGE:
		loop->Cond->U.I.Opcode = RC_OPCODE_SLT;
		break;
	case RC_OPCODE_SLT:
		loop->Cond->U.I.Opcode = RC_OPCODE_SGE;
		break;
	case RC_OPCODE_SLE:
		loop->Cond->U.I.Opcode = RC_OPCODE_SGT;
		break;
	case RC_OPCODE_SGT:
		loop->Cond->U.I.Opcode = RC_OPCODE_SLE;
		break;
	case RC_OPCODE_SEQ:
		loop->Cond->U.I.Opcode = RC_OPCODE_SNE;
		break;
	case RC_OPCODE_SNE:
		loop->Cond->U.I.Opcode = RC_OPCODE_SEQ;
		break;
	default:
		rc_error(s->C, "loop->Cond is not a conditional.\n");
		return 0;
	}

	/* Prepare the loop to be emulated */
	rc_remove_instruction(loop->Brk);
	rc_remove_instruction(loop->EndIf);
	rc_insert_instruction(loop->EndLoop->Prev, loop->EndIf);
	return 1;
}

void rc_transform_loops(struct radeon_compiler *c, void *user)
{
	struct emulate_loop_state * s = &c->loop_state;
	struct rc_instruction * ptr;

	memset(s, 0, sizeof(struct emulate_loop_state));
	s->C = c;
	for(ptr = s->C->Program.Instructions.Next;
			ptr != &s->C->Program.Instructions; ptr = ptr->Next) {
		if(ptr->Type == RC_INSTRUCTION_NORMAL &&
					ptr->U.I.Opcode == RC_OPCODE_BGNLOOP){
			if (!transform_loop(s, ptr))
				return;
		}
	}
}

void rc_unroll_loops(struct radeon_compiler *c, void *user)
{
	struct rc_instruction * inst;
	struct loop_info loop;

	for(inst = c->Program.Instructions.Next;
			inst != &c->Program.Instructions; inst = inst->Next) {

		if (inst->U.I.Opcode == RC_OPCODE_BGNLOOP) {
			if (build_loop_info(c, &loop, inst)) {
				try_unroll_loop(c, &loop);
			}
		}
	}
}

void rc_emulate_loops(struct radeon_compiler *c, void *user)
{
	struct emulate_loop_state * s = &c->loop_state;
	int i;
	/* Iterate backwards of the list of loops so that loops that nested
	 * loops are unrolled first.
	 */
	for( i = s->LoopCount - 1; i >= 0; i-- ){
		unsigned int iterations;

		if(!s->Loops[i].EndLoop){
			continue;
		}
		iterations = loop_max_possible_iterations(s->C, &s->Loops[i]);
		unroll_loop(s->C, &s->Loops[i], iterations);
	}
}
