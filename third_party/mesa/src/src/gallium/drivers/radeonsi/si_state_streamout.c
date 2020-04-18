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

#include "radeonsi_pipe.h"
#include "si_state.h"

/*
 * Stream out
 */

#if 0
void si_context_streamout_begin(struct r600_context *ctx)
{
	struct radeon_winsys_cs *cs = ctx->cs;
	struct si_so_target **t = ctx->so_targets;
	unsigned *strides = ctx->vs_shader_so_strides;
	unsigned buffer_en, i;

	buffer_en = (ctx->num_so_targets >= 1 && t[0] ? 1 : 0) |
		    (ctx->num_so_targets >= 2 && t[1] ? 2 : 0) |
		    (ctx->num_so_targets >= 3 && t[2] ? 4 : 0) |
		    (ctx->num_so_targets >= 4 && t[3] ? 8 : 0);

	ctx->num_cs_dw_streamout_end =
		12 + /* flush_vgt_streamout */
		util_bitcount(buffer_en) * 8 +
		3;

	si_need_cs_space(ctx,
			   12 + /* flush_vgt_streamout */
			   6 + /* enables */
			   util_bitcount(buffer_en & ctx->streamout_append_bitmask) * 8 +
			   util_bitcount(buffer_en & ~ctx->streamout_append_bitmask) * 6 +
			   ctx->num_cs_dw_streamout_end, TRUE);

	if (ctx->chip_class >= CAYMAN) {
		evergreen_flush_vgt_streamout(ctx);
		evergreen_set_streamout_enable(ctx, buffer_en);
	}

	for (i = 0; i < ctx->num_so_targets; i++) {
#if 0
		if (t[i]) {
			t[i]->stride = strides[i];
			t[i]->so_index = i;

			cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONTEXT_REG, 3, 0);
			cs->buf[cs->cdw++] = (R_028AD0_VGT_STRMOUT_BUFFER_SIZE_0 +
							16*i - SI_CONTEXT_REG_OFFSET) >> 2;
			cs->buf[cs->cdw++] = (t[i]->b.buffer_offset +
							t[i]->b.buffer_size) >> 2; /* BUFFER_SIZE (in DW) */
			cs->buf[cs->cdw++] = strides[i] >> 2;		   /* VTX_STRIDE (in DW) */
			cs->buf[cs->cdw++] = 0;			   /* BUFFER_BASE */

			cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
			cs->buf[cs->cdw++] =
				si_context_bo_reloc(ctx, si_resource(t[i]->b.buffer),
						      RADEON_USAGE_WRITE);

			if (ctx->streamout_append_bitmask & (1 << i)) {
				/* Append. */
				cs->buf[cs->cdw++] = PKT3(PKT3_STRMOUT_BUFFER_UPDATE, 4, 0);
				cs->buf[cs->cdw++] = STRMOUT_SELECT_BUFFER(i) |
							       STRMOUT_OFFSET_SOURCE(STRMOUT_OFFSET_FROM_MEM); /* control */
				cs->buf[cs->cdw++] = 0; /* unused */
				cs->buf[cs->cdw++] = 0; /* unused */
				cs->buf[cs->cdw++] = 0; /* src address lo */
				cs->buf[cs->cdw++] = 0; /* src address hi */

				cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
				cs->buf[cs->cdw++] =
					si_context_bo_reloc(ctx,  t[i]->filled_size,
							      RADEON_USAGE_READ);
			} else {
				/* Start from the beginning. */
				cs->buf[cs->cdw++] = PKT3(PKT3_STRMOUT_BUFFER_UPDATE, 4, 0);
				cs->buf[cs->cdw++] = STRMOUT_SELECT_BUFFER(i) |
							       STRMOUT_OFFSET_SOURCE(STRMOUT_OFFSET_FROM_PACKET); /* control */
				cs->buf[cs->cdw++] = 0; /* unused */
				cs->buf[cs->cdw++] = 0; /* unused */
				cs->buf[cs->cdw++] = t[i]->b.buffer_offset >> 2; /* buffer offset in DW */
				cs->buf[cs->cdw++] = 0; /* unused */
			}
		}
#endif
	}
}

void si_context_streamout_end(struct r600_context *ctx)
{
	struct radeon_winsys_cs *cs = ctx->cs;
	struct si_so_target **t = ctx->so_targets;
	unsigned i, flush_flags = 0;

	evergreen_flush_vgt_streamout(ctx);

	for (i = 0; i < ctx->num_so_targets; i++) {
#if 0
		if (t[i]) {
			cs->buf[cs->cdw++] = PKT3(PKT3_STRMOUT_BUFFER_UPDATE, 4, 0);
			cs->buf[cs->cdw++] = STRMOUT_SELECT_BUFFER(i) |
						       STRMOUT_OFFSET_SOURCE(STRMOUT_OFFSET_NONE) |
						       STRMOUT_STORE_BUFFER_FILLED_SIZE; /* control */
			cs->buf[cs->cdw++] = 0; /* dst address lo */
			cs->buf[cs->cdw++] = 0; /* dst address hi */
			cs->buf[cs->cdw++] = 0; /* unused */
			cs->buf[cs->cdw++] = 0; /* unused */

			cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
			cs->buf[cs->cdw++] =
				si_context_bo_reloc(ctx,  t[i]->filled_size,
						      RADEON_USAGE_WRITE);

			flush_flags |= S_0085F0_SO0_DEST_BASE_ENA(1) << i;
		}
#endif
	}

	evergreen_set_streamout_enable(ctx, 0);

	ctx->atom_surface_sync.flush_flags |= flush_flags;
	si_atom_dirty(ctx, &ctx->atom_surface_sync.atom);

	ctx->num_cs_dw_streamout_end = 0;

	/* XXX print some debug info */
	for (i = 0; i < ctx->num_so_targets; i++) {
		if (!t[i])
			continue;

		uint32_t *ptr = ctx->ws->buffer_map(t[i]->filled_size->cs_buf, ctx->cs, RADEON_USAGE_READ);
		printf("FILLED_SIZE%i: %u\n", i, *ptr);
		ctx->ws->buffer_unmap(t[i]->filled_size->cs_buf);
	}
}

void evergreen_flush_vgt_streamout(struct si_context *ctx)
{
	struct radeon_winsys_cs *cs = ctx->cs;

	cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONFIG_REG, 1, 0);
	cs->buf[cs->cdw++] = (R_0084FC_CP_STRMOUT_CNTL - SI_CONFIG_REG_OFFSET) >> 2;
	cs->buf[cs->cdw++] = 0;

	cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
	cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_SO_VGTSTREAMOUT_FLUSH) | EVENT_INDEX(0);

	cs->buf[cs->cdw++] = PKT3(PKT3_WAIT_REG_MEM, 5, 0);
	cs->buf[cs->cdw++] = WAIT_REG_MEM_EQUAL; /* wait until the register is equal to the reference value */
	cs->buf[cs->cdw++] = R_0084FC_CP_STRMOUT_CNTL >> 2;  /* register */
	cs->buf[cs->cdw++] = 0;
	cs->buf[cs->cdw++] = S_0084FC_OFFSET_UPDATE_DONE(1); /* reference value */
	cs->buf[cs->cdw++] = S_0084FC_OFFSET_UPDATE_DONE(1); /* mask */
	cs->buf[cs->cdw++] = 4; /* poll interval */
}

void evergreen_set_streamout_enable(struct si_context *ctx, unsigned buffer_enable_bit)
{
	struct radeon_winsys_cs *cs = ctx->cs;

	if (buffer_enable_bit) {
		cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONTEXT_REG, 1, 0);
		cs->buf[cs->cdw++] = (R_028B94_VGT_STRMOUT_CONFIG - SI_CONTEXT_REG_OFFSET) >> 2;
		cs->buf[cs->cdw++] = S_028B94_STREAMOUT_0_EN(1);

		cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONTEXT_REG, 1, 0);
		cs->buf[cs->cdw++] = (R_028B98_VGT_STRMOUT_BUFFER_CONFIG - SI_CONTEXT_REG_OFFSET) >> 2;
		cs->buf[cs->cdw++] = S_028B98_STREAM_0_BUFFER_EN(buffer_enable_bit);
	} else {
		cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONTEXT_REG, 1, 0);
		cs->buf[cs->cdw++] = (R_028B94_VGT_STRMOUT_CONFIG - SI_CONTEXT_REG_OFFSET) >> 2;
		cs->buf[cs->cdw++] = S_028B94_STREAMOUT_0_EN(0);
	}
}

#endif

struct pipe_stream_output_target *
si_create_so_target(struct pipe_context *ctx,
		    struct pipe_resource *buffer,
		    unsigned buffer_offset,
		    unsigned buffer_size)
{
#if 0
	struct si_context *rctx = (struct r600_context *)ctx;
	struct si_so_target *t;
	void *ptr;

	t = CALLOC_STRUCT(si_so_target);
	if (!t) {
		return NULL;
	}

	t->b.reference.count = 1;
	t->b.context = ctx;
	pipe_resource_reference(&t->b.buffer, buffer);
	t->b.buffer_offset = buffer_offset;
	t->b.buffer_size = buffer_size;

	t->filled_size = si_resource_create_custom(ctx->screen, PIPE_USAGE_STATIC, 4);
	ptr = rctx->ws->buffer_map(t->filled_size->cs_buf, rctx->cs, PIPE_TRANSFER_WRITE);
	memset(ptr, 0, t->filled_size->buf->size);
	rctx->ws->buffer_unmap(t->filled_size->cs_buf);

	return &t->b;
#endif
	return NULL;
}

void si_so_target_destroy(struct pipe_context *ctx,
			  struct pipe_stream_output_target *target)
{
#if 0
	struct si_so_target *t = (struct r600_so_target*)target;
	pipe_resource_reference(&t->b.buffer, NULL);
	si_resource_reference(&t->filled_size, NULL);
	FREE(t);
#endif
}

void si_set_so_targets(struct pipe_context *ctx,
		       unsigned num_targets,
		       struct pipe_stream_output_target **targets,
		       unsigned append_bitmask)
{
	assert(num_targets == 0);
#if 0
	struct si_context *rctx = (struct r600_context *)ctx;
	unsigned i;

	/* Stop streamout. */
	if (rctx->num_so_targets) {
		si_context_streamout_end(rctx);
	}

	/* Set the new targets. */
	for (i = 0; i < num_targets; i++) {
		pipe_so_target_reference((struct pipe_stream_output_target**)&rctx->so_targets[i], targets[i]);
	}
	for (; i < rctx->num_so_targets; i++) {
		pipe_so_target_reference((struct pipe_stream_output_target**)&rctx->so_targets[i], NULL);
	}

	rctx->num_so_targets = num_targets;
	rctx->streamout_start = num_targets != 0;
	rctx->streamout_append_bitmask = append_bitmask;
#endif
}
