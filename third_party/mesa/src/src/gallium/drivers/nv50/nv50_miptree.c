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

#include "nv50_context.h"
#include "nv50_resource.h"

static INLINE uint32_t
nv50_tex_choose_tile_dims(unsigned nx, unsigned ny, unsigned nz)
{
   return nvc0_tex_choose_tile_dims(nx, ny * 2, nz);
}

static uint32_t
nv50_mt_choose_storage_type(struct nv50_miptree *mt, boolean compressed)
{
   const unsigned ms = mt->ms_x + mt->ms_y;

   uint32_t tile_flags;

   if (unlikely(mt->base.base.flags & NOUVEAU_RESOURCE_FLAG_LINEAR))
      return 0;
   if (unlikely(mt->base.base.bind & PIPE_BIND_CURSOR))
      return 0;

   switch (mt->base.base.format) {
   case PIPE_FORMAT_Z16_UNORM:
      tile_flags = 0x6c + ms;
      break;
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      tile_flags = 0x18 + ms;
      break;
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      tile_flags = 0x128 + ms;
      break;
   case PIPE_FORMAT_Z32_FLOAT:
      tile_flags = 0x40 + ms;
      break;
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      tile_flags = 0x60 + ms;
      break;
   default:
      switch (util_format_get_blocksizebits(mt->base.base.format)) {
      case 128:
         assert(ms < 3);
         tile_flags = 0x74;
         break;
      case 64:
         switch (ms) {
         case 2: tile_flags = 0xfc; break;
         case 3: tile_flags = 0xfd; break;
         default:
            tile_flags = 0x70;
            break;
         }
         break;
      case 32:
         if (mt->base.base.bind & PIPE_BIND_SCANOUT) {
            assert(ms == 0);
            tile_flags = 0x7a;
         } else {
            switch (ms) {
            case 2: tile_flags = 0xf8; break;
            case 3: tile_flags = 0xf9; break;
            default:
               tile_flags = 0x70;
               break;
            }
         }
         break;
      case 16:
      case 8:
         tile_flags = 0x70;
         break;
      default:
         return 0;
      }
      if (mt->base.base.bind & PIPE_BIND_CURSOR)
         tile_flags = 0;
   }

   if (!compressed)
      tile_flags &= ~0x180;

   return tile_flags;
}

void
nv50_miptree_destroy(struct pipe_screen *pscreen, struct pipe_resource *pt)
{
   struct nv50_miptree *mt = nv50_miptree(pt);

   nouveau_bo_ref(NULL, &mt->base.bo);

   nouveau_fence_ref(NULL, &mt->base.fence);
   nouveau_fence_ref(NULL, &mt->base.fence_wr);

   FREE(mt);
}

boolean
nv50_miptree_get_handle(struct pipe_screen *pscreen,
                        struct pipe_resource *pt,
                        struct winsys_handle *whandle)
{
   struct nv50_miptree *mt = nv50_miptree(pt);
   unsigned stride;

   if (!mt || !mt->base.bo)
      return FALSE;

   stride = mt->level[0].pitch;

   return nouveau_screen_bo_get_handle(pscreen,
                                       mt->base.bo,
                                       stride,
                                       whandle);
}

const struct u_resource_vtbl nv50_miptree_vtbl =
{
   nv50_miptree_get_handle,         /* get_handle */
   nv50_miptree_destroy,            /* resource_destroy */
   nv50_miptree_transfer_new,       /* get_transfer */
   nv50_miptree_transfer_del,       /* transfer_destroy */
   nv50_miptree_transfer_map,       /* transfer_map */
   u_default_transfer_flush_region, /* transfer_flush_region */
   nv50_miptree_transfer_unmap,     /* transfer_unmap */
   u_default_transfer_inline_write  /* transfer_inline_write */
};

static INLINE boolean
nv50_miptree_init_ms_mode(struct nv50_miptree *mt)
{
   switch (mt->base.base.nr_samples) {
   case 8:
      mt->ms_mode = NV50_3D_MULTISAMPLE_MODE_MS8;
      mt->ms_x = 2;
      mt->ms_y = 1;
      break;
   case 4:
      mt->ms_mode = NV50_3D_MULTISAMPLE_MODE_MS4;
      mt->ms_x = 1;
      mt->ms_y = 1;
      break;
   case 2:
      mt->ms_mode = NV50_3D_MULTISAMPLE_MODE_MS2;
      mt->ms_x = 1;
      break;
   case 1:
   case 0:
      mt->ms_mode = NV50_3D_MULTISAMPLE_MODE_MS1;
      break;
   default:
      NOUVEAU_ERR("invalid nr_samples: %u\n", mt->base.base.nr_samples);
      return FALSE;
   }
   return TRUE;
}

boolean
nv50_miptree_init_layout_linear(struct nv50_miptree *mt)
{
   struct pipe_resource *pt = &mt->base.base;
   const unsigned blocksize = util_format_get_blocksize(pt->format);

   if (util_format_is_depth_or_stencil(pt->format))
      return FALSE;

   if ((pt->last_level > 0) || (pt->depth0 > 1) || (pt->array_size > 1))
      return FALSE;
   if (mt->ms_x | mt->ms_y)
      return FALSE;

   mt->level[0].pitch = align(pt->width0 * blocksize, 64);

   mt->total_size = mt->level[0].pitch * pt->height0;

   return TRUE;
}

static void
nv50_miptree_init_layout_tiled(struct nv50_miptree *mt)
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

      lvl->tile_mode = nv50_tex_choose_tile_dims(nbx, nby, d);

      tsx = NV50_TILE_SIZE_X(lvl->tile_mode); /* x is tile row pitch in bytes */
      tsy = NV50_TILE_SIZE_Y(lvl->tile_mode);
      tsz = NV50_TILE_SIZE_Z(lvl->tile_mode);

      lvl->pitch = align(nbx * blocksize, tsx);

      mt->total_size += lvl->pitch * align(nby, tsy) * align(d, tsz);

      w = u_minify(w, 1);
      h = u_minify(h, 1);
      d = u_minify(d, 1);
   }

   if (pt->array_size > 1) {
      mt->layer_stride = align(mt->total_size,
                               NV50_TILE_SIZE(mt->level[0].tile_mode));
      mt->total_size = mt->layer_stride * pt->array_size;
   }
}

struct pipe_resource *
nv50_miptree_create(struct pipe_screen *pscreen,
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

   mt->base.vtbl = &nv50_miptree_vtbl;
   *pt = *templ;
   pipe_reference_init(&pt->reference, 1);
   pt->screen = pscreen;

   bo_config.nv50.memtype = nv50_mt_choose_storage_type(mt, TRUE);

   if (!nv50_miptree_init_ms_mode(mt)) {
      FREE(mt);
      return NULL;
   }

   if (bo_config.nv50.memtype != 0) {
      nv50_miptree_init_layout_tiled(mt);
   } else
   if (!nv50_miptree_init_layout_linear(mt)) {
      FREE(mt);
      return NULL;
   }
   bo_config.nv50.tile_mode = mt->level[0].tile_mode;

   bo_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_NOSNOOP;
   if (mt->base.base.bind & (PIPE_BIND_CURSOR | PIPE_BIND_DISPLAY_TARGET))
      bo_flags |= NOUVEAU_BO_CONTIG;

   ret = nouveau_bo_new(dev, bo_flags, 4096, mt->total_size, &bo_config,
                        &mt->base.bo);
   if (ret) {
      FREE(mt);
      return NULL;
   }
   mt->base.domain = NOUVEAU_BO_VRAM;
   mt->base.address = mt->base.bo->offset;

   return pt;
}

struct pipe_resource *
nv50_miptree_from_handle(struct pipe_screen *pscreen,
                         const struct pipe_resource *templ,
                         struct winsys_handle *whandle)
{
   struct nv50_miptree *mt;
   unsigned stride;

   /* only supports 2D, non-mipmapped textures for the moment */
   if ((templ->target != PIPE_TEXTURE_2D &&
        templ->target != PIPE_TEXTURE_RECT) ||
       templ->last_level != 0 ||
       templ->depth0 != 1 ||
       templ->array_size > 1)
      return NULL;

   mt = CALLOC_STRUCT(nv50_miptree);
   if (!mt)
      return NULL;

   mt->base.bo = nouveau_screen_bo_from_handle(pscreen, whandle, &stride);
   if (mt->base.bo == NULL) {
      FREE(mt);
      return NULL;
   }
   mt->base.domain = NOUVEAU_BO_VRAM;
   mt->base.address = mt->base.bo->offset;

   mt->base.base = *templ;
   mt->base.vtbl = &nv50_miptree_vtbl;
   pipe_reference_init(&mt->base.base.reference, 1);
   mt->base.base.screen = pscreen;
   mt->level[0].pitch = stride;
   mt->level[0].offset = 0;
   mt->level[0].tile_mode = mt->base.bo->config.nv50.tile_mode;

   /* no need to adjust bo reference count */
   return &mt->base.base;
}


/* Offset of zslice @z from start of level @l. */
INLINE unsigned
nv50_mt_zslice_offset(const struct nv50_miptree *mt, unsigned l, unsigned z)
{
   const struct pipe_resource *pt = &mt->base.base;

   unsigned tds = NV50_TILE_SHIFT_Z(mt->level[l].tile_mode);
   unsigned ths = NV50_TILE_SHIFT_Y(mt->level[l].tile_mode);

   unsigned nby = util_format_get_nblocksy(pt->format,
                                           u_minify(pt->height0, l));

   /* to next 2D tile slice within a 3D tile */
   unsigned stride_2d = NV50_TILE_SIZE_2D(mt->level[l].tile_mode);

   /* to slice in the next (in z direction) 3D tile */
   unsigned stride_3d = (align(nby, (1 << ths)) * mt->level[l].pitch) << tds;

   return (z & ((1 << tds) - 1)) * stride_2d + (z >> tds) * stride_3d;
}

/* Surface functions.
 */

struct nv50_surface *
nv50_surface_from_miptree(struct nv50_miptree *mt,
                          const struct pipe_surface *templ)
{
   struct pipe_surface *ps;
   struct nv50_surface *ns = CALLOC_STRUCT(nv50_surface);
   if (!ns)
      return NULL;
   ps = &ns->base;

   pipe_reference_init(&ps->reference, 1);
   pipe_resource_reference(&ps->texture, &mt->base.base);

   ps->format = templ->format;
   ps->usage = templ->usage;
   ps->u.tex.level = templ->u.tex.level;
   ps->u.tex.first_layer = templ->u.tex.first_layer;
   ps->u.tex.last_layer = templ->u.tex.last_layer;

   ns->width = u_minify(mt->base.base.width0, ps->u.tex.level);
   ns->height = u_minify(mt->base.base.height0, ps->u.tex.level);
   ns->depth = ps->u.tex.last_layer - ps->u.tex.first_layer + 1;
   ns->offset = mt->level[templ->u.tex.level].offset;

   /* comment says there are going to be removed, but they're used by the st */
   ps->width = ns->width;
   ps->height = ns->height;

   ns->width <<= mt->ms_x;
   ns->height <<= mt->ms_y;

   return ns;
}

struct pipe_surface *
nv50_miptree_surface_new(struct pipe_context *pipe,
                         struct pipe_resource *pt,
                         const struct pipe_surface *templ)
{
   struct nv50_miptree *mt = nv50_miptree(pt);
   struct nv50_surface *ns = nv50_surface_from_miptree(mt, templ);
   if (!ns)
      return NULL;
   ns->base.context = pipe;

   if (ns->base.u.tex.first_layer) {
      const unsigned l = ns->base.u.tex.level;
      const unsigned z = ns->base.u.tex.first_layer;

      if (mt->layout_3d) {
         ns->offset += nv50_mt_zslice_offset(mt, l, z);

         /* TODO: switch to depth 1 tiles; but actually this shouldn't happen */
         if (ns->depth > 1 &&
             (z & (NV50_TILE_SIZE_Z(mt->level[l].tile_mode) - 1)))
            NOUVEAU_ERR("Creating unsupported 3D surface !\n");
      } else {
         ns->offset += mt->layer_stride * z;
      }
   }

   return &ns->base;
}
