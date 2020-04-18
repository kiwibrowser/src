/*
 * Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cros_gralloc_driver.h"
#include "../util.h"

#include <cstdlib>
#include <fcntl.h>
#include <xf86drm.h>

cros_gralloc_driver::cros_gralloc_driver() : drv_(nullptr)
{
}

cros_gralloc_driver::~cros_gralloc_driver()
{
	buffers_.clear();
	handles_.clear();

	if (drv_) {
		drv_destroy(drv_);
		drv_ = nullptr;
	}
}

int32_t cros_gralloc_driver::init()
{
	/*
	 * Create a driver from rendernode while filtering out
	 * the specified undesired driver.
	 *
	 * TODO(gsingh): Enable render nodes on udl/evdi.
	 */

	int fd;
	drmVersionPtr version;
	char const *str = "%s/renderD%d";
	const char *undesired[2] = { "vgem", nullptr };
	uint32_t num_nodes = 63;
	uint32_t min_node = 128;
	uint32_t max_node = (min_node + num_nodes);

	for (uint32_t i = 0; i < ARRAY_SIZE(undesired); i++) {
		for (uint32_t j = min_node; j < max_node; j++) {
			char *node;
			if (asprintf(&node, str, DRM_DIR_NAME, j) < 0)
				continue;

			fd = open(node, O_RDWR, 0);
			free(node);

			if (fd < 0)
				continue;

			version = drmGetVersion(fd);
			if (!version)
				continue;

			if (undesired[i] && !strcmp(version->name, undesired[i])) {
				drmFreeVersion(version);
				continue;
			}

			drmFreeVersion(version);
			drv_ = drv_create(fd);
			if (drv_)
				return 0;
		}
	}

	return -ENODEV;
}

bool cros_gralloc_driver::is_supported(const struct cros_gralloc_buffer_descriptor *descriptor)
{
	struct combination *combo;
	uint32_t resolved_format;
	resolved_format = drv_resolve_format(drv_, descriptor->drm_format, descriptor->use_flags);
	combo = drv_get_combination(drv_, resolved_format, descriptor->use_flags);
	return (combo != nullptr);
}

int32_t cros_gralloc_driver::allocate(const struct cros_gralloc_buffer_descriptor *descriptor,
				      buffer_handle_t *out_handle)
{
	uint32_t id;
	uint64_t mod;
	size_t num_planes;
	uint32_t resolved_format;

	struct bo *bo;
	struct cros_gralloc_handle *hnd;

	resolved_format = drv_resolve_format(drv_, descriptor->drm_format, descriptor->use_flags);
	bo = drv_bo_create(drv_, descriptor->width, descriptor->height, resolved_format,
			   descriptor->use_flags);
	if (!bo) {
		cros_gralloc_error("Failed to create bo.");
		return -ENOMEM;
	}

	/*
	 * If there is a desire for more than one kernel buffer, this can be
	 * removed once the ArcCodec and Wayland service have the ability to
	 * send more than one fd. GL/Vulkan drivers may also have to modified.
	 */
	if (drv_num_buffers_per_bo(bo) != 1) {
		drv_bo_destroy(bo);
		cros_gralloc_error("Can only support one buffer per bo.");
		return -EINVAL;
	}

	hnd = new cros_gralloc_handle();
	num_planes = drv_bo_get_num_planes(bo);

	hnd->base.version = sizeof(hnd->base);
	hnd->base.numFds = num_planes;
	hnd->base.numInts = handle_data_size - num_planes;

	for (size_t plane = 0; plane < num_planes; plane++) {
		hnd->fds[plane] = drv_bo_get_plane_fd(bo, plane);
		hnd->strides[plane] = drv_bo_get_plane_stride(bo, plane);
		hnd->offsets[plane] = drv_bo_get_plane_offset(bo, plane);

		mod = drv_bo_get_plane_format_modifier(bo, plane);
		hnd->format_modifiers[2 * plane] = static_cast<uint32_t>(mod >> 32);
		hnd->format_modifiers[2 * plane + 1] = static_cast<uint32_t>(mod);
	}

	hnd->width = drv_bo_get_width(bo);
	hnd->height = drv_bo_get_height(bo);
	hnd->format = drv_bo_get_format(bo);
	hnd->use_flags[0] = static_cast<uint32_t>(descriptor->use_flags >> 32);
	hnd->use_flags[1] = static_cast<uint32_t>(descriptor->use_flags);
	hnd->pixel_stride = drv_bo_get_stride_in_pixels(bo);
	hnd->magic = cros_gralloc_magic;
	hnd->droid_format = descriptor->droid_format;
	hnd->usage = descriptor->producer_usage;

	id = drv_bo_get_plane_handle(bo, 0).u32;
	auto buffer = new cros_gralloc_buffer(id, bo, hnd);

	std::lock_guard<std::mutex> lock(mutex_);
	buffers_.emplace(id, buffer);
	handles_.emplace(hnd, std::make_pair(buffer, 1));
	*out_handle = &hnd->base;
	return 0;
}

int32_t cros_gralloc_driver::retain(buffer_handle_t handle)
{
	uint32_t id;
	std::lock_guard<std::mutex> lock(mutex_);

	auto hnd = cros_gralloc_convert_handle(handle);
	if (!hnd) {
		cros_gralloc_error("Invalid handle.");
		return -EINVAL;
	}

	auto buffer = get_buffer(hnd);
	if (buffer) {
		handles_[hnd].second++;
		buffer->increase_refcount();
		return 0;
	}

	if (drmPrimeFDToHandle(drv_get_fd(drv_), hnd->fds[0], &id)) {
		cros_gralloc_error("drmPrimeFDToHandle failed.");
		return -errno;
	}

	if (buffers_.count(id)) {
		buffer = buffers_[id];
		buffer->increase_refcount();
	} else {
		struct bo *bo;
		struct drv_import_fd_data data;
		data.format = hnd->format;
		data.width = hnd->width;
		data.height = hnd->height;
		data.use_flags = static_cast<uint64_t>(hnd->use_flags[0]) << 32;
		data.use_flags |= hnd->use_flags[1];

		memcpy(data.fds, hnd->fds, sizeof(data.fds));
		memcpy(data.strides, hnd->strides, sizeof(data.strides));
		memcpy(data.offsets, hnd->offsets, sizeof(data.offsets));
		for (uint32_t plane = 0; plane < DRV_MAX_PLANES; plane++) {
			data.format_modifiers[plane] =
			    static_cast<uint64_t>(hnd->format_modifiers[2 * plane]) << 32;
			data.format_modifiers[plane] |= hnd->format_modifiers[2 * plane + 1];
		}

		bo = drv_bo_import(drv_, &data);
		if (!bo)
			return -EFAULT;

		id = drv_bo_get_plane_handle(bo, 0).u32;

		buffer = new cros_gralloc_buffer(id, bo, nullptr);
		buffers_.emplace(id, buffer);
	}

	handles_.emplace(hnd, std::make_pair(buffer, 1));
	return 0;
}

int32_t cros_gralloc_driver::release(buffer_handle_t handle)
{
	std::lock_guard<std::mutex> lock(mutex_);

	auto hnd = cros_gralloc_convert_handle(handle);
	if (!hnd) {
		cros_gralloc_error("Invalid handle.");
		return -EINVAL;
	}

	auto buffer = get_buffer(hnd);
	if (!buffer) {
		cros_gralloc_error("Invalid Reference.");
		return -EINVAL;
	}

	if (!--handles_[hnd].second)
		handles_.erase(hnd);

	if (buffer->decrease_refcount() == 0) {
		buffers_.erase(buffer->get_id());
		delete buffer;
	}

	return 0;
}

int32_t cros_gralloc_driver::lock(buffer_handle_t handle, int32_t acquire_fence,
				  const struct rectangle *rect, uint32_t map_flags,
				  uint8_t *addr[DRV_MAX_PLANES])
{
	int32_t ret = cros_gralloc_sync_wait(acquire_fence);
	if (ret)
		return ret;

	std::lock_guard<std::mutex> lock(mutex_);
	auto hnd = cros_gralloc_convert_handle(handle);
	if (!hnd) {
		cros_gralloc_error("Invalid handle.");
		return -EINVAL;
	}

	auto buffer = get_buffer(hnd);
	if (!buffer) {
		cros_gralloc_error("Invalid Reference.");
		return -EINVAL;
	}

	return buffer->lock(rect, map_flags, addr);
}

int32_t cros_gralloc_driver::unlock(buffer_handle_t handle, int32_t *release_fence)
{
	std::lock_guard<std::mutex> lock(mutex_);

	auto hnd = cros_gralloc_convert_handle(handle);
	if (!hnd) {
		cros_gralloc_error("Invalid handle.");
		return -EINVAL;
	}

	auto buffer = get_buffer(hnd);
	if (!buffer) {
		cros_gralloc_error("Invalid Reference.");
		return -EINVAL;
	}

	/*
	 * From the ANativeWindow::dequeueBuffer documentation:
	 *
	 * "A value of -1 indicates that the caller may access the buffer immediately without
	 * waiting on a fence."
	 */
	*release_fence = -1;
	return buffer->unlock();
}

int32_t cros_gralloc_driver::get_backing_store(buffer_handle_t handle, uint64_t *out_store)
{
	std::lock_guard<std::mutex> lock(mutex_);

	auto hnd = cros_gralloc_convert_handle(handle);
	if (!hnd) {
		cros_gralloc_error("Invalid handle.");
		return -EINVAL;
	}

	auto buffer = get_buffer(hnd);
	if (!buffer) {
		cros_gralloc_error("Invalid Reference.");
		return -EINVAL;
	}

	*out_store = static_cast<uint64_t>(buffer->get_id());
	return 0;
}

cros_gralloc_buffer *cros_gralloc_driver::get_buffer(cros_gralloc_handle_t hnd)
{
	/* Assumes driver mutex is held. */
	if (handles_.count(hnd))
		return handles_[hnd].first;

	return nullptr;
}
