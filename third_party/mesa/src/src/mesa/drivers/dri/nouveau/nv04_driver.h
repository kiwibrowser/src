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

#ifndef __NV04_DRIVER_H__
#define __NV04_DRIVER_H__

#include "nv04_context.h"

enum {
	NOUVEAU_STATE_BLEND = NUM_NOUVEAU_STATE,
	NOUVEAU_STATE_CONTROL,
	NUM_NV04_STATE
};

#define NV04_TEXTURE_UNITS 2

/* nv04_render.c */
void
nv04_render_init(struct gl_context *ctx);

void
nv04_render_destroy(struct gl_context *ctx);

/* nv04_surface.c */
GLboolean
nv04_surface_init(struct gl_context *ctx);

void
nv04_surface_takedown(struct gl_context *ctx);

void
nv04_surface_copy(struct gl_context *ctx,
		  struct nouveau_surface *dst, struct nouveau_surface *src,
		  int dx, int dy, int sx, int sy, int w, int h);

void
nv04_surface_fill(struct gl_context *ctx,
		  struct nouveau_surface *dst,
		  unsigned mask, unsigned value,
		  int dx, int dy, int w, int h);

/* nv04_state_fb.c */
void
nv04_emit_framebuffer(struct gl_context *ctx, int emit);

void
nv04_emit_scissor(struct gl_context *ctx, int emit);

/* nv04_state_raster.c */
void
nv04_defer_control(struct gl_context *ctx, int emit);

void
nv04_emit_control(struct gl_context *ctx, int emit);

void
nv04_defer_blend(struct gl_context *ctx, int emit);

void
nv04_emit_blend(struct gl_context *ctx, int emit);

/* nv04_state_frag.c */
void
nv04_emit_tex_env(struct gl_context *ctx, int emit);

/* nv04_state_tex.c */
void
nv04_emit_tex_obj(struct gl_context *ctx, int emit);

#endif
