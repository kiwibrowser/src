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

#ifndef STROKER_H
#define STROKER_H

#include "VG/openvg.h"
#include "api_consts.h"

struct path;
struct vg_state;
struct array;

struct stroker {
   void (*begin)(struct stroker *stroker);
   void (*process_subpath)(struct stroker *stroker);
   void (*end)(struct stroker *stroker);

   struct array *segments;
   struct array *control_points;
   struct path *path;

   VGfloat back1_x, back1_y;
   VGfloat back2_x, back2_y;

   VGfloat     stroke_width;
   VGfloat     miter_limit;
   VGCapStyle  cap_style;
   VGJoinStyle join_style;

   VGPathCommand last_cmd;
};

struct dash_stroker {
   struct stroker base;

   struct stroker stroker;

   VGfloat dash_pattern[VEGA_MAX_DASH_COUNT];
   VGint dash_pattern_num;
   VGfloat dash_phase;
   VGboolean dash_phase_reset;
};

void stroker_init(struct stroker *stroker,
                  struct vg_state *state);
void dash_stroker_init(struct stroker *stroker,
                       struct vg_state *state);
void dash_stroker_cleanup(struct dash_stroker *stroker);
void stroker_cleanup(struct stroker *stroker);

void stroker_begin(struct stroker *stroker);
void stroker_move_to(struct stroker *stroker, VGfloat x, VGfloat y);
void stroker_line_to(struct stroker *stroker, VGfloat x, VGfloat y);
void stroker_curve_to(struct stroker *stroker, VGfloat px1, VGfloat py1,
                      VGfloat px2, VGfloat py2,
                      VGfloat x, VGfloat y);
void stroker_end(struct stroker *stroker);

void stroker_emit_move_to(struct stroker *stroker, VGfloat x, VGfloat y);
void stroker_emit_line_to(struct stroker *stroker, VGfloat x, VGfloat y);
void stroker_emit_curve_to(struct stroker *stroker, VGfloat px1, VGfloat py1,
                           VGfloat px2, VGfloat py2,
                           VGfloat x, VGfloat y);

#endif
