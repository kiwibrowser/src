/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "intel_context.h"
#include "intel_buffers.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"

#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"

/**
 * Return pointer to current color drawing region, or NULL.
 */
struct intel_region *
intel_drawbuf_region(struct intel_context *intel)
{
   struct intel_renderbuffer *irbColor =
      intel_renderbuffer(intel->ctx.DrawBuffer->_ColorDrawBuffers[0]);
   if (irbColor && irbColor->mt)
      return irbColor->mt->region;
   else
      return NULL;
}

/**
 * Return pointer to current color reading region, or NULL.
 */
struct intel_region *
intel_readbuf_region(struct intel_context *intel)
{
   struct intel_renderbuffer *irb
      = intel_renderbuffer(intel->ctx.ReadBuffer->_ColorReadBuffer);
   if (irb && irb->mt)
      return irb->mt->region;
   else
      return NULL;
}

/**
 * Check if we're about to draw into the front color buffer.
 * If so, set the intel->front_buffer_dirty field to true.
 */
void
intel_check_front_buffer_rendering(struct intel_context *intel)
{
   const struct gl_framebuffer *fb = intel->ctx.DrawBuffer;
   if (_mesa_is_winsys_fbo(fb)) {
      /* drawing to window system buffer */
      if (fb->_NumColorDrawBuffers > 0) {
         if (fb->_ColorDrawBufferIndexes[0] == BUFFER_FRONT_LEFT) {
	    intel->front_buffer_dirty = true;
	 }
      }
   }
}

static void
intelDrawBuffer(struct gl_context * ctx, GLenum mode)
{
   if (ctx->DrawBuffer && _mesa_is_winsys_fbo(ctx->DrawBuffer)) {
      struct intel_context *const intel = intel_context(ctx);
      const bool was_front_buffer_rendering =
	intel->is_front_buffer_rendering;

      intel->is_front_buffer_rendering = (mode == GL_FRONT_LEFT)
	|| (mode == GL_FRONT) || (mode == GL_FRONT_AND_BACK);

      /* If we weren't front-buffer rendering before but we are now,
       * invalidate our DRI drawable so we'll ask for new buffers
       * (including the fake front) before we start rendering again.
       */
      if (!was_front_buffer_rendering && intel->is_front_buffer_rendering)
	 dri2InvalidateDrawable(intel->driContext->driDrawablePriv);
   }

   intel_draw_buffer(ctx);
}


static void
intelReadBuffer(struct gl_context * ctx, GLenum mode)
{
   if (ctx->DrawBuffer && _mesa_is_winsys_fbo(ctx->DrawBuffer)) {
      struct intel_context *const intel = intel_context(ctx);
      const bool was_front_buffer_reading =
	intel->is_front_buffer_reading;

      intel->is_front_buffer_reading = (mode == GL_FRONT_LEFT)
	|| (mode == GL_FRONT);

      /* If we weren't front-buffer reading before but we are now,
       * invalidate our DRI drawable so we'll ask for new buffers
       * (including the fake front) before we start reading again.
       */
      if (!was_front_buffer_reading && intel->is_front_buffer_reading)
	 dri2InvalidateDrawable(intel->driContext->driReadablePriv);
   }
}


void
intelInitBufferFuncs(struct dd_function_table *functions)
{
   functions->DrawBuffer = intelDrawBuffer;
   functions->ReadBuffer = intelReadBuffer;
}
