/**************************************************************************
 *
 * Copyright © 2009 VMware, Inc., Palo Alto, CA., USA
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "internal.h"

#include <sys/ioctl.h>
#include "xf86drm.h"
#include "libdrm_macros.h"

#include "i915_drm.h"

struct intel_bo
{
	struct kms_bo base;
	unsigned map_count;
};

static int
intel_get_prop(struct kms_driver *kms, unsigned key, unsigned *out)
{
	switch (key) {
	case KMS_BO_TYPE:
		*out = KMS_BO_TYPE_SCANOUT_X8R8G8B8 | KMS_BO_TYPE_CURSOR_64X64_A8R8G8B8;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int
intel_destroy(struct kms_driver *kms)
{
	free(kms);
	return 0;
}

static int
intel_bo_create(struct kms_driver *kms,
		 const unsigned width, const unsigned height,
		 const enum kms_bo_type type, const unsigned *attr,
		 struct kms_bo **out)
{
	struct drm_i915_gem_create arg;
	unsigned size, pitch;
	struct intel_bo *bo;
	int i, ret;

	for (i = 0; attr[i]; i += 2) {
		switch (attr[i]) {
		case KMS_WIDTH:
		case KMS_HEIGHT:
		case KMS_BO_TYPE:
			break;
		default:
			return -EINVAL;
		}
	}

	bo = calloc(1, sizeof(*bo));
	if (!bo)
		return -ENOMEM;

	if (type == KMS_BO_TYPE_CURSOR_64X64_A8R8G8B8) {
		pitch = 64 * 4;
		size = 64 * 64 * 4;
	} else if (type == KMS_BO_TYPE_SCANOUT_X8R8G8B8) {
		pitch = width * 4;
		pitch = (pitch + 512 - 1) & ~(512 - 1);
		size = pitch * ((height + 4 - 1) & ~(4 - 1));
	} else {
		free(bo);
		return -EINVAL;
	}

	memset(&arg, 0, sizeof(arg));
	arg.size = size;

	ret = drmCommandWriteRead(kms->fd, DRM_I915_GEM_CREATE, &arg, sizeof(arg));
	if (ret)
		goto err_free;

	bo->base.kms = kms;
	bo->base.handle = arg.handle;
	bo->base.size = size;
	bo->base.pitch = pitch;

	*out = &bo->base;
	if (type == KMS_BO_TYPE_SCANOUT_X8R8G8B8 && pitch > 512) {
		struct drm_i915_gem_set_tiling tile;

		memset(&tile, 0, sizeof(tile));
		tile.handle = bo->base.handle;
		tile.tiling_mode = I915_TILING_X;
		tile.stride = bo->base.pitch;

		ret = drmCommandWriteRead(kms->fd, DRM_I915_GEM_SET_TILING, &tile, sizeof(tile));
#if 0
		if (ret) {
			kms_bo_destroy(out);
			return ret;
		}
#endif
	}

	return 0;

err_free:
	free(bo);
	return ret;
}

static int
intel_bo_get_prop(struct kms_bo *bo, unsigned key, unsigned *out)
{
	switch (key) {
	default:
		return -EINVAL;
	}
}

static int
intel_bo_map(struct kms_bo *_bo, void **out)
{
	struct intel_bo *bo = (struct intel_bo *)_bo;
	struct drm_i915_gem_mmap_gtt arg;
	void *map = NULL;
	int ret;

	if (bo->base.ptr) {
		bo->map_count++;
		*out = bo->base.ptr;
		return 0;
	}

	memset(&arg, 0, sizeof(arg));
	arg.handle = bo->base.handle;

	ret = drmCommandWriteRead(bo->base.kms->fd, DRM_I915_GEM_MMAP_GTT, &arg, sizeof(arg));
	if (ret)
		return ret;

	map = drm_mmap(0, bo->base.size, PROT_READ | PROT_WRITE, MAP_SHARED, bo->base.kms->fd, arg.offset);
	if (map == MAP_FAILED)
		return -errno;

	bo->base.ptr = map;
	bo->map_count++;
	*out = bo->base.ptr;

	return 0;
}

static int
intel_bo_unmap(struct kms_bo *_bo)
{
	struct intel_bo *bo = (struct intel_bo *)_bo;
	bo->map_count--;
	return 0;
}

static int
intel_bo_destroy(struct kms_bo *_bo)
{
	struct intel_bo *bo = (struct intel_bo *)_bo;
	struct drm_gem_close arg;
	int ret;

	if (bo->base.ptr) {
		/* XXX Sanity check map_count */
		drm_munmap(bo->base.ptr, bo->base.size);
		bo->base.ptr = NULL;
	}

	memset(&arg, 0, sizeof(arg));
	arg.handle = bo->base.handle;

	ret = drmIoctl(bo->base.kms->fd, DRM_IOCTL_GEM_CLOSE, &arg);
	if (ret)
		return -errno;

	free(bo);
	return 0;
}

drm_private int
intel_create(int fd, struct kms_driver **out)
{
	struct kms_driver *kms;

	kms = calloc(1, sizeof(*kms));
	if (!kms)
		return -ENOMEM;

	kms->fd = fd;

	kms->bo_create = intel_bo_create;
	kms->bo_map = intel_bo_map;
	kms->bo_unmap = intel_bo_unmap;
	kms->bo_get_prop = intel_bo_get_prop;
	kms->bo_destroy = intel_bo_destroy;
	kms->get_prop = intel_get_prop;
	kms->destroy = intel_destroy;
	*out = kms;

	return 0;
}
