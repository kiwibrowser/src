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


#include "pipe/p_compiler.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_format.h"
#include "vmw_screen.h"

#include "vmw_surface.h"
#include "svga_drm_public.h"

#include "state_tracker/drm_driver.h"

#include "vmwgfx_drm.h"
#include <xf86drm.h>

#include <stdio.h>

struct dri1_api_version {
   int major;
   int minor;
   int patch_level;
};

static struct svga_winsys_surface *
vmw_drm_surface_from_handle(struct svga_winsys_screen *sws,
			    struct winsys_handle *whandle,
			    SVGA3dSurfaceFormat *format);
static boolean
vmw_drm_surface_get_handle(struct svga_winsys_screen *sws,
			   struct svga_winsys_surface *surface,
			   unsigned stride,
			   struct winsys_handle *whandle);

static struct dri1_api_version drm_required = { 2, 1, 0 };
static struct dri1_api_version drm_compat = { 2, 0, 0 };

static boolean
vmw_dri1_check_version(const struct dri1_api_version *cur,
		       const struct dri1_api_version *required,
		       const struct dri1_api_version *compat,
		       const char component[])
{
   if (cur->major > required->major && cur->major <= compat->major)
      return TRUE;
   if (cur->major == required->major && cur->minor >= required->minor)
      return TRUE;

   fprintf(stderr, "%s version failure.\n", component);
   fprintf(stderr, "%s version is %d.%d.%d and this driver can only work\n"
	   "with versions %d.%d.x through %d.x.x.\n",
	   component,
	   cur->major,
	   cur->minor,
	   cur->patch_level, required->major, required->minor, compat->major);
   return FALSE;
}

/* This is actually the entrypoint to the entire driver,
 * called by the target bootstrap code.
 */
struct svga_winsys_screen *
svga_drm_winsys_screen_create(int fd)
{
   struct vmw_winsys_screen *vws;
   struct dri1_api_version drm_ver;
   drmVersionPtr ver;

   ver = drmGetVersion(fd);
   if (ver == NULL)
      return NULL;

   drm_ver.major = ver->version_major;
   drm_ver.minor = ver->version_minor;
   drm_ver.patch_level = 0; /* ??? */

   drmFreeVersion(ver);
   if (!vmw_dri1_check_version(&drm_ver, &drm_required,
			       &drm_compat, "vmwgfx drm driver"))
      return NULL;

   vws = vmw_winsys_create( fd, FALSE );
   if (!vws)
      goto out_no_vws;

   /* XXX do this properly */
   vws->base.surface_from_handle = vmw_drm_surface_from_handle;
   vws->base.surface_get_handle = vmw_drm_surface_get_handle;

   return &vws->base;

out_no_vws:
   return NULL;
}

static INLINE boolean
vmw_dri1_intersect_src_bbox(struct drm_clip_rect *dst,
			    int dst_x,
			    int dst_y,
			    const struct drm_clip_rect *src,
			    const struct drm_clip_rect *bbox)
{
   int xy1;
   int xy2;

   xy1 = ((int)src->x1 > (int)bbox->x1 + dst_x) ? src->x1 :
      (int)bbox->x1 + dst_x;
   xy2 = ((int)src->x2 < (int)bbox->x2 + dst_x) ? src->x2 :
      (int)bbox->x2 + dst_x;
   if (xy1 >= xy2 || xy1 < 0)
      return FALSE;

   dst->x1 = xy1;
   dst->x2 = xy2;

   xy1 = ((int)src->y1 > (int)bbox->y1 + dst_y) ? src->y1 :
      (int)bbox->y1 + dst_y;
   xy2 = ((int)src->y2 < (int)bbox->y2 + dst_y) ? src->y2 :
      (int)bbox->y2 + dst_y;
   if (xy1 >= xy2 || xy1 < 0)
      return FALSE;

   dst->y1 = xy1;
   dst->y2 = xy2;
   return TRUE;
}

static struct svga_winsys_surface *
vmw_drm_surface_from_handle(struct svga_winsys_screen *sws,
			    struct winsys_handle *whandle,
			    SVGA3dSurfaceFormat *format)
{
    struct vmw_svga_winsys_surface *vsrf;
    struct svga_winsys_surface *ssrf;
    struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);
    union drm_vmw_surface_reference_arg arg;
    struct drm_vmw_surface_arg *req = &arg.req;
    struct drm_vmw_surface_create_req *rep = &arg.rep;
    int ret;
    int i;

    /**
     * The vmware device specific handle is the hardware SID.
     * FIXME: We probably want to move this to the ioctl implementations.
     */

    memset(&arg, 0, sizeof(arg));
    req->sid = whandle->handle;

    ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_REF_SURFACE,
			      &arg, sizeof(arg));

    if (ret) {
	fprintf(stderr, "Failed referencing shared surface. SID %d.\n"
		"Error %d (%s).\n",
		whandle->handle, ret, strerror(-ret));
	return NULL;
    }

    if (rep->mip_levels[0] != 1) {
	fprintf(stderr, "Incorrect number of mipmap levels on shared surface."
		" SID %d, levels %d\n",
		whandle->handle, rep->mip_levels[0]);
	goto out_mip;
    }

    for (i=1; i < DRM_VMW_MAX_SURFACE_FACES; ++i) {
	if (rep->mip_levels[i] != 0) {
	    fprintf(stderr, "Incorrect number of faces levels on shared surface."
		    " SID %d, face %d present.\n",
		    whandle->handle, i);
	    goto out_mip;
	}
   }

    vsrf = CALLOC_STRUCT(vmw_svga_winsys_surface);
    if (!vsrf)
	goto out_mip;

    pipe_reference_init(&vsrf->refcnt, 1);
    p_atomic_set(&vsrf->validated, 0);
    vsrf->screen = vws;
    vsrf->sid = whandle->handle;
    ssrf = svga_winsys_surface(vsrf);
    *format = rep->format;

    return ssrf;

out_mip:
    vmw_ioctl_surface_destroy(vws, whandle->handle);
    return NULL;
}

static boolean
vmw_drm_surface_get_handle(struct svga_winsys_screen *sws,
			   struct svga_winsys_surface *surface,
			   unsigned stride,
			   struct winsys_handle *whandle)
{
    struct vmw_svga_winsys_surface *vsrf;

    if (!surface)
	return FALSE;

    vsrf = vmw_svga_winsys_surface(surface);
    whandle->handle = vsrf->sid;
    whandle->stride = stride;

    return TRUE;
}
