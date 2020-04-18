/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "../cros_gralloc_driver.h"

#include <cassert>
#include <hardware/gralloc.h>
#include <memory.h>

struct gralloc0_module {
	gralloc_module_t base;
	std::unique_ptr<alloc_device_t> alloc;
	std::unique_ptr<cros_gralloc_driver> driver;
	bool initialized;
	std::mutex initialization_mutex;
};

/* This enumeration must match the one in <gralloc_drm.h>.
 * The functions supported by this gralloc's temporary private API are listed
 * below. Use of these functions is highly discouraged and should only be
 * reserved for cases where no alternative to get same information (such as
 * querying ANativeWindow) exists.
 */
// clang-format off
enum {
	GRALLOC_DRM_GET_STRIDE,
	GRALLOC_DRM_GET_FORMAT,
	GRALLOC_DRM_GET_DIMENSIONS,
	GRALLOC_DRM_GET_BACKING_STORE,
};
// clang-format on

static uint64_t gralloc0_convert_usage(int usage)
{
	uint64_t use_flags = BO_USE_NONE;

	if (usage & GRALLOC_USAGE_CURSOR)
		use_flags |= BO_USE_NONE;
	if ((usage & GRALLOC_USAGE_SW_READ_MASK) == GRALLOC_USAGE_SW_READ_RARELY)
		use_flags |= BO_USE_SW_READ_RARELY;
	if ((usage & GRALLOC_USAGE_SW_READ_MASK) == GRALLOC_USAGE_SW_READ_OFTEN)
		use_flags |= BO_USE_SW_READ_OFTEN;
	if ((usage & GRALLOC_USAGE_SW_WRITE_MASK) == GRALLOC_USAGE_SW_WRITE_RARELY)
		use_flags |= BO_USE_SW_WRITE_RARELY;
	if ((usage & GRALLOC_USAGE_SW_WRITE_MASK) == GRALLOC_USAGE_SW_WRITE_OFTEN)
		use_flags |= BO_USE_SW_WRITE_OFTEN;
	if (usage & GRALLOC_USAGE_HW_TEXTURE)
		use_flags |= BO_USE_TEXTURE;
	if (usage & GRALLOC_USAGE_HW_RENDER)
		use_flags |= BO_USE_RENDERING;
	if (usage & GRALLOC_USAGE_HW_2D)
		use_flags |= BO_USE_RENDERING;
	if (usage & GRALLOC_USAGE_HW_COMPOSER)
		/* HWC wants to use display hardware, but can defer to OpenGL. */
		use_flags |= BO_USE_SCANOUT | BO_USE_TEXTURE;
	if (usage & GRALLOC_USAGE_HW_FB)
		use_flags |= BO_USE_NONE;
	if (usage & GRALLOC_USAGE_EXTERNAL_DISP)
		/*
		 * This flag potentially covers external display for the normal drivers (i915,
		 * rockchip) and usb monitors (evdi/udl). It's complicated so ignore it.
		 * */
		use_flags |= BO_USE_NONE;
	if (usage & GRALLOC_USAGE_PROTECTED)
		use_flags |= BO_USE_PROTECTED;
	if (usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
		/*HACK: See b/30054495 */
		use_flags |= BO_USE_SW_READ_OFTEN;
	if (usage & GRALLOC_USAGE_HW_CAMERA_WRITE)
		use_flags |= BO_USE_CAMERA_WRITE;
	if (usage & GRALLOC_USAGE_HW_CAMERA_READ)
		use_flags |= BO_USE_CAMERA_READ;
	if (usage & GRALLOC_USAGE_RENDERSCRIPT)
		use_flags |= BO_USE_RENDERSCRIPT;

	return use_flags;
}

static uint32_t gralloc0_convert_map_usage(int map_usage)
{
	uint32_t map_flags = BO_MAP_NONE;

	if (map_usage & GRALLOC_USAGE_SW_READ_MASK)
		map_flags |= BO_MAP_READ;
	if (map_usage & GRALLOC_USAGE_SW_WRITE_MASK)
		map_flags |= BO_MAP_WRITE;

	return map_flags;
}

static int gralloc0_alloc(alloc_device_t *dev, int w, int h, int format, int usage,
			  buffer_handle_t *handle, int *stride)
{
	int32_t ret;
	bool supported;
	struct cros_gralloc_buffer_descriptor descriptor;
	auto mod = (struct gralloc0_module *)dev->common.module;

	descriptor.width = w;
	descriptor.height = h;
	descriptor.droid_format = format;
	descriptor.producer_usage = descriptor.consumer_usage = usage;
	descriptor.drm_format = cros_gralloc_convert_format(format);
	descriptor.use_flags = gralloc0_convert_usage(usage);

	supported = mod->driver->is_supported(&descriptor);
	if (!supported && (usage & GRALLOC_USAGE_HW_COMPOSER)) {
		descriptor.use_flags &= ~BO_USE_SCANOUT;
		supported = mod->driver->is_supported(&descriptor);
	}

	if (!supported) {
		cros_gralloc_error("Unsupported combination -- HAL format: %u, HAL usage: %u, "
				   "drv_format: %4.4s, use_flags: %llu",
				   format, usage, reinterpret_cast<char *>(&descriptor.drm_format),
				   static_cast<unsigned long long>(descriptor.use_flags));
		return -EINVAL;
	}

	ret = mod->driver->allocate(&descriptor, handle);
	if (ret)
		return ret;

	auto hnd = cros_gralloc_convert_handle(*handle);
	*stride = hnd->pixel_stride;

	return 0;
}

static int gralloc0_free(alloc_device_t *dev, buffer_handle_t handle)
{
	auto mod = (struct gralloc0_module *)dev->common.module;
	return mod->driver->release(handle);
}

static int gralloc0_close(struct hw_device_t *dev)
{
	/* Memory is freed by managed pointers on process close. */
	return 0;
}

static int gralloc0_init(struct gralloc0_module *mod, bool initialize_alloc)
{
	std::lock_guard<std::mutex> lock(mod->initialization_mutex);

	if (mod->initialized)
		return 0;

	mod->driver = std::make_unique<cros_gralloc_driver>();
	if (mod->driver->init()) {
		cros_gralloc_error("Failed to initialize driver.");
		return -ENODEV;
	}

	if (initialize_alloc) {
		mod->alloc = std::make_unique<alloc_device_t>();
		mod->alloc->alloc = gralloc0_alloc;
		mod->alloc->free = gralloc0_free;
		mod->alloc->common.tag = HARDWARE_DEVICE_TAG;
		mod->alloc->common.version = 0;
		mod->alloc->common.module = (hw_module_t *)mod;
		mod->alloc->common.close = gralloc0_close;
	}

	mod->initialized = true;
	return 0;
}

static int gralloc0_open(const struct hw_module_t *mod, const char *name, struct hw_device_t **dev)
{
	auto module = (struct gralloc0_module *)mod;

	if (module->initialized) {
		*dev = &module->alloc->common;
		return 0;
	}

	if (strcmp(name, GRALLOC_HARDWARE_GPU0)) {
		cros_gralloc_error("Incorrect device name - %s.", name);
		return -EINVAL;
	}

	if (gralloc0_init(module, true))
		return -ENODEV;

	*dev = &module->alloc->common;
	return 0;
}

static int gralloc0_register_buffer(struct gralloc_module_t const *module, buffer_handle_t handle)
{
	auto mod = (struct gralloc0_module *)module;

	if (!mod->initialized)
		if (gralloc0_init(mod, false))
			return -ENODEV;

	return mod->driver->retain(handle);
}

static int gralloc0_unregister_buffer(struct gralloc_module_t const *module, buffer_handle_t handle)
{
	auto mod = (struct gralloc0_module *)module;
	return mod->driver->release(handle);
}

static int gralloc0_lock(struct gralloc_module_t const *module, buffer_handle_t handle, int usage,
			 int l, int t, int w, int h, void **vaddr)
{
	return module->lockAsync(module, handle, usage, l, t, w, h, vaddr, -1);
}

static int gralloc0_unlock(struct gralloc_module_t const *module, buffer_handle_t handle)
{
	int32_t fence_fd, ret;
	auto mod = (struct gralloc0_module *)module;
	ret = mod->driver->unlock(handle, &fence_fd);
	if (ret)
		return ret;

	ret = cros_gralloc_sync_wait(fence_fd);
	if (ret)
		return ret;

	return 0;
}

static int gralloc0_perform(struct gralloc_module_t const *module, int op, ...)
{
	va_list args;
	int32_t *out_format, ret;
	uint64_t *out_store;
	buffer_handle_t handle;
	uint32_t *out_width, *out_height, *out_stride;
	auto mod = (struct gralloc0_module *)module;

	switch (op) {
	case GRALLOC_DRM_GET_STRIDE:
	case GRALLOC_DRM_GET_FORMAT:
	case GRALLOC_DRM_GET_DIMENSIONS:
	case GRALLOC_DRM_GET_BACKING_STORE:
		break;
	default:
		return -EINVAL;
	}

	va_start(args, op);

	ret = 0;
	handle = va_arg(args, buffer_handle_t);
	auto hnd = cros_gralloc_convert_handle(handle);
	if (!hnd) {
		cros_gralloc_error("Invalid handle.");
		return -EINVAL;
	}

	switch (op) {
	case GRALLOC_DRM_GET_STRIDE:
		out_stride = va_arg(args, uint32_t *);
		*out_stride = hnd->pixel_stride;
		break;
	case GRALLOC_DRM_GET_FORMAT:
		out_format = va_arg(args, int32_t *);
		*out_format = hnd->droid_format;
		break;
	case GRALLOC_DRM_GET_DIMENSIONS:
		out_width = va_arg(args, uint32_t *);
		out_height = va_arg(args, uint32_t *);
		*out_width = hnd->width;
		*out_height = hnd->height;
		break;
	case GRALLOC_DRM_GET_BACKING_STORE:
		out_store = va_arg(args, uint64_t *);
		ret = mod->driver->get_backing_store(handle, out_store);
		break;
	default:
		ret = -EINVAL;
	}

	va_end(args);

	return ret;
}

static int gralloc0_lock_ycbcr(struct gralloc_module_t const *module, buffer_handle_t handle,
			       int usage, int l, int t, int w, int h, struct android_ycbcr *ycbcr)
{
	return module->lockAsync_ycbcr(module, handle, usage, l, t, w, h, ycbcr, -1);
}

static int gralloc0_lock_async(struct gralloc_module_t const *module, buffer_handle_t handle,
			       int usage, int l, int t, int w, int h, void **vaddr, int fence_fd)
{
	int32_t ret;
	uint32_t map_flags;
	uint8_t *addr[DRV_MAX_PLANES];
	auto mod = (struct gralloc0_module *)module;
	struct rectangle rect = { .x = static_cast<uint32_t>(l),
				  .y = static_cast<uint32_t>(t),
				  .width = static_cast<uint32_t>(w),
				  .height = static_cast<uint32_t>(h) };

	auto hnd = cros_gralloc_convert_handle(handle);
	if (!hnd) {
		cros_gralloc_error("Invalid handle.");
		return -EINVAL;
	}

	if (hnd->droid_format == HAL_PIXEL_FORMAT_YCbCr_420_888) {
		cros_gralloc_error("HAL_PIXEL_FORMAT_YCbCr_*_888 format not compatible.");
		return -EINVAL;
	}

	assert(l >= 0);
	assert(t >= 0);
	assert(w >= 0);
	assert(h >= 0);

	map_flags = gralloc0_convert_map_usage(usage);
	ret = mod->driver->lock(handle, fence_fd, &rect, map_flags, addr);
	*vaddr = addr[0];
	return ret;
}

static int gralloc0_unlock_async(struct gralloc_module_t const *module, buffer_handle_t handle,
				 int *fence_fd)
{
	auto mod = (struct gralloc0_module *)module;
	return mod->driver->unlock(handle, fence_fd);
}

static int gralloc0_lock_async_ycbcr(struct gralloc_module_t const *module, buffer_handle_t handle,
				     int usage, int l, int t, int w, int h,
				     struct android_ycbcr *ycbcr, int fence_fd)
{
	int32_t ret;
	uint32_t map_flags;
	uint8_t *addr[DRV_MAX_PLANES] = { nullptr, nullptr, nullptr, nullptr };
	auto mod = (struct gralloc0_module *)module;
	struct rectangle rect = { .x = static_cast<uint32_t>(l),
				  .y = static_cast<uint32_t>(t),
				  .width = static_cast<uint32_t>(w),
				  .height = static_cast<uint32_t>(h) };

	auto hnd = cros_gralloc_convert_handle(handle);
	if (!hnd) {
		cros_gralloc_error("Invalid handle.");
		return -EINVAL;
	}

	if ((hnd->droid_format != HAL_PIXEL_FORMAT_YCbCr_420_888) &&
	    (hnd->droid_format != HAL_PIXEL_FORMAT_YV12) &&
	    (hnd->droid_format != HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)) {
		cros_gralloc_error("Non-YUV format not compatible.");
		return -EINVAL;
	}

	assert(l >= 0);
	assert(t >= 0);
	assert(w >= 0);
	assert(h >= 0);

	map_flags = gralloc0_convert_map_usage(usage);
	ret = mod->driver->lock(handle, fence_fd, &rect, map_flags, addr);
	if (ret)
		return ret;

	switch (hnd->format) {
	case DRM_FORMAT_NV12:
		ycbcr->y = addr[0];
		ycbcr->cb = addr[1];
		ycbcr->cr = addr[1] + 1;
		ycbcr->ystride = hnd->strides[0];
		ycbcr->cstride = hnd->strides[1];
		ycbcr->chroma_step = 2;
		break;
	case DRM_FORMAT_YVU420:
	case DRM_FORMAT_YVU420_ANDROID:
		ycbcr->y = addr[0];
		ycbcr->cb = addr[2];
		ycbcr->cr = addr[1];
		ycbcr->ystride = hnd->strides[0];
		ycbcr->cstride = hnd->strides[1];
		ycbcr->chroma_step = 1;
		break;
	default:
		module->unlock(module, handle);
		return -EINVAL;
	}

	return 0;
}

// clang-format off
static struct hw_module_methods_t gralloc0_module_methods = { .open = gralloc0_open };
// clang-format on

struct gralloc0_module HAL_MODULE_INFO_SYM = {
	.base =
	    {
		.common =
		    {
			.tag = HARDWARE_MODULE_TAG,
			.module_api_version = GRALLOC_MODULE_API_VERSION_0_3,
			.hal_api_version = 0,
			.id = GRALLOC_HARDWARE_MODULE_ID,
			.name = "CrOS Gralloc",
			.author = "Chrome OS",
			.methods = &gralloc0_module_methods,
		    },

		.registerBuffer = gralloc0_register_buffer,
		.unregisterBuffer = gralloc0_unregister_buffer,
		.lock = gralloc0_lock,
		.unlock = gralloc0_unlock,
		.perform = gralloc0_perform,
		.lock_ycbcr = gralloc0_lock_ycbcr,
		.lockAsync = gralloc0_lock_async,
		.unlockAsync = gralloc0_unlock_async,
		.lockAsync_ycbcr = gralloc0_lock_async_ycbcr,
	    },

	.alloc = nullptr,
	.driver = nullptr,
	.initialized = false,
};
