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
#include "main/mtypes.h"
#include "main/samplerobj.h"
#include "program/prog_parameter.h"

#include "intel_mipmap_tree.h"
#include "intel_batchbuffer.h"
#include "intel_tex.h"
#include "intel_fbo.h"
#include "intel_buffer_objects.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_wm.h"

/**
 * Convert an swizzle enumeration (i.e. SWIZZLE_X) to one of the Gen7.5+
 * "Shader Channel Select" enumerations (i.e. HSW_SCS_RED)
 */
static unsigned
swizzle_to_scs(GLenum swizzle)
{
   switch (swizzle) {
   case SWIZZLE_X:
      return HSW_SCS_RED;
   case SWIZZLE_Y:
      return HSW_SCS_GREEN;
   case SWIZZLE_Z:
      return HSW_SCS_BLUE;
   case SWIZZLE_W:
      return HSW_SCS_ALPHA;
   case SWIZZLE_ZERO:
      return HSW_SCS_ZERO;
   case SWIZZLE_ONE:
      return HSW_SCS_ONE;
   }

   assert(!"Should not get here: invalid swizzle mode");
   return HSW_SCS_ZERO;
}

void
gen7_set_surface_tiling(struct gen7_surface_state *surf, uint32_t tiling)
{
   switch (tiling) {
   case I915_TILING_NONE:
      surf->ss0.tiled_surface = 0;
      surf->ss0.tile_walk = 0;
      break;
   case I915_TILING_X:
      surf->ss0.tiled_surface = 1;
      surf->ss0.tile_walk = BRW_TILEWALK_XMAJOR;
      break;
   case I915_TILING_Y:
      surf->ss0.tiled_surface = 1;
      surf->ss0.tile_walk = BRW_TILEWALK_YMAJOR;
      break;
   }
}


void
gen7_set_surface_msaa(struct gen7_surface_state *surf, unsigned num_samples,
                      enum intel_msaa_layout layout)
{
   if (num_samples > 4)
      surf->ss4.num_multisamples = GEN7_SURFACE_MULTISAMPLECOUNT_8;
   else if (num_samples > 1)
      surf->ss4.num_multisamples = GEN7_SURFACE_MULTISAMPLECOUNT_4;
   else
      surf->ss4.num_multisamples = GEN7_SURFACE_MULTISAMPLECOUNT_1;

   surf->ss4.multisampled_surface_storage_format =
      layout == INTEL_MSAA_LAYOUT_IMS ?
      GEN7_SURFACE_MSFMT_DEPTH_STENCIL :
      GEN7_SURFACE_MSFMT_MSS;
}


void
gen7_set_surface_mcs_info(struct brw_context *brw,
                          struct gen7_surface_state *surf,
                          uint32_t surf_offset,
                          const struct intel_mipmap_tree *mcs_mt,
                          bool is_render_target)
{
   /* From the Ivy Bridge PRM, Vol4 Part1 p76, "MCS Base Address":
    *
    *     "The MCS surface must be stored as Tile Y."
    */
   assert(mcs_mt->region->tiling == I915_TILING_Y);

   /* Compute the pitch in units of tiles.  To do this we need to divide the
    * pitch in bytes by 128, since a single Y-tile is 128 bytes wide.
    */
   unsigned pitch_bytes = mcs_mt->region->pitch * mcs_mt->cpp;
   unsigned pitch_tiles = pitch_bytes / 128;

   /* The upper 20 bits of surface state DWORD 6 are the upper 20 bits of the
    * GPU address of the MCS buffer; the lower 12 bits contain other control
    * information.  Since buffer addresses are always on 4k boundaries (and
    * thus have their lower 12 bits zero), we can use an ordinary reloc to do
    * the necessary address translation.
    */
   assert ((mcs_mt->region->bo->offset & 0xfff) == 0);
   surf->ss6.mcs_enabled.mcs_enable = 1;
   surf->ss6.mcs_enabled.mcs_surface_pitch = pitch_tiles - 1;
   surf->ss6.mcs_enabled.mcs_base_address = mcs_mt->region->bo->offset >> 12;
   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
                           surf_offset +
                           offsetof(struct gen7_surface_state, ss6),
                           mcs_mt->region->bo,
                           surf->ss6.raw_data & 0xfff,
                           is_render_target ? I915_GEM_DOMAIN_RENDER
                           : I915_GEM_DOMAIN_SAMPLER,
                           is_render_target ? I915_GEM_DOMAIN_RENDER : 0);
}


void
gen7_check_surface_setup(struct gen7_surface_state *surf,
                         bool is_render_target)
{
   bool is_multisampled =
      surf->ss4.num_multisamples != GEN7_SURFACE_MULTISAMPLECOUNT_1;
   /* From the Graphics BSpec: vol5c Shared Functions [SNB+] > State >
    * SURFACE_STATE > SURFACE_STATE for most messages [DevIVB]: Surface Array
    * Spacing:
    *
    *   If Multisampled Surface Storage Format is MSFMT_MSS and Number of
    *   Multisamples is not MULTISAMPLECOUNT_1, this field must be set to
    *   ARYSPC_LOD0.
    */
   if (surf->ss4.multisampled_surface_storage_format == GEN7_SURFACE_MSFMT_MSS
       && is_multisampled)
      assert(surf->ss0.surface_array_spacing == GEN7_SURFACE_ARYSPC_LOD0);

   /* From the Graphics BSpec: vol5c Shared Functions [SNB+] > State >
    * SURFACE_STATE > SURFACE_STATE for most messages [DevIVB]: Multisampled
    * Surface Storage Format:
    *
    *   All multisampled render target surfaces must have this field set to
    *   MSFMT_MSS.
    *
    * But also:
    *
    *   This field is ignored if Number of Multisamples is MULTISAMPLECOUNT_1.
    */
   if (is_render_target && is_multisampled) {
      assert(surf->ss4.multisampled_surface_storage_format ==
             GEN7_SURFACE_MSFMT_MSS);
   }

   /* From the Graphics BSpec: vol5c Shared Functions [SNB+] > State >
    * SURFACE_STATE > SURFACE_STATE for most messages [DevIVB]: Multisampled
    * Surface Storage Format:
    *
    *   If the surface’s Number of Multisamples is MULTISAMPLECOUNT_8, Width
    *   is >= 8192 (meaning the actual surface width is >= 8193 pixels), this
    *   field must be set to MSFMT_MSS.
    */
   if (surf->ss4.num_multisamples == GEN7_SURFACE_MULTISAMPLECOUNT_8 &&
       surf->ss2.width >= 8192) {
      assert(surf->ss4.multisampled_surface_storage_format ==
             GEN7_SURFACE_MSFMT_MSS);
   }

   /* From the Graphics BSpec: vol5c Shared Functions [SNB+] > State >
    * SURFACE_STATE > SURFACE_STATE for most messages [DevIVB]: Multisampled
    * Surface Storage Format:
    *
    *   If the surface’s Number of Multisamples is MULTISAMPLECOUNT_8,
    *   ((Depth+1) * (Height+1)) is > 4,194,304, OR if the surface’s Number of
    *   Multisamples is MULTISAMPLECOUNT_4, ((Depth+1) * (Height+1)) is >
    *   8,388,608, this field must be set to MSFMT_DEPTH_STENCIL.This field
    *   must be set to MSFMT_DEPTH_STENCIL if Surface Format is one of the
    *   following: I24X8_UNORM, L24X8_UNORM, A24X8_UNORM, or
    *   R24_UNORM_X8_TYPELESS.
    *
    * But also:
    *
    *   This field is ignored if Number of Multisamples is MULTISAMPLECOUNT_1.
    */
   uint32_t depth = surf->ss3.depth + 1;
   uint32_t height = surf->ss2.height + 1;
   if (surf->ss4.num_multisamples == GEN7_SURFACE_MULTISAMPLECOUNT_8 &&
       depth * height > 4194304) {
      assert(surf->ss4.multisampled_surface_storage_format ==
             GEN7_SURFACE_MSFMT_DEPTH_STENCIL);
   }
   if (surf->ss4.num_multisamples == GEN7_SURFACE_MULTISAMPLECOUNT_4 &&
       depth * height > 8388608) {
      assert(surf->ss4.multisampled_surface_storage_format ==
             GEN7_SURFACE_MSFMT_DEPTH_STENCIL);
   }
   if (is_multisampled) {
      switch (surf->ss0.surface_format) {
      case BRW_SURFACEFORMAT_I24X8_UNORM:
      case BRW_SURFACEFORMAT_L24X8_UNORM:
      case BRW_SURFACEFORMAT_A24X8_UNORM:
      case BRW_SURFACEFORMAT_R24_UNORM_X8_TYPELESS:
         assert(surf->ss4.multisampled_surface_storage_format ==
                GEN7_SURFACE_MSFMT_DEPTH_STENCIL);
      }
   }
}


static void
gen7_update_buffer_texture_surface(struct gl_context *ctx,
                                   unsigned unit,
                                   uint32_t *binding_table,
                                   unsigned surf_index)
{
   struct brw_context *brw = brw_context(ctx);
   struct gl_texture_object *tObj = ctx->Texture.Unit[unit]._Current;
   struct gen7_surface_state *surf;
   struct intel_buffer_object *intel_obj =
      intel_buffer_object(tObj->BufferObject);
   drm_intel_bo *bo = intel_obj ? intel_obj->buffer : NULL;
   gl_format format = tObj->_BufferObjectFormat;
   int texel_size = _mesa_get_format_bytes(format);

   surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
			  sizeof(*surf), 32, &binding_table[surf_index]);
   memset(surf, 0, sizeof(*surf));

   surf->ss0.surface_type = BRW_SURFACE_BUFFER;
   surf->ss0.surface_format = brw_format_for_mesa_format(format);

   surf->ss0.render_cache_read_write = 1;

   if (surf->ss0.surface_format == 0 && format != MESA_FORMAT_RGBA_FLOAT32) {
      _mesa_problem(NULL, "bad format %s for texture buffer\n",
		    _mesa_get_format_name(format));
   }

   if (bo) {
      surf->ss1.base_addr = bo->offset; /* reloc */

      /* Emit relocation to surface contents.  Section 5.1.1 of the gen4
       * bspec ("Data Cache") says that the data cache does not exist as
       * a separate cache and is just the sampler cache.
       */
      drm_intel_bo_emit_reloc(brw->intel.batch.bo,
			      (binding_table[surf_index] +
			       offsetof(struct gen7_surface_state, ss1)),
			      bo, 0,
			      I915_GEM_DOMAIN_SAMPLER, 0);

      int w = intel_obj->Base.Size / texel_size;
      surf->ss2.width = w & 0x7f;            /* bits 6:0 of size or width */
      surf->ss2.height = (w >> 7) & 0x1fff;  /* bits 19:7 of size or width */
      surf->ss3.depth = (w >> 20) & 0x7f;    /* bits 26:20 of size or width */
      surf->ss3.pitch = texel_size - 1;
} else {
      surf->ss1.base_addr = 0;
      surf->ss2.width = 0;
      surf->ss2.height = 0;
      surf->ss3.depth = 0;
      surf->ss3.pitch = 0;
   }

   gen7_set_surface_tiling(surf, I915_TILING_NONE);

   gen7_check_surface_setup(surf, false /* is_render_target */);
}

static void
gen7_update_texture_surface(struct gl_context *ctx,
                            unsigned unit,
                            uint32_t *binding_table,
                            unsigned surf_index)
{
   struct brw_context *brw = brw_context(ctx);
   struct gl_texture_object *tObj = ctx->Texture.Unit[unit]._Current;
   struct intel_texture_object *intelObj = intel_texture_object(tObj);
   struct intel_mipmap_tree *mt = intelObj->mt;
   struct gl_texture_image *firstImage = tObj->Image[0][tObj->BaseLevel];
   struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit);
   struct gen7_surface_state *surf;
   int width, height, depth;

   if (tObj->Target == GL_TEXTURE_BUFFER) {
      gen7_update_buffer_texture_surface(ctx, unit, binding_table, surf_index);
      return;
   }

   /* We don't support MSAA for textures. */
   assert(!mt->array_spacing_lod0);
   assert(mt->num_samples <= 1);

   intel_miptree_get_dimensions_for_image(firstImage, &width, &height, &depth);

   surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
			  sizeof(*surf), 32, &binding_table[surf_index]);
   memset(surf, 0, sizeof(*surf));

   if (mt->align_h == 4)
      surf->ss0.vertical_alignment = 1;
   if (mt->align_w == 8)
      surf->ss0.horizontal_alignment = 1;

   surf->ss0.surface_type = translate_tex_target(tObj->Target);
   surf->ss0.surface_format = translate_tex_format(mt->format,
                                                   firstImage->InternalFormat,
                                                   tObj->DepthMode,
                                                   sampler->sRGBDecode);
   if (tObj->Target == GL_TEXTURE_CUBE_MAP) {
      surf->ss0.cube_pos_x = 1;
      surf->ss0.cube_pos_y = 1;
      surf->ss0.cube_pos_z = 1;
      surf->ss0.cube_neg_x = 1;
      surf->ss0.cube_neg_y = 1;
      surf->ss0.cube_neg_z = 1;
   }

   surf->ss0.is_array = depth > 1 && tObj->Target != GL_TEXTURE_3D;

   gen7_set_surface_tiling(surf, intelObj->mt->region->tiling);

   /* ss0 remaining fields:
    * - vert_line_stride (exists on gen6 but we ignore it)
    * - vert_line_stride_ofs (exists on gen6 but we ignore it)
    * - surface_array_spacing
    * - render_cache_read_write (exists on gen6 but ignored here)
    */

   surf->ss1.base_addr =
      intelObj->mt->region->bo->offset + intelObj->mt->offset; /* reloc */

   surf->ss2.width = width - 1;
   surf->ss2.height = height - 1;

   surf->ss3.pitch = (intelObj->mt->region->pitch * intelObj->mt->cpp) - 1;
   surf->ss3.depth = depth - 1;

   /* ss4: ignored? */

   surf->ss5.mip_count = intelObj->_MaxLevel - tObj->BaseLevel;
   surf->ss5.min_lod = 0;

   /* ss5 remaining fields:
    * - x_offset (N/A for textures?)
    * - y_offset (ditto)
    * - cache_control
    */

   if (brw->intel.is_haswell) {
      /* Handling GL_ALPHA as a surface format override breaks 1.30+ style
       * texturing functions that return a float, as our code generation always
       * selects the .x channel (which would always be 0).
       */
      const bool alpha_depth = tObj->DepthMode == GL_ALPHA &&
         (firstImage->_BaseFormat == GL_DEPTH_COMPONENT ||
          firstImage->_BaseFormat == GL_DEPTH_STENCIL);

      const int swizzle =
         unlikely(alpha_depth) ? SWIZZLE_XYZW : brw_get_texture_swizzle(tObj);

      surf->ss7.shader_channel_select_r = swizzle_to_scs(GET_SWZ(swizzle, 0));
      surf->ss7.shader_channel_select_g = swizzle_to_scs(GET_SWZ(swizzle, 1));
      surf->ss7.shader_channel_select_b = swizzle_to_scs(GET_SWZ(swizzle, 2));
      surf->ss7.shader_channel_select_a = swizzle_to_scs(GET_SWZ(swizzle, 3));
   }

   /* Emit relocation to surface contents */
   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
			   binding_table[surf_index] +
			   offsetof(struct gen7_surface_state, ss1),
			   intelObj->mt->region->bo, intelObj->mt->offset,
			   I915_GEM_DOMAIN_SAMPLER, 0);

   gen7_check_surface_setup(surf, false /* is_render_target */);
}

/**
 * Create the constant buffer surface.  Vertex/fragment shader constants will
 * be read from this buffer with Data Port Read instructions/messages.
 */
void
gen7_create_constant_surface(struct brw_context *brw,
			     drm_intel_bo *bo,
			     uint32_t offset,
			     int width,
			     uint32_t *out_offset)
{
   const GLint w = width - 1;
   struct gen7_surface_state *surf;

   surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
			  sizeof(*surf), 32, out_offset);
   memset(surf, 0, sizeof(*surf));

   surf->ss0.surface_type = BRW_SURFACE_BUFFER;
   surf->ss0.surface_format = BRW_SURFACEFORMAT_R32G32B32A32_FLOAT;

   surf->ss0.render_cache_read_write = 1;

   assert(bo);
   surf->ss1.base_addr = bo->offset + offset; /* reloc */

   surf->ss2.width = w & 0x7f;            /* bits 6:0 of size or width */
   surf->ss2.height = (w >> 7) & 0x1fff;  /* bits 19:7 of size or width */
   surf->ss3.depth = (w >> 20) & 0x7f;    /* bits 26:20 of size or width */
   surf->ss3.pitch = (16 - 1); /* stride between samples */
   gen7_set_surface_tiling(surf, I915_TILING_NONE); /* tiling now allowed */

   if (brw->intel.is_haswell) {
      surf->ss7.shader_channel_select_r = HSW_SCS_RED;
      surf->ss7.shader_channel_select_g = HSW_SCS_GREEN;
      surf->ss7.shader_channel_select_b = HSW_SCS_BLUE;
      surf->ss7.shader_channel_select_a = HSW_SCS_ALPHA;
   }

   /* Emit relocation to surface contents.  Section 5.1.1 of the gen4
    * bspec ("Data Cache") says that the data cache does not exist as
    * a separate cache and is just the sampler cache.
    */
   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
			   (*out_offset +
			    offsetof(struct gen7_surface_state, ss1)),
			   bo, offset,
			   I915_GEM_DOMAIN_SAMPLER, 0);

   gen7_check_surface_setup(surf, false /* is_render_target */);
}

static void
gen7_update_null_renderbuffer_surface(struct brw_context *brw, unsigned unit)
{
   /* From the Ivy bridge PRM, Vol4 Part1 p62 (Surface Type: Programming
    * Notes):
    *
    *     A null surface is used in instances where an actual surface is not
    *     bound. When a write message is generated to a null surface, no
    *     actual surface is written to. When a read message (including any
    *     sampling engine message) is generated to a null surface, the result
    *     is all zeros. Note that a null surface type is allowed to be used
    *     with all messages, even if it is not specificially indicated as
    *     supported. All of the remaining fields in surface state are ignored
    *     for null surfaces, with the following exceptions: Width, Height,
    *     Depth, LOD, and Render Target View Extent fields must match the
    *     depth buffer’s corresponding state for all render target surfaces,
    *     including null.
    */
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct gen7_surface_state *surf;

   /* _NEW_BUFFERS */
   const struct gl_framebuffer *fb = ctx->DrawBuffer;

   surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
			  sizeof(*surf), 32, &brw->wm.surf_offset[unit]);
   memset(surf, 0, sizeof(*surf));

   surf->ss0.surface_type = BRW_SURFACE_NULL;
   surf->ss0.surface_format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;

   surf->ss2.width = fb->Width - 1;
   surf->ss2.height = fb->Height - 1;

   /* From the Ivy bridge PRM, Vol4 Part1 p65 (Tiled Surface: Programming Notes):
    *
    *     If Surface Type is SURFTYPE_NULL, this field must be TRUE.
    */
   gen7_set_surface_tiling(surf, I915_TILING_Y);

   gen7_check_surface_setup(surf, true /* is_render_target */);
}

/**
 * Sets up a surface state structure to point at the given region.
 * While it is only used for the front/back buffer currently, it should be
 * usable for further buffers when doing ARB_draw_buffer support.
 */
static void
gen7_update_renderbuffer_surface(struct brw_context *brw,
				 struct gl_renderbuffer *rb,
				 unsigned int unit)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   struct intel_region *region = irb->mt->region;
   struct gen7_surface_state *surf;
   uint32_t tile_x, tile_y;
   gl_format rb_format = intel_rb_format(irb);

   surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
			  sizeof(*surf), 32, &brw->wm.surf_offset[unit]);
   memset(surf, 0, sizeof(*surf));

   /* Render targets can't use IMS layout */
   assert(irb->mt->msaa_layout != INTEL_MSAA_LAYOUT_IMS);

   if (irb->mt->align_h == 4)
      surf->ss0.vertical_alignment = 1;
   if (irb->mt->align_w == 8)
      surf->ss0.horizontal_alignment = 1;

   switch (rb_format) {
   case MESA_FORMAT_SARGB8:
      /* _NEW_BUFFERS
       *
       * Without GL_EXT_framebuffer_sRGB we shouldn't bind sRGB surfaces to the
       * blend/update as sRGB.
       */
      if (ctx->Color.sRGBEnabled)
	 surf->ss0.surface_format = brw_format_for_mesa_format(rb_format);
      else
	 surf->ss0.surface_format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
      break;
   default:
      assert(brw_render_target_supported(intel, rb));
      surf->ss0.surface_format = brw->render_target_format[rb_format];
      if (unlikely(!brw->format_supported_as_render_target[rb_format])) {
	 _mesa_problem(ctx, "%s: renderbuffer format %s unsupported\n",
		       __FUNCTION__, _mesa_get_format_name(rb_format));
      }
       break;
   }

   surf->ss0.surface_type = BRW_SURFACE_2D;
   surf->ss0.surface_array_spacing = irb->mt->array_spacing_lod0 ?
      GEN7_SURFACE_ARYSPC_LOD0 : GEN7_SURFACE_ARYSPC_FULL;

   /* reloc */
   surf->ss1.base_addr = intel_renderbuffer_tile_offsets(irb, &tile_x, &tile_y);
   surf->ss1.base_addr += region->bo->offset; /* reloc */

   assert(brw->has_surface_tile_offset);
   /* Note that the low bits of these fields are missing, so
    * there's the possibility of getting in trouble.
    */
   assert(tile_x % 4 == 0);
   assert(tile_y % 2 == 0);
   surf->ss5.x_offset = tile_x / 4;
   surf->ss5.y_offset = tile_y / 2;

   surf->ss2.width = rb->Width - 1;
   surf->ss2.height = rb->Height - 1;
   gen7_set_surface_tiling(surf, region->tiling);
   surf->ss3.pitch = (region->pitch * region->cpp) - 1;

   gen7_set_surface_msaa(surf, irb->mt->num_samples, irb->mt->msaa_layout);

   if (irb->mt->msaa_layout == INTEL_MSAA_LAYOUT_CMS) {
      gen7_set_surface_mcs_info(brw, surf, brw->wm.surf_offset[unit],
                                irb->mt->mcs_mt, true /* is_render_target */);
   }

   if (intel->is_haswell) {
      surf->ss7.shader_channel_select_r = HSW_SCS_RED;
      surf->ss7.shader_channel_select_g = HSW_SCS_GREEN;
      surf->ss7.shader_channel_select_b = HSW_SCS_BLUE;
      surf->ss7.shader_channel_select_a = HSW_SCS_ALPHA;
   }

   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
			   brw->wm.surf_offset[unit] +
			   offsetof(struct gen7_surface_state, ss1),
			   region->bo,
			   surf->ss1.base_addr - region->bo->offset,
			   I915_GEM_DOMAIN_RENDER,
			   I915_GEM_DOMAIN_RENDER);

   gen7_check_surface_setup(surf, true /* is_render_target */);
}

void
gen7_init_vtable_surface_functions(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   intel->vtbl.update_texture_surface = gen7_update_texture_surface;
   intel->vtbl.update_renderbuffer_surface = gen7_update_renderbuffer_surface;
   intel->vtbl.update_null_renderbuffer_surface =
      gen7_update_null_renderbuffer_surface;
   intel->vtbl.create_constant_surface = gen7_create_constant_surface;
}
