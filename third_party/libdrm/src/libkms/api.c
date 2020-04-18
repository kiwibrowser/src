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

#include "libdrm_macros.h"
#include "internal.h"

int kms_create(int fd, struct kms_driver **out)
{
	return linux_create(fd, out);
}

int kms_get_prop(struct kms_driver *kms, unsigned key, unsigned *out)
{
	switch (key) {
	case KMS_BO_TYPE:
		break;
	default:
		return -EINVAL;
	}
	return kms->get_prop(kms, key, out);
}

int kms_destroy(struct kms_driver **kms)
{
	if (!(*kms))
		return 0;

	free(*kms);
	*kms = NULL;
	return 0;
}

int kms_bo_create(struct kms_driver *kms, const unsigned *attr, struct kms_bo **out)
{
	unsigned width = 0;
	unsigned height = 0;
	enum kms_bo_type type = KMS_BO_TYPE_SCANOUT_X8R8G8B8;
	int i;

	for (i = 0; attr[i];) {
		unsigned key = attr[i++];
		unsigned value = attr[i++];

		switch (key) {
		case KMS_WIDTH:
			width = value;
			break;
		case KMS_HEIGHT:
			height = value;
			break;
		case KMS_BO_TYPE:
			type = value;
			break;
		default:
			return -EINVAL;
		}
	}

	if (width == 0 || height == 0)
		return -EINVAL;

	/* XXX sanity check type */

	if (type == KMS_BO_TYPE_CURSOR_64X64_A8R8G8B8 &&
	    (width != 64 || height != 64))
		return -EINVAL;

	return kms->bo_create(kms, width, height, type, attr, out);
}

int kms_bo_get_prop(struct kms_bo *bo, unsigned key, unsigned *out)
{
	switch (key) {
	case KMS_PITCH:
		*out = bo->pitch;
		break;
	case KMS_HANDLE:
		*out = bo->handle;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int kms_bo_map(struct kms_bo *bo, void **out)
{
	return bo->kms->bo_map(bo, out);
}

int kms_bo_unmap(struct kms_bo *bo)
{
	return bo->kms->bo_unmap(bo);
}

int kms_bo_destroy(struct kms_bo **bo)
{
	int ret;

	if (!(*bo))
		return 0;

	ret = (*bo)->kms->bo_destroy(*bo);
	if (ret)
		return ret;

	*bo = NULL;
	return 0;
}
