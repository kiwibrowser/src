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


/**
 * Code to convert images from tiled to linear and back.
 * XXX there are quite a few assumptions about color and z/stencil being
 * 32bpp.
 */


#include "util/u_format.h"
#include "util/u_memory.h"
#include "lp_tile_soa.h"
#include "lp_tile_image.h"


#define BYTES_PER_TILE (TILE_SIZE * TILE_SIZE * 4)


/**
 * Untile a 4x4 block of 32-bit words (all contiguous) to linear layout
 * at dst, with dst_stride words between rows.
 */
static void
untile_4_4_uint32(const uint32_t *src, uint32_t *dst, unsigned dst_stride)
{
   uint32_t *d0 = dst;
   uint32_t *d1 = d0 + dst_stride;
   uint32_t *d2 = d1 + dst_stride;
   uint32_t *d3 = d2 + dst_stride;

   d0[0] = src[0];   d0[1] = src[1];   d0[2] = src[4];   d0[3] = src[5];
   d1[0] = src[2];   d1[1] = src[3];   d1[2] = src[6];   d1[3] = src[7];
   d2[0] = src[8];   d2[1] = src[9];   d2[2] = src[12];  d2[3] = src[13];
   d3[0] = src[10];  d3[1] = src[11];  d3[2] = src[14];  d3[3] = src[15];
}



/**
 * Untile a 4x4 block of 16-bit words (all contiguous) to linear layout
 * at dst, with dst_stride words between rows.
 */
static void
untile_4_4_uint16(const uint16_t *src, uint16_t *dst, unsigned dst_stride)
{
   uint16_t *d0 = dst;
   uint16_t *d1 = d0 + dst_stride;
   uint16_t *d2 = d1 + dst_stride;
   uint16_t *d3 = d2 + dst_stride;

   d0[0] = src[0];   d0[1] = src[1];   d0[2] = src[4];   d0[3] = src[5];
   d1[0] = src[2];   d1[1] = src[3];   d1[2] = src[6];   d1[3] = src[7];
   d2[0] = src[8];   d2[1] = src[9];   d2[2] = src[12];  d2[3] = src[13];
   d3[0] = src[10];  d3[1] = src[11];  d3[2] = src[14];  d3[3] = src[15];
}



/**
 * Convert a 4x4 rect of 32-bit words from a linear layout into tiled
 * layout (in which all 16 words are contiguous).
 */
static void
tile_4_4_uint32(const uint32_t *src, uint32_t *dst, unsigned src_stride)
{
   const uint32_t *s0 = src;
   const uint32_t *s1 = s0 + src_stride;
   const uint32_t *s2 = s1 + src_stride;
   const uint32_t *s3 = s2 + src_stride;

   dst[0] = s0[0];   dst[1] = s0[1];   dst[4] = s0[2];   dst[5] = s0[3];
   dst[2] = s1[0];   dst[3] = s1[1];   dst[6] = s1[2];   dst[7] = s1[3];
   dst[8] = s2[0];   dst[9] = s2[1];   dst[12] = s2[2];  dst[13] = s2[3];
   dst[10] = s3[0];  dst[11] = s3[1];  dst[14] = s3[2];  dst[15] = s3[3];
}



/**
 * Convert a 4x4 rect of 16-bit words from a linear layout into tiled
 * layout (in which all 16 words are contiguous).
 */
static void
tile_4_4_uint16(const uint16_t *src, uint16_t *dst, unsigned src_stride)
{
   const uint16_t *s0 = src;
   const uint16_t *s1 = s0 + src_stride;
   const uint16_t *s2 = s1 + src_stride;
   const uint16_t *s3 = s2 + src_stride;

   dst[0] = s0[0];   dst[1] = s0[1];   dst[4] = s0[2];   dst[5] = s0[3];
   dst[2] = s1[0];   dst[3] = s1[1];   dst[6] = s1[2];   dst[7] = s1[3];
   dst[8] = s2[0];   dst[9] = s2[1];   dst[12] = s2[2];  dst[13] = s2[3];
   dst[10] = s3[0];  dst[11] = s3[1];  dst[14] = s3[2];  dst[15] = s3[3];
}



/**
 * Convert a tiled image into a linear image.
 * \param dst_stride  dest row stride in bytes
 */
void
lp_tiled_to_linear(const void *src, void *dst,
                   unsigned x, unsigned y,
                   unsigned width, unsigned height,
                   enum pipe_format format,
                   unsigned dst_stride,
                   unsigned tiles_per_row)
{
   assert(x % TILE_SIZE == 0);
   assert(y % TILE_SIZE == 0);
   /*assert(width % TILE_SIZE == 0);
     assert(height % TILE_SIZE == 0);*/

   /* Note that Z/stencil surfaces use a different tiling size than
    * color surfaces.
    */
   if (util_format_is_depth_or_stencil(format)) {
      const uint bpp = util_format_get_blocksize(format);
      const uint src_stride = dst_stride * TILE_VECTOR_WIDTH;
      const uint tile_w = TILE_VECTOR_WIDTH, tile_h = TILE_VECTOR_HEIGHT;
      const uint tiles_per_row = src_stride / (tile_w * tile_h * bpp);

      dst_stride /= bpp;   /* convert from bytes to words */

      if (bpp == 4) {
         const uint32_t *src32 = (const uint32_t *) src;
         uint32_t *dst32 = (uint32_t *) dst;
         uint i, j;

         for (j = 0; j < height; j += tile_h) {
            for (i = 0; i < width; i += tile_w) {
               /* compute offsets in 32-bit words */
               uint ii = i + x, jj = j + y;
               uint src_offset = (jj / tile_h * tiles_per_row + ii / tile_w)
                  * (tile_w * tile_h);
               uint dst_offset = jj * dst_stride + ii;
               untile_4_4_uint32(src32 + src_offset,
                                 dst32 + dst_offset,
                                 dst_stride);
            }
         }
      }
      else {
         const uint16_t *src16 = (const uint16_t *) src;
         uint16_t *dst16 = (uint16_t *) dst;
         uint i, j;

         assert(bpp == 2);

         for (j = 0; j < height; j += tile_h) {
            for (i = 0; i < width; i += tile_w) {
               /* compute offsets in 16-bit words */
               uint ii = i + x, jj = j + y;
               uint src_offset = (jj / tile_h * tiles_per_row + ii / tile_w)
                  * (tile_w * tile_h);
               uint dst_offset = jj * dst_stride + ii;
               untile_4_4_uint16(src16 + src_offset,
                                 dst16 + dst_offset,
                                 dst_stride);
            }
         }
      }
   }
   else {
      /* color image */
      const uint bpp = 4;
      const uint tile_w = TILE_SIZE, tile_h = TILE_SIZE;
      const uint bytes_per_tile = tile_w * tile_h * bpp;
      uint i, j;

      for (j = 0; j < height; j += tile_h) {
         for (i = 0; i < width; i += tile_w) {
            uint ii = i + x, jj = j + y;
            uint tile_offset = ((jj / tile_h) * tiles_per_row + ii / tile_w);
            uint byte_offset = tile_offset * bytes_per_tile;
            const uint8_t *src_tile = (uint8_t *) src + byte_offset;

            lp_tile_unswizzle_4ub(format,
                              src_tile,
                              dst, dst_stride,
                              ii, jj);
         }
      }
   }
}


/**
 * Convert a linear image into a tiled image.
 * \param src_stride  source row stride in bytes
 */
void
lp_linear_to_tiled(const void *src, void *dst,
                   unsigned x, unsigned y,
                   unsigned width, unsigned height,
                   enum pipe_format format,
                   unsigned src_stride,
                   unsigned tiles_per_row)
{
   assert(x % TILE_SIZE == 0);
   assert(y % TILE_SIZE == 0);
   /*
   assert(width % TILE_SIZE == 0);
   assert(height % TILE_SIZE == 0);
   */

   if (util_format_is_depth_or_stencil(format)) {
      const uint bpp = util_format_get_blocksize(format);
      const uint dst_stride = src_stride * TILE_VECTOR_WIDTH;
      const uint tile_w = TILE_VECTOR_WIDTH, tile_h = TILE_VECTOR_HEIGHT;
      const uint tiles_per_row = dst_stride / (tile_w * tile_h * bpp);

      src_stride /= bpp;   /* convert from bytes to words */

      if (bpp == 4) {
         const uint32_t *src32 = (const uint32_t *) src;
         uint32_t *dst32 = (uint32_t *) dst;
         uint i, j;

         for (j = 0; j < height; j += tile_h) {
            for (i = 0; i < width; i += tile_w) {
               /* compute offsets in 32-bit words */
               uint ii = i + x, jj = j + y;
               uint src_offset = jj * src_stride + ii;
               uint dst_offset = (jj / tile_h * tiles_per_row + ii / tile_w)
                  * (tile_w * tile_h);
               tile_4_4_uint32(src32 + src_offset,
                               dst32 + dst_offset,
                               src_stride);
            }
         }
      }
      else {
         const uint16_t *src16 = (const uint16_t *) src;
         uint16_t *dst16 = (uint16_t *) dst;
         uint i, j;

         assert(bpp == 2);

         for (j = 0; j < height; j += tile_h) {
            for (i = 0; i < width; i += tile_w) {
               /* compute offsets in 16-bit words */
               uint ii = i + x, jj = j + y;
               uint src_offset = jj * src_stride + ii;
               uint dst_offset = (jj / tile_h * tiles_per_row + ii / tile_w)
                  * (tile_w * tile_h);
               tile_4_4_uint16(src16 + src_offset,
                               dst16 + dst_offset,
                               src_stride);
            }
         }
      }
   }
   else {
      const uint bpp = 4;
      const uint tile_w = TILE_SIZE, tile_h = TILE_SIZE;
      const uint bytes_per_tile = tile_w * tile_h * bpp;
      uint i, j;

      for (j = 0; j < height; j += TILE_SIZE) {
         for (i = 0; i < width; i += TILE_SIZE) {
            uint ii = i + x, jj = j + y;
            uint tile_offset = ((jj / tile_h) * tiles_per_row + ii / tile_w);
            uint byte_offset = tile_offset * bytes_per_tile;
            uint8_t *dst_tile = (uint8_t *) dst + byte_offset;

            lp_tile_swizzle_4ub(format,
                             dst_tile,
                             src, src_stride,
                             ii, jj);
         }
      }
   }
}


/**
 * For testing only.
 */
void
test_tiled_linear_conversion(void *data,
                             enum pipe_format format,
                             unsigned width, unsigned height,
                             unsigned stride)
{
   /* size in tiles */
   unsigned wt = (width + TILE_SIZE - 1) / TILE_SIZE;
   unsigned ht = (height + TILE_SIZE - 1) / TILE_SIZE;

   uint8_t *tiled = MALLOC(wt * ht * TILE_SIZE * TILE_SIZE * 4);

   /*unsigned tiled_stride = wt * TILE_SIZE * TILE_SIZE * 4;*/

   lp_linear_to_tiled(data, tiled, 0, 0, width, height, format,
                      stride, wt);

   lp_tiled_to_linear(tiled, data, 0, 0, width, height, format,
                      stride, wt);

   FREE(tiled);
}

