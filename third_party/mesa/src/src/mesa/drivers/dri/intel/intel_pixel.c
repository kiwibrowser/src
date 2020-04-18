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

#include "main/accum.h"
#include "main/enums.h"
#include "main/state.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "swrast/swrast.h"

#include "intel_context.h"
#include "intel_pixel.h"
#include "intel_regions.h"

#define FILE_DEBUG_FLAG DEBUG_PIXEL

static GLenum
effective_func(GLenum func, bool src_alpha_is_one)
{
   if (src_alpha_is_one) {
      if (func == GL_SRC_ALPHA)
	 return GL_ONE;
      if (func == GL_ONE_MINUS_SRC_ALPHA)
	 return GL_ZERO;
   }

   return func;
}

/**
 * Check if any fragment operations are in effect which might effect
 * glDraw/CopyPixels.
 */
bool
intel_check_blit_fragment_ops(struct gl_context * ctx, bool src_alpha_is_one)
{
   if (ctx->NewState)
      _mesa_update_state(ctx);

   if (ctx->FragmentProgram._Enabled) {
      DBG("fallback due to fragment program\n");
      return false;
   }

   if (ctx->Color.BlendEnabled &&
       (effective_func(ctx->Color.Blend[0].SrcRGB, src_alpha_is_one) != GL_ONE ||
	effective_func(ctx->Color.Blend[0].DstRGB, src_alpha_is_one) != GL_ZERO ||
	ctx->Color.Blend[0].EquationRGB != GL_FUNC_ADD ||
	effective_func(ctx->Color.Blend[0].SrcA, src_alpha_is_one) != GL_ONE ||
	effective_func(ctx->Color.Blend[0].DstA, src_alpha_is_one) != GL_ZERO ||
	ctx->Color.Blend[0].EquationA != GL_FUNC_ADD)) {
      DBG("fallback due to blend\n");
      return false;
   }

   if (ctx->Texture._EnabledUnits) {
      DBG("fallback due to texturing\n");
      return false;
   }

   if (!(ctx->Color.ColorMask[0][0] &&
	 ctx->Color.ColorMask[0][1] &&
	 ctx->Color.ColorMask[0][2] &&
	 ctx->Color.ColorMask[0][3])) {
      DBG("fallback due to color masking\n");
      return false;
   }

   if (ctx->Color.AlphaEnabled) {
      DBG("fallback due to alpha\n");
      return false;
   }

   if (ctx->Depth.Test) {
      DBG("fallback due to depth test\n");
      return false;
   }

   if (ctx->Fog.Enabled) {
      DBG("fallback due to fog\n");
      return false;
   }

   if (ctx->_ImageTransferState) {
      DBG("fallback due to image transfer\n");
      return false;
   }

   if (ctx->Stencil._Enabled) {
      DBG("fallback due to image stencil\n");
      return false;
   }

   if (ctx->RenderMode != GL_RENDER) {
      DBG("fallback due to render mode\n");
      return false;
   }

   return true;
}

/* The intel_region struct doesn't really do enough to capture the
 * format of the pixels in the region.  For now this code assumes that
 * the region is a display surface and hence is either ARGB8888 or
 * RGB565.
 * XXX FBO: If we'd pass in the intel_renderbuffer instead of region, we'd
 * know the buffer's pixel format.
 *
 * \param format  as given to glDraw/ReadPixels
 * \param type  as given to glDraw/ReadPixels
 */
bool
intel_check_blit_format(struct intel_region * region,
                        GLenum format, GLenum type)
{
   if (region->cpp == 4 &&
       (type == GL_UNSIGNED_INT_8_8_8_8_REV ||
        type == GL_UNSIGNED_BYTE) && format == GL_BGRA) {
      return true;
   }

   if (region->cpp == 2 &&
       type == GL_UNSIGNED_SHORT_5_6_5_REV && format == GL_BGR) {
      return true;
   }

   DBG("%s: bad format for blit (cpp %d, type %s format %s)\n",
       __FUNCTION__, region->cpp,
       _mesa_lookup_enum_by_nr(type), _mesa_lookup_enum_by_nr(format));

   return false;
}

void
intelInitPixelFuncs(struct dd_function_table *functions)
{
   functions->Accum = _mesa_accum;
   functions->Bitmap = intelBitmap;
   functions->CopyPixels = intelCopyPixels;
   functions->DrawPixels = intelDrawPixels;
   functions->ReadPixels = intelReadPixels;
}

