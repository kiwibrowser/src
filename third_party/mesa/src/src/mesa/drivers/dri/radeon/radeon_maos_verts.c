/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     Tungsten Graphics Inc., Austin, Texas.

All Rights Reserved.

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
#include "main/imports.h"
#include "main/mtypes.h"

#include "vbo/vbo.h"
#include "math/m_translate.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "radeon_context.h"
#include "radeon_state.h"
#include "radeon_ioctl.h"
#include "radeon_tex.h"
#include "radeon_tcl.h"
#include "radeon_swtcl.h"
#include "radeon_maos.h"
#include "radeon_fog.h"

#define RADEON_TCL_MAX_SETUP 19

union emit_union { float f; GLuint ui; radeon_color_t rgba; };

static struct {
   void   (*emit)( struct gl_context *, GLuint, GLuint, void * );
   GLuint vertex_size;
   GLuint vertex_format;
} setup_tab[RADEON_TCL_MAX_SETUP];

#define DO_W    (IND & RADEON_CP_VC_FRMT_W0)
#define DO_RGBA (IND & RADEON_CP_VC_FRMT_PKCOLOR)
#define DO_SPEC_OR_FOG (IND & RADEON_CP_VC_FRMT_PKSPEC)
#define DO_SPEC ((IND & RADEON_CP_VC_FRMT_PKSPEC) && \
		 (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR))
#define DO_FOG  ((IND & RADEON_CP_VC_FRMT_PKSPEC) && ctx->Fog.Enabled && \
		 (ctx->Fog.FogCoordinateSource == GL_FOG_COORD))
#define DO_TEX0 (IND & RADEON_CP_VC_FRMT_ST0)
#define DO_TEX1 (IND & RADEON_CP_VC_FRMT_ST1)
#define DO_TEX2 (IND & RADEON_CP_VC_FRMT_ST2)
#define DO_PTEX (IND & RADEON_CP_VC_FRMT_Q0)
#define DO_NORM (IND & RADEON_CP_VC_FRMT_N0)

#define DO_TEX3 0

#define GET_TEXSOURCE(n)  n

/***********************************************************************
 *             Generate vertex emit functions               *
 ***********************************************************************/


/* Defined in order of increasing vertex size:
 */
#define IDX 0
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_PKCOLOR)
#define TAG(x) x##_rgba
#include "radeon_maos_vbtmp.h"

#define IDX 1
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_N0)
#define TAG(x) x##_n
#include "radeon_maos_vbtmp.h"

#define IDX 2
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_PKCOLOR|		\
	     RADEON_CP_VC_FRMT_ST0)
#define TAG(x) x##_rgba_st
#include "radeon_maos_vbtmp.h"

#define IDX 3
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_PKCOLOR|		\
	     RADEON_CP_VC_FRMT_N0)
#define TAG(x) x##_rgba_n
#include "radeon_maos_vbtmp.h"

#define IDX 4
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_ST0|		\
	     RADEON_CP_VC_FRMT_N0)
#define TAG(x) x##_st_n
#include "radeon_maos_vbtmp.h"

#define IDX 5
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_PKCOLOR|		\
	     RADEON_CP_VC_FRMT_ST0|		\
	     RADEON_CP_VC_FRMT_ST1)
#define TAG(x) x##_rgba_st_st
#include "radeon_maos_vbtmp.h"

#define IDX 6
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_PKCOLOR|		\
	     RADEON_CP_VC_FRMT_ST0|		\
	     RADEON_CP_VC_FRMT_N0)
#define TAG(x) x##_rgba_st_n
#include "radeon_maos_vbtmp.h"

#define IDX 7
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_PKCOLOR|		\
	     RADEON_CP_VC_FRMT_PKSPEC|		\
	     RADEON_CP_VC_FRMT_ST0|		\
	     RADEON_CP_VC_FRMT_ST1)
#define TAG(x) x##_rgba_spec_st_st
#include "radeon_maos_vbtmp.h"

#define IDX 8
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_ST0|		\
	     RADEON_CP_VC_FRMT_ST1|		\
	     RADEON_CP_VC_FRMT_N0)
#define TAG(x) x##_st_st_n
#include "radeon_maos_vbtmp.h"

#define IDX 9
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_PKCOLOR|		\
	     RADEON_CP_VC_FRMT_PKSPEC|		\
	     RADEON_CP_VC_FRMT_ST0|		\
	     RADEON_CP_VC_FRMT_ST1|		\
	     RADEON_CP_VC_FRMT_N0)
#define TAG(x) x##_rgba_spec_st_st_n
#include "radeon_maos_vbtmp.h"

#define IDX 10
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_PKCOLOR|		\
	     RADEON_CP_VC_FRMT_ST0|		\
	     RADEON_CP_VC_FRMT_Q0)
#define TAG(x) x##_rgba_stq
#include "radeon_maos_vbtmp.h"

#define IDX 11
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_PKCOLOR|		\
	     RADEON_CP_VC_FRMT_ST1|		\
	     RADEON_CP_VC_FRMT_Q1|		\
	     RADEON_CP_VC_FRMT_ST0|		\
	     RADEON_CP_VC_FRMT_Q0)
#define TAG(x) x##_rgba_stq_stq
#include "radeon_maos_vbtmp.h"

#define IDX 12
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_W0|		\
	     RADEON_CP_VC_FRMT_PKCOLOR|		\
	     RADEON_CP_VC_FRMT_PKSPEC|		\
	     RADEON_CP_VC_FRMT_ST0|		\
	     RADEON_CP_VC_FRMT_Q0|		\
	     RADEON_CP_VC_FRMT_ST1|		\
	     RADEON_CP_VC_FRMT_Q1|		\
	     RADEON_CP_VC_FRMT_N0)
#define TAG(x) x##_w_rgba_spec_stq_stq_n
#include "radeon_maos_vbtmp.h"

#define IDX 13
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_PKCOLOR|		\
	     RADEON_CP_VC_FRMT_ST0|		\
	     RADEON_CP_VC_FRMT_ST1|		\
	     RADEON_CP_VC_FRMT_ST2)
#define TAG(x) x##_rgba_st_st_st
#include "radeon_maos_vbtmp.h"

#define IDX 14
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_PKCOLOR|		\
	     RADEON_CP_VC_FRMT_PKSPEC|		\
	     RADEON_CP_VC_FRMT_ST0|		\
	     RADEON_CP_VC_FRMT_ST1|		\
	     RADEON_CP_VC_FRMT_ST2)
#define TAG(x) x##_rgba_spec_st_st_st
#include "radeon_maos_vbtmp.h"

#define IDX 15
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_ST0|		\
	     RADEON_CP_VC_FRMT_ST1|		\
	     RADEON_CP_VC_FRMT_ST2|		\
	     RADEON_CP_VC_FRMT_N0)
#define TAG(x) x##_st_st_st_n
#include "radeon_maos_vbtmp.h"

#define IDX 16
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_PKCOLOR|		\
	     RADEON_CP_VC_FRMT_PKSPEC|		\
	     RADEON_CP_VC_FRMT_ST0|		\
	     RADEON_CP_VC_FRMT_ST1|		\
	     RADEON_CP_VC_FRMT_ST2|		\
	     RADEON_CP_VC_FRMT_N0)
#define TAG(x) x##_rgba_spec_st_st_st_n
#include "radeon_maos_vbtmp.h"

#define IDX 17
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_PKCOLOR|		\
	     RADEON_CP_VC_FRMT_ST0|		\
	     RADEON_CP_VC_FRMT_Q0|		\
	     RADEON_CP_VC_FRMT_ST1|		\
	     RADEON_CP_VC_FRMT_Q1|		\
	     RADEON_CP_VC_FRMT_ST2|		\
	     RADEON_CP_VC_FRMT_Q2)
#define TAG(x) x##_rgba_stq_stq_stq
#include "radeon_maos_vbtmp.h"

#define IDX 18
#define IND (RADEON_CP_VC_FRMT_XY|		\
	     RADEON_CP_VC_FRMT_Z|		\
	     RADEON_CP_VC_FRMT_W0|		\
	     RADEON_CP_VC_FRMT_PKCOLOR|		\
	     RADEON_CP_VC_FRMT_PKSPEC|		\
	     RADEON_CP_VC_FRMT_ST0|		\
	     RADEON_CP_VC_FRMT_Q0|		\
	     RADEON_CP_VC_FRMT_ST1|		\
	     RADEON_CP_VC_FRMT_Q1|		\
	     RADEON_CP_VC_FRMT_ST2|		\
	     RADEON_CP_VC_FRMT_Q2|		\
	     RADEON_CP_VC_FRMT_N0)
#define TAG(x) x##_w_rgba_spec_stq_stq_stq_n
#include "radeon_maos_vbtmp.h"




/***********************************************************************
 *                         Initialization 
 ***********************************************************************/


static void init_tcl_verts( void )
{
   init_rgba();
   init_n();
   init_rgba_n();
   init_rgba_st();
   init_st_n();
   init_rgba_st_st();
   init_rgba_st_n();
   init_rgba_spec_st_st();
   init_st_st_n();
   init_rgba_spec_st_st_n();
   init_rgba_stq();
   init_rgba_stq_stq();
   init_w_rgba_spec_stq_stq_n();
   init_rgba_st_st_st();
   init_rgba_spec_st_st_st();
   init_st_st_st_n();
   init_rgba_spec_st_st_st_n();
   init_rgba_stq_stq_stq();
   init_w_rgba_spec_stq_stq_stq_n();
}


void radeonEmitArrays( struct gl_context *ctx, GLuint inputs )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLuint req = 0;
   GLuint unit;
   GLuint vtx = (rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] &
		 ~(RADEON_TCL_VTX_Q0|RADEON_TCL_VTX_Q1|RADEON_TCL_VTX_Q2));
   int i;
   static int firsttime = 1;

   if (firsttime) {
      init_tcl_verts();
      firsttime = 0;
   }

   if (1) {
      req |= RADEON_CP_VC_FRMT_Z;
      if (VB->AttribPtr[_TNL_ATTRIB_POS]->size == 4) {
	 req |= RADEON_CP_VC_FRMT_W0;
      }
   }

   if (inputs & VERT_BIT_NORMAL) {
      req |= RADEON_CP_VC_FRMT_N0;
   }

   if (inputs & VERT_BIT_COLOR0) {
      req |= RADEON_CP_VC_FRMT_PKCOLOR;
   }

   if (inputs & (VERT_BIT_COLOR1|VERT_BIT_FOG)) {
      req |= RADEON_CP_VC_FRMT_PKSPEC;
   }

   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      if (inputs & VERT_BIT_TEX(unit)) {
	 req |= RADEON_ST_BIT(unit);
	 /* assume we need the 3rd coord if texgen is active for r/q OR at least
	    3 coords are submitted. This may not be 100% correct */
	 if (VB->AttribPtr[_TNL_ATTRIB_TEX0 + unit]->size >= 3) {
	    req |= RADEON_Q_BIT(unit);
	    vtx |= RADEON_Q_BIT(unit);
	 }
	 if ( (ctx->Texture.Unit[unit].TexGenEnabled & (R_BIT | Q_BIT)) )
	    vtx |= RADEON_Q_BIT(unit);
	 else if ((VB->AttribPtr[_TNL_ATTRIB_TEX0 + unit]->size >= 3) &&
	          ((ctx->Texture.Unit[unit]._ReallyEnabled & (TEXTURE_CUBE_BIT)) == 0)) {
	    GLuint swaptexmatcol = (VB->AttribPtr[_TNL_ATTRIB_TEX0 + unit]->size - 3);
	    if (((rmesa->NeedTexMatrix >> unit) & 1) &&
		 (swaptexmatcol != ((rmesa->TexMatColSwap >> unit) & 1)))
	       radeonUploadTexMatrix( rmesa, unit, swaptexmatcol ) ;
	 }
      }
   }

   if (vtx != rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT]) {
      RADEON_STATECHANGE( rmesa, tcl );
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] = vtx;
   }

   for (i = 0 ; i < RADEON_TCL_MAX_SETUP ; i++) 
      if ((setup_tab[i].vertex_format & req) == req) 
	 break;

   if (rmesa->tcl.vertex_format == setup_tab[i].vertex_format &&
       rmesa->radeon.tcl.aos[0].bo)
      return;

   if (rmesa->radeon.tcl.aos[0].bo)
      radeonReleaseArrays( ctx, ~0 );

   radeonAllocDmaRegion( &rmesa->radeon,
			 &rmesa->radeon.tcl.aos[0].bo,
			 &rmesa->radeon.tcl.aos[0].offset,
			 VB->Count * setup_tab[i].vertex_size * 4, 
			 4);

   /* The vertex code expects Obj to be clean to element 3.  To fix
    * this, add more vertex code (for obj-2, obj-3) or preferably move
    * to maos.  
    */
   if (VB->AttribPtr[_TNL_ATTRIB_POS]->size < 3 ||
       (VB->AttribPtr[_TNL_ATTRIB_POS]->size == 3 &&
	(setup_tab[i].vertex_format & RADEON_CP_VC_FRMT_W0))) {

      _math_trans_4f( rmesa->tcl.ObjClean.data,
		      VB->AttribPtr[_TNL_ATTRIB_POS]->data,
		      VB->AttribPtr[_TNL_ATTRIB_POS]->stride,
		      GL_FLOAT,
		      VB->AttribPtr[_TNL_ATTRIB_POS]->size,
		      0,
		      VB->Count );

      switch (VB->AttribPtr[_TNL_ATTRIB_POS]->size) {
      case 1:
	    _mesa_vector4f_clean_elem(&rmesa->tcl.ObjClean, VB->Count, 1);
      case 2:
	    _mesa_vector4f_clean_elem(&rmesa->tcl.ObjClean, VB->Count, 2);
      case 3:
	 if (setup_tab[i].vertex_format & RADEON_CP_VC_FRMT_W0) {
	    _mesa_vector4f_clean_elem(&rmesa->tcl.ObjClean, VB->Count, 3);
	 }
      case 4:
      default:
	 break;
      }

      VB->AttribPtr[_TNL_ATTRIB_POS] = &rmesa->tcl.ObjClean;
   }


   radeon_bo_map(rmesa->radeon.tcl.aos[0].bo, 1);
   setup_tab[i].emit( ctx, 0, VB->Count, 
		      rmesa->radeon.tcl.aos[0].bo->ptr + rmesa->radeon.tcl.aos[0].offset);
   radeon_bo_unmap(rmesa->radeon.tcl.aos[0].bo);
   //   rmesa->radeon.tcl.aos[0].size = setup_tab[i].vertex_size;
   rmesa->radeon.tcl.aos[0].stride = setup_tab[i].vertex_size;
   rmesa->tcl.vertex_format = setup_tab[i].vertex_format;
   rmesa->radeon.tcl.aos_count = 1;
}


