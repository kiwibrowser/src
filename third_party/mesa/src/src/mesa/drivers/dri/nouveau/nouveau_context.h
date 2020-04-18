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

#ifndef __NOUVEAU_CONTEXT_H__
#define __NOUVEAU_CONTEXT_H__

#include "nouveau_screen.h"
#include "nouveau_state.h"
#include "nouveau_scratch.h"
#include "nouveau_render.h"

#include "main/bitset.h"

enum nouveau_fallback {
	HWTNL = 0,
	SWTNL,
	SWRAST,
};

#define BUFCTX_FB      0
#define BUFCTX_VTX     1
#define BUFCTX_TEX(i) (2 + (i))

struct nouveau_hw_state {
	struct nouveau_object *chan;
	struct nouveau_client *client;
	struct nouveau_pushbuf *pushbuf;
	struct nouveau_bufctx *bufctx;

	struct nouveau_object *null;
	struct nouveau_object *ntfy;
	struct nouveau_object *eng3d;
	struct nouveau_object *eng3dm;
	struct nouveau_object *surf3d;
	struct nouveau_object *m2mf;
	struct nouveau_object *surf2d;
	struct nouveau_object *rop;
	struct nouveau_object *patt;
	struct nouveau_object *rect;
	struct nouveau_object *swzsurf;
	struct nouveau_object *sifm;
};

struct nouveau_context {
	struct gl_context base;
	__DRIcontext *dri_context;
	struct nouveau_screen *screen;

	BITSET_DECLARE(dirty, MAX_NOUVEAU_STATE);
	enum nouveau_fallback fallback;

	struct nouveau_bo *fence;

	struct nouveau_hw_state hw;
	struct nouveau_render_state render;
	struct nouveau_scratch_state scratch;

	struct {
		GLboolean clear_blocked;
		int clear_seq;
	} hierz;
};

#define to_nouveau_context(ctx)	((struct nouveau_context *)(ctx))

#define context_dev(ctx) \
	(to_nouveau_context(ctx)->screen->device)
#define context_chipset(ctx) \
	(context_dev(ctx)->chipset)
#define context_chan(ctx) \
	(to_nouveau_context(ctx)->hw.chan)
#define context_client(ctx) \
	(to_nouveau_context(ctx)->hw.client)
#define context_push(ctx) \
	(to_nouveau_context(ctx)->hw.pushbuf)
#define context_eng3d(ctx) \
	(to_nouveau_context(ctx)->hw.eng3d)
#define context_drv(ctx) \
	(to_nouveau_context(ctx)->screen->driver)
#define context_dirty(ctx, s) \
	BITSET_SET(to_nouveau_context(ctx)->dirty, NOUVEAU_STATE_##s)
#define context_dirty_i(ctx, s, i) \
	BITSET_SET(to_nouveau_context(ctx)->dirty, NOUVEAU_STATE_##s##0 + i)
#define context_emit(ctx, s) \
	context_drv(ctx)->emit[NOUVEAU_STATE_##s](ctx, NOUVEAU_STATE_##s)

GLboolean
nouveau_context_create(gl_api api,
		       const struct gl_config *visual, __DRIcontext *dri_ctx,
		       unsigned major_version, unsigned minor_version,
		       uint32_t flags, unsigned *error, void *share_ctx);

GLboolean
nouveau_context_init(struct gl_context *ctx, struct nouveau_screen *screen,
		     const struct gl_config *visual, struct gl_context *share_ctx);

void
nouveau_context_deinit(struct gl_context *ctx);

void
nouveau_context_destroy(__DRIcontext *dri_ctx);

void
nouveau_update_renderbuffers(__DRIcontext *dri_ctx, __DRIdrawable *draw);

GLboolean
nouveau_context_make_current(__DRIcontext *dri_ctx, __DRIdrawable *ddraw,
			     __DRIdrawable *rdraw);

GLboolean
nouveau_context_unbind(__DRIcontext *dri_ctx);

void
nouveau_fallback(struct gl_context *ctx, enum nouveau_fallback mode);

void
nouveau_validate_framebuffer(struct gl_context *ctx);

#endif

