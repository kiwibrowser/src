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
#include "main/imports.h"
#include "main/mtypes.h"
#include "main/enums.h"
#include "main/colormac.h"
#include "main/light.h"

#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "r200_context.h"
#include "r200_state.h"
#include "r200_ioctl.h"
#include "r200_tcl.h"
#include "r200_swtcl.h"
#include "r200_maos.h"

#include "radeon_common_context.h"



#define HAVE_POINTS      1
#define HAVE_LINES       1
#define HAVE_LINE_LOOP   0
#define HAVE_LINE_STRIPS 1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0
#define HAVE_TRI_FANS    1
#define HAVE_QUADS       1
#define HAVE_QUAD_STRIPS 1
#define HAVE_POLYGONS    1
#define HAVE_ELTS        1


#define HW_POINTS           ((!(ctx->_TriangleCaps & DD_POINT_SMOOTH)) ? \
				R200_VF_PRIM_POINT_SPRITES : R200_VF_PRIM_POINTS)
#define HW_LINES            R200_VF_PRIM_LINES
#define HW_LINE_LOOP        0
#define HW_LINE_STRIP       R200_VF_PRIM_LINE_STRIP
#define HW_TRIANGLES        R200_VF_PRIM_TRIANGLES
#define HW_TRIANGLE_STRIP_0 R200_VF_PRIM_TRIANGLE_STRIP
#define HW_TRIANGLE_STRIP_1 0
#define HW_TRIANGLE_FAN     R200_VF_PRIM_TRIANGLE_FAN
#define HW_QUADS            R200_VF_PRIM_QUADS
#define HW_QUAD_STRIP       R200_VF_PRIM_QUAD_STRIP
#define HW_POLYGON          R200_VF_PRIM_POLYGON


static GLboolean discrete_prim[0x10] = {
   0,				/* 0 none */
   1,				/* 1 points */
   1,				/* 2 lines */
   0,				/* 3 line_strip */
   1,				/* 4 tri_list */
   0,				/* 5 tri_fan */
   0,				/* 6 tri_strip */
   0,				/* 7 tri_w_flags */
   1,				/* 8 rect list (unused) */
   1,				/* 9 3vert point */
   1,				/* a 3vert line */
   0,				/* b point sprite */
   0,				/* c line loop */
   1,				/* d quads */
   0,				/* e quad strip */
   0,				/* f polygon */
};
   

#define LOCAL_VARS r200ContextPtr rmesa = R200_CONTEXT(ctx)
#define ELT_TYPE  GLushort

#define ELT_INIT(prim, hw_prim) \
   r200TclPrimitive( ctx, prim, hw_prim | R200_VF_PRIM_WALK_IND )

#define GET_MESA_ELTS() TNL_CONTEXT(ctx)->vb.Elts


/* Don't really know how many elts will fit in what's left of cmdbuf,
 * as there is state to emit, etc:
 */

/* Testing on isosurf shows a maximum around here.  Don't know if it's
 * the card or driver or kernel module that is causing the behaviour.
 */
#define GET_MAX_HW_ELTS() 300

#define RESET_STIPPLE() do {			\
   R200_STATECHANGE( rmesa, lin );		\
   radeonEmitState(&rmesa->radeon);			\
} while (0)

#define AUTO_STIPPLE( mode )  do {		\
   R200_STATECHANGE( rmesa, lin );		\
   if (mode)					\
      rmesa->hw.lin.cmd[LIN_RE_LINE_PATTERN] |=	\
	 R200_LINE_PATTERN_AUTO_RESET;	\
   else						\
      rmesa->hw.lin.cmd[LIN_RE_LINE_PATTERN] &=	\
	 ~R200_LINE_PATTERN_AUTO_RESET;	\
   radeonEmitState(&rmesa->radeon);			\
} while (0)


#define ALLOC_ELTS(nr)	r200AllocElts( rmesa, nr )

static GLushort *r200AllocElts( r200ContextPtr rmesa, GLuint nr ) 
{
   if (rmesa->radeon.dma.flush == r200FlushElts &&
       rmesa->tcl.elt_used + nr*2 < R200_ELT_BUF_SZ) {

      GLushort *dest = (GLushort *)(rmesa->radeon.tcl.elt_dma_bo->ptr +
				    rmesa->radeon.tcl.elt_dma_offset + rmesa->tcl.elt_used);

      rmesa->tcl.elt_used += nr*2;

      return dest;
   }
   else {
      if (rmesa->radeon.dma.flush)
	 rmesa->radeon.dma.flush( rmesa->radeon.glCtx );

      r200EmitAOS( rmesa,
		   rmesa->radeon.tcl.aos_count, 0 );

      r200EmitMaxVtxIndex(rmesa, rmesa->radeon.tcl.aos[0].count);
      return r200AllocEltsOpenEnded( rmesa, rmesa->tcl.hw_primitive, nr );
   }
}


#define CLOSE_ELTS() 				\
do {						\
   if (0) R200_NEWPRIM( rmesa );		\
}						\
while (0)


/* TODO: Try to extend existing primitive if both are identical,
 * discrete and there are no intervening state changes.  (Somewhat
 * duplicates changes to DrawArrays code)
 */
static void r200EmitPrim( struct gl_context *ctx, 
		          GLenum prim, 
		          GLuint hwprim, 
		          GLuint start, 
		          GLuint count)	
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );
   r200TclPrimitive( ctx, prim, hwprim );
   
   //   fprintf(stderr,"Emit prim %d\n", rmesa->radeon.tcl.aos_count);

   r200EmitAOS( rmesa,
		rmesa->radeon.tcl.aos_count,
		start );
   
   /* Why couldn't this packet have taken an offset param?
    */
   r200EmitVbufPrim( rmesa,
		     rmesa->tcl.hw_primitive,
		     count - start );
}

#define EMIT_PRIM(ctx, prim, hwprim, start, count) do {         \
   r200EmitPrim( ctx, prim, hwprim, start, count );             \
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
			    R200_VF_TCL_OUTPUT_VTX_ENABLE|	\
			        R200_VF_PRIM_WALK_IND)))
#endif

#ifdef MESA_BIG_ENDIAN
/* We could do without (most of) this ugliness if dest was always 32 bit word aligned... */
#define EMIT_ELT(dest, offset, x) do {                          \
        int off = offset + ( ( (uintptr_t)dest & 0x2 ) >> 1 );     \
        GLushort *des = (GLushort *)( (uintptr_t)dest & ~0x2 );    \
        (des)[ off + 1 - 2 * ( off & 1 ) ] = (GLushort)(x);	\
	(void)rmesa; } while (0)
#else
#define EMIT_ELT(dest, offset, x) do {				\
	(dest)[offset] = (GLushort) (x);			\
	(void)rmesa; } while (0)
#endif

#define EMIT_TWO_ELTS(dest, offset, x, y)  *(GLuint *)((dest)+offset) = ((y)<<16)|(x);



#define TAG(x) tcl_##x
#include "tnl_dd/t_dd_dmatmp2.h"

/**********************************************************************/
/*                          External entrypoints                     */
/**********************************************************************/

void r200EmitPrimitive( struct gl_context *ctx, 
			  GLuint first,
			  GLuint last,
			  GLuint flags )
{
   tcl_render_tab_verts[flags&PRIM_MODE_MASK]( ctx, first, last, flags );
}

void r200EmitEltPrimitive( struct gl_context *ctx, 
			     GLuint first,
			     GLuint last,
			     GLuint flags )
{
   tcl_render_tab_elts[flags&PRIM_MODE_MASK]( ctx, first, last, flags );
}

void r200TclPrimitive( struct gl_context *ctx, 
			 GLenum prim,
			 int hw_prim )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint newprim = hw_prim | R200_VF_TCL_OUTPUT_VTX_ENABLE;

   radeon_prepare_render(&rmesa->radeon);
   if (rmesa->radeon.NewGLState)
      r200ValidateState( ctx );

   if (newprim != rmesa->tcl.hw_primitive ||
       !discrete_prim[hw_prim&0xf]) {
      /* need to disable perspective-correct texturing for point sprites */
      if ((prim & PRIM_MODE_MASK) == GL_POINTS && ctx->Point.PointSprite) {
	 if (rmesa->hw.set.cmd[SET_RE_CNTL] & R200_PERSPECTIVE_ENABLE) {
	    R200_STATECHANGE( rmesa, set );
	    rmesa->hw.set.cmd[SET_RE_CNTL] &= ~R200_PERSPECTIVE_ENABLE;
	 }
      }
      else if (!(rmesa->hw.set.cmd[SET_RE_CNTL] & R200_PERSPECTIVE_ENABLE)) {
	 R200_STATECHANGE( rmesa, set );
	 rmesa->hw.set.cmd[SET_RE_CNTL] |= R200_PERSPECTIVE_ENABLE;
      }
      R200_NEWPRIM( rmesa );
      rmesa->tcl.hw_primitive = newprim;
   }
}

/**
 * Predict total emit size for next rendering operation so there is no flush in middle of rendering
 * Prediction has to aim towards the best possible value that is worse than worst case scenario
 */
static GLuint r200EnsureEmitSize( struct gl_context * ctx , GLubyte* vimap_rev )
{
  r200ContextPtr rmesa = R200_CONTEXT(ctx);
  TNLcontext *tnl = TNL_CONTEXT(ctx);
  struct vertex_buffer *VB = &tnl->vb;
  GLuint space_required;
  GLuint state_size;
  GLuint nr_aos = 0;
  int i;
  /* predict number of aos to emit */
  for (i = 0; i < 15; ++i)
  {
    if (vimap_rev[i] != 255)
    {
      ++nr_aos;
    }
  }

  {
    /* count the prediction for state size */
    space_required = 0;
    state_size = radeonCountStateEmitSize( &rmesa->radeon );
    /* vtx may be changed in r200EmitArrays so account for it if not dirty */
    if (!rmesa->hw.vtx.dirty)
      state_size += rmesa->hw.vtx.check(rmesa->radeon.glCtx, &rmesa->hw.vtx);
    /* predict size for elements */
    for (i = 0; i < VB->PrimitiveCount; ++i)
    {
      if (!VB->Primitive[i].count)
	continue;
      /* If primitive.count is less than MAX_CONVERSION_SIZE
         rendering code may decide convert to elts.
	 In that case we have to make pessimistic prediction.
	 and use larger of 2 paths. */
      const GLuint elt_count =(VB->Primitive[i].count/GET_MAX_HW_ELTS() + 1);
      const GLuint elts = ELTS_BUFSZ(nr_aos) * elt_count;
      const GLuint index = INDEX_BUFSZ * elt_count;
      const GLuint vbuf = VBUF_BUFSZ;
      if ( (!VB->Elts && VB->Primitive[i].count >= MAX_CONVERSION_SIZE)
	  || vbuf > index + elts)
	space_required += vbuf;
      else
	space_required += index + elts;
      space_required += AOS_BUFSZ(nr_aos);
    }
  }

  radeon_print(RADEON_RENDER,RADEON_VERBOSE,
      "%s space %u, aos %d\n",
      __func__, space_required, AOS_BUFSZ(nr_aos) );
  /* flush the buffer in case we need more than is left. */
  if (rcommonEnsureCmdBufSpace(&rmesa->radeon, space_required + state_size, __FUNCTION__))
    return space_required + radeonCountStateEmitSize( &rmesa->radeon );
  else
    return space_required + state_size;
}


/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/


/* TCL render.
 */
static GLboolean r200_run_tcl_render( struct gl_context *ctx,
				      struct tnl_pipeline_stage *stage )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   GLubyte *vimap_rev;
/* use hw fixed order for simplicity, pos 0, weight 1, normal 2, fog 3, 
   color0 - color3 4-7, texcoord0 - texcoord5 8-13, pos 1 14. Must not use
   more than 12 of those at the same time. */
   GLubyte map_rev_fixed[15] = {255, 255, 255, 255, 255, 255, 255, 255,
			    255, 255, 255, 255, 255, 255, 255};


   /* TODO: separate this from the swtnl pipeline 
    */
   if (rmesa->radeon.TclFallback)
      return GL_TRUE;	/* fallback to software t&l */

   radeon_print(RADEON_RENDER, RADEON_NORMAL, "%s\n", __FUNCTION__);

   if (VB->Count == 0)
      return GL_FALSE;

   /* Validate state:
    */
   if (rmesa->radeon.NewGLState)
      if (!r200ValidateState( ctx ))
         return GL_TRUE; /* fallback to sw t&l */

   if (!ctx->VertexProgram._Enabled) {
   /* NOTE: inputs != tnl->render_inputs - these are the untransformed
    * inputs.
    */
      map_rev_fixed[0] = VERT_ATTRIB_POS;
      /* technically there is no reason we always need VA_COLOR0. In theory
         could disable it depending on lighting, color materials, texturing... */
      map_rev_fixed[4] = VERT_ATTRIB_COLOR0;

      if (ctx->Light.Enabled) {
	 map_rev_fixed[2] = VERT_ATTRIB_NORMAL;
      }

      /* this also enables VA_COLOR1 when using separate specular
         lighting model, which is unnecessary.
         FIXME: OTOH, we're missing the case where a ATI_fragment_shader accesses
         the secondary color (if lighting is disabled). The chip seems
         misconfigured for that though elsewhere (tcl output, might lock up) */
      if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR) {
	 map_rev_fixed[5] = VERT_ATTRIB_COLOR1;
      }

      if ( (ctx->Fog.FogCoordinateSource == GL_FOG_COORD) && ctx->Fog.Enabled ) {
	 map_rev_fixed[3] = VERT_ATTRIB_FOG;
      }

      for (i = 0 ; i < ctx->Const.MaxTextureUnits; i++) {
	 if (ctx->Texture.Unit[i]._ReallyEnabled) {
	    if (rmesa->TexGenNeedNormals[i]) {
	       map_rev_fixed[2] = VERT_ATTRIB_NORMAL;
	    }
	    map_rev_fixed[8 + i] = VERT_ATTRIB_TEX0 + i;
	 }
      }
      vimap_rev = &map_rev_fixed[0];
   }
   else {
      /* vtx_tcl_output_vtxfmt_0/1 need to match configuration of "fragment
	 part", since using some vertex interpolator later which is not in
	 out_vtxfmt0/1 will lock up. It seems to be ok to write in vertex
	 prog to a not enabled output however, so just don't mess with it.
	 We only need to change compsel. */
      GLuint out_compsel = 0;
      const GLbitfield64 vp_out =
	 rmesa->curr_vp_hw->mesa_program.Base.OutputsWritten;

      vimap_rev = &rmesa->curr_vp_hw->inputmap_rev[0];
      assert(vp_out & BITFIELD64_BIT(VERT_RESULT_HPOS));
      out_compsel = R200_OUTPUT_XYZW;
      if (vp_out & BITFIELD64_BIT(VERT_RESULT_COL0)) {
	 out_compsel |= R200_OUTPUT_COLOR_0;
      }
      if (vp_out & BITFIELD64_BIT(VERT_RESULT_COL1)) {
	 out_compsel |= R200_OUTPUT_COLOR_1;
      }
      if (vp_out & BITFIELD64_BIT(VERT_RESULT_FOGC)) {
         out_compsel |= R200_OUTPUT_DISCRETE_FOG;
      }
      if (vp_out & BITFIELD64_BIT(VERT_RESULT_PSIZ)) {
	 out_compsel |= R200_OUTPUT_PT_SIZE;
      }
      for (i = VERT_RESULT_TEX0; i < VERT_RESULT_TEX6; i++) {
	 if (vp_out & BITFIELD64_BIT(i)) {
	    out_compsel |= R200_OUTPUT_TEX_0 << (i - VERT_RESULT_TEX0);
	 }
      }
      if (rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] != out_compsel) {
	 R200_STATECHANGE( rmesa, vtx );
	 rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] = out_compsel;
      }
   }

   /* Do the actual work:
    */
   radeonReleaseArrays( ctx, ~0 /* stage->changed_inputs */ );
   GLuint emit_end = r200EnsureEmitSize( ctx, vimap_rev )
     + rmesa->radeon.cmdbuf.cs->cdw;
   r200EmitArrays( ctx, vimap_rev );

   for (i = 0 ; i < VB->PrimitiveCount ; i++)
   {
      GLuint prim = _tnl_translate_prim(&VB->Primitive[i]);
      GLuint start = VB->Primitive[i].start;
      GLuint length = VB->Primitive[i].count;

      if (!length)
	 continue;

      if (VB->Elts)
	 r200EmitEltPrimitive( ctx, start, start+length, prim );
      else
	 r200EmitPrimitive( ctx, start, start+length, prim );
   }
   if ( emit_end < rmesa->radeon.cmdbuf.cs->cdw )
     WARN_ONCE("Rendering was %d commands larger than predicted size."
	 " We might overflow  command buffer.\n", rmesa->radeon.cmdbuf.cs->cdw - emit_end);

   return GL_FALSE;		/* finished the pipe */
}



/* Initial state for tcl stage.  
 */
const struct tnl_pipeline_stage _r200_tcl_stage =
{
   "r200 render",
   NULL,			/*  private */
   NULL,
   NULL,
   NULL,
   r200_run_tcl_render	/* run */
};



/**********************************************************************/
/*                 Validate state at pipeline start                   */
/**********************************************************************/


/*-----------------------------------------------------------------------
 * Manage TCL fallbacks
 */


static void transition_to_swtnl( struct gl_context *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   R200_NEWPRIM( rmesa );

   r200ChooseVertexState( ctx );
   r200ChooseRenderState( ctx );

   _tnl_validate_shine_tables( ctx ); 

   tnl->Driver.NotifyMaterialChange = 
      _tnl_validate_shine_tables;

   radeonReleaseArrays( ctx, ~0 );

   /* Still using the D3D based hardware-rasterizer from the radeon;
    * need to put the card into D3D mode to make it work:
    */
   R200_STATECHANGE( rmesa, vap );
   rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL] &= ~(R200_VAP_TCL_ENABLE|R200_VAP_PROG_VTX_SHADER_ENABLE);
}

static void transition_to_hwtnl( struct gl_context *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   _tnl_need_projected_coords( ctx, GL_FALSE );

   r200UpdateMaterial( ctx );

   tnl->Driver.NotifyMaterialChange = r200UpdateMaterial;

   if ( rmesa->radeon.dma.flush )			
      rmesa->radeon.dma.flush( rmesa->radeon.glCtx );	

   rmesa->radeon.dma.flush = NULL;
   
   R200_STATECHANGE( rmesa, vap );
   rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL] |= R200_VAP_TCL_ENABLE;
   rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL] &= ~R200_VAP_FORCE_W_TO_ONE;

   if (ctx->VertexProgram._Enabled) {
      rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL] |= R200_VAP_PROG_VTX_SHADER_ENABLE;
   }

   if ( ((rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] & R200_FOG_USE_MASK)
      == R200_FOG_USE_SPEC_ALPHA) &&
      (ctx->Fog.FogCoordinateSource == GL_FOG_COORD )) {
      R200_STATECHANGE( rmesa, ctx );
      rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] &= ~R200_FOG_USE_MASK;
      rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] |= R200_FOG_USE_VTX_FOG;
   }

   R200_STATECHANGE( rmesa, vte );
   rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL] &= ~(R200_VTX_XY_FMT|R200_VTX_Z_FMT);
   rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL] |= R200_VTX_W0_FMT;

   if (R200_DEBUG & RADEON_FALLBACKS)
      fprintf(stderr, "R200 end tcl fallback\n");
}


static char *fallbackStrings[] = {
   "Rasterization fallback",
   "Unfilled triangles",
   "Twosided lighting, differing materials",
   "Materials in VB (maybe between begin/end)",
   "Texgen unit 0",
   "Texgen unit 1",
   "Texgen unit 2",
   "Texgen unit 3",
   "Texgen unit 4",
   "Texgen unit 5",
   "User disable",
   "Bitmap as points",
   "Vertex program"
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



void r200TclFallback( struct gl_context *ctx, GLuint bit, GLboolean mode )
{
	r200ContextPtr rmesa = R200_CONTEXT(ctx);
	GLuint oldfallback = rmesa->radeon.TclFallback;

	if (mode) {
		if (oldfallback == 0) {
			/* We have to flush before transition */
			if ( rmesa->radeon.dma.flush )
				rmesa->radeon.dma.flush( rmesa->radeon.glCtx );

			if (R200_DEBUG & RADEON_FALLBACKS)
				fprintf(stderr, "R200 begin tcl fallback %s\n",
						getFallbackString( bit ));
			rmesa->radeon.TclFallback |= bit;
			transition_to_swtnl( ctx );
		} else
			rmesa->radeon.TclFallback |= bit;
	} else {
		if (oldfallback == bit) {
			/* We have to flush before transition */
			if ( rmesa->radeon.dma.flush )
				rmesa->radeon.dma.flush( rmesa->radeon.glCtx );

			if (R200_DEBUG & RADEON_FALLBACKS)
				fprintf(stderr, "R200 end tcl fallback %s\n",
						getFallbackString( bit ));
			rmesa->radeon.TclFallback &= ~bit;
			transition_to_hwtnl( ctx );
		} else
			rmesa->radeon.TclFallback &= ~bit;
	}
}
