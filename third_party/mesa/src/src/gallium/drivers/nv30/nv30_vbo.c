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
#include "translate/translate.h"

#include "nouveau/nouveau_fence.h"
#include "nouveau/nv_object.xml.h"
#include "nv30-40_3d.xml.h"
#include "nv30_context.h"
#include "nv30_format.h"

static void
nv30_emit_vtxattr(struct nv30_context *nv30, struct pipe_vertex_buffer *vb,
                  struct pipe_vertex_element *ve, unsigned attr)
{
   const unsigned nc = util_format_get_nr_components(ve->src_format);
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nv04_resource *res = nv04_resource(vb->buffer);
   const void *data;
   float v[4];

   data = nouveau_resource_map_offset(&nv30->base, res, vb->buffer_offset +
                                      ve->src_offset, NOUVEAU_BO_RD);

   util_format_read_4f(ve->src_format, v, 0, data, 0, 0, 0, 1, 1);

   switch (nc) {
   case 4:
      BEGIN_NV04(push, NV30_3D(VTX_ATTR_4F(attr)), 4);
      PUSH_DATAf(push, v[0]);
      PUSH_DATAf(push, v[1]);
      PUSH_DATAf(push, v[2]);
      PUSH_DATAf(push, v[3]);
      break;
   case 3:
      BEGIN_NV04(push, NV30_3D(VTX_ATTR_3F(attr)), 3);
      PUSH_DATAf(push, v[0]);
      PUSH_DATAf(push, v[1]);
      PUSH_DATAf(push, v[2]);
      break;
   case 2:
      BEGIN_NV04(push, NV30_3D(VTX_ATTR_2F(attr)), 2);
      PUSH_DATAf(push, v[0]);
      PUSH_DATAf(push, v[1]);
      break;
   case 1:
      BEGIN_NV04(push, NV30_3D(VTX_ATTR_1F(attr)), 1);
      PUSH_DATAf(push, v[0]);
      break;
   default:
      assert(0);
      break;
   }
}

static INLINE void
nv30_vbuf_range(struct nv30_context *nv30, int vbi,
                uint32_t *base, uint32_t *size)
{
   assert(nv30->vbo_max_index != ~0);
   *base = nv30->vbo_min_index * nv30->vtxbuf[vbi].stride;
   *size = (nv30->vbo_max_index -
            nv30->vbo_min_index + 1) * nv30->vtxbuf[vbi].stride;
}

static void
nv30_prevalidate_vbufs(struct nv30_context *nv30)
{
   struct pipe_vertex_buffer *vb;
   struct nv04_resource *buf;
   int i;
   uint32_t base, size;

   nv30->vbo_fifo = nv30->vbo_user = 0;

   for (i = 0; i < nv30->num_vtxbufs; i++) {
      vb = &nv30->vtxbuf[i];
      if (!vb->stride || !vb->buffer) /* NOTE: user_buffer not implemented */
         continue;
      buf = nv04_resource(vb->buffer);

      /* NOTE: user buffers with temporary storage count as mapped by GPU */
      if (!nouveau_resource_mapped_by_gpu(vb->buffer)) {
         if (nv30->vbo_push_hint) {
            nv30->vbo_fifo = ~0;
            continue;
         } else {
            if (buf->status & NOUVEAU_BUFFER_STATUS_USER_MEMORY) {
               nv30->vbo_user |= 1 << i;
               assert(vb->stride > vb->buffer_offset);
               nv30_vbuf_range(nv30, i, &base, &size);
               nouveau_user_buffer_upload(&nv30->base, buf, base, size);
            } else {
               nouveau_buffer_migrate(&nv30->base, buf, NOUVEAU_BO_GART);
            }
            nv30->base.vbo_dirty = TRUE;
         }
      }
   }
}

static void
nv30_update_user_vbufs(struct nv30_context *nv30)
{
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
   uint32_t base, offset, size;
   int i;
   uint32_t written = 0;

   for (i = 0; i < nv30->vertex->num_elements; i++) {
      struct pipe_vertex_element *ve = &nv30->vertex->pipe[i];
      const int b = ve->vertex_buffer_index;
      struct pipe_vertex_buffer *vb = &nv30->vtxbuf[b];
      struct nv04_resource *buf = nv04_resource(vb->buffer);

      if (!(nv30->vbo_user & (1 << b)))
         continue;

      if (!vb->stride) {
         nv30_emit_vtxattr(nv30, vb, ve, i);
         continue;
      }
      nv30_vbuf_range(nv30, b, &base, &size);

      if (!(written & (1 << b))) {
         written |= 1 << b;
         nouveau_user_buffer_upload(&nv30->base, buf, base, size);
      }

      offset = vb->buffer_offset + ve->src_offset;

      BEGIN_NV04(push, NV30_3D(VTXBUF(i)), 1);
      PUSH_RESRC(push, NV30_3D(VTXBUF(i)), BUFCTX_VTXTMP, buf, offset,
                       NOUVEAU_BO_LOW | NOUVEAU_BO_RD,
                       0, NV30_3D_VTXBUF_DMA1);
   }
   nv30->base.vbo_dirty = TRUE;
}

static INLINE void
nv30_release_user_vbufs(struct nv30_context *nv30)
{
   uint32_t vbo_user = nv30->vbo_user;

   while (vbo_user) {
      int i = ffs(vbo_user) - 1;
      vbo_user &= ~(1 << i);

      nouveau_buffer_release_gpu_storage(nv04_resource(nv30->vtxbuf[i].buffer));
   }

   nouveau_bufctx_reset(nv30->bufctx, BUFCTX_VTXTMP);
}

void
nv30_vbo_validate(struct nv30_context *nv30)
{
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nv30_vertex_stateobj *vertex = nv30->vertex;
   struct pipe_vertex_element *ve;
   struct pipe_vertex_buffer *vb;
   unsigned i, redefine;

   nouveau_bufctx_reset(nv30->bufctx, BUFCTX_VTXBUF);
   if (!nv30->vertex || nv30->draw_flags)
      return;

   if (unlikely(vertex->need_conversion)) {
      nv30->vbo_fifo = ~0;
      nv30->vbo_user = 0;
   } else {
      nv30_prevalidate_vbufs(nv30);
   }

   if (!PUSH_SPACE(push, 128))
      return;

   redefine = MAX2(vertex->num_elements, nv30->state.num_vtxelts);
   BEGIN_NV04(push, NV30_3D(VTXFMT(0)), redefine);

   for (i = 0; i < vertex->num_elements; i++) {
      ve = &vertex->pipe[i];
      vb = &nv30->vtxbuf[ve->vertex_buffer_index];

      if (likely(vb->stride) || nv30->vbo_fifo)
         PUSH_DATA (push, (vb->stride << 8) | vertex->element[i].state);
      else
         PUSH_DATA (push, NV30_3D_VTXFMT_TYPE_V32_FLOAT);
   }

   for (; i < nv30->state.num_vtxelts; i++) {
      PUSH_DATA (push, NV30_3D_VTXFMT_TYPE_V32_FLOAT);
   }

   for (i = 0; i < vertex->num_elements; i++) {
      struct nv04_resource *res;
      unsigned offset;
      boolean user;

      ve = &vertex->pipe[i];
      vb = &nv30->vtxbuf[ve->vertex_buffer_index];
      user = (nv30->vbo_user & (1 << ve->vertex_buffer_index));

      res = nv04_resource(vb->buffer);

      if (nv30->vbo_fifo || unlikely(vb->stride == 0)) {
         if (!nv30->vbo_fifo)
            nv30_emit_vtxattr(nv30, vb, ve, i);
         continue;
      }

      offset = ve->src_offset + vb->buffer_offset;

      BEGIN_NV04(push, NV30_3D(VTXBUF(i)), 1);
      PUSH_RESRC(push, NV30_3D(VTXBUF(i)), user ? BUFCTX_VTXTMP : BUFCTX_VTXBUF,
                       res, offset, NOUVEAU_BO_LOW | NOUVEAU_BO_RD,
                       0, NV30_3D_VTXBUF_DMA1);
   }

   nv30->state.num_vtxelts = vertex->num_elements;
}

static void *
nv30_vertex_state_create(struct pipe_context *pipe, unsigned num_elements,
                         const struct pipe_vertex_element *elements)
{
    struct nv30_vertex_stateobj *so;
    struct translate_key transkey;
    unsigned i;

    assert(num_elements);

    so = MALLOC(sizeof(*so) + sizeof(*so->element) * num_elements);
    if (!so)
        return NULL;
    memcpy(so->pipe, elements, sizeof(*elements) * num_elements);
    so->num_elements = num_elements;
    so->need_conversion = FALSE;

    transkey.nr_elements = 0;
    transkey.output_stride = 0;

    for (i = 0; i < num_elements; i++) {
        const struct pipe_vertex_element *ve = &elements[i];
        const unsigned vbi = ve->vertex_buffer_index;
        enum pipe_format fmt = ve->src_format;

        so->element[i].state = nv30_vtxfmt(pipe->screen, fmt)->hw;
        if (!so->element[i].state) {
            switch (util_format_get_nr_components(fmt)) {
            case 1: fmt = PIPE_FORMAT_R32_FLOAT; break;
            case 2: fmt = PIPE_FORMAT_R32G32_FLOAT; break;
            case 3: fmt = PIPE_FORMAT_R32G32B32_FLOAT; break;
            case 4: fmt = PIPE_FORMAT_R32G32B32A32_FLOAT; break;
            default:
                assert(0);
                return NULL;
            }
            so->element[i].state = nv30_vtxfmt(pipe->screen, fmt)->hw;
            so->need_conversion = TRUE;
        }

        if (1) {
            unsigned j = transkey.nr_elements++;

            transkey.element[j].type = TRANSLATE_ELEMENT_NORMAL;
            transkey.element[j].input_format = ve->src_format;
            transkey.element[j].input_buffer = vbi;
            transkey.element[j].input_offset = ve->src_offset;
            transkey.element[j].instance_divisor = ve->instance_divisor;

            transkey.element[j].output_format = fmt;
            transkey.element[j].output_offset = transkey.output_stride;
            transkey.output_stride += (util_format_get_stride(fmt, 1) + 3) & ~3;
        }
    }

    so->translate = translate_create(&transkey);
    so->vtx_size = transkey.output_stride / 4;
    so->vtx_per_packet_max = NV04_PFIFO_MAX_PACKET_LEN / MAX2(so->vtx_size, 1);
    return so;
}

static void
nv30_vertex_state_delete(struct pipe_context *pipe, void *hwcso)
{
   struct nv30_vertex_stateobj *so = hwcso;

   if (so->translate)
      so->translate->release(so->translate);
   FREE(hwcso);
}

static void
nv30_vertex_state_bind(struct pipe_context *pipe, void *hwcso)
{
   struct nv30_context *nv30 = nv30_context(pipe);

   nv30->vertex = hwcso;
   nv30->dirty |= NV30_NEW_VERTEX;
}

static void
nv30_draw_arrays(struct nv30_context *nv30,
                 unsigned mode, unsigned start, unsigned count,
                 unsigned instance_count)
{
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
   unsigned prim;

   prim = nv30_prim_gl(mode);

   BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
   PUSH_DATA (push, prim);
   while (count) {
      const unsigned mpush = 2047 * 256;
      unsigned npush  = (count > mpush) ? mpush : count;
      unsigned wpush  = ((npush + 255) & ~255) >> 8;

      count -= npush;

      BEGIN_NI04(push, NV30_3D(VB_VERTEX_BATCH), wpush);
      while (npush >= 256) {
         PUSH_DATA (push, 0xff000000 | start);
         start += 256;
         npush -= 256;
      }

      if (npush)
         PUSH_DATA (push, ((npush - 1) << 24) | start);
   }
   BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
   PUSH_DATA (push, NV30_3D_VERTEX_BEGIN_END_STOP);
}

static void
nv30_draw_elements_inline_u08(struct nouveau_pushbuf *push, const uint8_t *map,
                              unsigned start, unsigned count)
{
   map += start;

   if (count & 1) {
      BEGIN_NV04(push, NV30_3D(VB_ELEMENT_U32), 1);
      PUSH_DATA (push, *map++);
   }

   count >>= 1;
   while (count) {
      unsigned npush = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN);
      count -= npush;

      BEGIN_NI04(push, NV30_3D(VB_ELEMENT_U16), npush);
      while (npush--) {
         PUSH_DATA (push, (map[1] << 16) | map[0]);
         map += 2;
      }
   }

}

static void
nv30_draw_elements_inline_u16(struct nouveau_pushbuf *push, const uint16_t *map,
                              unsigned start, unsigned count)
{
   map += start;

   if (count & 1) {
      BEGIN_NV04(push, NV30_3D(VB_ELEMENT_U32), 1);
      PUSH_DATA (push, *map++);
   }

   count >>= 1;
   while (count) {
      unsigned npush = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN);
      count -= npush;

      BEGIN_NI04(push, NV30_3D(VB_ELEMENT_U16), npush);
      while (npush--) {
         PUSH_DATA (push, (map[1] << 16) | map[0]);
         map += 2;
      }
   }
}

static void
nv30_draw_elements_inline_u32(struct nouveau_pushbuf *push, const uint32_t *map,
                              unsigned start, unsigned count)
{
   map += start;

   while (count) {
      const unsigned nr = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN);

      BEGIN_NI04(push, NV30_3D(VB_ELEMENT_U32), nr);
      PUSH_DATAp(push, map, nr);

      map += nr;
      count -= nr;
   }
}

static void
nv30_draw_elements_inline_u32_short(struct nouveau_pushbuf *push,
                                    const uint32_t *map,
                                    unsigned start, unsigned count)
{
   map += start;

   if (count & 1) {
      BEGIN_NV04(push, NV30_3D(VB_ELEMENT_U32), 1);
      PUSH_DATA (push, *map++);
   }

   count >>= 1;
   while (count) {
      unsigned npush = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN);;
      count -= npush;

      BEGIN_NI04(push, NV30_3D(VB_ELEMENT_U16), npush);
      while (npush--) {
         PUSH_DATA (push, (map[1] << 16) | map[0]);
         map += 2;
      }
   }
}

static void
nv30_draw_elements(struct nv30_context *nv30, boolean shorten,
                   unsigned mode, unsigned start, unsigned count,
                   unsigned instance_count, int32_t index_bias)
{
   const unsigned index_size = nv30->idxbuf.index_size;
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nouveau_object *eng3d = nv30->screen->eng3d;
   unsigned prim = nv30_prim_gl(mode);

#if 0 /*XXX*/
   if (index_bias != nv30->state.index_bias) {
      BEGIN_NV04(push, NV30_3D(VB_ELEMENT_BASE), 1);
      PUSH_DATA (push, index_bias);
      nv30->state.index_bias = index_bias;
   }
#endif

   if (eng3d->oclass == NV40_3D_CLASS && index_size > 1 &&
       nv30->idxbuf.buffer) {
      struct nv04_resource *res = nv04_resource(nv30->idxbuf.buffer);
      unsigned offset = nv30->idxbuf.offset;

      assert(nouveau_resource_mapped_by_gpu(&res->base));

      BEGIN_NV04(push, NV30_3D(IDXBUF_OFFSET), 2);
      PUSH_RESRC(push, NV30_3D(IDXBUF_OFFSET), BUFCTX_IDXBUF, res, offset,
                       NOUVEAU_BO_LOW | NOUVEAU_BO_RD, 0, 0);
      PUSH_MTHD (push, NV30_3D(IDXBUF_FORMAT), BUFCTX_IDXBUF, res->bo,
                       (index_size == 2) ? 0x00000010 : 0x00000000,
                       res->domain | NOUVEAU_BO_RD,
                       0, NV30_3D_IDXBUF_FORMAT_DMA1);
      BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
      PUSH_DATA (push, prim);
      while (count) {
         const unsigned mpush = 2047 * 256;
         unsigned npush  = (count > mpush) ? mpush : count;
         unsigned wpush  = ((npush + 255) & ~255) >> 8;

         count -= npush;

         BEGIN_NI04(push, NV30_3D(VB_INDEX_BATCH), wpush);
         while (npush >= 256) {
            PUSH_DATA (push, 0xff000000 | start);
            start += 256;
            npush -= 256;
         }

         if (npush)
            PUSH_DATA (push, ((npush - 1) << 24) | start);
      }
      BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
      PUSH_DATA (push, NV30_3D_VERTEX_BEGIN_END_STOP);
      PUSH_RESET(push, BUFCTX_IDXBUF);
   } else {
      const void *data;
      if (nv30->idxbuf.buffer)
         data = nouveau_resource_map_offset(&nv30->base,
                                            nv04_resource(nv30->idxbuf.buffer),
                                            nv30->idxbuf.offset, NOUVEAU_BO_RD);
      else
         data = nv30->idxbuf.user_buffer;
      if (!data)
         return;

      BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
      PUSH_DATA (push, prim);
      switch (index_size) {
      case 1:
         nv30_draw_elements_inline_u08(push, data, start, count);
         break;
      case 2:
         nv30_draw_elements_inline_u16(push, data, start, count);
         break;
      case 4:
         if (shorten)
            nv30_draw_elements_inline_u32_short(push, data, start, count);
         else
            nv30_draw_elements_inline_u32(push, data, start, count);
         break;
      default:
         assert(0);
         return;
      }
      BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
      PUSH_DATA (push, NV30_3D_VERTEX_BEGIN_END_STOP);
   }
}

static void
nv30_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
   struct nv30_context *nv30 = nv30_context(pipe);
   struct nouveau_pushbuf *push = nv30->base.pushbuf;

   /* For picking only a few vertices from a large user buffer, push is better,
    * if index count is larger and we expect repeated vertices, suggest upload.
    */
   nv30->vbo_push_hint = /* the 64 is heuristic */
      !(info->indexed &&
        ((info->max_index - info->min_index + 64) < info->count));

   nv30->vbo_min_index = info->min_index;
   nv30->vbo_max_index = info->max_index;

   if (nv30->vbo_push_hint != !!nv30->vbo_fifo)
      nv30->dirty |= NV30_NEW_ARRAYS;

   push->user_priv = &nv30->bufctx;
   if (nv30->vbo_user && !(nv30->dirty & (NV30_NEW_VERTEX | NV30_NEW_ARRAYS)))
      nv30_update_user_vbufs(nv30);

   nv30_state_validate(nv30, TRUE);
   if (nv30->draw_flags) {
      nv30_render_vbo(pipe, info);
      return;
   } else
   if (nv30->vbo_fifo) {
      nv30_push_vbo(nv30, info);
      return;
   }

   if (nv30->base.vbo_dirty) {
      BEGIN_NV04(push, NV30_3D(VTX_CACHE_INVALIDATE_1710), 1);
      PUSH_DATA (push, 0);
      nv30->base.vbo_dirty = FALSE;
   }

   if (!info->indexed) {
      nv30_draw_arrays(nv30,
                       info->mode, info->start, info->count,
                       info->instance_count);
   } else {
      boolean shorten = info->max_index <= 65535;

      if (info->primitive_restart != nv30->state.prim_restart) {
         if (info->primitive_restart) {
            BEGIN_NV04(push, NV40_3D(PRIM_RESTART_ENABLE), 2);
            PUSH_DATA (push, 1);
            PUSH_DATA (push, info->restart_index);

            if (info->restart_index > 65535)
               shorten = FALSE;
         } else {
            BEGIN_NV04(push, NV40_3D(PRIM_RESTART_ENABLE), 1);
            PUSH_DATA (push, 0);
         }
         nv30->state.prim_restart = info->primitive_restart;
      } else
      if (info->primitive_restart) {
         BEGIN_NV04(push, NV40_3D(PRIM_RESTART_INDEX), 1);
         PUSH_DATA (push, info->restart_index);

         if (info->restart_index > 65535)
            shorten = FALSE;
      }

      nv30_draw_elements(nv30, shorten,
                         info->mode, info->start, info->count,
                         info->instance_count, info->index_bias);
   }

   nv30_state_release(nv30);
   nv30_release_user_vbufs(nv30);
}

void
nv30_vbo_init(struct pipe_context *pipe)
{
   pipe->create_vertex_elements_state = nv30_vertex_state_create;
   pipe->delete_vertex_elements_state = nv30_vertex_state_delete;
   pipe->bind_vertex_elements_state = nv30_vertex_state_bind;
   pipe->draw_vbo = nv30_draw_vbo;
}
