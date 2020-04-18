/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Please run clang-format on this file after making changes:
 *
 * clang-format -style=file -i gralloctest.c
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include <cutils/native_handle.h>
#include <hardware/gralloc.h>
#include <sync/sync.h>
#include <system/graphics.h>

#define ALIGN(A, B) (((A) + (B)-1) / (B) * (B))
#define ARRAY_SIZE(A) (sizeof(A) / sizeof(*(A)))

#define CHECK(cond)                                                                                \
	do {                                                                                       \
		if (!(cond)) {                                                                     \
			fprintf(stderr, "[  FAILED  ] check in %s() %s:%d\n", __func__, __FILE__,  \
				__LINE__);                                                         \
			return 0;                                                                  \
		}                                                                                  \
	} while (0)

#define CHECK_NO_MSG(cond)                                                                         \
	do {                                                                                       \
		if (!(cond)) {                                                                     \
			return 0;                                                                  \
		}                                                                                  \
	} while (0)

/* Private API enumeration -- see <gralloc_drm.h> */
enum { GRALLOC_DRM_GET_STRIDE,
       GRALLOC_DRM_GET_FORMAT,
       GRALLOC_DRM_GET_DIMENSIONS,
       GRALLOC_DRM_GET_BACKING_STORE,
};

struct gralloctest_context {
	struct gralloc_module_t *module;
	struct alloc_device_t *device;
	int api;
};

struct gralloc_testcase {
	const char *name;
	int (*run_test)(struct gralloctest_context *ctx);
	int required_api;
};

struct combinations {
	int32_t format;
	int32_t usage;
};

// clang-format off
static struct combinations combos[] = {
	{ HAL_PIXEL_FORMAT_RGBA_8888,
	  GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN |
	  GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_COMPOSER |
	  GRALLOC_USAGE_HW_FB | GRALLOC_USAGE_CURSOR },
	{ HAL_PIXEL_FORMAT_RGBA_8888,
	  GRALLOC_USAGE_HW_FB | GRALLOC_USAGE_HW_RENDER |
	  GRALLOC_USAGE_HW_COMPOSER },
	{ HAL_PIXEL_FORMAT_RGBX_8888,
	  GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN },
	{ HAL_PIXEL_FORMAT_YCbCr_420_888,
	  GRALLOC_USAGE_EXTERNAL_DISP | GRALLOC_USAGE_HW_COMPOSER |
	  GRALLOC_USAGE_HW_TEXTURE },
	{ HAL_PIXEL_FORMAT_YCbCr_420_888,
	  GRALLOC_USAGE_RENDERSCRIPT | GRALLOC_USAGE_SW_READ_OFTEN |
	  GRALLOC_USAGE_SW_WRITE_OFTEN },
	{ HAL_PIXEL_FORMAT_YV12,
	  GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_HW_COMPOSER |
	  GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP },
	{ HAL_PIXEL_FORMAT_RGB_565,
	  GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN },
	{ HAL_PIXEL_FORMAT_BGRA_8888,
	  GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN },
	{ HAL_PIXEL_FORMAT_BLOB,
	  GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN },
};
// clang-format on

struct grallocinfo {
	buffer_handle_t handle;     /* handle to the buffer */
	int w;			    /* width  of buffer */
	int h;			    /* height of buffer */
	int format;		    /* format of the buffer */
	int usage;		    /* bitfield indicating usage */
	int fence_fd;		    /* fence file descriptor */
	void *vaddr;		    /* buffer virtual memory address */
	int stride;		    /* stride in pixels */
	struct android_ycbcr ycbcr; /* sw access for yuv buffers */
};

/* This function is meant to initialize the test to commonly used defaults. */
void grallocinfo_init(struct grallocinfo *info, int w, int h, int format, int usage)
{
	info->w = w;
	info->h = h;
	info->format = format;
	info->usage = usage;
	info->fence_fd = -1;
	info->vaddr = NULL;
	info->ycbcr.y = NULL;
	info->ycbcr.cb = NULL;
	info->ycbcr.cr = NULL;
	info->stride = 0;
}

static native_handle_t *duplicate_buffer_handle(buffer_handle_t handle)
{
	native_handle_t *hnd = native_handle_create(handle->numFds, handle->numInts);

	if (hnd == NULL)
		return NULL;

	const int *old_data = handle->data;
	int *new_data = hnd->data;

	int i;
	for (i = 0; i < handle->numFds; i++) {
		*new_data = dup(*old_data);
		old_data++;
		new_data++;
	}

	memcpy(new_data, old_data, sizeof(int) * handle->numInts);

	return hnd;
}

/****************************************************************
 * Wrappers around gralloc_module_t and alloc_device_t functions.
 * GraphicBufferMapper/GraphicBufferAllocator could replace this
 * in theory.
 ***************************************************************/

static int allocate(struct alloc_device_t *device, struct grallocinfo *info)
{
	int ret;

	ret = device->alloc(device, info->w, info->h, info->format, info->usage, &info->handle,
			    &info->stride);

	CHECK_NO_MSG(ret == 0);
	CHECK_NO_MSG(info->handle->version > 0);
	CHECK_NO_MSG(info->handle->numInts >= 0);
	CHECK_NO_MSG(info->handle->numFds >= 0);
	CHECK_NO_MSG(info->stride >= 0);

	return 1;
}

static int deallocate(struct alloc_device_t *device, struct grallocinfo *info)
{
	int ret;
	ret = device->free(device, info->handle);
	CHECK(ret == 0);
	return 1;
}

static int register_buffer(struct gralloc_module_t *module, struct grallocinfo *info)
{
	int ret;
	ret = module->registerBuffer(module, info->handle);
	return (ret == 0);
}

static int unregister_buffer(struct gralloc_module_t *module, struct grallocinfo *info)
{
	int ret;
	ret = module->unregisterBuffer(module, info->handle);
	return (ret == 0);
}

static int lock(struct gralloc_module_t *module, struct grallocinfo *info)
{
	int ret;

	ret = module->lock(module, info->handle, info->usage, 0, 0, (info->w) / 2, (info->h) / 2,
			   &info->vaddr);

	return (ret == 0);
}

static int unlock(struct gralloc_module_t *module, struct grallocinfo *info)
{
	int ret;
	ret = module->unlock(module, info->handle);
	return (ret == 0);
}

static int lock_ycbcr(struct gralloc_module_t *module, struct grallocinfo *info)
{
	int ret;

	ret = module->lock_ycbcr(module, info->handle, info->usage, 0, 0, (info->w) / 2,
				 (info->h) / 2, &info->ycbcr);

	return (ret == 0);
}

static int lock_async(struct gralloc_module_t *module, struct grallocinfo *info)
{
	int ret;

	ret = module->lockAsync(module, info->handle, info->usage, 0, 0, (info->w) / 2,
				(info->h) / 2, &info->vaddr, info->fence_fd);

	return (ret == 0);
}

static int unlock_async(struct gralloc_module_t *module, struct grallocinfo *info)
{
	int ret;

	ret = module->unlockAsync(module, info->handle, &info->fence_fd);

	return (ret == 0);
}

static int lock_async_ycbcr(struct gralloc_module_t *module, struct grallocinfo *info)
{
	int ret;

	ret = module->lockAsync_ycbcr(module, info->handle, info->usage, 0, 0, (info->w) / 2,
				      (info->h) / 2, &info->ycbcr, info->fence_fd);

	return (ret == 0);
}

/**************************************************************
 * END WRAPPERS                                               *
 **************************************************************/

/* This function tests initialization of gralloc module and allocator. */
static struct gralloctest_context *test_init_gralloc()
{
	int err;
	hw_module_t const *hw_module;
	struct gralloctest_context *ctx = calloc(1, sizeof(*ctx));

	err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_module);
	if (err)
		return NULL;

	gralloc_open(hw_module, &ctx->device);
	ctx->module = (gralloc_module_t *)hw_module;
	if (!ctx->module || !ctx->device)
		return NULL;

	switch (ctx->module->common.module_api_version) {
	case GRALLOC_MODULE_API_VERSION_0_3:
		ctx->api = 3;
		break;
	case GRALLOC_MODULE_API_VERSION_0_2:
		ctx->api = 2;
		break;
	default:
		ctx->api = 1;
	}

	return ctx;
}

static int test_close_gralloc(struct gralloctest_context *ctx)
{
	CHECK(gralloc_close(ctx->device) == 0);
	return 1;
}

/* This function tests allocation with varying buffer dimensions. */
static int test_alloc_varying_sizes(struct gralloctest_context *ctx)
{
	struct grallocinfo info;
	int i;

	grallocinfo_init(&info, 0, 0, HAL_PIXEL_FORMAT_BGRA_8888, GRALLOC_USAGE_SW_READ_OFTEN);

	for (i = 1; i < 1920; i++) {
		info.w = i;
		info.h = i;
		CHECK(allocate(ctx->device, &info));
		CHECK(deallocate(ctx->device, &info));
	}

	info.w = 1;
	for (i = 1; i < 1920; i++) {
		info.h = i;
		CHECK(allocate(ctx->device, &info));
		CHECK(deallocate(ctx->device, &info));
	}

	info.h = 1;
	for (i = 1; i < 1920; i++) {
		info.w = i;
		CHECK(allocate(ctx->device, &info));
		CHECK(deallocate(ctx->device, &info));
	}

	return 1;
}

/*
 * This function tests that we find at least one working format for each
 * combos which we consider important.
 */
static int test_alloc_combinations(struct gralloctest_context *ctx)
{
	int i;

	struct grallocinfo info;
	grallocinfo_init(&info, 512, 512, 0, 0);

	for (i = 0; i < ARRAY_SIZE(combos); i++) {
		info.format = combos[i].format;
		info.usage = combos[i].usage;
		CHECK(allocate(ctx->device, &info));
		CHECK(deallocate(ctx->device, &info));
	}

	return 1;
}

/*
 * This function tests the advertised API version.
 * Version_0_2 added (*lock_ycbcr)() method.
 * Version_0_3 added fence passing to/from lock/unlock.
 */
static int test_api(struct gralloctest_context *ctx)
{

	CHECK(ctx->module->registerBuffer);
	CHECK(ctx->module->unregisterBuffer);
	CHECK(ctx->module->lock);
	CHECK(ctx->module->unlock);

	switch (ctx->module->common.module_api_version) {
	case GRALLOC_MODULE_API_VERSION_0_3:
		CHECK(ctx->module->lock_ycbcr);
		CHECK(ctx->module->lockAsync);
		CHECK(ctx->module->unlockAsync);
		CHECK(ctx->module->lockAsync_ycbcr);
		break;
	case GRALLOC_MODULE_API_VERSION_0_2:
		CHECK(ctx->module->lock_ycbcr);
		CHECK(ctx->module->lockAsync == NULL);
		CHECK(ctx->module->unlockAsync == NULL);
		CHECK(ctx->module->lockAsync_ycbcr == NULL);
		break;
	case GRALLOC_MODULE_API_VERSION_0_1:
		CHECK(ctx->module->lockAsync == NULL);
		CHECK(ctx->module->unlockAsync == NULL);
		CHECK(ctx->module->lockAsync_ycbcr == NULL);
		CHECK(ctx->module->lock_ycbcr == NULL);
		break;
	default:
		return 0;
	}

	return 1;
}

/*
 * This function registers, unregisters, locks and unlocks the buffer in
 * various orders.
 */
static int test_gralloc_order(struct gralloctest_context *ctx)
{
	struct grallocinfo info, duplicate;

	grallocinfo_init(&info, 512, 512, HAL_PIXEL_FORMAT_BGRA_8888, GRALLOC_USAGE_SW_READ_OFTEN);

	grallocinfo_init(&duplicate, 512, 512, HAL_PIXEL_FORMAT_BGRA_8888,
			 GRALLOC_USAGE_SW_READ_OFTEN);

	CHECK(allocate(ctx->device, &info));

	/*
	 * Duplicate the buffer handle to simulate an additional reference
	 * in same process.
	 */
	native_handle_t *native_handle = duplicate_buffer_handle(info.handle);
	duplicate.handle = native_handle;

	CHECK(unregister_buffer(ctx->module, &duplicate) == 0);
	CHECK(register_buffer(ctx->module, &duplicate));

	CHECK(unlock(ctx->module, &duplicate) == 0);

	CHECK(lock(ctx->module, &duplicate));
	CHECK(duplicate.vaddr);
	CHECK(unlock(ctx->module, &duplicate));

	CHECK(unregister_buffer(ctx->module, &duplicate));

	CHECK(register_buffer(ctx->module, &duplicate));
	CHECK(unregister_buffer(ctx->module, &duplicate));
	CHECK(unregister_buffer(ctx->module, &duplicate) == 0);

	CHECK(register_buffer(ctx->module, &duplicate));
	CHECK(deallocate(ctx->device, &info));

	CHECK(lock(ctx->module, &duplicate));
	CHECK(lock(ctx->module, &duplicate));
	CHECK(unlock(ctx->module, &duplicate));
	CHECK(unlock(ctx->module, &duplicate));
	CHECK(unlock(ctx->module, &duplicate) == 0);
	CHECK(unregister_buffer(ctx->module, &duplicate));

	CHECK(native_handle_close(duplicate.handle) == 0);
	CHECK(native_handle_delete(native_handle) == 0);

	return 1;
}

/* This function tests CPU reads and writes. */
static int test_mapping(struct gralloctest_context *ctx)
{
	struct grallocinfo info;
	uint32_t *ptr = NULL;
	uint32_t magic_number = 0x000ABBA;

	grallocinfo_init(&info, 512, 512, HAL_PIXEL_FORMAT_BGRA_8888,
			 GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN);

	CHECK(allocate(ctx->device, &info));
	CHECK(lock(ctx->module, &info));

	ptr = (uint32_t *)info.vaddr;
	CHECK(ptr);
	ptr[(info.w) / 2] = magic_number;

	CHECK(unlock(ctx->module, &info));
	info.vaddr = NULL;
	ptr = NULL;

	CHECK(lock(ctx->module, &info));
	ptr = (uint32_t *)info.vaddr;
	CHECK(ptr);
	CHECK(ptr[info.w / 2] == magic_number);

	CHECK(unlock(ctx->module, &info));
	CHECK(deallocate(ctx->device, &info));

	return 1;
}

/* This function tests the private API we use in ARC++ -- not part of official
 * gralloc. */
static int test_perform(struct gralloctest_context *ctx)
{
	int32_t format;
	uint64_t id1, id2;
	uint32_t stride, width, height;
	struct grallocinfo info, duplicate;
	struct gralloc_module_t *mod = ctx->module;

	grallocinfo_init(&info, 650, 408, HAL_PIXEL_FORMAT_BGRA_8888, GRALLOC_USAGE_SW_READ_OFTEN);

	CHECK(allocate(ctx->device, &info));

	CHECK(mod->perform(mod, GRALLOC_DRM_GET_STRIDE, info.handle, &stride) == 0);
	CHECK(stride == info.stride);

	CHECK(mod->perform(mod, GRALLOC_DRM_GET_FORMAT, info.handle, &format) == 0);
	CHECK(format == info.format);

	CHECK(mod->perform(mod, GRALLOC_DRM_GET_DIMENSIONS, info.handle, &width, &height) == 0);
	CHECK(width == info.w);
	CHECK(height == info.h);

	native_handle_t *native_handle = duplicate_buffer_handle(info.handle);
	duplicate.handle = native_handle;

	CHECK(mod->perform(mod, GRALLOC_DRM_GET_BACKING_STORE, duplicate.handle, &id2));
	CHECK(register_buffer(mod, &duplicate));

	CHECK(mod->perform(mod, GRALLOC_DRM_GET_BACKING_STORE, info.handle, &id1) == 0);
	CHECK(mod->perform(mod, GRALLOC_DRM_GET_BACKING_STORE, duplicate.handle, &id2) == 0);
	CHECK(id1 == id2);

	CHECK(unregister_buffer(mod, &duplicate));
	CHECK(deallocate(ctx->device, &info));

	return 1;
}

/* This function tests that only YUV buffers work with *lock_ycbcr. */
static int test_ycbcr(struct gralloctest_context *ctx)

{
	struct grallocinfo info;
	grallocinfo_init(&info, 512, 512, HAL_PIXEL_FORMAT_YCbCr_420_888,
			 GRALLOC_USAGE_SW_READ_OFTEN);

	CHECK(allocate(ctx->device, &info));

	CHECK(lock(ctx->module, &info) == 0);
	CHECK(lock_ycbcr(ctx->module, &info));
	CHECK(info.ycbcr.y);
	CHECK(info.ycbcr.cb);
	CHECK(info.ycbcr.cr);
	CHECK(unlock(ctx->module, &info));

	CHECK(deallocate(ctx->device, &info));

	info.format = HAL_PIXEL_FORMAT_BGRA_8888;
	CHECK(allocate(ctx->device, &info));

	CHECK(lock_ycbcr(ctx->module, &info) == 0);
	CHECK(lock(ctx->module, &info));
	CHECK(unlock(ctx->module, &info));

	CHECK(deallocate(ctx->device, &info));

	return 1;
}

/*
 * This function tests a method ARC++ uses to query YUV buffer
 * info -- not part of official gralloc API.  This is used in
 * Mali, Mesa, the ArcCodec and  wayland_service.
 */
static int test_yuv_info(struct gralloctest_context *ctx)
{
	struct grallocinfo info;
	uintptr_t y_size, c_stride, c_size, cr_offset, cb_offset;
	uint32_t width, height;
	width = height = 512;

	/* <system/graphics.h> defines YV12 as having:
	 * - an even width
	 * - an even height
	 * - a horizontal stride multiple of 16 pixels
	 * - a vertical stride equal to the height
	 *
	 *   y_size = stride * height.
	 *   c_stride = ALIGN(stride/2, 16).
	 *   c_size = c_stride * height/2.
	 *   size = y_size + c_size * 2.
	 *   cr_offset = y_size.
	 *   cb_offset = y_size + c_size.
	 */

	grallocinfo_init(&info, width, height, HAL_PIXEL_FORMAT_YV12, GRALLOC_USAGE_SW_READ_OFTEN);

	CHECK(allocate(ctx->device, &info));

	y_size = info.stride * height;
	c_stride = ALIGN(info.stride / 2, 16);
	c_size = c_stride * height / 2;
	cr_offset = y_size;
	cb_offset = y_size + c_size;

	info.usage = 0;

	/*
	 * Check if the (*lock_ycbcr) with usage of zero returns the
	 * offsets and strides of the YV12 buffer. This is unofficial
	 * behavior we are testing here.
	 */
	CHECK(lock_ycbcr(ctx->module, &info));

	CHECK(info.stride == info.ycbcr.ystride);
	CHECK(c_stride == info.ycbcr.cstride);
	CHECK(cr_offset == (uintptr_t)info.ycbcr.cr);
	CHECK(cb_offset == (uintptr_t)info.ycbcr.cb);

	CHECK(unlock(ctx->module, &info));

	CHECK(deallocate(ctx->device, &info));

	return 1;
}

/* This function tests asynchronous locking and unlocking of buffers. */
static int test_async(struct gralloctest_context *ctx)

{
	struct grallocinfo rgba_info, ycbcr_info;
	grallocinfo_init(&rgba_info, 512, 512, HAL_PIXEL_FORMAT_BGRA_8888,
			 GRALLOC_USAGE_SW_READ_OFTEN);
	grallocinfo_init(&ycbcr_info, 512, 512, HAL_PIXEL_FORMAT_YCbCr_420_888,
			 GRALLOC_USAGE_SW_READ_OFTEN);

	CHECK(allocate(ctx->device, &rgba_info));
	CHECK(allocate(ctx->device, &ycbcr_info));

	CHECK(lock_async(ctx->module, &rgba_info));
	CHECK(lock_async_ycbcr(ctx->module, &ycbcr_info));

	CHECK(rgba_info.vaddr);
	CHECK(ycbcr_info.ycbcr.y);
	CHECK(ycbcr_info.ycbcr.cb);
	CHECK(ycbcr_info.ycbcr.cr);

	/*
	 * Wait on the fence returned from unlock_async and check it doesn't
	 * return an error.
	 */
	CHECK(unlock_async(ctx->module, &rgba_info));
	CHECK(unlock_async(ctx->module, &ycbcr_info));

	if (rgba_info.fence_fd >= 0) {
		CHECK(sync_wait(rgba_info.fence_fd, 10000) >= 0);
		CHECK(close(rgba_info.fence_fd) == 0);
	}

	if (ycbcr_info.fence_fd >= 0) {
		CHECK(sync_wait(ycbcr_info.fence_fd, 10000) >= 0);
		CHECK(close(ycbcr_info.fence_fd) == 0);
	}

	CHECK(deallocate(ctx->device, &rgba_info));
	CHECK(deallocate(ctx->device, &ycbcr_info));

	return 1;
}

static const struct gralloc_testcase tests[] = {
	{ "alloc_varying_sizes", test_alloc_varying_sizes, 1 },
	{ "alloc_combinations", test_alloc_combinations, 1 },
	{ "api", test_api, 1 },
	{ "gralloc_order", test_gralloc_order, 1 },
	{ "mapping", test_mapping, 1 },
	{ "perform", test_perform, 1 },
	{ "ycbcr", test_ycbcr, 2 },
	{ "yuv_info", test_yuv_info, 2 },
	{ "async", test_async, 3 },
};

static void print_help(const char *argv0)
{
	uint32_t i;
	printf("usage: %s <test_name>\n\n", argv0);
	printf("A valid name test is one the following:\n");
	for (i = 0; i < ARRAY_SIZE(tests); i++)
		printf("%s\n", tests[i].name);
}

int main(int argc, char *argv[])
{
	int ret = 0;
	uint32_t num_run = 0;

	setbuf(stdout, NULL);
	if (argc == 2) {
		uint32_t i;
		char *name = argv[1];

		struct gralloctest_context *ctx = test_init_gralloc();
		if (!ctx) {
			fprintf(stderr, "[  FAILED  ] to initialize gralloc.\n");
			return 1;
		}

		for (i = 0; i < ARRAY_SIZE(tests); i++) {
			if (strcmp(tests[i].name, name) && strcmp("all", name))
				continue;

			int success = 1;
			if (ctx->api >= tests[i].required_api)
				success = tests[i].run_test(ctx);

			printf("[ RUN      ] gralloctest.%s\n", tests[i].name);
			if (!success) {
				fprintf(stderr, "[  FAILED  ] gralloctest.%s\n", tests[i].name);
				ret |= 1;
			} else {
				printf("[  PASSED  ] gralloctest.%s\n", tests[i].name);
			}

			num_run++;
		}

		if (!test_close_gralloc(ctx)) {
			fprintf(stderr, "[  FAILED  ] to close gralloc.\n");
			return 1;
		}

		if (!num_run)
			goto print_usage;

		return ret;
	}

print_usage:
	print_help(argv[0]);
	return 0;
}
