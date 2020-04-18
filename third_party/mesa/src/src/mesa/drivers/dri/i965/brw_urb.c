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
#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

#define VS 0
#define GS 1
#define CLP 2
#define SF 3
#define CS 4

/** @file brw_urb.c
 *
 * Manages the division of the URB space between the various fixed-function
 * units.
 *
 * See the Thread Initiation Management section of the GEN4 B-Spec, and
 * the individual *_STATE structures for restrictions on numbers of
 * entries and threads.
 */

/*
 * Generally, a unit requires a min_nr_entries based on how many entries
 * it produces before the downstream unit gets unblocked and can use and
 * dereference some of its handles.
 *
 * The SF unit preallocates a PUE at the start of thread dispatch, and only
 * uses that one.  So it requires one entry per thread.
 *
 * For CLIP, the SF unit will hold the previous primitive while the
 * next is getting assembled, meaning that linestrips require 3 CLIP VUEs
 * (vertices) to ensure continued processing, trifans require 4, and tristrips
 * require 5.  There can be 1 or 2 threads, and each has the same requirement.
 *
 * GS has the same requirement as CLIP, but it never handles tristrips,
 * so we can lower the minimum to 4 for the POLYGONs (trifans) it produces.
 * We only run it single-threaded.
 *
 * For VS, the number of entries may be 8, 12, 16, or 32 (or 64 on G4X).
 * Each thread processes 2 preallocated VUEs (vertices) at a time, and they
 * get streamed down as soon as threads processing earlier vertices get
 * theirs accepted.
 *
 * Each unit will take the number of URB entries we give it (based on the
 * entry size calculated in brw_vs_emit.c for VUEs, brw_sf_emit.c for PUEs,
 * and brw_curbe.c for the CURBEs) and decide its maximum number of
 * threads it can support based on that. in brw_*_state.c.
 *
 * XXX: Are the min_entry_size numbers useful?
 * XXX: Verify min_nr_entries, esp for VS.
 * XXX: Verify SF min_entry_size.
 */
static const struct {
   GLuint min_nr_entries;
   GLuint preferred_nr_entries;
   GLuint min_entry_size;
   GLuint max_entry_size;
} limits[CS+1] = {
   { 16, 32, 1, 5 },			/* vs */
   { 4, 8,  1, 5 },			/* gs */
   { 5, 10,  1, 5 },			/* clp */
   { 1, 8,  1, 12 },		        /* sf */
   { 1, 4,  1, 32 }			/* cs */
};


static bool check_urb_layout(struct brw_context *brw)
{
   brw->urb.vs_start = 0;
   brw->urb.gs_start = brw->urb.nr_vs_entries * brw->urb.vsize;
   brw->urb.clip_start = brw->urb.gs_start + brw->urb.nr_gs_entries * brw->urb.vsize;
   brw->urb.sf_start = brw->urb.clip_start + brw->urb.nr_clip_entries * brw->urb.vsize;
   brw->urb.cs_start = brw->urb.sf_start + brw->urb.nr_sf_entries * brw->urb.sfsize;

   return brw->urb.cs_start + brw->urb.nr_cs_entries *
      brw->urb.csize <= brw->urb.size;
}

/* Most minimal update, forces re-emit of URB fence packet after GS
 * unit turned on/off.
 */
static void recalculate_urb_fence( struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;
   GLuint csize = brw->curbe.total_size;
   GLuint vsize = brw->vs.prog_data->urb_entry_size;
   GLuint sfsize = brw->sf.prog_data->urb_entry_size;

   if (csize < limits[CS].min_entry_size)
      csize = limits[CS].min_entry_size;

   if (vsize < limits[VS].min_entry_size)
      vsize = limits[VS].min_entry_size;

   if (sfsize < limits[SF].min_entry_size)
      sfsize = limits[SF].min_entry_size;

   if (brw->urb.vsize < vsize ||
       brw->urb.sfsize < sfsize ||
       brw->urb.csize < csize ||
       (brw->urb.constrained && (brw->urb.vsize > vsize ||
				 brw->urb.sfsize > sfsize ||
				 brw->urb.csize > csize))) {
      

      brw->urb.csize = csize;
      brw->urb.sfsize = sfsize;
      brw->urb.vsize = vsize;

      brw->urb.nr_vs_entries = limits[VS].preferred_nr_entries;	
      brw->urb.nr_gs_entries = limits[GS].preferred_nr_entries;	
      brw->urb.nr_clip_entries = limits[CLP].preferred_nr_entries;
      brw->urb.nr_sf_entries = limits[SF].preferred_nr_entries;	
      brw->urb.nr_cs_entries = limits[CS].preferred_nr_entries;	

      brw->urb.constrained = 0;

      if (intel->gen == 5) {
         brw->urb.nr_vs_entries = 128;
         brw->urb.nr_sf_entries = 48;
         if (check_urb_layout(brw)) {
            goto done;
         } else {
            brw->urb.constrained = 1;
            brw->urb.nr_vs_entries = limits[VS].preferred_nr_entries;
            brw->urb.nr_sf_entries = limits[SF].preferred_nr_entries;
         }
      } else if (intel->is_g4x) {
	 brw->urb.nr_vs_entries = 64;
	 if (check_urb_layout(brw)) {
	    goto done;
	 } else {
	    brw->urb.constrained = 1;
	    brw->urb.nr_vs_entries = limits[VS].preferred_nr_entries;
	 }
      }

      if (!check_urb_layout(brw)) {
	 brw->urb.nr_vs_entries = limits[VS].min_nr_entries;	
	 brw->urb.nr_gs_entries = limits[GS].min_nr_entries;	
	 brw->urb.nr_clip_entries = limits[CLP].min_nr_entries;
	 brw->urb.nr_sf_entries = limits[SF].min_nr_entries;	
	 brw->urb.nr_cs_entries = limits[CS].min_nr_entries;	

	 /* Mark us as operating with constrained nr_entries, so that next
	  * time we recalculate we'll resize the fences in the hope of
	  * escaping constrained mode and getting back to normal performance.
	  */
	 brw->urb.constrained = 1;
	 
	 if (!check_urb_layout(brw)) {
	    /* This is impossible, given the maximal sizes of urb
	     * entries and the values for minimum nr of entries
	     * provided above.
	     */
	    printf("couldn't calculate URB layout!\n");
	    exit(1);
	 }
	 
	 if (unlikely(INTEL_DEBUG & (DEBUG_URB|DEBUG_PERF)))
	    printf("URB CONSTRAINED\n");
      }

done:
      if (unlikely(INTEL_DEBUG & DEBUG_URB))
	 printf("URB fence: %d ..VS.. %d ..GS.. %d ..CLP.. %d ..SF.. %d ..CS.. %d\n",
		      brw->urb.vs_start,
		      brw->urb.gs_start,
		      brw->urb.clip_start,
		      brw->urb.sf_start,
		      brw->urb.cs_start, 
		      brw->urb.size);
      
      brw->state.dirty.brw |= BRW_NEW_URB_FENCE;
   }
}


const struct brw_tracked_state brw_recalculate_urb_fence = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_CURBE_OFFSETS,
      .cache = (CACHE_NEW_VS_PROG |
		CACHE_NEW_SF_PROG)
   },
   .emit = recalculate_urb_fence
};





void brw_upload_urb_fence(struct brw_context *brw)
{
   struct brw_urb_fence uf;
   memset(&uf, 0, sizeof(uf));

   uf.header.opcode = CMD_URB_FENCE;
   uf.header.length = sizeof(uf)/4-2;
   uf.header.vs_realloc = 1;
   uf.header.gs_realloc = 1;
   uf.header.clp_realloc = 1;
   uf.header.sf_realloc = 1;
   uf.header.vfe_realloc = 1;
   uf.header.cs_realloc = 1;

   /* The ordering below is correct, not the layout in the
    * instruction.
    *
    * There are 256/384 urb reg pairs in total.
    */
   uf.bits0.vs_fence  = brw->urb.gs_start;
   uf.bits0.gs_fence  = brw->urb.clip_start; 
   uf.bits0.clp_fence = brw->urb.sf_start; 
   uf.bits1.sf_fence  = brw->urb.cs_start; 
   uf.bits1.cs_fence  = brw->urb.size;

   /* erratum: URB_FENCE must not cross a 64byte cacheline */
   if ((brw->intel.batch.used & 15) > 12) {
      int pad = 16 - (brw->intel.batch.used & 15);
      do
	 brw->intel.batch.map[brw->intel.batch.used++] = MI_NOOP;
      while (--pad);
   }

   BRW_BATCH_STRUCT(brw, &uf);
}
