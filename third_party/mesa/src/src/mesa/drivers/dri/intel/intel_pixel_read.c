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

#include "main/glheader.h"
#include "main/enums.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "main/fbobject.h"
#include "main/image.h"
#include "main/bufferobj.h"
#include "main/readpix.h"
#include "main/state.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_blit.h"
#include "intel_buffers.h"
#include "intel_regions.h"
#include "intel_pixel.h"
#include "intel_buffer_objects.h"

#define FILE_DEBUG_FLAG DEBUG_PIXEL

/* For many applications, the new ability to pull the source buffers
 * back out of the GTT and then do the packing/conversion operations
 * in software will be as much of an improvement as trying to get the
 * blitter and/or texture engine to do the work.
 *
 * This step is gated on private backbuffers.
 *
 * Obviously the frontbuffer can't be pulled back, so that is either
 * an argument for blit/texture readpixels, or for blitting to a
 * temporary and then pulling that back.
 *
 * When the destination is a pbo, however, it's not clear if it is
 * ever going to be pulled to main memory (though the access param
 * will be a good hint).  So it sounds like we do want to be able to
 * choose between blit/texture implementation on the gpu and pullback
 * and cpu-based copying.
 *
 * Unless you can magically turn client memory into a PBO for the
 * duration of this call, there will be a cpu-based copying step in
 * any case.
 */

static bool
do_blit_readpixels(struct gl_context * ctx,
                   GLint x, GLint y, GLsizei width, GLsizei height,
                   GLenum format, GLenum type,
                   const struct gl_pixelstore_attrib *pack, GLvoid * pixels)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_region *src = intel_readbuf_region(intel);
   struct intel_buffer_object *dst = intel_buffer_object(pack->BufferObj);
   GLuint dst_offset;
   GLuint rowLength;
   drm_intel_bo *dst_buffer;
   bool all;
   GLint dst_x, dst_y;
   GLuint dirty;

   DBG("%s\n", __FUNCTION__);

   if (!src)
      return false;

   if (!_mesa_is_bufferobj(pack->BufferObj)) {
      /* PBO only for now:
       */
      DBG("%s - not PBO\n", __FUNCTION__);
      return false;
   }


   if (ctx->_ImageTransferState ||
       !intel_check_blit_format(src, format, type)) {
      DBG("%s - bad format for blit\n", __FUNCTION__);
      return false;
   }

   if (pack->Alignment != 1 || pack->SwapBytes || pack->LsbFirst) {
      DBG("%s: bad packing params\n", __FUNCTION__);
      return false;
   }

   if (pack->RowLength > 0)
      rowLength = pack->RowLength;
   else
      rowLength = width;

   if (pack->Invert) {
      DBG("%s: MESA_PACK_INVERT not done yet\n", __FUNCTION__);
      return false;
   }
   else {
      if (_mesa_is_winsys_fbo(ctx->ReadBuffer))
	 rowLength = -rowLength;
   }

   dst_offset = (GLintptr)pixels;
   dst_offset += _mesa_image_offset(2, pack, width, height,
				    format, type, 0, 0, 0);

   if (!_mesa_clip_copytexsubimage(ctx,
				   &dst_x, &dst_y,
				   &x, &y,
				   &width, &height)) {
      return true;
   }

   dirty = intel->front_buffer_dirty;
   intel_prepare_render(intel);
   intel->front_buffer_dirty = dirty;

   all = (width * height * src->cpp == dst->Base.Size &&
	  x == 0 && dst_offset == 0);

   dst_x = 0;
   dst_y = 0;

   dst_buffer = intel_bufferobj_buffer(intel, dst,
				       all ? INTEL_WRITE_FULL :
				       INTEL_WRITE_PART);

   if (_mesa_is_winsys_fbo(ctx->ReadBuffer))
      y = ctx->ReadBuffer->Height - (y + height);

   if (!intelEmitCopyBlit(intel,
			  src->cpp,
			  src->pitch, src->bo, 0, src->tiling,
			  rowLength, dst_buffer, dst_offset, false,
			  x, y,
			  dst_x, dst_y,
			  width, height,
			  GL_COPY)) {
      return false;
   }

   DBG("%s - DONE\n", __FUNCTION__);

   return true;
}

void
intelReadPixels(struct gl_context * ctx,
                GLint x, GLint y, GLsizei width, GLsizei height,
                GLenum format, GLenum type,
                const struct gl_pixelstore_attrib *pack, GLvoid * pixels)
{
   struct intel_context *intel = intel_context(ctx);
   bool dirty;

   intel_flush_rendering_to_batch(ctx);

   DBG("%s\n", __FUNCTION__);

   if (do_blit_readpixels
       (ctx, x, y, width, height, format, type, pack, pixels))
      return;

   /* glReadPixels() wont dirty the front buffer, so reset the dirty
    * flag after calling intel_prepare_render(). */
   dirty = intel->front_buffer_dirty;
   intel_prepare_render(intel);
   intel->front_buffer_dirty = dirty;

   fallback_debug("%s: fallback to swrast\n", __FUNCTION__);

   /* Update Mesa state before calling _mesa_readpixels().
    * XXX this may not be needed since ReadPixels no longer uses the
    * span code.
    */

   if (ctx->NewState)
      _mesa_update_state(ctx);

   _mesa_readpixels(ctx, x, y, width, height, format, type, pack, pixels);

   /* There's an intel_prepare_render() call in intelSpanRenderStart(). */
   intel->front_buffer_dirty = dirty;
}
