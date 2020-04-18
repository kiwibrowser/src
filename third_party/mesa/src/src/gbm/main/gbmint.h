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

#ifndef INTERNAL_H_
#define INTERNAL_H_

#include "gbm.h"
#include <sys/stat.h>

/* GCC visibility */
#if defined(__GNUC__) && __GNUC__ >= 4
#define GBM_EXPORT __attribute__ ((visibility("default")))
#else
#define GBM_EXPORT
#endif

/**
 * \file gbmint.h
 * \brief Internal implementation details of gbm
 */

/**
 * The device used for the memory allocation.
 *
 * The members of this structure should be not accessed directly
 */
struct gbm_device {
   /* Hack to make a gbm_device detectable by its first element. */
   struct gbm_device *(*dummy)(int);

   int fd;
   const char *name;
   unsigned int refcount;
   struct stat stat;

   void (*destroy)(struct gbm_device *gbm);
   int (*is_format_supported)(struct gbm_device *gbm,
                              uint32_t format,
                              uint32_t usage);

   struct gbm_bo *(*bo_create)(struct gbm_device *gbm,
                               uint32_t width, uint32_t height,
                               uint32_t format,
                               uint32_t usage);
   struct gbm_bo *(*bo_import)(struct gbm_device *gbm, uint32_t type,
                               void *buffer, uint32_t usage);
   int (*bo_write)(struct gbm_bo *bo, const void *buf, size_t data);
   void (*bo_destroy)(struct gbm_bo *bo);

   struct gbm_surface *(*surface_create)(struct gbm_device *gbm,
                                         uint32_t width, uint32_t height,
                                         uint32_t format, uint32_t flags);
   struct gbm_bo *(*surface_lock_front_buffer)(struct gbm_surface *surface);
   void (*surface_release_buffer)(struct gbm_surface *surface,
                                  struct gbm_bo *bo);
   int (*surface_has_free_buffers)(struct gbm_surface *surface);
   void (*surface_destroy)(struct gbm_surface *surface);
};

/**
 * The allocated buffer object.
 *
 * The members in this structure should not be accessed directly.
 */
struct gbm_bo {
   struct gbm_device *gbm;
   uint32_t width;
   uint32_t height;
   uint32_t stride;
   uint32_t format;
   union gbm_bo_handle  handle;
   void *user_data;
   void (*destroy_user_data)(struct gbm_bo *, void *);
};

struct gbm_surface {
   struct gbm_device *gbm;
   uint32_t width;
   uint32_t height;
   uint32_t format;
   uint32_t flags;
};

struct gbm_backend {
   const char *backend_name;
   struct gbm_device *(*create_device)(int fd);
};

GBM_EXPORT struct gbm_device *
_gbm_mesa_get_device(int fd);

#endif
