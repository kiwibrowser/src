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
#include "nouveau_util.h"
#include "nv10_3d.xml.h"
#include "nv10_driver.h"

void
nv10_emit_cull_face(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	GLenum mode = ctx->Polygon.CullFaceMode;

	BEGIN_NV04(push, NV10_3D(CULL_FACE_ENABLE), 1);
	PUSH_DATAb(push, ctx->Polygon.CullFlag);

	BEGIN_NV04(push, NV10_3D(CULL_FACE), 1);
	PUSH_DATA (push, (mode == GL_FRONT ? NV10_3D_CULL_FACE_FRONT :
			mode == GL_BACK ? NV10_3D_CULL_FACE_BACK :
			NV10_3D_CULL_FACE_FRONT_AND_BACK));
}

void
nv10_emit_front_face(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);

	BEGIN_NV04(push, NV10_3D(FRONT_FACE), 1);
	PUSH_DATA (push, ctx->Polygon.FrontFace == GL_CW ?
		 NV10_3D_FRONT_FACE_CW : NV10_3D_FRONT_FACE_CCW);
}

void
nv10_emit_line_mode(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	GLboolean smooth = ctx->Line.SmoothFlag &&
		ctx->Hint.LineSmooth == GL_NICEST;

	BEGIN_NV04(push, NV10_3D(LINE_WIDTH), 1);
	PUSH_DATA (push, MAX2(smooth ? 0 : 1,
			    ctx->Line.Width) * 8);
	BEGIN_NV04(push, NV10_3D(LINE_SMOOTH_ENABLE), 1);
	PUSH_DATAb(push, smooth);
}

void
nv10_emit_line_stipple(struct gl_context *ctx, int emit)
{
}

void
nv10_emit_point_mode(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);

	BEGIN_NV04(push, NV10_3D(POINT_SIZE), 1);
	PUSH_DATA (push, (uint32_t)(ctx->Point.Size * 8));

	BEGIN_NV04(push, NV10_3D(POINT_SMOOTH_ENABLE), 1);
	PUSH_DATAb(push, ctx->Point.SmoothFlag);
}

void
nv10_emit_polygon_mode(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);

	BEGIN_NV04(push, NV10_3D(POLYGON_MODE_FRONT), 2);
	PUSH_DATA (push, nvgl_polygon_mode(ctx->Polygon.FrontMode));
	PUSH_DATA (push, nvgl_polygon_mode(ctx->Polygon.BackMode));

	BEGIN_NV04(push, NV10_3D(POLYGON_SMOOTH_ENABLE), 1);
	PUSH_DATAb(push, ctx->Polygon.SmoothFlag);
}

void
nv10_emit_polygon_offset(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);

	BEGIN_NV04(push, NV10_3D(POLYGON_OFFSET_POINT_ENABLE), 3);
	PUSH_DATAb(push, ctx->Polygon.OffsetPoint);
	PUSH_DATAb(push, ctx->Polygon.OffsetLine);
	PUSH_DATAb(push, ctx->Polygon.OffsetFill);

	BEGIN_NV04(push, NV10_3D(POLYGON_OFFSET_FACTOR), 2);
	PUSH_DATAf(push, ctx->Polygon.OffsetFactor);
	PUSH_DATAf(push, ctx->Polygon.OffsetUnits);
}

void
nv10_emit_polygon_stipple(struct gl_context *ctx, int emit)
{
}
