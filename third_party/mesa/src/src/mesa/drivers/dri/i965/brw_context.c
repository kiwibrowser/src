/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */


#include "main/imports.h"
#include "main/macros.h"
#include "main/simple_list.h"

#include "vbo/vbo_context.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_draw.h"
#include "brw_state.h"

#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_span.h"
#include "intel_tex.h"
#include "intel_tex_obj.h"

#include "tnl/t_pipeline.h"
#include "glsl/ralloc.h"

/***************************************
 * Mesa's Driver Functions
 ***************************************/

static void brwInitDriverFunctions(struct intel_screen *screen,
				   struct dd_function_table *functions)
{
   intelInitDriverFunctions( functions );

   brwInitFragProgFuncs( functions );
   brw_init_queryobj_functions(functions);

   functions->BeginTransformFeedback = brw_begin_transform_feedback;

   if (screen->gen >= 7)
      functions->EndTransformFeedback = gen7_end_transform_feedback;
   else
      functions->EndTransformFeedback = brw_end_transform_feedback;
}

bool
brwCreateContext(int api,
	         const struct gl_config *mesaVis,
		 __DRIcontext *driContextPriv,
                 unsigned major_version,
                 unsigned minor_version,
                 uint32_t flags,
                 unsigned *error,
	         void *sharedContextPrivate)
{
   __DRIscreen *sPriv = driContextPriv->driScreenPriv;
   struct intel_screen *screen = sPriv->driverPrivate;
   struct dd_function_table functions;
   const unsigned req_version = major_version * 10 + minor_version;
   unsigned max_supported_version = 0;
   unsigned i;

#ifdef TEXTURE_FLOAT_ENABLED
   bool has_texture_float = true;
#else
   bool has_texture_float = false;
#endif

   bool supports_gl30 = has_texture_float &&
                        (screen->gen == 6 ||
                         (screen->gen == 7 &&
                          screen->kernel_has_gen7_sol_reset));

   /* Determine max_supported_version. */
   switch (api) {
   case API_OPENGL:
      max_supported_version = supports_gl30 ? 30 : 21;
      break;
   case API_OPENGLES:
      max_supported_version = 11;
      break;
   case API_OPENGLES2:
      max_supported_version = 20;
      break;
   case API_OPENGL_CORE:
      max_supported_version = supports_gl30 ? 31 : 0;
      break;
   default:
      break;
   }

   if (max_supported_version == 0) {
      *error = __DRI_CTX_ERROR_BAD_API;
      return false;
   } else if (req_version > max_supported_version) {
      *error = __DRI_CTX_ERROR_BAD_VERSION;
      return false;
   }

   struct brw_context *brw = rzalloc(NULL, struct brw_context);
   if (!brw) {
      printf("%s: failed to alloc context\n", __FUNCTION__);
      *error = __DRI_CTX_ERROR_NO_MEMORY;
      return false;
   }

   /* brwInitVtbl needs to know the chipset generation so that it can set the
    * right pointers.
    */
   brw->intel.gen = screen->gen;

   brwInitVtbl( brw );

   brwInitDriverFunctions(screen, &functions);

   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;

   if (!intelInitContext( intel, api, mesaVis, driContextPriv,
			  sharedContextPrivate, &functions )) {
      printf("%s: failed to init intel context\n", __FUNCTION__);
      *error = __DRI_CTX_ERROR_NO_MEMORY;
      return false;
   }

   brw_init_surface_formats(brw);

   /* Initialize swrast, tnl driver tables: */
   intelInitSpanFuncs(ctx);

   TNLcontext *tnl = TNL_CONTEXT(ctx);
   if (tnl)
      tnl->Driver.RunPipeline = _tnl_run_pipeline;

   ctx->Const.MaxDualSourceDrawBuffers = 1;
   ctx->Const.MaxDrawBuffers = BRW_MAX_DRAW_BUFFERS;
   ctx->Const.MaxTextureImageUnits = BRW_MAX_TEX_UNIT;
   ctx->Const.MaxTextureCoordUnits = 8; /* Mesa limit */
   ctx->Const.MaxTextureUnits = MIN2(ctx->Const.MaxTextureCoordUnits,
                                     ctx->Const.MaxTextureImageUnits);
   ctx->Const.MaxVertexTextureImageUnits = BRW_MAX_TEX_UNIT;
   ctx->Const.MaxCombinedTextureImageUnits =
      ctx->Const.MaxVertexTextureImageUnits +
      ctx->Const.MaxTextureImageUnits;

   ctx->Const.MaxTextureLevels = 14; /* 8192 */
   if (ctx->Const.MaxTextureLevels > MAX_TEXTURE_LEVELS)
	   ctx->Const.MaxTextureLevels = MAX_TEXTURE_LEVELS;
   ctx->Const.Max3DTextureLevels = 9;
   ctx->Const.MaxCubeTextureLevels = 12;

   if (intel->gen >= 7)
      ctx->Const.MaxArrayTextureLayers = 2048;
   else
      ctx->Const.MaxArrayTextureLayers = 512;

   ctx->Const.MaxTextureRectSize = (1<<12);
   
   ctx->Const.MaxTextureMaxAnisotropy = 16.0;

   /* Hardware only supports a limited number of transform feedback buffers.
    * So we need to override the Mesa default (which is based only on software
    * limits).
    */
   ctx->Const.MaxTransformFeedbackBuffers = BRW_MAX_SOL_BUFFERS;

   /* On Gen6, in the worst case, we use up one binding table entry per
    * transform feedback component (see comments above the definition of
    * BRW_MAX_SOL_BINDINGS, in brw_context.h), so we need to advertise a value
    * for MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS equal to
    * BRW_MAX_SOL_BINDINGS.
    *
    * In "separate components" mode, we need to divide this value by
    * BRW_MAX_SOL_BUFFERS, so that the total number of binding table entries
    * used up by all buffers will not exceed BRW_MAX_SOL_BINDINGS.
    */
   ctx->Const.MaxTransformFeedbackInterleavedComponents = BRW_MAX_SOL_BINDINGS;
   ctx->Const.MaxTransformFeedbackSeparateComponents =
      BRW_MAX_SOL_BINDINGS / BRW_MAX_SOL_BUFFERS;

   if (intel->gen == 6)
      ctx->Const.MaxSamples = 4;
   else if (intel->gen >= 7)
      ctx->Const.MaxSamples = 8;

   /* if conformance mode is set, swrast can handle any size AA point */
   ctx->Const.MaxPointSizeAA = 255.0;

   /* We want the GLSL compiler to emit code that uses condition codes */
   for (i = 0; i <= MESA_SHADER_FRAGMENT; i++) {
      ctx->ShaderCompilerOptions[i].MaxIfDepth = intel->gen < 6 ? 16 : UINT_MAX;
      ctx->ShaderCompilerOptions[i].EmitCondCodes = true;
      ctx->ShaderCompilerOptions[i].EmitNVTempInitialization = true;
      ctx->ShaderCompilerOptions[i].EmitNoNoise = true;
      ctx->ShaderCompilerOptions[i].EmitNoMainReturn = true;
      ctx->ShaderCompilerOptions[i].EmitNoIndirectInput = true;
      ctx->ShaderCompilerOptions[i].EmitNoIndirectOutput = true;

      ctx->ShaderCompilerOptions[i].EmitNoIndirectUniform =
	 (i == MESA_SHADER_FRAGMENT);
      ctx->ShaderCompilerOptions[i].EmitNoIndirectTemp =
	 (i == MESA_SHADER_FRAGMENT);
      ctx->ShaderCompilerOptions[i].LowerClipDistance = true;
   }

   ctx->Const.VertexProgram.MaxNativeInstructions = (16 * 1024);
   ctx->Const.VertexProgram.MaxAluInstructions = 0;
   ctx->Const.VertexProgram.MaxTexInstructions = 0;
   ctx->Const.VertexProgram.MaxTexIndirections = 0;
   ctx->Const.VertexProgram.MaxNativeAluInstructions = 0;
   ctx->Const.VertexProgram.MaxNativeTexInstructions = 0;
   ctx->Const.VertexProgram.MaxNativeTexIndirections = 0;
   ctx->Const.VertexProgram.MaxNativeAttribs = 16;
   ctx->Const.VertexProgram.MaxNativeTemps = 256;
   ctx->Const.VertexProgram.MaxNativeAddressRegs = 1;
   ctx->Const.VertexProgram.MaxNativeParameters = 1024;
   ctx->Const.VertexProgram.MaxEnvParams =
      MIN2(ctx->Const.VertexProgram.MaxNativeParameters,
	   ctx->Const.VertexProgram.MaxEnvParams);

   ctx->Const.FragmentProgram.MaxNativeInstructions = (16 * 1024);
   ctx->Const.FragmentProgram.MaxNativeAluInstructions = (16 * 1024);
   ctx->Const.FragmentProgram.MaxNativeTexInstructions = (16 * 1024);
   ctx->Const.FragmentProgram.MaxNativeTexIndirections = (16 * 1024);
   ctx->Const.FragmentProgram.MaxNativeAttribs = 12;
   ctx->Const.FragmentProgram.MaxNativeTemps = 256;
   ctx->Const.FragmentProgram.MaxNativeAddressRegs = 0;
   ctx->Const.FragmentProgram.MaxNativeParameters = 1024;
   ctx->Const.FragmentProgram.MaxEnvParams =
      MIN2(ctx->Const.FragmentProgram.MaxNativeParameters,
	   ctx->Const.FragmentProgram.MaxEnvParams);

   /* Fragment shaders use real, 32-bit twos-complement integers for all
    * integer types.
    */
   ctx->Const.FragmentProgram.LowInt.RangeMin = 31;
   ctx->Const.FragmentProgram.LowInt.RangeMax = 30;
   ctx->Const.FragmentProgram.LowInt.Precision = 0;
   ctx->Const.FragmentProgram.HighInt = ctx->Const.FragmentProgram.MediumInt
      = ctx->Const.FragmentProgram.LowInt;

   /* Gen6 converts quads to polygon in beginning of 3D pipeline,
      but we're not sure how it's actually done for vertex order,
      that affect provoking vertex decision. Always use last vertex
      convention for quad primitive which works as expected for now. */
   if (intel->gen >= 6)
       ctx->Const.QuadsFollowProvokingVertexConvention = false;

   ctx->Const.QueryCounterBits.Timestamp = 36;

   if (intel->is_g4x || intel->gen >= 5) {
      brw->CMD_VF_STATISTICS = GM45_3DSTATE_VF_STATISTICS;
      brw->CMD_PIPELINE_SELECT = CMD_PIPELINE_SELECT_GM45;
      brw->has_surface_tile_offset = true;
      if (intel->gen < 6)
	  brw->has_compr4 = true;
      brw->has_aa_line_parameters = true;
      brw->has_pln = true;
  } else {
      brw->CMD_VF_STATISTICS = GEN4_3DSTATE_VF_STATISTICS;
      brw->CMD_PIPELINE_SELECT = CMD_PIPELINE_SELECT_965;
   }

   /* WM maximum threads is number of EUs times number of threads per EU. */
   assert(intel->gen <= 7);

   if (intel->is_haswell) {
      if (intel->gt == 1) {
	 brw->max_wm_threads = 102;
	 brw->max_vs_threads = 70;
	 brw->urb.size = 128;
	 brw->urb.max_vs_entries = 640;
	 brw->urb.max_gs_entries = 256;
      } else if (intel->gt == 2) {
	 brw->max_wm_threads = 204;
	 brw->max_vs_threads = 280;
	 brw->urb.size = 256;
	 brw->urb.max_vs_entries = 1664;
	 brw->urb.max_gs_entries = 640;
      }
   } else if (intel->gen == 7) {
      if (intel->gt == 1) {
	 brw->max_wm_threads = 48;
	 brw->max_vs_threads = 36;
	 brw->max_gs_threads = 36;
	 brw->urb.size = 128;
	 brw->urb.max_vs_entries = 512;
	 brw->urb.max_gs_entries = 192;
      } else if (intel->gt == 2) {
	 brw->max_wm_threads = 172;
	 brw->max_vs_threads = 128;
	 brw->max_gs_threads = 128;
	 brw->urb.size = 256;
	 brw->urb.max_vs_entries = 704;
	 brw->urb.max_gs_entries = 320;
      } else {
	 assert(!"Unknown gen7 device.");
      }
   } else if (intel->gen == 6) {
      if (intel->gt == 2) {
	 brw->max_wm_threads = 80;
	 brw->max_vs_threads = 60;
	 brw->max_gs_threads = 60;
	 brw->urb.size = 64;            /* volume 5c.5 section 5.1 */
	 brw->urb.max_vs_entries = 256; /* volume 2a (see 3DSTATE_URB) */
	 brw->urb.max_gs_entries = 256;
      } else {
	 brw->max_wm_threads = 40;
	 brw->max_vs_threads = 24;
	 brw->max_gs_threads = 21; /* conservative; 24 if rendering disabled */
	 brw->urb.size = 32;            /* volume 5c.5 section 5.1 */
	 brw->urb.max_vs_entries = 256; /* volume 2a (see 3DSTATE_URB) */
	 brw->urb.max_gs_entries = 256;
      }
      brw->urb.gen6_gs_previously_active = false;
   } else if (intel->gen == 5) {
      brw->urb.size = 1024;
      brw->max_vs_threads = 72;
      brw->max_gs_threads = 32;
      brw->max_wm_threads = 12 * 6;
   } else if (intel->is_g4x) {
      brw->urb.size = 384;
      brw->max_vs_threads = 32;
      brw->max_gs_threads = 2;
      brw->max_wm_threads = 10 * 5;
   } else if (intel->gen < 6) {
      brw->urb.size = 256;
      brw->max_vs_threads = 16;
      brw->max_gs_threads = 2;
      brw->max_wm_threads = 8 * 4;
      brw->has_negative_rhw_bug = true;
   }

   if (intel->gen <= 7) {
      brw->needs_unlit_centroid_workaround = true;
   }

   brw->prim_restart.in_progress = false;
   brw->prim_restart.enable_cut_index = false;
   intel->hw_ctx = drm_intel_gem_context_create(intel->bufmgr);

   brw_init_state( brw );

   brw->curbe.last_buf = calloc(1, 4096);
   brw->curbe.next_buf = calloc(1, 4096);

   brw->state.dirty.mesa = ~0;
   brw->state.dirty.brw = ~0;

   brw->emit_state_always = 0;

   intel->batch.need_workaround_flush = true;

   ctx->VertexProgram._MaintainTnlProgram = true;
   ctx->FragmentProgram._MaintainTexEnvProgram = true;

   brw_draw_init( brw );

   brw->precompile = driQueryOptionb(&intel->optionCache, "shader_precompile");

   ctx->Const.NativeIntegers = true;
   ctx->Const.UniformBooleanTrue = 1;

   ctx->Const.ForceGLSLExtensionsWarn = driQueryOptionb(&intel->optionCache, "force_glsl_extensions_warn");

   ctx->Const.ContextFlags = 0;
   if ((flags & __DRI_CTX_FLAG_FORWARD_COMPATIBLE) != 0)
      ctx->Const.ContextFlags |= GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT;

   if ((flags & __DRI_CTX_FLAG_DEBUG) != 0)
      ctx->Const.ContextFlags |= GL_CONTEXT_FLAG_DEBUG_BIT;

   return true;
}

