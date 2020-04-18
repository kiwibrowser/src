/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "drv_priv.h"
#include "helpers.h"
#include "util.h"

static uint32_t subsample_stride(uint32_t stride, uint32_t format, size_t plane)
{

	if (plane != 0) {
		switch (format) {
		case DRM_FORMAT_YVU420:
		case DRM_FORMAT_YVU420_ANDROID:
			stride = DIV_ROUND_UP(stride, 2);
			break;
		}
	}

	return stride;
}

static uint32_t bpp_from_format(uint32_t format, size_t plane)
{
	assert(plane < drv_num_planes_from_format(format));

	switch (format) {
	case DRM_FORMAT_BGR233:
	case DRM_FORMAT_C8:
	case DRM_FORMAT_R8:
	case DRM_FORMAT_RGB332:
	case DRM_FORMAT_YVU420:
	case DRM_FORMAT_YVU420_ANDROID:
		return 8;

	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
		return (plane == 0) ? 8 : 4;

	case DRM_FORMAT_ABGR1555:
	case DRM_FORMAT_ABGR4444:
	case DRM_FORMAT_ARGB1555:
	case DRM_FORMAT_ARGB4444:
	case DRM_FORMAT_BGR565:
	case DRM_FORMAT_BGRA4444:
	case DRM_FORMAT_BGRA5551:
	case DRM_FORMAT_BGRX4444:
	case DRM_FORMAT_BGRX5551:
	case DRM_FORMAT_GR88:
	case DRM_FORMAT_RG88:
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_RGBA4444:
	case DRM_FORMAT_RGBA5551:
	case DRM_FORMAT_RGBX4444:
	case DRM_FORMAT_RGBX5551:
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_VYUY:
	case DRM_FORMAT_XBGR1555:
	case DRM_FORMAT_XBGR4444:
	case DRM_FORMAT_XRGB1555:
	case DRM_FORMAT_XRGB4444:
	case DRM_FORMAT_YUYV:
	case DRM_FORMAT_YVYU:
		return 16;

	case DRM_FORMAT_BGR888:
	case DRM_FORMAT_RGB888:
		return 24;

	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_AYUV:
	case DRM_FORMAT_BGRA1010102:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_BGRX1010102:
	case DRM_FORMAT_BGRX8888:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_RGBX1010102:
	case DRM_FORMAT_RGBX8888:
	case DRM_FORMAT_XBGR2101010:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_XRGB2101010:
	case DRM_FORMAT_XRGB8888:
		return 32;
	}

	fprintf(stderr, "drv: UNKNOWN FORMAT %d\n", format);
	return 0;
}

uint32_t drv_bo_get_stride_in_pixels(struct bo *bo)
{
	uint32_t bytes_per_pixel = DIV_ROUND_UP(bpp_from_format(bo->format, 0), 8);
	return DIV_ROUND_UP(bo->strides[0], bytes_per_pixel);
}

/*
 * This function returns the stride for a given format, width and plane.
 */
uint32_t drv_stride_from_format(uint32_t format, uint32_t width, size_t plane)
{
	uint32_t stride = DIV_ROUND_UP(width * bpp_from_format(format, plane), 8);

	/*
	 * The stride of Android YV12 buffers is required to be aligned to 16 bytes
	 * (see <system/graphics.h>).
	 */
	if (format == DRM_FORMAT_YVU420_ANDROID)
		stride = (plane == 0) ? ALIGN(stride, 32) : ALIGN(stride, 16);

	return stride;
}

uint32_t drv_size_from_format(uint32_t format, uint32_t stride, uint32_t height, size_t plane)
{
	assert(plane < drv_num_planes_from_format(format));
	uint32_t vertical_subsampling;

	switch (format) {
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_YVU420:
	case DRM_FORMAT_YVU420_ANDROID:
		vertical_subsampling = (plane == 0) ? 1 : 2;
		break;
	default:
		vertical_subsampling = 1;
	}

	return stride * DIV_ROUND_UP(height, vertical_subsampling);
}

/*
 * This function fills in the buffer object given the driver aligned stride of
 * the first plane, height and a format. This function assumes there is just
 * one kernel buffer per buffer object.
 */
int drv_bo_from_format(struct bo *bo, uint32_t stride, uint32_t aligned_height, uint32_t format)
{

	size_t p, num_planes;
	uint32_t offset = 0;

	num_planes = drv_num_planes_from_format(format);
	assert(num_planes);

	/*
	 * HAL_PIXEL_FORMAT_YV12 requires that (see <system/graphics.h>):
	 *  - the aligned height is same as the buffer's height.
	 *  - the chroma stride is 16 bytes aligned, i.e., the luma's strides
	 *    is 32 bytes aligned.
	 */
	if (format == DRM_FORMAT_YVU420_ANDROID) {
		assert(aligned_height == bo->height);
		assert(stride == ALIGN(stride, 32));
	}

	for (p = 0; p < num_planes; p++) {
		bo->strides[p] = subsample_stride(stride, format, p);
		bo->sizes[p] = drv_size_from_format(format, bo->strides[p], aligned_height, p);
		bo->offsets[p] = offset;
		offset += bo->sizes[p];
	}

	bo->total_size = offset;
	return 0;
}

int drv_dumb_bo_create(struct bo *bo, uint32_t width, uint32_t height, uint32_t format,
		       uint64_t use_flags)
{
	int ret;
	size_t plane;
	uint32_t aligned_width, aligned_height;
	struct drm_mode_create_dumb create_dumb;

	aligned_width = width;
	aligned_height = height;
	if (format == DRM_FORMAT_YVU420_ANDROID) {
		/*
		 * Align width to 32 pixels, so chroma strides are 16 bytes as
		 * Android requires.
		 */
		aligned_width = ALIGN(width, 32);
	}

	if (format == DRM_FORMAT_YVU420_ANDROID || format == DRM_FORMAT_YVU420) {
		aligned_height = 3 * DIV_ROUND_UP(height, 2);
	}

	memset(&create_dumb, 0, sizeof(create_dumb));
	create_dumb.height = aligned_height;
	create_dumb.width = aligned_width;
	create_dumb.bpp = bpp_from_format(format, 0);
	create_dumb.flags = 0;

	ret = drmIoctl(bo->drv->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb);
	if (ret) {
		fprintf(stderr, "drv: DRM_IOCTL_MODE_CREATE_DUMB failed\n");
		return ret;
	}

	drv_bo_from_format(bo, create_dumb.pitch, height, format);

	for (plane = 0; plane < bo->num_planes; plane++)
		bo->handles[plane].u32 = create_dumb.handle;

	bo->total_size = create_dumb.size;
	return 0;
}

int drv_dumb_bo_destroy(struct bo *bo)
{
	struct drm_mode_destroy_dumb destroy_dumb;
	int ret;

	memset(&destroy_dumb, 0, sizeof(destroy_dumb));
	destroy_dumb.handle = bo->handles[0].u32;

	ret = drmIoctl(bo->drv->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_dumb);
	if (ret) {
		fprintf(stderr, "drv: DRM_IOCTL_MODE_DESTROY_DUMB failed (handle=%x)\n",
			bo->handles[0].u32);
		return ret;
	}

	return 0;
}

int drv_gem_bo_destroy(struct bo *bo)
{
	struct drm_gem_close gem_close;
	int ret, error = 0;
	size_t plane, i;

	for (plane = 0; plane < bo->num_planes; plane++) {
		for (i = 0; i < plane; i++)
			if (bo->handles[i].u32 == bo->handles[plane].u32)
				break;
		/* Make sure close hasn't already been called on this handle */
		if (i != plane)
			continue;

		memset(&gem_close, 0, sizeof(gem_close));
		gem_close.handle = bo->handles[plane].u32;

		ret = drmIoctl(bo->drv->fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
		if (ret) {
			fprintf(stderr, "drv: DRM_IOCTL_GEM_CLOSE failed (handle=%x) error %d\n",
				bo->handles[plane].u32, ret);
			error = ret;
		}
	}

	return error;
}

int drv_prime_bo_import(struct bo *bo, struct drv_import_fd_data *data)
{
	int ret;
	size_t plane;
	struct drm_prime_handle prime_handle;

	for (plane = 0; plane < bo->num_planes; plane++) {
		memset(&prime_handle, 0, sizeof(prime_handle));
		prime_handle.fd = data->fds[plane];

		ret = drmIoctl(bo->drv->fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime_handle);

		if (ret) {
			fprintf(stderr, "drv: DRM_IOCTL_PRIME_FD_TO_HANDLE failed (fd=%u)\n",
				prime_handle.fd);

			/*
			 * Need to call GEM close on planes that were opened,
			 * if any. Adjust the num_planes variable to be the
			 * plane that failed, so GEM close will be called on
			 * planes before that plane.
			 */
			bo->num_planes = plane;
			drv_gem_bo_destroy(bo);
			return ret;
		}

		bo->handles[plane].u32 = prime_handle.handle;
	}

	for (plane = 0; plane < bo->num_planes; plane++) {
		pthread_mutex_lock(&bo->drv->driver_lock);
		drv_increment_reference_count(bo->drv, bo, plane);
		pthread_mutex_unlock(&bo->drv->driver_lock);
	}

	return 0;
}

void *drv_dumb_bo_map(struct bo *bo, struct vma *vma, size_t plane, uint32_t map_flags)
{
	int ret;
	size_t i;
	struct drm_mode_map_dumb map_dumb;

	memset(&map_dumb, 0, sizeof(map_dumb));
	map_dumb.handle = bo->handles[plane].u32;

	ret = drmIoctl(bo->drv->fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb);
	if (ret) {
		fprintf(stderr, "drv: DRM_IOCTL_MODE_MAP_DUMB failed \n");
		return MAP_FAILED;
	}

	for (i = 0; i < bo->num_planes; i++)
		if (bo->handles[i].u32 == bo->handles[plane].u32)
			vma->length += bo->sizes[i];

	return mmap(0, vma->length, drv_get_prot(map_flags), MAP_SHARED, bo->drv->fd,
		    map_dumb.offset);
}

int drv_bo_munmap(struct bo *bo, struct vma *vma)
{
	return munmap(vma->addr, vma->length);
}

int drv_mapping_destroy(struct bo *bo)
{
	int ret;
	size_t plane;
	struct mapping *mapping;
	uint32_t idx;

	/*
	 * This function is called right before the buffer is destroyed. It will free any mappings
	 * associated with the buffer.
	 */

	idx = 0;
	for (plane = 0; plane < bo->num_planes; plane++) {
		while (idx < drv_array_size(bo->drv->mappings)) {
			mapping = (struct mapping *)drv_array_at_idx(bo->drv->mappings, idx);
			if (mapping->vma->handle != bo->handles[plane].u32) {
				idx++;
				continue;
			}

			if (!--mapping->vma->refcount) {
				ret = bo->drv->backend->bo_unmap(bo, mapping->vma);
				if (ret) {
					fprintf(stderr, "drv: munmap failed");
					return ret;
				}

				free(mapping->vma);
			}

			/* This shrinks and shifts the array, so don't increment idx. */
			drv_array_remove(bo->drv->mappings, idx);
		}
	}

	return 0;
}

int drv_get_prot(uint32_t map_flags)
{
	return (BO_MAP_WRITE & map_flags) ? PROT_WRITE | PROT_READ : PROT_READ;
}

uintptr_t drv_get_reference_count(struct driver *drv, struct bo *bo, size_t plane)
{
	void *count;
	uintptr_t num = 0;

	if (!drmHashLookup(drv->buffer_table, bo->handles[plane].u32, &count))
		num = (uintptr_t)(count);

	return num;
}

void drv_increment_reference_count(struct driver *drv, struct bo *bo, size_t plane)
{
	uintptr_t num = drv_get_reference_count(drv, bo, plane);

	/* If a value isn't in the table, drmHashDelete is a no-op */
	drmHashDelete(drv->buffer_table, bo->handles[plane].u32);
	drmHashInsert(drv->buffer_table, bo->handles[plane].u32, (void *)(num + 1));
}

void drv_decrement_reference_count(struct driver *drv, struct bo *bo, size_t plane)
{
	uintptr_t num = drv_get_reference_count(drv, bo, plane);

	drmHashDelete(drv->buffer_table, bo->handles[plane].u32);

	if (num > 0)
		drmHashInsert(drv->buffer_table, bo->handles[plane].u32, (void *)(num - 1));
}

uint32_t drv_log_base2(uint32_t value)
{
	int ret = 0;

	while (value >>= 1)
		++ret;

	return ret;
}

void drv_add_combinations(struct driver *drv, const uint32_t *formats, uint32_t num_formats,
			  struct format_metadata *metadata, uint64_t use_flags)
{
	uint32_t i;

	for (i = 0; i < num_formats; i++) {
		struct combination combo = { .format = formats[i],
					     .metadata = *metadata,
					     .use_flags = use_flags };

		drv_array_append(drv->combos, &combo);
	}
}

void drv_modify_combination(struct driver *drv, uint32_t format, struct format_metadata *metadata,
			    uint64_t use_flags)
{
	uint32_t i;
	struct combination *combo;
	/* Attempts to add the specified flags to an existing combination. */
	for (i = 0; i < drv_array_size(drv->combos); i++) {
		combo = (struct combination *)drv_array_at_idx(drv->combos, i);
		if (combo->format == format && combo->metadata.tiling == metadata->tiling &&
		    combo->metadata.modifier == metadata->modifier)
			combo->use_flags |= use_flags;
	}
}

struct drv_array *drv_query_kms(struct driver *drv)
{
	struct drv_array *kms_items;
	uint64_t plane_type, use_flag;
	uint32_t i, j, k;

	drmModePlanePtr plane;
	drmModePropertyPtr prop;
	drmModePlaneResPtr resources;
	drmModeObjectPropertiesPtr props;

	kms_items = drv_array_init(sizeof(struct kms_item));
	if (!kms_items)
		goto out;

	/*
	 * The ability to return universal planes is only complete on
	 * ChromeOS kernel versions >= v3.18.  The SET_CLIENT_CAP ioctl
	 * therefore might return an error code, so don't check it.  If it
	 * fails, it'll just return the plane list as overlay planes, which is
	 * fine in our case (our drivers already have cursor bits set).
	 * modetest in libdrm does the same thing.
	 */
	drmSetClientCap(drv->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

	resources = drmModeGetPlaneResources(drv->fd);
	if (!resources)
		goto out;

	for (i = 0; i < resources->count_planes; i++) {
		plane = drmModeGetPlane(drv->fd, resources->planes[i]);
		if (!plane)
			goto out;

		props = drmModeObjectGetProperties(drv->fd, plane->plane_id, DRM_MODE_OBJECT_PLANE);
		if (!props)
			goto out;

		for (j = 0; j < props->count_props; j++) {
			prop = drmModeGetProperty(drv->fd, props->props[j]);
			if (prop) {
				if (strcmp(prop->name, "type") == 0) {
					plane_type = props->prop_values[j];
				}

				drmModeFreeProperty(prop);
			}
		}

		switch (plane_type) {
		case DRM_PLANE_TYPE_OVERLAY:
		case DRM_PLANE_TYPE_PRIMARY:
			use_flag = BO_USE_SCANOUT;
			break;
		case DRM_PLANE_TYPE_CURSOR:
			use_flag = BO_USE_CURSOR;
			break;
		default:
			assert(0);
		}

		for (j = 0; j < plane->count_formats; j++) {
			bool found = false;
			for (k = 0; k < drv_array_size(kms_items); k++) {
				struct kms_item *item = drv_array_at_idx(kms_items, k);
				if (item->format == plane->formats[j] &&
				    item->modifier == DRM_FORMAT_MOD_LINEAR) {
					item->use_flags |= use_flag;
					found = true;
					break;
				}
			}

			if (!found) {
				struct kms_item item = { .format = plane->formats[j],
							 .modifier = DRM_FORMAT_MOD_LINEAR,
							 .use_flags = use_flag };

				drv_array_append(kms_items, &item);
			}
		}

		drmModeFreeObjectProperties(props);
		drmModeFreePlane(plane);
	}

	drmModeFreePlaneResources(resources);
out:
	if (kms_items && !drv_array_size(kms_items)) {
		drv_array_destroy(kms_items);
		return NULL;
	}

	return kms_items;
}

int drv_modify_linear_combinations(struct driver *drv)
{
	uint32_t i, j;
	struct kms_item *item;
	struct combination *combo;
	struct drv_array *kms_items;

	/*
	 * All current drivers can scanout linear XRGB8888/ARGB8888 as a primary
	 * plane and as a cursor. Some drivers don't support
	 * drmModeGetPlaneResources, so add the combination here. Note that the
	 * kernel disregards the alpha component of ARGB unless it's an overlay
	 * plane.
	 */
	drv_modify_combination(drv, DRM_FORMAT_XRGB8888, &LINEAR_METADATA,
			       BO_USE_CURSOR | BO_USE_SCANOUT);
	drv_modify_combination(drv, DRM_FORMAT_ARGB8888, &LINEAR_METADATA,
			       BO_USE_CURSOR | BO_USE_SCANOUT);

	kms_items = drv_query_kms(drv);
	if (!kms_items)
		return 0;

	for (i = 0; i < drv_array_size(kms_items); i++) {
		item = (struct kms_item *)drv_array_at_idx(kms_items, i);
		for (j = 0; j < drv_array_size(drv->combos); j++) {
			combo = drv_array_at_idx(drv->combos, j);
			if (item->format == combo->format)
				combo->use_flags |= BO_USE_SCANOUT;
		}
	}

	drv_array_destroy(kms_items);
	return 0;
}

/*
 * Pick the best modifier from modifiers, according to the ordering
 * given by modifier_order.
 */
uint64_t drv_pick_modifier(const uint64_t *modifiers, uint32_t count,
			   const uint64_t *modifier_order, uint32_t order_count)
{
	uint32_t i, j;

	for (i = 0; i < order_count; i++) {
		for (j = 0; j < count; j++) {
			if (modifiers[j] == modifier_order[i]) {
				return modifiers[j];
			}
		}
	}

	return DRM_FORMAT_MOD_LINEAR;
}
