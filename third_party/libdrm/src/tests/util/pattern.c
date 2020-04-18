/*
 * Copyright 2008 Tungsten Graphics
 *   Jakob Bornecrantz <jakob@tungstengraphics.com>
 * Copyright 2008 Intel Corporation
 *   Jesse Barnes <jesse.barnes@intel.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <drm_fourcc.h>

#ifdef HAVE_CAIRO
#include <cairo.h>
#include <math.h>
#endif

#include "format.h"
#include "pattern.h"

struct color_rgb24 {
	unsigned int value:24;
} __attribute__((__packed__));

struct color_yuv {
	unsigned char y;
	unsigned char u;
	unsigned char v;
};

#define MAKE_YUV_601_Y(r, g, b) \
	((( 66 * (r) + 129 * (g) +  25 * (b) + 128) >> 8) + 16)
#define MAKE_YUV_601_U(r, g, b) \
	(((-38 * (r) -  74 * (g) + 112 * (b) + 128) >> 8) + 128)
#define MAKE_YUV_601_V(r, g, b) \
	(((112 * (r) -  94 * (g) -  18 * (b) + 128) >> 8) + 128)

#define MAKE_YUV_601(r, g, b) \
	{ .y = MAKE_YUV_601_Y(r, g, b), \
	  .u = MAKE_YUV_601_U(r, g, b), \
	  .v = MAKE_YUV_601_V(r, g, b) }

#define MAKE_RGBA(rgb, r, g, b, a) \
	((((r) >> (8 - (rgb)->red.length)) << (rgb)->red.offset) | \
	 (((g) >> (8 - (rgb)->green.length)) << (rgb)->green.offset) | \
	 (((b) >> (8 - (rgb)->blue.length)) << (rgb)->blue.offset) | \
	 (((a) >> (8 - (rgb)->alpha.length)) << (rgb)->alpha.offset))

#define MAKE_RGB24(rgb, r, g, b) \
	{ .value = MAKE_RGBA(rgb, r, g, b, 0) }

static void fill_smpte_yuv_planar(const struct util_yuv_info *yuv,
				  unsigned char *y_mem, unsigned char *u_mem,
				  unsigned char *v_mem, unsigned int width,
				  unsigned int height, unsigned int stride)
{
	const struct color_yuv colors_top[] = {
		MAKE_YUV_601(191, 192, 192),	/* grey */
		MAKE_YUV_601(192, 192, 0),	/* yellow */
		MAKE_YUV_601(0, 192, 192),	/* cyan */
		MAKE_YUV_601(0, 192, 0),	/* green */
		MAKE_YUV_601(192, 0, 192),	/* magenta */
		MAKE_YUV_601(192, 0, 0),	/* red */
		MAKE_YUV_601(0, 0, 192),	/* blue */
	};
	const struct color_yuv colors_middle[] = {
		MAKE_YUV_601(0, 0, 192),	/* blue */
		MAKE_YUV_601(19, 19, 19),	/* black */
		MAKE_YUV_601(192, 0, 192),	/* magenta */
		MAKE_YUV_601(19, 19, 19),	/* black */
		MAKE_YUV_601(0, 192, 192),	/* cyan */
		MAKE_YUV_601(19, 19, 19),	/* black */
		MAKE_YUV_601(192, 192, 192),	/* grey */
	};
	const struct color_yuv colors_bottom[] = {
		MAKE_YUV_601(0, 33, 76),	/* in-phase */
		MAKE_YUV_601(255, 255, 255),	/* super white */
		MAKE_YUV_601(50, 0, 106),	/* quadrature */
		MAKE_YUV_601(19, 19, 19),	/* black */
		MAKE_YUV_601(9, 9, 9),		/* 3.5% */
		MAKE_YUV_601(19, 19, 19),	/* 7.5% */
		MAKE_YUV_601(29, 29, 29),	/* 11.5% */
		MAKE_YUV_601(19, 19, 19),	/* black */
	};
	unsigned int cs = yuv->chroma_stride;
	unsigned int xsub = yuv->xsub;
	unsigned int ysub = yuv->ysub;
	unsigned int x;
	unsigned int y;

	/* Luma */
	for (y = 0; y < height * 6 / 9; ++y) {
		for (x = 0; x < width; ++x)
			y_mem[x] = colors_top[x * 7 / width].y;
		y_mem += stride;
	}

	for (; y < height * 7 / 9; ++y) {
		for (x = 0; x < width; ++x)
			y_mem[x] = colors_middle[x * 7 / width].y;
		y_mem += stride;
	}

	for (; y < height; ++y) {
		for (x = 0; x < width * 5 / 7; ++x)
			y_mem[x] = colors_bottom[x * 4 / (width * 5 / 7)].y;
		for (; x < width * 6 / 7; ++x)
			y_mem[x] = colors_bottom[(x - width * 5 / 7) * 3
						 / (width / 7) + 4].y;
		for (; x < width; ++x)
			y_mem[x] = colors_bottom[7].y;
		y_mem += stride;
	}

	/* Chroma */
	for (y = 0; y < height / ysub * 6 / 9; ++y) {
		for (x = 0; x < width; x += xsub) {
			u_mem[x*cs/xsub] = colors_top[x * 7 / width].u;
			v_mem[x*cs/xsub] = colors_top[x * 7 / width].v;
		}
		u_mem += stride * cs / xsub;
		v_mem += stride * cs / xsub;
	}

	for (; y < height / ysub * 7 / 9; ++y) {
		for (x = 0; x < width; x += xsub) {
			u_mem[x*cs/xsub] = colors_middle[x * 7 / width].u;
			v_mem[x*cs/xsub] = colors_middle[x * 7 / width].v;
		}
		u_mem += stride * cs / xsub;
		v_mem += stride * cs / xsub;
	}

	for (; y < height / ysub; ++y) {
		for (x = 0; x < width * 5 / 7; x += xsub) {
			u_mem[x*cs/xsub] =
				colors_bottom[x * 4 / (width * 5 / 7)].u;
			v_mem[x*cs/xsub] =
				colors_bottom[x * 4 / (width * 5 / 7)].v;
		}
		for (; x < width * 6 / 7; x += xsub) {
			u_mem[x*cs/xsub] = colors_bottom[(x - width * 5 / 7) *
							 3 / (width / 7) + 4].u;
			v_mem[x*cs/xsub] = colors_bottom[(x - width * 5 / 7) *
							 3 / (width / 7) + 4].v;
		}
		for (; x < width; x += xsub) {
			u_mem[x*cs/xsub] = colors_bottom[7].u;
			v_mem[x*cs/xsub] = colors_bottom[7].v;
		}
		u_mem += stride * cs / xsub;
		v_mem += stride * cs / xsub;
	}
}

static void fill_smpte_yuv_packed(const struct util_yuv_info *yuv, void *mem,
				  unsigned int width, unsigned int height,
				  unsigned int stride)
{
	const struct color_yuv colors_top[] = {
		MAKE_YUV_601(191, 192, 192),	/* grey */
		MAKE_YUV_601(192, 192, 0),	/* yellow */
		MAKE_YUV_601(0, 192, 192),	/* cyan */
		MAKE_YUV_601(0, 192, 0),	/* green */
		MAKE_YUV_601(192, 0, 192),	/* magenta */
		MAKE_YUV_601(192, 0, 0),	/* red */
		MAKE_YUV_601(0, 0, 192),	/* blue */
	};
	const struct color_yuv colors_middle[] = {
		MAKE_YUV_601(0, 0, 192),	/* blue */
		MAKE_YUV_601(19, 19, 19),	/* black */
		MAKE_YUV_601(192, 0, 192),	/* magenta */
		MAKE_YUV_601(19, 19, 19),	/* black */
		MAKE_YUV_601(0, 192, 192),	/* cyan */
		MAKE_YUV_601(19, 19, 19),	/* black */
		MAKE_YUV_601(192, 192, 192),	/* grey */
	};
	const struct color_yuv colors_bottom[] = {
		MAKE_YUV_601(0, 33, 76),	/* in-phase */
		MAKE_YUV_601(255, 255, 255),	/* super white */
		MAKE_YUV_601(50, 0, 106),	/* quadrature */
		MAKE_YUV_601(19, 19, 19),	/* black */
		MAKE_YUV_601(9, 9, 9),		/* 3.5% */
		MAKE_YUV_601(19, 19, 19),	/* 7.5% */
		MAKE_YUV_601(29, 29, 29),	/* 11.5% */
		MAKE_YUV_601(19, 19, 19),	/* black */
	};
	unsigned char *y_mem = (yuv->order & YUV_YC) ? mem : mem + 1;
	unsigned char *c_mem = (yuv->order & YUV_CY) ? mem : mem + 1;
	unsigned int u = (yuv->order & YUV_YCrCb) ? 2 : 0;
	unsigned int v = (yuv->order & YUV_YCbCr) ? 2 : 0;
	unsigned int x;
	unsigned int y;

	/* Luma */
	for (y = 0; y < height * 6 / 9; ++y) {
		for (x = 0; x < width; ++x)
			y_mem[2*x] = colors_top[x * 7 / width].y;
		y_mem += stride;
	}

	for (; y < height * 7 / 9; ++y) {
		for (x = 0; x < width; ++x)
			y_mem[2*x] = colors_middle[x * 7 / width].y;
		y_mem += stride;
	}

	for (; y < height; ++y) {
		for (x = 0; x < width * 5 / 7; ++x)
			y_mem[2*x] = colors_bottom[x * 4 / (width * 5 / 7)].y;
		for (; x < width * 6 / 7; ++x)
			y_mem[2*x] = colors_bottom[(x - width * 5 / 7) * 3
						   / (width / 7) + 4].y;
		for (; x < width; ++x)
			y_mem[2*x] = colors_bottom[7].y;
		y_mem += stride;
	}

	/* Chroma */
	for (y = 0; y < height * 6 / 9; ++y) {
		for (x = 0; x < width; x += 2) {
			c_mem[2*x+u] = colors_top[x * 7 / width].u;
			c_mem[2*x+v] = colors_top[x * 7 / width].v;
		}
		c_mem += stride;
	}

	for (; y < height * 7 / 9; ++y) {
		for (x = 0; x < width; x += 2) {
			c_mem[2*x+u] = colors_middle[x * 7 / width].u;
			c_mem[2*x+v] = colors_middle[x * 7 / width].v;
		}
		c_mem += stride;
	}

	for (; y < height; ++y) {
		for (x = 0; x < width * 5 / 7; x += 2) {
			c_mem[2*x+u] = colors_bottom[x * 4 / (width * 5 / 7)].u;
			c_mem[2*x+v] = colors_bottom[x * 4 / (width * 5 / 7)].v;
		}
		for (; x < width * 6 / 7; x += 2) {
			c_mem[2*x+u] = colors_bottom[(x - width * 5 / 7) *
						     3 / (width / 7) + 4].u;
			c_mem[2*x+v] = colors_bottom[(x - width * 5 / 7) *
						     3 / (width / 7) + 4].v;
		}
		for (; x < width; x += 2) {
			c_mem[2*x+u] = colors_bottom[7].u;
			c_mem[2*x+v] = colors_bottom[7].v;
		}
		c_mem += stride;
	}
}

static void fill_smpte_rgb16(const struct util_rgb_info *rgb, void *mem,
			     unsigned int width, unsigned int height,
			     unsigned int stride)
{
	const uint16_t colors_top[] = {
		MAKE_RGBA(rgb, 192, 192, 192, 255),	/* grey */
		MAKE_RGBA(rgb, 192, 192, 0, 255),	/* yellow */
		MAKE_RGBA(rgb, 0, 192, 192, 255),	/* cyan */
		MAKE_RGBA(rgb, 0, 192, 0, 255),		/* green */
		MAKE_RGBA(rgb, 192, 0, 192, 255),	/* magenta */
		MAKE_RGBA(rgb, 192, 0, 0, 255),		/* red */
		MAKE_RGBA(rgb, 0, 0, 192, 255),		/* blue */
	};
	const uint16_t colors_middle[] = {
		MAKE_RGBA(rgb, 0, 0, 192, 127),		/* blue */
		MAKE_RGBA(rgb, 19, 19, 19, 127),	/* black */
		MAKE_RGBA(rgb, 192, 0, 192, 127),	/* magenta */
		MAKE_RGBA(rgb, 19, 19, 19, 127),	/* black */
		MAKE_RGBA(rgb, 0, 192, 192, 127),	/* cyan */
		MAKE_RGBA(rgb, 19, 19, 19, 127),	/* black */
		MAKE_RGBA(rgb, 192, 192, 192, 127),	/* grey */
	};
	const uint16_t colors_bottom[] = {
		MAKE_RGBA(rgb, 0, 33, 76, 255),		/* in-phase */
		MAKE_RGBA(rgb, 255, 255, 255, 255),	/* super white */
		MAKE_RGBA(rgb, 50, 0, 106, 255),	/* quadrature */
		MAKE_RGBA(rgb, 19, 19, 19, 255),	/* black */
		MAKE_RGBA(rgb, 9, 9, 9, 255),		/* 3.5% */
		MAKE_RGBA(rgb, 19, 19, 19, 255),	/* 7.5% */
		MAKE_RGBA(rgb, 29, 29, 29, 255),	/* 11.5% */
		MAKE_RGBA(rgb, 19, 19, 19, 255),	/* black */
	};
	unsigned int x;
	unsigned int y;

	for (y = 0; y < height * 6 / 9; ++y) {
		for (x = 0; x < width; ++x)
			((uint16_t *)mem)[x] = colors_top[x * 7 / width];
		mem += stride;
	}

	for (; y < height * 7 / 9; ++y) {
		for (x = 0; x < width; ++x)
			((uint16_t *)mem)[x] = colors_middle[x * 7 / width];
		mem += stride;
	}

	for (; y < height; ++y) {
		for (x = 0; x < width * 5 / 7; ++x)
			((uint16_t *)mem)[x] =
				colors_bottom[x * 4 / (width * 5 / 7)];
		for (; x < width * 6 / 7; ++x)
			((uint16_t *)mem)[x] =
				colors_bottom[(x - width * 5 / 7) * 3
					      / (width / 7) + 4];
		for (; x < width; ++x)
			((uint16_t *)mem)[x] = colors_bottom[7];
		mem += stride;
	}
}

static void fill_smpte_rgb24(const struct util_rgb_info *rgb, void *mem,
			     unsigned int width, unsigned int height,
			     unsigned int stride)
{
	const struct color_rgb24 colors_top[] = {
		MAKE_RGB24(rgb, 192, 192, 192),	/* grey */
		MAKE_RGB24(rgb, 192, 192, 0),	/* yellow */
		MAKE_RGB24(rgb, 0, 192, 192),	/* cyan */
		MAKE_RGB24(rgb, 0, 192, 0),	/* green */
		MAKE_RGB24(rgb, 192, 0, 192),	/* magenta */
		MAKE_RGB24(rgb, 192, 0, 0),	/* red */
		MAKE_RGB24(rgb, 0, 0, 192),	/* blue */
	};
	const struct color_rgb24 colors_middle[] = {
		MAKE_RGB24(rgb, 0, 0, 192),	/* blue */
		MAKE_RGB24(rgb, 19, 19, 19),	/* black */
		MAKE_RGB24(rgb, 192, 0, 192),	/* magenta */
		MAKE_RGB24(rgb, 19, 19, 19),	/* black */
		MAKE_RGB24(rgb, 0, 192, 192),	/* cyan */
		MAKE_RGB24(rgb, 19, 19, 19),	/* black */
		MAKE_RGB24(rgb, 192, 192, 192),	/* grey */
	};
	const struct color_rgb24 colors_bottom[] = {
		MAKE_RGB24(rgb, 0, 33, 76),	/* in-phase */
		MAKE_RGB24(rgb, 255, 255, 255),	/* super white */
		MAKE_RGB24(rgb, 50, 0, 106),	/* quadrature */
		MAKE_RGB24(rgb, 19, 19, 19),	/* black */
		MAKE_RGB24(rgb, 9, 9, 9),	/* 3.5% */
		MAKE_RGB24(rgb, 19, 19, 19),	/* 7.5% */
		MAKE_RGB24(rgb, 29, 29, 29),	/* 11.5% */
		MAKE_RGB24(rgb, 19, 19, 19),	/* black */
	};
	unsigned int x;
	unsigned int y;

	for (y = 0; y < height * 6 / 9; ++y) {
		for (x = 0; x < width; ++x)
			((struct color_rgb24 *)mem)[x] =
				colors_top[x * 7 / width];
		mem += stride;
	}

	for (; y < height * 7 / 9; ++y) {
		for (x = 0; x < width; ++x)
			((struct color_rgb24 *)mem)[x] =
				colors_middle[x * 7 / width];
		mem += stride;
	}

	for (; y < height; ++y) {
		for (x = 0; x < width * 5 / 7; ++x)
			((struct color_rgb24 *)mem)[x] =
				colors_bottom[x * 4 / (width * 5 / 7)];
		for (; x < width * 6 / 7; ++x)
			((struct color_rgb24 *)mem)[x] =
				colors_bottom[(x - width * 5 / 7) * 3
					      / (width / 7) + 4];
		for (; x < width; ++x)
			((struct color_rgb24 *)mem)[x] = colors_bottom[7];
		mem += stride;
	}
}

static void fill_smpte_rgb32(const struct util_rgb_info *rgb, void *mem,
			     unsigned int width, unsigned int height,
			     unsigned int stride)
{
	const uint32_t colors_top[] = {
		MAKE_RGBA(rgb, 192, 192, 192, 255),	/* grey */
		MAKE_RGBA(rgb, 192, 192, 0, 255),	/* yellow */
		MAKE_RGBA(rgb, 0, 192, 192, 255),	/* cyan */
		MAKE_RGBA(rgb, 0, 192, 0, 255),		/* green */
		MAKE_RGBA(rgb, 192, 0, 192, 255),	/* magenta */
		MAKE_RGBA(rgb, 192, 0, 0, 255),		/* red */
		MAKE_RGBA(rgb, 0, 0, 192, 255),		/* blue */
	};
	const uint32_t colors_middle[] = {
		MAKE_RGBA(rgb, 0, 0, 192, 127),		/* blue */
		MAKE_RGBA(rgb, 19, 19, 19, 127),	/* black */
		MAKE_RGBA(rgb, 192, 0, 192, 127),	/* magenta */
		MAKE_RGBA(rgb, 19, 19, 19, 127),	/* black */
		MAKE_RGBA(rgb, 0, 192, 192, 127),	/* cyan */
		MAKE_RGBA(rgb, 19, 19, 19, 127),	/* black */
		MAKE_RGBA(rgb, 192, 192, 192, 127),	/* grey */
	};
	const uint32_t colors_bottom[] = {
		MAKE_RGBA(rgb, 0, 33, 76, 255),		/* in-phase */
		MAKE_RGBA(rgb, 255, 255, 255, 255),	/* super white */
		MAKE_RGBA(rgb, 50, 0, 106, 255),	/* quadrature */
		MAKE_RGBA(rgb, 19, 19, 19, 255),	/* black */
		MAKE_RGBA(rgb, 9, 9, 9, 255),		/* 3.5% */
		MAKE_RGBA(rgb, 19, 19, 19, 255),	/* 7.5% */
		MAKE_RGBA(rgb, 29, 29, 29, 255),	/* 11.5% */
		MAKE_RGBA(rgb, 19, 19, 19, 255),	/* black */
	};
	unsigned int x;
	unsigned int y;

	for (y = 0; y < height * 6 / 9; ++y) {
		for (x = 0; x < width; ++x)
			((uint32_t *)mem)[x] = colors_top[x * 7 / width];
		mem += stride;
	}

	for (; y < height * 7 / 9; ++y) {
		for (x = 0; x < width; ++x)
			((uint32_t *)mem)[x] = colors_middle[x * 7 / width];
		mem += stride;
	}

	for (; y < height; ++y) {
		for (x = 0; x < width * 5 / 7; ++x)
			((uint32_t *)mem)[x] =
				colors_bottom[x * 4 / (width * 5 / 7)];
		for (; x < width * 6 / 7; ++x)
			((uint32_t *)mem)[x] =
				colors_bottom[(x - width * 5 / 7) * 3
					      / (width / 7) + 4];
		for (; x < width; ++x)
			((uint32_t *)mem)[x] = colors_bottom[7];
		mem += stride;
	}
}

static void fill_smpte(const struct util_format_info *info, void *planes[3],
		       unsigned int width, unsigned int height,
		       unsigned int stride)
{
	unsigned char *u, *v;

	switch (info->format) {
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_VYUY:
	case DRM_FORMAT_YUYV:
	case DRM_FORMAT_YVYU:
		return fill_smpte_yuv_packed(&info->yuv, planes[0], width,
					     height, stride);

	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV16:
	case DRM_FORMAT_NV61:
		u = info->yuv.order & YUV_YCbCr ? planes[1] : planes[1] + 1;
		v = info->yuv.order & YUV_YCrCb ? planes[1] : planes[1] + 1;
		return fill_smpte_yuv_planar(&info->yuv, planes[0], u, v,
					     width, height, stride);

	case DRM_FORMAT_YUV420:
		return fill_smpte_yuv_planar(&info->yuv, planes[0], planes[1],
					     planes[2], width, height, stride);

	case DRM_FORMAT_YVU420:
		return fill_smpte_yuv_planar(&info->yuv, planes[0], planes[2],
					     planes[1], width, height, stride);

	case DRM_FORMAT_ARGB4444:
	case DRM_FORMAT_XRGB4444:
	case DRM_FORMAT_ABGR4444:
	case DRM_FORMAT_XBGR4444:
	case DRM_FORMAT_RGBA4444:
	case DRM_FORMAT_RGBX4444:
	case DRM_FORMAT_BGRA4444:
	case DRM_FORMAT_BGRX4444:
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_BGR565:
	case DRM_FORMAT_ARGB1555:
	case DRM_FORMAT_XRGB1555:
	case DRM_FORMAT_ABGR1555:
	case DRM_FORMAT_XBGR1555:
	case DRM_FORMAT_RGBA5551:
	case DRM_FORMAT_RGBX5551:
	case DRM_FORMAT_BGRA5551:
	case DRM_FORMAT_BGRX5551:
		return fill_smpte_rgb16(&info->rgb, planes[0],
					width, height, stride);

	case DRM_FORMAT_BGR888:
	case DRM_FORMAT_RGB888:
		return fill_smpte_rgb24(&info->rgb, planes[0],
					width, height, stride);
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_RGBX8888:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_BGRX8888:
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_XRGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_XBGR2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_RGBX1010102:
	case DRM_FORMAT_BGRA1010102:
	case DRM_FORMAT_BGRX1010102:
		return fill_smpte_rgb32(&info->rgb, planes[0],
					width, height, stride);
	}
}

/* swap these for big endian.. */
#define RED   2
#define GREEN 1
#define BLUE  0

static void make_pwetty(void *data, unsigned int width, unsigned int height,
			unsigned int stride, uint32_t format)
{
#ifdef HAVE_CAIRO
	cairo_surface_t *surface;
	cairo_t *cr;
	int x, y;
	cairo_format_t cairo_format;

	/* we can ignore the order of R,G,B channels */
	switch (format) {
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_ABGR8888:
		cairo_format = CAIRO_FORMAT_ARGB32;
		break;
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_BGR565:
		cairo_format = CAIRO_FORMAT_RGB16_565;
		break;
	default:
		return;
	}

	surface = cairo_image_surface_create_for_data(data,
						      cairo_format,
						      width, height,
						      stride);
	cr = cairo_create(surface);
	cairo_surface_destroy(surface);

	cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
	for (x = 0; x < width; x += 250)
		for (y = 0; y < height; y += 250) {
			char buf[64];

			cairo_move_to(cr, x, y - 20);
			cairo_line_to(cr, x, y + 20);
			cairo_move_to(cr, x - 20, y);
			cairo_line_to(cr, x + 20, y);
			cairo_new_sub_path(cr);
			cairo_arc(cr, x, y, 10, 0, M_PI * 2);
			cairo_set_line_width(cr, 4);
			cairo_set_source_rgb(cr, 0, 0, 0);
			cairo_stroke_preserve(cr);
			cairo_set_source_rgb(cr, 1, 1, 1);
			cairo_set_line_width(cr, 2);
			cairo_stroke(cr);

			snprintf(buf, sizeof buf, "%d, %d", x, y);
			cairo_move_to(cr, x + 20, y + 20);
			cairo_text_path(cr, buf);
			cairo_set_source_rgb(cr, 0, 0, 0);
			cairo_stroke_preserve(cr);
			cairo_set_source_rgb(cr, 1, 1, 1);
			cairo_fill(cr);
		}

	cairo_destroy(cr);
#endif
}

static void fill_tiles_yuv_planar(const struct util_format_info *info,
				  unsigned char *y_mem, unsigned char *u_mem,
				  unsigned char *v_mem, unsigned int width,
				  unsigned int height, unsigned int stride)
{
	const struct util_yuv_info *yuv = &info->yuv;
	unsigned int cs = yuv->chroma_stride;
	unsigned int xsub = yuv->xsub;
	unsigned int ysub = yuv->ysub;
	unsigned int x;
	unsigned int y;

	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			div_t d = div(x+y, width);
			uint32_t rgb32 = 0x00130502 * (d.quot >> 6)
				       + 0x000a1120 * (d.rem >> 6);
			struct color_yuv color =
				MAKE_YUV_601((rgb32 >> 16) & 0xff,
					     (rgb32 >> 8) & 0xff, rgb32 & 0xff);

			y_mem[x] = color.y;
			u_mem[x/xsub*cs] = color.u;
			v_mem[x/xsub*cs] = color.v;
		}

		y_mem += stride;
		if ((y + 1) % ysub == 0) {
			u_mem += stride * cs / xsub;
			v_mem += stride * cs / xsub;
		}
	}
}

static void fill_tiles_yuv_packed(const struct util_format_info *info,
				  void *mem, unsigned int width,
				  unsigned int height, unsigned int stride)
{
	const struct util_yuv_info *yuv = &info->yuv;
	unsigned char *y_mem = (yuv->order & YUV_YC) ? mem : mem + 1;
	unsigned char *c_mem = (yuv->order & YUV_CY) ? mem : mem + 1;
	unsigned int u = (yuv->order & YUV_YCrCb) ? 2 : 0;
	unsigned int v = (yuv->order & YUV_YCbCr) ? 2 : 0;
	unsigned int x;
	unsigned int y;

	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; x += 2) {
			div_t d = div(x+y, width);
			uint32_t rgb32 = 0x00130502 * (d.quot >> 6)
				       + 0x000a1120 * (d.rem >> 6);
			struct color_yuv color =
				MAKE_YUV_601((rgb32 >> 16) & 0xff,
					     (rgb32 >> 8) & 0xff, rgb32 & 0xff);

			y_mem[2*x] = color.y;
			c_mem[2*x+u] = color.u;
			y_mem[2*x+2] = color.y;
			c_mem[2*x+v] = color.v;
		}

		y_mem += stride;
		c_mem += stride;
	}
}

static void fill_tiles_rgb16(const struct util_format_info *info, void *mem,
			     unsigned int width, unsigned int height,
			     unsigned int stride)
{
	const struct util_rgb_info *rgb = &info->rgb;
	void *mem_base = mem;
	unsigned int x, y;

	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			div_t d = div(x+y, width);
			uint32_t rgb32 = 0x00130502 * (d.quot >> 6)
				       + 0x000a1120 * (d.rem >> 6);
			uint16_t color =
				MAKE_RGBA(rgb, (rgb32 >> 16) & 0xff,
					  (rgb32 >> 8) & 0xff, rgb32 & 0xff,
					  255);

			((uint16_t *)mem)[x] = color;
		}
		mem += stride;
	}

	make_pwetty(mem_base, width, height, stride, info->format);
}

static void fill_tiles_rgb24(const struct util_format_info *info, void *mem,
			     unsigned int width, unsigned int height,
			     unsigned int stride)
{
	const struct util_rgb_info *rgb = &info->rgb;
	unsigned int x, y;

	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			div_t d = div(x+y, width);
			uint32_t rgb32 = 0x00130502 * (d.quot >> 6)
				       + 0x000a1120 * (d.rem >> 6);
			struct color_rgb24 color =
				MAKE_RGB24(rgb, (rgb32 >> 16) & 0xff,
					   (rgb32 >> 8) & 0xff, rgb32 & 0xff);

			((struct color_rgb24 *)mem)[x] = color;
		}
		mem += stride;
	}
}

static void fill_tiles_rgb32(const struct util_format_info *info, void *mem,
			     unsigned int width, unsigned int height,
			     unsigned int stride)
{
	const struct util_rgb_info *rgb = &info->rgb;
	void *mem_base = mem;
	unsigned int x, y;

	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			div_t d = div(x+y, width);
			uint32_t rgb32 = 0x00130502 * (d.quot >> 6)
				       + 0x000a1120 * (d.rem >> 6);
			uint32_t alpha = ((y < height/2) && (x < width/2)) ? 127 : 255;
			uint32_t color =
				MAKE_RGBA(rgb, (rgb32 >> 16) & 0xff,
					  (rgb32 >> 8) & 0xff, rgb32 & 0xff,
					  alpha);

			((uint32_t *)mem)[x] = color;
		}
		mem += stride;
	}

	make_pwetty(mem_base, width, height, stride, info->format);
}

static void fill_tiles(const struct util_format_info *info, void *planes[3],
		       unsigned int width, unsigned int height,
		       unsigned int stride)
{
	unsigned char *u, *v;

	switch (info->format) {
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_VYUY:
	case DRM_FORMAT_YUYV:
	case DRM_FORMAT_YVYU:
		return fill_tiles_yuv_packed(info, planes[0],
					     width, height, stride);

	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV16:
	case DRM_FORMAT_NV61:
		u = info->yuv.order & YUV_YCbCr ? planes[1] : planes[1] + 1;
		v = info->yuv.order & YUV_YCrCb ? planes[1] : planes[1] + 1;
		return fill_tiles_yuv_planar(info, planes[0], u, v,
					     width, height, stride);

	case DRM_FORMAT_YUV420:
		return fill_tiles_yuv_planar(info, planes[0], planes[1],
					     planes[2], width, height, stride);

	case DRM_FORMAT_YVU420:
		return fill_tiles_yuv_planar(info, planes[0], planes[2],
					     planes[1], width, height, stride);

	case DRM_FORMAT_ARGB4444:
	case DRM_FORMAT_XRGB4444:
	case DRM_FORMAT_ABGR4444:
	case DRM_FORMAT_XBGR4444:
	case DRM_FORMAT_RGBA4444:
	case DRM_FORMAT_RGBX4444:
	case DRM_FORMAT_BGRA4444:
	case DRM_FORMAT_BGRX4444:
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_BGR565:
	case DRM_FORMAT_ARGB1555:
	case DRM_FORMAT_XRGB1555:
	case DRM_FORMAT_ABGR1555:
	case DRM_FORMAT_XBGR1555:
	case DRM_FORMAT_RGBA5551:
	case DRM_FORMAT_RGBX5551:
	case DRM_FORMAT_BGRA5551:
	case DRM_FORMAT_BGRX5551:
		return fill_tiles_rgb16(info, planes[0],
					width, height, stride);

	case DRM_FORMAT_BGR888:
	case DRM_FORMAT_RGB888:
		return fill_tiles_rgb24(info, planes[0],
					width, height, stride);
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_RGBX8888:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_BGRX8888:
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_XRGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_XBGR2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_RGBX1010102:
	case DRM_FORMAT_BGRA1010102:
	case DRM_FORMAT_BGRX1010102:
		return fill_tiles_rgb32(info, planes[0],
					width, height, stride);
	}
}

static void fill_plain(const struct util_format_info *info, void *planes[3],
		       unsigned int width, unsigned int height,
		       unsigned int stride)
{
	memset(planes[0], 0x77, stride * height);
}

/*
 * util_fill_pattern - Fill a buffer with a test pattern
 * @format: Pixel format
 * @pattern: Test pattern
 * @planes: Array of buffers
 * @width: Width in pixels
 * @height: Height in pixels
 * @stride: Line stride (pitch) in bytes
 *
 * Fill the buffers with the test pattern specified by the pattern parameter.
 * Supported formats vary depending on the selected pattern.
 */
void util_fill_pattern(uint32_t format, enum util_fill_pattern pattern,
		       void *planes[3], unsigned int width,
		       unsigned int height, unsigned int stride)
{
	const struct util_format_info *info;

	info = util_format_info_find(format);
	if (info == NULL)
		return;

	switch (pattern) {
	case UTIL_PATTERN_TILES:
		return fill_tiles(info, planes, width, height, stride);

	case UTIL_PATTERN_SMPTE:
		return fill_smpte(info, planes, width, height, stride);

	case UTIL_PATTERN_PLAIN:
		return fill_plain(info, planes, width, height, stride);

	default:
		printf("Error: unsupported test pattern %u.\n", pattern);
		break;
	}
}
