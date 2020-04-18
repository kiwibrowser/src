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
#include "nv20_3d.xml.h"
#include "nv20_driver.h"

#define NUM_VERTEX_ATTRS 16

static void
nv20_emit_material(struct gl_context *ctx, struct nouveau_array *a,
		   const void *v);

/* Vertex attribute format. */
static struct nouveau_attr_info nv20_vertex_attrs[VERT_ATTRIB_MAX] = {
	[VERT_ATTRIB_POS] = {
		.vbo_index = 0,
		.imm_method = NV20_3D_VERTEX_POS_4F_X,
		.imm_fields = 4,
	},
	[VERT_ATTRIB_NORMAL] = {
		.vbo_index = 2,
		.imm_method = NV20_3D_VERTEX_NOR_3F_X,
		.imm_fields = 3,
	},
	[VERT_ATTRIB_COLOR0] = {
		.vbo_index = 3,
		.imm_method = NV20_3D_VERTEX_COL_4F,
		.imm_fields = 4,
	},
	[VERT_ATTRIB_COLOR1] = {
		.vbo_index = 4,
		.imm_method = NV20_3D_VERTEX_COL2_3F,
		.imm_fields = 3,
	},
	[VERT_ATTRIB_FOG] = {
		.vbo_index = 5,
		.imm_method = NV20_3D_VERTEX_FOG_1F,
		.imm_fields = 1,
	},
	[VERT_ATTRIB_TEX0] = {
		.vbo_index = 9,
		.imm_method = NV20_3D_VERTEX_TX0_4F_S,
		.imm_fields = 4,
	},
	[VERT_ATTRIB_TEX1] = {
		.vbo_index = 10,
		.imm_method = NV20_3D_VERTEX_TX1_4F_S,
		.imm_fields = 4,
	},
	[VERT_ATTRIB_TEX2] = {
		.vbo_index = 11,
		.imm_method = NV20_3D_VERTEX_TX2_4F_S,
		.imm_fields = 4,
	},
	[VERT_ATTRIB_TEX3] = {
		.vbo_index = 12,
		.imm_method = NV20_3D_VERTEX_TX3_4F_S,
		.imm_fields = 4,
	},
	[VERT_ATTRIB_GENERIC0] = {
		.emit = nv20_emit_material,
	},
	[VERT_ATTRIB_GENERIC1] = {
		.emit = nv20_emit_material,
	},
	[VERT_ATTRIB_GENERIC2] = {
		.emit = nv20_emit_material,
	},
	[VERT_ATTRIB_GENERIC3] = {
		.emit = nv20_emit_material,
	},
	[VERT_ATTRIB_GENERIC4] = {
		.emit = nv20_emit_material,
	},
	[VERT_ATTRIB_GENERIC5] = {
		.emit = nv20_emit_material,
	},
	[VERT_ATTRIB_GENERIC6] = {
		.emit = nv20_emit_material,
	},
	[VERT_ATTRIB_GENERIC7] = {
		.emit = nv20_emit_material,
	},
	[VERT_ATTRIB_GENERIC8] = {
		.emit = nv20_emit_material,
	},
	[VERT_ATTRIB_GENERIC9] = {
		.emit = nv20_emit_material,
	},
};

static int
get_hw_format(int type)
{
	switch (type) {
	case GL_FLOAT:
		return NV20_3D_VTXBUF_FMT_TYPE_FLOAT;
	case GL_UNSIGNED_SHORT:
		return NV20_3D_VTXBUF_FMT_TYPE_USHORT;
	case GL_UNSIGNED_BYTE:
		return NV20_3D_VTXBUF_FMT_TYPE_UBYTE;
	default:
		assert(0);
	}
}

static void
nv20_render_set_format(struct gl_context *ctx)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	int i, attr, hw_format;

	FOR_EACH_ATTR(render, i, attr) {
		if (attr >= 0) {
			struct nouveau_array *a = &render->attrs[attr];

			hw_format = a->stride << 8 |
				a->fields << 4 |
				get_hw_format(a->type);

		} else {
			/* Unused attribute. */
			hw_format = NV20_3D_VTXBUF_FMT_TYPE_FLOAT;
		}

		BEGIN_NV04(push, NV20_3D(VTXBUF_FMT(i)), 1);
		PUSH_DATA (push, hw_format);
	}
}

static void
nv20_render_bind_vertices(struct gl_context *ctx)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	int i, attr;

	FOR_EACH_BOUND_ATTR(render, i, attr) {
		struct nouveau_array *a = &render->attrs[attr];

		BEGIN_NV04(push, NV20_3D(VTXBUF_OFFSET(i)), 1);
		PUSH_MTHD (push, NV20_3D(VTXBUF_OFFSET(i)), BUFCTX_VTX,
				 a->bo, a->offset, NOUVEAU_BO_LOW |
				 NOUVEAU_BO_OR | NOUVEAU_BO_GART |
				 NOUVEAU_BO_RD, 0,
				 NV20_3D_VTXBUF_OFFSET_DMA1);
	}
}

static void
nv20_render_release_vertices(struct gl_context *ctx)
{
	PUSH_RESET(context_push(ctx), BUFCTX_VTX);
}

/* Vertex array rendering defs. */
#define RENDER_LOCALS(ctx)

#define BATCH_VALIDATE()						\
	BEGIN_NV04(push, NV20_3D(VTXBUF_VALIDATE), 1);	\
	PUSH_DATA (push, 0)

#define BATCH_BEGIN(prim)					\
	BEGIN_NV04(push, NV20_3D(VERTEX_BEGIN_END), 1);	\
	PUSH_DATA (push, prim)
#define BATCH_END()						\
	BEGIN_NV04(push, NV20_3D(VERTEX_BEGIN_END), 1);	\
	PUSH_DATA (push, 0)

#define MAX_PACKET 0x400

#define MAX_OUT_L 0x100
#define BATCH_PACKET_L(n)						\
	BEGIN_NI04(push, NV20_3D(VTXBUF_BATCH), n)
#define BATCH_OUT_L(i, n)			\
	PUSH_DATA (push, ((n) - 1) << 24 | (i))

#define MAX_OUT_I16 0x2
#define BATCH_PACKET_I16(n)					\
	BEGIN_NI04(push, NV20_3D(VTXBUF_ELEMENT_U16), n)
#define BATCH_OUT_I16(i0, i1)			\
	PUSH_DATA (push, (i1) << 16 | (i0))

#define MAX_OUT_I32 0x1
#define BATCH_PACKET_I32(n)					\
	BEGIN_NI04(push, NV20_3D(VTXBUF_ELEMENT_U32), n)
#define BATCH_OUT_I32(i)			\
	PUSH_DATA (push, i)

#define IMM_PACKET(m, n)			\
	BEGIN_NV04(push, SUBC_3D(m), n)
#define IMM_OUT(x)				\
	PUSH_DATAf(push, x)

#define TAG(x) nv20_##x
#include "nouveau_render_t.c"
#include "nouveau_vbo_t.c"
#include "nouveau_swtnl_t.c"
