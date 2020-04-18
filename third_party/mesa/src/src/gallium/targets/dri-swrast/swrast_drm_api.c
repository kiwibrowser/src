/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
 * All Rights Reserved.
 * Copyright 2010 George Sapountzis <gsapountzis@gmail.com>
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

#include "pipe/p_compiler.h"
#include "util/u_memory.h"
#include "dri_sw_winsys.h"

#include "target-helpers/inline_debug_helper.h"
#include "target-helpers/inline_sw_helper.h"

#include "state_tracker/drm_driver.h"

DRM_DRIVER_DESCRIPTOR("swrast", NULL, NULL, NULL);

struct pipe_screen *
drisw_create_screen(struct drisw_loader_funcs *lf)
{
   struct sw_winsys *winsys = NULL;
   struct pipe_screen *screen = NULL;

   winsys = dri_create_sw_winsys(lf);
   if (winsys == NULL)
      return NULL;

   screen = sw_screen_create(winsys);
   if (!screen)
      goto fail;

   screen = debug_screen_wrap(screen);

   return screen;

fail:
   if (winsys)
      winsys->destroy(winsys);

   return NULL;
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
