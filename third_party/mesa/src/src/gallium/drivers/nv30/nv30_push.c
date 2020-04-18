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

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "translate/translate.h"

#include "nouveau/nv_object.xml.h"
#include "nv30-40_3d.xml.h"
#include "nv30_context.h"
#include "nv30_resource.h"

struct push_context {
   struct nouveau_pushbuf *push;

   const void *idxbuf;

   float edgeflag;
   int edgeflag_attr;

   uint32_t vertex_words;
   uint32_t packet_vertex_limit;

   struct translate *translate;

   boolean primitive_restart;
   uint32_t prim;
   uint32_t restart_index;
};

static INLINE unsigned
prim_restart_search_i08(uint8_t *elts, unsigned push, uint8_t index)
{
   unsigned i;
   for (i = 0; i < push; ++i)
      if (elts[i] == index)
         break;
   return i;
}

static INLINE unsigned
prim_restart_search_i16(uint16_t *elts, unsigned push, uint16_t index)
{
   unsigned i;
   for (i = 0; i < push; ++i)
      if (elts[i] == index)
         break;
   return i;
}

static INLINE unsigned
prim_restart_search_i32(uint32_t *elts, unsigned push, uint32_t index)
{
   unsigned i;
   for (i = 0; i < push; ++i)
      if (elts[i] == index)
         break;
   return i;
}

static void
emit_vertices_i08(struct push_context *ctx, unsigned start, unsigned count)
{
   uint8_t *elts = (uint8_t *)ctx->idxbuf + start;

   while (count) {
      unsigned push = MIN2(count, ctx->packet_vertex_limit);
      unsigned size, nr;

      nr = push;
      if (ctx->primitive_restart)
         nr = prim_restart_search_i08(elts, push, ctx->restart_index);

      size = ctx->vertex_words * nr;

      BEGIN_NI04(ctx->push, NV30_3D(VERTEX_DATA), size);

      ctx->translate->run_elts8(ctx->translate, elts, nr, 0, ctx->push->cur);

      ctx->push->cur += size;
      count -= nr;
      elts += nr;

      if (nr != push) {
         BEGIN_NV04(ctx->push, NV30_3D(VB_ELEMENT_U32), 1);
         PUSH_DATA (ctx->push, ctx->restart_index);
         count--;
         elts++;
      }
   }
}

static void
emit_vertices_i16(struct push_context *ctx, unsigned start, unsigned count)
{
   uint16_t *elts = (uint16_t *)ctx->idxbuf + start;

   while (count) {
      unsigned push = MIN2(count, ctx->packet_vertex_limit);
      unsigned size, nr;

      nr = push;
      if (ctx->primitive_restart)
         nr = prim_restart_search_i16(elts, push, ctx->restart_index);

      size = ctx->vertex_words * nr;

      BEGIN_NI04(ctx->push, NV30_3D(VERTEX_DATA), size);

      ctx->translate->run_elts16(ctx->translate, elts, nr, 0, ctx->push->cur);

      ctx->push->cur += size;
      count -= nr;
      elts += nr;

      if (nr != push) {
         BEGIN_NV04(ctx->push, NV30_3D(VB_ELEMENT_U32), 1);
         PUSH_DATA (ctx->push, ctx->restart_index);
         count--;
         elts++;
      }
   }
}

static void
emit_vertices_i32(struct push_context *ctx, unsigned start, unsigned count)
{
   uint32_t *elts = (uint32_t *)ctx->idxbuf + start;

   while (count) {
      unsigned push = MIN2(count, ctx->packet_vertex_limit);
      unsigned size, nr;

      nr = push;
      if (ctx->primitive_restart)
         nr = prim_restart_search_i32(elts, push, ctx->restart_index);

      size = ctx->vertex_words * nr;

      BEGIN_NI04(ctx->push, NV30_3D(VERTEX_DATA), size);

      ctx->translate->run_elts(ctx->translate, elts, nr, 0, ctx->push->cur);

      ctx->push->cur += size;
      count -= nr;
      elts += nr;

      if (nr != push) {
         BEGIN_NV04(ctx->push, NV30_3D(VB_ELEMENT_U32), 1);
         PUSH_DATA (ctx->push, ctx->restart_index);
         count--;
         elts++;
      }
   }
}

static void
emit_vertices_seq(struct push_context *ctx, unsigned start, unsigned count)
{
   while (count) {
      unsigned push = MIN2(count, ctx->packet_vertex_limit);
      unsigned size = ctx->vertex_words * push;

      BEGIN_NI04(ctx->push, NV30_3D(VERTEX_DATA), size);

      ctx->translate->run(ctx->translate, start, push, 0, ctx->push->cur);
      ctx->push->cur += size;
      count -= push;
      start += push;
   }
}

void
nv30_push_vbo(struct nv30_context *nv30, const struct pipe_draw_info *info)
{
   struct push_context ctx;
   unsigned i, index_size;
   boolean apply_bias = info->indexed && info->index_bias;

   ctx.push = nv30->base.pushbuf;
   ctx.translate = nv30->vertex->translate;
   ctx.packet_vertex_limit = nv30->vertex->vtx_per_packet_max;
   ctx.vertex_words = nv30->vertex->vtx_size;

   for (i = 0; i < nv30->num_vtxbufs; ++i) {
      uint8_t *data;
      struct pipe_vertex_buffer *vb = &nv30->vtxbuf[i];
      struct nv04_resource *res = nv04_resource(vb->buffer);

      data = nouveau_resource_map_offset(&nv30->base, res,
                                         vb->buffer_offset, NOUVEAU_BO_RD);

      if (apply_bias)
         data += info->index_bias * vb->stride;

      ctx.translate->set_buffer(ctx.translate, i, data, vb->stride, ~0);
   }

   if (info->indexed) {
      if (nv30->idxbuf.buffer)
         ctx.idxbuf = nouveau_resource_map_offset(&nv30->base,
            nv04_resource(nv30->idxbuf.buffer), nv30->idxbuf.offset,
            NOUVEAU_BO_RD);
      else
         ctx.idxbuf = nv30->idxbuf.user_buffer;
      if (!ctx.idxbuf) {
         nv30_state_release(nv30);
         return;
      }
      index_size = nv30->idxbuf.index_size;
      ctx.primitive_restart = info->primitive_restart;
      ctx.restart_index = info->restart_index;
   } else {
      ctx.idxbuf = NULL;
      index_size = 0;
      ctx.primitive_restart = FALSE;
      ctx.restart_index = 0;
   }

   if (nv30->screen->eng3d->oclass >= NV40_3D_CLASS) {
      BEGIN_NV04(ctx.push, NV40_3D(PRIM_RESTART_ENABLE), 2);
      PUSH_DATA (ctx.push, info->primitive_restart);
      PUSH_DATA (ctx.push, info->restart_index);
      nv30->state.prim_restart = info->primitive_restart;
   }

   ctx.prim = nv30_prim_gl(info->mode);

   PUSH_RESET(ctx.push, BUFCTX_IDXBUF);
   BEGIN_NV04(ctx.push, NV30_3D(VERTEX_BEGIN_END), 1);
   PUSH_DATA (ctx.push, ctx.prim);
   switch (index_size) {
   case 0:
      emit_vertices_seq(&ctx, info->start, info->count);
      break;
   case 1:
      emit_vertices_i08(&ctx, info->start, info->count);
      break;
   case 2:
      emit_vertices_i16(&ctx, info->start, info->count);
      break;
   case 4:
      emit_vertices_i32(&ctx, info->start, info->count);
      break;
   default:
      assert(0);
      break;
   }
   BEGIN_NV04(ctx.push, NV30_3D(VERTEX_BEGIN_END), 1);
   PUSH_DATA (ctx.push, NV30_3D_VERTEX_BEGIN_END_STOP);

   if (info->indexed)
      nouveau_resource_unmap(nv04_resource(nv30->idxbuf.buffer));

   for (i = 0; i < nv30->num_vtxbufs; ++i)
      nouveau_resource_unmap(nv04_resource(nv30->vtxbuf[i].buffer));

   nv30_state_release(nv30);
}
