/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "i915_surface.h"
#include "i915_resource.h"
#include "i915_state.h"
#include "i915_blit.h"
#include "i915_reg.h"
#include "i915_screen.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_pack_color.h"
#include "util/u_surface.h"

/*
 * surface functions using the render engine
 */

static void
i915_surface_copy_render(struct pipe_context *pipe,
                         struct pipe_resource *dst, unsigned dst_level,
                         unsigned dstx, unsigned dsty, unsigned dstz,
                         struct pipe_resource *src, unsigned src_level,
                         const struct pipe_box *src_box)
{
   struct i915_context *i915 = i915_context(pipe);

   /* Fallback for buffers. */
   if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
      util_resource_copy_region(pipe, dst, dst_level, dstx, dsty, dstz,
                                src, src_level, src_box);
      return;
   }

   if (!util_blitter_is_copy_supported(i915->blitter, dst, src,
                                       PIPE_MASK_RGBAZS)) {
      util_resource_copy_region(pipe, dst, dst_level, dstx, dsty, dstz,
                                src, src_level, src_box);
      return;
   }

   util_blitter_save_blend(i915->blitter, (void *)i915->blend);
   util_blitter_save_depth_stencil_alpha(i915->blitter, (void *)i915->depth_stencil);
   util_blitter_save_stencil_ref(i915->blitter, &i915->stencil_ref);
   util_blitter_save_rasterizer(i915->blitter, (void *)i915->rasterizer);
   util_blitter_save_fragment_shader(i915->blitter, i915->saved_fs);
   util_blitter_save_vertex_shader(i915->blitter, i915->saved_vs);
   util_blitter_save_viewport(i915->blitter, &i915->viewport);
   util_blitter_save_vertex_elements(i915->blitter, i915->saved_velems);
   util_blitter_save_vertex_buffers(i915->blitter, i915->saved_nr_vertex_buffers,
                                    i915->saved_vertex_buffers);

   util_blitter_save_framebuffer(i915->blitter, &i915->framebuffer);

   util_blitter_save_fragment_sampler_states(i915->blitter,
                                             i915->saved_nr_samplers,
                                             i915->saved_samplers);
   util_blitter_save_fragment_sampler_views(i915->blitter,
                                            i915->saved_nr_sampler_views,
                                            i915->saved_sampler_views);

   util_blitter_copy_texture(i915->blitter, dst, dst_level, ~0, dstx, dsty, dstz,
                            src, src_level, 0, src_box);
}

static void
i915_clear_render_target_render(struct pipe_context *pipe,
                                struct pipe_surface *dst,
                                const union pipe_color_union *color,
                                unsigned dstx, unsigned dsty,
                                unsigned width, unsigned height)
{
   struct i915_context *i915 = i915_context(pipe);
   struct pipe_framebuffer_state fb_state;

   util_blitter_save_framebuffer(i915->blitter, &i915->framebuffer);

   fb_state.width = dst->width;
   fb_state.height = dst->height;
   fb_state.nr_cbufs = 1;
   fb_state.cbufs[0] = dst;
   fb_state.zsbuf = NULL;
   pipe->set_framebuffer_state(pipe, &fb_state);

   if (i915->dirty)
      i915_update_derived(i915);

   i915_clear_emit(pipe, PIPE_CLEAR_COLOR, color, 0.0, 0x0,
                   dstx, dsty, width, height);

   pipe->set_framebuffer_state(pipe, &i915->blitter->saved_fb_state);
   util_unreference_framebuffer_state(&i915->blitter->saved_fb_state);
   i915->blitter->saved_fb_state.nr_cbufs = ~0;
}

static void
i915_clear_depth_stencil_render(struct pipe_context *pipe,
                                struct pipe_surface *dst,
                                unsigned clear_flags,
                                double depth,
                                unsigned stencil,
                                unsigned dstx, unsigned dsty,
                                unsigned width, unsigned height)
{
   struct i915_context *i915 = i915_context(pipe);
   struct pipe_framebuffer_state fb_state;

   util_blitter_save_framebuffer(i915->blitter, &i915->framebuffer);

   fb_state.width = dst->width;
   fb_state.height = dst->height;
   fb_state.nr_cbufs = 0;
   fb_state.zsbuf = dst;
   pipe->set_framebuffer_state(pipe, &fb_state);

   if (i915->dirty)
      i915_update_derived(i915);

   i915_clear_emit(pipe, clear_flags & PIPE_CLEAR_DEPTHSTENCIL,
                   NULL, depth, stencil,
                   dstx, dsty, width, height);

   pipe->set_framebuffer_state(pipe, &i915->blitter->saved_fb_state);
   util_unreference_framebuffer_state(&i915->blitter->saved_fb_state);
   i915->blitter->saved_fb_state.nr_cbufs = ~0;
}

/*
 * surface functions using the blitter
 */

/* Assumes all values are within bounds -- no checking at this level -
 * do it higher up if required.
 */
static void
i915_surface_copy_blitter(struct pipe_context *pipe,
                          struct pipe_resource *dst, unsigned dst_level,
                          unsigned dstx, unsigned dsty, unsigned dstz,
                          struct pipe_resource *src, unsigned src_level,
                          const struct pipe_box *src_box)
{
   struct i915_texture *dst_tex = i915_texture(dst);
   struct i915_texture *src_tex = i915_texture(src);
   struct pipe_resource *dpt = &dst_tex->b.b;
   struct pipe_resource *spt = &src_tex->b.b;
   unsigned dst_offset, src_offset;  /* in bytes */

   /* Fallback for buffers. */
   if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
      util_resource_copy_region(pipe, dst, dst_level, dstx, dsty, dstz,
                                src, src_level, src_box);
      return;
   }

   /* XXX cannot copy 3d regions at this time */
   assert(src_box->depth == 1);
   if (dst->target != PIPE_TEXTURE_CUBE &&
       dst->target != PIPE_TEXTURE_3D)
      assert(dstz == 0);
   dst_offset = i915_texture_offset(dst_tex, dst_level, dstz);

   if (src->target != PIPE_TEXTURE_CUBE &&
       src->target != PIPE_TEXTURE_3D)
      assert(src_box->z == 0);
   src_offset = i915_texture_offset(src_tex, src_level, src_box->z);

   assert( util_format_get_blocksize(dpt->format) == util_format_get_blocksize(spt->format) );
   assert( util_format_get_blockwidth(dpt->format) == util_format_get_blockwidth(spt->format) );
   assert( util_format_get_blockheight(dpt->format) == util_format_get_blockheight(spt->format) );
   assert( util_format_get_blockwidth(dpt->format) == 1 );
   assert( util_format_get_blockheight(dpt->format) == 1 );

   i915_copy_blit( i915_context(pipe),
                   util_format_get_blocksize(dpt->format),
                   (unsigned short) src_tex->stride, src_tex->buffer, src_offset,
                   (unsigned short) dst_tex->stride, dst_tex->buffer, dst_offset,
                   (short) src_box->x, (short) src_box->y, (short) dstx, (short) dsty,
                   (short) src_box->width, (short) src_box->height );
}

static void
i915_clear_render_target_blitter(struct pipe_context *pipe,
                                 struct pipe_surface *dst,
                                 const union pipe_color_union *color,
                                 unsigned dstx, unsigned dsty,
                                 unsigned width, unsigned height)
{
   struct i915_texture *tex = i915_texture(dst->texture);
   struct pipe_resource *pt = &tex->b.b;
   union util_color uc;
   unsigned offset = i915_texture_offset(tex, dst->u.tex.level, dst->u.tex.first_layer);

   assert(util_format_get_blockwidth(pt->format) == 1);
   assert(util_format_get_blockheight(pt->format) == 1);

   util_pack_color(color->f, dst->format, &uc);
   i915_fill_blit( i915_context(pipe),
                   util_format_get_blocksize(pt->format),
                   XY_COLOR_BLT_WRITE_ALPHA | XY_COLOR_BLT_WRITE_RGB,
                   (unsigned short) tex->stride,
                   tex->buffer, offset,
                   (short) dstx, (short) dsty,
                   (short) width, (short) height,
                   uc.ui );
}

static void
i915_clear_depth_stencil_blitter(struct pipe_context *pipe,
                                 struct pipe_surface *dst,
                                 unsigned clear_flags,
                                 double depth,
                                 unsigned stencil,
                                 unsigned dstx, unsigned dsty,
                                 unsigned width, unsigned height)
{
   struct i915_texture *tex = i915_texture(dst->texture);
   struct pipe_resource *pt = &tex->b.b;
   unsigned packedds;
   unsigned mask = 0;
   unsigned offset = i915_texture_offset(tex, dst->u.tex.level, dst->u.tex.first_layer);

   assert(util_format_get_blockwidth(pt->format) == 1);
   assert(util_format_get_blockheight(pt->format) == 1);

   packedds = util_pack_z_stencil(dst->format, depth, stencil);

   if (clear_flags & PIPE_CLEAR_DEPTH)
      mask |= XY_COLOR_BLT_WRITE_RGB;
   /* XXX presumably this does read-modify-write
      (otherwise this won't work anyway). Hence will only want to
      do it if really have stencil and it isn't cleared */
   if ((clear_flags & PIPE_CLEAR_STENCIL) ||
       (dst->format != PIPE_FORMAT_Z24_UNORM_S8_UINT))
      mask |= XY_COLOR_BLT_WRITE_ALPHA;

   i915_fill_blit( i915_context(pipe),
                   util_format_get_blocksize(pt->format),
                   mask,
                   (unsigned short) tex->stride,
                   tex->buffer, offset,
                   (short) dstx, (short) dsty,
                   (short) width, (short) height,
                   packedds );
}

/*
 * Screen surface functions
 */


static struct pipe_surface *
i915_create_surface(struct pipe_context *ctx,
                    struct pipe_resource *pt,
                    const struct pipe_surface *surf_tmpl)
{
   struct pipe_surface *ps;

   assert(surf_tmpl->u.tex.first_layer == surf_tmpl->u.tex.last_layer);
   if (pt->target != PIPE_TEXTURE_CUBE &&
       pt->target != PIPE_TEXTURE_3D)
      assert(surf_tmpl->u.tex.first_layer == 0);

   ps = CALLOC_STRUCT(pipe_surface);
   if (ps) {
      /* could subclass pipe_surface and store offset as it used to do */
      pipe_reference_init(&ps->reference, 1);
      pipe_resource_reference(&ps->texture, pt);
      ps->format = surf_tmpl->format;
      ps->width = u_minify(pt->width0, surf_tmpl->u.tex.level);
      ps->height = u_minify(pt->height0, surf_tmpl->u.tex.level);
      ps->u.tex.level = surf_tmpl->u.tex.level;
      ps->u.tex.first_layer = surf_tmpl->u.tex.first_layer;
      ps->u.tex.last_layer = surf_tmpl->u.tex.last_layer;
      ps->usage = surf_tmpl->usage;
      ps->context = ctx;
   }
   return ps;
}

static void
i915_surface_destroy(struct pipe_context *ctx,
                     struct pipe_surface *surf)
{
   pipe_resource_reference(&surf->texture, NULL);
   FREE(surf);
}


void
i915_init_surface_functions(struct i915_context *i915)
{
   if (i915_screen(i915->base.screen)->debug.use_blitter) {
      i915->base.resource_copy_region = i915_surface_copy_blitter;
      i915->base.clear_render_target = i915_clear_render_target_blitter;
      i915->base.clear_depth_stencil = i915_clear_depth_stencil_blitter;
   } else {
      i915->base.resource_copy_region = i915_surface_copy_render;
      i915->base.clear_render_target = i915_clear_render_target_render;
      i915->base.clear_depth_stencil = i915_clear_depth_stencil_render;
   }
   i915->base.create_surface = i915_create_surface;
   i915->base.surface_destroy = i915_surface_destroy;
}
