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


#include "main/glheader.h"
#include "main/context.h"
#include "main/extensions.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/imports.h"
#include "main/points.h"
#include "main/renderbuffer.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "drivers/common/driverfuncs.h"
#include "drivers/common/meta.h"

#include "intel_chipset.h"
#include "intel_buffers.h"
#include "intel_tex.h"
#include "intel_batchbuffer.h"
#include "intel_clear.h"
#include "intel_extensions.h"
#include "intel_pixel.h"
#include "intel_regions.h"
#include "intel_buffer_objects.h"
#include "intel_fbo.h"
#include "intel_bufmgr.h"
#include "intel_screen.h"
#include "intel_mipmap_tree.h"

#include "utils.h"
#include "../glsl/ralloc.h"

#ifndef INTEL_DEBUG
int INTEL_DEBUG = (0);
#endif


static const GLubyte *
intelGetString(struct gl_context * ctx, GLenum name)
{
   const struct intel_context *const intel = intel_context(ctx);
   const char *chipset;
   static char buffer[128];

   switch (name) {
   case GL_VENDOR:
      return (GLubyte *) "Intel Open Source Technology Center";
      break;

   case GL_RENDERER:
      switch (intel->intelScreen->deviceID) {
      case PCI_CHIP_845_G:
         chipset = "Intel(R) 845G";
         break;
      case PCI_CHIP_I830_M:
         chipset = "Intel(R) 830M";
         break;
      case PCI_CHIP_I855_GM:
         chipset = "Intel(R) 852GM/855GM";
         break;
      case PCI_CHIP_I865_G:
         chipset = "Intel(R) 865G";
         break;
      case PCI_CHIP_I915_G:
         chipset = "Intel(R) 915G";
         break;
      case PCI_CHIP_E7221_G:
	 chipset = "Intel (R) E7221G (i915)";
	 break;
      case PCI_CHIP_I915_GM:
         chipset = "Intel(R) 915GM";
         break;
      case PCI_CHIP_I945_G:
         chipset = "Intel(R) 945G";
         break;
      case PCI_CHIP_I945_GM:
         chipset = "Intel(R) 945GM";
         break;
      case PCI_CHIP_I945_GME:
         chipset = "Intel(R) 945GME";
         break;
      case PCI_CHIP_G33_G:
	 chipset = "Intel(R) G33";
	 break;
      case PCI_CHIP_Q35_G:
	 chipset = "Intel(R) Q35";
	 break;
      case PCI_CHIP_Q33_G:
	 chipset = "Intel(R) Q33";
	 break;
      case PCI_CHIP_IGD_GM:
      case PCI_CHIP_IGD_G:
	 chipset = "Intel(R) IGD";
	 break;
      case PCI_CHIP_I965_Q:
	 chipset = "Intel(R) 965Q";
	 break;
      case PCI_CHIP_I965_G:
      case PCI_CHIP_I965_G_1:
	 chipset = "Intel(R) 965G";
	 break;
      case PCI_CHIP_I946_GZ:
	 chipset = "Intel(R) 946GZ";
	 break;
      case PCI_CHIP_I965_GM:
	 chipset = "Intel(R) 965GM";
	 break;
      case PCI_CHIP_I965_GME:
	 chipset = "Intel(R) 965GME/GLE";
	 break;
      case PCI_CHIP_GM45_GM:
	 chipset = "Mobile Intel® GM45 Express Chipset";
	 break; 
      case PCI_CHIP_IGD_E_G:
	 chipset = "Intel(R) Integrated Graphics Device";
	 break;
      case PCI_CHIP_G45_G:
         chipset = "Intel(R) G45/G43";
         break;
      case PCI_CHIP_Q45_G:
         chipset = "Intel(R) Q45/Q43";
         break;
      case PCI_CHIP_G41_G:
         chipset = "Intel(R) G41";
         break;
      case PCI_CHIP_B43_G:
      case PCI_CHIP_B43_G1:
         chipset = "Intel(R) B43";
         break;
      case PCI_CHIP_ILD_G:
         chipset = "Intel(R) Ironlake Desktop";
         break;
      case PCI_CHIP_ILM_G:
         chipset = "Intel(R) Ironlake Mobile";
         break;
      case PCI_CHIP_SANDYBRIDGE_GT1:
      case PCI_CHIP_SANDYBRIDGE_GT2:
      case PCI_CHIP_SANDYBRIDGE_GT2_PLUS:
	 chipset = "Intel(R) Sandybridge Desktop";
	 break;
      case PCI_CHIP_SANDYBRIDGE_M_GT1:
      case PCI_CHIP_SANDYBRIDGE_M_GT2:
      case PCI_CHIP_SANDYBRIDGE_M_GT2_PLUS:
	 chipset = "Intel(R) Sandybridge Mobile";
	 break;
      case PCI_CHIP_SANDYBRIDGE_S:
	 chipset = "Intel(R) Sandybridge Server";
	 break;
      case PCI_CHIP_IVYBRIDGE_GT1:
      case PCI_CHIP_IVYBRIDGE_GT2:
	 chipset = "Intel(R) Ivybridge Desktop";
	 break;
      case PCI_CHIP_IVYBRIDGE_M_GT1:
      case PCI_CHIP_IVYBRIDGE_M_GT2:
	 chipset = "Intel(R) Ivybridge Mobile";
	 break;
      case PCI_CHIP_IVYBRIDGE_S_GT1:
      case PCI_CHIP_IVYBRIDGE_S_GT2:
	 chipset = "Intel(R) Ivybridge Server";
	 break;
      case PCI_CHIP_HASWELL_GT1:
      case PCI_CHIP_HASWELL_GT2:
      case PCI_CHIP_HASWELL_GT2_PLUS:
      case PCI_CHIP_HASWELL_SDV_GT1:
      case PCI_CHIP_HASWELL_SDV_GT2:
      case PCI_CHIP_HASWELL_SDV_GT2_PLUS:
      case PCI_CHIP_HASWELL_ULT_GT1:
      case PCI_CHIP_HASWELL_ULT_GT2:
      case PCI_CHIP_HASWELL_ULT_GT2_PLUS:
      case PCI_CHIP_HASWELL_CRW_GT1:
      case PCI_CHIP_HASWELL_CRW_GT2:
      case PCI_CHIP_HASWELL_CRW_GT2_PLUS:
	 chipset = "Intel(R) Haswell Desktop";
	 break;
      case PCI_CHIP_HASWELL_M_GT1:
      case PCI_CHIP_HASWELL_M_GT2:
      case PCI_CHIP_HASWELL_M_GT2_PLUS:
      case PCI_CHIP_HASWELL_SDV_M_GT1:
      case PCI_CHIP_HASWELL_SDV_M_GT2:
      case PCI_CHIP_HASWELL_SDV_M_GT2_PLUS:
      case PCI_CHIP_HASWELL_ULT_M_GT1:
      case PCI_CHIP_HASWELL_ULT_M_GT2:
      case PCI_CHIP_HASWELL_ULT_M_GT2_PLUS:
      case PCI_CHIP_HASWELL_CRW_M_GT1:
      case PCI_CHIP_HASWELL_CRW_M_GT2:
      case PCI_CHIP_HASWELL_CRW_M_GT2_PLUS:
	 chipset = "Intel(R) Haswell Mobile";
	 break;
      case PCI_CHIP_HASWELL_S_GT1:
      case PCI_CHIP_HASWELL_S_GT2:
      case PCI_CHIP_HASWELL_S_GT2_PLUS:
      case PCI_CHIP_HASWELL_SDV_S_GT1:
      case PCI_CHIP_HASWELL_SDV_S_GT2:
      case PCI_CHIP_HASWELL_SDV_S_GT2_PLUS:
      case PCI_CHIP_HASWELL_ULT_S_GT1:
      case PCI_CHIP_HASWELL_ULT_S_GT2:
      case PCI_CHIP_HASWELL_ULT_S_GT2_PLUS:
      case PCI_CHIP_HASWELL_CRW_S_GT1:
      case PCI_CHIP_HASWELL_CRW_S_GT2:
      case PCI_CHIP_HASWELL_CRW_S_GT2_PLUS:
	 chipset = "Intel(R) Haswell Server";
	 break;
      default:
         chipset = "Unknown Intel Chipset";
         break;
      }

      (void) driGetRendererString(buffer, chipset, 0);
      return (GLubyte *) buffer;

   default:
      return NULL;
   }
}

void
intel_downsample_for_dri2_flush(struct intel_context *intel,
                                __DRIdrawable *drawable)
{
   if (intel->gen < 6) {
      /* MSAA is not supported, so don't waste time checking for
       * a multisample buffer.
       */
      return;
   }

   struct gl_framebuffer *fb = drawable->driverPrivate;
   struct intel_renderbuffer *rb;

   /* Usually, only the back buffer will need to be downsampled. However,
    * the front buffer will also need it if the user has rendered into it.
    */
   static const gl_buffer_index buffers[2] = {
         BUFFER_BACK_LEFT,
         BUFFER_FRONT_LEFT,
   };

   for (int i = 0; i < 2; ++i) {
      rb = intel_get_renderbuffer(fb, buffers[i]);
      if (rb == NULL || rb->mt == NULL)
         continue;
      intel_miptree_downsample(intel, rb->mt);
   }
}

static void
intel_flush_front(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);
    __DRIcontext *driContext = intel->driContext;
    __DRIdrawable *driDrawable = driContext->driDrawablePriv;
    __DRIscreen *const screen = intel->intelScreen->driScrnPriv;

    if (_mesa_is_winsys_fbo(ctx->DrawBuffer) && intel->front_buffer_dirty) {
      if (screen->dri2.loader->flushFrontBuffer != NULL &&
          driDrawable &&
          driDrawable->loaderPrivate) {

         /* Downsample before flushing FAKE_FRONT_LEFT to FRONT_LEFT.
          *
          * This potentially downsamples both front and back buffer. It
          * is unnecessary to downsample the back, but harms nothing except
          * performance. And no one cares about front-buffer render
          * performance.
          */
         intel_downsample_for_dri2_flush(intel, driDrawable);

         screen->dri2.loader->flushFrontBuffer(driDrawable,
                                               driDrawable->loaderPrivate);

	 /* We set the dirty bit in intel_prepare_render() if we're
	  * front buffer rendering once we get there.
	  */
	 intel->front_buffer_dirty = false;
      }
   }
}

static unsigned
intel_bits_per_pixel(const struct intel_renderbuffer *rb)
{
   return _mesa_get_format_bytes(intel_rb_format(rb)) * 8;
}

static void
intel_query_dri2_buffers(struct intel_context *intel,
			 __DRIdrawable *drawable,
			 __DRIbuffer **buffers,
			 int *count);

static void
intel_process_dri2_buffer(struct intel_context *intel,
			  __DRIdrawable *drawable,
			  __DRIbuffer *buffer,
			  struct intel_renderbuffer *rb,
			  const char *buffer_name);

void
intel_update_renderbuffers(__DRIcontext *context, __DRIdrawable *drawable)
{
   struct gl_framebuffer *fb = drawable->driverPrivate;
   struct intel_renderbuffer *rb;
   struct intel_context *intel = context->driverPrivate;
   __DRIbuffer *buffers = NULL;
   int i, count;
   const char *region_name;

   /* If we're rendering to the fake front buffer, make sure all the
    * pending drawing has landed on the real front buffer.  Otherwise
    * when we eventually get to DRI2GetBuffersWithFormat the stale
    * real front buffer contents will get copied to the new fake front
    * buffer.
    */
   if (intel->is_front_buffer_rendering) {
      intel_flush(&intel->ctx);
      intel_flush_front(&intel->ctx);
   }

   /* Set this up front, so that in case our buffers get invalidated
    * while we're getting new buffers, we don't clobber the stamp and
    * thus ignore the invalidate. */
   drawable->lastStamp = drawable->dri2.stamp;

   if (unlikely(INTEL_DEBUG & DEBUG_DRI))
      fprintf(stderr, "enter %s, drawable %p\n", __func__, drawable);

   intel_query_dri2_buffers(intel, drawable, &buffers, &count);

   if (buffers == NULL)
      return;

   for (i = 0; i < count; i++) {
       switch (buffers[i].attachment) {
       case __DRI_BUFFER_FRONT_LEFT:
	   rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
	   region_name = "dri2 front buffer";
	   break;

       case __DRI_BUFFER_FAKE_FRONT_LEFT:
	   rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
	   region_name = "dri2 fake front buffer";
	   break;

       case __DRI_BUFFER_BACK_LEFT:
	   rb = intel_get_renderbuffer(fb, BUFFER_BACK_LEFT);
	   region_name = "dri2 back buffer";
	   break;

       case __DRI_BUFFER_DEPTH:
       case __DRI_BUFFER_HIZ:
       case __DRI_BUFFER_DEPTH_STENCIL:
       case __DRI_BUFFER_STENCIL:
       case __DRI_BUFFER_ACCUM:
       default:
	   fprintf(stderr,
		   "unhandled buffer attach event, attachment type %d\n",
		   buffers[i].attachment);
	   return;
       }

       intel_process_dri2_buffer(intel, drawable, &buffers[i], rb, region_name);
   }

   driUpdateFramebufferSize(&intel->ctx, drawable);
}

/**
 * intel_prepare_render should be called anywhere that curent read/drawbuffer
 * state is required.
 */
void
intel_prepare_render(struct intel_context *intel)
{
   __DRIcontext *driContext = intel->driContext;
   __DRIdrawable *drawable;

   drawable = driContext->driDrawablePriv;
   if (drawable && drawable->dri2.stamp != driContext->dri2.draw_stamp) {
      if (drawable->lastStamp != drawable->dri2.stamp)
	 intel_update_renderbuffers(driContext, drawable);
      intel_draw_buffer(&intel->ctx);
      driContext->dri2.draw_stamp = drawable->dri2.stamp;
   }

   drawable = driContext->driReadablePriv;
   if (drawable && drawable->dri2.stamp != driContext->dri2.read_stamp) {
      if (drawable->lastStamp != drawable->dri2.stamp)
	 intel_update_renderbuffers(driContext, drawable);
      driContext->dri2.read_stamp = drawable->dri2.stamp;
   }

   /* If we're currently rendering to the front buffer, the rendering
    * that will happen next will probably dirty the front buffer.  So
    * mark it as dirty here.
    */
   if (intel->is_front_buffer_rendering)
      intel->front_buffer_dirty = true;

   /* Wait for the swapbuffers before the one we just emitted, so we
    * don't get too many swaps outstanding for apps that are GPU-heavy
    * but not CPU-heavy.
    *
    * We're using intelDRI2Flush (called from the loader before
    * swapbuffer) and glFlush (for front buffer rendering) as the
    * indicator that a frame is done and then throttle when we get
    * here as we prepare to render the next frame.  At this point for
    * round trips for swap/copy and getting new buffers are done and
    * we'll spend less time waiting on the GPU.
    *
    * Unfortunately, we don't have a handle to the batch containing
    * the swap, and getting our hands on that doesn't seem worth it,
    * so we just us the first batch we emitted after the last swap.
    */
   if (intel->need_throttle && intel->first_post_swapbuffers_batch) {
      drm_intel_bo_wait_rendering(intel->first_post_swapbuffers_batch);
      drm_intel_bo_unreference(intel->first_post_swapbuffers_batch);
      intel->first_post_swapbuffers_batch = NULL;
      intel->need_throttle = false;
   }
}

static void
intel_viewport(struct gl_context *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
    struct intel_context *intel = intel_context(ctx);
    __DRIcontext *driContext = intel->driContext;

    if (intel->saved_viewport)
	intel->saved_viewport(ctx, x, y, w, h);

    if (_mesa_is_winsys_fbo(ctx->DrawBuffer)) {
       dri2InvalidateDrawable(driContext->driDrawablePriv);
       dri2InvalidateDrawable(driContext->driReadablePriv);
    }
}

static const struct dri_debug_control debug_control[] = {
   { "tex",   DEBUG_TEXTURE},
   { "state", DEBUG_STATE},
   { "ioctl", DEBUG_IOCTL},
   { "blit",  DEBUG_BLIT},
   { "mip",   DEBUG_MIPTREE},
   { "fall",  DEBUG_PERF},
   { "perf",  DEBUG_PERF},
   { "verb",  DEBUG_VERBOSE},
   { "bat",   DEBUG_BATCH},
   { "pix",   DEBUG_PIXEL},
   { "buf",   DEBUG_BUFMGR},
   { "reg",   DEBUG_REGION},
   { "fbo",   DEBUG_FBO},
   { "gs",    DEBUG_GS},
   { "sync",  DEBUG_SYNC},
   { "prim",  DEBUG_PRIMS },
   { "vert",  DEBUG_VERTS },
   { "dri",   DEBUG_DRI },
   { "sf",    DEBUG_SF },
   { "san",   DEBUG_SANITY },
   { "sleep", DEBUG_SLEEP },
   { "stats", DEBUG_STATS },
   { "tile",  DEBUG_TILE },
   { "wm",    DEBUG_WM },
   { "urb",   DEBUG_URB },
   { "vs",    DEBUG_VS },
   { "clip",  DEBUG_CLIP },
   { "aub",   DEBUG_AUB },
   { NULL,    0 }
};


static void
intelInvalidateState(struct gl_context * ctx, GLuint new_state)
{
    struct intel_context *intel = intel_context(ctx);

    if (ctx->swrast_context)
       _swrast_InvalidateState(ctx, new_state);
   _vbo_InvalidateState(ctx, new_state);

   intel->NewGLState |= new_state;

   if (intel->vtbl.invalidate_state)
      intel->vtbl.invalidate_state( intel, new_state );
}

void
intel_flush_rendering_to_batch(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);

   if (intel->Fallback)
      _swrast_flush(ctx);

   if (intel->gen < 4)
      INTEL_FIREVERTICES(intel);
}

void
_intel_flush(struct gl_context *ctx, const char *file, int line)
{
   struct intel_context *intel = intel_context(ctx);

   intel_flush_rendering_to_batch(ctx);

   if (intel->batch.used)
      _intel_batchbuffer_flush(intel, file, line);
}

static void
intel_glFlush(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);

   intel_flush(ctx);
   intel_flush_front(ctx);
   if (intel->is_front_buffer_rendering)
      intel->need_throttle = true;
}

void
intelFinish(struct gl_context * ctx)
{
   struct intel_context *intel = intel_context(ctx);

   intel_flush(ctx);
   intel_flush_front(ctx);

   if (intel->batch.last_bo)
      drm_intel_bo_wait_rendering(intel->batch.last_bo);
}

void
intelInitDriverFunctions(struct dd_function_table *functions)
{
   _mesa_init_driver_functions(functions);

   functions->Flush = intel_glFlush;
   functions->Finish = intelFinish;
   functions->GetString = intelGetString;
   functions->UpdateState = intelInvalidateState;

   intelInitTextureFuncs(functions);
   intelInitTextureImageFuncs(functions);
   intelInitTextureSubImageFuncs(functions);
   intelInitTextureCopyImageFuncs(functions);
   intelInitClearFuncs(functions);
   intelInitBufferFuncs(functions);
   intelInitPixelFuncs(functions);
   intelInitBufferObjectFuncs(functions);
   intel_init_syncobj_functions(functions);
}

bool
intelInitContext(struct intel_context *intel,
		 int api,
                 const struct gl_config * mesaVis,
                 __DRIcontext * driContextPriv,
                 void *sharedContextPrivate,
                 struct dd_function_table *functions)
{
   struct gl_context *ctx = &intel->ctx;
   struct gl_context *shareCtx = (struct gl_context *) sharedContextPrivate;
   __DRIscreen *sPriv = driContextPriv->driScreenPriv;
   struct intel_screen *intelScreen = sPriv->driverPrivate;
   int bo_reuse_mode;
   struct gl_config visual;

   /* we can't do anything without a connection to the device */
   if (intelScreen->bufmgr == NULL)
      return false;

   /* Can't rely on invalidate events, fall back to glViewport hack */
   if (!driContextPriv->driScreenPriv->dri2.useInvalidate) {
      intel->saved_viewport = functions->Viewport;
      functions->Viewport = intel_viewport;
   }

   if (mesaVis == NULL) {
      memset(&visual, 0, sizeof visual);
      mesaVis = &visual;
   }

   if (!_mesa_initialize_context(&intel->ctx, api, mesaVis, shareCtx,
                                 functions, (void *) intel)) {
      printf("%s: failed to init mesa context\n", __FUNCTION__);
      return false;
   }

   driContextPriv->driverPrivate = intel;
   intel->intelScreen = intelScreen;
   intel->driContext = driContextPriv;
   intel->driFd = sPriv->fd;

   intel->gen = intelScreen->gen;

   const int devID = intelScreen->deviceID;
   if (IS_SNB_GT1(devID) || IS_IVB_GT1(devID) || IS_HSW_GT1(devID))
      intel->gt = 1;
   else if (IS_SNB_GT2(devID) || IS_IVB_GT2(devID) || IS_HSW_GT2(devID))
      intel->gt = 2;
   else
      intel->gt = 0;

   if (IS_HASWELL(devID)) {
      intel->is_haswell = true;
   } else if (IS_G4X(devID)) {
      intel->is_g4x = true;
   } else if (IS_945(devID)) {
      intel->is_945 = true;
   }

   if (intel->gen >= 5) {
      intel->needs_ff_sync = true;
   }

   intel->has_separate_stencil = intel->intelScreen->hw_has_separate_stencil;
   intel->must_use_separate_stencil = intel->intelScreen->hw_must_use_separate_stencil;
   intel->has_hiz = intel->gen >= 6 && !intel->is_haswell;
   intel->has_llc = intel->intelScreen->hw_has_llc;
   intel->has_swizzling = intel->intelScreen->hw_has_swizzling;

   memset(&ctx->TextureFormatSupported,
	  0, sizeof(ctx->TextureFormatSupported));

   driParseConfigFiles(&intel->optionCache, &intelScreen->optionCache,
                       sPriv->myNum, (intel->gen >= 4) ? "i965" : "i915");
   if (intel->gen < 4)
      intel->maxBatchSize = 4096;
   else
      intel->maxBatchSize = sizeof(intel->batch.map);

   intel->bufmgr = intelScreen->bufmgr;

   bo_reuse_mode = driQueryOptioni(&intel->optionCache, "bo_reuse");
   switch (bo_reuse_mode) {
   case DRI_CONF_BO_REUSE_DISABLED:
      break;
   case DRI_CONF_BO_REUSE_ALL:
      intel_bufmgr_gem_enable_reuse(intel->bufmgr);
      break;
   }

   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 5.0;
   ctx->Const.MaxLineWidthAA = 5.0;
   ctx->Const.LineWidthGranularity = 0.5;

   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSize = 255.0;
   ctx->Const.MaxPointSizeAA = 3.0;
   ctx->Const.PointSizeGranularity = 1.0;

   ctx->Const.MaxSamples = 1.0;

   if (intel->gen >= 6)
      ctx->Const.MaxClipPlanes = 8;

   ctx->Const.StripTextureBorder = GL_TRUE;

   /* reinitialize the context point state.
    * It depend on constants in __struct gl_contextRec::Const
    */
   _mesa_init_point(ctx);

   if (intel->gen >= 4) {
      ctx->Const.MaxRenderbufferSize = 8192;
   } else {
      ctx->Const.MaxRenderbufferSize = 2048;
   }

   /* Initialize the software rasterizer and helper modules.
    *
    * As of GL 3.1 core, the gen4+ driver doesn't need the swrast context for
    * software fallbacks (which we have to support on legacy GL to do weird
    * glDrawPixels(), glBitmap(), and other functions).
    */
   if (intel->gen <= 3 || api != API_OPENGL_CORE) {
      _swrast_CreateContext(ctx);
   }

   _vbo_CreateContext(ctx);
   if (ctx->swrast_context) {
      _tnl_CreateContext(ctx);
      _swsetup_CreateContext(ctx);

      /* Configure swrast to match hardware characteristics: */
      _swrast_allow_pixel_fog(ctx, false);
      _swrast_allow_vertex_fog(ctx, true);
   }

   _mesa_meta_init(ctx);

   intel->hw_stencil = mesaVis->stencilBits && mesaVis->depthBits == 24;
   intel->hw_stipple = 1;

   /* XXX FBO: this doesn't seem to be used anywhere */
   switch (mesaVis->depthBits) {
   case 0:                     /* what to do in this case? */
   case 16:
      intel->polygon_offset_scale = 1.0;
      break;
   case 24:
      intel->polygon_offset_scale = 2.0;     /* req'd to pass glean */
      break;
   default:
      assert(0);
      break;
   }

   if (intel->gen >= 4)
      intel->polygon_offset_scale /= 0xffff;

   intel->RenderIndex = ~0;

   intelInitExtensions(ctx);

   INTEL_DEBUG = driParseDebugString(getenv("INTEL_DEBUG"), debug_control);
   if (INTEL_DEBUG & DEBUG_BUFMGR)
      dri_bufmgr_set_debug(intel->bufmgr, true);

   if (INTEL_DEBUG & DEBUG_AUB)
      drm_intel_bufmgr_gem_set_aub_dump(intel->bufmgr, true);

   intel_batchbuffer_init(intel);

   intel_fbo_init(intel);

   intel->use_texture_tiling = driQueryOptionb(&intel->optionCache,
					       "texture_tiling");
   intel->use_early_z = driQueryOptionb(&intel->optionCache, "early_z");

   if (!driQueryOptionb(&intel->optionCache, "hiz")) {
       intel->has_hiz = false;
       /* On gen6, you can only do separate stencil with HIZ. */
       if (intel->gen == 6)
	  intel->has_separate_stencil = false;
   }

   intel->prim.primitive = ~0;

   /* Force all software fallbacks */
#ifdef I915
   if (driQueryOptionb(&intel->optionCache, "no_rast")) {
      fprintf(stderr, "disabling 3D rasterization\n");
      intel->no_rast = 1;
   }
#endif

   if (driQueryOptionb(&intel->optionCache, "always_flush_batch")) {
      fprintf(stderr, "flushing batchbuffer before/after each draw call\n");
      intel->always_flush_batch = 1;
   }

   if (driQueryOptionb(&intel->optionCache, "always_flush_cache")) {
      fprintf(stderr, "flushing GPU caches before/after each draw call\n");
      intel->always_flush_cache = 1;
   }

   return true;
}

void
intelDestroyContext(__DRIcontext * driContextPriv)
{
   struct intel_context *intel =
      (struct intel_context *) driContextPriv->driverPrivate;
   struct gl_context *ctx = &intel->ctx;

   assert(intel);               /* should never be null */
   if (intel) {
      INTEL_FIREVERTICES(intel);

      /* Dump a final BMP in case the application doesn't call SwapBuffers */
      if (INTEL_DEBUG & DEBUG_AUB) {
         intel_batchbuffer_flush(intel);
	 aub_dump_bmp(&intel->ctx);
      }

      _mesa_meta_free(&intel->ctx);

      intel->vtbl.destroy(intel);

      if (ctx->swrast_context) {
         _swsetup_DestroyContext(&intel->ctx);
         _tnl_DestroyContext(&intel->ctx);
      }
      _vbo_DestroyContext(&intel->ctx);

      if (ctx->swrast_context)
         _swrast_DestroyContext(&intel->ctx);
      intel->Fallback = 0x0;      /* don't call _swrast_Flush later */

      intel_batchbuffer_free(intel);

      free(intel->prim.vb);
      intel->prim.vb = NULL;
      drm_intel_bo_unreference(intel->prim.vb_bo);
      intel->prim.vb_bo = NULL;
      drm_intel_bo_unreference(intel->first_post_swapbuffers_batch);
      intel->first_post_swapbuffers_batch = NULL;

      driDestroyOptionCache(&intel->optionCache);

      /* free the Mesa context */
      _mesa_free_context_data(&intel->ctx);

      _math_matrix_dtr(&intel->ViewportMatrix);

      ralloc_free(intel);
      driContextPriv->driverPrivate = NULL;
   }
}

GLboolean
intelUnbindContext(__DRIcontext * driContextPriv)
{
   /* Unset current context and dispath table */
   _mesa_make_current(NULL, NULL, NULL);

   return true;
}

GLboolean
intelMakeCurrent(__DRIcontext * driContextPriv,
                 __DRIdrawable * driDrawPriv,
                 __DRIdrawable * driReadPriv)
{
   struct intel_context *intel;
   GET_CURRENT_CONTEXT(curCtx);

   if (driContextPriv)
      intel = (struct intel_context *) driContextPriv->driverPrivate;
   else
      intel = NULL;

   /* According to the glXMakeCurrent() man page: "Pending commands to
    * the previous context, if any, are flushed before it is released."
    * But only flush if we're actually changing contexts.
    */
   if (intel_context(curCtx) && intel_context(curCtx) != intel) {
      _mesa_flush(curCtx);
   }

   if (driContextPriv) {
      struct gl_framebuffer *fb, *readFb;
      
      if (driDrawPriv == NULL && driReadPriv == NULL) {
	 fb = _mesa_get_incomplete_framebuffer();
	 readFb = _mesa_get_incomplete_framebuffer();
      } else {
	 fb = driDrawPriv->driverPrivate;
	 readFb = driReadPriv->driverPrivate;
	 driContextPriv->dri2.draw_stamp = driDrawPriv->dri2.stamp - 1;
	 driContextPriv->dri2.read_stamp = driReadPriv->dri2.stamp - 1;
      }

      intel_prepare_render(intel);
      _mesa_make_current(&intel->ctx, fb, readFb);
      
      /* We do this in intel_prepare_render() too, but intel->ctx.DrawBuffer
       * is NULL at that point.  We can't call _mesa_makecurrent()
       * first, since we need the buffer size for the initial
       * viewport.  So just call intel_draw_buffer() again here. */
      intel_draw_buffer(&intel->ctx);
   }
   else {
      _mesa_make_current(NULL, NULL, NULL);
   }

   return true;
}

/**
 * \brief Query DRI2 to obtain a DRIdrawable's buffers.
 *
 * To determine which DRI buffers to request, examine the renderbuffers
 * attached to the drawable's framebuffer. Then request the buffers with
 * DRI2GetBuffers() or DRI2GetBuffersWithFormat().
 *
 * This is called from intel_update_renderbuffers().
 *
 * \param drawable      Drawable whose buffers are queried.
 * \param buffers       [out] List of buffers returned by DRI2 query.
 * \param buffer_count  [out] Number of buffers returned.
 *
 * \see intel_update_renderbuffers()
 * \see DRI2GetBuffers()
 * \see DRI2GetBuffersWithFormat()
 */
static void
intel_query_dri2_buffers(struct intel_context *intel,
			 __DRIdrawable *drawable,
			 __DRIbuffer **buffers,
			 int *buffer_count)
{
   __DRIscreen *screen = intel->intelScreen->driScrnPriv;
   struct gl_framebuffer *fb = drawable->driverPrivate;
   int i = 0;
   const int max_attachments = 4;
   unsigned *attachments = calloc(2 * max_attachments, sizeof(unsigned));

   struct intel_renderbuffer *front_rb;
   struct intel_renderbuffer *back_rb;

   front_rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
   back_rb = intel_get_renderbuffer(fb, BUFFER_BACK_LEFT);

   if ((intel->is_front_buffer_rendering ||
	intel->is_front_buffer_reading ||
	!back_rb) && front_rb) {
      attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
      attachments[i++] = intel_bits_per_pixel(front_rb);
   }

   if (back_rb) {
      attachments[i++] = __DRI_BUFFER_BACK_LEFT;
      attachments[i++] = intel_bits_per_pixel(back_rb);
   }

   assert(i <= 2 * max_attachments);

   *buffers = screen->dri2.loader->getBuffersWithFormat(drawable,
							&drawable->w,
							&drawable->h,
							attachments, i / 2,
							buffer_count,
							drawable->loaderPrivate);
   free(attachments);
}

/**
 * \brief Assign a DRI buffer's DRM region to a renderbuffer.
 *
 * This is called from intel_update_renderbuffers().
 *
 * \par Note:
 *    DRI buffers whose attachment point is DRI2BufferStencil or
 *    DRI2BufferDepthStencil are handled as special cases.
 *
 * \param buffer_name is a human readable name, such as "dri2 front buffer",
 *        that is passed to intel_region_alloc_for_handle().
 *
 * \see intel_update_renderbuffers()
 * \see intel_region_alloc_for_handle()
 */
static void
intel_process_dri2_buffer(struct intel_context *intel,
			  __DRIdrawable *drawable,
			  __DRIbuffer *buffer,
			  struct intel_renderbuffer *rb,
			  const char *buffer_name)
{
   struct intel_region *region = NULL;

   if (!rb)
      return;

   unsigned num_samples = rb->Base.Base.NumSamples;

   /* We try to avoid closing and reopening the same BO name, because the first
    * use of a mapping of the buffer involves a bunch of page faulting which is
    * moderately expensive.
    */
   if (num_samples == 0) {
       if (rb->mt &&
           rb->mt->region &&
           rb->mt->region->name == buffer->name)
          return;
   } else {
       if (rb->mt &&
           rb->mt->singlesample_mt &&
           rb->mt->singlesample_mt->region &&
           rb->mt->singlesample_mt->region->name == buffer->name)
          return;
   }

   if (unlikely(INTEL_DEBUG & DEBUG_DRI)) {
      fprintf(stderr,
	      "attaching buffer %d, at %d, cpp %d, pitch %d\n",
	      buffer->name, buffer->attachment,
	      buffer->cpp, buffer->pitch);
   }

   intel_miptree_release(&rb->mt);
   region = intel_region_alloc_for_handle(intel->intelScreen,
                                          buffer->cpp,
                                          drawable->w,
                                          drawable->h,
                                          buffer->pitch / buffer->cpp,
                                          buffer->name,
                                          buffer_name);
   if (!region)
      return;

   rb->mt = intel_miptree_create_for_dri2_buffer(intel,
                                                 buffer->attachment,
                                                 intel_rb_format(rb),
                                                 num_samples,
                                                 region);
   intel_region_release(&region);
}
