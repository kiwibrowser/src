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

#ifndef POLYGON_H
#define POLYGON_H

#include "VG/openvg.h"

struct polygon;
struct vg_context;
struct vg_paint;
struct array;

struct polygon *polygon_create(int size);
struct polygon *polygon_create_from_data(float *data, int size);
void            polygon_destroy(struct polygon *poly);

void            polygon_resize(struct polygon *poly, int new_size);
int             polygon_size(struct polygon *poly);

int             polygon_vertex_count(struct polygon *poly);
float *         polygon_data(struct polygon *poly);

void            polygon_vertex_append(struct polygon *p,
                                      float x, float y);
void            polygon_append_polygon(struct polygon *dst,
                                       struct polygon *src);
void            polygon_set_vertex(struct polygon *p, int idx,
                                   float x, float y);
void            polygon_vertex(struct polygon *p, int idx,
                               float *vertex);

void            polygon_bounding_rect(struct polygon *p,
                                      float *rect);
int             polygon_contains_point(struct polygon *p,
                                       float x, float y);

VGboolean       polygon_is_closed(struct polygon *p);

void polygon_fill(struct polygon *p, struct vg_context *pipe);

/* TODO: make a file/module around this struct
 */
struct polygon_array {
   struct array *array;
   VGfloat min_x, max_x;
   VGfloat min_y, max_y;
};

void polygon_array_fill(struct polygon_array *polyarray, struct vg_context *ctx);

#endif
