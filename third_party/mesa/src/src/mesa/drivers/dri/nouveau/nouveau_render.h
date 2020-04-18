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

#ifndef __NOUVEAU_RENDER_H__
#define __NOUVEAU_RENDER_H__

#include "vbo/vbo_context.h"
#include "nouveau_array.h"

typedef void (*dispatch_t)(struct gl_context *, unsigned int, int, unsigned int);
typedef void (*emit_t)(struct gl_context *, struct nouveau_array *, const void *);

struct nouveau_attr_info {
	int vbo_index;
	int imm_method;
	int imm_fields;

	emit_t emit;
};

struct nouveau_swtnl_state {
	struct nouveau_bo *vbo;
	unsigned offset;
	void *buf;
	unsigned vertex_count;
	GLenum primitive;
};

struct nouveau_render_state {
	enum {
		VBO,
		IMM
	} mode;

	struct nouveau_array ib;
	struct nouveau_array attrs[VERT_ATTRIB_MAX];

	/* Maps a HW VBO index or IMM emission order to an index in
	 * the attrs array above (or -1 if unused). */
	int map[VERT_ATTRIB_MAX];

	int attr_count;
	int vertex_size;

	struct nouveau_swtnl_state swtnl;
};

#define to_render_state(ctx) (&to_nouveau_context(ctx)->render)

#define FOR_EACH_ATTR(render, i, attr)					\
	for (i = 0; attr = (render)->map[i], i < NUM_VERTEX_ATTRS; i++)

#define FOR_EACH_BOUND_ATTR(render, i, attr)				\
	for (i = 0; attr = (render)->map[i], i < render->attr_count; i++) \
		if (attr >= 0)

#endif
