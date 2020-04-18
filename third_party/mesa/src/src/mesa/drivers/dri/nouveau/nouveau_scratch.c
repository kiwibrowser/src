/*
 * Copyright (C) 2009-2010 Francisco Jerez.
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

#include "nouveau_driver.h"
#include "nouveau_context.h"

/*
 * Returns a pointer to a chunk of 'size' bytes long GART memory. 'bo'
 * and 'offset' will point to the returned memory.
 */
void *
nouveau_get_scratch(struct gl_context *ctx, unsigned size,
		    struct nouveau_bo **bo, unsigned *offset)
{
	struct nouveau_client *client = context_client(ctx);
	struct nouveau_scratch_state *scratch =
		&to_nouveau_context(ctx)->scratch;
	void *buf;

	if (scratch->buf && size <= NOUVEAU_SCRATCH_SIZE - scratch->offset) {
		nouveau_bo_ref(scratch->bo[scratch->index], bo);

		buf = scratch->buf + scratch->offset;
		*offset = scratch->offset;
		scratch->offset += size;

	} else if (size <= NOUVEAU_SCRATCH_SIZE) {
		scratch->index = (scratch->index + 1) % NOUVEAU_SCRATCH_COUNT;
		nouveau_bo_ref(scratch->bo[scratch->index], bo);

		nouveau_bo_map(*bo, NOUVEAU_BO_WR, client);
		buf = scratch->buf = (*bo)->map;

		*offset = 0;
		scratch->offset = size;

	} else {
		nouveau_bo_new(context_dev(ctx), NOUVEAU_BO_GART |
			       NOUVEAU_BO_MAP, 0, size, NULL, bo);

		nouveau_bo_map(*bo, NOUVEAU_BO_WR, client);
		buf = (*bo)->map;

		*offset = 0;
	}

	return buf;
}

void
nouveau_scratch_init(struct gl_context *ctx)
{
	struct nouveau_scratch_state *scratch =
		&to_nouveau_context(ctx)->scratch;
	int ret, i;

	for (i = 0; i < NOUVEAU_SCRATCH_COUNT; i++) {
		ret = nouveau_bo_new(context_dev(ctx), NOUVEAU_BO_GART |
				     NOUVEAU_BO_MAP, 0, NOUVEAU_SCRATCH_SIZE,
				     NULL, &scratch->bo[i]);
		assert(!ret);
	}
}

void
nouveau_scratch_destroy(struct gl_context *ctx)
{
	struct nouveau_scratch_state *scratch =
		&to_nouveau_context(ctx)->scratch;
	int i;

	for (i = 0; i < NOUVEAU_SCRATCH_COUNT; i++)
		nouveau_bo_ref(NULL, &scratch->bo[i]);
}
