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

#include "util/u_inlines.h"
#include "pipe/p_defines.h"
#include "util/u_math.h"
#include "util/u_upload_mgr.h"

#include "svga_context.h"
#include "svga_state.h"
#include "svga_draw.h"
#include "svga_tgsi.h"
#include "svga_screen.h"
#include "svga_resource_buffer.h"

#include "svga_hw_reg.h"


/***********************************************************************
 */


static enum pipe_error
emit_hw_vs_vdecl(struct svga_context *svga, unsigned dirty)
{
   const struct pipe_vertex_element *ve = svga->curr.velems->velem;
   SVGA3dVertexDecl decl;
   unsigned i;
   unsigned neg_bias = 0;

   assert(svga->curr.velems->count >=
          svga->curr.vs->base.info.file_count[TGSI_FILE_INPUT]);

   svga_hwtnl_reset_vdecl( svga->hwtnl, 
                           svga->curr.velems->count );

   /**
    * We can't set the VDECL offset to something negative, so we
    * must calculate a common negative additional index bias, and modify
    * the VDECL offsets accordingly so they *all* end up positive.
    *
    * Note that the exact value of the negative index bias is not that
    * important, since we compensate for it when we calculate the vertex
    * buffer offset below. The important thing is that all vertex buffer
    * offsets remain positive.
    *
    * Note that we use a negative bias variable in order to make the
    * rounding maths more easy to follow, and to avoid int / unsigned
    * confusion.
    */

   for (i = 0; i < svga->curr.velems->count; i++) {
      const struct pipe_vertex_buffer *vb =
         &svga->curr.vb[ve[i].vertex_buffer_index];
      struct svga_buffer *buffer;
      unsigned int offset = vb->buffer_offset + ve[i].src_offset;
      unsigned tmp_neg_bias = 0;

      if (!vb->buffer)
         continue;

      buffer = svga_buffer(vb->buffer);
      if (buffer->uploaded.start > offset) {
         tmp_neg_bias = buffer->uploaded.start - offset;
         if (vb->stride)
            tmp_neg_bias = (tmp_neg_bias + vb->stride - 1) / vb->stride;
         neg_bias = MAX2(neg_bias, tmp_neg_bias);
      }
   }

   for (i = 0; i < svga->curr.velems->count; i++) {
      const struct pipe_vertex_buffer *vb =
         &svga->curr.vb[ve[i].vertex_buffer_index];
      unsigned usage, index;
      struct svga_buffer *buffer;

      if (!vb->buffer)
         continue;

      buffer= svga_buffer(vb->buffer);
      svga_generate_vdecl_semantics( i, &usage, &index );

      /* SVGA_NEW_VELEMENT
       */
      decl.identity.type = svga->state.sw.ve_format[i];
      decl.identity.method = SVGA3D_DECLMETHOD_DEFAULT;
      decl.identity.usage = usage;
      decl.identity.usageIndex = index;
      decl.array.stride = vb->stride;

      /* Compensate for partially uploaded vbo, and
       * for the negative index bias.
       */
      decl.array.offset = (vb->buffer_offset
                           + ve[i].src_offset
			   + neg_bias * vb->stride
			   - buffer->uploaded.start);

      assert(decl.array.offset >= 0);

      svga_hwtnl_vdecl( svga->hwtnl,
                        i,
                        &decl,
                        buffer->uploaded.buffer ? buffer->uploaded.buffer :
                        vb->buffer );
   }

   svga_hwtnl_set_index_bias( svga->hwtnl, -neg_bias );
   return PIPE_OK;
}


static enum pipe_error
emit_hw_vdecl(struct svga_context *svga, unsigned dirty)
{
   /* SVGA_NEW_NEED_SWTNL
    */
   if (svga->state.sw.need_swtnl)
      return PIPE_OK; /* Do not emit during swtnl */

   return emit_hw_vs_vdecl( svga, dirty );
}


struct svga_tracked_state svga_hw_vdecl = 
{
   "hw vertex decl state (hwtnl version)",
   ( SVGA_NEW_NEED_SWTNL |
     SVGA_NEW_VELEMENT |
     SVGA_NEW_VBUFFER |
     SVGA_NEW_RAST |
     SVGA_NEW_FS |
     SVGA_NEW_VS ),
   emit_hw_vdecl
};






