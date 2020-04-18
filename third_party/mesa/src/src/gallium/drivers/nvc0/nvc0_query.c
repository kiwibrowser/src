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

#define NVC0_PUSH_EXPLICIT_SPACE_CHECKING

#include "nvc0_context.h"
#include "nouveau/nv_object.xml.h"

#define NVC0_QUERY_STATE_READY   0
#define NVC0_QUERY_STATE_ACTIVE  1
#define NVC0_QUERY_STATE_ENDED   2
#define NVC0_QUERY_STATE_FLUSHED 3

struct nvc0_query {
   uint32_t *data;
   uint16_t type;
   uint16_t index;
   uint32_t sequence;
   struct nouveau_bo *bo;
   uint32_t base;
   uint32_t offset; /* base + i * rotate */
   uint8_t state;
   boolean is64bit;
   uint8_t rotate;
   int nesting; /* only used for occlusion queries */
   struct nouveau_mm_allocation *mm;
};

#define NVC0_QUERY_ALLOC_SPACE 256

static INLINE struct nvc0_query *
nvc0_query(struct pipe_query *pipe)
{
   return (struct nvc0_query *)pipe;
}

static boolean
nvc0_query_allocate(struct nvc0_context *nvc0, struct nvc0_query *q, int size)
{
   struct nvc0_screen *screen = nvc0->screen;
   int ret;

   if (q->bo) {
      nouveau_bo_ref(NULL, &q->bo);
      if (q->mm) {
         if (q->state == NVC0_QUERY_STATE_READY)
            nouveau_mm_free(q->mm);
         else
            nouveau_fence_work(screen->base.fence.current,
                               nouveau_mm_free_work, q->mm);
      }
   }
   if (size) {
      q->mm = nouveau_mm_allocate(screen->base.mm_GART, size, &q->bo, &q->base);
      if (!q->bo)
         return FALSE;
      q->offset = q->base;

      ret = nouveau_bo_map(q->bo, 0, screen->base.client);
      if (ret) {
         nvc0_query_allocate(nvc0, q, 0);
         return FALSE;
      }
      q->data = (uint32_t *)((uint8_t *)q->bo->map + q->base);
   }
   return TRUE;
}

static void
nvc0_query_destroy(struct pipe_context *pipe, struct pipe_query *pq)
{
   nvc0_query_allocate(nvc0_context(pipe), nvc0_query(pq), 0);
   FREE(nvc0_query(pq));
}

static struct pipe_query *
nvc0_query_create(struct pipe_context *pipe, unsigned type)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nvc0_query *q;
   unsigned space = NVC0_QUERY_ALLOC_SPACE;

   q = CALLOC_STRUCT(nvc0_query);
   if (!q)
      return NULL;

   switch (type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      q->rotate = 32;
      space = NVC0_QUERY_ALLOC_SPACE;
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      q->is64bit = TRUE;
      space = 512;
      break;
   case PIPE_QUERY_SO_STATISTICS:
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      q->is64bit = TRUE;
      space = 64;
      break;
   case PIPE_QUERY_TIME_ELAPSED:
   case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_GPU_FINISHED:
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      space = 32;
      break;
   case NVC0_QUERY_TFB_BUFFER_OFFSET:
      space = 16;
      break;
   default:
      FREE(q);
      return NULL;
   }
   if (!nvc0_query_allocate(nvc0, q, space)) {
      FREE(q);
      return NULL;
   }

   q->type = type;

   if (q->rotate) {
      /* we advance before query_begin ! */
      q->offset -= q->rotate;
      q->data -= q->rotate / sizeof(*q->data);
   } else
   if (!q->is64bit)
      q->data[0] = 0; /* initialize sequence */

   return (struct pipe_query *)q;
}

static void
nvc0_query_get(struct nouveau_pushbuf *push, struct nvc0_query *q,
               unsigned offset, uint32_t get)
{
   offset += q->offset;

   PUSH_SPACE(push, 5);
   PUSH_REFN (push, q->bo, NOUVEAU_BO_GART | NOUVEAU_BO_WR);
   BEGIN_NVC0(push, NVC0_3D(QUERY_ADDRESS_HIGH), 4);
   PUSH_DATAh(push, q->bo->offset + offset);
   PUSH_DATA (push, q->bo->offset + offset);
   PUSH_DATA (push, q->sequence);
   PUSH_DATA (push, get);
}

static void
nvc0_query_rotate(struct nvc0_context *nvc0, struct nvc0_query *q)
{
   q->offset += q->rotate;
   q->data += q->rotate / sizeof(*q->data);
   if (q->offset - q->base == NVC0_QUERY_ALLOC_SPACE)
      nvc0_query_allocate(nvc0, q, NVC0_QUERY_ALLOC_SPACE);
}

static void
nvc0_query_begin(struct pipe_context *pipe, struct pipe_query *pq)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_query *q = nvc0_query(pq);

   /* For occlusion queries we have to change the storage, because a previous
    * query might set the initial render conition to FALSE even *after* we re-
    * initialized it to TRUE.
    */
   if (q->rotate) {
      nvc0_query_rotate(nvc0, q);

      /* XXX: can we do this with the GPU, and sync with respect to a previous
       *  query ?
       */
      q->data[0] = q->sequence; /* initialize sequence */
      q->data[1] = 1; /* initial render condition = TRUE */
      q->data[4] = q->sequence + 1; /* for comparison COND_MODE */
      q->data[5] = 0;
   }
   q->sequence++;

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      q->nesting = nvc0->screen->num_occlusion_queries_active++;
      if (q->nesting) {
         nvc0_query_get(push, q, 0x10, 0x0100f002);
      } else {
         PUSH_SPACE(push, 3);
         BEGIN_NVC0(push, NVC0_3D(COUNTER_RESET), 1);
         PUSH_DATA (push, NVC0_3D_COUNTER_RESET_SAMPLECNT);
         IMMED_NVC0(push, NVC0_3D(SAMPLECNT_ENABLE), 1);
      }
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      nvc0_query_get(push, q, 0x10, 0x06805002 | (q->index << 5));
      break;
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      nvc0_query_get(push, q, 0x10, 0x05805002 | (q->index << 5));
      break;
   case PIPE_QUERY_SO_STATISTICS:
      nvc0_query_get(push, q, 0x20, 0x05805002 | (q->index << 5));
      nvc0_query_get(push, q, 0x30, 0x06805002 | (q->index << 5));
      break;
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      nvc0_query_get(push, q, 0x10, 0x03005002 | (q->index << 5));
      break;
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_TIME_ELAPSED:
      nvc0_query_get(push, q, 0x10, 0x00005002);
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      nvc0_query_get(push, q, 0xc0 + 0x00, 0x00801002); /* VFETCH, VERTICES */
      nvc0_query_get(push, q, 0xc0 + 0x10, 0x01801002); /* VFETCH, PRIMS */
      nvc0_query_get(push, q, 0xc0 + 0x20, 0x02802002); /* VP, LAUNCHES */
      nvc0_query_get(push, q, 0xc0 + 0x30, 0x03806002); /* GP, LAUNCHES */
      nvc0_query_get(push, q, 0xc0 + 0x40, 0x04806002); /* GP, PRIMS_OUT */
      nvc0_query_get(push, q, 0xc0 + 0x50, 0x07804002); /* RAST, PRIMS_IN */
      nvc0_query_get(push, q, 0xc0 + 0x60, 0x08804002); /* RAST, PRIMS_OUT */
      nvc0_query_get(push, q, 0xc0 + 0x70, 0x0980a002); /* ROP, PIXELS */
      nvc0_query_get(push, q, 0xc0 + 0x80, 0x0d808002); /* TCP, LAUNCHES */
      nvc0_query_get(push, q, 0xc0 + 0x90, 0x0e809002); /* TEP, LAUNCHES */
      break;
   default:
      break;
   }
   q->state = NVC0_QUERY_STATE_ACTIVE;
}

static void
nvc0_query_end(struct pipe_context *pipe, struct pipe_query *pq)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_query *q = nvc0_query(pq);

   if (q->state != NVC0_QUERY_STATE_ACTIVE) {
      /* some queries don't require 'begin' to be called (e.g. GPU_FINISHED) */
      if (q->rotate)
         nvc0_query_rotate(nvc0, q);
      q->sequence++;
   }
   q->state = NVC0_QUERY_STATE_ENDED;

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      nvc0_query_get(push, q, 0, 0x0100f002);
      if (--nvc0->screen->num_occlusion_queries_active == 0) {
         PUSH_SPACE(push, 1);
         IMMED_NVC0(push, NVC0_3D(SAMPLECNT_ENABLE), 0);
      }
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      nvc0_query_get(push, q, 0, 0x06805002 | (q->index << 5));
      break;
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      nvc0_query_get(push, q, 0, 0x05805002 | (q->index << 5));
      break;
   case PIPE_QUERY_SO_STATISTICS:
      nvc0_query_get(push, q, 0x00, 0x05805002 | (q->index << 5));
      nvc0_query_get(push, q, 0x10, 0x06805002 | (q->index << 5));
      break;
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      /* TODO: How do we sum over all streams for render condition ? */
      /* PRIMS_DROPPED doesn't write sequence, use a ZERO query to sync on */
      nvc0_query_get(push, q, 0x00, 0x03005002 | (q->index << 5));
      nvc0_query_get(push, q, 0x20, 0x00005002);
      break;
   case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_TIME_ELAPSED:
      nvc0_query_get(push, q, 0, 0x00005002);
      break;
   case PIPE_QUERY_GPU_FINISHED:
      nvc0_query_get(push, q, 0, 0x1000f010);
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      nvc0_query_get(push, q, 0x00, 0x00801002); /* VFETCH, VERTICES */
      nvc0_query_get(push, q, 0x10, 0x01801002); /* VFETCH, PRIMS */
      nvc0_query_get(push, q, 0x20, 0x02802002); /* VP, LAUNCHES */
      nvc0_query_get(push, q, 0x30, 0x03806002); /* GP, LAUNCHES */
      nvc0_query_get(push, q, 0x40, 0x04806002); /* GP, PRIMS_OUT */
      nvc0_query_get(push, q, 0x50, 0x07804002); /* RAST, PRIMS_IN */
      nvc0_query_get(push, q, 0x60, 0x08804002); /* RAST, PRIMS_OUT */
      nvc0_query_get(push, q, 0x70, 0x0980a002); /* ROP, PIXELS */
      nvc0_query_get(push, q, 0x80, 0x0d808002); /* TCP, LAUNCHES */
      nvc0_query_get(push, q, 0x90, 0x0e809002); /* TEP, LAUNCHES */
      break;
   case NVC0_QUERY_TFB_BUFFER_OFFSET:
      /* indexed by TFB buffer instead of by vertex stream */
      nvc0_query_get(push, q, 0x00, 0x0d005002 | (q->index << 5));
      break;
   default:
      assert(0);
      break;
   }
}

static INLINE void
nvc0_query_update(struct nouveau_client *cli, struct nvc0_query *q)
{
   if (q->is64bit) {
      if (!nouveau_bo_map(q->bo, NOUVEAU_BO_RD | NOUVEAU_BO_NOBLOCK, cli))
         q->state = NVC0_QUERY_STATE_READY;
   } else {
      if (q->data[0] == q->sequence)
         q->state = NVC0_QUERY_STATE_READY;
   }
}

static boolean
nvc0_query_result(struct pipe_context *pipe, struct pipe_query *pq,
                  boolean wait, union pipe_query_result *result)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nvc0_query *q = nvc0_query(pq);
   uint64_t *res64 = (uint64_t*)result;
   uint32_t *res32 = (uint32_t*)result;
   boolean *res8 = (boolean*)result;
   uint64_t *data64 = (uint64_t *)q->data;
   unsigned i;

   if (q->state != NVC0_QUERY_STATE_READY)
      nvc0_query_update(nvc0->screen->base.client, q);

   if (q->state != NVC0_QUERY_STATE_READY) {
      if (!wait) {
         if (q->state != NVC0_QUERY_STATE_FLUSHED) {
            q->state = NVC0_QUERY_STATE_FLUSHED;
            /* flush for silly apps that spin on GL_QUERY_RESULT_AVAILABLE */
            PUSH_KICK(nvc0->base.pushbuf);
         }
         return FALSE;
      }
      if (nouveau_bo_wait(q->bo, NOUVEAU_BO_RD, nvc0->screen->base.client))
         return FALSE;
   }
   q->state = NVC0_QUERY_STATE_READY;

   switch (q->type) {
   case PIPE_QUERY_GPU_FINISHED:
      res8[0] = TRUE;
      break;
   case PIPE_QUERY_OCCLUSION_COUNTER: /* u32 sequence, u32 count, u64 time */
      res64[0] = q->data[1] - q->data[5];
      break;
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      res8[0] = q->data[1] != q->data[5];
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED: /* u64 count, u64 time */
   case PIPE_QUERY_PRIMITIVES_EMITTED: /* u64 count, u64 time */
      res64[0] = data64[0] - data64[2];
      break;
   case PIPE_QUERY_SO_STATISTICS:
      res64[0] = data64[0] - data64[4];
      res64[1] = data64[2] - data64[6];
      break;
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      res8[0] = data64[0] != data64[2];
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
   case PIPE_QUERY_PIPELINE_STATISTICS:
      for (i = 0; i < 10; ++i)
         res64[i] = data64[i * 2] - data64[24 + i * 2];
      break;
   case NVC0_QUERY_TFB_BUFFER_OFFSET:
      res32[0] = q->data[1];
      break;
   default:
      return FALSE;
   }

   return TRUE;
}

void
nvc0_query_fifo_wait(struct nouveau_pushbuf *push, struct pipe_query *pq)
{
   struct nvc0_query *q = nvc0_query(pq);
   unsigned offset = q->offset;

   if (q->type == PIPE_QUERY_SO_OVERFLOW_PREDICATE) offset += 0x20;

   PUSH_SPACE(push, 5);
   PUSH_REFN (push, q->bo, NOUVEAU_BO_GART | NOUVEAU_BO_RD);
   BEGIN_NVC0(push, SUBC_3D(NV84_SUBCHAN_SEMAPHORE_ADDRESS_HIGH), 4);
   PUSH_DATAh(push, q->bo->offset + offset);
   PUSH_DATA (push, q->bo->offset + offset);
   PUSH_DATA (push, q->sequence);
   PUSH_DATA (push, (1 << 12) |
              NV84_SUBCHAN_SEMAPHORE_TRIGGER_ACQUIRE_EQUAL);
}

static void
nvc0_render_condition(struct pipe_context *pipe,
                      struct pipe_query *pq, uint mode)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_query *q;
   uint32_t cond;
   boolean negated = FALSE;
   boolean wait =
      mode != PIPE_RENDER_COND_NO_WAIT &&
      mode != PIPE_RENDER_COND_BY_REGION_NO_WAIT;

   if (!pq) {
      PUSH_SPACE(push, 1);
      IMMED_NVC0(push, NVC0_3D(COND_MODE), NVC0_3D_COND_MODE_ALWAYS);
      return;
   }
   q = nvc0_query(pq);

   /* NOTE: comparison of 2 queries only works if both have completed */
   switch (q->type) {
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      cond = negated ? NVC0_3D_COND_MODE_EQUAL :
                       NVC0_3D_COND_MODE_NOT_EQUAL;
      wait = TRUE;
      break;
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      if (likely(!negated)) {
         if (unlikely(q->nesting))
            cond = wait ? NVC0_3D_COND_MODE_NOT_EQUAL :
                          NVC0_3D_COND_MODE_ALWAYS;
         else
            cond = NVC0_3D_COND_MODE_RES_NON_ZERO;
      } else {
         cond = wait ? NVC0_3D_COND_MODE_EQUAL : NVC0_3D_COND_MODE_ALWAYS;
      }
      break;
   default:
      assert(!"render condition query not a predicate");
      mode = NVC0_3D_COND_MODE_ALWAYS;
      break;
   }

   if (wait)
      nvc0_query_fifo_wait(push, pq);

   PUSH_SPACE(push, 4);
   PUSH_REFN (push, q->bo, NOUVEAU_BO_GART | NOUVEAU_BO_RD);
   BEGIN_NVC0(push, NVC0_3D(COND_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, q->bo->offset + q->offset);
   PUSH_DATA (push, q->bo->offset + q->offset);
   PUSH_DATA (push, cond);
}

void
nvc0_query_pushbuf_submit(struct nouveau_pushbuf *push,
                          struct pipe_query *pq, unsigned result_offset)
{
   struct nvc0_query *q = nvc0_query(pq);

#define NVC0_IB_ENTRY_1_NO_PREFETCH (1 << (31 - 8))

   nouveau_pushbuf_space(push, 0, 0, 1);
   nouveau_pushbuf_data(push, q->bo, q->offset + result_offset, 4 |
                        NVC0_IB_ENTRY_1_NO_PREFETCH);
}

void
nvc0_so_target_save_offset(struct pipe_context *pipe,
                           struct pipe_stream_output_target *ptarg,
                           unsigned index, boolean *serialize)
{
   struct nvc0_so_target *targ = nvc0_so_target(ptarg);

   if (*serialize) {
      *serialize = FALSE;
      PUSH_SPACE(nvc0_context(pipe)->base.pushbuf, 1);
      IMMED_NVC0(nvc0_context(pipe)->base.pushbuf, NVC0_3D(SERIALIZE), 0);
   }

   nvc0_query(targ->pq)->index = index;

   nvc0_query_end(pipe, targ->pq);
}

void
nvc0_init_query_functions(struct nvc0_context *nvc0)
{
   struct pipe_context *pipe = &nvc0->base.pipe;

   pipe->create_query = nvc0_query_create;
   pipe->destroy_query = nvc0_query_destroy;
   pipe->begin_query = nvc0_query_begin;
   pipe->end_query = nvc0_query_end;
   pipe->get_query_result = nvc0_query_result;
   pipe->render_condition = nvc0_render_condition;
}
