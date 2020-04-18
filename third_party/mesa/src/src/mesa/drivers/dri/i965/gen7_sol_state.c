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

/**
 * @file gen7_sol_state.c
 *
 * Controls the stream output logic (SOL) stage of the gen7 hardware, which is
 * used to implement GL_EXT_transform_feedback.
 */

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"

static void
upload_3dstate_so_buffers(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   /* BRW_NEW_VERTEX_PROGRAM */
   const struct gl_shader_program *vs_prog =
      ctx->Shader.CurrentVertexProgram;
   const struct gl_transform_feedback_info *linked_xfb_info =
      &vs_prog->LinkedTransformFeedback;
   /* _NEW_TRANSFORM_FEEDBACK */
   struct gl_transform_feedback_object *xfb_obj =
      ctx->TransformFeedback.CurrentObject;
   int i;

   /* Set up the up to 4 output buffers.  These are the ranges defined in the
    * gl_transform_feedback_object.
    */
   for (i = 0; i < 4; i++) {
      struct intel_buffer_object *bufferobj =
	 intel_buffer_object(xfb_obj->Buffers[i]);
      drm_intel_bo *bo;
      uint32_t start, end;
      uint32_t stride;

      if (!xfb_obj->Buffers[i]) {
	 /* The pitch of 0 in this command indicates that the buffer is
	  * unbound and won't be written to.
	  */
	 BEGIN_BATCH(4);
	 OUT_BATCH(_3DSTATE_SO_BUFFER << 16 | (4 - 2));
	 OUT_BATCH((i << SO_BUFFER_INDEX_SHIFT));
	 OUT_BATCH(0);
	 OUT_BATCH(0);
	 ADVANCE_BATCH();

	 continue;
      }

      bo = intel_bufferobj_buffer(intel, bufferobj, INTEL_WRITE_PART);
      stride = linked_xfb_info->BufferStride[i] * 4;

      start = xfb_obj->Offset[i];
      assert(start % 4 == 0);
      end = ALIGN(start + xfb_obj->Size[i], 4);
      assert(end <= bo->size);

      /* Offset the starting offset by the current vertex index into the
       * feedback buffer, offset register is always set to 0 at the start of the
       * batchbuffer.
       */
      start += brw->sol.offset_0_batch_start * stride;
      assert(start <= end);

      BEGIN_BATCH(4);
      OUT_BATCH(_3DSTATE_SO_BUFFER << 16 | (4 - 2));
      OUT_BATCH((i << SO_BUFFER_INDEX_SHIFT) | stride);
      OUT_RELOC(bo, I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER, start);
      OUT_RELOC(bo, I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER, end);
      ADVANCE_BATCH();
   }
}

/**
 * Outputs the 3DSTATE_SO_DECL_LIST command.
 *
 * The data output is a series of 64-bit entries containing a SO_DECL per
 * stream.  We only have one stream of rendering coming out of the GS unit, so
 * we only emit stream 0 (low 16 bits) SO_DECLs.
 */
static void
upload_3dstate_so_decl_list(struct brw_context *brw,
			    struct brw_vue_map *vue_map)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   /* BRW_NEW_VERTEX_PROGRAM */
   const struct gl_shader_program *vs_prog =
      ctx->Shader.CurrentVertexProgram;
   /* _NEW_TRANSFORM_FEEDBACK */
   const struct gl_transform_feedback_info *linked_xfb_info =
      &vs_prog->LinkedTransformFeedback;
   int i;
   uint16_t so_decl[128];
   int buffer_mask = 0;
   int next_offset[4] = {0, 0, 0, 0};

   STATIC_ASSERT(ARRAY_SIZE(so_decl) >= MAX_PROGRAM_OUTPUTS);

   /* Construct the list of SO_DECLs to be emitted.  The formatting of the
    * command is feels strange -- each dword pair contains a SO_DECL per stream.
    */
   for (i = 0; i < linked_xfb_info->NumOutputs; i++) {
      int buffer = linked_xfb_info->Outputs[i].OutputBuffer;
      uint16_t decl = 0;
      int vert_result = linked_xfb_info->Outputs[i].OutputRegister;
      unsigned component_mask =
         (1 << linked_xfb_info->Outputs[i].NumComponents) - 1;

      /* gl_PointSize is stored in VERT_RESULT_PSIZ.w. */
      if (vert_result == VERT_RESULT_PSIZ) {
         assert(linked_xfb_info->Outputs[i].NumComponents == 1);
         component_mask <<= 3;
      } else {
         component_mask <<= linked_xfb_info->Outputs[i].ComponentOffset;
      }

      buffer_mask |= 1 << buffer;

      decl |= buffer << SO_DECL_OUTPUT_BUFFER_SLOT_SHIFT;
      decl |= vue_map->vert_result_to_slot[vert_result] <<
	 SO_DECL_REGISTER_INDEX_SHIFT;
      decl |= component_mask << SO_DECL_COMPONENT_MASK_SHIFT;

      /* This assert should be true until GL_ARB_transform_feedback_instanced
       * is added and we start using the hole flag.
       */
      assert(linked_xfb_info->Outputs[i].DstOffset == next_offset[buffer]);

      next_offset[buffer] += linked_xfb_info->Outputs[i].NumComponents;

      so_decl[i] = decl;
   }

   BEGIN_BATCH(linked_xfb_info->NumOutputs * 2 + 3);
   OUT_BATCH(_3DSTATE_SO_DECL_LIST << 16 |
	     (linked_xfb_info->NumOutputs * 2 + 1));

   OUT_BATCH((buffer_mask << SO_STREAM_TO_BUFFER_SELECTS_0_SHIFT) |
	     (0 << SO_STREAM_TO_BUFFER_SELECTS_1_SHIFT) |
	     (0 << SO_STREAM_TO_BUFFER_SELECTS_2_SHIFT) |
	     (0 << SO_STREAM_TO_BUFFER_SELECTS_3_SHIFT));

   OUT_BATCH((linked_xfb_info->NumOutputs << SO_NUM_ENTRIES_0_SHIFT) |
	     (0 << SO_NUM_ENTRIES_1_SHIFT) |
	     (0 << SO_NUM_ENTRIES_2_SHIFT) |
	     (0 << SO_NUM_ENTRIES_3_SHIFT));

   for (i = 0; i < linked_xfb_info->NumOutputs; i++) {
      OUT_BATCH(so_decl[i]);
      OUT_BATCH(0);
   }

   ADVANCE_BATCH();
}

static void
upload_3dstate_streamout(struct brw_context *brw, bool active,
			 struct brw_vue_map *vue_map)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   /* _NEW_TRANSFORM_FEEDBACK */
   struct gl_transform_feedback_object *xfb_obj =
      ctx->TransformFeedback.CurrentObject;
   uint32_t dw1 = 0, dw2 = 0;
   int i;

   /* _NEW_RASTERIZER_DISCARD */
   if (ctx->RasterDiscard)
      dw1 |= SO_RENDERING_DISABLE;

   if (active) {
      int urb_entry_read_offset = 0;
      int urb_entry_read_length = (vue_map->num_slots + 1) / 2 -
	 urb_entry_read_offset;

      dw1 |= SO_FUNCTION_ENABLE;
      dw1 |= SO_STATISTICS_ENABLE;

      /* _NEW_LIGHT */
      if (ctx->Light.ProvokingVertex != GL_FIRST_VERTEX_CONVENTION)
	 dw1 |= SO_REORDER_TRAILING;

      for (i = 0; i < 4; i++) {
	 if (xfb_obj->Buffers[i]) {
	    dw1 |= SO_BUFFER_ENABLE(i);
	 }
      }

      /* We always read the whole vertex.  This could be reduced at some
       * point by reading less and offsetting the register index in the
       * SO_DECLs.
       */
      dw2 |= urb_entry_read_offset << SO_STREAM_0_VERTEX_READ_OFFSET_SHIFT;
      dw2 |= (urb_entry_read_length - 1) <<
	 SO_STREAM_0_VERTEX_READ_LENGTH_SHIFT;
   }

   BEGIN_BATCH(3);
   OUT_BATCH(_3DSTATE_STREAMOUT << 16 | (3 - 2));
   OUT_BATCH(dw1);
   OUT_BATCH(dw2);
   ADVANCE_BATCH();
}

static void
upload_sol_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   /* _NEW_TRANSFORM_FEEDBACK */
   struct gl_transform_feedback_object *xfb_obj =
      ctx->TransformFeedback.CurrentObject;
   bool active = xfb_obj->Active && !xfb_obj->Paused;

   if (active) {
      upload_3dstate_so_buffers(brw);
      /* CACHE_NEW_VS_PROG */
      upload_3dstate_so_decl_list(brw, &brw->vs.prog_data->vue_map);

      intel->batch.needs_sol_reset = true;
   }

   /* Finally, set up the SOL stage.  This command must always follow updates to
    * the nonpipelined SOL state (3DSTATE_SO_BUFFER, 3DSTATE_SO_DECL_LIST) or
    * MMIO register updates (current performed by the kernel at each batch
    * emit).
    */
   upload_3dstate_streamout(brw, active, &brw->vs.prog_data->vue_map);
}

const struct brw_tracked_state gen7_sol_state = {
   .dirty = {
      .mesa  = (_NEW_RASTERIZER_DISCARD |
		_NEW_LIGHT |
		_NEW_TRANSFORM_FEEDBACK),
      .brw   = (BRW_NEW_BATCH |
		BRW_NEW_VERTEX_PROGRAM),
      .cache = CACHE_NEW_VS_PROG,
   },
   .emit = upload_sol_state,
};

void
gen7_end_transform_feedback(struct gl_context *ctx,
			    struct gl_transform_feedback_object *obj)
{
   /* Because we have to rely on the kernel to reset our SO write offsets, and
    * we only get to do it once per batchbuffer, flush the batch after feedback
    * so another transform feedback can get the write offset reset it needs.
    *
    * This also covers any cache flushing required.
    */
   struct brw_context *brw = brw_context(ctx);
   struct intel_context *intel = &brw->intel;

   intel_batchbuffer_flush(intel);
}
