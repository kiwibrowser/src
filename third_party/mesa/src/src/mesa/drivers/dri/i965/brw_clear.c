/**************************************************************************
 *
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2009, 2012 Intel Corporation.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/condrender.h"
#include "swrast/swrast.h"
#include "drivers/common/meta.h"

#include "intel_batchbuffer.h"
#include "intel_context.h"
#include "intel_blit.h"
#include "intel_clear.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"

#define FILE_DEBUG_FLAG DEBUG_BLIT

static const char *buffer_names[] = {
   [BUFFER_FRONT_LEFT] = "front",
   [BUFFER_BACK_LEFT] = "back",
   [BUFFER_FRONT_RIGHT] = "front right",
   [BUFFER_BACK_RIGHT] = "back right",
   [BUFFER_DEPTH] = "depth",
   [BUFFER_STENCIL] = "stencil",
   [BUFFER_ACCUM] = "accum",
   [BUFFER_AUX0] = "aux0",
   [BUFFER_COLOR0] = "color0",
   [BUFFER_COLOR1] = "color1",
   [BUFFER_COLOR2] = "color2",
   [BUFFER_COLOR3] = "color3",
   [BUFFER_COLOR4] = "color4",
   [BUFFER_COLOR5] = "color5",
   [BUFFER_COLOR6] = "color6",
   [BUFFER_COLOR7] = "color7",
};

static void
debug_mask(const char *name, GLbitfield mask)
{
   GLuint i;

   if (unlikely(INTEL_DEBUG & DEBUG_BLIT)) {
      DBG("%s clear:", name);
      for (i = 0; i < BUFFER_COUNT; i++) {
	 if (mask & (1 << i))
	    DBG(" %s", buffer_names[i]);
      }
      DBG("\n");
   }
}

/**
 * Implements fast depth clears on gen6+.
 *
 * Fast clears basically work by setting a flag in each of the subspans
 * represented in the HiZ buffer that says "When you need the depth values for
 * this subspan, it's the hardware's current clear value."  Then later rendering
 * can just use the static clear value instead of referencing memory.
 *
 * The tricky part of the implementation is that you have to have the clear
 * value that was used on the depth buffer in place for all further rendering,
 * at least until a resolve to the real depth buffer happens.
 */
static bool
brw_fast_clear_depth(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct intel_renderbuffer *depth_irb =
      intel_get_renderbuffer(fb, BUFFER_DEPTH);
   struct intel_mipmap_tree *mt = depth_irb->mt;

   if (intel->gen < 6)
      return false;

   if (!mt->hiz_mt)
      return false;

   /* We only handle full buffer clears -- otherwise you'd have to track whether
    * a previous clear had happened at a different clear value and resolve it
    * first.
    */
   if (ctx->Scissor.Enabled) {
      perf_debug("Failed to fast clear depth due to scissor being enabled.  "
                 "Possible 5%% performance win if avoided.\n");
      return false;
   }

   /* The rendered area has to be 8x4 samples, not resolved pixels, so we look
    * at the miptree slice dimensions instead of renderbuffer size.
    */
   if (mt->level[depth_irb->mt_level].width % 8 != 0 ||
       mt->level[depth_irb->mt_level].height % 4 != 0) {
      perf_debug("Failed to fast clear depth due to width/height %d,%d not "
                 "being aligned to 8,4.  Possible 5%% performance win if "
                 "avoided\n",
                 mt->level[depth_irb->mt_level].width,
                 mt->level[depth_irb->mt_level].height);
      return false;
   }

   uint32_t depth_clear_value;
   switch (mt->format) {
   case MESA_FORMAT_Z32_FLOAT_X24S8:
   case MESA_FORMAT_S8_Z24:
      /* From the Sandy Bridge PRM, volume 2 part 1, page 314:
       *
       *     "[DevSNB+]: Several cases exist where Depth Buffer Clear cannot be
       *      enabled (the legacy method of clearing must be performed):
       *
       *      - If the depth buffer format is D32_FLOAT_S8X24_UINT or
       *        D24_UNORM_S8_UINT.
       */
      return false;

   case MESA_FORMAT_Z32_FLOAT:
      depth_clear_value = float_as_int(ctx->Depth.Clear);
      break;

   case MESA_FORMAT_Z16:
      /* From the Sandy Bridge PRM, volume 2 part 1, page 314:
       *
       *     "[DevSNB+]: Several cases exist where Depth Buffer Clear cannot be
       *      enabled (the legacy method of clearing must be performed):
       *
       *      - DevSNB{W/A}]: When depth buffer format is D16_UNORM and the
       *        width of the map (LOD0) is not multiple of 16, fast clear
       *        optimization must be disabled.
       */
      if (intel->gen == 6 && (mt->level[depth_irb->mt_level].width % 16) != 0)
	 return false;
      /* FALLTHROUGH */

   default:
      depth_clear_value = fb->_DepthMax * ctx->Depth.Clear;
      break;
   }

   /* If we're clearing to a new clear value, then we need to resolve any clear
    * flags out of the HiZ buffer into the real depth buffer.
    */
   if (mt->depth_clear_value != depth_clear_value) {
      intel_miptree_all_slices_resolve_depth(intel, mt);
      mt->depth_clear_value = depth_clear_value;
   }

   /* From the Sandy Bridge PRM, volume 2 part 1, page 313:
    *
    *     "If other rendering operations have preceded this clear, a
    *      PIPE_CONTROL with write cache flush enabled and Z-inhibit disabled
    *      must be issued before the rectangle primitive used for the depth
    *      buffer clear operation.
    */
   intel_batchbuffer_emit_mi_flush(intel);

   intel_hiz_exec(intel, mt, depth_irb->mt_level, depth_irb->mt_layer,
		  GEN6_HIZ_OP_DEPTH_CLEAR);

   if (intel->gen == 6) {
      /* From the Sandy Bridge PRM, volume 2 part 1, page 314:
       *
       *     "DevSNB, DevSNB-B{W/A}]: Depth buffer clear pass must be followed
       *      by a PIPE_CONTROL command with DEPTH_STALL bit set and Then
       *      followed by Depth FLUSH'
      */
      intel_batchbuffer_emit_mi_flush(intel);
   }

   /* Now, the HiZ buffer contains data that needs to be resolved to the depth
    * buffer.
    */
   intel_renderbuffer_set_needs_depth_resolve(depth_irb);

   return true;
}

/**
 * Called by ctx->Driver.Clear.
 */
static void
brw_clear(struct gl_context *ctx, GLbitfield mask)
{
   struct intel_context *intel = intel_context(ctx);

   if (!_mesa_check_conditional_render(ctx))
      return;

   if (mask & (BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_FRONT_RIGHT)) {
      intel->front_buffer_dirty = true;
   }

   intel_prepare_render(intel);

   if (mask & BUFFER_BIT_DEPTH) {
      if (brw_fast_clear_depth(ctx)) {
	 DBG("fast clear: depth\n");
	 mask &= ~BUFFER_BIT_DEPTH;
      }
   }

   GLbitfield tri_mask = mask & (BUFFER_BITS_COLOR |
				 BUFFER_BIT_STENCIL |
				 BUFFER_BIT_DEPTH);

   if (tri_mask) {
      debug_mask("tri", tri_mask);
      mask &= ~tri_mask;

      if (ctx->API == API_OPENGLES) {
         _mesa_meta_Clear(&intel->ctx, tri_mask);
      } else {
         _mesa_meta_glsl_Clear(&intel->ctx, tri_mask);
      }
   }

   /* Any strange buffers get passed off to swrast */
   if (mask) {
      debug_mask("swrast", mask);
      _swrast_Clear(ctx, mask);
   }
}


void
intelInitClearFuncs(struct dd_function_table *functions)
{
   functions->Clear = brw_clear;
}
