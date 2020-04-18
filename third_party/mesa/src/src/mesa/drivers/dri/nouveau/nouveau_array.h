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

#ifndef __NOUVEAU_ARRAY_H__
#define __NOUVEAU_ARRAY_H__

struct nouveau_array;

typedef unsigned (*extract_u_t)(struct nouveau_array *, int, int);
typedef float (*extract_f_t)(struct nouveau_array *, int, int);

struct nouveau_array {
	int attr;
	int stride, fields, type;

	struct nouveau_bo *bo;
	unsigned offset;
	const void *buf;

	extract_u_t extract_u;
	extract_f_t extract_f;
};

void
nouveau_init_array(struct nouveau_array *a, int attr, int stride,
		   int fields, int type, struct gl_buffer_object *obj,
		   const void *ptr, GLboolean map, struct gl_context *ctx);

void
nouveau_deinit_array(struct nouveau_array *a);

void
nouveau_cleanup_array(struct nouveau_array *a);

#endif
