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

#include "util/u_inlines.h"
#include "indices/u_indices.h"

#include "svga_hw_reg.h"
#include "svga_draw.h"
#include "svga_draw_private.h"
#include "svga_context.h"


#define DBG 0




static enum pipe_error generate_indices( struct svga_hwtnl *hwtnl,
                                         unsigned nr,
                                         unsigned index_size,
                                         u_generate_func generate,
                                         struct pipe_resource **out_buf )
{
   struct pipe_context *pipe = &hwtnl->svga->pipe;
   struct pipe_transfer *transfer;
   unsigned size = index_size * nr;
   struct pipe_resource *dst = NULL;
   void *dst_map = NULL;

   dst = pipe_buffer_create( pipe->screen, 
			     PIPE_BIND_INDEX_BUFFER, 
			     PIPE_USAGE_STATIC,
			     size );
   if (dst == NULL)
      goto fail;

   dst_map = pipe_buffer_map( pipe, dst, PIPE_TRANSFER_WRITE,
			      &transfer);
   if (dst_map == NULL)
      goto fail;

   generate( nr,
             dst_map );

   pipe_buffer_unmap( pipe, transfer );

   *out_buf = dst;
   return PIPE_OK;

fail:
   if (dst_map)
      pipe_buffer_unmap( pipe, transfer );

   if (dst)
      pipe->screen->resource_destroy( pipe->screen, dst );
   
   return PIPE_ERROR_OUT_OF_MEMORY;
}

static boolean compare( unsigned cached_nr,
                        unsigned nr,
                        unsigned type )
{
   if (type == U_GENERATE_REUSABLE)
      return cached_nr >= nr;
   else
      return cached_nr == nr;
}

static enum pipe_error retrieve_or_generate_indices( struct svga_hwtnl *hwtnl,
                                                     unsigned prim,
                                                     unsigned gen_type,
                                                     unsigned gen_nr,
                                                     unsigned gen_size,
                                                     u_generate_func generate,
                                                     struct pipe_resource **out_buf )
{
   enum pipe_error ret = PIPE_OK;
   int i;

   for (i = 0; i < IDX_CACHE_MAX; i++) {
      if (hwtnl->index_cache[prim][i].buffer != NULL &&
          hwtnl->index_cache[prim][i].generate == generate)
      {
         if (compare(hwtnl->index_cache[prim][i].gen_nr, gen_nr, gen_type))
         {
            pipe_resource_reference( out_buf,
                                   hwtnl->index_cache[prim][i].buffer );

            if (DBG) 
               debug_printf("%s retrieve %d/%d\n", __FUNCTION__, i, gen_nr);

            return PIPE_OK;
         }
         else if (gen_type == U_GENERATE_REUSABLE) 
         {
            pipe_resource_reference( &hwtnl->index_cache[prim][i].buffer,
                                   NULL );

            if (DBG) 
               debug_printf("%s discard %d/%d\n", __FUNCTION__, 
                            i, hwtnl->index_cache[prim][i].gen_nr);

            break;
         }
      }
   }

   if (i == IDX_CACHE_MAX)
   {
      unsigned smallest = 0;
      unsigned smallest_size = ~0;
      
      for (i = 0; i < IDX_CACHE_MAX && smallest_size; i++) {
         if (hwtnl->index_cache[prim][i].buffer == NULL)
         {
            smallest = i;
            smallest_size = 0;
         }
         else if (hwtnl->index_cache[prim][i].gen_nr < smallest)
         {
            smallest = i;
            smallest_size = hwtnl->index_cache[prim][i].gen_nr;
         }
      }

      assert (smallest != IDX_CACHE_MAX);

      pipe_resource_reference( &hwtnl->index_cache[prim][smallest].buffer,
                             NULL );

      if (DBG)
         debug_printf("%s discard smallest %d/%d\n", __FUNCTION__, 
                      smallest, smallest_size);
      
      i = smallest;
   }
      
      
   ret = generate_indices( hwtnl, 
                           gen_nr,
                           gen_size,
                           generate,
                           out_buf );
   if (ret != PIPE_OK)
      return ret;


   hwtnl->index_cache[prim][i].generate = generate;
   hwtnl->index_cache[prim][i].gen_nr = gen_nr;
   pipe_resource_reference( &hwtnl->index_cache[prim][i].buffer,
                          *out_buf );

   if (DBG)
      debug_printf("%s cache %d/%d\n", __FUNCTION__, 
                   i, hwtnl->index_cache[prim][i].gen_nr);

   return PIPE_OK;
}



static enum pipe_error
simple_draw_arrays( struct svga_hwtnl *hwtnl,
                    unsigned prim, unsigned start, unsigned count )
{
   SVGA3dPrimitiveRange range;
   unsigned hw_prim;
   unsigned hw_count;

   hw_prim = svga_translate_prim(prim, count, &hw_count);
   if (hw_count == 0)
      return PIPE_ERROR_BAD_INPUT;
      
   range.primType = hw_prim;
   range.primitiveCount = hw_count;
   range.indexArray.surfaceId = SVGA3D_INVALID_ID;
   range.indexArray.offset = 0;
   range.indexArray.stride = 0;
   range.indexWidth = 0;
   range.indexBias = start;

   /* Min/max index should be calculated prior to applying bias, so we
    * end up with min_index = 0, max_index = count - 1 and everybody
    * looking at those numbers knows to adjust them by
    * range.indexBias.
    */
   return svga_hwtnl_prim( hwtnl, &range, 0, count - 1, NULL );
}










enum pipe_error 
svga_hwtnl_draw_arrays( struct svga_hwtnl *hwtnl,
                        unsigned prim, 
                        unsigned start, 
                        unsigned count)
{
   unsigned gen_prim, gen_size, gen_nr, gen_type;
   u_generate_func gen_func;
   enum pipe_error ret = PIPE_OK;

   if (hwtnl->api_fillmode != PIPE_POLYGON_MODE_FILL && 
       prim >= PIPE_PRIM_TRIANGLES) 
   {
      gen_type = u_unfilled_generator( prim,
                                       start,
                                       count,
                                       hwtnl->api_fillmode,
                                       &gen_prim,
                                       &gen_size,
                                       &gen_nr,
                                       &gen_func );
   }
   else {
      gen_type = u_index_generator( svga_hw_prims,
                                    prim,
                                    start,
                                    count,
                                    hwtnl->api_pv,
                                    hwtnl->hw_pv,
                                    &gen_prim,
                                    &gen_size,
                                    &gen_nr,
                                    &gen_func );
   }

   if (gen_type == U_GENERATE_LINEAR) {
      return simple_draw_arrays( hwtnl, gen_prim, start, count );
   }
   else {
      struct pipe_resource *gen_buf = NULL;

      /* Need to draw as indexed primitive. 
       * Potentially need to run the gen func to build an index buffer.
       */
      ret = retrieve_or_generate_indices( hwtnl,
                                          prim,
                                          gen_type,
                                          gen_nr,
                                          gen_size,
                                          gen_func,
                                          &gen_buf );
      if (ret != PIPE_OK)
         goto done;

      ret = svga_hwtnl_simple_draw_range_elements( hwtnl,
                                                   gen_buf,
                                                   gen_size,
                                                   start,
                                                   0,
                                                   count - 1,
                                                   gen_prim,
                                                   0,
                                                   gen_nr );

      if (ret != PIPE_OK)
         goto done;

   done:
      if (gen_buf)
         pipe_resource_reference( &gen_buf, NULL );

      return ret;
   }
}

