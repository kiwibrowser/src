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



#include "main/glheader.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/enums.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"
#include "program/prog_statevars.h"
#include "intel_batchbuffer.h"
#include "intel_regions.h"
#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "brw_util.h"


/**
 * Partition the CURBE between the various users of constant values:
 * Note that vertex and fragment shaders can now fetch constants out
 * of constant buffers.  We no longer allocatea block of the GRF for
 * constants.  That greatly reduces the demand for space in the CURBE.
 * Some of the comments within are dated...
 */
static void calculate_curbe_offsets( struct brw_context *brw )
{
   struct gl_context *ctx = &brw->intel.ctx;
   /* CACHE_NEW_WM_PROG */
   const GLuint nr_fp_regs = (brw->wm.prog_data->nr_params + 15) / 16;
   
   /* BRW_NEW_VERTEX_PROGRAM */
   const GLuint nr_vp_regs = (brw->vs.prog_data->nr_params + 15) / 16;
   GLuint nr_clip_regs = 0;
   GLuint total_regs;

   /* _NEW_TRANSFORM */
   if (ctx->Transform.ClipPlanesEnabled) {
      GLuint nr_planes = 6 + _mesa_bitcount_64(ctx->Transform.ClipPlanesEnabled);
      nr_clip_regs = (nr_planes * 4 + 15) / 16;
   }


   total_regs = nr_fp_regs + nr_vp_regs + nr_clip_regs;

   /* This can happen - what to do?  Probably rather than falling
    * back, the best thing to do is emit programs which code the
    * constants as immediate values.  Could do this either as a static
    * cap on WM and VS, or adaptively.
    *
    * Unfortunately, this is currently dependent on the results of the
    * program generation process (in the case of wm), so this would
    * introduce the need to re-generate programs in the event of a
    * curbe allocation failure.
    */
   /* Max size is 32 - just large enough to
    * hold the 128 parameters allowed by
    * the fragment and vertex program
    * api's.  It's not clear what happens
    * when both VP and FP want to use 128
    * parameters, though. 
    */
   assert(total_regs <= 32);

   /* Lazy resize:
    */
   if (nr_fp_regs > brw->curbe.wm_size ||
       nr_vp_regs > brw->curbe.vs_size ||
       nr_clip_regs != brw->curbe.clip_size ||
       (total_regs < brw->curbe.total_size / 4 &&
	brw->curbe.total_size > 16)) {

      GLuint reg = 0;

      /* Calculate a new layout: 
       */
      reg = 0;
      brw->curbe.wm_start = reg;
      brw->curbe.wm_size = nr_fp_regs; reg += nr_fp_regs;
      brw->curbe.clip_start = reg;
      brw->curbe.clip_size = nr_clip_regs; reg += nr_clip_regs;
      brw->curbe.vs_start = reg;
      brw->curbe.vs_size = nr_vp_regs; reg += nr_vp_regs;
      brw->curbe.total_size = reg;

      if (0)
	 printf("curbe wm %d+%d clip %d+%d vs %d+%d\n",
		brw->curbe.wm_start,
		brw->curbe.wm_size,
		brw->curbe.clip_start,
		brw->curbe.clip_size,
		brw->curbe.vs_start,
		brw->curbe.vs_size );

      brw->state.dirty.brw |= BRW_NEW_CURBE_OFFSETS;
   }
}


const struct brw_tracked_state brw_curbe_offsets = {
   .dirty = {
      .mesa = _NEW_TRANSFORM,
      .brw  = BRW_NEW_VERTEX_PROGRAM | BRW_NEW_CONTEXT,
      .cache = CACHE_NEW_WM_PROG
   },
   .emit = calculate_curbe_offsets
};




/* Define the number of curbes within CS's urb allocation.  Multiple
 * urb entries -> multiple curbes.  These will be used by
 * fixed-function hardware in a double-buffering scheme to avoid a
 * pipeline stall each time the contents of the curbe is changed.
 */
void brw_upload_cs_urb_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(2);
   /* It appears that this is the state packet for the CS unit, ie. the
    * urb entries detailed here are housed in the CS range from the
    * URB_FENCE command.
    */
   OUT_BATCH(CMD_CS_URB_STATE << 16 | (2-2));

   /* BRW_NEW_URB_FENCE */
   if (brw->urb.csize == 0) {
      OUT_BATCH(0);
   } else {
      /* BRW_NEW_URB_FENCE */
      assert(brw->urb.nr_cs_entries);
      OUT_BATCH((brw->urb.csize - 1) << 4 | brw->urb.nr_cs_entries);
   }
   CACHED_BATCH();
}

static GLfloat fixed_plane[6][4] = {
   { 0,    0,   -1, 1 },
   { 0,    0,    1, 1 },
   { 0,   -1,    0, 1 },
   { 0,    1,    0, 1 },
   {-1,    0,    0, 1 },
   { 1,    0,    0, 1 }
};

/* Upload a new set of constants.  Too much variability to go into the
 * cache mechanism, but maybe would benefit from a comparison against
 * the current uploaded set of constants.
 */
static void
brw_upload_constant_buffer(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   const struct brw_vertex_program *vp =
      brw_vertex_program_const(brw->vertex_program);
   const GLuint sz = brw->curbe.total_size;
   const GLuint bufsz = sz * 16 * sizeof(GLfloat);
   GLfloat *buf;
   GLuint i;
   gl_clip_plane *clip_planes;

   if (sz == 0) {
      brw->curbe.last_bufsz  = 0;
      goto emit;
   }

   buf = brw->curbe.next_buf;

   /* fragment shader constants */
   if (brw->curbe.wm_size) {
      GLuint offset = brw->curbe.wm_start * 16;

      /* copy float constants */
      for (i = 0; i < brw->wm.prog_data->nr_params; i++) {
	 buf[offset + i] = *brw->wm.prog_data->param[i];
      }
   }


   /* When using the old VS backend, the clipplanes are actually delivered to
    * both CLIP and VS units.  VS uses them to calculate the outcode bitmasks.
    *
    * When using the new VS backend, it is responsible for setting up its own
    * clipplane constants if it needs them.  This results in a slight waste of
    * of curbe space, but the advantage is that the new VS backend can use its
    * general-purpose uniform layout code to store the clipplanes.
    */
   if (brw->curbe.clip_size) {
      GLuint offset = brw->curbe.clip_start * 16;
      GLuint j;

      /* If any planes are going this way, send them all this way:
       */
      for (i = 0; i < 6; i++) {
	 buf[offset + i * 4 + 0] = fixed_plane[i][0];
	 buf[offset + i * 4 + 1] = fixed_plane[i][1];
	 buf[offset + i * 4 + 2] = fixed_plane[i][2];
	 buf[offset + i * 4 + 3] = fixed_plane[i][3];
      }

      /* Clip planes: _NEW_TRANSFORM plus _NEW_PROJECTION to get to
       * clip-space:
       */
      clip_planes = brw_select_clip_planes(ctx);
      for (j = 0; j < MAX_CLIP_PLANES; j++) {
	 if (ctx->Transform.ClipPlanesEnabled & (1<<j)) {
	    buf[offset + i * 4 + 0] = clip_planes[j][0];
	    buf[offset + i * 4 + 1] = clip_planes[j][1];
	    buf[offset + i * 4 + 2] = clip_planes[j][2];
	    buf[offset + i * 4 + 3] = clip_planes[j][3];
	    i++;
	 }
      }
   }

   /* vertex shader constants */
   if (brw->curbe.vs_size) {
      GLuint offset = brw->curbe.vs_start * 16;
      GLuint nr = brw->vs.prog_data->nr_params / 4;

      if (brw->vs.prog_data->uses_new_param_layout) {
	 for (i = 0; i < brw->vs.prog_data->nr_params; i++) {
	    buf[offset + i] = *brw->vs.prog_data->param[i];
	 }
      } else {
	 /* Load the subset of push constants that will get used when
	  * we also have a pull constant buffer.
	  */
	 for (i = 0; i < vp->program.Base.Parameters->NumParameters; i++) {
	    if (brw->vs.constant_map[i] != -1) {
	       assert(brw->vs.constant_map[i] <= nr);
	       memcpy(buf + offset + brw->vs.constant_map[i] * 4,
		      vp->program.Base.Parameters->ParameterValues[i],
		      4 * sizeof(float));
	    }
	 }
      }
   }

   if (0) {
      for (i = 0; i < sz*16; i+=4) 
	 printf("curbe %d.%d: %f %f %f %f\n", i/8, i&4,
		buf[i+0], buf[i+1], buf[i+2], buf[i+3]);

      printf("last_buf %p buf %p sz %d/%d cmp %d\n",
	     brw->curbe.last_buf, buf,
	     bufsz, brw->curbe.last_bufsz,
	     brw->curbe.last_buf ? memcmp(buf, brw->curbe.last_buf, bufsz) : -1);
   }

   if (brw->curbe.curbe_bo != NULL &&
       bufsz == brw->curbe.last_bufsz &&
       memcmp(buf, brw->curbe.last_buf, bufsz) == 0) {
      /* constants have not changed */
   } else {
      /* Update the record of what our last set of constants was.  We
       * don't just flip the pointers because we don't fill in the
       * data in the padding between the entries.
       */
      memcpy(brw->curbe.last_buf, buf, bufsz);
      brw->curbe.last_bufsz = bufsz;

      if (brw->curbe.curbe_bo != NULL &&
	  brw->curbe.curbe_next_offset + bufsz > brw->curbe.curbe_bo->size)
      {
	 drm_intel_gem_bo_unmap_gtt(brw->curbe.curbe_bo);
	 drm_intel_bo_unreference(brw->curbe.curbe_bo);
	 brw->curbe.curbe_bo = NULL;
      }

      if (brw->curbe.curbe_bo == NULL) {
	 /* Allocate a single page for CURBE entries for this batchbuffer.
	  * They're generally around 64b.
	  */
	 brw->curbe.curbe_bo = drm_intel_bo_alloc(brw->intel.bufmgr, "CURBE",
						  4096, 1 << 6);
	 brw->curbe.curbe_next_offset = 0;
	 drm_intel_gem_bo_map_gtt(brw->curbe.curbe_bo);
	 assert(bufsz < 4096);
      }

      brw->curbe.curbe_offset = brw->curbe.curbe_next_offset;
      brw->curbe.curbe_next_offset += bufsz;
      brw->curbe.curbe_next_offset = ALIGN(brw->curbe.curbe_next_offset, 64);

      /* Copy data to the buffer:
       */
      memcpy(brw->curbe.curbe_bo->virtual + brw->curbe.curbe_offset,
	     buf,
	     bufsz);
   }

   /* Because this provokes an action (ie copy the constants into the
    * URB), it shouldn't be shortcircuited if identical to the
    * previous time - because eg. the urb destination may have
    * changed, or the urb contents different to last time.
    *
    * Note that the data referred to is actually copied internally,
    * not just used in place according to passed pointer.
    *
    * It appears that the CS unit takes care of using each available
    * URB entry (Const URB Entry == CURBE) in turn, and issuing
    * flushes as necessary when doublebuffering of CURBEs isn't
    * possible.
    */

emit:
   BEGIN_BATCH(2);
   if (brw->curbe.total_size == 0) {
      OUT_BATCH((CMD_CONST_BUFFER << 16) | (2 - 2));
      OUT_BATCH(0);
   } else {
      OUT_BATCH((CMD_CONST_BUFFER << 16) | (1 << 8) | (2 - 2));
      OUT_RELOC(brw->curbe.curbe_bo,
		I915_GEM_DOMAIN_INSTRUCTION, 0,
		(brw->curbe.total_size - 1) + brw->curbe.curbe_offset);
   }
   ADVANCE_BATCH();
}

/* This tracked state is unique in that the state it monitors varies
 * dynamically depending on the parameters tracked by the fragment and
 * vertex programs.  This is the template used as a starting point,
 * each context will maintain a copy of this internally and update as
 * required.
 */
const struct brw_tracked_state brw_constant_buffer = {
   .dirty = {
      .mesa = _NEW_PROGRAM_CONSTANTS,
      .brw  = (BRW_NEW_FRAGMENT_PROGRAM |
	       BRW_NEW_VERTEX_PROGRAM |
	       BRW_NEW_URB_FENCE | /* Implicit - hardware requires this, not used above */
	       BRW_NEW_PSP | /* Implicit - hardware requires this, not used above */
	       BRW_NEW_CURBE_OFFSETS |
	       BRW_NEW_BATCH),
      .cache = (CACHE_NEW_WM_PROG) 
   },
   .emit = brw_upload_constant_buffer,
};

