/*
 * Copyright 2012 Advanced Micro Devices, Inc.
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
 *      Christian König <christian.koenig@amd.com>
 */

#include "util/u_memory.h"
#include "radeonsi_pipe.h"
#include "radeonsi_pm4.h"
#include "sid.h"
#include "r600_hw_context_priv.h"

#define NUMBER_OF_STATES (sizeof(union si_state) / sizeof(struct si_pm4_state *))

void si_pm4_cmd_begin(struct si_pm4_state *state, unsigned opcode)
{
	state->last_opcode = opcode;
	state->last_pm4 = state->ndw++;
}

void si_pm4_cmd_add(struct si_pm4_state *state, uint32_t dw)
{
	state->pm4[state->ndw++] = dw;
}

void si_pm4_cmd_end(struct si_pm4_state *state, bool predicate)
{
	unsigned count;
	count = state->ndw - state->last_pm4 - 2;
	state->pm4[state->last_pm4] = PKT3(state->last_opcode,
					   count, predicate);

	assert(state->ndw <= SI_PM4_MAX_DW);
}

void si_pm4_set_reg(struct si_pm4_state *state, unsigned reg, uint32_t val)
{
	unsigned opcode;

	if (reg >= SI_CONFIG_REG_OFFSET && reg <= SI_CONFIG_REG_END) {
		opcode = PKT3_SET_CONFIG_REG;
		reg -= SI_CONFIG_REG_OFFSET;

	} else if (reg >= SI_SH_REG_OFFSET && reg <= SI_SH_REG_END) {
		opcode = PKT3_SET_SH_REG;
		reg -= SI_SH_REG_OFFSET;

	} else if (reg >= SI_CONTEXT_REG_OFFSET && reg <= SI_CONTEXT_REG_END) {
		opcode = PKT3_SET_CONTEXT_REG;
		reg -= SI_CONTEXT_REG_OFFSET;
	} else {
		R600_ERR("Invalid register offset %08x!\n", reg);
		return;
	}

	reg >>= 2;

	if (opcode != state->last_opcode || reg != (state->last_reg + 1)) {
		si_pm4_cmd_begin(state, opcode);
		si_pm4_cmd_add(state, reg);
	}

	state->last_reg = reg;
	si_pm4_cmd_add(state, val);
	si_pm4_cmd_end(state, false);
}

void si_pm4_add_bo(struct si_pm4_state *state,
                   struct si_resource *bo,
                   enum radeon_bo_usage usage)
{
	unsigned idx = state->nbo++;
	assert(idx < SI_PM4_MAX_BO);

	si_resource_reference(&state->bo[idx], bo);
	state->bo_usage[idx] = usage;
}

void si_pm4_sh_data_begin(struct si_pm4_state *state)
{
	si_pm4_cmd_begin(state, PKT3_NOP);
}

void si_pm4_sh_data_add(struct si_pm4_state *state, uint32_t dw)
{
	si_pm4_cmd_add(state, dw);
}

void si_pm4_sh_data_end(struct si_pm4_state *state, unsigned reg)
{
	unsigned offs = state->last_pm4 + 1;

	/* Bail if no data was added */
	if (state->ndw == offs) {
		state->ndw--;
		return;
	}

	si_pm4_cmd_end(state, false);

	si_pm4_cmd_begin(state, PKT3_SET_SH_REG_OFFSET);
	si_pm4_cmd_add(state, (reg - SI_SH_REG_OFFSET) >> 2);
	state->relocs[state->nrelocs++] = state->ndw;
	si_pm4_cmd_add(state, offs << 2);
	si_pm4_cmd_add(state, 0);
	si_pm4_cmd_end(state, false);
}

void si_pm4_inval_shader_cache(struct si_pm4_state *state)
{
	state->cp_coher_cntl |= S_0085F0_SH_ICACHE_ACTION_ENA(1);
	state->cp_coher_cntl |= S_0085F0_SH_KCACHE_ACTION_ENA(1);
}

void si_pm4_inval_texture_cache(struct si_pm4_state *state)
{
	state->cp_coher_cntl |= S_0085F0_TC_ACTION_ENA(1);
}

void si_pm4_inval_vertex_cache(struct si_pm4_state *state)
{
        /* Some GPUs don't have the vertex cache and must use the texture cache instead. */
	state->cp_coher_cntl |= S_0085F0_TC_ACTION_ENA(1);
}

void si_pm4_inval_fb_cache(struct si_pm4_state *state, unsigned nr_cbufs)
{
	state->cp_coher_cntl |= S_0085F0_CB_ACTION_ENA(1);
	state->cp_coher_cntl |= ((1 << nr_cbufs) - 1) << S_0085F0_CB0_DEST_BASE_ENA_SHIFT;
}

void si_pm4_inval_zsbuf_cache(struct si_pm4_state *state)
{
	state->cp_coher_cntl |= S_0085F0_DB_ACTION_ENA(1) | S_0085F0_DB_DEST_BASE_ENA(1);
}

void si_pm4_free_state(struct r600_context *rctx,
		       struct si_pm4_state *state,
		       unsigned idx)
{
	if (state == NULL)
		return;

	if (idx != ~0 && rctx->emitted.array[idx] == state) {
		rctx->emitted.array[idx] = NULL;
	}

	for (int i = 0; i < state->nbo; ++i) {
		si_resource_reference(&state->bo[i], NULL);
	}
	FREE(state);
}

uint32_t si_pm4_sync_flags(struct r600_context *rctx)
{
	uint32_t cp_coher_cntl = 0;

	for (int i = 0; i < NUMBER_OF_STATES; ++i) {
		struct si_pm4_state *state = rctx->queued.array[i];

		if (!state || rctx->emitted.array[i] == state)
			continue;

		cp_coher_cntl |= state->cp_coher_cntl;
	}
	return cp_coher_cntl;
}

unsigned si_pm4_dirty_dw(struct r600_context *rctx)
{
	unsigned count = 0;

	for (int i = 0; i < NUMBER_OF_STATES; ++i) {
		struct si_pm4_state *state = rctx->queued.array[i];

		if (!state || rctx->emitted.array[i] == state)
			continue;

		count += state->ndw;
	}

	return count;
}

void si_pm4_emit(struct r600_context *rctx, struct si_pm4_state *state)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	for (int i = 0; i < state->nbo; ++i) {
		r600_context_bo_reloc(rctx, state->bo[i],
				      state->bo_usage[i]);
	}

	memcpy(&cs->buf[cs->cdw], state->pm4, state->ndw * 4);

	for (int i = 0; i < state->nrelocs; ++i) {
		cs->buf[cs->cdw + state->relocs[i]] += cs->cdw << 2;
	}

	cs->cdw += state->ndw;
}

void si_pm4_emit_dirty(struct r600_context *rctx)
{
	for (int i = 0; i < NUMBER_OF_STATES; ++i) {
		struct si_pm4_state *state = rctx->queued.array[i];

		if (!state || rctx->emitted.array[i] == state)
			continue;

		si_pm4_emit(rctx, state);
		rctx->emitted.array[i] = state;
	}
}

void si_pm4_reset_emitted(struct r600_context *rctx)
{
	memset(&rctx->emitted, 0, sizeof(rctx->emitted));
}
