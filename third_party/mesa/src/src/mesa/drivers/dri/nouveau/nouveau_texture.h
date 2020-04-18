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

#ifndef __NOUVEAU_TEXTURE_H__
#define __NOUVEAU_TEXTURE_H__

#include "swrast/s_context.h"

struct nouveau_teximage {
	struct swrast_texture_image base;
	struct nouveau_surface surface;
	struct {
		struct nouveau_surface surface;
		int x, y;
	} transfer;
};
#define to_nouveau_teximage(x) ((struct nouveau_teximage *)(x))

struct nouveau_texture {
	struct gl_texture_object base;
	struct nouveau_surface surfaces[MAX_TEXTURE_LEVELS];
	GLboolean dirty;
};
#define to_nouveau_texture(x) ((struct nouveau_texture *)(x))

#define texture_dirty(t) \
	to_nouveau_texture(t)->dirty = GL_TRUE

void
nouveau_set_texbuffer(__DRIcontext *dri_ctx,
		      GLint target, GLint format,
		      __DRIdrawable *draw);

GLboolean
nouveau_texture_validate(struct gl_context *ctx, struct gl_texture_object *t);

void
nouveau_texture_reallocate(struct gl_context *ctx, struct gl_texture_object *t);

#endif
