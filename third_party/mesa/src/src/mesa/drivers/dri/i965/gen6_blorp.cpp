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
#include "gen6_blorp.h"

/**
 * \name Constants for BLORP VBO
 * \{
 */
#define GEN6_BLORP_NUM_VERTICES 3
#define GEN6_BLORP_NUM_VUE_ELEMS 8
#define GEN6_BLORP_VBO_SIZE (GEN6_BLORP_NUM_VERTICES \
                             * GEN6_BLORP_NUM_VUE_ELEMS \
                             * sizeof(float))
/** \} */


/**
 * Compute masks to determine how much of draw_x and draw_y should be
 * performed using the fine adjustment of "depth coordinate offset X/Y"
 * (dw5 of 3DSTATE_DEPTH_BUFFER).  See the emit_depthbuffer() function for
 * details.
 */
void
gen6_blorp_compute_tile_masks(const brw_blorp_params *params,
                              uint32_t *tile_mask_x, uint32_t *tile_mask_y)
{
   uint32_t depth_mask_x, depth_mask_y, hiz_mask_x, hiz_mask_y;
   intel_region_get_tile_masks(params->depth.mt->region,
                               &depth_mask_x, &depth_mask_y, false);
   intel_region_get_tile_masks(params->depth.mt->hiz_mt->region,
                               &hiz_mask_x, &hiz_mask_y, false);

   /* Each HiZ row represents 2 rows of pixels */
   hiz_mask_y = hiz_mask_y << 1 | 1;

   *tile_mask_x = depth_mask_x | hiz_mask_x;
   *tile_mask_y = depth_mask_y | hiz_mask_y;
}


void
gen6_blorp_emit_batch_head(struct brw_context *brw,
                           const brw_blorp_params *params)
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct intel_context *intel = &brw->intel;

   /* To ensure that the batch contains only the resolve, flush the batch
    * before beginning and after finishing emitting the resolve packets.
    *
    * Ideally, we would not need to flush for the resolve op. But, I suspect
    * that it's unsafe for CMD_PIPELINE_SELECT to occur multiple times in
    * a single batch, and there is no safe way to ensure that other than by
    * fencing the resolve with flushes. Ideally, we would just detect if
    * a batch is in progress and do the right thing, but that would require
    * the ability to *safely* access brw_context::state::dirty::brw
    * outside of the brw_upload_state() codepath.
    */
   intel_flush(ctx);

   /* CMD_PIPELINE_SELECT
    *
    * Select the 3D pipeline, as opposed to the media pipeline.
    */
   {
      BEGIN_BATCH(1);
      OUT_BATCH(brw->CMD_PIPELINE_SELECT << 16);
      ADVANCE_BATCH();
   }
}


/**
 * CMD_STATE_BASE_ADDRESS
 *
 * From the Sandy Bridge PRM, Volume 1, Part 1, Table STATE_BASE_ADDRESS:
 *     The following commands must be reissued following any change to the
 *     base addresses:
 *         3DSTATE_CC_POINTERS
 *         3DSTATE_BINDING_TABLE_POINTERS
 *         3DSTATE_SAMPLER_STATE_POINTERS
 *         3DSTATE_VIEWPORT_STATE_POINTERS
 *         MEDIA_STATE_POINTERS
 */
void
gen6_blorp_emit_state_base_address(struct brw_context *brw,
                                   const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(10);
   OUT_BATCH(CMD_STATE_BASE_ADDRESS << 16 | (10 - 2));
   OUT_BATCH(1); /* GeneralStateBaseAddressModifyEnable */
   /* SurfaceStateBaseAddress */
   OUT_RELOC(intel->batch.bo, I915_GEM_DOMAIN_SAMPLER, 0, 1);
   /* DynamicStateBaseAddress */
   OUT_RELOC(intel->batch.bo, (I915_GEM_DOMAIN_RENDER |
                               I915_GEM_DOMAIN_INSTRUCTION), 0, 1);
   OUT_BATCH(1); /* IndirectObjectBaseAddress */
   if (params->use_wm_prog) {
      OUT_RELOC(brw->cache.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
                1); /* Instruction base address: shader kernels */
   } else {
      OUT_BATCH(1); /* InstructionBaseAddress */
   }
   OUT_BATCH(1); /* GeneralStateUpperBound */
   /* Dynamic state upper bound.  Although the documentation says that
    * programming it to zero will cause it to be ignored, that is a lie.
    * If this isn't programmed to a real bound, the sampler border color
    * pointer is rejected, causing border color to mysteriously fail.
    */
   OUT_BATCH(0xfffff001);
   OUT_BATCH(1); /* IndirectObjectUpperBound*/
   OUT_BATCH(1); /* InstructionAccessUpperBound */
   ADVANCE_BATCH();
}


void
gen6_blorp_emit_vertices(struct brw_context *brw,
                         const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;
   uint32_t vertex_offset;

   /* Setup VBO for the rectangle primitive..
    *
    * A rectangle primitive (3DPRIM_RECTLIST) consists of only three
    * vertices. The vertices reside in screen space with DirectX coordinates
    * (that is, (0, 0) is the upper left corner).
    *
    *   v2 ------ implied
    *    |        |
    *    |        |
    *   v0 ----- v1
    *
    * Since the VS is disabled, the clipper loads each VUE directly from
    * the URB. This is controlled by the 3DSTATE_VERTEX_BUFFERS and
    * 3DSTATE_VERTEX_ELEMENTS packets below. The VUE contents are as follows:
    *   dw0: Reserved, MBZ.
    *   dw1: Render Target Array Index. The HiZ op does not use indexed
    *        vertices, so set the dword to 0.
    *   dw2: Viewport Index. The HiZ op disables viewport mapping and
    *        scissoring, so set the dword to 0.
    *   dw3: Point Width: The HiZ op does not emit the POINTLIST primitive, so
    *        set the dword to 0.
    *   dw4: Vertex Position X.
    *   dw5: Vertex Position Y.
    *   dw6: Vertex Position Z.
    *   dw7: Vertex Position W.
    *
    * For details, see the Sandybridge PRM, Volume 2, Part 1, Section 1.5.1
    * "Vertex URB Entry (VUE) Formats".
    */
   {
      float *vertex_data;

      const float vertices[GEN6_BLORP_VBO_SIZE] = {
         /* v0 */ 0, 0, 0, 0,     params->x0, params->y1, 0, 1,
         /* v1 */ 0, 0, 0, 0,     params->x1, params->y1, 0, 1,
         /* v2 */ 0, 0, 0, 0,     params->x0, params->y0, 0, 1,
      };

      vertex_data = (float *) brw_state_batch(brw, AUB_TRACE_VERTEX_BUFFER,
                                              GEN6_BLORP_VBO_SIZE, 32,
                                              &vertex_offset);
      memcpy(vertex_data, vertices, GEN6_BLORP_VBO_SIZE);
   }

   /* 3DSTATE_VERTEX_BUFFERS */
   {
      const int num_buffers = 1;
      const int batch_length = 1 + 4 * num_buffers;

      uint32_t dw0 = GEN6_VB0_ACCESS_VERTEXDATA |
                     (GEN6_BLORP_NUM_VUE_ELEMS * sizeof(float)) << BRW_VB0_PITCH_SHIFT;

      if (intel->gen >= 7)
         dw0 |= GEN7_VB0_ADDRESS_MODIFYENABLE;

      BEGIN_BATCH(batch_length);
      OUT_BATCH((_3DSTATE_VERTEX_BUFFERS << 16) | (batch_length - 2));
      OUT_BATCH(dw0);
      /* start address */
      OUT_RELOC(intel->batch.bo, I915_GEM_DOMAIN_VERTEX, 0,
		vertex_offset);
      /* end address */
      OUT_RELOC(intel->batch.bo, I915_GEM_DOMAIN_VERTEX, 0,
		vertex_offset + GEN6_BLORP_VBO_SIZE - 1);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_VERTEX_ELEMENTS
    *
    * Fetch dwords 0 - 7 from each VUE. See the comments above where
    * the vertex_bo is filled with data.
    */
   {
      const int num_elements = 2;
      const int batch_length = 1 + 2 * num_elements;

      BEGIN_BATCH(batch_length);
      OUT_BATCH((_3DSTATE_VERTEX_ELEMENTS << 16) | (batch_length - 2));
      /* Element 0 */
      OUT_BATCH(GEN6_VE0_VALID |
                BRW_SURFACEFORMAT_R32G32B32A32_FLOAT << BRW_VE0_FORMAT_SHIFT |
                0 << BRW_VE0_SRC_OFFSET_SHIFT);
      OUT_BATCH(BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_0_SHIFT |
                BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_1_SHIFT |
                BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_2_SHIFT |
                BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_3_SHIFT);
      /* Element 1 */
      OUT_BATCH(GEN6_VE0_VALID |
                BRW_SURFACEFORMAT_R32G32B32A32_FLOAT << BRW_VE0_FORMAT_SHIFT |
                16 << BRW_VE0_SRC_OFFSET_SHIFT);
      OUT_BATCH(BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_0_SHIFT |
                BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_1_SHIFT |
                BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_2_SHIFT |
                BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_3_SHIFT);
      ADVANCE_BATCH();
   }
}


/* 3DSTATE_URB
 *
 * Assign the entire URB to the VS. Even though the VS disabled, URB space
 * is still needed because the clipper loads the VUE's from the URB. From
 * the Sandybridge PRM, Volume 2, Part 1, Section 3DSTATE,
 * Dword 1.15:0 "VS Number of URB Entries":
 *     This field is always used (even if VS Function Enable is DISABLED).
 *
 * The warning below appears in the PRM (Section 3DSTATE_URB), but we can
 * safely ignore it because this batch contains only one draw call.
 *     Because of URB corruption caused by allocating a previous GS unit
 *     URB entry to the VS unit, software is required to send a “GS NULL
 *     Fence” (Send URB fence with VS URB size == 1 and GS URB size == 0)
 *     plus a dummy DRAW call before any case where VS will be taking over
 *     GS URB space.
 */
static void
gen6_blorp_emit_urb_config(struct brw_context *brw,
                           const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(3);
   OUT_BATCH(_3DSTATE_URB << 16 | (3 - 2));
   OUT_BATCH(brw->urb.max_vs_entries << GEN6_URB_VS_ENTRIES_SHIFT);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/* BLEND_STATE */
uint32_t
gen6_blorp_emit_blend_state(struct brw_context *brw,
                            const brw_blorp_params *params)
{
   uint32_t cc_blend_state_offset;

   struct gen6_blend_state *blend = (struct gen6_blend_state *)
      brw_state_batch(brw, AUB_TRACE_BLEND_STATE,
                      sizeof(struct gen6_blend_state), 64,
                      &cc_blend_state_offset);

   memset(blend, 0, sizeof(*blend));

   blend->blend1.pre_blend_clamp_enable = 1;
   blend->blend1.post_blend_clamp_enable = 1;
   blend->blend1.clamp_range = BRW_RENDERTARGET_CLAMPRANGE_FORMAT;

   blend->blend1.write_disable_r = false;
   blend->blend1.write_disable_g = false;
   blend->blend1.write_disable_b = false;
   blend->blend1.write_disable_a = false;

   return cc_blend_state_offset;
}


/* CC_STATE */
uint32_t
gen6_blorp_emit_cc_state(struct brw_context *brw,
                         const brw_blorp_params *params)
{
   uint32_t cc_state_offset;

   struct gen6_color_calc_state *cc = (struct gen6_color_calc_state *)
      brw_state_batch(brw, AUB_TRACE_CC_STATE,
                      sizeof(gen6_color_calc_state), 64,
                      &cc_state_offset);
   memset(cc, 0, sizeof(*cc));

   return cc_state_offset;
}


/**
 * \param out_offset is relative to
 *        CMD_STATE_BASE_ADDRESS.DynamicStateBaseAddress.
 */
uint32_t
gen6_blorp_emit_depth_stencil_state(struct brw_context *brw,
                                    const brw_blorp_params *params)
{
   uint32_t depthstencil_offset;

   struct gen6_depth_stencil_state *state;
   state = (struct gen6_depth_stencil_state *)
      brw_state_batch(brw, AUB_TRACE_DEPTH_STENCIL_STATE,
                      sizeof(*state), 64,
                      &depthstencil_offset);
   memset(state, 0, sizeof(*state));

   /* See the following sections of the Sandy Bridge PRM, Volume 1, Part2:
    *   - 7.5.3.1 Depth Buffer Clear
    *   - 7.5.3.2 Depth Buffer Resolve
    *   - 7.5.3.3 Hierarchical Depth Buffer Resolve
    */
   state->ds2.depth_write_enable = 1;
   if (params->hiz_op == GEN6_HIZ_OP_DEPTH_RESOLVE) {
      state->ds2.depth_test_enable = 1;
      state->ds2.depth_test_func = COMPAREFUNC_NEVER;
   }

   return depthstencil_offset;
}


/* 3DSTATE_CC_STATE_POINTERS
 *
 * The pointer offsets are relative to
 * CMD_STATE_BASE_ADDRESS.DynamicStateBaseAddress.
 *
 * The HiZ op doesn't use BLEND_STATE or COLOR_CALC_STATE.
 */
static void
gen6_blorp_emit_cc_state_pointers(struct brw_context *brw,
                                  const brw_blorp_params *params,
                                  uint32_t cc_blend_state_offset,
                                  uint32_t depthstencil_offset,
                                  uint32_t cc_state_offset)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_CC_STATE_POINTERS << 16 | (4 - 2));
   OUT_BATCH(cc_blend_state_offset | 1); /* BLEND_STATE offset */
   OUT_BATCH(depthstencil_offset | 1); /* DEPTH_STENCIL_STATE offset */
   OUT_BATCH(cc_state_offset | 1); /* COLOR_CALC_STATE offset */
   ADVANCE_BATCH();
}


/* WM push constants */
uint32_t
gen6_blorp_emit_wm_constants(struct brw_context *brw,
                             const brw_blorp_params *params)
{
   uint32_t wm_push_const_offset;

   void *constants = brw_state_batch(brw, AUB_TRACE_WM_CONSTANTS,
                                     sizeof(params->wm_push_consts),
                                     32, &wm_push_const_offset);
   memcpy(constants, &params->wm_push_consts,
          sizeof(params->wm_push_consts));

   return wm_push_const_offset;
}


/* SURFACE_STATE for renderbuffer or texture surface (see
 * brw_update_renderbuffer_surface and brw_update_texture_surface)
 */
static uint32_t
gen6_blorp_emit_surface_state(struct brw_context *brw,
                              const brw_blorp_params *params,
                              const brw_blorp_surface_info *surface,
                              uint32_t read_domains, uint32_t write_domain)
{
   uint32_t wm_surf_offset;
   uint32_t width = surface->width;
   uint32_t height = surface->height;
   if (surface->num_samples > 1) {
      /* Since gen6 uses INTEL_MSAA_LAYOUT_IMS, width and height are measured
       * in samples.  But SURFACE_STATE wants them in pixels, so we need to
       * divide them each by 2.
       */
      width /= 2;
      height /= 2;
   }
   struct intel_region *region = surface->mt->region;
   uint32_t tile_x, tile_y;

   uint32_t *surf = (uint32_t *)
      brw_state_batch(brw, AUB_TRACE_SURFACE_STATE, 6 * 4, 32,
                      &wm_surf_offset);

   surf[0] = (BRW_SURFACE_2D << BRW_SURFACE_TYPE_SHIFT |
              BRW_SURFACE_MIPMAPLAYOUT_BELOW << BRW_SURFACE_MIPLAYOUT_SHIFT |
              BRW_SURFACE_CUBEFACE_ENABLES |
              surface->brw_surfaceformat << BRW_SURFACE_FORMAT_SHIFT);

   /* reloc */
   surf[1] = (surface->compute_tile_offsets(&tile_x, &tile_y) +
              region->bo->offset);

   surf[2] = (0 << BRW_SURFACE_LOD_SHIFT |
              (width - 1) << BRW_SURFACE_WIDTH_SHIFT |
              (height - 1) << BRW_SURFACE_HEIGHT_SHIFT);

   uint32_t tiling = surface->map_stencil_as_y_tiled
      ? BRW_SURFACE_TILED | BRW_SURFACE_TILED_Y
      : brw_get_surface_tiling_bits(region->tiling);
   uint32_t pitch_bytes = region->pitch * region->cpp;
   if (surface->map_stencil_as_y_tiled)
      pitch_bytes *= 2;
   surf[3] = (tiling |
              0 << BRW_SURFACE_DEPTH_SHIFT |
              (pitch_bytes - 1) << BRW_SURFACE_PITCH_SHIFT);

   surf[4] = brw_get_surface_num_multisamples(surface->num_samples);

   /* Note that the low bits of these fields are missing, so
    * there's the possibility of getting in trouble.
    */
   assert(tile_x % 4 == 0);
   assert(tile_y % 2 == 0);
   surf[5] = ((tile_x / 4) << BRW_SURFACE_X_OFFSET_SHIFT |
              (tile_y / 2) << BRW_SURFACE_Y_OFFSET_SHIFT |
              (surface->mt->align_h == 4 ?
               BRW_SURFACE_VERTICAL_ALIGN_ENABLE : 0));

   /* Emit relocation to surface contents */
   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
                           wm_surf_offset + 4,
                           region->bo,
                           surf[1] - region->bo->offset,
                           read_domains, write_domain);

   return wm_surf_offset;
}


/* BINDING_TABLE.  See brw_wm_binding_table(). */
uint32_t
gen6_blorp_emit_binding_table(struct brw_context *brw,
                              const brw_blorp_params *params,
                              uint32_t wm_surf_offset_renderbuffer,
                              uint32_t wm_surf_offset_texture)
{
   uint32_t wm_bind_bo_offset;
   uint32_t *bind = (uint32_t *)
      brw_state_batch(brw, AUB_TRACE_BINDING_TABLE,
                      sizeof(uint32_t) *
                      BRW_BLORP_NUM_BINDING_TABLE_ENTRIES,
                      32, /* alignment */
                      &wm_bind_bo_offset);
   bind[BRW_BLORP_RENDERBUFFER_BINDING_TABLE_INDEX] =
      wm_surf_offset_renderbuffer;
   bind[BRW_BLORP_TEXTURE_BINDING_TABLE_INDEX] = wm_surf_offset_texture;

   return wm_bind_bo_offset;
}


/**
 * SAMPLER_STATE.  See brw_update_sampler_state().
 */
static uint32_t
gen6_blorp_emit_sampler_state(struct brw_context *brw,
                              const brw_blorp_params *params)
{
   uint32_t sampler_offset;

   struct brw_sampler_state *sampler = (struct brw_sampler_state *)
      brw_state_batch(brw, AUB_TRACE_SAMPLER_STATE,
                      sizeof(struct brw_sampler_state),
                      32, &sampler_offset);
   memset(sampler, 0, sizeof(*sampler));

   sampler->ss0.min_filter = BRW_MAPFILTER_LINEAR;
   sampler->ss0.mip_filter = BRW_MIPFILTER_NONE;
   sampler->ss0.mag_filter = BRW_MAPFILTER_LINEAR;

   sampler->ss1.r_wrap_mode = BRW_TEXCOORDMODE_CLAMP;
   sampler->ss1.s_wrap_mode = BRW_TEXCOORDMODE_CLAMP;
   sampler->ss1.t_wrap_mode = BRW_TEXCOORDMODE_CLAMP;

   sampler->ss0.min_mag_neq = 1;

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

   sampler->ss1.max_lod = U_FIXED(0, 6);
   sampler->ss1.min_lod = U_FIXED(0, 6);

   sampler->ss3.non_normalized_coord = 1;

   sampler->ss3.address_round |= BRW_ADDRESS_ROUNDING_ENABLE_U_MIN |
      BRW_ADDRESS_ROUNDING_ENABLE_V_MIN |
      BRW_ADDRESS_ROUNDING_ENABLE_R_MIN;
   sampler->ss3.address_round |= BRW_ADDRESS_ROUNDING_ENABLE_U_MAG |
      BRW_ADDRESS_ROUNDING_ENABLE_V_MAG |
      BRW_ADDRESS_ROUNDING_ENABLE_R_MAG;

   return sampler_offset;
}


/**
 * 3DSTATE_SAMPLER_STATE_POINTERS.  See upload_sampler_state_pointers().
 */
static void
gen6_blorp_emit_sampler_state_pointers(struct brw_context *brw,
                                       const brw_blorp_params *params,
                                       uint32_t sampler_offset)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_SAMPLER_STATE_POINTERS << 16 |
             VS_SAMPLER_STATE_CHANGE |
             GS_SAMPLER_STATE_CHANGE |
             PS_SAMPLER_STATE_CHANGE |
             (4 - 2));
   OUT_BATCH(0); /* VS */
   OUT_BATCH(0); /* GS */
   OUT_BATCH(sampler_offset);
   ADVANCE_BATCH();
}


/* 3DSTATE_VS
 *
 * Disable vertex shader.
 */
void
gen6_blorp_emit_vs_disable(struct brw_context *brw,
                           const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   if (intel->gen == 6) {
      /* From the BSpec, Volume 2a, Part 3 "Vertex Shader", Section
       * 3DSTATE_VS, Dword 5.0 "VS Function Enable":
       *
       *   [DevSNB] A pipeline flush must be programmed prior to a
       *   3DSTATE_VS command that causes the VS Function Enable to
       *   toggle. Pipeline flush can be executed by sending a PIPE_CONTROL
       *   command with CS stall bit set and a post sync operation.
       */
      intel_emit_post_sync_nonzero_flush(intel);
   }

   BEGIN_BATCH(6);
   OUT_BATCH(_3DSTATE_VS << 16 | (6 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/* 3DSTATE_GS
 *
 * Disable the geometry shader.
 */
void
gen6_blorp_emit_gs_disable(struct brw_context *brw,
                           const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(7);
   OUT_BATCH(_3DSTATE_GS << 16 | (7 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/* 3DSTATE_CLIP
 *
 * Disable the clipper.
 *
 * The BLORP op emits a rectangle primitive, which requires clipping to
 * be disabled. From page 10 of the Sandy Bridge PRM Volume 2 Part 1
 * Section 1.3 "3D Primitives Overview":
 *    RECTLIST:
 *    Either the CLIP unit should be DISABLED, or the CLIP unit's Clip
 *    Mode should be set to a value other than CLIPMODE_NORMAL.
 *
 * Also disable perspective divide. This doesn't change the clipper's
 * output, but does spare a few electrons.
 */
void
gen6_blorp_emit_clip_disable(struct brw_context *brw,
                             const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_CLIP << 16 | (4 - 2));
   OUT_BATCH(0);
   OUT_BATCH(GEN6_CLIP_PERSPECTIVE_DIVIDE_DISABLE);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/* 3DSTATE_SF
 *
 * Disable ViewportTransformEnable (dw2.1)
 *
 * From the SandyBridge PRM, Volume 2, Part 1, Section 1.3, "3D
 * Primitives Overview":
 *     RECTLIST: Viewport Mapping must be DISABLED (as is typical with the
 *     use of screen- space coordinates).
 *
 * A solid rectangle must be rendered, so set FrontFaceFillMode (dw2.4:3)
 * and BackFaceFillMode (dw2.5:6) to SOLID(0).
 *
 * From the Sandy Bridge PRM, Volume 2, Part 1, Section
 * 6.4.1.1 3DSTATE_SF, Field FrontFaceFillMode:
 *     SOLID: Any triangle or rectangle object found to be front-facing
 *     is rendered as a solid object. This setting is required when
 *     (rendering rectangle (RECTLIST) objects.
 */
static void
gen6_blorp_emit_sf_config(struct brw_context *brw,
                          const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(20);
   OUT_BATCH(_3DSTATE_SF << 16 | (20 - 2));
   OUT_BATCH((1 - 1) << GEN6_SF_NUM_OUTPUTS_SHIFT | /* only position */
             1 << GEN6_SF_URB_ENTRY_READ_LENGTH_SHIFT |
             0 << GEN6_SF_URB_ENTRY_READ_OFFSET_SHIFT);
   OUT_BATCH(0); /* dw2 */
   OUT_BATCH(params->num_samples > 1 ? GEN6_SF_MSRAST_ON_PATTERN : 0);
   for (int i = 0; i < 16; ++i)
      OUT_BATCH(0);
   ADVANCE_BATCH();
}


/**
 * Enable or disable thread dispatch and set the HiZ op appropriately.
 */
static void
gen6_blorp_emit_wm_config(struct brw_context *brw,
                          const brw_blorp_params *params,
                          uint32_t prog_offset,
                          brw_blorp_prog_data *prog_data)
{
   struct intel_context *intel = &brw->intel;
   uint32_t dw2, dw4, dw5, dw6;

   /* Even when thread dispatch is disabled, max threads (dw5.25:31) must be
    * nonzero to prevent the GPU from hanging. See the valid ranges in the
    * BSpec, Volume 2a.11 Windower, Section 3DSTATE_WM, Dword 5.25:31
    * "Maximum Number Of Threads".
    *
    * To be safe (and to minimize extraneous code) we go ahead and fully
    * configure the WM state whether or not there is a WM program.
    */

   dw2 = dw4 = dw5 = dw6 = 0;
   switch (params->hiz_op) {
   case GEN6_HIZ_OP_DEPTH_CLEAR:
      dw4 |= GEN6_WM_DEPTH_CLEAR;
      break;
   case GEN6_HIZ_OP_DEPTH_RESOLVE:
      dw4 |= GEN6_WM_DEPTH_RESOLVE;
      break;
   case GEN6_HIZ_OP_HIZ_RESOLVE:
      dw4 |= GEN6_WM_HIERARCHICAL_DEPTH_RESOLVE;
      break;
   case GEN6_HIZ_OP_NONE:
      break;
   default:
      assert(0);
      break;
   }
   dw4 |= GEN6_WM_STATISTICS_ENABLE;
   dw5 |= GEN6_WM_LINE_AA_WIDTH_1_0;
   dw5 |= GEN6_WM_LINE_END_CAP_AA_WIDTH_0_5;
   dw5 |= (brw->max_wm_threads - 1) << GEN6_WM_MAX_THREADS_SHIFT;
   dw6 |= 0 << GEN6_WM_BARYCENTRIC_INTERPOLATION_MODE_SHIFT; /* No interp */
   dw6 |= 0 << GEN6_WM_NUM_SF_OUTPUTS_SHIFT; /* No inputs from SF */
   if (params->use_wm_prog) {
      dw2 |= 1 << GEN6_WM_SAMPLER_COUNT_SHIFT; /* Up to 4 samplers */
      dw4 |= prog_data->first_curbe_grf << GEN6_WM_DISPATCH_START_GRF_SHIFT_0;
      dw5 |= GEN6_WM_16_DISPATCH_ENABLE;
      dw5 |= GEN6_WM_KILL_ENABLE; /* TODO: temporarily smash on */
      dw5 |= GEN6_WM_DISPATCH_ENABLE; /* We are rendering */
   }

   if (params->num_samples > 1) {
      dw6 |= GEN6_WM_MSRAST_ON_PATTERN;
      if (prog_data && prog_data->persample_msaa_dispatch)
         dw6 |= GEN6_WM_MSDISPMODE_PERSAMPLE;
      else
         dw6 |= GEN6_WM_MSDISPMODE_PERPIXEL;
   } else {
      dw6 |= GEN6_WM_MSRAST_OFF_PIXEL;
      dw6 |= GEN6_WM_MSDISPMODE_PERSAMPLE;
   }

   BEGIN_BATCH(9);
   OUT_BATCH(_3DSTATE_WM << 16 | (9 - 2));
   OUT_BATCH(params->use_wm_prog ? prog_offset : 0);
   OUT_BATCH(dw2);
   OUT_BATCH(0); /* No scratch needed */
   OUT_BATCH(dw4);
   OUT_BATCH(dw5);
   OUT_BATCH(dw6);
   OUT_BATCH(0); /* No other programs */
   OUT_BATCH(0); /* No other programs */
   ADVANCE_BATCH();
}


static void
gen6_blorp_emit_constant_ps(struct brw_context *brw,
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
   BEGIN_BATCH(5);
   OUT_BATCH(_3DSTATE_CONSTANT_PS << 16 |
             GEN6_CONSTANT_BUFFER_0_ENABLE |
             (5 - 2));
   OUT_BATCH(wm_push_const_offset + (BRW_BLORP_NUM_PUSH_CONST_REGS - 1));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/**
 * 3DSTATE_BINDING_TABLE_POINTERS
 */
static void
gen6_blorp_emit_binding_table_pointers(struct brw_context *brw,
                                       const brw_blorp_params *params,
                                       uint32_t wm_bind_bo_offset)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_BINDING_TABLE_POINTERS << 16 |
             GEN6_BINDING_TABLE_MODIFY_PS |
             (4 - 2));
   OUT_BATCH(0); /* vs -- ignored */
   OUT_BATCH(0); /* gs -- ignored */
   OUT_BATCH(wm_bind_bo_offset); /* wm/ps */
   ADVANCE_BATCH();
}


static void
gen6_blorp_emit_depth_stencil_config(struct brw_context *brw,
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

      intel_emit_post_sync_nonzero_flush(intel);
      intel_emit_depth_stall_flushes(intel);

      BEGIN_BATCH(7);
      OUT_BATCH(_3DSTATE_DEPTH_BUFFER << 16 | (7 - 2));
      uint32_t pitch_bytes =
         params->depth.mt->region->pitch * params->depth.mt->region->cpp;
      OUT_BATCH((pitch_bytes - 1) |
                params->depth_format << 18 |
                1 << 21 | /* separate stencil enable */
                1 << 22 | /* hiz enable */
                BRW_TILEWALK_YMAJOR << 26 |
                1 << 27 | /* y-tiled */
                BRW_SURFACE_2D << 29);
      OUT_RELOC(params->depth.mt->region->bo,
                I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                offset);
      OUT_BATCH(BRW_SURFACE_MIPMAPLAYOUT_BELOW << 1 |
                (params->depth.width + tile_x - 1) << 6 |
                (params->depth.height + tile_y - 1) << 19);
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
      OUT_BATCH((_3DSTATE_HIER_DEPTH_BUFFER << 16) | (3 - 2));
      OUT_BATCH(hiz_region->pitch * hiz_region->cpp - 1);
      OUT_RELOC(hiz_region->bo,
                I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                hiz_offset);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_STENCIL_BUFFER */
   {
      BEGIN_BATCH(3);
      OUT_BATCH((_3DSTATE_STENCIL_BUFFER << 16) | (3 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }
}


static void
gen6_blorp_emit_depth_disable(struct brw_context *brw,
                              const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(7);
   OUT_BATCH(_3DSTATE_DEPTH_BUFFER << 16 | (7 - 2));
   OUT_BATCH((BRW_DEPTHFORMAT_D32_FLOAT << 18) |
             (BRW_SURFACE_NULL << 29));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/* 3DSTATE_CLEAR_PARAMS
 *
 * From the Sandybridge PRM, Volume 2, Part 1, Section 3DSTATE_CLEAR_PARAMS:
 *   [DevSNB] 3DSTATE_CLEAR_PARAMS packet must follow the DEPTH_BUFFER_STATE
 *   packet when HiZ is enabled and the DEPTH_BUFFER_STATE changes.
 */
static void
gen6_blorp_emit_clear_params(struct brw_context *brw,
                             const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_CLEAR_PARAMS << 16 |
	     GEN5_DEPTH_CLEAR_VALID |
	     (2 - 2));
   OUT_BATCH(params->depth.mt ? params->depth.mt->depth_clear_value : 0);
   ADVANCE_BATCH();
}


/* 3DSTATE_DRAWING_RECTANGLE */
void
gen6_blorp_emit_drawing_rectangle(struct brw_context *brw,
                                  const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_DRAWING_RECTANGLE << 16 | (4 - 2));
   OUT_BATCH(0);
   OUT_BATCH(((params->x1 - 1) & 0xffff) |
             ((params->y1 - 1) << 16));
   OUT_BATCH(0);
   ADVANCE_BATCH();
}

/* 3DSTATE_VIEWPORT_STATE_POINTERS */
void
gen6_blorp_emit_viewport_state(struct brw_context *brw,
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

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_VIEWPORT_STATE_POINTERS << 16 | (4 - 2) |
	     GEN6_CC_VIEWPORT_MODIFY);
   OUT_BATCH(0); /* clip VP */
   OUT_BATCH(0); /* SF VP */
   OUT_BATCH(cc_vp_offset);
   ADVANCE_BATCH();
}


/* 3DPRIMITIVE */
static void
gen6_blorp_emit_primitive(struct brw_context *brw,
                          const brw_blorp_params *params)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(6);
   OUT_BATCH(CMD_3D_PRIM << 16 | (6 - 2) |
             _3DPRIM_RECTLIST << GEN4_3DPRIM_TOPOLOGY_TYPE_SHIFT |
             GEN4_3DPRIM_VERTEXBUFFER_ACCESS_SEQUENTIAL);
   OUT_BATCH(3); /* vertex count per instance */
   OUT_BATCH(0);
   OUT_BATCH(1); /* instance count */
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/**
 * \brief Execute a blit or render pass operation.
 *
 * To execute the operation, this function manually constructs and emits a
 * batch to draw a rectangle primitive. The batchbuffer is flushed before
 * constructing and after emitting the batch.
 *
 * This function alters no GL state.
 */
void
gen6_blorp_exec(struct intel_context *intel,
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

   uint32_t prog_offset = params->get_wm_prog(brw, &prog_data);
   gen6_blorp_emit_batch_head(brw, params);
   gen6_emit_3dstate_multisample(brw, params->num_samples);
   gen6_emit_3dstate_sample_mask(brw, params->num_samples, 1.0, false);
   gen6_blorp_emit_state_base_address(brw, params);
   gen6_blorp_emit_vertices(brw, params);
   gen6_blorp_emit_urb_config(brw, params);
   if (params->use_wm_prog) {
      cc_blend_state_offset = gen6_blorp_emit_blend_state(brw, params);
      cc_state_offset = gen6_blorp_emit_cc_state(brw, params);
   }
   depthstencil_offset = gen6_blorp_emit_depth_stencil_state(brw, params);
   gen6_blorp_emit_cc_state_pointers(brw, params, cc_blend_state_offset,
                                     depthstencil_offset, cc_state_offset);
   if (params->use_wm_prog) {
      uint32_t wm_surf_offset_renderbuffer;
      uint32_t wm_surf_offset_texture;
      uint32_t sampler_offset;
      wm_push_const_offset = gen6_blorp_emit_wm_constants(brw, params);
      wm_surf_offset_renderbuffer =
         gen6_blorp_emit_surface_state(brw, params, &params->dst,
                                       I915_GEM_DOMAIN_RENDER,
                                       I915_GEM_DOMAIN_RENDER);
      wm_surf_offset_texture =
         gen6_blorp_emit_surface_state(brw, params, &params->src,
                                       I915_GEM_DOMAIN_SAMPLER, 0);
      wm_bind_bo_offset =
         gen6_blorp_emit_binding_table(brw, params,
                                       wm_surf_offset_renderbuffer,
                                       wm_surf_offset_texture);
      sampler_offset = gen6_blorp_emit_sampler_state(brw, params);
      gen6_blorp_emit_sampler_state_pointers(brw, params, sampler_offset);
   }
   gen6_blorp_emit_vs_disable(brw, params);
   gen6_blorp_emit_gs_disable(brw, params);
   gen6_blorp_emit_clip_disable(brw, params);
   gen6_blorp_emit_sf_config(brw, params);
   if (params->use_wm_prog)
      gen6_blorp_emit_constant_ps(brw, params, wm_push_const_offset);
   gen6_blorp_emit_wm_config(brw, params, prog_offset, prog_data);
   if (params->use_wm_prog)
      gen6_blorp_emit_binding_table_pointers(brw, params, wm_bind_bo_offset);
   gen6_blorp_emit_viewport_state(brw, params);

   if (params->depth.mt)
      gen6_blorp_emit_depth_stencil_config(brw, params);
   else
      gen6_blorp_emit_depth_disable(brw, params);
   gen6_blorp_emit_clear_params(brw, params);
   gen6_blorp_emit_drawing_rectangle(brw, params);
   gen6_blorp_emit_primitive(brw, params);

   /* See comments above at first invocation of intel_flush() in
    * gen6_blorp_emit_batch_head().
    */
   intel_flush(ctx);

   /* Be safe. */
   brw->state.dirty.brw = ~0;
   brw->state.dirty.cache = ~0;
}

