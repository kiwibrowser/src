/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef IMAGES_H
#define IMAGES_H

#include "vg_context.h"
#include "pipe/p_state.h"

struct pipe_resource;
struct array;
struct vg_context;
struct pipe_surface;

struct vg_image {
   struct vg_object base;
   VGImageFormat format;
   VGint x, y;
   VGint width, height;

   struct vg_image *parent;

   struct pipe_sampler_view *sampler_view;
   struct pipe_sampler_state sampler;

   struct array *children_array;
};

struct vg_image *image_create(VGImageFormat format,
                              VGint width, VGint height);
void image_destroy(struct vg_image *img);

void image_clear(struct vg_image *img,
                 VGint x, VGint y, VGint width, VGint height);

void image_sub_data(struct vg_image *image,
                    const void * data,
                    VGint dataStride,
                    VGImageFormat dataFormat,
                    VGint x, VGint y,
                    VGint width, VGint height);

void image_get_sub_data(struct vg_image * image,
                        void * data,
                        VGint dataStride,
                        VGImageFormat dataFormat,
                        VGint x, VGint y,
                        VGint width, VGint height);

struct vg_image *image_child_image(struct vg_image *parent,
                                   VGint x, VGint y,
                                   VGint width, VGint height);

void image_copy(struct vg_image *dst, VGint dx, VGint dy,
                struct vg_image *src, VGint sx, VGint sy,
                VGint width, VGint height,
                VGboolean dither);

void image_draw(struct vg_image *img, struct matrix *matrix);

void image_set_pixels(VGint dx, VGint dy,
                      struct vg_image *src, VGint sx, VGint sy,
                      VGint width, VGint height);
void image_get_pixels(struct vg_image *dst, VGint dx, VGint dy,
                      VGint sx, VGint sy,
                      VGint width, VGint height);

VGint image_bind_samplers(struct vg_image *dst, struct pipe_sampler_state **samplers,
                          struct pipe_sampler_view **sampler_views);

VGboolean vg_image_overlaps(struct vg_image *dst,
                            struct vg_image *src);

VGint image_sampler_filter(struct vg_context *ctx);

void vg_copy_surface(struct vg_context *ctx,
                     struct pipe_surface *dst, VGint dx, VGint dy,
                     struct pipe_surface *src, VGint sx, VGint sy,
                     VGint width, VGint height);

#endif
