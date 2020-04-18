
#include "main/glheader.h"
#include "main/macros.h"
#include "main/mfeatures.h"
#include "main/mtypes.h"
#include "main/enums.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "main/formats.h"
#include "main/pbo.h"
#include "main/renderbuffer.h"
#include "main/texcompress.h"
#include "main/texgetimage.h"
#include "main/texobj.h"
#include "main/teximage.h"
#include "main/texstore.h"

#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_buffer_objects.h"
#include "intel_batchbuffer.h"
#include "intel_tex.h"
#include "intel_blit.h"
#include "intel_fbo.h"
#include "intel_span.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

/* Work back from the specified level of the image to the baselevel and create a
 * miptree of that size.
 */
struct intel_mipmap_tree *
intel_miptree_create_for_teximage(struct intel_context *intel,
				  struct intel_texture_object *intelObj,
				  struct intel_texture_image *intelImage,
				  bool expect_accelerated_upload)
{
   GLuint firstLevel;
   GLuint lastLevel;
   int width, height, depth;
   GLuint i;

   intel_miptree_get_dimensions_for_image(&intelImage->base.Base,
                                          &width, &height, &depth);

   DBG("%s\n", __FUNCTION__);

   if (intelImage->base.Base.Level > intelObj->base.BaseLevel &&
       (width == 1 ||
        (intelObj->base.Target != GL_TEXTURE_1D && height == 1) ||
        (intelObj->base.Target == GL_TEXTURE_3D && depth == 1))) {
      /* For this combination, we're at some lower mipmap level and
       * some important dimension is 1.  We can't extrapolate up to a
       * likely base level width/height/depth for a full mipmap stack
       * from this info, so just allocate this one level.
       */
      firstLevel = intelImage->base.Base.Level;
      lastLevel = intelImage->base.Base.Level;
   } else {
      /* If this image disrespects BaseLevel, allocate from level zero.
       * Usually BaseLevel == 0, so it's unlikely to happen.
       */
      if (intelImage->base.Base.Level < intelObj->base.BaseLevel)
	 firstLevel = 0;
      else
	 firstLevel = intelObj->base.BaseLevel;

      /* Figure out image dimensions at start level. */
      for (i = intelImage->base.Base.Level; i > firstLevel; i--) {
	 width <<= 1;
	 if (height != 1)
	    height <<= 1;
	 if (depth != 1)
	    depth <<= 1;
      }

      /* Guess a reasonable value for lastLevel.  This is probably going
       * to be wrong fairly often and might mean that we have to look at
       * resizable buffers, or require that buffers implement lazy
       * pagetable arrangements.
       */
      if ((intelObj->base.Sampler.MinFilter == GL_NEAREST ||
	   intelObj->base.Sampler.MinFilter == GL_LINEAR) &&
	  intelImage->base.Base.Level == firstLevel &&
	  (intel->gen < 4 || firstLevel == 0)) {
	 lastLevel = firstLevel;
      } else if (intelObj->base.Target == GL_TEXTURE_EXTERNAL_OES) {
	 lastLevel = firstLevel;
      } else {
	 lastLevel = firstLevel + _mesa_logbase2(MAX2(MAX2(width, height), depth));
      }
   }

   return intel_miptree_create(intel,
			       intelObj->base.Target,
			       intelImage->base.Base.TexFormat,
			       firstLevel,
			       lastLevel,
			       width,
			       height,
			       depth,
			       expect_accelerated_upload,
                               0 /* num_samples */,
                               INTEL_MSAA_LAYOUT_NONE);
}

/* There are actually quite a few combinations this will work for,
 * more than what I've listed here.
 */
static bool
check_pbo_format(GLenum format, GLenum type,
                 gl_format mesa_format)
{
   switch (mesa_format) {
   case MESA_FORMAT_ARGB8888:
      return (format == GL_BGRA && (type == GL_UNSIGNED_BYTE ||
				    type == GL_UNSIGNED_INT_8_8_8_8_REV));
   case MESA_FORMAT_RGB565:
      return (format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5);
   case MESA_FORMAT_L8:
      return (format == GL_LUMINANCE && type == GL_UNSIGNED_BYTE);
   case MESA_FORMAT_YCBCR:
      return (type == GL_UNSIGNED_SHORT_8_8_MESA || type == GL_UNSIGNED_BYTE);
   default:
      return false;
   }
}


/* XXX: Do this for TexSubImage also:
 */
static bool
try_pbo_upload(struct gl_context *ctx,
               struct gl_texture_image *image,
               const struct gl_pixelstore_attrib *unpack,
	       GLenum format, GLenum type, const void *pixels)
{
   struct intel_texture_image *intelImage = intel_texture_image(image);
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *pbo = intel_buffer_object(unpack->BufferObj);
   GLuint src_offset, src_stride;
   GLuint dst_x, dst_y, dst_stride;
   drm_intel_bo *dst_buffer, *src_buffer;

   if (!_mesa_is_bufferobj(unpack->BufferObj))
      return false;

   DBG("trying pbo upload\n");

   if (intel->ctx._ImageTransferState ||
       unpack->SkipPixels || unpack->SkipRows) {
      DBG("%s: image transfer\n", __FUNCTION__);
      return false;
   }

   if (!check_pbo_format(format, type, intelImage->base.Base.TexFormat)) {
      DBG("%s: format mismatch (upload to %s with format 0x%x, type 0x%x)\n",
	  __FUNCTION__, _mesa_get_format_name(intelImage->base.Base.TexFormat),
	  format, type);
      return false;
   }

   ctx->Driver.AllocTextureImageBuffer(ctx, image);

   if (!intelImage->mt) {
      DBG("%s: no miptree\n", __FUNCTION__);
      return false;
   }

   dst_buffer = intelImage->mt->region->bo;
   src_buffer = intel_bufferobj_source(intel, pbo, 64, &src_offset);
   /* note: potential 64-bit ptr to 32-bit int cast */
   src_offset += (GLuint) (unsigned long) pixels;

   if (unpack->RowLength > 0)
      src_stride = unpack->RowLength;
   else
      src_stride = image->Width;

   intel_miptree_get_image_offset(intelImage->mt, intelImage->base.Base.Level,
				  intelImage->base.Base.Face, 0,
				  &dst_x, &dst_y);

   dst_stride = intelImage->mt->region->pitch;

   if (!intelEmitCopyBlit(intel,
			  intelImage->mt->cpp,
			  src_stride, src_buffer,
			  src_offset, false,
			  dst_stride, dst_buffer, 0,
			  intelImage->mt->region->tiling,
			  0, 0, dst_x, dst_y, image->Width, image->Height,
			  GL_COPY)) {
      DBG("%s: blit failed\n", __FUNCTION__);
      return false;
   }

   DBG("%s: success\n", __FUNCTION__);
   return true;
}

static void
intelTexImage(struct gl_context * ctx,
              GLuint dims,
              struct gl_texture_image *texImage,
              GLenum format, GLenum type, const void *pixels,
              const struct gl_pixelstore_attrib *unpack)
{
   DBG("%s target %s level %d %dx%dx%d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(texImage->TexObject->Target),
       texImage->Level, texImage->Width, texImage->Height, texImage->Depth);

   /* Attempt to use the blitter for PBO image uploads.
    */
   if (dims <= 2 &&
       try_pbo_upload(ctx, texImage, unpack, format, type, pixels)) {
      return;
   }

   DBG("%s: upload image %dx%dx%d pixels %p\n",
       __FUNCTION__, texImage->Width, texImage->Height, texImage->Depth,
       pixels);

   _mesa_store_teximage(ctx, dims, texImage,
                        format, type, pixels, unpack);
}


/**
 * Binds a region to a texture image, like it was uploaded by glTexImage2D().
 *
 * Used for GLX_EXT_texture_from_pixmap and EGL image extensions,
 */
static void
intel_set_texture_image_region(struct gl_context *ctx,
			       struct gl_texture_image *image,
			       struct intel_region *region,
			       GLenum target,
			       GLenum internalFormat,
			       gl_format format,
                               uint32_t offset)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intel_image = intel_texture_image(image);
   struct gl_texture_object *texobj = image->TexObject;
   struct intel_texture_object *intel_texobj = intel_texture_object(texobj);

   _mesa_init_teximage_fields(&intel->ctx, image,
			      region->width, region->height, 1,
			      0, internalFormat, format);

   ctx->Driver.FreeTextureImageBuffer(ctx, image);

   intel_image->mt = intel_miptree_create_for_region(intel, target,
						     image->TexFormat,
						     region);
   if (intel_image->mt == NULL)
       return;

   intel_image->mt->offset = offset;
   intel_image->base.RowStride = region->pitch;

   /* Immediately validate the image to the object. */
   intel_miptree_reference(&intel_texobj->mt, intel_image->mt);
}

void
intelSetTexBuffer2(__DRIcontext *pDRICtx, GLint target,
		   GLint texture_format,
		   __DRIdrawable *dPriv)
{
   struct gl_framebuffer *fb = dPriv->driverPrivate;
   struct intel_context *intel = pDRICtx->driverPrivate;
   struct gl_context *ctx = &intel->ctx;
   struct intel_texture_object *intelObj;
   struct intel_renderbuffer *rb;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   int level = 0, internalFormat = 0;
   gl_format texFormat = MESA_FORMAT_NONE;

   texObj = _mesa_get_current_tex_object(ctx, target);
   intelObj = intel_texture_object(texObj);

   if (!intelObj)
      return;

   if (dPriv->lastStamp != dPriv->dri2.stamp ||
       !pDRICtx->driScreenPriv->dri2.useInvalidate)
      intel_update_renderbuffers(pDRICtx, dPriv);

   rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
   /* If the region isn't set, then intel_update_renderbuffers was unable
    * to get the buffers for the drawable.
    */
   if (!rb || !rb->mt)
      return;

   if (rb->mt->cpp == 4) {
      if (texture_format == __DRI_TEXTURE_FORMAT_RGB) {
         internalFormat = GL_RGB;
         texFormat = MESA_FORMAT_XRGB8888;
      }
      else {
         internalFormat = GL_RGBA;
         texFormat = MESA_FORMAT_ARGB8888;
      }
   } else if (rb->mt->cpp == 2) {
      internalFormat = GL_RGB;
      texFormat = MESA_FORMAT_RGB565;
   }

   _mesa_lock_texture(&intel->ctx, texObj);
   texImage = _mesa_get_tex_image(ctx, texObj, target, level);
   intel_set_texture_image_region(ctx, texImage, rb->mt->region, target,
				  internalFormat, texFormat, 0);
   _mesa_unlock_texture(&intel->ctx, texObj);
}

void
intelSetTexBuffer(__DRIcontext *pDRICtx, GLint target, __DRIdrawable *dPriv)
{
   /* The old interface didn't have the format argument, so copy our
    * implementation's behavior at the time.
    */
   intelSetTexBuffer2(pDRICtx, target, __DRI_TEXTURE_FORMAT_RGBA, dPriv);
}

#if FEATURE_OES_EGL_image
static void
intel_image_target_texture_2d(struct gl_context *ctx, GLenum target,
			      struct gl_texture_object *texObj,
			      struct gl_texture_image *texImage,
			      GLeglImageOES image_handle)
{
   struct intel_context *intel = intel_context(ctx);
   __DRIscreen *screen;
   __DRIimage *image;

   screen = intel->intelScreen->driScrnPriv;
   image = screen->dri2.image->lookupEGLImage(screen, image_handle,
					      screen->loaderPrivate);
   if (image == NULL)
      return;

   intel_set_texture_image_region(ctx, texImage, image->region,
				  target, image->internal_format,
                                  image->format, image->offset);
}
#endif

void
intelInitTextureImageFuncs(struct dd_function_table *functions)
{
   functions->TexImage = intelTexImage;

#if FEATURE_OES_EGL_image
   functions->EGLImageTargetTexture2D = intel_image_target_texture_2d;
#endif
}
