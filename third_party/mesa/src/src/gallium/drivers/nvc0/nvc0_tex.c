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

#include "nvc0_context.h"
#include "nvc0_resource.h"
#include "nv50/nv50_texture.xml.h"

#include "util/u_format.h"

#define NVE4_TIC_ENTRY_INVALID 0x000fffff
#define NVE4_TSC_ENTRY_INVALID 0xfff00000

#define NV50_TIC_0_SWIZZLE__MASK                      \
   (NV50_TIC_0_MAPA__MASK | NV50_TIC_0_MAPB__MASK |   \
    NV50_TIC_0_MAPG__MASK | NV50_TIC_0_MAPR__MASK)

static INLINE uint32_t
nv50_tic_swizzle(uint32_t tc, unsigned swz, boolean tex_int)
{
   switch (swz) {
   case PIPE_SWIZZLE_RED:
      return (tc & NV50_TIC_0_MAPR__MASK) >> NV50_TIC_0_MAPR__SHIFT;
   case PIPE_SWIZZLE_GREEN:
      return (tc & NV50_TIC_0_MAPG__MASK) >> NV50_TIC_0_MAPG__SHIFT;
   case PIPE_SWIZZLE_BLUE:
      return (tc & NV50_TIC_0_MAPB__MASK) >> NV50_TIC_0_MAPB__SHIFT;
   case PIPE_SWIZZLE_ALPHA:
      return (tc & NV50_TIC_0_MAPA__MASK) >> NV50_TIC_0_MAPA__SHIFT;
   case PIPE_SWIZZLE_ONE:
      return tex_int ? NV50_TIC_MAP_ONE_INT : NV50_TIC_MAP_ONE_FLOAT;
   case PIPE_SWIZZLE_ZERO:
   default:
      return NV50_TIC_MAP_ZERO;
   }
}

struct pipe_sampler_view *
nvc0_create_sampler_view(struct pipe_context *pipe,
                         struct pipe_resource *texture,
                         const struct pipe_sampler_view *templ)
{
   const struct util_format_description *desc;
   uint64_t address;
   uint32_t *tic;
   uint32_t swz[4];
   uint32_t depth;
   struct nv50_tic_entry *view;
   struct nv50_miptree *mt;
   boolean tex_int;

   view = MALLOC_STRUCT(nv50_tic_entry);
   if (!view)
      return NULL;
   mt = nv50_miptree(texture);

   view->pipe = *templ;
   view->pipe.reference.count = 1;
   view->pipe.texture = NULL;
   view->pipe.context = pipe;

   view->id = -1;

   pipe_resource_reference(&view->pipe.texture, texture);

   tic = &view->tic[0];

   desc = util_format_description(view->pipe.format);

   tic[0] = nvc0_format_table[view->pipe.format].tic;

   tex_int = util_format_is_pure_integer(view->pipe.format);

   swz[0] = nv50_tic_swizzle(tic[0], view->pipe.swizzle_r, tex_int);
   swz[1] = nv50_tic_swizzle(tic[0], view->pipe.swizzle_g, tex_int);
   swz[2] = nv50_tic_swizzle(tic[0], view->pipe.swizzle_b, tex_int);
   swz[3] = nv50_tic_swizzle(tic[0], view->pipe.swizzle_a, tex_int);
   tic[0] = (tic[0] & ~NV50_TIC_0_SWIZZLE__MASK) |
      (swz[0] << NV50_TIC_0_MAPR__SHIFT) |
      (swz[1] << NV50_TIC_0_MAPG__SHIFT) |
      (swz[2] << NV50_TIC_0_MAPB__SHIFT) |
      (swz[3] << NV50_TIC_0_MAPA__SHIFT);

   address = mt->base.address;

   tic[2] = 0x10001000 | NV50_TIC_2_NO_BORDER;

   if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
      tic[2] |= NV50_TIC_2_COLORSPACE_SRGB;

   /* check for linear storage type */
   if (unlikely(!nouveau_bo_memtype(nv04_resource(texture)->bo))) {
      if (texture->target == PIPE_BUFFER) {
         address +=
            view->pipe.u.buf.first_element * desc->block.bits / 8;
         tic[2] |= NV50_TIC_2_LINEAR | NV50_TIC_2_TARGET_BUFFER;
         tic[3] = 0;
         tic[4] = /* width */
            view->pipe.u.buf.last_element - view->pipe.u.buf.first_element + 1;
         tic[5] = 0;
      } else {
         /* must be 2D texture without mip maps */
         tic[2] |= NV50_TIC_2_LINEAR | NV50_TIC_2_TARGET_RECT;
         if (texture->target != PIPE_TEXTURE_RECT)
            tic[2] |= NV50_TIC_2_NORMALIZED_COORDS;
         tic[3] = mt->level[0].pitch;
         tic[4] = mt->base.base.width0;
         tic[5] = (1 << 16) | mt->base.base.height0;
      }
      tic[6] =
      tic[7] = 0;
      tic[1] = address;
      tic[2] |= address >> 32;
      return &view->pipe;
   }

   if (mt->base.base.target != PIPE_TEXTURE_RECT)
      tic[2] |= NV50_TIC_2_NORMALIZED_COORDS;

   tic[2] |=
      ((mt->level[0].tile_mode & 0x0f0) << (22 - 4)) |
      ((mt->level[0].tile_mode & 0xf00) << (25 - 8));

   depth = MAX2(mt->base.base.array_size, mt->base.base.depth0);

   if (mt->base.base.array_size > 1) {
      /* there doesn't seem to be a base layer field in TIC */
      address += view->pipe.u.tex.first_layer * mt->layer_stride;
      depth = view->pipe.u.tex.last_layer - view->pipe.u.tex.first_layer + 1;
   }
   tic[1] = address;
   tic[2] |= address >> 32;

   switch (mt->base.base.target) {
   case PIPE_TEXTURE_1D:
      tic[2] |= NV50_TIC_2_TARGET_1D;
      break;
/* case PIPE_TEXTURE_2D_MS: */
   case PIPE_TEXTURE_2D:
      tic[2] |= NV50_TIC_2_TARGET_2D;
      break;
   case PIPE_TEXTURE_RECT:
      tic[2] |= NV50_TIC_2_TARGET_RECT;
      break;
   case PIPE_TEXTURE_3D:
      tic[2] |= NV50_TIC_2_TARGET_3D;
      break;
   case PIPE_TEXTURE_CUBE:
      depth /= 6;
      if (depth > 1)
         tic[2] |= NV50_TIC_2_TARGET_CUBE_ARRAY;
      else
         tic[2] |= NV50_TIC_2_TARGET_CUBE;
      break;
   case PIPE_TEXTURE_1D_ARRAY:
      tic[2] |= NV50_TIC_2_TARGET_1D_ARRAY;
      break;
/* case PIPE_TEXTURE_2D_ARRAY_MS: */
   case PIPE_TEXTURE_2D_ARRAY:
      tic[2] |= NV50_TIC_2_TARGET_2D_ARRAY;
      break;
   default:
      NOUVEAU_ERR("invalid texture target: %d\n", mt->base.base.target);
      return FALSE;
   }

   if (mt->base.base.target == PIPE_BUFFER)
      tic[3] = mt->base.base.width0;
   else
      tic[3] = 0x00300000;

   tic[4] = (1 << 31) | (mt->base.base.width0 << mt->ms_x);

   tic[5] = (mt->base.base.height0 << mt->ms_y) & 0xffff;
   tic[5] |= depth << 16;
   tic[5] |= mt->base.base.last_level << 28;

   tic[6] = (mt->ms_x > 1) ? 0x88000000 : 0x03000000; /* sampling points */

   tic[7] = (view->pipe.u.tex.last_level << 4) | view->pipe.u.tex.first_level;

   /*
   if (mt->base.base.target == PIPE_TEXTURE_2D_MS ||
       mt->base.base.target == PIPE_TEXTURE_2D_ARRAY_MS)
      tic[7] |= mt->ms_mode << 12;
   */

   return &view->pipe;
}

static boolean
nvc0_validate_tic(struct nvc0_context *nvc0, int s)
{
   uint32_t commands[32];
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nouveau_bo *txc = nvc0->screen->txc;
   unsigned i;
   unsigned n = 0;
   boolean need_flush = FALSE;

   for (i = 0; i < nvc0->num_textures[s]; ++i) {
      struct nv50_tic_entry *tic = nv50_tic_entry(nvc0->textures[s][i]);
      struct nv04_resource *res;
      const boolean dirty = !!(nvc0->textures_dirty[s] & (1 << i));

      if (!tic) {
         if (dirty)
            commands[n++] = (i << 1) | 0;
         continue;
      }
      res = nv04_resource(tic->pipe.texture);

      if (tic->id < 0) {
         tic->id = nvc0_screen_tic_alloc(nvc0->screen, tic);

         PUSH_SPACE(push, 17);
         BEGIN_NVC0(push, NVC0_M2MF(OFFSET_OUT_HIGH), 2);
         PUSH_DATAh(push, txc->offset + (tic->id * 32));
         PUSH_DATA (push, txc->offset + (tic->id * 32));
         BEGIN_NVC0(push, NVC0_M2MF(LINE_LENGTH_IN), 2);
         PUSH_DATA (push, 32);
         PUSH_DATA (push, 1);
         BEGIN_NVC0(push, NVC0_M2MF(EXEC), 1);
         PUSH_DATA (push, 0x100111);
         BEGIN_NIC0(push, NVC0_M2MF(DATA), 8);
         PUSH_DATAp(push, &tic->tic[0], 8);

         need_flush = TRUE;
      } else
      if (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
         BEGIN_NVC0(push, NVC0_3D(TEX_CACHE_CTL), 1);
         PUSH_DATA (push, (tic->id << 4) | 1);
      }
      nvc0->screen->tic.lock[tic->id / 32] |= 1 << (tic->id % 32);

      res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;
      res->status |=  NOUVEAU_BUFFER_STATUS_GPU_READING;

      if (!dirty)
         continue;
      commands[n++] = (tic->id << 9) | (i << 1) | 1;

      BCTX_REFN(nvc0->bufctx_3d, TEX(s, i), res, RD);
   }
   for (; i < nvc0->state.num_textures[s]; ++i)
      commands[n++] = (i << 1) | 0;

   nvc0->state.num_textures[s] = nvc0->num_textures[s];

   if (n) {
      BEGIN_NIC0(push, NVC0_3D(BIND_TIC(s)), n);
      PUSH_DATAp(push, commands, n);
   }
   nvc0->textures_dirty[s] = 0;

   return need_flush;
}

static boolean
nve4_validate_tic(struct nvc0_context *nvc0, unsigned s)
{
   struct nouveau_bo *txc = nvc0->screen->txc;
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   unsigned i;
   boolean need_flush = FALSE;

   for (i = 0; i < nvc0->num_textures[s]; ++i) {
      struct nv50_tic_entry *tic = nv50_tic_entry(nvc0->textures[s][i]);
      struct nv04_resource *res;
      const boolean dirty = !!(nvc0->textures_dirty[s] & (1 << i));

      if (!tic) {
         nvc0->tex_handles[s][i] |= NVE4_TIC_ENTRY_INVALID;
         continue;
      }
      res = nv04_resource(tic->pipe.texture);

      if (tic->id < 0) {
         tic->id = nvc0_screen_tic_alloc(nvc0->screen, tic);

         PUSH_SPACE(push, 16);
         BEGIN_NVC0(push, NVE4_P2MF(DST_ADDRESS_HIGH), 2);
         PUSH_DATAh(push, txc->offset + (tic->id * 32));
         PUSH_DATA (push, txc->offset + (tic->id * 32));
         BEGIN_NVC0(push, NVE4_P2MF(LINE_LENGTH_IN), 2);
         PUSH_DATA (push, 32);
         PUSH_DATA (push, 1);
         BEGIN_1IC0(push, NVE4_P2MF(EXEC), 9);
         PUSH_DATA (push, 0x1001);
         PUSH_DATAp(push, &tic->tic[0], 8);

         need_flush = TRUE;
      } else
      if (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
         BEGIN_NVC0(push, NVC0_3D(TEX_CACHE_CTL), 1);
         PUSH_DATA (push, (tic->id << 4) | 1);
      }
      nvc0->screen->tic.lock[tic->id / 32] |= 1 << (tic->id % 32);

      res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;
      res->status |=  NOUVEAU_BUFFER_STATUS_GPU_READING;

      nvc0->tex_handles[s][i] &= ~NVE4_TIC_ENTRY_INVALID;
      nvc0->tex_handles[s][i] |= tic->id;
      if (dirty)
         BCTX_REFN(nvc0->bufctx_3d, TEX(s, i), res, RD);
   }
   for (; i < nvc0->state.num_textures[s]; ++i)
      nvc0->tex_handles[s][i] |= NVE4_TIC_ENTRY_INVALID;

   nvc0->state.num_textures[s] = nvc0->num_textures[s];

   return need_flush;
}

void nvc0_validate_textures(struct nvc0_context *nvc0)
{
   boolean need_flush;

   if (nvc0->screen->base.class_3d >= NVE4_3D_CLASS) {
      need_flush  = nve4_validate_tic(nvc0, 0);
      need_flush |= nve4_validate_tic(nvc0, 3);
      need_flush |= nve4_validate_tic(nvc0, 4);
   } else {
      need_flush  = nvc0_validate_tic(nvc0, 0);
      need_flush |= nvc0_validate_tic(nvc0, 3);
      need_flush |= nvc0_validate_tic(nvc0, 4);
   }

   if (need_flush) {
      BEGIN_NVC0(nvc0->base.pushbuf, NVC0_3D(TIC_FLUSH), 1);
      PUSH_DATA (nvc0->base.pushbuf, 0);
   }
}

static boolean
nvc0_validate_tsc(struct nvc0_context *nvc0, int s)
{
   uint32_t commands[16];
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   unsigned i;
   unsigned n = 0;
   boolean need_flush = FALSE;

   for (i = 0; i < nvc0->num_samplers[s]; ++i) {
      struct nv50_tsc_entry *tsc = nv50_tsc_entry(nvc0->samplers[s][i]);

      if (!(nvc0->samplers_dirty[s] & (1 << i)))
         continue;
      if (!tsc) {
         commands[n++] = (i << 4) | 0;
         continue;
      }
      if (tsc->id < 0) {
         tsc->id = nvc0_screen_tsc_alloc(nvc0->screen, tsc);

         nvc0_m2mf_push_linear(&nvc0->base, nvc0->screen->txc,
                               65536 + tsc->id * 32, NOUVEAU_BO_VRAM,
                               32, tsc->tsc);
         need_flush = TRUE;
      }
      nvc0->screen->tsc.lock[tsc->id / 32] |= 1 << (tsc->id % 32);

      commands[n++] = (tsc->id << 12) | (i << 4) | 1;
   }
   for (; i < nvc0->state.num_samplers[s]; ++i)
      commands[n++] = (i << 4) | 0;

   nvc0->state.num_samplers[s] = nvc0->num_samplers[s];

   if (n) {
      BEGIN_NIC0(push, NVC0_3D(BIND_TSC(s)), n);
      PUSH_DATAp(push, commands, n);
   }
   nvc0->samplers_dirty[s] = 0;

   return need_flush;
}

static boolean
nve4_validate_tsc(struct nvc0_context *nvc0, int s)
{
   struct nouveau_bo *txc = nvc0->screen->txc;
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   unsigned i;
   boolean need_flush = FALSE;

   for (i = 0; i < nvc0->num_samplers[s]; ++i) {
      struct nv50_tsc_entry *tsc = nv50_tsc_entry(nvc0->samplers[s][i]);

      if (!tsc) {
         nvc0->tex_handles[s][i] |= NVE4_TSC_ENTRY_INVALID;
         continue;
      }
      if (tsc->id < 0) {
         tsc->id = nvc0_screen_tsc_alloc(nvc0->screen, tsc);

         PUSH_SPACE(push, 16);
         BEGIN_NVC0(push, NVE4_P2MF(DST_ADDRESS_HIGH), 2);
         PUSH_DATAh(push, txc->offset + 65536 + (tsc->id * 32));
         PUSH_DATA (push, txc->offset + 65536 + (tsc->id * 32));
         BEGIN_NVC0(push, NVE4_P2MF(LINE_LENGTH_IN), 2);
         PUSH_DATA (push, 32);
         PUSH_DATA (push, 1);
         BEGIN_1IC0(push, NVE4_P2MF(EXEC), 9);
         PUSH_DATA (push, 0x1001);
         PUSH_DATAp(push, &tsc->tsc[0], 8);

         need_flush = TRUE;
      }
      nvc0->screen->tsc.lock[tsc->id / 32] |= 1 << (tsc->id % 32);

      nvc0->tex_handles[s][i] &= ~NVE4_TSC_ENTRY_INVALID;
      nvc0->tex_handles[s][i] |= tsc->id << 20;
   }
   for (; i < nvc0->state.num_samplers[s]; ++i)
      nvc0->tex_handles[s][i] |= NVE4_TSC_ENTRY_INVALID;

   nvc0->state.num_samplers[s] = nvc0->num_samplers[s];

   return need_flush;
}

void nvc0_validate_samplers(struct nvc0_context *nvc0)
{
   boolean need_flush;

   if (nvc0->screen->base.class_3d >= NVE4_3D_CLASS) {
      need_flush  = nve4_validate_tsc(nvc0, 0);
      need_flush |= nve4_validate_tsc(nvc0, 3);
      need_flush |= nve4_validate_tsc(nvc0, 4);
   } else {
      need_flush  = nvc0_validate_tsc(nvc0, 0);
      need_flush |= nvc0_validate_tsc(nvc0, 3);
      need_flush |= nvc0_validate_tsc(nvc0, 4);
   }

   if (need_flush) {
      BEGIN_NVC0(nvc0->base.pushbuf, NVC0_3D(TSC_FLUSH), 1);
      PUSH_DATA (nvc0->base.pushbuf, 0);
   }
}

/* Upload the "diagonal" entries for the possible texture sources ($t == $s).
 * At some point we might want to get a list of the combinations used by a
 * shader and fill in those entries instead of having it extract the handles.
 */
void
nve4_set_tex_handles(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   uint64_t address;
   unsigned s;

   if (nvc0->screen->base.class_3d < NVE4_3D_CLASS)
      return;
   address = nvc0->screen->uniform_bo->offset + (5 << 16);

   for (s = 0; s < 5; ++s, address += (1 << 9)) {
      uint32_t dirty = nvc0->textures_dirty[s] | nvc0->samplers_dirty[s];
      if (!dirty)
         continue;
      BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
      PUSH_DATA (push, 512);
      PUSH_DATAh(push, address);
      PUSH_DATA (push, address);
      do {
         int i = ffs(dirty) - 1;
         dirty &= ~(1 << i);

         BEGIN_NVC0(push, NVC0_3D(CB_POS), 2);
         PUSH_DATA (push, (8 + i) * 4);
         PUSH_DATA (push, nvc0->tex_handles[s][i]);
      } while (dirty);

      nvc0->textures_dirty[s] = 0;
      nvc0->samplers_dirty[s] = 0;
   }
}
