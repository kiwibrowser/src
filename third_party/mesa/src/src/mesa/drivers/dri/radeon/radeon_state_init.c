/*
 * Copyright 2000, 2001 VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Gareth Hughes <gareth@valinux.com>
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/imports.h"
#include "main/api_arrayelt.h"

#include "swrast/swrast.h"
#include "vbo/vbo.h"
#include "tnl/t_pipeline.h"
#include "swrast_setup/swrast_setup.h"

#include "radeon_context.h"
#include "radeon_mipmap_tree.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "radeon_queryobj.h"

#include "../r200/r200_reg.h"

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
static int cmdpkt( r100ContextPtr rmesa, int id ) 
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

#define CHECK( NM, FLAG, ADD )				\
static int check_##NM( struct gl_context *ctx, struct radeon_state_atom *atom )	\
{							\
   return FLAG ? atom->cmd_size + (ADD) : 0;			\
}

#define TCL_CHECK( NM, FLAG, ADD )				\
static int check_##NM( struct gl_context *ctx, struct radeon_state_atom *atom )	\
{							\
   r100ContextPtr rmesa = R100_CONTEXT(ctx);	\
   return (!rmesa->radeon.TclFallback && (FLAG)) ? atom->cmd_size + (ADD) : 0;	\
}


CHECK( always, GL_TRUE, 0 )
CHECK( always_add2, GL_TRUE, 2 )
CHECK( always_add4, GL_TRUE, 4 )
CHECK( tex0_mm, GL_TRUE, 3 )
CHECK( tex1_mm, GL_TRUE, 3 )
/* need this for the cubic_map on disabled unit 2 bug, maybe r100 only? */
CHECK( tex2_mm, GL_TRUE, 3 )
CHECK( cube0_mm, (ctx->Texture.Unit[0]._ReallyEnabled & TEXTURE_CUBE_BIT), 2 + 4*5 - CUBE_STATE_SIZE )
CHECK( cube1_mm, (ctx->Texture.Unit[1]._ReallyEnabled & TEXTURE_CUBE_BIT), 2 + 4*5 - CUBE_STATE_SIZE )
CHECK( cube2_mm, (ctx->Texture.Unit[2]._ReallyEnabled & TEXTURE_CUBE_BIT), 2 + 4*5 - CUBE_STATE_SIZE )
CHECK( fog_add4, ctx->Fog.Enabled, 4 )
TCL_CHECK( tcl_add4, GL_TRUE, 4 )
TCL_CHECK( tcl_tex0_add4, ctx->Texture.Unit[0]._ReallyEnabled, 4 )
TCL_CHECK( tcl_tex1_add4, ctx->Texture.Unit[1]._ReallyEnabled, 4 )
TCL_CHECK( tcl_tex2_add4, ctx->Texture.Unit[2]._ReallyEnabled, 4 )
TCL_CHECK( tcl_lighting, ctx->Light.Enabled, 0 )
TCL_CHECK( tcl_lighting_add4, ctx->Light.Enabled, 4 )
TCL_CHECK( tcl_eyespace_or_lighting_add4, ctx->_NeedEyeCoords || ctx->Light.Enabled, 4 )
TCL_CHECK( tcl_lit0_add6, ctx->Light.Enabled && ctx->Light.Light[0].Enabled, 6 )
TCL_CHECK( tcl_lit1_add6, ctx->Light.Enabled && ctx->Light.Light[1].Enabled, 6 )
TCL_CHECK( tcl_lit2_add6, ctx->Light.Enabled && ctx->Light.Light[2].Enabled, 6 )
TCL_CHECK( tcl_lit3_add6, ctx->Light.Enabled && ctx->Light.Light[3].Enabled, 6 )
TCL_CHECK( tcl_lit4_add6, ctx->Light.Enabled && ctx->Light.Light[4].Enabled, 6 )
TCL_CHECK( tcl_lit5_add6, ctx->Light.Enabled && ctx->Light.Light[5].Enabled, 6 )
TCL_CHECK( tcl_lit6_add6, ctx->Light.Enabled && ctx->Light.Light[6].Enabled, 6 )
TCL_CHECK( tcl_lit7_add6, ctx->Light.Enabled && ctx->Light.Light[7].Enabled, 6 )
TCL_CHECK( tcl_ucp0_add4, (ctx->Transform.ClipPlanesEnabled & 0x1), 4 )
TCL_CHECK( tcl_ucp1_add4, (ctx->Transform.ClipPlanesEnabled & 0x2), 4 )
TCL_CHECK( tcl_ucp2_add4, (ctx->Transform.ClipPlanesEnabled & 0x4), 4 )
TCL_CHECK( tcl_ucp3_add4, (ctx->Transform.ClipPlanesEnabled & 0x8), 4 )
TCL_CHECK( tcl_ucp4_add4, (ctx->Transform.ClipPlanesEnabled & 0x10), 4 )
TCL_CHECK( tcl_ucp5_add4, (ctx->Transform.ClipPlanesEnabled & 0x20), 4 )
TCL_CHECK( tcl_eyespace_or_fog_add4, ctx->_NeedEyeCoords || ctx->Fog.Enabled, 4 )

CHECK( txr0, (ctx->Texture.Unit[0]._ReallyEnabled & TEXTURE_RECT_BIT), 0 )
CHECK( txr1, (ctx->Texture.Unit[1]._ReallyEnabled & TEXTURE_RECT_BIT), 0 )
CHECK( txr2, (ctx->Texture.Unit[2]._ReallyEnabled & TEXTURE_RECT_BIT), 0 )

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

#define OUT_SCL(hdr, data) do {					\
    drm_radeon_cmd_header_t h;						\
    h.i = hdr;								\
    OUT_BATCH(CP_PACKET0(R200_SE_TCL_SCALAR_INDX_REG, 0));		\
    OUT_BATCH((h.scalars.offset) | (h.scalars.stride << RADEON_SCAL_INDX_DWORD_STRIDE_SHIFT)); \
    OUT_BATCH(CP_PACKET0_ONE(R200_SE_TCL_SCALAR_DATA_REG, h.scalars.count - 1));	\
    OUT_BATCH_TABLE((data), h.scalars.count);				\
  } while(0)

static void scl_emit(struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r100ContextPtr r100 = R100_CONTEXT(ctx);
   BATCH_LOCALS(&r100->radeon);
   uint32_t dwords = atom->check(ctx, atom);
   
   BEGIN_BATCH_NO_AUTOSTATE(dwords);
   OUT_SCL(atom->cmd[0], atom->cmd+1);
   END_BATCH();
}


static void vec_emit(struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r100ContextPtr r100 = R100_CONTEXT(ctx);
   BATCH_LOCALS(&r100->radeon);
   uint32_t dwords = atom->check(ctx, atom);

   BEGIN_BATCH_NO_AUTOSTATE(dwords);
   OUT_VEC(atom->cmd[0], atom->cmd+1);
   END_BATCH();
}


static void lit_emit(struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r100ContextPtr r100 = R100_CONTEXT(ctx);
   BATCH_LOCALS(&r100->radeon);
   uint32_t dwords = atom->check(ctx, atom);

   BEGIN_BATCH_NO_AUTOSTATE(dwords);
   OUT_VEC(atom->cmd[LIT_CMD_0], atom->cmd+1);
   OUT_SCL(atom->cmd[LIT_CMD_1], atom->cmd+LIT_CMD_1+1);
   END_BATCH();
}

static int check_always_ctx( struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r100ContextPtr r100 = R100_CONTEXT(ctx);
   struct radeon_renderbuffer *rrb, *drb;
   uint32_t dwords;

   rrb = radeon_get_colorbuffer(&r100->radeon);
   if (!rrb || !rrb->bo) {
      return 0;
   }

   drb = radeon_get_depthbuffer(&r100->radeon);

   dwords = 10;
   if (drb)
     dwords += 6;
   if (rrb)
     dwords += 8;

   return dwords;
}

static void ctx_emit_cs(struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r100ContextPtr r100 = R100_CONTEXT(ctx);
   BATCH_LOCALS(&r100->radeon);
   struct radeon_renderbuffer *rrb, *drb;
   uint32_t cbpitch = 0;
   uint32_t zbpitch = 0;
   uint32_t dwords = atom->check(ctx, atom);
   uint32_t depth_fmt;

   rrb = radeon_get_colorbuffer(&r100->radeon);
   if (!rrb || !rrb->bo) {
      fprintf(stderr, "no rrb\n");
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
	_mesa_problem(ctx, "unexpected format in ctx_emit_cs()");
   }

   cbpitch = (rrb->pitch / rrb->cpp);
   if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE)
       cbpitch |= R200_COLOR_TILE_ENABLE;
   if (rrb->bo->flags & RADEON_BO_FLAGS_MICRO_TILE)
       cbpitch |= RADEON_COLOR_MICROTILE_ENABLE;

   drb = radeon_get_depthbuffer(&r100->radeon);
   if (drb) {
     zbpitch = (drb->pitch / drb->cpp);
     if (drb->cpp == 4)
        depth_fmt = RADEON_DEPTH_FORMAT_24BIT_INT_Z;
     else
        depth_fmt = RADEON_DEPTH_FORMAT_16BIT_INT_Z;
     atom->cmd[CTX_RB3D_ZSTENCILCNTL] &= ~RADEON_DEPTH_FORMAT_MASK;
     atom->cmd[CTX_RB3D_ZSTENCILCNTL] |= depth_fmt;
     
   }

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

   // if (atom->cmd_size == CTX_STATE_SIZE_NEWDRM) {
   //   OUT_BATCH_TABLE((atom->cmd + 14), 4);
   // }

   END_BATCH();
   BEGIN_BATCH_NO_AUTOSTATE(4);
   OUT_BATCH(CP_PACKET0(RADEON_RE_TOP_LEFT, 0));
   OUT_BATCH(0);
   OUT_BATCH(CP_PACKET0(RADEON_RE_WIDTH_HEIGHT, 0));
   if (rrb) {
       OUT_BATCH(((rrb->base.Base.Width - 1) << RADEON_RE_WIDTH_SHIFT) |
                 ((rrb->base.Base.Height - 1) << RADEON_RE_HEIGHT_SHIFT));
   } else {
       OUT_BATCH(0);
   }
   END_BATCH();
}

static void cube_emit_cs(struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r100ContextPtr r100 = R100_CONTEXT(ctx);
   BATCH_LOCALS(&r100->radeon);
   uint32_t dwords = atom->check(ctx, atom);
   int i = atom->idx, j;
   radeonTexObj *t = r100->state.texture.unit[i].texobj;
   radeon_mipmap_level *lvl;
   uint32_t base_reg;

   if (!(ctx->Texture.Unit[i]._ReallyEnabled & TEXTURE_CUBE_BIT))
	return;

   if (!t)
	return;

   if (!t->mt)
	return;

   switch(i) {
	case 1: base_reg = RADEON_PP_CUBIC_OFFSET_T1_0; break;
	case 2: base_reg = RADEON_PP_CUBIC_OFFSET_T2_0; break;
	default:
	case 0: base_reg = RADEON_PP_CUBIC_OFFSET_T0_0; break;
   };
   BEGIN_BATCH_NO_AUTOSTATE(dwords);
   OUT_BATCH_TABLE(atom->cmd, 2);
   lvl = &t->mt->levels[0];
   for (j = 0; j < 5; j++) {
	OUT_BATCH(CP_PACKET0(base_reg + (4 * j), 0));
	OUT_BATCH_RELOC(lvl->faces[j].offset, t->mt->bo, lvl->faces[j].offset,
			RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
   }
   END_BATCH();
}

static void tex_emit_cs(struct gl_context *ctx, struct radeon_state_atom *atom)
{
   r100ContextPtr r100 = R100_CONTEXT(ctx);
   BATCH_LOCALS(&r100->radeon);
   uint32_t dwords = atom->cmd_size;
   int i = atom->idx;
   radeonTexObj *t = r100->state.texture.unit[i].texobj;
   radeon_mipmap_level *lvl;
   int hastexture = 1;

   if (!t)
	hastexture = 0;
   else {
	if (!t->mt && !t->bo)
		hastexture = 0;
   }
   dwords += 1;
   if (hastexture)
     dwords += 2;
   else
     dwords -= 2;
   BEGIN_BATCH_NO_AUTOSTATE(dwords);

   OUT_BATCH(CP_PACKET0(RADEON_PP_TXFILTER_0 + (24 * i), 1));
   OUT_BATCH_TABLE((atom->cmd + 1), 2);

   if (hastexture) {
     OUT_BATCH(CP_PACKET0(RADEON_PP_TXOFFSET_0 + (24 * i), 0));
     if (t->mt && !t->image_override) {
        if ((ctx->Texture.Unit[i]._ReallyEnabled & TEXTURE_CUBE_BIT)) {
            lvl = &t->mt->levels[t->minLod];
	    OUT_BATCH_RELOC(lvl->faces[5].offset, t->mt->bo, lvl->faces[5].offset,
			RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
        } else {
           OUT_BATCH_RELOC(t->tile_bits, t->mt->bo, get_base_teximage_offset(t),
		     RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
        }
      } else {
	if (t->bo)
            OUT_BATCH_RELOC(t->tile_bits, t->bo, 0,
                            RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
      }
   }

   OUT_BATCH(CP_PACKET0(RADEON_PP_TXCBLEND_0 + (i * 24), 1));
   OUT_BATCH_TABLE((atom->cmd+4), 2);
   OUT_BATCH(CP_PACKET0(RADEON_PP_BORDER_COLOR_0 + (i * 4), 0));
   OUT_BATCH((atom->cmd[TEX_PP_BORDER_COLOR]));
   END_BATCH();
}

/* Initialize the context's hardware state.
 */
void radeonInitState( r100ContextPtr rmesa )
{
   struct gl_context *ctx = rmesa->radeon.glCtx;
   GLuint i;

   rmesa->radeon.Fallback = 0;


   rmesa->radeon.hw.max_state_size = 0;

#define ALLOC_STATE_IDX( ATOM, CHK, SZ, NM, FLAG, IDX )		\
   do {								\
      rmesa->hw.ATOM.cmd_size = SZ;				\
      rmesa->hw.ATOM.cmd = (GLuint *)CALLOC(SZ * sizeof(int));	\
      rmesa->hw.ATOM.lastcmd = (GLuint *)CALLOC(SZ * sizeof(int)); \
      rmesa->hw.ATOM.name = NM;						\
      rmesa->hw.ATOM.is_tcl = FLAG;					\
      rmesa->hw.ATOM.check = check_##CHK;				\
      rmesa->hw.ATOM.dirty = GL_TRUE;					\
      rmesa->hw.ATOM.idx = IDX;					\
      rmesa->radeon.hw.max_state_size += SZ * sizeof(int);		\
   } while (0)

#define ALLOC_STATE( ATOM, CHK, SZ, NM, FLAG )		\
   ALLOC_STATE_IDX(ATOM, CHK, SZ, NM, FLAG, 0)

   /* Allocate state buffers:
    */
   ALLOC_STATE( ctx, always_add4, CTX_STATE_SIZE, "CTX/context", 0 );
   rmesa->hw.ctx.emit = ctx_emit_cs;
   rmesa->hw.ctx.check = check_always_ctx;
   ALLOC_STATE( lin, always, LIN_STATE_SIZE, "LIN/line", 0 );
   ALLOC_STATE( msk, always, MSK_STATE_SIZE, "MSK/mask", 0 );
   ALLOC_STATE( vpt, always, VPT_STATE_SIZE, "VPT/viewport", 0 );
   ALLOC_STATE( set, always, SET_STATE_SIZE, "SET/setup", 0 );
   ALLOC_STATE( msc, always, MSC_STATE_SIZE, "MSC/misc", 0 );
   ALLOC_STATE( zbs, always, ZBS_STATE_SIZE, "ZBS/zbias", 0 );
   ALLOC_STATE( tcl, always, TCL_STATE_SIZE, "TCL/tcl", 1 );
   ALLOC_STATE( mtl, tcl_lighting, MTL_STATE_SIZE, "MTL/material", 1 );
   ALLOC_STATE( grd, always_add2, GRD_STATE_SIZE, "GRD/guard-band", 1 );
   ALLOC_STATE( fog, fog_add4, FOG_STATE_SIZE, "FOG/fog", 1 );
   ALLOC_STATE( glt, tcl_lighting_add4, GLT_STATE_SIZE, "GLT/light-global", 1 );
   ALLOC_STATE( eye, tcl_lighting_add4, EYE_STATE_SIZE, "EYE/eye-vector", 1 );
   ALLOC_STATE_IDX( tex[0], tex0_mm, TEX_STATE_SIZE, "TEX/tex-0", 0, 0);
   ALLOC_STATE_IDX( tex[1], tex1_mm, TEX_STATE_SIZE, "TEX/tex-1", 0, 1);
   ALLOC_STATE_IDX( tex[2], tex2_mm, TEX_STATE_SIZE, "TEX/tex-2", 0, 2);
   ALLOC_STATE( mat[0], tcl_add4, MAT_STATE_SIZE, "MAT/modelproject", 1 );
   ALLOC_STATE( mat[1], tcl_eyespace_or_fog_add4, MAT_STATE_SIZE, "MAT/modelview", 1 );
   ALLOC_STATE( mat[2], tcl_eyespace_or_lighting_add4, MAT_STATE_SIZE, "MAT/it-modelview", 1 );
   ALLOC_STATE( mat[3], tcl_tex0_add4, MAT_STATE_SIZE, "MAT/texmat0", 1 );
   ALLOC_STATE( mat[4], tcl_tex1_add4, MAT_STATE_SIZE, "MAT/texmat1", 1 );
   ALLOC_STATE( mat[5], tcl_tex2_add4, MAT_STATE_SIZE, "MAT/texmat2", 1 );
   ALLOC_STATE( lit[0], tcl_lit0_add6, LIT_STATE_SIZE, "LIT/light-0", 1 );
   ALLOC_STATE( lit[1], tcl_lit1_add6, LIT_STATE_SIZE, "LIT/light-1", 1 );
   ALLOC_STATE( lit[2], tcl_lit2_add6, LIT_STATE_SIZE, "LIT/light-2", 1 );
   ALLOC_STATE( lit[3], tcl_lit3_add6, LIT_STATE_SIZE, "LIT/light-3", 1 );
   ALLOC_STATE( lit[4], tcl_lit4_add6, LIT_STATE_SIZE, "LIT/light-4", 1 );
   ALLOC_STATE( lit[5], tcl_lit5_add6, LIT_STATE_SIZE, "LIT/light-5", 1 );
   ALLOC_STATE( lit[6], tcl_lit6_add6, LIT_STATE_SIZE, "LIT/light-6", 1 );
   ALLOC_STATE( lit[7], tcl_lit7_add6, LIT_STATE_SIZE, "LIT/light-7", 1 );
   ALLOC_STATE( ucp[0], tcl_ucp0_add4, UCP_STATE_SIZE, "UCP/userclip-0", 1 );
   ALLOC_STATE( ucp[1], tcl_ucp1_add4, UCP_STATE_SIZE, "UCP/userclip-1", 1 );
   ALLOC_STATE( ucp[2], tcl_ucp2_add4, UCP_STATE_SIZE, "UCP/userclip-2", 1 );
   ALLOC_STATE( ucp[3], tcl_ucp3_add4, UCP_STATE_SIZE, "UCP/userclip-3", 1 );
   ALLOC_STATE( ucp[4], tcl_ucp4_add4, UCP_STATE_SIZE, "UCP/userclip-4", 1 );
   ALLOC_STATE( ucp[5], tcl_ucp5_add4, UCP_STATE_SIZE, "UCP/userclip-5", 1 );
   ALLOC_STATE( stp, always, STP_STATE_SIZE, "STP/stp", 0 );

   for (i = 0; i < 3; i++) {
      rmesa->hw.tex[i].emit = tex_emit_cs;
   }
   ALLOC_STATE_IDX( cube[0], cube0_mm, CUBE_STATE_SIZE, "CUBE/cube-0", 0, 0 );
   ALLOC_STATE_IDX( cube[1], cube1_mm, CUBE_STATE_SIZE, "CUBE/cube-1", 0, 1 );
   ALLOC_STATE_IDX( cube[2], cube2_mm, CUBE_STATE_SIZE, "CUBE/cube-2", 0, 2 );
   for (i = 0; i < 3; i++)
       rmesa->hw.cube[i].emit = cube_emit_cs;

   ALLOC_STATE_IDX( txr[0], txr0, TXR_STATE_SIZE, "TXR/txr-0", 0, 0 );
   ALLOC_STATE_IDX( txr[1], txr1, TXR_STATE_SIZE, "TXR/txr-1", 0, 1 );
   ALLOC_STATE_IDX( txr[2], txr2, TXR_STATE_SIZE, "TXR/txr-2", 0, 2 );

   radeonSetUpAtomList( rmesa );

   /* Fill in the packet headers:
    */
   rmesa->hw.ctx.cmd[CTX_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_PP_MISC);
   rmesa->hw.ctx.cmd[CTX_CMD_1] = cmdpkt(rmesa, RADEON_EMIT_PP_CNTL);
   rmesa->hw.ctx.cmd[CTX_CMD_2] = cmdpkt(rmesa, RADEON_EMIT_RB3D_COLORPITCH);
   rmesa->hw.lin.cmd[LIN_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_RE_LINE_PATTERN);
   rmesa->hw.lin.cmd[LIN_CMD_1] = cmdpkt(rmesa, RADEON_EMIT_SE_LINE_WIDTH);
   rmesa->hw.msk.cmd[MSK_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_RB3D_STENCILREFMASK);
   rmesa->hw.vpt.cmd[VPT_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_SE_VPORT_XSCALE);
   rmesa->hw.set.cmd[SET_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_SE_CNTL);
   rmesa->hw.set.cmd[SET_CMD_1] = cmdpkt(rmesa, RADEON_EMIT_SE_CNTL_STATUS);
   rmesa->hw.msc.cmd[MSC_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_RE_MISC);
   rmesa->hw.tex[0].cmd[TEX_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_PP_TXFILTER_0);
   rmesa->hw.tex[0].cmd[TEX_CMD_1] = cmdpkt(rmesa, RADEON_EMIT_PP_BORDER_COLOR_0);
   rmesa->hw.tex[1].cmd[TEX_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_PP_TXFILTER_1);
   rmesa->hw.tex[1].cmd[TEX_CMD_1] = cmdpkt(rmesa, RADEON_EMIT_PP_BORDER_COLOR_1);
   rmesa->hw.tex[2].cmd[TEX_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_PP_TXFILTER_2);
   rmesa->hw.tex[2].cmd[TEX_CMD_1] = cmdpkt(rmesa, RADEON_EMIT_PP_BORDER_COLOR_2);
   rmesa->hw.cube[0].cmd[CUBE_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_PP_CUBIC_FACES_0);
   rmesa->hw.cube[0].cmd[CUBE_CMD_1] = cmdpkt(rmesa, RADEON_EMIT_PP_CUBIC_OFFSETS_T0);
   rmesa->hw.cube[1].cmd[CUBE_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_PP_CUBIC_FACES_1);
   rmesa->hw.cube[1].cmd[CUBE_CMD_1] = cmdpkt(rmesa, RADEON_EMIT_PP_CUBIC_OFFSETS_T1);
   rmesa->hw.cube[2].cmd[CUBE_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_PP_CUBIC_FACES_2);
   rmesa->hw.cube[2].cmd[CUBE_CMD_1] = cmdpkt(rmesa, RADEON_EMIT_PP_CUBIC_OFFSETS_T2);
   rmesa->hw.zbs.cmd[ZBS_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_SE_ZBIAS_FACTOR);
   rmesa->hw.tcl.cmd[TCL_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_SE_TCL_OUTPUT_VTX_FMT);
   rmesa->hw.mtl.cmd[MTL_CMD_0] = 
      cmdpkt(rmesa, RADEON_EMIT_SE_TCL_MATERIAL_EMMISSIVE_RED);
   rmesa->hw.txr[0].cmd[TXR_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_PP_TEX_SIZE_0);
   rmesa->hw.txr[1].cmd[TXR_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_PP_TEX_SIZE_1);
   rmesa->hw.txr[2].cmd[TXR_CMD_0] = cmdpkt(rmesa, RADEON_EMIT_PP_TEX_SIZE_2);
   rmesa->hw.grd.cmd[GRD_CMD_0] = 
      cmdscl( RADEON_SS_VERT_GUARD_CLIP_ADJ_ADDR, 1, 4 );
   rmesa->hw.fog.cmd[FOG_CMD_0] = 
      cmdvec( RADEON_VS_FOG_PARAM_ADDR, 1, 4 );
   rmesa->hw.glt.cmd[GLT_CMD_0] = 
      cmdvec( RADEON_VS_GLOBAL_AMBIENT_ADDR, 1, 4 );
   rmesa->hw.eye.cmd[EYE_CMD_0] = 
      cmdvec( RADEON_VS_EYE_VECTOR_ADDR, 1, 4 );

   for (i = 0 ; i < 6; i++) {
      rmesa->hw.mat[i].cmd[MAT_CMD_0] = 
	 cmdvec( RADEON_VS_MATRIX_0_ADDR + i*4, 1, 16);
   }

   for (i = 0 ; i < 8; i++) {
      rmesa->hw.lit[i].cmd[LIT_CMD_0] = 
	 cmdvec( RADEON_VS_LIGHT_AMBIENT_ADDR + i, 8, 24 );
      rmesa->hw.lit[i].cmd[LIT_CMD_1] = 
	 cmdscl( RADEON_SS_LIGHT_DCD_ADDR + i, 8, 6 );
   }

   for (i = 0 ; i < 6; i++) {
      rmesa->hw.ucp[i].cmd[UCP_CMD_0] = 
	 cmdvec( RADEON_VS_UCP_ADDR + i, 1, 4 );
   }

   rmesa->hw.stp.cmd[STP_CMD_0] = CP_PACKET0(RADEON_RE_STIPPLE_ADDR, 0);
   rmesa->hw.stp.cmd[STP_DATA_0] = 0;
   rmesa->hw.stp.cmd[STP_CMD_1] = CP_PACKET0_ONE(RADEON_RE_STIPPLE_DATA, 31);

   rmesa->hw.grd.emit = scl_emit;
   rmesa->hw.fog.emit = vec_emit;
   rmesa->hw.glt.emit = vec_emit;
   rmesa->hw.eye.emit = vec_emit;
   for (i = 0; i < 6; i++)
      rmesa->hw.mat[i].emit = vec_emit;

   for (i = 0; i < 8; i++)
      rmesa->hw.lit[i].emit = lit_emit;

   for (i = 0; i < 6; i++)
      rmesa->hw.ucp[i].emit = vec_emit;

   rmesa->last_ReallyEnabled = -1;

   /* Initial Harware state:
    */
   rmesa->hw.ctx.cmd[CTX_PP_MISC] = (RADEON_ALPHA_TEST_PASS |
				     RADEON_CHROMA_FUNC_FAIL |
				     RADEON_CHROMA_KEY_NEAREST |
				     RADEON_SHADOW_FUNC_EQUAL |
				     RADEON_SHADOW_PASS_1 /*|
				     RADEON_RIGHT_HAND_CUBE_OGL */);

   rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] = (RADEON_FOG_VERTEX |
					  /* this bit unused for vertex fog */
					  RADEON_FOG_USE_DEPTH);

   rmesa->hw.ctx.cmd[CTX_RE_SOLID_COLOR] = 0x00000000;

   rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCNTL] = (RADEON_COMB_FCN_ADD_CLAMP |
					    RADEON_SRC_BLEND_GL_ONE |
					    RADEON_DST_BLEND_GL_ZERO );

   rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] = (RADEON_Z_TEST_LESS |
					       RADEON_STENCIL_TEST_ALWAYS |
					       RADEON_STENCIL_FAIL_KEEP |
					       RADEON_STENCIL_ZPASS_KEEP |
					       RADEON_STENCIL_ZFAIL_KEEP |
					       RADEON_Z_WRITE_ENABLE);

   if (rmesa->using_hyperz) {
       rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_Z_COMPRESSION_ENABLE |
						   RADEON_Z_DECOMPRESSION_ENABLE;
      if (rmesa->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL) {
	 /* works for q3, but slight rendering errors with glxgears ? */
/*	 rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_Z_HIERARCHY_ENABLE;*/
	 /* need this otherwise get lots of lockups with q3 ??? */
	 rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_FORCE_Z_DIRTY;
      } 
   }

   rmesa->hw.ctx.cmd[CTX_PP_CNTL] = (RADEON_SCISSOR_ENABLE |
				     RADEON_ANTI_ALIAS_NONE);

   rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] = (RADEON_PLANE_MASK_ENABLE |
				       RADEON_ZBLOCK16);

   switch ( driQueryOptioni( &rmesa->radeon.optionCache, "dither_mode" ) ) {
   case DRI_CONF_DITHER_XERRORDIFFRESET:
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |= RADEON_DITHER_INIT;
      break;
   case DRI_CONF_DITHER_ORDERED:
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |= RADEON_SCALE_DITHER_ENABLE;
      break;
   }
   if ( driQueryOptioni( &rmesa->radeon.optionCache, "round_mode" ) ==
	DRI_CONF_ROUND_ROUND )
      rmesa->radeon.state.color.roundEnable = RADEON_ROUND_ENABLE;
   else
      rmesa->radeon.state.color.roundEnable = 0;
   if ( driQueryOptioni (&rmesa->radeon.optionCache, "color_reduction" ) ==
	DRI_CONF_COLOR_REDUCTION_DITHER )
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |= RADEON_DITHER_ENABLE;
   else
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |= rmesa->radeon.state.color.roundEnable;


   rmesa->hw.set.cmd[SET_SE_CNTL] = (RADEON_FFACE_CULL_CCW |
				     RADEON_BFACE_SOLID |
				     RADEON_FFACE_SOLID |
/*  			     RADEON_BADVTX_CULL_DISABLE | */
				     RADEON_FLAT_SHADE_VTX_LAST |
				     RADEON_DIFFUSE_SHADE_GOURAUD |
				     RADEON_ALPHA_SHADE_GOURAUD |
				     RADEON_SPECULAR_SHADE_GOURAUD |
				     RADEON_FOG_SHADE_GOURAUD |
				     RADEON_VPORT_XY_XFORM_ENABLE |
				     RADEON_VPORT_Z_XFORM_ENABLE |
				     RADEON_VTX_PIX_CENTER_OGL |
				     RADEON_ROUND_MODE_TRUNC |
				     RADEON_ROUND_PREC_8TH_PIX);

   rmesa->hw.set.cmd[SET_SE_CNTL_STATUS] =
#ifdef MESA_BIG_ENDIAN
					    RADEON_VC_32BIT_SWAP;
#else
  					    RADEON_VC_NO_SWAP;
#endif

   if (!(rmesa->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL)) {
     rmesa->hw.set.cmd[SET_SE_CNTL_STATUS] |= RADEON_TCL_BYPASS;
   }

   rmesa->hw.set.cmd[SET_SE_COORDFMT] = (
      RADEON_VTX_W0_IS_NOT_1_OVER_W0 |
      RADEON_TEX1_W_ROUTING_USE_Q1);


   rmesa->hw.lin.cmd[LIN_RE_LINE_PATTERN] = ((1 << 16) | 0xffff);

   rmesa->hw.lin.cmd[LIN_RE_LINE_STATE] = 
      ((0 << RADEON_LINE_CURRENT_PTR_SHIFT) |
       (1 << RADEON_LINE_CURRENT_COUNT_SHIFT));

   rmesa->hw.lin.cmd[LIN_SE_LINE_WIDTH] = (1 << 4);

   rmesa->hw.msk.cmd[MSK_RB3D_STENCILREFMASK] = 
      ((0x00 << RADEON_STENCIL_REF_SHIFT) |
       (0xff << RADEON_STENCIL_MASK_SHIFT) |
       (0xff << RADEON_STENCIL_WRITEMASK_SHIFT));

   rmesa->hw.msk.cmd[MSK_RB3D_ROPCNTL] = RADEON_ROP_COPY;
   rmesa->hw.msk.cmd[MSK_RB3D_PLANEMASK] = 0xffffffff;

   rmesa->hw.msc.cmd[MSC_RE_MISC] = 
      ((0 << RADEON_STIPPLE_X_OFFSET_SHIFT) |
       (0 << RADEON_STIPPLE_Y_OFFSET_SHIFT) |
       RADEON_STIPPLE_BIG_BIT_ORDER);

   rmesa->hw.vpt.cmd[VPT_SE_VPORT_XSCALE]  = 0x00000000;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_XOFFSET] = 0x00000000;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_YSCALE]  = 0x00000000;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_YOFFSET] = 0x00000000;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_ZSCALE]  = 0x00000000;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_ZOFFSET] = 0x00000000;

   for ( i = 0 ; i < ctx->Const.MaxTextureUnits ; i++ ) {
      rmesa->hw.tex[i].cmd[TEX_PP_TXFILTER] = RADEON_BORDER_MODE_OGL;
      rmesa->hw.tex[i].cmd[TEX_PP_TXFORMAT] = 
	  (RADEON_TXFORMAT_ENDIAN_NO_SWAP |
	   RADEON_TXFORMAT_PERSPECTIVE_ENABLE |
	   (i << 24) | /* This is one of RADEON_TXFORMAT_ST_ROUTE_STQ[012] */
	   (2 << RADEON_TXFORMAT_WIDTH_SHIFT) |
	   (2 << RADEON_TXFORMAT_HEIGHT_SHIFT));

      /* Initialize the texture offset to the start of the card texture heap */
      //      rmesa->hw.tex[i].cmd[TEX_PP_TXOFFSET] =
      //	  rmesa->radeon.radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];

      rmesa->hw.tex[i].cmd[TEX_PP_BORDER_COLOR] = 0;
      rmesa->hw.tex[i].cmd[TEX_PP_TXCBLEND] =  
	  (RADEON_COLOR_ARG_A_ZERO |
	   RADEON_COLOR_ARG_B_ZERO |
	   RADEON_COLOR_ARG_C_CURRENT_COLOR |
	   RADEON_BLEND_CTL_ADD |
	   RADEON_SCALE_1X |
	   RADEON_CLAMP_TX);
      rmesa->hw.tex[i].cmd[TEX_PP_TXABLEND] = 
	  (RADEON_ALPHA_ARG_A_ZERO |
	   RADEON_ALPHA_ARG_B_ZERO |
	   RADEON_ALPHA_ARG_C_CURRENT_ALPHA |
	   RADEON_BLEND_CTL_ADD |
	   RADEON_SCALE_1X |
	   RADEON_CLAMP_TX);
      rmesa->hw.tex[i].cmd[TEX_PP_TFACTOR] = 0;

      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_FACES] = 0;
      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_OFFSET_0] =
	  rmesa->radeon.radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];
      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_OFFSET_1] =
	  rmesa->radeon.radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];
      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_OFFSET_2] =
	  rmesa->radeon.radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];
      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_OFFSET_3] =
	  rmesa->radeon.radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];
      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_OFFSET_4] =
	  rmesa->radeon.radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];
   }

   /* Can only add ST1 at the time of doing some multitex but can keep
    * it after that.  Errors if DIFFUSE is missing.
    */
   rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] = 
      (RADEON_TCL_VTX_Z0 |
       RADEON_TCL_VTX_W0 |
       RADEON_TCL_VTX_PK_DIFFUSE
	 );	/* need to keep this uptodate */
						   
   rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] =
      ( RADEON_TCL_COMPUTE_XYZW 	|
	(RADEON_TCL_TEX_INPUT_TEX_0 << RADEON_TCL_TEX_0_OUTPUT_SHIFT) |
	(RADEON_TCL_TEX_INPUT_TEX_1 << RADEON_TCL_TEX_1_OUTPUT_SHIFT) |
	(RADEON_TCL_TEX_INPUT_TEX_2 << RADEON_TCL_TEX_2_OUTPUT_SHIFT));


   /* XXX */
   rmesa->hw.tcl.cmd[TCL_MATRIX_SELECT_0] = 
      ((MODEL << RADEON_MODELVIEW_0_SHIFT) |
       (MODEL_IT << RADEON_IT_MODELVIEW_0_SHIFT));

   rmesa->hw.tcl.cmd[TCL_MATRIX_SELECT_1] = 
      ((MODEL_PROJ << RADEON_MODELPROJECT_0_SHIFT) |
       (TEXMAT_0 << RADEON_TEXMAT_0_SHIFT) |
       (TEXMAT_1 << RADEON_TEXMAT_1_SHIFT) |
       (TEXMAT_2 << RADEON_TEXMAT_2_SHIFT));

   rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] = 
      (RADEON_UCP_IN_CLIP_SPACE |
       RADEON_CULL_FRONT_IS_CCW);

   rmesa->hw.tcl.cmd[TCL_TEXTURE_PROC_CTL] = 0; 

   rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] = 
      (RADEON_SPECULAR_LIGHTS |
       RADEON_DIFFUSE_SPECULAR_COMBINE |
       RADEON_LOCAL_LIGHT_VEC_GL |
       (RADEON_LM_SOURCE_STATE_MULT << RADEON_EMISSIVE_SOURCE_SHIFT) |
       (RADEON_LM_SOURCE_STATE_MULT << RADEON_AMBIENT_SOURCE_SHIFT) |
       (RADEON_LM_SOURCE_STATE_MULT << RADEON_DIFFUSE_SOURCE_SHIFT) |
       (RADEON_LM_SOURCE_STATE_MULT << RADEON_SPECULAR_SOURCE_SHIFT));

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

   radeon_init_query_stateobj(&rmesa->radeon, R100_QUERYOBJ_CMDSIZE);
   rmesa->radeon.query.queryobj.cmd[R100_QUERYOBJ_CMD_0] = CP_PACKET0(RADEON_RB3D_ZPASS_DATA, 0);
   rmesa->radeon.query.queryobj.cmd[R100_QUERYOBJ_DATA_0] = 0;
     
   rmesa->radeon.hw.all_dirty = GL_TRUE;

   rcommonInitCmdBuf(&rmesa->radeon);
}
