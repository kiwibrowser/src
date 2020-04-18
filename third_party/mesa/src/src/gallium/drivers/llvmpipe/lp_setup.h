/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
#ifndef LP_SETUP_H
#define LP_SETUP_H

#include "pipe/p_compiler.h"
#include "lp_jit.h"

struct draw_context;
struct vertex_info;


struct pipe_resource;
struct pipe_query;
struct pipe_surface;
struct pipe_blend_color;
struct pipe_screen;
struct pipe_framebuffer_state;
struct lp_fragment_shader_variant;
struct lp_jit_context;
struct llvmpipe_query;
struct pipe_fence_handle;
struct lp_setup_variant;
struct lp_setup_context;

void lp_setup_reset( struct lp_setup_context *setup );

struct lp_setup_context *
lp_setup_create( struct pipe_context *pipe,
                 struct draw_context *draw );

void
lp_setup_clear(struct lp_setup_context *setup,
               const float *clear_color,
               double clear_depth,
               unsigned clear_stencil,
               unsigned flags);



void
lp_setup_flush( struct lp_setup_context *setup,
                struct pipe_fence_handle **fence,
                const char *reason);


void
lp_setup_bind_framebuffer( struct lp_setup_context *setup,
                           const struct pipe_framebuffer_state *fb );

void 
lp_setup_set_triangle_state( struct lp_setup_context *setup,
                             unsigned cullmode,
                             boolean front_is_ccw,
                             boolean scissor,
                             boolean gl_rasterization_rules );

void 
lp_setup_set_line_state( struct lp_setup_context *setup,
                         float line_width);

void 
lp_setup_set_point_state( struct lp_setup_context *setup,
                          float point_size,                          
                          boolean point_size_per_vertex,
                          uint sprite_coord_enable,
                          uint sprite_coord_origin);

void
lp_setup_set_setup_variant( struct lp_setup_context *setup,
			    const struct lp_setup_variant *variant );

void
lp_setup_set_fs_variant( struct lp_setup_context *setup,
                         struct lp_fragment_shader_variant *variant );

void
lp_setup_set_fs_constants(struct lp_setup_context *setup,
                          struct pipe_resource *buffer);


void
lp_setup_set_alpha_ref_value( struct lp_setup_context *setup,
                              float alpha_ref_value );

void
lp_setup_set_stencil_ref_values( struct lp_setup_context *setup,
                                 const ubyte refs[2] );

void
lp_setup_set_blend_color( struct lp_setup_context *setup,
                          const struct pipe_blend_color *blend_color );

void
lp_setup_set_scissor( struct lp_setup_context *setup,
                      const struct pipe_scissor_state *scissor );

void
lp_setup_set_fragment_sampler_views(struct lp_setup_context *setup,
                                    unsigned num,
                                    struct pipe_sampler_view **views);

void
lp_setup_set_fragment_sampler_state(struct lp_setup_context *setup,
                                    unsigned num,
                                    struct pipe_sampler_state **samplers);

unsigned
lp_setup_is_resource_referenced( const struct lp_setup_context *setup,
                                const struct pipe_resource *texture );

void
lp_setup_set_flatshade_first( struct lp_setup_context *setup, 
                              boolean flatshade_first );

void
lp_setup_set_vertex_info( struct lp_setup_context *setup, 
                          struct vertex_info *info );

void
lp_setup_begin_query(struct lp_setup_context *setup,
                     struct llvmpipe_query *pq);

void
lp_setup_end_query(struct lp_setup_context *setup,
                   struct llvmpipe_query *pq);

#endif
