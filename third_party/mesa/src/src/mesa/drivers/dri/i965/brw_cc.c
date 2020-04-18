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


#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_util.h"
#include "main/macros.h"
#include "intel_batchbuffer.h"

static void
brw_upload_cc_vp(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct brw_cc_viewport *ccv;

   ccv = brw_state_batch(brw, AUB_TRACE_CC_VP_STATE,
			 sizeof(*ccv), 32, &brw->cc.vp_offset);

   /* _NEW_TRANSFORM */
   if (ctx->Transform.DepthClamp) {
      /* _NEW_VIEWPORT */
      ccv->min_depth = MIN2(ctx->Viewport.Near, ctx->Viewport.Far);
      ccv->max_depth = MAX2(ctx->Viewport.Near, ctx->Viewport.Far);
   } else {
      ccv->min_depth = 0.0;
      ccv->max_depth = 1.0;
   }

   brw->state.dirty.cache |= CACHE_NEW_CC_VP;
}

const struct brw_tracked_state brw_cc_vp = {
   .dirty = {
      .mesa = _NEW_VIEWPORT | _NEW_TRANSFORM,
      .brw = BRW_NEW_BATCH,
      .cache = 0
   },
   .emit = brw_upload_cc_vp
};

/**
 * Modify blend function to force destination alpha to 1.0
 *
 * If \c function specifies a blend function that uses destination alpha,
 * replace it with a function that hard-wires destination alpha to 1.0.  This
 * is used when rendering to xRGB targets.
 */
static GLenum
fix_xRGB_alpha(GLenum function)
{
   switch (function) {
   case GL_DST_ALPHA:
      return GL_ONE;

   case GL_ONE_MINUS_DST_ALPHA:
   case GL_SRC_ALPHA_SATURATE:
      return GL_ZERO;
   }

   return function;
}

/**
 * Creates the state cache entry for the given CC unit key.
 */
static void upload_cc_unit(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &brw->intel.ctx;
   struct brw_cc_unit_state *cc;

   cc = brw_state_batch(brw, AUB_TRACE_CC_STATE,
			sizeof(*cc), 64, &brw->cc.state_offset);
   memset(cc, 0, sizeof(*cc));

   /* _NEW_STENCIL */
   if (ctx->Stencil._Enabled) {
      const unsigned back = ctx->Stencil._BackFace;

      cc->cc0.stencil_enable = 1;
      cc->cc0.stencil_func =
	 intel_translate_compare_func(ctx->Stencil.Function[0]);
      cc->cc0.stencil_fail_op =
	 intel_translate_stencil_op(ctx->Stencil.FailFunc[0]);
      cc->cc0.stencil_pass_depth_fail_op =
	 intel_translate_stencil_op(ctx->Stencil.ZFailFunc[0]);
      cc->cc0.stencil_pass_depth_pass_op =
	 intel_translate_stencil_op(ctx->Stencil.ZPassFunc[0]);
      cc->cc1.stencil_ref = ctx->Stencil.Ref[0];
      cc->cc1.stencil_write_mask = ctx->Stencil.WriteMask[0];
      cc->cc1.stencil_test_mask = ctx->Stencil.ValueMask[0];

      if (ctx->Stencil._TestTwoSide) {
	 cc->cc0.bf_stencil_enable = 1;
	 cc->cc0.bf_stencil_func =
	    intel_translate_compare_func(ctx->Stencil.Function[back]);
	 cc->cc0.bf_stencil_fail_op =
	    intel_translate_stencil_op(ctx->Stencil.FailFunc[back]);
	 cc->cc0.bf_stencil_pass_depth_fail_op =
	    intel_translate_stencil_op(ctx->Stencil.ZFailFunc[back]);
	 cc->cc0.bf_stencil_pass_depth_pass_op =
	    intel_translate_stencil_op(ctx->Stencil.ZPassFunc[back]);
	 cc->cc1.bf_stencil_ref = ctx->Stencil.Ref[back];
	 cc->cc2.bf_stencil_write_mask = ctx->Stencil.WriteMask[back];
	 cc->cc2.bf_stencil_test_mask = ctx->Stencil.ValueMask[back];
      }

      /* Not really sure about this:
       */
      if (ctx->Stencil.WriteMask[0] ||
	  (ctx->Stencil._TestTwoSide && ctx->Stencil.WriteMask[back]))
	 cc->cc0.stencil_write_enable = 1;
   }

   /* _NEW_COLOR */
   if (ctx->Color.ColorLogicOpEnabled && ctx->Color.LogicOp != GL_COPY) {
      cc->cc2.logicop_enable = 1;
      cc->cc5.logicop_func = intel_translate_logic_op(ctx->Color.LogicOp);
   } else if (ctx->Color.BlendEnabled) {
      GLenum eqRGB = ctx->Color.Blend[0].EquationRGB;
      GLenum eqA = ctx->Color.Blend[0].EquationA;
      GLenum srcRGB = ctx->Color.Blend[0].SrcRGB;
      GLenum dstRGB = ctx->Color.Blend[0].DstRGB;
      GLenum srcA = ctx->Color.Blend[0].SrcA;
      GLenum dstA = ctx->Color.Blend[0].DstA;

      /* If the renderbuffer is XRGB, we have to frob the blend function to
       * force the destination alpha to 1.0.  This means replacing GL_DST_ALPHA
       * with GL_ONE and GL_ONE_MINUS_DST_ALPHA with GL_ZERO.
       */
      if (ctx->DrawBuffer->Visual.alphaBits == 0) {
	 srcRGB = fix_xRGB_alpha(srcRGB);
	 srcA   = fix_xRGB_alpha(srcA);
	 dstRGB = fix_xRGB_alpha(dstRGB);
	 dstA   = fix_xRGB_alpha(dstA);
      }

      if (eqRGB == GL_MIN || eqRGB == GL_MAX) {
	 srcRGB = dstRGB = GL_ONE;
      }

      if (eqA == GL_MIN || eqA == GL_MAX) {
	 srcA = dstA = GL_ONE;
      }

      cc->cc6.dest_blend_factor = brw_translate_blend_factor(dstRGB);
      cc->cc6.src_blend_factor = brw_translate_blend_factor(srcRGB);
      cc->cc6.blend_function = brw_translate_blend_equation(eqRGB);

      cc->cc5.ia_dest_blend_factor = brw_translate_blend_factor(dstA);
      cc->cc5.ia_src_blend_factor = brw_translate_blend_factor(srcA);
      cc->cc5.ia_blend_function = brw_translate_blend_equation(eqA);

      cc->cc3.blend_enable = 1;
      cc->cc3.ia_blend_enable = (srcA != srcRGB ||
				dstA != dstRGB ||
				eqA != eqRGB);
   }

   if (ctx->Color.AlphaEnabled) {
      cc->cc3.alpha_test = 1;
      cc->cc3.alpha_test_func =
	 intel_translate_compare_func(ctx->Color.AlphaFunc);
      cc->cc3.alpha_test_format = BRW_ALPHATEST_FORMAT_UNORM8;

      UNCLAMPED_FLOAT_TO_UBYTE(cc->cc7.alpha_ref.ub[0], ctx->Color.AlphaRef);
   }

   if (ctx->Color.DitherFlag) {
      cc->cc5.dither_enable = 1;
      cc->cc6.y_dither_offset = 0;
      cc->cc6.x_dither_offset = 0;
   }

   /* _NEW_DEPTH */
   if (ctx->Depth.Test) {
      cc->cc2.depth_test = 1;
      cc->cc2.depth_test_function =
	 intel_translate_compare_func(ctx->Depth.Func);
      cc->cc2.depth_write_enable = ctx->Depth.Mask;
   }

   if (intel->stats_wm || unlikely(INTEL_DEBUG & DEBUG_STATS))
      cc->cc5.statistics_enable = 1;

   /* CACHE_NEW_CC_VP */
   cc->cc4.cc_viewport_state_offset = (intel->batch.bo->offset +
				       brw->cc.vp_offset) >> 5; /* reloc */

   brw->state.dirty.cache |= CACHE_NEW_CC_UNIT;

   /* Emit CC viewport relocation */
   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
			   (brw->cc.state_offset +
			    offsetof(struct brw_cc_unit_state, cc4)),
			   intel->batch.bo, brw->cc.vp_offset,
			   I915_GEM_DOMAIN_INSTRUCTION, 0);
}

const struct brw_tracked_state brw_cc_unit = {
   .dirty = {
      .mesa = _NEW_STENCIL | _NEW_COLOR | _NEW_DEPTH,
      .brw = BRW_NEW_BATCH,
      .cache = CACHE_NEW_CC_VP
   },
   .emit = upload_cc_unit,
};

static void upload_blend_constant_color(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;

   BEGIN_BATCH(5);
   OUT_BATCH(_3DSTATE_BLEND_CONSTANT_COLOR << 16 | (5-2));
   OUT_BATCH_F(ctx->Color.BlendColorUnclamped[0]);
   OUT_BATCH_F(ctx->Color.BlendColorUnclamped[1]);
   OUT_BATCH_F(ctx->Color.BlendColorUnclamped[2]);
   OUT_BATCH_F(ctx->Color.BlendColorUnclamped[3]);
   CACHED_BATCH();
}

const struct brw_tracked_state brw_blend_constant_color = {
   .dirty = {
      .mesa = _NEW_COLOR,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_blend_constant_color
};
