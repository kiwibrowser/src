/*
 * Copyright 2010 Christoph Bumiller
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_transfer.h"

#include "tgsi/tgsi_parse.h"

#include "nv50_stateobj.h"
#include "nv50_context.h"

#include "nv50_3d.xml.h"
#include "nv50_texture.xml.h"

#include "nouveau/nouveau_gldefs.h"

/* Caveats:
 *  ! pipe_sampler_state.normalized_coords is ignored - rectangle textures will
 *     use non-normalized coordinates, everything else won't
 *    (The relevant bit is in the TIC entry and not the TSC entry.)
 *
 *  ! pipe_sampler_state.seamless_cube_map is ignored - seamless filtering is
 *     always activated on NVA0 +
 *    (Give me the global bit, otherwise it's not worth the CPU work.)
 *
 *  ! pipe_sampler_state.border_color is not swizzled according to the texture
 *     swizzle in pipe_sampler_view
 *    (This will be ugly with indirect independent texture/sampler access,
 *     we'd have to emulate the logic in the shader. GL doesn't have that,
 *     D3D doesn't have swizzle, if we knew what we were implementing we'd be
 *     good.)
 *
 *  ! pipe_rasterizer_state.line_last_pixel is ignored - it is never drawn
 *
 *  ! pipe_rasterizer_state.flatshade_first also applies to QUADS
 *    (There's a GL query for that, forcing an exception is just ridiculous.)
 *
 *  ! pipe_rasterizer_state.gl_rasterization_rules is ignored - pixel centers
 *     are always at half integer coordinates and the top-left rule applies
 *    (There does not seem to be a hardware switch for this.)
 *
 *  ! pipe_rasterizer_state.sprite_coord_enable is masked with 0xff on NVC0
 *    (The hardware only has 8 slots meant for TexCoord and we have to assign
 *     in advance to maintain elegant separate shader objects.)
 */

static INLINE uint32_t
nv50_colormask(unsigned mask)
{
   uint32_t ret = 0;

   if (mask & PIPE_MASK_R)
      ret |= 0x0001;
   if (mask & PIPE_MASK_G)
      ret |= 0x0010;
   if (mask & PIPE_MASK_B)
      ret |= 0x0100;
   if (mask & PIPE_MASK_A)
      ret |= 0x1000;

   return ret;
}

#define NV50_BLEND_FACTOR_CASE(a, b) \
   case PIPE_BLENDFACTOR_##a: return NV50_3D_BLEND_FACTOR_##b

static INLINE uint32_t
nv50_blend_fac(unsigned factor)
{
   switch (factor) {
   NV50_BLEND_FACTOR_CASE(ONE, ONE);
   NV50_BLEND_FACTOR_CASE(SRC_COLOR, SRC_COLOR);
   NV50_BLEND_FACTOR_CASE(SRC_ALPHA, SRC_ALPHA);
   NV50_BLEND_FACTOR_CASE(DST_ALPHA, DST_ALPHA);
   NV50_BLEND_FACTOR_CASE(DST_COLOR, DST_COLOR);
   NV50_BLEND_FACTOR_CASE(SRC_ALPHA_SATURATE, SRC_ALPHA_SATURATE);
   NV50_BLEND_FACTOR_CASE(CONST_COLOR, CONSTANT_COLOR);
   NV50_BLEND_FACTOR_CASE(CONST_ALPHA, CONSTANT_ALPHA);
   NV50_BLEND_FACTOR_CASE(SRC1_COLOR, SRC1_COLOR);
   NV50_BLEND_FACTOR_CASE(SRC1_ALPHA, SRC1_ALPHA);
   NV50_BLEND_FACTOR_CASE(ZERO, ZERO);
   NV50_BLEND_FACTOR_CASE(INV_SRC_COLOR, ONE_MINUS_SRC_COLOR);
   NV50_BLEND_FACTOR_CASE(INV_SRC_ALPHA, ONE_MINUS_SRC_ALPHA);
   NV50_BLEND_FACTOR_CASE(INV_DST_ALPHA, ONE_MINUS_DST_ALPHA);
   NV50_BLEND_FACTOR_CASE(INV_DST_COLOR, ONE_MINUS_DST_COLOR);
   NV50_BLEND_FACTOR_CASE(INV_CONST_COLOR, ONE_MINUS_CONSTANT_COLOR);
   NV50_BLEND_FACTOR_CASE(INV_CONST_ALPHA, ONE_MINUS_CONSTANT_ALPHA);
   NV50_BLEND_FACTOR_CASE(INV_SRC1_COLOR, ONE_MINUS_SRC1_COLOR);
   NV50_BLEND_FACTOR_CASE(INV_SRC1_ALPHA, ONE_MINUS_SRC1_ALPHA);
   default:
      return NV50_3D_BLEND_FACTOR_ZERO;
   }
}

static void *
nv50_blend_state_create(struct pipe_context *pipe,
                        const struct pipe_blend_state *cso)
{
   struct nv50_blend_stateobj *so = CALLOC_STRUCT(nv50_blend_stateobj);
   int i;
   boolean emit_common_func = cso->rt[0].blend_enable;
   uint32_t ms;

   if (nv50_context(pipe)->screen->tesla->oclass >= NVA3_3D_CLASS) {
      SB_BEGIN_3D(so, BLEND_INDEPENDENT, 1);
      SB_DATA    (so, cso->independent_blend_enable);
   }

   so->pipe = *cso;

   SB_BEGIN_3D(so, COLOR_MASK_COMMON, 1);
   SB_DATA    (so, !cso->independent_blend_enable);

   SB_BEGIN_3D(so, BLEND_ENABLE_COMMON, 1);
   SB_DATA    (so, !cso->independent_blend_enable);

   if (cso->independent_blend_enable) {
      SB_BEGIN_3D(so, BLEND_ENABLE(0), 8);
      for (i = 0; i < 8; ++i) {
         SB_DATA(so, cso->rt[i].blend_enable);
         if (cso->rt[i].blend_enable)
            emit_common_func = TRUE;
      }

      if (nv50_context(pipe)->screen->tesla->oclass >= NVA3_3D_CLASS) {
         emit_common_func = FALSE;

         for (i = 0; i < 8; ++i) {
            if (!cso->rt[i].blend_enable)
               continue;
            SB_BEGIN_3D_(so, NVA3_3D_IBLEND_EQUATION_RGB(i), 6);
            SB_DATA     (so, nvgl_blend_eqn(cso->rt[i].rgb_func));
            SB_DATA     (so, nv50_blend_fac(cso->rt[i].rgb_src_factor));
            SB_DATA     (so, nv50_blend_fac(cso->rt[i].rgb_dst_factor));
            SB_DATA     (so, nvgl_blend_eqn(cso->rt[i].alpha_func));
            SB_DATA     (so, nv50_blend_fac(cso->rt[i].alpha_src_factor));
            SB_DATA     (so, nv50_blend_fac(cso->rt[i].alpha_dst_factor));
         }
      }
   } else {
      SB_BEGIN_3D(so, BLEND_ENABLE(0), 1);
      SB_DATA    (so, cso->rt[0].blend_enable);
   }

   if (emit_common_func) {
      SB_BEGIN_3D(so, BLEND_EQUATION_RGB, 5);
      SB_DATA    (so, nvgl_blend_eqn(cso->rt[0].rgb_func));
      SB_DATA    (so, nv50_blend_fac(cso->rt[0].rgb_src_factor));
      SB_DATA    (so, nv50_blend_fac(cso->rt[0].rgb_dst_factor));
      SB_DATA    (so, nvgl_blend_eqn(cso->rt[0].alpha_func));
      SB_DATA    (so, nv50_blend_fac(cso->rt[0].alpha_src_factor));
      SB_BEGIN_3D(so, BLEND_FUNC_DST_ALPHA, 1);
      SB_DATA    (so, nv50_blend_fac(cso->rt[0].alpha_dst_factor));
   }

   if (cso->logicop_enable) {
      SB_BEGIN_3D(so, LOGIC_OP_ENABLE, 2);
      SB_DATA    (so, 1);
      SB_DATA    (so, nvgl_logicop_func(cso->logicop_func));
   } else {
      SB_BEGIN_3D(so, LOGIC_OP_ENABLE, 1);
      SB_DATA    (so, 0);
   }

   if (cso->independent_blend_enable) {
      SB_BEGIN_3D(so, COLOR_MASK(0), 8);
      for (i = 0; i < 8; ++i)
         SB_DATA(so, nv50_colormask(cso->rt[i].colormask));
   } else {
      SB_BEGIN_3D(so, COLOR_MASK(0), 1);
      SB_DATA    (so, nv50_colormask(cso->rt[0].colormask));
   }

   ms = 0;
   if (cso->alpha_to_coverage)
      ms |= NV50_3D_MULTISAMPLE_CTRL_ALPHA_TO_COVERAGE;
   if (cso->alpha_to_one)
      ms |= NV50_3D_MULTISAMPLE_CTRL_ALPHA_TO_ONE;

   SB_BEGIN_3D(so, MULTISAMPLE_CTRL, 1);
   SB_DATA    (so, ms);

   assert(so->size <= (sizeof(so->state) / sizeof(so->state[0])));
   return so;
}

static void
nv50_blend_state_bind(struct pipe_context *pipe, void *hwcso)
{
   struct nv50_context *nv50 = nv50_context(pipe);

   nv50->blend = hwcso;
   nv50->dirty |= NV50_NEW_BLEND;
}

static void
nv50_blend_state_delete(struct pipe_context *pipe, void *hwcso)
{
   FREE(hwcso);
}

/* NOTE: ignoring line_last_pixel, using FALSE (set on screen init) */
static void *
nv50_rasterizer_state_create(struct pipe_context *pipe,
                             const struct pipe_rasterizer_state *cso)
{
   struct nv50_rasterizer_stateobj *so;
   uint32_t reg;

   so = CALLOC_STRUCT(nv50_rasterizer_stateobj);
   if (!so)
      return NULL;
   so->pipe = *cso;

#ifndef NV50_SCISSORS_CLIPPING
   SB_BEGIN_3D(so, SCISSOR_ENABLE(0), 1);
   SB_DATA    (so, cso->scissor);
#endif
    
   SB_BEGIN_3D(so, SHADE_MODEL, 1);
   SB_DATA    (so, cso->flatshade ? NV50_3D_SHADE_MODEL_FLAT :
                                    NV50_3D_SHADE_MODEL_SMOOTH);
   SB_BEGIN_3D(so, PROVOKING_VERTEX_LAST, 1);
   SB_DATA    (so, !cso->flatshade_first);
   SB_BEGIN_3D(so, VERTEX_TWO_SIDE_ENABLE, 1);
   SB_DATA    (so, cso->light_twoside);

   SB_BEGIN_3D(so, FRAG_COLOR_CLAMP_EN, 1);
   SB_DATA    (so, cso->clamp_fragment_color ? 0x11111111 : 0x00000000);

   SB_BEGIN_3D(so, MULTISAMPLE_ENABLE, 1);
   SB_DATA    (so, cso->multisample);

   SB_BEGIN_3D(so, LINE_WIDTH, 1);
   SB_DATA    (so, fui(cso->line_width));
   SB_BEGIN_3D(so, LINE_SMOOTH_ENABLE, 1);
   SB_DATA    (so, cso->line_smooth);

   SB_BEGIN_3D(so, LINE_STIPPLE_ENABLE, 1);
   if (cso->line_stipple_enable) {
      SB_DATA    (so, 1);
      SB_BEGIN_3D(so, LINE_STIPPLE, 1);
      SB_DATA    (so, (cso->line_stipple_pattern << 8) |
                  cso->line_stipple_factor);
   } else {
      SB_DATA    (so, 0);
   }

   if (!cso->point_size_per_vertex) {
      SB_BEGIN_3D(so, POINT_SIZE, 1);
      SB_DATA    (so, fui(cso->point_size));
   }
   SB_BEGIN_3D(so, POINT_SPRITE_ENABLE, 1);
   SB_DATA    (so, cso->point_quad_rasterization);
   SB_BEGIN_3D(so, POINT_SMOOTH_ENABLE, 1);
   SB_DATA    (so, cso->point_smooth);

   SB_BEGIN_3D(so, POLYGON_MODE_FRONT, 3);
   SB_DATA    (so, nvgl_polygon_mode(cso->fill_front));
   SB_DATA    (so, nvgl_polygon_mode(cso->fill_back));
   SB_DATA    (so, cso->poly_smooth);

   SB_BEGIN_3D(so, CULL_FACE_ENABLE, 3);
   SB_DATA    (so, cso->cull_face != PIPE_FACE_NONE);
   SB_DATA    (so, cso->front_ccw ? NV50_3D_FRONT_FACE_CCW :
                                    NV50_3D_FRONT_FACE_CW);
   switch (cso->cull_face) {
   case PIPE_FACE_FRONT_AND_BACK:
      SB_DATA(so, NV50_3D_CULL_FACE_FRONT_AND_BACK);
      break;
   case PIPE_FACE_FRONT:
      SB_DATA(so, NV50_3D_CULL_FACE_FRONT);
      break;
   case PIPE_FACE_BACK:
   default:
     SB_DATA(so, NV50_3D_CULL_FACE_BACK);
     break;
   }

   SB_BEGIN_3D(so, POLYGON_STIPPLE_ENABLE, 1);
   SB_DATA    (so, cso->poly_stipple_enable);
   SB_BEGIN_3D(so, POLYGON_OFFSET_POINT_ENABLE, 3);
   SB_DATA    (so, cso->offset_point);
   SB_DATA    (so, cso->offset_line);
   SB_DATA    (so, cso->offset_tri);

   if (cso->offset_point || cso->offset_line || cso->offset_tri) {
      SB_BEGIN_3D(so, POLYGON_OFFSET_FACTOR, 1);
      SB_DATA    (so, fui(cso->offset_scale));
      SB_BEGIN_3D(so, POLYGON_OFFSET_UNITS, 1);
      SB_DATA    (so, fui(cso->offset_units * 2.0f));
      SB_BEGIN_3D(so, POLYGON_OFFSET_CLAMP, 1);
      SB_DATA    (so, fui(cso->offset_clamp));
   }

   if (cso->depth_clip) {
      reg = 0;
   } else {
      reg =
         NV50_3D_VIEW_VOLUME_CLIP_CTRL_DEPTH_CLAMP_NEAR |
         NV50_3D_VIEW_VOLUME_CLIP_CTRL_DEPTH_CLAMP_FAR |
         NV50_3D_VIEW_VOLUME_CLIP_CTRL_UNK12_UNK1;
   }
#ifndef NV50_SCISSORS_CLIPPING
   reg |=
      NV50_3D_VIEW_VOLUME_CLIP_CTRL_UNK7 |
      NV50_3D_VIEW_VOLUME_CLIP_CTRL_UNK12_UNK1;
#endif
   SB_BEGIN_3D(so, VIEW_VOLUME_CLIP_CTRL, 1);
   SB_DATA    (so, reg);

   assert(so->size <= (sizeof(so->state) / sizeof(so->state[0])));
   return (void *)so;
}

static void
nv50_rasterizer_state_bind(struct pipe_context *pipe, void *hwcso)
{
   struct nv50_context *nv50 = nv50_context(pipe);

   nv50->rast = hwcso;
   nv50->dirty |= NV50_NEW_RASTERIZER;
}

static void
nv50_rasterizer_state_delete(struct pipe_context *pipe, void *hwcso)
{
   FREE(hwcso);
}

static void *
nv50_zsa_state_create(struct pipe_context *pipe,
                      const struct pipe_depth_stencil_alpha_state *cso)
{
   struct nv50_zsa_stateobj *so = CALLOC_STRUCT(nv50_zsa_stateobj);

   so->pipe = *cso;

   SB_BEGIN_3D(so, DEPTH_WRITE_ENABLE, 1);
   SB_DATA    (so, cso->depth.writemask);
   SB_BEGIN_3D(so, DEPTH_TEST_ENABLE, 1);
   if (cso->depth.enabled) {
      SB_DATA    (so, 1);
      SB_BEGIN_3D(so, DEPTH_TEST_FUNC, 1);
      SB_DATA    (so, nvgl_comparison_op(cso->depth.func));
   } else {
      SB_DATA    (so, 0);
   }

   if (cso->stencil[0].enabled) {
      SB_BEGIN_3D(so, STENCIL_ENABLE, 5);
      SB_DATA    (so, 1);
      SB_DATA    (so, nvgl_stencil_op(cso->stencil[0].fail_op));
      SB_DATA    (so, nvgl_stencil_op(cso->stencil[0].zfail_op));
      SB_DATA    (so, nvgl_stencil_op(cso->stencil[0].zpass_op));
      SB_DATA    (so, nvgl_comparison_op(cso->stencil[0].func));
      SB_BEGIN_3D(so, STENCIL_FRONT_MASK, 2);
      SB_DATA    (so, cso->stencil[0].writemask);
      SB_DATA    (so, cso->stencil[0].valuemask);
   } else {
      SB_BEGIN_3D(so, STENCIL_ENABLE, 1);
      SB_DATA    (so, 0);
   }

   if (cso->stencil[1].enabled) {
      assert(cso->stencil[0].enabled);
      SB_BEGIN_3D(so, STENCIL_TWO_SIDE_ENABLE, 5);
      SB_DATA    (so, 1);
      SB_DATA    (so, nvgl_stencil_op(cso->stencil[1].fail_op));
      SB_DATA    (so, nvgl_stencil_op(cso->stencil[1].zfail_op));
      SB_DATA    (so, nvgl_stencil_op(cso->stencil[1].zpass_op));
      SB_DATA    (so, nvgl_comparison_op(cso->stencil[1].func));
      SB_BEGIN_3D(so, STENCIL_BACK_MASK, 2);
      SB_DATA    (so, cso->stencil[1].writemask);
      SB_DATA    (so, cso->stencil[1].valuemask);
   } else {
      SB_BEGIN_3D(so, STENCIL_TWO_SIDE_ENABLE, 1);
      SB_DATA    (so, 0);
   }
    
   SB_BEGIN_3D(so, ALPHA_TEST_ENABLE, 1);
   if (cso->alpha.enabled) {
      SB_DATA    (so, 1);
      SB_BEGIN_3D(so, ALPHA_TEST_REF, 2);
      SB_DATA    (so, fui(cso->alpha.ref_value));
      SB_DATA    (so, nvgl_comparison_op(cso->alpha.func));
   } else {
      SB_DATA    (so, 0);
   }

   assert(so->size <= (sizeof(so->state) / sizeof(so->state[0])));
   return (void *)so;
}

static void
nv50_zsa_state_bind(struct pipe_context *pipe, void *hwcso)
{
   struct nv50_context *nv50 = nv50_context(pipe);

   nv50->zsa = hwcso;
   nv50->dirty |= NV50_NEW_ZSA;
}

static void
nv50_zsa_state_delete(struct pipe_context *pipe, void *hwcso)
{
   FREE(hwcso);
}

/* ====================== SAMPLERS AND TEXTURES ================================
 */

#define NV50_TSC_WRAP_CASE(n) \
    case PIPE_TEX_WRAP_##n: return NV50_TSC_WRAP_##n

static INLINE unsigned
nv50_tsc_wrap_mode(unsigned wrap)
{
   switch (wrap) {
   NV50_TSC_WRAP_CASE(REPEAT);
   NV50_TSC_WRAP_CASE(MIRROR_REPEAT);
   NV50_TSC_WRAP_CASE(CLAMP_TO_EDGE);
   NV50_TSC_WRAP_CASE(CLAMP_TO_BORDER);
   NV50_TSC_WRAP_CASE(CLAMP);
   NV50_TSC_WRAP_CASE(MIRROR_CLAMP_TO_EDGE);
   NV50_TSC_WRAP_CASE(MIRROR_CLAMP_TO_BORDER);
   NV50_TSC_WRAP_CASE(MIRROR_CLAMP);
   default:
       NOUVEAU_ERR("unknown wrap mode: %d\n", wrap);
       return NV50_TSC_WRAP_REPEAT;
   }
}

void *
nv50_sampler_state_create(struct pipe_context *pipe,
                          const struct pipe_sampler_state *cso)
{
   struct nv50_tsc_entry *so = CALLOC_STRUCT(nv50_tsc_entry);
   float f[2];

   so->id = -1;

   so->tsc[0] = (0x00026000 |
                 (nv50_tsc_wrap_mode(cso->wrap_s) << 0) |
                 (nv50_tsc_wrap_mode(cso->wrap_t) << 3) |
                 (nv50_tsc_wrap_mode(cso->wrap_r) << 6));

   if (nouveau_screen(pipe->screen)->class_3d >= NVE4_3D_CLASS) {
      if (cso->seamless_cube_map)
         so->tsc[1] |= NVE4_TSC_1_CUBE_SEAMLESS;
      if (!cso->normalized_coords)
         so->tsc[1] |= NVE4_TSC_1_FORCE_NONNORMALIZED_COORDS;
   }

   switch (cso->mag_img_filter) {
   case PIPE_TEX_FILTER_LINEAR:
      so->tsc[1] |= NV50_TSC_1_MAGF_LINEAR;
      break;
   case PIPE_TEX_FILTER_NEAREST:
   default:
      so->tsc[1] |= NV50_TSC_1_MAGF_NEAREST;
      break;
   }

   switch (cso->min_img_filter) {
   case PIPE_TEX_FILTER_LINEAR:
      so->tsc[1] |= NV50_TSC_1_MINF_LINEAR;
      break;
   case PIPE_TEX_FILTER_NEAREST:
   default:
      so->tsc[1] |= NV50_TSC_1_MINF_NEAREST;
      break;
   }

   switch (cso->min_mip_filter) {
   case PIPE_TEX_MIPFILTER_LINEAR:
      so->tsc[1] |= NV50_TSC_1_MIPF_LINEAR;
      break;
   case PIPE_TEX_MIPFILTER_NEAREST:
      so->tsc[1] |= NV50_TSC_1_MIPF_NEAREST;
      break;
   case PIPE_TEX_MIPFILTER_NONE:
   default:
      so->tsc[1] |= NV50_TSC_1_MIPF_NONE;
      break;
   }

   if (cso->max_anisotropy >= 16)
      so->tsc[0] |= (7 << 20);
   else
   if (cso->max_anisotropy >= 12)
      so->tsc[0] |= (6 << 20);
   else {
      so->tsc[0] |= (cso->max_anisotropy >> 1) << 20;

      if (cso->max_anisotropy >= 4)
         so->tsc[1] |= NV50_TSC_1_UNKN_ANISO_35;
      else
      if (cso->max_anisotropy >= 2)
         so->tsc[1] |= NV50_TSC_1_UNKN_ANISO_15;
   }

   if (cso->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
      /* NOTE: must be deactivated for non-shadow textures */
      so->tsc[0] |= (1 << 9);
      so->tsc[0] |= (nvgl_comparison_op(cso->compare_func) & 0x7) << 10;
   }

   f[0] = CLAMP(cso->lod_bias, -16.0f, 15.0f);
   so->tsc[1] |= ((int)(f[0] * 256.0f) & 0x1fff) << 12;

   f[0] = CLAMP(cso->min_lod, 0.0f, 15.0f);
   f[1] = CLAMP(cso->max_lod, 0.0f, 15.0f);
   so->tsc[2] |=
      (((int)(f[1] * 256.0f) & 0xfff) << 12) | ((int)(f[0] * 256.0f) & 0xfff);

   so->tsc[4] = fui(cso->border_color.f[0]);
   so->tsc[5] = fui(cso->border_color.f[1]);
   so->tsc[6] = fui(cso->border_color.f[2]);
   so->tsc[7] = fui(cso->border_color.f[3]);

   return (void *)so;
}

static void
nv50_sampler_state_delete(struct pipe_context *pipe, void *hwcso)
{
   unsigned s, i;

   for (s = 0; s < 3; ++s)
      for (i = 0; i < nv50_context(pipe)->num_samplers[s]; ++i)
         if (nv50_context(pipe)->samplers[s][i] == hwcso)
            nv50_context(pipe)->samplers[s][i] = NULL;

   nv50_screen_tsc_free(nv50_context(pipe)->screen, nv50_tsc_entry(hwcso));

   FREE(hwcso);
}

static INLINE void
nv50_stage_sampler_states_bind(struct nv50_context *nv50, int s,
                               unsigned nr, void **hwcso)
{
   unsigned i;

   for (i = 0; i < nr; ++i) {
      struct nv50_tsc_entry *old = nv50->samplers[s][i];

      nv50->samplers[s][i] = nv50_tsc_entry(hwcso[i]);
      if (old)
         nv50_screen_tsc_unlock(nv50->screen, old);
   }
   for (; i < nv50->num_samplers[s]; ++i)
      if (nv50->samplers[s][i])
         nv50_screen_tsc_unlock(nv50->screen, nv50->samplers[s][i]);

   nv50->num_samplers[s] = nr;

   nv50->dirty |= NV50_NEW_SAMPLERS;
}

static void
nv50_vp_sampler_states_bind(struct pipe_context *pipe, unsigned nr, void **s)
{
   nv50_stage_sampler_states_bind(nv50_context(pipe), 0, nr, s);
}

static void
nv50_fp_sampler_states_bind(struct pipe_context *pipe, unsigned nr, void **s)
{
   nv50_stage_sampler_states_bind(nv50_context(pipe), 2, nr, s);
}

static void
nv50_gp_sampler_states_bind(struct pipe_context *pipe, unsigned nr, void **s)
{
   nv50_stage_sampler_states_bind(nv50_context(pipe), 1, nr, s);
}

/* NOTE: only called when not referenced anywhere, won't be bound */
static void
nv50_sampler_view_destroy(struct pipe_context *pipe,
                          struct pipe_sampler_view *view)
{
   pipe_resource_reference(&view->texture, NULL);

   nv50_screen_tic_free(nv50_context(pipe)->screen, nv50_tic_entry(view));

   FREE(nv50_tic_entry(view));
}

static INLINE void
nv50_stage_set_sampler_views(struct nv50_context *nv50, int s,
                             unsigned nr,
                             struct pipe_sampler_view **views)
{
   unsigned i;

   for (i = 0; i < nr; ++i) {
      struct nv50_tic_entry *old = nv50_tic_entry(nv50->textures[s][i]);
      if (old)
         nv50_screen_tic_unlock(nv50->screen, old);

      pipe_sampler_view_reference(&nv50->textures[s][i], views[i]);
   }

   for (i = nr; i < nv50->num_textures[s]; ++i) {
      struct nv50_tic_entry *old = nv50_tic_entry(nv50->textures[s][i]);
      if (!old)
         continue;
      nv50_screen_tic_unlock(nv50->screen, old);

      pipe_sampler_view_reference(&nv50->textures[s][i], NULL);
   }

   nv50->num_textures[s] = nr;

   nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_TEXTURES);

   nv50->dirty |= NV50_NEW_TEXTURES;
}

static void
nv50_vp_set_sampler_views(struct pipe_context *pipe,
                          unsigned nr,
                          struct pipe_sampler_view **views)
{
   nv50_stage_set_sampler_views(nv50_context(pipe), 0, nr, views);
}

static void
nv50_fp_set_sampler_views(struct pipe_context *pipe,
                          unsigned nr,
                          struct pipe_sampler_view **views)
{
   nv50_stage_set_sampler_views(nv50_context(pipe), 2, nr, views);
}

static void
nv50_gp_set_sampler_views(struct pipe_context *pipe,
                          unsigned nr,
                          struct pipe_sampler_view **views)
{
   nv50_stage_set_sampler_views(nv50_context(pipe), 1, nr, views);
}

/* ============================= SHADERS =======================================
 */

static void *
nv50_sp_state_create(struct pipe_context *pipe,
                     const struct pipe_shader_state *cso, unsigned type)
{
   struct nv50_program *prog;

   prog = CALLOC_STRUCT(nv50_program);
   if (!prog)
      return NULL;

   prog->type = type;
   prog->pipe.tokens = tgsi_dup_tokens(cso->tokens);

   if (cso->stream_output.num_outputs)
      prog->pipe.stream_output = cso->stream_output;

   return (void *)prog;
}

static void
nv50_sp_state_delete(struct pipe_context *pipe, void *hwcso)
{
   struct nv50_program *prog = (struct nv50_program *)hwcso;

   nv50_program_destroy(nv50_context(pipe), prog);

   FREE((void *)prog->pipe.tokens);
   FREE(prog);
}

static void *
nv50_vp_state_create(struct pipe_context *pipe,
                     const struct pipe_shader_state *cso)
{
   return nv50_sp_state_create(pipe, cso, PIPE_SHADER_VERTEX);
}

static void
nv50_vp_state_bind(struct pipe_context *pipe, void *hwcso)
{
    struct nv50_context *nv50 = nv50_context(pipe);

    nv50->vertprog = hwcso;
    nv50->dirty |= NV50_NEW_VERTPROG;
}

static void *
nv50_fp_state_create(struct pipe_context *pipe,
                     const struct pipe_shader_state *cso)
{
   return nv50_sp_state_create(pipe, cso, PIPE_SHADER_FRAGMENT);
}

static void
nv50_fp_state_bind(struct pipe_context *pipe, void *hwcso)
{
    struct nv50_context *nv50 = nv50_context(pipe);

    nv50->fragprog = hwcso;
    nv50->dirty |= NV50_NEW_FRAGPROG;
}

static void *
nv50_gp_state_create(struct pipe_context *pipe,
                     const struct pipe_shader_state *cso)
{
   return nv50_sp_state_create(pipe, cso, PIPE_SHADER_GEOMETRY);
}

static void
nv50_gp_state_bind(struct pipe_context *pipe, void *hwcso)
{
    struct nv50_context *nv50 = nv50_context(pipe);

    nv50->gmtyprog = hwcso;
    nv50->dirty |= NV50_NEW_GMTYPROG;
}

static void
nv50_set_constant_buffer(struct pipe_context *pipe, uint shader, uint index,
                         struct pipe_constant_buffer *cb)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct pipe_resource *res = cb ? cb->buffer : NULL;
   const unsigned s = nv50_context_shader_stage(shader);
   const unsigned i = index;

   if (shader == PIPE_SHADER_COMPUTE)
      return;

   if (nv50->constbuf[s][i].user)
      nv50->constbuf[s][i].u.buf = NULL;
   else
   if (nv50->constbuf[s][i].u.buf)
      nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_CB(s, i));

   pipe_resource_reference(&nv50->constbuf[s][i].u.buf, res);

   nv50->constbuf[s][i].user = (cb && cb->user_buffer) ? TRUE : FALSE;
   if (nv50->constbuf[s][i].user) {
      nv50->constbuf[s][i].u.data = cb->user_buffer;
      nv50->constbuf[s][i].size = cb->buffer_size;
      nv50->constbuf_valid[s] |= 1 << i;
   } else
   if (res) {
      nv50->constbuf[s][i].offset = cb->buffer_offset;
      nv50->constbuf[s][i].size = align(cb->buffer_size, 0x100);
      nv50->constbuf_valid[s] |= 1 << i;
   } else {
      nv50->constbuf_valid[s] &= ~(1 << i);
   }
   nv50->constbuf_dirty[s] |= 1 << i;

   nv50->dirty |= NV50_NEW_CONSTBUF;
}

/* =============================================================================
 */

static void
nv50_set_blend_color(struct pipe_context *pipe,
                     const struct pipe_blend_color *bcol)
{
   struct nv50_context *nv50 = nv50_context(pipe);

   nv50->blend_colour = *bcol;
   nv50->dirty |= NV50_NEW_BLEND_COLOUR;
}

static void
nv50_set_stencil_ref(struct pipe_context *pipe,
                     const struct pipe_stencil_ref *sr)
{
   struct nv50_context *nv50 = nv50_context(pipe);

   nv50->stencil_ref = *sr;
   nv50->dirty |= NV50_NEW_STENCIL_REF;
}

static void
nv50_set_clip_state(struct pipe_context *pipe,
                    const struct pipe_clip_state *clip)
{
   struct nv50_context *nv50 = nv50_context(pipe);

   memcpy(nv50->clip.ucp, clip->ucp, sizeof(clip->ucp));

   nv50->dirty |= NV50_NEW_CLIP;
}

static void
nv50_set_sample_mask(struct pipe_context *pipe, unsigned sample_mask)
{
   struct nv50_context *nv50 = nv50_context(pipe);

   nv50->sample_mask = sample_mask;
   nv50->dirty |= NV50_NEW_SAMPLE_MASK;
}


static void
nv50_set_framebuffer_state(struct pipe_context *pipe,
                           const struct pipe_framebuffer_state *fb)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   unsigned i;

   nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_FB);

   for (i = 0; i < fb->nr_cbufs; ++i)
      pipe_surface_reference(&nv50->framebuffer.cbufs[i], fb->cbufs[i]);
   for (; i < nv50->framebuffer.nr_cbufs; ++i)
      pipe_surface_reference(&nv50->framebuffer.cbufs[i], NULL);

   nv50->framebuffer.nr_cbufs = fb->nr_cbufs;

   nv50->framebuffer.width = fb->width;
   nv50->framebuffer.height = fb->height;

   pipe_surface_reference(&nv50->framebuffer.zsbuf, fb->zsbuf);

   nv50->dirty |= NV50_NEW_FRAMEBUFFER;
}

static void
nv50_set_polygon_stipple(struct pipe_context *pipe,
                         const struct pipe_poly_stipple *stipple)
{
   struct nv50_context *nv50 = nv50_context(pipe);

   nv50->stipple = *stipple;
   nv50->dirty |= NV50_NEW_STIPPLE;
}

static void
nv50_set_scissor_state(struct pipe_context *pipe,
                       const struct pipe_scissor_state *scissor)
{
   struct nv50_context *nv50 = nv50_context(pipe);

   nv50->scissor = *scissor;
   nv50->dirty |= NV50_NEW_SCISSOR;
}

static void
nv50_set_viewport_state(struct pipe_context *pipe,
                        const struct pipe_viewport_state *vpt)
{
   struct nv50_context *nv50 = nv50_context(pipe);

   nv50->viewport = *vpt;
   nv50->dirty |= NV50_NEW_VIEWPORT;
}

static void
nv50_set_vertex_buffers(struct pipe_context *pipe,
                        unsigned count,
                        const struct pipe_vertex_buffer *vb)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   unsigned i;

   nv50->vbo_user = nv50->vbo_constant = 0;

   for (i = 0; i < count; ++i) {
      nv50->vtxbuf[i].stride = vb[i].stride;
      pipe_resource_reference(&nv50->vtxbuf[i].buffer, vb[i].buffer);
      if (!vb[i].buffer && vb[i].user_buffer) {
         nv50->vtxbuf[i].user_buffer = vb[i].user_buffer;
         nv50->vbo_user |= 1 << i;
         if (!vb[i].stride)
            nv50->vbo_constant |= 1 << i;
      } else {
         nv50->vtxbuf[i].buffer_offset = vb[i].buffer_offset;
      }
   }
   for (; i < nv50->num_vtxbufs; ++i)
      pipe_resource_reference(&nv50->vtxbuf[i].buffer, NULL);

   nv50->num_vtxbufs = count;

   nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_VERTEX);

   nv50->dirty |= NV50_NEW_ARRAYS;
}

static void
nv50_set_index_buffer(struct pipe_context *pipe,
                      const struct pipe_index_buffer *ib)
{
   struct nv50_context *nv50 = nv50_context(pipe);

   if (nv50->idxbuf.buffer)
      nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_INDEX);

   if (ib) {
      pipe_resource_reference(&nv50->idxbuf.buffer, ib->buffer);
      nv50->idxbuf.index_size = ib->index_size;
      if (ib->buffer) {
         nv50->idxbuf.offset = ib->offset;
         BCTX_REFN(nv50->bufctx_3d, INDEX, nv04_resource(ib->buffer), RD);
      } else {
         nv50->idxbuf.user_buffer = ib->user_buffer;
      }
   } else {
      pipe_resource_reference(&nv50->idxbuf.buffer, NULL);
   }
}

static void
nv50_vertex_state_bind(struct pipe_context *pipe, void *hwcso)
{
   struct nv50_context *nv50 = nv50_context(pipe);

   nv50->vertex = hwcso;
   nv50->dirty |= NV50_NEW_VERTEX;
}

static struct pipe_stream_output_target *
nv50_so_target_create(struct pipe_context *pipe,
                      struct pipe_resource *res,
                      unsigned offset, unsigned size)
{
   struct nv50_so_target *targ = MALLOC_STRUCT(nv50_so_target);
   if (!targ)
      return NULL;

   if (nouveau_context(pipe)->screen->class_3d >= NVA0_3D_CLASS) {
      targ->pq = pipe->create_query(pipe,
                                    NVA0_QUERY_STREAM_OUTPUT_BUFFER_OFFSET);
      if (!targ->pq) {
         FREE(targ);
         return NULL;
      }
   } else {
      targ->pq = NULL;
   }
   targ->clean = TRUE;

   targ->pipe.buffer_size = size;
   targ->pipe.buffer_offset = offset;
   targ->pipe.context = pipe;
   targ->pipe.buffer = NULL;
   pipe_resource_reference(&targ->pipe.buffer, res);
   pipe_reference_init(&targ->pipe.reference, 1);

   return &targ->pipe;
}

static void
nv50_so_target_destroy(struct pipe_context *pipe,
                       struct pipe_stream_output_target *ptarg)
{
   struct nv50_so_target *targ = nv50_so_target(ptarg);
   if (targ->pq)
      pipe->destroy_query(pipe, targ->pq);
   pipe_resource_reference(&targ->pipe.buffer, NULL);
   FREE(targ);
}

static void
nv50_set_stream_output_targets(struct pipe_context *pipe,
                               unsigned num_targets,
                               struct pipe_stream_output_target **targets,
                               unsigned append_mask)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   unsigned i;
   boolean serialize = TRUE;
   const boolean can_resume = nv50->screen->base.class_3d >= NVA0_3D_CLASS;

   assert(num_targets <= 4);

   for (i = 0; i < num_targets; ++i) {
      const boolean changed = nv50->so_target[i] != targets[i];
      if (!changed && (append_mask & (1 << i)))
         continue;
      nv50->so_targets_dirty |= 1 << i;

      if (can_resume && changed && nv50->so_target[i]) {
         nva0_so_target_save_offset(pipe, nv50->so_target[i], i, serialize);
         serialize = FALSE;
      }

      if (targets[i] && !(append_mask & (1 << i)))
         nv50_so_target(targets[i])->clean = TRUE;

      pipe_so_target_reference(&nv50->so_target[i], targets[i]);
   }
   for (; i < nv50->num_so_targets; ++i) {
      if (can_resume && nv50->so_target[i]) {
         nva0_so_target_save_offset(pipe, nv50->so_target[i], i, serialize);
         serialize = FALSE;
      }
      pipe_so_target_reference(&nv50->so_target[i], NULL);
      nv50->so_targets_dirty |= 1 << i;
   }
   nv50->num_so_targets = num_targets;

   if (nv50->so_targets_dirty)
      nv50->dirty |= NV50_NEW_STRMOUT;
}

void
nv50_init_state_functions(struct nv50_context *nv50)
{
   struct pipe_context *pipe = &nv50->base.pipe;

   pipe->create_blend_state = nv50_blend_state_create;
   pipe->bind_blend_state = nv50_blend_state_bind;
   pipe->delete_blend_state = nv50_blend_state_delete;

   pipe->create_rasterizer_state = nv50_rasterizer_state_create;
   pipe->bind_rasterizer_state = nv50_rasterizer_state_bind;
   pipe->delete_rasterizer_state = nv50_rasterizer_state_delete;

   pipe->create_depth_stencil_alpha_state = nv50_zsa_state_create;
   pipe->bind_depth_stencil_alpha_state = nv50_zsa_state_bind;
   pipe->delete_depth_stencil_alpha_state = nv50_zsa_state_delete;

   pipe->create_sampler_state = nv50_sampler_state_create;
   pipe->delete_sampler_state = nv50_sampler_state_delete;
   pipe->bind_vertex_sampler_states   = nv50_vp_sampler_states_bind;
   pipe->bind_fragment_sampler_states = nv50_fp_sampler_states_bind;
   pipe->bind_geometry_sampler_states = nv50_gp_sampler_states_bind;

   pipe->create_sampler_view = nv50_create_sampler_view;
   pipe->sampler_view_destroy = nv50_sampler_view_destroy;
   pipe->set_vertex_sampler_views   = nv50_vp_set_sampler_views;
   pipe->set_fragment_sampler_views = nv50_fp_set_sampler_views;
   pipe->set_geometry_sampler_views = nv50_gp_set_sampler_views;
 
   pipe->create_vs_state = nv50_vp_state_create;
   pipe->create_fs_state = nv50_fp_state_create;
   pipe->create_gs_state = nv50_gp_state_create;
   pipe->bind_vs_state = nv50_vp_state_bind;
   pipe->bind_fs_state = nv50_fp_state_bind;
   pipe->bind_gs_state = nv50_gp_state_bind;
   pipe->delete_vs_state = nv50_sp_state_delete;
   pipe->delete_fs_state = nv50_sp_state_delete;
   pipe->delete_gs_state = nv50_sp_state_delete;

   pipe->set_blend_color = nv50_set_blend_color;
   pipe->set_stencil_ref = nv50_set_stencil_ref;
   pipe->set_clip_state = nv50_set_clip_state;
   pipe->set_sample_mask = nv50_set_sample_mask;
   pipe->set_constant_buffer = nv50_set_constant_buffer;
   pipe->set_framebuffer_state = nv50_set_framebuffer_state;
   pipe->set_polygon_stipple = nv50_set_polygon_stipple;
   pipe->set_scissor_state = nv50_set_scissor_state;
   pipe->set_viewport_state = nv50_set_viewport_state;

   pipe->create_vertex_elements_state = nv50_vertex_state_create;
   pipe->delete_vertex_elements_state = nv50_vertex_state_delete;
   pipe->bind_vertex_elements_state = nv50_vertex_state_bind;

   pipe->set_vertex_buffers = nv50_set_vertex_buffers;
   pipe->set_index_buffer = nv50_set_index_buffer;

   pipe->create_stream_output_target = nv50_so_target_create;
   pipe->stream_output_target_destroy = nv50_so_target_destroy;
   pipe->set_stream_output_targets = nv50_set_stream_output_targets;
}
