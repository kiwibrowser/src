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
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_bitmask.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_text.h"

#include "svga_context.h"
#include "svga_tgsi.h"
#include "svga_hw_reg.h"
#include "svga_cmd.h"
#include "svga_debug.h"


static const struct tgsi_token *substitute_vs( 
   unsigned shader_id,
   const struct tgsi_token *old_tokens )
{
#if 0
   if (shader_id == 12) {
   static struct tgsi_token tokens[300];

   const char *text = 
      "VERT\n"
      "DCL IN[0]\n"
      "DCL IN[1]\n"
      "DCL IN[2]\n"
      "DCL OUT[0], POSITION\n"
      "DCL TEMP[0..4]\n"
      "IMM FLT32 {     1.0000,     1.0000,     1.0000,     1.0000 }\n"
      "IMM FLT32 {     0.45,     1.0000,     1.0000,     1.0000 }\n"
      "IMM FLT32 { 1.297863, 0.039245, 0.035993, 0.035976}\n"
      "IMM FLT32 { -0.019398, 1.696131, -0.202151, -0.202050  }\n"
      "IMM FLT32 { 0.051711, -0.348713, -0.979204, -0.978714  }\n"
      "IMM FLT32 { 0.000000, 0.000003, 139.491577, 141.421356 }\n"
      "DCL CONST[0..7]\n"
      "DCL CONST[9..16]\n"
      "  MOV TEMP[2], IMM[0]\n"

      "  MOV TEMP[2].xyz, IN[2]\n"
      "  MOV TEMP[2].xyz, IN[0]\n"
      "  MOV TEMP[2].xyz, IN[1]\n"

      "  MUL TEMP[1], IMM[3], TEMP[2].yyyy\n"
      "  MAD TEMP[3], IMM[2],  TEMP[2].xxxx, TEMP[1]\n"
      "  MAD TEMP[1], IMM[4], TEMP[2].zzzz, TEMP[3]\n"
      "  MAD TEMP[4], IMM[5], TEMP[2].wwww, TEMP[1]\n"

      "  MOV OUT[0], TEMP[4]\n"
      "  END\n";

   if (!tgsi_text_translate( text,
                             tokens,
                             Elements(tokens) ))
   {
      assert(0);
      return NULL;
   }

   return tokens;
   }
#endif

   return old_tokens;
}


/***********************************************************************
 * Vertex shaders 
 */

static void *
svga_create_vs_state(struct pipe_context *pipe,
                     const struct pipe_shader_state *templ)
{
   struct svga_context *svga = svga_context(pipe);
   struct svga_vertex_shader *vs = CALLOC_STRUCT(svga_vertex_shader);
   if (!vs)
      return NULL;

   /* substitute a debug shader?
    */
   vs->base.tokens = tgsi_dup_tokens(substitute_vs(svga->debug.shader_id,
                                                   templ->tokens));


   /* Collect basic info that we'll need later:
    */
   tgsi_scan_shader(vs->base.tokens, &vs->base.info);

   {
      /* Need to do construct a new template in case we substitued a
       * debug shader.
       */
      struct pipe_shader_state tmp2 = *templ;
      tmp2.tokens = vs->base.tokens;
      vs->draw_shader = draw_create_vertex_shader(svga->swtnl.draw, &tmp2);
   }

   vs->base.id = svga->debug.shader_id++;

   if (SVGA_DEBUG & DEBUG_TGSI || 0) {
      debug_printf("%s id: %u, inputs: %u, outputs: %u\n",
                   __FUNCTION__, vs->base.id,
                   vs->base.info.num_inputs, vs->base.info.num_outputs);
   }

   return vs;
}

static void svga_bind_vs_state(struct pipe_context *pipe, void *shader)
{
   struct svga_vertex_shader *vs = (struct svga_vertex_shader *)shader;
   struct svga_context *svga = svga_context(pipe);

   svga->curr.vs = vs;
   svga->dirty |= SVGA_NEW_VS;
}


static void svga_delete_vs_state(struct pipe_context *pipe, void *shader)
{
   struct svga_context *svga = svga_context(pipe);
   struct svga_vertex_shader *vs = (struct svga_vertex_shader *)shader;
   struct svga_shader_result *result, *tmp;
   enum pipe_error ret;

   svga_hwtnl_flush_retry( svga );

   draw_delete_vertex_shader(svga->swtnl.draw, vs->draw_shader);
   
   for (result = vs->base.results; result; result = tmp ) {
      tmp = result->next;

      ret = SVGA3D_DestroyShader(svga->swc, 
                                 result->id,
                                 SVGA3D_SHADERTYPE_VS );
      if(ret != PIPE_OK) {
         svga_context_flush(svga, NULL);
         ret = SVGA3D_DestroyShader(svga->swc, 
                                    result->id,
                                    SVGA3D_SHADERTYPE_VS );
         assert(ret == PIPE_OK);
      }

      util_bitmask_clear( svga->vs_bm, result->id );

      svga_destroy_shader_result( result );

      /*
       * Remove stale references to this result to ensure a new result on the
       * same address will be detected as a change.
       */
      if(result == svga->state.hw_draw.vs)
         svga->state.hw_draw.vs = NULL;
   }

   FREE((void *)vs->base.tokens);
   FREE(vs);
}


void svga_init_vs_functions( struct svga_context *svga )
{
   svga->pipe.create_vs_state = svga_create_vs_state;
   svga->pipe.bind_vs_state = svga_bind_vs_state;
   svga->pipe.delete_vs_state = svga_delete_vs_state;
}

