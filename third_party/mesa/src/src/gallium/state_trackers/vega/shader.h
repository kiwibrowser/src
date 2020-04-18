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

#ifndef SHADER_H
#define SHADER_H

#include "VG/openvg.h"

struct shader;
struct vg_paint;
struct vg_context;
struct vg_image;
struct matrix;

struct shader *shader_create(struct vg_context *context);
void shader_destroy(struct shader *shader);

void shader_set_color_transform(struct shader *shader, VGboolean set);

void shader_set_masking(struct shader *shader, VGboolean set);
VGboolean shader_is_masking(struct shader *shader);

void shader_set_paint(struct shader *shader, struct vg_paint *paint);
struct vg_paint *shader_paint(struct shader *shader);

void shader_set_image_mode(struct shader *shader, VGImageMode image_mode);
VGImageMode shader_image_mode(struct shader *shader);

void shader_set_drawing_image(struct shader *shader, VGboolean drawing_image);
VGboolean shader_drawing_image(struct shader *shader);

void shader_set_image(struct shader *shader, struct vg_image *img);

void shader_set_surface_matrix(struct shader *shader,
                               const struct matrix *mat);
void shader_set_paint_matrix(struct shader *shader, const struct matrix *mat);

void shader_bind(struct shader *shader);

#endif
