/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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


/**
 * \file image.c
 * Image handling.
 */


#include "glheader.h"
#include "colormac.h"
#include "glformats.h"
#include "image.h"
#include "imports.h"
#include "macros.h"
#include "mfeatures.h"
#include "mtypes.h"



/**
 * Flip the order of the 2 bytes in each word in the given array.
 *
 * \param p array.
 * \param n number of words.
 */
void
_mesa_swap2( GLushort *p, GLuint n )
{
   GLuint i;
   for (i = 0; i < n; i++) {
      p[i] = (p[i] >> 8) | ((p[i] << 8) & 0xff00);
   }
}



/*
 * Flip the order of the 4 bytes in each word in the given array.
 */
void
_mesa_swap4( GLuint *p, GLuint n )
{
   GLuint i, a, b;
   for (i = 0; i < n; i++) {
      b = p[i];
      a =  (b >> 24)
	| ((b >> 8) & 0xff00)
	| ((b << 8) & 0xff0000)
	| ((b << 24) & 0xff000000);
      p[i] = a;
   }
}


/**
 * Return the byte offset of a specific pixel in an image (1D, 2D or 3D).
 *
 * Pixel unpacking/packing parameters are observed according to \p packing.
 *
 * \param dimensions either 1, 2 or 3 to indicate dimensionality of image
 * \param packing  the pixelstore attributes
 * \param width  the image width
 * \param height  the image height
 * \param format  the pixel format (must be validated beforehand)
 * \param type  the pixel data type (must be validated beforehand)
 * \param img  which image in the volume (0 for 1D or 2D images)
 * \param row  row of pixel in the image (0 for 1D images)
 * \param column column of pixel in the image
 *
 * \return offset of pixel.
 *
 * \sa gl_pixelstore_attrib.
 */
GLintptr
_mesa_image_offset( GLuint dimensions,
                    const struct gl_pixelstore_attrib *packing,
                    GLsizei width, GLsizei height,
                    GLenum format, GLenum type,
                    GLint img, GLint row, GLint column )
{
   GLint alignment;        /* 1, 2 or 4 */
   GLint pixels_per_row;
   GLint rows_per_image;
   GLint skiprows;
   GLint skippixels;
   GLint skipimages;       /* for 3-D volume images */
   GLintptr offset;

   ASSERT(dimensions >= 1 && dimensions <= 3);

   alignment = packing->Alignment;
   if (packing->RowLength > 0) {
      pixels_per_row = packing->RowLength;
   }
   else {
      pixels_per_row = width;
   }
   if (packing->ImageHeight > 0) {
      rows_per_image = packing->ImageHeight;
   }
   else {
      rows_per_image = height;
   }

   skippixels = packing->SkipPixels;
   /* Note: SKIP_ROWS _is_ used for 1D images */
   skiprows = packing->SkipRows;
   /* Note: SKIP_IMAGES is only used for 3D images */
   skipimages = (dimensions == 3) ? packing->SkipImages : 0;

   if (type == GL_BITMAP) {
      /* BITMAP data */
      GLint bytes_per_row;
      GLint bytes_per_image;
      /* components per pixel for color or stencil index: */
      const GLint comp_per_pixel = 1;

      /* The pixel type and format should have been error checked earlier */
      assert(format == GL_COLOR_INDEX || format == GL_STENCIL_INDEX);

      bytes_per_row = alignment
                    * CEILING( comp_per_pixel*pixels_per_row, 8*alignment );

      bytes_per_image = bytes_per_row * rows_per_image;

      offset = (skipimages + img) * bytes_per_image
                 + (skiprows + row) * bytes_per_row
                 + (skippixels + column) / 8;
   }
   else {
      /* Non-BITMAP data */
      GLint bytes_per_pixel, bytes_per_row, remainder, bytes_per_image;
      GLint topOfImage;

      bytes_per_pixel = _mesa_bytes_per_pixel( format, type );

      /* The pixel type and format should have been error checked earlier */
      assert(bytes_per_pixel > 0);

      bytes_per_row = pixels_per_row * bytes_per_pixel;
      remainder = bytes_per_row % alignment;
      if (remainder > 0)
         bytes_per_row += (alignment - remainder);

      ASSERT(bytes_per_row % alignment == 0);

      bytes_per_image = bytes_per_row * rows_per_image;

      if (packing->Invert) {
         /* set pixel_addr to the last row */
         topOfImage = bytes_per_row * (height - 1);
         bytes_per_row = -bytes_per_row;
      }
      else {
         topOfImage = 0;
      }

      /* compute final pixel address */
      offset = (skipimages + img) * bytes_per_image
                 + topOfImage
                 + (skiprows + row) * bytes_per_row
                 + (skippixels + column) * bytes_per_pixel;
   }

   return offset;
}


/**
 * Return the address of a specific pixel in an image (1D, 2D or 3D).
 *
 * Pixel unpacking/packing parameters are observed according to \p packing.
 *
 * \param dimensions either 1, 2 or 3 to indicate dimensionality of image
 * \param packing  the pixelstore attributes
 * \param image  starting address of image data
 * \param width  the image width
 * \param height  the image height
 * \param format  the pixel format (must be validated beforehand)
 * \param type  the pixel data type (must be validated beforehand)
 * \param img  which image in the volume (0 for 1D or 2D images)
 * \param row  row of pixel in the image (0 for 1D images)
 * \param column column of pixel in the image
 *
 * \return address of pixel.
 *
 * \sa gl_pixelstore_attrib.
 */
GLvoid *
_mesa_image_address( GLuint dimensions,
                     const struct gl_pixelstore_attrib *packing,
                     const GLvoid *image,
                     GLsizei width, GLsizei height,
                     GLenum format, GLenum type,
                     GLint img, GLint row, GLint column )
{
   const GLubyte *addr = (const GLubyte *) image;

   addr += _mesa_image_offset(dimensions, packing, width, height,
                              format, type, img, row, column);

   return (GLvoid *) addr;
}


GLvoid *
_mesa_image_address1d( const struct gl_pixelstore_attrib *packing,
                       const GLvoid *image,
                       GLsizei width,
                       GLenum format, GLenum type,
                       GLint column )
{
   return _mesa_image_address(1, packing, image, width, 1,
                              format, type, 0, 0, column);
}


GLvoid *
_mesa_image_address2d( const struct gl_pixelstore_attrib *packing,
                       const GLvoid *image,
                       GLsizei width, GLsizei height,
                       GLenum format, GLenum type,
                       GLint row, GLint column )
{
   return _mesa_image_address(2, packing, image, width, height,
                              format, type, 0, row, column);
}


GLvoid *
_mesa_image_address3d( const struct gl_pixelstore_attrib *packing,
                       const GLvoid *image,
                       GLsizei width, GLsizei height,
                       GLenum format, GLenum type,
                       GLint img, GLint row, GLint column )
{
   return _mesa_image_address(3, packing, image, width, height,
                              format, type, img, row, column);
}



/**
 * Compute the stride (in bytes) between image rows.
 *
 * \param packing the pixelstore attributes
 * \param width image width.
 * \param format pixel format.
 * \param type pixel data type.
 * 
 * \return the stride in bytes for the given parameters, or -1 if error
 */
GLint
_mesa_image_row_stride( const struct gl_pixelstore_attrib *packing,
                        GLint width, GLenum format, GLenum type )
{
   GLint bytesPerRow, remainder;

   ASSERT(packing);

   if (type == GL_BITMAP) {
      if (packing->RowLength == 0) {
         bytesPerRow = (width + 7) / 8;
      }
      else {
         bytesPerRow = (packing->RowLength + 7) / 8;
      }
   }
   else {
      /* Non-BITMAP data */
      const GLint bytesPerPixel = _mesa_bytes_per_pixel(format, type);
      if (bytesPerPixel <= 0)
         return -1;  /* error */
      if (packing->RowLength == 0) {
         bytesPerRow = bytesPerPixel * width;
      }
      else {
         bytesPerRow = bytesPerPixel * packing->RowLength;
      }
   }

   remainder = bytesPerRow % packing->Alignment;
   if (remainder > 0) {
      bytesPerRow += (packing->Alignment - remainder);
   }

   if (packing->Invert) {
      /* negate the bytes per row (negative row stride) */
      bytesPerRow = -bytesPerRow;
   }

   return bytesPerRow;
}


/*
 * Compute the stride between images in a 3D texture (in bytes) for the given
 * pixel packing parameters and image width, format and type.
 */
GLint
_mesa_image_image_stride( const struct gl_pixelstore_attrib *packing,
                          GLint width, GLint height,
                          GLenum format, GLenum type )
{
   GLint bytesPerRow, bytesPerImage, remainder;

   ASSERT(packing);

   if (type == GL_BITMAP) {
      if (packing->RowLength == 0) {
         bytesPerRow = (width + 7) / 8;
      }
      else {
         bytesPerRow = (packing->RowLength + 7) / 8;
      }
   }
   else {
      const GLint bytesPerPixel = _mesa_bytes_per_pixel(format, type);

      if (bytesPerPixel <= 0)
         return -1;  /* error */
      if (packing->RowLength == 0) {
         bytesPerRow = bytesPerPixel * width;
      }
      else {
         bytesPerRow = bytesPerPixel * packing->RowLength;
      }
   }

   remainder = bytesPerRow % packing->Alignment;
   if (remainder > 0)
      bytesPerRow += (packing->Alignment - remainder);

   if (packing->ImageHeight == 0)
      bytesPerImage = bytesPerRow * height;
   else
      bytesPerImage = bytesPerRow * packing->ImageHeight;

   return bytesPerImage;
}



/**
 * "Expand" a bitmap from 1-bit per pixel to 8-bits per pixel.
 * This is typically used to convert a bitmap into a GLubyte/pixel texture.
 * "On" bits will set texels to \p onValue.
 * "Off" bits will not modify texels.
 * \param width  src bitmap width in pixels
 * \param height  src bitmap height in pixels
 * \param unpack  bitmap unpacking state
 * \param bitmap  the src bitmap data
 * \param destBuffer  start of dest buffer
 * \param destStride  row stride in dest buffer
 * \param onValue  if bit is 1, set destBuffer pixel to this value
 */
void
_mesa_expand_bitmap(GLsizei width, GLsizei height,
                    const struct gl_pixelstore_attrib *unpack,
                    const GLubyte *bitmap,
                    GLubyte *destBuffer, GLint destStride,
                    GLubyte onValue)
{
   const GLubyte *srcRow = (const GLubyte *)
      _mesa_image_address2d(unpack, bitmap, width, height,
                            GL_COLOR_INDEX, GL_BITMAP, 0, 0);
   const GLint srcStride = _mesa_image_row_stride(unpack, width,
                                                  GL_COLOR_INDEX, GL_BITMAP);
   GLint row, col;

#define SET_PIXEL(COL, ROW) \
   destBuffer[(ROW) * destStride + (COL)] = onValue;

   for (row = 0; row < height; row++) {
      const GLubyte *src = srcRow;

      if (unpack->LsbFirst) {
         /* Lsb first */
         GLubyte mask = 1U << (unpack->SkipPixels & 0x7);
         for (col = 0; col < width; col++) {

            if (*src & mask) {
               SET_PIXEL(col, row);
            }

            if (mask == 128U) {
               src++;
               mask = 1U;
            }
            else {
               mask = mask << 1;
            }
         }

         /* get ready for next row */
         if (mask != 1)
            src++;
      }
      else {
         /* Msb first */
         GLubyte mask = 128U >> (unpack->SkipPixels & 0x7);
         for (col = 0; col < width; col++) {

            if (*src & mask) {
               SET_PIXEL(col, row);
            }

            if (mask == 1U) {
               src++;
               mask = 128U;
            }
            else {
               mask = mask >> 1;
            }
         }

         /* get ready for next row */
         if (mask != 128)
            src++;
      }

      srcRow += srcStride;
   } /* row */

#undef SET_PIXEL
}




/**
 * Convert an array of RGBA colors from one datatype to another.
 * NOTE: src may equal dst.  In that case, we use a temporary buffer.
 */
void
_mesa_convert_colors(GLenum srcType, const GLvoid *src,
                     GLenum dstType, GLvoid *dst,
                     GLuint count, const GLubyte mask[])
{
   GLuint *tempBuffer;
   const GLboolean useTemp = (src == dst);

   tempBuffer = malloc(count * MAX_PIXEL_BYTES);
   if (!tempBuffer)
      return;

   ASSERT(srcType != dstType);

   switch (srcType) {
   case GL_UNSIGNED_BYTE:
      if (dstType == GL_UNSIGNED_SHORT) {
         const GLubyte (*src1)[4] = (const GLubyte (*)[4]) src;
         GLushort (*dst2)[4] = (GLushort (*)[4]) (useTemp ? tempBuffer : dst);
         GLuint i;
         for (i = 0; i < count; i++) {
            if (!mask || mask[i]) {
               dst2[i][RCOMP] = UBYTE_TO_USHORT(src1[i][RCOMP]);
               dst2[i][GCOMP] = UBYTE_TO_USHORT(src1[i][GCOMP]);
               dst2[i][BCOMP] = UBYTE_TO_USHORT(src1[i][BCOMP]);
               dst2[i][ACOMP] = UBYTE_TO_USHORT(src1[i][ACOMP]);
            }
         }
         if (useTemp)
            memcpy(dst, tempBuffer, count * 4 * sizeof(GLushort));
      }
      else {
         const GLubyte (*src1)[4] = (const GLubyte (*)[4]) src;
         GLfloat (*dst4)[4] = (GLfloat (*)[4]) (useTemp ? tempBuffer : dst);
         GLuint i;
         ASSERT(dstType == GL_FLOAT);
         for (i = 0; i < count; i++) {
            if (!mask || mask[i]) {
               dst4[i][RCOMP] = UBYTE_TO_FLOAT(src1[i][RCOMP]);
               dst4[i][GCOMP] = UBYTE_TO_FLOAT(src1[i][GCOMP]);
               dst4[i][BCOMP] = UBYTE_TO_FLOAT(src1[i][BCOMP]);
               dst4[i][ACOMP] = UBYTE_TO_FLOAT(src1[i][ACOMP]);
            }
         }
         if (useTemp)
            memcpy(dst, tempBuffer, count * 4 * sizeof(GLfloat));
      }
      break;
   case GL_UNSIGNED_SHORT:
      if (dstType == GL_UNSIGNED_BYTE) {
         const GLushort (*src2)[4] = (const GLushort (*)[4]) src;
         GLubyte (*dst1)[4] = (GLubyte (*)[4]) (useTemp ? tempBuffer : dst);
         GLuint i;
         for (i = 0; i < count; i++) {
            if (!mask || mask[i]) {
               dst1[i][RCOMP] = USHORT_TO_UBYTE(src2[i][RCOMP]);
               dst1[i][GCOMP] = USHORT_TO_UBYTE(src2[i][GCOMP]);
               dst1[i][BCOMP] = USHORT_TO_UBYTE(src2[i][BCOMP]);
               dst1[i][ACOMP] = USHORT_TO_UBYTE(src2[i][ACOMP]);
            }
         }
         if (useTemp)
            memcpy(dst, tempBuffer, count * 4 * sizeof(GLubyte));
      }
      else {
         const GLushort (*src2)[4] = (const GLushort (*)[4]) src;
         GLfloat (*dst4)[4] = (GLfloat (*)[4]) (useTemp ? tempBuffer : dst);
         GLuint i;
         ASSERT(dstType == GL_FLOAT);
         for (i = 0; i < count; i++) {
            if (!mask || mask[i]) {
               dst4[i][RCOMP] = USHORT_TO_FLOAT(src2[i][RCOMP]);
               dst4[i][GCOMP] = USHORT_TO_FLOAT(src2[i][GCOMP]);
               dst4[i][BCOMP] = USHORT_TO_FLOAT(src2[i][BCOMP]);
               dst4[i][ACOMP] = USHORT_TO_FLOAT(src2[i][ACOMP]);
            }
         }
         if (useTemp)
            memcpy(dst, tempBuffer, count * 4 * sizeof(GLfloat));
      }
      break;
   case GL_FLOAT:
      if (dstType == GL_UNSIGNED_BYTE) {
         const GLfloat (*src4)[4] = (const GLfloat (*)[4]) src;
         GLubyte (*dst1)[4] = (GLubyte (*)[4]) (useTemp ? tempBuffer : dst);
         GLuint i;
         for (i = 0; i < count; i++) {
            if (!mask || mask[i])
               _mesa_unclamped_float_rgba_to_ubyte(dst1[i], src4[i]);
         }
         if (useTemp)
            memcpy(dst, tempBuffer, count * 4 * sizeof(GLubyte));
      }
      else {
         const GLfloat (*src4)[4] = (const GLfloat (*)[4]) src;
         GLushort (*dst2)[4] = (GLushort (*)[4]) (useTemp ? tempBuffer : dst);
         GLuint i;
         ASSERT(dstType == GL_UNSIGNED_SHORT);
         for (i = 0; i < count; i++) {
            if (!mask || mask[i]) {
               UNCLAMPED_FLOAT_TO_USHORT(dst2[i][RCOMP], src4[i][RCOMP]);
               UNCLAMPED_FLOAT_TO_USHORT(dst2[i][GCOMP], src4[i][GCOMP]);
               UNCLAMPED_FLOAT_TO_USHORT(dst2[i][BCOMP], src4[i][BCOMP]);
               UNCLAMPED_FLOAT_TO_USHORT(dst2[i][ACOMP], src4[i][ACOMP]);
            }
         }
         if (useTemp)
            memcpy(dst, tempBuffer, count * 4 * sizeof(GLushort));
      }
      break;
   default:
      _mesa_problem(NULL, "Invalid datatype in _mesa_convert_colors");
   }

   free(tempBuffer);
}




/**
 * Perform basic clipping for glDrawPixels.  The image's position and size
 * and the unpack SkipPixels and SkipRows are adjusted so that the image
 * region is entirely within the window and scissor bounds.
 * NOTE: this will only work when glPixelZoom is (1, 1) or (1, -1).
 * If Pixel.ZoomY is -1, *destY will be changed to be the first row which
 * we'll actually write.  Beforehand, *destY-1 is the first drawing row.
 *
 * \return  GL_TRUE if image is ready for drawing or
 *          GL_FALSE if image was completely clipped away (draw nothing)
 */
GLboolean
_mesa_clip_drawpixels(const struct gl_context *ctx,
                      GLint *destX, GLint *destY,
                      GLsizei *width, GLsizei *height,
                      struct gl_pixelstore_attrib *unpack)
{
   const struct gl_framebuffer *buffer = ctx->DrawBuffer;

   if (unpack->RowLength == 0) {
      unpack->RowLength = *width;
   }

   ASSERT(ctx->Pixel.ZoomX == 1.0F);
   ASSERT(ctx->Pixel.ZoomY == 1.0F || ctx->Pixel.ZoomY == -1.0F);

   /* left clipping */
   if (*destX < buffer->_Xmin) {
      unpack->SkipPixels += (buffer->_Xmin - *destX);
      *width -= (buffer->_Xmin - *destX);
      *destX = buffer->_Xmin;
   }
   /* right clipping */
   if (*destX + *width > buffer->_Xmax)
      *width -= (*destX + *width - buffer->_Xmax);

   if (*width <= 0)
      return GL_FALSE;

   if (ctx->Pixel.ZoomY == 1.0F) {
      /* bottom clipping */
      if (*destY < buffer->_Ymin) {
         unpack->SkipRows += (buffer->_Ymin - *destY);
         *height -= (buffer->_Ymin - *destY);
         *destY = buffer->_Ymin;
      }
      /* top clipping */
      if (*destY + *height > buffer->_Ymax)
         *height -= (*destY + *height - buffer->_Ymax);
   }
   else { /* upside down */
      /* top clipping */
      if (*destY > buffer->_Ymax) {
         unpack->SkipRows += (*destY - buffer->_Ymax);
         *height -= (*destY - buffer->_Ymax);
         *destY = buffer->_Ymax;
      }
      /* bottom clipping */
      if (*destY - *height < buffer->_Ymin)
         *height -= (buffer->_Ymin - (*destY - *height));
      /* adjust destY so it's the first row to write to */
      (*destY)--;
   }

   if (*height <= 0)
      return GL_FALSE;

   return GL_TRUE;
}


/**
 * Perform clipping for glReadPixels.  The image's window position
 * and size, and the pack skipPixels, skipRows and rowLength are adjusted
 * so that the image region is entirely within the window bounds.
 * Note: this is different from _mesa_clip_drawpixels() in that the
 * scissor box is ignored, and we use the bounds of the current readbuffer
 * surface.
 *
 * \return  GL_TRUE if region to read is in bounds
 *          GL_FALSE if region is completely out of bounds (nothing to read)
 */
GLboolean
_mesa_clip_readpixels(const struct gl_context *ctx,
                      GLint *srcX, GLint *srcY,
                      GLsizei *width, GLsizei *height,
                      struct gl_pixelstore_attrib *pack)
{
   const struct gl_framebuffer *buffer = ctx->ReadBuffer;

   if (pack->RowLength == 0) {
      pack->RowLength = *width;
   }

   /* left clipping */
   if (*srcX < 0) {
      pack->SkipPixels += (0 - *srcX);
      *width -= (0 - *srcX);
      *srcX = 0;
   }
   /* right clipping */
   if (*srcX + *width > (GLsizei) buffer->Width)
      *width -= (*srcX + *width - buffer->Width);

   if (*width <= 0)
      return GL_FALSE;

   /* bottom clipping */
   if (*srcY < 0) {
      pack->SkipRows += (0 - *srcY);
      *height -= (0 - *srcY);
      *srcY = 0;
   }
   /* top clipping */
   if (*srcY + *height > (GLsizei) buffer->Height)
      *height -= (*srcY + *height - buffer->Height);

   if (*height <= 0)
      return GL_FALSE;

   return GL_TRUE;
}


/**
 * Do clipping for a glCopyTexSubImage call.
 * The framebuffer source region might extend outside the framebuffer
 * bounds.  Clip the source region against the framebuffer bounds and
 * adjust the texture/dest position and size accordingly.
 *
 * \return GL_FALSE if region is totally clipped, GL_TRUE otherwise.
 */
GLboolean
_mesa_clip_copytexsubimage(const struct gl_context *ctx,
                           GLint *destX, GLint *destY,
                           GLint *srcX, GLint *srcY,
                           GLsizei *width, GLsizei *height)
{
   const struct gl_framebuffer *fb = ctx->ReadBuffer;
   const GLint srcX0 = *srcX, srcY0 = *srcY;

   if (_mesa_clip_to_region(0, 0, fb->Width, fb->Height,
                            srcX, srcY, width, height)) {
      *destX = *destX + *srcX - srcX0;
      *destY = *destY + *srcY - srcY0;

      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}



/**
 * Clip the rectangle defined by (x, y, width, height) against the bounds
 * specified by [xmin, xmax) and [ymin, ymax).
 * \return GL_FALSE if rect is totally clipped, GL_TRUE otherwise.
 */
GLboolean
_mesa_clip_to_region(GLint xmin, GLint ymin,
                     GLint xmax, GLint ymax,
                     GLint *x, GLint *y,
                     GLsizei *width, GLsizei *height )
{
   /* left clipping */
   if (*x < xmin) {
      *width -= (xmin - *x);
      *x = xmin;
   }

   /* right clipping */
   if (*x + *width > xmax)
      *width -= (*x + *width - xmax);

   if (*width <= 0)
      return GL_FALSE;

   /* bottom (or top) clipping */
   if (*y < ymin) {
      *height -= (ymin - *y);
      *y = ymin;
   }

   /* top (or bottom) clipping */
   if (*y + *height > ymax)
      *height -= (*y + *height - ymax);

   if (*height <= 0)
      return GL_FALSE;

   return GL_TRUE;
}


/**
 * Clip dst coords against Xmax (or Ymax).
 */
static inline void
clip_right_or_top(GLint *srcX0, GLint *srcX1,
                  GLint *dstX0, GLint *dstX1,
                  GLint maxValue)
{
   GLfloat t, bias;

   if (*dstX1 > maxValue) {
      /* X1 outside right edge */
      ASSERT(*dstX0 < maxValue); /* X0 should be inside right edge */
      t = (GLfloat) (maxValue - *dstX0) / (GLfloat) (*dstX1 - *dstX0);
      /* chop off [t, 1] part */
      ASSERT(t >= 0.0 && t <= 1.0);
      *dstX1 = maxValue;
      bias = (*srcX0 < *srcX1) ? 0.5F : -0.5F;
      *srcX1 = *srcX0 + (GLint) (t * (*srcX1 - *srcX0) + bias);
   }
   else if (*dstX0 > maxValue) {
      /* X0 outside right edge */
      ASSERT(*dstX1 < maxValue); /* X1 should be inside right edge */
      t = (GLfloat) (maxValue - *dstX1) / (GLfloat) (*dstX0 - *dstX1);
      /* chop off [t, 1] part */
      ASSERT(t >= 0.0 && t <= 1.0);
      *dstX0 = maxValue;
      bias = (*srcX0 < *srcX1) ? -0.5F : 0.5F;
      *srcX0 = *srcX1 + (GLint) (t * (*srcX0 - *srcX1) + bias);
   }
}


/**
 * Clip dst coords against Xmin (or Ymin).
 */
static inline void
clip_left_or_bottom(GLint *srcX0, GLint *srcX1,
                    GLint *dstX0, GLint *dstX1,
                    GLint minValue)
{
   GLfloat t, bias;

   if (*dstX0 < minValue) {
      /* X0 outside left edge */
      ASSERT(*dstX1 > minValue); /* X1 should be inside left edge */
      t = (GLfloat) (minValue - *dstX0) / (GLfloat) (*dstX1 - *dstX0);
      /* chop off [0, t] part */
      ASSERT(t >= 0.0 && t <= 1.0);
      *dstX0 = minValue;
      bias = (*srcX0 < *srcX1) ? 0.5F : -0.5F; /* flipped??? */
      *srcX0 = *srcX0 + (GLint) (t * (*srcX1 - *srcX0) + bias);
   }
   else if (*dstX1 < minValue) {
      /* X1 outside left edge */
      ASSERT(*dstX0 > minValue); /* X0 should be inside left edge */
      t = (GLfloat) (minValue - *dstX1) / (GLfloat) (*dstX0 - *dstX1);
      /* chop off [0, t] part */
      ASSERT(t >= 0.0 && t <= 1.0);
      *dstX1 = minValue;
      bias = (*srcX0 < *srcX1) ? 0.5F : -0.5F;
      *srcX1 = *srcX1 + (GLint) (t * (*srcX0 - *srcX1) + bias);
   }
}


/**
 * Do clipping of blit src/dest rectangles.
 * The dest rect is clipped against both the buffer bounds and scissor bounds.
 * The src rect is just clipped against the buffer bounds.
 *
 * When either the src or dest rect is clipped, the other is also clipped
 * proportionately!
 *
 * Note that X0 need not be less than X1 (same for Y) for either the source
 * and dest rects.  That makes the clipping a little trickier.
 *
 * \return GL_TRUE if anything is left to draw, GL_FALSE if totally clipped
 */
GLboolean
_mesa_clip_blit(struct gl_context *ctx,
                GLint *srcX0, GLint *srcY0, GLint *srcX1, GLint *srcY1,
                GLint *dstX0, GLint *dstY0, GLint *dstX1, GLint *dstY1)
{
   const GLint srcXmin = 0;
   const GLint srcXmax = ctx->ReadBuffer->Width;
   const GLint srcYmin = 0;
   const GLint srcYmax = ctx->ReadBuffer->Height;

   /* these include scissor bounds */
   const GLint dstXmin = ctx->DrawBuffer->_Xmin;
   const GLint dstXmax = ctx->DrawBuffer->_Xmax;
   const GLint dstYmin = ctx->DrawBuffer->_Ymin;
   const GLint dstYmax = ctx->DrawBuffer->_Ymax;

   /*
   printf("PreClipX:  src: %d .. %d  dst: %d .. %d\n",
          *srcX0, *srcX1, *dstX0, *dstX1);
   printf("PreClipY:  src: %d .. %d  dst: %d .. %d\n",
          *srcY0, *srcY1, *dstY0, *dstY1);
   */

   /* trivial rejection tests */
   if (*dstX0 == *dstX1)
      return GL_FALSE; /* no width */
   if (*dstX0 <= dstXmin && *dstX1 <= dstXmin)
      return GL_FALSE; /* totally out (left) of bounds */
   if (*dstX0 >= dstXmax && *dstX1 >= dstXmax)
      return GL_FALSE; /* totally out (right) of bounds */

   if (*dstY0 == *dstY1)
      return GL_FALSE;
   if (*dstY0 <= dstYmin && *dstY1 <= dstYmin)
      return GL_FALSE;
   if (*dstY0 >= dstYmax && *dstY1 >= dstYmax)
      return GL_FALSE;

   if (*srcX0 == *srcX1)
      return GL_FALSE;
   if (*srcX0 <= srcXmin && *srcX1 <= srcXmin)
      return GL_FALSE;
   if (*srcX0 >= srcXmax && *srcX1 >= srcXmax)
      return GL_FALSE;

   if (*srcY0 == *srcY1)
      return GL_FALSE;
   if (*srcY0 <= srcYmin && *srcY1 <= srcYmin)
      return GL_FALSE;
   if (*srcY0 >= srcYmax && *srcY1 >= srcYmax)
      return GL_FALSE;

   /*
    * dest clip
    */
   clip_right_or_top(srcX0, srcX1, dstX0, dstX1, dstXmax);
   clip_right_or_top(srcY0, srcY1, dstY0, dstY1, dstYmax);
   clip_left_or_bottom(srcX0, srcX1, dstX0, dstX1, dstXmin);
   clip_left_or_bottom(srcY0, srcY1, dstY0, dstY1, dstYmin);

   /*
    * src clip (just swap src/dst values from above)
    */
   clip_right_or_top(dstX0, dstX1, srcX0, srcX1, srcXmax);
   clip_right_or_top(dstY0, dstY1, srcY0, srcY1, srcYmax);
   clip_left_or_bottom(dstX0, dstX1, srcX0, srcX1, srcXmin);
   clip_left_or_bottom(dstY0, dstY1, srcY0, srcY1, srcYmin);

   /*
   printf("PostClipX: src: %d .. %d  dst: %d .. %d\n",
          *srcX0, *srcX1, *dstX0, *dstX1);
   printf("PostClipY: src: %d .. %d  dst: %d .. %d\n",
          *srcY0, *srcY1, *dstY0, *dstY1);
   */

   ASSERT(*dstX0 >= dstXmin);
   ASSERT(*dstX0 <= dstXmax);
   ASSERT(*dstX1 >= dstXmin);
   ASSERT(*dstX1 <= dstXmax);

   ASSERT(*dstY0 >= dstYmin);
   ASSERT(*dstY0 <= dstYmax);
   ASSERT(*dstY1 >= dstYmin);
   ASSERT(*dstY1 <= dstYmax);

   ASSERT(*srcX0 >= srcXmin);
   ASSERT(*srcX0 <= srcXmax);
   ASSERT(*srcX1 >= srcXmin);
   ASSERT(*srcX1 <= srcXmax);

   ASSERT(*srcY0 >= srcYmin);
   ASSERT(*srcY0 <= srcYmax);
   ASSERT(*srcY1 >= srcYmin);
   ASSERT(*srcY1 <= srcYmax);

   return GL_TRUE;
}
