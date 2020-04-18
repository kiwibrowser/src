/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "main/enums.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/mfeatures.h"
#include "main/mtypes.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/context.h"
#include "main/teximage.h"
#include "main/image.h"

#include "swrast/swrast.h"
#include "drivers/common/meta.h"

#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_buffers.h"
#include "intel_blit.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_tex.h"
#include "intel_span.h"
#ifndef I915
#include "brw_context.h"
#endif

#define FILE_DEBUG_FLAG DEBUG_FBO

static struct gl_renderbuffer *
intel_new_renderbuffer(struct gl_context * ctx, GLuint name);

struct intel_region*
intel_get_rb_region(struct gl_framebuffer *fb, GLuint attIndex)
{
   struct intel_renderbuffer *irb = intel_get_renderbuffer(fb, attIndex);
   if (irb && irb->mt) {
      if (attIndex == BUFFER_STENCIL && irb->mt->stencil_mt)
	 return irb->mt->stencil_mt->region;
      else
	 return irb->mt->region;
   } else
      return NULL;
}

/**
 * Create a new framebuffer object.
 */
static struct gl_framebuffer *
intel_new_framebuffer(struct gl_context * ctx, GLuint name)
{
   /* Only drawable state in intel_framebuffer at this time, just use Mesa's
    * class
    */
   return _mesa_new_framebuffer(ctx, name);
}


/** Called by gl_renderbuffer::Delete() */
static void
intel_delete_renderbuffer(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   ASSERT(irb);

   intel_miptree_release(&irb->mt);

   _mesa_delete_renderbuffer(ctx, rb);
}

/**
 * \see dd_function_table::MapRenderbuffer
 */
static void
intel_map_renderbuffer(struct gl_context *ctx,
		       struct gl_renderbuffer *rb,
		       GLuint x, GLuint y, GLuint w, GLuint h,
		       GLbitfield mode,
		       GLubyte **out_map,
		       GLint *out_stride)
{
   struct intel_context *intel = intel_context(ctx);
   struct swrast_renderbuffer *srb = (struct swrast_renderbuffer *)rb;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   void *map;
   int stride;

   if (srb->Buffer) {
      /* this is a malloc'd renderbuffer (accum buffer), not an irb */
      GLint bpp = _mesa_get_format_bytes(rb->Format);
      GLint rowStride = srb->RowStride;
      *out_map = (GLubyte *) srb->Buffer + y * rowStride + x * bpp;
      *out_stride = rowStride;
      return;
   }

   /* We sometimes get called with this by our intel_span.c usage. */
   if (!irb->mt) {
      *out_map = NULL;
      *out_stride = 0;
      return;
   }

   /* For a window-system renderbuffer, we need to flip the mapping we receive
    * upside-down.  So we need to ask for a rectangle on flipped vertically, and
    * we then return a pointer to the bottom of it with a negative stride.
    */
   if (rb->Name == 0) {
      y = rb->Height - y - h;
   }

   intel_miptree_map(intel, irb->mt, irb->mt_level, irb->mt_layer,
		     x, y, w, h, mode, &map, &stride);

   if (rb->Name == 0) {
      map += (h - 1) * stride;
      stride = -stride;
   }

   DBG("%s: rb %d (%s) mt mapped: (%d, %d) (%dx%d) -> %p/%d\n",
       __FUNCTION__, rb->Name, _mesa_get_format_name(rb->Format),
       x, y, w, h, map, stride);

   *out_map = map;
   *out_stride = stride;
}

/**
 * \see dd_function_table::UnmapRenderbuffer
 */
static void
intel_unmap_renderbuffer(struct gl_context *ctx,
			 struct gl_renderbuffer *rb)
{
   struct intel_context *intel = intel_context(ctx);
   struct swrast_renderbuffer *srb = (struct swrast_renderbuffer *)rb;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   DBG("%s: rb %d (%s)\n", __FUNCTION__,
       rb->Name, _mesa_get_format_name(rb->Format));

   if (srb->Buffer) {
      /* this is a malloc'd renderbuffer (accum buffer) */
      /* nothing to do */
      return;
   }

   intel_miptree_unmap(intel, irb->mt, irb->mt_level, irb->mt_layer);
}


/**
 * Round up the requested multisample count to the next supported sample size.
 */
unsigned
intel_quantize_num_samples(struct intel_screen *intel, unsigned num_samples)
{
   switch (intel->gen) {
   case 6:
      /* Gen6 supports only 4x multisampling. */
      if (num_samples > 0)
         return 4;
      else
         return 0;
   case 7:
      /* Gen7 supports 4x and 8x multisampling. */
      if (num_samples > 4)
         return 8;
      else if (num_samples > 0)
         return 4;
      else
         return 0;
      return 0;
   default:
      /* MSAA unsupported.  However, a careful reading of
       * EXT_framebuffer_multisample reveals that we need to permit
       * num_samples to be 1 (since num_samples is permitted to be as high as
       * GL_MAX_SAMPLES, and GL_MAX_SAMPLES must be at least 1).  Since
       * platforms before Gen6 don't support MSAA, this is safe, because
       * multisampling won't happen anyhow.
       */
      if (num_samples > 0)
         return 1;
      return 0;
   }
}


/**
 * Called via glRenderbufferStorageEXT() to set the format and allocate
 * storage for a user-created renderbuffer.
 */
GLboolean
intel_alloc_renderbuffer_storage(struct gl_context * ctx, struct gl_renderbuffer *rb,
                                 GLenum internalFormat,
                                 GLuint width, GLuint height)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_screen *screen = intel->intelScreen;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   rb->NumSamples = intel_quantize_num_samples(screen, rb->NumSamples);

   switch (internalFormat) {
   default:
      /* Use the same format-choice logic as for textures.
       * Renderbuffers aren't any different from textures for us,
       * except they're less useful because you can't texture with
       * them.
       */
      rb->Format = intel->ctx.Driver.ChooseTextureFormat(ctx, GL_TEXTURE_2D,
							 internalFormat,
							 GL_NONE, GL_NONE);
      break;
   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      /* These aren't actual texture formats, so force them here. */
      if (intel->has_separate_stencil) {
	 rb->Format = MESA_FORMAT_S8;
      } else {
	 assert(!intel->must_use_separate_stencil);
	 rb->Format = MESA_FORMAT_S8_Z24;
      }
      break;
   }

   rb->Width = width;
   rb->Height = height;
   rb->_BaseFormat = _mesa_base_fbo_format(ctx, internalFormat);

   intel_miptree_release(&irb->mt);

   DBG("%s: %s: %s (%dx%d)\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(internalFormat),
       _mesa_get_format_name(rb->Format), width, height);

   if (width == 0 || height == 0)
      return true;

   irb->mt = intel_miptree_create_for_renderbuffer(intel, rb->Format,
						   width, height,
                                                   rb->NumSamples);
   if (!irb->mt)
      return false;

   return true;
}


#if FEATURE_OES_EGL_image
static void
intel_image_target_renderbuffer_storage(struct gl_context *ctx,
					struct gl_renderbuffer *rb,
					void *image_handle)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_renderbuffer *irb;
   __DRIscreen *screen;
   __DRIimage *image;

   screen = intel->intelScreen->driScrnPriv;
   image = screen->dri2.image->lookupEGLImage(screen, image_handle,
					      screen->loaderPrivate);
   if (image == NULL)
      return;

   /* __DRIimage is opaque to the core so it has to be checked here */
   switch (image->format) {
   case MESA_FORMAT_RGBA8888_REV:
      _mesa_error(&intel->ctx, GL_INVALID_OPERATION,
            "glEGLImageTargetRenderbufferStorage(unsupported image format");
      return;
      break;
   default:
      break;
   }

   irb = intel_renderbuffer(rb);
   intel_miptree_release(&irb->mt);
   irb->mt = intel_miptree_create_for_region(intel,
                                             GL_TEXTURE_2D,
                                             image->format,
                                             image->region);
   if (!irb->mt)
      return;

   rb->InternalFormat = image->internal_format;
   rb->Width = image->region->width;
   rb->Height = image->region->height;
   rb->Format = image->format;
   rb->_BaseFormat = _mesa_base_fbo_format(&intel->ctx,
					   image->internal_format);
}
#endif

/**
 * Called for each hardware renderbuffer when a _window_ is resized.
 * Just update fields.
 * Not used for user-created renderbuffers!
 */
static GLboolean
intel_alloc_window_storage(struct gl_context * ctx, struct gl_renderbuffer *rb,
                           GLenum internalFormat, GLuint width, GLuint height)
{
   ASSERT(rb->Name == 0);
   rb->Width = width;
   rb->Height = height;
   rb->InternalFormat = internalFormat;

   return true;
}


static void
intel_resize_buffers(struct gl_context *ctx, struct gl_framebuffer *fb,
		     GLuint width, GLuint height)
{
   int i;

   _mesa_resize_framebuffer(ctx, fb, width, height);

   fb->Initialized = true; /* XXX remove someday */

   if (_mesa_is_user_fbo(fb)) {
      return;
   }


   /* Make sure all window system renderbuffers are up to date */
   for (i = BUFFER_FRONT_LEFT; i <= BUFFER_BACK_RIGHT; i++) {
      struct gl_renderbuffer *rb = fb->Attachment[i].Renderbuffer;

      /* only resize if size is changing */
      if (rb && (rb->Width != width || rb->Height != height)) {
	 rb->AllocStorage(ctx, rb, rb->InternalFormat, width, height);
      }
   }
}


/** Dummy function for gl_renderbuffer::AllocStorage() */
static GLboolean
intel_nop_alloc_storage(struct gl_context * ctx, struct gl_renderbuffer *rb,
                        GLenum internalFormat, GLuint width, GLuint height)
{
   _mesa_problem(ctx, "intel_op_alloc_storage should never be called.");
   return false;
}

/**
 * Create a new intel_renderbuffer which corresponds to an on-screen window,
 * not a user-created renderbuffer.
 *
 * \param num_samples must be quantized.
 */
struct intel_renderbuffer *
intel_create_renderbuffer(gl_format format, unsigned num_samples)
{
   struct intel_renderbuffer *irb;
   struct gl_renderbuffer *rb;

   GET_CURRENT_CONTEXT(ctx);

   irb = CALLOC_STRUCT(intel_renderbuffer);
   if (!irb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "creating renderbuffer");
      return NULL;
   }

   rb = &irb->Base.Base;

   _mesa_init_renderbuffer(rb, 0);
   rb->ClassID = INTEL_RB_CLASS;
   rb->_BaseFormat = _mesa_get_format_base_format(format);
   rb->Format = format;
   rb->InternalFormat = rb->_BaseFormat;
   rb->NumSamples = num_samples;

   /* intel-specific methods */
   rb->Delete = intel_delete_renderbuffer;
   rb->AllocStorage = intel_alloc_window_storage;

   return irb;
}

/**
 * Private window-system buffers (as opposed to ones shared with the display
 * server created with intel_create_renderbuffer()) are most similar in their
 * handling to user-created renderbuffers, but they have a resize handler that
 * may be called at intel_update_renderbuffers() time.
 *
 * \param num_samples must be quantized.
 */
struct intel_renderbuffer *
intel_create_private_renderbuffer(gl_format format, unsigned num_samples)
{
   struct intel_renderbuffer *irb;

   irb = intel_create_renderbuffer(format, num_samples);
   irb->Base.Base.AllocStorage = intel_alloc_renderbuffer_storage;

   return irb;
}

/**
 * Create a new renderbuffer object.
 * Typically called via glBindRenderbufferEXT().
 */
static struct gl_renderbuffer *
intel_new_renderbuffer(struct gl_context * ctx, GLuint name)
{
   /*struct intel_context *intel = intel_context(ctx); */
   struct intel_renderbuffer *irb;
   struct gl_renderbuffer *rb;

   irb = CALLOC_STRUCT(intel_renderbuffer);
   if (!irb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "creating renderbuffer");
      return NULL;
   }

   rb = &irb->Base.Base;

   _mesa_init_renderbuffer(rb, name);
   rb->ClassID = INTEL_RB_CLASS;

   /* intel-specific methods */
   rb->Delete = intel_delete_renderbuffer;
   rb->AllocStorage = intel_alloc_renderbuffer_storage;
   /* span routines set in alloc_storage function */

   return rb;
}


/**
 * Called via glBindFramebufferEXT().
 */
static void
intel_bind_framebuffer(struct gl_context * ctx, GLenum target,
                       struct gl_framebuffer *fb, struct gl_framebuffer *fbread)
{
   if (target == GL_FRAMEBUFFER_EXT || target == GL_DRAW_FRAMEBUFFER_EXT) {
      intel_draw_buffer(ctx);
   }
   else {
      /* don't need to do anything if target == GL_READ_FRAMEBUFFER_EXT */
   }
}


/**
 * Called via glFramebufferRenderbufferEXT().
 */
static void
intel_framebuffer_renderbuffer(struct gl_context * ctx,
                               struct gl_framebuffer *fb,
                               GLenum attachment, struct gl_renderbuffer *rb)
{
   DBG("Intel FramebufferRenderbuffer %u %u\n", fb->Name, rb ? rb->Name : 0);

   _mesa_framebuffer_renderbuffer(ctx, fb, attachment, rb);
   intel_draw_buffer(ctx);
}

/**
 * \par Special case for separate stencil
 *
 *     When wrapping a depthstencil texture that uses separate stencil, this
 *     function is recursively called twice: once to create \c
 *     irb->wrapped_depth and again to create \c irb->wrapped_stencil.  On the
 *     call to create \c irb->wrapped_depth, the \c format and \c
 *     internal_format parameters do not match \c mt->format. In that case, \c
 *     mt->format is MESA_FORMAT_S8_Z24 and \c format is \c
 *     MESA_FORMAT_X8_Z24.
 *
 * @return true on success
 */

static bool
intel_renderbuffer_update_wrapper(struct intel_context *intel,
                                  struct intel_renderbuffer *irb,
				  struct gl_texture_image *image,
                                  uint32_t layer)
{
   struct gl_renderbuffer *rb = &irb->Base.Base;
   struct intel_texture_image *intel_image = intel_texture_image(image);
   struct intel_mipmap_tree *mt = intel_image->mt;
   int level = image->Level;

   rb->Format = image->TexFormat;
   rb->InternalFormat = image->InternalFormat;
   rb->_BaseFormat = image->_BaseFormat;
   rb->Width = mt->level[level].width;
   rb->Height = mt->level[level].height;

   rb->Delete = intel_delete_renderbuffer;
   rb->AllocStorage = intel_nop_alloc_storage;

   intel_miptree_check_level_layer(mt, level, layer);
   irb->mt_level = level;
   irb->mt_layer = layer;

   intel_miptree_reference(&irb->mt, mt);

   intel_renderbuffer_set_draw_offset(irb);

   if (mt->hiz_mt == NULL &&
       intel->vtbl.is_hiz_depth_format(intel, rb->Format)) {
      intel_miptree_alloc_hiz(intel, mt, 0 /* num_samples */);
      if (!mt->hiz_mt)
	 return false;
   }

   return true;
}

void
intel_renderbuffer_set_draw_offset(struct intel_renderbuffer *irb)
{
   unsigned int dst_x, dst_y;

   /* compute offset of the particular 2D image within the texture region */
   intel_miptree_get_image_offset(irb->mt,
				  irb->mt_level,
				  0, /* face, which we ignore */
				  irb->mt_layer,
				  &dst_x, &dst_y);

   irb->draw_x = dst_x;
   irb->draw_y = dst_y;
}

/**
 * Rendering to tiled buffers requires that the base address of the
 * buffer be aligned to a page boundary.  We generally render to
 * textures by pointing the surface at the mipmap image level, which
 * may not be aligned to a tile boundary.
 *
 * This function returns an appropriately-aligned base offset
 * according to the tiling restrictions, plus any required x/y offset
 * from there.
 */
uint32_t
intel_renderbuffer_tile_offsets(struct intel_renderbuffer *irb,
				uint32_t *tile_x,
				uint32_t *tile_y)
{
   struct intel_region *region = irb->mt->region;
   uint32_t mask_x, mask_y;

   intel_region_get_tile_masks(region, &mask_x, &mask_y, false);

   *tile_x = irb->draw_x & mask_x;
   *tile_y = irb->draw_y & mask_y;
   return intel_region_get_aligned_offset(region, irb->draw_x & ~mask_x,
                                          irb->draw_y & ~mask_y, false);
}

/**
 * Called by glFramebufferTexture[123]DEXT() (and other places) to
 * prepare for rendering into texture memory.  This might be called
 * many times to choose different texture levels, cube faces, etc
 * before intel_finish_render_texture() is ever called.
 */
static void
intel_render_texture(struct gl_context * ctx,
                     struct gl_framebuffer *fb,
                     struct gl_renderbuffer_attachment *att)
{
   struct intel_context *intel = intel_context(ctx);
   struct gl_texture_image *image = _mesa_get_attachment_teximage(att);
   struct intel_renderbuffer *irb = intel_renderbuffer(att->Renderbuffer);
   struct intel_texture_image *intel_image = intel_texture_image(image);
   struct intel_mipmap_tree *mt = intel_image->mt;
   int layer;

   (void) fb;

   if (att->CubeMapFace > 0) {
      assert(att->Zoffset == 0);
      layer = att->CubeMapFace;
   } else {
      layer = att->Zoffset;
   }

   if (!intel_image->mt) {
      /* Fallback on drawing to a texture that doesn't have a miptree
       * (has a border, width/height 0, etc.)
       */
      _mesa_reference_renderbuffer(&att->Renderbuffer, NULL);
      _swrast_render_texture(ctx, fb, att);
      return;
   }
   else if (!irb) {
      intel_miptree_check_level_layer(mt, att->TextureLevel, layer);

      irb = (struct intel_renderbuffer *)intel_new_renderbuffer(ctx, ~0);

      if (irb) {
         /* bind the wrapper to the attachment point */
         _mesa_reference_renderbuffer(&att->Renderbuffer, &irb->Base.Base);
      }
      else {
         /* fallback to software rendering */
         _swrast_render_texture(ctx, fb, att);
         return;
      }
   }

   if (!intel_renderbuffer_update_wrapper(intel, irb, image, layer)) {
       _mesa_reference_renderbuffer(&att->Renderbuffer, NULL);
       _swrast_render_texture(ctx, fb, att);
       return;
   }

   irb->tex_image = image;

   DBG("Begin render %s texture tex=%u w=%d h=%d refcount=%d\n",
       _mesa_get_format_name(image->TexFormat),
       att->Texture->Name, image->Width, image->Height,
       irb->Base.Base.RefCount);

   /* update drawing region, etc */
   intel_draw_buffer(ctx);
}


/**
 * Called by Mesa when rendering to a texture is done.
 */
static void
intel_finish_render_texture(struct gl_context * ctx,
                            struct gl_renderbuffer_attachment *att)
{
   struct intel_context *intel = intel_context(ctx);
   struct gl_texture_object *tex_obj = att->Texture;
   struct gl_texture_image *image =
      tex_obj->Image[att->CubeMapFace][att->TextureLevel];
   struct intel_renderbuffer *irb = intel_renderbuffer(att->Renderbuffer);

   DBG("Finish render %s texture tex=%u\n",
       _mesa_get_format_name(image->TexFormat), att->Texture->Name);

   if (irb)
      irb->tex_image = NULL;

   /* Since we've (probably) rendered to the texture and will (likely) use
    * it in the texture domain later on in this batchbuffer, flush the
    * batch.  Once again, we wish for a domain tracker in libdrm to cover
    * usage inside of a batchbuffer like GEM does in the kernel.
    */
   intel_batchbuffer_emit_mi_flush(intel);
}

/**
 * Do additional "completeness" testing of a framebuffer object.
 */
static void
intel_validate_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb)
{
   struct intel_context *intel = intel_context(ctx);
   const struct intel_renderbuffer *depthRb =
      intel_get_renderbuffer(fb, BUFFER_DEPTH);
   const struct intel_renderbuffer *stencilRb =
      intel_get_renderbuffer(fb, BUFFER_STENCIL);
   struct intel_mipmap_tree *depth_mt = NULL, *stencil_mt = NULL;
   int i;

   DBG("%s() on fb %p (%s)\n", __FUNCTION__,
       fb, (fb == ctx->DrawBuffer ? "drawbuffer" :
	    (fb == ctx->ReadBuffer ? "readbuffer" : "other buffer")));

   if (depthRb)
      depth_mt = depthRb->mt;
   if (stencilRb) {
      stencil_mt = stencilRb->mt;
      if (stencil_mt->stencil_mt)
	 stencil_mt = stencil_mt->stencil_mt;
   }

   if (depth_mt && stencil_mt) {
      if (depth_mt == stencil_mt) {
	 /* For true packed depth/stencil (not faked on prefers-separate-stencil
	  * hardware) we need to be sure they're the same level/layer, since
	  * we'll be emitting a single packet describing the packed setup.
	  */
	 if (depthRb->mt_level != stencilRb->mt_level ||
	     depthRb->mt_layer != stencilRb->mt_layer) {
	    DBG("depth image level/layer %d/%d != stencil image %d/%d\n",
		depthRb->mt_level,
		depthRb->mt_layer,
		stencilRb->mt_level,
		stencilRb->mt_layer);
	    fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
	 }
      } else {
	 if (!intel->has_separate_stencil) {
	    DBG("separate stencil unsupported\n");
	    fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
	 }
	 if (stencil_mt->format != MESA_FORMAT_S8) {
	    DBG("separate stencil is %s instead of S8\n",
		_mesa_get_format_name(stencil_mt->format));
	    fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
	 }
	 if (intel->gen < 7 && depth_mt->hiz_mt == NULL) {
	    /* Before Gen7, separate depth and stencil buffers can be used
	     * only if HiZ is enabled. From the Sandybridge PRM, Volume 2,
	     * Part 1, Bit 3DSTATE_DEPTH_BUFFER.SeparateStencilBufferEnable:
	     *     [DevSNB]: This field must be set to the same value (enabled
	     *     or disabled) as Hierarchical Depth Buffer Enable.
	     */
	    DBG("separate stencil without HiZ\n");
	    fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED;
	 }
      }
   }

   for (i = 0; i < Elements(fb->Attachment); i++) {
      struct gl_renderbuffer *rb;
      struct intel_renderbuffer *irb;

      if (fb->Attachment[i].Type == GL_NONE)
	 continue;

      /* A supported attachment will have a Renderbuffer set either
       * from being a Renderbuffer or being a texture that got the
       * intel_wrap_texture() treatment.
       */
      rb = fb->Attachment[i].Renderbuffer;
      if (rb == NULL) {
	 DBG("attachment without renderbuffer\n");
	 fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
	 continue;
      }

      if (fb->Attachment[i].Type == GL_TEXTURE) {
	 const struct gl_texture_image *img =
	    _mesa_get_attachment_teximage_const(&fb->Attachment[i]);

	 if (img->Border) {
	    DBG("texture with border\n");
	    fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
	    continue;
	 }
      }

      irb = intel_renderbuffer(rb);
      if (irb == NULL) {
	 DBG("software rendering renderbuffer\n");
	 fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
	 continue;
      }

      if (!intel->vtbl.render_target_supported(intel, rb)) {
	 DBG("Unsupported HW texture/renderbuffer format attached: %s\n",
	     _mesa_get_format_name(intel_rb_format(irb)));
	 fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
      }
   }
}

/**
 * Try to do a glBlitFramebuffer using glCopyTexSubImage2D
 * We can do this when the dst renderbuffer is actually a texture and
 * there is no scaling, mirroring or scissoring.
 *
 * \return new buffer mask indicating the buffers left to blit using the
 *         normal path.
 */
static GLbitfield
intel_blit_framebuffer_copy_tex_sub_image(struct gl_context *ctx,
                                          GLint srcX0, GLint srcY0,
                                          GLint srcX1, GLint srcY1,
                                          GLint dstX0, GLint dstY0,
                                          GLint dstX1, GLint dstY1,
                                          GLbitfield mask, GLenum filter)
{
   if (mask & GL_COLOR_BUFFER_BIT) {
      const struct gl_framebuffer *drawFb = ctx->DrawBuffer;
      const struct gl_framebuffer *readFb = ctx->ReadBuffer;
      const struct gl_renderbuffer_attachment *drawAtt =
         &drawFb->Attachment[drawFb->_ColorDrawBufferIndexes[0]];
      struct intel_renderbuffer *srcRb = 
         intel_renderbuffer(readFb->_ColorReadBuffer);

      /* If the source and destination are the same size with no
         mirroring, the rectangles are within the size of the
         texture and there is no scissor then we can use
         glCopyTexSubimage2D to implement the blit. This will end
         up as a fast hardware blit on some drivers */
      if (srcRb && drawAtt && drawAtt->Texture &&
          srcX0 - srcX1 == dstX0 - dstX1 &&
          srcY0 - srcY1 == dstY0 - dstY1 &&
          srcX1 >= srcX0 &&
          srcY1 >= srcY0 &&
          srcX0 >= 0 && srcX1 <= readFb->Width &&
          srcY0 >= 0 && srcY1 <= readFb->Height &&
          dstX0 >= 0 && dstX1 <= drawFb->Width &&
          dstY0 >= 0 && dstY1 <= drawFb->Height &&
          !ctx->Scissor.Enabled) {
         const struct gl_texture_object *texObj = drawAtt->Texture;
         const GLuint dstLevel = drawAtt->TextureLevel;
         const GLenum target = texObj->Target;

         struct gl_texture_image *texImage =
            _mesa_select_tex_image(ctx, texObj, target, dstLevel);

         if (intel_copy_texsubimage(intel_context(ctx),
                                    intel_texture_image(texImage),
                                    dstX0, dstY0,
                                    srcRb,
                                    srcX0, srcY0,
                                    srcX1 - srcX0, /* width */
                                    srcY1 - srcY0))
            mask &= ~GL_COLOR_BUFFER_BIT;
      }
   }

   return mask;
}

static void
intel_blit_framebuffer(struct gl_context *ctx,
                       GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                       GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                       GLbitfield mask, GLenum filter)
{
   /* Try faster, glCopyTexSubImage2D approach first which uses the BLT. */
   mask = intel_blit_framebuffer_copy_tex_sub_image(ctx,
                                                    srcX0, srcY0, srcX1, srcY1,
                                                    dstX0, dstY0, dstX1, dstY1,
                                                    mask, filter);
   if (mask == 0x0)
      return;

#ifndef I915
   mask = brw_blorp_framebuffer(intel_context(ctx),
                                srcX0, srcY0, srcX1, srcY1,
                                dstX0, dstY0, dstX1, dstY1,
                                mask, filter);
   if (mask == 0x0)
      return;
#endif

   _mesa_meta_BlitFramebuffer(ctx,
                              srcX0, srcY0, srcX1, srcY1,
                              dstX0, dstY0, dstX1, dstY1,
                              mask, filter);
}

/**
 * This is a no-op except on multisample buffers shared with DRI2.
 */
void
intel_renderbuffer_set_needs_downsample(struct intel_renderbuffer *irb)
{
   if (irb->mt && irb->mt->singlesample_mt)
      irb->mt->need_downsample = true;
}

void
intel_renderbuffer_set_needs_hiz_resolve(struct intel_renderbuffer *irb)
{
   if (irb->mt) {
      intel_miptree_slice_set_needs_hiz_resolve(irb->mt,
                                                irb->mt_level,
                                                irb->mt_layer);
   }
}

void
intel_renderbuffer_set_needs_depth_resolve(struct intel_renderbuffer *irb)
{
   if (irb->mt) {
      intel_miptree_slice_set_needs_depth_resolve(irb->mt,
                                                  irb->mt_level,
                                                  irb->mt_layer);
   }
}

bool
intel_renderbuffer_resolve_hiz(struct intel_context *intel,
			       struct intel_renderbuffer *irb)
{
   if (irb->mt)
      return intel_miptree_slice_resolve_hiz(intel,
                                             irb->mt,
                                             irb->mt_level,
                                             irb->mt_layer);

   return false;
}

bool
intel_renderbuffer_resolve_depth(struct intel_context *intel,
				 struct intel_renderbuffer *irb)
{
   if (irb->mt)
      return intel_miptree_slice_resolve_depth(intel,
                                               irb->mt,
                                               irb->mt_level,
                                               irb->mt_layer);

   return false;
}

/**
 * Do one-time context initializations related to GL_EXT_framebuffer_object.
 * Hook in device driver functions.
 */
void
intel_fbo_init(struct intel_context *intel)
{
   intel->ctx.Driver.NewFramebuffer = intel_new_framebuffer;
   intel->ctx.Driver.NewRenderbuffer = intel_new_renderbuffer;
   intel->ctx.Driver.MapRenderbuffer = intel_map_renderbuffer;
   intel->ctx.Driver.UnmapRenderbuffer = intel_unmap_renderbuffer;
   intel->ctx.Driver.BindFramebuffer = intel_bind_framebuffer;
   intel->ctx.Driver.FramebufferRenderbuffer = intel_framebuffer_renderbuffer;
   intel->ctx.Driver.RenderTexture = intel_render_texture;
   intel->ctx.Driver.FinishRenderTexture = intel_finish_render_texture;
   intel->ctx.Driver.ResizeBuffers = intel_resize_buffers;
   intel->ctx.Driver.ValidateFramebuffer = intel_validate_framebuffer;
   intel->ctx.Driver.BlitFramebuffer = intel_blit_framebuffer;

#if FEATURE_OES_EGL_image
   intel->ctx.Driver.EGLImageTargetRenderbufferStorage =
      intel_image_target_renderbuffer_storage;
#endif   
}
