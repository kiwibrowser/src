/*
 * Copyright © 2009 Intel Corporation
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
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "main/macros.h"
#include "intel_batchbuffer.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

/**
 * When the GS is not in use, we assign the entire URB space to the VS.  When
 * the GS is in use, we split the URB space evenly between the VS and the GS.
 * This is not ideal, but it's simple.
 *
 *           URB size / 2                   URB size / 2
 *   _____________-______________   _____________-______________
 *  /                            \ /                            \
 * +-------------------------------------------------------------+
 * | Vertex Shader Entries        | Geometry Shader Entries      |
 * +-------------------------------------------------------------+
 *
 * Sandybridge GT1 has 32kB of URB space, while GT2 has 64kB.
 * (See the Sandybridge PRM, Volume 2, Part 1, Section 1.4.7: 3DSTATE_URB.)
 */
static void
gen6_upload_urb( struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;
   int nr_vs_entries, nr_gs_entries;
   int total_urb_size = brw->urb.size * 1024; /* in bytes */

   /* CACHE_NEW_VS_PROG */
   brw->urb.vs_size = MAX2(brw->vs.prog_data->urb_entry_size, 1);

   /* We use the same VUE layout for VS outputs and GS outputs (as it's what
    * the SF and Clipper expect), so we can simply make the GS URB entry size
    * the same as for the VS.  This may technically be too large in cases
    * where we have few vertex attributes and a lot of varyings, since the VS
    * size is determined by the larger of the two.  For now, it's safe.
    */
   brw->urb.gs_size = brw->urb.vs_size;

   /* Calculate how many entries fit in each stage's section of the URB */
   if (brw->gs.prog_active) {
      nr_vs_entries = (total_urb_size/2) / (brw->urb.vs_size * 128);
      nr_gs_entries = (total_urb_size/2) / (brw->urb.gs_size * 128);
   } else {
      nr_vs_entries = total_urb_size / (brw->urb.vs_size * 128);
      nr_gs_entries = 0;
   }

   /* Then clamp to the maximum allowed by the hardware */
   if (nr_vs_entries > brw->urb.max_vs_entries)
      nr_vs_entries = brw->urb.max_vs_entries;

   if (nr_gs_entries > brw->urb.max_gs_entries)
      nr_gs_entries = brw->urb.max_gs_entries;

   /* Finally, both must be a multiple of 4 (see 3DSTATE_URB in the PRM). */
   brw->urb.nr_vs_entries = ROUND_DOWN_TO(nr_vs_entries, 4);
   brw->urb.nr_gs_entries = ROUND_DOWN_TO(nr_gs_entries, 4);

   assert(brw->urb.nr_vs_entries >= 24);
   assert(brw->urb.nr_vs_entries % 4 == 0);
   assert(brw->urb.nr_gs_entries % 4 == 0);
   assert(brw->urb.vs_size < 5);
   assert(brw->urb.gs_size < 5);

   BEGIN_BATCH(3);
   OUT_BATCH(_3DSTATE_URB << 16 | (3 - 2));
   OUT_BATCH(((brw->urb.vs_size - 1) << GEN6_URB_VS_SIZE_SHIFT) |
	     ((brw->urb.nr_vs_entries) << GEN6_URB_VS_ENTRIES_SHIFT));
   OUT_BATCH(((brw->urb.gs_size - 1) << GEN6_URB_GS_SIZE_SHIFT) |
	     ((brw->urb.nr_gs_entries) << GEN6_URB_GS_ENTRIES_SHIFT));
   ADVANCE_BATCH();

   /* From the PRM Volume 2 part 1, section 1.4.7:
    *
    *   Because of a urb corruption caused by allocating a previous gsunit’s
    *   urb entry to vsunit software is required to send a "GS NULL
    *   Fence"(Send URB fence with VS URB size == 1 and GS URB size == 0) plus
    *   a dummy DRAW call before any case where VS will be taking over GS URB
    *   space.
    *
    * It is not clear exactly what this means ("URB fence" is a command that
    * doesn't exist on Gen6).  So for now we just do a full pipeline flush as
    * a workaround.
    */
   if (brw->urb.gen6_gs_previously_active && !brw->gs.prog_active)
      intel_batchbuffer_emit_mi_flush(intel);
   brw->urb.gen6_gs_previously_active = brw->gs.prog_active;
}

const struct brw_tracked_state gen6_urb = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_CONTEXT,
      .cache = (CACHE_NEW_VS_PROG | CACHE_NEW_GS_PROG),
   },
   .emit = gen6_upload_urb,
};
