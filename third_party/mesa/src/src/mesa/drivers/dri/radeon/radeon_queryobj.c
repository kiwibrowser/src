/*
 * Copyright © 2008-2009 Maciej Cencora <m.cencora@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Maciej Cencora <m.cencora@gmail.com>
 *
 */
#include "radeon_common.h"
#include "radeon_queryobj.h"
#include "radeon_debug.h"

#include "main/imports.h"
#include "main/simple_list.h"

#include <inttypes.h>

static void radeonQueryGetResult(struct gl_context *ctx, struct gl_query_object *q)
{
	struct radeon_query_object *query = (struct radeon_query_object *)q;
        uint32_t *result;
	int i;

	radeon_print(RADEON_STATE, RADEON_VERBOSE,
			"%s: query id %d, result %d\n",
			__FUNCTION__, query->Base.Id, (int) query->Base.Result);

	radeon_bo_map(query->bo, GL_FALSE);
        result = query->bo->ptr;

	query->Base.Result = 0;
	for (i = 0; i < query->curr_offset/sizeof(uint32_t); ++i) {
		query->Base.Result += LE32_TO_CPU(result[i]);
		radeon_print(RADEON_STATE, RADEON_TRACE, "result[%d] = %d\n", i, LE32_TO_CPU(result[i]));
	}

	radeon_bo_unmap(query->bo);
}

static struct gl_query_object * radeonNewQueryObject(struct gl_context *ctx, GLuint id)
{
	struct radeon_query_object *query;

	query = calloc(1, sizeof(struct radeon_query_object));

	query->Base.Id = id;
	query->Base.Result = 0;
	query->Base.Active = GL_FALSE;
	query->Base.Ready = GL_TRUE;

	radeon_print(RADEON_STATE, RADEON_VERBOSE,"%s: query id %d\n", __FUNCTION__, query->Base.Id);

	return &query->Base;
}

static void radeonDeleteQuery(struct gl_context *ctx, struct gl_query_object *q)
{
	struct radeon_query_object *query = (struct radeon_query_object *)q;

	radeon_print(RADEON_STATE, RADEON_NORMAL, "%s: query id %d\n", __FUNCTION__, q->Id);

	if (query->bo) {
		radeon_bo_unref(query->bo);
	}

	free(query);
}

static void radeonWaitQuery(struct gl_context *ctx, struct gl_query_object *q)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	struct radeon_query_object *query = (struct radeon_query_object *)q;

	/* If the cmdbuf with packets for this query hasn't been flushed yet, do it now */
	if (radeon_bo_is_referenced_by_cs(query->bo, radeon->cmdbuf.cs))
		ctx->Driver.Flush(ctx);

	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s: query id %d, bo %p, offset %d\n", __FUNCTION__, q->Id, query->bo, query->curr_offset);

	radeonQueryGetResult(ctx, q);

	query->Base.Ready = GL_TRUE;
}


static void radeonBeginQuery(struct gl_context *ctx, struct gl_query_object *q)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	struct radeon_query_object *query = (struct radeon_query_object *)q;

	radeon_print(RADEON_STATE, RADEON_NORMAL, "%s: query id %d\n", __FUNCTION__, q->Id);

	assert(radeon->query.current == NULL);

	if (radeon->dma.flush)
		radeon->dma.flush(radeon->glCtx);

	if (!query->bo) {
		query->bo = radeon_bo_open(radeon->radeonScreen->bom, 0, RADEON_QUERY_PAGE_SIZE, RADEON_QUERY_PAGE_SIZE, RADEON_GEM_DOMAIN_GTT, 0);
	}
	query->curr_offset = 0;

	radeon->query.current = query;

	radeon->query.queryobj.dirty = GL_TRUE;
	radeon->hw.is_dirty = GL_TRUE;
}

void radeonEmitQueryEnd(struct gl_context *ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	struct radeon_query_object *query = radeon->query.current;

	if (!query)
		return;

	if (query->emitted_begin == GL_FALSE)
		return;

	radeon_print(RADEON_STATE, RADEON_NORMAL, "%s: query id %d, bo %p, offset %d\n", __FUNCTION__, query->Base.Id, query->bo, query->curr_offset);

	radeon_cs_space_check_with_bo(radeon->cmdbuf.cs,
				      query->bo,
				      0, RADEON_GEM_DOMAIN_GTT);

	radeon->vtbl.emit_query_finish(radeon);
}

static void radeonEndQuery(struct gl_context *ctx, struct gl_query_object *q)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);

	radeon_print(RADEON_STATE, RADEON_NORMAL, "%s: query id %d\n", __FUNCTION__, q->Id);

	if (radeon->dma.flush)
		radeon->dma.flush(radeon->glCtx);
	radeonEmitQueryEnd(ctx);

	radeon->query.current = NULL;
}

static void radeonCheckQuery(struct gl_context *ctx, struct gl_query_object *q)
{
	radeon_print(RADEON_STATE, RADEON_TRACE, "%s: query id %d\n", __FUNCTION__, q->Id);
\
#ifdef DRM_RADEON_GEM_BUSY
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);

	struct radeon_query_object *query = (struct radeon_query_object *)q;
	uint32_t domain;

	/* Need to perform a flush, as per ARB_occlusion_query spec */
	if (radeon_bo_is_referenced_by_cs(query->bo, radeon->cmdbuf.cs)) {
		ctx->Driver.Flush(ctx);
	}

	if (radeon_bo_is_busy(query->bo, &domain) == 0) {
		radeonQueryGetResult(ctx, q);
		query->Base.Ready = GL_TRUE;
	}
#else
	radeonWaitQuery(ctx, q);
#endif
}

void radeonInitQueryObjFunctions(struct dd_function_table *functions)
{
	functions->NewQueryObject = radeonNewQueryObject;
	functions->DeleteQuery = radeonDeleteQuery;
	functions->BeginQuery = radeonBeginQuery;
	functions->EndQuery = radeonEndQuery;
	functions->CheckQuery = radeonCheckQuery;
	functions->WaitQuery = radeonWaitQuery;
}

int radeon_check_query_active(struct gl_context *ctx, struct radeon_state_atom *atom)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	struct radeon_query_object *query = radeon->query.current;

	if (!query || query->emitted_begin)
		return 0;
	return atom->cmd_size;
}

void radeon_emit_queryobj(struct gl_context *ctx, struct radeon_state_atom *atom)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	BATCH_LOCALS(radeon);
	int dwords;

	dwords = (*atom->check) (ctx, atom);

	BEGIN_BATCH_NO_AUTOSTATE(dwords);
	OUT_BATCH_TABLE(atom->cmd, dwords);
	END_BATCH();

	radeon->query.current->emitted_begin = GL_TRUE;
}
