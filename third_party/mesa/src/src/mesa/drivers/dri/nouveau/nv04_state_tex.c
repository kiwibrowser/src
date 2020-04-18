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
#include "nouveau_texture.h"
#include "nouveau_util.h"
#include "nouveau_gldefs.h"
#include "nv_object.xml.h"
#include "nv04_3d.xml.h"
#include "nv04_driver.h"
#include "main/samplerobj.h"

static uint32_t
get_tex_format(struct gl_texture_image *ti)
{
	switch (ti->TexFormat) {
	case MESA_FORMAT_A8:
	case MESA_FORMAT_L8:
	case MESA_FORMAT_I8:
		return NV04_TEXTURED_TRIANGLE_FORMAT_COLOR_Y8;
	case MESA_FORMAT_ARGB1555:
		return NV04_TEXTURED_TRIANGLE_FORMAT_COLOR_A1R5G5B5;
	case MESA_FORMAT_ARGB4444:
		return NV04_TEXTURED_TRIANGLE_FORMAT_COLOR_A4R4G4B4;
	case MESA_FORMAT_RGB565:
		return NV04_TEXTURED_TRIANGLE_FORMAT_COLOR_R5G6B5;
	case MESA_FORMAT_ARGB8888:
		return NV04_TEXTURED_TRIANGLE_FORMAT_COLOR_A8R8G8B8;
	case MESA_FORMAT_XRGB8888:
		return NV04_TEXTURED_TRIANGLE_FORMAT_COLOR_X8R8G8B8;
	default:
		assert(0);
	}
}

void
nv04_emit_tex_obj(struct gl_context *ctx, int emit)
{
	struct nv04_context *nv04 = to_nv04_context(ctx);
	const int i = emit - NOUVEAU_STATE_TEX_OBJ0;
	struct nouveau_surface *s;
	uint32_t format = 0xa0, filter = 0x1010;

	if (ctx->Texture.Unit[i]._ReallyEnabled) {
		struct gl_texture_object *t = ctx->Texture.Unit[i]._Current;
		struct gl_texture_image *ti = t->Image[0][t->BaseLevel];
		const struct gl_sampler_object *sa = _mesa_get_samplerobj(ctx, i);
		int lod_max = 1, lod_bias = 0;

		if (!nouveau_texture_validate(ctx, t))
			return;

		s = &to_nouveau_texture(t)->surfaces[t->BaseLevel];

		if (sa->MinFilter != GL_NEAREST &&
		    sa->MinFilter != GL_LINEAR) {
			lod_max = CLAMP(MIN2(sa->MaxLod, t->_MaxLambda),
					0, 15) + 1;

			lod_bias = CLAMP(ctx->Texture.Unit[i].LodBias +
					 sa->LodBias, -16, 15) * 8;
		}

		format |= nvgl_wrap_mode(sa->WrapT) << 28 |
			nvgl_wrap_mode(sa->WrapS) << 24 |
			ti->HeightLog2 << 20 |
			ti->WidthLog2 << 16 |
			lod_max << 12 |
			get_tex_format(ti);

		filter |= log2i(sa->MaxAnisotropy) << 31 |
			nvgl_filter_mode(sa->MagFilter) << 28 |
			log2i(sa->MaxAnisotropy) << 27 |
			nvgl_filter_mode(sa->MinFilter) << 24 |
			(lod_bias & 0xff) << 16;

	} else {
		s = &to_nv04_context(ctx)->dummy_texture;

		format |= NV04_TEXTURED_TRIANGLE_FORMAT_ADDRESSU_REPEAT |
			NV04_TEXTURED_TRIANGLE_FORMAT_ADDRESSV_REPEAT |
			1 << 12 |
			NV04_TEXTURED_TRIANGLE_FORMAT_COLOR_A8R8G8B8;

		filter |= NV04_TEXTURED_TRIANGLE_FILTER_MINIFY_NEAREST |
			NV04_TEXTURED_TRIANGLE_FILTER_MAGNIFY_NEAREST;
	}

	nv04->texture[i] = s;
	nv04->format[i] = format;
	nv04->filter[i] = filter;
}
