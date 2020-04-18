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
#include "nv10_3d.xml.h"
#include "nouveau_util.h"
#include "nv10_driver.h"
#include "nv20_driver.h"

#define RC_IN_SHIFT_A	24
#define RC_IN_SHIFT_B	16
#define RC_IN_SHIFT_C	8
#define RC_IN_SHIFT_D	0
#define RC_IN_SHIFT_E	56
#define RC_IN_SHIFT_F	48
#define RC_IN_SHIFT_G	40

#define RC_IN_SOURCE(source)				\
	((uint64_t)NV10_3D_RC_IN_RGB_D_INPUT_##source)
#define RC_IN_USAGE(usage)					\
	((uint64_t)NV10_3D_RC_IN_RGB_D_COMPONENT_USAGE_##usage)
#define RC_IN_MAPPING(mapping)					\
	((uint64_t)NV10_3D_RC_IN_RGB_D_MAPPING_##mapping)

#define RC_OUT_BIAS	NV10_3D_RC_OUT_RGB_BIAS_BIAS_BY_NEGATIVE_ONE_HALF
#define RC_OUT_SCALE_1	NV10_3D_RC_OUT_RGB_SCALE_NONE
#define RC_OUT_SCALE_2	NV10_3D_RC_OUT_RGB_SCALE_SCALE_BY_TWO
#define RC_OUT_SCALE_4	NV10_3D_RC_OUT_RGB_SCALE_SCALE_BY_FOUR

/* Make the combiner do: spare0_i = A_i * B_i */
#define RC_OUT_AB	NV10_3D_RC_OUT_RGB_AB_OUTPUT_SPARE0
/* spare0_i = dot3(A, B) */
#define RC_OUT_DOT_AB	(NV10_3D_RC_OUT_RGB_AB_OUTPUT_SPARE0 |	\
			 NV10_3D_RC_OUT_RGB_AB_DOT_PRODUCT)
/* spare0_i = A_i * B_i + C_i * D_i */
#define RC_OUT_SUM	NV10_3D_RC_OUT_RGB_SUM_OUTPUT_SPARE0

struct combiner_state {
	struct gl_context *ctx;
	int unit;
	GLboolean premodulate;

	/* GL state */
	GLenum mode;
	GLenum *source;
	GLenum *operand;
	GLuint logscale;

	/* Derived HW state */
	uint64_t in;
	uint32_t out;
};

/* Initialize a combiner_state struct from the texture unit
 * context. */
#define INIT_COMBINER(chan, ctx, rc, i) do {			\
		struct gl_tex_env_combine_state *c =		\
			ctx->Texture.Unit[i]._CurrentCombine;	\
		(rc)->ctx = ctx;				\
		(rc)->unit = i;					\
		(rc)->premodulate = c->_NumArgs##chan == 4;	\
		(rc)->mode = c->Mode##chan;			\
		(rc)->source = c->Source##chan;			\
		(rc)->operand = c->Operand##chan;		\
		(rc)->logscale = c->ScaleShift##chan;		\
		(rc)->in = (rc)->out = 0;			\
	} while (0)

/* Get the RC input source for the specified EXT_texture_env_combine
 * source. */
static uint32_t
get_input_source(struct combiner_state *rc, int source)
{
	switch (source) {
	case GL_ZERO:
		return RC_IN_SOURCE(ZERO);

	case GL_TEXTURE:
		return RC_IN_SOURCE(TEXTURE0) + rc->unit;

	case GL_TEXTURE0:
		return RC_IN_SOURCE(TEXTURE0);

	case GL_TEXTURE1:
		return RC_IN_SOURCE(TEXTURE1);

	case GL_TEXTURE2:
		return RC_IN_SOURCE(TEXTURE2);

	case GL_TEXTURE3:
		return RC_IN_SOURCE(TEXTURE3);

	case GL_CONSTANT:
		return context_chipset(rc->ctx) >= 0x20 ?
			RC_IN_SOURCE(CONSTANT_COLOR0) :
			RC_IN_SOURCE(CONSTANT_COLOR0) + rc->unit;

	case GL_PRIMARY_COLOR:
		return RC_IN_SOURCE(PRIMARY_COLOR);

	case GL_PREVIOUS:
		return rc->unit ? RC_IN_SOURCE(SPARE0)
			: RC_IN_SOURCE(PRIMARY_COLOR);

	default:
		assert(0);
	}
}

/* Get the RC input mapping for the specified texture_env_combine
 * operand, possibly inverted or biased. */
#define INVERT 0x1
#define HALF_BIAS 0x2

static uint32_t
get_input_mapping(struct combiner_state *rc, int operand, int flags)
{
	int map = 0;

	if (is_color_operand(operand))
		map |= RC_IN_USAGE(RGB);
	else
		map |= RC_IN_USAGE(ALPHA);

	if (is_negative_operand(operand) == !(flags & INVERT))
		map |= flags & HALF_BIAS ?
			RC_IN_MAPPING(HALF_BIAS_NEGATE) :
			RC_IN_MAPPING(UNSIGNED_INVERT);
	else
		map |= flags & HALF_BIAS ?
			RC_IN_MAPPING(HALF_BIAS_NORMAL) :
			RC_IN_MAPPING(UNSIGNED_IDENTITY);

	return map;
}

static uint32_t
get_input_arg(struct combiner_state *rc, int arg, int flags)
{
	int source = rc->source[arg];
	int operand = rc->operand[arg];

	/* Fake several unsupported texture formats. */
	if (is_texture_source(source)) {
		int i = (source == GL_TEXTURE ?
			 rc->unit : source - GL_TEXTURE0);
		struct gl_texture_object *t = rc->ctx->Texture.Unit[i]._Current;
		gl_format format = t->Image[0][t->BaseLevel]->TexFormat;

		if (format == MESA_FORMAT_A8) {
			/* Emulated using I8. */
			if (is_color_operand(operand))
				return RC_IN_SOURCE(ZERO) |
					get_input_mapping(rc, operand, flags);

		} else if (format == MESA_FORMAT_L8) {
			/* Sometimes emulated using I8. */
			if (!is_color_operand(operand))
				return RC_IN_SOURCE(ZERO) |
					get_input_mapping(rc, operand,
							  flags ^ INVERT);

		} else if (format == MESA_FORMAT_XRGB8888) {
			/* Sometimes emulated using ARGB8888. */
			if (!is_color_operand(operand))
				return RC_IN_SOURCE(ZERO) |
					get_input_mapping(rc, operand,
							  flags ^ INVERT);
		}
	}

	return get_input_source(rc, source) |
		get_input_mapping(rc, operand, flags);
}

/* Bind the RC input variable <var> to the EXT_texture_env_combine
 * argument <arg>, possibly inverted or biased. */
#define INPUT_ARG(rc, var, arg, flags)					\
	(rc)->in |= get_input_arg(rc, arg, flags) << RC_IN_SHIFT_##var

/* Bind the RC input variable <var> to the RC source <src>. */
#define INPUT_SRC(rc, var, src, chan)					\
	(rc)->in |= (RC_IN_SOURCE(src) |				\
		     RC_IN_USAGE(chan)) << RC_IN_SHIFT_##var

/* Bind the RC input variable <var> to a constant +/-1 */
#define INPUT_ONE(rc, var, flags)					\
	(rc)->in |= (RC_IN_SOURCE(ZERO) |				\
		     (flags & INVERT ? RC_IN_MAPPING(EXPAND_NORMAL) :	\
		      RC_IN_MAPPING(UNSIGNED_INVERT))) << RC_IN_SHIFT_##var

static void
setup_combiner(struct combiner_state *rc)
{
	switch (rc->mode) {
	case GL_REPLACE:
		INPUT_ARG(rc, A, 0, 0);
		INPUT_ONE(rc, B, 0);

		rc->out = RC_OUT_AB;
		break;

	case GL_MODULATE:
		INPUT_ARG(rc, A, 0, 0);
		INPUT_ARG(rc, B, 1, 0);

		rc->out = RC_OUT_AB;
		break;

	case GL_ADD:
	case GL_ADD_SIGNED:
		if (rc->premodulate) {
			INPUT_ARG(rc, A, 0, 0);
			INPUT_ARG(rc, B, 1, 0);
			INPUT_ARG(rc, C, 2, 0);
			INPUT_ARG(rc, D, 3, 0);
		} else {
			INPUT_ARG(rc, A, 0, 0);
			INPUT_ONE(rc, B, 0);
			INPUT_ARG(rc, C, 1, 0);
			INPUT_ONE(rc, D, 0);
		}

		rc->out = RC_OUT_SUM |
			(rc->mode == GL_ADD_SIGNED ? RC_OUT_BIAS : 0);
		break;

	case GL_INTERPOLATE:
		INPUT_ARG(rc, A, 0, 0);
		INPUT_ARG(rc, B, 2, 0);
		INPUT_ARG(rc, C, 1, 0);
		INPUT_ARG(rc, D, 2, INVERT);

		rc->out = RC_OUT_SUM;
		break;

	case GL_SUBTRACT:
		INPUT_ARG(rc, A, 0, 0);
		INPUT_ONE(rc, B, 0);
		INPUT_ARG(rc, C, 1, 0);
		INPUT_ONE(rc, D, INVERT);

		rc->out = RC_OUT_SUM;
		break;

	case GL_DOT3_RGB:
	case GL_DOT3_RGBA:
		INPUT_ARG(rc, A, 0, HALF_BIAS);
		INPUT_ARG(rc, B, 1, HALF_BIAS);

		rc->out = RC_OUT_DOT_AB | RC_OUT_SCALE_4;

		assert(!rc->logscale);
		break;

	default:
		assert(0);
	}

	switch (rc->logscale) {
	case 0:
		rc->out |= RC_OUT_SCALE_1;
		break;
	case 1:
		rc->out |= RC_OUT_SCALE_2;
		break;
	case 2:
		rc->out |= RC_OUT_SCALE_4;
		break;
	default:
		assert(0);
	}
}

void
nv10_get_general_combiner(struct gl_context *ctx, int i,
			  uint32_t *a_in, uint32_t *a_out,
			  uint32_t *c_in, uint32_t *c_out, uint32_t *k)
{
	struct combiner_state rc_a, rc_c;

	if (ctx->Texture.Unit[i]._ReallyEnabled) {
		INIT_COMBINER(RGB, ctx, &rc_c, i);

		if (rc_c.mode == GL_DOT3_RGBA)
			rc_a = rc_c;
		else
			INIT_COMBINER(A, ctx, &rc_a, i);

		setup_combiner(&rc_c);
		setup_combiner(&rc_a);

	} else {
		rc_a.in = rc_a.out = rc_c.in = rc_c.out = 0;
	}

	*k = pack_rgba_f(MESA_FORMAT_ARGB8888,
			 ctx->Texture.Unit[i].EnvColor);
	*a_in = rc_a.in;
	*a_out = rc_a.out;
	*c_in = rc_c.in;
	*c_out = rc_c.out;
}

void
nv10_get_final_combiner(struct gl_context *ctx, uint64_t *in, int *n)
{
	struct combiner_state rc = {};

	/*
	 * The final fragment value equation is something like:
	 *	x_i = A_i * B_i + (1 - A_i) * C_i + D_i
	 *	x_alpha = G_alpha
	 * where D_i = E_i * F_i, i one of {red, green, blue}.
	 */
	if (ctx->Fog.ColorSumEnabled || ctx->Light.Enabled) {
		INPUT_SRC(&rc, D, E_TIMES_F, RGB);
		INPUT_SRC(&rc, F, SECONDARY_COLOR, RGB);
	}

	if (ctx->Fog.Enabled) {
		INPUT_SRC(&rc, A, FOG, ALPHA);
		INPUT_SRC(&rc, C, FOG, RGB);
		INPUT_SRC(&rc, E, FOG, ALPHA);
	} else {
		INPUT_ONE(&rc, A, 0);
		INPUT_ONE(&rc, C, 0);
		INPUT_ONE(&rc, E, 0);
	}

	if (ctx->Texture._EnabledUnits) {
		INPUT_SRC(&rc, B, SPARE0, RGB);
		INPUT_SRC(&rc, G, SPARE0, ALPHA);
	} else {
		INPUT_SRC(&rc, B, PRIMARY_COLOR, RGB);
		INPUT_SRC(&rc, G, PRIMARY_COLOR, ALPHA);
	}

	*in = rc.in;
	*n = log2i(ctx->Texture._EnabledUnits) + 1;
}

void
nv10_emit_tex_env(struct gl_context *ctx, int emit)
{
	const int i = emit - NOUVEAU_STATE_TEX_ENV0;
	struct nouveau_pushbuf *push = context_push(ctx);
	uint32_t a_in, a_out, c_in, c_out, k;

	nv10_get_general_combiner(ctx, i, &a_in, &a_out, &c_in, &c_out, &k);

	/* Enable the combiners we're going to need. */
	if (i == 1) {
		if (c_out || a_out)
			c_out |= 0x5 << 27;
		else
			c_out |= 0x3 << 27;
	}

	BEGIN_NV04(push, NV10_3D(RC_IN_ALPHA(i)), 1);
	PUSH_DATA (push, a_in);
	BEGIN_NV04(push, NV10_3D(RC_IN_RGB(i)), 1);
	PUSH_DATA (push, c_in);
	BEGIN_NV04(push, NV10_3D(RC_COLOR(i)), 1);
	PUSH_DATA (push, k);
	BEGIN_NV04(push, NV10_3D(RC_OUT_ALPHA(i)), 1);
	PUSH_DATA (push, a_out);
	BEGIN_NV04(push, NV10_3D(RC_OUT_RGB(i)), 1);
	PUSH_DATA (push, c_out);

	context_dirty(ctx, FRAG);
}

void
nv10_emit_frag(struct gl_context *ctx, int emit)
{
	struct nouveau_pushbuf *push = context_push(ctx);
	uint64_t in;
	int n;

	nv10_get_final_combiner(ctx, &in, &n);

	BEGIN_NV04(push, NV10_3D(RC_FINAL0), 2);
	PUSH_DATA (push, in);
	PUSH_DATA (push, in >> 32);
}
