/*
 * Copyright © 2008 Maciej Cencora <m.cencora@gmail.com>
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

#include "main/imports.h"
#include "main/simple_list.h"
#include "radeon_common_context.h"

extern void radeonEmitQueryBegin(struct gl_context *ctx);
extern void radeonEmitQueryEnd(struct gl_context *ctx);

extern void radeonInitQueryObjFunctions(struct dd_function_table *functions);

#define RADEON_QUERY_PAGE_SIZE 4096

int radeon_check_query_active(struct gl_context *ctx, struct radeon_state_atom *atom);
void radeon_emit_queryobj(struct gl_context *ctx, struct radeon_state_atom *atom);

static inline void radeon_init_query_stateobj(radeonContextPtr radeon, int SZ)
{
	radeon->query.queryobj.cmd_size = (SZ);
	radeon->query.queryobj.cmd = (uint32_t*)CALLOC((SZ) * sizeof(uint32_t));
	radeon->query.queryobj.name = "queryobj";
	radeon->query.queryobj.idx = 0;
	radeon->query.queryobj.check = radeon_check_query_active;
	radeon->query.queryobj.dirty = GL_FALSE;
	radeon->query.queryobj.emit = radeon_emit_queryobj;

	radeon->hw.max_state_size += (SZ);
	insert_at_tail(&radeon->hw.atomlist, &radeon->query.queryobj);
}

