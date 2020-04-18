/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

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

**************************************************************************/

/**
 * \file radeon_screen.c
 * Screen initialization functions for the Radeon driver.
 *
 * \author Kevin E. Martin <martin@valinux.com>
 * \author  Gareth Hughes <gareth@valinux.com>
 */

#include <errno.h>
#include "main/glheader.h"
#include "main/imports.h"
#include "main/mtypes.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/fbobject.h"
#include "swrast/s_renderbuffer.h"

#define STANDALONE_MMIO
#include "radeon_chipset.h"
#include "radeon_macros.h"
#include "radeon_screen.h"
#include "radeon_common.h"
#include "radeon_common_context.h"
#if defined(RADEON_R100)
#include "radeon_context.h"
#include "radeon_tex.h"
#elif defined(RADEON_R200)
#include "r200_context.h"
#include "r200_tex.h"
#endif

#include "utils.h"

#include "GL/internal/dri_interface.h"

/* Radeon configuration
 */
#include "xmlpool.h"

#define DRI_CONF_COMMAND_BUFFER_SIZE(def,min,max) \
DRI_CONF_OPT_BEGIN_V(command_buffer_size,int,def, # min ":" # max ) \
        DRI_CONF_DESC(en,"Size of command buffer (in KB)") \
        DRI_CONF_DESC(de,"Grösse des Befehlspuffers (in KB)") \
DRI_CONF_OPT_END

#if defined(RADEON_R100)	/* R100 */
PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
    DRI_CONF_SECTION_PERFORMANCE
        DRI_CONF_TCL_MODE(DRI_CONF_TCL_CODEGEN)
        DRI_CONF_FTHROTTLE_MODE(DRI_CONF_FTHROTTLE_IRQS)
        DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
        DRI_CONF_MAX_TEXTURE_UNITS(3,2,3)
        DRI_CONF_HYPERZ(false)
        DRI_CONF_COMMAND_BUFFER_SIZE(8, 8, 32)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_QUALITY
        DRI_CONF_TEXTURE_DEPTH(DRI_CONF_TEXTURE_DEPTH_FB)
        DRI_CONF_DEF_MAX_ANISOTROPY(1.0,"1.0,2.0,4.0,8.0,16.0")
        DRI_CONF_NO_NEG_LOD_BIAS(false)
        DRI_CONF_FORCE_S3TC_ENABLE(false)
        DRI_CONF_COLOR_REDUCTION(DRI_CONF_COLOR_REDUCTION_DITHER)
        DRI_CONF_ROUND_MODE(DRI_CONF_ROUND_TRUNC)
        DRI_CONF_DITHER_MODE(DRI_CONF_DITHER_XERRORDIFF)
        DRI_CONF_ALLOW_LARGE_TEXTURES(2)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_DEBUG
        DRI_CONF_NO_RAST(false)
    DRI_CONF_SECTION_END
DRI_CONF_END;
static const GLuint __driNConfigOptions = 15;

#elif defined(RADEON_R200)

PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
    DRI_CONF_SECTION_PERFORMANCE
        DRI_CONF_TCL_MODE(DRI_CONF_TCL_CODEGEN)
        DRI_CONF_FTHROTTLE_MODE(DRI_CONF_FTHROTTLE_IRQS)
        DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
        DRI_CONF_MAX_TEXTURE_UNITS(6,2,6)
        DRI_CONF_HYPERZ(false)
        DRI_CONF_COMMAND_BUFFER_SIZE(8, 8, 32)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_QUALITY
        DRI_CONF_TEXTURE_DEPTH(DRI_CONF_TEXTURE_DEPTH_FB)
        DRI_CONF_DEF_MAX_ANISOTROPY(1.0,"1.0,2.0,4.0,8.0,16.0")
        DRI_CONF_NO_NEG_LOD_BIAS(false)
        DRI_CONF_FORCE_S3TC_ENABLE(false)
        DRI_CONF_COLOR_REDUCTION(DRI_CONF_COLOR_REDUCTION_DITHER)
        DRI_CONF_ROUND_MODE(DRI_CONF_ROUND_TRUNC)
        DRI_CONF_DITHER_MODE(DRI_CONF_DITHER_XERRORDIFF)
        DRI_CONF_ALLOW_LARGE_TEXTURES(2)
        DRI_CONF_TEXTURE_BLEND_QUALITY(1.0,"0.0:1.0")
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_DEBUG
        DRI_CONF_NO_RAST(false)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_SOFTWARE
        DRI_CONF_NV_VERTEX_PROGRAM(false)
    DRI_CONF_SECTION_END
DRI_CONF_END;
static const GLuint __driNConfigOptions = 17;

#endif

#ifndef RADEON_INFO_TILE_CONFIG
#define RADEON_INFO_TILE_CONFIG 0x6
#endif

static int
radeonGetParam(__DRIscreen *sPriv, int param, void *value)
{
  int ret;
  drm_radeon_getparam_t gp = { 0 };
  struct drm_radeon_info info = { 0 };

  if (sPriv->drm_version.major >= 2) {
      info.value = (uint64_t)(uintptr_t)value;
      switch (param) {
      case RADEON_PARAM_DEVICE_ID:
          info.request = RADEON_INFO_DEVICE_ID;
          break;
      case RADEON_PARAM_NUM_GB_PIPES:
          info.request = RADEON_INFO_NUM_GB_PIPES;
          break;
      case RADEON_PARAM_NUM_Z_PIPES:
          info.request = RADEON_INFO_NUM_Z_PIPES;
          break;
      case RADEON_INFO_TILE_CONFIG:
	  info.request = RADEON_INFO_TILE_CONFIG;
          break;
      default:
          return -EINVAL;
      }
      ret = drmCommandWriteRead(sPriv->fd, DRM_RADEON_INFO, &info, sizeof(info));
  } else {
      gp.param = param;
      gp.value = value;

      ret = drmCommandWriteRead(sPriv->fd, DRM_RADEON_GETPARAM, &gp, sizeof(gp));
  }
  return ret;
}

#if defined(RADEON_R100)
static const __DRItexBufferExtension radeonTexBufferExtension = {
    { __DRI_TEX_BUFFER, __DRI_TEX_BUFFER_VERSION },
   radeonSetTexBuffer,
   radeonSetTexBuffer2,
};
#elif defined(RADEON_R200)
static const __DRItexBufferExtension r200TexBufferExtension = {
    { __DRI_TEX_BUFFER, __DRI_TEX_BUFFER_VERSION },
   r200SetTexBuffer,
   r200SetTexBuffer2,
};
#endif

static void
radeonDRI2Flush(__DRIdrawable *drawable)
{
    radeonContextPtr rmesa;

    rmesa = (radeonContextPtr) drawable->driContextPriv->driverPrivate;
    radeonFlush(rmesa->glCtx);
}

static const struct __DRI2flushExtensionRec radeonFlushExtension = {
    { __DRI2_FLUSH, __DRI2_FLUSH_VERSION },
    radeonDRI2Flush,
    dri2InvalidateDrawable,
};

static __DRIimage *
radeon_create_image_from_name(__DRIscreen *screen,
                              int width, int height, int format,
                              int name, int pitch, void *loaderPrivate)
{
   __DRIimage *image;
   radeonScreenPtr radeonScreen = screen->driverPrivate;

   if (name == 0)
      return NULL;

   image = CALLOC(sizeof *image);
   if (image == NULL)
      return NULL;

   switch (format) {
   case __DRI_IMAGE_FORMAT_RGB565:
      image->format = MESA_FORMAT_RGB565;
      image->internal_format = GL_RGB;
      image->data_type = GL_UNSIGNED_BYTE;
      break;
   case __DRI_IMAGE_FORMAT_XRGB8888:
      image->format = MESA_FORMAT_XRGB8888;
      image->internal_format = GL_RGB;
      image->data_type = GL_UNSIGNED_BYTE;
      break;
   case __DRI_IMAGE_FORMAT_ARGB8888:
      image->format = MESA_FORMAT_ARGB8888;
      image->internal_format = GL_RGBA;
      image->data_type = GL_UNSIGNED_BYTE;
      break;
   default:
      free(image);
      return NULL;
   }

   image->data = loaderPrivate;
   image->cpp = _mesa_get_format_bytes(image->format);
   image->width = width;
   image->pitch = pitch;
   image->height = height;

   image->bo = radeon_bo_open(radeonScreen->bom,
                              (uint32_t)name,
                              image->pitch * image->height * image->cpp,
                              0,
                              RADEON_GEM_DOMAIN_VRAM,
                              0);

   if (image->bo == NULL) {
      FREE(image);
      return NULL;
   }

   return image;
}

static __DRIimage *
radeon_create_image_from_renderbuffer(__DRIcontext *context,
                                      int renderbuffer, void *loaderPrivate)
{
   __DRIimage *image;
   radeonContextPtr radeon = context->driverPrivate;
   struct gl_renderbuffer *rb;
   struct radeon_renderbuffer *rrb;

   rb = _mesa_lookup_renderbuffer(radeon->glCtx, renderbuffer);
   if (!rb) {
      _mesa_error(radeon->glCtx,
                  GL_INVALID_OPERATION, "glRenderbufferExternalMESA");
      return NULL;
   }

   rrb = radeon_renderbuffer(rb);
   image = CALLOC(sizeof *image);
   if (image == NULL)
      return NULL;

   image->internal_format = rb->InternalFormat;
   image->format = rb->Format;
   image->cpp = rrb->cpp;
   image->data_type = GL_UNSIGNED_BYTE;
   image->data = loaderPrivate;
   radeon_bo_ref(rrb->bo);
   image->bo = rrb->bo;

   image->width = rb->Width;
   image->height = rb->Height;
   image->pitch = rrb->pitch / image->cpp;

   return image;
}

static void
radeon_destroy_image(__DRIimage *image)
{
   radeon_bo_unref(image->bo);
   FREE(image);
}

static __DRIimage *
radeon_create_image(__DRIscreen *screen,
                    int width, int height, int format,
                    unsigned int use,
                    void *loaderPrivate)
{
   __DRIimage *image;
   radeonScreenPtr radeonScreen = screen->driverPrivate;

   image = CALLOC(sizeof *image);
   if (image == NULL)
      return NULL;

   image->dri_format = format;

   switch (format) {
   case __DRI_IMAGE_FORMAT_RGB565:
      image->format = MESA_FORMAT_RGB565;
      image->internal_format = GL_RGB;
      image->data_type = GL_UNSIGNED_BYTE;
      break;
   case __DRI_IMAGE_FORMAT_XRGB8888:
      image->format = MESA_FORMAT_XRGB8888;
      image->internal_format = GL_RGB;
      image->data_type = GL_UNSIGNED_BYTE;
      break;
   case __DRI_IMAGE_FORMAT_ARGB8888:
      image->format = MESA_FORMAT_ARGB8888;
      image->internal_format = GL_RGBA;
      image->data_type = GL_UNSIGNED_BYTE;
      break;
   default:
      free(image);
      return NULL;
   }

   image->data = loaderPrivate;
   image->cpp = _mesa_get_format_bytes(image->format);
   image->width = width;
   image->height = height;
   image->pitch = ((image->cpp * image->width + 255) & ~255) / image->cpp;

   image->bo = radeon_bo_open(radeonScreen->bom,
                              0,
                              image->pitch * image->height * image->cpp,
                              0,
                              RADEON_GEM_DOMAIN_VRAM,
                              0);

   if (image->bo == NULL) {
      FREE(image);
      return NULL;
   }

   return image;
}

static GLboolean
radeon_query_image(__DRIimage *image, int attrib, int *value)
{
   switch (attrib) {
   case __DRI_IMAGE_ATTRIB_STRIDE:
      *value = image->pitch * image->cpp;
      return GL_TRUE;
   case __DRI_IMAGE_ATTRIB_HANDLE:
      *value = image->bo->handle;
      return GL_TRUE;
   case __DRI_IMAGE_ATTRIB_NAME:
      radeon_gem_get_kernel_name(image->bo, (uint32_t *) value);
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}

static struct __DRIimageExtensionRec radeonImageExtension = {
    { __DRI_IMAGE, 1 },
   radeon_create_image_from_name,
   radeon_create_image_from_renderbuffer,
   radeon_destroy_image,
   radeon_create_image,
   radeon_query_image
};

static int radeon_set_screen_flags(radeonScreenPtr screen, int device_id)
{
   screen->device_id = device_id;
   screen->chip_flags = 0;
   switch ( device_id ) {
#if defined(RADEON_R100)
   case PCI_CHIP_RN50_515E:
   case PCI_CHIP_RN50_5969:
	return -1;

   case PCI_CHIP_RADEON_LY:
   case PCI_CHIP_RADEON_LZ:
   case PCI_CHIP_RADEON_QY:
   case PCI_CHIP_RADEON_QZ:
      screen->chip_family = CHIP_FAMILY_RV100;
      break;

   case PCI_CHIP_RS100_4136:
   case PCI_CHIP_RS100_4336:
      screen->chip_family = CHIP_FAMILY_RS100;
      break;

   case PCI_CHIP_RS200_4137:
   case PCI_CHIP_RS200_4337:
   case PCI_CHIP_RS250_4237:
   case PCI_CHIP_RS250_4437:
      screen->chip_family = CHIP_FAMILY_RS200;
      break;

   case PCI_CHIP_RADEON_QD:
   case PCI_CHIP_RADEON_QE:
   case PCI_CHIP_RADEON_QF:
   case PCI_CHIP_RADEON_QG:
      /* all original radeons (7200) presumably have a stencil op bug */
      screen->chip_family = CHIP_FAMILY_R100;
      screen->chip_flags = RADEON_CHIPSET_TCL | RADEON_CHIPSET_BROKEN_STENCIL | RADEON_CHIPSET_DEPTH_ALWAYS_TILED;
      break;

   case PCI_CHIP_RV200_QW:
   case PCI_CHIP_RV200_QX:
   case PCI_CHIP_RADEON_LW:
   case PCI_CHIP_RADEON_LX:
      screen->chip_family = CHIP_FAMILY_RV200;
      screen->chip_flags = RADEON_CHIPSET_TCL | RADEON_CHIPSET_DEPTH_ALWAYS_TILED;
      break;

#elif defined(RADEON_R200)
   case PCI_CHIP_R200_BB:
   case PCI_CHIP_R200_QH:
   case PCI_CHIP_R200_QL:
   case PCI_CHIP_R200_QM:
      screen->chip_family = CHIP_FAMILY_R200;
      screen->chip_flags = RADEON_CHIPSET_TCL | RADEON_CHIPSET_DEPTH_ALWAYS_TILED;
      break;

   case PCI_CHIP_RV250_If:
   case PCI_CHIP_RV250_Ig:
   case PCI_CHIP_RV250_Ld:
   case PCI_CHIP_RV250_Lf:
   case PCI_CHIP_RV250_Lg:
      screen->chip_family = CHIP_FAMILY_RV250;
      screen->chip_flags = R200_CHIPSET_YCBCR_BROKEN | RADEON_CHIPSET_TCL | RADEON_CHIPSET_DEPTH_ALWAYS_TILED;
      break;

   case PCI_CHIP_RV280_4C6E:
   case PCI_CHIP_RV280_5960:
   case PCI_CHIP_RV280_5961:
   case PCI_CHIP_RV280_5962:
   case PCI_CHIP_RV280_5964:
   case PCI_CHIP_RV280_5965:
   case PCI_CHIP_RV280_5C61:
   case PCI_CHIP_RV280_5C63:
      screen->chip_family = CHIP_FAMILY_RV280;
      screen->chip_flags = RADEON_CHIPSET_TCL | RADEON_CHIPSET_DEPTH_ALWAYS_TILED;
      break;

   case PCI_CHIP_RS300_5834:
   case PCI_CHIP_RS300_5835:
   case PCI_CHIP_RS350_7834:
   case PCI_CHIP_RS350_7835:
      screen->chip_family = CHIP_FAMILY_RS300;
      screen->chip_flags = RADEON_CHIPSET_DEPTH_ALWAYS_TILED;
      break;
#endif

   default:
      fprintf(stderr, "unknown chip id 0x%x, can't guess.\n",
	      device_id);
      return -1;
   }

   return 0;
}

static radeonScreenPtr
radeonCreateScreen2(__DRIscreen *sPriv)
{
   radeonScreenPtr screen;
   int i;
   int ret;
   uint32_t device_id = 0;

   /* Allocate the private area */
   screen = (radeonScreenPtr) CALLOC( sizeof(*screen) );
   if ( !screen ) {
      fprintf(stderr, "%s: Could not allocate memory for screen structure", __FUNCTION__);
      fprintf(stderr, "leaving here\n");
      return NULL;
   }

   radeon_init_debug();

   /* parse information in __driConfigOptions */
   driParseOptionInfo (&screen->optionCache,
		       __driConfigOptions, __driNConfigOptions);

   screen->chip_flags = 0;

   screen->irq = 1;

   ret = radeonGetParam(sPriv, RADEON_PARAM_DEVICE_ID, &device_id);
   if (ret) {
     FREE( screen );
     fprintf(stderr, "drm_radeon_getparam_t (RADEON_PARAM_DEVICE_ID): %d\n", ret);
     return NULL;
   }

   ret = radeon_set_screen_flags(screen, device_id);
   if (ret == -1)
     return NULL;

   if (getenv("RADEON_NO_TCL"))
	   screen->chip_flags &= ~RADEON_CHIPSET_TCL;

   i = 0;
   screen->extensions[i++] = &dri2ConfigQueryExtension.base;

#if defined(RADEON_R100)
   screen->extensions[i++] = &radeonTexBufferExtension.base;
#elif defined(RADEON_R200)
   screen->extensions[i++] = &r200TexBufferExtension.base;
#endif

   screen->extensions[i++] = &radeonFlushExtension.base;
   screen->extensions[i++] = &radeonImageExtension.base;

   screen->extensions[i++] = NULL;
   sPriv->extensions = screen->extensions;

   screen->driScreen = sPriv;
   screen->bom = radeon_bo_manager_gem_ctor(sPriv->fd);
   if (screen->bom == NULL) {
       free(screen);
       return NULL;
   }
   return screen;
}

/* Destroy the device specific screen private data struct.
 */
static void
radeonDestroyScreen( __DRIscreen *sPriv )
{
    radeonScreenPtr screen = (radeonScreenPtr)sPriv->driverPrivate;

    if (!screen)
        return;

#ifdef RADEON_BO_TRACK
    radeon_tracker_print(&screen->bom->tracker, stderr);
#endif
    radeon_bo_manager_gem_dtor(screen->bom);

    /* free all option information */
    driDestroyOptionInfo (&screen->optionCache);

    FREE( screen );
    sPriv->driverPrivate = NULL;
}


/* Initialize the driver specific screen private data.
 */
static GLboolean
radeonInitDriver( __DRIscreen *sPriv )
{
    sPriv->driverPrivate = (void *) radeonCreateScreen2( sPriv );
    if ( !sPriv->driverPrivate ) {
        radeonDestroyScreen( sPriv );
        return GL_FALSE;
    }

    return GL_TRUE;
}



/**
 * Create the Mesa framebuffer and renderbuffers for a given window/drawable.
 *
 * \todo This function (and its interface) will need to be updated to support
 * pbuffers.
 */
static GLboolean
radeonCreateBuffer( __DRIscreen *driScrnPriv,
                    __DRIdrawable *driDrawPriv,
                    const struct gl_config *mesaVis,
                    GLboolean isPixmap )
{
    radeonScreenPtr screen = (radeonScreenPtr) driScrnPriv->driverPrivate;

    const GLboolean swDepth = GL_FALSE;
    const GLboolean swAlpha = GL_FALSE;
    const GLboolean swAccum = mesaVis->accumRedBits > 0;
    const GLboolean swStencil = mesaVis->stencilBits > 0 &&
	mesaVis->depthBits != 24;
    gl_format rgbFormat;
    struct radeon_framebuffer *rfb;

    if (isPixmap)
      return GL_FALSE; /* not implemented */

    rfb = CALLOC_STRUCT(radeon_framebuffer);
    if (!rfb)
      return GL_FALSE;

    _mesa_initialize_window_framebuffer(&rfb->base, mesaVis);

    if (mesaVis->redBits == 5)
        rgbFormat = _mesa_little_endian() ? MESA_FORMAT_RGB565 : MESA_FORMAT_RGB565_REV;
    else if (mesaVis->alphaBits == 0)
        rgbFormat = _mesa_little_endian() ? MESA_FORMAT_XRGB8888 : MESA_FORMAT_XRGB8888_REV;
    else
        rgbFormat = _mesa_little_endian() ? MESA_FORMAT_ARGB8888 : MESA_FORMAT_ARGB8888_REV;

    /* front color renderbuffer */
    rfb->color_rb[0] = radeon_create_renderbuffer(rgbFormat, driDrawPriv);
    _mesa_add_renderbuffer(&rfb->base, BUFFER_FRONT_LEFT, &rfb->color_rb[0]->base.Base);
    rfb->color_rb[0]->has_surface = 1;

    /* back color renderbuffer */
    if (mesaVis->doubleBufferMode) {
      rfb->color_rb[1] = radeon_create_renderbuffer(rgbFormat, driDrawPriv);
	_mesa_add_renderbuffer(&rfb->base, BUFFER_BACK_LEFT, &rfb->color_rb[1]->base.Base);
	rfb->color_rb[1]->has_surface = 1;
    }

    if (mesaVis->depthBits == 24) {
      if (mesaVis->stencilBits == 8) {
	struct radeon_renderbuffer *depthStencilRb =
           radeon_create_renderbuffer(MESA_FORMAT_S8_Z24, driDrawPriv);
	_mesa_add_renderbuffer(&rfb->base, BUFFER_DEPTH, &depthStencilRb->base.Base);
	_mesa_add_renderbuffer(&rfb->base, BUFFER_STENCIL, &depthStencilRb->base.Base);
	depthStencilRb->has_surface = screen->depthHasSurface;
      } else {
	/* depth renderbuffer */
	struct radeon_renderbuffer *depth =
           radeon_create_renderbuffer(MESA_FORMAT_X8_Z24, driDrawPriv);
	_mesa_add_renderbuffer(&rfb->base, BUFFER_DEPTH, &depth->base.Base);
	depth->has_surface = screen->depthHasSurface;
      }
    } else if (mesaVis->depthBits == 16) {
        /* just 16-bit depth buffer, no hw stencil */
	struct radeon_renderbuffer *depth =
           radeon_create_renderbuffer(MESA_FORMAT_Z16, driDrawPriv);
	_mesa_add_renderbuffer(&rfb->base, BUFFER_DEPTH, &depth->base.Base);
	depth->has_surface = screen->depthHasSurface;
    }

    _swrast_add_soft_renderbuffers(&rfb->base,
	    GL_FALSE, /* color */
	    swDepth,
	    swStencil,
	    swAccum,
	    swAlpha,
	    GL_FALSE /* aux */);
    driDrawPriv->driverPrivate = (void *) rfb;

    return (driDrawPriv->driverPrivate != NULL);
}


static void radeon_cleanup_renderbuffers(struct radeon_framebuffer *rfb)
{
	struct radeon_renderbuffer *rb;

	rb = rfb->color_rb[0];
	if (rb && rb->bo) {
		radeon_bo_unref(rb->bo);
		rb->bo = NULL;
	}
	rb = rfb->color_rb[1];
	if (rb && rb->bo) {
		radeon_bo_unref(rb->bo);
		rb->bo = NULL;
	}
	rb = radeon_get_renderbuffer(&rfb->base, BUFFER_DEPTH);
	if (rb && rb->bo) {
		radeon_bo_unref(rb->bo);
		rb->bo = NULL;
	}
}

void
radeonDestroyBuffer(__DRIdrawable *driDrawPriv)
{
    struct radeon_framebuffer *rfb;
    if (!driDrawPriv)
	return;

    rfb = (void*)driDrawPriv->driverPrivate;
    if (!rfb)
	return;
    radeon_cleanup_renderbuffers(rfb);
    _mesa_reference_framebuffer((struct gl_framebuffer **)(&(driDrawPriv->driverPrivate)), NULL);
}

#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

/**
 * This is the driver specific part of the createNewScreen entry point.
 * Called when using DRI2.
 *
 * \return the struct gl_config supported by this driver
 */
static const
__DRIconfig **radeonInitScreen2(__DRIscreen *psp)
{
   GLenum fb_format[3];
   GLenum fb_type[3];
   /* GLX_SWAP_COPY_OML is only supported because the Intel driver doesn't
    * support pageflipping at all.
    */
   static const GLenum back_buffer_modes[] = {
     GLX_NONE, GLX_SWAP_UNDEFINED_OML, /*, GLX_SWAP_COPY_OML*/
   };
   uint8_t depth_bits[4], stencil_bits[4], msaa_samples_array[1];
   int color;
   __DRIconfig **configs = NULL;

   if (!radeonInitDriver(psp)) {
       return NULL;
    }
   depth_bits[0] = 0;
   stencil_bits[0] = 0;
   depth_bits[1] = 16;
   stencil_bits[1] = 0;
   depth_bits[2] = 24;
   stencil_bits[2] = 0;
   depth_bits[3] = 24;
   stencil_bits[3] = 8;

   msaa_samples_array[0] = 0;

   fb_format[0] = GL_RGB;
   fb_type[0] = GL_UNSIGNED_SHORT_5_6_5;

   fb_format[1] = GL_BGR;
   fb_type[1] = GL_UNSIGNED_INT_8_8_8_8_REV;

   fb_format[2] = GL_BGRA;
   fb_type[2] = GL_UNSIGNED_INT_8_8_8_8_REV;

   for (color = 0; color < ARRAY_SIZE(fb_format); color++) {
      __DRIconfig **new_configs;

      new_configs = driCreateConfigs(fb_format[color], fb_type[color],
				     depth_bits,
				     stencil_bits,
				     ARRAY_SIZE(depth_bits),
				     back_buffer_modes,
				     ARRAY_SIZE(back_buffer_modes),
				     msaa_samples_array,
				     ARRAY_SIZE(msaa_samples_array),
				     GL_TRUE);
      configs = driConcatConfigs(configs, new_configs);
   }

   if (configs == NULL) {
      fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__,
              __LINE__);
      return NULL;
   }

   return (const __DRIconfig **)configs;
}

const struct __DriverAPIRec driDriverAPI = {
   .InitScreen      = radeonInitScreen2,
   .DestroyScreen   = radeonDestroyScreen,
#if defined(RADEON_R200)
   .CreateContext   = r200CreateContext,
   .DestroyContext  = r200DestroyContext,
#else
   .CreateContext   = r100CreateContext,
   .DestroyContext  = radeonDestroyContext,
#endif
   .CreateBuffer    = radeonCreateBuffer,
   .DestroyBuffer   = radeonDestroyBuffer,
   .MakeCurrent     = radeonMakeCurrent,
   .UnbindContext   = radeonUnbindContext,
};

/* This is the table of extensions that the loader will dlsym() for. */
PUBLIC const __DRIextension *__driDriverExtensions[] = {
    &driCoreExtension.base,
    &driDRI2Extension.base,
    NULL
};
