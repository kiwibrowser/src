/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * next paragraph) shall be included in all copies or substantial portionsalloc
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
#include "main/image.h"
#include "main/colormac.h"
#include "main/condrender.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "main/pbo.h"
#include "main/bufferobj.h"
#include "main/state.h"
#include "main/texobj.h"
#include "main/context.h"
#include "main/fbobject.h"
#include "swrast/swrast.h"
#include "drivers/common/meta.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_blit.h"
#include "intel_regions.h"
#include "intel_buffers.h"
#include "intel_pixel.h"
#include "intel_reg.h"


#define FILE_DEBUG_FLAG DEBUG_PIXEL


/* Unlike the other intel_pixel_* functions, the expectation here is
 * that the incoming data is not in a PBO.  With the XY_TEXT blit
 * method, there's no benefit haveing it in a PBO, but we could
 * implement a path based on XY_MONO_SRC_COPY_BLIT which might benefit
 * PBO bitmaps.  I think they are probably pretty rare though - I
 * wonder if Xgl uses them?
 */
static const GLubyte *map_pbo( struct gl_context *ctx,
			       GLsizei width, GLsizei height,
			       const struct gl_pixelstore_attrib *unpack,
			       const GLubyte *bitmap )
{
   GLubyte *buf;

   if (!_mesa_validate_pbo_access(2, unpack, width, height, 1,
				  GL_COLOR_INDEX, GL_BITMAP,
				  INT_MAX, (const GLvoid *) bitmap)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,"glBitmap(invalid PBO access)");
      return NULL;
   }

   buf = (GLubyte *) ctx->Driver.MapBufferRange(ctx, 0, unpack->BufferObj->Size,
						GL_MAP_READ_BIT,
						unpack->BufferObj);
   if (!buf) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBitmap(PBO is mapped)");
      return NULL;
   }

   return ADD_POINTERS(buf, bitmap);
}

static bool test_bit( const GLubyte *src, GLuint bit )
{
   return (src[bit/8] & (1<<(bit % 8))) ? 1 : 0;
}

static void set_bit( GLubyte *dest, GLuint bit )
{
   dest[bit/8] |= 1 << (bit % 8);
}

/* Extract a rectangle's worth of data from the bitmap.  Called
 * per chunk of HW-sized bitmap.
 */
static GLuint get_bitmap_rect(GLsizei width, GLsizei height,
			      const struct gl_pixelstore_attrib *unpack,
			      const GLubyte *bitmap,
			      GLuint x, GLuint y, 
			      GLuint w, GLuint h,
			      GLubyte *dest,
			      GLuint row_align,
			      bool invert)
{
   GLuint src_offset = (x + unpack->SkipPixels) & 0x7;
   GLuint mask = unpack->LsbFirst ? 0 : 7;
   GLuint bit = 0;
   GLint row, col;
   GLint first, last;
   GLint incr;
   GLuint count = 0;

   DBG("%s %d,%d %dx%d bitmap %dx%d skip %d src_offset %d mask %d\n",
       __FUNCTION__, x,y,w,h,width,height,unpack->SkipPixels, src_offset, mask);

   if (invert) {
      first = h-1;
      last = 0;
      incr = -1;
   }
   else {
      first = 0;
      last = h-1;
      incr = 1;
   }

   /* Require that dest be pre-zero'd.
    */
   for (row = first; row != (last+incr); row += incr) {
      const GLubyte *rowsrc = _mesa_image_address2d(unpack, bitmap, 
						    width, height, 
						    GL_COLOR_INDEX, GL_BITMAP, 
						    y + row, x);

      for (col = 0; col < w; col++, bit++) {
	 if (test_bit(rowsrc, (col + src_offset) ^ mask)) {
	    set_bit(dest, bit ^ 7);
	    count++;
	 }
      }

      if (row_align)
	 bit = ALIGN(bit, row_align);
   }

   return count;
}

/**
 * Returns the low Y value of the vertical range given, flipped according to
 * whether the framebuffer is or not.
 */
static INLINE int
y_flip(struct gl_framebuffer *fb, int y, int height)
{
   if (_mesa_is_user_fbo(fb))
      return y;
   else
      return fb->Height - y - height;
}

/*
 * Render a bitmap.
 */
static bool
do_blit_bitmap( struct gl_context *ctx, 
		GLint dstx, GLint dsty,
		GLsizei width, GLsizei height,
		const struct gl_pixelstore_attrib *unpack,
		const GLubyte *bitmap )
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_region *dst;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLfloat tmpColor[4];
   GLubyte ubcolor[4];
   GLuint color;
   GLsizei bitmap_width = width;
   GLsizei bitmap_height = height;
   GLint px, py;
   GLuint stipple[32];
   GLint orig_dstx = dstx;
   GLint orig_dsty = dsty;

   /* Update draw buffer bounds */
   _mesa_update_state(ctx);

   if (ctx->Depth.Test) {
      /* The blit path produces incorrect results when depth testing is on.
       * It seems the blit Z coord is always 1.0 (the far plane) so fragments
       * will likely be obscured by other, closer geometry.
       */
      return false;
   }

   intel_prepare_render(intel);
   dst = intel_drawbuf_region(intel);

   if (!dst)
       return false;

   if (_mesa_is_bufferobj(unpack->BufferObj)) {
      bitmap = map_pbo(ctx, width, height, unpack, bitmap);
      if (bitmap == NULL)
	 return true;	/* even though this is an error, we're done */
   }

   COPY_4V(tmpColor, ctx->Current.RasterColor);

   if (_mesa_need_secondary_color(ctx)) {
       ADD_3V(tmpColor, tmpColor, ctx->Current.RasterSecondaryColor);
   }

   UNCLAMPED_FLOAT_TO_UBYTE(ubcolor[0], tmpColor[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(ubcolor[1], tmpColor[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(ubcolor[2], tmpColor[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(ubcolor[3], tmpColor[3]);

   if (dst->cpp == 2)
      color = PACK_COLOR_565(ubcolor[0], ubcolor[1], ubcolor[2]);
   else
      color = PACK_COLOR_8888(ubcolor[3], ubcolor[0], ubcolor[1], ubcolor[2]);

   if (!intel_check_blit_fragment_ops(ctx, tmpColor[3] == 1.0F))
      return false;

   /* Clip to buffer bounds and scissor. */
   if (!_mesa_clip_to_region(fb->_Xmin, fb->_Ymin,
			     fb->_Xmax, fb->_Ymax,
			     &dstx, &dsty, &width, &height))
      goto out;

   dsty = y_flip(fb, dsty, height);

#define DY 32
#define DX 32

   /* Chop it all into chunks that can be digested by hardware: */
   for (py = 0; py < height; py += DY) {
      for (px = 0; px < width; px += DX) {
	 int h = MIN2(DY, height - py);
	 int w = MIN2(DX, width - px);
	 GLuint sz = ALIGN(ALIGN(w,8) * h, 64)/8;
	 GLenum logic_op = ctx->Color.ColorLogicOpEnabled ?
	    ctx->Color.LogicOp : GL_COPY;

	 assert(sz <= sizeof(stipple));
	 memset(stipple, 0, sz);

	 /* May need to adjust this when padding has been introduced in
	  * sz above:
	  *
	  * Have to translate destination coordinates back into source
	  * coordinates.
	  */
	 if (get_bitmap_rect(bitmap_width, bitmap_height, unpack,
			     bitmap,
			     -orig_dstx + (dstx + px),
			     -orig_dsty + y_flip(fb, dsty + py, h),
			     w, h,
			     (GLubyte *)stipple,
			     8,
			     _mesa_is_winsys_fbo(fb)) == 0)
	    continue;

	 if (!intelEmitImmediateColorExpandBlit(intel,
						dst->cpp,
						(GLubyte *)stipple,
						sz,
						color,
						dst->pitch,
						dst->bo,
						0,
						dst->tiling,
						dstx + px,
						dsty + py,
						w, h,
						logic_op)) {
	    return false;
	 }
      }
   }
out:

   if (unlikely(INTEL_DEBUG & DEBUG_SYNC))
      intel_batchbuffer_flush(intel);

   if (_mesa_is_bufferobj(unpack->BufferObj)) {
      /* done with PBO so unmap it now */
      ctx->Driver.UnmapBuffer(ctx, unpack->BufferObj);
   }

   intel_check_front_buffer_rendering(intel);

   return true;
}


/* There are a large number of possible ways to implement bitmap on
 * this hardware, most of them have some sort of drawback.  Here are a
 * few that spring to mind:
 * 
 * Blit:
 *    - XY_MONO_SRC_BLT_CMD
 *         - use XY_SETUP_CLIP_BLT for cliprect clipping.
 *    - XY_TEXT_BLT
 *    - XY_TEXT_IMMEDIATE_BLT
 *         - blit per cliprect, subject to maximum immediate data size.
 *    - XY_COLOR_BLT 
 *         - per pixel or run of pixels
 *    - XY_PIXEL_BLT
 *         - good for sparse bitmaps
 *
 * 3D engine:
 *    - Point per pixel
 *    - Translate bitmap to an alpha texture and render as a quad
 *    - Chop bitmap up into 32x32 squares and render w/polygon stipple.
 */
void
intelBitmap(struct gl_context * ctx,
	    GLint x, GLint y,
	    GLsizei width, GLsizei height,
	    const struct gl_pixelstore_attrib *unpack,
	    const GLubyte * pixels)
{
   if (!_mesa_check_conditional_render(ctx))
      return;

   if (do_blit_bitmap(ctx, x, y, width, height,
                          unpack, pixels))
      return;

   _mesa_meta_Bitmap(ctx, x, y, width, height, unpack, pixels);
}
