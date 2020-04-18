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


#ifndef INTERNAL_H_
#define INTERNAL_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libdrm_macros.h"
#include "libkms.h"

struct kms_driver
{
	int (*get_prop)(struct kms_driver *kms, const unsigned key,
			unsigned *out);
	int (*destroy)(struct kms_driver *kms);

	int (*bo_create)(struct kms_driver *kms,
			 unsigned width,
			 unsigned height,
			 enum kms_bo_type type,
			 const unsigned *attr,
			 struct kms_bo **out);
	int (*bo_get_prop)(struct kms_bo *bo, const unsigned key,
			   unsigned *out);
	int (*bo_map)(struct kms_bo *bo, void **out);
	int (*bo_unmap)(struct kms_bo *bo);
	int (*bo_destroy)(struct kms_bo *bo);

	int fd;
};

struct kms_bo
{
	struct kms_driver *kms;
	void *ptr;
	size_t size;
	size_t offset;
	size_t pitch;
	unsigned handle;
};

drm_private int linux_create(int fd, struct kms_driver **out);

drm_private int vmwgfx_create(int fd, struct kms_driver **out);

drm_private int intel_create(int fd, struct kms_driver **out);

drm_private int dumb_create(int fd, struct kms_driver **out);

drm_private int nouveau_create(int fd, struct kms_driver **out);

drm_private int radeon_create(int fd, struct kms_driver **out);

drm_private int exynos_create(int fd, struct kms_driver **out);

#endif
