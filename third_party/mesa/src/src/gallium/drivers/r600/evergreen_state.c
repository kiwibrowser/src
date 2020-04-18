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
#include "evergreend.h"

#include "pipe/p_shader_tokens.h"
#include "util/u_pack_color.h"
#include "util/u_memory.h"
#include "util/u_framebuffer.h"
#include "util/u_dual_blend.h"
#include "evergreen_compute.h"

static uint32_t eg_num_banks(uint32_t nbanks)
{
	switch (nbanks) {
	case 2:
		return 0;
	case 4:
		return 1;
	case 8:
	default:
		return 2;
	case 16:
		return 3;
	}
}


static unsigned eg_tile_split(unsigned tile_split)
{
	switch (tile_split) {
	case 64:	tile_split = 0;	break;
	case 128:	tile_split = 1;	break;
	case 256:	tile_split = 2;	break;
	case 512:	tile_split = 3;	break;
	default:
	case 1024:	tile_split = 4;	break;
	case 2048:	tile_split = 5;	break;
	case 4096:	tile_split = 6;	break;
	}
	return tile_split;
}

static unsigned eg_macro_tile_aspect(unsigned macro_tile_aspect)
{
	switch (macro_tile_aspect) {
	default:
	case 1:	macro_tile_aspect = 0;	break;
	case 2:	macro_tile_aspect = 1;	break;
	case 4:	macro_tile_aspect = 2;	break;
	case 8:	macro_tile_aspect = 3;	break;
	}
	return macro_tile_aspect;
}

static unsigned eg_bank_wh(unsigned bankwh)
{
	switch (bankwh) {
	default:
	case 1:	bankwh = 0;	break;
	case 2:	bankwh = 1;	break;
	case 4:	bankwh = 2;	break;
	case 8:	bankwh = 3;	break;
	}
	return bankwh;
}

static uint32_t r600_translate_blend_function(int blend_func)
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

static uint32_t r600_translate_blend_factor(int blend_fact)
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
		return V_028780_BLEND_CONST_COLOR;
	case PIPE_BLENDFACTOR_CONST_ALPHA:
		return V_028780_BLEND_CONST_ALPHA;
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
		return V_028780_BLEND_ONE_MINUS_CONST_COLOR;
	case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
		return V_028780_BLEND_ONE_MINUS_CONST_ALPHA;
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

static unsigned r600_tex_dim(unsigned dim, unsigned nr_samples)
{
	switch (dim) {
	default:
	case PIPE_TEXTURE_1D:
		return V_030000_SQ_TEX_DIM_1D;
	case PIPE_TEXTURE_1D_ARRAY:
		return V_030000_SQ_TEX_DIM_1D_ARRAY;
	case PIPE_TEXTURE_2D:
	case PIPE_TEXTURE_RECT:
		return nr_samples > 1 ? V_030000_SQ_TEX_DIM_2D_MSAA :
					V_030000_SQ_TEX_DIM_2D;
	case PIPE_TEXTURE_2D_ARRAY:
		return nr_samples > 1 ? V_030000_SQ_TEX_DIM_2D_ARRAY_MSAA :
					V_030000_SQ_TEX_DIM_2D_ARRAY;
	case PIPE_TEXTURE_3D:
		return V_030000_SQ_TEX_DIM_3D;
	case PIPE_TEXTURE_CUBE:
		return V_030000_SQ_TEX_DIM_CUBEMAP;
	}
}

static uint32_t r600_translate_dbformat(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_Z16_UNORM:
		return V_028040_Z_16;
	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		return V_028040_Z_24;
	case PIPE_FORMAT_Z32_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
		return V_028040_Z_32_FLOAT;
	default:
		return ~0U;
	}
}

static uint32_t r600_translate_colorswap(enum pipe_format format)
{
	switch (format) {
	/* 8-bit buffers. */
	case PIPE_FORMAT_L4A4_UNORM:
	case PIPE_FORMAT_A4R4_UNORM:
		return V_028C70_SWAP_ALT;

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
		return V_028C70_SWAP_ALT_REV;
	case PIPE_FORMAT_I8_UNORM:
	case PIPE_FORMAT_I8_SNORM:
	case PIPE_FORMAT_I8_UINT:
	case PIPE_FORMAT_I8_SINT:
	case PIPE_FORMAT_I16_UNORM:
	case PIPE_FORMAT_I16_SNORM:
	case PIPE_FORMAT_I16_UINT:
	case PIPE_FORMAT_I16_SINT:
	case PIPE_FORMAT_I16_FLOAT:
	case PIPE_FORMAT_I32_UINT:
	case PIPE_FORMAT_I32_SINT:
	case PIPE_FORMAT_I32_FLOAT:
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
		return V_028C70_SWAP_ALT;
	case PIPE_FORMAT_R8G8_UNORM:
	case PIPE_FORMAT_R8G8_SNORM:
	case PIPE_FORMAT_R8G8_UINT:
	case PIPE_FORMAT_R8G8_SINT:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_R16_UNORM:
	case PIPE_FORMAT_R16_SNORM:
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
	case PIPE_FORMAT_R16G16_SNORM:
	case PIPE_FORMAT_R16G16_UINT:
	case PIPE_FORMAT_R16G16_SINT:
		return V_028C70_SWAP_STD;

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
		return V_028C70_SWAP_STD;
	default:
		R600_ERR("unsupported colorswap format %d\n", format);
		return ~0U;
	}
	return ~0U;
}

static uint32_t r600_translate_colorformat(enum pipe_format format)
{
	switch (format) {
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

	case PIPE_FORMAT_Z16_UNORM:
		return V_028C70_COLOR_16;

	case PIPE_FORMAT_L8A8_UNORM:
	case PIPE_FORMAT_L8A8_SNORM:
	case PIPE_FORMAT_L8A8_UINT:
	case PIPE_FORMAT_L8A8_SINT:
	case PIPE_FORMAT_L8A8_SRGB:
	case PIPE_FORMAT_R8G8_UNORM:
	case PIPE_FORMAT_R8G8_SNORM:
	case PIPE_FORMAT_R8G8_UINT:
	case PIPE_FORMAT_R8G8_SINT:
		return V_028C70_COLOR_8_8;

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
		return V_028C70_COLOR_16;

	case PIPE_FORMAT_R16_FLOAT:
	case PIPE_FORMAT_A16_FLOAT:
	case PIPE_FORMAT_L16_FLOAT:
	case PIPE_FORMAT_I16_FLOAT:
		return V_028C70_COLOR_16_FLOAT;

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

	case PIPE_FORMAT_R32_UINT:
	case PIPE_FORMAT_R32_SINT:
	case PIPE_FORMAT_A32_UINT:
	case PIPE_FORMAT_A32_SINT:
	case PIPE_FORMAT_L32_UINT:
	case PIPE_FORMAT_L32_SINT:
	case PIPE_FORMAT_I32_UINT:
	case PIPE_FORMAT_I32_SINT:
		return V_028C70_COLOR_32;

	case PIPE_FORMAT_R32_FLOAT:
	case PIPE_FORMAT_A32_FLOAT:
	case PIPE_FORMAT_L32_FLOAT:
	case PIPE_FORMAT_I32_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT:
		return V_028C70_COLOR_32_FLOAT;

	case PIPE_FORMAT_R16G16_FLOAT:
	case PIPE_FORMAT_L16A16_FLOAT:
		return V_028C70_COLOR_16_16_FLOAT;

	case PIPE_FORMAT_R16G16_UNORM:
	case PIPE_FORMAT_R16G16_SNORM:
	case PIPE_FORMAT_R16G16_UINT:
	case PIPE_FORMAT_R16G16_SINT:
	case PIPE_FORMAT_L16A16_UNORM:
	case PIPE_FORMAT_L16A16_SNORM:
	case PIPE_FORMAT_L16A16_UINT:
	case PIPE_FORMAT_L16A16_SINT:
		return V_028C70_COLOR_16_16;

	case PIPE_FORMAT_R11G11B10_FLOAT:
		return V_028C70_COLOR_10_11_11_FLOAT;

	/* 64-bit buffers. */
	case PIPE_FORMAT_R16G16B16A16_UINT:
	case PIPE_FORMAT_R16G16B16A16_SINT:
	case PIPE_FORMAT_R16G16B16A16_UNORM:
	case PIPE_FORMAT_R16G16B16A16_SNORM:
		return V_028C70_COLOR_16_16_16_16;

	case PIPE_FORMAT_R16G16B16A16_FLOAT:
		return V_028C70_COLOR_16_16_16_16_FLOAT;

	case PIPE_FORMAT_R32G32_FLOAT:
	case PIPE_FORMAT_L32A32_FLOAT:
		return V_028C70_COLOR_32_32_FLOAT;

	case PIPE_FORMAT_R32G32_SINT:
	case PIPE_FORMAT_R32G32_UINT:
	case PIPE_FORMAT_L32A32_UINT:
	case PIPE_FORMAT_L32A32_SINT:
		return V_028C70_COLOR_32_32;

	/* 128-bit buffers. */
	case PIPE_FORMAT_R32G32B32A32_SNORM:
	case PIPE_FORMAT_R32G32B32A32_UNORM:
	case PIPE_FORMAT_R32G32B32A32_SINT:
	case PIPE_FORMAT_R32G32B32A32_UINT:
		return V_028C70_COLOR_32_32_32_32;
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
		return V_028C70_COLOR_32_32_32_32_FLOAT;

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

		/* 8-bit buffers. */
		case V_028C70_COLOR_8:
			return ENDIAN_NONE;

		/* 16-bit buffers. */
		case V_028C70_COLOR_5_6_5:
		case V_028C70_COLOR_1_5_5_5:
		case V_028C70_COLOR_4_4_4_4:
		case V_028C70_COLOR_16:
		case V_028C70_COLOR_8_8:
			return ENDIAN_8IN16;

		/* 32-bit buffers. */
		case V_028C70_COLOR_8_8_8_8:
		case V_028C70_COLOR_2_10_10_10:
		case V_028C70_COLOR_8_24:
		case V_028C70_COLOR_24_8:
		case V_028C70_COLOR_32_FLOAT:
		case V_028C70_COLOR_16_16_FLOAT:
		case V_028C70_COLOR_16_16:
			return ENDIAN_8IN32;

		/* 64-bit buffers. */
		case V_028C70_COLOR_16_16_16_16:
		case V_028C70_COLOR_16_16_16_16_FLOAT:
			return ENDIAN_8IN16;

		case V_028C70_COLOR_32_32_FLOAT:
		case V_028C70_COLOR_32_32:
		case V_028C70_COLOR_X24_8_32_FLOAT:
			return ENDIAN_8IN32;

		/* 96-bit buffers. */
		case V_028C70_COLOR_32_32_32_FLOAT:
		/* 128-bit buffers. */
		case V_028C70_COLOR_32_32_32_32_FLOAT:
		case V_028C70_COLOR_32_32_32_32:
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

boolean evergreen_is_format_supported(struct pipe_screen *screen,
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
		if (rscreen->info.drm_minor < 19)
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

static void *evergreen_create_blend_state_mode(struct pipe_context *ctx,
					       const struct pipe_blend_state *state, int mode)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_blend *blend = CALLOC_STRUCT(r600_pipe_blend);
	struct r600_pipe_state *rstate;
	uint32_t color_control = 0, target_mask;
	/* XXX there is more then 8 framebuffer */
	unsigned blend_cntl[8];

	if (blend == NULL) {
		return NULL;
	}

	rstate = &blend->rstate;

	rstate->id = R600_PIPE_STATE_BLEND;

	target_mask = 0;
	if (state->logicop_enable) {
		color_control |= (state->logicop_func << 16) | (state->logicop_func << 20);
	} else {
		color_control |= (0xcc << 16);
	}
	/* we pretend 8 buffer are used, CB_SHADER_MASK will disable unused one */
	if (state->independent_blend_enable) {
		for (int i = 0; i < 8; i++) {
			target_mask |= (state->rt[i].colormask << (4 * i));
		}
	} else {
		for (int i = 0; i < 8; i++) {
			target_mask |= (state->rt[0].colormask << (4 * i));
		}
	}
	blend->cb_target_mask = target_mask;

	if (target_mask)
		color_control |= S_028808_MODE(mode);
	else
		color_control |= S_028808_MODE(V_028808_CB_DISABLE);

	r600_pipe_state_add_reg(rstate, R_028808_CB_COLOR_CONTROL,
				color_control);
	/* only have dual source on MRT0 */
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

		blend_cntl[i] = 0;
		if (!state->rt[j].blend_enable)
			continue;

		blend_cntl[i] |= S_028780_BLEND_CONTROL_ENABLE(1);
		blend_cntl[i] |= S_028780_COLOR_COMB_FCN(r600_translate_blend_function(eqRGB));
		blend_cntl[i] |= S_028780_COLOR_SRCBLEND(r600_translate_blend_factor(srcRGB));
		blend_cntl[i] |= S_028780_COLOR_DESTBLEND(r600_translate_blend_factor(dstRGB));

		if (srcA != srcRGB || dstA != dstRGB || eqA != eqRGB) {
			blend_cntl[i] |= S_028780_SEPARATE_ALPHA_BLEND(1);
			blend_cntl[i] |= S_028780_ALPHA_COMB_FCN(r600_translate_blend_function(eqA));
			blend_cntl[i] |= S_028780_ALPHA_SRCBLEND(r600_translate_blend_factor(srcA));
			blend_cntl[i] |= S_028780_ALPHA_DESTBLEND(r600_translate_blend_factor(dstA));
		}
	}
	for (int i = 0; i < 8; i++) {
		r600_pipe_state_add_reg(rstate, R_028780_CB_BLEND0_CONTROL + i * 4, blend_cntl[i]);
	}

	r600_pipe_state_add_reg(rstate, R_028B70_DB_ALPHA_TO_MASK,
				S_028B70_ALPHA_TO_MASK_ENABLE(state->alpha_to_coverage) |
				S_028B70_ALPHA_TO_MASK_OFFSET0(2) |
				S_028B70_ALPHA_TO_MASK_OFFSET1(2) |
				S_028B70_ALPHA_TO_MASK_OFFSET2(2) |
				S_028B70_ALPHA_TO_MASK_OFFSET3(2));

	blend->alpha_to_one = state->alpha_to_one;
	return rstate;
}

static void *evergreen_create_blend_state(struct pipe_context *ctx,
					const struct pipe_blend_state *state)
{

	return evergreen_create_blend_state_mode(ctx, state, V_028808_CB_NORMAL);
}

static void *evergreen_create_dsa_state(struct pipe_context *ctx,
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

	/* misc */
	r600_pipe_state_add_reg(rstate, R_028800_DB_DEPTH_CONTROL, db_depth_control);
	return rstate;
}

static void *evergreen_create_rs_state(struct pipe_context *ctx,
					const struct pipe_rasterizer_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_rasterizer *rs = CALLOC_STRUCT(r600_pipe_rasterizer);
	struct r600_pipe_state *rstate;
	unsigned tmp;
	unsigned prov_vtx = 1, polygon_dual_mode;
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
	tmp = (unsigned)(state->point_size * 8.0);
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

	tmp = (unsigned)state->line_width * 8;
	r600_pipe_state_add_reg(rstate, R_028A08_PA_SU_LINE_CNTL, S_028A08_WIDTH(tmp));
	r600_pipe_state_add_reg(rstate, R_028A48_PA_SC_MODE_CNTL_0,
				S_028A48_MSAA_ENABLE(state->multisample) |
				S_028A48_VPORT_SCISSOR_ENABLE(state->scissor) |
				S_028A48_LINE_STIPPLE_ENABLE(state->line_stipple_enable));

	if (rctx->chip_class == CAYMAN) {
		r600_pipe_state_add_reg(rstate, CM_R_028BE4_PA_SU_VTX_CNTL,
					S_028C08_PIX_CENTER_HALF(state->gl_rasterization_rules) |
					S_028C08_QUANT_MODE(V_028C08_X_1_256TH));
	} else {
		r600_pipe_state_add_reg(rstate, R_028C08_PA_SU_VTX_CNTL,
					S_028C08_PIX_CENTER_HALF(state->gl_rasterization_rules) |
					S_028C08_QUANT_MODE(V_028C08_X_1_256TH));
	}
	r600_pipe_state_add_reg(rstate, R_028B7C_PA_SU_POLY_OFFSET_CLAMP, fui(state->offset_clamp));
	r600_pipe_state_add_reg(rstate, R_028814_PA_SU_SC_MODE_CNTL,
				S_028814_PROVOKING_VTX_LAST(prov_vtx) |
				S_028814_CULL_FRONT(state->rasterizer_discard || (state->cull_face & PIPE_FACE_FRONT) ? 1 : 0) |
				S_028814_CULL_BACK(state->rasterizer_discard || (state->cull_face & PIPE_FACE_BACK) ? 1 : 0) |
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

static void *evergreen_create_sampler_state(struct pipe_context *ctx,
					const struct pipe_sampler_state *state)
{
	struct r600_pipe_sampler_state *ss = CALLOC_STRUCT(r600_pipe_sampler_state);
	union util_color uc;
	unsigned aniso_flag_offset = state->max_anisotropy > 1 ? 2 : 0;

	if (ss == NULL) {
		return NULL;
	}

	/* directly into sampler avoid r6xx code to emit useless reg */
	ss->seamless_cube_map = false;
	util_pack_color(state->border_color.f, PIPE_FORMAT_B8G8R8A8_UNORM, &uc);
	ss->border_color_use = false;
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
	ss->tex_sampler_words[1] = S_03C004_MIN_LOD(S_FIXED(CLAMP(state->min_lod, 0, 15), 8)) |
				S_03C004_MAX_LOD(S_FIXED(CLAMP(state->max_lod, 0, 15), 8));
	/* R_03C008_SQ_TEX_SAMPLER_WORD2_0 */
	ss->tex_sampler_words[2] = S_03C008_LOD_BIAS(S_FIXED(CLAMP(state->lod_bias, -16, 16), 8)) |
				(state->seamless_cube_map ? 0 : S_03C008_DISABLE_CUBE_WRAP(1)) |
				S_03C008_TYPE(1);
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

static struct pipe_sampler_view *evergreen_create_sampler_view(struct pipe_context *ctx,
							struct pipe_resource *texture,
							const struct pipe_sampler_view *state)
{
	struct r600_screen *rscreen = (struct r600_screen*)ctx->screen;
	struct r600_pipe_sampler_view *view = CALLOC_STRUCT(r600_pipe_sampler_view);
	struct r600_texture *tmp = (struct r600_texture*)texture;
	unsigned format, endian;
	uint32_t word4 = 0, yuv_format = 0, pitch = 0;
	unsigned char swizzle[4], array_mode = 0, tile_type = 0;
	unsigned height, depth, width;
	unsigned macro_aspect, tile_split, bankh, bankw, nbanks;

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

	width = tmp->surface.level[0].npix_x;
	height = tmp->surface.level[0].npix_y;
	depth = tmp->surface.level[0].npix_z;
	pitch = tmp->surface.level[0].nblk_x * util_format_get_blockwidth(state->format);
	tile_type = tmp->tile_type;

	switch (tmp->surface.level[0].mode) {
	case RADEON_SURF_MODE_LINEAR_ALIGNED:
		array_mode = V_028C70_ARRAY_LINEAR_ALIGNED;
		break;
	case RADEON_SURF_MODE_2D:
		array_mode = V_028C70_ARRAY_2D_TILED_THIN1;
		break;
	case RADEON_SURF_MODE_1D:
		array_mode = V_028C70_ARRAY_1D_TILED_THIN1;
		break;
	case RADEON_SURF_MODE_LINEAR:
	default:
		array_mode = V_028C70_ARRAY_LINEAR_GENERAL;
		break;
	}
	tile_split = tmp->surface.tile_split;
	macro_aspect = tmp->surface.mtilea;
	bankw = tmp->surface.bankw;
	bankh = tmp->surface.bankh;
	tile_split = eg_tile_split(tile_split);
	macro_aspect = eg_macro_tile_aspect(macro_aspect);
	bankw = eg_bank_wh(bankw);
	bankh = eg_bank_wh(bankh);

	/* 128 bit formats require tile type = 1 */
	if (rscreen->chip_class == CAYMAN) {
		if (util_format_get_blocksize(state->format) >= 16)
			tile_type = 1;
	}
	nbanks = eg_num_banks(rscreen->tiling_info.num_banks);

	if (texture->target == PIPE_TEXTURE_1D_ARRAY) {
	        height = 1;
		depth = texture->array_size;
	} else if (texture->target == PIPE_TEXTURE_2D_ARRAY) {
		depth = texture->array_size;
	}

	view->tex_resource = &tmp->resource;
	view->tex_resource_words[0] = (S_030000_DIM(r600_tex_dim(texture->target, texture->nr_samples)) |
				       S_030000_PITCH((pitch / 8) - 1) |
				       S_030000_TEX_WIDTH(width - 1));
	if (rscreen->chip_class == CAYMAN)
		view->tex_resource_words[0] |= CM_S_030000_NON_DISP_TILING_ORDER(tile_type);
	else
		view->tex_resource_words[0] |= S_030000_NON_DISP_TILING_ORDER(tile_type);
	view->tex_resource_words[1] = (S_030004_TEX_HEIGHT(height - 1) |
				       S_030004_TEX_DEPTH(depth - 1) |
				       S_030004_ARRAY_MODE(array_mode));
	view->tex_resource_words[2] = (tmp->surface.level[0].offset + r600_resource_va(ctx->screen, texture)) >> 8;
	if (state->u.tex.last_level && texture->nr_samples <= 1) {
		view->tex_resource_words[3] = (tmp->surface.level[1].offset + r600_resource_va(ctx->screen, texture)) >> 8;
	} else {
		view->tex_resource_words[3] = (tmp->surface.level[0].offset + r600_resource_va(ctx->screen, texture)) >> 8;
	}
	view->tex_resource_words[4] = (word4 |
				       S_030010_SRF_MODE_ALL(V_030010_SRF_MODE_ZERO_CLAMP_MINUS_ONE) |
				       S_030010_ENDIAN_SWAP(endian));
	view->tex_resource_words[5] = S_030014_BASE_ARRAY(state->u.tex.first_layer) |
				      S_030014_LAST_ARRAY(state->u.tex.last_layer);
	if (texture->nr_samples > 1) {
		unsigned log_samples = util_logbase2(texture->nr_samples);
		if (rscreen->chip_class == CAYMAN) {
			view->tex_resource_words[4] |= S_030010_LOG2_NUM_FRAGMENTS(log_samples);
		}
		/* LAST_LEVEL holds log2(nr_samples) for multisample textures */
		view->tex_resource_words[5] |= S_030014_LAST_LEVEL(log_samples);
	} else {
		view->tex_resource_words[4] |= S_030010_BASE_LEVEL(state->u.tex.first_level);
		view->tex_resource_words[5] |= S_030014_LAST_LEVEL(state->u.tex.last_level);
	}
	/* aniso max 16 samples */
	view->tex_resource_words[6] = (S_030018_MAX_ANISO(4)) |
				      (S_030018_TILE_SPLIT(tile_split));
	view->tex_resource_words[7] = S_03001C_DATA_FORMAT(format) |
				      S_03001C_TYPE(V_03001C_SQ_TEX_VTX_VALID_TEXTURE) |
				      S_03001C_BANK_WIDTH(bankw) |
				      S_03001C_BANK_HEIGHT(bankh) |
				      S_03001C_MACRO_TILE_ASPECT(macro_aspect) |
				      S_03001C_NUM_BANKS(nbanks);
	return &view->base;
}

static void evergreen_set_vs_sampler_views(struct pipe_context *ctx, unsigned count,
					   struct pipe_sampler_view **views)
{
	r600_set_sampler_views(ctx, PIPE_SHADER_VERTEX, 0, count, views);
}

static void evergreen_set_ps_sampler_views(struct pipe_context *ctx, unsigned count,
					   struct pipe_sampler_view **views)
{
	r600_set_sampler_views(ctx, PIPE_SHADER_FRAGMENT, 0, count, views);
}

static void evergreen_set_clip_state(struct pipe_context *ctx,
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
					R_0285BC_PA_CL_UCP0_X + i * 16,
					fui(state->ucp[i][0]));
		r600_pipe_state_add_reg(rstate,
					R_0285C0_PA_CL_UCP0_Y + i * 16,
					fui(state->ucp[i][1]) );
		r600_pipe_state_add_reg(rstate,
					R_0285C4_PA_CL_UCP0_Z + i * 16,
					fui(state->ucp[i][2]));
		r600_pipe_state_add_reg(rstate,
					R_0285C8_PA_CL_UCP0_W + i * 16,
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

static void evergreen_set_polygon_stipple(struct pipe_context *ctx,
					 const struct pipe_poly_stipple *state)
{
}

static void evergreen_get_scissor_rect(struct r600_context *rctx,
				       unsigned tl_x, unsigned tl_y, unsigned br_x, unsigned br_y,
				       uint32_t *tl, uint32_t *br)
{
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

	*tl = S_028240_TL_X(tl_x) | S_028240_TL_Y(tl_y);
	*br = S_028244_BR_X(br_x) | S_028244_BR_Y(br_y);
}

static void evergreen_set_scissor_state(struct pipe_context *ctx,
					const struct pipe_scissor_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_state *rstate = CALLOC_STRUCT(r600_pipe_state);
	uint32_t tl, br;

	if (rstate == NULL)
		return;

	evergreen_get_scissor_rect(rctx, state->minx, state->miny, state->maxx, state->maxy, &tl, &br);

	rstate->id = R600_PIPE_STATE_SCISSOR;
	r600_pipe_state_add_reg(rstate, R_028250_PA_SC_VPORT_SCISSOR_0_TL, tl);
	r600_pipe_state_add_reg(rstate, R_028254_PA_SC_VPORT_SCISSOR_0_BR, br);

	free(rctx->states[R600_PIPE_STATE_SCISSOR]);
	rctx->states[R600_PIPE_STATE_SCISSOR] = rstate;
	r600_context_pipe_state_set(rctx, rstate);
}

static void evergreen_set_viewport_state(struct pipe_context *ctx,
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

void evergreen_init_color_surface(struct r600_context *rctx,
				  struct r600_surface *surf)
{
	struct r600_screen *rscreen = rctx->screen;
	struct r600_texture *rtex = (struct r600_texture*)surf->base.texture;
	struct pipe_resource *pipe_tex = surf->base.texture;
	unsigned level = surf->base.u.tex.level;
	unsigned pitch, slice;
	unsigned color_info, color_attrib, color_dim = 0;
	unsigned format, swap, ntype, endian;
	uint64_t offset, base_offset;
	unsigned tile_type, macro_aspect, tile_split, bankh, bankw, fmask_bankh, nbanks;
	const struct util_format_description *desc;
	int i;
	bool blend_clamp = 0, blend_bypass = 0;

	if (rtex->is_depth && !rtex->is_flushing_texture) {
		r600_init_flushed_depth_texture(&rctx->context, pipe_tex, NULL);
		rtex = rtex->flushed_depth_texture;
		assert(rtex);
	}

	offset = rtex->surface.level[level].offset;
	if (rtex->surface.level[level].mode < RADEON_SURF_MODE_1D) {
		offset += rtex->surface.level[level].slice_size *
			  surf->base.u.tex.first_layer;
	}
	pitch = (rtex->surface.level[level].nblk_x) / 8 - 1;
	slice = (rtex->surface.level[level].nblk_x * rtex->surface.level[level].nblk_y) / 64;
	if (slice) {
		slice = slice - 1;
	}
	color_info = 0;
	switch (rtex->surface.level[level].mode) {
	case RADEON_SURF_MODE_LINEAR_ALIGNED:
		color_info = S_028C70_ARRAY_MODE(V_028C70_ARRAY_LINEAR_ALIGNED);
		tile_type = 1;
		break;
	case RADEON_SURF_MODE_1D:
		color_info = S_028C70_ARRAY_MODE(V_028C70_ARRAY_1D_TILED_THIN1);
		tile_type = rtex->tile_type;
		break;
	case RADEON_SURF_MODE_2D:
		color_info = S_028C70_ARRAY_MODE(V_028C70_ARRAY_2D_TILED_THIN1);
		tile_type = rtex->tile_type;
		break;
	case RADEON_SURF_MODE_LINEAR:
	default:
		color_info = S_028C70_ARRAY_MODE(V_028C70_ARRAY_LINEAR_GENERAL);
		tile_type = 1;
		break;
	}
	tile_split = rtex->surface.tile_split;
	macro_aspect = rtex->surface.mtilea;
	bankw = rtex->surface.bankw;
	bankh = rtex->surface.bankh;
	fmask_bankh = rtex->fmask_bank_height;
	tile_split = eg_tile_split(tile_split);
	macro_aspect = eg_macro_tile_aspect(macro_aspect);
	bankw = eg_bank_wh(bankw);
	bankh = eg_bank_wh(bankh);
	fmask_bankh = eg_bank_wh(fmask_bankh);

	/* 128 bit formats require tile type = 1 */
	if (rscreen->chip_class == CAYMAN) {
		if (util_format_get_blocksize(surf->base.format) >= 16)
			tile_type = 1;
	}
	nbanks = eg_num_banks(rscreen->tiling_info.num_banks);
	desc = util_format_description(surf->base.format);
	for (i = 0; i < 4; i++) {
		if (desc->channel[i].type != UTIL_FORMAT_TYPE_VOID) {
			break;
		}
	}

	color_attrib = S_028C74_TILE_SPLIT(tile_split)|
			S_028C74_NUM_BANKS(nbanks) |
			S_028C74_BANK_WIDTH(bankw) |
			S_028C74_BANK_HEIGHT(bankh) |
			S_028C74_MACRO_TILE_ASPECT(macro_aspect) |
			S_028C74_NON_DISP_TILING_ORDER(tile_type) |
		        S_028C74_FMASK_BANK_HEIGHT(fmask_bankh);

	if (rctx->chip_class == CAYMAN && rtex->resource.b.b.nr_samples > 1) {
		unsigned log_samples = util_logbase2(rtex->resource.b.b.nr_samples);
		color_attrib |= S_028C74_NUM_SAMPLES(log_samples) |
				S_028C74_NUM_FRAGMENTS(log_samples);
	}

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

	format = r600_translate_colorformat(surf->base.format);
	assert(format != ~0);

	swap = r600_translate_colorswap(surf->base.format);
	assert(swap != ~0);

	if (rtex->resource.b.b.usage == PIPE_USAGE_STAGING) {
		endian = ENDIAN_NONE;
	} else {
		endian = r600_colorformat_endian_swap(format);
	}

	/* blend clamp should be set for all NORM/SRGB types */
	if (ntype == V_028C70_NUMBER_UNORM || ntype == V_028C70_NUMBER_SNORM ||
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

	surf->alphatest_bypass = ntype == V_028C70_NUMBER_UINT || ntype == V_028C70_NUMBER_SINT;

	color_info |= S_028C70_FORMAT(format) |
		S_028C70_COMP_SWAP(swap) |
		S_028C70_BLEND_CLAMP(blend_clamp) |
		S_028C70_BLEND_BYPASS(blend_bypass) |
		S_028C70_NUMBER_TYPE(ntype) |
		S_028C70_ENDIAN(endian);

	if (rtex->is_rat) {
		color_info |= S_028C70_RAT(1);
		color_dim = S_028C78_WIDTH_MAX(pipe_tex->width0)
				| S_028C78_HEIGHT_MAX(pipe_tex->height0);
	}

	/* EXPORT_NORM is an optimzation that can be enabled for better
	 * performance in certain cases.
	 * EXPORT_NORM can be enabled if:
	 * - 11-bit or smaller UNORM/SNORM/SRGB
	 * - 16-bit or smaller FLOAT
	 */
	if (desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS &&
	    ((desc->channel[i].size < 12 &&
	      desc->channel[i].type != UTIL_FORMAT_TYPE_FLOAT &&
	      ntype != V_028C70_NUMBER_UINT && ntype != V_028C70_NUMBER_SINT) ||
	     (desc->channel[i].size < 17 &&
	      desc->channel[i].type == UTIL_FORMAT_TYPE_FLOAT))) {
		color_info |= S_028C70_SOURCE_FORMAT(V_028C70_EXPORT_4C_16BPC);
		surf->export_16bpc = true;
	}

	if (rtex->fmask_size && rtex->cmask_size) {
		color_info |= S_028C70_COMPRESSION(1) | S_028C70_FAST_CLEAR(1);
	}

	base_offset = r600_resource_va(rctx->context.screen, pipe_tex);

	/* XXX handle enabling of CB beyond BASE8 which has different offset */
	surf->cb_color_base = (base_offset + offset) >> 8;
	surf->cb_color_dim = color_dim;
	surf->cb_color_info = color_info;
	surf->cb_color_pitch = S_028C64_PITCH_TILE_MAX(pitch);
	surf->cb_color_slice = S_028C68_SLICE_TILE_MAX(slice);
	if (rtex->surface.level[level].mode < RADEON_SURF_MODE_1D) {
		surf->cb_color_view = 0;
	} else {
		surf->cb_color_view = S_028C6C_SLICE_START(surf->base.u.tex.first_layer) |
				      S_028C6C_SLICE_MAX(surf->base.u.tex.last_layer);
	}
	surf->cb_color_attrib = color_attrib;
	if (rtex->fmask_size && rtex->cmask_size) {
		surf->cb_color_fmask = (base_offset + rtex->fmask_offset) >> 8;
		surf->cb_color_cmask = (base_offset + rtex->cmask_offset) >> 8;
	} else {
		surf->cb_color_fmask = surf->cb_color_base;
		surf->cb_color_cmask = surf->cb_color_base;
	}
	surf->cb_color_fmask_slice = S_028C88_TILE_MAX(slice);
	surf->cb_color_cmask_slice = S_028C80_TILE_MAX(rtex->cmask_slice_tile_max);

	surf->color_initialized = true;
}

static void evergreen_init_depth_surface(struct r600_context *rctx,
					 struct r600_surface *surf)
{
	struct r600_screen *rscreen = rctx->screen;
	struct pipe_screen *screen = &rscreen->screen;
	struct r600_texture *rtex = (struct r600_texture*)surf->base.texture;
	uint64_t offset;
	unsigned level, pitch, slice, format, array_mode;
	unsigned macro_aspect, tile_split, bankh, bankw, nbanks;

	level = surf->base.u.tex.level;
	format = r600_translate_dbformat(surf->base.format);
	assert(format != ~0);

	offset = r600_resource_va(screen, surf->base.texture);
	offset += rtex->surface.level[level].offset;
	pitch = (rtex->surface.level[level].nblk_x / 8) - 1;
	slice = (rtex->surface.level[level].nblk_x * rtex->surface.level[level].nblk_y) / 64;
	if (slice) {
		slice = slice - 1;
	}
	switch (rtex->surface.level[level].mode) {
	case RADEON_SURF_MODE_2D:
		array_mode = V_028C70_ARRAY_2D_TILED_THIN1;
		break;
	case RADEON_SURF_MODE_1D:
	case RADEON_SURF_MODE_LINEAR_ALIGNED:
	case RADEON_SURF_MODE_LINEAR:
	default:
		array_mode = V_028C70_ARRAY_1D_TILED_THIN1;
		break;
	}
	tile_split = rtex->surface.tile_split;
	macro_aspect = rtex->surface.mtilea;
	bankw = rtex->surface.bankw;
	bankh = rtex->surface.bankh;
	tile_split = eg_tile_split(tile_split);
	macro_aspect = eg_macro_tile_aspect(macro_aspect);
	bankw = eg_bank_wh(bankw);
	bankh = eg_bank_wh(bankh);
	nbanks = eg_num_banks(rscreen->tiling_info.num_banks);
	offset >>= 8;

	surf->db_depth_info = S_028040_ARRAY_MODE(array_mode) |
			      S_028040_FORMAT(format) |
			      S_028040_TILE_SPLIT(tile_split)|
			      S_028040_NUM_BANKS(nbanks) |
			      S_028040_BANK_WIDTH(bankw) |
			      S_028040_BANK_HEIGHT(bankh) |
			      S_028040_MACRO_TILE_ASPECT(macro_aspect);
	if (rscreen->chip_class == CAYMAN && rtex->resource.b.b.nr_samples > 1) {
		surf->db_depth_info |= S_028040_NUM_SAMPLES(util_logbase2(rtex->resource.b.b.nr_samples));
	}
	surf->db_depth_base = offset;
	surf->db_depth_view = S_028008_SLICE_START(surf->base.u.tex.first_layer) |
			      S_028008_SLICE_MAX(surf->base.u.tex.last_layer);
	surf->db_depth_size = S_028058_PITCH_TILE_MAX(pitch);
	surf->db_depth_slice = S_02805C_SLICE_TILE_MAX(slice);

	if (rtex->surface.flags & RADEON_SURF_SBUFFER) {
		uint64_t stencil_offset = rtex->surface.stencil_offset;
		unsigned i, stile_split = rtex->surface.stencil_tile_split;

		stile_split = eg_tile_split(stile_split);
		stencil_offset += r600_resource_va(screen, surf->base.texture);
		stencil_offset += rtex->surface.level[level].offset / 4;
		stencil_offset >>= 8;

		/* We're guessing the stencil offset from the depth offset.
		 * Make sure each mipmap level has a unique offset. */
		for (i = 1; i <= level; i++) {
			/* If two levels have the same address, add 256
			 * to the offset of the smaller level. */
			if ((rtex->surface.level[i-1].offset / 4) >> 8 ==
			    (rtex->surface.level[i].offset / 4) >> 8) {
				stencil_offset++;
			}
		}

		surf->db_stencil_base = stencil_offset;
		surf->db_stencil_info = 1 | S_028044_TILE_SPLIT(stile_split);
	} else {
		surf->db_stencil_base = offset;
		surf->db_stencil_info = 1;
	}

	surf->depth_initialized = true;
}

#define FILL_SREG(s0x, s0y, s1x, s1y, s2x, s2y, s3x, s3y)  \
	(((s0x) & 0xf) | (((s0y) & 0xf) << 4) |		   \
	(((s1x) & 0xf) << 8) | (((s1y) & 0xf) << 12) |	   \
	(((s2x) & 0xf) << 16) | (((s2y) & 0xf) << 20) |	   \
	 (((s3x) & 0xf) << 24) | (((s3y) & 0xf) << 28))

static uint32_t evergreen_set_ms_pos(struct pipe_context *ctx, struct r600_pipe_state *rstate, int nsample)
{
	/* 2xMSAA
	 * There are two locations (-4, 4), (4, -4). */
	static uint32_t sample_locs_2x[] = {
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
	};
	static unsigned max_dist_2x = 4;
	/* 4xMSAA
	 * There are 4 locations: (-2, -2), (2, 2), (-6, 6), (6, -6). */
	static uint32_t sample_locs_4x[] = {
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
	};
	static unsigned max_dist_4x = 6;
	/* 8xMSAA */
	static uint32_t sample_locs_8x[] = {
		FILL_SREG(-2, -5, 3, -4, -1, 5, -6, -2),
		FILL_SREG( 6,  0, 0,  0, -5, 3,  4,  4),
		FILL_SREG(-2, -5, 3, -4, -1, 5, -6, -2),
		FILL_SREG( 6,  0, 0,  0, -5, 3,  4,  4),
		FILL_SREG(-2, -5, 3, -4, -1, 5, -6, -2),
		FILL_SREG( 6,  0, 0,  0, -5, 3,  4,  4),
		FILL_SREG(-2, -5, 3, -4, -1, 5, -6, -2),
		FILL_SREG( 6,  0, 0,  0, -5, 3,  4,  4),
	};
	static unsigned max_dist_8x = 8;
	struct r600_context *rctx = (struct r600_context *)ctx;
	unsigned i;

	switch (nsample) {
	case 2:
		for (i = 0; i < Elements(sample_locs_2x); i++) {
			r600_pipe_state_add_reg(rstate, R_028C1C_PA_SC_AA_SAMPLE_LOCS_0 + i*4,
						sample_locs_2x[i]);
		}
		return max_dist_2x;
	case 4:
		for (i = 0; i < Elements(sample_locs_4x); i++) {
			r600_pipe_state_add_reg(rstate, R_028C1C_PA_SC_AA_SAMPLE_LOCS_0 + i*4,
						sample_locs_4x[i]);
		}
		return max_dist_4x;
	case 8:
		for (i = 0; i < Elements(sample_locs_8x); i++) {
			r600_pipe_state_add_reg(rstate, R_028C1C_PA_SC_AA_SAMPLE_LOCS_0 + i*4,
						sample_locs_8x[i]);
		}
		return max_dist_8x;
	default:
		R600_ERR("Invalid nr_samples %i\n", nsample);
		return 0;
	}
}

static uint32_t cayman_set_ms_pos(struct pipe_context *ctx, struct r600_pipe_state *rstate, int nsample)
{
	/* 2xMSAA
	 * There are two locations (-4, 4), (4, -4). */
	static uint32_t sample_locs_2x[] = {
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
	};
	static unsigned max_dist_2x = 4;
	/* 4xMSAA
	 * There are 4 locations: (-2, -2), (2, 2), (-6, 6), (6, -6). */
	static uint32_t sample_locs_4x[] = {
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
	};
	static unsigned max_dist_4x = 6;
	/* 8xMSAA */
	static uint32_t sample_locs_8x[] = {
		FILL_SREG(-2, -5, 3, -4, -1, 5, -6, -2),
		FILL_SREG(-2, -5, 3, -4, -1, 5, -6, -2),
		FILL_SREG(-2, -5, 3, -4, -1, 5, -6, -2),
		FILL_SREG(-2, -5, 3, -4, -1, 5, -6, -2),
		FILL_SREG( 6,  0, 0,  0, -5, 3,  4,  4),
		FILL_SREG( 6,  0, 0,  0, -5, 3,  4,  4),
		FILL_SREG( 6,  0, 0,  0, -5, 3,  4,  4),
		FILL_SREG( 6,  0, 0,  0, -5, 3,  4,  4),
	};
	static unsigned max_dist_8x = 8;
	/* 16xMSAA */
	static uint32_t sample_locs_16x[] = {
		FILL_SREG(-7, -3, 7, 3, 1, -5, -5, 5),
		FILL_SREG(-7, -3, 7, 3, 1, -5, -5, 5),
		FILL_SREG(-7, -3, 7, 3, 1, -5, -5, 5),
		FILL_SREG(-7, -3, 7, 3, 1, -5, -5, 5),
		FILL_SREG(-3, -7, 3, 7, 5, -1, -1, 1),
		FILL_SREG(-3, -7, 3, 7, 5, -1, -1, 1),
		FILL_SREG(-3, -7, 3, 7, 5, -1, -1, 1),
		FILL_SREG(-3, -7, 3, 7, 5, -1, -1, 1),
		FILL_SREG(-8, -6, 4, 2, 2, -8, -2, 6),
		FILL_SREG(-8, -6, 4, 2, 2, -8, -2, 6),
		FILL_SREG(-8, -6, 4, 2, 2, -8, -2, 6),
		FILL_SREG(-8, -6, 4, 2, 2, -8, -2, 6),
		FILL_SREG(-4, -2, 0, 4, 6, -4, -6, 0),
		FILL_SREG(-4, -2, 0, 4, 6, -4, -6, 0),
		FILL_SREG(-4, -2, 0, 4, 6, -4, -6, 0),
		FILL_SREG(-4, -2, 0, 4, 6, -4, -6, 0),
	};
	static unsigned max_dist_16x = 8;
	struct r600_context *rctx = (struct r600_context *)ctx;
	uint32_t max_dist, num_regs, *sample_locs;

	switch (nsample) {
	case 2:
		sample_locs = sample_locs_2x;
		num_regs = Elements(sample_locs_2x);
		max_dist = max_dist_2x;
		break;
	case 4:
		sample_locs = sample_locs_4x;
		num_regs = Elements(sample_locs_4x);
		max_dist = max_dist_4x;
		break;
	case 8:
		sample_locs = sample_locs_8x;
		num_regs = Elements(sample_locs_8x);
		max_dist = max_dist_8x;
		break;
	case 16:
		sample_locs = sample_locs_16x;
		num_regs = Elements(sample_locs_16x);
		max_dist = max_dist_16x;
		break;
	default:
		R600_ERR("Invalid nr_samples %i\n", nsample);
		return 0;
	}

	r600_pipe_state_add_reg(rstate, CM_R_028BF8_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0, sample_locs[0]);
	r600_pipe_state_add_reg(rstate, CM_R_028C08_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_0, sample_locs[1]);
	r600_pipe_state_add_reg(rstate, CM_R_028C18_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_0, sample_locs[2]);
	r600_pipe_state_add_reg(rstate, CM_R_028C28_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_0, sample_locs[3]);
	if (num_regs <= 8) {
		r600_pipe_state_add_reg(rstate, CM_R_028BFC_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_1, sample_locs[4]);
		r600_pipe_state_add_reg(rstate, CM_R_028C0C_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_1, sample_locs[5]);
		r600_pipe_state_add_reg(rstate, CM_R_028C1C_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_1, sample_locs[6]);
		r600_pipe_state_add_reg(rstate, CM_R_028C2C_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_1, sample_locs[7]);
	}
	if (num_regs <= 16) {
		r600_pipe_state_add_reg(rstate, CM_R_028C00_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_2, sample_locs[8]);
		r600_pipe_state_add_reg(rstate, CM_R_028C10_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_2, sample_locs[9]);
		r600_pipe_state_add_reg(rstate, CM_R_028C20_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_2, sample_locs[10]);
		r600_pipe_state_add_reg(rstate, CM_R_028C30_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_2, sample_locs[11]);
		r600_pipe_state_add_reg(rstate, CM_R_028C04_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_3, sample_locs[12]);
		r600_pipe_state_add_reg(rstate, CM_R_028C14_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_3, sample_locs[13]);
		r600_pipe_state_add_reg(rstate, CM_R_028C24_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_3, sample_locs[14]);
		r600_pipe_state_add_reg(rstate, CM_R_028C34_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_3, sample_locs[15]);
	}
	return max_dist;
}

static void evergreen_set_framebuffer_state(struct pipe_context *ctx,
					    const struct pipe_framebuffer_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_state *rstate = CALLOC_STRUCT(r600_pipe_state);
	struct r600_surface *surf;
	struct r600_resource *res;
	struct r600_texture *rtex;
	uint32_t tl, br, i, nr_samples, log_samples;

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
		surf = (struct r600_surface*)state->cbufs[i];
		res = (struct r600_resource*)surf->base.texture;
		rtex = (struct r600_texture*)res;

		r600_context_add_resource_size(ctx, state->cbufs[i]->texture);

		if (!surf->color_initialized) {
			evergreen_init_color_surface(rctx, surf);
		}

		if (!surf->export_16bpc) {
			rctx->export_16bpc = false;
		}

		r600_pipe_state_add_reg_bo(rstate, R_028C60_CB_COLOR0_BASE + i * 0x3C,
					   surf->cb_color_base, res, RADEON_USAGE_READWRITE);
		r600_pipe_state_add_reg(rstate, R_028C78_CB_COLOR0_DIM + i * 0x3C,
					surf->cb_color_dim);
		r600_pipe_state_add_reg_bo(rstate, R_028C70_CB_COLOR0_INFO + i * 0x3C,
					   surf->cb_color_info, res, RADEON_USAGE_READWRITE);
		r600_pipe_state_add_reg(rstate, R_028C64_CB_COLOR0_PITCH + i * 0x3C,
					surf->cb_color_pitch);
		r600_pipe_state_add_reg(rstate, R_028C68_CB_COLOR0_SLICE + i * 0x3C,
					surf->cb_color_slice);
		r600_pipe_state_add_reg(rstate, R_028C6C_CB_COLOR0_VIEW + i * 0x3C,
					surf->cb_color_view);
		r600_pipe_state_add_reg_bo(rstate, R_028C74_CB_COLOR0_ATTRIB + i * 0x3C,
					   surf->cb_color_attrib, res, RADEON_USAGE_READWRITE);
		r600_pipe_state_add_reg_bo(rstate, R_028C7C_CB_COLOR0_CMASK + i * 0x3c,
					   surf->cb_color_cmask, res, RADEON_USAGE_READWRITE);
		r600_pipe_state_add_reg(rstate, R_028C80_CB_COLOR0_CMASK_SLICE + i * 0x3c,
					surf->cb_color_cmask_slice);
		r600_pipe_state_add_reg_bo(rstate,  R_028C84_CB_COLOR0_FMASK + i * 0x3c,
					   surf->cb_color_fmask, res, RADEON_USAGE_READWRITE);
		r600_pipe_state_add_reg(rstate, R_028C88_CB_COLOR0_FMASK_SLICE + i * 0x3c,
					surf->cb_color_fmask_slice);

		/* Cayman can fetch from a compressed MSAA colorbuffer,
		 * so it's pointless to track them. */
		if (rctx->chip_class != CAYMAN && rtex->fmask_size && rtex->cmask_size) {
			rctx->compressed_cb_mask |= 1 << i;
		}
	}
	/* set CB_COLOR1_INFO for possible dual-src blending */
	if (i == 1 && !((struct r600_texture*)res)->is_rat) {
		r600_pipe_state_add_reg_bo(rstate, R_028C70_CB_COLOR0_INFO + 1 * 0x3C,
					   surf->cb_color_info, res, RADEON_USAGE_READWRITE);
		i++;
	}
	for (; i < 8 ; i++) {
		r600_pipe_state_add_reg(rstate, R_028C70_CB_COLOR0_INFO + i * 0x3C, 0);
	}

	/* Update alpha-test state dependencies.
	 * Alpha-test is done on the first colorbuffer only. */
	if (state->nr_cbufs) {
		surf = (struct r600_surface*)state->cbufs[0];
		if (rctx->alphatest_state.bypass != surf->alphatest_bypass) {
			rctx->alphatest_state.bypass = surf->alphatest_bypass;
			r600_atom_dirty(rctx, &rctx->alphatest_state.atom);
		}
		if (rctx->alphatest_state.cb0_export_16bpc != surf->export_16bpc) {
			rctx->alphatest_state.cb0_export_16bpc = surf->export_16bpc;
			r600_atom_dirty(rctx, &rctx->alphatest_state.atom);
		}
	}

	/* ZS buffer. */
	if (state->zsbuf) {
		surf = (struct r600_surface*)state->zsbuf;
		res = (struct r600_resource*)surf->base.texture;

		r600_context_add_resource_size(ctx, state->zsbuf->texture);

		if (!surf->depth_initialized) {
			evergreen_init_depth_surface(rctx, surf);
		}

		r600_pipe_state_add_reg_bo(rstate, R_028048_DB_Z_READ_BASE, surf->db_depth_base,
					   res, RADEON_USAGE_READWRITE);
		r600_pipe_state_add_reg_bo(rstate, R_028050_DB_Z_WRITE_BASE, surf->db_depth_base,
					   res, RADEON_USAGE_READWRITE);
		r600_pipe_state_add_reg(rstate, R_028008_DB_DEPTH_VIEW, surf->db_depth_view);

		r600_pipe_state_add_reg_bo(rstate, R_02804C_DB_STENCIL_READ_BASE, surf->db_stencil_base,
					   res, RADEON_USAGE_READWRITE);
		r600_pipe_state_add_reg_bo(rstate, R_028054_DB_STENCIL_WRITE_BASE, surf->db_stencil_base,
					   res, RADEON_USAGE_READWRITE);
		r600_pipe_state_add_reg_bo(rstate, R_028044_DB_STENCIL_INFO, surf->db_stencil_info,
					   res, RADEON_USAGE_READWRITE);

		r600_pipe_state_add_reg_bo(rstate, R_028040_DB_Z_INFO, surf->db_depth_info,
					   res, RADEON_USAGE_READWRITE);
		r600_pipe_state_add_reg(rstate, R_028058_DB_DEPTH_SIZE, surf->db_depth_size);
		r600_pipe_state_add_reg(rstate, R_02805C_DB_DEPTH_SLICE, surf->db_depth_slice);
	}

	/* Framebuffer dimensions. */
	evergreen_get_scissor_rect(rctx, 0, 0, state->width, state->height, &tl, &br);

	r600_pipe_state_add_reg(rstate,
				R_028204_PA_SC_WINDOW_SCISSOR_TL, tl);
	r600_pipe_state_add_reg(rstate,
				R_028208_PA_SC_WINDOW_SCISSOR_BR, br);

	/* Multisampling */
	if (state->nr_cbufs)
		nr_samples = state->cbufs[0]->texture->nr_samples;
	else if (state->zsbuf)
		nr_samples = state->zsbuf->texture->nr_samples;
	else
		nr_samples = 0;

	if (nr_samples > 1) {
		unsigned line_cntl = S_028C00_LAST_PIXEL(1) |
				     S_028C00_EXPAND_LINE_WIDTH(1);
		log_samples = util_logbase2(nr_samples);

		if (rctx->chip_class == CAYMAN) {
			unsigned max_dist = cayman_set_ms_pos(ctx, rstate, nr_samples);

			r600_pipe_state_add_reg(rstate, CM_R_028BDC_PA_SC_LINE_CNTL, line_cntl);
			r600_pipe_state_add_reg(rstate, CM_R_028BE0_PA_SC_AA_CONFIG,
						S_028BE0_MSAA_NUM_SAMPLES(log_samples) |
						S_028BE0_MAX_SAMPLE_DIST(max_dist) |
						S_028BE0_MSAA_EXPOSED_SAMPLES(log_samples));
			r600_pipe_state_add_reg(rstate, CM_R_028804_DB_EQAA,
						S_028804_MAX_ANCHOR_SAMPLES(log_samples) |
						S_028804_PS_ITER_SAMPLES(log_samples) |
						S_028804_MASK_EXPORT_NUM_SAMPLES(log_samples) |
						S_028804_ALPHA_TO_MASK_NUM_SAMPLES(log_samples) |
						S_028804_HIGH_QUALITY_INTERSECTIONS(1) |
						S_028804_STATIC_ANCHOR_ASSOCIATIONS(1));
		} else {
			unsigned max_dist = evergreen_set_ms_pos(ctx, rstate, nr_samples);

			r600_pipe_state_add_reg(rstate, R_028C00_PA_SC_LINE_CNTL, line_cntl);
			r600_pipe_state_add_reg(rstate, R_028C04_PA_SC_AA_CONFIG,
						S_028C04_MSAA_NUM_SAMPLES(log_samples) |
						S_028C04_MAX_SAMPLE_DIST(max_dist));
		}
	} else {
		log_samples = 0;

		if (rctx->chip_class == CAYMAN) {
			r600_pipe_state_add_reg(rstate, CM_R_028BDC_PA_SC_LINE_CNTL, S_028C00_LAST_PIXEL(1));
			r600_pipe_state_add_reg(rstate, CM_R_028BE0_PA_SC_AA_CONFIG, 0);
			r600_pipe_state_add_reg(rstate, CM_R_028804_DB_EQAA,
						S_028804_HIGH_QUALITY_INTERSECTIONS(1) |
						S_028804_STATIC_ANCHOR_ASSOCIATIONS(1));

		} else {
			r600_pipe_state_add_reg(rstate, R_028C00_PA_SC_LINE_CNTL, S_028C00_LAST_PIXEL(1));
			r600_pipe_state_add_reg(rstate, R_028C04_PA_SC_AA_CONFIG, 0);
		}
	}

	free(rctx->states[R600_PIPE_STATE_FRAMEBUFFER]);
	rctx->states[R600_PIPE_STATE_FRAMEBUFFER] = rstate;
	r600_context_pipe_state_set(rctx, rstate);

	if (state->zsbuf) {
		evergreen_polygon_offset_update(rctx);
	}

	if (rctx->cb_misc_state.nr_cbufs != state->nr_cbufs) {
		rctx->cb_misc_state.nr_cbufs = state->nr_cbufs;
		r600_atom_dirty(rctx, &rctx->cb_misc_state.atom);
	}

	if (state->nr_cbufs == 0 && rctx->alphatest_state.bypass) {
		rctx->alphatest_state.bypass = false;
		r600_atom_dirty(rctx, &rctx->alphatest_state.atom);
	}

	if (rctx->chip_class == CAYMAN && rctx->db_misc_state.log_samples != log_samples) {
		rctx->db_misc_state.log_samples = log_samples;
		r600_atom_dirty(rctx, &rctx->db_misc_state.atom);
	}
}

static void evergreen_emit_cb_misc_state(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	struct r600_cb_misc_state *a = (struct r600_cb_misc_state*)atom;
	unsigned fb_colormask = (1ULL << ((unsigned)a->nr_cbufs * 4)) - 1;
	unsigned ps_colormask = (1ULL << ((unsigned)a->nr_ps_color_outputs * 4)) - 1;

	r600_write_context_reg_seq(cs, R_028238_CB_TARGET_MASK, 2);
	r600_write_value(cs, a->blend_colormask & fb_colormask); /* R_028238_CB_TARGET_MASK */
	/* Always enable the first colorbuffer in CB_SHADER_MASK. This
	 * will assure that the alpha-test will work even if there is
	 * no colorbuffer bound. */
	r600_write_value(cs, 0xf | (a->dual_src_blend ? ps_colormask : 0) | fb_colormask); /* R_02823C_CB_SHADER_MASK */
}

static void evergreen_emit_db_misc_state(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	struct r600_db_misc_state *a = (struct r600_db_misc_state*)atom;
	unsigned db_render_control = 0;
	unsigned db_count_control = 0;
	unsigned db_render_override =
		S_02800C_FORCE_HIZ_ENABLE(V_02800C_FORCE_DISABLE) |
		S_02800C_FORCE_HIS_ENABLE0(V_02800C_FORCE_DISABLE) |
		S_02800C_FORCE_HIS_ENABLE1(V_02800C_FORCE_DISABLE);

	if (a->occlusion_query_enabled) {
		db_count_control |= S_028004_PERFECT_ZPASS_COUNTS(1);
		if (rctx->chip_class == CAYMAN) {
			db_count_control |= S_028004_SAMPLE_RATE(a->log_samples);
		}
		db_render_override |= S_02800C_NOOP_CULL_DISABLE(1);
	}

	if (a->flush_depthstencil_through_cb) {
		assert(a->copy_depth || a->copy_stencil);

		db_render_control |= S_028000_DEPTH_COPY_ENABLE(a->copy_depth) |
				     S_028000_STENCIL_COPY_ENABLE(a->copy_stencil) |
				     S_028000_COPY_CENTROID(1) |
				     S_028000_COPY_SAMPLE(a->copy_sample);
	}

	r600_write_context_reg_seq(cs, R_028000_DB_RENDER_CONTROL, 2);
	r600_write_value(cs, db_render_control); /* R_028000_DB_RENDER_CONTROL */
	r600_write_value(cs, db_count_control); /* R_028004_DB_COUNT_CONTROL */
	r600_write_context_reg(cs, R_02800C_DB_RENDER_OVERRIDE, db_render_override);
}

static void evergreen_emit_vertex_buffers(struct r600_context *rctx,
					  struct r600_vertexbuf_state *state,
					  unsigned resource_offset,
					  unsigned pkt_flags)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	uint32_t dirty_mask = state->dirty_mask;

	while (dirty_mask) {
		struct pipe_vertex_buffer *vb;
		struct r600_resource *rbuffer;
		uint64_t va;
		unsigned buffer_index = u_bit_scan(&dirty_mask);

		vb = &state->vb[buffer_index];
		rbuffer = (struct r600_resource*)vb->buffer;
		assert(rbuffer);

		va = r600_resource_va(&rctx->screen->screen, &rbuffer->b.b);
		va += vb->buffer_offset;

		/* fetch resources start at index 992 */
		r600_write_value(cs, PKT3(PKT3_SET_RESOURCE, 8, 0) | pkt_flags);
		r600_write_value(cs, (resource_offset + buffer_index) * 8);
		r600_write_value(cs, va); /* RESOURCEi_WORD0 */
		r600_write_value(cs, rbuffer->buf->size - vb->buffer_offset - 1); /* RESOURCEi_WORD1 */
		r600_write_value(cs, /* RESOURCEi_WORD2 */
				 S_030008_ENDIAN_SWAP(r600_endian_swap(32)) |
				 S_030008_STRIDE(vb->stride) |
				 S_030008_BASE_ADDRESS_HI(va >> 32UL));
		r600_write_value(cs, /* RESOURCEi_WORD3 */
				 S_03000C_DST_SEL_X(V_03000C_SQ_SEL_X) |
				 S_03000C_DST_SEL_Y(V_03000C_SQ_SEL_Y) |
				 S_03000C_DST_SEL_Z(V_03000C_SQ_SEL_Z) |
				 S_03000C_DST_SEL_W(V_03000C_SQ_SEL_W));
		r600_write_value(cs, 0); /* RESOURCEi_WORD4 */
		r600_write_value(cs, 0); /* RESOURCEi_WORD5 */
		r600_write_value(cs, 0); /* RESOURCEi_WORD6 */
		r600_write_value(cs, 0xc0000000); /* RESOURCEi_WORD7 */

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0) | pkt_flags);
		r600_write_value(cs, r600_context_bo_reloc(rctx, rbuffer, RADEON_USAGE_READ));
	}
	state->dirty_mask = 0;
}

static void evergreen_fs_emit_vertex_buffers(struct r600_context *rctx, struct r600_atom * atom)
{
	evergreen_emit_vertex_buffers(rctx, &rctx->vertex_buffer_state, 992, 0);
}

static void evergreen_cs_emit_vertex_buffers(struct r600_context *rctx, struct r600_atom * atom)
{
	evergreen_emit_vertex_buffers(rctx, &rctx->cs_vertex_buffer_state, 816,
				      RADEON_CP_PACKET3_COMPUTE_MODE);
}

static void evergreen_emit_constant_buffers(struct r600_context *rctx,
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
		uint64_t va;
		unsigned buffer_index = ffs(dirty_mask) - 1;

		cb = &state->cb[buffer_index];
		rbuffer = (struct r600_resource*)cb->buffer;
		assert(rbuffer);

		va = r600_resource_va(&rctx->screen->screen, &rbuffer->b.b);
		va += cb->buffer_offset;

		r600_write_context_reg(cs, reg_alu_constbuf_size + buffer_index * 4,
				       ALIGN_DIVUP(cb->buffer_size >> 4, 16));
		r600_write_context_reg(cs, reg_alu_const_cache + buffer_index * 4, va >> 8);

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0));
		r600_write_value(cs, r600_context_bo_reloc(rctx, rbuffer, RADEON_USAGE_READ));

		r600_write_value(cs, PKT3(PKT3_SET_RESOURCE, 8, 0));
		r600_write_value(cs, (buffer_id_base + buffer_index) * 8);
		r600_write_value(cs, va); /* RESOURCEi_WORD0 */
		r600_write_value(cs, rbuffer->buf->size - cb->buffer_offset - 1); /* RESOURCEi_WORD1 */
		r600_write_value(cs, /* RESOURCEi_WORD2 */
				 S_030008_ENDIAN_SWAP(r600_endian_swap(32)) |
				 S_030008_STRIDE(16) |
				 S_030008_BASE_ADDRESS_HI(va >> 32UL));
		r600_write_value(cs, /* RESOURCEi_WORD3 */
				 S_03000C_DST_SEL_X(V_03000C_SQ_SEL_X) |
				 S_03000C_DST_SEL_Y(V_03000C_SQ_SEL_Y) |
				 S_03000C_DST_SEL_Z(V_03000C_SQ_SEL_Z) |
				 S_03000C_DST_SEL_W(V_03000C_SQ_SEL_W));
		r600_write_value(cs, 0); /* RESOURCEi_WORD4 */
		r600_write_value(cs, 0); /* RESOURCEi_WORD5 */
		r600_write_value(cs, 0); /* RESOURCEi_WORD6 */
		r600_write_value(cs, 0xc0000000); /* RESOURCEi_WORD7 */

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0));
		r600_write_value(cs, r600_context_bo_reloc(rctx, rbuffer, RADEON_USAGE_READ));

		dirty_mask &= ~(1 << buffer_index);
	}
	state->dirty_mask = 0;
}

static void evergreen_emit_vs_constant_buffers(struct r600_context *rctx, struct r600_atom *atom)
{
	evergreen_emit_constant_buffers(rctx, &rctx->vs_constbuf_state, 176,
					R_028180_ALU_CONST_BUFFER_SIZE_VS_0,
					R_028980_ALU_CONST_CACHE_VS_0);
}

static void evergreen_emit_ps_constant_buffers(struct r600_context *rctx, struct r600_atom *atom)
{
	evergreen_emit_constant_buffers(rctx, &rctx->ps_constbuf_state, 0,
				       R_028140_ALU_CONST_BUFFER_SIZE_PS_0,
				       R_028940_ALU_CONST_CACHE_PS_0);
}

static void evergreen_emit_sampler_views(struct r600_context *rctx,
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

		r600_write_value(cs, PKT3(PKT3_SET_RESOURCE, 8, 0));
		r600_write_value(cs, (resource_id_base + resource_index) * 8);
		r600_write_array(cs, 8, rview->tex_resource_words);

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

static void evergreen_emit_vs_sampler_views(struct r600_context *rctx, struct r600_atom *atom)
{
	evergreen_emit_sampler_views(rctx, &rctx->vs_samplers.views, 176 + R600_MAX_CONST_BUFFERS);
}

static void evergreen_emit_ps_sampler_views(struct r600_context *rctx, struct r600_atom *atom)
{
	evergreen_emit_sampler_views(rctx, &rctx->ps_samplers.views, R600_MAX_CONST_BUFFERS);
}

static void evergreen_emit_sampler(struct r600_context *rctx,
				struct r600_textures_info *texinfo,
				unsigned resource_id_base,
				unsigned border_index_reg)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	unsigned i;

	for (i = 0; i < texinfo->n_samplers; i++) {

		if (texinfo->samplers[i] == NULL) {
			continue;
		}
		r600_write_value(cs, PKT3(PKT3_SET_SAMPLER, 3, 0));
		r600_write_value(cs, (resource_id_base + i) * 3);
		r600_write_array(cs, 3, texinfo->samplers[i]->tex_sampler_words);

		if (texinfo->samplers[i]->border_color_use) {
			r600_write_config_reg_seq(cs, border_index_reg, 5);
			r600_write_value(cs, i);
			r600_write_array(cs, 4, texinfo->samplers[i]->border_color);
		}
	}
}

static void evergreen_emit_vs_sampler(struct r600_context *rctx, struct r600_atom *atom)
{
	evergreen_emit_sampler(rctx, &rctx->vs_samplers, 18, R_00A414_TD_VS_SAMPLER0_BORDER_INDEX);
}

static void evergreen_emit_ps_sampler(struct r600_context *rctx, struct r600_atom *atom)
{
	evergreen_emit_sampler(rctx, &rctx->ps_samplers, 0, R_00A400_TD_PS_SAMPLER0_BORDER_INDEX);
}

static void evergreen_emit_sample_mask(struct r600_context *rctx, struct r600_atom *a)
{
	struct r600_sample_mask *s = (struct r600_sample_mask*)a;
	uint8_t mask = s->sample_mask;

	r600_write_context_reg(rctx->cs, R_028C3C_PA_SC_AA_MASK,
			       mask | (mask << 8) | (mask << 16) | (mask << 24));
}

static void cayman_emit_sample_mask(struct r600_context *rctx, struct r600_atom *a)
{
	struct r600_sample_mask *s = (struct r600_sample_mask*)a;
	struct radeon_winsys_cs *cs = rctx->cs;
	uint16_t mask = s->sample_mask;

	r600_write_context_reg_seq(cs, CM_R_028C38_PA_SC_AA_MASK_X0Y0_X1Y0, 2);
	r600_write_value(cs, mask | (mask << 16)); /* X0Y0_X1Y0 */
	r600_write_value(cs, mask | (mask << 16)); /* X0Y1_X1Y1 */
}

void evergreen_init_state_functions(struct r600_context *rctx)
{
	r600_init_atom(&rctx->cb_misc_state.atom, evergreen_emit_cb_misc_state, 0, 0);
	r600_atom_dirty(rctx, &rctx->cb_misc_state.atom);
	r600_init_atom(&rctx->db_misc_state.atom, evergreen_emit_db_misc_state, 7, 0);
	r600_atom_dirty(rctx, &rctx->db_misc_state.atom);
	r600_init_atom(&rctx->vertex_buffer_state.atom, evergreen_fs_emit_vertex_buffers, 0, 0);
	r600_init_atom(&rctx->cs_vertex_buffer_state.atom, evergreen_cs_emit_vertex_buffers, 0, 0);
	r600_init_atom(&rctx->vs_constbuf_state.atom, evergreen_emit_vs_constant_buffers, 0, 0);
	r600_init_atom(&rctx->ps_constbuf_state.atom, evergreen_emit_ps_constant_buffers, 0, 0);
	r600_init_atom(&rctx->vs_samplers.views.atom, evergreen_emit_vs_sampler_views, 0, 0);
	r600_init_atom(&rctx->ps_samplers.views.atom, evergreen_emit_ps_sampler_views, 0, 0);
	r600_init_atom(&rctx->cs_shader_state.atom, evergreen_emit_cs_shader, 0, 0);
	r600_init_atom(&rctx->vs_samplers.atom_sampler, evergreen_emit_vs_sampler, 0, 0);
	r600_init_atom(&rctx->ps_samplers.atom_sampler, evergreen_emit_ps_sampler, 0, 0);

	if (rctx->chip_class == EVERGREEN)
		r600_init_atom(&rctx->sample_mask.atom, evergreen_emit_sample_mask, 3, 0);
	else
		r600_init_atom(&rctx->sample_mask.atom, cayman_emit_sample_mask, 4, 0);
	rctx->sample_mask.sample_mask = ~0;
	r600_atom_dirty(rctx, &rctx->sample_mask.atom);

	rctx->context.create_blend_state = evergreen_create_blend_state;
	rctx->context.create_depth_stencil_alpha_state = evergreen_create_dsa_state;
	rctx->context.create_fs_state = r600_create_shader_state_ps;
	rctx->context.create_rasterizer_state = evergreen_create_rs_state;
	rctx->context.create_sampler_state = evergreen_create_sampler_state;
	rctx->context.create_sampler_view = evergreen_create_sampler_view;
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
	rctx->context.set_clip_state = evergreen_set_clip_state;
	rctx->context.set_constant_buffer = r600_set_constant_buffer;
	rctx->context.set_fragment_sampler_views = evergreen_set_ps_sampler_views;
	rctx->context.set_framebuffer_state = evergreen_set_framebuffer_state;
	rctx->context.set_polygon_stipple = evergreen_set_polygon_stipple;
	rctx->context.set_sample_mask = r600_set_sample_mask;
	rctx->context.set_scissor_state = evergreen_set_scissor_state;
	rctx->context.set_stencil_ref = r600_set_pipe_stencil_ref;
	rctx->context.set_vertex_buffers = r600_set_vertex_buffers;
	rctx->context.set_index_buffer = r600_set_index_buffer;
	rctx->context.set_vertex_sampler_views = evergreen_set_vs_sampler_views;
	rctx->context.set_viewport_state = evergreen_set_viewport_state;
	rctx->context.sampler_view_destroy = r600_sampler_view_destroy;
	rctx->context.texture_barrier = r600_texture_barrier;
	rctx->context.create_stream_output_target = r600_create_so_target;
	rctx->context.stream_output_target_destroy = r600_so_target_destroy;
	rctx->context.set_stream_output_targets = r600_set_so_targets;
	evergreen_init_compute_state_functions(rctx);
}

static void cayman_init_atom_start_cs(struct r600_context *rctx)
{
	struct r600_command_buffer *cb = &rctx->start_cs_cmd;

	r600_init_command_buffer(cb, 256, EMIT_EARLY);

	/* This must be first. */
	r600_store_value(cb, PKT3(PKT3_CONTEXT_CONTROL, 1, 0));
	r600_store_value(cb, 0x80000000);
	r600_store_value(cb, 0x80000000);

	r600_store_config_reg_seq(cb, R_008C00_SQ_CONFIG, 2);
	r600_store_value(cb, S_008C00_EXPORT_SRC_C(1)); /* R_008C00_SQ_CONFIG */
	/* always set the temp clauses */
	r600_store_value(cb, S_008C04_NUM_CLAUSE_TEMP_GPRS(4)); /* R_008C04_SQ_GPR_RESOURCE_MGMT_1 */

	r600_store_config_reg_seq(cb, R_008C10_SQ_GLOBAL_GPR_RESOURCE_MGMT_1, 2);
	r600_store_value(cb, 0); /* R_008C10_SQ_GLOBAL_GPR_RESOURCE_MGMT_1 */
	r600_store_value(cb, 0); /* R_008C14_SQ_GLOBAL_GPR_RESOURCE_MGMT_2 */

	r600_store_config_reg(cb, R_008D8C_SQ_DYN_GPR_CNTL_PS_FLUSH_REQ, (1 << 8));

	r600_store_context_reg(cb, R_028A4C_PA_SC_MODE_CNTL_1, 0);

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
	r600_store_value(cb, 0); /* R_028A40_VGT_GS_MODE */

	r600_store_context_reg_seq(cb, R_028B94_VGT_STRMOUT_CONFIG, 2);
	r600_store_value(cb, 0); /* R_028B94_VGT_STRMOUT_CONFIG */
	r600_store_value(cb, 0); /* R_028B98_VGT_STRMOUT_BUFFER_CONFIG */

	r600_store_context_reg_seq(cb, R_028AB4_VGT_REUSE_OFF, 2);
	r600_store_value(cb, 0); /* R_028AB4_VGT_REUSE_OFF */
	r600_store_value(cb, 0); /* R_028AB8_VGT_VTX_CNT_EN */

	r600_store_config_reg(cb, R_008A14_PA_CL_ENHANCE, (3 << 1) | 1);

	r600_store_context_reg(cb, CM_R_028AA8_IA_MULTI_VGT_PARAM, S_028AA8_SWITCH_ON_EOP(1) | S_028AA8_PARTIAL_VS_WAVE_ON(1) | S_028AA8_PRIMGROUP_SIZE(63));

	r600_store_context_reg_seq(cb, CM_R_028BD4_PA_SC_CENTROID_PRIORITY_0, 2);
	r600_store_value(cb, 0x76543210); /* CM_R_028BD4_PA_SC_CENTROID_PRIORITY_0 */
	r600_store_value(cb, 0xfedcba98); /* CM_R_028BD8_PA_SC_CENTROID_PRIORITY_1 */

	r600_store_context_reg_seq(cb, CM_R_0288E8_SQ_LDS_ALLOC, 2);
	r600_store_value(cb, 0); /* CM_R_0288E8_SQ_LDS_ALLOC */
	r600_store_value(cb, 0); /* R_0288EC_SQ_LDS_ALLOC_PS */

	r600_store_context_reg_seq(cb, R_028380_SQ_VTX_SEMANTIC_0, 34);
	r600_store_value(cb, 0); /* R_028380_SQ_VTX_SEMANTIC_0 */
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0); /* R_0283FC_SQ_VTX_SEMANTIC_31 */
	r600_store_value(cb, ~0); /* R_028400_VGT_MAX_VTX_INDX */
	r600_store_value(cb, 0); /* R_028404_VGT_MIN_VTX_INDX */

	r600_store_ctl_const(cb, R_03CFF0_SQ_VTX_BASE_VTX_LOC, 0);

	r600_store_context_reg_seq(cb, R_028028_DB_STENCIL_CLEAR, 2);
	r600_store_value(cb, 0); /* R_028028_DB_STENCIL_CLEAR */
	r600_store_value(cb, 0x3F800000); /* R_02802C_DB_DEPTH_CLEAR */

	r600_store_context_reg(cb, R_0286DC_SPI_FOG_CNTL, 0);

	r600_store_context_reg_seq(cb, R_028AC0_DB_SRESULTS_COMPARE_STATE0, 3);
	r600_store_value(cb, 0); /* R_028AC0_DB_SRESULTS_COMPARE_STATE0 */
	r600_store_value(cb, 0); /* R_028AC4_DB_SRESULTS_COMPARE_STATE1 */
	r600_store_value(cb, 0); /* R_028AC8_DB_PRELOAD_CONTROL */

	r600_store_context_reg(cb, R_028200_PA_SC_WINDOW_OFFSET, 0);
	r600_store_context_reg(cb, R_02820C_PA_SC_CLIPRECT_RULE, 0xFFFF);

	r600_store_context_reg_seq(cb, R_0282D0_PA_SC_VPORT_ZMIN_0, 2);
	r600_store_value(cb, 0); /* R_0282D0_PA_SC_VPORT_ZMIN_0 */
	r600_store_value(cb, 0x3F800000); /* R_0282D4_PA_SC_VPORT_ZMAX_0 */

	r600_store_context_reg(cb, R_028230_PA_SC_EDGERULE, 0xAAAAAAAA);
	r600_store_context_reg(cb, R_028818_PA_CL_VTE_CNTL, 0x0000043F);
	r600_store_context_reg(cb, R_028820_PA_CL_NANINF_CNTL, 0);

	r600_store_context_reg_seq(cb, CM_R_028BE8_PA_CL_GB_VERT_CLIP_ADJ, 4);
	r600_store_value(cb, 0x3F800000); /* CM_R_028BE8_PA_CL_GB_VERT_CLIP_ADJ */
	r600_store_value(cb, 0x3F800000); /* CM_R_028BEC_PA_CL_GB_VERT_DISC_ADJ */
	r600_store_value(cb, 0x3F800000); /* CM_R_028BF0_PA_CL_GB_HORZ_CLIP_ADJ */
	r600_store_value(cb, 0x3F800000); /* CM_R_028BF4_PA_CL_GB_HORZ_DISC_ADJ */

	r600_store_context_reg_seq(cb, R_028240_PA_SC_GENERIC_SCISSOR_TL, 2);
	r600_store_value(cb, 0); /* R_028240_PA_SC_GENERIC_SCISSOR_TL */
	r600_store_value(cb, S_028244_BR_X(16384) | S_028244_BR_Y(16384)); /* R_028244_PA_SC_GENERIC_SCISSOR_BR */

	r600_store_context_reg_seq(cb, R_028030_PA_SC_SCREEN_SCISSOR_TL, 2);
	r600_store_value(cb, 0); /* R_028030_PA_SC_SCREEN_SCISSOR_TL */
	r600_store_value(cb, S_028034_BR_X(16384) | S_028034_BR_Y(16384)); /* R_028034_PA_SC_SCREEN_SCISSOR_BR */

	r600_store_context_reg(cb, R_028848_SQ_PGM_RESOURCES_2_PS, S_028848_SINGLE_ROUND(V_SQ_ROUND_NEAREST_EVEN));
	r600_store_context_reg(cb, R_028864_SQ_PGM_RESOURCES_2_VS, S_028864_SINGLE_ROUND(V_SQ_ROUND_NEAREST_EVEN));
	r600_store_context_reg(cb, R_0288A8_SQ_PGM_RESOURCES_FS, 0);

	r600_store_context_reg(cb, R_028354_SX_SURFACE_SYNC, S_028354_SURFACE_SYNC_MASK(0xf));
	r600_store_context_reg(cb, R_028800_DB_DEPTH_CONTROL, 0);
	if (rctx->screen->has_streamout) {
		r600_store_context_reg(cb, R_028B28_VGT_STRMOUT_DRAW_OPAQUE_OFFSET, 0);
	}

	eg_store_loop_const(cb, R_03A200_SQ_LOOP_CONST_0, 0x01000FFF);
	eg_store_loop_const(cb, R_03A200_SQ_LOOP_CONST_0 + (32 * 4), 0x01000FFF);
}

void evergreen_init_common_regs(struct r600_command_buffer *cb,
	enum chip_class ctx_chip_class,
	enum radeon_family ctx_family,
	int ctx_drm_minor)
{
	int ps_prio;
	int vs_prio;
	int gs_prio;
	int es_prio;

	int hs_prio;
	int cs_prio;
	int ls_prio;

	int num_ps_gprs;
	int num_vs_gprs;
	int num_gs_gprs;
	int num_es_gprs;
	int num_hs_gprs;
	int num_ls_gprs;
	int num_temp_gprs;

	unsigned tmp;

	ps_prio = 0;
	vs_prio = 1;
	gs_prio = 2;
	es_prio = 3;
	hs_prio = 0;
	ls_prio = 0;
	cs_prio = 0;

	switch (ctx_family) {
	case CHIP_CEDAR:
	default:
		num_ps_gprs = 93;
		num_vs_gprs = 46;
		num_temp_gprs = 4;
		num_gs_gprs = 31;
		num_es_gprs = 31;
		num_hs_gprs = 23;
		num_ls_gprs = 23;
		break;
	case CHIP_REDWOOD:
		num_ps_gprs = 93;
		num_vs_gprs = 46;
		num_temp_gprs = 4;
		num_gs_gprs = 31;
		num_es_gprs = 31;
		num_hs_gprs = 23;
		num_ls_gprs = 23;
		break;
	case CHIP_JUNIPER:
		num_ps_gprs = 93;
		num_vs_gprs = 46;
		num_temp_gprs = 4;
		num_gs_gprs = 31;
		num_es_gprs = 31;
		num_hs_gprs = 23;
		num_ls_gprs = 23;
		break;
	case CHIP_CYPRESS:
	case CHIP_HEMLOCK:
		num_ps_gprs = 93;
		num_vs_gprs = 46;
		num_temp_gprs = 4;
		num_gs_gprs = 31;
		num_es_gprs = 31;
		num_hs_gprs = 23;
		num_ls_gprs = 23;
		break;
	case CHIP_PALM:
		num_ps_gprs = 93;
		num_vs_gprs = 46;
		num_temp_gprs = 4;
		num_gs_gprs = 31;
		num_es_gprs = 31;
		num_hs_gprs = 23;
		num_ls_gprs = 23;
		break;
	case CHIP_SUMO:
		num_ps_gprs = 93;
		num_vs_gprs = 46;
		num_temp_gprs = 4;
		num_gs_gprs = 31;
		num_es_gprs = 31;
		num_hs_gprs = 23;
		num_ls_gprs = 23;
		break;
	case CHIP_SUMO2:
		num_ps_gprs = 93;
		num_vs_gprs = 46;
		num_temp_gprs = 4;
		num_gs_gprs = 31;
		num_es_gprs = 31;
		num_hs_gprs = 23;
		num_ls_gprs = 23;
		break;
	case CHIP_BARTS:
		num_ps_gprs = 93;
		num_vs_gprs = 46;
		num_temp_gprs = 4;
		num_gs_gprs = 31;
		num_es_gprs = 31;
		num_hs_gprs = 23;
		num_ls_gprs = 23;
		break;
	case CHIP_TURKS:
		num_ps_gprs = 93;
		num_vs_gprs = 46;
		num_temp_gprs = 4;
		num_gs_gprs = 31;
		num_es_gprs = 31;
		num_hs_gprs = 23;
		num_ls_gprs = 23;
		break;
	case CHIP_CAICOS:
		num_ps_gprs = 93;
		num_vs_gprs = 46;
		num_temp_gprs = 4;
		num_gs_gprs = 31;
		num_es_gprs = 31;
		num_hs_gprs = 23;
		num_ls_gprs = 23;
		break;
	}

	tmp = 0;
	switch (ctx_family) {
	case CHIP_CEDAR:
	case CHIP_PALM:
	case CHIP_SUMO:
	case CHIP_SUMO2:
	case CHIP_CAICOS:
		break;
	default:
		tmp |= S_008C00_VC_ENABLE(1);
		break;
	}
	tmp |= S_008C00_EXPORT_SRC_C(1);
	tmp |= S_008C00_CS_PRIO(cs_prio);
	tmp |= S_008C00_LS_PRIO(ls_prio);
	tmp |= S_008C00_HS_PRIO(hs_prio);
	tmp |= S_008C00_PS_PRIO(ps_prio);
	tmp |= S_008C00_VS_PRIO(vs_prio);
	tmp |= S_008C00_GS_PRIO(gs_prio);
	tmp |= S_008C00_ES_PRIO(es_prio);

	/* enable dynamic GPR resource management */
	if (ctx_drm_minor >= 7) {
		r600_store_config_reg_seq(cb, R_008C00_SQ_CONFIG, 2);
		r600_store_value(cb, tmp); /* R_008C00_SQ_CONFIG */
		/* always set temp clauses */
		r600_store_value(cb, S_008C04_NUM_CLAUSE_TEMP_GPRS(num_temp_gprs)); /* R_008C04_SQ_GPR_RESOURCE_MGMT_1 */
		r600_store_config_reg_seq(cb, R_008C10_SQ_GLOBAL_GPR_RESOURCE_MGMT_1, 2);
		r600_store_value(cb, 0); /* R_008C10_SQ_GLOBAL_GPR_RESOURCE_MGMT_1 */
		r600_store_value(cb, 0); /* R_008C14_SQ_GLOBAL_GPR_RESOURCE_MGMT_2 */
		r600_store_config_reg(cb, R_008D8C_SQ_DYN_GPR_CNTL_PS_FLUSH_REQ, (1 << 8));
		r600_store_context_reg(cb, R_028838_SQ_DYN_GPR_RESOURCE_LIMIT_1,
					S_028838_PS_GPRS(0x1e) |
					S_028838_VS_GPRS(0x1e) |
					S_028838_GS_GPRS(0x1e) |
					S_028838_ES_GPRS(0x1e) |
					S_028838_HS_GPRS(0x1e) |
					S_028838_LS_GPRS(0x1e)); /* workaround for hw issues with dyn gpr - must set all limits to 240 instead of 0, 0x1e == 240 / 8*/
	} else {
		r600_store_config_reg_seq(cb, R_008C00_SQ_CONFIG, 4);
		r600_store_value(cb, tmp); /* R_008C00_SQ_CONFIG */

		tmp = S_008C04_NUM_PS_GPRS(num_ps_gprs);
		tmp |= S_008C04_NUM_VS_GPRS(num_vs_gprs);
		tmp |= S_008C04_NUM_CLAUSE_TEMP_GPRS(num_temp_gprs);
		r600_store_value(cb, tmp); /* R_008C04_SQ_GPR_RESOURCE_MGMT_1 */

		tmp = S_008C08_NUM_GS_GPRS(num_gs_gprs);
		tmp |= S_008C08_NUM_ES_GPRS(num_es_gprs);
		r600_store_value(cb, tmp); /* R_008C08_SQ_GPR_RESOURCE_MGMT_2 */

		tmp = S_008C0C_NUM_HS_GPRS(num_hs_gprs);
		tmp |= S_008C0C_NUM_HS_GPRS(num_ls_gprs);
		r600_store_value(cb, tmp); /* R_008C0C_SQ_GPR_RESOURCE_MGMT_3 */
	}

	r600_store_config_reg(cb, R_008E2C_SQ_LDS_RESOURCE_MGMT,
			      S_008E2C_NUM_PS_LDS(0x1000) | S_008E2C_NUM_LS_LDS(0x1000));

	r600_store_context_reg(cb, R_028A4C_PA_SC_MODE_CNTL_1, 0);

	r600_store_context_reg_seq(cb, R_028B94_VGT_STRMOUT_CONFIG, 2);
	r600_store_value(cb, 0); /* R_028B94_VGT_STRMOUT_CONFIG */
	r600_store_value(cb, 0); /* R_028B98_VGT_STRMOUT_BUFFER_CONFIG */

	r600_store_context_reg(cb, R_028230_PA_SC_EDGERULE, 0xAAAAAAAA);

	r600_store_context_reg_seq(cb, R_0282D0_PA_SC_VPORT_ZMIN_0, 2);
	r600_store_value(cb, 0); /* R_0282D0_PA_SC_VPORT_ZMIN_0 */
	r600_store_value(cb, 0x3F800000); /* R_0282D4_PA_SC_VPORT_ZMAX_0 */

	r600_store_context_reg_seq(cb, R_028AC0_DB_SRESULTS_COMPARE_STATE0, 3);
	r600_store_value(cb, 0); /* R_028AC0_DB_SRESULTS_COMPARE_STATE0 */
	r600_store_value(cb, 0); /* R_028AC4_DB_SRESULTS_COMPARE_STATE1 */
	r600_store_value(cb, 0); /* R_028AC8_DB_PRELOAD_CONTROL */

	r600_store_context_reg(cb, R_028848_SQ_PGM_RESOURCES_2_PS, S_028848_SINGLE_ROUND(V_SQ_ROUND_NEAREST_EVEN));
	r600_store_context_reg(cb, R_028864_SQ_PGM_RESOURCES_2_VS, S_028864_SINGLE_ROUND(V_SQ_ROUND_NEAREST_EVEN));

	r600_store_context_reg(cb, R_028354_SX_SURFACE_SYNC, S_028354_SURFACE_SYNC_MASK(0xf));

	return;
}

void evergreen_init_atom_start_cs(struct r600_context *rctx)
{
	struct r600_command_buffer *cb = &rctx->start_cs_cmd;
	int num_ps_threads;
	int num_vs_threads;
	int num_gs_threads;
	int num_es_threads;
	int num_hs_threads;
	int num_ls_threads;

	int num_ps_stack_entries;
	int num_vs_stack_entries;
	int num_gs_stack_entries;
	int num_es_stack_entries;
	int num_hs_stack_entries;
	int num_ls_stack_entries;
	enum radeon_family family;
	unsigned tmp;

	if (rctx->chip_class == CAYMAN) {
		cayman_init_atom_start_cs(rctx);
		return;
	}

	r600_init_command_buffer(cb, 256, EMIT_EARLY);

	/* This must be first. */
	r600_store_value(cb, PKT3(PKT3_CONTEXT_CONTROL, 1, 0));
	r600_store_value(cb, 0x80000000);
	r600_store_value(cb, 0x80000000);

	evergreen_init_common_regs(cb, rctx->chip_class
			, rctx->family, rctx->screen->info.drm_minor);

	family = rctx->family;
	switch (family) {
	case CHIP_CEDAR:
	default:
		num_ps_threads = 96;
		num_vs_threads = 16;
		num_gs_threads = 16;
		num_es_threads = 16;
		num_hs_threads = 16;
		num_ls_threads = 16;
		num_ps_stack_entries = 42;
		num_vs_stack_entries = 42;
		num_gs_stack_entries = 42;
		num_es_stack_entries = 42;
		num_hs_stack_entries = 42;
		num_ls_stack_entries = 42;
		break;
	case CHIP_REDWOOD:
		num_ps_threads = 128;
		num_vs_threads = 20;
		num_gs_threads = 20;
		num_es_threads = 20;
		num_hs_threads = 20;
		num_ls_threads = 20;
		num_ps_stack_entries = 42;
		num_vs_stack_entries = 42;
		num_gs_stack_entries = 42;
		num_es_stack_entries = 42;
		num_hs_stack_entries = 42;
		num_ls_stack_entries = 42;
		break;
	case CHIP_JUNIPER:
		num_ps_threads = 128;
		num_vs_threads = 20;
		num_gs_threads = 20;
		num_es_threads = 20;
		num_hs_threads = 20;
		num_ls_threads = 20;
		num_ps_stack_entries = 85;
		num_vs_stack_entries = 85;
		num_gs_stack_entries = 85;
		num_es_stack_entries = 85;
		num_hs_stack_entries = 85;
		num_ls_stack_entries = 85;
		break;
	case CHIP_CYPRESS:
	case CHIP_HEMLOCK:
		num_ps_threads = 128;
		num_vs_threads = 20;
		num_gs_threads = 20;
		num_es_threads = 20;
		num_hs_threads = 20;
		num_ls_threads = 20;
		num_ps_stack_entries = 85;
		num_vs_stack_entries = 85;
		num_gs_stack_entries = 85;
		num_es_stack_entries = 85;
		num_hs_stack_entries = 85;
		num_ls_stack_entries = 85;
		break;
	case CHIP_PALM:
		num_ps_threads = 96;
		num_vs_threads = 16;
		num_gs_threads = 16;
		num_es_threads = 16;
		num_hs_threads = 16;
		num_ls_threads = 16;
		num_ps_stack_entries = 42;
		num_vs_stack_entries = 42;
		num_gs_stack_entries = 42;
		num_es_stack_entries = 42;
		num_hs_stack_entries = 42;
		num_ls_stack_entries = 42;
		break;
	case CHIP_SUMO:
		num_ps_threads = 96;
		num_vs_threads = 25;
		num_gs_threads = 25;
		num_es_threads = 25;
		num_hs_threads = 25;
		num_ls_threads = 25;
		num_ps_stack_entries = 42;
		num_vs_stack_entries = 42;
		num_gs_stack_entries = 42;
		num_es_stack_entries = 42;
		num_hs_stack_entries = 42;
		num_ls_stack_entries = 42;
		break;
	case CHIP_SUMO2:
		num_ps_threads = 96;
		num_vs_threads = 25;
		num_gs_threads = 25;
		num_es_threads = 25;
		num_hs_threads = 25;
		num_ls_threads = 25;
		num_ps_stack_entries = 85;
		num_vs_stack_entries = 85;
		num_gs_stack_entries = 85;
		num_es_stack_entries = 85;
		num_hs_stack_entries = 85;
		num_ls_stack_entries = 85;
		break;
	case CHIP_BARTS:
		num_ps_threads = 128;
		num_vs_threads = 20;
		num_gs_threads = 20;
		num_es_threads = 20;
		num_hs_threads = 20;
		num_ls_threads = 20;
		num_ps_stack_entries = 85;
		num_vs_stack_entries = 85;
		num_gs_stack_entries = 85;
		num_es_stack_entries = 85;
		num_hs_stack_entries = 85;
		num_ls_stack_entries = 85;
		break;
	case CHIP_TURKS:
		num_ps_threads = 128;
		num_vs_threads = 20;
		num_gs_threads = 20;
		num_es_threads = 20;
		num_hs_threads = 20;
		num_ls_threads = 20;
		num_ps_stack_entries = 42;
		num_vs_stack_entries = 42;
		num_gs_stack_entries = 42;
		num_es_stack_entries = 42;
		num_hs_stack_entries = 42;
		num_ls_stack_entries = 42;
		break;
	case CHIP_CAICOS:
		num_ps_threads = 128;
		num_vs_threads = 10;
		num_gs_threads = 10;
		num_es_threads = 10;
		num_hs_threads = 10;
		num_ls_threads = 10;
		num_ps_stack_entries = 42;
		num_vs_stack_entries = 42;
		num_gs_stack_entries = 42;
		num_es_stack_entries = 42;
		num_hs_stack_entries = 42;
		num_ls_stack_entries = 42;
		break;
	}

	tmp = S_008C18_NUM_PS_THREADS(num_ps_threads);
	tmp |= S_008C18_NUM_VS_THREADS(num_vs_threads);
	tmp |= S_008C18_NUM_GS_THREADS(num_gs_threads);
	tmp |= S_008C18_NUM_ES_THREADS(num_es_threads);

	r600_store_config_reg_seq(cb, R_008C18_SQ_THREAD_RESOURCE_MGMT_1, 5);
	r600_store_value(cb, tmp); /* R_008C18_SQ_THREAD_RESOURCE_MGMT_1 */

	tmp = S_008C1C_NUM_HS_THREADS(num_hs_threads);
	tmp |= S_008C1C_NUM_LS_THREADS(num_ls_threads);
	r600_store_value(cb, tmp); /* R_008C1C_SQ_THREAD_RESOURCE_MGMT_2 */

	tmp = S_008C20_NUM_PS_STACK_ENTRIES(num_ps_stack_entries);
	tmp |= S_008C20_NUM_VS_STACK_ENTRIES(num_vs_stack_entries);
	r600_store_value(cb, tmp); /* R_008C20_SQ_STACK_RESOURCE_MGMT_1 */

	tmp = S_008C24_NUM_GS_STACK_ENTRIES(num_gs_stack_entries);
	tmp |= S_008C24_NUM_ES_STACK_ENTRIES(num_es_stack_entries);
	r600_store_value(cb, tmp); /* R_008C24_SQ_STACK_RESOURCE_MGMT_2 */

	tmp = S_008C28_NUM_HS_STACK_ENTRIES(num_hs_stack_entries);
	tmp |= S_008C28_NUM_LS_STACK_ENTRIES(num_ls_stack_entries);
	r600_store_value(cb, tmp); /* R_008C28_SQ_STACK_RESOURCE_MGMT_3 */

	r600_store_config_reg(cb, R_009100_SPI_CONFIG_CNTL, 0);
	r600_store_config_reg(cb, R_00913C_SPI_CONFIG_CNTL_1, S_00913C_VTX_DONE_DELAY(4));

	r600_store_context_reg_seq(cb, R_028900_SQ_ESGS_RING_ITEMSIZE, 6);
	r600_store_value(cb, 0); /* R_028900_SQ_ESGS_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_028904_SQ_GSVS_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_028908_SQ_ESTMP_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_02890C_SQ_GSTMP_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_028910_SQ_VSTMP_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_028914_SQ_PSTMP_RING_ITEMSIZE */

	r600_store_context_reg_seq(cb, R_02891C_SQ_GS_VERT_ITEMSIZE, 4);
	r600_store_value(cb, 0); /* R_02891C_SQ_GS_VERT_ITEMSIZE */
	r600_store_value(cb, 0); /* R_028920_SQ_GS_VERT_ITEMSIZE_1 */
	r600_store_value(cb, 0); /* R_028924_SQ_GS_VERT_ITEMSIZE_2 */
	r600_store_value(cb, 0); /* R_028928_SQ_GS_VERT_ITEMSIZE_3 */

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
	r600_store_value(cb, 0); /* R_028A40_VGT_GS_MODE */

	r600_store_context_reg_seq(cb, R_028AB4_VGT_REUSE_OFF, 2);
	r600_store_value(cb, 0); /* R_028AB4_VGT_REUSE_OFF */
	r600_store_value(cb, 0); /* R_028AB8_VGT_VTX_CNT_EN */

	r600_store_config_reg(cb, R_008A14_PA_CL_ENHANCE, (3 << 1) | 1);

	r600_store_context_reg_seq(cb, R_028380_SQ_VTX_SEMANTIC_0, 34);
	r600_store_value(cb, 0); /* R_028380_SQ_VTX_SEMANTIC_0 */
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0); /* R_0283FC_SQ_VTX_SEMANTIC_31 */
	r600_store_value(cb, ~0); /* R_028400_VGT_MAX_VTX_INDX */
	r600_store_value(cb, 0); /* R_028404_VGT_MIN_VTX_INDX */

	r600_store_ctl_const(cb, R_03CFF0_SQ_VTX_BASE_VTX_LOC, 0);

	r600_store_context_reg_seq(cb, R_028028_DB_STENCIL_CLEAR, 2);
	r600_store_value(cb, 0); /* R_028028_DB_STENCIL_CLEAR */
	r600_store_value(cb, 0x3F800000); /* R_02802C_DB_DEPTH_CLEAR */

	r600_store_context_reg(cb, R_028200_PA_SC_WINDOW_OFFSET, 0);
	r600_store_context_reg(cb, R_02820C_PA_SC_CLIPRECT_RULE, 0xFFFF);

	r600_store_context_reg(cb, R_0286DC_SPI_FOG_CNTL, 0);
	r600_store_context_reg(cb, R_028818_PA_CL_VTE_CNTL, 0x0000043F);
	r600_store_context_reg(cb, R_028820_PA_CL_NANINF_CNTL, 0);

	r600_store_context_reg_seq(cb, R_028AC0_DB_SRESULTS_COMPARE_STATE0, 3);
	r600_store_value(cb, 0); /* R_028AC0_DB_SRESULTS_COMPARE_STATE0 */
	r600_store_value(cb, 0); /* R_028AC4_DB_SRESULTS_COMPARE_STATE1 */
	r600_store_value(cb, 0); /* R_028AC8_DB_PRELOAD_CONTROL */

	r600_store_context_reg_seq(cb, R_028C0C_PA_CL_GB_VERT_CLIP_ADJ, 4);
	r600_store_value(cb, 0x3F800000); /* R_028C0C_PA_CL_GB_VERT_CLIP_ADJ */
	r600_store_value(cb, 0x3F800000); /* R_028C10_PA_CL_GB_VERT_DISC_ADJ */
	r600_store_value(cb, 0x3F800000); /* R_028C14_PA_CL_GB_HORZ_CLIP_ADJ */
	r600_store_value(cb, 0x3F800000); /* R_028C18_PA_CL_GB_HORZ_DISC_ADJ */

	r600_store_context_reg_seq(cb, R_028240_PA_SC_GENERIC_SCISSOR_TL, 2);
	r600_store_value(cb, 0); /* R_028240_PA_SC_GENERIC_SCISSOR_TL */
	r600_store_value(cb, S_028244_BR_X(16384) | S_028244_BR_Y(16384)); /* R_028244_PA_SC_GENERIC_SCISSOR_BR */

	r600_store_context_reg_seq(cb, R_028030_PA_SC_SCREEN_SCISSOR_TL, 2);
	r600_store_value(cb, 0); /* R_028030_PA_SC_SCREEN_SCISSOR_TL */
	r600_store_value(cb, S_028034_BR_X(16384) | S_028034_BR_Y(16384)); /* R_028034_PA_SC_SCREEN_SCISSOR_BR */

	r600_store_context_reg(cb, R_0288A8_SQ_PGM_RESOURCES_FS, 0);

	r600_store_context_reg(cb, R_028800_DB_DEPTH_CONTROL, 0);
	if (rctx->screen->has_streamout) {
		r600_store_context_reg(cb, R_028B28_VGT_STRMOUT_DRAW_OPAQUE_OFFSET, 0);
	}

	eg_store_loop_const(cb, R_03A200_SQ_LOOP_CONST_0, 0x01000FFF);
	eg_store_loop_const(cb, R_03A200_SQ_LOOP_CONST_0 + (32 * 4), 0x01000FFF);
}

void evergreen_polygon_offset_update(struct r600_context *rctx)
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
			offset_db_fmt_cntl |= S_028B78_POLY_OFFSET_DB_IS_FLOAT_FMT(1);
			break;
		case PIPE_FORMAT_Z16_UNORM:
			depth = -16;
			offset_units *= 4.0f;
			break;
		default:
			return;
		}
		/* XXX some of those reg can be computed with cso */
		offset_db_fmt_cntl |= S_028B78_POLY_OFFSET_NEG_NUM_DB_BITS(depth);
		r600_pipe_state_add_reg(&state,
				R_028B80_PA_SU_POLY_OFFSET_FRONT_SCALE,
				fui(rctx->rasterizer->offset_scale));
		r600_pipe_state_add_reg(&state,
				R_028B84_PA_SU_POLY_OFFSET_FRONT_OFFSET,
				fui(offset_units));
		r600_pipe_state_add_reg(&state,
				R_028B88_PA_SU_POLY_OFFSET_BACK_SCALE,
				fui(rctx->rasterizer->offset_scale));
		r600_pipe_state_add_reg(&state,
				R_028B8C_PA_SU_POLY_OFFSET_BACK_OFFSET,
				fui(offset_units));
		r600_pipe_state_add_reg(&state,
				R_028B78_PA_SU_POLY_OFFSET_DB_FMT_CNTL,
				offset_db_fmt_cntl);
		r600_context_pipe_state_set(rctx, &state);
	}
}

void evergreen_pipe_shader_ps(struct pipe_context *ctx, struct r600_pipe_shader *shader)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_state *rstate = &shader->rstate;
	struct r600_shader *rshader = &shader->shader;
	unsigned i, exports_ps, num_cout, spi_ps_in_control_0, spi_input_z, spi_ps_in_control_1, db_shader_control;
	int pos_index = -1, face_index = -1;
	int ninterp = 0;
	boolean have_linear = FALSE, have_centroid = FALSE, have_perspective = FALSE;
	unsigned spi_baryc_cntl, sid, tmp, idx = 0;
	unsigned z_export = 0, stencil_export = 0;

	rstate->nregs = 0;

	db_shader_control = S_02880C_Z_ORDER(V_02880C_EARLY_Z_THEN_LATE_Z);
	for (i = 0; i < rshader->ninput; i++) {
		/* evergreen NUM_INTERP only contains values interpolated into the LDS,
		   POSITION goes via GPRs from the SC so isn't counted */
		if (rshader->input[i].name == TGSI_SEMANTIC_POSITION)
			pos_index = i;
		else if (rshader->input[i].name == TGSI_SEMANTIC_FACE)
			face_index = i;
		else {
			ninterp++;
			if (rshader->input[i].interpolate == TGSI_INTERPOLATE_LINEAR)
				have_linear = TRUE;
			if (rshader->input[i].interpolate == TGSI_INTERPOLATE_PERSPECTIVE)
				have_perspective = TRUE;
			if (rshader->input[i].centroid)
				have_centroid = TRUE;
		}

		sid = rshader->input[i].spi_sid;

		if (sid) {

			tmp = S_028644_SEMANTIC(sid);

			if (rshader->input[i].name == TGSI_SEMANTIC_POSITION ||
				rshader->input[i].interpolate == TGSI_INTERPOLATE_CONSTANT ||
				(rshader->input[i].interpolate == TGSI_INTERPOLATE_COLOR &&
					rctx->rasterizer && rctx->rasterizer->flatshade)) {
				tmp |= S_028644_FLAT_SHADE(1);
			}

			if (rshader->input[i].name == TGSI_SEMANTIC_GENERIC &&
					(rctx->sprite_coord_enable & (1 << rshader->input[i].sid))) {
				tmp |= S_028644_PT_SPRITE_TEX(1);
			}

			r600_pipe_state_add_reg(rstate, R_028644_SPI_PS_INPUT_CNTL_0 + idx * 4,
					tmp);

			idx++;
		}
	}

	for (i = 0; i < rshader->noutput; i++) {
		if (rshader->output[i].name == TGSI_SEMANTIC_POSITION)
			z_export = 1;
		if (rshader->output[i].name == TGSI_SEMANTIC_STENCIL)
			stencil_export = 1;
	}
	if (rshader->uses_kill)
		db_shader_control |= S_02880C_KILL_ENABLE(1);

	db_shader_control |= S_02880C_Z_EXPORT_ENABLE(z_export);
	db_shader_control |= S_02880C_STENCIL_EXPORT_ENABLE(stencil_export);

	exports_ps = 0;
	for (i = 0; i < rshader->noutput; i++) {
		if (rshader->output[i].name == TGSI_SEMANTIC_POSITION ||
		    rshader->output[i].name == TGSI_SEMANTIC_STENCIL)
			exports_ps |= 1;
	}

	num_cout = rshader->nr_ps_color_exports;

	exports_ps |= S_02884C_EXPORT_COLORS(num_cout);
	if (!exports_ps) {
		/* always at least export 1 component per pixel */
		exports_ps = 2;
	}
	shader->nr_ps_color_outputs = num_cout;
	if (ninterp == 0) {
		ninterp = 1;
		have_perspective = TRUE;
	}

	if (!have_perspective && !have_linear)
		have_perspective = TRUE;

	spi_ps_in_control_0 = S_0286CC_NUM_INTERP(ninterp) |
		              S_0286CC_PERSP_GRADIENT_ENA(have_perspective) |
		              S_0286CC_LINEAR_GRADIENT_ENA(have_linear);
	spi_input_z = 0;
	if (pos_index != -1) {
		spi_ps_in_control_0 |=  S_0286CC_POSITION_ENA(1) |
			S_0286CC_POSITION_CENTROID(rshader->input[pos_index].centroid) |
			S_0286CC_POSITION_ADDR(rshader->input[pos_index].gpr);
		spi_input_z |= 1;
	}

	spi_ps_in_control_1 = 0;
	if (face_index != -1) {
		spi_ps_in_control_1 |= S_0286D0_FRONT_FACE_ENA(1) |
			S_0286D0_FRONT_FACE_ADDR(rshader->input[face_index].gpr);
	}

	spi_baryc_cntl = 0;
	if (have_perspective)
		spi_baryc_cntl |= S_0286E0_PERSP_CENTER_ENA(1) |
				  S_0286E0_PERSP_CENTROID_ENA(have_centroid);
	if (have_linear)
		spi_baryc_cntl |= S_0286E0_LINEAR_CENTER_ENA(1) |
				  S_0286E0_LINEAR_CENTROID_ENA(have_centroid);

	r600_pipe_state_add_reg(rstate, R_0286CC_SPI_PS_IN_CONTROL_0,
				spi_ps_in_control_0);
	r600_pipe_state_add_reg(rstate, R_0286D0_SPI_PS_IN_CONTROL_1,
				spi_ps_in_control_1);
	r600_pipe_state_add_reg(rstate, R_0286E4_SPI_PS_IN_CONTROL_2,
				0);
	r600_pipe_state_add_reg(rstate, R_0286D8_SPI_INPUT_Z, spi_input_z);
	r600_pipe_state_add_reg(rstate,
				R_0286E0_SPI_BARYC_CNTL,
				spi_baryc_cntl);

	r600_pipe_state_add_reg_bo(rstate,
				R_028840_SQ_PGM_START_PS,
				r600_resource_va(ctx->screen, (void *)shader->bo) >> 8,
				shader->bo, RADEON_USAGE_READ);
	r600_pipe_state_add_reg(rstate,
				R_028844_SQ_PGM_RESOURCES_PS,
				S_028844_NUM_GPRS(rshader->bc.ngpr) |
				S_028844_PRIME_CACHE_ON_DRAW(1) |
				S_028844_STACK_SIZE(rshader->bc.nstack));
	r600_pipe_state_add_reg(rstate,
				R_02884C_SQ_PGM_EXPORTS_PS,
				exports_ps);

	shader->db_shader_control = db_shader_control;
	shader->ps_depth_export = z_export | stencil_export;

	shader->sprite_coord_enable = rctx->sprite_coord_enable;
	if (rctx->rasterizer)
		shader->flatshade = rctx->rasterizer->flatshade;
}

void evergreen_pipe_shader_vs(struct pipe_context *ctx, struct r600_pipe_shader *shader)
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
					R_02861C_SPI_VS_OUT_ID_0 + i * 4,
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
			R_028860_SQ_PGM_RESOURCES_VS,
			S_028860_NUM_GPRS(rshader->bc.ngpr) |
			S_028860_STACK_SIZE(rshader->bc.nstack));
	r600_pipe_state_add_reg_bo(rstate,
			R_02885C_SQ_PGM_START_VS,
			r600_resource_va(ctx->screen, (void *)shader->bo) >> 8,
			shader->bo, RADEON_USAGE_READ);

	shader->pa_cl_vs_out_cntl =
		S_02881C_VS_OUT_CCDIST0_VEC_ENA((rshader->clip_dist_write & 0x0F) != 0) |
		S_02881C_VS_OUT_CCDIST1_VEC_ENA((rshader->clip_dist_write & 0xF0) != 0) |
		S_02881C_VS_OUT_MISC_VEC_ENA(rshader->vs_out_misc_write) |
		S_02881C_USE_VTX_POINT_SIZE(rshader->vs_out_point_size);
}

void evergreen_fetch_shader(struct pipe_context *ctx,
			    struct r600_vertex_element *ve)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_state *rstate = &ve->rstate;
	rstate->id = R600_PIPE_STATE_FETCH_SHADER;
	rstate->nregs = 0;
	r600_pipe_state_add_reg_bo(rstate, R_0288A4_SQ_PGM_START_FS,
				r600_resource_va(ctx->screen, (void *)ve->fetch_shader) >> 8,
				ve->fetch_shader, RADEON_USAGE_READ);
}

void *evergreen_create_resolve_blend(struct r600_context *rctx)
{
	struct pipe_blend_state blend;
	struct r600_pipe_state *rstate;

	memset(&blend, 0, sizeof(blend));
	blend.independent_blend_enable = true;
	blend.rt[0].colormask = 0xf;
	rstate = evergreen_create_blend_state_mode(&rctx->context, &blend, V_028808_CB_RESOLVE);
	return rstate;
}

void *evergreen_create_decompress_blend(struct r600_context *rctx)
{
	struct pipe_blend_state blend;
	struct r600_pipe_state *rstate;

	memset(&blend, 0, sizeof(blend));
	blend.independent_blend_enable = true;
	blend.rt[0].colormask = 0xf;
	rstate = evergreen_create_blend_state_mode(&rctx->context, &blend, V_028808_CB_DECOMPRESS);
	return rstate;
}

void *evergreen_create_db_flush_dsa(struct r600_context *rctx)
{
	struct pipe_depth_stencil_alpha_state dsa = {{0}};

	return rctx->context.create_depth_stencil_alpha_state(&rctx->context, &dsa);
}

void evergreen_update_dual_export_state(struct r600_context * rctx)
{
	unsigned dual_export = rctx->export_16bpc && rctx->nr_cbufs &&
			!rctx->ps_shader->current->ps_depth_export;

	unsigned db_source_format = dual_export ? V_02880C_EXPORT_DB_TWO :
			V_02880C_EXPORT_DB_FULL;

	unsigned db_shader_control = rctx->ps_shader->current->db_shader_control |
			S_02880C_DUAL_EXPORT_ENABLE(dual_export) |
			S_02880C_DB_SOURCE_FORMAT(db_source_format) |
			S_02880C_ALPHA_TO_MASK_DISABLE(rctx->cb0_is_integer);

	if (db_shader_control != rctx->db_shader_control) {
		struct r600_pipe_state rstate;

		rctx->db_shader_control = db_shader_control;

		rstate.nregs = 0;
		r600_pipe_state_add_reg(&rstate, R_02880C_DB_SHADER_CONTROL, db_shader_control);
		r600_context_pipe_state_set(rctx, &rstate);
	}
}
