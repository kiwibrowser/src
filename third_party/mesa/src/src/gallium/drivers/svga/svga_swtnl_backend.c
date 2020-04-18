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

#include "draw/draw_vbuf.h"
#include "draw/draw_context.h"
#include "draw/draw_vertex.h"

#include "util/u_debug.h"
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "svga_context.h"
#include "svga_state.h"
#include "svga_swtnl.h"

#include "svga_types.h"
#include "svga_reg.h"
#include "svga3d_reg.h"
#include "svga_draw.h"
#include "svga_swtnl_private.h"


static const struct vertex_info *
svga_vbuf_render_get_vertex_info( struct vbuf_render *render )
{
   struct svga_vbuf_render *svga_render = svga_vbuf_render(render);
   struct svga_context *svga = svga_render->svga;

   svga_swtnl_update_vdecl(svga);

   return &svga_render->vertex_info;
}


static boolean
svga_vbuf_render_allocate_vertices( struct vbuf_render *render,
                                    ushort vertex_size,
                                    ushort nr_vertices )
{
   struct svga_vbuf_render *svga_render = svga_vbuf_render(render);
   struct svga_context *svga = svga_render->svga;
   struct pipe_screen *screen = svga->pipe.screen;
   size_t size = (size_t)nr_vertices * (size_t)vertex_size;
   boolean new_vbuf = FALSE;
   boolean new_ibuf = FALSE;

   if (svga_render->vertex_size != vertex_size)
      svga->swtnl.new_vdecl = TRUE;
   svga_render->vertex_size = (size_t)vertex_size;

   if (svga->swtnl.new_vbuf)
      new_ibuf = new_vbuf = TRUE;
   svga->swtnl.new_vbuf = FALSE;

   if (svga_render->vbuf_size < svga_render->vbuf_offset + svga_render->vbuf_used + size)
      new_vbuf = TRUE;

   if (new_vbuf)
      pipe_resource_reference(&svga_render->vbuf, NULL);
   if (new_ibuf)
      pipe_resource_reference(&svga_render->ibuf, NULL);

   if (!svga_render->vbuf) {
      svga_render->vbuf_size = MAX2(size, svga_render->vbuf_alloc_size);
      svga_render->vbuf = pipe_buffer_create(screen,
                                             PIPE_BIND_VERTEX_BUFFER,
                                             PIPE_USAGE_STREAM,
                                             svga_render->vbuf_size);
      if(!svga_render->vbuf) {
         svga_context_flush(svga, NULL);
         assert(!svga_render->vbuf);
         svga_render->vbuf = pipe_buffer_create(screen,
                                                PIPE_BIND_VERTEX_BUFFER,
                                                PIPE_USAGE_STREAM,
                                                svga_render->vbuf_size);
         /* The buffer allocation may fail if we run out of memory.
          * The draw module's vbuf code should handle that without crashing.
          */
      }

      svga->swtnl.new_vdecl = TRUE;
      svga_render->vbuf_offset = 0;
   } else {
      svga_render->vbuf_offset += svga_render->vbuf_used;
   }

   svga_render->vbuf_used = 0;

   if (svga->swtnl.new_vdecl)
      svga_render->vdecl_offset = svga_render->vbuf_offset;

   return TRUE;
}

static void *
svga_vbuf_render_map_vertices( struct vbuf_render *render )
{
   struct svga_vbuf_render *svga_render = svga_vbuf_render(render);
   struct svga_context *svga = svga_render->svga;

   if (svga_render->vbuf) {
      char *ptr = (char*)pipe_buffer_map(&svga->pipe,
                                         svga_render->vbuf,
                                         PIPE_TRANSFER_WRITE |
                                         PIPE_TRANSFER_FLUSH_EXPLICIT |
                                         PIPE_TRANSFER_DISCARD_RANGE |
                                         PIPE_TRANSFER_UNSYNCHRONIZED,
                                         &svga_render->vbuf_transfer);
      if (ptr)
         return ptr + svga_render->vbuf_offset;
      else
         return NULL;
   }
   else {
      /* we probably ran out of memory when allocating the vertex buffer */
      return NULL;
   }
}

static void
svga_vbuf_render_unmap_vertices( struct vbuf_render *render,
                                 ushort min_index,
                                 ushort max_index )
{
   struct svga_vbuf_render *svga_render = svga_vbuf_render(render);
   struct svga_context *svga = svga_render->svga;
   unsigned offset, length;
   size_t used = svga_render->vertex_size * ((size_t)max_index + 1);

   offset = svga_render->vbuf_offset + svga_render->vertex_size * min_index;
   length = svga_render->vertex_size * (max_index + 1 - min_index);
   pipe_buffer_flush_mapped_range(&svga->pipe,
				  svga_render->vbuf_transfer,
				  offset, length);
   pipe_buffer_unmap(&svga->pipe, svga_render->vbuf_transfer);
   svga_render->min_index = min_index;
   svga_render->max_index = max_index;
   svga_render->vbuf_used = MAX2(svga_render->vbuf_used, used);
}

static void
svga_vbuf_render_set_primitive( struct vbuf_render *render,
                                unsigned prim )
{
   struct svga_vbuf_render *svga_render = svga_vbuf_render(render);
   svga_render->prim = prim;
}

static void
svga_vbuf_submit_state( struct svga_vbuf_render *svga_render )
{
   struct svga_context *svga = svga_render->svga;
   SVGA3dVertexDecl vdecl[PIPE_MAX_ATTRIBS];
   enum pipe_error ret;
   int i;

   /* if the vdecl or vbuf hasn't changed do nothing */
   if (!svga->swtnl.new_vdecl)
      return;

   memcpy(vdecl, svga_render->vdecl, sizeof(vdecl));

   /* flush the hw state */
   ret = svga_hwtnl_flush(svga->hwtnl);
   if (ret != PIPE_OK) {
      svga_context_flush(svga, NULL);
      ret = svga_hwtnl_flush(svga->hwtnl);
      /* if we hit this path we might become synced with hw */
      svga->swtnl.new_vbuf = TRUE;
      assert(ret == 0);
   }

   svga_hwtnl_reset_vdecl(svga->hwtnl, svga_render->vdecl_count);

   for (i = 0; i < svga_render->vdecl_count; i++) {
      vdecl[i].array.offset += svga_render->vdecl_offset;

      svga_hwtnl_vdecl( svga->hwtnl,
                        i,
                        &vdecl[i],
                        svga_render->vbuf );
   }

   /* We have already taken care of flatshading, so let the hwtnl
    * module use whatever is most convenient:
    */
   if (svga->state.sw.need_pipeline) {
      svga_hwtnl_set_flatshade(svga->hwtnl, FALSE, FALSE);
      svga_hwtnl_set_unfilled(svga->hwtnl, PIPE_POLYGON_MODE_FILL);
   }
   else {
      svga_hwtnl_set_flatshade( svga->hwtnl,
                                svga->curr.rast->templ.flatshade,
                                svga->curr.rast->templ.flatshade_first );

      svga_hwtnl_set_unfilled( svga->hwtnl,
                               svga->curr.rast->hw_unfilled );
   }

   svga->swtnl.new_vdecl = FALSE;
}

static void
svga_vbuf_render_draw_arrays( struct vbuf_render *render,
                              unsigned start,
                              uint nr )
{
   struct svga_vbuf_render *svga_render = svga_vbuf_render(render);
   struct svga_context *svga = svga_render->svga;
   unsigned bias = (svga_render->vbuf_offset - svga_render->vdecl_offset) / svga_render->vertex_size;
   enum pipe_error ret = PIPE_OK;

   /* off to hardware */
   svga_vbuf_submit_state(svga_render);

   /* Need to call update_state() again as the draw module may have
    * altered some of our state behind our backs.  Testcase:
    * redbook/polys.c
    */
   svga_update_state_retry( svga, SVGA_STATE_HW_DRAW );

   ret = svga_hwtnl_draw_arrays(svga->hwtnl, svga_render->prim, start + bias, nr);
   if (ret != PIPE_OK) {
      svga_context_flush(svga, NULL);
      ret = svga_hwtnl_draw_arrays(svga->hwtnl, svga_render->prim, start + bias, nr);
      svga->swtnl.new_vbuf = TRUE;
      assert(ret == PIPE_OK);
   }
}


static void
svga_vbuf_render_draw_elements( struct vbuf_render *render,
                                const ushort *indices,
                                uint nr_indices)
{
   struct svga_vbuf_render *svga_render = svga_vbuf_render(render);
   struct svga_context *svga = svga_render->svga;
   struct pipe_screen *screen = svga->pipe.screen;
   int bias = (svga_render->vbuf_offset - svga_render->vdecl_offset) / svga_render->vertex_size;
   boolean ret;
   size_t size = 2 * nr_indices;

   assert(( svga_render->vbuf_offset - svga_render->vdecl_offset) % svga_render->vertex_size == 0);
   
   if (svga_render->ibuf_size < svga_render->ibuf_offset + size)
      pipe_resource_reference(&svga_render->ibuf, NULL);

   if (!svga_render->ibuf) {
      svga_render->ibuf_size = MAX2(size, svga_render->ibuf_alloc_size);
      svga_render->ibuf = pipe_buffer_create(screen,
                                             PIPE_BIND_INDEX_BUFFER,
                                             PIPE_USAGE_STREAM,
                                             svga_render->ibuf_size);
      svga_render->ibuf_offset = 0;
   }

   pipe_buffer_write_nooverlap(&svga->pipe, svga_render->ibuf,
			       svga_render->ibuf_offset, 2 * nr_indices, indices);

   /* off to hardware */
   svga_vbuf_submit_state(svga_render);

   /* Need to call update_state() again as the draw module may have
    * altered some of our state behind our backs.  Testcase:
    * redbook/polys.c
    */
   svga_update_state_retry( svga, SVGA_STATE_HW_DRAW );

   ret = svga_hwtnl_draw_range_elements(svga->hwtnl,
                                        svga_render->ibuf,
                                        2,
                                        bias,
                                        svga_render->min_index,
                                        svga_render->max_index,
                                        svga_render->prim,
                                        svga_render->ibuf_offset / 2, nr_indices);
   if(ret != PIPE_OK) {
      svga_context_flush(svga, NULL);
      ret = svga_hwtnl_draw_range_elements(svga->hwtnl,
                                           svga_render->ibuf,
                                           2,
                                           bias,
                                           svga_render->min_index,
                                           svga_render->max_index,
                                           svga_render->prim,
                                           svga_render->ibuf_offset / 2, nr_indices);
      svga->swtnl.new_vbuf = TRUE;
      assert(ret == PIPE_OK);
   }

   svga_render->ibuf_offset += size;
}


static void
svga_vbuf_render_release_vertices( struct vbuf_render *render )
{

}


static void
svga_vbuf_render_destroy( struct vbuf_render *render )
{
   struct svga_vbuf_render *svga_render = svga_vbuf_render(render);

   pipe_resource_reference(&svga_render->vbuf, NULL);
   pipe_resource_reference(&svga_render->ibuf, NULL);
   FREE(svga_render);
}


/**
 * Create a new primitive render.
 */
struct vbuf_render *
svga_vbuf_render_create( struct svga_context *svga )
{
   struct svga_vbuf_render *svga_render = CALLOC_STRUCT(svga_vbuf_render);

   svga_render->svga = svga;
   svga_render->ibuf_size = 0;
   svga_render->vbuf_size = 0;
   svga_render->ibuf_alloc_size = 4*1024;
   svga_render->vbuf_alloc_size = 64*1024;
   svga_render->base.max_vertex_buffer_bytes = 64*1024/10;
   svga_render->base.max_indices = 65536;
   svga_render->base.get_vertex_info = svga_vbuf_render_get_vertex_info;
   svga_render->base.allocate_vertices = svga_vbuf_render_allocate_vertices;
   svga_render->base.map_vertices = svga_vbuf_render_map_vertices;
   svga_render->base.unmap_vertices = svga_vbuf_render_unmap_vertices;
   svga_render->base.set_primitive = svga_vbuf_render_set_primitive;
   svga_render->base.draw_elements = svga_vbuf_render_draw_elements;
   svga_render->base.draw_arrays = svga_vbuf_render_draw_arrays;
   svga_render->base.release_vertices = svga_vbuf_render_release_vertices;
   svga_render->base.destroy = svga_vbuf_render_destroy;

   return &svga_render->base;
}
