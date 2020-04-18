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
#include "r600_hw_context_priv.h"
#include "radeonsi_pm4.h"
#include "radeonsi_pipe.h"
#include "sid.h"
#include "util/u_memory.h"
#include <errno.h>

#define GROUP_FORCE_NEW_BLOCK	0

/* Get backends mask */
void si_get_backend_mask(struct r600_context *ctx)
{
	struct radeon_winsys_cs *cs = ctx->cs;
	struct si_resource *buffer;
	uint32_t *results;
	unsigned num_backends = ctx->screen->info.r600_num_backends;
	unsigned i, mask = 0;

	/* if backend_map query is supported by the kernel */
	if (ctx->screen->info.r600_backend_map_valid) {
		unsigned num_tile_pipes = ctx->screen->info.r600_num_tile_pipes;
		unsigned backend_map = ctx->screen->info.r600_backend_map;
		unsigned item_width, item_mask;

		if (ctx->chip_class >= CAYMAN) {
			item_width = 4;
			item_mask = 0x7;
		}

		while(num_tile_pipes--) {
			i = backend_map & item_mask;
			mask |= (1<<i);
			backend_map >>= item_width;
		}
		if (mask != 0) {
			ctx->backend_mask = mask;
			return;
		}
	}

	/* otherwise backup path for older kernels */

	/* create buffer for event data */
	buffer = si_resource_create_custom(&ctx->screen->screen,
					   PIPE_USAGE_STAGING,
					   ctx->max_db*16);
	if (!buffer)
		goto err;

	/* initialize buffer with zeroes */
	results = ctx->ws->buffer_map(buffer->cs_buf, ctx->cs, PIPE_TRANSFER_WRITE);
	if (results) {
		uint64_t va = 0;

		memset(results, 0, ctx->max_db * 4 * 4);
		ctx->ws->buffer_unmap(buffer->cs_buf);

		/* emit EVENT_WRITE for ZPASS_DONE */
		va = r600_resource_va(&ctx->screen->screen, (void *)buffer);
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 2, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_ZPASS_DONE) | EVENT_INDEX(1);
		cs->buf[cs->cdw++] = va;
		cs->buf[cs->cdw++] = va >> 32;

		cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
		cs->buf[cs->cdw++] = r600_context_bo_reloc(ctx, buffer, RADEON_USAGE_WRITE);

		/* analyze results */
		results = ctx->ws->buffer_map(buffer->cs_buf, ctx->cs, PIPE_TRANSFER_READ);
		if (results) {
			for(i = 0; i < ctx->max_db; i++) {
				/* at least highest bit will be set if backend is used */
				if (results[i*4 + 1])
					mask |= (1<<i);
			}
			ctx->ws->buffer_unmap(buffer->cs_buf);
		}
	}

	si_resource_reference(&buffer, NULL);

	if (mask != 0) {
		ctx->backend_mask = mask;
		return;
	}

err:
	/* fallback to old method - set num_backends lower bits to 1 */
	ctx->backend_mask = (~((uint32_t)0))>>(32-num_backends);
	return;
}

/* initialize */
void si_need_cs_space(struct r600_context *ctx, unsigned num_dw,
			boolean count_draw_in)
{
	/* The number of dwords we already used in the CS so far. */
	num_dw += ctx->cs->cdw;

	if (count_draw_in) {
		/* The number of dwords all the dirty states would take. */
		num_dw += ctx->pm4_dirty_cdwords;

		/* The upper-bound of how much a draw command would take. */
		num_dw += SI_MAX_DRAW_CS_DWORDS;
	}

	/* Count in queries_suspend. */
	num_dw += ctx->num_cs_dw_queries_suspend;

	/* Count in streamout_end at the end of CS. */
	num_dw += ctx->num_cs_dw_streamout_end;

	/* Count in render_condition(NULL) at the end of CS. */
	if (ctx->predicate_drawing) {
		num_dw += 3;
	}

	/* Count in framebuffer cache flushes at the end of CS. */
	num_dw += 7; /* one SURFACE_SYNC and CACHE_FLUSH_AND_INV (r6xx-only) */

	/* Save 16 dwords for the fence mechanism. */
	num_dw += 16;

	/* Flush if there's not enough space. */
	if (num_dw > RADEON_MAX_CMDBUF_DWORDS) {
		radeonsi_flush(&ctx->context, NULL, RADEON_FLUSH_ASYNC);
	}
}

static void r600_flush_framebuffer(struct r600_context *ctx)
{
	struct si_pm4_state *pm4;

	if (!(ctx->flags & R600_CONTEXT_DST_CACHES_DIRTY))
		return;

	pm4 = CALLOC_STRUCT(si_pm4_state);
	si_cmd_surface_sync(pm4, S_0085F0_CB0_DEST_BASE_ENA(1) |
				S_0085F0_CB1_DEST_BASE_ENA(1) |
				S_0085F0_CB2_DEST_BASE_ENA(1) |
				S_0085F0_CB3_DEST_BASE_ENA(1) |
				S_0085F0_CB4_DEST_BASE_ENA(1) |
				S_0085F0_CB5_DEST_BASE_ENA(1) |
				S_0085F0_CB6_DEST_BASE_ENA(1) |
				S_0085F0_CB7_DEST_BASE_ENA(1) |
				S_0085F0_DB_ACTION_ENA(1) |
				S_0085F0_DB_DEST_BASE_ENA(1));
	si_pm4_emit(ctx, pm4);
	si_pm4_free_state(ctx, pm4, ~0);

	ctx->flags &= ~R600_CONTEXT_DST_CACHES_DIRTY;
}

void si_context_flush(struct r600_context *ctx, unsigned flags)
{
	struct radeon_winsys_cs *cs = ctx->cs;
	bool queries_suspended = false;

#if 0
	bool streamout_suspended = false;
#endif

	if (!cs->cdw)
		return;

	/* suspend queries */
	if (ctx->num_cs_dw_queries_suspend) {
		r600_context_queries_suspend(ctx);
		queries_suspended = true;
	}

#if 0
	if (ctx->num_cs_dw_streamout_end) {
		r600_context_streamout_end(ctx);
		streamout_suspended = true;
	}
#endif

	r600_flush_framebuffer(ctx);

	/* partial flush is needed to avoid lockups on some chips with user fences */
	cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
	cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_PS_PARTIAL_FLUSH) | EVENT_INDEX(4);

	/* force to keep tiling flags */
	flags |= RADEON_FLUSH_KEEP_TILING_FLAGS;

	/* Flush the CS. */
	ctx->ws->cs_flush(ctx->cs, flags);

	ctx->pm4_dirty_cdwords = 0;
	ctx->flags = 0;

#if 0
	if (streamout_suspended) {
		ctx->streamout_start = TRUE;
		ctx->streamout_append_bitmask = ~0;
	}
#endif

	/* resume queries */
	if (queries_suspended) {
		r600_context_queries_resume(ctx);
	}

	/* set all valid group as dirty so they get reemited on
	 * next draw command
	 */
	si_pm4_reset_emitted(ctx);
}

void si_context_emit_fence(struct r600_context *ctx, struct si_resource *fence_bo, unsigned offset, unsigned value)
{
	struct radeon_winsys_cs *cs = ctx->cs;
	uint64_t va;

	si_need_cs_space(ctx, 10, FALSE);

	va = r600_resource_va(&ctx->screen->screen, (void*)fence_bo);
	va = va + (offset << 2);

	cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
	cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_PS_PARTIAL_FLUSH) | EVENT_INDEX(4);
	cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE_EOP, 4, 0);
	cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_CACHE_FLUSH_AND_INV_TS_EVENT) | EVENT_INDEX(5);
	cs->buf[cs->cdw++] = va & 0xFFFFFFFFUL;       /* ADDRESS_LO */
	/* DATA_SEL | INT_EN | ADDRESS_HI */
	cs->buf[cs->cdw++] = (1 << 29) | (0 << 24) | ((va >> 32UL) & 0xFF);
	cs->buf[cs->cdw++] = value;                   /* DATA_LO */
	cs->buf[cs->cdw++] = 0;                       /* DATA_HI */
	cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
	cs->buf[cs->cdw++] = r600_context_bo_reloc(ctx, fence_bo, RADEON_USAGE_WRITE);
}

static unsigned r600_query_read_result(char *map, unsigned start_index, unsigned end_index,
				       bool test_status_bit)
{
	uint32_t *current_result = (uint32_t*)map;
	uint64_t start, end;

	start = (uint64_t)current_result[start_index] |
		(uint64_t)current_result[start_index+1] << 32;
	end = (uint64_t)current_result[end_index] |
	      (uint64_t)current_result[end_index+1] << 32;

	if (!test_status_bit ||
	    ((start & 0x8000000000000000UL) && (end & 0x8000000000000000UL))) {
		return end - start;
	}
	return 0;
}

static boolean r600_query_result(struct r600_context *ctx, struct r600_query *query, boolean wait)
{
	unsigned results_base = query->results_start;
	char *map;

	map = ctx->ws->buffer_map(query->buffer->cs_buf, ctx->cs,
				  PIPE_TRANSFER_READ |
				  (wait ? 0 : PIPE_TRANSFER_DONTBLOCK));
	if (!map)
		return FALSE;

	/* count all results across all data blocks */
	switch (query->type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
		while (results_base != query->results_end) {
			query->result.u64 +=
				r600_query_read_result(map + results_base, 0, 2, true);
			results_base = (results_base + 16) % query->buffer->b.b.width0;
		}
		break;
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		while (results_base != query->results_end) {
			query->result.b = query->result.b ||
				r600_query_read_result(map + results_base, 0, 2, true) != 0;
			results_base = (results_base + 16) % query->buffer->b.b.width0;
		}
		break;
	case PIPE_QUERY_TIME_ELAPSED:
		while (results_base != query->results_end) {
			query->result.u64 +=
				r600_query_read_result(map + results_base, 0, 2, false);
			results_base = (results_base + query->result_size) % query->buffer->b.b.width0;
		}
		break;
	case PIPE_QUERY_PRIMITIVES_EMITTED:
		/* SAMPLE_STREAMOUTSTATS stores this structure:
		 * {
		 *    u64 NumPrimitivesWritten;
		 *    u64 PrimitiveStorageNeeded;
		 * }
		 * We only need NumPrimitivesWritten here. */
		while (results_base != query->results_end) {
			query->result.u64 +=
				r600_query_read_result(map + results_base, 2, 6, true);
			results_base = (results_base + query->result_size) % query->buffer->b.b.width0;
		}
		break;
	case PIPE_QUERY_PRIMITIVES_GENERATED:
		/* Here we read PrimitiveStorageNeeded. */
		while (results_base != query->results_end) {
			query->result.u64 +=
				r600_query_read_result(map + results_base, 0, 4, true);
			results_base = (results_base + query->result_size) % query->buffer->b.b.width0;
		}
		break;
	case PIPE_QUERY_SO_STATISTICS:
		while (results_base != query->results_end) {
			query->result.so.num_primitives_written +=
				r600_query_read_result(map + results_base, 2, 6, true);
			query->result.so.primitives_storage_needed +=
				r600_query_read_result(map + results_base, 0, 4, true);
			results_base = (results_base + query->result_size) % query->buffer->b.b.width0;
		}
		break;
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		while (results_base != query->results_end) {
			query->result.b = query->result.b ||
				r600_query_read_result(map + results_base, 2, 6, true) !=
				r600_query_read_result(map + results_base, 0, 4, true);
			results_base = (results_base + query->result_size) % query->buffer->b.b.width0;
		}
		break;
	default:
		assert(0);
	}

	query->results_start = query->results_end;
	ctx->ws->buffer_unmap(query->buffer->cs_buf);
	return TRUE;
}

void r600_query_begin(struct r600_context *ctx, struct r600_query *query)
{
	struct radeon_winsys_cs *cs = ctx->cs;
	unsigned new_results_end, i;
	uint32_t *results;
	uint64_t va;

	si_need_cs_space(ctx, query->num_cs_dw * 2, TRUE);

	new_results_end = (query->results_end + query->result_size) % query->buffer->b.b.width0;

	/* collect current results if query buffer is full */
	if (new_results_end == query->results_start) {
		r600_query_result(ctx, query, TRUE);
	}

	switch (query->type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		results = ctx->ws->buffer_map(query->buffer->cs_buf, ctx->cs, PIPE_TRANSFER_WRITE);
		if (results) {
			results = (uint32_t*)((char*)results + query->results_end);
			memset(results, 0, query->result_size);

			/* Set top bits for unused backends */
			for (i = 0; i < ctx->max_db; i++) {
				if (!(ctx->backend_mask & (1<<i))) {
					results[(i * 4)+1] = 0x80000000;
					results[(i * 4)+3] = 0x80000000;
				}
			}
			ctx->ws->buffer_unmap(query->buffer->cs_buf);
		}
		break;
	case PIPE_QUERY_TIME_ELAPSED:
		break;
	case PIPE_QUERY_PRIMITIVES_EMITTED:
	case PIPE_QUERY_PRIMITIVES_GENERATED:
	case PIPE_QUERY_SO_STATISTICS:
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		results = ctx->ws->buffer_map(query->buffer->cs_buf, ctx->cs, PIPE_TRANSFER_WRITE);
		results = (uint32_t*)((char*)results + query->results_end);
		memset(results, 0, query->result_size);
		ctx->ws->buffer_unmap(query->buffer->cs_buf);
		break;
	default:
		assert(0);
	}

	/* emit begin query */
	va = r600_resource_va(&ctx->screen->screen, (void*)query->buffer);
	va += query->results_end;

	switch (query->type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 2, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_ZPASS_DONE) | EVENT_INDEX(1);
		cs->buf[cs->cdw++] = va;
		cs->buf[cs->cdw++] = (va >> 32UL) & 0xFF;
		break;
	case PIPE_QUERY_PRIMITIVES_EMITTED:
	case PIPE_QUERY_PRIMITIVES_GENERATED:
	case PIPE_QUERY_SO_STATISTICS:
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 2, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_SAMPLE_STREAMOUTSTATS) | EVENT_INDEX(3);
		cs->buf[cs->cdw++] = query->results_end;
		cs->buf[cs->cdw++] = 0;
		break;
	case PIPE_QUERY_TIME_ELAPSED:
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE_EOP, 4, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_CACHE_FLUSH_AND_INV_TS_EVENT) | EVENT_INDEX(5);
		cs->buf[cs->cdw++] = va;
		cs->buf[cs->cdw++] = (3 << 29) | ((va >> 32UL) & 0xFF);
		cs->buf[cs->cdw++] = 0;
		cs->buf[cs->cdw++] = 0;
		break;
	default:
		assert(0);
	}
	cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
	cs->buf[cs->cdw++] = r600_context_bo_reloc(ctx, query->buffer, RADEON_USAGE_WRITE);

	ctx->num_cs_dw_queries_suspend += query->num_cs_dw;
}

void r600_query_end(struct r600_context *ctx, struct r600_query *query)
{
	struct radeon_winsys_cs *cs = ctx->cs;
	uint64_t va;

	va = r600_resource_va(&ctx->screen->screen, (void*)query->buffer);
	/* emit end query */
	switch (query->type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		va += query->results_end + 8;
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 2, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_ZPASS_DONE) | EVENT_INDEX(1);
		cs->buf[cs->cdw++] = va;
		cs->buf[cs->cdw++] = (va >> 32UL) & 0xFF;
		break;
	case PIPE_QUERY_PRIMITIVES_EMITTED:
	case PIPE_QUERY_PRIMITIVES_GENERATED:
	case PIPE_QUERY_SO_STATISTICS:
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 2, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_SAMPLE_STREAMOUTSTATS) | EVENT_INDEX(3);
		cs->buf[cs->cdw++] = query->results_end + query->result_size/2;
		cs->buf[cs->cdw++] = 0;
		break;
	case PIPE_QUERY_TIME_ELAPSED:
		va += query->results_end + query->result_size/2;
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE_EOP, 4, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_CACHE_FLUSH_AND_INV_TS_EVENT) | EVENT_INDEX(5);
		cs->buf[cs->cdw++] = va;
		cs->buf[cs->cdw++] = (3 << 29) | ((va >> 32UL) & 0xFF);
		cs->buf[cs->cdw++] = 0;
		cs->buf[cs->cdw++] = 0;
		break;
	default:
		assert(0);
	}
	cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
	cs->buf[cs->cdw++] = r600_context_bo_reloc(ctx, query->buffer, RADEON_USAGE_WRITE);

	query->results_end = (query->results_end + query->result_size) % query->buffer->b.b.width0;
	ctx->num_cs_dw_queries_suspend -= query->num_cs_dw;
}

void r600_query_predication(struct r600_context *ctx, struct r600_query *query, int operation,
			    int flag_wait)
{
	struct radeon_winsys_cs *cs = ctx->cs;
	uint64_t va;

	if (operation == PREDICATION_OP_CLEAR) {
		si_need_cs_space(ctx, 3, FALSE);

		cs->buf[cs->cdw++] = PKT3(PKT3_SET_PREDICATION, 1, 0);
		cs->buf[cs->cdw++] = 0;
		cs->buf[cs->cdw++] = PRED_OP(PREDICATION_OP_CLEAR);
	} else {
		unsigned results_base = query->results_start;
		unsigned count;
		uint32_t op;

		/* find count of the query data blocks */
		count = (query->buffer->b.b.width0 + query->results_end - query->results_start) % query->buffer->b.b.width0;
		count /= query->result_size;

		si_need_cs_space(ctx, 5 * count, TRUE);

		op = PRED_OP(operation) | PREDICATION_DRAW_VISIBLE |
				(flag_wait ? PREDICATION_HINT_WAIT : PREDICATION_HINT_NOWAIT_DRAW);
		va = r600_resource_va(&ctx->screen->screen, (void*)query->buffer);

		/* emit predicate packets for all data blocks */
		while (results_base != query->results_end) {
			cs->buf[cs->cdw++] = PKT3(PKT3_SET_PREDICATION, 1, 0);
			cs->buf[cs->cdw++] = (va + results_base) & 0xFFFFFFFFUL;
			cs->buf[cs->cdw++] = op | (((va + results_base) >> 32UL) & 0xFF);
			cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
			cs->buf[cs->cdw++] = r600_context_bo_reloc(ctx, query->buffer,
									     RADEON_USAGE_READ);
			results_base = (results_base + query->result_size) % query->buffer->b.b.width0;

			/* set CONTINUE bit for all packets except the first */
			op |= PREDICATION_CONTINUE;
		}
	}
}

struct r600_query *r600_context_query_create(struct r600_context *ctx, unsigned query_type)
{
	struct r600_query *query;
	unsigned buffer_size = 4096;

	query = CALLOC_STRUCT(r600_query);
	if (query == NULL)
		return NULL;

	query->type = query_type;

	switch (query_type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		query->result_size = 16 * ctx->max_db;
		query->num_cs_dw = 6;
		break;
	case PIPE_QUERY_TIME_ELAPSED:
		query->result_size = 16;
		query->num_cs_dw = 8;
		break;
	case PIPE_QUERY_PRIMITIVES_EMITTED:
	case PIPE_QUERY_PRIMITIVES_GENERATED:
	case PIPE_QUERY_SO_STATISTICS:
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		/* NumPrimitivesWritten, PrimitiveStorageNeeded. */
		query->result_size = 32;
		query->num_cs_dw = 6;
		break;
	default:
		assert(0);
		FREE(query);
		return NULL;
	}

	/* adjust buffer size to simplify offsets wrapping math */
	buffer_size -= buffer_size % query->result_size;

	/* Queries are normally read by the CPU after
	 * being written by the gpu, hence staging is probably a good
	 * usage pattern.
	 */
	query->buffer = si_resource_create_custom(&ctx->screen->screen,
						  PIPE_USAGE_STAGING,
						  buffer_size);
	if (!query->buffer) {
		FREE(query);
		return NULL;
	}
	return query;
}

void r600_context_query_destroy(struct r600_context *ctx, struct r600_query *query)
{
	si_resource_reference(&query->buffer, NULL);
	free(query);
}

boolean r600_context_query_result(struct r600_context *ctx,
				struct r600_query *query,
				boolean wait, void *vresult)
{
	boolean *result_b = (boolean*)vresult;
	uint64_t *result_u64 = (uint64_t*)vresult;
	struct pipe_query_data_so_statistics *result_so =
		(struct pipe_query_data_so_statistics*)vresult;

	if (!r600_query_result(ctx, query, wait))
		return FALSE;

	switch (query->type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
	case PIPE_QUERY_PRIMITIVES_EMITTED:
	case PIPE_QUERY_PRIMITIVES_GENERATED:
		*result_u64 = query->result.u64;
		break;
	case PIPE_QUERY_OCCLUSION_PREDICATE:
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		*result_b = query->result.b;
		break;
	case PIPE_QUERY_TIME_ELAPSED:
		*result_u64 = (1000000 * query->result.u64) / ctx->screen->info.r600_clock_crystal_freq;
		break;
	case PIPE_QUERY_SO_STATISTICS:
		*result_so = query->result.so;
		break;
	default:
		assert(0);
	}
	return TRUE;
}

void r600_context_queries_suspend(struct r600_context *ctx)
{
	struct r600_query *query;

	LIST_FOR_EACH_ENTRY(query, &ctx->active_query_list, list) {
		r600_query_end(ctx, query);
	}
	assert(ctx->num_cs_dw_queries_suspend == 0);
}

void r600_context_queries_resume(struct r600_context *ctx)
{
	struct r600_query *query;

	assert(ctx->num_cs_dw_queries_suspend == 0);

	LIST_FOR_EACH_ENTRY(query, &ctx->active_query_list, list) {
		r600_query_begin(ctx, query);
	}
}

void r600_context_draw_opaque_count(struct r600_context *ctx, struct r600_so_target *t)
{
	struct radeon_winsys_cs *cs = ctx->cs;
	si_need_cs_space(ctx, 14 + 21, TRUE);

	cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONTEXT_REG, 1, 0);
	cs->buf[cs->cdw++] = (R_028B28_VGT_STRMOUT_DRAW_OPAQUE_OFFSET - SI_CONTEXT_REG_OFFSET) >> 2;
	cs->buf[cs->cdw++] = 0;

	cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONTEXT_REG, 1, 0);
	cs->buf[cs->cdw++] = (R_028B30_VGT_STRMOUT_DRAW_OPAQUE_VERTEX_STRIDE - SI_CONTEXT_REG_OFFSET) >> 2;
	cs->buf[cs->cdw++] = t->stride >> 2;

#if 0
	cs->buf[cs->cdw++] = PKT3(PKT3_COPY_DW, 4, 0);
	cs->buf[cs->cdw++] = COPY_DW_SRC_IS_MEM | COPY_DW_DST_IS_REG;
	cs->buf[cs->cdw++] = 0; /* src address lo */
	cs->buf[cs->cdw++] = 0; /* src address hi */
	cs->buf[cs->cdw++] = R_028B2C_VGT_STRMOUT_DRAW_OPAQUE_BUFFER_FILLED_SIZE >> 2; /* dst register */
	cs->buf[cs->cdw++] = 0; /* unused */
#endif

	cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
	cs->buf[cs->cdw++] = r600_context_bo_reloc(ctx, t->filled_size, RADEON_USAGE_READ);

#if 0 /* I have not found this useful yet. */
	cs->buf[cs->cdw++] = PKT3(PKT3_COPY_DW, 4, 0);
	cs->buf[cs->cdw++] = COPY_DW_SRC_IS_REG | COPY_DW_DST_IS_REG;
	cs->buf[cs->cdw++] = R_028B2C_VGT_STRMOUT_DRAW_OPAQUE_BUFFER_FILLED_SIZE >> 2; /* src register */
	cs->buf[cs->cdw++] = 0; /* unused */
	cs->buf[cs->cdw++] = R_0085F4_CP_COHER_SIZE >> 2; /* dst register */
	cs->buf[cs->cdw++] = 0; /* unused */

	cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONFIG_REG, 1, 0);
	cs->buf[cs->cdw++] = (R_0085F0_CP_COHER_CNTL - SI_CONFIG_REG_OFFSET) >> 2;
	cs->buf[cs->cdw++] = S_0085F0_SO0_DEST_BASE_ENA(1) << t->so_index;

	cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONFIG_REG, 1, 0);
	cs->buf[cs->cdw++] = (R_0085F8_CP_COHER_BASE - SI_CONFIG_REG_OFFSET) >> 2;
	cs->buf[cs->cdw++] = t->b.buffer_offset >> 2;

	cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
	cs->buf[cs->cdw++] = r600_context_bo_reloc(ctx, (struct si_resource*)t->b.buffer,
							     RADEON_USAGE_WRITE);

	cs->buf[cs->cdw++] = PKT3(PKT3_WAIT_REG_MEM, 5, 0);
	cs->buf[cs->cdw++] = WAIT_REG_MEM_EQUAL; /* wait until the register is equal to the reference value */
	cs->buf[cs->cdw++] = R_0085FC_CP_COHER_STATUS >> 2;  /* register */
	cs->buf[cs->cdw++] = 0;
	cs->buf[cs->cdw++] = 0; /* reference value */
	cs->buf[cs->cdw++] = 0xffffffff; /* mask */
	cs->buf[cs->cdw++] = 4; /* poll interval */
#endif
}
