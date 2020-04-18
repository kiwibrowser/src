/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */
 

#include "main/macros.h"
#include "main/mtypes.h"
#include "main/glformats.h"
#include "main/samplerobj.h"
#include "main/texobj.h"

#include "st_context.h"
#include "st_cb_texture.h"
#include "st_format.h"
#include "st_atom.h"
#include "st_texture.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"

#include "cso_cache/cso_context.h"


/**
 * Convert GLenum texcoord wrap tokens to pipe tokens.
 */
static GLuint
gl_wrap_xlate(GLenum wrap)
{
   switch (wrap) {
   case GL_REPEAT:
      return PIPE_TEX_WRAP_REPEAT;
   case GL_CLAMP:
      return PIPE_TEX_WRAP_CLAMP;
   case GL_CLAMP_TO_EDGE:
      return PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   case GL_CLAMP_TO_BORDER:
      return PIPE_TEX_WRAP_CLAMP_TO_BORDER;
   case GL_MIRRORED_REPEAT:
      return PIPE_TEX_WRAP_MIRROR_REPEAT;
   case GL_MIRROR_CLAMP_EXT:
      return PIPE_TEX_WRAP_MIRROR_CLAMP;
   case GL_MIRROR_CLAMP_TO_EDGE_EXT:
      return PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE;
   case GL_MIRROR_CLAMP_TO_BORDER_EXT:
      return PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER;
   default:
      assert(0);
      return 0;
   }
}


static GLuint
gl_filter_to_mip_filter(GLenum filter)
{
   switch (filter) {
   case GL_NEAREST:
   case GL_LINEAR:
      return PIPE_TEX_MIPFILTER_NONE;

   case GL_NEAREST_MIPMAP_NEAREST:
   case GL_LINEAR_MIPMAP_NEAREST:
      return PIPE_TEX_MIPFILTER_NEAREST;

   case GL_NEAREST_MIPMAP_LINEAR:
   case GL_LINEAR_MIPMAP_LINEAR:
      return PIPE_TEX_MIPFILTER_LINEAR;

   default:
      assert(0);
      return PIPE_TEX_MIPFILTER_NONE;
   }
}


static GLuint
gl_filter_to_img_filter(GLenum filter)
{
   switch (filter) {
   case GL_NEAREST:
   case GL_NEAREST_MIPMAP_NEAREST:
   case GL_NEAREST_MIPMAP_LINEAR:
      return PIPE_TEX_FILTER_NEAREST;

   case GL_LINEAR:
   case GL_LINEAR_MIPMAP_NEAREST:
   case GL_LINEAR_MIPMAP_LINEAR:
      return PIPE_TEX_FILTER_LINEAR;

   default:
      assert(0);
      return PIPE_TEX_FILTER_NEAREST;
   }
}


static void
convert_sampler(struct st_context *st,
                struct pipe_sampler_state *sampler,
                GLuint texUnit)
{
   struct gl_texture_object *texobj;
   struct gl_context *ctx = st->ctx;
   struct gl_sampler_object *msamp;

   texobj = ctx->Texture.Unit[texUnit]._Current;
   if (!texobj) {
      texobj = _mesa_get_fallback_texture(ctx, TEXTURE_2D_INDEX);
   }

   msamp = _mesa_get_samplerobj(ctx, texUnit);

   memset(sampler, 0, sizeof(*sampler));
   sampler->wrap_s = gl_wrap_xlate(msamp->WrapS);
   sampler->wrap_t = gl_wrap_xlate(msamp->WrapT);
   sampler->wrap_r = gl_wrap_xlate(msamp->WrapR);

   sampler->min_img_filter = gl_filter_to_img_filter(msamp->MinFilter);
   sampler->min_mip_filter = gl_filter_to_mip_filter(msamp->MinFilter);
   sampler->mag_img_filter = gl_filter_to_img_filter(msamp->MagFilter);

   if (texobj->Target != GL_TEXTURE_RECTANGLE_ARB)
      sampler->normalized_coords = 1;

   sampler->lod_bias = ctx->Texture.Unit[texUnit].LodBias + msamp->LodBias;

   sampler->min_lod = CLAMP(msamp->MinLod,
                            0.0f,
                            (GLfloat) texobj->MaxLevel - texobj->BaseLevel);
   sampler->max_lod = MIN2((GLfloat) texobj->MaxLevel - texobj->BaseLevel,
                           msamp->MaxLod);
   if (sampler->max_lod < sampler->min_lod) {
      /* The GL spec doesn't seem to specify what to do in this case.
       * Swap the values.
       */
      float tmp = sampler->max_lod;
      sampler->max_lod = sampler->min_lod;
      sampler->min_lod = tmp;
      assert(sampler->min_lod <= sampler->max_lod);
   }

   if (msamp->BorderColor.ui[0] ||
       msamp->BorderColor.ui[1] ||
       msamp->BorderColor.ui[2] ||
       msamp->BorderColor.ui[3]) {
      struct gl_texture_image *teximg;
      GLboolean is_integer = GL_FALSE;

      teximg = texobj->Image[0][texobj->BaseLevel];

      if (teximg) {
         is_integer = _mesa_is_enum_format_integer(teximg->InternalFormat);
      }

      st_translate_color(&msamp->BorderColor,
                         &sampler->border_color,
                         teximg ? teximg->_BaseFormat : GL_RGBA, is_integer);
   }

   sampler->max_anisotropy = (msamp->MaxAnisotropy == 1.0 ?
                              0 : (GLuint) msamp->MaxAnisotropy);

   /* only care about ARB_shadow, not SGI shadow */
   if (msamp->CompareMode == GL_COMPARE_R_TO_TEXTURE) {
      sampler->compare_mode = PIPE_TEX_COMPARE_R_TO_TEXTURE;
      sampler->compare_func
         = st_compare_func_to_pipe(msamp->CompareFunc);
   }

   sampler->seamless_cube_map =
      ctx->Texture.CubeMapSeamless || msamp->CubeMapSeamless;
}


/**
 * Update the gallium driver's sampler state for fragment, vertex or
 * geometry shader stage.
 */
static void
update_shader_samplers(struct st_context *st,
                       unsigned shader_stage,
                       const struct gl_program *prog,
                       unsigned max_units,
                       struct pipe_sampler_state *samplers,
                       unsigned *num_samplers)
{
   GLuint unit;
   GLbitfield samplers_used;
   const GLuint old_max = *num_samplers;

   samplers_used = prog->SamplersUsed;

   if (*num_samplers == 0 && samplers_used == 0x0)
       return;

   *num_samplers = 0;

   /* loop over sampler units (aka tex image units) */
   for (unit = 0; unit < max_units; unit++, samplers_used >>= 1) {
      struct pipe_sampler_state *sampler = samplers + unit;

      if (samplers_used & 1) {
         const GLuint texUnit = prog->SamplerUnits[unit];

         convert_sampler(st, sampler, texUnit);

         *num_samplers = unit + 1;

         cso_single_sampler(st->cso_context, shader_stage, unit, sampler);
      }
      else if (samplers_used != 0 || unit < old_max) {
         cso_single_sampler(st->cso_context, shader_stage, unit, NULL);
      }
      else {
         /* if we've reset all the old samplers and we have no more new ones */
         break;
      }
   }

   cso_single_sampler_done(st->cso_context, shader_stage);
}


static void
update_samplers(struct st_context *st)
{
   const struct gl_context *ctx = st->ctx;

   update_shader_samplers(st,
                          PIPE_SHADER_FRAGMENT,
                          &ctx->FragmentProgram._Current->Base,
                          ctx->Const.MaxTextureImageUnits,
                          st->state.samplers[PIPE_SHADER_FRAGMENT],
                          &st->state.num_samplers[PIPE_SHADER_FRAGMENT]);

   update_shader_samplers(st,
                          PIPE_SHADER_VERTEX,
                          &ctx->VertexProgram._Current->Base,
                          ctx->Const.MaxVertexTextureImageUnits,
                          st->state.samplers[PIPE_SHADER_VERTEX],
                          &st->state.num_samplers[PIPE_SHADER_VERTEX]);

   if (ctx->GeometryProgram._Current) {
      update_shader_samplers(st,
                             PIPE_SHADER_GEOMETRY,
                             &ctx->GeometryProgram._Current->Base,
                             ctx->Const.MaxGeometryTextureImageUnits,
                             st->state.samplers[PIPE_SHADER_GEOMETRY],
                             &st->state.num_samplers[PIPE_SHADER_GEOMETRY]);
   }
}


const struct st_tracked_state st_update_sampler = {
   "st_update_sampler",					/* name */
   {							/* dirty */
      _NEW_TEXTURE,					/* mesa */
      0,						/* st */
   },
   update_samplers					/* update */
};
