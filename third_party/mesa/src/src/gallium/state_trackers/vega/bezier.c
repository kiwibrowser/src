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

#include "bezier.h"

#include "matrix.h"
#include "polygon.h"

#include "pipe/p_compiler.h"
#include "util/u_debug.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

static const float flatness = 0.5;


static INLINE void split_left(struct bezier *bez, VGfloat t, struct bezier* left)
{
    left->x1 = bez->x1;
    left->y1 = bez->y1;

    left->x2 = bez->x1 + t * (bez->x2 - bez->x1);
    left->y2 = bez->y1 + t * (bez->y2 - bez->y1);

    left->x3 = bez->x2 + t * (bez->x3 - bez->x2);
    left->y3 = bez->y2 + t * (bez->y3 - bez->y2);

    bez->x3 = bez->x3 + t * (bez->x4 - bez->x3);
    bez->y3 = bez->y3 + t * (bez->y4 - bez->y3);

    bez->x2 = left->x3 + t * (bez->x3 - left->x3);
    bez->y2 = left->y3 + t * (bez->y3 - left->y3);

    left->x3 = left->x2 + t * (left->x3 - left->x2);
    left->y3 = left->y2 + t * (left->y3 - left->y2);

    left->x4 = bez->x1 = left->x3 + t * (bez->x2 - left->x3);
    left->y4 = bez->y1 = left->y3 + t * (bez->y2 - left->y3);
}

static INLINE void split(struct bezier *bez,
                         struct bezier *first_half,
                         struct bezier *second_half)
{
   double c         = (bez->x2 + bez->x3) * 0.5;
   first_half->x2  = (bez->x1 + bez->x2) * 0.5;
   second_half->x3 = (bez->x3 + bez->x4) * 0.5;
   first_half->x1  = bez->x1;
   second_half->x4 = bez->x4;
   first_half->x3  = (first_half->x2 + c) * 0.5;
   second_half->x2 = (second_half->x3 + c) * 0.5;
   first_half->x4  = second_half->x1 =
                     (first_half->x3 + second_half->x2) * 0.5;

   c = (bez->y2 + bez->y3) / 2;
   first_half->y2  = (bez->y1 + bez->y2) * 0.5;
   second_half->y3 = (bez->y3 + bez->y4) * 0.5;
   first_half->y1  = bez->y1;
   second_half->y4 = bez->y4;
   first_half->y3  = (first_half->y2 + c) * 0.5;
   second_half->y2 = (second_half->y3 + c) * 0.5;
   first_half->y4  = second_half->y1 =
                     (first_half->y3 + second_half->y2) * 0.5;
}

struct polygon * bezier_to_polygon(struct bezier *bez)
{
   struct polygon *poly = polygon_create(64);
   polygon_vertex_append(poly, bez->x1, bez->y1);
   bezier_add_to_polygon(bez, poly);
   return poly;
}

void bezier_add_to_polygon(const struct bezier *bez,
                           struct polygon *poly)
{
   struct bezier beziers[32];
   struct bezier *b;

   beziers[0] = *bez;
   b = beziers;

   while (b >= beziers) {
      double y4y1 = b->y4 - b->y1;
      double x4x1 = b->x4 - b->x1;
      double l = ABS(x4x1) + ABS(y4y1);
      double d;
      if (l > 1.f) {
         d = ABS((x4x1)*(b->y1 - b->y2) - (y4y1)*(b->x1 - b->x2))
             + ABS((x4x1)*(b->y1 - b->y3) - (y4y1)*(b->x1 - b->x3));
      } else {
         d = ABS(b->x1 - b->x2) + ABS(b->y1 - b->y2) +
             ABS(b->x1 - b->x3) + ABS(b->y1 - b->y3);
         l = 1.;
      }
      if (d < flatness*l || b == beziers + 31) {
         /* good enough, we pop it off and add the endpoint */
         polygon_vertex_append(poly, b->x4, b->y4);
         --b;
      } else {
         /* split, second half of the bezier goes lower into the stack */
         split(b, b+1, b);
         ++b;
      }
   }
}

static void add_if_close(struct bezier *bez, VGfloat *length, VGfloat error)
{
   struct bezier left, right;     /* bez poly splits */
   VGfloat len = 0.0;        /* arc length */
   VGfloat chord;            /* chord length */

   len = len + line_length(bez->x1, bez->y1, bez->x2, bez->y2);
   len = len + line_length(bez->x2, bez->y2, bez->x3, bez->y3);
   len = len + line_length(bez->x3, bez->y3, bez->x4, bez->y4);

   chord = line_length(bez->x1, bez->y1, bez->x4, bez->y4);

   if ((len-chord) > error) {
      split(bez, &left, &right);                 /* split in two */
      add_if_close(&left, length, error);       /* try left side */
      add_if_close(&right, length, error);      /* try right side */
      return;
   }

   *length = *length + len;

   return;
}

float bezier_length(struct bezier *bez, float error)
{
   VGfloat length = 0.f;

   add_if_close(bez, &length, error);
   return length;
}

void bezier_init(struct bezier *bez,
                 float x1, float y1,
                 float x2, float y2,
                 float x3, float y3,
                 float x4, float y4)
{
   bez->x1 = x1;
   bez->y1 = y1;
   bez->x2 = x2;
   bez->y2 = y2;
   bez->x3 = x3;
   bez->y3 = y3;
   bez->x4 = x4;
   bez->y4 = y4;
#if 0
   debug_printf("bezier in [%f, %f, %f, %f, %f, %f]\n",
                x1, y1, x2, y2, x3, y3, x4, y4);
#endif
}


static INLINE void bezier_init2v(struct bezier *bez,
                                 float *pt1,
                                 float *pt2,
                                 float *pt3,
                                 float *pt4)
{
   bez->x1 = pt1[0];
   bez->y1 = pt1[1];

   bez->x2 = pt2[0];
   bez->y2 = pt2[1];

   bez->x3 = pt3[0];
   bez->y3 = pt3[1];

   bez->x4 = pt4[0];
   bez->y4 = pt4[1];
}


void bezier_transform(struct bezier *bez,
                      struct matrix *matrix)
{
   assert(matrix_is_affine(matrix));
   matrix_map_point(matrix, bez->x1, bez->y1, &bez->x1, &bez->y1);
   matrix_map_point(matrix, bez->x2, bez->y2, &bez->x2, &bez->y2);
   matrix_map_point(matrix, bez->x3, bez->y3, &bez->x3, &bez->y3);
   matrix_map_point(matrix, bez->x4, bez->y4, &bez->x4, &bez->y4);
}

static INLINE void bezier_point_at(const struct bezier *bez, float t, float *pt)
{
   float a, b, c, d;
   float m_t;
   m_t = 1. - t;
   b = m_t * m_t;
   c = t * t;
   d = c * t;
   a = b * m_t;
   b *= 3. * t;
   c *= 3. * m_t;
   pt[0] = a*bez->x1 + b*bez->x2 + c*bez->x3 + d*bez->x4;
   pt[1] = a*bez->y1 + b*bez->y2 + c*bez->y3 + d*bez->y4;
}

static INLINE void bezier_normal_at(const struct bezier *bez, float t, float *norm)
{
   float m_t = 1. - t;
   float a = m_t * m_t;
   float b = t * m_t;
   float c = t * t;

   norm[0] =  (bez->y2-bez->y1) * a + (bez->y3-bez->y2) * b + (bez->y4-bez->y3) * c;
   norm[1] = -(bez->x2-bez->x1) * a - (bez->x3-bez->x2) * b - (bez->x4-bez->x3) * c;
}

enum shift_result {
   Ok,
   Discard,
   Split,
   Circle
};

static enum shift_result good_offset(const struct bezier *b1,
                                     const struct bezier *b2,
                                     float offset, float threshold)
{
   const float o2 = offset*offset;
   const float max_dist_line = threshold*offset*offset;
   const float max_dist_normal = threshold*offset;
   const float spacing = 0.25;
   float i;
   for (i = spacing; i < 0.99; i += spacing) {
      float p1[2],p2[2], d, l;
      float normal[2];
      bezier_point_at(b1, i, p1);
      bezier_point_at(b2, i, p2);
      d = (p1[0] - p2[0])*(p1[0] - p2[0]) + (p1[1] - p2[1])*(p1[1] - p2[1]);
      if (ABS(d - o2) > max_dist_line)
         return Split;

      bezier_normal_at(b1, i, normal);
      l = ABS(normal[0]) + ABS(normal[1]);
      if (l != 0.) {
         d = ABS(normal[0]*(p1[1] - p2[1]) - normal[1]*(p1[0] - p2[0]) ) / l;
         if (d > max_dist_normal)
            return Split;
      }
   }
   return Ok;
}

static INLINE void shift_line_by_normal(float *l, float offset)
{
   float norm[4];
   float tx, ty;

   line_normal(l, norm);
   line_normalize(norm);

   tx = (norm[2] - norm[0]) * offset;
   ty = (norm[3] - norm[1]) * offset;
   l[0] += tx; l[1] += ty;
   l[2] += tx; l[3] += ty;
}

static INLINE VGboolean is_bezier_line(float (*points)[2], int count)
{
   float dx13 = points[2][0] - points[0][0];
   float dy13 = points[2][1] - points[0][1];

   float dx12 = points[1][0] - points[0][0];
   float dy12 = points[1][1] - points[0][1];

   debug_assert(count > 2);

   if (count == 3) {
      return floatsEqual(dx12 * dy13, dx13 * dy12);
   } else if (count == 4) {
      float dx14 = points[3][0] - points[0][0];
      float dy14 = points[3][1] - points[0][1];

      return (floatsEqual(dx12 * dy13, dx13 * dy12) &&
              floatsEqual(dx12 * dy14, dx14 * dy12));
   }

   return VG_FALSE;
}

static INLINE void compute_pt_normal(float *pt1, float *pt2, float *res)
{
   float line[4];
   float normal[4];
   line[0] = 0.f; line[1] = 0.f;
   line[2] = pt2[0] - pt1[0];
   line[3] = pt2[1] - pt1[1];
   line_normal(line, normal);
   line_normalize(normal);

   res[0] = normal[2];
   res[1] = normal[3];
}

static enum shift_result shift(const struct bezier *orig,
                               struct bezier *shifted,
                               float offset, float threshold)
{
   int i;
   int map[4];
   VGboolean p1_p2_equal = (orig->x1 == orig->x2 && orig->y1 == orig->y2);
   VGboolean p2_p3_equal = (orig->x2 == orig->x3 && orig->y2 == orig->y3);
   VGboolean p3_p4_equal = (orig->x3 == orig->x4 && orig->y3 == orig->y4);

   float points[4][2];
   int np = 0;
   float bounds[4];
   float points_shifted[4][2];
   float prev_normal[2];

   points[np][0] = orig->x1;
   points[np][1] = orig->y1;
   map[0] = 0;
   ++np;
   if (!p1_p2_equal) {
      points[np][0] = orig->x2;
      points[np][1] = orig->y2;
      ++np;
   }
   map[1] = np - 1;
   if (!p2_p3_equal) {
      points[np][0] = orig->x3;
      points[np][1] = orig->y3;
      ++np;
   }
   map[2] = np - 1;
   if (!p3_p4_equal) {
      points[np][0] = orig->x4;
      points[np][1] = orig->y4;
      ++np;
   }
   map[3] = np - 1;
   if (np == 1)
      return Discard;

   /* We need to specialcase lines of 3 or 4 points due to numerical
      instability in intersection code below */
   if (np > 2 && is_bezier_line(points, np)) {
      float l[4] = { points[0][0], points[0][1],
                     points[np-1][0], points[np-1][1] };
      float ctrl1[2], ctrl2[2];
      if (floatsEqual(points[0][0], points[np-1][0]) &&
          floatsEqual(points[0][1], points[np-1][1]))
         return Discard;

      shift_line_by_normal(l, offset);
      line_point_at(l, 0.33, ctrl1);
      line_point_at(l, 0.66, ctrl2);
      bezier_init(shifted, l[0], l[1],
                  ctrl1[0], ctrl1[1], ctrl2[0], ctrl2[1],
                  l[2], l[3]);
      return Ok;
   }

   bezier_bounds(orig, bounds);
   if (np == 4 && bounds[2] < .1*offset && bounds[3] < .1*offset) {
      float l = (orig->x1 - orig->x2)*(orig->x1 - orig->x2) +
                (orig->y1 - orig->y2)*(orig->y1 - orig->y1) *
                (orig->x3 - orig->x4)*(orig->x3 - orig->x4) +
                (orig->y3 - orig->y4)*(orig->y3 - orig->y4);
      float dot = (orig->x1 - orig->x2)*(orig->x3 - orig->x4) +
                  (orig->y1 - orig->y2)*(orig->y3 - orig->y4);
      if (dot < 0 && dot*dot < 0.8*l)
         /* the points are close and reverse dirction. Approximate the whole
            thing by a semi circle */
         return Circle;
   }

   compute_pt_normal(points[0], points[1], prev_normal);

   points_shifted[0][0] = points[0][0] + offset * prev_normal[0];
   points_shifted[0][1] = points[0][1] + offset * prev_normal[1];

   for (i = 1; i < np - 1; ++i) {
      float normal_sum[2], r;
      float next_normal[2];
      compute_pt_normal(points[i], points[i + 1], next_normal);

      normal_sum[0] = prev_normal[0] + next_normal[0];
      normal_sum[1] = prev_normal[1] + next_normal[1];

      r = 1.0 + prev_normal[0] * next_normal[0]
          + prev_normal[1] * next_normal[1];

      if (floatsEqual(r + 1, 1)) {
         points_shifted[i][0] = points[i][0] + offset * prev_normal[0];
         points_shifted[i][1] = points[i][1] + offset * prev_normal[1];
      } else {
         float k = offset / r;
         points_shifted[i][0] = points[i][0] + k * normal_sum[0];
         points_shifted[i][1] = points[i][1] + k * normal_sum[1];
      }

      prev_normal[0] = next_normal[0];
      prev_normal[1] = next_normal[1];
   }

   points_shifted[np - 1][0] = points[np - 1][0] + offset * prev_normal[0];
   points_shifted[np - 1][1] = points[np - 1][1] + offset * prev_normal[1];

   bezier_init2v(shifted,
                 points_shifted[map[0]], points_shifted[map[1]],
                 points_shifted[map[2]], points_shifted[map[3]]);

   return good_offset(orig, shifted, offset, threshold);
}

static VGboolean make_circle(const struct bezier *b, float offset, struct bezier *o)
{
   float normals[3][2];
   float dist;
   float angles[2];
   float sign = 1.f;
   int i;
   float circle[3][2];

   normals[0][0] = b->y2 - b->y1;
   normals[0][1] = b->x1 - b->x2;
   dist = sqrt(normals[0][0]*normals[0][0] + normals[0][1]*normals[0][1]);
   if (floatsEqual(dist + 1, 1.f))
      return VG_FALSE;
   normals[0][0] /= dist;
   normals[0][1] /= dist;

   normals[2][0] = b->y4 - b->y3;
   normals[2][1] = b->x3 - b->x4;
   dist = sqrt(normals[2][0]*normals[2][0] + normals[2][1]*normals[2][1]);
   if (floatsEqual(dist + 1, 1.f))
      return VG_FALSE;
   normals[2][0] /= dist;
   normals[2][1] /= dist;

   normals[1][0] = b->x1 - b->x2 - b->x3 + b->x4;
   normals[1][1] = b->y1 - b->y2 - b->y3 + b->y4;
   dist = -1*sqrt(normals[1][0]*normals[1][0] + normals[1][1]*normals[1][1]);
   normals[1][0] /= dist;
   normals[1][1] /= dist;

   for (i = 0; i < 2; ++i) {
      float cos_a = normals[i][0]*normals[i+1][0] + normals[i][1]*normals[i+1][1];
      if (cos_a > 1.)
         cos_a = 1.;
      if (cos_a < -1.)
         cos_a = -1;
      angles[i] = acos(cos_a)/M_PI;
   }

   if (angles[0] + angles[1] > 1.) {
      /* more than 180 degrees */
      normals[1][0] = -normals[1][0];
      normals[1][1] = -normals[1][1];
      angles[0] = 1. - angles[0];
      angles[1] = 1. - angles[1];
      sign = -1.;
   }

   circle[0][0] = b->x1 + normals[0][0]*offset;
   circle[0][1] = b->y1 + normals[0][1]*offset;

   circle[1][0] = 0.5*(b->x1 + b->x4) + normals[1][0]*offset;
   circle[1][1] = 0.5*(b->y1 + b->y4) + normals[1][1]*offset;

   circle[2][0] = b->x4 + normals[2][0]*offset;
   circle[2][1] = b->y4 + normals[2][1]*offset;

   for (i = 0; i < 2; ++i) {
      float kappa = 2.*KAPPA * sign * offset * angles[i];

      o->x1 = circle[i][0];
      o->y1 = circle[i][1];
      o->x2 = circle[i][0] - normals[i][1]*kappa;
      o->y2 = circle[i][1] + normals[i][0]*kappa;
      o->x3 = circle[i+1][0] + normals[i+1][1]*kappa;
      o->y3 = circle[i+1][1] - normals[i+1][0]*kappa;
      o->x4 = circle[i+1][0];
      o->y4 = circle[i+1][1];

      ++o;
   }
   return VG_TRUE;
}

int bezier_translate_by_normal(struct bezier *bez,
                               struct bezier *curves,
                               int max_curves,
                               float normal_len,
                               float threshold)
{
   struct bezier beziers[10];
   struct bezier *b, *o;

   /* fixme: this should really be floatsEqual */
   if (bez->x1 == bez->x2 && bez->x1 == bez->x3 && bez->x1 == bez->x4 &&
       bez->y1 == bez->y2 && bez->y1 == bez->y3 && bez->y1 == bez->y4)
      return 0;

   --max_curves;
redo:
   beziers[0] = *bez;
   b = beziers;
   o = curves;

   while (b >= beziers) {
      int stack_segments = b - beziers + 1;
      enum shift_result res;
      if ((stack_segments == 10) || (o - curves == max_curves - stack_segments)) {
         threshold *= 1.5;
         if (threshold > 2.)
            goto give_up;
         goto redo;
      }
      res = shift(b, o, normal_len, threshold);
      if (res == Discard) {
         --b;
      } else if (res == Ok) {
         ++o;
         --b;
         continue;
      } else if (res == Circle && max_curves - (o - curves) >= 2) {
         /* add semi circle */
         if (make_circle(b, normal_len, o))
            o += 2;
         --b;
      } else {
         split(b, b+1, b);
         ++b;
      }
   }

give_up:
   while (b >= beziers) {
      enum shift_result res = shift(b, o, normal_len, threshold);

      /* if res isn't Ok or Split then *o is undefined */
      if (res == Ok || res == Split)
         ++o;

      --b;
   }

   debug_assert(o - curves <= max_curves);
   return o - curves;
}

void bezier_bounds(const struct bezier *bez,
                   float *bounds/*x/y/width/height*/)
{
   float xmin = bez->x1;
   float xmax = bez->x1;
   float ymin = bez->y1;
   float ymax = bez->y1;

   if (bez->x2 < xmin)
      xmin = bez->x2;
   else if (bez->x2 > xmax)
      xmax = bez->x2;
   if (bez->x3 < xmin)
      xmin = bez->x3;
   else if (bez->x3 > xmax)
      xmax = bez->x3;
   if (bez->x4 < xmin)
      xmin = bez->x4;
   else if (bez->x4 > xmax)
      xmax = bez->x4;

   if (bez->y2 < ymin)
      ymin = bez->y2;
   else if (bez->y2 > ymax)
      ymax = bez->y2;
   if (bez->y3 < ymin)
      ymin = bez->y3;
   else if (bez->y3 > ymax)
      ymax = bez->y3;
   if (bez->y4 < ymin)
      ymin = bez->y4;
   else if (bez->y4 > ymax)
      ymax = bez->y4;

   bounds[0] = xmin; /* x */
   bounds[1] = ymin; /* y */
   bounds[2] = xmax - xmin; /* width */
   bounds[3] = ymax - ymin; /* height */
}

void bezier_start_tangent(const struct bezier *bez,
                          float *tangent)
{
   tangent[0] = bez->x1;
   tangent[1] = bez->y1;
   tangent[2] = bez->x2;
   tangent[3] = bez->y2;

   if (null_line(tangent)) {
      tangent[0] = bez->x1;
      tangent[1] = bez->y1;
      tangent[2] = bez->x3;
      tangent[3] = bez->y3;
   }
   if (null_line(tangent)) {
      tangent[0] = bez->x1;
      tangent[1] = bez->y1;
      tangent[2] = bez->x4;
      tangent[3] = bez->y4;
   }
}


static INLINE VGfloat bezier_t_at_length(struct bezier *bez,
                                         VGfloat at_length,
                                         VGfloat error)
{
   VGfloat len = bezier_length(bez, error);
   VGfloat t   = 1.0;
   VGfloat last_bigger = 1.;

   if (at_length > len || floatsEqual(at_length, len))
      return t;

   if (floatIsZero(at_length))
      return 0.f;

   t *= 0.5;
   while (1) {
      struct bezier right = *bez;
      struct bezier left;
      VGfloat tmp_len;
      split_left(&right, t, &left);
      tmp_len = bezier_length(&left, error);
      if (ABS(tmp_len - at_length) < error)
         break;

      if (tmp_len < at_length) {
         t += (last_bigger - t)*.5;
      } else {
         last_bigger = t;
         t -= t*.5;
      }
   }
   return t;
}

void bezier_point_at_length(struct bezier *bez,
                            float length,
                            float *point,
                            float *normal)
{
   /* ~0.000001 seems to be required to pass G2080x tests */
   VGfloat t = bezier_t_at_length(bez, length, 0.000001);
   bezier_point_at(bez, t, point);
   bezier_normal_at(bez, t, normal);
   vector_unit(normal);
}

void bezier_point_at_t(struct bezier *bez, float t,
                       float *point, float *normal)
{
   bezier_point_at(bez, t, point);
   bezier_normal_at(bez, t, normal);
   vector_unit(normal);
}

void bezier_exact_bounds(const struct bezier *bez,
                         float *bounds/*x/y/width/height*/)
{
   struct polygon *poly = polygon_create(64);
   polygon_vertex_append(poly, bez->x1, bez->y1);
   bezier_add_to_polygon(bez, poly);
   polygon_bounding_rect(poly, bounds);
   polygon_destroy(poly);
}

