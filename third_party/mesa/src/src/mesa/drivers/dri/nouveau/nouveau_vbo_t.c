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
#include "nouveau_bufferobj.h"
#include "nouveau_util.h"

#include "main/bufferobj.h"
#include "main/glformats.h"
#include "main/image.h"

/* Arbitrary pushbuf length we can assume we can get with a single
 * call to WAIT_RING. */
#define PUSHBUF_DWORDS 65536

/* Functions to turn GL arrays or index buffers into nouveau_array
 * structures. */

static int
get_array_stride(struct gl_context *ctx, const struct gl_client_array *a)
{
	struct nouveau_render_state *render = to_render_state(ctx);

	if (render->mode == VBO && !_mesa_is_bufferobj(a->BufferObj))
		/* Pack client buffers. */
		return align(_mesa_sizeof_type(a->Type) * a->Size, 4);
	else
		return a->StrideB;
}

static void
vbo_init_arrays(struct gl_context *ctx, const struct _mesa_index_buffer *ib,
		const struct gl_client_array **arrays)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	GLboolean imm = (render->mode == IMM);
	int i, attr;

	if (ib)
		nouveau_init_array(&render->ib, 0, 0, ib->count, ib->type,
				   ib->obj, ib->ptr, GL_TRUE, ctx);

	FOR_EACH_BOUND_ATTR(render, i, attr) {
		const struct gl_client_array *array = arrays[attr];

		nouveau_init_array(&render->attrs[attr], attr,
				   get_array_stride(ctx, array),
				   array->Size, array->Type,
				   imm ? array->BufferObj : NULL,
				   array->Ptr, imm, ctx);
	}
}

static void
vbo_deinit_arrays(struct gl_context *ctx, const struct _mesa_index_buffer *ib,
		  const struct gl_client_array **arrays)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	int i, attr;

	if (ib)
		nouveau_cleanup_array(&render->ib);

	FOR_EACH_BOUND_ATTR(render, i, attr) {
		struct nouveau_array *a = &render->attrs[attr];

		if (render->mode == IMM)
			nouveau_bo_ref(NULL, &a->bo);

		nouveau_deinit_array(a);
		render->map[i] = -1;
	}

	render->attr_count = 0;
}

/* Make some rendering decisions from the GL context. */

static void
vbo_choose_render_mode(struct gl_context *ctx, const struct gl_client_array **arrays)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	int i;

	render->mode = VBO;

	if (ctx->Light.Enabled) {
		for (i = 0; i < MAT_ATTRIB_MAX; i++) {
			if (arrays[VERT_ATTRIB_GENERIC0 + i]->StrideB) {
				render->mode = IMM;
				break;
			}
		}
	}
}

static void
vbo_emit_attr(struct gl_context *ctx, const struct gl_client_array **arrays,
	      int attr)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	struct nouveau_render_state *render = to_render_state(ctx);
	const struct gl_client_array *array = arrays[attr];
	struct nouveau_array *a = &render->attrs[attr];
	RENDER_LOCALS(ctx);

	if (!array->StrideB) {
		if (attr >= VERT_ATTRIB_GENERIC0)
			/* nouveau_update_state takes care of materials. */
			return;

		/* Constant attribute. */
		nouveau_init_array(a, attr, array->StrideB, array->Size,
				   array->Type, array->BufferObj, array->Ptr,
				   GL_TRUE, ctx);
		EMIT_IMM(ctx, a, 0);
		nouveau_deinit_array(a);

	} else {
		/* Varying attribute. */
		struct nouveau_attr_info *info = &TAG(vertex_attrs)[attr];

		if (render->mode == VBO) {
			render->map[info->vbo_index] = attr;
			render->vertex_size += array->_ElementSize;
			render->attr_count = MAX2(render->attr_count,
						  info->vbo_index + 1);
		} else {
			render->map[render->attr_count++] = attr;
			render->vertex_size += 4 * info->imm_fields;
		}
	}
}

#define MAT(a) (VERT_ATTRIB_GENERIC0 + MAT_ATTRIB_##a)

static void
vbo_choose_attrs(struct gl_context *ctx, const struct gl_client_array **arrays)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	int i;

	/* Reset the vertex size. */
	render->vertex_size = 0;
	render->attr_count = 0;

	vbo_emit_attr(ctx, arrays, VERT_ATTRIB_COLOR0);
	if (ctx->Fog.ColorSumEnabled && !ctx->Light.Enabled)
		vbo_emit_attr(ctx, arrays, VERT_ATTRIB_COLOR1);

	for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
		if (ctx->Texture._EnabledCoordUnits & (1 << i))
			vbo_emit_attr(ctx, arrays, VERT_ATTRIB_TEX0 + i);
	}

	if (ctx->Fog.Enabled && ctx->Fog.FogCoordinateSource == GL_FOG_COORD)
		vbo_emit_attr(ctx, arrays, VERT_ATTRIB_FOG);

	if (ctx->Light.Enabled ||
	    (ctx->Texture._GenFlags & TEXGEN_NEED_NORMALS))
		vbo_emit_attr(ctx, arrays, VERT_ATTRIB_NORMAL);

	if (ctx->Light.Enabled && render->mode == IMM) {
		vbo_emit_attr(ctx, arrays, MAT(FRONT_AMBIENT));
		vbo_emit_attr(ctx, arrays, MAT(FRONT_DIFFUSE));
		vbo_emit_attr(ctx, arrays, MAT(FRONT_SPECULAR));
		vbo_emit_attr(ctx, arrays, MAT(FRONT_SHININESS));

		if (ctx->Light.Model.TwoSide) {
			vbo_emit_attr(ctx, arrays, MAT(BACK_AMBIENT));
			vbo_emit_attr(ctx, arrays, MAT(BACK_DIFFUSE));
			vbo_emit_attr(ctx, arrays, MAT(BACK_SPECULAR));
			vbo_emit_attr(ctx, arrays, MAT(BACK_SHININESS));
		}
	}

	vbo_emit_attr(ctx, arrays, VERT_ATTRIB_POS);
}

static int
get_max_client_stride(struct gl_context *ctx, const struct gl_client_array **arrays)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	int i, attr, s = 0;

	FOR_EACH_BOUND_ATTR(render, i, attr) {
		const struct gl_client_array *a = arrays[attr];

		if (!_mesa_is_bufferobj(a->BufferObj))
			s = MAX2(s, get_array_stride(ctx, a));
	}

	return s;
}

static void
TAG(vbo_render_prims)(struct gl_context *ctx,
		      const struct _mesa_prim *prims, GLuint nr_prims,
		      const struct _mesa_index_buffer *ib,
		      GLboolean index_bounds_valid,
		      GLuint min_index, GLuint max_index,
		      struct gl_transform_feedback_object *tfb_vertcount);

static GLboolean
vbo_maybe_split(struct gl_context *ctx, const struct gl_client_array **arrays,
	    const struct _mesa_prim *prims, GLuint nr_prims,
	    const struct _mesa_index_buffer *ib,
	    GLuint min_index, GLuint max_index)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_render_state *render = to_render_state(ctx);
	struct nouveau_bufctx *bufctx = nctx->hw.bufctx;
	unsigned pushbuf_avail = PUSHBUF_DWORDS - 2 * (bufctx->relocs +
						       render->attr_count),
		vert_avail = get_max_vertices(ctx, NULL, pushbuf_avail),
		idx_avail = get_max_vertices(ctx, ib, pushbuf_avail);
	int stride;

	/* Try to keep client buffers smaller than the scratch BOs. */
	if (render->mode == VBO &&
	    (stride = get_max_client_stride(ctx, arrays)))
		    vert_avail = MIN2(vert_avail,
				      NOUVEAU_SCRATCH_SIZE / stride);

	if (max_index - min_index > vert_avail ||
	    (ib && ib->count > idx_avail)) {
		struct split_limits limits = {
			.max_verts = vert_avail,
			.max_indices = idx_avail,
			.max_vb_size = ~0,
		};

		vbo_split_prims(ctx, arrays, prims, nr_prims, ib, min_index,
				max_index, TAG(vbo_render_prims), &limits);
		return GL_TRUE;
	}

	return GL_FALSE;
}

/* VBO rendering path. */

static GLboolean
check_update_array(struct nouveau_array *a, unsigned offset,
		   struct nouveau_bo *bo, int *pdelta)
{
	int delta = *pdelta;
	GLboolean dirty;

	if (a->bo == bo) {
		if (delta < 0)
			delta = ((int)offset - (int)a->offset) / a->stride;

		dirty = (delta < 0 ||
			 offset != (a->offset + delta * a->stride));
	} else {
		dirty = GL_TRUE;
	}

	*pdelta = (dirty ? 0 : delta);
	return dirty;
}

static void
vbo_bind_vertices(struct gl_context *ctx, const struct gl_client_array **arrays,
		  int base, unsigned min_index, unsigned max_index, int *pdelta)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	struct nouveau_bo *bo[NUM_VERTEX_ATTRS];
	unsigned offset[NUM_VERTEX_ATTRS];
	GLboolean dirty = GL_FALSE;
	int i, j, attr;
	RENDER_LOCALS(ctx);

	*pdelta = -1;

	FOR_EACH_BOUND_ATTR(render, i, attr) {
		const struct gl_client_array *array = arrays[attr];
		struct gl_buffer_object *obj = array->BufferObj;
		struct nouveau_array *a = &render->attrs[attr];
		unsigned delta = (base + min_index) * array->StrideB;

		bo[i] = NULL;

		if (nouveau_bufferobj_hw(obj)) {
			/* Array in a buffer obj. */
			nouveau_bo_ref(to_nouveau_bufferobj(obj)->bo, &bo[i]);
			offset[i] = delta + (intptr_t)array->Ptr;

		} else {
			int n = max_index - min_index + 1;
			char *sp = (char *)ADD_POINTERS(
				nouveau_bufferobj_sys(obj), array->Ptr) + delta;
			char *dp  = nouveau_get_scratch(ctx, n * a->stride,
							&bo[i], &offset[i]);

			/* Array in client memory, move it to a
			 * scratch buffer obj. */
			for (j = 0; j < n; j++)
				memcpy(dp + j * a->stride,
				       sp + j * array->StrideB,
				       a->stride);
		}

		dirty |= check_update_array(a, offset[i], bo[i], pdelta);
	}

	*pdelta -= min_index;

	if (dirty) {
		/* Buffers changed, update the attribute binding. */
		FOR_EACH_BOUND_ATTR(render, i, attr) {
			struct nouveau_array *a = &render->attrs[attr];

			nouveau_bo_ref(NULL, &a->bo);
			a->offset = offset[i];
			a->bo = bo[i];
		}

		TAG(render_release_vertices)(ctx);
		TAG(render_bind_vertices)(ctx);
	} else {
		/* Just cleanup. */
		FOR_EACH_BOUND_ATTR(render, i, attr)
			nouveau_bo_ref(NULL, &bo[i]);
	}

	BATCH_VALIDATE();
}

static void
vbo_draw_vbo(struct gl_context *ctx, const struct gl_client_array **arrays,
	     const struct _mesa_prim *prims, GLuint nr_prims,
	     const struct _mesa_index_buffer *ib, GLuint min_index,
	     GLuint max_index)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	dispatch_t dispatch = get_array_dispatch(&to_render_state(ctx)->ib);
	int i, delta = 0, basevertex = 0;
	RENDER_LOCALS(ctx);

	TAG(render_set_format)(ctx);

	for (i = 0; i < nr_prims; i++) {
		unsigned start = prims[i].start,
			count = prims[i].count;

		if (i == 0 || basevertex != prims[i].basevertex) {
			basevertex = prims[i].basevertex;
			vbo_bind_vertices(ctx, arrays, basevertex, min_index,
					  max_index, &delta);

			nouveau_pushbuf_bufctx(push, nctx->hw.bufctx);
			if (nouveau_pushbuf_validate(push)) {
				nouveau_pushbuf_bufctx(push, NULL);
				return;
			}
		}

		if (count > get_max_vertices(ctx, ib, PUSH_AVAIL(push)))
			PUSH_SPACE(push, PUSHBUF_DWORDS);

		BATCH_BEGIN(nvgl_primitive(prims[i].mode));
		dispatch(ctx, start, delta, count);
		BATCH_END();
	}

	nouveau_pushbuf_bufctx(push, NULL);
	TAG(render_release_vertices)(ctx);
}

/* Immediate rendering path. */

static unsigned
extract_id(struct nouveau_array *a, int i, int j)
{
	return j;
}

static void
vbo_draw_imm(struct gl_context *ctx, const struct gl_client_array **arrays,
	     const struct _mesa_prim *prims, GLuint nr_prims,
	     const struct _mesa_index_buffer *ib, GLuint min_index,
	     GLuint max_index)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	extract_u_t extract = ib ? render->ib.extract_u : extract_id;
	int i, j, k, attr;
	RENDER_LOCALS(ctx);

	nouveau_pushbuf_bufctx(push, nctx->hw.bufctx);
	if (nouveau_pushbuf_validate(push)) {
		nouveau_pushbuf_bufctx(push, NULL);
		return;
	}

	for (i = 0; i < nr_prims; i++) {
		unsigned start = prims[i].start,
			end = start + prims[i].count;

		if (prims[i].count > get_max_vertices(ctx, ib,
						      PUSH_AVAIL(push)))
			PUSH_SPACE(push, PUSHBUF_DWORDS);

		BATCH_BEGIN(nvgl_primitive(prims[i].mode));

		for (; start < end; start++) {
			j = prims[i].basevertex +
				extract(&render->ib, 0, start);

			FOR_EACH_BOUND_ATTR(render, k, attr)
				EMIT_IMM(ctx, &render->attrs[attr], j);
		}

		BATCH_END();
	}

	nouveau_pushbuf_bufctx(push, NULL);
}

/* draw_prims entry point when we're doing hw-tnl. */

static void
TAG(vbo_render_prims)(struct gl_context *ctx,
		      const struct _mesa_prim *prims, GLuint nr_prims,
		      const struct _mesa_index_buffer *ib,
		      GLboolean index_bounds_valid,
		      GLuint min_index, GLuint max_index,
		      struct gl_transform_feedback_object *tfb_vertcount)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	const struct gl_client_array **arrays = ctx->Array._DrawArrays;

	if (!index_bounds_valid)
		vbo_get_minmax_indices(ctx, prims, ib, &min_index, &max_index,
				       nr_prims);

	vbo_choose_render_mode(ctx, arrays);
	vbo_choose_attrs(ctx, arrays);

	if (vbo_maybe_split(ctx, arrays, prims, nr_prims, ib, min_index,
			    max_index))
		return;

	vbo_init_arrays(ctx, ib, arrays);

	if (render->mode == VBO)
		vbo_draw_vbo(ctx, arrays, prims, nr_prims, ib, min_index,
			     max_index);
	else
		vbo_draw_imm(ctx, arrays, prims, nr_prims, ib, min_index,
			     max_index);

	vbo_deinit_arrays(ctx, ib, arrays);
}

/* VBO rendering entry points. */

static void
TAG(vbo_check_render_prims)(struct gl_context *ctx,
			    const struct _mesa_prim *prims, GLuint nr_prims,
			    const struct _mesa_index_buffer *ib,
			    GLboolean index_bounds_valid,
			    GLuint min_index, GLuint max_index,
			    struct gl_transform_feedback_object *tfb_vertcount)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);

	nouveau_validate_framebuffer(ctx);

	if (nctx->fallback == HWTNL)
		TAG(vbo_render_prims)(ctx, prims, nr_prims, ib,
				      index_bounds_valid, min_index, max_index,
				      tfb_vertcount);

	if (nctx->fallback == SWTNL)
		_tnl_vbo_draw_prims(ctx, prims, nr_prims, ib,
				    index_bounds_valid, min_index, max_index,
				    tfb_vertcount);
}

void
TAG(vbo_init)(struct gl_context *ctx)
{
	struct nouveau_render_state *render = to_render_state(ctx);
	int i;

	for (i = 0; i < VERT_ATTRIB_MAX; i++)
		render->map[i] = -1;

	vbo_set_draw_func(ctx, TAG(vbo_check_render_prims));
	vbo_use_buffer_objects(ctx);
}

void
TAG(vbo_destroy)(struct gl_context *ctx)
{
}
