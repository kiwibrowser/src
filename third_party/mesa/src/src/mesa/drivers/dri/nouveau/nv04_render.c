/*
 * Copyright (C) 2009 Francisco Jerez.
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
#include "nouveau_util.h"
#include "nv04_3d.xml.h"
#include "nv04_driver.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "tnl/t_vertex.h"

#define NUM_VERTEX_ATTRS 6

static void
swtnl_update_viewport(struct gl_context *ctx)
{
	float *viewport = to_nv04_context(ctx)->viewport;
	struct gl_framebuffer *fb = ctx->DrawBuffer;

	get_viewport_scale(ctx, viewport);
	get_viewport_translate(ctx, &viewport[MAT_TX]);

	/* It wants normalized Z coordinates. */
	viewport[MAT_SZ] /= fb->_DepthMaxF;
	viewport[MAT_TZ] /= fb->_DepthMaxF;
}

static void
swtnl_emit_attr(struct gl_context *ctx, struct tnl_attr_map *m, int attr, int emit)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);

	if (tnl->render_inputs_bitset & BITFIELD64_BIT(attr))
		*m = (struct tnl_attr_map) {
			.attrib = attr,
			.format = emit,
		};
	else
		*m = (struct tnl_attr_map) {
			.format = EMIT_PAD,
			.offset = _tnl_format_info[emit].attrsize,
		};
}

static void
swtnl_choose_attrs(struct gl_context *ctx)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct nouveau_object *fahrenheit = nv04_context_engine(ctx);
	struct nv04_context *nctx = to_nv04_context(ctx);
	static struct tnl_attr_map map[NUM_VERTEX_ATTRS];
	int n = 0;

	tnl->vb.AttribPtr[VERT_ATTRIB_POS] = tnl->vb.NdcPtr;

	swtnl_emit_attr(ctx, &map[n++], _TNL_ATTRIB_POS, EMIT_4F_VIEWPORT);
	swtnl_emit_attr(ctx, &map[n++], _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_BGRA);
	swtnl_emit_attr(ctx, &map[n++], _TNL_ATTRIB_COLOR1, EMIT_3UB_3F_BGR);
	swtnl_emit_attr(ctx, &map[n++], _TNL_ATTRIB_FOG, EMIT_1UB_1F);
	swtnl_emit_attr(ctx, &map[n++], _TNL_ATTRIB_TEX0, EMIT_2F);
	if (nv04_mtex_engine(fahrenheit))
		swtnl_emit_attr(ctx, &map[n++], _TNL_ATTRIB_TEX1, EMIT_2F);

	swtnl_update_viewport(ctx);

	_tnl_install_attrs(ctx, map, n, nctx->viewport, 0);
}

/* TnL renderer entry points */

static void
swtnl_restart_ttri(struct nv04_context *nv04, struct nouveau_pushbuf *push)
{
	BEGIN_NV04(push, NV04_TTRI(COLORKEY), 7);
	PUSH_DATA (push, nv04->colorkey);
	PUSH_RELOC(push, nv04->texture[0]->bo, nv04->texture[0]->offset,
			 NOUVEAU_BO_LOW, 0, 0);
	PUSH_RELOC(push, nv04->texture[0]->bo, nv04->format[0], NOUVEAU_BO_OR,
			 NV04_TEXTURED_TRIANGLE_FORMAT_DMA_A,
			 NV04_TEXTURED_TRIANGLE_FORMAT_DMA_B);
	PUSH_DATA (push, nv04->filter[0]);
	PUSH_DATA (push, nv04->blend);
	PUSH_DATA (push, nv04->ctrl[0] & ~0x3e000000);
	PUSH_DATA (push, nv04->fog);
}

static void
swtnl_restart_mtri(struct nv04_context *nv04, struct nouveau_pushbuf *push)
{
	BEGIN_NV04(push, NV04_MTRI(OFFSET(0)), 8);
	PUSH_RELOC(push, nv04->texture[0]->bo, nv04->texture[0]->offset,
			 NOUVEAU_BO_LOW, 0, 0);
	PUSH_RELOC(push, nv04->texture[1]->bo, nv04->texture[1]->offset,
			 NOUVEAU_BO_LOW, 0, 0);
	PUSH_RELOC(push, nv04->texture[0]->bo, nv04->format[0], NOUVEAU_BO_OR,
			 NV04_TEXTURED_TRIANGLE_FORMAT_DMA_A,
			 NV04_TEXTURED_TRIANGLE_FORMAT_DMA_B);
	PUSH_RELOC(push, nv04->texture[1]->bo, nv04->format[1], NOUVEAU_BO_OR,
			 NV04_TEXTURED_TRIANGLE_FORMAT_DMA_A,
			 NV04_TEXTURED_TRIANGLE_FORMAT_DMA_B);
	PUSH_DATA (push, nv04->filter[0]);
	PUSH_DATA (push, nv04->filter[1]);
	PUSH_DATA (push, nv04->alpha[0]);
	PUSH_DATA (push, nv04->color[0]);
	BEGIN_NV04(push, NV04_MTRI(COMBINE_ALPHA(1)), 8);
	PUSH_DATA (push, nv04->alpha[1]);
	PUSH_DATA (push, nv04->color[1]);
	PUSH_DATA (push, nv04->factor);
	PUSH_DATA (push, nv04->blend & ~0x0000000f);
	PUSH_DATA (push, nv04->ctrl[0]);
	PUSH_DATA (push, nv04->ctrl[1]);
	PUSH_DATA (push, nv04->ctrl[2]);
	PUSH_DATA (push, nv04->fog);
}

static inline bool
swtnl_restart(struct gl_context *ctx, int multi, unsigned vertex_size)
{
	const int tex_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD;
	struct nv04_context *nv04 = to_nv04_context(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	struct nouveau_pushbuf_refn refs[] = {
		{ nv04->texture[0]->bo, tex_flags },
		{ nv04->texture[1]->bo, tex_flags },
	};

	/* wait for enough space for state, and at least one whole primitive */
	if (nouveau_pushbuf_space(push, 32 + (4 * vertex_size), 4, 0) ||
	    nouveau_pushbuf_refn (push, refs, multi ? 2 : 1))
		return false;

	/* emit engine state */
	if (multi)
		swtnl_restart_mtri(nv04, push);
	else
		swtnl_restart_ttri(nv04, push);

	return true;
}

static void
swtnl_start(struct gl_context *ctx)
{
	struct nouveau_object *eng3d = nv04_context_engine(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	unsigned vertex_size;

	nouveau_pushbuf_bufctx(push, push->user_priv);
	nouveau_pushbuf_validate(push);

	swtnl_choose_attrs(ctx);

	vertex_size = TNL_CONTEXT(ctx)->clipspace.vertex_size / 4;
	if (eng3d->oclass == NV04_MULTITEX_TRIANGLE_CLASS)
		swtnl_restart(ctx, 1, vertex_size);
	else
		swtnl_restart(ctx, 0, vertex_size);
}

static void
swtnl_finish(struct gl_context *ctx)
{
	struct nouveau_pushbuf *push = context_push(ctx);

	nouveau_pushbuf_bufctx(push, NULL);
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

#define BEGIN_PRIMITIVE(n)						\
	struct nouveau_object *eng3d = to_nv04_context(ctx)->eng3d;	\
	struct nouveau_pushbuf *push = context_push(ctx);		\
	int vertex_size = TNL_CONTEXT(ctx)->clipspace.vertex_size / 4;	\
	int multi = (eng3d->oclass == NV04_MULTITEX_TRIANGLE_CLASS);	\
									\
	if (PUSH_AVAIL(push) < 32 + (n * vertex_size)) {		\
		if (!swtnl_restart(ctx, multi, vertex_size))		\
			return;						\
	}								\
									\
	BEGIN_NV04(push, NV04_TTRI(TLVERTEX_SX(0)), n * vertex_size);

#define OUT_VERTEX(i)							\
	PUSH_DATAp(push, _tnl_get_vertex(ctx, i), vertex_size);

#define END_PRIMITIVE(draw)						\
	if (multi) {							\
		BEGIN_NV04(push, NV04_MTRI(DRAWPRIMITIVE(0)), 1);	\
		PUSH_DATA (push, draw);					\
	} else {							\
		BEGIN_NV04(push, NV04_TTRI(DRAWPRIMITIVE(0)), 1);	\
		PUSH_DATA (push, draw);					\
	}

static void
swtnl_points(struct gl_context *ctx, GLuint first, GLuint last)
{
}

static void
swtnl_line(struct gl_context *ctx, GLuint v1, GLuint v2)
{
}

static void
swtnl_triangle(struct gl_context *ctx, GLuint v1, GLuint v2, GLuint v3)
{
	BEGIN_PRIMITIVE(3);
	OUT_VERTEX(v1);
	OUT_VERTEX(v2);
	OUT_VERTEX(v3);
	END_PRIMITIVE(0x102);
}

static void
swtnl_quad(struct gl_context *ctx, GLuint v1, GLuint v2, GLuint v3, GLuint v4)
{
	BEGIN_PRIMITIVE(4);
	OUT_VERTEX(v1);
	OUT_VERTEX(v2);
	OUT_VERTEX(v3);
	OUT_VERTEX(v4);
	END_PRIMITIVE(0x213103);
}

/* TnL initialization. */
void
nv04_render_init(struct gl_context *ctx)
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

	_tnl_need_projected_coords(ctx, GL_TRUE);
	_tnl_init_vertices(ctx, tnl->vb.Size,
			   NUM_VERTEX_ATTRS * 4 * sizeof(GLfloat));
	_tnl_allow_pixel_fog(ctx, GL_FALSE);
	_tnl_wakeup(ctx);
}

void
nv04_render_destroy(struct gl_context *ctx)
{
}
