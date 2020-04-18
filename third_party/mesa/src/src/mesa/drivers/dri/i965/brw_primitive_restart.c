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
 *
 * Authors:
 *    Jordan Justen <jordan.l.justen@intel.com>
 *
 */

#include "main/imports.h"
#include "main/bufferobj.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_draw.h"

#include "intel_batchbuffer.h"

/**
 * Check if the hardware's cut index support can handle the primitive
 * restart index value.
 */
static bool
can_cut_index_handle_restart_index(struct gl_context *ctx,
                                   const struct _mesa_index_buffer *ib)
{
   struct intel_context *intel = intel_context(ctx);

   /* Haswell supports an arbitrary cut index. */
   if (intel->is_haswell)
      return true;

   bool cut_index_will_work;

   switch (ib->type) {
   case GL_UNSIGNED_BYTE:
      cut_index_will_work = (ctx->Array.RestartIndex & 0xff) == 0xff;
      break;
   case GL_UNSIGNED_SHORT:
      cut_index_will_work = (ctx->Array.RestartIndex & 0xffff) == 0xffff;
      break;
   case GL_UNSIGNED_INT:
      cut_index_will_work = ctx->Array.RestartIndex == 0xffffffff;
      break;
   default:
      cut_index_will_work = false;
      assert(0);
   }

   return cut_index_will_work;
}

/**
 * Check if the hardware's cut index support can handle the primitive
 * restart case.
 */
static bool
can_cut_index_handle_prims(struct gl_context *ctx,
                           const struct _mesa_prim *prim,
                           GLuint nr_prims,
                           const struct _mesa_index_buffer *ib)
{
   struct brw_context *brw = brw_context(ctx);

   if (brw->sol.counting_primitives_generated ||
       brw->sol.counting_primitives_written) {
      /* Counting primitives generated in hardware is not currently
       * supported, so take the software path. We need to investigate
       * the *_PRIMITIVES_COUNT registers to allow this to be handled
       * entirely in hardware.
       */
      return false;
   }

   if (!can_cut_index_handle_restart_index(ctx, ib)) {
      /* The primitive restart index can't be handled, so take
       * the software path
       */
      return false;
   }

   for ( ; nr_prims > 0; nr_prims--) {
      switch(prim->mode) {
      case GL_POINTS:
      case GL_LINES:
      case GL_LINE_STRIP:
      case GL_TRIANGLES:
      case GL_TRIANGLE_STRIP:
         /* Cut index supports these primitive types */
         break;
      default:
         /* Cut index does not support these primitive types */
      //case GL_LINE_LOOP:
      //case GL_TRIANGLE_FAN:
      //case GL_QUADS:
      //case GL_QUAD_STRIP:
      //case GL_POLYGON:
         return false;
      }
   }

   return true;
}

/**
 * Check if primitive restart is enabled, and if so, handle it properly.
 *
 * In some cases the support will be handled in software. When available
 * hardware will handle primitive restart.
 */
GLboolean
brw_handle_primitive_restart(struct gl_context *ctx,
                             const struct _mesa_prim *prim,
                             GLuint nr_prims,
                             const struct _mesa_index_buffer *ib)
{
   struct brw_context *brw = brw_context(ctx);

   /* We only need to handle cases where there is an index buffer. */
   if (ib == NULL) {
      return GL_FALSE;
   }

   /* If the driver has requested software handling of primitive restarts,
    * then the VBO module has already taken care of things, and we can
    * just draw as normal.
    */
   if (ctx->Const.PrimitiveRestartInSoftware) {
      return GL_FALSE;
   }

   /* If we have set the in_progress flag, then we are in the middle
    * of handling the primitive restart draw.
    */
   if (brw->prim_restart.in_progress) {
      return GL_FALSE;
   }

   /* If PrimitiveRestart is not enabled, then we aren't concerned about
    * handling this draw.
    */
   if (!(ctx->Array.PrimitiveRestart)) {
      return GL_FALSE;
   }

   /* Signal that we are in the process of handling the
    * primitive restart draw
    */
   brw->prim_restart.in_progress = true;

   if (can_cut_index_handle_prims(ctx, prim, nr_prims, ib)) {
      /* Cut index should work for primitive restart, so use it
       */
      brw->prim_restart.enable_cut_index = true;
      brw_draw_prims(ctx, prim, nr_prims, ib, GL_FALSE, -1, -1, NULL);
      brw->prim_restart.enable_cut_index = false;
   } else {
      /* Not all the primitive draw modes are supported by the cut index,
       * so take the software path
       */
      vbo_sw_primitive_restart(ctx, prim, nr_prims, ib);
   }

   brw->prim_restart.in_progress = false;

   /* The primitive restart draw was completed, so return true. */
   return GL_TRUE;
}

static void
haswell_upload_cut_index(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;

   /* Don't trigger on Ivybridge */
   if (!intel->is_haswell)
      return;

   const unsigned cut_index_setting =
      ctx->Array.PrimitiveRestart ? HSW_CUT_INDEX_ENABLE : 0;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_VF << 16 | cut_index_setting | (2 - 2));
   OUT_BATCH(ctx->Array.RestartIndex);
   ADVANCE_BATCH();
}

const struct brw_tracked_state haswell_cut_index = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM,
      .brw   = 0,
      .cache = 0,
   },
   .emit = haswell_upload_cut_index,
};
