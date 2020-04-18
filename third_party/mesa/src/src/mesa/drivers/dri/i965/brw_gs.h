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
 

#ifndef BRW_GS_H
#define BRW_GS_H


#include "brw_context.h"
#include "brw_eu.h"

#define MAX_GS_VERTS (4)	     

struct brw_gs_prog_key {
   GLbitfield64 attrs;

   /**
    * Hardware primitive type being drawn, e.g. _3DPRIM_TRILIST.
    */
   GLuint primitive:8;

   GLuint pv_first:1;
   GLuint need_gs_prog:1;
   GLuint userclip_active:1;
   GLuint rasterizer_discard:1;

   /**
    * Number of varyings that are output to transform feedback.
    */
   GLuint num_transform_feedback_bindings:7; /* 0-BRW_MAX_SOL_BINDINGS */

   /**
    * Map from the index of a transform feedback binding table entry to the
    * gl_vert_result that should be streamed out through that binding table
    * entry.
    */
   unsigned char transform_feedback_bindings[BRW_MAX_SOL_BINDINGS];

   /**
    * Map from the index of a transform feedback binding table entry to the
    * swizzles that should be used when streaming out data through that
    * binding table entry.
    */
   unsigned char transform_feedback_swizzles[BRW_MAX_SOL_BINDINGS];
};

struct brw_gs_compile {
   struct brw_compile func;
   struct brw_gs_prog_key key;
   struct brw_gs_prog_data prog_data;
   
   struct {
      struct brw_reg R0;

      /**
       * Register holding streamed vertex buffer pointers -- see the Sandy
       * Bridge PRM, volume 2 part 1, section 4.4.2 (GS Thread Payload
       * [DevSNB]).  These pointers are delivered in GRF 1.
       */
      struct brw_reg SVBI;

      struct brw_reg vertex[MAX_GS_VERTS];
      struct brw_reg header;
      struct brw_reg temp;

      /**
       * Register holding destination indices for streamed buffer writes.
       * Only used for SOL programs.
       */
      struct brw_reg destination_indices;
   } reg;

   /* Number of registers used to store vertex data */
   GLuint nr_regs;

   struct brw_vue_map vue_map;
};

#define ATTR_SIZE  (4*4)

void brw_gs_quads( struct brw_gs_compile *c, struct brw_gs_prog_key *key );
void brw_gs_quad_strip( struct brw_gs_compile *c, struct brw_gs_prog_key *key );
void brw_gs_lines( struct brw_gs_compile *c );
void gen6_sol_program(struct brw_gs_compile *c, struct brw_gs_prog_key *key,
                      unsigned num_verts, bool check_edge_flag);

#endif
