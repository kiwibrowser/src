/**************************************************************************
 *
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2009 Intel Corporation.
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

#include "intel_context.h"
#include "intel_blit.h"
#include "intel_clear.h"
#include "intel_fbo.h"
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
 * Called by ctx->Driver.Clear.
 */
static void
intelClear(struct gl_context *ctx, GLbitfield mask)
{
   struct intel_context *intel = intel_context(ctx);
   const GLuint colorMask = *((GLuint *) & ctx->Color.ColorMask[0]);
   GLbitfield tri_mask = 0;
   GLbitfield blit_mask = 0;
   GLbitfield swrast_mask = 0;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct intel_renderbuffer *irb;
   int i;

   if (mask & (BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_FRONT_RIGHT)) {
      intel->front_buffer_dirty = true;
   }

   if (0)
      fprintf(stderr, "%s\n", __FUNCTION__);

   /* Get SW clears out of the way: Anything without an intel_renderbuffer */
   for (i = 0; i < BUFFER_COUNT; i++) {
      if (!(mask & (1 << i)))
	 continue;

      irb = intel_get_renderbuffer(fb, i);
      if (unlikely(!irb)) {
	 swrast_mask |= (1 << i);
	 mask &= ~(1 << i);
      }
   }
   if (unlikely(swrast_mask)) {
      debug_mask("swrast", swrast_mask);
      _swrast_Clear(ctx, swrast_mask);
   }

   /* HW color buffers (front, back, aux, generic FBO, etc) */
   if (colorMask == ~0) {
      /* clear all R,G,B,A */
      blit_mask |= (mask & BUFFER_BITS_COLOR);
   }
   else {
      /* glColorMask in effect */
      tri_mask |= (mask & BUFFER_BITS_COLOR);
   }

   /* Make sure we have up to date buffers before we start looking at
    * the tiling bits to determine how to clear. */
   intel_prepare_render(intel);

   /* HW stencil */
   if (mask & BUFFER_BIT_STENCIL) {
      const struct intel_region *stencilRegion
         = intel_get_rb_region(fb, BUFFER_STENCIL);
      if (stencilRegion) {
         /* have hw stencil */
         if (stencilRegion->tiling == I915_TILING_Y ||
	     (ctx->Stencil.WriteMask[0] & 0xff) != 0xff) {
	    /* We have to use the 3D engine if we're clearing a partial mask
	     * of the stencil buffer, or if we're on a 965 which has a tiled
	     * depth/stencil buffer in a layout we can't blit to.
	     */
            tri_mask |= BUFFER_BIT_STENCIL;
         }
         else {
            /* clearing all stencil bits, use blitting */
            blit_mask |= BUFFER_BIT_STENCIL;
         }
      }
   }

   /* HW depth */
   if (mask & BUFFER_BIT_DEPTH) {
      const struct intel_region *irb = intel_get_rb_region(fb, BUFFER_DEPTH);

      /* clear depth with whatever method is used for stencil (see above) */
      if (irb->tiling == I915_TILING_Y || tri_mask & BUFFER_BIT_STENCIL)
         tri_mask |= BUFFER_BIT_DEPTH;
      else
         blit_mask |= BUFFER_BIT_DEPTH;
   }

   /* If we're doing a tri pass for depth/stencil, include a likely color
    * buffer with it.
    */
   if (mask & (BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL)) {
      int color_bit = ffs(mask & BUFFER_BITS_COLOR);
      if (color_bit != 0) {
	 tri_mask |= blit_mask & (1 << (color_bit - 1));
	 blit_mask &= ~(1 << (color_bit - 1));
      }
   }

   /* Anything left, just use tris */
   tri_mask |= mask & ~blit_mask;

   if (blit_mask) {
      debug_mask("blit", blit_mask);
      tri_mask |= intelClearWithBlit(ctx, blit_mask);
   }

   if (tri_mask) {
      debug_mask("tri", tri_mask);
      if (ctx->API == API_OPENGLES)
	 _mesa_meta_Clear(&intel->ctx, tri_mask);
      else
	 _mesa_meta_glsl_Clear(&intel->ctx, tri_mask);
   }
}


void
intelInitClearFuncs(struct dd_function_table *functions)
{
   functions->Clear = intelClear;
}
