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
#include "intel_batchbuffer.h"
#include "main/macros.h"

static void
upload_cc_state_pointers(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_CC_STATE_POINTERS << 16 | (2 - 2));
   OUT_BATCH(brw->cc.state_offset | 1);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_cc_state_pointer = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH,
      .cache = CACHE_NEW_COLOR_CALC_STATE
   },
   .emit = upload_cc_state_pointers,
};

static void
upload_blend_state_pointer(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_BLEND_STATE_POINTERS << 16 | (2 - 2));
   OUT_BATCH(brw->cc.blend_state_offset | 1);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_blend_state_pointer = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH,
      .cache = CACHE_NEW_BLEND_STATE
   },
   .emit = upload_blend_state_pointer,
};

static void
upload_depth_stencil_state_pointer(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_DEPTH_STENCIL_STATE_POINTERS << 16 | (2 - 2));
   OUT_BATCH(brw->cc.depth_stencil_state_offset | 1);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_depth_stencil_state_pointer = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH,
      .cache = CACHE_NEW_DEPTH_STENCIL_STATE
   },
   .emit = upload_depth_stencil_state_pointer,
};
