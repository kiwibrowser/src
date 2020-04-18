/*
 * Copyright (C) 2007-2010 The Nouveau Project.
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
#include "nv_object.xml.h"
#include "nv_m2mf.xml.h"
#include "nv01_2d.xml.h"
#include "nv04_3d.xml.h"
#include "nouveau_context.h"
#include "nouveau_util.h"
#include "nv04_driver.h"

static inline int
swzsurf_format(gl_format format)
{
	switch (format) {
	case MESA_FORMAT_A8:
	case MESA_FORMAT_L8:
	case MESA_FORMAT_I8:
	case MESA_FORMAT_RGB332:
		return NV04_SWIZZLED_SURFACE_FORMAT_COLOR_Y8;

	case MESA_FORMAT_RGB565:
	case MESA_FORMAT_RGB565_REV:
	case MESA_FORMAT_ARGB4444:
	case MESA_FORMAT_ARGB4444_REV:
	case MESA_FORMAT_ARGB1555:
	case MESA_FORMAT_RGBA5551:
	case MESA_FORMAT_ARGB1555_REV:
	case MESA_FORMAT_AL88:
	case MESA_FORMAT_AL88_REV:
	case MESA_FORMAT_YCBCR:
	case MESA_FORMAT_YCBCR_REV:
	case MESA_FORMAT_Z16:
		return NV04_SWIZZLED_SURFACE_FORMAT_COLOR_R5G6B5;

	case MESA_FORMAT_RGBA8888:
	case MESA_FORMAT_RGBA8888_REV:
	case MESA_FORMAT_XRGB8888:
	case MESA_FORMAT_ARGB8888:
	case MESA_FORMAT_ARGB8888_REV:
	case MESA_FORMAT_S8_Z24:
	case MESA_FORMAT_Z24_S8:
	case MESA_FORMAT_Z32:
		return NV04_SWIZZLED_SURFACE_FORMAT_COLOR_A8R8G8B8;

	default:
		assert(0);
	}
}

static inline int
surf2d_format(gl_format format)
{
	switch (format) {
	case MESA_FORMAT_A8:
	case MESA_FORMAT_L8:
	case MESA_FORMAT_I8:
	case MESA_FORMAT_RGB332:
		return NV04_CONTEXT_SURFACES_2D_FORMAT_Y8;

	case MESA_FORMAT_RGB565:
	case MESA_FORMAT_RGB565_REV:
	case MESA_FORMAT_ARGB4444:
	case MESA_FORMAT_ARGB4444_REV:
	case MESA_FORMAT_ARGB1555:
	case MESA_FORMAT_RGBA5551:
	case MESA_FORMAT_ARGB1555_REV:
	case MESA_FORMAT_AL88:
	case MESA_FORMAT_AL88_REV:
	case MESA_FORMAT_YCBCR:
	case MESA_FORMAT_YCBCR_REV:
	case MESA_FORMAT_Z16:
		return NV04_CONTEXT_SURFACES_2D_FORMAT_R5G6B5;

	case MESA_FORMAT_RGBA8888:
	case MESA_FORMAT_RGBA8888_REV:
	case MESA_FORMAT_XRGB8888:
	case MESA_FORMAT_ARGB8888:
	case MESA_FORMAT_ARGB8888_REV:
	case MESA_FORMAT_S8_Z24:
	case MESA_FORMAT_Z24_S8:
	case MESA_FORMAT_Z32:
		return NV04_CONTEXT_SURFACES_2D_FORMAT_Y32;

	default:
		assert(0);
	}
}

static inline int
rect_format(gl_format format)
{
	switch (format) {
	case MESA_FORMAT_A8:
	case MESA_FORMAT_L8:
	case MESA_FORMAT_I8:
	case MESA_FORMAT_RGB332:
		return NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A8R8G8B8;

	case MESA_FORMAT_RGB565:
	case MESA_FORMAT_RGB565_REV:
	case MESA_FORMAT_ARGB4444:
	case MESA_FORMAT_ARGB4444_REV:
	case MESA_FORMAT_ARGB1555:
	case MESA_FORMAT_RGBA5551:
	case MESA_FORMAT_ARGB1555_REV:
	case MESA_FORMAT_AL88:
	case MESA_FORMAT_AL88_REV:
	case MESA_FORMAT_YCBCR:
	case MESA_FORMAT_YCBCR_REV:
	case MESA_FORMAT_Z16:
		return NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A16R5G6B5;

	case MESA_FORMAT_RGBA8888:
	case MESA_FORMAT_RGBA8888_REV:
	case MESA_FORMAT_XRGB8888:
	case MESA_FORMAT_ARGB8888:
	case MESA_FORMAT_ARGB8888_REV:
	case MESA_FORMAT_S8_Z24:
	case MESA_FORMAT_Z24_S8:
	case MESA_FORMAT_Z32:
		return NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A8R8G8B8;

	default:
		assert(0);
	}
}

static inline int
sifm_format(gl_format format)
{
	switch (format) {
	case MESA_FORMAT_A8:
	case MESA_FORMAT_L8:
	case MESA_FORMAT_I8:
	case MESA_FORMAT_RGB332:
		return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_AY8;

	case MESA_FORMAT_RGB565:
	case MESA_FORMAT_RGB565_REV:
	case MESA_FORMAT_ARGB4444:
	case MESA_FORMAT_ARGB4444_REV:
	case MESA_FORMAT_ARGB1555:
	case MESA_FORMAT_RGBA5551:
	case MESA_FORMAT_ARGB1555_REV:
	case MESA_FORMAT_AL88:
	case MESA_FORMAT_AL88_REV:
	case MESA_FORMAT_YCBCR:
	case MESA_FORMAT_YCBCR_REV:
	case MESA_FORMAT_Z16:
		return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_R5G6B5;

	case MESA_FORMAT_RGBA8888:
	case MESA_FORMAT_RGBA8888_REV:
	case MESA_FORMAT_XRGB8888:
	case MESA_FORMAT_ARGB8888:
	case MESA_FORMAT_ARGB8888_REV:
	case MESA_FORMAT_S8_Z24:
	case MESA_FORMAT_Z24_S8:
	case MESA_FORMAT_Z32:
		return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_A8R8G8B8;

	default:
		assert(0);
	}
}

static void
nv04_surface_copy_swizzle(struct gl_context *ctx,
			  struct nouveau_surface *dst,
			  struct nouveau_surface *src,
			  int dx, int dy, int sx, int sy,
			  int w, int h)
{
	struct nouveau_pushbuf_refn refs[] = {
		{ src->bo, NOUVEAU_BO_RD | NOUVEAU_BO_VRAM | NOUVEAU_BO_GART },
		{ dst->bo, NOUVEAU_BO_WR | NOUVEAU_BO_VRAM },
	};
	struct nouveau_pushbuf *push = context_push(ctx);
	struct nouveau_hw_state *hw = &to_nouveau_context(ctx)->hw;
	struct nouveau_object *swzsurf = hw->swzsurf;
	struct nv04_fifo *fifo = hw->chan->data;
	/* Max width & height may not be the same on all HW, but must be POT */
	const unsigned max_w = 1024;
	const unsigned max_h = 1024;
	unsigned sub_w = w > max_w ? max_w : w;
	unsigned sub_h = h > max_h ? max_h : h;
	unsigned x, y;

        /* Swizzled surfaces must be POT  */
	assert(_mesa_is_pow_two(dst->width) &&
	       _mesa_is_pow_two(dst->height));

	if (context_chipset(ctx) < 0x10) {
		BEGIN_NV04(push, NV01_SUBC(SURF, OBJECT), 1);
		PUSH_DATA (push, swzsurf->handle);
	}

	for (y = 0; y < h; y += sub_h) {
		sub_h = MIN2(sub_h, h - y);

		for (x = 0; x < w; x += sub_w) {
			sub_w = MIN2(sub_w, w - x);

			if (nouveau_pushbuf_space(push, 64, 4, 0) ||
			    nouveau_pushbuf_refn (push, refs, 2))
				return;

			BEGIN_NV04(push, NV04_SSWZ(DMA_IMAGE), 1);
			PUSH_DATA (push, fifo->vram);
			BEGIN_NV04(push, NV04_SSWZ(FORMAT), 2);
			PUSH_DATA (push, swzsurf_format(dst->format) |
					 log2i(dst->width) << 16 |
					 log2i(dst->height) << 24);
			PUSH_RELOC(push, dst->bo, dst->offset, NOUVEAU_BO_LOW, 0, 0);

			BEGIN_NV04(push, NV03_SIFM(DMA_IMAGE), 1);
			PUSH_RELOC(push, src->bo, 0, NOUVEAU_BO_OR, fifo->vram, fifo->gart);
			BEGIN_NV04(push, NV05_SIFM(SURFACE), 1);
			PUSH_DATA (push, swzsurf->handle);

			BEGIN_NV04(push, NV03_SIFM(COLOR_FORMAT), 8);
			PUSH_DATA (push, sifm_format(src->format));
			PUSH_DATA (push, NV03_SCALED_IMAGE_FROM_MEMORY_OPERATION_SRCCOPY);
			PUSH_DATA (push, (y + dy) << 16 | (x + dx));
			PUSH_DATA (push, sub_h << 16 | sub_w);
			PUSH_DATA (push, (y + dy) << 16 | (x + dx));
			PUSH_DATA (push, sub_h << 16 | sub_w);
			PUSH_DATA (push, 1 << 20);
			PUSH_DATA (push, 1 << 20);

			BEGIN_NV04(push, NV03_SIFM(SIZE), 4);
			PUSH_DATA (push, align(sub_h, 2) << 16 | align(sub_w, 2));
			PUSH_DATA (push, src->pitch  |
					 NV03_SCALED_IMAGE_FROM_MEMORY_FORMAT_ORIGIN_CENTER |
					 NV03_SCALED_IMAGE_FROM_MEMORY_FORMAT_FILTER_POINT_SAMPLE);
			PUSH_RELOC(push, src->bo, src->offset + (y + sy) * src->pitch +
					 (x + sx) * src->cpp, NOUVEAU_BO_LOW, 0, 0);
			PUSH_DATA (push, 0);
		}
	}

	if (context_chipset(ctx) < 0x10) {
		BEGIN_NV04(push, NV01_SUBC(SURF, OBJECT), 1);
		PUSH_DATA (push, hw->surf3d->handle);
	}
}

static void
nv04_surface_copy_m2mf(struct gl_context *ctx,
		       struct nouveau_surface *dst,
		       struct nouveau_surface *src,
		       int dx, int dy, int sx, int sy,
		       int w, int h)
{
	struct nouveau_pushbuf_refn refs[] = {
		{ src->bo, NOUVEAU_BO_RD | NOUVEAU_BO_VRAM | NOUVEAU_BO_GART },
		{ dst->bo, NOUVEAU_BO_WR | NOUVEAU_BO_VRAM | NOUVEAU_BO_GART },
	};
	struct nouveau_pushbuf *push = context_push(ctx);
	struct nouveau_hw_state *hw = &to_nouveau_context(ctx)->hw;
	struct nv04_fifo *fifo = hw->chan->data;
	unsigned dst_offset = dst->offset + dy * dst->pitch + dx * dst->cpp;
	unsigned src_offset = src->offset + sy * src->pitch + sx * src->cpp;

	while (h) {
		int count = (h > 2047) ? 2047 : h;

		if (nouveau_pushbuf_space(push, 16, 4, 0) ||
		    nouveau_pushbuf_refn (push, refs, 2))
			return;

		BEGIN_NV04(push, NV03_M2MF(DMA_BUFFER_IN), 2);
		PUSH_RELOC(push, src->bo, 0, NOUVEAU_BO_OR, fifo->vram, fifo->gart);
		PUSH_RELOC(push, dst->bo, 0, NOUVEAU_BO_OR, fifo->vram, fifo->gart);
		BEGIN_NV04(push, NV03_M2MF(OFFSET_IN), 8);
		PUSH_RELOC(push, src->bo, src->offset, NOUVEAU_BO_LOW, 0, 0);
		PUSH_RELOC(push, dst->bo, dst->offset, NOUVEAU_BO_LOW, 0, 0);
		PUSH_DATA (push, src->pitch);
		PUSH_DATA (push, dst->pitch);
		PUSH_DATA (push, w * src->cpp);
		PUSH_DATA (push, count);
		PUSH_DATA (push, 0x0101);
		PUSH_DATA (push, 0);

		src_offset += src->pitch * count;
		dst_offset += dst->pitch * count;
		h -= count;
	}
}

typedef unsigned (*get_offset_t)(struct nouveau_surface *s,
				 unsigned x, unsigned y);

static unsigned
get_linear_offset(struct nouveau_surface *s, unsigned x, unsigned y)
{
	return x * s->cpp + y * s->pitch;
}

static unsigned
get_swizzled_offset(struct nouveau_surface *s, unsigned x, unsigned y)
{
	unsigned k = log2i(MIN2(s->width, s->height));

	unsigned u = (x & 0x001) << 0 |
		(x & 0x002) << 1 |
		(x & 0x004) << 2 |
		(x & 0x008) << 3 |
		(x & 0x010) << 4 |
		(x & 0x020) << 5 |
		(x & 0x040) << 6 |
		(x & 0x080) << 7 |
		(x & 0x100) << 8 |
		(x & 0x200) << 9 |
		(x & 0x400) << 10 |
		(x & 0x800) << 11;

	unsigned v = (y & 0x001) << 1 |
		(y & 0x002) << 2 |
		(y & 0x004) << 3 |
		(y & 0x008) << 4 |
		(y & 0x010) << 5 |
		(y & 0x020) << 6 |
		(y & 0x040) << 7 |
		(y & 0x080) << 8 |
		(y & 0x100) << 9 |
		(y & 0x200) << 10 |
		(y & 0x400) << 11 |
		(y & 0x800) << 12;

	return s->cpp * (((u | v) & ~(~0 << 2*k)) |
			 (x & (~0 << k)) << k |
			 (y & (~0 << k)) << k);
}

static void
nv04_surface_copy_cpu(struct gl_context *ctx,
		      struct nouveau_surface *dst,
		      struct nouveau_surface *src,
		      int dx, int dy, int sx, int sy,
		      int w, int h)
{
	int x, y;
	get_offset_t get_dst = (dst->layout == SWIZZLED ?
				get_swizzled_offset : get_linear_offset);
	get_offset_t get_src = (src->layout == SWIZZLED ?
				get_swizzled_offset : get_linear_offset);
	void *dp, *sp;

	nouveau_bo_map(dst->bo, NOUVEAU_BO_WR, context_client(ctx));
	nouveau_bo_map(src->bo, NOUVEAU_BO_RD, context_client(ctx));

	dp = dst->bo->map + dst->offset;
	sp = src->bo->map + src->offset;

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			memcpy(dp + get_dst(dst, dx + x, dy + y),
			       sp + get_src(src, sx + x, sy + y), dst->cpp);
		}
	}
}

void
nv04_surface_copy(struct gl_context *ctx,
		  struct nouveau_surface *dst,
		  struct nouveau_surface *src,
		  int dx, int dy, int sx, int sy,
		  int w, int h)
{
	if (_mesa_is_format_compressed(src->format)) {
		sx = get_format_blocksx(src->format, sx);
		sy = get_format_blocksy(src->format, sy);
		dx = get_format_blocksx(dst->format, dx);
		dy = get_format_blocksy(dst->format, dy);
		w = get_format_blocksx(src->format, w);
		h = get_format_blocksy(src->format, h);
	}

	/* Linear texture copy. */
	if ((src->layout == LINEAR && dst->layout == LINEAR) ||
	    dst->width <= 2 || dst->height <= 1) {
		nv04_surface_copy_m2mf(ctx, dst, src, dx, dy, sx, sy, w, h);
		return;
	}

	/* Swizzle using sifm+swzsurf. */
        if (src->layout == LINEAR && dst->layout == SWIZZLED &&
	    dst->cpp != 1 && !(dst->offset & 63)) {
		nv04_surface_copy_swizzle(ctx, dst, src, dx, dy, sx, sy, w, h);
		return;
	}

	/* Fallback to CPU copy. */
	nv04_surface_copy_cpu(ctx, dst, src, dx, dy, sx, sy, w, h);
}

void
nv04_surface_fill(struct gl_context *ctx,
		  struct nouveau_surface *dst,
		  unsigned mask, unsigned value,
		  int dx, int dy, int w, int h)
{
	struct nouveau_pushbuf_refn refs[] = {
		{ dst->bo, NOUVEAU_BO_WR | NOUVEAU_BO_VRAM | NOUVEAU_BO_GART },
	};
	struct nouveau_pushbuf *push = context_push(ctx);
	struct nouveau_hw_state *hw = &to_nouveau_context(ctx)->hw;
	struct nv04_fifo *fifo = hw->chan->data;

	if (nouveau_pushbuf_space(push, 64, 4, 0) ||
	    nouveau_pushbuf_refn (push, refs, 1))
		return;

	BEGIN_NV04(push, NV04_SF2D(DMA_IMAGE_SOURCE), 2);
	PUSH_RELOC(push, dst->bo, 0, NOUVEAU_BO_OR, fifo->vram, fifo->gart);
	PUSH_RELOC(push, dst->bo, 0, NOUVEAU_BO_OR, fifo->vram, fifo->gart);
	BEGIN_NV04(push, NV04_SF2D(FORMAT), 4);
	PUSH_DATA (push, surf2d_format(dst->format));
	PUSH_DATA (push, (dst->pitch << 16) | dst->pitch);
	PUSH_RELOC(push, dst->bo, dst->offset, NOUVEAU_BO_LOW, 0, 0);
	PUSH_RELOC(push, dst->bo, dst->offset, NOUVEAU_BO_LOW, 0, 0);

	BEGIN_NV04(push, NV01_PATT(COLOR_FORMAT), 1);
	PUSH_DATA (push, rect_format(dst->format));
	BEGIN_NV04(push, NV01_PATT(MONOCHROME_COLOR1), 1);
	PUSH_DATA (push, mask | ~0ll << (8 * dst->cpp));

	BEGIN_NV04(push, NV04_GDI(COLOR_FORMAT), 1);
	PUSH_DATA (push, rect_format(dst->format));
	BEGIN_NV04(push, NV04_GDI(COLOR1_A), 1);
	PUSH_DATA (push, value);
	BEGIN_NV04(push, NV04_GDI(UNCLIPPED_RECTANGLE_POINT(0)), 2);
	PUSH_DATA (push, (dx << 16) | dy);
	PUSH_DATA (push, ( w << 16) |  h);
}

void
nv04_surface_takedown(struct gl_context *ctx)
{
	struct nouveau_hw_state *hw = &to_nouveau_context(ctx)->hw;

	nouveau_object_del(&hw->swzsurf);
	nouveau_object_del(&hw->sifm);
	nouveau_object_del(&hw->rect);
	nouveau_object_del(&hw->rop);
	nouveau_object_del(&hw->patt);
	nouveau_object_del(&hw->surf2d);
	nouveau_object_del(&hw->m2mf);
	nouveau_object_del(&hw->ntfy);
}

GLboolean
nv04_surface_init(struct gl_context *ctx)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	struct nouveau_hw_state *hw = &to_nouveau_context(ctx)->hw;
	struct nouveau_object *chan = hw->chan;
	unsigned handle = 0x88000000, class;
	int ret;

	/* Notifier object. */
	ret = nouveau_object_new(chan, handle++, NOUVEAU_NOTIFIER_CLASS,
				 &(struct nv04_notify) {
					.length = 32,
				 }, sizeof(struct nv04_notify), &hw->ntfy);
	if (ret)
		goto fail;

	/* Memory to memory format. */
	ret = nouveau_object_new(chan, handle++, NV03_M2MF_CLASS,
				 NULL, 0, &hw->m2mf);
	if (ret)
		goto fail;

	BEGIN_NV04(push, NV01_SUBC(M2MF, OBJECT), 1);
	PUSH_DATA (push, hw->m2mf->handle);
	BEGIN_NV04(push, NV03_M2MF(DMA_NOTIFY), 1);
	PUSH_DATA (push, hw->ntfy->handle);

	/* Context surfaces 2D. */
	if (context_chipset(ctx) < 0x10)
		class = NV04_SURFACE_2D_CLASS;
	else
		class = NV10_SURFACE_2D_CLASS;

	ret = nouveau_object_new(chan, handle++, class, NULL, 0, &hw->surf2d);
	if (ret)
		goto fail;

	BEGIN_NV04(push, NV01_SUBC(SF2D, OBJECT), 1);
	PUSH_DATA (push, hw->surf2d->handle);

	/* Raster op. */
	ret = nouveau_object_new(chan, handle++, NV03_ROP_CLASS,
				 NULL, 0, &hw->rop);
	if (ret)
		goto fail;

	BEGIN_NV04(push, NV01_SUBC(PATT, OBJECT), 1);
	PUSH_DATA (push, hw->rop->handle);
	BEGIN_NV04(push, NV01_ROP(DMA_NOTIFY), 1);
	PUSH_DATA (push, hw->ntfy->handle);

	BEGIN_NV04(push, NV01_ROP(ROP), 1);
	PUSH_DATA (push, 0xca); /* DPSDxax in the GDI speech. */

	/* Image pattern. */
	ret = nouveau_object_new(chan, handle++, NV04_PATTERN_CLASS,
				 NULL, 0, &hw->patt);
	if (ret)
		goto fail;

	BEGIN_NV04(push, NV01_SUBC(PATT, OBJECT), 1);
	PUSH_DATA (push, hw->patt->handle);
	BEGIN_NV04(push, NV01_PATT(DMA_NOTIFY), 1);
	PUSH_DATA (push, hw->ntfy->handle);

	BEGIN_NV04(push, NV01_PATT(MONOCHROME_FORMAT), 3);
	PUSH_DATA (push, NV04_IMAGE_PATTERN_MONOCHROME_FORMAT_LE);
	PUSH_DATA (push, NV04_IMAGE_PATTERN_MONOCHROME_SHAPE_8X8);
	PUSH_DATA (push, NV04_IMAGE_PATTERN_PATTERN_SELECT_MONO);

	BEGIN_NV04(push, NV01_PATT(MONOCHROME_COLOR0), 4);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, ~0);
	PUSH_DATA (push, ~0);

	/* GDI rectangle text. */
	ret = nouveau_object_new(chan, handle++, NV04_GDI_CLASS,
				 NULL, 0, &hw->rect);
	if (ret)
		goto fail;

	BEGIN_NV04(push, NV01_SUBC(GDI, OBJECT), 1);
	PUSH_DATA (push, hw->rect->handle);
	BEGIN_NV04(push, NV04_GDI(DMA_NOTIFY), 1);
	PUSH_DATA (push, hw->ntfy->handle);
	BEGIN_NV04(push, NV04_GDI(SURFACE), 1);
	PUSH_DATA (push, hw->surf2d->handle);
	BEGIN_NV04(push, NV04_GDI(ROP), 1);
	PUSH_DATA (push, hw->rop->handle);
	BEGIN_NV04(push, NV04_GDI(PATTERN), 1);
	PUSH_DATA (push, hw->patt->handle);

	BEGIN_NV04(push, NV04_GDI(OPERATION), 1);
	PUSH_DATA (push, NV04_GDI_RECTANGLE_TEXT_OPERATION_ROP_AND);
	BEGIN_NV04(push, NV04_GDI(MONOCHROME_FORMAT), 1);
	PUSH_DATA (push, NV04_GDI_RECTANGLE_TEXT_MONOCHROME_FORMAT_LE);

	/* Swizzled surface. */
	if (context_chipset(ctx) < 0x20)
		class = NV04_SURFACE_SWZ_CLASS;
	else
		class = NV20_SURFACE_SWZ_CLASS;

	ret = nouveau_object_new(chan, handle++, class, NULL, 0, &hw->swzsurf);
	if (ret)
		goto fail;

	BEGIN_NV04(push, NV01_SUBC(SURF, OBJECT), 1);
	PUSH_DATA (push, hw->swzsurf->handle);

	/* Scaled image from memory. */
	if  (context_chipset(ctx) < 0x10)
		class = NV04_SIFM_CLASS;
	else
		class = NV10_SIFM_CLASS;

	ret = nouveau_object_new(chan, handle++, class, NULL, 0, &hw->sifm);
	if (ret)
		goto fail;

	BEGIN_NV04(push, NV01_SUBC(SIFM, OBJECT), 1);
	PUSH_DATA (push, hw->sifm->handle);

	if (context_chipset(ctx) >= 0x10) {
		BEGIN_NV04(push, NV05_SIFM(COLOR_CONVERSION), 1);
		PUSH_DATA (push, NV05_SCALED_IMAGE_FROM_MEMORY_COLOR_CONVERSION_TRUNCATE);
	}

	return GL_TRUE;

fail:
	nv04_surface_takedown(ctx);
	return GL_FALSE;
}
