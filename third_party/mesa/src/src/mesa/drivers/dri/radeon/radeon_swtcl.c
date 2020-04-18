/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

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
#include "main/mtypes.h"
#include "main/colormac.h"
#include "main/enums.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/simple_list.h"

#include "math/m_xform.h"

#include "swrast_setup/swrast_setup.h"

#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "radeon_context.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "radeon_swtcl.h"
#include "radeon_tcl.h"
#include "radeon_debug.h"


/* R100: xyzw, c0, c1/fog, stq[0..2]  = 4+1+1+3*3 = 15  right? */
/* R200: xyzw, c0, c1/fog, strq[0..5] = 4+1+1+4*6 = 30 */
#define RADEON_MAX_TNL_VERTEX_SIZE (15 * sizeof(GLfloat))	/* for mesa _tnl stage */

/***********************************************************************
 *                         Initialization 
 ***********************************************************************/

#define EMIT_ATTR( ATTR, STYLE, F0 )					\
do {									\
   rmesa->radeon.swtcl.vertex_attrs[rmesa->radeon.swtcl.vertex_attr_count].attrib = (ATTR);	\
   rmesa->radeon.swtcl.vertex_attrs[rmesa->radeon.swtcl.vertex_attr_count].format = (STYLE);	\
   rmesa->radeon.swtcl.vertex_attr_count++;					\
   fmt_0 |= F0;								\
} while (0)

#define EMIT_PAD( N )							\
do {									\
   rmesa->radeon.swtcl.vertex_attrs[rmesa->radeon.swtcl.vertex_attr_count].attrib = 0;		\
   rmesa->radeon.swtcl.vertex_attrs[rmesa->radeon.swtcl.vertex_attr_count].format = EMIT_PAD;	\
   rmesa->radeon.swtcl.vertex_attrs[rmesa->radeon.swtcl.vertex_attr_count].offset = (N);		\
   rmesa->radeon.swtcl.vertex_attr_count++;					\
} while (0)

static GLuint radeon_cp_vc_frmts[3][2] =
{
   { RADEON_CP_VC_FRMT_ST0, RADEON_CP_VC_FRMT_ST0 | RADEON_CP_VC_FRMT_Q0 },
   { RADEON_CP_VC_FRMT_ST1, RADEON_CP_VC_FRMT_ST1 | RADEON_CP_VC_FRMT_Q1 },
   { RADEON_CP_VC_FRMT_ST2, RADEON_CP_VC_FRMT_ST2 | RADEON_CP_VC_FRMT_Q2 },
};

static void radeonSetVertexFormat( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLbitfield64 index_bitset = tnl->render_inputs_bitset;
   int fmt_0 = 0;
   int offset = 0;

   /* Important:
    */
   if ( VB->NdcPtr != NULL ) {
      VB->AttribPtr[VERT_ATTRIB_POS] = VB->NdcPtr;
   }
   else {
      VB->AttribPtr[VERT_ATTRIB_POS] = VB->ClipPtr;
   }

   assert( VB->AttribPtr[VERT_ATTRIB_POS] != NULL );
   rmesa->radeon.swtcl.vertex_attr_count = 0;

   /* EMIT_ATTR's must be in order as they tell t_vertex.c how to
    * build up a hardware vertex.
    */
   if ( !rmesa->swtcl.needproj ||
        (index_bitset & BITFIELD64_RANGE(_TNL_ATTRIB_TEX0, _TNL_NUM_TEX))) {
      /* for projtex */
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_4F, 
		 RADEON_CP_VC_FRMT_XY |	RADEON_CP_VC_FRMT_Z | RADEON_CP_VC_FRMT_W0 );
      offset = 4;
   }
   else {
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_3F, 
		 RADEON_CP_VC_FRMT_XY |	RADEON_CP_VC_FRMT_Z );
      offset = 3;
   }

   rmesa->swtcl.coloroffset = offset;
#if MESA_LITTLE_ENDIAN 
   EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_RGBA, 
	      RADEON_CP_VC_FRMT_PKCOLOR );
#else
   EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_ABGR,
	      RADEON_CP_VC_FRMT_PKCOLOR );
#endif
   offset += 1;

   rmesa->swtcl.specoffset = 0;
   if (index_bitset &
       (BITFIELD64_BIT(_TNL_ATTRIB_COLOR1) | BITFIELD64_BIT(_TNL_ATTRIB_FOG))) {

#if MESA_LITTLE_ENDIAN 
      if (index_bitset & BITFIELD64_BIT(_TNL_ATTRIB_COLOR1)) {
	 rmesa->swtcl.specoffset = offset;
	 EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_3UB_3F_RGB,
	 	    RADEON_CP_VC_FRMT_PKSPEC );
      }
      else {
	 EMIT_PAD( 3 );
      }

      if (index_bitset & BITFIELD64_BIT(_TNL_ATTRIB_FOG)) {
	 EMIT_ATTR( _TNL_ATTRIB_FOG, EMIT_1UB_1F,
	 	    RADEON_CP_VC_FRMT_PKSPEC );
      }
      else {
	 EMIT_PAD( 1 );
      }
#else
      if (index_bitset & BITFIELD64_BIT(_TNL_ATTRIB_FOG)) {
	 EMIT_ATTR( _TNL_ATTRIB_FOG, EMIT_1UB_1F,
	 	    RADEON_CP_VC_FRMT_PKSPEC );
      }
      else {
	 EMIT_PAD( 1 );
      }

      if (index_bitset & BITFIELD64_BIT(_TNL_ATTRIB_COLOR1)) {
	 rmesa->swtcl.specoffset = offset;
	 EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_3UB_3F_BGR,
	 	    RADEON_CP_VC_FRMT_PKSPEC );
      }
      else {
	 EMIT_PAD( 3 );
      }
#endif
   }

   if (index_bitset & BITFIELD64_RANGE(_TNL_ATTRIB_TEX0, _TNL_NUM_TEX)) {
      int i;

      for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
	 if (index_bitset & BITFIELD64_BIT(_TNL_ATTRIB_TEX(i))) {
	    GLuint sz = VB->AttribPtr[_TNL_ATTRIB_TEX0 + i]->size;

	    switch (sz) {
	    case 1:
	    case 2:
	       EMIT_ATTR( _TNL_ATTRIB_TEX0+i, EMIT_2F,
			  radeon_cp_vc_frmts[i][0] );
	       break;
	    case 3:
	       if (ctx->Texture.Unit[i]._ReallyEnabled & (TEXTURE_CUBE_BIT) ) {
	           EMIT_ATTR( _TNL_ATTRIB_TEX0+i, EMIT_3F,
			      radeon_cp_vc_frmts[i][1] );
               } else {
	           EMIT_ATTR( _TNL_ATTRIB_TEX0+i, EMIT_2F,
			      radeon_cp_vc_frmts[i][0] );
               }
               break;
	    case 4:
	       if (ctx->Texture.Unit[i]._ReallyEnabled & (TEXTURE_CUBE_BIT) ) {
		  EMIT_ATTR( _TNL_ATTRIB_TEX0+i, EMIT_3F,
			     radeon_cp_vc_frmts[i][1] );
	       } else {
		  EMIT_ATTR( _TNL_ATTRIB_TEX0+i, EMIT_3F_XYW,
			     radeon_cp_vc_frmts[i][1] );
	       }
	       break;
	    default:
	       continue;
	    };
	 }
      }
   }

   if (rmesa->radeon.tnl_index_bitset != index_bitset ||
       fmt_0 != rmesa->swtcl.vertex_format) {
      RADEON_NEWPRIM(rmesa);
      rmesa->swtcl.vertex_format = fmt_0;
      rmesa->radeon.swtcl.vertex_size =
	  _tnl_install_attrs( ctx,
			      rmesa->radeon.swtcl.vertex_attrs, 
			      rmesa->radeon.swtcl.vertex_attr_count,
			      NULL, 0 );
      rmesa->radeon.swtcl.vertex_size /= 4;
      rmesa->radeon.tnl_index_bitset = index_bitset;
      radeon_print(RADEON_SWRENDER, RADEON_VERBOSE,
	  "%s: vertex_size= %d floats\n",  __FUNCTION__, rmesa->radeon.swtcl.vertex_size);
   }
}

static void radeon_predict_emit_size( r100ContextPtr rmesa )
{

    if (!rmesa->radeon.swtcl.emit_prediction) {
        const int state_size = radeonCountStateEmitSize( &rmesa->radeon );
        const int scissor_size = 8;
        const int prims_size = 8;
        const int vertex_size = 7;

        if (rcommonEnsureCmdBufSpace(&rmesa->radeon,
                    state_size +
                    (scissor_size + prims_size + vertex_size),
                    __FUNCTION__))
            rmesa->radeon.swtcl.emit_prediction = radeonCountStateEmitSize( &rmesa->radeon );
        else
            rmesa->radeon.swtcl.emit_prediction = state_size;
        rmesa->radeon.swtcl.emit_prediction += scissor_size + prims_size + vertex_size
            + rmesa->radeon.cmdbuf.cs->cdw;
    }
}

static void radeonRenderStart( struct gl_context *ctx )
{
    r100ContextPtr rmesa = R100_CONTEXT( ctx );

    radeonSetVertexFormat( ctx );

    if (rmesa->radeon.dma.flush != 0 &&
            rmesa->radeon.dma.flush != rcommon_flush_last_swtcl_prim)
        rmesa->radeon.dma.flush( ctx );
}


/**
 * Set vertex state for SW TCL.  The primary purpose of this function is to
 * determine in advance whether or not the hardware can / should do the
 * projection divide or Mesa should do it.
 */
void radeonChooseVertexState( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   GLuint se_coord_fmt = rmesa->hw.set.cmd[SET_SE_COORDFMT];
   
   se_coord_fmt &= ~(RADEON_VTX_XY_PRE_MULT_1_OVER_W0 |
		     RADEON_VTX_Z_PRE_MULT_1_OVER_W0 |
		     RADEON_VTX_W0_IS_NOT_1_OVER_W0);

   /* We must ensure that we don't do _tnl_need_projected_coords while in a
    * rasterization fallback.  As this function will be called again when we
    * leave a rasterization fallback, we can just skip it for now.
    */
   if (rmesa->radeon.Fallback != 0)
      return;

   /* HW perspective divide is a win, but tiny vertex formats are a
    * bigger one.
    */

   if ((0 == (tnl->render_inputs_bitset & 
        (BITFIELD64_RANGE(_TNL_ATTRIB_TEX0, _TNL_NUM_TEX)
         | BITFIELD64_BIT(_TNL_ATTRIB_COLOR1))))
        || (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
      rmesa->swtcl.needproj = GL_TRUE;
      se_coord_fmt |= (RADEON_VTX_XY_PRE_MULT_1_OVER_W0 |
		      RADEON_VTX_Z_PRE_MULT_1_OVER_W0);
   }
   else {
      rmesa->swtcl.needproj = GL_FALSE;
      se_coord_fmt |= (RADEON_VTX_W0_IS_NOT_1_OVER_W0);
   }

   _tnl_need_projected_coords( ctx, rmesa->swtcl.needproj );

   if ( se_coord_fmt != rmesa->hw.set.cmd[SET_SE_COORDFMT] ) {
      RADEON_STATECHANGE( rmesa, set );
      rmesa->hw.set.cmd[SET_SE_COORDFMT] = se_coord_fmt;
   }
}

void r100_swtcl_flush(struct gl_context *ctx, uint32_t current_offset)
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);



   radeonEmitState(&rmesa->radeon);
   radeonEmitVertexAOS( rmesa,
			rmesa->radeon.swtcl.vertex_size,
			rmesa->radeon.swtcl.bo,
			current_offset);

		      
   radeonEmitVbufPrim( rmesa,
		       rmesa->swtcl.vertex_format,
		       rmesa->radeon.swtcl.hw_primitive,
		       rmesa->radeon.swtcl.numverts);
   if ( rmesa->radeon.swtcl.emit_prediction < rmesa->radeon.cmdbuf.cs->cdw )
     WARN_ONCE("Rendering was %d commands larger than predicted size."
	 " We might overflow  command buffer.\n",
	 rmesa->radeon.cmdbuf.cs->cdw - rmesa->radeon.swtcl.emit_prediction );


   rmesa->radeon.swtcl.emit_prediction = 0;

}

/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware primitives where possible.
 * Try to simulate missing primitives with indexed vertices.
 */
#define HAVE_POINTS      1
#define HAVE_LINES       1
#define HAVE_LINE_STRIPS 1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0
#define HAVE_TRI_FANS    1
#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0
#define HAVE_POLYGONS    0
/* \todo: is it possible to make "ELTS" work with t_vertex code ? */
#define HAVE_ELTS        0

static const GLuint hw_prim[GL_POLYGON+1] = {
   RADEON_CP_VC_CNTL_PRIM_TYPE_POINT,
   RADEON_CP_VC_CNTL_PRIM_TYPE_LINE,
   0,
   RADEON_CP_VC_CNTL_PRIM_TYPE_LINE_STRIP,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_STRIP,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_FAN,
   0,
   0,
   0
};

static INLINE void
radeonDmaPrimitive( r100ContextPtr rmesa, GLenum prim )
{
   RADEON_NEWPRIM( rmesa );
   rmesa->radeon.swtcl.hw_primitive = hw_prim[prim];
   //   assert(rmesa->radeon.dma.current.ptr == rmesa->radeon.dma.current.start);
}

static void* radeon_alloc_verts( r100ContextPtr rmesa , GLuint nr, GLuint size )
{
   void *rv;
   do {
     radeon_predict_emit_size( rmesa );
     rv = rcommonAllocDmaLowVerts( &rmesa->radeon, nr, size );
   } while (!rv);
   return rv;
}

#define LOCAL_VARS r100ContextPtr rmesa = R100_CONTEXT(ctx)
#define INIT( prim ) radeonDmaPrimitive( rmesa, prim )
#define FLUSH()  RADEON_NEWPRIM( rmesa )
#define GET_CURRENT_VB_MAX_VERTS()					10\
//  (((int)rmesa->radeon.dma.current.end - (int)rmesa->radeon.dma.current.ptr) / (rmesa->radeon.swtcl.vertex_size*4))
#define GET_SUBSEQUENT_VB_MAX_VERTS() \
  ((RADEON_BUFFER_SIZE) / (rmesa->radeon.swtcl.vertex_size*4))
#define ALLOC_VERTS( nr ) radeon_alloc_verts( rmesa, nr, rmesa->radeon.swtcl.vertex_size * 4 )
#define EMIT_VERTS( ctx, j, nr, buf ) \
  _tnl_emit_vertices_to_buffer(ctx, j, (j)+(nr), buf)

#define TAG(x) radeon_dma_##x
#include "tnl_dd/t_dd_dmatmp.h"


/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/


static GLboolean radeon_run_render( struct gl_context *ctx,
				    struct tnl_pipeline_stage *stage )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   tnl_render_func *tab = TAG(render_tab_verts);
   GLuint i;

   if (rmesa->radeon.swtcl.RenderIndex != 0 ||   
       !radeon_dma_validate_render( ctx, VB ))
      return GL_TRUE;		

   radeon_prepare_render(&rmesa->radeon);
   if (rmesa->radeon.NewGLState)
      radeonValidateState( ctx );

   tnl->Driver.Render.Start( ctx );

   for (i = 0 ; i < VB->PrimitiveCount ; i++)
   {
      GLuint prim = VB->Primitive[i].mode;
      GLuint start = VB->Primitive[i].start;
      GLuint length = VB->Primitive[i].count;

      if (!length)
	 continue;

      radeon_print(RADEON_SWRENDER, RADEON_NORMAL,
	  "radeon_render.c: prim %s %d..%d\n",
		 _mesa_lookup_enum_by_nr(prim & PRIM_MODE_MASK), 
		 start, start+length);

      if (length)
	 tab[prim & PRIM_MODE_MASK]( ctx, start, start + length, prim );
   }

   tnl->Driver.Render.Finish( ctx );

   return GL_FALSE;		/* finished the pipe */
}



const struct tnl_pipeline_stage _radeon_render_stage =
{
   "radeon render",
   NULL,
   NULL,
   NULL,
   NULL,
   radeon_run_render		/* run */
};


/**************************************************************************/


static const GLuint reduced_hw_prim[GL_POLYGON+1] = {
   RADEON_CP_VC_CNTL_PRIM_TYPE_POINT,
   RADEON_CP_VC_CNTL_PRIM_TYPE_LINE,
   RADEON_CP_VC_CNTL_PRIM_TYPE_LINE,
   RADEON_CP_VC_CNTL_PRIM_TYPE_LINE,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST
};

static void radeonRasterPrimitive( struct gl_context *ctx, GLuint hwprim );
static void radeonRenderPrimitive( struct gl_context *ctx, GLenum prim );
static void radeonResetLineStipple( struct gl_context *ctx );


/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/

#undef LOCAL_VARS
#undef ALLOC_VERTS
#define CTX_ARG r100ContextPtr rmesa
#define GET_VERTEX_DWORDS() rmesa->radeon.swtcl.vertex_size
#define ALLOC_VERTS( n, size ) radeon_alloc_verts( rmesa, n, (size) * 4 )
#undef LOCAL_VARS
#define LOCAL_VARS						\
   r100ContextPtr rmesa = R100_CONTEXT(ctx);		\
   const char *radeonverts = (char *)rmesa->radeon.swtcl.verts;
#define VERT(x) (radeonVertex *)(radeonverts + ((x) * (vertsize) * sizeof(int)))
#define VERTEX radeonVertex 
#undef TAG
#define TAG(x) radeon_##x
#include "tnl_dd/t_dd_triemit.h"


/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define QUAD( a, b, c, d ) radeon_quad( rmesa, a, b, c, d )
#define TRI( a, b, c )     radeon_triangle( rmesa, a, b, c )
#define LINE( a, b )       radeon_line( rmesa, a, b )
#define POINT( a )         radeon_point( rmesa, a )

/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define RADEON_TWOSIDE_BIT	0x01
#define RADEON_UNFILLED_BIT	0x02
#define RADEON_MAX_TRIFUNC	0x04


static struct {
   tnl_points_func	        points;
   tnl_line_func		line;
   tnl_triangle_func	triangle;
   tnl_quad_func		quad;
} rast_tab[RADEON_MAX_TRIFUNC];


#define DO_FALLBACK  0
#define DO_OFFSET    0
#define DO_UNFILLED (IND & RADEON_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & RADEON_TWOSIDE_BIT)
#define DO_FLAT      0
#define DO_TRI       1
#define DO_QUAD      1
#define DO_LINE      1
#define DO_POINTS    1
#define DO_FULL_QUAD 1

#define HAVE_SPEC   1
#define HAVE_BACK_COLORS  0
#define HAVE_HW_FLATSHADE 1
#define TAB rast_tab

#define DEPTH_SCALE 1.0
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW( a ) (a < 0)
#define GET_VERTEX(e) (rmesa->radeon.swtcl.verts + ((e) * rmesa->radeon.swtcl.vertex_size * sizeof(int)))

#define VERT_SET_RGBA( v, c )  					\
do {								\
   radeon_color_t *color = (radeon_color_t *)&((v)->ui[coloroffset]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(color->red, (c)[0]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->green, (c)[1]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->blue, (c)[2]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->alpha, (c)[3]);		\
} while (0)

#define VERT_COPY_RGBA( v0, v1 ) v0->ui[coloroffset] = v1->ui[coloroffset]

#define VERT_SET_SPEC( v, c )					\
do {								\
   if (specoffset) {						\
      radeon_color_t *spec = (radeon_color_t *)&((v)->ui[specoffset]);	\
      UNCLAMPED_FLOAT_TO_UBYTE(spec->red, (c)[0]);	\
      UNCLAMPED_FLOAT_TO_UBYTE(spec->green, (c)[1]);	\
      UNCLAMPED_FLOAT_TO_UBYTE(spec->blue, (c)[2]);	\
   }								\
} while (0)
#define VERT_COPY_SPEC( v0, v1 )			\
do {							\
   if (specoffset) {					\
      radeon_color_t *spec0 = (radeon_color_t *)&((v0)->ui[specoffset]);	\
      radeon_color_t *spec1 = (radeon_color_t *)&((v1)->ui[specoffset]);	\
      spec0->red   = spec1->red;	\
      spec0->green = spec1->green;	\
      spec0->blue  = spec1->blue; 	\
   }							\
} while (0)

/* These don't need LE32_TO_CPU() as they used to save and restore
 * colors which are already in the correct format.
 */
#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[coloroffset] = color[idx]
#define VERT_SAVE_SPEC( idx )    if (specoffset) spec[idx] = v[idx]->ui[specoffset]
#define VERT_RESTORE_SPEC( idx ) if (specoffset) v[idx]->ui[specoffset] = spec[idx]

#undef LOCAL_VARS
#undef TAG
#undef INIT

#define LOCAL_VARS(n)							\
   r100ContextPtr rmesa = R100_CONTEXT(ctx);			\
   GLuint color[n] = {0}, spec[n] = {0};						\
   GLuint coloroffset = rmesa->swtcl.coloroffset;	\
   GLuint specoffset = rmesa->swtcl.specoffset;			\
   (void) color; (void) spec; (void) coloroffset; (void) specoffset;

/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

#define RASTERIZE(x) radeonRasterPrimitive( ctx, reduced_hw_prim[x] )
#define RENDER_PRIMITIVE rmesa->radeon.swtcl.render_primitive
#undef TAG
#define TAG(x) x
#include "tnl_dd/t_dd_unfilled.h"
#undef IND


/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/


#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (RADEON_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (RADEON_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (RADEON_TWOSIDE_BIT|RADEON_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"


static void init_rast_tab( void )
{
   init();
   init_twoside();
   init_unfilled();
   init_twoside_unfilled();
}

/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

#define RENDER_POINTS( start, count )		\
   for ( ; start < count ; start++)		\
      radeon_point( rmesa, VERT(start) )
#define RENDER_LINE( v0, v1 ) \
   radeon_line( rmesa, VERT(v0), VERT(v1) )
#define RENDER_TRI( v0, v1, v2 )  \
   radeon_triangle( rmesa, VERT(v0), VERT(v1), VERT(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) \
   radeon_quad( rmesa, VERT(v0), VERT(v1), VERT(v2), VERT(v3) )
#undef INIT
#define INIT(x) do {					\
   radeonRenderPrimitive( ctx, x );			\
} while (0)
#undef LOCAL_VARS
#define LOCAL_VARS						\
   r100ContextPtr rmesa = R100_CONTEXT(ctx);		\
   const GLuint vertsize = rmesa->radeon.swtcl.vertex_size;		\
   const char *radeonverts = (char *)rmesa->radeon.swtcl.verts;		\
   const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
   const GLboolean stipple = ctx->Line.StippleFlag;		\
   (void) elt; (void) stipple;
#define RESET_STIPPLE	if ( stipple ) radeonResetLineStipple( ctx );
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) (x)
#define TAG(x) radeon_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) radeon_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"



/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/

void radeonChooseRenderState( struct gl_context *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLuint index = 0;
   GLuint flags = ctx->_TriangleCaps;

   if (!rmesa->radeon.TclFallback || rmesa->radeon.Fallback) 
      return;

   if (flags & DD_TRI_LIGHT_TWOSIDE) index |= RADEON_TWOSIDE_BIT;
   if (flags & DD_TRI_UNFILLED)      index |= RADEON_UNFILLED_BIT;

   if (index != rmesa->radeon.swtcl.RenderIndex) {
      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.ClippedLine = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;

      if (index == 0) {
	 tnl->Driver.Render.PrimTabVerts = radeon_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = radeon_render_tab_elts;
	 tnl->Driver.Render.ClippedPolygon = radeon_fast_clipped_poly;
      } else {
	 tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
	 tnl->Driver.Render.ClippedPolygon = _tnl_RenderClippedPolygon;
      }

      rmesa->radeon.swtcl.RenderIndex = index;
   }
}


/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/


static void radeonRasterPrimitive( struct gl_context *ctx, GLuint hwprim )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);

   if (rmesa->radeon.swtcl.hw_primitive != hwprim) {
      RADEON_NEWPRIM( rmesa );
      rmesa->radeon.swtcl.hw_primitive = hwprim;
   }
}

static void radeonRenderPrimitive( struct gl_context *ctx, GLenum prim )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   rmesa->radeon.swtcl.render_primitive = prim;
   if (prim < GL_TRIANGLES || !(ctx->_TriangleCaps & DD_TRI_UNFILLED)) 
      radeonRasterPrimitive( ctx, reduced_hw_prim[prim] );
}

static void radeonRenderFinish( struct gl_context *ctx )
{
}

static void radeonResetLineStipple( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   RADEON_STATECHANGE( rmesa, lin );
}


/**********************************************************************/
/*           Transition to/from hardware rasterization.               */
/**********************************************************************/

static const char * const fallbackStrings[] = {
   "Texture mode",
   "glDrawBuffer(GL_FRONT_AND_BACK)",
   "glEnable(GL_STENCIL) without hw stencil buffer",
   "glRenderMode(selection or feedback)",
   "glBlendEquation",
   "glBlendFunc",
   "RADEON_NO_RAST",
   "Mixing GL_CLAMP_TO_BORDER and GL_CLAMP (or GL_MIRROR_CLAMP_ATI)"
};


static const char *getFallbackString(GLuint bit)
{
   int i = 0;
   while (bit > 1) {
      i++;
      bit >>= 1;
   }
   return fallbackStrings[i];
}


void radeonFallback( struct gl_context *ctx, GLuint bit, GLboolean mode )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint oldfallback = rmesa->radeon.Fallback;

   if (mode) {
      rmesa->radeon.Fallback |= bit;
      if (oldfallback == 0) {
	 radeon_firevertices(&rmesa->radeon);
	 TCL_FALLBACK( ctx, RADEON_TCL_FALLBACK_RASTER, GL_TRUE );
	 _swsetup_Wakeup( ctx );
	 rmesa->radeon.swtcl.RenderIndex = ~0;
         if (RADEON_DEBUG & RADEON_FALLBACKS) {
            fprintf(stderr, "Radeon begin rasterization fallback: 0x%x %s\n",
                    bit, getFallbackString(bit));
         }
      }
   }
   else {
      rmesa->radeon.Fallback &= ~bit;
      if (oldfallback == bit) {
	 _swrast_flush( ctx );
	 tnl->Driver.Render.Start = radeonRenderStart;
	 tnl->Driver.Render.PrimitiveNotify = radeonRenderPrimitive;
	 tnl->Driver.Render.Finish = radeonRenderFinish;

	 tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
	 tnl->Driver.Render.CopyPV = _tnl_copy_pv;
	 tnl->Driver.Render.Interp = _tnl_interp;

	 tnl->Driver.Render.ResetLineStipple = radeonResetLineStipple;
	 TCL_FALLBACK( ctx, RADEON_TCL_FALLBACK_RASTER, GL_FALSE );
	 if (rmesa->radeon.TclFallback) {
	    /* These are already done if rmesa->radeon.TclFallback goes to
	     * zero above. But not if it doesn't (RADEON_NO_TCL for
	     * example?)
	     */
	    _tnl_invalidate_vertex_state( ctx, ~0 );
	    _tnl_invalidate_vertices( ctx, ~0 );
	    rmesa->radeon.tnl_index_bitset = 0;
	    radeonChooseVertexState( ctx );
	    radeonChooseRenderState( ctx );
	 }
         if (RADEON_DEBUG & RADEON_FALLBACKS) {
            fprintf(stderr, "Radeon end rasterization fallback: 0x%x %s\n",
                    bit, getFallbackString(bit));
         }
      }
   }
}


/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/

void radeonInitSwtcl( struct gl_context *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      firsttime = 0;
   }
   rmesa->radeon.swtcl.emit_prediction = 0;

   tnl->Driver.Render.Start = radeonRenderStart;
   tnl->Driver.Render.Finish = radeonRenderFinish;
   tnl->Driver.Render.PrimitiveNotify = radeonRenderPrimitive;
   tnl->Driver.Render.ResetLineStipple = radeonResetLineStipple;
   tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
   tnl->Driver.Render.CopyPV = _tnl_copy_pv;
   tnl->Driver.Render.Interp = _tnl_interp;

   _tnl_init_vertices( ctx, ctx->Const.MaxArrayLockSize + 12, 
		       RADEON_MAX_TNL_VERTEX_SIZE);
   
   rmesa->radeon.swtcl.verts = (GLubyte *)tnl->clipspace.vertex_buf;
   rmesa->radeon.swtcl.RenderIndex = ~0;
   rmesa->radeon.swtcl.render_primitive = GL_TRIANGLES;
   rmesa->radeon.swtcl.hw_primitive = 0;
}

