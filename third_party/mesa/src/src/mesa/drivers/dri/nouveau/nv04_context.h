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

#ifndef __NV04_CONTEXT_H__
#define __NV04_CONTEXT_H__

#include "nouveau_context.h"
#include "nv_object.xml.h"

struct nv04_context {
	struct nouveau_context base;
	struct nouveau_object *eng3d;
	struct nouveau_surface dummy_texture;
	float viewport[16];

	uint32_t colorkey;
	struct nouveau_surface *texture[2];
	uint32_t format[2];
	uint32_t filter[2];
	uint32_t alpha[2];
	uint32_t color[2];
	uint32_t factor;
	uint32_t blend;
	uint32_t ctrl[3];
	uint32_t fog;
};
#define to_nv04_context(ctx) ((struct nv04_context *)(ctx))

#define nv04_mtex_engine(obj) ((obj)->oclass == NV04_MULTITEX_TRIANGLE_CLASS)

struct nouveau_object *
nv04_context_engine(struct gl_context *ctx);

extern const struct nouveau_driver nv04_driver;

#endif
