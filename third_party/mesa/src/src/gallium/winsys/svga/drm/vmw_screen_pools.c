/**********************************************************
 * Copyright 2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/


#include "vmw_screen.h"

#include "vmw_buffer.h"
#include "vmw_fence.h"

#include "pipebuffer/pb_buffer.h"
#include "pipebuffer/pb_bufmgr.h"

/*
 * TODO: Have the query pool always ask the fence manager for
 * SVGA_FENCE_FLAG_QUERY signaled. Unfortunately, pb_fenced doesn't
 * support that currently, so we'd have to create a separate
 * pb_fence_ops wrapper that does this implicitly.
 */

/**
 * vmw_pools_cleanup - Destroy the buffer pools.
 *
 * @vws: pointer to a struct vmw_winsys_screen.
 */
void
vmw_pools_cleanup(struct vmw_winsys_screen *vws)
{
   if(vws->pools.gmr_fenced)
      vws->pools.gmr_fenced->destroy(vws->pools.gmr_fenced);
   if (vws->pools.query_fenced)
      vws->pools.query_fenced->destroy(vws->pools.query_fenced);

   /* gmr_mm pool is already destroyed above */

   if (vws->pools.gmr_slab_fenced)
      vws->pools.gmr_slab_fenced->destroy(vws->pools.gmr_slab_fenced);

   if(vws->pools.gmr)
      vws->pools.gmr->destroy(vws->pools.gmr);
   if(vws->pools.query)
      vws->pools.query->destroy(vws->pools.query);
}


/**
 * vmw_query_pools_init - Create a pool of query buffers.
 *
 * @vws: Pointer to a struct vmw_winsys_screen.
 *
 * Typically this pool should be created on demand when we
 * detect that the app will be using queries. There's nothing
 * special with this pool other than the backing kernel buffer size,
 * which is limited to 8192.
 */
boolean
vmw_query_pools_init(struct vmw_winsys_screen *vws)
{
   vws->pools.query = vmw_gmr_bufmgr_create(vws);
   if(!vws->pools.query)
      return FALSE;

   vws->pools.query_mm = mm_bufmgr_create(vws->pools.query,
					  VMW_QUERY_POOL_SIZE,
					  3 /* 8 alignment */);
   if(!vws->pools.query_mm)
      goto out_no_query_mm;

   vws->pools.query_fenced = fenced_bufmgr_create(
      vws->pools.query_mm,
      vmw_fence_ops_create(vws),
      VMW_QUERY_POOL_SIZE,
      ~0);

   if(!vws->pools.query_fenced)
      goto out_no_query_fenced;

   return TRUE;

  out_no_query_fenced:
   vws->pools.query_mm->destroy(vws->pools.query_mm);
  out_no_query_mm:
   vws->pools.query->destroy(vws->pools.query);
   return FALSE;
}

/**
 * vmw_pools_init - Create a pool of GMR buffers.
 *
 * @vws: Pointer to a struct vmw_winsys_screen.
 */
boolean
vmw_pools_init(struct vmw_winsys_screen *vws)
{
   struct pb_desc desc;

   vws->pools.gmr = vmw_gmr_bufmgr_create(vws);
   if(!vws->pools.gmr)
      goto error;

   vws->pools.gmr_mm = mm_bufmgr_create(vws->pools.gmr,
                                        VMW_GMR_POOL_SIZE,
                                        12 /* 4096 alignment */);
   if(!vws->pools.gmr_mm)
      goto error;

   /*
    * We disallow "CPU" buffers to be created by the fenced_bufmgr_create,
    * because that defers "GPU" buffer creation to buffer validation,
    * and at buffer validation we have no means of handling failures
    * due to pools space shortage or fragmentation. Effectively this
    * makes sure all failures are reported immediately on buffer allocation,
    * and we can revert to allocating directly from the kernel.
    */
   vws->pools.gmr_fenced = fenced_bufmgr_create(
      vws->pools.gmr_mm,
      vmw_fence_ops_create(vws),
      VMW_GMR_POOL_SIZE,
      0);

#ifdef DEBUG
   vws->pools.gmr_fenced = pb_debug_manager_create(vws->pools.gmr_fenced,
						   4096,
						   4096);
#endif
   if(!vws->pools.gmr_fenced)
      goto error;

   /*
    * The slab pool allocates buffers directly from the kernel except
    * for very small buffers which are allocated from a slab in order
    * not to waste memory, since a kernel buffer is a minimum 4096 bytes.
    *
    * Here we use it only for emergency in the case our pre-allocated
    * buffer pool runs out of memory.
    */
   desc.alignment = 64;
   desc.usage = ~0;
   vws->pools.gmr_slab = pb_slab_range_manager_create(vws->pools.gmr,
						      64,
						      8192,
						      16384,
						      &desc);
   if (!vws->pools.gmr_slab)
       goto error;

   vws->pools.gmr_slab_fenced =
       fenced_bufmgr_create(vws->pools.gmr_slab,
			    vmw_fence_ops_create(vws),
			    VMW_MAX_BUFFER_SIZE,
			    0);

   if (!vws->pools.gmr_slab_fenced)
       goto error;

   vws->pools.query_fenced = NULL;
   vws->pools.query_mm = NULL;
   vws->pools.query = NULL;

   return TRUE;

error:
   vmw_pools_cleanup(vws);
   return FALSE;
}

