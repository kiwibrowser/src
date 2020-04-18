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

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

static void
brw_upload_clip_unit(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct brw_clip_unit_state *clip;

   clip = brw_state_batch(brw, AUB_TRACE_CLIP_STATE,
			  sizeof(*clip), 32, &brw->clip.state_offset);
   memset(clip, 0, sizeof(*clip));

   /* BRW_NEW_PROGRAM_CACHE | CACHE_NEW_CLIP_PROG */
   clip->thread0.grf_reg_count = (ALIGN(brw->clip.prog_data->total_grf, 16) /
				 16 - 1);
   clip->thread0.kernel_start_pointer =
      brw_program_reloc(brw,
			brw->clip.state_offset +
			offsetof(struct brw_clip_unit_state, thread0),
			brw->clip.prog_offset +
			(clip->thread0.grf_reg_count << 1)) >> 6;

   clip->thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;
   clip->thread1.single_program_flow = 1;

   clip->thread3.urb_entry_read_length = brw->clip.prog_data->urb_read_length;
   clip->thread3.const_urb_entry_read_length =
      brw->clip.prog_data->curb_read_length;

   /* BRW_NEW_CURBE_OFFSETS */
   clip->thread3.const_urb_entry_read_offset = brw->curbe.clip_start * 2;
   clip->thread3.dispatch_grf_start_reg = 1;
   clip->thread3.urb_entry_read_offset = 0;

   /* BRW_NEW_URB_FENCE */
   clip->thread4.nr_urb_entries = brw->urb.nr_clip_entries;
   clip->thread4.urb_entry_allocation_size = brw->urb.vsize - 1;
   /* If we have enough clip URB entries to run two threads, do so.
    */
   if (brw->urb.nr_clip_entries >= 10) {
      /* Half of the URB entries go to each thread, and it has to be an
       * even number.
       */
      assert(brw->urb.nr_clip_entries % 2 == 0);
      
      /* Although up to 16 concurrent Clip threads are allowed on Ironlake,
       * only 2 threads can output VUEs at a time.
       */
      if (intel->gen == 5)
         clip->thread4.max_threads = 16 - 1;
      else
         clip->thread4.max_threads = 2 - 1;
   } else {
      assert(brw->urb.nr_clip_entries >= 5);
      clip->thread4.max_threads = 1 - 1;
   }

   if (unlikely(INTEL_DEBUG & DEBUG_STATS))
      clip->thread4.stats_enable = 1;

   clip->clip5.userclip_enable_flags = 0x7f;
   clip->clip5.userclip_must_clip = 1;
   clip->clip5.guard_band_enable = 0;
   /* _NEW_TRANSFORM */
   if (!ctx->Transform.DepthClamp)
      clip->clip5.viewport_z_clip_enable = 1;
   clip->clip5.viewport_xy_clip_enable = 1;
   clip->clip5.vertex_position_space = BRW_CLIP_NDCSPACE;
   clip->clip5.api_mode = BRW_CLIP_API_OGL;
   clip->clip5.clip_mode = brw->clip.prog_data->clip_mode;

   if (intel->is_g4x)
      clip->clip5.negative_w_clip_test = 1;

   clip->clip6.clipper_viewport_state_ptr = 0;
   clip->viewport_xmin = -1;
   clip->viewport_xmax = 1;
   clip->viewport_ymin = -1;
   clip->viewport_ymax = 1;

   brw->state.dirty.cache |= CACHE_NEW_CLIP_UNIT;
}

const struct brw_tracked_state brw_clip_unit = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM,
      .brw   = (BRW_NEW_BATCH |
		BRW_NEW_PROGRAM_CACHE |
		BRW_NEW_CURBE_OFFSETS |
		BRW_NEW_URB_FENCE),
      .cache = CACHE_NEW_CLIP_PROG
   },
   .emit = brw_upload_clip_unit,
};
