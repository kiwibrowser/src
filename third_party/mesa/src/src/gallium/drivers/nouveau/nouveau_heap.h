/*
 * Copyright 2007 Nouveau Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __NOUVEAU_HEAP_H__
#define __NOUVEAU_HEAP_H__

struct nouveau_heap {
	struct nouveau_heap *prev;
	struct nouveau_heap *next;

	void *priv;

	unsigned start;
	unsigned size;

	int in_use;
};

int
nouveau_heap_init(struct nouveau_heap **heap, unsigned start,
                  unsigned size);

void
nouveau_heap_destroy(struct nouveau_heap **heap);

int
nouveau_heap_alloc(struct nouveau_heap *heap, unsigned size, void *priv,
                   struct nouveau_heap **);

void
nouveau_heap_free(struct nouveau_heap **);

#endif
