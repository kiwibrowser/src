/*
 * Copyright © 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_util.h"
#include "program/prog_parameter.h"
#include "program/prog_statevars.h"
#include "intel_batchbuffer.h"

static void
gen6_upload_vs_push_constants(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   /* _BRW_NEW_VERTEX_PROGRAM */
   const struct brw_vertex_program *vp =
      brw_vertex_program_const(brw->vertex_program);
   unsigned int nr_params = brw->vs.prog_data->nr_params / 4;
   bool uses_clip_distance = vp->program.UsesClipDistance;

   if (brw->vertex_program->IsNVProgram)
      _mesa_load_tracked_matrices(ctx);

   /* Updates the ParamaterValues[i] pointers for all parameters of the
    * basic type of PROGRAM_STATE_VAR.
    */
   /* XXX: Should this happen somewhere before to get our state flag set? */
   _mesa_load_state_parameters(ctx, vp->program.Base.Parameters);

   /* CACHE_NEW_VS_PROG | _NEW_TRANSFORM */
   if (brw->vs.prog_data->nr_params == 0 && !ctx->Transform.ClipPlanesEnabled) {
      brw->vs.push_const_size = 0;
   } else {
      int params_uploaded = 0;
      float *param;
      int i;

      param = brw_state_batch(brw, AUB_TRACE_VS_CONSTANTS,
			      (MAX_CLIP_PLANES + nr_params) *
			      4 * sizeof(float),
			      32, &brw->vs.push_const_offset);

      if (brw->vs.prog_data->uses_new_param_layout) {
	 for (i = 0; i < brw->vs.prog_data->nr_params; i++) {
	    *param = *brw->vs.prog_data->param[i];
	    param++;
	 }
	 params_uploaded += brw->vs.prog_data->nr_params / 4;
      } else {
         /* This should be loaded like any other param, but it's ad-hoc
          * until we redo the VS backend.
          */
         if (ctx->Transform.ClipPlanesEnabled != 0 && !uses_clip_distance) {
            gl_clip_plane *clip_planes = brw_select_clip_planes(ctx);
            int num_userclip_plane_consts
               = _mesa_logbase2(ctx->Transform.ClipPlanesEnabled) + 1;
            int num_floats = 4 * num_userclip_plane_consts;
            memcpy(param, clip_planes, num_floats * sizeof(float));
            param += num_floats;
            params_uploaded += num_userclip_plane_consts;
         }

         /* Align to a reg for convenience for brw_vs_emit.c */
         if (params_uploaded & 1) {
            param += 4;
            params_uploaded++;
         }

	 for (i = 0; i < vp->program.Base.Parameters->NumParameters; i++) {
	    if (brw->vs.constant_map[i] != -1) {
	       memcpy(param + brw->vs.constant_map[i] * 4,
		      vp->program.Base.Parameters->ParameterValues[i],
		      4 * sizeof(float));
	       params_uploaded++;
	    }
	 }
      }

      if (0) {
	 printf("VS constant buffer:\n");
	 for (i = 0; i < params_uploaded; i++) {
	    float *buf = param + i * 4;
	    printf("%d: %f %f %f %f\n",
		   i, buf[0], buf[1], buf[2], buf[3]);
	 }
      }

      brw->vs.push_const_size = (params_uploaded + 1) / 2;
      /* We can only push 32 registers of constants at a time. */
      assert(brw->vs.push_const_size <= 32);
   }
}

const struct brw_tracked_state gen6_vs_push_constants = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM | _NEW_PROGRAM_CONSTANTS,
      .brw   = (BRW_NEW_BATCH |
		BRW_NEW_VERTEX_PROGRAM),
      .cache = CACHE_NEW_VS_PROG,
   },
   .emit = gen6_upload_vs_push_constants,
};

static void
upload_vs_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   uint32_t floating_point_mode = 0;

   /* From the BSpec, Volume 2a, Part 3 "Vertex Shader", Section
    * 3DSTATE_VS, Dword 5.0 "VS Function Enable":
    *   [DevSNB] A pipeline flush must be programmed prior to a 3DSTATE_VS
    *   command that causes the VS Function Enable to toggle. Pipeline
    *   flush can be executed by sending a PIPE_CONTROL command with CS
    *   stall bit set and a post sync operation.
    */
   intel_emit_post_sync_nonzero_flush(intel);

   if (brw->vs.push_const_size == 0) {
      /* Disable the push constant buffers. */
      BEGIN_BATCH(5);
      OUT_BATCH(_3DSTATE_CONSTANT_VS << 16 | (5 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(5);
      OUT_BATCH(_3DSTATE_CONSTANT_VS << 16 |
		GEN6_CONSTANT_BUFFER_0_ENABLE |
		(5 - 2));
      /* Pointer to the VS constant buffer.  Covered by the set of
       * state flags from gen6_upload_vs_constants
       */
      OUT_BATCH(brw->vs.push_const_offset +
		brw->vs.push_const_size - 1);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* Use ALT floating point mode for ARB vertex programs, because they
    * require 0^0 == 1.
    */
   if (intel->ctx.Shader.CurrentVertexProgram == NULL)
      floating_point_mode = GEN6_VS_FLOATING_POINT_MODE_ALT;

   BEGIN_BATCH(6);
   OUT_BATCH(_3DSTATE_VS << 16 | (6 - 2));
   OUT_BATCH(brw->vs.prog_offset);
   OUT_BATCH(floating_point_mode |
	     ((ALIGN(brw->sampler.count, 4)/4) << GEN6_VS_SAMPLER_COUNT_SHIFT));

   if (brw->vs.prog_data->total_scratch) {
      OUT_RELOC(brw->vs.scratch_bo,
		I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		ffs(brw->vs.prog_data->total_scratch) - 11);
   } else {
      OUT_BATCH(0);
   }

   OUT_BATCH((1 << GEN6_VS_DISPATCH_START_GRF_SHIFT) |
	     (brw->vs.prog_data->urb_read_length << GEN6_VS_URB_READ_LENGTH_SHIFT) |
	     (0 << GEN6_VS_URB_ENTRY_READ_OFFSET_SHIFT));

   OUT_BATCH(((brw->max_vs_threads - 1) << GEN6_VS_MAX_THREADS_SHIFT) |
	     GEN6_VS_STATISTICS_ENABLE |
	     GEN6_VS_ENABLE);
   ADVANCE_BATCH();

   /* Based on my reading of the simulator, the VS constants don't get
    * pulled into the VS FF unit until an appropriate pipeline flush
    * happens, and instead the 3DSTATE_CONSTANT_VS packet just adds
    * references to them into a little FIFO.  The flushes are common,
    * but don't reliably happen between this and a 3DPRIMITIVE, causing
    * the primitive to use the wrong constants.  Then the FIFO
    * containing the constant setup gets added to again on the next
    * constants change, and eventually when a flush does happen the
    * unit is overwhelmed by constant changes and dies.
    *
    * To avoid this, send a PIPE_CONTROL down the line that will
    * update the unit immediately loading the constants.  The flush
    * type bits here were those set by the STATE_BASE_ADDRESS whose
    * move in a82a43e8d99e1715dd11c9c091b5ab734079b6a6 triggered the
    * bug reports that led to this workaround, and may be more than
    * what is strictly required to avoid the issue.
    */
   intel_emit_post_sync_nonzero_flush(intel);

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_PIPE_CONTROL | (4 - 2));
   OUT_BATCH(PIPE_CONTROL_DEPTH_STALL |
	     PIPE_CONTROL_INSTRUCTION_FLUSH |
	     PIPE_CONTROL_STATE_CACHE_INVALIDATE);
   OUT_BATCH(0); /* address */
   OUT_BATCH(0); /* write data */
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen6_vs_state = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM | _NEW_PROGRAM_CONSTANTS,
      .brw   = (BRW_NEW_CONTEXT |
		BRW_NEW_VERTEX_PROGRAM |
		BRW_NEW_BATCH),
      .cache = CACHE_NEW_VS_PROG | CACHE_NEW_SAMPLER
   },
   .emit = upload_vs_state,
};
