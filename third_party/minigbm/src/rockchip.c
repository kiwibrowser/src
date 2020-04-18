/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifdef DRV_ROCKCHIP

#include <errno.h>
#include <rockchip_drm.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <xf86drm.h>

#include "drv_priv.h"
#include "helpers.h"
#include "util.h"

struct rockchip_private_map_data {
	void *cached_addr;
	void *gem_addr;
};

static const uint32_t render_target_formats[] = { DRM_FORMAT_ABGR8888, DRM_FORMAT_ARGB8888,
						  DRM_FORMAT_RGB565, DRM_FORMAT_XBGR8888,
						  DRM_FORMAT_XRGB8888 };

static const uint32_t texture_source_formats[] = { DRM_FORMAT_R8, DRM_FORMAT_NV12,
						   DRM_FORMAT_YVU420, DRM_FORMAT_YVU420_ANDROID };

static int afbc_bo_from_format(struct bo *bo, uint32_t width, uint32_t height, uint32_t format)
{
	/* We've restricted ourselves to four bytes per pixel. */
	const uint32_t pixel_size = 4;

	const uint32_t clump_width = 4;
	const uint32_t clump_height = 4;

#define AFBC_NARROW 1
#if AFBC_NARROW == 1
	const uint32_t block_width = 4 * clump_width;
	const uint32_t block_height = 4 * clump_height;
#else
	const uint32_t block_width = 8 * clump_width;
	const uint32_t block_height = 2 * clump_height;
#endif

	const uint32_t header_block_size = 16;
	const uint32_t body_block_size = block_width * block_height * pixel_size;
	const uint32_t width_in_blocks = DIV_ROUND_UP(width, block_width);
	const uint32_t height_in_blocks = DIV_ROUND_UP(height, block_height);
	const uint32_t total_blocks = width_in_blocks * height_in_blocks;

	const uint32_t header_plane_size = total_blocks * header_block_size;
	const uint32_t body_plane_size = total_blocks * body_block_size;

	/* GPU requires 64 bytes, but EGL import code expects 1024 byte
	 * alignement for the body plane. */
	const uint32_t body_plane_alignment = 1024;

	const uint32_t body_plane_offset = ALIGN(header_plane_size, body_plane_alignment);
	const uint32_t total_size = body_plane_offset + body_plane_size;

	bo->strides[0] = width_in_blocks * block_width * pixel_size;
	bo->sizes[0] = total_size;
	bo->offsets[0] = 0;

	bo->total_size = total_size;

	bo->format_modifiers[0] = DRM_FORMAT_MOD_CHROMEOS_ROCKCHIP_AFBC;

	return 0;
}

static int rockchip_add_kms_item(struct driver *drv, const struct kms_item *item)
{
	uint32_t i, j;
	uint64_t use_flags;
	struct combination *combo;
	struct format_metadata metadata;

	for (i = 0; i < drv_array_size(drv->combos); i++) {
		combo = (struct combination *)drv_array_at_idx(drv->combos, i);
		if (combo->format == item->format) {
			if (item->modifier == DRM_FORMAT_MOD_CHROMEOS_ROCKCHIP_AFBC) {
				use_flags = BO_USE_RENDERING | BO_USE_SCANOUT | BO_USE_TEXTURE;
				metadata.modifier = item->modifier;
				metadata.tiling = 0;
				metadata.priority = 2;

				for (j = 0; j < ARRAY_SIZE(texture_source_formats); j++) {
					if (item->format == texture_source_formats[j])
						use_flags &= ~BO_USE_RENDERING;
				}

				drv_add_combinations(drv, &item->format, 1, &metadata, use_flags);
			} else {
				combo->use_flags |= item->use_flags;
			}
		}
	}

	return 0;
}

static int rockchip_init(struct driver *drv)
{
	int ret;
	uint32_t i;
	struct drv_array *kms_items;
	struct format_metadata metadata;

	metadata.tiling = 0;
	metadata.priority = 1;
	metadata.modifier = DRM_FORMAT_MOD_LINEAR;

	drv_add_combinations(drv, render_target_formats, ARRAY_SIZE(render_target_formats),
			     &metadata, BO_USE_RENDER_MASK);

	drv_add_combinations(drv, texture_source_formats, ARRAY_SIZE(texture_source_formats),
			     &metadata, BO_USE_TEXTURE_MASK);

	drv_modify_combination(drv, DRM_FORMAT_XRGB8888, &metadata, BO_USE_CURSOR | BO_USE_SCANOUT);
	drv_modify_combination(drv, DRM_FORMAT_ARGB8888, &metadata, BO_USE_CURSOR | BO_USE_SCANOUT);

	/* Camera ISP supports only NV12 output. */
	drv_modify_combination(drv, DRM_FORMAT_NV12, &metadata,
			       BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE);
	/*
	 * R8 format is used for Android's HAL_PIXEL_FORMAT_BLOB and is used for JPEG snapshots
	 * from camera.
	 */
	drv_modify_combination(drv, DRM_FORMAT_R8, &metadata,
			       BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE);

	kms_items = drv_query_kms(drv);
	if (!kms_items)
		return 0;

	for (i = 0; i < drv_array_size(kms_items); i++) {
		ret = rockchip_add_kms_item(drv, (struct kms_item *)drv_array_at_idx(kms_items, i));
		if (ret) {
			drv_array_destroy(kms_items);
			return ret;
		}
	}

	drv_array_destroy(kms_items);
	return 0;
}

static bool has_modifier(const uint64_t *list, uint32_t count, uint64_t modifier)
{
	uint32_t i;
	for (i = 0; i < count; i++)
		if (list[i] == modifier)
			return true;

	return false;
}

static int rockchip_bo_create_with_modifiers(struct bo *bo, uint32_t width, uint32_t height,
					     uint32_t format, const uint64_t *modifiers,
					     uint32_t count)
{
	int ret;
	size_t plane;
	struct drm_rockchip_gem_create gem_create;

	if (format == DRM_FORMAT_NV12) {
		uint32_t w_mbs = DIV_ROUND_UP(ALIGN(width, 16), 16);
		uint32_t h_mbs = DIV_ROUND_UP(ALIGN(height, 16), 16);

		uint32_t aligned_width = w_mbs * 16;
		uint32_t aligned_height = DIV_ROUND_UP(h_mbs * 16 * 3, 2);

		drv_bo_from_format(bo, aligned_width, height, format);
		bo->total_size = bo->strides[0] * aligned_height + w_mbs * h_mbs * 128;
	} else if (width <= 2560 &&
		   has_modifier(modifiers, count, DRM_FORMAT_MOD_CHROMEOS_ROCKCHIP_AFBC)) {
		/* If the caller has decided they can use AFBC, always
		 * pick that */
		afbc_bo_from_format(bo, width, height, format);
	} else {
		if (!has_modifier(modifiers, count, DRM_FORMAT_MOD_LINEAR)) {
			errno = EINVAL;
			fprintf(stderr, "no usable modifier found\n");
			return -1;
		}

		uint32_t stride;
		/*
		 * Since the ARM L1 cache line size is 64 bytes, align to that
		 * as a performance optimization. For YV12, the Mali cmem allocator
		 * requires that chroma planes are aligned to 64-bytes, so align the
		 * luma plane to 128 bytes.
		 */
		stride = drv_stride_from_format(format, width, 0);
		if (format == DRM_FORMAT_YVU420 || format == DRM_FORMAT_YVU420_ANDROID)
			stride = ALIGN(stride, 128);
		else
			stride = ALIGN(stride, 64);

		drv_bo_from_format(bo, stride, height, format);
	}

	memset(&gem_create, 0, sizeof(gem_create));
	gem_create.size = bo->total_size;

	ret = drmIoctl(bo->drv->fd, DRM_IOCTL_ROCKCHIP_GEM_CREATE, &gem_create);

	if (ret) {
		fprintf(stderr, "drv: DRM_IOCTL_ROCKCHIP_GEM_CREATE failed (size=%llu)\n",
			gem_create.size);
		return ret;
	}

	for (plane = 0; plane < bo->num_planes; plane++)
		bo->handles[plane].u32 = gem_create.handle;

	return 0;
}

static int rockchip_bo_create(struct bo *bo, uint32_t width, uint32_t height, uint32_t format,
			      uint64_t use_flags)
{
	uint64_t modifiers[] = { DRM_FORMAT_MOD_LINEAR };
	return rockchip_bo_create_with_modifiers(bo, width, height, format, modifiers,
						 ARRAY_SIZE(modifiers));
}

static void *rockchip_bo_map(struct bo *bo, struct vma *vma, size_t plane, uint32_t map_flags)
{
	int ret;
	struct drm_rockchip_gem_map_off gem_map;
	struct rockchip_private_map_data *priv;

	/* We can only map buffers created with SW access flags, which should
	 * have no modifiers (ie, not AFBC). */
	if (bo->format_modifiers[0] == DRM_FORMAT_MOD_CHROMEOS_ROCKCHIP_AFBC)
		return MAP_FAILED;

	memset(&gem_map, 0, sizeof(gem_map));
	gem_map.handle = bo->handles[0].u32;

	ret = drmIoctl(bo->drv->fd, DRM_IOCTL_ROCKCHIP_GEM_MAP_OFFSET, &gem_map);
	if (ret) {
		fprintf(stderr, "drv: DRM_IOCTL_ROCKCHIP_GEM_MAP_OFFSET failed\n");
		return MAP_FAILED;
	}

	void *addr = mmap(0, bo->total_size, drv_get_prot(map_flags), MAP_SHARED, bo->drv->fd,
			  gem_map.offset);

	vma->length = bo->total_size;

	if (bo->use_flags & BO_USE_RENDERSCRIPT) {
		priv = calloc(1, sizeof(*priv));
		priv->cached_addr = calloc(1, bo->total_size);
		priv->gem_addr = addr;
		vma->priv = priv;
		addr = priv->cached_addr;
	}

	return addr;
}

static int rockchip_bo_unmap(struct bo *bo, struct vma *vma)
{
	if (vma->priv) {
		struct rockchip_private_map_data *priv = vma->priv;
		vma->addr = priv->gem_addr;
		free(priv->cached_addr);
		free(priv);
		vma->priv = NULL;
	}

	return munmap(vma->addr, vma->length);
}

static int rockchip_bo_invalidate(struct bo *bo, struct mapping *mapping)
{
	if (mapping->vma->priv) {
		struct rockchip_private_map_data *priv = mapping->vma->priv;
		memcpy(priv->cached_addr, priv->gem_addr, bo->total_size);
	}

	return 0;
}

static int rockchip_bo_flush(struct bo *bo, struct mapping *mapping)
{
	struct rockchip_private_map_data *priv = mapping->vma->priv;
	if (priv && (mapping->vma->map_flags & BO_MAP_WRITE))
		memcpy(priv->gem_addr, priv->cached_addr, bo->total_size);

	return 0;
}

static uint32_t rockchip_resolve_format(uint32_t format, uint64_t use_flags)
{
	switch (format) {
	case DRM_FORMAT_FLEX_IMPLEMENTATION_DEFINED:
		/* Camera subsystem requires NV12. */
		if (use_flags & (BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE))
			return DRM_FORMAT_NV12;
		/*HACK: See b/28671744 */
		return DRM_FORMAT_XBGR8888;
	case DRM_FORMAT_FLEX_YCbCr_420_888:
		return DRM_FORMAT_NV12;
	default:
		return format;
	}
}

const struct backend backend_rockchip = {
	.name = "rockchip",
	.init = rockchip_init,
	.bo_create = rockchip_bo_create,
	.bo_create_with_modifiers = rockchip_bo_create_with_modifiers,
	.bo_destroy = drv_gem_bo_destroy,
	.bo_import = drv_prime_bo_import,
	.bo_map = rockchip_bo_map,
	.bo_unmap = rockchip_bo_unmap,
	.bo_invalidate = rockchip_bo_invalidate,
	.bo_flush = rockchip_bo_flush,
	.resolve_format = rockchip_resolve_format,
};

#endif
