/*
 * Copyright © 2011 Intel Corporation
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
 */

/** \file gen6_sol.c
 *
 * Code to initialize the binding table entries used by transform feedback.
 */

#include "main/macros.h"
#include "brw_context.h"
#include "intel_batchbuffer.h"
#include "brw_defines.h"
#include "brw_state.h"

static void
gen6_update_sol_surfaces(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   /* _NEW_TRANSFORM_FEEDBACK */
   struct gl_transform_feedback_object *xfb_obj =
      ctx->TransformFeedback.CurrentObject;
   /* BRW_NEW_VERTEX_PROGRAM */
   const struct gl_shader_program *shaderprog =
      ctx->Shader.CurrentVertexProgram;
   const struct gl_transform_feedback_info *linked_xfb_info =
      &shaderprog->LinkedTransformFeedback;
   int i;

   for (i = 0; i < BRW_MAX_SOL_BINDINGS; ++i) {
      const int surf_index = SURF_INDEX_SOL_BINDING(i);
      if (xfb_obj->Active && !xfb_obj->Paused &&
          i < linked_xfb_info->NumOutputs) {
         unsigned buffer = linked_xfb_info->Outputs[i].OutputBuffer;
         unsigned buffer_offset =
            xfb_obj->Offset[buffer] / 4 +
            linked_xfb_info->Outputs[i].DstOffset;
         brw_update_sol_surface(
            brw, xfb_obj->Buffers[buffer], &brw->gs.surf_offset[surf_index],
            linked_xfb_info->Outputs[i].NumComponents,
            linked_xfb_info->BufferStride[buffer], buffer_offset);
      } else {
         brw->gs.surf_offset[surf_index] = 0;
      }
   }

   brw->state.dirty.brw |= BRW_NEW_SURFACES;
}

const struct brw_tracked_state gen6_sol_surface = {
   .dirty = {
      .mesa = _NEW_TRANSFORM_FEEDBACK,
      .brw = (BRW_NEW_BATCH |
              BRW_NEW_VERTEX_PROGRAM),
      .cache = 0
   },
   .emit = gen6_update_sol_surfaces,
};

/**
 * Constructs the binding table for the WM surface state, which maps unit
 * numbers to surface state objects.
 */
static void
brw_gs_upload_binding_table(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   /* BRW_NEW_VERTEX_PROGRAM */
   const struct gl_shader_program *shaderprog =
      ctx->Shader.CurrentVertexProgram;
   bool has_surfaces = false;
   uint32_t *bind;

   if (shaderprog) {
      const struct gl_transform_feedback_info *linked_xfb_info =
	 &shaderprog->LinkedTransformFeedback;
      /* Currently we only ever upload surfaces for SOL. */
      has_surfaces = linked_xfb_info->NumOutputs != 0;
   }

   /* Skip making a binding table if we don't have anything to put in it. */
   if (!has_surfaces) {
      if (brw->gs.bind_bo_offset != 0) {
	 brw->state.dirty.brw |= BRW_NEW_GS_BINDING_TABLE;
	 brw->gs.bind_bo_offset = 0;
      }
      return;
   }

   /* Might want to calculate nr_surfaces first, to avoid taking up so much
    * space for the binding table.
    */
   bind = brw_state_batch(brw, AUB_TRACE_BINDING_TABLE,
			  sizeof(uint32_t) * BRW_MAX_GS_SURFACES,
			  32, &brw->gs.bind_bo_offset);

   /* BRW_NEW_SURFACES */
   memcpy(bind, brw->gs.surf_offset, BRW_MAX_GS_SURFACES * sizeof(uint32_t));

   brw->state.dirty.brw |= BRW_NEW_GS_BINDING_TABLE;
}

const struct brw_tracked_state gen6_gs_binding_table = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
	      BRW_NEW_VERTEX_PROGRAM |
	      BRW_NEW_SURFACES),
      .cache = 0
   },
   .emit = brw_gs_upload_binding_table,
};

static void
gen6_update_sol_indices(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_GS_SVB_INDEX << 16 | (4 - 2));
   OUT_BATCH(0);
   OUT_BATCH(brw->sol.svbi_0_starting_index); /* BRW_NEW_SOL_INDICES */
   OUT_BATCH(brw->sol.svbi_0_max_index); /* BRW_NEW_SOL_INDICES */
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen6_sol_indices = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
              BRW_NEW_SOL_INDICES),
      .cache = 0
   },
   .emit = gen6_update_sol_indices,
};

void
brw_begin_transform_feedback(struct gl_context *ctx, GLenum mode,
			     struct gl_transform_feedback_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   const struct gl_shader_program *vs_prog =
      ctx->Shader.CurrentVertexProgram;
   const struct gl_transform_feedback_info *linked_xfb_info =
      &vs_prog->LinkedTransformFeedback;
   struct gl_transform_feedback_object *xfb_obj =
      ctx->TransformFeedback.CurrentObject;

   unsigned max_index = 0xffffffff;

   /* Compute the maximum number of vertices that we can write without
    * overflowing any of the buffers currently being used for feedback.
    */
   for (int i = 0; i < BRW_MAX_SOL_BUFFERS; ++i) {
      unsigned stride = linked_xfb_info->BufferStride[i];

      /* Skip any inactive buffers, which have a stride of 0. */
      if (stride == 0)
	 continue;

      unsigned max_for_this_buffer = xfb_obj->Size[i] / (4 * stride);
      max_index = MIN2(max_index, max_for_this_buffer);
   }

   /* Initialize the SVBI 0 register to zero and set the maximum index.
    * These values will be sent to the hardware on the next draw.
    */
   brw->state.dirty.brw |= BRW_NEW_SOL_INDICES;
   brw->sol.svbi_0_starting_index = 0;
   brw->sol.svbi_0_max_index = max_index;
   brw->sol.offset_0_batch_start = 0;
}

void
brw_end_transform_feedback(struct gl_context *ctx,
                           struct gl_transform_feedback_object *obj)
{
   /* After EndTransformFeedback, it's likely that the client program will try
    * to draw using the contents of the transform feedback buffer as vertex
    * input.  In order for this to work, we need to flush the data through at
    * least the GS stage of the pipeline, and flush out the render cache.  For
    * simplicity, just do a full flush.
    */
   struct brw_context *brw = brw_context(ctx);
   struct intel_context *intel = &brw->intel;
   intel_batchbuffer_emit_mi_flush(intel);
}
