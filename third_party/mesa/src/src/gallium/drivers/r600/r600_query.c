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
#include "r600_pipe.h"
#include "r600d.h"
#include "util/u_memory.h"

static bool r600_is_timer_query(unsigned type)
{
	return type == PIPE_QUERY_TIME_ELAPSED ||
	       type == PIPE_QUERY_TIMESTAMP ||
	       type == PIPE_QUERY_TIMESTAMP_DISJOINT;
}

static bool r600_query_needs_begin(unsigned type)
{
	return type != PIPE_QUERY_GPU_FINISHED &&
	       type != PIPE_QUERY_TIMESTAMP;
}

static struct r600_resource *r600_new_query_buffer(struct r600_context *ctx, unsigned type)
{
	unsigned j, i, num_results, buf_size = 4096;
	uint32_t *results;
	/* Queries are normally read by the CPU after
	 * being written by the gpu, hence staging is probably a good
	 * usage pattern.
	 */
	struct r600_resource *buf = (struct r600_resource*)
		pipe_buffer_create(&ctx->screen->screen, PIPE_BIND_CUSTOM,
				   PIPE_USAGE_STAGING, buf_size);

	switch (type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		results = ctx->ws->buffer_map(buf->cs_buf, ctx->cs, PIPE_TRANSFER_WRITE);
		memset(results, 0, buf_size);

		/* Set top bits for unused backends. */
		num_results = buf_size / (16 * ctx->max_db);
		for (j = 0; j < num_results; j++) {
			for (i = 0; i < ctx->max_db; i++) {
				if (!(ctx->backend_mask & (1<<i))) {
					results[(i * 4)+1] = 0x80000000;
					results[(i * 4)+3] = 0x80000000;
				}
			}
			results += 4 * ctx->max_db;
		}
		ctx->ws->buffer_unmap(buf->cs_buf);
		break;
	case PIPE_QUERY_TIME_ELAPSED:
	case PIPE_QUERY_TIMESTAMP:
		break;
	case PIPE_QUERY_PRIMITIVES_EMITTED:
	case PIPE_QUERY_PRIMITIVES_GENERATED:
	case PIPE_QUERY_SO_STATISTICS:
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		results = ctx->ws->buffer_map(buf->cs_buf, ctx->cs, PIPE_TRANSFER_WRITE);
		memset(results, 0, buf_size);
		ctx->ws->buffer_unmap(buf->cs_buf);
		break;
	default:
		assert(0);
	}
	return buf;
}

static void r600_emit_query_begin(struct r600_context *ctx, struct r600_query *query)
{
	struct radeon_winsys_cs *cs = ctx->cs;
	uint64_t va;

	r600_need_cs_space(ctx, query->num_cs_dw * 2, TRUE);

	/* Get a new query buffer if needed. */
	if (query->buffer.results_end + query->result_size > query->buffer.buf->b.b.width0) {
		struct r600_query_buffer *qbuf = MALLOC_STRUCT(r600_query_buffer);
		*qbuf = query->buffer;
		query->buffer.buf = r600_new_query_buffer(ctx, query->type);
		query->buffer.results_end = 0;
		query->buffer.previous = qbuf;
	}

	/* emit begin query */
	va = r600_resource_va(&ctx->screen->screen, (void*)query->buffer.buf);
	va += query->buffer.results_end;

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
		cs->buf[cs->cdw++] = va;
		cs->buf[cs->cdw++] = (va >> 32UL) & 0xFF;
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
	cs->buf[cs->cdw++] = r600_context_bo_reloc(ctx, query->buffer.buf, RADEON_USAGE_WRITE);

	if (r600_is_timer_query(query->type)) {
		ctx->num_cs_dw_timer_queries_suspend += query->num_cs_dw;
	} else {
		ctx->num_cs_dw_nontimer_queries_suspend += query->num_cs_dw;
	}
}

static void r600_emit_query_end(struct r600_context *ctx, struct r600_query *query)
{
	struct radeon_winsys_cs *cs = ctx->cs;
	uint64_t va;

	/* The queries which need begin already called this in begin_query. */
	if (!r600_query_needs_begin(query->type)) {
		r600_need_cs_space(ctx, query->num_cs_dw, FALSE);
	}

	va = r600_resource_va(&ctx->screen->screen, (void*)query->buffer.buf);
	/* emit end query */
	switch (query->type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		va += query->buffer.results_end + 8;
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
		cs->buf[cs->cdw++] = query->buffer.results_end + query->result_size/2;
		cs->buf[cs->cdw++] = 0;
		break;
	case PIPE_QUERY_TIME_ELAPSED:
		va += query->buffer.results_end + query->result_size/2;
		/* fall through */
	case PIPE_QUERY_TIMESTAMP:
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
	cs->buf[cs->cdw++] = r600_context_bo_reloc(ctx, query->buffer.buf, RADEON_USAGE_WRITE);

	query->buffer.results_end += query->result_size;

	if (r600_query_needs_begin(query->type)) {
		if (r600_is_timer_query(query->type)) {
			ctx->num_cs_dw_timer_queries_suspend -= query->num_cs_dw;
		} else {
			ctx->num_cs_dw_nontimer_queries_suspend -= query->num_cs_dw;
		}
	}
}

static void r600_emit_query_predication(struct r600_context *ctx, struct r600_query *query,
					int operation, bool flag_wait)
{
	struct radeon_winsys_cs *cs = ctx->cs;

	if (operation == PREDICATION_OP_CLEAR) {
		r600_need_cs_space(ctx, 3, FALSE);

		cs->buf[cs->cdw++] = PKT3(PKT3_SET_PREDICATION, 1, 0);
		cs->buf[cs->cdw++] = 0;
		cs->buf[cs->cdw++] = PRED_OP(PREDICATION_OP_CLEAR);
	} else {
		struct r600_query_buffer *qbuf;
		unsigned count;
		uint32_t op;

		/* Find how many results there are. */
		count = 0;
		for (qbuf = &query->buffer; qbuf; qbuf = qbuf->previous) {
			count += qbuf->results_end / query->result_size;
		}

		r600_need_cs_space(ctx, 5 * count, TRUE);

		op = PRED_OP(operation) | PREDICATION_DRAW_VISIBLE |
				(flag_wait ? PREDICATION_HINT_WAIT : PREDICATION_HINT_NOWAIT_DRAW);

		/* emit predicate packets for all data blocks */
		for (qbuf = &query->buffer; qbuf; qbuf = qbuf->previous) {
			unsigned results_base = 0;
			uint64_t va = r600_resource_va(&ctx->screen->screen, &qbuf->buf->b.b);

			while (results_base < qbuf->results_end) {
				cs->buf[cs->cdw++] = PKT3(PKT3_SET_PREDICATION, 1, 0);
				cs->buf[cs->cdw++] = (va + results_base) & 0xFFFFFFFFUL;
				cs->buf[cs->cdw++] = op | (((va + results_base) >> 32UL) & 0xFF);
				cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
				cs->buf[cs->cdw++] = r600_context_bo_reloc(ctx, qbuf->buf, RADEON_USAGE_READ);
				results_base += query->result_size;

				/* set CONTINUE bit for all packets except the first */
				op |= PREDICATION_CONTINUE;
			}
		} while (qbuf);
	}
}

static struct pipe_query *r600_create_query(struct pipe_context *ctx, unsigned query_type)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	struct r600_query *query;

	query = CALLOC_STRUCT(r600_query);
	if (query == NULL)
		return NULL;

	query->type = query_type;

	switch (query_type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		query->result_size = 16 * rctx->max_db;
		query->num_cs_dw = 6;
		break;
	case PIPE_QUERY_TIME_ELAPSED:
		query->result_size = 16;
		query->num_cs_dw = 8;
		break;
	case PIPE_QUERY_TIMESTAMP:
		query->result_size = 8;
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

	query->buffer.buf = r600_new_query_buffer(rctx, query_type);
	if (!query->buffer.buf) {
		FREE(query);
		return NULL;
	}
	return (struct pipe_query*)query;
}

static void r600_destroy_query(struct pipe_context *ctx, struct pipe_query *query)
{
	struct r600_query *rquery = (struct r600_query*)query;
	struct r600_query_buffer *prev = rquery->buffer.previous;

	/* Release all query buffers. */
	while (prev) {
		struct r600_query_buffer *qbuf = prev;
		prev = prev->previous;
		pipe_resource_reference((struct pipe_resource**)&qbuf->buf, NULL);
		FREE(qbuf);
	}

	pipe_resource_reference((struct pipe_resource**)&rquery->buffer.buf, NULL);
	FREE(query);
}

static void r600_update_occlusion_query_state(struct r600_context *rctx,
					      unsigned type, int diff)
{
	if (type == PIPE_QUERY_OCCLUSION_COUNTER ||
	    type == PIPE_QUERY_OCCLUSION_PREDICATE) {
		bool enable;

		rctx->num_occlusion_queries += diff;
		assert(rctx->num_occlusion_queries >= 0);

		enable = rctx->num_occlusion_queries != 0;

		if (rctx->db_misc_state.occlusion_query_enabled != enable) {
			rctx->db_misc_state.occlusion_query_enabled = enable;
			r600_atom_dirty(rctx, &rctx->db_misc_state.atom);
		}
	}
}

static void r600_begin_query(struct pipe_context *ctx, struct pipe_query *query)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_query *rquery = (struct r600_query *)query;
	struct r600_query_buffer *prev = rquery->buffer.previous;

	if (!r600_query_needs_begin(rquery->type)) {
		assert(0);
		return;
	}

	/* Discard the old query buffers. */
	while (prev) {
		struct r600_query_buffer *qbuf = prev;
		prev = prev->previous;
		pipe_resource_reference((struct pipe_resource**)&qbuf->buf, NULL);
		FREE(qbuf);
	}

	/* Obtain a new buffer if the current one can't be mapped without a stall. */
	if (rctx->ws->cs_is_buffer_referenced(rctx->cs, rquery->buffer.buf->cs_buf, RADEON_USAGE_READWRITE) ||
	    rctx->ws->buffer_is_busy(rquery->buffer.buf->buf, RADEON_USAGE_READWRITE)) {
		pipe_resource_reference((struct pipe_resource**)&rquery->buffer.buf, NULL);
		rquery->buffer.buf = r600_new_query_buffer(rctx, rquery->type);
	}

	rquery->buffer.results_end = 0;
	rquery->buffer.previous = NULL;

	r600_update_occlusion_query_state(rctx, rquery->type, 1);

	r600_emit_query_begin(rctx, rquery);

	if (r600_is_timer_query(rquery->type)) {
		LIST_ADDTAIL(&rquery->list, &rctx->active_timer_queries);
	} else {
		LIST_ADDTAIL(&rquery->list, &rctx->active_nontimer_queries);
	}
}

static void r600_end_query(struct pipe_context *ctx, struct pipe_query *query)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_query *rquery = (struct r600_query *)query;

	r600_emit_query_end(rctx, rquery);

	if (r600_query_needs_begin(rquery->type)) {
		LIST_DELINIT(&rquery->list);
	}

	r600_update_occlusion_query_state(rctx, rquery->type, -1);
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

static boolean r600_get_query_buffer_result(struct r600_context *ctx,
					    struct r600_query *query,
					    struct r600_query_buffer *qbuf,
					    boolean wait,
					    union pipe_query_result *result)
{
	unsigned results_base = 0;
	char *map;

	map = ctx->ws->buffer_map(qbuf->buf->cs_buf, ctx->cs,
				  PIPE_TRANSFER_READ |
				  (wait ? 0 : PIPE_TRANSFER_DONTBLOCK));
	if (!map)
		return FALSE;

	/* count all results across all data blocks */
	switch (query->type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
		while (results_base != qbuf->results_end) {
			result->u64 +=
				r600_query_read_result(map + results_base, 0, 2, true);
			results_base += 16;
		}
		break;
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		while (results_base != qbuf->results_end) {
			result->b = result->b ||
				r600_query_read_result(map + results_base, 0, 2, true) != 0;
			results_base += 16;
		}
		break;
	case PIPE_QUERY_TIME_ELAPSED:
		while (results_base != qbuf->results_end) {
			result->u64 +=
				r600_query_read_result(map + results_base, 0, 2, false);
			results_base += query->result_size;
		}
		break;
	case PIPE_QUERY_TIMESTAMP:
	{
		uint32_t *current_result = (uint32_t*)map;
		result->u64 = (uint64_t)current_result[0] |
			      (uint64_t)current_result[1] << 32;
		break;
	}
	case PIPE_QUERY_PRIMITIVES_EMITTED:
		/* SAMPLE_STREAMOUTSTATS stores this structure:
		 * {
		 *    u64 NumPrimitivesWritten;
		 *    u64 PrimitiveStorageNeeded;
		 * }
		 * We only need NumPrimitivesWritten here. */
		while (results_base != qbuf->results_end) {
			result->u64 +=
				r600_query_read_result(map + results_base, 2, 6, true);
			results_base += query->result_size;
		}
		break;
	case PIPE_QUERY_PRIMITIVES_GENERATED:
		/* Here we read PrimitiveStorageNeeded. */
		while (results_base != qbuf->results_end) {
			result->u64 +=
				r600_query_read_result(map + results_base, 0, 4, true);
			results_base += query->result_size;
		}
		break;
	case PIPE_QUERY_SO_STATISTICS:
		while (results_base != qbuf->results_end) {
			result->so_statistics.num_primitives_written +=
				r600_query_read_result(map + results_base, 2, 6, true);
			result->so_statistics.primitives_storage_needed +=
				r600_query_read_result(map + results_base, 0, 4, true);
			results_base += query->result_size;
		}
		break;
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		while (results_base != qbuf->results_end) {
			result->b = result->b ||
				r600_query_read_result(map + results_base, 2, 6, true) !=
				r600_query_read_result(map + results_base, 0, 4, true);
			results_base += query->result_size;
		}
		break;
	default:
		assert(0);
	}

	ctx->ws->buffer_unmap(qbuf->buf->cs_buf);
	return TRUE;
}

static boolean r600_get_query_result(struct pipe_context *ctx,
					struct pipe_query *query,
					boolean wait, union pipe_query_result *result)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_query *rquery = (struct r600_query *)query;
	struct r600_query_buffer *qbuf;

	util_query_clear_result(result, rquery->type);

	for (qbuf = &rquery->buffer; qbuf; qbuf = qbuf->previous) {
		if (!r600_get_query_buffer_result(rctx, rquery, qbuf, wait, result)) {
			return FALSE;
		}
	}

	/* Convert the time to expected units. */
	if (rquery->type == PIPE_QUERY_TIME_ELAPSED ||
	    rquery->type == PIPE_QUERY_TIMESTAMP) {
		result->u64 = (1000000 * result->u64) / rctx->screen->info.r600_clock_crystal_freq;
	}
	return TRUE;
}

static void r600_render_condition(struct pipe_context *ctx,
				  struct pipe_query *query,
				  uint mode)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_query *rquery = (struct r600_query *)query;
	bool wait_flag = false;

	rctx->current_render_cond = query;
	rctx->current_render_cond_mode = mode;

	if (query == NULL) {
		if (rctx->predicate_drawing) {
			rctx->predicate_drawing = false;
			r600_emit_query_predication(rctx, NULL, PREDICATION_OP_CLEAR, false);
		}
		return;
	}

	if (mode == PIPE_RENDER_COND_WAIT ||
	    mode == PIPE_RENDER_COND_BY_REGION_WAIT) {
		wait_flag = true;
	}

	rctx->predicate_drawing = true;

	switch (rquery->type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		r600_emit_query_predication(rctx, rquery, PREDICATION_OP_ZPASS, wait_flag);
		break;
	case PIPE_QUERY_PRIMITIVES_EMITTED:
	case PIPE_QUERY_PRIMITIVES_GENERATED:
	case PIPE_QUERY_SO_STATISTICS:
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		r600_emit_query_predication(rctx, rquery, PREDICATION_OP_PRIMCOUNT, wait_flag);
		break;
	default:
		assert(0);
	}
}

void r600_suspend_nontimer_queries(struct r600_context *ctx)
{
	struct r600_query *query;

	LIST_FOR_EACH_ENTRY(query, &ctx->active_nontimer_queries, list) {
		r600_emit_query_end(ctx, query);
	}
	assert(ctx->num_cs_dw_nontimer_queries_suspend == 0);
}

void r600_resume_nontimer_queries(struct r600_context *ctx)
{
	struct r600_query *query;

	assert(ctx->num_cs_dw_nontimer_queries_suspend == 0);

	LIST_FOR_EACH_ENTRY(query, &ctx->active_nontimer_queries, list) {
		r600_emit_query_begin(ctx, query);
	}
}

void r600_suspend_timer_queries(struct r600_context *ctx)
{
	struct r600_query *query;

	LIST_FOR_EACH_ENTRY(query, &ctx->active_timer_queries, list) {
		r600_emit_query_end(ctx, query);
	}

	assert(ctx->num_cs_dw_timer_queries_suspend == 0);
}

void r600_resume_timer_queries(struct r600_context *ctx)
{
	struct r600_query *query;

	assert(ctx->num_cs_dw_timer_queries_suspend == 0);

	LIST_FOR_EACH_ENTRY(query, &ctx->active_timer_queries, list) {
		r600_emit_query_begin(ctx, query);
	}
}

void r600_init_query_functions(struct r600_context *rctx)
{
	rctx->context.create_query = r600_create_query;
	rctx->context.destroy_query = r600_destroy_query;
	rctx->context.begin_query = r600_begin_query;
	rctx->context.end_query = r600_end_query;
	rctx->context.get_query_result = r600_get_query_result;

	if (rctx->screen->info.r600_num_backends > 0)
	    rctx->context.render_condition = r600_render_condition;
}
