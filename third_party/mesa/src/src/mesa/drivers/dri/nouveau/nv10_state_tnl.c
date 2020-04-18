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
#include "nv10_3d.xml.h"
#include "nv10_driver.h"

void
nv10_emit_clip_plane(struct gl_context *ctx, int emit)
{
}

static inline unsigned
get_material_bitmask(unsigned m)
{
	unsigned ret = 0;

	if (m & MAT_BIT_FRONT_EMISSION)
		ret |= NV10_3D_COLOR_MATERIAL_EMISSION;
	if (m & MAT_BIT_FRONT_AMBIENT)
		ret |= NV10_3D_COLOR_MATERIAL_AMBIENT;
	if (m & MAT_BIT_FRONT_DIFFUSE)
		ret |= NV10_3D_COLOR_MATERIAL_DIFFUSE;
	if (m & MAT_BIT_FRONT_SPECULAR)
		ret |= NV10_3D_COLOR_MATERIAL_SPECULAR;

	return ret;
}

void
nv10_emit_color_material(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	unsigned mask = get_material_bitmask(ctx->Light._ColorMaterialBitmask);

	BEGIN_NV04(push, NV10_3D(COLOR_MATERIAL), 1);
	PUSH_DATA (push, ctx->Light.ColorMaterialEnabled ? mask : 0);
}

static unsigned
get_fog_mode(unsigned mode)
{
	switch (mode) {
	case GL_LINEAR:
		return NV10_3D_FOG_MODE_LINEAR;
	case GL_EXP:
		return NV10_3D_FOG_MODE_EXP;
	case GL_EXP2:
		return NV10_3D_FOG_MODE_EXP2;
	default:
		assert(0);
	}
}

static unsigned
get_fog_source(unsigned source, unsigned distance_mode)
{
	switch (source) {
	case GL_FOG_COORDINATE_EXT:
		return NV10_3D_FOG_COORD_FOG;
	case GL_FRAGMENT_DEPTH_EXT:
		switch (distance_mode) {
		case GL_EYE_PLANE_ABSOLUTE_NV:
			return NV10_3D_FOG_COORD_DIST_ORTHOGONAL_ABS;
		case GL_EYE_PLANE:
			return NV10_3D_FOG_COORD_DIST_ORTHOGONAL;
		case GL_EYE_RADIAL_NV:
			return NV10_3D_FOG_COORD_DIST_RADIAL;
		default:
			assert(0);
		}
	default:
		assert(0);
	}
}

void
nv10_get_fog_coeff(struct gl_context *ctx, float k[3])
{
	struct gl_fog_attrib *f = &ctx->Fog;

	switch (f->Mode) {
	case GL_LINEAR:
		k[0] = 2 + f->Start / (f->End - f->Start);
		k[1] = -1 / (f->End - f->Start);
		break;

	case GL_EXP:
		k[0] = 1.5;
		k[1] = -0.09 * f->Density;
		break;

	case GL_EXP2:
		k[0] = 1.5;
		k[1] = -0.21 * f->Density;
		break;

	default:
		assert(0);
	}

	k[2] = 0;
}

void
nv10_emit_fog(struct gl_context *ctx, int emit)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	struct gl_fog_attrib *f = &ctx->Fog;
	unsigned source = nctx->fallback == HWTNL ?
		f->FogCoordinateSource : GL_FOG_COORDINATE_EXT;
	float k[3];

	nv10_get_fog_coeff(ctx, k);

	BEGIN_NV04(push, NV10_3D(FOG_MODE), 4);
	PUSH_DATA (push, get_fog_mode(f->Mode));
	PUSH_DATA (push, get_fog_source(source, f->FogDistanceMode));
	PUSH_DATAb(push, f->Enabled);
	PUSH_DATA (push, pack_rgba_f(MESA_FORMAT_RGBA8888_REV, f->Color));

	BEGIN_NV04(push, NV10_3D(FOG_COEFF(0)), 3);
	PUSH_DATAp(push, k, 3);

	context_dirty(ctx, FRAG);
}

static inline unsigned
get_light_mode(struct gl_light *l)
{
	if (l->Enabled) {
		if (l->_Flags & LIGHT_SPOT)
			return NV10_3D_ENABLED_LIGHTS_0_DIRECTIONAL;
		else if (l->_Flags & LIGHT_POSITIONAL)
			return NV10_3D_ENABLED_LIGHTS_0_POSITIONAL;
		else
			return NV10_3D_ENABLED_LIGHTS_0_NONPOSITIONAL;
	} else {
		return NV10_3D_ENABLED_LIGHTS_0_DISABLED;
	}
}

void
nv10_emit_light_enable(struct gl_context *ctx, int emit)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	uint32_t en_lights = 0;
	int i;

	if (nctx->fallback != HWTNL) {
		BEGIN_NV04(push, NV10_3D(LIGHTING_ENABLE), 1);
		PUSH_DATA (push, 0);
		return;
	}

	for (i = 0; i < MAX_LIGHTS; i++)
		en_lights |= get_light_mode(&ctx->Light.Light[i]) << 2 * i;

	BEGIN_NV04(push, NV10_3D(ENABLED_LIGHTS), 1);
	PUSH_DATA (push, en_lights);
	BEGIN_NV04(push, NV10_3D(LIGHTING_ENABLE), 1);
	PUSH_DATAb(push, ctx->Light.Enabled);
	BEGIN_NV04(push, NV10_3D(NORMALIZE_ENABLE), 1);
	PUSH_DATAb(push, ctx->Transform.Normalize);
}

void
nv10_emit_light_model(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	struct gl_lightmodel *m = &ctx->Light.Model;

	BEGIN_NV04(push, NV10_3D(SEPARATE_SPECULAR_ENABLE), 1);
	PUSH_DATAb(push, m->ColorControl == GL_SEPARATE_SPECULAR_COLOR);

	BEGIN_NV04(push, NV10_3D(LIGHT_MODEL), 1);
	PUSH_DATA (push, ((m->LocalViewer ?
			 NV10_3D_LIGHT_MODEL_LOCAL_VIEWER : 0) |
			(_mesa_need_secondary_color(ctx) ?
			 NV10_3D_LIGHT_MODEL_SEPARATE_SPECULAR : 0) |
			(!ctx->Light.Enabled && ctx->Fog.ColorSumEnabled ?
			 NV10_3D_LIGHT_MODEL_VERTEX_SPECULAR : 0)));
}

static float
get_shine(const float p[], float x)
{
	const int n = 15;
	const float *y = &p[1];
	float f = (n - 1) * (1 - 1 / (1 + p[0] * x))
		/ (1 - 1 / (1 + p[0] * 1024));
	int i = f;

	/* Linear interpolation in f-space (Faster and somewhat more
	 * accurate than x-space). */
	if (x == 0)
		return y[0];
	else if (i > n - 2)
		return y[n - 1];
	else
		return y[i] + (y[i + 1] - y[i]) * (f - i);
}

static const float nv10_spot_params[2][16] = {
	{ 0.02, -3.80e-05, -1.77, -2.41, -2.71, -2.88, -2.98, -3.06,
	  -3.11, -3.17, -3.23, -3.28, -3.37, -3.47, -3.83, -5.11 },
	{ 0.02, -0.01, 1.77, 2.39, 2.70, 2.87, 2.98, 3.06,
	  3.10, 3.16, 3.23, 3.27, 3.37, 3.47, 3.83, 5.11 },
};

void
nv10_get_spot_coeff(struct gl_light *l, float k[7])
{
	float e = l->SpotExponent;
	float a0, b0, a1, a2, b2, a3;

	if (e > 0)
		a0 = -1 - 5.36e-3 / sqrt(e);
	else
		a0 = -1;
	b0 = 1 / (1 + 0.273 * e);

	a1 = get_shine(nv10_spot_params[0], e);

	a2 = get_shine(nv10_spot_params[1], e);
	b2 = 1 / (1 + 0.273 * e);

	a3 = 0.9 + 0.278 * e;

	if (l->SpotCutoff > 0) {
		float cutoff = MAX2(a3, 1 / (1 - l->_CosCutoff));

		k[0] = MAX2(0, a0 + b0 * cutoff);
		k[1] = a1;
		k[2] = a2 + b2 * cutoff;
		k[3] = - cutoff * l->_NormSpotDirection[0];
		k[4] = - cutoff * l->_NormSpotDirection[1];
		k[5] = - cutoff * l->_NormSpotDirection[2];
		k[6] = 1 - cutoff;

	} else {
		k[0] = b0;
		k[1] = a1;
		k[2] = a2 + b2;
		k[3] = - l->_NormSpotDirection[0];
		k[4] = - l->_NormSpotDirection[1];
		k[5] = - l->_NormSpotDirection[2];
		k[6] = -1;
	}
}

void
nv10_emit_light_source(struct gl_context *ctx, int emit)
{
	const int i = emit - NOUVEAU_STATE_LIGHT_SOURCE0;
	struct nouveau_pushbuf *push = context_push(ctx);
	struct gl_light *l = &ctx->Light.Light[i];

	if (l->_Flags & LIGHT_POSITIONAL) {
		BEGIN_NV04(push, NV10_3D(LIGHT_POSITION_X(i)), 3);
		PUSH_DATAp(push, l->_Position, 3);

		BEGIN_NV04(push, NV10_3D(LIGHT_ATTENUATION_CONSTANT(i)), 3);
		PUSH_DATAf(push, l->ConstantAttenuation);
		PUSH_DATAf(push, l->LinearAttenuation);
		PUSH_DATAf(push, l->QuadraticAttenuation);

	} else {
		BEGIN_NV04(push, NV10_3D(LIGHT_DIRECTION_X(i)), 3);
		PUSH_DATAp(push, l->_VP_inf_norm, 3);

		BEGIN_NV04(push, NV10_3D(LIGHT_HALF_VECTOR_X(i)), 3);
		PUSH_DATAp(push, l->_h_inf_norm, 3);
	}

	if (l->_Flags & LIGHT_SPOT) {
		float k[7];

		nv10_get_spot_coeff(l, k);

		BEGIN_NV04(push, NV10_3D(LIGHT_SPOT_CUTOFF(i, 0)), 7);
		PUSH_DATAp(push, k, 7);
	}
}

#define USE_COLOR_MATERIAL(attr)					\
	(ctx->Light.ColorMaterialEnabled &&				\
	 ctx->Light._ColorMaterialBitmask & (1 << MAT_ATTRIB_FRONT_##attr))

void
nv10_emit_material_ambient(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	float (*mat)[4] = ctx->Light.Material.Attrib;
	float c_scene[3], c_factor[3];
	struct gl_light *l;

	if (USE_COLOR_MATERIAL(AMBIENT)) {
		COPY_3V(c_scene, ctx->Light.Model.Ambient);
		COPY_3V(c_factor, mat[MAT_ATTRIB_FRONT_EMISSION]);

	} else if (USE_COLOR_MATERIAL(EMISSION)) {
		SCALE_3V(c_scene, mat[MAT_ATTRIB_FRONT_AMBIENT],
			 ctx->Light.Model.Ambient);
		ZERO_3V(c_factor);

	} else {
		COPY_3V(c_scene, ctx->Light._BaseColor[0]);
		ZERO_3V(c_factor);
	}

	BEGIN_NV04(push, NV10_3D(LIGHT_MODEL_AMBIENT_R), 3);
	PUSH_DATAp(push, c_scene, 3);

	if (ctx->Light.ColorMaterialEnabled) {
		BEGIN_NV04(push, NV10_3D(MATERIAL_FACTOR_R), 3);
		PUSH_DATAp(push, c_factor, 3);
	}

	foreach(l, &ctx->Light.EnabledList) {
		const int i = l - ctx->Light.Light;
		float *c_light = (USE_COLOR_MATERIAL(AMBIENT) ?
				  l->Ambient :
				  l->_MatAmbient[0]);

		BEGIN_NV04(push, NV10_3D(LIGHT_AMBIENT_R(i)), 3);
		PUSH_DATAp(push, c_light, 3);
	}
}

void
nv10_emit_material_diffuse(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	GLfloat (*mat)[4] = ctx->Light.Material.Attrib;
	struct gl_light *l;

	BEGIN_NV04(push, NV10_3D(MATERIAL_FACTOR_A), 1);
	PUSH_DATAf(push, mat[MAT_ATTRIB_FRONT_DIFFUSE][3]);

	foreach(l, &ctx->Light.EnabledList) {
		const int i = l - ctx->Light.Light;
		float *c_light = (USE_COLOR_MATERIAL(DIFFUSE) ?
				  l->Diffuse :
				  l->_MatDiffuse[0]);

		BEGIN_NV04(push, NV10_3D(LIGHT_DIFFUSE_R(i)), 3);
		PUSH_DATAp(push, c_light, 3);
	}
}

void
nv10_emit_material_specular(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	struct gl_light *l;

	foreach(l, &ctx->Light.EnabledList) {
		const int i = l - ctx->Light.Light;
		float *c_light = (USE_COLOR_MATERIAL(SPECULAR) ?
				  l->Specular :
				  l->_MatSpecular[0]);

		BEGIN_NV04(push, NV10_3D(LIGHT_SPECULAR_R(i)), 3);
		PUSH_DATAp(push, c_light, 3);
	}
}

static const float nv10_shininess_param[6][16] = {
	{ 0.70, 0.00, 0.06, 0.06, 0.05, 0.04, 0.02, 0.00,
	  -0.06, -0.13, -0.24, -0.36, -0.51, -0.66, -0.82, -1.00 },
	{ 0.01, 1.00, -2.29, -2.77, -2.96, -3.06, -3.12, -3.18,
	  -3.24, -3.29, -3.36, -3.43, -3.51, -3.75, -4.33, -5.11 },
	{ 0.02, 0.00, 2.28, 2.75, 2.94, 3.04, 3.1, 3.15,
	  3.18, 3.22, 3.27, 3.32, 3.39, 3.48, 3.84, 5.11 },
	{ 0.70, 0.00, 0.05, 0.06, 0.06, 0.06, 0.05, 0.04,
	  0.02, 0.01, -0.03, -0.12, -0.25, -0.43, -0.68, -0.99 },
	{ 0.01, 1.00, -1.61, -2.35, -2.67, -2.84, -2.96, -3.05,
	  -3.08, -3.14, -3.2, -3.26, -3.32, -3.42, -3.54, -4.21 },
	{ 0.01, 0.00, 2.25, 2.73, 2.92, 3.03, 3.09, 3.15,
	  3.16, 3.21, 3.25, 3.29, 3.35, 3.43, 3.56, 4.22 },
};

void
nv10_get_shininess_coeff(float s, float k[6])
{
	int i;

	for (i = 0; i < 6; i++)
		k[i] = get_shine(nv10_shininess_param[i], s);
}

void
nv10_emit_material_shininess(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	float (*mat)[4] = ctx->Light.Material.Attrib;
	float k[6];

	nv10_get_shininess_coeff(
		CLAMP(mat[MAT_ATTRIB_FRONT_SHININESS][0], 0, 1024),
		k);

	BEGIN_NV04(push, NV10_3D(MATERIAL_SHININESS(0)), 6);
	PUSH_DATAp(push, k, 6);
}

void
nv10_emit_modelview(struct gl_context *ctx, int emit)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	GLmatrix *m = ctx->ModelviewMatrixStack.Top;

	if (nctx->fallback != HWTNL)
		return;

	if (ctx->Light._NeedEyeCoords || ctx->Fog.Enabled ||
	    (ctx->Texture._GenFlags & TEXGEN_NEED_EYE_COORD)) {
		BEGIN_NV04(push, NV10_3D(MODELVIEW_MATRIX(0, 0)), 16);
		PUSH_DATAm(push, m->m);
	}

	if (ctx->Light.Enabled ||
	    (ctx->Texture._GenFlags & TEXGEN_NEED_EYE_COORD)) {
		int i, j;

		BEGIN_NV04(push, NV10_3D(INVERSE_MODELVIEW_MATRIX(0, 0)), 12);
		for (i = 0; i < 3; i++)
			for (j = 0; j < 4; j++)
				PUSH_DATAf(push, m->inv[4*i + j]);
	}
}

void
nv10_emit_point_parameter(struct gl_context *ctx, int emit)
{
}

void
nv10_emit_projection(struct gl_context *ctx, int emit)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_pushbuf *push = context_push(ctx);
	GLmatrix m;

	_math_matrix_ctr(&m);
	get_viewport_scale(ctx, m.m);

	if (nv10_use_viewport_zclear(ctx))
		m.m[MAT_SZ] /= 8;

	if (nctx->fallback == HWTNL)
		_math_matrix_mul_matrix(&m, &m, &ctx->_ModelProjectMatrix);

	BEGIN_NV04(push, NV10_3D(PROJECTION_MATRIX(0)), 16);
	PUSH_DATAm(push, m.m);

	_math_matrix_dtr(&m);
}
