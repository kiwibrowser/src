/*
 * Copyright (C) 2008 Nicolai Haehnle.
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

#include "radeon_program.h"

#include <stdio.h>

#include "radeon_compiler.h"
#include "radeon_dataflow.h"


/**
 * Transform the given clause in the following way:
 *  1. Replace it with an empty clause
 *  2. For every instruction in the original clause, try the given
 *     transformations in order.
 *  3. If one of the transformations returns GL_TRUE, assume that it
 *     has emitted the appropriate instruction(s) into the new clause;
 *     otherwise, copy the instruction verbatim.
 *
 * \note The transformation is currently not recursive; in other words,
 * instructions emitted by transformations are not transformed.
 *
 * \note The transform is called 'local' because it can only look at
 * one instruction at a time.
 */
void rc_local_transform(
	struct radeon_compiler * c,
	void *user)
{
	struct radeon_program_transformation *transformations =
		(struct radeon_program_transformation*)user;
	struct rc_instruction * inst = c->Program.Instructions.Next;

	while(inst != &c->Program.Instructions) {
		struct rc_instruction * current = inst;
		int i;

		inst = inst->Next;

		for(i = 0; transformations[i].function; ++i) {
			struct radeon_program_transformation* t = transformations + i;

			if (t->function(c, current, t->userData))
				break;
		}
	}
}

struct get_used_temporaries_data {
	unsigned char * Used;
	unsigned int UsedLength;
};

static void get_used_temporaries_cb(
	void * userdata,
	struct rc_instruction * inst,
	rc_register_file file,
	unsigned int index,
	unsigned int mask)
{
	struct get_used_temporaries_data * d = userdata;

	if (file != RC_FILE_TEMPORARY)
		return;

	if (index >= d->UsedLength)
		return;

	d->Used[index] |= mask;
}

/**
 * This function fills in the parameter 'used' with a writemask that
 * represent which components of each temporary register are used by the
 * program.  This is meant to be combined with rc_find_free_temporary_list as a
 * more efficient version of rc_find_free_temporary.
 * @param used The function does not initialize this parameter.
 */
void rc_get_used_temporaries(
	struct radeon_compiler * c,
	unsigned char * used,
	unsigned int used_length)
{
	struct rc_instruction * inst;
	struct get_used_temporaries_data d;
	d.Used = used;
	d.UsedLength = used_length;

	for(inst = c->Program.Instructions.Next;
			inst != &c->Program.Instructions; inst = inst->Next) {

		rc_for_all_reads_mask(inst, get_used_temporaries_cb, &d);
		rc_for_all_writes_mask(inst, get_used_temporaries_cb, &d);
	}
}

/* Search a list of used temporaries for a free one
 * \sa rc_get_used_temporaries
 * @note If this functions finds a free temporary, it will mark it as used
 * in the used temporary list (param 'used')
 * @param used list of used temporaries
 * @param used_length number of items in param 'used'
 * @param mask which components must be free in the temporary index that is
 * returned.
 * @return -1 If there are no more free temporaries, otherwise the index of
 * a temporary register where the components specified in param 'mask' are
 * not being used.
 */
int rc_find_free_temporary_list(
	struct radeon_compiler * c,
	unsigned char * used,
	unsigned int used_length,
	unsigned int mask)
{
	int i;
	for(i = 0; i < used_length; i++) {
		if ((~used[i] & mask) == mask) {
			used[i] |= mask;
			return i;
		}
	}
	return -1;
}

unsigned int rc_find_free_temporary(struct radeon_compiler * c)
{
	unsigned char used[RC_REGISTER_MAX_INDEX];
	int free;

	memset(used, 0, sizeof(used));

	rc_get_used_temporaries(c, used, RC_REGISTER_MAX_INDEX);

	free = rc_find_free_temporary_list(c, used, RC_REGISTER_MAX_INDEX,
								RC_MASK_XYZW);
	if (free < 0) {
		rc_error(c, "Ran out of temporary registers\n");
		return 0;
	}
	return free;
}


struct rc_instruction *rc_alloc_instruction(struct radeon_compiler * c)
{
	struct rc_instruction * inst = memory_pool_malloc(&c->Pool, sizeof(struct rc_instruction));

	memset(inst, 0, sizeof(struct rc_instruction));

	inst->U.I.Opcode = RC_OPCODE_ILLEGAL_OPCODE;
	inst->U.I.DstReg.WriteMask = RC_MASK_XYZW;
	inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_XYZW;
	inst->U.I.SrcReg[1].Swizzle = RC_SWIZZLE_XYZW;
	inst->U.I.SrcReg[2].Swizzle = RC_SWIZZLE_XYZW;

	return inst;
}

void rc_insert_instruction(struct rc_instruction * after, struct rc_instruction * inst)
{
	inst->Prev = after;
	inst->Next = after->Next;

	inst->Prev->Next = inst;
	inst->Next->Prev = inst;
}

struct rc_instruction *rc_insert_new_instruction(struct radeon_compiler * c, struct rc_instruction * after)
{
	struct rc_instruction * inst = rc_alloc_instruction(c);

	rc_insert_instruction(after, inst);

	return inst;
}

void rc_remove_instruction(struct rc_instruction * inst)
{
	inst->Prev->Next = inst->Next;
	inst->Next->Prev = inst->Prev;
}

/**
 * Return the number of instructions in the program.
 */
unsigned int rc_recompute_ips(struct radeon_compiler * c)
{
	unsigned int ip = 0;
	struct rc_instruction * inst;

	for(inst = c->Program.Instructions.Next;
	    inst != &c->Program.Instructions;
	    inst = inst->Next) {
		inst->IP = ip++;
	}

	c->Program.Instructions.IP = 0xcafedead;

	return ip;
}
