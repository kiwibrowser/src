/*
 * Copyright 2009 Nicolai Hähnle <nhaehnle@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "radeon_emulate_branches.h"

#include <stdio.h>

#include "radeon_compiler.h"
#include "radeon_dataflow.h"

#define VERBOSE 0

#define DBG(...) do { if (VERBOSE) fprintf(stderr, __VA_ARGS__); } while(0)


struct proxy_info {
	unsigned int Proxied:1;
	unsigned int Index:RC_REGISTER_INDEX_BITS;
};

struct register_proxies {
	struct proxy_info Temporary[RC_REGISTER_MAX_INDEX];
};

struct branch_info {
	struct rc_instruction * If;
	struct rc_instruction * Else;
};

struct emulate_branch_state {
	struct radeon_compiler * C;

	struct branch_info * Branches;
	unsigned int BranchCount;
	unsigned int BranchReserved;
};


static void handle_if(struct emulate_branch_state * s, struct rc_instruction * inst)
{
	struct branch_info * branch;
	struct rc_instruction * inst_mov;

	memory_pool_array_reserve(&s->C->Pool, struct branch_info,
			s->Branches, s->BranchCount, s->BranchReserved, 1);

	DBG("%s\n", __FUNCTION__);

	branch = &s->Branches[s->BranchCount++];
	memset(branch, 0, sizeof(struct branch_info));
	branch->If = inst;

	/* Make a safety copy of the decision register, because we will need
	 * it at ENDIF time and it might be overwritten in both branches. */
	inst_mov = rc_insert_new_instruction(s->C, inst->Prev);
	inst_mov->U.I.Opcode = RC_OPCODE_MOV;
	inst_mov->U.I.DstReg.File = RC_FILE_TEMPORARY;
	inst_mov->U.I.DstReg.Index = rc_find_free_temporary(s->C);
	inst_mov->U.I.DstReg.WriteMask = RC_MASK_X;
	inst_mov->U.I.SrcReg[0] = inst->U.I.SrcReg[0];

	inst->U.I.SrcReg[0].File = RC_FILE_TEMPORARY;
	inst->U.I.SrcReg[0].Index = inst_mov->U.I.DstReg.Index;
	inst->U.I.SrcReg[0].Swizzle = 0;
	inst->U.I.SrcReg[0].Abs = 0;
	inst->U.I.SrcReg[0].Negate = 0;
}

static void handle_else(struct emulate_branch_state * s, struct rc_instruction * inst)
{
	struct branch_info * branch;

	if (!s->BranchCount) {
		rc_error(s->C, "Encountered ELSE outside of branches");
		return;
	}

	DBG("%s\n", __FUNCTION__);

	branch = &s->Branches[s->BranchCount - 1];
	branch->Else = inst;
}


struct state_and_proxies {
	struct emulate_branch_state * S;
	struct register_proxies * Proxies;
};

static struct proxy_info * get_proxy_info(struct state_and_proxies * sap,
			rc_register_file file, unsigned int index)
{
	if (file == RC_FILE_TEMPORARY) {
		return &sap->Proxies->Temporary[index];
	} else {
		return 0;
	}
}

static void scan_write(void * userdata, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int comp)
{
	struct state_and_proxies * sap = userdata;
	struct proxy_info * proxy = get_proxy_info(sap, file, index);

	if (proxy && !proxy->Proxied) {
		proxy->Proxied = 1;
		proxy->Index = rc_find_free_temporary(sap->S->C);
	}
}

static void remap_proxy_function(void * userdata, struct rc_instruction * inst,
		rc_register_file * pfile, unsigned int * pindex)
{
	struct state_and_proxies * sap = userdata;
	struct proxy_info * proxy = get_proxy_info(sap, *pfile, *pindex);

	if (proxy && proxy->Proxied) {
		*pfile = RC_FILE_TEMPORARY;
		*pindex = proxy->Index;
	}
}

/**
 * Redirect all writes in the instruction range [begin, end) to proxy
 * temporary registers.
 */
static void allocate_and_insert_proxies(struct emulate_branch_state * s,
		struct register_proxies * proxies,
		struct rc_instruction * begin,
		struct rc_instruction * end)
{
	struct state_and_proxies sap;

	sap.S = s;
	sap.Proxies = proxies;

	for(struct rc_instruction * inst = begin; inst != end; inst = inst->Next) {
		rc_for_all_writes_mask(inst, scan_write, &sap);
		rc_remap_registers(inst, remap_proxy_function, &sap);
	}

	for(unsigned int index = 0; index < RC_REGISTER_MAX_INDEX; ++index) {
		if (proxies->Temporary[index].Proxied) {
			struct rc_instruction * inst_mov = rc_insert_new_instruction(s->C, begin->Prev);
			inst_mov->U.I.Opcode = RC_OPCODE_MOV;
			inst_mov->U.I.DstReg.File = RC_FILE_TEMPORARY;
			inst_mov->U.I.DstReg.Index = proxies->Temporary[index].Index;
			inst_mov->U.I.DstReg.WriteMask = RC_MASK_XYZW;
			inst_mov->U.I.SrcReg[0].File = RC_FILE_TEMPORARY;
			inst_mov->U.I.SrcReg[0].Index = index;
		}
	}
}


static void inject_cmp(struct emulate_branch_state * s,
		struct rc_instruction * inst_if,
		struct rc_instruction * inst_endif,
		rc_register_file file, unsigned int index,
		struct proxy_info ifproxy,
		struct proxy_info elseproxy)
{
	struct rc_instruction * inst_cmp = rc_insert_new_instruction(s->C, inst_endif);
	inst_cmp->U.I.Opcode = RC_OPCODE_CMP;
	inst_cmp->U.I.DstReg.File = file;
	inst_cmp->U.I.DstReg.Index = index;
	inst_cmp->U.I.DstReg.WriteMask = RC_MASK_XYZW;
	inst_cmp->U.I.SrcReg[0] = inst_if->U.I.SrcReg[0];
	inst_cmp->U.I.SrcReg[0].Abs = 1;
	inst_cmp->U.I.SrcReg[0].Negate = RC_MASK_XYZW;
	inst_cmp->U.I.SrcReg[1].File = RC_FILE_TEMPORARY;
	inst_cmp->U.I.SrcReg[1].Index = ifproxy.Proxied ? ifproxy.Index : index;
	inst_cmp->U.I.SrcReg[2].File = RC_FILE_TEMPORARY;
	inst_cmp->U.I.SrcReg[2].Index = elseproxy.Proxied ? elseproxy.Index : index;
}

static void handle_endif(struct emulate_branch_state * s, struct rc_instruction * inst)
{
	struct branch_info * branch;
	struct register_proxies IfProxies;
	struct register_proxies ElseProxies;

	if (!s->BranchCount) {
		rc_error(s->C, "Encountered ENDIF outside of branches");
		return;
	}

	DBG("%s\n", __FUNCTION__);

	branch = &s->Branches[s->BranchCount - 1];

	memset(&IfProxies, 0, sizeof(IfProxies));
	memset(&ElseProxies, 0, sizeof(ElseProxies));

	allocate_and_insert_proxies(s, &IfProxies, branch->If->Next, branch->Else ? branch->Else : inst);

	if (branch->Else)
		allocate_and_insert_proxies(s, &ElseProxies, branch->Else->Next, inst);

	/* Insert the CMP instructions at the end. */
	for(unsigned int index = 0; index < RC_REGISTER_MAX_INDEX; ++index) {
		if (IfProxies.Temporary[index].Proxied || ElseProxies.Temporary[index].Proxied) {
			inject_cmp(s, branch->If, inst, RC_FILE_TEMPORARY, index,
					IfProxies.Temporary[index], ElseProxies.Temporary[index]);
		}
	}

	/* Remove all traces of the branch instructions */
	rc_remove_instruction(branch->If);
	if (branch->Else)
		rc_remove_instruction(branch->Else);
	rc_remove_instruction(inst);

	s->BranchCount--;

	if (VERBOSE) {
		DBG("Program after ENDIF handling:\n");
		rc_print_program(&s->C->Program);
	}
}


struct remap_output_data {
	unsigned int Output:RC_REGISTER_INDEX_BITS;
	unsigned int Temporary:RC_REGISTER_INDEX_BITS;
};

static void remap_output_function(void * userdata, struct rc_instruction * inst,
		rc_register_file * pfile, unsigned int * pindex)
{
	struct remap_output_data * data = userdata;

	if (*pfile == RC_FILE_OUTPUT && *pindex == data->Output) {
		*pfile = RC_FILE_TEMPORARY;
		*pindex = data->Temporary;
	}
}


/**
 * Output registers cannot be read from and so cannot be dealt with like
 * temporary registers.
 *
 * We do the simplest thing: If an output registers is written within
 * a branch, then *all* writes to this register are proxied to a
 * temporary register, and a final MOV is appended to the end of
 * the program.
 */
static void fix_output_writes(struct emulate_branch_state * s, struct rc_instruction * inst)
{
	const struct rc_opcode_info * opcode;

	if (!s->BranchCount)
		return;

	opcode = rc_get_opcode_info(inst->U.I.Opcode);

	if (!opcode->HasDstReg)
		return;

	if (inst->U.I.DstReg.File == RC_FILE_OUTPUT) {
		struct remap_output_data remap;
		struct rc_instruction * inst_mov;

		remap.Output = inst->U.I.DstReg.Index;
		remap.Temporary = rc_find_free_temporary(s->C);

		for(struct rc_instruction * inst = s->C->Program.Instructions.Next;
		    inst != &s->C->Program.Instructions;
		    inst = inst->Next) {
			rc_remap_registers(inst, &remap_output_function, &remap);
		}

		inst_mov = rc_insert_new_instruction(s->C, s->C->Program.Instructions.Prev);
		inst_mov->U.I.Opcode = RC_OPCODE_MOV;
		inst_mov->U.I.DstReg.File = RC_FILE_OUTPUT;
		inst_mov->U.I.DstReg.Index = remap.Output;
		inst_mov->U.I.DstReg.WriteMask = RC_MASK_XYZW;
		inst_mov->U.I.SrcReg[0].File = RC_FILE_TEMPORARY;
		inst_mov->U.I.SrcReg[0].Index = remap.Temporary;
	}
}

/**
 * Remove branch instructions; instead, execute both branches
 * on different register sets and choose between their results
 * using CMP instructions in place of the original ENDIF.
 */
void rc_emulate_branches(struct radeon_compiler *c, void *user)
{
	struct emulate_branch_state s;
	struct rc_instruction * ptr;

	memset(&s, 0, sizeof(s));
	s.C = c;

	/* Untypical loop because we may remove the current instruction */
	ptr = c->Program.Instructions.Next;
	while(ptr != &c->Program.Instructions) {
		struct rc_instruction * inst = ptr;
		ptr = ptr->Next;

		if (inst->Type == RC_INSTRUCTION_NORMAL) {
			switch(inst->U.I.Opcode) {
			case RC_OPCODE_IF:
				handle_if(&s, inst);
				break;
			case RC_OPCODE_ELSE:
				handle_else(&s, inst);
				break;
			case RC_OPCODE_ENDIF:
				handle_endif(&s, inst);
				break;
			default:
				fix_output_writes(&s, inst);
				break;
			}
		} else {
			rc_error(c, "%s: unhandled instruction type\n", __FUNCTION__);
		}
	}
}
