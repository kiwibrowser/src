/*
 * Copyright © 2011 Intel Corporation
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
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
 */

#ifndef _GBM_GALLIUM_DRMINT_H_
#define _GBM_GALLIUM_DRMINT_H_

#include "pipe/p_state.h"

#include "gbmint.h"

#include "common.h"
#include "common_drm.h"

struct gbm_gallium_drm_device {
   struct gbm_drm_device base;

   struct pipe_screen *screen;
   void *driver;

   struct pipe_resource *(*lookup_egl_image)(void *data,
                                             void *egl_image);
   void *lookup_egl_image_data;

};

struct gbm_gallium_drm_bo {
   struct gbm_drm_bo base;

   struct pipe_resource *resource;
};

static inline struct gbm_gallium_drm_device *
gbm_gallium_drm_device(struct gbm_device *gbm)
{
   return (struct gbm_gallium_drm_device *) gbm;
}

static inline struct gbm_gallium_drm_bo *
gbm_gallium_drm_bo(struct gbm_bo *bo)
{
   return (struct gbm_gallium_drm_bo *) bo;
}

struct gbm_device *
gbm_gallium_drm_device_create(int fd);

int
gallium_screen_create(struct gbm_gallium_drm_device *gdrm);

void
gallium_screen_destroy(struct gbm_gallium_drm_device *gdrm);

#endif
