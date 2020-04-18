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

#include "intel_fbo.h"
#include "brw_context.h"
#include "brw_state.h"

static void
gen6_upload_depth_stencil_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct gen6_depth_stencil_state *ds;
   struct intel_renderbuffer *depth_irb;

   /* _NEW_BUFFERS */
   depth_irb = intel_get_renderbuffer(ctx->DrawBuffer, BUFFER_DEPTH);

   ds = brw_state_batch(brw, AUB_TRACE_DEPTH_STENCIL_STATE,
			sizeof(*ds), 64,
			&brw->cc.depth_stencil_state_offset);
   memset(ds, 0, sizeof(*ds));

   /* _NEW_STENCIL */
   if (ctx->Stencil._Enabled) {
      int back = ctx->Stencil._BackFace;

      ds->ds0.stencil_enable = 1;
      ds->ds0.stencil_func =
	 intel_translate_compare_func(ctx->Stencil.Function[0]);
      ds->ds0.stencil_fail_op =
	 intel_translate_stencil_op(ctx->Stencil.FailFunc[0]);
      ds->ds0.stencil_pass_depth_fail_op =
	 intel_translate_stencil_op(ctx->Stencil.ZFailFunc[0]);
      ds->ds0.stencil_pass_depth_pass_op =
	 intel_translate_stencil_op(ctx->Stencil.ZPassFunc[0]);
      ds->ds1.stencil_write_mask = ctx->Stencil.WriteMask[0];
      ds->ds1.stencil_test_mask = ctx->Stencil.ValueMask[0];

      if (ctx->Stencil._TestTwoSide) {
	 ds->ds0.bf_stencil_enable = 1;
	 ds->ds0.bf_stencil_func =
	    intel_translate_compare_func(ctx->Stencil.Function[back]);
	 ds->ds0.bf_stencil_fail_op =
	    intel_translate_stencil_op(ctx->Stencil.FailFunc[back]);
	 ds->ds0.bf_stencil_pass_depth_fail_op =
	    intel_translate_stencil_op(ctx->Stencil.ZFailFunc[back]);
	 ds->ds0.bf_stencil_pass_depth_pass_op =
	    intel_translate_stencil_op(ctx->Stencil.ZPassFunc[back]);
	 ds->ds1.bf_stencil_write_mask = ctx->Stencil.WriteMask[back];
	 ds->ds1.bf_stencil_test_mask = ctx->Stencil.ValueMask[back];
      }

      /* Not really sure about this:
       */
      if (ctx->Stencil.WriteMask[0] ||
	  (ctx->Stencil._TestTwoSide && ctx->Stencil.WriteMask[back]))
	 ds->ds0.stencil_write_enable = 1;
   }

   /* _NEW_DEPTH */
   if (ctx->Depth.Test && depth_irb) {
      ds->ds2.depth_test_enable = ctx->Depth.Test;
      ds->ds2.depth_test_func = intel_translate_compare_func(ctx->Depth.Func);
      ds->ds2.depth_write_enable = ctx->Depth.Mask;
   }

   brw->state.dirty.cache |= CACHE_NEW_DEPTH_STENCIL_STATE;
}

const struct brw_tracked_state gen6_depth_stencil_state = {
   .dirty = {
      .mesa = _NEW_DEPTH | _NEW_STENCIL | _NEW_BUFFERS,
      .brw  = BRW_NEW_BATCH,
      .cache = 0,
   },
   .emit = gen6_upload_depth_stencil_state,
};
