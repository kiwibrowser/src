/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>

#include "intel_batchbuffer.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"

#include "brw_blorp.h"
#include "gen7_blorp.h"


/* 3DSTATE_URB_VS
 * 3DSTATE_URB_HS
 * 3DSTATE_URB_DS
 * 3DSTATE_URB_GS
 *
 * If the 3DSTATE_URB_VS is emitted, than the others must be also. From the
 * BSpec, Volume 2a "3D Pipeline Overview", Section 1.7.1 3DSTATE_URB_VS:
 *     3DSTATE_URB_HS, 3DSTATE_URB_DS, and 3DSTATE_URB_GS must also be
 *     programmed in order for the programming of this state to be
 *     valid.
 */
static void
gen7_blorp_emit_urb_config(struct brw_context *brw,
                           const brw_blorp_params *params)
{
   /* The minimum valid value is 32. See 3DSTATE_URB_VS,
    * Dword 1.15:0 "VS Number of URB Entries".
    */
   int num_vs_entries = 32;
   int vs_size = 2;
   int vs_start = 2; /* skip over push constants */

   gen7_emit_urb_state(brw, num_vs_entries, vs_size, vs_start);
}


/* 3DSTATE_BLEND_STATE_POINTERS */
static void
gen7_blorp_emit_blend_state_pointer(struct brw_context *brw,
                                    const brw_blorp_params *params,
                                    uint32_t cc_blend_state_offset)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_BLEND_STATE_POINTERS << 16 | (2 - 2));
   OUT_BATCH(cc_blend_state_offset | 1);
   ADVANCE_BATCH();
}


/* 3DSTATE_CC_STATE_POINTERS */
static void
gen7_blorp_emit_cc_state_pointer(struct brw_context *brw,
                                 const brw_blorp_params *params,
                                 uint32_t cc_state_offset)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_CC_STATE_POINTERS << 16 | (2 - 2));
   OUT_BATCH(cc_state_offset | 1);
   ADVANCE_BATCH();
}

static void
gen7_blorp_emit_cc_viewport(struct brw_context *brw,
			    const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;
   struct brw_cc_viewport *ccv;
   uint32_t cc_vp_offset;

   ccv = (struct brw_cc_viewport *)brw_state_batch(brw, AUB_TRACE_CC_VP_STATE,
						   sizeof(*ccv), 32,
						   &cc_vp_offset);
   ccv->min_depth = 0.0;
   ccv->max_depth = 1.0;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_VIEWPORT_STATE_POINTERS_CC << 16 | (2 - 2));
   OUT_BATCH(cc_vp_offset);
   ADVANCE_BATCH();
}


/* 3DSTATE_DEPTH_STENCIL_STATE_POINTERS
 *
 * The offset is relative to CMD_STATE_BASE_ADDRESS.DynamicStateBaseAddress.
 */
static void
gen7_blorp_emit_depth_stencil_state_pointers(struct brw_context *brw,
                                             const brw_blorp_params *params,
                                             uint32_t depthstencil_offset)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_DEPTH_STENCIL_STATE_POINTERS << 16 | (2 - 2));
   OUT_BATCH(depthstencil_offset | 1);
   ADVANCE_BATCH();
}


/* SURFACE_STATE for renderbuffer or texture surface (see
 * brw_update_renderbuffer_surface and brw_update_texture_surface)
 */
static uint32_t
gen7_blorp_emit_surface_state(struct brw_context *brw,
                              const brw_blorp_params *params,
                              const brw_blorp_surface_info *surface,
                              uint32_t read_domains, uint32_t write_domain,
                              bool is_render_target)
{
   struct intel_context *intel = &brw->intel;

   uint32_t wm_surf_offset;
   uint32_t width = surface->width;
   uint32_t height = surface->height;
   /* Note: since gen7 uses INTEL_MSAA_LAYOUT_CMS or INTEL_MSAA_LAYOUT_UMS for
    * color surfaces, width and height are measured in pixels; we don't need
    * to divide them by 2 as we do for Gen6 (see
    * gen6_blorp_emit_surface_state).
    */
   struct intel_region *region = surface->mt->region;
   uint32_t tile_x, tile_y;

   struct gen7_surface_state *surf = (struct gen7_surface_state *)
      brw_state_batch(brw, AUB_TRACE_SURFACE_STATE, sizeof(*surf), 32,
                      &wm_surf_offset);
   memset(surf, 0, sizeof(*surf));

   if (surface->mt->align_h == 4)
      surf->ss0.vertical_alignment = 1;
   if (surface->mt->align_w == 8)
      surf->ss0.horizontal_alignment = 1;

   surf->ss0.surface_format = surface->brw_surfaceformat;
   surf->ss0.surface_type = BRW_SURFACE_2D;
   surf->ss0.surface_array_spacing = surface->array_spacing_lod0 ?
      GEN7_SURFACE_ARYSPC_LOD0 : GEN7_SURFACE_ARYSPC_FULL;

   /* reloc */
   surf->ss1.base_addr = surface->compute_tile_offsets(&tile_x, &tile_y);
   surf->ss1.base_addr += region->bo->offset;

   /* Note that the low bits of these fields are missing, so
    * there's the possibility of getting in trouble.
    */
   assert(tile_x % 4 == 0);
   assert(tile_y % 2 == 0);
   surf->ss5.x_offset = tile_x / 4;
   surf->ss5.y_offset = tile_y / 2;

   surf->ss2.width = width - 1;
   surf->ss2.height = height - 1;

   uint32_t tiling = surface->map_stencil_as_y_tiled
      ? I915_TILING_Y : region->tiling;
   gen7_set_surface_tiling(surf, tiling);

   uint32_t pitch_bytes = region->pitch * region->cpp;
   if (surface->map_stencil_as_y_tiled)
      pitch_bytes *= 2;
   surf->ss3.pitch = pitch_bytes - 1;

   gen7_set_surface_msaa(surf, surface->num_samples, surface->msaa_layout);
   if (surface->msaa_layout == INTEL_MSAA_LAYOUT_CMS) {
      gen7_set_surface_mcs_info(brw, surf, wm_surf_offset,
                                surface->mt->mcs_mt, is_render_target);
   }

   if (intel->is_haswell) {
      surf->ss7.shader_channel_select_r = HSW_SCS_RED;
      surf->ss7.shader_channel_select_g = HSW_SCS_GREEN;
      surf->ss7.shader_channel_select_b = HSW_SCS_BLUE;
      surf->ss7.shader_channel_select_a = HSW_SCS_ALPHA;
   }

   /* Emit relocation to surface contents */
   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
                           wm_surf_offset +
                           offsetof(struct gen7_surface_state, ss1),
                           region->bo,
                           surf->ss1.base_addr - region->bo->offset,
                           read_domains, write_domain);

   gen7_check_surface_setup(surf, is_render_target);

   return wm_surf_offset;
}


/**
 * SAMPLER_STATE.  See gen7_update_sampler_state().
 */
static uint32_t
gen7_blorp_emit_sampler_state(struct brw_context *brw,
                              const brw_blorp_params *params)
{
   uint32_t sampler_offset;

   struct gen7_sampler_state *sampler = (struct gen7_sampler_state *)
      brw_state_batch(brw, AUB_TRACE_SAMPLER_STATE,
                      sizeof(struct gen7_sampler_state),
                      32, &sampler_offset);
   memset(sampler, 0, sizeof(*sampler));

   sampler->ss0.min_filter = BRW_MAPFILTER_LINEAR;
   sampler->ss0.mip_filter = BRW_MIPFILTER_NONE;
   sampler->ss0.mag_filter = BRW_MAPFILTER_LINEAR;

   sampler->ss3.r_wrap_mode = BRW_TEXCOORDMODE_CLAMP;
   sampler->ss3.s_wrap_mode = BRW_TEXCOORDMODE_CLAMP;
   sampler->ss3.t_wrap_mode = BRW_TEXCOORDMODE_CLAMP;

   //   sampler->ss0.min_mag_neq = 1;

   /* Set LOD bias: 
    */
   sampler->ss0.lod_bias = 0;

   sampler->ss0.lod_preclamp = 1; /* OpenGL mode */
   sampler->ss0.default_color_mode = 0; /* OpenGL/DX10 mode */

   /* Set BaseMipLevel, MaxLOD, MinLOD: 
    *
    * XXX: I don't think that using firstLevel, lastLevel works,
    * because we always setup the surface state as if firstLevel ==
    * level zero.  Probably have to subtract firstLevel from each of
    * these:
    */
   sampler->ss0.base_level = U_FIXED(0, 1);

   sampler->ss1.max_lod = U_FIXED(0, 8);
   sampler->ss1.min_lod = U_FIXED(0, 8);

   sampler->ss3.non_normalized_coord = 1;

   sampler->ss3.address_round |= BRW_ADDRESS_ROUNDING_ENABLE_U_MIN |
      BRW_ADDRESS_ROUNDING_ENABLE_V_MIN |
      BRW_ADDRESS_ROUNDING_ENABLE_R_MIN;
   sampler->ss3.address_round |= BRW_ADDRESS_ROUNDING_ENABLE_U_MAG |
      BRW_ADDRESS_ROUNDING_ENABLE_V_MAG |
      BRW_ADDRESS_ROUNDING_ENABLE_R_MAG;

   return sampler_offset;
}


/* 3DSTATE_HS
 *
 * Disable the hull shader.
 */
static void
gen7_blorp_emit_hs_disable(struct brw_context *brw,
                           const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(7);
   OUT_BATCH(_3DSTATE_HS << 16 | (7 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/* 3DSTATE_TE
 *
 * Disable the tesselation engine.
 */
static void
gen7_blorp_emit_te_disable(struct brw_context *brw,
                           const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_TE << 16 | (4 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/* 3DSTATE_DS
 *
 * Disable the domain shader.
 */
static void
gen7_blorp_emit_ds_disable(struct brw_context *brw,
                           const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(6);
   OUT_BATCH(_3DSTATE_DS << 16 | (6 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/* 3DSTATE_STREAMOUT
 *
 * Disable streamout.
 */
static void
gen7_blorp_emit_streamout_disable(struct brw_context *brw,
                                  const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(3);
   OUT_BATCH(_3DSTATE_STREAMOUT << 16 | (3 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


static void
gen7_blorp_emit_sf_config(struct brw_context *brw,
                          const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   /* 3DSTATE_SF
    *
    * Disable ViewportTransformEnable (dw1.1)
    *
    * From the SandyBridge PRM, Volume 2, Part 1, Section 1.3, "3D
    * Primitives Overview":
    *     RECTLIST: Viewport Mapping must be DISABLED (as is typical with the
    *     use of screen- space coordinates).
    *
    * A solid rectangle must be rendered, so set FrontFaceFillMode (dw1.6:5)
    * and BackFaceFillMode (dw1.4:3) to SOLID(0).
    *
    * From the Sandy Bridge PRM, Volume 2, Part 1, Section
    * 6.4.1.1 3DSTATE_SF, Field FrontFaceFillMode:
    *     SOLID: Any triangle or rectangle object found to be front-facing
    *     is rendered as a solid object. This setting is required when
    *     (rendering rectangle (RECTLIST) objects.
    */
   {
      BEGIN_BATCH(7);
      OUT_BATCH(_3DSTATE_SF << 16 | (7 - 2));
      OUT_BATCH(params->depth_format <<
                GEN7_SF_DEPTH_BUFFER_SURFACE_FORMAT_SHIFT);
      OUT_BATCH(params->num_samples > 1 ? GEN6_SF_MSRAST_ON_PATTERN : 0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_SBE */
   {
      BEGIN_BATCH(14);
      OUT_BATCH(_3DSTATE_SBE << 16 | (14 - 2));
      OUT_BATCH((1 - 1) << GEN7_SBE_NUM_OUTPUTS_SHIFT | /* only position */
                1 << GEN7_SBE_URB_ENTRY_READ_LENGTH_SHIFT |
                0 << GEN7_SBE_URB_ENTRY_READ_OFFSET_SHIFT);
      for (int i = 0; i < 12; ++i)
         OUT_BATCH(0);
      ADVANCE_BATCH();
   }
}


/**
 * Disable thread dispatch (dw5.19) and enable the HiZ op.
 */
static void
gen7_blorp_emit_wm_config(struct brw_context *brw,
                          const brw_blorp_params *params,
                          brw_blorp_prog_data *prog_data)
{
   struct intel_context *intel = &brw->intel;

   uint32_t dw1 = 0, dw2 = 0;

   switch (params->hiz_op) {
   case GEN6_HIZ_OP_DEPTH_CLEAR:
      dw1 |= GEN7_WM_DEPTH_CLEAR;
      break;
   case GEN6_HIZ_OP_DEPTH_RESOLVE:
      dw1 |= GEN7_WM_DEPTH_RESOLVE;
      break;
   case GEN6_HIZ_OP_HIZ_RESOLVE:
      dw1 |= GEN7_WM_HIERARCHICAL_DEPTH_RESOLVE;
      break;
   case GEN6_HIZ_OP_NONE:
      break;
   default:
      assert(0);
      break;
   }
   dw1 |= GEN7_WM_STATISTICS_ENABLE;
   dw1 |= GEN7_WM_LINE_AA_WIDTH_1_0;
   dw1 |= GEN7_WM_LINE_END_CAP_AA_WIDTH_0_5;
   dw1 |= 0 << GEN7_WM_BARYCENTRIC_INTERPOLATION_MODE_SHIFT; /* No interp */
   if (params->use_wm_prog) {
      dw1 |= GEN7_WM_KILL_ENABLE; /* TODO: temporarily smash on */
      dw1 |= GEN7_WM_DISPATCH_ENABLE; /* We are rendering */
   }

      if (params->num_samples > 1) {
         dw1 |= GEN7_WM_MSRAST_ON_PATTERN;
         if (prog_data && prog_data->persample_msaa_dispatch)
            dw2 |= GEN7_WM_MSDISPMODE_PERSAMPLE;
         else
            dw2 |= GEN7_WM_MSDISPMODE_PERPIXEL;
      } else {
         dw1 |= GEN7_WM_MSRAST_OFF_PIXEL;
         dw2 |= GEN7_WM_MSDISPMODE_PERSAMPLE;
      }

   BEGIN_BATCH(3);
   OUT_BATCH(_3DSTATE_WM << 16 | (3 - 2));
   OUT_BATCH(dw1);
   OUT_BATCH(dw2);
   ADVANCE_BATCH();
}


/**
 * 3DSTATE_PS
 *
 * Pixel shader dispatch is disabled above in 3DSTATE_WM, dw1.29. Despite
 * that, thread dispatch info must still be specified.
 *     - Maximum Number of Threads (dw4.24:31) must be nonzero, as the BSpec
 *       states that the valid range for this field is [0x3, 0x2f].
 *     - A dispatch mode must be given; that is, at least one of the
 *       "N Pixel Dispatch Enable" (N=8,16,32) fields must be set. This was
 *       discovered through simulator error messages.
 */
static void
gen7_blorp_emit_ps_config(struct brw_context *brw,
                          const brw_blorp_params *params,
                          uint32_t prog_offset,
                          brw_blorp_prog_data *prog_data)
{
   struct intel_context *intel = &brw->intel;
   uint32_t dw2, dw4, dw5;
   const int max_threads_shift = brw->intel.is_haswell ?
      HSW_PS_MAX_THREADS_SHIFT : IVB_PS_MAX_THREADS_SHIFT;

   dw2 = dw4 = dw5 = 0;
   dw4 |= (brw->max_wm_threads - 1) << max_threads_shift;

   /* If there's a WM program, we need to do 16-pixel dispatch since that's
    * what the program is compiled for.  If there isn't, then it shouldn't
    * matter because no program is actually being run.  However, the hardware
    * gets angry if we don't enable at least one dispatch mode, so just enable
    * 16-pixel dispatch unconditionally.
    */
   dw4 |= GEN7_PS_16_DISPATCH_ENABLE;

   if (intel->is_haswell)
      dw4 |= SET_FIELD(1, HSW_PS_SAMPLE_MASK); /* 1 sample for now */
   if (params->use_wm_prog) {
      dw2 |= 1 << GEN7_PS_SAMPLER_COUNT_SHIFT; /* Up to 4 samplers */
      dw4 |= GEN7_PS_PUSH_CONSTANT_ENABLE;
      dw5 |= prog_data->first_curbe_grf << GEN7_PS_DISPATCH_START_GRF_SHIFT_0;
   }

   BEGIN_BATCH(8);
   OUT_BATCH(_3DSTATE_PS << 16 | (8 - 2));
   OUT_BATCH(params->use_wm_prog ? prog_offset : 0);
   OUT_BATCH(dw2);
   OUT_BATCH(0);
   OUT_BATCH(dw4);
   OUT_BATCH(dw5);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


static void
gen7_blorp_emit_binding_table_pointers_ps(struct brw_context *brw,
                                          const brw_blorp_params *params,
                                          uint32_t wm_bind_bo_offset)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_BINDING_TABLE_POINTERS_PS << 16 | (2 - 2));
   OUT_BATCH(wm_bind_bo_offset);
   ADVANCE_BATCH();
}


static void
gen7_blorp_emit_sampler_state_pointers_ps(struct brw_context *brw,
                                          const brw_blorp_params *params,
                                          uint32_t sampler_offset)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_SAMPLER_STATE_POINTERS_PS << 16 | (2 - 2));
   OUT_BATCH(sampler_offset);
   ADVANCE_BATCH();
}


static void
gen7_blorp_emit_constant_ps(struct brw_context *brw,
                            const brw_blorp_params *params,
                            uint32_t wm_push_const_offset)
{
   struct intel_context *intel = &brw->intel;

   /* Make sure the push constants fill an exact integer number of
    * registers.
    */
   assert(sizeof(brw_blorp_wm_push_constants) % 32 == 0);

   /* There must be at least one register worth of push constant data. */
   assert(BRW_BLORP_NUM_PUSH_CONST_REGS > 0);

   /* Enable push constant buffer 0. */
   BEGIN_BATCH(7);
   OUT_BATCH(_3DSTATE_CONSTANT_PS << 16 |
             (7 - 2));
   OUT_BATCH(BRW_BLORP_NUM_PUSH_CONST_REGS);
   OUT_BATCH(0);
   OUT_BATCH(wm_push_const_offset);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


static void
gen7_blorp_emit_depth_stencil_config(struct brw_context *brw,
                                     const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;
   uint32_t draw_x = params->depth.x_offset;
   uint32_t draw_y = params->depth.y_offset;
   uint32_t tile_mask_x, tile_mask_y;

   gen6_blorp_compute_tile_masks(params, &tile_mask_x, &tile_mask_y);

   /* 3DSTATE_DEPTH_BUFFER */
   {
      uint32_t tile_x = draw_x & tile_mask_x;
      uint32_t tile_y = draw_y & tile_mask_y;
      uint32_t offset =
         intel_region_get_aligned_offset(params->depth.mt->region,
                                         draw_x & ~tile_mask_x,
                                         draw_y & ~tile_mask_y, false);

      /* According to the Sandy Bridge PRM, volume 2 part 1, pp326-327
       * (3DSTATE_DEPTH_BUFFER dw5), in the documentation for "Depth
       * Coordinate Offset X/Y":
       *
       *   "The 3 LSBs of both offsets must be zero to ensure correct
       *   alignment"
       *
       * We have no guarantee that tile_x and tile_y are correctly aligned,
       * since they are determined by the mipmap layout, which is only aligned
       * to multiples of 4.
       *
       * So, to avoid hanging the GPU, just smash the low order 3 bits of
       * tile_x and tile_y to 0.  This is a temporary workaround until we come
       * up with a better solution.
       */
      tile_x &= ~7;
      tile_y &= ~7;

      intel_emit_depth_stall_flushes(intel);

      BEGIN_BATCH(7);
      OUT_BATCH(GEN7_3DSTATE_DEPTH_BUFFER << 16 | (7 - 2));
      uint32_t pitch_bytes =
         params->depth.mt->region->pitch * params->depth.mt->region->cpp;
      OUT_BATCH((pitch_bytes - 1) |
                params->depth_format << 18 |
                1 << 22 | /* hiz enable */
                1 << 28 | /* depth write */
                BRW_SURFACE_2D << 29);
      OUT_RELOC(params->depth.mt->region->bo,
                I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                offset);
      OUT_BATCH((params->depth.width + tile_x - 1) << 4 |
                (params->depth.height + tile_y - 1) << 18);
      OUT_BATCH(0);
      OUT_BATCH(tile_x |
                tile_y << 16);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_HIER_DEPTH_BUFFER */
   {
      struct intel_region *hiz_region = params->depth.mt->hiz_mt->region;
      uint32_t hiz_offset =
         intel_region_get_aligned_offset(hiz_region,
                                         draw_x & ~tile_mask_x,
                                         (draw_y & ~tile_mask_y) / 2, false);

      BEGIN_BATCH(3);
      OUT_BATCH((GEN7_3DSTATE_HIER_DEPTH_BUFFER << 16) | (3 - 2));
      OUT_BATCH(hiz_region->pitch * hiz_region->cpp - 1);
      OUT_RELOC(hiz_region->bo,
                I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                hiz_offset);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_STENCIL_BUFFER */
   {
      BEGIN_BATCH(3);
      OUT_BATCH((GEN7_3DSTATE_STENCIL_BUFFER << 16) | (3 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }
}


static void
gen7_blorp_emit_depth_disable(struct brw_context *brw,
                              const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(7);
   OUT_BATCH(GEN7_3DSTATE_DEPTH_BUFFER << 16 | (7 - 2));
   OUT_BATCH(BRW_DEPTHFORMAT_D32_FLOAT << 18 | (BRW_SURFACE_NULL << 29));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/* 3DSTATE_CLEAR_PARAMS
 *
 * From the BSpec, Volume 2a.11 Windower, Section 1.5.6.3.2
 * 3DSTATE_CLEAR_PARAMS:
 *    [DevIVB] 3DSTATE_CLEAR_PARAMS must always be programmed in the along
 *    with the other Depth/Stencil state commands(i.e.  3DSTATE_DEPTH_BUFFER,
 *    3DSTATE_STENCIL_BUFFER, or 3DSTATE_HIER_DEPTH_BUFFER).
 */
static void
gen7_blorp_emit_clear_params(struct brw_context *brw,
                             const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(3);
   OUT_BATCH(GEN7_3DSTATE_CLEAR_PARAMS << 16 | (3 - 2));
   OUT_BATCH(params->depth.mt ? params->depth.mt->depth_clear_value : 0);
   OUT_BATCH(GEN7_DEPTH_CLEAR_VALID);
   ADVANCE_BATCH();
}


/* 3DPRIMITIVE */
static void
gen7_blorp_emit_primitive(struct brw_context *brw,
                          const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(7);
   OUT_BATCH(CMD_3D_PRIM << 16 | (7 - 2));
   OUT_BATCH(GEN7_3DPRIM_VERTEXBUFFER_ACCESS_SEQUENTIAL |
             _3DPRIM_RECTLIST);
   OUT_BATCH(3); /* vertex count per instance */
   OUT_BATCH(0);
   OUT_BATCH(1); /* instance count */
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/**
 * \copydoc gen6_blorp_exec()
 */
void
gen7_blorp_exec(struct intel_context *intel,
                const brw_blorp_params *params)
{
   struct gl_context *ctx = &intel->ctx;
   struct brw_context *brw = brw_context(ctx);
   brw_blorp_prog_data *prog_data = NULL;
   uint32_t cc_blend_state_offset = 0;
   uint32_t cc_state_offset = 0;
   uint32_t depthstencil_offset;
   uint32_t wm_push_const_offset = 0;
   uint32_t wm_bind_bo_offset = 0;
   uint32_t sampler_offset = 0;

   uint32_t prog_offset = params->get_wm_prog(brw, &prog_data);
   gen6_blorp_emit_batch_head(brw, params);
   gen7_allocate_push_constants(brw);
   gen6_emit_3dstate_multisample(brw, params->num_samples);
   gen6_emit_3dstate_sample_mask(brw, params->num_samples, 1.0, false);
   gen6_blorp_emit_state_base_address(brw, params);
   gen6_blorp_emit_vertices(brw, params);
   gen7_blorp_emit_urb_config(brw, params);
   if (params->use_wm_prog) {
      cc_blend_state_offset = gen6_blorp_emit_blend_state(brw, params);
      cc_state_offset = gen6_blorp_emit_cc_state(brw, params);
      gen7_blorp_emit_blend_state_pointer(brw, params, cc_blend_state_offset);
      gen7_blorp_emit_cc_state_pointer(brw, params, cc_state_offset);
   }
   depthstencil_offset = gen6_blorp_emit_depth_stencil_state(brw, params);
   gen7_blorp_emit_depth_stencil_state_pointers(brw, params,
                                                depthstencil_offset);
   if (params->use_wm_prog) {
      uint32_t wm_surf_offset_renderbuffer;
      uint32_t wm_surf_offset_texture;
      wm_push_const_offset = gen6_blorp_emit_wm_constants(brw, params);
      wm_surf_offset_renderbuffer =
         gen7_blorp_emit_surface_state(brw, params, &params->dst,
                                       I915_GEM_DOMAIN_RENDER,
                                       I915_GEM_DOMAIN_RENDER,
                                       true /* is_render_target */);
      wm_surf_offset_texture =
         gen7_blorp_emit_surface_state(brw, params, &params->src,
                                       I915_GEM_DOMAIN_SAMPLER, 0,
                                       false /* is_render_target */);
      wm_bind_bo_offset =
         gen6_blorp_emit_binding_table(brw, params,
                                       wm_surf_offset_renderbuffer,
                                       wm_surf_offset_texture);
      sampler_offset = gen7_blorp_emit_sampler_state(brw, params);
   }
   gen6_blorp_emit_vs_disable(brw, params);
   gen7_blorp_emit_hs_disable(brw, params);
   gen7_blorp_emit_te_disable(brw, params);
   gen7_blorp_emit_ds_disable(brw, params);
   gen6_blorp_emit_gs_disable(brw, params);
   gen7_blorp_emit_streamout_disable(brw, params);
   gen6_blorp_emit_clip_disable(brw, params);
   gen7_blorp_emit_sf_config(brw, params);
   gen7_blorp_emit_wm_config(brw, params, prog_data);
   if (params->use_wm_prog) {
      gen7_blorp_emit_binding_table_pointers_ps(brw, params,
                                                wm_bind_bo_offset);
      gen7_blorp_emit_sampler_state_pointers_ps(brw, params, sampler_offset);
      gen7_blorp_emit_constant_ps(brw, params, wm_push_const_offset);
   }
   gen7_blorp_emit_ps_config(brw, params, prog_offset, prog_data);
   gen7_blorp_emit_cc_viewport(brw, params);

   if (params->depth.mt)
      gen7_blorp_emit_depth_stencil_config(brw, params);
   else
      gen7_blorp_emit_depth_disable(brw, params);
   gen7_blorp_emit_clear_params(brw, params);
   gen6_blorp_emit_drawing_rectangle(brw, params);
   gen7_blorp_emit_primitive(brw, params);

   /* See comments above at first invocation of intel_flush() in
    * gen6_blorp_emit_batch_head().
    */
   intel_flush(ctx);

   /* Be safe. */
   brw->state.dirty.brw = ~0;
   brw->state.dirty.cache = ~0;
}
