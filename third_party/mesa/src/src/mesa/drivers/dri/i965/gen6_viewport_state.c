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
#include "main/fbobject.h"

/* The clip VP defines the guardband region where expensive clipping is skipped
 * and fragments are allowed to be generated and clipped out cheaply by the SF.
 *
 * By setting it to NDC bounds of [-1,1], we don't do GB clipping.  It's
 * supposed to cause seams to become visible in apps due to shared edges taking
 * different clip/no clip paths depending on whether the rest of the prim ends
 * up in the guardband or not.
 */
static void
gen6_upload_clip_vp(struct brw_context *brw)
{
   struct brw_clipper_viewport *vp;

   vp = brw_state_batch(brw, AUB_TRACE_CLIP_VP_STATE,
			sizeof(*vp), 32, &brw->clip.vp_offset);

   vp->xmin = -1.0;
   vp->xmax = 1.0;
   vp->ymin = -1.0;
   vp->ymax = 1.0;

   brw->state.dirty.cache |= CACHE_NEW_CLIP_VP;
}

const struct brw_tracked_state gen6_clip_vp = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH,
      .cache = 0,
   },
   .emit = gen6_upload_clip_vp,
};

static void
gen6_upload_sf_vp(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   const GLfloat depth_scale = 1.0F / ctx->DrawBuffer->_DepthMaxF;
   struct brw_sf_viewport *sfv;
   GLfloat y_scale, y_bias;
   const bool render_to_fbo = _mesa_is_user_fbo(ctx->DrawBuffer);
   const GLfloat *v = ctx->Viewport._WindowMap.m;

   sfv = brw_state_batch(brw, AUB_TRACE_SF_VP_STATE,
			 sizeof(*sfv), 32, &brw->sf.vp_offset);
   memset(sfv, 0, sizeof(*sfv));

   /* _NEW_BUFFERS */
   if (render_to_fbo) {
      y_scale = 1.0;
      y_bias = 0;
   } else {
      y_scale = -1.0;
      y_bias = ctx->DrawBuffer->Height;
   }

   /* _NEW_VIEWPORT */
   sfv->viewport.m00 = v[MAT_SX];
   sfv->viewport.m11 = v[MAT_SY] * y_scale;
   sfv->viewport.m22 = v[MAT_SZ] * depth_scale;
   sfv->viewport.m30 = v[MAT_TX];
   sfv->viewport.m31 = v[MAT_TY] * y_scale + y_bias;
   sfv->viewport.m32 = v[MAT_TZ] * depth_scale;

   brw->state.dirty.cache |= CACHE_NEW_SF_VP;
}

const struct brw_tracked_state gen6_sf_vp = {
   .dirty = {
      .mesa = _NEW_VIEWPORT | _NEW_BUFFERS,
      .brw = BRW_NEW_BATCH,
      .cache = 0,
   },
   .emit = gen6_upload_sf_vp,
};

static void upload_viewport_state_pointers(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_VIEWPORT_STATE_POINTERS << 16 | (4 - 2) |
	     GEN6_CC_VIEWPORT_MODIFY |
	     GEN6_SF_VIEWPORT_MODIFY |
	     GEN6_CLIP_VIEWPORT_MODIFY);
   OUT_BATCH(brw->clip.vp_offset);
   OUT_BATCH(brw->sf.vp_offset);
   OUT_BATCH(brw->cc.vp_offset);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen6_viewport_state = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
	      BRW_NEW_STATE_BASE_ADDRESS),
      .cache = (CACHE_NEW_CLIP_VP |
		CACHE_NEW_SF_VP |
		CACHE_NEW_CC_VP)
   },
   .emit = upload_viewport_state_pointers,
};
