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

#include "path.h"

#include "stroker.h"
#include "polygon.h"
#include "bezier.h"
#include "matrix.h"
#include "vg_context.h"
#include "util_array.h"
#include "arc.h"
#include "path_utils.h"
#include "paint.h"
#include "shader.h"

#include "util/u_memory.h"

#include <assert.h>

#define DEBUG_PATH 0

struct path {
   struct vg_object base;
   VGbitfield caps;
   VGboolean dirty;
   VGboolean dirty_stroke;

   VGPathDatatype datatype;

   VGfloat scale;
   VGfloat bias;

   VGint num_segments;

   struct array * segments;
   struct array * control_points;

   struct {
      struct polygon_array polygon_array;
      struct matrix matrix;
   } fill_polys;

   struct {
      struct path *path;
      struct matrix matrix;
      VGfloat stroke_width;
      VGfloat miter_limit;
      VGCapStyle cap_style;
      VGJoinStyle join_style;
   } stroked;
};


static INLINE void data_at(void **data,
                           struct path *p,
                           VGint start, VGint count,
                           VGfloat *out)
{
   VGPathDatatype dt = p->datatype;
   VGint i;
   VGint end = start + count;
   VGfloat *itr = out;

   switch(dt) {
   case VG_PATH_DATATYPE_S_8: {
      VGbyte **bdata = (VGbyte **)data;
      for (i = start; i < end; ++i) {
         *itr = (*bdata)[i];
         ++itr;
      }
      *bdata += count;
   }
      break;
   case VG_PATH_DATATYPE_S_16: {
      VGshort **bdata = (VGshort **)data;
      for (i = start; i < end; ++i) {
         *itr = (*bdata)[i];
         ++itr;
      }
      *bdata += count;
   }
      break;
   case VG_PATH_DATATYPE_S_32: {
      VGint **bdata = (VGint **)data;
      for (i = start; i < end; ++i) {
         *itr = (*bdata)[i];
         ++itr;
      }
      *bdata += count;
   }
      break;
   case VG_PATH_DATATYPE_F: {
      VGfloat **fdata = (VGfloat **)data;
      for (i = start; i < end; ++i) {
         *itr = (*fdata)[i];
         ++itr;
      }
      *fdata += count;
   }
      break;
   default:
      debug_assert(!"Unknown path datatype!");
   }
}


void vg_float_to_datatype(VGPathDatatype datatype,
                          VGubyte *common_data,
                          const VGfloat *data,
                          VGint num_coords)
{
   VGint i;
   switch(datatype) {
   case VG_PATH_DATATYPE_S_8: {
      for (i = 0; i < num_coords; ++i) {
         common_data[i] = (VGubyte)data[i];
      }
   }
      break;
   case VG_PATH_DATATYPE_S_16: {
      VGshort *buf = (VGshort*)common_data;
      for (i = 0; i < num_coords; ++i) {
         buf[i] = (VGshort)data[i];
      }
   }
      break;
   case VG_PATH_DATATYPE_S_32: {
      VGint *buf = (VGint*)common_data;
      for (i = 0; i < num_coords; ++i) {
         buf[i] = (VGint)data[i];
      }
   }
      break;
   case VG_PATH_DATATYPE_F: {
      memcpy(common_data, data, sizeof(VGfloat) * num_coords);
   }
      break;
   default:
      debug_assert(!"Unknown path datatype!");
   }
}

static void coords_adjust_by_scale_bias(struct path *p,
                                        void *pdata, VGint num_coords,
                                        VGfloat scale, VGfloat bias,
                                        VGPathDatatype datatype)
{
   VGfloat data[8];
   void *coords = (VGfloat *)pdata;
   VGubyte *common_data = (VGubyte *)pdata;
   VGint size_dst = size_for_datatype(datatype);
   VGint i;

   for (i = 0; i < num_coords; ++i) {
      data_at(&coords, p, 0, 1, data);
      data[0] = data[0] * scale + bias;
      vg_float_to_datatype(datatype, common_data, data, 1);
      common_data += size_dst;
   }
}

struct path * path_create(VGPathDatatype dt, VGfloat scale, VGfloat bias,
                          VGint segmentCapacityHint,
                          VGint coordCapacityHint,
                          VGbitfield capabilities)
{
   struct path *path = CALLOC_STRUCT(path);

   vg_init_object(&path->base, vg_current_context(), VG_OBJECT_PATH);
   path->caps = capabilities & VG_PATH_CAPABILITY_ALL;
   vg_context_add_object(vg_current_context(), &path->base);

   path->datatype = dt;
   path->scale = scale;
   path->bias = bias;

   path->segments = array_create(size_for_datatype(VG_PATH_DATATYPE_S_8));
   path->control_points = array_create(size_for_datatype(dt));

   path->dirty = VG_TRUE;
   path->dirty_stroke = VG_TRUE;

   return path;
}

static void polygon_array_cleanup(struct polygon_array *polyarray)
{
   if (polyarray->array) {
      VGint i;

      for (i = 0; i < polyarray->array->num_elements; i++) {
         struct polygon *p = ((struct polygon **) polyarray->array->data)[i];
         polygon_destroy(p);
      }

      array_destroy(polyarray->array);
      polyarray->array = NULL;
   }
}

void path_destroy(struct path *p)
{
   vg_context_remove_object(vg_current_context(), &p->base);

   array_destroy(p->segments);
   array_destroy(p->control_points);

   polygon_array_cleanup(&p->fill_polys.polygon_array);

   if (p->stroked.path)
      path_destroy(p->stroked.path);

   FREE(p);
}

VGbitfield path_capabilities(struct path *p)
{
   return p->caps;
}

void path_set_capabilities(struct path *p, VGbitfield bf)
{
   p->caps = (bf & VG_PATH_CAPABILITY_ALL);
}

void path_append_data(struct path *p,
                      VGint numSegments,
                      const VGubyte * pathSegments,
                      const void * pathData)
{
   VGint old_segments = p->num_segments;
   VGint num_new_coords = num_elements_for_segments(pathSegments, numSegments);
   array_append_data(p->segments, pathSegments, numSegments);
   array_append_data(p->control_points, pathData, num_new_coords);

   p->num_segments += numSegments;
   if (!floatsEqual(p->scale, 1.f) || !floatsEqual(p->bias, 0.f)) {
      VGubyte *coords = (VGubyte*)p->control_points->data;
      coords_adjust_by_scale_bias(p,
                                  coords + old_segments * p->control_points->datatype_size,
                                  num_new_coords,
                                  p->scale, p->bias, p->datatype);
   }
   p->dirty = VG_TRUE;
   p->dirty_stroke = VG_TRUE;
}

VGint path_num_segments(struct path *p)
{
   return p->num_segments;
}

static INLINE void map_if_relative(VGfloat ox, VGfloat oy,
                                   VGboolean relative,
                                   VGfloat *x, VGfloat *y)
{
   if (relative) {
      if (x)
         *x += ox;
      if (y)
         *y += oy;
   }
}

static INLINE void close_polygon(struct polygon *current,
                                 VGfloat sx, VGfloat sy,
                                 VGfloat ox, VGfloat oy,
                                 struct  matrix *matrix)
{
   if (!floatsEqual(sx, ox) ||
       !floatsEqual(sy, oy)) {
      VGfloat x0 = sx;
      VGfloat y0 = sy;
      matrix_map_point(matrix, x0, y0, &x0, &y0);
      polygon_vertex_append(current, x0, y0);
   }
}

static void convert_path(struct path *p,
                          VGPathDatatype to,
                          void *dst,
                          VGint num_coords)
{
   VGfloat data[8];
   void *coords = (VGfloat *)p->control_points->data;
   VGubyte *common_data = (VGubyte *)dst;
   VGint size_dst = size_for_datatype(to);
   VGint i;

   for (i = 0; i < num_coords; ++i) {
      data_at(&coords, p, 0, 1, data);
      vg_float_to_datatype(to, common_data, data, 1);
      common_data += size_dst;
   }
}

static void polygon_array_calculate_bounds( struct polygon_array *polyarray )
{
   struct array *polys = polyarray->array;
   VGfloat min_x, max_x;
   VGfloat min_y, max_y;
   VGfloat bounds[4];
   unsigned i;

   assert(polys);

   if (!polys->num_elements) {
      polyarray->min_x = 0.0f;
      polyarray->min_y = 0.0f;
      polyarray->max_x = 0.0f;
      polyarray->max_y = 0.0f;
      return;
   }

   polygon_bounding_rect((((struct polygon**)polys->data)[0]), bounds);
   min_x = bounds[0];
   min_y = bounds[1];
   max_x = bounds[0] + bounds[2];
   max_y = bounds[1] + bounds[3];
   for (i = 1; i < polys->num_elements; ++i) {
      struct polygon *p = (((struct polygon**)polys->data)[i]);
      polygon_bounding_rect(p, bounds);
      min_x = MIN2(min_x, bounds[0]);
      min_y = MIN2(min_y, bounds[1]);
      max_x = MAX2(max_x, bounds[0] + bounds[2]);
      max_y = MAX2(max_y, bounds[1] + bounds[3]);
   }

   polyarray->min_x = min_x;
   polyarray->min_y = min_y;
   polyarray->max_x = max_x;
   polyarray->max_y = max_y;
}


static struct polygon_array * path_get_fill_polygons(struct path *p, struct matrix *matrix)
{
   VGint i;
   struct polygon *current = 0;
   VGfloat sx, sy, px, py, ox, oy;
   VGfloat x0, y0, x1, y1, x2, y2, x3, y3;
   VGfloat data[8];
   void *coords = (VGfloat *)p->control_points->data;
   struct array *array;

   memset(data, 0, sizeof(data));

   if (p->fill_polys.polygon_array.array)
   {
      if (memcmp( &p->fill_polys.matrix,
                  matrix,
                  sizeof *matrix ) == 0 && p->dirty == VG_FALSE)
      {
         return &p->fill_polys.polygon_array;
      }
      else {
         polygon_array_cleanup(&p->fill_polys.polygon_array);
      }
   }

   /* an array of pointers to polygons */
   array = array_create(sizeof(struct polygon *));

   sx = sy = px = py = ox = oy = 0.f;

   if (p->num_segments)
      current = polygon_create(32);

   for (i = 0; i < p->num_segments; ++i) {
      VGubyte segment = ((VGubyte*)(p->segments->data))[i];
      VGint command = SEGMENT_COMMAND(segment);
      VGboolean relative = SEGMENT_ABS_REL(segment);

      switch(command) {
      case VG_CLOSE_PATH:
         close_polygon(current, sx, sy, ox, oy, matrix);
         ox = sx;
         oy = sy;
         break;
      case VG_MOVE_TO:
         if (current && polygon_vertex_count(current) > 0) {
            /* add polygon */
            close_polygon(current, sx, sy, ox, oy, matrix);
            array_append_data(array, &current, 1);
            current = polygon_create(32);
         }
         data_at(&coords, p, 0, 2, data);
         x0 = data[0];
         y0 = data[1];
         map_if_relative(ox, oy, relative, &x0, &y0);
         sx = x0;
         sy = y0;
         ox = x0;
         oy = y0;
         px = x0;
         py = y0;
         matrix_map_point(matrix, x0, y0, &x0, &y0);
         polygon_vertex_append(current, x0, y0);
         break;
      case VG_LINE_TO:
         data_at(&coords, p, 0, 2, data);
         x0 = data[0];
         y0 = data[1];
         map_if_relative(ox, oy, relative, &x0, &y0);
         ox = x0;
         oy = y0;
         px = x0;
         py = y0;
         matrix_map_point(matrix, x0, y0, &x0, &y0);
         polygon_vertex_append(current, x0, y0);
         break;
      case VG_HLINE_TO:
         data_at(&coords, p, 0, 1, data);
         x0 = data[0];
         y0 = oy;
         map_if_relative(ox, oy, relative, &x0, 0);
         ox = x0;
         px = x0;
         py = y0;
         matrix_map_point(matrix, x0, y0, &x0, &y0);
         polygon_vertex_append(current, x0, y0);
         break;
      case VG_VLINE_TO:
         data_at(&coords, p, 0, 1, data);
         x0 = ox;
         y0 = data[0];
         map_if_relative(ox, oy, relative, 0, &y0);
         oy = y0;
         px = x0;
         py = y0;
         matrix_map_point(matrix, x0, y0, &x0, &y0);
         polygon_vertex_append(current, x0, y0);
         break;
      case VG_CUBIC_TO: {
         struct bezier bezier;
         data_at(&coords, p, 0, 6, data);
         x0 = ox;
         y0 = oy;
         x1 = data[0];
         y1 = data[1];
         x2 = data[2];
         y2 = data[3];
         x3 = data[4];
         y3 = data[5];
         map_if_relative(ox, oy, relative, &x1, &y1);
         map_if_relative(ox, oy, relative, &x2, &y2);
         map_if_relative(ox, oy, relative, &x3, &y3);
         ox = x3;
         oy = y3;
         px = x2;
         py = y2;
         assert(matrix_is_affine(matrix));
         matrix_map_point(matrix, x0, y0, &x0, &y0);
         matrix_map_point(matrix, x1, y1, &x1, &y1);
         matrix_map_point(matrix, x2, y2, &x2, &y2);
         matrix_map_point(matrix, x3, y3, &x3, &y3);
         bezier_init(&bezier, x0, y0, x1, y1,
                       x2, y2, x3, y3);
         bezier_add_to_polygon(&bezier, current);
      }
         break;
      case VG_QUAD_TO: {
         struct bezier bezier;
         data_at(&coords, p, 0, 4, data);
         x0 = ox;
         y0 = oy;
         x1 = data[0];
         y1 = data[1];
         x3 = data[2];
         y3 = data[3];
         map_if_relative(ox, oy, relative, &x1, &y1);
         map_if_relative(ox, oy, relative, &x3, &y3);
         px = x1;
         py = y1;
         { /* form a cubic out of it */
            x2 = (x3 + 2*x1) / 3.f;
            y2 = (y3 + 2*y1) / 3.f;
            x1 = (x0 + 2*x1) / 3.f;
            y1 = (y0 + 2*y1) / 3.f;
         }
         ox = x3;
         oy = y3;
         assert(matrix_is_affine(matrix));
         matrix_map_point(matrix, x0, y0, &x0, &y0);
         matrix_map_point(matrix, x1, y1, &x1, &y1);
         matrix_map_point(matrix, x2, y2, &x2, &y2);
         matrix_map_point(matrix, x3, y3, &x3, &y3);
         bezier_init(&bezier, x0, y0, x1, y1,
                       x2, y2, x3, y3);
         bezier_add_to_polygon(&bezier, current);
      }
         break;
      case VG_SQUAD_TO: {
         struct bezier bezier;
         data_at(&coords, p, 0, 2, data);
         x0 = ox;
         y0 = oy;
         x1 = 2*ox-px;
         y1 = 2*oy-py;
         x3 = data[0];
         y3 = data[1];
         map_if_relative(ox, oy, relative, &x3, &y3);
         px = x1;
         py = y1;
         { /* form a cubic out of it */
            x2 = (x3 + 2*x1) / 3.f;
            y2 = (y3 + 2*y1) / 3.f;
            x1 = (x0 + 2*x1) / 3.f;
            y1 = (y0 + 2*y1) / 3.f;
         }
         ox = x3;
         oy = y3;
         assert(matrix_is_affine(matrix));
         matrix_map_point(matrix, x0, y0, &x0, &y0);
         matrix_map_point(matrix, x1, y1, &x1, &y1);
         matrix_map_point(matrix, x2, y2, &x2, &y2);
         matrix_map_point(matrix, x3, y3, &x3, &y3);
         bezier_init(&bezier, x0, y0, x1, y1,
                     x2, y2, x3, y3);
         bezier_add_to_polygon(&bezier, current);
      }
         break;
      case VG_SCUBIC_TO: {
         struct bezier bezier;
         data_at(&coords, p, 0, 4, data);
         x0 = ox;
         y0 = oy;
         x1 = 2*ox-px;
         y1 = 2*oy-py;
         x2 = data[0];
         y2 = data[1];
         x3 = data[2];
         y3 = data[3];
         map_if_relative(ox, oy, relative, &x2, &y2);
         map_if_relative(ox, oy, relative, &x3, &y3);
         ox = x3;
         oy = y3;
         px = x2;
         py = y2;
         assert(matrix_is_affine(matrix));
         matrix_map_point(matrix, x0, y0, &x0, &y0);
         matrix_map_point(matrix, x1, y1, &x1, &y1);
         matrix_map_point(matrix, x2, y2, &x2, &y2);
         matrix_map_point(matrix, x3, y3, &x3, &y3);
         bezier_init(&bezier, x0, y0, x1, y1,
                              x2, y2, x3, y3);
         bezier_add_to_polygon(&bezier, current);
      }
         break;
      case VG_SCCWARC_TO:
      case VG_SCWARC_TO:
      case VG_LCCWARC_TO:
      case VG_LCWARC_TO: {
         VGfloat rh, rv, rot;
         struct arc arc;

         data_at(&coords, p, 0, 5, data);
         x0  = ox;
         y0  = oy;
         rh  = data[0];
         rv  = data[1];
         rot = data[2];
         x1  = data[3];
         y1  = data[4];
         map_if_relative(ox, oy, relative, &x1, &y1);
#if 0
         debug_printf("------- ARC (%f, %f), (%f, %f) %f, %f, %f\n",
                      x0, y0, x1, y1, rh, rv, rot);
#endif
         arc_init(&arc, command, x0, y0, x1, y1,
                  rh, rv, rot);
         arc_add_to_polygon(&arc, current,
                            matrix);
         ox = x1;
         oy = y1;
         px = x1;
         py = y1;
      }
         break;
      default:
         abort();
         assert(!"Unknown segment!");
      }
   }
   if (current) {
      if (polygon_vertex_count(current) > 0) {
         close_polygon(current, sx, sy, ox, oy, matrix);
         array_append_data(array, &current, 1);
      } else
         polygon_destroy(current);
   }

   p->fill_polys.polygon_array.array = array;
   p->fill_polys.matrix = *matrix;

   polygon_array_calculate_bounds( &p->fill_polys.polygon_array );

   p->dirty = VG_FALSE;

   return &p->fill_polys.polygon_array;
}

VGbyte path_datatype_size(struct path *p)
{
   return size_for_datatype(p->datatype);
}

VGPathDatatype path_datatype(struct path *p)
{
   return p->datatype;
}

VGfloat path_scale(struct path *p)
{
   return p->scale;
}

VGfloat path_bias(struct path *p)
{
   return p->bias;
}

VGint path_num_coords(struct path *p)
{
   return num_elements_for_segments((VGubyte*)p->segments->data,
                                    p->num_segments);
}

void path_modify_coords(struct path *p,
                        VGint startIndex,
                        VGint numSegments,
                        const void * pathData)
{
   VGubyte *segments = (VGubyte*)(p->segments->data);
   VGint count = num_elements_for_segments(&segments[startIndex], numSegments);
   VGint start_cp = num_elements_for_segments(segments, startIndex);

   array_change_data(p->control_points, pathData, start_cp, count);
   coords_adjust_by_scale_bias(p,
                               ((VGubyte*)p->control_points->data) +
                               (startIndex * p->control_points->datatype_size),
                               path_num_coords(p),
                               p->scale, p->bias, p->datatype);
   p->dirty = VG_TRUE;
   p->dirty_stroke = VG_TRUE;
}

void path_for_each_segment(struct path *path,
                           path_for_each_cb cb,
                           void *user_data)
{
   VGint i;
   struct path_for_each_data p;
   VGfloat data[8];
   void *coords = (VGfloat *)path->control_points->data;

   p.coords = data;
   p.sx = p.sy = p.px = p.py = p.ox = p.oy = 0.f;
   p.user_data = user_data;

   for (i = 0; i < path->num_segments; ++i) {
      VGint command;
      VGboolean relative;

      p.segment = ((VGubyte*)(path->segments->data))[i];
      command = SEGMENT_COMMAND(p.segment);
      relative = SEGMENT_ABS_REL(p.segment);

      switch(command) {
      case VG_CLOSE_PATH:
         cb(path, &p);
         break;
      case VG_MOVE_TO:
         data_at(&coords, path, 0, 2, data);
         map_if_relative(p.ox, p.oy, relative, &data[0], &data[1]);
         cb(path, &p);
         p.sx = data[0];
         p.sy = data[1];
         p.ox = data[0];
         p.oy = data[1];
         p.px = data[0];
         p.py = data[1];
         break;
      case VG_LINE_TO:
         data_at(&coords, path, 0, 2, data);
         map_if_relative(p.ox, p.oy, relative, &data[0], &data[1]);
         cb(path, &p);
         p.ox = data[0];
         p.oy = data[1];
         p.px = data[0];
         p.py = data[1];
         break;
      case VG_HLINE_TO:
         data_at(&coords, path, 0, 1, data);
         map_if_relative(p.ox, p.oy, relative, &data[0], 0);
         p.segment = VG_LINE_TO;
         data[1] = p.oy;
         cb(path, &p);
         p.ox = data[0];
         p.oy = data[1];
         p.px = data[0];
         p.py = data[1];
         break;
      case VG_VLINE_TO:
         data_at(&coords, path, 0, 1, data);
         map_if_relative(p.ox, p.oy, relative, 0, &data[0]);
         p.segment = VG_LINE_TO;
         data[1] = data[0];
         data[0] = p.ox;
         cb(path, &p);
         p.ox = data[0];
         p.oy = data[1];
         p.px = data[0];
         p.py = data[1];
         break;
      case VG_CUBIC_TO: {
         data_at(&coords, path, 0, 6, data);
         map_if_relative(p.ox, p.oy, relative, &data[0], &data[1]);
         map_if_relative(p.ox, p.oy, relative, &data[2], &data[3]);
         map_if_relative(p.ox, p.oy, relative, &data[4], &data[5]);
         cb(path, &p);
         p.px = data[2];
         p.py = data[3];
         p.ox = data[4];
         p.oy = data[5];
      }
         break;
      case VG_QUAD_TO: {
         data_at(&coords, path, 0, 4, data);
         map_if_relative(p.ox, p.oy, relative, &data[0], &data[1]);
         map_if_relative(p.ox, p.oy, relative, &data[2], &data[3]);
         cb(path, &p);
         p.px = data[0];
         p.py = data[1];
         p.ox = data[2];
         p.oy = data[3];
      }
         break;
      case VG_SQUAD_TO: {
         data_at(&coords, path, 0, 2, data);
         map_if_relative(p.ox, p.oy, relative, &data[0], &data[1]);
         cb(path, &p);
         p.px = 2*p.ox-p.px;
         p.py = 2*p.oy-p.py;
         p.ox = data[2];
         p.oy = data[3];
      }
         break;
      case VG_SCUBIC_TO: {
         data_at(&coords, path, 0, 4, data);
         map_if_relative(p.ox, p.oy, relative, &data[0], &data[1]);
         map_if_relative(p.ox, p.oy, relative, &data[2], &data[3]);
         cb(path, &p);
         p.px = data[0];
         p.py = data[1];
         p.ox = data[2];
         p.oy = data[3];
      }
         break;
      case VG_SCCWARC_TO:
      case VG_SCWARC_TO:
      case VG_LCCWARC_TO:
      case VG_LCWARC_TO: {
         data_at(&coords, path, 0, 5, data);
         map_if_relative(p.ox, p.oy, relative, &data[3], &data[4]);
#if 0
         debug_printf("------- ARC (%f, %f), (%f, %f) %f, %f, %f\n",
                      p.ox, p.oy, data[3], data[4], data[0], data[1], data[2]);
#endif
         cb(path, &p);
         p.ox = data[3];
         p.oy = data[4];
         p.px = data[3];
         p.py = data[4];
      }
         break;
      default:
         abort();
         assert(!"Unknown segment!");
      }
   }
}

struct transform_data {
   struct array *segments;
   struct array *coords;

   struct matrix *matrix;

   VGPathDatatype datatype;
};

static VGboolean transform_cb(struct path *p,
                              struct path_for_each_data *pd)
{
   struct transform_data *td = (struct transform_data *)pd->user_data;
   VGint num_coords = num_elements_for_segments(&pd->segment, 1);
   VGubyte segment = SEGMENT_COMMAND(pd->segment);/* abs bit is 0 */
   VGfloat data[8];
   VGubyte common_data[sizeof(VGfloat)*8];

   memcpy(data, pd->coords, sizeof(VGfloat) * num_coords);

   switch(segment) {
   case VG_CLOSE_PATH:
      break;
   case VG_MOVE_TO:
      matrix_map_point(td->matrix,
                       data[0], data[1], &data[0], &data[1]);
      break;
   case VG_LINE_TO:
      matrix_map_point(td->matrix,
                       data[0], data[1], &data[0], &data[1]);
      break;
   case VG_HLINE_TO:
   case VG_VLINE_TO:
      assert(0);
      break;
   case VG_QUAD_TO:
      matrix_map_point(td->matrix,
                       data[0], data[1], &data[0], &data[1]);
      matrix_map_point(td->matrix,
                       data[2], data[3], &data[2], &data[3]);
      break;
   case VG_CUBIC_TO:
      matrix_map_point(td->matrix,
                       data[0], data[1], &data[0], &data[1]);
      matrix_map_point(td->matrix,
                       data[2], data[3], &data[2], &data[3]);
      matrix_map_point(td->matrix,
                       data[4], data[5], &data[4], &data[5]);
      break;
   case VG_SQUAD_TO:
      matrix_map_point(td->matrix,
                       data[0], data[1], &data[0], &data[1]);
      break;
   case VG_SCUBIC_TO:
      matrix_map_point(td->matrix,
                       data[0], data[1], &data[0], &data[1]);
      matrix_map_point(td->matrix,
                       data[2], data[3], &data[2], &data[3]);
      break;
   case VG_SCCWARC_TO:
   case VG_SCWARC_TO:
   case VG_LCCWARC_TO:
   case VG_LCWARC_TO: {
      struct arc arc;
      struct path *path = path_create(td->datatype,
                                      1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);
      arc_init(&arc, segment,
               pd->ox, pd->oy, data[3], data[4],
               data[0], data[1], data[2]);

      arc_to_path(&arc, path, td->matrix);

      num_coords = path_num_coords(path);

      array_append_data(td->segments, path->segments->data,
                        path->num_segments);
      array_append_data(td->coords, path->control_points->data,
                        num_coords);
      path_destroy(path);

      return VG_TRUE;
   }
      break;
   default:
      break;
   }

   vg_float_to_datatype(td->datatype, common_data, data, num_coords);

   array_append_data(td->segments, &pd->segment, 1);
   array_append_data(td->coords, common_data, num_coords);
   return VG_TRUE;
}

void path_transform(struct path *dst, struct path *src)
{
   struct transform_data data;
   struct vg_context *ctx = dst->base.ctx;

   data.segments =  dst->segments;
   data.coords   =  dst->control_points;
   data.matrix   = &ctx->state.vg.path_user_to_surface_matrix;
   data.datatype = dst->datatype;

   path_for_each_segment(src, transform_cb, (void*)&data);

   dst->num_segments = dst->segments->num_elements;
   dst->dirty = VG_TRUE;
   dst->dirty_stroke = VG_TRUE;
}

void path_append_path(struct path *dst,
                      struct path *src)
{
   VGint num_coords = path_num_coords(src);
   void *dst_data = malloc(size_for_datatype(dst->datatype) * num_coords);
   array_append_data(dst->segments,
                     src->segments->data,
                     src->num_segments);
   convert_path(src, dst->datatype,
                dst_data, num_coords);
   array_append_data(dst->control_points,
                     dst_data,
                     num_coords);
   free(dst_data);

   dst->num_segments += src->num_segments;
   dst->dirty = VG_TRUE;
   dst->dirty_stroke = VG_TRUE;
}

static INLINE VGboolean is_segment_arc(VGubyte segment)
{
   VGubyte scommand = SEGMENT_COMMAND(segment);
   return (scommand == VG_SCCWARC_TO ||
           scommand == VG_SCWARC_TO ||
           scommand == VG_LCCWARC_TO ||
           scommand == VG_LCWARC_TO);
}

struct path_iter_data {
   struct path *path;
   VGubyte segment;
   void *coords;
   VGfloat px, py, ox, oy, sx, sy;
};
static INLINE VGubyte normalize_coords(struct path_iter_data *pd,
                                       VGint *num_coords,
                                       VGfloat *data)
{
   VGint command = SEGMENT_COMMAND(pd->segment);
   VGboolean relative = SEGMENT_ABS_REL(pd->segment);

   switch(command) {
   case VG_CLOSE_PATH:
      *num_coords = 0;
      pd->ox = pd->sx;
      pd->oy = pd->sy;
      return VG_CLOSE_PATH;
      break;
   case VG_MOVE_TO:
      data_at(&pd->coords, pd->path, 0, 2, data);
      map_if_relative(pd->ox, pd->oy, relative, &data[0], &data[1]);
      pd->sx = data[0];
      pd->sy = data[1];
      pd->ox = data[0];
      pd->oy = data[1];
      pd->px = data[0];
      pd->py = data[1];
      *num_coords = 2;
      return VG_MOVE_TO_ABS;
      break;
   case VG_LINE_TO:
      data_at(&pd->coords, pd->path, 0, 2, data);
      map_if_relative(pd->ox, pd->oy, relative, &data[0], &data[1]);
      pd->ox = data[0];
      pd->oy = data[1];
      pd->px = data[0];
      pd->py = data[1];
      *num_coords = 2;
      return VG_LINE_TO_ABS;
      break;
   case VG_HLINE_TO:
      data_at(&pd->coords, pd->path, 0, 1, data);
      map_if_relative(pd->ox, pd->oy, relative, &data[0], 0);
      data[1] = pd->oy;
      pd->ox = data[0];
      pd->oy = data[1];
      pd->px = data[0];
      pd->py = data[1];
      *num_coords = 2;
      return VG_LINE_TO_ABS;
      break;
   case VG_VLINE_TO:
      data_at(&pd->coords, pd->path, 0, 1, data);
      map_if_relative(pd->ox, pd->oy, relative, 0, &data[0]);
      data[1] = data[0];
      data[0] = pd->ox;
      pd->ox = data[0];
      pd->oy = data[1];
      pd->px = data[0];
      pd->py = data[1];
      *num_coords = 2;
      return VG_LINE_TO_ABS;
      break;
   case VG_CUBIC_TO: {
      data_at(&pd->coords, pd->path, 0, 6, data);
      map_if_relative(pd->ox, pd->oy, relative, &data[0], &data[1]);
      map_if_relative(pd->ox, pd->oy, relative, &data[2], &data[3]);
      map_if_relative(pd->ox, pd->oy, relative, &data[4], &data[5]);
      pd->px = data[2];
      pd->py = data[3];
      pd->ox = data[4];
      pd->oy = data[5];
      *num_coords = 6;
      return VG_CUBIC_TO_ABS;
   }
      break;
   case VG_QUAD_TO: {
      VGfloat x0, y0, x1, y1, x2, y2, x3, y3;
      data_at(&pd->coords, pd->path, 0, 4, data);
      x0 = pd->ox;
      y0 = pd->oy;
      x1 = data[0];
      y1 = data[1];
      x3 = data[2];
      y3 = data[3];
      map_if_relative(pd->ox, pd->oy, relative, &x1, &y1);
      map_if_relative(pd->ox, pd->oy, relative, &x3, &y3);
      pd->px = x1;
      pd->py = y1;
      { /* form a cubic out of it */
         x2 = (x3 + 2*x1) / 3.f;
         y2 = (y3 + 2*y1) / 3.f;
         x1 = (x0 + 2*x1) / 3.f;
         y1 = (y0 + 2*y1) / 3.f;
      }
      pd->ox = x3;
      pd->oy = y3;
      data[0] = x1;
      data[1] = y1;
      data[2] = x2;
      data[3] = y2;
      data[4] = x3;
      data[5] = y3;
      *num_coords = 6;
      return VG_CUBIC_TO_ABS;
   }
      break;
   case VG_SQUAD_TO: {
      VGfloat x0, y0, x1, y1, x2, y2, x3, y3;
      data_at(&pd->coords, pd->path, 0, 2, data);
      x0 = pd->ox;
      y0 = pd->oy;
      x1 = 2 * pd->ox - pd->px;
      y1 = 2 * pd->oy - pd->py;
      x3 = data[0];
      y3 = data[1];
      map_if_relative(pd->ox, pd->oy, relative, &x3, &y3);
      pd->px = x1;
      pd->py = y1;
      { /* form a cubic out of it */
         x2 = (x3 + 2*x1) / 3.f;
         y2 = (y3 + 2*y1) / 3.f;
         x1 = (x0 + 2*x1) / 3.f;
         y1 = (y0 + 2*y1) / 3.f;
      }
      pd->ox = x3;
      pd->oy = y3;
      data[0] = x1;
      data[1] = y1;
      data[2] = x2;
      data[3] = y2;
      data[4] = x3;
      data[5] = y3;
      *num_coords = 6;
      return VG_CUBIC_TO_ABS;
   }
      break;
   case VG_SCUBIC_TO: {
      VGfloat x0, y0, x1, y1, x2, y2, x3, y3;
      data_at(&pd->coords, pd->path, 0, 4, data);
      x0 = pd->ox;
      y0 = pd->oy;
      x1 = 2*pd->ox-pd->px;
      y1 = 2*pd->oy-pd->py;
      x2 = data[0];
      y2 = data[1];
      x3 = data[2];
      y3 = data[3];
      map_if_relative(pd->ox, pd->oy, relative, &x2, &y2);
      map_if_relative(pd->ox, pd->oy, relative, &x3, &y3);
      pd->ox = x3;
      pd->oy = y3;
      pd->px = x2;
      pd->py = y2;
      data[0] = x1;
      data[1] = y1;
      data[2] = x2;
      data[3] = y2;
      data[4] = x3;
      data[5] = y3;
      *num_coords = 6;
      return VG_CUBIC_TO_ABS;
   }
      break;
   case VG_SCCWARC_TO:
   case VG_SCWARC_TO:
   case VG_LCCWARC_TO:
   case VG_LCWARC_TO: {
      data_at(&pd->coords, pd->path, 0, 5, data);
      map_if_relative(pd->ox, pd->oy, relative, &data[3], &data[4]);
      pd->ox = data[3];
      pd->oy = data[4];
      pd->px = data[3];
      pd->py = data[4];
      *num_coords = 5;
      return command | VG_ABSOLUTE;
   }
      break;
   default:
      abort();
      assert(!"Unknown segment!");
   }
}

static void linearly_interpolate(VGfloat *result,
                                 const VGfloat *start,
                                 const VGfloat *end,
                                 VGfloat amount,
                                 VGint number)
{
   VGint i;
   for (i = 0; i < number; ++i) {
      result[i] = start[i] + (end[i] - start[i]) * amount;
   }
}

VGboolean path_interpolate(struct path *dst,
                           struct path *start, struct path *end,
                           VGfloat amount)
{
   /* temporary path that we can discard if it will turn
    * out that start is not compatible with end */
   struct path *res_path = path_create(dst->datatype,
                                       1.0, 0.0,
                                       0, 0, dst->caps);
   VGint i;
   VGfloat start_coords[8];
   VGfloat end_coords[8];
   VGfloat results[8];
   VGubyte common_data[sizeof(VGfloat)*8];
   struct path_iter_data start_iter, end_iter;

   memset(&start_iter, 0, sizeof(struct path_iter_data));
   memset(&end_iter, 0, sizeof(struct path_iter_data));

   start_iter.path = start;
   start_iter.coords = start->control_points->data;
   end_iter.path = end;
   end_iter.coords = end->control_points->data;

   for (i = 0; i < start->num_segments; ++i) {
      VGubyte segment;
      VGubyte ssegment, esegment;
      VGint snum_coords, enum_coords;
      start_iter.segment = ((VGubyte*)(start->segments->data))[i];
      end_iter.segment = ((VGubyte*)(end->segments->data))[i];

      ssegment = normalize_coords(&start_iter, &snum_coords,
                                  start_coords);
      esegment = normalize_coords(&end_iter, &enum_coords,
                                  end_coords);

      if (is_segment_arc(ssegment)) {
         if (!is_segment_arc(esegment)) {
            path_destroy(res_path);
            return VG_FALSE;
         }
         if (amount > 0.5)
            segment = esegment;
         else
            segment = ssegment;
      } else if (is_segment_arc(esegment)) {
         path_destroy(res_path);
         return VG_FALSE;
      }
      else if (ssegment != esegment) {
         path_destroy(res_path);
         return VG_FALSE;
      }
      else
         segment = ssegment;

      linearly_interpolate(results, start_coords, end_coords,
                           amount, snum_coords);
      vg_float_to_datatype(dst->datatype, common_data, results, snum_coords);
      path_append_data(res_path, 1, &segment, common_data);
   }

   path_append_path(dst, res_path);
   path_destroy(res_path);

   dst->dirty = VG_TRUE;
   dst->dirty_stroke = VG_TRUE;

   return VG_TRUE;
}

void path_clear(struct path *p, VGbitfield capabilities)
{
   path_set_capabilities(p, capabilities);
   array_destroy(p->segments);
   array_destroy(p->control_points);
   p->segments = array_create(size_for_datatype(VG_PATH_DATATYPE_S_8));
   p->control_points = array_create(size_for_datatype(p->datatype));
   p->num_segments = 0;
   p->dirty = VG_TRUE;
   p->dirty_stroke = VG_TRUE;
}

struct path * path_create_stroke(struct path *p,
                                 struct matrix *matrix)
{
   VGint i;
   VGfloat sx, sy, px, py, ox, oy;
   VGfloat x0, y0, x1, y1, x2, y2, x3, y3;
   VGfloat data[8];
   void *coords = (VGfloat *)p->control_points->data;
   int dashed = (p->base.ctx->state.vg.stroke.dash_pattern_num ? 1 : 0);
   struct dash_stroker stroker;
   struct vg_state *vg_state = &p->base.ctx->state.vg;

   if (p->stroked.path)
   {
      /* ### compare the dash patterns to see if we can cache them.
       *     for now we simply always bail out if the path is dashed.
       */
      if (memcmp( &p->stroked.matrix,
                  matrix,
                  sizeof *matrix ) == 0 &&
          !dashed && !p->dirty_stroke &&
          floatsEqual(p->stroked.stroke_width, vg_state->stroke.line_width.f) &&
          floatsEqual(p->stroked.miter_limit, vg_state->stroke.miter_limit.f) &&
          p->stroked.cap_style == vg_state->stroke.cap_style &&
          p->stroked.join_style == vg_state->stroke.join_style)
      {
         return p->stroked.path;
      }
      else {
         path_destroy( p->stroked.path );
         p->stroked.path = NULL;
      }
   }


   sx = sy = px = py = ox = oy = 0.f;

   if (dashed)
      dash_stroker_init((struct stroker *)&stroker, vg_state);
   else
      stroker_init((struct stroker *)&stroker, vg_state);

   stroker_begin((struct stroker *)&stroker);

   for (i = 0; i < p->num_segments; ++i) {
      VGubyte segment = ((VGubyte*)(p->segments->data))[i];
      VGint command = SEGMENT_COMMAND(segment);
      VGboolean relative = SEGMENT_ABS_REL(segment);

      switch(command) {
      case VG_CLOSE_PATH: {
            VGfloat x0 = sx;
            VGfloat y0 = sy;
            matrix_map_point(matrix, x0, y0, &x0, &y0);
            stroker_line_to((struct stroker *)&stroker, x0, y0);
      }
         break;
      case VG_MOVE_TO:
         data_at(&coords, p, 0, 2, data);
         x0 = data[0];
         y0 = data[1];
         map_if_relative(ox, oy, relative, &x0, &y0);
         sx = x0;
         sy = y0;
         ox = x0;
         oy = y0;
         px = x0;
         py = y0;
         matrix_map_point(matrix, x0, y0, &x0, &y0);
         stroker_move_to((struct stroker *)&stroker, x0, y0);
         break;
      case VG_LINE_TO:
         data_at(&coords, p, 0, 2, data);
         x0 = data[0];
         y0 = data[1];
         map_if_relative(ox, oy, relative, &x0, &y0);
         ox = x0;
         oy = y0;
         px = x0;
         py = y0;
         matrix_map_point(matrix, x0, y0, &x0, &y0);
         stroker_line_to((struct stroker *)&stroker, x0, y0);
         break;
      case VG_HLINE_TO:
         data_at(&coords, p, 0, 1, data);
         x0 = data[0];
         y0 = oy;
         map_if_relative(ox, oy, relative, &x0, 0);
         ox = x0;
         px = x0;
         py = y0;
         matrix_map_point(matrix, x0, y0, &x0, &y0);
         stroker_line_to((struct stroker *)&stroker, x0, y0);
         break;
      case VG_VLINE_TO:
         data_at(&coords, p, 0, 1, data);
         x0 = ox;
         y0 = data[0];
         map_if_relative(ox, oy, relative, 0, &y0);
         oy = y0;
         px = x0;
         py = y0;
         matrix_map_point(matrix, x0, y0, &x0, &y0);
         stroker_line_to((struct stroker *)&stroker, x0, y0);
         break;
      case VG_CUBIC_TO: {
         data_at(&coords, p, 0, 6, data);
         x0 = ox;
         y0 = oy;
         x1 = data[0];
         y1 = data[1];
         x2 = data[2];
         y2 = data[3];
         x3 = data[4];
         y3 = data[5];
         map_if_relative(ox, oy, relative, &x1, &y1);
         map_if_relative(ox, oy, relative, &x2, &y2);
         map_if_relative(ox, oy, relative, &x3, &y3);
         if (floatsEqual(x1, ox) && floatsEqual(y1, oy) &&
             floatsEqual(x1, x2) && floatsEqual(y1, y2) &&
             floatsEqual(x2, x3) && floatsEqual(y2, y3)) {
            /*ignore the empty segment */
            continue;
         } else if (floatsEqual(x3, ox) && floatsEqual(y3, oy)) {
            /* if dup vertex, emit a line */
            ox = x3;
            oy = y3;
            matrix_map_point(matrix, x3, y3, &x3, &y3);
            stroker_line_to((struct stroker *)&stroker, x3, y3);
            continue;
         }
         ox = x3;
         oy = y3;
         px = x2;
         py = y2;
         assert(matrix_is_affine(matrix));
         matrix_map_point(matrix, x0, y0, &x0, &y0);
         matrix_map_point(matrix, x1, y1, &x1, &y1);
         matrix_map_point(matrix, x2, y2, &x2, &y2);
         matrix_map_point(matrix, x3, y3, &x3, &y3);
         stroker_curve_to((struct stroker *)&stroker, x1, y1, x2, y2, x3, y3);
      }
         break;
      case VG_QUAD_TO: {
         data_at(&coords, p, 0, 4, data);
         x0 = ox;
         y0 = oy;
         x1 = data[0];
         y1 = data[1];
         x3 = data[2];
         y3 = data[3];
         map_if_relative(ox, oy, relative, &x1, &y1);
         map_if_relative(ox, oy, relative, &x3, &y3);
         px = x1;
         py = y1;
         { /* form a cubic out of it */
            x2 = (x3 + 2*x1) / 3.f;
            y2 = (y3 + 2*y1) / 3.f;
            x1 = (x0 + 2*x1) / 3.f;
            y1 = (y0 + 2*y1) / 3.f;
         }
         if (floatsEqual(x1, ox) && floatsEqual(y1, oy) &&
             floatsEqual(x1, x2) && floatsEqual(y1, y2) &&
             floatsEqual(x2, x3) && floatsEqual(y2, y3)) {
            /*ignore the empty segment */
            continue;
         } else if (floatsEqual(x3, ox) && floatsEqual(y3, oy)) {
            /* if dup vertex, emit a line */
            ox = x3;
            oy = y3;
            matrix_map_point(matrix, x3, y3, &x3, &y3);
            stroker_line_to((struct stroker *)&stroker, x3, y3);
            continue;
         }
         ox = x3;
         oy = y3;
         assert(matrix_is_affine(matrix));
         matrix_map_point(matrix, x0, y0, &x0, &y0);
         matrix_map_point(matrix, x1, y1, &x1, &y1);
         matrix_map_point(matrix, x2, y2, &x2, &y2);
         matrix_map_point(matrix, x3, y3, &x3, &y3);
         stroker_curve_to((struct stroker *)&stroker, x1, y1, x2, y2, x3, y3);
      }
         break;
      case VG_SQUAD_TO: {
         data_at(&coords, p, 0, 2, data);
         x0 = ox;
         y0 = oy;
         x1 = 2*ox-px;
         y1 = 2*oy-py;
         x3 = data[0];
         y3 = data[1];
         map_if_relative(ox, oy, relative, &x3, &y3);
         px = x1;
         py = y1;
         { /* form a cubic out of it */
            x2 = (x3 + 2*x1) / 3.f;
            y2 = (y3 + 2*y1) / 3.f;
            x1 = (x0 + 2*x1) / 3.f;
            y1 = (y0 + 2*y1) / 3.f;
         }
         if (floatsEqual(x1, ox) && floatsEqual(y1, oy) &&
             floatsEqual(x1, x2) && floatsEqual(y1, y2) &&
             floatsEqual(x2, x3) && floatsEqual(y2, y3)) {
            /*ignore the empty segment */
            continue;
         } else if (floatsEqual(x3, ox) && floatsEqual(y3, oy)) {
            /* if dup vertex, emit a line */
            ox = x3;
            oy = y3;
            matrix_map_point(matrix, x3, y3, &x3, &y3);
            stroker_line_to((struct stroker *)&stroker, x3, y3);
            continue;
         }
         ox = x3;
         oy = y3;
         assert(matrix_is_affine(matrix));
         matrix_map_point(matrix, x0, y0, &x0, &y0);
         matrix_map_point(matrix, x1, y1, &x1, &y1);
         matrix_map_point(matrix, x2, y2, &x2, &y2);
         matrix_map_point(matrix, x3, y3, &x3, &y3);
         stroker_curve_to((struct stroker *)&stroker, x1, y1, x2, y2, x3, y3);
      }
         break;
      case VG_SCUBIC_TO: {
         data_at(&coords, p, 0, 4, data);
         x0 = ox;
         y0 = oy;
         x1 = 2*ox-px;
         y1 = 2*oy-py;
         x2 = data[0];
         y2 = data[1];
         x3 = data[2];
         y3 = data[3];
         map_if_relative(ox, oy, relative, &x2, &y2);
         map_if_relative(ox, oy, relative, &x3, &y3);
         if (floatsEqual(x1, ox) && floatsEqual(y1, oy) &&
             floatsEqual(x1, x2) && floatsEqual(y1, y2) &&
             floatsEqual(x2, x3) && floatsEqual(y2, y3)) {
            /*ignore the empty segment */
            continue;
         } else if (floatsEqual(x3, ox) && floatsEqual(y3, oy)) {
            /* if dup vertex, emit a line */
            ox = x3;
            oy = y3;
            matrix_map_point(matrix, x3, y3, &x3, &y3);
            stroker_line_to((struct stroker *)&stroker, x3, y3);
            continue;
         }
         ox = x3;
         oy = y3;
         px = x2;
         py = y2;
         assert(matrix_is_affine(matrix));
         matrix_map_point(matrix, x0, y0, &x0, &y0);
         matrix_map_point(matrix, x1, y1, &x1, &y1);
         matrix_map_point(matrix, x2, y2, &x2, &y2);
         matrix_map_point(matrix, x3, y3, &x3, &y3);
         stroker_curve_to((struct stroker *)&stroker, x1, y1, x2, y2, x3, y3);
      }
         break;
      case VG_SCCWARC_TO:
      case VG_SCWARC_TO:
      case VG_LCCWARC_TO:
      case VG_LCWARC_TO: {
         VGfloat rh, rv, rot;
         struct arc arc;

         data_at(&coords, p, 0, 5, data);
         x0  = ox;
         y0  = oy;
         rh  = data[0];
         rv  = data[1];
         rot = data[2];
         x1  = data[3];
         y1  = data[4];
         map_if_relative(ox, oy, relative, &x1, &y1);
         if (floatsEqual(x1, ox) && floatsEqual(y1, oy)) {
            /* if dup vertex, emit a line */
            ox = x1;
            oy = y1;
            matrix_map_point(matrix, x1, y1, &x1, &y1);
            stroker_line_to((struct stroker *)&stroker, x1, y1);
            continue;
         }
         arc_init(&arc, command, x0, y0, x1, y1,
                  rh, rv, rot);
         arc_stroke_cb(&arc, (struct stroker *)&stroker,
                       matrix);
         ox = x1;
         oy = y1;
         px = x1;
         py = y1;
      }
         break;
      default:
         abort();
         assert(!"Unknown segment!");
      }
   }

   stroker_end((struct stroker *)&stroker);

   if (dashed)
      dash_stroker_cleanup((struct dash_stroker *)&stroker);
   else
      stroker_cleanup((struct stroker *)&stroker);

   p->stroked.path = stroker.base.path;
   p->stroked.matrix = *matrix;
   p->dirty_stroke = VG_FALSE;
   p->stroked.stroke_width = vg_state->stroke.line_width.f;
   p->stroked.miter_limit = vg_state->stroke.miter_limit.f;
   p->stroked.cap_style = vg_state->stroke.cap_style;
   p->stroked.join_style = vg_state->stroke.join_style;

   return stroker.base.path;
}

void path_render(struct path *p, VGbitfield paintModes,
                 struct matrix *mat)
{
   struct vg_context *ctx = vg_current_context();
   struct matrix paint_matrix;

   vg_validate_state(ctx);

   shader_set_drawing_image(ctx->shader, VG_FALSE);
   shader_set_image(ctx->shader, 0);
#if 0
   fprintf(stderr, "Matrix(11=%f 12=%f 13=%f 21=%f 22=%f 23=%f 31=%f 32=%f 33=%f)\n",
           mat->m[0], mat->m[1], mat->m[2],
           mat->m[3], mat->m[4], mat->m[5],
           mat->m[6], mat->m[7], mat->m[8]);
#endif
   if ((paintModes & VG_FILL_PATH) &&
       vg_get_paint_matrix(ctx,
                           &ctx->state.vg.fill_paint_to_user_matrix,
                           mat,
                           &paint_matrix)) {
      /* First the fill */
      shader_set_surface_matrix(ctx->shader, mat);
      shader_set_paint(ctx->shader, ctx->state.vg.fill_paint);
      shader_set_paint_matrix(ctx->shader, &paint_matrix);
      shader_bind(ctx->shader);
      path_fill(p);
   }

   if ((paintModes & VG_STROKE_PATH) &&
       vg_get_paint_matrix(ctx,
                           &ctx->state.vg.stroke_paint_to_user_matrix,
                           mat,
                           &paint_matrix)) {
      /* 8.7.5: "line width less than or equal to 0 prevents stroking from
       *  taking place."*/
      if (ctx->state.vg.stroke.line_width.f <= 0)
         return;
      shader_set_surface_matrix(ctx->shader, mat);
      shader_set_paint(ctx->shader, ctx->state.vg.stroke_paint);
      shader_set_paint_matrix(ctx->shader, &paint_matrix);
      shader_bind(ctx->shader);
      path_stroke(p);
   }
}

void path_fill(struct path *p)
{
   struct vg_context *ctx = vg_current_context();
   struct matrix identity;

   matrix_load_identity(&identity);

   {
      struct polygon_array *polygon_array = path_get_fill_polygons(p, &identity);
      struct array *polys = polygon_array->array;

      if (!polygon_array || !polys || !polys->num_elements) {
         return;
      }
      polygon_array_fill(polygon_array, ctx);
   }
}

void path_stroke(struct path *p)
{
   struct vg_context *ctx = vg_current_context();
   VGFillRule old_fill = ctx->state.vg.fill_rule;
   struct matrix identity;
   struct path *stroke;

   matrix_load_identity(&identity);
   stroke = path_create_stroke(p, &identity);
   if (stroke && !path_is_empty(stroke)) {
      ctx->state.vg.fill_rule = VG_NON_ZERO;

      path_fill(stroke);

      ctx->state.vg.fill_rule = old_fill;
   }
}

void path_move_to(struct path *p, float x, float y)
{
   VGubyte segment = VG_MOVE_TO_ABS;
   VGubyte common_data[sizeof(VGfloat) * 2];
   VGfloat data[2] = {x, y};

   vg_float_to_datatype(p->datatype, common_data, data, 2);
   path_append_data(p, 1, &segment, common_data);
}

void path_line_to(struct path *p, float x, float y)
{
   VGubyte segment = VG_LINE_TO_ABS;
   VGubyte common_data[sizeof(VGfloat) * 2];
   VGfloat data[2] = {x, y};

   vg_float_to_datatype(p->datatype, common_data, data, 2);

   path_append_data(p, 1, &segment, common_data);
}

void path_cubic_to(struct path *p, float px1, float py1,
                   float px2, float py2,
                   float x, float y)
{
   VGubyte segment = VG_CUBIC_TO_ABS;
   VGubyte common_data[sizeof(VGfloat) * 6];
   VGfloat data[6];

   data[0] = px1; data[1] = py1;
   data[2] = px2; data[3] = py2;
   data[4] = x;   data[5] = y;

   vg_float_to_datatype(p->datatype, common_data, data, 6);

   path_append_data(p, 1, &segment, common_data);
}

static INLINE void line_bounds(VGfloat *line /*x1,y1,x2,y2*/,
                               VGfloat *bounds)
{
   bounds[0] = MIN2(line[0], line[2]);
   bounds[1] = MIN2(line[1], line[3]);
   bounds[2] = MAX2(line[0], line[2]) - bounds[0];
   bounds[3] = MAX2(line[1], line[3]) - bounds[1];
}

static INLINE void unite_bounds(VGfloat *bounds,
                                VGfloat *el)
{
   VGfloat cx1, cy1, cx2, cy2;
   VGfloat nx1, ny1, nx2, ny2;

   cx1 = bounds[0];
   cy1 = bounds[1];
   cx2 = bounds[0] + bounds[2];
   cy2 = bounds[1] + bounds[3];

   nx1 = el[0];
   ny1 = el[1];
   nx2 = el[0] + el[2];
   ny2 = el[1] + el[3];

   bounds[0] = MIN2(cx1, nx1);
   bounds[1] = MIN2(cy1, ny1);
   bounds[2] = MAX2(cx2, nx2) - bounds[0];
   bounds[3] = MAX2(cy2, ny2) - bounds[1];
}

static INLINE void set_bounds(VGfloat *bounds,
                              VGfloat *element_bounds,
                              VGboolean *initialized)
{
   if (!(*initialized)) {
      memcpy(bounds, element_bounds, 4 * sizeof(VGfloat));
      *initialized = VG_TRUE;
   } else
      unite_bounds(bounds, element_bounds);
}

void path_bounding_rect(struct path *p, float *x, float *y,
                        float *w, float *h)
{
   VGint i;
   VGfloat coords[8];
   struct path_iter_data iter;
   VGint num_coords;
   VGfloat bounds[4];
   VGfloat element_bounds[4];
   VGfloat ox, oy;
   VGboolean bounds_inited = VG_FALSE;

   memset(&iter, 0, sizeof(struct path_iter_data));
   memset(&bounds, 0, sizeof(bounds));

   if (!p->num_segments) {
      bounds[2] = -1;
      bounds[3] = -1;
   }


   iter.path = p;
   iter.coords = p->control_points->data;

   for (i = 0; i < p->num_segments; ++i) {
      VGubyte segment;
      iter.segment = ((VGubyte*)(p->segments->data))[i];

      ox = iter.ox;
      oy = iter.oy;

      segment = normalize_coords(&iter, &num_coords, coords);

      switch(segment) {
      case VG_CLOSE_PATH:
      case VG_MOVE_TO_ABS:
         break;
      case VG_LINE_TO_ABS: {
         VGfloat line[4] = {ox, oy, coords[0], coords[1]};
         line_bounds(line, element_bounds);
         set_bounds(bounds, element_bounds, &bounds_inited);
      }
         break;
      case VG_CUBIC_TO_ABS: {
         struct bezier bezier;
         bezier_init(&bezier, ox, oy,
                     coords[0], coords[1],
                     coords[2], coords[3],
                     coords[4], coords[5]);
         bezier_exact_bounds(&bezier, element_bounds);
         set_bounds(bounds, element_bounds, &bounds_inited);
      }
         break;
      case VG_SCCWARC_TO:
      case VG_SCWARC_TO:
      case VG_LCCWARC_TO:
      case VG_LCWARC_TO: {
         struct arc arc;
         struct matrix identity;
         struct path *path = path_create(VG_PATH_DATATYPE_F,
                                         1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);

         matrix_load_identity(&identity);
         arc_init(&arc, segment,
                  ox, oy, coords[3], coords[4],
                  coords[0], coords[1], coords[2]);

         arc_to_path(&arc, path, &identity);

         path_bounding_rect(path, element_bounds + 0, element_bounds + 1,
                            element_bounds + 2, element_bounds + 3);
         set_bounds(bounds, element_bounds, &bounds_inited);
      }
         break;
      default:
         assert(0);
      }
   }

   *x = bounds[0];
   *y = bounds[1];
   *w = bounds[2];
   *h = bounds[3];
}

float path_length(struct path *p, int start_segment, int num_segments)
{
   VGint i;
   VGfloat coords[8];
   struct path_iter_data iter;
   VGint num_coords;
   VGfloat length = 0;
   VGfloat ox, oy;
   VGboolean in_range = VG_FALSE;

   memset(&iter, 0, sizeof(struct path_iter_data));

   iter.path = p;
   iter.coords = p->control_points->data;

   for (i = 0; i < (start_segment + num_segments); ++i) {
      VGubyte segment;

      iter.segment = ((VGubyte*)(p->segments->data))[i];

      ox = iter.ox;
      oy = iter.oy;

      segment = normalize_coords(&iter, &num_coords, coords);

      in_range = (i >= start_segment) && i <= (start_segment + num_segments);
      if (!in_range)
         continue;

      switch(segment) {
      case VG_MOVE_TO_ABS:
         break;
      case VG_CLOSE_PATH: {
         VGfloat line[4] = {ox, oy, iter.sx, iter.sy};
         length += line_lengthv(line);
      }
         break;
      case VG_LINE_TO_ABS: {
         VGfloat line[4] = {ox, oy, coords[0], coords[1]};
         length += line_lengthv(line);
      }
         break;
      case VG_CUBIC_TO_ABS: {
         struct bezier bezier;
         bezier_init(&bezier, ox, oy,
                     coords[0], coords[1],
                     coords[2], coords[3],
                     coords[4], coords[5]);
         length += bezier_length(&bezier, BEZIER_DEFAULT_ERROR);
      }
         break;
      case VG_SCCWARC_TO:
      case VG_SCWARC_TO:
      case VG_LCCWARC_TO:
      case VG_LCWARC_TO: {
         struct arc arc;
         struct matrix identity;
         struct path *path = path_create(VG_PATH_DATATYPE_F,
                                         1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);

         matrix_load_identity(&identity);
         arc_init(&arc, segment,
                  ox, oy, coords[3], coords[4],
                  coords[0], coords[1], coords[2]);

         arc_to_path(&arc, path, &identity);

         length += path_length(path, 0, path_num_segments(path));
      }
         break;
      default:
         assert(0);
      }
   }

   return length;
}

static INLINE VGboolean point_on_current_segment(VGfloat distance,
                                                 VGfloat length,
                                                 VGfloat segment_length)
{
   return
      (((floatIsZero(distance) || distance < 0) && floatIsZero(length)) ||
       ((distance > length || floatsEqual(distance, length)) &&
        (floatsEqual(distance, length + segment_length) ||
         distance < (length + segment_length))));
}

static VGboolean path_point_segment(struct path_iter_data iter,
                                    struct path_iter_data prev_iter,
                                    VGfloat coords[8],
                                    VGfloat distance,
                                    VGfloat length, VGfloat *current_length,
                                    VGfloat *point, VGfloat *normal)
{
   switch (iter.segment) {
   case VG_MOVE_TO_ABS:
      break;
   case VG_CLOSE_PATH: {
      VGfloat line[4] = {prev_iter.ox, prev_iter.oy, iter.sx, iter.sy};
      VGboolean on_current_segment = VG_FALSE;
      *current_length = line_lengthv(line);
      on_current_segment = point_on_current_segment(distance,
                                                    length,
                                                    *current_length);
      if (on_current_segment) {
         VGfloat at = (distance - length) / line_lengthv(line);
         line_normal_vector(line, normal);
         line_point_at(line, at, point);
         return VG_TRUE;
      }
   }
      break;
   case VG_LINE_TO_ABS: {
      VGfloat line[4] = {prev_iter.ox, prev_iter.oy, coords[0], coords[1]};
      VGboolean on_current_segment = VG_FALSE;
      *current_length = line_lengthv(line);
      on_current_segment = point_on_current_segment(distance,
                                                    length,
                                                    *current_length);
      if (on_current_segment) {
         VGfloat at = (distance - length) / line_lengthv(line);
         line_normal_vector(line, normal);
         line_point_at(line, at, point);
         return VG_TRUE;
      }
   }
      break;
   case VG_CUBIC_TO_ABS: {
      struct bezier bezier;
      bezier_init(&bezier, prev_iter.ox, prev_iter.oy,
                  coords[0], coords[1],
                  coords[2], coords[3],
                  coords[4], coords[5]);
      *current_length = bezier_length(&bezier, BEZIER_DEFAULT_ERROR);
      if (point_on_current_segment(distance, length, *current_length)) {
         bezier_point_at_length(&bezier, distance - length,
                                point, normal);
         return VG_TRUE;
      }
   }
      break;
   case VG_SCCWARC_TO:
   case VG_SCWARC_TO:
   case VG_LCCWARC_TO:
   case VG_LCWARC_TO: {
      struct arc arc;
      struct matrix identity;
      struct path *path = path_create(VG_PATH_DATATYPE_F,
                                      1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);

      matrix_load_identity(&identity);
      arc_init(&arc, iter.segment,
               prev_iter.ox, prev_iter.oy, coords[3], coords[4],
               coords[0], coords[1], coords[2]);

      arc_to_path(&arc, path, &identity);

      *current_length = path_length(path, 0, path_num_segments(path));
      if (point_on_current_segment(distance, length, *current_length)) {
         path_point(path, 0, path_num_segments(path),
                    distance - length, point, normal);
         return VG_TRUE;
      }
   }
      break;
   default:
      assert(0);
   }
   return VG_FALSE;
}

void path_point(struct path *p, VGint start_segment, VGint num_segments,
                VGfloat distance, VGfloat *point, VGfloat *normal)
{
   VGint i;
   VGfloat coords[8];
   struct path_iter_data iter, prev_iter;
   VGint num_coords;
   VGfloat length = 0;
   VGfloat current_length = 0;

   memset(&iter, 0, sizeof(struct path_iter_data));
   memset(&prev_iter, 0, sizeof(struct path_iter_data));

   point[0] = 0;
   point[1] = 0;

   normal[0] = 0;
   normal[1] = -1;

   iter.path = p;
   iter.coords = p->control_points->data;
   if (distance < 0)
      distance = 0;

   for (i = 0; i < (start_segment + num_segments); ++i) {
      VGboolean outside_range = (i < start_segment ||
                                 i >= (start_segment + num_segments));

      prev_iter = iter;

      iter.segment = ((VGubyte*)(p->segments->data))[i];
      iter.segment = normalize_coords(&iter, &num_coords, coords);

      if (outside_range)
         continue;

      if (path_point_segment(iter, prev_iter, coords,
                             distance, length, &current_length,
                             point, normal))
         return;

      length += current_length;
   }

   /*
    *OpenVG 1.0 - 8.6.11 vgPointAlongPath
    *
    * If distance is greater than or equal to the path length
    *(i.e., the value returned by vgPathLength when called with the same
    *startSegment and numSegments parameters), the visual ending point of
    *the path is used.
    */
   {
      switch (iter.segment) {
      case VG_MOVE_TO_ABS:
         break;
      case VG_CLOSE_PATH: {
         VGfloat line[4] = {prev_iter.ox, prev_iter.oy, iter.sx, iter.sy};
         line_normal_vector(line, normal);
         line_point_at(line, 1.f, point);
      }
         break;
      case VG_LINE_TO_ABS: {
         VGfloat line[4] = {prev_iter.ox, prev_iter.oy, coords[0], coords[1]};
         line_normal_vector(line, normal);
         line_point_at(line, 1.f, point);
      }
         break;
      case VG_CUBIC_TO_ABS: {
         struct bezier bezier;
         bezier_init(&bezier, prev_iter.ox, prev_iter.oy,
                     coords[0], coords[1],
                     coords[2], coords[3],
                     coords[4], coords[5]);
         bezier_point_at_t(&bezier, 1.f, point, normal);
      }
         break;
      case VG_SCCWARC_TO:
      case VG_SCWARC_TO:
      case VG_LCCWARC_TO:
      case VG_LCWARC_TO: {
         struct arc arc;
         struct matrix identity;
         struct path *path = path_create(VG_PATH_DATATYPE_F,
                                         1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);

         matrix_load_identity(&identity);
         arc_init(&arc, iter.segment,
                  prev_iter.ox, prev_iter.oy, coords[3], coords[4],
                  coords[0], coords[1], coords[2]);

         arc_to_path(&arc, path, &identity);

         path_point(path, 0, path_num_segments(path),
                    /* to make sure we're bigger than len * 2 it */
                    2 * path_length(path, 0, path_num_segments(path)),
                    point, normal);
      }
         break;
      default:
         assert(0);
      }
   }
}

VGboolean path_is_empty(struct path *p)
{
   return p->segments->num_elements == 0;
}
