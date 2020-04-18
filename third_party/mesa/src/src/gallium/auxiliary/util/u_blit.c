/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/**
 * @file
 * Copy/blit pixel rect between surfaces
 *  
 * @author Brian Paul
 */


#include "pipe/p_context.h"
#include "util/u_debug.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_state.h"

#include "util/u_blit.h"
#include "util/u_draw_quad.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_sampler.h"
#include "util/u_simple_shaders.h"

#include "cso_cache/cso_context.h"


struct blit_state
{
   struct pipe_context *pipe;
   struct cso_context *cso;

   struct pipe_blend_state blend_write_color, blend_keep_color;
   struct pipe_depth_stencil_alpha_state dsa_keep_depthstencil;
   struct pipe_depth_stencil_alpha_state dsa_write_depthstencil;
   struct pipe_depth_stencil_alpha_state dsa_write_depth;
   struct pipe_depth_stencil_alpha_state dsa_write_stencil;
   struct pipe_rasterizer_state rasterizer;
   struct pipe_sampler_state sampler;
   struct pipe_viewport_state viewport;
   struct pipe_vertex_element velem[2];
   enum pipe_texture_target internal_target;

   void *vs;
   void *fs[PIPE_MAX_TEXTURE_TYPES][TGSI_WRITEMASK_XYZW + 1];
   void *fs_depthstencil[PIPE_MAX_TEXTURE_TYPES];
   void *fs_depth[PIPE_MAX_TEXTURE_TYPES];
   void *fs_stencil[PIPE_MAX_TEXTURE_TYPES];

   struct pipe_resource *vbuf;  /**< quad vertices */
   unsigned vbuf_slot;

   float vertices[4][2][4];   /**< vertex/texcoords for quad */

   boolean has_stencil_export;
};


/**
 * Create state object for blit.
 * Intended to be created once and re-used for many blit() calls.
 */
struct blit_state *
util_create_blit(struct pipe_context *pipe, struct cso_context *cso)
{
   struct blit_state *ctx;
   uint i;

   ctx = CALLOC_STRUCT(blit_state);
   if (!ctx)
      return NULL;

   ctx->pipe = pipe;
   ctx->cso = cso;

   /* disabled blending/masking */
   ctx->blend_write_color.rt[0].colormask = PIPE_MASK_RGBA;

   /* depth stencil states */
   ctx->dsa_write_depth.depth.enabled = 1;
   ctx->dsa_write_depth.depth.writemask = 1;
   ctx->dsa_write_depth.depth.func = PIPE_FUNC_ALWAYS;
   ctx->dsa_write_stencil.stencil[0].enabled = 1;
   ctx->dsa_write_stencil.stencil[0].func = PIPE_FUNC_ALWAYS;
   ctx->dsa_write_stencil.stencil[0].fail_op = PIPE_STENCIL_OP_REPLACE;
   ctx->dsa_write_stencil.stencil[0].zpass_op = PIPE_STENCIL_OP_REPLACE;
   ctx->dsa_write_stencil.stencil[0].zfail_op = PIPE_STENCIL_OP_REPLACE;
   ctx->dsa_write_stencil.stencil[0].valuemask = 0xff;
   ctx->dsa_write_stencil.stencil[0].writemask = 0xff;
   ctx->dsa_write_depthstencil.depth = ctx->dsa_write_depth.depth;
   ctx->dsa_write_depthstencil.stencil[0] = ctx->dsa_write_stencil.stencil[0];

   /* rasterizer */
   ctx->rasterizer.cull_face = PIPE_FACE_NONE;
   ctx->rasterizer.gl_rasterization_rules = 1;
   ctx->rasterizer.depth_clip = 1;

   /* samplers */
   ctx->sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   ctx->sampler.min_img_filter = 0; /* set later */
   ctx->sampler.mag_img_filter = 0; /* set later */

   /* vertex elements state */
   for (i = 0; i < 2; i++) {
      ctx->velem[i].src_offset = i * 4 * sizeof(float);
      ctx->velem[i].instance_divisor = 0;
      ctx->velem[i].vertex_buffer_index = 0;
      ctx->velem[i].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   }

   ctx->vbuf = NULL;

   /* init vertex data that doesn't change */
   for (i = 0; i < 4; i++) {
      ctx->vertices[i][0][3] = 1.0f; /* w */
      ctx->vertices[i][1][2] = 0.0f; /* r */
      ctx->vertices[i][1][3] = 1.0f; /* q */
   }

   if(pipe->screen->get_param(pipe->screen, PIPE_CAP_NPOT_TEXTURES))
      ctx->internal_target = PIPE_TEXTURE_2D;
   else
      ctx->internal_target = PIPE_TEXTURE_RECT;

   ctx->has_stencil_export =
      pipe->screen->get_param(pipe->screen, PIPE_CAP_SHADER_STENCIL_EXPORT);

   return ctx;
}


/**
 * Destroy a blit context
 */
void
util_destroy_blit(struct blit_state *ctx)
{
   struct pipe_context *pipe = ctx->pipe;
   unsigned i, j;

   if (ctx->vs)
      pipe->delete_vs_state(pipe, ctx->vs);

   for (i = 0; i < Elements(ctx->fs); i++) {
      for (j = 0; j < Elements(ctx->fs[i]); j++) {
         if (ctx->fs[i][j])
            pipe->delete_fs_state(pipe, ctx->fs[i][j]);
      }
   }

   for (i = 0; i < PIPE_MAX_TEXTURE_TYPES; i++) {
      if (ctx->fs_depthstencil[i]) {
         pipe->delete_fs_state(pipe, ctx->fs_depthstencil[i]);
      }
      if (ctx->fs_depth[i]) {
         pipe->delete_fs_state(pipe, ctx->fs_depth[i]);
      }
      if (ctx->fs_stencil[i]) {
         pipe->delete_fs_state(pipe, ctx->fs_stencil[i]);
      }
   }

   pipe_resource_reference(&ctx->vbuf, NULL);

   FREE(ctx);
}


/**
 * Helper function to set the fragment shaders.
 */
static INLINE void
set_fragment_shader(struct blit_state *ctx, uint writemask,
                    enum pipe_texture_target pipe_tex)
{
   if (!ctx->fs[pipe_tex][writemask]) {
      unsigned tgsi_tex = util_pipe_tex_to_tgsi_tex(pipe_tex, 0);

      ctx->fs[pipe_tex][writemask] =
         util_make_fragment_tex_shader_writemask(ctx->pipe, tgsi_tex,
                                                 TGSI_INTERPOLATE_LINEAR,
                                                 writemask);
   }

   cso_set_fragment_shader_handle(ctx->cso, ctx->fs[pipe_tex][writemask]);
}


/**
 * Helper function to set the shader which writes depth and stencil.
 */
static INLINE void
set_depthstencil_fragment_shader(struct blit_state *ctx,
                                 enum pipe_texture_target pipe_tex)
{
   if (!ctx->fs_depthstencil[pipe_tex]) {
      unsigned tgsi_tex = util_pipe_tex_to_tgsi_tex(pipe_tex, 0);

      ctx->fs_depthstencil[pipe_tex] =
         util_make_fragment_tex_shader_writedepthstencil(ctx->pipe, tgsi_tex,
                                                  TGSI_INTERPOLATE_LINEAR);
   }

   cso_set_fragment_shader_handle(ctx->cso, ctx->fs_depthstencil[pipe_tex]);
}


/**
 * Helper function to set the shader which writes depth.
 */
static INLINE void
set_depth_fragment_shader(struct blit_state *ctx,
                          enum pipe_texture_target pipe_tex)
{
   if (!ctx->fs_depth[pipe_tex]) {
      unsigned tgsi_tex = util_pipe_tex_to_tgsi_tex(pipe_tex, 0);

      ctx->fs_depth[pipe_tex] =
         util_make_fragment_tex_shader_writedepth(ctx->pipe, tgsi_tex,
                                                  TGSI_INTERPOLATE_LINEAR);
   }

   cso_set_fragment_shader_handle(ctx->cso, ctx->fs_depth[pipe_tex]);
}


/**
 * Helper function to set the shader which writes stencil.
 */
static INLINE void
set_stencil_fragment_shader(struct blit_state *ctx,
                            enum pipe_texture_target pipe_tex)
{
   if (!ctx->fs_stencil[pipe_tex]) {
      unsigned tgsi_tex = util_pipe_tex_to_tgsi_tex(pipe_tex, 0);

      ctx->fs_stencil[pipe_tex] =
         util_make_fragment_tex_shader_writestencil(ctx->pipe, tgsi_tex,
                                                    TGSI_INTERPOLATE_LINEAR);
   }

   cso_set_fragment_shader_handle(ctx->cso, ctx->fs_stencil[pipe_tex]);
}


/**
 * Helper function to set the vertex shader.
 */
static INLINE void
set_vertex_shader(struct blit_state *ctx)
{
   /* vertex shader - still required to provide the linkage between
    * fragment shader input semantics and vertex_element/buffers.
    */
   if (!ctx->vs) {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_GENERIC };
      const uint semantic_indexes[] = { 0, 0 };
      ctx->vs = util_make_vertex_passthrough_shader(ctx->pipe, 2,
                                                    semantic_names,
                                                    semantic_indexes);
   }

   cso_set_vertex_shader_handle(ctx->cso, ctx->vs);
}


/**
 * Get offset of next free slot in vertex buffer for quad vertices.
 */
static unsigned
get_next_slot( struct blit_state *ctx )
{
   const unsigned max_slots = 4096 / sizeof ctx->vertices;

   if (ctx->vbuf_slot >= max_slots) {
      pipe_resource_reference(&ctx->vbuf, NULL);
      ctx->vbuf_slot = 0;
   }

   if (!ctx->vbuf) {
      ctx->vbuf = pipe_buffer_create(ctx->pipe->screen,
                                     PIPE_BIND_VERTEX_BUFFER,
                                     PIPE_USAGE_STREAM,
                                     max_slots * sizeof ctx->vertices);
   }
   
   return ctx->vbuf_slot++ * sizeof ctx->vertices;
}




/**
 * Setup vertex data for the textured quad we'll draw.
 * Note: y=0=top
 */
static unsigned
setup_vertex_data_tex(struct blit_state *ctx,
                      float x0, float y0, float x1, float y1,
                      float s0, float t0, float s1, float t1,
                      float z)
{
   unsigned offset;

   ctx->vertices[0][0][0] = x0;
   ctx->vertices[0][0][1] = y0;
   ctx->vertices[0][0][2] = z;
   ctx->vertices[0][1][0] = s0; /*s*/
   ctx->vertices[0][1][1] = t0; /*t*/

   ctx->vertices[1][0][0] = x1;
   ctx->vertices[1][0][1] = y0;
   ctx->vertices[1][0][2] = z;
   ctx->vertices[1][1][0] = s1; /*s*/
   ctx->vertices[1][1][1] = t0; /*t*/

   ctx->vertices[2][0][0] = x1;
   ctx->vertices[2][0][1] = y1;
   ctx->vertices[2][0][2] = z;
   ctx->vertices[2][1][0] = s1;
   ctx->vertices[2][1][1] = t1;

   ctx->vertices[3][0][0] = x0;
   ctx->vertices[3][0][1] = y1;
   ctx->vertices[3][0][2] = z;
   ctx->vertices[3][1][0] = s0;
   ctx->vertices[3][1][1] = t1;

   offset = get_next_slot( ctx );

   if (ctx->vbuf) {
      pipe_buffer_write_nooverlap(ctx->pipe, ctx->vbuf,
                                  offset, sizeof(ctx->vertices), ctx->vertices);
   }

   return offset;
}


/**
 * \return TRUE if two regions overlap, FALSE otherwise
 */
static boolean
regions_overlap(int srcX0, int srcY0,
                int srcX1, int srcY1,
                int dstX0, int dstY0,
                int dstX1, int dstY1)
{
   if (MAX2(srcX0, srcX1) < MIN2(dstX0, dstX1))
      return FALSE; /* src completely left of dst */

   if (MAX2(dstX0, dstX1) < MIN2(srcX0, srcX1))
      return FALSE; /* dst completely left of src */

   if (MAX2(srcY0, srcY1) < MIN2(dstY0, dstY1))
      return FALSE; /* src completely above dst */

   if (MAX2(dstY0, dstY1) < MIN2(srcY0, srcY1))
      return FALSE; /* dst completely above src */

   return TRUE; /* some overlap */
}


/**
 * Can we blit from src format to dest format with a simple copy?
 */
static boolean
formats_compatible(enum pipe_format src_format,
                   enum pipe_format dst_format)
{
   if (src_format == dst_format) {
      return TRUE;
   }
   else {
      const struct util_format_description *src_desc =
         util_format_description(src_format);
      const struct util_format_description *dst_desc =
         util_format_description(dst_format);
      return util_is_format_compatible(src_desc, dst_desc);
   }
}


/**
 * Copy pixel block from src surface to dst surface.
 * Overlapping regions are acceptable.
 * Flipping and stretching are supported.
 * \param filter  one of PIPE_TEX_MIPFILTER_NEAREST/LINEAR
 * \param writemask  controls which channels in the dest surface are sourced
 *                   from the src surface.  Disabled channels are sourced
 *                   from (0,0,0,1).
 */
void
util_blit_pixels(struct blit_state *ctx,
                 struct pipe_resource *src_tex,
                 unsigned src_level,
                 int srcX0, int srcY0,
                 int srcX1, int srcY1,
                 int srcZ0,
                 struct pipe_surface *dst,
                 int dstX0, int dstY0,
                 int dstX1, int dstY1,
                 float z, uint filter,
                 uint writemask, uint zs_writemask)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   enum pipe_format src_format, dst_format;
   struct pipe_sampler_view *sampler_view = NULL;
   struct pipe_sampler_view sv_templ;
   struct pipe_surface *dst_surface;
   struct pipe_framebuffer_state fb;
   const int srcW = abs(srcX1 - srcX0);
   const int srcH = abs(srcY1 - srcY0);
   unsigned offset;
   boolean overlap;
   float s0, t0, s1, t1;
   boolean normalized;
   boolean is_stencil, is_depth, blit_depth, blit_stencil;
   const struct util_format_description *src_desc =
         util_format_description(src_tex->format);

   assert(filter == PIPE_TEX_MIPFILTER_NEAREST ||
          filter == PIPE_TEX_MIPFILTER_LINEAR);

   assert(src_level <= src_tex->last_level);

   /* do the regions overlap? */
   overlap = src_tex == dst->texture &&
             dst->u.tex.level == src_level &&
             dst->u.tex.first_layer == srcZ0 &&
      regions_overlap(srcX0, srcY0, srcX1, srcY1,
                      dstX0, dstY0, dstX1, dstY1);

   src_format = util_format_linear(src_tex->format);
   dst_format = util_format_linear(dst->texture->format);

   /* See whether we will blit depth or stencil. */
   is_depth = util_format_has_depth(src_desc);
   is_stencil = util_format_has_stencil(src_desc);

   blit_depth = is_depth && (zs_writemask & BLIT_WRITEMASK_Z);
   blit_stencil = is_stencil && (zs_writemask & BLIT_WRITEMASK_STENCIL);

   assert((writemask && !zs_writemask && !is_depth && !is_stencil) ||
          (!writemask && (blit_depth || blit_stencil)));

   /*
    * Check for simple case:  no format conversion, no flipping, no stretching,
    * no overlapping, same number of samples.
    * Filter mode should not matter since there's no stretching.
    */
   if (formats_compatible(src_format, dst_format) &&
       src_tex->nr_samples == dst->texture->nr_samples &&
       is_stencil == blit_stencil &&
       is_depth == blit_depth &&
       srcX0 < srcX1 &&
       dstX0 < dstX1 &&
       srcY0 < srcY1 &&
       dstY0 < dstY1 &&
       (dstX1 - dstX0) == (srcX1 - srcX0) &&
       (dstY1 - dstY0) == (srcY1 - srcY0) &&
       !overlap) {
      struct pipe_box src_box;
      src_box.x = srcX0;
      src_box.y = srcY0;
      src_box.z = srcZ0;
      src_box.width = srcW;
      src_box.height = srcH;
      src_box.depth = 1;
      pipe->resource_copy_region(pipe,
                                 dst->texture, dst->u.tex.level,
                                 dstX0, dstY0, dst->u.tex.first_layer,/* dest */
                                 src_tex, src_level,
                                 &src_box);
      return;
   }

   /* XXX Reading multisample textures is unimplemented. */
   assert(src_tex->nr_samples <= 1);
   if (src_tex->nr_samples > 1) {
      return;
   }

   /* It's a mistake to call this function with a stencil format and
    * without shader stencil export. We don't do software fallbacks here.
    * Ignore stencil and only copy depth.
    */
   if (blit_stencil && !ctx->has_stencil_export) {
      blit_stencil = FALSE;

      if (!blit_depth)
         return;
   }

   if (dst_format == dst->format) {
      dst_surface = dst;
   } else {
      struct pipe_surface templ = *dst;
      templ.format = dst_format;
      dst_surface = pipe->create_surface(pipe, dst->texture, &templ);
   }

   /* Create a temporary texture when src and dest alias.
    */
   if (src_tex == dst_surface->texture &&
       dst_surface->u.tex.level == src_level &&
       dst_surface->u.tex.first_layer == srcZ0) {
      /* Make a temporary texture which contains a copy of the source pixels.
       * Then we'll sample from the temporary texture.
       */
      struct pipe_resource texTemp;
      struct pipe_resource *tex;
      struct pipe_sampler_view sv_templ;
      struct pipe_box src_box;
      const int srcLeft = MIN2(srcX0, srcX1);
      const int srcTop = MIN2(srcY0, srcY1);

      if (srcLeft != srcX0) {
         /* left-right flip */
         int tmp = dstX0;
         dstX0 = dstX1;
         dstX1 = tmp;
      }

      if (srcTop != srcY0) {
         /* up-down flip */
         int tmp = dstY0;
         dstY0 = dstY1;
         dstY1 = tmp;
      }

      /* create temp texture */
      memset(&texTemp, 0, sizeof(texTemp));
      texTemp.target = ctx->internal_target;
      texTemp.format = src_format;
      texTemp.last_level = 0;
      texTemp.width0 = srcW;
      texTemp.height0 = srcH;
      texTemp.depth0 = 1;
      texTemp.array_size = 1;
      texTemp.bind = PIPE_BIND_SAMPLER_VIEW;

      tex = screen->resource_create(screen, &texTemp);
      if (!tex)
         return;

      src_box.x = srcLeft;
      src_box.y = srcTop;
      src_box.z = srcZ0;
      src_box.width = srcW;
      src_box.height = srcH;
      src_box.depth = 1;
      /* load temp texture */
      pipe->resource_copy_region(pipe,
                                 tex, 0, 0, 0, 0,  /* dest */
                                 src_tex, src_level, &src_box);

      normalized = tex->target != PIPE_TEXTURE_RECT;
      if(normalized) {
         s0 = 0.0f;
         s1 = 1.0f;
         t0 = 0.0f;
         t1 = 1.0f;
      }
      else {
         s0 = 0;
         s1 = srcW;
         t0 = 0;
         t1 = srcH;
      }

      u_sampler_view_default_template(&sv_templ, tex, tex->format);
      if (!blit_depth && blit_stencil) {
         /* set a stencil-only format, e.g. Z24S8 --> X24S8 */
         sv_templ.format = util_format_stencil_only(tex->format);
         assert(sv_templ.format != PIPE_FORMAT_NONE);
      }
      sampler_view = pipe->create_sampler_view(pipe, tex, &sv_templ);

      if (!sampler_view) {
         pipe_resource_reference(&tex, NULL);
         return;
      }
      pipe_resource_reference(&tex, NULL);
   }
   else {
      /* Directly sample from the source resource/texture */
      u_sampler_view_default_template(&sv_templ, src_tex, src_format);
      if (!blit_depth && blit_stencil) {
         /* set a stencil-only format, e.g. Z24S8 --> X24S8 */
         sv_templ.format = util_format_stencil_only(src_format);
         assert(sv_templ.format != PIPE_FORMAT_NONE);
      }
      sampler_view = pipe->create_sampler_view(pipe, src_tex, &sv_templ);

      if (!sampler_view) {
         return;
      }

      s0 = srcX0;
      s1 = srcX1;
      t0 = srcY0;
      t1 = srcY1;
      normalized = sampler_view->texture->target != PIPE_TEXTURE_RECT;
      if(normalized)
      {
         s0 /= (float)(u_minify(sampler_view->texture->width0, src_level));
         s1 /= (float)(u_minify(sampler_view->texture->width0, src_level));
         t0 /= (float)(u_minify(sampler_view->texture->height0, src_level));
         t1 /= (float)(u_minify(sampler_view->texture->height0, src_level));
      }
   }

   assert(screen->is_format_supported(screen, sampler_view->format,
                     ctx->internal_target, sampler_view->texture->nr_samples,
                     PIPE_BIND_SAMPLER_VIEW));
   assert(screen->is_format_supported(screen, dst_format, ctx->internal_target,
                     dst_surface->texture->nr_samples,
                     is_depth || is_stencil ? PIPE_BIND_DEPTH_STENCIL :
                                              PIPE_BIND_RENDER_TARGET));

   /* save state (restored below) */
   cso_save_blend(ctx->cso);
   cso_save_depth_stencil_alpha(ctx->cso);
   cso_save_rasterizer(ctx->cso);
   cso_save_sample_mask(ctx->cso);
   cso_save_samplers(ctx->cso, PIPE_SHADER_FRAGMENT);
   cso_save_sampler_views(ctx->cso, PIPE_SHADER_FRAGMENT);
   cso_save_stream_outputs(ctx->cso);
   cso_save_viewport(ctx->cso);
   cso_save_framebuffer(ctx->cso);
   cso_save_fragment_shader(ctx->cso);
   cso_save_vertex_shader(ctx->cso);
   cso_save_geometry_shader(ctx->cso);
   cso_save_vertex_elements(ctx->cso);
   cso_save_vertex_buffers(ctx->cso);

   /* set misc state we care about */
   if (writemask)
      cso_set_blend(ctx->cso, &ctx->blend_write_color);
   else
      cso_set_blend(ctx->cso, &ctx->blend_keep_color);

   cso_set_sample_mask(ctx->cso, ~0);
   cso_set_rasterizer(ctx->cso, &ctx->rasterizer);
   cso_set_vertex_elements(ctx->cso, 2, ctx->velem);
   cso_set_stream_outputs(ctx->cso, 0, NULL, 0);

   /* default sampler state */
   ctx->sampler.normalized_coords = normalized;
   ctx->sampler.min_img_filter = filter;
   ctx->sampler.mag_img_filter = filter;
   ctx->sampler.min_lod = src_level;
   ctx->sampler.max_lod = src_level;

   /* Depth stencil state, fragment shader and sampler setup depending on what
    * we blit.
    */
   if (blit_depth && blit_stencil) {
      cso_single_sampler(ctx->cso, PIPE_SHADER_FRAGMENT, 0, &ctx->sampler);
      /* don't filter stencil */
      ctx->sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
      ctx->sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
      cso_single_sampler(ctx->cso, PIPE_SHADER_FRAGMENT, 1, &ctx->sampler);

      cso_set_depth_stencil_alpha(ctx->cso, &ctx->dsa_write_depthstencil);
      set_depthstencil_fragment_shader(ctx, sampler_view->texture->target);
   }
   else if (blit_depth) {
      cso_single_sampler(ctx->cso, PIPE_SHADER_FRAGMENT, 0, &ctx->sampler);
      cso_set_depth_stencil_alpha(ctx->cso, &ctx->dsa_write_depth);
      set_depth_fragment_shader(ctx, sampler_view->texture->target);
   }
   else if (blit_stencil) {
      /* don't filter stencil */
      ctx->sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
      ctx->sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
      cso_single_sampler(ctx->cso, PIPE_SHADER_FRAGMENT, 0, &ctx->sampler);

      cso_set_depth_stencil_alpha(ctx->cso, &ctx->dsa_write_stencil);
      set_stencil_fragment_shader(ctx, sampler_view->texture->target);
   }
   else { /* color */
      cso_single_sampler(ctx->cso, PIPE_SHADER_FRAGMENT, 0, &ctx->sampler);
      cso_set_depth_stencil_alpha(ctx->cso, &ctx->dsa_keep_depthstencil);
      set_fragment_shader(ctx, writemask, sampler_view->texture->target);
   }
   cso_single_sampler_done(ctx->cso, PIPE_SHADER_FRAGMENT);

   /* textures */
   if (blit_depth && blit_stencil) {
      /* Setup two samplers, one for depth and the other one for stencil. */
      struct pipe_sampler_view templ;
      struct pipe_sampler_view *views[2];

      templ = *sampler_view;
      templ.format = util_format_stencil_only(templ.format);
      assert(templ.format != PIPE_FORMAT_NONE);

      views[0] = sampler_view;
      views[1] = pipe->create_sampler_view(pipe, views[0]->texture, &templ);
      cso_set_sampler_views(ctx->cso, PIPE_SHADER_FRAGMENT, 2, views);

      pipe_sampler_view_reference(&views[1], NULL);
   }
   else {
      cso_set_sampler_views(ctx->cso, PIPE_SHADER_FRAGMENT, 1, &sampler_view);
   }

   /* viewport */
   ctx->viewport.scale[0] = 0.5f * dst_surface->width;
   ctx->viewport.scale[1] = 0.5f * dst_surface->height;
   ctx->viewport.scale[2] = 0.5f;
   ctx->viewport.scale[3] = 1.0f;
   ctx->viewport.translate[0] = 0.5f * dst_surface->width;
   ctx->viewport.translate[1] = 0.5f * dst_surface->height;
   ctx->viewport.translate[2] = 0.5f;
   ctx->viewport.translate[3] = 0.0f;
   cso_set_viewport(ctx->cso, &ctx->viewport);

   set_vertex_shader(ctx);
   cso_set_geometry_shader_handle(ctx->cso, NULL);

   /* drawing dest */
   memset(&fb, 0, sizeof(fb));
   fb.width = dst_surface->width;
   fb.height = dst_surface->height;
   if (blit_depth || blit_stencil) {
      fb.zsbuf = dst_surface;
   } else {
      fb.nr_cbufs = 1;
      fb.cbufs[0] = dst_surface;
   }
   cso_set_framebuffer(ctx->cso, &fb);

   /* draw quad */
   offset = setup_vertex_data_tex(ctx,
                                  (float) dstX0 / dst_surface->width * 2.0f - 1.0f,
                                  (float) dstY0 / dst_surface->height * 2.0f - 1.0f,
                                  (float) dstX1 / dst_surface->width * 2.0f - 1.0f,
                                  (float) dstY1 / dst_surface->height * 2.0f - 1.0f,
                                  s0, t0,
                                  s1, t1,
                                  z);

   if (ctx->vbuf) {
      util_draw_vertex_buffer(ctx->pipe, ctx->cso, ctx->vbuf, offset,
                              PIPE_PRIM_TRIANGLE_FAN,
                              4,  /* verts */
                              2); /* attribs/vert */
   }

   /* restore state we changed */
   cso_restore_blend(ctx->cso);
   cso_restore_depth_stencil_alpha(ctx->cso);
   cso_restore_rasterizer(ctx->cso);
   cso_restore_sample_mask(ctx->cso);
   cso_restore_samplers(ctx->cso, PIPE_SHADER_FRAGMENT);
   cso_restore_sampler_views(ctx->cso, PIPE_SHADER_FRAGMENT);
   cso_restore_viewport(ctx->cso);
   cso_restore_framebuffer(ctx->cso);
   cso_restore_fragment_shader(ctx->cso);
   cso_restore_vertex_shader(ctx->cso);
   cso_restore_geometry_shader(ctx->cso);
   cso_restore_vertex_elements(ctx->cso);
   cso_restore_vertex_buffers(ctx->cso);
   cso_restore_stream_outputs(ctx->cso);

   pipe_sampler_view_reference(&sampler_view, NULL);
   if (dst_surface != dst)
      pipe_surface_reference(&dst_surface, NULL);
}


/**
 * Copy pixel block from src texture to dst surface.
 * The sampler view's first_level field indicates the source
 * mipmap level to use.
 * XXX need some control over blitting Z and/or stencil.
 */
void
util_blit_pixels_tex(struct blit_state *ctx,
                     struct pipe_sampler_view *src_sampler_view,
                     int srcX0, int srcY0,
                     int srcX1, int srcY1,
                     struct pipe_surface *dst,
                     int dstX0, int dstY0,
                     int dstX1, int dstY1,
                     float z, uint filter)
{
   boolean normalized = src_sampler_view->texture->target != PIPE_TEXTURE_RECT;
   struct pipe_framebuffer_state fb;
   float s0, t0, s1, t1;
   unsigned offset;
   struct pipe_resource *tex = src_sampler_view->texture;

   assert(filter == PIPE_TEX_MIPFILTER_NEAREST ||
          filter == PIPE_TEX_MIPFILTER_LINEAR);

   assert(tex);
   assert(tex->width0 != 0);
   assert(tex->height0 != 0);

   s0 = srcX0;
   s1 = srcX1;
   t0 = srcY0;
   t1 = srcY1;

   if(normalized)
   {
      /* normalize according to the mipmap level's size */
      int level = src_sampler_view->u.tex.first_level;
      float w = (float) u_minify(tex->width0, level);
      float h = (float) u_minify(tex->height0, level);
      s0 /= w;
      s1 /= w;
      t0 /= h;
      t1 /= h;
   }

   assert(ctx->pipe->screen->is_format_supported(ctx->pipe->screen, dst->format,
                                                 PIPE_TEXTURE_2D,
                                                 dst->texture->nr_samples,
                                                 PIPE_BIND_RENDER_TARGET));

   /* save state (restored below) */
   cso_save_blend(ctx->cso);
   cso_save_depth_stencil_alpha(ctx->cso);
   cso_save_rasterizer(ctx->cso);
   cso_save_sample_mask(ctx->cso);
   cso_save_samplers(ctx->cso, PIPE_SHADER_FRAGMENT);
   cso_save_sampler_views(ctx->cso, PIPE_SHADER_FRAGMENT);
   cso_save_stream_outputs(ctx->cso);
   cso_save_viewport(ctx->cso);
   cso_save_framebuffer(ctx->cso);
   cso_save_fragment_shader(ctx->cso);
   cso_save_vertex_shader(ctx->cso);
   cso_save_geometry_shader(ctx->cso);
   cso_save_vertex_elements(ctx->cso);
   cso_save_vertex_buffers(ctx->cso);

   /* set misc state we care about */
   cso_set_blend(ctx->cso, &ctx->blend_write_color);
   cso_set_depth_stencil_alpha(ctx->cso, &ctx->dsa_keep_depthstencil);
   cso_set_sample_mask(ctx->cso, ~0);
   cso_set_rasterizer(ctx->cso, &ctx->rasterizer);
   cso_set_vertex_elements(ctx->cso, 2, ctx->velem);
   cso_set_stream_outputs(ctx->cso, 0, NULL, 0);

   /* sampler */
   ctx->sampler.normalized_coords = normalized;
   ctx->sampler.min_img_filter = filter;
   ctx->sampler.mag_img_filter = filter;
   cso_single_sampler(ctx->cso, PIPE_SHADER_FRAGMENT, 0, &ctx->sampler);
   cso_single_sampler_done(ctx->cso, PIPE_SHADER_FRAGMENT);

   /* viewport */
   ctx->viewport.scale[0] = 0.5f * dst->width;
   ctx->viewport.scale[1] = 0.5f * dst->height;
   ctx->viewport.scale[2] = 0.5f;
   ctx->viewport.scale[3] = 1.0f;
   ctx->viewport.translate[0] = 0.5f * dst->width;
   ctx->viewport.translate[1] = 0.5f * dst->height;
   ctx->viewport.translate[2] = 0.5f;
   ctx->viewport.translate[3] = 0.0f;
   cso_set_viewport(ctx->cso, &ctx->viewport);

   /* texture */
   cso_set_sampler_views(ctx->cso, PIPE_SHADER_FRAGMENT, 1, &src_sampler_view);

   /* shaders */
   set_fragment_shader(ctx, TGSI_WRITEMASK_XYZW,
                       src_sampler_view->texture->target);
   set_vertex_shader(ctx);
   cso_set_geometry_shader_handle(ctx->cso, NULL);

   /* drawing dest */
   memset(&fb, 0, sizeof(fb));
   fb.width = dst->width;
   fb.height = dst->height;
   fb.nr_cbufs = 1;
   fb.cbufs[0] = dst;
   cso_set_framebuffer(ctx->cso, &fb);

   /* draw quad */
   offset = setup_vertex_data_tex(ctx,
                                  (float) dstX0 / dst->width * 2.0f - 1.0f,
                                  (float) dstY0 / dst->height * 2.0f - 1.0f,
                                  (float) dstX1 / dst->width * 2.0f - 1.0f,
                                  (float) dstY1 / dst->height * 2.0f - 1.0f,
                                  s0, t0, s1, t1,
                                  z);

   util_draw_vertex_buffer(ctx->pipe, ctx->cso,
                           ctx->vbuf, offset,
                           PIPE_PRIM_TRIANGLE_FAN,
                           4,  /* verts */
                           2); /* attribs/vert */

   /* restore state we changed */
   cso_restore_blend(ctx->cso);
   cso_restore_depth_stencil_alpha(ctx->cso);
   cso_restore_rasterizer(ctx->cso);
   cso_restore_sample_mask(ctx->cso);
   cso_restore_samplers(ctx->cso, PIPE_SHADER_FRAGMENT);
   cso_restore_sampler_views(ctx->cso, PIPE_SHADER_FRAGMENT);
   cso_restore_viewport(ctx->cso);
   cso_restore_framebuffer(ctx->cso);
   cso_restore_fragment_shader(ctx->cso);
   cso_restore_vertex_shader(ctx->cso);
   cso_restore_geometry_shader(ctx->cso);
   cso_restore_vertex_elements(ctx->cso);
   cso_restore_vertex_buffers(ctx->cso);
   cso_restore_stream_outputs(ctx->cso);
}
