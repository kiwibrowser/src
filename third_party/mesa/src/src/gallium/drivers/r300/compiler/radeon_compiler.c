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

#include "radeon_compiler.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "radeon_dataflow.h"
#include "radeon_program.h"
#include "radeon_program_pair.h"
#include "radeon_compiler_util.h"


void rc_init(struct radeon_compiler * c)
{
	memset(c, 0, sizeof(*c));

	memory_pool_init(&c->Pool);
	c->Program.Instructions.Prev = &c->Program.Instructions;
	c->Program.Instructions.Next = &c->Program.Instructions;
	c->Program.Instructions.U.I.Opcode = RC_OPCODE_ILLEGAL_OPCODE;
}

void rc_destroy(struct radeon_compiler * c)
{
	rc_constants_destroy(&c->Program.Constants);
	memory_pool_destroy(&c->Pool);
	free(c->ErrorMsg);
}

void rc_debug(struct radeon_compiler * c, const char * fmt, ...)
{
	va_list ap;

	if (!(c->Debug & RC_DBG_LOG))
		return;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void rc_error(struct radeon_compiler * c, const char * fmt, ...)
{
	va_list ap;

	c->Error = 1;

	if (!c->ErrorMsg) {
		/* Only remember the first error */
		char buf[1024];
		int written;

		va_start(ap, fmt);
		written = vsnprintf(buf, sizeof(buf), fmt, ap);
		va_end(ap);

		if (written < sizeof(buf)) {
			c->ErrorMsg = strdup(buf);
		} else {
			c->ErrorMsg = malloc(written + 1);

			va_start(ap, fmt);
			vsnprintf(c->ErrorMsg, written + 1, fmt, ap);
			va_end(ap);
		}
	}

	if (c->Debug & RC_DBG_LOG) {
		fprintf(stderr, "r300compiler error: ");

		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}

int rc_if_fail_helper(struct radeon_compiler * c, const char * file, int line, const char * assertion)
{
	rc_error(c, "ICE at %s:%i: assertion failed: %s\n", file, line, assertion);
	return 1;
}

/**
 * Recompute c->Program.InputsRead and c->Program.OutputsWritten
 * based on which inputs and outputs are actually referenced
 * in program instructions.
 */
void rc_calculate_inputs_outputs(struct radeon_compiler * c)
{
	struct rc_instruction *inst;

	c->Program.InputsRead = 0;
	c->Program.OutputsWritten = 0;

	for(inst = c->Program.Instructions.Next; inst != &c->Program.Instructions; inst = inst->Next)
	{
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);
		int i;

		for (i = 0; i < opcode->NumSrcRegs; ++i) {
			if (inst->U.I.SrcReg[i].File == RC_FILE_INPUT)
				c->Program.InputsRead |= 1 << inst->U.I.SrcReg[i].Index;
		}

		if (opcode->HasDstReg) {
			if (inst->U.I.DstReg.File == RC_FILE_OUTPUT)
				c->Program.OutputsWritten |= 1 << inst->U.I.DstReg.Index;
		}
	}
}

/**
 * Rewrite the program such that everything that source the given input
 * register will source new_input instead.
 */
void rc_move_input(struct radeon_compiler * c, unsigned input, struct rc_src_register new_input)
{
	struct rc_instruction * inst;

	c->Program.InputsRead &= ~(1 << input);

	for(inst = c->Program.Instructions.Next; inst != &c->Program.Instructions; inst = inst->Next) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);
		unsigned i;

		for(i = 0; i < opcode->NumSrcRegs; ++i) {
			if (inst->U.I.SrcReg[i].File == RC_FILE_INPUT && inst->U.I.SrcReg[i].Index == input) {
				inst->U.I.SrcReg[i].File = new_input.File;
				inst->U.I.SrcReg[i].Index = new_input.Index;
				inst->U.I.SrcReg[i].Swizzle = combine_swizzles(new_input.Swizzle, inst->U.I.SrcReg[i].Swizzle);
				if (!inst->U.I.SrcReg[i].Abs) {
					inst->U.I.SrcReg[i].Negate ^= new_input.Negate;
					inst->U.I.SrcReg[i].Abs = new_input.Abs;
				}

				c->Program.InputsRead |= 1 << new_input.Index;
			}
		}
	}
}


/**
 * Rewrite the program such that everything that writes into the given
 * output register will instead write to new_output. The new_output
 * writemask is honoured.
 */
void rc_move_output(struct radeon_compiler * c, unsigned output, unsigned new_output, unsigned writemask)
{
	struct rc_instruction * inst;

	c->Program.OutputsWritten &= ~(1 << output);

	for(inst = c->Program.Instructions.Next; inst != &c->Program.Instructions; inst = inst->Next) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);

		if (opcode->HasDstReg) {
			if (inst->U.I.DstReg.File == RC_FILE_OUTPUT && inst->U.I.DstReg.Index == output) {
				inst->U.I.DstReg.Index = new_output;
				inst->U.I.DstReg.WriteMask &= writemask;

				c->Program.OutputsWritten |= 1 << new_output;
			}
		}
	}
}


/**
 * Rewrite the program such that a given output is duplicated.
 */
void rc_copy_output(struct radeon_compiler * c, unsigned output, unsigned dup_output)
{
	unsigned tempreg = rc_find_free_temporary(c);
	struct rc_instruction * inst;

	for(inst = c->Program.Instructions.Next; inst != &c->Program.Instructions; inst = inst->Next) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);

		if (opcode->HasDstReg) {
			if (inst->U.I.DstReg.File == RC_FILE_OUTPUT && inst->U.I.DstReg.Index == output) {
				inst->U.I.DstReg.File = RC_FILE_TEMPORARY;
				inst->U.I.DstReg.Index = tempreg;
			}
		}
	}

	inst = rc_insert_new_instruction(c, c->Program.Instructions.Prev);
	inst->U.I.Opcode = RC_OPCODE_MOV;
	inst->U.I.DstReg.File = RC_FILE_OUTPUT;
	inst->U.I.DstReg.Index = output;

	inst->U.I.SrcReg[0].File = RC_FILE_TEMPORARY;
	inst->U.I.SrcReg[0].Index = tempreg;
	inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_XYZW;

	inst = rc_insert_new_instruction(c, c->Program.Instructions.Prev);
	inst->U.I.Opcode = RC_OPCODE_MOV;
	inst->U.I.DstReg.File = RC_FILE_OUTPUT;
	inst->U.I.DstReg.Index = dup_output;

	inst->U.I.SrcReg[0].File = RC_FILE_TEMPORARY;
	inst->U.I.SrcReg[0].Index = tempreg;
	inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_XYZW;

	c->Program.OutputsWritten |= 1 << dup_output;
}


/**
 * Introduce standard code fragment to deal with fragment.position.
 */
void rc_transform_fragment_wpos(struct radeon_compiler * c, unsigned wpos, unsigned new_input,
                                int full_vtransform)
{
	unsigned tempregi = rc_find_free_temporary(c);
	struct rc_instruction * inst_rcp;
	struct rc_instruction * inst_mul;
	struct rc_instruction * inst_mad;
	struct rc_instruction * inst;

	c->Program.InputsRead &= ~(1 << wpos);
	c->Program.InputsRead |= 1 << new_input;

	/* perspective divide */
	inst_rcp = rc_insert_new_instruction(c, &c->Program.Instructions);
	inst_rcp->U.I.Opcode = RC_OPCODE_RCP;

	inst_rcp->U.I.DstReg.File = RC_FILE_TEMPORARY;
	inst_rcp->U.I.DstReg.Index = tempregi;
	inst_rcp->U.I.DstReg.WriteMask = RC_MASK_W;

	inst_rcp->U.I.SrcReg[0].File = RC_FILE_INPUT;
	inst_rcp->U.I.SrcReg[0].Index = new_input;
	inst_rcp->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_WWWW;

	inst_mul = rc_insert_new_instruction(c, inst_rcp);
	inst_mul->U.I.Opcode = RC_OPCODE_MUL;

	inst_mul->U.I.DstReg.File = RC_FILE_TEMPORARY;
	inst_mul->U.I.DstReg.Index = tempregi;
	inst_mul->U.I.DstReg.WriteMask = RC_MASK_XYZ;

	inst_mul->U.I.SrcReg[0].File = RC_FILE_INPUT;
	inst_mul->U.I.SrcReg[0].Index = new_input;

	inst_mul->U.I.SrcReg[1].File = RC_FILE_TEMPORARY;
	inst_mul->U.I.SrcReg[1].Index = tempregi;
	inst_mul->U.I.SrcReg[1].Swizzle = RC_SWIZZLE_WWWW;

	/* viewport transformation */
	inst_mad = rc_insert_new_instruction(c, inst_mul);
	inst_mad->U.I.Opcode = RC_OPCODE_MAD;

	inst_mad->U.I.DstReg.File = RC_FILE_TEMPORARY;
	inst_mad->U.I.DstReg.Index = tempregi;
	inst_mad->U.I.DstReg.WriteMask = RC_MASK_XYZ;

	inst_mad->U.I.SrcReg[0].File = RC_FILE_TEMPORARY;
	inst_mad->U.I.SrcReg[0].Index = tempregi;
	inst_mad->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_XYZ0;

	inst_mad->U.I.SrcReg[1].File = RC_FILE_CONSTANT;
	inst_mad->U.I.SrcReg[1].Swizzle = RC_SWIZZLE_XYZ0;

	inst_mad->U.I.SrcReg[2].File = RC_FILE_CONSTANT;
	inst_mad->U.I.SrcReg[2].Swizzle = RC_SWIZZLE_XYZ0;

	if (full_vtransform) {
		inst_mad->U.I.SrcReg[1].Index = rc_constants_add_state(&c->Program.Constants, RC_STATE_R300_VIEWPORT_SCALE, 0);
		inst_mad->U.I.SrcReg[2].Index = rc_constants_add_state(&c->Program.Constants, RC_STATE_R300_VIEWPORT_OFFSET, 0);
	} else {
		inst_mad->U.I.SrcReg[1].Index =
		inst_mad->U.I.SrcReg[2].Index = rc_constants_add_state(&c->Program.Constants, RC_STATE_R300_WINDOW_DIMENSION, 0);
	}

	for (inst = inst_mad->Next; inst != &c->Program.Instructions; inst = inst->Next) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);
		unsigned i;

		for(i = 0; i < opcode->NumSrcRegs; i++) {
			if (inst->U.I.SrcReg[i].File == RC_FILE_INPUT &&
			    inst->U.I.SrcReg[i].Index == wpos) {
				inst->U.I.SrcReg[i].File = RC_FILE_TEMPORARY;
				inst->U.I.SrcReg[i].Index = tempregi;
			}
		}
	}
}


/**
 * The FACE input in hardware contains 1 if it's a back face, 0 otherwise.
 * Gallium and OpenGL define it the other way around.
 *
 * So let's just negate FACE at the beginning of the shader and rewrite the rest
 * of the shader to read from the newly allocated temporary.
 */
void rc_transform_fragment_face(struct radeon_compiler *c, unsigned face)
{
	unsigned tempregi = rc_find_free_temporary(c);
	struct rc_instruction *inst_add;
	struct rc_instruction *inst;

	/* perspective divide */
	inst_add = rc_insert_new_instruction(c, &c->Program.Instructions);
	inst_add->U.I.Opcode = RC_OPCODE_ADD;

	inst_add->U.I.DstReg.File = RC_FILE_TEMPORARY;
	inst_add->U.I.DstReg.Index = tempregi;
	inst_add->U.I.DstReg.WriteMask = RC_MASK_X;

	inst_add->U.I.SrcReg[0].File = RC_FILE_NONE;
	inst_add->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_1111;

	inst_add->U.I.SrcReg[1].File = RC_FILE_INPUT;
	inst_add->U.I.SrcReg[1].Index = face;
	inst_add->U.I.SrcReg[1].Swizzle = RC_SWIZZLE_XXXX;
	inst_add->U.I.SrcReg[1].Negate = RC_MASK_XYZW;

	for (inst = inst_add->Next; inst != &c->Program.Instructions; inst = inst->Next) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);
		unsigned i;

		for(i = 0; i < opcode->NumSrcRegs; i++) {
			if (inst->U.I.SrcReg[i].File == RC_FILE_INPUT &&
			    inst->U.I.SrcReg[i].Index == face) {
				inst->U.I.SrcReg[i].File = RC_FILE_TEMPORARY;
				inst->U.I.SrcReg[i].Index = tempregi;
			}
		}
	}
}

static void reg_count_callback(void * userdata, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int mask)
{
	struct rc_program_stats *s = userdata;
	if (file == RC_FILE_TEMPORARY)
		(int)index > s->num_temp_regs ? s->num_temp_regs = index : 0;
	if (file == RC_FILE_INLINE)
		s->num_inline_literals++;
}

void rc_get_stats(struct radeon_compiler *c, struct rc_program_stats *s)
{
	struct rc_instruction * tmp;
	memset(s, 0, sizeof(*s));

	for(tmp = c->Program.Instructions.Next; tmp != &c->Program.Instructions;
							tmp = tmp->Next){
		const struct rc_opcode_info * info;
		rc_for_all_reads_mask(tmp, reg_count_callback, s);
		if (tmp->Type == RC_INSTRUCTION_NORMAL) {
			info = rc_get_opcode_info(tmp->U.I.Opcode);
			if (info->Opcode == RC_OPCODE_BEGIN_TEX)
				continue;
			if (tmp->U.I.PreSub.Opcode != RC_PRESUB_NONE)
				s->num_presub_ops++;
		} else {
			if (tmp->U.P.RGB.Src[RC_PAIR_PRESUB_SRC].Used)
				s->num_presub_ops++;
			if (tmp->U.P.Alpha.Src[RC_PAIR_PRESUB_SRC].Used)
				s->num_presub_ops++;
			/* Assuming alpha will never be a flow control or
			 * a tex instruction. */
			if (tmp->U.P.Alpha.Opcode != RC_OPCODE_NOP)
				s->num_alpha_insts++;
			if (tmp->U.P.RGB.Opcode != RC_OPCODE_NOP)
				s->num_rgb_insts++;
			if (tmp->U.P.RGB.Omod != RC_OMOD_MUL_1 &&
				tmp->U.P.RGB.Omod != RC_OMOD_DISABLE) {
				s->num_omod_ops++;
			}
			if (tmp->U.P.Alpha.Omod != RC_OMOD_MUL_1 &&
				tmp->U.P.Alpha.Omod != RC_OMOD_DISABLE) {
				s->num_omod_ops++;
			}
			info = rc_get_opcode_info(tmp->U.P.RGB.Opcode);
		}
		if (info->IsFlowControl)
			s->num_fc_insts++;
		if (info->HasTexture)
			s->num_tex_insts++;
		s->num_insts++;
	}
	/* Increment here because the reg_count_callback store the max
	 * temporary reg index in s->nun_temp_regs. */
	s->num_temp_regs++;
}

static void print_stats(struct radeon_compiler * c)
{
	struct rc_program_stats s;

	if (c->initial_num_insts <= 5)
		return;

	rc_get_stats(c, &s);

	switch (c->type) {
	case RC_VERTEX_PROGRAM:
		fprintf(stderr,"~~~~~~~~~ VERTEX PROGRAM ~~~~~~~~\n"
			       "~%4u Instructions\n"
			       "~%4u Flow Control Instructions\n"
			       "~%4u Temporary Registers\n"
			       "~~~~~~~~~~~~~~ END ~~~~~~~~~~~~~~\n",
			       s.num_insts, s.num_fc_insts, s.num_temp_regs);
		break;

	case RC_FRAGMENT_PROGRAM:
		fprintf(stderr,"~~~~~~~~ FRAGMENT PROGRAM ~~~~~~~\n"
			       "~%4u Instructions\n"
			       "~%4u Vector Instructions (RGB)\n"
			       "~%4u Scalar Instructions (Alpha)\n"
			       "~%4u Flow Control Instructions\n"
			       "~%4u Texture Instructions\n"
			       "~%4u Presub Operations\n"
			       "~%4u OMOD Operations\n"
			       "~%4u Temporary Registers\n"
			       "~%4u Inline Literals\n"
			       "~~~~~~~~~~~~~~ END ~~~~~~~~~~~~~~\n",
			       s.num_insts, s.num_rgb_insts, s.num_alpha_insts,
			       s.num_fc_insts, s.num_tex_insts, s.num_presub_ops,
			       s.num_omod_ops, s.num_temp_regs, s.num_inline_literals);
		break;
	default:
		assert(0);
	}
}

static const char *shader_name[RC_NUM_PROGRAM_TYPES] = {
	"Vertex Program",
	"Fragment Program"
};

void rc_run_compiler_passes(struct radeon_compiler *c, struct radeon_compiler_pass *list)
{
	for (unsigned i = 0; list[i].name; i++) {
		if (list[i].predicate) {
			list[i].run(c, list[i].user);

			if (c->Error)
				return;

			if ((c->Debug & RC_DBG_LOG) && list[i].dump) {
				fprintf(stderr, "%s: after '%s'\n", shader_name[c->type], list[i].name);
				rc_print_program(&c->Program);
			}
		}
	}
}

/* Executes a list of compiler passes given in the parameter 'list'. */
void rc_run_compiler(struct radeon_compiler *c, struct radeon_compiler_pass *list)
{
	struct rc_program_stats s;

	rc_get_stats(c, &s);
	c->initial_num_insts = s.num_insts;

	if (c->Debug & RC_DBG_LOG) {
		fprintf(stderr, "%s: before compilation\n", shader_name[c->type]);
		rc_print_program(&c->Program);
	}

	rc_run_compiler_passes(c, list);

	if (c->Debug & RC_DBG_STATS)
		print_stats(c);
}

void rc_validate_final_shader(struct radeon_compiler *c, void *user)
{
	/* Check the number of constants. */
	if (c->Program.Constants.Count > c->max_constants) {
		rc_error(c, "Too many constants. Max: %i, Got: %i\n",
			 c->max_constants, c->Program.Constants.Count);
	}
}
