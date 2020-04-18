/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
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

#ifndef SVGA_SURFACE_H
#define SVGA_SURFACE_H


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "svga_screen_cache.h"

struct pipe_context;
struct pipe_screen;
struct svga_context;
struct svga_texture;
struct svga_winsys_surface;
enum SVGA3dSurfaceFormat;


struct svga_surface
{
   struct pipe_surface base;

   struct svga_host_surface_cache_key key;
   struct svga_winsys_surface *handle;

   unsigned real_face;
   unsigned real_level;
   unsigned real_zslice;

   boolean dirty;
};


extern void
svga_propagate_surface(struct svga_context *svga, struct pipe_surface *surf);

extern boolean
svga_surface_needs_propagation(const struct pipe_surface *surf);

struct svga_winsys_surface *
svga_texture_view_surface(struct svga_context *svga,
                          struct svga_texture *tex,
                          SVGA3dSurfaceFlags flags,
                          SVGA3dSurfaceFormat format,
                          unsigned start_mip,
                          unsigned num_mip,
                          int face_pick,
                          int zslice_pick,
                          struct svga_host_surface_cache_key *key); /* OUT */


void
svga_texture_copy_handle(struct svga_context *svga,
                         struct svga_winsys_surface *src_handle,
                         unsigned src_x, unsigned src_y, unsigned src_z,
                         unsigned src_level, unsigned src_face,
                         struct svga_winsys_surface *dst_handle,
                         unsigned dst_x, unsigned dst_y, unsigned dst_z,
                         unsigned dst_level, unsigned dst_face,
                         unsigned width, unsigned height, unsigned depth);


static INLINE struct svga_surface *
svga_surface(struct pipe_surface *surface)
{
   assert(surface);
   return (struct svga_surface *)surface;
}


static INLINE const struct svga_surface *
svga_surface_const(const struct pipe_surface *surface)
{
   assert(surface);
   return (const struct svga_surface *)surface;
}

#endif
