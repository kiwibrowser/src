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

/**
 * @file
 * This file implements the SVGA interface into this winsys, defined
 * in drivers/svga/svga_winsys.h.
 *
 * @author Keith Whitwell
 * @author Jose Fonseca
 */


#include "svga_cmd.h"
#include "svga3d_caps.h"

#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "pipebuffer/pb_buffer.h"
#include "pipebuffer/pb_bufmgr.h"
#include "svga_winsys.h"
#include "vmw_context.h"
#include "vmw_screen.h"
#include "vmw_surface.h"
#include "vmw_buffer.h"
#include "vmw_fence.h"


static struct svga_winsys_buffer *
vmw_svga_winsys_buffer_create(struct svga_winsys_screen *sws,
                              unsigned alignment,
                              unsigned usage,
                              unsigned size)
{
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);
   struct pb_desc desc;
   struct pb_manager *provider;
   struct pb_buffer *buffer;

   memset(&desc, 0, sizeof desc);
   desc.alignment = alignment;
   desc.usage = usage;

   if (usage == SVGA_BUFFER_USAGE_PINNED) {
      if (vws->pools.query_fenced == NULL && !vmw_query_pools_init(vws))
	 return NULL;
      provider = vws->pools.query_fenced;
   } else
      provider = vws->pools.gmr_fenced;

   assert(provider);
   buffer = provider->create_buffer(provider, size, &desc);

   if(!buffer && provider == vws->pools.gmr_fenced) {

      assert(provider);
      provider = vws->pools.gmr_slab_fenced;
      buffer = provider->create_buffer(provider, size, &desc);
   }

   if (!buffer)
      return NULL;

   return vmw_svga_winsys_buffer(buffer);
}


static void *
vmw_svga_winsys_buffer_map(struct svga_winsys_screen *sws,
                           struct svga_winsys_buffer *buf,
                           unsigned flags)
{
   (void)sws;
   return pb_map(vmw_pb_buffer(buf), flags, NULL);
}


static void
vmw_svga_winsys_buffer_unmap(struct svga_winsys_screen *sws,
                             struct svga_winsys_buffer *buf)
{
   (void)sws;
   pb_unmap(vmw_pb_buffer(buf));
}


static void
vmw_svga_winsys_buffer_destroy(struct svga_winsys_screen *sws,
                               struct svga_winsys_buffer *buf)
{
   struct pb_buffer *pbuf = vmw_pb_buffer(buf);
   (void)sws;
   pb_reference(&pbuf, NULL);
}


static void
vmw_svga_winsys_fence_reference(struct svga_winsys_screen *sws,
                                struct pipe_fence_handle **pdst,
                                struct pipe_fence_handle *src)
{
    struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);

    vmw_fence_reference(vws, pdst, src);
}


static int
vmw_svga_winsys_fence_signalled(struct svga_winsys_screen *sws,
                                struct pipe_fence_handle *fence,
                                unsigned flag)
{
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);

   return vmw_fence_signalled(vws, fence, flag);
}


static int
vmw_svga_winsys_fence_finish(struct svga_winsys_screen *sws,
                             struct pipe_fence_handle *fence,
                             unsigned flag)
{
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);

   return vmw_fence_finish(vws, fence, flag);
}



static struct svga_winsys_surface *
vmw_svga_winsys_surface_create(struct svga_winsys_screen *sws,
                               SVGA3dSurfaceFlags flags,
                               SVGA3dSurfaceFormat format,
                               SVGA3dSize size,
                               uint32 numFaces,
                               uint32 numMipLevels)
{
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);
   struct vmw_svga_winsys_surface *surface;

   surface = CALLOC_STRUCT(vmw_svga_winsys_surface);
   if(!surface)
      goto no_surface;

   pipe_reference_init(&surface->refcnt, 1);
   p_atomic_set(&surface->validated, 0);
   surface->screen = vws;
   surface->sid = vmw_ioctl_surface_create(vws,
                                           flags, format, size,
                                           numFaces, numMipLevels);
   if(surface->sid == SVGA3D_INVALID_ID)
      goto no_sid;

   return svga_winsys_surface(surface);

no_sid:
   FREE(surface);
no_surface:
   return NULL;
}


static boolean
vmw_svga_winsys_surface_is_flushed(struct svga_winsys_screen *sws,
                                   struct svga_winsys_surface *surface)
{
   struct vmw_svga_winsys_surface *vsurf = vmw_svga_winsys_surface(surface);
   return (p_atomic_read(&vsurf->validated) == 0);
}


static void
vmw_svga_winsys_surface_ref(struct svga_winsys_screen *sws,
			    struct svga_winsys_surface **pDst,
			    struct svga_winsys_surface *src)
{
   struct vmw_svga_winsys_surface *d_vsurf = vmw_svga_winsys_surface(*pDst);
   struct vmw_svga_winsys_surface *s_vsurf = vmw_svga_winsys_surface(src);

   vmw_svga_winsys_surface_reference(&d_vsurf, s_vsurf);
   *pDst = svga_winsys_surface(d_vsurf);
}


static void
vmw_svga_winsys_destroy(struct svga_winsys_screen *sws)
{
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);

   vmw_winsys_destroy(vws);
}


static SVGA3dHardwareVersion
vmw_svga_winsys_get_hw_version(struct svga_winsys_screen *sws)
{
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);

   return (SVGA3dHardwareVersion) vws->ioctl.hwversion;
}


static boolean
vmw_svga_winsys_get_cap(struct svga_winsys_screen *sws,
                        SVGA3dDevCapIndex index,
                        SVGA3dDevCapResult *result)
{
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);
   const uint32 *capsBlock;
   const SVGA3dCapsRecord *capsRecord = NULL;
   uint32 offset;
   const SVGA3dCapPair *capArray;
   int numCaps, first, last;

   if(vws->ioctl.hwversion < SVGA3D_HWVERSION_WS6_B1)
      return FALSE;

   /*
    * Search linearly through the caps block records for the specified type.
    */
   capsBlock = (const uint32 *)vws->ioctl.buffer;
   for (offset = 0; capsBlock[offset] != 0; offset += capsBlock[offset]) {
      const SVGA3dCapsRecord *record;
      assert(offset < SVGA_FIFO_3D_CAPS_SIZE);
      record = (const SVGA3dCapsRecord *) (capsBlock + offset);
      if ((record->header.type >= SVGA3DCAPS_RECORD_DEVCAPS_MIN) &&
          (record->header.type <= SVGA3DCAPS_RECORD_DEVCAPS_MAX) &&
          (!capsRecord || (record->header.type > capsRecord->header.type))) {
         capsRecord = record;
      }
   }

   if(!capsRecord)
      return FALSE;

   /*
    * Calculate the number of caps from the size of the record.
    */
   capArray = (const SVGA3dCapPair *) capsRecord->data;
   numCaps = (int) ((capsRecord->header.length * sizeof(uint32) -
                     sizeof capsRecord->header) / (2 * sizeof(uint32)));

   /*
    * Binary-search for the cap with the specified index.
    */
   for (first = 0, last = numCaps - 1; first <= last; ) {
      int mid = (first + last) / 2;

      if ((SVGA3dDevCapIndex) capArray[mid][0] == index) {
         /*
          * Found it.
          */
         result->u = capArray[mid][1];
         return TRUE;
      }

      /*
       * Divide and conquer.
       */
      if ((SVGA3dDevCapIndex) capArray[mid][0] > index) {
         last = mid - 1;
      } else {
         first = mid + 1;
      }
   }

   return FALSE;
}


boolean
vmw_winsys_screen_init_svga(struct vmw_winsys_screen *vws)
{
   vws->base.destroy = vmw_svga_winsys_destroy;
   vws->base.get_hw_version = vmw_svga_winsys_get_hw_version;
   vws->base.get_cap = vmw_svga_winsys_get_cap;
   vws->base.context_create = vmw_svga_winsys_context_create;
   vws->base.surface_create = vmw_svga_winsys_surface_create;
   vws->base.surface_is_flushed = vmw_svga_winsys_surface_is_flushed;
   vws->base.surface_reference = vmw_svga_winsys_surface_ref;
   vws->base.buffer_create = vmw_svga_winsys_buffer_create;
   vws->base.buffer_map = vmw_svga_winsys_buffer_map;
   vws->base.buffer_unmap = vmw_svga_winsys_buffer_unmap;
   vws->base.buffer_destroy = vmw_svga_winsys_buffer_destroy;
   vws->base.fence_reference = vmw_svga_winsys_fence_reference;
   vws->base.fence_signalled = vmw_svga_winsys_fence_signalled;
   vws->base.fence_finish = vmw_svga_winsys_fence_finish;

   return TRUE;
}


