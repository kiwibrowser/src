/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2010 Chia-I Wu <olv@0xlab.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _NATIVE_DRM_H_
#define _NATIVE_DRM_H_

#include <xf86drm.h>
#include <xf86drmMode.h>

#include "pipe/p_compiler.h"
#include "util/u_format.h"
#include "pipe/p_state.h"
#include "state_tracker/drm_driver.h"

#include "common/native.h"
#include "common/native_helper.h"

#ifdef HAVE_WAYLAND_BACKEND
#include "common/native_wayland_drm_bufmgr_helper.h"
#endif

#include "gbm_gallium_drmint.h"

struct drm_config;
struct drm_crtc;
struct drm_connector;
struct drm_mode;
struct drm_surface;

struct drm_display {
   struct native_display base;

   const struct native_event_handler *event_handler;

   struct gbm_gallium_drm_device *gbmdrm;
   int own_gbm;
   int fd;
   char *device_name;
   struct drm_config *config;

   /* for modesetting */
   drmModeResPtr resources;
   struct drm_connector *connectors;
   int num_connectors;

   struct drm_surface **shown_surfaces;
   /* save the original settings of the CRTCs */
   struct drm_crtc *saved_crtcs;

#ifdef HAVE_WAYLAND_BACKEND
   struct wl_drm *wl_server_drm; /* for EGL_WL_bind_wayland_display */
#endif
};

struct drm_config {
   struct native_config base;
};

struct drm_crtc {
   drmModeCrtcPtr crtc;
   uint32_t connectors[32];
   int num_connectors;
};

struct drm_framebuffer {
   struct pipe_resource *texture;
   boolean is_passive;

   uint32_t buffer_id;
};

struct drm_surface {
   struct native_surface base;
   struct drm_display *drmdpy;

   struct resource_surface *rsurf;
   enum pipe_format color_format;
   int width, height;

   unsigned int sequence_number;
   struct drm_framebuffer front_fb, back_fb;

   boolean is_shown;
   struct drm_crtc current_crtc;

   boolean have_pageflip;
};

struct drm_connector {
   struct native_connector base;

   uint32_t connector_id;
   drmModeConnectorPtr connector;
   struct drm_mode *drm_modes;
   int num_modes;
};

struct drm_mode {
   struct native_mode base;
   drmModeModeInfo mode;
};

static INLINE struct drm_display *
drm_display(const struct native_display *ndpy)
{
   return (struct drm_display *) ndpy;
}

static INLINE struct drm_config *
drm_config(const struct native_config *nconf)
{
   return (struct drm_config *) nconf;
}

static INLINE struct drm_surface *
drm_surface(const struct native_surface *nsurf)
{
   return (struct drm_surface *) nsurf;
}

static INLINE struct drm_connector *
drm_connector(const struct native_connector *nconn)
{
   return (struct drm_connector *) nconn;
}

static INLINE struct drm_mode *
drm_mode(const struct native_mode *nmode)
{
   return (struct drm_mode *) nmode;
}

boolean
drm_display_init_modeset(struct native_display *ndpy);

void
drm_display_fini_modeset(struct native_display *ndpy);

struct native_surface *
drm_display_create_surface_from_resource(struct native_display *ndpy,
                                         struct pipe_resource *resource);

#endif /* _NATIVE_DRM_H_ */
