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

#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"
#include "util/u_inlines.h"
#include "pipe/p_state.h"

#include "svga_context.h"
#include "svga_swtnl.h"
#include "svga_state.h"
#include "svga_swtnl_private.h"



enum pipe_error
svga_swtnl_draw_vbo(struct svga_context *svga,
                    const struct pipe_draw_info *info)
{
   struct pipe_transfer *vb_transfer[PIPE_MAX_ATTRIBS];
   struct pipe_transfer *ib_transfer = NULL;
   struct pipe_transfer *cb_transfer = NULL;
   struct draw_context *draw = svga->swtnl.draw;
   unsigned i;
   const void *map;
   enum pipe_error ret;

   assert(!svga->dirty);
   assert(svga->state.sw.need_swtnl);
   assert(draw);

   /* Make sure that the need_swtnl flag does not go away */
   svga->state.sw.in_swtnl_draw = TRUE;

   ret = svga_update_state(svga, SVGA_STATE_SWTNL_DRAW);
   if (ret != PIPE_OK) {
      svga_context_flush(svga, NULL);
      ret = svga_update_state(svga, SVGA_STATE_SWTNL_DRAW);
      svga->swtnl.new_vbuf = TRUE;
      assert(ret == PIPE_OK);
   }

   /*
    * Map vertex buffers
    */
   for (i = 0; i < svga->curr.num_vertex_buffers; i++) {
      if (svga->curr.vb[i].buffer) {
         map = pipe_buffer_map(&svga->pipe,
                               svga->curr.vb[i].buffer,
                               PIPE_TRANSFER_READ,
                               &vb_transfer[i]);

         draw_set_mapped_vertex_buffer(draw, i, map);
      }
   }

   /* Map index buffer, if present */
   map = NULL;
   if (info->indexed && svga->curr.ib.buffer) {
      map = pipe_buffer_map(&svga->pipe, svga->curr.ib.buffer,
                            PIPE_TRANSFER_READ,
                            &ib_transfer);
      draw_set_indexes(draw,
                       (const ubyte *) map + svga->curr.ib.offset,
                       svga->curr.ib.index_size);
   }

   if (svga->curr.cb[PIPE_SHADER_VERTEX]) {
      map = pipe_buffer_map(&svga->pipe,
                            svga->curr.cb[PIPE_SHADER_VERTEX],
                            PIPE_TRANSFER_READ,
			    &cb_transfer);
      assert(map);
      draw_set_mapped_constant_buffer(
         draw, PIPE_SHADER_VERTEX, 0,
         map,
         svga->curr.cb[PIPE_SHADER_VERTEX]->width0);
   }

   draw_vbo(draw, info);

   draw_flush(svga->swtnl.draw);

   /* Ensure the draw module didn't touch this */
   assert(i == svga->curr.num_vertex_buffers);
   
   /*
    * unmap vertex/index buffers
    */
   for (i = 0; i < svga->curr.num_vertex_buffers; i++) {
      if (svga->curr.vb[i].buffer) {
         pipe_buffer_unmap(&svga->pipe, vb_transfer[i]);
         draw_set_mapped_vertex_buffer(draw, i, NULL);
      }
   }

   if (ib_transfer) {
      pipe_buffer_unmap(&svga->pipe, ib_transfer);
      draw_set_indexes(draw, NULL, 0);
   }

   if (svga->curr.cb[PIPE_SHADER_VERTEX]) {
      pipe_buffer_unmap(&svga->pipe, cb_transfer);
   }

   /* Now safe to remove the need_swtnl flag in any update_state call */
   svga->state.sw.in_swtnl_draw = FALSE;
   svga->dirty |= SVGA_NEW_NEED_PIPELINE | SVGA_NEW_NEED_SWVFETCH;

   return ret;
}




boolean svga_init_swtnl( struct svga_context *svga )
{
   svga->swtnl.backend = svga_vbuf_render_create(svga);
   if(!svga->swtnl.backend)
      goto fail;

   /*
    * Create drawing context and plug our rendering stage into it.
    */
   svga->swtnl.draw = draw_create(&svga->pipe);
   if (svga->swtnl.draw == NULL)
      goto fail;


   draw_set_rasterize_stage(svga->swtnl.draw, 
                            draw_vbuf_stage( svga->swtnl.draw, svga->swtnl.backend ));

   draw_set_render(svga->swtnl.draw, svga->swtnl.backend);

   draw_install_aaline_stage(svga->swtnl.draw, &svga->pipe);
   draw_install_aapoint_stage(svga->swtnl.draw, &svga->pipe);
   draw_install_pstipple_stage(svga->swtnl.draw, &svga->pipe);

   if (debug_get_bool_option("SVGA_SWTNL_FSE", FALSE))
      draw_set_driver_clipping(svga->swtnl.draw, TRUE, TRUE, TRUE);

   return TRUE;

fail:
   if (svga->swtnl.backend)
      svga->swtnl.backend->destroy( svga->swtnl.backend );

   if (svga->swtnl.draw)
      draw_destroy( svga->swtnl.draw );

   return FALSE;
}


void svga_destroy_swtnl( struct svga_context *svga )
{
   draw_destroy( svga->swtnl.draw );
}
