/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
 * All Rights Reserved.
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

#include "lp_context.h"
#include "lp_state.h"
#include "lp_texture.h"

#include "util/u_memory.h"
#include "draw/draw_context.h"


static void *
llvmpipe_create_stream_output_state(struct pipe_context *pipe,
                                    const struct pipe_stream_output_info *templ)
{
   struct lp_so_state *so;
   so = (struct lp_so_state *) CALLOC_STRUCT(lp_so_state);

   if (so) {
      so->base.num_outputs = templ->num_outputs;
      memcpy(so->base.stride, templ->stride, sizeof(templ->stride));
      memcpy(so->base.output, templ->output,
             templ->num_outputs * sizeof(templ->output[0]));
   }
   return so;
}

static void
llvmpipe_bind_stream_output_state(struct pipe_context *pipe,
                                  void *so)
{
   struct llvmpipe_context *lp = llvmpipe_context(pipe);
   struct lp_so_state *lp_so = (struct lp_so_state *) so;

   lp->so = lp_so;

   lp->dirty |= LP_NEW_SO;

   if (lp_so)
      draw_set_so_state(lp->draw, &lp_so->base);
}

static void
llvmpipe_delete_stream_output_state(struct pipe_context *pipe, void *so)
{
   FREE( so );
}

static void
llvmpipe_set_stream_output_buffers(struct pipe_context *pipe,
                                   struct pipe_resource **buffers,
                                   int *offsets,
                                   int num_buffers)
{
   struct llvmpipe_context *lp = llvmpipe_context(pipe);
   int i;
   void *map_buffers[PIPE_MAX_SO_BUFFERS];

   assert(num_buffers <= PIPE_MAX_SO_BUFFERS);
   if (num_buffers > PIPE_MAX_SO_BUFFERS)
      num_buffers = PIPE_MAX_SO_BUFFERS;

   lp->dirty |= LP_NEW_SO_BUFFERS;

   for (i = 0; i < num_buffers; ++i) {
      void *mapped;
      struct llvmpipe_resource *res = llvmpipe_resource(buffers[i]);

      if (!res) {
         /* the whole call is invalid, bail out */
         lp->so_target.num_buffers = 0;
         draw_set_mapped_so_buffers(lp->draw, 0, 0);
         return;
      }

      lp->so_target.buffer[i] = res;
      lp->so_target.offset[i] = offsets[i];
      lp->so_target.so_count[i] = 0;

      mapped = res->data;
      if (offsets[i] >= 0)
         map_buffers[i] = ((char*)mapped) + offsets[i];
      else {
         /* this is a buffer append */
         assert(!"appending not implemented");
         map_buffers[i] = mapped;
      }
   }
   lp->so_target.num_buffers = num_buffers;

   draw_set_mapped_so_buffers(lp->draw, map_buffers, num_buffers);
}

void
llvmpipe_init_so_funcs(struct llvmpipe_context *llvmpipe)
{
#if 0
   llvmpipe->pipe.create_stream_output_state =
      llvmpipe_create_stream_output_state;
   llvmpipe->pipe.bind_stream_output_state =
      llvmpipe_bind_stream_output_state;
   llvmpipe->pipe.delete_stream_output_state =
      llvmpipe_delete_stream_output_state;

   llvmpipe->pipe.set_stream_output_buffers =
      llvmpipe_set_stream_output_buffers;
#else
   (void) llvmpipe_create_stream_output_state;
   (void) llvmpipe_bind_stream_output_state;
   (void) llvmpipe_delete_stream_output_state;
   (void) llvmpipe_set_stream_output_buffers;
#endif
}
