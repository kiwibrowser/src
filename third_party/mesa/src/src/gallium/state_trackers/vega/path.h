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

#ifndef _PATH_H
#define _PATH_H

#include "VG/openvg.h"

struct path;
struct polygon;
struct matrix;

enum fill_rule {
   ODD_EVEN_FILL,
   WINDING_FILL
};


struct path_for_each_data {
   VGubyte segment;
   /* all coords are absolute, even if segment is relative */
   const VGfloat *coords;
   VGfloat sx, sy, ox, oy, px, py;
   void *user_data;
};

typedef VGboolean (*path_for_each_cb)(struct path *p,
                                      struct path_for_each_data *data);


struct path *path_create(VGPathDatatype dt, VGfloat scale, VGfloat bias,
                         VGint segmentCapacityHint,
                         VGint coordCapacityHint,
                         VGbitfield capabilities);
void path_destroy(struct path *p);

VGbitfield path_capabilities(struct path *p);
void path_set_capabilities(struct path *p, VGbitfield bf);

void path_append_data(struct path *p,
                      VGint numSegments,
                      const VGubyte * pathSegments,
                      const void * pathData);

void path_append_path(struct path *dst,
                      struct path *src);

VGint path_num_segments(struct path *p);

void path_bounding_rect(struct path *p, float *x, float *y,
                        float *w, float *h);
float path_length(struct path *p, int start_segment, int num_segments);

void path_set_fill_rule(enum fill_rule fill);
enum fill_rule path_fill_rule(enum fill_rule fill);

VGboolean path_is_empty(struct path *p);

VGbyte path_datatype_size(struct path *p);

VGPathDatatype path_datatype(struct path *p);
VGfloat path_scale(struct path *p);
VGfloat path_bias(struct path *p);
VGint path_num_coords(struct path *p);

void path_modify_coords(struct path *p,
                        VGint startIndex,
                        VGint numSegments,
                        const void * pathData);

struct path *path_create_stroke(struct path *p,
                                struct matrix *m);

void path_for_each_segment(struct path *path,
                           path_for_each_cb cb,
                           void *user_data);

void path_transform(struct path *dst, struct path *src);
VGboolean path_interpolate(struct path *dst,
                           struct path *start, struct path *end,
                           VGfloat amount);

void path_clear(struct path *p, VGbitfield capabilities);
void path_render(struct path *p, VGbitfield paintModes, struct matrix *mat);
void path_fill(struct path *p);
void path_stroke(struct path *p);

void path_move_to(struct path *p, float x, float y);
void path_line_to(struct path *p, float x, float y);
void path_cubic_to(struct path *p, float px1, float py1,
                   float px2, float py2,
                   float x, float y);

void path_point(struct path *p, VGint startSegment, VGint numSegments,
                VGfloat distance, VGfloat *point, VGfloat *normal);



void vg_float_to_datatype(VGPathDatatype datatype,
                          VGubyte *common_data,
                          const VGfloat *data,
                          VGint num_coords);
#endif
