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

#ifndef BEZIER_H
#define BEZIER_H

struct polygon;
struct matrix;

struct bezier {
    float x1, y1;
    float x2, y2;
    float x3, y3;
    float x4, y4;
};


#define BEZIER_DEFAULT_ERROR 0.01

/* kappa as being l of a circle with r = 1, we can emulate any
 * circle of radius r by using the formula
 * l = r . kappa
 * More at:
 * http://www.whizkidtech.redprince.net/bezier/circle/ */
#define KAPPA 0.5522847498

void bezier_init(struct bezier *bez,
                 float x1, float y1,
                 float x2, float y2,
                 float x3, float y3,
                 float x4, float y4);

struct polygon *bezier_to_polygon(struct bezier *bez);
void bezier_add_to_polygon(const struct bezier *bez,
                           struct polygon *poly);
float bezier_length(struct bezier *bez, float error);
void bezier_transform(struct bezier *bez,
                      struct matrix *mat);

int bezier_translate_by_normal(struct bezier *b,
                               struct bezier *curves,
                               int max_curves,
                               float normal_len,
                               float threshold);
void bezier_bounds(const struct bezier *bez,
                   float *bounds/*x/y/width/height*/);
void bezier_exact_bounds(const struct bezier *bez,
                         float *bounds/*x/y/width/height*/);

void bezier_start_tangent(const struct bezier *bez,
                          float *tangent);

void bezier_point_at_length(struct bezier *bez, float length,
                            float *point, float *normal);
void bezier_point_at_t(struct bezier *bez, float t,
                       float *point, float *normal);

#endif
