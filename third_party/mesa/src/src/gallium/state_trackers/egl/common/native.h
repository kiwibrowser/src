/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2009-2010 Chia-I Wu <olv@0xlab.org>
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

#ifndef _NATIVE_H_
#define _NATIVE_H_

#include "EGL/egl.h"  /* for EGL native types */

#include "pipe/p_compiler.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "state_tracker/sw_winsys.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "native_buffer.h"
#include "native_modeset.h"
#include "native_wayland_bufmgr.h"

/**
 * Only color buffers are listed.  The others are allocated privately through,
 * for example, st_renderbuffer_alloc_storage().
 */
enum native_attachment {
   NATIVE_ATTACHMENT_FRONT_LEFT,
   NATIVE_ATTACHMENT_BACK_LEFT,
   NATIVE_ATTACHMENT_FRONT_RIGHT,
   NATIVE_ATTACHMENT_BACK_RIGHT,

   NUM_NATIVE_ATTACHMENTS
};

enum native_param_type {
   /*
    * Return TRUE if window/pixmap surfaces use the buffers of the native
    * types.
    */
   NATIVE_PARAM_USE_NATIVE_BUFFER,

   /**
    * Return TRUE if native_surface::present can preserve the buffer.
    */
   NATIVE_PARAM_PRESERVE_BUFFER,

   /**
    * Return the maximum supported swap interval.
    */
   NATIVE_PARAM_MAX_SWAP_INTERVAL,

   /**
    * Return TRUE if the display supports premultiplied alpha, regardless of
    * the surface color format.
    *
    * Note that returning TRUE for this parameter will make
    * EGL_VG_ALPHA_FORMAT_PRE_BIT to be set for all EGLConfig's with non-zero
    * EGL_ALPHA_SIZE.  EGL_VG_ALPHA_FORMAT attribute of a surface will affect
    * how the surface is presented.
    */
   NATIVE_PARAM_PREMULTIPLIED_ALPHA,

   /**
    * Return TRUE if native_surface::present supports presenting a partial
    * surface.
    */
   NATIVE_PARAM_PRESENT_REGION
};

/**
 * Control how a surface presentation should happen.
 */
struct native_present_control {
   /**< the attachment to present */
   enum native_attachment natt;

   /**< the contents of the presented attachment should be preserved */
   boolean preserve;

   /**< wait until the given vsyncs has passed since the last presentation */
   uint swap_interval;

   /**< pixels use premultiplied alpha */
   boolean premultiplied_alpha;

   /**< The region to present. y=0=top.
        If num_rects is 0, the whole surface is to be presented */
   int num_rects;
   const int *rects; /* x, y, width, height */
};

struct native_surface {
   /**
    * Available for caller's use.
    */
   void *user_data;

   void (*destroy)(struct native_surface *nsurf);

   /**
    * Present the given buffer to the native engine.
    */
   boolean (*present)(struct native_surface *nsurf,
                      const struct native_present_control *ctrl);

   /**
    * Validate the buffers of the surface.  textures, if not NULL, points to an
    * array of size NUM_NATIVE_ATTACHMENTS and the returned textures are owned
    * by the caller.  A sequence number is also returned.  The caller can use
    * it to check if anything has changed since the last call. Any of the
    * pointers may be NULL and it indicates the caller has no interest in those
    * values.
    *
    * If this function is called multiple times with different attachment
    * masks, those not listed in the latest call might be destroyed.  This
    * behavior might change in the future.
    */
   boolean (*validate)(struct native_surface *nsurf, uint attachment_mask,
                       unsigned int *seq_num, struct pipe_resource **textures,
                       int *width, int *height);

   /**
    * Wait until all native commands affecting the surface has been executed.
    */
   void (*wait)(struct native_surface *nsurf);
};

/**
 * Describe a native display config.
 */
struct native_config {
   /* available buffers and their format */
   uint buffer_mask;
   enum pipe_format color_format;

   /* supported surface types */
   boolean window_bit;
   boolean pixmap_bit;
   boolean scanout_bit;

   int native_visual_id;
   int native_visual_type;
   int level;
   boolean transparent_rgb;
   int transparent_rgb_values[3];
};

/**
 * A pipe winsys abstracts the OS.  A pipe screen abstracts the graphcis
 * hardware.  A native display consists of a pipe winsys, a pipe screen, and
 * the native display server.
 */
struct native_display {
   /**
    * The pipe screen of the native display.
    */
   struct pipe_screen *screen;

   /**
    * Context used for copy operations.
    */
   struct pipe_context *pipe;

   /**
    * Available for caller's use.
    */
   void *user_data;

   /**
    * Initialize and create the pipe screen.
    */
   boolean (*init_screen)(struct native_display *ndpy);

   void (*destroy)(struct native_display *ndpy);

   /**
    * Query the parameters of the native display.
    *
    * The return value is defined by the parameter.
    */
   int (*get_param)(struct native_display *ndpy,
                    enum native_param_type param);

   /**
    * Get the supported configs.  The configs are owned by the display, but
    * the returned array should be FREE()ed.
    */
   const struct native_config **(*get_configs)(struct native_display *ndpy,
                                               int *num_configs);

   /**
    * Get the color format of the pixmap.  Required unless no config has
    * pixmap_bit set.
    */
   boolean (*get_pixmap_format)(struct native_display *ndpy,
                                EGLNativePixmapType pix,
                                enum pipe_format *format);

   /**
    * Copy the contents of the resource to the pixmap's front-left attachment.
    * This is used to implement eglCopyBuffers.  Required unless no config has
    * pixmap_bit set.
    */
   boolean (*copy_to_pixmap)(struct native_display *ndpy,
                             EGLNativePixmapType pix,
                             struct pipe_resource *src);

   /**
    * Create a window surface.  Required unless no config has window_bit set.
    */
   struct native_surface *(*create_window_surface)(struct native_display *ndpy,
                                                   EGLNativeWindowType win,
                                                   const struct native_config *nconf);

   /**
    * Create a pixmap surface.  The native config may be NULL.  In that case, a
    * "best config" will be picked.  Required unless no config has pixmap_bit
    * set.
    */
   struct native_surface *(*create_pixmap_surface)(struct native_display *ndpy,
                                                   EGLNativePixmapType pix,
                                                   const struct native_config *nconf);

   const struct native_display_buffer *buffer;
   const struct native_display_modeset *modeset;
   const struct native_display_wayland_bufmgr *wayland_bufmgr;
};

/**
 * The handler for events that a native display may generate.  The events are
 * generated asynchronously and the handler may be called by any thread at any
 * time.
 */
struct native_event_handler {
   /**
    * This function is called when a surface needs to be validated.
    */
   void (*invalid_surface)(struct native_display *ndpy,
                           struct native_surface *nsurf,
                           unsigned int seq_num);

   struct pipe_screen *(*new_drm_screen)(struct native_display *ndpy,
                                         const char *name, int fd);
   struct pipe_screen *(*new_sw_screen)(struct native_display *ndpy,
                                        struct sw_winsys *ws);

   struct pipe_resource *(*lookup_egl_image)(struct native_display *ndpy,
                                             void *egl_image);
};

/**
 * Test whether an attachment is set in the mask.
 */
static INLINE boolean
native_attachment_mask_test(uint mask, enum native_attachment att)
{
   return !!(mask & (1 << att));
}

/**
 * Get the display copy context
 */
static INLINE struct pipe_context *
ndpy_get_copy_context(struct native_display *ndpy)
{
   if (!ndpy->pipe)
      ndpy->pipe = ndpy->screen->context_create(ndpy->screen, NULL);
   return ndpy->pipe;
}

/**
 * Free display screen and context resources
 */
static INLINE void
ndpy_uninit(struct native_display *ndpy)
{
   if (ndpy->pipe)
      ndpy->pipe->destroy(ndpy->pipe);
   if (ndpy->screen)
      ndpy->screen->destroy(ndpy->screen);
}

struct native_platform {
   const char *name;

   /**
    * Create the native display and usually establish a connection to the
    * display server.
    *
    * No event should be generated at this stage.
    */
   struct native_display *(*create_display)(void *dpy, boolean use_sw);
};

const struct native_platform *
native_get_gdi_platform(const struct native_event_handler *event_handler);

const struct native_platform *
native_get_x11_platform(const struct native_event_handler *event_handler);

const struct native_platform *
native_get_wayland_platform(const struct native_event_handler *event_handler);

const struct native_platform *
native_get_drm_platform(const struct native_event_handler *event_handler);

const struct native_platform *
native_get_fbdev_platform(const struct native_event_handler *event_handler);

const struct native_platform *
native_get_null_platform(const struct native_event_handler *event_handler);

const struct native_platform *
native_get_android_platform(const struct native_event_handler *event_handler);

#ifdef __cplusplus
}
#endif

#endif /* _NATIVE_H_ */
