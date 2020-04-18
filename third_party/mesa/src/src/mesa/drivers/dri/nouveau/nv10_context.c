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

#include <stdbool.h>
#include "main/state.h"
#include "nouveau_driver.h"
#include "nouveau_context.h"
#include "nouveau_fbo.h"
#include "nouveau_util.h"
#include "nv_object.xml.h"
#include "nv10_3d.xml.h"
#include "nv04_driver.h"
#include "nv10_driver.h"

static GLboolean
use_fast_zclear(struct gl_context *ctx, GLbitfield buffers)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct gl_framebuffer *fb = ctx->DrawBuffer;

	if (buffers & BUFFER_BIT_STENCIL) {
		/*
		 * The stencil test is bypassed when fast Z clears are
		 * enabled.
		 */
		nctx->hierz.clear_blocked = GL_TRUE;
		context_dirty(ctx, ZCLEAR);
		return GL_FALSE;
	}

	return !nctx->hierz.clear_blocked &&
		fb->_Xmax == fb->Width && fb->_Xmin == 0 &&
		fb->_Ymax == fb->Height && fb->_Ymin == 0;
}

GLboolean
nv10_use_viewport_zclear(struct gl_context *ctx)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct gl_framebuffer *fb = ctx->DrawBuffer;
	struct gl_renderbuffer *depthRb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;

	return context_chipset(ctx) < 0x17 &&
		!nctx->hierz.clear_blocked && depthRb &&
		(_mesa_get_format_bits(depthRb->Format,
				       GL_DEPTH_BITS) >= 24);
}

float
nv10_transform_depth(struct gl_context *ctx, float z)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);

	if (nv10_use_viewport_zclear(ctx))
		return 2097152.0 * (z + (nctx->hierz.clear_seq & 7));
	else
		return ctx->DrawBuffer->_DepthMaxF * z;
}

static void
nv10_zclear(struct gl_context *ctx, GLbitfield *buffers)
{
	/*
	 * Pre-nv17 cards don't have native support for fast Z clears,
	 * but in some cases we can still "clear" the Z buffer without
	 * actually blitting to it if we're willing to sacrifice a few
	 * bits of depth precision.
	 *
	 * Each time a clear is requested we modify the viewport
	 * transform in such a way that the old contents of the depth
	 * buffer are clamped to the requested clear value when
	 * they're read by the GPU.
	 */
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct gl_framebuffer *fb = ctx->DrawBuffer;
	struct nouveau_framebuffer *nfb = to_nouveau_framebuffer(fb);
	struct nouveau_surface *s = &to_nouveau_renderbuffer(
		fb->Attachment[BUFFER_DEPTH].Renderbuffer)->surface;

	if (nv10_use_viewport_zclear(ctx)) {
		int x, y, w, h;
		float z = ctx->Depth.Clear;
		uint32_t value = pack_zs_f(s->format, z, 0);

		get_scissors(fb, &x, &y, &w, &h);
		*buffers &= ~BUFFER_BIT_DEPTH;

		if (use_fast_zclear(ctx, *buffers)) {
			if (nfb->hierz.clear_value != value) {
				/* Don't fast clear if we're changing
				 * the depth value. */
				nfb->hierz.clear_value = value;

			} else if (z == 0.0) {
				nctx->hierz.clear_seq++;
				context_dirty(ctx, ZCLEAR);

				if ((nctx->hierz.clear_seq & 7) != 0 &&
				    nctx->hierz.clear_seq != 1)
					/* We didn't wrap around -- no need to
					 * clear the depth buffer for real. */
					return;

			} else if (z == 1.0) {
				nctx->hierz.clear_seq--;
				context_dirty(ctx, ZCLEAR);

				if ((nctx->hierz.clear_seq & 7) != 7)
					/* No wrap around */
					return;
			}
		}

		value = pack_zs_f(s->format,
				  (z + (nctx->hierz.clear_seq & 7)) / 8, 0);
		context_drv(ctx)->surface_fill(ctx, s, ~0, value, x, y, w, h);
	}
}

static void
nv17_zclear(struct gl_context *ctx, GLbitfield *buffers)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	struct nouveau_framebuffer *nfb = to_nouveau_framebuffer(
		ctx->DrawBuffer);
	struct nouveau_surface *s = &to_nouveau_renderbuffer(
		nfb->base.Attachment[BUFFER_DEPTH].Renderbuffer)->surface;

	/* Clear the hierarchical depth buffer */
	BEGIN_NV04(push, NV17_3D(HIERZ_FILL_VALUE), 1);
	PUSH_DATA (push, pack_zs_f(s->format, ctx->Depth.Clear, 0));
	BEGIN_NV04(push, NV17_3D(HIERZ_BUFFER_CLEAR), 1);
	PUSH_DATA (push, 1);

	/* Mark the depth buffer as cleared */
	if (use_fast_zclear(ctx, *buffers)) {
		if (nctx->hierz.clear_seq)
			*buffers &= ~BUFFER_BIT_DEPTH;

		nfb->hierz.clear_value =
			pack_zs_f(s->format, ctx->Depth.Clear, 0);
		nctx->hierz.clear_seq++;

		context_dirty(ctx, ZCLEAR);
	}
}

static void
nv10_clear(struct gl_context *ctx, GLbitfield buffers)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);

	nouveau_validate_framebuffer(ctx);

	nouveau_pushbuf_bufctx(push, nctx->hw.bufctx);
	if (nouveau_pushbuf_validate(push)) {
		nouveau_pushbuf_bufctx(push, NULL);
		return;
	}

	if ((buffers & BUFFER_BIT_DEPTH) && ctx->Depth.Mask) {
		if (context_chipset(ctx) >= 0x17)
			nv17_zclear(ctx, &buffers);
		else
			nv10_zclear(ctx, &buffers);

		/* Emit the zclear state if it's dirty */
		_mesa_update_state(ctx);
	}

	nouveau_pushbuf_bufctx(push, NULL);
	nouveau_clear(ctx, buffers);
}

static void
nv10_hwctx_init(struct gl_context *ctx)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	struct nouveau_hw_state *hw = &to_nouveau_context(ctx)->hw;
	struct nv04_fifo *fifo = hw->chan->data;
	int i;

	BEGIN_NV04(push, NV01_SUBC(3D, OBJECT), 1);
	PUSH_DATA (push, hw->eng3d->handle);
	BEGIN_NV04(push, NV10_3D(DMA_NOTIFY), 1);
	PUSH_DATA (push, hw->ntfy->handle);

	BEGIN_NV04(push, NV10_3D(DMA_TEXTURE0), 3);
	PUSH_DATA (push, fifo->vram);
	PUSH_DATA (push, fifo->gart);
	PUSH_DATA (push, fifo->gart);
	BEGIN_NV04(push, NV10_3D(DMA_COLOR), 2);
	PUSH_DATA (push, fifo->vram);
	PUSH_DATA (push, fifo->vram);

	BEGIN_NV04(push, NV04_GRAPH(3D, NOP), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV10_3D(RT_HORIZ), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV10_3D(VIEWPORT_CLIP_HORIZ(0)), 1);
	PUSH_DATA (push, 0x7ff << 16 | 0x800);
	BEGIN_NV04(push, NV10_3D(VIEWPORT_CLIP_VERT(0)), 1);
	PUSH_DATA (push, 0x7ff << 16 | 0x800);

	for (i = 1; i < 8; i++) {
		BEGIN_NV04(push, NV10_3D(VIEWPORT_CLIP_HORIZ(i)), 1);
		PUSH_DATA (push, 0);
		BEGIN_NV04(push, NV10_3D(VIEWPORT_CLIP_VERT(i)), 1);
		PUSH_DATA (push, 0);
	}

	BEGIN_NV04(push, SUBC_3D(0x290), 1);
	PUSH_DATA (push, 0x10 << 16 | 1);
	BEGIN_NV04(push, SUBC_3D(0x3f4), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV04_GRAPH(3D, NOP), 1);
	PUSH_DATA (push, 0);

	if (context_chipset(ctx) >= 0x17) {
		BEGIN_NV04(push, NV17_3D(UNK01AC), 2);
		PUSH_DATA (push, fifo->vram);
		PUSH_DATA (push, fifo->vram);

		BEGIN_NV04(push, SUBC_3D(0xd84), 1);
		PUSH_DATA (push, 0x3);

		BEGIN_NV04(push, NV17_3D(COLOR_MASK_ENABLE), 1);
		PUSH_DATA (push, 1);
	}

	if (context_chipset(ctx) >= 0x11) {
		BEGIN_NV04(push, SUBC_3D(0x120), 3);
		PUSH_DATA (push, 0);
		PUSH_DATA (push, 1);
		PUSH_DATA (push, 2);

		BEGIN_NV04(push, NV04_GRAPH(3D, NOP), 1);
		PUSH_DATA (push, 0);
	}

	BEGIN_NV04(push, NV04_GRAPH(3D, NOP), 1);
	PUSH_DATA (push, 0);

	/* Set state */
	BEGIN_NV04(push, NV10_3D(FOG_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(ALPHA_FUNC_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(ALPHA_FUNC_FUNC), 2);
	PUSH_DATA (push, 0x207);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(TEX_ENABLE(0)), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV10_3D(BLEND_FUNC_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(DITHER_ENABLE), 2);
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(LINE_SMOOTH_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(VERTEX_WEIGHT_ENABLE), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(BLEND_FUNC_SRC), 4);
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0x8006);
	BEGIN_NV04(push, NV10_3D(STENCIL_MASK), 8);
	PUSH_DATA (push, 0xff);
	PUSH_DATA (push, 0x207);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0xff);
	PUSH_DATA (push, 0x1e00);
	PUSH_DATA (push, 0x1e00);
	PUSH_DATA (push, 0x1e00);
	PUSH_DATA (push, 0x1d01);
	BEGIN_NV04(push, NV10_3D(NORMALIZE_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(FOG_ENABLE), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(LIGHT_MODEL), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(SEPARATE_SPECULAR_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(ENABLED_LIGHTS), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(POLYGON_OFFSET_POINT_ENABLE), 3);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(DEPTH_FUNC), 1);
	PUSH_DATA (push, 0x201);
	BEGIN_NV04(push, NV10_3D(DEPTH_WRITE_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(DEPTH_TEST_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(POLYGON_OFFSET_FACTOR), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(POINT_SIZE), 1);
	PUSH_DATA (push, 8);
	BEGIN_NV04(push, NV10_3D(POINT_PARAMETERS_ENABLE), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(LINE_WIDTH), 1);
	PUSH_DATA (push, 8);
	BEGIN_NV04(push, NV10_3D(LINE_SMOOTH_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(POLYGON_MODE_FRONT), 2);
	PUSH_DATA (push, 0x1b02);
	PUSH_DATA (push, 0x1b02);
	BEGIN_NV04(push, NV10_3D(CULL_FACE), 2);
	PUSH_DATA (push, 0x405);
	PUSH_DATA (push, 0x901);
	BEGIN_NV04(push, NV10_3D(POLYGON_SMOOTH_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(CULL_FACE_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(TEX_GEN_MODE(0, 0)), 8);
	for (i = 0; i < 8; i++)
		PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV10_3D(TEX_MATRIX_ENABLE(0)), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(FOG_COEFF(0)), 3);
	PUSH_DATA (push, 0x3fc00000);	/* -1.50 */
	PUSH_DATA (push, 0xbdb8aa0a);	/* -0.09 */
	PUSH_DATA (push, 0);		/*  0.00 */

	BEGIN_NV04(push, NV04_GRAPH(3D, NOP), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV10_3D(FOG_MODE), 2);
	PUSH_DATA (push, 0x802);
	PUSH_DATA (push, 2);
	/* for some reason VIEW_MATRIX_ENABLE need to be 6 instead of 4 when
	 * using texturing, except when using the texture matrix
	 */
	BEGIN_NV04(push, NV10_3D(VIEW_MATRIX_ENABLE), 1);
	PUSH_DATA (push, 6);
	BEGIN_NV04(push, NV10_3D(COLOR_MASK), 1);
	PUSH_DATA (push, 0x01010101);

	/* Set vertex component */
	BEGIN_NV04(push, NV10_3D(VERTEX_COL_4F_R), 4);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);
	BEGIN_NV04(push, NV10_3D(VERTEX_COL2_3F_R), 3);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(VERTEX_NOR_3F_X), 3);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATAf(push, 1.0);
	BEGIN_NV04(push, NV10_3D(VERTEX_TX0_4F_S), 4);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);
	BEGIN_NV04(push, NV10_3D(VERTEX_TX1_4F_S), 4);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);
	BEGIN_NV04(push, NV10_3D(VERTEX_FOG_1F), 1);
	PUSH_DATAf(push, 0.0);
	BEGIN_NV04(push, NV10_3D(EDGEFLAG_ENABLE), 1);
	PUSH_DATA (push, 1);

	BEGIN_NV04(push, NV10_3D(DEPTH_RANGE_NEAR), 2);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 16777216.0);

	PUSH_KICK (push);
}

static void
nv10_context_destroy(struct gl_context *ctx)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);

	nv04_surface_takedown(ctx);
	nv10_swtnl_destroy(ctx);
	nv10_vbo_destroy(ctx);

	nouveau_object_del(&nctx->hw.eng3d);

	nouveau_context_deinit(ctx);
	FREE(ctx);
}

static struct gl_context *
nv10_context_create(struct nouveau_screen *screen, const struct gl_config *visual,
		    struct gl_context *share_ctx)
{
	struct nouveau_context *nctx;
	struct gl_context *ctx;
	unsigned celsius_class;
	int ret;

	nctx = CALLOC_STRUCT(nouveau_context);
	if (!nctx)
		return NULL;

	ctx = &nctx->base;

	if (!nouveau_context_init(ctx, screen, visual, share_ctx))
		goto fail;

	ctx->Extensions.ARB_texture_env_crossbar = true;
	ctx->Extensions.ARB_texture_env_combine = true;
	ctx->Extensions.ARB_texture_env_dot3 = true;
	ctx->Extensions.NV_fog_distance = true;
	ctx->Extensions.NV_texture_rectangle = true;
	if (ctx->Mesa_DXTn) {
		ctx->Extensions.EXT_texture_compression_s3tc = true;
		ctx->Extensions.S3_s3tc = true;
	}

	/* GL constants. */
	ctx->Const.MaxTextureLevels = 12;
	ctx->Const.MaxTextureCoordUnits = NV10_TEXTURE_UNITS;
	ctx->Const.MaxTextureImageUnits = NV10_TEXTURE_UNITS;
	ctx->Const.MaxTextureUnits = NV10_TEXTURE_UNITS;
	ctx->Const.MaxTextureMaxAnisotropy = 2;
	ctx->Const.MaxTextureLodBias = 15;
	ctx->Driver.Clear = nv10_clear;

	/* 2D engine. */
	ret = nv04_surface_init(ctx);
	if (!ret)
		goto fail;

	/* 3D engine. */
	if (context_chipset(ctx) >= 0x17)
		celsius_class = NV17_3D_CLASS;
	else if (context_chipset(ctx) >= 0x11)
		celsius_class = NV15_3D_CLASS;
	else
		celsius_class = NV10_3D_CLASS;

	ret = nouveau_object_new(context_chan(ctx), 0xbeef0001, celsius_class,
				 NULL, 0, &nctx->hw.eng3d);
	if (ret)
		goto fail;

	nv10_hwctx_init(ctx);
	nv10_vbo_init(ctx);
	nv10_swtnl_init(ctx);

	return ctx;

fail:
	nv10_context_destroy(ctx);
	return NULL;
}

const struct nouveau_driver nv10_driver = {
	.context_create = nv10_context_create,
	.context_destroy = nv10_context_destroy,
	.surface_copy = nv04_surface_copy,
	.surface_fill = nv04_surface_fill,
	.emit = (nouveau_state_func[]) {
		nv10_emit_alpha_func,
		nv10_emit_blend_color,
		nv10_emit_blend_equation,
		nv10_emit_blend_func,
		nv10_emit_clip_plane,
		nv10_emit_clip_plane,
		nv10_emit_clip_plane,
		nv10_emit_clip_plane,
		nv10_emit_clip_plane,
		nv10_emit_clip_plane,
		nv10_emit_color_mask,
		nv10_emit_color_material,
		nv10_emit_cull_face,
		nv10_emit_front_face,
		nv10_emit_depth,
		nv10_emit_dither,
		nv10_emit_frag,
		nv10_emit_framebuffer,
		nv10_emit_fog,
		nv10_emit_light_enable,
		nv10_emit_light_model,
		nv10_emit_light_source,
		nv10_emit_light_source,
		nv10_emit_light_source,
		nv10_emit_light_source,
		nv10_emit_light_source,
		nv10_emit_light_source,
		nv10_emit_light_source,
		nv10_emit_light_source,
		nv10_emit_line_stipple,
		nv10_emit_line_mode,
		nv10_emit_logic_opcode,
		nv10_emit_material_ambient,
		nouveau_emit_nothing,
		nv10_emit_material_diffuse,
		nouveau_emit_nothing,
		nv10_emit_material_specular,
		nouveau_emit_nothing,
		nv10_emit_material_shininess,
		nouveau_emit_nothing,
		nv10_emit_modelview,
		nv10_emit_point_mode,
		nv10_emit_point_parameter,
		nv10_emit_polygon_mode,
		nv10_emit_polygon_offset,
		nv10_emit_polygon_stipple,
		nv10_emit_projection,
		nv10_emit_render_mode,
		nv10_emit_scissor,
		nv10_emit_shade_model,
		nv10_emit_stencil_func,
		nv10_emit_stencil_mask,
		nv10_emit_stencil_op,
		nv10_emit_tex_env,
		nv10_emit_tex_env,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nv10_emit_tex_gen,
		nv10_emit_tex_gen,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nv10_emit_tex_mat,
		nv10_emit_tex_mat,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nv10_emit_tex_obj,
		nv10_emit_tex_obj,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nv10_emit_viewport,
		nv10_emit_zclear
	},
	.num_emit = NUM_NV10_STATE,
};
