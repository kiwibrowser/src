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

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_util.h"
#include "program/prog_parameter.h"
#include "program/prog_statevars.h"
#include "intel_batchbuffer.h"

static void
upload_vs_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   uint32_t floating_point_mode = 0;
   const int max_threads_shift = brw->intel.is_haswell ?
      HSW_VS_MAX_THREADS_SHIFT : GEN6_VS_MAX_THREADS_SHIFT;

   gen7_emit_vs_workaround_flush(intel);

   /* BRW_NEW_VS_BINDING_TABLE */
   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_BINDING_TABLE_POINTERS_VS << 16 | (2 - 2));
   OUT_BATCH(brw->vs.bind_bo_offset);
   ADVANCE_BATCH();

   /* CACHE_NEW_SAMPLER */
   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_SAMPLER_STATE_POINTERS_VS << 16 | (2 - 2));
   OUT_BATCH(brw->sampler.offset);
   ADVANCE_BATCH();

   if (brw->vs.push_const_size == 0) {
      /* Disable the push constant buffers. */
      BEGIN_BATCH(7);
      OUT_BATCH(_3DSTATE_CONSTANT_VS << 16 | (7 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(7);
      OUT_BATCH(_3DSTATE_CONSTANT_VS << 16 | (7 - 2));
      OUT_BATCH(brw->vs.push_const_size);
      OUT_BATCH(0);
      /* Pointer to the VS constant buffer.  Covered by the set of
       * state flags from gen6_prepare_wm_contants
       */
      OUT_BATCH(brw->vs.push_const_offset);
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

   OUT_BATCH(((brw->max_vs_threads - 1) << max_threads_shift) |
	     GEN6_VS_STATISTICS_ENABLE |
	     GEN6_VS_ENABLE);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_vs_state = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM | _NEW_PROGRAM_CONSTANTS,
      .brw   = (BRW_NEW_CONTEXT |
		BRW_NEW_VERTEX_PROGRAM |
		BRW_NEW_VS_BINDING_TABLE |
		BRW_NEW_BATCH),
      .cache = CACHE_NEW_VS_PROG | CACHE_NEW_SAMPLER
   },
   .emit = upload_vs_state,
};
