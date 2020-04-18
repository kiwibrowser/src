/*
 * Copyright 2010 Marek Olšák <maraeo@gmail.com
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
#ifndef R600_RESOURCE_H
#define R600_RESOURCE_H

#include "r600.h"

/* flag to indicate a resource is to be used as a transfer so should not be tiled */
#define R600_RESOURCE_FLAG_TRANSFER		PIPE_RESOURCE_FLAG_DRV_PRIV
#define R600_RESOURCE_FLAG_FLUSHED_DEPTH	(PIPE_RESOURCE_FLAG_DRV_PRIV << 1)
#define R600_RESOURCE_FLAG_FORCE_TILING		(PIPE_RESOURCE_FLAG_DRV_PRIV << 2)

struct r600_transfer {
	struct pipe_transfer		transfer;
	struct r600_resource		*staging;
	unsigned			offset;
};

struct compute_memory_item;

struct r600_resource_global {
	struct r600_resource base;
	struct compute_memory_item *chunk;
};

struct r600_texture {
	struct r600_resource		resource;

	unsigned			array_mode[PIPE_MAX_TEXTURE_LEVELS];
	unsigned			pitch_override;
	unsigned			size;
	unsigned			tile_type;
	bool				is_depth;
	bool				is_rat;
	unsigned			dirty_level_mask; /* each bit says if that mipmap is compressed */
	struct r600_texture		*flushed_depth_texture;
	boolean				is_flushing_texture;
	struct radeon_surface		surface;

	/* FMASK and CMASK can only be used with MSAA textures for now.
	 * MSAA textures cannot have mipmaps. */
	unsigned			fmask_offset, fmask_size, fmask_bank_height;
	unsigned			cmask_offset, cmask_size, cmask_slice_tile_max;
};

#define R600_TEX_IS_TILED(tex, level) ((tex)->array_mode[level] != V_038000_ARRAY_LINEAR_GENERAL && (tex)->array_mode[level] != V_038000_ARRAY_LINEAR_ALIGNED)

struct r600_fmask_info {
	unsigned size;
	unsigned alignment;
	unsigned bank_height;
};

struct r600_cmask_info {
	unsigned size;
	unsigned alignment;
	unsigned slice_tile_max;
};

struct r600_surface {
	struct pipe_surface		base;

	bool color_initialized;
	bool depth_initialized;

	/* Misc. color flags. */
	bool alphatest_bypass;
	bool export_16bpc;

	/* Color registers. */
	unsigned cb_color_info;
	unsigned cb_color_base;
	unsigned cb_color_view;
	unsigned cb_color_size;		/* R600 only */
	unsigned cb_color_dim;		/* EG only */
	unsigned cb_color_pitch;	/* EG only */
	unsigned cb_color_slice;	/* EG only */
	unsigned cb_color_attrib;	/* EG only */
	unsigned cb_color_fmask;	/* CB_COLORn_FMASK (EG) or CB_COLORn_FRAG (r600) */
	unsigned cb_color_fmask_slice;	/* EG only */
	unsigned cb_color_cmask;	/* CB_COLORn_CMASK (EG) or CB_COLORn_TILE (r600) */
	unsigned cb_color_cmask_slice;	/* EG only */
	unsigned cb_color_mask;		/* R600 only */
	struct r600_resource *cb_buffer_fmask; /* Used for FMASK relocations. R600 only */
	struct r600_resource *cb_buffer_cmask; /* Used for CMASK relocations. R600 only */

	/* DB registers. */
	unsigned db_depth_info;		/* DB_Z_INFO (EG) or DB_DEPTH_INFO (r600) */
	unsigned db_depth_base;		/* DB_Z_READ/WRITE_BASE (EG) or DB_DEPTH_BASE (r600) */
	unsigned db_depth_view;
	unsigned db_depth_size;
	unsigned db_depth_slice;	/* EG only */
	unsigned db_stencil_base;	/* EG only */
	unsigned db_stencil_info;	/* EG only */
	unsigned db_prefetch_limit;	/* R600 only */
};

void r600_resource_destroy(struct pipe_screen *screen, struct pipe_resource *res);
void r600_init_screen_resource_functions(struct pipe_screen *screen);

/* r600_texture */
void r600_texture_get_fmask_info(struct r600_screen *rscreen,
				 struct r600_texture *rtex,
				 unsigned nr_samples,
				 struct r600_fmask_info *out);
void r600_texture_get_cmask_info(struct r600_screen *rscreen,
				 struct r600_texture *rtex,
				 struct r600_cmask_info *out);
struct pipe_resource *r600_texture_create(struct pipe_screen *screen,
					const struct pipe_resource *templ);
struct pipe_resource *r600_texture_from_handle(struct pipe_screen *screen,
						const struct pipe_resource *base,
						struct winsys_handle *whandle);

static INLINE struct r600_resource *r600_resource(struct pipe_resource *r)
{
	return (struct r600_resource*)r;
}

bool r600_init_flushed_depth_texture(struct pipe_context *ctx,
				     struct pipe_resource *texture,
				     struct r600_texture **staging);

/* r600_texture.c texture transfer functions. */
struct pipe_transfer* r600_texture_get_transfer(struct pipe_context *ctx,
						struct pipe_resource *texture,
						unsigned level,
						unsigned usage,
						const struct pipe_box *box);
void r600_texture_transfer_destroy(struct pipe_context *ctx,
				   struct pipe_transfer *trans);
void* r600_texture_transfer_map(struct pipe_context *ctx,
				struct pipe_transfer* transfer);
void r600_texture_transfer_unmap(struct pipe_context *ctx,
				 struct pipe_transfer* transfer);

#endif
