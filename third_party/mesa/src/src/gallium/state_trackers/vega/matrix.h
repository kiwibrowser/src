/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef MATRIX_H
#define MATRIX_H

#include "VG/openvg.h"

#include "pipe/p_compiler.h"
#include "util/u_math.h"

#include <stdio.h>
#include <math.h>

#define floatsEqual(x, y) (fabs(x - y) <= 0.00001f * MIN2(fabs(x), fabs(y)))
#define floatIsZero(x) (floatsEqual((x) + 1, 1))
#define ABS(x) (fabsf(x))

#define DEGREES_TO_RADIANS(d) (0.0174532925199 * (d))
#define FLT_TO_INT(flt) float_to_int_floor(((VGuint*)&(flt))[0])

static INLINE VGint float_to_int_floor(VGuint bits)
{
   int sign = (bits >> 31) ? -1 : 1;
   int exp  = ((bits >> 23) & 255) - 127;
   int mant = bits & 0x007fffff;
   int sh   = 23 - exp;

   /* abs(value) >= 2^31 -> clamp. */

   if (exp >= 31)
      return (VGint)((sign < 0) ? 0x80000000u : 0x7fffffffu);

   /* abs(value) < 1 -> return -1 or 0. */

   if (exp < 0)
      return (sign < 0 && (exp > -127 || mant != 0)) ? -1 : 0;

   /* abs(value) >= 2^23 -> shift left. */

   mant |= 0x00800000;
   if (sh <= 0)
      return sign * (mant << -sh);

   /* Negative -> add a rounding term. */

   if (sign < 0)
      mant += (1 << sh) - 1;

   /* Shift right to obtain the result. */

   return sign * (mant >> sh);
}


struct matrix {
   VGfloat m[9];
};

static INLINE void matrix_init(struct matrix *mat,
                               const VGfloat *val)
{
   memcpy(mat->m, val, sizeof(VGfloat) * 9);
}

static INLINE void matrix_inits(struct matrix *mat,
                                VGfloat m11, VGfloat m12, VGfloat m13,
                                VGfloat m21, VGfloat m22, VGfloat m23,
                                VGfloat m31, VGfloat m32, VGfloat m33)
{
   mat->m[0] = m11; mat->m[1] = m12; mat->m[2] = m13;
   mat->m[3] = m21; mat->m[4] = m22; mat->m[5] = m23;
   mat->m[6] = m31; mat->m[7] = m32; mat->m[8] = m33;
}

static INLINE void matrix_load_identity(struct matrix *matrix)
{
   static const VGfloat identity[9] = {1.f, 0.f, 0.f,
                                       0.f, 1.f, 0.f,
                                       0.f, 0.f, 1.f};
   memcpy(matrix->m, identity, sizeof(identity));
}

static INLINE VGboolean matrix_is_identity(struct matrix *matrix)
{
   return floatsEqual(matrix->m[0], 1) && floatIsZero(matrix->m[1]) &&
      floatIsZero(matrix->m[2]) &&
      floatIsZero(matrix->m[3]) && floatsEqual(matrix->m[4], 1) &&
      floatIsZero(matrix->m[5]) &&
      floatIsZero(matrix->m[6]) && floatIsZero(matrix->m[7]) &&
      floatIsZero(matrix->m[8]);
}

static INLINE VGboolean matrix_is_affine(struct matrix *matrix)
{
   return floatIsZero(matrix->m[2]) && floatIsZero(matrix->m[5])
      && floatsEqual(matrix->m[8], 1);
}


static INLINE void matrix_make_affine(struct matrix *matrix)
{
   matrix->m[2] = 0.f;
   matrix->m[5] = 0.f;
   matrix->m[8] = 1.f;
}

static INLINE void matrix_mult(struct matrix *dst,
                               const struct matrix *src)
{
   VGfloat m11 = dst->m[0]*src->m[0] + dst->m[3]*src->m[1] + dst->m[6]*src->m[2];
   VGfloat m12 = dst->m[0]*src->m[3] + dst->m[3]*src->m[4] + dst->m[6]*src->m[5];
   VGfloat m13 = dst->m[0]*src->m[6] + dst->m[3]*src->m[7] + dst->m[6]*src->m[8];

   VGfloat m21 = dst->m[1]*src->m[0] + dst->m[4]*src->m[1] + dst->m[7]*src->m[2];
   VGfloat m22 = dst->m[1]*src->m[3] + dst->m[4]*src->m[4] + dst->m[7]*src->m[5];
   VGfloat m23 = dst->m[1]*src->m[6] + dst->m[4]*src->m[7] + dst->m[7]*src->m[8];

   VGfloat m31 = dst->m[2]*src->m[0] + dst->m[5]*src->m[1] + dst->m[8]*src->m[2];
   VGfloat m32 = dst->m[2]*src->m[3] + dst->m[5]*src->m[4] + dst->m[8]*src->m[5];
   VGfloat m33 = dst->m[2]*src->m[6] + dst->m[5]*src->m[7] + dst->m[8]*src->m[8];

   dst->m[0] = m11; dst->m[1] = m21; dst->m[2] = m31;
   dst->m[3] = m12; dst->m[4] = m22; dst->m[5] = m32;
   dst->m[6] = m13; dst->m[7] = m23; dst->m[8] = m33;
}


static INLINE void matrix_map_point(struct matrix *mat,
                                    VGfloat x, VGfloat y,
                                    VGfloat *out_x, VGfloat *out_y)
{
   /* to be able to do matrix_map_point(m, x, y, &x, &y) use
    * temporaries */
   VGfloat tmp_x = x, tmp_y = y;

   *out_x = mat->m[0]*tmp_x + mat->m[3]*tmp_y + mat->m[6];
   *out_y = mat->m[1]*tmp_x + mat->m[4]*tmp_y + mat->m[7];
   if (!matrix_is_affine(mat)) {
      VGfloat w = 1/(mat->m[2]*tmp_x + mat->m[5]*tmp_y + mat->m[8]);
      *out_x *= w;
      *out_y *= w;
   }
}

static INLINE void matrix_translate(struct matrix *dst,
                                    VGfloat tx, VGfloat ty)
{
   if (!matrix_is_affine(dst)) {
      struct matrix trans_matrix;
      matrix_load_identity(&trans_matrix);
      trans_matrix.m[6] = tx;
      trans_matrix.m[7] = ty;
      matrix_mult(dst, &trans_matrix);
   } else {
      dst->m[6] += tx*dst->m[0] + ty*dst->m[3];
      dst->m[7] += ty*dst->m[4] + tx*dst->m[1];
   }
}

static INLINE void matrix_scale(struct matrix *dst,
                                VGfloat sx, VGfloat sy)
{
   if (!matrix_is_affine(dst)) {
      struct matrix scale_matrix;
      matrix_load_identity(&scale_matrix);
      scale_matrix.m[0] = sx;
      scale_matrix.m[4] = sy;
      matrix_mult(dst, &scale_matrix);
   } else {
      dst->m[0] *= sx; dst->m[1] *= sx;
      dst->m[3] *= sy; dst->m[4] *= sy;
   }
}

static INLINE void matrix_shear(struct matrix *dst,
                                VGfloat shx, VGfloat shy)
{
   struct matrix shear_matrix;
   matrix_load_identity(&shear_matrix);
   shear_matrix.m[1] = shy;
   shear_matrix.m[3] = shx;
   matrix_mult(dst, &shear_matrix);
}

static INLINE void matrix_rotate(struct matrix *dst,
                                 VGfloat angle)
{
   struct matrix mat;
   float sin_val = 0;
   float cos_val = 0;


   if (floatsEqual(angle, 90) || floatsEqual(angle, -270))
      sin_val = 1.f;
   else if (floatsEqual(angle, 270) || floatsEqual(angle, -90))
      sin_val = -1.f;
   else if (floatsEqual(angle, 180))
      cos_val = -1.f;
   else {
      float radians = DEGREES_TO_RADIANS(angle);
      sin_val = sin(radians);
      cos_val = cos(radians);
   }

   if (!matrix_is_affine(dst)) {
      matrix_load_identity(&mat);
      mat.m[0] =  cos_val;   mat.m[1] =  sin_val;
      mat.m[3] = -sin_val;   mat.m[4] =  cos_val;

      matrix_mult(dst, &mat);
   } else  {
      VGfloat m11 =  cos_val*dst->m[0] + sin_val*dst->m[3];
      VGfloat m12 =  cos_val*dst->m[1] + sin_val*dst->m[4];
      VGfloat m21 = -sin_val*dst->m[0] + cos_val*dst->m[3];
      VGfloat m22 = -sin_val*dst->m[1] + cos_val*dst->m[4];
      dst->m[0] = m11; dst->m[1] = m12;
      dst->m[3] = m21; dst->m[4] = m22;
   }
}


static INLINE VGfloat matrix_determinant(struct matrix *mat)
{
   return mat->m[0]*(mat->m[8]*mat->m[4]-mat->m[7]*mat->m[5]) -
      mat->m[3]*(mat->m[8]*mat->m[1]-mat->m[7]*mat->m[2])+
      mat->m[6]*(mat->m[5]*mat->m[1]-mat->m[4]*mat->m[2]);
}


static INLINE void matrix_adjoint(struct matrix *mat)
{
    VGfloat h[9];
    h[0] = mat->m[4]*mat->m[8] - mat->m[5]*mat->m[7];
    h[3] = mat->m[5]*mat->m[6] - mat->m[3]*mat->m[8];
    h[6] = mat->m[3]*mat->m[7] - mat->m[4]*mat->m[6];
    h[1] = mat->m[2]*mat->m[7] - mat->m[1]*mat->m[8];
    h[4] = mat->m[0]*mat->m[8] - mat->m[2]*mat->m[6];
    h[7] = mat->m[1]*mat->m[6] - mat->m[0]*mat->m[7];
    h[2] = mat->m[1]*mat->m[5] - mat->m[2]*mat->m[4];
    h[5] = mat->m[2]*mat->m[3] - mat->m[0]*mat->m[5];
    h[8] = mat->m[0]*mat->m[4] - mat->m[1]*mat->m[3];


    memcpy(mat->m, h, sizeof(VGfloat) * 9);
}

static INLINE void matrix_divs(struct matrix *mat,
                               VGfloat s)
{
   mat->m[0] /= s;
   mat->m[1] /= s;
   mat->m[2] /= s;
   mat->m[3] /= s;
   mat->m[4] /= s;
   mat->m[5] /= s;
   mat->m[6] /= s;
   mat->m[7] /= s;
   mat->m[8] /= s;
}

static INLINE VGboolean matrix_invert(struct matrix *mat)
{
   VGfloat det = matrix_determinant(mat);

   if (floatIsZero(det))
      return VG_FALSE;

   matrix_adjoint(mat);
   matrix_divs(mat, det);
   return VG_TRUE;
}

static INLINE VGboolean matrix_is_invertible(struct matrix *mat)
{
   return !floatIsZero(matrix_determinant(mat));
}


static INLINE VGboolean matrix_square_to_quad(VGfloat dx0, VGfloat dy0,
                                              VGfloat dx1, VGfloat dy1,
                                              VGfloat dx3, VGfloat dy3,
                                              VGfloat dx2, VGfloat dy2,
                                              struct matrix *mat)
{
   VGfloat ax  = dx0 - dx1 + dx2 - dx3;
   VGfloat ay  = dy0 - dy1 + dy2 - dy3;

   if (floatIsZero(ax) && floatIsZero(ay)) {
      /* affine case */
      matrix_inits(mat,
                   dx1 - dx0, dy1 - dy0,  0,
                   dx2 - dx1, dy2 - dy1,  0,
                         dx0,       dy0,  1);
   } else {
      VGfloat a, b, c, d, e, f, g, h;
      VGfloat ax1 = dx1 - dx2;
      VGfloat ax2 = dx3 - dx2;
      VGfloat ay1 = dy1 - dy2;
      VGfloat ay2 = dy3 - dy2;

      /* determinants */
      VGfloat gtop    =  ax  * ay2 - ax2 * ay;
      VGfloat htop    =  ax1 * ay  - ax  * ay1;
      VGfloat bottom  =  ax1 * ay2 - ax2 * ay1;

      if (!bottom)
         return VG_FALSE;

      g = gtop / bottom;
      h = htop / bottom;

      a = dx1 - dx0 + g * dx1;
      b = dx3 - dx0 + h * dx3;
      c = dx0;
      d = dy1 - dy0 + g * dy1;
      e = dy3 - dy0 + h * dy3;
      f = dy0;

      matrix_inits(mat,
                   a,  d,   g,
                   b,  e,   h,
                   c,  f, 1.f);
   }

   return VG_TRUE;
}

static INLINE VGboolean matrix_quad_to_square(VGfloat sx0, VGfloat sy0,
                                              VGfloat sx1, VGfloat sy1,
                                              VGfloat sx2, VGfloat sy2,
                                              VGfloat sx3, VGfloat sy3,
                                              struct matrix *mat)
{
   if (!matrix_square_to_quad(sx0, sy0, sx1, sy1,
                              sx2, sy2, sx3, sy3,
                              mat))
      return VG_FALSE;

    return matrix_invert(mat);
}


static INLINE VGboolean matrix_quad_to_quad(VGfloat dx0, VGfloat dy0,
                                            VGfloat dx1, VGfloat dy1,
                                            VGfloat dx2, VGfloat dy2,
                                            VGfloat dx3, VGfloat dy3,
                                            VGfloat sx0, VGfloat sy0,
                                            VGfloat sx1, VGfloat sy1,
                                            VGfloat sx2, VGfloat sy2,
                                            VGfloat sx3, VGfloat sy3,
                                            struct matrix *mat)
{
   struct matrix sqr_to_qd;

   if (!matrix_square_to_quad(dx0, dy0, dx1, dy1,
                              dx2, dy2, dx3, dy3,
                              mat))
      return VG_FALSE;

   if (!matrix_quad_to_square(sx0, sy0, sx1, sy1,
                              sx2, sy2, sx3, sy3,
                              &sqr_to_qd))
      return VG_FALSE;

   matrix_mult(mat, &sqr_to_qd);

   return VG_TRUE;
}


static INLINE VGboolean null_line(const VGfloat *l)
{
   return floatsEqual(l[0], l[2]) && floatsEqual(l[1], l[3]);
}

static INLINE void line_normal(float *l, float *norm)
{
   norm[0] = l[0];
   norm[1] = l[1];

   norm[2] = l[0] + (l[3] - l[1]);
   norm[3] = l[1] - (l[2] - l[0]);
}

static INLINE void line_normalize(float *l)
{
   float x = l[2] - l[0];
   float y = l[3] - l[1];
   float len = sqrt(x*x + y*y);
   l[2] = l[0] + x/len;
   l[3] = l[1] + y/len;
}

static INLINE VGfloat line_length(VGfloat x1, VGfloat y1,
                                  VGfloat x2, VGfloat y2)
{
   VGfloat x = x2 - x1;
   VGfloat y = y2 - y1;
   return sqrt(x*x + y*y);
}

static INLINE VGfloat line_lengthv(const VGfloat *l)
{
   VGfloat x = l[2] - l[0];
   VGfloat y = l[3] - l[1];
   return sqrt(x*x + y*y);
}


static INLINE void line_point_at(float *l, float t, float *pt)
{
   float dx = l[2] - l[0];
   float dy = l[3] - l[1];

   pt[0] = l[0] + dx * t;
   pt[1] = l[1] + dy * t;
}

static INLINE void vector_unit(float *vec)
{
   float len = sqrt(vec[0] * vec[0] + vec[1] * vec[1]);
   vec[0] /= len;
   vec[1] /= len;
}

static INLINE void line_normal_vector(float *line, float *vec)
{
   VGfloat normal[4];

   line_normal(line, normal);

   vec[0] = normal[2] - normal[0];
   vec[1] = normal[3] - normal[1];

   vector_unit(vec);
}

#endif
