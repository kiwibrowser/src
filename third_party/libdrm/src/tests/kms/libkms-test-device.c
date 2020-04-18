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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "util/common.h"
#include "libkms-test.h"

static const char *const connector_names[] = {
	"Unknown",
	"VGA",
	"DVI-I",
	"DVI-D",
	"DVI-A",
	"Composite",
	"SVIDEO",
	"LVDS",
	"Component",
	"9PinDIN",
	"DisplayPort",
	"HDMI-A",
	"HDMI-B",
	"TV",
	"eDP",
	"Virtual",
	"DSI",
};

static void kms_device_probe_screens(struct kms_device *device)
{
	unsigned int counts[ARRAY_SIZE(connector_names)];
	struct kms_screen *screen;
	drmModeRes *res;
	int i;

	memset(counts, 0, sizeof(counts));

	res = drmModeGetResources(device->fd);
	if (!res)
		return;

	device->screens = calloc(res->count_connectors, sizeof(screen));
	if (!device->screens)
		return;

	for (i = 0; i < res->count_connectors; i++) {
		unsigned int *count;
		const char *type;
		int len;

		screen = kms_screen_create(device, res->connectors[i]);
		if (!screen)
			continue;

		/* assign a unique name to this screen */
		type = connector_names[screen->type];
		count = &counts[screen->type];

		len = snprintf(NULL, 0, "%s-%u", type, *count);

		screen->name = malloc(len + 1);
		if (!screen->name) {
			free(screen);
			continue;
		}

		snprintf(screen->name, len + 1, "%s-%u", type, *count);
		(*count)++;

		device->screens[i] = screen;
		device->num_screens++;
	}

	drmModeFreeResources(res);
}

static void kms_device_probe_crtcs(struct kms_device *device)
{
	struct kms_crtc *crtc;
	drmModeRes *res;
	int i;

	res = drmModeGetResources(device->fd);
	if (!res)
		return;

	device->crtcs = calloc(res->count_crtcs, sizeof(crtc));
	if (!device->crtcs)
		return;

	for (i = 0; i < res->count_crtcs; i++) {
		crtc = kms_crtc_create(device, res->crtcs[i]);
		if (!crtc)
			continue;

		device->crtcs[i] = crtc;
		device->num_crtcs++;
	}

	drmModeFreeResources(res);
}

static void kms_device_probe_planes(struct kms_device *device)
{
	struct kms_plane *plane;
	drmModePlaneRes *res;
	unsigned int i;

	res = drmModeGetPlaneResources(device->fd);
	if (!res)
		return;

	device->planes = calloc(res->count_planes, sizeof(plane));
	if (!device->planes)
		return;

	for (i = 0; i < res->count_planes; i++) {
		plane = kms_plane_create(device, res->planes[i]);
		if (!plane)
			continue;

		device->planes[i] = plane;
		device->num_planes++;
	}

	drmModeFreePlaneResources(res);
}

static void kms_device_probe(struct kms_device *device)
{
	kms_device_probe_screens(device);
	kms_device_probe_crtcs(device);
	kms_device_probe_planes(device);
}

struct kms_device *kms_device_open(int fd)
{
	struct kms_device *device;

	device = calloc(1, sizeof(*device));
	if (!device)
		return NULL;

	device->fd = fd;

	kms_device_probe(device);

	return device;
}

void kms_device_close(struct kms_device *device)
{
	unsigned int i;

	for (i = 0; i < device->num_planes; i++)
		kms_plane_free(device->planes[i]);

	free(device->planes);

	for (i = 0; i < device->num_crtcs; i++)
		kms_crtc_free(device->crtcs[i]);

	free(device->crtcs);

	for (i = 0; i < device->num_screens; i++)
		kms_screen_free(device->screens[i]);

	free(device->screens);

	if (device->fd >= 0)
		close(device->fd);

	free(device);
}

struct kms_plane *kms_device_find_plane_by_type(struct kms_device *device,
						uint32_t type,
						unsigned int index)
{
	unsigned int i;

	for (i = 0; i < device->num_planes; i++) {
		if (device->planes[i]->type == type) {
			if (index == 0)
				return device->planes[i];

			index--;
		}
	}

	return NULL;
}
