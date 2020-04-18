/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "intel_batchbuffer.h"

#include "main/macros.h"
#include "main/samplerobj.h"

/**
 * Sets the sampler state for a single unit.
 */
static void
gen7_update_sampler_state(struct brw_context *brw, int unit, int ss_index,
			  struct gen7_sampler_state *sampler)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *texObj = texUnit->_Current;
   struct gl_sampler_object *gl_sampler = _mesa_get_samplerobj(ctx, unit);
   bool using_nearest = false;

   /* These don't use samplers at all. */
   if (texObj->Target == GL_TEXTURE_BUFFER)
      return;

   switch (gl_sampler->MinFilter) {
   case GL_NEAREST:
      sampler->ss0.min_filter = BRW_MAPFILTER_NEAREST;
      sampler->ss0.mip_filter = BRW_MIPFILTER_NONE;
      using_nearest = true;
      break;
   case GL_LINEAR:
      sampler->ss0.min_filter = BRW_MAPFILTER_LINEAR;
      sampler->ss0.mip_filter = BRW_MIPFILTER_NONE;
      break;
   case GL_NEAREST_MIPMAP_NEAREST:
      sampler->ss0.min_filter = BRW_MAPFILTER_NEAREST;
      sampler->ss0.mip_filter = BRW_MIPFILTER_NEAREST;
      break;
   case GL_LINEAR_MIPMAP_NEAREST:
      sampler->ss0.min_filter = BRW_MAPFILTER_LINEAR;
      sampler->ss0.mip_filter = BRW_MIPFILTER_NEAREST;
      break;
   case GL_NEAREST_MIPMAP_LINEAR:
      sampler->ss0.min_filter = BRW_MAPFILTER_NEAREST;
      sampler->ss0.mip_filter = BRW_MIPFILTER_LINEAR;
      break;
   case GL_LINEAR_MIPMAP_LINEAR:
      sampler->ss0.min_filter = BRW_MAPFILTER_LINEAR;
      sampler->ss0.mip_filter = BRW_MIPFILTER_LINEAR;
      break;
   default:
      break;
   }

   /* Set Anisotropy: */
   if (gl_sampler->MaxAnisotropy > 1.0) {
      sampler->ss0.min_filter = BRW_MAPFILTER_ANISOTROPIC;
      sampler->ss0.mag_filter = BRW_MAPFILTER_ANISOTROPIC;

      if (gl_sampler->MaxAnisotropy > 2.0) {
	 sampler->ss3.max_aniso = MIN2((gl_sampler->MaxAnisotropy - 2) / 2,
				       BRW_ANISORATIO_16);
      }
   }
   else {
      switch (gl_sampler->MagFilter) {
      case GL_NEAREST:
	 sampler->ss0.mag_filter = BRW_MAPFILTER_NEAREST;
	 using_nearest = true;
	 break;
      case GL_LINEAR:
	 sampler->ss0.mag_filter = BRW_MAPFILTER_LINEAR;
	 break;
      default:
	 break;
      }
   }

   sampler->ss3.r_wrap_mode = translate_wrap_mode(gl_sampler->WrapR,
						  using_nearest);
   sampler->ss3.s_wrap_mode = translate_wrap_mode(gl_sampler->WrapS,
						  using_nearest);
   sampler->ss3.t_wrap_mode = translate_wrap_mode(gl_sampler->WrapT,
						  using_nearest);

   /* Cube-maps on 965 and later must use the same wrap mode for all 3
    * coordinate dimensions.  Futher, only CUBE and CLAMP are valid.
    */
   if (texObj->Target == GL_TEXTURE_CUBE_MAP) {
      if (ctx->Texture.CubeMapSeamless &&
	  (gl_sampler->MinFilter != GL_NEAREST ||
	   gl_sampler->MagFilter != GL_NEAREST)) {
	 sampler->ss3.r_wrap_mode = BRW_TEXCOORDMODE_CUBE;
	 sampler->ss3.s_wrap_mode = BRW_TEXCOORDMODE_CUBE;
	 sampler->ss3.t_wrap_mode = BRW_TEXCOORDMODE_CUBE;
      } else {
	 sampler->ss3.r_wrap_mode = BRW_TEXCOORDMODE_CLAMP;
	 sampler->ss3.s_wrap_mode = BRW_TEXCOORDMODE_CLAMP;
	 sampler->ss3.t_wrap_mode = BRW_TEXCOORDMODE_CLAMP;
      }
   } else if (texObj->Target == GL_TEXTURE_1D) {
      /* There's a bug in 1D texture sampling - it actually pays
       * attention to the wrap_t value, though it should not.
       * Override the wrap_t value here to GL_REPEAT to keep
       * any nonexistent border pixels from floating in.
       */
      sampler->ss3.t_wrap_mode = BRW_TEXCOORDMODE_WRAP;
   }

   /* Set shadow function: */
   if (gl_sampler->CompareMode == GL_COMPARE_R_TO_TEXTURE_ARB) {
      /* Shadowing is "enabled" by emitting a particular sampler
       * message (sample_c).  So need to recompile WM program when
       * shadow comparison is enabled on each/any texture unit.
       */
      sampler->ss1.shadow_function =
	 intel_translate_shadow_compare_func(gl_sampler->CompareFunc);
   }

   /* Set LOD bias: */
   sampler->ss0.lod_bias = S_FIXED(CLAMP(texUnit->LodBias +
					 gl_sampler->LodBias, -16, 15), 8);

   sampler->ss0.lod_preclamp = 1; /* OpenGL mode */
   sampler->ss0.default_color_mode = 0; /* OpenGL/DX10 mode */

   /* Set BaseMipLevel, MaxLOD, MinLOD:
    *
    * XXX: I don't think that using firstLevel, lastLevel works,
    * because we always setup the surface state as if firstLevel ==
    * level zero.  Probably have to subtract firstLevel from each of
    * these:
    */
   sampler->ss0.base_level = U_FIXED(0, 1);

   sampler->ss1.max_lod = U_FIXED(CLAMP(gl_sampler->MaxLod, 0, 13), 8);
   sampler->ss1.min_lod = U_FIXED(CLAMP(gl_sampler->MinLod, 0, 13), 8);

   /* The sampler can handle non-normalized texture rectangle coordinates
    * natively
    */
   if (texObj->Target == GL_TEXTURE_RECTANGLE) {
      sampler->ss3.non_normalized_coord = 1;
   }

   upload_default_color(brw, gl_sampler, unit, ss_index);

   sampler->ss2.default_color_pointer = brw->wm.sdc_offset[ss_index] >> 5;

   if (sampler->ss0.min_filter != BRW_MAPFILTER_NEAREST)
      sampler->ss3.address_round |= BRW_ADDRESS_ROUNDING_ENABLE_U_MIN |
                                    BRW_ADDRESS_ROUNDING_ENABLE_V_MIN |
                                    BRW_ADDRESS_ROUNDING_ENABLE_R_MIN;
   if (sampler->ss0.mag_filter != BRW_MAPFILTER_NEAREST)
      sampler->ss3.address_round |= BRW_ADDRESS_ROUNDING_ENABLE_U_MAG |
                                    BRW_ADDRESS_ROUNDING_ENABLE_V_MAG |
                                    BRW_ADDRESS_ROUNDING_ENABLE_R_MAG;
}


static void
gen7_upload_samplers(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct gen7_sampler_state *samplers;

   /* BRW_NEW_VERTEX_PROGRAM and BRW_NEW_FRAGMENT_PROGRAM */
   struct gl_program *vs = (struct gl_program *) brw->vertex_program;
   struct gl_program *fs = (struct gl_program *) brw->fragment_program;

   GLbitfield SamplersUsed = vs->SamplersUsed | fs->SamplersUsed;

   brw->sampler.count = _mesa_fls(SamplersUsed);

   if (brw->sampler.count == 0)
      return;

   samplers = brw_state_batch(brw, AUB_TRACE_SAMPLER_STATE,
			      brw->sampler.count * sizeof(*samplers),
			      32, &brw->sampler.offset);
   memset(samplers, 0, brw->sampler.count * sizeof(*samplers));

   for (unsigned s = 0; s < brw->sampler.count; s++) {
      if (SamplersUsed & (1 << s)) {
         const unsigned unit = (fs->SamplersUsed & (1 << s)) ?
            fs->SamplerUnits[s] : vs->SamplerUnits[s];
         if (ctx->Texture.Unit[unit]._ReallyEnabled)
            gen7_update_sampler_state(brw, unit, s, &samplers[s]);
      }
   }

   brw->state.dirty.cache |= CACHE_NEW_SAMPLER;
}

const struct brw_tracked_state gen7_samplers = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_VERTEX_PROGRAM |
             BRW_NEW_FRAGMENT_PROGRAM,
      .cache = 0
   },
   .emit = gen7_upload_samplers,
};
