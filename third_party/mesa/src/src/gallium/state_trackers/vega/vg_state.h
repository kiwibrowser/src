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

#ifndef VG_STATE_H
#define VG_STATE_H

#include "VG/openvg.h"

#include "api_consts.h"
#include "matrix.h"

struct vg_value
{
   VGfloat f;
   VGint   i;
};

struct vg_state {
   /* Mode settings */
   VGMatrixMode matrix_mode;
   VGFillRule fill_rule;
   VGImageQuality image_quality;
   VGRenderingQuality rendering_quality;
   VGBlendMode blend_mode;
   VGImageMode image_mode;

   /* Scissoring rectangles */
   struct vg_value  scissor_rects[32*4];
   VGint  scissor_rects_num;

   /* Color Transformation */
   VGboolean color_transform;
   VGfloat color_transform_values[8];

   /* Stroke parameters */
   struct {
      struct vg_value line_width;
      VGCapStyle cap_style;
      VGJoinStyle join_style;
      struct vg_value miter_limit;
      struct vg_value dash_pattern[VEGA_MAX_DASH_COUNT];
      VGint   dash_pattern_num;
      struct vg_value dash_phase;
      VGboolean dash_phase_reset;
   } stroke;

   /* Edge fill color for VG_TILE_FILL tiling mode */
   VGfloat tile_fill_color[4];
   VGint tile_fill_colori[4];

   /* Color for vgClear */
   VGfloat clear_color[4];
   VGint clear_colori[4];

   /* Glyph origin */
   struct vg_value glyph_origin[2];

   /* Enable/disable alpha masking and scissoring */
   VGboolean masking;
   VGboolean scissoring;

   /* Pixel layout information */
   VGPixelLayout pixel_layout;
   VGPixelLayout screen_layout;

   /* Source format selection for image filters */
   VGboolean filter_format_linear;
   VGboolean filter_format_premultiplied;

   /* Destination write enable mask for image filters */
   VGbitfield filter_channel_mask;

   struct matrix path_user_to_surface_matrix;
   struct matrix image_user_to_surface_matrix;
   struct matrix fill_paint_to_user_matrix;
   struct matrix stroke_paint_to_user_matrix;
   struct matrix glyph_user_to_surface_matrix;

   struct vg_paint *stroke_paint;
   struct vg_paint *fill_paint;
};

void vg_init_state(struct vg_state *state);
struct matrix * vg_state_matrix(struct vg_state *state);

#endif
