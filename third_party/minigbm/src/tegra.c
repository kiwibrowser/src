/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifdef DRV_TEGRA

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <tegra_drm.h>
#include <xf86drm.h>

#include "drv_priv.h"
#include "helpers.h"
#include "util.h"

/*
 * GOB (Group Of Bytes) is the basic unit of the blocklinear layout.
 * GOBs are arranged to blocks, where the height of the block (measured
 * in GOBs) is configurable.
 */
#define NV_BLOCKLINEAR_GOB_HEIGHT 8
#define NV_BLOCKLINEAR_GOB_WIDTH 64
#define NV_DEFAULT_BLOCK_HEIGHT_LOG2 4
#define NV_PREFERRED_PAGE_SIZE (128 * 1024)

// clang-format off
enum nv_mem_kind
{
	NV_MEM_KIND_PITCH = 0,
	NV_MEM_KIND_C32_2CRA = 0xdb,
	NV_MEM_KIND_GENERIC_16Bx2 = 0xfe,
};

enum tegra_map_type {
	TEGRA_READ_TILED_BUFFER = 0,
	TEGRA_WRITE_TILED_BUFFER = 1,
};
// clang-format on

struct tegra_private_map_data {
	void *tiled;
	void *untiled;
};

static const uint32_t render_target_formats[] = { DRM_FORMAT_ARGB8888, DRM_FORMAT_XRGB8888 };

static int compute_block_height_log2(int height)
{
	int block_height_log2 = NV_DEFAULT_BLOCK_HEIGHT_LOG2;

	if (block_height_log2 > 0) {
		/* Shrink, if a smaller block height could cover the whole
		 * surface height. */
		int proposed = NV_BLOCKLINEAR_GOB_HEIGHT << (block_height_log2 - 1);
		while (proposed >= height) {
			block_height_log2--;
			if (block_height_log2 == 0)
				break;
			proposed /= 2;
		}
	}
	return block_height_log2;
}

static void compute_layout_blocklinear(int width, int height, int format, enum nv_mem_kind *kind,
				       uint32_t *block_height_log2, uint32_t *stride,
				       uint32_t *size)
{
	int pitch = drv_stride_from_format(format, width, 0);

	/* Align to blocklinear blocks. */
	pitch = ALIGN(pitch, NV_BLOCKLINEAR_GOB_WIDTH);

	/* Compute padded height. */
	*block_height_log2 = compute_block_height_log2(height);
	int block_height = 1 << *block_height_log2;
	int padded_height = ALIGN(height, NV_BLOCKLINEAR_GOB_HEIGHT * block_height);

	int bytes = pitch * padded_height;

	/* Pad the allocation to the preferred page size.
	 * This will reduce the required page table size (see discussion in NV
	 * bug 1321091), and also acts as a WAR for NV bug 1325421.
	 */
	bytes = ALIGN(bytes, NV_PREFERRED_PAGE_SIZE);

	*kind = NV_MEM_KIND_C32_2CRA;
	*stride = pitch;
	*size = bytes;
}

static void compute_layout_linear(int width, int height, int format, uint32_t *stride,
				  uint32_t *size)
{
	*stride = ALIGN(drv_stride_from_format(format, width, 0), 64);
	*size = *stride * height;
}

static void transfer_tile(struct bo *bo, uint8_t *tiled, uint8_t *untiled, enum tegra_map_type type,
			  uint32_t bytes_per_pixel, uint32_t gob_top, uint32_t gob_left,
			  uint32_t gob_size_pixels, uint8_t *tiled_last)
{
	uint8_t *tmp;
	uint32_t x, y, k;
	for (k = 0; k < gob_size_pixels; k++) {
		/*
		 * Given the kth pixel starting from the tile specified by
		 * gob_top and gob_left, unswizzle to get the standard (x, y)
		 * representation.
		 */
		x = gob_left + (((k >> 3) & 8) | ((k >> 1) & 4) | (k & 3));
		y = gob_top + ((k >> 7 << 3) | ((k >> 3) & 6) | ((k >> 2) & 1));

		if (tiled >= tiled_last)
			return;

		if (x >= bo->width || y >= bo->height) {
			tiled += bytes_per_pixel;
			continue;
		}

		tmp = untiled + y * bo->strides[0] + x * bytes_per_pixel;

		if (type == TEGRA_READ_TILED_BUFFER)
			memcpy(tmp, tiled, bytes_per_pixel);
		else if (type == TEGRA_WRITE_TILED_BUFFER)
			memcpy(tiled, tmp, bytes_per_pixel);

		/* Move on to next pixel. */
		tiled += bytes_per_pixel;
	}
}

static void transfer_tiled_memory(struct bo *bo, uint8_t *tiled, uint8_t *untiled,
				  enum tegra_map_type type)
{
	uint32_t gob_width, gob_height, gob_size_bytes, gob_size_pixels, gob_count_x, gob_count_y,
	    gob_top, gob_left;
	uint32_t i, j, offset;
	uint8_t *tmp, *tiled_last;
	uint32_t bytes_per_pixel = drv_stride_from_format(bo->format, 1, 0);

	/*
	 * The blocklinear format consists of 8*(2^n) x 64 byte sized tiles,
	 * where 0 <= n <= 4.
	 */
	gob_width = DIV_ROUND_UP(NV_BLOCKLINEAR_GOB_WIDTH, bytes_per_pixel);
	gob_height = NV_BLOCKLINEAR_GOB_HEIGHT * (1 << NV_DEFAULT_BLOCK_HEIGHT_LOG2);
	/* Calculate the height from maximum possible gob height */
	while (gob_height > NV_BLOCKLINEAR_GOB_HEIGHT && gob_height >= 2 * bo->height)
		gob_height /= 2;

	gob_size_bytes = gob_height * NV_BLOCKLINEAR_GOB_WIDTH;
	gob_size_pixels = gob_height * gob_width;

	gob_count_x = DIV_ROUND_UP(bo->strides[0], NV_BLOCKLINEAR_GOB_WIDTH);
	gob_count_y = DIV_ROUND_UP(bo->height, gob_height);

	tiled_last = tiled + bo->total_size;

	offset = 0;
	for (j = 0; j < gob_count_y; j++) {
		gob_top = j * gob_height;
		for (i = 0; i < gob_count_x; i++) {
			tmp = tiled + offset;
			gob_left = i * gob_width;

			transfer_tile(bo, tmp, untiled, type, bytes_per_pixel, gob_top, gob_left,
				      gob_size_pixels, tiled_last);

			offset += gob_size_bytes;
		}
	}
}

static int tegra_init(struct driver *drv)
{
	struct format_metadata metadata;
	uint64_t use_flags = BO_USE_RENDER_MASK;

	metadata.tiling = NV_MEM_KIND_PITCH;
	metadata.priority = 1;
	metadata.modifier = DRM_FORMAT_MOD_LINEAR;

	drv_add_combinations(drv, render_target_formats, ARRAY_SIZE(render_target_formats),
			     &metadata, use_flags);

	drv_modify_combination(drv, DRM_FORMAT_XRGB8888, &metadata, BO_USE_CURSOR | BO_USE_SCANOUT);
	drv_modify_combination(drv, DRM_FORMAT_ARGB8888, &metadata, BO_USE_CURSOR | BO_USE_SCANOUT);

	use_flags &= ~BO_USE_SW_WRITE_OFTEN;
	use_flags &= ~BO_USE_SW_READ_OFTEN;
	use_flags &= ~BO_USE_LINEAR;

	metadata.tiling = NV_MEM_KIND_C32_2CRA;
	metadata.priority = 2;

	drv_add_combinations(drv, render_target_formats, ARRAY_SIZE(render_target_formats),
			     &metadata, use_flags);

	drv_modify_combination(drv, DRM_FORMAT_XRGB8888, &metadata, BO_USE_SCANOUT);
	drv_modify_combination(drv, DRM_FORMAT_ARGB8888, &metadata, BO_USE_SCANOUT);
	return 0;
}

static int tegra_bo_create(struct bo *bo, uint32_t width, uint32_t height, uint32_t format,
			   uint64_t use_flags)
{
	uint32_t size, stride, block_height_log2 = 0;
	enum nv_mem_kind kind = NV_MEM_KIND_PITCH;
	struct drm_tegra_gem_create gem_create;
	int ret;

	if (use_flags &
	    (BO_USE_CURSOR | BO_USE_LINEAR | BO_USE_SW_READ_OFTEN | BO_USE_SW_WRITE_OFTEN))
		compute_layout_linear(width, height, format, &stride, &size);
	else
		compute_layout_blocklinear(width, height, format, &kind, &block_height_log2,
					   &stride, &size);

	memset(&gem_create, 0, sizeof(gem_create));
	gem_create.size = size;
	gem_create.flags = 0;

	ret = drmIoctl(bo->drv->fd, DRM_IOCTL_TEGRA_GEM_CREATE, &gem_create);
	if (ret) {
		fprintf(stderr, "drv: DRM_IOCTL_TEGRA_GEM_CREATE failed (size=%zu)\n", size);
		return ret;
	}

	bo->handles[0].u32 = gem_create.handle;
	bo->offsets[0] = 0;
	bo->total_size = bo->sizes[0] = size;
	bo->strides[0] = stride;

	if (kind != NV_MEM_KIND_PITCH) {
		struct drm_tegra_gem_set_tiling gem_tile;

		memset(&gem_tile, 0, sizeof(gem_tile));
		gem_tile.handle = bo->handles[0].u32;
		gem_tile.mode = DRM_TEGRA_GEM_TILING_MODE_BLOCK;
		gem_tile.value = block_height_log2;

		ret = drmCommandWriteRead(bo->drv->fd, DRM_TEGRA_GEM_SET_TILING, &gem_tile,
					  sizeof(gem_tile));
		if (ret < 0) {
			drv_gem_bo_destroy(bo);
			return ret;
		}

		/* Encode blocklinear parameters for EGLImage creation. */
		bo->tiling = (kind & 0xff) | ((block_height_log2 & 0xf) << 8);
		bo->format_modifiers[0] = fourcc_mod_code(NV, bo->tiling);
	}

	return 0;
}

static int tegra_bo_import(struct bo *bo, struct drv_import_fd_data *data)
{
	int ret;
	struct drm_tegra_gem_get_tiling gem_get_tiling;

	ret = drv_prime_bo_import(bo, data);
	if (ret)
		return ret;

	/* TODO(gsingh): export modifiers and get rid of backdoor tiling. */
	memset(&gem_get_tiling, 0, sizeof(gem_get_tiling));
	gem_get_tiling.handle = bo->handles[0].u32;

	ret = drmIoctl(bo->drv->fd, DRM_IOCTL_TEGRA_GEM_GET_TILING, &gem_get_tiling);
	if (ret) {
		drv_gem_bo_destroy(bo);
		return ret;
	}

	/* NOTE(djmk): we only know about one tiled format, so if our drmIoctl call tells us we are
	   tiled, assume it is this format (NV_MEM_KIND_C32_2CRA) otherwise linear (KIND_PITCH). */
	if (gem_get_tiling.mode == DRM_TEGRA_GEM_TILING_MODE_PITCH) {
		bo->tiling = NV_MEM_KIND_PITCH;
	} else if (gem_get_tiling.mode == DRM_TEGRA_GEM_TILING_MODE_BLOCK) {
		bo->tiling = NV_MEM_KIND_C32_2CRA;
	} else {
		fprintf(stderr, "tegra_bo_import: unknown tile format %d", gem_get_tiling.mode);
		drv_gem_bo_destroy(bo);
		assert(0);
	}

	bo->format_modifiers[0] = fourcc_mod_code(NV, bo->tiling);
	return 0;
}

static void *tegra_bo_map(struct bo *bo, struct vma *vma, size_t plane, uint32_t map_flags)
{
	int ret;
	struct drm_tegra_gem_mmap gem_map;
	struct tegra_private_map_data *priv;

	memset(&gem_map, 0, sizeof(gem_map));
	gem_map.handle = bo->handles[0].u32;

	ret = drmCommandWriteRead(bo->drv->fd, DRM_TEGRA_GEM_MMAP, &gem_map, sizeof(gem_map));
	if (ret < 0) {
		fprintf(stderr, "drv: DRM_TEGRA_GEM_MMAP failed\n");
		return MAP_FAILED;
	}

	void *addr = mmap(0, bo->total_size, drv_get_prot(map_flags), MAP_SHARED, bo->drv->fd,
			  gem_map.offset);
	vma->length = bo->total_size;
	if ((bo->tiling & 0xFF) == NV_MEM_KIND_C32_2CRA && addr != MAP_FAILED) {
		priv = calloc(1, sizeof(*priv));
		priv->untiled = calloc(1, bo->total_size);
		priv->tiled = addr;
		vma->priv = priv;
		transfer_tiled_memory(bo, priv->tiled, priv->untiled, TEGRA_READ_TILED_BUFFER);
		addr = priv->untiled;
	}

	return addr;
}

static int tegra_bo_unmap(struct bo *bo, struct vma *vma)
{
	if (vma->priv) {
		struct tegra_private_map_data *priv = vma->priv;
		vma->addr = priv->tiled;
		free(priv->untiled);
		free(priv);
		vma->priv = NULL;
	}

	return munmap(vma->addr, vma->length);
}

static int tegra_bo_flush(struct bo *bo, struct mapping *mapping)
{
	struct tegra_private_map_data *priv = mapping->vma->priv;

	if (priv && (mapping->vma->map_flags & BO_MAP_WRITE))
		transfer_tiled_memory(bo, priv->tiled, priv->untiled, TEGRA_WRITE_TILED_BUFFER);

	return 0;
}

const struct backend backend_tegra = {
	.name = "tegra",
	.init = tegra_init,
	.bo_create = tegra_bo_create,
	.bo_destroy = drv_gem_bo_destroy,
	.bo_import = tegra_bo_import,
	.bo_map = tegra_bo_map,
	.bo_unmap = tegra_bo_unmap,
	.bo_flush = tegra_bo_flush,
};

#endif
