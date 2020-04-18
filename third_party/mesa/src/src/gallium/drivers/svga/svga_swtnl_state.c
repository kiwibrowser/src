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
#include "draw/draw_vbuf.h"
#include "util/u_inlines.h"
#include "pipe/p_state.h"

#include "svga_context.h"
#include "svga_swtnl.h"
#include "svga_state.h"
#include "svga_tgsi.h"
#include "svga_swtnl_private.h"


#define SVGA_POINT_ADJ_X -0.375
#define SVGA_POINT_ADJ_Y -0.5

#define SVGA_LINE_ADJ_X -0.5
#define SVGA_LINE_ADJ_Y -0.5

#define SVGA_TRIANGLE_ADJ_X -0.375
#define SVGA_TRIANGLE_ADJ_Y -0.5


static void set_draw_viewport( struct svga_context *svga )
{
   struct pipe_viewport_state vp = svga->curr.viewport;
   float adjx = 0;
   float adjy = 0;

   switch (svga->curr.reduced_prim) {
   case PIPE_PRIM_POINTS:
      adjx = SVGA_POINT_ADJ_X;
      adjy = SVGA_POINT_ADJ_Y;
      break;
   case PIPE_PRIM_LINES:
      /* XXX: This is to compensate for the fact that wide lines are
       * going to be drawn with triangles, but we're not catching all
       * cases where that will happen.
       */
      if (svga->curr.rast->need_pipeline & SVGA_PIPELINE_FLAG_LINES)
      {
         adjx = SVGA_LINE_ADJ_X + 0.175;
         adjy = SVGA_LINE_ADJ_Y - 0.175;
      }
      else {
         adjx = SVGA_LINE_ADJ_X;
         adjy = SVGA_LINE_ADJ_Y;
      }
      break;
   case PIPE_PRIM_TRIANGLES:
      adjx += SVGA_TRIANGLE_ADJ_X;
      adjy += SVGA_TRIANGLE_ADJ_Y;
      break;
   }

   vp.translate[0] += adjx;
   vp.translate[1] += adjy;

   draw_set_viewport_state(svga->swtnl.draw, &vp);
}

static enum pipe_error
update_swtnl_draw( struct svga_context *svga,
                   unsigned dirty )
{
   draw_flush( svga->swtnl.draw );

   if (dirty & SVGA_NEW_VS) 
      draw_bind_vertex_shader(svga->swtnl.draw,
                              svga->curr.vs->draw_shader);

   if (dirty & SVGA_NEW_FS) 
      draw_bind_fragment_shader(svga->swtnl.draw,
                                svga->curr.fs->draw_shader);

   if (dirty & SVGA_NEW_VBUFFER)
      draw_set_vertex_buffers(svga->swtnl.draw, 
                              svga->curr.num_vertex_buffers, 
                              svga->curr.vb);

   if (dirty & SVGA_NEW_VELEMENT)
      draw_set_vertex_elements(svga->swtnl.draw, 
                               svga->curr.velems->count, 
                               svga->curr.velems->velem );

   if (dirty & SVGA_NEW_CLIP)
      draw_set_clip_state(svga->swtnl.draw, 
                          &svga->curr.clip);

   if (dirty & (SVGA_NEW_VIEWPORT |
                SVGA_NEW_REDUCED_PRIMITIVE | 
                SVGA_NEW_RAST))
      set_draw_viewport( svga );

   if (dirty & SVGA_NEW_RAST)
      draw_set_rasterizer_state(svga->swtnl.draw,
                                &svga->curr.rast->templ,
                                (void *) svga->curr.rast);

   if (dirty & SVGA_NEW_FRAME_BUFFER)
      draw_set_mrd(svga->swtnl.draw, 
                   svga->curr.depthscale);

   return 0;
}


struct svga_tracked_state svga_update_swtnl_draw =
{
   "update draw module state",
   (SVGA_NEW_VS |
    SVGA_NEW_VBUFFER |
    SVGA_NEW_VELEMENT |
    SVGA_NEW_CLIP |
    SVGA_NEW_VIEWPORT |
    SVGA_NEW_RAST |
    SVGA_NEW_FRAME_BUFFER |
    SVGA_NEW_REDUCED_PRIMITIVE),
   update_swtnl_draw
};


enum pipe_error
svga_swtnl_update_vdecl( struct svga_context *svga )
{
   struct svga_vbuf_render *svga_render = svga_vbuf_render(svga->swtnl.backend);
   struct draw_context *draw = svga->swtnl.draw;
   struct vertex_info *vinfo = &svga_render->vertex_info;
   SVGA3dVertexDecl vdecl[PIPE_MAX_ATTRIBS];
   const enum interp_mode colorInterp =
      svga->curr.rast->templ.flatshade ? INTERP_CONSTANT : INTERP_LINEAR;
   struct svga_fragment_shader *fs = svga->curr.fs;
   int offset = 0;
   int nr_decls = 0;
   int src, i;

   memset(vinfo, 0, sizeof(*vinfo));
   memset(vdecl, 0, sizeof(vdecl));

   /* always add position */
   src = draw_find_shader_output(draw, TGSI_SEMANTIC_POSITION, 0);
   draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_LINEAR, src);
   vinfo->attrib[0].emit = EMIT_4F;
   vdecl[0].array.offset = offset;
   vdecl[0].identity.type = SVGA3D_DECLTYPE_FLOAT4;
   vdecl[0].identity.usage = SVGA3D_DECLUSAGE_POSITIONT;
   vdecl[0].identity.usageIndex = 0;
   offset += 16;
   nr_decls++;

   for (i = 0; i < fs->base.info.num_inputs; i++) {
      const unsigned sem_name = fs->base.info.input_semantic_name[i];
      const unsigned sem_index = fs->base.info.input_semantic_index[i];

      src = draw_find_shader_output(draw, sem_name, sem_index);

      vdecl[nr_decls].array.offset = offset;
      vdecl[nr_decls].identity.usageIndex = sem_index;

      switch (sem_name) {
      case TGSI_SEMANTIC_COLOR:
         draw_emit_vertex_attr(vinfo, EMIT_4F, colorInterp, src);
         vdecl[nr_decls].identity.usage = SVGA3D_DECLUSAGE_COLOR;
         vdecl[nr_decls].identity.type = SVGA3D_DECLTYPE_FLOAT4;
         offset += 16;
         nr_decls++;
         break;
      case TGSI_SEMANTIC_GENERIC:
         draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_PERSPECTIVE, src);
         vdecl[nr_decls].identity.usage = SVGA3D_DECLUSAGE_TEXCOORD;
         vdecl[nr_decls].identity.type = SVGA3D_DECLTYPE_FLOAT4;
         vdecl[nr_decls].identity.usageIndex =
            svga_remap_generic_index(fs->generic_remap_table, sem_index);
         offset += 16;
         nr_decls++;
         break;
      case TGSI_SEMANTIC_FOG:
         draw_emit_vertex_attr(vinfo, EMIT_1F, INTERP_PERSPECTIVE, src);
         vdecl[nr_decls].identity.usage = SVGA3D_DECLUSAGE_TEXCOORD;
         vdecl[nr_decls].identity.type = SVGA3D_DECLTYPE_FLOAT1;
         assert(vdecl[nr_decls].identity.usageIndex == 0);
         offset += 4;
         nr_decls++;
         break;
      case TGSI_SEMANTIC_POSITION:
         /* generated internally, not a vertex shader output */
         break;
      default:
         assert(0);
      }
   }

   draw_compute_vertex_size(vinfo);

   svga_render->vdecl_count = nr_decls;
   for (i = 0; i < svga_render->vdecl_count; i++)
      vdecl[i].array.stride = offset;

   if (memcmp(svga_render->vdecl, vdecl, sizeof(vdecl)) == 0)
      return 0;

   memcpy(svga_render->vdecl, vdecl, sizeof(vdecl));
   svga->swtnl.new_vdecl = TRUE;

   return 0;
}


static enum pipe_error
update_swtnl_vdecl( struct svga_context *svga,
                    unsigned dirty )
{
   return svga_swtnl_update_vdecl( svga );
}


struct svga_tracked_state svga_update_swtnl_vdecl =
{
   "update draw module vdecl",
   (SVGA_NEW_VS |
    SVGA_NEW_FS),
   update_swtnl_vdecl
};
