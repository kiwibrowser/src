/**********************************************************
 * Copyright 2009-2011 VMware, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *********************************************************
 * Authors:
 * Zack Rusin <zackr-at-vmware-dot-com>
 */
#include "xa_priv.h"

#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"

#include "util/u_memory.h"

#include "tgsi/tgsi_ureg.h"

#include "cso_cache/cso_context.h"
#include "cso_cache/cso_hash.h"

/* Vertex shader:
 * IN[0]    = vertex pos
 * IN[1]    = src tex coord | solid fill color
 * IN[2]    = mask tex coord
 * IN[3]    = dst tex coord
 * CONST[0] = (2/dst_width, 2/dst_height, 1, 1)
 * CONST[1] = (-1, -1, 0, 0)
 *
 * OUT[0]   = vertex pos
 * OUT[1]   = src tex coord | solid fill color
 * OUT[2]   = mask tex coord
 * OUT[3]   = dst tex coord
 */

/* Fragment shader:
 * SAMP[0]  = src
 * SAMP[1]  = mask
 * SAMP[2]  = dst
 * IN[0]    = pos src | solid fill color
 * IN[1]    = pos mask
 * IN[2]    = pos dst
 * CONST[0] = (0, 0, 0, 1)
 *
 * OUT[0] = color
 */

static void
print_fs_traits(int fs_traits)
{
    const char *strings[] = {
	"FS_COMPOSITE",		/* = 1 << 0, */
	"FS_MASK",		/* = 1 << 1, */
	"FS_SOLID_FILL",	/* = 1 << 2, */
	"FS_LINGRAD_FILL",	/* = 1 << 3, */
	"FS_RADGRAD_FILL",	/* = 1 << 4, */
	"FS_CA_FULL",		/* = 1 << 5, *//* src.rgba * mask.rgba */
	"FS_CA_SRCALPHA",	/* = 1 << 6, *//* src.aaaa * mask.rgba */
	"FS_YUV",		/* = 1 << 7, */
	"FS_SRC_REPEAT_NONE",	/* = 1 << 8, */
	"FS_MASK_REPEAT_NONE",	/* = 1 << 9, */
	"FS_SRC_SWIZZLE_RGB",	/* = 1 << 10, */
	"FS_MASK_SWIZZLE_RGB",	/* = 1 << 11, */
	"FS_SRC_SET_ALPHA",	/* = 1 << 12, */
	"FS_MASK_SET_ALPHA",	/* = 1 << 13, */
	"FS_SRC_LUMINANCE",	/* = 1 << 14, */
	"FS_MASK_LUMINANCE",	/* = 1 << 15, */
	"FS_DST_LUMINANCE",     /* = 1 << 15, */
    };
    int i, k;

    debug_printf("%s: ", __func__);

    for (i = 0, k = 1; k < (1 << 16); i++, k <<= 1) {
	if (fs_traits & k)
	    debug_printf("%s, ", strings[i]);
    }

    debug_printf("\n");
}

struct xa_shaders {
    struct xa_context *r;

    struct cso_hash *vs_hash;
    struct cso_hash *fs_hash;
};

static INLINE void
src_in_mask(struct ureg_program *ureg,
	    struct ureg_dst dst,
	    struct ureg_src src,
	    struct ureg_src mask,
	    unsigned component_alpha, unsigned mask_luminance)
{
    if (component_alpha == FS_CA_FULL) {
	ureg_MUL(ureg, dst, src, mask);
    } else if (component_alpha == FS_CA_SRCALPHA) {
	ureg_MUL(ureg, dst, ureg_scalar(src, TGSI_SWIZZLE_W), mask);
    } else {
	if (mask_luminance)
	    ureg_MUL(ureg, dst, src, ureg_scalar(mask, TGSI_SWIZZLE_X));
	else
	    ureg_MUL(ureg, dst, src, ureg_scalar(mask, TGSI_SWIZZLE_W));
    }
}

static struct ureg_src
vs_normalize_coords(struct ureg_program *ureg,
		    struct ureg_src coords,
		    struct ureg_src const0, struct ureg_src const1)
{
    struct ureg_dst tmp = ureg_DECL_temporary(ureg);
    struct ureg_src ret;

    ureg_MAD(ureg, tmp, coords, const0, const1);
    ret = ureg_src(tmp);
    ureg_release_temporary(ureg, tmp);
    return ret;
}

static void
linear_gradient(struct ureg_program *ureg,
		struct ureg_dst out,
		struct ureg_src pos,
		struct ureg_src sampler,
		struct ureg_src coords,
		struct ureg_src const0124,
		struct ureg_src matrow0,
		struct ureg_src matrow1, struct ureg_src matrow2)
{
    struct ureg_dst temp0 = ureg_DECL_temporary(ureg);
    struct ureg_dst temp1 = ureg_DECL_temporary(ureg);
    struct ureg_dst temp2 = ureg_DECL_temporary(ureg);
    struct ureg_dst temp3 = ureg_DECL_temporary(ureg);
    struct ureg_dst temp4 = ureg_DECL_temporary(ureg);
    struct ureg_dst temp5 = ureg_DECL_temporary(ureg);

    ureg_MOV(ureg, ureg_writemask(temp0, TGSI_WRITEMASK_XY), pos);
    ureg_MOV(ureg,
	     ureg_writemask(temp0, TGSI_WRITEMASK_Z),
	     ureg_scalar(const0124, TGSI_SWIZZLE_Y));

    ureg_DP3(ureg, temp1, matrow0, ureg_src(temp0));
    ureg_DP3(ureg, temp2, matrow1, ureg_src(temp0));
    ureg_DP3(ureg, temp3, matrow2, ureg_src(temp0));
    ureg_RCP(ureg, temp3, ureg_src(temp3));
    ureg_MUL(ureg, temp1, ureg_src(temp1), ureg_src(temp3));
    ureg_MUL(ureg, temp2, ureg_src(temp2), ureg_src(temp3));

    ureg_MOV(ureg, ureg_writemask(temp4, TGSI_WRITEMASK_X), ureg_src(temp1));
    ureg_MOV(ureg, ureg_writemask(temp4, TGSI_WRITEMASK_Y), ureg_src(temp2));

    ureg_MUL(ureg, temp0,
	     ureg_scalar(coords, TGSI_SWIZZLE_Y),
	     ureg_scalar(ureg_src(temp4), TGSI_SWIZZLE_Y));
    ureg_MAD(ureg, temp1,
	     ureg_scalar(coords, TGSI_SWIZZLE_X),
	     ureg_scalar(ureg_src(temp4), TGSI_SWIZZLE_X), ureg_src(temp0));

    ureg_MUL(ureg, temp2, ureg_src(temp1), ureg_scalar(coords, TGSI_SWIZZLE_Z));

    ureg_TEX(ureg, out, TGSI_TEXTURE_1D, ureg_src(temp2), sampler);

    ureg_release_temporary(ureg, temp0);
    ureg_release_temporary(ureg, temp1);
    ureg_release_temporary(ureg, temp2);
    ureg_release_temporary(ureg, temp3);
    ureg_release_temporary(ureg, temp4);
    ureg_release_temporary(ureg, temp5);
}

static void
radial_gradient(struct ureg_program *ureg,
		struct ureg_dst out,
		struct ureg_src pos,
		struct ureg_src sampler,
		struct ureg_src coords,
		struct ureg_src const0124,
		struct ureg_src matrow0,
		struct ureg_src matrow1, struct ureg_src matrow2)
{
    struct ureg_dst temp0 = ureg_DECL_temporary(ureg);
    struct ureg_dst temp1 = ureg_DECL_temporary(ureg);
    struct ureg_dst temp2 = ureg_DECL_temporary(ureg);
    struct ureg_dst temp3 = ureg_DECL_temporary(ureg);
    struct ureg_dst temp4 = ureg_DECL_temporary(ureg);
    struct ureg_dst temp5 = ureg_DECL_temporary(ureg);

    ureg_MOV(ureg, ureg_writemask(temp0, TGSI_WRITEMASK_XY), pos);
    ureg_MOV(ureg,
	     ureg_writemask(temp0, TGSI_WRITEMASK_Z),
	     ureg_scalar(const0124, TGSI_SWIZZLE_Y));

    ureg_DP3(ureg, temp1, matrow0, ureg_src(temp0));
    ureg_DP3(ureg, temp2, matrow1, ureg_src(temp0));
    ureg_DP3(ureg, temp3, matrow2, ureg_src(temp0));
    ureg_RCP(ureg, temp3, ureg_src(temp3));
    ureg_MUL(ureg, temp1, ureg_src(temp1), ureg_src(temp3));
    ureg_MUL(ureg, temp2, ureg_src(temp2), ureg_src(temp3));

    ureg_MOV(ureg, ureg_writemask(temp5, TGSI_WRITEMASK_X), ureg_src(temp1));
    ureg_MOV(ureg, ureg_writemask(temp5, TGSI_WRITEMASK_Y), ureg_src(temp2));

    ureg_MUL(ureg, temp0, ureg_scalar(coords, TGSI_SWIZZLE_Y),
	     ureg_scalar(ureg_src(temp5), TGSI_SWIZZLE_Y));
    ureg_MAD(ureg, temp1,
	     ureg_scalar(coords, TGSI_SWIZZLE_X),
	     ureg_scalar(ureg_src(temp5), TGSI_SWIZZLE_X), ureg_src(temp0));
    ureg_ADD(ureg, temp1, ureg_src(temp1), ureg_src(temp1));
    ureg_MUL(ureg, temp3,
	     ureg_scalar(ureg_src(temp5), TGSI_SWIZZLE_Y),
	     ureg_scalar(ureg_src(temp5), TGSI_SWIZZLE_Y));
    ureg_MAD(ureg, temp4,
	     ureg_scalar(ureg_src(temp5), TGSI_SWIZZLE_X),
	     ureg_scalar(ureg_src(temp5), TGSI_SWIZZLE_X), ureg_src(temp3));
    ureg_MOV(ureg, temp4, ureg_negate(ureg_src(temp4)));
    ureg_MUL(ureg, temp2, ureg_scalar(coords, TGSI_SWIZZLE_Z), ureg_src(temp4));
    ureg_MUL(ureg, temp0,
	     ureg_scalar(const0124, TGSI_SWIZZLE_W), ureg_src(temp2));
    ureg_MUL(ureg, temp3, ureg_src(temp1), ureg_src(temp1));
    ureg_SUB(ureg, temp2, ureg_src(temp3), ureg_src(temp0));
    ureg_RSQ(ureg, temp2, ureg_abs(ureg_src(temp2)));
    ureg_RCP(ureg, temp2, ureg_src(temp2));
    ureg_SUB(ureg, temp1, ureg_src(temp2), ureg_src(temp1));
    ureg_ADD(ureg, temp0,
	     ureg_scalar(coords, TGSI_SWIZZLE_Z),
	     ureg_scalar(coords, TGSI_SWIZZLE_Z));
    ureg_RCP(ureg, temp0, ureg_src(temp0));
    ureg_MUL(ureg, temp2, ureg_src(temp1), ureg_src(temp0));
    ureg_TEX(ureg, out, TGSI_TEXTURE_1D, ureg_src(temp2), sampler);

    ureg_release_temporary(ureg, temp0);
    ureg_release_temporary(ureg, temp1);
    ureg_release_temporary(ureg, temp2);
    ureg_release_temporary(ureg, temp3);
    ureg_release_temporary(ureg, temp4);
    ureg_release_temporary(ureg, temp5);
}

static void *
create_vs(struct pipe_context *pipe, unsigned vs_traits)
{
    struct ureg_program *ureg;
    struct ureg_src src;
    struct ureg_dst dst;
    struct ureg_src const0, const1;
    boolean is_fill = (vs_traits & VS_FILL) != 0;
    boolean is_composite = (vs_traits & VS_COMPOSITE) != 0;
    boolean has_mask = (vs_traits & VS_MASK) != 0;
    boolean is_yuv = (vs_traits & VS_YUV) != 0;
    unsigned input_slot = 0;

    ureg = ureg_create(TGSI_PROCESSOR_VERTEX);
    if (ureg == NULL)
	return 0;

    const0 = ureg_DECL_constant(ureg, 0);
    const1 = ureg_DECL_constant(ureg, 1);

    /* it has to be either a fill or a composite op */
    debug_assert((is_fill ^ is_composite) ^ is_yuv);

    src = ureg_DECL_vs_input(ureg, input_slot++);
    dst = ureg_DECL_output(ureg, TGSI_SEMANTIC_POSITION, 0);
    src = vs_normalize_coords(ureg, src, const0, const1);
    ureg_MOV(ureg, dst, src);

    if (is_yuv) {
	src = ureg_DECL_vs_input(ureg, input_slot++);
	dst = ureg_DECL_output(ureg, TGSI_SEMANTIC_GENERIC, 0);
	ureg_MOV(ureg, dst, src);
    }

    if (is_composite) {
	src = ureg_DECL_vs_input(ureg, input_slot++);
	dst = ureg_DECL_output(ureg, TGSI_SEMANTIC_GENERIC, 0);
	ureg_MOV(ureg, dst, src);
    }

    if (is_fill) {
	src = ureg_DECL_vs_input(ureg, input_slot++);
	dst = ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, 0);
	ureg_MOV(ureg, dst, src);
    }

    if (has_mask) {
	src = ureg_DECL_vs_input(ureg, input_slot++);
	dst = ureg_DECL_output(ureg, TGSI_SEMANTIC_GENERIC, 1);
	ureg_MOV(ureg, dst, src);
    }

    ureg_END(ureg);

    return ureg_create_shader_and_destroy(ureg, pipe);
}

static void *
create_yuv_shader(struct pipe_context *pipe, struct ureg_program *ureg)
{
    struct ureg_src y_sampler, u_sampler, v_sampler;
    struct ureg_src pos;
    struct ureg_src matrow0, matrow1, matrow2, matrow3;
    struct ureg_dst y, u, v, rgb;
    struct ureg_dst out = ureg_DECL_output(ureg,
					   TGSI_SEMANTIC_COLOR,
					   0);

    pos = ureg_DECL_fs_input(ureg,
			     TGSI_SEMANTIC_GENERIC, 0,
			     TGSI_INTERPOLATE_PERSPECTIVE);

    rgb = ureg_DECL_temporary(ureg);
    y = ureg_DECL_temporary(ureg);
    u = ureg_DECL_temporary(ureg);
    v = ureg_DECL_temporary(ureg);

    y_sampler = ureg_DECL_sampler(ureg, 0);
    u_sampler = ureg_DECL_sampler(ureg, 1);
    v_sampler = ureg_DECL_sampler(ureg, 2);

    matrow0 = ureg_DECL_constant(ureg, 0);
    matrow1 = ureg_DECL_constant(ureg, 1);
    matrow2 = ureg_DECL_constant(ureg, 2);
    matrow3 = ureg_DECL_constant(ureg, 3);

    ureg_TEX(ureg, y, TGSI_TEXTURE_2D, pos, y_sampler);
    ureg_TEX(ureg, u, TGSI_TEXTURE_2D, pos, u_sampler);
    ureg_TEX(ureg, v, TGSI_TEXTURE_2D, pos, v_sampler);

    ureg_MOV(ureg, rgb, matrow3);
    ureg_MAD(ureg, rgb,
	     ureg_scalar(ureg_src(y), TGSI_SWIZZLE_X), matrow0, ureg_src(rgb));
    ureg_MAD(ureg, rgb,
	     ureg_scalar(ureg_src(u), TGSI_SWIZZLE_X), matrow1, ureg_src(rgb));
    ureg_MAD(ureg, rgb,
	     ureg_scalar(ureg_src(v), TGSI_SWIZZLE_X), matrow2, ureg_src(rgb));

    ureg_MOV(ureg, out, ureg_src(rgb));

    ureg_release_temporary(ureg, rgb);
    ureg_release_temporary(ureg, y);
    ureg_release_temporary(ureg, u);
    ureg_release_temporary(ureg, v);

    ureg_END(ureg);

    return ureg_create_shader_and_destroy(ureg, pipe);
}

static INLINE void
xrender_tex(struct ureg_program *ureg,
	    struct ureg_dst dst,
	    struct ureg_src coords,
	    struct ureg_src sampler,
	    struct ureg_src imm0,
	    boolean repeat_none, boolean swizzle, boolean set_alpha)
{
    if (repeat_none) {
	struct ureg_dst tmp0 = ureg_DECL_temporary(ureg);
	struct ureg_dst tmp1 = ureg_DECL_temporary(ureg);

	ureg_SGT(ureg, tmp1, ureg_swizzle(coords,
					  TGSI_SWIZZLE_X,
					  TGSI_SWIZZLE_Y,
					  TGSI_SWIZZLE_X,
					  TGSI_SWIZZLE_Y), ureg_scalar(imm0,
								       TGSI_SWIZZLE_X));
	ureg_SLT(ureg, tmp0,
		 ureg_swizzle(coords, TGSI_SWIZZLE_X, TGSI_SWIZZLE_Y,
			      TGSI_SWIZZLE_X, TGSI_SWIZZLE_Y), ureg_scalar(imm0,
									   TGSI_SWIZZLE_W));
	ureg_MIN(ureg, tmp0, ureg_src(tmp0), ureg_src(tmp1));
	ureg_MIN(ureg, tmp0, ureg_scalar(ureg_src(tmp0), TGSI_SWIZZLE_X),
		 ureg_scalar(ureg_src(tmp0), TGSI_SWIZZLE_Y));
	ureg_TEX(ureg, tmp1, TGSI_TEXTURE_2D, coords, sampler);
	if (swizzle)
	    ureg_MOV(ureg, tmp1, ureg_swizzle(ureg_src(tmp1),
					      TGSI_SWIZZLE_Z,
					      TGSI_SWIZZLE_Y, TGSI_SWIZZLE_X,
					      TGSI_SWIZZLE_W));
	if (set_alpha)
	    ureg_MOV(ureg,
		     ureg_writemask(tmp1, TGSI_WRITEMASK_W),
		     ureg_scalar(imm0, TGSI_SWIZZLE_W));
	ureg_MUL(ureg, dst, ureg_src(tmp1), ureg_src(tmp0));
	ureg_release_temporary(ureg, tmp0);
	ureg_release_temporary(ureg, tmp1);
    } else {
	if (swizzle) {
	    struct ureg_dst tmp = ureg_DECL_temporary(ureg);

	    ureg_TEX(ureg, tmp, TGSI_TEXTURE_2D, coords, sampler);
	    ureg_MOV(ureg, dst, ureg_swizzle(ureg_src(tmp),
					     TGSI_SWIZZLE_Z,
					     TGSI_SWIZZLE_Y, TGSI_SWIZZLE_X,
					     TGSI_SWIZZLE_W));
	    ureg_release_temporary(ureg, tmp);
	} else {
	    ureg_TEX(ureg, dst, TGSI_TEXTURE_2D, coords, sampler);
	}
	if (set_alpha)
	    ureg_MOV(ureg,
		     ureg_writemask(dst, TGSI_WRITEMASK_W),
		     ureg_scalar(imm0, TGSI_SWIZZLE_W));
    }
}

static void *
create_fs(struct pipe_context *pipe, unsigned fs_traits)
{
    struct ureg_program *ureg;
    struct ureg_src /*dst_sampler, */ src_sampler, mask_sampler;
    struct ureg_src /*dst_pos, */ src_input, mask_pos;
    struct ureg_dst src, mask;
    struct ureg_dst out;
    struct ureg_src imm0 = { 0 };
    unsigned has_mask = (fs_traits & FS_MASK) != 0;
    unsigned is_fill = (fs_traits & FS_FILL) != 0;
    unsigned is_composite = (fs_traits & FS_COMPOSITE) != 0;
    unsigned is_solid = (fs_traits & FS_SOLID_FILL) != 0;
    unsigned is_lingrad = (fs_traits & FS_LINGRAD_FILL) != 0;
    unsigned is_radgrad = (fs_traits & FS_RADGRAD_FILL) != 0;
    unsigned comp_alpha_mask = fs_traits & FS_COMPONENT_ALPHA;
    unsigned is_yuv = (fs_traits & FS_YUV) != 0;
    unsigned src_repeat_none = (fs_traits & FS_SRC_REPEAT_NONE) != 0;
    unsigned mask_repeat_none = (fs_traits & FS_MASK_REPEAT_NONE) != 0;
    unsigned src_swizzle = (fs_traits & FS_SRC_SWIZZLE_RGB) != 0;
    unsigned mask_swizzle = (fs_traits & FS_MASK_SWIZZLE_RGB) != 0;
    unsigned src_set_alpha = (fs_traits & FS_SRC_SET_ALPHA) != 0;
    unsigned mask_set_alpha = (fs_traits & FS_MASK_SET_ALPHA) != 0;
    unsigned src_luminance = (fs_traits & FS_SRC_LUMINANCE) != 0;
    unsigned mask_luminance = (fs_traits & FS_MASK_LUMINANCE) != 0;
    unsigned dst_luminance = (fs_traits & FS_DST_LUMINANCE) != 0;

#if 0
    print_fs_traits(fs_traits);
#else
    (void)print_fs_traits;
#endif

    ureg = ureg_create(TGSI_PROCESSOR_FRAGMENT);
    if (ureg == NULL)
	return 0;

    /* it has to be either a fill, a composite op or a yuv conversion */
    debug_assert((is_fill ^ is_composite) ^ is_yuv);
    (void)is_yuv;

    out = ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, 0);

    if (src_repeat_none || mask_repeat_none ||
	src_set_alpha || mask_set_alpha || src_luminance) {
	imm0 = ureg_imm4f(ureg, 0, 0, 0, 1);
    }
    if (is_composite) {
	src_sampler = ureg_DECL_sampler(ureg, 0);
	src_input = ureg_DECL_fs_input(ureg,
				       TGSI_SEMANTIC_GENERIC, 0,
				       TGSI_INTERPOLATE_PERSPECTIVE);
    } else if (is_fill) {
	if (is_solid)
	    src_input = ureg_DECL_fs_input(ureg,
					   TGSI_SEMANTIC_COLOR, 0,
					   TGSI_INTERPOLATE_PERSPECTIVE);
	else
	    src_input = ureg_DECL_fs_input(ureg,
					   TGSI_SEMANTIC_POSITION, 0,
					   TGSI_INTERPOLATE_PERSPECTIVE);
    } else {
	debug_assert(is_yuv);
	return create_yuv_shader(pipe, ureg);
    }

    if (has_mask) {
	mask_sampler = ureg_DECL_sampler(ureg, 1);
	mask_pos = ureg_DECL_fs_input(ureg,
				      TGSI_SEMANTIC_GENERIC, 1,
				      TGSI_INTERPOLATE_PERSPECTIVE);
    }
#if 0				/* unused right now */
    dst_sampler = ureg_DECL_sampler(ureg, 2);
    dst_pos = ureg_DECL_fs_input(ureg,
				 TGSI_SEMANTIC_POSITION, 2,
				 TGSI_INTERPOLATE_PERSPECTIVE);
#endif

    if (is_composite) {
	if (has_mask || src_luminance || dst_luminance)
	    src = ureg_DECL_temporary(ureg);
	else
	    src = out;
	xrender_tex(ureg, src, src_input, src_sampler, imm0,
		    src_repeat_none, src_swizzle, src_set_alpha);
    } else if (is_fill) {
	if (is_solid) {
	    if (has_mask || src_luminance || dst_luminance)
		src = ureg_dst(src_input);
	    else
		ureg_MOV(ureg, out, src_input);
	} else if (is_lingrad || is_radgrad) {
	    struct ureg_src coords, const0124, matrow0, matrow1, matrow2;

	    if (has_mask || src_luminance || dst_luminance)
		src = ureg_DECL_temporary(ureg);
	    else
		src = out;

	    coords = ureg_DECL_constant(ureg, 0);
	    const0124 = ureg_DECL_constant(ureg, 1);
	    matrow0 = ureg_DECL_constant(ureg, 2);
	    matrow1 = ureg_DECL_constant(ureg, 3);
	    matrow2 = ureg_DECL_constant(ureg, 4);

	    if (is_lingrad) {
		linear_gradient(ureg, src,
				src_input, src_sampler,
				coords, const0124, matrow0, matrow1, matrow2);
	    } else if (is_radgrad) {
		radial_gradient(ureg, src,
				src_input, src_sampler,
				coords, const0124, matrow0, matrow1, matrow2);
	    }
	} else
	    debug_assert(!"Unknown fill type!");
    }
    if (src_luminance) {
	ureg_MOV(ureg, src, ureg_scalar(ureg_src(src), TGSI_SWIZZLE_X));
	ureg_MOV(ureg, ureg_writemask(src, TGSI_WRITEMASK_XYZ),
		 ureg_scalar(imm0, TGSI_SWIZZLE_X));
	if (!has_mask && !dst_luminance)
	    ureg_MOV(ureg, out, ureg_src(src));
    }

    if (has_mask) {
	mask = ureg_DECL_temporary(ureg);
	xrender_tex(ureg, mask, mask_pos, mask_sampler, imm0,
		    mask_repeat_none, mask_swizzle, mask_set_alpha);
	/* src IN mask */

	src_in_mask(ureg, (dst_luminance) ? src : out, ureg_src(src),
		    ureg_src(mask),
		    comp_alpha_mask, mask_luminance);

	ureg_release_temporary(ureg, mask);
    }

    if (dst_luminance) {
	/*
	 * Make sure the alpha channel goes into the output L8 surface.
	 */
	ureg_MOV(ureg, out, ureg_scalar(ureg_src(src), TGSI_SWIZZLE_W));
    }

    ureg_END(ureg);

    return ureg_create_shader_and_destroy(ureg, pipe);
}

struct xa_shaders *
xa_shaders_create(struct xa_context *r)
{
    struct xa_shaders *sc = CALLOC_STRUCT(xa_shaders);

    sc->r = r;
    sc->vs_hash = cso_hash_create();
    sc->fs_hash = cso_hash_create();

    return sc;
}

static void
cache_destroy(struct cso_context *cso,
	      struct cso_hash *hash, unsigned processor)
{
    struct cso_hash_iter iter = cso_hash_first_node(hash);

    while (!cso_hash_iter_is_null(iter)) {
	void *shader = (void *)cso_hash_iter_data(iter);

	if (processor == PIPE_SHADER_FRAGMENT) {
	    cso_delete_fragment_shader(cso, shader);
	} else if (processor == PIPE_SHADER_VERTEX) {
	    cso_delete_vertex_shader(cso, shader);
	}
	iter = cso_hash_erase(hash, iter);
    }
    cso_hash_delete(hash);
}

void
xa_shaders_destroy(struct xa_shaders *sc)
{
    cache_destroy(sc->r->cso, sc->vs_hash, PIPE_SHADER_VERTEX);
    cache_destroy(sc->r->cso, sc->fs_hash, PIPE_SHADER_FRAGMENT);

    FREE(sc);
}

static INLINE void *
shader_from_cache(struct pipe_context *pipe,
		  unsigned type, struct cso_hash *hash, unsigned key)
{
    void *shader = 0;

    struct cso_hash_iter iter = cso_hash_find(hash, key);

    if (cso_hash_iter_is_null(iter)) {
	if (type == PIPE_SHADER_VERTEX)
	    shader = create_vs(pipe, key);
	else
	    shader = create_fs(pipe, key);
	cso_hash_insert(hash, key, shader);
    } else
	shader = (void *)cso_hash_iter_data(iter);

    return shader;
}

struct xa_shader
xa_shaders_get(struct xa_shaders *sc, unsigned vs_traits, unsigned fs_traits)
{
    struct xa_shader shader = { NULL, NULL };
    void *vs, *fs;

    vs = shader_from_cache(sc->r->pipe, PIPE_SHADER_VERTEX,
			   sc->vs_hash, vs_traits);
    fs = shader_from_cache(sc->r->pipe, PIPE_SHADER_FRAGMENT,
			   sc->fs_hash, fs_traits);

    debug_assert(vs && fs);
    if (!vs || !fs)
	return shader;

    shader.vs = vs;
    shader.fs = fs;

    return shader;
}
