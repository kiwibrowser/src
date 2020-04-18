/**************************************************************************

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
#include "main/api_arrayelt.h"
#include "main/enums.h"
#include "main/colormac.h"
#include "main/light.h"
#include "main/framebuffer.h"
#include "main/fbobject.h"

#include "swrast/swrast.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "swrast_setup/swrast_setup.h"
#include "drivers/common/meta.h"

#include "radeon_common.h"
#include "radeon_mipmap_tree.h"
#include "r200_context.h"
#include "r200_ioctl.h"
#include "r200_state.h"
#include "r200_tcl.h"
#include "r200_tex.h"
#include "r200_swtcl.h"
#include "r200_vertprog.h"


/* =============================================================
 * Alpha blending
 */

static void r200AlphaFunc( struct gl_context *ctx, GLenum func, GLfloat ref )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   int pp_misc = rmesa->hw.ctx.cmd[CTX_PP_MISC];
   GLubyte refByte;

   CLAMPED_FLOAT_TO_UBYTE(refByte, ref);

   R200_STATECHANGE( rmesa, ctx );

   pp_misc &= ~(R200_ALPHA_TEST_OP_MASK | R200_REF_ALPHA_MASK);
   pp_misc |= (refByte & R200_REF_ALPHA_MASK);

   switch ( func ) {
   case GL_NEVER:
      pp_misc |= R200_ALPHA_TEST_FAIL;
      break;
   case GL_LESS:
      pp_misc |= R200_ALPHA_TEST_LESS;
      break;
   case GL_EQUAL:
      pp_misc |= R200_ALPHA_TEST_EQUAL;
      break;
   case GL_LEQUAL:
      pp_misc |= R200_ALPHA_TEST_LEQUAL;
      break;
   case GL_GREATER:
      pp_misc |= R200_ALPHA_TEST_GREATER;
      break;
   case GL_NOTEQUAL:
      pp_misc |= R200_ALPHA_TEST_NEQUAL;
      break;
   case GL_GEQUAL:
      pp_misc |= R200_ALPHA_TEST_GEQUAL;
      break;
   case GL_ALWAYS:
      pp_misc |= R200_ALPHA_TEST_PASS;
      break;
   }

   rmesa->hw.ctx.cmd[CTX_PP_MISC] = pp_misc;
}

static void r200BlendColor( struct gl_context *ctx, const GLfloat cf[4] )
{
   GLubyte color[4];
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   R200_STATECHANGE( rmesa, ctx );
   CLAMPED_FLOAT_TO_UBYTE(color[0], cf[0]);
   CLAMPED_FLOAT_TO_UBYTE(color[1], cf[1]);
   CLAMPED_FLOAT_TO_UBYTE(color[2], cf[2]);
   CLAMPED_FLOAT_TO_UBYTE(color[3], cf[3]);
   rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCOLOR] = radeonPackColor( 4, color[0], color[1], color[2], color[3] );
}

/**
 * Calculate the hardware blend factor setting.  This same function is used
 * for source and destination of both alpha and RGB.
 *
 * \returns
 * The hardware register value for the specified blend factor.  This value
 * will need to be shifted into the correct position for either source or
 * destination factor.
 *
 * \todo
 * Since the two cases where source and destination are handled differently
 * are essentially error cases, they should never happen.  Determine if these
 * cases can be removed.
 */
static int blend_factor( GLenum factor, GLboolean is_src )
{
   int func;

   switch ( factor ) {
   case GL_ZERO:
      func = R200_BLEND_GL_ZERO;
      break;
   case GL_ONE:
      func = R200_BLEND_GL_ONE;
      break;
   case GL_DST_COLOR:
      func = R200_BLEND_GL_DST_COLOR;
      break;
   case GL_ONE_MINUS_DST_COLOR:
      func = R200_BLEND_GL_ONE_MINUS_DST_COLOR;
      break;
   case GL_SRC_COLOR:
      func = R200_BLEND_GL_SRC_COLOR;
      break;
   case GL_ONE_MINUS_SRC_COLOR:
      func = R200_BLEND_GL_ONE_MINUS_SRC_COLOR;
      break;
   case GL_SRC_ALPHA:
      func = R200_BLEND_GL_SRC_ALPHA;
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      func = R200_BLEND_GL_ONE_MINUS_SRC_ALPHA;
      break;
   case GL_DST_ALPHA:
      func = R200_BLEND_GL_DST_ALPHA;
      break;
   case GL_ONE_MINUS_DST_ALPHA:
      func = R200_BLEND_GL_ONE_MINUS_DST_ALPHA;
      break;
   case GL_SRC_ALPHA_SATURATE:
      func = (is_src) ? R200_BLEND_GL_SRC_ALPHA_SATURATE : R200_BLEND_GL_ZERO;
      break;
   case GL_CONSTANT_COLOR:
      func = R200_BLEND_GL_CONST_COLOR;
      break;
   case GL_ONE_MINUS_CONSTANT_COLOR:
      func = R200_BLEND_GL_ONE_MINUS_CONST_COLOR;
      break;
   case GL_CONSTANT_ALPHA:
      func = R200_BLEND_GL_CONST_ALPHA;
      break;
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      func = R200_BLEND_GL_ONE_MINUS_CONST_ALPHA;
      break;
   default:
      func = (is_src) ? R200_BLEND_GL_ONE : R200_BLEND_GL_ZERO;
   }
   return func;
}

/**
 * Sets both the blend equation and the blend function.
 * This is done in a single
 * function because some blend equations (i.e., \c GL_MIN and \c GL_MAX)
 * change the interpretation of the blend function.
 * Also, make sure that blend function and blend equation are set to their default
 * value if color blending is not enabled, since at least blend equations GL_MIN
 * and GL_FUNC_REVERSE_SUBTRACT will cause wrong results otherwise for
 * unknown reasons.
 */
static void r200_set_blend_state( struct gl_context * ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint cntl = rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &
      ~(R200_ROP_ENABLE | R200_ALPHA_BLEND_ENABLE | R200_SEPARATE_ALPHA_ENABLE);

   int func = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
      (R200_BLEND_GL_ZERO << R200_DST_BLEND_SHIFT);
   int eqn = R200_COMB_FCN_ADD_CLAMP;
   int funcA = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
      (R200_BLEND_GL_ZERO << R200_DST_BLEND_SHIFT);
   int eqnA = R200_COMB_FCN_ADD_CLAMP;

   R200_STATECHANGE( rmesa, ctx );

   if (ctx->Color.ColorLogicOpEnabled) {
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] =  cntl | R200_ROP_ENABLE;
      rmesa->hw.ctx.cmd[CTX_RB3D_ABLENDCNTL] = eqn | func;
      rmesa->hw.ctx.cmd[CTX_RB3D_CBLENDCNTL] = eqn | func;
      return;
   } else if (ctx->Color.BlendEnabled) {
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] =  cntl | R200_ALPHA_BLEND_ENABLE | R200_SEPARATE_ALPHA_ENABLE;
   }
   else {
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] = cntl;
      rmesa->hw.ctx.cmd[CTX_RB3D_ABLENDCNTL] = eqn | func;
      rmesa->hw.ctx.cmd[CTX_RB3D_CBLENDCNTL] = eqn | func;
      return;
   }

   func = (blend_factor( ctx->Color.Blend[0].SrcRGB, GL_TRUE ) << R200_SRC_BLEND_SHIFT) |
      (blend_factor( ctx->Color.Blend[0].DstRGB, GL_FALSE ) << R200_DST_BLEND_SHIFT);

   switch(ctx->Color.Blend[0].EquationRGB) {
   case GL_FUNC_ADD:
      eqn = R200_COMB_FCN_ADD_CLAMP;
      break;

   case GL_FUNC_SUBTRACT:
      eqn = R200_COMB_FCN_SUB_CLAMP;
      break;

   case GL_FUNC_REVERSE_SUBTRACT:
      eqn = R200_COMB_FCN_RSUB_CLAMP;
      break;

   case GL_MIN:
      eqn = R200_COMB_FCN_MIN;
      func = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
         (R200_BLEND_GL_ONE << R200_DST_BLEND_SHIFT);
      break;

   case GL_MAX:
      eqn = R200_COMB_FCN_MAX;
      func = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
         (R200_BLEND_GL_ONE << R200_DST_BLEND_SHIFT);
      break;

   default:
      fprintf( stderr, "[%s:%u] Invalid RGB blend equation (0x%04x).\n",
         __FUNCTION__, __LINE__, ctx->Color.Blend[0].EquationRGB );
      return;
   }

   funcA = (blend_factor( ctx->Color.Blend[0].SrcA, GL_TRUE ) << R200_SRC_BLEND_SHIFT) |
      (blend_factor( ctx->Color.Blend[0].DstA, GL_FALSE ) << R200_DST_BLEND_SHIFT);

   switch(ctx->Color.Blend[0].EquationA) {
   case GL_FUNC_ADD:
      eqnA = R200_COMB_FCN_ADD_CLAMP;
      break;

   case GL_FUNC_SUBTRACT:
      eqnA = R200_COMB_FCN_SUB_CLAMP;
      break;

   case GL_FUNC_REVERSE_SUBTRACT:
      eqnA = R200_COMB_FCN_RSUB_CLAMP;
      break;

   case GL_MIN:
      eqnA = R200_COMB_FCN_MIN;
      funcA = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
         (R200_BLEND_GL_ONE << R200_DST_BLEND_SHIFT);
      break;

   case GL_MAX:
      eqnA = R200_COMB_FCN_MAX;
      funcA = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
         (R200_BLEND_GL_ONE << R200_DST_BLEND_SHIFT);
      break;

   default:
      fprintf( stderr, "[%s:%u] Invalid A blend equation (0x%04x).\n",
         __FUNCTION__, __LINE__, ctx->Color.Blend[0].EquationA );
      return;
   }

   rmesa->hw.ctx.cmd[CTX_RB3D_ABLENDCNTL] = eqnA | funcA;
   rmesa->hw.ctx.cmd[CTX_RB3D_CBLENDCNTL] = eqn | func;

}

static void r200BlendEquationSeparate( struct gl_context *ctx,
				       GLenum modeRGB, GLenum modeA )
{
      r200_set_blend_state( ctx );
}

static void r200BlendFuncSeparate( struct gl_context *ctx,
				     GLenum sfactorRGB, GLenum dfactorRGB,
				     GLenum sfactorA, GLenum dfactorA )
{
      r200_set_blend_state( ctx );
}


/* =============================================================
 * Depth testing
 */

static void r200DepthFunc( struct gl_context *ctx, GLenum func )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   R200_STATECHANGE( rmesa, ctx );
   rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] &= ~R200_Z_TEST_MASK;

   switch ( ctx->Depth.Func ) {
   case GL_NEVER:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_TEST_NEVER;
      break;
   case GL_LESS:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_TEST_LESS;
      break;
   case GL_EQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_TEST_EQUAL;
      break;
   case GL_LEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_TEST_LEQUAL;
      break;
   case GL_GREATER:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_TEST_GREATER;
      break;
   case GL_NOTEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_TEST_NEQUAL;
      break;
   case GL_GEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_TEST_GEQUAL;
      break;
   case GL_ALWAYS:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_TEST_ALWAYS;
      break;
   }
}

static void r200DepthMask( struct gl_context *ctx, GLboolean flag )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   R200_STATECHANGE( rmesa, ctx );

   if ( ctx->Depth.Mask ) {
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |=  R200_Z_WRITE_ENABLE;
   } else {
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] &= ~R200_Z_WRITE_ENABLE;
   }
}


/* =============================================================
 * Fog
 */


static void r200Fogfv( struct gl_context *ctx, GLenum pname, const GLfloat *param )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   union { int i; float f; } c, d;
   GLubyte col[4];
   GLuint i;

   c.i = rmesa->hw.fog.cmd[FOG_C];
   d.i = rmesa->hw.fog.cmd[FOG_D];

   switch (pname) {
   case GL_FOG_MODE:
      if (!ctx->Fog.Enabled)
	 return;
      R200_STATECHANGE(rmesa, tcl);
      rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~R200_TCL_FOG_MASK;
      switch (ctx->Fog.Mode) {
      case GL_LINEAR:
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= R200_TCL_FOG_LINEAR;
	 if (ctx->Fog.Start == ctx->Fog.End) {
	    c.f = 1.0F;
	    d.f = 1.0F;
	 }
	 else {
	    c.f = ctx->Fog.End/(ctx->Fog.End-ctx->Fog.Start);
	    d.f = -1.0/(ctx->Fog.End-ctx->Fog.Start);
	 }
	 break;
      case GL_EXP:
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= R200_TCL_FOG_EXP;
	 c.f = 0.0;
	 d.f = -ctx->Fog.Density;
	 break;
      case GL_EXP2:
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= R200_TCL_FOG_EXP2;
	 c.f = 0.0;
	 d.f = -(ctx->Fog.Density * ctx->Fog.Density);
	 break;
      default:
	 return;
      }
      break;
   case GL_FOG_DENSITY:
      switch (ctx->Fog.Mode) {
      case GL_EXP:
	 c.f = 0.0;
	 d.f = -ctx->Fog.Density;
	 break;
      case GL_EXP2:
	 c.f = 0.0;
	 d.f = -(ctx->Fog.Density * ctx->Fog.Density);
	 break;
      default:
	 break;
      }
      break;
   case GL_FOG_START:
   case GL_FOG_END:
      if (ctx->Fog.Mode == GL_LINEAR) {
	 if (ctx->Fog.Start == ctx->Fog.End) {
	    c.f = 1.0F;
	    d.f = 1.0F;
	 } else {
	    c.f = ctx->Fog.End/(ctx->Fog.End-ctx->Fog.Start);
	    d.f = -1.0/(ctx->Fog.End-ctx->Fog.Start);
	 }
      }
      break;
   case GL_FOG_COLOR:
      R200_STATECHANGE( rmesa, ctx );
      _mesa_unclamped_float_rgba_to_ubyte(col, ctx->Fog.Color );
      i = radeonPackColor( 4, col[0], col[1], col[2], 0 );
      rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] &= ~R200_FOG_COLOR_MASK;
      rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] |= i;
      break;
   case GL_FOG_COORD_SRC: {
      GLuint out_0 = rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0];
      GLuint fog   = rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR];

      fog &= ~R200_FOG_USE_MASK;
      if ( ctx->Fog.FogCoordinateSource == GL_FOG_COORD || ctx->VertexProgram.Enabled) {
	 fog   |= R200_FOG_USE_VTX_FOG;
	 out_0 |= R200_VTX_DISCRETE_FOG;
      }
      else {
	 fog   |=  R200_FOG_USE_SPEC_ALPHA;
	 out_0 &= ~R200_VTX_DISCRETE_FOG;
      }

      if ( fog != rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] ) {
	 R200_STATECHANGE( rmesa, ctx );
	 rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] = fog;
      }

      if (out_0 != rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0]) {
	 R200_STATECHANGE( rmesa, vtx );
	 rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] = out_0;
      }

      break;
   }
   default:
      return;
   }

   if (c.i != rmesa->hw.fog.cmd[FOG_C] || d.i != rmesa->hw.fog.cmd[FOG_D]) {
      R200_STATECHANGE( rmesa, fog );
      rmesa->hw.fog.cmd[FOG_C] = c.i;
      rmesa->hw.fog.cmd[FOG_D] = d.i;
   }
}

/* =============================================================
 * Culling
 */

static void r200CullFace( struct gl_context *ctx, GLenum unused )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint s = rmesa->hw.set.cmd[SET_SE_CNTL];
   GLuint t = rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL];

   s |= R200_FFACE_SOLID | R200_BFACE_SOLID;
   t &= ~(R200_CULL_FRONT | R200_CULL_BACK);

   if ( ctx->Polygon.CullFlag ) {
      switch ( ctx->Polygon.CullFaceMode ) {
      case GL_FRONT:
	 s &= ~R200_FFACE_SOLID;
	 t |= R200_CULL_FRONT;
	 break;
      case GL_BACK:
	 s &= ~R200_BFACE_SOLID;
	 t |= R200_CULL_BACK;
	 break;
      case GL_FRONT_AND_BACK:
	 s &= ~(R200_FFACE_SOLID | R200_BFACE_SOLID);
	 t |= (R200_CULL_FRONT | R200_CULL_BACK);
	 break;
      }
   }

   if ( rmesa->hw.set.cmd[SET_SE_CNTL] != s ) {
      R200_STATECHANGE(rmesa, set );
      rmesa->hw.set.cmd[SET_SE_CNTL] = s;
   }

   if ( rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] != t ) {
      R200_STATECHANGE(rmesa, tcl );
      rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] = t;
   }
}

static void r200FrontFace( struct gl_context *ctx, GLenum mode )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   int cull_face = (mode == GL_CW) ? R200_FFACE_CULL_CW : R200_FFACE_CULL_CCW;

   R200_STATECHANGE( rmesa, set );
   rmesa->hw.set.cmd[SET_SE_CNTL] &= ~R200_FFACE_CULL_DIR_MASK;

   R200_STATECHANGE( rmesa, tcl );
   rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~R200_CULL_FRONT_IS_CCW;

   /* Winding is inverted when rendering to FBO */
   if (ctx->DrawBuffer && _mesa_is_user_fbo(ctx->DrawBuffer))
      cull_face = (mode == GL_CCW) ? R200_FFACE_CULL_CW : R200_FFACE_CULL_CCW;
   rmesa->hw.set.cmd[SET_SE_CNTL] |= cull_face;

   if ( mode == GL_CCW )
      rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= R200_CULL_FRONT_IS_CCW;
}

/* =============================================================
 * Point state
 */
static void r200PointSize( struct gl_context *ctx, GLfloat size )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *fcmd = (GLfloat *)rmesa->hw.ptp.cmd;

   radeon_print(RADEON_STATE, RADEON_TRACE,
       "%s(%p) size: %f, fixed point result: %d.%d (%d/16)\n",
       __func__, ctx, size,
       ((GLuint)(ctx->Point.Size * 16.0))/16,
       (((GLuint)(ctx->Point.Size * 16.0))&15)*100/16,
       ((GLuint)(ctx->Point.Size * 16.0))&15);

   R200_STATECHANGE( rmesa, cst );
   R200_STATECHANGE( rmesa, ptp );
   rmesa->hw.cst.cmd[CST_RE_POINTSIZE] &= ~0xffff;
   rmesa->hw.cst.cmd[CST_RE_POINTSIZE] |= ((GLuint)(ctx->Point.Size * 16.0));
/* this is the size param of the point size calculation (point size reg value
   is not used when calculation is active). */
   fcmd[PTP_VPORT_SCALE_PTSIZE] = ctx->Point.Size;
}

static void r200PointParameter( struct gl_context *ctx, GLenum pname, const GLfloat *params)
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *fcmd = (GLfloat *)rmesa->hw.ptp.cmd;

   switch (pname) {
   case GL_POINT_SIZE_MIN:
   /* Can clamp both in tcl and setup - just set both (as does fglrx) */
      R200_STATECHANGE( rmesa, lin );
      R200_STATECHANGE( rmesa, ptp );
      rmesa->hw.lin.cmd[LIN_SE_LINE_WIDTH] &= 0xffff;
      rmesa->hw.lin.cmd[LIN_SE_LINE_WIDTH] |= (GLuint)(ctx->Point.MinSize * 16.0) << 16;
      fcmd[PTP_CLAMP_MIN] = ctx->Point.MinSize;
      break;
   case GL_POINT_SIZE_MAX:
      R200_STATECHANGE( rmesa, cst );
      R200_STATECHANGE( rmesa, ptp );
      rmesa->hw.cst.cmd[CST_RE_POINTSIZE] &= 0xffff;
      rmesa->hw.cst.cmd[CST_RE_POINTSIZE] |= (GLuint)(ctx->Point.MaxSize * 16.0) << 16;
      fcmd[PTP_CLAMP_MAX] = ctx->Point.MaxSize;
      break;
   case GL_POINT_DISTANCE_ATTENUATION:
      R200_STATECHANGE( rmesa, vtx );
      R200_STATECHANGE( rmesa, spr );
      R200_STATECHANGE( rmesa, ptp );
      GLfloat *fcmd = (GLfloat *)rmesa->hw.ptp.cmd;
      rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] &=
	 ~(R200_PS_MULT_MASK | R200_PS_LIN_ATT_ZERO | R200_PS_SE_SEL_STATE);
      /* can't rely on ctx->Point._Attenuated here and test for NEW_POINT in
	 r200ValidateState looks like overkill */
      if (ctx->Point.Params[0] != 1.0 ||
	  ctx->Point.Params[1] != 0.0 ||
	  ctx->Point.Params[2] != 0.0 ||
	  (ctx->VertexProgram.Enabled && ctx->VertexProgram.PointSizeEnabled)) {
	 /* all we care for vp would be the ps_se_sel_state setting */
	 fcmd[PTP_ATT_CONST_QUAD] = ctx->Point.Params[2];
	 fcmd[PTP_ATT_CONST_LIN] = ctx->Point.Params[1];
	 fcmd[PTP_ATT_CONST_CON] = ctx->Point.Params[0];
	 rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] |= R200_PS_MULT_ATTENCONST;
	 if (ctx->Point.Params[1] == 0.0)
	    rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] |= R200_PS_LIN_ATT_ZERO;
/* FIXME: setting this here doesn't look quite ok - we only want to do
          that if we're actually drawing points probably */
	 rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] |= R200_OUTPUT_PT_SIZE;
	 rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] |= R200_VTX_POINT_SIZE;
      }
      else {
	 rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] |=
	    R200_PS_SE_SEL_STATE | R200_PS_MULT_CONST;
	 rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] &= ~R200_OUTPUT_PT_SIZE;
	 rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] &= ~R200_VTX_POINT_SIZE;
      }
      break;
   case GL_POINT_FADE_THRESHOLD_SIZE:
      /* don't support multisampling, so doesn't matter. */
      break;
   /* can't do these but don't need them.
   case GL_POINT_SPRITE_R_MODE_NV:
   case GL_POINT_SPRITE_COORD_ORIGIN: */
   default:
      fprintf(stderr, "bad pname parameter in r200PointParameter\n");
      return;
   }
}

/* =============================================================
 * Line state
 */
static void r200LineWidth( struct gl_context *ctx, GLfloat widthf )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   R200_STATECHANGE( rmesa, lin );
   R200_STATECHANGE( rmesa, set );

   /* Line width is stored in U6.4 format.
    * Same min/max limits for AA, non-AA lines.
    */
   rmesa->hw.lin.cmd[LIN_SE_LINE_WIDTH] &= ~0xffff;
   rmesa->hw.lin.cmd[LIN_SE_LINE_WIDTH] |= (GLuint)
      (CLAMP(widthf, ctx->Const.MinLineWidth, ctx->Const.MaxLineWidth) * 16.0);

   if ( widthf > 1.0 ) {
      rmesa->hw.set.cmd[SET_SE_CNTL] |=  R200_WIDELINE_ENABLE;
   } else {
      rmesa->hw.set.cmd[SET_SE_CNTL] &= ~R200_WIDELINE_ENABLE;
   }
}

static void r200LineStipple( struct gl_context *ctx, GLint factor, GLushort pattern )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   R200_STATECHANGE( rmesa, lin );
   rmesa->hw.lin.cmd[LIN_RE_LINE_PATTERN] =
      ((((GLuint)factor & 0xff) << 16) | ((GLuint)pattern));
}


/* =============================================================
 * Masks
 */
static void r200ColorMask( struct gl_context *ctx,
			   GLboolean r, GLboolean g,
			   GLboolean b, GLboolean a )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint mask;
   struct radeon_renderbuffer *rrb;
   GLuint flag = rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] & ~R200_PLANE_MASK_ENABLE;

   rrb = radeon_get_colorbuffer(&rmesa->radeon);
   if (!rrb)
     return;
   mask = radeonPackColor( rrb->cpp,
			   ctx->Color.ColorMask[0][RCOMP],
			   ctx->Color.ColorMask[0][GCOMP],
			   ctx->Color.ColorMask[0][BCOMP],
			   ctx->Color.ColorMask[0][ACOMP] );


   if (!(r && g && b && a))
      flag |= R200_PLANE_MASK_ENABLE;

   if ( rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] != flag ) {
      R200_STATECHANGE( rmesa, ctx );
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] = flag;
   }

   if ( rmesa->hw.msk.cmd[MSK_RB3D_PLANEMASK] != mask ) {
      R200_STATECHANGE( rmesa, msk );
      rmesa->hw.msk.cmd[MSK_RB3D_PLANEMASK] = mask;
   }
}


/* =============================================================
 * Polygon state
 */

static void r200PolygonOffset( struct gl_context *ctx,
			       GLfloat factor, GLfloat units )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   const GLfloat depthScale = 1.0F / ctx->DrawBuffer->_DepthMaxF;
   float_ui32_type constant =  { units * depthScale };
   float_ui32_type factoru = { factor };

/*    factor *= 2; */
/*    constant *= 2; */

/*    fprintf(stderr, "%s f:%f u:%f\n", __FUNCTION__, factor, constant); */

   R200_STATECHANGE( rmesa, zbs );
   rmesa->hw.zbs.cmd[ZBS_SE_ZBIAS_FACTOR]   = factoru.ui32;
   rmesa->hw.zbs.cmd[ZBS_SE_ZBIAS_CONSTANT] = constant.ui32;
}

static void r200PolygonMode( struct gl_context *ctx, GLenum face, GLenum mode )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLboolean flag = (ctx->_TriangleCaps & DD_TRI_UNFILLED) != 0;

   /* Can't generally do unfilled via tcl, but some good special
    * cases work.
    */
   TCL_FALLBACK( ctx, R200_TCL_FALLBACK_UNFILLED, flag);
   if (rmesa->radeon.TclFallback) {
      r200ChooseRenderState( ctx );
      r200ChooseVertexState( ctx );
   }
}


/* =============================================================
 * Rendering attributes
 *
 * We really don't want to recalculate all this every time we bind a
 * texture.  These things shouldn't change all that often, so it makes
 * sense to break them out of the core texture state update routines.
 */

/* Examine lighting and texture state to determine if separate specular
 * should be enabled.
 */
static void r200UpdateSpecular( struct gl_context *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   uint32_t p = rmesa->hw.ctx.cmd[CTX_PP_CNTL];

   R200_STATECHANGE( rmesa, tcl );
   R200_STATECHANGE( rmesa, vtx );

   rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] &= ~(3<<R200_VTX_COLOR_0_SHIFT);
   rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] &= ~(3<<R200_VTX_COLOR_1_SHIFT);
   rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] &= ~R200_OUTPUT_COLOR_0;
   rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] &= ~R200_OUTPUT_COLOR_1;
   rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] &= ~R200_LIGHTING_ENABLE;

   p &= ~R200_SPECULAR_ENABLE;

   rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] |= R200_DIFFUSE_SPECULAR_COMBINE;


   if (ctx->Light.Enabled &&
       ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR) {
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] |=
	 ((R200_VTX_FP_RGBA << R200_VTX_COLOR_0_SHIFT) |
	  (R200_VTX_FP_RGBA << R200_VTX_COLOR_1_SHIFT));
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] |= R200_OUTPUT_COLOR_0;
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] |= R200_OUTPUT_COLOR_1;
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] |= R200_LIGHTING_ENABLE;
      p |=  R200_SPECULAR_ENABLE;
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] &=
	 ~R200_DIFFUSE_SPECULAR_COMBINE;
   }
   else if (ctx->Light.Enabled) {
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] |=
	 ((R200_VTX_FP_RGBA << R200_VTX_COLOR_0_SHIFT));
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] |= R200_OUTPUT_COLOR_0;
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] |= R200_LIGHTING_ENABLE;
   } else if (ctx->Fog.ColorSumEnabled ) {
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] |=
	 ((R200_VTX_FP_RGBA << R200_VTX_COLOR_0_SHIFT) |
	  (R200_VTX_FP_RGBA << R200_VTX_COLOR_1_SHIFT));
      p |=  R200_SPECULAR_ENABLE;
   } else {
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] |=
	 ((R200_VTX_FP_RGBA << R200_VTX_COLOR_0_SHIFT));
   }

   if (ctx->Fog.Enabled) {
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] |=
	 ((R200_VTX_FP_RGBA << R200_VTX_COLOR_1_SHIFT));
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] |= R200_OUTPUT_COLOR_1;
   }

   if ( rmesa->hw.ctx.cmd[CTX_PP_CNTL] != p ) {
      R200_STATECHANGE( rmesa, ctx );
      rmesa->hw.ctx.cmd[CTX_PP_CNTL] = p;
   }

   /* Update vertex/render formats
    */
   if (rmesa->radeon.TclFallback) {
      r200ChooseRenderState( ctx );
      r200ChooseVertexState( ctx );
   }
}


/* =============================================================
 * Materials
 */


/* Update on colormaterial, material emmissive/ambient,
 * lightmodel.globalambient
 */
static void update_global_ambient( struct gl_context *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   float *fcmd = (float *)R200_DB_STATE( glt );

   /* Need to do more if both emmissive & ambient are PREMULT:
    * I believe this is not nessary when using source_material. This condition thus
    * will never happen currently, and the function has no dependencies on materials now
    */
   if ((rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_1] &
       ((3 << R200_FRONT_EMISSIVE_SOURCE_SHIFT) |
	(3 << R200_FRONT_AMBIENT_SOURCE_SHIFT))) == 0)
   {
      COPY_3V( &fcmd[GLT_RED],
	       ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_EMISSION]);
      ACC_SCALE_3V( &fcmd[GLT_RED],
		   ctx->Light.Model.Ambient,
		   ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_AMBIENT]);
   }
   else
   {
      COPY_3V( &fcmd[GLT_RED], ctx->Light.Model.Ambient );
   }

   R200_DB_STATECHANGE(rmesa, &rmesa->hw.glt);
}

/* Update on change to
 *    - light[p].colors
 *    - light[p].enabled
 */
static void update_light_colors( struct gl_context *ctx, GLuint p )
{
   struct gl_light *l = &ctx->Light.Light[p];

/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   if (l->Enabled) {
      r200ContextPtr rmesa = R200_CONTEXT(ctx);
      float *fcmd = (float *)R200_DB_STATE( lit[p] );

      COPY_4V( &fcmd[LIT_AMBIENT_RED], l->Ambient );
      COPY_4V( &fcmd[LIT_DIFFUSE_RED], l->Diffuse );
      COPY_4V( &fcmd[LIT_SPECULAR_RED], l->Specular );

      R200_DB_STATECHANGE( rmesa, &rmesa->hw.lit[p] );
   }
}

static void r200ColorMaterial( struct gl_context *ctx, GLenum face, GLenum mode )
{
      r200ContextPtr rmesa = R200_CONTEXT(ctx);
      GLuint light_model_ctl1 = rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_1];
      light_model_ctl1 &= ~((0xf << R200_FRONT_EMISSIVE_SOURCE_SHIFT) |
			   (0xf << R200_FRONT_AMBIENT_SOURCE_SHIFT) |
			   (0xf << R200_FRONT_DIFFUSE_SOURCE_SHIFT) |
		   (0xf << R200_FRONT_SPECULAR_SOURCE_SHIFT) |
		   (0xf << R200_BACK_EMISSIVE_SOURCE_SHIFT) |
		   (0xf << R200_BACK_AMBIENT_SOURCE_SHIFT) |
		   (0xf << R200_BACK_DIFFUSE_SOURCE_SHIFT) |
		   (0xf << R200_BACK_SPECULAR_SOURCE_SHIFT));

   if (ctx->Light.ColorMaterialEnabled) {
      GLuint mask = ctx->Light._ColorMaterialBitmask;

      if (mask & MAT_BIT_FRONT_EMISSION) {
	 light_model_ctl1 |= (R200_LM1_SOURCE_VERTEX_COLOR_0 <<
			     R200_FRONT_EMISSIVE_SOURCE_SHIFT);
      }
      else
	 light_model_ctl1 |= (R200_LM1_SOURCE_MATERIAL_0 <<
			     R200_FRONT_EMISSIVE_SOURCE_SHIFT);

      if (mask & MAT_BIT_FRONT_AMBIENT) {
	 light_model_ctl1 |= (R200_LM1_SOURCE_VERTEX_COLOR_0 <<
			     R200_FRONT_AMBIENT_SOURCE_SHIFT);
      }
      else
         light_model_ctl1 |= (R200_LM1_SOURCE_MATERIAL_0 <<
			     R200_FRONT_AMBIENT_SOURCE_SHIFT);

      if (mask & MAT_BIT_FRONT_DIFFUSE) {
	 light_model_ctl1 |= (R200_LM1_SOURCE_VERTEX_COLOR_0 <<
			     R200_FRONT_DIFFUSE_SOURCE_SHIFT);
      }
      else
         light_model_ctl1 |= (R200_LM1_SOURCE_MATERIAL_0 <<
			     R200_FRONT_DIFFUSE_SOURCE_SHIFT);

      if (mask & MAT_BIT_FRONT_SPECULAR) {
	 light_model_ctl1 |= (R200_LM1_SOURCE_VERTEX_COLOR_0 <<
			     R200_FRONT_SPECULAR_SOURCE_SHIFT);
      }
      else {
         light_model_ctl1 |= (R200_LM1_SOURCE_MATERIAL_0 <<
			     R200_FRONT_SPECULAR_SOURCE_SHIFT);
      }

      if (mask & MAT_BIT_BACK_EMISSION) {
	 light_model_ctl1 |= (R200_LM1_SOURCE_VERTEX_COLOR_0 <<
			     R200_BACK_EMISSIVE_SOURCE_SHIFT);
      }

      else light_model_ctl1 |= (R200_LM1_SOURCE_MATERIAL_1 <<
			     R200_BACK_EMISSIVE_SOURCE_SHIFT);

      if (mask & MAT_BIT_BACK_AMBIENT) {
	 light_model_ctl1 |= (R200_LM1_SOURCE_VERTEX_COLOR_0 <<
			     R200_BACK_AMBIENT_SOURCE_SHIFT);
      }
      else light_model_ctl1 |= (R200_LM1_SOURCE_MATERIAL_1 <<
			     R200_BACK_AMBIENT_SOURCE_SHIFT);

      if (mask & MAT_BIT_BACK_DIFFUSE) {
	 light_model_ctl1 |= (R200_LM1_SOURCE_VERTEX_COLOR_0 <<
			     R200_BACK_DIFFUSE_SOURCE_SHIFT);
   }
      else light_model_ctl1 |= (R200_LM1_SOURCE_MATERIAL_1 <<
			     R200_BACK_DIFFUSE_SOURCE_SHIFT);

      if (mask & MAT_BIT_BACK_SPECULAR) {
	 light_model_ctl1 |= (R200_LM1_SOURCE_VERTEX_COLOR_0 <<
			     R200_BACK_SPECULAR_SOURCE_SHIFT);
      }
      else {
         light_model_ctl1 |= (R200_LM1_SOURCE_MATERIAL_1 <<
			     R200_BACK_SPECULAR_SOURCE_SHIFT);
      }
      }
   else {
       /* Default to SOURCE_MATERIAL:
        */
     light_model_ctl1 |=
        (R200_LM1_SOURCE_MATERIAL_0 << R200_FRONT_EMISSIVE_SOURCE_SHIFT) |
        (R200_LM1_SOURCE_MATERIAL_0 << R200_FRONT_AMBIENT_SOURCE_SHIFT) |
        (R200_LM1_SOURCE_MATERIAL_0 << R200_FRONT_DIFFUSE_SOURCE_SHIFT) |
        (R200_LM1_SOURCE_MATERIAL_0 << R200_FRONT_SPECULAR_SOURCE_SHIFT) |
        (R200_LM1_SOURCE_MATERIAL_1 << R200_BACK_EMISSIVE_SOURCE_SHIFT) |
        (R200_LM1_SOURCE_MATERIAL_1 << R200_BACK_AMBIENT_SOURCE_SHIFT) |
        (R200_LM1_SOURCE_MATERIAL_1 << R200_BACK_DIFFUSE_SOURCE_SHIFT) |
        (R200_LM1_SOURCE_MATERIAL_1 << R200_BACK_SPECULAR_SOURCE_SHIFT);
   }

   if (light_model_ctl1 != rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_1]) {
      R200_STATECHANGE( rmesa, tcl );
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_1] = light_model_ctl1;
   }


}

void r200UpdateMaterial( struct gl_context *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat (*mat)[4] = ctx->Light.Material.Attrib;
   GLfloat *fcmd = (GLfloat *)R200_DB_STATE( mtl[0] );
   GLfloat *fcmd2 = (GLfloat *)R200_DB_STATE( mtl[1] );
   GLuint mask = ~0;

   /* Might be possible and faster to update everything unconditionally? */
   if (ctx->Light.ColorMaterialEnabled)
      mask &= ~ctx->Light._ColorMaterialBitmask;

   if (R200_DEBUG & RADEON_STATE)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (mask & MAT_BIT_FRONT_EMISSION) {
      fcmd[MTL_EMMISSIVE_RED]   = mat[MAT_ATTRIB_FRONT_EMISSION][0];
      fcmd[MTL_EMMISSIVE_GREEN] = mat[MAT_ATTRIB_FRONT_EMISSION][1];
      fcmd[MTL_EMMISSIVE_BLUE]  = mat[MAT_ATTRIB_FRONT_EMISSION][2];
      fcmd[MTL_EMMISSIVE_ALPHA] = mat[MAT_ATTRIB_FRONT_EMISSION][3];
   }
   if (mask & MAT_BIT_FRONT_AMBIENT) {
      fcmd[MTL_AMBIENT_RED]     = mat[MAT_ATTRIB_FRONT_AMBIENT][0];
      fcmd[MTL_AMBIENT_GREEN]   = mat[MAT_ATTRIB_FRONT_AMBIENT][1];
      fcmd[MTL_AMBIENT_BLUE]    = mat[MAT_ATTRIB_FRONT_AMBIENT][2];
      fcmd[MTL_AMBIENT_ALPHA]   = mat[MAT_ATTRIB_FRONT_AMBIENT][3];
   }
   if (mask & MAT_BIT_FRONT_DIFFUSE) {
      fcmd[MTL_DIFFUSE_RED]     = mat[MAT_ATTRIB_FRONT_DIFFUSE][0];
      fcmd[MTL_DIFFUSE_GREEN]   = mat[MAT_ATTRIB_FRONT_DIFFUSE][1];
      fcmd[MTL_DIFFUSE_BLUE]    = mat[MAT_ATTRIB_FRONT_DIFFUSE][2];
      fcmd[MTL_DIFFUSE_ALPHA]   = mat[MAT_ATTRIB_FRONT_DIFFUSE][3];
   }
   if (mask & MAT_BIT_FRONT_SPECULAR) {
      fcmd[MTL_SPECULAR_RED]    = mat[MAT_ATTRIB_FRONT_SPECULAR][0];
      fcmd[MTL_SPECULAR_GREEN]  = mat[MAT_ATTRIB_FRONT_SPECULAR][1];
      fcmd[MTL_SPECULAR_BLUE]   = mat[MAT_ATTRIB_FRONT_SPECULAR][2];
      fcmd[MTL_SPECULAR_ALPHA]  = mat[MAT_ATTRIB_FRONT_SPECULAR][3];
   }
   if (mask & MAT_BIT_FRONT_SHININESS) {
      fcmd[MTL_SHININESS]       = mat[MAT_ATTRIB_FRONT_SHININESS][0];
   }

   if (mask & MAT_BIT_BACK_EMISSION) {
      fcmd2[MTL_EMMISSIVE_RED]   = mat[MAT_ATTRIB_BACK_EMISSION][0];
      fcmd2[MTL_EMMISSIVE_GREEN] = mat[MAT_ATTRIB_BACK_EMISSION][1];
      fcmd2[MTL_EMMISSIVE_BLUE]  = mat[MAT_ATTRIB_BACK_EMISSION][2];
      fcmd2[MTL_EMMISSIVE_ALPHA] = mat[MAT_ATTRIB_BACK_EMISSION][3];
   }
   if (mask & MAT_BIT_BACK_AMBIENT) {
      fcmd2[MTL_AMBIENT_RED]     = mat[MAT_ATTRIB_BACK_AMBIENT][0];
      fcmd2[MTL_AMBIENT_GREEN]   = mat[MAT_ATTRIB_BACK_AMBIENT][1];
      fcmd2[MTL_AMBIENT_BLUE]    = mat[MAT_ATTRIB_BACK_AMBIENT][2];
      fcmd2[MTL_AMBIENT_ALPHA]   = mat[MAT_ATTRIB_BACK_AMBIENT][3];
   }
   if (mask & MAT_BIT_BACK_DIFFUSE) {
      fcmd2[MTL_DIFFUSE_RED]     = mat[MAT_ATTRIB_BACK_DIFFUSE][0];
      fcmd2[MTL_DIFFUSE_GREEN]   = mat[MAT_ATTRIB_BACK_DIFFUSE][1];
      fcmd2[MTL_DIFFUSE_BLUE]    = mat[MAT_ATTRIB_BACK_DIFFUSE][2];
      fcmd2[MTL_DIFFUSE_ALPHA]   = mat[MAT_ATTRIB_BACK_DIFFUSE][3];
   }
   if (mask & MAT_BIT_BACK_SPECULAR) {
      fcmd2[MTL_SPECULAR_RED]    = mat[MAT_ATTRIB_BACK_SPECULAR][0];
      fcmd2[MTL_SPECULAR_GREEN]  = mat[MAT_ATTRIB_BACK_SPECULAR][1];
      fcmd2[MTL_SPECULAR_BLUE]   = mat[MAT_ATTRIB_BACK_SPECULAR][2];
      fcmd2[MTL_SPECULAR_ALPHA]  = mat[MAT_ATTRIB_BACK_SPECULAR][3];
   }
   if (mask & MAT_BIT_BACK_SHININESS) {
      fcmd2[MTL_SHININESS]       = mat[MAT_ATTRIB_BACK_SHININESS][0];
   }

   R200_DB_STATECHANGE( rmesa, &rmesa->hw.mtl[0] );
   R200_DB_STATECHANGE( rmesa, &rmesa->hw.mtl[1] );

   /* currently material changes cannot trigger a global ambient change, I believe this is correct
    update_global_ambient( ctx ); */
}

/* _NEW_LIGHT
 * _NEW_MODELVIEW
 * _MESA_NEW_NEED_EYE_COORDS
 *
 * Uses derived state from mesa:
 *       _VP_inf_norm
 *       _h_inf_norm
 *       _Position
 *       _NormSpotDirection
 *       _ModelViewInvScale
 *       _NeedEyeCoords
 *       _EyeZDir
 *
 * which are calculated in light.c and are correct for the current
 * lighting space (model or eye), hence dependencies on _NEW_MODELVIEW
 * and _MESA_NEW_NEED_EYE_COORDS.
 */
static void update_light( struct gl_context *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   /* Have to check these, or have an automatic shortcircuit mechanism
    * to remove noop statechanges. (Or just do a better job on the
    * front end).
    */
   {
      GLuint tmp = rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0];

      if (ctx->_NeedEyeCoords)
	 tmp &= ~R200_LIGHT_IN_MODELSPACE;
      else
	 tmp |= R200_LIGHT_IN_MODELSPACE;

      if (tmp != rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0])
      {
	 R200_STATECHANGE( rmesa, tcl );
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] = tmp;
      }
   }

   {
      GLfloat *fcmd = (GLfloat *)R200_DB_STATE( eye );
      fcmd[EYE_X] = ctx->_EyeZDir[0];
      fcmd[EYE_Y] = ctx->_EyeZDir[1];
      fcmd[EYE_Z] = - ctx->_EyeZDir[2];
      fcmd[EYE_RESCALE_FACTOR] = ctx->_ModelViewInvScale;
      R200_DB_STATECHANGE( rmesa, &rmesa->hw.eye );
   }



   if (ctx->Light.Enabled) {
      GLint p;
      for (p = 0 ; p < MAX_LIGHTS; p++) {
	 if (ctx->Light.Light[p].Enabled) {
	    struct gl_light *l = &ctx->Light.Light[p];
	    GLfloat *fcmd = (GLfloat *)R200_DB_STATE( lit[p] );

	    if (l->EyePosition[3] == 0.0) {
	       COPY_3FV( &fcmd[LIT_POSITION_X], l->_VP_inf_norm );
	       COPY_3FV( &fcmd[LIT_DIRECTION_X], l->_h_inf_norm );
	       fcmd[LIT_POSITION_W] = 0;
	       fcmd[LIT_DIRECTION_W] = 0;
	    } else {
	       COPY_4V( &fcmd[LIT_POSITION_X], l->_Position );
	       fcmd[LIT_DIRECTION_X] = -l->_NormSpotDirection[0];
	       fcmd[LIT_DIRECTION_Y] = -l->_NormSpotDirection[1];
	       fcmd[LIT_DIRECTION_Z] = -l->_NormSpotDirection[2];
	       fcmd[LIT_DIRECTION_W] = 0;
	    }

	    R200_DB_STATECHANGE( rmesa, &rmesa->hw.lit[p] );
	 }
      }
   }
}

static void r200Lightfv( struct gl_context *ctx, GLenum light,
			   GLenum pname, const GLfloat *params )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLint p = light - GL_LIGHT0;
   struct gl_light *l = &ctx->Light.Light[p];
   GLfloat *fcmd = (GLfloat *)rmesa->hw.lit[p].cmd;


   switch (pname) {
   case GL_AMBIENT:
   case GL_DIFFUSE:
   case GL_SPECULAR:
      update_light_colors( ctx, p );
      break;

   case GL_SPOT_DIRECTION:
      /* picked up in update_light */
      break;

   case GL_POSITION: {
      /* positions picked up in update_light, but can do flag here */
      GLuint flag = (p&1)? R200_LIGHT_1_IS_LOCAL : R200_LIGHT_0_IS_LOCAL;
      GLuint idx = TCL_PER_LIGHT_CTL_0 + p/2;

      R200_STATECHANGE(rmesa, tcl);
      if (l->EyePosition[3] != 0.0F)
	 rmesa->hw.tcl.cmd[idx] |= flag;
      else
	 rmesa->hw.tcl.cmd[idx] &= ~flag;
      break;
   }

   case GL_SPOT_EXPONENT:
      R200_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_SPOT_EXPONENT] = params[0];
      break;

   case GL_SPOT_CUTOFF: {
      GLuint flag = (p&1) ? R200_LIGHT_1_IS_SPOT : R200_LIGHT_0_IS_SPOT;
      GLuint idx = TCL_PER_LIGHT_CTL_0 + p/2;

      R200_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_SPOT_CUTOFF] = l->_CosCutoff;

      R200_STATECHANGE(rmesa, tcl);
      if (l->SpotCutoff != 180.0F)
	 rmesa->hw.tcl.cmd[idx] |= flag;
      else
	 rmesa->hw.tcl.cmd[idx] &= ~flag;

      break;
   }

   case GL_CONSTANT_ATTENUATION:
      R200_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_ATTEN_CONST] = params[0];
      if ( params[0] == 0.0 )
	 fcmd[LIT_ATTEN_CONST_INV] = FLT_MAX;
      else
	 fcmd[LIT_ATTEN_CONST_INV] = 1.0 / params[0];
      break;
   case GL_LINEAR_ATTENUATION:
      R200_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_ATTEN_LINEAR] = params[0];
      break;
   case GL_QUADRATIC_ATTENUATION:
      R200_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_ATTEN_QUADRATIC] = params[0];
      break;
   default:
      return;
   }

   /* Set RANGE_ATTEN only when needed */
   switch (pname) {
   case GL_POSITION:
   case GL_CONSTANT_ATTENUATION:
   case GL_LINEAR_ATTENUATION:
   case GL_QUADRATIC_ATTENUATION: {
      GLuint *icmd = (GLuint *)R200_DB_STATE( tcl );
      GLuint idx = TCL_PER_LIGHT_CTL_0 + p/2;
      GLuint atten_flag = ( p&1 ) ? R200_LIGHT_1_ENABLE_RANGE_ATTEN
				  : R200_LIGHT_0_ENABLE_RANGE_ATTEN;
      GLuint atten_const_flag = ( p&1 ) ? R200_LIGHT_1_CONSTANT_RANGE_ATTEN
				  : R200_LIGHT_0_CONSTANT_RANGE_ATTEN;

      if ( l->EyePosition[3] == 0.0F ||
	   ( ( fcmd[LIT_ATTEN_CONST] == 0.0 || fcmd[LIT_ATTEN_CONST] == 1.0 ) &&
	     fcmd[LIT_ATTEN_QUADRATIC] == 0.0 && fcmd[LIT_ATTEN_LINEAR] == 0.0 ) ) {
	 /* Disable attenuation */
	 icmd[idx] &= ~atten_flag;
      } else {
	 if ( fcmd[LIT_ATTEN_QUADRATIC] == 0.0 && fcmd[LIT_ATTEN_LINEAR] == 0.0 ) {
	    /* Enable only constant portion of attenuation calculation */
	    icmd[idx] |= ( atten_flag | atten_const_flag );
	 } else {
	    /* Enable full attenuation calculation */
	    icmd[idx] &= ~atten_const_flag;
	    icmd[idx] |= atten_flag;
	 }
      }

      R200_DB_STATECHANGE( rmesa, &rmesa->hw.tcl );
      break;
   }
   default:
     break;
   }
}

static void r200UpdateLocalViewer ( struct gl_context *ctx )
{
/* It looks like for the texgen modes GL_SPHERE_MAP, GL_NORMAL_MAP and
   GL_REFLECTION_MAP we need R200_LOCAL_VIEWER set (fglrx does exactly that
   for these and only these modes). This means specular highlights may turn out
   wrong in some cases when lighting is enabled but GL_LIGHT_MODEL_LOCAL_VIEWER
   is not set, though it seems to happen rarely and the effect seems quite
   subtle. May need TCL fallback to fix it completely, though I'm not sure
   how you'd identify the cases where the specular highlights indeed will
   be wrong. Don't know if fglrx does something special in that case.
*/
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   R200_STATECHANGE( rmesa, tcl );
   if (ctx->Light.Model.LocalViewer ||
       ctx->Texture._GenFlags & TEXGEN_NEED_NORMALS)
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] |= R200_LOCAL_VIEWER;
   else
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] &= ~R200_LOCAL_VIEWER;
}

static void r200LightModelfv( struct gl_context *ctx, GLenum pname,
				const GLfloat *param )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   switch (pname) {
      case GL_LIGHT_MODEL_AMBIENT:
	 update_global_ambient( ctx );
	 break;

      case GL_LIGHT_MODEL_LOCAL_VIEWER:
	 r200UpdateLocalViewer( ctx );
         break;

      case GL_LIGHT_MODEL_TWO_SIDE:
	 R200_STATECHANGE( rmesa, tcl );
	 if (ctx->Light.Model.TwoSide)
	    rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] |= R200_LIGHT_TWOSIDE;
	 else
	    rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] &= ~(R200_LIGHT_TWOSIDE);
	 if (rmesa->radeon.TclFallback) {
	    r200ChooseRenderState( ctx );
	    r200ChooseVertexState( ctx );
	 }
         break;

      case GL_LIGHT_MODEL_COLOR_CONTROL:
	 r200UpdateSpecular(ctx);
         break;

      default:
         break;
   }
}

static void r200ShadeModel( struct gl_context *ctx, GLenum mode )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint s = rmesa->hw.set.cmd[SET_SE_CNTL];

   s &= ~(R200_DIFFUSE_SHADE_MASK |
	  R200_ALPHA_SHADE_MASK |
	  R200_SPECULAR_SHADE_MASK |
	  R200_FOG_SHADE_MASK |
	  R200_DISC_FOG_SHADE_MASK);

   switch ( mode ) {
   case GL_FLAT:
      s |= (R200_DIFFUSE_SHADE_FLAT |
	    R200_ALPHA_SHADE_FLAT |
	    R200_SPECULAR_SHADE_FLAT |
	    R200_FOG_SHADE_FLAT |
	    R200_DISC_FOG_SHADE_FLAT);
      break;
   case GL_SMOOTH:
      s |= (R200_DIFFUSE_SHADE_GOURAUD |
	    R200_ALPHA_SHADE_GOURAUD |
	    R200_SPECULAR_SHADE_GOURAUD |
	    R200_FOG_SHADE_GOURAUD |
	    R200_DISC_FOG_SHADE_GOURAUD);
      break;
   default:
      return;
   }

   if ( rmesa->hw.set.cmd[SET_SE_CNTL] != s ) {
      R200_STATECHANGE( rmesa, set );
      rmesa->hw.set.cmd[SET_SE_CNTL] = s;
   }
}


/* =============================================================
 * User clip planes
 */

static void r200ClipPlane( struct gl_context *ctx, GLenum plane, const GLfloat *eq )
{
   GLint p = (GLint) plane - (GLint) GL_CLIP_PLANE0;
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLint *ip = (GLint *)ctx->Transform._ClipUserPlane[p];

   R200_STATECHANGE( rmesa, ucp[p] );
   rmesa->hw.ucp[p].cmd[UCP_X] = ip[0];
   rmesa->hw.ucp[p].cmd[UCP_Y] = ip[1];
   rmesa->hw.ucp[p].cmd[UCP_Z] = ip[2];
   rmesa->hw.ucp[p].cmd[UCP_W] = ip[3];
}

static void r200UpdateClipPlanes( struct gl_context *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint p;

   for (p = 0; p < ctx->Const.MaxClipPlanes; p++) {
      if (ctx->Transform.ClipPlanesEnabled & (1 << p)) {
	 GLint *ip = (GLint *)ctx->Transform._ClipUserPlane[p];

	 R200_STATECHANGE( rmesa, ucp[p] );
	 rmesa->hw.ucp[p].cmd[UCP_X] = ip[0];
	 rmesa->hw.ucp[p].cmd[UCP_Y] = ip[1];
	 rmesa->hw.ucp[p].cmd[UCP_Z] = ip[2];
	 rmesa->hw.ucp[p].cmd[UCP_W] = ip[3];
      }
   }
}


/* =============================================================
 * Stencil
 */

static void
r200StencilFuncSeparate( struct gl_context *ctx, GLenum face, GLenum func,
                         GLint ref, GLuint mask )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint refmask = (((ctx->Stencil.Ref[0] & 0xff) << R200_STENCIL_REF_SHIFT) |
		     ((ctx->Stencil.ValueMask[0] & 0xff) << R200_STENCIL_MASK_SHIFT));

   R200_STATECHANGE( rmesa, ctx );
   R200_STATECHANGE( rmesa, msk );

   rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] &= ~R200_STENCIL_TEST_MASK;
   rmesa->hw.msk.cmd[MSK_RB3D_STENCILREFMASK] &= ~(R200_STENCIL_REF_MASK|
						   R200_STENCIL_VALUE_MASK);

   switch ( ctx->Stencil.Function[0] ) {
   case GL_NEVER:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_TEST_NEVER;
      break;
   case GL_LESS:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_TEST_LESS;
      break;
   case GL_EQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_TEST_EQUAL;
      break;
   case GL_LEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_TEST_LEQUAL;
      break;
   case GL_GREATER:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_TEST_GREATER;
      break;
   case GL_NOTEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_TEST_NEQUAL;
      break;
   case GL_GEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_TEST_GEQUAL;
      break;
   case GL_ALWAYS:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_TEST_ALWAYS;
      break;
   }

   rmesa->hw.msk.cmd[MSK_RB3D_STENCILREFMASK] |= refmask;
}

static void
r200StencilMaskSeparate( struct gl_context *ctx, GLenum face, GLuint mask )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   R200_STATECHANGE( rmesa, msk );
   rmesa->hw.msk.cmd[MSK_RB3D_STENCILREFMASK] &= ~R200_STENCIL_WRITE_MASK;
   rmesa->hw.msk.cmd[MSK_RB3D_STENCILREFMASK] |=
      ((ctx->Stencil.WriteMask[0] & 0xff) << R200_STENCIL_WRITEMASK_SHIFT);
}

static void
r200StencilOpSeparate( struct gl_context *ctx, GLenum face, GLenum fail,
                       GLenum zfail, GLenum zpass )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   R200_STATECHANGE( rmesa, ctx );
   rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] &= ~(R200_STENCIL_FAIL_MASK |
					       R200_STENCIL_ZFAIL_MASK |
					       R200_STENCIL_ZPASS_MASK);

   switch ( ctx->Stencil.FailFunc[0] ) {
   case GL_KEEP:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_FAIL_KEEP;
      break;
   case GL_ZERO:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_FAIL_ZERO;
      break;
   case GL_REPLACE:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_FAIL_REPLACE;
      break;
   case GL_INCR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_FAIL_INC;
      break;
   case GL_DECR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_FAIL_DEC;
      break;
   case GL_INCR_WRAP_EXT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_FAIL_INC_WRAP;
      break;
   case GL_DECR_WRAP_EXT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_FAIL_DEC_WRAP;
      break;
   case GL_INVERT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_FAIL_INVERT;
      break;
   }

   switch ( ctx->Stencil.ZFailFunc[0] ) {
   case GL_KEEP:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZFAIL_KEEP;
      break;
   case GL_ZERO:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZFAIL_ZERO;
      break;
   case GL_REPLACE:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZFAIL_REPLACE;
      break;
   case GL_INCR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZFAIL_INC;
      break;
   case GL_DECR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZFAIL_DEC;
      break;
   case GL_INCR_WRAP_EXT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZFAIL_INC_WRAP;
      break;
   case GL_DECR_WRAP_EXT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZFAIL_DEC_WRAP;
      break;
   case GL_INVERT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZFAIL_INVERT;
      break;
   }

   switch ( ctx->Stencil.ZPassFunc[0] ) {
   case GL_KEEP:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZPASS_KEEP;
      break;
   case GL_ZERO:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZPASS_ZERO;
      break;
   case GL_REPLACE:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZPASS_REPLACE;
      break;
   case GL_INCR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZPASS_INC;
      break;
   case GL_DECR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZPASS_DEC;
      break;
   case GL_INCR_WRAP_EXT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZPASS_INC_WRAP;
      break;
   case GL_DECR_WRAP_EXT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZPASS_DEC_WRAP;
      break;
   case GL_INVERT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZPASS_INVERT;
      break;
   }
}


/* =============================================================
 * Window position and viewport transformation
 */

/**
 * Called when window size or position changes or viewport or depth range
 * state is changed.  We update the hardware viewport state here.
 */
void r200UpdateWindow( struct gl_context *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   __DRIdrawable *dPriv = radeon_get_drawable(&rmesa->radeon);
   GLfloat xoffset = 0;
   GLfloat yoffset = dPriv ? (GLfloat) dPriv->h : 0;
   const GLfloat *v = ctx->Viewport._WindowMap.m;
   const GLboolean render_to_fbo = (ctx->DrawBuffer ? _mesa_is_user_fbo(ctx->DrawBuffer) : 0);
   const GLfloat depthScale = 1.0F / ctx->DrawBuffer->_DepthMaxF;
   GLfloat y_scale, y_bias;

   if (render_to_fbo) {
      y_scale = 1.0;
      y_bias = 0;
   } else {
      y_scale = -1.0;
      y_bias = yoffset;
   }

   float_ui32_type sx = { v[MAT_SX] };
   float_ui32_type tx = { v[MAT_TX] + xoffset };
   float_ui32_type sy = { v[MAT_SY] * y_scale };
   float_ui32_type ty = { (v[MAT_TY] * y_scale) + y_bias };
   float_ui32_type sz = { v[MAT_SZ] * depthScale };
   float_ui32_type tz = { v[MAT_TZ] * depthScale };

   R200_STATECHANGE( rmesa, vpt );

   rmesa->hw.vpt.cmd[VPT_SE_VPORT_XSCALE]  = sx.ui32;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_XOFFSET] = tx.ui32;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_YSCALE]  = sy.ui32;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_YOFFSET] = ty.ui32;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_ZSCALE]  = sz.ui32;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_ZOFFSET] = tz.ui32;
}

void r200_vtbl_update_scissor( struct gl_context *ctx )
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   unsigned x1, y1, x2, y2;
   struct radeon_renderbuffer *rrb;

   R200_SET_STATE(r200, set, SET_RE_CNTL, R200_SCISSOR_ENABLE | r200->hw.set.cmd[SET_RE_CNTL]);

   if (r200->radeon.state.scissor.enabled) {
      x1 = r200->radeon.state.scissor.rect.x1;
      y1 = r200->radeon.state.scissor.rect.y1;
      x2 = r200->radeon.state.scissor.rect.x2;
      y2 = r200->radeon.state.scissor.rect.y2;
   } else {
      rrb = radeon_get_colorbuffer(&r200->radeon);
      x1 = 0;
      y1 = 0;
      x2 = rrb->base.Base.Width - 1;
      y2 = rrb->base.Base.Height - 1;
   }

   R200_SET_STATE(r200, sci, SCI_XY_1, x1 | (y1 << 16));
   R200_SET_STATE(r200, sci, SCI_XY_2, x2 | (y2 << 16));
}


static void r200Viewport( struct gl_context *ctx, GLint x, GLint y,
			    GLsizei width, GLsizei height )
{
   /* Don't pipeline viewport changes, conflict with window offset
    * setting below.  Could apply deltas to rescue pipelined viewport
    * values, or keep the originals hanging around.
    */
   r200UpdateWindow( ctx );

   radeon_viewport(ctx, x, y, width, height);
}

static void r200DepthRange( struct gl_context *ctx, GLclampd nearval,
			      GLclampd farval )
{
   r200UpdateWindow( ctx );
}

void r200UpdateViewportOffset( struct gl_context *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   __DRIdrawable *dPriv = radeon_get_drawable(&rmesa->radeon);
   GLfloat xoffset = (GLfloat)0;
   GLfloat yoffset = (GLfloat)dPriv->h;
   const GLfloat *v = ctx->Viewport._WindowMap.m;

   float_ui32_type tx;
   float_ui32_type ty;

   tx.f = v[MAT_TX] + xoffset;
   ty.f = (- v[MAT_TY]) + yoffset;

   if ( rmesa->hw.vpt.cmd[VPT_SE_VPORT_XOFFSET] != tx.ui32 ||
	rmesa->hw.vpt.cmd[VPT_SE_VPORT_YOFFSET] != ty.ui32 )
   {
      /* Note: this should also modify whatever data the context reset
       * code uses...
       */
      R200_STATECHANGE( rmesa, vpt );
      rmesa->hw.vpt.cmd[VPT_SE_VPORT_XOFFSET] = tx.ui32;
      rmesa->hw.vpt.cmd[VPT_SE_VPORT_YOFFSET] = ty.ui32;

      /* update polygon stipple x/y screen offset */
      {
         GLuint stx, sty;
         GLuint m = rmesa->hw.msc.cmd[MSC_RE_MISC];

         m &= ~(R200_STIPPLE_X_OFFSET_MASK |
                R200_STIPPLE_Y_OFFSET_MASK);

         /* add magic offsets, then invert */
         stx = 31 - ((-1) & R200_STIPPLE_COORD_MASK);
         sty = 31 - ((dPriv->h - 1)
                     & R200_STIPPLE_COORD_MASK);

         m |= ((stx << R200_STIPPLE_X_OFFSET_SHIFT) |
               (sty << R200_STIPPLE_Y_OFFSET_SHIFT));

         if ( rmesa->hw.msc.cmd[MSC_RE_MISC] != m ) {
            R200_STATECHANGE( rmesa, msc );
	    rmesa->hw.msc.cmd[MSC_RE_MISC] = m;
         }
      }
   }

   radeonUpdateScissor( ctx );
}



/* =============================================================
 * Miscellaneous
 */

static void r200RenderMode( struct gl_context *ctx, GLenum mode )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   FALLBACK( rmesa, R200_FALLBACK_RENDER_MODE, (mode != GL_RENDER) );
}


static GLuint r200_rop_tab[] = {
   R200_ROP_CLEAR,
   R200_ROP_AND,
   R200_ROP_AND_REVERSE,
   R200_ROP_COPY,
   R200_ROP_AND_INVERTED,
   R200_ROP_NOOP,
   R200_ROP_XOR,
   R200_ROP_OR,
   R200_ROP_NOR,
   R200_ROP_EQUIV,
   R200_ROP_INVERT,
   R200_ROP_OR_REVERSE,
   R200_ROP_COPY_INVERTED,
   R200_ROP_OR_INVERTED,
   R200_ROP_NAND,
   R200_ROP_SET,
};

static void r200LogicOpCode( struct gl_context *ctx, GLenum opcode )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint rop = (GLuint)opcode - GL_CLEAR;

   ASSERT( rop < 16 );

   R200_STATECHANGE( rmesa, msk );
   rmesa->hw.msk.cmd[MSK_RB3D_ROPCNTL] = r200_rop_tab[rop];
}

/* =============================================================
 * State enable/disable
 */

static void r200Enable( struct gl_context *ctx, GLenum cap, GLboolean state )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint p, flag;

   if ( R200_DEBUG & RADEON_STATE )
      fprintf( stderr, "%s( %s = %s )\n", __FUNCTION__,
	       _mesa_lookup_enum_by_nr( cap ),
	       state ? "GL_TRUE" : "GL_FALSE" );

   switch ( cap ) {
      /* Fast track this one...
       */
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
      break;

   case GL_ALPHA_TEST:
      R200_STATECHANGE( rmesa, ctx );
      if (state) {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= R200_ALPHA_TEST_ENABLE;
      } else {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~R200_ALPHA_TEST_ENABLE;
      }
      break;

   case GL_BLEND:
   case GL_COLOR_LOGIC_OP:
      r200_set_blend_state( ctx );
      break;

   case GL_CLIP_PLANE0:
   case GL_CLIP_PLANE1:
   case GL_CLIP_PLANE2:
   case GL_CLIP_PLANE3:
   case GL_CLIP_PLANE4:
   case GL_CLIP_PLANE5:
      p = cap-GL_CLIP_PLANE0;
      R200_STATECHANGE( rmesa, tcl );
      if (state) {
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= (R200_UCP_ENABLE_0<<p);
	 r200ClipPlane( ctx, cap, NULL );
      }
      else {
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~(R200_UCP_ENABLE_0<<p);
      }
      break;

   case GL_COLOR_MATERIAL:
      r200ColorMaterial( ctx, 0, 0 );
      r200UpdateMaterial( ctx );
      break;

   case GL_CULL_FACE:
      r200CullFace( ctx, 0 );
      break;

   case GL_DEPTH_TEST:
      R200_STATECHANGE(rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |=  R200_Z_ENABLE;
      } else {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &= ~R200_Z_ENABLE;
      }
      break;

   case GL_DITHER:
      R200_STATECHANGE(rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |=  R200_DITHER_ENABLE;
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &= ~rmesa->radeon.state.color.roundEnable;
      } else {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &= ~R200_DITHER_ENABLE;
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |=  rmesa->radeon.state.color.roundEnable;
      }
      break;

   case GL_FOG:
      R200_STATECHANGE(rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= R200_FOG_ENABLE;
	 r200Fogfv( ctx, GL_FOG_MODE, NULL );
      } else {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~R200_FOG_ENABLE;
	 R200_STATECHANGE(rmesa, tcl);
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~R200_TCL_FOG_MASK;
      }
      r200UpdateSpecular( ctx ); /* for PK_SPEC */
      if (rmesa->radeon.TclFallback)
	 r200ChooseVertexState( ctx );
      _mesa_allow_light_in_model( ctx, !state );
      break;

   case GL_LIGHT0:
   case GL_LIGHT1:
   case GL_LIGHT2:
   case GL_LIGHT3:
   case GL_LIGHT4:
   case GL_LIGHT5:
   case GL_LIGHT6:
   case GL_LIGHT7:
      R200_STATECHANGE(rmesa, tcl);
      p = cap - GL_LIGHT0;
      if (p&1)
	 flag = (R200_LIGHT_1_ENABLE |
		 R200_LIGHT_1_ENABLE_AMBIENT |
		 R200_LIGHT_1_ENABLE_SPECULAR);
      else
	 flag = (R200_LIGHT_0_ENABLE |
		 R200_LIGHT_0_ENABLE_AMBIENT |
		 R200_LIGHT_0_ENABLE_SPECULAR);

      if (state)
	 rmesa->hw.tcl.cmd[p/2 + TCL_PER_LIGHT_CTL_0] |= flag;
      else
	 rmesa->hw.tcl.cmd[p/2 + TCL_PER_LIGHT_CTL_0] &= ~flag;

      /*
       */
      update_light_colors( ctx, p );
      break;

   case GL_LIGHTING:
      r200UpdateSpecular(ctx);
      /* for reflection map fixup - might set recheck_texgen for all units too */
      rmesa->radeon.NewGLState |= _NEW_TEXTURE;
      break;

   case GL_LINE_SMOOTH:
      R200_STATECHANGE( rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] |=  R200_ANTI_ALIAS_LINE;
      } else {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~R200_ANTI_ALIAS_LINE;
      }
      break;

   case GL_LINE_STIPPLE:
      R200_STATECHANGE( rmesa, set );
      if ( state ) {
	 rmesa->hw.set.cmd[SET_RE_CNTL] |=  R200_PATTERN_ENABLE;
      } else {
	 rmesa->hw.set.cmd[SET_RE_CNTL] &= ~R200_PATTERN_ENABLE;
      }
      break;

   case GL_NORMALIZE:
      R200_STATECHANGE( rmesa, tcl );
      if ( state ) {
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] |=  R200_NORMALIZE_NORMALS;
      } else {
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] &= ~R200_NORMALIZE_NORMALS;
      }
      break;

      /* Pointsize registers on r200 only work for point sprites, and point smooth
       * doesn't work for point sprites (and isn't needed for 1.0 sized aa points).
       * In any case, setting pointmin == pointsizemax == 1.0 for aa points
       * is enough to satisfy conform.
       */
   case GL_POINT_SMOOTH:
      break;

      /* These don't really do anything, as we don't use the 3vtx
       * primitives yet.
       */
#if 0
   case GL_POLYGON_OFFSET_POINT:
      R200_STATECHANGE( rmesa, set );
      if ( state ) {
	 rmesa->hw.set.cmd[SET_SE_CNTL] |=  R200_ZBIAS_ENABLE_POINT;
      } else {
	 rmesa->hw.set.cmd[SET_SE_CNTL] &= ~R200_ZBIAS_ENABLE_POINT;
      }
      break;

   case GL_POLYGON_OFFSET_LINE:
      R200_STATECHANGE( rmesa, set );
      if ( state ) {
	 rmesa->hw.set.cmd[SET_SE_CNTL] |=  R200_ZBIAS_ENABLE_LINE;
      } else {
	 rmesa->hw.set.cmd[SET_SE_CNTL] &= ~R200_ZBIAS_ENABLE_LINE;
      }
      break;
#endif

   case GL_POINT_SPRITE_ARB:
      R200_STATECHANGE( rmesa, spr );
      if ( state ) {
	 int i;
	 for (i = 0; i < 6; i++) {
	    rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] |=
		ctx->Point.CoordReplace[i] << (R200_PS_GEN_TEX_0_SHIFT + i);
	 }
      } else {
	 rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] &= ~R200_PS_GEN_TEX_MASK;
      }
      break;

   case GL_POLYGON_OFFSET_FILL:
      R200_STATECHANGE( rmesa, set );
      if ( state ) {
	 rmesa->hw.set.cmd[SET_SE_CNTL] |=  R200_ZBIAS_ENABLE_TRI;
      } else {
	 rmesa->hw.set.cmd[SET_SE_CNTL] &= ~R200_ZBIAS_ENABLE_TRI;
      }
      break;

   case GL_POLYGON_SMOOTH:
      R200_STATECHANGE( rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] |=  R200_ANTI_ALIAS_POLY;
      } else {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~R200_ANTI_ALIAS_POLY;
      }
      break;

   case GL_POLYGON_STIPPLE:
      R200_STATECHANGE(rmesa, set );
      if ( state ) {
	 rmesa->hw.set.cmd[SET_RE_CNTL] |=  R200_STIPPLE_ENABLE;
      } else {
	 rmesa->hw.set.cmd[SET_RE_CNTL] &= ~R200_STIPPLE_ENABLE;
      }
      break;

   case GL_RESCALE_NORMAL_EXT: {
      GLboolean tmp = ctx->_NeedEyeCoords ? state : !state;
      R200_STATECHANGE( rmesa, tcl );
      if ( tmp ) {
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] |=  R200_RESCALE_NORMALS;
      } else {
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] &= ~R200_RESCALE_NORMALS;
      }
      break;
   }

   case GL_SCISSOR_TEST:
      radeon_firevertices(&rmesa->radeon);
      rmesa->radeon.state.scissor.enabled = state;
      radeonUpdateScissor( ctx );
      break;

   case GL_STENCIL_TEST:
      {
	 GLboolean hw_stencil = GL_FALSE;
	 if (ctx->DrawBuffer) {
	    struct radeon_renderbuffer *rrbStencil
	       = radeon_get_renderbuffer(ctx->DrawBuffer, BUFFER_STENCIL);
	    hw_stencil = (rrbStencil && rrbStencil->bo);
	 }

	 if (hw_stencil) {
	    R200_STATECHANGE( rmesa, ctx );
	    if ( state ) {
	       rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |=  R200_STENCIL_ENABLE;
	    } else {
	       rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &= ~R200_STENCIL_ENABLE;
	    }
	 } else {
	    FALLBACK( rmesa, R200_FALLBACK_STENCIL, state );
	 }
      }
      break;

   case GL_TEXTURE_GEN_Q:
   case GL_TEXTURE_GEN_R:
   case GL_TEXTURE_GEN_S:
   case GL_TEXTURE_GEN_T:
      /* Picked up in r200UpdateTextureState.
       */
      rmesa->recheck_texgen[ctx->Texture.CurrentUnit] = GL_TRUE;
      break;

   case GL_COLOR_SUM_EXT:
      r200UpdateSpecular ( ctx );
      break;

   case GL_VERTEX_PROGRAM_ARB:
      if (!state) {
	 GLuint i;
	 rmesa->curr_vp_hw = NULL;
	 R200_STATECHANGE( rmesa, vap );
	 rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL] &= ~R200_VAP_PROG_VTX_SHADER_ENABLE;
	 /* mark all tcl atoms (tcl vector state got overwritten) dirty
	    not sure about tcl scalar state - we need at least grd
	    with vert progs too.
	    ucp looks like it doesn't get overwritten (may even work
	    with vp for pos-invariant progs if we're lucky) */
	 R200_STATECHANGE( rmesa, mtl[0] );
	 R200_STATECHANGE( rmesa, mtl[1] );
	 R200_STATECHANGE( rmesa, fog );
	 R200_STATECHANGE( rmesa, glt );
	 R200_STATECHANGE( rmesa, eye );
	 for (i = R200_MTX_MV; i <= R200_MTX_TEX5; i++) {
	    R200_STATECHANGE( rmesa, mat[i] );
	 }
	 for (i = 0 ; i < 8; i++) {
	    R200_STATECHANGE( rmesa, lit[i] );
	 }
	 R200_STATECHANGE( rmesa, tcl );
	 for (i = 0; i <= ctx->Const.MaxClipPlanes; i++) {
	    if (ctx->Transform.ClipPlanesEnabled & (1 << i)) {
	       rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= (R200_UCP_ENABLE_0 << i);
	    }
/*	    else {
	       rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~(R200_UCP_ENABLE_0 << i);
	    }*/
	 }
	 /* ugly. Need to call everything which might change compsel. */
	 r200UpdateSpecular( ctx );
#if 0
	/* shouldn't be necessary, as it's picked up anyway in r200ValidateState (_NEW_PROGRAM),
	   but without it doom3 locks up at always the same places. Why? */
	/* FIXME: This can (and should) be replaced by a call to the TCL_STATE_FLUSH reg before
	   accessing VAP_SE_VAP_CNTL. Requires drm changes (done). Remove after some time... */
	 r200UpdateTextureState( ctx );
	 /* if we call r200UpdateTextureState we need the code below because we are calling it with
	    non-current derived enabled values which may revert the state atoms for frag progs even when
	    they already got disabled... ugh
	    Should really figure out why we need to call r200UpdateTextureState in the first place */
	 GLuint unit;
	 for (unit = 0; unit < R200_MAX_TEXTURE_UNITS; unit++) {
	    R200_STATECHANGE( rmesa, pix[unit] );
	    R200_STATECHANGE( rmesa, tex[unit] );
	    rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT] &=
		~(R200_TXFORMAT_ST_ROUTE_MASK | R200_TXFORMAT_LOOKUP_DISABLE);
	    rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT] |= unit << R200_TXFORMAT_ST_ROUTE_SHIFT;
	    /* need to guard this with drmSupportsFragmentShader? Should never get here if
	       we don't announce ATI_fs, right? */
	    rmesa->hw.tex[unit].cmd[TEX_PP_TXMULTI_CTL] = 0;
         }
	 R200_STATECHANGE( rmesa, cst );
	 R200_STATECHANGE( rmesa, tf );
	 rmesa->hw.cst.cmd[CST_PP_CNTL_X] = 0;
#endif
      }
      else {
	 /* picked up later */
      }
      /* call functions which change hw state based on ARB_vp enabled or not. */
      r200PointParameter( ctx, GL_POINT_DISTANCE_ATTENUATION, NULL );
      r200Fogfv( ctx, GL_FOG_COORD_SRC, NULL );
      break;

   case GL_VERTEX_PROGRAM_POINT_SIZE_ARB:
      r200PointParameter( ctx, GL_POINT_DISTANCE_ATTENUATION, NULL );
      break;

   case GL_FRAGMENT_SHADER_ATI:
      if ( !state ) {
	 /* restore normal tex env colors and make sure tex env combine will get updated
	    mark env atoms dirty (as their data was overwritten by afs even
	    if they didn't change) and restore tex coord routing */
	 GLuint unit;
	 for (unit = 0; unit < R200_MAX_TEXTURE_UNITS; unit++) {
	    R200_STATECHANGE( rmesa, pix[unit] );
	    R200_STATECHANGE( rmesa, tex[unit] );
	    rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT] &=
		~(R200_TXFORMAT_ST_ROUTE_MASK | R200_TXFORMAT_LOOKUP_DISABLE);
	    rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT] |= unit << R200_TXFORMAT_ST_ROUTE_SHIFT;
	    rmesa->hw.tex[unit].cmd[TEX_PP_TXMULTI_CTL] = 0;
         }
	 R200_STATECHANGE( rmesa, cst );
	 R200_STATECHANGE( rmesa, tf );
	 rmesa->hw.cst.cmd[CST_PP_CNTL_X] = 0;
      }
      else {
	 /* need to mark this dirty as pix/tf atoms have overwritten the data
	    even if the data in the atoms didn't change */
	 R200_STATECHANGE( rmesa, atf );
	 R200_STATECHANGE( rmesa, afs[1] );
	 /* everything else picked up in r200UpdateTextureState hopefully */
      }
      break;
   default:
      return;
   }
}


void r200LightingSpaceChange( struct gl_context *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLboolean tmp;

   if (R200_DEBUG & RADEON_STATE)
      fprintf(stderr, "%s %d BEFORE %x\n", __FUNCTION__, ctx->_NeedEyeCoords,
	      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0]);

   if (ctx->_NeedEyeCoords)
      tmp = ctx->Transform.RescaleNormals;
   else
      tmp = !ctx->Transform.RescaleNormals;

   R200_STATECHANGE( rmesa, tcl );
   if ( tmp ) {
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] |=  R200_RESCALE_NORMALS;
   } else {
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] &= ~R200_RESCALE_NORMALS;
   }

   if (R200_DEBUG & RADEON_STATE)
      fprintf(stderr, "%s %d AFTER %x\n", __FUNCTION__, ctx->_NeedEyeCoords,
	      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0]);
}

/* =============================================================
 * Deferred state management - matrices, textures, other?
 */




static void upload_matrix( r200ContextPtr rmesa, GLfloat *src, int idx )
{
   float *dest = ((float *)R200_DB_STATE( mat[idx] ))+MAT_ELT_0;
   int i;


   for (i = 0 ; i < 4 ; i++) {
      *dest++ = src[i];
      *dest++ = src[i+4];
      *dest++ = src[i+8];
      *dest++ = src[i+12];
   }

   R200_DB_STATECHANGE( rmesa, &rmesa->hw.mat[idx] );
}

static void upload_matrix_t( r200ContextPtr rmesa, const GLfloat *src, int idx )
{
   float *dest = ((float *)R200_DB_STATE( mat[idx] ))+MAT_ELT_0;
   memcpy(dest, src, 16*sizeof(float));
   R200_DB_STATECHANGE( rmesa, &rmesa->hw.mat[idx] );
}


static void update_texturematrix( struct gl_context *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );
   GLuint tpc = rmesa->hw.tcg.cmd[TCG_TEX_PROC_CTL_0];
   GLuint compsel = rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL];
   int unit;

   if (R200_DEBUG & RADEON_STATE)
      fprintf(stderr, "%s before COMPSEL: %x\n", __FUNCTION__,
	      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL]);

   rmesa->TexMatEnabled = 0;
   rmesa->TexMatCompSel = 0;

   for (unit = 0 ; unit < ctx->Const.MaxTextureUnits; unit++) {
      if (!ctx->Texture.Unit[unit]._ReallyEnabled)
	 continue;

      if (ctx->TextureMatrixStack[unit].Top->type != MATRIX_IDENTITY) {
	 rmesa->TexMatEnabled |= (R200_TEXGEN_TEXMAT_0_ENABLE|
				  R200_TEXMAT_0_ENABLE) << unit;

	 rmesa->TexMatCompSel |= R200_OUTPUT_TEX_0 << unit;

	 if (rmesa->TexGenEnabled & (R200_TEXMAT_0_ENABLE << unit)) {
	    /* Need to preconcatenate any active texgen
	     * obj/eyeplane matrices:
	     */
	    _math_matrix_mul_matrix( &rmesa->tmpmat,
				     ctx->TextureMatrixStack[unit].Top,
				     &rmesa->TexGenMatrix[unit] );
	    upload_matrix( rmesa, rmesa->tmpmat.m, R200_MTX_TEX0+unit );
	 }
	 else {
	    upload_matrix( rmesa, ctx->TextureMatrixStack[unit].Top->m,
			   R200_MTX_TEX0+unit );
	 }
      }
      else if (rmesa->TexGenEnabled & (R200_TEXMAT_0_ENABLE << unit)) {
	 upload_matrix( rmesa, rmesa->TexGenMatrix[unit].m,
			R200_MTX_TEX0+unit );
      }
   }

   tpc = (rmesa->TexMatEnabled | rmesa->TexGenEnabled);
   if (tpc != rmesa->hw.tcg.cmd[TCG_TEX_PROC_CTL_0]) {
      R200_STATECHANGE(rmesa, tcg);
      rmesa->hw.tcg.cmd[TCG_TEX_PROC_CTL_0] = tpc;
   }

   compsel &= ~R200_OUTPUT_TEX_MASK;
   compsel |= rmesa->TexMatCompSel | rmesa->TexGenCompSel;
   if (compsel != rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL]) {
      R200_STATECHANGE(rmesa, vtx);
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] = compsel;
   }
}

static GLboolean r200ValidateBuffers(struct gl_context *ctx)
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   struct radeon_renderbuffer *rrb;
   struct radeon_dma_bo *dma_bo;
   int i, ret;

	if (RADEON_DEBUG & RADEON_IOCTL)
		fprintf(stderr, "%s\n", __FUNCTION__);
   radeon_cs_space_reset_bos(rmesa->radeon.cmdbuf.cs);

   rrb = radeon_get_colorbuffer(&rmesa->radeon);
   /* color buffer */
   if (rrb && rrb->bo) {
     radeon_cs_space_add_persistent_bo(rmesa->radeon.cmdbuf.cs, rrb->bo,
				       0, RADEON_GEM_DOMAIN_VRAM);
   }

   /* depth buffer */
   rrb = radeon_get_depthbuffer(&rmesa->radeon);
   /* color buffer */
   if (rrb && rrb->bo) {
     radeon_cs_space_add_persistent_bo(rmesa->radeon.cmdbuf.cs, rrb->bo,
				       0, RADEON_GEM_DOMAIN_VRAM);
   }

   for (i = 0; i < ctx->Const.MaxTextureImageUnits; ++i) {
      radeonTexObj *t;

      if (!ctx->Texture.Unit[i]._ReallyEnabled)
	 continue;

      t = radeon_tex_obj(ctx->Texture.Unit[i]._Current);
      if (t->image_override && t->bo)
	radeon_cs_space_add_persistent_bo(rmesa->radeon.cmdbuf.cs, t->bo,
			   RADEON_GEM_DOMAIN_GTT | RADEON_GEM_DOMAIN_VRAM, 0);
      else if (t->mt->bo)
	radeon_cs_space_add_persistent_bo(rmesa->radeon.cmdbuf.cs, t->mt->bo,
			   RADEON_GEM_DOMAIN_GTT | RADEON_GEM_DOMAIN_VRAM, 0);
   }

   dma_bo = first_elem(&rmesa->radeon.dma.reserved);
   {
       ret = radeon_cs_space_check_with_bo(rmesa->radeon.cmdbuf.cs, dma_bo->bo, RADEON_GEM_DOMAIN_GTT, 0);
       if (ret)
	   return GL_FALSE;
   }
   return GL_TRUE;
}

GLboolean r200ValidateState( struct gl_context *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint new_state = rmesa->radeon.NewGLState;

   if (new_state & _NEW_BUFFERS) {
      _mesa_update_framebuffer(ctx);
      /* this updates the DrawBuffer's Width/Height if it's a FBO */
      _mesa_update_draw_buffer_bounds(ctx);

      R200_STATECHANGE(rmesa, ctx);
   }

   if (new_state & (_NEW_TEXTURE | _NEW_PROGRAM | _NEW_PROGRAM_CONSTANTS)) {
      r200UpdateTextureState( ctx );
      new_state |= rmesa->radeon.NewGLState; /* may add TEXTURE_MATRIX */
      r200UpdateLocalViewer( ctx );
   }

   /* we need to do a space check here */
   if (!r200ValidateBuffers(ctx))
     return GL_FALSE;

/* FIXME: don't really need most of these when vertex progs are enabled */

   /* Need an event driven matrix update?
    */
   if (new_state & (_NEW_MODELVIEW|_NEW_PROJECTION))
      upload_matrix( rmesa, ctx->_ModelProjectMatrix.m, R200_MTX_MVP );

   /* Need these for lighting (shouldn't upload otherwise)
    */
   if (new_state & (_NEW_MODELVIEW)) {
      upload_matrix( rmesa, ctx->ModelviewMatrixStack.Top->m, R200_MTX_MV );
      upload_matrix_t( rmesa, ctx->ModelviewMatrixStack.Top->inv, R200_MTX_IMV );
   }

   /* Does this need to be triggered on eg. modelview for
    * texgen-derived objplane/eyeplane matrices?
    */
   if (new_state & (_NEW_TEXTURE|_NEW_TEXTURE_MATRIX)) {
      update_texturematrix( ctx );
   }

   if (new_state & (_NEW_LIGHT|_NEW_MODELVIEW|_MESA_NEW_NEED_EYE_COORDS)) {
      update_light( ctx );
   }

   /* emit all active clip planes if projection matrix changes.
    */
   if (new_state & (_NEW_PROJECTION)) {
      if (ctx->Transform.ClipPlanesEnabled)
	 r200UpdateClipPlanes( ctx );
   }

   if (new_state & (_NEW_PROGRAM|
                    _NEW_PROGRAM_CONSTANTS |
   /* need to test for pretty much anything due to possible parameter bindings */
	_NEW_MODELVIEW|_NEW_PROJECTION|_NEW_TRANSFORM|
	_NEW_LIGHT|_NEW_TEXTURE|_NEW_TEXTURE_MATRIX|
	_NEW_FOG|_NEW_POINT|_NEW_TRACK_MATRIX)) {
      if (ctx->VertexProgram._Enabled) {
	 r200SetupVertexProg( ctx );
      }
      else TCL_FALLBACK(ctx, R200_TCL_FALLBACK_VERTEX_PROGRAM, 0);
   }

   rmesa->radeon.NewGLState = 0;
   return GL_TRUE;
}


static void r200InvalidateState( struct gl_context *ctx, GLuint new_state )
{
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   _ae_invalidate_state( ctx, new_state );
   R200_CONTEXT(ctx)->radeon.NewGLState |= new_state;
}

/* A hack.  The r200 can actually cope just fine with materials
 * between begin/ends, so fix this.
 * Should map to inputs just like the generic vertex arrays for vertex progs.
 * In theory there could still be too many and we'd still need a fallback.
 */
static GLboolean check_material( struct gl_context *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLint i;

   for (i = _TNL_ATTRIB_MAT_FRONT_AMBIENT;
	i < _TNL_ATTRIB_MAT_BACK_INDEXES;
	i++)
      if (tnl->vb.AttribPtr[i] &&
	  tnl->vb.AttribPtr[i]->stride)
	 return GL_TRUE;

   return GL_FALSE;
}

static void r200WrapRunPipeline( struct gl_context *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLboolean has_material;

   if (0)
      fprintf(stderr, "%s, newstate: %x\n", __FUNCTION__, rmesa->radeon.NewGLState);

   /* Validate state:
    */
   if (rmesa->radeon.NewGLState)
      if (!r200ValidateState( ctx ))
	 FALLBACK(rmesa, RADEON_FALLBACK_TEXTURE, GL_TRUE);

   has_material = !ctx->VertexProgram._Enabled && ctx->Light.Enabled && check_material( ctx );

   if (has_material) {
      TCL_FALLBACK( ctx, R200_TCL_FALLBACK_MATERIAL, GL_TRUE );
   }

   /* Run the pipeline.
    */
   _tnl_run_pipeline( ctx );

   if (has_material) {
      TCL_FALLBACK( ctx, R200_TCL_FALLBACK_MATERIAL, GL_FALSE );
   }
}


static void r200PolygonStipple( struct gl_context *ctx, const GLubyte *mask )
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   GLint i;

   radeon_firevertices(&r200->radeon);

   radeon_print(RADEON_STATE, RADEON_TRACE,
		   "%s(%p) first 32 bits are %x.\n",
		   __func__,
		   ctx,
		   *(uint32_t*)mask);

   R200_STATECHANGE(r200, stp);

   /* Must flip pattern upside down.
    */
   for ( i = 31 ; i >= 0; i--) {
     r200->hw.stp.cmd[3 + i] = ((GLuint *) mask)[i];
   }
}
/* Initialize the driver's state functions.
 */
void r200InitStateFuncs( radeonContextPtr radeon, struct dd_function_table *functions )
{
   functions->UpdateState		= r200InvalidateState;
   functions->LightingSpaceChange	= r200LightingSpaceChange;

   functions->DrawBuffer		= radeonDrawBuffer;
   functions->ReadBuffer		= radeonReadBuffer;

   functions->CopyPixels                = _mesa_meta_CopyPixels;
   functions->DrawPixels                = _mesa_meta_DrawPixels;
   functions->ReadPixels                = radeonReadPixels;

   functions->AlphaFunc			= r200AlphaFunc;
   functions->BlendColor		= r200BlendColor;
   functions->BlendEquationSeparate	= r200BlendEquationSeparate;
   functions->BlendFuncSeparate		= r200BlendFuncSeparate;
   functions->ClipPlane			= r200ClipPlane;
   functions->ColorMask			= r200ColorMask;
   functions->CullFace			= r200CullFace;
   functions->DepthFunc			= r200DepthFunc;
   functions->DepthMask			= r200DepthMask;
   functions->DepthRange		= r200DepthRange;
   functions->Enable			= r200Enable;
   functions->Fogfv			= r200Fogfv;
   functions->FrontFace			= r200FrontFace;
   functions->Hint			= NULL;
   functions->LightModelfv		= r200LightModelfv;
   functions->Lightfv			= r200Lightfv;
   functions->LineStipple		= r200LineStipple;
   functions->LineWidth			= r200LineWidth;
   functions->LogicOpcode		= r200LogicOpCode;
   functions->PolygonMode		= r200PolygonMode;
   functions->PolygonOffset		= r200PolygonOffset;
   functions->PolygonStipple		= r200PolygonStipple;
   functions->PointParameterfv		= r200PointParameter;
   functions->PointSize			= r200PointSize;
   functions->RenderMode		= r200RenderMode;
   functions->Scissor			= radeonScissor;
   functions->ShadeModel		= r200ShadeModel;
   functions->StencilFuncSeparate	= r200StencilFuncSeparate;
   functions->StencilMaskSeparate	= r200StencilMaskSeparate;
   functions->StencilOpSeparate		= r200StencilOpSeparate;
   functions->Viewport			= r200Viewport;
}


void r200InitTnlFuncs( struct gl_context *ctx )
{
   TNL_CONTEXT(ctx)->Driver.NotifyMaterialChange = r200UpdateMaterial;
   TNL_CONTEXT(ctx)->Driver.RunPipeline = r200WrapRunPipeline;
}
