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

#include "pipe/p_defines.h"
#include "util/u_string.h"
#include "svga_screen.h"
#include "svga_surface.h"
#include "svga_context.h"
#include "svga_debug.h"


static void svga_flush( struct pipe_context *pipe,
                        struct pipe_fence_handle **fence )
{
   struct svga_context *svga = svga_context(pipe);

   /* Emit buffered drawing commands, and any back copies.
    */
   svga_surfaces_flush( svga );

   /* Flush command queue.
    */
   svga_context_flush(svga, fence);

   SVGA_DBG(DEBUG_DMA|DEBUG_PERF, "%s fence_ptr %p\n",
            __FUNCTION__, fence ? *fence : 0x0);

   /* Enable to dump BMPs of the color/depth buffers each frame */
   if (0) {
      struct pipe_framebuffer_state *fb = &svga->curr.framebuffer;
      static unsigned frame_no = 1;
      char filename[256];
      unsigned i;

      for (i = 0; i < fb->nr_cbufs; i++) {
         util_snprintf(filename, sizeof(filename), "cbuf%u_%04u", i, frame_no);
         debug_dump_surface_bmp(&svga->pipe, filename, fb->cbufs[i]);
      }

      if (0 && fb->zsbuf) {
         util_snprintf(filename, sizeof(filename), "zsbuf_%04u", frame_no);
         debug_dump_surface_bmp(&svga->pipe, filename, fb->zsbuf);
      }

      ++frame_no;
   }
}


void svga_init_flush_functions( struct svga_context *svga )
{
   svga->pipe.flush = svga_flush;
}
