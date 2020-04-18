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

#include <errno.h>
#include <time.h>
#include "main/glheader.h"
#include "main/context.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/hash.h"
#include "main/fbobject.h"
#include "main/mfeatures.h"
#include "main/version.h"
#include "swrast/s_renderbuffer.h"

#include "utils.h"
#include "xmlpool.h"

PUBLIC const char __driConfigOptions[] =
   DRI_CONF_BEGIN
   DRI_CONF_SECTION_PERFORMANCE
      DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_ALWAYS_SYNC)
      /* Options correspond to DRI_CONF_BO_REUSE_DISABLED,
       * DRI_CONF_BO_REUSE_ALL
       */
      DRI_CONF_OPT_BEGIN_V(bo_reuse, enum, 1, "0:1")
	 DRI_CONF_DESC_BEGIN(en, "Buffer object reuse")
	    DRI_CONF_ENUM(0, "Disable buffer object reuse")
	    DRI_CONF_ENUM(1, "Enable reuse of all sizes of buffer objects")
	 DRI_CONF_DESC_END
      DRI_CONF_OPT_END

      DRI_CONF_OPT_BEGIN(texture_tiling, bool, true)
	 DRI_CONF_DESC(en, "Enable texture tiling")
      DRI_CONF_OPT_END

      DRI_CONF_OPT_BEGIN(hiz, bool, true)
	 DRI_CONF_DESC(en, "Enable Hierarchical Z on gen6+")
      DRI_CONF_OPT_END

      DRI_CONF_OPT_BEGIN(early_z, bool, false)
	 DRI_CONF_DESC(en, "Enable early Z in classic mode (unstable, 945-only).")
      DRI_CONF_OPT_END

      DRI_CONF_OPT_BEGIN(fragment_shader, bool, true)
	 DRI_CONF_DESC(en, "Enable limited ARB_fragment_shader support on 915/945.")
      DRI_CONF_OPT_END

   DRI_CONF_SECTION_END
   DRI_CONF_SECTION_QUALITY
      DRI_CONF_FORCE_S3TC_ENABLE(false)
      DRI_CONF_ALLOW_LARGE_TEXTURES(2)
   DRI_CONF_SECTION_END
   DRI_CONF_SECTION_DEBUG
     DRI_CONF_NO_RAST(false)
     DRI_CONF_ALWAYS_FLUSH_BATCH(false)
     DRI_CONF_ALWAYS_FLUSH_CACHE(false)
     DRI_CONF_FORCE_GLSL_EXTENSIONS_WARN(false)
     DRI_CONF_DISABLE_BLEND_FUNC_EXTENDED(false)

      DRI_CONF_OPT_BEGIN(stub_occlusion_query, bool, false)
	 DRI_CONF_DESC(en, "Enable stub ARB_occlusion_query support on 915/945.")
      DRI_CONF_OPT_END

      DRI_CONF_OPT_BEGIN(shader_precompile, bool, true)
	 DRI_CONF_DESC(en, "Perform code generation at shader link time.")
      DRI_CONF_OPT_END
   DRI_CONF_SECTION_END
DRI_CONF_END;

const GLuint __driNConfigOptions = 15;

#include "intel_batchbuffer.h"
#include "intel_buffers.h"
#include "intel_bufmgr.h"
#include "intel_chipset.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_screen.h"
#include "intel_tex.h"
#include "intel_regions.h"

#include "i915_drm.h"

#ifdef USE_NEW_INTERFACE
static PFNGLXCREATECONTEXTMODES create_context_modes = NULL;
#endif /*USE_NEW_INTERFACE */

/**
 * For debugging purposes, this returns a time in seconds.
 */
double
get_time(void)
{
   struct timespec tp;

   clock_gettime(CLOCK_MONOTONIC, &tp);

   return tp.tv_sec + tp.tv_nsec / 1000000000.0;
}

void
aub_dump_bmp(struct gl_context *ctx)
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;

   for (int i = 0; i < fb->_NumColorDrawBuffers; i++) {
      struct intel_renderbuffer *irb =
	 intel_renderbuffer(fb->_ColorDrawBuffers[i]);

      if (irb && irb->mt) {
	 enum aub_dump_bmp_format format;

	 switch (irb->Base.Base.Format) {
	 case MESA_FORMAT_ARGB8888:
	 case MESA_FORMAT_XRGB8888:
	    format = AUB_DUMP_BMP_FORMAT_ARGB_8888;
	    break;
	 default:
	    continue;
	 }

	 drm_intel_gem_bo_aub_dump_bmp(irb->mt->region->bo,
				       irb->draw_x,
				       irb->draw_y,
				       irb->Base.Base.Width,
				       irb->Base.Base.Height,
				       format,
				       irb->mt->region->pitch *
				       irb->mt->region->cpp,
				       0);
      }
   }
}

static const __DRItexBufferExtension intelTexBufferExtension = {
    { __DRI_TEX_BUFFER, __DRI_TEX_BUFFER_VERSION },
   intelSetTexBuffer,
   intelSetTexBuffer2,
};

static void
intelDRI2Flush(__DRIdrawable *drawable)
{
   GET_CURRENT_CONTEXT(ctx);
   struct intel_context *intel = intel_context(ctx);
   if (intel == NULL)
      return;

   if (intel->gen < 4)
      INTEL_FIREVERTICES(intel);

   intel_downsample_for_dri2_flush(intel, drawable);
   intel->need_throttle = true;

   if (intel->batch.used)
      intel_batchbuffer_flush(intel);

   if (INTEL_DEBUG & DEBUG_AUB) {
      aub_dump_bmp(ctx);
   }
}

static const struct __DRI2flushExtensionRec intelFlushExtension = {
    { __DRI2_FLUSH, __DRI2_FLUSH_VERSION },
    intelDRI2Flush,
    dri2InvalidateDrawable,
};

struct intel_image_format intel_image_formats[] = {
   { __DRI_IMAGE_FOURCC_ARGB8888, __DRI_IMAGE_COMPONENTS_RGBA, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_ARGB8888, 4 } } },

   { __DRI_IMAGE_FOURCC_XRGB8888, __DRI_IMAGE_COMPONENTS_RGB, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_XRGB8888, 4 }, } },

   { __DRI_IMAGE_FOURCC_YUV410, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 2, 2, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 2, 2, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { __DRI_IMAGE_FOURCC_YUV411, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 2, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 2, 0, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { __DRI_IMAGE_FOURCC_YUV420, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 1, 1, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 1, 1, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { __DRI_IMAGE_FOURCC_YUV422, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 1, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 1, 0, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { __DRI_IMAGE_FOURCC_YUV444, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { __DRI_IMAGE_FOURCC_NV12, __DRI_IMAGE_COMPONENTS_Y_UV, 2,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 1, 1, __DRI_IMAGE_FORMAT_GR88, 2 } } },

   { __DRI_IMAGE_FOURCC_NV16, __DRI_IMAGE_COMPONENTS_Y_UV, 2,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 1, 0, __DRI_IMAGE_FORMAT_GR88, 2 } } },

   /* For YUYV buffers, we set up two overlapping DRI images and treat
    * them as planar buffers in the compositors.  Plane 0 is GR88 and
    * samples YU or YV pairs and places Y into the R component, while
    * plane 1 is ARGB and samples YUYV clusters and places pairs and
    * places U into the G component and V into A.  This lets the
    * texture sampler interpolate the Y components correctly when
    * sampling from plane 0, and interpolate U and V correctly when
    * sampling from plane 1. */
   { __DRI_IMAGE_FOURCC_YUYV, __DRI_IMAGE_COMPONENTS_Y_XUXV, 2,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_GR88, 2 },
       { 0, 1, 0, __DRI_IMAGE_FORMAT_ARGB8888, 4 } } }
};

static __DRIimage *
intel_allocate_image(int dri_format, void *loaderPrivate)
{
    __DRIimage *image;

    image = CALLOC(sizeof *image);
    if (image == NULL)
	return NULL;

    image->dri_format = dri_format;
    image->offset = 0;

    switch (dri_format) {
    case __DRI_IMAGE_FORMAT_RGB565:
       image->format = MESA_FORMAT_RGB565;
       break;
    case __DRI_IMAGE_FORMAT_XRGB8888:
       image->format = MESA_FORMAT_XRGB8888;
       break;
    case __DRI_IMAGE_FORMAT_ARGB8888:
       image->format = MESA_FORMAT_ARGB8888;
       break;
    case __DRI_IMAGE_FORMAT_ABGR8888:
       image->format = MESA_FORMAT_RGBA8888_REV;
       break;
    case __DRI_IMAGE_FORMAT_XBGR8888:
       image->format = MESA_FORMAT_RGBX8888_REV;
       break;
    case __DRI_IMAGE_FORMAT_R8:
       image->format = MESA_FORMAT_R8;
       break;
    case __DRI_IMAGE_FORMAT_GR88:
       image->format = MESA_FORMAT_GR88;
       break;
    case __DRI_IMAGE_FORMAT_NONE:
       image->format = MESA_FORMAT_NONE;
       break;
    default:
       free(image);
       return NULL;
    }

    image->internal_format = _mesa_get_format_base_format(image->format);
    image->data = loaderPrivate;

    return image;
}

static __DRIimage *
intel_create_image_from_name(__DRIscreen *screen,
			     int width, int height, int format,
			     int name, int pitch, void *loaderPrivate)
{
    struct intel_screen *intelScreen = screen->driverPrivate;
    __DRIimage *image;
    int cpp;

    image = intel_allocate_image(format, loaderPrivate);
    if (image->format == MESA_FORMAT_NONE)
       cpp = 1;
    else
       cpp = _mesa_get_format_bytes(image->format);
    image->region = intel_region_alloc_for_handle(intelScreen,
						  cpp, width, height,
						  pitch, name, "image");
    if (image->region == NULL) {
       FREE(image);
       return NULL;
    }

    return image;	
}

static __DRIimage *
intel_create_image_from_renderbuffer(__DRIcontext *context,
				     int renderbuffer, void *loaderPrivate)
{
   __DRIimage *image;
   struct intel_context *intel = context->driverPrivate;
   struct gl_renderbuffer *rb;
   struct intel_renderbuffer *irb;

   rb = _mesa_lookup_renderbuffer(&intel->ctx, renderbuffer);
   if (!rb) {
      _mesa_error(&intel->ctx,
		  GL_INVALID_OPERATION, "glRenderbufferExternalMESA");
      return NULL;
   }

   irb = intel_renderbuffer(rb);
   image = CALLOC(sizeof *image);
   if (image == NULL)
      return NULL;

   image->internal_format = rb->InternalFormat;
   image->format = rb->Format;
   image->offset = 0;
   image->data = loaderPrivate;
   intel_region_reference(&image->region, irb->mt->region);

   switch (image->format) {
   case MESA_FORMAT_RGB565:
      image->dri_format = __DRI_IMAGE_FORMAT_RGB565;
      break;
   case MESA_FORMAT_XRGB8888:
      image->dri_format = __DRI_IMAGE_FORMAT_XRGB8888;
      break;
   case MESA_FORMAT_ARGB8888:
      image->dri_format = __DRI_IMAGE_FORMAT_ARGB8888;
      break;
   case MESA_FORMAT_RGBA8888_REV:
      image->dri_format = __DRI_IMAGE_FORMAT_ABGR8888;
      break;
   case MESA_FORMAT_R8:
      image->dri_format = __DRI_IMAGE_FORMAT_R8;
      break;
   case MESA_FORMAT_RG88:
      image->dri_format = __DRI_IMAGE_FORMAT_GR88;
      break;
   }

   return image;
}

static void
intel_destroy_image(__DRIimage *image)
{
    intel_region_release(&image->region);
    FREE(image);
}

static __DRIimage *
intel_create_image(__DRIscreen *screen,
		   int width, int height, int format,
		   unsigned int use,
		   void *loaderPrivate)
{
   __DRIimage *image;
   struct intel_screen *intelScreen = screen->driverPrivate;
   uint32_t tiling;
   int cpp;

   tiling = I915_TILING_X;
   if (use & __DRI_IMAGE_USE_CURSOR) {
      if (width != 64 || height != 64)
	 return NULL;
      tiling = I915_TILING_NONE;
   }

   image = intel_allocate_image(format, loaderPrivate);
   cpp = _mesa_get_format_bytes(image->format);
   image->region =
      intel_region_alloc(intelScreen, tiling, cpp, width, height, true);
   if (image->region == NULL) {
      FREE(image);
      return NULL;
   }
   
   return image;
}

static GLboolean
intel_query_image(__DRIimage *image, int attrib, int *value)
{
   switch (attrib) {
   case __DRI_IMAGE_ATTRIB_STRIDE:
      *value = image->region->pitch * image->region->cpp;
      return true;
   case __DRI_IMAGE_ATTRIB_HANDLE:
      *value = image->region->bo->handle;
      return true;
   case __DRI_IMAGE_ATTRIB_NAME:
      return intel_region_flink(image->region, (uint32_t *) value);
   case __DRI_IMAGE_ATTRIB_FORMAT:
      *value = image->dri_format;
      return true;
   case __DRI_IMAGE_ATTRIB_WIDTH:
      *value = image->region->width;
      return true;
   case __DRI_IMAGE_ATTRIB_HEIGHT:
      *value = image->region->height;
      return true;
   case __DRI_IMAGE_ATTRIB_COMPONENTS:
      if (image->planar_format == NULL)
         return false;
      *value = image->planar_format->components;
      return true;
  default:
      return false;
   }
}

static __DRIimage *
intel_dup_image(__DRIimage *orig_image, void *loaderPrivate)
{
   __DRIimage *image;

   image = CALLOC(sizeof *image);
   if (image == NULL)
      return NULL;

   intel_region_reference(&image->region, orig_image->region);
   if (image->region == NULL) {
      FREE(image);
      return NULL;
   }

   image->internal_format = orig_image->internal_format;
   image->planar_format   = orig_image->planar_format;
   image->dri_format      = orig_image->dri_format;
   image->format          = orig_image->format;
   image->offset          = orig_image->offset;
   image->data            = loaderPrivate;

   memcpy(image->strides, orig_image->strides, sizeof(image->strides));
   memcpy(image->offsets, orig_image->offsets, sizeof(image->offsets));

   return image;
}

static GLboolean
intel_validate_usage(__DRIimage *image, unsigned int use)
{
   if (use & __DRI_IMAGE_USE_CURSOR) {
      if (image->region->width != 64 || image->region->height != 64)
	 return GL_FALSE;
   }

   return GL_TRUE;
}

static __DRIimage *
intel_create_image_from_names(__DRIscreen *screen,
                              int width, int height, int fourcc,
                              int *names, int num_names,
                              int *strides, int *offsets,
                              void *loaderPrivate)
{
    struct intel_image_format *f = NULL;
    __DRIimage *image;
    int i, index;

    if (screen == NULL || names == NULL || num_names != 1)
        return NULL;

    for (i = 0; i < ARRAY_SIZE(intel_image_formats); i++) {
        if (intel_image_formats[i].fourcc == fourcc) {
           f = &intel_image_formats[i];
        }
    }

    if (f == NULL)
        return NULL;

    image = intel_create_image_from_name(screen, width, height,
                                         __DRI_IMAGE_FORMAT_NONE,
                                         names[0], strides[0],
                                         loaderPrivate);

    if (image == NULL)
        return NULL;

    image->planar_format = f;
    for (i = 0; i < f->nplanes; i++) {
        index = f->planes[i].buffer_index;
        image->offsets[index] = offsets[index];
        image->strides[index] = strides[index];
    }

    return image;
}

static __DRIimage *
intel_from_planar(__DRIimage *parent, int plane, void *loaderPrivate)
{
    int width, height, offset, stride, dri_format, cpp, index, pitch;
    struct intel_image_format *f;
    uint32_t mask_x, mask_y;
    __DRIimage *image;

    if (parent == NULL || parent->planar_format == NULL)
        return NULL;

    f = parent->planar_format;

    if (plane >= f->nplanes)
        return NULL;

    width = parent->region->width >> f->planes[plane].width_shift;
    height = parent->region->height >> f->planes[plane].height_shift;
    dri_format = f->planes[plane].dri_format;
    index = f->planes[plane].buffer_index;
    offset = parent->offsets[index];
    stride = parent->strides[index];

    image = intel_allocate_image(dri_format, loaderPrivate);
    cpp = _mesa_get_format_bytes(image->format); /* safe since no none format */
    pitch = stride / cpp;
    if (offset + height * cpp * pitch > parent->region->bo->size) {
       _mesa_warning(NULL, "intel_create_sub_image: subimage out of bounds");
       FREE(image);
       return NULL;
    }

    image->region = calloc(sizeof(*image->region), 1);
    if (image->region == NULL) {
       FREE(image);
       return NULL;
    }

    image->region->cpp = _mesa_get_format_bytes(image->format);
    image->region->width = width;
    image->region->height = height;
    image->region->pitch = pitch;
    image->region->refcount = 1;
    image->region->bo = parent->region->bo;
    drm_intel_bo_reference(image->region->bo);
    image->region->tiling = parent->region->tiling;
    image->region->screen = parent->region->screen;
    image->offset = offset;

    intel_region_get_tile_masks(image->region, &mask_x, &mask_y, false);
    if (offset & mask_x)
       _mesa_warning(NULL,
                     "intel_create_sub_image: offset not on tile boundary");

    return image;
}

static struct __DRIimageExtensionRec intelImageExtension = {
    { __DRI_IMAGE, 5 },
    intel_create_image_from_name,
    intel_create_image_from_renderbuffer,
    intel_destroy_image,
    intel_create_image,
    intel_query_image,
    intel_dup_image,
    intel_validate_usage,
    intel_create_image_from_names,
    intel_from_planar
};

static const __DRIextension *intelScreenExtensions[] = {
    &intelTexBufferExtension.base,
    &intelFlushExtension.base,
    &intelImageExtension.base,
    &dri2ConfigQueryExtension.base,
    NULL
};

static bool
intel_get_param(__DRIscreen *psp, int param, int *value)
{
   int ret;
   struct drm_i915_getparam gp;

   memset(&gp, 0, sizeof(gp));
   gp.param = param;
   gp.value = value;

   ret = drmCommandWriteRead(psp->fd, DRM_I915_GETPARAM, &gp, sizeof(gp));
   if (ret) {
      if (ret != -EINVAL)
	 _mesa_warning(NULL, "drm_i915_getparam: %d", ret);
      return false;
   }

   return true;
}

static bool
intel_get_boolean(__DRIscreen *psp, int param)
{
   int value = 0;
   return intel_get_param(psp, param, &value) && value;
}

static void
nop_callback(GLuint key, void *data, void *userData)
{
}

static void
intelDestroyScreen(__DRIscreen * sPriv)
{
   struct intel_screen *intelScreen = sPriv->driverPrivate;

   dri_bufmgr_destroy(intelScreen->bufmgr);
   driDestroyOptionInfo(&intelScreen->optionCache);

   /* Some regions may still have references to them at this point, so
    * flush the hash table to prevent _mesa_DeleteHashTable() from
    * complaining about the hash not being empty; */
   _mesa_HashDeleteAll(intelScreen->named_regions, nop_callback, NULL);
   _mesa_DeleteHashTable(intelScreen->named_regions);

   FREE(intelScreen);
   sPriv->driverPrivate = NULL;
}


/**
 * This is called when we need to set up GL rendering to a new X window.
 */
static GLboolean
intelCreateBuffer(__DRIscreen * driScrnPriv,
                  __DRIdrawable * driDrawPriv,
                  const struct gl_config * mesaVis, GLboolean isPixmap)
{
   struct intel_renderbuffer *rb;
   struct intel_screen *screen = (struct intel_screen*) driScrnPriv->driverPrivate;
   gl_format rgbFormat;
   unsigned num_samples = intel_quantize_num_samples(screen, mesaVis->samples);
   struct gl_framebuffer *fb;

   if (isPixmap)
      return false;

   fb = CALLOC_STRUCT(gl_framebuffer);
   if (!fb)
      return false;

   _mesa_initialize_window_framebuffer(fb, mesaVis);

   if (mesaVis->redBits == 5)
      rgbFormat = MESA_FORMAT_RGB565;
   else if (mesaVis->alphaBits == 0)
      rgbFormat = MESA_FORMAT_XRGB8888;
   else
      rgbFormat = MESA_FORMAT_ARGB8888;

   /* setup the hardware-based renderbuffers */
   rb = intel_create_renderbuffer(rgbFormat, num_samples);
   _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &rb->Base.Base);

   if (mesaVis->doubleBufferMode) {
      rb = intel_create_renderbuffer(rgbFormat, num_samples);
      _mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &rb->Base.Base);
   }

   /*
    * Assert here that the gl_config has an expected depth/stencil bit
    * combination: one of d24/s8, d16/s0, d0/s0. (See intelInitScreen2(),
    * which constructs the advertised configs.)
    */
   if (mesaVis->depthBits == 24) {
      assert(mesaVis->stencilBits == 8);

      if (screen->hw_has_separate_stencil) {
         rb = intel_create_private_renderbuffer(MESA_FORMAT_X8_Z24,
                                                num_samples);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &rb->Base.Base);
         rb = intel_create_private_renderbuffer(MESA_FORMAT_S8,
                                                num_samples);
         _mesa_add_renderbuffer(fb, BUFFER_STENCIL, &rb->Base.Base);
      } else {
         /*
          * Use combined depth/stencil. Note that the renderbuffer is
          * attached to two attachment points.
          */
         rb = intel_create_private_renderbuffer(MESA_FORMAT_S8_Z24,
                                                num_samples);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &rb->Base.Base);
         _mesa_add_renderbuffer(fb, BUFFER_STENCIL, &rb->Base.Base);
      }
   }
   else if (mesaVis->depthBits == 16) {
      assert(mesaVis->stencilBits == 0);
      rb = intel_create_private_renderbuffer(MESA_FORMAT_Z16,
                                             num_samples);
      _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &rb->Base.Base);
   }
   else {
      assert(mesaVis->depthBits == 0);
      assert(mesaVis->stencilBits == 0);
   }

   /* now add any/all software-based renderbuffers we may need */
   _swrast_add_soft_renderbuffers(fb,
                                  false, /* never sw color */
                                  false, /* never sw depth */
                                  false, /* never sw stencil */
                                  mesaVis->accumRedBits > 0,
                                  false, /* never sw alpha */
                                  false  /* never sw aux */ );
   driDrawPriv->driverPrivate = fb;

   return true;
}

static void
intelDestroyBuffer(__DRIdrawable * driDrawPriv)
{
    struct gl_framebuffer *fb = driDrawPriv->driverPrivate;
  
    _mesa_reference_framebuffer(&fb, NULL);
}

/* There are probably better ways to do this, such as an
 * init-designated function to register chipids and createcontext
 * functions.
 */
extern bool
i830CreateContext(const struct gl_config *mesaVis,
		  __DRIcontext *driContextPriv,
		  void *sharedContextPrivate);

extern bool
i915CreateContext(int api,
		  const struct gl_config *mesaVis,
		  __DRIcontext *driContextPriv,
                  unsigned major_version,
                  unsigned minor_version,
                  unsigned *error,
		  void *sharedContextPrivate);
extern bool
brwCreateContext(int api,
	         const struct gl_config *mesaVis,
	         __DRIcontext *driContextPriv,
                 unsigned major_version,
                 unsigned minor_version,
                 uint32_t flags,
                 unsigned *error,
		 void *sharedContextPrivate);

static GLboolean
intelCreateContext(gl_api api,
		   const struct gl_config * mesaVis,
                   __DRIcontext * driContextPriv,
		   unsigned major_version,
		   unsigned minor_version,
		   uint32_t flags,
		   unsigned *error,
                   void *sharedContextPrivate)
{
   __DRIscreen *sPriv = driContextPriv->driScreenPriv;
   struct intel_screen *intelScreen = sPriv->driverPrivate;
   bool success = false;

#ifdef I915
   if (IS_9XX(intelScreen->deviceID)) {
      success = i915CreateContext(api, mesaVis, driContextPriv,
                                  major_version, minor_version, error,
                                  sharedContextPrivate);
   } else {
      switch (api) {
      case API_OPENGL:
         if (major_version > 1 || minor_version > 3) {
            *error = __DRI_CTX_ERROR_BAD_VERSION;
            success = false;
         }
         break;
      case API_OPENGLES:
         break;
      default:
         *error = __DRI_CTX_ERROR_BAD_API;
         success = false;
      }

      if (success) {
         intelScreen->no_vbo = true;
         success = i830CreateContext(mesaVis, driContextPriv,
                                     sharedContextPrivate);
         if (!success)
            *error = __DRI_CTX_ERROR_NO_MEMORY;
      }
   }
#else
   success = brwCreateContext(api, mesaVis,
                              driContextPriv,
                              major_version, minor_version, flags,
                              error, sharedContextPrivate);
#endif

   if (success)
      return true;

   if (driContextPriv->driverPrivate != NULL)
      intelDestroyContext(driContextPriv);

   return false;
}

static bool
intel_init_bufmgr(struct intel_screen *intelScreen)
{
   __DRIscreen *spriv = intelScreen->driScrnPriv;
   int num_fences = 0;

   intelScreen->no_hw = getenv("INTEL_NO_HW") != NULL;

   intelScreen->bufmgr = intel_bufmgr_gem_init(spriv->fd, BATCH_SZ);
   if (intelScreen->bufmgr == NULL) {
      fprintf(stderr, "[%s:%u] Error initializing buffer manager.\n",
	      __func__, __LINE__);
      return false;
   }

   if (!intel_get_param(spriv, I915_PARAM_NUM_FENCES_AVAIL, &num_fences) ||
       num_fences == 0) {
      fprintf(stderr, "[%s: %u] Kernel 2.6.29 required.\n", __func__, __LINE__);
      return false;
   }

   drm_intel_bufmgr_gem_enable_fenced_relocs(intelScreen->bufmgr);

   intelScreen->named_regions = _mesa_NewHashTable();

   intelScreen->relaxed_relocations = 0;
   intelScreen->relaxed_relocations |=
      intel_get_boolean(spriv, I915_PARAM_HAS_RELAXED_DELTA) << 0;

   return true;
}

/**
 * Override intel_screen.hw_has_separate_stencil with environment variable
 * INTEL_SEPARATE_STENCIL.
 *
 * Valid values for INTEL_SEPARATE_STENCIL are "0" and "1". If an invalid
 * valid value is encountered, a warning is emitted and INTEL_SEPARATE_STENCIL
 * is ignored.
 */
static void
intel_override_separate_stencil(struct intel_screen *screen)
{
   const char *s = getenv("INTEL_SEPARATE_STENCIL");
   if (!s) {
      return;
   } else if (!strncmp("0", s, 2)) {
      screen->hw_has_separate_stencil = false;
   } else if (!strncmp("1", s, 2)) {
      screen->hw_has_separate_stencil = true;
   } else {
      fprintf(stderr,
	      "warning: env variable INTEL_SEPARATE_STENCIL=\"%s\" has "
	      "invalid value and is ignored", s);
   }
}

static bool
intel_detect_swizzling(struct intel_screen *screen)
{
   drm_intel_bo *buffer;
   unsigned long flags = 0;
   unsigned long aligned_pitch;
   uint32_t tiling = I915_TILING_X;
   uint32_t swizzle_mode = 0;

   buffer = drm_intel_bo_alloc_tiled(screen->bufmgr, "swizzle test",
				     64, 64, 4,
				     &tiling, &aligned_pitch, flags);
   if (buffer == NULL)
      return false;

   drm_intel_bo_get_tiling(buffer, &tiling, &swizzle_mode);
   drm_intel_bo_unreference(buffer);

   if (swizzle_mode == I915_BIT_6_SWIZZLE_NONE)
      return false;
   else
      return true;
}

static __DRIconfig**
intel_screen_make_configs(__DRIscreen *dri_screen)
{
   static const GLenum back_buffer_modes[] = {
       GLX_NONE, GLX_SWAP_UNDEFINED_OML, GLX_SWAP_COPY_OML
   };

   static const uint8_t singlesample_samples[1] = {0};
   static const uint8_t multisample_samples[2]  = {4, 8};

   struct intel_screen *screen = dri_screen->driverPrivate;
   GLenum fb_format[3];
   GLenum fb_type[3];
   uint8_t depth_bits[4], stencil_bits[4];
   __DRIconfig **configs = NULL;

   fb_format[0] = GL_RGB;
   fb_type[0] = GL_UNSIGNED_SHORT_5_6_5;

   fb_format[1] = GL_BGR;
   fb_type[1] = GL_UNSIGNED_INT_8_8_8_8_REV;

   fb_format[2] = GL_BGRA;
   fb_type[2] = GL_UNSIGNED_INT_8_8_8_8_REV;

   /* Generate singlesample configs without accumulation buffer. */
   for (int i = 0; i < ARRAY_SIZE(fb_format); i++) {
      __DRIconfig **new_configs;
      const int num_depth_stencil_bits = 2;

      /* Starting with DRI2 protocol version 1.1 we can request a depth/stencil
       * buffer that has a different number of bits per pixel than the color
       * buffer.  This isn't yet supported here.
       */
      depth_bits[0] = 0;
      stencil_bits[0] = 0;

      if (fb_type[i] == GL_UNSIGNED_SHORT_5_6_5) {
         depth_bits[1] = 16;
         stencil_bits[1] = 0;
      } else {
         depth_bits[1] = 24;
         stencil_bits[1] = 8;
      }

      new_configs = driCreateConfigs(fb_format[i], fb_type[i],
                                     depth_bits,
                                     stencil_bits,
                                     num_depth_stencil_bits,
                                     back_buffer_modes,
                                     ARRAY_SIZE(back_buffer_modes),
                                     singlesample_samples, 1,
                                     false);
      configs = driConcatConfigs(configs, new_configs);
   }

   /* Generate the minimum possible set of configs that include an
    * accumulation buffer.
    */
   for (int i = 0; i < ARRAY_SIZE(fb_format); i++) {
      __DRIconfig **new_configs;

      if (fb_type[i] == GL_UNSIGNED_SHORT_5_6_5) {
         depth_bits[0] = 16;
         stencil_bits[0] = 0;
      } else {
         depth_bits[0] = 24;
         stencil_bits[0] = 8;
      }

      new_configs = driCreateConfigs(fb_format[i], fb_type[i],
                                     depth_bits, stencil_bits, 1,
                                     back_buffer_modes + 1, 1,
                                     singlesample_samples, 1,
                                     true);
      configs = driConcatConfigs(configs, new_configs);
   }

   /* Generate multisample configs.
    *
    * This loop breaks early, and hence is a no-op, on gen < 6.
    *
    * Multisample configs must follow the singlesample configs in order to
    * work around an X server bug present in 1.12. The X server chooses to
    * associate the first listed RGBA888-Z24S8 config, regardless of its
    * sample count, with the 32-bit depth visual used for compositing.
    *
    * Only doublebuffer configs with GLX_SWAP_UNDEFINED_OML behavior are
    * supported.  Singlebuffer configs are not supported because no one wants
    * them. GLX_SWAP_COPY_OML is not supported due to page flipping.
    */
   for (int i = 0; i < ARRAY_SIZE(fb_format); i++) {
      if (screen->gen < 6)
         break;

      __DRIconfig **new_configs;
      const int num_depth_stencil_bits = 2;
      int num_msaa_modes = 0;

      depth_bits[0] = 0;
      stencil_bits[0] = 0;

      if (fb_type[i] == GL_UNSIGNED_SHORT_5_6_5) {
         depth_bits[1] = 16;
         stencil_bits[1] = 0;
      } else {
         depth_bits[1] = 24;
         stencil_bits[1] = 8;
      }

      if (screen->gen >= 7)
         num_msaa_modes = 2;
      else if (screen->gen == 6)
         num_msaa_modes = 1;

      new_configs = driCreateConfigs(fb_format[i], fb_type[i],
                                     depth_bits,
                                     stencil_bits,
                                     num_depth_stencil_bits,
                                     back_buffer_modes + 1, 1,
                                     multisample_samples,
                                     num_msaa_modes,
                                     false);
      configs = driConcatConfigs(configs, new_configs);
   }

   if (configs == NULL) {
      fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__,
              __LINE__);
      return NULL;
   }

   return configs;
}

/**
 * This is the driver specific part of the createNewScreen entry point.
 * Called when using DRI2.
 *
 * \return the struct gl_config supported by this driver
 */
static const
__DRIconfig **intelInitScreen2(__DRIscreen *psp)
{
   struct intel_screen *intelScreen;
   unsigned int api_mask;

   if (psp->dri2.loader->base.version <= 2 ||
       psp->dri2.loader->getBuffersWithFormat == NULL) {
      fprintf(stderr,
	      "\nERROR!  DRI2 loader with getBuffersWithFormat() "
	      "support required\n");
      return false;
   }

   /* Allocate the private area */
   intelScreen = CALLOC(sizeof *intelScreen);
   if (!intelScreen) {
      fprintf(stderr, "\nERROR!  Allocating private area failed\n");
      return false;
   }
   /* parse information in __driConfigOptions */
   driParseOptionInfo(&intelScreen->optionCache,
                      __driConfigOptions, __driNConfigOptions);

   intelScreen->driScrnPriv = psp;
   psp->driverPrivate = (void *) intelScreen;

   if (!intel_init_bufmgr(intelScreen))
       return false;

   intelScreen->deviceID = drm_intel_bufmgr_gem_get_devid(intelScreen->bufmgr);

   intelScreen->kernel_has_gen7_sol_reset =
      intel_get_boolean(intelScreen->driScrnPriv,
			I915_PARAM_HAS_GEN7_SOL_RESET);

   if (IS_GEN7(intelScreen->deviceID)) {
      intelScreen->gen = 7;
   } else if (IS_GEN6(intelScreen->deviceID)) {
      intelScreen->gen = 6;
   } else if (IS_GEN5(intelScreen->deviceID)) {
      intelScreen->gen = 5;
   } else if (IS_965(intelScreen->deviceID)) {
      intelScreen->gen = 4;
   } else if (IS_9XX(intelScreen->deviceID)) {
      intelScreen->gen = 3;
   } else {
      intelScreen->gen = 2;
   }

   intelScreen->hw_has_separate_stencil = intelScreen->gen >= 6;
   intelScreen->hw_must_use_separate_stencil = intelScreen->gen >= 7;

   int has_llc = 0;
   bool success = intel_get_param(intelScreen->driScrnPriv, I915_PARAM_HAS_LLC,
				  &has_llc);
   if (success && has_llc)
      intelScreen->hw_has_llc = true;
   else if (!success && intelScreen->gen >= 6)
      intelScreen->hw_has_llc = true;

   intel_override_separate_stencil(intelScreen);

   api_mask = (1 << __DRI_API_OPENGL);
#if FEATURE_ES1
   api_mask |= (1 << __DRI_API_GLES);
#endif
#if FEATURE_ES2
   api_mask |= (1 << __DRI_API_GLES2);
#endif

   if (IS_9XX(intelScreen->deviceID) || IS_965(intelScreen->deviceID))
      psp->api_mask = api_mask;

   intelScreen->hw_has_swizzling = intel_detect_swizzling(intelScreen);

   psp->extensions = intelScreenExtensions;

   return (const __DRIconfig**) intel_screen_make_configs(psp);
}

struct intel_buffer {
   __DRIbuffer base;
   struct intel_region *region;
};

static __DRIbuffer *
intelAllocateBuffer(__DRIscreen *screen,
		    unsigned attachment, unsigned format,
		    int width, int height)
{
   struct intel_buffer *intelBuffer;
   struct intel_screen *intelScreen = screen->driverPrivate;

   assert(attachment == __DRI_BUFFER_FRONT_LEFT ||
          attachment == __DRI_BUFFER_BACK_LEFT);

   intelBuffer = CALLOC(sizeof *intelBuffer);
   if (intelBuffer == NULL)
      return NULL;

   /* The front and back buffers are color buffers, which are X tiled. */
   intelBuffer->region = intel_region_alloc(intelScreen,
                                            I915_TILING_X,
                                            format / 8,
                                            width,
                                            height,
                                            true);
   
   if (intelBuffer->region == NULL) {
	   FREE(intelBuffer);
	   return NULL;
   }
   
   intel_region_flink(intelBuffer->region, &intelBuffer->base.name);

   intelBuffer->base.attachment = attachment;
   intelBuffer->base.cpp = intelBuffer->region->cpp;
   intelBuffer->base.pitch =
         intelBuffer->region->pitch * intelBuffer->region->cpp;

   return &intelBuffer->base;
}

static void
intelReleaseBuffer(__DRIscreen *screen, __DRIbuffer *buffer)
{
   struct intel_buffer *intelBuffer = (struct intel_buffer *) buffer;

   intel_region_release(&intelBuffer->region);
   free(intelBuffer);
}


const struct __DriverAPIRec driDriverAPI = {
   .InitScreen		 = intelInitScreen2,
   .DestroyScreen	 = intelDestroyScreen,
   .CreateContext	 = intelCreateContext,
   .DestroyContext	 = intelDestroyContext,
   .CreateBuffer	 = intelCreateBuffer,
   .DestroyBuffer	 = intelDestroyBuffer,
   .MakeCurrent		 = intelMakeCurrent,
   .UnbindContext	 = intelUnbindContext,
   .AllocateBuffer       = intelAllocateBuffer,
   .ReleaseBuffer        = intelReleaseBuffer
};

/* This is the table of extensions that the loader will dlsym() for. */
PUBLIC const __DRIextension *__driDriverExtensions[] = {
    &driCoreExtension.base,
    &driDRI2Extension.base,
    NULL
};
