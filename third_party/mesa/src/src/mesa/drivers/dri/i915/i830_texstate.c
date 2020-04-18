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
#include "main/colormac.h"
#include "main/macros.h"
#include "main/samplerobj.h"

#include "intel_mipmap_tree.h"
#include "intel_tex.h"

#include "i830_context.h"
#include "i830_reg.h"
#include "intel_chipset.h"


static GLuint
translate_texture_format(GLuint mesa_format)
{
   switch (mesa_format) {
   case MESA_FORMAT_L8:
      return MAPSURF_8BIT | MT_8BIT_L8;
   case MESA_FORMAT_I8:
      return MAPSURF_8BIT | MT_8BIT_I8;
   case MESA_FORMAT_A8:
      return MAPSURF_8BIT | MT_8BIT_I8; /* Kludge! */
   case MESA_FORMAT_AL88:
      return MAPSURF_16BIT | MT_16BIT_AY88;
   case MESA_FORMAT_RGB565:
      return MAPSURF_16BIT | MT_16BIT_RGB565;
   case MESA_FORMAT_ARGB1555:
      return MAPSURF_16BIT | MT_16BIT_ARGB1555;
   case MESA_FORMAT_ARGB4444:
      return MAPSURF_16BIT | MT_16BIT_ARGB4444;
   case MESA_FORMAT_ARGB8888:
      return MAPSURF_32BIT | MT_32BIT_ARGB8888;
   case MESA_FORMAT_XRGB8888:
      return MAPSURF_32BIT | MT_32BIT_XRGB8888;
   case MESA_FORMAT_YCBCR_REV:
      return (MAPSURF_422 | MT_422_YCRCB_NORMAL);
   case MESA_FORMAT_YCBCR:
      return (MAPSURF_422 | MT_422_YCRCB_SWAPY);
   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_FXT1);
   case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_RGB_DXT1:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_DXT1);
   case MESA_FORMAT_RGBA_DXT3:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_DXT2_3);
   case MESA_FORMAT_RGBA_DXT5:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_DXT4_5);
   default:
      fprintf(stderr, "%s: bad image format %s\n", __FUNCTION__,
	      _mesa_get_format_name(mesa_format));
      abort();
      return 0;
   }
}




/* The i915 (and related graphics cores) do not support GL_CLAMP.  The
 * Intel drivers for "other operating systems" implement GL_CLAMP as
 * GL_CLAMP_TO_EDGE, so the same is done here.
 */
static GLuint
translate_wrap_mode(GLenum wrap)
{
   switch (wrap) {
   case GL_REPEAT:
      return TEXCOORDMODE_WRAP;
   case GL_CLAMP:
   case GL_CLAMP_TO_EDGE:
      return TEXCOORDMODE_CLAMP;        /* not really correct */
   case GL_CLAMP_TO_BORDER:
      return TEXCOORDMODE_CLAMP_BORDER;
   case GL_MIRRORED_REPEAT:
      return TEXCOORDMODE_MIRROR;
   default:
      return TEXCOORDMODE_WRAP;
   }
}


/* Recalculate all state from scratch.  Perhaps not the most
 * efficient, but this has gotten complex enough that we need
 * something which is understandable and reliable.
 */
static bool
i830_update_tex_unit(struct intel_context *intel, GLuint unit, GLuint ss3)
{
   struct gl_context *ctx = &intel->ctx;
   struct i830_context *i830 = i830_context(ctx);
   struct gl_texture_unit *tUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = tUnit->_Current;
   struct intel_texture_object *intelObj = intel_texture_object(tObj);
   struct gl_texture_image *firstImage;
   struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit);
   GLuint *state = i830->state.Tex[unit], format, pitch;
   GLint lodbias;
   GLubyte border[4];
   GLuint dst_x, dst_y;

   memset(state, 0, sizeof(*state));

   /*We need to refcount these. */

   if (i830->state.tex_buffer[unit] != NULL) {
       drm_intel_bo_unreference(i830->state.tex_buffer[unit]);
       i830->state.tex_buffer[unit] = NULL;
   }

   if (!intel_finalize_mipmap_tree(intel, unit))
      return false;

   /* Get first image here, since intelObj->firstLevel will get set in
    * the intel_finalize_mipmap_tree() call above.
    */
   firstImage = tObj->Image[0][tObj->BaseLevel];

   intel_miptree_get_image_offset(intelObj->mt, tObj->BaseLevel, 0, 0,
				  &dst_x, &dst_y);

   drm_intel_bo_reference(intelObj->mt->region->bo);
   i830->state.tex_buffer[unit] = intelObj->mt->region->bo;
   pitch = intelObj->mt->region->pitch * intelObj->mt->cpp;

   /* XXX: This calculation is probably broken for tiled images with
    * a non-page-aligned offset.
    */
   i830->state.tex_offset[unit] = dst_x * intelObj->mt->cpp + dst_y * pitch;

   format = translate_texture_format(firstImage->TexFormat);

   state[I830_TEXREG_TM0LI] = (_3DSTATE_LOAD_STATE_IMMEDIATE_2 |
                               (LOAD_TEXTURE_MAP0 << unit) | 4);

   state[I830_TEXREG_TM0S1] =
      (((firstImage->Height - 1) << TM0S1_HEIGHT_SHIFT) |
       ((firstImage->Width - 1) << TM0S1_WIDTH_SHIFT) | format);

   if (intelObj->mt->region->tiling != I915_TILING_NONE) {
      state[I830_TEXREG_TM0S1] |= TM0S1_TILED_SURFACE;
      if (intelObj->mt->region->tiling == I915_TILING_Y)
	 state[I830_TEXREG_TM0S1] |= TM0S1_TILE_WALK;
   }

   state[I830_TEXREG_TM0S2] =
      ((((pitch / 4) - 1) << TM0S2_PITCH_SHIFT) | TM0S2_CUBE_FACE_ENA_MASK);

   {
      if (tObj->Target == GL_TEXTURE_CUBE_MAP)
         state[I830_TEXREG_CUBE] = (_3DSTATE_MAP_CUBE | MAP_UNIT(unit) |
                                    CUBE_NEGX_ENABLE |
                                    CUBE_POSX_ENABLE |
                                    CUBE_NEGY_ENABLE |
                                    CUBE_POSY_ENABLE |
                                    CUBE_NEGZ_ENABLE | CUBE_POSZ_ENABLE);
      else
         state[I830_TEXREG_CUBE] = (_3DSTATE_MAP_CUBE | MAP_UNIT(unit));
   }




   {
      GLuint minFilt, mipFilt, magFilt;
      float maxlod;
      uint32_t minlod_fixed, maxlod_fixed;

      switch (sampler->MinFilter) {
      case GL_NEAREST:
         minFilt = FILTER_NEAREST;
         mipFilt = MIPFILTER_NONE;
         break;
      case GL_LINEAR:
         minFilt = FILTER_LINEAR;
         mipFilt = MIPFILTER_NONE;
         break;
      case GL_NEAREST_MIPMAP_NEAREST:
         minFilt = FILTER_NEAREST;
         mipFilt = MIPFILTER_NEAREST;
         break;
      case GL_LINEAR_MIPMAP_NEAREST:
         minFilt = FILTER_LINEAR;
         mipFilt = MIPFILTER_NEAREST;
         break;
      case GL_NEAREST_MIPMAP_LINEAR:
         minFilt = FILTER_NEAREST;
         mipFilt = MIPFILTER_LINEAR;
         break;
      case GL_LINEAR_MIPMAP_LINEAR:
         minFilt = FILTER_LINEAR;
         mipFilt = MIPFILTER_LINEAR;
         break;
      default:
         return false;
      }

      if (sampler->MaxAnisotropy > 1.0) {
         minFilt = FILTER_ANISOTROPIC;
         magFilt = FILTER_ANISOTROPIC;
      }
      else {
         switch (sampler->MagFilter) {
         case GL_NEAREST:
            magFilt = FILTER_NEAREST;
            break;
         case GL_LINEAR:
            magFilt = FILTER_LINEAR;
            break;
         default:
            return false;
         }
      }

      lodbias = (int) ((tUnit->LodBias + sampler->LodBias) * 16.0);
      if (lodbias < -64)
          lodbias = -64;
      if (lodbias > 63)
          lodbias = 63;
      
      state[I830_TEXREG_TM0S3] = ((lodbias << TM0S3_LOD_BIAS_SHIFT) & 
                                  TM0S3_LOD_BIAS_MASK);
#if 0
      /* YUV conversion:
       */
      if (firstImage->TexFormat->MesaFormat == MESA_FORMAT_YCBCR ||
          firstImage->TexFormat->MesaFormat == MESA_FORMAT_YCBCR_REV)
         state[I830_TEXREG_TM0S3] |= SS2_COLORSPACE_CONVERSION;
#endif

      /* We get one field with fraction bits for the maximum
       * addressable (smallest resolution) LOD.  Use it to cover both
       * MAX_LEVEL and MAX_LOD.
       */
      minlod_fixed = U_FIXED(CLAMP(sampler->MinLod, 0.0, 11), 4);
      maxlod = MIN2(sampler->MaxLod, tObj->_MaxLevel - tObj->BaseLevel);
      if (intel->intelScreen->deviceID == PCI_CHIP_I855_GM ||
	  intel->intelScreen->deviceID == PCI_CHIP_I865_G) {
	 maxlod_fixed = U_FIXED(CLAMP(maxlod, 0.0, 11.75), 2);
	 maxlod_fixed = MAX2(maxlod_fixed, (minlod_fixed + 3) >> 2);
	 state[I830_TEXREG_TM0S3] |= maxlod_fixed << TM0S3_MIN_MIP_SHIFT;
	 state[I830_TEXREG_TM0S2] |= TM0S2_LOD_PRECLAMP;
      } else {
	 maxlod_fixed = U_FIXED(CLAMP(maxlod, 0.0, 11), 0);
	 maxlod_fixed = MAX2(maxlod_fixed, (minlod_fixed + 15) >> 4);
	 state[I830_TEXREG_TM0S3] |= maxlod_fixed << TM0S3_MIN_MIP_SHIFT_830;
      }
      state[I830_TEXREG_TM0S3] |= minlod_fixed << TM0S3_MAX_MIP_SHIFT;
      state[I830_TEXREG_TM0S3] |= ((minFilt << TM0S3_MIN_FILTER_SHIFT) |
                                   (mipFilt << TM0S3_MIP_FILTER_SHIFT) |
                                   (magFilt << TM0S3_MAG_FILTER_SHIFT));
   }

   {
      GLenum ws = sampler->WrapS;
      GLenum wt = sampler->WrapT;


      /* 3D textures not available on i830
       */
      if (tObj->Target == GL_TEXTURE_3D)
         return false;

      state[I830_TEXREG_MCS] = (_3DSTATE_MAP_COORD_SET_CMD |
                                MAP_UNIT(unit) |
                                ENABLE_TEXCOORD_PARAMS |
                                ss3 |
                                ENABLE_ADDR_V_CNTL |
                                TEXCOORD_ADDR_V_MODE(translate_wrap_mode(wt))
                                | ENABLE_ADDR_U_CNTL |
                                TEXCOORD_ADDR_U_MODE(translate_wrap_mode
                                                     (ws)));
   }

   /* convert border color from float to ubyte */
   CLAMPED_FLOAT_TO_UBYTE(border[0], sampler->BorderColor.f[0]);
   CLAMPED_FLOAT_TO_UBYTE(border[1], sampler->BorderColor.f[1]);
   CLAMPED_FLOAT_TO_UBYTE(border[2], sampler->BorderColor.f[2]);
   CLAMPED_FLOAT_TO_UBYTE(border[3], sampler->BorderColor.f[3]);

   state[I830_TEXREG_TM0S4] = PACK_COLOR_8888(border[3],
					      border[0],
					      border[1],
					      border[2]);

   I830_ACTIVESTATE(i830, I830_UPLOAD_TEX(unit), true);
   /* memcmp was already disabled, but definitely won't work as the
    * region might now change and that wouldn't be detected:
    */
   I830_STATECHANGE(i830, I830_UPLOAD_TEX(unit));
   return true;
}




void
i830UpdateTextureState(struct intel_context *intel)
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   bool ok = true;
   GLuint i;

   for (i = 0; i < I830_TEX_UNITS && ok; i++) {
      switch (intel->ctx.Texture.Unit[i]._ReallyEnabled) {
      case TEXTURE_1D_BIT:
      case TEXTURE_2D_BIT:
      case TEXTURE_CUBE_BIT:
         ok = i830_update_tex_unit(intel, i, TEXCOORDS_ARE_NORMAL);
         break;
      case TEXTURE_RECT_BIT:
         ok = i830_update_tex_unit(intel, i, TEXCOORDS_ARE_IN_TEXELUNITS);
         break;
      case 0:{
	 struct i830_context *i830 = i830_context(&intel->ctx);
         if (i830->state.active & I830_UPLOAD_TEX(i)) 
            I830_ACTIVESTATE(i830, I830_UPLOAD_TEX(i), false);

	 if (i830->state.tex_buffer[i] != NULL) {
	    drm_intel_bo_unreference(i830->state.tex_buffer[i]);
	    i830->state.tex_buffer[i] = NULL;
	 }
         break;
      }
      case TEXTURE_3D_BIT:
      default:
         ok = false;
         break;
      }
   }

   FALLBACK(intel, I830_FALLBACK_TEXTURE, !ok);

   if (ok)
      i830EmitTextureBlend(i830);
}
