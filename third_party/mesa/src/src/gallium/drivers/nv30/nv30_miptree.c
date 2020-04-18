/*
 * Copyright 2012 Red Hat Inc.
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
 *
 * Authors: Ben Skeggs
 *
 */

#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_surface.h"

#include "nouveau/nv_m2mf.xml.h"
#include "nv30_screen.h"
#include "nv30_context.h"
#include "nv30_resource.h"
#include "nv30_transfer.h"

static INLINE unsigned
layer_offset(struct pipe_resource *pt, unsigned level, unsigned layer)
{
   struct nv30_miptree *mt = nv30_miptree(pt);
   struct nv30_miptree_level *lvl = &mt->level[level];

   if (pt->target == PIPE_TEXTURE_CUBE)
      return (layer * mt->layer_size) + lvl->offset;

   return lvl->offset + (layer * lvl->zslice_size);
}

static boolean
nv30_miptree_get_handle(struct pipe_screen *pscreen,
                        struct pipe_resource *pt,
                        struct winsys_handle *handle)
{
   struct nv30_miptree *mt = nv30_miptree(pt);
   unsigned stride;

   if (!mt || !mt->base.bo)
      return FALSE;

   stride = mt->level[0].pitch;

   return nouveau_screen_bo_get_handle(pscreen, mt->base.bo, stride, handle);
}

static void
nv30_miptree_destroy(struct pipe_screen *pscreen, struct pipe_resource *pt)
{
   struct nv30_miptree *mt = nv30_miptree(pt);

   nouveau_bo_ref(NULL, &mt->base.bo);
   FREE(mt);
}

struct nv30_transfer {
   struct pipe_transfer base;
   struct nv30_rect img;
   struct nv30_rect tmp;
   unsigned nblocksx;
   unsigned nblocksy;
};

static INLINE struct nv30_transfer *
nv30_transfer(struct pipe_transfer *ptx)
{
   return (struct nv30_transfer *)ptx;
}

static INLINE void
define_rect(struct pipe_resource *pt, unsigned level, unsigned z,
            unsigned x, unsigned y, unsigned w, unsigned h,
            struct nv30_rect *rect)
{
   struct nv30_miptree *mt = nv30_miptree(pt);
   struct nv30_miptree_level *lvl = &mt->level[level];

   rect->w = u_minify(pt->width0, level) << mt->ms_x;
   rect->w = util_format_get_nblocksx(pt->format, rect->w);
   rect->h = u_minify(pt->height0, level) << mt->ms_y;
   rect->h = util_format_get_nblocksy(pt->format, rect->h);
   rect->d = 1;
   rect->z = 0;
   if (mt->swizzled) {
      if (pt->target == PIPE_TEXTURE_3D) {
         rect->d = u_minify(pt->depth0, level);
         rect->z = z; z = 0;
      }
      rect->pitch = 0;
   } else {
      rect->pitch = lvl->pitch;
   }

   rect->bo     = mt->base.bo;
   rect->domain = NOUVEAU_BO_VRAM;
   rect->offset = layer_offset(pt, level, z);
   rect->cpp    = util_format_get_blocksize(pt->format);

   rect->x0     = util_format_get_nblocksx(pt->format, x) << mt->ms_x;
   rect->y0     = util_format_get_nblocksy(pt->format, y) << mt->ms_y;
   rect->x1     = rect->x0 + (w << mt->ms_x);
   rect->y1     = rect->y0 + (h << mt->ms_y);
}

void
nv30_resource_copy_region(struct pipe_context *pipe,
                          struct pipe_resource *dstres, unsigned dst_level,
                          unsigned dstx, unsigned dsty, unsigned dstz,
                          struct pipe_resource *srcres, unsigned src_level,
                          const struct pipe_box *src_box)
{
   struct nv30_context *nv30 = nv30_context(pipe);
   struct nv30_rect src, dst;

   if (dstres->target == PIPE_BUFFER && srcres->target == PIPE_BUFFER) {
      util_resource_copy_region(pipe, dstres, dst_level, dstx, dsty, dstz,
                                      srcres, src_level, src_box);
      return;
   }

   define_rect(srcres, src_level, src_box->z, src_box->x, src_box->y,
                       src_box->width, src_box->height, &src);
   define_rect(dstres, dst_level, dstz, dstx, dsty,
                       src_box->width, src_box->height, &dst);

   nv30_transfer_rect(nv30, NEAREST, &src, &dst);
}

void
nv30_resource_resolve(struct pipe_context *pipe,
                      const struct pipe_resolve_info *info)
{
   struct nv30_context *nv30 = nv30_context(pipe);
   struct nv30_rect src, dst;

   define_rect(info->src.res, 0, 0, info->src.x0, info->src.y0,
               info->src.x1 - info->src.x0, info->src.y1 - info->src.y0, &src);
   define_rect(info->dst.res, info->dst.level, 0, info->dst.x0, info->dst.y0,
               info->dst.x1 - info->dst.x0, info->dst.y1 - info->dst.y0, &dst);

   nv30_transfer_rect(nv30, BILINEAR, &src, &dst);
}

static struct pipe_transfer *
nv30_miptree_transfer_new(struct pipe_context *pipe, struct pipe_resource *pt,
                          unsigned level, unsigned usage,
                          const struct pipe_box *box)
{
   struct nv30_context *nv30 = nv30_context(pipe);
   struct nouveau_device *dev = nv30->screen->base.device;
   struct nv30_transfer *tx;
   int ret;

   tx = CALLOC_STRUCT(nv30_transfer);
   if (!tx)
      return NULL;
   pipe_resource_reference(&tx->base.resource, pt);
   tx->base.level = level;
   tx->base.usage = usage;
   tx->base.box = *box;
   tx->base.stride = util_format_get_nblocksx(pt->format, box->width) *
                     util_format_get_blocksize(pt->format);
   tx->base.layer_stride = util_format_get_nblocksy(pt->format, box->height) *
                           tx->base.stride;

   tx->nblocksx = util_format_get_nblocksx(pt->format, box->width);
   tx->nblocksy = util_format_get_nblocksy(pt->format, box->height);

   define_rect(pt, level, box->z, box->x, box->y,
                   tx->nblocksx, tx->nblocksy, &tx->img);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0,
                        tx->base.layer_stride, NULL, &tx->tmp.bo);
   if (ret) {
      pipe_resource_reference(&tx->base.resource, NULL);
      FREE(tx);
      return NULL;
   }

   tx->tmp.domain = NOUVEAU_BO_GART;
   tx->tmp.offset = 0;
   tx->tmp.pitch  = tx->base.stride;
   tx->tmp.cpp    = tx->img.cpp;
   tx->tmp.w      = tx->nblocksx;
   tx->tmp.h      = tx->nblocksy;
   tx->tmp.d      = 1;
   tx->tmp.x0     = 0;
   tx->tmp.y0     = 0;
   tx->tmp.x1     = tx->tmp.w;
   tx->tmp.y1     = tx->tmp.h;
   tx->tmp.z      = 0;

   if (usage & PIPE_TRANSFER_READ)
      nv30_transfer_rect(nv30, NEAREST, &tx->img, &tx->tmp);

   return &tx->base;
}

static void
nv30_miptree_transfer_del(struct pipe_context *pipe, struct pipe_transfer *ptx)
{
   struct nv30_context *nv30 = nv30_context(pipe);
   struct nv30_transfer *tx = nv30_transfer(ptx);

   if (ptx->usage & PIPE_TRANSFER_WRITE)
      nv30_transfer_rect(nv30, NEAREST, &tx->tmp, &tx->img);

   nouveau_bo_ref(NULL, &tx->tmp.bo);
   pipe_resource_reference(&ptx->resource, NULL);
   FREE(tx);
}

static void *
nv30_miptree_transfer_map(struct pipe_context *pipe, struct pipe_transfer *ptx)
{
   struct nv30_context *nv30 = nv30_context(pipe);
   struct nv30_transfer *tx = nv30_transfer(ptx);
   unsigned access = 0;
   int ret;

   if (tx->tmp.bo->map)
      return tx->tmp.bo->map;

   if (ptx->usage & PIPE_TRANSFER_READ)
      access |= NOUVEAU_BO_RD;
   if (ptx->usage & PIPE_TRANSFER_WRITE)
      access |= NOUVEAU_BO_WR;

   ret = nouveau_bo_map(tx->tmp.bo, access, nv30->base.client);
   if (ret)
      return NULL;
   return tx->tmp.bo->map;
}

static void
nv30_miptree_transfer_unmap(struct pipe_context *pipe,
                            struct pipe_transfer *ptx)
{
}

const struct u_resource_vtbl nv30_miptree_vtbl = {
   nv30_miptree_get_handle,
   nv30_miptree_destroy,
   nv30_miptree_transfer_new,
   nv30_miptree_transfer_del,
   nv30_miptree_transfer_map,
   u_default_transfer_flush_region,
   nv30_miptree_transfer_unmap,
   u_default_transfer_inline_write
};

struct pipe_resource *
nv30_miptree_create(struct pipe_screen *pscreen,
                    const struct pipe_resource *tmpl)
{
   struct nouveau_device *dev = nouveau_screen(pscreen)->device;
   struct nv30_miptree *mt = CALLOC_STRUCT(nv30_miptree);
   struct pipe_resource *pt = &mt->base.base;
   unsigned blocksz, size;
   unsigned w, h, d, l;
   int ret;

   switch (tmpl->nr_samples) {
   case 4:
      mt->ms_mode = 0x00004000;
      mt->ms_x = 1;
      mt->ms_y = 1;
      break;
   case 2:
      mt->ms_mode = 0x00003000;
      mt->ms_x = 1;
      mt->ms_y = 0;
      break;
   default:
      mt->ms_mode = 0x00000000;
      mt->ms_x = 0;
      mt->ms_y = 0;
      break;
   }

   mt->base.vtbl = &nv30_miptree_vtbl;
   *pt = *tmpl;
   pipe_reference_init(&pt->reference, 1);
   pt->screen = pscreen;

   w = pt->width0 << mt->ms_x;
   h = pt->height0 << mt->ms_y;
   d = (pt->target == PIPE_TEXTURE_3D) ? pt->depth0 : 1;
   blocksz = util_format_get_blocksize(pt->format);

   if ((pt->target == PIPE_TEXTURE_RECT) ||
       !util_is_power_of_two(pt->width0) ||
       !util_is_power_of_two(pt->height0) ||
       !util_is_power_of_two(pt->depth0) ||
       util_format_is_compressed(pt->format) ||
       util_format_is_float(pt->format) || mt->ms_mode) {
      mt->uniform_pitch = util_format_get_nblocksx(pt->format, w) * blocksz;
      mt->uniform_pitch = align(mt->uniform_pitch, 64);
   }

   if (!mt->uniform_pitch)
      mt->swizzled = TRUE;

   size = 0;
   for (l = 0; l <= pt->last_level; l++) {
      struct nv30_miptree_level *lvl = &mt->level[l];
      unsigned nbx = util_format_get_nblocksx(pt->format, w);
      unsigned nby = util_format_get_nblocksx(pt->format, h);

      lvl->offset = size;
      lvl->pitch  = mt->uniform_pitch;
      if (!lvl->pitch)
         lvl->pitch = nbx * blocksz;

      lvl->zslice_size = lvl->pitch * nby;
      size += lvl->zslice_size * d;

      w = u_minify(w, 1);
      h = u_minify(h, 1);
      d = u_minify(d, 1);
   }

   mt->layer_size = size;
   if (pt->target == PIPE_TEXTURE_CUBE) {
      if (!mt->uniform_pitch)
         mt->layer_size = align(mt->layer_size, 128);
      size = mt->layer_size * 6;
   }

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 256, size, NULL, &mt->base.bo);
   if (ret) {
      FREE(mt);
      return NULL;
   }

   mt->base.domain = NOUVEAU_BO_VRAM;
   return &mt->base.base;
}

struct pipe_resource *
nv30_miptree_from_handle(struct pipe_screen *pscreen,
                         const struct pipe_resource *tmpl,
                         struct winsys_handle *handle)
{
   struct nv30_miptree *mt;
   unsigned stride;

   /* only supports 2D, non-mipmapped textures for the moment */
   if ((tmpl->target != PIPE_TEXTURE_2D &&
        tmpl->target != PIPE_TEXTURE_RECT) ||
       tmpl->last_level != 0 ||
       tmpl->depth0 != 1 ||
       tmpl->array_size > 1)
      return NULL;

   mt = CALLOC_STRUCT(nv30_miptree);
   if (!mt)
      return NULL;

   mt->base.bo = nouveau_screen_bo_from_handle(pscreen, handle, &stride);
   if (mt->base.bo == NULL) {
      FREE(mt);
      return NULL;
   }

   mt->base.base = *tmpl;
   mt->base.vtbl = &nv30_miptree_vtbl;
   pipe_reference_init(&mt->base.base.reference, 1);
   mt->base.base.screen = pscreen;
   mt->uniform_pitch = stride;
   mt->level[0].pitch = mt->uniform_pitch;
   mt->level[0].offset = 0;

   /* no need to adjust bo reference count */
   return &mt->base.base;
}

struct pipe_surface *
nv30_miptree_surface_new(struct pipe_context *pipe,
                         struct pipe_resource *pt,
                         const struct pipe_surface *tmpl)
{
   struct nv30_miptree *mt = nv30_miptree(pt); /* guaranteed */
   struct nv30_surface *ns;
   struct pipe_surface *ps;
   struct nv30_miptree_level *lvl = &mt->level[tmpl->u.tex.level];

   ns = CALLOC_STRUCT(nv30_surface);
   if (!ns)
      return NULL;
   ps = &ns->base;

   pipe_reference_init(&ps->reference, 1);
   pipe_resource_reference(&ps->texture, pt);
   ps->context = pipe;
   ps->format = tmpl->format;
   ps->usage = tmpl->usage;
   ps->u.tex.level = tmpl->u.tex.level;
   ps->u.tex.first_layer = tmpl->u.tex.first_layer;
   ps->u.tex.last_layer = tmpl->u.tex.last_layer;

   ns->width = u_minify(pt->width0, ps->u.tex.level);
   ns->height = u_minify(pt->height0, ps->u.tex.level);
   ns->depth = ps->u.tex.last_layer - ps->u.tex.first_layer + 1;
   ns->offset = layer_offset(pt, ps->u.tex.level, ps->u.tex.first_layer);
   if (mt->swizzled)
      ns->pitch = 4096; /* random, just something the hw won't reject.. */
   else
      ns->pitch = lvl->pitch;

   /* comment says there are going to be removed, but they're used by the st */
   ps->width = ns->width;
   ps->height = ns->height;
   return ps;
}

void
nv30_miptree_surface_del(struct pipe_context *pipe, struct pipe_surface *ps)
{
   struct nv30_surface *ns = nv30_surface(ps);

   pipe_resource_reference(&ps->texture, NULL);
   FREE(ns);
}
