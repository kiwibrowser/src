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

#include "arc.h"

#include "matrix.h"
#include "bezier.h"
#include "polygon.h"
#include "stroker.h"
#include "path.h"

#include "util/u_debug.h"
#include "util/u_math.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEBUG_ARCS 0

static const VGfloat two_pi = M_PI * 2;


static const double coeffs3Low[2][4][4] = {
   {
      {  3.85268,   -21.229,      -0.330434,    0.0127842  },
      { -1.61486,     0.706564,    0.225945,    0.263682   },
      { -0.910164,    0.388383,    0.00551445,  0.00671814 },
      { -0.630184,    0.192402,    0.0098871,   0.0102527  }
   },
   {
      { -0.162211,    9.94329,     0.13723,     0.0124084  },
      { -0.253135,    0.00187735,  0.0230286,   0.01264    },
      { -0.0695069,  -0.0437594,   0.0120636,   0.0163087  },
      { -0.0328856,  -0.00926032, -0.00173573,  0.00527385 }
   }
};

/* coefficients for error estimation
   while using cubic Bézier curves for approximation
   1/4 <= b/a <= 1 */
static const double coeffs3High[2][4][4] = {
   {
      {  0.0899116, -19.2349,     -4.11711,     0.183362   },
      {  0.138148,   -1.45804,     1.32044,     1.38474    },
      {  0.230903,   -0.450262,    0.219963,    0.414038   },
      {  0.0590565,  -0.101062,    0.0430592,   0.0204699  }
   },
   {
      {  0.0164649,   9.89394,     0.0919496,   0.00760802 },
      {  0.0191603,  -0.0322058,   0.0134667,  -0.0825018  },
      {  0.0156192,  -0.017535,    0.00326508, -0.228157   },
      { -0.0236752,   0.0405821,  -0.0173086,   0.176187   }
   }
};

/* safety factor to convert the "best" error approximation
   into a "max bound" error */
static const double safety3[] = {
   0.001, 4.98, 0.207, 0.0067
};

/* The code below is from the OpenVG 1.1 Spec
 * Section 18.4 */

/* Given: Points (x0, y0) and (x1, y1)
 * Return: TRUE if a solution exists, FALSE otherwise
 *         Circle centers are written to (cx0, cy0) and (cx1, cy1)
 */
static VGboolean
find_unit_circles(double x0, double y0, double x1, double y1,
                  double *cx0, double *cy0,
                  double *cx1, double *cy1)
{
   /* Compute differences and averages */
   double dx = x0 - x1;
   double dy = y0 - y1;
   double xm = (x0 + x1)/2;
   double ym = (y0 + y1)/2;
   double dsq, disc, s, sdx, sdy;

   /* Solve for intersecting unit circles */
   dsq = dx*dx + dy*dy;
   if (dsq == 0.0) return VG_FALSE; /* Points are coincident */
   disc = 1.0/dsq - 1.0/4.0;

   /* the precision we care about here is around float so if we're
    * around the float defined zero then make it official to avoid
    * precision problems later on */
   if (floatIsZero(disc))
      disc = 0.0;

   if (disc < 0.0) return VG_FALSE; /* Points are too far apart */
   s = sqrt(disc);
   sdx = s*dx;
   sdy = s*dy;
   *cx0 = xm + sdy;
   *cy0 = ym - sdx;
   *cx1 = xm - sdy;
   *cy1 = ym + sdx;
   return VG_TRUE;
}


/* Given:  Ellipse parameters rh, rv, rot (in degrees),
 *         endpoints (x0, y0) and (x1, y1)
 * Return: TRUE if a solution exists, FALSE otherwise
 *         Ellipse centers are written to (cx0, cy0) and (cx1, cy1)
 */
static VGboolean
find_ellipses(double rh, double rv, double rot,
              double x0, double y0, double x1, double y1,
              double *cx0, double *cy0, double *cx1, double *cy1)
{
   double COS, SIN, x0p, y0p, x1p, y1p, pcx0, pcy0, pcx1, pcy1;
   /* Convert rotation angle from degrees to radians */
   rot *= M_PI/180.0;
   /* Pre-compute rotation matrix entries */
   COS = cos(rot); SIN = sin(rot);
   /* Transform (x0, y0) and (x1, y1) into unit space */
   /* using (inverse) rotate, followed by (inverse) scale   */
   x0p = (x0*COS + y0*SIN)/rh;
   y0p = (-x0*SIN + y0*COS)/rv;
   x1p = (x1*COS + y1*SIN)/rh;
   y1p = (-x1*SIN + y1*COS)/rv;
   if (!find_unit_circles(x0p, y0p, x1p, y1p,
                          &pcx0, &pcy0, &pcx1, &pcy1)) {
      return VG_FALSE;
   }
   /* Transform back to original coordinate space */
   /* using (forward) scale followed by (forward) rotate */
   pcx0 *= rh; pcy0 *= rv;
   pcx1 *= rh; pcy1 *= rv;
   *cx0 = pcx0*COS - pcy0*SIN;
   *cy0 = pcx0*SIN + pcy0*COS;
   *cx1 = pcx1*COS - pcy1*SIN;
   *cy1 = pcx1*SIN + pcy1*COS;
   return VG_TRUE;
}

static INLINE VGboolean
try_to_fix_radii(struct arc *arc)
{
   double COS, SIN, rot, x0p, y0p, x1p, y1p;
   double dx, dy, dsq, scale;

   /* Convert rotation angle from degrees to radians */
   rot = DEGREES_TO_RADIANS(arc->theta);

   /* Pre-compute rotation matrix entries */
   COS = cos(rot); SIN = sin(rot);

   /* Transform (x0, y0) and (x1, y1) into unit space */
   /* using (inverse) rotate, followed by (inverse) scale   */
   x0p = (arc->x1*COS + arc->y1*SIN)/arc->a;
   y0p = (-arc->x1*SIN + arc->y1*COS)/arc->b;
   x1p = (arc->x2*COS + arc->y2*SIN)/arc->a;
   y1p = (-arc->x2*SIN + arc->y2*COS)/arc->b;
   /* Compute differences and averages */
   dx = x0p - x1p;
   dy = y0p - y1p;

   dsq = dx*dx + dy*dy;
#if 0
   if (dsq <= 0.001) {
      debug_printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAaaaaa\n");
   }
#endif
   scale = 1/(2/sqrt(dsq));
   arc->a *= scale;
   arc->b *= scale;
   return VG_TRUE;
}

static INLINE double vector_normalize(double *v)
{
   double sq = v[0] * v[0] + v[1] * v[1];
   return sqrt(sq);
}
static INLINE double vector_orientation(double *v)
{
   double norm = vector_normalize(v);
   double cosa = v[0] / norm;
   double sina = v[1] / norm;
   return (sina>=0 ? acos(cosa) : 2*M_PI - acos(cosa));
}
static INLINE double vector_dot(double *v0,
                                double *v1)
{
   return v0[0] * v1[0] + v0[1] * v1[1];
}

static INLINE double vector_angles(double *v0,
                                   double *v1)
{
   double dot = vector_dot(v0, v1);
   double norm0 = vector_normalize(v0);
   double norm1 = vector_normalize(v1);

   return acos(dot / (norm0 * norm1));
}

static VGboolean find_angles(struct arc *arc)
{
   double vec0[2], vec1[2];
   double lambda1, lambda2;
   double angle;
   struct matrix matrix;

   if (floatIsZero(arc->a) || floatIsZero(arc->b)) {
      return VG_FALSE;
   }
   /* map the points to an identity circle */
   matrix_load_identity(&matrix);
   matrix_scale(&matrix, 1.f, arc->a/arc->b);
   matrix_rotate(&matrix, -arc->theta);
   matrix_map_point(&matrix,
                    arc->x1, arc->y1,
                    &arc->x1, &arc->y1);
   matrix_map_point(&matrix,
                    arc->x2, arc->y2,
                    &arc->x2, &arc->y2);
   matrix_map_point(&matrix,
                    arc->cx, arc->cy,
                    &arc->cx, &arc->cy);

#if DEBUG_ARCS
   debug_printf("Matrix 3 [%f, %f, %f| %f, %f, %f| %f, %f, %f]\n",
                matrix.m[0], matrix.m[1], matrix.m[2],
                matrix.m[3], matrix.m[4], matrix.m[5],
                matrix.m[6], matrix.m[7], matrix.m[8]);
   debug_printf("Endpoints [%f, %f], [%f, %f]\n",
                arc->x1, arc->y1, arc->x2, arc->y2);
#endif

   vec0[0] = arc->x1 - arc->cx;
   vec0[1] = arc->y1 - arc->cy;
   vec1[0] = arc->x2 - arc->cx;
   vec1[1] = arc->y2 - arc->cy;

#if DEBUG_ARCS
   debug_printf("Vec is [%f, %f], [%f, %f], [%f, %f]\n",
                vec0[0], vec0[1], vec1[0], vec1[1], arc->cx, arc->cy);
#endif

   lambda1 = vector_orientation(vec0);

   if (isnan(lambda1))
      lambda1 = 0.f;

   if (arc->type == VG_SCWARC_TO ||
       arc->type == VG_SCCWARC_TO)
      angle = vector_angles(vec0, vec1);
   else if (arc->type == VG_LCWARC_TO ||
            arc->type == VG_LCCWARC_TO) {
      angle = 2*M_PI - vector_angles(vec0, vec1);
   } else
      abort();

   if (isnan(angle))
      angle = M_PI;


   if (arc->type == VG_SCWARC_TO ||
       arc->type == VG_LCWARC_TO)
      lambda2 = lambda1 - angle;
   else
      lambda2 = lambda1 + angle;

#if DEBUG_ARCS
   debug_printf("Angle is %f and (%f, %f)\n", angle, lambda1, lambda2);
#endif

#if 0
   arc->eta1 = atan2(sin(lambda1) / arc->b,
                     cos(lambda1) / arc->a);
   arc->eta2 = atan2(sin(lambda2) / arc->b,
                     cos(lambda2) / arc->a);

   /* make sure we have eta1 <= eta2 <= eta1 + 2 PI */
   arc->eta2 -= two_pi * floor((arc->eta2 - arc->eta1) / two_pi);

   /* the preceding correction fails if we have exactly et2 - eta1 = 2 PI
      it reduces the interval to zero length */
   if ((lambda2 - lambda1 > M_PI) && (arc->eta2 - arc->eta1 < M_PI)) {
      arc->eta2 += 2 * M_PI;
   }
#else
   arc->eta1 = lambda1;
   arc->eta2 = lambda2;
#endif

   return VG_TRUE;
}

#if DEBUG_ARCS
static void check_endpoints(struct arc *arc)
{
   double x1, y1, x2, y2;

   double a_cos_eta1 = arc->a * cos(arc->eta1);
   double b_sin_eta1 = arc->b * sin(arc->eta1);
   x1 = arc->cx + a_cos_eta1 * arc->cos_theta -
        b_sin_eta1 * arc->sin_theta;
   y1 = arc->cy + a_cos_eta1 * arc->sin_theta +
        b_sin_eta1 * arc->cos_theta;

   double a_cos_eta2 = arc->a * cos(arc->eta2);
   double b_sin_eta2 = arc->b * sin(arc->eta2);
   x2 = arc->cx + a_cos_eta2 * arc->cos_theta -
        b_sin_eta2 * arc->sin_theta;
   y2 = arc->cy + a_cos_eta2 * arc->sin_theta +
        b_sin_eta2 * arc->cos_theta;

   debug_printf("Computed (%f, %f), (%f, %f)\n",
                x1, y1, x2, y2);
   debug_printf("Real     (%f, %f), (%f, %f)\n",
                arc->x1, arc->y1,
                arc->x2, arc->y2);
}
#endif

void arc_init(struct arc *arc,
              VGPathSegment type,
              VGfloat x1, VGfloat y1,
              VGfloat x2, VGfloat y2,
              VGfloat rh, VGfloat rv,
              VGfloat rot)
{
   assert(type == VG_SCCWARC_TO ||
          type == VG_SCWARC_TO ||
          type == VG_LCCWARC_TO ||
          type == VG_LCWARC_TO);
   arc->type = type;
   arc->x1  = x1;
   arc->y1  = y1;
   arc->x2  = x2;
   arc->y2  = y2;
   arc->a   = rh;
   arc->b   = rv;
   arc->theta = rot;
   arc->cos_theta = cos(arc->theta);
   arc->sin_theta = sin(arc->theta);
   {
      double cx0, cy0, cx1, cy1;
      double cx, cy;
      arc->is_valid =  find_ellipses(rh, rv, rot, x1, y1, x2, y2,
                                     &cx0, &cy0, &cx1, &cy1);

      if (!arc->is_valid && try_to_fix_radii(arc)) {
         rh = arc->a;
         rv = arc->b;
         arc->is_valid =
            find_ellipses(rh, rv, rot, x1, y1, x2, y2,
                          &cx0, &cy0, &cx1, &cy1);
      }

      if (type == VG_SCWARC_TO ||
          type == VG_LCCWARC_TO) {
         cx = cx1;
         cy = cy1;
      } else {
         cx = cx0;
         cy = cy0;
      }
#if DEBUG_ARCS
      debug_printf("Centers are : (%f, %f) , (%f, %f). Real (%f, %f)\n",
                   cx0, cy0, cx1, cy1, cx, cy);
#endif
      arc->cx = cx;
      arc->cy = cy;
      if (arc->is_valid) {
         arc->is_valid = find_angles(arc);
#if DEBUG_ARCS
         check_endpoints(arc);
#endif
         /* remap a few points. find_angles requires
          * rot in angles, the rest of the code
          * will need them in radians. and find_angles
          * modifies the center to match an identity
          * circle so lets reset it */
         arc->theta = DEGREES_TO_RADIANS(rot);
         arc->cos_theta = cos(arc->theta);
         arc->sin_theta = sin(arc->theta);
         arc->cx = cx;
         arc->cy = cy;
      }
   }
}

static INLINE double rational_function(double x, const double *c)
{
   return (x * (x * c[0] + c[1]) + c[2]) / (x + c[3]);
}

static double estimate_error(struct arc *arc,
                             double etaA, double etaB)
{
   double eta  = 0.5 * (etaA + etaB);

   double x    = arc->b / arc->a;
   double dEta = etaB - etaA;
   double cos2 = cos(2 * eta);
   double cos4 = cos(4 * eta);
   double cos6 = cos(6 * eta);
   double c0, c1;

   /* select the right coeficients set according to degree and b/a */
   const double (*coeffs)[4][4];
   const double *safety;
   coeffs = (x < 0.25) ? coeffs3Low : coeffs3High;
   safety = safety3;

   c0 = rational_function(x, coeffs[0][0])
        + cos2 * rational_function(x, coeffs[0][1])
        + cos4 * rational_function(x, coeffs[0][2])
        + cos6 * rational_function(x, coeffs[0][3]);

   c1 = rational_function(x, coeffs[1][0])
        + cos2 * rational_function(x, coeffs[1][1])
        + cos4 * rational_function(x, coeffs[1][2])
        + cos6 * rational_function(x, coeffs[1][3]);

   return rational_function(x, safety) * arc->a * exp(c0 + c1 * dEta);
}

struct arc_cb {
   void (*move)(struct arc_cb *cb, VGfloat x, VGfloat y);
   void (*point)(struct arc_cb *cb, VGfloat x, VGfloat y);
   void (*bezier)(struct arc_cb *cb, struct bezier *bezier);

   void *user_data;
};

static void cb_null_move(struct arc_cb *cb, VGfloat x, VGfloat y)
{
}

static void polygon_point(struct arc_cb *cb, VGfloat x, VGfloat y)
{
   struct polygon *poly = (struct polygon*)cb->user_data;
   polygon_vertex_append(poly, x, y);
}

static void polygon_bezier(struct arc_cb *cb, struct bezier *bezier)
{
   struct polygon *poly = (struct polygon*)cb->user_data;
   bezier_add_to_polygon(bezier, poly);
}

static void stroke_point(struct arc_cb *cb, VGfloat x, VGfloat y)
{
   struct stroker *stroker = (struct stroker*)cb->user_data;
   stroker_line_to(stroker, x, y);
}

static void stroke_curve(struct arc_cb *cb, struct bezier *bezier)
{
   struct stroker *stroker = (struct stroker*)cb->user_data;
   stroker_curve_to(stroker,
                    bezier->x2, bezier->y2,
                    bezier->x3, bezier->y3,
                    bezier->x4, bezier->y4);
}

static void stroke_emit_point(struct arc_cb *cb, VGfloat x, VGfloat y)
{
   struct stroker *stroker = (struct stroker*)cb->user_data;
   stroker_emit_line_to(stroker, x, y);
}

static void stroke_emit_curve(struct arc_cb *cb, struct bezier *bezier)
{
   struct stroker *stroker = (struct stroker*)cb->user_data;
   stroker_emit_curve_to(stroker,
                         bezier->x2, bezier->y2,
                         bezier->x3, bezier->y3,
                         bezier->x4, bezier->y4);
}

static void arc_path_move(struct arc_cb *cb, VGfloat x, VGfloat y)
{
   struct path *path = (struct path*)cb->user_data;
   path_move_to(path, x, y);
}

static void arc_path_point(struct arc_cb *cb, VGfloat x, VGfloat y)
{
   struct path *path = (struct path*)cb->user_data;
   path_line_to(path, x, y);
}

static void arc_path_bezier(struct arc_cb *cb, struct bezier *bezier)
{
   struct path *path = (struct path*)cb->user_data;
   path_cubic_to(path,
                 bezier->x2, bezier->y2,
                 bezier->x3, bezier->y3,
                 bezier->x4, bezier->y4);
}

static INLINE int num_beziers_needed(struct arc *arc)
{
   double threshold = 0.05;
   VGboolean found = VG_FALSE;
   int n = 1;
   double min_eta, max_eta;

   min_eta = MIN2(arc->eta1, arc->eta2);
   max_eta = MAX2(arc->eta1, arc->eta2);

   while ((! found) && (n < 1024)) {
      double d_eta = (max_eta - min_eta) / n;
      if (d_eta <= 0.5 * M_PI) {
         double eta_b = min_eta;
         int i;
         found = VG_TRUE;
         for (i = 0; found && (i < n); ++i) {
            double etaA = eta_b;
            eta_b += d_eta;
            found = (estimate_error(arc, etaA, eta_b) <= threshold);
         }
      }
      n = n << 1;
   }

   return n;
}

static void arc_to_beziers(struct arc *arc,
                           struct arc_cb cb,
                           struct matrix *matrix)
{
   int i;
   int n = 1;
   double d_eta, eta_b, cos_eta_b,
      sin_eta_b, a_cos_eta_b, b_sin_eta_b, a_sin_eta_b,
      b_cos_eta_b, x_b, y_b, x_b_dot, y_b_dot, lx, ly;
   double t, alpha;

   { /* always move to the start of the arc */
      VGfloat x = arc->x1;
      VGfloat y = arc->y1;
      matrix_map_point(matrix, x, y, &x, &y);
      cb.move(&cb, x, y);
   }

   if (!arc->is_valid) {
      VGfloat x = arc->x2;
      VGfloat y = arc->y2;
      matrix_map_point(matrix, x, y, &x, &y);
      cb.point(&cb, x, y);
      return;
   }

   /* find the number of Bézier curves needed */
   n = num_beziers_needed(arc);

   d_eta = (arc->eta2 - arc->eta1) / n;
   eta_b = arc->eta1;

   cos_eta_b  = cos(eta_b);
   sin_eta_b  = sin(eta_b);
   a_cos_eta_b = arc->a * cos_eta_b;
   b_sin_eta_b = arc->b * sin_eta_b;
   a_sin_eta_b = arc->a * sin_eta_b;
   b_cos_eta_b = arc->b * cos_eta_b;
   x_b       = arc->cx + a_cos_eta_b * arc->cos_theta -
               b_sin_eta_b * arc->sin_theta;
   y_b       = arc->cy + a_cos_eta_b * arc->sin_theta +
               b_sin_eta_b * arc->cos_theta;
   x_b_dot    = -a_sin_eta_b * arc->cos_theta -
                b_cos_eta_b * arc->sin_theta;
   y_b_dot    = -a_sin_eta_b * arc->sin_theta +
                b_cos_eta_b * arc->cos_theta;

   {
      VGfloat x = x_b, y = y_b;
      matrix_map_point(matrix, x, y, &x, &y);
      cb.point(&cb, x, y);
   }
   lx = x_b;
   ly = y_b;

   t     = tan(0.5 * d_eta);
   alpha = sin(d_eta) * (sqrt(4 + 3 * t * t) - 1) / 3;

   for (i = 0; i < n; ++i) {
      struct bezier bezier;
      double xA    = x_b;
      double yA    = y_b;
      double xADot = x_b_dot;
      double yADot = y_b_dot;

      eta_b    += d_eta;
      cos_eta_b  = cos(eta_b);
      sin_eta_b  = sin(eta_b);
      a_cos_eta_b = arc->a * cos_eta_b;
      b_sin_eta_b = arc->b * sin_eta_b;
      a_sin_eta_b = arc->a * sin_eta_b;
      b_cos_eta_b = arc->b * cos_eta_b;
      x_b       = arc->cx + a_cos_eta_b * arc->cos_theta -
                  b_sin_eta_b * arc->sin_theta;
      y_b       = arc->cy + a_cos_eta_b * arc->sin_theta +
                  b_sin_eta_b * arc->cos_theta;
      x_b_dot    = -a_sin_eta_b * arc->cos_theta -
                   b_cos_eta_b * arc->sin_theta;
      y_b_dot    = -a_sin_eta_b * arc->sin_theta +
                   b_cos_eta_b * arc->cos_theta;

      bezier_init(&bezier,
                  lx, ly,
                  (float) (xA + alpha * xADot), (float) (yA + alpha * yADot),
                  (float) (x_b - alpha * x_b_dot), (float) (y_b - alpha * y_b_dot),
                  (float) x_b,                   (float) y_b);
#if 0
      debug_printf("%d) Bezier (%f, %f), (%f, %f), (%f, %f), (%f, %f)\n",
                   i,
                   bezier.x1, bezier.y1,
                   bezier.x2, bezier.y2,
                   bezier.x3, bezier.y3,
                   bezier.x4, bezier.y4);
#endif
      bezier_transform(&bezier, matrix);
      cb.bezier(&cb, &bezier);
      lx = x_b;
      ly = y_b;
   }
}


void arc_add_to_polygon(struct arc *arc,
                        struct polygon *poly,
                        struct matrix *matrix)
{
   struct arc_cb cb;

   cb.move = cb_null_move;
   cb.point = polygon_point;
   cb.bezier = polygon_bezier;
   cb.user_data = poly;

   arc_to_beziers(arc, cb, matrix);
}

void arc_stroke_cb(struct arc *arc,
                   struct stroker *stroke,
                   struct matrix *matrix)
{
   struct arc_cb cb;

   cb.move = cb_null_move;
   cb.point = stroke_point;
   cb.bezier = stroke_curve;
   cb.user_data = stroke;

   arc_to_beziers(arc, cb, matrix);
}

void arc_stroker_emit(struct arc *arc,
                      struct stroker *stroker,
                      struct matrix *matrix)
{
   struct arc_cb cb;

   cb.move = cb_null_move;
   cb.point = stroke_emit_point;
   cb.bezier = stroke_emit_curve;
   cb.user_data = stroker;

   arc_to_beziers(arc, cb, matrix);
}

void arc_to_path(struct arc *arc,
                 struct path *path,
                 struct matrix *matrix)
{
   struct arc_cb cb;

   cb.move = arc_path_move;
   cb.point = arc_path_point;
   cb.bezier = arc_path_bezier;
   cb.user_data = path;

   arc_to_beziers(arc, cb, matrix);
}
