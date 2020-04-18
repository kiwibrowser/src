/*
 * Copyright (C) 2009 Nicolai Haehnle.
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

#include "radeon_dataflow.h"

#include "radeon_compiler.h"
#include "radeon_compiler_util.h"
#include "radeon_list.h"
#include "radeon_swizzle.h"
#include "radeon_variable.h"

struct src_clobbered_reads_cb_data {
	rc_register_file File;
	unsigned int Index;
	unsigned int Mask;
	struct rc_reader_data * ReaderData;
};

typedef void (*rc_presub_replace_fn)(struct rc_instruction *,
						struct rc_instruction *,
						unsigned int);

static struct rc_src_register chain_srcregs(struct rc_src_register outer, struct rc_src_register inner)
{
	struct rc_src_register combine;
	combine.File = inner.File;
	combine.Index = inner.Index;
	combine.RelAddr = inner.RelAddr;
	if (outer.Abs) {
		combine.Abs = 1;
		combine.Negate = outer.Negate;
	} else {
		combine.Abs = inner.Abs;
		combine.Negate = swizzle_mask(outer.Swizzle, inner.Negate);
		combine.Negate ^= outer.Negate;
	}
	combine.Swizzle = combine_swizzles(inner.Swizzle, outer.Swizzle);
	return combine;
}

static void copy_propagate_scan_read(void * data, struct rc_instruction * inst,
						struct rc_src_register * src)
{
	rc_register_file file = src->File;
	struct rc_reader_data * reader_data = data;

	if(!rc_inst_can_use_presub(inst,
				reader_data->Writer->U.I.PreSub.Opcode,
				rc_swizzle_to_writemask(src->Swizzle),
				src,
				&reader_data->Writer->U.I.PreSub.SrcReg[0],
				&reader_data->Writer->U.I.PreSub.SrcReg[1])) {
		reader_data->Abort = 1;
		return;
	}

	/* XXX This could probably be handled better. */
	if (file == RC_FILE_ADDRESS) {
		reader_data->Abort = 1;
		return;
	}

	/* These instructions cannot read from the constants file.
	 * see radeonTransformTEX()
	 */
	if(reader_data->Writer->U.I.SrcReg[0].File != RC_FILE_TEMPORARY &&
			reader_data->Writer->U.I.SrcReg[0].File != RC_FILE_INPUT &&
				(inst->U.I.Opcode == RC_OPCODE_TEX ||
				inst->U.I.Opcode == RC_OPCODE_TXB ||
				inst->U.I.Opcode == RC_OPCODE_TXP ||
				inst->U.I.Opcode == RC_OPCODE_TXD ||
				inst->U.I.Opcode == RC_OPCODE_TXL ||
				inst->U.I.Opcode == RC_OPCODE_KIL)){
		reader_data->Abort = 1;
		return;
	}
}

static void src_clobbered_reads_cb(
	void * data,
	struct rc_instruction * inst,
	struct rc_src_register * src)
{
	struct src_clobbered_reads_cb_data * sc_data = data;

	if (src->File == sc_data->File
	    && src->Index == sc_data->Index
	    && (rc_swizzle_to_writemask(src->Swizzle) & sc_data->Mask)) {

		sc_data->ReaderData->AbortOnRead = RC_MASK_XYZW;
	}

	if (src->RelAddr && sc_data->File == RC_FILE_ADDRESS) {
		sc_data->ReaderData->AbortOnRead = RC_MASK_XYZW;
	}
}

static void is_src_clobbered_scan_write(
	void * data,
	struct rc_instruction * inst,
	rc_register_file file,
	unsigned int index,
	unsigned int mask)
{
	struct src_clobbered_reads_cb_data sc_data;
	struct rc_reader_data * reader_data = data;
	sc_data.File = file;
	sc_data.Index = index;
	sc_data.Mask = mask;
	sc_data.ReaderData = reader_data;
	rc_for_all_reads_src(reader_data->Writer,
					src_clobbered_reads_cb, &sc_data);
}

static void copy_propagate(struct radeon_compiler * c, struct rc_instruction * inst_mov)
{
	struct rc_reader_data reader_data;
	unsigned int i;

	if (inst_mov->U.I.DstReg.File != RC_FILE_TEMPORARY ||
	    inst_mov->U.I.WriteALUResult ||
	    inst_mov->U.I.SaturateMode)
		return;

	/* Get a list of all the readers of this MOV instruction. */
	reader_data.ExitOnAbort = 1;
	rc_get_readers(c, inst_mov, &reader_data,
		       copy_propagate_scan_read, NULL,
		       is_src_clobbered_scan_write);

	if (reader_data.Abort || reader_data.ReaderCount == 0)
		return;

	/* Propagate the MOV instruction. */
	for (i = 0; i < reader_data.ReaderCount; i++) {
		struct rc_instruction * inst = reader_data.Readers[i].Inst;
		*reader_data.Readers[i].U.I.Src = chain_srcregs(*reader_data.Readers[i].U.I.Src, inst_mov->U.I.SrcReg[0]);

		if (inst_mov->U.I.SrcReg[0].File == RC_FILE_PRESUB)
			inst->U.I.PreSub = inst_mov->U.I.PreSub;
	}

	/* Finally, remove the original MOV instruction */
	rc_remove_instruction(inst_mov);
}

/**
 * Check if a source register is actually always the same
 * swizzle constant.
 */
static int is_src_uniform_constant(struct rc_src_register src,
		rc_swizzle * pswz, unsigned int * pnegate)
{
	int have_used = 0;

	if (src.File != RC_FILE_NONE) {
		*pswz = 0;
		return 0;
	}

	for(unsigned int chan = 0; chan < 4; ++chan) {
		unsigned int swz = GET_SWZ(src.Swizzle, chan);
		if (swz < 4) {
			*pswz = 0;
			return 0;
		}
		if (swz == RC_SWIZZLE_UNUSED)
			continue;

		if (!have_used) {
			*pswz = swz;
			*pnegate = GET_BIT(src.Negate, chan);
			have_used = 1;
		} else {
			if (swz != *pswz || *pnegate != GET_BIT(src.Negate, chan)) {
				*pswz = 0;
				return 0;
			}
		}
	}

	return 1;
}

static void constant_folding_mad(struct rc_instruction * inst)
{
	rc_swizzle swz = 0;
	unsigned int negate= 0;

	if (is_src_uniform_constant(inst->U.I.SrcReg[2], &swz, &negate)) {
		if (swz == RC_SWIZZLE_ZERO) {
			inst->U.I.Opcode = RC_OPCODE_MUL;
			return;
		}
	}

	if (is_src_uniform_constant(inst->U.I.SrcReg[1], &swz, &negate)) {
		if (swz == RC_SWIZZLE_ONE) {
			inst->U.I.Opcode = RC_OPCODE_ADD;
			if (negate)
				inst->U.I.SrcReg[0].Negate ^= RC_MASK_XYZW;
			inst->U.I.SrcReg[1] = inst->U.I.SrcReg[2];
			return;
		} else if (swz == RC_SWIZZLE_ZERO) {
			inst->U.I.Opcode = RC_OPCODE_MOV;
			inst->U.I.SrcReg[0] = inst->U.I.SrcReg[2];
			return;
		}
	}

	if (is_src_uniform_constant(inst->U.I.SrcReg[0], &swz, &negate)) {
		if (swz == RC_SWIZZLE_ONE) {
			inst->U.I.Opcode = RC_OPCODE_ADD;
			if (negate)
				inst->U.I.SrcReg[1].Negate ^= RC_MASK_XYZW;
			inst->U.I.SrcReg[0] = inst->U.I.SrcReg[2];
			return;
		} else if (swz == RC_SWIZZLE_ZERO) {
			inst->U.I.Opcode = RC_OPCODE_MOV;
			inst->U.I.SrcReg[0] = inst->U.I.SrcReg[2];
			return;
		}
	}
}

static void constant_folding_mul(struct rc_instruction * inst)
{
	rc_swizzle swz = 0;
	unsigned int negate = 0;

	if (is_src_uniform_constant(inst->U.I.SrcReg[0], &swz, &negate)) {
		if (swz == RC_SWIZZLE_ONE) {
			inst->U.I.Opcode = RC_OPCODE_MOV;
			inst->U.I.SrcReg[0] = inst->U.I.SrcReg[1];
			if (negate)
				inst->U.I.SrcReg[0].Negate ^= RC_MASK_XYZW;
			return;
		} else if (swz == RC_SWIZZLE_ZERO) {
			inst->U.I.Opcode = RC_OPCODE_MOV;
			inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_0000;
			return;
		}
	}

	if (is_src_uniform_constant(inst->U.I.SrcReg[1], &swz, &negate)) {
		if (swz == RC_SWIZZLE_ONE) {
			inst->U.I.Opcode = RC_OPCODE_MOV;
			if (negate)
				inst->U.I.SrcReg[0].Negate ^= RC_MASK_XYZW;
			return;
		} else if (swz == RC_SWIZZLE_ZERO) {
			inst->U.I.Opcode = RC_OPCODE_MOV;
			inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_0000;
			return;
		}
	}
}

static void constant_folding_add(struct rc_instruction * inst)
{
	rc_swizzle swz = 0;
	unsigned int negate = 0;

	if (is_src_uniform_constant(inst->U.I.SrcReg[0], &swz, &negate)) {
		if (swz == RC_SWIZZLE_ZERO) {
			inst->U.I.Opcode = RC_OPCODE_MOV;
			inst->U.I.SrcReg[0] = inst->U.I.SrcReg[1];
			return;
		}
	}

	if (is_src_uniform_constant(inst->U.I.SrcReg[1], &swz, &negate)) {
		if (swz == RC_SWIZZLE_ZERO) {
			inst->U.I.Opcode = RC_OPCODE_MOV;
			return;
		}
	}
}

/**
 * Replace 0.0, 1.0 and 0.5 immediate constants by their
 * respective swizzles. Simplify instructions like ADD dst, src, 0;
 */
static void constant_folding(struct radeon_compiler * c, struct rc_instruction * inst)
{
	const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);
	unsigned int i;

	/* Replace 0.0, 1.0 and 0.5 immediates by their explicit swizzles */
	for(unsigned int src = 0; src < opcode->NumSrcRegs; ++src) {
		struct rc_constant * constant;
		struct rc_src_register newsrc;
		int have_real_reference;
		unsigned int chan;

		/* If there are only 0, 0.5, 1, or _ swizzles, mark the source as a constant. */
		for (chan = 0; chan < 4; ++chan)
			if (GET_SWZ(inst->U.I.SrcReg[src].Swizzle, chan) <= 3)
				break;
		if (chan == 4) {
			inst->U.I.SrcReg[src].File = RC_FILE_NONE;
			continue;
		}

		/* Convert immediates to swizzles. */
		if (inst->U.I.SrcReg[src].File != RC_FILE_CONSTANT ||
		    inst->U.I.SrcReg[src].RelAddr ||
		    inst->U.I.SrcReg[src].Index >= c->Program.Constants.Count)
			continue;

		constant =
			&c->Program.Constants.Constants[inst->U.I.SrcReg[src].Index];

		if (constant->Type != RC_CONSTANT_IMMEDIATE)
			continue;

		newsrc = inst->U.I.SrcReg[src];
		have_real_reference = 0;
		for (chan = 0; chan < 4; ++chan) {
			unsigned int swz = GET_SWZ(newsrc.Swizzle, chan);
			unsigned int newswz;
			float imm;
			float baseimm;

			if (swz >= 4)
				continue;

			imm = constant->u.Immediate[swz];
			baseimm = imm;
			if (imm < 0.0)
				baseimm = -baseimm;

			if (baseimm == 0.0) {
				newswz = RC_SWIZZLE_ZERO;
			} else if (baseimm == 1.0) {
				newswz = RC_SWIZZLE_ONE;
			} else if (baseimm == 0.5 && c->has_half_swizzles) {
				newswz = RC_SWIZZLE_HALF;
			} else {
				have_real_reference = 1;
				continue;
			}

			SET_SWZ(newsrc.Swizzle, chan, newswz);
			if (imm < 0.0 && !newsrc.Abs)
				newsrc.Negate ^= 1 << chan;
		}

		if (!have_real_reference) {
			newsrc.File = RC_FILE_NONE;
			newsrc.Index = 0;
		}

		/* don't make the swizzle worse */
		if (!c->SwizzleCaps->IsNative(inst->U.I.Opcode, newsrc) &&
		    c->SwizzleCaps->IsNative(inst->U.I.Opcode, inst->U.I.SrcReg[src]))
			continue;

		inst->U.I.SrcReg[src] = newsrc;
	}

	/* Simplify instructions based on constants */
	if (inst->U.I.Opcode == RC_OPCODE_MAD)
		constant_folding_mad(inst);

	/* note: MAD can simplify to MUL or ADD */
	if (inst->U.I.Opcode == RC_OPCODE_MUL)
		constant_folding_mul(inst);
	else if (inst->U.I.Opcode == RC_OPCODE_ADD)
		constant_folding_add(inst);

	/* In case this instruction has been converted, make sure all of the
	 * registers that are no longer used are empty. */
	opcode = rc_get_opcode_info(inst->U.I.Opcode);
	for(i = opcode->NumSrcRegs; i < 3; i++) {
		memset(&inst->U.I.SrcReg[i], 0, sizeof(struct rc_src_register));
	}
}

/**
 * If src and dst use the same register, this function returns a writemask that
 * indicates wich components are read by src.  Otherwise zero is returned.
 */
static unsigned int src_reads_dst_mask(struct rc_src_register src,
						struct rc_dst_register dst)
{
	if (dst.File != src.File || dst.Index != src.Index) {
		return 0;
	}
	return rc_swizzle_to_writemask(src.Swizzle);
}

/* Return 1 if the source registers has a constant swizzle (e.g. 0, 0.5, 1.0)
 * in any of its channels.  Return 0 otherwise. */
static int src_has_const_swz(struct rc_src_register src) {
	int chan;
	for(chan = 0; chan < 4; chan++) {
		unsigned int swz = GET_SWZ(src.Swizzle, chan);
		if (swz == RC_SWIZZLE_ZERO || swz == RC_SWIZZLE_HALF
						|| swz == RC_SWIZZLE_ONE) {
			return 1;
		}
	}
	return 0;
}

static void presub_scan_read(
	void * data,
	struct rc_instruction * inst,
	struct rc_src_register * src)
{
	struct rc_reader_data * reader_data = data;
	rc_presubtract_op * presub_opcode = reader_data->CbData;

	if (!rc_inst_can_use_presub(inst, *presub_opcode,
			reader_data->Writer->U.I.DstReg.WriteMask,
			src,
			&reader_data->Writer->U.I.SrcReg[0],
			&reader_data->Writer->U.I.SrcReg[1])) {
		reader_data->Abort = 1;
		return;
	}
}

static int presub_helper(
	struct radeon_compiler * c,
	struct rc_instruction * inst_add,
	rc_presubtract_op presub_opcode,
	rc_presub_replace_fn presub_replace)
{
	struct rc_reader_data reader_data;
	unsigned int i;
	rc_presubtract_op cb_op = presub_opcode;

	reader_data.CbData = &cb_op;
	reader_data.ExitOnAbort = 1;
	rc_get_readers(c, inst_add, &reader_data, presub_scan_read, NULL,
						is_src_clobbered_scan_write);

	if (reader_data.Abort || reader_data.ReaderCount == 0)
		return 0;

	for(i = 0; i < reader_data.ReaderCount; i++) {
		unsigned int src_index;
		struct rc_reader reader = reader_data.Readers[i];
		const struct rc_opcode_info * info =
				rc_get_opcode_info(reader.Inst->U.I.Opcode);

		for (src_index = 0; src_index < info->NumSrcRegs; src_index++) {
			if (&reader.Inst->U.I.SrcReg[src_index] == reader.U.I.Src)
				presub_replace(inst_add, reader.Inst, src_index);
		}
	}
	return 1;
}

/* This function assumes that inst_add->U.I.SrcReg[0] and
 * inst_add->U.I.SrcReg[1] aren't both negative. */
static void presub_replace_add(
	struct rc_instruction * inst_add,
	struct rc_instruction * inst_reader,
	unsigned int src_index)
{
	rc_presubtract_op presub_opcode;
	if (inst_add->U.I.SrcReg[1].Negate || inst_add->U.I.SrcReg[0].Negate)
		presub_opcode = RC_PRESUB_SUB;
	else
		presub_opcode = RC_PRESUB_ADD;

	if (inst_add->U.I.SrcReg[1].Negate) {
		inst_reader->U.I.PreSub.SrcReg[0] = inst_add->U.I.SrcReg[1];
		inst_reader->U.I.PreSub.SrcReg[1] = inst_add->U.I.SrcReg[0];
	} else {
		inst_reader->U.I.PreSub.SrcReg[0] = inst_add->U.I.SrcReg[0];
		inst_reader->U.I.PreSub.SrcReg[1] = inst_add->U.I.SrcReg[1];
	}
	inst_reader->U.I.PreSub.SrcReg[0].Negate = 0;
	inst_reader->U.I.PreSub.SrcReg[1].Negate = 0;
	inst_reader->U.I.PreSub.Opcode = presub_opcode;
	inst_reader->U.I.SrcReg[src_index] =
			chain_srcregs(inst_reader->U.I.SrcReg[src_index],
					inst_reader->U.I.PreSub.SrcReg[0]);
	inst_reader->U.I.SrcReg[src_index].File = RC_FILE_PRESUB;
	inst_reader->U.I.SrcReg[src_index].Index = presub_opcode;
}

static int is_presub_candidate(
	struct radeon_compiler * c,
	struct rc_instruction * inst)
{
	const struct rc_opcode_info * info = rc_get_opcode_info(inst->U.I.Opcode);
	unsigned int i;
	unsigned int is_constant[2] = {0, 0};

	assert(inst->U.I.Opcode == RC_OPCODE_ADD);

	if (inst->U.I.PreSub.Opcode != RC_PRESUB_NONE
			|| inst->U.I.SaturateMode
			|| inst->U.I.WriteALUResult
			|| inst->U.I.Omod) {
		return 0;
	}

	/* If both sources use a constant swizzle, then we can't convert it to
	 * a presubtract operation.  In fact for the ADD and SUB presubtract
	 * operations neither source can contain a constant swizzle.  This
	 * specific case is checked in peephole_add_presub_add() when
	 * we make sure the swizzles for both sources are equal, so we
	 * don't need to worry about it here. */
	for (i = 0; i < 2; i++) {
		int chan;
		for (chan = 0; chan < 4; chan++) {
			rc_swizzle swz =
				get_swz(inst->U.I.SrcReg[i].Swizzle, chan);
			if (swz == RC_SWIZZLE_ONE
					|| swz == RC_SWIZZLE_ZERO
					|| swz == RC_SWIZZLE_HALF) {
				is_constant[i] = 1;
			}
		}
	}
	if (is_constant[0] && is_constant[1])
		return 0;

	for(i = 0; i < info->NumSrcRegs; i++) {
		struct rc_src_register src = inst->U.I.SrcReg[i];
		if (src_reads_dst_mask(src, inst->U.I.DstReg))
			return 0;

		src.File = RC_FILE_PRESUB;
		if (!c->SwizzleCaps->IsNative(inst->U.I.Opcode, src))
			return 0;
	}
	return 1;
}

static int peephole_add_presub_add(
	struct radeon_compiler * c,
	struct rc_instruction * inst_add)
{
	unsigned dstmask = inst_add->U.I.DstReg.WriteMask;
        unsigned src0_neg = inst_add->U.I.SrcReg[0].Negate & dstmask;
        unsigned src1_neg = inst_add->U.I.SrcReg[1].Negate & dstmask;

	if (inst_add->U.I.SrcReg[0].Swizzle != inst_add->U.I.SrcReg[1].Swizzle)
		return 0;

	/* src0 and src1 can't have absolute values */
	if (inst_add->U.I.SrcReg[0].Abs || inst_add->U.I.SrcReg[1].Abs)
	        return 0;

	/* presub_replace_add() assumes only one is negative */
	if (inst_add->U.I.SrcReg[0].Negate && inst_add->U.I.SrcReg[1].Negate)
	        return 0;

        /* if src0 is negative, at least all bits of dstmask have to be set */
        if (inst_add->U.I.SrcReg[0].Negate && src0_neg != dstmask)
	        return 0;

        /* if src1 is negative, at least all bits of dstmask have to be set */
        if (inst_add->U.I.SrcReg[1].Negate && src1_neg != dstmask)
	        return 0;

	if (!is_presub_candidate(c, inst_add))
		return 0;

	if (presub_helper(c, inst_add, RC_PRESUB_ADD, presub_replace_add)) {
		rc_remove_instruction(inst_add);
		return 1;
	}
	return 0;
}

static void presub_replace_inv(
	struct rc_instruction * inst_add,
	struct rc_instruction * inst_reader,
	unsigned int src_index)
{
	/* We must be careful not to modify inst_add, since it
	 * is possible it will remain part of the program.*/
	inst_reader->U.I.PreSub.SrcReg[0] = inst_add->U.I.SrcReg[1];
	inst_reader->U.I.PreSub.SrcReg[0].Negate = 0;
	inst_reader->U.I.PreSub.Opcode = RC_PRESUB_INV;
	inst_reader->U.I.SrcReg[src_index] = chain_srcregs(inst_reader->U.I.SrcReg[src_index],
						inst_reader->U.I.PreSub.SrcReg[0]);

	inst_reader->U.I.SrcReg[src_index].File = RC_FILE_PRESUB;
	inst_reader->U.I.SrcReg[src_index].Index = RC_PRESUB_INV;
}

/**
 * PRESUB_INV: ADD TEMP[0], none.1, -TEMP[1]
 * Use the presubtract 1 - src0 for all readers of TEMP[0].  The first source
 * of the add instruction must have the constatnt 1 swizzle.  This function
 * does not check const registers to see if their value is 1.0, so it should
 * be called after the constant_folding optimization.
 * @return
 * 	0 if the ADD instruction is still part of the program.
 * 	1 if the ADD instruction is no longer part of the program.
 */
static int peephole_add_presub_inv(
	struct radeon_compiler * c,
	struct rc_instruction * inst_add)
{
	unsigned int i, swz;

	if (!is_presub_candidate(c, inst_add))
		return 0;

	/* Check if src0 is 1. */
	/* XXX It would be nice to use is_src_uniform_constant here, but that
	 * function only works if the register's file is RC_FILE_NONE */
	for(i = 0; i < 4; i++ ) {
		swz = GET_SWZ(inst_add->U.I.SrcReg[0].Swizzle, i);
		if(((1 << i) & inst_add->U.I.DstReg.WriteMask)
						&& swz != RC_SWIZZLE_ONE) {
			return 0;
		}
	}

	/* Check src1. */
	if ((inst_add->U.I.SrcReg[1].Negate & inst_add->U.I.DstReg.WriteMask) !=
						inst_add->U.I.DstReg.WriteMask
		|| inst_add->U.I.SrcReg[1].Abs
		|| (inst_add->U.I.SrcReg[1].File != RC_FILE_TEMPORARY
			&& inst_add->U.I.SrcReg[1].File != RC_FILE_CONSTANT)
		|| src_has_const_swz(inst_add->U.I.SrcReg[1])) {

		return 0;
	}

	if (presub_helper(c, inst_add, RC_PRESUB_INV, presub_replace_inv)) {
		rc_remove_instruction(inst_add);
		return 1;
	}
	return 0;
}

struct peephole_mul_cb_data {
	struct rc_dst_register * Writer;
	unsigned int Clobbered;
};

static void omod_filter_reader_cb(
	void * userdata,
	struct rc_instruction * inst,
	rc_register_file file,
	unsigned int index,
	unsigned int mask)
{
	struct peephole_mul_cb_data * d = userdata;
	if (rc_src_reads_dst_mask(file, mask, index,
		d->Writer->File, d->Writer->Index, d->Writer->WriteMask)) {

		d->Clobbered = 1;
	}
}

static void omod_filter_writer_cb(
	void * userdata,
	struct rc_instruction * inst,
	rc_register_file file,
	unsigned int index,
	unsigned int mask)
{
	struct peephole_mul_cb_data * d = userdata;
	if (file == d->Writer->File && index == d->Writer->Index &&
					(mask & d->Writer->WriteMask)) {
		d->Clobbered = 1;
	}
}

static int peephole_mul_omod(
	struct radeon_compiler * c,
	struct rc_instruction * inst_mul,
	struct rc_list * var_list)
{
	unsigned int chan = 0, swz, i;
	int const_index = -1;
	int temp_index = -1;
	float const_value;
	rc_omod_op omod_op = RC_OMOD_DISABLE;
	struct rc_list * writer_list;
	struct rc_variable * var;
	struct peephole_mul_cb_data cb_data;

	for (i = 0; i < 2; i++) {
		unsigned int j;
		if (inst_mul->U.I.SrcReg[i].File != RC_FILE_CONSTANT
			&& inst_mul->U.I.SrcReg[i].File != RC_FILE_TEMPORARY) {
			return 0;
		}
		if (inst_mul->U.I.SrcReg[i].File == RC_FILE_TEMPORARY) {
			if (temp_index != -1) {
				/* The instruction has two temp sources */
				return 0;
			} else {
				temp_index = i;
				continue;
			}
		}
		/* If we get this far Src[i] must be a constant src */
		if (inst_mul->U.I.SrcReg[i].Negate) {
			return 0;
		}
		/* The constant src needs to read from the same swizzle */
		swz = RC_SWIZZLE_UNUSED;
		chan = 0;
		for (j = 0; j < 4; j++) {
			unsigned int j_swz =
				GET_SWZ(inst_mul->U.I.SrcReg[i].Swizzle, j);
			if (j_swz == RC_SWIZZLE_UNUSED) {
				continue;
			}
			if (swz == RC_SWIZZLE_UNUSED) {
				swz = j_swz;
				chan = j;
			} else if (j_swz != swz) {
				return 0;
			}
		}

		if (const_index != -1) {
			/* The instruction has two constant sources */
			return 0;
		} else {
			const_index = i;
		}
	}

	if (!rc_src_reg_is_immediate(c, inst_mul->U.I.SrcReg[const_index].File,
				inst_mul->U.I.SrcReg[const_index].Index)) {
		return 0;
	}
	const_value = rc_get_constant_value(c,
			inst_mul->U.I.SrcReg[const_index].Index,
			inst_mul->U.I.SrcReg[const_index].Swizzle,
			inst_mul->U.I.SrcReg[const_index].Negate,
			chan);

	if (const_value == 2.0f) {
		omod_op = RC_OMOD_MUL_2;
	} else if (const_value == 4.0f) {
		omod_op = RC_OMOD_MUL_4;
	} else if (const_value == 8.0f) {
		omod_op = RC_OMOD_MUL_8;
	} else if (const_value == (1.0f / 2.0f)) {
		omod_op = RC_OMOD_DIV_2;
	} else if (const_value == (1.0f / 4.0f)) {
		omod_op = RC_OMOD_DIV_4;
	} else if (const_value == (1.0f / 8.0f)) {
		omod_op = RC_OMOD_DIV_8;
	} else {
		return 0;
	}

	writer_list = rc_variable_list_get_writers_one_reader(var_list,
		RC_INSTRUCTION_NORMAL, &inst_mul->U.I.SrcReg[temp_index]);

	if (!writer_list) {
		return 0;
	}

	cb_data.Clobbered = 0;
	cb_data.Writer = &inst_mul->U.I.DstReg;
	for (var = writer_list->Item; var; var = var->Friend) {
		struct rc_instruction * inst;
		const struct rc_opcode_info * info = rc_get_opcode_info(
				var->Inst->U.I.Opcode);
		if (info->HasTexture) {
			return 0;
		}
		if (var->Inst->U.I.SaturateMode != RC_SATURATE_NONE) {
			return 0;
		}
		for (inst = inst_mul->Prev; inst != var->Inst;
							inst = inst->Prev) {
			rc_for_all_reads_mask(inst, omod_filter_reader_cb,
								&cb_data);
			rc_for_all_writes_mask(inst, omod_filter_writer_cb,
								&cb_data);
			if (cb_data.Clobbered) {
				break;
			}
		}
	}

	if (cb_data.Clobbered) {
		return 0;
	}

	/* Rewrite the instructions */
	for (var = writer_list->Item; var; var = var->Friend) {
		struct rc_variable * writer = writer_list->Item;
		unsigned conversion_swizzle = rc_make_conversion_swizzle(
					writer->Inst->U.I.DstReg.WriteMask,
					inst_mul->U.I.DstReg.WriteMask);
		writer->Inst->U.I.Omod = omod_op;
		writer->Inst->U.I.DstReg.File = inst_mul->U.I.DstReg.File;
		writer->Inst->U.I.DstReg.Index = inst_mul->U.I.DstReg.Index;
		rc_normal_rewrite_writemask(writer->Inst, conversion_swizzle);
		writer->Inst->U.I.SaturateMode = inst_mul->U.I.SaturateMode;
	}

	rc_remove_instruction(inst_mul);

	return 1;
}

/**
 * @return
 * 	0 if inst is still part of the program.
 * 	1 if inst is no longer part of the program.
 */
static int peephole(struct radeon_compiler * c, struct rc_instruction * inst)
{
	switch(inst->U.I.Opcode){
	case RC_OPCODE_ADD:
		if (c->has_presub) {
			if(peephole_add_presub_inv(c, inst))
				return 1;
			if(peephole_add_presub_add(c, inst))
				return 1;
		}
		break;
	default:
		break;
	}
	return 0;
}

void rc_optimize(struct radeon_compiler * c, void *user)
{
	struct rc_instruction * inst = c->Program.Instructions.Next;
	struct rc_list * var_list;
	while(inst != &c->Program.Instructions) {
		struct rc_instruction * cur = inst;
		inst = inst->Next;

		constant_folding(c, cur);

		if(peephole(c, cur))
			continue;

		if (cur->U.I.Opcode == RC_OPCODE_MOV) {
			copy_propagate(c, cur);
			/* cur may no longer be part of the program */
		}
	}

	if (!c->has_omod) {
		return;
	}

	inst = c->Program.Instructions.Next;
	while(inst != &c->Program.Instructions) {
		struct rc_instruction * cur = inst;
		inst = inst->Next;
		if (cur->U.I.Opcode == RC_OPCODE_MUL) {
			var_list = rc_get_variables(c);
			peephole_mul_omod(c, cur, var_list);
		}
	}
}
