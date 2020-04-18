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
#include "main/light.h"
#include "main/enums.h"

#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "radeon_common.h"
#include "radeon_context.h"
#include "radeon_state.h"
#include "radeon_ioctl.h"
#include "radeon_tcl.h"
#include "radeon_swtcl.h"
#include "radeon_maos.h"
#include "radeon_common_context.h"



/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware primitives where possible.
 * Try to simulate missing primitives with indexed vertices.
 */
#define HAVE_POINTS      1
#define HAVE_LINES       1
#define HAVE_LINE_LOOP   0
#define HAVE_LINE_STRIPS 1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0
#define HAVE_TRI_FANS    1
#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0
#define HAVE_POLYGONS    1
#define HAVE_ELTS        1


#define HW_POINTS           RADEON_CP_VC_CNTL_PRIM_TYPE_POINT
#define HW_LINES            RADEON_CP_VC_CNTL_PRIM_TYPE_LINE
#define HW_LINE_LOOP        0
#define HW_LINE_STRIP       RADEON_CP_VC_CNTL_PRIM_TYPE_LINE_STRIP
#define HW_TRIANGLES        RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST
#define HW_TRIANGLE_STRIP_0 RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_STRIP
#define HW_TRIANGLE_STRIP_1 0
#define HW_TRIANGLE_FAN     RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_FAN
#define HW_QUADS            0
#define HW_QUAD_STRIP       0
#define HW_POLYGON          RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_FAN


static GLboolean discrete_prim[0x10] = {
   0,				/* 0 none */
   1,				/* 1 points */
   1,				/* 2 lines */
   0,				/* 3 line_strip */
   1,				/* 4 tri_list */
   0,				/* 5 tri_fan */
   0,				/* 6 tri_type2 */
   1,				/* 7 rect list (unused) */
   1,				/* 8 3vert point */
   1,				/* 9 3vert line */
   0,
   0,
   0,
   0,
   0,
   0,
};
   

#define LOCAL_VARS r100ContextPtr rmesa = R100_CONTEXT(ctx)
#define ELT_TYPE  GLushort

#define ELT_INIT(prim, hw_prim) \
   radeonTclPrimitive( ctx, prim, hw_prim | RADEON_CP_VC_CNTL_PRIM_WALK_IND )

#define GET_MESA_ELTS() rmesa->tcl.Elts


/* Don't really know how many elts will fit in what's left of cmdbuf,
 * as there is state to emit, etc:
 */

/* Testing on isosurf shows a maximum around here.  Don't know if it's
 * the card or driver or kernel module that is causing the behaviour.
 */
#define GET_MAX_HW_ELTS() 300


#define RESET_STIPPLE() do {			\
   RADEON_STATECHANGE( rmesa, lin );		\
   radeonEmitState(&rmesa->radeon);			\
} while (0)

#define AUTO_STIPPLE( mode )  do {		\
   RADEON_STATECHANGE( rmesa, lin );		\
   if (mode)					\
      rmesa->hw.lin.cmd[LIN_RE_LINE_PATTERN] |=	\
	 RADEON_LINE_PATTERN_AUTO_RESET;	\
   else						\
      rmesa->hw.lin.cmd[LIN_RE_LINE_PATTERN] &=	\
	 ~RADEON_LINE_PATTERN_AUTO_RESET;	\
   radeonEmitState(&rmesa->radeon);		\
} while (0)



#define ALLOC_ELTS(nr)	radeonAllocElts( rmesa, nr )

static GLushort *radeonAllocElts( r100ContextPtr rmesa, GLuint nr ) 
{
      if (rmesa->radeon.dma.flush)
	 rmesa->radeon.dma.flush( rmesa->radeon.glCtx );

      radeonEmitAOS( rmesa,
		     rmesa->radeon.tcl.aos_count, 0 );

      return radeonAllocEltsOpenEnded( rmesa, rmesa->tcl.vertex_format,
				       rmesa->tcl.hw_primitive, nr );
}

#define CLOSE_ELTS() if (0)  RADEON_NEWPRIM( rmesa )



/* TODO: Try to extend existing primitive if both are identical,
 * discrete and there are no intervening state changes.  (Somewhat
 * duplicates changes to DrawArrays code)
 */
static void radeonEmitPrim( struct gl_context *ctx, 
		       GLenum prim, 
		       GLuint hwprim, 
		       GLuint start, 
		       GLuint count)	
{
   r100ContextPtr rmesa = R100_CONTEXT( ctx );
   radeonTclPrimitive( ctx, prim, hwprim );
   
   radeonEmitAOS( rmesa,
		  rmesa->radeon.tcl.aos_count,
		  start );
   
   /* Why couldn't this packet have taken an offset param?
    */
   radeonEmitVbufPrim( rmesa,
		       rmesa->tcl.vertex_format,
		       rmesa->tcl.hw_primitive,
		       count - start );
}

#define EMIT_PRIM( ctx, prim, hwprim, start, count ) do {       \
   radeonEmitPrim( ctx, prim, hwprim, start, count );           \
   (void) rmesa; } while (0)

#define MAX_CONVERSION_SIZE 40

/* Try & join small primitives
 */
#if 0
#define PREFER_DISCRETE_ELT_PRIM( NR, PRIM ) 0
#else
#define PREFER_DISCRETE_ELT_PRIM( NR, PRIM )			\
  ((NR) < 20 ||							\
   ((NR) < 40 &&						\
    rmesa->tcl.hw_primitive == (PRIM|				\
			    RADEON_CP_VC_CNTL_PRIM_WALK_IND|	\
			    RADEON_CP_VC_CNTL_TCL_ENABLE)))
#endif

#ifdef MESA_BIG_ENDIAN
/* We could do without (most of) this ugliness if dest was always 32 bit word aligned... */
#define EMIT_ELT(dest, offset, x) do {				\
	int off = offset + ( ( (uintptr_t)dest & 0x2 ) >> 1 );	\
	GLushort *des = (GLushort *)( (uintptr_t)dest & ~0x2 );	\
	(des)[ off + 1 - 2 * ( off & 1 ) ] = (GLushort)(x); 	\
	(void)rmesa; } while (0)
#else
#define EMIT_ELT(dest, offset, x) do {				\
	(dest)[offset] = (GLushort) (x);			\
	(void)rmesa; } while (0)
#endif

#define EMIT_TWO_ELTS(dest, offset, x, y)  *(GLuint *)(dest+offset) = ((y)<<16)|(x);



#define TAG(x) tcl_##x
#include "tnl_dd/t_dd_dmatmp2.h"

/**********************************************************************/
/*                          External entrypoints                     */
/**********************************************************************/

void radeonEmitPrimitive( struct gl_context *ctx, 
			  GLuint first,
			  GLuint last,
			  GLuint flags )
{
   tcl_render_tab_verts[flags&PRIM_MODE_MASK]( ctx, first, last, flags );
}

void radeonEmitEltPrimitive( struct gl_context *ctx, 
			     GLuint first,
			     GLuint last,
			     GLuint flags )
{
   tcl_render_tab_elts[flags&PRIM_MODE_MASK]( ctx, first, last, flags );
}

void radeonTclPrimitive( struct gl_context *ctx, 
			 GLenum prim,
			 int hw_prim )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLuint se_cntl;
   GLuint newprim = hw_prim | RADEON_CP_VC_CNTL_TCL_ENABLE;

   radeon_prepare_render(&rmesa->radeon);
   if (rmesa->radeon.NewGLState)
      radeonValidateState( ctx );

   if (newprim != rmesa->tcl.hw_primitive ||
       !discrete_prim[hw_prim&0xf]) {
      RADEON_NEWPRIM( rmesa );
      rmesa->tcl.hw_primitive = newprim;
   }

   se_cntl = rmesa->hw.set.cmd[SET_SE_CNTL];
   se_cntl &= ~RADEON_FLAT_SHADE_VTX_LAST;

   if (prim == GL_POLYGON && ctx->Light.ShadeModel == GL_FLAT) 
      se_cntl |= RADEON_FLAT_SHADE_VTX_0;
   else
      se_cntl |= RADEON_FLAT_SHADE_VTX_LAST;

   if (se_cntl != rmesa->hw.set.cmd[SET_SE_CNTL]) {
      RADEON_STATECHANGE( rmesa, set );
      rmesa->hw.set.cmd[SET_SE_CNTL] = se_cntl;
   }
}

/**
 * Predict total emit size for next rendering operation so there is no flush in middle of rendering
 * Prediction has to aim towards the best possible value that is worse than worst case scenario
 */
static GLuint radeonEnsureEmitSize( struct gl_context * ctx , GLuint inputs )
{
  r100ContextPtr rmesa = R100_CONTEXT(ctx);
  TNLcontext *tnl = TNL_CONTEXT(ctx);
  struct vertex_buffer *VB = &tnl->vb;
  GLuint space_required;
  GLuint state_size;
  GLuint nr_aos = 1; /* radeonEmitArrays does always emit one */
  int i;
  /* list of flags that are allocating aos object */
  const GLuint flags_to_check[] = {
    VERT_BIT_NORMAL,
    VERT_BIT_COLOR0,
    VERT_BIT_COLOR1,
    VERT_BIT_FOG
  };
  /* predict number of aos to emit */
  for (i=0; i < sizeof(flags_to_check)/sizeof(flags_to_check[0]); ++i)
  {
    if (inputs & flags_to_check[i])
      ++nr_aos;
  }
  for (i = 0; i < ctx->Const.MaxTextureUnits; ++i)
  {
    if (inputs & VERT_BIT_TEX(i))
      ++nr_aos;
  }

  {
    /* count the prediction for state size */
    space_required = 0;
    state_size = radeonCountStateEmitSize( &rmesa->radeon );
    /* tcl may be changed in radeonEmitArrays so account for it if not dirty */
    if (!rmesa->hw.tcl.dirty)
      state_size += rmesa->hw.tcl.check( rmesa->radeon.glCtx, &rmesa->hw.tcl );
    /* predict size for elements */
    for (i = 0; i < VB->PrimitiveCount; ++i)
    {
      /* If primitive.count is less than MAX_CONVERSION_SIZE
	 rendering code may decide convert to elts.
	 In that case we have to make pessimistic prediction.
	 and use larger of 2 paths. */
      const GLuint elts = ELTS_BUFSZ(nr_aos);
      const GLuint index = INDEX_BUFSZ;
      const GLuint vbuf = VBUF_BUFSZ;
      if (!VB->Primitive[i].count)
	continue;
      if ( (!VB->Elts && VB->Primitive[i].count >= MAX_CONVERSION_SIZE)
	  || vbuf > index + elts)
	space_required += vbuf;
      else
	space_required += index + elts;
      space_required += VB->Primitive[i].count * 3;
      space_required += AOS_BUFSZ(nr_aos);
    }
    space_required += SCISSOR_BUFSZ;
  }
  /* flush the buffer in case we need more than is left. */
  if (rcommonEnsureCmdBufSpace(&rmesa->radeon, space_required, __FUNCTION__))
    return space_required + radeonCountStateEmitSize( &rmesa->radeon );
  else
    return space_required + state_size;
}

/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/


/* TCL render.
 */
static GLboolean radeon_run_tcl_render( struct gl_context *ctx,
					struct tnl_pipeline_stage *stage )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint inputs = VERT_BIT_POS | VERT_BIT_COLOR0;
   GLuint i;
   GLuint emit_end;

   /* TODO: separate this from the swtnl pipeline 
    */
   if (rmesa->radeon.TclFallback)
      return GL_TRUE;	/* fallback to software t&l */

   if (VB->Count == 0)
      return GL_FALSE;

   /* NOTE: inputs != tnl->render_inputs - these are the untransformed
    * inputs.
    */
   if (ctx->Light.Enabled) {
      inputs |= VERT_BIT_NORMAL;
   }

   if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR) {
      inputs |= VERT_BIT_COLOR1;
   }

   if ( (ctx->Fog.FogCoordinateSource == GL_FOG_COORD) && ctx->Fog.Enabled ) {
      inputs |= VERT_BIT_FOG;
   }

   for (i = 0 ; i < ctx->Const.MaxTextureUnits; i++) {
      if (ctx->Texture.Unit[i]._ReallyEnabled) {
      /* TODO: probably should not emit texture coords when texgen is enabled */
	 if (rmesa->TexGenNeedNormals[i]) {
	    inputs |= VERT_BIT_NORMAL;
	 }
	 inputs |= VERT_BIT_TEX(i);
      }
   }

   radeonReleaseArrays( ctx, ~0 );
   emit_end = radeonEnsureEmitSize( ctx, inputs )
     + rmesa->radeon.cmdbuf.cs->cdw;
   radeonEmitArrays( ctx, inputs );

   rmesa->tcl.Elts = VB->Elts;

   for (i = 0 ; i < VB->PrimitiveCount ; i++)
   {
      GLuint prim = _tnl_translate_prim(&VB->Primitive[i]);
      GLuint start = VB->Primitive[i].start;
      GLuint length = VB->Primitive[i].count;

      if (!length)
	 continue;

      if (rmesa->tcl.Elts)
	 radeonEmitEltPrimitive( ctx, start, start+length, prim );
      else
	 radeonEmitPrimitive( ctx, start, start+length, prim );
   }

   if (emit_end < rmesa->radeon.cmdbuf.cs->cdw)
      WARN_ONCE("Rendering was %d commands larger than predicted size."
	  " We might overflow  command buffer.\n", rmesa->radeon.cmdbuf.cs->cdw - emit_end);

   return GL_FALSE;		/* finished the pipe */
}



/* Initial state for tcl stage.  
 */
const struct tnl_pipeline_stage _radeon_tcl_stage =
{
   "radeon render",
   NULL,
   NULL,
   NULL,
   NULL,
   radeon_run_tcl_render	/* run */
};



/**********************************************************************/
/*                 Validate state at pipeline start                   */
/**********************************************************************/


/*-----------------------------------------------------------------------
 * Manage TCL fallbacks
 */


static void transition_to_swtnl( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint se_cntl;

   RADEON_NEWPRIM( rmesa );
   rmesa->swtcl.vertex_format = 0;

   radeonChooseVertexState( ctx );
   radeonChooseRenderState( ctx );

   _tnl_validate_shine_tables( ctx ); 

   tnl->Driver.NotifyMaterialChange = 
      _tnl_validate_shine_tables;

   radeonReleaseArrays( ctx, ~0 );

   se_cntl = rmesa->hw.set.cmd[SET_SE_CNTL];
   se_cntl |= RADEON_FLAT_SHADE_VTX_LAST;
	 
   if (se_cntl != rmesa->hw.set.cmd[SET_SE_CNTL]) {
      RADEON_STATECHANGE( rmesa, set );
      rmesa->hw.set.cmd[SET_SE_CNTL] = se_cntl;
   }
}


static void transition_to_hwtnl( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint se_coord_fmt = rmesa->hw.set.cmd[SET_SE_COORDFMT];

   se_coord_fmt &= ~(RADEON_VTX_XY_PRE_MULT_1_OVER_W0 |
		     RADEON_VTX_Z_PRE_MULT_1_OVER_W0 |
		     RADEON_VTX_W0_IS_NOT_1_OVER_W0);
   se_coord_fmt |= RADEON_VTX_W0_IS_NOT_1_OVER_W0;

   if ( se_coord_fmt != rmesa->hw.set.cmd[SET_SE_COORDFMT] ) {
      RADEON_STATECHANGE( rmesa, set );
      rmesa->hw.set.cmd[SET_SE_COORDFMT] = se_coord_fmt;
      _tnl_need_projected_coords( ctx, GL_FALSE );
   }

   radeonUpdateMaterial( ctx );

   tnl->Driver.NotifyMaterialChange = radeonUpdateMaterial;

   if ( rmesa->radeon.dma.flush )			
      rmesa->radeon.dma.flush( rmesa->radeon.glCtx );	

   rmesa->radeon.dma.flush = NULL;
   rmesa->swtcl.vertex_format = 0;
   
   //   if (rmesa->swtcl.indexed_verts.buf) 
   //      radeonReleaseDmaRegion( rmesa, &rmesa->swtcl.indexed_verts, 
   //			      __FUNCTION__ );

   if (RADEON_DEBUG & RADEON_FALLBACKS)
      fprintf(stderr, "Radeon end tcl fallback\n");
}

static char *fallbackStrings[] = {
   "Rasterization fallback",
   "Unfilled triangles",
   "Twosided lighting, differing materials",
   "Materials in VB (maybe between begin/end)",
   "Texgen unit 0",
   "Texgen unit 1",
   "Texgen unit 2",
   "User disable",
   "Fogcoord with separate specular lighting"
};


static char *getFallbackString(GLuint bit)
{
   int i = 0;
   while (bit > 1) {
      i++;
      bit >>= 1;
   }
   return fallbackStrings[i];
}



void radeonTclFallback( struct gl_context *ctx, GLuint bit, GLboolean mode )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLuint oldfallback = rmesa->radeon.TclFallback;

   if (mode) {
      rmesa->radeon.TclFallback |= bit;
      if (oldfallback == 0) {
	 if (RADEON_DEBUG & RADEON_FALLBACKS)
	    fprintf(stderr, "Radeon begin tcl fallback %s\n",
		    getFallbackString( bit ));
	 transition_to_swtnl( ctx );
      }
   }
   else {
      rmesa->radeon.TclFallback &= ~bit;
      if (oldfallback == bit) {
	 if (RADEON_DEBUG & RADEON_FALLBACKS)
	    fprintf(stderr, "Radeon end tcl fallback %s\n",
		    getFallbackString( bit ));
	 transition_to_hwtnl( ctx );
      }
   }
}
