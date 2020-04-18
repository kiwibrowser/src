/*
 * Copyright (C) 2010 Maciej Cencora <m.cencora@gmail.com>
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

#include "radeon_tile.h"

#include <stdint.h>
#include <string.h>

#include "main/macros.h"
#include "radeon_debug.h"

#define MICRO_TILE_SIZE 32

static void micro_tile_8_x_4_8bit(const void * const src, unsigned src_pitch,
                                  void * const dst, unsigned dst_pitch,
                                  unsigned width, unsigned height)
{
    unsigned row; /* current source row */
    unsigned col; /* current source column */
    unsigned k; /* number of processed tiles */
    const unsigned tile_width = 8, tile_height = 4;
    const unsigned tiles_in_row = (width + (tile_width - 1)) / tile_width;

    k = 0;
    for (row = 0; row < height; row += tile_height)
    {
        for (col = 0; col < width; col += tile_width, ++k)
        {
            uint8_t *src2 = (uint8_t *)src + src_pitch * row + col;
            uint8_t *dst2 = (uint8_t *)dst + row * dst_pitch +
                             (k % tiles_in_row) * MICRO_TILE_SIZE / sizeof(uint8_t);
            unsigned j;

            for (j = 0; j < MIN2(tile_height, height - row); ++j)
            {
                unsigned columns = MIN2(tile_width, width - col);
                memcpy(dst2, src2, columns * sizeof(uint8_t));
                dst2 += tile_width;
                src2 += src_pitch;
            }
        }
    }
}

static void micro_tile_4_x_4_16bit(const void * const src, unsigned src_pitch,
                                   void * const dst, unsigned dst_pitch,
                                   unsigned width, unsigned height)
{
    unsigned row; /* current source row */
    unsigned col; /* current source column */
    unsigned k; /* number of processed tiles */
    const unsigned tile_width = 4, tile_height = 4;
    const unsigned tiles_in_row = (width + (tile_width - 1)) / tile_width;

    k = 0;
    for (row = 0; row < height; row += tile_height)
    {
        for (col = 0; col < width; col += tile_width, ++k)
        {
            uint16_t *src2 = (uint16_t *)src + src_pitch * row + col;
            uint16_t *dst2 = (uint16_t *)dst + row * dst_pitch +
                             (k % tiles_in_row) * MICRO_TILE_SIZE / sizeof(uint16_t);
            unsigned j;

            for (j = 0; j < MIN2(tile_height, height - row); ++j)
            {
                unsigned columns = MIN2(tile_width, width - col);
                memcpy(dst2, src2, columns * sizeof(uint16_t));
                dst2 += tile_width;
                src2 += src_pitch;
            }
        }
    }
}

static void micro_tile_8_x_2_16bit(const void * const src, unsigned src_pitch,
                                   void * const dst, unsigned dst_pitch,
                                   unsigned width, unsigned height)
{
    unsigned row; /* current source row */
    unsigned col; /* current source column */
    unsigned k; /* number of processed tiles */
    const unsigned tile_width = 8, tile_height = 2;
    const unsigned tiles_in_row = (width + (tile_width - 1)) / tile_width;

    k = 0;
    for (row = 0; row < height; row += tile_height)
    {
        for (col = 0; col < width; col += tile_width, ++k)
        {
            uint16_t *src2 = (uint16_t *)src + src_pitch * row + col;
            uint16_t *dst2 = (uint16_t *)dst + row * dst_pitch +
                             (k % tiles_in_row) * MICRO_TILE_SIZE / sizeof(uint16_t);
            unsigned j;

            for (j = 0; j < MIN2(tile_height, height - row); ++j)
            {
                unsigned columns = MIN2(tile_width, width - col);
                memcpy(dst2, src2, columns * sizeof(uint16_t));
                dst2 += tile_width;
                src2 += src_pitch;
            }
        }
    }
}

static void micro_tile_4_x_2_32bit(const void * const src, unsigned src_pitch,
                                   void * const dst, unsigned dst_pitch,
                                   unsigned width, unsigned height)
{
    unsigned row; /* current source row */
    unsigned col; /* current source column */
    unsigned k; /* number of processed tiles */
    const unsigned tile_width = 4, tile_height = 2;
    const unsigned tiles_in_row = (width + (tile_width - 1)) / tile_width;

    k = 0;
    for (row = 0; row < height; row += tile_height)
    {
        for (col = 0; col < width; col += tile_width, ++k)
        {
            uint32_t *src2 = (uint32_t *)src + src_pitch * row + col;
            uint32_t *dst2 = (uint32_t *)dst + row * dst_pitch +
                             (k % tiles_in_row) * MICRO_TILE_SIZE / sizeof(uint32_t);
            unsigned j;

            for (j = 0; j < MIN2(tile_height, height - row); ++j)
            {
                unsigned columns = MIN2(tile_width, width - col);
                memcpy(dst2, src2, columns * sizeof(uint32_t));
                dst2 += tile_width;
                src2 += src_pitch;
            }
        }
    }
}

static void micro_tile_2_x_2_64bit(const void * const src, unsigned src_pitch,
                                   void * const dst, unsigned dst_pitch,
                                   unsigned width, unsigned height)
{
    unsigned row; /* current source row */
    unsigned col; /* current source column */
    unsigned k; /* number of processed tiles */
    const unsigned tile_width = 2, tile_height = 2;
    const unsigned tiles_in_row = (width + (tile_width - 1)) / tile_width;

    k = 0;
    for (row = 0; row < height; row += tile_height)
    {
        for (col = 0; col < width; col += tile_width, ++k)
        {
            uint64_t *src2 = (uint64_t *)src + src_pitch * row + col;
            uint64_t *dst2 = (uint64_t *)dst + row * dst_pitch +
                             (k % tiles_in_row) * MICRO_TILE_SIZE / sizeof(uint64_t);
            unsigned j;

            for (j = 0; j < MIN2(tile_height, height - row); ++j)
            {
                unsigned columns = MIN2(tile_width, width - col);
                memcpy(dst2, src2, columns * sizeof(uint64_t));
                dst2 += tile_width;
                src2 += src_pitch;
            }
        }
    }
}

static void micro_tile_1_x_1_128bit(const void * src, unsigned src_pitch,
                                    void * dst, unsigned dst_pitch,
                                    unsigned width, unsigned height)
{
    unsigned i, j;
    const unsigned elem_size = 16; /* sizeof(uint128_t) */

    for (j = 0; j < height; ++j)
    {
        for (i = 0; i < width; ++i)
        {
            memcpy(dst, src, width * elem_size);
            dst += dst_pitch * elem_size;
            src += src_pitch * elem_size;
        }
    }
}

void tile_image(const void * src, unsigned src_pitch,
                void *dst, unsigned dst_pitch,
                gl_format format, unsigned width, unsigned height)
{
    assert(src_pitch >= width);
    assert(dst_pitch >= width);

    radeon_print(RADEON_TEXTURE, RADEON_TRACE,
                 "Software tiling: src_pitch %d, dst_pitch %d, width %d, height %d, bpp %d\n",
                 src_pitch, dst_pitch, width, height, _mesa_get_format_bytes(format));

    switch (_mesa_get_format_bytes(format))
    {
        case 16:
            micro_tile_1_x_1_128bit(src, src_pitch, dst, dst_pitch, width, height);
            break;
        case 8:
            micro_tile_2_x_2_64bit(src, src_pitch, dst, dst_pitch, width, height);
            break;
        case 4:
            micro_tile_4_x_2_32bit(src, src_pitch, dst, dst_pitch, width, height);
            break;
        case 2:
            if (_mesa_get_format_bits(format, GL_DEPTH_BITS))
            {
                micro_tile_4_x_4_16bit(src, src_pitch, dst, dst_pitch, width, height);
            }
            else
            {
                micro_tile_8_x_2_16bit(src, src_pitch, dst, dst_pitch, width, height);
            }
            break;
        case 1:
            micro_tile_8_x_4_8bit(src, src_pitch, dst, dst_pitch, width, height);
            break;
        default:
            assert(0);
            break;
    }
}

static void micro_untile_8_x_4_8bit(const void * const src, unsigned src_pitch,
                                    void * const dst, unsigned dst_pitch,
                                    unsigned width, unsigned height)
{
    unsigned row; /* current destination row */
    unsigned col; /* current destination column */
    unsigned k; /* current tile number */
    const unsigned tile_width = 8, tile_height = 4;
    const unsigned tiles_in_row = (width + (tile_width - 1)) / tile_width;

    assert(src_pitch % tile_width == 0);

    k = 0;
    for (row = 0; row < height; row += tile_height)
    {
        for (col = 0; col < width; col += tile_width, ++k)
        {
            uint8_t *src2 = (uint8_t *)src + row * src_pitch +
                             (k % tiles_in_row) * MICRO_TILE_SIZE / sizeof(uint8_t);
            uint8_t *dst2 = (uint8_t *)dst + dst_pitch * row + col;
            unsigned j;

            for (j = 0; j < MIN2(tile_height, height - row); ++j)
            {
                unsigned columns = MIN2(tile_width, width - col);
                memcpy(dst2, src2, columns * sizeof(uint8_t));
                dst2 += dst_pitch;
                src2 += tile_width;
            }
        }
    }
}

static void micro_untile_8_x_2_16bit(const void * const src, unsigned src_pitch,
                                     void * const dst, unsigned dst_pitch,
                                     unsigned width, unsigned height)
{
    unsigned row; /* current destination row */
    unsigned col; /* current destination column */
    unsigned k; /* current tile number */
    const unsigned tile_width = 8, tile_height = 2;
    const unsigned tiles_in_row = (width + (tile_width - 1)) / tile_width;

    assert(src_pitch % tile_width == 0);

    k = 0;
    for (row = 0; row < height; row += tile_height)
    {
        for (col = 0; col < width; col += tile_width, ++k)
        {
            uint16_t *src2 = (uint16_t *)src + row * src_pitch +
                             (k % tiles_in_row) * MICRO_TILE_SIZE / sizeof(uint16_t);
            uint16_t *dst2 = (uint16_t *)dst + dst_pitch * row + col;
            unsigned j;

            for (j = 0; j < MIN2(tile_height, height - row); ++j)
            {
                unsigned columns = MIN2(tile_width, width - col);
                memcpy(dst2, src2, columns * sizeof(uint16_t));
                dst2 += dst_pitch;
                src2 += tile_width;
            }
        }
    }
}

static void micro_untile_4_x_4_16bit(const void * const src, unsigned src_pitch,
                                     void * const dst, unsigned dst_pitch,
                                     unsigned width, unsigned height)
{
    unsigned row; /* current destination row */
    unsigned col; /* current destination column */
    unsigned k; /* current tile number */
    const unsigned tile_width = 4, tile_height = 4;
    const unsigned tiles_in_row = (width + (tile_width - 1)) / tile_width;

    assert(src_pitch % tile_width == 0);

    k = 0;
    for (row = 0; row < height; row += tile_height)
    {
        for (col = 0; col < width; col += tile_width, ++k)
        {
            uint16_t *src2 = (uint16_t *)src + row * src_pitch +
                             (k % tiles_in_row) * MICRO_TILE_SIZE / sizeof(uint16_t);
            uint16_t *dst2 = (uint16_t *)dst + dst_pitch * row + col;
            unsigned j;

            for (j = 0; j < MIN2(tile_height, height - row); ++j)
            {
                unsigned columns = MIN2(tile_width, width - col);
                memcpy(dst2, src2, columns * sizeof(uint16_t));
                dst2 += dst_pitch;
                src2 += tile_width;
            }
        }
    }
}

static void micro_untile_4_x_2_32bit(const void * const src, unsigned src_pitch,
                                     void * const dst, unsigned dst_pitch,
                                     unsigned width, unsigned height)
{
    unsigned row; /* current destination row */
    unsigned col; /* current destination column */
    unsigned k; /* current tile number */
    const unsigned tile_width = 4, tile_height = 2;
    const unsigned tiles_in_row = (width + (tile_width - 1)) / tile_width;

    assert(src_pitch % tile_width == 0);

    k = 0;
    for (row = 0; row < height; row += tile_height)
    {
        for (col = 0; col < width; col += tile_width, ++k)
        {
            uint32_t *src2 = (uint32_t *)src + row * src_pitch +
                             (k % tiles_in_row) * MICRO_TILE_SIZE / sizeof(uint32_t);
            uint32_t *dst2 = (uint32_t *)dst + dst_pitch * row + col;
            unsigned j;

            for (j = 0; j < MIN2(tile_height, height - row); ++j)
            {
                unsigned columns = MIN2(tile_width, width - col);
                memcpy(dst2, src2, columns * sizeof(uint32_t));
                dst2 += dst_pitch;
                src2 += tile_width;
            }
        }
    }
}

static void micro_untile_2_x_2_64bit(const void * const src, unsigned src_pitch,
                                     void * const dst, unsigned dst_pitch,
                                     unsigned width, unsigned height)
{
    unsigned row; /* current destination row */
    unsigned col; /* current destination column */
    unsigned k; /* current tile number */
    const unsigned tile_width = 2, tile_height = 2;
    const unsigned tiles_in_row = (width + (tile_width - 1)) / tile_width;

    assert(src_pitch % tile_width == 0);

    k = 0;
    for (row = 0; row < height; row += tile_height)
    {
        for (col = 0; col < width; col += tile_width, ++k)
        {
            uint64_t *src2 = (uint64_t *)src + row * src_pitch +
                             (k % tiles_in_row) * MICRO_TILE_SIZE / sizeof(uint64_t);
            uint64_t *dst2 = (uint64_t *)dst + dst_pitch * row + col;
            unsigned j;

            for (j = 0; j < MIN2(tile_height, height - row); ++j)
            {
                unsigned columns = MIN2(tile_width, width - col);
                memcpy(dst2, src2, columns * sizeof(uint64_t));
                dst2 += dst_pitch;
                src2 += tile_width;
            }
        }
    }
}

static void micro_untile_1_x_1_128bit(const void * src, unsigned src_pitch,
                                      void * dst, unsigned dst_pitch,
                                      unsigned width, unsigned height)
{
    unsigned i, j;
    const unsigned elem_size = 16; /* sizeof(uint128_t) */

    for (j = 0; j < height; ++j)
    {
        for (i = 0; i < width; ++i)
        {
            memcpy(dst, src, width * elem_size);
            dst += dst_pitch * elem_size;
            src += src_pitch * elem_size;
        }
    }
}

void untile_image(const void * src, unsigned src_pitch,
                  void *dst, unsigned dst_pitch,
                  gl_format format, unsigned width, unsigned height)
{
    assert(src_pitch >= width);
    assert(dst_pitch >= width);

    radeon_print(RADEON_TEXTURE, RADEON_TRACE,
                 "Software untiling: src_pitch %d, dst_pitch %d, width %d, height %d, bpp %d\n",
                 src_pitch, dst_pitch, width, height, _mesa_get_format_bytes(format));

    switch (_mesa_get_format_bytes(format))
    {
        case 16:
            micro_untile_1_x_1_128bit(src, src_pitch, dst, dst_pitch, width, height);
            break;
        case 8:
            micro_untile_2_x_2_64bit(src, src_pitch, dst, dst_pitch, width, height);
            break;
        case 4:
            micro_untile_4_x_2_32bit(src, src_pitch, dst, dst_pitch, width, height);
            break;
        case 2:
            if (_mesa_get_format_bits(format, GL_DEPTH_BITS))
            {
                micro_untile_4_x_4_16bit(src, src_pitch, dst, dst_pitch, width, height);
            }
            else
            {
                micro_untile_8_x_2_16bit(src, src_pitch, dst, dst_pitch, width, height);
            }
            break;
        case 1:
            micro_untile_8_x_4_8bit(src, src_pitch, dst, dst_pitch, width, height);
            break;
        default:
            assert(0);
            break;
    }
}

void get_tile_size(gl_format format, unsigned *block_width, unsigned *block_height)
{
    switch (_mesa_get_format_bytes(format))
    {
        case 16:
            *block_width = 1;
            *block_height = 1;
            break;
        case 8:
            *block_width = 2;
            *block_height = 2;
            break;
        case 4:
            *block_width = 4;
            *block_height = 2;
            break;
        case 2:
            if (_mesa_get_format_bits(format, GL_DEPTH_BITS))
            {
                *block_width = 4;
                *block_height = 4;
            }
            else
            {
                *block_width = 8;
                *block_height = 2;
            }
            break;
        case 1:
            *block_width = 8;
            *block_height = 4;
            break;
        default:
            assert(0);
            break;
    }
}
