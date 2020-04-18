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
#include "nouveau_bufferobj.h"
#include "nouveau_fbo.h"
#include "nv_object.xml.h"

#include "main/dd.h"
#include "main/framebuffer.h"
#include "main/fbobject.h"
#include "main/light.h"
#include "main/state.h"
#include "main/version.h"
#include "drivers/common/meta.h"
#include "drivers/common/driverfuncs.h"
#include "swrast/swrast.h"
#include "swrast/s_context.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"

GLboolean
nouveau_context_create(gl_api api,
		       const struct gl_config *visual, __DRIcontext *dri_ctx,
		       unsigned major_version,
		       unsigned minor_version,
		       uint32_t flags,
		       unsigned *error,
		       void *share_ctx)
{
	__DRIscreen *dri_screen = dri_ctx->driScreenPriv;
	struct nouveau_screen *screen = dri_screen->driverPrivate;
	struct nouveau_context *nctx;
	struct gl_context *ctx;

	switch (api) {
	case API_OPENGL:
		/* Do after-the-fact version checking (below).
		 */
		break;
	case API_OPENGLES:
		/* NV10 and NV20 can support OpenGL ES 1.0 only.  Older chips
		 * cannot do even that.
		 */
		if ((screen->device->chipset & 0xf0) == 0x00) {
			*error = __DRI_CTX_ERROR_BAD_API;
			return GL_FALSE;
		} else if (minor_version != 0) {
			*error = __DRI_CTX_ERROR_BAD_VERSION;
			return GL_FALSE;
		}
		break;
	case API_OPENGLES2:
	case API_OPENGL_CORE:
		*error = __DRI_CTX_ERROR_BAD_API;
		return GL_FALSE;
	}

	/* API and flag filtering is handled in dri2CreateContextAttribs.
	 */
	(void) flags;

	ctx = screen->driver->context_create(screen, visual, share_ctx);
	if (!ctx) {
		*error = __DRI_CTX_ERROR_NO_MEMORY;
		return GL_FALSE;
	}

	nctx = to_nouveau_context(ctx);
	nctx->dri_context = dri_ctx;
	dri_ctx->driverPrivate = ctx;

	_mesa_compute_version(ctx);
	if (ctx->Version < major_version * 10 + minor_version) {
	   nouveau_context_destroy(dri_ctx);
	   *error = __DRI_CTX_ERROR_BAD_VERSION;
	   return GL_FALSE;
	}

	if (nouveau_bo_new(context_dev(ctx), NOUVEAU_BO_VRAM, 0, 4096,
			   NULL, &nctx->fence)) {
		nouveau_context_destroy(dri_ctx);
		*error = __DRI_CTX_ERROR_NO_MEMORY;
		return GL_FALSE;
	}

	*error = __DRI_CTX_ERROR_SUCCESS;
	return GL_TRUE;
}

GLboolean
nouveau_context_init(struct gl_context *ctx, struct nouveau_screen *screen,
		     const struct gl_config *visual, struct gl_context *share_ctx)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct dd_function_table functions;
	int ret;

	nctx->screen = screen;
	nctx->fallback = HWTNL;

	/* Initialize the function pointers. */
	_mesa_init_driver_functions(&functions);
	nouveau_driver_functions_init(&functions);
	nouveau_bufferobj_functions_init(&functions);
	nouveau_texture_functions_init(&functions);
	nouveau_fbo_functions_init(&functions);

	/* Initialize the mesa context. */
	_mesa_initialize_context(ctx, API_OPENGL, visual,
                                 share_ctx, &functions, NULL);

	nouveau_state_init(ctx);
	nouveau_scratch_init(ctx);
	_mesa_meta_init(ctx);
	_swrast_CreateContext(ctx);
	_vbo_CreateContext(ctx);
	_tnl_CreateContext(ctx);
	nouveau_span_functions_init(ctx);
	_mesa_allow_light_in_model(ctx, GL_FALSE);

	/* Allocate a hardware channel. */
	ret = nouveau_object_new(&context_dev(ctx)->object, 0xbeef0000,
				 NOUVEAU_FIFO_CHANNEL_CLASS,
				 &(struct nv04_fifo){
					.vram = 0xbeef0201,
					.gart = 0xbeef0202
				 }, sizeof(struct nv04_fifo), &nctx->hw.chan);
	if (ret) {
		nouveau_error("Error initializing the FIFO.\n");
		return GL_FALSE;
	}

	/* Allocate a client (thread data) */
	ret = nouveau_client_new(context_dev(ctx), &nctx->hw.client);
	if (ret) {
		nouveau_error("Error creating thread data\n");
		return GL_FALSE;
	}

	/* Allocate a push buffer */
	ret = nouveau_pushbuf_new(nctx->hw.client, nctx->hw.chan, 4,
				  512 * 1024, true, &nctx->hw.pushbuf);
	if (ret) {
		nouveau_error("Error allocating DMA push buffer\n");
		return GL_FALSE;
	}

	/* Allocate buffer context */
	ret = nouveau_bufctx_new(nctx->hw.client, 16, &nctx->hw.bufctx);
	if (ret) {
		nouveau_error("Error allocating buffer context\n");
		return GL_FALSE;
	}

	nctx->hw.pushbuf->user_priv = nctx->hw.bufctx;

	/* Allocate NULL object */
	ret = nouveau_object_new(nctx->hw.chan, 0x00000000, NV01_NULL_CLASS,
				 NULL, 0, &nctx->hw.null);
	if (ret) {
		nouveau_error("Error allocating NULL object\n");
		return GL_FALSE;
	}

	/* Enable any supported extensions. */
	ctx->Extensions.EXT_blend_color = true;
	ctx->Extensions.EXT_blend_minmax = true;
	ctx->Extensions.EXT_fog_coord = true;
	ctx->Extensions.EXT_framebuffer_blit = true;
	ctx->Extensions.EXT_framebuffer_object = true;
	ctx->Extensions.EXT_packed_depth_stencil = true;
	ctx->Extensions.EXT_secondary_color = true;
	ctx->Extensions.EXT_texture_filter_anisotropic = true;
	ctx->Extensions.NV_blend_square = true;
	ctx->Extensions.NV_texture_env_combine4 = true;

	return GL_TRUE;
}

void
nouveau_context_deinit(struct gl_context *ctx)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);

	if (TNL_CONTEXT(ctx))
		_tnl_DestroyContext(ctx);

	if (vbo_context(ctx))
		_vbo_DestroyContext(ctx);

	if (SWRAST_CONTEXT(ctx))
		_swrast_DestroyContext(ctx);

	if (ctx->Meta)
		_mesa_meta_free(ctx);

	nouveau_bufctx_del(&nctx->hw.bufctx);
	nouveau_pushbuf_del(&nctx->hw.pushbuf);
	nouveau_client_del(&nctx->hw.client);
	nouveau_object_del(&nctx->hw.chan);

	nouveau_scratch_destroy(ctx);
	_mesa_free_context_data(ctx);
}

void
nouveau_context_destroy(__DRIcontext *dri_ctx)
{
	struct nouveau_context *nctx = dri_ctx->driverPrivate;
	struct gl_context *ctx = &nctx->base;

	nouveau_bo_ref(NULL, &nctx->fence);
	context_drv(ctx)->context_destroy(ctx);
}

void
nouveau_update_renderbuffers(__DRIcontext *dri_ctx, __DRIdrawable *draw)
{
	struct gl_context *ctx = dri_ctx->driverPrivate;
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	__DRIscreen *screen = dri_ctx->driScreenPriv;
	struct gl_framebuffer *fb = draw->driverPrivate;
	struct nouveau_framebuffer *nfb = to_nouveau_framebuffer(fb);
	unsigned int attachments[10];
	__DRIbuffer *buffers = NULL;
	int i = 0, count, ret;

	if (draw->lastStamp == draw->dri2.stamp)
		return;
	draw->lastStamp = draw->dri2.stamp;

	if (nfb->need_front)
		attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
	if (fb->Visual.doubleBufferMode)
		attachments[i++] = __DRI_BUFFER_BACK_LEFT;
	if (fb->Visual.haveDepthBuffer && fb->Visual.haveStencilBuffer)
		attachments[i++] = __DRI_BUFFER_DEPTH_STENCIL;
	else if (fb->Visual.haveDepthBuffer)
		attachments[i++] = __DRI_BUFFER_DEPTH;
	else if (fb->Visual.haveStencilBuffer)
		attachments[i++] = __DRI_BUFFER_STENCIL;

	buffers = (*screen->dri2.loader->getBuffers)(draw, &draw->w, &draw->h,
						     attachments, i, &count,
						     draw->loaderPrivate);
	if (buffers == NULL)
		return;

	for (i = 0; i < count; i++) {
		struct gl_renderbuffer *rb;
		struct nouveau_surface *s;
		uint32_t old_name;
		int index;

		switch (buffers[i].attachment) {
		case __DRI_BUFFER_FRONT_LEFT:
		case __DRI_BUFFER_FAKE_FRONT_LEFT:
			index = BUFFER_FRONT_LEFT;
			break;
		case __DRI_BUFFER_BACK_LEFT:
			index = BUFFER_BACK_LEFT;
			break;
		case __DRI_BUFFER_DEPTH:
		case __DRI_BUFFER_DEPTH_STENCIL:
			index = BUFFER_DEPTH;
			break;
		case __DRI_BUFFER_STENCIL:
			index = BUFFER_STENCIL;
			break;
		default:
			assert(0);
		}

		rb = fb->Attachment[index].Renderbuffer;
		s = &to_nouveau_renderbuffer(rb)->surface;

		s->width = draw->w;
		s->height = draw->h;
		s->pitch = buffers[i].pitch;
		s->cpp = buffers[i].cpp;

		if (index == BUFFER_DEPTH && s->bo) {
			ret = nouveau_bo_name_get(s->bo, &old_name);
			/*
			 * Disable fast Z clears in the next frame, the
			 * depth buffer contents are undefined.
			 */
			if (!ret && old_name != buffers[i].name)
				nctx->hierz.clear_seq = 0;
		}

		nouveau_bo_ref(NULL, &s->bo);
		ret = nouveau_bo_name_ref(context_dev(ctx),
					  buffers[i].name, &s->bo);
		assert(!ret);
	}

	_mesa_resize_framebuffer(ctx, fb, draw->w, draw->h);
}

static void
update_framebuffer(__DRIcontext *dri_ctx, __DRIdrawable *draw,
		   int *stamp)
{
	struct gl_context *ctx = dri_ctx->driverPrivate;
	struct gl_framebuffer *fb = draw->driverPrivate;

	*stamp = draw->dri2.stamp;

	nouveau_update_renderbuffers(dri_ctx, draw);
	_mesa_resize_framebuffer(ctx, fb, draw->w, draw->h);

	/* Clean up references to the old framebuffer objects. */
	context_dirty(ctx, FRAMEBUFFER);
	nouveau_bufctx_reset(to_nouveau_context(ctx)->hw.bufctx, BUFCTX_FB);
	PUSH_KICK(context_push(ctx));
}

GLboolean
nouveau_context_make_current(__DRIcontext *dri_ctx, __DRIdrawable *dri_draw,
			     __DRIdrawable *dri_read)
{
	if (dri_ctx) {
		struct nouveau_context *nctx = dri_ctx->driverPrivate;
		struct gl_context *ctx = &nctx->base;

		/* Ask the X server for new renderbuffers. */
		if (dri_draw->driverPrivate != ctx->WinSysDrawBuffer)
			update_framebuffer(dri_ctx, dri_draw,
					   &dri_ctx->dri2.draw_stamp);

		if (dri_draw != dri_read &&
		    dri_read->driverPrivate != ctx->WinSysReadBuffer)
			update_framebuffer(dri_ctx, dri_read,
					   &dri_ctx->dri2.read_stamp);

		/* Pass it down to mesa. */
		_mesa_make_current(ctx, dri_draw->driverPrivate,
				   dri_read->driverPrivate);
		_mesa_update_state(ctx);

	} else {
		_mesa_make_current(NULL, NULL, NULL);
	}

	return GL_TRUE;
}

GLboolean
nouveau_context_unbind(__DRIcontext *dri_ctx)
{
	/* Unset current context and dispatch table */
	_mesa_make_current(NULL, NULL, NULL);

	return GL_TRUE;
}

void
nouveau_fallback(struct gl_context *ctx, enum nouveau_fallback mode)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);

	nctx->fallback = MAX2(HWTNL, mode);

	if (mode < SWRAST) {
		nouveau_state_emit(ctx);
#if 0
		nouveau_bo_state_emit(ctx);
#endif
	} else {
		PUSH_KICK(context_push(ctx));
	}
}

static void
validate_framebuffer(__DRIcontext *dri_ctx, __DRIdrawable *draw,
		     int *stamp)
{
	struct gl_framebuffer *fb = draw->driverPrivate;
	struct nouveau_framebuffer *nfb = to_nouveau_framebuffer(fb);
	GLboolean need_front =
		(fb->_ColorDrawBufferIndexes[0] == BUFFER_FRONT_LEFT ||
		 fb->_ColorReadBufferIndex == BUFFER_FRONT_LEFT);

	if (nfb->need_front != need_front) {
		nfb->need_front = need_front;
		dri2InvalidateDrawable(draw);
	}

	if (draw->dri2.stamp != *stamp)
		update_framebuffer(dri_ctx, draw, stamp);
}

void
nouveau_validate_framebuffer(struct gl_context *ctx)
{
	__DRIcontext *dri_ctx = to_nouveau_context(ctx)->dri_context;
	__DRIdrawable *dri_draw = dri_ctx->driDrawablePriv;
	__DRIdrawable *dri_read = dri_ctx->driReadablePriv;

	if (_mesa_is_winsys_fbo(ctx->DrawBuffer))
		validate_framebuffer(dri_ctx, dri_draw,
				     &dri_ctx->dri2.draw_stamp);

	if (_mesa_is_winsys_fbo(ctx->ReadBuffer))
		validate_framebuffer(dri_ctx, dri_read,
				     &dri_ctx->dri2.read_stamp);

	if (ctx->NewState & _NEW_BUFFERS)
		_mesa_update_state(ctx);
}
