/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
 


#include "intel_batchbuffer.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

#include "main/fbobject.h"

/* Constant single cliprect for framebuffer object or DRI2 drawing */
static void upload_drawing_rect(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_DRAWING_RECTANGLE << 16 | (4 - 2));
   OUT_BATCH(0); /* xmin, ymin */
   OUT_BATCH(((ctx->DrawBuffer->Width - 1) & 0xffff) |
	    ((ctx->DrawBuffer->Height - 1) << 16));
   OUT_BATCH(0);
   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_drawing_rect = {
   .dirty = {
      .mesa = _NEW_BUFFERS,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_drawing_rect
};

/**
 * Upload the binding table pointers, which point each stage's array of surface
 * state pointers.
 *
 * The binding table pointers are relative to the surface state base address,
 * which points at the batchbuffer containing the streamed batch state.
 */
static void upload_binding_table_pointers(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(6);
   OUT_BATCH(_3DSTATE_BINDING_TABLE_POINTERS << 16 | (6 - 2));
   OUT_BATCH(brw->vs.bind_bo_offset);
   OUT_BATCH(0); /* gs */
   OUT_BATCH(0); /* clip */
   OUT_BATCH(0); /* sf */
   OUT_BATCH(brw->wm.bind_bo_offset);
   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_binding_table_pointers = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
	      BRW_NEW_STATE_BASE_ADDRESS |
	      BRW_NEW_VS_BINDING_TABLE |
	      BRW_NEW_GS_BINDING_TABLE |
	      BRW_NEW_PS_BINDING_TABLE),
      .cache = 0,
   },
   .emit = upload_binding_table_pointers,
};

/**
 * Upload the binding table pointers, which point each stage's array of surface
 * state pointers.
 *
 * The binding table pointers are relative to the surface state base address,
 * which points at the batchbuffer containing the streamed batch state.
 */
static void upload_gen6_binding_table_pointers(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_BINDING_TABLE_POINTERS << 16 |
	     GEN6_BINDING_TABLE_MODIFY_VS |
	     GEN6_BINDING_TABLE_MODIFY_GS |
	     GEN6_BINDING_TABLE_MODIFY_PS |
	     (4 - 2));
   OUT_BATCH(brw->vs.bind_bo_offset); /* vs */
   OUT_BATCH(brw->gs.bind_bo_offset); /* gs */
   OUT_BATCH(brw->wm.bind_bo_offset); /* wm/ps */
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen6_binding_table_pointers = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
	      BRW_NEW_STATE_BASE_ADDRESS |
	      BRW_NEW_VS_BINDING_TABLE |
	      BRW_NEW_GS_BINDING_TABLE |
	      BRW_NEW_PS_BINDING_TABLE),
      .cache = 0,
   },
   .emit = upload_gen6_binding_table_pointers,
};

/**
 * Upload pointers to the per-stage state.
 *
 * The state pointers in this packet are all relative to the general state
 * base address set by CMD_STATE_BASE_ADDRESS, which is 0.
 */
static void upload_pipelined_state_pointers(struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;

   if (intel->gen == 5) {
      /* Need to flush before changing clip max threads for errata. */
      BEGIN_BATCH(1);
      OUT_BATCH(MI_FLUSH);
      ADVANCE_BATCH();
   }

   BEGIN_BATCH(7);
   OUT_BATCH(_3DSTATE_PIPELINED_POINTERS << 16 | (7 - 2));
   OUT_RELOC(intel->batch.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
	     brw->vs.state_offset);
   if (brw->gs.prog_active)
      OUT_RELOC(brw->intel.batch.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
		brw->gs.state_offset | 1);
   else
      OUT_BATCH(0);
   OUT_RELOC(brw->intel.batch.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
	     brw->clip.state_offset | 1);
   OUT_RELOC(brw->intel.batch.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
	     brw->sf.state_offset);
   OUT_RELOC(brw->intel.batch.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
	     brw->wm.state_offset);
   OUT_RELOC(brw->intel.batch.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
	     brw->cc.state_offset);
   ADVANCE_BATCH();

   brw->state.dirty.brw |= BRW_NEW_PSP;
}

static void upload_psp_urb_cbs(struct brw_context *brw )
{
   upload_pipelined_state_pointers(brw);
   brw_upload_urb_fence(brw);
   brw_upload_cs_urb_state(brw);
}

const struct brw_tracked_state brw_psp_urb_cbs = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_URB_FENCE |
	      BRW_NEW_BATCH |
	      BRW_NEW_STATE_BASE_ADDRESS),
      .cache = (CACHE_NEW_VS_UNIT | 
		CACHE_NEW_GS_UNIT | 
		CACHE_NEW_GS_PROG | 
		CACHE_NEW_CLIP_UNIT | 
		CACHE_NEW_SF_UNIT | 
		CACHE_NEW_WM_UNIT | 
		CACHE_NEW_CC_UNIT)
   },
   .emit = upload_psp_urb_cbs,
};

uint32_t
brw_depthbuffer_format(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct intel_renderbuffer *drb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
   struct intel_renderbuffer *srb;

   if (!drb &&
       (srb = intel_get_renderbuffer(fb, BUFFER_STENCIL)) &&
       !srb->mt->stencil_mt &&
       (intel_rb_format(srb) == MESA_FORMAT_S8_Z24 ||
	intel_rb_format(srb) == MESA_FORMAT_Z32_FLOAT_X24S8)) {
      drb = srb;
   }

   if (!drb)
      return BRW_DEPTHFORMAT_D32_FLOAT;

   switch (drb->mt->format) {
   case MESA_FORMAT_Z16:
      return BRW_DEPTHFORMAT_D16_UNORM;
   case MESA_FORMAT_Z32_FLOAT:
      return BRW_DEPTHFORMAT_D32_FLOAT;
   case MESA_FORMAT_X8_Z24:
      if (intel->gen >= 6) {
	 return BRW_DEPTHFORMAT_D24_UNORM_X8_UINT;
      } else {
	 /* Use D24_UNORM_S8, not D24_UNORM_X8.
	  *
	  * D24_UNORM_X8 was not introduced until Gen5. (See the Ironlake PRM,
	  * Volume 2, Part 1, Section 8.4.6 "Depth/Stencil Buffer State", Bits
	  * 3DSTATE_DEPTH_BUFFER.Surface_Format).
	  *
	  * However, on Gen5, D24_UNORM_X8 may be used only if separate
	  * stencil is enabled, and we never enable it. From the Ironlake PRM,
	  * same section as above, Bit 3DSTATE_DEPTH_BUFFER.Separate_Stencil_Buffer_Enable:
	  *     If this field is disabled, the Surface Format of the depth
	  *     buffer cannot be D24_UNORM_X8_UINT.
	  */
	 return BRW_DEPTHFORMAT_D24_UNORM_S8_UINT;
      }
   case MESA_FORMAT_S8_Z24:
      return BRW_DEPTHFORMAT_D24_UNORM_S8_UINT;
   case MESA_FORMAT_Z32_FLOAT_X24S8:
      return BRW_DEPTHFORMAT_D32_FLOAT_S8X24_UINT;
   default:
      _mesa_problem(ctx, "Unexpected depth format %s\n",
		    _mesa_get_format_name(intel_rb_format(drb)));
      return BRW_DEPTHFORMAT_D16_UNORM;
   }
}

static void emit_depthbuffer(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   /* _NEW_BUFFERS */
   struct intel_renderbuffer *depth_irb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
   struct intel_renderbuffer *stencil_irb = intel_get_renderbuffer(fb, BUFFER_STENCIL);
   struct intel_mipmap_tree *stencil_mt = NULL;
   struct intel_region *hiz_region = NULL;
   unsigned int len;
   bool separate_stencil = false;

   /* Amount by which drawing should be offset in order to draw to the
    * appropriate miplevel/zoffset/cubeface.  We will extract these values
    * from depth_irb or stencil_irb once we determine which is present.
    */
   uint32_t draw_x = 0, draw_y = 0;

   /* Masks used to determine how much of the draw_x and draw_y offsets should
    * be performed using the fine adjustment of "depth coordinate offset X/Y"
    * (dw5 of 3DSTATE_DEPTH_BUFFER).  Any remaining coarse adjustment will be
    * performed by changing the base addresses of the buffers.
    *
    * Since the HiZ, depth, and stencil buffers all use the same "depth
    * coordinate offset X/Y" values, we need to make sure that the coarse
    * adjustment will be possible to apply to all three buffers.  Since coarse
    * adjustment can only be applied in multiples of the tile size, we will OR
    * together the tile masks of all the buffers to determine which offsets to
    * perform as fine adjustments.
    */
   uint32_t tile_mask_x = 0, tile_mask_y = 0;

   if (depth_irb) {
      intel_region_get_tile_masks(depth_irb->mt->region,
                                  &tile_mask_x, &tile_mask_y, false);
   }

   if (depth_irb &&
       depth_irb->mt &&
       depth_irb->mt->hiz_mt) {
      hiz_region = depth_irb->mt->hiz_mt->region;

      uint32_t hiz_tile_mask_x, hiz_tile_mask_y;
      intel_region_get_tile_masks(hiz_region,
                                  &hiz_tile_mask_x, &hiz_tile_mask_y, false);

      /* Each HiZ row represents 2 rows of pixels */
      hiz_tile_mask_y = hiz_tile_mask_y << 1 | 1;

      tile_mask_x |= hiz_tile_mask_x;
      tile_mask_y |= hiz_tile_mask_y;
   }

   /* 3DSTATE_DEPTH_BUFFER, 3DSTATE_STENCIL_BUFFER are both
    * non-pipelined state that will need the PIPE_CONTROL workaround.
    */
   if (intel->gen == 6) {
      intel_emit_post_sync_nonzero_flush(intel);
      intel_emit_depth_stall_flushes(intel);
   }

   /* Find the real separate stencil mt if present. */
   if (stencil_irb) {
      stencil_mt = stencil_irb->mt;
      if (stencil_mt->stencil_mt)
	 stencil_mt = stencil_mt->stencil_mt;

      if (stencil_mt->format == MESA_FORMAT_S8) {
	 separate_stencil = true;

         /* Separate stencil buffer uses 64x64 tiles. */
         tile_mask_x |= 63;
         tile_mask_y |= 63;
      } else {
         uint32_t stencil_tile_mask_x, stencil_tile_mask_y;
         intel_region_get_tile_masks(stencil_mt->region,
                                     &stencil_tile_mask_x,
                                     &stencil_tile_mask_y, false);

         tile_mask_x |= stencil_tile_mask_x;
         tile_mask_y |= stencil_tile_mask_y;
      }
   }

   /* If there's a packed depth/stencil bound to stencil only, we need to
    * emit the packed depth/stencil buffer packet.
    */
   if (!depth_irb && stencil_irb && !separate_stencil)
      depth_irb = stencil_irb;

   if (intel->gen >= 6)
      len = 7;
   else if (intel->is_g4x || intel->gen == 5)
      len = 6;
   else
      len = 5;

   if (!depth_irb && !separate_stencil) {
      BEGIN_BATCH(len);
      OUT_BATCH(_3DSTATE_DEPTH_BUFFER << 16 | (len - 2));
      OUT_BATCH((BRW_DEPTHFORMAT_D32_FLOAT << 18) |
		(BRW_SURFACE_NULL << 29));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);

      if (intel->is_g4x || intel->gen >= 5)
         OUT_BATCH(0);

      if (intel->gen >= 6)
	 OUT_BATCH(0);

      ADVANCE_BATCH();

   } else if (!depth_irb && separate_stencil) {
      uint32_t tile_x, tile_y;

      /*
       * There exists a separate stencil buffer but no depth buffer.
       *
       * The stencil buffer inherits most of its fields from
       * 3DSTATE_DEPTH_BUFFER: namely the tile walk, surface type, width, and
       * height.
       *
       * Enable the hiz bit because it and the separate stencil bit must have
       * the same value. From Section 2.11.5.6.1.1 3DSTATE_DEPTH_BUFFER, Bit
       * 1.21 "Separate Stencil Enable":
       *     [DevIL]: If this field is enabled, Hierarchical Depth Buffer
       *     Enable must also be enabled.
       *
       *     [DevGT]: This field must be set to the same value (enabled or
       *     disabled) as Hierarchical Depth Buffer Enable
       *
       * The tiled bit must be set. From the Sandybridge PRM, Volume 2, Part 1,
       * Section 7.5.5.1.1 3DSTATE_DEPTH_BUFFER, Bit 1.27 Tiled Surface:
       *     [DevGT+]: This field must be set to TRUE.
       */
      assert(intel->has_separate_stencil);

      draw_x = stencil_irb->draw_x;
      draw_y = stencil_irb->draw_y;
      tile_x = draw_x & tile_mask_x;
      tile_y = draw_y & tile_mask_y;

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
      if (intel->gen >= 6) {
         tile_x &= ~7;
         tile_y &= ~7;
      }

      BEGIN_BATCH(len);
      OUT_BATCH(_3DSTATE_DEPTH_BUFFER << 16 | (len - 2));
      OUT_BATCH((BRW_DEPTHFORMAT_D32_FLOAT << 18) |
	        (1 << 21) | /* separate stencil enable */
	        (1 << 22) | /* hiz enable */
	        (BRW_TILEWALK_YMAJOR << 26) |
	        (1 << 27) | /* tiled surface */
	        (BRW_SURFACE_2D << 29));
      OUT_BATCH(0);
      OUT_BATCH(((stencil_irb->Base.Base.Width + tile_x - 1) << 6) |
	         (stencil_irb->Base.Base.Height + tile_y - 1) << 19);
      OUT_BATCH(0);

      if (intel->is_g4x || intel->gen >= 5)
         OUT_BATCH(tile_x | (tile_y << 16));
      else
	 assert(tile_x == 0 && tile_y == 0);

      if (intel->gen >= 6)
	 OUT_BATCH(0);

      ADVANCE_BATCH();

   } else {
      struct intel_region *region = depth_irb->mt->region;
      uint32_t tile_x, tile_y, offset;

      /* If using separate stencil, hiz must be enabled. */
      assert(!separate_stencil || hiz_region);

      assert(intel->gen < 6 || region->tiling == I915_TILING_Y);
      assert(!hiz_region || region->tiling == I915_TILING_Y);

      draw_x = depth_irb->draw_x;
      draw_y = depth_irb->draw_y;
      tile_x = draw_x & tile_mask_x;
      tile_y = draw_y & tile_mask_y;

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

      offset = intel_region_get_aligned_offset(region,
                                               draw_x & ~tile_mask_x,
                                               draw_y & ~tile_mask_y, false);

      BEGIN_BATCH(len);
      OUT_BATCH(_3DSTATE_DEPTH_BUFFER << 16 | (len - 2));
      OUT_BATCH(((region->pitch * region->cpp) - 1) |
		(brw_depthbuffer_format(brw) << 18) |
		((hiz_region ? 1 : 0) << 21) | /* separate stencil enable */
		((hiz_region ? 1 : 0) << 22) | /* hiz enable */
		(BRW_TILEWALK_YMAJOR << 26) |
		((region->tiling != I915_TILING_NONE) << 27) |
		(BRW_SURFACE_2D << 29));
      OUT_RELOC(region->bo,
		I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		offset);
      OUT_BATCH((BRW_SURFACE_MIPMAPLAYOUT_BELOW << 1) |
		(((depth_irb->Base.Base.Width + tile_x) - 1) << 6) |
		(((depth_irb->Base.Base.Height + tile_y) - 1) << 19));
      OUT_BATCH(0);

      if (intel->is_g4x || intel->gen >= 5)
         OUT_BATCH(tile_x | (tile_y << 16));
      else
	 assert(tile_x == 0 && tile_y == 0);

      if (intel->gen >= 6)
	 OUT_BATCH(0);

      ADVANCE_BATCH();
   }

   if (hiz_region || separate_stencil) {
      /*
       * In the 3DSTATE_DEPTH_BUFFER batch emitted above, the 'separate
       * stencil enable' and 'hiz enable' bits were set. Therefore we must
       * emit 3DSTATE_HIER_DEPTH_BUFFER and 3DSTATE_STENCIL_BUFFER. Even if
       * there is no stencil buffer, 3DSTATE_STENCIL_BUFFER must be emitted;
       * failure to do so causes hangs on gen5 and a stall on gen6.
       */

      /* Emit hiz buffer. */
      if (hiz_region) {
         uint32_t hiz_offset =
            intel_region_get_aligned_offset(hiz_region,
                                            draw_x & ~tile_mask_x,
                                            (draw_y & ~tile_mask_y) / 2,
                                            false);

	 BEGIN_BATCH(3);
	 OUT_BATCH((_3DSTATE_HIER_DEPTH_BUFFER << 16) | (3 - 2));
	 OUT_BATCH(hiz_region->pitch * hiz_region->cpp - 1);
	 OUT_RELOC(hiz_region->bo,
		   I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		   hiz_offset);
	 ADVANCE_BATCH();
      } else {
	 BEGIN_BATCH(3);
	 OUT_BATCH((_3DSTATE_HIER_DEPTH_BUFFER << 16) | (3 - 2));
	 OUT_BATCH(0);
	 OUT_BATCH(0);
	 ADVANCE_BATCH();
      }

      /* Emit stencil buffer. */
      if (separate_stencil) {
	 struct intel_region *region = stencil_mt->region;

         /* Note: we can't compute the stencil offset using
          * intel_region_get_aligned_offset(), because stencil_region claims
          * that the region is untiled; in fact it's W tiled.
          */
         uint32_t stencil_offset =
            (draw_y & ~tile_mask_y) * region->pitch +
            (draw_x & ~tile_mask_x) * 64;

	 BEGIN_BATCH(3);
	 OUT_BATCH((_3DSTATE_STENCIL_BUFFER << 16) | (3 - 2));
         /* The stencil buffer has quirky pitch requirements.  From Vol 2a,
          * 11.5.6.2.1 3DSTATE_STENCIL_BUFFER, field "Surface Pitch":
          *    The pitch must be set to 2x the value computed based on width, as
          *    the stencil buffer is stored with two rows interleaved.
          */
	 OUT_BATCH(2 * region->pitch * region->cpp - 1);
	 OUT_RELOC(region->bo,
		   I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		   stencil_offset);
	 ADVANCE_BATCH();
      } else {
	 BEGIN_BATCH(3);
	 OUT_BATCH((_3DSTATE_STENCIL_BUFFER << 16) | (3 - 2));
	 OUT_BATCH(0);
	 OUT_BATCH(0);
	 ADVANCE_BATCH();
      }
   }

   /*
    * On Gen >= 6, emit clear params for safety. If using hiz, then clear
    * params must be emitted.
    *
    * From Section 2.11.5.6.4.1 3DSTATE_CLEAR_PARAMS:
    *     3DSTATE_CLEAR_PARAMS packet must follow the DEPTH_BUFFER_STATE packet
    *     when HiZ is enabled and the DEPTH_BUFFER_STATE changes.
    */
   if (intel->gen >= 6 || hiz_region) {
      if (intel->gen == 6)
	 intel_emit_post_sync_nonzero_flush(intel);

      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_CLEAR_PARAMS << 16 |
		GEN5_DEPTH_CLEAR_VALID |
		(2 - 2));
      OUT_BATCH(depth_irb ? depth_irb->mt->depth_clear_value : 0);
      ADVANCE_BATCH();
   }
}

const struct brw_tracked_state brw_depthbuffer = {
   .dirty = {
      .mesa = _NEW_BUFFERS,
      .brw = BRW_NEW_BATCH,
      .cache = 0,
   },
   .emit = emit_depthbuffer,
};



/***********************************************************************
 * Polygon stipple packet
 */

static void upload_polygon_stipple(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &brw->intel.ctx;
   GLuint i;

   /* _NEW_POLYGON */
   if (!ctx->Polygon.StippleFlag)
      return;

   if (intel->gen == 6)
      intel_emit_post_sync_nonzero_flush(intel);

   BEGIN_BATCH(33);
   OUT_BATCH(_3DSTATE_POLY_STIPPLE_PATTERN << 16 | (33 - 2));

   /* Polygon stipple is provided in OpenGL order, i.e. bottom
    * row first.  If we're rendering to a window (i.e. the
    * default frame buffer object, 0), then we need to invert
    * it to match our pixel layout.  But if we're rendering
    * to a FBO (i.e. any named frame buffer object), we *don't*
    * need to invert - we already match the layout.
    */
   if (_mesa_is_winsys_fbo(ctx->DrawBuffer)) {
      for (i = 0; i < 32; i++)
	  OUT_BATCH(ctx->PolygonStipple[31 - i]); /* invert */
   }
   else {
      for (i = 0; i < 32; i++)
	 OUT_BATCH(ctx->PolygonStipple[i]);
   }
   CACHED_BATCH();
}

const struct brw_tracked_state brw_polygon_stipple = {
   .dirty = {
      .mesa = (_NEW_POLYGONSTIPPLE |
	       _NEW_POLYGON),
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_polygon_stipple
};


/***********************************************************************
 * Polygon stipple offset packet
 */

static void upload_polygon_stipple_offset(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &brw->intel.ctx;

   /* _NEW_POLYGON */
   if (!ctx->Polygon.StippleFlag)
      return;

   if (intel->gen == 6)
      intel_emit_post_sync_nonzero_flush(intel);

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_POLY_STIPPLE_OFFSET << 16 | (2-2));

   /* _NEW_BUFFERS
    *
    * If we're drawing to a system window we have to invert the Y axis
    * in order to match the OpenGL pixel coordinate system, and our
    * offset must be matched to the window position.  If we're drawing
    * to a user-created FBO then our native pixel coordinate system
    * works just fine, and there's no window system to worry about.
    */
   if (_mesa_is_winsys_fbo(brw->intel.ctx.DrawBuffer))
      OUT_BATCH((32 - (ctx->DrawBuffer->Height & 31)) & 31);
   else
      OUT_BATCH(0);
   CACHED_BATCH();
}

const struct brw_tracked_state brw_polygon_stipple_offset = {
   .dirty = {
      .mesa = (_NEW_BUFFERS |
	       _NEW_POLYGON),
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_polygon_stipple_offset
};

/**********************************************************************
 * AA Line parameters
 */
static void upload_aa_line_parameters(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &brw->intel.ctx;

   if (!ctx->Line.SmoothFlag || !brw->has_aa_line_parameters)
      return;

   if (intel->gen == 6)
      intel_emit_post_sync_nonzero_flush(intel);

   OUT_BATCH(_3DSTATE_AA_LINE_PARAMETERS << 16 | (3 - 2));
   /* use legacy aa line coverage computation */
   OUT_BATCH(0);
   OUT_BATCH(0);
   CACHED_BATCH();
}

const struct brw_tracked_state brw_aa_line_parameters = {
   .dirty = {
      .mesa = _NEW_LINE,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_aa_line_parameters
};

/***********************************************************************
 * Line stipple packet
 */

static void upload_line_stipple(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &brw->intel.ctx;
   GLfloat tmp;
   GLint tmpi;

   if (!ctx->Line.StippleFlag)
      return;

   if (intel->gen == 6)
      intel_emit_post_sync_nonzero_flush(intel);

   BEGIN_BATCH(3);
   OUT_BATCH(_3DSTATE_LINE_STIPPLE_PATTERN << 16 | (3 - 2));
   OUT_BATCH(ctx->Line.StipplePattern);
   tmp = 1.0 / (GLfloat) ctx->Line.StippleFactor;
   tmpi = tmp * (1<<13);
   OUT_BATCH(tmpi << 16 | ctx->Line.StippleFactor);
   CACHED_BATCH();
}

const struct brw_tracked_state brw_line_stipple = {
   .dirty = {
      .mesa = _NEW_LINE,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_line_stipple
};


/***********************************************************************
 * Misc invariant state packets
 */

static void upload_invariant_state( struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;

   /* 3DSTATE_SIP, 3DSTATE_MULTISAMPLE, etc. are nonpipelined. */
   if (intel->gen == 6)
      intel_emit_post_sync_nonzero_flush(intel);

   /* Select the 3D pipeline (as opposed to media) */
   BEGIN_BATCH(1);
   OUT_BATCH(brw->CMD_PIPELINE_SELECT << 16 | 0);
   ADVANCE_BATCH();

   if (intel->gen < 6) {
      /* Disable depth offset clamping. */
      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_GLOBAL_DEPTH_OFFSET_CLAMP << 16 | (2 - 2));
      OUT_BATCH_F(0.0);
      ADVANCE_BATCH();
   }

   if (intel->gen == 6) {
      int i;

      for (i = 0; i < 4; i++) {
         BEGIN_BATCH(4);
         OUT_BATCH(_3DSTATE_GS_SVB_INDEX << 16 | (4 - 2));
         OUT_BATCH(i << SVB_INDEX_SHIFT);
         OUT_BATCH(0);
         OUT_BATCH(0xffffffff);
         ADVANCE_BATCH();
      }
   }

   BEGIN_BATCH(2);
   OUT_BATCH(CMD_STATE_SIP << 16 | (2 - 2));
   OUT_BATCH(0);
   ADVANCE_BATCH();

   BEGIN_BATCH(1);
   OUT_BATCH(brw->CMD_VF_STATISTICS << 16 |
	     (unlikely(INTEL_DEBUG & DEBUG_STATS) ? 1 : 0));
   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_invariant_state = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_invariant_state
};

/**
 * Define the base addresses which some state is referenced from.
 *
 * This allows us to avoid having to emit relocations for the objects,
 * and is actually required for binding table pointers on gen6.
 *
 * Surface state base address covers binding table pointers and
 * surface state objects, but not the surfaces that the surface state
 * objects point to.
 */
static void upload_state_base_address( struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;

   /* FINISHME: According to section 3.6.1 "STATE_BASE_ADDRESS" of
    * vol1a of the G45 PRM, MI_FLUSH with the ISC invalidate should be
    * programmed prior to STATE_BASE_ADDRESS.
    *
    * However, given that the instruction SBA (general state base
    * address) on this chipset is always set to 0 across X and GL,
    * maybe this isn't required for us in particular.
    */

   if (intel->gen >= 6) {
      if (intel->gen == 6)
	 intel_emit_post_sync_nonzero_flush(intel);

       BEGIN_BATCH(10);
       OUT_BATCH(CMD_STATE_BASE_ADDRESS << 16 | (10 - 2));
       /* General state base address: stateless DP read/write requests */
       OUT_BATCH(1);
       /* Surface state base address:
	* BINDING_TABLE_STATE
	* SURFACE_STATE
	*/
       OUT_RELOC(intel->batch.bo, I915_GEM_DOMAIN_SAMPLER, 0, 1);
        /* Dynamic state base address:
	 * SAMPLER_STATE
	 * SAMPLER_BORDER_COLOR_STATE
	 * CLIP, SF, WM/CC viewport state
	 * COLOR_CALC_STATE
	 * DEPTH_STENCIL_STATE
	 * BLEND_STATE
	 * Push constants (when INSTPM: CONSTANT_BUFFER Address Offset
	 * Disable is clear, which we rely on)
	 */
       OUT_RELOC(intel->batch.bo, (I915_GEM_DOMAIN_RENDER |
				   I915_GEM_DOMAIN_INSTRUCTION), 0, 1);

       OUT_BATCH(1); /* Indirect object base address: MEDIA_OBJECT data */
       OUT_RELOC(brw->cache.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
		 1); /* Instruction base address: shader kernels (incl. SIP) */

       OUT_BATCH(1); /* General state upper bound */
       /* Dynamic state upper bound.  Although the documentation says that
	* programming it to zero will cause it to be ignored, that is a lie.
	* If this isn't programmed to a real bound, the sampler border color
	* pointer is rejected, causing border color to mysteriously fail.
	*/
       OUT_BATCH(0xfffff001);
       OUT_BATCH(1); /* Indirect object upper bound */
       OUT_BATCH(1); /* Instruction access upper bound */
       ADVANCE_BATCH();
   } else if (intel->gen == 5) {
       BEGIN_BATCH(8);
       OUT_BATCH(CMD_STATE_BASE_ADDRESS << 16 | (8 - 2));
       OUT_BATCH(1); /* General state base address */
       OUT_RELOC(intel->batch.bo, I915_GEM_DOMAIN_SAMPLER, 0,
		 1); /* Surface state base address */
       OUT_BATCH(1); /* Indirect object base address */
       OUT_RELOC(brw->cache.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
		 1); /* Instruction base address */
       OUT_BATCH(0xfffff001); /* General state upper bound */
       OUT_BATCH(1); /* Indirect object upper bound */
       OUT_BATCH(1); /* Instruction access upper bound */
       ADVANCE_BATCH();
   } else {
       BEGIN_BATCH(6);
       OUT_BATCH(CMD_STATE_BASE_ADDRESS << 16 | (6 - 2));
       OUT_BATCH(1); /* General state base address */
       OUT_RELOC(intel->batch.bo, I915_GEM_DOMAIN_SAMPLER, 0,
		 1); /* Surface state base address */
       OUT_BATCH(1); /* Indirect object base address */
       OUT_BATCH(1); /* General state upper bound */
       OUT_BATCH(1); /* Indirect object upper bound */
       ADVANCE_BATCH();
   }

   /* According to section 3.6.1 of VOL1 of the 965 PRM,
    * STATE_BASE_ADDRESS updates require a reissue of:
    *
    * 3DSTATE_PIPELINE_POINTERS
    * 3DSTATE_BINDING_TABLE_POINTERS
    * MEDIA_STATE_POINTERS
    *
    * and this continues through Ironlake.  The Sandy Bridge PRM, vol
    * 1 part 1 says that the folowing packets must be reissued:
    *
    * 3DSTATE_CC_POINTERS
    * 3DSTATE_BINDING_TABLE_POINTERS
    * 3DSTATE_SAMPLER_STATE_POINTERS
    * 3DSTATE_VIEWPORT_STATE_POINTERS
    * MEDIA_STATE_POINTERS
    *
    * Those are always reissued following SBA updates anyway (new
    * batch time), except in the case of the program cache BO
    * changing.  Having a separate state flag makes the sequence more
    * obvious.
    */

   brw->state.dirty.brw |= BRW_NEW_STATE_BASE_ADDRESS;
}

const struct brw_tracked_state brw_state_base_address = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
	      BRW_NEW_PROGRAM_CACHE),
      .cache = 0,
   },
   .emit = upload_state_base_address
};
