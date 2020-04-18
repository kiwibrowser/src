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

#ifndef __NOUVEAU_SCRATCH_H__
#define __NOUVEAU_SCRATCH_H__

#define NOUVEAU_SCRATCH_COUNT 2
#define NOUVEAU_SCRATCH_SIZE 3*1024*1024

struct nouveau_scratch_state {
	struct nouveau_bo *bo[NOUVEAU_SCRATCH_COUNT];

	int index;
	int offset;
	void *buf;
};

void *
nouveau_get_scratch(struct gl_context *ctx, unsigned size,
		    struct nouveau_bo **bo, unsigned *offset);

void
nouveau_scratch_init(struct gl_context *ctx);

void
nouveau_scratch_destroy(struct gl_context *ctx);

#endif
