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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef LP_STATE_H
#define LP_STATE_H

#include "pipe/p_state.h"
#include "lp_jit.h"
#include "lp_state_fs.h"
#include "gallivm/lp_bld.h"


#define LP_NEW_VIEWPORT      0x1
#define LP_NEW_RASTERIZER    0x2
#define LP_NEW_FS            0x4
#define LP_NEW_BLEND         0x8
#define LP_NEW_CLIP          0x10
#define LP_NEW_SCISSOR       0x20
#define LP_NEW_STIPPLE       0x40
#define LP_NEW_FRAMEBUFFER   0x80
#define LP_NEW_DEPTH_STENCIL_ALPHA 0x100
#define LP_NEW_CONSTANTS     0x200
#define LP_NEW_SAMPLER       0x400
#define LP_NEW_SAMPLER_VIEW  0x800
#define LP_NEW_VERTEX        0x1000
#define LP_NEW_VS            0x2000
#define LP_NEW_QUERY         0x4000
#define LP_NEW_BLEND_COLOR   0x8000
#define LP_NEW_GS            0x10000
#define LP_NEW_SO            0x20000
#define LP_NEW_SO_BUFFERS    0x40000



struct vertex_info;
struct pipe_context;
struct llvmpipe_context;



/** Subclass of pipe_shader_state */
struct lp_vertex_shader
{
   struct pipe_shader_state shader;
   struct draw_vertex_shader *draw_data;
};

/** Subclass of pipe_shader_state */
struct lp_geometry_shader {
   struct pipe_shader_state shader;
   struct draw_geometry_shader *draw_data;
};

/** Vertex element state */
struct lp_velems_state
{
   unsigned count;
   struct pipe_vertex_element velem[PIPE_MAX_ATTRIBS];
};

struct lp_so_state {
   struct pipe_stream_output_info base;
};


void
llvmpipe_set_framebuffer_state(struct pipe_context *,
                               const struct pipe_framebuffer_state *);

void
llvmpipe_update_fs(struct llvmpipe_context *lp);

void 
llvmpipe_update_setup(struct llvmpipe_context *lp);

void
llvmpipe_update_derived(struct llvmpipe_context *llvmpipe);

void
llvmpipe_init_sampler_funcs(struct llvmpipe_context *llvmpipe);

void
llvmpipe_init_blend_funcs(struct llvmpipe_context *llvmpipe);

void
llvmpipe_init_vertex_funcs(struct llvmpipe_context *llvmpipe);

void
llvmpipe_init_draw_funcs(struct llvmpipe_context *llvmpipe);

void
llvmpipe_init_clip_funcs(struct llvmpipe_context *llvmpipe);

void
llvmpipe_init_fs_funcs(struct llvmpipe_context *llvmpipe);

void
llvmpipe_init_vs_funcs(struct llvmpipe_context *llvmpipe);

void
llvmpipe_init_gs_funcs(struct llvmpipe_context *llvmpipe);

void
llvmpipe_init_rasterizer_funcs(struct llvmpipe_context *llvmpipe);

void
llvmpipe_init_so_funcs(struct llvmpipe_context *llvmpipe);

void
llvmpipe_prepare_vertex_sampling(struct llvmpipe_context *ctx,
                                 unsigned num,
                                 struct pipe_sampler_view **views);
void
llvmpipe_cleanup_vertex_sampling(struct llvmpipe_context *ctx);


#endif
