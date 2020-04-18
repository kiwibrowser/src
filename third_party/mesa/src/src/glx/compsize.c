/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

#include <GL/gl.h>
#include "glxclient.h"

/*
** Return the number of elements per group of a specified format
*/
GLint
__glElementsPerGroup(GLenum format, GLenum type)
{
   /*
    ** To make row length computation valid for image extraction,
    ** packed pixel types assume elements per group equals one.
    */
   switch (type) {
   case GL_UNSIGNED_BYTE_3_3_2:
   case GL_UNSIGNED_BYTE_2_3_3_REV:
   case GL_UNSIGNED_SHORT_5_6_5:
   case GL_UNSIGNED_SHORT_5_6_5_REV:
   case GL_UNSIGNED_SHORT_4_4_4_4:
   case GL_UNSIGNED_SHORT_4_4_4_4_REV:
   case GL_UNSIGNED_SHORT_5_5_5_1:
   case GL_UNSIGNED_SHORT_1_5_5_5_REV:
   case GL_UNSIGNED_SHORT_8_8_APPLE:
   case GL_UNSIGNED_SHORT_8_8_REV_APPLE:
   case GL_UNSIGNED_INT_8_8_8_8:
   case GL_UNSIGNED_INT_8_8_8_8_REV:
   case GL_UNSIGNED_INT_10_10_10_2:
   case GL_UNSIGNED_INT_2_10_10_10_REV:
   case GL_UNSIGNED_INT_24_8_NV:
      return 1;
   default:
      break;
   }

   switch (format) {
   case GL_RGB:
   case GL_BGR:
      return 3;
   case GL_RG:
   case GL_422_EXT:
   case GL_422_REV_EXT:
   case GL_422_AVERAGE_EXT:
   case GL_422_REV_AVERAGE_EXT:
   case GL_DEPTH_STENCIL_NV:
   case GL_YCBCR_422_APPLE:
   case GL_LUMINANCE_ALPHA:
      return 2;
   case GL_RGBA:
   case GL_BGRA:
   case GL_ABGR_EXT:
      return 4;
   case GL_COLOR_INDEX:
   case GL_STENCIL_INDEX:
   case GL_DEPTH_COMPONENT:
   case GL_RED:
   case GL_GREEN:
   case GL_BLUE:
   case GL_ALPHA:
   case GL_LUMINANCE:
   case GL_INTENSITY:
      return 1;
   default:
      return 0;
   }
}

/*
** Return the number of bytes per element, based on the element type (other
** than GL_BITMAP).
*/
GLint
__glBytesPerElement(GLenum type)
{
   switch (type) {
   case GL_UNSIGNED_SHORT:
   case GL_SHORT:
   case GL_UNSIGNED_SHORT_5_6_5:
   case GL_UNSIGNED_SHORT_5_6_5_REV:
   case GL_UNSIGNED_SHORT_4_4_4_4:
   case GL_UNSIGNED_SHORT_4_4_4_4_REV:
   case GL_UNSIGNED_SHORT_5_5_5_1:
   case GL_UNSIGNED_SHORT_1_5_5_5_REV:
   case GL_UNSIGNED_SHORT_8_8_APPLE:
   case GL_UNSIGNED_SHORT_8_8_REV_APPLE:
      return 2;
   case GL_UNSIGNED_BYTE:
   case GL_BYTE:
   case GL_UNSIGNED_BYTE_3_3_2:
   case GL_UNSIGNED_BYTE_2_3_3_REV:
      return 1;
   case GL_INT:
   case GL_UNSIGNED_INT:
   case GL_FLOAT:
   case GL_UNSIGNED_INT_8_8_8_8:
   case GL_UNSIGNED_INT_8_8_8_8_REV:
   case GL_UNSIGNED_INT_10_10_10_2:
   case GL_UNSIGNED_INT_2_10_10_10_REV:
   case GL_UNSIGNED_INT_24_8_NV:
      return 4;
   default:
      return 0;
   }
}

/*
** Compute memory required for internal packed array of data of given type
** and format.
*/
GLint
__glImageSize(GLsizei width, GLsizei height, GLsizei depth,
              GLenum format, GLenum type, GLenum target)
{
   int bytes_per_row;
   int components;

   switch (target) {
   case GL_PROXY_TEXTURE_1D:
   case GL_PROXY_TEXTURE_2D:
   case GL_PROXY_TEXTURE_3D:
   case GL_PROXY_TEXTURE_4D_SGIS:
   case GL_PROXY_TEXTURE_CUBE_MAP:
   case GL_PROXY_TEXTURE_RECTANGLE_ARB:
   case GL_PROXY_HISTOGRAM:
   case GL_PROXY_COLOR_TABLE:
   case GL_PROXY_TEXTURE_COLOR_TABLE_SGI:
   case GL_PROXY_POST_CONVOLUTION_COLOR_TABLE:
   case GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE:
   case GL_PROXY_POST_IMAGE_TRANSFORM_COLOR_TABLE_HP:
      return 0;
   }

   if (width < 0 || height < 0 || depth < 0) {
      return 0;
   }

   /*
    ** Zero is returned if either format or type are invalid.
    */
   components = __glElementsPerGroup(format, type);
   if (type == GL_BITMAP) {
      if (format == GL_COLOR_INDEX || format == GL_STENCIL_INDEX) {
         bytes_per_row = (width + 7) >> 3;
      }
      else {
         return 0;
      }
   }
   else {
      bytes_per_row = __glBytesPerElement(type) * width;
   }

   return bytes_per_row * height * depth * components;
}
