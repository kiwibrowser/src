/*
 * Copyright 2012 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Christian König <christian.koenig@amd.com>
 */

#ifndef SI_STATE_H
#define SI_STATE_H

#include "radeonsi_pm4.h"

struct si_state_blend {
	struct si_pm4_state	pm4;
	uint32_t		cb_target_mask;
	uint32_t		cb_color_control;
};

struct si_state_viewport {
	struct si_pm4_state		pm4;
	struct pipe_viewport_state	viewport;
};

struct si_state_rasterizer {
	struct si_pm4_state	pm4;
	bool			flatshade;
	unsigned		sprite_coord_enable;
	unsigned		pa_sc_line_stipple;
	unsigned		pa_su_sc_mode_cntl;
	unsigned		pa_cl_clip_cntl;
	unsigned		pa_cl_vs_out_cntl;
	float			offset_units;
	float			offset_scale;
};

struct si_state_dsa {
	struct si_pm4_state	pm4;
	unsigned		alpha_ref;
	unsigned		db_render_override;
	unsigned		db_render_control;
	uint8_t			valuemask[2];
	uint8_t			writemask[2];
};

struct si_vertex_element
{
	unsigned			count;
	uint32_t			rsrc_word3[PIPE_MAX_ATTRIBS];
	struct pipe_vertex_element	elements[PIPE_MAX_ATTRIBS];
};

union si_state {
	struct {
		struct si_pm4_state		*sync;
		struct si_pm4_state		*init;
		struct si_state_blend		*blend;
		struct si_pm4_state		*blend_color;
		struct si_pm4_state		*clip;
		struct si_pm4_state		*scissor;
		struct si_state_viewport	*viewport;
		struct si_pm4_state		*framebuffer;
		struct si_state_rasterizer	*rasterizer;
		struct si_state_dsa		*dsa;
		struct si_pm4_state		*fb_rs;
		struct si_pm4_state		*fb_blend;
		struct si_pm4_state		*dsa_stencil_ref;
		struct si_pm4_state		*vs;
		struct si_pm4_state		*vs_const;
		struct si_pm4_state		*ps;
		struct si_pm4_state		*ps_sampler_views;
		struct si_pm4_state		*ps_sampler;
		struct si_pm4_state		*ps_const;
		struct si_pm4_state		*spi;
		struct si_pm4_state		*vertex_buffers;
		struct si_pm4_state		*texture_barrier;
		struct si_pm4_state		*draw_info;
		struct si_pm4_state		*draw;
	} named;
	struct si_pm4_state	*array[0];
};

#define si_pm4_block_idx(member) \
	(offsetof(union si_state, named.member) / sizeof(struct si_pm4_state *))

#define si_pm4_bind_state(rctx, member, value) \
	do { \
		(rctx)->queued.named.member = (value); \
	} while(0)

#define si_pm4_delete_state(rctx, member, value) \
	do { \
		if ((rctx)->queued.named.member == (value)) { \
			(rctx)->queued.named.member = NULL; \
		} \
		si_pm4_free_state(rctx, (struct si_pm4_state *)(value), \
				  si_pm4_block_idx(member)); \
	} while(0)

#define si_pm4_set_state(rctx, member, value) \
	do { \
		if ((rctx)->queued.named.member != (value)) { \
			si_pm4_free_state(rctx, \
				(struct si_pm4_state *)(rctx)->queued.named.member, \
				si_pm4_block_idx(member)); \
			(rctx)->queued.named.member = (value); \
		} \
	} while(0)

/* si_state.c */
struct si_pipe_shader_selector;

bool si_is_format_supported(struct pipe_screen *screen,
			    enum pipe_format format,
			    enum pipe_texture_target target,
			    unsigned sample_count,
			    unsigned usage);
int si_shader_select(struct pipe_context *ctx,
		     struct si_pipe_shader_selector *sel,
		     unsigned *dirty);
void si_init_state_functions(struct r600_context *rctx);
void si_init_config(struct r600_context *rctx);

/* si_state_streamout.c */
struct pipe_stream_output_target *
si_create_so_target(struct pipe_context *ctx,
		    struct pipe_resource *buffer,
		    unsigned buffer_offset,
		    unsigned buffer_size);
void si_so_target_destroy(struct pipe_context *ctx,
			  struct pipe_stream_output_target *target);
void si_set_so_targets(struct pipe_context *ctx,
		       unsigned num_targets,
		       struct pipe_stream_output_target **targets,
		       unsigned append_bitmask);

/* si_state_draw.c */
void si_draw_vbo(struct pipe_context *ctx, const struct pipe_draw_info *dinfo);

/* si_commands.c */
void si_cmd_surface_sync(struct si_pm4_state *pm4, uint32_t cp_coher_cntl);

#endif
