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
#include "util/u_surface.h"
#include "util/u_blitter.h"
#include "util/u_format.h"
#include "radeonsi_pipe.h"
#include "si_state.h"

enum r600_blitter_op /* bitmask */
{
	R600_SAVE_TEXTURES      = 1,
	R600_SAVE_FRAMEBUFFER   = 2,
	R600_DISABLE_RENDER_COND = 4,

	R600_CLEAR         = 0,

	R600_CLEAR_SURFACE = R600_SAVE_FRAMEBUFFER,

	R600_COPY          = R600_SAVE_FRAMEBUFFER | R600_SAVE_TEXTURES |
			     R600_DISABLE_RENDER_COND,

	R600_DECOMPRESS    = R600_SAVE_FRAMEBUFFER | R600_DISABLE_RENDER_COND,
};

static void r600_blitter_begin(struct pipe_context *ctx, enum r600_blitter_op op)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	r600_context_queries_suspend(rctx);

	util_blitter_save_blend(rctx->blitter, rctx->queued.named.blend);
	util_blitter_save_depth_stencil_alpha(rctx->blitter, rctx->queued.named.dsa);
	util_blitter_save_stencil_ref(rctx->blitter, &rctx->stencil_ref);
	util_blitter_save_rasterizer(rctx->blitter, rctx->queued.named.rasterizer);
	util_blitter_save_fragment_shader(rctx->blitter, rctx->ps_shader);
	util_blitter_save_vertex_shader(rctx->blitter, rctx->vs_shader);
	util_blitter_save_vertex_elements(rctx->blitter, rctx->vertex_elements);
	if (rctx->queued.named.viewport) {
		util_blitter_save_viewport(rctx->blitter, &rctx->queued.named.viewport->viewport);
	}
	util_blitter_save_vertex_buffers(rctx->blitter,
					 rctx->nr_vertex_buffers,
					 rctx->vertex_buffer);
	util_blitter_save_so_targets(rctx->blitter, rctx->num_so_targets,
				     (struct pipe_stream_output_target**)rctx->so_targets);

	if (op & R600_SAVE_FRAMEBUFFER)
		util_blitter_save_framebuffer(rctx->blitter, &rctx->framebuffer);

	if (op & R600_SAVE_TEXTURES) {
		util_blitter_save_fragment_sampler_states(
			rctx->blitter, rctx->ps_samplers.n_samplers,
			(void**)rctx->ps_samplers.samplers);

		util_blitter_save_fragment_sampler_views(
			rctx->blitter, rctx->ps_samplers.n_views,
			(struct pipe_sampler_view**)rctx->ps_samplers.views);
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
	r600_context_queries_resume(rctx);
}

static unsigned u_num_layers(struct pipe_resource *r, unsigned level)
{
	switch (r->target) {
	case PIPE_TEXTURE_CUBE:
		return 6;
	case PIPE_TEXTURE_3D:
		return u_minify(r->depth0, level);
	case PIPE_TEXTURE_1D_ARRAY:
		return r->array_size;
	case PIPE_TEXTURE_2D_ARRAY:
		return r->array_size;
	default:
		return 1;
	}
}

void si_blit_uncompress_depth(struct pipe_context *ctx, struct r600_resource_texture *texture)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	unsigned layer, level;
	float depth = 1.0f;

	if (!texture->dirty_db)
		return;

	for (level = 0; level <= texture->resource.b.b.last_level; level++) {
		unsigned num_layers = u_num_layers(&texture->resource.b.b, level);

		for (layer = 0; layer < num_layers; layer++) {
			struct pipe_surface *zsurf, *cbsurf, surf_tmpl;

			surf_tmpl.format = texture->real_format;
			surf_tmpl.u.tex.level = level;
			surf_tmpl.u.tex.first_layer = layer;
			surf_tmpl.u.tex.last_layer = layer;
			surf_tmpl.usage = PIPE_BIND_DEPTH_STENCIL;

			zsurf = ctx->create_surface(ctx, &texture->resource.b.b, &surf_tmpl);

			surf_tmpl.format = texture->flushed_depth_texture->real_format;
			surf_tmpl.usage = PIPE_BIND_RENDER_TARGET;
			cbsurf = ctx->create_surface(ctx,
					(struct pipe_resource*)texture->flushed_depth_texture, &surf_tmpl);

			r600_blitter_begin(ctx, R600_DECOMPRESS);
			util_blitter_custom_depth_stencil(rctx->blitter, zsurf, cbsurf, ~0, rctx->custom_dsa_flush, depth);
			r600_blitter_end(ctx);

			pipe_surface_reference(&zsurf, NULL);
			pipe_surface_reference(&cbsurf, NULL);
		}
	}

	texture->dirty_db = FALSE;
}

void si_flush_depth_textures(struct r600_context *rctx)
{
	unsigned int i;

	/* FIXME: This handles fragment shader textures only. */

	for (i = 0; i < rctx->ps_samplers.n_views; ++i) {
		struct si_pipe_sampler_view *view;
		struct r600_resource_texture *tex;

		view = rctx->ps_samplers.views[i];
		if (!view) continue;

		tex = (struct r600_resource_texture *)view->base.texture;
		if (!tex->depth)
			continue;

		if (tex->is_flushing_texture)
			continue;

		si_blit_uncompress_depth(&rctx->context, tex);
	}

	/* also check CB here */
	for (i = 0; i < rctx->framebuffer.nr_cbufs; i++) {
		struct r600_resource_texture *tex;
		tex = (struct r600_resource_texture *)rctx->framebuffer.cbufs[i]->texture;

		if (!tex->depth)
			continue;

		if (tex->is_flushing_texture)
			continue;

		si_blit_uncompress_depth(&rctx->context, tex);
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



/* Copy a block of pixels from one surface to another using HW. */
static void r600_hw_copy_region(struct pipe_context *ctx,
				struct pipe_resource *dst,
				unsigned dst_level,
				unsigned dstx, unsigned dsty, unsigned dstz,
				struct pipe_resource *src,
				unsigned src_level,
				const struct pipe_box *src_box)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	r600_blitter_begin(ctx, R600_COPY);
	util_blitter_copy_texture(rctx->blitter, dst, dst_level, ~0, dstx, dsty, dstz,
				  src, src_level, 0, src_box);
	r600_blitter_end(ctx);
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
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)tex;
	struct r600_screen *rscreen = (struct r600_screen *)tex->screen;
	unsigned pixsize = util_format_get_blocksize(rtex->real_format);
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

static void r600_reset_blittable_to_compressed(struct pipe_resource *tex,
					       unsigned level,
					       struct texture_orig_info *orig)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)tex;
	struct r600_screen *rscreen = (struct r600_screen *)tex->screen;

	tex->format = orig->format;
	tex->width0 = orig->width0;
	tex->height0 = orig->height0;
	rtex->surface.level[0].npix_x = orig->npix0_x;
	rtex->surface.level[0].npix_y = orig->npix0_y;
	rtex->surface.level[level].npix_x = orig->npix_x;
	rtex->surface.level[level].npix_y = orig->npix_y;
}

static void r600_resource_copy_region(struct pipe_context *ctx,
				      struct pipe_resource *dst,
				      unsigned dst_level,
				      unsigned dstx, unsigned dsty, unsigned dstz,
				      struct pipe_resource *src,
				      unsigned src_level,
				      const struct pipe_box *src_box)
{
	struct r600_resource_texture *rsrc = (struct r600_resource_texture*)src;
	struct texture_orig_info orig_info[2];
	struct pipe_box sbox;
	const struct pipe_box *psbox;
	boolean restore_orig[2];

	memset(orig_info, 0, sizeof(orig_info));

	/* Fallback for buffers. */
	if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
		util_resource_copy_region(ctx, dst, dst_level, dstx, dsty, dstz,
                                          src, src_level, src_box);
		return;
	}

	if (rsrc->depth && !rsrc->is_flushing_texture)
		r600_texture_depth_flush(ctx, src, FALSE);

	restore_orig[0] = restore_orig[1] = FALSE;

	if (util_format_is_compressed(src->format)) {
		r600_compressed_to_blittable(src, src_level, &orig_info[0]);
		restore_orig[0] = TRUE;
		sbox.x = util_format_get_nblocksx(orig_info[0].format, src_box->x);
		sbox.y = util_format_get_nblocksy(orig_info[0].format, src_box->y);
		sbox.z = src_box->z;
		sbox.width = util_format_get_nblocksx(orig_info[0].format, src_box->width);
		sbox.height = util_format_get_nblocksy(orig_info[0].format, src_box->height);
		sbox.depth = src_box->depth;
		psbox=&sbox;
	} else
		psbox=src_box;

	if (util_format_is_compressed(dst->format)) {
		r600_compressed_to_blittable(dst, dst_level, &orig_info[1]);
		restore_orig[1] = TRUE;
		/* translate the dst box as well */
		dstx = util_format_get_nblocksx(orig_info[1].format, dstx);
		dsty = util_format_get_nblocksy(orig_info[1].format, dsty);
	}

	r600_hw_copy_region(ctx, dst, dst_level, dstx, dsty, dstz,
			    src, src_level, psbox);

	if (restore_orig[0])
		r600_reset_blittable_to_compressed(src, src_level, &orig_info[0]);

	if (restore_orig[1])
		r600_reset_blittable_to_compressed(dst, dst_level, &orig_info[1]);
}

void si_init_blit_functions(struct r600_context *rctx)
{
	rctx->context.clear = r600_clear;
	rctx->context.clear_render_target = r600_clear_render_target;
	rctx->context.clear_depth_stencil = r600_clear_depth_stencil;
	rctx->context.resource_copy_region = r600_resource_copy_region;
}

void r600_blit_push_depth(struct pipe_context *ctx, struct r600_resource_texture *texture)
{
	struct pipe_box sbox;

	sbox.x = sbox.y = sbox.z = 0;
	sbox.width = texture->resource.b.b.width0;
	sbox.height = texture->resource.b.b.height0;
	/* XXX that might be wrong */
	sbox.depth = 1;

	r600_hw_copy_region(ctx, (struct pipe_resource *)texture, 0,
			    0, 0, 0,
			    (struct pipe_resource *)texture->flushed_depth_texture, 0,
			    &sbox);
}
