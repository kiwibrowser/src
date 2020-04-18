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
 *
 * Authors:
 *      Jerome Glisse
 *      Corbin Simpson
 */
#include <errno.h>
#include "pipe/p_screen.h"
#include "util/u_format.h"
#include "util/u_format_s3tc.h"
#include "util/u_math.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "pipebuffer/pb_buffer.h"
#include "radeonsi_pipe.h"
#include "r600_resource.h"
#include "sid.h"

/* Copy from a full GPU texture to a transfer's staging one. */
static void r600_copy_to_staging_texture(struct pipe_context *ctx, struct r600_transfer *rtransfer)
{
	struct pipe_transfer *transfer = (struct pipe_transfer*)rtransfer;
	struct pipe_resource *texture = transfer->resource;

	ctx->resource_copy_region(ctx, rtransfer->staging_texture,
				0, 0, 0, 0, texture, transfer->level,
				&transfer->box);
}


/* Copy from a transfer's staging texture to a full GPU one. */
static void r600_copy_from_staging_texture(struct pipe_context *ctx, struct r600_transfer *rtransfer)
{
	struct pipe_transfer *transfer = (struct pipe_transfer*)rtransfer;
	struct pipe_resource *texture = transfer->resource;
	struct pipe_box sbox;

	sbox.x = sbox.y = sbox.z = 0;
	sbox.width = transfer->box.width;
	sbox.height = transfer->box.height;
	/* XXX that might be wrong */
	sbox.depth = 1;
	ctx->resource_copy_region(ctx, texture, transfer->level,
				  transfer->box.x, transfer->box.y, transfer->box.z,
				  rtransfer->staging_texture,
				  0, &sbox);
}

static unsigned r600_texture_get_offset(struct r600_resource_texture *rtex,
					unsigned level, unsigned layer)
{
	return rtex->surface.level[level].offset +
	       layer * rtex->surface.level[level].slice_size;
}

static int r600_init_surface(struct radeon_surface *surface,
			     const struct pipe_resource *ptex,
			     unsigned array_mode)
{
	surface->npix_x = ptex->width0;
	surface->npix_y = ptex->height0;
	surface->npix_z = ptex->depth0;
	surface->blk_w = util_format_get_blockwidth(ptex->format);
	surface->blk_h = util_format_get_blockheight(ptex->format);
	surface->blk_d = 1;
	surface->array_size = 1;
	surface->last_level = ptex->last_level;
	surface->bpe = util_format_get_blocksize(ptex->format);
	/* align byte per element on dword */
	if (surface->bpe == 3) {
		surface->bpe = 4;
	}
	surface->nsamples = 1;
	surface->flags = 0;
	switch (array_mode) {
	case V_009910_ARRAY_1D_TILED_THIN1:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_MODE_1D, MODE);
		break;
	case V_009910_ARRAY_2D_TILED_THIN1:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_MODE_2D, MODE);
		break;
	case V_009910_ARRAY_LINEAR_ALIGNED:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_MODE_LINEAR_ALIGNED, MODE);
		break;
	case V_009910_ARRAY_LINEAR_GENERAL:
	default:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_MODE_LINEAR, MODE);
		break;
	}
	switch (ptex->target) {
	case PIPE_TEXTURE_1D:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_1D, TYPE);
		break;
	case PIPE_TEXTURE_RECT:
	case PIPE_TEXTURE_2D:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_2D, TYPE);
		break;
	case PIPE_TEXTURE_3D:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_3D, TYPE);
		break;
	case PIPE_TEXTURE_1D_ARRAY:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_1D_ARRAY, TYPE);
		surface->array_size = ptex->array_size;
		break;
	case PIPE_TEXTURE_2D_ARRAY:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_2D_ARRAY, TYPE);
		surface->array_size = ptex->array_size;
		break;
	case PIPE_TEXTURE_CUBE:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_CUBEMAP, TYPE);
		break;
	case PIPE_BUFFER:
	default:
		return -EINVAL;
	}
	if (ptex->bind & PIPE_BIND_SCANOUT) {
		surface->flags |= RADEON_SURF_SCANOUT;
	}
	if (util_format_is_depth_and_stencil(ptex->format)) {
		surface->flags |= RADEON_SURF_ZBUFFER;
		surface->flags |= RADEON_SURF_SBUFFER;
	}

	return 0;
}

static int r600_setup_surface(struct pipe_screen *screen,
			      struct r600_resource_texture *rtex,
			      unsigned array_mode,
			      unsigned pitch_in_bytes_override)
{
	struct r600_screen *rscreen = (struct r600_screen*)screen;
	int r;

	if (util_format_is_depth_or_stencil(rtex->real_format)) {
		rtex->surface.flags |= RADEON_SURF_ZBUFFER;
		rtex->surface.flags |= RADEON_SURF_SBUFFER;
	}

	r = rscreen->ws->surface_init(rscreen->ws, &rtex->surface);
	if (r) {
		return r;
	}
	if (pitch_in_bytes_override && pitch_in_bytes_override != rtex->surface.level[0].pitch_bytes) {
		/* old ddx on evergreen over estimate alignment for 1d, only 1 level
		 * for those
		 */
		rtex->surface.level[0].nblk_x = pitch_in_bytes_override / rtex->surface.bpe;
		rtex->surface.level[0].pitch_bytes = pitch_in_bytes_override;
		rtex->surface.level[0].slice_size = pitch_in_bytes_override * rtex->surface.level[0].nblk_y;
		if (rtex->surface.flags & RADEON_SURF_SBUFFER) {
			rtex->surface.stencil_offset = rtex->surface.level[0].slice_size;
		}
	}
	return 0;
}

/* Figure out whether u_blitter will fallback to a transfer operation.
 * If so, don't use a staging resource.
 */
static boolean permit_hardware_blit(struct pipe_screen *screen,
					const struct pipe_resource *res)
{
	unsigned bind;

	if (util_format_is_depth_or_stencil(res->format))
		bind = PIPE_BIND_DEPTH_STENCIL;
	else
		bind = PIPE_BIND_RENDER_TARGET;

	/* hackaround for S3TC */
	if (util_format_is_compressed(res->format))
		return TRUE;

	if (!screen->is_format_supported(screen,
				res->format,
				res->target,
				res->nr_samples,
                                bind))
		return FALSE;

	if (!screen->is_format_supported(screen,
				res->format,
				res->target,
				res->nr_samples,
                                PIPE_BIND_SAMPLER_VIEW))
		return FALSE;

	switch (res->usage) {
	case PIPE_USAGE_STREAM:
	case PIPE_USAGE_STAGING:
		return FALSE;

	default:
		return TRUE;
	}
}

static boolean r600_texture_get_handle(struct pipe_screen* screen,
					struct pipe_resource *ptex,
					struct winsys_handle *whandle)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)ptex;
	struct si_resource *resource = &rtex->resource;
	struct radeon_surface *surface = &rtex->surface;
	struct r600_screen *rscreen = (struct r600_screen*)screen;

	rscreen->ws->buffer_set_tiling(resource->buf,
				       NULL,
				       surface->level[0].mode >= RADEON_SURF_MODE_1D ?
				       RADEON_LAYOUT_TILED : RADEON_LAYOUT_LINEAR,
				       surface->level[0].mode >= RADEON_SURF_MODE_2D ?
				       RADEON_LAYOUT_TILED : RADEON_LAYOUT_LINEAR,
				       surface->bankw, surface->bankh,
				       surface->tile_split,
				       surface->stencil_tile_split,
				       surface->mtilea,
				       surface->level[0].pitch_bytes);

	return rscreen->ws->buffer_get_handle(resource->buf,
					      surface->level[0].pitch_bytes, whandle);
}

static void r600_texture_destroy(struct pipe_screen *screen,
				 struct pipe_resource *ptex)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)ptex;
	struct si_resource *resource = &rtex->resource;

	if (rtex->flushed_depth_texture)
		si_resource_reference((struct si_resource **)&rtex->flushed_depth_texture, NULL);

	pb_reference(&resource->buf, NULL);
	FREE(rtex);
}

/* Needs adjustment for pixelformat:
 */
static INLINE unsigned u_box_volume( const struct pipe_box *box )
{
	return box->width * box->depth * box->height;
};

static struct pipe_transfer* si_texture_get_transfer(struct pipe_context *ctx,
						     struct pipe_resource *texture,
						     unsigned level,
						     unsigned usage,
						     const struct pipe_box *box)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)texture;
	struct pipe_resource resource;
	struct r600_transfer *trans;
	int r;
	boolean use_staging_texture = FALSE;

	/* We cannot map a tiled texture directly because the data is
	 * in a different order, therefore we do detiling using a blit.
	 *
	 * Also, use a temporary in GTT memory for read transfers, as
	 * the CPU is much happier reading out of cached system memory
	 * than uncached VRAM.
	 */
	if (rtex->surface.level[level].mode != RADEON_SURF_MODE_LINEAR_ALIGNED &&
	    rtex->surface.level[level].mode != RADEON_SURF_MODE_LINEAR)
		use_staging_texture = TRUE;

	if ((usage & PIPE_TRANSFER_READ) && u_box_volume(box) > 1024)
		use_staging_texture = TRUE;

	/* XXX: Use a staging texture for uploads if the underlying BO
	 * is busy.  No interface for checking that currently? so do
	 * it eagerly whenever the transfer doesn't require a readback
	 * and might block.
	 */
	if ((usage & PIPE_TRANSFER_WRITE) &&
			!(usage & (PIPE_TRANSFER_READ |
					PIPE_TRANSFER_DONTBLOCK |
					PIPE_TRANSFER_UNSYNCHRONIZED)))
		use_staging_texture = TRUE;

	if (!permit_hardware_blit(ctx->screen, texture) ||
		(texture->flags & R600_RESOURCE_FLAG_TRANSFER))
		use_staging_texture = FALSE;

	if (use_staging_texture && (usage & PIPE_TRANSFER_MAP_DIRECTLY))
		return NULL;

	trans = CALLOC_STRUCT(r600_transfer);
	if (trans == NULL)
		return NULL;
	pipe_resource_reference(&trans->transfer.resource, texture);
	trans->transfer.level = level;
	trans->transfer.usage = usage;
	trans->transfer.box = *box;
	if (rtex->depth) {
		/* XXX: only readback the rectangle which is being mapped?
		*/
		/* XXX: when discard is true, no need to read back from depth texture
		*/
		r = r600_texture_depth_flush(ctx, texture, FALSE);
		if (r < 0) {
			R600_ERR("failed to create temporary texture to hold untiled copy\n");
			pipe_resource_reference(&trans->transfer.resource, NULL);
			FREE(trans);
			return NULL;
		}
		trans->transfer.stride = rtex->flushed_depth_texture->surface.level[level].pitch_bytes;
		trans->offset = r600_texture_get_offset(rtex->flushed_depth_texture, level, box->z);
		return &trans->transfer;
	} else if (use_staging_texture) {
		resource.target = PIPE_TEXTURE_2D;
		resource.format = texture->format;
		resource.width0 = box->width;
		resource.height0 = box->height;
		resource.depth0 = 1;
		resource.array_size = 1;
		resource.last_level = 0;
		resource.nr_samples = 0;
		resource.usage = PIPE_USAGE_STAGING;
		resource.bind = 0;
		resource.flags = R600_RESOURCE_FLAG_TRANSFER;
		/* For texture reading, the temporary (detiled) texture is used as
		 * a render target when blitting from a tiled texture. */
		if (usage & PIPE_TRANSFER_READ) {
			resource.bind |= PIPE_BIND_RENDER_TARGET;
		}
		/* For texture writing, the temporary texture is used as a sampler
		 * when blitting into a tiled texture. */
		if (usage & PIPE_TRANSFER_WRITE) {
			resource.bind |= PIPE_BIND_SAMPLER_VIEW;
		}
		/* Create the temporary texture. */
		trans->staging_texture = ctx->screen->resource_create(ctx->screen, &resource);
		if (trans->staging_texture == NULL) {
			R600_ERR("failed to create temporary texture to hold untiled copy\n");
			pipe_resource_reference(&trans->transfer.resource, NULL);
			FREE(trans);
			return NULL;
		}

		trans->transfer.stride = ((struct r600_resource_texture *)trans->staging_texture)
					->surface.level[0].pitch_bytes;
		if (usage & PIPE_TRANSFER_READ) {
			r600_copy_to_staging_texture(ctx, trans);
			/* Always referenced in the blit. */
			radeonsi_flush(ctx, NULL, 0);
		}
		return &trans->transfer;
	}
	trans->transfer.stride = rtex->surface.level[level].pitch_bytes;
	trans->transfer.layer_stride = rtex->surface.level[level].slice_size;
	trans->offset = r600_texture_get_offset(rtex, level, box->z);
	return &trans->transfer;
}

static void si_texture_transfer_destroy(struct pipe_context *ctx,
					struct pipe_transfer *transfer)
{
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;
	struct pipe_resource *texture = transfer->resource;
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)texture;

	if (rtransfer->staging_texture) {
		if (transfer->usage & PIPE_TRANSFER_WRITE) {
			r600_copy_from_staging_texture(ctx, rtransfer);
		}
		pipe_resource_reference(&rtransfer->staging_texture, NULL);
	}

	if (rtex->depth && !rtex->is_flushing_texture) {
		if ((transfer->usage & PIPE_TRANSFER_WRITE) && rtex->flushed_depth_texture)
			r600_blit_push_depth(ctx, rtex);
	}

	pipe_resource_reference(&transfer->resource, NULL);
	FREE(transfer);
}

static void* si_texture_transfer_map(struct pipe_context *ctx,
				     struct pipe_transfer* transfer)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;
	struct radeon_winsys_cs_handle *buf;
	enum pipe_format format = transfer->resource->format;
	unsigned offset = 0;
	char *map;

	if (rtransfer->staging_texture) {
		buf = si_resource(rtransfer->staging_texture)->cs_buf;
	} else {
		struct r600_resource_texture *rtex = (struct r600_resource_texture*)transfer->resource;

		if (rtex->flushed_depth_texture)
			buf = rtex->flushed_depth_texture->resource.cs_buf;
		else
			buf = si_resource(transfer->resource)->cs_buf;

		offset = rtransfer->offset +
			transfer->box.y / util_format_get_blockheight(format) * transfer->stride +
			transfer->box.x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);
	}

	if (!(map = rctx->ws->buffer_map(buf, rctx->cs, transfer->usage))) {
		return NULL;
	}

	return map + offset;
}

static void si_texture_transfer_unmap(struct pipe_context *ctx,
				      struct pipe_transfer* transfer)
{
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct radeon_winsys_cs_handle *buf;

	if (rtransfer->staging_texture) {
		buf = si_resource(rtransfer->staging_texture)->cs_buf;
	} else {
		struct r600_resource_texture *rtex = (struct r600_resource_texture*)transfer->resource;

		if (rtex->flushed_depth_texture) {
			buf = rtex->flushed_depth_texture->resource.cs_buf;
		} else {
			buf = si_resource(transfer->resource)->cs_buf;
		}
	}
	rctx->ws->buffer_unmap(buf);
}

static const struct u_resource_vtbl r600_texture_vtbl =
{
	r600_texture_get_handle,	/* get_handle */
	r600_texture_destroy,		/* resource_destroy */
	si_texture_get_transfer,	/* get_transfer */
	si_texture_transfer_destroy,	/* transfer_destroy */
	si_texture_transfer_map,	/* transfer_map */
	u_default_transfer_flush_region,/* transfer_flush_region */
	si_texture_transfer_unmap,	/* transfer_unmap */
	NULL	/* transfer_inline_write */
};

static struct r600_resource_texture *
r600_texture_create_object(struct pipe_screen *screen,
			   const struct pipe_resource *base,
			   unsigned array_mode,
			   unsigned pitch_in_bytes_override,
			   unsigned max_buffer_size,
			   struct pb_buffer *buf,
			   boolean alloc_bo,
			   struct radeon_surface *surface)
{
	struct r600_resource_texture *rtex;
	struct si_resource *resource;
	struct r600_screen *rscreen = (struct r600_screen*)screen;
	int r;

	rtex = CALLOC_STRUCT(r600_resource_texture);
	if (rtex == NULL)
		return NULL;

	resource = &rtex->resource;
	resource->b.b = *base;
	resource->b.vtbl = &r600_texture_vtbl;
	pipe_reference_init(&resource->b.b.reference, 1);
	resource->b.b.screen = screen;
	rtex->pitch_override = pitch_in_bytes_override;
	rtex->real_format = base->format;

	/* only mark depth textures the HW can hit as depth textures */
	if (util_format_is_depth_or_stencil(rtex->real_format) && permit_hardware_blit(screen, base))
		rtex->depth = 1;

	rtex->surface = *surface;
	r = r600_setup_surface(screen, rtex, array_mode, pitch_in_bytes_override);
	if (r) {
		FREE(rtex);
		return NULL;
	}

	/* Now create the backing buffer. */
	if (!buf && alloc_bo) {
		unsigned base_align = rtex->surface.bo_alignment;
		unsigned size = rtex->surface.bo_size;

		base_align = rtex->surface.bo_alignment;
		if (!si_init_resource(rscreen, resource, size, base_align, base->bind, base->usage)) {
			FREE(rtex);
			return NULL;
		}
	} else if (buf) {
		resource->buf = buf;
		resource->cs_buf = rscreen->ws->buffer_get_cs_handle(buf);
		resource->domains = RADEON_DOMAIN_GTT | RADEON_DOMAIN_VRAM;
	}

	return rtex;
}

struct pipe_resource *si_texture_create(struct pipe_screen *screen,
					const struct pipe_resource *templ)
{
	struct r600_screen *rscreen = (struct r600_screen*)screen;
	struct radeon_surface surface;
	unsigned array_mode = 0;
	int r;

#if 0
	if (!(templ->flags & R600_RESOURCE_FLAG_TRANSFER) &&
	    !(templ->bind & PIPE_BIND_SCANOUT)) {
		if (permit_hardware_blit(screen, templ)) {
			array_mode = V_009910_ARRAY_2D_TILED_THIN1;
		}
	}
#endif

	r = r600_init_surface(&surface, templ, array_mode);
	if (r) {
		return NULL;
	}
	r = rscreen->ws->surface_best(rscreen->ws, &surface);
	if (r) {
		return NULL;
	}
	return (struct pipe_resource *)r600_texture_create_object(screen, templ, array_mode,
								  0, 0, NULL, TRUE, &surface);
}

static struct pipe_surface *r600_create_surface(struct pipe_context *pipe,
						struct pipe_resource *texture,
						const struct pipe_surface *surf_tmpl)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)texture;
	struct r600_surface *surface = CALLOC_STRUCT(r600_surface);
	unsigned level = surf_tmpl->u.tex.level;

	assert(surf_tmpl->u.tex.first_layer == surf_tmpl->u.tex.last_layer);
	if (surface == NULL)
		return NULL;
	/* XXX no offset */
/*	offset = r600_texture_get_offset(rtex, level, surf_tmpl->u.tex.first_layer);*/
	pipe_reference_init(&surface->base.reference, 1);
	pipe_resource_reference(&surface->base.texture, texture);
	surface->base.context = pipe;
	surface->base.format = surf_tmpl->format;
	surface->base.width = rtex->surface.level[level].npix_x;
	surface->base.height = rtex->surface.level[level].npix_y;
	surface->base.usage = surf_tmpl->usage;
	surface->base.texture = texture;
	surface->base.u.tex.first_layer = surf_tmpl->u.tex.first_layer;
	surface->base.u.tex.last_layer = surf_tmpl->u.tex.last_layer;
	surface->base.u.tex.level = level;

	return &surface->base;
}

static void r600_surface_destroy(struct pipe_context *pipe,
				 struct pipe_surface *surface)
{
	pipe_resource_reference(&surface->texture, NULL);
	FREE(surface);
}

struct pipe_resource *si_texture_from_handle(struct pipe_screen *screen,
					     const struct pipe_resource *templ,
					     struct winsys_handle *whandle)
{
	struct r600_screen *rscreen = (struct r600_screen*)screen;
	struct pb_buffer *buf = NULL;
	unsigned stride = 0;
	unsigned array_mode = 0;
	enum radeon_bo_layout micro, macro;
	struct radeon_surface surface;
	int r;

	/* Support only 2D textures without mipmaps */
	if ((templ->target != PIPE_TEXTURE_2D && templ->target != PIPE_TEXTURE_RECT) ||
	      templ->depth0 != 1 || templ->last_level != 0)
		return NULL;

	buf = rscreen->ws->buffer_from_handle(rscreen->ws, whandle, &stride);
	if (!buf)
		return NULL;

	rscreen->ws->buffer_get_tiling(buf, &micro, &macro,
				       &surface.bankw, &surface.bankh,
				       &surface.tile_split,
				       &surface.stencil_tile_split,
				       &surface.mtilea);

	if (macro == RADEON_LAYOUT_TILED)
		array_mode = V_009910_ARRAY_2D_TILED_THIN1;
	else if (micro == RADEON_LAYOUT_TILED)
		array_mode = V_009910_ARRAY_1D_TILED_THIN1;
	else
		array_mode = 0;

	r = r600_init_surface(&surface, templ, array_mode);
	if (r) {
		return NULL;
	}
	return (struct pipe_resource *)r600_texture_create_object(screen, templ, array_mode,
								  stride, 0, buf, FALSE, &surface);
}

int r600_texture_depth_flush(struct pipe_context *ctx,
			     struct pipe_resource *texture, boolean just_create)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)texture;
	struct pipe_resource resource;

	if (rtex->flushed_depth_texture)
		goto out;

	resource.target = texture->target;
	resource.format = texture->format;
	resource.width0 = texture->width0;
	resource.height0 = texture->height0;
	resource.depth0 = texture->depth0;
	resource.array_size = texture->array_size;
	resource.last_level = texture->last_level;
	resource.nr_samples = texture->nr_samples;
	resource.usage = PIPE_USAGE_DYNAMIC;
	resource.bind = texture->bind | PIPE_BIND_DEPTH_STENCIL;
	resource.flags = R600_RESOURCE_FLAG_TRANSFER | texture->flags;

	rtex->flushed_depth_texture = (struct r600_resource_texture *)ctx->screen->resource_create(ctx->screen, &resource);
	if (rtex->flushed_depth_texture == NULL) {
		R600_ERR("failed to create temporary texture to hold untiled copy\n");
		return -ENOMEM;
	}

	((struct r600_resource_texture *)rtex->flushed_depth_texture)->is_flushing_texture = TRUE;
out:
	if (just_create)
		return 0;

	/* XXX: only do this if the depth texture has actually changed:
	 */
	si_blit_uncompress_depth(ctx, rtex);
	return 0;
}

void si_init_surface_functions(struct r600_context *r600)
{
	r600->context.create_surface = r600_create_surface;
	r600->context.surface_destroy = r600_surface_destroy;
}
