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
#include "r600_formats.h"
#include "r600d.h"

#include <errno.h>
#include "util/u_format_s3tc.h"
#include "util/u_memory.h"

/* Copy from a full GPU texture to a transfer's staging one. */
static void r600_copy_to_staging_texture(struct pipe_context *ctx, struct r600_transfer *rtransfer)
{
	struct pipe_transfer *transfer = (struct pipe_transfer*)rtransfer;
	struct pipe_resource *texture = transfer->resource;

	ctx->resource_copy_region(ctx, &rtransfer->staging->b.b,
				0, 0, 0, 0, texture, transfer->level,
				&transfer->box);
}


/* Copy from a transfer's staging texture to a full GPU one. */
static void r600_copy_from_staging_texture(struct pipe_context *ctx, struct r600_transfer *rtransfer)
{
	struct pipe_transfer *transfer = (struct pipe_transfer*)rtransfer;
	struct pipe_resource *texture = transfer->resource;
	struct pipe_box sbox;

	u_box_origin_2d(transfer->box.width, transfer->box.height, &sbox);

	ctx->resource_copy_region(ctx, texture, transfer->level,
				  transfer->box.x, transfer->box.y, transfer->box.z,
				  &rtransfer->staging->b.b,
				  0, &sbox);
}

unsigned r600_texture_get_offset(struct r600_texture *rtex,
					unsigned level, unsigned layer)
{
	return rtex->surface.level[level].offset +
	       layer * rtex->surface.level[level].slice_size;
}

static int r600_init_surface(struct r600_screen *rscreen,
			     struct radeon_surface *surface,
			     const struct pipe_resource *ptex,
			     unsigned array_mode,
			     bool is_transfer, bool is_flushed_depth)
{
	const struct util_format_description *desc =
		util_format_description(ptex->format);
	bool is_depth, is_stencil;

	is_depth = util_format_has_depth(desc);
	is_stencil = util_format_has_stencil(desc);

	surface->npix_x = ptex->width0;
	surface->npix_y = ptex->height0;
	surface->npix_z = ptex->depth0;
	surface->blk_w = util_format_get_blockwidth(ptex->format);
	surface->blk_h = util_format_get_blockheight(ptex->format);
	surface->blk_d = 1;
	surface->array_size = 1;
	surface->last_level = ptex->last_level;

	if (rscreen->chip_class >= EVERGREEN &&
	    !is_transfer && !is_flushed_depth &&
	    ptex->format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT) {
		surface->bpe = 4; /* stencil is allocated separately on evergreen */
	} else {
		surface->bpe = util_format_get_blocksize(ptex->format);
		/* align byte per element on dword */
		if (surface->bpe == 3) {
			surface->bpe = 4;
		}
	}

	surface->nsamples = ptex->nr_samples ? ptex->nr_samples : 1;
	surface->flags = 0;

	switch (array_mode) {
	case V_038000_ARRAY_1D_TILED_THIN1:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_MODE_1D, MODE);
		break;
	case V_038000_ARRAY_2D_TILED_THIN1:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_MODE_2D, MODE);
		break;
	case V_038000_ARRAY_LINEAR_ALIGNED:
		surface->flags |= RADEON_SURF_SET(RADEON_SURF_MODE_LINEAR_ALIGNED, MODE);
		break;
	case V_038000_ARRAY_LINEAR_GENERAL:
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

	if (!is_transfer && !is_flushed_depth && is_depth) {
		surface->flags |= RADEON_SURF_ZBUFFER;

		if (is_stencil) {
			surface->flags |= RADEON_SURF_SBUFFER;
		}
	}
	return 0;
}

static int r600_setup_surface(struct pipe_screen *screen,
			      struct r600_texture *rtex,
			      unsigned pitch_in_bytes_override)
{
	struct pipe_resource *ptex = &rtex->resource.b.b;
	struct r600_screen *rscreen = (struct r600_screen*)screen;
	unsigned i;
	int r;

	r = rscreen->ws->surface_init(rscreen->ws, &rtex->surface);
	if (r) {
		return r;
	}
	rtex->size = rtex->surface.bo_size;
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
	for (i = 0; i <= ptex->last_level; i++) {
		switch (rtex->surface.level[i].mode) {
		case RADEON_SURF_MODE_LINEAR_ALIGNED:
			rtex->array_mode[i] = V_038000_ARRAY_LINEAR_ALIGNED;
			break;
		case RADEON_SURF_MODE_1D:
			rtex->array_mode[i] = V_038000_ARRAY_1D_TILED_THIN1;
			break;
		case RADEON_SURF_MODE_2D:
			rtex->array_mode[i] = V_038000_ARRAY_2D_TILED_THIN1;
			break;
		default:
		case RADEON_SURF_MODE_LINEAR:
			rtex->array_mode[i] = 0;
			break;
		}
	}
	return 0;
}

static boolean r600_texture_get_handle(struct pipe_screen* screen,
					struct pipe_resource *ptex,
					struct winsys_handle *whandle)
{
	struct r600_texture *rtex = (struct r600_texture*)ptex;
	struct r600_resource *resource = &rtex->resource;
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
				       rtex->surface.level[0].pitch_bytes);

	return rscreen->ws->buffer_get_handle(resource->buf,
					      rtex->surface.level[0].pitch_bytes, whandle);
}

static void r600_texture_destroy(struct pipe_screen *screen,
				 struct pipe_resource *ptex)
{
	struct r600_texture *rtex = (struct r600_texture*)ptex;
	struct r600_resource *resource = &rtex->resource;

	if (rtex->flushed_depth_texture)
		pipe_resource_reference((struct pipe_resource **)&rtex->flushed_depth_texture, NULL);

	pb_reference(&resource->buf, NULL);
	FREE(rtex);
}

static const struct u_resource_vtbl r600_texture_vtbl =
{
	r600_texture_get_handle,	/* get_handle */
	r600_texture_destroy,		/* resource_destroy */
	r600_texture_get_transfer,	/* get_transfer */
	r600_texture_transfer_destroy,	/* transfer_destroy */
	r600_texture_transfer_map,	/* transfer_map */
	NULL,				/* transfer_flush_region */
	r600_texture_transfer_unmap,	/* transfer_unmap */
	NULL				/* transfer_inline_write */
};

/* The number of samples can be specified independently of the texture. */
void r600_texture_get_fmask_info(struct r600_screen *rscreen,
				 struct r600_texture *rtex,
				 unsigned nr_samples,
				 struct r600_fmask_info *out)
{
	/* FMASK is allocated pretty much like an ordinary texture.
	 * Here we use bpe in the units of bits, not bytes. */
	struct radeon_surface fmask = rtex->surface;

	switch (nr_samples) {
	case 2:
		/* This should be 8,1, but we should set nsamples > 1
		 * for the allocator to treat it as a multisample surface.
		 * Let's set 4,2 then. */
	case 4:
		fmask.bpe = 4;
		fmask.nsamples = 2;
		break;
	case 8:
		fmask.bpe = 8;
		fmask.nsamples = 4;
		break;
	case 16:
		fmask.bpe = 16;
		fmask.nsamples = 4;
		break;
	default:
		R600_ERR("Invalid sample count for FMASK allocation.\n");
		return;
	}

	/* R600-R700 errata? Anyway, this fixes colorbuffer corruption. */
	if (rscreen->chip_class <= R700) {
		fmask.bpe *= 2;
	}

	if (rscreen->chip_class >= EVERGREEN) {
		fmask.bankh = nr_samples <= 4 ? 4 : 1;
	}

	if (rscreen->ws->surface_init(rscreen->ws, &fmask)) {
		R600_ERR("Got error in surface_init while allocating FMASK.\n");
		return;
	}
	assert(fmask.level[0].mode == RADEON_SURF_MODE_2D);

	out->bank_height = fmask.bankh;
	out->alignment = MAX2(256, fmask.bo_alignment);
	out->size = (fmask.bo_size + 7) / 8;
}

static void r600_texture_allocate_fmask(struct r600_screen *rscreen,
					struct r600_texture *rtex)
{
	struct r600_fmask_info fmask;

	r600_texture_get_fmask_info(rscreen, rtex,
				    rtex->resource.b.b.nr_samples, &fmask);

	/* Reserve space for FMASK while converting bits back to bytes. */
	rtex->fmask_bank_height = fmask.bank_height;
	rtex->fmask_offset = align(rtex->size, fmask.alignment);
	rtex->fmask_size = fmask.size;
	rtex->size = rtex->fmask_offset + rtex->fmask_size;
#if 0
	printf("FMASK width=%u, height=%i, bits=%u, size=%u\n",
	       fmask.npix_x, fmask.npix_y, fmask.bpe * fmask.nsamples, rtex->fmask_size);
#endif
}

void r600_texture_get_cmask_info(struct r600_screen *rscreen,
				 struct r600_texture *rtex,
				 struct r600_cmask_info *out)
{
	unsigned cmask_tile_width = 8;
	unsigned cmask_tile_height = 8;
	unsigned cmask_tile_elements = cmask_tile_width * cmask_tile_height;
	unsigned element_bits = 4;
	unsigned cmask_cache_bits = 1024;
	unsigned num_pipes = rscreen->tiling_info.num_channels;
	unsigned pipe_interleave_bytes = rscreen->tiling_info.group_bytes;

	unsigned elements_per_macro_tile = (cmask_cache_bits / element_bits) * num_pipes;
	unsigned pixels_per_macro_tile = elements_per_macro_tile * cmask_tile_elements;
	unsigned sqrt_pixels_per_macro_tile = sqrt(pixels_per_macro_tile);
	unsigned macro_tile_width = util_next_power_of_two(sqrt_pixels_per_macro_tile);
	unsigned macro_tile_height = pixels_per_macro_tile / macro_tile_width;

	unsigned pitch_elements = align(rtex->surface.npix_x, macro_tile_width);
	unsigned height = align(rtex->surface.npix_y, macro_tile_height);

	unsigned base_align = num_pipes * pipe_interleave_bytes;
	unsigned slice_bytes =
		((pitch_elements * height * element_bits + 7) / 8) / cmask_tile_elements;

	assert(macro_tile_width % 128 == 0);
	assert(macro_tile_height % 128 == 0);

	out->slice_tile_max = ((pitch_elements * height) / (128*128)) - 1;
	out->alignment = MAX2(256, base_align);
	out->size = rtex->surface.array_size * align(slice_bytes, base_align);
}

static void r600_texture_allocate_cmask(struct r600_screen *rscreen,
					struct r600_texture *rtex)
{
	struct r600_cmask_info cmask;

	r600_texture_get_cmask_info(rscreen, rtex, &cmask);

	rtex->cmask_slice_tile_max = cmask.slice_tile_max;
	rtex->cmask_offset = align(rtex->size, cmask.alignment);
	rtex->cmask_size = cmask.size;
	rtex->size = rtex->cmask_offset + rtex->cmask_size;
#if 0
	printf("CMASK: macro tile width = %u, macro tile height = %u, "
	       "pitch elements = %u, height = %u, slice tile max = %u\n",
	       macro_tile_width, macro_tile_height, pitch_elements, height,
	       rtex->cmask_slice_tile_max);
#endif
}

static struct r600_texture *
r600_texture_create_object(struct pipe_screen *screen,
			   const struct pipe_resource *base,
			   unsigned pitch_in_bytes_override,
			   struct pb_buffer *buf,
			   boolean alloc_bo,
			   struct radeon_surface *surface)
{
	struct r600_texture *rtex;
	struct r600_resource *resource;
	struct r600_screen *rscreen = (struct r600_screen*)screen;
	int r;

	rtex = CALLOC_STRUCT(r600_texture);
	if (rtex == NULL)
		return NULL;

	resource = &rtex->resource;
	resource->b.b = *base;
	resource->b.vtbl = &r600_texture_vtbl;
	pipe_reference_init(&resource->b.b.reference, 1);
	resource->b.b.screen = screen;
	rtex->pitch_override = pitch_in_bytes_override;

	/* don't include stencil-only formats which we don't support for rendering */
	rtex->is_depth = util_format_has_depth(util_format_description(rtex->resource.b.b.format));

	rtex->surface = *surface;
	r = r600_setup_surface(screen, rtex,
			       pitch_in_bytes_override);
	if (r) {
		FREE(rtex);
		return NULL;
	}

	if (base->nr_samples > 1 && !rtex->is_depth && alloc_bo) {
		r600_texture_allocate_cmask(rscreen, rtex);
		r600_texture_allocate_fmask(rscreen, rtex);
	}

	if (!rtex->is_depth && base->nr_samples > 1 &&
	    (!rtex->fmask_size || !rtex->cmask_size)) {
		FREE(rtex);
		return NULL;
	}

	/* Now create the backing buffer. */
	if (!buf && alloc_bo) {
		unsigned base_align = rtex->surface.bo_alignment;
		unsigned usage = R600_TEX_IS_TILED(rtex, 0) ? PIPE_USAGE_STATIC : base->usage;

		if (!r600_init_resource(rscreen, resource, rtex->size, base_align, base->bind, usage)) {
			FREE(rtex);
			return NULL;
		}
	} else if (buf) {
		resource->buf = buf;
		resource->cs_buf = rscreen->ws->buffer_get_cs_handle(buf);
		resource->domains = RADEON_DOMAIN_GTT | RADEON_DOMAIN_VRAM;
	}

	if (rtex->cmask_size) {
		/* Initialize the cmask to 0xCC (= compressed state). */
		char *ptr = rscreen->ws->buffer_map(resource->cs_buf, NULL, PIPE_TRANSFER_WRITE);
		memset(ptr + rtex->cmask_offset, 0xCC, rtex->cmask_size);
		rscreen->ws->buffer_unmap(resource->cs_buf);
	}
	return rtex;
}

struct pipe_resource *r600_texture_create(struct pipe_screen *screen,
						const struct pipe_resource *templ)
{
	struct r600_screen *rscreen = (struct r600_screen*)screen;
	struct radeon_surface surface;
	unsigned array_mode = 0;
	int r;

	if (!(templ->flags & R600_RESOURCE_FLAG_TRANSFER)) {
		if (templ->flags & R600_RESOURCE_FLAG_FORCE_TILING) {
			array_mode = V_038000_ARRAY_2D_TILED_THIN1;
		} else if (!(templ->bind & PIPE_BIND_SCANOUT) &&
		    templ->usage != PIPE_USAGE_STAGING &&
		    templ->usage != PIPE_USAGE_STREAM) {
			array_mode = V_038000_ARRAY_2D_TILED_THIN1;
		} else if (util_format_is_compressed(templ->format)) {
			array_mode = V_038000_ARRAY_1D_TILED_THIN1;
		}
	}

	/* XXX tiling is broken for the 422 formats */
	if (util_format_description(templ->format)->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED)
		array_mode = V_038000_ARRAY_LINEAR_ALIGNED;

	r = r600_init_surface(rscreen, &surface, templ, array_mode,
			      templ->flags & R600_RESOURCE_FLAG_TRANSFER,
			      templ->flags & R600_RESOURCE_FLAG_FLUSHED_DEPTH);
	if (r) {
		return NULL;
	}
	r = rscreen->ws->surface_best(rscreen->ws, &surface);
	if (r) {
		return NULL;
	}
	return (struct pipe_resource *)r600_texture_create_object(screen, templ,
								  0, NULL, TRUE, &surface);
}

static struct pipe_surface *r600_create_surface(struct pipe_context *pipe,
						struct pipe_resource *texture,
						const struct pipe_surface *templ)
{
	struct r600_texture *rtex = (struct r600_texture*)texture;
	struct r600_surface *surface = CALLOC_STRUCT(r600_surface);
	unsigned level = templ->u.tex.level;

	assert(templ->u.tex.first_layer == templ->u.tex.last_layer);
	if (surface == NULL)
		return NULL;
	pipe_reference_init(&surface->base.reference, 1);
	pipe_resource_reference(&surface->base.texture, texture);
	surface->base.context = pipe;
	surface->base.format = templ->format;
	surface->base.width = rtex->surface.level[level].npix_x;
	surface->base.height = rtex->surface.level[level].npix_y;
	surface->base.usage = templ->usage;
	surface->base.u = templ->u;
	return &surface->base;
}

static void r600_surface_destroy(struct pipe_context *pipe,
				 struct pipe_surface *surface)
{
	struct r600_surface *surf = (struct r600_surface*)surface;
	pipe_resource_reference((struct pipe_resource**)&surf->cb_buffer_fmask, NULL);
	pipe_resource_reference((struct pipe_resource**)&surf->cb_buffer_cmask, NULL);
	pipe_resource_reference(&surface->texture, NULL);
	FREE(surface);
}

struct pipe_resource *r600_texture_from_handle(struct pipe_screen *screen,
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
		array_mode = V_0280A0_ARRAY_2D_TILED_THIN1;
	else if (micro == RADEON_LAYOUT_TILED)
		array_mode = V_0280A0_ARRAY_1D_TILED_THIN1;
	else
		array_mode = 0;

	r = r600_init_surface(rscreen, &surface, templ, array_mode, false, false);
	if (r) {
		return NULL;
	}
	return (struct pipe_resource *)r600_texture_create_object(screen, templ,
								  stride, buf, FALSE, &surface);
}

bool r600_init_flushed_depth_texture(struct pipe_context *ctx,
				     struct pipe_resource *texture,
				     struct r600_texture **staging)
{
	struct r600_texture *rtex = (struct r600_texture*)texture;
	struct pipe_resource resource;
	struct r600_texture **flushed_depth_texture = staging ?
			staging : &rtex->flushed_depth_texture;

	if (!staging && rtex->flushed_depth_texture)
		return true; /* it's ready */

	resource.target = texture->target;
	resource.format = texture->format;
	resource.width0 = texture->width0;
	resource.height0 = texture->height0;
	resource.depth0 = texture->depth0;
	resource.array_size = texture->array_size;
	resource.last_level = texture->last_level;
	resource.nr_samples = texture->nr_samples;
	resource.usage = staging ? PIPE_USAGE_STAGING : PIPE_USAGE_STATIC;
	resource.bind = texture->bind & ~PIPE_BIND_DEPTH_STENCIL;
	resource.flags = texture->flags | R600_RESOURCE_FLAG_FLUSHED_DEPTH;

	if (staging)
		resource.flags |= R600_RESOURCE_FLAG_TRANSFER;

	*flushed_depth_texture = (struct r600_texture *)ctx->screen->resource_create(ctx->screen, &resource);
	if (*flushed_depth_texture == NULL) {
		R600_ERR("failed to create temporary texture to hold flushed depth\n");
		return false;
	}

	(*flushed_depth_texture)->is_flushing_texture = TRUE;
	return true;
}

struct pipe_transfer* r600_texture_get_transfer(struct pipe_context *ctx,
						struct pipe_resource *texture,
						unsigned level,
						unsigned usage,
						const struct pipe_box *box)
{
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct r600_texture *rtex = (struct r600_texture*)texture;
	struct pipe_resource resource;
	struct r600_transfer *trans;
	boolean use_staging_texture = FALSE;

	/* We cannot map a tiled texture directly because the data is
	 * in a different order, therefore we do detiling using a blit.
	 *
	 * Also, use a temporary in GTT memory for read transfers, as
	 * the CPU is much happier reading out of cached system memory
	 * than uncached VRAM.
	 */
	if (R600_TEX_IS_TILED(rtex, level)) {
		use_staging_texture = TRUE;
	}

	/* Use a staging texture for uploads if the underlying BO is busy. */
	if (!(usage & PIPE_TRANSFER_READ) &&
	    (rctx->ws->cs_is_buffer_referenced(rctx->cs, rtex->resource.cs_buf, RADEON_USAGE_READWRITE) ||
	     rctx->ws->buffer_is_busy(rtex->resource.buf, RADEON_USAGE_READWRITE))) {
		use_staging_texture = TRUE;
	}

	if (texture->flags & R600_RESOURCE_FLAG_TRANSFER) {
		use_staging_texture = FALSE;
	}

	if (use_staging_texture && (usage & PIPE_TRANSFER_MAP_DIRECTLY)) {
		return NULL;
	}

	trans = CALLOC_STRUCT(r600_transfer);
	if (trans == NULL)
		return NULL;
	pipe_resource_reference(&trans->transfer.resource, texture);
	trans->transfer.level = level;
	trans->transfer.usage = usage;
	trans->transfer.box = *box;
	if (rtex->is_depth) {
		/* XXX: only readback the rectangle which is being mapped?
		*/
		/* XXX: when discard is true, no need to read back from depth texture
		*/
		struct r600_texture *staging_depth;

		if (!r600_init_flushed_depth_texture(ctx, texture, &staging_depth)) {
			R600_ERR("failed to create temporary texture to hold untiled copy\n");
			pipe_resource_reference(&trans->transfer.resource, NULL);
			FREE(trans);
			return NULL;
		}

		r600_blit_decompress_depth(ctx, rtex, staging_depth,
					   level, level,
					   box->z, box->z + box->depth - 1,
					   0, 0);

		trans->transfer.stride = staging_depth->surface.level[level].pitch_bytes;
		trans->offset = r600_texture_get_offset(staging_depth, level, box->z);
		trans->staging = (struct r600_resource*)staging_depth;
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
		trans->staging = (struct r600_resource*)ctx->screen->resource_create(ctx->screen, &resource);
		if (trans->staging == NULL) {
			R600_ERR("failed to create temporary texture to hold untiled copy\n");
			pipe_resource_reference(&trans->transfer.resource, NULL);
			FREE(trans);
			return NULL;
		}

		trans->transfer.stride =
			((struct r600_texture *)trans->staging)->surface.level[0].pitch_bytes;
		if (usage & PIPE_TRANSFER_READ) {
			r600_copy_to_staging_texture(ctx, trans);
			/* Always referenced in the blit. */
			r600_flush(ctx, NULL, 0);
		}
		return &trans->transfer;
	}
	trans->transfer.stride = rtex->surface.level[level].pitch_bytes;
	trans->transfer.layer_stride = rtex->surface.level[level].slice_size;
	trans->offset = r600_texture_get_offset(rtex, level, box->z);
	return &trans->transfer;
}

void r600_texture_transfer_destroy(struct pipe_context *ctx,
				   struct pipe_transfer *transfer)
{
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;
	struct pipe_resource *texture = transfer->resource;
	struct r600_texture *rtex = (struct r600_texture*)texture;

	if ((transfer->usage & PIPE_TRANSFER_WRITE) && rtransfer->staging) {
		if (rtex->is_depth) {
			ctx->resource_copy_region(ctx, texture, transfer->level,
						  transfer->box.x, transfer->box.y, transfer->box.z,
						  &rtransfer->staging->b.b, transfer->level,
						  &transfer->box);
		} else {
			r600_copy_from_staging_texture(ctx, rtransfer);
		}
	}

	if (rtransfer->staging)
		pipe_resource_reference((struct pipe_resource**)&rtransfer->staging, NULL);

	pipe_resource_reference(&transfer->resource, NULL);
	FREE(transfer);
}

void* r600_texture_transfer_map(struct pipe_context *ctx,
				struct pipe_transfer* transfer)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;
	struct radeon_winsys_cs_handle *buf;
	struct r600_texture *rtex =
			(struct r600_texture*)transfer->resource;
	enum pipe_format format = transfer->resource->format;
	unsigned offset = 0;
	char *map;

	if ((transfer->resource->bind & PIPE_BIND_GLOBAL) && transfer->resource->target == PIPE_BUFFER) {
		return r600_compute_global_transfer_map(ctx, transfer);
	}

	if (rtransfer->staging) {
		buf = ((struct r600_resource *)rtransfer->staging)->cs_buf;
	} else {
		buf = ((struct r600_resource *)transfer->resource)->cs_buf;
	}

	if (rtex->is_depth || !rtransfer->staging)
		offset = rtransfer->offset +
			transfer->box.y / util_format_get_blockheight(format) * transfer->stride +
			transfer->box.x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);

	if (!(map = rctx->ws->buffer_map(buf, rctx->cs, transfer->usage))) {
		return NULL;
	}

	return map + offset;
}

void r600_texture_transfer_unmap(struct pipe_context *ctx,
				 struct pipe_transfer* transfer)
{
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct radeon_winsys_cs_handle *buf;

	if ((transfer->resource->bind & PIPE_BIND_GLOBAL) && transfer->resource->target == PIPE_BUFFER) {
		return r600_compute_global_transfer_unmap(ctx, transfer);
	}

	if (rtransfer->staging) {
		buf = ((struct r600_resource *)rtransfer->staging)->cs_buf;
	} else {
		buf = ((struct r600_resource *)transfer->resource)->cs_buf;
	}
	rctx->ws->buffer_unmap(buf);
}

void r600_init_surface_functions(struct r600_context *r600)
{
	r600->context.create_surface = r600_create_surface;
	r600->context.surface_destroy = r600_surface_destroy;
}

static unsigned r600_get_swizzle_combined(const unsigned char *swizzle_format,
		const unsigned char *swizzle_view)
{
	unsigned i;
	unsigned char swizzle[4];
	unsigned result = 0;
	const uint32_t swizzle_shift[4] = {
		16, 19, 22, 25,
	};
	const uint32_t swizzle_bit[4] = {
		0, 1, 2, 3,
	};

	if (swizzle_view) {
		util_format_compose_swizzles(swizzle_format, swizzle_view, swizzle);
	} else {
		memcpy(swizzle, swizzle_format, 4);
	}

	/* Get swizzle. */
	for (i = 0; i < 4; i++) {
		switch (swizzle[i]) {
		case UTIL_FORMAT_SWIZZLE_Y:
			result |= swizzle_bit[1] << swizzle_shift[i];
			break;
		case UTIL_FORMAT_SWIZZLE_Z:
			result |= swizzle_bit[2] << swizzle_shift[i];
			break;
		case UTIL_FORMAT_SWIZZLE_W:
			result |= swizzle_bit[3] << swizzle_shift[i];
			break;
		case UTIL_FORMAT_SWIZZLE_0:
			result |= V_038010_SQ_SEL_0 << swizzle_shift[i];
			break;
		case UTIL_FORMAT_SWIZZLE_1:
			result |= V_038010_SQ_SEL_1 << swizzle_shift[i];
			break;
		default: /* UTIL_FORMAT_SWIZZLE_X */
			result |= swizzle_bit[0] << swizzle_shift[i];
		}
	}
	return result;
}

/* texture format translate */
uint32_t r600_translate_texformat(struct pipe_screen *screen,
				  enum pipe_format format,
				  const unsigned char *swizzle_view,
				  uint32_t *word4_p, uint32_t *yuv_format_p)
{
	uint32_t result = 0, word4 = 0, yuv_format = 0;
	const struct util_format_description *desc;
	boolean uniform = TRUE;
	static int r600_enable_s3tc = -1;
	bool is_srgb_valid = FALSE;

	int i;
	const uint32_t sign_bit[4] = {
		S_038010_FORMAT_COMP_X(V_038010_SQ_FORMAT_COMP_SIGNED),
		S_038010_FORMAT_COMP_Y(V_038010_SQ_FORMAT_COMP_SIGNED),
		S_038010_FORMAT_COMP_Z(V_038010_SQ_FORMAT_COMP_SIGNED),
		S_038010_FORMAT_COMP_W(V_038010_SQ_FORMAT_COMP_SIGNED)
	};
	desc = util_format_description(format);

	word4 |= r600_get_swizzle_combined(desc->swizzle, swizzle_view);

	/* Colorspace (return non-RGB formats directly). */
	switch (desc->colorspace) {
		/* Depth stencil formats */
	case UTIL_FORMAT_COLORSPACE_ZS:
		switch (format) {
		case PIPE_FORMAT_Z16_UNORM:
			result = FMT_16;
			goto out_word4;
		case PIPE_FORMAT_X24S8_UINT:
			word4 |= S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_INT);
		case PIPE_FORMAT_Z24X8_UNORM:
		case PIPE_FORMAT_Z24_UNORM_S8_UINT:
			result = FMT_8_24;
			goto out_word4;
		case PIPE_FORMAT_S8X24_UINT:
			word4 |= S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_INT);
		case PIPE_FORMAT_X8Z24_UNORM:
		case PIPE_FORMAT_S8_UINT_Z24_UNORM:
			result = FMT_24_8;
			goto out_word4;
		case PIPE_FORMAT_S8_UINT:
			result = FMT_8;
			word4 |= S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_INT);
			goto out_word4;
		case PIPE_FORMAT_Z32_FLOAT:
			result = FMT_32_FLOAT;
			goto out_word4;
		case PIPE_FORMAT_X32_S8X24_UINT:
			word4 |= S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_INT);
		case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
			result = FMT_X24_8_32_FLOAT;
			goto out_word4;
		default:
			goto out_unknown;
		}

	case UTIL_FORMAT_COLORSPACE_YUV:
		yuv_format |= (1 << 30);
		switch (format) {
		case PIPE_FORMAT_UYVY:
		case PIPE_FORMAT_YUYV:
		default:
			break;
		}
		goto out_unknown; /* XXX */

	case UTIL_FORMAT_COLORSPACE_SRGB:
		word4 |= S_038010_FORCE_DEGAMMA(1);
		break;

	default:
		break;
	}

	if (r600_enable_s3tc == -1) {
		struct r600_screen *rscreen = (struct r600_screen *)screen;
		if (rscreen->info.drm_minor >= 9)
			r600_enable_s3tc = 1;
		else
			r600_enable_s3tc = debug_get_bool_option("R600_ENABLE_S3TC", FALSE);
	}

	if (desc->layout == UTIL_FORMAT_LAYOUT_RGTC) {
		if (!r600_enable_s3tc)
			goto out_unknown;

		switch (format) {
		case PIPE_FORMAT_RGTC1_SNORM:
		case PIPE_FORMAT_LATC1_SNORM:
			word4 |= sign_bit[0];
		case PIPE_FORMAT_RGTC1_UNORM:
		case PIPE_FORMAT_LATC1_UNORM:
			result = FMT_BC4;
			goto out_word4;
		case PIPE_FORMAT_RGTC2_SNORM:
		case PIPE_FORMAT_LATC2_SNORM:
			word4 |= sign_bit[0] | sign_bit[1];
		case PIPE_FORMAT_RGTC2_UNORM:
		case PIPE_FORMAT_LATC2_UNORM:
			result = FMT_BC5;
			goto out_word4;
		default:
			goto out_unknown;
		}
	}

	if (desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {

		if (!r600_enable_s3tc)
			goto out_unknown;

		if (!util_format_s3tc_enabled) {
			goto out_unknown;
		}

		switch (format) {
		case PIPE_FORMAT_DXT1_RGB:
		case PIPE_FORMAT_DXT1_RGBA:
		case PIPE_FORMAT_DXT1_SRGB:
		case PIPE_FORMAT_DXT1_SRGBA:
			result = FMT_BC1;
			is_srgb_valid = TRUE;
			goto out_word4;
		case PIPE_FORMAT_DXT3_RGBA:
		case PIPE_FORMAT_DXT3_SRGBA:
			result = FMT_BC2;
			is_srgb_valid = TRUE;
			goto out_word4;
		case PIPE_FORMAT_DXT5_RGBA:
		case PIPE_FORMAT_DXT5_SRGBA:
			result = FMT_BC3;
			is_srgb_valid = TRUE;
			goto out_word4;
		default:
			goto out_unknown;
		}
	}

	if (desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED) {
		switch (format) {
		case PIPE_FORMAT_R8G8_B8G8_UNORM:
		case PIPE_FORMAT_G8R8_B8R8_UNORM:
			result = FMT_GB_GR;
			goto out_word4;
		case PIPE_FORMAT_G8R8_G8B8_UNORM:
		case PIPE_FORMAT_R8G8_R8B8_UNORM:
			result = FMT_BG_RG;
			goto out_word4;
		default:
			goto out_unknown;
		}
	}

	if (format == PIPE_FORMAT_R9G9B9E5_FLOAT) {
		result = FMT_5_9_9_9_SHAREDEXP;
		goto out_word4;
	} else if (format == PIPE_FORMAT_R11G11B10_FLOAT) {
		result = FMT_10_11_11_FLOAT;
		goto out_word4;
	}


	for (i = 0; i < desc->nr_channels; i++) {
		if (desc->channel[i].type == UTIL_FORMAT_TYPE_SIGNED) {
			word4 |= sign_bit[i];
		}
	}

	/* R8G8Bx_SNORM - XXX CxV8U8 */

	/* See whether the components are of the same size. */
	for (i = 1; i < desc->nr_channels; i++) {
		uniform = uniform && desc->channel[0].size == desc->channel[i].size;
	}

	/* Non-uniform formats. */
	if (!uniform) {
		if (desc->colorspace != UTIL_FORMAT_COLORSPACE_SRGB &&
		    desc->channel[0].pure_integer)
			word4 |= S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_INT);
		switch(desc->nr_channels) {
		case 3:
			if (desc->channel[0].size == 5 &&
			    desc->channel[1].size == 6 &&
			    desc->channel[2].size == 5) {
				result = FMT_5_6_5;
				goto out_word4;
			}
			goto out_unknown;
		case 4:
			if (desc->channel[0].size == 5 &&
			    desc->channel[1].size == 5 &&
			    desc->channel[2].size == 5 &&
			    desc->channel[3].size == 1) {
				result = FMT_1_5_5_5;
				goto out_word4;
			}
			if (desc->channel[0].size == 10 &&
			    desc->channel[1].size == 10 &&
			    desc->channel[2].size == 10 &&
			    desc->channel[3].size == 2) {
				result = FMT_2_10_10_10;
				goto out_word4;
			}
			goto out_unknown;
		}
		goto out_unknown;
	}

	/* Find the first non-VOID channel. */
	for (i = 0; i < 4; i++) {
		if (desc->channel[i].type != UTIL_FORMAT_TYPE_VOID) {
			break;
		}
	}

	if (i == 4)
		goto out_unknown;

	/* uniform formats */
	switch (desc->channel[i].type) {
	case UTIL_FORMAT_TYPE_UNSIGNED:
	case UTIL_FORMAT_TYPE_SIGNED:
#if 0
		if (!desc->channel[i].normalized &&
		    desc->colorspace != UTIL_FORMAT_COLORSPACE_SRGB) {
			goto out_unknown;
		}
#endif
		if (desc->colorspace != UTIL_FORMAT_COLORSPACE_SRGB &&
		    desc->channel[i].pure_integer)
			word4 |= S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_INT);

		switch (desc->channel[i].size) {
		case 4:
			switch (desc->nr_channels) {
			case 2:
				result = FMT_4_4;
				goto out_word4;
			case 4:
				result = FMT_4_4_4_4;
				goto out_word4;
			}
			goto out_unknown;
		case 8:
			switch (desc->nr_channels) {
			case 1:
				result = FMT_8;
				goto out_word4;
			case 2:
				result = FMT_8_8;
				goto out_word4;
			case 4:
				result = FMT_8_8_8_8;
				is_srgb_valid = TRUE;
				goto out_word4;
			}
			goto out_unknown;
		case 16:
			switch (desc->nr_channels) {
			case 1:
				result = FMT_16;
				goto out_word4;
			case 2:
				result = FMT_16_16;
				goto out_word4;
			case 4:
				result = FMT_16_16_16_16;
				goto out_word4;
			}
			goto out_unknown;
		case 32:
			switch (desc->nr_channels) {
			case 1:
				result = FMT_32;
				goto out_word4;
			case 2:
				result = FMT_32_32;
				goto out_word4;
			case 4:
				result = FMT_32_32_32_32;
				goto out_word4;
			}
		}
		goto out_unknown;

	case UTIL_FORMAT_TYPE_FLOAT:
		switch (desc->channel[i].size) {
		case 16:
			switch (desc->nr_channels) {
			case 1:
				result = FMT_16_FLOAT;
				goto out_word4;
			case 2:
				result = FMT_16_16_FLOAT;
				goto out_word4;
			case 4:
				result = FMT_16_16_16_16_FLOAT;
				goto out_word4;
			}
			goto out_unknown;
		case 32:
			switch (desc->nr_channels) {
			case 1:
				result = FMT_32_FLOAT;
				goto out_word4;
			case 2:
				result = FMT_32_32_FLOAT;
				goto out_word4;
			case 4:
				result = FMT_32_32_32_32_FLOAT;
				goto out_word4;
			}
		}
		goto out_unknown;
	}

out_word4:

	if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB && !is_srgb_valid)
		return ~0;
	if (word4_p)
		*word4_p = word4;
	if (yuv_format_p)
		*yuv_format_p = yuv_format;
	return result;
out_unknown:
	/* R600_ERR("Unable to handle texformat %d %s\n", format, util_format_name(format)); */
	return ~0;
}
