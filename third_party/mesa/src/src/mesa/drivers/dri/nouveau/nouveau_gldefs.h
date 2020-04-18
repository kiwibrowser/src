/*
 * Copyright (C) 2007-2010 The Nouveau Project.
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

#ifndef __NOUVEAU_GLDEFS_H__
#define __NOUVEAU_GLDEFS_H__

static inline unsigned
nvgl_blend_func(unsigned func)
{
	switch (func) {
	case GL_ZERO:
		return 0x0000;
	case GL_ONE:
		return 0x0001;
	case GL_SRC_COLOR:
		return 0x0300;
	case GL_ONE_MINUS_SRC_COLOR:
		return 0x0301;
	case GL_SRC_ALPHA:
		return 0x0302;
	case GL_ONE_MINUS_SRC_ALPHA:
		return 0x0303;
	case GL_DST_ALPHA:
		return 0x0304;
	case GL_ONE_MINUS_DST_ALPHA:
		return 0x0305;
	case GL_DST_COLOR:
		return 0x0306;
	case GL_ONE_MINUS_DST_COLOR:
		return 0x0307;
	case GL_SRC_ALPHA_SATURATE:
		return 0x0308;
	case GL_CONSTANT_COLOR:
		return 0x8001;
	case GL_ONE_MINUS_CONSTANT_COLOR:
		return 0x8002;
	case GL_CONSTANT_ALPHA:
		return 0x8003;
	case GL_ONE_MINUS_CONSTANT_ALPHA:
		return 0x8004;
	default:
		assert(0);
	}
}

static inline unsigned
nvgl_blend_eqn(unsigned eqn)
{
	switch (eqn) {
	case GL_FUNC_ADD:
		return 0x8006;
	case GL_MIN:
		return 0x8007;
	case GL_MAX:
		return 0x8008;
	case GL_FUNC_SUBTRACT:
		return 0x800a;
	case GL_FUNC_REVERSE_SUBTRACT:
		return 0x800b;
	default:
		assert(0);
	}
}

static inline unsigned
nvgl_logicop_func(unsigned func)
{
	switch (func) {
	case GL_CLEAR:
		return 0x1500;
	case GL_NOR:
		return 0x1508;
	case GL_AND_INVERTED:
		return 0x1504;
	case GL_COPY_INVERTED:
		return 0x150c;
	case GL_AND_REVERSE:
		return 0x1502;
	case GL_INVERT:
		return 0x150a;
	case GL_XOR:
		return 0x1506;
	case GL_NAND:
		return 0x150e;
	case GL_AND:
		return 0x1501;
	case GL_EQUIV:
		return 0x1509;
	case GL_NOOP:
		return 0x1505;
	case GL_OR_INVERTED:
		return 0x150d;
	case GL_COPY:
		return 0x1503;
	case GL_OR_REVERSE:
		return 0x150b;
	case GL_OR:
		return 0x1507;
	case GL_SET:
		return 0x150f;
	default:
		assert(0);
	}
}

static inline unsigned
nvgl_comparison_op(unsigned op)
{
	switch (op) {
	case GL_NEVER:
		return 0x0200;
	case GL_LESS:
		return 0x0201;
	case GL_EQUAL:
		return 0x0202;
	case GL_LEQUAL:
		return 0x0203;
	case GL_GREATER:
		return 0x0204;
	case GL_NOTEQUAL:
		return 0x0205;
	case GL_GEQUAL:
		return 0x0206;
	case GL_ALWAYS:
		return 0x0207;
	default:
		assert(0);
	}
}

static inline unsigned
nvgl_polygon_mode(unsigned mode)
{
	switch (mode) {
	case GL_POINT:
		return 0x1b00;
	case GL_LINE:
		return 0x1b01;
	case GL_FILL:
		return 0x1b02;
	default:
		assert(0);
	}
}

static inline unsigned
nvgl_stencil_op(unsigned op)
{
	switch (op) {
	case GL_ZERO:
		return 0x0000;
	case GL_INVERT:
		return 0x150a;
	case GL_KEEP:
		return 0x1e00;
	case GL_REPLACE:
		return 0x1e01;
	case GL_INCR:
		return 0x1e02;
	case GL_DECR:
		return 0x1e03;
	case GL_INCR_WRAP_EXT:
		return 0x8507;
	case GL_DECR_WRAP_EXT:
		return 0x8508;
	default:
		assert(0);
	}
}

static inline unsigned
nvgl_primitive(unsigned prim)
{
	switch (prim) {
	case GL_POINTS:
		return 0x0001;
	case GL_LINES:
		return 0x0002;
	case GL_LINE_LOOP:
		return 0x0003;
	case GL_LINE_STRIP:
		return 0x0004;
	case GL_TRIANGLES:
		return 0x0005;
	case GL_TRIANGLE_STRIP:
		return 0x0006;
	case GL_TRIANGLE_FAN:
		return 0x0007;
	case GL_QUADS:
		return 0x0008;
	case GL_QUAD_STRIP:
		return 0x0009;
	case GL_POLYGON:
		return 0x000a;
	default:
		assert(0);
	}
}

static inline unsigned
nvgl_wrap_mode(unsigned wrap)
{
	switch (wrap) {
	case GL_REPEAT:
		return 0x1;
	case GL_MIRRORED_REPEAT:
		return 0x2;
	case GL_CLAMP:
	case GL_CLAMP_TO_EDGE:
		return 0x3;
	case GL_CLAMP_TO_BORDER:
		return 0x4;
	default:
		assert(0);
	}
}

static inline unsigned
nvgl_filter_mode(unsigned filter)
{
	switch (filter) {
	case GL_NEAREST:
		return 0x1;
	case GL_LINEAR:
		return 0x2;
	case GL_NEAREST_MIPMAP_NEAREST:
		return 0x3;
	case GL_LINEAR_MIPMAP_NEAREST:
		return 0x4;
	case GL_NEAREST_MIPMAP_LINEAR:
		return 0x5;
	case GL_LINEAR_MIPMAP_LINEAR:
		return 0x6;
	default:
		assert(0);
	}
}

static inline unsigned
nvgl_texgen_mode(unsigned mode)
{
	switch (mode) {
	case GL_EYE_LINEAR:
		return 0x2400;
	case GL_OBJECT_LINEAR:
		return 0x2401;
	case GL_SPHERE_MAP:
		return 0x2402;
	case GL_NORMAL_MAP:
		return 0x8511;
	case GL_REFLECTION_MAP:
		return 0x8512;
	default:
		assert(0);
	}
}

#endif
