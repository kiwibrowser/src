/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <xf86drm.h>

#include "drv.h"
#include "gbm_helpers.h"
#include "gbm_priv.h"
#include "util.h"

PUBLIC int gbm_device_get_fd(struct gbm_device *gbm)
{

	return drv_get_fd(gbm->drv);
}

PUBLIC const char *gbm_device_get_backend_name(struct gbm_device *gbm)
{
	return drv_get_name(gbm->drv);
}

PUBLIC int gbm_device_is_format_supported(struct gbm_device *gbm, uint32_t format, uint32_t usage)
{
	uint64_t use_flags;

	if (usage & GBM_BO_USE_CURSOR && usage & GBM_BO_USE_RENDERING)
		return 0;

	use_flags = gbm_convert_usage(usage);

	return (drv_get_combination(gbm->drv, format, use_flags) != NULL);
}

PUBLIC struct gbm_device *gbm_create_device(int fd)
{
	struct gbm_device *gbm;

	gbm = (struct gbm_device *)malloc(sizeof(*gbm));

	if (!gbm)
		return NULL;

	gbm->drv = drv_create(fd);
	if (!gbm->drv) {
		free(gbm);
		return NULL;
	}

	return gbm;
}

PUBLIC void gbm_device_destroy(struct gbm_device *gbm)
{
	drv_destroy(gbm->drv);
	free(gbm);
}

PUBLIC struct gbm_surface *gbm_surface_create(struct gbm_device *gbm, uint32_t width,
					      uint32_t height, uint32_t format, uint32_t usage)
{
	struct gbm_surface *surface = (struct gbm_surface *)malloc(sizeof(*surface));

	if (!surface)
		return NULL;

	return surface;
}

PUBLIC void gbm_surface_destroy(struct gbm_surface *surface)
{
	free(surface);
}

PUBLIC struct gbm_bo *gbm_surface_lock_front_buffer(struct gbm_surface *surface)
{
	return NULL;
}

PUBLIC void gbm_surface_release_buffer(struct gbm_surface *surface, struct gbm_bo *bo)
{
}

static struct gbm_bo *gbm_bo_new(struct gbm_device *gbm, uint32_t format)
{
	struct gbm_bo *bo;

	bo = (struct gbm_bo *)calloc(1, sizeof(*bo));
	if (!bo)
		return NULL;

	bo->gbm = gbm;
	bo->gbm_format = format;

	return bo;
}

PUBLIC struct gbm_bo *gbm_bo_create(struct gbm_device *gbm, uint32_t width, uint32_t height,
				    uint32_t format, uint32_t usage)
{
	struct gbm_bo *bo;

	if (!gbm_device_is_format_supported(gbm, format, usage))
		return NULL;

	bo = gbm_bo_new(gbm, format);

	if (!bo)
		return NULL;

	bo->bo = drv_bo_create(gbm->drv, width, height, format, gbm_convert_usage(usage));

	if (!bo->bo) {
		free(bo);
		return NULL;
	}

	return bo;
}

PUBLIC struct gbm_bo *gbm_bo_create_with_modifiers(struct gbm_device *gbm, uint32_t width,
						   uint32_t height, uint32_t format,
						   const uint64_t *modifiers, uint32_t count)
{
	struct gbm_bo *bo;

	bo = gbm_bo_new(gbm, format);

	if (!bo)
		return NULL;

	bo->bo = drv_bo_create_with_modifiers(gbm->drv, width, height, format, modifiers, count);

	if (!bo->bo) {
		free(bo);
		return NULL;
	}

	return bo;
}

PUBLIC void gbm_bo_destroy(struct gbm_bo *bo)
{
	if (bo->destroy_user_data) {
		bo->destroy_user_data(bo, bo->user_data);
		bo->destroy_user_data = NULL;
		bo->user_data = NULL;
	}

	drv_bo_destroy(bo->bo);
	free(bo);
}

PUBLIC struct gbm_bo *gbm_bo_import(struct gbm_device *gbm, uint32_t type, void *buffer,
				    uint32_t usage)
{
	struct gbm_bo *bo;
	struct drv_import_fd_data drv_data;
	struct gbm_import_fd_data *fd_data = buffer;
	struct gbm_import_fd_planar_data *fd_planar_data = buffer;
	uint32_t gbm_format;
	size_t num_planes, i;

	memset(&drv_data, 0, sizeof(drv_data));
	drv_data.use_flags = gbm_convert_usage(usage);
	switch (type) {
	case GBM_BO_IMPORT_FD:
		gbm_format = fd_data->format;
		drv_data.width = fd_data->width;
		drv_data.height = fd_data->height;
		drv_data.format = fd_data->format;
		drv_data.fds[0] = fd_data->fd;
		drv_data.strides[0] = fd_data->stride;
		break;
	case GBM_BO_IMPORT_FD_PLANAR:
		gbm_format = fd_planar_data->format;
		drv_data.width = fd_planar_data->width;
		drv_data.height = fd_planar_data->height;
		drv_data.format = fd_planar_data->format;
		num_planes = drv_num_planes_from_format(drv_data.format);

		assert(num_planes);

		for (i = 0; i < num_planes; i++) {
			drv_data.fds[i] = fd_planar_data->fds[i];
			drv_data.offsets[i] = fd_planar_data->offsets[i];
			drv_data.strides[i] = fd_planar_data->strides[i];
			drv_data.format_modifiers[i] = fd_planar_data->format_modifiers[i];
		}

		for (i = num_planes; i < GBM_MAX_PLANES; i++)
			drv_data.fds[i] = -1;

		break;
	default:
		return NULL;
	}

	if (!gbm_device_is_format_supported(gbm, gbm_format, usage))
		return NULL;

	bo = gbm_bo_new(gbm, gbm_format);

	if (!bo)
		return NULL;

	bo->bo = drv_bo_import(gbm->drv, &drv_data);

	if (!bo->bo) {
		free(bo);
		return NULL;
	}

	return bo;
}

PUBLIC void *gbm_bo_map(struct gbm_bo *bo, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
			uint32_t transfer_flags, uint32_t *stride, void **map_data, size_t plane)
{
	void *addr;
	off_t offset;
	uint32_t map_flags;
	struct rectangle rect = { .x = x, .y = y, .width = width, .height = height };
	if (!bo || width == 0 || height == 0 || !stride || !map_data)
		return NULL;

	*stride = gbm_bo_get_plane_stride(bo, plane);
	map_flags = (transfer_flags & GBM_BO_TRANSFER_READ) ? BO_MAP_READ : BO_MAP_NONE;
	map_flags |= (transfer_flags & GBM_BO_TRANSFER_WRITE) ? BO_MAP_WRITE : BO_MAP_NONE;

	addr = drv_bo_map(bo->bo, &rect, map_flags, (struct mapping **)map_data, plane);
	if (addr == MAP_FAILED)
		return MAP_FAILED;

	offset = gbm_bo_get_plane_stride(bo, plane) * rect.y;
	offset += drv_stride_from_format(bo->gbm_format, rect.x, plane);
	return (void *)((uint8_t *)addr + offset);
}

PUBLIC void gbm_bo_unmap(struct gbm_bo *bo, void *map_data)
{
	assert(bo);
	drv_bo_flush(bo->bo, map_data);
}

PUBLIC uint32_t gbm_bo_get_width(struct gbm_bo *bo)
{
	return drv_bo_get_width(bo->bo);
}

PUBLIC uint32_t gbm_bo_get_height(struct gbm_bo *bo)
{
	return drv_bo_get_height(bo->bo);
}

PUBLIC uint32_t gbm_bo_get_stride(struct gbm_bo *bo)
{
	return gbm_bo_get_plane_stride(bo, 0);
}

PUBLIC uint32_t gbm_bo_get_stride_or_tiling(struct gbm_bo *bo)
{
	return drv_bo_get_stride_or_tiling(bo->bo);
}

PUBLIC uint32_t gbm_bo_get_format(struct gbm_bo *bo)
{
	return bo->gbm_format;
}

PUBLIC uint64_t gbm_bo_get_format_modifier(struct gbm_bo *bo)
{
	return gbm_bo_get_plane_format_modifier(bo, 0);
}

PUBLIC struct gbm_device *gbm_bo_get_device(struct gbm_bo *bo)
{
	return bo->gbm;
}

PUBLIC union gbm_bo_handle gbm_bo_get_handle(struct gbm_bo *bo)
{
	return gbm_bo_get_plane_handle(bo, 0);
}

PUBLIC int gbm_bo_get_fd(struct gbm_bo *bo)
{
	return gbm_bo_get_plane_fd(bo, 0);
}

PUBLIC size_t gbm_bo_get_num_planes(struct gbm_bo *bo)
{
	return drv_bo_get_num_planes(bo->bo);
}

PUBLIC union gbm_bo_handle gbm_bo_get_plane_handle(struct gbm_bo *bo, size_t plane)
{
	return (union gbm_bo_handle)drv_bo_get_plane_handle(bo->bo, plane).u64;
}

PUBLIC int gbm_bo_get_plane_fd(struct gbm_bo *bo, size_t plane)
{
	return drv_bo_get_plane_fd(bo->bo, plane);
}

PUBLIC uint32_t gbm_bo_get_plane_offset(struct gbm_bo *bo, size_t plane)
{
	return drv_bo_get_plane_offset(bo->bo, plane);
}

PUBLIC uint32_t gbm_bo_get_plane_size(struct gbm_bo *bo, size_t plane)
{
	return drv_bo_get_plane_size(bo->bo, plane);
}

PUBLIC uint32_t gbm_bo_get_plane_stride(struct gbm_bo *bo, size_t plane)
{
	return drv_bo_get_plane_stride(bo->bo, plane);
}

PUBLIC uint64_t gbm_bo_get_plane_format_modifier(struct gbm_bo *bo, size_t plane)
{
	return drv_bo_get_plane_format_modifier(bo->bo, plane);
}

PUBLIC void gbm_bo_set_user_data(struct gbm_bo *bo, void *data,
				 void (*destroy_user_data)(struct gbm_bo *, void *))
{
	bo->user_data = data;
	bo->destroy_user_data = destroy_user_data;
}

PUBLIC void *gbm_bo_get_user_data(struct gbm_bo *bo)
{
	return bo->user_data;
}
