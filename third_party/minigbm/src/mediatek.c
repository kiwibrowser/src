/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifdef DRV_MEDIATEK

// clang-format off
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <mediatek_drm.h>
// clang-format on

#include "drv_priv.h"
#include "helpers.h"
#include "util.h"

struct mediatek_private_map_data {
	void *cached_addr;
	void *gem_addr;
};

static const uint32_t render_target_formats[] = { DRM_FORMAT_ABGR8888, DRM_FORMAT_ARGB8888,
						  DRM_FORMAT_RGB565, DRM_FORMAT_XBGR8888,
						  DRM_FORMAT_XRGB8888 };

static const uint32_t texture_source_formats[] = { DRM_FORMAT_R8, DRM_FORMAT_YVU420,
						   DRM_FORMAT_YVU420_ANDROID };

static int mediatek_init(struct driver *drv)
{
	drv_add_combinations(drv, render_target_formats, ARRAY_SIZE(render_target_formats),
			     &LINEAR_METADATA, BO_USE_RENDER_MASK);

	drv_add_combinations(drv, texture_source_formats, ARRAY_SIZE(texture_source_formats),
			     &LINEAR_METADATA, BO_USE_TEXTURE_MASK);

	return drv_modify_linear_combinations(drv);
}

static int mediatek_bo_create(struct bo *bo, uint32_t width, uint32_t height, uint32_t format,
			      uint64_t use_flags)
{
	int ret;
	size_t plane;
	uint32_t stride;
	struct drm_mtk_gem_create gem_create;

	/*
	 * Since the ARM L1 cache line size is 64 bytes, align to that as a
	 * performance optimization.
	 */
	stride = drv_stride_from_format(format, width, 0);
	stride = ALIGN(stride, 64);
	drv_bo_from_format(bo, stride, height, format);

	memset(&gem_create, 0, sizeof(gem_create));
	gem_create.size = bo->total_size;

	ret = drmIoctl(bo->drv->fd, DRM_IOCTL_MTK_GEM_CREATE, &gem_create);
	if (ret) {
		fprintf(stderr, "drv: DRM_IOCTL_MTK_GEM_CREATE failed (size=%llu)\n",
			gem_create.size);
		return ret;
	}

	for (plane = 0; plane < bo->num_planes; plane++)
		bo->handles[plane].u32 = gem_create.handle;

	return 0;
}

static void *mediatek_bo_map(struct bo *bo, struct vma *vma, size_t plane, uint32_t map_flags)
{
	int ret;
	struct drm_mtk_gem_map_off gem_map;
	struct mediatek_private_map_data *priv;

	memset(&gem_map, 0, sizeof(gem_map));
	gem_map.handle = bo->handles[0].u32;

	ret = drmIoctl(bo->drv->fd, DRM_IOCTL_MTK_GEM_MAP_OFFSET, &gem_map);
	if (ret) {
		fprintf(stderr, "drv: DRM_IOCTL_MTK_GEM_MAP_OFFSET failed\n");
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

static int mediatek_bo_unmap(struct bo *bo, struct vma *vma)
{
	if (vma->priv) {
		struct mediatek_private_map_data *priv = vma->priv;
		vma->addr = priv->gem_addr;
		free(priv->cached_addr);
		free(priv);
		vma->priv = NULL;
	}

	return munmap(vma->addr, vma->length);
}

static int mediatek_bo_invalidate(struct bo *bo, struct mapping *mapping)
{
	if (mapping->vma->priv) {
		struct mediatek_private_map_data *priv = mapping->vma->priv;
		memcpy(priv->cached_addr, priv->gem_addr, bo->total_size);
	}

	return 0;
}

static int mediatek_bo_flush(struct bo *bo, struct mapping *mapping)
{
	struct mediatek_private_map_data *priv = mapping->vma->priv;
	if (priv && (mapping->vma->map_flags & BO_MAP_WRITE))
		memcpy(priv->gem_addr, priv->cached_addr, bo->total_size);

	return 0;
}

static uint32_t mediatek_resolve_format(uint32_t format, uint64_t use_flags)
{
	switch (format) {
	case DRM_FORMAT_FLEX_IMPLEMENTATION_DEFINED:
		/*HACK: See b/28671744 */
		return DRM_FORMAT_XBGR8888;
	case DRM_FORMAT_FLEX_YCbCr_420_888:
		return DRM_FORMAT_YVU420;
	default:
		return format;
	}
}

const struct backend backend_mediatek = {
	.name = "mediatek",
	.init = mediatek_init,
	.bo_create = mediatek_bo_create,
	.bo_destroy = drv_gem_bo_destroy,
	.bo_import = drv_prime_bo_import,
	.bo_map = mediatek_bo_map,
	.bo_unmap = mediatek_bo_unmap,
	.bo_invalidate = mediatek_bo_invalidate,
	.bo_flush = mediatek_bo_flush,
	.resolve_format = mediatek_resolve_format,
};

#endif
