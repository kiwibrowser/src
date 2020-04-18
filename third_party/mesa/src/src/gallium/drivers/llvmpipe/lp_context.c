/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2008 VMware, Inc.  All rights reserved.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/* Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"
#include "lp_clear.h"
#include "lp_context.h"
#include "lp_flush.h"
#include "lp_perf.h"
#include "lp_state.h"
#include "lp_surface.h"
#include "lp_query.h"
#include "lp_setup.h"


/** shared by all contexts */
unsigned llvmpipe_variant_count;


static void llvmpipe_destroy( struct pipe_context *pipe )
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context( pipe );
   uint i, j;

   lp_print_counters();

   /* This will also destroy llvmpipe->setup:
    */
   if (llvmpipe->draw)
      draw_destroy( llvmpipe->draw );

   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      pipe_surface_reference(&llvmpipe->framebuffer.cbufs[i], NULL);
   }

   pipe_surface_reference(&llvmpipe->framebuffer.zsbuf, NULL);

   for (i = 0; i < Elements(llvmpipe->sampler_views[0]); i++) {
      pipe_sampler_view_reference(&llvmpipe->sampler_views[PIPE_SHADER_FRAGMENT][i], NULL);
   }

   for (i = 0; i < Elements(llvmpipe->sampler_views[0]); i++) {
      pipe_sampler_view_reference(&llvmpipe->sampler_views[PIPE_SHADER_VERTEX][i], NULL);
   }

   for (i = 0; i < Elements(llvmpipe->constants); i++) {
      for (j = 0; j < Elements(llvmpipe->constants[i]); j++) {
         pipe_resource_reference(&llvmpipe->constants[i][j], NULL);
      }
   }

   for (i = 0; i < llvmpipe->num_vertex_buffers; i++) {
      pipe_resource_reference(&llvmpipe->vertex_buffer[i].buffer, NULL);
   }

   lp_delete_setup_variants(llvmpipe);

   align_free( llvmpipe );
}

static void
do_flush( struct pipe_context *pipe,
          struct pipe_fence_handle **fence)
{
   llvmpipe_flush(pipe, fence, __FUNCTION__);
}


static void
llvmpipe_render_condition ( struct pipe_context *pipe,
                            struct pipe_query *query,
                            uint mode )
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context( pipe );

   llvmpipe->render_cond_query = query;
   llvmpipe->render_cond_mode = mode;
}

struct pipe_context *
llvmpipe_create_context( struct pipe_screen *screen, void *priv )
{
   struct llvmpipe_context *llvmpipe;

   llvmpipe = align_malloc(sizeof(struct llvmpipe_context), 16);
   if (!llvmpipe)
      return NULL;

   util_init_math();

   memset(llvmpipe, 0, sizeof *llvmpipe);

   make_empty_list(&llvmpipe->fs_variants_list);

   make_empty_list(&llvmpipe->setup_variants_list);


   llvmpipe->pipe.screen = screen;
   llvmpipe->pipe.priv = priv;

   /* Init the pipe context methods */
   llvmpipe->pipe.destroy = llvmpipe_destroy;
   llvmpipe->pipe.set_framebuffer_state = llvmpipe_set_framebuffer_state;
   llvmpipe->pipe.clear = llvmpipe_clear;
   llvmpipe->pipe.flush = do_flush;

   llvmpipe->pipe.render_condition = llvmpipe_render_condition;

   llvmpipe_init_blend_funcs(llvmpipe);
   llvmpipe_init_clip_funcs(llvmpipe);
   llvmpipe_init_draw_funcs(llvmpipe);
   llvmpipe_init_sampler_funcs(llvmpipe);
   llvmpipe_init_query_funcs( llvmpipe );
   llvmpipe_init_vertex_funcs(llvmpipe);
   llvmpipe_init_so_funcs(llvmpipe);
   llvmpipe_init_fs_funcs(llvmpipe);
   llvmpipe_init_vs_funcs(llvmpipe);
   llvmpipe_init_gs_funcs(llvmpipe);
   llvmpipe_init_rasterizer_funcs(llvmpipe);
   llvmpipe_init_context_resource_funcs( &llvmpipe->pipe );
   llvmpipe_init_surface_functions(llvmpipe);

   /*
    * Create drawing context and plug our rendering stage into it.
    */
   llvmpipe->draw = draw_create(&llvmpipe->pipe);
   if (!llvmpipe->draw)
      goto fail;

   /* FIXME: devise alternative to draw_texture_samplers */

   llvmpipe->setup = lp_setup_create( &llvmpipe->pipe,
                                      llvmpipe->draw );
   if (!llvmpipe->setup)
      goto fail;

   /* plug in AA line/point stages */
   draw_install_aaline_stage(llvmpipe->draw, &llvmpipe->pipe);
   draw_install_aapoint_stage(llvmpipe->draw, &llvmpipe->pipe);
   draw_install_pstipple_stage(llvmpipe->draw, &llvmpipe->pipe);

   /* convert points and lines into triangles: 
    * (otherwise, draw points and lines natively)
    */
   draw_wide_point_sprites(llvmpipe->draw, FALSE);
   draw_enable_point_sprites(llvmpipe->draw, FALSE);
   draw_wide_point_threshold(llvmpipe->draw, 10000.0);
   draw_wide_line_threshold(llvmpipe->draw, 10000.0);

   lp_reset_counters();

   return &llvmpipe->pipe;

 fail:
   llvmpipe_destroy(&llvmpipe->pipe);
   return NULL;
}

