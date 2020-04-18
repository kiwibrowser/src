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
#include "nouveau_fbo.h"
#include "nouveau_context.h"

#include "swrast/swrast.h"
#include "swrast/s_context.h"



static void
renderbuffer_map_unmap(struct gl_context *ctx, struct gl_renderbuffer *rb,
		       GLboolean map)
{
	struct nouveau_surface *s = &to_nouveau_renderbuffer(rb)->surface;

	if (map)
		nouveau_bo_map(s->bo, NOUVEAU_BO_RDWR, context_client(ctx));
}

static void
framebuffer_map_unmap(struct gl_context *ctx, struct gl_framebuffer *fb, GLboolean map)
{
	int i;

	for (i = 0; i < fb->_NumColorDrawBuffers; i++)
		renderbuffer_map_unmap(ctx, fb->_ColorDrawBuffers[i], map);

	renderbuffer_map_unmap(ctx, fb->_ColorReadBuffer, map);

	if (fb->Attachment[BUFFER_DEPTH].Renderbuffer)
		renderbuffer_map_unmap(ctx, fb->Attachment[BUFFER_DEPTH].Renderbuffer, map);
}

static void
span_map_unmap(struct gl_context *ctx, GLboolean map)
{
	int i;

	framebuffer_map_unmap(ctx, ctx->DrawBuffer, map);

	if (ctx->ReadBuffer != ctx->DrawBuffer)
		framebuffer_map_unmap(ctx, ctx->ReadBuffer, map);

	for (i = 0; i < ctx->Const.MaxTextureUnits; i++)
		if (map)
			_swrast_map_texture(ctx, ctx->Texture.Unit[i]._Current);
		else
			_swrast_unmap_texture(ctx, ctx->Texture.Unit[i]._Current);
}

static void
nouveau_span_start(struct gl_context *ctx)
{
	nouveau_fallback(ctx, SWRAST);
	span_map_unmap(ctx, GL_TRUE);
}

static void
nouveau_span_finish(struct gl_context *ctx)
{
	span_map_unmap(ctx, GL_FALSE);
	nouveau_fallback(ctx, HWTNL);
}

void
nouveau_span_functions_init(struct gl_context *ctx)
{
	struct swrast_device_driver *swdd =
		_swrast_GetDeviceDriverReference(ctx);

	swdd->SpanRenderStart = nouveau_span_start;
	swdd->SpanRenderFinish = nouveau_span_finish;
}
