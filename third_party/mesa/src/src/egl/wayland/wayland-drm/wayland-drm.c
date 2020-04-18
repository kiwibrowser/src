/*
 * Copyright © 2011 Kristian Høgsberg
 * Copyright © 2011 Benjamin Franzke
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Kristian Høgsberg <krh@bitplanet.net>
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <wayland-server.h>
#include "wayland-drm.h"
#include "wayland-drm-server-protocol.h"

struct wl_drm {
	struct wl_display *display;

	void *user_data;
	char *device_name;

	struct wayland_drm_callbacks *callbacks;
};

static void
destroy_buffer(struct wl_resource *resource)
{
	struct wl_drm_buffer *buffer = resource->data;
	struct wl_drm *drm = buffer->drm;

	drm->callbacks->release_buffer(drm->user_data, buffer);
	free(buffer);
}

static void
buffer_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

const static struct wl_buffer_interface drm_buffer_interface = {
	buffer_destroy
};

static void
create_buffer(struct wl_client *client, struct wl_resource *resource,
              uint32_t id, uint32_t name, int32_t width, int32_t height,
              uint32_t format,
              int32_t offset0, int32_t stride0,
              int32_t offset1, int32_t stride1,
              int32_t offset2, int32_t stride2)
{
	struct wl_drm *drm = resource->data;
	struct wl_drm_buffer *buffer;

	buffer = calloc(1, sizeof *buffer);
	if (buffer == NULL) {
		wl_resource_post_no_memory(resource);
		return;
	}

	buffer->drm = drm;
	buffer->buffer.width = width;
	buffer->buffer.height = height;
	buffer->format = format;
	buffer->offset[0] = offset0;
	buffer->stride[0] = stride0;
	buffer->offset[1] = offset1;
	buffer->stride[1] = stride1;
	buffer->offset[2] = offset2;
	buffer->stride[2] = stride2;

        drm->callbacks->reference_buffer(drm->user_data, name, buffer);
	if (buffer->driver_buffer == NULL) {
		wl_resource_post_error(resource,
				       WL_DRM_ERROR_INVALID_NAME,
				       "invalid name");
		return;
	}

	buffer->buffer.resource.object.id = id;
	buffer->buffer.resource.object.interface = &wl_buffer_interface;
	buffer->buffer.resource.object.implementation =
		(void (**)(void)) &drm_buffer_interface;
	buffer->buffer.resource.data = buffer;

	buffer->buffer.resource.destroy = destroy_buffer;
	buffer->buffer.resource.client = resource->client;

	wl_client_add_resource(resource->client, &buffer->buffer.resource);
}

static void
drm_create_buffer(struct wl_client *client, struct wl_resource *resource,
		  uint32_t id, uint32_t name, int32_t width, int32_t height,
		  uint32_t stride, uint32_t format)
{
        switch (format) {
        case WL_DRM_FORMAT_ARGB8888:
        case WL_DRM_FORMAT_XRGB8888:
        case WL_DRM_FORMAT_YUYV:
                break;
        default:
                wl_resource_post_error(resource,
                                       WL_DRM_ERROR_INVALID_FORMAT,
                                       "invalid format");
           return;
        }

        create_buffer(client, resource, id,
                      name, width, height, format, 0, stride, 0, 0, 0, 0);
}

static void
drm_create_planar_buffer(struct wl_client *client,
                         struct wl_resource *resource,
                         uint32_t id, uint32_t name,
                         int32_t width, int32_t height, uint32_t format,
                         int32_t offset0, int32_t stride0,
                         int32_t offset1, int32_t stride1,
                         int32_t offset2, int32_t stride2)
{
        switch (format) {
	case WL_DRM_FORMAT_YUV410:
	case WL_DRM_FORMAT_YUV411:
	case WL_DRM_FORMAT_YUV420:
	case WL_DRM_FORMAT_YUV422:
	case WL_DRM_FORMAT_YUV444:
	case WL_DRM_FORMAT_NV12:
        case WL_DRM_FORMAT_NV16:
                break;
        default:
                wl_resource_post_error(resource,
                                       WL_DRM_ERROR_INVALID_FORMAT,
                                       "invalid format");
           return;
        }

        create_buffer(client, resource, id, name, width, height, format,
                      offset0, stride0, offset1, stride1, offset2, stride2);
}

static void
drm_authenticate(struct wl_client *client,
		 struct wl_resource *resource, uint32_t id)
{
	struct wl_drm *drm = resource->data;

	if (drm->callbacks->authenticate(drm->user_data, id) < 0)
		wl_resource_post_error(resource,
				       WL_DRM_ERROR_AUTHENTICATE_FAIL,
				       "authenicate failed");
	else
		wl_resource_post_event(resource, WL_DRM_AUTHENTICATED);
}

const static struct wl_drm_interface drm_interface = {
	drm_authenticate,
	drm_create_buffer,
        drm_create_planar_buffer
};

static void
bind_drm(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_drm *drm = data;
	struct wl_resource *resource;

	resource = wl_client_add_object(client, &wl_drm_interface,
					&drm_interface, id, data);
	wl_resource_post_event(resource, WL_DRM_DEVICE, drm->device_name);
	wl_resource_post_event(resource, WL_DRM_FORMAT,
			       WL_DRM_FORMAT_ARGB8888);
	wl_resource_post_event(resource, WL_DRM_FORMAT,
			       WL_DRM_FORMAT_XRGB8888);
        wl_resource_post_event(resource, WL_DRM_FORMAT, WL_DRM_FORMAT_YUV410);
        wl_resource_post_event(resource, WL_DRM_FORMAT, WL_DRM_FORMAT_YUV411);
        wl_resource_post_event(resource, WL_DRM_FORMAT, WL_DRM_FORMAT_YUV420);
        wl_resource_post_event(resource, WL_DRM_FORMAT, WL_DRM_FORMAT_YUV422);
        wl_resource_post_event(resource, WL_DRM_FORMAT, WL_DRM_FORMAT_YUV444);
        wl_resource_post_event(resource, WL_DRM_FORMAT, WL_DRM_FORMAT_NV12);
        wl_resource_post_event(resource, WL_DRM_FORMAT, WL_DRM_FORMAT_NV16);
        wl_resource_post_event(resource, WL_DRM_FORMAT, WL_DRM_FORMAT_YUYV);
}

struct wl_drm *
wayland_drm_init(struct wl_display *display, char *device_name,
                 struct wayland_drm_callbacks *callbacks, void *user_data)
{
	struct wl_drm *drm;

	drm = malloc(sizeof *drm);

	drm->display = display;
	drm->device_name = strdup(device_name);
	drm->callbacks = callbacks;
	drm->user_data = user_data;

	wl_display_add_global(display, &wl_drm_interface, drm, bind_drm);

	return drm;
}

void
wayland_drm_uninit(struct wl_drm *drm)
{
	free(drm->device_name);

	/* FIXME: need wl_display_del_{object,global} */

	free(drm);
}

int
wayland_buffer_is_drm(struct wl_buffer *buffer)
{
	return buffer->resource.object.implementation == 
		(void (**)(void)) &drm_buffer_interface;
}

uint32_t
wayland_drm_buffer_get_format(struct wl_buffer *buffer_base)
{
	struct wl_drm_buffer *buffer = (struct wl_drm_buffer *) buffer_base;

	return buffer->format;
}

void *
wayland_drm_buffer_get_buffer(struct wl_buffer *buffer_base)
{
	struct wl_drm_buffer *buffer = (struct wl_drm_buffer *) buffer_base;

	return buffer->driver_buffer;
}
