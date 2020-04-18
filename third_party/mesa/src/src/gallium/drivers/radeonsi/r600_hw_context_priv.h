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

#include "radeonsi_pipe.h"
#include "util/u_hash_table.h"
#include "os/os_thread.h"

#define SI_MAX_DRAW_CS_DWORDS 18

#define PKT_COUNT_C                     0xC000FFFF
#define PKT_COUNT_S(x)                  (((x) & 0x3FFF) << 16)

static INLINE unsigned r600_context_bo_reloc(struct r600_context *ctx, struct si_resource *rbo,
					     enum radeon_bo_usage usage)
{
	assert(usage);
	return ctx->ws->cs_add_reloc(ctx->cs, rbo->cs_buf, usage, rbo->domains) * 4;
}

#endif
