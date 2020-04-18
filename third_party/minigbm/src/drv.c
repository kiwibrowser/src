/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <xf86drm.h>

#include "drv_priv.h"
#include "helpers.h"
#include "util.h"

#ifdef DRV_AMDGPU
extern const struct backend backend_amdgpu;
#endif
extern const struct backend backend_evdi;
#ifdef DRV_EXYNOS
extern const struct backend backend_exynos;
#endif
#ifdef DRV_I915
extern const struct backend backend_i915;
#endif
#ifdef DRV_MARVELL
extern const struct backend backend_marvell;
#endif
#ifdef DRV_MEDIATEK
extern const struct backend backend_mediatek;
#endif
#ifdef DRV_MESON
extern const struct backend backend_meson;
#endif
#ifdef DRV_MSM
extern const struct backend backend_msm;
#endif
extern const struct backend backend_nouveau;
#ifdef DRV_RADEON
extern const struct backend backend_radeon;
#endif
#ifdef DRV_ROCKCHIP
extern const struct backend backend_rockchip;
#endif
#ifdef DRV_TEGRA
extern const struct backend backend_tegra;
#endif
extern const struct backend backend_udl;
#ifdef DRV_VC4
extern const struct backend backend_vc4;
#endif
extern const struct backend backend_vgem;
extern const struct backend backend_virtio_gpu;

static const struct backend *drv_get_backend(int fd)
{
	drmVersionPtr drm_version;
	unsigned int i;

	drm_version = drmGetVersion(fd);

	if (!drm_version)
		return NULL;

	const struct backend *backend_list[] = {
#ifdef DRV_AMDGPU
		&backend_amdgpu,
#endif
		&backend_evdi,
#ifdef DRV_EXYNOS
		&backend_exynos,
#endif
#ifdef DRV_I915
		&backend_i915,
#endif
#ifdef DRV_MARVELL
		&backend_marvell,
#endif
#ifdef DRV_MEDIATEK
		&backend_mediatek,
#endif
#ifdef DRV_MESON
		&backend_meson,
#endif
#ifdef DRV_MSM
		&backend_msm,
#endif
		&backend_nouveau,
#ifdef DRV_RADEON
		&backend_radeon,
#endif
#ifdef DRV_ROCKCHIP
		&backend_rockchip,
#endif
#ifdef DRV_TEGRA
		&backend_tegra,
#endif
		&backend_udl,
#ifdef DRV_VC4
		&backend_vc4,
#endif
		&backend_vgem,     &backend_virtio_gpu,
	};

	for (i = 0; i < ARRAY_SIZE(backend_list); i++)
		if (!strcmp(drm_version->name, backend_list[i]->name)) {
			drmFreeVersion(drm_version);
			return backend_list[i];
		}

	drmFreeVersion(drm_version);
	return NULL;
}

struct driver *drv_create(int fd)
{
	struct driver *drv;
	int ret;

	drv = (struct driver *)calloc(1, sizeof(*drv));

	if (!drv)
		return NULL;

	drv->fd = fd;
	drv->backend = drv_get_backend(fd);

	if (!drv->backend)
		goto free_driver;

	if (pthread_mutex_init(&drv->driver_lock, NULL))
		goto free_driver;

	drv->buffer_table = drmHashCreate();
	if (!drv->buffer_table)
		goto free_lock;

	drv->mappings = drv_array_init(sizeof(struct mapping));
	if (!drv->mappings)
		goto free_buffer_table;

	drv->combos = drv_array_init(sizeof(struct combination));
	if (!drv->combos)
		goto free_mappings;

	if (drv->backend->init) {
		ret = drv->backend->init(drv);
		if (ret) {
			drv_array_destroy(drv->combos);
			goto free_mappings;
		}
	}

	return drv;

free_mappings:
	drv_array_destroy(drv->mappings);
free_buffer_table:
	drmHashDestroy(drv->buffer_table);
free_lock:
	pthread_mutex_destroy(&drv->driver_lock);
free_driver:
	free(drv);
	return NULL;
}

void drv_destroy(struct driver *drv)
{
	pthread_mutex_lock(&drv->driver_lock);

	if (drv->backend->close)
		drv->backend->close(drv);

	drmHashDestroy(drv->buffer_table);
	drv_array_destroy(drv->mappings);
	drv_array_destroy(drv->combos);

	pthread_mutex_unlock(&drv->driver_lock);
	pthread_mutex_destroy(&drv->driver_lock);

	free(drv);
}

int drv_get_fd(struct driver *drv)
{
	return drv->fd;
}

const char *drv_get_name(struct driver *drv)
{
	return drv->backend->name;
}

struct combination *drv_get_combination(struct driver *drv, uint32_t format, uint64_t use_flags)
{
	struct combination *curr, *best;

	if (format == DRM_FORMAT_NONE || use_flags == BO_USE_NONE)
		return 0;

	best = NULL;
	uint32_t i;
	for (i = 0; i < drv_array_size(drv->combos); i++) {
		curr = drv_array_at_idx(drv->combos, i);
		if ((format == curr->format) && use_flags == (curr->use_flags & use_flags))
			if (!best || best->metadata.priority < curr->metadata.priority)
				best = curr;
	}

	return best;
}

struct bo *drv_bo_new(struct driver *drv, uint32_t width, uint32_t height, uint32_t format,
		      uint64_t use_flags)
{

	struct bo *bo;
	bo = (struct bo *)calloc(1, sizeof(*bo));

	if (!bo)
		return NULL;

	bo->drv = drv;
	bo->width = width;
	bo->height = height;
	bo->format = format;
	bo->use_flags = use_flags;
	bo->num_planes = drv_num_planes_from_format(format);

	if (!bo->num_planes) {
		free(bo);
		return NULL;
	}

	return bo;
}

struct bo *drv_bo_create(struct driver *drv, uint32_t width, uint32_t height, uint32_t format,
			 uint64_t use_flags)
{
	int ret;
	size_t plane;
	struct bo *bo;

	bo = drv_bo_new(drv, width, height, format, use_flags);

	if (!bo)
		return NULL;

	ret = drv->backend->bo_create(bo, width, height, format, use_flags);

	if (ret) {
		free(bo);
		return NULL;
	}

	pthread_mutex_lock(&drv->driver_lock);

	for (plane = 0; plane < bo->num_planes; plane++) {
		if (plane > 0)
			assert(bo->offsets[plane] >= bo->offsets[plane - 1]);

		drv_increment_reference_count(drv, bo, plane);
	}

	pthread_mutex_unlock(&drv->driver_lock);

	return bo;
}

struct bo *drv_bo_create_with_modifiers(struct driver *drv, uint32_t width, uint32_t height,
					uint32_t format, const uint64_t *modifiers, uint32_t count)
{
	int ret;
	size_t plane;
	struct bo *bo;

	if (!drv->backend->bo_create_with_modifiers) {
		errno = ENOENT;
		return NULL;
	}

	bo = drv_bo_new(drv, width, height, format, BO_USE_NONE);

	if (!bo)
		return NULL;

	ret = drv->backend->bo_create_with_modifiers(bo, width, height, format, modifiers, count);

	if (ret) {
		free(bo);
		return NULL;
	}

	pthread_mutex_lock(&drv->driver_lock);

	for (plane = 0; plane < bo->num_planes; plane++) {
		if (plane > 0)
			assert(bo->offsets[plane] >= bo->offsets[plane - 1]);

		drv_increment_reference_count(drv, bo, plane);
	}

	pthread_mutex_unlock(&drv->driver_lock);

	return bo;
}

void drv_bo_destroy(struct bo *bo)
{
	size_t plane;
	uintptr_t total = 0;
	struct driver *drv = bo->drv;

	pthread_mutex_lock(&drv->driver_lock);

	for (plane = 0; plane < bo->num_planes; plane++)
		drv_decrement_reference_count(drv, bo, plane);

	for (plane = 0; plane < bo->num_planes; plane++)
		total += drv_get_reference_count(drv, bo, plane);

	pthread_mutex_unlock(&drv->driver_lock);

	if (total == 0) {
		assert(drv_mapping_destroy(bo) == 0);
		bo->drv->backend->bo_destroy(bo);
	}

	free(bo);
}

struct bo *drv_bo_import(struct driver *drv, struct drv_import_fd_data *data)
{
	int ret;
	size_t plane;
	struct bo *bo;
	off_t seek_end;

	bo = drv_bo_new(drv, data->width, data->height, data->format, data->use_flags);

	if (!bo)
		return NULL;

	ret = drv->backend->bo_import(bo, data);
	if (ret) {
		free(bo);
		return NULL;
	}

	for (plane = 0; plane < bo->num_planes; plane++) {
		bo->strides[plane] = data->strides[plane];
		bo->offsets[plane] = data->offsets[plane];
		bo->format_modifiers[plane] = data->format_modifiers[plane];

		seek_end = lseek(data->fds[plane], 0, SEEK_END);
		if (seek_end == (off_t)(-1)) {
			fprintf(stderr, "drv: lseek() failed with %s\n", strerror(errno));
			goto destroy_bo;
		}

		lseek(data->fds[plane], 0, SEEK_SET);
		if (plane == bo->num_planes - 1 || data->offsets[plane + 1] == 0)
			bo->sizes[plane] = seek_end - data->offsets[plane];
		else
			bo->sizes[plane] = data->offsets[plane + 1] - data->offsets[plane];

		if ((int64_t)bo->offsets[plane] + bo->sizes[plane] > seek_end) {
			fprintf(stderr, "drv: buffer size is too large.\n");
			goto destroy_bo;
		}

		bo->total_size += bo->sizes[plane];
	}

	return bo;

destroy_bo:
	drv_bo_destroy(bo);
	return NULL;
}

void *drv_bo_map(struct bo *bo, const struct rectangle *rect, uint32_t map_flags,
		 struct mapping **map_data, size_t plane)
{
	uint32_t i;
	uint8_t *addr;
	struct mapping mapping;

	assert(rect->width >= 0);
	assert(rect->height >= 0);
	assert(rect->x + rect->width <= drv_bo_get_width(bo));
	assert(rect->y + rect->height <= drv_bo_get_height(bo));
	assert(BO_MAP_READ_WRITE & map_flags);
	/* No CPU access for protected buffers. */
	assert(!(bo->use_flags & BO_USE_PROTECTED));

	memset(&mapping, 0, sizeof(mapping));
	mapping.rect = *rect;
	mapping.refcount = 1;

	pthread_mutex_lock(&bo->drv->driver_lock);

	for (i = 0; i < drv_array_size(bo->drv->mappings); i++) {
		struct mapping *prior = (struct mapping *)drv_array_at_idx(bo->drv->mappings, i);
		if (prior->vma->handle != bo->handles[plane].u32 ||
		    prior->vma->map_flags != map_flags)
			continue;

		if (rect->x != prior->rect.x || rect->y != prior->rect.y ||
		    rect->width != prior->rect.width || rect->height != prior->rect.height)
			continue;

		prior->refcount++;
		*map_data = prior;
		goto exact_match;
	}

	for (i = 0; i < drv_array_size(bo->drv->mappings); i++) {
		struct mapping *prior = (struct mapping *)drv_array_at_idx(bo->drv->mappings, i);
		if (prior->vma->handle != bo->handles[plane].u32 ||
		    prior->vma->map_flags != map_flags)
			continue;

		prior->vma->refcount++;
		mapping.vma = prior->vma;
		goto success;
	}

	mapping.vma = calloc(1, sizeof(*mapping.vma));
	addr = bo->drv->backend->bo_map(bo, mapping.vma, plane, map_flags);
	if (addr == MAP_FAILED) {
		*map_data = NULL;
		free(mapping.vma);
		pthread_mutex_unlock(&bo->drv->driver_lock);
		return MAP_FAILED;
	}

	mapping.vma->refcount = 1;
	mapping.vma->addr = addr;
	mapping.vma->handle = bo->handles[plane].u32;
	mapping.vma->map_flags = map_flags;

success:
	*map_data = drv_array_append(bo->drv->mappings, &mapping);
exact_match:
	drv_bo_invalidate(bo, *map_data);
	addr = (uint8_t *)((*map_data)->vma->addr);
	addr += drv_bo_get_plane_offset(bo, plane);
	pthread_mutex_unlock(&bo->drv->driver_lock);
	return (void *)addr;
}

int drv_bo_unmap(struct bo *bo, struct mapping *mapping)
{
	uint32_t i;
	int ret = drv_bo_flush(bo, mapping);
	if (ret)
		return ret;

	pthread_mutex_lock(&bo->drv->driver_lock);

	if (--mapping->refcount)
		goto out;

	if (!--mapping->vma->refcount) {
		ret = bo->drv->backend->bo_unmap(bo, mapping->vma);
		free(mapping->vma);
	}

	for (i = 0; i < drv_array_size(bo->drv->mappings); i++) {
		if (mapping == (struct mapping *)drv_array_at_idx(bo->drv->mappings, i)) {
			drv_array_remove(bo->drv->mappings, i);
			break;
		}
	}

out:
	pthread_mutex_unlock(&bo->drv->driver_lock);
	return ret;
}

int drv_bo_invalidate(struct bo *bo, struct mapping *mapping)
{
	int ret = 0;

	assert(mapping);
	assert(mapping->vma);
	assert(mapping->refcount > 0);
	assert(mapping->vma->refcount > 0);

	if (bo->drv->backend->bo_invalidate)
		ret = bo->drv->backend->bo_invalidate(bo, mapping);

	return ret;
}

int drv_bo_flush(struct bo *bo, struct mapping *mapping)
{
	int ret = 0;

	assert(mapping);
	assert(mapping->vma);
	assert(mapping->refcount > 0);
	assert(mapping->vma->refcount > 0);
	assert(!(bo->use_flags & BO_USE_PROTECTED));

	if (bo->drv->backend->bo_flush)
		ret = bo->drv->backend->bo_flush(bo, mapping);

	return ret;
}

uint32_t drv_bo_get_width(struct bo *bo)
{
	return bo->width;
}

uint32_t drv_bo_get_height(struct bo *bo)
{
	return bo->height;
}

uint32_t drv_bo_get_stride_or_tiling(struct bo *bo)
{
	return bo->tiling ? bo->tiling : drv_bo_get_plane_stride(bo, 0);
}

size_t drv_bo_get_num_planes(struct bo *bo)
{
	return bo->num_planes;
}

union bo_handle drv_bo_get_plane_handle(struct bo *bo, size_t plane)
{
	return bo->handles[plane];
}

#ifndef DRM_RDWR
#define DRM_RDWR O_RDWR
#endif

int drv_bo_get_plane_fd(struct bo *bo, size_t plane)
{

	int ret, fd;
	assert(plane < bo->num_planes);

	ret = drmPrimeHandleToFD(bo->drv->fd, bo->handles[plane].u32, DRM_CLOEXEC | DRM_RDWR, &fd);

	return (ret) ? ret : fd;
}

uint32_t drv_bo_get_plane_offset(struct bo *bo, size_t plane)
{
	assert(plane < bo->num_planes);
	return bo->offsets[plane];
}

uint32_t drv_bo_get_plane_size(struct bo *bo, size_t plane)
{
	assert(plane < bo->num_planes);
	return bo->sizes[plane];
}

uint32_t drv_bo_get_plane_stride(struct bo *bo, size_t plane)
{
	assert(plane < bo->num_planes);
	return bo->strides[plane];
}

uint64_t drv_bo_get_plane_format_modifier(struct bo *bo, size_t plane)
{
	assert(plane < bo->num_planes);
	return bo->format_modifiers[plane];
}

uint32_t drv_bo_get_format(struct bo *bo)
{
	return bo->format;
}

uint32_t drv_resolve_format(struct driver *drv, uint32_t format, uint64_t use_flags)
{
	if (drv->backend->resolve_format)
		return drv->backend->resolve_format(format, use_flags);

	return format;
}

size_t drv_num_planes_from_format(uint32_t format)
{
	switch (format) {
	case DRM_FORMAT_ABGR1555:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_ABGR4444:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_ARGB1555:
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ARGB4444:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_AYUV:
	case DRM_FORMAT_BGR233:
	case DRM_FORMAT_BGR565:
	case DRM_FORMAT_BGR888:
	case DRM_FORMAT_BGRA1010102:
	case DRM_FORMAT_BGRA4444:
	case DRM_FORMAT_BGRA5551:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_BGRX1010102:
	case DRM_FORMAT_BGRX4444:
	case DRM_FORMAT_BGRX5551:
	case DRM_FORMAT_BGRX8888:
	case DRM_FORMAT_C8:
	case DRM_FORMAT_GR88:
	case DRM_FORMAT_R8:
	case DRM_FORMAT_RG88:
	case DRM_FORMAT_RGB332:
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_RGBA4444:
	case DRM_FORMAT_RGBA5551:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_RGBX1010102:
	case DRM_FORMAT_RGBX4444:
	case DRM_FORMAT_RGBX5551:
	case DRM_FORMAT_RGBX8888:
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_VYUY:
	case DRM_FORMAT_XBGR1555:
	case DRM_FORMAT_XBGR2101010:
	case DRM_FORMAT_XBGR4444:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_XRGB1555:
	case DRM_FORMAT_XRGB2101010:
	case DRM_FORMAT_XRGB4444:
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_YUYV:
	case DRM_FORMAT_YVYU:
		return 1;
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
		return 2;
	case DRM_FORMAT_YVU420:
	case DRM_FORMAT_YVU420_ANDROID:
		return 3;
	}

	fprintf(stderr, "drv: UNKNOWN FORMAT %d\n", format);
	return 0;
}

uint32_t drv_num_buffers_per_bo(struct bo *bo)
{
	uint32_t count = 0;
	size_t plane, p;

	for (plane = 0; plane < bo->num_planes; plane++) {
		for (p = 0; p < plane; p++)
			if (bo->handles[p].u32 == bo->handles[plane].u32)
				break;
		if (p == plane)
			count++;
	}

	return count;
}
