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

#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/colormac.h"
#include "main/renderbuffer.h"
#include "main/framebuffer.h"

#include "intel_batchbuffer.h" 
#include "intel_regions.h" 
#include "intel_fbo.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "brw_draw.h"
#include "brw_vs.h"
#include "brw_wm.h"

#include "gen6_blorp.h"
#include "gen7_blorp.h"

#include "glsl/ralloc.h"

static void
dri_bo_release(drm_intel_bo **bo)
{
   drm_intel_bo_unreference(*bo);
   *bo = NULL;
}


/**
 * called from intelDestroyContext()
 */
static void brw_destroy_context( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);

   brw_destroy_state(brw);
   brw_draw_destroy( brw );

   ralloc_free(brw->wm.compile_data);

   dri_bo_release(&brw->curbe.curbe_bo);
   dri_bo_release(&brw->vs.const_bo);
   dri_bo_release(&brw->wm.const_bo);

   free(brw->curbe.last_buf);
   free(brw->curbe.next_buf);

   drm_intel_gem_context_destroy(intel->hw_ctx);
}

/**
 * Update the hardware state for drawing into a window or framebuffer object.
 *
 * Called by glDrawBuffer, glBindFramebufferEXT, MakeCurrent, and other
 * places within the driver.
 *
 * Basically, this needs to be called any time the current framebuffer
 * changes, the renderbuffers change, or we need to draw into different
 * color buffers.
 */
static void
brw_update_draw_buffer(struct intel_context *intel)
{
   struct gl_context *ctx = &intel->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;

   if (!fb) {
      /* this can happen during the initial context initialization */
      return;
   }

   /* Do this here, not core Mesa, since this function is called from
    * many places within the driver.
    */
   if (ctx->NewState & _NEW_BUFFERS) {
      /* this updates the DrawBuffer->_NumColorDrawBuffers fields, etc */
      _mesa_update_framebuffer(ctx);
      /* this updates the DrawBuffer's Width/Height if it's a FBO */
      _mesa_update_draw_buffer_bounds(ctx);
   }

   if (fb->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      /* this may occur when we're called by glBindFrameBuffer() during
       * the process of someone setting up renderbuffers, etc.
       */
      /*_mesa_debug(ctx, "DrawBuffer: incomplete user FBO\n");*/
      return;
   }

   /* Mesa's Stencil._Enabled field is updated when
    * _NEW_BUFFERS | _NEW_STENCIL, but i965 code assumes that the value
    * only changes with _NEW_STENCIL (which seems sensible).  So flag it
    * here since this is the _NEW_BUFFERS path.
    */
   intel->NewGLState |= (_NEW_DEPTH | _NEW_STENCIL);

   /* The driver uses this in places that need to look up
    * renderbuffers' buffer objects.
    */
   intel->NewGLState |= _NEW_BUFFERS;

   /* update viewport/scissor since it depends on window size */
   intel->NewGLState |= _NEW_VIEWPORT | _NEW_SCISSOR;

   /* Update culling direction which changes depending on the
    * orientation of the buffer:
    */
   intel->NewGLState |= _NEW_POLYGON;
}

/**
 * called from intel_batchbuffer_flush and children before sending a
 * batchbuffer off.
 *
 * Note that ALL state emitted here must fit in the reserved space
 * at the end of a batchbuffer.  If you add more GPU state, increase
 * the BATCH_RESERVED macro.
 */
static void brw_finish_batch(struct intel_context *intel)
{
   struct brw_context *brw = brw_context(&intel->ctx);
   brw_emit_query_end(brw);

   if (brw->curbe.curbe_bo) {
      drm_intel_gem_bo_unmap_gtt(brw->curbe.curbe_bo);
      drm_intel_bo_unreference(brw->curbe.curbe_bo);
      brw->curbe.curbe_bo = NULL;
   }
}


/**
 * called from intelFlushBatchLocked
 */
static void brw_new_batch( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);

   /* If the kernel supports hardware contexts, then most hardware state is
    * preserved between batches; we only need to re-emit state that is required
    * to be in every batch.  Otherwise we need to re-emit all the state that
    * would otherwise be stored in the context (which for all intents and
    * purposes means everything).
    */
   if (intel->hw_ctx == NULL)
      brw->state.dirty.brw |= BRW_NEW_CONTEXT;

   brw->state.dirty.brw |= BRW_NEW_BATCH;

   /* Assume that the last command before the start of our batch was a
    * primitive, for safety.
    */
   intel->batch.need_workaround_flush = true;

   brw->state_batch_count = 0;

   /* Gen7 needs to track what the real transform feedback vertex count was at
    * the start of the batch, since the kernel will be resetting the offset to
    * 0.
    */
   brw->sol.offset_0_batch_start = brw->sol.svbi_0_starting_index;

   brw->vb.nr_current_buffers = 0;
   brw->ib.type = -1;

   /* Mark that the current program cache BO has been used by the GPU.
    * It will be reallocated if we need to put new programs in for the
    * next batch.
    */
   brw->cache.bo_used_by_gpu = true;
}

static void brw_invalidate_state( struct intel_context *intel, GLuint new_state )
{
   /* nothing */
}

/**
 * \see intel_context.vtbl.is_hiz_depth_format
 */
static bool brw_is_hiz_depth_format(struct intel_context *intel,
                                    gl_format format)
{
   if (!intel->has_hiz)
      return false;

   switch (format) {
   case MESA_FORMAT_Z32_FLOAT:
   case MESA_FORMAT_Z32_FLOAT_X24S8:
   case MESA_FORMAT_X8_Z24:
   case MESA_FORMAT_S8_Z24:
      return true;
   default:
      return false;
   }
}

void brwInitVtbl( struct brw_context *brw )
{
   brw->intel.vtbl.check_vertex_size = 0;
   brw->intel.vtbl.emit_state = 0;
   brw->intel.vtbl.reduced_primitive_state = 0;
   brw->intel.vtbl.render_start = 0;
   brw->intel.vtbl.update_texture_state = 0;

   brw->intel.vtbl.invalidate_state = brw_invalidate_state;
   brw->intel.vtbl.new_batch = brw_new_batch;
   brw->intel.vtbl.finish_batch = brw_finish_batch;
   brw->intel.vtbl.destroy = brw_destroy_context;
   brw->intel.vtbl.update_draw_buffer = brw_update_draw_buffer;
   brw->intel.vtbl.debug_batch = brw_debug_batch;
   brw->intel.vtbl.annotate_aub = brw_annotate_aub;
   brw->intel.vtbl.render_target_supported = brw_render_target_supported;
   brw->intel.vtbl.is_hiz_depth_format = brw_is_hiz_depth_format;

   assert(brw->intel.gen >= 4);
   if (brw->intel.gen >= 7) {
      gen7_init_vtable_surface_functions(brw);
   } else if (brw->intel.gen >= 4) {
      gen4_init_vtable_surface_functions(brw);
   }
}
