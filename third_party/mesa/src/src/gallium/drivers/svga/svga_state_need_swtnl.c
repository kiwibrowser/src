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
#include "pipe/p_state.h"


#include "svga_context.h"
#include "svga_state.h"
#include "svga_debug.h"
#include "svga_hw_reg.h"

/***********************************************************************
 */


/**
 * Given a gallium vertex element format, return the corresponding SVGA3D
 * format.  Return SVGA3D_DECLTYPE_MAX for unsupported gallium formats.
 */
static INLINE SVGA3dDeclType 
svga_translate_vertex_format(enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_R32_FLOAT:            return SVGA3D_DECLTYPE_FLOAT1;
   case PIPE_FORMAT_R32G32_FLOAT:         return SVGA3D_DECLTYPE_FLOAT2;
   case PIPE_FORMAT_R32G32B32_FLOAT:      return SVGA3D_DECLTYPE_FLOAT3;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:   return SVGA3D_DECLTYPE_FLOAT4;
   case PIPE_FORMAT_B8G8R8A8_UNORM:       return SVGA3D_DECLTYPE_D3DCOLOR;
   case PIPE_FORMAT_R8G8B8A8_USCALED:     return SVGA3D_DECLTYPE_UBYTE4;
   case PIPE_FORMAT_R16G16_SSCALED:       return SVGA3D_DECLTYPE_SHORT2;
   case PIPE_FORMAT_R16G16B16A16_SSCALED: return SVGA3D_DECLTYPE_SHORT4;
   case PIPE_FORMAT_R8G8B8A8_UNORM:       return SVGA3D_DECLTYPE_UBYTE4N;
   case PIPE_FORMAT_R16G16_SNORM:         return SVGA3D_DECLTYPE_SHORT2N;
   case PIPE_FORMAT_R16G16B16A16_SNORM:   return SVGA3D_DECLTYPE_SHORT4N;
   case PIPE_FORMAT_R16G16_UNORM:         return SVGA3D_DECLTYPE_USHORT2N;
   case PIPE_FORMAT_R16G16B16A16_UNORM:   return SVGA3D_DECLTYPE_USHORT4N;
   case PIPE_FORMAT_R10G10B10X2_USCALED:  return SVGA3D_DECLTYPE_UDEC3;
   case PIPE_FORMAT_R10G10B10X2_SNORM:    return SVGA3D_DECLTYPE_DEC3N;
   case PIPE_FORMAT_R16G16_FLOAT:         return SVGA3D_DECLTYPE_FLOAT16_2;
   case PIPE_FORMAT_R16G16B16A16_FLOAT:   return SVGA3D_DECLTYPE_FLOAT16_4;

   default:
      /* There are many formats without hardware support.  This case
       * will be hit regularly, meaning we'll need swvfetch.
       */
      return SVGA3D_DECLTYPE_MAX;
   }
}


static enum pipe_error
update_need_swvfetch( struct svga_context *svga,
                      unsigned dirty )
{
   unsigned i;
   boolean need_swvfetch = FALSE;

   if (!svga->curr.velems) {
      /* No vertex elements bound. */
      return 0;
   }

   for (i = 0; i < svga->curr.velems->count; i++) {
      svga->state.sw.ve_format[i] = svga_translate_vertex_format(svga->curr.velems->velem[i].src_format);
      if (svga->state.sw.ve_format[i] == SVGA3D_DECLTYPE_MAX) {
         /* Unsupported format - use software fetch */
         need_swvfetch = TRUE;
         break;
      }
   }

   if (need_swvfetch != svga->state.sw.need_swvfetch) {
      svga->state.sw.need_swvfetch = need_swvfetch;
      svga->dirty |= SVGA_NEW_NEED_SWVFETCH;
   }
   
   return PIPE_OK;
}

struct svga_tracked_state svga_update_need_swvfetch = 
{
   "update need_swvfetch",
   ( SVGA_NEW_VELEMENT ),
   update_need_swvfetch
};


/*********************************************************************** 
 */

static enum pipe_error
update_need_pipeline( struct svga_context *svga,
                      unsigned dirty )
{
   
   boolean need_pipeline = FALSE;
   struct svga_vertex_shader *vs = svga->curr.vs;

   /* SVGA_NEW_RAST, SVGA_NEW_REDUCED_PRIMITIVE
    */
   if (svga->curr.rast->need_pipeline & (1 << svga->curr.reduced_prim)) {
      SVGA_DBG(DEBUG_SWTNL, "%s: rast need_pipeline (0x%x) & prim (0x%x)\n",
                 __FUNCTION__,
                 svga->curr.rast->need_pipeline,
                 (1 << svga->curr.reduced_prim) );
      SVGA_DBG(DEBUG_SWTNL, "%s: rast need_pipeline tris (%s), lines (%s), points (%s)\n",
                 __FUNCTION__,
                 svga->curr.rast->need_pipeline_tris_str,
                 svga->curr.rast->need_pipeline_lines_str,
                 svga->curr.rast->need_pipeline_points_str);
      need_pipeline = TRUE;
   }

   /* EDGEFLAGS
    */
    if (vs && vs->base.info.writes_edgeflag) {
      SVGA_DBG(DEBUG_SWTNL, "%s: edgeflags\n", __FUNCTION__);
      need_pipeline = TRUE;
   }

   /* SVGA_NEW_FS, SVGA_NEW_RAST, SVGA_NEW_REDUCED_PRIMITIVE
    */
   if (svga->curr.reduced_prim == PIPE_PRIM_POINTS) {
      unsigned sprite_coord_gen = svga->curr.rast->templ.sprite_coord_enable;
      unsigned generic_inputs =
         svga->curr.fs ? svga->curr.fs->generic_inputs : 0;

      if (sprite_coord_gen &&
          (generic_inputs & ~sprite_coord_gen)) {
         /* The fragment shader is using some generic inputs that are
          * not being replaced by auto-generated point/sprite coords (and
          * auto sprite coord generation is turned on).
          * The SVGA3D interface does not support that: if we enable
          * SVGA3D_RS_POINTSPRITEENABLE it gets enabled for _all_
          * texture coordinate sets.
          * To solve this, we have to use the draw-module's wide/sprite
          * point stage.
          */
         need_pipeline = TRUE;
      }
   }

   if (need_pipeline != svga->state.sw.need_pipeline) {
      svga->state.sw.need_pipeline = need_pipeline;
      svga->dirty |= SVGA_NEW_NEED_PIPELINE;
   }

   /* DEBUG */
   if (0 && svga->state.sw.need_pipeline)
      debug_printf("sw.need_pipeline = %d\n", svga->state.sw.need_pipeline);

   return PIPE_OK;
}


struct svga_tracked_state svga_update_need_pipeline = 
{
   "need pipeline",
   (SVGA_NEW_RAST |
    SVGA_NEW_FS |
    SVGA_NEW_VS |
    SVGA_NEW_REDUCED_PRIMITIVE),
   update_need_pipeline
};


/*********************************************************************** 
 */

static enum pipe_error
update_need_swtnl( struct svga_context *svga,
                   unsigned dirty )
{
   boolean need_swtnl;

   if (svga->debug.no_swtnl) {
      svga->state.sw.need_swvfetch = FALSE;
      svga->state.sw.need_pipeline = FALSE;
   }

   need_swtnl = (svga->state.sw.need_swvfetch ||
                 svga->state.sw.need_pipeline);

   if (svga->debug.force_swtnl) {
      need_swtnl = TRUE;
   }

   /*
    * Some state changes the draw module does makes us believe we
    * we don't need swtnl. This causes the vdecl code to pickup
    * the wrong buffers and vertex formats. Try trivial/line-wide.
    */
   if (svga->state.sw.in_swtnl_draw)
      need_swtnl = TRUE;

   if (need_swtnl != svga->state.sw.need_swtnl) {
      SVGA_DBG(DEBUG_SWTNL|DEBUG_PERF,
               "%s: need_swvfetch %s, need_pipeline %s\n",
               __FUNCTION__,
               svga->state.sw.need_swvfetch ? "true" : "false",
               svga->state.sw.need_pipeline ? "true" : "false");

      svga->state.sw.need_swtnl = need_swtnl;
      svga->dirty |= SVGA_NEW_NEED_SWTNL;
      svga->swtnl.new_vdecl = TRUE;
   }
  
   return PIPE_OK;
}


struct svga_tracked_state svga_update_need_swtnl =
{
   "need swtnl",
   (SVGA_NEW_NEED_PIPELINE |
    SVGA_NEW_NEED_SWVFETCH),
   update_need_swtnl
};
