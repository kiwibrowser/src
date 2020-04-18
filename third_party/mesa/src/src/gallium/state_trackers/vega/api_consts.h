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

#ifndef API_CONSTS_H
#define API_CONSTS_H

/*must be at least 32*/
#define VEGA_MAX_SCISSOR_RECTS 32

/*must be at least 16*/
#define VEGA_MAX_DASH_COUNT 32

/*must be at least 7*/
#define VEGA_MAX_KERNEL_SIZE 7

/*must be at least 15*/
#define VEGA_MAX_SEPARABLE_KERNEL_SIZE 15

/*must be at least 32*/
#define VEGA_MAX_COLOR_RAMP_STOPS 256

#define VEGA_MAX_IMAGE_WIDTH 2048

#define VEGA_MAX_IMAGE_HEIGHT 2048

#define VEGA_MAX_IMAGE_PIXELS (2048*2048)

#define VEGA_MAX_IMAGE_BYTES (2048*2048 * 4)

/*must be at least 128*/
#define VEGA_MAX_GAUSSIAN_STD_DEVIATION 128

#endif
