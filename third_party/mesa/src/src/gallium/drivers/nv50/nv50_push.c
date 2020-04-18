
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "translate/translate.h"

#include "nv50_context.h"
#include "nv50_resource.h"

#include "nv50_3d.xml.h"

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
   uint32_t instance_id;
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

      BEGIN_NI04(ctx->push, NV50_3D(VERTEX_DATA), size);

      ctx->translate->run_elts8(ctx->translate, elts, nr, ctx->instance_id,
                                ctx->push->cur);

      ctx->push->cur += size;
      count -= nr;
      elts += nr;

      if (nr != push) {
         count--;
         elts++;
         BEGIN_NV04(ctx->push, NV50_3D(VB_ELEMENT_U32), 1);
         PUSH_DATA (ctx->push, ctx->restart_index);
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

      BEGIN_NI04(ctx->push, NV50_3D(VERTEX_DATA), size);

      ctx->translate->run_elts16(ctx->translate, elts, nr, ctx->instance_id,
                                 ctx->push->cur);

      ctx->push->cur += size;
      count -= nr;
      elts += nr;

      if (nr != push) {
         count--;
         elts++;
         BEGIN_NV04(ctx->push, NV50_3D(VB_ELEMENT_U32), 1);
         PUSH_DATA (ctx->push, ctx->restart_index);
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

      BEGIN_NI04(ctx->push, NV50_3D(VERTEX_DATA), size);

      ctx->translate->run_elts(ctx->translate, elts, nr, ctx->instance_id,
                               ctx->push->cur);

      ctx->push->cur += size;
      count -= nr;
      elts += nr;

      if (nr != push) {
         count--;
         elts++;
         BEGIN_NV04(ctx->push, NV50_3D(VB_ELEMENT_U32), 1);
         PUSH_DATA (ctx->push, ctx->restart_index);
      }
   }
}

static void
emit_vertices_seq(struct push_context *ctx, unsigned start, unsigned count)
{
   while (count) {
      unsigned push = MIN2(count, ctx->packet_vertex_limit);
      unsigned size = ctx->vertex_words * push;

      BEGIN_NI04(ctx->push, NV50_3D(VERTEX_DATA), size);

      ctx->translate->run(ctx->translate, start, push, ctx->instance_id,
                          ctx->push->cur);
      ctx->push->cur += size;
      count -= push;
      start += push;
   }
}


#define NV50_PRIM_GL_CASE(n) \
   case PIPE_PRIM_##n: return NV50_3D_VERTEX_BEGIN_GL_PRIMITIVE_##n

static INLINE unsigned
nv50_prim_gl(unsigned prim)
{
   switch (prim) {
   NV50_PRIM_GL_CASE(POINTS);
   NV50_PRIM_GL_CASE(LINES);
   NV50_PRIM_GL_CASE(LINE_LOOP);
   NV50_PRIM_GL_CASE(LINE_STRIP);
   NV50_PRIM_GL_CASE(TRIANGLES);
   NV50_PRIM_GL_CASE(TRIANGLE_STRIP);
   NV50_PRIM_GL_CASE(TRIANGLE_FAN);
   NV50_PRIM_GL_CASE(QUADS);
   NV50_PRIM_GL_CASE(QUAD_STRIP);
   NV50_PRIM_GL_CASE(POLYGON);
   NV50_PRIM_GL_CASE(LINES_ADJACENCY);
   NV50_PRIM_GL_CASE(LINE_STRIP_ADJACENCY);
   NV50_PRIM_GL_CASE(TRIANGLES_ADJACENCY);
   NV50_PRIM_GL_CASE(TRIANGLE_STRIP_ADJACENCY);
   /*
   NV50_PRIM_GL_CASE(PATCHES); */
   default:
      return NV50_3D_VERTEX_BEGIN_GL_PRIMITIVE_POINTS;
      break;
   }
}

void
nv50_push_vbo(struct nv50_context *nv50, const struct pipe_draw_info *info)
{
   struct push_context ctx;
   unsigned i, index_size;
   unsigned inst_count = info->instance_count;
   unsigned vert_count = info->count;
   boolean apply_bias = info->indexed && info->index_bias;

   ctx.push = nv50->base.pushbuf;
   ctx.translate = nv50->vertex->translate;
   ctx.packet_vertex_limit = nv50->vertex->packet_vertex_limit;
   ctx.vertex_words = nv50->vertex->vertex_size;

   for (i = 0; i < nv50->num_vtxbufs; ++i) {
      const struct pipe_vertex_buffer *vb = &nv50->vtxbuf[i];
      const uint8_t *data;

      if (unlikely(vb->buffer))
         data = nouveau_resource_map_offset(&nv50->base,
            nv04_resource(vb->buffer), vb->buffer_offset, NOUVEAU_BO_RD);
      else
         data = vb->user_buffer;

      if (apply_bias && likely(!(nv50->vertex->instance_bufs & (1 << i))))
         data += (ptrdiff_t)info->index_bias * vb->stride;

      ctx.translate->set_buffer(ctx.translate, i, data, vb->stride, ~0);
   }

   if (info->indexed) {
      if (nv50->idxbuf.buffer) {
         ctx.idxbuf = nouveau_resource_map_offset(&nv50->base,
            nv04_resource(nv50->idxbuf.buffer), nv50->idxbuf.offset,
            NOUVEAU_BO_RD);
      } else {
         ctx.idxbuf = nv50->idxbuf.user_buffer;
      }
      if (!ctx.idxbuf)
         return;
      index_size = nv50->idxbuf.index_size;
      ctx.primitive_restart = info->primitive_restart;
      ctx.restart_index = info->restart_index;
   } else {
      if (unlikely(info->count_from_stream_output)) {
         struct pipe_context *pipe = &nv50->base.pipe;
         struct nv50_so_target *targ;
         targ = nv50_so_target(info->count_from_stream_output);
         if (!targ->pq) {
            NOUVEAU_ERR("draw_stream_output not supported on pre-NVA0 cards\n");
            return;
         }
         pipe->get_query_result(pipe, targ->pq, TRUE, (void *)&vert_count);
         vert_count /= targ->stride;
      }
      ctx.idxbuf = NULL;
      index_size = 0;
      ctx.primitive_restart = FALSE;
      ctx.restart_index = 0;
   }

   ctx.instance_id = info->start_instance;
   ctx.prim = nv50_prim_gl(info->mode);

   if (info->primitive_restart) {
      BEGIN_NV04(ctx.push, NV50_3D(PRIM_RESTART_ENABLE), 2);
      PUSH_DATA (ctx.push, 1);
      PUSH_DATA (ctx.push, info->restart_index);
   } else
   if (nv50->state.prim_restart) {
      BEGIN_NV04(ctx.push, NV50_3D(PRIM_RESTART_ENABLE), 1);
      PUSH_DATA (ctx.push, 0);
   }
   nv50->state.prim_restart = info->primitive_restart;

   while (inst_count--) {
      BEGIN_NV04(ctx.push, NV50_3D(VERTEX_BEGIN_GL), 1);
      PUSH_DATA (ctx.push, ctx.prim);
      switch (index_size) {
      case 0:
         emit_vertices_seq(&ctx, info->start, vert_count);
         break;
      case 1:
         emit_vertices_i08(&ctx, info->start, vert_count);
         break;
      case 2:
         emit_vertices_i16(&ctx, info->start, vert_count);
         break;
      case 4:
         emit_vertices_i32(&ctx, info->start, vert_count);
         break;
      default:
         assert(0);
         break;
      }
      BEGIN_NV04(ctx.push, NV50_3D(VERTEX_END_GL), 1);
      PUSH_DATA (ctx.push, 0);

      ctx.instance_id++;
      ctx.prim |= NV50_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
   }
}
