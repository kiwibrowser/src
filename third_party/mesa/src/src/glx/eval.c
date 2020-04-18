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

#include "packrender.h"

/*
** Routines to pack evaluator maps into the transport buffer.  Maps are
** allowed to have extra arbitrary data, so these routines extract just
** the information that the GL needs.
*/

void
__glFillMap1f(GLint k, GLint order, GLint stride,
              const GLfloat * points, GLubyte * pc)
{
   if (stride == k) {
      /* Just copy the data */
      __GLX_PUT_FLOAT_ARRAY(0, points, order * k);
   }
   else {
      GLint i;

      for (i = 0; i < order; i++) {
         __GLX_PUT_FLOAT_ARRAY(0, points, k);
         points += stride;
         pc += k * __GLX_SIZE_FLOAT32;
      }
   }
}

void
__glFillMap1d(GLint k, GLint order, GLint stride,
              const GLdouble * points, GLubyte * pc)
{
   if (stride == k) {
      /* Just copy the data */
      __GLX_PUT_DOUBLE_ARRAY(0, points, order * k);
   }
   else {
      GLint i;
      for (i = 0; i < order; i++) {
         __GLX_PUT_DOUBLE_ARRAY(0, points, k);
         points += stride;
         pc += k * __GLX_SIZE_FLOAT64;
      }
   }
}

void
__glFillMap2f(GLint k, GLint majorOrder, GLint minorOrder,
              GLint majorStride, GLint minorStride,
              const GLfloat * points, GLfloat * data)
{
   GLint i, j, x;

   if ((minorStride == k) && (majorStride == minorOrder * k)) {
      /* Just copy the data */
      __GLX_MEM_COPY(data, points, majorOrder * majorStride *
                     __GLX_SIZE_FLOAT32);
      return;
   }
   for (i = 0; i < majorOrder; i++) {
      for (j = 0; j < minorOrder; j++) {
         for (x = 0; x < k; x++) {
            data[x] = points[x];
         }
         points += minorStride;
         data += k;
      }
      points += majorStride - minorStride * minorOrder;
   }
}

void
__glFillMap2d(GLint k, GLint majorOrder, GLint minorOrder,
              GLint majorStride, GLint minorStride,
              const GLdouble * points, GLdouble * data)
{
   int i, j, x;

   if ((minorStride == k) && (majorStride == minorOrder * k)) {
      /* Just copy the data */
      __GLX_MEM_COPY(data, points, majorOrder * majorStride *
                     __GLX_SIZE_FLOAT64);
      return;
   }

#ifdef __GLX_ALIGN64
   x = k * __GLX_SIZE_FLOAT64;
#endif
   for (i = 0; i < majorOrder; i++) {
      for (j = 0; j < minorOrder; j++) {
#ifdef __GLX_ALIGN64
         __GLX_MEM_COPY(data, points, x);
#else
         for (x = 0; x < k; x++) {
            data[x] = points[x];
         }
#endif
         points += minorStride;
         data += k;
      }
      points += majorStride - minorStride * minorOrder;
   }
}
