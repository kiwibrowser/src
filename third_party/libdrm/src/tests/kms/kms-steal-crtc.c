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
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <drm_fourcc.h>

#include "util/pattern.h"
#include "libkms-test.h"

static void signal_handler(int signum)
{
}

int main(int argc, char *argv[])
{
	struct kms_framebuffer *fb;
	struct kms_screen *screen;
	struct kms_device *device;
	unsigned int index = 0;
	struct sigaction sa;
	int fd, err;
	void *ptr;

	if (argc < 2) {
		fprintf(stderr, "usage: %s DEVICE\n", argv[0]);
		return 1;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;

	err = sigaction(SIGINT, &sa, NULL);
	if (err < 0) {
		fprintf(stderr, "sigaction() failed: %m\n");
		return 1;
	}

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open() failed: %m\n");
		return 1;
	}

	device = kms_device_open(fd);
	if (!device) {
		fprintf(stderr, "kms_device_open() failed: %m\n");
		return 1;
	}

	if (device->num_screens < 1) {
		fprintf(stderr, "no screens found\n");
		kms_device_close(device);
		close(fd);
		return 1;
	}

	/* TODO: allow command-line to override */
	screen = device->screens[0];

	printf("Using screen %s, resolution %ux%u\n", screen->name,
	       screen->width, screen->height);

	fb = kms_framebuffer_create(device, screen->width, screen->height,
				    DRM_FORMAT_XRGB8888);
	if (!fb) {
		fprintf(stderr, "kms_framebuffer_create() failed\n");
		return 1;
	}

	err = kms_framebuffer_map(fb, &ptr);
	if (err < 0) {
		fprintf(stderr, "kms_framebuffer_map() failed: %d\n", err);
		return 1;
	}

	util_fill_pattern(fb->format, UTIL_PATTERN_SMPTE, &ptr, fb->width,
			  fb->height, fb->pitch);

	kms_framebuffer_unmap(fb);

	err = kms_screen_set(screen, device->crtcs[index++], fb);
	if (err < 0) {
		fprintf(stderr, "kms_screen_set() failed: %d\n", err);
		return 1;
	}

	while (true) {
		int nfds = STDIN_FILENO + 1;
		struct timeval timeout;
		fd_set fds;

		memset(&timeout, 0, sizeof(timeout));
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);

		err = select(nfds, &fds, NULL, NULL, &timeout);
		if (err < 0) {
			if (errno == EINTR)
				break;

			fprintf(stderr, "select() failed: %d\n", errno);
			break;
		}

		if (err > 0) {
			if (FD_ISSET(STDIN_FILENO, &fds))
				break;
		}

		/* switch CRTC */
		if (index >= device->num_crtcs)
			index = 0;

		err = kms_screen_set(screen, device->crtcs[index], fb);
		if (err < 0) {
			fprintf(stderr, "kms_screen_set() failed: %d\n", err);
			break;
		}

		index++;
	}

	kms_framebuffer_free(fb);
	kms_device_close(device);
	close(fd);

	return 0;
}
