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
#include "radeonsi_pipe.h"
#include "sid.h"

static struct pipe_query *r600_create_query(struct pipe_context *ctx, unsigned query_type)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	return (struct pipe_query*)r600_context_query_create(rctx, query_type);
}

static void r600_destroy_query(struct pipe_context *ctx, struct pipe_query *query)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	r600_context_query_destroy(rctx, (struct r600_query *)query);
}

static void r600_begin_query(struct pipe_context *ctx, struct pipe_query *query)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_query *rquery = (struct r600_query *)query;

	memset(&rquery->result, 0, sizeof(rquery->result));
	rquery->results_start = rquery->results_end;
	r600_query_begin(rctx, (struct r600_query *)query);
	LIST_ADDTAIL(&rquery->list, &rctx->active_query_list);
}

static void r600_end_query(struct pipe_context *ctx, struct pipe_query *query)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_query *rquery = (struct r600_query *)query;

	r600_query_end(rctx, rquery);
	LIST_DELINIT(&rquery->list);
}

static boolean r600_get_query_result(struct pipe_context *ctx,
					struct pipe_query *query,
					boolean wait, union pipe_query_result *vresult)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_query *rquery = (struct r600_query *)query;

	return r600_context_query_result(rctx, rquery, wait, vresult);
}

static void r600_render_condition(struct pipe_context *ctx,
				  struct pipe_query *query,
				  uint mode)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_query *rquery = (struct r600_query *)query;
	int wait_flag = 0;

	/* If we already have nonzero result, render unconditionally */
	if (query != NULL && rquery->result.u64 != 0) {
		if (rctx->current_render_cond) {
			r600_render_condition(ctx, NULL, 0);
		}
		return;
	}

	rctx->current_render_cond = query;
	rctx->current_render_cond_mode = mode;

	if (query == NULL) {
		if (rctx->predicate_drawing) {
			rctx->predicate_drawing = false;
			r600_query_predication(rctx, NULL, PREDICATION_OP_CLEAR, 1);
		}
		return;
	}

	if (mode == PIPE_RENDER_COND_WAIT ||
	    mode == PIPE_RENDER_COND_BY_REGION_WAIT) {
		wait_flag = 1;
	}

	rctx->predicate_drawing = true;

	switch (rquery->type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		r600_query_predication(rctx, rquery, PREDICATION_OP_ZPASS, wait_flag);
		break;
	case PIPE_QUERY_PRIMITIVES_EMITTED:
	case PIPE_QUERY_PRIMITIVES_GENERATED:
	case PIPE_QUERY_SO_STATISTICS:
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		r600_query_predication(rctx, rquery, PREDICATION_OP_PRIMCOUNT, wait_flag);
		break;
	default:
		assert(0);
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
