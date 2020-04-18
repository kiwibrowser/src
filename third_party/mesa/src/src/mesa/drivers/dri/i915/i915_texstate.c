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
#include "main/macros.h"
#include "main/colormac.h"
#include "main/samplerobj.h"

#include "intel_mipmap_tree.h"
#include "intel_tex.h"

#include "i915_context.h"
#include "i915_reg.h"


static GLuint
translate_texture_format(gl_format mesa_format, GLenum DepthMode)
{
   switch (mesa_format) {
   case MESA_FORMAT_L8:
      return MAPSURF_8BIT | MT_8BIT_L8;
   case MESA_FORMAT_I8:
      return MAPSURF_8BIT | MT_8BIT_I8;
   case MESA_FORMAT_A8:
      return MAPSURF_8BIT | MT_8BIT_A8;
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
   case MESA_FORMAT_RGBA8888_REV:
      return MAPSURF_32BIT | MT_32BIT_ABGR8888;
   case MESA_FORMAT_YCBCR_REV:
      return (MAPSURF_422 | MT_422_YCRCB_NORMAL);
   case MESA_FORMAT_YCBCR:
      return (MAPSURF_422 | MT_422_YCRCB_SWAPY);
   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_FXT1);
   case MESA_FORMAT_Z16:
      if (DepthMode == GL_ALPHA)
          return (MAPSURF_16BIT | MT_16BIT_A16);
      else if (DepthMode == GL_INTENSITY)
          return (MAPSURF_16BIT | MT_16BIT_I16);
      else
          return (MAPSURF_16BIT | MT_16BIT_L16);
   case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_RGB_DXT1:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_DXT1);
   case MESA_FORMAT_RGBA_DXT3:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_DXT2_3);
   case MESA_FORMAT_RGBA_DXT5:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_DXT4_5);
   case MESA_FORMAT_S8_Z24:
   case MESA_FORMAT_X8_Z24:
      if (DepthMode == GL_ALPHA)
	 return (MAPSURF_32BIT | MT_32BIT_x8A24);
      else if (DepthMode == GL_INTENSITY)
	 return (MAPSURF_32BIT | MT_32BIT_x8I24);
      else
	 return (MAPSURF_32BIT | MT_32BIT_x8L24);
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
      return TEXCOORDMODE_CLAMP_EDGE;   /* not quite correct */
   case GL_CLAMP_TO_EDGE:
      return TEXCOORDMODE_CLAMP_EDGE;
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
i915_update_tex_unit(struct intel_context *intel, GLuint unit, GLuint ss3)
{
   struct gl_context *ctx = &intel->ctx;
   struct i915_context *i915 = i915_context(ctx);
   struct gl_texture_unit *tUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = tUnit->_Current;
   struct intel_texture_object *intelObj = intel_texture_object(tObj);
   struct gl_texture_image *firstImage;
   struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit);
   GLuint *state = i915->state.Tex[unit], format, pitch;
   GLint lodbias, aniso = 0;
   GLubyte border[4];
   GLfloat maxlod;

   memset(state, 0, sizeof(*state));

   /*We need to refcount these. */

   if (i915->state.tex_buffer[unit] != NULL) {
       drm_intel_bo_unreference(i915->state.tex_buffer[unit]);
       i915->state.tex_buffer[unit] = NULL;
   }

   if (!intel_finalize_mipmap_tree(intel, unit))
      return false;

   /* Get first image here, since intelObj->firstLevel will get set in
    * the intel_finalize_mipmap_tree() call above.
    */
   firstImage = tObj->Image[0][tObj->BaseLevel];

   drm_intel_bo_reference(intelObj->mt->region->bo);
   i915->state.tex_buffer[unit] = intelObj->mt->region->bo;
   i915->state.tex_offset[unit] = intelObj->mt->offset;

   format = translate_texture_format(firstImage->TexFormat,
				     tObj->DepthMode);
   pitch = intelObj->mt->region->pitch * intelObj->mt->cpp;

   state[I915_TEXREG_MS3] =
      (((firstImage->Height - 1) << MS3_HEIGHT_SHIFT) |
       ((firstImage->Width - 1) << MS3_WIDTH_SHIFT) | format);

   if (intelObj->mt->region->tiling != I915_TILING_NONE) {
      state[I915_TEXREG_MS3] |= MS3_TILED_SURFACE;
      if (intelObj->mt->region->tiling == I915_TILING_Y)
	 state[I915_TEXREG_MS3] |= MS3_TILE_WALK;
   }

   /* We get one field with fraction bits for the maximum addressable
    * (lowest resolution) LOD.  Use it to cover both MAX_LEVEL and
    * MAX_LOD.
    */
   maxlod = MIN2(sampler->MaxLod, tObj->_MaxLevel - tObj->BaseLevel);
   state[I915_TEXREG_MS4] =
      ((((pitch / 4) - 1) << MS4_PITCH_SHIFT) |
       MS4_CUBE_FACE_ENA_MASK |
       (U_FIXED(CLAMP(maxlod, 0.0, 11.0), 2) << MS4_MAX_LOD_SHIFT) |
       ((firstImage->Depth - 1) << MS4_VOLUME_DEPTH_SHIFT));


   {
      GLuint minFilt, mipFilt, magFilt;

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
         if (sampler->MaxAnisotropy > 2.0)
            aniso = SS2_MAX_ANISO_4;
         else
            aniso = SS2_MAX_ANISO_2;
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
      if (lodbias < -256)
          lodbias = -256;
      if (lodbias > 255)
          lodbias = 255;
      state[I915_TEXREG_SS2] = ((lodbias << SS2_LOD_BIAS_SHIFT) & 
                                SS2_LOD_BIAS_MASK);

      /* YUV conversion:
       */
      if (firstImage->TexFormat == MESA_FORMAT_YCBCR ||
          firstImage->TexFormat == MESA_FORMAT_YCBCR_REV)
         state[I915_TEXREG_SS2] |= SS2_COLORSPACE_CONVERSION;

      /* Shadow:
       */
      if (sampler->CompareMode == GL_COMPARE_R_TO_TEXTURE_ARB &&
          tObj->Target != GL_TEXTURE_3D) {
         if (tObj->Target == GL_TEXTURE_1D) 
            return false;

         state[I915_TEXREG_SS2] |=
            (SS2_SHADOW_ENABLE |
             intel_translate_shadow_compare_func(sampler->CompareFunc));

         minFilt = FILTER_4X4_FLAT;
         magFilt = FILTER_4X4_FLAT;
      }

      state[I915_TEXREG_SS2] |= ((minFilt << SS2_MIN_FILTER_SHIFT) |
                                 (mipFilt << SS2_MIP_FILTER_SHIFT) |
                                 (magFilt << SS2_MAG_FILTER_SHIFT) |
                                 aniso);
   }

   {
      GLenum ws = sampler->WrapS;
      GLenum wt = sampler->WrapT;
      GLenum wr = sampler->WrapR;
      float minlod;

      /* We program 1D textures as 2D textures, so the 2D texcoord could
       * result in sampling border values if we don't set the T wrap to
       * repeat.
       */
      if (tObj->Target == GL_TEXTURE_1D)
	 wt = GL_REPEAT;

      /* 3D textures don't seem to respect the border color.
       * Fallback if there's ever a danger that they might refer to
       * it.  
       * 
       * Effectively this means fallback on 3D clamp or
       * clamp_to_border.
       */
      if (tObj->Target == GL_TEXTURE_3D &&
          (sampler->MinFilter != GL_NEAREST ||
           sampler->MagFilter != GL_NEAREST) &&
          (ws == GL_CLAMP ||
           wt == GL_CLAMP ||
           wr == GL_CLAMP ||
           ws == GL_CLAMP_TO_BORDER ||
           wt == GL_CLAMP_TO_BORDER || wr == GL_CLAMP_TO_BORDER))
         return false;

      /* Only support TEXCOORDMODE_CLAMP_EDGE and TEXCOORDMODE_CUBE (not 
       * used) when using cube map texture coordinates
       */
      if (tObj->Target == GL_TEXTURE_CUBE_MAP_ARB &&
          (((ws != GL_CLAMP) && (ws != GL_CLAMP_TO_EDGE)) ||
           ((wt != GL_CLAMP) && (wt != GL_CLAMP_TO_EDGE))))
          return false;

      /*
       * According to 3DSTATE_MAP_STATE at page of 104 in Bspec
       * Vol3d 3D Instructions:
       *   [DevGDG and DevAlv]: Must be a power of 2 for cube maps.
       *   [DevLPT, DevCST and DevBLB]: If not a power of 2, cube maps
       *      must have all faces enabled.
       *
       * But, as I tested on pineview(DevBLB derived), the rendering is
       * bad(you will find the color isn't samplered right in some
       * fragments). After checking, it seems that the texture layout is
       * wrong: making the width and height align of 4(although this
       * doesn't make much sense) will fix this issue and also broke some
       * others. Well, Bspec mentioned nothing about the layout alignment
       * and layout for NPOT cube map.  I guess the Bspec just assume it's
       * a POT cube map.
       *
       * Thus, I guess we need do this for other platforms as well.
       */
      if (tObj->Target == GL_TEXTURE_CUBE_MAP_ARB &&
          !is_power_of_two(firstImage->Height))
         return false;

      state[I915_TEXREG_SS3] = ss3;     /* SS3_NORMALIZED_COORDS */

      state[I915_TEXREG_SS3] |=
         ((translate_wrap_mode(ws) << SS3_TCX_ADDR_MODE_SHIFT) |
          (translate_wrap_mode(wt) << SS3_TCY_ADDR_MODE_SHIFT) |
          (translate_wrap_mode(wr) << SS3_TCZ_ADDR_MODE_SHIFT));

      minlod = MIN2(sampler->MinLod, tObj->_MaxLevel - tObj->BaseLevel);
      state[I915_TEXREG_SS3] |= (unit << SS3_TEXTUREMAP_INDEX_SHIFT);
      state[I915_TEXREG_SS3] |= (U_FIXED(CLAMP(minlod, 0.0, 11.0), 4) <<
				 SS3_MIN_LOD_SHIFT);

   }

   /* convert border color from float to ubyte */
   CLAMPED_FLOAT_TO_UBYTE(border[0], sampler->BorderColor.f[0]);
   CLAMPED_FLOAT_TO_UBYTE(border[1], sampler->BorderColor.f[1]);
   CLAMPED_FLOAT_TO_UBYTE(border[2], sampler->BorderColor.f[2]);
   CLAMPED_FLOAT_TO_UBYTE(border[3], sampler->BorderColor.f[3]);

   if (firstImage->_BaseFormat == GL_DEPTH_COMPONENT) {
      /* GL specs that border color for depth textures is taken from the
       * R channel, while the hardware uses A.  Spam R into all the channels
       * for safety.
       */
      state[I915_TEXREG_SS4] = PACK_COLOR_8888(border[0],
					       border[0],
					       border[0],
					       border[0]);
   } else {
      state[I915_TEXREG_SS4] = PACK_COLOR_8888(border[3],
					       border[0],
					       border[1],
					       border[2]);
   }


   I915_ACTIVESTATE(i915, I915_UPLOAD_TEX(unit), true);
   /* memcmp was already disabled, but definitely won't work as the
    * region might now change and that wouldn't be detected:
    */
   I915_STATECHANGE(i915, I915_UPLOAD_TEX(unit));


#if 0
   DBG(TEXTURE, "state[I915_TEXREG_SS2] = 0x%x\n", state[I915_TEXREG_SS2]);
   DBG(TEXTURE, "state[I915_TEXREG_SS3] = 0x%x\n", state[I915_TEXREG_SS3]);
   DBG(TEXTURE, "state[I915_TEXREG_SS4] = 0x%x\n", state[I915_TEXREG_SS4]);
   DBG(TEXTURE, "state[I915_TEXREG_MS2] = 0x%x\n", state[I915_TEXREG_MS2]);
   DBG(TEXTURE, "state[I915_TEXREG_MS3] = 0x%x\n", state[I915_TEXREG_MS3]);
   DBG(TEXTURE, "state[I915_TEXREG_MS4] = 0x%x\n", state[I915_TEXREG_MS4]);
#endif

   return true;
}




void
i915UpdateTextureState(struct intel_context *intel)
{
   bool ok = true;
   GLuint i;

   for (i = 0; i < I915_TEX_UNITS && ok; i++) {
      switch (intel->ctx.Texture.Unit[i]._ReallyEnabled) {
      case TEXTURE_1D_BIT:
      case TEXTURE_2D_BIT:
      case TEXTURE_CUBE_BIT:
      case TEXTURE_3D_BIT:
         ok = i915_update_tex_unit(intel, i, SS3_NORMALIZED_COORDS);
         break;
      case TEXTURE_RECT_BIT:
         ok = i915_update_tex_unit(intel, i, 0);
         break;
      case 0:{
            struct i915_context *i915 = i915_context(&intel->ctx);
            if (i915->state.active & I915_UPLOAD_TEX(i))
               I915_ACTIVESTATE(i915, I915_UPLOAD_TEX(i), false);

	    if (i915->state.tex_buffer[i] != NULL) {
	       drm_intel_bo_unreference(i915->state.tex_buffer[i]);
	       i915->state.tex_buffer[i] = NULL;
	    }

            break;
         }
      default:
         ok = false;
         break;
      }
   }

   FALLBACK(intel, I915_FALLBACK_TEXTURE, !ok);
}
