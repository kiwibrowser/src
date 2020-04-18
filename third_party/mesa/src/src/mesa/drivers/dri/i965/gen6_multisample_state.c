/*
 * Copyright © 2012 Intel Corporation
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

#include "intel_batchbuffer.h"

#include "brw_context.h"
#include "brw_defines.h"


/**
 * 3DSTATE_MULTISAMPLE
 */
void
gen6_emit_3dstate_multisample(struct brw_context *brw,
                              unsigned num_samples)
{
   struct intel_context *intel = &brw->intel;

   uint32_t number_of_multisamples = 0;
   uint32_t sample_positions_3210 = 0;
   uint32_t sample_positions_7654 = 0;

   switch (num_samples) {
   case 0:
   case 1:
      number_of_multisamples = MS_NUMSAMPLES_1;
      break;
   case 4:
      number_of_multisamples = MS_NUMSAMPLES_4;
      /* Sample positions:
       *   2 6 a e
       * 2   0
       * 6       1
       * a 2
       * e     3
       */
      sample_positions_3210 = 0xae2ae662;
      break;
   case 8:
      number_of_multisamples = MS_NUMSAMPLES_8;
      /* Sample positions are based on a solution to the "8 queens" puzzle.
       * Rationale: in a solution to the 8 queens puzzle, no two queens share
       * a row, column, or diagonal.  This is a desirable property for samples
       * in a multisampling pattern, because it ensures that the samples are
       * relatively uniformly distributed through the pixel.
       *
       * There are several solutions to the 8 queens puzzle (see
       * http://en.wikipedia.org/wiki/Eight_queens_puzzle).  This solution was
       * chosen because it has a queen close to the center; this should
       * improve the accuracy of centroid interpolation, since the hardware
       * implements centroid interpolation by choosing the centermost sample
       * that overlaps with the primitive being drawn.
       *
       * Note: from the Ivy Bridge PRM, Vol2 Part1 p304 (3DSTATE_MULTISAMPLE:
       * Programming Notes):
       *
       *     "When programming the sample offsets (for NUMSAMPLES_4 or _8 and
       *     MSRASTMODE_xxx_PATTERN), the order of the samples 0 to 3 (or 7
       *     for 8X) must have monotonically increasing distance from the
       *     pixel center. This is required to get the correct centroid
       *     computation in the device."
       *
       * Sample positions:
       *   1 3 5 7 9 b d f
       * 1     5
       * 3           2
       * 5               6
       * 7 4
       * 9       0
       * b             3
       * d         1
       * f   7
       */
      sample_positions_3210 = 0xdbb39d79;
      sample_positions_7654 = 0x3ff55117;
      break;
   default:
      assert(!"Unrecognized num_samples in gen6_emit_3dstate_multisample");
      break;
   }

   int len = intel->gen >= 7 ? 4 : 3;
   BEGIN_BATCH(len);
   OUT_BATCH(_3DSTATE_MULTISAMPLE << 16 | (len - 2));
   OUT_BATCH(MS_PIXEL_LOCATION_CENTER | number_of_multisamples);
   OUT_BATCH(sample_positions_3210);
   if (intel->gen >= 7)
      OUT_BATCH(sample_positions_7654);
   ADVANCE_BATCH();
}


/**
 * 3DSTATE_SAMPLE_MASK
 */
void
gen6_emit_3dstate_sample_mask(struct brw_context *brw,
                              unsigned num_samples, float coverage,
                              bool coverage_invert)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_SAMPLE_MASK << 16 | (2 - 2));
   if (num_samples > 1) {
      int coverage_int = (int) (num_samples * coverage + 0.5);
      uint32_t coverage_bits = (1 << coverage_int) - 1;
      if (coverage_invert)
         coverage_bits ^= (1 << num_samples) - 1;
      OUT_BATCH(coverage_bits);
   } else {
      OUT_BATCH(1);
   }
   ADVANCE_BATCH();
}


static void upload_multisample_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   float coverage = 1.0;
   float coverage_invert = false;

   /* _NEW_BUFFERS */
   unsigned num_samples = ctx->DrawBuffer->Visual.samples;

   /* _NEW_MULTISAMPLE */
   if (ctx->Multisample._Enabled && ctx->Multisample.SampleCoverage) {
      coverage = ctx->Multisample.SampleCoverageValue;
      coverage_invert = ctx->Multisample.SampleCoverageInvert;
   }

   /* 3DSTATE_MULTISAMPLE is nonpipelined. */
   intel_emit_post_sync_nonzero_flush(intel);

   gen6_emit_3dstate_multisample(brw, num_samples);
   gen6_emit_3dstate_sample_mask(brw, num_samples, coverage, coverage_invert);
}


const struct brw_tracked_state gen6_multisample_state = {
   .dirty = {
      .mesa = _NEW_BUFFERS |
              _NEW_MULTISAMPLE,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_multisample_state
};
