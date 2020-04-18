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

#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "tnl/t_vertex.h"

#define SWTNL_VBO_SIZE 65536

static enum tnl_attr_format
swtnl_get_format(int type, int fields) {
	switch (type) {
	case GL_FLOAT:
		switch (fields){
		case 1:
			return EMIT_1F;
		case 2:
			return EMIT_2F;
		case 3:
			return EMIT_3F;
		case 4:
			return EMIT_4F;
		default:
			assert(0);
		}
	case GL_UNSIGNED_BYTE:
		switch (fields) {
		case 4:
			return EMIT_4UB_4F_RGBA;
		default:
			assert(0);
		}
	default:
		assert(0);
	}
}

static struct swtnl_attr_info {
	int type;
	int fields;
} swtnl_attrs[VERT_ATTRIB_MAX] = {
	[VERT_ATTRIB_POS] = {
		.type = GL_FLOAT,
		.fields = 4,
	},
	[VERT_ATTRIB_NORMAL] = {
		.type = GL_FLOAT,
		.fields = -1,
	},
	[VERT_ATTRIB_COLOR0] = {
		.type = GL_UNSIGNED_BYTE,
		.fields = 4,
	},
	[VERT_ATTRIB_COLOR1] = {
		.type = GL_UNSIGNED_BYTE,
		.fields = 4,
	},
	[VERT_ATTRIB_FOG] = {
		.type = GL_FLOAT,
		.fields = 1,
	},
	[VERT_ATTRIB_TEX0] = {
		.type = GL_FLOAT,
		.fields = -1,
	},
	[VERT_ATTRIB_TEX1] = {
		.type = GL_FLOAT,
		.fields = -1,
	},
	[VERT_ATTRIB_TEX2] = {
		.type = GL_FLOAT,
		.fields = -1,
	},
	[VERT_ATTRIB_TEX3] = {
		.type = GL_FLOAT,
		.fields = -1,
	},
};

static void
swtnl_choose_attrs(struct gl_context *ctx)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct tnl_clipspace *vtx = &tnl->clipspace;
	static struct tnl_attr_map map[NUM_VERTEX_ATTRS];
	int fields, attr, i, n = 0;

	render->mode = VBO;
	render->attr_count = NUM_VERTEX_ATTRS;

	/* We always want non Ndc coords format */
	tnl->vb.AttribPtr[VERT_ATTRIB_POS] = tnl->vb.ClipPtr;

	for (i = 0; i < VERT_ATTRIB_MAX; i++) {
		struct nouveau_attr_info *ha = &TAG(vertex_attrs)[i];
		struct swtnl_attr_info *sa = &swtnl_attrs[i];
		struct nouveau_array *a = &render->attrs[i];

		if (!sa->fields)
			continue; /* Unsupported attribute. */

		if (tnl->render_inputs_bitset & BITFIELD64_BIT(i)) {
			if (sa->fields > 0)
				fields = sa->fields;
			else
				fields = tnl->vb.AttribPtr[i]->size;

			map[n++] = (struct tnl_attr_map) {
				.attrib = i,
				.format = swtnl_get_format(sa->type, fields),
			};

			render->map[ha->vbo_index] = i;
			a->attr = i;
			a->fields = fields;
			a->type = sa->type;
		}
	}

	_tnl_install_attrs(ctx, map, n, NULL, 0);

	FOR_EACH_BOUND_ATTR(render, i, attr)
		render->attrs[attr].stride = vtx->vertex_size;

	TAG(render_set_format)(ctx);
}

static void
swtnl_alloc_vertices(struct gl_context *ctx)
{
	struct nouveau_swtnl_state *swtnl = &to_render_state(ctx)->swtnl;

	nouveau_bo_ref(NULL, &swtnl->vbo);
	swtnl->buf = nouveau_get_scratch(ctx, SWTNL_VBO_SIZE, &swtnl->vbo,
					 &swtnl->offset);
	swtnl->vertex_count = 0;
}

static void
swtnl_bind_vertices(struct gl_context *ctx)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	struct nouveau_swtnl_state *swtnl = &render->swtnl;
	struct tnl_clipspace *vtx = &TNL_CONTEXT(ctx)->clipspace;
	int i;

	for (i = 0; i < vtx->attr_count; i++) {
		struct tnl_clipspace_attr *ta = &vtx->attr[i];
		struct nouveau_array *a = &render->attrs[ta->attrib];

		nouveau_bo_ref(swtnl->vbo, &a->bo);
		a->offset = swtnl->offset + ta->vertoffset;
	}

	TAG(render_bind_vertices)(ctx);
}

static void
swtnl_unbind_vertices(struct gl_context *ctx)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	int i, attr;

	TAG(render_release_vertices)(ctx);

	FOR_EACH_BOUND_ATTR(render, i, attr) {
		nouveau_bo_ref(NULL, &render->attrs[attr].bo);
		render->map[i] = -1;
	}

	render->attr_count = 0;
}

static void
swtnl_flush_vertices(struct gl_context *ctx)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	struct nouveau_swtnl_state *swtnl = &to_render_state(ctx)->swtnl;
	unsigned npush, start = 0, count = swtnl->vertex_count;
	RENDER_LOCALS(ctx);

	swtnl_bind_vertices(ctx);

	while (count) {
		npush = get_max_vertices(ctx, NULL, PUSH_AVAIL(push));
		npush = MIN2(npush / 12 * 12, count);
		count -= npush;

		if (!npush) {
			PUSH_KICK(push);
			continue;
		}

		BATCH_BEGIN(nvgl_primitive(swtnl->primitive));
		EMIT_VBO(L, ctx, start, 0, npush);
		BATCH_END();

		PUSH_KICK(push);
	}

	swtnl_alloc_vertices(ctx);
}

/* TnL renderer entry points */

static void
swtnl_start(struct gl_context *ctx)
{
	swtnl_choose_attrs(ctx);
}

static void
swtnl_finish(struct gl_context *ctx)
{
	swtnl_flush_vertices(ctx);
	swtnl_unbind_vertices(ctx);
}

static void
swtnl_primitive(struct gl_context *ctx, GLenum mode)
{
}

static void
swtnl_reset_stipple(struct gl_context *ctx)
{
}

/* Primitive rendering */

#define BEGIN_PRIMITIVE(p, n)						\
	struct nouveau_swtnl_state *swtnl = &to_render_state(ctx)->swtnl; \
	int vertex_len = TNL_CONTEXT(ctx)->clipspace.vertex_size;	\
									\
	if (swtnl->vertex_count + (n) > SWTNL_VBO_SIZE/vertex_len	\
	    || (swtnl->vertex_count && swtnl->primitive != p))		\
		swtnl_flush_vertices(ctx);				\
									\
	swtnl->primitive = p;

#define OUT_VERTEX(i) do {						\
		memcpy(swtnl->buf + swtnl->vertex_count * vertex_len,	\
		       _tnl_get_vertex(ctx, (i)), vertex_len);		\
		swtnl->vertex_count++;					\
	} while (0)

static void
swtnl_points(struct gl_context *ctx, GLuint first, GLuint last)
{
	int i, count;

	while (first < last) {
		BEGIN_PRIMITIVE(GL_POINTS, last - first);

		count = MIN2(SWTNL_VBO_SIZE / vertex_len, last - first);
		for (i = 0; i < count; i++)
			OUT_VERTEX(first + i);

		first += count;
	}
}

static void
swtnl_line(struct gl_context *ctx, GLuint v1, GLuint v2)
{
	BEGIN_PRIMITIVE(GL_LINES, 2);
	OUT_VERTEX(v1);
	OUT_VERTEX(v2);
}

static void
swtnl_triangle(struct gl_context *ctx, GLuint v1, GLuint v2, GLuint v3)
{
	BEGIN_PRIMITIVE(GL_TRIANGLES, 3);
	OUT_VERTEX(v1);
	OUT_VERTEX(v2);
	OUT_VERTEX(v3);
}

static void
swtnl_quad(struct gl_context *ctx, GLuint v1, GLuint v2, GLuint v3, GLuint v4)
{
	BEGIN_PRIMITIVE(GL_QUADS, 4);
	OUT_VERTEX(v1);
	OUT_VERTEX(v2);
	OUT_VERTEX(v3);
	OUT_VERTEX(v4);
}

/* TnL initialization. */
void
TAG(swtnl_init)(struct gl_context *ctx)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);

	tnl->Driver.RunPipeline = _tnl_run_pipeline;
	tnl->Driver.Render.Interp = _tnl_interp;
	tnl->Driver.Render.CopyPV = _tnl_copy_pv;
	tnl->Driver.Render.ClippedPolygon = _tnl_RenderClippedPolygon;
	tnl->Driver.Render.ClippedLine = _tnl_RenderClippedLine;
	tnl->Driver.Render.BuildVertices = _tnl_build_vertices;

	tnl->Driver.Render.Start = swtnl_start;
	tnl->Driver.Render.Finish = swtnl_finish;
	tnl->Driver.Render.PrimitiveNotify = swtnl_primitive;
	tnl->Driver.Render.ResetLineStipple = swtnl_reset_stipple;

	tnl->Driver.Render.Points = swtnl_points;
	tnl->Driver.Render.Line = swtnl_line;
	tnl->Driver.Render.Triangle = swtnl_triangle;
	tnl->Driver.Render.Quad = swtnl_quad;

	_tnl_init_vertices(ctx, tnl->vb.Size,
			   NUM_VERTEX_ATTRS * 4 * sizeof(GLfloat));
	_tnl_need_projected_coords(ctx, GL_FALSE);
	_tnl_allow_vertex_fog(ctx, GL_FALSE);
	_tnl_wakeup(ctx);

	swtnl_alloc_vertices(ctx);
}

void
TAG(swtnl_destroy)(struct gl_context *ctx)
{
	nouveau_bo_ref(NULL, &to_render_state(ctx)->swtnl.vbo);
}
