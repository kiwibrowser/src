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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "util/u_debug.h"
#include "lp_limits.h"
#include "lp_memory.h"

/**
 * 32bpp RGBA swizzled tiles.  One for for each thread and each
 * possible colorbuf.  Adds up to quite a bit 8*8*64*64*4 == 1MB.
 * Several schemes exist to reduce this, such as scaling back the
 * number of threads or using a smaller tilesize when multiple
 * colorbuffers are bound.
 */
PIPE_ALIGN_VAR(LP_MIN_VECTOR_ALIGN)
uint8_t lp_swizzled_cbuf[LP_MAX_THREADS][PIPE_MAX_COLOR_BUFS][TILE_SIZE * TILE_SIZE * 4];


/* A single dummy tile used in a couple of out-of-memory situations. 
 */
PIPE_ALIGN_VAR(LP_MIN_VECTOR_ALIGN)
uint8_t lp_dummy_tile[TILE_SIZE * TILE_SIZE * 4];

