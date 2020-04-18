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

/*
 * Vertex submission helper definitions shared among the software and
 * hardware TnL paths.
 */

#include "nouveau_gldefs.h"

#include "main/light.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"

#define OUT_INDICES_L(r, i, d, n)		\
	BATCH_OUT_L(i + d, n);			\
	(void)r
#define OUT_INDICES_I16(r, i, d, n)				\
	BATCH_OUT_I16(r->ib.extract_u(&r->ib, 0, i) + d,	\
		      r->ib.extract_u(&r->ib, 0, i + 1) + d)
#define OUT_INDICES_I32(r, i, d, n)			\
	BATCH_OUT_I32(r->ib.extract_u(&r->ib, 0, i) + d)

/*
 * Emit <n> vertices using BATCH_OUT_<out>, MAX_OUT_<out> at a time,
 * grouping them in packets of length MAX_PACKET.
 *
 * out:   hardware index data type.
 * ctx:   GL context.
 * start: element within the index buffer to begin with.
 * delta: integer correction that will be added to each index found in
 *        the index buffer.
 */
#define EMIT_VBO(out, ctx, start, delta, n) do {			\
		struct nouveau_render_state *render = to_render_state(ctx); \
		int npush = n;						\
									\
		while (npush) {						\
			int npack = MIN2(npush, MAX_PACKET * MAX_OUT_##out); \
			npush -= npack;					\
									\
			BATCH_PACKET_##out((npack + MAX_OUT_##out - 1)	\
					   / MAX_OUT_##out);		\
			while (npack) {					\
				int nout = MIN2(npack, MAX_OUT_##out);	\
				npack -= nout;				\
									\
				OUT_INDICES_##out(render, start, delta, \
						  nout);		\
				start += nout;				\
			}						\
		}							\
	} while (0)

/*
 * Emit the <n>-th element of the array <a>, using IMM_OUT.
 */
#define EMIT_IMM(ctx, a, n) do {					\
		struct nouveau_attr_info *info =			\
			&TAG(vertex_attrs)[(a)->attr];			\
		int m;							\
									\
		if (!info->emit) {					\
			IMM_PACKET(info->imm_method, info->imm_fields);	\
									\
			for (m = 0; m < (a)->fields; m++)		\
				IMM_OUT((a)->extract_f(a, n, m));	\
									\
			for (m = (a)->fields; m < info->imm_fields; m++) \
				IMM_OUT(((float []){0, 0, 0, 1})[m]);	\
									\
		} else {						\
			info->emit(ctx, a, (a)->buf + n * (a)->stride);	\
		}							\
	} while (0)

static void
dispatch_l(struct gl_context *ctx, unsigned int start, int delta,
	   unsigned int n)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	RENDER_LOCALS(ctx);

	EMIT_VBO(L, ctx, start, delta, n);
}

static void
dispatch_i32(struct gl_context *ctx, unsigned int start, int delta,
	     unsigned int n)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	RENDER_LOCALS(ctx);

	EMIT_VBO(I32, ctx, start, delta, n);
}

static void
dispatch_i16(struct gl_context *ctx, unsigned int start, int delta,
	     unsigned int n)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	RENDER_LOCALS(ctx);

	EMIT_VBO(I32, ctx, start, delta, n & 1);
	EMIT_VBO(I16, ctx, start, delta, n & ~1);
}

/*
 * Select an appropriate dispatch function for the given index buffer.
 */
static dispatch_t
get_array_dispatch(struct nouveau_array *a)
{
	if (!a->fields)
		return dispatch_l;
	else if (a->type == GL_UNSIGNED_INT)
		return dispatch_i32;
	else
		return dispatch_i16;
}

/*
 * Returns how many vertices you can draw using <n> pushbuf dwords.
 */
static inline unsigned
get_max_vertices(struct gl_context *ctx, const struct _mesa_index_buffer *ib,
		 int n)
{
	struct nouveau_render_state *render = to_render_state(ctx);

	if (render->mode == IMM) {
		return MAX2(0, n - 4) / (render->vertex_size / 4 +
					 render->attr_count);
	} else {
		unsigned max_out;

		if (ib) {
			switch (ib->type) {
			case GL_UNSIGNED_INT:
				max_out = MAX_OUT_I32;
				break;

			case GL_UNSIGNED_SHORT:
				max_out = MAX_OUT_I16;
				break;

			case GL_UNSIGNED_BYTE:
				max_out = MAX_OUT_I16;
				break;

			default:
				assert(0);
				max_out = 0;
				break;
			}
		} else {
			max_out = MAX_OUT_L;
		}

		return MAX2(0, n - 7) * max_out * MAX_PACKET / (1 + MAX_PACKET);
	}
}

static void
TAG(emit_material)(struct gl_context *ctx, struct nouveau_array *a,
		   const void *v)
{
	int attr = a->attr - VERT_ATTRIB_GENERIC0;
	int state = ((int []) {
			NOUVEAU_STATE_MATERIAL_FRONT_AMBIENT,
			NOUVEAU_STATE_MATERIAL_BACK_AMBIENT,
			NOUVEAU_STATE_MATERIAL_FRONT_DIFFUSE,
			NOUVEAU_STATE_MATERIAL_BACK_DIFFUSE,
			NOUVEAU_STATE_MATERIAL_FRONT_SPECULAR,
			NOUVEAU_STATE_MATERIAL_BACK_SPECULAR,
			NOUVEAU_STATE_MATERIAL_FRONT_AMBIENT,
			NOUVEAU_STATE_MATERIAL_BACK_AMBIENT,
			NOUVEAU_STATE_MATERIAL_FRONT_SHININESS,
			NOUVEAU_STATE_MATERIAL_BACK_SHININESS
		}) [attr];

	COPY_4V(ctx->Light.Material.Attrib[attr], (float *)v);
	_mesa_update_material(ctx, 1 << attr);

	context_drv(ctx)->emit[state](ctx, state);
}
