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
brw_upload_gs_unit(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct brw_gs_unit_state *gs;

   gs = brw_state_batch(brw, AUB_TRACE_GS_STATE,
			sizeof(*gs), 32, &brw->gs.state_offset);

   memset(gs, 0, sizeof(*gs));

   /* BRW_NEW_PROGRAM_CACHE | CACHE_NEW_GS_PROG */
   if (brw->gs.prog_active) {
      gs->thread0.grf_reg_count = (ALIGN(brw->gs.prog_data->total_grf, 16) /
				   16 - 1);

      gs->thread0.kernel_start_pointer =
	 brw_program_reloc(brw,
			   brw->gs.state_offset +
			   offsetof(struct brw_gs_unit_state, thread0),
			   brw->gs.prog_offset +
			   (gs->thread0.grf_reg_count << 1)) >> 6;

      gs->thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;
      gs->thread1.single_program_flow = 1;

      gs->thread3.dispatch_grf_start_reg = 1;
      gs->thread3.const_urb_entry_read_offset = 0;
      gs->thread3.const_urb_entry_read_length = 0;
      gs->thread3.urb_entry_read_offset = 0;
      gs->thread3.urb_entry_read_length = brw->gs.prog_data->urb_read_length;

      /* BRW_NEW_URB_FENCE */
      gs->thread4.nr_urb_entries = brw->urb.nr_gs_entries;
      gs->thread4.urb_entry_allocation_size = brw->urb.vsize - 1;

      if (brw->urb.nr_gs_entries >= 8)
	 gs->thread4.max_threads = 1;
      else
	 gs->thread4.max_threads = 0;
   }

   if (intel->gen == 5)
      gs->thread4.rendering_enable = 1;

   if (unlikely(INTEL_DEBUG & DEBUG_STATS))
      gs->thread4.stats_enable = 1;

   brw->state.dirty.cache |= CACHE_NEW_GS_UNIT;
}

const struct brw_tracked_state brw_gs_unit = {
   .dirty = {
      .mesa  = 0,
      .brw   = (BRW_NEW_BATCH |
		BRW_NEW_PROGRAM_CACHE |
		BRW_NEW_CURBE_OFFSETS |
		BRW_NEW_URB_FENCE),
      .cache = CACHE_NEW_GS_PROG
   },
   .emit = brw_upload_gs_unit,
};
