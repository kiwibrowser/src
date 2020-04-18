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
#include "nouveau_fbo.h"
#include "nouveau_util.h"
#include "nv04_3d.xml.h"
#include "nv04_driver.h"

static inline unsigned
get_rt_format(gl_format format)
{
	switch (format) {
	case MESA_FORMAT_XRGB8888:
		return NV04_CONTEXT_SURFACES_3D_FORMAT_COLOR_X8R8G8B8_X8R8G8B8;
	case MESA_FORMAT_ARGB8888:
		return NV04_CONTEXT_SURFACES_3D_FORMAT_COLOR_A8R8G8B8;
	case MESA_FORMAT_RGB565:
		return NV04_CONTEXT_SURFACES_3D_FORMAT_COLOR_R5G6B5;
	default:
		assert(0);
	}
}

void
nv04_emit_framebuffer(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	struct gl_framebuffer *fb = ctx->DrawBuffer;
	struct nouveau_surface *s;
	uint32_t rt_format = NV04_CONTEXT_SURFACES_3D_FORMAT_TYPE_PITCH;
	uint32_t rt_pitch = 0, zeta_pitch = 0;
	unsigned bo_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR;

	if (fb->_Status != GL_FRAMEBUFFER_COMPLETE_EXT)
		return;

	PUSH_RESET(push, BUFCTX_FB);

	/* Render target */
	if (fb->_ColorDrawBuffers[0]) {
		s = &to_nouveau_renderbuffer(
			fb->_ColorDrawBuffers[0])->surface;

		rt_format |= get_rt_format(s->format);
		zeta_pitch = rt_pitch = s->pitch;

		BEGIN_NV04(push, NV04_SF3D(OFFSET_COLOR), 1);
		PUSH_MTHDl(push, NV04_SF3D(OFFSET_COLOR), BUFCTX_FB,
				 s->bo, 0, bo_flags);
	}

	/* depth/stencil */
	if (fb->Attachment[BUFFER_DEPTH].Renderbuffer) {
		s = &to_nouveau_renderbuffer(
			fb->Attachment[BUFFER_DEPTH].Renderbuffer)->surface;

		zeta_pitch = s->pitch;

		BEGIN_NV04(push, NV04_SF3D(OFFSET_ZETA), 1);
		PUSH_MTHDl(push, NV04_SF3D(OFFSET_ZETA), BUFCTX_FB,
				 s->bo, 0, bo_flags);
	}

	BEGIN_NV04(push, NV04_SF3D(FORMAT), 1);
	PUSH_DATA (push, rt_format);
	BEGIN_NV04(push, NV04_SF3D(PITCH), 1);
	PUSH_DATA (push, zeta_pitch << 16 | rt_pitch);

	/* Recompute the scissor state. */
	context_dirty(ctx, SCISSOR);
}

void
nv04_emit_scissor(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	int x, y, w, h;

	get_scissors(ctx->DrawBuffer, &x, &y, &w, &h);

	BEGIN_NV04(push, NV04_SF3D(CLIP_HORIZONTAL), 2);
	PUSH_DATA (push, w << 16 | x);
	PUSH_DATA (push, h << 16 | y);
}
