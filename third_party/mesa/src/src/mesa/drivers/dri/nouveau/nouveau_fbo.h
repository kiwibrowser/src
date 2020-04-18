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

#ifndef __NOUVEAU_FBO_H__
#define __NOUVEAU_FBO_H__

struct nouveau_framebuffer {
	struct gl_framebuffer base;
	GLboolean need_front;

	struct {
		struct nouveau_bo *bo;
		uint32_t clear_value;
	} hierz;
};
#define to_nouveau_framebuffer(x) ((struct nouveau_framebuffer *)(x))

struct nouveau_renderbuffer {
	struct gl_renderbuffer base;
	struct nouveau_surface surface;
};
#define to_nouveau_renderbuffer(x) ((struct nouveau_renderbuffer *)(x))

struct gl_framebuffer *
nouveau_framebuffer_dri_new(const struct gl_config *visual);

struct gl_renderbuffer *
nouveau_renderbuffer_dri_new(GLenum format, __DRIdrawable *drawable);

void
nouveau_fbo_functions_init(struct dd_function_table *functions);

#endif
