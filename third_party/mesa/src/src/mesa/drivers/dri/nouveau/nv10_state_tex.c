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
#include "nouveau_gldefs.h"
#include "nouveau_texture.h"
#include "nv10_3d.xml.h"
#include "nouveau_util.h"
#include "nv10_driver.h"
#include "main/samplerobj.h"

void
nv10_emit_tex_gen(struct gl_context *ctx, int emit)
{
	const int i = emit - NOUVEAU_STATE_TEX_GEN0;
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	struct gl_texture_unit *unit = &ctx->Texture.Unit[i];
	int j;

	for (j = 0; j < 4; j++) {
		if (nctx->fallback == HWTNL && (unit->TexGenEnabled & 1 << j)) {
			struct gl_texgen *coord = get_texgen_coord(unit, j);
			float *k = get_texgen_coeff(coord);

			if (k) {
				BEGIN_NV04(push, NV10_3D(TEX_GEN_COEFF(i, j)), 4);
				PUSH_DATAp(push, k, 4);
			}

			BEGIN_NV04(push, NV10_3D(TEX_GEN_MODE(i,j)), 1);
			PUSH_DATA (push, nvgl_texgen_mode(coord->Mode));

		} else {
			BEGIN_NV04(push, NV10_3D(TEX_GEN_MODE(i,j)), 1);
			PUSH_DATA (push, 0);
		}
	}

	context_dirty_i(ctx, TEX_MAT, i);
}

void
nv10_emit_tex_mat(struct gl_context *ctx, int emit)
{
	const int i = emit - NOUVEAU_STATE_TEX_MAT0;
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);

	if (nctx->fallback == HWTNL &&
	    ((ctx->Texture._TexMatEnabled & 1 << i) ||
	     ctx->Texture.Unit[i]._GenFlags)) {
		BEGIN_NV04(push, NV10_3D(TEX_MATRIX_ENABLE(i)), 1);
		PUSH_DATA (push, 1);

		BEGIN_NV04(push, NV10_3D(TEX_MATRIX(i, 0)), 16);
		PUSH_DATAm(push, ctx->TextureMatrixStack[i].Top->m);

	} else {
		BEGIN_NV04(push, NV10_3D(TEX_MATRIX_ENABLE(i)), 1);
		PUSH_DATA (push, 0);
	}
}

static uint32_t
get_tex_format_pot(struct gl_texture_image *ti)
{
	switch (ti->TexFormat) {
	case MESA_FORMAT_ARGB8888:
		return NV10_3D_TEX_FORMAT_FORMAT_A8R8G8B8;

	case MESA_FORMAT_XRGB8888:
		return NV10_3D_TEX_FORMAT_FORMAT_X8R8G8B8;

	case MESA_FORMAT_ARGB1555:
		return NV10_3D_TEX_FORMAT_FORMAT_A1R5G5B5;

	case MESA_FORMAT_ARGB4444:
		return NV10_3D_TEX_FORMAT_FORMAT_A4R4G4B4;

	case MESA_FORMAT_RGB565:
		return NV10_3D_TEX_FORMAT_FORMAT_R5G6B5;

	case MESA_FORMAT_A8:
	case MESA_FORMAT_I8:
		return NV10_3D_TEX_FORMAT_FORMAT_I8;

	case MESA_FORMAT_L8:
		return NV10_3D_TEX_FORMAT_FORMAT_L8;

	case MESA_FORMAT_RGB_DXT1:
	case MESA_FORMAT_RGBA_DXT1:
		return NV10_3D_TEX_FORMAT_FORMAT_DXT1;

	case MESA_FORMAT_RGBA_DXT3:
		return NV10_3D_TEX_FORMAT_FORMAT_DXT3;

	case MESA_FORMAT_RGBA_DXT5:
		return NV10_3D_TEX_FORMAT_FORMAT_DXT5;

	default:
		assert(0);
	}
}

static uint32_t
get_tex_format_rect(struct gl_texture_image *ti)
{
	switch (ti->TexFormat) {
	case MESA_FORMAT_ARGB1555:
		return NV10_3D_TEX_FORMAT_FORMAT_A1R5G5B5_RECT;

	case MESA_FORMAT_RGB565:
		return NV10_3D_TEX_FORMAT_FORMAT_R5G6B5_RECT;

	case MESA_FORMAT_ARGB8888:
	case MESA_FORMAT_XRGB8888:
		return NV10_3D_TEX_FORMAT_FORMAT_A8R8G8B8_RECT;

	case MESA_FORMAT_A8:
	case MESA_FORMAT_L8:
	case MESA_FORMAT_I8:
		return NV10_3D_TEX_FORMAT_FORMAT_I8_RECT;

	default:
		assert(0);
	}
}

void
nv10_emit_tex_obj(struct gl_context *ctx, int emit)
{
	const int i = emit - NOUVEAU_STATE_TEX_OBJ0;
	struct nouveau_pushbuf *push = context_push(ctx);
	const int bo_flags = NOUVEAU_BO_RD | NOUVEAU_BO_GART | NOUVEAU_BO_VRAM;
	struct gl_texture_object *t;
	struct nouveau_surface *s;
	struct gl_texture_image *ti;
	const struct gl_sampler_object *sa;
	uint32_t tx_format, tx_filter, tx_enable;

	PUSH_RESET(push, BUFCTX_TEX(i));

	if (!ctx->Texture.Unit[i]._ReallyEnabled) {
		BEGIN_NV04(push, NV10_3D(TEX_ENABLE(i)), 1);
		PUSH_DATA (push, 0);
		return;
	}

	t = ctx->Texture.Unit[i]._Current;
	s = &to_nouveau_texture(t)->surfaces[t->BaseLevel];
	ti = t->Image[0][t->BaseLevel];
	sa = _mesa_get_samplerobj(ctx, i);

	if (!nouveau_texture_validate(ctx, t))
		return;

	/* Recompute the texturing registers. */
	tx_format = nvgl_wrap_mode(sa->WrapT) << 28
		| nvgl_wrap_mode(sa->WrapS) << 24
		| ti->HeightLog2 << 20
		| ti->WidthLog2 << 16
		| 5 << 4 | 1 << 12;

	tx_filter = nvgl_filter_mode(sa->MagFilter) << 28
		| nvgl_filter_mode(sa->MinFilter) << 24;

	tx_enable = NV10_3D_TEX_ENABLE_ENABLE
		| log2i(sa->MaxAnisotropy) << 4;

	if (t->Target == GL_TEXTURE_RECTANGLE) {
		BEGIN_NV04(push, NV10_3D(TEX_NPOT_PITCH(i)), 1);
		PUSH_DATA (push, s->pitch << 16);
		BEGIN_NV04(push, NV10_3D(TEX_NPOT_SIZE(i)), 1);
		PUSH_DATA (push, align(s->width, 2) << 16 | s->height);

		tx_format |= get_tex_format_rect(ti);
	} else {
		tx_format |= get_tex_format_pot(ti);
	}

	if (sa->MinFilter != GL_NEAREST &&
	    sa->MinFilter != GL_LINEAR) {
		int lod_min = sa->MinLod;
		int lod_max = MIN2(sa->MaxLod, t->_MaxLambda);
		int lod_bias = sa->LodBias
			+ ctx->Texture.Unit[i].LodBias;

		lod_max = CLAMP(lod_max, 0, 15);
		lod_min = CLAMP(lod_min, 0, 15);
		lod_bias = CLAMP(lod_bias, 0, 15);

		tx_format |= NV10_3D_TEX_FORMAT_MIPMAP;
		tx_filter |= lod_bias << 8;
		tx_enable |= lod_min << 26
			| lod_max << 14;
	}

	/* Write it to the hardware. */
	BEGIN_NV04(push, NV10_3D(TEX_FORMAT(i)), 1);
	PUSH_MTHD (push, NV10_3D(TEX_FORMAT(i)), BUFCTX_TEX(i),
			 s->bo, tx_format, bo_flags | NOUVEAU_BO_OR,
			 NV10_3D_TEX_FORMAT_DMA0,
			 NV10_3D_TEX_FORMAT_DMA1);

	BEGIN_NV04(push, NV10_3D(TEX_OFFSET(i)), 1);
	PUSH_MTHDl(push, NV10_3D(TEX_OFFSET(i)), BUFCTX_TEX(i),
			 s->bo, s->offset, bo_flags);

	BEGIN_NV04(push, NV10_3D(TEX_FILTER(i)), 1);
	PUSH_DATA (push, tx_filter);

	BEGIN_NV04(push, NV10_3D(TEX_ENABLE(i)), 1);
	PUSH_DATA (push, tx_enable);
}

