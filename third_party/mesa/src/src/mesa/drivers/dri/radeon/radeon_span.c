/**************************************************************************

Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.
Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */

#include "main/glheader.h"
#include "main/texformat.h"
#include "main/renderbuffer.h"
#include "main/samplerobj.h"
#include "swrast/swrast.h"
#include "swrast/s_renderbuffer.h"

#include "radeon_common.h"
#include "radeon_span.h"


static void
radeon_renderbuffer_map(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
	struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);
	GLubyte *map;
	int stride;

	if (!rb || !rrb)
		return;

	ctx->Driver.MapRenderbuffer(ctx, rb, 0, 0, rb->Width, rb->Height,
				    GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
				    &map, &stride);

	rrb->base.Map = map;
	rrb->base.RowStride = stride;
	/* No floating point color buffers, use GLubytes */
	rrb->base.ColorType = GL_UNSIGNED_BYTE;
}

static void
radeon_renderbuffer_unmap(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
	struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);
	if (!rb || !rrb)
		return;

	ctx->Driver.UnmapRenderbuffer(ctx, rb);

	rrb->base.Map = NULL;
	rrb->base.RowStride = 0;
}

static void
radeon_map_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb)
{
	GLuint i;

	radeon_print(RADEON_MEMORY, RADEON_TRACE,
		"%s( %p , fb %p )\n",
		     __func__, ctx, fb);

	/* check for render to textures */
	for (i = 0; i < BUFFER_COUNT; i++)
		radeon_renderbuffer_map(ctx, fb->Attachment[i].Renderbuffer);

	radeon_check_front_buffer_rendering(ctx);
}

static void
radeon_unmap_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb)
{
	GLuint i;

	radeon_print(RADEON_MEMORY, RADEON_TRACE,
		"%s( %p , fb %p)\n",
		     __func__, ctx, fb);

	/* check for render to textures */
	for (i = 0; i < BUFFER_COUNT; i++)
		radeon_renderbuffer_unmap(ctx, fb->Attachment[i].Renderbuffer);

	radeon_check_front_buffer_rendering(ctx);
}

static void radeonSpanRenderStart(struct gl_context * ctx)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	int i;

	radeon_firevertices(rmesa);

	for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
		if (ctx->Texture.Unit[i]._ReallyEnabled) {
			radeon_validate_texture_miptree(ctx, _mesa_get_samplerobj(ctx, i),
							ctx->Texture.Unit[i]._Current);
			radeon_swrast_map_texture_images(ctx, ctx->Texture.Unit[i]._Current);
		}
	}
	
	radeon_map_framebuffer(ctx, ctx->DrawBuffer);
	if (ctx->ReadBuffer != ctx->DrawBuffer)
		radeon_map_framebuffer(ctx, ctx->ReadBuffer);
}

static void radeonSpanRenderFinish(struct gl_context * ctx)
{
	int i;

	_swrast_flush(ctx);

	for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++)
		if (ctx->Texture.Unit[i]._ReallyEnabled)
			radeon_swrast_unmap_texture_images(ctx, ctx->Texture.Unit[i]._Current);

	radeon_unmap_framebuffer(ctx, ctx->DrawBuffer);
	if (ctx->ReadBuffer != ctx->DrawBuffer)
		radeon_unmap_framebuffer(ctx, ctx->ReadBuffer);
}

void radeonInitSpanFuncs(struct gl_context * ctx)
{
	struct swrast_device_driver *swdd =
	    _swrast_GetDeviceDriverReference(ctx);
	swdd->SpanRenderStart = radeonSpanRenderStart;
	swdd->SpanRenderFinish = radeonSpanRenderFinish;
}

