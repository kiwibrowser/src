
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
#include "main/pbo.h"
#include "main/texobj.h"
#include "main/texstore.h"
#include "main/texcompress.h"
#include "main/enums.h"

#include "intel_context.h"
#include "intel_tex.h"
#include "intel_mipmap_tree.h"
#include "intel_blit.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

static bool
intel_blit_texsubimage(struct gl_context * ctx,
		       struct gl_texture_image *texImage,
		       GLint xoffset, GLint yoffset,
		       GLint width, GLint height,
		       GLenum format, GLenum type, const void *pixels,
		       const struct gl_pixelstore_attrib *packing)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intelImage = intel_texture_image(texImage);
   GLuint dstRowStride = 0;
   drm_intel_bo *temp_bo = NULL;
   unsigned int blit_x = 0, blit_y = 0;
   unsigned long pitch;
   uint32_t tiling_mode = I915_TILING_NONE;
   GLubyte *dstMap;

   /* Try to do a blit upload of the subimage if the texture is
    * currently busy.
    */
   if (!intelImage->mt)
      return false;

   /* The blitter can't handle Y tiling */
   if (intelImage->mt->region->tiling == I915_TILING_Y)
      return false;

   if (texImage->TexObject->Target != GL_TEXTURE_2D)
      return false;

   /* On gen6, it's probably not worth swapping to the blit ring to do
    * this because of all the overhead involved.
    */
   if (intel->gen >= 6)
      return false;

   if (!drm_intel_bo_busy(intelImage->mt->region->bo))
      return false;

   DBG("BLT subimage %s target %s level %d offset %d,%d %dx%d\n",
       __FUNCTION__,
       _mesa_lookup_enum_by_nr(texImage->TexObject->Target),
       texImage->Level, xoffset, yoffset, width, height);

   pixels = _mesa_validate_pbo_teximage(ctx, 2, width, height, 1,
					format, type, pixels, packing,
					"glTexSubImage");
   if (!pixels)
      return false;

   temp_bo = drm_intel_bo_alloc_tiled(intel->bufmgr,
				      "subimage blit bo",
				      width, height,
				      intelImage->mt->cpp,
				      &tiling_mode,
				      &pitch,
				      0);
   if (temp_bo == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "intelTexSubImage");
      return false;
   }

   if (drm_intel_gem_bo_map_gtt(temp_bo)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "intelTexSubImage");
      return false;
   }

   dstMap = temp_bo->virtual;
   dstRowStride = pitch;

   intel_miptree_get_image_offset(intelImage->mt, texImage->Level,
				  intelImage->base.Base.Face, 0,
				  &blit_x, &blit_y);
   blit_x += xoffset;
   blit_y += yoffset;

   if (!_mesa_texstore(ctx, 2, texImage->_BaseFormat,
		       texImage->TexFormat,
		       dstRowStride,
		       &dstMap,
		       width, height, 1,
		       format, type, pixels, packing)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "intelTexSubImage");
   }

   bool ret;
   unsigned int dst_pitch = intelImage->mt->region->pitch *
      intelImage->mt->cpp;

   drm_intel_gem_bo_unmap_gtt(temp_bo);

   ret = intelEmitCopyBlit(intel,
			   intelImage->mt->cpp,
			   dstRowStride / intelImage->mt->cpp,
			   temp_bo, 0, false,
			   dst_pitch / intelImage->mt->cpp,
			   intelImage->mt->region->bo, 0,
			   intelImage->mt->region->tiling,
			   0, 0, blit_x, blit_y, width, height,
			   GL_COPY);
   assert(ret);

   drm_intel_bo_unreference(temp_bo);
   _mesa_unmap_teximage_pbo(ctx, packing);

   return true;
}

static void
intelTexSubImage(struct gl_context * ctx,
                 GLuint dims,
                 struct gl_texture_image *texImage,
                 GLint xoffset, GLint yoffset, GLint zoffset,
                 GLsizei width, GLsizei height, GLsizei depth,
                 GLenum format, GLenum type,
                 const GLvoid * pixels,
                 const struct gl_pixelstore_attrib *packing)
{
   /* The intel_blit_texsubimage() function only handles 2D images */
   if (dims != 2 || !intel_blit_texsubimage(ctx, texImage,
			       xoffset, yoffset,
			       width, height,
			       format, type, pixels, packing)) {
      _mesa_store_texsubimage(ctx, dims, texImage,
                              xoffset, yoffset, zoffset,
                              width, height, depth,
                              format, type, pixels, packing);
   }
}

void
intelInitTextureSubImageFuncs(struct dd_function_table *functions)
{
   functions->TexSubImage = intelTexSubImage;
}
