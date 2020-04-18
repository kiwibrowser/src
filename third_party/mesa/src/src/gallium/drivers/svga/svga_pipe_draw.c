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

#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_prim.h"
#include "util/u_time.h"
#include "indices/u_indices.h"

#include "svga_hw_reg.h"
#include "svga_context.h"
#include "svga_screen.h"
#include "svga_draw.h"
#include "svga_state.h"
#include "svga_swtnl.h"
#include "svga_debug.h"
#include "svga_resource_buffer.h"
#include "util/u_upload_mgr.h"

/**
 * Determine the ranges to upload for the user-buffers referenced
 * by the next draw command.
 *
 * TODO: It might be beneficial to support multiple ranges. In that case,
 * the struct svga_buffer::uploaded member should be made an array or a
 * list, since we need to account for the possibility that different ranges
 * may be uploaded to different hardware buffers chosen by the utility
 * upload manager.
 */

static void
svga_user_buffer_range(struct svga_context *svga,
                       unsigned start,
                       unsigned count,
                       unsigned instance_count)
{
   const struct pipe_vertex_element *ve = svga->curr.velems->velem;
   int i;

   /*
    * Release old uploaded range (if not done already) and
    * initialize new ranges.
    */

   for (i=0; i < svga->curr.velems->count; i++) {
      struct pipe_vertex_buffer *vb =
         &svga->curr.vb[ve[i].vertex_buffer_index];

      if (vb->buffer && svga_buffer_is_user_buffer(vb->buffer)) {
         struct svga_buffer *buffer = svga_buffer(vb->buffer);

         pipe_resource_reference(&buffer->uploaded.buffer, NULL);
         buffer->uploaded.start = ~0;
         buffer->uploaded.end = 0;
      }
   }

   for (i=0; i < svga->curr.velems->count; i++) {
      struct pipe_vertex_buffer *vb =
         &svga->curr.vb[ve[i].vertex_buffer_index];

      if (vb->buffer && svga_buffer_is_user_buffer(vb->buffer)) {
         struct svga_buffer *buffer = svga_buffer(vb->buffer);
         unsigned first, size;
         unsigned instance_div = ve[i].instance_divisor;
         unsigned elemSize = util_format_get_blocksize(ve[i].src_format);

         svga->dirty |= SVGA_NEW_VBUFFER;

         if (instance_div) {
            first = ve[i].src_offset;
            count = (instance_count + instance_div - 1) / instance_div;
            size = vb->stride * (count - 1) + elemSize;
         } else {
            first = vb->stride * start + ve[i].src_offset;
            size = vb->stride * (count - 1) + elemSize;
         }

         buffer->uploaded.start = MIN2(buffer->uploaded.start, first);
         buffer->uploaded.end = MAX2(buffer->uploaded.end, first + size);
      }
   }
}

/**
 * svga_upload_user_buffers - upload parts of user buffers
 *
 * This function streams a part of a user buffer to hw and fills
 * svga_buffer::uploaded with information on the upload.
 */

static int
svga_upload_user_buffers(struct svga_context *svga,
                         unsigned start,
                         unsigned count,
                         unsigned instance_count)
{
   const struct pipe_vertex_element *ve = svga->curr.velems->velem;
   unsigned i;
   int ret;

   svga_user_buffer_range(svga, start, count, instance_count);

   for (i=0; i < svga->curr.velems->count; i++) {
      struct pipe_vertex_buffer *vb =
         &svga->curr.vb[ve[i].vertex_buffer_index];

      if (vb->buffer && svga_buffer_is_user_buffer(vb->buffer)) {
         struct svga_buffer *buffer = svga_buffer(vb->buffer);

         /*
          * Check if already uploaded. Otherwise go ahead and upload.
          */

         if (buffer->uploaded.buffer)
            continue;

         ret = u_upload_buffer( svga->upload_vb,
                                0,
                                buffer->uploaded.start,
                                buffer->uploaded.end - buffer->uploaded.start,
                                &buffer->b.b,
                                &buffer->uploaded.offset,
                                &buffer->uploaded.buffer);

         if (ret)
            return ret;

         if (0)
            debug_printf("%s: %d: orig buf %p upl buf %p ofs %d sofs %d"
                         " sz %d\n",
                         __FUNCTION__,
                         i,
                         buffer,
                         buffer->uploaded.buffer,
                         buffer->uploaded.offset,
                         buffer->uploaded.start,
                         buffer->uploaded.end - buffer->uploaded.start);

         vb->buffer_offset = buffer->uploaded.offset;
      }
   }

   return PIPE_OK;
}

/**
 * svga_release_user_upl_buffers - release uploaded parts of user buffers
 *
 * This function releases the hw copy of the uploaded fraction of the
 * user-buffer. It's important to do this as soon as all draw calls
 * affecting the uploaded fraction are issued, as this allows for
 * efficient reuse of the hardware surface backing the uploaded fraction.
 *
 * svga_buffer::source_offset is set to 0, and svga_buffer::uploaded::buffer
 * is set to 0.
 */

static void
svga_release_user_upl_buffers(struct svga_context *svga)
{
   unsigned i;
   unsigned nr;

   nr = svga->curr.num_vertex_buffers;

   for (i = 0; i < nr; ++i) {
      struct pipe_vertex_buffer *vb = &svga->curr.vb[i];

      if (vb->buffer && svga_buffer_is_user_buffer(vb->buffer)) {
         struct svga_buffer *buffer = svga_buffer(vb->buffer);

         /* The buffer_offset is relative to the uploaded buffer.
          * Since we're discarding that buffer we need to reset this offset
          * so it's not inadvertantly applied to a subsequent draw.
          *
          * XXX a root problem here is that the svga->curr.vb[] information
          * is getting set both by gallium API calls and by code in
          * svga_upload_user_buffers().  We should instead have two copies
          * of the vertex buffer information and choose between as needed.
          */
         vb->buffer_offset = 0;

         buffer->uploaded.start = ~0;
         buffer->uploaded.end = 0;
         if (buffer->uploaded.buffer)
            pipe_resource_reference(&buffer->uploaded.buffer, NULL);
      }
   }
}



static enum pipe_error
retry_draw_range_elements( struct svga_context *svga,
                           struct pipe_resource *index_buffer,
                           unsigned index_size,
                           int index_bias,
                           unsigned min_index,
                           unsigned max_index,
                           unsigned prim, 
                           unsigned start, 
                           unsigned count,
                           unsigned instance_count,
                           boolean do_retry )
{
   enum pipe_error ret = PIPE_OK;

   svga_hwtnl_set_unfilled( svga->hwtnl,
                            svga->curr.rast->hw_unfilled );

   svga_hwtnl_set_flatshade( svga->hwtnl,
                             svga->curr.rast->templ.flatshade,
                             svga->curr.rast->templ.flatshade_first );

   ret = svga_upload_user_buffers( svga, min_index + index_bias,
                                   max_index - min_index + 1, instance_count );
   if (ret != PIPE_OK)
      goto retry;

   ret = svga_update_state( svga, SVGA_STATE_HW_DRAW );
   if (ret != PIPE_OK)
      goto retry;

   ret = svga_hwtnl_draw_range_elements( svga->hwtnl,
                                         index_buffer, index_size, index_bias,
                                         min_index, max_index,
                                         prim, start, count );
   if (ret != PIPE_OK)
      goto retry;

   return PIPE_OK;

retry:
   svga_context_flush( svga, NULL );

   if (do_retry)
   {
      return retry_draw_range_elements( svga,
                                        index_buffer, index_size, index_bias,
                                        min_index, max_index,
                                        prim, start, count,
                                        instance_count, FALSE );
   }

   return ret;
}


static enum pipe_error
retry_draw_arrays( struct svga_context *svga,
                   unsigned prim, 
                   unsigned start, 
                   unsigned count,
                   unsigned instance_count,
                   boolean do_retry )
{
   enum pipe_error ret;

   svga_hwtnl_set_unfilled( svga->hwtnl,
                            svga->curr.rast->hw_unfilled );

   svga_hwtnl_set_flatshade( svga->hwtnl,
                             svga->curr.rast->templ.flatshade,
                             svga->curr.rast->templ.flatshade_first );

   ret = svga_upload_user_buffers( svga, start, count, instance_count );

   if (ret != PIPE_OK)
      goto retry;

   ret = svga_update_state( svga, SVGA_STATE_HW_DRAW );
   if (ret != PIPE_OK)
      goto retry;

   ret = svga_hwtnl_draw_arrays( svga->hwtnl, prim,
                                 start, count );
   if (ret != PIPE_OK)
      goto retry;

   return PIPE_OK;

retry:
   if (ret == PIPE_ERROR_OUT_OF_MEMORY && do_retry) 
   {
      svga_context_flush( svga, NULL );

      return retry_draw_arrays( svga,
                                prim,
                                start,
                                count,
                                instance_count,
                                FALSE );
   }

   return ret;
}


static void
svga_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
   struct svga_context *svga = svga_context( pipe );
   unsigned reduced_prim = u_reduced_prim( info->mode );
   unsigned count = info->count;
   enum pipe_error ret = 0;
   boolean needed_swtnl;

   if (!u_trim_pipe_prim( info->mode, &count ))
      return;

   /*
    * Mark currently bound target surfaces as dirty
    * doesn't really matter if it is done before drawing.
    *
    * TODO If we ever normaly return something other then
    * true we should not mark it as dirty then.
    */
   svga_mark_surfaces_dirty(svga_context(pipe));

   if (svga->curr.reduced_prim != reduced_prim) {
      svga->curr.reduced_prim = reduced_prim;
      svga->dirty |= SVGA_NEW_REDUCED_PRIMITIVE;
   }
   
   needed_swtnl = svga->state.sw.need_swtnl;

   svga_update_state_retry( svga, SVGA_STATE_NEED_SWTNL );

#ifdef DEBUG
   if (svga->curr.vs->base.id == svga->debug.disable_shader ||
       svga->curr.fs->base.id == svga->debug.disable_shader)
      return;
#endif

   if (svga->state.sw.need_swtnl) {
      if (!needed_swtnl) {
         /*
          * We're switching from HW to SW TNL.  SW TNL will require mapping all
          * currently bound vertex buffers, some of which may already be
          * referenced in the current command buffer as result of previous HW
          * TNL. So flush now, to prevent the context to flush while a referred
          * vertex buffer is mapped.
          */

         svga_context_flush(svga, NULL);
      }

      /* Avoid leaking the previous hwtnl bias to swtnl */
      svga_hwtnl_set_index_bias( svga->hwtnl, 0 );
      ret = svga_swtnl_draw_vbo( svga, info );
   }
   else {
      if (info->indexed && svga->curr.ib.buffer) {
         unsigned offset;

         assert(svga->curr.ib.offset % svga->curr.ib.index_size == 0);
         offset = svga->curr.ib.offset / svga->curr.ib.index_size;

         ret = retry_draw_range_elements( svga,
                                          svga->curr.ib.buffer,
                                          svga->curr.ib.index_size,
                                          info->index_bias,
                                          info->min_index,
                                          info->max_index,
                                          info->mode,
                                          info->start + offset,
                                          info->count,
                                          info->instance_count,
                                          TRUE );
      }
      else {
         ret = retry_draw_arrays( svga,
                                  info->mode,
                                  info->start,
                                  info->count,
                                  info->instance_count,
                                  TRUE );
      }
   }

   /* XXX: Silence warnings, do something sensible here? */
   (void)ret;

   svga_release_user_upl_buffers( svga );

   if (SVGA_DEBUG & DEBUG_FLUSH) {
      svga_hwtnl_flush_retry( svga );
      svga_context_flush(svga, NULL);
   }
}


void svga_init_draw_functions( struct svga_context *svga )
{
   svga->pipe.draw_vbo = svga_draw_vbo;
}
