/*
 * Copyright (C) 2010 Advanced Micro Devices, Inc.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef RADEON_BLIT_H
#define RADEON_BLIT_H

void r100_blit_init(struct r100_context *r100);

unsigned r100_check_blit(gl_format mesa_format, uint32_t dst_pitch);

unsigned r100_blit(struct gl_context *ctx,
                   struct radeon_bo *src_bo,
                   intptr_t src_offset,
                   gl_format src_mesaformat,
                   unsigned src_pitch,
                   unsigned src_width,
                   unsigned src_height,
                   unsigned src_x_offset,
                   unsigned src_y_offset,
                   struct radeon_bo *dst_bo,
                   intptr_t dst_offset,
                   gl_format dst_mesaformat,
                   unsigned dst_pitch,
                   unsigned dst_width,
                   unsigned dst_height,
                   unsigned dst_x_offset,
                   unsigned dst_y_offset,
                   unsigned width,
                   unsigned height,
                   unsigned flip_y);

#endif // RADEON_BLIT_H
