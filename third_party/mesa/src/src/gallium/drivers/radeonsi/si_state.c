/*
 * Copyright 2012 Advanced Micro Devices, Inc.
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
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Christian König <christian.koenig@amd.com>
 */

#include "util/u_memory.h"
#include "util/u_framebuffer.h"
#include "util/u_blitter.h"
#include "util/u_pack_color.h"
#include "tgsi/tgsi_parse.h"
#include "radeonsi_pipe.h"
#include "radeonsi_shader.h"
#include "si_state.h"
#include "sid.h"

/*
 * inferred framebuffer and blender state
 */
static void si_update_fb_blend_state(struct r600_context *rctx)
{
	struct si_pm4_state *pm4;
	struct si_state_blend *blend = rctx->queued.named.blend;
	uint32_t mask;

	if (blend == NULL)
		return;

	pm4 = CALLOC_STRUCT(si_pm4_state);
	if (pm4 == NULL)
		return;

	mask = (1ULL << ((unsigned)rctx->framebuffer.nr_cbufs * 4)) - 1;
	mask &= blend->cb_target_mask;
	si_pm4_set_reg(pm4, R_028238_CB_TARGET_MASK, mask);

	si_pm4_set_state(rctx, fb_blend, pm4);
}

/*
 * Blender functions
 */

static uint32_t si_translate_blend_function(int blend_func)
{
	switch (blend_func) {
	case PIPE_BLEND_ADD:
		return V_028780_COMB_DST_PLUS_SRC;
	case PIPE_BLEND_SUBTRACT:
		return V_028780_COMB_SRC_MINUS_DST;
	case PIPE_BLEND_REVERSE_SUBTRACT:
		return V_028780_COMB_DST_MINUS_SRC;
	case PIPE_BLEND_MIN:
		return V_028780_COMB_MIN_DST_SRC;
	case PIPE_BLEND_MAX:
		return V_028780_COMB_MAX_DST_SRC;
	default:
		R600_ERR("Unknown blend function %d\n", blend_func);
		assert(0);
		break;
	}
	return 0;
}

static uint32_t si_translate_blend_factor(int blend_fact)
{
	switch (blend_fact) {
	case PIPE_BLENDFACTOR_ONE:
		return V_028780_BLEND_ONE;
	case PIPE_BLENDFACTOR_SRC_COLOR:
		return V_028780_BLEND_SRC_COLOR;
	case PIPE_BLENDFACTOR_SRC_ALPHA:
		return V_028780_BLEND_SRC_ALPHA;
	case PIPE_BLENDFACTOR_DST_ALPHA:
		return V_028780_BLEND_DST_ALPHA;
	case PIPE_BLENDFACTOR_DST_COLOR:
		return V_028780_BLEND_DST_COLOR;
	case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
		return V_028780_BLEND_SRC_ALPHA_SATURATE;
	case PIPE_BLENDFACTOR_CONST_COLOR:
		return V_028780_BLEND_CONSTANT_COLOR;
	case PIPE_BLENDFACTOR_CONST_ALPHA:
		return V_028780_BLEND_CONSTANT_ALPHA;
	case PIPE_BLENDFACTOR_ZERO:
		return V_028780_BLEND_ZERO;
	case PIPE_BLENDFACTOR_INV_SRC_COLOR:
		return V_028780_BLEND_ONE_MINUS_SRC_COLOR;
	case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
		return V_028780_BLEND_ONE_MINUS_SRC_ALPHA;
	case PIPE_BLENDFACTOR_INV_DST_ALPHA:
		return V_028780_BLEND_ONE_MINUS_DST_ALPHA;
	case PIPE_BLENDFACTOR_INV_DST_COLOR:
		return V_028780_BLEND_ONE_MINUS_DST_COLOR;
	case PIPE_BLENDFACTOR_INV_CONST_COLOR:
		return V_028780_BLEND_ONE_MINUS_CONSTANT_COLOR;
	case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
		return V_028780_BLEND_ONE_MINUS_CONSTANT_ALPHA;
	case PIPE_BLENDFACTOR_SRC1_COLOR:
		return V_028780_BLEND_SRC1_COLOR;
	case PIPE_BLENDFACTOR_SRC1_ALPHA:
		return V_028780_BLEND_SRC1_ALPHA;
	case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
		return V_028780_BLEND_INV_SRC1_COLOR;
	case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
		return V_028780_BLEND_INV_SRC1_ALPHA;
	default:
		R600_ERR("Bad blend factor %d not supported!\n", blend_fact);
		assert(0);
		break;
	}
	return 0;
}

static void *si_create_blend_state(struct pipe_context *ctx,
				   const struct pipe_blend_state *state)
{
	struct si_state_blend *blend = CALLOC_STRUCT(si_state_blend);
	struct si_pm4_state *pm4 = &blend->pm4;

	uint32_t color_control;

	if (blend == NULL)
		return NULL;

	color_control = S_028808_MODE(V_028808_CB_NORMAL);
	if (state->logicop_enable) {
		color_control |= S_028808_ROP3(state->logicop_func | (state->logicop_func << 4));
	} else {
		color_control |= S_028808_ROP3(0xcc);
	}
	si_pm4_set_reg(pm4, R_028808_CB_COLOR_CONTROL, color_control);

	si_pm4_set_reg(pm4, R_028C38_PA_SC_AA_MASK_X0Y0_X1Y0, ~0);
	si_pm4_set_reg(pm4, R_028C3C_PA_SC_AA_MASK_X0Y1_X1Y1, ~0);

	blend->cb_target_mask = 0;
	for (int i = 0; i < 8; i++) {
		/* state->rt entries > 0 only written if independent blending */
		const int j = state->independent_blend_enable ? i : 0;

		unsigned eqRGB = state->rt[j].rgb_func;
		unsigned srcRGB = state->rt[j].rgb_src_factor;
		unsigned dstRGB = state->rt[j].rgb_dst_factor;
		unsigned eqA = state->rt[j].alpha_func;
		unsigned srcA = state->rt[j].alpha_src_factor;
		unsigned dstA = state->rt[j].alpha_dst_factor;

		unsigned blend_cntl = 0;

		/* we pretend 8 buffer are used, CB_SHADER_MASK will disable unused one */
		blend->cb_target_mask |= state->rt[j].colormask << (4 * i);

		if (!state->rt[j].blend_enable) {
			si_pm4_set_reg(pm4, R_028780_CB_BLEND0_CONTROL + i * 4, blend_cntl);
			continue;
		}

		blend_cntl |= S_028780_ENABLE(1);
		blend_cntl |= S_028780_COLOR_COMB_FCN(si_translate_blend_function(eqRGB));
		blend_cntl |= S_028780_COLOR_SRCBLEND(si_translate_blend_factor(srcRGB));
		blend_cntl |= S_028780_COLOR_DESTBLEND(si_translate_blend_factor(dstRGB));

		if (srcA != srcRGB || dstA != dstRGB || eqA != eqRGB) {
			blend_cntl |= S_028780_SEPARATE_ALPHA_BLEND(1);
			blend_cntl |= S_028780_ALPHA_COMB_FCN(si_translate_blend_function(eqA));
			blend_cntl |= S_028780_ALPHA_SRCBLEND(si_translate_blend_factor(srcA));
			blend_cntl |= S_028780_ALPHA_DESTBLEND(si_translate_blend_factor(dstA));
		}
		si_pm4_set_reg(pm4, R_028780_CB_BLEND0_CONTROL + i * 4, blend_cntl);
	}

	return blend;
}

static void si_bind_blend_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	si_pm4_bind_state(rctx, blend, (struct si_state_blend *)state);
	si_update_fb_blend_state(rctx);
}

static void si_delete_blend_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	si_pm4_delete_state(rctx, blend, (struct si_state_blend *)state);
}

static void si_set_blend_color(struct pipe_context *ctx,
			       const struct pipe_blend_color *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);

        if (pm4 == NULL)
                return;

	si_pm4_set_reg(pm4, R_028414_CB_BLEND_RED, fui(state->color[0]));
	si_pm4_set_reg(pm4, R_028418_CB_BLEND_GREEN, fui(state->color[1]));
	si_pm4_set_reg(pm4, R_02841C_CB_BLEND_BLUE, fui(state->color[2]));
	si_pm4_set_reg(pm4, R_028420_CB_BLEND_ALPHA, fui(state->color[3]));

	si_pm4_set_state(rctx, blend_color, pm4);
}

/*
 * Clipping, scissors and viewport
 */

static void si_set_clip_state(struct pipe_context *ctx,
			      const struct pipe_clip_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);

	if (pm4 == NULL)
		return;

	for (int i = 0; i < 6; i++) {
		si_pm4_set_reg(pm4, R_0285BC_PA_CL_UCP_0_X + i * 16,
			       fui(state->ucp[i][0]));
		si_pm4_set_reg(pm4, R_0285C0_PA_CL_UCP_0_Y + i * 16,
			       fui(state->ucp[i][1]));
		si_pm4_set_reg(pm4, R_0285C4_PA_CL_UCP_0_Z + i * 16,
			       fui(state->ucp[i][2]));
		si_pm4_set_reg(pm4, R_0285C8_PA_CL_UCP_0_W + i * 16,
			       fui(state->ucp[i][3]));
        }

	si_pm4_set_state(rctx, clip, pm4);
}

static void si_set_scissor_state(struct pipe_context *ctx,
				 const struct pipe_scissor_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);
	uint32_t tl, br;

	if (pm4 == NULL)
		return;

	tl = S_028240_TL_X(state->minx) | S_028240_TL_Y(state->miny);
	br = S_028244_BR_X(state->maxx) | S_028244_BR_Y(state->maxy);
	si_pm4_set_reg(pm4, R_028210_PA_SC_CLIPRECT_0_TL, tl);
	si_pm4_set_reg(pm4, R_028214_PA_SC_CLIPRECT_0_BR, br);
	si_pm4_set_reg(pm4, R_028218_PA_SC_CLIPRECT_1_TL, tl);
	si_pm4_set_reg(pm4, R_02821C_PA_SC_CLIPRECT_1_BR, br);
	si_pm4_set_reg(pm4, R_028220_PA_SC_CLIPRECT_2_TL, tl);
	si_pm4_set_reg(pm4, R_028224_PA_SC_CLIPRECT_2_BR, br);
	si_pm4_set_reg(pm4, R_028228_PA_SC_CLIPRECT_3_TL, tl);
	si_pm4_set_reg(pm4, R_02822C_PA_SC_CLIPRECT_3_BR, br);

	si_pm4_set_state(rctx, scissor, pm4);
}

static void si_set_viewport_state(struct pipe_context *ctx,
				  const struct pipe_viewport_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_state_viewport *viewport = CALLOC_STRUCT(si_state_viewport);
	struct si_pm4_state *pm4 = &viewport->pm4;

	if (viewport == NULL)
		return;

	viewport->viewport = *state;
	si_pm4_set_reg(pm4, R_0282D0_PA_SC_VPORT_ZMIN_0, 0x00000000);
	si_pm4_set_reg(pm4, R_0282D4_PA_SC_VPORT_ZMAX_0, 0x3F800000);
	si_pm4_set_reg(pm4, R_02843C_PA_CL_VPORT_XSCALE_0, fui(state->scale[0]));
	si_pm4_set_reg(pm4, R_028440_PA_CL_VPORT_XOFFSET_0, fui(state->translate[0]));
	si_pm4_set_reg(pm4, R_028444_PA_CL_VPORT_YSCALE_0, fui(state->scale[1]));
	si_pm4_set_reg(pm4, R_028448_PA_CL_VPORT_YOFFSET_0, fui(state->translate[1]));
	si_pm4_set_reg(pm4, R_02844C_PA_CL_VPORT_ZSCALE_0, fui(state->scale[2]));
	si_pm4_set_reg(pm4, R_028450_PA_CL_VPORT_ZOFFSET_0, fui(state->translate[2]));
	si_pm4_set_reg(pm4, R_028818_PA_CL_VTE_CNTL, 0x0000043F);

	si_pm4_set_state(rctx, viewport, viewport);
}

/*
 * inferred state between framebuffer and rasterizer
 */
static void si_update_fb_rs_state(struct r600_context *rctx)
{
	struct si_state_rasterizer *rs = rctx->queued.named.rasterizer;
	struct si_pm4_state *pm4;
	unsigned offset_db_fmt_cntl = 0, depth;
	float offset_units;

	if (!rs || !rctx->framebuffer.zsbuf)
		return;

	offset_units = rctx->queued.named.rasterizer->offset_units;
	switch (rctx->framebuffer.zsbuf->texture->format) {
	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		depth = -24;
		offset_units *= 2.0f;
		break;
	case PIPE_FORMAT_Z32_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
		depth = -23;
		offset_units *= 1.0f;
		offset_db_fmt_cntl |= S_028B78_POLY_OFFSET_DB_IS_FLOAT_FMT(1);
		break;
	case PIPE_FORMAT_Z16_UNORM:
		depth = -16;
		offset_units *= 4.0f;
		break;
	default:
		return;
	}

	pm4 = CALLOC_STRUCT(si_pm4_state);
	/* FIXME some of those reg can be computed with cso */
	offset_db_fmt_cntl |= S_028B78_POLY_OFFSET_NEG_NUM_DB_BITS(depth);
	si_pm4_set_reg(pm4, R_028B80_PA_SU_POLY_OFFSET_FRONT_SCALE,
		       fui(rctx->queued.named.rasterizer->offset_scale));
	si_pm4_set_reg(pm4, R_028B84_PA_SU_POLY_OFFSET_FRONT_OFFSET, fui(offset_units));
	si_pm4_set_reg(pm4, R_028B88_PA_SU_POLY_OFFSET_BACK_SCALE,
		       fui(rctx->queued.named.rasterizer->offset_scale));
	si_pm4_set_reg(pm4, R_028B8C_PA_SU_POLY_OFFSET_BACK_OFFSET, fui(offset_units));
	si_pm4_set_reg(pm4, R_028B78_PA_SU_POLY_OFFSET_DB_FMT_CNTL, offset_db_fmt_cntl);

	si_pm4_set_state(rctx, fb_rs, pm4);
}

/*
 * Rasterizer
 */

static uint32_t si_translate_fill(uint32_t func)
{
	switch(func) {
	case PIPE_POLYGON_MODE_FILL:
		return V_028814_X_DRAW_TRIANGLES;
	case PIPE_POLYGON_MODE_LINE:
		return V_028814_X_DRAW_LINES;
	case PIPE_POLYGON_MODE_POINT:
		return V_028814_X_DRAW_POINTS;
	default:
		assert(0);
		return V_028814_X_DRAW_POINTS;
	}
}

static void *si_create_rs_state(struct pipe_context *ctx,
				const struct pipe_rasterizer_state *state)
{
	struct si_state_rasterizer *rs = CALLOC_STRUCT(si_state_rasterizer);
	struct si_pm4_state *pm4 = &rs->pm4;
	unsigned tmp;
	unsigned prov_vtx = 1, polygon_dual_mode;
	unsigned clip_rule;
	float psize_min, psize_max;

	if (rs == NULL) {
		return NULL;
	}

	polygon_dual_mode = (state->fill_front != PIPE_POLYGON_MODE_FILL ||
				state->fill_back != PIPE_POLYGON_MODE_FILL);

	if (state->flatshade_first)
		prov_vtx = 0;

	rs->flatshade = state->flatshade;
	rs->sprite_coord_enable = state->sprite_coord_enable;
	rs->pa_sc_line_stipple = state->line_stipple_enable ?
				S_028A0C_LINE_PATTERN(state->line_stipple_pattern) |
				S_028A0C_REPEAT_COUNT(state->line_stipple_factor) : 0;
	rs->pa_su_sc_mode_cntl =
		S_028814_PROVOKING_VTX_LAST(prov_vtx) |
		S_028814_CULL_FRONT(state->rasterizer_discard || (state->cull_face & PIPE_FACE_FRONT) ? 1 : 0) |
		S_028814_CULL_BACK(state->rasterizer_discard || (state->cull_face & PIPE_FACE_BACK) ? 1 : 0) |
		S_028814_FACE(!state->front_ccw) |
		S_028814_POLY_OFFSET_FRONT_ENABLE(state->offset_tri) |
		S_028814_POLY_OFFSET_BACK_ENABLE(state->offset_tri) |
		S_028814_POLY_OFFSET_PARA_ENABLE(state->offset_tri) |
		S_028814_POLY_MODE(polygon_dual_mode) |
		S_028814_POLYMODE_FRONT_PTYPE(si_translate_fill(state->fill_front)) |
		S_028814_POLYMODE_BACK_PTYPE(si_translate_fill(state->fill_back));
	rs->pa_cl_clip_cntl =
		S_028810_PS_UCP_MODE(3) |
		S_028810_ZCLIP_NEAR_DISABLE(!state->depth_clip) |
		S_028810_ZCLIP_FAR_DISABLE(!state->depth_clip) |
		S_028810_DX_LINEAR_ATTR_CLIP_ENA(1);
	rs->pa_cl_vs_out_cntl =
		S_02881C_USE_VTX_POINT_SIZE(state->point_size_per_vertex) |
		S_02881C_VS_OUT_MISC_VEC_ENA(state->point_size_per_vertex);

	clip_rule = state->scissor ? 0xAAAA : 0xFFFF;

	/* offset */
	rs->offset_units = state->offset_units;
	rs->offset_scale = state->offset_scale * 12.0f;

	/* XXX: Flat shading hangs the GPU */
	tmp = S_0286D4_FLAT_SHADE_ENA(0);
	if (state->sprite_coord_enable) {
		tmp |= S_0286D4_PNT_SPRITE_ENA(1) |
			S_0286D4_PNT_SPRITE_OVRD_X(V_0286D4_SPI_PNT_SPRITE_SEL_S) |
			S_0286D4_PNT_SPRITE_OVRD_Y(V_0286D4_SPI_PNT_SPRITE_SEL_T) |
			S_0286D4_PNT_SPRITE_OVRD_Z(V_0286D4_SPI_PNT_SPRITE_SEL_0) |
			S_0286D4_PNT_SPRITE_OVRD_W(V_0286D4_SPI_PNT_SPRITE_SEL_1);
		if (state->sprite_coord_mode != PIPE_SPRITE_COORD_UPPER_LEFT) {
			tmp |= S_0286D4_PNT_SPRITE_TOP_1(1);
		}
	}
	si_pm4_set_reg(pm4, R_0286D4_SPI_INTERP_CONTROL_0, tmp);

	si_pm4_set_reg(pm4, R_028820_PA_CL_NANINF_CNTL, 0x00000000);
	/* point size 12.4 fixed point */
	tmp = (unsigned)(state->point_size * 8.0);
	si_pm4_set_reg(pm4, R_028A00_PA_SU_POINT_SIZE, S_028A00_HEIGHT(tmp) | S_028A00_WIDTH(tmp));

	if (state->point_size_per_vertex) {
		psize_min = util_get_min_point_size(state);
		psize_max = 8192;
	} else {
		/* Force the point size to be as if the vertex output was disabled. */
		psize_min = state->point_size;
		psize_max = state->point_size;
	}
	/* Divide by two, because 0.5 = 1 pixel. */
	si_pm4_set_reg(pm4, R_028A04_PA_SU_POINT_MINMAX,
			S_028A04_MIN_SIZE(r600_pack_float_12p4(psize_min/2)) |
			S_028A04_MAX_SIZE(r600_pack_float_12p4(psize_max/2)));

	tmp = (unsigned)state->line_width * 8;
	si_pm4_set_reg(pm4, R_028A08_PA_SU_LINE_CNTL, S_028A08_WIDTH(tmp));
	si_pm4_set_reg(pm4, R_028A48_PA_SC_MODE_CNTL_0,
			S_028A48_LINE_STIPPLE_ENABLE(state->line_stipple_enable));

	si_pm4_set_reg(pm4, R_028BDC_PA_SC_LINE_CNTL, 0x00000400);
	si_pm4_set_reg(pm4, R_028BE4_PA_SU_VTX_CNTL,
			S_028BE4_PIX_CENTER(state->gl_rasterization_rules));
	si_pm4_set_reg(pm4, R_028BE8_PA_CL_GB_VERT_CLIP_ADJ, 0x3F800000);
	si_pm4_set_reg(pm4, R_028BEC_PA_CL_GB_VERT_DISC_ADJ, 0x3F800000);
	si_pm4_set_reg(pm4, R_028BF0_PA_CL_GB_HORZ_CLIP_ADJ, 0x3F800000);
	si_pm4_set_reg(pm4, R_028BF4_PA_CL_GB_HORZ_DISC_ADJ, 0x3F800000);

	si_pm4_set_reg(pm4, R_028B7C_PA_SU_POLY_OFFSET_CLAMP, fui(state->offset_clamp));
	si_pm4_set_reg(pm4, R_02820C_PA_SC_CLIPRECT_RULE, clip_rule);

	return rs;
}

static void si_bind_rs_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_state_rasterizer *rs = (struct si_state_rasterizer *)state;

	if (state == NULL)
		return;

	// TODO
	rctx->sprite_coord_enable = rs->sprite_coord_enable;
	rctx->pa_sc_line_stipple = rs->pa_sc_line_stipple;
	rctx->pa_su_sc_mode_cntl = rs->pa_su_sc_mode_cntl;
	rctx->pa_cl_clip_cntl = rs->pa_cl_clip_cntl;
	rctx->pa_cl_vs_out_cntl = rs->pa_cl_vs_out_cntl;

	si_pm4_bind_state(rctx, rasterizer, rs);
	si_update_fb_rs_state(rctx);
}

static void si_delete_rs_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	si_pm4_delete_state(rctx, rasterizer, (struct si_state_rasterizer *)state);
}

/*
 * infeered state between dsa and stencil ref
 */
static void si_update_dsa_stencil_ref(struct r600_context *rctx)
{
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);
	struct pipe_stencil_ref *ref = &rctx->stencil_ref;
        struct si_state_dsa *dsa = rctx->queued.named.dsa;

        if (pm4 == NULL)
                return;

	si_pm4_set_reg(pm4, R_028430_DB_STENCILREFMASK,
		       S_028430_STENCILTESTVAL(ref->ref_value[0]) |
		       S_028430_STENCILMASK(dsa->valuemask[0]) |
		       S_028430_STENCILWRITEMASK(dsa->writemask[0]));
	si_pm4_set_reg(pm4, R_028434_DB_STENCILREFMASK_BF,
		       S_028434_STENCILTESTVAL_BF(ref->ref_value[1]) |
		       S_028434_STENCILMASK_BF(dsa->valuemask[1]) |
		       S_028434_STENCILWRITEMASK_BF(dsa->writemask[1]));

	si_pm4_set_state(rctx, dsa_stencil_ref, pm4);
}

static void si_set_pipe_stencil_ref(struct pipe_context *ctx,
				    const struct pipe_stencil_ref *state)
{
        struct r600_context *rctx = (struct r600_context *)ctx;
        rctx->stencil_ref = *state;
	si_update_dsa_stencil_ref(rctx);
}


/*
 * DSA
 */

static uint32_t si_translate_stencil_op(int s_op)
{
	switch (s_op) {
	case PIPE_STENCIL_OP_KEEP:
		return V_02842C_STENCIL_KEEP;
	case PIPE_STENCIL_OP_ZERO:
		return V_02842C_STENCIL_ZERO;
	case PIPE_STENCIL_OP_REPLACE:
		return V_02842C_STENCIL_REPLACE_TEST;
	case PIPE_STENCIL_OP_INCR:
		return V_02842C_STENCIL_ADD_CLAMP;
	case PIPE_STENCIL_OP_DECR:
		return V_02842C_STENCIL_SUB_CLAMP;
	case PIPE_STENCIL_OP_INCR_WRAP:
		return V_02842C_STENCIL_ADD_WRAP;
	case PIPE_STENCIL_OP_DECR_WRAP:
		return V_02842C_STENCIL_SUB_WRAP;
	case PIPE_STENCIL_OP_INVERT:
		return V_02842C_STENCIL_INVERT;
	default:
		R600_ERR("Unknown stencil op %d", s_op);
		assert(0);
		break;
	}
	return 0;
}

static void *si_create_dsa_state(struct pipe_context *ctx,
				 const struct pipe_depth_stencil_alpha_state *state)
{
	struct si_state_dsa *dsa = CALLOC_STRUCT(si_state_dsa);
	struct si_pm4_state *pm4 = &dsa->pm4;
	unsigned db_depth_control, /* alpha_test_control, */ alpha_ref;
	unsigned db_render_override, db_render_control;
	uint32_t db_stencil_control = 0;

	if (dsa == NULL) {
		return NULL;
	}

	dsa->valuemask[0] = state->stencil[0].valuemask;
	dsa->valuemask[1] = state->stencil[1].valuemask;
	dsa->writemask[0] = state->stencil[0].writemask;
	dsa->writemask[1] = state->stencil[1].writemask;

	db_depth_control = S_028800_Z_ENABLE(state->depth.enabled) |
		S_028800_Z_WRITE_ENABLE(state->depth.writemask) |
		S_028800_ZFUNC(state->depth.func);

	/* stencil */
	if (state->stencil[0].enabled) {
		db_depth_control |= S_028800_STENCIL_ENABLE(1);
		db_depth_control |= S_028800_STENCILFUNC(state->stencil[0].func);
		db_stencil_control |= S_02842C_STENCILFAIL(si_translate_stencil_op(state->stencil[0].fail_op));
		db_stencil_control |= S_02842C_STENCILZPASS(si_translate_stencil_op(state->stencil[0].zpass_op));
		db_stencil_control |= S_02842C_STENCILZFAIL(si_translate_stencil_op(state->stencil[0].zfail_op));

		if (state->stencil[1].enabled) {
			db_depth_control |= S_028800_BACKFACE_ENABLE(1);
			db_depth_control |= S_028800_STENCILFUNC_BF(state->stencil[1].func);
			db_stencil_control |= S_02842C_STENCILFAIL_BF(si_translate_stencil_op(state->stencil[1].fail_op));
			db_stencil_control |= S_02842C_STENCILZPASS_BF(si_translate_stencil_op(state->stencil[1].zpass_op));
			db_stencil_control |= S_02842C_STENCILZFAIL_BF(si_translate_stencil_op(state->stencil[1].zfail_op));
		}
	}

	/* alpha */
	//alpha_test_control = 0;
	alpha_ref = 0;
	if (state->alpha.enabled) {
		//alpha_test_control = S_028410_ALPHA_FUNC(state->alpha.func);
		//alpha_test_control |= S_028410_ALPHA_TEST_ENABLE(1);
		alpha_ref = fui(state->alpha.ref_value);
	}
	dsa->alpha_ref = alpha_ref;

	/* misc */
	db_render_control = 0;
	db_render_override = S_02800C_FORCE_HIZ_ENABLE(V_02800C_FORCE_DISABLE) |
		S_02800C_FORCE_HIS_ENABLE0(V_02800C_FORCE_DISABLE) |
		S_02800C_FORCE_HIS_ENABLE1(V_02800C_FORCE_DISABLE);
	/* TODO db_render_override depends on query */
	si_pm4_set_reg(pm4, R_028020_DB_DEPTH_BOUNDS_MIN, 0x00000000);
	si_pm4_set_reg(pm4, R_028024_DB_DEPTH_BOUNDS_MAX, 0x00000000);
	si_pm4_set_reg(pm4, R_028028_DB_STENCIL_CLEAR, 0x00000000);
	si_pm4_set_reg(pm4, R_02802C_DB_DEPTH_CLEAR, 0x3F800000);
	//si_pm4_set_reg(pm4, R_028410_SX_ALPHA_TEST_CONTROL, alpha_test_control);
	si_pm4_set_reg(pm4, R_028800_DB_DEPTH_CONTROL, db_depth_control);
	si_pm4_set_reg(pm4, R_028000_DB_RENDER_CONTROL, db_render_control);
	si_pm4_set_reg(pm4, R_02800C_DB_RENDER_OVERRIDE, db_render_override);
	si_pm4_set_reg(pm4, R_02842C_DB_STENCIL_CONTROL, db_stencil_control);
	si_pm4_set_reg(pm4, R_028AC0_DB_SRESULTS_COMPARE_STATE0, 0x0);
	si_pm4_set_reg(pm4, R_028AC4_DB_SRESULTS_COMPARE_STATE1, 0x0);
	si_pm4_set_reg(pm4, R_028AC8_DB_PRELOAD_CONTROL, 0x0);
	si_pm4_set_reg(pm4, R_028B70_DB_ALPHA_TO_MASK, 0x0000AA00);
	dsa->db_render_override = db_render_override;

	return dsa;
}

static void si_bind_dsa_state(struct pipe_context *ctx, void *state)
{
        struct r600_context *rctx = (struct r600_context *)ctx;
        struct si_state_dsa *dsa = state;

        if (state == NULL)
                return;

	si_pm4_bind_state(rctx, dsa, dsa);
	si_update_dsa_stencil_ref(rctx);

	// TODO
        rctx->alpha_ref = dsa->alpha_ref;
        rctx->alpha_ref_dirty = true;
}

static void si_delete_dsa_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	si_pm4_delete_state(rctx, dsa, (struct si_state_dsa *)state);
}

static void *si_create_db_flush_dsa(struct r600_context *rctx)
{
	struct pipe_depth_stencil_alpha_state dsa;
        struct si_state_dsa *state;

	memset(&dsa, 0, sizeof(dsa));

	state = rctx->context.create_depth_stencil_alpha_state(&rctx->context, &dsa);
	si_pm4_set_reg(&state->pm4, R_028000_DB_RENDER_CONTROL,
		       S_028000_DEPTH_COPY(1) |
		       S_028000_STENCIL_COPY(1) |
		       S_028000_COPY_CENTROID(1));
        return state;
}

/*
 * format translation
 */
static uint32_t si_translate_colorformat(enum pipe_format format)
{
	switch (format) {
	/* 8-bit buffers. */
	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_A8_UINT:
	case PIPE_FORMAT_A8_SINT:
	case PIPE_FORMAT_I8_UNORM:
	case PIPE_FORMAT_I8_UINT:
	case PIPE_FORMAT_I8_SINT:
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_L8_UINT:
	case PIPE_FORMAT_L8_SINT:
	case PIPE_FORMAT_L8_SRGB:
	case PIPE_FORMAT_R8_UNORM:
	case PIPE_FORMAT_R8_SNORM:
	case PIPE_FORMAT_R8_UINT:
	case PIPE_FORMAT_R8_SINT:
		return V_028C70_COLOR_8;

	/* 16-bit buffers. */
	case PIPE_FORMAT_B5G6R5_UNORM:
		return V_028C70_COLOR_5_6_5;

	case PIPE_FORMAT_B5G5R5A1_UNORM:
	case PIPE_FORMAT_B5G5R5X1_UNORM:
		return V_028C70_COLOR_1_5_5_5;

	case PIPE_FORMAT_B4G4R4A4_UNORM:
	case PIPE_FORMAT_B4G4R4X4_UNORM:
		return V_028C70_COLOR_4_4_4_4;

	case PIPE_FORMAT_L8A8_UNORM:
	case PIPE_FORMAT_L8A8_UINT:
	case PIPE_FORMAT_L8A8_SINT:
	case PIPE_FORMAT_L8A8_SRGB:
	case PIPE_FORMAT_R8G8_UNORM:
	case PIPE_FORMAT_R8G8_UINT:
	case PIPE_FORMAT_R8G8_SINT:
		return V_028C70_COLOR_8_8;

	case PIPE_FORMAT_Z16_UNORM:
	case PIPE_FORMAT_R16_UNORM:
	case PIPE_FORMAT_R16_UINT:
	case PIPE_FORMAT_R16_SINT:
	case PIPE_FORMAT_R16_FLOAT:
	case PIPE_FORMAT_R16G16_FLOAT:
		return V_028C70_COLOR_16;

	/* 32-bit buffers. */
	case PIPE_FORMAT_A8B8G8R8_SRGB:
	case PIPE_FORMAT_A8B8G8R8_UNORM:
	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case PIPE_FORMAT_B8G8R8A8_SRGB:
	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case PIPE_FORMAT_B8G8R8X8_UNORM:
	case PIPE_FORMAT_R8G8B8A8_SNORM:
	case PIPE_FORMAT_R8G8B8A8_UNORM:
	case PIPE_FORMAT_R8G8B8X8_UNORM:
	case PIPE_FORMAT_R8SG8SB8UX8U_NORM:
	case PIPE_FORMAT_X8B8G8R8_UNORM:
	case PIPE_FORMAT_X8R8G8B8_UNORM:
	case PIPE_FORMAT_R8G8B8_UNORM:
	case PIPE_FORMAT_R8G8B8A8_SSCALED:
	case PIPE_FORMAT_R8G8B8A8_USCALED:
	case PIPE_FORMAT_R8G8B8A8_SINT:
	case PIPE_FORMAT_R8G8B8A8_UINT:
		return V_028C70_COLOR_8_8_8_8;

	case PIPE_FORMAT_R10G10B10A2_UNORM:
	case PIPE_FORMAT_R10G10B10X2_SNORM:
	case PIPE_FORMAT_B10G10R10A2_UNORM:
	case PIPE_FORMAT_B10G10R10A2_UINT:
	case PIPE_FORMAT_R10SG10SB10SA2U_NORM:
		return V_028C70_COLOR_2_10_10_10;

	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		return V_028C70_COLOR_8_24;

	case PIPE_FORMAT_X8Z24_UNORM:
	case PIPE_FORMAT_S8_UINT_Z24_UNORM:
		return V_028C70_COLOR_24_8;

	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
		return V_028C70_COLOR_X24_8_32_FLOAT;

	case PIPE_FORMAT_R32_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT:
		return V_028C70_COLOR_32;

	case PIPE_FORMAT_R16G16_SSCALED:
	case PIPE_FORMAT_R16G16_UNORM:
	case PIPE_FORMAT_R16G16_UINT:
	case PIPE_FORMAT_R16G16_SINT:
		return V_028C70_COLOR_16_16;

	case PIPE_FORMAT_R11G11B10_FLOAT:
		return V_028C70_COLOR_10_11_11;

	/* 64-bit buffers. */
	case PIPE_FORMAT_R16G16B16_USCALED:
	case PIPE_FORMAT_R16G16B16_SSCALED:
	case PIPE_FORMAT_R16G16B16A16_UINT:
	case PIPE_FORMAT_R16G16B16A16_SINT:
	case PIPE_FORMAT_R16G16B16A16_USCALED:
	case PIPE_FORMAT_R16G16B16A16_SSCALED:
	case PIPE_FORMAT_R16G16B16A16_UNORM:
	case PIPE_FORMAT_R16G16B16A16_SNORM:
	case PIPE_FORMAT_R16G16B16_FLOAT:
	case PIPE_FORMAT_R16G16B16A16_FLOAT:
		return V_028C70_COLOR_16_16_16_16;

	case PIPE_FORMAT_R32G32_FLOAT:
	case PIPE_FORMAT_R32G32_USCALED:
	case PIPE_FORMAT_R32G32_SSCALED:
	case PIPE_FORMAT_R32G32_SINT:
	case PIPE_FORMAT_R32G32_UINT:
		return V_028C70_COLOR_32_32;

	/* 128-bit buffers. */
	case PIPE_FORMAT_R32G32B32A32_SNORM:
	case PIPE_FORMAT_R32G32B32A32_UNORM:
	case PIPE_FORMAT_R32G32B32A32_SSCALED:
	case PIPE_FORMAT_R32G32B32A32_USCALED:
	case PIPE_FORMAT_R32G32B32A32_SINT:
	case PIPE_FORMAT_R32G32B32A32_UINT:
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
		return V_028C70_COLOR_32_32_32_32;

	/* YUV buffers. */
	case PIPE_FORMAT_UYVY:
	case PIPE_FORMAT_YUYV:
	/* 96-bit buffers. */
	case PIPE_FORMAT_R32G32B32_FLOAT:
	/* 8-bit buffers. */
	case PIPE_FORMAT_L4A4_UNORM:
	case PIPE_FORMAT_R4A4_UNORM:
	case PIPE_FORMAT_A4R4_UNORM:
	default:
		return ~0U; /* Unsupported. */
	}
}

static uint32_t si_translate_colorswap(enum pipe_format format)
{
	switch (format) {
	/* 8-bit buffers. */
	case PIPE_FORMAT_L4A4_UNORM:
	case PIPE_FORMAT_A4R4_UNORM:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_A8_UINT:
	case PIPE_FORMAT_A8_SINT:
	case PIPE_FORMAT_R4A4_UNORM:
		return V_028C70_SWAP_ALT_REV;
	case PIPE_FORMAT_I8_UNORM:
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_I8_UINT:
	case PIPE_FORMAT_I8_SINT:
	case PIPE_FORMAT_L8_UINT:
	case PIPE_FORMAT_L8_SINT:
	case PIPE_FORMAT_L8_SRGB:
	case PIPE_FORMAT_R8_UNORM:
	case PIPE_FORMAT_R8_SNORM:
	case PIPE_FORMAT_R8_UINT:
	case PIPE_FORMAT_R8_SINT:
		return V_028C70_SWAP_STD;

	/* 16-bit buffers. */
	case PIPE_FORMAT_B5G6R5_UNORM:
		return V_028C70_SWAP_STD_REV;

	case PIPE_FORMAT_B5G5R5A1_UNORM:
	case PIPE_FORMAT_B5G5R5X1_UNORM:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_B4G4R4A4_UNORM:
	case PIPE_FORMAT_B4G4R4X4_UNORM:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_Z16_UNORM:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_L8A8_UNORM:
	case PIPE_FORMAT_L8A8_UINT:
	case PIPE_FORMAT_L8A8_SINT:
	case PIPE_FORMAT_L8A8_SRGB:
		return V_028C70_SWAP_ALT;
	case PIPE_FORMAT_R8G8_UNORM:
	case PIPE_FORMAT_R8G8_UINT:
	case PIPE_FORMAT_R8G8_SINT:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_R16_UNORM:
	case PIPE_FORMAT_R16_UINT:
	case PIPE_FORMAT_R16_SINT:
	case PIPE_FORMAT_R16_FLOAT:
		return V_028C70_SWAP_STD;

	/* 32-bit buffers. */
	case PIPE_FORMAT_A8B8G8R8_SRGB:
		return V_028C70_SWAP_STD_REV;
	case PIPE_FORMAT_B8G8R8A8_SRGB:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case PIPE_FORMAT_B8G8R8X8_UNORM:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case PIPE_FORMAT_X8R8G8B8_UNORM:
		return V_028C70_SWAP_ALT_REV;
	case PIPE_FORMAT_R8G8B8A8_SNORM:
	case PIPE_FORMAT_R8G8B8A8_UNORM:
	case PIPE_FORMAT_R8G8B8A8_SSCALED:
	case PIPE_FORMAT_R8G8B8A8_USCALED:
	case PIPE_FORMAT_R8G8B8A8_SINT:
	case PIPE_FORMAT_R8G8B8A8_UINT:
	case PIPE_FORMAT_R8G8B8X8_UNORM:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_A8B8G8R8_UNORM:
	case PIPE_FORMAT_X8B8G8R8_UNORM:
	/* case PIPE_FORMAT_R8SG8SB8UX8U_NORM: */
		return V_028C70_SWAP_STD_REV;

	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_X8Z24_UNORM:
	case PIPE_FORMAT_S8_UINT_Z24_UNORM:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_R10G10B10A2_UNORM:
	case PIPE_FORMAT_R10G10B10X2_SNORM:
	case PIPE_FORMAT_R10SG10SB10SA2U_NORM:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_B10G10R10A2_UNORM:
	case PIPE_FORMAT_B10G10R10A2_UINT:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_R11G11B10_FLOAT:
	case PIPE_FORMAT_R32_FLOAT:
	case PIPE_FORMAT_R32_UINT:
	case PIPE_FORMAT_R32_SINT:
	case PIPE_FORMAT_Z32_FLOAT:
	case PIPE_FORMAT_R16G16_FLOAT:
	case PIPE_FORMAT_R16G16_UNORM:
	case PIPE_FORMAT_R16G16_UINT:
	case PIPE_FORMAT_R16G16_SINT:
		return V_028C70_SWAP_STD;

	/* 64-bit buffers. */
	case PIPE_FORMAT_R32G32_FLOAT:
	case PIPE_FORMAT_R32G32_UINT:
	case PIPE_FORMAT_R32G32_SINT:
	case PIPE_FORMAT_R16G16B16A16_UNORM:
	case PIPE_FORMAT_R16G16B16A16_SNORM:
	case PIPE_FORMAT_R16G16B16A16_USCALED:
	case PIPE_FORMAT_R16G16B16A16_SSCALED:
	case PIPE_FORMAT_R16G16B16A16_UINT:
	case PIPE_FORMAT_R16G16B16A16_SINT:
	case PIPE_FORMAT_R16G16B16A16_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:

	/* 128-bit buffers. */
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
	case PIPE_FORMAT_R32G32B32A32_SNORM:
	case PIPE_FORMAT_R32G32B32A32_UNORM:
	case PIPE_FORMAT_R32G32B32A32_SSCALED:
	case PIPE_FORMAT_R32G32B32A32_USCALED:
	case PIPE_FORMAT_R32G32B32A32_SINT:
	case PIPE_FORMAT_R32G32B32A32_UINT:
		return V_028C70_SWAP_STD;
	default:
		R600_ERR("unsupported colorswap format %d\n", format);
		return ~0U;
	}
	return ~0U;
}

static uint32_t si_colorformat_endian_swap(uint32_t colorformat)
{
	if (R600_BIG_ENDIAN) {
		switch(colorformat) {
		/* 8-bit buffers. */
		case V_028C70_COLOR_8:
			return V_028C70_ENDIAN_NONE;

		/* 16-bit buffers. */
		case V_028C70_COLOR_5_6_5:
		case V_028C70_COLOR_1_5_5_5:
		case V_028C70_COLOR_4_4_4_4:
		case V_028C70_COLOR_16:
		case V_028C70_COLOR_8_8:
			return V_028C70_ENDIAN_8IN16;

		/* 32-bit buffers. */
		case V_028C70_COLOR_8_8_8_8:
		case V_028C70_COLOR_2_10_10_10:
		case V_028C70_COLOR_8_24:
		case V_028C70_COLOR_24_8:
		case V_028C70_COLOR_16_16:
			return V_028C70_ENDIAN_8IN32;

		/* 64-bit buffers. */
		case V_028C70_COLOR_16_16_16_16:
			return V_028C70_ENDIAN_8IN16;

		case V_028C70_COLOR_32_32:
			return V_028C70_ENDIAN_8IN32;

		/* 128-bit buffers. */
		case V_028C70_COLOR_32_32_32_32:
			return V_028C70_ENDIAN_8IN32;
		default:
			return V_028C70_ENDIAN_NONE; /* Unsupported. */
		}
	} else {
		return V_028C70_ENDIAN_NONE;
	}
}

/* Returns the size in bits of the widest component of a CB format */
static unsigned si_colorformat_max_comp_size(uint32_t colorformat)
{
	switch(colorformat) {
	case V_028C70_COLOR_4_4_4_4:
		return 4;

	case V_028C70_COLOR_1_5_5_5:
	case V_028C70_COLOR_5_5_5_1:
		return 5;

	case V_028C70_COLOR_5_6_5:
		return 6;

	case V_028C70_COLOR_8:
	case V_028C70_COLOR_8_8:
	case V_028C70_COLOR_8_8_8_8:
		return 8;

	case V_028C70_COLOR_10_10_10_2:
	case V_028C70_COLOR_2_10_10_10:
		return 10;

	case V_028C70_COLOR_10_11_11:
	case V_028C70_COLOR_11_11_10:
		return 11;

	case V_028C70_COLOR_16:
	case V_028C70_COLOR_16_16:
	case V_028C70_COLOR_16_16_16_16:
		return 16;

	case V_028C70_COLOR_8_24:
	case V_028C70_COLOR_24_8:
		return 24;

	case V_028C70_COLOR_32:
	case V_028C70_COLOR_32_32:
	case V_028C70_COLOR_32_32_32_32:
	case V_028C70_COLOR_X24_8_32_FLOAT:
		return 32;
	}

	assert(!"Unknown maximum component size");
	return 0;
}

static uint32_t si_translate_dbformat(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_Z16_UNORM:
		return V_028040_Z_16;
	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		return V_028040_Z_24; /* XXX no longer supported on SI */
	case PIPE_FORMAT_Z32_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
		return V_028040_Z_32_FLOAT;
	default:
		return ~0U;
	}
}

/*
 * Texture translation
 */

static uint32_t si_translate_texformat(struct pipe_screen *screen,
				       enum pipe_format format,
				       const struct util_format_description *desc,
				       int first_non_void)
{
	boolean uniform = TRUE;
	int i;

	/* Colorspace (return non-RGB formats directly). */
	switch (desc->colorspace) {
	/* Depth stencil formats */
	case UTIL_FORMAT_COLORSPACE_ZS:
		switch (format) {
		case PIPE_FORMAT_Z16_UNORM:
			return V_008F14_IMG_DATA_FORMAT_16;
		case PIPE_FORMAT_X24S8_UINT:
		case PIPE_FORMAT_Z24X8_UNORM:
		case PIPE_FORMAT_Z24_UNORM_S8_UINT:
			return V_008F14_IMG_DATA_FORMAT_24_8;
		case PIPE_FORMAT_S8X24_UINT:
		case PIPE_FORMAT_X8Z24_UNORM:
		case PIPE_FORMAT_S8_UINT_Z24_UNORM:
			return V_008F14_IMG_DATA_FORMAT_8_24;
		case PIPE_FORMAT_S8_UINT:
			return V_008F14_IMG_DATA_FORMAT_8;
		case PIPE_FORMAT_Z32_FLOAT:
			return V_008F14_IMG_DATA_FORMAT_32;
		case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
			return V_008F14_IMG_DATA_FORMAT_X24_8_32;
		default:
			goto out_unknown;
		}

	case UTIL_FORMAT_COLORSPACE_YUV:
		goto out_unknown; /* TODO */

	case UTIL_FORMAT_COLORSPACE_SRGB:
		break;

	default:
		break;
	}

	/* TODO compressed formats */

	if (format == PIPE_FORMAT_R9G9B9E5_FLOAT) {
		return V_008F14_IMG_DATA_FORMAT_5_9_9_9;
	} else if (format == PIPE_FORMAT_R11G11B10_FLOAT) {
		return V_008F14_IMG_DATA_FORMAT_10_11_11;
	}

	/* R8G8Bx_SNORM - TODO CxV8U8 */

	/* See whether the components are of the same size. */
	for (i = 1; i < desc->nr_channels; i++) {
		uniform = uniform && desc->channel[0].size == desc->channel[i].size;
	}

	/* Non-uniform formats. */
	if (!uniform) {
		switch(desc->nr_channels) {
		case 3:
			if (desc->channel[0].size == 5 &&
			    desc->channel[1].size == 6 &&
			    desc->channel[2].size == 5) {
				return V_008F14_IMG_DATA_FORMAT_5_6_5;
			}
			goto out_unknown;
		case 4:
			if (desc->channel[0].size == 5 &&
			    desc->channel[1].size == 5 &&
			    desc->channel[2].size == 5 &&
			    desc->channel[3].size == 1) {
				return V_008F14_IMG_DATA_FORMAT_1_5_5_5;
			}
			if (desc->channel[0].size == 10 &&
			    desc->channel[1].size == 10 &&
			    desc->channel[2].size == 10 &&
			    desc->channel[3].size == 2) {
				return V_008F14_IMG_DATA_FORMAT_2_10_10_10;
			}
			goto out_unknown;
		}
		goto out_unknown;
	}

	if (first_non_void < 0 || first_non_void > 3)
		goto out_unknown;

	/* uniform formats */
	switch (desc->channel[first_non_void].size) {
	case 4:
		switch (desc->nr_channels) {
		case 2:
			return V_008F14_IMG_DATA_FORMAT_4_4;
		case 4:
			return V_008F14_IMG_DATA_FORMAT_4_4_4_4;
		}
		break;
	case 8:
		switch (desc->nr_channels) {
		case 1:
			return V_008F14_IMG_DATA_FORMAT_8;
		case 2:
			return V_008F14_IMG_DATA_FORMAT_8_8;
		case 4:
			return V_008F14_IMG_DATA_FORMAT_8_8_8_8;
		}
		break;
	case 16:
		switch (desc->nr_channels) {
		case 1:
			return V_008F14_IMG_DATA_FORMAT_16;
		case 2:
			return V_008F14_IMG_DATA_FORMAT_16_16;
		case 4:
			return V_008F14_IMG_DATA_FORMAT_16_16_16_16;
		}
		break;
	case 32:
		switch (desc->nr_channels) {
		case 1:
			return V_008F14_IMG_DATA_FORMAT_32;
		case 2:
			return V_008F14_IMG_DATA_FORMAT_32_32;
		case 3:
			return V_008F14_IMG_DATA_FORMAT_32_32_32;
		case 4:
			return V_008F14_IMG_DATA_FORMAT_32_32_32_32;
		}
	}

out_unknown:
	/* R600_ERR("Unable to handle texformat %d %s\n", format, util_format_name(format)); */
	return ~0;
}

static unsigned si_tex_wrap(unsigned wrap)
{
	switch (wrap) {
	default:
	case PIPE_TEX_WRAP_REPEAT:
		return V_008F30_SQ_TEX_WRAP;
	case PIPE_TEX_WRAP_CLAMP:
		return V_008F30_SQ_TEX_CLAMP_HALF_BORDER;
	case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
		return V_008F30_SQ_TEX_CLAMP_LAST_TEXEL;
	case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
		return V_008F30_SQ_TEX_CLAMP_BORDER;
	case PIPE_TEX_WRAP_MIRROR_REPEAT:
		return V_008F30_SQ_TEX_MIRROR;
	case PIPE_TEX_WRAP_MIRROR_CLAMP:
		return V_008F30_SQ_TEX_MIRROR_ONCE_HALF_BORDER;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
		return V_008F30_SQ_TEX_MIRROR_ONCE_LAST_TEXEL;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
		return V_008F30_SQ_TEX_MIRROR_ONCE_BORDER;
	}
}

static unsigned si_tex_filter(unsigned filter)
{
	switch (filter) {
	default:
	case PIPE_TEX_FILTER_NEAREST:
		return V_008F38_SQ_TEX_XY_FILTER_POINT;
	case PIPE_TEX_FILTER_LINEAR:
		return V_008F38_SQ_TEX_XY_FILTER_BILINEAR;
	}
}

static unsigned si_tex_mipfilter(unsigned filter)
{
	switch (filter) {
	case PIPE_TEX_MIPFILTER_NEAREST:
		return V_008F38_SQ_TEX_Z_FILTER_POINT;
	case PIPE_TEX_MIPFILTER_LINEAR:
		return V_008F38_SQ_TEX_Z_FILTER_LINEAR;
	default:
	case PIPE_TEX_MIPFILTER_NONE:
		return V_008F38_SQ_TEX_Z_FILTER_NONE;
	}
}

static unsigned si_tex_compare(unsigned compare)
{
	switch (compare) {
	default:
	case PIPE_FUNC_NEVER:
		return V_008F30_SQ_TEX_DEPTH_COMPARE_NEVER;
	case PIPE_FUNC_LESS:
		return V_008F30_SQ_TEX_DEPTH_COMPARE_LESS;
	case PIPE_FUNC_EQUAL:
		return V_008F30_SQ_TEX_DEPTH_COMPARE_EQUAL;
	case PIPE_FUNC_LEQUAL:
		return V_008F30_SQ_TEX_DEPTH_COMPARE_LESSEQUAL;
	case PIPE_FUNC_GREATER:
		return V_008F30_SQ_TEX_DEPTH_COMPARE_GREATER;
	case PIPE_FUNC_NOTEQUAL:
		return V_008F30_SQ_TEX_DEPTH_COMPARE_NOTEQUAL;
	case PIPE_FUNC_GEQUAL:
		return V_008F30_SQ_TEX_DEPTH_COMPARE_GREATEREQUAL;
	case PIPE_FUNC_ALWAYS:
		return V_008F30_SQ_TEX_DEPTH_COMPARE_ALWAYS;
	}
}

static unsigned si_tex_dim(unsigned dim)
{
	switch (dim) {
	default:
	case PIPE_TEXTURE_1D:
		return V_008F1C_SQ_RSRC_IMG_1D;
	case PIPE_TEXTURE_1D_ARRAY:
		return V_008F1C_SQ_RSRC_IMG_1D_ARRAY;
	case PIPE_TEXTURE_2D:
	case PIPE_TEXTURE_RECT:
		return V_008F1C_SQ_RSRC_IMG_2D;
	case PIPE_TEXTURE_2D_ARRAY:
		return V_008F1C_SQ_RSRC_IMG_2D_ARRAY;
	case PIPE_TEXTURE_3D:
		return V_008F1C_SQ_RSRC_IMG_3D;
	case PIPE_TEXTURE_CUBE:
		return V_008F1C_SQ_RSRC_IMG_CUBE;
	}
}

/*
 * Format support testing
 */

static bool si_is_sampler_format_supported(struct pipe_screen *screen, enum pipe_format format)
{
	return si_translate_texformat(screen, format, util_format_description(format),
				      util_format_get_first_non_void_channel(format)) != ~0U;
}

static uint32_t si_translate_vertexformat(struct pipe_screen *screen,
					  enum pipe_format format,
					  const struct util_format_description *desc,
					  int first_non_void)
{
	unsigned type = desc->channel[first_non_void].type;
	int i;

	if (type == UTIL_FORMAT_TYPE_FIXED)
		return V_008F0C_BUF_DATA_FORMAT_INVALID;

	/* See whether the components are of the same size. */
	for (i = 0; i < desc->nr_channels; i++) {
		if (desc->channel[first_non_void].size != desc->channel[i].size)
			return V_008F0C_BUF_DATA_FORMAT_INVALID;
	}

	switch (desc->channel[first_non_void].size) {
	case 8:
		switch (desc->nr_channels) {
		case 1:
			return V_008F0C_BUF_DATA_FORMAT_8;
		case 2:
			return V_008F0C_BUF_DATA_FORMAT_8_8;
		case 3:
		case 4:
			return V_008F0C_BUF_DATA_FORMAT_8_8_8_8;
		}
		break;
	case 16:
		switch (desc->nr_channels) {
		case 1:
			return V_008F0C_BUF_DATA_FORMAT_16;
		case 2:
			return V_008F0C_BUF_DATA_FORMAT_16_16;
		case 3:
		case 4:
			return V_008F0C_BUF_DATA_FORMAT_16_16_16_16;
		}
		break;
	case 32:
		if (type != UTIL_FORMAT_TYPE_FLOAT)
			return V_008F0C_BUF_DATA_FORMAT_INVALID;

		switch (desc->nr_channels) {
		case 1:
			return V_008F0C_BUF_DATA_FORMAT_32;
		case 2:
			return V_008F0C_BUF_DATA_FORMAT_32_32;
		case 3:
			return V_008F0C_BUF_DATA_FORMAT_32_32_32;
		case 4:
			return V_008F0C_BUF_DATA_FORMAT_32_32_32_32;
		}
		break;
	}

	return V_008F0C_BUF_DATA_FORMAT_INVALID;
}

static bool si_is_vertex_format_supported(struct pipe_screen *screen, enum pipe_format format)
{
	const struct util_format_description *desc;
	int first_non_void;
	unsigned data_format;

	desc = util_format_description(format);
	first_non_void = util_format_get_first_non_void_channel(format);
	data_format = si_translate_vertexformat(screen, format, desc, first_non_void);
	return data_format != V_008F0C_BUF_DATA_FORMAT_INVALID;
}

static bool si_is_colorbuffer_format_supported(enum pipe_format format)
{
	return si_translate_colorformat(format) != ~0U &&
		si_translate_colorswap(format) != ~0U;
}

static bool si_is_zs_format_supported(enum pipe_format format)
{
	return si_translate_dbformat(format) != ~0U;
}

bool si_is_format_supported(struct pipe_screen *screen,
			    enum pipe_format format,
			    enum pipe_texture_target target,
			    unsigned sample_count,
			    unsigned usage)
{
	unsigned retval = 0;

	if (target >= PIPE_MAX_TEXTURE_TYPES) {
		R600_ERR("r600: unsupported texture type %d\n", target);
		return FALSE;
	}

	if (!util_format_is_supported(format, usage))
		return FALSE;

	/* Multisample */
	if (sample_count > 1)
		return FALSE;

	if ((usage & PIPE_BIND_SAMPLER_VIEW) &&
	    si_is_sampler_format_supported(screen, format)) {
		retval |= PIPE_BIND_SAMPLER_VIEW;
	}

	if ((usage & (PIPE_BIND_RENDER_TARGET |
		      PIPE_BIND_DISPLAY_TARGET |
		      PIPE_BIND_SCANOUT |
		      PIPE_BIND_SHARED)) &&
	    si_is_colorbuffer_format_supported(format)) {
		retval |= usage &
			  (PIPE_BIND_RENDER_TARGET |
			   PIPE_BIND_DISPLAY_TARGET |
			   PIPE_BIND_SCANOUT |
			   PIPE_BIND_SHARED);
	}

	if ((usage & PIPE_BIND_DEPTH_STENCIL) &&
	    si_is_zs_format_supported(format)) {
		retval |= PIPE_BIND_DEPTH_STENCIL;
	}

	if ((usage & PIPE_BIND_VERTEX_BUFFER) &&
	    si_is_vertex_format_supported(screen, format)) {
		retval |= PIPE_BIND_VERTEX_BUFFER;
	}

	if (usage & PIPE_BIND_TRANSFER_READ)
		retval |= PIPE_BIND_TRANSFER_READ;
	if (usage & PIPE_BIND_TRANSFER_WRITE)
		retval |= PIPE_BIND_TRANSFER_WRITE;

	return retval == usage;
}

/*
 * framebuffer handling
 */

static void si_cb(struct r600_context *rctx, struct si_pm4_state *pm4,
		  const struct pipe_framebuffer_state *state, int cb)
{
	struct r600_resource_texture *rtex;
	struct r600_surface *surf;
	unsigned level = state->cbufs[cb]->u.tex.level;
	unsigned pitch, slice;
	unsigned color_info, color_attrib;
	unsigned format, swap, ntype, endian;
	uint64_t offset;
	unsigned blocksize;
	const struct util_format_description *desc;
	int i;
	unsigned blend_clamp = 0, blend_bypass = 0;
	unsigned max_comp_size;

	surf = (struct r600_surface *)state->cbufs[cb];
	rtex = (struct r600_resource_texture*)state->cbufs[cb]->texture;
	blocksize = util_format_get_blocksize(rtex->real_format);

	if (rtex->depth)
		rctx->have_depth_fb = TRUE;

	if (rtex->depth && !rtex->is_flushing_texture) {
	        r600_texture_depth_flush(&rctx->context, state->cbufs[cb]->texture, TRUE);
		rtex = rtex->flushed_depth_texture;
	}

	offset = rtex->surface.level[level].offset;
	if (rtex->surface.level[level].mode < RADEON_SURF_MODE_1D) {
		offset += rtex->surface.level[level].slice_size *
			  state->cbufs[cb]->u.tex.first_layer;
	}
	pitch = (rtex->surface.level[level].nblk_x) / 8 - 1;
	slice = (rtex->surface.level[level].nblk_x * rtex->surface.level[level].nblk_y) / 64;
	if (slice) {
		slice = slice - 1;
	}

	color_attrib = S_028C74_TILE_MODE_INDEX(8);
	switch (rtex->surface.level[level].mode) {
	case RADEON_SURF_MODE_LINEAR_ALIGNED:
		color_attrib = S_028C74_TILE_MODE_INDEX(8);
		break;
	case RADEON_SURF_MODE_1D:
		color_attrib = S_028C74_TILE_MODE_INDEX(9);
		break;
	case RADEON_SURF_MODE_2D:
		if (rtex->resource.b.b.bind & PIPE_BIND_SCANOUT) {
			switch (blocksize) {
			case 1:
				color_attrib = S_028C74_TILE_MODE_INDEX(10);
				break;
			case 2:
				color_attrib = S_028C74_TILE_MODE_INDEX(11);
				break;
			case 4:
				color_attrib = S_028C74_TILE_MODE_INDEX(12);
				break;
			}
			break;
		} else switch (blocksize) {
		case 1:
			color_attrib = S_028C74_TILE_MODE_INDEX(14);
			break;
		case 2:
			color_attrib = S_028C74_TILE_MODE_INDEX(15);
			break;
		case 4:
			color_attrib = S_028C74_TILE_MODE_INDEX(16);
			break;
		case 8:
			color_attrib = S_028C74_TILE_MODE_INDEX(17);
			break;
		default:
			color_attrib = S_028C74_TILE_MODE_INDEX(13);
		}
		break;
	}

	desc = util_format_description(surf->base.format);
	for (i = 0; i < 4; i++) {
		if (desc->channel[i].type != UTIL_FORMAT_TYPE_VOID) {
			break;
		}
	}
	if (desc->channel[i].type == UTIL_FORMAT_TYPE_FLOAT) {
		ntype = V_028C70_NUMBER_FLOAT;
	} else {
		ntype = V_028C70_NUMBER_UNORM;
		if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
			ntype = V_028C70_NUMBER_SRGB;
		else if (desc->channel[i].type == UTIL_FORMAT_TYPE_SIGNED) {
			if (desc->channel[i].normalized)
				ntype = V_028C70_NUMBER_SNORM;
			else if (desc->channel[i].pure_integer)
				ntype = V_028C70_NUMBER_SINT;
		} else if (desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED) {
			if (desc->channel[i].normalized)
				ntype = V_028C70_NUMBER_UNORM;
			else if (desc->channel[i].pure_integer)
				ntype = V_028C70_NUMBER_UINT;
		}
	}

	format = si_translate_colorformat(surf->base.format);
	swap = si_translate_colorswap(surf->base.format);
	if (rtex->resource.b.b.usage == PIPE_USAGE_STAGING) {
		endian = V_028C70_ENDIAN_NONE;
	} else {
		endian = si_colorformat_endian_swap(format);
	}

	/* blend clamp should be set for all NORM/SRGB types */
	if (ntype == V_028C70_NUMBER_UNORM ||
	    ntype == V_028C70_NUMBER_SNORM ||
	    ntype == V_028C70_NUMBER_SRGB)
		blend_clamp = 1;

	/* set blend bypass according to docs if SINT/UINT or
	   8/24 COLOR variants */
	if (ntype == V_028C70_NUMBER_UINT || ntype == V_028C70_NUMBER_SINT ||
	    format == V_028C70_COLOR_8_24 || format == V_028C70_COLOR_24_8 ||
	    format == V_028C70_COLOR_X24_8_32_FLOAT) {
		blend_clamp = 0;
		blend_bypass = 1;
	}

	color_info = S_028C70_FORMAT(format) |
		S_028C70_COMP_SWAP(swap) |
		S_028C70_BLEND_CLAMP(blend_clamp) |
		S_028C70_BLEND_BYPASS(blend_bypass) |
		S_028C70_NUMBER_TYPE(ntype) |
		S_028C70_ENDIAN(endian);

	rctx->alpha_ref_dirty = true;

	offset += r600_resource_va(rctx->context.screen, state->cbufs[cb]->texture);
	offset >>= 8;

	/* FIXME handle enabling of CB beyond BASE8 which has different offset */
	si_pm4_add_bo(pm4, &rtex->resource, RADEON_USAGE_READWRITE);
	si_pm4_set_reg(pm4, R_028C60_CB_COLOR0_BASE + cb * 0x3C, offset);
	si_pm4_set_reg(pm4, R_028C64_CB_COLOR0_PITCH + cb * 0x3C, S_028C64_TILE_MAX(pitch));
	si_pm4_set_reg(pm4, R_028C68_CB_COLOR0_SLICE + cb * 0x3C, S_028C68_TILE_MAX(slice));

	if (rtex->surface.level[level].mode < RADEON_SURF_MODE_1D) {
		si_pm4_set_reg(pm4, R_028C6C_CB_COLOR0_VIEW + cb * 0x3C, 0x00000000);
	} else {
		si_pm4_set_reg(pm4, R_028C6C_CB_COLOR0_VIEW + cb * 0x3C,
			       S_028C6C_SLICE_START(state->cbufs[cb]->u.tex.first_layer) |
			       S_028C6C_SLICE_MAX(state->cbufs[cb]->u.tex.last_layer));
	}
	si_pm4_set_reg(pm4, R_028C70_CB_COLOR0_INFO + cb * 0x3C, color_info);
	si_pm4_set_reg(pm4, R_028C74_CB_COLOR0_ATTRIB + cb * 0x3C, color_attrib);

	/* Determine pixel shader export format */
	max_comp_size = si_colorformat_max_comp_size(format);
	if (ntype == V_028C70_NUMBER_SRGB ||
	    ((ntype == V_028C70_NUMBER_UNORM || ntype == V_028C70_NUMBER_SNORM) &&
	     max_comp_size <= 10) ||
	    (ntype == V_028C70_NUMBER_FLOAT && max_comp_size <= 16)) {
		rctx->export_16bpc |= 1 << cb;
		rctx->spi_shader_col_format |= V_028714_SPI_SHADER_FP16_ABGR << (4 * cb);
	} else
		rctx->spi_shader_col_format |= V_028714_SPI_SHADER_32_ABGR << (4 * cb);
}

static void si_db(struct r600_context *rctx, struct si_pm4_state *pm4,
		  const struct pipe_framebuffer_state *state)
{
	struct r600_resource_texture *rtex;
	struct r600_surface *surf;
	unsigned level, pitch, slice, format;
	uint32_t z_info, s_info;
	uint64_t z_offs, s_offs;

	if (state->zsbuf == NULL) {
		si_pm4_set_reg(pm4, R_028040_DB_Z_INFO, 0);
		si_pm4_set_reg(pm4, R_028044_DB_STENCIL_INFO, 0);
		return;
	}

	surf = (struct r600_surface *)state->zsbuf;
	level = surf->base.u.tex.level;
	rtex = (struct r600_resource_texture*)surf->base.texture;

	format = si_translate_dbformat(rtex->real_format);

	z_offs = r600_resource_va(rctx->context.screen, surf->base.texture);
	z_offs += rtex->surface.level[level].offset;

	s_offs = r600_resource_va(rctx->context.screen, surf->base.texture);
	s_offs += rtex->surface.stencil_offset;
	z_offs += rtex->surface.level[level].offset / 4;

	z_offs >>= 8;
	s_offs >>= 8;

	pitch = (rtex->surface.level[level].nblk_x / 8) - 1;
	slice = (rtex->surface.level[level].nblk_x * rtex->surface.level[level].nblk_y) / 64;
	if (slice) {
		slice = slice - 1;
	}

	z_info = S_028040_FORMAT(format);
	s_info = S_028044_FORMAT(1);

	if (rtex->surface.level[level].mode == RADEON_SURF_MODE_1D) {
		z_info |= S_028040_TILE_MODE_INDEX(4);
		s_info |= S_028044_TILE_MODE_INDEX(4);

	} else if (rtex->surface.level[level].mode == RADEON_SURF_MODE_2D) {
		switch (format) {
		case V_028040_Z_16:
			z_info |= S_028040_TILE_MODE_INDEX(5);
			s_info |= S_028044_TILE_MODE_INDEX(5);
			break;
		case V_028040_Z_24:
		case V_028040_Z_32_FLOAT:
			z_info |= S_028040_TILE_MODE_INDEX(6);
			s_info |= S_028044_TILE_MODE_INDEX(6);
			break;
		default:
			z_info |= S_028040_TILE_MODE_INDEX(7);
			s_info |= S_028044_TILE_MODE_INDEX(7);
		}

	} else {
		R600_ERR("Invalid DB tiling mode %d!\n",
			 rtex->surface.level[level].mode);
		si_pm4_set_reg(pm4, R_028040_DB_Z_INFO, 0);
		si_pm4_set_reg(pm4, R_028044_DB_STENCIL_INFO, 0);
		return;
	}

	si_pm4_set_reg(pm4, R_028008_DB_DEPTH_VIEW,
		       S_028008_SLICE_START(state->zsbuf->u.tex.first_layer) |
		       S_028008_SLICE_MAX(state->zsbuf->u.tex.last_layer));

	si_pm4_set_reg(pm4, R_02803C_DB_DEPTH_INFO, 0x1);
	if (format != ~0U) {
		si_pm4_set_reg(pm4, R_028040_DB_Z_INFO, z_info);

	} else {
		si_pm4_set_reg(pm4, R_028040_DB_Z_INFO, 0);
	}

	if (rtex->surface.flags & RADEON_SURF_SBUFFER) {
		si_pm4_set_reg(pm4, R_028044_DB_STENCIL_INFO, s_info);
	} else {
		si_pm4_set_reg(pm4, R_028044_DB_STENCIL_INFO, 0);
	}

	si_pm4_add_bo(pm4, &rtex->resource, RADEON_USAGE_READWRITE);
	si_pm4_set_reg(pm4, R_028048_DB_Z_READ_BASE, z_offs);
	si_pm4_set_reg(pm4, R_02804C_DB_STENCIL_READ_BASE, s_offs);
	si_pm4_set_reg(pm4, R_028050_DB_Z_WRITE_BASE, z_offs);
	si_pm4_set_reg(pm4, R_028054_DB_STENCIL_WRITE_BASE, s_offs);

	si_pm4_set_reg(pm4, R_028058_DB_DEPTH_SIZE, S_028058_PITCH_TILE_MAX(pitch));
	si_pm4_set_reg(pm4, R_02805C_DB_DEPTH_SLICE, S_02805C_SLICE_TILE_MAX(slice));
}

static void si_set_framebuffer_state(struct pipe_context *ctx,
				     const struct pipe_framebuffer_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);
	uint32_t shader_mask, tl, br;
	int tl_x, tl_y, br_x, br_y;

	if (pm4 == NULL)
		return;

	si_pm4_inval_fb_cache(pm4, state->nr_cbufs);

	if (state->zsbuf)
		si_pm4_inval_zsbuf_cache(pm4);

	util_copy_framebuffer_state(&rctx->framebuffer, state);

	/* build states */
	rctx->have_depth_fb = 0;
	rctx->export_16bpc = 0;
	rctx->spi_shader_col_format = 0;
	for (int i = 0; i < state->nr_cbufs; i++) {
		si_cb(rctx, pm4, state, i);
	}
	assert(!(rctx->export_16bpc & ~0xff));
	si_db(rctx, pm4, state);

	shader_mask = 0;
	for (int i = 0; i < state->nr_cbufs; i++) {
		shader_mask |= 0xf << (i * 4);
	}
	tl_x = 0;
	tl_y = 0;
	br_x = state->width;
	br_y = state->height;
#if 0 /* These shouldn't be necessary on SI, see PA_SC_ENHANCE register */
	/* EG hw workaround */
	if (br_x == 0)
		tl_x = 1;
	if (br_y == 0)
		tl_y = 1;
	/* cayman hw workaround */
	if (rctx->chip_class == CAYMAN) {
		if (br_x == 1 && br_y == 1)
			br_x = 2;
	}
#endif
	tl = S_028240_TL_X(tl_x) | S_028240_TL_Y(tl_y);
	br = S_028244_BR_X(br_x) | S_028244_BR_Y(br_y);

	si_pm4_set_reg(pm4, R_028240_PA_SC_GENERIC_SCISSOR_TL, tl);
	si_pm4_set_reg(pm4, R_028244_PA_SC_GENERIC_SCISSOR_BR, br);
	si_pm4_set_reg(pm4, R_028250_PA_SC_VPORT_SCISSOR_0_TL, tl);
	si_pm4_set_reg(pm4, R_028254_PA_SC_VPORT_SCISSOR_0_BR, br);
	si_pm4_set_reg(pm4, R_028030_PA_SC_SCREEN_SCISSOR_TL, tl);
	si_pm4_set_reg(pm4, R_028034_PA_SC_SCREEN_SCISSOR_BR, br);
	si_pm4_set_reg(pm4, R_028204_PA_SC_WINDOW_SCISSOR_TL, tl);
	si_pm4_set_reg(pm4, R_028208_PA_SC_WINDOW_SCISSOR_BR, br);
	si_pm4_set_reg(pm4, R_028200_PA_SC_WINDOW_OFFSET, 0x00000000);
	si_pm4_set_reg(pm4, R_028230_PA_SC_EDGERULE, 0xAAAAAAAA);
	si_pm4_set_reg(pm4, R_02823C_CB_SHADER_MASK, shader_mask);
	si_pm4_set_reg(pm4, R_028714_SPI_SHADER_COL_FORMAT,
		       rctx->spi_shader_col_format);
	si_pm4_set_reg(pm4, R_028BE0_PA_SC_AA_CONFIG, 0x00000000);

	si_pm4_set_state(rctx, framebuffer, pm4);
	si_update_fb_rs_state(rctx);
	si_update_fb_blend_state(rctx);
}

/*
 * shaders
 */

/* Compute the key for the hw shader variant */
static INLINE unsigned si_shader_selector_key(struct pipe_context *ctx,
					      struct si_pipe_shader_selector *sel)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	unsigned key = 0;

	if (sel->type == PIPE_SHADER_FRAGMENT) {
		if (sel->fs_write_all)
			key |= rctx->framebuffer.nr_cbufs;
		key |= rctx->export_16bpc << 4;
		/*if (rctx->queued.named.rasterizer)
			  key |= rctx->queued.named.rasterizer->flatshade << 12;*/
		/*key |== rctx->two_side << 13;*/
	}

	return key;
}

/* Select the hw shader variant depending on the current state.
 * (*dirty) is set to 1 if current variant was changed */
int si_shader_select(struct pipe_context *ctx,
		     struct si_pipe_shader_selector *sel,
		     unsigned *dirty)
{
	unsigned key;
	struct si_pipe_shader * shader = NULL;
	int r;

	key = si_shader_selector_key(ctx, sel);

	/* Check if we don't need to change anything.
	 * This path is also used for most shaders that don't need multiple
	 * variants, it will cost just a computation of the key and this
	 * test. */
	if (likely(sel->current && sel->current->key == key)) {
		return 0;
	}

	/* lookup if we have other variants in the list */
	if (sel->num_shaders > 1) {
		struct si_pipe_shader *p = sel->current, *c = p->next_variant;

		while (c && c->key != key) {
			p = c;
			c = c->next_variant;
		}

		if (c) {
			p->next_variant = c->next_variant;
			shader = c;
		}
	}

	if (unlikely(!shader)) {
		shader = CALLOC(1, sizeof(struct si_pipe_shader));
		shader->selector = sel;

		r = si_pipe_shader_create(ctx, shader);
		if (unlikely(r)) {
			R600_ERR("Failed to build shader variant (type=%u, key=%u) %d\n",
				 sel->type, key, r);
			sel->current = NULL;
			return r;
		}

		/* We don't know the value of fs_write_all property until we built
		 * at least one variant, so we may need to recompute the key (include
		 * rctx->framebuffer.nr_cbufs) after building first variant. */
		if (sel->type == PIPE_SHADER_FRAGMENT &&
		    sel->num_shaders == 0 &&
		    shader->shader.fs_write_all) {
			sel->fs_write_all = 1;
			key = si_shader_selector_key(ctx, sel);
		}

		shader->key = key;
		sel->num_shaders++;
	}

	if (dirty)
		*dirty = 1;

	shader->next_variant = sel->current;
	sel->current = shader;

	return 0;
}

static void *si_create_shader_state(struct pipe_context *ctx,
				    const struct pipe_shader_state *state,
				    unsigned pipe_shader_type)
{
	struct si_pipe_shader_selector *sel = CALLOC_STRUCT(si_pipe_shader_selector);
	int r;

	sel->type = pipe_shader_type;
	sel->tokens = tgsi_dup_tokens(state->tokens);
	sel->so = state->stream_output;

	r = si_shader_select(ctx, sel, NULL);
	if (r) {
	    free(sel);
	    return NULL;
	}

	return sel;
}

static void *si_create_fs_state(struct pipe_context *ctx,
				const struct pipe_shader_state *state)
{
	return si_create_shader_state(ctx, state, PIPE_SHADER_FRAGMENT);
}

static void *si_create_vs_state(struct pipe_context *ctx,
				const struct pipe_shader_state *state)
{
	return si_create_shader_state(ctx, state, PIPE_SHADER_VERTEX);
}

static void si_bind_vs_shader(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pipe_shader_selector *sel = state;

	if (rctx->vs_shader == sel)
		return;

	rctx->shader_dirty = true;
	rctx->vs_shader = sel;

	if (sel && sel->current)
		si_pm4_bind_state(rctx, vs, sel->current->pm4);
	else
		si_pm4_bind_state(rctx, vs, rctx->dummy_pixel_shader->pm4);
}

static void si_bind_ps_shader(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pipe_shader_selector *sel = state;

	if (rctx->ps_shader == sel)
		return;

	rctx->shader_dirty = true;
	rctx->ps_shader = sel;

	if (sel && sel->current)
		si_pm4_bind_state(rctx, ps, sel->current->pm4);
	else
		si_pm4_bind_state(rctx, ps, rctx->dummy_pixel_shader->pm4);
}

static void si_delete_shader_selector(struct pipe_context *ctx,
				      struct si_pipe_shader_selector *sel)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pipe_shader *p = sel->current, *c;

	while (p) {
		c = p->next_variant;
		si_pm4_delete_state(rctx, vs, p->pm4);
		si_pipe_shader_destroy(ctx, p);
		free(p);
		p = c;
	}

	free(sel->tokens);
	free(sel);
 }

static void si_delete_vs_shader(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pipe_shader_selector *sel = (struct si_pipe_shader_selector *)state;

	if (rctx->vs_shader == sel) {
		rctx->vs_shader = NULL;
	}

	si_delete_shader_selector(ctx, sel);
}

static void si_delete_ps_shader(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pipe_shader_selector *sel = (struct si_pipe_shader_selector *)state;

	if (rctx->ps_shader == sel) {
		rctx->ps_shader = NULL;
	}

	si_delete_shader_selector(ctx, sel);
}

/*
 * Samplers
 */

static struct pipe_sampler_view *si_create_sampler_view(struct pipe_context *ctx,
							struct pipe_resource *texture,
							const struct pipe_sampler_view *state)
{
	struct si_pipe_sampler_view *view = CALLOC_STRUCT(si_pipe_sampler_view);
	struct r600_resource_texture *tmp = (struct r600_resource_texture*)texture;
	const struct util_format_description *desc = util_format_description(state->format);
	unsigned blocksize = util_format_get_blocksize(tmp->real_format);
	unsigned format, num_format, /*endian,*/ tiling_index;
	uint32_t pitch = 0;
	unsigned char state_swizzle[4], swizzle[4];
	unsigned height, depth, width, offset_level, last_level;
	int first_non_void;
	uint64_t va;

	if (view == NULL)
		return NULL;

	/* initialize base object */
	view->base = *state;
	view->base.texture = NULL;
	pipe_reference(NULL, &texture->reference);
	view->base.texture = texture;
	view->base.reference.count = 1;
	view->base.context = ctx;

	state_swizzle[0] = state->swizzle_r;
	state_swizzle[1] = state->swizzle_g;
	state_swizzle[2] = state->swizzle_b;
	state_swizzle[3] = state->swizzle_a;
	util_format_compose_swizzles(desc->swizzle, state_swizzle, swizzle);

	first_non_void = util_format_get_first_non_void_channel(state->format);
	switch (desc->channel[first_non_void].type) {
	case UTIL_FORMAT_TYPE_FLOAT:
		num_format = V_008F14_IMG_NUM_FORMAT_FLOAT;
		break;
	case UTIL_FORMAT_TYPE_SIGNED:
		num_format = V_008F14_IMG_NUM_FORMAT_SNORM;
		break;
	case UTIL_FORMAT_TYPE_UNSIGNED:
	default:
		num_format = V_008F14_IMG_NUM_FORMAT_UNORM;
	}

	format = si_translate_texformat(ctx->screen, state->format, desc, first_non_void);
	if (format == ~0) {
		format = 0;
	}

	if (tmp->depth && !tmp->is_flushing_texture) {
		r600_texture_depth_flush(ctx, texture, TRUE);
		tmp = tmp->flushed_depth_texture;
	}

	/* not supported any more */
	//endian = si_colorformat_endian_swap(format);

	offset_level = state->u.tex.first_level;
	last_level = state->u.tex.last_level - offset_level;
	width = tmp->surface.level[offset_level].npix_x;
	height = tmp->surface.level[offset_level].npix_y;
	depth = tmp->surface.level[offset_level].npix_z;
	pitch = tmp->surface.level[offset_level].nblk_x * util_format_get_blockwidth(state->format);

	if (texture->target == PIPE_TEXTURE_1D_ARRAY) {
	        height = 1;
		depth = texture->array_size;
	} else if (texture->target == PIPE_TEXTURE_2D_ARRAY) {
		depth = texture->array_size;
	}

	tiling_index = 8;
	switch (tmp->surface.level[state->u.tex.first_level].mode) {
	case RADEON_SURF_MODE_LINEAR_ALIGNED:
		tiling_index = 8;
		break;
	case RADEON_SURF_MODE_1D:
		tiling_index = 9;
		break;
	case RADEON_SURF_MODE_2D:
		if (tmp->resource.b.b.bind & PIPE_BIND_SCANOUT) {
			switch (blocksize) {
			case 1:
				tiling_index = 10;
				break;
			case 2:
				tiling_index = 11;
				break;
			case 4:
				tiling_index = 12;
				break;
			}
			break;
		} else switch (blocksize) {
		case 1:
			tiling_index = 14;
			break;
		case 2:
			tiling_index = 15;
			break;
		case 4:
			tiling_index = 16;
			break;
		case 8:
			tiling_index = 17;
			break;
		default:
			tiling_index = 13;
		}
		break;
	}

	va = r600_resource_va(ctx->screen, texture);
	va += tmp->surface.level[offset_level].offset;
	view->state[0] = va >> 8;
	view->state[1] = (S_008F14_BASE_ADDRESS_HI(va >> 40) |
			  S_008F14_DATA_FORMAT(format) |
			  S_008F14_NUM_FORMAT(num_format));
	view->state[2] = (S_008F18_WIDTH(width - 1) |
			  S_008F18_HEIGHT(height - 1));
	view->state[3] = (S_008F1C_DST_SEL_X(si_map_swizzle(swizzle[0])) |
			  S_008F1C_DST_SEL_Y(si_map_swizzle(swizzle[1])) |
			  S_008F1C_DST_SEL_Z(si_map_swizzle(swizzle[2])) |
			  S_008F1C_DST_SEL_W(si_map_swizzle(swizzle[3])) |
			  S_008F1C_BASE_LEVEL(offset_level) |
			  S_008F1C_LAST_LEVEL(last_level) |
			  S_008F1C_TILING_INDEX(tiling_index) |
			  S_008F1C_TYPE(si_tex_dim(texture->target)));
	view->state[4] = (S_008F20_DEPTH(depth - 1) | S_008F20_PITCH(pitch - 1));
	view->state[5] = (S_008F24_BASE_ARRAY(state->u.tex.first_layer) |
			  S_008F24_LAST_ARRAY(state->u.tex.last_layer));
	view->state[6] = 0;
	view->state[7] = 0;

	return &view->base;
}

static void si_sampler_view_destroy(struct pipe_context *ctx,
				    struct pipe_sampler_view *state)
{
	struct r600_pipe_sampler_view *resource = (struct r600_pipe_sampler_view *)state;

	pipe_resource_reference(&state->texture, NULL);
	FREE(resource);
}

static void *si_create_sampler_state(struct pipe_context *ctx,
				     const struct pipe_sampler_state *state)
{
	struct si_pipe_sampler_state *rstate = CALLOC_STRUCT(si_pipe_sampler_state);
	union util_color uc;
	unsigned aniso_flag_offset = state->max_anisotropy > 1 ? 2 : 0;
	unsigned border_color_type;

	if (rstate == NULL) {
		return NULL;
	}

	util_pack_color(state->border_color.f, PIPE_FORMAT_B8G8R8A8_UNORM, &uc);
	switch (uc.ui) {
	case 0x000000FF:
		border_color_type = V_008F3C_SQ_TEX_BORDER_COLOR_OPAQUE_BLACK;
		break;
	case 0x00000000:
		border_color_type = V_008F3C_SQ_TEX_BORDER_COLOR_TRANS_BLACK;
		break;
	case 0xFFFFFFFF:
		border_color_type = V_008F3C_SQ_TEX_BORDER_COLOR_OPAQUE_WHITE;
		break;
	default: /* Use border color pointer */
		border_color_type = V_008F3C_SQ_TEX_BORDER_COLOR_REGISTER;
	}

	rstate->val[0] = (S_008F30_CLAMP_X(si_tex_wrap(state->wrap_s)) |
			  S_008F30_CLAMP_Y(si_tex_wrap(state->wrap_t)) |
			  S_008F30_CLAMP_Z(si_tex_wrap(state->wrap_r)) |
			  (state->max_anisotropy & 0x7) << 9 | /* XXX */
			  S_008F30_DEPTH_COMPARE_FUNC(si_tex_compare(state->compare_func)) |
			  S_008F30_FORCE_UNNORMALIZED(!state->normalized_coords) |
			  aniso_flag_offset << 16 | /* XXX */
			  S_008F30_DISABLE_CUBE_WRAP(!state->seamless_cube_map));
	rstate->val[1] = (S_008F34_MIN_LOD(S_FIXED(CLAMP(state->min_lod, 0, 15), 8)) |
			  S_008F34_MAX_LOD(S_FIXED(CLAMP(state->max_lod, 0, 15), 8)));
	rstate->val[2] = (S_008F38_LOD_BIAS(S_FIXED(CLAMP(state->lod_bias, -16, 16), 8)) |
			  S_008F38_XY_MAG_FILTER(si_tex_filter(state->mag_img_filter)) |
			  S_008F38_XY_MIN_FILTER(si_tex_filter(state->min_img_filter)) |
			  S_008F38_MIP_FILTER(si_tex_mipfilter(state->min_mip_filter)));
	rstate->val[3] = S_008F3C_BORDER_COLOR_TYPE(border_color_type);

#if 0
	if (border_color_type == 3) {
		si_pm4_set_reg(pm4, R_00A404_TD_PS_SAMPLER0_BORDER_RED, fui(state->border_color.f[0]));
		si_pm4_set_reg(pm4, R_00A408_TD_PS_SAMPLER0_BORDER_GREEN, fui(state->border_color.f[1]));
		si_pm4_set_reg(pm4, R_00A40C_TD_PS_SAMPLER0_BORDER_BLUE, fui(state->border_color.f[2]));
		si_pm4_set_reg(pm4, R_00A410_TD_PS_SAMPLER0_BORDER_ALPHA, fui(state->border_color.f[3]));
	}
#endif
	return rstate;
}

static void si_set_vs_sampler_view(struct pipe_context *ctx, unsigned count,
				   struct pipe_sampler_view **views)
{
	assert(count == 0);
}

static void si_set_ps_sampler_view(struct pipe_context *ctx, unsigned count,
				   struct pipe_sampler_view **views)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pipe_sampler_view **resource = (struct si_pipe_sampler_view **)views;
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);
	int i, j;
	int has_depth = 0;

	if (!count)
		goto out;

	si_pm4_inval_texture_cache(pm4);

	si_pm4_sh_data_begin(pm4);
	for (i = 0; i < count; i++) {
		pipe_sampler_view_reference(
			(struct pipe_sampler_view **)&rctx->ps_samplers.views[i],
			views[i]);

		if (views[i]) {
			struct r600_resource_texture *tex = (void *)resource[i]->base.texture;

			si_pm4_add_bo(pm4, &tex->resource, RADEON_USAGE_READ);

			for (j = 0; j < Elements(resource[i]->state); ++j) {
				si_pm4_sh_data_add(pm4, resource[i]->state[j]);
			}
		}
	}

	for (i = count; i < NUM_TEX_UNITS; i++) {
		if (rctx->ps_samplers.views[i])
			pipe_sampler_view_reference((struct pipe_sampler_view **)&rctx->ps_samplers.views[i], NULL);
	}

	si_pm4_sh_data_end(pm4, R_00B040_SPI_SHADER_USER_DATA_PS_4);

out:
	si_pm4_set_state(rctx, ps_sampler_views, pm4);
	rctx->have_depth_texture = has_depth;
	rctx->ps_samplers.n_views = count;
}

static void si_bind_vs_sampler(struct pipe_context *ctx, unsigned count, void **states)
{
	assert(count == 0);
}

static void si_bind_ps_sampler(struct pipe_context *ctx, unsigned count, void **states)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pipe_sampler_state **rstates = (struct si_pipe_sampler_state **)states;
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);
	int i, j;

	if (!count)
		goto out;

	si_pm4_inval_texture_cache(pm4);

	si_pm4_sh_data_begin(pm4);
	for (i = 0; i < count; i++) {
		for (j = 0; j < Elements(rstates[i]->val); ++j) {
			si_pm4_sh_data_add(pm4, rstates[i]->val[j]);
		}
	}
	si_pm4_sh_data_end(pm4, R_00B038_SPI_SHADER_USER_DATA_PS_2);

	memcpy(rctx->ps_samplers.samplers, states, sizeof(void*) * count);

out:
	si_pm4_set_state(rctx, ps_sampler, pm4);
	rctx->ps_samplers.n_samplers = count;
}

static void si_set_sample_mask(struct pipe_context *pipe, unsigned sample_mask)
{
}

static void si_delete_sampler_state(struct pipe_context *ctx, void *state)
{
	free(state);
}

/*
 * Constants
 */
static void si_set_constant_buffer(struct pipe_context *ctx, uint shader, uint index,
			    struct pipe_constant_buffer *cb)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_resource *rbuffer = cb ? si_resource(cb->buffer) : NULL;
	struct si_pm4_state *pm4;
	uint64_t va_offset;
	uint32_t offset;

	/* Note that the state tracker can unbind constant buffers by
	 * passing NULL here.
	 */
	if (cb == NULL)
		return;

	pm4 = CALLOC_STRUCT(si_pm4_state);
	si_pm4_inval_shader_cache(pm4);

	if (cb->user_buffer)
		r600_upload_const_buffer(rctx, &rbuffer, cb->user_buffer, cb->buffer_size, &offset);
	else
		offset = 0;
	va_offset = r600_resource_va(ctx->screen, (void*)rbuffer);
	va_offset += offset;

	si_pm4_add_bo(pm4, rbuffer, RADEON_USAGE_READ);

	switch (shader) {
	case PIPE_SHADER_VERTEX:
		si_pm4_set_reg(pm4, R_00B130_SPI_SHADER_USER_DATA_VS_0, va_offset);
		si_pm4_set_reg(pm4, R_00B134_SPI_SHADER_USER_DATA_VS_1, va_offset >> 32);
		si_pm4_set_state(rctx, vs_const, pm4);
		break;

	case PIPE_SHADER_FRAGMENT:
		si_pm4_set_reg(pm4, R_00B030_SPI_SHADER_USER_DATA_PS_0, va_offset);
		si_pm4_set_reg(pm4, R_00B034_SPI_SHADER_USER_DATA_PS_1, va_offset >> 32);
		si_pm4_set_state(rctx, ps_const, pm4);
		break;

	default:
		R600_ERR("unsupported %d\n", shader);
	}

	if (cb->buffer != &rbuffer->b.b)
		si_resource_reference(&rbuffer, NULL);
}

/*
 * Vertex elements & buffers
 */

static void *si_create_vertex_elements(struct pipe_context *ctx,
				       unsigned count,
				       const struct pipe_vertex_element *elements)
{
	struct si_vertex_element *v = CALLOC_STRUCT(si_vertex_element);
	int i;

	assert(count < PIPE_MAX_ATTRIBS);
	if (!v)
		return NULL;

	v->count = count;
	for (i = 0; i < count; ++i) {
		const struct util_format_description *desc;
		unsigned data_format, num_format;
		int first_non_void;

		desc = util_format_description(elements[i].src_format);
		first_non_void = util_format_get_first_non_void_channel(elements[i].src_format);
		data_format = si_translate_vertexformat(ctx->screen, elements[i].src_format,
							desc, first_non_void);

		switch (desc->channel[first_non_void].type) {
		case UTIL_FORMAT_TYPE_FIXED:
			num_format = V_008F0C_BUF_NUM_FORMAT_USCALED; /* XXX */
			break;
		case UTIL_FORMAT_TYPE_SIGNED:
			num_format = V_008F0C_BUF_NUM_FORMAT_SNORM;
			break;
		case UTIL_FORMAT_TYPE_UNSIGNED:
			num_format = V_008F0C_BUF_NUM_FORMAT_UNORM;
			break;
		case UTIL_FORMAT_TYPE_FLOAT:
		default:
			num_format = V_008F14_IMG_NUM_FORMAT_FLOAT;
		}

		v->rsrc_word3[i] = S_008F0C_DST_SEL_X(si_map_swizzle(desc->swizzle[0])) |
				   S_008F0C_DST_SEL_Y(si_map_swizzle(desc->swizzle[1])) |
				   S_008F0C_DST_SEL_Z(si_map_swizzle(desc->swizzle[2])) |
				   S_008F0C_DST_SEL_W(si_map_swizzle(desc->swizzle[3])) |
				   S_008F0C_NUM_FORMAT(num_format) |
				   S_008F0C_DATA_FORMAT(data_format);
	}
	memcpy(v->elements, elements, sizeof(struct pipe_vertex_element) * count);

	return v;
}

static void si_bind_vertex_elements(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_vertex_element *v = (struct si_vertex_element*)state;

	rctx->vertex_elements = v;
}

static void si_delete_vertex_element(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	if (rctx->vertex_elements == state)
		rctx->vertex_elements = NULL;
	FREE(state);
}

static void si_set_vertex_buffers(struct pipe_context *ctx, unsigned count,
				  const struct pipe_vertex_buffer *buffers)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	util_copy_vertex_buffers(rctx->vertex_buffer, &rctx->nr_vertex_buffers, buffers, count);
}

static void si_set_index_buffer(struct pipe_context *ctx,
				const struct pipe_index_buffer *ib)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	if (ib) {
		pipe_resource_reference(&rctx->index_buffer.buffer, ib->buffer);
	        memcpy(&rctx->index_buffer, ib, sizeof(*ib));
	} else {
		pipe_resource_reference(&rctx->index_buffer.buffer, NULL);
	}
}

/*
 * Misc
 */
static void si_set_polygon_stipple(struct pipe_context *ctx,
				   const struct pipe_poly_stipple *state)
{
}

static void si_texture_barrier(struct pipe_context *ctx)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);

	si_pm4_inval_texture_cache(pm4);
	si_pm4_inval_fb_cache(pm4, rctx->framebuffer.nr_cbufs);
	si_pm4_set_state(rctx, texture_barrier, pm4);
}

void si_init_state_functions(struct r600_context *rctx)
{
	rctx->context.create_blend_state = si_create_blend_state;
	rctx->context.bind_blend_state = si_bind_blend_state;
	rctx->context.delete_blend_state = si_delete_blend_state;
	rctx->context.set_blend_color = si_set_blend_color;

	rctx->context.create_rasterizer_state = si_create_rs_state;
	rctx->context.bind_rasterizer_state = si_bind_rs_state;
	rctx->context.delete_rasterizer_state = si_delete_rs_state;

	rctx->context.create_depth_stencil_alpha_state = si_create_dsa_state;
	rctx->context.bind_depth_stencil_alpha_state = si_bind_dsa_state;
	rctx->context.delete_depth_stencil_alpha_state = si_delete_dsa_state;
	rctx->custom_dsa_flush = si_create_db_flush_dsa(rctx);

	rctx->context.set_clip_state = si_set_clip_state;
	rctx->context.set_scissor_state = si_set_scissor_state;
	rctx->context.set_viewport_state = si_set_viewport_state;
	rctx->context.set_stencil_ref = si_set_pipe_stencil_ref;

	rctx->context.set_framebuffer_state = si_set_framebuffer_state;

	rctx->context.create_vs_state = si_create_vs_state;
	rctx->context.create_fs_state = si_create_fs_state;
	rctx->context.bind_vs_state = si_bind_vs_shader;
	rctx->context.bind_fs_state = si_bind_ps_shader;
	rctx->context.delete_vs_state = si_delete_vs_shader;
	rctx->context.delete_fs_state = si_delete_ps_shader;

	rctx->context.create_sampler_state = si_create_sampler_state;
	rctx->context.bind_vertex_sampler_states = si_bind_vs_sampler;
	rctx->context.bind_fragment_sampler_states = si_bind_ps_sampler;
	rctx->context.delete_sampler_state = si_delete_sampler_state;

	rctx->context.create_sampler_view = si_create_sampler_view;
	rctx->context.set_vertex_sampler_views = si_set_vs_sampler_view;
	rctx->context.set_fragment_sampler_views = si_set_ps_sampler_view;
	rctx->context.sampler_view_destroy = si_sampler_view_destroy;

	rctx->context.set_sample_mask = si_set_sample_mask;

	rctx->context.set_constant_buffer = si_set_constant_buffer;

	rctx->context.create_vertex_elements_state = si_create_vertex_elements;
	rctx->context.bind_vertex_elements_state = si_bind_vertex_elements;
	rctx->context.delete_vertex_elements_state = si_delete_vertex_element;
	rctx->context.set_vertex_buffers = si_set_vertex_buffers;
	rctx->context.set_index_buffer = si_set_index_buffer;

	rctx->context.create_stream_output_target = si_create_so_target;
	rctx->context.stream_output_target_destroy = si_so_target_destroy;
	rctx->context.set_stream_output_targets = si_set_so_targets;

	rctx->context.texture_barrier = si_texture_barrier;
	rctx->context.set_polygon_stipple = si_set_polygon_stipple;

	rctx->context.draw_vbo = si_draw_vbo;
}

void si_init_config(struct r600_context *rctx)
{
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);

	si_pm4_cmd_begin(pm4, PKT3_CONTEXT_CONTROL);
	si_pm4_cmd_add(pm4, 0x80000000);
	si_pm4_cmd_add(pm4, 0x80000000);
	si_pm4_cmd_end(pm4, false);

	si_pm4_set_reg(pm4, R_028A4C_PA_SC_MODE_CNTL_1, 0x0);

	si_pm4_set_reg(pm4, R_028A10_VGT_OUTPUT_PATH_CNTL, 0x0);
	si_pm4_set_reg(pm4, R_028A14_VGT_HOS_CNTL, 0x0);
	si_pm4_set_reg(pm4, R_028A18_VGT_HOS_MAX_TESS_LEVEL, 0x0);
	si_pm4_set_reg(pm4, R_028A1C_VGT_HOS_MIN_TESS_LEVEL, 0x0);
	si_pm4_set_reg(pm4, R_028A20_VGT_HOS_REUSE_DEPTH, 0x0);
	si_pm4_set_reg(pm4, R_028A24_VGT_GROUP_PRIM_TYPE, 0x0);
	si_pm4_set_reg(pm4, R_028A28_VGT_GROUP_FIRST_DECR, 0x0);
	si_pm4_set_reg(pm4, R_028A2C_VGT_GROUP_DECR, 0x0);
	si_pm4_set_reg(pm4, R_028A30_VGT_GROUP_VECT_0_CNTL, 0x0);
	si_pm4_set_reg(pm4, R_028A34_VGT_GROUP_VECT_1_CNTL, 0x0);
	si_pm4_set_reg(pm4, R_028A38_VGT_GROUP_VECT_0_FMT_CNTL, 0x0);
	si_pm4_set_reg(pm4, R_028A3C_VGT_GROUP_VECT_1_FMT_CNTL, 0x0);
	si_pm4_set_reg(pm4, R_028A40_VGT_GS_MODE, 0x0);
	si_pm4_set_reg(pm4, R_028A84_VGT_PRIMITIVEID_EN, 0x0);
	si_pm4_set_reg(pm4, R_028A8C_VGT_PRIMITIVEID_RESET, 0x0);
	si_pm4_set_reg(pm4, R_028B94_VGT_STRMOUT_CONFIG, 0x0);
	si_pm4_set_reg(pm4, R_028B98_VGT_STRMOUT_BUFFER_CONFIG, 0x0);
	si_pm4_set_reg(pm4, R_028AA8_IA_MULTI_VGT_PARAM,
		       S_028AA8_SWITCH_ON_EOP(1) |
		       S_028AA8_PARTIAL_VS_WAVE_ON(1) |
		       S_028AA8_PRIMGROUP_SIZE(63));
	si_pm4_set_reg(pm4, R_028AB4_VGT_REUSE_OFF, 0x00000000);
	si_pm4_set_reg(pm4, R_028AB8_VGT_VTX_CNT_EN, 0x0);
	si_pm4_set_reg(pm4, R_008A14_PA_CL_ENHANCE, (3 << 1) | 1);

	si_pm4_set_reg(pm4, R_028B54_VGT_SHADER_STAGES_EN, 0);
	si_pm4_set_reg(pm4, R_028BD4_PA_SC_CENTROID_PRIORITY_0, 0x76543210);
	si_pm4_set_reg(pm4, R_028BD8_PA_SC_CENTROID_PRIORITY_1, 0xfedcba98);

	si_pm4_set_reg(pm4, R_028804_DB_EQAA, 0x110000);

	si_pm4_set_state(rctx, init, pm4);
}
