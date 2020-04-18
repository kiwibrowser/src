/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
 * Copyright 2010 LunarG, Inc.  All Rights Reserved.
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

#ifndef RENDERER_H
#define RENDERER_H

#include "VG/openvg.h"

struct renderer;

struct vg_context;
struct vg_state;
struct st_framebuffer;
struct pipe_resource;
struct pipe_sampler_state;
struct pipe_sampler_view;
struct pipe_surface;
struct pipe_vertex_element;
struct pipe_vertex_buffer;
struct matrix;

struct renderer *renderer_create(struct vg_context *owner);
void renderer_destroy(struct renderer *);

void renderer_validate(struct renderer *renderer,
                       VGbitfield dirty,
                       const struct st_framebuffer *stfb,
                       const struct vg_state *state);

void renderer_validate_for_shader(struct renderer *renderer,
                                  const struct pipe_sampler_state **samplers,
                                  struct pipe_sampler_view **views,
                                  VGint num_samplers,
                                  const struct matrix *modelview,
                                  void *fs,
                                  const void *const_buffer,
                                  VGint const_buffer_len);

void renderer_validate_for_mask_rendering(struct renderer *renderer,
                                          struct pipe_surface *dst,
                                          const struct matrix *modelview);

VGboolean renderer_copy_begin(struct renderer *renderer,
                              struct pipe_surface *dst,
                              VGboolean y0_top,
                              struct pipe_sampler_view *src);

void renderer_copy(struct renderer *renderer,
                   VGint x, VGint y, VGint w, VGint h,
                   VGint sx, VGint sy, VGint sw, VGint sh);

void renderer_copy_end(struct renderer *renderer);

VGboolean renderer_drawtex_begin(struct renderer *renderer,
                                 struct pipe_sampler_view *src);

void renderer_drawtex(struct renderer *renderer,
                      VGint x, VGint y, VGint w, VGint h,
                      VGint sx, VGint sy, VGint sw, VGint sh);

void renderer_drawtex_end(struct renderer *renderer);

VGboolean renderer_scissor_begin(struct renderer *renderer,
                                 VGboolean restore_dsa);

void renderer_scissor(struct renderer *renderer,
                      VGint x, VGint y, VGint width, VGint height);

void renderer_scissor_end(struct renderer *renderer);

VGboolean renderer_clear_begin(struct renderer *renderer);

void renderer_clear(struct renderer *renderer,
                    VGint x, VGint y, VGint width, VGint height,
                    const VGfloat color[4]);

void renderer_clear_end(struct renderer *renderer);

VGboolean renderer_filter_begin(struct renderer *renderer,
                                struct pipe_resource *dst,
                                VGboolean y0_top,
                                VGbitfield channel_mask,
                                const struct pipe_sampler_state **samplers,
                                struct pipe_sampler_view **views,
                                VGint num_samplers,
                                void *fs,
                                const void *const_buffer,
                                VGint const_buffer_len);

void renderer_filter(struct renderer *renderer,
                     VGint x, VGint y, VGint w, VGint h,
                     VGint sx, VGint sy, VGint sw, VGint sh);

void renderer_filter_end(struct renderer *renderer);

VGboolean renderer_polygon_stencil_begin(struct renderer *renderer,
                                         struct pipe_vertex_element *velem,
                                         VGFillRule rule,
                                         VGboolean restore_dsa);

void renderer_polygon_stencil(struct renderer *renderer,
                              struct pipe_vertex_buffer *vbuf,
                              VGuint mode, VGuint start, VGuint count);

void renderer_polygon_stencil_end(struct renderer *renderer);

VGboolean renderer_polygon_fill_begin(struct renderer *renderer,
                                      VGboolean save_dsa);

void renderer_polygon_fill(struct renderer *renderer,
                           VGfloat min_x, VGfloat min_y,
                           VGfloat max_x, VGfloat max_y);

void renderer_polygon_fill_end(struct renderer *renderer);

void renderer_texture_quad(struct renderer *,
                           struct pipe_resource *texture,
                           VGfloat x1offset, VGfloat y1offset,
                           VGfloat x2offset, VGfloat y2offset,
                           VGfloat x1, VGfloat y1,
                           VGfloat x2, VGfloat y2,
                           VGfloat x3, VGfloat y3,
                           VGfloat x4, VGfloat y4);

void renderer_copy_surface(struct renderer *r,
                           struct pipe_surface *src,
                           int sx1, int sy1,
                           int sx2, int sy2,
                           struct pipe_surface *dst,
                           int dx1, int dy1,
                           int dx2, int dy2,
                           float z, unsigned filter);


#endif
