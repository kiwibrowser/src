/**************************************************************************
 *
 * Copyright 2011 Christian König.
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

#ifndef vl_video_buffer_h
#define vl_video_buffer_h

#include "pipe/p_context.h"
#include "pipe/p_video_decoder.h"

#include "vl_defines.h"

/**
 * implementation of a planar ycbcr buffer
 */

/* planar buffer for vl data upload and manipulation */
struct vl_video_buffer
{
   struct pipe_video_buffer base;
   unsigned                 num_planes;
   struct pipe_resource     *resources[VL_NUM_COMPONENTS];
   struct pipe_sampler_view *sampler_view_planes[VL_NUM_COMPONENTS];
   struct pipe_sampler_view *sampler_view_components[VL_NUM_COMPONENTS];
   struct pipe_surface      *surfaces[VL_MAX_SURFACES];
};

/**
 * get subformats for each plane
 */
const enum pipe_format *
vl_video_buffer_formats(struct pipe_screen *screen, enum pipe_format format);

/**
 * get YUV plane order
 */
const unsigned *
vl_video_buffer_plane_order(enum pipe_format format);

/**
 * get maximum size of video buffers
 */
unsigned
vl_video_buffer_max_size(struct pipe_screen *screen);

/**
 * check if video buffer format is supported for a codec/profile
 * can be used as default implementation of screen->is_video_format_supported
 */
boolean
vl_video_buffer_is_format_supported(struct pipe_screen *screen,
                                    enum pipe_format format,
                                    enum pipe_video_profile profile);

/*
 * set the associated data for the given video buffer
 */
void
vl_video_buffer_set_associated_data(struct pipe_video_buffer *vbuf,
                                    struct pipe_video_decoder *vdec,
                                    void *associated_data,
                                    void (*destroy_associated_data)(void *));

/*
 * get the associated data for the given video buffer
 */
void *
vl_video_buffer_get_associated_data(struct pipe_video_buffer *vbuf,
                                    struct pipe_video_decoder *vdec);

/**
 * fill a resource template for the given plane
 */
void
vl_vide_buffer_template(struct pipe_resource *templ,
			const struct pipe_video_buffer *templat,
                        enum pipe_format resource_format,
                        unsigned depth, unsigned usage, unsigned plane);

/**
 * creates a video buffer, can be used as a standard implementation for pipe->create_video_buffer
 */
struct pipe_video_buffer *
vl_video_buffer_create(struct pipe_context *pipe,
                       const struct pipe_video_buffer *templat);

/**
 * extended create function, gets depth, usage and formats for each plane seperately
 */
struct pipe_video_buffer *
vl_video_buffer_create_ex(struct pipe_context *pipe,
                          const struct pipe_video_buffer *templat,
                          const enum pipe_format resource_formats[VL_NUM_COMPONENTS],
                          unsigned depth, unsigned usage);

/**
 * even more extended create function, provide the pipe_resource for each plane
 */
struct pipe_video_buffer *
vl_video_buffer_create_ex2(struct pipe_context *pipe,
                           const struct pipe_video_buffer *templat,
                           struct pipe_resource *resources[VL_NUM_COMPONENTS]);

#endif /* vl_video_buffer_h */
