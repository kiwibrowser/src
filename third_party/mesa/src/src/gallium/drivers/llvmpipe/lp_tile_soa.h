/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef LP_TILE_SOA_H
#define LP_TILE_SOA_H

#include "pipe/p_compiler.h"
#include "tgsi/tgsi_exec.h" /* for TGSI_NUM_CHANNELS */
#include "lp_limits.h"

#ifdef __cplusplus
extern "C" {
#endif


struct pipe_transfer;


#define TILE_VECTOR_HEIGHT 4
#define TILE_VECTOR_WIDTH 4

extern const unsigned char
tile_offset[TILE_VECTOR_HEIGHT][TILE_VECTOR_WIDTH];

#define TILE_C_STRIDE (TILE_VECTOR_HEIGHT * TILE_VECTOR_WIDTH) //16
#define TILE_X_STRIDE (TGSI_NUM_CHANNELS * TILE_C_STRIDE) //64
#define TILE_Y_STRIDE (TILE_VECTOR_HEIGHT * TILE_SIZE * TGSI_NUM_CHANNELS) //1024


#ifdef DEBUG
extern unsigned lp_tile_unswizzle_count;
extern unsigned lp_tile_swizzle_count;
#endif


/**
 * Return offset of the given pixel (and color channel) from the start
 * of a tile, in bytes.
 */
static INLINE unsigned
tile_pixel_offset(unsigned x, unsigned y, unsigned c)
{
   unsigned ix = (x / TILE_VECTOR_WIDTH) * TILE_X_STRIDE;
   unsigned iy = (y / TILE_VECTOR_HEIGHT) * TILE_Y_STRIDE;
   unsigned offset = iy + ix + c * TILE_C_STRIDE +
      tile_offset[y % TILE_VECTOR_HEIGHT][x % TILE_VECTOR_WIDTH];
   return offset;
}


#define TILE_PIXEL(_p, _x, _y, _c)   ((_p)[tile_pixel_offset(_x, _y, _c)])


void
lp_tile_swizzle_4ub(enum pipe_format format,
                 uint8_t *dst,
                 const void *src, unsigned src_stride,
                 unsigned x, unsigned y);


void
lp_tile_unswizzle_4ub(enum pipe_format format,
                  const uint8_t *src,
                  void *dst, unsigned dst_stride,
                  unsigned x, unsigned y);



#ifdef __cplusplus
}
#endif

#endif
