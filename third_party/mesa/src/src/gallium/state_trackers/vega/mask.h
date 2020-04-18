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

#ifndef MASK_H
#define MASK_H

#include "vg_context.h"

struct path;
struct vg_image;
struct pipe_resource;

struct vg_mask_layer *mask_layer_create(VGint width, VGint height);
void mask_layer_destroy(struct vg_mask_layer *layer);
void mask_layer_fill(struct vg_mask_layer *layer,
                     VGint x, VGint y,
                     VGint width, VGint height,
                     VGfloat value);
VGint mask_layer_width(struct vg_mask_layer *layer);
VGint mask_layer_height(struct vg_mask_layer *layer);
void mask_copy(struct vg_mask_layer *layer,
               VGint sx, VGint sy,
               VGint dx, VGint dy,
               VGint width, VGint height);

void mask_render_to(struct path *path,
                    VGbitfield paint_modes,
                    VGMaskOperation operation);

void mask_using_layer(struct vg_mask_layer *layer,
                      VGMaskOperation operation,
                      VGint x, VGint y,
                      VGint width, VGint height);
void mask_using_image(struct vg_image *image,
                      VGMaskOperation operation,
                      VGint x, VGint y,
                      VGint width, VGint height);
void mask_fill(VGint x, VGint y,
               VGint width, VGint height,
               VGfloat value);

VGint mask_bind_samplers(struct pipe_sampler_state **samplers,
                         struct pipe_sampler_view **sampler_views);

#endif
