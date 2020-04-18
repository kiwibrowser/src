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
#include "nv10_driver.h"
#include "nv20_driver.h"

void
nv20_emit_tex_env(struct gl_context *ctx, int emit)
{
	const int i = emit - NOUVEAU_STATE_TEX_ENV0;
	struct nouveau_pushbuf *push = context_push(ctx);
	uint32_t a_in, a_out, c_in, c_out, k;

	nv10_get_general_combiner(ctx, i, &a_in, &a_out, &c_in, &c_out, &k);

	BEGIN_NV04(push, NV20_3D(RC_IN_ALPHA(i)), 1);
	PUSH_DATA (push, a_in);
	BEGIN_NV04(push, NV20_3D(RC_OUT_ALPHA(i)), 1);
	PUSH_DATA (push, a_out);
	BEGIN_NV04(push, NV20_3D(RC_IN_RGB(i)), 1);
	PUSH_DATA (push, c_in);
	BEGIN_NV04(push, NV20_3D(RC_OUT_RGB(i)), 1);
	PUSH_DATA (push, c_out);
	BEGIN_NV04(push, NV20_3D(RC_CONSTANT_COLOR0(i)), 1);
	PUSH_DATA (push, k);

	context_dirty(ctx, FRAG);
}

void
nv20_emit_frag(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	uint64_t in;
	int n;

	nv10_get_final_combiner(ctx, &in, &n);

	BEGIN_NV04(push, NV20_3D(RC_FINAL0), 2);
	PUSH_DATA (push, in);
	PUSH_DATA (push, in >> 32);

	BEGIN_NV04(push, NV20_3D(RC_ENABLE), 1);
	PUSH_DATA (push, n);
}
