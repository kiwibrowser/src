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

#include "libkms-test.h"

static int kms_plane_probe(struct kms_plane *plane)
{
	struct kms_device *device = plane->device;
	drmModeObjectPropertiesPtr props;
	drmModePlane *p;
	unsigned int i;

	p = drmModeGetPlane(device->fd, plane->id);
	if (!p)
		return -ENODEV;

	/* TODO: allow dynamic assignment to CRTCs */
	if (p->crtc_id == 0) {
		for (i = 0; i < device->num_crtcs; i++) {
			if (p->possible_crtcs & (1 << i)) {
				p->crtc_id = device->crtcs[i]->id;
				break;
			}
		}
	}

	for (i = 0; i < device->num_crtcs; i++) {
		if (device->crtcs[i]->id == p->crtc_id) {
			plane->crtc = device->crtcs[i];
			break;
		}
	}

	plane->formats = calloc(p->count_formats, sizeof(uint32_t));
	if (!plane->formats)
		return -ENOMEM;

	for (i = 0; i < p->count_formats; i++)
		plane->formats[i] = p->formats[i];

	plane->num_formats = p->count_formats;

	drmModeFreePlane(p);

	props = drmModeObjectGetProperties(device->fd, plane->id,
					   DRM_MODE_OBJECT_PLANE);
	if (!props)
		return -ENODEV;

	for (i = 0; i < props->count_props; i++) {
		drmModePropertyPtr prop;

		prop = drmModeGetProperty(device->fd, props->props[i]);
		if (prop) {
			if (strcmp(prop->name, "type") == 0)
				plane->type = props->prop_values[i];

			drmModeFreeProperty(prop);
		}
	}

	drmModeFreeObjectProperties(props);

	return 0;
}

struct kms_plane *kms_plane_create(struct kms_device *device, uint32_t id)
{
	struct kms_plane *plane;

	plane = calloc(1, sizeof(*plane));
	if (!plane)
		return NULL;

	plane->device = device;
	plane->id = id;

	kms_plane_probe(plane);

	return plane;
}

void kms_plane_free(struct kms_plane *plane)
{
	free(plane);
}

int kms_plane_set(struct kms_plane *plane, struct kms_framebuffer *fb,
		  unsigned int x, unsigned int y)
{
	struct kms_device *device = plane->device;
	int err;

	err = drmModeSetPlane(device->fd, plane->id, plane->crtc->id, fb->id,
			      0, x, y, fb->width, fb->height, 0 << 16,
			      0 << 16, fb->width << 16, fb->height << 16);
	if (err < 0)
		return -errno;

	return 0;
}

bool kms_plane_supports_format(struct kms_plane *plane, uint32_t format)
{
	unsigned int i;

	for (i = 0; i < plane->num_formats; i++)
		if (plane->formats[i] == format)
			return true;

	return false;
}
