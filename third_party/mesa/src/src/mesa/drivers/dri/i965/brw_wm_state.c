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
                   


#include "intel_fbo.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_wm.h"

/***********************************************************************
 * WM unit - fragment programs and rasterization
 */

bool
brw_color_buffer_write_enabled(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   const struct gl_fragment_program *fp = brw->fragment_program;
   int i;

   /* _NEW_BUFFERS */
   for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
      struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[i];

      /* _NEW_COLOR */
      if (rb &&
	  (fp->Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_COLOR) ||
	   fp->Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DATA0 + i)) &&
	  (ctx->Color.ColorMask[i][0] ||
	   ctx->Color.ColorMask[i][1] ||
	   ctx->Color.ColorMask[i][2] ||
	   ctx->Color.ColorMask[i][3])) {
	 return true;
      }
   }

   return false;
}

/**
 * Setup wm hardware state.  See page 225 of Volume 2
 */
static void
brw_upload_wm_unit(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   const struct gl_fragment_program *fp = brw->fragment_program;
   struct brw_wm_unit_state *wm;

   wm = brw_state_batch(brw, AUB_TRACE_WM_STATE,
			sizeof(*wm), 32, &brw->wm.state_offset);
   memset(wm, 0, sizeof(*wm));

   if (brw->wm.prog_data->prog_offset_16) {
      /* These two fields should be the same pre-gen6, which is why we
       * only have one hardware field to program for both dispatch
       * widths.
       */
      assert(brw->wm.prog_data->first_curbe_grf ==
	     brw->wm.prog_data->first_curbe_grf_16);
   }

   /* BRW_NEW_PROGRAM_CACHE | CACHE_NEW_WM_PROG */
   wm->thread0.grf_reg_count = brw->wm.prog_data->reg_blocks;
   wm->wm9.grf_reg_count_2 = brw->wm.prog_data->reg_blocks_16;

   wm->thread0.kernel_start_pointer =
      brw_program_reloc(brw,
			brw->wm.state_offset +
			offsetof(struct brw_wm_unit_state, thread0),
			brw->wm.prog_offset +
			(wm->thread0.grf_reg_count << 1)) >> 6;

   wm->wm9.kernel_start_pointer_2 =
      brw_program_reloc(brw,
			brw->wm.state_offset +
			offsetof(struct brw_wm_unit_state, wm9),
			brw->wm.prog_offset +
			brw->wm.prog_data->prog_offset_16 +
			(wm->wm9.grf_reg_count_2 << 1)) >> 6;

   wm->thread1.depth_coef_urb_read_offset = 1;
   wm->thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;

   wm->thread1.binding_table_entry_count = 0;

   if (brw->wm.prog_data->total_scratch != 0) {
      wm->thread2.scratch_space_base_pointer =
	 brw->wm.scratch_bo->offset >> 10; /* reloc */
      wm->thread2.per_thread_scratch_space =
	 ffs(brw->wm.prog_data->total_scratch) - 11;
   } else {
      wm->thread2.scratch_space_base_pointer = 0;
      wm->thread2.per_thread_scratch_space = 0;
   }

   wm->thread3.dispatch_grf_start_reg = brw->wm.prog_data->first_curbe_grf;
   wm->thread3.urb_entry_read_length = brw->wm.prog_data->urb_read_length;
   wm->thread3.urb_entry_read_offset = 0;
   wm->thread3.const_urb_entry_read_length =
      brw->wm.prog_data->curb_read_length;
   /* BRW_NEW_CURBE_OFFSETS */
   wm->thread3.const_urb_entry_read_offset = brw->curbe.wm_start * 2;

   if (intel->gen == 5)
      wm->wm4.sampler_count = 0; /* hardware requirement */
   else {
      /* CACHE_NEW_SAMPLER */
      wm->wm4.sampler_count = (brw->sampler.count + 1) / 4;
   }

   if (brw->sampler.count) {
      /* reloc */
      wm->wm4.sampler_state_pointer = (intel->batch.bo->offset +
				       brw->sampler.offset) >> 5;
   } else {
      wm->wm4.sampler_state_pointer = 0;
   }

   /* BRW_NEW_FRAGMENT_PROGRAM */
   wm->wm5.program_uses_depth = (fp->Base.InputsRead &
				 (1 << FRAG_ATTRIB_WPOS)) != 0;
   wm->wm5.program_computes_depth = (fp->Base.OutputsWritten &
				     BITFIELD64_BIT(FRAG_RESULT_DEPTH)) != 0;
   /* _NEW_BUFFERS
    * Override for NULL depthbuffer case, required by the Pixel Shader Computed
    * Depth field.
    */
   if (!intel_get_renderbuffer(ctx->DrawBuffer, BUFFER_DEPTH))
      wm->wm5.program_computes_depth = 0;

   /* _NEW_COLOR */
   wm->wm5.program_uses_killpixel = fp->UsesKill || ctx->Color.AlphaEnabled;


   /* BRW_NEW_FRAGMENT_PROGRAM
    *
    * If using the fragment shader backend, the program is always
    * 8-wide.  If not, it's always 16.
    */
   if (ctx->Shader._CurrentFragmentProgram) {
      struct brw_shader *shader = (struct brw_shader *)
	 ctx->Shader._CurrentFragmentProgram->_LinkedShaders[MESA_SHADER_FRAGMENT];

      if (shader != NULL && shader->ir != NULL) {
	 wm->wm5.enable_8_pix = 1;
	 if (brw->wm.prog_data->prog_offset_16)
	    wm->wm5.enable_16_pix = 1;
      }
   }
   if (!wm->wm5.enable_8_pix)
      wm->wm5.enable_16_pix = 1;

   wm->wm5.max_threads = brw->max_wm_threads - 1;

   /* _NEW_BUFFERS | _NEW_COLOR */
   if (brw_color_buffer_write_enabled(brw) ||
       wm->wm5.program_uses_killpixel ||
       wm->wm5.program_computes_depth) {
      wm->wm5.thread_dispatch_enable = 1;
   }

   wm->wm5.legacy_line_rast = 0;
   wm->wm5.legacy_global_depth_bias = 0;
   wm->wm5.early_depth_test = 1;	        /* never need to disable */
   wm->wm5.line_aa_region_width = 0;
   wm->wm5.line_endcap_aa_region_width = 1;

   /* _NEW_POLYGONSTIPPLE */
   wm->wm5.polygon_stipple = ctx->Polygon.StippleFlag;

   /* _NEW_POLYGON */
   if (ctx->Polygon.OffsetFill) {
      wm->wm5.depth_offset = 1;
      /* Something wierd going on with legacy_global_depth_bias,
       * offset_constant, scaling and MRD.  This value passes glean
       * but gives some odd results elsewere (eg. the
       * quad-offset-units test).
       */
      wm->global_depth_offset_constant = ctx->Polygon.OffsetUnits * 2;

      /* This is the only value that passes glean:
       */
      wm->global_depth_offset_scale = ctx->Polygon.OffsetFactor;
   }

   /* _NEW_LINE */
   wm->wm5.line_stipple = ctx->Line.StippleFlag;

   /* _NEW_DEPTH */
   if (unlikely(INTEL_DEBUG & DEBUG_STATS) || intel->stats_wm)
      wm->wm4.stats_enable = 1;

   /* Emit scratch space relocation */
   if (brw->wm.prog_data->total_scratch != 0) {
      drm_intel_bo_emit_reloc(intel->batch.bo,
			      brw->wm.state_offset +
			      offsetof(struct brw_wm_unit_state, thread2),
			      brw->wm.scratch_bo,
			      wm->thread2.per_thread_scratch_space,
			      I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER);
   }

   /* Emit sampler state relocation */
   if (brw->sampler.count != 0) {
      drm_intel_bo_emit_reloc(intel->batch.bo,
			      brw->wm.state_offset +
			      offsetof(struct brw_wm_unit_state, wm4),
			      intel->batch.bo, (brw->sampler.offset |
						wm->wm4.stats_enable |
						(wm->wm4.sampler_count << 2)),
			      I915_GEM_DOMAIN_INSTRUCTION, 0);
   }

   brw->state.dirty.cache |= CACHE_NEW_WM_UNIT;
}

const struct brw_tracked_state brw_wm_unit = {
   .dirty = {
      .mesa = (_NEW_POLYGON | 
	       _NEW_POLYGONSTIPPLE | 
	       _NEW_LINE | 
	       _NEW_COLOR |
	       _NEW_DEPTH |
	       _NEW_BUFFERS),

      .brw = (BRW_NEW_BATCH |
	      BRW_NEW_PROGRAM_CACHE |
	      BRW_NEW_FRAGMENT_PROGRAM |
	      BRW_NEW_CURBE_OFFSETS),

      .cache = (CACHE_NEW_WM_PROG |
		CACHE_NEW_SAMPLER)
   },
   .emit = brw_upload_wm_unit,
};

