/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
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
 */
#include "r600_pipe.h"
#include "util/u_surface.h"
#include "util/u_blitter.h"
#include "util/u_format.h"

enum r600_blitter_op /* bitmask */
{
	R600_SAVE_FRAGMENT_STATE = 1,
	R600_SAVE_TEXTURES       = 2,
	R600_SAVE_FRAMEBUFFER    = 4,
	R600_DISABLE_RENDER_COND = 8,

	R600_CLEAR         = R600_SAVE_FRAGMENT_STATE,

	R600_CLEAR_SURFACE = R600_SAVE_FRAGMENT_STATE | R600_SAVE_FRAMEBUFFER,

	R600_COPY_BUFFER   = R600_DISABLE_RENDER_COND,

	R600_COPY_TEXTURE  = R600_SAVE_FRAGMENT_STATE | R600_SAVE_FRAMEBUFFER | R600_SAVE_TEXTURES |
			     R600_DISABLE_RENDER_COND,

	R600_DECOMPRESS    = R600_SAVE_FRAGMENT_STATE | R600_SAVE_FRAMEBUFFER | R600_DISABLE_RENDER_COND,

	R600_COLOR_RESOLVE = R600_SAVE_FRAGMENT_STATE | R600_SAVE_FRAMEBUFFER | R600_DISABLE_RENDER_COND
};

static void r600_blitter_begin(struct pipe_context *ctx, enum r600_blitter_op op)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	r600_suspend_nontimer_queries(rctx);

	util_blitter_save_vertex_buffers(rctx->blitter,
					 util_last_bit(rctx->vertex_buffer_state.enabled_mask),
					 rctx->vertex_buffer_state.vb);
	util_blitter_save_vertex_elements(rctx->blitter, rctx->vertex_elements);
	util_blitter_save_vertex_shader(rctx->blitter, rctx->vs_shader);
	util_blitter_save_so_targets(rctx->blitter, rctx->num_so_targets,
				     (struct pipe_stream_output_target**)rctx->so_targets);
	util_blitter_save_rasterizer(rctx->blitter, rctx->states[R600_PIPE_STATE_RASTERIZER]);

	if (op & R600_SAVE_FRAGMENT_STATE) {
		if (rctx->states[R600_PIPE_STATE_VIEWPORT]) {
			util_blitter_save_viewport(rctx->blitter, &rctx->viewport);
		}
		util_blitter_save_fragment_shader(rctx->blitter, rctx->ps_shader);
		util_blitter_save_blend(rctx->blitter, rctx->states[R600_PIPE_STATE_BLEND]);
		util_blitter_save_depth_stencil_alpha(rctx->blitter, rctx->states[R600_PIPE_STATE_DSA]);
		if (rctx->states[R600_PIPE_STATE_STENCIL_REF]) {
			util_blitter_save_stencil_ref(rctx->blitter, &rctx->stencil_ref);
		}
                util_blitter_save_sample_mask(rctx->blitter, rctx->sample_mask.sample_mask);
	}

	if (op & R600_SAVE_FRAMEBUFFER)
		util_blitter_save_framebuffer(rctx->blitter, &rctx->framebuffer);

	if (op & R600_SAVE_TEXTURES) {
		util_blitter_save_fragment_sampler_states(
			rctx->blitter, rctx->ps_samplers.n_samplers,
			(void**)rctx->ps_samplers.samplers);

		util_blitter_save_fragment_sampler_views(
			rctx->blitter, util_last_bit(rctx->ps_samplers.views.enabled_mask),
			(struct pipe_sampler_view**)rctx->ps_samplers.views.views);
	}

	if ((op & R600_DISABLE_RENDER_COND) && rctx->current_render_cond) {
		rctx->saved_render_cond = rctx->current_render_cond;
		rctx->saved_render_cond_mode = rctx->current_render_cond_mode;
		rctx->context.render_condition(&rctx->context, NULL, 0);
	}

}

static void r600_blitter_end(struct pipe_context *ctx)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	if (rctx->saved_render_cond) {
		rctx->context.render_condition(&rctx->context,
					       rctx->saved_render_cond,
					       rctx->saved_render_cond_mode);
		rctx->saved_render_cond = NULL;
	}
	r600_resume_nontimer_queries(rctx);
}

static unsigned u_max_layer(struct pipe_resource *r, unsigned level)
{
	switch (r->target) {
	case PIPE_TEXTURE_CUBE:
		return 6 - 1;
	case PIPE_TEXTURE_3D:
		return u_minify(r->depth0, level) - 1;
	case PIPE_TEXTURE_1D_ARRAY:
	case PIPE_TEXTURE_2D_ARRAY:
		return r->array_size - 1;
	default:
		return 0;
	}
}

static unsigned u_max_sample(struct pipe_resource *r)
{
	return r->nr_samples ? r->nr_samples - 1 : 0;
}

void r600_blit_decompress_depth(struct pipe_context *ctx,
		struct r600_texture *texture,
		struct r600_texture *staging,
		unsigned first_level, unsigned last_level,
		unsigned first_layer, unsigned last_layer,
		unsigned first_sample, unsigned last_sample)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	unsigned layer, level, sample, checked_last_layer, max_layer, max_sample;
	struct r600_texture *flushed_depth_texture = staging ?
			staging : texture->flushed_depth_texture;
	const struct util_format_description *desc =
		util_format_description(texture->resource.b.b.format);
	float depth;

	if (!staging && !texture->dirty_level_mask)
		return;

	max_sample = u_max_sample(&texture->resource.b.b);

	/* XXX Decompressing MSAA depth textures is broken on R6xx.
	 * There is also a hardlock if CMASK and FMASK are not present.
	 * Just skip this until we find out how to fix it. */
	if (rctx->chip_class == R600 && max_sample > 0) {
		texture->dirty_level_mask = 0;
		return;
	}

	if (rctx->family == CHIP_RV610 || rctx->family == CHIP_RV630 ||
	    rctx->family == CHIP_RV620 || rctx->family == CHIP_RV635)
		depth = 0.0f;
	else
		depth = 1.0f;

	/* Enable decompression in DB_RENDER_CONTROL */
	rctx->db_misc_state.flush_depthstencil_through_cb = true;
	rctx->db_misc_state.copy_depth = util_format_has_depth(desc);
	rctx->db_misc_state.copy_stencil = util_format_has_stencil(desc);
	rctx->db_misc_state.copy_sample = first_sample;
	r600_atom_dirty(rctx, &rctx->db_misc_state.atom);


	for (level = first_level; level <= last_level; level++) {
		if (!staging && !(texture->dirty_level_mask & (1 << level)))
			continue;

		/* The smaller the mipmap level, the less layers there are
		 * as far as 3D textures are concerned. */
		max_layer = u_max_layer(&texture->resource.b.b, level);
		checked_last_layer = last_layer < max_layer ? last_layer : max_layer;

		for (layer = first_layer; layer <= checked_last_layer; layer++) {
			for (sample = first_sample; sample <= last_sample; sample++) {
				struct pipe_surface *zsurf, *cbsurf, surf_tmpl;

				if (sample != rctx->db_misc_state.copy_sample) {
					rctx->db_misc_state.copy_sample = sample;
					r600_atom_dirty(rctx, &rctx->db_misc_state.atom);
				}

				surf_tmpl.format = texture->resource.b.b.format;
				surf_tmpl.u.tex.level = level;
				surf_tmpl.u.tex.first_layer = layer;
				surf_tmpl.u.tex.last_layer = layer;
				surf_tmpl.usage = PIPE_BIND_DEPTH_STENCIL;

				zsurf = ctx->create_surface(ctx, &texture->resource.b.b, &surf_tmpl);

				surf_tmpl.format = flushed_depth_texture->resource.b.b.format;
				surf_tmpl.u.tex.level = level;
				surf_tmpl.u.tex.first_layer = layer;
				surf_tmpl.u.tex.last_layer = layer;
				surf_tmpl.usage = PIPE_BIND_RENDER_TARGET;
				cbsurf = ctx->create_surface(ctx,
						&flushed_depth_texture->resource.b.b, &surf_tmpl);

				r600_blitter_begin(ctx, R600_DECOMPRESS);
				util_blitter_custom_depth_stencil(rctx->blitter, zsurf, cbsurf, 1 << sample,
								  rctx->custom_dsa_flush, depth);
				r600_blitter_end(ctx);

				pipe_surface_reference(&zsurf, NULL);
				pipe_surface_reference(&cbsurf, NULL);
			}
		}

		/* The texture will always be dirty if some layers or samples aren't flushed.
		 * I don't think this case occurs often though. */
		if (!staging &&
		    first_layer == 0 && last_layer == max_layer &&
		    first_sample == 0 && last_sample == max_sample) {
			texture->dirty_level_mask &= ~(1 << level);
		}
	}

	/* reenable compression in DB_RENDER_CONTROL */
	rctx->db_misc_state.flush_depthstencil_through_cb = false;
	r600_atom_dirty(rctx, &rctx->db_misc_state.atom);
}

void r600_decompress_depth_textures(struct r600_context *rctx,
			       struct r600_samplerview_state *textures)
{
	unsigned i;
	unsigned depth_texture_mask = textures->compressed_depthtex_mask;

	while (depth_texture_mask) {
		struct pipe_sampler_view *view;
		struct r600_texture *tex;

		i = u_bit_scan(&depth_texture_mask);

		view = &textures->views[i]->base;
		assert(view);

		tex = (struct r600_texture *)view->texture;
		assert(tex->is_depth && !tex->is_flushing_texture);

		r600_blit_decompress_depth(&rctx->context, tex, NULL,
					   view->u.tex.first_level, view->u.tex.last_level,
					   0, u_max_layer(&tex->resource.b.b, view->u.tex.first_level),
					   0, u_max_sample(&tex->resource.b.b));
	}
}

static void r600_blit_decompress_color(struct pipe_context *ctx,
		struct r600_texture *rtex,
		unsigned first_level, unsigned last_level,
		unsigned first_layer, unsigned last_layer)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	unsigned layer, level, checked_last_layer, max_layer;

	assert(rctx->chip_class != CAYMAN);

	if (!rtex->dirty_level_mask)
		return;

	for (level = first_level; level <= last_level; level++) {
		if (!(rtex->dirty_level_mask & (1 << level)))
			continue;

		/* The smaller the mipmap level, the less layers there are
		 * as far as 3D textures are concerned. */
		max_layer = u_max_layer(&rtex->resource.b.b, level);
		checked_last_layer = last_layer < max_layer ? last_layer : max_layer;

		for (layer = first_layer; layer <= checked_last_layer; layer++) {
			struct pipe_surface *cbsurf, surf_tmpl;

			surf_tmpl.format = rtex->resource.b.b.format;
			surf_tmpl.u.tex.level = level;
			surf_tmpl.u.tex.first_layer = layer;
			surf_tmpl.u.tex.last_layer = layer;
			surf_tmpl.usage = PIPE_BIND_RENDER_TARGET;
			cbsurf = ctx->create_surface(ctx, &rtex->resource.b.b, &surf_tmpl);

			r600_blitter_begin(ctx, R600_DECOMPRESS);
			util_blitter_custom_color(rctx->blitter, cbsurf,
						  rctx->custom_blend_decompress);
			r600_blitter_end(ctx);

			pipe_surface_reference(&cbsurf, NULL);
		}

		/* The texture will always be dirty if some layers or samples aren't flushed.
		 * I don't think this case occurs often though. */
		if (first_layer == 0 && last_layer == max_layer) {
			rtex->dirty_level_mask &= ~(1 << level);
		}
	}
}

void r600_decompress_color_textures(struct r600_context *rctx,
				    struct r600_samplerview_state *textures)
{
	unsigned i;
	unsigned mask = textures->compressed_colortex_mask;

	/* Cayman cannot decompress an MSAA colorbuffer,
	 * but it can read it compressed, so skip this. */
	assert(rctx->chip_class != CAYMAN);
	if (rctx->chip_class == CAYMAN) {
		return;
	}

	while (mask) {
		struct pipe_sampler_view *view;
		struct r600_texture *tex;

		i = u_bit_scan(&mask);

		view = &textures->views[i]->base;
		assert(view);

		tex = (struct r600_texture *)view->texture;
		assert(tex->cmask_size && tex->fmask_size);

		r600_blit_decompress_color(&rctx->context, tex,
					   view->u.tex.first_level, view->u.tex.last_level,
					   0, u_max_layer(&tex->resource.b.b, view->u.tex.first_level));
	}
}

static void r600_copy_first_sample(struct pipe_context *ctx,
				   const struct pipe_resolve_info *info)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_texture *rsrc = (struct r600_texture*)info->src.res;
	struct pipe_surface *dst_view, dst_templ;
	struct pipe_sampler_view src_templ, *src_view;
	struct pipe_box box;

	if (rsrc->is_depth && !rsrc->is_flushing_texture) {
		if (!r600_init_flushed_depth_texture(ctx, info->src.res, NULL))
			return; /* error */

		/* Decompress the first sample only. */
		r600_blit_decompress_depth(ctx,	rsrc, NULL,
					   0, 0,
					   info->src.layer, info->src.layer,
					   0, 0);
	}
	if (rctx->chip_class != CAYMAN && rsrc->fmask_size && rsrc->cmask_size) {
		r600_blit_decompress_color(ctx, rsrc,
					   0, 0,
					   info->src.layer, info->src.layer);
	}

	/* this is correct for upside-down blits too */
	u_box_2d(info->src.x0,
		 info->src.y0,
		 info->src.x1 - info->src.x0,
		 info->src.y1 - info->src.y0, &box);

	/* Initialize the surface. */
	util_blitter_default_dst_texture(&dst_templ, info->dst.res,
					 info->dst.level, info->dst.layer, &box);
	dst_view = ctx->create_surface(ctx, info->dst.res, &dst_templ);

	/* Initialize the sampler view. */
	util_blitter_default_src_texture(&src_templ, info->src.res, 0);
	src_view = ctx->create_sampler_view(ctx, info->src.res, &src_templ);

	/* Copy the first sample into dst. */
	r600_blitter_begin(ctx, R600_COPY_TEXTURE);
	util_blitter_copy_texture_view(rctx->blitter, dst_view, ~0, info->dst.x0,
				       info->dst.y0, src_view, 0, &box,
				       info->src.res->width0, info->src.res->height0,
				       info->mask);
	r600_blitter_end(ctx);

	pipe_surface_reference(&dst_view, NULL);
	pipe_sampler_view_reference(&src_view, NULL);
}

static boolean is_simple_resolve(const struct pipe_resolve_info *info)
{
   unsigned dst_width = u_minify(info->dst.res->width0, info->dst.level);
   unsigned dst_height = u_minify(info->dst.res->height0, info->dst.level);
   struct r600_texture *dst = (struct r600_texture*)info->dst.res;
   unsigned dst_tile_mode = dst->surface.level[info->dst.level].mode;

   return info->dst.res->format == info->src.res->format &&
          dst_width == info->src.res->width0 &&
          dst_height == info->src.res->height0 &&
          info->dst.x0 == 0 &&
          info->dst.y0 == 0 &&
          info->dst.x1 == dst_width &&
          info->dst.y1 == dst_height &&
          info->src.x0 == 0 &&
          info->src.y0 == 0 &&
          info->src.x1 == dst_width &&
          info->src.y1 == dst_height &&
          /* Dst must be tiled. If it's not, we have to use a temporary
	   * resource which is tiled. */
	  dst_tile_mode >= RADEON_SURF_MODE_1D;
}

static void r600_color_resolve(struct pipe_context *ctx,
			       const struct pipe_resolve_info *info)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct pipe_screen *screen = ctx->screen;
	struct pipe_resource *tmp, templ;
	struct pipe_box box;
	unsigned sample_mask =
		rctx->chip_class == CAYMAN ? ~0 : ((1ull << MAX2(1, info->src.res->nr_samples)) - 1);

	assert((info->mask & PIPE_MASK_RGBA) == PIPE_MASK_RGBA);

	if (is_simple_resolve(info)) {
		r600_blitter_begin(ctx, R600_COLOR_RESOLVE);
		util_blitter_custom_resolve_color(rctx->blitter,
						  info->dst.res, info->dst.level, info->dst.layer,
						  info->src.res, info->src.layer,
						  sample_mask, rctx->custom_blend_resolve);
		r600_blitter_end(ctx);
		return;
	}

	/* resolve into a temporary texture, then blit */
	templ.target = PIPE_TEXTURE_2D;
	templ.format = info->src.res->format;
	templ.width0 = info->src.res->width0;
	templ.height0 = info->src.res->height0;
	templ.depth0 = 1;
	templ.array_size = 1;
	templ.last_level = 0;
	templ.nr_samples = 0;
	templ.usage = PIPE_USAGE_STATIC;
	templ.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
	templ.flags = R600_RESOURCE_FLAG_FORCE_TILING; /* dst must not have a linear layout */

	tmp = screen->resource_create(screen, &templ);

	/* XXX use scissor, so that only the needed part of the resource is resolved */
	r600_blitter_begin(ctx, R600_COLOR_RESOLVE);
	util_blitter_custom_resolve_color(rctx->blitter,
					  tmp, 0, 0,
					  info->src.res, info->src.layer,
					  sample_mask, rctx->custom_blend_resolve);
	r600_blitter_end(ctx);

	/* this is correct for upside-down blits too */
	u_box_2d(info->src.x0,
		 info->src.y0,
		 info->src.x1 - info->src.x0,
		 info->src.y1 - info->src.y0, &box);

	r600_blitter_begin(ctx, R600_COPY_TEXTURE);
	util_blitter_copy_texture(rctx->blitter, info->dst.res, info->dst.level,
				  ~0, info->dst.x0, info->dst.y0, info->dst.layer,
				  tmp, 0, 0, &box);
	r600_blitter_end(ctx);

	pipe_resource_reference(&tmp, NULL);
}

static void r600_resource_resolve(struct pipe_context *ctx,
				  const struct pipe_resolve_info *info)
{
	/* make sure we're doing a resolve operation */
	assert(info->src.res->nr_samples > 1);
	assert(info->dst.res->nr_samples <= 1);

	/* limitations of multisample resources */
	assert(info->src.res->last_level == 0);
	assert(info->src.res->target == PIPE_TEXTURE_2D ||
	       info->src.res->target == PIPE_TEXTURE_2D_ARRAY);

	/* check if the resolve box is valid */
	assert(info->dst.x0 < info->dst.x1);
	assert(info->dst.y0 < info->dst.y1);

	/* scaled resolve isn't allowed */
	assert(abs(info->dst.x0 - info->dst.x1) ==
	       abs(info->src.x0 - info->src.x1));
	assert(abs(info->dst.y0 - info->dst.y1) ==
	       abs(info->src.y0 - info->src.y1));

	if ((info->mask & PIPE_MASK_ZS) ||
	    util_format_is_pure_integer(info->src.res->format)) {
		r600_copy_first_sample(ctx, info);
	} else {
		r600_color_resolve(ctx, info);
	}
}

static void r600_clear(struct pipe_context *ctx, unsigned buffers,
		       const union pipe_color_union *color,
		       double depth, unsigned stencil)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct pipe_framebuffer_state *fb = &rctx->framebuffer;

	r600_blitter_begin(ctx, R600_CLEAR);
	util_blitter_clear(rctx->blitter, fb->width, fb->height,
			   fb->nr_cbufs, buffers, fb->nr_cbufs ? fb->cbufs[0]->format : PIPE_FORMAT_NONE,
			   color, depth, stencil);
	r600_blitter_end(ctx);
}

static void r600_clear_render_target(struct pipe_context *ctx,
				     struct pipe_surface *dst,
				     const union pipe_color_union *color,
				     unsigned dstx, unsigned dsty,
				     unsigned width, unsigned height)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	r600_blitter_begin(ctx, R600_CLEAR_SURFACE);
	util_blitter_clear_render_target(rctx->blitter, dst, color,
					 dstx, dsty, width, height);
	r600_blitter_end(ctx);
}

static void r600_clear_depth_stencil(struct pipe_context *ctx,
				     struct pipe_surface *dst,
				     unsigned clear_flags,
				     double depth,
				     unsigned stencil,
				     unsigned dstx, unsigned dsty,
				     unsigned width, unsigned height)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	r600_blitter_begin(ctx, R600_CLEAR_SURFACE);
	util_blitter_clear_depth_stencil(rctx->blitter, dst, clear_flags, depth, stencil,
					 dstx, dsty, width, height);
	r600_blitter_end(ctx);
}

void r600_copy_buffer(struct pipe_context *ctx, struct
		      pipe_resource *dst, unsigned dstx,
		      struct pipe_resource *src, const struct pipe_box *src_box)
{
	struct r600_context *rctx = (struct r600_context*)ctx;

	if (rctx->screen->has_streamout &&
	    /* Require dword alignment. */
	    dstx % 4 == 0 && src_box->x % 4 == 0 && src_box->width % 4 == 0) {
		r600_blitter_begin(ctx, R600_COPY_BUFFER);
		util_blitter_copy_buffer(rctx->blitter, dst, dstx, src, src_box->x, src_box->width);
		r600_blitter_end(ctx);
	} else {
		util_resource_copy_region(ctx, dst, 0, dstx, 0, 0, src, 0, src_box);
	}
}

struct texture_orig_info {
	unsigned format;
	unsigned width0;
	unsigned height0;
	unsigned npix_x;
	unsigned npix_y;
	unsigned npix0_x;
	unsigned npix0_y;
};

static void r600_compressed_to_blittable(struct pipe_resource *tex,
				   unsigned level,
				   struct texture_orig_info *orig)
{
	struct r600_texture *rtex = (struct r600_texture*)tex;
	unsigned pixsize = util_format_get_blocksize(rtex->resource.b.b.format);
	int new_format;
	int new_height, new_width;

	orig->format = tex->format;
	orig->width0 = tex->width0;
	orig->height0 = tex->height0;
	orig->npix0_x = rtex->surface.level[0].npix_x;
	orig->npix0_y = rtex->surface.level[0].npix_y;
	orig->npix_x = rtex->surface.level[level].npix_x;
	orig->npix_y = rtex->surface.level[level].npix_y;

	if (pixsize == 8)
		new_format = PIPE_FORMAT_R16G16B16A16_UINT; /* 64-bit block */
	else
		new_format = PIPE_FORMAT_R32G32B32A32_UINT; /* 128-bit block */

	new_width = util_format_get_nblocksx(tex->format, orig->width0);
	new_height = util_format_get_nblocksy(tex->format, orig->height0);

	tex->width0 = new_width;
	tex->height0 = new_height;
	tex->format = new_format;
	rtex->surface.level[0].npix_x = util_format_get_nblocksx(orig->format, orig->npix0_x);
	rtex->surface.level[0].npix_y = util_format_get_nblocksy(orig->format, orig->npix0_y);
	rtex->surface.level[level].npix_x = util_format_get_nblocksx(orig->format, orig->npix_x);
	rtex->surface.level[level].npix_y = util_format_get_nblocksy(orig->format, orig->npix_y);
}

static void r600_subsampled_2x1_32bpp_to_blittable(struct pipe_resource *tex,
						   unsigned level,
						   struct texture_orig_info *orig)
{
	struct r600_texture *rtex = (struct r600_texture*)tex;

	orig->format = tex->format;
	orig->width0 = tex->width0;
	orig->height0 = tex->height0;
	orig->npix0_x = rtex->surface.level[0].npix_x;
	orig->npix0_y = rtex->surface.level[0].npix_y;
	orig->npix_x = rtex->surface.level[level].npix_x;
	orig->npix_y = rtex->surface.level[level].npix_y;

	tex->width0 = util_format_get_nblocksx(orig->format, orig->width0);
	tex->format = PIPE_FORMAT_R8G8B8A8_UINT;
	rtex->surface.level[0].npix_x = util_format_get_nblocksx(orig->format, orig->npix0_x);
	rtex->surface.level[level].npix_x = util_format_get_nblocksx(orig->format, orig->npix_x);
}

static void r600_change_format(struct pipe_resource *tex,
			       unsigned level,
			       struct texture_orig_info *orig,
			       enum pipe_format format)
{
	struct r600_texture *rtex = (struct r600_texture*)tex;

	orig->format = tex->format;
	orig->width0 = tex->width0;
	orig->height0 = tex->height0;
	orig->npix0_x = rtex->surface.level[0].npix_x;
	orig->npix0_y = rtex->surface.level[0].npix_y;
	orig->npix_x = rtex->surface.level[level].npix_x;
	orig->npix_y = rtex->surface.level[level].npix_y;

	tex->format = format;
}

static void r600_reset_blittable_to_orig(struct pipe_resource *tex,
					 unsigned level,
					 struct texture_orig_info *orig)
{
	struct r600_texture *rtex = (struct r600_texture*)tex;

	tex->format = orig->format;
	tex->width0 = orig->width0;
	tex->height0 = orig->height0;
	rtex->surface.level[0].npix_x = orig->npix0_x;
	rtex->surface.level[0].npix_y = orig->npix0_y;
	rtex->surface.level[level].npix_x = orig->npix_x;
	rtex->surface.level[level].npix_y = orig->npix_y;
}

static bool util_format_is_subsampled_2x1_32bpp(enum pipe_format format)
{
	const struct util_format_description *desc = util_format_description(format);

	return desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED &&
	       desc->block.width == 2 &&
	       desc->block.height == 1 &&
	       desc->block.bits == 32;
}

static void r600_resource_copy_region(struct pipe_context *ctx,
				      struct pipe_resource *dst,
				      unsigned dst_level,
				      unsigned dstx, unsigned dsty, unsigned dstz,
				      struct pipe_resource *src,
				      unsigned src_level,
				      const struct pipe_box *src_box)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_texture *rsrc = (struct r600_texture*)src;
	struct texture_orig_info orig_info[2];
	struct pipe_box sbox;
	const struct pipe_box *psbox = src_box;
	boolean restore_orig[2];
	unsigned last_sample, i;

	memset(orig_info, 0, sizeof(orig_info));

	/* Handle buffers first. */
	if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
		r600_copy_buffer(ctx, dst, dstx, src, src_box);
		return;
	}

	assert(u_max_sample(dst) == u_max_sample(src));
	last_sample = u_max_sample(dst);

	/* This must be done before entering u_blitter to avoid recursion. */
	if (rsrc->is_depth && !rsrc->is_flushing_texture) {
		if (!r600_init_flushed_depth_texture(ctx, src, NULL))
			return; /* error */

		r600_blit_decompress_depth(ctx, rsrc, NULL,
					   src_level, src_level,
					   src_box->z, src_box->z + src_box->depth - 1,
					   0, u_max_sample(src));
	}
	if (rctx->chip_class != CAYMAN && rsrc->fmask_size && rsrc->cmask_size) {
		r600_blit_decompress_color(ctx, rsrc, src_level, src_level,
					   src_box->z, src_box->z + src_box->depth - 1);
	}

	restore_orig[0] = restore_orig[1] = FALSE;

	if (util_format_is_compressed(src->format) &&
	    util_format_is_compressed(dst->format)) {
		r600_compressed_to_blittable(src, src_level, &orig_info[0]);
		restore_orig[0] = TRUE;
		sbox.x = util_format_get_nblocksx(orig_info[0].format, src_box->x);
		sbox.y = util_format_get_nblocksy(orig_info[0].format, src_box->y);
		sbox.z = src_box->z;
		sbox.width = util_format_get_nblocksx(orig_info[0].format, src_box->width);
		sbox.height = util_format_get_nblocksy(orig_info[0].format, src_box->height);
		sbox.depth = src_box->depth;
		psbox = &sbox;

		r600_compressed_to_blittable(dst, dst_level, &orig_info[1]);
		restore_orig[1] = TRUE;
		/* translate the dst box as well */
		dstx = util_format_get_nblocksx(orig_info[1].format, dstx);
		dsty = util_format_get_nblocksy(orig_info[1].format, dsty);
	} else if (!util_blitter_is_copy_supported(rctx->blitter, dst, src,
						   PIPE_MASK_RGBAZS)) {
		if (util_format_is_subsampled_2x1_32bpp(src->format) &&
		    util_format_is_subsampled_2x1_32bpp(dst->format)) {
			r600_subsampled_2x1_32bpp_to_blittable(src, src_level, &orig_info[0]);
			r600_subsampled_2x1_32bpp_to_blittable(dst, dst_level, &orig_info[1]);

			sbox = *src_box;
			sbox.x = util_format_get_nblocksx(orig_info[0].format, src_box->x);
			sbox.width = util_format_get_nblocksx(orig_info[0].format, src_box->width);
			psbox = &sbox;

			dstx = util_format_get_nblocksx(orig_info[1].format, dstx);
		} else {
			unsigned blocksize = util_format_get_blocksize(src->format);

			switch (blocksize) {
			case 1:
				r600_change_format(src, src_level, &orig_info[0],
						   PIPE_FORMAT_R8_UNORM);
				r600_change_format(dst, dst_level, &orig_info[1],
						   PIPE_FORMAT_R8_UNORM);
				break;
			case 4:
				r600_change_format(src, src_level, &orig_info[0],
						   PIPE_FORMAT_R8G8B8A8_UNORM);
				r600_change_format(dst, dst_level, &orig_info[1],
						   PIPE_FORMAT_R8G8B8A8_UNORM);
				break;
			default:
				fprintf(stderr, "Unhandled format %s with blocksize %u\n",
					util_format_short_name(src->format), blocksize);
				assert(0);
			}
		}
		restore_orig[0] = TRUE;
		restore_orig[1] = TRUE;
	}

	/* XXX Properly implement multisample textures on Cayman. In the meantime,
	 * copy only the first sample (which is the only one that doesn't return garbage). */
	if (rctx->chip_class == CAYMAN) {
		r600_blitter_begin(ctx, R600_COPY_TEXTURE);
		util_blitter_copy_texture(rctx->blitter, dst, dst_level, ~0, dstx, dsty, dstz,
					  src, src_level, 0, psbox);
		r600_blitter_end(ctx);
	} else {
		for (i = 0; i <= last_sample; i++) {
			r600_blitter_begin(ctx, R600_COPY_TEXTURE);
			util_blitter_copy_texture(rctx->blitter, dst, dst_level, 1 << i, dstx, dsty, dstz,
						  src, src_level, i, psbox);
			r600_blitter_end(ctx);
		}
	}

	if (restore_orig[0])
		r600_reset_blittable_to_orig(src, src_level, &orig_info[0]);

	if (restore_orig[1])
		r600_reset_blittable_to_orig(dst, dst_level, &orig_info[1]);
}

void r600_init_blit_functions(struct r600_context *rctx)
{
	rctx->context.clear = r600_clear;
	rctx->context.clear_render_target = r600_clear_render_target;
	rctx->context.clear_depth_stencil = r600_clear_depth_stencil;
	rctx->context.resource_copy_region = r600_resource_copy_region;
	rctx->context.resource_resolve = r600_resource_resolve;
}
