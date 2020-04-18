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

#include "nouveau_driver.h"
#include "nouveau_context.h"
#include "nouveau_util.h"
#include "nv_object.xml.h"
#include "nv04_3d.xml.h"
#include "nv04_driver.h"

#define COMBINER_SHIFT(in)						\
	(NV04_MULTITEX_TRIANGLE_COMBINE_COLOR_ARGUMENT##in##__SHIFT	\
	 - NV04_MULTITEX_TRIANGLE_COMBINE_COLOR_ARGUMENT0__SHIFT)
#define COMBINER_SOURCE(reg)					\
	NV04_MULTITEX_TRIANGLE_COMBINE_COLOR_ARGUMENT0_##reg
#define COMBINER_INVERT					\
	NV04_MULTITEX_TRIANGLE_COMBINE_COLOR_INVERSE0
#define COMBINER_ALPHA					\
	NV04_MULTITEX_TRIANGLE_COMBINE_COLOR_ALPHA0

struct combiner_state {
	struct gl_context *ctx;
	int unit;
	GLboolean alpha;
	GLboolean premodulate;

	/* GL state */
	GLenum mode;
	GLenum *source;
	GLenum *operand;
	GLuint logscale;

	/* Derived HW state */
	uint32_t hw;
};

#define __INIT_COMBINER_ALPHA_A GL_TRUE
#define __INIT_COMBINER_ALPHA_RGB GL_FALSE

/* Initialize a combiner_state struct from the texture unit
 * context. */
#define INIT_COMBINER(chan, ctx, rc, i) do {			\
		struct gl_tex_env_combine_state *c =		\
			ctx->Texture.Unit[i]._CurrentCombine;	\
		(rc)->ctx = ctx;				\
		(rc)->unit = i;					\
		(rc)->alpha = __INIT_COMBINER_ALPHA_##chan;	\
		(rc)->premodulate = c->_NumArgs##chan == 4;	\
		(rc)->mode = c->Mode##chan;			\
		(rc)->source = c->Source##chan;			\
		(rc)->operand = c->Operand##chan;		\
		(rc)->logscale = c->ScaleShift##chan;		\
		(rc)->hw = 0;					\
	} while (0)

/* Get the combiner source for the specified EXT_texture_env_combine
 * source. */
static uint32_t
get_input_source(struct combiner_state *rc, int source)
{
	switch (source) {
	case GL_ZERO:
		return COMBINER_SOURCE(ZERO);

	case GL_TEXTURE:
		return rc->unit ? COMBINER_SOURCE(TEXTURE1) :
			COMBINER_SOURCE(TEXTURE0);

	case GL_TEXTURE0:
		return COMBINER_SOURCE(TEXTURE0);

	case GL_TEXTURE1:
		return COMBINER_SOURCE(TEXTURE1);

	case GL_CONSTANT:
		return COMBINER_SOURCE(CONSTANT);

	case GL_PRIMARY_COLOR:
		return COMBINER_SOURCE(PRIMARY_COLOR);

	case GL_PREVIOUS:
		return rc->unit ? COMBINER_SOURCE(PREVIOUS) :
			COMBINER_SOURCE(PRIMARY_COLOR);

	default:
		assert(0);
	}
}

/* Get the (possibly inverted) combiner input mapping for the
 * specified EXT_texture_env_combine operand. */
#define INVERT 0x1

static uint32_t
get_input_mapping(struct combiner_state *rc, int operand, int flags)
{
	int map = 0;

	if (!is_color_operand(operand) && !rc->alpha)
		map |= COMBINER_ALPHA;

	if (is_negative_operand(operand) == !(flags & INVERT))
		map |= COMBINER_INVERT;

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
				return COMBINER_SOURCE(ZERO) |
					get_input_mapping(rc, operand, flags);

		} else if (format == MESA_FORMAT_L8) {
			/* Emulated using I8. */
			if (!is_color_operand(operand))
				return COMBINER_SOURCE(ZERO) |
					get_input_mapping(rc, operand,
							  flags ^ INVERT);
		}
	}

	return get_input_source(rc, source) |
		get_input_mapping(rc, operand, flags);
}

/* Bind the combiner input <in> to the combiner source <src>,
 * possibly inverted. */
#define INPUT_SRC(rc, in, src, flags)					\
	(rc)->hw |= ((flags & INVERT ? COMBINER_INVERT : 0) |		\
		   COMBINER_SOURCE(src)) << COMBINER_SHIFT(in)

/* Bind the combiner input <in> to the EXT_texture_env_combine
 * argument <arg>, possibly inverted. */
#define INPUT_ARG(rc, in, arg, flags)					\
	(rc)->hw |= get_input_arg(rc, arg, flags) << COMBINER_SHIFT(in)

#define UNSIGNED_OP(rc)							\
	(rc)->hw |= ((rc)->logscale ?					\
		     NV04_MULTITEX_TRIANGLE_COMBINE_COLOR_MAP_SCALE2 :	\
		     NV04_MULTITEX_TRIANGLE_COMBINE_COLOR_MAP_IDENTITY)
#define SIGNED_OP(rc)							\
	(rc)->hw |= ((rc)->logscale ?					\
		     NV04_MULTITEX_TRIANGLE_COMBINE_COLOR_MAP_BIAS_SCALE2 : \
		     NV04_MULTITEX_TRIANGLE_COMBINE_COLOR_MAP_BIAS)

static void
setup_combiner(struct combiner_state *rc)
{
	switch (rc->mode) {
	case GL_REPLACE:
		INPUT_ARG(rc, 0, 0, 0);
		INPUT_SRC(rc, 1, ZERO, INVERT);
		INPUT_SRC(rc, 2, ZERO, 0);
		INPUT_SRC(rc, 3, ZERO, 0);
		UNSIGNED_OP(rc);
		break;

	case GL_MODULATE:
		INPUT_ARG(rc, 0, 0, 0);
		INPUT_ARG(rc, 1, 1, 0);
		INPUT_SRC(rc, 2, ZERO, 0);
		INPUT_SRC(rc, 3, ZERO, 0);
		UNSIGNED_OP(rc);
		break;

	case GL_ADD:
	case GL_ADD_SIGNED:
		if (rc->premodulate) {
			INPUT_ARG(rc, 0, 0, 0);
			INPUT_ARG(rc, 1, 1, 0);
			INPUT_ARG(rc, 2, 2, 0);
			INPUT_ARG(rc, 3, 3, 0);
		} else {
			INPUT_ARG(rc, 0, 0, 0);
			INPUT_SRC(rc, 1, ZERO, INVERT);
			INPUT_ARG(rc, 2, 1, 0);
			INPUT_SRC(rc, 3, ZERO, INVERT);
		}

		if (rc->mode == GL_ADD_SIGNED)
			SIGNED_OP(rc);
		else
			UNSIGNED_OP(rc);

		break;

	case GL_INTERPOLATE:
		INPUT_ARG(rc, 0, 0, 0);
		INPUT_ARG(rc, 1, 2, 0);
		INPUT_ARG(rc, 2, 1, 0);
		INPUT_ARG(rc, 3, 2, INVERT);
		UNSIGNED_OP(rc);
		break;

	default:
		assert(0);
	}
}

static unsigned
get_texenv_mode(unsigned mode)
{
	switch (mode) {
	case GL_REPLACE:
		return 0x1;
	case GL_DECAL:
		return 0x3;
	case GL_MODULATE:
		return 0x4;
	default:
		assert(0);
	}
}

void
nv04_emit_tex_env(struct gl_context *ctx, int emit)
{
	struct nv04_context *nv04 = to_nv04_context(ctx);
	const int i = emit - NOUVEAU_STATE_TEX_ENV0;
	struct combiner_state rc_a = {}, rc_c = {};

	/* Compute the new combiner state. */
	if (ctx->Texture.Unit[i]._ReallyEnabled) {
		INIT_COMBINER(A, ctx, &rc_a, i);
		setup_combiner(&rc_a);

		INIT_COMBINER(RGB, ctx, &rc_c, i);
		setup_combiner(&rc_c);

	} else {
		if (i == 0) {
			INPUT_SRC(&rc_a, 0, PRIMARY_COLOR, 0);
			INPUT_SRC(&rc_c, 0, PRIMARY_COLOR, 0);
		} else {
			INPUT_SRC(&rc_a, 0, PREVIOUS, 0);
			INPUT_SRC(&rc_c, 0, PREVIOUS, 0);
		}

		INPUT_SRC(&rc_a, 1, ZERO, INVERT);
		INPUT_SRC(&rc_c, 1, ZERO, INVERT);
		INPUT_SRC(&rc_a, 2, ZERO, 0);
		INPUT_SRC(&rc_c, 2, ZERO, 0);
		INPUT_SRC(&rc_a, 3, ZERO, 0);
		INPUT_SRC(&rc_c, 3, ZERO, 0);

		UNSIGNED_OP(&rc_a);
		UNSIGNED_OP(&rc_c);
	}

	/* calculate non-multitex state */
	nv04->blend &= ~NV04_TEXTURED_TRIANGLE_BLEND_TEXTURE_MAP__MASK;
	if (ctx->Texture._EnabledUnits)
		nv04->blend |= get_texenv_mode(ctx->Texture.Unit[0].EnvMode);
	else
		nv04->blend |= get_texenv_mode(GL_MODULATE);

	/* update calculated multitex state */
	nv04->alpha[i] = rc_a.hw;
	nv04->color[i] = rc_c.hw;
	nv04->factor   = pack_rgba_f(MESA_FORMAT_ARGB8888,
				     ctx->Texture.Unit[0].EnvColor);
}
