/**************************************************************************
 * 
 * Copyright 2010 VMware, Inc.  All Rights Reserved.
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
 * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#ifndef LP_TILE_IMAGE_H
#define LP_TILE_IMAGE_H


void
lp_tiled_to_linear(const void *src, void *dst,
                   unsigned x, unsigned y,
                   unsigned width, unsigned height,
                   enum pipe_format format,
                   unsigned dst_stride,
                   unsigned tiles_per_row);


void
lp_linear_to_tiled(const void *src, void *dst,
                   unsigned x, unsigned y,
                   unsigned width, unsigned height,
                   enum pipe_format format,
                   unsigned src_stride,
                   unsigned tiles_per_row);


void
test_tiled_linear_conversion(void *data,
                             enum pipe_format format,
                             unsigned width, unsigned height,
                             unsigned stride);


#endif /* LP_TILE_IMAGE_H */
