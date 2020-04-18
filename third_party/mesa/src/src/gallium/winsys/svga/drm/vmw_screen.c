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

#include "vmw_context.h"

#include "util/u_memory.h"
#include "pipe/p_compiler.h"


/* Called from vmw_drm_create_screen(), creates and initializes the
 * vmw_winsys_screen structure, which is the main entity in this
 * module.
 */
struct vmw_winsys_screen *
vmw_winsys_create( int fd, boolean use_old_scanout_flag )
{
   struct vmw_winsys_screen *vws = CALLOC_STRUCT(vmw_winsys_screen);
   if (!vws)
      goto out_no_vws;

   vws->ioctl.drm_fd = fd;
   vws->use_old_scanout_flag = use_old_scanout_flag;

   if (!vmw_ioctl_init(vws))
      goto out_no_ioctl;

   if(!vmw_pools_init(vws))
      goto out_no_pools;

   if (!vmw_winsys_screen_init_svga(vws))
      goto out_no_svga;

   return vws;
out_no_svga:
   vmw_pools_cleanup(vws);
out_no_pools:
   vmw_ioctl_cleanup(vws);
out_no_ioctl:
   FREE(vws);
out_no_vws:
   return NULL;
}

void
vmw_winsys_destroy(struct vmw_winsys_screen *vws)
{
   vmw_pools_cleanup(vws);
   vmw_ioctl_cleanup(vws);
   FREE(vws);
}
