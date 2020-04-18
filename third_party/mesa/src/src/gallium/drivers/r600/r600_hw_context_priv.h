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
 *
 * Authors:
 *      Jerome Glisse
 */
#ifndef R600_PRIV_H
#define R600_PRIV_H

#include "r600_pipe.h"

#define R600_MAX_DRAW_CS_DWORDS 16

/* these flags are used in register flags and added into block flags */
#define REG_FLAG_NEED_BO 1
#define REG_FLAG_DIRTY_ALWAYS 2
#define REG_FLAG_RV6XX_SBU 4
#define REG_FLAG_NOT_R600 8
#define REG_FLAG_ENABLE_ALWAYS 16
#define REG_FLAG_FLUSH_CHANGE 64

#define GROUP_FORCE_NEW_BLOCK	0

struct r600_reg {
	unsigned			offset;
	unsigned			flags;
	unsigned			sbu_flags;
};

/*
 * r600_hw_context.c
 */
int r600_context_add_block(struct r600_context *ctx, const struct r600_reg *reg, unsigned nreg,
			   unsigned opcode, unsigned offset_base);
void r600_context_dirty_block(struct r600_context *ctx, struct r600_block *block,
			      int dirty, int index);
int r600_setup_block_table(struct r600_context *ctx);
int r600_state_sampler_init(struct r600_context *ctx, uint32_t offset);
void r600_context_pipe_state_set_sampler(struct r600_context *ctx, struct r600_pipe_state *state, unsigned offset);
void r600_context_ps_partial_flush(struct r600_context *ctx);

/*
 * evergreen_hw_context.c
 */
void evergreen_flush_vgt_streamout(struct r600_context *ctx);
void evergreen_set_streamout_enable(struct r600_context *ctx, unsigned buffer_enable_bit);

#endif
