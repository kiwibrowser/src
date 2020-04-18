/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_string.h"
#include "util/u_math.h"

#include "svga_winsys.h"
#include "svga_public.h"
#include "svga_context.h"
#include "svga_format.h"
#include "svga_screen.h"
#include "svga_resource_texture.h"
#include "svga_resource.h"
#include "svga_debug.h"

#include "svga3d_shaderdefs.h"


#ifdef DEBUG
int SVGA_DEBUG = 0;

static const struct debug_named_value svga_debug_flags[] = {
   { "dma",      DEBUG_DMA, NULL },
   { "tgsi",     DEBUG_TGSI, NULL },
   { "pipe",     DEBUG_PIPE, NULL },
   { "state",    DEBUG_STATE, NULL },
   { "screen",   DEBUG_SCREEN, NULL },
   { "tex",      DEBUG_TEX, NULL },
   { "swtnl",    DEBUG_SWTNL, NULL },
   { "const",    DEBUG_CONSTS, NULL },
   { "viewport", DEBUG_VIEWPORT, NULL },
   { "views",    DEBUG_VIEWS, NULL },
   { "perf",     DEBUG_PERF, NULL },
   { "flush",    DEBUG_FLUSH, NULL },
   { "sync",     DEBUG_SYNC, NULL },
   { "cache",    DEBUG_CACHE, NULL },
   DEBUG_NAMED_VALUE_END
};
#endif

static const char *
svga_get_vendor( struct pipe_screen *pscreen )
{
   return "VMware, Inc.";
}


static const char *
svga_get_name( struct pipe_screen *pscreen )
{
   const char *build = "", *llvm = "", *mutex = "";
   static char name[100];
#ifdef DEBUG
   /* Only return internal details in the DEBUG version:
    */
   build = "build: DEBUG;";
   mutex = "mutex: " PIPE_ATOMIC ";";
#ifdef HAVE_LLVM
   llvm = "LLVM;";
#endif
#else
   build = "build: RELEASE;";
#endif

   util_snprintf(name, sizeof(name), "SVGA3D; %s %s %s", build, mutex, llvm);
   return name;
}




static float
svga_get_paramf(struct pipe_screen *screen, enum pipe_capf param)
{
   struct svga_screen *svgascreen = svga_screen(screen);
   struct svga_winsys_screen *sws = svgascreen->sws;
   SVGA3dDevCapResult result;

   switch (param) {
   case PIPE_CAPF_MAX_LINE_WIDTH:
      /* fall-through */
   case PIPE_CAPF_MAX_LINE_WIDTH_AA:
      return 7.0;

   case PIPE_CAPF_MAX_POINT_WIDTH:
      /* fall-through */
   case PIPE_CAPF_MAX_POINT_WIDTH_AA:
      return svgascreen->maxPointSize;

   case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
      if(!sws->get_cap(sws, SVGA3D_DEVCAP_MAX_TEXTURE_ANISOTROPY, &result))
         return 4.0;
      return result.u;

   case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
      return 15.0;
   case PIPE_CAPF_GUARD_BAND_LEFT:
   case PIPE_CAPF_GUARD_BAND_TOP:
   case PIPE_CAPF_GUARD_BAND_RIGHT:
   case PIPE_CAPF_GUARD_BAND_BOTTOM:
      return 0.0;
   }

   debug_printf("Unexpected PIPE_CAPF_ query %u\n", param);
   return 0;
}


static int
svga_get_param(struct pipe_screen *screen, enum pipe_cap param)
{
   struct svga_screen *svgascreen = svga_screen(screen);
   struct svga_winsys_screen *sws = svgascreen->sws;
   SVGA3dDevCapResult result;

   switch (param) {
   case PIPE_CAP_MAX_COMBINED_SAMPLERS:
      return 16;
   case PIPE_CAP_NPOT_TEXTURES:
      return 1;
   case PIPE_CAP_TWO_SIDED_STENCIL:
      return 1;
   case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
      return 0;
   case PIPE_CAP_ANISOTROPIC_FILTER:
      return 1;
   case PIPE_CAP_POINT_SPRITE:
      return 1;
   case PIPE_CAP_MAX_RENDER_TARGETS:
      if(!sws->get_cap(sws, SVGA3D_DEVCAP_MAX_RENDER_TARGETS, &result))
         return 1;
      if(!result.u)
         return 1;
      return MIN2(result.u, PIPE_MAX_COLOR_BUFS);
   case PIPE_CAP_OCCLUSION_QUERY:
      return 1;
   case PIPE_CAP_TIMER_QUERY:
      return 0;
   case PIPE_CAP_TEXTURE_SHADOW_MAP:
      return 1;
   case PIPE_CAP_TEXTURE_SWIZZLE:
      return 1;
   case PIPE_CAP_USER_VERTEX_BUFFERS:
   case PIPE_CAP_USER_INDEX_BUFFERS:
      return 0;
   case PIPE_CAP_USER_CONSTANT_BUFFERS:
      return 1;
   case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
      return 16;

   case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
      {
         unsigned levels = SVGA_MAX_TEXTURE_LEVELS;
         if (sws->get_cap(sws, SVGA3D_DEVCAP_MAX_TEXTURE_WIDTH, &result))
            levels = MIN2(util_logbase2(result.u) + 1, levels);
         else
            levels = 12 /* 2048x2048 */;
         if (sws->get_cap(sws, SVGA3D_DEVCAP_MAX_TEXTURE_HEIGHT, &result))
            levels = MIN2(util_logbase2(result.u) + 1, levels);
         else
            levels = 12 /* 2048x2048 */;
         return levels;
      }

   case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      if (!sws->get_cap(sws, SVGA3D_DEVCAP_MAX_VOLUME_EXTENT, &result))
         return 8;  /* max 128x128x128 */
      return MIN2(util_logbase2(result.u) + 1, SVGA_MAX_TEXTURE_LEVELS);

   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
      /*
       * No mechanism to query the host, and at least limited to 2048x2048 on
       * certain hardware.
       */
      return MIN2(screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_2D_LEVELS),
                  12 /* 2048x2048 */);

   case PIPE_CAP_BLEND_EQUATION_SEPARATE: /* req. for GL 1.5 */
      return 1;

   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
      return 1;
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
      return 0;

   case PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE:
      return 1;

   case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
      return 1; /* The color outputs of vertex shaders are not clamped */
   case PIPE_CAP_VERTEX_COLOR_CLAMPED:
      return 0; /* The driver can't clamp vertex colors */
   case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
      return 0; /* The driver can't clamp fragment colors */

   case PIPE_CAP_MIXED_COLORBUFFER_FORMATS:
      return 1; /* expected for GL_ARB_framebuffer_object */

   case PIPE_CAP_GLSL_FEATURE_LEVEL:
      return 120;

   /* Unsupported features */
   case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
   case PIPE_CAP_SM3:
   case PIPE_CAP_SHADER_STENCIL_EXPORT:
   case PIPE_CAP_DEPTH_CLIP_DISABLE:
   case PIPE_CAP_SEAMLESS_CUBE_MAP:
   case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
   case PIPE_CAP_INDEP_BLEND_ENABLE:
   case PIPE_CAP_INDEP_BLEND_FUNC:
   case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
   case PIPE_CAP_PRIMITIVE_RESTART:
   case PIPE_CAP_TGSI_INSTANCEID:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
   case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
   case PIPE_CAP_SCALED_RESOLVE:
   case PIPE_CAP_MIN_TEXEL_OFFSET:
   case PIPE_CAP_MAX_TEXEL_OFFSET:
   case PIPE_CAP_CONDITIONAL_RENDER:
   case PIPE_CAP_TEXTURE_BARRIER:
   case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
   case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
   case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
   case PIPE_CAP_TGSI_CAN_COMPACT_VARYINGS:
   case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
   case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_COMPUTE:
   case PIPE_CAP_START_INSTANCE:
   case PIPE_CAP_QUERY_TIMESTAMP:
      return 0;
   case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
      return 1;
   }

   debug_printf("Unexpected PIPE_CAP_ query %u\n", param);
   return 0;
}

static int svga_get_shader_param(struct pipe_screen *screen, unsigned shader, enum pipe_shader_cap param)
{
   struct svga_screen *svgascreen = svga_screen(screen);
   struct svga_winsys_screen *sws = svgascreen->sws;
   SVGA3dDevCapResult result;

   switch (shader)
   {
   case PIPE_SHADER_FRAGMENT:
      switch (param)
      {
      case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
      case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
      case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
      case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
         return 512;
      case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         return SVGA3D_MAX_NESTING_LEVEL;
      case PIPE_SHADER_CAP_MAX_INPUTS:
         return 10;
      case PIPE_SHADER_CAP_MAX_CONSTS:
         return 224;
      case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         return 1;
      case PIPE_SHADER_CAP_MAX_TEMPS:
         if (!sws->get_cap(sws, SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_TEMPS, &result))
            return 32;
         return MIN2(result.u, SVGA3D_TEMPREG_MAX);
      case PIPE_SHADER_CAP_MAX_ADDRS:
      case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
	 /* 
	  * Although PS 3.0 has some addressing abilities it can only represent
	  * loops that can be statically determined and unrolled. Given we can
	  * only handle a subset of the cases that the state tracker already
	  * does it is better to defer loop unrolling to the state tracker.
	  */
         return 0;
      case PIPE_SHADER_CAP_MAX_PREDS:
         return 1;
      case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
         return 1;
      case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
      case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
      case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
         return 0;
      case PIPE_SHADER_CAP_SUBROUTINES:
         return 0;
      case PIPE_SHADER_CAP_INTEGERS:
         return 0;
      case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
         return 16;
      default:
         debug_printf("Unexpected vertex shader query %u\n", param);
         return 0;
      }
      break;
   case PIPE_SHADER_VERTEX:
      switch (param)
      {
      case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
      case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
         if (!sws->get_cap(sws, SVGA3D_DEVCAP_MAX_VERTEX_SHADER_INSTRUCTIONS, &result))
            return 512;
         return result.u;
      case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
      case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
         /* XXX: until we have vertex texture support */
         return 0;
      case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         return SVGA3D_MAX_NESTING_LEVEL;
      case PIPE_SHADER_CAP_MAX_INPUTS:
         return 16;
      case PIPE_SHADER_CAP_MAX_CONSTS:
         return 256;
      case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         return 1;
      case PIPE_SHADER_CAP_MAX_TEMPS:
         if (!sws->get_cap(sws, SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEMPS, &result))
            return 32;
         return MIN2(result.u, SVGA3D_TEMPREG_MAX);
      case PIPE_SHADER_CAP_MAX_ADDRS:
         return 1;
      case PIPE_SHADER_CAP_MAX_PREDS:
         return 1;
      case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
         return 1;
      case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
      case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
         return 1;
      case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
         return 0;
      case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
         return 1;
      case PIPE_SHADER_CAP_SUBROUTINES:
         return 0;
      case PIPE_SHADER_CAP_INTEGERS:
         return 0;
      case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
         return 0;
      default:
         debug_printf("Unexpected vertex shader query %u\n", param);
         return 0;
      }
      break;
   case PIPE_SHADER_GEOMETRY:
      /* no support for geometry shaders at this time */
      return 0;
   default:
      debug_printf("Unexpected shader type (%u) query\n", shader);
      return 0;
   }
   return 0;
}


static boolean
svga_is_format_supported( struct pipe_screen *screen,
                          enum pipe_format format,
                          enum pipe_texture_target target,
                          unsigned sample_count,
                          unsigned tex_usage)
{
   struct svga_screen *ss = svga_screen(screen);
   SVGA3dSurfaceFormat svga_format;
   SVGA3dSurfaceFormatCaps caps;
   SVGA3dSurfaceFormatCaps mask;

   assert(tex_usage);

   if (sample_count > 1) {
      return FALSE;
   }

   svga_format = svga_translate_format(ss, format, tex_usage);
   if (svga_format == SVGA3D_FORMAT_INVALID) {
      return FALSE;
   }

   /*
    * Override host capabilities, so that we end up with the same
    * visuals for all virtual hardware implementations.
    */

   if (tex_usage & PIPE_BIND_DISPLAY_TARGET) {
      switch (svga_format) {
      case SVGA3D_A8R8G8B8:
      case SVGA3D_X8R8G8B8:
      case SVGA3D_R5G6B5:
         break;

      /* Often unsupported/problematic. This means we end up with the same
       * visuals for all virtual hardware implementations.
       */
      case SVGA3D_A4R4G4B4:
      case SVGA3D_A1R5G5B5:
         return FALSE;
         
      default:
         return FALSE;
      }
   }
   
   /*
    * Query the host capabilities.
    */

   svga_get_format_cap(ss, svga_format, &caps);

   mask.value = 0;
   if (tex_usage & PIPE_BIND_RENDER_TARGET) {
      mask.offscreenRenderTarget = 1;
   }
   if (tex_usage & PIPE_BIND_DEPTH_STENCIL) {
      mask.zStencil = 1;
   }
   if (tex_usage & PIPE_BIND_SAMPLER_VIEW) {
      mask.texture = 1;
   }

   return (caps.value & mask.value) == mask.value;
}


static void
svga_fence_reference(struct pipe_screen *screen,
                     struct pipe_fence_handle **ptr,
                     struct pipe_fence_handle *fence)
{
   struct svga_winsys_screen *sws = svga_screen(screen)->sws;
   sws->fence_reference(sws, ptr, fence);
}


static boolean
svga_fence_signalled(struct pipe_screen *screen,
                     struct pipe_fence_handle *fence)
{
   struct svga_winsys_screen *sws = svga_screen(screen)->sws;
   return sws->fence_signalled(sws, fence, 0) == 0;
}


static boolean
svga_fence_finish(struct pipe_screen *screen,
                  struct pipe_fence_handle *fence,
                  uint64_t timeout)
{
   struct svga_winsys_screen *sws = svga_screen(screen)->sws;

   SVGA_DBG(DEBUG_DMA|DEBUG_PERF, "%s fence_ptr %p\n",
            __FUNCTION__, fence);

   return sws->fence_finish(sws, fence, 0) == 0;
}


static void
svga_destroy_screen( struct pipe_screen *screen )
{
   struct svga_screen *svgascreen = svga_screen(screen);
   
   svga_screen_cache_cleanup(svgascreen);

   pipe_mutex_destroy(svgascreen->swc_mutex);
   pipe_mutex_destroy(svgascreen->tex_mutex);

   svgascreen->sws->destroy(svgascreen->sws);
   
   FREE(svgascreen);
}


/**
 * Create a new svga_screen object
 */
struct pipe_screen *
svga_screen_create(struct svga_winsys_screen *sws)
{
   struct svga_screen *svgascreen;
   struct pipe_screen *screen;
   SVGA3dDevCapResult result;
   boolean use_vs30, use_ps30;

#ifdef DEBUG
   SVGA_DEBUG = debug_get_flags_option("SVGA_DEBUG", svga_debug_flags, 0 );
#endif

   svgascreen = CALLOC_STRUCT(svga_screen);
   if (!svgascreen)
      goto error1;

   svgascreen->debug.force_level_surface_view =
      debug_get_bool_option("SVGA_FORCE_LEVEL_SURFACE_VIEW", FALSE);
   svgascreen->debug.force_surface_view =
      debug_get_bool_option("SVGA_FORCE_SURFACE_VIEW", FALSE);
   svgascreen->debug.force_sampler_view =
      debug_get_bool_option("SVGA_FORCE_SAMPLER_VIEW", FALSE);
   svgascreen->debug.no_surface_view =
      debug_get_bool_option("SVGA_NO_SURFACE_VIEW", FALSE);
   svgascreen->debug.no_sampler_view =
      debug_get_bool_option("SVGA_NO_SAMPLER_VIEW", FALSE);

   screen = &svgascreen->screen;

   screen->destroy = svga_destroy_screen;
   screen->get_name = svga_get_name;
   screen->get_vendor = svga_get_vendor;
   screen->get_param = svga_get_param;
   screen->get_shader_param = svga_get_shader_param;
   screen->get_paramf = svga_get_paramf;
   screen->is_format_supported = svga_is_format_supported;
   screen->context_create = svga_context_create;
   screen->fence_reference = svga_fence_reference;
   screen->fence_signalled = svga_fence_signalled;
   screen->fence_finish = svga_fence_finish;
   svgascreen->sws = sws;

   svga_init_screen_resource_functions(svgascreen);

   if (sws->get_hw_version) {
      svgascreen->hw_version = sws->get_hw_version(sws);
   } else {
      svgascreen->hw_version = SVGA3D_HWVERSION_WS65_B1;
   }

   use_ps30 =
      sws->get_cap(sws, SVGA3D_DEVCAP_FRAGMENT_SHADER_VERSION, &result) &&
      result.u >= SVGA3DPSVERSION_30 ? TRUE : FALSE;

   use_vs30 =
      sws->get_cap(sws, SVGA3D_DEVCAP_VERTEX_SHADER_VERSION, &result) &&
      result.u >= SVGA3DVSVERSION_30 ? TRUE : FALSE;

   /* we require Shader model 3.0 or later */
   if (!use_ps30 || !use_vs30)
      goto error2;

   /*
    * The D16, D24X8, and D24S8 formats always do an implicit shadow compare
    * when sampled from, where as the DF16, DF24, and D24S8_INT do not.  So
    * we prefer the later when available.
    *
    * This mimics hardware vendors extensions for D3D depth sampling. See also
    * http://aras-p.info/texts/D3D9GPUHacks.html
    */

   {
      boolean has_df16, has_df24, has_d24s8_int;
      SVGA3dSurfaceFormatCaps caps;
      SVGA3dSurfaceFormatCaps mask;
      mask.value = 0;
      mask.zStencil = 1;
      mask.texture = 1;

      svgascreen->depth.z16 = SVGA3D_Z_D16;
      svgascreen->depth.x8z24 = SVGA3D_Z_D24X8;
      svgascreen->depth.s8z24 = SVGA3D_Z_D24S8;

      svga_get_format_cap(svgascreen, SVGA3D_Z_DF16, &caps);
      has_df16 = (caps.value & mask.value) == mask.value;

      svga_get_format_cap(svgascreen, SVGA3D_Z_DF24, &caps);
      has_df24 = (caps.value & mask.value) == mask.value;

      svga_get_format_cap(svgascreen, SVGA3D_Z_D24S8_INT, &caps);
      has_d24s8_int = (caps.value & mask.value) == mask.value;

      /* XXX: We might want some other logic here.
       * Like if we only have d24s8_int we should
       * emulate the other formats with that.
       */
      if (has_df16) {
         svgascreen->depth.z16 = SVGA3D_Z_DF16;
      }
      if (has_df24) {
         svgascreen->depth.x8z24 = SVGA3D_Z_DF24;
      }
      if (has_d24s8_int) {
         svgascreen->depth.s8z24 = SVGA3D_Z_D24S8_INT;
      }
   }

   if (!sws->get_cap(sws, SVGA3D_DEVCAP_MAX_POINT_SIZE, &result)) {
      svgascreen->maxPointSize = 1.0F;
   } else {
      /* Keep this to a reasonable size to avoid failures in
       * conform/pntaa.c:
       */
      svgascreen->maxPointSize = MIN2(result.f, 80.0f);
   }

   pipe_mutex_init(svgascreen->tex_mutex);
   pipe_mutex_init(svgascreen->swc_mutex);

   svga_screen_cache_init(svgascreen);

   return screen;
error2:
   FREE(svgascreen);
error1:
   return NULL;
}

struct svga_winsys_screen *
svga_winsys_screen(struct pipe_screen *screen)
{
   return svga_screen(screen)->sws;
}

#ifdef DEBUG
struct svga_screen *
svga_screen(struct pipe_screen *screen)
{
   assert(screen);
   assert(screen->destroy == svga_destroy_screen);
   return (struct svga_screen *)screen;
}
#endif
