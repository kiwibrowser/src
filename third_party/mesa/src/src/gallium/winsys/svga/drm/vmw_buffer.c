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
 * SVGA buffer manager for Guest Memory Regions (GMRs).
 * 
 * GMRs are used for pixel and vertex data upload/download to/from the virtual
 * SVGA hardware. There is a limited number of GMRs available, and 
 * creating/destroying them is also a slow operation so we must suballocate 
 * them.
 * 
 * This file implements a pipebuffer library's buffer manager, so that we can
 * use pipepbuffer's suballocation, fencing, and debugging facilities with GMRs. 
 * 
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "svga_cmd.h"

#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "pipebuffer/pb_buffer.h"
#include "pipebuffer/pb_bufmgr.h"

#include "svga_winsys.h"

#include "vmw_screen.h"
#include "vmw_buffer.h"


struct vmw_gmr_bufmgr;


struct vmw_gmr_buffer
{
   struct pb_buffer base;
   
   struct vmw_gmr_bufmgr *mgr;
   
   struct vmw_region *region;
   void *map;
};


extern const struct pb_vtbl vmw_gmr_buffer_vtbl;


static INLINE struct vmw_gmr_buffer *
vmw_gmr_buffer(struct pb_buffer *buf)
{
   assert(buf);
   assert(buf->vtbl == &vmw_gmr_buffer_vtbl);
   return (struct vmw_gmr_buffer *)buf;
}


struct vmw_gmr_bufmgr
{
   struct pb_manager base;
   
   struct vmw_winsys_screen *vws;
};


static INLINE struct vmw_gmr_bufmgr *
vmw_gmr_bufmgr(struct pb_manager *mgr)
{
   assert(mgr);
   return (struct vmw_gmr_bufmgr *)mgr;
}


static void
vmw_gmr_buffer_destroy(struct pb_buffer *_buf)
{
   struct vmw_gmr_buffer *buf = vmw_gmr_buffer(_buf);

   vmw_ioctl_region_unmap(buf->region);
   
   vmw_ioctl_region_destroy(buf->region);

   FREE(buf);
}


static void *
vmw_gmr_buffer_map(struct pb_buffer *_buf,
                   unsigned flags,
                   void *flush_ctx)
{
   struct vmw_gmr_buffer *buf = vmw_gmr_buffer(_buf);
   return buf->map;
}


static void
vmw_gmr_buffer_unmap(struct pb_buffer *_buf)
{
   /* Do nothing */
   (void)_buf;
}


static void
vmw_gmr_buffer_get_base_buffer(struct pb_buffer *buf,
                           struct pb_buffer **base_buf,
                           unsigned *offset)
{
   *base_buf = buf;
   *offset = 0;
}


static enum pipe_error
vmw_gmr_buffer_validate( struct pb_buffer *_buf, 
                         struct pb_validate *vl,
                         unsigned flags )
{
   /* Always pinned */
   return PIPE_OK;
}


static void
vmw_gmr_buffer_fence( struct pb_buffer *_buf, 
                      struct pipe_fence_handle *fence )
{
   /* We don't need to do anything, as the pipebuffer library
    * will take care of delaying the destruction of fenced buffers */  
}


const struct pb_vtbl vmw_gmr_buffer_vtbl = {
   vmw_gmr_buffer_destroy,
   vmw_gmr_buffer_map,
   vmw_gmr_buffer_unmap,
   vmw_gmr_buffer_validate,
   vmw_gmr_buffer_fence,
   vmw_gmr_buffer_get_base_buffer
};


static struct pb_buffer *
vmw_gmr_bufmgr_create_buffer(struct pb_manager *_mgr,
                         pb_size size,
                         const struct pb_desc *desc) 
{
   struct vmw_gmr_bufmgr *mgr = vmw_gmr_bufmgr(_mgr);
   struct vmw_winsys_screen *vws = mgr->vws;
   struct vmw_gmr_buffer *buf;
   
   buf = CALLOC_STRUCT(vmw_gmr_buffer);
   if(!buf)
      goto error1;

   pipe_reference_init(&buf->base.reference, 1);
   buf->base.alignment = desc->alignment;
   buf->base.usage = desc->usage;
   buf->base.size = size;
   buf->base.vtbl = &vmw_gmr_buffer_vtbl;
   buf->mgr = mgr;

   buf->region = vmw_ioctl_region_create(vws, size);
   if(!buf->region)
      goto error2;
	 
   buf->map = vmw_ioctl_region_map(buf->region);
   if(!buf->map)
      goto error3;

   return &buf->base;

error3:
   vmw_ioctl_region_destroy(buf->region);
error2:
   FREE(buf);
error1:
   return NULL;
}


static void
vmw_gmr_bufmgr_flush(struct pb_manager *mgr) 
{
   /* No-op */
}


static void
vmw_gmr_bufmgr_destroy(struct pb_manager *_mgr) 
{
   struct vmw_gmr_bufmgr *mgr = vmw_gmr_bufmgr(_mgr);
   FREE(mgr);
}


struct pb_manager *
vmw_gmr_bufmgr_create(struct vmw_winsys_screen *vws) 
{
   struct vmw_gmr_bufmgr *mgr;
   
   mgr = CALLOC_STRUCT(vmw_gmr_bufmgr);
   if(!mgr)
      return NULL;

   mgr->base.destroy = vmw_gmr_bufmgr_destroy;
   mgr->base.create_buffer = vmw_gmr_bufmgr_create_buffer;
   mgr->base.flush = vmw_gmr_bufmgr_flush;
   
   mgr->vws = vws;
   
   return &mgr->base;
}


boolean
vmw_gmr_bufmgr_region_ptr(struct pb_buffer *buf, 
                          struct SVGAGuestPtr *ptr)
{
   struct pb_buffer *base_buf;
   unsigned offset = 0;
   struct vmw_gmr_buffer *gmr_buf;
   
   pb_get_base_buffer( buf, &base_buf, &offset );
   
   gmr_buf = vmw_gmr_buffer(base_buf);
   if(!gmr_buf)
      return FALSE;
   
   *ptr = vmw_ioctl_region_ptr(gmr_buf->region);
   
   ptr->offset += offset;
   
   return TRUE;
}
