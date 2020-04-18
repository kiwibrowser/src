/**********************************************************
 * Copyright 2009-2011 VMware, Inc.  All rights reserved.
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
/*
 * TODO:
 *
 * Fencing is currently a bit inefficient, since we need to call the
 * kernel do determine a fence object signaled status if the fence is not
 * signaled. This can be greatly improved upon by using the fact that the
 * execbuf ioctl returns the last signaled fence seqno, as does the
 * fence signaled ioctl. We should set up a ring of fence objects and
 * walk through them checking for signaled status each time we receive a
 * new passed fence seqno.
 */

#include "util/u_memory.h"
#include "util/u_atomic.h"

#include "pipebuffer/pb_buffer_fenced.h"

#include "vmw_screen.h"
#include "vmw_fence.h"

struct vmw_fence_ops 
{
   struct pb_fence_ops base;

   struct vmw_winsys_screen *vws;
};

struct vmw_fence
{
   int32_t refcount;
   uint32_t handle;
   uint32_t mask;
   int32_t signalled;
};

/**
 * vmw_fence - return the vmw_fence object identified by a
 * struct pipe_fence_handle *
 *
 * @fence: The opaque pipe fence handle.
 */
static INLINE struct vmw_fence *
vmw_fence(struct pipe_fence_handle *fence)
{
   return (struct vmw_fence *) fence;
}

/**
 * vmw_fence_create - Create a user-space fence object.
 *
 * @handle: Handle identifying the kernel fence object.
 * @mask: Mask of flags that this fence object may signal.
 *
 * Returns NULL on failure.
 */
struct pipe_fence_handle *
vmw_fence_create(uint32_t handle, uint32_t mask)
{
   struct vmw_fence *fence = CALLOC_STRUCT(vmw_fence);

   if (!fence)
      return NULL;

   p_atomic_set(&fence->refcount, 1);
   fence->handle = handle;
   fence->mask = mask;
   p_atomic_set(&fence->signalled, 0);

   return (struct pipe_fence_handle *) fence;
}

/**
 * vmw_fence_ops - Return the vmw_fence_ops structure backing a
 * struct pb_fence_ops pointer.
 *
 * @ops: Pointer to a struct pb_fence_ops.
 *
 */
static INLINE struct vmw_fence_ops *
vmw_fence_ops(struct pb_fence_ops *ops)
{
   assert(ops);
   return (struct vmw_fence_ops *)ops;
}



/**
 * vmw_fence_reference - Reference / unreference a vmw fence object.
 *
 * @vws: Pointer to the winsys screen.
 * @ptr: Pointer to reference transfer destination.
 * @fence: Pointer to object to reference. May be NULL.
 */
void
vmw_fence_reference(struct vmw_winsys_screen *vws,
		    struct pipe_fence_handle **ptr,
		    struct pipe_fence_handle *fence)
{
   if (*ptr) {
      struct vmw_fence *vfence = vmw_fence(*ptr);

      if (p_atomic_dec_zero(&vfence->refcount)) {
	 vmw_ioctl_fence_unref(vws, vfence->handle);
	 FREE(vfence);
      }
   }

   if (fence) {
      struct vmw_fence *vfence = vmw_fence(fence);

      p_atomic_inc(&vfence->refcount);
   }

   *ptr = fence;
}


/**
 * vmw_fence_signalled - Check whether a fence object is signalled.
 *
 * @vws: Pointer to the winsys screen.
 * @fence: Handle to the fence object.
 * @flag: Fence flags to check. If the fence object can't signal
 * a flag, it is assumed to be already signaled.
 *
 * Returns 0 if the fence object was signaled, nonzero otherwise.
 */
int
vmw_fence_signalled(struct vmw_winsys_screen *vws,
		   struct pipe_fence_handle *fence,
		   unsigned flag)
{
   struct vmw_fence *vfence;
   int32_t vflags = SVGA_FENCE_FLAG_EXEC;
   int ret;
   uint32_t old;

   if (!fence)
      return 0;

   vfence = vmw_fence(fence);
   old = p_atomic_read(&vfence->signalled);

   vflags &= ~vfence->mask;

   if ((old & vflags) == vflags)
      return 0;

   ret = vmw_ioctl_fence_signalled(vws, vfence->handle, vflags);

   if (ret == 0) {
      int32_t prev = old;

      do {
	 old = prev;
	 prev = p_atomic_cmpxchg(&vfence->signalled, old, old | vflags);
      } while (prev != old);
   }

   return ret;
}

/**
 * vmw_fence_finish - Wait for a fence object to signal.
 *
 * @vws: Pointer to the winsys screen.
 * @fence: Handle to the fence object.
 * @flag: Fence flags to wait for. If the fence object can't signal
 * a flag, it is assumed to be already signaled.
 *
 * Returns 0 if the wait succeeded. Nonzero otherwise.
 */
int
vmw_fence_finish(struct vmw_winsys_screen *vws,
		 struct pipe_fence_handle *fence,
		 unsigned flag)
{
   struct vmw_fence *vfence;
   int32_t vflags = SVGA_FENCE_FLAG_EXEC;
   int ret;
   uint32_t old;

   if (!fence)
      return 0;

   vfence = vmw_fence(fence);
   old = p_atomic_read(&vfence->signalled);
   vflags &= ~vfence->mask;

   if ((old & vflags) == vflags)
      return 0;

   ret = vmw_ioctl_fence_finish(vws, vfence->handle, vflags);

   if (ret == 0) {
      int32_t prev = old;

      do {
	 old = prev;
	 prev = p_atomic_cmpxchg(&vfence->signalled, old, old | vflags);
      } while (prev != old);
   }

   return ret;
}


/**
 * vmw_fence_ops_fence_reference - wrapper for the pb_fence_ops api.
 *
 * wrapper around vmw_fence_reference.
 */
static void
vmw_fence_ops_fence_reference(struct pb_fence_ops *ops,
                              struct pipe_fence_handle **ptr,
                              struct pipe_fence_handle *fence)
{
   struct vmw_winsys_screen *vws = vmw_fence_ops(ops)->vws;

   vmw_fence_reference(vws, ptr, fence);
}

/**
 * vmw_fence_ops_fence_signalled - wrapper for the pb_fence_ops api.
 *
 * wrapper around vmw_fence_signalled.
 */
static int
vmw_fence_ops_fence_signalled(struct pb_fence_ops *ops,
                              struct pipe_fence_handle *fence,
                              unsigned flag)
{
   struct vmw_winsys_screen *vws = vmw_fence_ops(ops)->vws;

   return vmw_fence_signalled(vws, fence, flag);
}


/**
 * vmw_fence_ops_fence_finish - wrapper for the pb_fence_ops api.
 *
 * wrapper around vmw_fence_finish.
 */
static int
vmw_fence_ops_fence_finish(struct pb_fence_ops *ops,
                           struct pipe_fence_handle *fence,
                           unsigned flag)
{
   struct vmw_winsys_screen *vws = vmw_fence_ops(ops)->vws;

   return vmw_fence_finish(vws, fence, flag);
}


/**
 * vmw_fence_ops_destroy - Destroy a pb_fence_ops function table.
 *
 * @ops: The function table to destroy.
 *
 * Part of the pb_fence_ops api.
 */
static void
vmw_fence_ops_destroy(struct pb_fence_ops *ops)
{
   FREE(ops);
}


/**
 * vmw_fence_ops_create - Create a pb_fence_ops function table.
 *
 * @vws: Pointer to a struct vmw_winsys_screen.
 *
 * Returns a pointer to a pb_fence_ops function table to interface
 * with pipe_buffer. This function is typically called on driver setup.
 *
 * Returns NULL on failure.
 */
struct pb_fence_ops *
vmw_fence_ops_create(struct vmw_winsys_screen *vws) 
{
   struct vmw_fence_ops *ops;

   ops = CALLOC_STRUCT(vmw_fence_ops);
   if(!ops)
      return NULL;

   ops->base.destroy = &vmw_fence_ops_destroy;
   ops->base.fence_reference = &vmw_fence_ops_fence_reference;
   ops->base.fence_signalled = &vmw_fence_ops_fence_signalled;
   ops->base.fence_finish = &vmw_fence_ops_fence_finish;

   ops->vws = vws;

   return &ops->base;
}


