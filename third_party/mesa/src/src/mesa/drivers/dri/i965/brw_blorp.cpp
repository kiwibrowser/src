/*
 * Copyright © 2012 Intel Corporation
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

#include "intel_fbo.h"

#include "brw_blorp.h"
#include "brw_defines.h"
#include "gen6_blorp.h"
#include "gen7_blorp.h"

brw_blorp_mip_info::brw_blorp_mip_info()
   : mt(NULL),
     width(0),
     height(0),
     x_offset(0),
     y_offset(0)
{
}

brw_blorp_surface_info::brw_blorp_surface_info()
   : map_stencil_as_y_tiled(false),
     num_samples(0)
{
}

void
brw_blorp_mip_info::set(struct intel_mipmap_tree *mt,
                        unsigned int level, unsigned int layer)
{
   intel_miptree_check_level_layer(mt, level, layer);

   this->mt = mt;
   this->width = mt->level[level].width;
   this->height = mt->level[level].height;

   /* Construct a dummy renderbuffer just to extract tile offsets. */
   struct intel_renderbuffer rb;
   rb.mt = mt;
   rb.mt_level = level;
   rb.mt_layer = layer;
   intel_renderbuffer_set_draw_offset(&rb);
   x_offset = rb.draw_x;
   y_offset = rb.draw_y;
}

void
brw_blorp_surface_info::set(struct brw_context *brw,
                            struct intel_mipmap_tree *mt,
                            unsigned int level, unsigned int layer)
{
   brw_blorp_mip_info::set(mt, level, layer);
   this->num_samples = mt->num_samples;
   this->array_spacing_lod0 = mt->array_spacing_lod0;
   this->map_stencil_as_y_tiled = false;
   this->msaa_layout = mt->msaa_layout;

   switch (mt->format) {
   case MESA_FORMAT_S8:
      /* The miptree is a W-tiled stencil buffer.  Surface states can't be set
       * up for W tiling, so we'll need to use Y tiling and have the WM
       * program swizzle the coordinates.
       */
      this->map_stencil_as_y_tiled = true;
      this->brw_surfaceformat = BRW_SURFACEFORMAT_R8_UNORM;
      break;
   case MESA_FORMAT_X8_Z24:
   case MESA_FORMAT_Z32_FLOAT:
      /* The miptree consists of 32 bits per pixel, arranged either as 24-bit
       * depth values interleaved with 8 "don't care" bits, or as 32-bit
       * floating point depth values.  Since depth values don't require any
       * blending, it doesn't matter how we interpret the bit pattern as long
       * as we copy the right amount of data, so just map it as 8-bit BGRA.
       */
      this->brw_surfaceformat = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
      break;
   case MESA_FORMAT_Z16:
      /* The miptree consists of 16 bits per pixel of depth data.  Since depth
       * values don't require any blending, it doesn't matter how we interpret
       * the bit pattern as long as we copy the right amount of data, so just
       * map is as 8-bit RG.
       */
      this->brw_surfaceformat = BRW_SURFACEFORMAT_R8G8_UNORM;
      break;
   default:
      /* Blorp blits don't support any sort of format conversion (except
       * between sRGB and linear), so we can safely assume that the format is
       * supported as a render target, even if this is the source image.  So
       * we can convert to a surface format using brw->render_target_format.
       */
      assert(brw->format_supported_as_render_target[mt->format]);
      this->brw_surfaceformat = brw->render_target_format[mt->format];
      break;
   }
}


/**
 * Split x_offset and y_offset into a base offset (in bytes) and a remaining
 * x/y offset (in pixels).  Note: we can't do this by calling
 * intel_renderbuffer_tile_offsets(), because the offsets may have been
 * adjusted to account for Y vs. W tiling differences.  So we compute it
 * directly from the adjusted offsets.
 */
uint32_t
brw_blorp_surface_info::compute_tile_offsets(uint32_t *tile_x,
                                             uint32_t *tile_y) const
{
   struct intel_region *region = mt->region;
   uint32_t mask_x, mask_y;

   intel_region_get_tile_masks(region, &mask_x, &mask_y,
                               map_stencil_as_y_tiled);

   *tile_x = x_offset & mask_x;
   *tile_y = y_offset & mask_y;

   return intel_region_get_aligned_offset(region, x_offset & ~mask_x,
                                          y_offset & ~mask_y,
                                          map_stencil_as_y_tiled);
}


brw_blorp_params::brw_blorp_params()
   : x0(0),
     y0(0),
     x1(0),
     y1(0),
     depth_format(0),
     hiz_op(GEN6_HIZ_OP_NONE),
     num_samples(0),
     use_wm_prog(false)
{
}

extern "C" {
void
intel_hiz_exec(struct intel_context *intel, struct intel_mipmap_tree *mt,
	       unsigned int level, unsigned int layer, gen6_hiz_op op)
{
   brw_hiz_op_params params(mt, level, layer, op);
   brw_blorp_exec(intel, &params);
}

} /* extern "C" */

void
brw_blorp_exec(struct intel_context *intel, const brw_blorp_params *params)
{
   switch (intel->gen) {
   case 6:
      gen6_blorp_exec(intel, params);
      break;
   case 7:
      gen7_blorp_exec(intel, params);
      break;
   default:
      /* BLORP is not supported before Gen6. */
      assert(false);
      break;
   }
}

brw_hiz_op_params::brw_hiz_op_params(struct intel_mipmap_tree *mt,
                                     unsigned int level,
                                     unsigned int layer,
                                     gen6_hiz_op op)
{
   this->hiz_op = op;

   depth.set(mt, level, layer);
   x1 = depth.width;
   y1 = depth.height;

   assert(mt->hiz_mt != NULL);

   switch (mt->format) {
   case MESA_FORMAT_Z16:       depth_format = BRW_DEPTHFORMAT_D16_UNORM; break;
   case MESA_FORMAT_Z32_FLOAT: depth_format = BRW_DEPTHFORMAT_D32_FLOAT; break;
   case MESA_FORMAT_X8_Z24:    depth_format = BRW_DEPTHFORMAT_D24_UNORM_X8_UINT; break;
   default:                    assert(0); break;
   }
}

uint32_t
brw_hiz_op_params::get_wm_prog(struct brw_context *brw,
                               brw_blorp_prog_data **prog_data) const
{
   return 0;
}
