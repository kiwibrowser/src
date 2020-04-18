/*
 * Copyright 2007 Nouveau Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __NOUVEAU_LOCAL_H__
#define __NOUVEAU_LOCAL_H__

static inline uint32_t
PUSH_AVAIL(struct nouveau_pushbuf *push)
{
	return push->end - push->cur;
}

static inline int
PUSH_SPACE(struct nouveau_pushbuf *push, uint32_t size)
{
	if (PUSH_AVAIL(push) < size)
		return nouveau_pushbuf_space(push, size, 0, 0) == 0;
	return 1;
}

static inline void
PUSH_DATA(struct nouveau_pushbuf *push, uint32_t data)
{
	*push->cur++ = data;
}

static inline void
PUSH_DATAf(struct nouveau_pushbuf *push, float v)
{
	union { float f; uint32_t i; } d = { .f = v };
	PUSH_DATA(push, d.i);
}

static inline void
PUSH_DATAb(struct nouveau_pushbuf *push, GLboolean x)
{
	PUSH_DATA(push, x ? 1 : 0);
}

static inline void
PUSH_DATAm(struct nouveau_pushbuf *push, float m[16])
{
	int i, j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			PUSH_DATAf(push, m[4*j + i]);
}

static inline void
PUSH_DATAp(struct nouveau_pushbuf *push, const void *data, uint32_t size)
{
	memcpy(push->cur, data, size * 4);
	push->cur += size;
}

static inline void
PUSH_RELOC(struct nouveau_pushbuf *push, struct nouveau_bo *bo, uint32_t offset,
	   uint32_t flags, uint32_t vor, uint32_t tor)
{
	nouveau_pushbuf_reloc(push, bo, offset, flags, vor, tor);
}

static inline void
PUSH_KICK(struct nouveau_pushbuf *push)
{
	nouveau_pushbuf_kick(push, push->channel);
}

static struct nouveau_bufctx *
BUFCTX(struct nouveau_pushbuf *push)
{
	return push->user_priv;
}

static inline void
PUSH_RESET(struct nouveau_pushbuf *push, int bin)
{
	nouveau_bufctx_reset(BUFCTX(push), bin);
}

static inline void
PUSH_MTHDl(struct nouveau_pushbuf *push, int subc, int mthd, int bin,
	   struct nouveau_bo *bo, uint32_t offset, uint32_t access)
{
	nouveau_bufctx_mthd(BUFCTX(push), bin, (1 << 18) | (subc << 13) | mthd,
			    bo, offset, access | NOUVEAU_BO_LOW, 0, 0);
	PUSH_DATA(push, bo->offset + offset);
}

static inline void
PUSH_MTHDs(struct nouveau_pushbuf *push, int subc, int mthd, int bin,
	   struct nouveau_bo *bo, uint32_t data, uint32_t access,
	   uint32_t vor, uint32_t tor)
{
	nouveau_bufctx_mthd(BUFCTX(push), bin, (1 << 18) | (subc << 13) | mthd,
			    bo, data, access | NOUVEAU_BO_OR, vor, tor);

	if (bo->flags & NOUVEAU_BO_VRAM)
		PUSH_DATA(push, data | vor);
	else
		PUSH_DATA(push, data | tor);
}

static inline void
PUSH_MTHD(struct nouveau_pushbuf *push, int subc, int mthd, int bin,
	  struct nouveau_bo *bo, uint32_t data, uint32_t access,
	  uint32_t vor, uint32_t tor)
{
	nouveau_bufctx_mthd(BUFCTX(push), bin, (1 << 18) | (subc << 13) | mthd,
			    bo, data, access | NOUVEAU_BO_OR, vor, tor);

	if (access & NOUVEAU_BO_LOW)
		data += bo->offset;

	if (access & NOUVEAU_BO_OR) {
		if (bo->flags & NOUVEAU_BO_VRAM)
			data |= vor;
		else
			data |= tor;
	}

	PUSH_DATA(push, data);
}

static inline void
BEGIN_NV04(struct nouveau_pushbuf *push, int subc, int mthd, int size)
{
	PUSH_SPACE(push, size + 1);
	PUSH_DATA (push, 0x00000000 | (size << 18) | (subc << 13) | mthd);
}

static inline void
BEGIN_NI04(struct nouveau_pushbuf *push, int subc, int mthd, int size)
{
	PUSH_SPACE(push, size + 1);
	PUSH_DATA (push, 0x40000000 | (size << 18) | (subc << 13) | mthd);
}

/* subchannel assignment */
#define SUBC_M2MF(mthd)  0, (mthd)
#define NV03_M2MF(mthd)  SUBC_M2MF(NV04_M2MF_##mthd)
#define SUBC_NVSW(mthd)  1, (mthd)
#define SUBC_SF2D(mthd)  2, (mthd)
#define NV04_SF2D(mthd)  SUBC_SF2D(NV04_CONTEXT_SURFACES_2D_##mthd)
#define NV10_SF2D(mthd)  SUBC_SF2D(NV10_CONTEXT_SURFACES_2D_##mthd)
#define SUBC_PATT(mthd)  3, (mthd)
#define NV01_PATT(mthd)  SUBC_PATT(NV04_IMAGE_PATTERN_##mthd)
#define NV01_ROP(mthd)   SUBC_PATT(NV03_CONTEXT_ROP_##mthd)
#define SUBC_GDI(mthd)   4, (mthd)
#define NV04_GDI(mthd)   SUBC_GDI(NV04_GDI_RECTANGLE_TEXT_##mthd)
#define SUBC_SIFM(mthd)  5, (mthd)
#define NV03_SIFM(mthd)  SUBC_SIFM(NV03_SCALED_IMAGE_FROM_MEMORY_##mthd)
#define NV05_SIFM(mthd)  SUBC_SIFM(NV05_SCALED_IMAGE_FROM_MEMORY_##mthd)
#define SUBC_SURF(mthd)  6, (mthd)
#define NV04_SSWZ(mthd)  SUBC_SURF(NV04_SWIZZLED_SURFACE_##mthd)
#define NV04_SF3D(mthd)  SUBC_SURF(NV04_CONTEXT_SURFACES_3D_##mthd)
#define SUBC_3D(mthd)    7, (mthd)
#define NV04_TTRI(mthd)  SUBC_3D(NV04_TEXTURED_TRIANGLE_##mthd)
#define NV04_MTRI(mthd)  SUBC_3D(NV04_MULTITEX_TRIANGLE_##mthd)
#define NV10_3D(mthd)    SUBC_3D(NV10_3D_##mthd)
#define NV11_3D(mthd)    SUBC_3D(NV11_3D_##mthd)
#define NV17_3D(mthd)    SUBC_3D(NV17_3D_##mthd)
#define NV20_3D(mthd)    SUBC_3D(NV20_3D_##mthd)
#define NV25_3D(mthd)    SUBC_3D(NV25_3D_##mthd)

#define NV01_SUBC(subc, mthd) SUBC_##subc((NV01_SUBCHAN_##mthd))
#define NV11_SUBC(subc, mthd) SUBC_##subc((NV11_SUBCHAN_##mthd))

#define NV04_GRAPH(subc, mthd) SUBC_##subc((NV04_GRAPH_##mthd))

#endif
