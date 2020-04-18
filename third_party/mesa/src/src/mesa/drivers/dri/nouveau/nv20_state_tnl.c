/*
 * Copyright (C) 2009-2010 Francisco Jerez.
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

#include "nouveau_driver.h"
#include "nouveau_context.h"
#include "nouveau_gldefs.h"
#include "nouveau_util.h"
#include "nv20_3d.xml.h"
#include "nv10_driver.h"
#include "nv20_driver.h"

#define LIGHT_MODEL_AMBIENT_R(side)			\
	((side) ? NV20_3D_LIGHT_MODEL_BACK_AMBIENT_R :	\
	 NV20_3D_LIGHT_MODEL_FRONT_AMBIENT_R)
#define LIGHT_AMBIENT_R(side, i)			\
	((side) ? NV20_3D_LIGHT_BACK_AMBIENT_R(i) :	\
	 NV20_3D_LIGHT_FRONT_AMBIENT_R(i))
#define LIGHT_DIFFUSE_R(side, i)			\
	((side) ? NV20_3D_LIGHT_BACK_DIFFUSE_R(i) :	\
	 NV20_3D_LIGHT_FRONT_DIFFUSE_R(i))
#define LIGHT_SPECULAR_R(side, i)			\
	((side) ? NV20_3D_LIGHT_BACK_SPECULAR_R(i) :	\
	 NV20_3D_LIGHT_FRONT_SPECULAR_R(i))
#define MATERIAL_FACTOR_R(side)				\
	((side) ? NV20_3D_MATERIAL_FACTOR_BACK_R :	\
	 NV20_3D_MATERIAL_FACTOR_FRONT_R)
#define MATERIAL_FACTOR_A(side)				\
	((side) ? NV20_3D_MATERIAL_FACTOR_BACK_A :	\
	 NV20_3D_MATERIAL_FACTOR_FRONT_A)
#define MATERIAL_SHININESS(side)			\
	((side) ? NV20_3D_BACK_MATERIAL_SHININESS(0) :	\
	 NV20_3D_FRONT_MATERIAL_SHININESS(0))

void
nv20_emit_clip_plane(struct gl_context *ctx, int emit)
{
}

static inline unsigned
get_material_bitmask(unsigned m)
{
	unsigned ret = 0;

	if (m & MAT_BIT_FRONT_EMISSION)
		ret |= NV20_3D_COLOR_MATERIAL_FRONT_EMISSION_COL1;
	if (m & MAT_BIT_FRONT_AMBIENT)
		ret |= NV20_3D_COLOR_MATERIAL_FRONT_AMBIENT_COL1;
	if (m & MAT_BIT_FRONT_DIFFUSE)
		ret |= NV20_3D_COLOR_MATERIAL_FRONT_DIFFUSE_COL1;
	if (m & MAT_BIT_FRONT_SPECULAR)
		ret |= NV20_3D_COLOR_MATERIAL_FRONT_SPECULAR_COL1;

	if (m & MAT_BIT_BACK_EMISSION)
		ret |= NV20_3D_COLOR_MATERIAL_BACK_EMISSION_COL1;
	if (m & MAT_BIT_BACK_AMBIENT)
		ret |= NV20_3D_COLOR_MATERIAL_BACK_AMBIENT_COL1;
	if (m & MAT_BIT_BACK_DIFFUSE)
		ret |= NV20_3D_COLOR_MATERIAL_BACK_DIFFUSE_COL1;
	if (m & MAT_BIT_BACK_SPECULAR)
		ret |= NV20_3D_COLOR_MATERIAL_BACK_SPECULAR_COL1;

	return ret;
}

void
nv20_emit_color_material(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	unsigned mask = get_material_bitmask(ctx->Light._ColorMaterialBitmask);

	BEGIN_NV04(push, NV20_3D(COLOR_MATERIAL), 1);
	PUSH_DATA (push, ctx->Light.ColorMaterialEnabled ? mask : 0);
}

static unsigned
get_fog_mode_signed(unsigned mode)
{
	switch (mode) {
	case GL_LINEAR:
		return NV20_3D_FOG_MODE_LINEAR_SIGNED;
	case GL_EXP:
		return NV20_3D_FOG_MODE_EXP_SIGNED;
	case GL_EXP2:
		return NV20_3D_FOG_MODE_EXP2_SIGNED;
	default:
		assert(0);
	}
}

static unsigned
get_fog_mode_unsigned(unsigned mode)
{
	switch (mode) {
	case GL_LINEAR:
		return NV20_3D_FOG_MODE_LINEAR_UNSIGNED;
	case GL_EXP:
		return NV20_3D_FOG_MODE_EXP_UNSIGNED;
	case GL_EXP2:
		return NV20_3D_FOG_MODE_EXP2_UNSIGNED;
	default:
		assert(0);
	}
}

static unsigned
get_fog_source(unsigned source, unsigned distance_mode)
{
	switch (source) {
	case GL_FOG_COORDINATE_EXT:
		return NV20_3D_FOG_COORD_FOG;
	case GL_FRAGMENT_DEPTH_EXT:
		switch (distance_mode) {
		case GL_EYE_PLANE_ABSOLUTE_NV:
			return NV20_3D_FOG_COORD_DIST_ORTHOGONAL_ABS;
		case GL_EYE_PLANE:
			return NV20_3D_FOG_COORD_DIST_ORTHOGONAL;
		case GL_EYE_RADIAL_NV:
			return NV20_3D_FOG_COORD_DIST_RADIAL;
		default:
			assert(0);
		}
	default:
		assert(0);
	}
}

void
nv20_emit_fog(struct gl_context *ctx, int emit)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	struct gl_fog_attrib *f = &ctx->Fog;
	unsigned source = nctx->fallback == HWTNL ?
		f->FogCoordinateSource : GL_FOG_COORDINATE_EXT;
	float k[3];

	nv10_get_fog_coeff(ctx, k);

	BEGIN_NV04(push, NV20_3D(FOG_MODE), 4);
	PUSH_DATA (push, ((source == GL_FRAGMENT_DEPTH_EXT &&
			 f->FogDistanceMode == GL_EYE_PLANE_ABSOLUTE_NV) ?
			get_fog_mode_unsigned(f->Mode) :
			get_fog_mode_signed(f->Mode)));
	PUSH_DATA (push, get_fog_source(source, f->FogDistanceMode));
	PUSH_DATAb(push, f->Enabled);
	PUSH_DATA (push, pack_rgba_f(MESA_FORMAT_RGBA8888_REV, f->Color));

	BEGIN_NV04(push, NV20_3D(FOG_COEFF(0)), 3);
	PUSH_DATAp(push, k, 3);
}

void
nv20_emit_light_model(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	struct gl_lightmodel *m = &ctx->Light.Model;

	BEGIN_NV04(push, NV20_3D(SEPARATE_SPECULAR_ENABLE), 1);
	PUSH_DATAb(push, m->ColorControl == GL_SEPARATE_SPECULAR_COLOR);

	BEGIN_NV04(push, NV20_3D(LIGHT_MODEL), 1);
	PUSH_DATA (push, ((m->LocalViewer ?
			 NV20_3D_LIGHT_MODEL_VIEWER_LOCAL :
			 NV20_3D_LIGHT_MODEL_VIEWER_NONLOCAL) |
			(_mesa_need_secondary_color(ctx) ?
			 NV20_3D_LIGHT_MODEL_SEPARATE_SPECULAR :
			 0)));

	BEGIN_NV04(push, NV20_3D(LIGHT_MODEL_TWO_SIDE_ENABLE), 1);
	PUSH_DATAb(push, ctx->Light.Model.TwoSide);
}

void
nv20_emit_light_source(struct gl_context *ctx, int emit)
{
	const int i = emit - NOUVEAU_STATE_LIGHT_SOURCE0;
	struct nouveau_pushbuf *push = context_push(ctx);
	struct gl_light *l = &ctx->Light.Light[i];

	if (l->_Flags & LIGHT_POSITIONAL) {
		BEGIN_NV04(push, NV20_3D(LIGHT_POSITION_X(i)), 3);
		PUSH_DATAp(push, l->_Position, 3);

		BEGIN_NV04(push, NV20_3D(LIGHT_ATTENUATION_CONSTANT(i)), 3);
		PUSH_DATAf(push, l->ConstantAttenuation);
		PUSH_DATAf(push, l->LinearAttenuation);
		PUSH_DATAf(push, l->QuadraticAttenuation);

	} else {
		BEGIN_NV04(push, NV20_3D(LIGHT_DIRECTION_X(i)), 3);
		PUSH_DATAp(push, l->_VP_inf_norm, 3);

		BEGIN_NV04(push, NV20_3D(LIGHT_HALF_VECTOR_X(i)), 3);
		PUSH_DATAp(push, l->_h_inf_norm, 3);
	}

	if (l->_Flags & LIGHT_SPOT) {
		float k[7];

		nv10_get_spot_coeff(l, k);

		BEGIN_NV04(push, NV20_3D(LIGHT_SPOT_CUTOFF(i, 0)), 7);
		PUSH_DATAp(push, k, 7);
	}
}

#define USE_COLOR_MATERIAL(attr, side)					\
	(ctx->Light.ColorMaterialEnabled &&				\
	 ctx->Light._ColorMaterialBitmask & (1 << MAT_ATTRIB_##attr(side)))

void
nv20_emit_material_ambient(struct gl_context *ctx, int emit)
{
	const int side = emit - NOUVEAU_STATE_MATERIAL_FRONT_AMBIENT;
	struct nouveau_pushbuf *push = context_push(ctx);
	float (*mat)[4] = ctx->Light.Material.Attrib;
	float c_scene[3], c_factor[3];
	struct gl_light *l;

	if (USE_COLOR_MATERIAL(AMBIENT, side)) {
		COPY_3V(c_scene, mat[MAT_ATTRIB_EMISSION(side)]);
		COPY_3V(c_factor, ctx->Light.Model.Ambient);

	} else if (USE_COLOR_MATERIAL(EMISSION, side)) {
		SCALE_3V(c_scene, mat[MAT_ATTRIB_AMBIENT(side)],
			 ctx->Light.Model.Ambient);
		ASSIGN_3V(c_factor, 1, 1, 1);

	} else {
		COPY_3V(c_scene, ctx->Light._BaseColor[side]);
		ZERO_3V(c_factor);
	}

	BEGIN_NV04(push, SUBC_3D(LIGHT_MODEL_AMBIENT_R(side)), 3);
	PUSH_DATAp(push, c_scene, 3);

	if (ctx->Light.ColorMaterialEnabled) {
		BEGIN_NV04(push, SUBC_3D(MATERIAL_FACTOR_R(side)), 3);
		PUSH_DATAp(push, c_factor, 3);
	}

	foreach(l, &ctx->Light.EnabledList) {
		const int i = l - ctx->Light.Light;
		float *c_light = (USE_COLOR_MATERIAL(AMBIENT, side) ?
				  l->Ambient :
				  l->_MatAmbient[side]);

		BEGIN_NV04(push, SUBC_3D(LIGHT_AMBIENT_R(side, i)), 3);
		PUSH_DATAp(push, c_light, 3);
	}
}

void
nv20_emit_material_diffuse(struct gl_context *ctx, int emit)
{
	const int side = emit - NOUVEAU_STATE_MATERIAL_FRONT_DIFFUSE;
	struct nouveau_pushbuf *push = context_push(ctx);
	GLfloat (*mat)[4] = ctx->Light.Material.Attrib;
	struct gl_light *l;

	BEGIN_NV04(push, SUBC_3D(MATERIAL_FACTOR_A(side)), 1);
	PUSH_DATAf(push, mat[MAT_ATTRIB_DIFFUSE(side)][3]);

	foreach(l, &ctx->Light.EnabledList) {
		const int i = l - ctx->Light.Light;
		float *c_light = (USE_COLOR_MATERIAL(DIFFUSE, side) ?
				  l->Diffuse :
				  l->_MatDiffuse[side]);

		BEGIN_NV04(push, SUBC_3D(LIGHT_DIFFUSE_R(side, i)), 3);
		PUSH_DATAp(push, c_light, 3);
	}
}

void
nv20_emit_material_specular(struct gl_context *ctx, int emit)
{
	const int side = emit - NOUVEAU_STATE_MATERIAL_FRONT_SPECULAR;
	struct nouveau_pushbuf *push = context_push(ctx);
	struct gl_light *l;

	foreach(l, &ctx->Light.EnabledList) {
		const int i = l - ctx->Light.Light;
		float *c_light = (USE_COLOR_MATERIAL(SPECULAR, side) ?
				  l->Specular :
				  l->_MatSpecular[side]);

		BEGIN_NV04(push, SUBC_3D(LIGHT_SPECULAR_R(side, i)), 3);
		PUSH_DATAp(push, c_light, 3);
	}
}

void
nv20_emit_material_shininess(struct gl_context *ctx, int emit)
{
	const int side = emit - NOUVEAU_STATE_MATERIAL_FRONT_SHININESS;
	struct nouveau_pushbuf *push = context_push(ctx);
	float (*mat)[4] = ctx->Light.Material.Attrib;
	float k[6];

	nv10_get_shininess_coeff(
		CLAMP(mat[MAT_ATTRIB_SHININESS(side)][0], 0, 1024),
		k);

	BEGIN_NV04(push, SUBC_3D(MATERIAL_SHININESS(side)), 6);
	PUSH_DATAp(push, k, 6);
}

void
nv20_emit_modelview(struct gl_context *ctx, int emit)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	GLmatrix *m = ctx->ModelviewMatrixStack.Top;

	if (nctx->fallback != HWTNL)
		return;

	if (ctx->Light._NeedEyeCoords || ctx->Fog.Enabled ||
	    (ctx->Texture._GenFlags & TEXGEN_NEED_EYE_COORD)) {
		BEGIN_NV04(push, NV20_3D(MODELVIEW_MATRIX(0, 0)), 16);
		PUSH_DATAm(push, m->m);
	}

	if (ctx->Light.Enabled ||
	    (ctx->Texture._GenFlags & TEXGEN_NEED_EYE_COORD)) {
		int i, j;

		BEGIN_NV04(push, NV20_3D(INVERSE_MODELVIEW_MATRIX(0, 0)), 12);
		for (i = 0; i < 3; i++)
			for (j = 0; j < 4; j++)
				PUSH_DATAf(push, m->inv[4*i + j]);
	}
}

void
nv20_emit_projection(struct gl_context *ctx, int emit)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	GLmatrix m;

	_math_matrix_ctr(&m);
	get_viewport_scale(ctx, m.m);

	if (nctx->fallback == HWTNL)
		_math_matrix_mul_matrix(&m, &m, &ctx->_ModelProjectMatrix);

	BEGIN_NV04(push, NV20_3D(PROJECTION_MATRIX(0)), 16);
	PUSH_DATAm(push, m.m);

	_math_matrix_dtr(&m);
}
