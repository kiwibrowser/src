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
 *
 * Wrappers for DRM ioctl functionlaity used by the rest of the vmw
 * drm winsys.
 *
 * Based on svgaicd_escape.c
 */


#include "svga_cmd.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "svgadump/svga_dump.h"
#include "vmw_screen.h"
#include "vmw_context.h"
#include "vmw_fence.h"
#include "xf86drm.h"
#include "vmwgfx_drm.h"
#include "svga3d_caps.h"

#include "os/os_mman.h"

#include <errno.h>
#include <unistd.h>

struct vmw_region
{
   SVGAGuestPtr ptr;
   uint32_t handle;
   uint64_t map_handle;
   void *data;
   uint32_t map_count;
   int drm_fd;
   uint32_t size;
};

/* XXX: This isn't a real hardware flag, but just a hack for kernel to
 * know about primary surfaces. In newer versions of the kernel
 * interface the driver uses a special field.
 */
#define SVGA3D_SURFACE_HINT_SCANOUT (1 << 9)

uint32
vmw_ioctl_context_create(struct vmw_winsys_screen *vws)
{
   struct drm_vmw_context_arg c_arg;
   int ret;

   VMW_FUNC;

   ret = drmCommandRead(vws->ioctl.drm_fd, DRM_VMW_CREATE_CONTEXT,
			&c_arg, sizeof(c_arg));

   if (ret)
      return -1;

   vmw_printf("Context id is %d\n", c_arg.cid);

   return c_arg.cid;
}

void
vmw_ioctl_context_destroy(struct vmw_winsys_screen *vws, uint32 cid)
{
   struct drm_vmw_context_arg c_arg;

   VMW_FUNC;

   memset(&c_arg, 0, sizeof(c_arg));
   c_arg.cid = cid;

   (void)drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_UNREF_CONTEXT,
			 &c_arg, sizeof(c_arg));

}

uint32
vmw_ioctl_surface_create(struct vmw_winsys_screen *vws,
			      SVGA3dSurfaceFlags flags,
			      SVGA3dSurfaceFormat format,
			      SVGA3dSize size,
			      uint32_t numFaces, uint32_t numMipLevels)
{
   union drm_vmw_surface_create_arg s_arg;
   struct drm_vmw_surface_create_req *req = &s_arg.req;
   struct drm_vmw_surface_arg *rep = &s_arg.rep;
   struct drm_vmw_size sizes[DRM_VMW_MAX_SURFACE_FACES*
			     DRM_VMW_MAX_MIP_LEVELS];
   struct drm_vmw_size *cur_size;
   uint32_t iFace;
   uint32_t iMipLevel;
   int ret;

   vmw_printf("%s flags %d format %d\n", __FUNCTION__, flags, format);

   memset(&s_arg, 0, sizeof(s_arg));
   if (vws->use_old_scanout_flag &&
       (flags & SVGA3D_SURFACE_HINT_SCANOUT)) {
      req->flags = (uint32_t) flags;
      req->scanout = false;
   } else if (flags & SVGA3D_SURFACE_HINT_SCANOUT) {
      req->flags = (uint32_t) (flags & ~SVGA3D_SURFACE_HINT_SCANOUT);
      req->scanout = true;
   } else {
      req->flags = (uint32_t) flags;
      req->scanout = false;
   }
   req->format = (uint32_t) format;
   req->shareable = 1;

   assert(numFaces * numMipLevels < DRM_VMW_MAX_SURFACE_FACES*
	  DRM_VMW_MAX_MIP_LEVELS);
   cur_size = sizes;
   for (iFace = 0; iFace < numFaces; ++iFace) {
      SVGA3dSize mipSize = size;

      req->mip_levels[iFace] = numMipLevels;
      for (iMipLevel = 0; iMipLevel < numMipLevels; ++iMipLevel) {
	 cur_size->width = mipSize.width;
	 cur_size->height = mipSize.height;
	 cur_size->depth = mipSize.depth;
	 mipSize.width = MAX2(mipSize.width >> 1, 1);
	 mipSize.height = MAX2(mipSize.height >> 1, 1);
	 mipSize.depth = MAX2(mipSize.depth >> 1, 1);
	 cur_size++;
      }
   }
   for (iFace = numFaces; iFace < SVGA3D_MAX_SURFACE_FACES; ++iFace) {
      req->mip_levels[iFace] = 0;
   }

   req->size_addr = (unsigned long)&sizes;

   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_CREATE_SURFACE,
			     &s_arg, sizeof(s_arg));

   if (ret)
      return -1;

   vmw_printf("Surface id is %d\n", rep->sid);

   return rep->sid;
}

void
vmw_ioctl_surface_destroy(struct vmw_winsys_screen *vws, uint32 sid)
{
   struct drm_vmw_surface_arg s_arg;

   VMW_FUNC;

   memset(&s_arg, 0, sizeof(s_arg));
   s_arg.sid = sid;

   (void)drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_UNREF_SURFACE,
			 &s_arg, sizeof(s_arg));
}

void
vmw_ioctl_command(struct vmw_winsys_screen *vws, int32_t cid,
		  uint32_t throttle_us, void *commands, uint32_t size,
		  struct pipe_fence_handle **pfence)
{
   struct drm_vmw_execbuf_arg arg;
   struct drm_vmw_fence_rep rep;
   int ret;

#ifdef DEBUG
   {
      static boolean firsttime = TRUE;
      static boolean debug = FALSE;
      static boolean skip = FALSE;
      if (firsttime) {
         debug = debug_get_bool_option("SVGA_DUMP_CMD", FALSE);
         skip = debug_get_bool_option("SVGA_SKIP_CMD", FALSE);
      }
      if (debug) {
         VMW_FUNC;
         svga_dump_commands(commands, size);
      }
      firsttime = FALSE;
      if (skip) {
         size = 0;
      }
   }
#endif

   memset(&arg, 0, sizeof(arg));
   memset(&rep, 0, sizeof(rep));

   rep.error = -EFAULT;
   if (pfence)
      arg.fence_rep = (unsigned long)&rep;
   arg.commands = (unsigned long)commands;
   arg.command_size = size;
   arg.throttle_us = throttle_us;
   arg.version = DRM_VMW_EXECBUF_VERSION;

   do {
       ret = drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_EXECBUF, &arg, sizeof(arg));
   } while(ret == -ERESTART);
   if (ret) {
      debug_printf("%s error %s.\n", __FUNCTION__, strerror(-ret));
   }

   if (rep.error) {

      /*
       * Kernel has already synced, or caller requested no fence.
       */
      if (pfence)
	 *pfence = NULL;
   } else {
      if (pfence) {
	 *pfence = vmw_fence_create(rep.handle, rep.mask);

	 if (*pfence == NULL) {
	    /*
	     * Fence creation failed. Need to sync.
	     */
	    (void) vmw_ioctl_fence_finish(vws, rep.handle, rep.mask);
	    vmw_ioctl_fence_unref(vws, rep.handle);
	 }
      }
   }
}


struct vmw_region *
vmw_ioctl_region_create(struct vmw_winsys_screen *vws, uint32_t size)
{
   struct vmw_region *region;
   union drm_vmw_alloc_dmabuf_arg arg;
   struct drm_vmw_alloc_dmabuf_req *req = &arg.req;
   struct drm_vmw_dmabuf_rep *rep = &arg.rep;
   int ret;

   vmw_printf("%s: size = %u\n", __FUNCTION__, size);

   region = CALLOC_STRUCT(vmw_region);
   if (!region)
      goto out_err1;

   memset(&arg, 0, sizeof(arg));
   req->size = size;
   do {
      ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_ALLOC_DMABUF, &arg,
				sizeof(arg));
   } while (ret == -ERESTART);

   if (ret) {
      debug_printf("IOCTL failed %d: %s\n", ret, strerror(-ret));
      goto out_err1;
   }

   region->ptr.gmrId = rep->cur_gmr_id;
   region->ptr.offset = rep->cur_gmr_offset;
   region->data = NULL;
   region->handle = rep->handle;
   region->map_handle = rep->map_handle;
   region->map_count = 0;
   region->size = size;
   region->drm_fd = vws->ioctl.drm_fd;

   vmw_printf("   gmrId = %u, offset = %u\n",
              region->ptr.gmrId, region->ptr.offset);

   return region;

 out_err1:
   FREE(region);
   return NULL;
}

void
vmw_ioctl_region_destroy(struct vmw_region *region)
{
   struct drm_vmw_unref_dmabuf_arg arg;

   vmw_printf("%s: gmrId = %u, offset = %u\n", __FUNCTION__,
              region->ptr.gmrId, region->ptr.offset);

   if (region->data) {
      os_munmap(region->data, region->size);
      region->data = NULL;
   }

   memset(&arg, 0, sizeof(arg));
   arg.handle = region->handle;
   drmCommandWrite(region->drm_fd, DRM_VMW_UNREF_DMABUF, &arg, sizeof(arg));

   FREE(region);
}

SVGAGuestPtr
vmw_ioctl_region_ptr(struct vmw_region *region)
{
   return region->ptr;
}

void *
vmw_ioctl_region_map(struct vmw_region *region)
{
   void *map;

   vmw_printf("%s: gmrId = %u, offset = %u\n", __FUNCTION__,
              region->ptr.gmrId, region->ptr.offset);

   if (region->data == NULL) {
      map = os_mmap(NULL, region->size, PROT_READ | PROT_WRITE, MAP_SHARED,
		 region->drm_fd, region->map_handle);
      if (map == MAP_FAILED) {
	 debug_printf("%s: Map failed.\n", __FUNCTION__);
	 return NULL;
      }

      region->data = map;
   }

   ++region->map_count;

   return region->data;
}

void
vmw_ioctl_region_unmap(struct vmw_region *region)
{
   vmw_printf("%s: gmrId = %u, offset = %u\n", __FUNCTION__,
              region->ptr.gmrId, region->ptr.offset);
   --region->map_count;
}

void
vmw_ioctl_fence_unref(struct vmw_winsys_screen *vws,
		      uint32_t handle)
{
   struct drm_vmw_fence_arg arg;
   int ret;
   
   memset(&arg, 0, sizeof(arg));
   arg.handle = handle;

   ret = drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_FENCE_UNREF,
			 &arg, sizeof(arg));
   if (ret != 0)
      debug_printf("%s Failed\n", __FUNCTION__);
}

static INLINE uint32_t
vmw_drm_fence_flags(uint32_t flags)
{
    uint32_t dflags = 0;

    if (flags & SVGA_FENCE_FLAG_EXEC)
	dflags |= DRM_VMW_FENCE_FLAG_EXEC;
    if (flags & SVGA_FENCE_FLAG_QUERY)
	dflags |= DRM_VMW_FENCE_FLAG_QUERY;

    return dflags;
}


int
vmw_ioctl_fence_signalled(struct vmw_winsys_screen *vws,
			  uint32_t handle,
			  uint32_t flags)
{
   struct drm_vmw_fence_signaled_arg arg;
   uint32_t vflags = vmw_drm_fence_flags(flags);
   int ret;

   memset(&arg, 0, sizeof(arg));
   arg.handle = handle;
   arg.flags = vflags;

   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_FENCE_SIGNALED,
			     &arg, sizeof(arg));

   if (ret != 0)
      return ret;

   return (arg.signaled) ? 0 : -1;
}



int
vmw_ioctl_fence_finish(struct vmw_winsys_screen *vws,
                       uint32_t handle,
		       uint32_t flags)
{
   struct drm_vmw_fence_wait_arg arg;
   uint32_t vflags = vmw_drm_fence_flags(flags);
   int ret;

   memset(&arg, 0, sizeof(arg));

   arg.handle = handle;
   arg.timeout_us = 10*1000000;
   arg.lazy = 0;
   arg.flags = vflags;

   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_FENCE_WAIT,
			     &arg, sizeof(arg));

   if (ret != 0)
      debug_printf("%s Failed\n", __FUNCTION__);
   
   return 0;
}


boolean
vmw_ioctl_init(struct vmw_winsys_screen *vws)
{
   struct drm_vmw_getparam_arg gp_arg;
   struct drm_vmw_get_3d_cap_arg cap_arg;
   unsigned int size;
   int ret;

   VMW_FUNC;

   memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_3D;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
			     &gp_arg, sizeof(gp_arg));
   if (ret || gp_arg.value == 0) {
      debug_printf("No 3D enabled (%i, %s).\n", ret, strerror(-ret));
      goto out_no_3d;
   }

   memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_FIFO_HW_VERSION;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
			     &gp_arg, sizeof(gp_arg));
   if (ret) {
      debug_printf("Failed to get fifo hw version"
		   " (%i, %s).\n", ret, strerror(-ret));
      goto out_no_3d;
   }
   vws->ioctl.hwversion = gp_arg.value;

   size = SVGA_FIFO_3D_CAPS_SIZE * sizeof(uint32_t);
   vws->ioctl.buffer = calloc(1, size);
   if (!vws->ioctl.buffer) {
      debug_printf("Failed alloc fifo 3D caps buffer.\n");
      goto out_no_3d;
   }

   memset(&cap_arg, 0, sizeof(cap_arg));
   cap_arg.buffer = (uint64_t) (unsigned long) (vws->ioctl.buffer);
   cap_arg.max_size = size;

   ret = drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_GET_3D_CAP,
			 &cap_arg, sizeof(cap_arg));

   if (ret) {
      debug_printf("Failed to get 3D capabilities"
		   " (%i, %s).\n", ret, strerror(-ret));
      goto out_no_caps;
   }

   vmw_printf("%s OK\n", __FUNCTION__);
   return TRUE;
  out_no_caps:
   free(vws->ioctl.buffer);
  out_no_3d:
   debug_printf("%s Failed\n", __FUNCTION__);
   return FALSE;
}



void
vmw_ioctl_cleanup(struct vmw_winsys_screen *vws)
{
   VMW_FUNC;
}
