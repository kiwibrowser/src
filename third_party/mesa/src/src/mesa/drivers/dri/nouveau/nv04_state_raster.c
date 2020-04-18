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
#include "nv_object.xml.h"
#include "nv04_3d.xml.h"
#include "nv04_driver.h"

static unsigned
get_comparison_op(unsigned op)
{
	switch (op) {
	case GL_NEVER:
		return 0x1;
	case GL_LESS:
		return 0x2;
	case GL_EQUAL:
		return 0x3;
	case GL_LEQUAL:
		return 0x4;
	case GL_GREATER:
		return 0x5;
	case GL_NOTEQUAL:
		return 0x6;
	case GL_GEQUAL:
		return 0x7;
	case GL_ALWAYS:
		return 0x8;
	default:
		assert(0);
	}
}

static unsigned
get_stencil_op(unsigned op)
{
	switch (op) {
	case GL_KEEP:
		return 0x1;
	case GL_ZERO:
		return 0x2;
	case GL_REPLACE:
		return 0x3;
	case GL_INCR:
		return 0x4;
	case GL_DECR:
		return 0x5;
	case GL_INVERT:
		return 0x6;
	case GL_INCR_WRAP:
		return 0x7;
	case GL_DECR_WRAP:
		return 0x8;
	default:
		assert(0);
	}
}

static unsigned
get_blend_func(unsigned func)
{
	switch (func) {
	case GL_ZERO:
		return 0x1;
	case GL_ONE:
		return 0x2;
	case GL_SRC_COLOR:
		return 0x3;
	case GL_ONE_MINUS_SRC_COLOR:
		return 0x4;
	case GL_SRC_ALPHA:
		return 0x5;
	case GL_ONE_MINUS_SRC_ALPHA:
		return 0x6;
	case GL_DST_ALPHA:
		return 0x7;
	case GL_ONE_MINUS_DST_ALPHA:
		return 0x8;
	case GL_DST_COLOR:
		return 0x9;
	case GL_ONE_MINUS_DST_COLOR:
		return 0xa;
	case GL_SRC_ALPHA_SATURATE:
		return 0xb;
	default:
		assert(0);
	}
}

void
nv04_defer_control(struct gl_context *ctx, int emit)
{
	context_dirty(ctx, CONTROL);
}

void
nv04_emit_control(struct gl_context *ctx, int emit)
{
	struct nv04_context *nv04 = to_nv04_context(ctx);
	int cull = ctx->Polygon.CullFaceMode;
	int front = ctx->Polygon.FrontFace;

	nv04->ctrl[0] = NV04_TEXTURED_TRIANGLE_CONTROL_Z_FORMAT_FIXED |
			NV04_TEXTURED_TRIANGLE_CONTROL_ORIGIN_CORNER;
	nv04->ctrl[1] = 0;
	nv04->ctrl[2] = 0;

	/* Dithering. */
	if (ctx->Color.DitherFlag)
		nv04->ctrl[0] |= NV04_TEXTURED_TRIANGLE_CONTROL_DITHER_ENABLE;

	/* Cull mode. */
	if (!ctx->Polygon.CullFlag)
		nv04->ctrl[0] |= NV04_TEXTURED_TRIANGLE_CONTROL_CULL_MODE_NONE;
	else if (cull == GL_FRONT_AND_BACK)
		nv04->ctrl[0] |= NV04_TEXTURED_TRIANGLE_CONTROL_CULL_MODE_BOTH;
	else
		nv04->ctrl[0] |= (cull == GL_FRONT) ^ (front == GL_CCW) ?
				 NV04_TEXTURED_TRIANGLE_CONTROL_CULL_MODE_CW :
				 NV04_TEXTURED_TRIANGLE_CONTROL_CULL_MODE_CCW;

	/* Depth test. */
	if (ctx->Depth.Test)
		nv04->ctrl[0] |= NV04_TEXTURED_TRIANGLE_CONTROL_Z_ENABLE;
	if (ctx->Depth.Mask)
		nv04->ctrl[0] |= NV04_TEXTURED_TRIANGLE_CONTROL_Z_WRITE;

	nv04->ctrl[0] |= get_comparison_op(ctx->Depth.Func) << 16;

	/* Alpha test. */
	if (ctx->Color.AlphaEnabled)
		nv04->ctrl[0] |= NV04_TEXTURED_TRIANGLE_CONTROL_ALPHA_ENABLE;

	nv04->ctrl[0] |= get_comparison_op(ctx->Color.AlphaFunc) << 8 |
			 FLOAT_TO_UBYTE(ctx->Color.AlphaRef);

	/* Color mask. */
	if (ctx->Color.ColorMask[0][RCOMP])
		nv04->ctrl[0] |= NV04_MULTITEX_TRIANGLE_CONTROL0_RED_WRITE;
	if (ctx->Color.ColorMask[0][GCOMP])
		nv04->ctrl[0] |= NV04_MULTITEX_TRIANGLE_CONTROL0_GREEN_WRITE;
	if (ctx->Color.ColorMask[0][BCOMP])
		nv04->ctrl[0] |= NV04_MULTITEX_TRIANGLE_CONTROL0_BLUE_WRITE;
	if (ctx->Color.ColorMask[0][ACOMP])
		nv04->ctrl[0] |= NV04_MULTITEX_TRIANGLE_CONTROL0_ALPHA_WRITE;

	/* Stencil test. */
	if (ctx->Stencil.WriteMask[0])
		nv04->ctrl[0] |= NV04_MULTITEX_TRIANGLE_CONTROL0_STENCIL_WRITE;

	if (ctx->Stencil.Enabled)
		nv04->ctrl[1] |= NV04_MULTITEX_TRIANGLE_CONTROL1_STENCIL_ENABLE;

	nv04->ctrl[1] |= get_comparison_op(ctx->Stencil.Function[0]) << 4 |
			 ctx->Stencil.Ref[0] << 8 |
			 ctx->Stencil.ValueMask[0] << 16 |
			 ctx->Stencil.WriteMask[0] << 24;

	nv04->ctrl[2] |= get_stencil_op(ctx->Stencil.ZPassFunc[0]) << 8 |
			 get_stencil_op(ctx->Stencil.ZFailFunc[0]) << 4 |
			 get_stencil_op(ctx->Stencil.FailFunc[0]);
}

void
nv04_defer_blend(struct gl_context *ctx, int emit)
{
	context_dirty(ctx, BLEND);
}

void
nv04_emit_blend(struct gl_context *ctx, int emit)
{
	struct nv04_context *nv04 = to_nv04_context(ctx);

	nv04->blend &= NV04_TEXTURED_TRIANGLE_BLEND_TEXTURE_MAP__MASK;
	nv04->blend |= NV04_TEXTURED_TRIANGLE_BLEND_MASK_BIT_MSB |
		       NV04_TEXTURED_TRIANGLE_BLEND_TEXTURE_PERSPECTIVE_ENABLE;

	/* Alpha blending. */
	nv04->blend |= get_blend_func(ctx->Color.Blend[0].DstRGB) << 28 |
		       get_blend_func(ctx->Color.Blend[0].SrcRGB) << 24;

	if (ctx->Color.BlendEnabled)
		nv04->blend |= NV04_TEXTURED_TRIANGLE_BLEND_BLEND_ENABLE;

	/* Shade model. */
	if (ctx->Light.ShadeModel == GL_SMOOTH)
		nv04->blend |= NV04_TEXTURED_TRIANGLE_BLEND_SHADE_MODE_GOURAUD;
	else
		nv04->blend |= NV04_TEXTURED_TRIANGLE_BLEND_SHADE_MODE_FLAT;

	/* Secondary color */
	if (_mesa_need_secondary_color(ctx))
		nv04->blend |= NV04_TEXTURED_TRIANGLE_BLEND_SPECULAR_ENABLE;

	/* Fog. */
	if (ctx->Fog.Enabled) {
		nv04->blend |= NV04_TEXTURED_TRIANGLE_BLEND_FOG_ENABLE;
		nv04->fog = pack_rgba_f(MESA_FORMAT_ARGB8888, ctx->Fog.Color);
	}
}
