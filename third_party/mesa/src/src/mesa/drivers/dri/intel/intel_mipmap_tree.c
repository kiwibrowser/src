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

#include <GL/gl.h>
#include <GL/internal/dri_interface.h>

#include "intel_batchbuffer.h"
#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_resolve_map.h"
#include "intel_span.h"
#include "intel_tex_layout.h"
#include "intel_tex.h"
#include "intel_blit.h"

#ifndef I915
#include "brw_blorp.h"
#endif

#include "main/enums.h"
#include "main/formats.h"
#include "main/glformats.h"
#include "main/texcompress_etc.h"
#include "main/teximage.h"

#define FILE_DEBUG_FLAG DEBUG_MIPTREE

static GLenum
target_to_target(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
      return GL_TEXTURE_CUBE_MAP_ARB;
   default:
      return target;
   }
}

/**
 * @param for_region Indicates that the caller is
 *        intel_miptree_create_for_region(). If true, then do not create
 *        \c stencil_mt.
 */
static struct intel_mipmap_tree *
intel_miptree_create_internal(struct intel_context *intel,
			      GLenum target,
			      gl_format format,
			      GLuint first_level,
			      GLuint last_level,
			      GLuint width0,
			      GLuint height0,
			      GLuint depth0,
			      bool for_region,
                              GLuint num_samples,
                              enum intel_msaa_layout msaa_layout)
{
   struct intel_mipmap_tree *mt = calloc(sizeof(*mt), 1);
   int compress_byte = 0;

   DBG("%s target %s format %s level %d..%d <-- %p\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target),
       _mesa_get_format_name(format),
       first_level, last_level, mt);

   if (_mesa_is_format_compressed(format))
      compress_byte = intel_compressed_num_bytes(format);

   mt->target = target_to_target(target);
   mt->format = format;
   mt->first_level = first_level;
   mt->last_level = last_level;
   mt->width0 = width0;
   mt->height0 = height0;
   mt->cpp = compress_byte ? compress_byte : _mesa_get_format_bytes(mt->format);
   mt->num_samples = num_samples;
   mt->compressed = compress_byte ? 1 : 0;
   mt->msaa_layout = msaa_layout;
   mt->refcount = 1; 

   /* array_spacing_lod0 is only used for non-IMS MSAA surfaces.  TODO: can we
    * use it elsewhere?
    */
   switch (msaa_layout) {
   case INTEL_MSAA_LAYOUT_NONE:
   case INTEL_MSAA_LAYOUT_IMS:
      mt->array_spacing_lod0 = false;
      break;
   case INTEL_MSAA_LAYOUT_UMS:
   case INTEL_MSAA_LAYOUT_CMS:
      mt->array_spacing_lod0 = true;
      break;
   }

   if (target == GL_TEXTURE_CUBE_MAP) {
      assert(depth0 == 1);
      mt->depth0 = 6;
   } else {
      mt->depth0 = depth0;
   }

   if (!for_region &&
       _mesa_is_depthstencil_format(_mesa_get_format_base_format(format)) &&
       (intel->must_use_separate_stencil ||
	(intel->has_separate_stencil &&
	 intel->vtbl.is_hiz_depth_format(intel, format)))) {
      /* MSAA stencil surfaces always use IMS layout. */
      enum intel_msaa_layout msaa_layout =
         num_samples > 1 ? INTEL_MSAA_LAYOUT_IMS : INTEL_MSAA_LAYOUT_NONE;
      mt->stencil_mt = intel_miptree_create(intel,
                                            mt->target,
                                            MESA_FORMAT_S8,
                                            mt->first_level,
                                            mt->last_level,
                                            mt->width0,
                                            mt->height0,
                                            mt->depth0,
                                            true,
                                            num_samples,
                                            msaa_layout);
      if (!mt->stencil_mt) {
	 intel_miptree_release(&mt);
	 return NULL;
      }

      /* Fix up the Z miptree format for how we're splitting out separate
       * stencil.  Gen7 expects there to be no stencil bits in its depth buffer.
       */
      if (mt->format == MESA_FORMAT_S8_Z24) {
	 mt->format = MESA_FORMAT_X8_Z24;
      } else if (mt->format == MESA_FORMAT_Z32_FLOAT_X24S8) {
	 mt->format = MESA_FORMAT_Z32_FLOAT;
	 mt->cpp = 4;
      } else {
	 _mesa_problem(NULL, "Unknown format %s in separate stencil mt\n",
		       _mesa_get_format_name(mt->format));
      }
   }

   intel_get_texture_alignment_unit(intel, mt->format,
				    &mt->align_w, &mt->align_h);

#ifdef I915
   (void) intel;
   if (intel->is_945)
      i945_miptree_layout(mt);
   else
      i915_miptree_layout(mt);
#else
   brw_miptree_layout(intel, mt);
#endif

   return mt;
}


struct intel_mipmap_tree *
intel_miptree_create(struct intel_context *intel,
		     GLenum target,
		     gl_format format,
		     GLuint first_level,
		     GLuint last_level,
		     GLuint width0,
		     GLuint height0,
		     GLuint depth0,
		     bool expect_accelerated_upload,
                     GLuint num_samples,
                     enum intel_msaa_layout msaa_layout)
{
   struct intel_mipmap_tree *mt;
   uint32_t tiling = I915_TILING_NONE;
   GLenum base_format;
   bool wraps_etc1 = false;
   GLuint total_width, total_height;

   if (format == MESA_FORMAT_ETC1_RGB8) {
      format = MESA_FORMAT_RGBX8888_REV;
      wraps_etc1 = true;
   }

   base_format = _mesa_get_format_base_format(format);

   if (intel->use_texture_tiling && !_mesa_is_format_compressed(format)) {
      if (intel->gen >= 4 &&
	  (base_format == GL_DEPTH_COMPONENT ||
	   base_format == GL_DEPTH_STENCIL_EXT))
	 tiling = I915_TILING_Y;
      else if (msaa_layout != INTEL_MSAA_LAYOUT_NONE) {
         /* From p82 of the Sandy Bridge PRM, dw3[1] of SURFACE_STATE ("Tiled
          * Surface"):
          *
          *   [DevSNB+]: For multi-sample render targets, this field must be
          *   1. MSRTs can only be tiled.
          *
          * Our usual reason for preferring X tiling (fast blits using the
          * blitting engine) doesn't apply to MSAA, since we'll generally be
          * downsampling or upsampling when blitting between the MSAA buffer
          * and another buffer, and the blitting engine doesn't support that.
          * So use Y tiling, since it makes better use of the cache.
          */
         tiling = I915_TILING_Y;
      } else if (width0 >= 64)
	 tiling = I915_TILING_X;
   }

   mt = intel_miptree_create_internal(intel, target, format,
				      first_level, last_level, width0,
				      height0, depth0,
				      false, num_samples, msaa_layout);
   /*
    * pitch == 0 || height == 0  indicates the null texture
    */
   if (!mt || !mt->total_width || !mt->total_height) {
      intel_miptree_release(&mt);
      return NULL;
   }

   total_width = mt->total_width;
   total_height = mt->total_height;

   if (format == MESA_FORMAT_S8) {
      /* The stencil buffer is W tiled. However, we request from the kernel a
       * non-tiled buffer because the GTT is incapable of W fencing.  So round
       * up the width and height to match the size of W tiles (64x64).
       */
      tiling = I915_TILING_NONE;
      total_width = ALIGN(total_width, 64);
      total_height = ALIGN(total_height, 64);
   }

   mt->wraps_etc1 = wraps_etc1;
   mt->region = intel_region_alloc(intel->intelScreen,
				   tiling,
				   mt->cpp,
				   total_width,
				   total_height,
				   expect_accelerated_upload);
   mt->offset = 0;

   if (!mt->region) {
       intel_miptree_release(&mt);
       return NULL;
   }

   return mt;
}


struct intel_mipmap_tree *
intel_miptree_create_for_region(struct intel_context *intel,
				GLenum target,
				gl_format format,
				struct intel_region *region)
{
   struct intel_mipmap_tree *mt;

   mt = intel_miptree_create_internal(intel, target, format,
				      0, 0,
				      region->width, region->height, 1,
				      true, 0 /* num_samples */,
                                      INTEL_MSAA_LAYOUT_NONE);
   if (!mt)
      return mt;

   intel_region_reference(&mt->region, region);

   return mt;
}

/**
 * Determine which MSAA layout should be used by the MSAA surface being
 * created, based on the chip generation and the surface type.
 */
static enum intel_msaa_layout
compute_msaa_layout(struct intel_context *intel, gl_format format)
{
   /* Prior to Gen7, all MSAA surfaces used IMS layout. */
   if (intel->gen < 7)
      return INTEL_MSAA_LAYOUT_IMS;

   /* In Gen7, IMS layout is only used for depth and stencil buffers. */
   switch (_mesa_get_format_base_format(format)) {
   case GL_DEPTH_COMPONENT:
   case GL_STENCIL_INDEX:
   case GL_DEPTH_STENCIL:
      return INTEL_MSAA_LAYOUT_IMS;
   default:
      /* From the Ivy Bridge PRM, Vol4 Part1 p77 ("MCS Enable"):
       *
       *   This field must be set to 0 for all SINT MSRTs when all RT channels
       *   are not written
       *
       * In practice this means that we have to disable MCS for all signed
       * integer MSAA buffers.  The alternative, to disable MCS only when one
       * of the render target channels is disabled, is impractical because it
       * would require converting between CMS and UMS MSAA layouts on the fly,
       * which is expensive.
       */
      if (_mesa_get_format_datatype(format) == GL_INT) {
         /* TODO: is this workaround needed for future chipsets? */
         assert(intel->gen == 7);
         return INTEL_MSAA_LAYOUT_UMS;
      } else {
         return INTEL_MSAA_LAYOUT_CMS;
      }
   }
}

/**
 * For a singlesample DRI2 buffer, this simply wraps the given region with a miptree.
 *
 * For a multisample DRI2 buffer, this wraps the given region with
 * a singlesample miptree, then creates a multisample miptree into which the
 * singlesample miptree is embedded as a child.
 */
struct intel_mipmap_tree*
intel_miptree_create_for_dri2_buffer(struct intel_context *intel,
                                     unsigned dri_attachment,
                                     gl_format format,
                                     uint32_t num_samples,
                                     struct intel_region *region)
{
   struct intel_mipmap_tree *singlesample_mt = NULL;
   struct intel_mipmap_tree *multisample_mt = NULL;
   GLenum base_format = _mesa_get_format_base_format(format);

   /* Only the front and back buffers, which are color buffers, are shared
    * through DRI2.
    */
   assert(dri_attachment == __DRI_BUFFER_BACK_LEFT ||
          dri_attachment == __DRI_BUFFER_FRONT_LEFT ||
          dri_attachment == __DRI_BUFFER_FAKE_FRONT_LEFT);
   assert(base_format == GL_RGB || base_format == GL_RGBA);

   singlesample_mt = intel_miptree_create_for_region(intel, GL_TEXTURE_2D,
                                                     format, region);
   if (!singlesample_mt)
      return NULL;

   if (num_samples == 0)
      return singlesample_mt;

   multisample_mt = intel_miptree_create_for_renderbuffer(intel,
                                                          format,
                                                          region->width,
                                                          region->height,
                                                          num_samples);
   if (!multisample_mt) {
      intel_miptree_release(&singlesample_mt);
      return NULL;
   }

   multisample_mt->singlesample_mt = singlesample_mt;
   multisample_mt->need_downsample = false;

   if (intel->is_front_buffer_rendering &&
       (dri_attachment == __DRI_BUFFER_FRONT_LEFT ||
        dri_attachment == __DRI_BUFFER_FAKE_FRONT_LEFT)) {
      intel_miptree_upsample(intel, multisample_mt);
   }

   return multisample_mt;
}

struct intel_mipmap_tree*
intel_miptree_create_for_renderbuffer(struct intel_context *intel,
                                      gl_format format,
                                      uint32_t width,
                                      uint32_t height,
                                      uint32_t num_samples)
{
   struct intel_mipmap_tree *mt;
   uint32_t depth = 1;
   enum intel_msaa_layout msaa_layout = INTEL_MSAA_LAYOUT_NONE;
   const uint32_t singlesample_width = width;
   const uint32_t singlesample_height = height;
   bool ok;

   if (num_samples > 1) {
      /* Adjust width/height/depth for MSAA */
      msaa_layout = compute_msaa_layout(intel, format);
      if (msaa_layout == INTEL_MSAA_LAYOUT_IMS) {
         /* In the Sandy Bridge PRM, volume 4, part 1, page 31, it says:
          *
          *     "Any of the other messages (sample*, LOD, load4) used with a
          *      (4x) multisampled surface will in-effect sample a surface with
          *      double the height and width as that indicated in the surface
          *      state. Each pixel position on the original-sized surface is
          *      replaced with a 2x2 of samples with the following arrangement:
          *
          *         sample 0 sample 2
          *         sample 1 sample 3"
          *
          * Thus, when sampling from a multisampled texture, it behaves as
          * though the layout in memory for (x,y,sample) is:
          *
          *      (0,0,0) (0,0,2)   (1,0,0) (1,0,2)
          *      (0,0,1) (0,0,3)   (1,0,1) (1,0,3)
          *
          *      (0,1,0) (0,1,2)   (1,1,0) (1,1,2)
          *      (0,1,1) (0,1,3)   (1,1,1) (1,1,3)
          *
          * However, the actual layout of multisampled data in memory is:
          *
          *      (0,0,0) (1,0,0)   (0,0,1) (1,0,1)
          *      (0,1,0) (1,1,0)   (0,1,1) (1,1,1)
          *
          *      (0,0,2) (1,0,2)   (0,0,3) (1,0,3)
          *      (0,1,2) (1,1,2)   (0,1,3) (1,1,3)
          *
          * This pattern repeats for each 2x2 pixel block.
          *
          * As a result, when calculating the size of our 4-sample buffer for
          * an odd width or height, we have to align before scaling up because
          * sample 3 is in that bottom right 2x2 block.
          */
         switch (num_samples) {
         case 4:
            width = ALIGN(width, 2) * 2;
            height = ALIGN(height, 2) * 2;
            break;
         case 8:
            width = ALIGN(width, 2) * 4;
            height = ALIGN(height, 2) * 2;
            break;
         default:
            /* num_samples should already have been quantized to 0, 1, 4, or
             * 8.
             */
            assert(false);
         }
      } else {
         /* Non-interleaved */
         depth = num_samples;
      }
   }

   mt = intel_miptree_create(intel, GL_TEXTURE_2D, format, 0, 0,
			     width, height, depth, true, num_samples,
                             msaa_layout);
   if (!mt)
      goto fail;

   if (intel->vtbl.is_hiz_depth_format(intel, format)) {
      ok = intel_miptree_alloc_hiz(intel, mt, num_samples);
      if (!ok)
         goto fail;
   }

   if (mt->msaa_layout == INTEL_MSAA_LAYOUT_CMS) {
      ok = intel_miptree_alloc_mcs(intel, mt, num_samples);
      if (!ok)
         goto fail;
   }

   mt->singlesample_width0 = singlesample_width;
   mt->singlesample_height0 = singlesample_height;

   return mt;

fail:
   intel_miptree_release(&mt);
   return NULL;
}

void
intel_miptree_reference(struct intel_mipmap_tree **dst,
                        struct intel_mipmap_tree *src)
{
   if (*dst == src)
      return;

   intel_miptree_release(dst);

   if (src) {
      src->refcount++;
      DBG("%s %p refcount now %d\n", __FUNCTION__, src, src->refcount);
   }

   *dst = src;
}


void
intel_miptree_release(struct intel_mipmap_tree **mt)
{
   if (!*mt)
      return;

   DBG("%s %p refcount will be %d\n", __FUNCTION__, *mt, (*mt)->refcount - 1);
   if (--(*mt)->refcount <= 0) {
      GLuint i;

      DBG("%s deleting %p\n", __FUNCTION__, *mt);

      intel_region_release(&((*mt)->region));
      intel_miptree_release(&(*mt)->stencil_mt);
      intel_miptree_release(&(*mt)->hiz_mt);
      intel_miptree_release(&(*mt)->mcs_mt);
      intel_miptree_release(&(*mt)->singlesample_mt);
      intel_resolve_map_clear(&(*mt)->hiz_map);

      for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
	 free((*mt)->level[i].slice);
      }

      free(*mt);
   }
   *mt = NULL;
}

void
intel_miptree_get_dimensions_for_image(struct gl_texture_image *image,
                                       int *width, int *height, int *depth)
{
   switch (image->TexObject->Target) {
   case GL_TEXTURE_1D_ARRAY:
      *width = image->Width;
      *height = 1;
      *depth = image->Height;
      break;
   default:
      *width = image->Width;
      *height = image->Height;
      *depth = image->Depth;
      break;
   }
}

/**
 * Can the image be pulled into a unified mipmap tree?  This mirrors
 * the completeness test in a lot of ways.
 *
 * Not sure whether I want to pass gl_texture_image here.
 */
bool
intel_miptree_match_image(struct intel_mipmap_tree *mt,
                          struct gl_texture_image *image)
{
   struct intel_texture_image *intelImage = intel_texture_image(image);
   GLuint level = intelImage->base.Base.Level;
   int width, height, depth;

   if (target_to_target(image->TexObject->Target) != mt->target)
      return false;

   if (image->TexFormat != mt->format &&
       !(image->TexFormat == MESA_FORMAT_S8_Z24 &&
	 mt->format == MESA_FORMAT_X8_Z24 &&
	 mt->stencil_mt)) {
      return false;
   }

   intel_miptree_get_dimensions_for_image(image, &width, &height, &depth);

   if (mt->target == GL_TEXTURE_CUBE_MAP)
      depth = 6;

   /* Test image dimensions against the base level image adjusted for
    * minification.  This will also catch images not present in the
    * tree, changed targets, etc.
    */
   if (width != mt->level[level].width ||
       height != mt->level[level].height ||
       depth != mt->level[level].depth)
      return false;

   return true;
}


void
intel_miptree_set_level_info(struct intel_mipmap_tree *mt,
			     GLuint level,
			     GLuint x, GLuint y,
			     GLuint w, GLuint h, GLuint d)
{
   mt->level[level].width = w;
   mt->level[level].height = h;
   mt->level[level].depth = d;
   mt->level[level].level_x = x;
   mt->level[level].level_y = y;

   DBG("%s level %d size: %d,%d,%d offset %d,%d\n", __FUNCTION__,
       level, w, h, d, x, y);

   assert(mt->level[level].slice == NULL);

   mt->level[level].slice = calloc(d, sizeof(*mt->level[0].slice));
   mt->level[level].slice[0].x_offset = mt->level[level].level_x;
   mt->level[level].slice[0].y_offset = mt->level[level].level_y;
}


void
intel_miptree_set_image_offset(struct intel_mipmap_tree *mt,
			       GLuint level, GLuint img,
			       GLuint x, GLuint y)
{
   if (img == 0 && level == 0)
      assert(x == 0 && y == 0);

   assert(img < mt->level[level].depth);

   mt->level[level].slice[img].x_offset = mt->level[level].level_x + x;
   mt->level[level].slice[img].y_offset = mt->level[level].level_y + y;

   DBG("%s level %d img %d pos %d,%d\n",
       __FUNCTION__, level, img,
       mt->level[level].slice[img].x_offset,
       mt->level[level].slice[img].y_offset);
}


/**
 * For cube map textures, either the \c face parameter can be used, of course,
 * or the cube face can be interpreted as a depth layer and the \c layer
 * parameter used.
 */
void
intel_miptree_get_image_offset(struct intel_mipmap_tree *mt,
			       GLuint level, GLuint face, GLuint layer,
			       GLuint *x, GLuint *y)
{
   int slice;

   if (face > 0) {
      assert(mt->target == GL_TEXTURE_CUBE_MAP);
      assert(face < 6);
      assert(layer == 0);
      slice = face;
   } else {
      /* This branch may be taken even if the texture target is a cube map. In
       * that case, the caller chose to interpret each cube face as a layer.
       */
      assert(face == 0);
      slice = layer;
   }

   *x = mt->level[level].slice[slice].x_offset;
   *y = mt->level[level].slice[slice].y_offset;
}

static void
intel_miptree_copy_slice(struct intel_context *intel,
			 struct intel_mipmap_tree *dst_mt,
			 struct intel_mipmap_tree *src_mt,
			 int level,
			 int face,
			 int depth)

{
   gl_format format = src_mt->format;
   uint32_t width = src_mt->level[level].width;
   uint32_t height = src_mt->level[level].height;

   assert(depth < src_mt->level[level].depth);

   if (dst_mt->compressed) {
      height = ALIGN(height, dst_mt->align_h) / dst_mt->align_h;
      width = ALIGN(width, dst_mt->align_w);
   }

   uint32_t dst_x, dst_y, src_x, src_y;
   intel_miptree_get_image_offset(dst_mt, level, face, depth,
				  &dst_x, &dst_y);
   intel_miptree_get_image_offset(src_mt, level, face, depth,
				  &src_x, &src_y);

   DBG("validate blit mt %p %d,%d/%d -> mt %p %d,%d/%d (%dx%d)\n",
       src_mt, src_x, src_y, src_mt->region->pitch * src_mt->region->cpp,
       dst_mt, dst_x, dst_y, dst_mt->region->pitch * dst_mt->region->cpp,
       width, height);

   if (!intelEmitCopyBlit(intel,
			  dst_mt->region->cpp,
			  src_mt->region->pitch, src_mt->region->bo,
			  0, src_mt->region->tiling,
			  dst_mt->region->pitch, dst_mt->region->bo,
			  0, dst_mt->region->tiling,
			  src_x, src_y,
			  dst_x, dst_y,
			  width, height,
			  GL_COPY)) {

      fallback_debug("miptree validate blit for %s failed\n",
		     _mesa_get_format_name(format));
      void *dst = intel_region_map(intel, dst_mt->region, GL_MAP_WRITE_BIT);
      void *src = intel_region_map(intel, src_mt->region, GL_MAP_READ_BIT);

      _mesa_copy_rect(dst,
		      dst_mt->cpp,
		      dst_mt->region->pitch,
		      dst_x, dst_y,
		      width, height,
		      src, src_mt->region->pitch,
		      src_x, src_y);

      intel_region_unmap(intel, dst_mt->region);
      intel_region_unmap(intel, src_mt->region);
   }

   if (src_mt->stencil_mt) {
      intel_miptree_copy_slice(intel,
                               dst_mt->stencil_mt, src_mt->stencil_mt,
                               level, face, depth);
   }
}

/**
 * Copies the image's current data to the given miptree, and associates that
 * miptree with the image.
 */
void
intel_miptree_copy_teximage(struct intel_context *intel,
			    struct intel_texture_image *intelImage,
			    struct intel_mipmap_tree *dst_mt)
{
   struct intel_mipmap_tree *src_mt = intelImage->mt;
   int level = intelImage->base.Base.Level;
   int face = intelImage->base.Base.Face;
   GLuint depth = intelImage->base.Base.Depth;

   for (int slice = 0; slice < depth; slice++) {
      intel_miptree_copy_slice(intel, dst_mt, src_mt, level, face, slice);
   }

   intel_miptree_reference(&intelImage->mt, dst_mt);
}

bool
intel_miptree_alloc_mcs(struct intel_context *intel,
                        struct intel_mipmap_tree *mt,
                        GLuint num_samples)
{
   assert(mt->mcs_mt == NULL);
   assert(intel->gen >= 7); /* MCS only used on Gen7+ */

   /* Choose the correct format for the MCS buffer.  All that really matters
    * is that we allocate the right buffer size, since we'll always be
    * accessing this miptree using MCS-specific hardware mechanisms, which
    * infer the correct format based on num_samples.
    */
   gl_format format;
   switch (num_samples) {
   case 4:
      /* 8 bits/pixel are required for MCS data when using 4x MSAA (2 bits for
       * each sample).
       */
      format = MESA_FORMAT_R8;
      break;
   case 8:
      /* 32 bits/pixel are required for MCS data when using 8x MSAA (3 bits
       * for each sample, plus 8 padding bits).
       */
      format = MESA_FORMAT_R_UINT32;
      break;
   default:
      assert(!"Unrecognized sample count in intel_miptree_alloc_mcs");
      break;
   };

   /* From the Ivy Bridge PRM, Vol4 Part1 p76, "MCS Base Address":
    *
    *     "The MCS surface must be stored as Tile Y."
    *
    * We set msaa_format to INTEL_MSAA_LAYOUT_CMS to force
    * intel_miptree_create() to use Y tiling.  msaa_format is otherwise
    * ignored for the MCS miptree.
    */
   mt->mcs_mt = intel_miptree_create(intel,
                                     mt->target,
                                     format,
                                     mt->first_level,
                                     mt->last_level,
                                     mt->width0,
                                     mt->height0,
                                     mt->depth0,
                                     true,
                                     0 /* num_samples */,
                                     INTEL_MSAA_LAYOUT_CMS);

   /* From the Ivy Bridge PRM, Vol 2 Part 1 p326:
    *
    *     When MCS buffer is enabled and bound to MSRT, it is required that it
    *     is cleared prior to any rendering.
    *
    * Since we don't use the MCS buffer for any purpose other than rendering,
    * it makes sense to just clear it immediately upon allocation.
    *
    * Note: the clear value for MCS buffers is all 1's, so we memset to 0xff.
    */
   void *data = intel_region_map(intel, mt->mcs_mt->region, 0);
   memset(data, 0xff, mt->mcs_mt->region->bo->size);
   intel_region_unmap(intel, mt->mcs_mt->region);

   return mt->mcs_mt;
}

bool
intel_miptree_alloc_hiz(struct intel_context *intel,
			struct intel_mipmap_tree *mt,
                        GLuint num_samples)
{
   assert(mt->hiz_mt == NULL);
   /* MSAA HiZ surfaces always use IMS layout. */
   mt->hiz_mt = intel_miptree_create(intel,
                                     mt->target,
                                     MESA_FORMAT_X8_Z24,
                                     mt->first_level,
                                     mt->last_level,
                                     mt->width0,
                                     mt->height0,
                                     mt->depth0,
                                     true,
                                     num_samples,
                                     INTEL_MSAA_LAYOUT_IMS);

   if (!mt->hiz_mt)
      return false;

   /* Mark that all slices need a HiZ resolve. */
   struct intel_resolve_map *head = &mt->hiz_map;
   for (int level = mt->first_level; level <= mt->last_level; ++level) {
      for (int layer = 0; layer < mt->level[level].depth; ++layer) {
	 head->next = malloc(sizeof(*head->next));
	 head->next->prev = head;
	 head->next->next = NULL;
	 head = head->next;

	 head->level = level;
	 head->layer = layer;
	 head->need = GEN6_HIZ_OP_HIZ_RESOLVE;
      }
   }

   return true;
}

void
intel_miptree_slice_set_needs_hiz_resolve(struct intel_mipmap_tree *mt,
					  uint32_t level,
					  uint32_t layer)
{
   intel_miptree_check_level_layer(mt, level, layer);

   if (!mt->hiz_mt)
      return;

   intel_resolve_map_set(&mt->hiz_map,
			 level, layer, GEN6_HIZ_OP_HIZ_RESOLVE);
}


void
intel_miptree_slice_set_needs_depth_resolve(struct intel_mipmap_tree *mt,
                                            uint32_t level,
                                            uint32_t layer)
{
   intel_miptree_check_level_layer(mt, level, layer);

   if (!mt->hiz_mt)
      return;

   intel_resolve_map_set(&mt->hiz_map,
			 level, layer, GEN6_HIZ_OP_DEPTH_RESOLVE);
}

static bool
intel_miptree_slice_resolve(struct intel_context *intel,
			    struct intel_mipmap_tree *mt,
			    uint32_t level,
			    uint32_t layer,
			    enum gen6_hiz_op need)
{
   intel_miptree_check_level_layer(mt, level, layer);

   struct intel_resolve_map *item =
	 intel_resolve_map_get(&mt->hiz_map, level, layer);

   if (!item || item->need != need)
      return false;

   intel_hiz_exec(intel, mt, level, layer, need);
   intel_resolve_map_remove(item);
   return true;
}

bool
intel_miptree_slice_resolve_hiz(struct intel_context *intel,
				struct intel_mipmap_tree *mt,
				uint32_t level,
				uint32_t layer)
{
   return intel_miptree_slice_resolve(intel, mt, level, layer,
				      GEN6_HIZ_OP_HIZ_RESOLVE);
}

bool
intel_miptree_slice_resolve_depth(struct intel_context *intel,
				  struct intel_mipmap_tree *mt,
				  uint32_t level,
				  uint32_t layer)
{
   return intel_miptree_slice_resolve(intel, mt, level, layer,
				      GEN6_HIZ_OP_DEPTH_RESOLVE);
}

static bool
intel_miptree_all_slices_resolve(struct intel_context *intel,
				 struct intel_mipmap_tree *mt,
				 enum gen6_hiz_op need)
{
   bool did_resolve = false;
   struct intel_resolve_map *i, *next;

   for (i = mt->hiz_map.next; i; i = next) {
      next = i->next;
      if (i->need != need)
	 continue;

      intel_hiz_exec(intel, mt, i->level, i->layer, need);
      intel_resolve_map_remove(i);
      did_resolve = true;
   }

   return did_resolve;
}

bool
intel_miptree_all_slices_resolve_hiz(struct intel_context *intel,
				     struct intel_mipmap_tree *mt)
{
   return intel_miptree_all_slices_resolve(intel, mt,
					   GEN6_HIZ_OP_HIZ_RESOLVE);
}

bool
intel_miptree_all_slices_resolve_depth(struct intel_context *intel,
				       struct intel_mipmap_tree *mt)
{
   return intel_miptree_all_slices_resolve(intel, mt,
					   GEN6_HIZ_OP_DEPTH_RESOLVE);
}

static void
intel_miptree_updownsample(struct intel_context *intel,
                           struct intel_mipmap_tree *src,
                           struct intel_mipmap_tree *dst,
                           unsigned width,
                           unsigned height)
{
#ifndef I915
   int src_x0 = 0;
   int src_y0 = 0;
   int dst_x0 = 0;
   int dst_y0 = 0;

   intel_miptree_slice_resolve_depth(intel, src, 0, 0);
   intel_miptree_slice_resolve_depth(intel, dst, 0, 0);

   brw_blorp_blit_miptrees(intel,
                           src, 0 /* level */, 0 /* layer */,
                           dst, 0 /* level */, 0 /* layer */,
                           src_x0, src_y0,
                           dst_x0, dst_y0,
                           width, height,
                           false, false /*mirror x, y*/);

   if (src->stencil_mt) {
      brw_blorp_blit_miptrees(intel,
                              src->stencil_mt, 0 /* level */, 0 /* layer */,
                              dst->stencil_mt, 0 /* level */, 0 /* layer */,
                              src_x0, src_y0,
                              dst_x0, dst_y0,
                              width, height,
                              false, false /*mirror x, y*/);
   }
#endif /* I915 */
}

static void
assert_is_flat(struct intel_mipmap_tree *mt)
{
   assert(mt->target == GL_TEXTURE_2D);
   assert(mt->first_level == 0);
   assert(mt->last_level == 0);
}

/**
 * \brief Downsample from mt to mt->singlesample_mt.
 *
 * If the miptree needs no downsample, then skip.
 */
void
intel_miptree_downsample(struct intel_context *intel,
                         struct intel_mipmap_tree *mt)
{
   /* Only flat, renderbuffer-like miptrees are supported. */
   assert_is_flat(mt);

   if (!mt->need_downsample)
      return;
   intel_miptree_updownsample(intel,
                              mt, mt->singlesample_mt,
                              mt->singlesample_mt->width0,
                              mt->singlesample_mt->height0);
   mt->need_downsample = false;

   /* Strictly speaking, after a downsample on a depth miptree, a hiz
    * resolve is needed on the singlesample miptree. However, since the
    * singlesample miptree is never rendered to, the hiz resolve will never
    * occur. Therefore we do not mark the needed hiz resolve after
    * downsampling.
    */
}

/**
 * \brief Upsample from mt->singlesample_mt to mt.
 *
 * The upsample is done unconditionally.
 */
void
intel_miptree_upsample(struct intel_context *intel,
                       struct intel_mipmap_tree *mt)
{
   /* Only flat, renderbuffer-like miptrees are supported. */
   assert_is_flat(mt);
   assert(!mt->need_downsample);

   intel_miptree_updownsample(intel,
                              mt->singlesample_mt, mt,
                              mt->singlesample_mt->width0,
                              mt->singlesample_mt->height0);
   intel_miptree_slice_set_needs_hiz_resolve(mt, 0, 0);
}

static void
intel_miptree_map_gtt(struct intel_context *intel,
		      struct intel_mipmap_tree *mt,
		      struct intel_miptree_map *map,
		      unsigned int level, unsigned int slice)
{
   unsigned int bw, bh;
   void *base;
   unsigned int image_x, image_y;
   int x = map->x;
   int y = map->y;

   /* For compressed formats, the stride is the number of bytes per
    * row of blocks.  intel_miptree_get_image_offset() already does
    * the divide.
    */
   _mesa_get_format_block_size(mt->format, &bw, &bh);
   assert(y % bh == 0);
   y /= bh;

   base = intel_region_map(intel, mt->region, map->mode);

   if (base == NULL)
      map->ptr = NULL;
   else {
      /* Note that in the case of cube maps, the caller must have passed the
       * slice number referencing the face.
      */
      intel_miptree_get_image_offset(mt, level, 0, slice, &image_x, &image_y);
      x += image_x;
      y += image_y;

      map->stride = mt->region->pitch * mt->cpp;
      map->ptr = base + y * map->stride + x * mt->cpp;
   }

   DBG("%s: %d,%d %dx%d from mt %p (%s) %d,%d = %p/%d\n", __FUNCTION__,
       map->x, map->y, map->w, map->h,
       mt, _mesa_get_format_name(mt->format),
       x, y, map->ptr, map->stride);
}

static void
intel_miptree_unmap_gtt(struct intel_context *intel,
			struct intel_mipmap_tree *mt,
			struct intel_miptree_map *map,
			unsigned int level,
			unsigned int slice)
{
   intel_region_unmap(intel, mt->region);
}

static void
intel_miptree_map_blit(struct intel_context *intel,
		       struct intel_mipmap_tree *mt,
		       struct intel_miptree_map *map,
		       unsigned int level, unsigned int slice)
{
   unsigned int image_x, image_y;
   int x = map->x;
   int y = map->y;
   int ret;

   /* The blitter requires the pitch to be aligned to 4. */
   map->stride = ALIGN(map->w * mt->region->cpp, 4);

   map->bo = drm_intel_bo_alloc(intel->bufmgr, "intel_miptree_map_blit() temp",
				map->stride * map->h, 4096);
   if (!map->bo) {
      fprintf(stderr, "Failed to allocate blit temporary\n");
      goto fail;
   }

   intel_miptree_get_image_offset(mt, level, 0, slice, &image_x, &image_y);
   x += image_x;
   y += image_y;

   if (!intelEmitCopyBlit(intel,
			  mt->region->cpp,
			  mt->region->pitch, mt->region->bo,
			  0, mt->region->tiling,
			  map->stride / mt->region->cpp, map->bo,
			  0, I915_TILING_NONE,
			  x, y,
			  0, 0,
			  map->w, map->h,
			  GL_COPY)) {
      fprintf(stderr, "Failed to blit\n");
      goto fail;
   }

   intel_batchbuffer_flush(intel);
   ret = drm_intel_bo_map(map->bo, (map->mode & GL_MAP_WRITE_BIT) != 0);
   if (ret) {
      fprintf(stderr, "Failed to map blit temporary\n");
      goto fail;
   }

   map->ptr = map->bo->virtual;

   DBG("%s: %d,%d %dx%d from mt %p (%s) %d,%d = %p/%d\n", __FUNCTION__,
       map->x, map->y, map->w, map->h,
       mt, _mesa_get_format_name(mt->format),
       x, y, map->ptr, map->stride);

   return;

fail:
   drm_intel_bo_unreference(map->bo);
   map->ptr = NULL;
   map->stride = 0;
}

static void
intel_miptree_unmap_blit(struct intel_context *intel,
			 struct intel_mipmap_tree *mt,
			 struct intel_miptree_map *map,
			 unsigned int level,
			 unsigned int slice)
{
   assert(!(map->mode & GL_MAP_WRITE_BIT));

   drm_intel_bo_unmap(map->bo);
   drm_intel_bo_unreference(map->bo);
}

static void
intel_miptree_map_s8(struct intel_context *intel,
		     struct intel_mipmap_tree *mt,
		     struct intel_miptree_map *map,
		     unsigned int level, unsigned int slice)
{
   map->stride = map->w;
   map->buffer = map->ptr = malloc(map->stride * map->h);
   if (!map->buffer)
      return;

   /* One of either READ_BIT or WRITE_BIT or both is set.  READ_BIT implies no
    * INVALIDATE_RANGE_BIT.  WRITE_BIT needs the original values read in unless
    * invalidate is set, since we'll be writing the whole rectangle from our
    * temporary buffer back out.
    */
   if (!(map->mode & GL_MAP_INVALIDATE_RANGE_BIT)) {
      uint8_t *untiled_s8_map = map->ptr;
      uint8_t *tiled_s8_map = intel_region_map(intel, mt->region,
					       GL_MAP_READ_BIT);
      unsigned int image_x, image_y;

      intel_miptree_get_image_offset(mt, level, 0, slice, &image_x, &image_y);

      for (uint32_t y = 0; y < map->h; y++) {
	 for (uint32_t x = 0; x < map->w; x++) {
	    ptrdiff_t offset = intel_offset_S8(mt->region->pitch,
	                                       x + image_x + map->x,
	                                       y + image_y + map->y,
					       intel->has_swizzling);
	    untiled_s8_map[y * map->w + x] = tiled_s8_map[offset];
	 }
      }

      intel_region_unmap(intel, mt->region);

      DBG("%s: %d,%d %dx%d from mt %p %d,%d = %p/%d\n", __FUNCTION__,
	  map->x, map->y, map->w, map->h,
	  mt, map->x + image_x, map->y + image_y, map->ptr, map->stride);
   } else {
      DBG("%s: %d,%d %dx%d from mt %p = %p/%d\n", __FUNCTION__,
	  map->x, map->y, map->w, map->h,
	  mt, map->ptr, map->stride);
   }
}

static void
intel_miptree_unmap_s8(struct intel_context *intel,
		       struct intel_mipmap_tree *mt,
		       struct intel_miptree_map *map,
		       unsigned int level,
		       unsigned int slice)
{
   if (map->mode & GL_MAP_WRITE_BIT) {
      unsigned int image_x, image_y;
      uint8_t *untiled_s8_map = map->ptr;
      uint8_t *tiled_s8_map = intel_region_map(intel, mt->region, map->mode);

      intel_miptree_get_image_offset(mt, level, 0, slice, &image_x, &image_y);

      for (uint32_t y = 0; y < map->h; y++) {
	 for (uint32_t x = 0; x < map->w; x++) {
	    ptrdiff_t offset = intel_offset_S8(mt->region->pitch,
	                                       x + map->x,
	                                       y + map->y,
					       intel->has_swizzling);
	    tiled_s8_map[offset] = untiled_s8_map[y * map->w + x];
	 }
      }

      intel_region_unmap(intel, mt->region);
   }

   free(map->buffer);
}

static void
intel_miptree_map_etc1(struct intel_context *intel,
                       struct intel_mipmap_tree *mt,
                       struct intel_miptree_map *map,
                       unsigned int level,
                       unsigned int slice)
{
   /* For justification of these invariants,
    * see intel_mipmap_tree:wraps_etc1.
    */
   assert(mt->wraps_etc1);
   assert(mt->format == MESA_FORMAT_RGBX8888_REV);

   /* From the GL_OES_compressed_ETC1_RGB8_texture spec:
    *   INVALID_OPERATION is generated by CompressedTexSubImage2D,
    *   TexSubImage2D, or CopyTexSubImage2D if the texture image <level>
    *   bound to <target> has internal format ETC1_RGB8_OES.
    *
    * This implies that intel_miptree_map_etc1() can only be called from
    * glCompressedTexImage2D, and hence the assertions below hold.
    */
   assert(map->mode & GL_MAP_WRITE_BIT);
   assert(map->mode & GL_MAP_INVALIDATE_RANGE_BIT);
   assert(map->x == 0);
   assert(map->y == 0);

   /* Each ETC1 block contains 4x4 pixels in 8 bytes. */
   map->stride = 2 * map->w;
   map->buffer = map->ptr = malloc(map->stride * map->h);
}

static void
intel_miptree_unmap_etc1(struct intel_context *intel,
                         struct intel_mipmap_tree *mt,
                         struct intel_miptree_map *map,
                         unsigned int level,
                         unsigned int slice)
{
   uint32_t image_x;
   uint32_t image_y;
   intel_miptree_get_image_offset(mt, level, 0, slice, &image_x, &image_y);

   uint8_t *xbgr = intel_region_map(intel, mt->region, map->mode)
                 + image_y * mt->region->pitch * mt->region->cpp
                 + image_x * mt->region->cpp;

   _mesa_etc1_unpack_rgba8888(xbgr, mt->region->pitch * mt->region->cpp,
                              map->ptr, map->stride,
                              map->w, map->h);

   intel_region_unmap(intel, mt->region);
   free(map->buffer);
}

/**
 * Mapping function for packed depth/stencil miptrees backed by real separate
 * miptrees for depth and stencil.
 *
 * On gen7, and to support HiZ pre-gen7, we have to have the stencil buffer
 * separate from the depth buffer.  Yet at the GL API level, we have to expose
 * packed depth/stencil textures and FBO attachments, and Mesa core expects to
 * be able to map that memory for texture storage and glReadPixels-type
 * operations.  We give Mesa core that access by mallocing a temporary and
 * copying the data between the actual backing store and the temporary.
 */
static void
intel_miptree_map_depthstencil(struct intel_context *intel,
			       struct intel_mipmap_tree *mt,
			       struct intel_miptree_map *map,
			       unsigned int level, unsigned int slice)
{
   struct intel_mipmap_tree *z_mt = mt;
   struct intel_mipmap_tree *s_mt = mt->stencil_mt;
   bool map_z32f_x24s8 = mt->format == MESA_FORMAT_Z32_FLOAT;
   int packed_bpp = map_z32f_x24s8 ? 8 : 4;

   map->stride = map->w * packed_bpp;
   map->buffer = map->ptr = malloc(map->stride * map->h);
   if (!map->buffer)
      return;

   /* One of either READ_BIT or WRITE_BIT or both is set.  READ_BIT implies no
    * INVALIDATE_RANGE_BIT.  WRITE_BIT needs the original values read in unless
    * invalidate is set, since we'll be writing the whole rectangle from our
    * temporary buffer back out.
    */
   if (!(map->mode & GL_MAP_INVALIDATE_RANGE_BIT)) {
      uint32_t *packed_map = map->ptr;
      uint8_t *s_map = intel_region_map(intel, s_mt->region, GL_MAP_READ_BIT);
      uint32_t *z_map = intel_region_map(intel, z_mt->region, GL_MAP_READ_BIT);
      unsigned int s_image_x, s_image_y;
      unsigned int z_image_x, z_image_y;

      intel_miptree_get_image_offset(s_mt, level, 0, slice,
				     &s_image_x, &s_image_y);
      intel_miptree_get_image_offset(z_mt, level, 0, slice,
				     &z_image_x, &z_image_y);

      for (uint32_t y = 0; y < map->h; y++) {
	 for (uint32_t x = 0; x < map->w; x++) {
	    int map_x = map->x + x, map_y = map->y + y;
	    ptrdiff_t s_offset = intel_offset_S8(s_mt->region->pitch,
						 map_x + s_image_x,
						 map_y + s_image_y,
						 intel->has_swizzling);
	    ptrdiff_t z_offset = ((map_y + z_image_y) * z_mt->region->pitch +
				  (map_x + z_image_x));
	    uint8_t s = s_map[s_offset];
	    uint32_t z = z_map[z_offset];

	    if (map_z32f_x24s8) {
	       packed_map[(y * map->w + x) * 2 + 0] = z;
	       packed_map[(y * map->w + x) * 2 + 1] = s;
	    } else {
	       packed_map[y * map->w + x] = (s << 24) | (z & 0x00ffffff);
	    }
	 }
      }

      intel_region_unmap(intel, s_mt->region);
      intel_region_unmap(intel, z_mt->region);

      DBG("%s: %d,%d %dx%d from z mt %p %d,%d, s mt %p %d,%d = %p/%d\n",
	  __FUNCTION__,
	  map->x, map->y, map->w, map->h,
	  z_mt, map->x + z_image_x, map->y + z_image_y,
	  s_mt, map->x + s_image_x, map->y + s_image_y,
	  map->ptr, map->stride);
   } else {
      DBG("%s: %d,%d %dx%d from mt %p = %p/%d\n", __FUNCTION__,
	  map->x, map->y, map->w, map->h,
	  mt, map->ptr, map->stride);
   }
}

static void
intel_miptree_unmap_depthstencil(struct intel_context *intel,
				 struct intel_mipmap_tree *mt,
				 struct intel_miptree_map *map,
				 unsigned int level,
				 unsigned int slice)
{
   struct intel_mipmap_tree *z_mt = mt;
   struct intel_mipmap_tree *s_mt = mt->stencil_mt;
   bool map_z32f_x24s8 = mt->format == MESA_FORMAT_Z32_FLOAT;

   if (map->mode & GL_MAP_WRITE_BIT) {
      uint32_t *packed_map = map->ptr;
      uint8_t *s_map = intel_region_map(intel, s_mt->region, map->mode);
      uint32_t *z_map = intel_region_map(intel, z_mt->region, map->mode);
      unsigned int s_image_x, s_image_y;
      unsigned int z_image_x, z_image_y;

      intel_miptree_get_image_offset(s_mt, level, 0, slice,
				     &s_image_x, &s_image_y);
      intel_miptree_get_image_offset(z_mt, level, 0, slice,
				     &z_image_x, &z_image_y);

      for (uint32_t y = 0; y < map->h; y++) {
	 for (uint32_t x = 0; x < map->w; x++) {
	    ptrdiff_t s_offset = intel_offset_S8(s_mt->region->pitch,
						 x + s_image_x + map->x,
						 y + s_image_y + map->y,
						 intel->has_swizzling);
	    ptrdiff_t z_offset = ((y + z_image_y) * z_mt->region->pitch +
				  (x + z_image_x));

	    if (map_z32f_x24s8) {
	       z_map[z_offset] = packed_map[(y * map->w + x) * 2 + 0];
	       s_map[s_offset] = packed_map[(y * map->w + x) * 2 + 1];
	    } else {
	       uint32_t packed = packed_map[y * map->w + x];
	       s_map[s_offset] = packed >> 24;
	       z_map[z_offset] = packed;
	    }
	 }
      }

      intel_region_unmap(intel, s_mt->region);
      intel_region_unmap(intel, z_mt->region);

      DBG("%s: %d,%d %dx%d from z mt %p (%s) %d,%d, s mt %p %d,%d = %p/%d\n",
	  __FUNCTION__,
	  map->x, map->y, map->w, map->h,
	  z_mt, _mesa_get_format_name(z_mt->format),
	  map->x + z_image_x, map->y + z_image_y,
	  s_mt, map->x + s_image_x, map->y + s_image_y,
	  map->ptr, map->stride);
   }

   free(map->buffer);
}

/**
 * Create and attach a map to the miptree at (level, slice). Return the
 * attached map.
 */
static struct intel_miptree_map*
intel_miptree_attach_map(struct intel_mipmap_tree *mt,
                         unsigned int level,
                         unsigned int slice,
                         unsigned int x,
                         unsigned int y,
                         unsigned int w,
                         unsigned int h,
                         GLbitfield mode)
{
   struct intel_miptree_map *map = calloc(1, sizeof(*map));

   if (!map)
      return NULL;

   assert(mt->level[level].slice[slice].map == NULL);
   mt->level[level].slice[slice].map = map;

   map->mode = mode;
   map->x = x;
   map->y = y;
   map->w = w;
   map->h = h;

   return map;
}

/**
 * Release the map at (level, slice).
 */
static void
intel_miptree_release_map(struct intel_mipmap_tree *mt,
                         unsigned int level,
                         unsigned int slice)
{
   struct intel_miptree_map **map;

   map = &mt->level[level].slice[slice].map;
   free(*map);
   *map = NULL;
}

static void
intel_miptree_map_singlesample(struct intel_context *intel,
                               struct intel_mipmap_tree *mt,
                               unsigned int level,
                               unsigned int slice,
                               unsigned int x,
                               unsigned int y,
                               unsigned int w,
                               unsigned int h,
                               GLbitfield mode,
                               void **out_ptr,
                               int *out_stride)
{
   struct intel_miptree_map *map;

   assert(mt->num_samples <= 1);

   map = intel_miptree_attach_map(mt, level, slice, x, y, w, h, mode);
   if (!map){
      *out_ptr = NULL;
      *out_stride = 0;
      return;
   }

   intel_miptree_slice_resolve_depth(intel, mt, level, slice);
   if (map->mode & GL_MAP_WRITE_BIT) {
      intel_miptree_slice_set_needs_hiz_resolve(mt, level, slice);
   }

   if (mt->format == MESA_FORMAT_S8) {
      intel_miptree_map_s8(intel, mt, map, level, slice);
   } else if (mt->wraps_etc1) {
      intel_miptree_map_etc1(intel, mt, map, level, slice);
   } else if (mt->stencil_mt) {
      intel_miptree_map_depthstencil(intel, mt, map, level, slice);
   } else if (intel->has_llc &&
	      !(mode & GL_MAP_WRITE_BIT) &&
	      !mt->compressed &&
	      mt->region->tiling == I915_TILING_X) {
      intel_miptree_map_blit(intel, mt, map, level, slice);
   } else {
      intel_miptree_map_gtt(intel, mt, map, level, slice);
   }

   *out_ptr = map->ptr;
   *out_stride = map->stride;

   if (map->ptr == NULL)
      intel_miptree_release_map(mt, level, slice);
}

static void
intel_miptree_unmap_singlesample(struct intel_context *intel,
                                 struct intel_mipmap_tree *mt,
                                 unsigned int level,
                                 unsigned int slice)
{
   struct intel_miptree_map *map = mt->level[level].slice[slice].map;

   assert(mt->num_samples <= 1);

   if (!map)
      return;

   DBG("%s: mt %p (%s) level %d slice %d\n", __FUNCTION__,
       mt, _mesa_get_format_name(mt->format), level, slice);

   if (mt->format == MESA_FORMAT_S8) {
      intel_miptree_unmap_s8(intel, mt, map, level, slice);
   } else if (mt->wraps_etc1) {
      intel_miptree_unmap_etc1(intel, mt, map, level, slice);
   } else if (mt->stencil_mt) {
      intel_miptree_unmap_depthstencil(intel, mt, map, level, slice);
   } else if (map->bo) {
      intel_miptree_unmap_blit(intel, mt, map, level, slice);
   } else {
      intel_miptree_unmap_gtt(intel, mt, map, level, slice);
   }

   intel_miptree_release_map(mt, level, slice);
}

static void
intel_miptree_map_multisample(struct intel_context *intel,
                              struct intel_mipmap_tree *mt,
                              unsigned int level,
                              unsigned int slice,
                              unsigned int x,
                              unsigned int y,
                              unsigned int w,
                              unsigned int h,
                              GLbitfield mode,
                              void **out_ptr,
                              int *out_stride)
{
   struct intel_miptree_map *map;

   assert(mt->num_samples > 1);

   /* Only flat, renderbuffer-like miptrees are supported. */
   if (mt->target != GL_TEXTURE_2D ||
       mt->first_level != 0 ||
       mt->last_level != 0) {
      _mesa_problem(&intel->ctx, "attempt to map a multisample miptree for "
                    "which (target, first_level, last_level != "
                    "(GL_TEXTURE_2D, 0, 0)");
      goto fail;
   }

   map = intel_miptree_attach_map(mt, level, slice, x, y, w, h, mode);
   if (!map)
      goto fail;

   if (!mt->singlesample_mt) {
      mt->singlesample_mt =
         intel_miptree_create_for_renderbuffer(intel,
                                               mt->format,
                                               mt->singlesample_width0,
                                               mt->singlesample_height0,
                                               0 /*num_samples*/);
      if (!mt->singlesample_mt)
         goto fail;

      map->singlesample_mt_is_tmp = true;
      mt->need_downsample = true;
   }

   intel_miptree_downsample(intel, mt);
   intel_miptree_map_singlesample(intel, mt->singlesample_mt,
                                  level, slice,
                                  x, y, w, h,
                                  mode,
                                  out_ptr, out_stride);
   return;

fail:
   intel_miptree_release_map(mt, level, slice);
   *out_ptr = NULL;
   *out_stride = 0;
}

static void
intel_miptree_unmap_multisample(struct intel_context *intel,
                                struct intel_mipmap_tree *mt,
                                unsigned int level,
                                unsigned int slice)
{
   struct intel_miptree_map *map = mt->level[level].slice[slice].map;

   assert(mt->num_samples > 1);

   if (!map)
      return;

   intel_miptree_unmap_singlesample(intel, mt->singlesample_mt, level, slice);

   mt->need_downsample = false;
   if (map->mode & GL_MAP_WRITE_BIT)
      intel_miptree_upsample(intel, mt);

   if (map->singlesample_mt_is_tmp)
      intel_miptree_release(&mt->singlesample_mt);

   intel_miptree_release_map(mt, level, slice);
}

void
intel_miptree_map(struct intel_context *intel,
		  struct intel_mipmap_tree *mt,
		  unsigned int level,
		  unsigned int slice,
		  unsigned int x,
		  unsigned int y,
		  unsigned int w,
		  unsigned int h,
		  GLbitfield mode,
		  void **out_ptr,
		  int *out_stride)
{
   if (mt->num_samples <= 1)
      intel_miptree_map_singlesample(intel, mt,
                                     level, slice,
                                     x, y, w, h,
                                     mode,
                                     out_ptr, out_stride);
   else
      intel_miptree_map_multisample(intel, mt,
                                    level, slice,
                                    x, y, w, h,
                                    mode,
                                    out_ptr, out_stride);
}

void
intel_miptree_unmap(struct intel_context *intel,
		    struct intel_mipmap_tree *mt,
		    unsigned int level,
		    unsigned int slice)
{
   if (mt->num_samples <= 1)
      intel_miptree_unmap_singlesample(intel, mt, level, slice);
   else
      intel_miptree_unmap_multisample(intel, mt, level, slice);
}
