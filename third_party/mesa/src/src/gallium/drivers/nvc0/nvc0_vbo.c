/*
 * Copyright 2010 Christoph Bumiller
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

#define NVC0_PUSH_EXPLICIT_SPACE_CHECKING

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "translate/translate.h"

#include "nvc0_context.h"
#include "nvc0_resource.h"

#include "nvc0_3d.xml.h"

void
nvc0_vertex_state_delete(struct pipe_context *pipe,
                         void *hwcso)
{
   struct nvc0_vertex_stateobj *so = hwcso;

   if (so->translate)
      so->translate->release(so->translate);
   FREE(hwcso);
}

void *
nvc0_vertex_state_create(struct pipe_context *pipe,
                         unsigned num_elements,
                         const struct pipe_vertex_element *elements)
{
    struct nvc0_vertex_stateobj *so;
    struct translate_key transkey;
    unsigned i;
    unsigned src_offset_max = 0;

    so = MALLOC(sizeof(*so) +
                num_elements * sizeof(struct nvc0_vertex_element));
    if (!so)
        return NULL;
    so->num_elements = num_elements;
    so->instance_elts = 0;
    so->instance_bufs = 0;
    so->shared_slots = FALSE;
    so->need_conversion = FALSE;

    memset(so->vb_access_size, 0, sizeof(so->vb_access_size));

    for (i = 0; i < PIPE_MAX_ATTRIBS; ++i)
       so->min_instance_div[i] = 0xffffffff;

    transkey.nr_elements = 0;
    transkey.output_stride = 0;

    for (i = 0; i < num_elements; ++i) {
        const struct pipe_vertex_element *ve = &elements[i];
        const unsigned vbi = ve->vertex_buffer_index;
        unsigned size;
        enum pipe_format fmt = ve->src_format;

        so->element[i].pipe = elements[i];
        so->element[i].state = nvc0_format_table[fmt].vtx;

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
            so->element[i].state = nvc0_format_table[fmt].vtx;
            so->need_conversion = TRUE;
        }
        size = util_format_get_blocksize(fmt);

        src_offset_max = MAX2(src_offset_max, ve->src_offset);

        if (so->vb_access_size[vbi] < (ve->src_offset + size))
           so->vb_access_size[vbi] = ve->src_offset + size;

        if (unlikely(ve->instance_divisor)) {
           so->instance_elts |= 1 << i;
           so->instance_bufs |= 1 << vbi;
           if (ve->instance_divisor < so->min_instance_div[vbi])
              so->min_instance_div[vbi] = ve->instance_divisor;
        }

        if (1) {
            unsigned ca;
            unsigned j = transkey.nr_elements++;

            ca = util_format_description(fmt)->channel[0].size / 8;
            if (ca != 1 && ca != 2)
               ca = 4;

            transkey.element[j].type = TRANSLATE_ELEMENT_NORMAL;
            transkey.element[j].input_format = ve->src_format;
            transkey.element[j].input_buffer = vbi;
            transkey.element[j].input_offset = ve->src_offset;
            transkey.element[j].instance_divisor = ve->instance_divisor;

            transkey.output_stride = align(transkey.output_stride, ca);
            transkey.element[j].output_format = fmt;
            transkey.element[j].output_offset = transkey.output_stride;
            transkey.output_stride += size;

            so->element[i].state_alt = so->element[i].state;
            so->element[i].state_alt |= transkey.element[j].output_offset << 7;
        }

        so->element[i].state |= i << NVC0_3D_VERTEX_ATTRIB_FORMAT_BUFFER__SHIFT;
    }
    transkey.output_stride = align(transkey.output_stride, 4);

    so->size = transkey.output_stride;
    so->translate = translate_create(&transkey);

    if (so->instance_elts || src_offset_max >= (1 << 14))
       return so;
    so->shared_slots = TRUE;

    for (i = 0; i < num_elements; ++i) {
       const unsigned b = elements[i].vertex_buffer_index;
       const unsigned s = elements[i].src_offset;
       so->element[i].state &= ~NVC0_3D_VERTEX_ATTRIB_FORMAT_BUFFER__MASK;
       so->element[i].state |= b << NVC0_3D_VERTEX_ATTRIB_FORMAT_BUFFER__SHIFT;
       so->element[i].state |= s << NVC0_3D_VERTEX_ATTRIB_FORMAT_OFFSET__SHIFT;
    }
    return so;
}

#define NVC0_3D_VERTEX_ATTRIB_INACTIVE                                       \
   NVC0_3D_VERTEX_ATTRIB_FORMAT_TYPE_FLOAT |                                 \
   NVC0_3D_VERTEX_ATTRIB_FORMAT_SIZE_32 | NVC0_3D_VERTEX_ATTRIB_FORMAT_CONST

#define VTX_ATTR(a, c, t, s)                            \
   ((NVC0_3D_VTX_ATTR_DEFINE_TYPE_##t) |                \
    (NVC0_3D_VTX_ATTR_DEFINE_SIZE_##s) |                \
    ((a) << NVC0_3D_VTX_ATTR_DEFINE_ATTR__SHIFT) |      \
    ((c) << NVC0_3D_VTX_ATTR_DEFINE_COMP__SHIFT))

static void
nvc0_set_constant_vertex_attrib(struct nvc0_context *nvc0, const unsigned a)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct pipe_vertex_element *ve = &nvc0->vertex->element[a].pipe;
   struct pipe_vertex_buffer *vb = &nvc0->vtxbuf[ve->vertex_buffer_index];
   uint32_t mode;
   const struct util_format_description *desc;
   void *dst;
   const void *src = (const uint8_t *)vb->user_buffer + ve->src_offset;
   assert(!vb->buffer);

   desc = util_format_description(ve->src_format);

   PUSH_SPACE(push, 6);
   BEGIN_NVC0(push, NVC0_3D(VTX_ATTR_DEFINE), 5);
   dst = &push->cur[1];
   if (desc->channel[0].pure_integer) {
      if (desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED) {
         mode = VTX_ATTR(a, 4, SINT, 32);
         desc->unpack_rgba_sint(dst, 0, src, 0, 1, 1);
      } else {
         mode = VTX_ATTR(a, 4, UINT, 32);
         desc->unpack_rgba_uint(dst, 0, src, 0, 1, 1);
      }
   } else {
      mode = VTX_ATTR(a, 4, FLOAT, 32);
      desc->unpack_rgba_float(dst, 0, src, 0, 1, 1);
   }
   push->cur[0] = mode;
   push->cur += 5;
}

static INLINE void
nvc0_user_vbuf_range(struct nvc0_context *nvc0, int vbi,
                     uint32_t *base, uint32_t *size)
{
   if (unlikely(nvc0->vertex->instance_bufs & (1 << vbi))) {
      const uint32_t div = nvc0->vertex->min_instance_div[vbi];
      *base = nvc0->instance_off * nvc0->vtxbuf[vbi].stride;
      *size = (nvc0->instance_max / div) * nvc0->vtxbuf[vbi].stride +
         nvc0->vertex->vb_access_size[vbi];
   } else {
      /* NOTE: if there are user buffers, we *must* have index bounds */
      assert(nvc0->vb_elt_limit != ~0);
      *base = nvc0->vb_elt_first * nvc0->vtxbuf[vbi].stride;
      *size = nvc0->vb_elt_limit * nvc0->vtxbuf[vbi].stride +
         nvc0->vertex->vb_access_size[vbi];
   }
}

static INLINE void
nvc0_release_user_vbufs(struct nvc0_context *nvc0)
{
   if (nvc0->vbo_user) {
      nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_VTX_TMP);
      nouveau_scratch_done(&nvc0->base);
   }
}

static void
nvc0_update_user_vbufs(struct nvc0_context *nvc0)
{
   uint64_t address[PIPE_MAX_ATTRIBS];
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   int i;
   uint32_t written = 0;

   PUSH_SPACE(push, nvc0->vertex->num_elements * 8);
   for (i = 0; i < nvc0->vertex->num_elements; ++i) {
      struct pipe_vertex_element *ve = &nvc0->vertex->element[i].pipe;
      const unsigned b = ve->vertex_buffer_index;
      struct pipe_vertex_buffer *vb = &nvc0->vtxbuf[b];
      uint32_t base, size;

      if (!(nvc0->vbo_user & (1 << b)))
         continue;
      if (!vb->stride) {
         nvc0_set_constant_vertex_attrib(nvc0, i);
         continue;
      }
      nvc0_user_vbuf_range(nvc0, b, &base, &size);

      if (!(written & (1 << b))) {
         struct nouveau_bo *bo;
         const uint32_t bo_flags = NOUVEAU_BO_RD | NOUVEAU_BO_GART;
         written |= 1 << b;
         address[b] = nouveau_scratch_data(&nvc0->base, vb->user_buffer,
                                           base, size, &bo);
         if (bo)
            BCTX_REFN_bo(nvc0->bufctx_3d, VTX_TMP, bo_flags, bo);
      }

      BEGIN_1IC0(push, NVC0_3D(MACRO_VERTEX_ARRAY_SELECT), 5);
      PUSH_DATA (push, i);
      PUSH_DATAh(push, address[b] + base + size - 1);
      PUSH_DATA (push, address[b] + base + size - 1);
      PUSH_DATAh(push, address[b] + ve->src_offset);
      PUSH_DATA (push, address[b] + ve->src_offset);
   }
   nvc0->base.vbo_dirty = TRUE;
}

static void
nvc0_update_user_vbufs_shared(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   uint32_t mask = nvc0->vbo_user & ~nvc0->constant_vbos;

   PUSH_SPACE(push, nvc0->num_vtxbufs * 8);
   while (mask) {
      struct nouveau_bo *bo;
      const uint32_t bo_flags = NOUVEAU_BO_RD | NOUVEAU_BO_GART;
      uint64_t address;
      uint32_t base, size;
      const int b = ffs(mask) - 1;
      mask &= ~(1 << b);

      nvc0_user_vbuf_range(nvc0, b, &base, &size);

      address = nouveau_scratch_data(&nvc0->base, nvc0->vtxbuf[b].user_buffer,
                                     base, size, &bo);
      if (bo)
         BCTX_REFN_bo(nvc0->bufctx_3d, VTX_TMP, bo_flags, bo);

      BEGIN_1IC0(push, NVC0_3D(MACRO_VERTEX_ARRAY_SELECT), 5);
      PUSH_DATA (push, b);
      PUSH_DATAh(push, address + base + size - 1);
      PUSH_DATA (push, address + base + size - 1);
      PUSH_DATAh(push, address);
      PUSH_DATA (push, address);
   }

   mask = nvc0->state.constant_elts;
   while (mask) {
      int i = ffs(mask) - 1;
      mask &= ~(1 << i);
      nvc0_set_constant_vertex_attrib(nvc0, i);
   }
}

static void
nvc0_validate_vertex_buffers(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   const struct nvc0_vertex_stateobj *vertex = nvc0->vertex;
   uint32_t refd = 0;
   unsigned i;

   PUSH_SPACE(push, vertex->num_elements * 8);
   for (i = 0; i < vertex->num_elements; ++i) {
      const struct nvc0_vertex_element *ve;
      const struct pipe_vertex_buffer *vb;
      struct nv04_resource *res;
      unsigned b;
      unsigned limit, offset;

      if (nvc0->state.constant_elts & (1 << i))
         continue;
      ve = &vertex->element[i];
      b = ve->pipe.vertex_buffer_index;
      vb = &nvc0->vtxbuf[b];

      if (!vb->buffer) {
         if (vb->stride) {
            if (ve->pipe.instance_divisor) {
               BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_DIVISOR(i)), 1);
               PUSH_DATA (push, ve->pipe.instance_divisor);
            }
            BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(i)), 1);
            PUSH_DATA (push, (1 << 12) | vb->stride);
         }
         /* address/value set in nvc0_update_user_vbufs */
         continue;
      }
      res = nv04_resource(vb->buffer);
      offset = ve->pipe.src_offset + vb->buffer_offset;
      limit = vb->buffer->width0 - 1;

      if (unlikely(ve->pipe.instance_divisor)) {
         BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(i)), 4);
         PUSH_DATA (push, (1 << 12) | vb->stride);
         PUSH_DATAh(push, res->address + offset);
         PUSH_DATA (push, res->address + offset);
         PUSH_DATA (push, ve->pipe.instance_divisor);
      } else {
         BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(i)), 3);
         PUSH_DATA (push, (1 << 12) | vb->stride);
         PUSH_DATAh(push, res->address + offset);
         PUSH_DATA (push, res->address + offset);
      }
      BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_LIMIT_HIGH(i)), 2);
      PUSH_DATAh(push, res->address + limit);
      PUSH_DATA (push, res->address + limit);

      if (!(refd & (1 << b))) {
         refd |= 1 << b;
         BCTX_REFN(nvc0->bufctx_3d, VTX, res, RD);
      }
   }
   if (nvc0->vbo_user)
      nvc0_update_user_vbufs(nvc0);
}

static void
nvc0_validate_vertex_buffers_shared(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   unsigned b;
   const uint32_t mask = nvc0->vbo_user;

   PUSH_SPACE(push, nvc0->num_vtxbufs * 8);
   for (b = 0; b < nvc0->num_vtxbufs; ++b) {
      struct pipe_vertex_buffer *vb = &nvc0->vtxbuf[b];
      struct nv04_resource *buf;
      uint32_t offset, limit;

      if (mask & (1 << b)) {
         if (vb->stride) {
            BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(b)), 1);
            PUSH_DATA (push, NVC0_3D_VERTEX_ARRAY_FETCH_ENABLE | vb->stride);
         }
         /* address/value set in nvc0_update_user_vbufs_shared */
         continue;
      }
      buf = nv04_resource(vb->buffer);
      offset = vb->buffer_offset;
      limit = buf->base.width0 - 1;

      BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(b)), 3);
      PUSH_DATA (push, NVC0_3D_VERTEX_ARRAY_FETCH_ENABLE | vb->stride);
      PUSH_DATAh(push, buf->address + offset);
      PUSH_DATA (push, buf->address + offset);
      BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_LIMIT_HIGH(b)), 2);
      PUSH_DATAh(push, buf->address + limit);
      PUSH_DATA (push, buf->address + limit);

      BCTX_REFN(nvc0->bufctx_3d, VTX, buf, RD);
   }
   if (nvc0->vbo_user)
      nvc0_update_user_vbufs_shared(nvc0);
}

void
nvc0_vertex_arrays_validate(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_vertex_stateobj *vertex = nvc0->vertex;
   struct nvc0_vertex_element *ve;
   uint32_t const_vbos;
   unsigned i;
   uint8_t vbo_mode;
   boolean update_vertex;

   nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_VTX);

   if (unlikely(vertex->need_conversion) ||
       unlikely(nvc0->vertprog->vp.edgeflag < PIPE_MAX_ATTRIBS)) {
      vbo_mode = 3;
   } else {
      vbo_mode = (nvc0->vbo_user && nvc0->vbo_push_hint) ? 1 : 0;
   }
   const_vbos = vbo_mode ? 0 : nvc0->constant_vbos;

   update_vertex = (nvc0->dirty & NVC0_NEW_VERTEX) ||
      (const_vbos != nvc0->state.constant_vbos) ||
      (vbo_mode != nvc0->state.vbo_mode);

   if (update_vertex) {
      const unsigned n = MAX2(vertex->num_elements, nvc0->state.num_vtxelts);

      nvc0->state.constant_vbos = const_vbos;
      nvc0->state.constant_elts = 0;
      nvc0->state.num_vtxelts = vertex->num_elements;
      nvc0->state.vbo_mode = vbo_mode;

      if (unlikely(vbo_mode)) {
         if (unlikely(nvc0->state.instance_elts & 3)) {
            /* translate mode uses only 2 vertex buffers */
            nvc0->state.instance_elts &= ~3;
            PUSH_SPACE(push, 3);
            BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_PER_INSTANCE(0)), 2);
            PUSH_DATA (push, 0);
            PUSH_DATA (push, 0);
         }

         PUSH_SPACE(push, n * 2 + 4);

         BEGIN_NVC0(push, NVC0_3D(VERTEX_ATTRIB_FORMAT(0)), n);
         for (i = 0; i < vertex->num_elements; ++i)
            PUSH_DATA(push, vertex->element[i].state_alt);
         for (; i < n; ++i)
            PUSH_DATA(push, NVC0_3D_VERTEX_ATTRIB_INACTIVE);

         BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(0)), 1);
         PUSH_DATA (push, (1 << 12) | vertex->size);
         for (i = 1; i < n; ++i)
            IMMED_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(i)), 0);
      } else {
         uint32_t *restrict data;

         if (unlikely(vertex->instance_elts != nvc0->state.instance_elts)) {
            nvc0->state.instance_elts = vertex->instance_elts;
            assert(n); /* if (n == 0), both masks should be 0 */
            PUSH_SPACE(push, 3);
            BEGIN_NVC0(push, NVC0_3D(MACRO_VERTEX_ARRAY_PER_INSTANCE), 2);
            PUSH_DATA (push, n);
            PUSH_DATA (push, vertex->instance_elts);
         }

         PUSH_SPACE(push, n * 2 + 1);
         BEGIN_NVC0(push, NVC0_3D(VERTEX_ATTRIB_FORMAT(0)), n);
         data = push->cur;
         push->cur += n;
         for (i = 0; i < vertex->num_elements; ++i) {
            ve = &vertex->element[i];
            data[i] = ve->state;
            if (unlikely(const_vbos & (1 << ve->pipe.vertex_buffer_index))) {
               nvc0->state.constant_elts |= 1 << i;
               data[i] |= NVC0_3D_VERTEX_ATTRIB_FORMAT_CONST;
               IMMED_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(i)), 0);
            }
         }
         for (; i < n; ++i) {
            data[i] = NVC0_3D_VERTEX_ATTRIB_INACTIVE;
            IMMED_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(i)), 0);
         }
      }
   }
   if (nvc0->state.vbo_mode) /* using translate, don't set up arrays here */
      return;

   if (vertex->shared_slots)
      nvc0_validate_vertex_buffers_shared(nvc0);
   else
      nvc0_validate_vertex_buffers(nvc0);
}

void
nvc0_idxbuf_validate(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nv04_resource *buf = nv04_resource(nvc0->idxbuf.buffer);

   assert(buf);
   assert(nouveau_resource_mapped_by_gpu(&buf->base));

   PUSH_SPACE(push, 6);
   BEGIN_NVC0(push, NVC0_3D(INDEX_ARRAY_START_HIGH), 5);
   PUSH_DATAh(push, buf->address + nvc0->idxbuf.offset);
   PUSH_DATA (push, buf->address + nvc0->idxbuf.offset);
   PUSH_DATAh(push, buf->address + buf->base.width0 - 1);
   PUSH_DATA (push, buf->address + buf->base.width0 - 1);
   PUSH_DATA (push, nvc0->idxbuf.index_size >> 1);

   BCTX_REFN(nvc0->bufctx_3d, IDX, buf, RD);
}

#define NVC0_PRIM_GL_CASE(n) \
   case PIPE_PRIM_##n: return NVC0_3D_VERTEX_BEGIN_GL_PRIMITIVE_##n

static INLINE unsigned
nvc0_prim_gl(unsigned prim)
{
   switch (prim) {
   NVC0_PRIM_GL_CASE(POINTS);
   NVC0_PRIM_GL_CASE(LINES);
   NVC0_PRIM_GL_CASE(LINE_LOOP);
   NVC0_PRIM_GL_CASE(LINE_STRIP);
   NVC0_PRIM_GL_CASE(TRIANGLES);
   NVC0_PRIM_GL_CASE(TRIANGLE_STRIP);
   NVC0_PRIM_GL_CASE(TRIANGLE_FAN);
   NVC0_PRIM_GL_CASE(QUADS);
   NVC0_PRIM_GL_CASE(QUAD_STRIP);
   NVC0_PRIM_GL_CASE(POLYGON);
   NVC0_PRIM_GL_CASE(LINES_ADJACENCY);
   NVC0_PRIM_GL_CASE(LINE_STRIP_ADJACENCY);
   NVC0_PRIM_GL_CASE(TRIANGLES_ADJACENCY);
   NVC0_PRIM_GL_CASE(TRIANGLE_STRIP_ADJACENCY);
   /*
   NVC0_PRIM_GL_CASE(PATCHES); */
   default:
      return NVC0_3D_VERTEX_BEGIN_GL_PRIMITIVE_POINTS;
   }
}

static void
nvc0_draw_vbo_kick_notify(struct nouveau_pushbuf *push)
{
   struct nvc0_screen *screen = push->user_priv;

   nouveau_fence_update(&screen->base, TRUE);
}

static void
nvc0_draw_arrays(struct nvc0_context *nvc0,
                 unsigned mode, unsigned start, unsigned count,
                 unsigned instance_count)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   unsigned prim;

   if (nvc0->state.index_bias) {
      PUSH_SPACE(push, 1);
      IMMED_NVC0(push, NVC0_3D(VB_ELEMENT_BASE), 0);
      nvc0->state.index_bias = 0;
   }

   prim = nvc0_prim_gl(mode);

   while (instance_count--) {
      PUSH_SPACE(push, 6);
      BEGIN_NVC0(push, NVC0_3D(VERTEX_BEGIN_GL), 1);
      PUSH_DATA (push, prim);
      BEGIN_NVC0(push, NVC0_3D(VERTEX_BUFFER_FIRST), 2);
      PUSH_DATA (push, start);
      PUSH_DATA (push, count);
      IMMED_NVC0(push, NVC0_3D(VERTEX_END_GL), 0);

      prim |= NVC0_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
   }
}

static void
nvc0_draw_elements_inline_u08(struct nouveau_pushbuf *push, const uint8_t *map,
                              unsigned start, unsigned count)
{
   map += start;

   if (count & 3) {
      unsigned i;
      PUSH_SPACE(push, 4);
      BEGIN_NIC0(push, NVC0_3D(VB_ELEMENT_U32), count & 3);
      for (i = 0; i < (count & 3); ++i)
         PUSH_DATA(push, *map++);
      count &= ~3;
   }
   while (count) {
      unsigned i, nr = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN * 4) / 4;

      PUSH_SPACE(push, nr + 1);
      BEGIN_NIC0(push, NVC0_3D(VB_ELEMENT_U8), nr);
      for (i = 0; i < nr; ++i) {
         PUSH_DATA(push,
                  (map[3] << 24) | (map[2] << 16) | (map[1] << 8) | map[0]);
         map += 4;
      }
      count -= nr * 4;
   }
}

static void
nvc0_draw_elements_inline_u16(struct nouveau_pushbuf *push, const uint16_t *map,
                              unsigned start, unsigned count)
{
   map += start;

   if (count & 1) {
      count &= ~1;
      PUSH_SPACE(push, 2);
      BEGIN_NVC0(push, NVC0_3D(VB_ELEMENT_U32), 1);
      PUSH_DATA (push, *map++);
   }
   while (count) {
      unsigned i, nr = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN * 2) / 2;

      PUSH_SPACE(push, nr + 1);
      BEGIN_NIC0(push, NVC0_3D(VB_ELEMENT_U16), nr);
      for (i = 0; i < nr; ++i) {
         PUSH_DATA(push, (map[1] << 16) | map[0]);
         map += 2;
      }
      count -= nr * 2;
   }
}

static void
nvc0_draw_elements_inline_u32(struct nouveau_pushbuf *push, const uint32_t *map,
                              unsigned start, unsigned count)
{
   map += start;

   while (count) {
      const unsigned nr = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN);

      PUSH_SPACE(push, nr + 1);
      BEGIN_NIC0(push, NVC0_3D(VB_ELEMENT_U32), nr);
      PUSH_DATAp(push, map, nr);

      map += nr;
      count -= nr;
   }
}

static void
nvc0_draw_elements_inline_u32_short(struct nouveau_pushbuf *push,
                                    const uint32_t *map,
                                    unsigned start, unsigned count)
{
   map += start;

   if (count & 1) {
      count--;
      PUSH_SPACE(push, 1);
      BEGIN_NVC0(push, NVC0_3D(VB_ELEMENT_U32), 1);
      PUSH_DATA (push, *map++);
   }
   while (count) {
      unsigned i, nr = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN * 2) / 2;

      PUSH_SPACE(push, nr + 1);
      BEGIN_NIC0(push, NVC0_3D(VB_ELEMENT_U16), nr);
      for (i = 0; i < nr; ++i) {
         PUSH_DATA(push, (map[1] << 16) | map[0]);
         map += 2;
      }
      count -= nr * 2;
   }
}

static void
nvc0_draw_elements(struct nvc0_context *nvc0, boolean shorten,
                   unsigned mode, unsigned start, unsigned count,
                   unsigned instance_count, int32_t index_bias)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   unsigned prim;
   const unsigned index_size = nvc0->idxbuf.index_size;

   prim = nvc0_prim_gl(mode);

   if (index_bias != nvc0->state.index_bias) {
      PUSH_SPACE(push, 2);
      BEGIN_NVC0(push, NVC0_3D(VB_ELEMENT_BASE), 1);
      PUSH_DATA (push, index_bias);
      nvc0->state.index_bias = index_bias;
   }

   if (nvc0->idxbuf.buffer) {
      PUSH_SPACE(push, 1);
      IMMED_NVC0(push, NVC0_3D(VERTEX_BEGIN_GL), prim);
      do {
         PUSH_SPACE(push, 7);
         BEGIN_NVC0(push, NVC0_3D(INDEX_BATCH_FIRST), 2);
         PUSH_DATA (push, start);
         PUSH_DATA (push, count);
         if (--instance_count) {
            BEGIN_NVC0(push, NVC0_3D(VERTEX_END_GL), 2);
            PUSH_DATA (push, 0);
            PUSH_DATA (push, prim | NVC0_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT);
         }
      } while (instance_count);
      IMMED_NVC0(push, NVC0_3D(VERTEX_END_GL), 0);
   } else {
      const void *data = nvc0->idxbuf.user_buffer;

      while (instance_count--) {
         PUSH_SPACE(push, 2);
         BEGIN_NVC0(push, NVC0_3D(VERTEX_BEGIN_GL), 1);
         PUSH_DATA (push, prim);
         switch (index_size) {
         case 1:
            nvc0_draw_elements_inline_u08(push, data, start, count);
            break;
         case 2:
            nvc0_draw_elements_inline_u16(push, data, start, count);
            break;
         case 4:
            if (shorten)
               nvc0_draw_elements_inline_u32_short(push, data, start, count);
            else
               nvc0_draw_elements_inline_u32(push, data, start, count);
            break;
         default:
            assert(0);
            return;
         }
         PUSH_SPACE(push, 1);
         IMMED_NVC0(push, NVC0_3D(VERTEX_END_GL), 0);

         prim |= NVC0_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
      }
   }
}

static void
nvc0_draw_stream_output(struct nvc0_context *nvc0,
                        const struct pipe_draw_info *info)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_so_target *so = nvc0_so_target(info->count_from_stream_output);
   struct nv04_resource *res = nv04_resource(so->pipe.buffer);
   unsigned mode = nvc0_prim_gl(info->mode);
   unsigned num_instances = info->instance_count;

   if (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
      res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;
      PUSH_SPACE(push, 2);
      IMMED_NVC0(push, NVC0_3D(SERIALIZE), 0);
      nvc0_query_fifo_wait(push, so->pq);
      IMMED_NVC0(push, NVC0_3D(VERTEX_ARRAY_FLUSH), 0);
   }

   while (num_instances--) {
      PUSH_SPACE(push, 8);
      BEGIN_NVC0(push, NVC0_3D(VERTEX_BEGIN_GL), 1);
      PUSH_DATA (push, mode);
      BEGIN_NVC0(push, NVC0_3D(DRAW_TFB_BASE), 1);
      PUSH_DATA (push, 0);
      BEGIN_NVC0(push, NVC0_3D(DRAW_TFB_STRIDE), 1);
      PUSH_DATA (push, so->stride);
      BEGIN_NVC0(push, NVC0_3D(DRAW_TFB_BYTES), 1);
      nvc0_query_pushbuf_submit(push, so->pq, 0x4);
      IMMED_NVC0(push, NVC0_3D(VERTEX_END_GL), 0);

      mode |= NVC0_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
   }
}

void
nvc0_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;

   /* NOTE: caller must ensure that (min_index + index_bias) is >= 0 */
   nvc0->vb_elt_first = info->min_index + info->index_bias;
   nvc0->vb_elt_limit = info->max_index - info->min_index;
   nvc0->instance_off = info->start_instance;
   nvc0->instance_max = info->instance_count - 1;

   /* For picking only a few vertices from a large user buffer, push is better,
    * if index count is larger and we expect repeated vertices, suggest upload.
    */
   nvc0->vbo_push_hint =
      info->indexed && (nvc0->vb_elt_limit >= (info->count * 2));

   /* Check whether we want to switch vertex-submission mode. */
   if (nvc0->vbo_user && !(nvc0->dirty & (NVC0_NEW_ARRAYS | NVC0_NEW_VERTEX))) {
      if (nvc0->vbo_push_hint != !!nvc0->state.vbo_mode)
         if (nvc0->state.vbo_mode != 3)
            nvc0->dirty |= NVC0_NEW_ARRAYS;

      if (!(nvc0->dirty & NVC0_NEW_ARRAYS) && nvc0->state.vbo_mode == 0) {
         if (nvc0->vertex->shared_slots)
            nvc0_update_user_vbufs_shared(nvc0);
         else
            nvc0_update_user_vbufs(nvc0);
      }
   }

   /* 8 as minimum to avoid immediate double validation of new buffers */
   nvc0_state_validate(nvc0, ~0, 8);

   push->kick_notify = nvc0_draw_vbo_kick_notify;

   if (nvc0->state.vbo_mode) {
      nvc0_push_vbo(nvc0, info);
      push->kick_notify = nvc0_default_kick_notify;
      return;
   }

   /* space for base instance, flush, and prim restart */
   PUSH_SPACE(push, 8);

   if (nvc0->state.instance_base != info->start_instance) {
      nvc0->state.instance_base = info->start_instance;
      /* NOTE: this does not affect the shader input, should it ? */
      BEGIN_NVC0(push, NVC0_3D(VB_INSTANCE_BASE), 1);
      PUSH_DATA (push, info->start_instance);
   }

   if (nvc0->base.vbo_dirty) {
      IMMED_NVC0(push, NVC0_3D(VERTEX_ARRAY_FLUSH), 0);
      nvc0->base.vbo_dirty = FALSE;
   }

   if (info->indexed) {
      boolean shorten = info->max_index <= 65535;

      if (info->primitive_restart != nvc0->state.prim_restart) {
         if (info->primitive_restart) {
            BEGIN_NVC0(push, NVC0_3D(PRIM_RESTART_ENABLE), 2);
            PUSH_DATA (push, 1);
            PUSH_DATA (push, info->restart_index);

            if (info->restart_index > 65535)
               shorten = FALSE;
         } else {
            IMMED_NVC0(push, NVC0_3D(PRIM_RESTART_ENABLE), 0);
         }
         nvc0->state.prim_restart = info->primitive_restart;
      } else
      if (info->primitive_restart) {
         BEGIN_NVC0(push, NVC0_3D(PRIM_RESTART_INDEX), 1);
         PUSH_DATA (push, info->restart_index);

         if (info->restart_index > 65535)
            shorten = FALSE;
      }

      nvc0_draw_elements(nvc0, shorten,
                         info->mode, info->start, info->count,
                         info->instance_count, info->index_bias);
   } else
   if (unlikely(info->count_from_stream_output)) {
      nvc0_draw_stream_output(nvc0, info);
   } else {
      nvc0_draw_arrays(nvc0,
                       info->mode, info->start, info->count,
                       info->instance_count);
   }
   push->kick_notify = nvc0_default_kick_notify;

   nvc0_release_user_vbufs(nvc0);
}
