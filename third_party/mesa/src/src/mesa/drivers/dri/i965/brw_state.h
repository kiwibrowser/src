/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
    

#ifndef BRW_STATE_H
#define BRW_STATE_H

#include "brw_context.h"

#ifdef __cplusplus
extern "C" {
#endif

enum intel_msaa_layout;

extern const struct brw_tracked_state brw_blend_constant_color;
extern const struct brw_tracked_state brw_cc_vp;
extern const struct brw_tracked_state brw_cc_unit;
extern const struct brw_tracked_state brw_clip_prog;
extern const struct brw_tracked_state brw_clip_unit;
extern const struct brw_tracked_state brw_vs_pull_constants;
extern const struct brw_tracked_state brw_wm_pull_constants;
extern const struct brw_tracked_state brw_constant_buffer;
extern const struct brw_tracked_state brw_curbe_offsets;
extern const struct brw_tracked_state brw_invariant_state;
extern const struct brw_tracked_state brw_gs_prog;
extern const struct brw_tracked_state brw_gs_unit;
extern const struct brw_tracked_state brw_line_stipple;
extern const struct brw_tracked_state brw_aa_line_parameters;
extern const struct brw_tracked_state brw_pipelined_state_pointers;
extern const struct brw_tracked_state brw_binding_table_pointers;
extern const struct brw_tracked_state brw_depthbuffer;
extern const struct brw_tracked_state brw_polygon_stipple_offset;
extern const struct brw_tracked_state brw_polygon_stipple;
extern const struct brw_tracked_state brw_program_parameters;
extern const struct brw_tracked_state brw_recalculate_urb_fence;
extern const struct brw_tracked_state brw_samplers;
extern const struct brw_tracked_state brw_sf_prog;
extern const struct brw_tracked_state brw_sf_unit;
extern const struct brw_tracked_state brw_sf_vp;
extern const struct brw_tracked_state brw_state_base_address;
extern const struct brw_tracked_state brw_urb_fence;
extern const struct brw_tracked_state brw_vertex_state;
extern const struct brw_tracked_state brw_vs_prog;
extern const struct brw_tracked_state brw_vs_ubo_surfaces;
extern const struct brw_tracked_state brw_vs_unit;
extern const struct brw_tracked_state brw_wm_input_sizes;
extern const struct brw_tracked_state brw_wm_prog;
extern const struct brw_tracked_state brw_renderbuffer_surfaces;
extern const struct brw_tracked_state brw_texture_surfaces;
extern const struct brw_tracked_state brw_wm_binding_table;
extern const struct brw_tracked_state brw_vs_binding_table;
extern const struct brw_tracked_state brw_wm_ubo_surfaces;
extern const struct brw_tracked_state brw_wm_unit;

extern const struct brw_tracked_state brw_psp_urb_cbs;

extern const struct brw_tracked_state brw_pipe_control;

extern const struct brw_tracked_state brw_drawing_rect;
extern const struct brw_tracked_state brw_indices;
extern const struct brw_tracked_state brw_vertices;
extern const struct brw_tracked_state brw_index_buffer;
extern const struct brw_tracked_state gen6_binding_table_pointers;
extern const struct brw_tracked_state gen6_blend_state;
extern const struct brw_tracked_state gen6_cc_state_pointers;
extern const struct brw_tracked_state gen6_clip_state;
extern const struct brw_tracked_state gen6_clip_vp;
extern const struct brw_tracked_state gen6_color_calc_state;
extern const struct brw_tracked_state gen6_depth_stencil_state;
extern const struct brw_tracked_state gen6_gs_state;
extern const struct brw_tracked_state gen6_gs_binding_table;
extern const struct brw_tracked_state gen6_multisample_state;
extern const struct brw_tracked_state gen6_renderbuffer_surfaces;
extern const struct brw_tracked_state gen6_sampler_state;
extern const struct brw_tracked_state gen6_scissor_state;
extern const struct brw_tracked_state gen6_sol_indices;
extern const struct brw_tracked_state gen6_sol_surface;
extern const struct brw_tracked_state gen6_sf_state;
extern const struct brw_tracked_state gen6_sf_vp;
extern const struct brw_tracked_state gen6_urb;
extern const struct brw_tracked_state gen6_viewport_state;
extern const struct brw_tracked_state gen6_vs_push_constants;
extern const struct brw_tracked_state gen6_vs_state;
extern const struct brw_tracked_state gen6_wm_push_constants;
extern const struct brw_tracked_state gen6_wm_state;
extern const struct brw_tracked_state gen7_depthbuffer;
extern const struct brw_tracked_state gen7_blend_state_pointer;
extern const struct brw_tracked_state gen7_cc_state_pointer;
extern const struct brw_tracked_state gen7_cc_viewport_state_pointer;
extern const struct brw_tracked_state gen7_clip_state;
extern const struct brw_tracked_state gen7_depth_stencil_state_pointer;
extern const struct brw_tracked_state gen7_disable_stages;
extern const struct brw_tracked_state gen7_ps_state;
extern const struct brw_tracked_state gen7_push_constant_alloc;
extern const struct brw_tracked_state gen7_samplers;
extern const struct brw_tracked_state gen7_sbe_state;
extern const struct brw_tracked_state gen7_sf_clip_viewport;
extern const struct brw_tracked_state gen7_sf_clip_viewport_state_pointer;
extern const struct brw_tracked_state gen7_sf_state;
extern const struct brw_tracked_state gen7_sol_state;
extern const struct brw_tracked_state gen7_urb;
extern const struct brw_tracked_state gen7_vs_state;
extern const struct brw_tracked_state gen7_wm_constants;
extern const struct brw_tracked_state gen7_wm_constant_surface;
extern const struct brw_tracked_state gen7_wm_state;
extern const struct brw_tracked_state gen7_wm_surfaces;
extern const struct brw_tracked_state haswell_cut_index;

/* brw_misc_state.c */
uint32_t
brw_depthbuffer_format(struct brw_context *brw);


/***********************************************************************
 * brw_state.c
 */
void brw_upload_state(struct brw_context *brw);
void brw_init_state(struct brw_context *brw);
void brw_destroy_state(struct brw_context *brw);

/***********************************************************************
 * brw_state_cache.c
 */

void brw_upload_cache(struct brw_cache *cache,
		      enum brw_cache_id cache_id,
		      const void *key,
		      GLuint key_sz,
		      const void *data,
		      GLuint data_sz,
		      const void *aux,
		      GLuint aux_sz,
		      uint32_t *out_offset, void *out_aux);

bool brw_search_cache(struct brw_cache *cache,
		      enum brw_cache_id cache_id,
		      const void *key,
		      GLuint key_size,
		      uint32_t *inout_offset, void *out_aux);
void brw_state_cache_check_size( struct brw_context *brw );

void brw_init_caches( struct brw_context *brw );
void brw_destroy_caches( struct brw_context *brw );

/***********************************************************************
 * brw_state_batch.c
 */
#define BRW_BATCH_STRUCT(brw, s) intel_batchbuffer_data(&brw->intel, (s), \
							sizeof(*(s)), false)

void *brw_state_batch(struct brw_context *brw,
		      enum state_struct_type type,
		      int size,
		      int alignment,
		      uint32_t *out_offset);

/* brw_wm_surface_state.c */
void gen4_init_vtable_surface_functions(struct brw_context *brw);
uint32_t brw_get_surface_tiling_bits(uint32_t tiling);
uint32_t brw_get_surface_num_multisamples(unsigned num_samples);
void brw_create_constant_surface(struct brw_context *brw,
				 drm_intel_bo *bo,
				 uint32_t offset,
				 int width,
				 uint32_t *out_offset);

uint32_t brw_format_for_mesa_format(gl_format mesa_format);

GLuint translate_tex_target(GLenum target);

GLuint translate_tex_format(gl_format mesa_format,
			    GLenum internal_format,
			    GLenum depth_mode,
			    GLenum srgb_decode);

int brw_get_texture_swizzle(const struct gl_texture_object *t);

/* gen7_wm_surface_state.c */
void gen7_set_surface_tiling(struct gen7_surface_state *surf, uint32_t tiling);
void gen7_set_surface_msaa(struct gen7_surface_state *surf,
                           unsigned num_samples,
                           enum intel_msaa_layout layout);
void gen7_set_surface_mcs_info(struct brw_context *brw,
                               struct gen7_surface_state *surf,
                               uint32_t surf_offset,
                               const struct intel_mipmap_tree *mcs_mt,
                               bool is_render_target);
void gen7_check_surface_setup(struct gen7_surface_state *surf,
                              bool is_render_target);
void gen7_init_vtable_surface_functions(struct brw_context *brw);
void gen7_create_constant_surface(struct brw_context *brw,
				  drm_intel_bo *bo,
				  uint32_t offset,
				  int width,
				  uint32_t *out_offset);

/* brw_wm_sampler_state.c */
uint32_t translate_wrap_mode(GLenum wrap, bool using_nearest);
void upload_default_color(struct brw_context *brw,
			  struct gl_sampler_object *sampler,
			  int unit, int ss_index);

/* gen6_sf_state.c */
uint32_t
get_attr_override(struct brw_vue_map *vue_map, int urb_entry_read_offset,
                  int fs_attr, bool two_side_color, uint32_t *max_source_attr);

#ifdef __cplusplus
}
#endif

#endif
