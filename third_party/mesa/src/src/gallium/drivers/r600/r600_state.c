/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
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
 */
#include "r600_formats.h"
#include "r600d.h"

#include "pipe/p_shader_tokens.h"
#include "util/u_pack_color.h"
#include "util/u_memory.h"
#include "util/u_framebuffer.h"
#include "util/u_dual_blend.h"

static uint32_t r600_translate_blend_function(int blend_func)
{
	switch (blend_func) {
	case PIPE_BLEND_ADD:
		return V_028804_COMB_DST_PLUS_SRC;
	case PIPE_BLEND_SUBTRACT:
		return V_028804_COMB_SRC_MINUS_DST;
	case PIPE_BLEND_REVERSE_SUBTRACT:
		return V_028804_COMB_DST_MINUS_SRC;
	case PIPE_BLEND_MIN:
		return V_028804_COMB_MIN_DST_SRC;
	case PIPE_BLEND_MAX:
		return V_028804_COMB_MAX_DST_SRC;
	default:
		R600_ERR("Unknown blend function %d\n", blend_func);
		assert(0);
		break;
	}
	return 0;
}

static uint32_t r600_translate_blend_factor(int blend_fact)
{
	switch (blend_fact) {
	case PIPE_BLENDFACTOR_ONE:
		return V_028804_BLEND_ONE;
	case PIPE_BLENDFACTOR_SRC_COLOR:
		return V_028804_BLEND_SRC_COLOR;
	case PIPE_BLENDFACTOR_SRC_ALPHA:
		return V_028804_BLEND_SRC_ALPHA;
	case PIPE_BLENDFACTOR_DST_ALPHA:
		return V_028804_BLEND_DST_ALPHA;
	case PIPE_BLENDFACTOR_DST_COLOR:
		return V_028804_BLEND_DST_COLOR;
	case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
		return V_028804_BLEND_SRC_ALPHA_SATURATE;
	case PIPE_BLENDFACTOR_CONST_COLOR:
		return V_028804_BLEND_CONST_COLOR;
	case PIPE_BLENDFACTOR_CONST_ALPHA:
		return V_028804_BLEND_CONST_ALPHA;
	case PIPE_BLENDFACTOR_ZERO:
		return V_028804_BLEND_ZERO;
	case PIPE_BLENDFACTOR_INV_SRC_COLOR:
		return V_028804_BLEND_ONE_MINUS_SRC_COLOR;
	case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
		return V_028804_BLEND_ONE_MINUS_SRC_ALPHA;
	case PIPE_BLENDFACTOR_INV_DST_ALPHA:
		return V_028804_BLEND_ONE_MINUS_DST_ALPHA;
	case PIPE_BLENDFACTOR_INV_DST_COLOR:
		return V_028804_BLEND_ONE_MINUS_DST_COLOR;
	case PIPE_BLENDFACTOR_INV_CONST_COLOR:
		return V_028804_BLEND_ONE_MINUS_CONST_COLOR;
	case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
		return V_028804_BLEND_ONE_MINUS_CONST_ALPHA;
	case PIPE_BLENDFACTOR_SRC1_COLOR:
		return V_028804_BLEND_SRC1_COLOR;
	case PIPE_BLENDFACTOR_SRC1_ALPHA:
		return V_028804_BLEND_SRC1_ALPHA;
	case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
		return V_028804_BLEND_INV_SRC1_COLOR;
	case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
		return V_028804_BLEND_INV_SRC1_ALPHA;
	default:
		R600_ERR("Bad blend factor %d not supported!\n", blend_fact);
		assert(0);
		break;
	}
	return 0;
}

static unsigned r600_tex_dim(unsigned dim, unsigned nr_samples)
{
	switch (dim) {
	default:
	case PIPE_TEXTURE_1D:
		return V_038000_SQ_TEX_DIM_1D;
	case PIPE_TEXTURE_1D_ARRAY:
		return V_038000_SQ_TEX_DIM_1D_ARRAY;
	case PIPE_TEXTURE_2D:
	case PIPE_TEXTURE_RECT:
		return nr_samples > 1 ? V_038000_SQ_TEX_DIM_2D_MSAA :
					V_038000_SQ_TEX_DIM_2D;
	case PIPE_TEXTURE_2D_ARRAY:
		return nr_samples > 1 ? V_038000_SQ_TEX_DIM_2D_ARRAY_MSAA :
					V_038000_SQ_TEX_DIM_2D_ARRAY;
	case PIPE_TEXTURE_3D:
		return V_038000_SQ_TEX_DIM_3D;
	case PIPE_TEXTURE_CUBE:
		return V_038000_SQ_TEX_DIM_CUBEMAP;
	}
}

static uint32_t r600_translate_dbformat(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_Z16_UNORM:
		return V_028010_DEPTH_16;
	case PIPE_FORMAT_Z24X8_UNORM:
		return V_028010_DEPTH_X8_24;
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		return V_028010_DEPTH_8_24;
	case PIPE_FORMAT_Z32_FLOAT:
		return V_028010_DEPTH_32_FLOAT;
	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
		return V_028010_DEPTH_X24_8_32_FLOAT;
	default:
		return ~0U;
	}
}

static uint32_t r600_translate_colorswap(enum pipe_format format)
{
	switch (format) {
	/* 8-bit buffers. */
	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_A8_SNORM:
	case PIPE_FORMAT_A8_UINT:
	case PIPE_FORMAT_A8_SINT:
	case PIPE_FORMAT_A16_UNORM:
	case PIPE_FORMAT_A16_SNORM:
	case PIPE_FORMAT_A16_UINT:
	case PIPE_FORMAT_A16_SINT:
	case PIPE_FORMAT_A16_FLOAT:
	case PIPE_FORMAT_A32_UINT:
	case PIPE_FORMAT_A32_SINT:
	case PIPE_FORMAT_A32_FLOAT:
	case PIPE_FORMAT_R4A4_UNORM:
		return V_0280A0_SWAP_ALT_REV;
	case PIPE_FORMAT_I8_UNORM:
	case PIPE_FORMAT_I8_SNORM:
	case PIPE_FORMAT_I8_UINT:
	case PIPE_FORMAT_I8_SINT:
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_L8_SNORM:
	case PIPE_FORMAT_L8_UINT:
	case PIPE_FORMAT_L8_SINT:
	case PIPE_FORMAT_L8_SRGB:
	case PIPE_FORMAT_L16_UNORM:
	case PIPE_FORMAT_L16_SNORM:
	case PIPE_FORMAT_L16_UINT:
	case PIPE_FORMAT_L16_SINT:
	case PIPE_FORMAT_L16_FLOAT:
	case PIPE_FORMAT_L32_UINT:
	case PIPE_FORMAT_L32_SINT:
	case PIPE_FORMAT_L32_FLOAT:
	case PIPE_FORMAT_I16_UNORM:
	case PIPE_FORMAT_I16_SNORM:
	case PIPE_FORMAT_I16_UINT:
	case PIPE_FORMAT_I16_SINT:
	case PIPE_FORMAT_I16_FLOAT:
	case PIPE_FORMAT_I32_UINT:
	case PIPE_FORMAT_I32_SINT:
	case PIPE_FORMAT_I32_FLOAT:
	case PIPE_FORMAT_R8_UNORM:
	case PIPE_FORMAT_R8_SNORM:
	case PIPE_FORMAT_R8_UINT:
	case PIPE_FORMAT_R8_SINT:
		return V_0280A0_SWAP_STD;

	case PIPE_FORMAT_L4A4_UNORM:
	case PIPE_FORMAT_A4R4_UNORM:
		return V_0280A0_SWAP_ALT;

	/* 16-bit buffers. */
	case PIPE_FORMAT_B5G6R5_UNORM:
		return V_0280A0_SWAP_STD_REV;

	case PIPE_FORMAT_B5G5R5A1_UNORM:
	case PIPE_FORMAT_B5G5R5X1_UNORM:
		return V_0280A0_SWAP_ALT;

	case PIPE_FORMAT_B4G4R4A4_UNORM:
	case PIPE_FORMAT_B4G4R4X4_UNORM:
		return V_0280A0_SWAP_ALT;

	case PIPE_FORMAT_Z16_UNORM:
		return V_0280A0_SWAP_STD;

	case PIPE_FORMAT_L8A8_UNORM:
	case PIPE_FORMAT_L8A8_SNORM:
	case PIPE_FORMAT_L8A8_UINT:
	case PIPE_FORMAT_L8A8_SINT:
	case PIPE_FORMAT_L8A8_SRGB:
	case PIPE_FORMAT_L16A16_UNORM:
	case PIPE_FORMAT_L16A16_SNORM:
	case PIPE_FORMAT_L16A16_UINT:
	case PIPE_FORMAT_L16A16_SINT:
	case PIPE_FORMAT_L16A16_FLOAT:
	case PIPE_FORMAT_L32A32_UINT:
	case PIPE_FORMAT_L32A32_SINT:
	case PIPE_FORMAT_L32A32_FLOAT:
		return V_0280A0_SWAP_ALT;
	case PIPE_FORMAT_R8G8_UNORM:
	case PIPE_FORMAT_R8G8_SNORM:
	case PIPE_FORMAT_R8G8_UINT:
	case PIPE_FORMAT_R8G8_SINT:
		return V_0280A0_SWAP_STD;

	case PIPE_FORMAT_R16_UNORM:
	case PIPE_FORMAT_R16_SNORM:
	case PIPE_FORMAT_R16_UINT:
	case PIPE_FORMAT_R16_SINT:
	case PIPE_FORMAT_R16_FLOAT:
		return V_0280A0_SWAP_STD;

	/* 32-bit buffers. */

	case PIPE_FORMAT_A8B8G8R8_SRGB:
		return V_0280A0_SWAP_STD_REV;
	case PIPE_FORMAT_B8G8R8A8_SRGB:
		return V_0280A0_SWAP_ALT;

	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case PIPE_FORMAT_B8G8R8X8_UNORM:
		return V_0280A0_SWAP_ALT;

	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case PIPE_FORMAT_X8R8G8B8_UNORM:
		return V_0280A0_SWAP_ALT_REV;
	case PIPE_FORMAT_R8G8B8A8_SNORM:
	case PIPE_FORMAT_R8G8B8A8_UNORM:
	case PIPE_FORMAT_R8G8B8X8_UNORM:
	case PIPE_FORMAT_R8G8B8A8_SINT:
	case PIPE_FORMAT_R8G8B8A8_UINT:
		return V_0280A0_SWAP_STD;

	case PIPE_FORMAT_A8B8G8R8_UNORM:
	case PIPE_FORMAT_X8B8G8R8_UNORM:
	/* case PIPE_FORMAT_R8SG8SB8UX8U_NORM: */
		return V_0280A0_SWAP_STD_REV;

	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		return V_0280A0_SWAP_STD;

	case PIPE_FORMAT_X8Z24_UNORM:
	case PIPE_FORMAT_S8_UINT_Z24_UNORM:
		return V_0280A0_SWAP_STD;

	case PIPE_FORMAT_R10G10B10A2_UNORM:
	case PIPE_FORMAT_R10G10B10X2_SNORM:
	case PIPE_FORMAT_R10SG10SB10SA2U_NORM:
		return V_0280A0_SWAP_STD;

	case PIPE_FORMAT_B10G10R10A2_UNORM:
	case PIPE_FORMAT_B10G10R10A2_UINT:
		return V_0280A0_SWAP_ALT;

	case PIPE_FORMAT_R11G11B10_FLOAT:
	case PIPE_FORMAT_R16G16_UNORM:
	case PIPE_FORMAT_R16G16_SNORM:
	case PIPE_FORMAT_R16G16_FLOAT:
	case PIPE_FORMAT_R16G16_UINT:
	case PIPE_FORMAT_R16G16_SINT:
	case PIPE_FORMAT_R32_UINT:
	case PIPE_FORMAT_R32_SINT:
	case PIPE_FORMAT_R32_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT:
		return V_0280A0_SWAP_STD;

	/* 64-bit buffers. */
	case PIPE_FORMAT_R32G32_FLOAT:
	case PIPE_FORMAT_R32G32_UINT:
	case PIPE_FORMAT_R32G32_SINT:
	case PIPE_FORMAT_R16G16B16A16_UNORM:
	case PIPE_FORMAT_R16G16B16A16_SNORM:
	case PIPE_FORMAT_R16G16B16A16_UINT:
	case PIPE_FORMAT_R16G16B16A16_SINT:
	case PIPE_FORMAT_R16G16B16A16_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:

	/* 128-bit buffers. */
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
	case PIPE_FORMAT_R32G32B32A32_SNORM:
	case PIPE_FORMAT_R32G32B32A32_UNORM:
	case PIPE_FORMAT_R32G32B32A32_SINT:
	case PIPE_FORMAT_R32G32B32A32_UINT:
		return V_0280A0_SWAP_STD;
	default:
		R600_ERR("unsupported colorswap format %d\n", format);
		return ~0U;
	}
	return ~0U;
}

static uint32_t r600_translate_colorformat(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_L4A4_UNORM:
	case PIPE_FORMAT_R4A4_UNORM:
	case PIPE_FORMAT_A4R4_UNORM:
		return V_0280A0_COLOR_4_4;

	/* 8-bit buffers. */
	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_A8_SNORM:
	case PIPE_FORMAT_A8_UINT:
	case PIPE_FORMAT_A8_SINT:
	case PIPE_FORMAT_I8_UNORM:
	case PIPE_FORMAT_I8_SNORM:
	case PIPE_FORMAT_I8_UINT:
	case PIPE_FORMAT_I8_SINT:
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_L8_SNORM:
	case PIPE_FORMAT_L8_UINT:
	case PIPE_FORMAT_L8_SINT:
	case PIPE_FORMAT_L8_SRGB:
	case PIPE_FORMAT_R8_UNORM:
	case PIPE_FORMAT_R8_SNORM:
	case PIPE_FORMAT_R8_UINT:
	case PIPE_FORMAT_R8_SINT:
		return V_0280A0_COLOR_8;

	/* 16-bit buffers. */
	case PIPE_FORMAT_B5G6R5_UNORM:
		return V_0280A0_COLOR_5_6_5;

	case PIPE_FORMAT_B5G5R5A1_UNORM:
	case PIPE_FORMAT_B5G5R5X1_UNORM:
		return V_0280A0_COLOR_1_5_5_5;

	case PIPE_FORMAT_B4G4R4A4_UNORM:
	case PIPE_FORMAT_B4G4R4X4_UNORM:
		return V_0280A0_COLOR_4_4_4_4;

	case PIPE_FORMAT_Z16_UNORM:
		return V_0280A0_COLOR_16;

	case PIPE_FORMAT_L8A8_UNORM:
	case PIPE_FORMAT_L8A8_SNORM:
	case PIPE_FORMAT_L8A8_UINT:
	case PIPE_FORMAT_L8A8_SINT:
	case PIPE_FORMAT_L8A8_SRGB:
	case PIPE_FORMAT_R8G8_UNORM:
	case PIPE_FORMAT_R8G8_SNORM:
	case PIPE_FORMAT_R8G8_UINT:
	case PIPE_FORMAT_R8G8_SINT:
		return V_0280A0_COLOR_8_8;

	case PIPE_FORMAT_R16_UNORM:
	case PIPE_FORMAT_R16_SNORM:
	case PIPE_FORMAT_R16_UINT:
	case PIPE_FORMAT_R16_SINT:
	case PIPE_FORMAT_A16_UNORM:
	case PIPE_FORMAT_A16_SNORM:
	case PIPE_FORMAT_A16_UINT:
	case PIPE_FORMAT_A16_SINT:
	case PIPE_FORMAT_L16_UNORM:
	case PIPE_FORMAT_L16_SNORM:
	case PIPE_FORMAT_L16_UINT:
	case PIPE_FORMAT_L16_SINT:
	case PIPE_FORMAT_I16_UNORM:
	case PIPE_FORMAT_I16_SNORM:
	case PIPE_FORMAT_I16_UINT:
	case PIPE_FORMAT_I16_SINT:
		return V_0280A0_COLOR_16;

	case PIPE_FORMAT_R16_FLOAT:
	case PIPE_FORMAT_A16_FLOAT:
	case PIPE_FORMAT_L16_FLOAT:
	case PIPE_FORMAT_I16_FLOAT:
		return V_0280A0_COLOR_16_FLOAT;

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
	case PIPE_FORMAT_R8G8B8A8_SINT:
	case PIPE_FORMAT_R8G8B8A8_UINT:
		return V_0280A0_COLOR_8_8_8_8;

	case PIPE_FORMAT_R10G10B10A2_UNORM:
	case PIPE_FORMAT_R10G10B10X2_SNORM:
	case PIPE_FORMAT_B10G10R10A2_UNORM:
	case PIPE_FORMAT_B10G10R10A2_UINT:
	case PIPE_FORMAT_R10SG10SB10SA2U_NORM:
		return V_0280A0_COLOR_2_10_10_10;

	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		return V_0280A0_COLOR_8_24;

	case PIPE_FORMAT_X8Z24_UNORM:
	case PIPE_FORMAT_S8_UINT_Z24_UNORM:
		return V_0280A0_COLOR_24_8;

	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
		return V_0280A0_COLOR_X24_8_32_FLOAT;

	case PIPE_FORMAT_R32_UINT:
	case PIPE_FORMAT_R32_SINT:
	case PIPE_FORMAT_A32_UINT:
	case PIPE_FORMAT_A32_SINT:
	case PIPE_FORMAT_L32_UINT:
	case PIPE_FORMAT_L32_SINT:
	case PIPE_FORMAT_I32_UINT:
	case PIPE_FORMAT_I32_SINT:
		return V_0280A0_COLOR_32;

	case PIPE_FORMAT_R32_FLOAT:
	case PIPE_FORMAT_A32_FLOAT:
	case PIPE_FORMAT_L32_FLOAT:
	case PIPE_FORMAT_I32_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT:
		return V_0280A0_COLOR_32_FLOAT;

	case PIPE_FORMAT_R16G16_FLOAT:
	case PIPE_FORMAT_L16A16_FLOAT:
		return V_0280A0_COLOR_16_16_FLOAT;

	case PIPE_FORMAT_R16G16_UNORM:
	case PIPE_FORMAT_R16G16_SNORM:
	case PIPE_FORMAT_R16G16_UINT:
	case PIPE_FORMAT_R16G16_SINT:
	case PIPE_FORMAT_L16A16_UNORM:
	case PIPE_FORMAT_L16A16_SNORM:
	case PIPE_FORMAT_L16A16_UINT:
	case PIPE_FORMAT_L16A16_SINT:
		return V_0280A0_COLOR_16_16;

	case PIPE_FORMAT_R11G11B10_FLOAT:
		return V_0280A0_COLOR_10_11_11_FLOAT;

	/* 64-bit buffers. */
	case PIPE_FORMAT_R16G16B16A16_UINT:
	case PIPE_FORMAT_R16G16B16A16_SINT:
	case PIPE_FORMAT_R16G16B16A16_UNORM:
	case PIPE_FORMAT_R16G16B16A16_SNORM:
		return V_0280A0_COLOR_16_16_16_16;

	case PIPE_FORMAT_R16G16B16A16_FLOAT:
		return V_0280A0_COLOR_16_16_16_16_FLOAT;

	case PIPE_FORMAT_R32G32_FLOAT:
	case PIPE_FORMAT_L32A32_FLOAT:
		return V_0280A0_COLOR_32_32_FLOAT;

	case PIPE_FORMAT_R32G32_SINT:
	case PIPE_FORMAT_R32G32_UINT:
	case PIPE_FORMAT_L32A32_UINT:
	case PIPE_FORMAT_L32A32_SINT:
		return V_0280A0_COLOR_32_32;

	/* 128-bit buffers. */
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
		return V_0280A0_COLOR_32_32_32_32_FLOAT;
	case PIPE_FORMAT_R32G32B32A32_SNORM:
	case PIPE_FORMAT_R32G32B32A32_UNORM:
	case PIPE_FORMAT_R32G32B32A32_SINT:
	case PIPE_FORMAT_R32G32B32A32_UINT:
		return V_0280A0_COLOR_32_32_32_32;

	/* YUV buffers. */
	case PIPE_FORMAT_UYVY:
	case PIPE_FORMAT_YUYV:
	default:
		return ~0U; /* Unsupported. */
	}
}

static uint32_t r600_colorformat_endian_swap(uint32_t colorformat)
{
	if (R600_BIG_ENDIAN) {
		switch(colorformat) {
		case V_0280A0_COLOR_4_4:
			return ENDIAN_NONE;

		/* 8-bit buffers. */
		case V_0280A0_COLOR_8:
			return ENDIAN_NONE;

		/* 16-bit buffers. */
		case V_0280A0_COLOR_5_6_5:
		case V_0280A0_COLOR_1_5_5_5:
		case V_0280A0_COLOR_4_4_4_4:
		case V_0280A0_COLOR_16:
		case V_0280A0_COLOR_8_8:
			return ENDIAN_8IN16;

		/* 32-bit buffers. */
		case V_0280A0_COLOR_8_8_8_8:
		case V_0280A0_COLOR_2_10_10_10:
		case V_0280A0_COLOR_8_24:
		case V_0280A0_COLOR_24_8:
		case V_0280A0_COLOR_32_FLOAT:
		case V_0280A0_COLOR_16_16_FLOAT:
		case V_0280A0_COLOR_16_16:
			return ENDIAN_8IN32;

		/* 64-bit buffers. */
		case V_0280A0_COLOR_16_16_16_16:
		case V_0280A0_COLOR_16_16_16_16_FLOAT:
			return ENDIAN_8IN16;

		case V_0280A0_COLOR_32_32_FLOAT:
		case V_0280A0_COLOR_32_32:
		case V_0280A0_COLOR_X24_8_32_FLOAT:
			return ENDIAN_8IN32;

		/* 128-bit buffers. */
		case V_0280A0_COLOR_32_32_32_FLOAT:
		case V_0280A0_COLOR_32_32_32_32_FLOAT:
		case V_0280A0_COLOR_32_32_32_32:
			return ENDIAN_8IN32;
		default:
			return ENDIAN_NONE; /* Unsupported. */
		}
	} else {
		return ENDIAN_NONE;
	}
}

static bool r600_is_sampler_format_supported(struct pipe_screen *screen, enum pipe_format format)
{
	return r600_translate_texformat(screen, format, NULL, NULL, NULL) != ~0U;
}

static bool r600_is_colorbuffer_format_supported(enum pipe_format format)
{
	return r600_translate_colorformat(format) != ~0U &&
	       r600_translate_colorswap(format) != ~0U;
}

static bool r600_is_zs_format_supported(enum pipe_format format)
{
	return r600_translate_dbformat(format) != ~0U;
}

boolean r600_is_format_supported(struct pipe_screen *screen,
				 enum pipe_format format,
				 enum pipe_texture_target target,
				 unsigned sample_count,
				 unsigned usage)
{
	struct r600_screen *rscreen = (struct r600_screen*)screen;
	unsigned retval = 0;

	if (target >= PIPE_MAX_TEXTURE_TYPES) {
		R600_ERR("r600: unsupported texture type %d\n", target);
		return FALSE;
	}

	if (!util_format_is_supported(format, usage))
		return FALSE;

	if (sample_count > 1) {
		if (rscreen->info.drm_minor < 22)
			return FALSE;

		/* R11G11B10 is broken on R6xx. */
		if (rscreen->chip_class == R600 &&
		    format == PIPE_FORMAT_R11G11B10_FLOAT)
			return FALSE;

		/* MSAA integer colorbuffers hang. */
		if (util_format_is_pure_integer(format))
			return FALSE;

		switch (sample_count) {
		case 2:
		case 4:
		case 8:
			break;
		default:
			return FALSE;
		}
	}

	if ((usage & PIPE_BIND_SAMPLER_VIEW) &&
	    r600_is_sampler_format_supported(screen, format)) {
		retval |= PIPE_BIND_SAMPLER_VIEW;
	}

	if ((usage & (PIPE_BIND_RENDER_TARGET |
		      PIPE_BIND_DISPLAY_TARGET |
		      PIPE_BIND_SCANOUT |
		      PIPE_BIND_SHARED)) &&
	    r600_is_colorbuffer_format_supported(format)) {
		retval |= usage &
			  (PIPE_BIND_RENDER_TARGET |
			   PIPE_BIND_DISPLAY_TARGET |
			   PIPE_BIND_SCANOUT |
			   PIPE_BIND_SHARED);
	}

	if ((usage & PIPE_BIND_DEPTH_STENCIL) &&
	    r600_is_zs_format_supported(format)) {
		retval |= PIPE_BIND_DEPTH_STENCIL;
	}

	if ((usage & PIPE_BIND_VERTEX_BUFFER) &&
	    r600_is_vertex_format_supported(format)) {
		retval |= PIPE_BIND_VERTEX_BUFFER;
	}

	if (usage & PIPE_BIND_TRANSFER_READ)
		retval |= PIPE_BIND_TRANSFER_READ;
	if (usage & PIPE_BIND_TRANSFER_WRITE)
		retval |= PIPE_BIND_TRANSFER_WRITE;

	return retval == usage;
}

void r600_polygon_offset_update(struct r600_context *rctx)
{
	struct r600_pipe_state state;

	state.id = R600_PIPE_STATE_POLYGON_OFFSET;
	state.nregs = 0;
	if (rctx->rasterizer && rctx->framebuffer.zsbuf) {
		float offset_units = rctx->rasterizer->offset_units;
		unsigned offset_db_fmt_cntl = 0, depth;

		switch (rctx->framebuffer.zsbuf->format) {
		case PIPE_FORMAT_Z24X8_UNORM:
		case PIPE_FORMAT_Z24_UNORM_S8_UINT:
			depth = -24;
			offset_units *= 2.0f;
			break;
		case PIPE_FORMAT_Z32_FLOAT:
		case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
			depth = -23;
			offset_units *= 1.0f;
			offset_db_fmt_cntl |= S_028DF8_POLY_OFFSET_DB_IS_FLOAT_FMT(1);
			break;
		case PIPE_FORMAT_Z16_UNORM:
			depth = -16;
			offset_units *= 4.0f;
			break;
		default:
			return;
		}
		/* XXX some of those reg can be computed with cso */
		offset_db_fmt_cntl |= S_028DF8_POLY_OFFSET_NEG_NUM_DB_BITS(depth);
		r600_pipe_state_add_reg(&state,
				R_028E00_PA_SU_POLY_OFFSET_FRONT_SCALE,
				fui(rctx->rasterizer->offset_scale));
		r600_pipe_state_add_reg(&state,
				R_028E04_PA_SU_POLY_OFFSET_FRONT_OFFSET,
				fui(offset_units));
		r600_pipe_state_add_reg(&state,
				R_028E08_PA_SU_POLY_OFFSET_BACK_SCALE,
				fui(rctx->rasterizer->offset_scale));
		r600_pipe_state_add_reg(&state,
				R_028E0C_PA_SU_POLY_OFFSET_BACK_OFFSET,
				fui(offset_units));
		r600_pipe_state_add_reg(&state,
				R_028DF8_PA_SU_POLY_OFFSET_DB_FMT_CNTL,
				offset_db_fmt_cntl);
		r600_context_pipe_state_set(rctx, &state);
	}
}

static void *r600_create_blend_state_mode(struct pipe_context *ctx,
					  const struct pipe_blend_state *state,
					  int mode)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_blend *blend = CALLOC_STRUCT(r600_pipe_blend);
	struct r600_pipe_state *rstate;
	uint32_t color_control = 0, target_mask = 0;

	if (blend == NULL) {
		return NULL;
	}
	rstate = &blend->rstate;

	rstate->id = R600_PIPE_STATE_BLEND;

	/* R600 does not support per-MRT blends */
	if (rctx->family > CHIP_R600)
		color_control |= S_028808_PER_MRT_BLEND(1);

	if (state->logicop_enable) {
		color_control |= (state->logicop_func << 16) | (state->logicop_func << 20);
	} else {
		color_control |= (0xcc << 16);
	}
	/* we pretend 8 buffer are used, CB_SHADER_MASK will disable unused one */
	if (state->independent_blend_enable) {
		for (int i = 0; i < 8; i++) {
			if (state->rt[i].blend_enable) {
				color_control |= S_028808_TARGET_BLEND_ENABLE(1 << i);
			}
			target_mask |= (state->rt[i].colormask << (4 * i));
		}
	} else {
		for (int i = 0; i < 8; i++) {
			if (state->rt[0].blend_enable) {
				color_control |= S_028808_TARGET_BLEND_ENABLE(1 << i);
			}
			target_mask |= (state->rt[0].colormask << (4 * i));
		}
	}

	if (target_mask)
		color_control |= S_028808_SPECIAL_OP(mode);
	else
		color_control |= S_028808_SPECIAL_OP(V_028808_DISABLE);

	blend->cb_target_mask = target_mask;
	blend->cb_color_control = color_control;
	/* only MRT0 has dual src blend */
	blend->dual_src_blend = util_blend_state_is_dual(state, 0);
	for (int i = 0; i < 8; i++) {
		/* state->rt entries > 0 only written if independent blending */
		const int j = state->independent_blend_enable ? i : 0;

		unsigned eqRGB = state->rt[j].rgb_func;
		unsigned srcRGB = state->rt[j].rgb_src_factor;
		unsigned dstRGB = state->rt[j].rgb_dst_factor;

		unsigned eqA = state->rt[j].alpha_func;
		unsigned srcA = state->rt[j].alpha_src_factor;
		unsigned dstA = state->rt[j].alpha_dst_factor;
		uint32_t bc = 0;

		if (!state->rt[j].blend_enable)
			continue;

		bc |= S_028804_COLOR_COMB_FCN(r600_translate_blend_function(eqRGB));
		bc |= S_028804_COLOR_SRCBLEND(r600_translate_blend_factor(srcRGB));
		bc |= S_028804_COLOR_DESTBLEND(r600_translate_blend_factor(dstRGB));

		if (srcA != srcRGB || dstA != dstRGB || eqA != eqRGB) {
			bc |= S_028804_SEPARATE_ALPHA_BLEND(1);
			bc |= S_028804_ALPHA_COMB_FCN(r600_translate_blend_function(eqA));
			bc |= S_028804_ALPHA_SRCBLEND(r600_translate_blend_factor(srcA));
			bc |= S_028804_ALPHA_DESTBLEND(r600_translate_blend_factor(dstA));
		}

		/* R600 does not support per-MRT blends */
		if (rctx->family > CHIP_R600)
			r600_pipe_state_add_reg(rstate, R_028780_CB_BLEND0_CONTROL + i * 4, bc);
		if (i == 0)
			r600_pipe_state_add_reg(rstate, R_028804_CB_BLEND_CONTROL, bc);
	}

	r600_pipe_state_add_reg(rstate, R_028D44_DB_ALPHA_TO_MASK,
				S_028D44_ALPHA_TO_MASK_ENABLE(state->alpha_to_coverage) |
				S_028D44_ALPHA_TO_MASK_OFFSET0(2) |
				S_028D44_ALPHA_TO_MASK_OFFSET1(2) |
				S_028D44_ALPHA_TO_MASK_OFFSET2(2) |
				S_028D44_ALPHA_TO_MASK_OFFSET3(2));

	blend->alpha_to_one = state->alpha_to_one;
	return rstate;
}


static void *r600_create_blend_state(struct pipe_context *ctx,
				     const struct pipe_blend_state *state)
{
	return r600_create_blend_state_mode(ctx, state, V_028808_SPECIAL_NORMAL);
}

static void *r600_create_dsa_state(struct pipe_context *ctx,
				   const struct pipe_depth_stencil_alpha_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_dsa *dsa = CALLOC_STRUCT(r600_pipe_dsa);
	unsigned db_depth_control, alpha_test_control, alpha_ref;
	struct r600_pipe_state *rstate;

	if (dsa == NULL) {
		return NULL;
	}

	dsa->valuemask[0] = state->stencil[0].valuemask;
	dsa->valuemask[1] = state->stencil[1].valuemask;
	dsa->writemask[0] = state->stencil[0].writemask;
	dsa->writemask[1] = state->stencil[1].writemask;

	rstate = &dsa->rstate;

	rstate->id = R600_PIPE_STATE_DSA;
	db_depth_control = S_028800_Z_ENABLE(state->depth.enabled) |
		S_028800_Z_WRITE_ENABLE(state->depth.writemask) |
		S_028800_ZFUNC(state->depth.func);

	/* stencil */
	if (state->stencil[0].enabled) {
		db_depth_control |= S_028800_STENCIL_ENABLE(1);
		db_depth_control |= S_028800_STENCILFUNC(state->stencil[0].func); /* translates straight */
		db_depth_control |= S_028800_STENCILFAIL(r600_translate_stencil_op(state->stencil[0].fail_op));
		db_depth_control |= S_028800_STENCILZPASS(r600_translate_stencil_op(state->stencil[0].zpass_op));
		db_depth_control |= S_028800_STENCILZFAIL(r600_translate_stencil_op(state->stencil[0].zfail_op));

		if (state->stencil[1].enabled) {
			db_depth_control |= S_028800_BACKFACE_ENABLE(1);
			db_depth_control |= S_028800_STENCILFUNC_BF(state->stencil[1].func); /* translates straight */
			db_depth_control |= S_028800_STENCILFAIL_BF(r600_translate_stencil_op(state->stencil[1].fail_op));
			db_depth_control |= S_028800_STENCILZPASS_BF(r600_translate_stencil_op(state->stencil[1].zpass_op));
			db_depth_control |= S_028800_STENCILZFAIL_BF(r600_translate_stencil_op(state->stencil[1].zfail_op));
		}
	}

	/* alpha */
	alpha_test_control = 0;
	alpha_ref = 0;
	if (state->alpha.enabled) {
		alpha_test_control = S_028410_ALPHA_FUNC(state->alpha.func);
		alpha_test_control |= S_028410_ALPHA_TEST_ENABLE(1);
		alpha_ref = fui(state->alpha.ref_value);
	}
	dsa->sx_alpha_test_control = alpha_test_control & 0xff;
	dsa->alpha_ref = alpha_ref;

	r600_pipe_state_add_reg(rstate, R_028800_DB_DEPTH_CONTROL, db_depth_control);
	return rstate;
}

static void *r600_create_rs_state(struct pipe_context *ctx,
				  const struct pipe_rasterizer_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_rasterizer *rs = CALLOC_STRUCT(r600_pipe_rasterizer);
	struct r600_pipe_state *rstate;
	unsigned tmp;
	unsigned prov_vtx = 1, polygon_dual_mode;
	unsigned sc_mode_cntl;
	float psize_min, psize_max;

	if (rs == NULL) {
		return NULL;
	}

	polygon_dual_mode = (state->fill_front != PIPE_POLYGON_MODE_FILL ||
				state->fill_back != PIPE_POLYGON_MODE_FILL);

	if (state->flatshade_first)
		prov_vtx = 0;

	rstate = &rs->rstate;
	rs->flatshade = state->flatshade;
	rs->sprite_coord_enable = state->sprite_coord_enable;
	rs->two_side = state->light_twoside;
	rs->clip_plane_enable = state->clip_plane_enable;
	rs->pa_sc_line_stipple = state->line_stipple_enable ?
				S_028A0C_LINE_PATTERN(state->line_stipple_pattern) |
				S_028A0C_REPEAT_COUNT(state->line_stipple_factor) : 0;
	rs->pa_cl_clip_cntl =
		S_028810_PS_UCP_MODE(3) |
		S_028810_ZCLIP_NEAR_DISABLE(!state->depth_clip) |
		S_028810_ZCLIP_FAR_DISABLE(!state->depth_clip) |
		S_028810_DX_LINEAR_ATTR_CLIP_ENA(1);
	rs->multisample_enable = state->multisample;

	/* offset */
	rs->offset_units = state->offset_units;
	rs->offset_scale = state->offset_scale * 12.0f;

	rstate->id = R600_PIPE_STATE_RASTERIZER;
	tmp = S_0286D4_FLAT_SHADE_ENA(1);
	if (state->sprite_coord_enable) {
		tmp |= S_0286D4_PNT_SPRITE_ENA(1) |
			S_0286D4_PNT_SPRITE_OVRD_X(2) |
			S_0286D4_PNT_SPRITE_OVRD_Y(3) |
			S_0286D4_PNT_SPRITE_OVRD_Z(0) |
			S_0286D4_PNT_SPRITE_OVRD_W(1);
		if (state->sprite_coord_mode != PIPE_SPRITE_COORD_UPPER_LEFT) {
			tmp |= S_0286D4_PNT_SPRITE_TOP_1(1);
		}
	}
	r600_pipe_state_add_reg(rstate, R_0286D4_SPI_INTERP_CONTROL_0, tmp);

	/* point size 12.4 fixed point */
	tmp = r600_pack_float_12p4(state->point_size/2);
	r600_pipe_state_add_reg(rstate, R_028A00_PA_SU_POINT_SIZE, S_028A00_HEIGHT(tmp) | S_028A00_WIDTH(tmp));

	if (state->point_size_per_vertex) {
		psize_min = util_get_min_point_size(state);
		psize_max = 8192;
	} else {
		/* Force the point size to be as if the vertex output was disabled. */
		psize_min = state->point_size;
		psize_max = state->point_size;
	}
	/* Divide by two, because 0.5 = 1 pixel. */
	r600_pipe_state_add_reg(rstate, R_028A04_PA_SU_POINT_MINMAX,
				S_028A04_MIN_SIZE(r600_pack_float_12p4(psize_min/2)) |
				S_028A04_MAX_SIZE(r600_pack_float_12p4(psize_max/2)));

	tmp = r600_pack_float_12p4(state->line_width/2);
	r600_pipe_state_add_reg(rstate, R_028A08_PA_SU_LINE_CNTL, S_028A08_WIDTH(tmp));

	if (rctx->chip_class >= R700) {
		sc_mode_cntl =
			S_028A4C_MSAA_ENABLE(state->multisample) |
			S_028A4C_FORCE_EOV_CNTDWN_ENABLE(1) |
			S_028A4C_FORCE_EOV_REZ_ENABLE(1) |
			S_028A4C_R700_ZMM_LINE_OFFSET(1) |
			S_028A4C_R700_VPORT_SCISSOR_ENABLE(state->scissor);
	} else {
		sc_mode_cntl =
			S_028A4C_MSAA_ENABLE(state->multisample) |
			S_028A4C_WALK_ALIGN8_PRIM_FITS_ST(1) |
			S_028A4C_FORCE_EOV_CNTDWN_ENABLE(1);
		rs->scissor_enable = state->scissor;
	}
	sc_mode_cntl |= S_028A4C_LINE_STIPPLE_ENABLE(state->line_stipple_enable);
	
	r600_pipe_state_add_reg(rstate, R_028A4C_PA_SC_MODE_CNTL, sc_mode_cntl);

	r600_pipe_state_add_reg(rstate, R_028C08_PA_SU_VTX_CNTL,
				S_028C08_PIX_CENTER_HALF(state->gl_rasterization_rules) |
				S_028C08_QUANT_MODE(V_028C08_X_1_256TH));

	r600_pipe_state_add_reg(rstate, R_028DFC_PA_SU_POLY_OFFSET_CLAMP, fui(state->offset_clamp));
	r600_pipe_state_add_reg(rstate, R_028814_PA_SU_SC_MODE_CNTL,
				S_028814_PROVOKING_VTX_LAST(prov_vtx) |
				S_028814_CULL_FRONT(state->cull_face & PIPE_FACE_FRONT ? 1 : 0) |
				S_028814_CULL_BACK(state->cull_face & PIPE_FACE_BACK ? 1 : 0) |
				S_028814_FACE(!state->front_ccw) |
				S_028814_POLY_OFFSET_FRONT_ENABLE(state->offset_tri) |
				S_028814_POLY_OFFSET_BACK_ENABLE(state->offset_tri) |
				S_028814_POLY_OFFSET_PARA_ENABLE(state->offset_tri) |
				S_028814_POLY_MODE(polygon_dual_mode) |
				S_028814_POLYMODE_FRONT_PTYPE(r600_translate_fill(state->fill_front)) |
				S_028814_POLYMODE_BACK_PTYPE(r600_translate_fill(state->fill_back)));
	r600_pipe_state_add_reg(rstate, R_028350_SX_MISC, S_028350_MULTIPASS(state->rasterizer_discard));
	return rstate;
}

static void *r600_create_sampler_state(struct pipe_context *ctx,
					const struct pipe_sampler_state *state)
{
	struct r600_pipe_sampler_state *ss = CALLOC_STRUCT(r600_pipe_sampler_state);
	union util_color uc;
	unsigned aniso_flag_offset = state->max_anisotropy > 1 ? 4 : 0;

	if (ss == NULL) {
		return NULL;
	}

	ss->seamless_cube_map = state->seamless_cube_map;
	ss->border_color_use = false;
	util_pack_color(state->border_color.f, PIPE_FORMAT_B8G8R8A8_UNORM, &uc);
	/* R_03C000_SQ_TEX_SAMPLER_WORD0_0 */
	ss->tex_sampler_words[0] = S_03C000_CLAMP_X(r600_tex_wrap(state->wrap_s)) |
				S_03C000_CLAMP_Y(r600_tex_wrap(state->wrap_t)) |
				S_03C000_CLAMP_Z(r600_tex_wrap(state->wrap_r)) |
				S_03C000_XY_MAG_FILTER(r600_tex_filter(state->mag_img_filter) | aniso_flag_offset) |
				S_03C000_XY_MIN_FILTER(r600_tex_filter(state->min_img_filter) | aniso_flag_offset) |
				S_03C000_MIP_FILTER(r600_tex_mipfilter(state->min_mip_filter)) |
				S_03C000_MAX_ANISO(r600_tex_aniso_filter(state->max_anisotropy)) |
				S_03C000_DEPTH_COMPARE_FUNCTION(r600_tex_compare(state->compare_func)) |
				S_03C000_BORDER_COLOR_TYPE(uc.ui ? V_03C000_SQ_TEX_BORDER_COLOR_REGISTER : 0);
	/* R_03C004_SQ_TEX_SAMPLER_WORD1_0 */
	ss->tex_sampler_words[1] = S_03C004_MIN_LOD(S_FIXED(CLAMP(state->min_lod, 0, 15), 6)) |
				S_03C004_MAX_LOD(S_FIXED(CLAMP(state->max_lod, 0, 15), 6)) |
				S_03C004_LOD_BIAS(S_FIXED(CLAMP(state->lod_bias, -16, 16), 6));
	/* R_03C008_SQ_TEX_SAMPLER_WORD2_0 */
	ss->tex_sampler_words[2] = S_03C008_TYPE(1);
	if (uc.ui) {
		ss->border_color_use = true;
		/* R_00A400_TD_PS_SAMPLER0_BORDER_RED */
		ss->border_color[0] = fui(state->border_color.f[0]);
		/* R_00A404_TD_PS_SAMPLER0_BORDER_GREEN */
		ss->border_color[1] = fui(state->border_color.f[1]);
		/* R_00A408_TD_PS_SAMPLER0_BORDER_BLUE */
		ss->border_color[2] = fui(state->border_color.f[2]);
		/* R_00A40C_TD_PS_SAMPLER0_BORDER_ALPHA */
		ss->border_color[3] = fui(state->border_color.f[3]);
	}
	return ss;
}

static struct pipe_sampler_view *r600_create_sampler_view(struct pipe_context *ctx,
							struct pipe_resource *texture,
							const struct pipe_sampler_view *state)
{
	struct r600_pipe_sampler_view *view = CALLOC_STRUCT(r600_pipe_sampler_view);
	struct r600_texture *tmp = (struct r600_texture*)texture;
	unsigned format, endian;
	uint32_t word4 = 0, yuv_format = 0, pitch = 0;
	unsigned char swizzle[4], array_mode = 0, tile_type = 0;
	unsigned width, height, depth, offset_level, last_level;

	if (view == NULL)
		return NULL;

	/* initialize base object */
	view->base = *state;
	view->base.texture = NULL;
	pipe_reference(NULL, &texture->reference);
	view->base.texture = texture;
	view->base.reference.count = 1;
	view->base.context = ctx;

	swizzle[0] = state->swizzle_r;
	swizzle[1] = state->swizzle_g;
	swizzle[2] = state->swizzle_b;
	swizzle[3] = state->swizzle_a;

	format = r600_translate_texformat(ctx->screen, state->format,
					  swizzle,
					  &word4, &yuv_format);
	assert(format != ~0);
	if (format == ~0) {
		FREE(view);
		return NULL;
	}

	if (tmp->is_depth && !tmp->is_flushing_texture) {
		if (!r600_init_flushed_depth_texture(ctx, texture, NULL)) {
			FREE(view);
			return NULL;
		}
		tmp = tmp->flushed_depth_texture;
	}

	endian = r600_colorformat_endian_swap(format);

	offset_level = state->u.tex.first_level;
	last_level = state->u.tex.last_level - offset_level;
	width = tmp->surface.level[offset_level].npix_x;
	height = tmp->surface.level[offset_level].npix_y;
	depth = tmp->surface.level[offset_level].npix_z;
	pitch = tmp->surface.level[offset_level].nblk_x * util_format_get_blockwidth(state->format);
	tile_type = tmp->tile_type;

	if (texture->target == PIPE_TEXTURE_1D_ARRAY) {
		height = 1;
		depth = texture->array_size;
	} else if (texture->target == PIPE_TEXTURE_2D_ARRAY) {
		depth = texture->array_size;
	}
	switch (tmp->surface.level[offset_level].mode) {
	case RADEON_SURF_MODE_LINEAR_ALIGNED:
		array_mode = V_038000_ARRAY_LINEAR_ALIGNED;
		break;
	case RADEON_SURF_MODE_1D:
		array_mode = V_038000_ARRAY_1D_TILED_THIN1;
		break;
	case RADEON_SURF_MODE_2D:
		array_mode = V_038000_ARRAY_2D_TILED_THIN1;
		break;
	case RADEON_SURF_MODE_LINEAR:
	default:
		array_mode = V_038000_ARRAY_LINEAR_GENERAL;
		break;
	}

	view->tex_resource = &tmp->resource;
	view->tex_resource_words[0] = (S_038000_DIM(r600_tex_dim(texture->target, texture->nr_samples)) |
				       S_038000_TILE_MODE(array_mode) |
				       S_038000_TILE_TYPE(tile_type) |
				       S_038000_PITCH((pitch / 8) - 1) |
				       S_038000_TEX_WIDTH(width - 1));
	view->tex_resource_words[1] = (S_038004_TEX_HEIGHT(height - 1) |
				       S_038004_TEX_DEPTH(depth - 1) |
				       S_038004_DATA_FORMAT(format));
	view->tex_resource_words[2] = tmp->surface.level[offset_level].offset >> 8;
	if (offset_level >= tmp->surface.last_level) {
		view->tex_resource_words[3] = tmp->surface.level[offset_level].offset >> 8;
	} else {
		view->tex_resource_words[3] = tmp->surface.level[offset_level + 1].offset >> 8;
	}
	view->tex_resource_words[4] = (word4 |
				       S_038010_SRF_MODE_ALL(V_038010_SRF_MODE_ZERO_CLAMP_MINUS_ONE) |
				       S_038010_REQUEST_SIZE(1) |
				       S_038010_ENDIAN_SWAP(endian) |
				       S_038010_BASE_LEVEL(0));
	view->tex_resource_words[5] = (S_038014_BASE_ARRAY(state->u.tex.first_layer) |
				       S_038014_LAST_ARRAY(state->u.tex.last_layer));
	if (texture->nr_samples > 1) {
		/* LAST_LEVEL holds log2(nr_samples) for multisample textures */
		view->tex_resource_words[5] |= S_038014_LAST_LEVEL(util_logbase2(texture->nr_samples));
	} else {
		view->tex_resource_words[5] |= S_038014_LAST_LEVEL(last_level);
	}
	view->tex_resource_words[6] = (S_038018_TYPE(V_038010_SQ_TEX_VTX_VALID_TEXTURE) |
				       S_038018_MAX_ANISO(4 /* max 16 samples */));
	return &view->base;
}

static void r600_set_vs_sampler_views(struct pipe_context *ctx, unsigned count,
				      struct pipe_sampler_view **views)
{
	r600_set_sampler_views(ctx, PIPE_SHADER_VERTEX, 0, count, views);
}

static void r600_set_ps_sampler_views(struct pipe_context *ctx, unsigned count,
				      struct pipe_sampler_view **views)
{
	r600_set_sampler_views(ctx, PIPE_SHADER_FRAGMENT, 0, count, views);
}

static void r600_set_clip_state(struct pipe_context *ctx,
				const struct pipe_clip_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_state *rstate = CALLOC_STRUCT(r600_pipe_state);
	struct pipe_constant_buffer cb;

	if (rstate == NULL)
		return;

	rctx->clip = *state;
	rstate->id = R600_PIPE_STATE_CLIP;
	for (int i = 0; i < 6; i++) {
		r600_pipe_state_add_reg(rstate,
					R_028E20_PA_CL_UCP0_X + i * 16,
					fui(state->ucp[i][0]));
		r600_pipe_state_add_reg(rstate,
					R_028E24_PA_CL_UCP0_Y + i * 16,
					fui(state->ucp[i][1]) );
		r600_pipe_state_add_reg(rstate,
					R_028E28_PA_CL_UCP0_Z + i * 16,
					fui(state->ucp[i][2]));
		r600_pipe_state_add_reg(rstate,
					R_028E2C_PA_CL_UCP0_W + i * 16,
					fui(state->ucp[i][3]));
	}

	free(rctx->states[R600_PIPE_STATE_CLIP]);
	rctx->states[R600_PIPE_STATE_CLIP] = rstate;
	r600_context_pipe_state_set(rctx, rstate);

	cb.buffer = NULL;
	cb.user_buffer = state->ucp;
	cb.buffer_offset = 0;
	cb.buffer_size = 4*4*8;
	r600_set_constant_buffer(ctx, PIPE_SHADER_VERTEX, 1, &cb);
	pipe_resource_reference(&cb.buffer, NULL);
}

static void r600_set_polygon_stipple(struct pipe_context *ctx,
					 const struct pipe_poly_stipple *state)
{
}

void r600_set_scissor_state(struct r600_context *rctx,
			    const struct pipe_scissor_state *state)
{
	struct r600_pipe_state *rstate = CALLOC_STRUCT(r600_pipe_state);
	uint32_t tl, br;

	if (rstate == NULL)
		return;

	rstate->id = R600_PIPE_STATE_SCISSOR;
	tl = S_028240_TL_X(state->minx) | S_028240_TL_Y(state->miny) | S_028240_WINDOW_OFFSET_DISABLE(1);
	br = S_028244_BR_X(state->maxx) | S_028244_BR_Y(state->maxy);
	r600_pipe_state_add_reg(rstate,
				R_028250_PA_SC_VPORT_SCISSOR_0_TL, tl);
	r600_pipe_state_add_reg(rstate,
				R_028254_PA_SC_VPORT_SCISSOR_0_BR, br);

	free(rctx->states[R600_PIPE_STATE_SCISSOR]);
	rctx->states[R600_PIPE_STATE_SCISSOR] = rstate;
	r600_context_pipe_state_set(rctx, rstate);
}

static void r600_pipe_set_scissor_state(struct pipe_context *ctx,
					const struct pipe_scissor_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	if (rctx->chip_class == R600) {
		rctx->scissor_state = *state;

		if (!rctx->scissor_enable)
			return;
	}

	r600_set_scissor_state(rctx, state);
}

static void r600_set_viewport_state(struct pipe_context *ctx,
					const struct pipe_viewport_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_state *rstate = CALLOC_STRUCT(r600_pipe_state);

	if (rstate == NULL)
		return;

	rctx->viewport = *state;
	rstate->id = R600_PIPE_STATE_VIEWPORT;
	r600_pipe_state_add_reg(rstate, R_02843C_PA_CL_VPORT_XSCALE_0, fui(state->scale[0]));
	r600_pipe_state_add_reg(rstate, R_028444_PA_CL_VPORT_YSCALE_0, fui(state->scale[1]));
	r600_pipe_state_add_reg(rstate, R_02844C_PA_CL_VPORT_ZSCALE_0, fui(state->scale[2]));
	r600_pipe_state_add_reg(rstate, R_028440_PA_CL_VPORT_XOFFSET_0, fui(state->translate[0]));
	r600_pipe_state_add_reg(rstate, R_028448_PA_CL_VPORT_YOFFSET_0, fui(state->translate[1]));
	r600_pipe_state_add_reg(rstate, R_028450_PA_CL_VPORT_ZOFFSET_0, fui(state->translate[2]));

	free(rctx->states[R600_PIPE_STATE_VIEWPORT]);
	rctx->states[R600_PIPE_STATE_VIEWPORT] = rstate;
	r600_context_pipe_state_set(rctx, rstate);
}

static struct r600_resource *r600_buffer_create_helper(struct r600_screen *rscreen,
						       unsigned size, unsigned alignment)
{
	struct pipe_resource buffer;

	memset(&buffer, 0, sizeof buffer);
	buffer.target = PIPE_BUFFER;
	buffer.format = PIPE_FORMAT_R8_UNORM;
	buffer.bind = PIPE_BIND_CUSTOM;
	buffer.usage = PIPE_USAGE_STATIC;
	buffer.flags = 0;
	buffer.width0 = size;
	buffer.height0 = 1;
	buffer.depth0 = 1;
	buffer.array_size = 1;

	return (struct r600_resource*)
		r600_buffer_create(&rscreen->screen, &buffer, alignment);
}

static void r600_init_color_surface(struct r600_context *rctx,
				    struct r600_surface *surf,
				    bool force_cmask_fmask)
{
	struct r600_screen *rscreen = rctx->screen;
	struct r600_texture *rtex = (struct r600_texture*)surf->base.texture;
	unsigned level = surf->base.u.tex.level;
	unsigned pitch, slice;
	unsigned color_info;
	unsigned format, swap, ntype, endian;
	unsigned offset;
	const struct util_format_description *desc;
	int i;
	bool blend_bypass = 0, blend_clamp = 1;

	if (rtex->is_depth && !rtex->is_flushing_texture) {
		r600_init_flushed_depth_texture(&rctx->context, surf->base.texture, NULL);
		rtex = rtex->flushed_depth_texture;
		assert(rtex);
	}

	offset = rtex->surface.level[level].offset;
	if (rtex->surface.level[level].mode < RADEON_SURF_MODE_1D) {
		offset += rtex->surface.level[level].slice_size *
			  surf->base.u.tex.first_layer;
	}
	pitch = rtex->surface.level[level].nblk_x / 8 - 1;
	slice = (rtex->surface.level[level].nblk_x * rtex->surface.level[level].nblk_y) / 64;
	if (slice) {
		slice = slice - 1;
	}
	color_info = 0;
	switch (rtex->surface.level[level].mode) {
	case RADEON_SURF_MODE_LINEAR_ALIGNED:
		color_info = S_0280A0_ARRAY_MODE(V_038000_ARRAY_LINEAR_ALIGNED);
		break;
	case RADEON_SURF_MODE_1D:
		color_info = S_0280A0_ARRAY_MODE(V_038000_ARRAY_1D_TILED_THIN1);
		break;
	case RADEON_SURF_MODE_2D:
		color_info = S_0280A0_ARRAY_MODE(V_038000_ARRAY_2D_TILED_THIN1);
		break;
	case RADEON_SURF_MODE_LINEAR:
	default:
		color_info = S_0280A0_ARRAY_MODE(V_038000_ARRAY_LINEAR_GENERAL);
		break;
	}

	desc = util_format_description(surf->base.format);

	for (i = 0; i < 4; i++) {
		if (desc->channel[i].type != UTIL_FORMAT_TYPE_VOID) {
			break;
		}
	}

	ntype = V_0280A0_NUMBER_UNORM;
	if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
		ntype = V_0280A0_NUMBER_SRGB;
	else if (desc->channel[i].type == UTIL_FORMAT_TYPE_SIGNED) {
		if (desc->channel[i].normalized)
			ntype = V_0280A0_NUMBER_SNORM;
		else if (desc->channel[i].pure_integer)
			ntype = V_0280A0_NUMBER_SINT;
	} else if (desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED) {
		if (desc->channel[i].normalized)
			ntype = V_0280A0_NUMBER_UNORM;
		else if (desc->channel[i].pure_integer)
			ntype = V_0280A0_NUMBER_UINT;
	}

	format = r600_translate_colorformat(surf->base.format);
	assert(format != ~0);

	swap = r600_translate_colorswap(surf->base.format);
	assert(swap != ~0);

	if (rtex->resource.b.b.usage == PIPE_USAGE_STAGING) {
		endian = ENDIAN_NONE;
	} else {
		endian = r600_colorformat_endian_swap(format);
	}

	/* set blend bypass according to docs if SINT/UINT or
	   8/24 COLOR variants */
	if (ntype == V_0280A0_NUMBER_UINT || ntype == V_0280A0_NUMBER_SINT ||
	    format == V_0280A0_COLOR_8_24 || format == V_0280A0_COLOR_24_8 ||
	    format == V_0280A0_COLOR_X24_8_32_FLOAT) {
		blend_clamp = 0;
		blend_bypass = 1;
	}

	surf->alphatest_bypass = ntype == V_0280A0_NUMBER_UINT || ntype == V_0280A0_NUMBER_SINT;

	color_info |= S_0280A0_FORMAT(format) |
		S_0280A0_COMP_SWAP(swap) |
		S_0280A0_BLEND_BYPASS(blend_bypass) |
		S_0280A0_BLEND_CLAMP(blend_clamp) |
		S_0280A0_NUMBER_TYPE(ntype) |
		S_0280A0_ENDIAN(endian);

	/* EXPORT_NORM is an optimzation that can be enabled for better
	 * performance in certain cases
	 */
	if (rctx->chip_class == R600) {
		/* EXPORT_NORM can be enabled if:
		 * - 11-bit or smaller UNORM/SNORM/SRGB
		 * - BLEND_CLAMP is enabled
		 * - BLEND_FLOAT32 is disabled
		 */
		if (desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS &&
		    (desc->channel[i].size < 12 &&
		     desc->channel[i].type != UTIL_FORMAT_TYPE_FLOAT &&
		     ntype != V_0280A0_NUMBER_UINT &&
		     ntype != V_0280A0_NUMBER_SINT) &&
		    G_0280A0_BLEND_CLAMP(color_info) &&
		    !G_0280A0_BLEND_FLOAT32(color_info)) {
			color_info |= S_0280A0_SOURCE_FORMAT(V_0280A0_EXPORT_NORM);
			surf->export_16bpc = true;
		}
	} else {
		/* EXPORT_NORM can be enabled if:
		 * - 11-bit or smaller UNORM/SNORM/SRGB
		 * - 16-bit or smaller FLOAT
		 */
		if (desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS &&
		    ((desc->channel[i].size < 12 &&
		      desc->channel[i].type != UTIL_FORMAT_TYPE_FLOAT &&
		      ntype != V_0280A0_NUMBER_UINT && ntype != V_0280A0_NUMBER_SINT) ||
		    (desc->channel[i].size < 17 &&
		     desc->channel[i].type == UTIL_FORMAT_TYPE_FLOAT))) {
			color_info |= S_0280A0_SOURCE_FORMAT(V_0280A0_EXPORT_NORM);
			surf->export_16bpc = true;
		}
	}

	/* These might not always be initialized to zero. */
	surf->cb_color_base = offset >> 8;
	surf->cb_color_size = S_028060_PITCH_TILE_MAX(pitch) |
			      S_028060_SLICE_TILE_MAX(slice);
	surf->cb_color_fmask = surf->cb_color_base;
	surf->cb_color_cmask = surf->cb_color_base;
	surf->cb_color_mask = 0;

	pipe_resource_reference((struct pipe_resource**)&surf->cb_buffer_cmask,
				&rtex->resource.b.b);
	pipe_resource_reference((struct pipe_resource**)&surf->cb_buffer_fmask,
				&rtex->resource.b.b);

	if (rtex->cmask_size) {
		surf->cb_color_cmask = rtex->cmask_offset >> 8;
		surf->cb_color_mask |= S_028100_CMASK_BLOCK_MAX(rtex->cmask_slice_tile_max);

		if (rtex->fmask_size) {
			color_info |= S_0280A0_TILE_MODE(V_0280A0_FRAG_ENABLE);
			surf->cb_color_fmask = rtex->fmask_offset >> 8;
			surf->cb_color_mask |= S_028100_FMASK_TILE_MAX(slice);
		} else { /* cmask only */
			color_info |= S_0280A0_TILE_MODE(V_0280A0_CLEAR_ENABLE);
		}
	} else if (force_cmask_fmask) {
		/* Allocate dummy FMASK and CMASK if they aren't allocated already.
		 *
		 * R6xx needs FMASK and CMASK for the destination buffer of color resolve,
		 * otherwise it hangs. We don't have FMASK and CMASK pre-allocated,
		 * because it's not an MSAA buffer.
		 */
		struct r600_cmask_info cmask;
		struct r600_fmask_info fmask;

		r600_texture_get_cmask_info(rscreen, rtex, &cmask);
		r600_texture_get_fmask_info(rscreen, rtex, 8, &fmask);

		/* CMASK. */
		if (!rctx->dummy_cmask ||
		    rctx->dummy_cmask->buf->size < cmask.size ||
		    rctx->dummy_cmask->buf->alignment % cmask.alignment != 0) {
			struct pipe_transfer *transfer;
			void *ptr;

			pipe_resource_reference((struct pipe_resource**)&rctx->dummy_cmask, NULL);
			rctx->dummy_cmask = r600_buffer_create_helper(rscreen, cmask.size, cmask.alignment);

			/* Set the contents to 0xCC. */
			ptr = pipe_buffer_map(&rctx->context, &rctx->dummy_cmask->b.b, PIPE_TRANSFER_WRITE, &transfer);
			memset(ptr, 0xCC, cmask.size);
			pipe_buffer_unmap(&rctx->context, transfer);
		}
		pipe_resource_reference((struct pipe_resource**)&surf->cb_buffer_cmask,
					&rctx->dummy_cmask->b.b);

		/* FMASK. */
		if (!rctx->dummy_fmask ||
		    rctx->dummy_fmask->buf->size < fmask.size ||
		    rctx->dummy_fmask->buf->alignment % fmask.alignment != 0) {
			pipe_resource_reference((struct pipe_resource**)&rctx->dummy_fmask, NULL);
			rctx->dummy_fmask = r600_buffer_create_helper(rscreen, fmask.size, fmask.alignment);

		}
		pipe_resource_reference((struct pipe_resource**)&surf->cb_buffer_fmask,
					&rctx->dummy_fmask->b.b);

		/* Init the registers. */
		color_info |= S_0280A0_TILE_MODE(V_0280A0_FRAG_ENABLE);
		surf->cb_color_cmask = 0;
		surf->cb_color_fmask = 0;
		surf->cb_color_mask = S_028100_CMASK_BLOCK_MAX(cmask.slice_tile_max) |
				      S_028100_FMASK_TILE_MAX(slice);
	}

	surf->cb_color_info = color_info;

	if (rtex->surface.level[level].mode < RADEON_SURF_MODE_1D) {
		surf->cb_color_view = 0;
	} else {
		surf->cb_color_view = S_028080_SLICE_START(surf->base.u.tex.first_layer) |
				      S_028080_SLICE_MAX(surf->base.u.tex.last_layer);
	}

	surf->color_initialized = true;
}

static void r600_init_depth_surface(struct r600_context *rctx,
				    struct r600_surface *surf)
{
	struct r600_texture *rtex = (struct r600_texture*)surf->base.texture;
	unsigned level, pitch, slice, format, offset, array_mode;

	level = surf->base.u.tex.level;
	offset = rtex->surface.level[level].offset;
	pitch = rtex->surface.level[level].nblk_x / 8 - 1;
	slice = (rtex->surface.level[level].nblk_x * rtex->surface.level[level].nblk_y) / 64;
	if (slice) {
		slice = slice - 1;
	}
	switch (rtex->surface.level[level].mode) {
	case RADEON_SURF_MODE_2D:
		array_mode = V_0280A0_ARRAY_2D_TILED_THIN1;
		break;
	case RADEON_SURF_MODE_1D:
	case RADEON_SURF_MODE_LINEAR_ALIGNED:
	case RADEON_SURF_MODE_LINEAR:
	default:
		array_mode = V_0280A0_ARRAY_1D_TILED_THIN1;
		break;
	}

	format = r600_translate_dbformat(surf->base.format);
	assert(format != ~0);

	surf->db_depth_info = S_028010_ARRAY_MODE(array_mode) | S_028010_FORMAT(format);
	surf->db_depth_base = offset >> 8;
	surf->db_depth_view = S_028004_SLICE_START(surf->base.u.tex.first_layer) |
			      S_028004_SLICE_MAX(surf->base.u.tex.last_layer);
	surf->db_depth_size = S_028000_PITCH_TILE_MAX(pitch) | S_028000_SLICE_TILE_MAX(slice);
	surf->db_prefetch_limit = (rtex->surface.level[level].nblk_y / 8) - 1;

	surf->depth_initialized = true;
}

#define FILL_SREG(s0x, s0y, s1x, s1y, s2x, s2y, s3x, s3y)  \
	(((s0x) & 0xf) | (((s0y) & 0xf) << 4) |		   \
	(((s1x) & 0xf) << 8) | (((s1y) & 0xf) << 12) |	   \
	(((s2x) & 0xf) << 16) | (((s2y) & 0xf) << 20) |	   \
	 (((s3x) & 0xf) << 24) | (((s3y) & 0xf) << 28))

static uint32_t r600_set_ms_pos(struct pipe_context *ctx, struct r600_pipe_state *rstate, int nsample)
{
	static uint32_t sample_locs_2x[] = {
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
	};
	static unsigned max_dist_2x = 4;
	static uint32_t sample_locs_4x[] = {
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
	};
	static unsigned max_dist_4x = 6;
	static uint32_t sample_locs_8x[] = {
		FILL_SREG(-2, -5, 3, -4, -1, 5, -6, -2),
		FILL_SREG( 6,  0, 0,  0, -5, 3,  4,  4),
	};
	static unsigned max_dist_8x = 8;
	struct r600_context *rctx = (struct r600_context *)ctx;

	if (rctx->family == CHIP_R600) {
		switch (nsample) {
		case 0:
		case 1:
			return 0;
		case 2:
			r600_pipe_state_add_reg(rstate, R_008B40_PA_SC_AA_SAMPLE_LOCS_2S, sample_locs_2x[0]);
			return max_dist_2x;
		case 4:
			r600_pipe_state_add_reg(rstate, R_008B44_PA_SC_AA_SAMPLE_LOCS_4S, sample_locs_4x[0]);
			return max_dist_4x;
		case 8:
			r600_pipe_state_add_reg(rstate, R_008B48_PA_SC_AA_SAMPLE_LOCS_8S_WD0, sample_locs_8x[0]);
			r600_pipe_state_add_reg(rstate, R_008B4C_PA_SC_AA_SAMPLE_LOCS_8S_WD1, sample_locs_8x[1]);
			return max_dist_8x;
		}
	} else {
		switch (nsample) {
		case 0:
		case 1:
			r600_pipe_state_add_reg(rstate, R_028C1C_PA_SC_AA_SAMPLE_LOCS_MCTX, 0);
			r600_pipe_state_add_reg(rstate, R_028C20_PA_SC_AA_SAMPLE_LOCS_8D_WD1_MCTX, 0);
			return 0;
		case 2:
			r600_pipe_state_add_reg(rstate, R_028C1C_PA_SC_AA_SAMPLE_LOCS_MCTX, sample_locs_2x[0]);
			r600_pipe_state_add_reg(rstate, R_028C20_PA_SC_AA_SAMPLE_LOCS_8D_WD1_MCTX, sample_locs_2x[1]);
			return max_dist_2x;
		case 4:
			r600_pipe_state_add_reg(rstate, R_028C1C_PA_SC_AA_SAMPLE_LOCS_MCTX, sample_locs_4x[0]);
			r600_pipe_state_add_reg(rstate, R_028C20_PA_SC_AA_SAMPLE_LOCS_8D_WD1_MCTX, sample_locs_4x[1]);
			return max_dist_4x;
		case 8:
			r600_pipe_state_add_reg(rstate, R_028C1C_PA_SC_AA_SAMPLE_LOCS_MCTX, sample_locs_8x[0]);
			r600_pipe_state_add_reg(rstate, R_028C20_PA_SC_AA_SAMPLE_LOCS_8D_WD1_MCTX, sample_locs_8x[1]);
			return max_dist_8x;
		}
	}
	R600_ERR("Invalid nr_samples %i\n", nsample);
	return 0;
}

static void r600_set_framebuffer_state(struct pipe_context *ctx,
					const struct pipe_framebuffer_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_state *rstate = CALLOC_STRUCT(r600_pipe_state);
	struct r600_surface *surf;
	struct r600_resource *res;
	struct r600_texture *rtex;
	uint32_t tl, br, i, nr_samples, max_dist;
	bool is_resolve = state->nr_cbufs == 2 &&
			  state->cbufs[0]->texture->nr_samples > 1 &&
		          state->cbufs[1]->texture->nr_samples <= 1;
	/* The resolve buffer must have CMASK and FMASK to prevent hardlocks on R6xx. */
	bool cb1_force_cmask_fmask = rctx->chip_class == R600 && is_resolve;

	if (rstate == NULL)
		return;

	r600_flush_framebuffer(rctx, false);

	/* unreference old buffer and reference new one */
	rstate->id = R600_PIPE_STATE_FRAMEBUFFER;

	util_copy_framebuffer_state(&rctx->framebuffer, state);

	/* Colorbuffers. */
	rctx->export_16bpc = true;
	rctx->nr_cbufs = state->nr_cbufs;
	rctx->cb0_is_integer = state->nr_cbufs &&
			       util_format_is_pure_integer(state->cbufs[0]->format);
	rctx->compressed_cb_mask = 0;

	for (i = 0; i < state->nr_cbufs; i++) {
		bool force_cmask_fmask = cb1_force_cmask_fmask && i == 1;
		surf = (struct r600_surface*)state->cbufs[i];
		res = (struct r600_resource*)surf->base.texture;
		rtex = (struct r600_texture*)res;

		r600_context_add_resource_size(ctx, state->cbufs[i]->texture);

		if (!surf->color_initialized || force_cmask_fmask) {
			r600_init_color_surface(rctx, surf, force_cmask_fmask);
			if (force_cmask_fmask) {
				/* re-initialize later without compression */
				surf->color_initialized = false;
			}
		}

		if (!surf->export_16bpc) {
			rctx->export_16bpc = false;
		}

		r600_pipe_state_add_reg_bo(rstate, R_028040_CB_COLOR0_BASE + i * 4,
					   surf->cb_color_base, res, RADEON_USAGE_READWRITE);
		r600_pipe_state_add_reg_bo(rstate, R_0280A0_CB_COLOR0_INFO + i * 4,
					   surf->cb_color_info, res, RADEON_USAGE_READWRITE);
		r600_pipe_state_add_reg(rstate, R_028060_CB_COLOR0_SIZE + i * 4,
					surf->cb_color_size);
		r600_pipe_state_add_reg(rstate, R_028080_CB_COLOR0_VIEW + i * 4,
					surf->cb_color_view);
		r600_pipe_state_add_reg_bo(rstate, R_0280E0_CB_COLOR0_FRAG + i * 4,
					   surf->cb_color_fmask, surf->cb_buffer_fmask,
					   RADEON_USAGE_READWRITE);
		r600_pipe_state_add_reg_bo(rstate, R_0280C0_CB_COLOR0_TILE + i * 4,
					   surf->cb_color_cmask, surf->cb_buffer_cmask,
					   RADEON_USAGE_READWRITE);
		r600_pipe_state_add_reg(rstate, R_028100_CB_COLOR0_MASK + i * 4,
					surf->cb_color_mask);

		if (rtex->fmask_size && rtex->cmask_size) {
			rctx->compressed_cb_mask |= 1 << i;
		}
	}
	/* set CB_COLOR1_INFO for possible dual-src blending */
	if (i == 1) {
		r600_pipe_state_add_reg_bo(rstate, R_0280A0_CB_COLOR0_INFO + 1 * 4,
					   surf->cb_color_info, res, RADEON_USAGE_READWRITE);
		i++;
	}
	for (; i < 8 ; i++) {
		r600_pipe_state_add_reg(rstate, R_0280A0_CB_COLOR0_INFO + i * 4, 0);
	}

	/* Update alpha-test state dependencies.
	 * Alpha-test is done on the first colorbuffer only. */
	if (state->nr_cbufs) {
		surf = (struct r600_surface*)state->cbufs[0];
		if (rctx->alphatest_state.bypass != surf->alphatest_bypass) {
			rctx->alphatest_state.bypass = surf->alphatest_bypass;
			r600_atom_dirty(rctx, &rctx->alphatest_state.atom);
		}
	}

	/* ZS buffer. */
	if (state->zsbuf) {
		surf = (struct r600_surface*)state->zsbuf;
		res = (struct r600_resource*)surf->base.texture;

		r600_context_add_resource_size(ctx, state->zsbuf->texture);

		if (!surf->depth_initialized) {
			r600_init_depth_surface(rctx, surf);
		}

		r600_pipe_state_add_reg_bo(rstate, R_02800C_DB_DEPTH_BASE, surf->db_depth_base,
					   res, RADEON_USAGE_READWRITE);
		r600_pipe_state_add_reg(rstate, R_028000_DB_DEPTH_SIZE, surf->db_depth_size);
		r600_pipe_state_add_reg(rstate, R_028004_DB_DEPTH_VIEW, surf->db_depth_view);
		r600_pipe_state_add_reg_bo(rstate, R_028010_DB_DEPTH_INFO, surf->db_depth_info,
					   res, RADEON_USAGE_READWRITE);
		r600_pipe_state_add_reg(rstate, R_028D34_DB_PREFETCH_LIMIT, surf->db_prefetch_limit);
	}

	/* Framebuffer dimensions. */
	tl = S_028240_TL_X(0) | S_028240_TL_Y(0) | S_028240_WINDOW_OFFSET_DISABLE(1);
	br = S_028244_BR_X(state->width) | S_028244_BR_Y(state->height);

	r600_pipe_state_add_reg(rstate,
				R_028204_PA_SC_WINDOW_SCISSOR_TL, tl);
	r600_pipe_state_add_reg(rstate,
				R_028208_PA_SC_WINDOW_SCISSOR_BR, br);

	/* If we're doing MSAA resolve... */
	if (is_resolve) {
		r600_pipe_state_add_reg(rstate, R_0287A0_CB_SHADER_CONTROL, 1);
	} else {
		/* Always enable the first colorbuffer in CB_SHADER_CONTROL. This
		 * will assure that the alpha-test will work even if there is
		 * no colorbuffer bound. */
		r600_pipe_state_add_reg(rstate, R_0287A0_CB_SHADER_CONTROL,
					(1ull << MAX2(state->nr_cbufs, 1)) - 1);
	}

	/* Multisampling */
	if (state->nr_cbufs)
		nr_samples = state->cbufs[0]->texture->nr_samples;
	else if (state->zsbuf)
		nr_samples = state->zsbuf->texture->nr_samples;
	else
		nr_samples = 0;

	max_dist = r600_set_ms_pos(ctx, rstate, nr_samples);

	if (nr_samples > 1) {
		unsigned log_samples = util_logbase2(nr_samples);

		r600_pipe_state_add_reg(rstate, R_028C00_PA_SC_LINE_CNTL,
					S_028C00_LAST_PIXEL(1) |
					S_028C00_EXPAND_LINE_WIDTH(1));
		r600_pipe_state_add_reg(rstate, R_028C04_PA_SC_AA_CONFIG,
					S_028C04_MSAA_NUM_SAMPLES(log_samples) |
					S_028C04_MAX_SAMPLE_DIST(max_dist));
	} else {
		r600_pipe_state_add_reg(rstate, R_028C00_PA_SC_LINE_CNTL, S_028C00_LAST_PIXEL(1));
		r600_pipe_state_add_reg(rstate, R_028C04_PA_SC_AA_CONFIG, 0);
	}

	free(rctx->states[R600_PIPE_STATE_FRAMEBUFFER]);
	rctx->states[R600_PIPE_STATE_FRAMEBUFFER] = rstate;
	r600_context_pipe_state_set(rctx, rstate);

	if (state->zsbuf) {
		r600_polygon_offset_update(rctx);
	}

	if (rctx->cb_misc_state.nr_cbufs != state->nr_cbufs) {
		rctx->cb_misc_state.nr_cbufs = state->nr_cbufs;
		r600_atom_dirty(rctx, &rctx->cb_misc_state.atom);
	}

	if (state->nr_cbufs == 0 && rctx->alphatest_state.bypass) {
		rctx->alphatest_state.bypass = false;
		r600_atom_dirty(rctx, &rctx->alphatest_state.atom);
	}
}

static void r600_emit_cb_misc_state(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	struct r600_cb_misc_state *a = (struct r600_cb_misc_state*)atom;

	if (G_028808_SPECIAL_OP(a->cb_color_control) == V_028808_SPECIAL_RESOLVE_BOX) {
		r600_write_context_reg_seq(cs, R_028238_CB_TARGET_MASK, 2);
		if (rctx->chip_class == R600) {
			r600_write_value(cs, 0xff); /* R_028238_CB_TARGET_MASK */
			r600_write_value(cs, 0xff); /* R_02823C_CB_SHADER_MASK */
		} else {
			r600_write_value(cs, 0xf); /* R_028238_CB_TARGET_MASK */
			r600_write_value(cs, 0xf); /* R_02823C_CB_SHADER_MASK */
		}
		r600_write_context_reg(cs, R_028808_CB_COLOR_CONTROL, a->cb_color_control);
	} else {
		unsigned fb_colormask = (1ULL << ((unsigned)a->nr_cbufs * 4)) - 1;
		unsigned ps_colormask = (1ULL << ((unsigned)a->nr_ps_color_outputs * 4)) - 1;
		unsigned multiwrite = a->multiwrite && a->nr_cbufs > 1;

		r600_write_context_reg_seq(cs, R_028238_CB_TARGET_MASK, 2);
		r600_write_value(cs, a->blend_colormask & fb_colormask); /* R_028238_CB_TARGET_MASK */
		/* Always enable the first color output to make sure alpha-test works even without one. */
		r600_write_value(cs, 0xf | (multiwrite ? fb_colormask : ps_colormask)); /* R_02823C_CB_SHADER_MASK */
		r600_write_context_reg(cs, R_028808_CB_COLOR_CONTROL,
				       a->cb_color_control |
				       S_028808_MULTIWRITE_ENABLE(multiwrite));
	}
}

static void r600_emit_db_misc_state(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	struct r600_db_misc_state *a = (struct r600_db_misc_state*)atom;
	unsigned db_render_control = 0;
	unsigned db_render_override =
		S_028D10_FORCE_HIZ_ENABLE(V_028D10_FORCE_DISABLE) |
		S_028D10_FORCE_HIS_ENABLE0(V_028D10_FORCE_DISABLE) |
		S_028D10_FORCE_HIS_ENABLE1(V_028D10_FORCE_DISABLE);

	if (a->occlusion_query_enabled) {
		if (rctx->chip_class >= R700) {
			db_render_control |= S_028D0C_R700_PERFECT_ZPASS_COUNTS(1);
		}
		db_render_override |= S_028D10_NOOP_CULL_DISABLE(1);
	}
	if (a->flush_depthstencil_through_cb) {
		assert(a->copy_depth || a->copy_stencil);

		db_render_control |= S_028D0C_DEPTH_COPY_ENABLE(a->copy_depth) |
				     S_028D0C_STENCIL_COPY_ENABLE(a->copy_stencil) |
				     S_028D0C_COPY_CENTROID(1) |
				     S_028D0C_COPY_SAMPLE(a->copy_sample);
	}

	r600_write_context_reg_seq(cs, R_028D0C_DB_RENDER_CONTROL, 2);
	r600_write_value(cs, db_render_control); /* R_028D0C_DB_RENDER_CONTROL */
	r600_write_value(cs, db_render_override); /* R_028D10_DB_RENDER_OVERRIDE */
}

static void r600_emit_vertex_buffers(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	uint32_t dirty_mask = rctx->vertex_buffer_state.dirty_mask;

	while (dirty_mask) {
		struct pipe_vertex_buffer *vb;
		struct r600_resource *rbuffer;
		unsigned offset;
		unsigned buffer_index = u_bit_scan(&dirty_mask);

		vb = &rctx->vertex_buffer_state.vb[buffer_index];
		rbuffer = (struct r600_resource*)vb->buffer;
		assert(rbuffer);

		offset = vb->buffer_offset;

		/* fetch resources start at index 320 */
		r600_write_value(cs, PKT3(PKT3_SET_RESOURCE, 7, 0));
		r600_write_value(cs, (320 + buffer_index) * 7);
		r600_write_value(cs, offset); /* RESOURCEi_WORD0 */
		r600_write_value(cs, rbuffer->buf->size - offset - 1); /* RESOURCEi_WORD1 */
		r600_write_value(cs, /* RESOURCEi_WORD2 */
				 S_038008_ENDIAN_SWAP(r600_endian_swap(32)) |
				 S_038008_STRIDE(vb->stride));
		r600_write_value(cs, 0); /* RESOURCEi_WORD3 */
		r600_write_value(cs, 0); /* RESOURCEi_WORD4 */
		r600_write_value(cs, 0); /* RESOURCEi_WORD5 */
		r600_write_value(cs, 0xc0000000); /* RESOURCEi_WORD6 */

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0));
		r600_write_value(cs, r600_context_bo_reloc(rctx, rbuffer, RADEON_USAGE_READ));
	}
}

static void r600_emit_constant_buffers(struct r600_context *rctx,
				       struct r600_constbuf_state *state,
				       unsigned buffer_id_base,
				       unsigned reg_alu_constbuf_size,
				       unsigned reg_alu_const_cache)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	uint32_t dirty_mask = state->dirty_mask;

	while (dirty_mask) {
		struct pipe_constant_buffer *cb;
		struct r600_resource *rbuffer;
		unsigned offset;
		unsigned buffer_index = ffs(dirty_mask) - 1;

		cb = &state->cb[buffer_index];
		rbuffer = (struct r600_resource*)cb->buffer;
		assert(rbuffer);

		offset = cb->buffer_offset;

		r600_write_context_reg(cs, reg_alu_constbuf_size + buffer_index * 4,
				       ALIGN_DIVUP(cb->buffer_size >> 4, 16));
		r600_write_context_reg(cs, reg_alu_const_cache + buffer_index * 4, offset >> 8);

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0));
		r600_write_value(cs, r600_context_bo_reloc(rctx, rbuffer, RADEON_USAGE_READ));

		r600_write_value(cs, PKT3(PKT3_SET_RESOURCE, 7, 0));
		r600_write_value(cs, (buffer_id_base + buffer_index) * 7);
		r600_write_value(cs, offset); /* RESOURCEi_WORD0 */
		r600_write_value(cs, rbuffer->buf->size - offset - 1); /* RESOURCEi_WORD1 */
		r600_write_value(cs, /* RESOURCEi_WORD2 */
				 S_038008_ENDIAN_SWAP(r600_endian_swap(32)) |
				 S_038008_STRIDE(16));
		r600_write_value(cs, 0); /* RESOURCEi_WORD3 */
		r600_write_value(cs, 0); /* RESOURCEi_WORD4 */
		r600_write_value(cs, 0); /* RESOURCEi_WORD5 */
		r600_write_value(cs, 0xc0000000); /* RESOURCEi_WORD6 */

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0));
		r600_write_value(cs, r600_context_bo_reloc(rctx, rbuffer, RADEON_USAGE_READ));

		dirty_mask &= ~(1 << buffer_index);
	}
	state->dirty_mask = 0;
}

static void r600_emit_vs_constant_buffers(struct r600_context *rctx, struct r600_atom *atom)
{
	r600_emit_constant_buffers(rctx, &rctx->vs_constbuf_state, 160,
				   R_028180_ALU_CONST_BUFFER_SIZE_VS_0,
				   R_028980_ALU_CONST_CACHE_VS_0);
}

static void r600_emit_ps_constant_buffers(struct r600_context *rctx, struct r600_atom *atom)
{
	r600_emit_constant_buffers(rctx, &rctx->ps_constbuf_state, 0,
				   R_028140_ALU_CONST_BUFFER_SIZE_PS_0,
				   R_028940_ALU_CONST_CACHE_PS_0);
}

static void r600_emit_sampler_views(struct r600_context *rctx,
				    struct r600_samplerview_state *state,
				    unsigned resource_id_base)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	uint32_t dirty_mask = state->dirty_mask;

	while (dirty_mask) {
		struct r600_pipe_sampler_view *rview;
		unsigned resource_index = u_bit_scan(&dirty_mask);
		unsigned reloc;

		rview = state->views[resource_index];
		assert(rview);

		r600_write_value(cs, PKT3(PKT3_SET_RESOURCE, 7, 0));
		r600_write_value(cs, (resource_id_base + resource_index) * 7);
		r600_write_array(cs, 7, rview->tex_resource_words);

		/* XXX The kernel needs two relocations. This is stupid. */
		reloc = r600_context_bo_reloc(rctx, rview->tex_resource,
					      RADEON_USAGE_READ);
		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0));
		r600_write_value(cs, reloc);
		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0));
		r600_write_value(cs, reloc);
	}
	state->dirty_mask = 0;
}

static void r600_emit_vs_sampler_views(struct r600_context *rctx, struct r600_atom *atom)
{
	r600_emit_sampler_views(rctx, &rctx->vs_samplers.views, 160 + R600_MAX_CONST_BUFFERS);
}

static void r600_emit_ps_sampler_views(struct r600_context *rctx, struct r600_atom *atom)
{
	r600_emit_sampler_views(rctx, &rctx->ps_samplers.views, R600_MAX_CONST_BUFFERS);
}

static void r600_emit_sampler(struct r600_context *rctx,
				struct r600_textures_info *texinfo,
				unsigned resource_id_base,
				unsigned border_color_reg)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	unsigned i;

	for (i = 0; i < texinfo->n_samplers; i++) {

		if (texinfo->samplers[i] == NULL) {
			continue;
		}

		/* TEX_ARRAY_OVERRIDE must be set for array textures to disable
		 * filtering between layers.
		 * Don't update TEX_ARRAY_OVERRIDE if we don't have the sampler view.
		 */
		if (texinfo->views.views[i]) {
			if (texinfo->views.views[i]->base.texture->target == PIPE_TEXTURE_1D_ARRAY ||
			    texinfo->views.views[i]->base.texture->target == PIPE_TEXTURE_2D_ARRAY) {
				texinfo->samplers[i]->tex_sampler_words[0] |= S_03C000_TEX_ARRAY_OVERRIDE(1);
				texinfo->is_array_sampler[i] = true;
			} else {
				texinfo->samplers[i]->tex_sampler_words[0] &= C_03C000_TEX_ARRAY_OVERRIDE;
				texinfo->is_array_sampler[i] = false;
			}
		}

		r600_write_value(cs, PKT3(PKT3_SET_SAMPLER, 3, 0));
		r600_write_value(cs, (resource_id_base + i) * 3);
		r600_write_array(cs, 3, texinfo->samplers[i]->tex_sampler_words);

		if (texinfo->samplers[i]->border_color_use) {
			unsigned offset;

			offset = border_color_reg;
			offset += i * 16;
			r600_write_config_reg_seq(cs, offset, 4);
			r600_write_array(cs, 4, texinfo->samplers[i]->border_color);
		}
	}
}

static void r600_emit_vs_sampler(struct r600_context *rctx, struct r600_atom *atom)
{
	r600_emit_sampler(rctx, &rctx->vs_samplers, 18, R_00A600_TD_VS_SAMPLER0_BORDER_RED);
}

static void r600_emit_ps_sampler(struct r600_context *rctx, struct r600_atom *atom)
{
	r600_emit_sampler(rctx, &rctx->ps_samplers, 0, R_00A400_TD_PS_SAMPLER0_BORDER_RED);
}

static void r600_emit_seamless_cube_map(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	unsigned tmp;

	tmp = S_009508_DISABLE_CUBE_ANISO(1) |
		S_009508_SYNC_GRADIENT(1) |
		S_009508_SYNC_WALKER(1) |
		S_009508_SYNC_ALIGNER(1);
	if (!rctx->seamless_cube_map.enabled) {
		tmp |= S_009508_DISABLE_CUBE_WRAP(1);
	}
	r600_write_config_reg(cs, R_009508_TA_CNTL_AUX, tmp);
}

static void r600_emit_sample_mask(struct r600_context *rctx, struct r600_atom *a)
{
	struct r600_sample_mask *s = (struct r600_sample_mask*)a;
	uint8_t mask = s->sample_mask;

	r600_write_context_reg(rctx->cs, R_028C48_PA_SC_AA_MASK,
			       mask | (mask << 8) | (mask << 16) | (mask << 24));
}

void r600_init_state_functions(struct r600_context *rctx)
{
	r600_init_atom(&rctx->seamless_cube_map.atom, r600_emit_seamless_cube_map, 3, 0);
	r600_atom_dirty(rctx, &rctx->seamless_cube_map.atom);
	r600_init_atom(&rctx->cb_misc_state.atom, r600_emit_cb_misc_state, 0, 0);
	r600_atom_dirty(rctx, &rctx->cb_misc_state.atom);
	r600_init_atom(&rctx->db_misc_state.atom, r600_emit_db_misc_state, 4, 0);
	r600_atom_dirty(rctx, &rctx->db_misc_state.atom);
	r600_init_atom(&rctx->vertex_buffer_state.atom, r600_emit_vertex_buffers, 0, 0);
	r600_init_atom(&rctx->vs_constbuf_state.atom, r600_emit_vs_constant_buffers, 0, 0);
	r600_init_atom(&rctx->ps_constbuf_state.atom, r600_emit_ps_constant_buffers, 0, 0);
	r600_init_atom(&rctx->vs_samplers.views.atom, r600_emit_vs_sampler_views, 0, 0);
	r600_init_atom(&rctx->ps_samplers.views.atom, r600_emit_ps_sampler_views, 0, 0);
	/* sampler must be emited before TA_CNTL_AUX otherwise DISABLE_CUBE_WRAP change
	 * does not take effect
	 */
	r600_init_atom(&rctx->vs_samplers.atom_sampler, r600_emit_vs_sampler, 0, EMIT_EARLY);
	r600_init_atom(&rctx->ps_samplers.atom_sampler, r600_emit_ps_sampler, 0, EMIT_EARLY);

	r600_init_atom(&rctx->sample_mask.atom, r600_emit_sample_mask, 3, 0);
	rctx->sample_mask.sample_mask = ~0;
	r600_atom_dirty(rctx, &rctx->sample_mask.atom);

	rctx->context.create_blend_state = r600_create_blend_state;
	rctx->context.create_depth_stencil_alpha_state = r600_create_dsa_state;
	rctx->context.create_fs_state = r600_create_shader_state_ps;
	rctx->context.create_rasterizer_state = r600_create_rs_state;
	rctx->context.create_sampler_state = r600_create_sampler_state;
	rctx->context.create_sampler_view = r600_create_sampler_view;
	rctx->context.create_vertex_elements_state = r600_create_vertex_elements;
	rctx->context.create_vs_state = r600_create_shader_state_vs;
	rctx->context.bind_blend_state = r600_bind_blend_state;
	rctx->context.bind_depth_stencil_alpha_state = r600_bind_dsa_state;
	rctx->context.bind_fragment_sampler_states = r600_bind_ps_samplers;
	rctx->context.bind_fs_state = r600_bind_ps_shader;
	rctx->context.bind_rasterizer_state = r600_bind_rs_state;
	rctx->context.bind_vertex_elements_state = r600_bind_vertex_elements;
	rctx->context.bind_vertex_sampler_states = r600_bind_vs_samplers;
	rctx->context.bind_vs_state = r600_bind_vs_shader;
	rctx->context.delete_blend_state = r600_delete_state;
	rctx->context.delete_depth_stencil_alpha_state = r600_delete_state;
	rctx->context.delete_fs_state = r600_delete_ps_shader;
	rctx->context.delete_rasterizer_state = r600_delete_rs_state;
	rctx->context.delete_sampler_state = r600_delete_sampler;
	rctx->context.delete_vertex_elements_state = r600_delete_vertex_element;
	rctx->context.delete_vs_state = r600_delete_vs_shader;
	rctx->context.set_blend_color = r600_set_blend_color;
	rctx->context.set_clip_state = r600_set_clip_state;
	rctx->context.set_constant_buffer = r600_set_constant_buffer;
	rctx->context.set_fragment_sampler_views = r600_set_ps_sampler_views;
	rctx->context.set_framebuffer_state = r600_set_framebuffer_state;
	rctx->context.set_polygon_stipple = r600_set_polygon_stipple;
	rctx->context.set_sample_mask = r600_set_sample_mask;
	rctx->context.set_scissor_state = r600_pipe_set_scissor_state;
	rctx->context.set_stencil_ref = r600_set_pipe_stencil_ref;
	rctx->context.set_vertex_buffers = r600_set_vertex_buffers;
	rctx->context.set_index_buffer = r600_set_index_buffer;
	rctx->context.set_vertex_sampler_views = r600_set_vs_sampler_views;
	rctx->context.set_viewport_state = r600_set_viewport_state;
	rctx->context.sampler_view_destroy = r600_sampler_view_destroy;
	rctx->context.texture_barrier = r600_texture_barrier;
	rctx->context.create_stream_output_target = r600_create_so_target;
	rctx->context.stream_output_target_destroy = r600_so_target_destroy;
	rctx->context.set_stream_output_targets = r600_set_so_targets;
}

/* Adjust GPR allocation on R6xx/R7xx */
void r600_adjust_gprs(struct r600_context *rctx)
{
	struct r600_pipe_state rstate;
	unsigned num_ps_gprs = rctx->default_ps_gprs;
	unsigned num_vs_gprs = rctx->default_vs_gprs;
	unsigned tmp;
	int diff;

	/* XXX: Following call moved from r600_bind_[ps|vs]_shader,
	 * it seems eg+ doesn't need it, r6xx/7xx probably need it only for
	 * adjusting the GPR allocation?
	 * Do we need this if we aren't really changing config below? */
	r600_inval_shader_cache(rctx);

	if (rctx->ps_shader->current->shader.bc.ngpr > rctx->default_ps_gprs)
	{
		diff = rctx->ps_shader->current->shader.bc.ngpr - rctx->default_ps_gprs;
		num_vs_gprs -= diff;
		num_ps_gprs += diff;
	}

	if (rctx->vs_shader->current->shader.bc.ngpr > rctx->default_vs_gprs)
	{
		diff = rctx->vs_shader->current->shader.bc.ngpr - rctx->default_vs_gprs;
		num_ps_gprs -= diff;
		num_vs_gprs += diff;
	}

	tmp = 0;
	tmp |= S_008C04_NUM_PS_GPRS(num_ps_gprs);
	tmp |= S_008C04_NUM_VS_GPRS(num_vs_gprs);
	tmp |= S_008C04_NUM_CLAUSE_TEMP_GPRS(rctx->r6xx_num_clause_temp_gprs);
	rstate.nregs = 0;
	r600_pipe_state_add_reg(&rstate, R_008C04_SQ_GPR_RESOURCE_MGMT_1, tmp);

	r600_context_pipe_state_set(rctx, &rstate);
}

void r600_init_atom_start_cs(struct r600_context *rctx)
{
	int ps_prio;
	int vs_prio;
	int gs_prio;
	int es_prio;
	int num_ps_gprs;
	int num_vs_gprs;
	int num_gs_gprs;
	int num_es_gprs;
	int num_temp_gprs;
	int num_ps_threads;
	int num_vs_threads;
	int num_gs_threads;
	int num_es_threads;
	int num_ps_stack_entries;
	int num_vs_stack_entries;
	int num_gs_stack_entries;
	int num_es_stack_entries;
	enum radeon_family family;
	struct r600_command_buffer *cb = &rctx->start_cs_cmd;
	uint32_t tmp;

	r600_init_command_buffer(cb, 256, EMIT_EARLY);

	/* R6xx requires this packet at the start of each command buffer */
	if (rctx->chip_class == R600) {
		r600_store_value(cb, PKT3(PKT3_START_3D_CMDBUF, 0, 0));
		r600_store_value(cb, 0);
	}
	/* All asics require this one */
	r600_store_value(cb, PKT3(PKT3_CONTEXT_CONTROL, 1, 0));
	r600_store_value(cb, 0x80000000);
	r600_store_value(cb, 0x80000000);

	family = rctx->family;
	ps_prio = 0;
	vs_prio = 1;
	gs_prio = 2;
	es_prio = 3;
	switch (family) {
	case CHIP_R600:
		num_ps_gprs = 192;
		num_vs_gprs = 56;
		num_temp_gprs = 4;
		num_gs_gprs = 0;
		num_es_gprs = 0;
		num_ps_threads = 136;
		num_vs_threads = 48;
		num_gs_threads = 4;
		num_es_threads = 4;
		num_ps_stack_entries = 128;
		num_vs_stack_entries = 128;
		num_gs_stack_entries = 0;
		num_es_stack_entries = 0;
		break;
	case CHIP_RV630:
	case CHIP_RV635:
		num_ps_gprs = 84;
		num_vs_gprs = 36;
		num_temp_gprs = 4;
		num_gs_gprs = 0;
		num_es_gprs = 0;
		num_ps_threads = 144;
		num_vs_threads = 40;
		num_gs_threads = 4;
		num_es_threads = 4;
		num_ps_stack_entries = 40;
		num_vs_stack_entries = 40;
		num_gs_stack_entries = 32;
		num_es_stack_entries = 16;
		break;
	case CHIP_RV610:
	case CHIP_RV620:
	case CHIP_RS780:
	case CHIP_RS880:
	default:
		num_ps_gprs = 84;
		num_vs_gprs = 36;
		num_temp_gprs = 4;
		num_gs_gprs = 0;
		num_es_gprs = 0;
		num_ps_threads = 136;
		num_vs_threads = 48;
		num_gs_threads = 4;
		num_es_threads = 4;
		num_ps_stack_entries = 40;
		num_vs_stack_entries = 40;
		num_gs_stack_entries = 32;
		num_es_stack_entries = 16;
		break;
	case CHIP_RV670:
		num_ps_gprs = 144;
		num_vs_gprs = 40;
		num_temp_gprs = 4;
		num_gs_gprs = 0;
		num_es_gprs = 0;
		num_ps_threads = 136;
		num_vs_threads = 48;
		num_gs_threads = 4;
		num_es_threads = 4;
		num_ps_stack_entries = 40;
		num_vs_stack_entries = 40;
		num_gs_stack_entries = 32;
		num_es_stack_entries = 16;
		break;
	case CHIP_RV770:
		num_ps_gprs = 192;
		num_vs_gprs = 56;
		num_temp_gprs = 4;
		num_gs_gprs = 0;
		num_es_gprs = 0;
		num_ps_threads = 188;
		num_vs_threads = 60;
		num_gs_threads = 0;
		num_es_threads = 0;
		num_ps_stack_entries = 256;
		num_vs_stack_entries = 256;
		num_gs_stack_entries = 0;
		num_es_stack_entries = 0;
		break;
	case CHIP_RV730:
	case CHIP_RV740:
		num_ps_gprs = 84;
		num_vs_gprs = 36;
		num_temp_gprs = 4;
		num_gs_gprs = 0;
		num_es_gprs = 0;
		num_ps_threads = 188;
		num_vs_threads = 60;
		num_gs_threads = 0;
		num_es_threads = 0;
		num_ps_stack_entries = 128;
		num_vs_stack_entries = 128;
		num_gs_stack_entries = 0;
		num_es_stack_entries = 0;
		break;
	case CHIP_RV710:
		num_ps_gprs = 192;
		num_vs_gprs = 56;
		num_temp_gprs = 4;
		num_gs_gprs = 0;
		num_es_gprs = 0;
		num_ps_threads = 144;
		num_vs_threads = 48;
		num_gs_threads = 0;
		num_es_threads = 0;
		num_ps_stack_entries = 128;
		num_vs_stack_entries = 128;
		num_gs_stack_entries = 0;
		num_es_stack_entries = 0;
		break;
	}

	rctx->default_ps_gprs = num_ps_gprs;
	rctx->default_vs_gprs = num_vs_gprs;
	rctx->r6xx_num_clause_temp_gprs = num_temp_gprs;

	/* SQ_CONFIG */
	tmp = 0;
	switch (family) {
	case CHIP_RV610:
	case CHIP_RV620:
	case CHIP_RS780:
	case CHIP_RS880:
	case CHIP_RV710:
		break;
	default:
		tmp |= S_008C00_VC_ENABLE(1);
		break;
	}
	tmp |= S_008C00_DX9_CONSTS(0);
	tmp |= S_008C00_ALU_INST_PREFER_VECTOR(1);
	tmp |= S_008C00_PS_PRIO(ps_prio);
	tmp |= S_008C00_VS_PRIO(vs_prio);
	tmp |= S_008C00_GS_PRIO(gs_prio);
	tmp |= S_008C00_ES_PRIO(es_prio);
	r600_store_config_reg(cb, R_008C00_SQ_CONFIG, tmp);

	/* SQ_GPR_RESOURCE_MGMT_2 */
	tmp = S_008C08_NUM_GS_GPRS(num_gs_gprs);
	tmp |= S_008C08_NUM_ES_GPRS(num_es_gprs);
	r600_store_config_reg_seq(cb, R_008C08_SQ_GPR_RESOURCE_MGMT_2, 4);
	r600_store_value(cb, tmp);

	/* SQ_THREAD_RESOURCE_MGMT */
	tmp = S_008C0C_NUM_PS_THREADS(num_ps_threads);
	tmp |= S_008C0C_NUM_VS_THREADS(num_vs_threads);
	tmp |= S_008C0C_NUM_GS_THREADS(num_gs_threads);
	tmp |= S_008C0C_NUM_ES_THREADS(num_es_threads);
	r600_store_value(cb, tmp); /* R_008C0C_SQ_THREAD_RESOURCE_MGMT */

	/* SQ_STACK_RESOURCE_MGMT_1 */
	tmp = S_008C10_NUM_PS_STACK_ENTRIES(num_ps_stack_entries);
	tmp |= S_008C10_NUM_VS_STACK_ENTRIES(num_vs_stack_entries);
	r600_store_value(cb, tmp); /* R_008C10_SQ_STACK_RESOURCE_MGMT_1 */

	/* SQ_STACK_RESOURCE_MGMT_2 */
	tmp = S_008C14_NUM_GS_STACK_ENTRIES(num_gs_stack_entries);
	tmp |= S_008C14_NUM_ES_STACK_ENTRIES(num_es_stack_entries);
	r600_store_value(cb, tmp); /* R_008C14_SQ_STACK_RESOURCE_MGMT_2 */

	r600_store_config_reg(cb, R_009714_VC_ENHANCE, 0);

	if (rctx->chip_class >= R700) {
		r600_store_config_reg(cb, R_008D8C_SQ_DYN_GPR_CNTL_PS_FLUSH_REQ, 0x00004000);
		r600_store_config_reg(cb, R_009830_DB_DEBUG, 0);
		r600_store_config_reg(cb, R_009838_DB_WATERMARKS, 0x00420204);
		r600_store_context_reg(cb, R_0286C8_SPI_THREAD_GROUPING, 0);
	} else {
		r600_store_config_reg(cb, R_008D8C_SQ_DYN_GPR_CNTL_PS_FLUSH_REQ, 0);
		r600_store_config_reg(cb, R_009830_DB_DEBUG, 0x82000000);
		r600_store_config_reg(cb, R_009838_DB_WATERMARKS, 0x01020204);
		r600_store_context_reg(cb, R_0286C8_SPI_THREAD_GROUPING, 1);
	}
	r600_store_context_reg_seq(cb, R_0288A8_SQ_ESGS_RING_ITEMSIZE, 9);
	r600_store_value(cb, 0); /* R_0288A8_SQ_ESGS_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_0288AC_SQ_GSVS_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_0288B0_SQ_ESTMP_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_0288B4_SQ_GSTMP_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_0288B8_SQ_VSTMP_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_0288BC_SQ_PSTMP_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_0288C0_SQ_FBUF_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_0288C4_SQ_REDUC_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_0288C8_SQ_GS_VERT_ITEMSIZE */

	r600_store_context_reg_seq(cb, R_028A10_VGT_OUTPUT_PATH_CNTL, 13);
	r600_store_value(cb, 0); /* R_028A10_VGT_OUTPUT_PATH_CNTL */
	r600_store_value(cb, 0); /* R_028A14_VGT_HOS_CNTL */
	r600_store_value(cb, 0); /* R_028A18_VGT_HOS_MAX_TESS_LEVEL */
	r600_store_value(cb, 0); /* R_028A1C_VGT_HOS_MIN_TESS_LEVEL */
	r600_store_value(cb, 0); /* R_028A20_VGT_HOS_REUSE_DEPTH */
	r600_store_value(cb, 0); /* R_028A24_VGT_GROUP_PRIM_TYPE */
	r600_store_value(cb, 0); /* R_028A28_VGT_GROUP_FIRST_DECR */
	r600_store_value(cb, 0); /* R_028A2C_VGT_GROUP_DECR */
	r600_store_value(cb, 0); /* R_028A30_VGT_GROUP_VECT_0_CNTL */
	r600_store_value(cb, 0); /* R_028A34_VGT_GROUP_VECT_1_CNTL */
	r600_store_value(cb, 0); /* R_028A38_VGT_GROUP_VECT_0_FMT_CNTL */
	r600_store_value(cb, 0); /* R_028A3C_VGT_GROUP_VECT_1_FMT_CNTL */
	r600_store_value(cb, 0); /* R_028A40_VGT_GS_MODE, 0); */

	r600_store_context_reg(cb, R_028A84_VGT_PRIMITIVEID_EN, 0);
	r600_store_context_reg(cb, R_028AA0_VGT_INSTANCE_STEP_RATE_0, 0);
	r600_store_context_reg(cb, R_028AA4_VGT_INSTANCE_STEP_RATE_1, 0);

	r600_store_context_reg_seq(cb, R_028AB0_VGT_STRMOUT_EN, 3);
	r600_store_value(cb, 0); /* R_028AB0_VGT_STRMOUT_EN */
	r600_store_value(cb, 1); /* R_028AB4_VGT_REUSE_OFF */
	r600_store_value(cb, 0); /* R_028AB8_VGT_VTX_CNT_EN */

	r600_store_context_reg(cb, R_028B20_VGT_STRMOUT_BUFFER_EN, 0);

	r600_store_context_reg_seq(cb, R_028400_VGT_MAX_VTX_INDX, 2);
	r600_store_value(cb, ~0); /* R_028400_VGT_MAX_VTX_INDX */
	r600_store_value(cb, 0); /* R_028404_VGT_MIN_VTX_INDX */

	r600_store_ctl_const(cb, R_03CFF0_SQ_VTX_BASE_VTX_LOC, 0);

	r600_store_context_reg_seq(cb, R_028028_DB_STENCIL_CLEAR, 2);
	r600_store_value(cb, 0); /* R_028028_DB_STENCIL_CLEAR */
	r600_store_value(cb, 0x3F800000); /* R_02802C_DB_DEPTH_CLEAR */

	r600_store_context_reg_seq(cb, R_0286DC_SPI_FOG_CNTL, 3);
	r600_store_value(cb, 0); /* R_0286DC_SPI_FOG_CNTL */
	r600_store_value(cb, 0); /* R_0286E0_SPI_FOG_FUNC_SCALE */
	r600_store_value(cb, 0); /* R_0286E4_SPI_FOG_FUNC_BIAS */

	r600_store_context_reg_seq(cb, R_028D2C_DB_SRESULTS_COMPARE_STATE1, 2);
	r600_store_value(cb, 0); /* R_028D2C_DB_SRESULTS_COMPARE_STATE1 */
	r600_store_value(cb, 0); /* R_028D30_DB_PRELOAD_CONTROL */

	r600_store_context_reg(cb, R_028820_PA_CL_NANINF_CNTL, 0);
	r600_store_context_reg(cb, R_028A48_PA_SC_MPASS_PS_CNTL, 0);

	r600_store_context_reg_seq(cb, R_028C0C_PA_CL_GB_VERT_CLIP_ADJ, 4);
	r600_store_value(cb, 0x3F800000); /* R_028C0C_PA_CL_GB_VERT_CLIP_ADJ */
	r600_store_value(cb, 0x3F800000); /* R_028C10_PA_CL_GB_VERT_DISC_ADJ */
	r600_store_value(cb, 0x3F800000); /* R_028C14_PA_CL_GB_HORZ_CLIP_ADJ */
	r600_store_value(cb, 0x3F800000); /* R_028C18_PA_CL_GB_HORZ_DISC_ADJ */

	r600_store_context_reg_seq(cb, R_0282D0_PA_SC_VPORT_ZMIN_0, 2);
	r600_store_value(cb, 0); /* R_0282D0_PA_SC_VPORT_ZMIN_0 */
	r600_store_value(cb, 0x3F800000); /* R_0282D4_PA_SC_VPORT_ZMAX_0 */

	r600_store_context_reg(cb, R_028818_PA_CL_VTE_CNTL, 0x43F);

	r600_store_context_reg(cb, R_028200_PA_SC_WINDOW_OFFSET, 0);
	r600_store_context_reg(cb, R_02820C_PA_SC_CLIPRECT_RULE, 0xFFFF);

	if (rctx->chip_class >= R700) {
		r600_store_context_reg(cb, R_028230_PA_SC_EDGERULE, 0xAAAAAAAA);
	}

	r600_store_context_reg_seq(cb, R_028C30_CB_CLRCMP_CONTROL, 4);
	r600_store_value(cb, 0x1000000);  /* R_028C30_CB_CLRCMP_CONTROL */
	r600_store_value(cb, 0);          /* R_028C34_CB_CLRCMP_SRC */
	r600_store_value(cb, 0xFF);       /* R_028C38_CB_CLRCMP_DST */
	r600_store_value(cb, 0xFFFFFFFF); /* R_028C3C_CB_CLRCMP_MSK */

	r600_store_context_reg_seq(cb, R_028030_PA_SC_SCREEN_SCISSOR_TL, 2);
	r600_store_value(cb, 0); /* R_028030_PA_SC_SCREEN_SCISSOR_TL */
	r600_store_value(cb, S_028034_BR_X(8192) | S_028034_BR_Y(8192)); /* R_028034_PA_SC_SCREEN_SCISSOR_BR */

	r600_store_context_reg_seq(cb, R_028240_PA_SC_GENERIC_SCISSOR_TL, 2);
	r600_store_value(cb, 0); /* R_028240_PA_SC_GENERIC_SCISSOR_TL */
	r600_store_value(cb, S_028244_BR_X(8192) | S_028244_BR_Y(8192)); /* R_028244_PA_SC_GENERIC_SCISSOR_BR */

	r600_store_context_reg_seq(cb, R_0288CC_SQ_PGM_CF_OFFSET_PS, 2);
	r600_store_value(cb, 0); /* R_0288CC_SQ_PGM_CF_OFFSET_PS */
	r600_store_value(cb, 0); /* R_0288D0_SQ_PGM_CF_OFFSET_VS */

	r600_store_context_reg(cb, R_0288A4_SQ_PGM_RESOURCES_FS, 0);
	r600_store_context_reg(cb, R_0288DC_SQ_PGM_CF_OFFSET_FS, 0);

	if (rctx->chip_class == R700 && rctx->screen->has_streamout)
		r600_store_context_reg(cb, R_028354_SX_SURFACE_SYNC, S_028354_SURFACE_SYNC_MASK(0xf));
	r600_store_context_reg(cb, R_028800_DB_DEPTH_CONTROL, 0);
	if (rctx->screen->has_streamout) {
		r600_store_context_reg(cb, R_028B28_VGT_STRMOUT_DRAW_OPAQUE_OFFSET, 0);
	}

	r600_store_loop_const(cb, R_03E200_SQ_LOOP_CONST_0, 0x1000FFF);
	r600_store_loop_const(cb, R_03E200_SQ_LOOP_CONST_0 + (32 * 4), 0x1000FFF);
}

void r600_pipe_shader_ps(struct pipe_context *ctx, struct r600_pipe_shader *shader)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_state *rstate = &shader->rstate;
	struct r600_shader *rshader = &shader->shader;
	unsigned i, exports_ps, num_cout, spi_ps_in_control_0, spi_input_z, spi_ps_in_control_1, db_shader_control;
	int pos_index = -1, face_index = -1;
	unsigned tmp, sid, ufi = 0;
	int need_linear = 0;
	unsigned z_export = 0, stencil_export = 0;

	rstate->nregs = 0;

	for (i = 0; i < rshader->ninput; i++) {
		if (rshader->input[i].name == TGSI_SEMANTIC_POSITION)
			pos_index = i;
		if (rshader->input[i].name == TGSI_SEMANTIC_FACE)
			face_index = i;

		sid = rshader->input[i].spi_sid;

		tmp = S_028644_SEMANTIC(sid);

		if (rshader->input[i].name == TGSI_SEMANTIC_POSITION ||
			rshader->input[i].interpolate == TGSI_INTERPOLATE_CONSTANT ||
			(rshader->input[i].interpolate == TGSI_INTERPOLATE_COLOR &&
				rctx->rasterizer && rctx->rasterizer->flatshade))
			tmp |= S_028644_FLAT_SHADE(1);

		if (rshader->input[i].name == TGSI_SEMANTIC_GENERIC &&
				rctx->sprite_coord_enable & (1 << rshader->input[i].sid)) {
			tmp |= S_028644_PT_SPRITE_TEX(1);
		}

		if (rshader->input[i].centroid)
			tmp |= S_028644_SEL_CENTROID(1);

		if (rshader->input[i].interpolate == TGSI_INTERPOLATE_LINEAR) {
			need_linear = 1;
			tmp |= S_028644_SEL_LINEAR(1);
		}

		r600_pipe_state_add_reg(rstate, R_028644_SPI_PS_INPUT_CNTL_0 + i * 4,
				tmp);
	}

	db_shader_control = S_02880C_Z_ORDER(V_02880C_EARLY_Z_THEN_LATE_Z);
	for (i = 0; i < rshader->noutput; i++) {
		if (rshader->output[i].name == TGSI_SEMANTIC_POSITION)
			z_export = 1;
		if (rshader->output[i].name == TGSI_SEMANTIC_STENCIL)
			stencil_export = 1;
	}
	db_shader_control |= S_02880C_Z_EXPORT_ENABLE(z_export);
	db_shader_control |= S_02880C_STENCIL_REF_EXPORT_ENABLE(stencil_export);
	if (rshader->uses_kill)
		db_shader_control |= S_02880C_KILL_ENABLE(1);

	exports_ps = 0;
	for (i = 0; i < rshader->noutput; i++) {
		if (rshader->output[i].name == TGSI_SEMANTIC_POSITION ||
		    rshader->output[i].name == TGSI_SEMANTIC_STENCIL) {
			exports_ps |= 1;
		}
	}
	num_cout = rshader->nr_ps_color_exports;
	exports_ps |= S_028854_EXPORT_COLORS(num_cout);
	if (!exports_ps) {
		/* always at least export 1 component per pixel */
		exports_ps = 2;
	}

	shader->nr_ps_color_outputs = num_cout;

	spi_ps_in_control_0 = S_0286CC_NUM_INTERP(rshader->ninput) |
				S_0286CC_PERSP_GRADIENT_ENA(1)|
				S_0286CC_LINEAR_GRADIENT_ENA(need_linear);
	spi_input_z = 0;
	if (pos_index != -1) {
		spi_ps_in_control_0 |= (S_0286CC_POSITION_ENA(1) |
					S_0286CC_POSITION_CENTROID(rshader->input[pos_index].centroid) |
					S_0286CC_POSITION_ADDR(rshader->input[pos_index].gpr) |
					S_0286CC_BARYC_SAMPLE_CNTL(1));
		spi_input_z |= 1;
	}

	spi_ps_in_control_1 = 0;
	if (face_index != -1) {
		spi_ps_in_control_1 |= S_0286D0_FRONT_FACE_ENA(1) |
			S_0286D0_FRONT_FACE_ADDR(rshader->input[face_index].gpr);
	}

	/* HW bug in original R600 */
	if (rctx->family == CHIP_R600)
		ufi = 1;

	r600_pipe_state_add_reg(rstate, R_0286CC_SPI_PS_IN_CONTROL_0, spi_ps_in_control_0);
	r600_pipe_state_add_reg(rstate, R_0286D0_SPI_PS_IN_CONTROL_1, spi_ps_in_control_1);
	r600_pipe_state_add_reg(rstate, R_0286D8_SPI_INPUT_Z, spi_input_z);
	r600_pipe_state_add_reg_bo(rstate,
				   R_028840_SQ_PGM_START_PS,
				   0, shader->bo, RADEON_USAGE_READ);
	r600_pipe_state_add_reg(rstate,
				R_028850_SQ_PGM_RESOURCES_PS,
				S_028850_NUM_GPRS(rshader->bc.ngpr) |
				S_028850_STACK_SIZE(rshader->bc.nstack) |
				S_028850_UNCACHED_FIRST_INST(ufi));
	r600_pipe_state_add_reg(rstate,
				R_028854_SQ_PGM_EXPORTS_PS,
				exports_ps);
	/* only set some bits here, the other bits are set in the dsa state */
	shader->db_shader_control = db_shader_control;
	shader->ps_depth_export = z_export | stencil_export;

	shader->sprite_coord_enable = rctx->sprite_coord_enable;
	if (rctx->rasterizer)
		shader->flatshade = rctx->rasterizer->flatshade;
}

void r600_pipe_shader_vs(struct pipe_context *ctx, struct r600_pipe_shader *shader)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_state *rstate = &shader->rstate;
	struct r600_shader *rshader = &shader->shader;
	unsigned spi_vs_out_id[10] = {};
	unsigned i, tmp, nparams = 0;

	/* clear previous register */
	rstate->nregs = 0;

	for (i = 0; i < rshader->noutput; i++) {
		if (rshader->output[i].spi_sid) {
			tmp = rshader->output[i].spi_sid << ((nparams & 3) * 8);
			spi_vs_out_id[nparams / 4] |= tmp;
			nparams++;
		}
	}

	for (i = 0; i < 10; i++) {
		r600_pipe_state_add_reg(rstate,
					R_028614_SPI_VS_OUT_ID_0 + i * 4,
					spi_vs_out_id[i]);
	}

	/* Certain attributes (position, psize, etc.) don't count as params.
	 * VS is required to export at least one param and r600_shader_from_tgsi()
	 * takes care of adding a dummy export.
	 */
	if (nparams < 1)
		nparams = 1;

	r600_pipe_state_add_reg(rstate,
				R_0286C4_SPI_VS_OUT_CONFIG,
				S_0286C4_VS_EXPORT_COUNT(nparams - 1));
	r600_pipe_state_add_reg(rstate,
				R_028868_SQ_PGM_RESOURCES_VS,
				S_028868_NUM_GPRS(rshader->bc.ngpr) |
				S_028868_STACK_SIZE(rshader->bc.nstack));
	r600_pipe_state_add_reg_bo(rstate,
			R_028858_SQ_PGM_START_VS,
			0, shader->bo, RADEON_USAGE_READ);

	shader->pa_cl_vs_out_cntl =
		S_02881C_VS_OUT_CCDIST0_VEC_ENA((rshader->clip_dist_write & 0x0F) != 0) |
		S_02881C_VS_OUT_CCDIST1_VEC_ENA((rshader->clip_dist_write & 0xF0) != 0) |
		S_02881C_VS_OUT_MISC_VEC_ENA(rshader->vs_out_misc_write) |
		S_02881C_USE_VTX_POINT_SIZE(rshader->vs_out_point_size);
}

void r600_fetch_shader(struct pipe_context *ctx,
		       struct r600_vertex_element *ve)
{
	struct r600_pipe_state *rstate;
	struct r600_context *rctx = (struct r600_context *)ctx;

	rstate = &ve->rstate;
	rstate->id = R600_PIPE_STATE_FETCH_SHADER;
	rstate->nregs = 0;
	r600_pipe_state_add_reg_bo(rstate, R_028894_SQ_PGM_START_FS,
				0,
				ve->fetch_shader, RADEON_USAGE_READ);
}

void *r600_create_resolve_blend(struct r600_context *rctx)
{
	struct pipe_blend_state blend;
	struct r600_pipe_state *rstate;
	unsigned i;

	memset(&blend, 0, sizeof(blend));
	blend.independent_blend_enable = true;
	for (i = 0; i < 2; i++) {
		blend.rt[i].colormask = 0xf;
		blend.rt[i].blend_enable = 1;
		blend.rt[i].rgb_func = PIPE_BLEND_ADD;
		blend.rt[i].alpha_func = PIPE_BLEND_ADD;
		blend.rt[i].rgb_src_factor = PIPE_BLENDFACTOR_ZERO;
		blend.rt[i].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
		blend.rt[i].alpha_src_factor = PIPE_BLENDFACTOR_ZERO;
		blend.rt[i].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
	}
	rstate = r600_create_blend_state_mode(&rctx->context, &blend, V_028808_SPECIAL_RESOLVE_BOX);
	return rstate;
}

void *r700_create_resolve_blend(struct r600_context *rctx)
{
	struct pipe_blend_state blend;
	struct r600_pipe_state *rstate;

	memset(&blend, 0, sizeof(blend));
	blend.independent_blend_enable = true;
	blend.rt[0].colormask = 0xf;
	rstate = r600_create_blend_state_mode(&rctx->context, &blend, V_028808_SPECIAL_RESOLVE_BOX);
	return rstate;
}

void *r600_create_decompress_blend(struct r600_context *rctx)
{
	struct pipe_blend_state blend;
	struct r600_pipe_state *rstate;

	memset(&blend, 0, sizeof(blend));
	blend.independent_blend_enable = true;
	blend.rt[0].colormask = 0xf;
	rstate = r600_create_blend_state_mode(&rctx->context, &blend, V_028808_SPECIAL_EXPAND_SAMPLES);
	return rstate;
}

void *r600_create_db_flush_dsa(struct r600_context *rctx)
{
	struct pipe_depth_stencil_alpha_state dsa;
	boolean quirk = false;

	if (rctx->family == CHIP_RV610 || rctx->family == CHIP_RV630 ||
		rctx->family == CHIP_RV620 || rctx->family == CHIP_RV635)
		quirk = true;

	memset(&dsa, 0, sizeof(dsa));

	if (quirk) {
		dsa.depth.enabled = 1;
		dsa.depth.func = PIPE_FUNC_LEQUAL;
		dsa.stencil[0].enabled = 1;
		dsa.stencil[0].func = PIPE_FUNC_ALWAYS;
		dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_KEEP;
		dsa.stencil[0].zfail_op = PIPE_STENCIL_OP_INCR;
		dsa.stencil[0].writemask = 0xff;
	}

	return rctx->context.create_depth_stencil_alpha_state(&rctx->context, &dsa);
}

void r600_update_dual_export_state(struct r600_context * rctx)
{
	unsigned dual_export = rctx->export_16bpc && rctx->nr_cbufs &&
			       !rctx->ps_shader->current->ps_depth_export;
	unsigned db_shader_control = rctx->ps_shader->current->db_shader_control |
				     S_02880C_DUAL_EXPORT_ENABLE(dual_export);

	if (db_shader_control != rctx->db_shader_control) {
		struct r600_pipe_state rstate;

		rctx->db_shader_control = db_shader_control;
		rstate.nregs = 0;
		r600_pipe_state_add_reg(&rstate, R_02880C_DB_SHADER_CONTROL, db_shader_control);
		r600_context_pipe_state_set(rctx, &rstate);
	}
}
