/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

#include "util/u_math.h"
#include "util/u_debug.h"

#include "vl_csc.h"

/*
 * Color space conversion formulas
 *
 * To convert YCbCr to RGB,
 *    vec4  ycbcr, rgb
 *    mat44 csc
 *    rgb = csc * ycbcr
 *
 * To calculate the color space conversion matrix csc with ProcAmp adjustments,
 *    mat44 csc, cstd, procamp, bias
 *    csc = cstd * (procamp * bias)
 *
 * Where cstd is a matrix corresponding to one of the color standards (BT.601, BT.709, etc)
 * adjusted for the kind of YCbCr -> RGB mapping wanted (1:1, full),
 * bias is a matrix corresponding to the kind of YCbCr -> RGB mapping wanted (1:1, full)
 *
 * To calculate procamp,
 *    mat44 procamp, hue, saturation, brightness, contrast
 *    procamp = brightness * (saturation * (contrast * hue))
 * Alternatively,
 *    procamp = saturation * (brightness * (contrast * hue))
 *
 * contrast
 * [ c, 0, 0, 0]
 * [ 0, c, 0, 0]
 * [ 0, 0, c, 0]
 * [ 0, 0, 0, 1]
 *
 * brightness
 * [ 1, 0, 0, b]
 * [ 0, 1, 0, 0]
 * [ 0, 0, 1, 0]
 * [ 0, 0, 0, 1]
 *
 * saturation
 * [ 1, 0, 0, 0]
 * [ 0, s, 0, 0]
 * [ 0, 0, s, 0]
 * [ 0, 0, 0, 1]
 *
 * hue
 * [ 1,       0,      0, 0]
 * [ 0,  cos(h), sin(h), 0]
 * [ 0, -sin(h), cos(h), 0]
 * [ 0,       0,      0, 1]
 *
 * procamp
 * [ c,           0,          0, b]
 * [ 0,  c*s*cos(h), c*s*sin(h), 0]
 * [ 0, -c*s*sin(h), c*s*cos(h), 0]
 * [ 0,           0,          0, 1]
 *
 * bias
 * [ 1, 0, 0,  ybias]
 * [ 0, 1, 0, cbbias]
 * [ 0, 0, 1, crbias]
 * [ 0, 0, 0,      1]
 *
 * csc
 * [ c*cstd[ 0], c*cstd[ 1]*s*cos(h) - c*cstd[ 2]*s*sin(h), c*cstd[ 2]*s*cos(h) + c*cstd[ 1]*s*sin(h), cstd[ 3] + cstd[ 0]*(b + c*ybias) + cstd[ 1]*(c*cbbias*s*cos(h) + c*crbias*s*sin(h)) + cstd[ 2]*(c*crbias*s*cos(h) - c*cbbias*s*sin(h))]
 * [ c*cstd[ 4], c*cstd[ 5]*s*cos(h) - c*cstd[ 6]*s*sin(h), c*cstd[ 6]*s*cos(h) + c*cstd[ 5]*s*sin(h), cstd[ 7] + cstd[ 4]*(b + c*ybias) + cstd[ 5]*(c*cbbias*s*cos(h) + c*crbias*s*sin(h)) + cstd[ 6]*(c*crbias*s*cos(h) - c*cbbias*s*sin(h))]
 * [ c*cstd[ 8], c*cstd[ 9]*s*cos(h) - c*cstd[10]*s*sin(h), c*cstd[10]*s*cos(h) + c*cstd[ 9]*s*sin(h), cstd[11] + cstd[ 8]*(b + c*ybias) + cstd[ 9]*(c*cbbias*s*cos(h) + c*crbias*s*sin(h)) + cstd[10]*(c*crbias*s*cos(h) - c*cbbias*s*sin(h))]
 * [ c*cstd[12], c*cstd[13]*s*cos(h) - c*cstd[14]*s*sin(h), c*cstd[14]*s*cos(h) + c*cstd[13]*s*sin(h), cstd[15] + cstd[12]*(b + c*ybias) + cstd[13]*(c*cbbias*s*cos(h) + c*crbias*s*sin(h)) + cstd[14]*(c*crbias*s*cos(h) - c*cbbias*s*sin(h))]
 */

/*
 * Converts ITU-R BT.601 YCbCr pixels to RGB pixels where:
 * Y is in [16,235], Cb and Cr are in [16,240]
 * R, G, and B are in [16,235]
 */
static const vl_csc_matrix bt_601 =
{
   { 1.0f,  0.0f,    1.371f, 0.0f, },
   { 1.0f, -0.336f, -0.698f, 0.0f, },
   { 1.0f,  1.732f,  0.0f,   0.0f, }
};

/*
 * Converts ITU-R BT.601 YCbCr pixels to RGB pixels where:
 * Y is in [16,235], Cb and Cr are in [16,240]
 * R, G, and B are in [0,255]
 */
static const vl_csc_matrix bt_601_full =
{
   { 1.164f,  0.0f,    1.596f, 0.0f, },
   { 1.164f, -0.391f, -0.813f, 0.0f, },
   { 1.164f,  2.018f,  0.0f,   0.0f, }
};

/*
 * Converts ITU-R BT.709 YCbCr pixels to RGB pixels where:
 * Y is in [16,235], Cb and Cr are in [16,240]
 * R, G, and B are in [16,235]
 */
static const vl_csc_matrix bt_709 =
{
   { 1.0f,  0.0f,    1.540f, 0.0f, },
   { 1.0f, -0.183f, -0.459f, 0.0f, },
   { 1.0f,  1.816f,  0.0f,   0.0f, }
};

/*
 * Converts ITU-R BT.709 YCbCr pixels to RGB pixels where:
 * Y is in [16,235], Cb and Cr are in [16,240]
 * R, G, and B are in [0,255]
 */
static const vl_csc_matrix bt_709_full =
{
   { 1.164f,  0.0f,    1.793f, 0.0f, },
   { 1.164f, -0.213f, -0.534f, 0.0f, },
   { 1.164f,  2.115f,  0.0f,   0.0f, }
};

static const vl_csc_matrix smpte240m =
{
   { 1.0f,  0.0f,    1.582f, 0.0f, },
   { 1.0f, -0.228f, -0.478f, 0.0f, },
   { 1.0f,  1.833f,  0.0f,   0.0f, }
};

static const vl_csc_matrix smpte240m_full =
{
   { 1.164f,  0.0f,    1.794f, 0.0f, },
   { 1.164f, -0.258f, -0.543f, 0.0f, },
   { 1.164f,  2.079f,  0.0f,   0.0f, }
};

static const vl_csc_matrix identity =
{
   { 1.0f, 0.0f, 0.0f, 0.0f, },
   { 0.0f, 1.0f, 0.0f, 0.0f, },
   { 0.0f, 0.0f, 1.0f, 0.0f, }
};

const struct vl_procamp vl_default_procamp = {
   0.0f,  /* brightness */
   1.0f,  /* contrast   */
   1.0f,  /* saturation */
   0.0f   /* hue        */
};

void vl_csc_get_matrix(enum VL_CSC_COLOR_STANDARD cs,
                       struct vl_procamp *procamp,
                       bool full_range,
                       vl_csc_matrix *matrix)
{
   float ybias = full_range ? -16.0f/255.0f : 0.0f;
   float cbbias = -128.0f/255.0f;
   float crbias = -128.0f/255.0f;

   const struct vl_procamp *p = procamp ? procamp : &vl_default_procamp;
   float c = p->contrast;
   float s = p->saturation;
   float b = p->brightness;
   float h = p->hue;

   const vl_csc_matrix *cstd;

   assert(matrix);

   switch (cs) {
      case VL_CSC_COLOR_STANDARD_BT_601:
         cstd = full_range ? &bt_601_full : &bt_601;
         break;
      case VL_CSC_COLOR_STANDARD_BT_709:
         cstd = full_range ? &bt_709_full : &bt_709;
         break;
      case VL_CSC_COLOR_STANDARD_SMPTE_240M:
         cstd = full_range ? &smpte240m_full : &smpte240m;
         break;
      case VL_CSC_COLOR_STANDARD_IDENTITY:
      default:
         assert(cs == VL_CSC_COLOR_STANDARD_IDENTITY);
         memcpy(matrix, identity, sizeof(vl_csc_matrix));
         return;
   }

   (*matrix)[0][0] = c * (*cstd)[0][0];
   (*matrix)[0][1] = c * (*cstd)[0][1] * s * cosf(h) - c * (*cstd)[0][2] * s * sinf(h);
   (*matrix)[0][2] = c * (*cstd)[0][2] * s * cosf(h) + c * (*cstd)[0][1] * s * sinf(h);
   (*matrix)[0][3] = (*cstd)[0][3] + (*cstd)[0][0] * (b + c * ybias) +
                     (*cstd)[0][1] * (c * cbbias * s * cosf(h) + c * crbias * s * sinf(h)) +
                     (*cstd)[0][2] * (c * crbias * s * cosf(h) - c * cbbias * s * sinf(h));

   (*matrix)[1][0] = c * (*cstd)[1][0];
   (*matrix)[1][1] = c * (*cstd)[1][1] * s * cosf(h) - c * (*cstd)[1][2] * s * sinf(h);
   (*matrix)[1][2] = c * (*cstd)[1][2] * s * cosf(h) + c * (*cstd)[1][1] * s * sinf(h);
   (*matrix)[1][3] = (*cstd)[1][3] + (*cstd)[1][0] * (b + c * ybias) +
                     (*cstd)[1][1] * (c * cbbias * s * cosf(h) + c * crbias * s * sinf(h)) +
                     (*cstd)[1][2] * (c * crbias * s * cosf(h) - c * cbbias * s * sinf(h));

   (*matrix)[2][0] = c * (*cstd)[2][0];
   (*matrix)[2][1] = c * (*cstd)[2][1] * s * cosf(h) - c * (*cstd)[2][2] * s * sinf(h);
   (*matrix)[2][2] = c * (*cstd)[2][2] * s * cosf(h) + c * (*cstd)[2][1] * s * sinf(h);
   (*matrix)[2][3] = (*cstd)[2][3] + (*cstd)[2][0] * (b + c * ybias) +
                     (*cstd)[2][1] * (c * cbbias * s * cosf(h) + c * crbias * s * sinf(h)) +
                     (*cstd)[2][2] * (c * crbias * s * cosf(h) - c * cbbias * s * sinf(h));
}
