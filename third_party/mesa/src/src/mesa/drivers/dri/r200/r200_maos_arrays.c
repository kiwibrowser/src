/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

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

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/colormac.h"
#include "main/imports.h"
#include "main/macros.h"

#include "swrast_setup/swrast_setup.h"
#include "math/m_translate.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"

#include "r200_context.h"
#include "r200_ioctl.h"
#include "r200_state.h"
#include "r200_swtcl.h"
#include "r200_maos.h"
#include "r200_tcl.h"

#if defined(USE_X86_ASM)
#define COPY_DWORDS( dst, src, nr )					\
do {									\
	int __tmp;							\
	__asm__ __volatile__( "rep ; movsl"				\
			      : "=%c" (__tmp), "=D" (dst), "=S" (__tmp)	\
			      : "0" (nr),				\
			        "D" ((long)dst),			\
			        "S" ((long)src) );			\
} while (0)
#else
#define COPY_DWORDS( dst, src, nr )		\
do {						\
   int j;					\
   for ( j = 0 ; j < nr ; j++ )			\
      dst[j] = ((int *)src)[j];			\
   dst += nr;					\
} while (0)
#endif

/* Emit any changed arrays to new GART memory, re-emit a packet to
 * update the arrays.  
 */
void r200EmitArrays( struct gl_context *ctx, GLubyte *vimap_rev )
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );
   struct vertex_buffer *VB = &TNL_CONTEXT( ctx )->vb;
   GLuint nr = 0;
   GLuint vfmt0 = 0, vfmt1 = 0;
   GLuint count = VB->Count;
   GLuint i, emitsize;

   //   fprintf(stderr,"emit arrays\n");
   for ( i = 0; i < 15; i++ ) {
      GLubyte attrib = vimap_rev[i];
      if (attrib != 255) {
	 switch (i) {
	 case 0:
	    emitsize = (VB->AttribPtr[attrib]->size);
	    switch (emitsize) {
	    case 4:
	       vfmt0 |= R200_VTX_W0;
	       /* fallthrough */
	    case 3:
	       vfmt0 |= R200_VTX_Z0;
	       break;
	    case 2:
	       break;
	    default: assert(0);
	    }
	    break;
	 case 1:
	    assert(attrib == VERT_ATTRIB_WEIGHT);
	    emitsize = (VB->AttribPtr[attrib]->size);
	    vfmt0 |= emitsize << R200_VTX_WEIGHT_COUNT_SHIFT;
	    break;
	 case 2:
	    assert(attrib == VERT_ATTRIB_NORMAL);
	    emitsize = 3;
	    vfmt0 |= R200_VTX_N0;
	    break;
	 case 3:
	    /* special handling to fix up fog. Will get us into trouble with vbos...*/
	    assert(attrib == VERT_ATTRIB_FOG);
	    if (!rmesa->radeon.tcl.aos[i].bo) {
	       if (ctx->VertexProgram._Enabled)
		  rcommon_emit_vector( ctx,
				       &(rmesa->radeon.tcl.aos[nr]),
				       (char *)VB->AttribPtr[attrib]->data,
				       1,
				       VB->AttribPtr[attrib]->stride,
				       count);
	       else
		 rcommon_emit_vecfog( ctx,
				      &(rmesa->radeon.tcl.aos[nr]),
				      (char *)VB->AttribPtr[attrib]->data,
				      VB->AttribPtr[attrib]->stride,
				      count);
	    }
	    vfmt0 |= R200_VTX_DISCRETE_FOG;
	    goto after_emit;
	    break;
	 case 4:
	 case 5:
	 case 6:
	 case 7:
	    if (VB->AttribPtr[attrib]->size == 4 &&
	       (VB->AttribPtr[attrib]->stride != 0 ||
		VB->AttribPtr[attrib]->data[0][3] != 1.0)) emitsize = 4;
	    else emitsize = 3;
	    if (emitsize == 4)
	       vfmt0 |= R200_VTX_FP_RGBA << (R200_VTX_COLOR_0_SHIFT + (i - 4) * 2);
	    else {
	       vfmt0 |= R200_VTX_FP_RGB << (R200_VTX_COLOR_0_SHIFT + (i - 4) * 2);
	    }
	    break;
	 case 8:
	 case 9:
	 case 10:
	 case 11:
	 case 12:
	 case 13:
	    emitsize = VB->AttribPtr[attrib]->size;
	    vfmt1 |= emitsize << (R200_VTX_TEX0_COMP_CNT_SHIFT + (i - 8) * 3);
	    break;
	 case 14:
	    emitsize = VB->AttribPtr[attrib]->size >= 2 ? VB->AttribPtr[attrib]->size : 2;
	    switch (emitsize) {
	    case 2:
	       vfmt0 |= R200_VTX_XY1;
	       /* fallthrough */
	    case 3:
	       vfmt0 |= R200_VTX_Z1;
	       /* fallthrough */
	    case 4:
	       vfmt0 |= R200_VTX_W1;
	       /* fallthrough */
	    }
	    break;
	 default:
	    assert(0);
	    emitsize = 0;
	 }
	 if (!rmesa->radeon.tcl.aos[nr].bo) {
	   rcommon_emit_vector( ctx,
				&(rmesa->radeon.tcl.aos[nr]),
				(char *)VB->AttribPtr[attrib]->data,
				emitsize,
				VB->AttribPtr[attrib]->stride,
				count );
	 }
after_emit:
	 assert(nr < 12);
	 nr++;
      }
   }

   if (vfmt0 != rmesa->hw.vtx.cmd[VTX_VTXFMT_0] ||
       vfmt1 != rmesa->hw.vtx.cmd[VTX_VTXFMT_1]) {
      R200_STATECHANGE( rmesa, vtx );
      rmesa->hw.vtx.cmd[VTX_VTXFMT_0] = vfmt0;
      rmesa->hw.vtx.cmd[VTX_VTXFMT_1] = vfmt1;
   }

   rmesa->radeon.tcl.aos_count = nr;
}

