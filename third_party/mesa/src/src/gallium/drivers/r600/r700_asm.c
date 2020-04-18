/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
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
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "r600_asm.h"
#include "r700_sq.h"

void r700_bytecode_cf_vtx_build(uint32_t *bytecode, const struct r600_bytecode_cf *cf)
{
	unsigned count = (cf->ndw / 4) - 1;
	*bytecode++ = S_SQ_CF_WORD0_ADDR(cf->addr >> 1);
	*bytecode++ = cf->inst |
			S_SQ_CF_WORD1_BARRIER(1) |
			S_SQ_CF_WORD1_COUNT(count) |
			S_SQ_CF_WORD1_COUNT_3(count >> 3);
}

int r700_bytecode_alu_build(struct r600_bytecode *bc, struct r600_bytecode_alu *alu, unsigned id)
{
	bc->bytecode[id++] = S_SQ_ALU_WORD0_SRC0_SEL(alu->src[0].sel) |
		S_SQ_ALU_WORD0_SRC0_REL(alu->src[0].rel) |
		S_SQ_ALU_WORD0_SRC0_CHAN(alu->src[0].chan) |
		S_SQ_ALU_WORD0_SRC0_NEG(alu->src[0].neg) |
		S_SQ_ALU_WORD0_SRC1_SEL(alu->src[1].sel) |
		S_SQ_ALU_WORD0_SRC1_REL(alu->src[1].rel) |
		S_SQ_ALU_WORD0_SRC1_CHAN(alu->src[1].chan) |
		S_SQ_ALU_WORD0_SRC1_NEG(alu->src[1].neg) |
		S_SQ_ALU_WORD0_PRED_SEL(alu->pred_sel) |
		S_SQ_ALU_WORD0_LAST(alu->last);

	/* don't replace gpr by pv or ps for destination register */
	if (alu->is_op3) {
		bc->bytecode[id++] = S_SQ_ALU_WORD1_DST_GPR(alu->dst.sel) |
					S_SQ_ALU_WORD1_DST_CHAN(alu->dst.chan) |
			                S_SQ_ALU_WORD1_DST_REL(alu->dst.rel) |
			                S_SQ_ALU_WORD1_CLAMP(alu->dst.clamp) |
					S_SQ_ALU_WORD1_OP3_SRC2_SEL(alu->src[2].sel) |
					S_SQ_ALU_WORD1_OP3_SRC2_REL(alu->src[2].rel) |
					S_SQ_ALU_WORD1_OP3_SRC2_CHAN(alu->src[2].chan) |
					S_SQ_ALU_WORD1_OP3_SRC2_NEG(alu->src[2].neg) |
					S_SQ_ALU_WORD1_OP3_ALU_INST(alu->inst) |
					S_SQ_ALU_WORD1_BANK_SWIZZLE(alu->bank_swizzle);
	} else {
		bc->bytecode[id++] = S_SQ_ALU_WORD1_DST_GPR(alu->dst.sel) |
					S_SQ_ALU_WORD1_DST_CHAN(alu->dst.chan) |
			                S_SQ_ALU_WORD1_DST_REL(alu->dst.rel) |
			                S_SQ_ALU_WORD1_CLAMP(alu->dst.clamp) |
					S_SQ_ALU_WORD1_OP2_SRC0_ABS(alu->src[0].abs) |
					S_SQ_ALU_WORD1_OP2_SRC1_ABS(alu->src[1].abs) |
					S_SQ_ALU_WORD1_OP2_WRITE_MASK(alu->dst.write) |
					S_SQ_ALU_WORD1_OP2_OMOD(alu->omod) |
					S_SQ_ALU_WORD1_OP2_ALU_INST(alu->inst) |
					S_SQ_ALU_WORD1_BANK_SWIZZLE(alu->bank_swizzle) |
			                S_SQ_ALU_WORD1_OP2_UPDATE_EXECUTE_MASK(alu->execute_mask) |
			                S_SQ_ALU_WORD1_OP2_UPDATE_PRED(alu->update_pred);
	}
	return 0;
}
