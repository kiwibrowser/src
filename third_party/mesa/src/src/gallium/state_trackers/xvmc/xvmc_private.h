/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

#ifndef xvmc_private_h
#define xvmc_private_h

#include <X11/Xlib.h>
#include <X11/extensions/XvMClib.h>

#include "pipe/p_video_state.h"

#include "util/u_debug.h"
#include "util/u_math.h"

#include "vl/vl_csc.h"
#include "vl/vl_compositor.h"

#define BLOCK_SIZE_SAMPLES 64
#define BLOCK_SIZE_BYTES (BLOCK_SIZE_SAMPLES * 2)

struct pipe_video_decoder;
struct pipe_video_buffer;

struct pipe_sampler_view;
struct pipe_fence_handle;

typedef struct
{
   struct vl_screen *vscreen;
   struct pipe_context *pipe;
   struct pipe_video_decoder *decoder;

   enum VL_CSC_COLOR_STANDARD color_standard;
   struct vl_procamp procamp;
   struct vl_compositor compositor;
   struct vl_compositor_state cstate;

   unsigned short subpicture_max_width;
   unsigned short subpicture_max_height;

} XvMCContextPrivate;

typedef struct
{
   struct pipe_video_buffer *video_buffer;

   /* nonzero if this picture is already being decoded */
   int picture_structure;

   XvMCSurface *ref[2];

   struct pipe_fence_handle *fence;

   /* The subpicture associated with this surface, if any. */
   XvMCSubpicture *subpicture;

   /* Some XvMC functions take a surface but not a context,
      so we keep track of which context each surface belongs to. */
   XvMCContext *context;
} XvMCSurfacePrivate;

typedef struct
{
   struct pipe_sampler_view *sampler;

   /* optional palette for this subpicture */
   struct pipe_sampler_view *palette;

   struct u_rect src_rect;
   struct u_rect dst_rect;

   /* The surface this subpicture is currently associated with, if any. */
   XvMCSurface *surface;

   /* Some XvMC functions take a subpicture but not a context,
      so we keep track of which context each subpicture belongs to. */
   XvMCContext *context;
} XvMCSubpicturePrivate;

#define XVMC_OUT   0
#define XVMC_ERR   1
#define XVMC_WARN  2
#define XVMC_TRACE 3

static INLINE void XVMC_MSG(unsigned int level, const char *fmt, ...)
{
   static int debug_level = -1;

   if (debug_level == -1) {
      debug_level = MAX2(debug_get_num_option("XVMC_DEBUG", 0), 0);
   }

   if (level <= debug_level) {
      va_list ap;
      va_start(ap, fmt);
      _debug_vprintf(fmt, ap);
      va_end(ap);
   }
}

#endif /* xvmc_private_h */
