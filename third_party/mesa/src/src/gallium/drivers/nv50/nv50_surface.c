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

#include "nv50_context.h"
#include "nv50_resource.h"

#include "nv50_defs.xml.h"
#include "nv50_texture.xml.h"

#define NV50_ENG2D_SUPPORTED_FORMATS 0xff0843e080608409ULL

/* return TRUE for formats that can be converted among each other by NV50_2D */
static INLINE boolean
nv50_2d_format_faithful(enum pipe_format format)
{
   uint8_t id = nv50_format_table[format].rt;

   return (id >= 0xc0) &&
      (NV50_ENG2D_SUPPORTED_FORMATS & (1ULL << (id - 0xc0)));
}

static INLINE uint8_t
nv50_2d_format(enum pipe_format format)
{
   uint8_t id = nv50_format_table[format].rt;

   /* Hardware values for color formats range from 0xc0 to 0xff,
    * but the 2D engine doesn't support all of them.
    */
   if ((id >= 0xc0) && (NV50_ENG2D_SUPPORTED_FORMATS & (1ULL << (id - 0xc0))))
      return id;

   switch (util_format_get_blocksize(format)) {
   case 1:
      return NV50_SURFACE_FORMAT_R8_UNORM;
   case 2:
      return NV50_SURFACE_FORMAT_R16_UNORM;
   case 4:
      return NV50_SURFACE_FORMAT_BGRA8_UNORM;
   default:
      return 0;
   }
}

static int
nv50_2d_texture_set(struct nouveau_pushbuf *push, int dst,
                    struct nv50_miptree *mt, unsigned level, unsigned layer)
{
   struct nouveau_bo *bo = mt->base.bo;
   uint32_t width, height, depth;
   uint32_t format;
   uint32_t mthd = dst ? NV50_2D_DST_FORMAT : NV50_2D_SRC_FORMAT;
   uint32_t offset = mt->level[level].offset;

   format = nv50_2d_format(mt->base.base.format);
   if (!format) {
      NOUVEAU_ERR("invalid/unsupported surface format: %s\n",
                  util_format_name(mt->base.base.format));
      return 1;
   }

   width = u_minify(mt->base.base.width0, level) << mt->ms_x;
   height = u_minify(mt->base.base.height0, level) << mt->ms_y;

   offset = mt->level[level].offset;
   if (!mt->layout_3d) {
      offset += mt->layer_stride * layer;
      depth = 1;
      layer = 0;
   } else {
      depth = u_minify(mt->base.base.depth0, level);
   }

   if (!nouveau_bo_memtype(bo)) {
      BEGIN_NV04(push, SUBC_2D(mthd), 2);
      PUSH_DATA (push, format);
      PUSH_DATA (push, 1);
      BEGIN_NV04(push, SUBC_2D(mthd + 0x14), 5);
      PUSH_DATA (push, mt->level[level].pitch);
      PUSH_DATA (push, width);
      PUSH_DATA (push, height);
      PUSH_DATAh(push, bo->offset + offset);
      PUSH_DATA (push, bo->offset + offset);
   } else {
      BEGIN_NV04(push, SUBC_2D(mthd), 5);
      PUSH_DATA (push, format);
      PUSH_DATA (push, 0);
      PUSH_DATA (push, mt->level[level].tile_mode);
      PUSH_DATA (push, depth);
      PUSH_DATA (push, layer);
      BEGIN_NV04(push, SUBC_2D(mthd + 0x18), 4);
      PUSH_DATA (push, width);
      PUSH_DATA (push, height);
      PUSH_DATAh(push, bo->offset + offset);
      PUSH_DATA (push, bo->offset + offset);
   }

#if 0
   if (dst) {
      BEGIN_NV04(push, SUBC_2D(NV50_2D_CLIP_X), 4);
      PUSH_DATA (push, 0);
      PUSH_DATA (push, 0);
      PUSH_DATA (push, width);
      PUSH_DATA (push, height);
   }
#endif
   return 0;
}

static int
nv50_2d_texture_do_copy(struct nouveau_pushbuf *push,
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
   uint32_t ctrl;
#if 0
   ret = MARK_RING(chan, 2 * 16 + 32, 4);
   if (ret)
      return ret;
#endif
   ret = nv50_2d_texture_set(push, 1, dst, dst_level, dz);
   if (ret)
      return ret;

   ret = nv50_2d_texture_set(push, 0, src, src_level, sz);
   if (ret)
      return ret;

   /* NOTE: 2D engine doesn't work for MS8 */
   if (src->ms_x)
      ctrl = 0x11;

   /* 0/1 = CENTER/CORNER, 00/10 = POINT/BILINEAR */
   BEGIN_NV04(push, NV50_2D(BLIT_CONTROL), 1);
   PUSH_DATA (push, ctrl);
   BEGIN_NV04(push, NV50_2D(BLIT_DST_X), 4);
   PUSH_DATA (push, dx << dst->ms_x);
   PUSH_DATA (push, dy << dst->ms_y);
   PUSH_DATA (push, w << dst->ms_x);
   PUSH_DATA (push, h << dst->ms_y);
   BEGIN_NV04(push, NV50_2D(BLIT_DU_DX_FRACT), 4);
   PUSH_DATA (push, duvdxy[2 + ((int)src->ms_x - (int)dst->ms_x)] & 0xf0000000);
   PUSH_DATA (push, duvdxy[2 + ((int)src->ms_x - (int)dst->ms_x)] & 0x0000000f);
   PUSH_DATA (push, duvdxy[2 + ((int)src->ms_y - (int)dst->ms_y)] & 0xf0000000);
   PUSH_DATA (push, duvdxy[2 + ((int)src->ms_y - (int)dst->ms_y)] & 0x0000000f);
   BEGIN_NV04(push, NV50_2D(BLIT_SRC_X_FRACT), 4);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, sx << src->ms_x);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, sy << src->ms_y);

   return 0;
}

static void
nv50_resource_copy_region(struct pipe_context *pipe,
                          struct pipe_resource *dst, unsigned dst_level,
                          unsigned dstx, unsigned dsty, unsigned dstz,
                          struct pipe_resource *src, unsigned src_level,
                          const struct pipe_box *src_box)
{
   struct nv50_context *nv50 = nv50_context(pipe);
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
         nv50_m2mf_transfer_rect(nv50, &drect, &srect, nx, ny);

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

   assert((src->format == dst->format) ||
          (nv50_2d_format_faithful(src->format) &&
           nv50_2d_format_faithful(dst->format)));

   BCTX_REFN(nv50->bufctx, 2D, nv04_resource(src), RD);
   BCTX_REFN(nv50->bufctx, 2D, nv04_resource(dst), WR);
   nouveau_pushbuf_bufctx(nv50->base.pushbuf, nv50->bufctx);
   nouveau_pushbuf_validate(nv50->base.pushbuf);

   for (; dst_layer < dstz + src_box->depth; ++dst_layer, ++src_layer) {
      ret = nv50_2d_texture_do_copy(nv50->base.pushbuf,
                                    nv50_miptree(dst), dst_level,
                                    dstx, dsty, dst_layer,
                                    nv50_miptree(src), src_level,
                                    src_box->x, src_box->y, src_layer,
                                    src_box->width, src_box->height);
      if (ret)
         break;
   }
   nouveau_bufctx_reset(nv50->bufctx, NV50_BIND_2D);
}

static void
nv50_clear_render_target(struct pipe_context *pipe,
                         struct pipe_surface *dst,
			 const union pipe_color_union *color,
                         unsigned dstx, unsigned dsty,
                         unsigned width, unsigned height)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_miptree *mt = nv50_miptree(dst->texture);
   struct nv50_surface *sf = nv50_surface(dst);
   struct nouveau_bo *bo = mt->base.bo;

   BEGIN_NV04(push, NV50_3D(CLEAR_COLOR(0)), 4);
   PUSH_DATAf(push, color->f[0]);
   PUSH_DATAf(push, color->f[1]);
   PUSH_DATAf(push, color->f[2]);
   PUSH_DATAf(push, color->f[3]);
#if 0
   if (MARK_RING(chan, 18, 2))
      return;
#endif
   BEGIN_NV04(push, NV50_3D(RT_CONTROL), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_3D(RT_ADDRESS_HIGH(0)), 5);
   PUSH_DATAh(push, bo->offset + sf->offset);
   PUSH_DATA (push, bo->offset + sf->offset);
   PUSH_DATA (push, nv50_format_table[dst->format].rt);
   PUSH_DATA (push, mt->level[sf->base.u.tex.level].tile_mode);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(RT_HORIZ(0)), 2);
   if (nouveau_bo_memtype(bo))
      PUSH_DATA(push, sf->width);
   else
      PUSH_DATA(push, NV50_3D_RT_HORIZ_LINEAR | mt->level[0].pitch);
   PUSH_DATA (push, sf->height);
   BEGIN_NV04(push, NV50_3D(RT_ARRAY_MODE), 1);
   PUSH_DATA (push, 1);

   if (!nouveau_bo_memtype(bo)) {
      BEGIN_NV04(push, NV50_3D(ZETA_ENABLE), 1);
      PUSH_DATA (push, 0);
   }

   /* NOTE: only works with D3D clear flag (5097/0x143c bit 4) */

   BEGIN_NV04(push, NV50_3D(VIEWPORT_HORIZ(0)), 2);
   PUSH_DATA (push, (width << 16) | dstx);
   PUSH_DATA (push, (height << 16) | dsty);

   BEGIN_NV04(push, NV50_3D(CLEAR_BUFFERS), 1);
   PUSH_DATA (push, 0x3c);

   nv50->dirty |= NV50_NEW_FRAMEBUFFER;
}

static void
nv50_clear_depth_stencil(struct pipe_context *pipe,
                         struct pipe_surface *dst,
                         unsigned clear_flags,
                         double depth,
                         unsigned stencil,
                         unsigned dstx, unsigned dsty,
                         unsigned width, unsigned height)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_miptree *mt = nv50_miptree(dst->texture);
   struct nv50_surface *sf = nv50_surface(dst);
   struct nouveau_bo *bo = mt->base.bo;
   uint32_t mode = 0;

   assert(nouveau_bo_memtype(bo)); /* ZETA cannot be linear */

   if (clear_flags & PIPE_CLEAR_DEPTH) {
      BEGIN_NV04(push, NV50_3D(CLEAR_DEPTH), 1);
      PUSH_DATAf(push, depth);
      mode |= NV50_3D_CLEAR_BUFFERS_Z;
   }

   if (clear_flags & PIPE_CLEAR_STENCIL) {
      BEGIN_NV04(push, NV50_3D(CLEAR_STENCIL), 1);
      PUSH_DATA (push, stencil & 0xff);
      mode |= NV50_3D_CLEAR_BUFFERS_S;
   }
#if 0
   if (MARK_RING(chan, 17, 2))
      return;
#endif
   BEGIN_NV04(push, NV50_3D(ZETA_ADDRESS_HIGH), 5);
   PUSH_DATAh(push, bo->offset + sf->offset);
   PUSH_DATA (push, bo->offset + sf->offset);
   PUSH_DATA (push, nv50_format_table[dst->format].rt);
   PUSH_DATA (push, mt->level[sf->base.u.tex.level].tile_mode);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(ZETA_ENABLE), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_3D(ZETA_HORIZ), 3);
   PUSH_DATA (push, sf->width);
   PUSH_DATA (push, sf->height);
   PUSH_DATA (push, (1 << 16) | 1);

   BEGIN_NV04(push, NV50_3D(VIEWPORT_HORIZ(0)), 2);
   PUSH_DATA (push, (width << 16) | dstx);
   PUSH_DATA (push, (height << 16) | dsty);

   BEGIN_NV04(push, NV50_3D(CLEAR_BUFFERS), 1);
   PUSH_DATA (push, mode);

   nv50->dirty |= NV50_NEW_FRAMEBUFFER;
}

void
nv50_clear(struct pipe_context *pipe, unsigned buffers,
           const union pipe_color_union *color,
           double depth, unsigned stencil)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct pipe_framebuffer_state *fb = &nv50->framebuffer;
   unsigned i;
   uint32_t mode = 0;

   /* don't need NEW_BLEND, COLOR_MASK doesn't affect CLEAR_BUFFERS */
   if (!nv50_state_validate(nv50, NV50_NEW_FRAMEBUFFER, 9 + (fb->nr_cbufs * 2)))
      return;

   if (buffers & PIPE_CLEAR_COLOR && fb->nr_cbufs) {
      BEGIN_NV04(push, NV50_3D(CLEAR_COLOR(0)), 4);
      PUSH_DATAf(push, color->f[0]);
      PUSH_DATAf(push, color->f[1]);
      PUSH_DATAf(push, color->f[2]);
      PUSH_DATAf(push, color->f[3]);
      mode =
         NV50_3D_CLEAR_BUFFERS_R | NV50_3D_CLEAR_BUFFERS_G |
         NV50_3D_CLEAR_BUFFERS_B | NV50_3D_CLEAR_BUFFERS_A;
   }

   if (buffers & PIPE_CLEAR_DEPTH) {
      BEGIN_NV04(push, NV50_3D(CLEAR_DEPTH), 1);
      PUSH_DATA (push, fui(depth));
      mode |= NV50_3D_CLEAR_BUFFERS_Z;
   }

   if (buffers & PIPE_CLEAR_STENCIL) {
      BEGIN_NV04(push, NV50_3D(CLEAR_STENCIL), 1);
      PUSH_DATA (push, stencil & 0xff);
      mode |= NV50_3D_CLEAR_BUFFERS_S;
   }

   BEGIN_NV04(push, NV50_3D(CLEAR_BUFFERS), 1);
   PUSH_DATA (push, mode);

   for (i = 1; i < fb->nr_cbufs; i++) {
      BEGIN_NV04(push, NV50_3D(CLEAR_BUFFERS), 1);
      PUSH_DATA (push, (i << 6) | 0x3c);
   }
}


struct nv50_blitctx
{
   struct nv50_screen *screen;
   struct {
      struct pipe_framebuffer_state fb;
      struct nv50_program *vp;
      struct nv50_program *gp;
      struct nv50_program *fp;
      unsigned num_textures[3];
      unsigned num_samplers[3];
      struct pipe_sampler_view *texture[2];
      struct nv50_tsc_entry *sampler[2];
      unsigned dirty;
   } saved;
   struct nv50_program vp;
   struct nv50_program fp;
   struct nv50_tsc_entry sampler[2]; /* nearest, bilinear */
   uint32_t fp_offset;
   uint16_t color_mask;
   uint8_t filter;
};

static void
nv50_blitctx_make_vp(struct nv50_blitctx *blit)
{
   static const uint32_t code[] =
   {
      0x10000001, /* mov b32 o[0x00] s[0x00] */ /* HPOS.x */
      0x0423c788,
      0x10000205, /* mov b32 o[0x04] s[0x04] */ /* HPOS.y */
      0x0423c788,
      0x10000409, /* mov b32 o[0x08] s[0x08] */ /* TEXC.x */
      0x0423c788,
      0x1000060d, /* mov b32 o[0x0c] s[0x0c] */ /* TEXC.y */
      0x0423c788,
      0x10000811, /* exit mov b32 o[0x10] s[0x10] */ /* TEXC.z */
      0x0423c789,
   };

   blit->vp.type = PIPE_SHADER_VERTEX;
   blit->vp.translated = TRUE;
   blit->vp.code = (uint32_t *)code; /* const_cast */
   blit->vp.code_size = sizeof(code);
   blit->vp.max_gpr = 4;
   blit->vp.max_out = 5;
   blit->vp.out_nr = 2;
   blit->vp.out[0].mask = 0x3;
   blit->vp.out[0].sn = TGSI_SEMANTIC_POSITION;
   blit->vp.out[1].hw = 2;
   blit->vp.out[1].mask = 0x7;
   blit->vp.out[1].sn = TGSI_SEMANTIC_GENERIC;
   blit->vp.vp.attrs[0] = 0x73;
   blit->vp.vp.psiz = 0x40;
   blit->vp.vp.edgeflag = 0x40;
}

static void
nv50_blitctx_make_fp(struct nv50_blitctx *blit)
{
   static const uint32_t code[] =
   {
      /* 3 coords RGBA in, RGBA out, also for Z32_FLOAT(_S8X24_UINT) */
      0x80000000, /* interp $r0 v[0x0] */
      0x80010004, /* interp $r1 v[0x4] */
      0x80020009, /* interp $r2 flat v[0x8] */
      0x00040780,
      0xf6800001, /* texauto live { $r0,1,2,3 } $t0 $s0 { $r0,1,2 } */
      0x0000c785, /* exit */

      /* 3 coords ZS in, S encoded in R, Z encoded in GBA (8_UNORM) */
      0x80000000, /* interp $r0 v[0x00] */
      0x80010004, /* interp $r1 v[0x04] */
      0x80020108, /* interp $r2 flat v[0x8] */
      0x10008010, /* mov b32 $r4 $r0 */
      0xf2820201, /* texauto live { $r0,#,#,# } $t1 $s1 { $r0,1,2 } */
      0x00000784,
      0xa000000d, /* cvt f32 $r3 s32 $r0 */
      0x44014780,
      0x10000801, /* mov b32 $r0 $r4 */
      0x0403c780,
      0xf2800001, /* texauto live { $r0,#,#,# } $t0 $s0 { $r0,1,2 } */
      0x00000784,
      0xc03f0009, /* mul f32 $r2 $r0 (2^24 - 1) */
      0x04b7ffff,
      0xa0000409, /* cvt rni s32 $r2 f32 $r2 */
      0x8c004780,
      0xc0010601, /* mul f32 $r0 $r3 1/0xff */
      0x03b8080b,
      0xd03f0405, /* and b32 $r1 $r2 0x0000ff */
      0x0000000f,
      0xd000040d, /* and b32 $r3 $r2 0xff0000 */
      0x000ff003,
      0xd0000409, /* and b32 $r2 $r2 0x00ff00 */
      0x00000ff3,
      0xa0000205, /* cvt f32 $r1 s32 $r1 */
      0x44014780,
      0xa000060d, /* cvt f32 $r3 s32 $r3 */
      0x44014780,
      0xa0000409, /* cvt f32 $r2 s32 $r2 */
      0x44014780,
      0xc0010205, /* mul f32 $r1 $r1 1/0x0000ff */
      0x03b8080b,
      0xc001060d, /* mul f32 $r3 $r3 1/0x00ff00 */
      0x0338080b,
      0xc0010409, /* mul f32 $r2 $r2 1/0xff0000 */
      0x0378080b,
      0xf0000001, /* exit never nop */
      0xe0000001,

      /* 3 coords ZS in, Z encoded in RGB, S encoded in A (U8_UNORM) */
      0x80000000, /* interp $r0 v[0x00] */
      0x80010004, /* interp $r1 v[0x04] */
      0x80020108, /* interp $r2 flat v[0x8] */
      0x10008010, /* mov b32 $r4 $r0 */
      0xf2820201, /* texauto live { $r0,#,#,# } $t1 $s1 { $r0,1,2 } */
      0x00000784,
      0xa000000d, /* cvt f32 $r3 s32 $r0 */
      0x44014780,
      0x10000801, /* mov b32 $r0 $r4 */
      0x0403c780,
      0xf2800001, /* texauto live { $r0,#,#,# } $t0 $s0 { $r0,1,2 } */
      0x00000784,
      0xc03f0009, /* mul f32 $r2 $r0 (2^24 - 1) */
      0x04b7ffff,
      0xa0000409, /* cvt rni s32 $r2 f32 $r2 */
      0x8c004780,
      0xc001060d, /* mul f32 $r3 $r3 1/0xff */
      0x03b8080b,
      0xd03f0401, /* and b32 $r0 $r2 0x0000ff */
      0x0000000f,
      0xd0000405, /* and b32 $r1 $r2 0x00ff00 */
      0x00000ff3,
      0xd0000409, /* and b32 $r2 $r2 0xff0000 */
      0x000ff003,
      0xa0000001, /* cvt f32 $r0 s32 $r0 */
      0x44014780,
      0xa0000205, /* cvt f32 $r1 s32 $r1 */
      0x44014780,
      0xa0000409, /* cvt f32 $r2 s32 $r2 */
      0x44014780,
      0xc0010001, /* mul f32 $r0 $r0 1/0x0000ff */
      0x03b8080b,
      0xc0010205, /* mul f32 $r1 $r1 1/0x00ff00 */
      0x0378080b,
      0xc0010409, /* mul f32 $r2 $r2 1/0xff0000 */
      0x0338080b,
      0xf0000001, /* exit never nop */
      0xe0000001
   };

   blit->fp.type = PIPE_SHADER_FRAGMENT;
   blit->fp.translated = TRUE;
   blit->fp.code = (uint32_t *)code; /* const_cast */
   blit->fp.code_size = sizeof(code);
   blit->fp.max_gpr = 5;
   blit->fp.max_out = 4;
   blit->fp.in_nr = 1;
   blit->fp.in[0].mask = 0x7; /* last component flat */
   blit->fp.in[0].linear = 1;
   blit->fp.in[0].sn = TGSI_SEMANTIC_GENERIC;
   blit->fp.out_nr = 1;
   blit->fp.out[0].mask = 0xf;
   blit->fp.out[0].sn = TGSI_SEMANTIC_COLOR;
   blit->fp.fp.interp = 0x00020403;
   blit->fp.gp.primid = 0x80;
}

static void
nv50_blitctx_make_sampler(struct nv50_blitctx *blit)
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
nv50_blit_zeta_to_colour_format(enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_Z16_UNORM:               return PIPE_FORMAT_R16_UNORM;
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:             return PIPE_FORMAT_R8G8B8A8_UNORM;
   case PIPE_FORMAT_Z32_FLOAT:               return PIPE_FORMAT_R32_FLOAT;
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT: return PIPE_FORMAT_R32G32_FLOAT;
   default:
      assert(0);
      return PIPE_FORMAT_NONE;
   }
}

static void
nv50_blitctx_get_color_mask_and_fp(struct nv50_blitctx *blit,
                                   enum pipe_format format, uint8_t mask)
{
   blit->color_mask = 0;

   switch (format) {
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      blit->fp_offset = 0xb0;
      if (mask & PIPE_MASK_Z)
         blit->color_mask |= 0x0111;
      if (mask & PIPE_MASK_S)
         blit->color_mask |= 0x1000;
      break;
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      blit->fp_offset = 0x18;
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
nv50_blit_set_dst(struct nv50_context *nv50,
                  struct pipe_resource *res, unsigned level, unsigned layer)
{
   struct pipe_context *pipe = &nv50->base.pipe;
   struct pipe_surface templ;

   if (util_format_is_depth_or_stencil(res->format))
      templ.format = nv50_blit_zeta_to_colour_format(res->format);
   else
      templ.format = res->format;

   templ.usage = PIPE_USAGE_STREAM;
   templ.u.tex.level = level;
   templ.u.tex.first_layer = templ.u.tex.last_layer = layer;

   nv50->framebuffer.cbufs[0] = nv50_miptree_surface_new(pipe, res, &templ);
   nv50->framebuffer.nr_cbufs = 1;
   nv50->framebuffer.zsbuf = NULL;
   nv50->framebuffer.width = nv50->framebuffer.cbufs[0]->width;
   nv50->framebuffer.height = nv50->framebuffer.cbufs[0]->height;
}

static INLINE void
nv50_blit_fixup_tic_entry(struct pipe_sampler_view *view)
{
   struct nv50_tic_entry *ent = nv50_tic_entry(view);

   ent->tic[2] &= ~(1 << 31); /* scaled coordinates, ok with 3d textures ? */

   /* magic: */

   ent->tic[3] = 0x20000000; /* affects quality of near vertical edges in MS8 */
}

static void
nv50_blit_set_src(struct nv50_context *nv50,
                  struct pipe_resource *res, unsigned level, unsigned layer)
{
   struct pipe_context *pipe = &nv50->base.pipe;
   struct pipe_sampler_view templ;

   templ.format = res->format;
   templ.u.tex.first_layer = templ.u.tex.last_layer = layer;
   templ.u.tex.first_level = templ.u.tex.last_level = level;
   templ.swizzle_r = PIPE_SWIZZLE_RED;
   templ.swizzle_g = PIPE_SWIZZLE_GREEN;
   templ.swizzle_b = PIPE_SWIZZLE_BLUE;
   templ.swizzle_a = PIPE_SWIZZLE_ALPHA;

   nv50->textures[2][0] = nv50_create_sampler_view(pipe, res, &templ);
   nv50->textures[2][1] = NULL;

   nv50_blit_fixup_tic_entry(nv50->textures[2][0]);

   nv50->num_textures[0] = nv50->num_textures[1] = 0;
   nv50->num_textures[2] = 1;

   templ.format = nv50_zs_to_s_format(res->format);
   if (templ.format != res->format) {
      nv50->textures[2][1] = nv50_create_sampler_view(pipe, res, &templ);
      nv50_blit_fixup_tic_entry(nv50->textures[2][1]);
      nv50->num_textures[2] = 2;
   }
}

static void
nv50_blitctx_prepare_state(struct nv50_blitctx *blit)
{
   struct nouveau_pushbuf *push = blit->screen->base.pushbuf;

   /* blend state */
   BEGIN_NV04(push, NV50_3D(COLOR_MASK(0)), 1);
   PUSH_DATA (push, blit->color_mask);
   BEGIN_NV04(push, NV50_3D(BLEND_ENABLE(0)), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(LOGIC_OP_ENABLE), 1);
   PUSH_DATA (push, 0);

   /* rasterizer state */
#ifndef NV50_SCISSORS_CLIPPING
   BEGIN_NV04(push, NV50_3D(SCISSOR_ENABLE(0)), 1);
   PUSH_DATA (push, 1);
#endif
   BEGIN_NV04(push, NV50_3D(VERTEX_TWO_SIDE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(FRAG_COLOR_CLAMP_EN), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(MULTISAMPLE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(MSAA_MASK(0)), 4);
   PUSH_DATA (push, 0xffff);
   PUSH_DATA (push, 0xffff);
   PUSH_DATA (push, 0xffff);
   PUSH_DATA (push, 0xffff);
   BEGIN_NV04(push, NV50_3D(POLYGON_MODE_FRONT), 3);
   PUSH_DATA (push, NV50_3D_POLYGON_MODE_FRONT_FILL);
   PUSH_DATA (push, NV50_3D_POLYGON_MODE_BACK_FILL);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(CULL_FACE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(POLYGON_STIPPLE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(POLYGON_OFFSET_FILL_ENABLE), 1);
   PUSH_DATA (push, 0);

   /* zsa state */
   BEGIN_NV04(push, NV50_3D(DEPTH_TEST_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(STENCIL_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(ALPHA_TEST_ENABLE), 1);
   PUSH_DATA (push, 0);
}

static void
nv50_blitctx_pre_blit(struct nv50_blitctx *blit, struct nv50_context *nv50)
{
   int s;

   blit->saved.fb.width = nv50->framebuffer.width;
   blit->saved.fb.height = nv50->framebuffer.height;
   blit->saved.fb.nr_cbufs = nv50->framebuffer.nr_cbufs;
   blit->saved.fb.cbufs[0] = nv50->framebuffer.cbufs[0];
   blit->saved.fb.zsbuf = nv50->framebuffer.zsbuf;

   blit->saved.vp = nv50->vertprog;
   blit->saved.gp = nv50->gmtyprog;
   blit->saved.fp = nv50->fragprog;

   nv50->vertprog = &blit->vp;
   nv50->gmtyprog = NULL;
   nv50->fragprog = &blit->fp;

   for (s = 0; s < 3; ++s) {
      blit->saved.num_textures[s] = nv50->num_textures[s];
      blit->saved.num_samplers[s] = nv50->num_samplers[s];
   }
   blit->saved.texture[0] = nv50->textures[2][0];
   blit->saved.texture[1] = nv50->textures[2][1];
   blit->saved.sampler[0] = nv50->samplers[2][0];
   blit->saved.sampler[1] = nv50->samplers[2][1];

   nv50->samplers[2][0] = &blit->sampler[blit->filter];
   nv50->samplers[2][1] = &blit->sampler[blit->filter];

   nv50->num_samplers[0] = nv50->num_samplers[1] = 0;
   nv50->num_samplers[2] = 2;

   blit->saved.dirty = nv50->dirty;

   nv50->dirty =
      NV50_NEW_FRAMEBUFFER |
      NV50_NEW_VERTPROG | NV50_NEW_FRAGPROG | NV50_NEW_GMTYPROG |
      NV50_NEW_TEXTURES | NV50_NEW_SAMPLERS;
}

static void
nv50_blitctx_post_blit(struct nv50_context *nv50, struct nv50_blitctx *blit)
{
   int s;

   pipe_surface_reference(&nv50->framebuffer.cbufs[0], NULL);

   nv50->framebuffer.width = blit->saved.fb.width;
   nv50->framebuffer.height = blit->saved.fb.height;
   nv50->framebuffer.nr_cbufs = blit->saved.fb.nr_cbufs;
   nv50->framebuffer.cbufs[0] = blit->saved.fb.cbufs[0];
   nv50->framebuffer.zsbuf = blit->saved.fb.zsbuf;

   nv50->vertprog = blit->saved.vp;
   nv50->gmtyprog = blit->saved.gp;
   nv50->fragprog = blit->saved.fp;

   pipe_sampler_view_reference(&nv50->textures[2][0], NULL);
   pipe_sampler_view_reference(&nv50->textures[2][1], NULL);

   for (s = 0; s < 3; ++s) {
      nv50->num_textures[s] = blit->saved.num_textures[s];
      nv50->num_samplers[s] = blit->saved.num_samplers[s];
   }
   nv50->textures[2][0] = blit->saved.texture[0];
   nv50->textures[2][1] = blit->saved.texture[1];
   nv50->samplers[2][0] = blit->saved.sampler[0];
   nv50->samplers[2][1] = blit->saved.sampler[1];

   nv50->dirty = blit->saved.dirty |
      (NV50_NEW_FRAMEBUFFER | NV50_NEW_SCISSOR | NV50_NEW_SAMPLE_MASK |
       NV50_NEW_RASTERIZER | NV50_NEW_ZSA | NV50_NEW_BLEND |
       NV50_NEW_TEXTURES | NV50_NEW_SAMPLERS |
       NV50_NEW_VERTPROG | NV50_NEW_GMTYPROG | NV50_NEW_FRAGPROG);
}

static void
nv50_resource_resolve(struct pipe_context *pipe,
                      const struct pipe_resolve_info *info)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nv50_screen *screen = nv50->screen;
   struct nv50_blitctx *blit = screen->blitctx;
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct pipe_resource *src = info->src.res;
   struct pipe_resource *dst = info->dst.res;
   float x0, x1, y0, y1, z;
   float x_range, y_range;

   nv50_blitctx_get_color_mask_and_fp(blit, dst->format, info->mask);

   blit->filter = util_format_is_depth_or_stencil(dst->format) ? 0 : 1;

   nv50_blitctx_pre_blit(blit, nv50);

   nv50_blit_set_dst(nv50, dst, info->dst.level, info->dst.layer);
   nv50_blit_set_src(nv50, src, 0,               info->src.layer);

   nv50_blitctx_prepare_state(blit);

   nv50_state_validate(nv50, ~0, 36);

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

   z = (float)info->src.layer;

   BEGIN_NV04(push, NV50_3D(FP_START_ID), 1);
   PUSH_DATA (push,
              blit->fp.code_base + blit->fp_offset);

   BEGIN_NV04(push, NV50_3D(VIEWPORT_TRANSFORM_EN), 1);
   PUSH_DATA (push, 0);

   /* Draw a large triangle in screen coordinates covering the whole
    * render target, with scissors defining the destination region.
    * The vertex is supplied with non-normalized texture coordinates
    * arranged in a way to yield the desired offset and scale.
    */

   BEGIN_NV04(push, NV50_3D(SCISSOR_HORIZ(0)), 2);
   PUSH_DATA (push, (info->dst.x1 << 16) | info->dst.x0);
   PUSH_DATA (push, (info->dst.y1 << 16) | info->dst.y0);

   BEGIN_NV04(push, NV50_3D(VERTEX_BEGIN_GL), 1);
   PUSH_DATA (push, NV50_3D_VERTEX_BEGIN_GL_PRIMITIVE_TRIANGLES);
   BEGIN_NV04(push, NV50_3D(VTX_ATTR_3F_X(1)), 3);
   PUSH_DATAf(push, x0);
   PUSH_DATAf(push, y0);
   PUSH_DATAf(push, z);
   BEGIN_NV04(push, NV50_3D(VTX_ATTR_2F_X(0)), 2);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 0.0f);
   BEGIN_NV04(push, NV50_3D(VTX_ATTR_3F_X(1)), 3);
   PUSH_DATAf(push, x1);
   PUSH_DATAf(push, y0);
   PUSH_DATAf(push, z);
   BEGIN_NV04(push, NV50_3D(VTX_ATTR_2F_X(0)), 2);
   PUSH_DATAf(push, 16384 << nv50_miptree(dst)->ms_x);
   PUSH_DATAf(push, 0.0f);
   BEGIN_NV04(push, NV50_3D(VTX_ATTR_3F_X(1)), 3);
   PUSH_DATAf(push, x0);
   PUSH_DATAf(push, y1);
   PUSH_DATAf(push, z);
   BEGIN_NV04(push, NV50_3D(VTX_ATTR_2F_X(0)), 2);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 16384 << nv50_miptree(dst)->ms_y);
   BEGIN_NV04(push, NV50_3D(VERTEX_END_GL), 1);
   PUSH_DATA (push, 0);

   /* re-enable normally constant state */

   BEGIN_NV04(push, NV50_3D(VIEWPORT_TRANSFORM_EN), 1);
   PUSH_DATA (push, 1);

   nv50_blitctx_post_blit(nv50, blit);
}

boolean
nv50_blitctx_create(struct nv50_screen *screen)
{
   screen->blitctx = CALLOC_STRUCT(nv50_blitctx);
   if (!screen->blitctx) {
      NOUVEAU_ERR("failed to allocate blit context\n");
      return FALSE;
   }

   screen->blitctx->screen = screen;

   nv50_blitctx_make_vp(screen->blitctx);
   nv50_blitctx_make_fp(screen->blitctx);

   nv50_blitctx_make_sampler(screen->blitctx);

   screen->blitctx->color_mask = 0x1111;

   return TRUE;
}

void
nv50_init_surface_functions(struct nv50_context *nv50)
{
   struct pipe_context *pipe = &nv50->base.pipe;

   pipe->resource_copy_region = nv50_resource_copy_region;
   pipe->resource_resolve = nv50_resource_resolve;
   pipe->clear_render_target = nv50_clear_render_target;
   pipe->clear_depth_stencil = nv50_clear_depth_stencil;
}


