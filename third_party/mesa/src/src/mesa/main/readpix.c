/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "glheader.h"
#include "imports.h"
#include "bufferobj.h"
#include "context.h"
#include "enums.h"
#include "readpix.h"
#include "framebuffer.h"
#include "formats.h"
#include "format_unpack.h"
#include "image.h"
#include "mtypes.h"
#include "pack.h"
#include "pbo.h"
#include "state.h"
#include "glformats.h"
#include "fbobject.h"
#include "teximage.h"

/* Inexplicably, GL_HALF_FLOAT_OES has a different value than GL_HALF_FLOAT.
 */
#ifndef GL_HALF_FLOAT_OES
#define GL_HALF_FLOAT_OES 0x8D61
#endif

/**
 * Tries to implement glReadPixels() of GL_DEPTH_COMPONENT using memcpy of the
 * mapping.
 */
static GLboolean
fast_read_depth_pixels( struct gl_context *ctx,
			GLint x, GLint y,
			GLsizei width, GLsizei height,
			GLenum type, GLvoid *pixels,
			const struct gl_pixelstore_attrib *packing )
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   GLubyte *map, *dst;
   int stride, dstStride, j;

   if (ctx->Pixel.DepthScale != 1.0 || ctx->Pixel.DepthBias != 0.0)
      return GL_FALSE;

   if (packing->SwapBytes)
      return GL_FALSE;

   if (_mesa_get_format_datatype(rb->Format) != GL_UNSIGNED_NORMALIZED)
      return GL_FALSE;

   if (!((type == GL_UNSIGNED_SHORT && rb->Format == MESA_FORMAT_Z16) ||
	 type == GL_UNSIGNED_INT))
      return GL_FALSE;

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);

   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
      return GL_TRUE;  /* don't bother trying the slow path */
   }

   dstStride = _mesa_image_row_stride(packing, width, GL_DEPTH_COMPONENT, type);
   dst = (GLubyte *) _mesa_image_address2d(packing, pixels, width, height,
					   GL_DEPTH_COMPONENT, type, 0, 0);

   for (j = 0; j < height; j++) {
      if (type == GL_UNSIGNED_INT) {
	 _mesa_unpack_uint_z_row(rb->Format, width, map, (GLuint *)dst);
      } else {
	 ASSERT(type == GL_UNSIGNED_SHORT && rb->Format == MESA_FORMAT_Z16);
	 memcpy(dst, map, width * 2);
      }

      map += stride;
      dst += dstStride;
   }
   ctx->Driver.UnmapRenderbuffer(ctx, rb);

   return GL_TRUE;
}

/**
 * Read pixels for format=GL_DEPTH_COMPONENT.
 */
static void
read_depth_pixels( struct gl_context *ctx,
                   GLint x, GLint y,
                   GLsizei width, GLsizei height,
                   GLenum type, GLvoid *pixels,
                   const struct gl_pixelstore_attrib *packing )
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   GLint j;
   GLubyte *dst, *map;
   int dstStride, stride;
   GLfloat *depthValues;

   if (!rb)
      return;

   /* clipping should have been done already */
   ASSERT(x >= 0);
   ASSERT(y >= 0);
   ASSERT(x + width <= (GLint) rb->Width);
   ASSERT(y + height <= (GLint) rb->Height);

   if (fast_read_depth_pixels(ctx, x, y, width, height, type, pixels, packing))
      return;

   dstStride = _mesa_image_row_stride(packing, width, GL_DEPTH_COMPONENT, type);
   dst = (GLubyte *) _mesa_image_address2d(packing, pixels, width, height,
					   GL_DEPTH_COMPONENT, type, 0, 0);

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
      return;
   }

   depthValues = (GLfloat *) malloc(width * sizeof(GLfloat));

   if (depthValues) {
      /* General case (slower) */
      for (j = 0; j < height; j++, y++) {
         _mesa_unpack_float_z_row(rb->Format, width, map, depthValues);
         _mesa_pack_depth_span(ctx, width, dst, type, depthValues, packing);

         dst += dstStride;
         map += stride;
      }
   }
   else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
   }

   free(depthValues);

   ctx->Driver.UnmapRenderbuffer(ctx, rb);
}


/**
 * Read pixels for format=GL_STENCIL_INDEX.
 */
static void
read_stencil_pixels( struct gl_context *ctx,
                     GLint x, GLint y,
                     GLsizei width, GLsizei height,
                     GLenum type, GLvoid *pixels,
                     const struct gl_pixelstore_attrib *packing )
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLint j;
   GLubyte *map, *stencil;
   GLint stride;

   if (!rb)
      return;

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
      return;
   }

   stencil = (GLubyte *) malloc(width * sizeof(GLubyte));

   if (stencil) {
      /* process image row by row */
      for (j = 0; j < height; j++) {
         GLvoid *dest;

         _mesa_unpack_ubyte_stencil_row(rb->Format, width, map, stencil);
         dest = _mesa_image_address2d(packing, pixels, width, height,
                                      GL_STENCIL_INDEX, type, j, 0);

         _mesa_pack_stencil_span(ctx, width, type, dest, stencil, packing);

         map += stride;
      }
   }
   else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
   }

   free(stencil);

   ctx->Driver.UnmapRenderbuffer(ctx, rb);
}


/**
 * Try to do glReadPixels of RGBA data using a simple memcpy or swizzle.
 * \return GL_TRUE if successful, GL_FALSE otherwise (use the slow path)
 */
static GLboolean
fast_read_rgba_pixels_memcpy( struct gl_context *ctx,
			      GLint x, GLint y,
			      GLsizei width, GLsizei height,
			      GLenum format, GLenum type,
			      GLvoid *pixels,
			      const struct gl_pixelstore_attrib *packing,
			      GLbitfield transferOps )
{
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
   GLubyte *dst, *map;
   int dstStride, stride, j, texelBytes;
   GLboolean swizzle_rb = GL_FALSE, copy_xrgb = GL_FALSE;

   /* XXX we could check for other swizzle/special cases here as needed */
   if (rb->Format == MESA_FORMAT_RGBA8888_REV &&
       format == GL_BGRA &&
       type == GL_UNSIGNED_INT_8_8_8_8_REV &&
       !ctx->Pack.SwapBytes) {
      swizzle_rb = GL_TRUE;
   }
   else if (rb->Format == MESA_FORMAT_XRGB8888 &&
       format == GL_BGRA &&
       type == GL_UNSIGNED_INT_8_8_8_8_REV &&
       !ctx->Pack.SwapBytes) {
      copy_xrgb = GL_TRUE;
   }
   else if (!_mesa_format_matches_format_and_type(rb->Format, format, type,
                                                  ctx->Pack.SwapBytes))
      return GL_FALSE;

   /* If the format is unsigned normalized then we can ignore clamping
    * because the values are already in the range [0,1] so it won't
    * have any effect anyway.
    */
   if (_mesa_get_format_datatype(rb->Format) == GL_UNSIGNED_NORMALIZED)
      transferOps &= ~IMAGE_CLAMP_BIT;

   if (transferOps)
      return GL_FALSE;

   dstStride = _mesa_image_row_stride(packing, width, format, type);
   dst = (GLubyte *) _mesa_image_address2d(packing, pixels, width, height,
					   format, type, 0, 0);

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
      return GL_TRUE;  /* don't bother trying the slow path */
   }

   texelBytes = _mesa_get_format_bytes(rb->Format);

   if (swizzle_rb) {
      /* swap R/B */
      for (j = 0; j < height; j++) {
         int i;
         for (i = 0; i < width; i++) {
            GLuint *dst4 = (GLuint *) dst, *map4 = (GLuint *) map;
            GLuint pixel = map4[i];
            dst4[i] = (pixel & 0xff00ff00)
                   | ((pixel & 0x00ff0000) >> 16)
                   | ((pixel & 0x000000ff) << 16);
         }
         dst += dstStride;
         map += stride;
      }
   } else if (copy_xrgb) {
      /* convert xrgb -> argb */
      for (j = 0; j < height; j++) {
         GLuint *dst4 = (GLuint *) dst, *map4 = (GLuint *) map;
         int i;
         for (i = 0; i < width; i++) {
            dst4[i] = map4[i] | 0xff000000;  /* set A=0xff */
         }
         dst += dstStride;
         map += stride;
      }
   } else {
      /* just memcpy */
      for (j = 0; j < height; j++) {
         memcpy(dst, map, width * texelBytes);
         dst += dstStride;
         map += stride;
      }
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);

   return GL_TRUE;
}

static void
slow_read_rgba_pixels( struct gl_context *ctx,
		       GLint x, GLint y,
		       GLsizei width, GLsizei height,
		       GLenum format, GLenum type,
		       GLvoid *pixels,
		       const struct gl_pixelstore_attrib *packing,
		       GLbitfield transferOps )
{
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
   const gl_format rbFormat = _mesa_get_srgb_format_linear(rb->Format);
   void *rgba;
   GLubyte *dst, *map;
   int dstStride, stride, j;
   GLboolean dst_is_integer = _mesa_is_enum_format_integer(format);
   GLboolean dst_is_uint = _mesa_is_format_unsigned(rbFormat);

   dstStride = _mesa_image_row_stride(packing, width, format, type);
   dst = (GLubyte *) _mesa_image_address2d(packing, pixels, width, height,
					   format, type, 0, 0);

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
      return;
   }

   rgba = malloc(width * MAX_PIXEL_BYTES);
   if (!rgba)
      goto done;

   for (j = 0; j < height; j++) {
      if (dst_is_integer) {
	 _mesa_unpack_uint_rgba_row(rbFormat, width, map, (GLuint (*)[4]) rgba);
         _mesa_rebase_rgba_uint(width, (GLuint (*)[4]) rgba,
                                rb->_BaseFormat);
         if (dst_is_uint) {
            _mesa_pack_rgba_span_from_uints(ctx, width, (GLuint (*)[4]) rgba, format,
                                            type, dst);
         } else {
            _mesa_pack_rgba_span_from_ints(ctx, width, (GLint (*)[4]) rgba, format,
                                           type, dst);
         }
      } else {
	 _mesa_unpack_rgba_row(rbFormat, width, map, (GLfloat (*)[4]) rgba);
         _mesa_rebase_rgba_float(width, (GLfloat (*)[4]) rgba,
                                 rb->_BaseFormat);
	 _mesa_pack_rgba_span_float(ctx, width, (GLfloat (*)[4]) rgba, format,
                                    type, dst, packing, transferOps);
      }
      dst += dstStride;
      map += stride;
   }

   free(rgba);

done:
   ctx->Driver.UnmapRenderbuffer(ctx, rb);
}

/*
 * Read R, G, B, A, RGB, L, or LA pixels.
 */
static void
read_rgba_pixels( struct gl_context *ctx,
                  GLint x, GLint y,
                  GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid *pixels,
                  const struct gl_pixelstore_attrib *packing )
{
   GLbitfield transferOps = ctx->_ImageTransferState;
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->_ColorReadBuffer;

   if (!rb)
      return;

   if ((ctx->Color._ClampReadColor == GL_TRUE || type != GL_FLOAT) &&
       !_mesa_is_enum_format_integer(format)) {
      transferOps |= IMAGE_CLAMP_BIT;
   }

   /* Try the optimized paths first. */
   if (fast_read_rgba_pixels_memcpy(ctx, x, y, width, height,
                                    format, type, pixels, packing,
                                    transferOps)) {
      return;
   }

   slow_read_rgba_pixels(ctx, x, y, width, height,
			 format, type, pixels, packing, transferOps);
}

/**
 * For a packed depth/stencil buffer being read as depth/stencil, just memcpy the
 * data (possibly swapping 8/24 vs 24/8 as we go).
 */
static GLboolean
fast_read_depth_stencil_pixels(struct gl_context *ctx,
			       GLint x, GLint y,
			       GLsizei width, GLsizei height,
			       GLubyte *dst, int dstStride)
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   struct gl_renderbuffer *stencilRb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLubyte *map;
   int stride, i;

   if (rb != stencilRb)
      return GL_FALSE;

   if (rb->Format != MESA_FORMAT_Z24_S8 &&
       rb->Format != MESA_FORMAT_S8_Z24)
      return GL_FALSE;

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
      return GL_TRUE;  /* don't bother trying the slow path */
   }

   for (i = 0; i < height; i++) {
      _mesa_unpack_uint_24_8_depth_stencil_row(rb->Format, width,
					       map, (GLuint *)dst);
      map += stride;
      dst += dstStride;
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);

   return GL_TRUE;
}


/**
 * For non-float-depth and stencil buffers being read as 24/8 depth/stencil,
 * copy the integer data directly instead of converting depth to float and
 * re-packing.
 */
static GLboolean
fast_read_depth_stencil_pixels_separate(struct gl_context *ctx,
					GLint x, GLint y,
					GLsizei width, GLsizei height,
					uint32_t *dst, int dstStride)
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *depthRb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   struct gl_renderbuffer *stencilRb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLubyte *depthMap, *stencilMap, *stencilVals;
   int depthStride, stencilStride, i, j;

   if (_mesa_get_format_datatype(depthRb->Format) != GL_UNSIGNED_NORMALIZED)
      return GL_FALSE;

   ctx->Driver.MapRenderbuffer(ctx, depthRb, x, y, width, height,
			       GL_MAP_READ_BIT, &depthMap, &depthStride);
   if (!depthMap) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
      return GL_TRUE;  /* don't bother trying the slow path */
   }

   ctx->Driver.MapRenderbuffer(ctx, stencilRb, x, y, width, height,
			       GL_MAP_READ_BIT, &stencilMap, &stencilStride);
   if (!stencilMap) {
      ctx->Driver.UnmapRenderbuffer(ctx, depthRb);
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
      return GL_TRUE;  /* don't bother trying the slow path */
   }

   stencilVals = (GLubyte *) malloc(width * sizeof(GLubyte));

   if (stencilVals) {
      for (j = 0; j < height; j++) {
         _mesa_unpack_uint_z_row(depthRb->Format, width, depthMap, dst);
         _mesa_unpack_ubyte_stencil_row(stencilRb->Format, width,
                                        stencilMap, stencilVals);

         for (i = 0; i < width; i++) {
            dst[i] = (dst[i] & 0xffffff00) | stencilVals[i];
         }

         depthMap += depthStride;
         stencilMap += stencilStride;
         dst += dstStride / 4;
      }
   }
   else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
   }

   free(stencilVals);

   ctx->Driver.UnmapRenderbuffer(ctx, depthRb);
   ctx->Driver.UnmapRenderbuffer(ctx, stencilRb);

   return GL_TRUE;
}

static void
slow_read_depth_stencil_pixels_separate(struct gl_context *ctx,
					GLint x, GLint y,
					GLsizei width, GLsizei height,
					GLenum type,
					const struct gl_pixelstore_attrib *packing,
					GLubyte *dst, int dstStride)
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *depthRb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   struct gl_renderbuffer *stencilRb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLubyte *depthMap, *stencilMap;
   int depthStride, stencilStride, j;
   GLubyte *stencilVals;
   GLfloat *depthVals;


   /* The depth and stencil buffers might be separate, or a single buffer.
    * If one buffer, only map it once.
    */
   ctx->Driver.MapRenderbuffer(ctx, depthRb, x, y, width, height,
			       GL_MAP_READ_BIT, &depthMap, &depthStride);
   if (!depthMap) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
      return;
   }

   if (stencilRb != depthRb) {
      ctx->Driver.MapRenderbuffer(ctx, stencilRb, x, y, width, height,
                                  GL_MAP_READ_BIT, &stencilMap,
                                  &stencilStride);
      if (!stencilMap) {
         ctx->Driver.UnmapRenderbuffer(ctx, depthRb);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
         return;
      }
   }
   else {
      stencilMap = depthMap;
      stencilStride = depthStride;
   }

   stencilVals = (GLubyte *) malloc(width * sizeof(GLubyte));
   depthVals = (GLfloat *) malloc(width * sizeof(GLfloat));

   if (stencilVals && depthVals) {
      for (j = 0; j < height; j++) {
         _mesa_unpack_float_z_row(depthRb->Format, width, depthMap, depthVals);
         _mesa_unpack_ubyte_stencil_row(stencilRb->Format, width,
                                        stencilMap, stencilVals);

         _mesa_pack_depth_stencil_span(ctx, width, type, (GLuint *)dst,
                                       depthVals, stencilVals, packing);

         depthMap += depthStride;
         stencilMap += stencilStride;
         dst += dstStride;
      }
   }
   else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
   }

   free(stencilVals);
   free(depthVals);

   ctx->Driver.UnmapRenderbuffer(ctx, depthRb);
   if (stencilRb != depthRb) {
      ctx->Driver.UnmapRenderbuffer(ctx, stencilRb);
   }
}


/**
 * Read combined depth/stencil values.
 * We'll have already done error checking to be sure the expected
 * depth and stencil buffers really exist.
 */
static void
read_depth_stencil_pixels(struct gl_context *ctx,
                          GLint x, GLint y,
                          GLsizei width, GLsizei height,
                          GLenum type, GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing )
{
   const GLboolean scaleOrBias
      = ctx->Pixel.DepthScale != 1.0 || ctx->Pixel.DepthBias != 0.0;
   const GLboolean stencilTransfer = ctx->Pixel.IndexShift
      || ctx->Pixel.IndexOffset || ctx->Pixel.MapStencilFlag;
   GLubyte *dst;
   int dstStride;

   dst = (GLubyte *) _mesa_image_address2d(packing, pixels,
					   width, height,
					   GL_DEPTH_STENCIL_EXT,
					   type, 0, 0);
   dstStride = _mesa_image_row_stride(packing, width,
				      GL_DEPTH_STENCIL_EXT, type);

   /* Fast 24/8 reads. */
   if (type == GL_UNSIGNED_INT_24_8 &&
       !scaleOrBias && !stencilTransfer && !packing->SwapBytes) {
      if (fast_read_depth_stencil_pixels(ctx, x, y, width, height,
					 dst, dstStride))
	 return;

      if (fast_read_depth_stencil_pixels_separate(ctx, x, y, width, height,
						  (uint32_t *)dst, dstStride))
	 return;
   }

   slow_read_depth_stencil_pixels_separate(ctx, x, y, width, height,
					   type, packing,
					   dst, dstStride);
}



/**
 * Software fallback routine for ctx->Driver.ReadPixels().
 * By time we get here, all error checking will have been done.
 */
void
_mesa_readpixels(struct gl_context *ctx,
                 GLint x, GLint y, GLsizei width, GLsizei height,
                 GLenum format, GLenum type,
                 const struct gl_pixelstore_attrib *packing,
                 GLvoid *pixels)
{
   struct gl_pixelstore_attrib clippedPacking = *packing;

   if (ctx->NewState)
      _mesa_update_state(ctx);

   /* Do all needed clipping here, so that we can forget about it later */
   if (_mesa_clip_readpixels(ctx, &x, &y, &width, &height, &clippedPacking)) {

      pixels = _mesa_map_pbo_dest(ctx, &clippedPacking, pixels);

      if (pixels) {
         switch (format) {
         case GL_STENCIL_INDEX:
            read_stencil_pixels(ctx, x, y, width, height, type, pixels,
                                &clippedPacking);
            break;
         case GL_DEPTH_COMPONENT:
            read_depth_pixels(ctx, x, y, width, height, type, pixels,
                              &clippedPacking);
            break;
         case GL_DEPTH_STENCIL_EXT:
            read_depth_stencil_pixels(ctx, x, y, width, height, type, pixels,
                                      &clippedPacking);
            break;
         default:
            /* all other formats should be color formats */
            read_rgba_pixels(ctx, x, y, width, height, format, type, pixels,
                             &clippedPacking);
         }

         _mesa_unmap_pbo_dest(ctx, &clippedPacking);
      }
   }
}


void GLAPIENTRY
_mesa_ReadnPixelsARB( GLint x, GLint y, GLsizei width, GLsizei height,
		      GLenum format, GLenum type, GLsizei bufSize,
                      GLvoid *pixels )
{
   GLenum err;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   FLUSH_CURRENT(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glReadPixels(%d, %d, %s, %s, %p)\n",
                  width, height,
                  _mesa_lookup_enum_by_nr(format),
                  _mesa_lookup_enum_by_nr(type),
                  pixels);

   if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE,
                   "glReadPixels(width=%d height=%d)", width, height );
      return;
   }

   /* OpenGL ES 1.x and OpenGL ES 2.0 impose additional restrictions on the
    * combinations of format and type that can be used.
    *
    * Technically, only two combinations are actually allowed:
    * GL_RGBA/GL_UNSIGNED_BYTE, and some implementation-specific internal
    * preferred combination.  This code doesn't know what that preferred
    * combination is, and Mesa can handle anything valid.  Just work instead.
    */
   if (_mesa_is_gles(ctx) && ctx->Version < 30) {
      err = _mesa_es_error_check_format_and_type(format, type, 2);
      if (err == GL_NO_ERROR) {
         if (type == GL_FLOAT || type == GL_HALF_FLOAT_OES) {
            err = GL_INVALID_OPERATION;
         } else if (format == GL_DEPTH_COMPONENT
                    || format == GL_DEPTH_STENCIL) {
            err = GL_INVALID_ENUM;
         }
      }

      if (err != GL_NO_ERROR) {
         _mesa_error(ctx, err, "glReadPixels(invalid format %s and/or type %s)",
                     _mesa_lookup_enum_by_nr(format),
                     _mesa_lookup_enum_by_nr(type));
         return;
      }
   }

   if (ctx->NewState)
      _mesa_update_state(ctx);

   err = _mesa_error_check_format_and_type(ctx, format, type);
   if (err != GL_NO_ERROR) {
      _mesa_error(ctx, err, "glReadPixels(invalid format %s and/or type %s)",
                  _mesa_lookup_enum_by_nr(format),
                  _mesa_lookup_enum_by_nr(type));
      return;
   }

   if (ctx->ReadBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                  "glReadPixels(incomplete framebuffer)" );
      return;
   }

   if (_mesa_is_user_fbo(ctx->ReadBuffer) &&
       ctx->ReadBuffer->Visual.samples > 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glReadPixels(multisample FBO)");
      return;
   }

   if (!_mesa_source_buffer_exists(ctx, format)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glReadPixels(no readbuffer)");
      return;
   }

   /* Check that the destination format and source buffer are both
    * integer-valued or both non-integer-valued.
    */
   if (ctx->Extensions.EXT_texture_integer && _mesa_is_color_format(format)) {
      const struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
      const GLboolean srcInteger = _mesa_is_format_integer_color(rb->Format);
      const GLboolean dstInteger = _mesa_is_enum_format_integer(format);
      if (dstInteger != srcInteger) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glReadPixels(integer / non-integer format mismatch");
         return;
      }
   }

   if (width == 0 || height == 0)
      return; /* nothing to do */

   if (!_mesa_validate_pbo_access(2, &ctx->Pack, width, height, 1,
                                  format, type, bufSize, pixels)) {
      if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glReadPixels(out of bounds PBO access)");
      } else {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glReadnPixelsARB(out of bounds access:"
                     " bufSize (%d) is too small)", bufSize);
      }
      return;
   }

   if (_mesa_is_bufferobj(ctx->Pack.BufferObj) &&
       _mesa_bufferobj_mapped(ctx->Pack.BufferObj)) {
      /* buffer is mapped - that's an error */
      _mesa_error(ctx, GL_INVALID_OPERATION, "glReadPixels(PBO is mapped)");
      return;
   }

   ctx->Driver.ReadPixels(ctx, x, y, width, height,
			  format, type, &ctx->Pack, pixels);
}

void GLAPIENTRY
_mesa_ReadPixels( GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type, GLvoid *pixels )
{
   _mesa_ReadnPixelsARB(x, y, width, height, format, type, INT_MAX, pixels);
}
