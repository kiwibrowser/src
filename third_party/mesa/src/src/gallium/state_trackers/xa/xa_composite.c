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
 * Thomas Hellstrom <thellstrom-at-vmware-dot-com>
 */

#include "xa_composite.h"
#include "xa_context.h"
#include "xa_priv.h"
#include "cso_cache/cso_context.h"
#include "util/u_sampler.h"
#include "util/u_inlines.h"


/*XXX also in Xrender.h but the including it here breaks compilition */
#define XFixedToDouble(f)    (((double) (f)) / 65536.)

struct xa_composite_blend {
    unsigned op : 8;

    unsigned alpha_dst : 4;
    unsigned alpha_src : 4;

    unsigned rgb_src : 8;    /**< PIPE_BLENDFACTOR_x */
    unsigned rgb_dst : 8;    /**< PIPE_BLENDFACTOR_x */
};

#define XA_BLEND_OP_OVER 3
static const struct xa_composite_blend xa_blends[] = {
    { xa_op_clear,
      0, 0, PIPE_BLENDFACTOR_ZERO, PIPE_BLENDFACTOR_ZERO},
    { xa_op_src,
      0, 0, PIPE_BLENDFACTOR_ONE, PIPE_BLENDFACTOR_ZERO},
    { xa_op_dst,
      0, 0, PIPE_BLENDFACTOR_ZERO, PIPE_BLENDFACTOR_ONE},
    { xa_op_over,
      0, 1, PIPE_BLENDFACTOR_ONE, PIPE_BLENDFACTOR_INV_SRC_ALPHA},
    { xa_op_over_reverse,
      1, 0, PIPE_BLENDFACTOR_INV_DST_ALPHA, PIPE_BLENDFACTOR_ONE},
    { xa_op_in,
      1, 0, PIPE_BLENDFACTOR_DST_ALPHA, PIPE_BLENDFACTOR_ZERO},
    { xa_op_in_reverse,
      0, 1, PIPE_BLENDFACTOR_ZERO, PIPE_BLENDFACTOR_SRC_ALPHA},
    { xa_op_out,
      1, 0, PIPE_BLENDFACTOR_INV_DST_ALPHA, PIPE_BLENDFACTOR_ZERO},
    { xa_op_out_reverse,
      0, 1, PIPE_BLENDFACTOR_ZERO, PIPE_BLENDFACTOR_INV_SRC_ALPHA},
    { xa_op_atop,
      1, 1, PIPE_BLENDFACTOR_DST_ALPHA, PIPE_BLENDFACTOR_INV_SRC_ALPHA},
    { xa_op_atop_reverse,
      1, 1, PIPE_BLENDFACTOR_INV_DST_ALPHA, PIPE_BLENDFACTOR_SRC_ALPHA},
    { xa_op_xor,
      1, 1, PIPE_BLENDFACTOR_INV_DST_ALPHA, PIPE_BLENDFACTOR_INV_SRC_ALPHA},
    { xa_op_add,
      0, 0, PIPE_BLENDFACTOR_ONE, PIPE_BLENDFACTOR_ONE},
};


/*
 * The alpha value stored in a luminance texture is read by the
 * hardware as color.
 */
static unsigned
xa_convert_blend_for_luminance(unsigned factor)
{
    switch(factor) {
    case PIPE_BLENDFACTOR_DST_ALPHA:
	return PIPE_BLENDFACTOR_DST_COLOR;
    case PIPE_BLENDFACTOR_INV_DST_ALPHA:
	return PIPE_BLENDFACTOR_INV_DST_COLOR;
    default:
	break;
    }
    return factor;
}


static boolean
blend_for_op(struct xa_composite_blend *blend,
	     enum xa_composite_op op,
	     struct xa_picture *src_pic,
	     struct xa_picture *mask_pic,
	     struct xa_picture *dst_pic)
{
    const int num_blends =
	sizeof(xa_blends)/sizeof(struct xa_composite_blend);
    int i;
    boolean supported = FALSE;

    /*
     * Temporarily disable component alpha since it appears buggy.
     */
    if (mask_pic && mask_pic->component_alpha)
	return FALSE;

    /*
     * our default in case something goes wrong
     */
    *blend = xa_blends[XA_BLEND_OP_OVER];

    for (i = 0; i < num_blends; ++i) {
	if (xa_blends[i].op == op) {
	    *blend = xa_blends[i];
	    supported = TRUE;
	}
    }

    if (!dst_pic->srf)
	return supported;

    if (dst_pic->srf->tex->format == PIPE_FORMAT_L8_UNORM) {
	blend->rgb_src = xa_convert_blend_for_luminance(blend->rgb_src);
	blend->rgb_dst = xa_convert_blend_for_luminance(blend->rgb_dst);
    }

    /*
     * If there's no dst alpha channel, adjust the blend op so that we'll treat
     * it as always 1.
     */

    if (xa_format_a(dst_pic->pict_format) == 0 && blend->alpha_dst) {
	if (blend->rgb_src == PIPE_BLENDFACTOR_DST_ALPHA)
	    blend->rgb_src = PIPE_BLENDFACTOR_ONE;
	else if (blend->rgb_src == PIPE_BLENDFACTOR_INV_DST_ALPHA)
	    blend->rgb_src = PIPE_BLENDFACTOR_ZERO;
    }

    /*
     * If the source alpha is being used, then we should only be in a case where
     * the source blend factor is 0, and the source blend value is the mask
     * channels multiplied by the source picture's alpha.
     */
    if (mask_pic && mask_pic->component_alpha &&
	xa_format_rgb(mask_pic->pict_format) &&
	blend->alpha_src) {
	if (blend->rgb_dst == PIPE_BLENDFACTOR_SRC_ALPHA) {
	    blend->rgb_dst = PIPE_BLENDFACTOR_SRC_COLOR;
	} else if (blend->rgb_dst == PIPE_BLENDFACTOR_INV_SRC_ALPHA) {
	    blend->rgb_dst = PIPE_BLENDFACTOR_INV_SRC_COLOR;
	}
    }

    return supported;
}


static INLINE int
xa_repeat_to_gallium(int mode)
{
    switch(mode) {
    case xa_wrap_clamp_to_border:
	return PIPE_TEX_WRAP_CLAMP_TO_BORDER;
    case xa_wrap_repeat:
	return PIPE_TEX_WRAP_REPEAT;
    case xa_wrap_mirror_repeat:
	return PIPE_TEX_WRAP_MIRROR_REPEAT;
    case xa_wrap_clamp_to_edge:
	return PIPE_TEX_WRAP_CLAMP_TO_EDGE;
    default:
	break;
    }
    return PIPE_TEX_WRAP_REPEAT;
}

static INLINE boolean
xa_filter_to_gallium(int xrender_filter, int *out_filter)
{

    switch (xrender_filter) {
    case xa_filter_nearest:
	*out_filter = PIPE_TEX_FILTER_NEAREST;
	break;
    case xa_filter_linear:
	*out_filter = PIPE_TEX_FILTER_LINEAR;
	break;
    default:
	*out_filter = PIPE_TEX_FILTER_NEAREST;
	return FALSE;
    }
    return TRUE;
}

static int
xa_is_filter_accelerated(struct xa_picture *pic)
{
    int filter;
    if (pic && !xa_filter_to_gallium(pic->filter, &filter))
	return 0;
    return 1;
}

XA_EXPORT int
xa_composite_check_accelerated(const struct xa_composite *comp)
{
    struct xa_composite_blend blend;
    struct xa_picture *src_pic = comp->src;

    if (!xa_is_filter_accelerated(src_pic) ||
	!xa_is_filter_accelerated(comp->mask)) {
	return -XA_ERR_INVAL;
    }


    if (src_pic->src_pict) {
	if (src_pic->src_pict->type != xa_src_pict_solid_fill)
	    return -XA_ERR_INVAL;

	/*
	 * Currently we don't support solid fill with a mask.
	 * We can easily do that, but that would require shader,
	 * sampler view setup and vertex setup modification.
	 */
	if (comp->mask)
	    return -XA_ERR_INVAL;
    }

    if (blend_for_op(&blend, comp->op, comp->src, comp->mask, comp->dst)) {
	struct xa_picture *mask = comp->mask;
	if (mask && mask->component_alpha &&
	    xa_format_rgb(mask->pict_format)) {
	    if (blend.alpha_src && blend.rgb_src != PIPE_BLENDFACTOR_ZERO) {
		return -XA_ERR_INVAL;
	    }
	}

	return XA_ERR_NONE;
    }
    return -XA_ERR_INVAL;
}

static int
bind_composite_blend_state(struct xa_context *ctx,
			   const struct xa_composite *comp)
{
    struct xa_composite_blend blend_opt;
    struct pipe_blend_state blend;

    if (!blend_for_op(&blend_opt, comp->op, comp->src, comp->mask, comp->dst))
	return -XA_ERR_INVAL;

    memset(&blend, 0, sizeof(struct pipe_blend_state));
    blend.rt[0].blend_enable = 1;
    blend.rt[0].colormask = PIPE_MASK_RGBA;

    blend.rt[0].rgb_src_factor   = blend_opt.rgb_src;
    blend.rt[0].alpha_src_factor = blend_opt.rgb_src;
    blend.rt[0].rgb_dst_factor   = blend_opt.rgb_dst;
    blend.rt[0].alpha_dst_factor = blend_opt.rgb_dst;

    cso_set_blend(ctx->cso, &blend);
    return XA_ERR_NONE;
}

static unsigned int
picture_format_fixups(struct xa_picture *src_pic,
		      int mask)
{
    boolean set_alpha = FALSE;
    boolean swizzle = FALSE;
    unsigned ret = 0;
    struct xa_surface *src = src_pic->srf;
    enum xa_formats src_hw_format, src_pic_format;
    enum xa_surface_type src_hw_type, src_pic_type;

    if (!src)
	return 0;

    src_hw_format = xa_surface_format(src);
    src_pic_format = src_pic->pict_format;

    set_alpha = (xa_format_type_is_color(src_pic_format) &&
		 xa_format_a(src_pic_format) == 0);

    if (set_alpha)
	ret |= mask ? FS_MASK_SET_ALPHA : FS_SRC_SET_ALPHA;

    if (src_hw_format == src_pic_format) {
	if (src->tex->format == PIPE_FORMAT_L8_UNORM)
	    return ((mask) ? FS_MASK_LUMINANCE : FS_SRC_LUMINANCE);

	return ret;
    }

    src_hw_type = xa_format_type(src_hw_format);
    src_pic_type = xa_format_type(src_pic_format);

    swizzle = ((src_hw_type == xa_type_argb &&
		src_pic_type == xa_type_abgr) ||
	       ((src_hw_type == xa_type_abgr &&
		 src_pic_type == xa_type_argb)));

    if (!swizzle && (src_hw_type != src_pic_type))
      return ret;

    if (swizzle)
	ret |= mask ? FS_MASK_SWIZZLE_RGB : FS_SRC_SWIZZLE_RGB;

    return ret;
}

static int
bind_shaders(struct xa_context *ctx, const struct xa_composite *comp)
{
    unsigned vs_traits = 0, fs_traits = 0;
    struct xa_shader shader;
    struct xa_picture *src_pic = comp->src;
    struct xa_picture *mask_pic = comp->mask;

    ctx->has_solid_color = FALSE;

    if (src_pic) {
	if (src_pic->wrap == xa_wrap_clamp_to_border && src_pic->has_transform)
	    fs_traits |= FS_SRC_REPEAT_NONE;

	if (src_pic->src_pict) {
	    if (src_pic->src_pict->type == xa_src_pict_solid_fill) {
		fs_traits |= FS_SOLID_FILL | FS_FILL;
		vs_traits |= VS_SOLID_FILL;
		xa_pixel_to_float4(src_pic->src_pict->solid_fill.color,
				   ctx->solid_color);
		ctx->has_solid_color = TRUE;
	    }
	} else {
	    fs_traits |= FS_COMPOSITE;
	    vs_traits |= VS_COMPOSITE;
	}

	fs_traits |= picture_format_fixups(src_pic, 0);
    }

    if (mask_pic) {
	vs_traits |= VS_MASK;
	fs_traits |= FS_MASK;
	if (mask_pic->wrap == xa_wrap_clamp_to_border &&
	    mask_pic->has_transform)
	    fs_traits |= FS_MASK_REPEAT_NONE;

	if (mask_pic->component_alpha) {
	    struct xa_composite_blend blend;
	    if (!blend_for_op(&blend, comp->op, src_pic, mask_pic, NULL))
		return -XA_ERR_INVAL;

	    if (blend.alpha_src) {
		fs_traits |= FS_CA_SRCALPHA;
	    } else
		fs_traits |= FS_CA_FULL;
	}

	fs_traits |= picture_format_fixups(mask_pic, 1);
    }

    if (ctx->srf->format == PIPE_FORMAT_L8_UNORM)
	fs_traits |= FS_DST_LUMINANCE;

    shader = xa_shaders_get(ctx->shaders, vs_traits, fs_traits);
    cso_set_vertex_shader_handle(ctx->cso, shader.vs);
    cso_set_fragment_shader_handle(ctx->cso, shader.fs);
    return XA_ERR_NONE;
}

static void
bind_samplers(struct xa_context *ctx,
	      const struct xa_composite *comp)
{
    struct pipe_sampler_state *samplers[PIPE_MAX_SAMPLERS];
    struct pipe_sampler_state src_sampler, mask_sampler;
    struct pipe_sampler_view view_templ;
    struct pipe_sampler_view *src_view;
    struct pipe_context *pipe = ctx->pipe;
    struct xa_picture *src_pic = comp->src;
    struct xa_picture *mask_pic = comp->mask;

    ctx->num_bound_samplers = 0;

    memset(&src_sampler, 0, sizeof(struct pipe_sampler_state));
    memset(&mask_sampler, 0, sizeof(struct pipe_sampler_state));

    if (src_pic) {
	if (ctx->has_solid_color) {
	    samplers[0] = NULL;
	    pipe_sampler_view_reference(&ctx->bound_sampler_views[0], NULL);
	} else {
	    unsigned src_wrap = xa_repeat_to_gallium(src_pic->wrap);
	    int filter;

	    (void) xa_filter_to_gallium(src_pic->filter, &filter);

	    src_sampler.wrap_s = src_wrap;
	    src_sampler.wrap_t = src_wrap;
	    src_sampler.min_img_filter = filter;
	    src_sampler.mag_img_filter = filter;
	    src_sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST;
	    src_sampler.normalized_coords = 1;
	    samplers[0] = &src_sampler;
	    ctx->num_bound_samplers = 1;
	    u_sampler_view_default_template(&view_templ,
					    src_pic->srf->tex,
					    src_pic->srf->tex->format);
	    src_view = pipe->create_sampler_view(pipe, src_pic->srf->tex,
						 &view_templ);
	    pipe_sampler_view_reference(&ctx->bound_sampler_views[0], NULL);
	    ctx->bound_sampler_views[0] = src_view;
	}
    }

    if (mask_pic) {
	unsigned mask_wrap = xa_repeat_to_gallium(mask_pic->wrap);
	int filter;

	(void) xa_filter_to_gallium(mask_pic->filter, &filter);

	mask_sampler.wrap_s = mask_wrap;
	mask_sampler.wrap_t = mask_wrap;
	mask_sampler.min_img_filter = filter;
	mask_sampler.mag_img_filter = filter;
	src_sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST;
	mask_sampler.normalized_coords = 1;
	samplers[1] = &mask_sampler;
	ctx->num_bound_samplers = 2;
	u_sampler_view_default_template(&view_templ,
					mask_pic->srf->tex,
					mask_pic->srf->tex->format);
	src_view = pipe->create_sampler_view(pipe, mask_pic->srf->tex,
					     &view_templ);
	pipe_sampler_view_reference(&ctx->bound_sampler_views[1], NULL);
	ctx->bound_sampler_views[1] = src_view;


	/*
	 * If src is a solid color, we have no src view, so set up a
	 * dummy one that will not be used anyway.
	 */
	if (ctx->bound_sampler_views[0] == NULL)
	    pipe_sampler_view_reference(&ctx->bound_sampler_views[0],
					src_view);

    }

    cso_set_samplers(ctx->cso, PIPE_SHADER_FRAGMENT, ctx->num_bound_samplers,
		     (const struct pipe_sampler_state **)samplers);
    cso_set_sampler_views(ctx->cso, PIPE_SHADER_FRAGMENT, ctx->num_bound_samplers,
				   ctx->bound_sampler_views);
}

XA_EXPORT int
xa_composite_prepare(struct xa_context *ctx,
		     const struct xa_composite *comp)
{
    struct xa_surface *dst_srf = comp->dst->srf;
    int ret;

    ret = xa_ctx_srf_create(ctx, dst_srf);
    if (ret != XA_ERR_NONE)
	return ret;

    ctx->dst = dst_srf;
    renderer_bind_destination(ctx, ctx->srf, ctx->srf->width,
			      ctx->srf->height);

    ret = bind_composite_blend_state(ctx, comp);
    if (ret != XA_ERR_NONE)
	return ret;
    ret = bind_shaders(ctx, comp);
    if (ret != XA_ERR_NONE)
	return ret;
    bind_samplers(ctx, comp);

    if (ctx->num_bound_samplers == 0 ) { /* solid fill */
	renderer_begin_solid(ctx);
    } else {
	renderer_begin_textures(ctx);
	ctx->comp = comp;
    }

    xa_ctx_srf_destroy(ctx);
    return XA_ERR_NONE;
}

XA_EXPORT void
xa_composite_rect(struct xa_context *ctx,
		  int srcX, int srcY, int maskX, int maskY,
		  int dstX, int dstY, int width, int height)
{
    if (ctx->num_bound_samplers == 0 ) { /* solid fill */
	renderer_solid(ctx, dstX, dstY, dstX + width, dstY + height,
		       ctx->solid_color);
    } else {
	const struct xa_composite *comp = ctx->comp;
	int pos[6] = {srcX, srcY, maskX, maskY, dstX, dstY};
	const float *src_matrix = NULL;
	const float *mask_matrix = NULL;

	if (comp->src->has_transform)
	    src_matrix = comp->src->transform;
	if (comp->mask && comp->mask->has_transform)
	    mask_matrix = comp->mask->transform;

	renderer_texture(ctx, pos, width, height,
			 src_matrix, mask_matrix);
    }
}

XA_EXPORT void
xa_composite_done(struct xa_context *ctx)
{
    renderer_draw_flush(ctx);
    ctx->pipe->flush(ctx->pipe, &ctx->last_fence);

    ctx->comp = NULL;
    ctx->has_solid_color = FALSE;
    xa_ctx_sampler_views_destroy(ctx);
}

static const struct xa_composite_allocation a = {
    .xa_composite_size = sizeof(struct xa_composite),
    .xa_picture_size = sizeof(struct xa_picture),
    .xa_source_pict_size = sizeof(union xa_source_pict),
};

XA_EXPORT const struct xa_composite_allocation *
xa_composite_allocation(void)
{
    return &a;
}
