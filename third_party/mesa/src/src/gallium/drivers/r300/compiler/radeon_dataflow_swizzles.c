/*
 * Copyright (C) 2009 Nicolai Haehnle.
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

#include "radeon_dataflow.h"

#include "radeon_compiler.h"
#include "radeon_swizzle.h"


static void rewrite_source(struct radeon_compiler * c,
		struct rc_instruction * inst, unsigned src)
{
	struct rc_swizzle_split split;
	unsigned int tempreg = rc_find_free_temporary(c);
	unsigned int usemask;

	usemask = 0;
	for(unsigned int chan = 0; chan < 4; ++chan) {
		if (GET_SWZ(inst->U.I.SrcReg[src].Swizzle, chan) != RC_SWIZZLE_UNUSED)
			usemask |= 1 << chan;
	}

	c->SwizzleCaps->Split(inst->U.I.SrcReg[src], usemask, &split);

	for(unsigned int phase = 0; phase < split.NumPhases; ++phase) {
		struct rc_instruction * mov = rc_insert_new_instruction(c, inst->Prev);
		unsigned int phase_refmask;
		unsigned int masked_negate;

		mov->U.I.Opcode = RC_OPCODE_MOV;
		mov->U.I.DstReg.File = RC_FILE_TEMPORARY;
		mov->U.I.DstReg.Index = tempreg;
		mov->U.I.DstReg.WriteMask = split.Phase[phase];
		mov->U.I.SrcReg[0] = inst->U.I.SrcReg[src];
		mov->U.I.PreSub = inst->U.I.PreSub;

		phase_refmask = 0;
		for(unsigned int chan = 0; chan < 4; ++chan) {
			if (!GET_BIT(split.Phase[phase], chan))
				SET_SWZ(mov->U.I.SrcReg[0].Swizzle, chan, RC_SWIZZLE_UNUSED);
			else
				phase_refmask |= 1 << GET_SWZ(mov->U.I.SrcReg[0].Swizzle, chan);
		}

		phase_refmask &= RC_MASK_XYZW;

		masked_negate = split.Phase[phase] & mov->U.I.SrcReg[0].Negate;
		if (masked_negate == 0)
			mov->U.I.SrcReg[0].Negate = 0;
		else if (masked_negate == split.Phase[phase])
			mov->U.I.SrcReg[0].Negate = RC_MASK_XYZW;

	}

	inst->U.I.SrcReg[src].File = RC_FILE_TEMPORARY;
	inst->U.I.SrcReg[src].Index = tempreg;
	inst->U.I.SrcReg[src].Swizzle = 0;
	inst->U.I.SrcReg[src].Negate = RC_MASK_NONE;
	inst->U.I.SrcReg[src].Abs = 0;
	for(unsigned int chan = 0; chan < 4; ++chan) {
		SET_SWZ(inst->U.I.SrcReg[src].Swizzle, chan,
				GET_BIT(usemask, chan) ? chan : RC_SWIZZLE_UNUSED);
	}
}

void rc_dataflow_swizzles(struct radeon_compiler * c, void *user)
{
	struct rc_instruction * inst;

	for(inst = c->Program.Instructions.Next; inst != &c->Program.Instructions; inst = inst->Next) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);
		unsigned int src;

		for(src = 0; src < opcode->NumSrcRegs; ++src) {
			if (!c->SwizzleCaps->IsNative(inst->U.I.Opcode, inst->U.I.SrcReg[src]))
				rewrite_source(c, inst, src);
		}
	}
}
