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
#include <stdlib.h>
#include <string.h>
#include "internal.h"

#include "xf86drm.h"
#include "libdrm_macros.h"
#include "vmwgfx_drm.h"

struct vmwgfx_bo
{
	struct kms_bo base;
	uint64_t map_handle;
	unsigned map_count;
};

static int
vmwgfx_get_prop(struct kms_driver *kms, unsigned key, unsigned *out)
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
vmwgfx_destroy(struct kms_driver *kms)
{
	free(kms);
	return 0;
}

static int
vmwgfx_bo_create(struct kms_driver *kms,
		 const unsigned width, const unsigned height,
		 const enum kms_bo_type type, const unsigned *attr,
		 struct kms_bo **out)
{
	struct vmwgfx_bo *bo;
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
		return -EINVAL;

	{
		union drm_vmw_alloc_dmabuf_arg arg;
		struct drm_vmw_alloc_dmabuf_req *req = &arg.req;
		struct drm_vmw_dmabuf_rep *rep = &arg.rep;

		memset(&arg, 0, sizeof(arg));
		req->size = width * height * 4;
		bo->base.size = req->size;
		bo->base.pitch = width * 4;
		bo->base.kms = kms;

		do {
			ret = drmCommandWriteRead(bo->base.kms->fd,
						  DRM_VMW_ALLOC_DMABUF,
						  &arg, sizeof(arg));
		} while (ret == -ERESTART);

		if (ret)
			goto err_free;

		bo->base.handle = rep->handle;
		bo->map_handle = rep->map_handle;
		bo->base.handle = rep->cur_gmr_id;
		bo->base.offset = rep->cur_gmr_offset;
	}

	*out = &bo->base;

	return 0;

err_free:
	free(bo);
	return ret;
}

static int
vmwgfx_bo_get_prop(struct kms_bo *bo, unsigned key, unsigned *out)
{
	switch (key) {
	default:
		return -EINVAL;
	}
}

static int
vmwgfx_bo_map(struct kms_bo *_bo, void **out)
{
	struct vmwgfx_bo *bo = (struct vmwgfx_bo *)_bo;
	void *map;

	if (bo->base.ptr) {
		bo->map_count++;
		*out = bo->base.ptr;
		return 0;
	}

	map = drm_mmap(NULL, bo->base.size, PROT_READ | PROT_WRITE, MAP_SHARED, bo->base.kms->fd, bo->map_handle);
	if (map == MAP_FAILED)
		return -errno;

	bo->base.ptr = map;
	bo->map_count++;
	*out = bo->base.ptr;

	return 0;
}

static int
vmwgfx_bo_unmap(struct kms_bo *_bo)
{
	struct vmwgfx_bo *bo = (struct vmwgfx_bo *)_bo;
	bo->map_count--;
	return 0;
}

static int
vmwgfx_bo_destroy(struct kms_bo *_bo)
{
	struct vmwgfx_bo *bo = (struct vmwgfx_bo *)_bo;
	struct drm_vmw_unref_dmabuf_arg arg;

	if (bo->base.ptr) {
		/* XXX Sanity check map_count */
		drm_munmap(bo->base.ptr, bo->base.size);
		bo->base.ptr = NULL;
	}

	memset(&arg, 0, sizeof(arg));
	arg.handle = bo->base.handle;
	drmCommandWrite(bo->base.kms->fd, DRM_VMW_UNREF_DMABUF, &arg, sizeof(arg));

	free(bo);
	return 0;
}

drm_private int
vmwgfx_create(int fd, struct kms_driver **out)
{
	struct kms_driver *kms;

	kms = calloc(1, sizeof(*kms));
	if (!kms)
		return -ENOMEM;

	kms->fd = fd;

	kms->bo_create = vmwgfx_bo_create;
	kms->bo_map = vmwgfx_bo_map;
	kms->bo_unmap = vmwgfx_bo_unmap;
	kms->bo_get_prop = vmwgfx_bo_get_prop;
	kms->bo_destroy = vmwgfx_bo_destroy;
	kms->get_prop = vmwgfx_get_prop;
	kms->destroy = vmwgfx_destroy;
	*out = kms;
	return 0;
}
