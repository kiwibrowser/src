/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "util/u_memory.h"
#include "pipe/p_shader_tokens.h"
#include "draw/draw_context.h"
#include "draw/draw_vertex.h"
#include "i915_context.h"
#include "i915_state.h"
#include "i915_debug.h"
#include "i915_fpc.h"
#include "i915_reg.h"

static uint find_mapping(const struct i915_fragment_shader* fs, int unit)
{
   int i;
   for (i = 0; i < I915_TEX_UNITS ; i++)
   {
      if (fs->generic_mapping[i] == unit)
         return i;
   }
   debug_printf("Mapping not found\n");
   return 0;
}



/***********************************************************************
 * Determine the hardware vertex layout.
 * Depends on vertex/fragment shader state.
 */
static void calculate_vertex_layout(struct i915_context *i915)
{
   const struct i915_fragment_shader *fs = i915->fs;
   const enum interp_mode colorInterp = i915->rasterizer->color_interp;
   struct vertex_info vinfo;
   boolean texCoords[I915_TEX_UNITS], colors[2], fog, needW, face;
   uint i;
   int src;

   memset(texCoords, 0, sizeof(texCoords));
   colors[0] = colors[1] = fog = needW = face = FALSE;
   memset(&vinfo, 0, sizeof(vinfo));

   /* Determine which fragment program inputs are needed.  Setup HW vertex
    * layout below, in the HW-specific attribute order.
    */
   for (i = 0; i < fs->info.num_inputs; i++) {
      switch (fs->info.input_semantic_name[i]) {
      case TGSI_SEMANTIC_POSITION:
         {
            uint unit = I915_SEMANTIC_POS;
            texCoords[find_mapping(fs, unit)] = TRUE;
         }
         break;
      case TGSI_SEMANTIC_COLOR:
         assert(fs->info.input_semantic_index[i] < 2);
         colors[fs->info.input_semantic_index[i]] = TRUE;
         break;
      case TGSI_SEMANTIC_GENERIC:
         {
            /* texcoords/varyings/other generic */
            uint unit = fs->info.input_semantic_index[i];

            texCoords[find_mapping(fs, unit)] = TRUE;
            needW = TRUE;
         }
         break;
      case TGSI_SEMANTIC_FOG:
         fog = TRUE;
         break;
      case TGSI_SEMANTIC_FACE:
         face = TRUE;
         break;
      default:
         debug_printf("Unknown input type %d\n", fs->info.input_semantic_name[i]);
         assert(0);
      }
   }


   /* pos */
   src = draw_find_shader_output(i915->draw, TGSI_SEMANTIC_POSITION, 0);
   if (needW) {
      draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_LINEAR, src);
      vinfo.hwfmt[0] |= S4_VFMT_XYZW;
      vinfo.attrib[0].emit = EMIT_4F;
   }
   else {
      draw_emit_vertex_attr(&vinfo, EMIT_3F, INTERP_LINEAR, src);
      vinfo.hwfmt[0] |= S4_VFMT_XYZ;
      vinfo.attrib[0].emit = EMIT_3F;
   }

   /* hardware point size */
   /* XXX todo */

   /* primary color */
   if (colors[0]) {
      src = draw_find_shader_output(i915->draw, TGSI_SEMANTIC_COLOR, 0);
      draw_emit_vertex_attr(&vinfo, EMIT_4UB_BGRA, colorInterp, src);
      vinfo.hwfmt[0] |= S4_VFMT_COLOR;
   }

   /* secondary color */
   if (colors[1]) {
      src = draw_find_shader_output(i915->draw, TGSI_SEMANTIC_COLOR, 1);
      draw_emit_vertex_attr(&vinfo, EMIT_4UB_BGRA, colorInterp, src);
      vinfo.hwfmt[0] |= S4_VFMT_SPEC_FOG;
   }

   /* fog coord, not fog blend factor */
   if (fog) {
      src = draw_find_shader_output(i915->draw, TGSI_SEMANTIC_FOG, 0);
      draw_emit_vertex_attr(&vinfo, EMIT_1F, INTERP_PERSPECTIVE, src);
      vinfo.hwfmt[0] |= S4_VFMT_FOG_PARAM;
   }

   /* texcoords/varyings */
   for (i = 0; i < I915_TEX_UNITS; i++) {
      uint hwtc;
      if (texCoords[i]) {
         hwtc = TEXCOORDFMT_4D;
         src = draw_find_shader_output(i915->draw, TGSI_SEMANTIC_GENERIC, fs->generic_mapping[i]);
         draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_PERSPECTIVE, src);
      }
      else {
         hwtc = TEXCOORDFMT_NOT_PRESENT;
      }
      vinfo.hwfmt[1] |= hwtc << (i * 4);
   }

   /* front/back face */
   if (face) {
      uint slot = find_mapping(fs, I915_SEMANTIC_FACE);
      debug_printf("Front/back face is broken\n");
      /* XXX Because of limitations in the draw module, currently src will be 0
       * for SEMANTIC_FACE, so this aliases to POS. We need to fix in the draw
       * module by adding an extra shader output.
       */
      src = draw_find_shader_output(i915->draw, TGSI_SEMANTIC_FACE, 0);
      draw_emit_vertex_attr(&vinfo, EMIT_1F, INTERP_CONSTANT, src);
      vinfo.hwfmt[1] &= ~(TEXCOORDFMT_NOT_PRESENT << (slot * 4));
      vinfo.hwfmt[1] |= TEXCOORDFMT_1D << (slot * 4);
   }

   draw_compute_vertex_size(&vinfo);

   if (memcmp(&i915->current.vertex_info, &vinfo, sizeof(vinfo))) {
      /* Need to set this flag so that the LIS2/4 registers get set.
       * It also means the i915_update_immediate() function must be called
       * after this one, in i915_update_derived().
       */
      i915->dirty |= I915_NEW_VERTEX_FORMAT;

      memcpy(&i915->current.vertex_info, &vinfo, sizeof(vinfo));
   }
}

struct i915_tracked_state i915_update_vertex_layout = {
   "vertex_layout",
   calculate_vertex_layout,
   I915_NEW_RASTERIZER | I915_NEW_FS | I915_NEW_VS
};



/***********************************************************************
 */
static struct i915_tracked_state *atoms[] = {
   &i915_update_vertex_layout,
   &i915_hw_samplers,
   &i915_hw_sampler_views,
   &i915_hw_immediate,
   &i915_hw_dynamic,
   &i915_hw_fs,
   &i915_hw_framebuffer,
   &i915_hw_dst_buf_vars,
   &i915_hw_constants,
   NULL,
};

void i915_update_derived(struct i915_context *i915)
{
   int i;

   if (I915_DBG_ON(DBG_ATOMS))
      i915_dump_dirty(i915, __FUNCTION__);

   for (i = 0; atoms[i]; i++)
      if (atoms[i]->dirty & i915->dirty)
         atoms[i]->update(i915);

   i915->dirty = 0;
}
