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

static void kms_screen_probe(struct kms_screen *screen)
{
	struct kms_device *device = screen->device;
	drmModeConnector *con;

	con = drmModeGetConnector(device->fd, screen->id);
	if (!con)
		return;

	screen->type = con->connector_type;

	if (con->connection == DRM_MODE_CONNECTED)
		screen->connected = true;
	else
		screen->connected = false;

	memcpy(&screen->mode, &con->modes[0], sizeof(drmModeModeInfo));
	screen->width = screen->mode.hdisplay;
	screen->height = screen->mode.vdisplay;

	drmModeFreeConnector(con);
}

struct kms_screen *kms_screen_create(struct kms_device *device, uint32_t id)
{
	struct kms_screen *screen;

	screen = calloc(1, sizeof(*screen));
	if (!screen)
		return NULL;

	screen->device = device;
	screen->id = id;

	kms_screen_probe(screen);

	return screen;
}

void kms_screen_free(struct kms_screen *screen)
{
	if (screen)
		free(screen->name);

	free(screen);
}

int kms_screen_set(struct kms_screen *screen, struct kms_crtc *crtc,
		   struct kms_framebuffer *fb)
{
	struct kms_device *device = screen->device;
	int err;

	err = drmModeSetCrtc(device->fd, crtc->id, fb->id, 0, 0, &screen->id,
			     1, &screen->mode);
	if (err < 0)
		return -errno;

	return 0;
}
