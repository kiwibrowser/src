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
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_bitmask.h"
#include "tgsi/tgsi_parse.h"
#include "draw/draw_context.h"

#include "svga_context.h"
#include "svga_tgsi.h"
#include "svga_hw_reg.h"
#include "svga_cmd.h"
#include "svga_debug.h"


/***********************************************************************
 * Fragment shaders 
 */

static void *
svga_create_fs_state(struct pipe_context *pipe,
                     const struct pipe_shader_state *templ)
{
   struct svga_context *svga = svga_context(pipe);
   struct svga_fragment_shader *fs;

   fs = CALLOC_STRUCT(svga_fragment_shader);
   if (!fs)
      return NULL;

   fs->base.tokens = tgsi_dup_tokens(templ->tokens);

   /* Collect basic info that we'll need later:
    */
   tgsi_scan_shader(fs->base.tokens, &fs->base.info);

   fs->base.id = svga->debug.shader_id++;
   
   fs->generic_inputs = svga_get_generic_inputs_mask(&fs->base.info);

   svga_remap_generics(fs->generic_inputs, fs->generic_remap_table);

   fs->draw_shader = draw_create_fragment_shader(svga->swtnl.draw, templ);

   if (SVGA_DEBUG & DEBUG_TGSI || 0) {
      debug_printf("%s id: %u, inputs: %u, outputs: %u\n",
                   __FUNCTION__, fs->base.id,
                   fs->base.info.num_inputs, fs->base.info.num_outputs);
   }

   return fs;
}

static void
svga_bind_fs_state(struct pipe_context *pipe, void *shader)
{
   struct svga_fragment_shader *fs = (struct svga_fragment_shader *) shader;
   struct svga_context *svga = svga_context(pipe);

   svga->curr.fs = fs;
   svga->dirty |= SVGA_NEW_FS;
}

static
void svga_delete_fs_state(struct pipe_context *pipe, void *shader)
{
   struct svga_context *svga = svga_context(pipe);
   struct svga_fragment_shader *fs = (struct svga_fragment_shader *) shader;
   struct svga_shader_result *result, *tmp;
   enum pipe_error ret;

   svga_hwtnl_flush_retry( svga );

   draw_delete_fragment_shader(svga->swtnl.draw, fs->draw_shader);

   for (result = fs->base.results; result; result = tmp ) {
      tmp = result->next;

      ret = SVGA3D_DestroyShader(svga->swc, 
                                 result->id,
                                 SVGA3D_SHADERTYPE_PS );
      if(ret != PIPE_OK) {
         svga_context_flush(svga, NULL);
         ret = SVGA3D_DestroyShader(svga->swc, 
                                    result->id,
                                    SVGA3D_SHADERTYPE_PS );
         assert(ret == PIPE_OK);
      }

      util_bitmask_clear( svga->fs_bm, result->id );

      svga_destroy_shader_result( result );

      /*
       * Remove stale references to this result to ensure a new result on the
       * same address will be detected as a change.
       */
      if(result == svga->state.hw_draw.fs)
         svga->state.hw_draw.fs = NULL;
   }

   FREE((void *)fs->base.tokens);
   FREE(fs);
}


void svga_init_fs_functions( struct svga_context *svga )
{
   svga->pipe.create_fs_state = svga_create_fs_state;
   svga->pipe.bind_fs_state = svga_bind_fs_state;
   svga->pipe.delete_fs_state = svga_delete_fs_state;
}

