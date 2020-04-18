/*
 * Copyright 2008 Ben Skeggs
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

#include <stdint.h>

#include "pipe/p_defines.h"

#include "util/u_inlines.h"
#include "util/u_pack_color.h"
#include "util/u_format.h"
#include "util/u_surface.h"

#include "nvc0_context.h"
#include "nvc0_resource.h"

#include "nv50/nv50_defs.xml.h"
#include "nv50/nv50_texture.xml.h"

#define NVC0_ENG2D_SUPPORTED_FORMATS 0xff9ccfe1cce3ccc9ULL

/* return TRUE for formats that can be converted among each other by NVC0_2D */
static INLINE boolean
nvc0_2d_format_faithful(enum pipe_format format)
{
   uint8_t id = nvc0_format_table[format].rt;

   return (id >= 0xc0) && (NVC0_ENG2D_SUPPORTED_FORMATS & (1ULL << (id - 0xc0)));
}

static INLINE uint8_t
nvc0_2d_format(enum pipe_format format)
{
   uint8_t id = nvc0_format_table[format].rt;

   /* Hardware values for color formats range from 0xc0 to 0xff,
    * but the 2D engine doesn't support all of them.
    */
   if (nvc0_2d_format_faithful(format))
      return id;

   switch (util_format_get_blocksize(format)) {
   case 1:
      return NV50_SURFACE_FORMAT_R8_UNORM;
   case 2:
      return NV50_SURFACE_FORMAT_R16_UNORM;
   case 4:
      return NV50_SURFACE_FORMAT_BGRA8_UNORM;
   case 8:
      return NV50_SURFACE_FORMAT_RGBA16_UNORM;
   case 16:
      return NV50_SURFACE_FORMAT_RGBA32_FLOAT;
   default:
      return 0;
   }
}

static int
nvc0_2d_texture_set(struct nouveau_pushbuf *push, boolean dst,
                    struct nv50_miptree *mt, unsigned level, unsigned layer)
{
   struct nouveau_bo *bo = mt->base.bo;
   uint32_t width, height, depth;
   uint32_t format;
   uint32_t mthd = dst ? NVC0_2D_DST_FORMAT : NVC0_2D_SRC_FORMAT;
   uint32_t offset = mt->level[level].offset;

   format = nvc0_2d_format(mt->base.base.format);
   if (!format) {
      NOUVEAU_ERR("invalid/unsupported surface format: %s\n",
                  util_format_name(mt->base.base.format));
      return 1;
   }

   width = u_minify(mt->base.base.width0, level) << mt->ms_x;
   height = u_minify(mt->base.base.height0, level) << mt->ms_y;
   depth = u_minify(mt->base.base.depth0, level);

   /* layer has to be < depth, and depth > tile depth / 2 */

   if (!mt->layout_3d) {
      offset += mt->layer_stride * layer;
      layer = 0;
      depth = 1;
   } else
   if (!dst) {
      offset += nvc0_mt_zslice_offset(mt, level, layer);
      layer = 0;
   }

   if (nouveau_bo_memtype(bo)) {
      BEGIN_NVC0(push, SUBC_2D(mthd), 2);
      PUSH_DATA (push, format);
      PUSH_DATA (push, 1);
      BEGIN_NVC0(push, SUBC_2D(mthd + 0x14), 5);
      PUSH_DATA (push, mt->level[level].pitch);
      PUSH_DATA (push, width);
      PUSH_DATA (push, height);
      PUSH_DATAh(push, bo->offset + offset);
      PUSH_DATA (push, bo->offset + offset);
   } else {
      BEGIN_NVC0(push, SUBC_2D(mthd), 5);
      PUSH_DATA (push, format);
      PUSH_DATA (push, 0);
      PUSH_DATA (push, mt->level[level].tile_mode);
      PUSH_DATA (push, depth);
      PUSH_DATA (push, layer);
      BEGIN_NVC0(push, SUBC_2D(mthd + 0x18), 4);
      PUSH_DATA (push, width);
      PUSH_DATA (push, height);
      PUSH_DATAh(push, bo->offset + offset);
      PUSH_DATA (push, bo->offset + offset);
   }

#if 0
   if (dst) {
      BEGIN_NVC0(push, SUBC_2D(NVC0_2D_CLIP_X), 4);
      PUSH_DATA (push, 0);
      PUSH_DATA (push, 0);
      PUSH_DATA (push, width);
      PUSH_DATA (push, height);
   }
#endif
   return 0;
}

static int
nvc0_2d_texture_do_copy(struct nouveau_pushbuf *push,
                        struct nv50_miptree *dst, unsigned dst_level,
                        unsigned dx, unsigned dy, unsigned dz,
                        struct nv50_miptree *src, unsigned src_level,
                        unsigned sx, unsigned sy, unsigned sz,
                        unsigned w, unsigned h)
{
   static const uint32_t duvdxy[5] =
   {
      0x40000000, 0x80000000, 0x00000001, 0x00000002, 0x00000004
   };

   int ret;
   uint32_t ctrl = 0x00;

   ret = PUSH_SPACE(push, 2 * 16 + 32);
   if (ret)
      return ret;

   ret = nvc0_2d_texture_set(push, TRUE, dst, dst_level, dz);
   if (ret)
      return ret;

   ret = nvc0_2d_texture_set(push, FALSE, src, src_level, sz);
   if (ret)
      return ret;

   /* NOTE: 2D engine doesn't work for MS8 */
   if (src->ms_x)
      ctrl = 0x11;

   /* 0/1 = CENTER/CORNER, 00/10 = POINT/BILINEAR */
   BEGIN_NVC0(push, NVC0_2D(BLIT_CONTROL), 1);
   PUSH_DATA (push, ctrl);
   BEGIN_NVC0(push, NVC0_2D(BLIT_DST_X), 4);
   PUSH_DATA (push, dx << dst->ms_x);
   PUSH_DATA (push, dy << dst->ms_y);
   PUSH_DATA (push, w << dst->ms_x);
   PUSH_DATA (push, h << dst->ms_y);
   BEGIN_NVC0(push, NVC0_2D(BLIT_DU_DX_FRACT), 4);
   PUSH_DATA (push, duvdxy[2 + ((int)src->ms_x - (int)dst->ms_x)] & 0xf0000000);
   PUSH_DATA (push, duvdxy[2 + ((int)src->ms_x - (int)dst->ms_x)] & 0x0000000f);
   PUSH_DATA (push, duvdxy[2 + ((int)src->ms_y - (int)dst->ms_y)] & 0xf0000000);
   PUSH_DATA (push, duvdxy[2 + ((int)src->ms_y - (int)dst->ms_y)] & 0x0000000f);
   BEGIN_NVC0(push, NVC0_2D(BLIT_SRC_X_FRACT), 4);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, sx << src->ms_x);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, sy << src->ms_x);

   return 0;
}

static void
nvc0_resource_copy_region(struct pipe_context *pipe,
                          struct pipe_resource *dst, unsigned dst_level,
                          unsigned dstx, unsigned dsty, unsigned dstz,
                          struct pipe_resource *src, unsigned src_level,
                          const struct pipe_box *src_box)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   int ret;
   boolean m2mf;
   unsigned dst_layer = dstz, src_layer = src_box->z;

   /* Fallback for buffers. */
   if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
      util_resource_copy_region(pipe, dst, dst_level, dstx, dsty, dstz,
                                src, src_level, src_box);
      return;
   }

   /* 0 and 1 are equal, only supporting 0/1, 2, 4 and 8 */
   assert((src->nr_samples | 1) == (dst->nr_samples | 1));

   m2mf = (src->format == dst->format) ||
      (util_format_get_blocksizebits(src->format) ==
       util_format_get_blocksizebits(dst->format));

   nv04_resource(dst)->status |= NOUVEAU_BUFFER_STATUS_GPU_WRITING;

   if (m2mf) {
      struct nv50_m2mf_rect drect, srect;
      unsigned i;
      unsigned nx = util_format_get_nblocksx(src->format, src_box->width);
      unsigned ny = util_format_get_nblocksy(src->format, src_box->height);

      nv50_m2mf_rect_setup(&drect, dst, dst_level, dstx, dsty, dstz);
      nv50_m2mf_rect_setup(&srect, src, src_level,
                           src_box->x, src_box->y, src_box->z);

      for (i = 0; i < src_box->depth; ++i) {
         nvc0->m2mf_copy_rect(nvc0, &drect, &srect, nx, ny);

         if (nv50_miptree(dst)->layout_3d)
            drect.z++;
         else
            drect.base += nv50_miptree(dst)->layer_stride;

         if (nv50_miptree(src)->layout_3d)
            srect.z++;
         else
            srect.base += nv50_miptree(src)->layer_stride;
      }
      return;
   }

   assert(nvc0_2d_format_faithful(src->format));
   assert(nvc0_2d_format_faithful(dst->format));

   BCTX_REFN(nvc0->bufctx, 2D, nv04_resource(src), RD);
   BCTX_REFN(nvc0->bufctx, 2D, nv04_resource(dst), WR);
   nouveau_pushbuf_bufctx(nvc0->base.pushbuf, nvc0->bufctx);
   nouveau_pushbuf_validate(nvc0->base.pushbuf);

   for (; dst_layer < dstz + src_box->depth; ++dst_layer, ++src_layer) {
      ret = nvc0_2d_texture_do_copy(nvc0->base.pushbuf,
                                    nv50_miptree(dst), dst_level,
                                    dstx, dsty, dst_layer,
                                    nv50_miptree(src), src_level,
                                    src_box->x, src_box->y, src_layer,
                                    src_box->width, src_box->height);
      if (ret)
         break;
   }
   nouveau_bufctx_reset(nvc0->bufctx, 0);
}

static void
nvc0_clear_render_target(struct pipe_context *pipe,
                         struct pipe_surface *dst,
                         const union pipe_color_union *color,
                         unsigned dstx, unsigned dsty,
                         unsigned width, unsigned height)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nv50_surface *sf = nv50_surface(dst);
   struct nv04_resource *res = nv04_resource(sf->base.texture);
   unsigned z;

   BEGIN_NVC0(push, NVC0_3D(CLEAR_COLOR(0)), 4);
   PUSH_DATAf(push, color->f[0]);
   PUSH_DATAf(push, color->f[1]);
   PUSH_DATAf(push, color->f[2]);
   PUSH_DATAf(push, color->f[3]);

   BEGIN_NVC0(push, NVC0_3D(SCREEN_SCISSOR_HORIZ), 2);
   PUSH_DATA (push, ( width << 16) | dstx);
   PUSH_DATA (push, (height << 16) | dsty);

   BEGIN_NVC0(push, NVC0_3D(RT_CONTROL), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_3D(RT_ADDRESS_HIGH(0)), 9);
   PUSH_DATAh(push, res->address + sf->offset);
   PUSH_DATA (push, res->address + sf->offset);
   if (likely(nouveau_bo_memtype(res->bo))) {
      struct nv50_miptree *mt = nv50_miptree(dst->texture);

      PUSH_DATA(push, sf->width);
      PUSH_DATA(push, sf->height);
      PUSH_DATA(push, nvc0_format_table[dst->format].rt);
      PUSH_DATA(push, (mt->layout_3d << 16) |
               mt->level[sf->base.u.tex.level].tile_mode);
      PUSH_DATA(push, dst->u.tex.first_layer + sf->depth);
      PUSH_DATA(push, mt->layer_stride >> 2);
      PUSH_DATA(push, dst->u.tex.first_layer);
   } else {
      if (res->base.target == PIPE_BUFFER) {
         PUSH_DATA(push, 262144);
         PUSH_DATA(push, 1);
      } else {
         PUSH_DATA(push, nv50_miptree(&res->base)->level[0].pitch);
         PUSH_DATA(push, sf->height);
      }
      PUSH_DATA(push, nvc0_format_table[sf->base.format].rt);
      PUSH_DATA(push, 1 << 12);
      PUSH_DATA(push, 1);
      PUSH_DATA(push, 0);
      PUSH_DATA(push, 0);

      IMMED_NVC0(push, NVC0_3D(ZETA_ENABLE), 0);

      /* tiled textures don't have to be fenced, they're not mapped directly */
      nvc0_resource_fence(res, NOUVEAU_BO_WR);
   }

   for (z = 0; z < sf->depth; ++z) {
      BEGIN_NVC0(push, NVC0_3D(CLEAR_BUFFERS), 1);
      PUSH_DATA (push, 0x3c |
                 (z << NVC0_3D_CLEAR_BUFFERS_LAYER__SHIFT));
   }

   nvc0->dirty |= NVC0_NEW_FRAMEBUFFER;
}

static void
nvc0_clear_depth_stencil(struct pipe_context *pipe,
                         struct pipe_surface *dst,
                         unsigned clear_flags,
                         double depth,
                         unsigned stencil,
                         unsigned dstx, unsigned dsty,
                         unsigned width, unsigned height)
{
	struct nvc0_context *nvc0 = nvc0_context(pipe);
	struct nouveau_pushbuf *push = nvc0->base.pushbuf;
	struct nv50_miptree *mt = nv50_miptree(dst->texture);
	struct nv50_surface *sf = nv50_surface(dst);
	uint32_t mode = 0;
	int unk = mt->base.base.target == PIPE_TEXTURE_2D;
	unsigned z;

	if (clear_flags & PIPE_CLEAR_DEPTH) {
		BEGIN_NVC0(push, NVC0_3D(CLEAR_DEPTH), 1);
		PUSH_DATAf(push, depth);
		mode |= NVC0_3D_CLEAR_BUFFERS_Z;
	}

	if (clear_flags & PIPE_CLEAR_STENCIL) {
		BEGIN_NVC0(push, NVC0_3D(CLEAR_STENCIL), 1);
		PUSH_DATA (push, stencil & 0xff);
		mode |= NVC0_3D_CLEAR_BUFFERS_S;
	}

	BEGIN_NVC0(push, NVC0_3D(SCREEN_SCISSOR_HORIZ), 2);
	PUSH_DATA (push, ( width << 16) | dstx);
	PUSH_DATA (push, (height << 16) | dsty);

	BEGIN_NVC0(push, NVC0_3D(ZETA_ADDRESS_HIGH), 5);
	PUSH_DATAh(push, mt->base.address + sf->offset);
	PUSH_DATA (push, mt->base.address + sf->offset);
	PUSH_DATA (push, nvc0_format_table[dst->format].rt);
	PUSH_DATA (push, mt->level[sf->base.u.tex.level].tile_mode);
	PUSH_DATA (push, mt->layer_stride >> 2);
	BEGIN_NVC0(push, NVC0_3D(ZETA_ENABLE), 1);
	PUSH_DATA (push, 1);
	BEGIN_NVC0(push, NVC0_3D(ZETA_HORIZ), 3);
	PUSH_DATA (push, sf->width);
	PUSH_DATA (push, sf->height);
	PUSH_DATA (push, (unk << 16) | (dst->u.tex.first_layer + sf->depth));
	BEGIN_NVC0(push, NVC0_3D(ZETA_BASE_LAYER), 1);
	PUSH_DATA (push, dst->u.tex.first_layer);

	for (z = 0; z < sf->depth; ++z) {
		BEGIN_NVC0(push, NVC0_3D(CLEAR_BUFFERS), 1);
		PUSH_DATA (push, mode |
			   (z << NVC0_3D_CLEAR_BUFFERS_LAYER__SHIFT));
	}

	nvc0->dirty |= NVC0_NEW_FRAMEBUFFER;
}

void
nvc0_clear(struct pipe_context *pipe, unsigned buffers,
           const union pipe_color_union *color,
           double depth, unsigned stencil)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct pipe_framebuffer_state *fb = &nvc0->framebuffer;
   unsigned i;
   uint32_t mode = 0;

   /* don't need NEW_BLEND, COLOR_MASK doesn't affect CLEAR_BUFFERS */
   if (!nvc0_state_validate(nvc0, NVC0_NEW_FRAMEBUFFER, 9 + (fb->nr_cbufs * 2)))
      return;

   if (buffers & PIPE_CLEAR_COLOR && fb->nr_cbufs) {
      BEGIN_NVC0(push, NVC0_3D(CLEAR_COLOR(0)), 4);
      PUSH_DATAf(push, color->f[0]);
      PUSH_DATAf(push, color->f[1]);
      PUSH_DATAf(push, color->f[2]);
      PUSH_DATAf(push, color->f[3]);
      mode =
         NVC0_3D_CLEAR_BUFFERS_R | NVC0_3D_CLEAR_BUFFERS_G |
         NVC0_3D_CLEAR_BUFFERS_B | NVC0_3D_CLEAR_BUFFERS_A;
   }

   if (buffers & PIPE_CLEAR_DEPTH) {
      BEGIN_NVC0(push, NVC0_3D(CLEAR_DEPTH), 1);
      PUSH_DATA (push, fui(depth));
      mode |= NVC0_3D_CLEAR_BUFFERS_Z;
   }

   if (buffers & PIPE_CLEAR_STENCIL) {
      BEGIN_NVC0(push, NVC0_3D(CLEAR_STENCIL), 1);
      PUSH_DATA (push, stencil & 0xff);
      mode |= NVC0_3D_CLEAR_BUFFERS_S;
   }

   BEGIN_NVC0(push, NVC0_3D(CLEAR_BUFFERS), 1);
   PUSH_DATA (push, mode);

   for (i = 1; i < fb->nr_cbufs; i++) {
      BEGIN_NVC0(push, NVC0_3D(CLEAR_BUFFERS), 1);
      PUSH_DATA (push, (i << 6) | 0x3c);
   }
}


struct nvc0_blitctx
{
   struct nvc0_screen *screen;
   struct {
      struct pipe_framebuffer_state fb;
      struct nvc0_program *vp;
      struct nvc0_program *tcp;
      struct nvc0_program *tep;
      struct nvc0_program *gp;
      struct nvc0_program *fp;
      unsigned num_textures[5];
      unsigned num_samplers[5];
      struct pipe_sampler_view *texture[2];
      struct nv50_tsc_entry *sampler[2];
      unsigned dirty;
   } saved;
   struct nvc0_program vp;
   struct nvc0_program fp;
   struct nv50_tsc_entry sampler[2]; /* nearest, bilinear */
   uint32_t fp_offset;
   uint16_t color_mask;
   uint8_t filter;
};

static void
nvc0_blitctx_make_vp(struct nvc0_blitctx *blit)
{
   static const uint32_t code[] =
   {
      0xfff01c66, 0x06000080, /* vfetch b128 { $r0 $r1 $r2 $r3 } a[0x80] */
      0xfff11c26, 0x06000090, /* vfetch b64 { $r4 $r5 } a[0x90]*/
      0x03f01c66, 0x0a7e0070, /* export b128 o[0x70] { $r0 $r1 $r2 $r3 } */
      0x13f01c26, 0x0a7e0080, /* export b64 o[0x80] { $r4 $r5 } */
      0x00001de7, 0x80000000, /* exit */
   };

   blit->vp.type = PIPE_SHADER_VERTEX;
   blit->vp.translated = TRUE;
   blit->vp.code = (uint32_t *)code; /* no relocations -> no modification */
   blit->vp.code_size = sizeof(code);
   blit->vp.max_gpr = 6;
   blit->vp.vp.edgeflag = PIPE_MAX_ATTRIBS;

   blit->vp.hdr[0]  = 0x00020461; /* vertprog magic */
   blit->vp.hdr[4]  = 0x000ff000; /* no outputs read */
   blit->vp.hdr[6]  = 0x0000003f; /* a[0x80], a[0x90] */
   blit->vp.hdr[13] = 0x0003f000; /* o[0x70], o[0x80] */
}

static void
nvc0_blitctx_make_fp(struct nvc0_blitctx *blit)
{
   static const uint32_t code_nvc0[] = /* use nvc0dis */
   {
      /* 2 coords RGBA in, RGBA out, also for Z32_FLOAT(_S8X24_UINT)
       * NOTE:
       * NVC0 doesn't like tex 3d on non-3d textures, but there should
       * only be 2d and 2d-array MS resources anyway.
       */
      0xfff01c00, 0xc07e0080,
      0xfff05c00, 0xc07e0084,
      0x00001e86, 0x8013c000,
      0x00001de7, 0x80000000,
      /* size: 0x70 + padding  */
      0, 0, 0, 0,

      /* 2 coords ZS in, S encoded in R, Z encoded in GBA (8_UNORM)
       * Setup float outputs in a way that conversion to UNORM yields the
       * desired byte value.
       */
      /* NOTE: need to repeat header */
      0x00021462, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
      0x80000000, 0x0000000f, 0x00000000, 0x00000000, 0x00000000,
      0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
      0x00000000, 0x00000000, 0x00000000, 0x0000000f, 0x00000000,
      0xfff09c00, 0xc07e0080,
      0xfff0dc00, 0xc07e0084,
      0x00201e86, 0x80104000,
      0x00205f06, 0x80104101,
      0xfc009c02, 0x312dffff,
      0x05001c88,
      0x09009e88,
      0x04001c02, 0x30ee0202,
      0xfc205c02, 0x38000003,
      0x0020dc02, 0x3803fc00,
      0x00209c02, 0x380003fc,
      0x05005c88,
      0x0d00dc88,
      0x09209e04, 0x18000000,
      0x04105c02, 0x30ee0202,
      0x0430dc02, 0x30ce0202,
      0x04209c02, 0x30de0202,
      0x00001de7, 0x80000000,
      /* size: 0xd0 + padding */
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

      /* 2 coords ZS in, Z encoded in RGB, S encoded in A (U8_UNORM) */
      0x00021462, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
      0x80000000, 0x0000000f, 0x00000000, 0x00000000, 0x00000000,
      0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
      0x00000000, 0x00000000, 0x00000000, 0x0000000f, 0x00000000,
      0xfff09c00, 0xc07e0080,
      0xfff0dc00, 0xc07e0084,
      0x00201e86, 0x80104000,
      0x00205f06, 0x80104101,
      0xfc009c02, 0x312dffff,
      0x0500dc88,
      0x09009e88,
      0x0430dc02, 0x30ee0202,
      0xfc201c02, 0x38000003,
      0x00205c02, 0x380003fc,
      0x00209c02, 0x3803fc00,
      0x01001c88,
      0x05005c88,
      0x09209e04, 0x18000000,
      0x04001c02, 0x30ee0202,
      0x04105c02, 0x30de0202,
      0x04209c02, 0x30ce0202,
      0x00001de7, 0x80000000,
   };
   static const uint32_t code_nve4[] = /* use nvc0dis */
   {
      /* 2 coords RGBA in, RGBA out, also for Z32_FLOAT(_S8X24_UINT)
       * NOTE:
       * NVC0 doesn't like tex 3d on non-3d textures, but there should
       * only be 2d and 2d-array MS resources anyway.
       */
      0x2202e237, 0x200002ec,
      0xfff01c00, 0xc07e0080,
      0xfff05c00, 0xc07e0084,
      0x00001e86, 0x8013c000,
      0x00001de6, 0xf0000000,
      0x00001de7, 0x80000000,
      /* size: 0x80 */

      /* 2 coords ZS in, S encoded in R, Z encoded in GBA (8_UNORM)
       * Setup float outputs in a way that conversion to UNORM yields the
       * desired byte value.
       */
      /* NOTE: need to repeat header */
      0x00021462, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
      0x80000000, 0x0000000f, 0x00000000, 0x00000000, 0x00000000,
      0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
      0x00000000, 0x00000000, 0x00000000, 0x0000000f, 0x00000000,
      0x0202e237, 0x22804c22,
      0xfff09c00, 0xc07e0080,
      0xfff0dc00, 0xc07e0084,
      0x00201e86, 0x80104008,
      0x00205f06, 0x80104009,
      0x00001de6, 0xf0000000,
      0xfc009c02, 0x312dffff,
      0x05201e04, 0x18000000,
      0x00428047, 0x22020272,
      0x09209c84, 0x14000000,
      0x04001c02, 0x30ee0202,
      0xfc205c02, 0x38000003,
      0x0020dc02, 0x3803fc00,
      0x00209c02, 0x380003fc,
      0x05205e04, 0x18000000,
      0x0d20de04, 0x18000000,
      0x42004277, 0x200002e0,
      0x09209e04, 0x18000000,
      0x04105c02, 0x30ee0202,
      0x0430dc02, 0x30ce0202,
      0x04209c02, 0x30de0202,
      0x00001de7, 0x80000000,
      /* size: 0x100 */

      /* 2 coords ZS in, Z encoded in RGB, S encoded in A (U8_UNORM) */
      0x00021462, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
      0x80000000, 0x0000000f, 0x00000000, 0x00000000, 0x00000000,
      0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
      0x00000000, 0x00000000, 0x00000000, 0x0000000f, 0x00000000,
      0x0202e237, 0x22804c22,
      0xfff09c00, 0xc07e0080,
      0xfff0dc00, 0xc07e0084,
      0x00201e86, 0x80104008,
      0x00205f06, 0x80104009,
      0x00001de6, 0xf0000000,
      0xfc009c02, 0x312dffff,
      0x0520de04, 0x18000000,
      0x00428047, 0x22020272,
      0x09209c84, 0x14000000,
      0x0430dc02, 0x30ee0202,
      0xfc201c02, 0x38000003,
      0x00205c02, 0x380003fc,
      0x00209c02, 0x3803fc00,
      0x01201e04, 0x18000000,
      0x05205e04, 0x18000000,
      0x42004277, 0x200002e0,
      0x09209e04, 0x18000000,
      0x04001c02, 0x30ee0202,
      0x04105c02, 0x30de0202,
      0x04209c02, 0x30ce0202,
      0x00001de7, 0x80000000,
   };

   blit->fp.type = PIPE_SHADER_FRAGMENT;
   blit->fp.translated = TRUE;
   if (blit->screen->base.class_3d >= NVE4_3D_CLASS) {
      blit->fp.code = (uint32_t *)code_nve4; /* const_cast */
      blit->fp.code_size = sizeof(code_nve4);
   } else {
      blit->fp.code = (uint32_t *)code_nvc0; /* const_cast */
      blit->fp.code_size = sizeof(code_nvc0);
   }
   blit->fp.max_gpr = 4;

   blit->fp.hdr[0]  = 0x00021462; /* fragprog magic */
   blit->fp.hdr[5]  = 0x80000000;
   blit->fp.hdr[6]  = 0x0000000f; /* 2 linear */
   blit->fp.hdr[18] = 0x0000000f; /* 1 colour output */
}

static void
nvc0_blitctx_make_sampler(struct nvc0_blitctx *blit)
{
   /* clamp to edge, min/max lod = 0, nearest filtering */

   blit->sampler[0].id = -1;

   blit->sampler[0].tsc[0] = NV50_TSC_0_SRGB_CONVERSION_ALLOWED |
      (NV50_TSC_WRAP_CLAMP_TO_EDGE << NV50_TSC_0_WRAPS__SHIFT) |
      (NV50_TSC_WRAP_CLAMP_TO_EDGE << NV50_TSC_0_WRAPT__SHIFT) |
      (NV50_TSC_WRAP_CLAMP_TO_EDGE << NV50_TSC_0_WRAPR__SHIFT);
   blit->sampler[0].tsc[1] =
      NV50_TSC_1_MAGF_NEAREST | NV50_TSC_1_MINF_NEAREST | NV50_TSC_1_MIPF_NONE;

   /* clamp to edge, min/max lod = 0, bilinear filtering */

   blit->sampler[1].id = -1;

   blit->sampler[1].tsc[0] = blit->sampler[0].tsc[0];
   blit->sampler[1].tsc[1] =
      NV50_TSC_1_MAGF_LINEAR | NV50_TSC_1_MINF_LINEAR | NV50_TSC_1_MIPF_NONE;
}

/* Since shaders cannot export stencil, we cannot copy stencil values when
 * rendering to ZETA, so we attach the ZS surface to a colour render target.
 */
static INLINE enum pipe_format
nvc0_blit_zeta_to_colour_format(enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_Z16_UNORM:               return PIPE_FORMAT_R16_UNORM;
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:             return PIPE_FORMAT_R8G8B8A8_UNORM;
   case PIPE_FORMAT_Z32_FLOAT:               return PIPE_FORMAT_R32_FLOAT;
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:    return PIPE_FORMAT_R32G32_FLOAT;
   default:
      assert(0);
      return PIPE_FORMAT_NONE;
   }
}

static void
nvc0_blitctx_get_color_mask_and_fp(struct nvc0_blitctx *blit,
                                   enum pipe_format format, uint8_t mask)
{
   blit->color_mask = 0;

   switch (format) {
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      blit->fp_offset = 0x180;
      if (mask & PIPE_MASK_Z)
         blit->color_mask |= 0x0111;
      if (mask & PIPE_MASK_S)
         blit->color_mask |= 0x1000;
      break;
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      blit->fp_offset = 0x80;
      if (mask & PIPE_MASK_Z)
         blit->color_mask |= 0x1110;
      if (mask & PIPE_MASK_S)
         blit->color_mask |= 0x0001;
      break;
   default:
      blit->fp_offset = 0;
      if (mask & (PIPE_MASK_R | PIPE_MASK_Z)) blit->color_mask |= 0x0001;
      if (mask & (PIPE_MASK_G | PIPE_MASK_S)) blit->color_mask |= 0x0010;
      if (mask & PIPE_MASK_B) blit->color_mask |= 0x0100;
      if (mask & PIPE_MASK_A) blit->color_mask |= 0x1000;
      break;
   }
}

static void
nvc0_blit_set_dst(struct nvc0_context *nvc0,
                  struct pipe_resource *res, unsigned level, unsigned layer)
{
   struct pipe_context *pipe = &nvc0->base.pipe;
   struct pipe_surface templ;

   if (util_format_is_depth_or_stencil(res->format))
      templ.format = nvc0_blit_zeta_to_colour_format(res->format);
   else
      templ.format = res->format;

   templ.usage = PIPE_USAGE_STREAM;
   templ.u.tex.level = level;
   templ.u.tex.first_layer = templ.u.tex.last_layer = layer;

   nvc0->framebuffer.cbufs[0] = nvc0_miptree_surface_new(pipe, res, &templ);
   nvc0->framebuffer.nr_cbufs = 1;
   nvc0->framebuffer.zsbuf = NULL;
   nvc0->framebuffer.width = nvc0->framebuffer.cbufs[0]->width;
   nvc0->framebuffer.height = nvc0->framebuffer.cbufs[0]->height;
}

static INLINE void
nvc0_blit_fixup_tic_entry(struct pipe_sampler_view *view, const boolean filter)
{
   struct nv50_tic_entry *ent = nv50_tic_entry(view);

   ent->tic[2] &= ~(1 << 31); /* scaled coordinates, ok with 3d textures ? */

   /* magic: */

   if (filter) {
      /* affects quality of near vertical edges in MS8: */
      ent->tic[3] = 0x20000000;
   } else {
      ent->tic[3] = 0;
      ent->tic[6] = 0;
   }
}

static void
nvc0_blit_set_src(struct nvc0_context *nvc0,
                  struct pipe_resource *res, unsigned level, unsigned layer,
                  const boolean filter)
{
   struct pipe_context *pipe = &nvc0->base.pipe;
   struct pipe_sampler_view templ;
   int s;

   templ.format = res->format;
   templ.u.tex.first_layer = templ.u.tex.last_layer = layer;
   templ.u.tex.first_level = templ.u.tex.last_level = level;
   templ.swizzle_r = PIPE_SWIZZLE_RED;
   templ.swizzle_g = PIPE_SWIZZLE_GREEN;
   templ.swizzle_b = PIPE_SWIZZLE_BLUE;
   templ.swizzle_a = PIPE_SWIZZLE_ALPHA;

   nvc0->textures[4][0] = nvc0_create_sampler_view(pipe, res, &templ);
   nvc0->textures[4][1] = NULL;

   nvc0_blit_fixup_tic_entry(nvc0->textures[4][0], filter);

   for (s = 0; s <= 3; ++s)
      nvc0->num_textures[s] = 0;
   nvc0->num_textures[4] = 1;

   templ.format = nv50_zs_to_s_format(res->format);
   if (templ.format != res->format) {
      nvc0->textures[4][1] = nvc0_create_sampler_view(pipe, res, &templ);
      nvc0_blit_fixup_tic_entry(nvc0->textures[4][1], filter);
      nvc0->num_textures[4] = 2;
   }
}

static void
nvc0_blitctx_prepare_state(struct nvc0_blitctx *blit)
{
   struct nouveau_pushbuf *push = blit->screen->base.pushbuf;

   /* TODO: maybe make this a MACRO (if we need more logic) ? */

   /* blend state */
   BEGIN_NVC0(push, NVC0_3D(COLOR_MASK(0)), 1);
   PUSH_DATA (push, blit->color_mask);
   BEGIN_NVC0(push, NVC0_3D(BLEND_ENABLE(0)), 1);
   PUSH_DATA (push, 0);
   IMMED_NVC0(push, NVC0_3D(LOGIC_OP_ENABLE), 0);

   /* rasterizer state */
   BEGIN_NVC0(push, NVC0_3D(FRAG_COLOR_CLAMP_EN), 1);
   PUSH_DATA (push, 0);
   IMMED_NVC0(push, NVC0_3D(MULTISAMPLE_ENABLE), 0);
   BEGIN_NVC0(push, NVC0_3D(MSAA_MASK(0)), 4);
   PUSH_DATA (push, 0xffff);
   PUSH_DATA (push, 0xffff);
   PUSH_DATA (push, 0xffff);
   PUSH_DATA (push, 0xffff);
   BEGIN_NVC0(push, NVC0_3D(MACRO_POLYGON_MODE_FRONT), 1);
   PUSH_DATA (push, NVC0_3D_MACRO_POLYGON_MODE_FRONT_FILL);
   BEGIN_NVC0(push, NVC0_3D(MACRO_POLYGON_MODE_BACK), 1);
   PUSH_DATA (push, NVC0_3D_MACRO_POLYGON_MODE_BACK_FILL);
   IMMED_NVC0(push, NVC0_3D(POLYGON_SMOOTH_ENABLE), 0);
   IMMED_NVC0(push, NVC0_3D(POLYGON_OFFSET_FILL_ENABLE), 0);
   IMMED_NVC0(push, NVC0_3D(POLYGON_STIPPLE_ENABLE), 0);
   IMMED_NVC0(push, NVC0_3D(CULL_FACE_ENABLE), 0);

   /* zsa state */
   IMMED_NVC0(push, NVC0_3D(DEPTH_TEST_ENABLE), 0);
   IMMED_NVC0(push, NVC0_3D(STENCIL_ENABLE), 0);
   IMMED_NVC0(push, NVC0_3D(ALPHA_TEST_ENABLE), 0);

   /* disable transform feedback */
   IMMED_NVC0(push, NVC0_3D(TFB_ENABLE), 0);
}

static void
nvc0_blitctx_pre_blit(struct nvc0_blitctx *blit, struct nvc0_context *nvc0)
{
   int s;

   blit->saved.fb.width = nvc0->framebuffer.width;
   blit->saved.fb.height = nvc0->framebuffer.height;
   blit->saved.fb.nr_cbufs = nvc0->framebuffer.nr_cbufs;
   blit->saved.fb.cbufs[0] = nvc0->framebuffer.cbufs[0];
   blit->saved.fb.zsbuf = nvc0->framebuffer.zsbuf;

   blit->saved.vp = nvc0->vertprog;
   blit->saved.tcp = nvc0->tctlprog;
   blit->saved.tep = nvc0->tevlprog;
   blit->saved.gp = nvc0->gmtyprog;
   blit->saved.fp = nvc0->fragprog;

   nvc0->vertprog = &blit->vp;
   nvc0->fragprog = &blit->fp;
   nvc0->tctlprog = NULL;
   nvc0->tevlprog = NULL;
   nvc0->gmtyprog = NULL;

   for (s = 0; s <= 4; ++s) {
      blit->saved.num_textures[s] = nvc0->num_textures[s];
      blit->saved.num_samplers[s] = nvc0->num_samplers[s];
      nvc0->textures_dirty[s] = (1 << nvc0->num_textures[s]) - 1;
      nvc0->samplers_dirty[s] = (1 << nvc0->num_samplers[s]) - 1;
   }
   blit->saved.texture[0] = nvc0->textures[4][0];
   blit->saved.texture[1] = nvc0->textures[4][1];
   blit->saved.sampler[0] = nvc0->samplers[4][0];
   blit->saved.sampler[1] = nvc0->samplers[4][1];

   nvc0->samplers[4][0] = &blit->sampler[blit->filter];
   nvc0->samplers[4][1] = &blit->sampler[blit->filter];

   for (s = 0; s <= 3; ++s)
      nvc0->num_samplers[s] = 0;
   nvc0->num_samplers[4] = 2;

   blit->saved.dirty = nvc0->dirty;

   nvc0->textures_dirty[4] |= 3;
   nvc0->samplers_dirty[4] |= 3;

   nvc0->dirty = NVC0_NEW_FRAMEBUFFER |
      NVC0_NEW_VERTPROG | NVC0_NEW_FRAGPROG |
      NVC0_NEW_TCTLPROG | NVC0_NEW_TEVLPROG | NVC0_NEW_GMTYPROG |
      NVC0_NEW_TEXTURES | NVC0_NEW_SAMPLERS;
}

static void
nvc0_blitctx_post_blit(struct nvc0_context *nvc0, struct nvc0_blitctx *blit)
{
   int s;

   pipe_surface_reference(&nvc0->framebuffer.cbufs[0], NULL);

   nvc0->framebuffer.width = blit->saved.fb.width;
   nvc0->framebuffer.height = blit->saved.fb.height;
   nvc0->framebuffer.nr_cbufs = blit->saved.fb.nr_cbufs;
   nvc0->framebuffer.cbufs[0] = blit->saved.fb.cbufs[0];
   nvc0->framebuffer.zsbuf = blit->saved.fb.zsbuf;

   nvc0->vertprog = blit->saved.vp;
   nvc0->tctlprog = blit->saved.tcp;
   nvc0->tevlprog = blit->saved.tep;
   nvc0->gmtyprog = blit->saved.gp;
   nvc0->fragprog = blit->saved.fp;

   pipe_sampler_view_reference(&nvc0->textures[4][0], NULL);
   pipe_sampler_view_reference(&nvc0->textures[4][1], NULL);

   for (s = 0; s <= 4; ++s) {
      nvc0->num_textures[s] = blit->saved.num_textures[s];
      nvc0->num_samplers[s] = blit->saved.num_samplers[s];
      nvc0->textures_dirty[s] = (1 << nvc0->num_textures[s]) - 1;
      nvc0->samplers_dirty[s] = (1 << nvc0->num_samplers[s]) - 1;
   }
   nvc0->textures[4][0] = blit->saved.texture[0];
   nvc0->textures[4][1] = blit->saved.texture[1];
   nvc0->samplers[4][0] = blit->saved.sampler[0];
   nvc0->samplers[4][1] = blit->saved.sampler[1];

   nvc0->textures_dirty[4] |= 3;
   nvc0->samplers_dirty[4] |= 3;

   nvc0->dirty = blit->saved.dirty |
      (NVC0_NEW_FRAMEBUFFER | NVC0_NEW_SCISSOR | NVC0_NEW_SAMPLE_MASK |
       NVC0_NEW_RASTERIZER | NVC0_NEW_ZSA | NVC0_NEW_BLEND |
       NVC0_NEW_TEXTURES | NVC0_NEW_SAMPLERS |
       NVC0_NEW_VERTPROG | NVC0_NEW_FRAGPROG |
       NVC0_NEW_TCTLPROG | NVC0_NEW_TEVLPROG | NVC0_NEW_GMTYPROG |
       NVC0_NEW_TFB_TARGETS);
}

static void
nvc0_resource_resolve(struct pipe_context *pipe,
                      const struct pipe_resolve_info *info)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nvc0_screen *screen = nvc0->screen;
   struct nvc0_blitctx *blit = screen->blitctx;
   struct nouveau_pushbuf *push = screen->base.pushbuf;
   struct pipe_resource *src = info->src.res;
   struct pipe_resource *dst = info->dst.res;
   float x0, x1, y0, y1;
   float x_range, y_range;

   /* Would need more shader variants or, better, just change the TIC target.
    * But no API creates 3D MS textures ...
    */
   if (src->target == PIPE_TEXTURE_3D)
      return;

   nvc0_blitctx_get_color_mask_and_fp(blit, dst->format, info->mask);

   blit->filter = util_format_is_depth_or_stencil(dst->format) ? 0 : 1;

   nvc0_blitctx_pre_blit(blit, nvc0);

   nvc0_blit_set_dst(nvc0, dst, info->dst.level, info->dst.layer);
   nvc0_blit_set_src(nvc0, src, 0,               info->src.layer, blit->filter);

   nvc0_blitctx_prepare_state(blit);

   nvc0_state_validate(nvc0, ~0, 36);

   x_range =
      (float)(info->src.x1 - info->src.x0) /
      (float)(info->dst.x1 - info->dst.x0);
   y_range =
      (float)(info->src.y1 - info->src.y0) /
      (float)(info->dst.y1 - info->dst.y0);

   x0 = (float)info->src.x0 - x_range * (float)info->dst.x0;
   y0 = (float)info->src.y0 - y_range * (float)info->dst.y0;

   x1 = x0 + 16384.0f * x_range;
   y1 = y0 + 16384.0f * y_range;

   x0 *= (float)(1 << nv50_miptree(src)->ms_x);
   x1 *= (float)(1 << nv50_miptree(src)->ms_x);
   y0 *= (float)(1 << nv50_miptree(src)->ms_y);
   y1 *= (float)(1 << nv50_miptree(src)->ms_y);

   BEGIN_NVC0(push, NVC0_3D(SP_START_ID(5)), 1);
   PUSH_DATA (push,
              blit->fp.code_base + blit->fp_offset);

   IMMED_NVC0(push, NVC0_3D(VIEWPORT_TRANSFORM_EN), 0);

   /* Draw a large triangle in screen coordinates covering the whole
    * render target, with scissors defining the destination region.
    * The vertex is supplied with non-normalized texture coordinates
    * arranged in a way to yield the desired offset and scale.
    */

   BEGIN_NVC0(push, NVC0_3D(SCISSOR_HORIZ(0)), 2);
   PUSH_DATA (push, (info->dst.x1 << 16) | info->dst.x0);
   PUSH_DATA (push, (info->dst.y1 << 16) | info->dst.y0);

   IMMED_NVC0(push, NVC0_3D(VERTEX_BEGIN_GL),
              NVC0_3D_VERTEX_BEGIN_GL_PRIMITIVE_TRIANGLES);

   BEGIN_NVC0(push, NVC0_3D(VTX_ATTR_DEFINE), 3);
   PUSH_DATA (push, 0x74201);
   PUSH_DATAf(push, x0);
   PUSH_DATAf(push, y0);
   BEGIN_NVC0(push, NVC0_3D(VTX_ATTR_DEFINE), 3);
   PUSH_DATA (push, 0x74200);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 0.0f);
   BEGIN_NVC0(push, NVC0_3D(VTX_ATTR_DEFINE), 3);
   PUSH_DATA (push, 0x74201);
   PUSH_DATAf(push, x1);
   PUSH_DATAf(push, y0);
   BEGIN_NVC0(push, NVC0_3D(VTX_ATTR_DEFINE), 3);
   PUSH_DATA (push, 0x74200);
   PUSH_DATAf(push, 16384 << nv50_miptree(dst)->ms_x);
   PUSH_DATAf(push, 0.0f);
   BEGIN_NVC0(push, NVC0_3D(VTX_ATTR_DEFINE), 3);
   PUSH_DATA (push, 0x74201);
   PUSH_DATAf(push, x0);
   PUSH_DATAf(push, y1);
   BEGIN_NVC0(push, NVC0_3D(VTX_ATTR_DEFINE), 3);
   PUSH_DATA (push, 0x74200);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 16384 << nv50_miptree(dst)->ms_y);

   IMMED_NVC0(push, NVC0_3D(VERTEX_END_GL), 0);

   /* re-enable normally constant state */

   IMMED_NVC0(push, NVC0_3D(VIEWPORT_TRANSFORM_EN), 1);

   nvc0_blitctx_post_blit(nvc0, blit);
}

boolean
nvc0_blitctx_create(struct nvc0_screen *screen)
{
   screen->blitctx = CALLOC_STRUCT(nvc0_blitctx);
   if (!screen->blitctx) {
      NOUVEAU_ERR("failed to allocate blit context\n");
      return FALSE;
   }

   screen->blitctx->screen = screen;

   nvc0_blitctx_make_vp(screen->blitctx);
   nvc0_blitctx_make_fp(screen->blitctx);

   nvc0_blitctx_make_sampler(screen->blitctx);

   screen->blitctx->color_mask = 0x1111;

   return TRUE;
}


void
nvc0_init_surface_functions(struct nvc0_context *nvc0)
{
   struct pipe_context *pipe = &nvc0->base.pipe;

   pipe->resource_copy_region = nvc0_resource_copy_region;
   pipe->resource_resolve = nvc0_resource_resolve;
   pipe->clear_render_target = nvc0_clear_render_target;
   pipe->clear_depth_stencil = nvc0_clear_depth_stencil;
}

