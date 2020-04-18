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
#include "intel_batchbuffer.h"

static void
upload_gs_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   /* Disable all the constant buffers. */
   BEGIN_BATCH(5);
   OUT_BATCH(_3DSTATE_CONSTANT_GS << 16 | (5 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();

   if (brw->gs.prog_active) {
      BEGIN_BATCH(7);
      OUT_BATCH(_3DSTATE_GS << 16 | (7 - 2));
      OUT_BATCH(brw->gs.prog_offset);
      OUT_BATCH(GEN6_GS_SPF_MODE | GEN6_GS_VECTOR_MASK_ENABLE);
      OUT_BATCH(0); /* no scratch space */
      OUT_BATCH((2 << GEN6_GS_DISPATCH_START_GRF_SHIFT) |
	        (brw->gs.prog_data->urb_read_length << GEN6_GS_URB_READ_LENGTH_SHIFT));
      OUT_BATCH(((brw->max_gs_threads - 1) << GEN6_GS_MAX_THREADS_SHIFT) |
	        GEN6_GS_STATISTICS_ENABLE |
		GEN6_GS_SO_STATISTICS_ENABLE |
		GEN6_GS_RENDERING_ENABLE);
      OUT_BATCH(GEN6_GS_SVBI_PAYLOAD_ENABLE |
                GEN6_GS_SVBI_POSTINCREMENT_ENABLE |
                (brw->gs.prog_data->svbi_postincrement_value <<
                 GEN6_GS_SVBI_POSTINCREMENT_VALUE_SHIFT) |
                GEN6_GS_ENABLE);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(7);
      OUT_BATCH(_3DSTATE_GS << 16 | (7 - 2));
      OUT_BATCH(0); /* prog_bo */
      OUT_BATCH((0 << GEN6_GS_SAMPLER_COUNT_SHIFT) |
		(0 << GEN6_GS_BINDING_TABLE_ENTRY_COUNT_SHIFT));
      OUT_BATCH(0); /* scratch space base offset */
      OUT_BATCH((1 << GEN6_GS_DISPATCH_START_GRF_SHIFT) |
		(0 << GEN6_GS_URB_READ_LENGTH_SHIFT) |
		(0 << GEN6_GS_URB_ENTRY_READ_OFFSET_SHIFT));
      OUT_BATCH((0 << GEN6_GS_MAX_THREADS_SHIFT) |
		GEN6_GS_STATISTICS_ENABLE |
		GEN6_GS_RENDERING_ENABLE);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }
}

const struct brw_tracked_state gen6_gs_state = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM,
      .brw   = BRW_NEW_CONTEXT,
      .cache = CACHE_NEW_GS_PROG
   },
   .emit = upload_gs_state,
};
