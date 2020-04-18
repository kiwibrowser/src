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

#ifndef PAINT_H
#define PAINT_H

#include "vg_context.h"

#include "VG/openvg.h"
#include "pipe/p_state.h"

struct vg_paint;
struct vg_image;
struct pipe_sampler_state;
struct pipe_resource;

struct vg_paint *paint_create(struct vg_context *ctx);
void paint_destroy(struct vg_paint *paint);

void paint_set_color(struct vg_paint *paint,
                     const VGfloat *color);
void paint_get_color(struct vg_paint *paint,
                     VGfloat *color);

void paint_set_coloriv(struct vg_paint *paint,
                      const VGint *color);
void paint_get_coloriv(struct vg_paint *paint,
                      VGint *color);

void paint_set_colori(struct vg_paint *paint,
                      VGuint rgba);

VGuint paint_colori(struct vg_paint *paint);

void paint_set_type(struct vg_paint *paint, VGPaintType type);
VGPaintType paint_type(struct vg_paint *paint);
void paint_resolve_type(struct vg_paint *paint);

void paint_set_linear_gradient(struct vg_paint *paint,
                               const VGfloat *coords);
void paint_linear_gradient(struct vg_paint *paint,
                           VGfloat *coords);
void paint_set_linear_gradienti(struct vg_paint *paint,
                               const VGint *coords);
void paint_linear_gradienti(struct vg_paint *paint,
                           VGint *coords);


void paint_set_radial_gradient(struct vg_paint *paint,
                               const VGfloat *values);
void paint_radial_gradient(struct vg_paint *paint,
                           VGfloat *coords);
void paint_set_radial_gradienti(struct vg_paint *paint,
                                const VGint *values);
void paint_radial_gradienti(struct vg_paint *paint,
                            VGint *coords);


void paint_set_ramp_stops(struct vg_paint *paint, const VGfloat *stops,
                          int num);
void paint_ramp_stops(struct vg_paint *paint, VGfloat *stops,
                      int num);

void paint_set_ramp_stopsi(struct vg_paint *paint, const VGint *stops,
                           int num);
void paint_ramp_stopsi(struct vg_paint *paint, VGint *stops,
                       int num);

int paint_num_ramp_stops(struct vg_paint *paint);

void paint_set_spread_mode(struct vg_paint *paint,
                           VGint mode);
VGColorRampSpreadMode paint_spread_mode(struct vg_paint *paint);


void paint_set_pattern(struct vg_paint *paint,
                       struct vg_image *img);
void paint_set_pattern_tiling(struct vg_paint *paint,
                              VGTilingMode mode);
VGTilingMode paint_pattern_tiling(struct vg_paint *paint);

void paint_set_color_ramp_premultiplied(struct vg_paint *paint,
                                        VGboolean set);
VGboolean paint_color_ramp_premultiplied(struct vg_paint *paint);


VGint paint_bind_samplers(struct vg_paint *paint, struct pipe_sampler_state **samplers,
                          struct pipe_sampler_view **sampler_views);

VGboolean paint_is_degenerate(struct vg_paint *paint);

VGint paint_constant_buffer_size(struct vg_paint *paint);

void paint_fill_constant_buffer(struct vg_paint *paint,
                                const struct matrix *mat,
                                void *buffer);

VGboolean paint_is_opaque(struct vg_paint *paint);

#endif
