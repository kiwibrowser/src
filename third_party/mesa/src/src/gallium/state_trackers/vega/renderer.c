/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
 * Copyright 2010 LunarG, Inc.  All Rights Reserved.
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

#include "renderer.h"

#include "vg_context.h"

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "pipe/p_screen.h"
#include "pipe/p_shader_tokens.h"

#include "util/u_draw_quad.h"
#include "util/u_simple_shaders.h"
#include "util/u_memory.h"
#include "util/u_sampler.h"
#include "util/u_surface.h"
#include "util/u_math.h"
#include "util/u_format.h"

#include "cso_cache/cso_context.h"
#include "tgsi/tgsi_ureg.h"

typedef enum {
   RENDERER_STATE_INIT,
   RENDERER_STATE_COPY,
   RENDERER_STATE_DRAWTEX,
   RENDERER_STATE_SCISSOR,
   RENDERER_STATE_CLEAR,
   RENDERER_STATE_FILTER,
   RENDERER_STATE_POLYGON_STENCIL,
   RENDERER_STATE_POLYGON_FILL,
   NUM_RENDERER_STATES
} RendererState;

typedef enum {
   RENDERER_VS_PLAIN,
   RENDERER_VS_COLOR,
   RENDERER_VS_TEXTURE,
   NUM_RENDERER_VS
} RendererVs;

typedef enum {
   RENDERER_FS_COLOR,
   RENDERER_FS_TEXTURE,
   RENDERER_FS_SCISSOR,
   RENDERER_FS_WHITE,
   NUM_RENDERER_FS
} RendererFs;

struct renderer {
   struct pipe_context *pipe;
   struct cso_context *cso;

   VGbitfield dirty;
   struct {
      struct pipe_rasterizer_state rasterizer;
      struct pipe_depth_stencil_alpha_state dsa;
      struct pipe_framebuffer_state fb;
   } g3d;
   struct matrix projection;

   struct matrix mvp;
   struct pipe_resource *vs_cbuf;

   struct pipe_resource *fs_cbuf;
   VGfloat fs_cbuf_data[32];
   VGint fs_cbuf_len;

   struct pipe_vertex_element velems[2];
   VGfloat vertices[4][2][4];

   void *cached_vs[NUM_RENDERER_VS];
   void *cached_fs[NUM_RENDERER_FS];

   RendererState state;

   /* state data */
   union {
      struct {
         VGint tex_width;
         VGint tex_height;
      } copy;

      struct {
         VGint tex_width;
         VGint tex_height;
      } drawtex;

      struct {
         VGboolean restore_dsa;
      } scissor;

      struct {
         VGboolean use_sampler;
         VGint tex_width, tex_height;
      } filter;

      struct {
         struct pipe_depth_stencil_alpha_state dsa;
         VGboolean manual_two_sides;
         VGboolean restore_dsa;
      } polygon_stencil;
   } u;
};

/**
 * Return VG_TRUE if the renderer can use the resource as the asked bindings.
 */
static VGboolean renderer_can_support(struct renderer *renderer,
                                      struct pipe_resource *res,
                                      unsigned bindings)
{
   struct pipe_screen *screen = renderer->pipe->screen;

   return screen->is_format_supported(screen,
         res->format, res->target, 0, bindings);
}

/**
 * Set the model-view-projection matrix used by vertex shaders.
 */
static void renderer_set_mvp(struct renderer *renderer,
                             const struct matrix *mvp)
{
   struct matrix *cur = &renderer->mvp;
   struct pipe_resource *cbuf;
   VGfloat consts[3][4];
   VGint i;

   /* projection only */
   if (!mvp)
      mvp = &renderer->projection;

   /* re-upload only if necessary */
   if (memcmp(cur, mvp, sizeof(*mvp)) == 0)
      return;

   /* 3x3 matrix to 3 constant vectors (no Z) */
   for (i = 0; i < 3; i++) {
      consts[i][0] = mvp->m[i + 0];
      consts[i][1] = mvp->m[i + 3];
      consts[i][2] = 0.0f;
      consts[i][3] = mvp->m[i + 6];
   }

   cbuf = renderer->vs_cbuf;
   pipe_resource_reference(&cbuf, NULL);
   cbuf = pipe_buffer_create(renderer->pipe->screen,
                             PIPE_BIND_CONSTANT_BUFFER,
                             PIPE_USAGE_STATIC,
                             sizeof(consts));
   if (cbuf) {
      pipe_buffer_write(renderer->pipe, cbuf,
            0, sizeof(consts), consts);
   }
   pipe_set_constant_buffer(renderer->pipe,
         PIPE_SHADER_VERTEX, 0, cbuf);

   memcpy(cur, mvp, sizeof(*mvp));
   renderer->vs_cbuf = cbuf;
}

/**
 * Create a simple vertex shader that passes through position and the given
 * attribute.
 */
static void *create_passthrough_vs(struct pipe_context *pipe, int semantic_name)
{
   struct ureg_program *ureg;
   struct ureg_src src[2], constants[3];
   struct ureg_dst dst[2], tmp;
   int i;

   ureg = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!ureg)
      return NULL;

   /* position is in user coordinates */
   src[0] = ureg_DECL_vs_input(ureg, 0);
   dst[0] = ureg_DECL_output(ureg, TGSI_SEMANTIC_POSITION, 0);
   tmp = ureg_DECL_temporary(ureg);
   for (i = 0; i < Elements(constants); i++)
      constants[i] = ureg_DECL_constant(ureg, i);

   /* transform to clipped coordinates */
   ureg_DP4(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), src[0], constants[0]);
   ureg_DP4(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_Y), src[0], constants[1]);
   ureg_MOV(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_Z), src[0]);
   ureg_DP4(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_W), src[0], constants[2]);
   ureg_MOV(ureg, dst[0], ureg_src(tmp));

   if (semantic_name >= 0) {
      src[1] = ureg_DECL_vs_input(ureg, 1);
      dst[1] = ureg_DECL_output(ureg, semantic_name, 0);
      ureg_MOV(ureg, dst[1], src[1]);
   }

   ureg_END(ureg);

   return ureg_create_shader_and_destroy(ureg, pipe);
}

/**
 * Set renderer vertex shader.
 *
 * This function modifies vertex_shader state.
 */
static void renderer_set_vs(struct renderer *r, RendererVs id)
{
   /* create as needed */
   if (!r->cached_vs[id]) {
      int semantic_name = -1;

      switch (id) {
      case RENDERER_VS_PLAIN:
         break;
      case RENDERER_VS_COLOR:
         semantic_name = TGSI_SEMANTIC_COLOR;
         break;
      case RENDERER_VS_TEXTURE:
         semantic_name = TGSI_SEMANTIC_GENERIC;
         break;
      default:
         assert(!"Unknown renderer vs id");
         break;
      }

      r->cached_vs[id] = create_passthrough_vs(r->pipe, semantic_name);
   }

   cso_set_vertex_shader_handle(r->cso, r->cached_vs[id]);
}

/**
 * Create a simple fragment shader that sets the depth to 0.0f.
 */
static void *create_scissor_fs(struct pipe_context *pipe)
{
   struct ureg_program *ureg;
   struct ureg_dst out;
   struct ureg_src imm;

   ureg = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   out = ureg_DECL_output(ureg, TGSI_SEMANTIC_POSITION, 0);
   imm = ureg_imm4f(ureg, 0.0f, 0.0f, 0.0f, 0.0f);

   ureg_MOV(ureg, ureg_writemask(out, TGSI_WRITEMASK_Z), imm);
   ureg_END(ureg);

   return ureg_create_shader_and_destroy(ureg, pipe);
}

/**
 * Create a simple fragment shader that sets the color to white.
 */
static void *create_white_fs(struct pipe_context *pipe)
{
   struct ureg_program *ureg;
   struct ureg_dst out;
   struct ureg_src imm;

   ureg = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   out = ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, 0);
   imm = ureg_imm4f(ureg, 1.0f, 1.0f, 1.0f, 1.0f);

   ureg_MOV(ureg, out, imm);
   ureg_END(ureg);

   return ureg_create_shader_and_destroy(ureg, pipe);
}

/**
 * Set renderer fragment shader.
 *
 * This function modifies fragment_shader state.
 */
static void renderer_set_fs(struct renderer *r, RendererFs id)
{
   /* create as needed */
   if (!r->cached_fs[id]) {
      void *fs = NULL;

      switch (id) {
      case RENDERER_FS_COLOR:
         fs = util_make_fragment_passthrough_shader(r->pipe);
         break;
      case RENDERER_FS_TEXTURE:
         fs = util_make_fragment_tex_shader(r->pipe,
               TGSI_TEXTURE_2D, TGSI_INTERPOLATE_LINEAR);
         break;
      case RENDERER_FS_SCISSOR:
         fs = create_scissor_fs(r->pipe);
         break;
      case RENDERER_FS_WHITE:
         fs = create_white_fs(r->pipe);
         break;
      default:
         assert(!"Unknown renderer fs id");
         break;
      }

      r->cached_fs[id] = fs;
   }

   cso_set_fragment_shader_handle(r->cso, r->cached_fs[id]);
}

typedef enum {
   VEGA_Y0_TOP,
   VEGA_Y0_BOTTOM
} VegaOrientation;

static void vg_set_viewport(struct renderer *r,
                            VegaOrientation orientation)
{
   const struct pipe_framebuffer_state *fb = &r->g3d.fb;
   struct pipe_viewport_state viewport;
   VGfloat y_scale = (orientation == VEGA_Y0_BOTTOM) ? -2.f : 2.f;

   viewport.scale[0] =  fb->width / 2.f;
   viewport.scale[1] =  fb->height / y_scale;
   viewport.scale[2] =  1.0;
   viewport.scale[3] =  1.0;
   viewport.translate[0] = fb->width / 2.f;
   viewport.translate[1] = fb->height / 2.f;
   viewport.translate[2] = 0.0;
   viewport.translate[3] = 0.0;

   cso_set_viewport(r->cso, &viewport);
}

/**
 * Set renderer target.
 *
 * This function modifies framebuffer and viewport states.
 */
static void renderer_set_target(struct renderer *r,
                                struct pipe_surface *cbuf,
                                struct pipe_surface *zsbuf,
                                VGboolean y0_top)
{
   struct pipe_framebuffer_state fb;

   memset(&fb, 0, sizeof(fb));
   fb.width = cbuf->width;
   fb.height = cbuf->height;
   fb.cbufs[0] = cbuf;
   fb.nr_cbufs = 1;
   fb.zsbuf = zsbuf;
   cso_set_framebuffer(r->cso, &fb);

   vg_set_viewport(r, (y0_top) ? VEGA_Y0_TOP : VEGA_Y0_BOTTOM);
}

/**
 * Set renderer blend state.  Blending is disabled.
 *
 * This function modifies blend state.
 */
static void renderer_set_blend(struct renderer *r,
                               VGbitfield channel_mask)
{
   struct pipe_blend_state blend;

   memset(&blend, 0, sizeof(blend));

   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;

   if (channel_mask & VG_RED)
      blend.rt[0].colormask |= PIPE_MASK_R;
   if (channel_mask & VG_GREEN)
      blend.rt[0].colormask |= PIPE_MASK_G;
   if (channel_mask & VG_BLUE)
      blend.rt[0].colormask |= PIPE_MASK_B;
   if (channel_mask & VG_ALPHA)
      blend.rt[0].colormask |= PIPE_MASK_A;

   cso_set_blend(r->cso, &blend);
}

/**
 * Set renderer sampler and view states.
 *
 * This function modifies samplers and fragment_sampler_views states.
 */
static void renderer_set_samplers(struct renderer *r,
                                  uint num_views,
                                  struct pipe_sampler_view **views)
{
   struct pipe_sampler_state sampler;
   unsigned tex_filter = PIPE_TEX_FILTER_NEAREST;
   unsigned tex_wrap = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   uint i;

   memset(&sampler, 0, sizeof(sampler));

   sampler.min_img_filter = tex_filter;
   sampler.mag_img_filter = tex_filter;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;

   sampler.wrap_s = tex_wrap;
   sampler.wrap_t = tex_wrap;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;

   sampler.normalized_coords = 1;

   /* set samplers */
   for (i = 0; i < num_views; i++)
      cso_single_sampler(r->cso, PIPE_SHADER_FRAGMENT, i, &sampler);
   cso_single_sampler_done(r->cso, PIPE_SHADER_FRAGMENT);

   /* set views */
   cso_set_sampler_views(r->cso, PIPE_SHADER_FRAGMENT, num_views, views);
}

/**
 * Set custom renderer fragment shader, and optionally set samplers and views
 * and upload the fragment constant buffer.
 *
 * This function modifies fragment_shader, samplers and fragment_sampler_views
 * states.
 */
static void renderer_set_custom_fs(struct renderer *renderer,
                                   void *fs,
                                   const struct pipe_sampler_state **samplers,
                                   struct pipe_sampler_view **views,
                                   VGint num_samplers,
                                   const void *const_buffer,
                                   VGint const_buffer_len)
{
   cso_set_fragment_shader_handle(renderer->cso, fs);

   /* set samplers and views */
   if (num_samplers) {
      cso_set_samplers(renderer->cso, PIPE_SHADER_FRAGMENT, num_samplers, samplers);
      cso_set_sampler_views(renderer->cso, PIPE_SHADER_FRAGMENT, num_samplers, views);
   }

   /* upload fs constant buffer */
   if (const_buffer_len) {
      struct pipe_resource *cbuf = renderer->fs_cbuf;

      if (!cbuf || renderer->fs_cbuf_len != const_buffer_len ||
          memcmp(renderer->fs_cbuf_data, const_buffer, const_buffer_len)) {
         pipe_resource_reference(&cbuf, NULL);

         cbuf = pipe_buffer_create(renderer->pipe->screen,
               PIPE_BIND_CONSTANT_BUFFER, PIPE_USAGE_STATIC,
               const_buffer_len);
         pipe_buffer_write(renderer->pipe, cbuf, 0,
               const_buffer_len, const_buffer);
         pipe_set_constant_buffer(renderer->pipe,
               PIPE_SHADER_FRAGMENT, 0, cbuf);

         renderer->fs_cbuf = cbuf;
         if (const_buffer_len <= sizeof(renderer->fs_cbuf_data)) {
            memcpy(renderer->fs_cbuf_data, const_buffer, const_buffer_len);
            renderer->fs_cbuf_len = const_buffer_len;
         }
         else {
            renderer->fs_cbuf_len = 0;
         }
      }
   }
}

/**
 * Setup renderer quad position.
 */
static void renderer_quad_pos(struct renderer *r,
                              VGfloat x0, VGfloat y0,
                              VGfloat x1, VGfloat y1,
                              VGboolean scissor)
{
   VGfloat z;

   /* the depth test is used for scissoring */
   z = (scissor) ? 0.0f : 1.0f;

   /* positions */
   r->vertices[0][0][0] = x0;
   r->vertices[0][0][1] = y0;
   r->vertices[0][0][2] = z;

   r->vertices[1][0][0] = x1;
   r->vertices[1][0][1] = y0;
   r->vertices[1][0][2] = z;

   r->vertices[2][0][0] = x1;
   r->vertices[2][0][1] = y1;
   r->vertices[2][0][2] = z;

   r->vertices[3][0][0] = x0;
   r->vertices[3][0][1] = y1;
   r->vertices[3][0][2] = z;
}

/**
 * Setup renderer quad texture coordinates.
 */
static void renderer_quad_texcoord(struct renderer *r,
                                   VGfloat x0, VGfloat y0,
                                   VGfloat x1, VGfloat y1,
                                   VGint tex_width, VGint tex_height)
{
   VGfloat s0, t0, s1, t1, r0, q0;
   VGint i;

   s0 = x0 / tex_width;
   s1 = x1 / tex_width;
   t0 = y0 / tex_height;
   t1 = y1 / tex_height;
   r0 = 0.0f;
   q0 = 1.0f;

   /* texcoords */
   r->vertices[0][1][0] = s0;
   r->vertices[0][1][1] = t0;

   r->vertices[1][1][0] = s1;
   r->vertices[1][1][1] = t0;

   r->vertices[2][1][0] = s1;
   r->vertices[2][1][1] = t1;

   r->vertices[3][1][0] = s0;
   r->vertices[3][1][1] = t1;

   for (i = 0; i < 4; i++) {
      r->vertices[i][1][2] = r0;
      r->vertices[i][1][3] = q0;
   }
}

/**
 * Draw renderer quad.
 */
static void renderer_quad_draw(struct renderer *r)
{
   util_draw_user_vertex_buffer(r->cso, r->vertices, PIPE_PRIM_TRIANGLE_FAN,
                                Elements(r->vertices),     /* verts */
                                Elements(r->vertices[0])); /* attribs/vert */
}

/**
 * Prepare the renderer for copying.
 */
VGboolean renderer_copy_begin(struct renderer *renderer,
                              struct pipe_surface *dst,
                              VGboolean y0_top,
                              struct pipe_sampler_view *src)
{
   assert(renderer->state == RENDERER_STATE_INIT);

   /* sanity check */
   if (!renderer_can_support(renderer,
            dst->texture, PIPE_BIND_RENDER_TARGET) ||
       !renderer_can_support(renderer,
          src->texture, PIPE_BIND_SAMPLER_VIEW))
      return VG_FALSE;

   cso_save_framebuffer(renderer->cso);
   cso_save_viewport(renderer->cso);
   cso_save_blend(renderer->cso);
   cso_save_samplers(renderer->cso, PIPE_SHADER_FRAGMENT);
   cso_save_sampler_views(renderer->cso, PIPE_SHADER_FRAGMENT);
   cso_save_fragment_shader(renderer->cso);
   cso_save_vertex_shader(renderer->cso);

   renderer_set_target(renderer, dst, NULL, y0_top);

   renderer_set_blend(renderer, ~0);
   renderer_set_samplers(renderer, 1, &src);

   renderer_set_fs(renderer, RENDERER_FS_TEXTURE);
   renderer_set_vs(renderer, RENDERER_VS_TEXTURE);

   renderer_set_mvp(renderer, NULL);

   /* remember the texture size */
   renderer->u.copy.tex_width = src->texture->width0;
   renderer->u.copy.tex_height = src->texture->height0;
   renderer->state = RENDERER_STATE_COPY;

   return VG_TRUE;
}

/**
 * Draw into the destination rectangle given by (x, y, w, h).  The texture is
 * sampled from within the rectangle given by (sx, sy, sw, sh).
 *
 * The coordinates are in surface coordinates.
 */
void renderer_copy(struct renderer *renderer,
                   VGint x, VGint y, VGint w, VGint h,
                   VGint sx, VGint sy, VGint sw, VGint sh)
{
   assert(renderer->state == RENDERER_STATE_COPY);

   /* there is no depth buffer for scissoring anyway */
   renderer_quad_pos(renderer, x, y, x + w, y + h, VG_FALSE);
   renderer_quad_texcoord(renderer, sx, sy, sx + sw, sy + sh,
         renderer->u.copy.tex_width,
         renderer->u.copy.tex_height);

   renderer_quad_draw(renderer);
}

/**
 * End copying and restore the states.
 */
void renderer_copy_end(struct renderer *renderer)
{
   assert(renderer->state == RENDERER_STATE_COPY);

   cso_restore_framebuffer(renderer->cso);
   cso_restore_viewport(renderer->cso);
   cso_restore_blend(renderer->cso);
   cso_restore_samplers(renderer->cso, PIPE_SHADER_FRAGMENT);
   cso_restore_sampler_views(renderer->cso, PIPE_SHADER_FRAGMENT);
   cso_restore_fragment_shader(renderer->cso);
   cso_restore_vertex_shader(renderer->cso);

   renderer->state = RENDERER_STATE_INIT;
}

/**
 * Prepare the renderer for textured drawing.
 */
VGboolean renderer_drawtex_begin(struct renderer *renderer,
                                 struct pipe_sampler_view *src)
{
   assert(renderer->state == RENDERER_STATE_INIT);

   if (!renderer_can_support(renderer, src->texture, PIPE_BIND_SAMPLER_VIEW))
      return VG_FALSE;

   cso_save_blend(renderer->cso);
   cso_save_samplers(renderer->cso, PIPE_SHADER_FRAGMENT);
   cso_save_sampler_views(renderer->cso, PIPE_SHADER_FRAGMENT);
   cso_save_fragment_shader(renderer->cso);
   cso_save_vertex_shader(renderer->cso);

   renderer_set_blend(renderer, ~0);

   renderer_set_samplers(renderer, 1, &src);

   renderer_set_fs(renderer, RENDERER_FS_TEXTURE);
   renderer_set_vs(renderer, RENDERER_VS_TEXTURE);

   renderer_set_mvp(renderer, NULL);

   /* remember the texture size */
   renderer->u.drawtex.tex_width = src->texture->width0;
   renderer->u.drawtex.tex_height = src->texture->height0;
   renderer->state = RENDERER_STATE_DRAWTEX;

   return VG_TRUE;
}

/**
 * Draw into the destination rectangle given by (x, y, w, h).  The texture is
 * sampled from within the rectangle given by (sx, sy, sw, sh).
 *
 * The coordinates are in surface coordinates.
 */
void renderer_drawtex(struct renderer *renderer,
                      VGint x, VGint y, VGint w, VGint h,
                      VGint sx, VGint sy, VGint sw, VGint sh)
{
   assert(renderer->state == RENDERER_STATE_DRAWTEX);

   /* with scissoring */
   renderer_quad_pos(renderer, x, y, x + w, y + h, VG_TRUE);
   renderer_quad_texcoord(renderer, sx, sy, sx + sw, sy + sh,
         renderer->u.drawtex.tex_width,
         renderer->u.drawtex.tex_height);

   renderer_quad_draw(renderer);
}

/**
 * End textured drawing and restore the states.
 */
void renderer_drawtex_end(struct renderer *renderer)
{
   assert(renderer->state == RENDERER_STATE_DRAWTEX);

   cso_restore_blend(renderer->cso);
   cso_restore_samplers(renderer->cso, PIPE_SHADER_FRAGMENT);
   cso_restore_sampler_views(renderer->cso, PIPE_SHADER_FRAGMENT);
   cso_restore_fragment_shader(renderer->cso);
   cso_restore_vertex_shader(renderer->cso);

   renderer->state = RENDERER_STATE_INIT;
}

/**
 * Prepare the renderer for scissor update.  This will reset the depth buffer
 * to 1.0f.
 */
VGboolean renderer_scissor_begin(struct renderer *renderer,
                                 VGboolean restore_dsa)
{
   struct pipe_depth_stencil_alpha_state dsa;

   assert(renderer->state == RENDERER_STATE_INIT);

   if (restore_dsa)
      cso_save_depth_stencil_alpha(renderer->cso);
   cso_save_blend(renderer->cso);
   cso_save_fragment_shader(renderer->cso);

   /* enable depth writes */
   memset(&dsa, 0, sizeof(dsa));
   dsa.depth.enabled = 1;
   dsa.depth.writemask = 1;
   dsa.depth.func = PIPE_FUNC_ALWAYS;
   cso_set_depth_stencil_alpha(renderer->cso, &dsa);

   /* disable color writes */
   renderer_set_blend(renderer, 0);
   renderer_set_fs(renderer, RENDERER_FS_SCISSOR);

   renderer_set_mvp(renderer, NULL);

   renderer->u.scissor.restore_dsa = restore_dsa;
   renderer->state = RENDERER_STATE_SCISSOR;

   /* clear the depth buffer to 1.0f */
   renderer->pipe->clear(renderer->pipe,
         PIPE_CLEAR_DEPTHSTENCIL, NULL, 1.0f, 0);

   return VG_TRUE;
}

/**
 * Add a scissor rectangle.  Depth values inside the rectangle will be set to
 * 0.0f.
 */
void renderer_scissor(struct renderer *renderer,
                      VGint x, VGint y, VGint width, VGint height)
{
   assert(renderer->state == RENDERER_STATE_SCISSOR);

   renderer_quad_pos(renderer, x, y, x + width, y + height, VG_FALSE);
   renderer_quad_draw(renderer);
}

/**
 * End scissor update and restore the states.
 */
void renderer_scissor_end(struct renderer *renderer)
{
   assert(renderer->state == RENDERER_STATE_SCISSOR);

   if (renderer->u.scissor.restore_dsa)
      cso_restore_depth_stencil_alpha(renderer->cso);
   cso_restore_blend(renderer->cso);
   cso_restore_fragment_shader(renderer->cso);

   renderer->state = RENDERER_STATE_INIT;
}

/**
 * Prepare the renderer for clearing.
 */
VGboolean renderer_clear_begin(struct renderer *renderer)
{
   assert(renderer->state == RENDERER_STATE_INIT);

   cso_save_blend(renderer->cso);
   cso_save_fragment_shader(renderer->cso);
   cso_save_vertex_shader(renderer->cso);

   renderer_set_blend(renderer, ~0);
   renderer_set_fs(renderer, RENDERER_FS_COLOR);
   renderer_set_vs(renderer, RENDERER_VS_COLOR);

   renderer_set_mvp(renderer, NULL);

   renderer->state = RENDERER_STATE_CLEAR;

   return VG_TRUE;
}

/**
 * Clear the framebuffer with the specified region and color.
 *
 * The coordinates are in surface coordinates.
 */
void renderer_clear(struct renderer *renderer,
                    VGint x, VGint y, VGint width, VGint height,
                    const VGfloat color[4])
{
   VGuint i;

   assert(renderer->state == RENDERER_STATE_CLEAR);

   renderer_quad_pos(renderer, x, y, x + width, y + height, VG_TRUE);
   for (i = 0; i < 4; i++)
      memcpy(renderer->vertices[i][1], color, sizeof(VGfloat) * 4);

   renderer_quad_draw(renderer);
}

/**
 * End clearing and retore the states.
 */
void renderer_clear_end(struct renderer *renderer)
{
   assert(renderer->state == RENDERER_STATE_CLEAR);

   cso_restore_blend(renderer->cso);
   cso_restore_fragment_shader(renderer->cso);
   cso_restore_vertex_shader(renderer->cso);

   renderer->state = RENDERER_STATE_INIT;
}

/**
 * Prepare the renderer for image filtering.
 */
VGboolean renderer_filter_begin(struct renderer *renderer,
                                struct pipe_resource *dst,
                                VGboolean y0_top,
                                VGbitfield channel_mask,
                                const struct pipe_sampler_state **samplers,
                                struct pipe_sampler_view **views,
                                VGint num_samplers,
                                void *fs,
                                const void *const_buffer,
                                VGint const_buffer_len)
{
   struct pipe_surface *surf, surf_tmpl;

   assert(renderer->state == RENDERER_STATE_INIT);

   if (!fs)
      return VG_FALSE;
   if (!renderer_can_support(renderer, dst, PIPE_BIND_RENDER_TARGET))
      return VG_FALSE;

   u_surface_default_template(&surf_tmpl, dst,
                              PIPE_BIND_RENDER_TARGET);
   surf = renderer->pipe->create_surface(renderer->pipe, dst, &surf_tmpl);
   if (!surf)
      return VG_FALSE;

   cso_save_framebuffer(renderer->cso);
   cso_save_viewport(renderer->cso);
   cso_save_blend(renderer->cso);

   /* set the image as the target */
   renderer_set_target(renderer, surf, NULL, y0_top);
   pipe_surface_reference(&surf, NULL);

   renderer_set_blend(renderer, channel_mask);

   if (num_samplers) {
      struct pipe_resource *tex;

      cso_save_samplers(renderer->cso, PIPE_SHADER_FRAGMENT);
      cso_save_sampler_views(renderer->cso, PIPE_SHADER_FRAGMENT);
      cso_save_fragment_shader(renderer->cso);
      cso_save_vertex_shader(renderer->cso);

      renderer_set_custom_fs(renderer, fs,
                             samplers, views, num_samplers,
                             const_buffer, const_buffer_len);
      renderer_set_vs(renderer, RENDERER_VS_TEXTURE);

      tex = views[0]->texture;
      renderer->u.filter.tex_width = tex->width0;
      renderer->u.filter.tex_height = tex->height0;
      renderer->u.filter.use_sampler = VG_TRUE;
   }
   else {
      cso_save_fragment_shader(renderer->cso);

      renderer_set_custom_fs(renderer, fs, NULL, NULL, 0,
                             const_buffer, const_buffer_len);

      renderer->u.filter.use_sampler = VG_FALSE;
   }

   renderer_set_mvp(renderer, NULL);

   renderer->state = RENDERER_STATE_FILTER;

   return VG_TRUE;
}

/**
 * Draw into a rectangle of the destination with the specified region of the
 * texture(s).
 *
 * The coordinates are in surface coordinates.
 */
void renderer_filter(struct renderer *renderer,
                    VGint x, VGint y, VGint w, VGint h,
                    VGint sx, VGint sy, VGint sw, VGint sh)
{
   assert(renderer->state == RENDERER_STATE_FILTER);

   renderer_quad_pos(renderer, x, y, x + w, y + h, VG_FALSE);
   if (renderer->u.filter.use_sampler) {
      renderer_quad_texcoord(renderer, sx, sy, sx + sw, sy + sh,
            renderer->u.filter.tex_width,
            renderer->u.filter.tex_height);
   }

   renderer_quad_draw(renderer);
}

/**
 * End image filtering and restore the states.
 */
void renderer_filter_end(struct renderer *renderer)
{
   assert(renderer->state == RENDERER_STATE_FILTER);

   if (renderer->u.filter.use_sampler) {
      cso_restore_samplers(renderer->cso, PIPE_SHADER_FRAGMENT);
      cso_restore_sampler_views(renderer->cso, PIPE_SHADER_FRAGMENT);
      cso_restore_vertex_shader(renderer->cso);
   }

   cso_restore_framebuffer(renderer->cso);
   cso_restore_viewport(renderer->cso);
   cso_restore_blend(renderer->cso);
   cso_restore_fragment_shader(renderer->cso);

   renderer->state = RENDERER_STATE_INIT;
}

/**
 * Prepare the renderer for polygon silhouette rendering.
 */
VGboolean renderer_polygon_stencil_begin(struct renderer *renderer,
                                         struct pipe_vertex_element *velem,
                                         VGFillRule rule,
                                         VGboolean restore_dsa)
{
   struct pipe_depth_stencil_alpha_state *dsa;
   VGboolean manual_two_sides;

   assert(renderer->state == RENDERER_STATE_INIT);

   cso_save_vertex_elements(renderer->cso);
   cso_save_blend(renderer->cso);
   cso_save_depth_stencil_alpha(renderer->cso);

   cso_set_vertex_elements(renderer->cso, 1, velem);

   /* disable color writes */
   renderer_set_blend(renderer, 0);

   manual_two_sides = VG_FALSE;
   dsa = &renderer->u.polygon_stencil.dsa;
   memset(dsa, 0, sizeof(*dsa));
   if (rule == VG_EVEN_ODD) {
      dsa->stencil[0].enabled = 1;
      dsa->stencil[0].writemask = 1;
      dsa->stencil[0].fail_op = PIPE_STENCIL_OP_KEEP;
      dsa->stencil[0].zfail_op = PIPE_STENCIL_OP_KEEP;
      dsa->stencil[0].zpass_op = PIPE_STENCIL_OP_INVERT;
      dsa->stencil[0].func = PIPE_FUNC_ALWAYS;
      dsa->stencil[0].valuemask = ~0;
   }
   else {
      assert(rule == VG_NON_ZERO);

      /* front face */
      dsa->stencil[0].enabled = 1;
      dsa->stencil[0].writemask = ~0;
      dsa->stencil[0].fail_op = PIPE_STENCIL_OP_KEEP;
      dsa->stencil[0].zfail_op = PIPE_STENCIL_OP_KEEP;
      dsa->stencil[0].zpass_op = PIPE_STENCIL_OP_INCR_WRAP;
      dsa->stencil[0].func = PIPE_FUNC_ALWAYS;
      dsa->stencil[0].valuemask = ~0;

      if (renderer->pipe->screen->get_param(renderer->pipe->screen,
                                            PIPE_CAP_TWO_SIDED_STENCIL)) {
         /* back face */
         dsa->stencil[1] = dsa->stencil[0];
         dsa->stencil[1].zpass_op = PIPE_STENCIL_OP_DECR_WRAP;
      }
      else {
         manual_two_sides = VG_TRUE;
      }
   }
   cso_set_depth_stencil_alpha(renderer->cso, dsa);

   if (manual_two_sides)
      cso_save_rasterizer(renderer->cso);

   renderer->u.polygon_stencil.manual_two_sides = manual_two_sides;
   renderer->u.polygon_stencil.restore_dsa = restore_dsa;
   renderer->state = RENDERER_STATE_POLYGON_STENCIL;

   return VG_TRUE;
}

/**
 * Render a polygon silhouette to stencil buffer.
 */
void renderer_polygon_stencil(struct renderer *renderer,
                              struct pipe_vertex_buffer *vbuf,
                              VGuint mode, VGuint start, VGuint count)
{
   assert(renderer->state == RENDERER_STATE_POLYGON_STENCIL);

   cso_set_vertex_buffers(renderer->cso, 1, vbuf);

   if (!renderer->u.polygon_stencil.manual_two_sides) {
      cso_draw_arrays(renderer->cso, mode, start, count);
   }
   else {
      struct pipe_rasterizer_state raster;
      struct pipe_depth_stencil_alpha_state dsa;

      raster = renderer->g3d.rasterizer;
      dsa = renderer->u.polygon_stencil.dsa;

      /* front */
      raster.cull_face = PIPE_FACE_BACK;
      dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_INCR_WRAP;

      cso_set_rasterizer(renderer->cso, &raster);
      cso_set_depth_stencil_alpha(renderer->cso, &dsa);
      cso_draw_arrays(renderer->cso, mode, start, count);

      /* back */
      raster.cull_face = PIPE_FACE_FRONT;
      dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_DECR_WRAP;

      cso_set_rasterizer(renderer->cso, &raster);
      cso_set_depth_stencil_alpha(renderer->cso, &dsa);
      cso_draw_arrays(renderer->cso, mode, start, count);
   }
}

/**
 * End polygon silhouette rendering.
 */
void renderer_polygon_stencil_end(struct renderer *renderer)
{
   assert(renderer->state == RENDERER_STATE_POLYGON_STENCIL);

   if (renderer->u.polygon_stencil.manual_two_sides)
      cso_restore_rasterizer(renderer->cso);

   cso_restore_vertex_elements(renderer->cso);

   /* restore color writes */
   cso_restore_blend(renderer->cso);

   if (renderer->u.polygon_stencil.restore_dsa)
      cso_restore_depth_stencil_alpha(renderer->cso);

   renderer->state = RENDERER_STATE_INIT;
}

/**
 * Prepare the renderer for polygon filling.
 */
VGboolean renderer_polygon_fill_begin(struct renderer *renderer,
                                      VGboolean save_dsa)
{
   struct pipe_depth_stencil_alpha_state dsa;

   assert(renderer->state == RENDERER_STATE_INIT);

   if (save_dsa)
      cso_save_depth_stencil_alpha(renderer->cso);

   /* setup stencil ops */
   memset(&dsa, 0, sizeof(dsa));
   dsa.stencil[0].enabled = 1;
   dsa.stencil[0].func = PIPE_FUNC_NOTEQUAL;
   dsa.stencil[0].fail_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].zfail_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].valuemask = ~0;
   dsa.stencil[0].writemask = ~0;
   dsa.depth = renderer->g3d.dsa.depth;
   cso_set_depth_stencil_alpha(renderer->cso, &dsa);

   renderer->state = RENDERER_STATE_POLYGON_FILL;

   return VG_TRUE;
}

/**
 * Fill a polygon.
 */
void renderer_polygon_fill(struct renderer *renderer,
                           VGfloat min_x, VGfloat min_y,
                           VGfloat max_x, VGfloat max_y)
{
   assert(renderer->state == RENDERER_STATE_POLYGON_FILL);

   renderer_quad_pos(renderer, min_x, min_y, max_x, max_y, VG_TRUE);
   renderer_quad_draw(renderer);
}

/**
 * End polygon filling.
 */
void renderer_polygon_fill_end(struct renderer *renderer)
{
   assert(renderer->state == RENDERER_STATE_POLYGON_FILL);

   cso_restore_depth_stencil_alpha(renderer->cso);

   renderer->state = RENDERER_STATE_INIT;
}

struct renderer * renderer_create(struct vg_context *owner)
{
   struct renderer *renderer;
   struct pipe_rasterizer_state *raster;
   struct pipe_stencil_ref sr;
   VGint i;

   renderer = CALLOC_STRUCT(renderer);
   if (!renderer)
      return NULL;

   renderer->pipe = owner->pipe;
   renderer->cso = owner->cso_context;

   /* init vertex data that doesn't change */
   for (i = 0; i < 4; i++)
      renderer->vertices[i][0][3] = 1.0f; /* w */

   for (i = 0; i < 2; i++) {
      renderer->velems[i].src_offset = i * 4 * sizeof(float);
      renderer->velems[i].instance_divisor = 0;
      renderer->velems[i].vertex_buffer_index = 0;
      renderer->velems[i].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   }
   cso_set_vertex_elements(renderer->cso, 2, renderer->velems);

   /* GL rasterization rules */
   raster = &renderer->g3d.rasterizer;
   memset(raster, 0, sizeof(*raster));
   raster->gl_rasterization_rules = 1;
   raster->depth_clip = 1;
   cso_set_rasterizer(renderer->cso, raster);

   /* fixed at 0 */
   memset(&sr, 0, sizeof(sr));
   cso_set_stencil_ref(renderer->cso, &sr);

   renderer_set_vs(renderer, RENDERER_VS_PLAIN);

   renderer->state = RENDERER_STATE_INIT;

   return renderer;
}

void renderer_destroy(struct renderer *ctx)
{
   int i;

   for (i = 0; i < NUM_RENDERER_VS; i++) {
      if (ctx->cached_vs[i])
         cso_delete_vertex_shader(ctx->cso, ctx->cached_vs[i]);
   }
   for (i = 0; i < NUM_RENDERER_FS; i++) {
      if (ctx->cached_fs[i])
         cso_delete_fragment_shader(ctx->cso, ctx->cached_fs[i]);
   }

   pipe_resource_reference(&ctx->vs_cbuf, NULL);
   pipe_resource_reference(&ctx->fs_cbuf, NULL);

   FREE(ctx);
}

static void update_clip_state(struct renderer *renderer,
                              const struct vg_state *state)
{
   struct pipe_depth_stencil_alpha_state *dsa = &renderer->g3d.dsa;

   memset(dsa, 0, sizeof(struct pipe_depth_stencil_alpha_state));

   if (state->scissoring) {
      struct pipe_framebuffer_state *fb = &renderer->g3d.fb;
      int i;

      renderer_scissor_begin(renderer, VG_FALSE);

      for (i = 0; i < state->scissor_rects_num; ++i) {
         const float x      = state->scissor_rects[i * 4 + 0].f;
         const float y      = state->scissor_rects[i * 4 + 1].f;
         const float width  = state->scissor_rects[i * 4 + 2].f;
         const float height = state->scissor_rects[i * 4 + 3].f;
         VGint x0, y0, x1, y1, iw, ih;

         x0 = (VGint) x;
         y0 = (VGint) y;
         if (x0 < 0)
            x0 = 0;
         if (y0 < 0)
            y0 = 0;

         /* note that x1 and y1 are exclusive */
         x1 = (VGint) ceilf(x + width);
         y1 = (VGint) ceilf(y + height);
         if (x1 > fb->width)
            x1 = fb->width;
         if (y1 > fb->height)
            y1 = fb->height;

         iw = x1 - x0;
         ih = y1 - y0;
         if (iw > 0 && ih> 0 )
            renderer_scissor(renderer, x0, y0, iw, ih);
      }

      renderer_scissor_end(renderer);

      dsa->depth.enabled = 1; /* glEnable(GL_DEPTH_TEST); */
      dsa->depth.writemask = 0;/*glDepthMask(FALSE);*/
      dsa->depth.func = PIPE_FUNC_GEQUAL;
   }
}

static void renderer_validate_blend(struct renderer *renderer,
                                     const struct vg_state *state,
                                     enum pipe_format fb_format)
{
   struct pipe_blend_state blend;

   memset(&blend, 0, sizeof(blend));
   blend.rt[0].colormask = PIPE_MASK_RGBA;
   blend.rt[0].rgb_src_factor   = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].rgb_dst_factor   = PIPE_BLENDFACTOR_ZERO;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;

   /* TODO alpha masking happens after blending? */

   switch (state->blend_mode) {
   case VG_BLEND_SRC:
      blend.rt[0].rgb_src_factor   = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].rgb_dst_factor   = PIPE_BLENDFACTOR_ZERO;
      blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
      break;
   case VG_BLEND_SRC_OVER:
      /* use the blend state only when there is no alpha channel */
      if (!util_format_has_alpha(fb_format)) {
         blend.rt[0].rgb_src_factor   = PIPE_BLENDFACTOR_SRC_ALPHA;
         blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
         blend.rt[0].rgb_dst_factor   = PIPE_BLENDFACTOR_INV_SRC_ALPHA;
         blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_INV_SRC_ALPHA;
         blend.rt[0].blend_enable = 1;
      }
      break;
   case VG_BLEND_SRC_IN:
      blend.rt[0].rgb_src_factor   = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_DST_ALPHA;
      blend.rt[0].rgb_dst_factor   = PIPE_BLENDFACTOR_ZERO;
      blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.rt[0].blend_enable = 1;
      break;
   case VG_BLEND_DST_IN:
      blend.rt[0].rgb_src_factor   = PIPE_BLENDFACTOR_ZERO;
      blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ZERO;
      blend.rt[0].rgb_dst_factor   = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_SRC_ALPHA;
      blend.rt[0].blend_enable = 1;
      break;
   case VG_BLEND_DST_OVER:
   case VG_BLEND_MULTIPLY:
   case VG_BLEND_SCREEN:
   case VG_BLEND_DARKEN:
   case VG_BLEND_LIGHTEN:
   case VG_BLEND_ADDITIVE:
      /* need a shader */
      break;
   default:
      assert(!"not implemented blend mode");
      break;
   }

   cso_set_blend(renderer->cso, &blend);
}

/**
 * Propogate OpenVG state changes to the renderer.  Only framebuffer, blending
 * and scissoring states are relevant here.
 */
void renderer_validate(struct renderer *renderer,
                       VGbitfield dirty,
                       const struct st_framebuffer *stfb,
                       const struct vg_state *state)
{
   assert(renderer->state == RENDERER_STATE_INIT);

   dirty |= renderer->dirty;
   renderer->dirty = 0;

   if (dirty & FRAMEBUFFER_DIRTY) {
      struct pipe_framebuffer_state *fb = &renderer->g3d.fb;
      struct matrix *proj = &renderer->projection;

      memset(fb, 0, sizeof(struct pipe_framebuffer_state));
      fb->width  = stfb->width;
      fb->height = stfb->height;
      fb->nr_cbufs = 1;
      fb->cbufs[0] = stfb->strb->surface;
      fb->zsbuf = stfb->dsrb->surface;

      cso_set_framebuffer(renderer->cso, fb);
      vg_set_viewport(renderer, VEGA_Y0_BOTTOM);

      matrix_load_identity(proj);
      matrix_translate(proj, -1.0f, -1.0f);
      matrix_scale(proj, 2.0f / fb->width, 2.0f / fb->height);

      /* we also got a new depth buffer */
      if (dirty & DEPTH_STENCIL_DIRTY) {
         renderer->pipe->clear(renderer->pipe,
               PIPE_CLEAR_DEPTHSTENCIL, NULL, 0.0, 0);
      }
   }

   /* must be last because it renders to the depth buffer*/
   if (dirty & DEPTH_STENCIL_DIRTY) {
      update_clip_state(renderer, state);
      cso_set_depth_stencil_alpha(renderer->cso, &renderer->g3d.dsa);
   }

   if (dirty & BLEND_DIRTY)
      renderer_validate_blend(renderer, state, stfb->strb->format);
}

/**
 * Prepare the renderer for OpenVG pipeline.
 */
void renderer_validate_for_shader(struct renderer *renderer,
                                  const struct pipe_sampler_state **samplers,
                                  struct pipe_sampler_view **views,
                                  VGint num_samplers,
                                  const struct matrix *modelview,
                                  void *fs,
                                  const void *const_buffer,
                                  VGint const_buffer_len)
{
   struct matrix mvp = renderer->projection;

   /* will be used in POLYGON_STENCIL and POLYGON_FILL */
   matrix_mult(&mvp, modelview);
   renderer_set_mvp(renderer, &mvp);

   renderer_set_custom_fs(renderer, fs,
                          samplers, views, num_samplers,
                          const_buffer, const_buffer_len);
}

void renderer_validate_for_mask_rendering(struct renderer *renderer,
                                          struct pipe_surface *dst,
                                          const struct matrix *modelview)
{
   struct matrix mvp = renderer->projection;

   /* will be used in POLYGON_STENCIL and POLYGON_FILL */
   matrix_mult(&mvp, modelview);
   renderer_set_mvp(renderer, &mvp);

   renderer_set_target(renderer, dst, renderer->g3d.fb.zsbuf, VG_FALSE);
   renderer_set_blend(renderer, ~0);
   renderer_set_fs(renderer, RENDERER_FS_WHITE);

   /* set internal dirty flags (hacky!) */
   renderer->dirty = FRAMEBUFFER_DIRTY | BLEND_DIRTY;
}

void renderer_copy_surface(struct renderer *ctx,
                           struct pipe_surface *src,
                           int srcX0, int srcY0,
                           int srcX1, int srcY1,
                           struct pipe_surface *dst,
                           int dstX0, int dstY0,
                           int dstX1, int dstY1,
                           float z, unsigned filter)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_sampler_view view_templ;
   struct pipe_sampler_view *view;
   struct pipe_box src_box;
   struct pipe_resource texTemp, *tex;
   const struct pipe_framebuffer_state *fb = &ctx->g3d.fb;
   const int srcW = abs(srcX1 - srcX0);
   const int srcH = abs(srcY1 - srcY0);
   const int srcLeft = MIN2(srcX0, srcX1);
   const int srcTop = MIN2(srcY0, srcY1);

   assert(filter == PIPE_TEX_MIPFILTER_NEAREST ||
          filter == PIPE_TEX_MIPFILTER_LINEAR);

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

   assert(screen->is_format_supported(screen, src->format, PIPE_TEXTURE_2D,
                                      0, PIPE_BIND_SAMPLER_VIEW));
   assert(screen->is_format_supported(screen, dst->format, PIPE_TEXTURE_2D,
                                      0, PIPE_BIND_SAMPLER_VIEW));
   assert(screen->is_format_supported(screen, dst->format, PIPE_TEXTURE_2D,
                                      0, PIPE_BIND_RENDER_TARGET));

   /*
    * XXX for now we're always creating a temporary texture.
    * Strictly speaking that's not always needed.
    */

   /* create temp texture */
   memset(&texTemp, 0, sizeof(texTemp));
   texTemp.target = PIPE_TEXTURE_2D;
   texTemp.format = src->format;
   texTemp.last_level = 0;
   texTemp.width0 = srcW;
   texTemp.height0 = srcH;
   texTemp.depth0 = 1;
   texTemp.array_size = 1;
   texTemp.bind = PIPE_BIND_SAMPLER_VIEW;

   tex = screen->resource_create(screen, &texTemp);
   if (!tex)
      return;

   u_sampler_view_default_template(&view_templ, tex, tex->format);
   view = pipe->create_sampler_view(pipe, tex, &view_templ);

   if (!view)
      return;

   u_box_2d_zslice(srcLeft, srcTop, src->u.tex.first_layer, srcW, srcH, &src_box);

   pipe->resource_copy_region(pipe,
                              tex, 0, 0, 0, 0,  /* dest */
                              src->texture, 0, &src_box);

   assert(floatsEqual(z, 0.0f));

   /* draw */
   if (fb->cbufs[0] == dst) {
      /* transform back to surface coordinates */
      dstY0 = dst->height - dstY0;
      dstY1 = dst->height - dstY1;

      if (renderer_drawtex_begin(ctx, view)) {
         renderer_drawtex(ctx,
               dstX0, dstY0, dstX1 - dstX0, dstY1 - dstY0,
               0, 0, view->texture->width0, view->texture->height0);
         renderer_drawtex_end(ctx);
      }
   }
   else {
      if (renderer_copy_begin(ctx, dst, VG_TRUE, view)) {
         renderer_copy(ctx,
               dstX0, dstY0, dstX1 - dstX0, dstY1 - dstY0,
               0, 0, view->texture->width0, view->texture->height0);
         renderer_copy_end(ctx);
      }
   }
}

void renderer_texture_quad(struct renderer *r,
                           struct pipe_resource *tex,
                           VGfloat x1offset, VGfloat y1offset,
                           VGfloat x2offset, VGfloat y2offset,
                           VGfloat x1, VGfloat y1,
                           VGfloat x2, VGfloat y2,
                           VGfloat x3, VGfloat y3,
                           VGfloat x4, VGfloat y4)
{
   const VGfloat z = 0.0f;

   assert(r->state == RENDERER_STATE_INIT);
   assert(tex->width0 != 0);
   assert(tex->height0 != 0);

   cso_save_vertex_shader(r->cso);

   renderer_set_vs(r, RENDERER_VS_TEXTURE);

   /* manually set up positions */
   r->vertices[0][0][0] = x1;
   r->vertices[0][0][1] = y1;
   r->vertices[0][0][2] = z;

   r->vertices[1][0][0] = x2;
   r->vertices[1][0][1] = y2;
   r->vertices[1][0][2] = z;

   r->vertices[2][0][0] = x3;
   r->vertices[2][0][1] = y3;
   r->vertices[2][0][2] = z;

   r->vertices[3][0][0] = x4;
   r->vertices[3][0][1] = y4;
   r->vertices[3][0][2] = z;

   /* texcoords */
   renderer_quad_texcoord(r, x1offset, y1offset,
         x2offset, y2offset, tex->width0, tex->height0);

   renderer_quad_draw(r);

   cso_restore_vertex_shader(r->cso);
}
