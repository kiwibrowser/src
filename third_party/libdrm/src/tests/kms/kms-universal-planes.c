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

#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <drm_fourcc.h>
#include "xf86drm.h"

#include "util/common.h"
#include "libkms-test.h"

static const uint32_t formats[] = {
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_RGBA8888,
};

static uint32_t choose_format(struct kms_plane *plane)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(formats); i++)
		if (kms_plane_supports_format(plane, formats[i]))
			return formats[i];

	return 0;
}

static void prepare_framebuffer(struct kms_framebuffer *fb, bool invert)
{
	const unsigned int block_size = 16;
	uint32_t colors[2];
	unsigned int i, j;
	uint32_t *buf;
	void *ptr;
	int err;

	switch (fb->format) {
	case DRM_FORMAT_XRGB8888:
		printf("using XRGB8888 format\n");
		           /* XXRRGGBB */
		colors[0] = 0xffff0000;
		colors[1] = 0xff0000ff;
		break;

	case DRM_FORMAT_XBGR8888:
		printf("using XBGR8888 format\n");
		           /* XXBBGGRR */
		colors[0] = 0xff0000ff;
		colors[1] = 0xffff0000;
		break;

	case DRM_FORMAT_RGBA8888:
		printf("using RGBA8888 format\n");
		           /* RRGGBBAA */
		colors[0] = 0xff0000ff;
		colors[1] = 0x0000ffff;
		break;

	default:
		colors[0] = 0xffffffff;
		colors[1] = 0xffffffff;
		break;
	}

	err = kms_framebuffer_map(fb, &ptr);
	if (err < 0) {
		fprintf(stderr, "kms_framebuffer_map() failed: %s\n",
			strerror(-err));
		return;
	}

	buf = ptr;

	for (j = 0; j < fb->height; j++) {
		for (i = 0; i < fb->width; i++) {
			unsigned int color = (j / block_size) ^
					     (i / block_size);

			if (invert)
				color ^= color;

			*buf++ = colors[color & 1];
		}
	}

	kms_framebuffer_unmap(fb);
}

int main(int argc, char *argv[])
{
	static const char opts[] = "chopv";
	static struct option options[] = {
		{ "cursor", 0, 0, 'c' },
		{ "help", 0, 0, 'h' },
		{ "overlay", 0, 0, 'o' },
		{ "primary", 0, 0, 'p' },
		{ "verbose", 0, 0, 'v' },
		{ 0, 0, 0, 0 },
	};
	struct kms_framebuffer *cursor = NULL;
	struct kms_framebuffer *root = NULL;
	struct kms_framebuffer *fb = NULL;
	struct kms_device *device;
	bool use_overlay = false;
	bool use_primary = false;
	struct kms_plane *plane;
	bool use_cursor = false;
	bool verbose = false;
	unsigned int i;
	int opt, idx;
	int fd, err;

	while ((opt = getopt_long(argc, argv, opts, options, &idx)) != -1) {
		switch (opt) {
		case 'c':
			use_cursor = true;
			break;

		case 'h':
			break;

		case 'o':
			use_overlay = true;
			break;

		case 'p':
			use_primary = true;
			break;

		case 'v':
			verbose = true;
			break;

		default:
			printf("unknown option \"%c\"\n", opt);
			return 1;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "usage: %s [options] DEVICE\n", argv[0]);
		return 1;
	}

	fd = open(argv[optind], O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open() failed: %m\n");
		return 1;
	}

	err = drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	if (err < 0) {
		fprintf(stderr, "drmSetClientCap() failed: %d\n", err);
		return 1;
	}

	device = kms_device_open(fd);
	if (!device)
		return 1;

	if (verbose) {
		printf("Screens: %u\n", device->num_screens);

		for (i = 0; i < device->num_screens; i++) {
			struct kms_screen *screen = device->screens[i];
			const char *status = "disconnected";

			if (screen->connected)
				status = "connected";

			printf("  %u: %x\n", i, screen->id);
			printf("    Status: %s\n", status);
			printf("    Name: %s\n", screen->name);
			printf("    Resolution: %ux%u\n", screen->width,
			       screen->height);
		}

		printf("Planes: %u\n", device->num_planes);

		for (i = 0; i < device->num_planes; i++) {
			const char *type = NULL;

			plane = device->planes[i];
			switch (plane->type) {
			case DRM_PLANE_TYPE_OVERLAY:
				type = "overlay";
				break;

			case DRM_PLANE_TYPE_PRIMARY:
				type = "primary";
				break;

			case DRM_PLANE_TYPE_CURSOR:
				type = "cursor";
				break;
			}

			printf("  %u: %p\n", i, plane);
			printf("    ID: %x\n", plane->id);
			printf("    CRTC: %x\n", plane->crtc->id);
			printf("    Type: %x (%s)\n", plane->type, type);
		}
	}

	if (use_cursor) {
		unsigned int x, y;
		uint32_t format;

		plane = kms_device_find_plane_by_type(device,
						      DRM_PLANE_TYPE_CURSOR,
						      0);
		if (!plane) {
			fprintf(stderr, "no cursor plane found\n");
			return 1;
		}

		format = choose_format(plane);
		if (!format) {
			fprintf(stderr, "no matching format found\n");
			return 1;
		}

		cursor = kms_framebuffer_create(device, 32, 32, format);
		if (!cursor) {
			fprintf(stderr, "failed to create cursor buffer\n");
			return 1;
		}

		prepare_framebuffer(cursor, false);

		x = (device->screens[0]->width - cursor->width) / 2;
		y = (device->screens[0]->height - cursor->height) / 2;

		kms_plane_set(plane, cursor, x, y);
	}

	if (use_overlay) {
		uint32_t format;

		plane = kms_device_find_plane_by_type(device,
						      DRM_PLANE_TYPE_OVERLAY,
						      0);
		if (!plane) {
			fprintf(stderr, "no overlay plane found\n");
			return 1;
		}

		format = choose_format(plane);
		if (!format) {
			fprintf(stderr, "no matching format found\n");
			return 1;
		}

		fb = kms_framebuffer_create(device, 320, 240, format);
		if (!fb)
			return 1;

		prepare_framebuffer(fb, false);

		kms_plane_set(plane, fb, 0, 0);
	}

	if (use_primary) {
		unsigned int x, y;
		uint32_t format;

		plane = kms_device_find_plane_by_type(device,
						      DRM_PLANE_TYPE_PRIMARY,
						      0);
		if (!plane) {
			fprintf(stderr, "no primary plane found\n");
			return 1;
		}

		format = choose_format(plane);
		if (!format) {
			fprintf(stderr, "no matching format found\n");
			return 1;
		}

		root = kms_framebuffer_create(device, 640, 480, format);
		if (!root)
			return 1;

		prepare_framebuffer(root, true);

		x = (device->screens[0]->width - root->width) / 2;
		y = (device->screens[0]->height - root->height) / 2;

		kms_plane_set(plane, root, x, y);
	}

	while (1) {
		struct timeval timeout = { 1, 0 };
		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);

		err = select(STDIN_FILENO + 1, &fds, NULL, NULL, &timeout);
		if (err < 0) {
			fprintf(stderr, "select() failed: %m\n");
			break;
		}

		/* timeout */
		if (err == 0)
			continue;

		if (FD_ISSET(STDIN_FILENO, &fds))
			break;
	}

	if (cursor)
		kms_framebuffer_free(cursor);

	if (root)
		kms_framebuffer_free(root);

	if (fb)
		kms_framebuffer_free(fb);

	kms_device_close(device);
	close(fd);

	return 0;
}
