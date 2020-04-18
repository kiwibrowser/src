/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
      
#include "main/glheader.h"
#include "main/macros.h"
#include "main/enums.h"

#include "intel_batchbuffer.h"

#include "brw_defines.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_util.h"
#include "brw_state.h"
#include "brw_gs.h"

#include "glsl/ralloc.h"

static void compile_gs_prog( struct brw_context *brw,
			     struct brw_gs_prog_key *key )
{
   struct intel_context *intel = &brw->intel;
   struct brw_gs_compile c;
   const GLuint *program;
   void *mem_ctx;
   GLuint program_size;

   memset(&c, 0, sizeof(c));
   
   c.key = *key;
   c.vue_map = brw->vs.prog_data->vue_map;
   c.nr_regs = (c.vue_map.num_slots + 1)/2;

   mem_ctx = ralloc_context(NULL);
   
   /* Begin the compilation:
    */
   brw_init_compile(brw, &c.func, mem_ctx);

   c.func.single_program_flow = 1;

   /* For some reason the thread is spawned with only 4 channels
    * unmasked.  
    */
   brw_set_mask_control(&c.func, BRW_MASK_DISABLE);

   if (intel->gen >= 6) {
      unsigned num_verts;
      bool check_edge_flag;
      /* On Sandybridge, we use the GS for implementing transform feedback
       * (called "Stream Out" in the PRM).
       */
      switch (key->primitive) {
      case _3DPRIM_POINTLIST:
         num_verts = 1;
         check_edge_flag = false;
	 break;
      case _3DPRIM_LINELIST:
      case _3DPRIM_LINESTRIP:
      case _3DPRIM_LINELOOP:
         num_verts = 2;
         check_edge_flag = false;
	 break;
      case _3DPRIM_TRILIST:
      case _3DPRIM_TRIFAN:
      case _3DPRIM_TRISTRIP:
      case _3DPRIM_RECTLIST:
	 num_verts = 3;
         check_edge_flag = false;
         break;
      case _3DPRIM_QUADLIST:
      case _3DPRIM_QUADSTRIP:
      case _3DPRIM_POLYGON:
         num_verts = 3;
         check_edge_flag = true;
         break;
      default:
	 assert(!"Unexpected primitive type in Gen6 SOL program.");
	 return;
      }
      gen6_sol_program(&c, key, num_verts, check_edge_flag);
   } else {
      /* On Gen4-5, we use the GS to decompose certain types of primitives.
       * Note that primitives which don't require a GS program have already
       * been weeded out by now.
       */
      switch (key->primitive) {
      case _3DPRIM_QUADLIST:
	 brw_gs_quads( &c, key );
	 break;
      case _3DPRIM_QUADSTRIP:
	 brw_gs_quad_strip( &c, key );
	 break;
      case _3DPRIM_LINELOOP:
	 brw_gs_lines( &c );
	 break;
      default:
	 ralloc_free(mem_ctx);
	 return;
      }
   }

   /* get the program
    */
   program = brw_get_program(&c.func, &program_size);

   if (unlikely(INTEL_DEBUG & DEBUG_GS)) {
      int i;

      printf("gs:\n");
      for (i = 0; i < program_size / sizeof(struct brw_instruction); i++)
	 brw_disasm(stdout, &((struct brw_instruction *)program)[i],
		    intel->gen);
      printf("\n");
    }

   brw_upload_cache(&brw->cache, BRW_GS_PROG,
		    &c.key, sizeof(c.key),
		    program, program_size,
		    &c.prog_data, sizeof(c.prog_data),
		    &brw->gs.prog_offset, &brw->gs.prog_data);
   ralloc_free(mem_ctx);
}

static void populate_key( struct brw_context *brw,
			  struct brw_gs_prog_key *key )
{
   static const unsigned swizzle_for_offset[4] = {
      BRW_SWIZZLE4(0, 1, 2, 3),
      BRW_SWIZZLE4(1, 2, 3, 3),
      BRW_SWIZZLE4(2, 3, 3, 3),
      BRW_SWIZZLE4(3, 3, 3, 3)
   };

   struct gl_context *ctx = &brw->intel.ctx;
   struct intel_context *intel = &brw->intel;

   memset(key, 0, sizeof(*key));

   /* CACHE_NEW_VS_PROG (part of VUE map) */
   key->attrs = brw->vs.prog_data->outputs_written;

   /* BRW_NEW_PRIMITIVE */
   key->primitive = brw->primitive;

   /* _NEW_LIGHT */
   key->pv_first = (ctx->Light.ProvokingVertex == GL_FIRST_VERTEX_CONVENTION);
   if (key->primitive == _3DPRIM_QUADLIST && ctx->Light.ShadeModel != GL_FLAT) {
      /* Provide consistent primitive order with brw_set_prim's
       * optimization of single quads to trifans.
       */
      key->pv_first = true;
   }

   /* CACHE_NEW_VS_PROG (part of VUE map)*/
   key->userclip_active = brw->vs.prog_data->userclip;

   if (intel->gen >= 7) {
      /* On Gen7 and later, we don't use GS (yet). */
      key->need_gs_prog = false;
   } else if (intel->gen == 6) {
      /* On Gen6, GS is used for transform feedback. */
      /* _NEW_TRANSFORM_FEEDBACK */
      if (ctx->TransformFeedback.CurrentObject->Active &&
          !ctx->TransformFeedback.CurrentObject->Paused) {
         const struct gl_shader_program *shaderprog =
            ctx->Shader.CurrentVertexProgram;
         const struct gl_transform_feedback_info *linked_xfb_info =
            &shaderprog->LinkedTransformFeedback;
         int i;

         /* Make sure that the VUE slots won't overflow the unsigned chars in
          * key->transform_feedback_bindings[].
          */
         STATIC_ASSERT(BRW_VERT_RESULT_MAX <= 256);

         /* Make sure that we don't need more binding table entries than we've
          * set aside for use in transform feedback.  (We shouldn't, since we
          * set aside enough binding table entries to have one per component).
          */
         assert(linked_xfb_info->NumOutputs <= BRW_MAX_SOL_BINDINGS);

         key->need_gs_prog = true;
         key->num_transform_feedback_bindings = linked_xfb_info->NumOutputs;
         for (i = 0; i < key->num_transform_feedback_bindings; ++i) {
            key->transform_feedback_bindings[i] =
               linked_xfb_info->Outputs[i].OutputRegister;
            key->transform_feedback_swizzles[i] =
               swizzle_for_offset[linked_xfb_info->Outputs[i].ComponentOffset];
         }
      }
      /* On Gen6, GS is also used for rasterizer discard. */
      /* _NEW_RASTERIZER_DISCARD */
      if (ctx->RasterDiscard) {
         key->need_gs_prog = true;
         key->rasterizer_discard = true;
      }
   } else {
      /* Pre-gen6, GS is used to transform QUADLIST, QUADSTRIP, and LINELOOP
       * into simpler primitives.
       */
      key->need_gs_prog = (brw->primitive == _3DPRIM_QUADLIST ||
                           brw->primitive == _3DPRIM_QUADSTRIP ||
                           brw->primitive == _3DPRIM_LINELOOP);
   }
}

/* Calculate interpolants for triangle and line rasterization.
 */
static void
brw_upload_gs_prog(struct brw_context *brw)
{
   struct brw_gs_prog_key key;
   /* Populate the key:
    */
   populate_key(brw, &key);

   if (brw->gs.prog_active != key.need_gs_prog) {
      brw->state.dirty.cache |= CACHE_NEW_GS_PROG;
      brw->gs.prog_active = key.need_gs_prog;
   }

   if (brw->gs.prog_active) {
      if (!brw_search_cache(&brw->cache, BRW_GS_PROG,
			    &key, sizeof(key),
			    &brw->gs.prog_offset, &brw->gs.prog_data)) {
	 compile_gs_prog( brw, &key );
      }
   }
}


const struct brw_tracked_state brw_gs_prog = {
   .dirty = {
      .mesa  = (_NEW_LIGHT |
                _NEW_TRANSFORM_FEEDBACK |
                _NEW_RASTERIZER_DISCARD),
      .brw   = BRW_NEW_PRIMITIVE,
      .cache = CACHE_NEW_VS_PROG
   },
   .emit = brw_upload_gs_prog
};
