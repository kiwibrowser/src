/*
 * Copyright (C) 2009 Maciej Cencora <m.cencora@gmail.com>
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "radeon_common.h"
#include "r200_context.h"
#include "r200_blit.h"

static inline uint32_t cmdpacket0(struct radeon_screen *rscrn,
                                  int reg, int count)
{
    if (count)
	    return CP_PACKET0(reg, count - 1);
    return CP_PACKET2;
}

/* common formats supported as both textures and render targets */
unsigned r200_check_blit(gl_format mesa_format, uint32_t dst_pitch)
{
    /* XXX others?  BE/LE? */
    switch (mesa_format) {
    case MESA_FORMAT_ARGB8888:
    case MESA_FORMAT_XRGB8888:
    case MESA_FORMAT_RGB565:
    case MESA_FORMAT_ARGB4444:
    case MESA_FORMAT_ARGB1555:
    case MESA_FORMAT_A8:
    case MESA_FORMAT_L8:
    case MESA_FORMAT_I8:
    /* swizzled */
    case MESA_FORMAT_RGBA8888:
    case MESA_FORMAT_RGBA8888_REV:
	    break;
    default:
	    return 0;
    }

    /* Rendering to small buffer doesn't work.
     * Looks like a hw limitation.
     */
    if (dst_pitch < 32)
            return 0;

    /* ??? */
    if (_mesa_get_format_bits(mesa_format, GL_DEPTH_BITS) > 0)
	    return 0;

    return 1;
}

static inline void emit_vtx_state(struct r200_context *r200)
{
    BATCH_LOCALS(&r200->radeon);

    BEGIN_BATCH(14);
    if (r200->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL) {
	    OUT_BATCH_REGVAL(R200_SE_VAP_CNTL_STATUS, 0);
    } else {
	    OUT_BATCH_REGVAL(R200_SE_VAP_CNTL_STATUS, RADEON_TCL_BYPASS);
    }
    OUT_BATCH_REGVAL(R200_SE_VAP_CNTL, (R200_VAP_FORCE_W_TO_ONE |
					(9 << R200_VAP_VF_MAX_VTX_NUM__SHIFT)));
    OUT_BATCH_REGVAL(R200_SE_VTX_STATE_CNTL, 0);
    OUT_BATCH_REGVAL(R200_SE_VTE_CNTL, 0);
    OUT_BATCH_REGVAL(R200_SE_VTX_FMT_0, R200_VTX_XY);
    OUT_BATCH_REGVAL(R200_SE_VTX_FMT_1, (2 << R200_VTX_TEX0_COMP_CNT_SHIFT));
    OUT_BATCH_REGVAL(RADEON_SE_CNTL, (RADEON_DIFFUSE_SHADE_GOURAUD |
				      RADEON_BFACE_SOLID |
				      RADEON_FFACE_SOLID |
				      RADEON_VTX_PIX_CENTER_OGL |
				      RADEON_ROUND_MODE_ROUND |
				      RADEON_ROUND_PREC_4TH_PIX));
    END_BATCH();
}

static void inline emit_tx_setup(struct r200_context *r200,
				 gl_format src_mesa_format,
				 gl_format dst_mesa_format,
				 struct radeon_bo *bo,
				 intptr_t offset,
				 unsigned width,
				 unsigned height,
				 unsigned pitch)
{
    uint32_t txformat = R200_TXFORMAT_NON_POWER2;
    BATCH_LOCALS(&r200->radeon);

    assert(width <= 2048);
    assert(height <= 2048);
    assert(offset % 32 == 0);

    /* XXX others?  BE/LE? */
    switch (src_mesa_format) {
    case MESA_FORMAT_ARGB8888:
	    txformat |= R200_TXFORMAT_ARGB8888 | R200_TXFORMAT_ALPHA_IN_MAP;
	    break;
    case MESA_FORMAT_RGBA8888:
	    txformat |= R200_TXFORMAT_RGBA8888 | R200_TXFORMAT_ALPHA_IN_MAP;
	    break;
    case MESA_FORMAT_RGBA8888_REV:
	    txformat |= R200_TXFORMAT_ABGR8888 | R200_TXFORMAT_ALPHA_IN_MAP;
	    break;
    case MESA_FORMAT_XRGB8888:
	    txformat |= R200_TXFORMAT_ARGB8888;
	    break;
    case MESA_FORMAT_RGB565:
	    txformat |= R200_TXFORMAT_RGB565;
	    break;
    case MESA_FORMAT_ARGB4444:
	    txformat |= R200_TXFORMAT_ARGB4444 | R200_TXFORMAT_ALPHA_IN_MAP;
	    break;
    case MESA_FORMAT_ARGB1555:
	    txformat |= R200_TXFORMAT_ARGB1555 | R200_TXFORMAT_ALPHA_IN_MAP;
	    break;
    case MESA_FORMAT_A8:
    case MESA_FORMAT_I8:
	    txformat |= R200_TXFORMAT_I8 | R200_TXFORMAT_ALPHA_IN_MAP;
	    break;
    case MESA_FORMAT_L8:
	    txformat |= R200_TXFORMAT_I8;
	    break;
    case MESA_FORMAT_AL88:
	    txformat |= R200_TXFORMAT_AI88 | R200_TXFORMAT_ALPHA_IN_MAP;
	    break;
    default:
	    break;
    }

    if (bo->flags & RADEON_BO_FLAGS_MACRO_TILE)
	offset |= R200_TXO_MACRO_TILE;
    if (bo->flags & RADEON_BO_FLAGS_MICRO_TILE)
	offset |= R200_TXO_MICRO_TILE;

    switch (dst_mesa_format) {
    case MESA_FORMAT_ARGB8888:
    case MESA_FORMAT_XRGB8888:
    case MESA_FORMAT_RGB565:
    case MESA_FORMAT_ARGB4444:
    case MESA_FORMAT_ARGB1555:
    case MESA_FORMAT_A8:
    case MESA_FORMAT_L8:
    case MESA_FORMAT_I8:
    default:
	    /* no swizzle required */
	    BEGIN_BATCH(10);
	    OUT_BATCH_REGVAL(RADEON_PP_CNTL, (RADEON_TEX_0_ENABLE |
					      RADEON_TEX_BLEND_0_ENABLE));
	    OUT_BATCH_REGVAL(R200_PP_TXCBLEND_0, (R200_TXC_ARG_A_ZERO |
						  R200_TXC_ARG_B_ZERO |
						  R200_TXC_ARG_C_R0_COLOR |
						  R200_TXC_OP_MADD));
	    OUT_BATCH_REGVAL(R200_PP_TXCBLEND2_0, (R200_TXC_CLAMP_0_1 |
						   R200_TXC_OUTPUT_REG_R0));
	    OUT_BATCH_REGVAL(R200_PP_TXABLEND_0, (R200_TXA_ARG_A_ZERO |
						  R200_TXA_ARG_B_ZERO |
						  R200_TXA_ARG_C_R0_ALPHA |
						  R200_TXA_OP_MADD));
	    OUT_BATCH_REGVAL(R200_PP_TXABLEND2_0, (R200_TXA_CLAMP_0_1 |
						   R200_TXA_OUTPUT_REG_R0));
	    END_BATCH();
	    break;
    case MESA_FORMAT_RGBA8888:
	    BEGIN_BATCH(10);
	    OUT_BATCH_REGVAL(RADEON_PP_CNTL, (RADEON_TEX_0_ENABLE |
					      RADEON_TEX_BLEND_0_ENABLE));
	    OUT_BATCH_REGVAL(R200_PP_TXCBLEND_0, (R200_TXC_ARG_A_ZERO |
						  R200_TXC_ARG_B_ZERO |
						  R200_TXC_ARG_C_R0_COLOR |
						  R200_TXC_OP_MADD));
	    OUT_BATCH_REGVAL(R200_PP_TXCBLEND2_0, (R200_TXC_CLAMP_0_1 |
						   R200_TXC_OUTPUT_ROTATE_GBA |
						   R200_TXC_OUTPUT_REG_R0));
	    OUT_BATCH_REGVAL(R200_PP_TXABLEND_0, (R200_TXA_ARG_A_ZERO |
						  R200_TXA_ARG_B_ZERO |
						  R200_TXA_ARG_C_R0_ALPHA |
						  R200_TXA_OP_MADD));
	    OUT_BATCH_REGVAL(R200_PP_TXABLEND2_0, (R200_TXA_CLAMP_0_1 |
						   (R200_TXA_REPL_RED << R200_TXA_REPL_ARG_C_SHIFT) |
						   R200_TXA_OUTPUT_REG_R0));
	    END_BATCH();
	    break;
    case MESA_FORMAT_RGBA8888_REV:
	    BEGIN_BATCH(34);
	    OUT_BATCH_REGVAL(RADEON_PP_CNTL, (RADEON_TEX_0_ENABLE |
					      RADEON_TEX_BLEND_0_ENABLE |
					      RADEON_TEX_BLEND_1_ENABLE |
					      RADEON_TEX_BLEND_2_ENABLE |
					      RADEON_TEX_BLEND_3_ENABLE));
	    /* r1.r = r0.b */
	    OUT_BATCH_REGVAL(R200_PP_TXCBLEND_0, (R200_TXC_ARG_A_ZERO |
						  R200_TXC_ARG_B_ZERO |
						  R200_TXC_ARG_C_R0_COLOR |
						  R200_TXC_OP_MADD));
	    OUT_BATCH_REGVAL(R200_PP_TXCBLEND2_0, (R200_TXC_CLAMP_0_1 |
						   R200_TXC_OUTPUT_MASK_R |
						   (R200_TXC_REPL_BLUE << R200_TXC_REPL_ARG_C_SHIFT) |
						   R200_TXC_OUTPUT_REG_R1));
	    /* r1.a = r0.a */
	    OUT_BATCH_REGVAL(R200_PP_TXABLEND_0, (R200_TXA_ARG_A_ZERO |
						  R200_TXA_ARG_B_ZERO |
						  R200_TXA_ARG_C_R0_ALPHA |
						  R200_TXA_OP_MADD));
	    OUT_BATCH_REGVAL(R200_PP_TXABLEND2_0, (R200_TXA_CLAMP_0_1 |
						   R200_TXA_OUTPUT_REG_R1));
	    /* r1.g = r0.g */
	    OUT_BATCH_REGVAL(R200_PP_TXCBLEND_1, (R200_TXC_ARG_A_ZERO |
						  R200_TXC_ARG_B_ZERO |
						  R200_TXC_ARG_C_R0_COLOR |
						  R200_TXC_OP_MADD));
	    OUT_BATCH_REGVAL(R200_PP_TXCBLEND2_1, (R200_TXC_CLAMP_0_1 |
						   R200_TXC_OUTPUT_MASK_G |
						   (R200_TXC_REPL_GREEN << R200_TXC_REPL_ARG_C_SHIFT) |
						   R200_TXC_OUTPUT_REG_R1));
	    /* r1.a = r0.a */
	    OUT_BATCH_REGVAL(R200_PP_TXABLEND_1, (R200_TXA_ARG_A_ZERO |
						  R200_TXA_ARG_B_ZERO |
						  R200_TXA_ARG_C_R0_ALPHA |
						  R200_TXA_OP_MADD));
	    OUT_BATCH_REGVAL(R200_PP_TXABLEND2_1, (R200_TXA_CLAMP_0_1 |
						   R200_TXA_OUTPUT_REG_R1));
	    /* r1.b = r0.r */
	    OUT_BATCH_REGVAL(R200_PP_TXCBLEND_2, (R200_TXC_ARG_A_ZERO |
						  R200_TXC_ARG_B_ZERO |
						  R200_TXC_ARG_C_R0_COLOR |
						  R200_TXC_OP_MADD));
	    OUT_BATCH_REGVAL(R200_PP_TXCBLEND2_2, (R200_TXC_CLAMP_0_1 |
						   R200_TXC_OUTPUT_MASK_B |
						   (R200_TXC_REPL_RED << R200_TXC_REPL_ARG_C_SHIFT) |
						   R200_TXC_OUTPUT_REG_R1));
	    /* r1.a = r0.a */
	    OUT_BATCH_REGVAL(R200_PP_TXABLEND_2, (R200_TXA_ARG_A_ZERO |
						  R200_TXA_ARG_B_ZERO |
						  R200_TXA_ARG_C_R0_ALPHA |
						  R200_TXA_OP_MADD));
	    OUT_BATCH_REGVAL(R200_PP_TXABLEND2_2, (R200_TXA_CLAMP_0_1 |
						   R200_TXA_OUTPUT_REG_R1));
	    /* r0.rgb = r1.rgb */
	    OUT_BATCH_REGVAL(R200_PP_TXCBLEND_3, (R200_TXC_ARG_A_ZERO |
						  R200_TXC_ARG_B_ZERO |
						  R200_TXC_ARG_C_R1_COLOR |
						  R200_TXC_OP_MADD));
	    OUT_BATCH_REGVAL(R200_PP_TXCBLEND2_3, (R200_TXC_CLAMP_0_1 |
						   R200_TXC_OUTPUT_REG_R0));
	    /* r0.a = r1.a */
	    OUT_BATCH_REGVAL(R200_PP_TXABLEND_3, (R200_TXA_ARG_A_ZERO |
						  R200_TXA_ARG_B_ZERO |
						  R200_TXA_ARG_C_R1_ALPHA |
						  R200_TXA_OP_MADD));
	    OUT_BATCH_REGVAL(R200_PP_TXABLEND2_3, (R200_TXA_CLAMP_0_1 |
						   R200_TXA_OUTPUT_REG_R0));
	    END_BATCH();
	    break;
    }

    BEGIN_BATCH(18);
    OUT_BATCH_REGVAL(R200_PP_CNTL_X, 0);
    OUT_BATCH_REGVAL(R200_PP_TXMULTI_CTL_0, 0);
    OUT_BATCH_REGVAL(R200_PP_TXFILTER_0, (R200_CLAMP_S_CLAMP_LAST |
					  R200_CLAMP_T_CLAMP_LAST |
					  R200_MAG_FILTER_NEAREST |
					  R200_MIN_FILTER_NEAREST));
    OUT_BATCH_REGVAL(R200_PP_TXFORMAT_0, txformat);
    OUT_BATCH_REGVAL(R200_PP_TXFORMAT_X_0, 0);
    OUT_BATCH_REGVAL(R200_PP_TXSIZE_0, ((width - 1) |
					((height - 1) << RADEON_TEX_VSIZE_SHIFT)));
    OUT_BATCH_REGVAL(R200_PP_TXPITCH_0, pitch * _mesa_get_format_bytes(src_mesa_format) - 32);

    OUT_BATCH_REGSEQ(R200_PP_TXOFFSET_0, 1);
    OUT_BATCH_RELOC(offset, bo, offset, RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);

    END_BATCH();
}

static inline void emit_cb_setup(struct r200_context *r200,
				 struct radeon_bo *bo,
				 intptr_t offset,
				 gl_format mesa_format,
				 unsigned pitch,
				 unsigned width,
				 unsigned height)
{
    uint32_t dst_pitch = pitch;
    uint32_t dst_format = 0;
    BATCH_LOCALS(&r200->radeon);

    /* XXX others?  BE/LE? */
    switch (mesa_format) {
    case MESA_FORMAT_ARGB8888:
    case MESA_FORMAT_XRGB8888:
    case MESA_FORMAT_RGBA8888:
    case MESA_FORMAT_RGBA8888_REV:
	    dst_format = RADEON_COLOR_FORMAT_ARGB8888;
	    break;
    case MESA_FORMAT_RGB565:
	    dst_format = RADEON_COLOR_FORMAT_RGB565;
	    break;
    case MESA_FORMAT_ARGB4444:
	    dst_format = RADEON_COLOR_FORMAT_ARGB4444;
	    break;
    case MESA_FORMAT_ARGB1555:
	    dst_format = RADEON_COLOR_FORMAT_ARGB1555;
	    break;
    case MESA_FORMAT_A8:
    case MESA_FORMAT_L8:
    case MESA_FORMAT_I8:
	    dst_format = RADEON_COLOR_FORMAT_RGB8;
	    break;
    default:
	    break;
    }

    if (bo->flags & RADEON_BO_FLAGS_MACRO_TILE)
	dst_pitch |= R200_COLOR_TILE_ENABLE;
    if (bo->flags & RADEON_BO_FLAGS_MICRO_TILE)
	dst_pitch |= R200_COLOR_MICROTILE_ENABLE;

    BEGIN_BATCH_NO_AUTOSTATE(22);
    OUT_BATCH_REGVAL(R200_RE_AUX_SCISSOR_CNTL, 0);
    OUT_BATCH_REGVAL(R200_RE_CNTL, 0);
    OUT_BATCH_REGVAL(RADEON_RE_TOP_LEFT, 0);
    OUT_BATCH_REGVAL(RADEON_RE_WIDTH_HEIGHT, (((width - 1) << RADEON_RE_WIDTH_SHIFT) |
					      ((height - 1) << RADEON_RE_HEIGHT_SHIFT)));
    OUT_BATCH_REGVAL(RADEON_RB3D_PLANEMASK, 0xffffffff);
    OUT_BATCH_REGVAL(RADEON_RB3D_BLENDCNTL, RADEON_SRC_BLEND_GL_ONE | RADEON_DST_BLEND_GL_ZERO);
    OUT_BATCH_REGVAL(RADEON_RB3D_CNTL, dst_format);

    OUT_BATCH_REGSEQ(RADEON_RB3D_COLOROFFSET, 1);
    OUT_BATCH_RELOC(offset, bo, offset, 0, RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0);
    OUT_BATCH_REGSEQ(RADEON_RB3D_COLORPITCH, 1);
    OUT_BATCH_RELOC(dst_pitch, bo, dst_pitch, 0, RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0);

    END_BATCH();
}

static GLboolean validate_buffers(struct r200_context *r200,
                                  struct radeon_bo *src_bo,
                                  struct radeon_bo *dst_bo)
{
    int ret;

    radeon_cs_space_reset_bos(r200->radeon.cmdbuf.cs);

    ret = radeon_cs_space_check_with_bo(r200->radeon.cmdbuf.cs,
                                        src_bo, RADEON_GEM_DOMAIN_VRAM | RADEON_GEM_DOMAIN_GTT, 0);
    if (ret)
        return GL_FALSE;

    ret = radeon_cs_space_check_with_bo(r200->radeon.cmdbuf.cs,
                                        dst_bo, 0, RADEON_GEM_DOMAIN_VRAM | RADEON_GEM_DOMAIN_GTT);
    if (ret)
        return GL_FALSE;

    return GL_TRUE;
}

/**
 * Calculate texcoords for given image region.
 * Output values are [minx, maxx, miny, maxy]
 */
static inline void calc_tex_coords(float img_width, float img_height,
				   float x, float y,
				   float reg_width, float reg_height,
				   unsigned flip_y, float *buf)
{
    buf[0] = x / img_width;
    buf[1] = buf[0] + reg_width / img_width;
    buf[2] = y / img_height;
    buf[3] = buf[2] + reg_height / img_height;
    if (flip_y)
    {
        buf[2] = 1.0 - buf[2];
        buf[3] = 1.0 - buf[3];
    }
}

static inline void emit_draw_packet(struct r200_context *r200,
				    unsigned src_width, unsigned src_height,
				    unsigned src_x_offset, unsigned src_y_offset,
				    unsigned dst_x_offset, unsigned dst_y_offset,
				    unsigned reg_width, unsigned reg_height,
				    unsigned flip_y)
{
    float texcoords[4];
    float verts[12];
    BATCH_LOCALS(&r200->radeon);

    calc_tex_coords(src_width, src_height,
                    src_x_offset, src_y_offset,
                    reg_width, reg_height,
                    flip_y, texcoords);

    verts[0] = dst_x_offset;
    verts[1] = dst_y_offset + reg_height;
    verts[2] = texcoords[0];
    verts[3] = texcoords[3];

    verts[4] = dst_x_offset + reg_width;
    verts[5] = dst_y_offset + reg_height;
    verts[6] = texcoords[1];
    verts[7] = texcoords[3];

    verts[8] = dst_x_offset + reg_width;
    verts[9] = dst_y_offset;
    verts[10] = texcoords[1];
    verts[11] = texcoords[2];

    BEGIN_BATCH(14);
    OUT_BATCH(R200_CP_CMD_3D_DRAW_IMMD_2 | (12 << 16));
    OUT_BATCH(RADEON_CP_VC_CNTL_PRIM_WALK_RING |
	      RADEON_CP_VC_CNTL_PRIM_TYPE_RECT_LIST |
              (3 << 16));
    OUT_BATCH_TABLE(verts, 12);
    END_BATCH();
}

/**
 * Copy a region of [@a width x @a height] pixels from source buffer
 * to destination buffer.
 * @param[in] r200 r200 context
 * @param[in] src_bo source radeon buffer object
 * @param[in] src_offset offset of the source image in the @a src_bo
 * @param[in] src_mesaformat source image format
 * @param[in] src_pitch aligned source image width
 * @param[in] src_width source image width
 * @param[in] src_height source image height
 * @param[in] src_x_offset x offset in the source image
 * @param[in] src_y_offset y offset in the source image
 * @param[in] dst_bo destination radeon buffer object
 * @param[in] dst_offset offset of the destination image in the @a dst_bo
 * @param[in] dst_mesaformat destination image format
 * @param[in] dst_pitch aligned destination image width
 * @param[in] dst_width destination image width
 * @param[in] dst_height destination image height
 * @param[in] dst_x_offset x offset in the destination image
 * @param[in] dst_y_offset y offset in the destination image
 * @param[in] width region width
 * @param[in] height region height
 * @param[in] flip_y set if y coords of the source image need to be flipped
 */
unsigned r200_blit(struct gl_context *ctx,
                   struct radeon_bo *src_bo,
                   intptr_t src_offset,
                   gl_format src_mesaformat,
                   unsigned src_pitch,
                   unsigned src_width,
                   unsigned src_height,
                   unsigned src_x_offset,
                   unsigned src_y_offset,
                   struct radeon_bo *dst_bo,
                   intptr_t dst_offset,
                   gl_format dst_mesaformat,
                   unsigned dst_pitch,
                   unsigned dst_width,
                   unsigned dst_height,
                   unsigned dst_x_offset,
                   unsigned dst_y_offset,
                   unsigned reg_width,
                   unsigned reg_height,
                   unsigned flip_y)
{
    struct r200_context *r200 = R200_CONTEXT(ctx);

    if (!r200_check_blit(dst_mesaformat, dst_pitch))
        return GL_FALSE;

    /* Make sure that colorbuffer has even width - hw limitation */
    if (dst_pitch % 2 > 0)
        ++dst_pitch;

    /* Need to clamp the region size to make sure
     * we don't read outside of the source buffer
     * or write outside of the destination buffer.
     */
    if (reg_width + src_x_offset > src_width)
        reg_width = src_width - src_x_offset;
    if (reg_height + src_y_offset > src_height)
        reg_height = src_height - src_y_offset;
    if (reg_width + dst_x_offset > dst_width)
        reg_width = dst_width - dst_x_offset;
    if (reg_height + dst_y_offset > dst_height)
        reg_height = dst_height - dst_y_offset;

    if (src_bo == dst_bo) {
        return GL_FALSE;
    }

    if (src_offset % 32 || dst_offset % 32) {
        return GL_FALSE;
    }

    if (0) {
        fprintf(stderr, "src: size [%d x %d], pitch %d, "
                "offset [%d x %d], format %s, bo %p\n",
                src_width, src_height, src_pitch,
                src_x_offset, src_y_offset,
                _mesa_get_format_name(src_mesaformat),
                src_bo);
        fprintf(stderr, "dst: pitch %d, offset[%d x %d], format %s, bo %p\n",
                dst_pitch, dst_x_offset, dst_y_offset,
                _mesa_get_format_name(dst_mesaformat), dst_bo);
        fprintf(stderr, "region: %d x %d\n", reg_width, reg_height);
    }

    /* Flush is needed to make sure that source buffer has correct data */
    radeonFlush(r200->radeon.glCtx);

    rcommonEnsureCmdBufSpace(&r200->radeon, 102, __FUNCTION__);

    if (!validate_buffers(r200, src_bo, dst_bo))
        return GL_FALSE;

    /* 14 */
    emit_vtx_state(r200);
    /* 52 */
    emit_tx_setup(r200, src_mesaformat, dst_mesaformat, src_bo, src_offset, src_width, src_height, src_pitch);
    /* 22 */
    emit_cb_setup(r200, dst_bo, dst_offset, dst_mesaformat, dst_pitch, dst_width, dst_height);
    /* 14 */
    emit_draw_packet(r200, src_width, src_height,
                     src_x_offset, src_y_offset,
                     dst_x_offset, dst_y_offset,
                     reg_width, reg_height,
                     flip_y);

    radeonFlush(ctx);

    return GL_TRUE;
}
