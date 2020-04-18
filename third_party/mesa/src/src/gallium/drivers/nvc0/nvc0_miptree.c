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

#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"

#include "nvc0_context.h"
#include "nvc0_resource.h"

uint32_t
nvc0_tex_choose_tile_dims(unsigned nx, unsigned ny, unsigned nz)
{
   uint32_t tile_mode = 0x000;

   if (ny > 64) tile_mode = 0x040; /* height 128 tiles */
   else
   if (ny > 32) tile_mode = 0x030; /* height 64 tiles */
   else
   if (ny > 16) tile_mode = 0x020; /* height 32 tiles */
   else
   if (ny >  8) tile_mode = 0x010; /* height 16 tiles */

   if (nz == 1)
      return tile_mode;
   else
   if (tile_mode > 0x020)
      tile_mode = 0x020;

   if (nz > 16 && tile_mode < 0x020)
      return tile_mode | 0x500; /* depth 32 tiles */
   if (nz > 8) return tile_mode | 0x400; /* depth 16 tiles */
   if (nz > 4) return tile_mode | 0x300; /* depth 8 tiles */
   if (nz > 2) return tile_mode | 0x200; /* depth 4 tiles */

   return tile_mode | 0x100;
}

static uint32_t
nvc0_mt_choose_storage_type(struct nv50_miptree *mt, boolean compressed)
{
   const unsigned ms = util_logbase2(mt->base.base.nr_samples);

   uint32_t tile_flags;

   compressed = FALSE; /* not yet supported */

   if (unlikely(mt->base.base.bind & PIPE_BIND_CURSOR))
      return 0;
   if (unlikely(mt->base.base.flags & NOUVEAU_RESOURCE_FLAG_LINEAR))
      return 0;

   switch (mt->base.base.format) {
   case PIPE_FORMAT_Z16_UNORM:
      if (compressed)
         tile_flags = 0x02 + ms;
      else
         tile_flags = 0x01;
      break;
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      if (compressed)
         tile_flags = 0x51 + ms;
      else
         tile_flags = 0x46;
      break;
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      if (compressed)
         tile_flags = 0x17 + ms;
      else
         tile_flags = 0x11;
      break;
   case PIPE_FORMAT_Z32_FLOAT:
      if (compressed)
         tile_flags = 0x86 + ms;
      else
         tile_flags = 0x7b;
      break;
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      if (compressed)
         tile_flags = 0xce + ms;
      else
         tile_flags = 0xc3;
      break;
   default:
      switch (util_format_get_blocksizebits(mt->base.base.format)) {
      case 128:
         if (compressed)
            tile_flags = 0xf4 + ms;
         else
            tile_flags = 0xfe;
         break;
      case 64:
         if (compressed) {
            switch (ms) {
            case 0: tile_flags = 0xe6; break;
            case 1: tile_flags = 0xeb; break;
            case 2: tile_flags = 0xed; break;
            case 3: tile_flags = 0xf2; break;
            default:
               return 0;
            }
         } else {
            tile_flags = 0xfe;
         }
         break;
      case 32:
         if (compressed) {
            switch (ms) {
            case 0: tile_flags = 0xdb; break;
            case 1: tile_flags = 0xdd; break;
            case 2: tile_flags = 0xdf; break;
            case 3: tile_flags = 0xe4; break;
            default:
               return 0;
            }
         } else {
            tile_flags = 0xfe;
         }
         break;
      case 16:
      case 8:
         tile_flags = 0xfe;
         break;
      default:
         return 0;
      }
      break;
   }

   return tile_flags;
}

static INLINE boolean
nvc0_miptree_init_ms_mode(struct nv50_miptree *mt)
{
   switch (mt->base.base.nr_samples) {
   case 8:
      mt->ms_mode = NVC0_3D_MULTISAMPLE_MODE_MS8;
      mt->ms_x = 2;
      mt->ms_y = 1;
      break;
   case 4:
      mt->ms_mode = NVC0_3D_MULTISAMPLE_MODE_MS4;
      mt->ms_x = 1;
      mt->ms_y = 1;
      break;
   case 2:
      mt->ms_mode = NVC0_3D_MULTISAMPLE_MODE_MS2;
      mt->ms_x = 1;
      break;
   case 1:
   case 0:
      mt->ms_mode = NVC0_3D_MULTISAMPLE_MODE_MS1;
      break;
   default:
      NOUVEAU_ERR("invalid nr_samples: %u\n", mt->base.base.nr_samples);
      return FALSE;
   }
   return TRUE;
}

boolean
nv50_miptree_init_layout_linear(struct nv50_miptree *);

static void
nvc0_miptree_init_layout_video(struct nv50_miptree *mt)
{
   const struct pipe_resource *pt = &mt->base.base;
   const unsigned blocksize = util_format_get_blocksize(pt->format);

   unsigned nbx = util_format_get_nblocksx(pt->format, pt->width0);
   unsigned nby = util_format_get_nblocksy(pt->format, pt->height0);

   assert(pt->last_level == 0);
   assert(mt->ms_x == 0 &&
          mt->ms_y == 0);
   assert(!util_format_is_compressed(pt->format));

   assert(nby > 8);
   mt->level[0].tile_mode = 0x10;
   mt->level[0].pitch = align(nbx * blocksize, 64);
   mt->total_size = align(nby, 16) * mt->level[0].pitch;

   if (pt->array_size > 1) {
      mt->layer_stride = align(mt->total_size, NVC0_TILE_SIZE(0x10));
      mt->total_size = mt->layer_stride * pt->array_size;
   }
}

static void
nvc0_miptree_init_layout_tiled(struct nv50_miptree *mt)
{
   struct pipe_resource *pt = &mt->base.base;
   unsigned w, h, d, l;
   const unsigned blocksize = util_format_get_blocksize(pt->format);

   mt->layout_3d = pt->target == PIPE_TEXTURE_3D;

   w = pt->width0 << mt->ms_x;
   h = pt->height0 << mt->ms_y;

   /* For 3D textures, a mipmap is spanned by all the layers, for array
    * textures and cube maps, each layer contains its own mipmaps.
    */
   d = mt->layout_3d ? pt->depth0 : 1;

   for (l = 0; l <= pt->last_level; ++l) {
      struct nv50_miptree_level *lvl = &mt->level[l];
      unsigned tsx, tsy, tsz;
      unsigned nbx = util_format_get_nblocksx(pt->format, w);
      unsigned nby = util_format_get_nblocksy(pt->format, h);

      lvl->offset = mt->total_size;

      lvl->tile_mode = nvc0_tex_choose_tile_dims(nbx, nby, d);

      tsx = NVC0_TILE_SIZE_X(lvl->tile_mode); /* x is tile row pitch in bytes */
      tsy = NVC0_TILE_SIZE_Y(lvl->tile_mode);
      tsz = NVC0_TILE_SIZE_Z(lvl->tile_mode);

      lvl->pitch = align(nbx * blocksize, tsx);

      mt->total_size += lvl->pitch * align(nby, tsy) * align(d, tsz);

      w = u_minify(w, 1);
      h = u_minify(h, 1);
      d = u_minify(d, 1);
   }

   if (pt->array_size > 1) {
      mt->layer_stride = align(mt->total_size,
                               NVC0_TILE_SIZE(mt->level[0].tile_mode));
      mt->total_size = mt->layer_stride * pt->array_size;
   }
}

const struct u_resource_vtbl nvc0_miptree_vtbl =
{
   nv50_miptree_get_handle,         /* get_handle */
   nv50_miptree_destroy,            /* resource_destroy */
   nvc0_miptree_transfer_new,       /* get_transfer */
   nvc0_miptree_transfer_del,       /* transfer_destroy */
   nvc0_miptree_transfer_map,       /* transfer_map */
   u_default_transfer_flush_region, /* transfer_flush_region */
   nvc0_miptree_transfer_unmap,     /* transfer_unmap */
   u_default_transfer_inline_write  /* transfer_inline_write */
};

struct pipe_resource *
nvc0_miptree_create(struct pipe_screen *pscreen,
                    const struct pipe_resource *templ)
{
   struct nouveau_device *dev = nouveau_screen(pscreen)->device;
   struct nv50_miptree *mt = CALLOC_STRUCT(nv50_miptree);
   struct pipe_resource *pt = &mt->base.base;
   int ret;
   union nouveau_bo_config bo_config;
   uint32_t bo_flags;

   if (!mt)
      return NULL;

   mt->base.vtbl = &nvc0_miptree_vtbl;
   *pt = *templ;
   pipe_reference_init(&pt->reference, 1);
   pt->screen = pscreen;

   bo_config.nvc0.memtype = nvc0_mt_choose_storage_type(mt, TRUE);

   if (!nvc0_miptree_init_ms_mode(mt)) {
      FREE(mt);
      return NULL;
   }

   if (unlikely(pt->flags & NVC0_RESOURCE_FLAG_VIDEO)) {
      nvc0_miptree_init_layout_video(mt);
   } else
   if (likely(bo_config.nvc0.memtype)) {
      nvc0_miptree_init_layout_tiled(mt);
   } else
   if (!nv50_miptree_init_layout_linear(mt)) {
      FREE(mt);
      return NULL;
   }
   bo_config.nvc0.tile_mode = mt->level[0].tile_mode;

   mt->base.domain = NOUVEAU_BO_VRAM;

   bo_flags = mt->base.domain | NOUVEAU_BO_NOSNOOP;

   if (mt->base.base.bind & (PIPE_BIND_CURSOR | PIPE_BIND_DISPLAY_TARGET))
      bo_flags |= NOUVEAU_BO_CONTIG;

   ret = nouveau_bo_new(dev, bo_flags, 4096, mt->total_size, &bo_config,
                        &mt->base.bo);
   if (ret) {
      FREE(mt);
      return NULL;
   }
   mt->base.address = mt->base.bo->offset;

   return pt;
}

/* Offset of zslice @z from start of level @l. */
INLINE unsigned
nvc0_mt_zslice_offset(const struct nv50_miptree *mt, unsigned l, unsigned z)
{
   const struct pipe_resource *pt = &mt->base.base;

   unsigned tds = NVC0_TILE_SHIFT_Z(mt->level[l].tile_mode);
   unsigned ths = NVC0_TILE_SHIFT_Y(mt->level[l].tile_mode);

   unsigned nby = util_format_get_nblocksy(pt->format,
                                           u_minify(pt->height0, l));

   /* to next 2D tile slice within a 3D tile */
   unsigned stride_2d = NVC0_TILE_SIZE_2D(mt->level[l].tile_mode);

   /* to slice in the next (in z direction) 3D tile */
   unsigned stride_3d = (align(nby, (1 << ths)) * mt->level[l].pitch) << tds;

   return (z & (1 << (tds - 1))) * stride_2d + (z >> tds) * stride_3d;
}

/* Surface functions.
 */

struct pipe_surface *
nvc0_miptree_surface_new(struct pipe_context *pipe,
                         struct pipe_resource *pt,
                         const struct pipe_surface *templ)
{
   struct nv50_surface *ns = nv50_surface_from_miptree(nv50_miptree(pt), templ);
   if (!ns)
      return NULL;
   ns->base.context = pipe;
   return &ns->base;
}
