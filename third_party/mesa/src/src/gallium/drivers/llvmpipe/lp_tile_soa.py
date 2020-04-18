#!/usr/bin/env python

CopyRight = '''
/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * Pixel format accessor functions.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */
'''


import sys
import os.path

sys.path.insert(0, os.path.join(os.path.dirname(sys.argv[0]), '../../auxiliary/util'))

from u_format_pack import *


def is_format_supported(format):
    '''Determines whether we actually have the plumbing necessary to generate the 
    to read/write to/from this format.'''

    # FIXME: Ideally we would support any format combination here.

    if format.name == 'PIPE_FORMAT_R11G11B10_FLOAT':
        return True;

    if format.name == 'PIPE_FORMAT_R9G9B9E5_FLOAT':
        return True;

    if format.layout != PLAIN:
        return False

    for i in range(4):
        channel = format.channels[i]
        if channel.type not in (VOID, UNSIGNED, SIGNED, FLOAT):
            return False
        if channel.type == FLOAT and channel.size not in (16, 32 ,64):
            return False

    if format.colorspace not in ('rgb', 'srgb'):
        return False

    return True


def generate_format_read(format, dst_channel, dst_native_type, dst_suffix):
    '''Generate the function to read pixels from a particular format'''

    name = format.short_name()

    src_native_type = native_type(format)

    print 'static void'
    print 'lp_tile_%s_swizzle_%s(%s * restrict dst, const uint8_t * restrict src, unsigned src_stride, unsigned x0, unsigned y0)' % (name, dst_suffix, dst_native_type)
    print '{'
    print '   unsigned x, y;'
    print '   const uint8_t *src_row = src + y0*src_stride;'
    print '   for (y = 0; y < TILE_SIZE; ++y) {'
    print '      const %s *src_pixel = (const %s *)(src_row + x0*%u);' % (src_native_type, src_native_type, format.stride())
    print '      for (x = 0; x < TILE_SIZE; ++x) {'

    names = ['']*4
    if format.colorspace in ('rgb', 'srgb'):
        for i in range(4):
            swizzle = format.swizzles[i]
            if swizzle < 4:
                names[swizzle] += 'rgba'[i]
    elif format.colorspace == 'zs':
        swizzle = format.swizzles[0]
        if swizzle < 4:
            names[swizzle] = 'z'
        else:
            assert False
    else:
        assert False

    if format.name == 'PIPE_FORMAT_R11G11B10_FLOAT':
        print '         float tmp[3];'
        print '         uint8_t r, g, b;'
        print '         r11g11b10f_to_float3(*src_pixel++, tmp);'
        for i in range(3):
            print '         %s = tmp[%d] * 0xff;' % (names[i], i)
    elif format.name == 'PIPE_FORMAT_R9G9B9E5_FLOAT':
        print '         float tmp[3];'
        print '         uint8_t r, g, b;'
        print '         rgb9e5_to_float3(*src_pixel++, tmp);'
        for i in range(3):
            print '         %s = tmp[%d] * 0xff;' % (names[i], i)
    elif format.layout == PLAIN:
        if not format.is_array():
            print '         %s pixel = *src_pixel++;' % src_native_type
            shift = 0;
            for i in range(4):
                src_channel = format.channels[i]
                width = src_channel.size
                if names[i]:
                    value = 'pixel'
                    mask = (1 << width) - 1
                    if shift:
                        value = '(%s >> %u)' % (value, shift)
                    if shift + width < format.block_size():
                        value = '(%s & 0x%x)' % (value, mask)
                    value = conversion_expr(src_channel, dst_channel, dst_native_type, value, clamp=False)
                    print '         %s %s = %s;' % (dst_native_type, names[i], value)
                shift += width
        else:
            for i in range(4):
                if names[i]:
                    print '         %s %s;' % (dst_native_type, names[i])
            for i in range(4):
                src_channel = format.channels[i]
                if names[i]:
                    value = '(*src_pixel++)'
                    value = conversion_expr(src_channel, dst_channel, dst_native_type, value, clamp=False)
                    print '         %s = %s;' % (names[i], value)
                elif src_channel.size:
                    print '         ++src_pixel;'
    else:
        assert False

    for i in range(4):
        if format.colorspace in ('rgb', 'srgb'):
            swizzle = format.swizzles[i]
            if swizzle < 4:
                value = names[swizzle]
            elif swizzle == SWIZZLE_0:
                value = '0'
            elif swizzle == SWIZZLE_1:
                value = get_one(dst_channel)
            else:
                assert False
        elif format.colorspace == 'zs':
            if i < 3:
                value = 'z'
            else:
                value = get_one(dst_channel)
        else:
            assert False
        print '         TILE_PIXEL(dst, x, y, %u) = %s; /* %s */' % (i, value, 'rgba'[i])

    print '      }'
    print '      src_row += src_stride;'
    print '   }'
    print '}'
    print
    

def pack_rgba(format, src_channel, r, g, b, a):
    """Return an expression for packing r, g, b, a into a pixel of the
    given format.  Ex: '(b << 24) | (g << 16) | (r << 8) | (a << 0)'
    """
    assert format.colorspace in ('rgb', 'srgb')
    inv_swizzle = format.inv_swizzles()
    shift = 0
    expr = None
    for i in range(4):
        # choose r, g, b, or a depending on the inverse swizzle term
        if inv_swizzle[i] == 0:
            value = r
        elif inv_swizzle[i] == 1:
            value = g
        elif inv_swizzle[i] == 2:
            value = b
        elif inv_swizzle[i] == 3:
            value = a
        else:
            value = None

        if value:
            dst_channel = format.channels[i]
            dst_native_type = native_type(format)
            value = conversion_expr(src_channel, dst_channel, dst_native_type, value, clamp=False)
            term = "((%s) << %d)" % (value, shift)
            if expr:
                expr = expr + " | " + term
            else:
                expr = term

        width = format.channels[i].size
        shift = shift + width
    return expr


def emit_unrolled_unswizzle_code(format, src_channel):
    '''Emit code for writing a block based on unrolled loops.
    This is considerably faster than the TILE_PIXEL-based code below.
    '''
    dst_native_type = 'uint%u_t' % format.block_size()
    print '   const unsigned dstpix_stride = dst_stride / %d;' % format.stride()
    print '   %s *dstpix = (%s *) dst;' % (dst_native_type, dst_native_type)
    print '   unsigned int qx, qy, i;'
    print
    print '   for (qy = 0; qy < TILE_SIZE; qy += TILE_VECTOR_HEIGHT) {'
    print '      const unsigned py = y0 + qy;'
    print '      for (qx = 0; qx < TILE_SIZE; qx += TILE_VECTOR_WIDTH) {'
    print '         const unsigned px = x0 + qx;'
    print '         const uint8_t *r = src + 0 * TILE_C_STRIDE;'
    print '         const uint8_t *g = src + 1 * TILE_C_STRIDE;'
    print '         const uint8_t *b = src + 2 * TILE_C_STRIDE;'
    print '         const uint8_t *a = src + 3 * TILE_C_STRIDE;'
    print '         (void) r; (void) g; (void) b; (void) a; /* silence warnings */'
    print '         for (i = 0; i < TILE_C_STRIDE; i += 2) {'
    print '            const uint32_t pixel0 = %s;' % pack_rgba(format, src_channel, "r[i+0]", "g[i+0]", "b[i+0]", "a[i+0]")
    print '            const uint32_t pixel1 = %s;' % pack_rgba(format, src_channel, "r[i+1]", "g[i+1]", "b[i+1]", "a[i+1]")
    print '            const unsigned offset = (py + tile_y_offset[i]) * dstpix_stride + (px + tile_x_offset[i]);'
    print '            dstpix[offset + 0] = pixel0;'
    print '            dstpix[offset + 1] = pixel1;'
    print '         }'
    print '         src += TILE_X_STRIDE;'
    print '      }'
    print '   }'


def emit_tile_pixel_unswizzle_code(format, src_channel):
    '''Emit code for writing a block based on the TILE_PIXEL macro.'''
    dst_native_type = native_type(format)

    inv_swizzle = format.inv_swizzles()

    print '   unsigned x, y;'
    print '   uint8_t *dst_row = dst + y0*dst_stride;'
    print '   for (y = 0; y < TILE_SIZE; ++y) {'
    print '      %s *dst_pixel = (%s *)(dst_row + x0*%u);' % (dst_native_type, dst_native_type, format.stride())
    print '      for (x = 0; x < TILE_SIZE; ++x) {'

    if format.name == 'PIPE_FORMAT_R11G11B10_FLOAT':
        print '         float tmp[3];'
        for i in range(3):
            print '         tmp[%d] = ubyte_to_float(TILE_PIXEL(src, x, y, %u));' % (i, inv_swizzle[i])
        print '         *dst_pixel++ = float3_to_r11g11b10f(tmp);'
    elif format.name == 'PIPE_FORMAT_R9G9B9E5_FLOAT':
        print '         float tmp[3];'
        for i in range(3):
            print '         tmp[%d] = ubyte_to_float(TILE_PIXEL(src, x, y, %u));' % (i, inv_swizzle[i])
        print '         *dst_pixel++ = float3_to_rgb9e5(tmp);'
    elif format.layout == PLAIN:
        if not format.is_array():
            print '         %s pixel = 0;' % dst_native_type
            shift = 0;
            for i in range(4):
                dst_channel = format.channels[i]
                width = dst_channel.size
                if inv_swizzle[i] is not None:
                    value = 'TILE_PIXEL(src, x, y, %u)' % inv_swizzle[i]
                    value = conversion_expr(src_channel, dst_channel, dst_native_type, value, clamp=False)
                    if shift:
                        value = '(%s << %u)' % (value, shift)
                    print '         pixel |= %s;' % value
                shift += width
            print '         *dst_pixel++ = pixel;'
        else:
            for i in range(4):
                dst_channel = format.channels[i]
                if inv_swizzle[i] is not None:
                    value = 'TILE_PIXEL(src, x, y, %u)' % inv_swizzle[i]
                    value = conversion_expr(src_channel, dst_channel, dst_native_type, value, clamp=False)
                    print '         *dst_pixel++ = %s;' % value
                elif dst_channel.size:
                    print '         ++dst_pixel;'
    else:
        assert False

    print '      }'
    print '      dst_row += dst_stride;'
    print '   }'


def generate_format_write(format, src_channel, src_native_type, src_suffix):
    '''Generate the function to write pixels to a particular format'''

    name = format.short_name()

    print 'static void'
    print 'lp_tile_%s_unswizzle_%s(const %s * restrict src, uint8_t * restrict dst, unsigned dst_stride, unsigned x0, unsigned y0)' % (name, src_suffix, src_native_type)
    print '{'
    if format.layout == PLAIN \
        and format.colorspace == 'rgb' \
        and format.block_size() <= 32 \
        and format.is_pot() \
        and not format.is_mixed() \
        and (format.channels[0].type == UNSIGNED \
             or format.channels[1].type == UNSIGNED):
        emit_unrolled_unswizzle_code(format, src_channel)
    else:
        emit_tile_pixel_unswizzle_code(format, src_channel)
    print '}'
    print
    

def generate_sse2():
    print '''
#if defined(PIPE_ARCH_SSE)

#include "util/u_sse.h"

static ALWAYS_INLINE void 
swz4( const __m128i * restrict x, 
      const __m128i * restrict y, 
      const __m128i * restrict z, 
      const __m128i * restrict w, 
      __m128i * restrict a, 
      __m128i * restrict b, 
      __m128i * restrict c, 
      __m128i * restrict d)
{
   __m128i i, j, k, l;
   __m128i m, n, o, p;
   __m128i e, f, g, h;

   m = _mm_unpacklo_epi8(*x,*y);
   n = _mm_unpackhi_epi8(*x,*y);
   o = _mm_unpacklo_epi8(*z,*w);
   p = _mm_unpackhi_epi8(*z,*w);

   i = _mm_unpacklo_epi16(m,n);
   j = _mm_unpackhi_epi16(m,n);
   k = _mm_unpacklo_epi16(o,p);
   l = _mm_unpackhi_epi16(o,p);

   e = _mm_unpacklo_epi8(i,j);
   f = _mm_unpackhi_epi8(i,j);
   g = _mm_unpacklo_epi8(k,l);
   h = _mm_unpackhi_epi8(k,l);

   *a = _mm_unpacklo_epi64(e,g);
   *b = _mm_unpackhi_epi64(e,g);
   *c = _mm_unpacklo_epi64(f,h);
   *d = _mm_unpackhi_epi64(f,h);
}

static ALWAYS_INLINE void
unswz4( const __m128i * restrict a, 
        const __m128i * restrict b, 
        const __m128i * restrict c, 
        const __m128i * restrict d, 
        __m128i * restrict x, 
        __m128i * restrict y, 
        __m128i * restrict z, 
        __m128i * restrict w)
{
   __m128i i, j, k, l;
   __m128i m, n, o, p;

   i = _mm_unpacklo_epi8(*a,*b);
   j = _mm_unpackhi_epi8(*a,*b);
   k = _mm_unpacklo_epi8(*c,*d);
   l = _mm_unpackhi_epi8(*c,*d);

   m = _mm_unpacklo_epi16(i,k);
   n = _mm_unpackhi_epi16(i,k);
   o = _mm_unpacklo_epi16(j,l);
   p = _mm_unpackhi_epi16(j,l);

   *x = _mm_unpacklo_epi64(m,n);
   *y = _mm_unpackhi_epi64(m,n);
   *z = _mm_unpacklo_epi64(o,p);
   *w = _mm_unpackhi_epi64(o,p);
}

static void
lp_tile_b8g8r8a8_unorm_swizzle_4ub_sse2(uint8_t * restrict dst,
                                        const uint8_t * restrict src, unsigned src_stride,
                                        unsigned x0, unsigned y0)
{
   __m128i *dst128 = (__m128i *) dst;
   unsigned x, y;
   
   src += y0 * src_stride;
   src += x0 * sizeof(uint32_t);

   for (y = 0; y < TILE_SIZE; y += 4) {
      const uint8_t *src_row = src;

      for (x = 0; x < TILE_SIZE; x += 4) {
         swz4((const __m128i *) (src_row + 0 * src_stride),
              (const __m128i *) (src_row + 1 * src_stride),
              (const __m128i *) (src_row + 2 * src_stride),
              (const __m128i *) (src_row + 3 * src_stride),
              dst128 + 2,     /* b */
              dst128 + 1,     /* g */
              dst128 + 0,     /* r */
              dst128 + 3);    /* a */

         dst128 += 4;
         src_row += sizeof(__m128i);
      }

      src += 4 * src_stride;
   }
}

static void
lp_tile_b8g8r8a8_unorm_unswizzle_4ub_sse2(const uint8_t * restrict src,
                                          uint8_t * restrict dst, unsigned dst_stride,
                                          unsigned x0, unsigned y0)
{
   unsigned int x, y;
   const __m128i *src128 = (const __m128i *) src;
   
   dst += y0 * dst_stride;
   dst += x0 * sizeof(uint32_t);
   
   for (y = 0; y < TILE_SIZE; y += 4) {
      const uint8_t *dst_row = dst;

      for (x = 0; x < TILE_SIZE; x += 4) {
         unswz4( &src128[2],     /* b */
                 &src128[1],     /* g */
                 &src128[0],     /* r */
                 &src128[3],     /* a */
                 (__m128i *) (dst_row + 0 * dst_stride),
                 (__m128i *) (dst_row + 1 * dst_stride),
                 (__m128i *) (dst_row + 2 * dst_stride),
                 (__m128i *) (dst_row + 3 * dst_stride));

         src128 += 4;
         dst_row += sizeof(__m128i);;
      }

      dst += 4 * dst_stride;
   }
}

static void
lp_tile_b8g8r8x8_unorm_swizzle_4ub_sse2(uint8_t * restrict dst,
                                        const uint8_t * restrict src, unsigned src_stride,
                                        unsigned x0, unsigned y0)
{
   __m128i *dst128 = (__m128i *) dst;
   unsigned x, y;

   src += y0 * src_stride;
   src += x0 * sizeof(uint32_t);

   for (y = 0; y < TILE_SIZE; y += 4) {
      const uint8_t *src_row = src;

      for (x = 0; x < TILE_SIZE; x += 4) {
         swz4((const __m128i *) (src_row + 0 * src_stride),
              (const __m128i *) (src_row + 1 * src_stride),
              (const __m128i *) (src_row + 2 * src_stride),
              (const __m128i *) (src_row + 3 * src_stride),
              dst128 + 2,     /* b */
              dst128 + 1,     /* g */
              dst128 + 0,     /* r */
              dst128 + 3);    /* a */

         dst128 += 4;
         src_row += sizeof(__m128i);
      }

      src += 4 * src_stride;
   }
}

static void
lp_tile_b8g8r8x8_unorm_unswizzle_4ub_sse2(const uint8_t * restrict src,
                                          uint8_t * restrict dst, unsigned dst_stride,
                                          unsigned x0, unsigned y0)
{
   unsigned int x, y;
   const __m128i *src128 = (const __m128i *) src;

   dst += y0 * dst_stride;
   dst += x0 * sizeof(uint32_t);

   for (y = 0; y < TILE_SIZE; y += 4) {
      const uint8_t *dst_row = dst;

      for (x = 0; x < TILE_SIZE; x += 4) {
         unswz4( &src128[2],     /* b */
                 &src128[1],     /* g */
                 &src128[0],     /* r */
                 &src128[3],     /* a */
                 (__m128i *) (dst_row + 0 * dst_stride),
                 (__m128i *) (dst_row + 1 * dst_stride),
                 (__m128i *) (dst_row + 2 * dst_stride),
                 (__m128i *) (dst_row + 3 * dst_stride));

         src128 += 4;
         dst_row += sizeof(__m128i);;
      }

      dst += 4 * dst_stride;
   }
}

#endif /* PIPE_ARCH_SSE */
'''


def generate_swizzle(formats, dst_channel, dst_native_type, dst_suffix):
    '''Generate the dispatch function to read pixels from any format'''

    for format in formats:
        if is_format_supported(format):
            generate_format_read(format, dst_channel, dst_native_type, dst_suffix)

    print 'void'
    print 'lp_tile_swizzle_%s(enum pipe_format format, %s *dst, const void *src, unsigned src_stride, unsigned x, unsigned y)' % (dst_suffix, dst_native_type)
    print '{'
    print '   void (*func)(%s * restrict dst, const uint8_t * restrict src, unsigned src_stride, unsigned x0, unsigned y0);' % dst_native_type
    print '#ifdef DEBUG'
    print '   lp_tile_swizzle_count += 1;'
    print '#endif'
    print '   switch(format) {'
    for format in formats:
        if is_format_supported(format):
            print '   case %s:' % format.name
            func_name = 'lp_tile_%s_swizzle_%s' % (format.short_name(), dst_suffix)
            if format.name == 'PIPE_FORMAT_B8G8R8A8_UNORM' or format.name == 'PIPE_FORMAT_B8G8R8X8_UNORM':
                print '#ifdef PIPE_ARCH_SSE'
                print '      func = util_cpu_caps.has_sse2 ? %s_sse2 : %s;' % (func_name, func_name)
                print '#else'
                print '      func = %s;' % (func_name,)
                print '#endif'
            else:
                print '      func = %s;' % (func_name,)
            print '      break;'
    print '   default:'
    print '      debug_printf("%s: unsupported format %s\\n", __FUNCTION__, util_format_name(format));'
    print '      return;'
    print '   }'
    print '   func(dst, (const uint8_t *)src, src_stride, x, y);'
    print '}'
    print


def generate_unswizzle(formats, src_channel, src_native_type, src_suffix):
    '''Generate the dispatch function to write pixels to any format'''

    for format in formats:
        if is_format_supported(format):
            generate_format_write(format, src_channel, src_native_type, src_suffix)

    print 'void'
    print 'lp_tile_unswizzle_%s(enum pipe_format format, const %s *src, void *dst, unsigned dst_stride, unsigned x, unsigned y)' % (src_suffix, src_native_type)
    
    print '{'
    print '   void (*func)(const %s * restrict src, uint8_t * restrict dst, unsigned dst_stride, unsigned x0, unsigned y0);' % src_native_type
    print '#ifdef DEBUG'
    print '   lp_tile_unswizzle_count += 1;'
    print '#endif'
    print '   switch(format) {'
    for format in formats:
        if is_format_supported(format):
            print '   case %s:' % format.name
            func_name = 'lp_tile_%s_unswizzle_%s' % (format.short_name(), src_suffix)
            if format.name == 'PIPE_FORMAT_B8G8R8A8_UNORM' or format.name == 'PIPE_FORMAT_B8G8R8X8_UNORM':
                print '#ifdef PIPE_ARCH_SSE'
                print '      func = util_cpu_caps.has_sse2 ? %s_sse2 : %s;' % (func_name, func_name)
                print '#else'
                print '      func = %s;' % (func_name,)
                print '#endif'
            else:
                print '      func = %s;' % (func_name,)
            print '      break;'
    print '   default:'
    print '      debug_printf("%s: unsupported format %s\\n", __FUNCTION__, util_format_name(format));'
    print '      return;'
    print '   }'
    print '   func(src, (uint8_t *)dst, dst_stride, x, y);'
    print '}'
    print


def main():
    formats = []
    for arg in sys.argv[1:]:
        formats.extend(parse(arg))

    print '/* This file is autogenerated by lp_tile_soa.py from u_format.csv. Do not edit directly. */'
    print
    # This will print the copyright message on the top of this file
    print CopyRight.strip()
    print
    print '#include "pipe/p_compiler.h"'
    print '#include "util/u_math.h"'
    print '#include "util/u_format.h"'
    print '#include "util/u_format_r11g11b10f.h"'
    print '#include "util/u_format_rgb9e5.h"'
    print '#include "util/u_half.h"'
    print '#include "util/u_cpu_detect.h"'
    print '#include "lp_tile_soa.h"'
    print
    print '#ifdef DEBUG'
    print 'unsigned lp_tile_unswizzle_count = 0;'
    print 'unsigned lp_tile_swizzle_count = 0;'
    print '#endif'
    print
    print 'const unsigned char'
    print 'tile_offset[TILE_VECTOR_HEIGHT][TILE_VECTOR_WIDTH] = {'
    print '   {  0,  1,  4,  5},'
    print '   {  2,  3,  6,  7},'
    print '   {  8,  9, 12, 13},'
    print '   { 10, 11, 14, 15}'
    print '};'
    print
    print '/* Note: these lookup tables could be replaced with some'
    print ' * bit-twiddling code, but this is a little faster.'
    print ' */'
    print 'static unsigned tile_x_offset[TILE_VECTOR_WIDTH * TILE_VECTOR_HEIGHT] = {'
    print '   0, 1, 0, 1, 2, 3, 2, 3,'
    print '   0, 1, 0, 1, 2, 3, 2, 3'
    print '};'
    print
    print 'static unsigned tile_y_offset[TILE_VECTOR_WIDTH * TILE_VECTOR_HEIGHT] = {'
    print '   0, 0, 1, 1, 0, 0, 1, 1,'
    print '   2, 2, 3, 3, 2, 2, 3, 3'
    print '};'
    print

    generate_sse2()

    channel = Channel(UNSIGNED, True, False, 8)
    native_type = 'uint8_t'
    suffix = '4ub'

    generate_swizzle(formats, channel, native_type, suffix)
    generate_unswizzle(formats, channel, native_type, suffix)


if __name__ == '__main__':
    main()
