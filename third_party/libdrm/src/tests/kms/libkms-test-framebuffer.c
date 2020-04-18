/*
 * Copyright © 2014 NVIDIA Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <string.h>

#include <sys/mman.h>

#include <drm_fourcc.h>

#include "xf86drm.h"

#include "libkms-test.h"

struct kms_framebuffer *kms_framebuffer_create(struct kms_device *device,
					       unsigned int width,
					       unsigned int height,
					       uint32_t format)
{
	uint32_t handles[4], pitches[4], offsets[4];
	struct drm_mode_create_dumb args;
	struct kms_framebuffer *fb;
	int err;

	fb = calloc(1, sizeof(*fb));
	if (!fb)
		return NULL;

	fb->device = device;
	fb->width = width;
	fb->height = height;
	fb->format = format;

	memset(&args, 0, sizeof(args));
	args.width = width;
	args.height = height;

	switch (format) {
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_RGBA8888:
		args.bpp = 32;
		break;

	default:
		free(fb);
		return NULL;
	}

	err = drmIoctl(device->fd, DRM_IOCTL_MODE_CREATE_DUMB, &args);
	if (err < 0) {
		free(fb);
		return NULL;
	}

	fb->handle = args.handle;
	fb->pitch = args.pitch;
	fb->size = args.size;

	handles[0] = fb->handle;
	pitches[0] = fb->pitch;
	offsets[0] = 0;

	err = drmModeAddFB2(device->fd, width, height, format, handles,
			    pitches, offsets, &fb->id, 0);
	if (err < 0) {
		kms_framebuffer_free(fb);
		return NULL;
	}

	return fb;
}

void kms_framebuffer_free(struct kms_framebuffer *fb)
{
	struct kms_device *device = fb->device;
	struct drm_mode_destroy_dumb args;
	int err;

	if (fb->id) {
		err = drmModeRmFB(device->fd, fb->id);
		if (err < 0) {
			/* not much we can do now */
		}
	}

	memset(&args, 0, sizeof(args));
	args.handle = fb->handle;

	err = drmIoctl(device->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &args);
	if (err < 0) {
		/* not much we can do now */
	}

	free(fb);
}

int kms_framebuffer_map(struct kms_framebuffer *fb, void **ptrp)
{
	struct kms_device *device = fb->device;
	struct drm_mode_map_dumb args;
	void *ptr;
	int err;

	if (fb->ptr) {
		*ptrp = fb->ptr;
		return 0;
	}

	memset(&args, 0, sizeof(args));
	args.handle = fb->handle;

	err = drmIoctl(device->fd, DRM_IOCTL_MODE_MAP_DUMB, &args);
	if (err < 0)
		return -errno;

	ptr = mmap(0, fb->size, PROT_READ | PROT_WRITE, MAP_SHARED,
		   device->fd, args.offset);
	if (ptr == MAP_FAILED)
		return -errno;

	*ptrp = fb->ptr = ptr;

	return 0;
}

void kms_framebuffer_unmap(struct kms_framebuffer *fb)
{
	if (fb->ptr) {
		munmap(fb->ptr, fb->size);
		fb->ptr = NULL;
	}
}
