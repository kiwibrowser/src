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

#ifndef __NOUVEAU_SURFACE_H__
#define __NOUVEAU_SURFACE_H__

enum nouveau_surface_layout {
	LINEAR = 0,
	TILED,
	SWIZZLED,
};

struct nouveau_surface {
	struct nouveau_bo *bo;
	unsigned offset;

	enum nouveau_surface_layout layout;

	gl_format format;
	unsigned cpp, pitch;

	unsigned width, height;
};

void
nouveau_surface_alloc(struct gl_context *ctx, struct nouveau_surface *s,
		      enum nouveau_surface_layout layout,
		      unsigned flags, unsigned format,
		      unsigned width, unsigned height);

void
nouveau_surface_ref(struct nouveau_surface *src,
		    struct nouveau_surface *dst);

#endif
