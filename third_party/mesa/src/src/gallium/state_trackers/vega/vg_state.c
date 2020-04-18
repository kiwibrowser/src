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

#include "vg_state.h"

#include <string.h>

void vg_init_state(struct vg_state *state)
{
   state->matrix_mode = VG_MATRIX_PATH_USER_TO_SURFACE;
   state->fill_rule = VG_EVEN_ODD;
   state->image_quality = VG_IMAGE_QUALITY_FASTER;
   state->rendering_quality = VG_RENDERING_QUALITY_BETTER;
   state->blend_mode = VG_BLEND_SRC_OVER;
   state->image_mode = VG_DRAW_IMAGE_NORMAL;

   memset(state->scissor_rects, 0, sizeof(state->scissor_rects));
   state->scissor_rects_num = 0;

   state->color_transform = VG_FALSE;
   state->color_transform_values[0] = 1.0f;
   state->color_transform_values[1] = 1.0f;
   state->color_transform_values[2] = 1.0f;
   state->color_transform_values[3] = 1.0f;
   state->color_transform_values[4] = 0.0f;
   state->color_transform_values[5] = 0.0f;
   state->color_transform_values[6] = 0.0f;
   state->color_transform_values[7] = 0.0f;

   /* Stroke parameters */
   state->stroke.line_width.f       = 1.0f;
   state->stroke.line_width.i       = 1;
   state->stroke.cap_style        = VG_CAP_BUTT;
   state->stroke.join_style       = VG_JOIN_MITER;
   state->stroke.miter_limit.f      = 4.0f;
   state->stroke.miter_limit.i      = 4;
   state->stroke.dash_pattern_num = 0;
   state->stroke.dash_phase.f       = 0.0f;
   state->stroke.dash_phase.i       = 0;
   state->stroke.dash_phase_reset = VG_FALSE;

   /* Edge fill color for VG_TILE_FILL tiling mode */
   state->tile_fill_color[0] = 0.0f;
   state->tile_fill_color[1] = 0.0f;
   state->tile_fill_color[2] = 0.0f;
   state->tile_fill_color[3] = 0.0f;

   /* Color for vgClear */
   state->clear_color[0] = 0.0f;
   state->clear_color[1] = 0.0f;
   state->clear_color[2] = 0.0f;
   state->clear_color[3] = 0.0f;

   /* Glyph origin */
   state->glyph_origin[0].f = 0.0f;
   state->glyph_origin[1].f = 0.0f;
   state->glyph_origin[0].i = 0;
   state->glyph_origin[1].i = 0;

   /* Enable/disable alpha masking and scissoring */
   state->masking = VG_FALSE;
   state->scissoring = VG_FALSE;

   /* Pixel layout information */
   state->pixel_layout = VG_PIXEL_LAYOUT_UNKNOWN;
   state->screen_layout = VG_PIXEL_LAYOUT_UNKNOWN;

   /* Source format selection for image filters */
   state->filter_format_linear = VG_FALSE;
   state->filter_format_premultiplied = VG_FALSE;

   /* Destination write enable mask for image filters */
   state->filter_channel_mask = (VG_RED | VG_GREEN | VG_BLUE | VG_ALPHA);

   matrix_load_identity(&state->path_user_to_surface_matrix);
   matrix_load_identity(&state->image_user_to_surface_matrix);
   matrix_load_identity(&state->fill_paint_to_user_matrix);
   matrix_load_identity(&state->stroke_paint_to_user_matrix);
   matrix_load_identity(&state->glyph_user_to_surface_matrix);
}

struct matrix *vg_state_matrix(struct vg_state *state)
{
    switch(state->matrix_mode) {
    case VG_MATRIX_PATH_USER_TO_SURFACE:
       return &state->path_user_to_surface_matrix;
    case VG_MATRIX_IMAGE_USER_TO_SURFACE:
       return &state->image_user_to_surface_matrix;
    case VG_MATRIX_FILL_PAINT_TO_USER:
       return &state->fill_paint_to_user_matrix;
    case VG_MATRIX_STROKE_PAINT_TO_USER:
       return &state->stroke_paint_to_user_matrix;
#ifdef OPENVG_VERSION_1_1
    case VG_MATRIX_GLYPH_USER_TO_SURFACE:
       return &state->glyph_user_to_surface_matrix;
#endif
    default:
       break;
    }
    return NULL;
}
