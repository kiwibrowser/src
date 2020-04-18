/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include "pipe/p_screen.h"
#include "util/u_memory.h"
#include "lp_debug.h"
#include "lp_fence.h"


/**
 * Create a new fence object.
 *
 * The rank will be the number of bins in the scene.  Whenever a rendering
 * thread hits a fence command, it'll increment the fence counter.  When
 * the counter == the rank, the fence is finished.
 *
 * \param rank  the expected finished value of the fence counter.
 */
struct lp_fence *
lp_fence_create(unsigned rank)
{
   static int fence_id;
   struct lp_fence *fence = CALLOC_STRUCT(lp_fence);

   if (!fence)
      return NULL;

   pipe_reference_init(&fence->reference, 1);

   pipe_mutex_init(fence->mutex);
   pipe_condvar_init(fence->signalled);

   fence->id = fence_id++;
   fence->rank = rank;

   if (LP_DEBUG & DEBUG_FENCE)
      debug_printf("%s %d\n", __FUNCTION__, fence->id);

   return fence;
}


/** Destroy a fence.  Called when refcount hits zero. */
void
lp_fence_destroy(struct lp_fence *fence)
{
   if (LP_DEBUG & DEBUG_FENCE)
      debug_printf("%s %d\n", __FUNCTION__, fence->id);

   pipe_mutex_destroy(fence->mutex);
   pipe_condvar_destroy(fence->signalled);
   FREE(fence);
}


/**
 * Called by the rendering threads to increment the fence counter.
 * When the counter == the rank, the fence is finished.
 */
void
lp_fence_signal(struct lp_fence *fence)
{
   if (LP_DEBUG & DEBUG_FENCE)
      debug_printf("%s %d\n", __FUNCTION__, fence->id);

   pipe_mutex_lock(fence->mutex);

   fence->count++;
   assert(fence->count <= fence->rank);

   if (LP_DEBUG & DEBUG_FENCE)
      debug_printf("%s count=%u rank=%u\n", __FUNCTION__,
                   fence->count, fence->rank);

   /* Wakeup all threads waiting on the mutex:
    */
   pipe_condvar_broadcast(fence->signalled);

   pipe_mutex_unlock(fence->mutex);
}

boolean
lp_fence_signalled(struct lp_fence *f)
{
   return f->count == f->rank;
}

void
lp_fence_wait(struct lp_fence *f)
{
   if (LP_DEBUG & DEBUG_FENCE)
      debug_printf("%s %d\n", __FUNCTION__, f->id);

   pipe_mutex_lock(f->mutex);
   assert(f->issued);
   while (f->count < f->rank) {
      pipe_condvar_wait(f->signalled, f->mutex);
   }
   pipe_mutex_unlock(f->mutex);
}


