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
*/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/imports.h"
#include "main/enums.h"
#include "main/colormac.h"
#include "main/api_arrayelt.h"

#include "swrast/swrast.h"
#include "vbo/vbo.h"
#include "tnl/t_pipeline.h"
#include "swrast_setup/swrast_setup.h"

#include "radeon_common.h"
#include "radeon_mipmap_tree.h"
#include "r200_context.h"
#include "r200_ioctl.h"
#include "r200_state.h"
#include "radeon_queryobj.h"

#include "xmlpool.h"

/* New (1.3) state mechanism.  3 commands (packet, scalar, vector) in
 * 1.3 cmdbuffers allow all previous state to be updated as well as
 * the tcl scalar and vector areas.
 */
static struct {
	int start;
	int len;
	const char *name;
} packet[RADEON_MAX_STATE_PACKETS] = {
	{RADEON_PP_MISC, 7, "RADEON_PP_MISC"},
	{RADEON_PP_CNTL, 3, "RADEON_PP_CNTL"},
	{RADEON_RB3D_COLORPITCH, 1, "RADEON_RB3D_COLORPITCH"},
	{RADEON_RE_LINE_PATTERN, 2, "RADEON_RE_LINE_PATTERN"},
	{RADEON_SE_LINE_WIDTH, 1, "RADEON_SE_LINE_WIDTH"},
	{RADEON_PP_LUM_MATRIX, 1, "RADEON_PP_LUM_MATRIX"},
	{RADEON_PP_ROT_MATRIX_0, 2, "RADEON_PP_ROT_MATRIX_0"},
	{RADEON_RB3D_STENCILREFMASK, 3, "RADEON_RB3D_STENCILREFMASK"},
	{RADEON_SE_VPORT_XSCALE, 6, "RADEON_SE_VPORT_XSCALE"},
	{RADEON_SE_CNTL, 2, "RADEON_SE_CNTL"},
	{RADEON_SE_CNTL_STATUS, 1, "RADEON_SE_CNTL_STATUS"},
	{RADEON_RE_MISC, 1, "RADEON_RE_MISC"},
	{RADEON_PP_TXFILTER_0, 6, "RADEON_PP_TXFILTER_0"},
	{RADEON_PP_BORDER_COLOR_0, 1, "RADEON_PP_BORDER_COLOR_0"},
	{RADEON_PP_TXFILTER_1, 6, "RADEON_PP_TXFILTER_1"},
	{RADEON_PP_BORDER_COLOR_1, 1, "RADEON_PP_BORDER_COLOR_1"},
	{RADEON_PP_TXFILTER_2, 6, "RADEON_PP_TXFILTER_2"},
	{RADEON_PP_BORDER_COLOR_2, 1, "RADEON_PP_BORDER_COLOR_2"},
	{RADEON_SE_ZBIAS_FACTOR, 2, "RADEON_SE_ZBIAS_FACTOR"},
	{RADEON_SE_TCL_OUTPUT_VTX_FMT, 11, "RADEON_SE_TCL_OUTPUT_VTX_FMT"},
	{RADEON_SE_TCL_MATERIAL_EMMISSIVE_RED, 17,
		    "RADEON_SE_TCL_MATERIAL_EMMISSIVE_RED"},
	{R200_PP_TXCBLEND_0, 4, "R200_PP_TXCBLEND_0"},
	{R200_PP_TXCBLEND_1, 4, "R200_PP_TXCBLEND_1"},
	{R200_PP_TXCBLEND_2, 4, "R200_PP_TXCBLEND_2"},
	{R200_PP_TXCBLEND_3, 4, "R200_PP_TXCBLEND_3"},
	{R200_PP_TXCBLEND_4, 4, "R200_PP_TXCBLEND_4"},
	{R200_PP_TXCBLEND_5, 4, "R200_PP_TXCBLEND_5"},
	{R200_PP_TXCBLEND_6, 4, "R200_PP_TXCBLEND_6"},
	{R200_PP_TXCBLEND_7, 4, "R200_PP_TXCBLEND_7"},
	{R200_SE_TCL_LIGHT_MODEL_CTL_0, 6, "R200_SE_TCL_LIGHT_MODEL_CTL_0"},
	{R200_PP_TFACTOR_0, 6, "R200_PP_TFACTOR_0"},
	{R200_SE_VTX_FMT_0, 4, "R200_SE_VTX_FMT_0"},
	{R200_SE_VAP_CNTL, 1, "R200_SE_VAP_CNTL"},
	{R200_SE_TCL_MATRIX_SEL_0, 5, "R200_SE_TCL_MATRIX_SEL_0"},
	{R200_SE_TCL_TEX_PROC_CTL_2, 5, "R200_SE_TCL_TEX_PROC_CTL_2"},
	{R200_SE_TCL_UCP_VERT_BLEND_CTL, 1, "R200_SE_TCL_UCP_VERT_BLEND_CTL"},
	{R200_PP_TXFILTER_0, 6, "R200_PP_TXFILTER_0"},
	{R200_PP_TXFILTER_1, 6, "R200_PP_TXFILTER_1"},
	{R200_PP_TXFILTER_2, 6, "R200_PP_TXFILTER_2"},
	{R200_PP_TXFILTER_3, 6, "R200_PP_TXFILTER_3"},
	{R200_PP_TXFILTER_4, 6, "R200_PP_TXFILTER_4"},
	{R200_PP_TXFILTER_5, 6, "R200_PP_TXFILTER_5"},
	{R200_PP_TXOFFSET_0, 1, "R200_PP_TXOFFSET_0"},
	{R200_PP_TXOFFSET_1, 1, "R200_PP_TXOFFSET_1"},
	{R200_PP_TXOFFSET_2, 1, "R200_PP_TXOFFSET_2"},
	{R200_PP_TXOFFSET_3, 1, "R200_PP_TXOFFSET_3"},
	{R200_PP_TXOFFSET_4, 1, "R200_PP_TXOFFSET_4"},
	{R200_PP_TXOFFSET_5, 1, "R200_PP_TXOFFSET_5"},
	{R200_SE_VTE_CNTL, 1, "R200_SE_VTE_CNTL"},
	{R200_SE_TCL_OUTPUT_VTX_COMP_SEL, 1,
	 "R200_SE_TCL_OUTPUT_VTX_COMP_SEL"},
	{R200_PP_TAM_DEBUG3, 1, "R200_PP_TAM_DEBUG3"},
	{R200_PP_CNTL_X, 1, "R200_PP_CNTL_X"},
	{R200_RB3D_DEPTHXY_OFFSET, 1, "R200_RB3D_DEPTHXY_OFFSET"},
	{R200_RE_AUX_SCISSOR_CNTL, 1, "R200_RE_AUX_SCISSOR_CNTL"},
	{R200_RE_SCISSOR_TL_0, 2, "R200_RE_SCISSOR_TL_0"},
	{R200_RE_SCISSOR_TL_1, 2, "R200_RE_SCISSOR_TL_1"},
	{R200_RE_SCISSOR_TL_2, 2, "R200_RE_SCISSOR_TL_2"},
	{R200_SE_VAP_CNTL_STATUS, 1, "R200_SE_VAP_CNTL_STATUS"},
	{R200_SE_VTX_STATE_CNTL, 1, "R200_SE_VTX_STATE_CNTL"},
	{R200_RE_POINTSIZE, 1, "R200_RE_POINTSIZE"},
	{R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_0, 4,
		    "R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_0"},
	{R200_PP_CUBIC_FACES_0, 1, "R200_PP_CUBIC_FACES_0"},	/* 61 */
	{R200_PP_CUBIC_OFFSET_F1_0, 5, "R200_PP_CUBIC_OFFSET_F1_0"}, /* 62 */
	{R200_PP_CUBIC_FACES_1, 1, "R200_PP_CUBIC_FACES_1"},
	{R200_PP_CUBIC_OFFSET_F1_1, 5, "R200_PP_CUBIC_OFFSET_F1_1"},
	{R200_PP_CUBIC_FACES_2, 1, "R200_PP_CUBIC_FACES_2"},
	{R200_PP_CUBIC_OFFSET_F1_2, 5, "R200_PP_CUBIC_OFFSET_F1_2"},
	{R200_PP_CUBIC_FACES_3, 1, "R200_PP_CUBIC_FACES_3"},
	{R200_PP_CUBIC_OFFSET_F1_3, 5, "R200_PP_CUBIC_OFFSET_F1_3"},
	{R200_PP_CUBIC_FACES_4, 1, "R200_PP_CUBIC_FACES_4"},
	{R200_PP_CUBIC_OFFSET_F1_4, 5, "R200_PP_CUBIC_OFFSET_F1_4"},
	{R200_PP_CUBIC_FACES_5, 1, "R200_PP_CUBIC_FACES_5"},
	{R200_PP_CUBIC_OFFSET_F1_5, 5, "R200_PP_CUBIC_OFFSET_F1_5"},
	{RADEON_PP_TEX_SIZE_0, 2, "RADEON_PP_TEX_SIZE_0"},
	{RADEON_PP_TEX_SIZE_1, 2, "RADEON_PP_TEX_SIZE_1"},
	{RADEON_PP_TEX_SIZE_2, 2, "RADEON_PP_TEX_SIZE_2"},
	{R200_RB3D_BLENDCOLOR, 3, "R200_RB3D_BLENDCOLOR"},
	{R200_SE_TCL_POINT_SPRITE_CNTL, 1, "R200_SE_TCL_POINT_SPRITE_CNTL"},
	{RADEON_PP_CUBIC_FACES_0, 1, "RADEON_PP_CUBIC_FACES_0"},
	{RADEON_PP_CUBIC_OFFSET_T0_0, 5, "RADEON_PP_CUBIC_OFFSET_T0_0"},
	{RADEON_PP_CUBIC_FACES_1, 1, "RADEON_PP_CUBIC_FACES_1"},
	{RADEON_PP_CUBIC_OFFSET_T1_0, 5, "RADEON_PP_CUBIC_OFFSET_T1_0"},
	{RADEON_PP_CUBIC_FACES_2, 1, "RADEON_PP_CUBIC_FACES_2"},
	{RADEON_PP_CUBIC_OFFSET_T2_0, 5, "RADEON_PP_CUBIC_OFFSET_T2_0"},
	{R200_PP_TRI_PERF, 2, "R200_PP_TRI_PERF"},
	{R200_PP_TXCBLEND_8, 32, "R200_PP_AFS_0"},     /* 85 */
	{R200_PP_TXCBLEND_0, 32, "R200_PP_AFS_1"},
	{R200_PP_TFACTOR_0, 8, "R200_ATF_TFACTOR"},
	{R200_PP_TXFILTER_0, 8, "R200_PP_TXCTLALL_0"},
	{R200_PP_TXFILTER_1, 8, "R200_PP_TXCTLALL_1"},
	{R200_PP_TXFILTER_2, 8, "R200_PP_TXCTLALL_2"},
	{R200_PP_TXFILTER_3, 8, "R200_PP_TXCTLALL_3"},
	{R200_PP_TXFILTER_4, 8, "R200_PP_TXCTLALL_4"},
	{R200_PP_TXFILTER_5, 8, "R200_PP_TXCTLALL_5"},
	{R200_VAP_PVS_CNTL_1, 2, "R200_VAP_PVS_CNTL"},
};

/* =============================================================
 * State initialization
 */
static int cmdpkt( r200ContextPtr rmesa, int id ) 
{
   return CP_PACKET0(packet[id].start, packet[id].len - 1);
}

static int cmdvec( int offset, int stride, int count ) 
{
   drm_radeon_cmd_header_t h;
   h.i = 0;
   h.vectors.cmd_type = RADEON_CMD_VECTORS;
   h.vectors.offset = offset;
   h.vectors.stride = stride;
   h.vectors.count = count;
   return h.i;
}

/* warning: the count here is divided by 4 compared to other cmds
   (so it doesn't exceed the char size)! */
static int cmdveclinear( int offset, int count ) 
{
   drm_radeon_cmd_header_t h;
   h.i = 0;
   h.veclinear.cmd_type = RADEON_CMD_VECLINEAR;
   h.veclinear.addr_lo = offset & 0xff;
   h.veclinear.addr_hi = (offset & 0xff00) >> 8;
   h.veclinear.count = count;
   return h.i;
}

static int cmdscl( int offset, int stride, int count ) 
{
   drm_radeon_cmd_header_t h;
   h.i = 0;
   h.scalars.cmd_type = RADEON_CMD_SCALARS;
   h.scalars.offset = offset;
   h.scalars.stride = stride;
   h.scalars.count = count;
   return h.i;
}

static int cmdscl2( int offset, int stride, int count ) 
{
   drm_radeon_cmd_header_t h;
   h.i = 0;
   h.scalars.cmd_type = RADEON_CMD_SCALARS2;
   h.scalars.offset = offset - 0x100;
   h.scalars.stride = stride;
   h.scalars.count = count;
   return h.i;
}

/**
 * Check functions are used to check if state is active.
 * If it is active check function returns maximum emit size.
 */
#define CHECK( NM, FLAG, ADD )				\
static int check_##NM( struct gl_context *ctx, struct radeon_state_atom *atom) \
{							\
   r200ContextPtr rmesa = R200_CONTEXT(ctx);		\
   (void) rmesa;					\
   return (FLAG) ? atom->cmd_size + (ADD) : 0;			\
}

#define TCL_CHECK( NM, FLAG, ADD )				\
static int check_##NM( struct gl_context *ctx, struct radeon_state_atom *atom) \
{									\
   r200ContextPtr rmesa = R200_CONTEXT(ctx);				\
   return (!rmesa->radeon.TclFallback && !ctx->VertexProgram._Enabled && (FLAG)) ? atom->cmd_size + (ADD) : 0; \
}

#define TCL_OR_VP_CHECK( NM, FLAG, ADD )			\
static int check_##NM( struct gl_context *ctx, struct radeon_state_atom *atom ) \
{							\
   r200ContextPtr rmesa = R200_CONTEXT(ctx);		\
   return (!rmesa->radeon.TclFallback && (FLAG)) ? atom->cmd_size + (ADD) : 0;	\
}

#define VP_CHECK( NM, FLAG, ADD )				\
static int check_##NM( struct gl_context *ctx, struct radeon_state_atom *atom ) \
{									\
   r200ContextPtr rmesa = R200_CONTEXT(ctx);				\
   (void) atom;								\
   return (!rmesa->radeon.TclFallback && ctx->VertexProgram._Enabled && (FLAG)) ? atom->cmd_size + (ADD) : 0; \
}

CHECK( always, GL_TRUE, 0 )
CHECK( always_add4, GL_TRUE, 4 )
CHECK( never, GL_FALSE, 0 )
CHECK( tex_any, ctx->Texture._EnabledUnits, 0 )
CHECK( tf, (ctx->Texture._EnabledUnits && !ctx->ATIFragmentShader._Enabled), 0 );
CHECK( pix_zero, !ctx->ATIFragmentShader._Enabled, 0 )
   CHECK( texenv, (rmesa->state.envneeded & (1 << (atom->idx)) && !ctx->ATIFragmentShader._Enabled), 0 )
CHECK( afs_pass1, (ctx->ATIFragmentShader._Enabled && (ctx->ATIFragmentShader.Current->NumPasses > 1)), 0 )
CHECK( afs, ctx->ATIFragmentShader._Enabled, 0 )
CHECK( tex_cube, rmesa->state.texture.unit[atom->idx].unitneeded & TEXTURE_CUBE_BIT, 3 + 3*5 - CUBE_STATE_SIZE )
CHECK( tex_cube_cs, rmesa->state.texture.unit[atom->idx].unitneeded & TEXTURE_CUBE_BIT, 2 + 4*5 - CUBE_STATE_SIZE )
TCL_CHECK( tcl_fog_add4, ctx->Fog.Enabled, 4 )
TCL_CHECK( tcl, GL_TRUE, 0 )
TCL_CHECK( tcl_add8, GL_TRUE, 8 )
TCL_CHECK( tcl_add4, GL_TRUE, 4 )
TCL_CHECK( tcl_tex_add4, rmesa->state.texture.unit[atom->idx].unitneeded, 4 )
TCL_CHECK( tcl_lighting_add4, ctx->Light.Enabled, 4 )
TCL_CHECK( tcl_lighting_add6, ctx->Light.Enabled, 6 )
TCL_CHECK( tcl_light_add6, ctx->Light.Enabled && ctx->Light.Light[atom->idx].Enabled, 6 )
TCL_OR_VP_CHECK( tcl_ucp_add4, (ctx->Transform.ClipPlanesEnabled & (1 << (atom->idx))), 4 )
TCL_OR_VP_CHECK( tcl_or_vp, GL_TRUE, 0 )
TCL_OR_VP_CHECK( tcl_or_vp_add2, GL_TRUE, 2 )
VP_CHECK( tcl_vp, GL_TRUE, 0 )
VP_CHECK( tcl_vp_add4, GL_TRUE, 4 )
VP_CHECK( tcl_vp_size_add4, ctx->VertexProgram.Current->Base.NumNativeInstructions > 64, 4 )
VP_CHECK( tcl_vpp_size_add4, ctx->VertexProgram.Current->Base.NumNativeParameters > 96, 4 )

#define OUT_VEC(hdr, data) do {			\
    drm_radeon_cmd_header_t h;					\
    h.i = hdr;								\
    OUT_BATCH(CP_PACKET0(RADEON_SE_TCL_STATE_FLUSH, 0));		\
    OUT_BATCH(0);							\
    OUT_BATCH(CP_PACKET0(R200_SE_TCL_VECTOR_INDX_REG, 0));		\
    OUT_BATCH(h.vectors.offset | (h.vectors.stride << RADEON_VEC_INDX_OCTWORD_STRIDE_SHIFT)); \
    OUT_BATCH(CP_PACKET0_ONE(R200_SE_TCL_VECTOR_DATA_REG, h.vectors.count - 1));	\
    OUT_BATCH_TABLE((data), h.vectors.count);				\
  } while(0)

#define OUT_VECLINEAR(hdr, data) do {					\
    drm_radeon_cmd_header_t h;						\
    uint32_t _start, _sz;						\
    h.i = hdr;								\
    _start = h.veclinear.addr_lo | (h.veclinear.addr_hi << 8);		\
    _sz = h.veclinear.count * 4;					\
    if (_sz) {								\
    BEGIN_BATCH_NO_AUTOSTATE(dwords); \
    OUT_BATCH(CP_PACKET0(RADEON_SE_TCL_STATE_FLUSH, 0));		\
    OUT_BATCH(0);							\
    OUT_BATCH(CP_PACKET0(R200_SE_TCL_VECTOR_INDX_REG, 0));		\
    OUT_BATCH(_start | (1 << RADEON_VEC_INDX_OCTWORD_STRIDE_SHIFT));	\
    OUT_BATCH(CP_PACKET0_ONE(R200_SE_TCL_VECTOR_DATA_REG, _sz - 1));	\
    OUT_BATCH_TABLE((data), _sz);					\
    END_BATCH(); \
    } \
  } while(0)

#define OUT_SCL(hdr, data) do {					\
    drm_radeon_cmd_header_t h;						\
    h.i = hdr;								\
    OUT_BATCH(CP_PACKET0(R200_SE_TCL_SCALAR_INDX_REG, 0));		\
    OUT_BATCH((h.scalars.offset) | (h.scalars.stride << RADEON_SCAL_INDX_DWORD_STRIDE_SHIFT)); \
    OUT_BATCH(CP_PACKET0_ONE(R200_SE_TCL_SCALAR_DATA_REG, h.scalars.count - 1));	\
    OUT_BATCH_TABLE((data), h.scalars.count);				\
  } while(0)

#define OUT_SCL2(hdr, data) do {					\
    drm_radeon_cmd_header_t h;						\
    h.i = hdr;								\
    OUT_BATCH(CP_PACKET0(R200_SE_TCL_SCALAR_INDX_REG, 0));		\
    OUT_BATCH((h.scalars.offset + 0x100) | (h.scalars.stride << RADEON_SCAL_INDX_DWORD_STRIDE_SHIFT)); \
    OUT_BATCH(CP_PACKET0_ONE(R200_SE_TCL_SCALAR_DATA_REG, h.scalars.count - 1));	\
    OUT_BATCH_TABLE((data), h.scalars.count);				\
  } while(0)
static int check_rrb(struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   struct radeon_renderbuffer *rrb;
   rrb = radeon_get_colorbuffer(&r200->radeon);
   if (!rrb || !rrb->bo)
      return 0;
   return atom->cmd_size;
}

static int check_polygon_stipple(struct gl_context *ctx,
		struct radeon_state_atom *atom)
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   if (r200->hw.set.cmd[SET_RE_CNTL] & R200_STIPPLE_ENABLE)
	   return atom->cmd_size;
   return 0;
}

static void mtl_emit(struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   BATCH_LOCALS(&r200->radeon);
   uint32_t dwords = atom->check(ctx, atom);

   BEGIN_BATCH_NO_AUTOSTATE(dwords);
   OUT_VEC(atom->cmd[MTL_CMD_0], (atom->cmd+1));
   OUT_SCL2(atom->cmd[MTL_CMD_1], (atom->cmd + 18));
   END_BATCH();
}

static void lit_emit(struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   BATCH_LOCALS(&r200->radeon);
   uint32_t dwords = atom->check(ctx, atom);

   BEGIN_BATCH_NO_AUTOSTATE(dwords);
   OUT_VEC(atom->cmd[LIT_CMD_0], atom->cmd+1);
   OUT_SCL(atom->cmd[LIT_CMD_1], atom->cmd+LIT_CMD_1+1);
   END_BATCH();
}

static void ptp_emit(struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   BATCH_LOCALS(&r200->radeon);
   uint32_t dwords = atom->check(ctx, atom);

   BEGIN_BATCH_NO_AUTOSTATE(dwords);
   OUT_VEC(atom->cmd[PTP_CMD_0], atom->cmd+1);
   OUT_VEC(atom->cmd[PTP_CMD_1], atom->cmd+PTP_CMD_1+1);
   END_BATCH();
}

static void veclinear_emit(struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   BATCH_LOCALS(&r200->radeon);
   uint32_t dwords = atom->check(ctx, atom);

   OUT_VECLINEAR(atom->cmd[0], atom->cmd+1);
}

static void scl_emit(struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   BATCH_LOCALS(&r200->radeon);
   uint32_t dwords = atom->check(ctx, atom);

   BEGIN_BATCH_NO_AUTOSTATE(dwords);
   OUT_SCL(atom->cmd[0], atom->cmd+1);
   END_BATCH();
}


static void vec_emit(struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   BATCH_LOCALS(&r200->radeon);
   uint32_t dwords = atom->check(ctx, atom);

   BEGIN_BATCH_NO_AUTOSTATE(dwords);
   OUT_VEC(atom->cmd[0], atom->cmd+1);
   END_BATCH();
}

static int check_always_ctx( struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   struct radeon_renderbuffer *rrb, *drb;
   uint32_t dwords;

   rrb = radeon_get_colorbuffer(&r200->radeon);
   if (!rrb || !rrb->bo) {
      return 0;
   }

   drb = radeon_get_depthbuffer(&r200->radeon);

   dwords = 10;
   if (drb)
     dwords += 6;
   if (rrb)
     dwords += 8;
   if (atom->cmd_size == CTX_STATE_SIZE_NEWDRM)
     dwords += 4;


   return dwords;
}

static void ctx_emit_cs(struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   BATCH_LOCALS(&r200->radeon);
   struct radeon_renderbuffer *rrb, *drb;
   uint32_t cbpitch = 0;
   uint32_t zbpitch = 0;
   uint32_t dwords = atom->check(ctx, atom);
   uint32_t depth_fmt;

   rrb = radeon_get_colorbuffer(&r200->radeon);
   if (!rrb || !rrb->bo) {
      return;
   }

   atom->cmd[CTX_RB3D_CNTL] &= ~(0xf << 10);
   if (rrb->cpp == 4)
	atom->cmd[CTX_RB3D_CNTL] |= RADEON_COLOR_FORMAT_ARGB8888;
   else switch (rrb->base.Base.Format) {
   case MESA_FORMAT_RGB565:
	atom->cmd[CTX_RB3D_CNTL] |= RADEON_COLOR_FORMAT_RGB565;
	break;
   case MESA_FORMAT_ARGB4444:
	atom->cmd[CTX_RB3D_CNTL] |= RADEON_COLOR_FORMAT_ARGB4444;
	break;
   case MESA_FORMAT_ARGB1555:
	atom->cmd[CTX_RB3D_CNTL] |= RADEON_COLOR_FORMAT_ARGB1555;
	break;
   default:
	_mesa_problem(ctx, "Unexpected format in ctx_emit_cs");
   }

   cbpitch = (rrb->pitch / rrb->cpp);
   if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE)
       cbpitch |= R200_COLOR_TILE_ENABLE;
   if (rrb->bo->flags & RADEON_BO_FLAGS_MICRO_TILE)
       cbpitch |= R200_COLOR_MICROTILE_ENABLE;


   drb = radeon_get_depthbuffer(&r200->radeon);
   if (drb) {
     zbpitch = (drb->pitch / drb->cpp);
     if (drb->cpp == 4)
        depth_fmt = RADEON_DEPTH_FORMAT_24BIT_INT_Z;
     else
        depth_fmt = RADEON_DEPTH_FORMAT_16BIT_INT_Z;
     atom->cmd[CTX_RB3D_ZSTENCILCNTL] &= ~RADEON_DEPTH_FORMAT_MASK;
     atom->cmd[CTX_RB3D_ZSTENCILCNTL] |= depth_fmt;
   }

   /* output the first 7 bytes of context */
   BEGIN_BATCH_NO_AUTOSTATE(dwords);

   /* In the CS case we need to split this up */
   OUT_BATCH(CP_PACKET0(packet[0].start, 3));
   OUT_BATCH_TABLE((atom->cmd + 1), 4);

   if (drb) {
     OUT_BATCH(CP_PACKET0(RADEON_RB3D_DEPTHOFFSET, 0));
     OUT_BATCH_RELOC(0, drb->bo, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);

     OUT_BATCH(CP_PACKET0(RADEON_RB3D_DEPTHPITCH, 0));
     OUT_BATCH(zbpitch);
   }

   OUT_BATCH(CP_PACKET0(RADEON_RB3D_ZSTENCILCNTL, 0));
   OUT_BATCH(atom->cmd[CTX_RB3D_ZSTENCILCNTL]);
   OUT_BATCH(CP_PACKET0(RADEON_PP_CNTL, 1));
   OUT_BATCH(atom->cmd[CTX_PP_CNTL]);
   OUT_BATCH(atom->cmd[CTX_RB3D_CNTL]);


   if (rrb) {
     OUT_BATCH(CP_PACKET0(RADEON_RB3D_COLOROFFSET, 0));
     OUT_BATCH_RELOC(rrb->draw_offset, rrb->bo, rrb->draw_offset, 0, RADEON_GEM_DOMAIN_VRAM, 0);

     OUT_BATCH(CP_PACKET0(RADEON_RB3D_COLORPITCH, 0));
     OUT_BATCH_RELOC(cbpitch, rrb->bo, cbpitch, 0, RADEON_GEM_DOMAIN_VRAM, 0);
   }

   if (atom->cmd_size == CTX_STATE_SIZE_NEWDRM) {
     OUT_BATCH_TABLE((atom->cmd + 14), 4);
   }

   END_BATCH();
}

static int get_tex_mm_size(struct gl_context* ctx, struct radeon_state_atom *atom)
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   uint32_t dwords = atom->cmd_size + 2;
   int hastexture = 1;
   int i = atom->idx;
   radeonTexObj *t = r200->state.texture.unit[i].texobj;
   if (!t)
	hastexture = 0;
   else {
	if (!t->mt && !t->bo)
		hastexture = 0;
   }

   if (!hastexture)
     dwords -= 4;
   return dwords;
}

static int check_tex_pair_mm(struct gl_context* ctx, struct radeon_state_atom *atom)
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   /** XOR is bit flip operation so use it for finding pair */
   if (!(r200->state.texture.unit[atom->idx].unitneeded | r200->state.texture.unit[atom->idx ^ 1].unitneeded))
     return 0;

   return get_tex_mm_size(ctx, atom);
}

static int check_tex_mm(struct gl_context* ctx, struct radeon_state_atom *atom)
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   if (!(r200->state.texture.unit[atom->idx].unitneeded))
     return 0;

   return get_tex_mm_size(ctx, atom);
}


static void tex_emit_mm(struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   BATCH_LOCALS(&r200->radeon);
   uint32_t dwords = atom->check(ctx, atom);
   int i = atom->idx;
   radeonTexObj *t = r200->state.texture.unit[i].texobj;

   if (!r200->state.texture.unit[i].unitneeded && !(dwords <= atom->cmd_size))
        dwords -= 4;
   BEGIN_BATCH_NO_AUTOSTATE(dwords);

   OUT_BATCH(CP_PACKET0(R200_PP_TXFILTER_0 + (32 * i), 7));
   OUT_BATCH_TABLE((atom->cmd + 1), 8);

   if (dwords > atom->cmd_size) {
     OUT_BATCH(CP_PACKET0(R200_PP_TXOFFSET_0 + (24 * i), 0));
     if (t->mt && !t->image_override) {
        OUT_BATCH_RELOC(t->tile_bits, t->mt->bo, 0,
		  RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
      } else {
	if (t->bo)
            OUT_BATCH_RELOC(t->tile_bits, t->bo, 0,
                            RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
      }
   }
   END_BATCH();
}

static void cube_emit_cs(struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r200ContextPtr r200 = R200_CONTEXT(ctx);
   BATCH_LOCALS(&r200->radeon);
   uint32_t dwords = atom->check(ctx, atom);
   int i = atom->idx, j;
   radeonTexObj *t = r200->state.texture.unit[i].texobj;
   radeon_mipmap_level *lvl;
   if (!(t && !t->image_override))
     dwords = 2;

   BEGIN_BATCH_NO_AUTOSTATE(dwords);
   OUT_BATCH_TABLE(atom->cmd, 2);

   if (t && !t->image_override) {
     lvl = &t->mt->levels[0];
     for (j = 1; j <= 5; j++) {
       OUT_BATCH(CP_PACKET0(R200_PP_CUBIC_OFFSET_F1_0 + (24*i) + (4 * (j-1)), 0));
       OUT_BATCH_RELOC(lvl->faces[j].offset, t->mt->bo, lvl->faces[j].offset,
			RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
     }
   }
   END_BATCH();
}

/* Initialize the context's hardware state.
 */
void r200InitState( r200ContextPtr rmesa )
{
   struct gl_context *ctx = rmesa->radeon.glCtx;
   GLuint i;

   rmesa->radeon.Fallback = 0;

   rmesa->radeon.hw.max_state_size = 0;

#define ALLOC_STATE( ATOM, CHK, SZ, NM, IDX )				\
   do {								\
      rmesa->hw.ATOM.cmd_size = SZ;				\
      rmesa->hw.ATOM.cmd = (GLuint *)CALLOC(SZ * sizeof(int));	\
      rmesa->hw.ATOM.lastcmd = (GLuint *)CALLOC(SZ * sizeof(int));	\
      rmesa->hw.ATOM.name = NM;					\
      rmesa->hw.ATOM.idx = IDX;					\
      if (check_##CHK != check_never) {				\
         rmesa->hw.ATOM.check = check_##CHK;			\
         rmesa->radeon.hw.max_state_size += SZ * sizeof(int);	\
      } else {							\
         rmesa->hw.ATOM.check = NULL;				\
      }								\
      rmesa->hw.ATOM.dirty = GL_FALSE;				\
   } while (0)


   /* Allocate state buffers:
    */
   ALLOC_STATE( ctx, always_add4, CTX_STATE_SIZE_NEWDRM, "CTX/context", 0 );

   rmesa->hw.ctx.emit = ctx_emit_cs;
   rmesa->hw.ctx.check = check_always_ctx;
   ALLOC_STATE( set, always, SET_STATE_SIZE, "SET/setup", 0 );
   ALLOC_STATE( lin, always, LIN_STATE_SIZE, "LIN/line", 0 );
   ALLOC_STATE( msk, always, MSK_STATE_SIZE, "MSK/mask", 0 );
   ALLOC_STATE( vpt, always, VPT_STATE_SIZE, "VPT/viewport", 0 );
   ALLOC_STATE( vtx, always, VTX_STATE_SIZE, "VTX/vertex", 0 );
   ALLOC_STATE( vap, always, VAP_STATE_SIZE, "VAP/vap", 0 );
   ALLOC_STATE( vte, always, VTE_STATE_SIZE, "VTE/vte", 0 );
   ALLOC_STATE( msc, always, MSC_STATE_SIZE, "MSC/misc", 0 );
   ALLOC_STATE( cst, always, CST_STATE_SIZE, "CST/constant", 0 );
   ALLOC_STATE( zbs, always, ZBS_STATE_SIZE, "ZBS/zbias", 0 );
   ALLOC_STATE( tf, tf, TF_STATE_SIZE, "TF/tfactor", 0 );
   {
      int state_size = TEX_STATE_SIZE_NEWDRM;
      if (rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_R200) {
         /* make sure texture units 0/1 are emitted pair-wise for r200 t0 hang workaround */
         ALLOC_STATE( tex[0], tex_pair_mm, state_size, "TEX/tex-0", 0 );
         ALLOC_STATE( tex[1], tex_pair_mm, state_size, "TEX/tex-1", 1 );
         ALLOC_STATE( tam, tex_any, TAM_STATE_SIZE, "TAM/tam", 0 );
      }
      else {
         ALLOC_STATE( tex[0], tex_mm, state_size, "TEX/tex-0", 0 );
         ALLOC_STATE( tex[1], tex_mm, state_size, "TEX/tex-1", 1 );
         ALLOC_STATE( tam, never, TAM_STATE_SIZE, "TAM/tam", 0 );
      }
      ALLOC_STATE( tex[2], tex_mm, state_size, "TEX/tex-2", 2 );
      ALLOC_STATE( tex[3], tex_mm, state_size, "TEX/tex-3", 3 );
      ALLOC_STATE( tex[4], tex_mm, state_size, "TEX/tex-4", 4 );
      ALLOC_STATE( tex[5], tex_mm, state_size, "TEX/tex-5", 5 );
      ALLOC_STATE( atf, afs, ATF_STATE_SIZE, "ATF/tfactor", 0 );
      ALLOC_STATE( afs[0], afs_pass1, AFS_STATE_SIZE, "AFS/afsinst-0", 0 );
      ALLOC_STATE( afs[1], afs, AFS_STATE_SIZE, "AFS/afsinst-1", 1 );
   }

   ALLOC_STATE( stp, polygon_stipple, STP_STATE_SIZE, "STP/stp", 0 );

   for (i = 0; i < 6; i++)
      rmesa->hw.tex[i].emit = tex_emit_mm;
   ALLOC_STATE( cube[0], tex_cube, CUBE_STATE_SIZE, "CUBE/tex-0", 0 );
   ALLOC_STATE( cube[1], tex_cube, CUBE_STATE_SIZE, "CUBE/tex-1", 1 );
   ALLOC_STATE( cube[2], tex_cube, CUBE_STATE_SIZE, "CUBE/tex-2", 2 );
   ALLOC_STATE( cube[3], tex_cube, CUBE_STATE_SIZE, "CUBE/tex-3", 3 );
   ALLOC_STATE( cube[4], tex_cube, CUBE_STATE_SIZE, "CUBE/tex-4", 4 );
   ALLOC_STATE( cube[5], tex_cube, CUBE_STATE_SIZE, "CUBE/tex-5", 5 );
   for (i = 0; i < 6; i++) {
      rmesa->hw.cube[i].emit = cube_emit_cs;
      rmesa->hw.cube[i].check = check_tex_cube_cs;
   }

   ALLOC_STATE( pvs, tcl_vp, PVS_STATE_SIZE, "PVS/pvscntl", 0 );
   ALLOC_STATE( vpi[0], tcl_vp_add4, VPI_STATE_SIZE, "VP/vertexprog-0", 0 );
   ALLOC_STATE( vpi[1], tcl_vp_size_add4, VPI_STATE_SIZE, "VP/vertexprog-1", 1 );
   ALLOC_STATE( vpp[0], tcl_vp_add4, VPP_STATE_SIZE, "VPP/vertexparam-0", 0 );
   ALLOC_STATE( vpp[1], tcl_vpp_size_add4, VPP_STATE_SIZE, "VPP/vertexparam-1", 1 );

   /* FIXME: this atom has two commands, we need only one (ucp_vert_blend) for vp */
   ALLOC_STATE( tcl, tcl_or_vp, TCL_STATE_SIZE, "TCL/tcl", 0 );
   ALLOC_STATE( msl, tcl, MSL_STATE_SIZE, "MSL/matrix-select", 0 );
   ALLOC_STATE( tcg, tcl, TCG_STATE_SIZE, "TCG/texcoordgen", 0 );
   ALLOC_STATE( mtl[0], tcl_lighting_add6, MTL_STATE_SIZE, "MTL0/material0", 0 );
   ALLOC_STATE( mtl[1], tcl_lighting_add6, MTL_STATE_SIZE, "MTL1/material1", 1 );
   ALLOC_STATE( grd, tcl_or_vp_add2, GRD_STATE_SIZE, "GRD/guard-band", 0 );
   ALLOC_STATE( fog, tcl_fog_add4, FOG_STATE_SIZE, "FOG/fog", 0 );
   ALLOC_STATE( glt, tcl_lighting_add4, GLT_STATE_SIZE, "GLT/light-global", 0 );
   ALLOC_STATE( eye, tcl_lighting_add4, EYE_STATE_SIZE, "EYE/eye-vector", 0 );
   ALLOC_STATE( mat[R200_MTX_MV], tcl_add4, MAT_STATE_SIZE, "MAT/modelview", 0 );
   ALLOC_STATE( mat[R200_MTX_IMV], tcl_add4, MAT_STATE_SIZE, "MAT/it-modelview", 0 );
   ALLOC_STATE( mat[R200_MTX_MVP], tcl_add4, MAT_STATE_SIZE, "MAT/modelproject", 0 );
   ALLOC_STATE( mat[R200_MTX_TEX0], tcl_tex_add4, MAT_STATE_SIZE, "MAT/texmat0", 0 );
   ALLOC_STATE( mat[R200_MTX_TEX1], tcl_tex_add4, MAT_STATE_SIZE, "MAT/texmat1", 1 );
   ALLOC_STATE( mat[R200_MTX_TEX2], tcl_tex_add4, MAT_STATE_SIZE, "MAT/texmat2", 2 );
   ALLOC_STATE( mat[R200_MTX_TEX3], tcl_tex_add4, MAT_STATE_SIZE, "MAT/texmat3", 3 );
   ALLOC_STATE( mat[R200_MTX_TEX4], tcl_tex_add4, MAT_STATE_SIZE, "MAT/texmat4", 4 );
   ALLOC_STATE( mat[R200_MTX_TEX5], tcl_tex_add4, MAT_STATE_SIZE, "MAT/texmat5", 5 );
   ALLOC_STATE( ucp[0], tcl_ucp_add4, UCP_STATE_SIZE, "UCP/userclip-0", 0 );
   ALLOC_STATE( ucp[1], tcl_ucp_add4, UCP_STATE_SIZE, "UCP/userclip-1", 1 );
   ALLOC_STATE( ucp[2], tcl_ucp_add4, UCP_STATE_SIZE, "UCP/userclip-2", 2 );
   ALLOC_STATE( ucp[3], tcl_ucp_add4, UCP_STATE_SIZE, "UCP/userclip-3", 3 );
   ALLOC_STATE( ucp[4], tcl_ucp_add4, UCP_STATE_SIZE, "UCP/userclip-4", 4 );
   ALLOC_STATE( ucp[5], tcl_ucp_add4, UCP_STATE_SIZE, "UCP/userclip-5", 5 );
   ALLOC_STATE( lit[0], tcl_light_add6, LIT_STATE_SIZE, "LIT/light-0", 0 );
   ALLOC_STATE( lit[1], tcl_light_add6, LIT_STATE_SIZE, "LIT/light-1", 1 );
   ALLOC_STATE( lit[2], tcl_light_add6, LIT_STATE_SIZE, "LIT/light-2", 2 );
   ALLOC_STATE( lit[3], tcl_light_add6, LIT_STATE_SIZE, "LIT/light-3", 3 );
   ALLOC_STATE( lit[4], tcl_light_add6, LIT_STATE_SIZE, "LIT/light-4", 4 );
   ALLOC_STATE( lit[5], tcl_light_add6, LIT_STATE_SIZE, "LIT/light-5", 5 );
   ALLOC_STATE( lit[6], tcl_light_add6, LIT_STATE_SIZE, "LIT/light-6", 6 );
   ALLOC_STATE( lit[7], tcl_light_add6, LIT_STATE_SIZE, "LIT/light-7", 7 );
   ALLOC_STATE( sci, rrb, SCI_STATE_SIZE, "SCI/scissor", 0 );
   ALLOC_STATE( pix[0], pix_zero, PIX_STATE_SIZE, "PIX/pixstage-0", 0 );
   ALLOC_STATE( pix[1], texenv, PIX_STATE_SIZE, "PIX/pixstage-1", 1 );
   ALLOC_STATE( pix[2], texenv, PIX_STATE_SIZE, "PIX/pixstage-2", 2 );
   ALLOC_STATE( pix[3], texenv, PIX_STATE_SIZE, "PIX/pixstage-3", 3 );
   ALLOC_STATE( pix[4], texenv, PIX_STATE_SIZE, "PIX/pixstage-4", 4 );
   ALLOC_STATE( pix[5], texenv, PIX_STATE_SIZE, "PIX/pixstage-5", 5 );
   ALLOC_STATE( prf, always, PRF_STATE_SIZE, "PRF/performance-tri", 0 );
   ALLOC_STATE( spr, always, SPR_STATE_SIZE, "SPR/pointsprite", 0 );
   ALLOC_STATE( ptp, tcl_add8, PTP_STATE_SIZE, "PTP/pointparams", 0 );

   r200SetUpAtomList( rmesa );

   /* Fill in the packet headers:
    */
   rmesa->hw.ctx.cmd[CTX_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_PP_MISC);
   rmesa->hw.ctx.cmd[CTX_CMD_1] = cmdpkt(rmesa, RADEON_EMIT_PP_CNTL);
   rmesa->hw.ctx.cmd[CTX_CMD_2] = cmdpkt(rmesa, RADEON_EMIT_RB3D_COLORPITCH);
   rmesa->hw.ctx.cmd[CTX_CMD_3] = cmdpkt(rmesa, R200_EMIT_RB3D_BLENDCOLOR);
   rmesa->hw.lin.cmd[LIN_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_RE_LINE_PATTERN);
   rmesa->hw.lin.cmd[LIN_CMD_1] = cmdpkt(rmesa, RADEON_EMIT_SE_LINE_WIDTH);
   rmesa->hw.msk.cmd[MSK_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_RB3D_STENCILREFMASK);
   rmesa->hw.vpt.cmd[VPT_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_SE_VPORT_XSCALE);
   rmesa->hw.set.cmd[SET_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_SE_CNTL);
   rmesa->hw.msc.cmd[MSC_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_RE_MISC);
   rmesa->hw.cst.cmd[CST_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_CNTL_X);
   rmesa->hw.cst.cmd[CST_CMD_1] = cmdpkt(rmesa, R200_EMIT_RB3D_DEPTHXY_OFFSET);
   rmesa->hw.cst.cmd[CST_CMD_2] = cmdpkt(rmesa, R200_EMIT_RE_AUX_SCISSOR_CNTL);
   rmesa->hw.cst.cmd[CST_CMD_4] = cmdpkt(rmesa, R200_EMIT_SE_VAP_CNTL_STATUS);
   rmesa->hw.cst.cmd[CST_CMD_5] = cmdpkt(rmesa, R200_EMIT_RE_POINTSIZE);
   rmesa->hw.cst.cmd[CST_CMD_6] = cmdpkt(rmesa, R200_EMIT_TCL_INPUT_VTX_VECTOR_ADDR_0);
   rmesa->hw.tam.cmd[TAM_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_TAM_DEBUG3);
   rmesa->hw.tf.cmd[TF_CMD_0] = cmdpkt(rmesa, R200_EMIT_TFACTOR_0);
   rmesa->hw.atf.cmd[ATF_CMD_0] = cmdpkt(rmesa, R200_EMIT_ATF_TFACTOR);
   rmesa->hw.tex[0].cmd[TEX_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_TXCTLALL_0);
   rmesa->hw.tex[0].cmd[TEX_CMD_1_NEWDRM] = cmdpkt(rmesa, R200_EMIT_PP_TXOFFSET_0);
   rmesa->hw.tex[1].cmd[TEX_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_TXCTLALL_1);
   rmesa->hw.tex[1].cmd[TEX_CMD_1_NEWDRM] = cmdpkt(rmesa, R200_EMIT_PP_TXOFFSET_1);
   rmesa->hw.tex[2].cmd[TEX_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_TXCTLALL_2);
   rmesa->hw.tex[2].cmd[TEX_CMD_1_NEWDRM] = cmdpkt(rmesa, R200_EMIT_PP_TXOFFSET_2);
   rmesa->hw.tex[3].cmd[TEX_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_TXCTLALL_3);
   rmesa->hw.tex[3].cmd[TEX_CMD_1_NEWDRM] = cmdpkt(rmesa, R200_EMIT_PP_TXOFFSET_3);
   rmesa->hw.tex[4].cmd[TEX_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_TXCTLALL_4);
   rmesa->hw.tex[4].cmd[TEX_CMD_1_NEWDRM] = cmdpkt(rmesa, R200_EMIT_PP_TXOFFSET_4);
   rmesa->hw.tex[5].cmd[TEX_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_TXCTLALL_5);
   rmesa->hw.tex[5].cmd[TEX_CMD_1_NEWDRM] = cmdpkt(rmesa, R200_EMIT_PP_TXOFFSET_5);
   rmesa->hw.afs[0].cmd[AFS_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_AFS_0);
   rmesa->hw.afs[1].cmd[AFS_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_AFS_1);
   rmesa->hw.pvs.cmd[PVS_CMD_0] = cmdpkt(rmesa, R200_EMIT_VAP_PVS_CNTL);
   rmesa->hw.cube[0].cmd[CUBE_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_CUBIC_FACES_0);
   rmesa->hw.cube[0].cmd[CUBE_CMD_1] = cmdpkt(rmesa, R200_EMIT_PP_CUBIC_OFFSETS_0);
   rmesa->hw.cube[1].cmd[CUBE_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_CUBIC_FACES_1);
   rmesa->hw.cube[1].cmd[CUBE_CMD_1] = cmdpkt(rmesa, R200_EMIT_PP_CUBIC_OFFSETS_1);
   rmesa->hw.cube[2].cmd[CUBE_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_CUBIC_FACES_2);
   rmesa->hw.cube[2].cmd[CUBE_CMD_1] = cmdpkt(rmesa, R200_EMIT_PP_CUBIC_OFFSETS_2);
   rmesa->hw.cube[3].cmd[CUBE_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_CUBIC_FACES_3);
   rmesa->hw.cube[3].cmd[CUBE_CMD_1] = cmdpkt(rmesa, R200_EMIT_PP_CUBIC_OFFSETS_3);
   rmesa->hw.cube[4].cmd[CUBE_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_CUBIC_FACES_4);
   rmesa->hw.cube[4].cmd[CUBE_CMD_1] = cmdpkt(rmesa, R200_EMIT_PP_CUBIC_OFFSETS_4);
   rmesa->hw.cube[5].cmd[CUBE_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_CUBIC_FACES_5);
   rmesa->hw.cube[5].cmd[CUBE_CMD_1] = cmdpkt(rmesa, R200_EMIT_PP_CUBIC_OFFSETS_5);
   rmesa->hw.pix[0].cmd[PIX_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_TXCBLEND_0);
   rmesa->hw.pix[1].cmd[PIX_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_TXCBLEND_1);
   rmesa->hw.pix[2].cmd[PIX_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_TXCBLEND_2);
   rmesa->hw.pix[3].cmd[PIX_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_TXCBLEND_3);
   rmesa->hw.pix[4].cmd[PIX_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_TXCBLEND_4);
   rmesa->hw.pix[5].cmd[PIX_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_TXCBLEND_5);
   rmesa->hw.zbs.cmd[ZBS_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_SE_ZBIAS_FACTOR);
   rmesa->hw.tcl.cmd[TCL_CMD_0] = cmdpkt(rmesa, R200_EMIT_TCL_LIGHT_MODEL_CTL_0);
   rmesa->hw.tcl.cmd[TCL_CMD_1] = cmdpkt(rmesa, R200_EMIT_TCL_UCP_VERT_BLEND_CTL);
   rmesa->hw.tcg.cmd[TCG_CMD_0] = cmdpkt(rmesa, R200_EMIT_TEX_PROC_CTL_2);
   rmesa->hw.msl.cmd[MSL_CMD_0] = cmdpkt(rmesa, R200_EMIT_MATRIX_SELECT_0);
   rmesa->hw.vap.cmd[VAP_CMD_0] = cmdpkt(rmesa, R200_EMIT_VAP_CTL);
   rmesa->hw.vtx.cmd[VTX_CMD_0] = cmdpkt(rmesa, R200_EMIT_VTX_FMT_0);
   rmesa->hw.vtx.cmd[VTX_CMD_1] = cmdpkt(rmesa, R200_EMIT_OUTPUT_VTX_COMP_SEL);
   rmesa->hw.vtx.cmd[VTX_CMD_2] = cmdpkt(rmesa, R200_EMIT_SE_VTX_STATE_CNTL);
   rmesa->hw.vte.cmd[VTE_CMD_0] = cmdpkt(rmesa, R200_EMIT_VTE_CNTL);
   rmesa->hw.prf.cmd[PRF_CMD_0] = cmdpkt(rmesa, R200_EMIT_PP_TRI_PERF_CNTL);
   rmesa->hw.spr.cmd[SPR_CMD_0] = cmdpkt(rmesa, R200_EMIT_TCL_POINT_SPRITE_CNTL);

   rmesa->hw.sci.cmd[SCI_CMD_1] = CP_PACKET0(R200_RE_TOP_LEFT, 0);
   rmesa->hw.sci.cmd[SCI_CMD_2] = CP_PACKET0(R200_RE_WIDTH_HEIGHT, 0);

   rmesa->hw.stp.cmd[STP_CMD_0] = CP_PACKET0(RADEON_RE_STIPPLE_ADDR, 0);
   rmesa->hw.stp.cmd[STP_DATA_0] = 0;
   rmesa->hw.stp.cmd[STP_CMD_1] = CP_PACKET0_ONE(RADEON_RE_STIPPLE_DATA, 31);

   rmesa->hw.mtl[0].emit = mtl_emit;
   rmesa->hw.mtl[1].emit = mtl_emit;

   rmesa->hw.vpi[0].emit = veclinear_emit;
   rmesa->hw.vpi[1].emit = veclinear_emit;
   rmesa->hw.vpp[0].emit = veclinear_emit;
   rmesa->hw.vpp[1].emit = veclinear_emit;

   rmesa->hw.grd.emit = scl_emit;
   rmesa->hw.fog.emit = vec_emit;
   rmesa->hw.glt.emit = vec_emit;
   rmesa->hw.eye.emit = vec_emit;

   for (i = R200_MTX_MV; i <= R200_MTX_TEX5; i++)
      rmesa->hw.mat[i].emit = vec_emit;

   for (i = 0; i < 8; i++)
      rmesa->hw.lit[i].emit = lit_emit;

   for (i = 0; i < 6; i++)
      rmesa->hw.ucp[i].emit = vec_emit;

   rmesa->hw.ptp.emit = ptp_emit;

   rmesa->hw.mtl[0].cmd[MTL_CMD_0] = 
      cmdvec( R200_VS_MAT_0_EMISS, 1, 16 );
   rmesa->hw.mtl[0].cmd[MTL_CMD_1] = 
      cmdscl2( R200_SS_MAT_0_SHININESS, 1, 1 );
   rmesa->hw.mtl[1].cmd[MTL_CMD_0] =
      cmdvec( R200_VS_MAT_1_EMISS, 1, 16 );
   rmesa->hw.mtl[1].cmd[MTL_CMD_1] =
      cmdscl2( R200_SS_MAT_1_SHININESS, 1, 1 );

   rmesa->hw.vpi[0].cmd[VPI_CMD_0] =
      cmdveclinear( R200_PVS_PROG0, 64 );
   rmesa->hw.vpi[1].cmd[VPI_CMD_0] =
      cmdveclinear( R200_PVS_PROG1, 64 );
   rmesa->hw.vpp[0].cmd[VPP_CMD_0] =
      cmdveclinear( R200_PVS_PARAM0, 96 );
   rmesa->hw.vpp[1].cmd[VPP_CMD_0] =
      cmdveclinear( R200_PVS_PARAM1, 96 );

   rmesa->hw.grd.cmd[GRD_CMD_0] = 
      cmdscl( R200_SS_VERT_GUARD_CLIP_ADJ_ADDR, 1, 4 );
   rmesa->hw.fog.cmd[FOG_CMD_0] = 
      cmdvec( R200_VS_FOG_PARAM_ADDR, 1, 4 );
   rmesa->hw.glt.cmd[GLT_CMD_0] = 
      cmdvec( R200_VS_GLOBAL_AMBIENT_ADDR, 1, 4 );
   rmesa->hw.eye.cmd[EYE_CMD_0] = 
      cmdvec( R200_VS_EYE_VECTOR_ADDR, 1, 4 );

   rmesa->hw.mat[R200_MTX_MV].cmd[MAT_CMD_0] = 
      cmdvec( R200_VS_MATRIX_0_MV, 1, 16);
   rmesa->hw.mat[R200_MTX_IMV].cmd[MAT_CMD_0] = 
      cmdvec( R200_VS_MATRIX_1_INV_MV, 1, 16);
   rmesa->hw.mat[R200_MTX_MVP].cmd[MAT_CMD_0] = 
      cmdvec( R200_VS_MATRIX_2_MVP, 1, 16);
   rmesa->hw.mat[R200_MTX_TEX0].cmd[MAT_CMD_0] = 
      cmdvec( R200_VS_MATRIX_3_TEX0, 1, 16);
   rmesa->hw.mat[R200_MTX_TEX1].cmd[MAT_CMD_0] = 
      cmdvec( R200_VS_MATRIX_4_TEX1, 1, 16);
   rmesa->hw.mat[R200_MTX_TEX2].cmd[MAT_CMD_0] = 
      cmdvec( R200_VS_MATRIX_5_TEX2, 1, 16);
   rmesa->hw.mat[R200_MTX_TEX3].cmd[MAT_CMD_0] = 
      cmdvec( R200_VS_MATRIX_6_TEX3, 1, 16);
   rmesa->hw.mat[R200_MTX_TEX4].cmd[MAT_CMD_0] = 
      cmdvec( R200_VS_MATRIX_7_TEX4, 1, 16);
   rmesa->hw.mat[R200_MTX_TEX5].cmd[MAT_CMD_0] = 
      cmdvec( R200_VS_MATRIX_8_TEX5, 1, 16);

   for (i = 0 ; i < 8; i++) {
      rmesa->hw.lit[i].cmd[LIT_CMD_0] = 
	 cmdvec( R200_VS_LIGHT_AMBIENT_ADDR + i, 8, 24 );
      rmesa->hw.lit[i].cmd[LIT_CMD_1] = 
	 cmdscl( R200_SS_LIGHT_DCD_ADDR + i, 8, 7 );
   }

   for (i = 0 ; i < 6; i++) {
      rmesa->hw.ucp[i].cmd[UCP_CMD_0] = 
	 cmdvec( R200_VS_UCP_ADDR + i, 1, 4 );
   }

   rmesa->hw.ptp.cmd[PTP_CMD_0] =
      cmdvec( R200_VS_PNT_SPRITE_VPORT_SCALE, 1, 4 );
   rmesa->hw.ptp.cmd[PTP_CMD_1] =
      cmdvec( R200_VS_PNT_SPRITE_ATT_CONST, 1, 12 );

   /* Initial Harware state:
    */
   rmesa->hw.ctx.cmd[CTX_PP_MISC] = (R200_ALPHA_TEST_PASS
				     /* | R200_RIGHT_HAND_CUBE_OGL*/);

   rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] = (R200_FOG_VERTEX |
					  R200_FOG_USE_SPEC_ALPHA);

   rmesa->hw.ctx.cmd[CTX_RE_SOLID_COLOR] = 0x00000000;

   rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCNTL] = (R200_COMB_FCN_ADD_CLAMP |
				(R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
				(R200_BLEND_GL_ZERO << R200_DST_BLEND_SHIFT));

   rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCOLOR] = 0x00000000;
   rmesa->hw.ctx.cmd[CTX_RB3D_ABLENDCNTL] = (R200_COMB_FCN_ADD_CLAMP |
				(R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
				(R200_BLEND_GL_ZERO << R200_DST_BLEND_SHIFT));
   rmesa->hw.ctx.cmd[CTX_RB3D_CBLENDCNTL] = (R200_COMB_FCN_ADD_CLAMP |
				(R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
				(R200_BLEND_GL_ZERO << R200_DST_BLEND_SHIFT));

   rmesa->hw.ctx.cmd[CTX_RB3D_DEPTHOFFSET] =
      rmesa->radeon.radeonScreen->depthOffset + rmesa->radeon.radeonScreen->fbLocation;

   rmesa->hw.ctx.cmd[CTX_RB3D_DEPTHPITCH] = 
      ((rmesa->radeon.radeonScreen->depthPitch &
	R200_DEPTHPITCH_MASK) |
       R200_DEPTH_ENDIAN_NO_SWAP);
   
   if (rmesa->using_hyperz)
      rmesa->hw.ctx.cmd[CTX_RB3D_DEPTHPITCH] |= R200_DEPTH_HYPERZ;

   rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] = (R200_Z_TEST_LESS |
					       R200_STENCIL_TEST_ALWAYS |
					       R200_STENCIL_FAIL_KEEP |
					       R200_STENCIL_ZPASS_KEEP |
					       R200_STENCIL_ZFAIL_KEEP |
					       R200_Z_WRITE_ENABLE);

   if (rmesa->using_hyperz) {
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_COMPRESSION_ENABLE |
						  R200_Z_DECOMPRESSION_ENABLE;
/*      if (rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_R200)
	 rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_Z_HIERARCHY_ENABLE;*/
   }

   rmesa->hw.ctx.cmd[CTX_PP_CNTL] = (R200_ANTI_ALIAS_NONE 
 				     | R200_TEX_BLEND_0_ENABLE);

   switch ( driQueryOptioni( &rmesa->radeon.optionCache, "dither_mode" ) ) {
   case DRI_CONF_DITHER_XERRORDIFFRESET:
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |= R200_DITHER_INIT;
      break;
   case DRI_CONF_DITHER_ORDERED:
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |= R200_SCALE_DITHER_ENABLE;
      break;
   }
   if ( driQueryOptioni( &rmesa->radeon.optionCache, "round_mode" ) ==
	DRI_CONF_ROUND_ROUND )
      rmesa->radeon.state.color.roundEnable = R200_ROUND_ENABLE;
   else
      rmesa->radeon.state.color.roundEnable = 0;
   if ( driQueryOptioni (&rmesa->radeon.optionCache, "color_reduction" ) ==
	DRI_CONF_COLOR_REDUCTION_DITHER )
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |= R200_DITHER_ENABLE;
   else
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |= rmesa->radeon.state.color.roundEnable;

   rmesa->hw.prf.cmd[PRF_PP_TRI_PERF] = R200_TRI_CUTOFF_MASK - R200_TRI_CUTOFF_MASK * 
			driQueryOptionf (&rmesa->radeon.optionCache,"texture_blend_quality");
   rmesa->hw.prf.cmd[PRF_PP_PERF_CNTL] = 0;

   rmesa->hw.set.cmd[SET_SE_CNTL] = (R200_FFACE_CULL_CCW |
				     R200_BFACE_SOLID |
				     R200_FFACE_SOLID |
				     R200_FLAT_SHADE_VTX_LAST |
				     R200_DIFFUSE_SHADE_GOURAUD |
				     R200_ALPHA_SHADE_GOURAUD |
				     R200_SPECULAR_SHADE_GOURAUD |
				     R200_FOG_SHADE_GOURAUD |
				     R200_DISC_FOG_SHADE_GOURAUD |
				     R200_VTX_PIX_CENTER_OGL |
				     R200_ROUND_MODE_TRUNC |
				     R200_ROUND_PREC_8TH_PIX);

   rmesa->hw.set.cmd[SET_RE_CNTL] = (R200_PERSPECTIVE_ENABLE |
				     R200_SCISSOR_ENABLE);

   rmesa->hw.lin.cmd[LIN_RE_LINE_PATTERN] = ((1 << 16) | 0xffff);

   rmesa->hw.lin.cmd[LIN_RE_LINE_STATE] = 
      ((0 << R200_LINE_CURRENT_PTR_SHIFT) |
       (1 << R200_LINE_CURRENT_COUNT_SHIFT));

   rmesa->hw.lin.cmd[LIN_SE_LINE_WIDTH] = (1 << 4);

   rmesa->hw.msk.cmd[MSK_RB3D_STENCILREFMASK] = 
      ((0x00 << R200_STENCIL_REF_SHIFT) |
       (0xff << R200_STENCIL_MASK_SHIFT) |
       (0xff << R200_STENCIL_WRITEMASK_SHIFT));

   rmesa->hw.msk.cmd[MSK_RB3D_ROPCNTL] = R200_ROP_COPY;
   rmesa->hw.msk.cmd[MSK_RB3D_PLANEMASK] = 0xffffffff;

   rmesa->hw.tam.cmd[TAM_DEBUG3] = 0;

   rmesa->hw.msc.cmd[MSC_RE_MISC] = 
      ((0 << R200_STIPPLE_X_OFFSET_SHIFT) |
       (0 << R200_STIPPLE_Y_OFFSET_SHIFT) |
       R200_STIPPLE_BIG_BIT_ORDER);


   rmesa->hw.cst.cmd[CST_PP_CNTL_X] = 0;
   rmesa->hw.cst.cmd[CST_RB3D_DEPTHXY_OFFSET] = 0;
   rmesa->hw.cst.cmd[CST_RE_AUX_SCISSOR_CNTL] = 0x0;
   rmesa->hw.cst.cmd[CST_SE_VAP_CNTL_STATUS] =
#ifdef MESA_BIG_ENDIAN
						R200_VC_32BIT_SWAP;
#else
						R200_VC_NO_SWAP;
#endif

   if (!(rmesa->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL)) {
      /* Bypass TCL */
      rmesa->hw.cst.cmd[CST_SE_VAP_CNTL_STATUS] |= (1<<8);
   }

   rmesa->hw.cst.cmd[CST_RE_POINTSIZE] =
      (((GLuint)(ctx->Const.MaxPointSize * 16.0)) << R200_MAXPOINTSIZE_SHIFT) | 0x10;
   rmesa->hw.cst.cmd[CST_SE_TCL_INPUT_VTX_0] =
      (0x0 << R200_VERTEX_POSITION_ADDR__SHIFT);
   rmesa->hw.cst.cmd[CST_SE_TCL_INPUT_VTX_1] =
      (0x02 << R200_VTX_COLOR_0_ADDR__SHIFT) |
      (0x03 << R200_VTX_COLOR_1_ADDR__SHIFT);
   rmesa->hw.cst.cmd[CST_SE_TCL_INPUT_VTX_2] =
      (0x06 << R200_VTX_TEX_0_ADDR__SHIFT) |
      (0x07 << R200_VTX_TEX_1_ADDR__SHIFT) |
      (0x08 << R200_VTX_TEX_2_ADDR__SHIFT) |
      (0x09 << R200_VTX_TEX_3_ADDR__SHIFT);
   rmesa->hw.cst.cmd[CST_SE_TCL_INPUT_VTX_3] =
      (0x0A << R200_VTX_TEX_4_ADDR__SHIFT) |
      (0x0B << R200_VTX_TEX_5_ADDR__SHIFT);
  

   rmesa->hw.vpt.cmd[VPT_SE_VPORT_XSCALE]  = 0x00000000;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_XOFFSET] = 0x00000000;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_YSCALE]  = 0x00000000;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_YOFFSET] = 0x00000000;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_ZSCALE]  = 0x00000000;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_ZOFFSET] = 0x00000000;

   for ( i = 0 ; i < ctx->Const.MaxTextureUnits ; i++ ) {
      rmesa->hw.tex[i].cmd[TEX_PP_TXFILTER] = R200_BORDER_MODE_OGL;
      rmesa->hw.tex[i].cmd[TEX_PP_TXFORMAT] = 
         ((i << R200_TXFORMAT_ST_ROUTE_SHIFT) |  /* <-- note i */
          (2 << R200_TXFORMAT_WIDTH_SHIFT) |
          (2 << R200_TXFORMAT_HEIGHT_SHIFT));
      rmesa->hw.tex[i].cmd[TEX_PP_BORDER_COLOR] = 0;
      rmesa->hw.tex[i].cmd[TEX_PP_TXFORMAT_X] =
         (/* R200_TEXCOORD_PROJ | */
          R200_LOD_BIAS_CORRECTION);	/* Small default bias */
      rmesa->hw.tex[i].cmd[TEX_PP_TXOFFSET_NEWDRM] =
	     rmesa->radeon.radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];
      rmesa->hw.tex[i].cmd[TEX_PP_CUBIC_FACES] = 0;
      rmesa->hw.tex[i].cmd[TEX_PP_TXMULTI_CTL] = 0;

      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_FACES] = 0;
      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_OFFSET_F1] =
         rmesa->radeon.radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];
      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_OFFSET_F2] =
         rmesa->radeon.radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];
      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_OFFSET_F3] =
         rmesa->radeon.radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];
      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_OFFSET_F4] =
         rmesa->radeon.radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];
      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_OFFSET_F5] =
         rmesa->radeon.radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];

      rmesa->hw.pix[i].cmd[PIX_PP_TXCBLEND] =
         (R200_TXC_ARG_A_ZERO |
          R200_TXC_ARG_B_ZERO |
          R200_TXC_ARG_C_DIFFUSE_COLOR |
          R200_TXC_OP_MADD);

      rmesa->hw.pix[i].cmd[PIX_PP_TXCBLEND2] =
         ((i << R200_TXC_TFACTOR_SEL_SHIFT) |
          R200_TXC_SCALE_1X |
          R200_TXC_CLAMP_0_1 |
          R200_TXC_OUTPUT_REG_R0);

      rmesa->hw.pix[i].cmd[PIX_PP_TXABLEND] =
         (R200_TXA_ARG_A_ZERO |
          R200_TXA_ARG_B_ZERO |
          R200_TXA_ARG_C_DIFFUSE_ALPHA |
          R200_TXA_OP_MADD);

      rmesa->hw.pix[i].cmd[PIX_PP_TXABLEND2] =
         ((i << R200_TXA_TFACTOR_SEL_SHIFT) |
          R200_TXA_SCALE_1X |
          R200_TXA_CLAMP_0_1 |
          R200_TXA_OUTPUT_REG_R0);
   }

   rmesa->hw.tf.cmd[TF_TFACTOR_0] = 0;
   rmesa->hw.tf.cmd[TF_TFACTOR_1] = 0;
   rmesa->hw.tf.cmd[TF_TFACTOR_2] = 0;
   rmesa->hw.tf.cmd[TF_TFACTOR_3] = 0;
   rmesa->hw.tf.cmd[TF_TFACTOR_4] = 0;
   rmesa->hw.tf.cmd[TF_TFACTOR_5] = 0;

   rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL] = 
      (R200_VAP_TCL_ENABLE | 
       (0x9 << R200_VAP_VF_MAX_VTX_NUM__SHIFT));

   rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL] = 
      (R200_VPORT_X_SCALE_ENA |
       R200_VPORT_Y_SCALE_ENA |
       R200_VPORT_Z_SCALE_ENA |
       R200_VPORT_X_OFFSET_ENA |
       R200_VPORT_Y_OFFSET_ENA |
       R200_VPORT_Z_OFFSET_ENA |
/* FIXME: Turn on for tex rect only */
       R200_VTX_ST_DENORMALIZED |  
       R200_VTX_W0_FMT); 


   rmesa->hw.vtx.cmd[VTX_VTXFMT_0] = 0;
   rmesa->hw.vtx.cmd[VTX_VTXFMT_1] = 0;
   rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] = 
      ((R200_VTX_Z0 | R200_VTX_W0 |
       (R200_VTX_FP_RGBA << R200_VTX_COLOR_0_SHIFT)));	
   rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_1] = 0;
   rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] = (R200_OUTPUT_XYZW);
   rmesa->hw.vtx.cmd[VTX_STATE_CNTL] = R200_VSC_UPDATE_USER_COLOR_0_ENABLE;
						   

   /* Matrix selection */
   rmesa->hw.msl.cmd[MSL_MATRIX_SELECT_0] = 
      (R200_MTX_MV << R200_MODELVIEW_0_SHIFT);
   
   rmesa->hw.msl.cmd[MSL_MATRIX_SELECT_1] = 
       (R200_MTX_IMV << R200_IT_MODELVIEW_0_SHIFT);

   rmesa->hw.msl.cmd[MSL_MATRIX_SELECT_2] = 
      (R200_MTX_MVP << R200_MODELPROJECT_0_SHIFT);

   rmesa->hw.msl.cmd[MSL_MATRIX_SELECT_3] = 
      ((R200_MTX_TEX0 << R200_TEXMAT_0_SHIFT) |
       (R200_MTX_TEX1 << R200_TEXMAT_1_SHIFT) |
       (R200_MTX_TEX2 << R200_TEXMAT_2_SHIFT) |
       (R200_MTX_TEX3 << R200_TEXMAT_3_SHIFT));

   rmesa->hw.msl.cmd[MSL_MATRIX_SELECT_4] = 
      ((R200_MTX_TEX4 << R200_TEXMAT_4_SHIFT) |
       (R200_MTX_TEX5 << R200_TEXMAT_5_SHIFT));


   /* General TCL state */
   rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] = 
      (R200_SPECULAR_LIGHTS |
       R200_DIFFUSE_SPECULAR_COMBINE |
       R200_LOCAL_LIGHT_VEC_GL |
       R200_LM0_SOURCE_MATERIAL_0 << R200_FRONT_SHININESS_SOURCE_SHIFT |
       R200_LM0_SOURCE_MATERIAL_1 << R200_BACK_SHININESS_SOURCE_SHIFT);

   rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_1] = 
      ((R200_LM1_SOURCE_MATERIAL_0 << R200_FRONT_EMISSIVE_SOURCE_SHIFT) |
       (R200_LM1_SOURCE_MATERIAL_0 << R200_FRONT_AMBIENT_SOURCE_SHIFT) |
       (R200_LM1_SOURCE_MATERIAL_0 << R200_FRONT_DIFFUSE_SOURCE_SHIFT) |
       (R200_LM1_SOURCE_MATERIAL_0 << R200_FRONT_SPECULAR_SOURCE_SHIFT) |
       (R200_LM1_SOURCE_MATERIAL_1 << R200_BACK_EMISSIVE_SOURCE_SHIFT) |
       (R200_LM1_SOURCE_MATERIAL_1 << R200_BACK_AMBIENT_SOURCE_SHIFT) |
       (R200_LM1_SOURCE_MATERIAL_1 << R200_BACK_DIFFUSE_SOURCE_SHIFT) |
       (R200_LM1_SOURCE_MATERIAL_1 << R200_BACK_SPECULAR_SOURCE_SHIFT)); 

   rmesa->hw.tcl.cmd[TCL_PER_LIGHT_CTL_0] = 0; /* filled in via callbacks */
   rmesa->hw.tcl.cmd[TCL_PER_LIGHT_CTL_1] = 0;
   rmesa->hw.tcl.cmd[TCL_PER_LIGHT_CTL_2] = 0;
   rmesa->hw.tcl.cmd[TCL_PER_LIGHT_CTL_3] = 0;
   
   rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] = 
      (R200_UCP_IN_CLIP_SPACE |
       R200_CULL_FRONT_IS_CCW);

   /* Texgen/Texmat state */
   rmesa->hw.tcg.cmd[TCG_TEX_PROC_CTL_2] = 0x00ffffff;
   rmesa->hw.tcg.cmd[TCG_TEX_PROC_CTL_3] = 
      ((0 << R200_TEXGEN_0_INPUT_TEX_SHIFT) |
       (1 << R200_TEXGEN_1_INPUT_TEX_SHIFT) |
       (2 << R200_TEXGEN_2_INPUT_TEX_SHIFT) |
       (3 << R200_TEXGEN_3_INPUT_TEX_SHIFT) |
       (4 << R200_TEXGEN_4_INPUT_TEX_SHIFT) |
       (5 << R200_TEXGEN_5_INPUT_TEX_SHIFT)); 
   rmesa->hw.tcg.cmd[TCG_TEX_PROC_CTL_0] = 0; 
   rmesa->hw.tcg.cmd[TCG_TEX_PROC_CTL_1] =  
      ((0 << R200_TEXGEN_0_INPUT_SHIFT) |
       (1 << R200_TEXGEN_1_INPUT_SHIFT) |
       (2 << R200_TEXGEN_2_INPUT_SHIFT) |
       (3 << R200_TEXGEN_3_INPUT_SHIFT) |
       (4 << R200_TEXGEN_4_INPUT_SHIFT) |
       (5 << R200_TEXGEN_5_INPUT_SHIFT)); 
   rmesa->hw.tcg.cmd[TCG_TEX_CYL_WRAP_CTL] = 0;


   for (i = 0 ; i < 8; i++) {
      struct gl_light *l = &ctx->Light.Light[i];
      GLenum p = GL_LIGHT0 + i;
      *(float *)&(rmesa->hw.lit[i].cmd[LIT_RANGE_CUTOFF]) = FLT_MAX;

      ctx->Driver.Lightfv( ctx, p, GL_AMBIENT, l->Ambient );
      ctx->Driver.Lightfv( ctx, p, GL_DIFFUSE, l->Diffuse );
      ctx->Driver.Lightfv( ctx, p, GL_SPECULAR, l->Specular );
      ctx->Driver.Lightfv( ctx, p, GL_POSITION, NULL );
      ctx->Driver.Lightfv( ctx, p, GL_SPOT_DIRECTION, NULL );
      ctx->Driver.Lightfv( ctx, p, GL_SPOT_EXPONENT, &l->SpotExponent );
      ctx->Driver.Lightfv( ctx, p, GL_SPOT_CUTOFF, &l->SpotCutoff );
      ctx->Driver.Lightfv( ctx, p, GL_CONSTANT_ATTENUATION,
			   &l->ConstantAttenuation );
      ctx->Driver.Lightfv( ctx, p, GL_LINEAR_ATTENUATION, 
			   &l->LinearAttenuation );
      ctx->Driver.Lightfv( ctx, p, GL_QUADRATIC_ATTENUATION, 
			   &l->QuadraticAttenuation );
      *(float *)&(rmesa->hw.lit[i].cmd[LIT_ATTEN_XXX]) = 0.0;
   }

   ctx->Driver.LightModelfv( ctx, GL_LIGHT_MODEL_AMBIENT, 
			     ctx->Light.Model.Ambient );

   TNL_CONTEXT(ctx)->Driver.NotifyMaterialChange( ctx );

   for (i = 0 ; i < 6; i++) {
      ctx->Driver.ClipPlane( ctx, GL_CLIP_PLANE0 + i, NULL );
   }

   ctx->Driver.Fogfv( ctx, GL_FOG_MODE, NULL );
   ctx->Driver.Fogfv( ctx, GL_FOG_DENSITY, &ctx->Fog.Density );
   ctx->Driver.Fogfv( ctx, GL_FOG_START, &ctx->Fog.Start );
   ctx->Driver.Fogfv( ctx, GL_FOG_END, &ctx->Fog.End );
   ctx->Driver.Fogfv( ctx, GL_FOG_COLOR, ctx->Fog.Color );
   ctx->Driver.Fogfv( ctx, GL_FOG_COORDINATE_SOURCE_EXT, NULL );
   
   rmesa->hw.grd.cmd[GRD_VERT_GUARD_CLIP_ADJ] = IEEE_ONE;
   rmesa->hw.grd.cmd[GRD_VERT_GUARD_DISCARD_ADJ] = IEEE_ONE;
   rmesa->hw.grd.cmd[GRD_HORZ_GUARD_CLIP_ADJ] = IEEE_ONE;
   rmesa->hw.grd.cmd[GRD_HORZ_GUARD_DISCARD_ADJ] = IEEE_ONE;

   rmesa->hw.eye.cmd[EYE_X] = 0;
   rmesa->hw.eye.cmd[EYE_Y] = 0;
   rmesa->hw.eye.cmd[EYE_Z] = IEEE_ONE;
   rmesa->hw.eye.cmd[EYE_RESCALE_FACTOR] = IEEE_ONE;

   rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] =
      R200_PS_SE_SEL_STATE | R200_PS_MULT_CONST;

   /* ptp_eye is presumably used to calculate the attenuation wrt a different
      location? In any case, since point attenuation triggers _needeyecoords,
      it is constant. Probably ignored as long as R200_PS_USE_MODEL_EYE_VEC
      isn't set */
   rmesa->hw.ptp.cmd[PTP_EYE_X] = 0;
   rmesa->hw.ptp.cmd[PTP_EYE_Y] = 0;
   rmesa->hw.ptp.cmd[PTP_EYE_Z] = IEEE_ONE | 0x80000000; /* -1.0 */
   rmesa->hw.ptp.cmd[PTP_EYE_3] = 0;
   /* no idea what the ptp_vport_scale values are good for, except the
      PTSIZE one - hopefully doesn't matter */
   rmesa->hw.ptp.cmd[PTP_VPORT_SCALE_0] = IEEE_ONE;
   rmesa->hw.ptp.cmd[PTP_VPORT_SCALE_1] = IEEE_ONE;
   rmesa->hw.ptp.cmd[PTP_VPORT_SCALE_PTSIZE] = IEEE_ONE;
   rmesa->hw.ptp.cmd[PTP_VPORT_SCALE_3] = IEEE_ONE;
   rmesa->hw.ptp.cmd[PTP_ATT_CONST_QUAD] = 0;
   rmesa->hw.ptp.cmd[PTP_ATT_CONST_LIN] = 0;
   rmesa->hw.ptp.cmd[PTP_ATT_CONST_CON] = IEEE_ONE;
   rmesa->hw.ptp.cmd[PTP_ATT_CONST_3] = 0;
   rmesa->hw.ptp.cmd[PTP_CLAMP_MIN] = IEEE_ONE;
   rmesa->hw.ptp.cmd[PTP_CLAMP_MAX] = 0x44ffe000; /* 2047 */
   rmesa->hw.ptp.cmd[PTP_CLAMP_2] = 0;
   rmesa->hw.ptp.cmd[PTP_CLAMP_3] = 0;

   r200LightingSpaceChange( ctx );

   radeon_init_query_stateobj(&rmesa->radeon, R200_QUERYOBJ_CMDSIZE);
   rmesa->radeon.query.queryobj.cmd[R200_QUERYOBJ_CMD_0] = CP_PACKET0(RADEON_RB3D_ZPASS_DATA, 0);
   rmesa->radeon.query.queryobj.cmd[R200_QUERYOBJ_DATA_0] = 0;

   rmesa->radeon.hw.all_dirty = GL_TRUE;

   rcommonInitCmdBuf(&rmesa->radeon);
}
