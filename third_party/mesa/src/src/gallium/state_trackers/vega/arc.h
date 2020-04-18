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

#ifndef ARC_H
#define ARC_H

#include "VG/openvg.h"

struct polygon;
struct matrix;
struct stroker;
struct path;

struct arc {
   VGPathSegment type;

   VGfloat cx, cy;

   VGfloat a, b;

   VGfloat theta;
   VGfloat cos_theta, sin_theta;

   VGfloat eta1;
   VGfloat eta2;

   VGfloat x1, y1, x2, y2;

   VGboolean is_valid;
};

void arc_init(struct arc *arc,
              VGPathSegment type,
              VGfloat x1, VGfloat y1,
              VGfloat x2, VGfloat y2,
              VGfloat rh, VGfloat rv,
              VGfloat rot);

void arc_add_to_polygon(struct arc *arc,
                        struct polygon *poly,
                        struct matrix *matrix);


void arc_to_path(struct arc *arc,
                 struct path *p,
                 struct matrix *matrix);

void arc_stroke_cb(struct arc *arc,
                   struct stroker *stroke,
                   struct matrix *matrix);

void arc_stroker_emit(struct arc *arc,
                      struct stroker *stroke,
                      struct matrix *matrix);


#endif
