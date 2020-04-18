/*
 * Copyright (C) 2009 Francisco Jerez.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __NV10_DRIVER_H__
#define __NV10_DRIVER_H__

enum {
	NOUVEAU_STATE_ZCLEAR = NUM_NOUVEAU_STATE,
	NUM_NV10_STATE
};

#define NV10_TEXTURE_UNITS 2

/* nv10_context.c */
extern const struct nouveau_driver nv10_driver;

GLboolean
nv10_use_viewport_zclear(struct gl_context *ctx);

float
nv10_transform_depth(struct gl_context *ctx, float z);

/* nv10_render.c */
void
nv10_vbo_init(struct gl_context *ctx);

void
nv10_vbo_destroy(struct gl_context *ctx);

void
nv10_swtnl_init(struct gl_context *ctx);

void
nv10_swtnl_destroy(struct gl_context *ctx);

/* nv10_state_fb.c */
void
nv10_emit_framebuffer(struct gl_context *ctx, int emit);

void
nv10_emit_render_mode(struct gl_context *ctx, int emit);

void
nv10_emit_scissor(struct gl_context *ctx, int emit);

void
nv10_emit_viewport(struct gl_context *ctx, int emit);

void
nv10_emit_zclear(struct gl_context *ctx, int emit);

/* nv10_state_polygon.c */
void
nv10_emit_cull_face(struct gl_context *ctx, int emit);

void
nv10_emit_front_face(struct gl_context *ctx, int emit);

void
nv10_emit_line_mode(struct gl_context *ctx, int emit);

void
nv10_emit_line_stipple(struct gl_context *ctx, int emit);

void
nv10_emit_point_mode(struct gl_context *ctx, int emit);

void
nv10_emit_polygon_mode(struct gl_context *ctx, int emit);

void
nv10_emit_polygon_offset(struct gl_context *ctx, int emit);

void
nv10_emit_polygon_stipple(struct gl_context *ctx, int emit);

/* nv10_state_raster.c */
void
nv10_emit_alpha_func(struct gl_context *ctx, int emit);

void
nv10_emit_blend_color(struct gl_context *ctx, int emit);

void
nv10_emit_blend_equation(struct gl_context *ctx, int emit);

void
nv10_emit_blend_func(struct gl_context *ctx, int emit);

void
nv10_emit_color_mask(struct gl_context *ctx, int emit);

void
nv10_emit_depth(struct gl_context *ctx, int emit);

void
nv10_emit_dither(struct gl_context *ctx, int emit);

void
nv10_emit_logic_opcode(struct gl_context *ctx, int emit);

void
nv10_emit_shade_model(struct gl_context *ctx, int emit);

void
nv10_emit_stencil_func(struct gl_context *ctx, int emit);

void
nv10_emit_stencil_mask(struct gl_context *ctx, int emit);

void
nv10_emit_stencil_op(struct gl_context *ctx, int emit);

/* nv10_state_frag.c */
void
nv10_get_general_combiner(struct gl_context *ctx, int i,
			  uint32_t *a_in, uint32_t *a_out,
			  uint32_t *c_in, uint32_t *c_out, uint32_t *k);

void
nv10_get_final_combiner(struct gl_context *ctx, uint64_t *in, int *n);

void
nv10_emit_tex_env(struct gl_context *ctx, int emit);

void
nv10_emit_frag(struct gl_context *ctx, int emit);

/* nv10_state_tex.c */
void
nv10_emit_tex_gen(struct gl_context *ctx, int emit);

void
nv10_emit_tex_mat(struct gl_context *ctx, int emit);

void
nv10_emit_tex_obj(struct gl_context *ctx, int emit);

/* nv10_state_tnl.c */
void
nv10_get_fog_coeff(struct gl_context *ctx, float k[3]);

void
nv10_get_spot_coeff(struct gl_light *l, float k[7]);

void
nv10_get_shininess_coeff(float s, float k[6]);

void
nv10_emit_clip_plane(struct gl_context *ctx, int emit);

void
nv10_emit_color_material(struct gl_context *ctx, int emit);

void
nv10_emit_fog(struct gl_context *ctx, int emit);

void
nv10_emit_light_enable(struct gl_context *ctx, int emit);

void
nv10_emit_light_model(struct gl_context *ctx, int emit);

void
nv10_emit_light_source(struct gl_context *ctx, int emit);

void
nv10_emit_material_ambient(struct gl_context *ctx, int emit);

void
nv10_emit_material_diffuse(struct gl_context *ctx, int emit);

void
nv10_emit_material_specular(struct gl_context *ctx, int emit);

void
nv10_emit_material_shininess(struct gl_context *ctx, int emit);

void
nv10_emit_modelview(struct gl_context *ctx, int emit);

void
nv10_emit_point_parameter(struct gl_context *ctx, int emit);

void
nv10_emit_projection(struct gl_context *ctx, int emit);

#endif
