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

#ifndef __NOUVEAU_UTIL_H__
#define __NOUVEAU_UTIL_H__

#include "main/formats.h"
#include "main/colormac.h"

static inline unsigned
pack_rgba_i(gl_format f, uint8_t c[])
{
	switch (f) {
	case MESA_FORMAT_ARGB8888:
		return PACK_COLOR_8888(c[ACOMP], c[RCOMP], c[GCOMP], c[BCOMP]);
	case MESA_FORMAT_ARGB8888_REV:
		return PACK_COLOR_8888(c[BCOMP], c[GCOMP], c[RCOMP], c[ACOMP]);
	case MESA_FORMAT_XRGB8888:
		return PACK_COLOR_8888(0, c[RCOMP], c[GCOMP], c[BCOMP]);
	case MESA_FORMAT_XRGB8888_REV:
		return PACK_COLOR_8888(c[BCOMP], c[GCOMP], c[RCOMP], 0);
	case MESA_FORMAT_RGBA8888:
		return PACK_COLOR_8888(c[RCOMP], c[GCOMP], c[BCOMP], c[ACOMP]);
	case MESA_FORMAT_RGBA8888_REV:
		return PACK_COLOR_8888(c[ACOMP], c[BCOMP], c[GCOMP], c[RCOMP]);
	case MESA_FORMAT_RGB565:
		return PACK_COLOR_565(c[RCOMP], c[GCOMP], c[BCOMP]);
	default:
		assert(0);
	}
}

static inline unsigned
pack_zs_i(gl_format f, uint32_t z, uint8_t s)
{
	switch (f) {
	case MESA_FORMAT_Z24_S8:
		return (z & 0xffffff00) | (s & 0xff);
	case MESA_FORMAT_Z24_X8:
		return (z & 0xffffff00);
	case MESA_FORMAT_Z16:
		return (z & 0xffff0000) >> 16;
	default:
		assert(0);
	}
}

static inline unsigned
pack_rgba_f(gl_format f, float c[])
{
	return pack_rgba_i(f, (uint8_t []) {
			   FLOAT_TO_UBYTE(c[RCOMP]),
			   FLOAT_TO_UBYTE(c[GCOMP]),
			   FLOAT_TO_UBYTE(c[BCOMP]),
			   FLOAT_TO_UBYTE(c[ACOMP]) });
}

static inline unsigned
pack_rgba_clamp_f(gl_format f, float c[])
{
	GLubyte bytes[4];
	_mesa_unclamped_float_rgba_to_ubyte(bytes, c);
	return pack_rgba_i(f, bytes);
}

static inline unsigned
pack_zs_f(gl_format f, float z, uint8_t s)
{
	return pack_zs_i(f, FLOAT_TO_UINT(z), s);
}

/* Integer base-2 logarithm, rounded towards zero. */
static inline unsigned
log2i(unsigned i)
{
	unsigned r = 0;

	if (i & 0xffff0000) {
		i >>= 16;
		r += 16;
	}
	if (i & 0x0000ff00) {
		i >>= 8;
		r += 8;
	}
	if (i & 0x000000f0) {
		i >>= 4;
		r += 4;
	}
	if (i & 0x0000000c) {
		i >>= 2;
		r += 2;
	}
	if (i & 0x00000002) {
		r += 1;
	}
	return r;
}

static inline unsigned
align(unsigned x, unsigned m)
{
	return (x + m - 1) & ~(m - 1);
}

static inline void
get_scissors(struct gl_framebuffer *fb, int *x, int *y, int *w, int *h)
{
	*w = fb->_Xmax - fb->_Xmin;
	*h = fb->_Ymax - fb->_Ymin;
	*x = fb->_Xmin;
	*y = (fb->Name ? fb->_Ymin :
	      /* Window system FBO: Flip the Y coordinate. */
	      fb->Height - fb->_Ymax);
}

static inline void
get_viewport_scale(struct gl_context *ctx, float a[16])
{
	struct gl_viewport_attrib *vp = &ctx->Viewport;
	struct gl_framebuffer *fb = ctx->DrawBuffer;

	a[MAT_SX] = (float)vp->Width / 2;

	if (fb->Name)
		a[MAT_SY] = (float)vp->Height / 2;
	else
		/* Window system FBO: Flip the Y coordinate. */
		a[MAT_SY] = - (float)vp->Height / 2;

	a[MAT_SZ] = fb->_DepthMaxF * (vp->Far - vp->Near) / 2;
}

static inline void
get_viewport_translate(struct gl_context *ctx, float a[4])
{
	struct gl_viewport_attrib *vp = &ctx->Viewport;
	struct gl_framebuffer *fb = ctx->DrawBuffer;

	a[0] = (float)vp->Width / 2 + vp->X;

	if (fb->Name)
		a[1] = (float)vp->Height / 2 + vp->Y;
	else
		/* Window system FBO: Flip the Y coordinate. */
		a[1] = fb->Height - (float)vp->Height / 2 - vp->Y;

	a[2] = fb->_DepthMaxF * (vp->Far + vp->Near) / 2;
}

static inline GLboolean
is_color_operand(int op)
{
	return op == GL_SRC_COLOR || op == GL_ONE_MINUS_SRC_COLOR;
}

static inline GLboolean
is_negative_operand(int op)
{
	return op == GL_ONE_MINUS_SRC_COLOR || op == GL_ONE_MINUS_SRC_ALPHA;
}

static inline GLboolean
is_texture_source(int s)
{
	return s == GL_TEXTURE || (s >= GL_TEXTURE0 && s <= GL_TEXTURE31);
}

static inline struct gl_texgen *
get_texgen_coord(struct gl_texture_unit *u, int i)
{
	return ((struct gl_texgen *[])
		{ &u->GenS, &u->GenT, &u->GenR, &u->GenQ }) [i];
}

static inline float *
get_texgen_coeff(struct gl_texgen *c)
{
	if (c->Mode == GL_OBJECT_LINEAR)
		return c->ObjectPlane;
	else if (c->Mode == GL_EYE_LINEAR)
		return c->EyePlane;
	else
		return NULL;
}

static inline unsigned
get_format_blocksx(gl_format format,
		       unsigned x)
{
	GLuint blockwidth;
	GLuint blockheight;
	_mesa_get_format_block_size(format, &blockwidth, &blockheight);
	return (x + blockwidth - 1) / blockwidth;
}

static inline unsigned
get_format_blocksy(gl_format format,
		       unsigned y)
{
	GLuint blockwidth;
	GLuint blockheight;
	_mesa_get_format_block_size(format, &blockwidth, &blockheight);
	return (y + blockheight - 1) / blockheight;
}

#endif
