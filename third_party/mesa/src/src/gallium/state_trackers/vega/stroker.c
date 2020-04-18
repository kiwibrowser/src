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

#include "stroker.h"

#include "path.h"
#include "vg_state.h"
#include "util_array.h"
#include "arc.h"
#include "bezier.h"
#include "matrix.h"
#include "path_utils.h"
#include "polygon.h"

#include "util/u_math.h"

#ifndef M_2PI
#define M_2PI 6.28318530717958647692528676655900576
#endif

#define STROKE_SEGMENTS 0
#define STROKE_DEBUG 0
#define DEBUG_EMITS 0

static const VGfloat curve_threshold = 0.25f;

static const VGfloat zero_coords[] = {0.f, 0.f};

enum intersection_type {
   NoIntersections,
   BoundedIntersection,
   UnboundedIntersection,
};

enum line_join_mode {
   FlatJoin,
   SquareJoin,
   MiterJoin,
   RoundJoin,
   RoundCap
};

struct stroke_iterator {
   void (*next)(struct stroke_iterator *);
   VGboolean (*has_next)(struct stroke_iterator *);

   VGPathCommand (*current_command)(struct stroke_iterator *it);
   void (*current_coords)(struct stroke_iterator *it, VGfloat *coords);

   VGint position;
   VGint coord_position;

   const VGubyte *cmds;
   const VGfloat *coords;
   VGint num_commands;
   VGint num_coords;

   struct polygon *curve_poly;
   VGint curve_index;
};

static VGPathCommand stroke_itr_command(struct stroke_iterator *itr)
{
   return itr->current_command(itr);
}

static void stroke_itr_coords(struct stroke_iterator *itr, VGfloat *coords)
{
   itr->current_coords(itr, coords);
}

static void stroke_fw_itr_coords(struct stroke_iterator *itr, VGfloat *coords)
{
   if (itr->position >= itr->num_commands)
      return;
   switch (stroke_itr_command(itr)) {
   case VG_MOVE_TO_ABS:
      coords[0] = itr->coords[itr->coord_position];
      coords[1] = itr->coords[itr->coord_position + 1];
      break;
   case VG_LINE_TO_ABS:
      coords[0] = itr->coords[itr->coord_position];
      coords[1] = itr->coords[itr->coord_position + 1];
      break;
   case VG_CUBIC_TO_ABS:
      coords[0] = itr->coords[itr->coord_position];
      coords[1] = itr->coords[itr->coord_position + 1];
      coords[2] = itr->coords[itr->coord_position + 2];
      coords[3] = itr->coords[itr->coord_position + 3];
      coords[4] = itr->coords[itr->coord_position + 4];
      coords[5] = itr->coords[itr->coord_position + 5];
      break;
   default:
      debug_assert(!"invalid command!\n");
   }
}


static void stroke_bw_itr_coords(struct stroke_iterator *itr, VGfloat *coords)
{
   if (itr->position >= itr->num_commands)
      return;
   switch (stroke_itr_command(itr)) {
   case VG_MOVE_TO_ABS:
      coords[0] = itr->coords[itr->coord_position];
      coords[1] = itr->coords[itr->coord_position + 1];
      break;
   case VG_LINE_TO_ABS:
      coords[0] = itr->coords[itr->coord_position];
      coords[1] = itr->coords[itr->coord_position + 1];
      break;
   case VG_CUBIC_TO_ABS:
      coords[0] = itr->coords[itr->coord_position + 4];
      coords[1] = itr->coords[itr->coord_position + 5];
      coords[2] = itr->coords[itr->coord_position + 2];
      coords[3] = itr->coords[itr->coord_position + 3];
      coords[4] = itr->coords[itr->coord_position + 0];
      coords[5] = itr->coords[itr->coord_position + 1];
      break;
   default:
      debug_assert(!"invalid command!\n");
   }
}


static VGPathCommand stroke_fw_current_command(struct stroke_iterator *it)
{
   return it->cmds[it->position];
}

static VGPathCommand stroke_bw_current_command(struct stroke_iterator *it)
{
   VGPathCommand prev_cmd;
   if (it->position == it->num_commands  -1)
      return VG_MOVE_TO_ABS;

   prev_cmd = it->cmds[it->position + 1];
   return prev_cmd;
}

static VGboolean stroke_fw_has_next(struct stroke_iterator *itr)
{
   return itr->position < (itr->num_commands - 1);
}

static VGboolean stroke_bw_has_next(struct stroke_iterator *itr)
{
   return itr->position > 0;
}

static void stroke_fw_next(struct stroke_iterator *itr)
{
   VGubyte cmd;
   debug_assert(stroke_fw_has_next(itr));

   cmd = stroke_itr_command(itr);

   itr->coord_position += num_elements_for_segments(&cmd, 1);
   ++itr->position;
}

static void stroke_bw_next(struct stroke_iterator *itr)
{
   VGubyte cmd;
   debug_assert(stroke_bw_has_next(itr));

   --itr->position;
   cmd = stroke_itr_command(itr);

   itr->coord_position -= num_elements_for_segments(&cmd, 1);
}

static void stroke_itr_common_init(struct stroke_iterator *itr,
                                   struct array *cmds,
                                   struct array *coords)
{
   itr->cmds = (VGubyte*)cmds->data;
   itr->num_commands = cmds->num_elements;

   itr->coords = (VGfloat*)coords->data;
   itr->num_coords = coords->num_elements;
}

static void stroke_forward_iterator(struct stroke_iterator *itr,
                                    struct array *cmds,
                                    struct array *coords)
{
   stroke_itr_common_init(itr, cmds, coords);
   itr->position = 0;
   itr->coord_position = 0;

   itr->next = stroke_fw_next;
   itr->has_next = stroke_fw_has_next;
   itr->current_command = stroke_fw_current_command;
   itr->current_coords = stroke_fw_itr_coords;
}

static void stroke_backward_iterator(struct stroke_iterator *itr,
                                     struct array *cmds,
                                     struct array *coords)
{
   VGubyte cmd;
   stroke_itr_common_init(itr, cmds, coords);
   itr->position = itr->num_commands - 1;

   cmd = stroke_bw_current_command(itr);
   itr->coord_position = itr->num_coords -
                         num_elements_for_segments(&cmd, 1);

   itr->next = stroke_bw_next;
   itr->has_next = stroke_bw_has_next;
   itr->current_command = stroke_bw_current_command;
   itr->current_coords = stroke_bw_itr_coords;
}



static void stroke_flat_next(struct stroke_iterator *itr)
{
   VGubyte cmd;

   if (itr->curve_index >= 0) {
      ++itr->curve_index;
      if (itr->curve_index >= polygon_vertex_count(itr->curve_poly)) {
         itr->curve_index = -1;
         polygon_destroy(itr->curve_poly);
         itr->curve_poly = 0;
      } else
         return;
   }
   debug_assert(stroke_fw_has_next(itr));

   cmd = itr->cmds[itr->position];
   itr->coord_position += num_elements_for_segments(&cmd, 1);
   ++itr->position;

   cmd = itr->cmds[itr->position];

   if (cmd == VG_CUBIC_TO_ABS) {
      struct bezier bezier;
      VGfloat bez[8];

      bez[0] = itr->coords[itr->coord_position - 2];
      bez[1] = itr->coords[itr->coord_position - 1];
      bez[2] = itr->coords[itr->coord_position];
      bez[3] = itr->coords[itr->coord_position + 1];
      bez[4] = itr->coords[itr->coord_position + 2];
      bez[5] = itr->coords[itr->coord_position + 3];
      bez[6] = itr->coords[itr->coord_position + 4];
      bez[7] = itr->coords[itr->coord_position + 5];

      bezier_init(&bezier,
                  bez[0], bez[1],
                  bez[2], bez[3],
                  bez[4], bez[5],
                  bez[6], bez[7]);
      /* skip the first one, it's the same as the prev point */
      itr->curve_index = 1;
      if (itr->curve_poly) {
         polygon_destroy(itr->curve_poly);
         itr->curve_poly = 0;
      }
      itr->curve_poly = bezier_to_polygon(&bezier);
   }
}

static VGboolean stroke_flat_has_next(struct stroke_iterator *itr)
{
   return  (itr->curve_index >= 0 &&
            itr->curve_index < (polygon_vertex_count(itr->curve_poly)-1))
            || itr->position < (itr->num_commands - 1);
}

static VGPathCommand stroke_flat_current_command(struct stroke_iterator *it)
{
   if (it->cmds[it->position] == VG_CUBIC_TO_ABS) {
      return VG_LINE_TO_ABS;
   }
   return it->cmds[it->position];
}

static void stroke_flat_itr_coords(struct stroke_iterator *itr, VGfloat *coords)
{
   if (itr->curve_index <= -1 && itr->position >= itr->num_commands)
      return;

   if (itr->curve_index >= 0) {
      polygon_vertex(itr->curve_poly, itr->curve_index,
                     coords);
      return;
   }

   switch (stroke_itr_command(itr)) {
   case VG_MOVE_TO_ABS:
      coords[0] = itr->coords[itr->coord_position];
      coords[1] = itr->coords[itr->coord_position + 1];
      break;
   case VG_LINE_TO_ABS:
      coords[0] = itr->coords[itr->coord_position];
      coords[1] = itr->coords[itr->coord_position + 1];
      break;
   case VG_CUBIC_TO_ABS:
   default:
      debug_assert(!"invalid command!\n");
   }
}

static void stroke_flat_iterator(struct stroke_iterator *itr,
                                 struct array *cmds,
                                 struct array *coords)
{
   stroke_itr_common_init(itr, cmds, coords);
   itr->position = 0;
   itr->coord_position = 0;

   itr->next = stroke_flat_next;
   itr->has_next = stroke_flat_has_next;
   itr->current_command = stroke_flat_current_command;
   itr->current_coords = stroke_flat_itr_coords;
   itr->curve_index = -1;
   itr->curve_poly = 0;
}


static INLINE VGboolean finite_coords4(const VGfloat *c)
{
   return
      isfinite(c[0]) && isfinite(c[1]) &&
      isfinite(c[2]) && isfinite(c[3]);
}

/* from Graphics Gems II */
#define SAME_SIGNS(a, b) ((a) * (b) >= 0)
static VGboolean do_lines_intersect(VGfloat x1, VGfloat y1, VGfloat x2, VGfloat y2,
                                    VGfloat x3, VGfloat y3, VGfloat x4, VGfloat y4)
{
   VGfloat a1, a2, b1, b2, c1, c2; /* Coefficients of line eqns */
   VGfloat r1, r2, r3, r4;         /* 'sign' values */

   a1 = y2 - y1;
   b1 = x1 - x2;
   c1 = x2 * y1 - x1 * y2;

   r3 = a1 * x3 + b1 * y3 + c1;
   r4 = a1 * x4 + b1 * y4 + c1;

   if (r3 != 0 && r4 != 0 && SAME_SIGNS(r3, r4))
      return VG_FALSE;

   a2 = y4 - y3;
   b2 = x3 - x4;
   c2 = x4 * y3 - x3 * y4;

   r1 = a2 * x1 + b2 * y1 + c2;
   r2 = a2 * x2 + b2 * y2 + c2;

   if (r1 != 0 && r2 != 0 && SAME_SIGNS(r1, r2))
      return VG_FALSE;

   return VG_TRUE;
}

static INLINE VGfloat line_dx(const VGfloat *l)
{
   return l[2] - l[0];
}

static INLINE VGfloat line_dy(const VGfloat *l)
{
   return l[3] - l[1];
}

static INLINE VGfloat line_angle(const VGfloat *l)
{
   const VGfloat dx = line_dx(l);
   const VGfloat dy = line_dy(l);

   const VGfloat theta = atan2(-dy, dx) * 360.0 / M_2PI;

   const VGfloat theta_normalized = theta < 0 ? theta + 360 : theta;

   if (floatsEqual(theta_normalized, 360.f))
      return 0;
   else
      return theta_normalized;
}

static INLINE void line_set_length(VGfloat *l, VGfloat len)
{
   VGfloat uv[] = {l[0], l[1], l[2], l[3]};
   if (null_line(l))
      return;
   line_normalize(uv);
   l[2] = l[0] + line_dx(uv) * len;
   l[3] = l[1] + line_dy(uv) * len;
}

static INLINE void line_translate(VGfloat *l, VGfloat x, VGfloat y)
{
   l[0] += x;
   l[1] += y;
   l[2] += x;
   l[3] += y;
}

static INLINE VGfloat line_angle_to(const VGfloat *l1,
                                    const VGfloat *l2)
{
   VGfloat a1, a2, delta, delta_normalized;
   if (null_line(l1) || null_line(l1))
      return 0;

   a1 = line_angle(l1);
   a2 = line_angle(l2);

   delta = a2 - a1;
   delta_normalized = delta < 0 ? delta + 360 : delta;

   if (floatsEqual(delta, 360.f))
      return 0;
   else
      return delta_normalized;
}

static INLINE VGfloat line_angles(const VGfloat *l1,
                                  const VGfloat *l2)
{
   VGfloat cos_line, rad = 0;

   if (null_line(l1) || null_line(l2))
      return 0;

   cos_line = (line_dx(l1)*line_dx(l2) + line_dy(l1)*line_dy(l2)) /
              (line_lengthv(l1)*line_lengthv(l2));
   rad = 0;

   if (cos_line >= -1.0 && cos_line <= 1.0)
      rad = acos(cos_line);
   return rad * 360 / M_2PI;
}


static INLINE VGfloat adapted_angle_on_x(const VGfloat *line)
{
   const VGfloat identity[] = {0, 0, 1, 0};
   VGfloat angle = line_angles(line, identity);
   if (line_dy(line) > 0)
      angle = 360 - angle;
   return angle;
}

static enum intersection_type line_intersect(const VGfloat *l1,
                                             const VGfloat *l2,
                                             float *intersection_point)
{
   VGfloat isect[2] = { 0 };
   enum intersection_type type;
   VGboolean dx_zero, ldx_zero;

   if (null_line(l1) || null_line(l2) ||
       !finite_coords4(l1) || !finite_coords4(l2))
      return NoIntersections;

   type = do_lines_intersect(l1[0], l1[1], l1[2], l1[3], l2[0], l2[1], l2[2], l2[3])
          ? BoundedIntersection : UnboundedIntersection;

   dx_zero  = floatsEqual(line_dx(l1) + 1, 1);
   ldx_zero = floatsEqual(line_dx(l2) + 1, 1);

   /* one of the lines is vertical */
   if (dx_zero && ldx_zero) {
      type = NoIntersections;
   } else if (dx_zero) {
      VGfloat la = line_dy(l2) / line_dx(l2);
      isect[0] = l1[0];
      isect[1] = la * l1[0] + l2[1] - la * l2[0];
   } else if (ldx_zero) {
      VGfloat ta = line_dy(l1) / line_dx(l1);
      isect[0] = l2[0];
      isect[1] = ta * l2[0] + l1[1] - ta*l1[0];
   } else {
      VGfloat x;
      VGfloat ta = line_dy(l1) / line_dx(l1);
      VGfloat la = line_dy(l2) / line_dx(l2);
      if (ta == la)
         return NoIntersections;

      x = ( - l2[1] + la * l2[0] + l1[1] - ta * l1[0] ) / (la - ta);
      isect[0] = x;
      isect[1] = ta*(x - l1[0]) + l1[1];
   }
   if (intersection_point) {
      intersection_point[0] = isect[0];
      intersection_point[1] = isect[1];
   }
   return type;
}

static INLINE enum line_join_mode stroker_join_mode(struct stroker *s)
{
   switch(s->join_style) {
   case VG_JOIN_MITER:
      return MiterJoin;
   case VG_JOIN_ROUND:
      return RoundJoin;
   case VG_JOIN_BEVEL:
      return FlatJoin;
   default:
      return FlatJoin;
   }
}

static INLINE enum line_join_mode stroker_cap_mode(struct stroker *s)
{
   switch(s->cap_style) {
   case VG_CAP_BUTT:
      return FlatJoin;
   case VG_CAP_ROUND:
      return RoundCap;
   case VG_CAP_SQUARE:
      return SquareJoin;
   default:
      return FlatJoin;
   }
}

void stroker_emit_move_to(struct stroker *stroker, VGfloat x, VGfloat y)
{
   VGubyte cmds = VG_MOVE_TO_ABS;
   VGfloat coords[2] = {x, y};
#if DEBUG_EMITS
   debug_printf("emit move %f, %f\n", x, y);
#endif
   stroker->back2_x = stroker->back1_x;
   stroker->back2_y = stroker->back1_y;
   stroker->back1_x = x;
   stroker->back1_y = y;

   path_append_data(stroker->path,
                    1,
                    &cmds, &coords);
}

void stroker_emit_line_to(struct stroker *stroker, VGfloat x, VGfloat y)
{
   VGubyte cmds = VG_LINE_TO_ABS;
   VGfloat coords[2] = {x, y};
#if DEBUG_EMITS
   debug_printf("emit line %f, %f\n", x, y);
#endif
   stroker->back2_x = stroker->back1_x;
   stroker->back2_y = stroker->back1_y;
   stroker->back1_x = x;
   stroker->back1_y = y;
   path_append_data(stroker->path,
                    1,
                    &cmds, &coords);
}

void stroker_emit_curve_to(struct stroker *stroker, VGfloat px1, VGfloat py1,
                                  VGfloat px2, VGfloat py2,
                                  VGfloat x, VGfloat y)
{
   VGubyte cmds = VG_CUBIC_TO_ABS;
   VGfloat coords[6] = {px1, py1, px2, py2, x, y};
#if DEBUG_EMITS
   debug_printf("emit curve %f, %f, %f, %f, %f, %f\n", px1, py1,
                px2, py2, x, y);
#endif

   if (px2 == x && py2 == y) {
      if (px1 == x && py1 == y) {
         stroker->back2_x = stroker->back1_x;
         stroker->back2_y = stroker->back1_y;
      } else {
         stroker->back2_x = px1;
         stroker->back2_y = py1;
      }
   } else {
      stroker->back2_x = px2;
      stroker->back2_y = py2;
   }
   stroker->back1_x = x;
   stroker->back1_y = y;

   path_append_data(stroker->path,
                    1,
                    &cmds, &coords);
}

static INLINE void create_round_join(struct stroker *stroker,
                                     VGfloat x1, VGfloat y1,
                                     VGfloat x2, VGfloat y2,
                                     VGfloat width, VGfloat height)
{
   struct arc arc;
   struct matrix matrix;

   matrix_load_identity(&matrix);

   /*stroker_emit_line_to(stroker, nx, ny);*/

   arc_init(&arc, VG_SCCWARC_TO,
            x1, y1, x2, y2, width/2, height/2, 0);
   arc_stroker_emit(&arc, stroker, &matrix);
}


static void create_joins(struct stroker *stroker,
                         VGfloat focal_x, VGfloat focal_y,
                         const VGfloat *next_line, enum line_join_mode join)
{
#if DEBUG_EMITS
   debug_printf("create_joins: focal=[%f, %f], next_line=[%f, %f,%f, %f]\n",
                focal_x, focal_y,
                next_line[0], next_line[1], next_line[2], next_line[3]);
#endif
   /* if we're alredy connected do nothing */
   if (floatsEqual(stroker->back1_x, next_line[0]) &&
       floatsEqual(stroker->back1_y, next_line[1]))
      return;

   if (join == FlatJoin) {
      stroker_emit_line_to(stroker, next_line[0], next_line[1]);
   } else {
      VGfloat prev_line[] = {stroker->back2_x, stroker->back2_y,
                             stroker->back1_x, stroker->back1_y};

      VGfloat isect[2] = { 0 };
      enum intersection_type type = line_intersect(prev_line, next_line, isect);

      if (join == SquareJoin) {
         VGfloat offset = stroker->stroke_width / 2;
         VGfloat l1[4] = {prev_line[0],
                          prev_line[1],
                          prev_line[2],
                          prev_line[3]};
         VGfloat l2[4] = {next_line[2],
                          next_line[3],
                          next_line[0],
                          next_line[1]};

         line_translate(l1, line_dx(l1), line_dy(l1));
         line_set_length(l1, offset);

         line_translate(l2, line_dx(l2), line_dy(l2));
         line_set_length(l2, offset);

         stroker_emit_line_to(stroker, l1[2], l1[3]);
         stroker_emit_line_to(stroker, l2[2], l2[3]);
         stroker_emit_line_to(stroker, l2[0], l2[1]);
      } else if (join == RoundJoin) {
         VGfloat offset = stroker->stroke_width / 2;
         VGfloat short_cut[4] = {prev_line[2], prev_line[3],
                                 next_line[0], next_line[1]};
         VGfloat angle = line_angles(prev_line, short_cut);

         if (type == BoundedIntersection ||
             (angle > 90 && !floatsEqual(angle, 90.f))) {
            stroker_emit_line_to(stroker, next_line[0], next_line[1]);
            return;
         }
         create_round_join(stroker, prev_line[2], prev_line[3],
                           next_line[0], next_line[1],
                           offset * 2, offset * 2);

         stroker_emit_line_to(stroker, next_line[0], next_line[1]);
      } else if (join == RoundCap) {
         VGfloat offset = stroker->stroke_width / 2;
         VGfloat l1[4] = { prev_line[0], prev_line[1],
                           prev_line[2], prev_line[3] };
         VGfloat l2[4] = {focal_x, focal_y,
                          prev_line[2], prev_line[3]};

         line_translate(l1, line_dx(l1), line_dy(l1));
         line_set_length(l1, KAPPA * offset);

         /* normal between prev_line and focal */
         line_translate(l2, -line_dy(l2), line_dx(l2));
         line_set_length(l2, KAPPA * offset);

         stroker_emit_curve_to(stroker, l1[2], l1[3],
                               l2[2], l2[3],
                               l2[0], l2[1]);

         l2[0] = l2[0];
         l2[1] = l2[1];
         l2[2] = l2[0] - line_dx(l2);
         l2[3] = l2[1] - line_dy(l2);

         line_translate(l1, next_line[0] - l1[0], next_line[1] - l1[1]);

         stroker_emit_curve_to(stroker,
                               l2[2], l2[3],
                               l1[2], l1[3],
                               l1[0], l1[1]);
      } else if (join == MiterJoin) {
         VGfloat miter_line[4] = {stroker->back1_x, stroker->back1_y,
                                  isect[0], isect[1]};
         VGfloat sl = (stroker->stroke_width * stroker->miter_limit);
         VGfloat inside_line[4] = {prev_line[2], prev_line[3],
                                   next_line[0], next_line[1]};
         VGfloat angle = line_angle_to(inside_line, prev_line);

         if (type == BoundedIntersection ||
             (angle > 90 && !floatsEqual(angle, 90.f))) {
            /*
            debug_printf("f = %f, nl = %f, pl = %f, is = %f\n",
                         focal_x, next_line[0],
                         prev_line[2], isect[0]);*/
            stroker_emit_line_to(stroker, next_line[0], next_line[1]);
            return;
         }

         if (type == NoIntersections || line_lengthv(miter_line) > sl) {
            stroker_emit_line_to(stroker, next_line[0], next_line[1]);
         } else {
            stroker_emit_line_to(stroker, isect[0], isect[1]);
            stroker_emit_line_to(stroker, next_line[0], next_line[1]);
         }
      } else {
         debug_assert(!"create_joins bad join style");
      }
   }
}

static void stroker_add_segment(struct stroker *stroker,
                                VGPathCommand cmd,
                                const VGfloat *coords,
                                VGint num_coords)
{
   /* skip duplicated points */
   if (stroker->segments->num_elements &&
       stroker->last_cmd == cmd) {
      VGfloat *data = stroker->control_points->data;
      data += stroker->control_points->num_elements;
      data -= num_coords;
      switch (cmd) {
      case VG_MOVE_TO_ABS:
         if (floatsEqual(coords[0], data[0]) &&
             floatsEqual(coords[1], data[1]))
            return;
         break;
      case VG_LINE_TO_ABS:
         if (floatsEqual(coords[0], data[0]) &&
             floatsEqual(coords[1], data[1]))
            return;
         break;
      case VG_CUBIC_TO_ABS:
         if (floatsEqual(coords[0], data[0]) &&
             floatsEqual(coords[1], data[1]) &&
             floatsEqual(coords[2], data[2]) &&
             floatsEqual(coords[3], data[3]) &&
             floatsEqual(coords[4], data[4]) &&
             floatsEqual(coords[5], data[5]))
            return;
         break;
      default:
         debug_assert(!"Invalid stroke segment");
      }
   } else if (stroker->last_cmd == VG_CUBIC_TO_ABS &&
              cmd == VG_LINE_TO_ABS) {
      VGfloat *data = stroker->control_points->data;
      data += stroker->control_points->num_elements;
      data -= 2;
      if (floatsEqual(coords[0], data[0]) &&
          floatsEqual(coords[1], data[1]))
         return;
   }
   stroker->last_cmd = cmd;
   array_append_data(stroker->segments, &cmd, 1);
   array_append_data(stroker->control_points, coords, num_coords);
}

void stroker_move_to(struct stroker *stroker, VGfloat x, VGfloat y)
{
   VGfloat coords[2] = {x, y};
#if STROKE_SEGMENTS
   debug_printf("stroker_move_to(%f, %f)\n", x, y);
#endif

   if (stroker->segments->num_elements > 0)
      stroker->process_subpath(stroker);

   array_reset(stroker->segments);
   array_reset(stroker->control_points);

   stroker_add_segment(stroker, VG_MOVE_TO_ABS, coords, 2);
}

void stroker_line_to(struct stroker *stroker, VGfloat x, VGfloat y)
{
   VGfloat coords[] = {x, y};

#if STROKE_SEGMENTS
   debug_printf("stroker_line_to(%f, %f)\n", x, y);
#endif
   if (!stroker->segments->num_elements)
      stroker_add_segment(stroker, VG_MOVE_TO_ABS, zero_coords, 2);

   stroker_add_segment(stroker, VG_LINE_TO_ABS, coords, 2);
}

void stroker_curve_to(struct stroker *stroker, VGfloat px1, VGfloat py1,
                      VGfloat px2, VGfloat py2,
                      VGfloat x, VGfloat y)
{
   VGfloat coords[] = {px1, py1,
                       px2, py2,
                       x, y};
#if STROKE_SEGMENTS
   debug_printf("stroker_curve_to(%f, %f, %f, %f, %f, %f)\n",
                px1, py1, px2, py2, x, y);
#endif
   if (!stroker->segments->num_elements)
      stroker_add_segment(stroker, VG_MOVE_TO_ABS, zero_coords, 2);

   stroker_add_segment(stroker, VG_CUBIC_TO_ABS, coords, 6);
}

static INLINE VGboolean is_segment_null(VGPathCommand cmd,
                                        VGfloat *coords,
                                        VGfloat *res)
{
   switch(cmd) {
   case VG_MOVE_TO_ABS:
   case VG_LINE_TO_ABS:
      return floatsEqual(coords[0], res[0]) &&
         floatsEqual(coords[1], res[1]);
      break;
   case VG_CUBIC_TO_ABS:
      return floatsEqual(coords[0], res[0]) &&
         floatsEqual(coords[1], res[1]) &&
         floatsEqual(coords[2], res[0]) &&
         floatsEqual(coords[3], res[1]) &&
         floatsEqual(coords[4], res[0]) &&
         floatsEqual(coords[5], res[1]);
      break;
   default:
      assert(0);
   }
   return VG_FALSE;
}

static VGboolean vg_stroke_outline(struct stroke_iterator *it,
                                struct stroker *stroker,
                                VGboolean cap_first,
                                VGfloat *start_tangent)
{
#define MAX_OFFSET 16
   struct bezier offset_curves[MAX_OFFSET];
   VGPathCommand first_element;
   VGfloat start[2], prev[2];
   VGboolean first = VG_TRUE;
   VGfloat offset;

   first_element = stroke_itr_command(it);
   if (first_element != VG_MOVE_TO_ABS) {
      stroker_emit_move_to(stroker, 0.f, 0.f);
      prev[0] = 0.f;
      prev[1] = 0.f;
   }
   stroke_itr_coords(it, start);
#if STROKE_DEBUG
   debug_printf(" -> (side) [%.2f, %.2f]\n",
                start[0],
                start[1]);
#endif

   prev[0] = start[0];
   prev[1] = start[1];

   offset = stroker->stroke_width / 2;

   if (!it->has_next(it)) {
      /* single point */

      return VG_TRUE;
   }

   while (it->has_next(it)) {
      VGPathCommand cmd;
      VGfloat coords[8];

      it->next(it);
      cmd = stroke_itr_command(it);
      stroke_itr_coords(it, coords);

      if (cmd == VG_LINE_TO_ABS) {
         VGfloat line[4] = {prev[0], prev[1], coords[0], coords[1]};
         VGfloat normal[4];
         line_normal(line, normal);

#if STROKE_DEBUG
         debug_printf("\n ---> (side) lineto [%.2f, %.2f]\n", coords[0], coords[1]);
#endif
         line_set_length(normal, offset);
         line_translate(line, line_dx(normal), line_dy(normal));

         /* if we are starting a new subpath, move to correct starting point */
         if (first) {
            if (cap_first)
               create_joins(stroker, prev[0], prev[1], line,
                            stroker_cap_mode(stroker));
            else
               stroker_emit_move_to(stroker, line[0], line[1]);
            memcpy(start_tangent, line,
                   sizeof(VGfloat) * 4);
            first = VG_FALSE;
         } else {
            create_joins(stroker, prev[0], prev[1], line,
                         stroker_join_mode(stroker));
         }

         /* add the stroke for this line */
         stroker_emit_line_to(stroker, line[2], line[3]);
         prev[0] = coords[0];
         prev[1] = coords[1];
      } else if (cmd == VG_CUBIC_TO_ABS) {
#if STROKE_DEBUG
         debug_printf("\n ---> (side) cubicTo [%.2f, %.2f]\n",
                coords[4],
                coords[5]);
#endif
         struct bezier bezier;
         int count;

         bezier_init(&bezier,
                     prev[0], prev[1], coords[0], coords[1],
                     coords[2], coords[3], coords[4], coords[5]);

         count = bezier_translate_by_normal(&bezier,
                                            offset_curves,
                                            MAX_OFFSET,
                                            offset,
                                            curve_threshold);

         if (count) {
            /* if we are starting a new subpath, move to correct starting point */
            VGfloat tangent[4];
            VGint i;

            bezier_start_tangent(&bezier, tangent);
            line_translate(tangent,
                           offset_curves[0].x1 - bezier.x1,
                           offset_curves[0].y1 - bezier.y1);
            if (first) {
               VGfloat pt[2] = {offset_curves[0].x1,
                                offset_curves[0].y1};

               if (cap_first) {
                  create_joins(stroker, prev[0], prev[1], tangent,
                               stroker_cap_mode(stroker));
               } else {
                  stroker_emit_move_to(stroker, pt[0], pt[1]);
               }
               start_tangent[0] = tangent[0];
               start_tangent[1] = tangent[1];
               start_tangent[2] = tangent[2];
               start_tangent[3] = tangent[3];
               first = VG_FALSE;
            } else {
               create_joins(stroker, prev[0], prev[1], tangent,
                            stroker_join_mode(stroker));
            }

            /* add these beziers */
            for (i = 0; i < count; ++i) {
               struct bezier *bez = &offset_curves[i];
               stroker_emit_curve_to(stroker,
                                     bez->x2, bez->y2,
                                     bez->x3, bez->y3,
                                     bez->x4, bez->y4);
            }
         }

         prev[0] = coords[4];
         prev[1] = coords[5];
      }
   }

   if (floatsEqual(start[0], prev[0]) &&
       floatsEqual(start[1], prev[1])) {
      /* closed subpath, join first and last point */
#if STROKE_DEBUG
      debug_printf("\n stroker: closed subpath\n");
#endif
      create_joins(stroker, prev[0], prev[1], start_tangent,
                   stroker_join_mode(stroker));
      return VG_TRUE;
   } else {
#if STROKE_DEBUG
      debug_printf("\n stroker: open subpath\n");
#endif
      return VG_FALSE;
   }
#undef MAX_OFFSET
}

static void stroker_process_subpath(struct stroker *stroker)
{
   VGboolean fwclosed, bwclosed;
   VGfloat fw_start_tangent[4], bw_start_tangent[4];
   struct stroke_iterator fwit;
   struct stroke_iterator bwit;
   debug_assert(stroker->segments->num_elements > 0);

   memset(fw_start_tangent, 0,
          sizeof(VGfloat)*4);
   memset(bw_start_tangent, 0,
          sizeof(VGfloat)*4);

   stroke_forward_iterator(&fwit, stroker->segments,
                           stroker->control_points);
   stroke_backward_iterator(&bwit, stroker->segments,
                            stroker->control_points);

   debug_assert(fwit.cmds[0] == VG_MOVE_TO_ABS);

   fwclosed = vg_stroke_outline(&fwit, stroker, VG_FALSE, fw_start_tangent);
   bwclosed = vg_stroke_outline(&bwit, stroker, !fwclosed, bw_start_tangent);

   if (!bwclosed)
      create_joins(stroker,
                   fwit.coords[0], fwit.coords[1], fw_start_tangent,
                   stroker_cap_mode(stroker));
   else {
      /* hack to handle the requirement of the VG spec that says that strokes
       * of len==0 that have butt cap or round cap still need
       * to be rendered. (8.7.4 Stroke Generation) */
      if (stroker->segments->num_elements <= 3) {
         VGPathCommand cmd;
         VGfloat data[8], coords[8];
         struct stroke_iterator *it = &fwit;

         stroke_forward_iterator(it, stroker->segments,
                                 stroker->control_points);
         cmd = stroke_itr_command(it);
         stroke_itr_coords(it, coords);
         if (cmd != VG_MOVE_TO_ABS) {
            memset(data, 0, sizeof(VGfloat) * 8);
            if (!is_segment_null(cmd, coords, data))
               return;
         } else {
            data[0] = coords[0];
            data[1] = coords[1];
         }
         while (it->has_next(it)) {
            it->next(it);
            cmd = stroke_itr_command(it);
            stroke_itr_coords(it, coords);
            if (!is_segment_null(cmd, coords, data))
               return;
         }
         /* generate the square/round cap */
         if (stroker->cap_style == VG_CAP_SQUARE) {
            VGfloat offset = stroker->stroke_width / 2;
            stroker_emit_move_to(stroker, data[0] - offset,
                                 data[1] - offset);
            stroker_emit_line_to(stroker, data[0] + offset,
                                 data[1] - offset);
            stroker_emit_line_to(stroker, data[0] + offset,
                                 data[1] + offset);
            stroker_emit_line_to(stroker, data[0] - offset,
                                 data[1] + offset);
            stroker_emit_line_to(stroker, data[0] - offset,
                                 data[1] - offset);
         } else if (stroker->cap_style == VG_CAP_ROUND) {
            VGfloat offset = stroker->stroke_width / 2;
            VGfloat cx = data[0], cy = data[1];
            { /*circle */
               struct arc arc;
               struct matrix matrix;
               matrix_load_identity(&matrix);

               stroker_emit_move_to(stroker, cx + offset, cy);
               arc_init(&arc, VG_SCCWARC_TO,
                        cx + offset, cy,
                        cx - offset, cy,
                        offset, offset, 0);
               arc_stroker_emit(&arc, stroker, &matrix);
               arc_init(&arc, VG_SCCWARC_TO,
                         cx - offset, cy,
                         cx + offset, cy,
                         offset, offset, 0);
               arc_stroker_emit(&arc, stroker, &matrix);
            }
         }
      }
   }
}

static INLINE VGfloat dash_pattern(struct dash_stroker *stroker,
                                   VGint idx)
{
   if (stroker->dash_pattern[idx] < 0)
      return 0.f;
   return stroker->dash_pattern[idx];
}

static void dash_stroker_process_subpath(struct stroker *str)
{
   struct dash_stroker *stroker = (struct dash_stroker *)str;
   VGfloat sum_length = 0;
   VGint i;
   VGint idash = 0;
   VGfloat pos = 0;
   VGfloat elen = 0;
   VGfloat doffset = stroker->dash_phase;
   VGfloat estart = 0;
   VGfloat estop = 0;
   VGfloat cline[4];
   struct stroke_iterator it;
   VGfloat prev[2];
   VGfloat move_to_pos[2];
   VGfloat line_to_pos[2];

   VGboolean has_move_to = VG_FALSE;

   stroke_flat_iterator(&it, stroker->base.segments,
                        stroker->base.control_points);

   stroke_itr_coords(&it, prev);
   move_to_pos[0] = prev[0];
   move_to_pos[1] = prev[1];

   debug_assert(stroker->dash_pattern_num > 0);

   for (i = 0; i < stroker->dash_pattern_num; ++i) {
      sum_length += dash_pattern(stroker, i);
   }

   if (floatIsZero(sum_length)) {
      return;
   }

   doffset -= floorf(doffset / sum_length) * sum_length;

   while (!floatIsZero(doffset) && doffset >= dash_pattern(stroker, idash)) {
      doffset -= dash_pattern(stroker, idash);
      idash = (idash + 1) % stroker->dash_pattern_num;
   }

   while (it.has_next(&it)) {
      VGPathCommand cmd;
      VGfloat coords[8];
      VGboolean done;

      it.next(&it);
      cmd = stroke_itr_command(&it);
      stroke_itr_coords(&it, coords);

      debug_assert(cmd == VG_LINE_TO_ABS);
      cline[0] = prev[0];
      cline[1] = prev[1];
      cline[2] = coords[0];
      cline[3] = coords[1];

      elen = line_lengthv(cline);

      estop = estart + elen;

      done = pos >= estop;
      while (!done) {
         VGfloat p2[2];

         VGint idash_incr = 0;
         VGboolean has_offset = doffset > 0;
         VGfloat dpos = pos + dash_pattern(stroker, idash) - doffset - estart;

         debug_assert(dpos >= 0);

         if (dpos > elen) { /* dash extends this line */
            doffset = dash_pattern(stroker, idash) - (dpos - elen);
            pos = estop;
            done = VG_TRUE;
            p2[0] = cline[2];
            p2[1] = cline[3];
         } else { /* Dash is on this line */
            line_point_at(cline, dpos/elen, p2);
            pos = dpos + estart;
            done = pos >= estop;
            idash_incr = 1;
            doffset = 0;
         }

         if (idash % 2 == 0) {
            line_to_pos[0] = p2[0];
            line_to_pos[1] = p2[1];

            if (!has_offset || !has_move_to) {
               stroker_move_to(&stroker->stroker, move_to_pos[0], move_to_pos[1]);
               has_move_to = VG_TRUE;
            }
            stroker_line_to(&stroker->stroker, line_to_pos[0], line_to_pos[1]);
         } else {
            move_to_pos[0] = p2[0];
            move_to_pos[1] = p2[1];
         }

         idash = (idash + idash_incr) % stroker->dash_pattern_num;
      }

      estart = estop;
      prev[0] = coords[0];
      prev[1] = coords[1];
   }

   if (it.curve_poly) {
      polygon_destroy(it.curve_poly);
      it.curve_poly = 0;
   }

   stroker->base.path = stroker->stroker.path;
}

static void default_begin(struct stroker *stroker)
{
   array_reset(stroker->segments);
   array_reset(stroker->control_points);
}

static void default_end(struct stroker *stroker)
{
   if (stroker->segments->num_elements > 0)
      stroker->process_subpath(stroker);
}


static void dash_stroker_begin(struct stroker *stroker)
{
   struct dash_stroker *dasher =
      (struct dash_stroker *)stroker;

   default_begin(&dasher->stroker);
   default_begin(stroker);
}

static void dash_stroker_end(struct stroker *stroker)
{
   struct dash_stroker *dasher =
      (struct dash_stroker *)stroker;

   default_end(stroker);
   default_end(&dasher->stroker);
}

void stroker_init(struct stroker *stroker,
                  struct vg_state *state)
{
   stroker->stroke_width = state->stroke.line_width.f;
   stroker->miter_limit = state->stroke.miter_limit.f;
   stroker->cap_style = state->stroke.cap_style;
   stroker->join_style = state->stroke.join_style;

   stroker->begin = default_begin;
   stroker->process_subpath = stroker_process_subpath;
   stroker->end = default_end;

   stroker->segments = array_create(sizeof(VGubyte));
   stroker->control_points = array_create(sizeof(VGfloat));

   stroker->back1_x = 0;
   stroker->back1_y = 0;
   stroker->back2_x = 0;
   stroker->back2_y = 0;

   stroker->path = path_create(VG_PATH_DATATYPE_F, 1.0f, 0.0f,
                               0, 0, VG_PATH_CAPABILITY_ALL);

   /* Initialize with an invalid value */
   stroker->last_cmd = (VGPathCommand)0;
}

void dash_stroker_init(struct stroker *str,
                       struct vg_state *state)
{
   struct dash_stroker *stroker = (struct dash_stroker *)str;
   int i;

   stroker_init(str, state);
   stroker_init(&stroker->stroker, state);

   {
      int real_num = state->stroke.dash_pattern_num;
      if (real_num % 2)/* if odd, ignore the last one */
         --real_num;
      for (i = 0; i < real_num; ++i)
         stroker->dash_pattern[i] = state->stroke.dash_pattern[i].f;
      stroker->dash_pattern_num = real_num;
   }

   stroker->dash_phase = state->stroke.dash_phase.f;
   stroker->dash_phase_reset = state->stroke.dash_phase_reset;

   stroker->base.begin = dash_stroker_begin;
   stroker->base.process_subpath = dash_stroker_process_subpath;
   stroker->base.end = dash_stroker_end;
   path_destroy(stroker->base.path);
   stroker->base.path = NULL;
}

void stroker_begin(struct stroker *stroker)
{
   stroker->begin(stroker);
}

void stroker_end(struct stroker *stroker)
{
   stroker->end(stroker);
}

void stroker_cleanup(struct stroker *stroker)
{
   array_destroy(stroker->segments);
   array_destroy(stroker->control_points);
}

void dash_stroker_cleanup(struct dash_stroker *stroker)
{
   /* if stroker->base.path is null means we never
    * processed a valid path so delete the temp one
    * we already created */
   if (!stroker->base.path)
      path_destroy(stroker->stroker.path);
   stroker_cleanup(&stroker->stroker);
   stroker_cleanup((struct stroker*)stroker);
}
