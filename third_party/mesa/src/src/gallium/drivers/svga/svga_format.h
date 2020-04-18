/**********************************************************
 * Copyright 2011 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#ifndef SVGA_FORMAT_H_
#define SVGA_FORMAT_H_


#include "pipe/p_format.h"
#include "svga_types.h"
#include "svga_reg.h"
#include "svga3d_reg.h"


struct svga_screen;


enum SVGA3dSurfaceFormat
svga_translate_format(struct svga_screen *ss,
                      enum pipe_format format,
                      unsigned bind);

void
svga_get_format_cap(struct svga_screen *ss,
                    SVGA3dSurfaceFormat format,
                    SVGA3dSurfaceFormatCaps *caps);

void
svga_format_size(SVGA3dSurfaceFormat format,
                 unsigned *block_width,
                 unsigned *block_height,
                 unsigned *bytes_per_block);


#endif /* SVGA_FORMAT_H_ */
