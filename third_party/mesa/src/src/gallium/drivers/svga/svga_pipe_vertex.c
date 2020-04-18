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
#include "util/u_memory.h"
#include "util/u_transfer.h"
#include "tgsi/tgsi_parse.h"

#include "svga_screen.h"
#include "svga_resource_buffer.h"
#include "svga_context.h"


static void svga_set_vertex_buffers(struct pipe_context *pipe,
                                    unsigned count,
                                    const struct pipe_vertex_buffer *buffers)
{
   struct svga_context *svga = svga_context(pipe);
   unsigned i;
   boolean any_user_buffer = FALSE;

   /* Check for no change */
   if (count == svga->curr.num_vertex_buffers &&
       memcmp(svga->curr.vb, buffers, count * sizeof buffers[0]) == 0)
      return;

   /* Adjust refcounts */
   for (i = 0; i < count; i++) {
      pipe_resource_reference(&svga->curr.vb[i].buffer, buffers[i].buffer);
      if (svga_buffer_is_user_buffer(buffers[i].buffer))
         any_user_buffer = TRUE;
   }

   for ( ; i < svga->curr.num_vertex_buffers; i++)
      pipe_resource_reference(&svga->curr.vb[i].buffer, NULL);

   /* Copy remaining data */
   memcpy(svga->curr.vb, buffers, count * sizeof buffers[0]);
   svga->curr.num_vertex_buffers = count;
   svga->curr.any_user_vertex_buffers = any_user_buffer;

   svga->dirty |= SVGA_NEW_VBUFFER;
}


static void svga_set_index_buffer(struct pipe_context *pipe,
                                  const struct pipe_index_buffer *ib)
{
   struct svga_context *svga = svga_context(pipe);

   if (ib) {
      pipe_resource_reference(&svga->curr.ib.buffer, ib->buffer);
      memcpy(&svga->curr.ib, ib, sizeof(svga->curr.ib));
   }
   else {
      pipe_resource_reference(&svga->curr.ib.buffer, NULL);
      memset(&svga->curr.ib, 0, sizeof(svga->curr.ib));
   }

   /* TODO make this more like a state */
}


static void *
svga_create_vertex_elements_state(struct pipe_context *pipe,
                                  unsigned count,
                                  const struct pipe_vertex_element *attribs)
{
   struct svga_velems_state *velems;
   assert(count <= PIPE_MAX_ATTRIBS);
   velems = (struct svga_velems_state *) MALLOC(sizeof(struct svga_velems_state));
   if (velems) {
      velems->count = count;
      memcpy(velems->velem, attribs, sizeof(*attribs) * count);
   }
   return velems;
}

static void svga_bind_vertex_elements_state(struct pipe_context *pipe,
                                            void *velems)
{
   struct svga_context *svga = svga_context(pipe);
   struct svga_velems_state *svga_velems = (struct svga_velems_state *) velems;

   svga->curr.velems = svga_velems;
   svga->dirty |= SVGA_NEW_VELEMENT;
}

static void svga_delete_vertex_elements_state(struct pipe_context *pipe,
                                              void *velems)
{
   FREE(velems);
}

void svga_cleanup_vertex_state( struct svga_context *svga )
{
   unsigned i;
   
   for (i = 0 ; i < svga->curr.num_vertex_buffers; i++)
      pipe_resource_reference(&svga->curr.vb[i].buffer, NULL);
}


void svga_init_vertex_functions( struct svga_context *svga )
{
   svga->pipe.set_vertex_buffers = svga_set_vertex_buffers;
   svga->pipe.set_index_buffer = svga_set_index_buffer;
   svga->pipe.create_vertex_elements_state = svga_create_vertex_elements_state;
   svga->pipe.bind_vertex_elements_state = svga_bind_vertex_elements_state;
   svga->pipe.delete_vertex_elements_state = svga_delete_vertex_elements_state;
}


