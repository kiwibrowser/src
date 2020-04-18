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

#include "i915_context.h"
#include "main/imports.h"
#include "main/macros.h"
#include "intel_tris.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "tnl/t_vertex.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "../glsl/ralloc.h"

#include "i915_reg.h"
#include "i915_program.h"

#include "intel_span.h"

/***************************************
 * Mesa's Driver Functions
 ***************************************/

/* Override intel default.
 */
static void
i915InvalidateState(struct gl_context * ctx, GLuint new_state)
{
   _swrast_InvalidateState(ctx, new_state);
   _swsetup_InvalidateState(ctx, new_state);
   _vbo_InvalidateState(ctx, new_state);
   _tnl_InvalidateState(ctx, new_state);
   _tnl_invalidate_vertex_state(ctx, new_state);
   intel_context(ctx)->NewGLState |= new_state;

   /* Todo: gather state values under which tracked parameters become
    * invalidated, add callbacks for things like
    * ProgramLocalParameters, etc.
    */
   {
      struct i915_fragment_program *p =
         (struct i915_fragment_program *) ctx->FragmentProgram._Current;
      if (p && p->nr_params)
         p->params_uptodate = 0;
   }

   if (new_state & (_NEW_STENCIL | _NEW_BUFFERS | _NEW_POLYGON))
      i915_update_stencil(ctx);
   if (new_state & (_NEW_LIGHT))
       i915_update_provoking_vertex(ctx);
   if (new_state & (_NEW_PROGRAM | _NEW_PROGRAM_CONSTANTS))
       i915_update_program(ctx);
   if (new_state & (_NEW_PROGRAM | _NEW_POINT))
       i915_update_sprite_point_enable(ctx);
}


static void
i915InitDriverFunctions(struct dd_function_table *functions)
{
   intelInitDriverFunctions(functions);
   i915InitStateFunctions(functions);
   i915InitFragProgFuncs(functions);
   functions->UpdateState = i915InvalidateState;
}

/* Note: this is shared with i830. */
void
intel_init_texture_formats(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_screen *intel_screen = intel->intelScreen;

   ctx->TextureFormatSupported[MESA_FORMAT_ARGB8888] = true;
   if (intel_screen->deviceID != PCI_CHIP_I830_M &&
       intel_screen->deviceID != PCI_CHIP_845_G)
      ctx->TextureFormatSupported[MESA_FORMAT_XRGB8888] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_ARGB4444] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_ARGB1555] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_RGB565] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_L8] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_A8] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_I8] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_AL88] = true;

   /* Depth and stencil */
   ctx->TextureFormatSupported[MESA_FORMAT_S8_Z24] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_X8_Z24] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_S8] = intel->has_separate_stencil;

   /*
    * This was disabled in initial FBO enabling to avoid combinations
    * of depth+stencil that wouldn't work together.  We since decided
    * that it was OK, since it's up to the app to come up with the
    * combo that actually works, so this can probably be re-enabled.
    */
   /*
   ctx->TextureFormatSupported[MESA_FORMAT_Z16] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_Z24] = true;
   */

   /* ctx->Extensions.MESA_ycbcr_texture */
   ctx->TextureFormatSupported[MESA_FORMAT_YCBCR] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_YCBCR_REV] = true;

   /* GL_3DFX_texture_compression_FXT1 */
   ctx->TextureFormatSupported[MESA_FORMAT_RGB_FXT1] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_RGBA_FXT1] = true;

   /* GL_EXT_texture_compression_s3tc */
   ctx->TextureFormatSupported[MESA_FORMAT_RGB_DXT1] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_RGBA_DXT1] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_RGBA_DXT3] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_RGBA_DXT5] = true;
}

extern const struct tnl_pipeline_stage *intel_pipeline[];

bool
i915CreateContext(int api,
		  const struct gl_config * mesaVis,
                  __DRIcontext * driContextPriv,
                  unsigned major_version,
                  unsigned minor_version,
                  unsigned *error,
                  void *sharedContextPrivate)
{
   struct dd_function_table functions;
   struct i915_context *i915 = rzalloc(NULL, struct i915_context);
   struct intel_context *intel = &i915->intel;
   struct gl_context *ctx = &intel->ctx;

   if (!i915) {
      *error = __DRI_CTX_ERROR_NO_MEMORY;
      return false;
   }

   i915InitVtbl(i915);

   i915InitDriverFunctions(&functions);

   if (!intelInitContext(intel, api, mesaVis, driContextPriv,
                         sharedContextPrivate, &functions)) {
      *error = __DRI_CTX_ERROR_NO_MEMORY;
      return false;
   }

   /* Now that the extension bits are known, filter against the requested API
    * and version.
    */
   switch (api) {
   case API_OPENGL: {
      const unsigned max_version =
         (ctx->Extensions.ARB_fragment_shader &&
          ctx->Extensions.ARB_occlusion_query) ? 20 : 15;
      const unsigned req_version = major_version * 10 + minor_version;

      if (req_version > max_version) {
         *error = __DRI_CTX_ERROR_BAD_VERSION;
         return false;
      }
      break;
   }
   case API_OPENGLES:
   case API_OPENGLES2:
      break;
   default:
      *error = __DRI_CTX_ERROR_BAD_API;
      return false;
   }

   intel_init_texture_formats(ctx);

   _math_matrix_ctr(&intel->ViewportMatrix);

   /* Initialize swrast, tnl driver tables: */
   intelInitSpanFuncs(ctx);
   intelInitTriFuncs(ctx);

   /* Install the customized pipeline: */
   _tnl_destroy_pipeline(ctx);
   _tnl_install_pipeline(ctx, intel_pipeline);

   if (intel->no_rast)
      FALLBACK(intel, INTEL_FALLBACK_USER, 1);

   ctx->Const.MaxTextureUnits = I915_TEX_UNITS;
   ctx->Const.MaxTextureImageUnits = I915_TEX_UNITS;
   ctx->Const.MaxTextureCoordUnits = I915_TEX_UNITS;
   ctx->Const.MaxVarying = I915_TEX_UNITS;
   ctx->Const.MaxCombinedTextureImageUnits =
      ctx->Const.MaxVertexTextureImageUnits +
      ctx->Const.MaxTextureImageUnits;

   /* Advertise the full hardware capabilities.  The new memory
    * manager should cope much better with overload situations:
    */
   ctx->Const.MaxTextureLevels = 12;
   ctx->Const.Max3DTextureLevels = 9;
   ctx->Const.MaxCubeTextureLevels = 12;
   ctx->Const.MaxTextureRectSize = (1 << 11);
   ctx->Const.MaxTextureUnits = I915_TEX_UNITS;

   ctx->Const.MaxTextureMaxAnisotropy = 4.0;

   /* GL_ARB_fragment_program limits - don't think Mesa actually
    * validates programs against these, and in any case one ARB
    * instruction can translate to more than one HW instruction, so
    * we'll still have to check and fallback each time.
    */
   ctx->Const.FragmentProgram.MaxNativeTemps = I915_MAX_TEMPORARY;
   ctx->Const.FragmentProgram.MaxNativeAttribs = 11;    /* 8 tex, 2 color, fog */
   ctx->Const.FragmentProgram.MaxNativeParameters = I915_MAX_CONSTANT;
   ctx->Const.FragmentProgram.MaxNativeAluInstructions = I915_MAX_ALU_INSN;
   ctx->Const.FragmentProgram.MaxNativeTexInstructions = I915_MAX_TEX_INSN;
   ctx->Const.FragmentProgram.MaxNativeInstructions = (I915_MAX_ALU_INSN +
                                                       I915_MAX_TEX_INSN);
   ctx->Const.FragmentProgram.MaxNativeTexIndirections =
      I915_MAX_TEX_INDIRECT;
   ctx->Const.FragmentProgram.MaxNativeAddressRegs = 0; /* I don't think we have one */
   ctx->Const.FragmentProgram.MaxEnvParams =
      MIN2(ctx->Const.FragmentProgram.MaxNativeParameters,
	   ctx->Const.FragmentProgram.MaxEnvParams);

   /* i915 stores all values in single-precision floats.  Values aren't set
    * for other program targets because software is used for those targets.
    */
   ctx->Const.FragmentProgram.MediumFloat.RangeMin = 127;
   ctx->Const.FragmentProgram.MediumFloat.RangeMax = 127;
   ctx->Const.FragmentProgram.MediumFloat.Precision = 23;
   ctx->Const.FragmentProgram.LowFloat = ctx->Const.FragmentProgram.HighFloat =
      ctx->Const.FragmentProgram.MediumFloat;
   ctx->Const.FragmentProgram.MediumInt.RangeMin = 24;
   ctx->Const.FragmentProgram.MediumInt.RangeMax = 24;
   ctx->Const.FragmentProgram.MediumInt.Precision = 0;
   ctx->Const.FragmentProgram.LowInt = ctx->Const.FragmentProgram.HighInt =
      ctx->Const.FragmentProgram.MediumInt;

   ctx->FragmentProgram._MaintainTexEnvProgram = true;

   /* FINISHME: Are there other options that should be enabled for software
    * FINISHME: vertex shaders?
    */
   ctx->ShaderCompilerOptions[MESA_SHADER_VERTEX].EmitCondCodes = true;

   struct gl_shader_compiler_options *const fs_options =
      & ctx->ShaderCompilerOptions[MESA_SHADER_FRAGMENT];
   fs_options->MaxIfDepth = 0;
   fs_options->EmitNoNoise = true;
   fs_options->EmitNoPow = true;
   fs_options->EmitNoMainReturn = true;
   fs_options->EmitNoIndirectInput = true;
   fs_options->EmitNoIndirectOutput = true;
   fs_options->EmitNoIndirectUniform = true;
   fs_options->EmitNoIndirectTemp = true;

   ctx->Const.MaxDrawBuffers = 1;

   _tnl_init_vertices(ctx, ctx->Const.MaxArrayLockSize + 12,
                      36 * sizeof(GLfloat));

   intel->verts = TNL_CONTEXT(ctx)->clipspace.vertex_buf;

   i915InitState(i915);

   /* Always enable pixel fog.  Vertex fog using fog coord will conflict
    * with fog code appended onto fragment program.
    */
   _tnl_allow_vertex_fog(ctx, 0);
   _tnl_allow_pixel_fog(ctx, 1);

   return true;
}
