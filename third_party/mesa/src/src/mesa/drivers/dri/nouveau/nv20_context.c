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
#include "nouveau_driver.h"
#include "nouveau_context.h"
#include "nouveau_fbo.h"
#include "nouveau_util.h"
#include "nv_object.xml.h"
#include "nv20_3d.xml.h"
#include "nv04_driver.h"
#include "nv10_driver.h"
#include "nv20_driver.h"

static void
nv20_clear(struct gl_context *ctx, GLbitfield buffers)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	struct gl_framebuffer *fb = ctx->DrawBuffer;
	uint32_t clear = 0;

	nouveau_validate_framebuffer(ctx);

	nouveau_pushbuf_bufctx(push, nctx->hw.bufctx);
	if (nouveau_pushbuf_validate(push)) {
		nouveau_pushbuf_bufctx(push, NULL);
		return;
	}

	if (buffers & BUFFER_BITS_COLOR) {
		struct nouveau_surface *s = &to_nouveau_renderbuffer(
			fb->_ColorDrawBuffers[0])->surface;

		if (ctx->Color.ColorMask[0][RCOMP])
			clear |= NV20_3D_CLEAR_BUFFERS_COLOR_R;
		if (ctx->Color.ColorMask[0][GCOMP])
			clear |= NV20_3D_CLEAR_BUFFERS_COLOR_G;
		if (ctx->Color.ColorMask[0][BCOMP])
			clear |= NV20_3D_CLEAR_BUFFERS_COLOR_B;
		if (ctx->Color.ColorMask[0][ACOMP])
			clear |= NV20_3D_CLEAR_BUFFERS_COLOR_A;

		BEGIN_NV04(push, NV20_3D(CLEAR_VALUE), 1);
		PUSH_DATA (push, pack_rgba_clamp_f(s->format, ctx->Color.ClearColor.f));

		buffers &= ~BUFFER_BITS_COLOR;
	}

	if (buffers & (BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL)) {
		struct nouveau_surface *s = &to_nouveau_renderbuffer(
			fb->Attachment[BUFFER_DEPTH].Renderbuffer)->surface;

		if (buffers & BUFFER_BIT_DEPTH && ctx->Depth.Mask)
			clear |= NV20_3D_CLEAR_BUFFERS_DEPTH;
		if (buffers & BUFFER_BIT_STENCIL && ctx->Stencil.WriteMask[0])
			clear |= NV20_3D_CLEAR_BUFFERS_STENCIL;

		BEGIN_NV04(push, NV20_3D(CLEAR_DEPTH_VALUE), 1);
		PUSH_DATA (push, pack_zs_f(s->format, ctx->Depth.Clear,
					 ctx->Stencil.Clear));

		buffers &= ~(BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL);
	}

	BEGIN_NV04(push, NV20_3D(CLEAR_BUFFERS), 1);
	PUSH_DATA (push, clear);

	nouveau_pushbuf_bufctx(push, NULL);
	nouveau_clear(ctx, buffers);
}

static void
nv20_hwctx_init(struct gl_context *ctx)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	struct nouveau_hw_state *hw = &to_nouveau_context(ctx)->hw;
	struct nv04_fifo *fifo = hw->chan->data;
	int i;

	BEGIN_NV04(push, NV01_SUBC(3D, OBJECT), 1);
	PUSH_DATA (push, hw->eng3d->handle);
	BEGIN_NV04(push, NV20_3D(DMA_NOTIFY), 1);
	PUSH_DATA (push, hw->ntfy->handle);
	BEGIN_NV04(push, NV20_3D(DMA_TEXTURE0), 2);
	PUSH_DATA (push, fifo->vram);
	PUSH_DATA (push, fifo->gart);
	BEGIN_NV04(push, NV20_3D(DMA_COLOR), 2);
	PUSH_DATA (push, fifo->vram);
	PUSH_DATA (push, fifo->vram);
	BEGIN_NV04(push, NV20_3D(DMA_VTXBUF0), 2);
	PUSH_DATA (push, fifo->vram);
	PUSH_DATA (push, fifo->gart);

	BEGIN_NV04(push, NV20_3D(DMA_QUERY), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV20_3D(RT_HORIZ), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV20_3D(VIEWPORT_CLIP_HORIZ(0)), 1);
	PUSH_DATA (push, 0xfff << 16 | 0x0);
	BEGIN_NV04(push, NV20_3D(VIEWPORT_CLIP_VERT(0)), 1);
	PUSH_DATA (push, 0xfff << 16 | 0x0);

	for (i = 1; i < NV20_3D_VIEWPORT_CLIP_HORIZ__LEN; i++) {
		BEGIN_NV04(push, NV20_3D(VIEWPORT_CLIP_HORIZ(i)), 1);
		PUSH_DATA (push, 0);
		BEGIN_NV04(push, NV20_3D(VIEWPORT_CLIP_VERT(i)), 1);
		PUSH_DATA (push, 0);
	}

	BEGIN_NV04(push, NV20_3D(VIEWPORT_CLIP_MODE), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, SUBC_3D(0x17e0), 3);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);

	if (context_chipset(ctx) >= 0x25) {
		BEGIN_NV04(push, NV20_3D(TEX_RCOMP), 1);
		PUSH_DATA (push, NV20_3D_TEX_RCOMP_LEQUAL | 0xdb0);
	} else {
		BEGIN_NV04(push, SUBC_3D(0x1e68), 1);
		PUSH_DATA (push, 0x4b800000); /* 16777216.000000 */
		BEGIN_NV04(push, NV20_3D(TEX_RCOMP), 1);
		PUSH_DATA (push, NV20_3D_TEX_RCOMP_LEQUAL);
	}

	BEGIN_NV04(push, SUBC_3D(0x290), 1);
	PUSH_DATA (push, 0x10 << 16 | 1);
	BEGIN_NV04(push, SUBC_3D(0x9fc), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, SUBC_3D(0x1d80), 1);
	PUSH_DATA (push, 1);
	BEGIN_NV04(push, SUBC_3D(0x9f8), 1);
	PUSH_DATA (push, 4);
	BEGIN_NV04(push, SUBC_3D(0x17ec), 3);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 0.0);

	if (context_chipset(ctx) >= 0x25) {
		BEGIN_NV04(push, SUBC_3D(0x1d88), 1);
		PUSH_DATA (push, 3);

		BEGIN_NV04(push, NV25_3D(DMA_HIERZ), 1);
		PUSH_DATA (push, fifo->vram);
		BEGIN_NV04(push, NV25_3D(UNK01AC), 1);
		PUSH_DATA (push, fifo->vram);
	}

	BEGIN_NV04(push, NV20_3D(DMA_FENCE), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, SUBC_3D(0x1e98), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV04_GRAPH(3D, NOTIFY), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, SUBC_3D(0x120), 3);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 2);

	if (context_chipset(ctx) >= 0x25) {
		BEGIN_NV04(push, SUBC_3D(0x1da4), 1);
		PUSH_DATA (push, 0);
	}

	BEGIN_NV04(push, NV20_3D(RT_HORIZ), 2);
	PUSH_DATA (push, 0 << 16 | 0);
	PUSH_DATA (push, 0 << 16 | 0);

	BEGIN_NV04(push, NV20_3D(ALPHA_FUNC_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(ALPHA_FUNC_FUNC), 2);
	PUSH_DATA (push, NV20_3D_ALPHA_FUNC_FUNC_ALWAYS);
	PUSH_DATA (push, 0);

	for (i = 0; i < NV20_3D_TEX__LEN; i++) {
		BEGIN_NV04(push, NV20_3D(TEX_ENABLE(i)), 1);
		PUSH_DATA (push, 0);
	}

	BEGIN_NV04(push, NV20_3D(TEX_SHADER_OP), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(TEX_SHADER_CULL_MODE), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV20_3D(RC_IN_ALPHA(0)), 4);
	PUSH_DATA (push, 0x30d410d0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(RC_OUT_RGB(0)), 4);
	PUSH_DATA (push, 0x00000c00);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(RC_ENABLE), 1);
	PUSH_DATA (push, 0x00011101);
	BEGIN_NV04(push, NV20_3D(RC_FINAL0), 2);
	PUSH_DATA (push, 0x130e0300);
	PUSH_DATA (push, 0x0c091c80);
	BEGIN_NV04(push, NV20_3D(RC_OUT_ALPHA(0)), 4);
	PUSH_DATA (push, 0x00000c00);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(RC_IN_RGB(0)), 4);
	PUSH_DATA (push, 0x20c400c0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(RC_COLOR0), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(RC_CONSTANT_COLOR0(0)), 4);
	PUSH_DATA (push, 0x035125a0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0x40002000);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV20_3D(MULTISAMPLE_CONTROL), 1);
	PUSH_DATA (push, 0xffff0000);
	BEGIN_NV04(push, NV20_3D(BLEND_FUNC_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(DITHER_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(STENCIL_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(BLEND_FUNC_SRC), 4);
	PUSH_DATA (push, NV20_3D_BLEND_FUNC_SRC_ONE);
	PUSH_DATA (push, NV20_3D_BLEND_FUNC_DST_ZERO);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, NV20_3D_BLEND_EQUATION_FUNC_ADD);
	BEGIN_NV04(push, NV20_3D(STENCIL_MASK), 7);
	PUSH_DATA (push, 0xff);
	PUSH_DATA (push, NV20_3D_STENCIL_FUNC_FUNC_ALWAYS);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0xff);
	PUSH_DATA (push, NV20_3D_STENCIL_OP_FAIL_KEEP);
	PUSH_DATA (push, NV20_3D_STENCIL_OP_ZFAIL_KEEP);
	PUSH_DATA (push, NV20_3D_STENCIL_OP_ZPASS_KEEP);

	BEGIN_NV04(push, NV20_3D(COLOR_LOGIC_OP_ENABLE), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, NV20_3D_COLOR_LOGIC_OP_OP_COPY);
	BEGIN_NV04(push, SUBC_3D(0x17cc), 1);
	PUSH_DATA (push, 0);
	if (context_chipset(ctx) >= 0x25) {
		BEGIN_NV04(push, SUBC_3D(0x1d84), 1);
		PUSH_DATA (push, 1);
	}
	BEGIN_NV04(push, NV20_3D(LIGHTING_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(LIGHT_MODEL), 1);
	PUSH_DATA (push, NV20_3D_LIGHT_MODEL_VIEWER_NONLOCAL);
	BEGIN_NV04(push, NV20_3D(SEPARATE_SPECULAR_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(LIGHT_MODEL_TWO_SIDE_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(ENABLED_LIGHTS), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(NORMALIZE_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(POLYGON_STIPPLE_PATTERN(0)),
		   NV20_3D_POLYGON_STIPPLE_PATTERN__LEN);
	for (i = 0; i < NV20_3D_POLYGON_STIPPLE_PATTERN__LEN; i++) {
		PUSH_DATA (push, 0xffffffff);
	}

	BEGIN_NV04(push, NV20_3D(POLYGON_OFFSET_POINT_ENABLE), 3);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(DEPTH_FUNC), 1);
	PUSH_DATA (push, NV20_3D_DEPTH_FUNC_LESS);
	BEGIN_NV04(push, NV20_3D(DEPTH_WRITE_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(DEPTH_TEST_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(POLYGON_OFFSET_FACTOR), 2);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	BEGIN_NV04(push, NV20_3D(DEPTH_CLAMP), 1);
	PUSH_DATA (push, 1);
	if (context_chipset(ctx) < 0x25) {
		BEGIN_NV04(push, SUBC_3D(0x1d84), 1);
		PUSH_DATA (push, 3);
	}
	BEGIN_NV04(push, NV20_3D(POINT_SIZE), 1);
	if (context_chipset(ctx) >= 0x25)
		PUSH_DATAf(push, 1.0);
	else
		PUSH_DATA (push, 8);

	if (context_chipset(ctx) >= 0x25) {
		BEGIN_NV04(push, NV20_3D(POINT_PARAMETERS_ENABLE), 1);
		PUSH_DATA (push, 0);
		BEGIN_NV04(push, SUBC_3D(0x0a1c), 1);
		PUSH_DATA (push, 0x800);
	} else {
		BEGIN_NV04(push, NV20_3D(POINT_PARAMETERS_ENABLE), 2);
		PUSH_DATA (push, 0);
		PUSH_DATA (push, 0);
	}

	BEGIN_NV04(push, NV20_3D(LINE_WIDTH), 1);
	PUSH_DATA (push, 8);
	BEGIN_NV04(push, NV20_3D(LINE_SMOOTH_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(POLYGON_MODE_FRONT), 2);
	PUSH_DATA (push, NV20_3D_POLYGON_MODE_FRONT_FILL);
	PUSH_DATA (push, NV20_3D_POLYGON_MODE_BACK_FILL);
	BEGIN_NV04(push, NV20_3D(CULL_FACE), 2);
	PUSH_DATA (push, NV20_3D_CULL_FACE_BACK);
	PUSH_DATA (push, NV20_3D_FRONT_FACE_CCW);
	BEGIN_NV04(push, NV20_3D(POLYGON_SMOOTH_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(CULL_FACE_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV20_3D(SHADE_MODEL), 1);
	PUSH_DATA (push, NV20_3D_SHADE_MODEL_SMOOTH);
	BEGIN_NV04(push, NV20_3D(POLYGON_STIPPLE_ENABLE), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV20_3D(TEX_GEN_MODE(0,0)),
		   4 * NV20_3D_TEX_GEN_MODE__ESIZE);
	for (i=0; i < 4 * NV20_3D_TEX_GEN_MODE__LEN; i++)
		PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV20_3D(FOG_COEFF(0)), 3);
	PUSH_DATAf(push, 1.5);
	PUSH_DATAf(push, -0.090168);
	PUSH_DATAf(push, 0.0);
	BEGIN_NV04(push, NV20_3D(FOG_MODE), 2);
	PUSH_DATA (push, NV20_3D_FOG_MODE_EXP_SIGNED);
	PUSH_DATA (push, NV20_3D_FOG_COORD_FOG);
	BEGIN_NV04(push, NV20_3D(FOG_ENABLE), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV20_3D(ENGINE), 1);
	PUSH_DATA (push, NV20_3D_ENGINE_FIXED);

	for (i = 0; i < NV20_3D_TEX_MATRIX_ENABLE__LEN; i++) {
		BEGIN_NV04(push, NV20_3D(TEX_MATRIX_ENABLE(i)), 1);
		PUSH_DATA (push, 0);
	}

	BEGIN_NV04(push, NV20_3D(VERTEX_ATTR_4F_X(1)), 4 * 15);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 1.0);
	for (i = 0; i < 12; i++) {
		PUSH_DATAf(push, 0.0);
		PUSH_DATAf(push, 0.0);
		PUSH_DATAf(push, 0.0);
		PUSH_DATAf(push, 1.0);
	}

	BEGIN_NV04(push, NV20_3D(EDGEFLAG_ENABLE), 1);
	PUSH_DATA (push, 1);
	BEGIN_NV04(push, NV20_3D(COLOR_MASK), 1);
	PUSH_DATA (push, 0x00010101);
	BEGIN_NV04(push, NV20_3D(CLEAR_VALUE), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV20_3D(DEPTH_RANGE_NEAR), 2);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 16777216.0);

	BEGIN_NV04(push, NV20_3D(VIEWPORT_TRANSLATE_X), 4);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 16777215.0);

	BEGIN_NV04(push, NV20_3D(VIEWPORT_SCALE_X), 4);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 16777215.0 * 0.5);
	PUSH_DATAf(push, 65535.0);

	PUSH_KICK (push);
}

static void
nv20_context_destroy(struct gl_context *ctx)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);

	nv04_surface_takedown(ctx);
	nv20_swtnl_destroy(ctx);
	nv20_vbo_destroy(ctx);

	nouveau_object_del(&nctx->hw.eng3d);

	nouveau_context_deinit(ctx);
	FREE(ctx);
}

static struct gl_context *
nv20_context_create(struct nouveau_screen *screen, const struct gl_config *visual,
		    struct gl_context *share_ctx)
{
	struct nouveau_context *nctx;
	struct gl_context *ctx;
	unsigned kelvin_class;
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
	ctx->Const.MaxTextureCoordUnits = NV20_TEXTURE_UNITS;
	ctx->Const.MaxTextureImageUnits = NV20_TEXTURE_UNITS;
	ctx->Const.MaxTextureUnits = NV20_TEXTURE_UNITS;
	ctx->Const.MaxTextureMaxAnisotropy = 8;
	ctx->Const.MaxTextureLodBias = 15;
	ctx->Driver.Clear = nv20_clear;

	/* 2D engine. */
	ret = nv04_surface_init(ctx);
	if (!ret)
		goto fail;

	/* 3D engine. */
	if (context_chipset(ctx) >= 0x25)
		kelvin_class = NV25_3D_CLASS;
	else
		kelvin_class = NV20_3D_CLASS;

	ret = nouveau_object_new(context_chan(ctx), 0xbeef0001, kelvin_class,
				 NULL, 0, &nctx->hw.eng3d);
	if (ret)
		goto fail;

	nv20_hwctx_init(ctx);
	nv20_vbo_init(ctx);
	nv20_swtnl_init(ctx);

	return ctx;

fail:
	nv20_context_destroy(ctx);
	return NULL;
}

const struct nouveau_driver nv20_driver = {
	.context_create = nv20_context_create,
	.context_destroy = nv20_context_destroy,
	.surface_copy = nv04_surface_copy,
	.surface_fill = nv04_surface_fill,
	.emit = (nouveau_state_func[]) {
		nv10_emit_alpha_func,
		nv10_emit_blend_color,
		nv10_emit_blend_equation,
		nv10_emit_blend_func,
		nv20_emit_clip_plane,
		nv20_emit_clip_plane,
		nv20_emit_clip_plane,
		nv20_emit_clip_plane,
		nv20_emit_clip_plane,
		nv20_emit_clip_plane,
		nv10_emit_color_mask,
		nv20_emit_color_material,
		nv10_emit_cull_face,
		nv10_emit_front_face,
		nv10_emit_depth,
		nv10_emit_dither,
		nv20_emit_frag,
		nv20_emit_framebuffer,
		nv20_emit_fog,
		nv10_emit_light_enable,
		nv20_emit_light_model,
		nv20_emit_light_source,
		nv20_emit_light_source,
		nv20_emit_light_source,
		nv20_emit_light_source,
		nv20_emit_light_source,
		nv20_emit_light_source,
		nv20_emit_light_source,
		nv20_emit_light_source,
		nv10_emit_line_stipple,
		nv10_emit_line_mode,
		nv20_emit_logic_opcode,
		nv20_emit_material_ambient,
		nv20_emit_material_ambient,
		nv20_emit_material_diffuse,
		nv20_emit_material_diffuse,
		nv20_emit_material_specular,
		nv20_emit_material_specular,
		nv20_emit_material_shininess,
		nv20_emit_material_shininess,
		nv20_emit_modelview,
		nv20_emit_point_mode,
		nv10_emit_point_parameter,
		nv10_emit_polygon_mode,
		nv10_emit_polygon_offset,
		nv10_emit_polygon_stipple,
		nv20_emit_projection,
		nv10_emit_render_mode,
		nv10_emit_scissor,
		nv10_emit_shade_model,
		nv10_emit_stencil_func,
		nv10_emit_stencil_mask,
		nv10_emit_stencil_op,
		nv20_emit_tex_env,
		nv20_emit_tex_env,
		nv20_emit_tex_env,
		nv20_emit_tex_env,
		nv20_emit_tex_gen,
		nv20_emit_tex_gen,
		nv20_emit_tex_gen,
		nv20_emit_tex_gen,
		nv20_emit_tex_mat,
		nv20_emit_tex_mat,
		nv20_emit_tex_mat,
		nv20_emit_tex_mat,
		nv20_emit_tex_obj,
		nv20_emit_tex_obj,
		nv20_emit_tex_obj,
		nv20_emit_tex_obj,
		nv20_emit_viewport,
		nv20_emit_tex_shader
	},
	.num_emit = NUM_NV20_STATE,
};
