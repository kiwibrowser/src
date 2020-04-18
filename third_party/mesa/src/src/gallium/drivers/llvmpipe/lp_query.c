/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2010 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/* Authors:
 *    Keith Whitwell, Qicheng Christopher Li, Brian Paul
 */

#include "draw/draw_context.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "lp_context.h"
#include "lp_flush.h"
#include "lp_fence.h"
#include "lp_query.h"
#include "lp_state.h"


static struct llvmpipe_query *llvmpipe_query( struct pipe_query *p )
{
   return (struct llvmpipe_query *)p;
}

static struct pipe_query *
llvmpipe_create_query(struct pipe_context *pipe, 
                      unsigned type)
{
   struct llvmpipe_query *pq;

   assert(type == PIPE_QUERY_OCCLUSION_COUNTER);

   pq = CALLOC_STRUCT( llvmpipe_query );

   return (struct pipe_query *) pq;
}


static void
llvmpipe_destroy_query(struct pipe_context *pipe, struct pipe_query *q)
{
   struct llvmpipe_query *pq = llvmpipe_query(q);

   /* Ideally we would refcount queries & not get destroyed until the
    * last scene had finished with us.
    */
   if (pq->fence) {
      if (!lp_fence_issued(pq->fence))
         llvmpipe_flush(pipe, NULL, __FUNCTION__);

      if (!lp_fence_signalled(pq->fence))
         lp_fence_wait(pq->fence);

      lp_fence_reference(&pq->fence, NULL);
   }

   FREE(pq);
}


static boolean
llvmpipe_get_query_result(struct pipe_context *pipe, 
                          struct pipe_query *q,
                          boolean wait,
                          union pipe_query_result *vresult)
{
   struct llvmpipe_query *pq = llvmpipe_query(q);
   uint64_t *result = (uint64_t *)vresult;
   int i;

   if (!pq->fence) {
      /* no fence because there was no scene, so results is zero */
      *result = 0;
      return TRUE;
   }

   if (!lp_fence_signalled(pq->fence)) {
      if (!lp_fence_issued(pq->fence))
         llvmpipe_flush(pipe, NULL, __FUNCTION__);
         
      if (!wait)
         return FALSE;

      lp_fence_wait(pq->fence);
   }

   /* Sum the results from each of the threads:
    */
   *result = 0;
   for (i = 0; i < LP_MAX_THREADS; i++) {
      *result += pq->count[i];
   }

   return TRUE;
}


static void
llvmpipe_begin_query(struct pipe_context *pipe, struct pipe_query *q)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context( pipe );
   struct llvmpipe_query *pq = llvmpipe_query(q);

   /* Check if the query is already in the scene.  If so, we need to
    * flush the scene now.  Real apps shouldn't re-use a query in a
    * frame of rendering.
    */
   if (pq->fence && !lp_fence_issued(pq->fence)) {
      llvmpipe_finish(pipe, __FUNCTION__);
   }


   memset(pq->count, 0, sizeof(pq->count));
   lp_setup_begin_query(llvmpipe->setup, pq);

   llvmpipe->active_query_count++;
   llvmpipe->dirty |= LP_NEW_QUERY;
}


static void
llvmpipe_end_query(struct pipe_context *pipe, struct pipe_query *q)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context( pipe );
   struct llvmpipe_query *pq = llvmpipe_query(q);

   lp_setup_end_query(llvmpipe->setup, pq);

   assert(llvmpipe->active_query_count);
   llvmpipe->active_query_count--;
   llvmpipe->dirty |= LP_NEW_QUERY;
}

boolean
llvmpipe_check_render_cond(struct llvmpipe_context *lp)
{
   struct pipe_context *pipe = &lp->pipe;
   boolean b, wait;
   uint64_t result;

   if (!lp->render_cond_query)
      return TRUE; /* no query predicate, draw normally */
   wait = (lp->render_cond_mode == PIPE_RENDER_COND_WAIT ||
           lp->render_cond_mode == PIPE_RENDER_COND_BY_REGION_WAIT);

   b = pipe->get_query_result(pipe, lp->render_cond_query, wait, (void*)&result);
   if (b)
      return result > 0;
   else
      return TRUE;
}

void llvmpipe_init_query_funcs(struct llvmpipe_context *llvmpipe )
{
   llvmpipe->pipe.create_query = llvmpipe_create_query;
   llvmpipe->pipe.destroy_query = llvmpipe_destroy_query;
   llvmpipe->pipe.begin_query = llvmpipe_begin_query;
   llvmpipe->pipe.end_query = llvmpipe_end_query;
   llvmpipe->pipe.get_query_result = llvmpipe_get_query_result;
}


