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

#include "main/mtypes.h"
#include "main/enums.h"
#include "main/image.h"
#include "main/teximage.h"
#include "main/texstate.h"
#include "main/fbobject.h"

#include "drivers/common/meta.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_fbo.h"
#include "intel_tex.h"
#include "intel_blit.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE


bool
intel_copy_texsubimage(struct intel_context *intel,
                       struct intel_texture_image *intelImage,
                       GLint dstx, GLint dsty,
                       struct intel_renderbuffer *irb,
                       GLint x, GLint y, GLsizei width, GLsizei height)
{
   struct gl_context *ctx = &intel->ctx;
   struct intel_region *region;
   const GLenum internalFormat = intelImage->base.Base.InternalFormat;
   bool copy_supported = false;
   bool copy_supported_with_alpha_override = false;

   intel_prepare_render(intel);

   if (!intelImage->mt || !irb || !irb->mt) {
      if (unlikely(INTEL_DEBUG & DEBUG_PERF))
	 fprintf(stderr, "%s fail %p %p (0x%08x)\n",
		 __FUNCTION__, intelImage->mt, irb, internalFormat);
      return false;
   } else {
      region = irb->mt->region;
      assert(region);
   }

   copy_supported = intelImage->base.Base.TexFormat == intel_rb_format(irb);

   /* Converting ARGB8888 to XRGB8888 is trivial: ignore the alpha bits */
   if (intel_rb_format(irb) == MESA_FORMAT_ARGB8888 &&
       intelImage->base.Base.TexFormat == MESA_FORMAT_XRGB8888) {
      copy_supported = true;
   }

   /* Converting XRGB8888 to ARGB8888 requires setting the alpha bits to 1.0 */
   if (intel_rb_format(irb) == MESA_FORMAT_XRGB8888 &&
       intelImage->base.Base.TexFormat == MESA_FORMAT_ARGB8888) {
      copy_supported_with_alpha_override = true;
   }

   if (!copy_supported && !copy_supported_with_alpha_override) {
      if (unlikely(INTEL_DEBUG & DEBUG_PERF))
	 fprintf(stderr, "%s mismatched formats %s, %s\n",
		 __FUNCTION__,
		 _mesa_get_format_name(intelImage->base.Base.TexFormat),
		 _mesa_get_format_name(intel_rb_format(irb)));
      return false;
   }

   {
      GLuint image_x, image_y;
      GLshort src_pitch;

      /* get dest x/y in destination texture */
      intel_miptree_get_image_offset(intelImage->mt,
				     intelImage->base.Base.Level,
				     intelImage->base.Base.Face,
				     0,
				     &image_x, &image_y);

      /* The blitter can't handle Y-tiled buffers. */
      if (intelImage->mt->region->tiling == I915_TILING_Y) {
	 return false;
      }

      if (_mesa_is_winsys_fbo(ctx->ReadBuffer)) {
	 /* Flip vertical orientation for system framebuffers */
	 y = ctx->ReadBuffer->Height - (y + height);
	 src_pitch = -region->pitch;
      } else {
	 /* reading from a FBO, y is already oriented the way we like */
	 src_pitch = region->pitch;
      }

      /* blit from src buffer to texture */
      if (!intelEmitCopyBlit(intel,
			     intelImage->mt->cpp,
			     src_pitch,
			     region->bo,
			     0,
			     region->tiling,
			     intelImage->mt->region->pitch,
			     intelImage->mt->region->bo,
			     0,
			     intelImage->mt->region->tiling,
			     irb->draw_x + x, irb->draw_y + y,
			     image_x + dstx, image_y + dsty,
			     width, height,
			     GL_COPY)) {
	 return false;
      }
   }

   if (copy_supported_with_alpha_override)
      intel_set_teximage_alpha_to_one(ctx, intelImage);

   return true;
}


static void
intelCopyTexSubImage(struct gl_context *ctx, GLuint dims,
                     struct gl_texture_image *texImage,
                     GLint xoffset, GLint yoffset, GLint zoffset,
                     struct gl_renderbuffer *rb,
                     GLint x, GLint y,
                     GLsizei width, GLsizei height)
{
   if (dims == 3 || !intel_copy_texsubimage(intel_context(ctx),
                               intel_texture_image(texImage),
                               xoffset, yoffset,
                               intel_renderbuffer(rb), x, y, width, height)) {
      fallback_debug("%s - fallback to swrast\n", __FUNCTION__);
      _mesa_meta_CopyTexSubImage(ctx, dims, texImage,
                                 xoffset, yoffset, zoffset,
                                 rb, x, y, width, height);
   }
}


void
intelInitTextureCopyImageFuncs(struct dd_function_table *functions)
{
   functions->CopyTexSubImage = intelCopyTexSubImage;
}
