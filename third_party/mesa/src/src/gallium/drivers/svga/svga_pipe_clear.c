/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
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

#include "svga_cmd.h"
#include "svga_debug.h"

#include "pipe/p_defines.h"
#include "util/u_pack_color.h"

#include "svga_context.h"
#include "svga_state.h"
#include "svga_surface.h"


static enum pipe_error
try_clear(struct svga_context *svga, 
          unsigned buffers,
          const union pipe_color_union *color,
          double depth,
          unsigned stencil)
{
   enum pipe_error ret = PIPE_OK;
   SVGA3dRect rect = { 0, 0, 0, 0 };
   boolean restore_viewport = FALSE;
   SVGA3dClearFlag flags = 0;
   struct pipe_framebuffer_state *fb = &svga->curr.framebuffer;
   union util_color uc = {0};

   ret = svga_update_state(svga, SVGA_STATE_HW_CLEAR);
   if (ret != PIPE_OK)
      return ret;

   if (svga->rebind.rendertargets) {
      ret = svga_reemit_framebuffer_bindings(svga);
      if (ret != PIPE_OK) {
         return ret;
      }
   }

   if ((buffers & PIPE_CLEAR_COLOR) && fb->cbufs[0]) {
      flags |= SVGA3D_CLEAR_COLOR;
      util_pack_color(color->f, PIPE_FORMAT_B8G8R8A8_UNORM, &uc);

      rect.w = fb->cbufs[0]->width;
      rect.h = fb->cbufs[0]->height;
   }

   if ((buffers & PIPE_CLEAR_DEPTHSTENCIL) && fb->zsbuf) {
      if (buffers & PIPE_CLEAR_DEPTH)
         flags |= SVGA3D_CLEAR_DEPTH;

      if ((svga->curr.framebuffer.zsbuf->format == PIPE_FORMAT_S8_UINT_Z24_UNORM) &&
          (buffers & PIPE_CLEAR_STENCIL))
         flags |= SVGA3D_CLEAR_STENCIL;

      rect.w = MAX2(rect.w, fb->zsbuf->width);
      rect.h = MAX2(rect.h, fb->zsbuf->height);
   }

   if (memcmp(&rect, &svga->state.hw_clear.viewport, sizeof(rect)) != 0) {
      restore_viewport = TRUE;
      ret = SVGA3D_SetViewport(svga->swc, &rect);
      if (ret != PIPE_OK)
         return ret;
   }

   ret = SVGA3D_ClearRect(svga->swc, flags, uc.ui, depth, stencil,
                          rect.x, rect.y, rect.w, rect.h);
   if (ret != PIPE_OK)
      return ret;

   if (restore_viewport) {
      memcpy(&rect, &svga->state.hw_clear.viewport, sizeof rect);
      ret = SVGA3D_SetViewport(svga->swc, &rect);
   }
   
   return ret;
}

/**
 * Clear the given surface to the specified value.
 * No masking, no scissor (clear entire buffer).
 */
void
svga_clear(struct pipe_context *pipe, unsigned buffers,
           const union pipe_color_union *color,
	   double depth, unsigned stencil)
{
   struct svga_context *svga = svga_context( pipe );
   enum pipe_error ret;

   if (buffers & PIPE_CLEAR_COLOR)
      SVGA_DBG(DEBUG_DMA, "clear sid %p\n",
               svga_surface(svga->curr.framebuffer.cbufs[0])->handle);

   /* flush any queued prims (don't want them to appear after the clear!) */
   svga_hwtnl_flush_retry(svga);

   ret = try_clear( svga, buffers, color, depth, stencil );

   if (ret == PIPE_ERROR_OUT_OF_MEMORY) {
      /* Flush command buffer and retry:
       */
      svga_context_flush( svga, NULL );

      ret = try_clear( svga, buffers, color, depth, stencil );
   }

   /*
    * Mark target surfaces as dirty
    * TODO Mark only cleared surfaces.
    */
   svga_mark_surfaces_dirty(svga);

   assert (ret == PIPE_OK);
}
