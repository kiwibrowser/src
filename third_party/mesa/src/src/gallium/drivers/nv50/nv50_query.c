/*
 * Copyright 2011 Nouveau Project
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
 * Authors: Christoph Bumiller
 */

#define NV50_PUSH_EXPLICIT_SPACE_CHECKING

#include "nv50_context.h"
#include "nouveau/nv_object.xml.h"

/* XXX: Nested queries, and simultaneous queries on multiple gallium contexts
 * (since we use only a single GPU channel per screen) will not work properly.
 *
 * The first is not that big of an issue because OpenGL does not allow nested
 * queries anyway.
 */

struct nv50_query {
   uint32_t *data;
   uint16_t type;
   uint16_t index;
   uint32_t sequence;
   struct nouveau_bo *bo;
   uint32_t base;
   uint32_t offset; /* base + i * 16 */
   boolean ready;
   boolean flushed;
   boolean is64bit;
   struct nouveau_mm_allocation *mm;
};

#define NV50_QUERY_ALLOC_SPACE 128

static INLINE struct nv50_query *
nv50_query(struct pipe_query *pipe)
{
   return (struct nv50_query *)pipe;
}

static boolean
nv50_query_allocate(struct nv50_context *nv50, struct nv50_query *q, int size)
{
   struct nv50_screen *screen = nv50->screen;
   int ret;

   if (q->bo) {
      nouveau_bo_ref(NULL, &q->bo);
      if (q->mm) {
         if (q->ready)
            nouveau_mm_free(q->mm);
         else
            nouveau_fence_work(screen->base.fence.current, nouveau_mm_free_work,
                               q->mm);
      }
   }
   if (size) {
      q->mm = nouveau_mm_allocate(screen->base.mm_GART, size, &q->bo, &q->base);
      if (!q->bo)
         return FALSE;
      q->offset = q->base;

      ret = nouveau_bo_map(q->bo, 0, screen->base.client);
      if (ret) {
         nv50_query_allocate(nv50, q, 0);
         return FALSE;
      }
      q->data = (uint32_t *)((uint8_t *)q->bo->map + q->base);
   }
   return TRUE;
}

static void
nv50_query_destroy(struct pipe_context *pipe, struct pipe_query *pq)
{
   nv50_query_allocate(nv50_context(pipe), nv50_query(pq), 0);
   FREE(nv50_query(pq));
}

static struct pipe_query *
nv50_query_create(struct pipe_context *pipe, unsigned type)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nv50_query *q;

   q = CALLOC_STRUCT(nv50_query);
   if (!q)
      return NULL;

   if (!nv50_query_allocate(nv50, q, NV50_QUERY_ALLOC_SPACE)) {
      FREE(q);
      return NULL;
   }

   q->is64bit = (type == PIPE_QUERY_PRIMITIVES_GENERATED ||
                 type == PIPE_QUERY_PRIMITIVES_EMITTED ||
                 type == PIPE_QUERY_SO_STATISTICS);
   q->type = type;

   if (q->type == PIPE_QUERY_OCCLUSION_COUNTER) {
      q->offset -= 16;
      q->data -= 16 / sizeof(*q->data); /* we advance before query_begin ! */
   }

   return (struct pipe_query *)q;
}

static void
nv50_query_get(struct nouveau_pushbuf *push, struct nv50_query *q,
               unsigned offset, uint32_t get)
{
   offset += q->offset;

   PUSH_SPACE(push, 5);
   PUSH_REFN (push, q->bo, NOUVEAU_BO_GART | NOUVEAU_BO_WR);
   BEGIN_NV04(push, NV50_3D(QUERY_ADDRESS_HIGH), 4);
   PUSH_DATAh(push, q->bo->offset + offset);
   PUSH_DATA (push, q->bo->offset + offset);
   PUSH_DATA (push, q->sequence);
   PUSH_DATA (push, get);
}

static void
nv50_query_begin(struct pipe_context *pipe, struct pipe_query *pq)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_query *q = nv50_query(pq);

   /* For occlusion queries we have to change the storage, because a previous
    * query might set the initial render conition to FALSE even *after* we re-
    * initialized it to TRUE.
    */
   if (q->type == PIPE_QUERY_OCCLUSION_COUNTER) {
      q->offset += 16;
      q->data += 16 / sizeof(*q->data);
      if (q->offset - q->base == NV50_QUERY_ALLOC_SPACE)
         nv50_query_allocate(nv50, q, NV50_QUERY_ALLOC_SPACE);

      /* XXX: can we do this with the GPU, and sync with respect to a previous
       *  query ?
       */
      q->data[1] = 1; /* initial render condition = TRUE */
   }
   if (!q->is64bit)
      q->data[0] = q->sequence++; /* the previously used one */

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      PUSH_SPACE(push, 4);
      BEGIN_NV04(push, NV50_3D(COUNTER_RESET), 1);
      PUSH_DATA (push, NV50_3D_COUNTER_RESET_SAMPLECNT);
      BEGIN_NV04(push, NV50_3D(SAMPLECNT_ENABLE), 1);
      PUSH_DATA (push, 1);
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      nv50_query_get(push, q, 0x10, 0x06805002);
      break;
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      nv50_query_get(push, q, 0x10, 0x05805002);
      break;
   case PIPE_QUERY_SO_STATISTICS:
      nv50_query_get(push, q, 0x20, 0x05805002);
      nv50_query_get(push, q, 0x30, 0x06805002);
      break;
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_TIME_ELAPSED:
      nv50_query_get(push, q, 0x10, 0x00005002);
      break;
   default:
      break;
   }
   q->ready = FALSE;
}

static void
nv50_query_end(struct pipe_context *pipe, struct pipe_query *pq)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_query *q = nv50_query(pq);

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      nv50_query_get(push, q, 0, 0x0100f002);
      PUSH_SPACE(push, 2);
      BEGIN_NV04(push, NV50_3D(SAMPLECNT_ENABLE), 1);
      PUSH_DATA (push, 0);
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      nv50_query_get(push, q, 0, 0x06805002);
      break;
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      nv50_query_get(push, q, 0, 0x05805002);
      break;
   case PIPE_QUERY_SO_STATISTICS:
      nv50_query_get(push, q, 0x00, 0x05805002);
      nv50_query_get(push, q, 0x10, 0x06805002);
      break;
   case PIPE_QUERY_TIMESTAMP:
      q->sequence++;
      /* fall through */
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_TIME_ELAPSED:
      nv50_query_get(push, q, 0, 0x00005002);
      break;
   case PIPE_QUERY_GPU_FINISHED:
      q->sequence++;
      nv50_query_get(push, q, 0, 0x1000f010);
      break;
   case NVA0_QUERY_STREAM_OUTPUT_BUFFER_OFFSET:
      nv50_query_get(push, q, 0, 0x0d005002 | (q->index << 5));
      break;
   default:
      assert(0);
      break;
   }
   q->ready = q->flushed = FALSE;
}

static INLINE boolean
nv50_query_ready(struct nv50_query *q)
{
   return q->ready || (!q->is64bit && (q->data[0] == q->sequence));
}

static boolean
nv50_query_result(struct pipe_context *pipe, struct pipe_query *pq,
                  boolean wait, union pipe_query_result *result)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nv50_query *q = nv50_query(pq);
   uint64_t *res64 = (uint64_t *)result;
   uint32_t *res32 = (uint32_t *)result;
   boolean *res8 = (boolean *)result;
   uint64_t *data64 = (uint64_t *)q->data;

   if (!q->ready) /* update ? */
      q->ready = nv50_query_ready(q);
   if (!q->ready) {
      if (!wait) {
         /* for broken apps that spin on GL_QUERY_RESULT_AVAILABLE */
         if (!q->flushed) {
            q->flushed = TRUE;
            PUSH_KICK(nv50->base.pushbuf);
         }
         return FALSE;
      }
      if (nouveau_bo_wait(q->bo, NOUVEAU_BO_RD, nv50->screen->base.client))
         return FALSE;
   }
   q->ready = TRUE;

   switch (q->type) {
   case PIPE_QUERY_GPU_FINISHED:
      res8[0] = TRUE;
      break;
   case PIPE_QUERY_OCCLUSION_COUNTER: /* u32 sequence, u32 count, u64 time */
      res64[0] = q->data[1];
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED: /* u64 count, u64 time */
   case PIPE_QUERY_PRIMITIVES_EMITTED: /* u64 count, u64 time */
      res64[0] = data64[0] - data64[2];
      break;
   case PIPE_QUERY_SO_STATISTICS:
      res64[0] = data64[0] - data64[4];
      res64[1] = data64[2] - data64[6];
      break;
   case PIPE_QUERY_TIMESTAMP:
      res64[0] = data64[1];
      break;
   case PIPE_QUERY_TIMESTAMP_DISJOINT: /* u32 sequence, u32 0, u64 time */
      res64[0] = 1000000000;
      res8[8] = (data64[1] == data64[3]) ? FALSE : TRUE;
      break;
   case PIPE_QUERY_TIME_ELAPSED:
      res64[0] = data64[1] - data64[3];
      break;
   case NVA0_QUERY_STREAM_OUTPUT_BUFFER_OFFSET:
      res32[0] = q->data[1];
      break;
   default:
      return FALSE;
   }

   return TRUE;
}

void
nv84_query_fifo_wait(struct nouveau_pushbuf *push, struct pipe_query *pq)
{
   struct nv50_query *q = nv50_query(pq);
   unsigned offset = q->offset;

   PUSH_SPACE(push, 5);
   PUSH_REFN (push, q->bo, NOUVEAU_BO_GART | NOUVEAU_BO_RD);
   BEGIN_NV04(push, SUBC_3D(NV84_SUBCHAN_SEMAPHORE_ADDRESS_HIGH), 4);
   PUSH_DATAh(push, q->bo->offset + offset);
   PUSH_DATA (push, q->bo->offset + offset);
   PUSH_DATA (push, q->sequence);
   PUSH_DATA (push, NV84_SUBCHAN_SEMAPHORE_TRIGGER_ACQUIRE_EQUAL);
}

static void
nv50_render_condition(struct pipe_context *pipe,
                      struct pipe_query *pq, uint mode)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_query *q;

   PUSH_SPACE(push, 6);

   if (!pq) {
      BEGIN_NV04(push, NV50_3D(COND_MODE), 1);
      PUSH_DATA (push, NV50_3D_COND_MODE_ALWAYS);
      return;
   }
   q = nv50_query(pq);

   if (mode == PIPE_RENDER_COND_WAIT ||
       mode == PIPE_RENDER_COND_BY_REGION_WAIT) {
      BEGIN_NV04(push, SUBC_3D(NV50_GRAPH_SERIALIZE), 1);
      PUSH_DATA (push, 0);
   }

   BEGIN_NV04(push, NV50_3D(COND_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, q->bo->offset + q->offset);
   PUSH_DATA (push, q->bo->offset + q->offset);
   PUSH_DATA (push, NV50_3D_COND_MODE_RES_NON_ZERO);
}

void
nv50_query_pushbuf_submit(struct nouveau_pushbuf *push,
                          struct pipe_query *pq, unsigned result_offset)
{
   struct nv50_query *q = nv50_query(pq);

   /* XXX: does this exist ? */
#define NV50_IB_ENTRY_1_NO_PREFETCH (0 << (31 - 8))

   nouveau_pushbuf_space(push, 0, 0, 1);
   nouveau_pushbuf_data(push, q->bo, q->offset + result_offset, 4 |
                        NV50_IB_ENTRY_1_NO_PREFETCH);
}

void
nva0_so_target_save_offset(struct pipe_context *pipe,
                           struct pipe_stream_output_target *ptarg,
                           unsigned index, boolean serialize)
{
   struct nv50_so_target *targ = nv50_so_target(ptarg);

   if (serialize) {
      struct nouveau_pushbuf *push = nv50_context(pipe)->base.pushbuf;
      PUSH_SPACE(push, 2);
      BEGIN_NV04(push, SUBC_3D(NV50_GRAPH_SERIALIZE), 1);
      PUSH_DATA (push, 0);
   }

   nv50_query(targ->pq)->index = index;
   nv50_query_end(pipe, targ->pq);
}

void
nv50_init_query_functions(struct nv50_context *nv50)
{
   struct pipe_context *pipe = &nv50->base.pipe;

   pipe->create_query = nv50_query_create;
   pipe->destroy_query = nv50_query_destroy;
   pipe->begin_query = nv50_query_begin;
   pipe->end_query = nv50_query_end;
   pipe->get_query_result = nv50_query_result;
   pipe->render_condition = nv50_render_condition;
}
