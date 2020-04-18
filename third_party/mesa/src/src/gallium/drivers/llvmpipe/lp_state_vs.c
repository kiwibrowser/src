/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "pipe/p_defines.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_parse.h"
#include "util/u_memory.h"
#include "draw/draw_context.h"

#include "lp_context.h"
#include "lp_debug.h"
#include "lp_state.h"


static void *
llvmpipe_create_vs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct lp_vertex_shader *state;

   state = CALLOC_STRUCT(lp_vertex_shader);
   if (state == NULL ) 
      goto fail;

   /* copy shader tokens, the ones passed in will go away.
    */
   state->shader.tokens = tgsi_dup_tokens(templ->tokens);
   if (state->shader.tokens == NULL)
      goto fail;

   state->draw_data = draw_create_vertex_shader(llvmpipe->draw, templ);
   if (state->draw_data == NULL) 
      goto fail;

   if (LP_DEBUG & DEBUG_TGSI) {
      debug_printf("llvmpipe: Create vertex shader %p:\n", (void *) state);
      tgsi_dump(templ->tokens, 0);
   }

   return state;

fail:
   if (state) {
      FREE( (void *)state->shader.tokens );
      FREE( state->draw_data );
      FREE( state );
   }
   return NULL;
}


static void
llvmpipe_bind_vs_state(struct pipe_context *pipe, void *_vs)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   const struct lp_vertex_shader *vs = (const struct lp_vertex_shader *)_vs;

   if (llvmpipe->vs == vs)
      return;

   draw_bind_vertex_shader(llvmpipe->draw, 
                           vs ? vs->draw_data : NULL);

   llvmpipe->vs = vs;

   llvmpipe->dirty |= LP_NEW_VS;
}


static void
llvmpipe_delete_vs_state(struct pipe_context *pipe, void *vs)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   struct lp_vertex_shader *state =
      (struct lp_vertex_shader *)vs;

   draw_delete_vertex_shader(llvmpipe->draw, state->draw_data);
   FREE( (void *)state->shader.tokens );
   FREE( state );
}



void
llvmpipe_init_vs_funcs(struct llvmpipe_context *llvmpipe)
{
   llvmpipe->pipe.create_vs_state = llvmpipe_create_vs_state;
   llvmpipe->pipe.bind_vs_state   = llvmpipe_bind_vs_state;
   llvmpipe->pipe.delete_vs_state = llvmpipe_delete_vs_state;
}
