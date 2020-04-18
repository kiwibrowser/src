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

#include "i830_context.h"
#include "main/imports.h"
#include "tnl/tnl.h"
#include "tnl/t_vertex.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "intel_span.h"
#include "intel_tris.h"
#include "../glsl/ralloc.h"

/***************************************
 * Mesa's Driver Functions
 ***************************************/

static void
i830InitDriverFunctions(struct dd_function_table *functions)
{
   intelInitDriverFunctions(functions);
   i830InitStateFuncs(functions);
}

extern const struct tnl_pipeline_stage *intel_pipeline[];

bool
i830CreateContext(const struct gl_config * mesaVis,
                  __DRIcontext * driContextPriv,
                  void *sharedContextPrivate)
{
   struct dd_function_table functions;
   struct i830_context *i830 = rzalloc(NULL, struct i830_context);
   struct intel_context *intel = &i830->intel;
   struct gl_context *ctx = &intel->ctx;
   if (!i830)
      return false;

   i830InitVtbl(i830);
   i830InitDriverFunctions(&functions);

   if (!intelInitContext(intel, __DRI_API_OPENGL, mesaVis, driContextPriv,
                         sharedContextPrivate, &functions)) {
      FREE(i830);
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

   intel->ctx.Const.MaxTextureUnits = I830_TEX_UNITS;
   intel->ctx.Const.MaxTextureImageUnits = I830_TEX_UNITS;
   intel->ctx.Const.MaxTextureCoordUnits = I830_TEX_UNITS;

   /* Advertise the full hardware capabilities.  The new memory
    * manager should cope much better with overload situations:
    */
   ctx->Const.MaxTextureLevels = 12;
   ctx->Const.Max3DTextureLevels = 9;
   ctx->Const.MaxCubeTextureLevels = 11;
   ctx->Const.MaxTextureRectSize = (1 << 11);
   ctx->Const.MaxTextureUnits = I830_TEX_UNITS;

   ctx->Const.MaxTextureMaxAnisotropy = 2.0;

   ctx->Const.MaxDrawBuffers = 1;

   _tnl_init_vertices(ctx, ctx->Const.MaxArrayLockSize + 12,
                      18 * sizeof(GLfloat));

   intel->verts = TNL_CONTEXT(ctx)->clipspace.vertex_buf;

   i830InitState(i830);

   _tnl_allow_vertex_fog(ctx, 1);
   _tnl_allow_pixel_fog(ctx, 0);

   return true;
}
