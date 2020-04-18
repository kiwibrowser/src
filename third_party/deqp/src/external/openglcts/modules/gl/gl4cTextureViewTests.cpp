/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ /*!
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/**
 * \file  gl4cTextureViewTests.cpp
 * \brief Implements conformance tests for "texture view" functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cTextureViewTests.hpp"
#include "deFloat16.h"
#include "deMath.h"
#include "gluContextInfo.hpp"
#include "glwFunctions.hpp"
#include "tcuFloat.hpp"
#include "tcuTestLog.hpp"
#include <algorithm>

/* Type definitions needed to handle GL_R11F_G11F_B10F internal format */
typedef tcu::Float<deUint32, 5, 5, 15, 0> Float10;
typedef tcu::Float<deUint32, 5, 6, 15, 0> Float11;

namespace gl4cts
{
using namespace TextureView;

/** Stores internalformat->view class associations */
const int internalformat_view_compatibility_array[] = {
	/*      [internalformat]                       [view class]        */
	GL_RGBA32F, VIEW_CLASS_128_BITS, GL_RGBA32UI, VIEW_CLASS_128_BITS, GL_RGBA32I, VIEW_CLASS_128_BITS, GL_RGB32F,
	VIEW_CLASS_96_BITS, GL_RGB32UI, VIEW_CLASS_96_BITS, GL_RGB32I, VIEW_CLASS_96_BITS, GL_RGBA16F, VIEW_CLASS_64_BITS,
	GL_RG32F, VIEW_CLASS_64_BITS, GL_RGBA16UI, VIEW_CLASS_64_BITS, GL_RG32UI, VIEW_CLASS_64_BITS, GL_RGBA16I,
	VIEW_CLASS_64_BITS, GL_RG32I, VIEW_CLASS_64_BITS, GL_RGBA16, VIEW_CLASS_64_BITS, GL_RGBA16_SNORM,
	VIEW_CLASS_64_BITS, GL_RGB16, VIEW_CLASS_48_BITS, GL_RGB16_SNORM, VIEW_CLASS_48_BITS, GL_RGB16F, VIEW_CLASS_48_BITS,
	GL_RGB16UI, VIEW_CLASS_48_BITS, GL_RGB16I, VIEW_CLASS_48_BITS, GL_RG16F, VIEW_CLASS_32_BITS, GL_R11F_G11F_B10F,
	VIEW_CLASS_32_BITS, GL_R32F, VIEW_CLASS_32_BITS, GL_RGB10_A2UI, VIEW_CLASS_32_BITS, GL_RGBA8UI, VIEW_CLASS_32_BITS,
	GL_RG16UI, VIEW_CLASS_32_BITS, GL_R32UI, VIEW_CLASS_32_BITS, GL_RGBA8I, VIEW_CLASS_32_BITS, GL_RG16I,
	VIEW_CLASS_32_BITS, GL_R32I, VIEW_CLASS_32_BITS, GL_RGB10_A2, VIEW_CLASS_32_BITS, GL_RGBA8, VIEW_CLASS_32_BITS,
	GL_RG16, VIEW_CLASS_32_BITS, GL_RGBA8_SNORM, VIEW_CLASS_32_BITS, GL_RG16_SNORM, VIEW_CLASS_32_BITS, GL_SRGB8_ALPHA8,
	VIEW_CLASS_32_BITS, GL_RGB9_E5, VIEW_CLASS_32_BITS, GL_RGB8, VIEW_CLASS_24_BITS, GL_RGB8_SNORM, VIEW_CLASS_24_BITS,
	GL_SRGB8, VIEW_CLASS_24_BITS, GL_RGB8UI, VIEW_CLASS_24_BITS, GL_RGB8I, VIEW_CLASS_24_BITS, GL_R16F,
	VIEW_CLASS_16_BITS, GL_RG8UI, VIEW_CLASS_16_BITS, GL_R16UI, VIEW_CLASS_16_BITS, GL_RG8I, VIEW_CLASS_16_BITS,
	GL_R16I, VIEW_CLASS_16_BITS, GL_RG8, VIEW_CLASS_16_BITS, GL_R16, VIEW_CLASS_16_BITS, GL_RG8_SNORM,
	VIEW_CLASS_16_BITS, GL_R16_SNORM, VIEW_CLASS_16_BITS, GL_R8UI, VIEW_CLASS_8_BITS, GL_R8I, VIEW_CLASS_8_BITS, GL_R8,
	VIEW_CLASS_8_BITS, GL_R8_SNORM, VIEW_CLASS_8_BITS,

	/* Compressed texture formats. */
	GL_COMPRESSED_RED_RGTC1, VIEW_CLASS_RGTC1_RED, GL_COMPRESSED_SIGNED_RED_RGTC1, VIEW_CLASS_RGTC1_RED,
	GL_COMPRESSED_RG_RGTC2, VIEW_CLASS_RGTC2_RG, GL_COMPRESSED_SIGNED_RG_RGTC2, VIEW_CLASS_RGTC2_RG,
	GL_COMPRESSED_RGBA_BPTC_UNORM, VIEW_CLASS_BPTC_UNORM, GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM, VIEW_CLASS_BPTC_UNORM,
	GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT, VIEW_CLASS_BPTC_FLOAT, GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,
	VIEW_CLASS_BPTC_FLOAT
};

const int n_internalformat_view_compatibility_array_entries =
	sizeof(internalformat_view_compatibility_array) / sizeof(internalformat_view_compatibility_array[0]);

/** Stores all internalformats valid in OpenGL 4.x. Information whether particular internalformat
 *  can be considered supported can be retrieved by calling TextureViewTests::isInternalformatSupported()
 *  function.
 */
const glw::GLenum valid_gl_internalformats[] = {
	/* Section 8.5.1 */
	GL_RGBA32F,		   /* >= GL 4.0 */
	GL_RGBA32I,		   /* >= GL 4.0 */
	GL_RGBA32UI,	   /* >= GL 4.0 */
	GL_RGBA16,		   /* >= GL 4.0 */
	GL_RGBA16F,		   /* >= GL 4.0 */
	GL_RGBA16I,		   /* >= GL 4.0 */
	GL_RGBA16UI,	   /* >= GL 4.0 */
	GL_RGBA8,		   /* >= GL 4.0 */
	GL_RGBA8I,		   /* >= GL 4.0 */
	GL_RGBA8UI,		   /* >= GL 4.0 */
	GL_SRGB8_ALPHA8,   /* >= GL 4.0 */
	GL_RGB10_A2,	   /* >= GL 4.0 */
	GL_RGB10_A2UI,	 /* >= GL 4.0 */
	GL_RGB5_A1,		   /* >= GL 4.0 */
	GL_RGBA4,		   /* >= GL 4.0 */
	GL_R11F_G11F_B10F, /* >= GL 4.0 */
	GL_RGB565,		   /* >= GL 4.2 */
	GL_RG32F,		   /* >= GL 4.0 */
	GL_RG32I,		   /* >= GL 4.0 */
	GL_RG32UI,		   /* >= GL 4.0 */
	GL_RG16,		   /* >= GL 4.0 */
	GL_RG16F,		   /* >= GL 4.0 */
	GL_RG16I,		   /* >= GL 4.0 */
	GL_RG16UI,		   /* >= GL 4.0 */
	GL_RG8,			   /* >= GL 4.0 */
	GL_RG8I,		   /* >= GL 4.0 */
	GL_RG8UI,		   /* >= GL 4.0 */
	GL_R32F,		   /* >= GL 4.0 */
	GL_R32I,		   /* >= GL 4.0 */
	GL_R32UI,		   /* >= GL 4.0 */
	GL_R16F,		   /* >= GL 4.0 */
	GL_R16I,		   /* >= GL 4.0 */
	GL_R16UI,		   /* >= GL 4.0 */
	GL_R16,			   /* >= GL 4.0 */
	GL_R8,			   /* >= GL 4.0 */
	GL_R8I,			   /* >= GL 4.0 */
	GL_R8UI,		   /* >= GL 4.0 */
	GL_RGBA16_SNORM,   /* >= GL 4.0 */
	GL_RGBA8_SNORM,	/* >= GL 4.0 */
	GL_RGB32F,		   /* >= GL 4.0 */
	GL_RGB32I,		   /* >= GL 4.0 */
	GL_RGB32UI,		   /* >= GL 4.0 */
	GL_RGB16_SNORM,	/* >= GL 4.0 */
	GL_RGB16F,		   /* >= GL 4.0 */
	GL_RGB16I,		   /* >= GL 4.0 */
	GL_RGB16UI,		   /* >= GL 4.0 */
	GL_RGB16,		   /* >= GL 4.0 */
	GL_RGB8_SNORM,	 /* >= GL 4.0 */
	GL_RGB8,		   /* >= GL 4.0 */
	GL_RGB8I,		   /* >= GL 4.0 */
	GL_RGB8UI,		   /* >= GL 4.0 */
	GL_SRGB8,		   /* >= GL 4.0 */
	GL_RGB9_E5,		   /* >= GL 4.0 */
	GL_RG16_SNORM,	 /* >= GL 4.0 */
	GL_RG8_SNORM,	  /* >= GL 4.0 */
	GL_R16_SNORM,	  /* >= GL 4.0 */
	GL_R8_SNORM,	   /* >= GL 4.0 */

	GL_DEPTH_COMPONENT32F, /* >= GL 4.0 */
	GL_DEPTH_COMPONENT24,  /* >= GL 4.0 */
	GL_DEPTH_COMPONENT16,  /* >= GL 4.0 */

	GL_DEPTH32F_STENCIL8, /* >= GL 4.0 */
	GL_DEPTH24_STENCIL8,  /* >= GL 4.0 */

	/* Table 8.14: generic compressed internalformats have been removed */
	GL_COMPRESSED_RED_RGTC1,					  /* >= GL 4.0 */
	GL_COMPRESSED_SIGNED_RED_RGTC1,				  /* >= GL 4.0 */
	GL_COMPRESSED_RG_RGTC2,						  /* >= GL 4.0 */
	GL_COMPRESSED_SIGNED_RG_RGTC2,				  /* >= GL 4.0 */
	GL_COMPRESSED_RGBA_BPTC_UNORM,				  /* >= GL 4.2 */
	GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,		  /* >= GL 4.2 */
	GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,		  /* >= GL 4.2 */
	GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,		  /* >= GL 4.2 */
	GL_COMPRESSED_RGB8_ETC2,					  /* >= GL 4.3 */
	GL_COMPRESSED_SRGB8_ETC2,					  /* >= GL 4.3 */
	GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  /* >= GL 4.3 */
	GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, /* >= GL 4.3 */
	GL_COMPRESSED_RGBA8_ETC2_EAC,				  /* >= GL 4.3 */
	GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,		  /* >= GL 4.3 */
	GL_COMPRESSED_R11_EAC,						  /* >= GL 4.3 */
	GL_COMPRESSED_SIGNED_R11_EAC,				  /* >= GL 4.3 */
	GL_COMPRESSED_RG11_EAC,						  /* >= GL 4.3 */
	GL_COMPRESSED_SIGNED_RG11_EAC,				  /* >= GL 4.3 */
};

const int n_valid_gl_internalformats = sizeof(valid_gl_internalformats) / sizeof(valid_gl_internalformats[0]);

/** An array of texture targets that is used by a number of TextureViewUtilities methods. */
static glw::GLenum valid_texture_targets[] = { GL_TEXTURE_1D,
											   GL_TEXTURE_1D_ARRAY,
											   GL_TEXTURE_2D,
											   GL_TEXTURE_2D_ARRAY,
											   GL_TEXTURE_2D_MULTISAMPLE,
											   GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
											   GL_TEXTURE_3D,
											   GL_TEXTURE_BUFFER,
											   GL_TEXTURE_CUBE_MAP,
											   GL_TEXTURE_CUBE_MAP_ARRAY,
											   GL_TEXTURE_RECTANGLE };
const unsigned int n_valid_texture_targets = sizeof(valid_texture_targets) / sizeof(valid_texture_targets[0]);

/** Retrieves amount of components defined by user-specified internalformat.
 *
 *  This function throws TestError exception if @param internalformat is not recognized.
 *
 *  @param internalformat Internalformat to use for the query.
 *
 *  @return Requested value.
 **/
unsigned int TextureViewUtilities::getAmountOfComponentsForInternalformat(const glw::GLenum internalformat)
{
	unsigned int result = 0;

	switch (internalformat)
	{
	case GL_COMPRESSED_RGBA_BPTC_UNORM:
	case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
	case GL_RGB10_A2:
	case GL_RGB10_A2UI:
	case GL_RGB5_A1:
	case GL_RGBA16F:
	case GL_RGBA16I:
	case GL_RGBA16UI:
	case GL_RGBA16:
	case GL_RGBA16_SNORM:
	case GL_RGBA32F:
	case GL_RGBA32I:
	case GL_RGBA32UI:
	case GL_RGBA4:
	case GL_RGBA8I:
	case GL_RGBA8UI:
	case GL_RGBA8:
	case GL_RGBA8_SNORM:
	case GL_SRGB8_ALPHA8:
	{
		result = 4;

		break;
	}

	case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
	case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
	case GL_R11F_G11F_B10F:
	case GL_RGB16_SNORM:
	case GL_RGB16F:
	case GL_RGB16I:
	case GL_RGB16UI:
	case GL_RGB16:
	case GL_RGB32F:
	case GL_RGB32I:
	case GL_RGB32UI:
	case GL_RGB565:
	case GL_RGB8:
	case GL_RGB8_SNORM:
	case GL_RGB8I:
	case GL_RGB8UI:
	case GL_RGB9_E5:
	case GL_SRGB8:
	{
		result = 3;

		break;
	}

	case GL_COMPRESSED_RG_RGTC2:
	case GL_COMPRESSED_SIGNED_RG_RGTC2:
	case GL_RG16:
	case GL_RG16F:
	case GL_RG16I:
	case GL_RG16UI:
	case GL_RG16_SNORM:
	case GL_RG32F:
	case GL_RG32I:
	case GL_RG32UI:
	case GL_RG8:
	case GL_RG8_SNORM:
	case GL_RG8I:
	case GL_RG8UI:
	{
		result = 2;

		break;
	}

	case GL_COMPRESSED_RED_RGTC1:
	case GL_COMPRESSED_SIGNED_RED_RGTC1:
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH32F_STENCIL8: /* only one piece of information can be retrieved at a time */
	case GL_DEPTH24_STENCIL8:  /* only one piece of information can be retrieved at a time */
	case GL_R16:
	case GL_R16_SNORM:
	case GL_R16F:
	case GL_R16I:
	case GL_R16UI:
	case GL_R32F:
	case GL_R32I:
	case GL_R32UI:
	case GL_R8_SNORM:
	case GL_R8:
	case GL_R8I:
	case GL_R8UI:
	{
		result = 1;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized internalformat");
	}
	} /* switch (interalformat) */

	return result;
}

/** Retrieves block size used by user-specified compressed internalformat.
 *
 *  Throws TestError exception if @param internalformat is not recognized.
 *
 *  @param internalformat Compressed internalformat to use for the query.
 *
 *  @return Requested information (in bytes).
 **/
unsigned int TextureViewUtilities::getBlockSizeForCompressedInternalformat(const glw::GLenum internalformat)
{
	unsigned int result = 0;

	switch (internalformat)
	{
	case GL_COMPRESSED_RED_RGTC1:
	case GL_COMPRESSED_SIGNED_RED_RGTC1:
	{
		result = 8;

		break;
	}

	case GL_COMPRESSED_RG_RGTC2:
	case GL_COMPRESSED_SIGNED_RG_RGTC2:
	case GL_COMPRESSED_RGBA_BPTC_UNORM:
	case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
	case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
	case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
	{
		result = 16;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized internalformat");
	}
	} /* switch (internalformat) */

	return result;
}

/** Retrieves amount of bits used for R/G/B/A components by user-specified
 *  *non-compressed* internalformat.
 *
 *  Throws TestError exception if @param internalformat is not recognized.
 *
 *  @param internalformat Internalformat to use for the query. Must not describe
 *                        compressed internalformat.
 *  @param out_rgba_size  Must be spacious enough to hold 4 ints. Deref will be
 *                        used to store requested information for R/G/B/A channels.
 *                        Must not be NULL.
 **/
void TextureViewUtilities::getComponentSizeForInternalformat(const glw::GLenum internalformat,
															 unsigned int*	 out_rgba_size)
{
	/* Note: Compressed textures are not supported by this function */

	/* Reset all the values before we continue. */
	memset(out_rgba_size, 0, 4 /* rgba */ * sizeof(unsigned int));

	/* Depending on the user-specified internalformat, update relevant arguments */
	switch (internalformat)
	{
	case GL_RGBA32F:
	case GL_RGBA32I:
	case GL_RGBA32UI:
	{
		out_rgba_size[0] = 32;
		out_rgba_size[1] = 32;
		out_rgba_size[2] = 32;
		out_rgba_size[3] = 32;

		break;
	}

	case GL_RGBA16F:
	case GL_RGBA16I:
	case GL_RGBA16UI:
	case GL_RGBA16:
	case GL_RGBA16_SNORM:
	{
		out_rgba_size[0] = 16;
		out_rgba_size[1] = 16;
		out_rgba_size[2] = 16;
		out_rgba_size[3] = 16;

		break;
	}

	case GL_RGBA8I:
	case GL_RGBA8UI:
	case GL_RGBA8:
	case GL_RGBA8_SNORM:
	case GL_SRGB8_ALPHA8:
	{
		out_rgba_size[0] = 8;
		out_rgba_size[1] = 8;
		out_rgba_size[2] = 8;
		out_rgba_size[3] = 8;

		break;
	}

	case GL_RGB10_A2:
	case GL_RGB10_A2UI:
	{
		out_rgba_size[0] = 10;
		out_rgba_size[1] = 10;
		out_rgba_size[2] = 10;
		out_rgba_size[3] = 2;

		break;
	}

	case GL_RGB5_A1:
	{
		out_rgba_size[0] = 5;
		out_rgba_size[1] = 5;
		out_rgba_size[2] = 5;
		out_rgba_size[3] = 1;

		break;
	}

	case GL_RGBA4:
	{
		out_rgba_size[0] = 4;
		out_rgba_size[1] = 4;
		out_rgba_size[2] = 4;
		out_rgba_size[3] = 4;

		break;
	}

	case GL_RGB9_E5:
	{
		out_rgba_size[0] = 9;
		out_rgba_size[1] = 9;
		out_rgba_size[2] = 9;
		out_rgba_size[3] = 5;

		break;
	}

	case GL_R11F_G11F_B10F:
	{
		out_rgba_size[0] = 11;
		out_rgba_size[1] = 11;
		out_rgba_size[2] = 10;

		break;
	}

	case GL_RGB565:
	{
		out_rgba_size[0] = 5;
		out_rgba_size[1] = 6;
		out_rgba_size[2] = 5;

		break;
	}

	case GL_RGB32F:
	case GL_RGB32I:
	case GL_RGB32UI:
	{
		out_rgba_size[0] = 32;
		out_rgba_size[1] = 32;
		out_rgba_size[2] = 32;

		break;
	}

	case GL_RGB16_SNORM:
	case GL_RGB16F:
	case GL_RGB16I:
	case GL_RGB16UI:
	case GL_RGB16:
	{
		out_rgba_size[0] = 16;
		out_rgba_size[1] = 16;
		out_rgba_size[2] = 16;

		break;
	}

	case GL_RGB8_SNORM:
	case GL_RGB8:
	case GL_RGB8I:
	case GL_RGB8UI:
	case GL_SRGB8:
	{
		out_rgba_size[0] = 8;
		out_rgba_size[1] = 8;
		out_rgba_size[2] = 8;

		break;
	}

	case GL_RG32F:
	case GL_RG32I:
	case GL_RG32UI:
	{
		out_rgba_size[0] = 32;
		out_rgba_size[1] = 32;

		break;
	}

	case GL_RG16:
	case GL_RG16F:
	case GL_RG16I:
	case GL_RG16UI:
	case GL_RG16_SNORM:
	{
		out_rgba_size[0] = 16;
		out_rgba_size[1] = 16;

		break;
	}

	case GL_RG8:
	case GL_RG8I:
	case GL_RG8UI:
	case GL_RG8_SNORM:
	{
		out_rgba_size[0] = 8;
		out_rgba_size[1] = 8;

		break;
	}

	case GL_R32F:
	case GL_R32I:
	case GL_R32UI:
	{
		out_rgba_size[0] = 32;

		break;
	}

	case GL_R16F:
	case GL_R16I:
	case GL_R16UI:
	case GL_R16:
	case GL_R16_SNORM:
	case GL_DEPTH_COMPONENT16:
	{
		out_rgba_size[0] = 16;

		break;
	}

	case GL_R8:
	case GL_R8I:
	case GL_R8UI:
	case GL_R8_SNORM:
	{
		out_rgba_size[0] = 8;

		break;
	}

	case GL_DEPTH_COMPONENT24:
	{
		out_rgba_size[0] = 24;

		break;
	}

	case GL_DEPTH32F_STENCIL8:
	{
		out_rgba_size[0] = 32;
		out_rgba_size[1] = 8;

		break;
	}

	case GL_DEPTH24_STENCIL8:
	{
		out_rgba_size[0] = 24;
		out_rgba_size[1] = 8;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized internalformat");
	}
	} /* switch (interalformat) */
}

/** Tells how many bits per components should be used to define input data with
 *  user-specified type.
 *
 *  Throws TestError exception if @param type is not recognized.
 *
 *  @param type          Type to use for the query.
 *  @param out_rgba_size Deref will be used to store requested information. Must
 *                       not be NULL. Must be capacious enough to hold 4 ints.
 *
 **/
void TextureViewUtilities::getComponentSizeForType(const glw::GLenum type, unsigned int* out_rgba_size)
{
	memset(out_rgba_size, 0, sizeof(unsigned int) * 4 /* rgba */);

	switch (type)
	{
	case GL_BYTE:
	case GL_UNSIGNED_BYTE:
	{
		out_rgba_size[0] = 8;
		out_rgba_size[1] = 8;
		out_rgba_size[2] = 8;
		out_rgba_size[3] = 8;

		break;
	}

	case GL_FLOAT:
	case GL_UNSIGNED_INT:
	case GL_INT:
	{
		out_rgba_size[0] = 32;
		out_rgba_size[1] = 32;
		out_rgba_size[2] = 32;
		out_rgba_size[3] = 32;

		break;
	}

	case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
	{
		out_rgba_size[0] = 8;
		out_rgba_size[1] = 24;
		out_rgba_size[2] = 32;
		out_rgba_size[3] = 0;

		break;
	}

	case GL_HALF_FLOAT:
	case GL_UNSIGNED_SHORT:
	case GL_SHORT:
	{
		out_rgba_size[0] = 16;
		out_rgba_size[1] = 16;
		out_rgba_size[2] = 16;
		out_rgba_size[3] = 16;

		break;
	}

	case GL_UNSIGNED_INT_10_10_10_2:
	{
		out_rgba_size[0] = 10;
		out_rgba_size[1] = 10;
		out_rgba_size[2] = 10;
		out_rgba_size[3] = 2;

		break;
	}

	case GL_UNSIGNED_INT_10F_11F_11F_REV:
	{
		out_rgba_size[0] = 11;
		out_rgba_size[1] = 11;
		out_rgba_size[2] = 10;

		break;
	}

	case GL_UNSIGNED_INT_24_8:
	{
		out_rgba_size[0] = 24;
		out_rgba_size[1] = 8;
		out_rgba_size[2] = 0;
		out_rgba_size[3] = 0;

		break;
	}

	case GL_UNSIGNED_INT_2_10_10_10_REV:
	{
		out_rgba_size[0] = 10;
		out_rgba_size[1] = 10;
		out_rgba_size[2] = 10;
		out_rgba_size[3] = 2;

		break;
	}

	case GL_UNSIGNED_INT_5_9_9_9_REV:
	{
		out_rgba_size[0] = 9;
		out_rgba_size[1] = 9;
		out_rgba_size[2] = 9;
		out_rgba_size[3] = 5;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized type");
	}
	} /* switch (type) */
}

/** Returns strings naming GL error codes.
 *
 *  @param error_code GL error code.
 *
 *  @return Requested strings or "[?]" if @param error_code was not
 *          recognized.
 **/
const char* TextureViewUtilities::getErrorCodeString(const glw::GLint error_code)
{
	switch (error_code)
	{
	case GL_INVALID_ENUM:
		return "GL_INVALID_ENUM";
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return "GL_INVALID_FRAMEBUFFER_OPERATION";
	case GL_INVALID_OPERATION:
		return "GL_INVALID_OPERATION";
	case GL_INVALID_VALUE:
		return "GL_INVALID_VALUE";
	case GL_NO_ERROR:
		return "GL_NO_ERROR";
	case GL_OUT_OF_MEMORY:
		return "GL_OUT_OF_MEMORY";
	case GL_STACK_OVERFLOW:
		return "GL_STACK_OVERFLOW";
	case GL_STACK_UNDERFLOW:
		return "GL_STACK_UNDERFLOW";
	default:
		return "[?]";
	}
}

/** Tells what the format of user-specified internalformat is (eg. whether it's a FP,
 *  unorm, snorm, etc.). Note: this is NOT the GL-speak format.
 *
 *  Supports both compressed and non-compressed internalformats.
 *  Throws TestError exception if @param internalformat is not recognized.
 *
 *  @param internalformat Internalformat to use for the query.
 *
 *  @return Requested information.
 *
 **/
_format TextureViewUtilities::getFormatOfInternalformat(const glw::GLenum internalformat)
{
	_format result = FORMAT_UNDEFINED;

	switch (internalformat)
	{
	case GL_COMPRESSED_RG_RGTC2:
	case GL_COMPRESSED_RGBA_BPTC_UNORM:
	case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
	case GL_COMPRESSED_RED_RGTC1:
	case GL_RGBA16:
	case GL_RGBA4:
	case GL_RGBA8:
	case GL_RGB10_A2:
	case GL_RGB16:
	case GL_RGB5_A1:
	case GL_RGB565:
	case GL_RGB8:
	case GL_RG16:
	case GL_RG8:
	case GL_R16:
	case GL_R8:
	case GL_SRGB8:
	case GL_SRGB8_ALPHA8:
	{
		result = FORMAT_UNORM;

		break;
	}

	case GL_COMPRESSED_SIGNED_RED_RGTC1:
	case GL_COMPRESSED_SIGNED_RG_RGTC2:
	case GL_RGBA16_SNORM:
	case GL_RGBA8_SNORM:
	case GL_RGB16_SNORM:
	case GL_RGB8_SNORM:
	case GL_RG16_SNORM:
	case GL_RG8_SNORM:
	case GL_R16_SNORM:
	case GL_R8_SNORM:
	{
		result = FORMAT_SNORM;

		break;
	}

	case GL_RGB10_A2UI:
	case GL_RGBA16UI:
	case GL_RGBA32UI:
	case GL_RGBA8UI:
	case GL_RGB16UI:
	case GL_RGB32UI:
	case GL_RGB8UI:
	case GL_RG16UI:
	case GL_RG32UI:
	case GL_RG8UI:
	case GL_R16UI:
	case GL_R32UI:
	case GL_R8UI:
	{
		result = FORMAT_UNSIGNED_INTEGER;

		break;
	}

	case GL_RGB9_E5:
	{
		result = FORMAT_RGBE;

		break;
	}

	case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
	case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32F:
	case GL_R11F_G11F_B10F:
	case GL_RGBA16F:
	case GL_RGBA32F:
	case GL_RGB16F:
	case GL_RGB32F:
	case GL_RG16F:
	case GL_RG32F:
	case GL_R16F:
	case GL_R32F:
	{
		result = FORMAT_FLOAT;

		break;
	}

	case GL_RGBA16I:
	case GL_RGBA32I:
	case GL_RGBA8I:
	case GL_RGB16I:
	case GL_RGB32I:
	case GL_RGB8I:
	case GL_RG16I:
	case GL_RG32I:
	case GL_RG8I:
	case GL_R16I:
	case GL_R32I:
	case GL_R8I:
	{
		result = FORMAT_SIGNED_INTEGER;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized internalformat");
	}
	} /* switch (interalformat) */

	return result;
}

/** Returns GL format that is compatible with user-specified internalformat.
 *
 *  Throws TestError exception if @param internalformat is not recognized.
 *
 *  @param internalformat Internalformat to use for the query.
 *
 *  @return Requested information.
 **/
glw::GLenum TextureViewUtilities::getGLFormatOfInternalformat(const glw::GLenum internalformat)
{
	glw::GLenum result = FORMAT_UNDEFINED;

	switch (internalformat)
	{
	case GL_COMPRESSED_RGBA_BPTC_UNORM:
	case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
	case GL_COMPRESSED_RGBA8_ETC2_EAC:
	case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
	case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
	case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
	{
		result = GL_COMPRESSED_RGBA;

		break;
	}

	case GL_RGB10_A2:
	case GL_RGB5_A1:
	case GL_RGBA16:
	case GL_RGBA16F:
	case GL_RGBA16_SNORM:
	case GL_RGBA32F:
	case GL_RGBA4:
	case GL_RGBA8:
	case GL_RGBA8_SNORM:
	case GL_SRGB8_ALPHA8:
	{
		result = GL_RGBA;

		break;
	}

	case GL_RGB10_A2UI:
	case GL_RGBA16I:
	case GL_RGBA16UI:
	case GL_RGBA32I:
	case GL_RGBA32UI:
	case GL_RGBA8I:
	case GL_RGBA8UI:
	{
		result = GL_RGBA_INTEGER;

		break;
	}

	case GL_COMPRESSED_RGB8_ETC2:
	case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
	case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
	case GL_COMPRESSED_SRGB8_ETC2:
	{
		result = GL_COMPRESSED_RGB;

		break;
	}

	case GL_R11F_G11F_B10F:
	case GL_RGB16:
	case GL_RGB16_SNORM:
	case GL_RGB16F:
	case GL_RGB32F:
	case GL_RGB565:
	case GL_RGB8:
	case GL_RGB8_SNORM:
	case GL_RGB9_E5:
	case GL_SRGB8:
	{
		result = GL_RGB;

		break;
	}

	case GL_RGB16I:
	case GL_RGB16UI:
	case GL_RGB32I:
	case GL_RGB32UI:
	case GL_RGB8I:
	case GL_RGB8UI:
	{
		result = GL_RGB_INTEGER;

		break;
	}

	case GL_COMPRESSED_RG_RGTC2:
	case GL_COMPRESSED_RG11_EAC:
	case GL_COMPRESSED_SIGNED_RG_RGTC2:
	case GL_COMPRESSED_SIGNED_RG11_EAC:
	{
		result = GL_COMPRESSED_RG;

		break;
	}

	case GL_RG16:
	case GL_RG16_SNORM:
	case GL_RG16F:
	case GL_RG32F:
	case GL_RG8:
	case GL_RG8_SNORM:
	{
		result = GL_RG;

		break;
	}

	case GL_RG16I:
	case GL_RG16UI:
	case GL_RG32I:
	case GL_RG32UI:
	case GL_RG8I:
	case GL_RG8UI:
	{
		result = GL_RG_INTEGER;

		break;
	}

	case GL_COMPRESSED_R11_EAC:
	case GL_COMPRESSED_RED_RGTC1:
	case GL_COMPRESSED_SIGNED_R11_EAC:
	case GL_COMPRESSED_SIGNED_RED_RGTC1:
	{
		result = GL_COMPRESSED_RED;

		break;
	}

	case GL_R16:
	case GL_R16F:
	case GL_R16_SNORM:
	case GL_R32F:
	case GL_R8:
	case GL_R8_SNORM:
	{
		result = GL_RED;

		break;
	}

	case GL_R16I:
	case GL_R16UI:
	case GL_R32I:
	case GL_R32UI:
	case GL_R8I:
	case GL_R8UI:
	{
		result = GL_RED_INTEGER;

		break;
	}

	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32F:
	{
		result = GL_DEPTH_COMPONENT;

		break;
	}

	case GL_DEPTH24_STENCIL8:
	case GL_DEPTH32F_STENCIL8:
	{
		result = GL_DEPTH_STENCIL;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized internalformat");
	}
	} /* switch (internalformat) */

	return result;
}

/** Returns a string that corresponds to a GLSL type that can act as input to user-specified
 *  sampler type, and which can hold user-specified amount of components.
 *
 *  Throws TestError exception if either of the arguments was found invalid.
 *
 *  @param sampler_type Type of the sampler to use for the query.
 *  @param n_components Amount of components to use for the query.
 *
 *  @return Requested string.
 **/
const char* TextureViewUtilities::getGLSLDataTypeForSamplerType(const _sampler_type sampler_type,
																const unsigned int  n_components)
{
	const char* result = "";

	switch (sampler_type)
	{
	case SAMPLER_TYPE_FLOAT:
	{
		switch (n_components)
		{
		case 1:
			result = "float";
			break;
		case 2:
			result = "vec2";
			break;
		case 3:
			result = "vec3";
			break;
		case 4:
			result = "vec4";
			break;

		default:
		{
			TCU_FAIL("Unsupported number of components");
		}
		} /* switch (n_components) */

		break;
	}

	case SAMPLER_TYPE_SIGNED_INTEGER:
	{
		switch (n_components)
		{
		case 1:
			result = "int";
			break;
		case 2:
			result = "ivec2";
			break;
		case 3:
			result = "ivec3";
			break;
		case 4:
			result = "ivec4";
			break;

		default:
		{
			TCU_FAIL("Unsupported number of components");
		}
		} /* switch (n_components) */

		break;
	}

	case SAMPLER_TYPE_UNSIGNED_INTEGER:
	{
		switch (n_components)
		{
		case 1:
			result = "uint";
			break;
		case 2:
			result = "uvec2";
			break;
		case 3:
			result = "uvec3";
			break;
		case 4:
			result = "uvec4";
			break;

		default:
		{
			TCU_FAIL("Unsupported number of components");
		}
		} /* switch (n_components) */

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized sampler type");
	}
	} /* switch (sampler_type) */

	return result;
}

/** Retrieves a string defining a sampler type in GLSL which corresponds to user-specified internal
 *  sampler type.
 *
 *  Throws TestError exception if @param sampler_type was not recognized.
 *
 *  @param sampler_type Internal sampler type to use for the query.
 *
 *  @return Requested string.
 **/
const char* TextureViewUtilities::getGLSLTypeForSamplerType(const _sampler_type sampler_type)
{
	const char* result = "";

	switch (sampler_type)
	{
	case SAMPLER_TYPE_FLOAT:
		result = "sampler2D";
		break;
	case SAMPLER_TYPE_SIGNED_INTEGER:
		result = "isampler2D";
		break;
	case SAMPLER_TYPE_UNSIGNED_INTEGER:
		result = "usampler2D";
		break;

	default:
	{
		TCU_FAIL("Unrecognized sampler type");
	}
	} /* switch (sampler_type) */

	return result;
}

/** Returns a vector of texture+view internalformat combinations that are known to be incompatible.
 *
 *  @return Requested information.
 **/
TextureViewUtilities::_incompatible_internalformat_pairs TextureViewUtilities::
	getIllegalTextureAndViewInternalformatCombinations()
{
	TextureViewUtilities::_incompatible_internalformat_pairs result;

	/* Iterate in two loops over the set of supported internalformats */
	for (int n_texture_internalformat = 0;
		 n_texture_internalformat <
		 (n_internalformat_view_compatibility_array_entries / 2); /* the array stores two values per entry */
		 ++n_texture_internalformat)
	{
		glw::GLenum src_internalformat = internalformat_view_compatibility_array[(n_texture_internalformat * 2) + 0];
		_view_class src_view_class =
			(_view_class)internalformat_view_compatibility_array[(n_texture_internalformat * 2) + 1];

		for (int n_view_internalformat = n_texture_internalformat + 1;
			 n_view_internalformat < (n_internalformat_view_compatibility_array_entries >> 1); ++n_view_internalformat)
		{
			glw::GLenum view_internalformat = internalformat_view_compatibility_array[(n_view_internalformat * 2) + 0];
			_view_class view_view_class =
				(_view_class)internalformat_view_compatibility_array[(n_view_internalformat * 2) + 1];

			if (src_view_class != view_view_class)
			{
				result.push_back(_internalformat_pair(src_internalformat, view_internalformat));
			}
		} /* for (all internalformats we can use for the texture view) */
	}	 /* for (all internalformats we can use for the parent texture) */

	return result;
}

/** Returns a vector of texture+view target texture combinations that are known to be incompatible.
 *
 *  @return Requested information.
 **/
TextureViewUtilities::_incompatible_texture_target_pairs TextureViewUtilities::
	getIllegalTextureAndViewTargetCombinations()
{
	_incompatible_texture_target_pairs result;

	/* Iterate through all combinations of texture targets and store those that are
	 * reported as invalid
	 */
	for (unsigned int n_parent_texture_target = 0; n_parent_texture_target < n_valid_texture_targets;
		 ++n_parent_texture_target)
	{
		glw::GLenum parent_texture_target = valid_texture_targets[n_parent_texture_target];

		for (unsigned int n_view_texture_target = 0; n_view_texture_target < n_valid_texture_targets;
			 ++n_view_texture_target)
		{
			glw::GLenum view_texture_target = valid_texture_targets[n_view_texture_target];

			if (!isLegalTextureTargetForTextureView(parent_texture_target, view_texture_target))
			{
				result.push_back(_internalformat_pair(parent_texture_target, view_texture_target));
			}
		} /* for (all texture targets considered for views) */
	}	 /* for (all texture targets considered for parent texture) */

	return result;
}

/** Returns internalformats associated with user-specified view class.
 *
 *  @param view_class View class to use for the query.
 *
 *  @return Requested information.
 **/
TextureViewUtilities::_internalformats TextureViewUtilities::getInternalformatsFromViewClass(_view_class view_class)
{
	_internalformats result;

	/* Iterate over the data array and push those internalformats that match the requested view class */
	const unsigned int n_array_elements = n_internalformat_view_compatibility_array_entries;

	for (unsigned int n_array_pair = 0; n_array_pair < (n_array_elements >> 1); ++n_array_pair)
	{
		const glw::GLenum internalformat = internalformat_view_compatibility_array[n_array_pair * 2 + 0];
		const _view_class current_view_class =
			(_view_class)internalformat_view_compatibility_array[n_array_pair * 2 + 1];

		if (current_view_class == view_class)
		{
			result.push_back(internalformat);
		}
	} /* for (all pairs in the data array) */

	return result;
}

/** Returns a string defining user-specified internalformat.
 *
 *  Throws a TestError exception if @param internalformat was not recognized.
 *
 *  @param internalformat Internalformat to use for the query.
 *
 *  @return Requested string.
 **/
const char* TextureViewUtilities::getInternalformatString(const glw::GLenum internalformat)
{
	const char* result = "[?]";

	switch (internalformat)
	{
	case GL_RGBA32F:
		result = "GL_RGBA32F";
		break;
	case GL_RGBA32I:
		result = "GL_RGBA32I";
		break;
	case GL_RGBA32UI:
		result = "GL_RGBA32UI";
		break;
	case GL_RGBA16:
		result = "GL_RGBA16";
		break;
	case GL_RGBA16F:
		result = "GL_RGBA16F";
		break;
	case GL_RGBA16I:
		result = "GL_RGBA16I";
		break;
	case GL_RGBA16UI:
		result = "GL_RGBA16UI";
		break;
	case GL_RGBA8:
		result = "GL_RGBA8";
		break;
	case GL_RGBA8I:
		result = "GL_RGBA8I";
		break;
	case GL_RGBA8UI:
		result = "GL_RGBA8UI";
		break;
	case GL_SRGB8_ALPHA8:
		result = "GL_SRGB8_ALPHA8";
		break;
	case GL_RGB10_A2:
		result = "GL_RGB10_A2";
		break;
	case GL_RGB10_A2UI:
		result = "GL_RGB10_A2UI";
		break;
	case GL_RGB5_A1:
		result = "GL_RGB5_A1";
		break;
	case GL_RGBA4:
		result = "GL_RGBA4";
		break;
	case GL_R11F_G11F_B10F:
		result = "GL_R11F_G11F_B10F";
		break;
	case GL_RGB565:
		result = "GL_RGB565";
		break;
	case GL_RG32F:
		result = "GL_RG32F";
		break;
	case GL_RG32I:
		result = "GL_RG32I";
		break;
	case GL_RG32UI:
		result = "GL_RG32UI";
		break;
	case GL_RG16:
		result = "GL_RG16";
		break;
	case GL_RG16F:
		result = "GL_RG16F";
		break;
	case GL_RG16I:
		result = "GL_RG16I";
		break;
	case GL_RG16UI:
		result = "GL_RG16UI";
		break;
	case GL_RG8:
		result = "GL_RG8";
		break;
	case GL_RG8I:
		result = "GL_RG8I";
		break;
	case GL_RG8UI:
		result = "GL_RG8UI";
		break;
	case GL_R32F:
		result = "GL_R32F";
		break;
	case GL_R32I:
		result = "GL_R32I";
		break;
	case GL_R32UI:
		result = "GL_R32UI";
		break;
	case GL_R16F:
		result = "GL_R16F";
		break;
	case GL_R16I:
		result = "GL_R16I";
		break;
	case GL_R16UI:
		result = "GL_R16UI";
		break;
	case GL_R16:
		result = "GL_R16";
		break;
	case GL_R8:
		result = "GL_R8";
		break;
	case GL_R8I:
		result = "GL_R8I";
		break;
	case GL_R8UI:
		result = "GL_R8UI";
		break;
	case GL_RGBA16_SNORM:
		result = "GL_RGBA16_SNORM";
		break;
	case GL_RGBA8_SNORM:
		result = "GL_RGBA8_SNORM";
		break;
	case GL_RGB32F:
		result = "GL_RGB32F";
		break;
	case GL_RGB32I:
		result = "GL_RGB32I";
		break;
	case GL_RGB32UI:
		result = "GL_RGB32UI";
		break;
	case GL_RGB16_SNORM:
		result = "GL_RGB16_SNORM";
		break;
	case GL_RGB16F:
		result = "GL_RGB16F";
		break;
	case GL_RGB16I:
		result = "GL_RGB16I";
		break;
	case GL_RGB16UI:
		result = "GL_RGB16UI";
		break;
	case GL_RGB16:
		result = "GL_RGB16";
		break;
	case GL_RGB8_SNORM:
		result = "GL_RGB8_SNORM";
		break;
	case GL_RGB8:
		result = "GL_RGB8";
		break;
	case GL_RGB8I:
		result = "GL_RGB8I";
		break;
	case GL_RGB8UI:
		result = "GL_RGB8UI";
		break;
	case GL_SRGB8:
		result = "GL_SRGB8";
		break;
	case GL_RGB9_E5:
		result = "GL_RGB9_E5";
		break;
	case GL_RG16_SNORM:
		result = "GL_RG16_SNORM";
		break;
	case GL_RG8_SNORM:
		result = "GL_RG8_SNORM";
		break;
	case GL_R16_SNORM:
		result = "GL_R16_SNORM";
		break;
	case GL_R8_SNORM:
		result = "GL_R8_SNORM";
		break;
	case GL_DEPTH_COMPONENT32F:
		result = "GL_DEPTH_COMPONENT32F";
		break;
	case GL_DEPTH_COMPONENT24:
		result = "GL_DEPTH_COMPONENT24";
		break;
	case GL_DEPTH_COMPONENT16:
		result = "GL_DEPTH_COMPONENT16";
		break;
	case GL_DEPTH32F_STENCIL8:
		result = "GL_DEPTH32F_STENCIL8";
		break;
	case GL_DEPTH24_STENCIL8:
		result = "GL_DEPTH24_STENCIL8";
		break;
	case GL_COMPRESSED_RED_RGTC1:
		result = "GL_COMPRESSED_RED_RGTC1";
		break;
	case GL_COMPRESSED_SIGNED_RED_RGTC1:
		result = "GL_COMPRESSED_SIGNED_RED_RGTC1";
		break;
	case GL_COMPRESSED_RG_RGTC2:
		result = "GL_COMPRESSED_RG_RGTC2";
		break;
	case GL_COMPRESSED_SIGNED_RG_RGTC2:
		result = "GL_COMPRESSED_SIGNED_RG_RGTC2";
		break;
	case GL_COMPRESSED_RGBA_BPTC_UNORM:
		result = "GL_COMPRESSED_RGBA_BPTC_UNORM";
		break;
	case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
		result = "GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM";
		break;
	case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
		result = "GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT";
		break;
	case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
		result = "GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT";
		break;
	case GL_COMPRESSED_RGB8_ETC2:
		result = "GL_COMPRESSED_RGB8_ETC2";
		break;
	case GL_COMPRESSED_SRGB8_ETC2:
		result = "GL_COMPRESSED_SRGB8_ETC2";
		break;
	case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
		result = "GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2";
		break;
	case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
		result = "GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2";
		break;
	case GL_COMPRESSED_RGBA8_ETC2_EAC:
		result = "GL_COMPRESSED_RGBA8_ETC2_EAC";
		break;
	case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
		result = "GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC";
		break;
	case GL_COMPRESSED_R11_EAC:
		result = "GL_COMPRESSED_R11_EAC";
		break;
	case GL_COMPRESSED_SIGNED_R11_EAC:
		result = "GL_COMPRESSED_SIGNED_R11_EAC";
		break;
	case GL_COMPRESSED_RG11_EAC:
		result = "GL_COMPRESSED_RG11_EAC";
		break;
	case GL_COMPRESSED_SIGNED_RG11_EAC:
		result = "GL_COMPRESSED_SIGNED_RG11_EAC";
		break;

	default:
		TCU_FAIL("Unrecognized internalformat");
	}

	return result;
}

/** Returns all texture+view internalformat pairs that are valid in light of GL_ARB_texture_view specification.
 *
 *  @return As described.
 **/
TextureViewUtilities::_compatible_internalformat_pairs TextureViewUtilities::
	getLegalTextureAndViewInternalformatCombinations()
{
	_compatible_internalformat_pairs result;

	/* Iterate over all view classes */
	for (int current_view_class_it = static_cast<int>(VIEW_CLASS_FIRST);
		 current_view_class_it != static_cast<int>(VIEW_CLASS_COUNT); current_view_class_it++)
	{
		_view_class		 current_view_class			= static_cast<_view_class>(current_view_class_it);
		_internalformats view_class_internalformats = getInternalformatsFromViewClass(current_view_class);

		/* Store all combinations in the result vector */
		for (_internalformats_const_iterator left_iterator = view_class_internalformats.begin();
			 left_iterator != view_class_internalformats.end(); left_iterator++)
		{
			for (_internalformats_const_iterator right_iterator = view_class_internalformats.begin();
				 right_iterator != view_class_internalformats.end(); ++right_iterator)
			{
				result.push_back(_internalformat_pair(*left_iterator, *right_iterator));
			} /* for (all internalformats to be used as right-side values) */
		}	 /* for (all internalformats to be used as left-side values) */
	}		  /* for (all view classes) */

	return result;
}

/** Returns all valid texture+view texture targets pairs.
 *
 *  @return As per description.
 **/
TextureViewUtilities::_compatible_texture_target_pairs TextureViewUtilities::getLegalTextureAndViewTargetCombinations()
{
	_compatible_texture_target_pairs result;

	/* Iterate over all texture targets valid for a glTextureView() call. Consider each one of them as
	 * original texture target.
	 */
	for (unsigned int n_original_texture_target = 0; n_original_texture_target < n_valid_texture_targets;
		 ++n_original_texture_target)
	{
		const glw::GLenum original_texture_target = valid_texture_targets[n_original_texture_target];

		/* Iterate again, but this time consider each texture target as a valid new target */
		for (unsigned int n_compatible_texture_target = 0; n_compatible_texture_target < n_valid_texture_targets;
			 ++n_compatible_texture_target)
		{
			const glw::GLenum view_texture_target = valid_texture_targets[n_compatible_texture_target];

			if (TextureViewUtilities::isLegalTextureTargetForTextureView(original_texture_target, view_texture_target))
			{
				result.push_back(_texture_target_pair(original_texture_target, view_texture_target));
			}
		} /* for (all texture targets that are potentially compatible) */
	}	 /* for (all original texture targets) */

	return result;
}

/** Returns major & minor version for user-specified CTS rendering context type.
 *
 *  @param context_type      CTS rendering context type.
 *  @param out_major_version Deref will be used to store major version. Must not be NULL.
 *  @param out_minor_version Deref will be used to store minor version. Must not be NULL.
 *
 **/
void TextureViewUtilities::getMajorMinorVersionFromContextVersion(const glu::ContextType& context_type,
																  glw::GLint*			  out_major_version,
																  glw::GLint*			  out_minor_version)
{
	if (context_type.getAPI() == glu::ApiType::core(4, 0))
	{
		*out_major_version = 4;
		*out_minor_version = 0;
	}
	else if (context_type.getAPI() == glu::ApiType::core(4, 1))
	{
		*out_major_version = 4;
		*out_minor_version = 1;
	}
	else if (context_type.getAPI() == glu::ApiType::core(4, 2))
	{
		*out_major_version = 4;
		*out_minor_version = 2;
	}
	else if (context_type.getAPI() == glu::ApiType::core(4, 3))
	{
		*out_major_version = 4;
		*out_minor_version = 3;
	}
	else if (context_type.getAPI() == glu::ApiType::core(4, 4))
	{
		*out_major_version = 4;
		*out_minor_version = 4;
	}
	else if (context_type.getAPI() == glu::ApiType::core(4, 5))
	{
		*out_major_version = 4;
		*out_minor_version = 5;
	}
	else if (context_type.getAPI() == glu::ApiType::core(4, 6))
	{
		*out_major_version = 4;
		*out_minor_version = 6;
	}
	else
	{
		TCU_FAIL("Unrecognized rendering context version");
	}
}

/** Tells which sampler can be used to sample a texture defined with user-specified
 *  internalformat.
 *
 *  Supports both compressed and non-compressed internalformats.
 *  Throws TestError exception if @param internalformat was not recognized.
 *
 *  @param internalformat Internalformat to use for the query.
 *
 *  @return Requested information.
 **/
_sampler_type TextureViewUtilities::getSamplerTypeForInternalformat(const glw::GLenum internalformat)
{
	_sampler_type result = SAMPLER_TYPE_UNDEFINED;

	/* Compressed internalformats not supported at the moment */

	switch (internalformat)
	{
	case GL_COMPRESSED_RED_RGTC1:
	case GL_COMPRESSED_SIGNED_RED_RGTC1:
	case GL_COMPRESSED_RG_RGTC2:
	case GL_COMPRESSED_SIGNED_RG_RGTC2:
	case GL_COMPRESSED_RGBA_BPTC_UNORM:
	case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
	case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
	case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32F:
	case GL_RGBA16:
	case GL_RGBA16_SNORM:
	case GL_RGBA16F:
	case GL_RGBA32F:
	case GL_RGBA4:
	case GL_RGBA8:
	case GL_RGBA8_SNORM:
	case GL_RGB10_A2:
	case GL_RGB16:
	case GL_RGB16_SNORM:
	case GL_RGB16F:
	case GL_RGB32F:
	case GL_RGB5_A1:
	case GL_RGB565:
	case GL_RGB8:
	case GL_RGB8_SNORM:
	case GL_RGB9_E5:
	case GL_RG16:
	case GL_RG16_SNORM:
	case GL_RG16F:
	case GL_RG32F:
	case GL_RG8:
	case GL_RG8_SNORM:
	case GL_R11F_G11F_B10F:
	case GL_R16:
	case GL_R16F:
	case GL_R16_SNORM:
	case GL_R32F:
	case GL_R8:
	case GL_R8_SNORM:
	case GL_SRGB8_ALPHA8:
	case GL_SRGB8:
	{
		result = SAMPLER_TYPE_FLOAT;

		break;
	}

	case GL_RGB10_A2UI:
	case GL_RGBA32UI:
	case GL_RGBA16UI:
	case GL_RGBA8UI:
	case GL_RGB16UI:
	case GL_RGB32UI:
	case GL_RGB8UI:
	case GL_RG16UI:
	case GL_RG32UI:
	case GL_RG8UI:
	case GL_R16UI:
	case GL_R32UI:
	case GL_R8UI:
	{
		result = SAMPLER_TYPE_UNSIGNED_INTEGER;

		break;
	}

	case GL_RGBA16I:
	case GL_RGBA32I:
	case GL_RGBA8I:
	case GL_RGB16I:
	case GL_RGB32I:
	case GL_RGB8I:
	case GL_RG16I:
	case GL_RG32I:
	case GL_RG8I:
	case GL_R16I:
	case GL_R32I:
	case GL_R8I:
	{
		result = SAMPLER_TYPE_SIGNED_INTEGER;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized internalformat");
	}
	} /* switch (interalformat) */

	return result;
}

/** Tells how many bytes are required to define a texture mip-map using
 *  user-specified internalformat and type, assuming user-defined mip-map
 *  resolution. Compressed internalformats are NOT supported.
 *
 *  Throws TestError exception if @param internalformat or @param type are
 *  found invalid.
 *
 *  @param internalformat Internalformat to use for the query.
 *  @param type           Type to use for the query.
 *  @param width          Mip-map width to use for the query.
 *  @param height         Mip-map height to use for the query.
 *
 *  @return Requested information.
 **/
unsigned int TextureViewUtilities::getTextureDataSize(const glw::GLenum internalformat, const glw::GLenum type,
													  const unsigned int width, const unsigned int height)
{
	unsigned int internalformat_rgba_size[4] = { 0 };
	unsigned int type_rgba_size[4]			 = { 0 };
	unsigned int texel_size					 = 0;

	TextureViewUtilities::getComponentSizeForInternalformat(internalformat, internalformat_rgba_size);
	TextureViewUtilities::getComponentSizeForType(type, type_rgba_size);

	if (internalformat_rgba_size[0] == 0)
	{
		type_rgba_size[0] = 0;
	}

	if (internalformat_rgba_size[1] == 0)
	{
		type_rgba_size[1] = 0;
	}

	if (internalformat_rgba_size[2] == 0)
	{
		type_rgba_size[2] = 0;
	}

	if (internalformat_rgba_size[3] == 0)
	{
		type_rgba_size[3] = 0;
	}

	texel_size = type_rgba_size[0] + type_rgba_size[1] + type_rgba_size[2] + type_rgba_size[3];

	/* Current implementation assumes we do not need to use bit resolution when
	 * preparing texel data. Make extra sure we're not wrong. */
	DE_ASSERT((texel_size % 8) == 0);

	texel_size /= 8; /* bits per byte */

	return texel_size * width * height;
}

/** Returns a string corresponding to a GL enum describing a texture target.
 *
 *  @return As per description or "[?]" if the enum was not recognized.
 **/
const char* TextureViewUtilities::getTextureTargetString(const glw::GLenum texture_target)
{
	const char* result = "[?]";

	switch (texture_target)
	{
	case GL_TEXTURE_1D:
		result = "GL_TEXTURE_1D";
		break;
	case GL_TEXTURE_1D_ARRAY:
		result = "GL_TEXTURE_1D_ARRAY";
		break;
	case GL_TEXTURE_2D:
		result = "GL_TEXTURE_2D";
		break;
	case GL_TEXTURE_2D_ARRAY:
		result = "GL_TEXTURE_2D_ARRAY";
		break;
	case GL_TEXTURE_2D_MULTISAMPLE:
		result = "GL_TEXTURE_2D_MULTISAMPLE";
		break;
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		result = "GL_TEXTURE_2D_MULTISAMPLE_ARRAY";
		break;
	case GL_TEXTURE_3D:
		result = "GL_TEXTURE_3D";
		break;
	case GL_TEXTURE_BUFFER:
		result = "GL_TEXTURE_BUFFER";
		break;
	case GL_TEXTURE_CUBE_MAP:
		result = "GL_TEXTURE_CUBE_MAP";
		break;
	case GL_TEXTURE_CUBE_MAP_ARRAY:
		result = "GL_TEXTURE_CUBE_MAP_ARRAY";
		break;
	case GL_TEXTURE_RECTANGLE:
		result = "GL_TEXTURE_RECTANGLE";
		break;
	}

	return result;
}

/** Returns GL type that can be used to define a texture mip-map defined
 *  with an internalformat of @param internalformat.
 *
 *  Throws TestError exception if @param internalformat was found to be invalid.
 *
 *  @param internalformat Internalformat to use for the query.
 *
 *  @return Requested information.
 **/
glw::GLenum TextureViewUtilities::getTypeCompatibleWithInternalformat(const glw::GLenum internalformat)
{
	glw::GLenum result = GL_NONE;

	/* Compressed internalformats not supported at the moment */

	switch (internalformat)
	{
	case GL_RGBA8_SNORM:
	case GL_RGB8_SNORM:
	case GL_RG8_SNORM:
	case GL_R8_SNORM:
	case GL_RGBA8I:
	case GL_RGB8I:
	case GL_RG8I:
	case GL_R8I:
	{
		result = GL_BYTE;

		break;
	}

	case GL_DEPTH24_STENCIL8:
	{
		result = GL_UNSIGNED_INT_24_8;

		break;
	}

	case GL_DEPTH32F_STENCIL8:
	{
		result = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;

		break;
	}

	case GL_RGBA16F:
	case GL_RGB16F:
	case GL_RG16F:
	case GL_R16F:
	{
		result = GL_HALF_FLOAT;

		break;
	}

	case GL_DEPTH_COMPONENT32F:
	case GL_RGBA32F:
	case GL_RGB32F:
	case GL_RG32F:
	case GL_R11F_G11F_B10F:
	case GL_R32F:
	{
		result = GL_FLOAT;

		break;
	}

	case GL_RGBA16_SNORM:
	case GL_RGB16_SNORM:
	case GL_RG16_SNORM:
	case GL_R16_SNORM:
	{
		result = GL_SHORT;

		break;
	}

	case GL_RGBA4:
	case GL_RGBA8:
	case GL_RGB10_A2:
	case GL_RGB5_A1:
	case GL_RGB565:
	case GL_RGB8:
	case GL_RGB9_E5:
	case GL_RG8:
	case GL_R8:
	case GL_SRGB8_ALPHA8:
	case GL_SRGB8:
	case GL_RGBA8UI:
	case GL_RGB8UI:
	case GL_RG8UI:
	case GL_R8UI:
	{
		result = GL_UNSIGNED_BYTE;

		break;
	}

	case GL_R16I:
	case GL_RGBA16I:
	case GL_RGB16I:
	case GL_RG16I:
	{
		result = GL_SHORT;

		break;
	}

	case GL_DEPTH_COMPONENT16:
	case GL_RGBA16:
	case GL_RGB16:
	case GL_RG16:
	case GL_R16:
	case GL_RGBA16UI:
	case GL_RGB16UI:
	case GL_RG16UI:
	case GL_R16UI:
	{
		result = GL_UNSIGNED_SHORT;

		break;
	}

	case GL_RGBA32I:
	case GL_RGB32I:
	case GL_RG32I:
	case GL_R32I:
	{
		result = GL_INT;

		break;
	}

	case GL_DEPTH_COMPONENT24:
	case GL_RGBA32UI:
	case GL_RGB32UI:
	case GL_RG32UI:
	case GL_R32UI:
	{
		result = GL_UNSIGNED_INT;

		break;
	}

	case GL_RGB10_A2UI:
	{
		result = GL_UNSIGNED_INT_2_10_10_10_REV;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized internalformat");
	}
	} /* switch (interalformat) */

	return result;
}

/** Tells what view class is the user-specified internalformat associated with.
 *
 *  Implements Table 8.21 from OpenGL Specification 4.3
 *
 *  @param internalformat Internalformat to use for the query.
 *
 *  @return Requested information or VIEW_CLASS_UNDEFINED if @param internalformat
 *          has not been recognized.
 **/
_view_class TextureViewUtilities::getViewClassForInternalformat(const glw::GLenum internalformat)
{
	_view_class result = VIEW_CLASS_UNDEFINED;

	/* Note that n_internalformat_view_compatibility_array_entries needs to be divided by 2
	 * because the value refers to a total number of entries in the array, not to the number
	 * of pairs that can be read.
	 */
	for (int n_entry = 0; n_entry < (n_internalformat_view_compatibility_array_entries >> 1); n_entry++)
	{
		glw::GLenum array_internalformat = internalformat_view_compatibility_array[(n_entry * 2) + 0];
		_view_class view_class			 = (_view_class)internalformat_view_compatibility_array[(n_entry * 2) + 1];

		if (array_internalformat == internalformat)
		{
			result = view_class;

			break;
		}
	} /* for (all pairs in data array) */

	return result;
}

/** Initializes texture storage for either an immutable or mutable texture object,
 *  depending on configuration of the test run the storage is to be initialized for.
 *
 *  @param gl                          GL entry-points to use for storage initialization.
 *  @param init_mutable_to             true if a mutable texture storage should be initialized,
 *                                     false to initialize immutable texture storage.
 *  @param texture_target              Texture target to be used.
 *  @param texture_depth               Depth to be used for texture storage. Only used
 *                                     for texture targets that use the depth information.
 *  @param texture_height              Height to be used for texture storage. Only used
 *                                     for texture targets that use the height information.
 *  @param texture_width               Width to be used for texture storage.
 *  @param texture_internalformat      Internalformat to be used for texture storage.
 *  @param texture_format              Format to be used for texture storage.
 *  @param texture_type                Type to be used for texture storage.
 *  @param n_levels_needed             Amount of mip-map levels that should be used for texture storage.
 *                                     Only used for texture targets that support mip-maps.
 *  @param n_cubemaps_needed           Amount of cube-maps to be used for initialization of cube map
 *                                     array texture storage. Only used if @param texture_internalformat
 *                                     is set to GL_TEXTURE_CUBE_MAP_ARRAY.
 *  @param bo_id                       ID of a buffer object to be used for initialization of
 *                                     buffer texture storage. Only used if @param texture_internalformat
 *                                     is set to GL_TEXTURE_BUFFEER.
 *
 **/
void TextureViewUtilities::initTextureStorage(const glw::Functions& gl, bool init_mutable_to,
											  glw::GLenum texture_target, glw::GLint texture_depth,
											  glw::GLint texture_height, glw::GLint texture_width,
											  glw::GLenum texture_internalformat, glw::GLenum texture_format,
											  glw::GLenum texture_type, unsigned int n_levels_needed,
											  unsigned int n_cubemaps_needed, glw::GLint bo_id)
{
	const glw::GLenum cubemap_texture_targets[] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
													GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
													GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };
	const unsigned int n_cubemap_texture_targets = sizeof(cubemap_texture_targets) / sizeof(cubemap_texture_targets[0]);

	/* If we're going to be initializing a multisample texture object,
	 * determine how many samples can be used for GL_RGBA8 internalformat,
	 * given texture target that is of our interest */
	glw::GLint gl_max_color_texture_samples_value = 0;

	gl.getIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &gl_max_color_texture_samples_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed for GL_MAX_COLOR_TEXTURE_SAMPLES");

	if (texture_target == GL_TEXTURE_BUFFER)
	{
		gl.texBuffer(GL_TEXTURE_BUFFER, texture_internalformat, bo_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexBuffer() call failed for GL_TEXTURE_BUFFER target");
	}
	else if (init_mutable_to)
	{
		for (unsigned int n_level = 0; n_level < n_levels_needed; ++n_level)
		{
			/* If level != 0 and we're trying to initialize a texture target which
			 * only accepts a single level, leave now
			 */
			if (n_level != 0 &&
				(texture_target == GL_TEXTURE_RECTANGLE || texture_target == GL_TEXTURE_2D_MULTISAMPLE ||
				 texture_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY || texture_target == GL_TEXTURE_BUFFER))
			{
				break;
			}

			/* Initialize mutable texture storage */
			switch (texture_target)
			{
			case GL_TEXTURE_1D:
			{
				gl.texImage1D(texture_target, n_level, texture_internalformat, texture_width >> n_level, 0, /* border */
							  texture_format, texture_type, DE_NULL);										/* pixels */

				GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D() call failed for GL_TEXTURE_1D texture target");

				break;
			}

			case GL_TEXTURE_1D_ARRAY:
			case GL_TEXTURE_2D:
			case GL_TEXTURE_RECTANGLE:
			{
				gl.texImage2D(texture_target, n_level, texture_internalformat, texture_width >> n_level,
							  texture_height >> n_level, 0,			  /* border */
							  texture_format, texture_type, DE_NULL); /* pixels */

				GLU_EXPECT_NO_ERROR(gl.getError(),
									(texture_target == GL_TEXTURE_1D_ARRAY) ?
										"glTexImage2D() call failed for GL_TEXTURE_1D_ARRAY texture target" :
										(texture_target == GL_TEXTURE_2D) ?
										"glTexImage2D() call failed for GL_TEXTURE_2D texture target" :
										"glTexImage2D() call failed for GL_TEXTURE_RECTANGLE texture target");

				break;
			}

			case GL_TEXTURE_2D_ARRAY:
			case GL_TEXTURE_3D:
			{
				gl.texImage3D(texture_target, n_level, texture_internalformat, texture_width >> n_level,
							  texture_height >> n_level, texture_depth >> n_level, 0, /* border */
							  texture_format, texture_type, DE_NULL);				  /* pixels */

				GLU_EXPECT_NO_ERROR(gl.getError(),
									(texture_target == GL_TEXTURE_2D_ARRAY) ?
										"glTexImage3D() call failed for GL_TEXTURE_2D_ARRAY texture target" :
										"glTexImage3D() call failed for GL_TEXTURE_3D texture target");

				break;
			}

			case GL_TEXTURE_2D_MULTISAMPLE:
			{
				gl.texImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, gl_max_color_texture_samples_value,
										 texture_internalformat, texture_width >> n_level, texture_height >> n_level,
										 GL_TRUE); /* fixedsamplelocations */

				GLU_EXPECT_NO_ERROR(
					gl.getError(),
					"glTexImage2DMultisample() call failed for GL_TEXTURE_2D_MULTISAMPLE texture target");

				break;
			}

			case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
			{
				gl.texImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, gl_max_color_texture_samples_value,
										 texture_internalformat, texture_width >> n_level, texture_height >> n_level,
										 texture_depth >> n_level, GL_TRUE); /* fixedsamplelocations */

				GLU_EXPECT_NO_ERROR(
					gl.getError(),
					"glTexImage3DMultisample() call failed for GL_TEXTURE_2D_MULTISAMPLE_ARRAY texture target");

				break;
			}

			case GL_TEXTURE_CUBE_MAP:
			{
				for (unsigned int n_cubemap_texture_target = 0; n_cubemap_texture_target < n_cubemap_texture_targets;
					 ++n_cubemap_texture_target)
				{
					glw::GLenum cubemap_texture_target = cubemap_texture_targets[n_cubemap_texture_target];

					gl.texImage2D(cubemap_texture_target, n_level, texture_internalformat, texture_width >> n_level,
								  texture_height >> n_level, 0,			  /* border */
								  texture_format, texture_type, DE_NULL); /* pixels */

					GLU_EXPECT_NO_ERROR(gl.getError(),
										"glTexImage2D() call failed for one of the cube-map texture targets");

					break;
				} /* for (all cube-map texture targets) */

				break;
			}

			case GL_TEXTURE_CUBE_MAP_ARRAY:
			{
				gl.texImage3D(texture_target, n_level, texture_internalformat, texture_width >> n_level,
							  texture_height >> n_level, 6 /* layer-faces */ * n_cubemaps_needed, 0, /* border */
							  texture_format, texture_type, DE_NULL);								 /* pixels */

				GLU_EXPECT_NO_ERROR(gl.getError(),
									"glTexImage3D() call failed for GL_TEXTURE_CUBE_MAP_ARRAY texture target");

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized texture target");
			}
			} /* switch (texture_target) */
		}	 /* for (all levels) */
	}		  /* if (texture_type == TEST_TEXTURE_TYPE_MUTABLE_TEXTURE_OBJECT) */
	else
	{
		/* Initialize immutable texture storage */
		switch (texture_target)
		{
		case GL_TEXTURE_1D:
		{
			gl.texStorage1D(texture_target, n_levels_needed, texture_internalformat, texture_width);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage1D() call failed for GL_TEXTURE_1D texture target");

			break;
		}

		case GL_TEXTURE_1D_ARRAY:
		case GL_TEXTURE_2D:
		case GL_TEXTURE_CUBE_MAP:
		case GL_TEXTURE_RECTANGLE:
		{
			const unsigned n_levels = (texture_target == GL_TEXTURE_RECTANGLE) ? 1 : n_levels_needed;

			gl.texStorage2D(texture_target, n_levels, texture_internalformat, texture_width, texture_height);

			GLU_EXPECT_NO_ERROR(gl.getError(),
								(texture_target == GL_TEXTURE_1D_ARRAY) ?
									"glTexStorage2D() call failed for GL_TEXTURE_1D_ARRAY texture target" :
									(texture_target == GL_TEXTURE_2D) ?
									"glTexStorage2D() call failed for GL_TEXTURE_2D texture target" :
									(texture_target == GL_TEXTURE_CUBE_MAP) ?
									"glTexStorage2D() call failed for GL_TEXTURE_CUBE_MAP texture target" :
									"glTexStorage2D() call failed for GL_TEXTURE_RECTANGLE texture target");

			break;
		}

		case GL_TEXTURE_2D_ARRAY:
		case GL_TEXTURE_3D:
		{
			gl.texStorage3D(texture_target, n_levels_needed, texture_internalformat, texture_width, texture_height,
							texture_depth);

			GLU_EXPECT_NO_ERROR(gl.getError(),
								(texture_target == GL_TEXTURE_2D_ARRAY) ?
									"glTexStorage3D() call failed for GL_TEXTURE_2D_ARRAY texture target" :
									"glTexStorage3D() call failed for GL_TEXTURE_3D texture target");

			break;
		}

		case GL_TEXTURE_2D_MULTISAMPLE:
		{
			gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, gl_max_color_texture_samples_value,
									   texture_internalformat, texture_width, texture_height,
									   GL_TRUE); /* fixedsamplelocations */

			GLU_EXPECT_NO_ERROR(gl.getError(),
								"glTexStorage2DMultisample() call failed for GL_TEXTURE_2D_MULTISAMPLE texture target");

			break;
		}

		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		{
			gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, gl_max_color_texture_samples_value,
									   texture_internalformat, texture_width, texture_height, texture_depth,
									   GL_TRUE); /* fixedsamplelocations */

			GLU_EXPECT_NO_ERROR(
				gl.getError(),
				"glTexStorage3DMultisample() call failed for GL_TEXTURE_2D_MULTISAMPLE_ARRAY texture target");

			break;
		}

		case GL_TEXTURE_CUBE_MAP_ARRAY:
		{
			const unsigned int actual_texture_depth = 6 /* layer-faces */ * n_cubemaps_needed;

			gl.texStorage3D(texture_target, n_levels_needed, texture_internalformat, texture_width, texture_height,
							actual_texture_depth);

			GLU_EXPECT_NO_ERROR(gl.getError(),
								"glTexStorage3D() call failed for GL_TEXTURE_CUBE_MAP_ARRAY texture target");

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized texture target");
		}
		} /* switch (texture_target) */
	}
}

/** Tells whether a parent texture object, storage of which uses @param original_internalformat
 *  internalformat, can be used to generate a texture view using @param view_internalformat
 *  internalformat.
 *
 *  @param original_internalformat Internalformat used for parent texture object storage.
 *  @param view_internalformat     Internalformat to be used for view texture object storage.
 *
 *  @return true if the internalformats are compatible, false otherwise.
 **/
bool TextureViewUtilities::isInternalformatCompatibleForTextureView(glw::GLenum original_internalformat,
																	glw::GLenum view_internalformat)
{
	const _view_class original_internalformat_view_class = getViewClassForInternalformat(original_internalformat);
	const _view_class view_internalformat_view_class	 = getViewClassForInternalformat(view_internalformat);

	return (original_internalformat_view_class == view_internalformat_view_class);
}

/** Tells whether user-specified internalformat is compressed.
 *
 *  @param internalformat Internalformat to use for the query.
 *
 *  @return true if @param internalformat is a known compressed internalformat,
 *          false otherwise.
 **/
bool TextureViewUtilities::isInternalformatCompressed(const glw::GLenum internalformat)
{
	bool result = false;

	switch (internalformat)
	{
	case GL_COMPRESSED_RED_RGTC1:
	case GL_COMPRESSED_SIGNED_RED_RGTC1:
	case GL_COMPRESSED_RG_RGTC2:
	case GL_COMPRESSED_SIGNED_RG_RGTC2:
	case GL_COMPRESSED_RGBA_BPTC_UNORM:
	case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
	case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
	case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
	case GL_COMPRESSED_RGB8_ETC2:
	case GL_COMPRESSED_SRGB8_ETC2:
	case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
	case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
	case GL_COMPRESSED_RGBA8_ETC2_EAC:
	case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
	case GL_COMPRESSED_R11_EAC:
	case GL_COMPRESSED_SIGNED_R11_EAC:
	case GL_COMPRESSED_RG11_EAC:
	case GL_COMPRESSED_SIGNED_RG11_EAC:
	{
		result = true;

		break;
	}
	} /* switch (internalformat) */

	return result;
}

/** Tells whether user-specified internalformat operates in sRGB color space.
 *
 *  @param internalformat Internalformat to use for the query.
 *
 *  @return true if @param internalformat is a known sRGB internalformat,
 *          false otherwise.
 **/
bool TextureViewUtilities::isInternalformatSRGB(const glw::GLenum internalformat)
{
	return (internalformat == GL_SRGB8 || internalformat == GL_SRGB8_ALPHA8 ||
			internalformat == GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM);
}

/** Tells whether user-specified internalformat is supported by OpenGL of a given version.
 *
 *  @param internalformat Internalformat to use for the query.
 *  @param major_version  Major version of the rendering context.
 *  @param minor_version  Minor version of the rendering context.
 *
 *  @return true if the internalformat is supported, false otherwise.
 **/
bool TextureViewUtilities::isInternalformatSupported(glw::GLenum internalformat, const glw::GLint major_version,
													 const glw::GLint minor_version)
{
	(void)major_version;
	/* NOTE: This function, as it stands right now, does not consider OpenGL contexts
	 *       lesser than 4.
	 **/
	glw::GLint minimum_minor_version = 0;

	DE_ASSERT(major_version >= 4);

	switch (internalformat)
	{
	/* >= OpenGL 4.0 */
	case GL_RGBA32F:
	case GL_RGBA32I:
	case GL_RGBA32UI:
	case GL_RGBA16:
	case GL_RGBA16F:
	case GL_RGBA16I:
	case GL_RGBA16UI:
	case GL_RGBA8:
	case GL_RGBA8I:
	case GL_RGBA8UI:
	case GL_SRGB8_ALPHA8:
	case GL_RGB10_A2:
	case GL_RGB10_A2UI:
	case GL_RGB5_A1:
	case GL_RGBA4:
	case GL_R11F_G11F_B10F:
	case GL_RG32F:
	case GL_RG32I:
	case GL_RG32UI:
	case GL_RG16:
	case GL_RG16F:
	case GL_RG16I:
	case GL_RG16UI:
	case GL_RG8:
	case GL_RG8I:
	case GL_RG8UI:
	case GL_R32F:
	case GL_R32I:
	case GL_R32UI:
	case GL_R16F:
	case GL_R16I:
	case GL_R16UI:
	case GL_R16:
	case GL_R8:
	case GL_R8I:
	case GL_R8UI:
	case GL_RGBA16_SNORM:
	case GL_RGBA8_SNORM:
	case GL_RGB32F:
	case GL_RGB32I:
	case GL_RGB32UI:
	case GL_RGB16_SNORM:
	case GL_RGB16F:
	case GL_RGB16I:
	case GL_RGB16UI:
	case GL_RGB16:
	case GL_RGB8_SNORM:
	case GL_RGB8:
	case GL_RGB8I:
	case GL_RGB8UI:
	case GL_SRGB8:
	case GL_RGB9_E5:
	case GL_RG16_SNORM:
	case GL_RG8_SNORM:
	case GL_R16_SNORM:
	case GL_R8_SNORM:
	case GL_DEPTH_COMPONENT32F:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH32F_STENCIL8:
	case GL_DEPTH24_STENCIL8:
	case GL_COMPRESSED_RED_RGTC1:
	case GL_COMPRESSED_SIGNED_RED_RGTC1:
	case GL_COMPRESSED_RG_RGTC2:
	case GL_COMPRESSED_SIGNED_RG_RGTC2:
	{
		/* Already covered by default value of minimum_minor_version */

		break;
	}

	/* >= OpenGL 4.2 */
	case GL_RGB565:
	case GL_COMPRESSED_RGBA_BPTC_UNORM:
	case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
	case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
	case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
	{
		minimum_minor_version = 2;

		break;
	}

	/* >= OpenGL 4.3 */
	case GL_COMPRESSED_RGB8_ETC2:
	case GL_COMPRESSED_SRGB8_ETC2:
	case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
	case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
	case GL_COMPRESSED_RGBA8_ETC2_EAC:
	case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
	case GL_COMPRESSED_R11_EAC:
	case GL_COMPRESSED_SIGNED_R11_EAC:
	case GL_COMPRESSED_RG11_EAC:
	case GL_COMPRESSED_SIGNED_RG11_EAC:
	{
		minimum_minor_version = 3;

		break;
	}

	default:
		TCU_FAIL("Unrecognized internalformat");
	}

	return (minor_version >= minimum_minor_version);
}

/** Tells whether a parent texture object using @param original_texture_target texture target
 *  can be used to generate a texture view of @param view_texture_target texture target.
 *
 *  @param original_texture_target Texture target used by parent texture;
 *  @param view_texture_target     Texture target to be used for view texture;
 *
 *  @return true if the texture targets are compatible, false otherwise.
 **/
bool TextureViewUtilities::isLegalTextureTargetForTextureView(glw::GLenum original_texture_target,
															  glw::GLenum view_texture_target)
{
	bool result = false;

	switch (original_texture_target)
	{
	case GL_TEXTURE_1D:
	{
		result = (view_texture_target == GL_TEXTURE_1D || view_texture_target == GL_TEXTURE_1D_ARRAY);

		break;
	}

	case GL_TEXTURE_2D:
	{
		result = (view_texture_target == GL_TEXTURE_2D || view_texture_target == GL_TEXTURE_2D_ARRAY);

		break;
	}

	case GL_TEXTURE_3D:
	{
		result = (view_texture_target == GL_TEXTURE_3D);

		break;
	}

	case GL_TEXTURE_CUBE_MAP:
	{
		result = (view_texture_target == GL_TEXTURE_CUBE_MAP || view_texture_target == GL_TEXTURE_2D ||
				  view_texture_target == GL_TEXTURE_2D_ARRAY || view_texture_target == GL_TEXTURE_CUBE_MAP_ARRAY);

		break;
	}

	case GL_TEXTURE_RECTANGLE:
	{
		result = (view_texture_target == GL_TEXTURE_RECTANGLE);

		break;
	}

	case GL_TEXTURE_BUFFER:
	{
		/* No targets supported */

		break;
	}

	case GL_TEXTURE_1D_ARRAY:
	{
		result = (view_texture_target == GL_TEXTURE_1D_ARRAY || view_texture_target == GL_TEXTURE_1D);

		break;
	}

	case GL_TEXTURE_2D_ARRAY:
	{
		result = (view_texture_target == GL_TEXTURE_2D_ARRAY || view_texture_target == GL_TEXTURE_2D ||
				  view_texture_target == GL_TEXTURE_CUBE_MAP || view_texture_target == GL_TEXTURE_CUBE_MAP_ARRAY);

		break;
	}

	case GL_TEXTURE_CUBE_MAP_ARRAY:
	{
		result = (view_texture_target == GL_TEXTURE_CUBE_MAP_ARRAY || view_texture_target == GL_TEXTURE_2D_ARRAY ||
				  view_texture_target == GL_TEXTURE_2D || view_texture_target == GL_TEXTURE_CUBE_MAP);

		break;
	}

	case GL_TEXTURE_2D_MULTISAMPLE:
	{
		result = (view_texture_target == GL_TEXTURE_2D_MULTISAMPLE ||
				  view_texture_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY);

		break;
	}

	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
	{
		result = (view_texture_target == GL_TEXTURE_2D_MULTISAMPLE ||
				  view_texture_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY);

		break;
	}
	} /* switch (original_texture_target) */

	return result;
}

/** Constructor.
 *
 *  @param context Rendering context.
 **/
TextureViewTestGetTexParameter::TextureViewTestGetTexParameter(deqp::Context& context)
	: TestCase(context, "gettexparameter", "Verifies glGetTexParameterfv() and glGetTexParameteriv() "
										   "work as specified")
{
	/* Left blank on purpose */
}

/** De-initializes all GL objects created for the test. */
void TextureViewTestGetTexParameter::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Deinitialize all test runs */
	for (_test_runs_iterator it = m_test_runs.begin(); it != m_test_runs.end(); ++it)
	{
		_test_run& test_run = *it;

		if (test_run.parent_texture_object_id != 0)
		{
			gl.deleteTextures(1, &test_run.parent_texture_object_id);

			test_run.parent_texture_object_id = 0;
		}

		if (test_run.texture_view_object_created_from_immutable_to_id != 0)
		{
			gl.deleteTextures(1, &test_run.texture_view_object_created_from_immutable_to_id);

			test_run.texture_view_object_created_from_immutable_to_id = 0;
		}

		if (test_run.texture_view_object_created_from_view_to_id != 0)
		{
			gl.deleteTextures(1, &test_run.texture_view_object_created_from_view_to_id);

			test_run.texture_view_object_created_from_view_to_id = 0;
		}
	}
	m_test_runs.clear();
}

/** Initializes test run descriptors used by the test. This also includes
 *  all GL objects used by all the iterations.
 **/
void TextureViewTestGetTexParameter::initTestRuns()
{
	const glw::Functions& gl				= m_context.getRenderContext().getFunctions();
	const int			  n_cubemaps_needed = 4; /* only used for GL_TEXTURE_CUBE_MAP_ARRAY */
	const int			  texture_depth		= 16;
	const int			  texture_height	= 32;
	const int			  texture_width		= 64;

	const glw::GLenum texture_targets[] = {
		GL_TEXTURE_1D,		 GL_TEXTURE_1D_ARRAY,		GL_TEXTURE_2D,
		GL_TEXTURE_2D_ARRAY, GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
		GL_TEXTURE_3D,		 GL_TEXTURE_CUBE_MAP,		GL_TEXTURE_CUBE_MAP_ARRAY,
		GL_TEXTURE_RECTANGLE
	};
	const _test_texture_type texture_types[] = { TEST_TEXTURE_TYPE_NO_STORAGE_ALLOCATED,
												 TEST_TEXTURE_TYPE_IMMUTABLE_TEXTURE_OBJECT,
												 TEST_TEXTURE_TYPE_MUTABLE_TEXTURE_OBJECT,
												 TEST_TEXTURE_TYPE_TEXTURE_VIEW_CREATED_FROM_IMMUTABLE_TEXTURE_OBJECT,
												 TEST_TEXTURE_TYPE_TEXTURE_VIEW_CREATED_FROM_TEXTURE_VIEW };
	const unsigned int n_texture_targets = sizeof(texture_targets) / sizeof(texture_targets[0]);
	const unsigned int n_texture_types   = sizeof(texture_types) / sizeof(texture_types[0]);

	/* Iterate through all texture types supported by the test */
	for (unsigned int n_texture_type = 0; n_texture_type < n_texture_types; ++n_texture_type)
	{
		const _test_texture_type texture_type = texture_types[n_texture_type];

		/* Iterate through all texture targets supported by the test */
		for (unsigned int n_texture_target = 0; n_texture_target < n_texture_targets; ++n_texture_target)
		{
			_test_run		  new_test_run;
			const glw::GLenum texture_target = texture_targets[n_texture_target];

			/* Texture buffers are neither immutable nor mutable. In order to avoid testing
			 * them in both cases, let's assume they are immutable objects */
			if (texture_target == GL_TEXTURE_BUFFER && texture_type == TEST_TEXTURE_TYPE_MUTABLE_TEXTURE_OBJECT)
			{
				continue;
			}

			/* Set up test run properties. Since we're only testing a single
			 * configuration, we can set these to predefined values..
			 */
			const int  n_levels_needed		 = 6;
			glw::GLint n_min_layer			 = 1;
			glw::GLint n_num_layers			 = 2;
			glw::GLint n_min_level			 = 2;
			glw::GLint n_num_levels			 = 3;
			int		   parent_texture_depth  = texture_depth;
			int		   parent_texture_height = texture_height;
			int		   parent_texture_width  = texture_width;

			new_test_run.texture_target = texture_target;
			new_test_run.texture_type   = texture_type;

			/* Take note of target-specific restrictions */
			if (texture_target == GL_TEXTURE_CUBE_MAP || texture_target == GL_TEXTURE_CUBE_MAP_ARRAY)
			{
				n_num_layers = 6 /* layer-faces */ * 2; /* as per spec */

				/* Make sure that cube face width matches its height */
				parent_texture_height = 64;
				parent_texture_width  = 64;

				/* Also change the depth so that there's at least a few layers
				 * we can use in the test for GL_TEXTURE_CUBE_MAP_ARRAY case
				 */
				parent_texture_depth = 64;
			}

			if (texture_target == GL_TEXTURE_CUBE_MAP)
			{
				/* Texture views created from a cube map texture should always
				 * use a minimum layer of zero
				 */
				n_min_layer  = 0;
				n_num_layers = 6;
			}

			if (texture_target == GL_TEXTURE_CUBE_MAP_ARRAY)
			{
				/* Slightly modify the values we'll use for <minlayer>
				 * and <numlayers> arguments passed to glTextureView() calls
				 * so that we can test the "view from view from texture" case
				 */
				n_min_layer = 0;
			}

			if (texture_target == GL_TEXTURE_1D || texture_target == GL_TEXTURE_2D ||
				texture_target == GL_TEXTURE_2D_MULTISAMPLE || texture_target == GL_TEXTURE_3D ||
				texture_target == GL_TEXTURE_RECTANGLE)
			{
				/* All these texture targets are single-layer only. glTextureView()
				 * also requires <numlayers> argument to be set to 1 for them, so
				 * take this into account.
				 **/
				n_min_layer  = 0;
				n_num_layers = 1;
			}

			if (texture_target == GL_TEXTURE_2D_MULTISAMPLE || texture_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY ||
				texture_target == GL_TEXTURE_RECTANGLE)
			{
				/* All these texture targets do not support mip-maps */
				n_min_level = 0;
			}

			/* Initialize parent texture object */
			gl.genTextures(1, &new_test_run.parent_texture_object_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed");

			gl.bindTexture(texture_target, new_test_run.parent_texture_object_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

			if (texture_type != TEST_TEXTURE_TYPE_NO_STORAGE_ALLOCATED)
			{
				TextureViewUtilities::initTextureStorage(gl, (texture_type == TEST_TEXTURE_TYPE_MUTABLE_TEXTURE_OBJECT),
														 texture_target, parent_texture_depth, parent_texture_height,
														 parent_texture_width, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
														 n_levels_needed, n_cubemaps_needed, 0); /* bo_id */
			}

			/* Update expected view-specific property values to include interactions
			 * with immutable textures. */
			if (texture_type == TEST_TEXTURE_TYPE_IMMUTABLE_TEXTURE_OBJECT ||
				texture_type == TEST_TEXTURE_TYPE_TEXTURE_VIEW_CREATED_FROM_IMMUTABLE_TEXTURE_OBJECT ||
				texture_type == TEST_TEXTURE_TYPE_TEXTURE_VIEW_CREATED_FROM_TEXTURE_VIEW)
			{
				/* Set expected GL_TEXTURE_IMMUTABLE_LEVELS property value to the number
				 * of levels we'll be using for the immutable texture storage. For selected
				 * texture targets that do no take <levels> argument, we'll change this
				 * value on a case-by-case basis.
				 */
				new_test_run.expected_n_immutable_levels = n_levels_needed;

				/* Set expected GL_TEXTURE_VIEW_NUM_LAYERS property value to 1, as per GL spec.
				 * This value will be modified for selected texture targets */
				new_test_run.expected_n_num_layers = 1;

				/* Configured expected GL_TEXTURE_VIEW_NUM_LEVELS value as per GL spec */
				new_test_run.expected_n_num_levels = n_levels_needed;

				/* Initialize immutable texture storage */
				switch (texture_target)
				{
				case GL_TEXTURE_1D_ARRAY:
				{
					/* Update expected GL_TEXTURE_VIEW_NUM_LAYERS property value as per GL specification */
					new_test_run.expected_n_num_layers = texture_height;

					break;
				}

				case GL_TEXTURE_CUBE_MAP:
				{
					/* Update expected GL_TEXTURE_VIEW_NUM_LAYERS property value as per GL specification */
					new_test_run.expected_n_num_layers = 6;

					break;
				}

				case GL_TEXTURE_RECTANGLE:
				{
					new_test_run.expected_n_immutable_levels = 1;
					new_test_run.expected_n_num_levels		 = 1;

					break;
				}

				case GL_TEXTURE_2D_ARRAY:
				{
					/* Update expected GL_TEXTURE_VIEW_NUM_LAYERS property value as per GL specification */
					new_test_run.expected_n_num_layers = texture_depth;

					break;
				}

				case GL_TEXTURE_2D_MULTISAMPLE:
				{
					/* 2D multisample texture are not mip-mapped, so update
					 * expected GL_TEXTURE_IMMUTABLE_LEVELS and GL_TEXTURE_VIEW_NUM_LEVELS
					 * value accordingly */
					new_test_run.expected_n_immutable_levels = 1;
					new_test_run.expected_n_num_levels		 = 1;

					break;
				}

				case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
				{
					/* 2D multisample array textures are not mip-mapped, so update
					 * expected GL_TEXTURE_IMMUTABLE_LEVELS and GL_TEXTURE_VIEW_NUM_LEVELS
					 * values accordingly */
					new_test_run.expected_n_immutable_levels = 1;
					new_test_run.expected_n_num_levels		 = 1;

					/* Update expected GL_TEXTURE_VIEW_NUM_LAYERS property value as per GL specification */
					new_test_run.expected_n_num_layers = texture_depth;

					break;
				}

				case GL_TEXTURE_CUBE_MAP_ARRAY:
				{
					const unsigned int actual_texture_depth = 6 /* layer-faces */ * n_cubemaps_needed;

					/* Update expected GL_TEXTURE_VIEW_NUM_LAYERS property value as per GL specification */
					new_test_run.expected_n_num_layers = actual_texture_depth;

					break;
				}
				} /* switch (texture_target) */
			}

			/* Initialize the view(s) */
			if (texture_type == TEST_TEXTURE_TYPE_TEXTURE_VIEW_CREATED_FROM_IMMUTABLE_TEXTURE_OBJECT ||
				texture_type == TEST_TEXTURE_TYPE_TEXTURE_VIEW_CREATED_FROM_TEXTURE_VIEW)
			{
				const unsigned int n_iterations =
					(texture_type == TEST_TEXTURE_TYPE_TEXTURE_VIEW_CREATED_FROM_TEXTURE_VIEW) ? 2 : 1;

				for (unsigned int n_iteration = 0; n_iteration < n_iterations; ++n_iteration)
				{
					glw::GLuint* parent_id_ptr = (n_iteration == 0) ?
													 &new_test_run.parent_texture_object_id :
													 &new_test_run.texture_view_object_created_from_immutable_to_id;
					glw::GLuint* view_id_ptr = (n_iteration == 0) ?
												   &new_test_run.texture_view_object_created_from_immutable_to_id :
												   &new_test_run.texture_view_object_created_from_view_to_id;

					gl.genTextures(1, view_id_ptr);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed");

					gl.textureView(*view_id_ptr, new_test_run.texture_target, *parent_id_ptr,
								   GL_RGBA8, /* use the parent texture object's internalformat */
								   n_min_level, n_num_levels, n_min_layer, n_num_layers);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureView() call failed");

					/* Query parent object's properties */
					glw::GLint parent_min_level			 = -1;
					glw::GLint parent_min_layer			 = -1;
					glw::GLint parent_num_layers		 = -1;
					glw::GLint parent_num_levels		 = -1;
					glw::GLint parent_n_immutable_levels = -1;

					gl.bindTexture(texture_target, *parent_id_ptr);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

					gl.getTexParameteriv(texture_target, GL_TEXTURE_IMMUTABLE_LEVELS, &parent_n_immutable_levels);
					GLU_EXPECT_NO_ERROR(
						gl.getError(),
						"glGetTexParameteriv() failed for GL_TEXTURE_IMMUTABLE_LEVELS pname queried for parent object");

					gl.getTexParameteriv(texture_target, GL_TEXTURE_VIEW_MIN_LAYER, &parent_min_layer);
					GLU_EXPECT_NO_ERROR(
						gl.getError(),
						"glGetTexParameteriv() failed for GL_TEXTURE_VIEW_MIN_LAYER pname queried for parent object");

					gl.getTexParameteriv(texture_target, GL_TEXTURE_VIEW_MIN_LEVEL, &parent_min_level);
					GLU_EXPECT_NO_ERROR(
						gl.getError(),
						"glGetTexParameteriv() failed for GL_TEXTURE_VIEW_MIN_LEVEL pname queried for parent object");

					gl.getTexParameteriv(texture_target, GL_TEXTURE_VIEW_NUM_LAYERS, &parent_num_layers);
					GLU_EXPECT_NO_ERROR(
						gl.getError(),
						"glGetTexParameteriv() failed for GL_TEXTURE_VIEW_NUM_LAYERS pname queried for parent object");

					gl.getTexParameteriv(texture_target, GL_TEXTURE_VIEW_NUM_LEVELS, &parent_num_levels);
					GLU_EXPECT_NO_ERROR(
						gl.getError(),
						"glGetTexParameteriv() failed for GL_TEXTURE_VIEW_NUM_LEVELS pname queried for parent object");

					/* Update test run-specific expected values as per GL_ARB_texture_view extension specification */
					/*
					 * - TEXTURE_IMMUTABLE_LEVELS is set to the value of TEXTURE_IMMUTABLE_LEVELS
					 *   from the original texture.
					 */
					new_test_run.expected_n_immutable_levels = parent_n_immutable_levels;

					/*
					 * - TEXTURE_VIEW_MIN_LEVEL is set to <minlevel> plus the value of
					 *   TEXTURE_VIEW_MIN_LEVEL from the original texture.
					 */
					new_test_run.expected_n_min_level = n_min_level + parent_min_level;

					/*
					 * - TEXTURE_VIEW_MIN_LAYER is set to <minlayer> plus the value of
					 *   TEXTURE_VIEW_MIN_LAYER from the original texture.
					 */
					new_test_run.expected_n_min_layer = n_min_layer + parent_min_layer;

					/*
					 * - TEXTURE_VIEW_NUM_LAYERS is set to the lesser of <numlayers> and the
					 *   value of TEXTURE_VIEW_NUM_LAYERS from the original texture minus
					 *   <minlayer>.
					 *
					 */
					if ((parent_num_layers - n_min_layer) < n_num_layers)
					{
						new_test_run.expected_n_num_layers = parent_num_layers - n_min_layer;
					}
					else
					{
						new_test_run.expected_n_num_layers = n_num_layers;
					}

					/*
					 * - TEXTURE_VIEW_NUM_LEVELS is set to the lesser of <numlevels> and the
					 *   value of TEXTURE_VIEW_NUM_LEVELS from the original texture minus
					 *   <minlevels>.
					 *
					 */
					if ((parent_num_levels - n_min_level) < n_num_levels)
					{
						new_test_run.expected_n_num_levels = parent_num_levels - n_min_level;
					}
					else
					{
						new_test_run.expected_n_num_levels = n_num_levels;
					}
				} /* for (all iterations) */
			}	 /* if (texture_type == TEST_TEXTURE_TYPE_TEXTURE_VIEW_CREATED_FROM_IMMUTABLE_TEXTURE_OBJECT ||
			 texture_type == TEST_TEXTURE_TYPE_TEXTURE_VIEW_CREATED_FROM_TEXTURE_VIEW) */

			/* Store the descriptor */
			m_test_runs.push_back(new_test_run);
		} /* for (all texture targets) */
	}	 /* for (all texture types) */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult TextureViewTestGetTexParameter::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Make sure GL_ARB_texture_view is reported as supported before carrying on
	 * with actual execution */
	const std::vector<std::string>& extensions = m_context.getContextInfo().getExtensions();

	if (std::find(extensions.begin(), extensions.end(), "GL_ARB_texture_view") == extensions.end())
	{
		throw tcu::NotSupportedError("GL_ARB_texture_view is not supported");
	}

	/* Initialize all objects necessary to execute the test */
	initTestRuns();

	/* Iterate through all test runs and issue the queries */
	for (_test_runs_const_iterator test_run_iterator = m_test_runs.begin(); test_run_iterator != m_test_runs.end();
		 test_run_iterator++)
	{
		glw::GLfloat	 query_texture_immutable_levels_value_float = -1.0f;
		glw::GLint		 query_texture_immutable_levels_value_int   = -1;
		glw::GLfloat	 query_texture_view_min_layer_value_float   = -1.0f;
		glw::GLint		 query_texture_view_min_layer_value_int		= -1;
		glw::GLfloat	 query_texture_view_min_level_value_float   = -1.0f;
		glw::GLint		 query_texture_view_min_level_value_int		= -1;
		glw::GLfloat	 query_texture_view_num_layers_value_float  = -1.0f;
		glw::GLint		 query_texture_view_num_layers_value_int	= -1;
		glw::GLfloat	 query_texture_view_num_levels_value_float  = -1.0f;
		glw::GLint		 query_texture_view_num_levels_value_int	= -1;
		const _test_run& test_run									= *test_run_iterator;
		glw::GLint		 texture_object_id							= 0;

		switch (test_run.texture_type)
		{
		case TEST_TEXTURE_TYPE_IMMUTABLE_TEXTURE_OBJECT:
			texture_object_id = test_run.parent_texture_object_id;
			break;
		case TEST_TEXTURE_TYPE_MUTABLE_TEXTURE_OBJECT:
			texture_object_id = test_run.parent_texture_object_id;
			break;
		case TEST_TEXTURE_TYPE_NO_STORAGE_ALLOCATED:
			texture_object_id = test_run.parent_texture_object_id;
			break;
		case TEST_TEXTURE_TYPE_TEXTURE_VIEW_CREATED_FROM_IMMUTABLE_TEXTURE_OBJECT:
			texture_object_id = test_run.texture_view_object_created_from_immutable_to_id;
			break;
		case TEST_TEXTURE_TYPE_TEXTURE_VIEW_CREATED_FROM_TEXTURE_VIEW:
			texture_object_id = test_run.texture_view_object_created_from_view_to_id;
			break;

		default:
		{
			TCU_FAIL("Unrecognized texture type");
		}
		}

		/* Bind the texture object of our interest to the target */
		gl.bindTexture(test_run.texture_target, texture_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

		/* Run all the queries */
		gl.getTexParameterfv(test_run.texture_target, GL_TEXTURE_IMMUTABLE_LEVELS,
							 &query_texture_immutable_levels_value_float);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexParameterfv() failed for GL_TEXTURE_IMMUTABLE_LEVELS pname");

		gl.getTexParameteriv(test_run.texture_target, GL_TEXTURE_IMMUTABLE_LEVELS,
							 &query_texture_immutable_levels_value_int);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexPrameteriv() failed for GL_TEXTURE_IMMUTABLE_LEVELS pname");

		gl.getTexParameterfv(test_run.texture_target, GL_TEXTURE_VIEW_MIN_LAYER,
							 &query_texture_view_min_layer_value_float);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexParameterfv() failed for GL_TEXTURE_VIEW_MIN_LAYER pname");

		gl.getTexParameteriv(test_run.texture_target, GL_TEXTURE_VIEW_MIN_LAYER,
							 &query_texture_view_min_layer_value_int);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexParameteriv() failed for GL_TEXTURE_VIEW_MIN_LAYER pname");

		gl.getTexParameterfv(test_run.texture_target, GL_TEXTURE_VIEW_MIN_LEVEL,
							 &query_texture_view_min_level_value_float);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexParameterfv() failed for GL_TEXTURE_VIEW_MIN_LEVEL pname");

		gl.getTexParameteriv(test_run.texture_target, GL_TEXTURE_VIEW_MIN_LEVEL,
							 &query_texture_view_min_level_value_int);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexParameteriv() failed for GL_TEXTURE_VIEW_MIN_LEVEL pname");

		gl.getTexParameterfv(test_run.texture_target, GL_TEXTURE_VIEW_NUM_LAYERS,
							 &query_texture_view_num_layers_value_float);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexParameterfv() failed for GL_TEXTURE_VIEW_NUM_LAYERS pname");

		gl.getTexParameteriv(test_run.texture_target, GL_TEXTURE_VIEW_NUM_LAYERS,
							 &query_texture_view_num_layers_value_int);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexParameteriv() failed for GL_TEXTURE_VIEW_NUM_LAYERS pname");

		gl.getTexParameterfv(test_run.texture_target, GL_TEXTURE_VIEW_NUM_LEVELS,
							 &query_texture_view_num_levels_value_float);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexParameterfv() failed for GL_TEXTURE_VIEW_NUM_LEVELS pname");

		gl.getTexParameteriv(test_run.texture_target, GL_TEXTURE_VIEW_NUM_LEVELS,
							 &query_texture_view_num_levels_value_int);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexParameteriv() failed for GL_TEXTURE_VIEW_NUM_LEVELS pname");

		/* Verify the results */
		const float epsilon = 1e-5f;

		if (de::abs(query_texture_immutable_levels_value_float - (float)test_run.expected_n_immutable_levels) > epsilon)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid floating-point value reported for GL_TEXTURE_IMMUTABLE_LEVELS pname "
							   << "(expected: " << test_run.expected_n_immutable_levels
							   << " found: " << query_texture_immutable_levels_value_float << ")."
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid FP value reported for GL_TEXTURE_IMMUTABLE_LEVELS pname");
		}

		if (query_texture_immutable_levels_value_int != test_run.expected_n_immutable_levels)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid integer value reported for GL_TEXTURE_IMMUTABLE_LEVELS pname "
							   << "(expected: " << test_run.expected_n_immutable_levels
							   << " found: " << query_texture_immutable_levels_value_int << ")."
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid FP value reported for GL_TEXTURE_IMMUTABLE_LEVELS pname");
		}

		if (de::abs(query_texture_view_min_layer_value_float - (float)test_run.expected_n_min_layer) > epsilon)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid floating-point value reported for GL_TEXTURE_VIEW_MIN_LAYER pname "
							   << "(expected: " << test_run.expected_n_min_layer
							   << " found: " << query_texture_view_min_layer_value_float << ")."
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid FP value reported for GL_TEXTURE_VIEW_MIN_LAYER pname");
		}

		if (query_texture_view_min_layer_value_int != test_run.expected_n_min_layer)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid integer value reported for GL_TEXTURE_VIEW_MIN_LAYER pname "
							   << "(expected: " << test_run.expected_n_min_layer
							   << " found: " << query_texture_view_min_layer_value_int << ")."
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid FP value reported for GL_TEXTURE_VIEW_MIN_LAYER pname");
		}

		if (de::abs(query_texture_view_min_level_value_float - (float)test_run.expected_n_min_level) > epsilon)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid floating-point value reported for GL_TEXTURE_VIEW_MIN_LEVEL pname "
							   << "(expected: " << test_run.expected_n_min_level
							   << " found: " << query_texture_view_min_level_value_float << ")."
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid FP value reported for GL_TEXTURE_VIEW_MIN_LEVEL pname");
		}

		if (query_texture_view_min_level_value_int != test_run.expected_n_min_level)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid integer value reported for GL_TEXTURE_VIEW_MIN_LEVEL pname "
							   << "(expected: " << test_run.expected_n_min_level
							   << " found: " << query_texture_view_min_level_value_int << ")."
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid FP value reported for GL_TEXTURE_VIEW_MIN_LEVEL pname");
		}

		if (de::abs(query_texture_view_num_layers_value_float - (float)test_run.expected_n_num_layers) > epsilon)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid floating-point value reported for GL_TEXTURE_VIEW_NUM_LAYERS pname "
							   << "(expected: " << test_run.expected_n_num_layers
							   << " found: " << query_texture_view_num_layers_value_float << ")."
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid FP value reported for GL_TEXTURE_VIEW_NUM_LAYERS pname");
		}

		if (query_texture_view_num_layers_value_int != test_run.expected_n_num_layers)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid integer value reported for GL_TEXTURE_VIEW_NUM_LAYERS pname "
							   << "(expected: " << test_run.expected_n_num_layers
							   << " found: " << query_texture_view_num_layers_value_int << ")."
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid FP value reported for GL_TEXTURE_VIEW_NUM_LAYERS pname");
		}

		if (de::abs(query_texture_view_num_levels_value_float - (float)test_run.expected_n_num_levels) > epsilon)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid floating-point value reported for GL_TEXTURE_VIEW_NUM_LEVELS pname "
							   << "(expected: " << test_run.expected_n_num_levels
							   << " found: " << query_texture_view_num_levels_value_float << ")."
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid FP value reported for GL_TEXTURE_VIEW_NUM_LEVELS pname");
		}

		if (query_texture_view_num_levels_value_int != test_run.expected_n_num_levels)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid integer value reported for GL_TEXTURE_VIEW_NUM_LEVELS pname "
							   << "(expected: " << test_run.expected_n_num_levels
							   << " found: " << query_texture_view_num_levels_value_int << ")."
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid FP value reported for GL_TEXTURE_VIEW_NUM_LEVELS pname");
		}
	} /* for (all test runs) */

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context
 **/
TextureViewTestErrors::TextureViewTestErrors(deqp::Context& context)
	: TestCase(context, "errors", "test_description")
	, m_bo_id(0)
	, m_reference_immutable_to_1d_id(0)
	, m_reference_immutable_to_2d_id(0)
	, m_reference_immutable_to_2d_array_id(0)
	, m_reference_immutable_to_2d_array_32_by_33_id(0)
	, m_reference_immutable_to_2d_multisample_id(0)
	, m_reference_immutable_to_3d_id(0)
	, m_reference_immutable_to_cube_map_id(0)
	, m_reference_immutable_to_cube_map_array_id(0)
	, m_reference_immutable_to_rectangle_id(0)
	, m_reference_mutable_to_2d_id(0)
	, m_test_modified_to_id_1(0)
	, m_test_modified_to_id_2(0)
	, m_test_modified_to_id_3(0)
	, m_view_bound_to_id(0)
	, m_view_never_bound_to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes all GL objects that may have been generated for the test. */
void TextureViewTestErrors::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	if (m_reference_immutable_to_1d_id != 0)
	{
		gl.deleteTextures(1, &m_reference_immutable_to_1d_id);

		m_reference_immutable_to_1d_id = 0;
	}

	if (m_reference_immutable_to_2d_id != 0)
	{
		gl.deleteTextures(1, &m_reference_immutable_to_2d_id);

		m_reference_immutable_to_2d_id = 0;
	}

	if (m_reference_immutable_to_2d_array_id != 0)
	{
		gl.deleteTextures(1, &m_reference_immutable_to_2d_array_id);

		m_reference_immutable_to_2d_array_id = 0;
	}

	if (m_reference_immutable_to_2d_array_32_by_33_id != 0)
	{
		gl.deleteTextures(1, &m_reference_immutable_to_2d_array_32_by_33_id);

		m_reference_immutable_to_2d_array_32_by_33_id = 0;
	}

	if (m_reference_immutable_to_2d_multisample_id != 0)
	{
		gl.deleteTextures(1, &m_reference_immutable_to_2d_multisample_id);

		m_reference_immutable_to_2d_multisample_id = 0;
	}

	if (m_reference_immutable_to_3d_id != 0)
	{
		gl.deleteTextures(1, &m_reference_immutable_to_3d_id);

		m_reference_immutable_to_3d_id = 0;
	}

	if (m_reference_immutable_to_cube_map_id != 0)
	{
		gl.deleteTextures(1, &m_reference_immutable_to_cube_map_id);

		m_reference_immutable_to_cube_map_id = 0;
	}

	if (m_reference_immutable_to_cube_map_array_id != 0)
	{
		gl.deleteTextures(1, &m_reference_immutable_to_cube_map_array_id);

		m_reference_immutable_to_cube_map_array_id = 0;
	}

	if (m_reference_immutable_to_rectangle_id != 0)
	{
		gl.deleteTextures(1, &m_reference_immutable_to_rectangle_id);

		m_reference_immutable_to_rectangle_id = 0;
	}

	if (m_reference_mutable_to_2d_id != 0)
	{
		gl.deleteTextures(1, &m_reference_mutable_to_2d_id);

		m_reference_mutable_to_2d_id = 0;
	}

	if (m_test_modified_to_id_1 != 0)
	{
		gl.deleteTextures(1, &m_test_modified_to_id_1);

		m_test_modified_to_id_1 = 0;
	}

	if (m_test_modified_to_id_2 != 0)
	{
		gl.deleteTextures(1, &m_test_modified_to_id_2);

		m_test_modified_to_id_2 = 0;
	}

	if (m_test_modified_to_id_3 != 0)
	{
		gl.deleteTextures(1, &m_test_modified_to_id_3);

		m_test_modified_to_id_3 = 0;
	}

	if (m_view_bound_to_id != 0)
	{
		gl.deleteTextures(1, &m_view_bound_to_id);

		m_view_bound_to_id = 0;
	}

	if (m_view_never_bound_to_id != 0)
	{
		gl.deleteTextures(1, &m_view_never_bound_to_id);

		m_view_never_bound_to_id = 0;
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult TextureViewTestErrors::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Make sure GL_ARB_texture_view is reported as supported before carrying on
	 * with actual execution */
	const std::vector<std::string>& extensions = m_context.getContextInfo().getExtensions();

	if (std::find(extensions.begin(), extensions.end(), "GL_ARB_texture_view") == extensions.end())
	{
		throw tcu::NotSupportedError("GL_ARB_texture_view is not supported");
	}

	/* Create a buffer object that we'll need to use to define storage of
	 * buffer textures */
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed");

	gl.bindBuffer(GL_TEXTURE_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed");

	gl.bufferData(GL_TEXTURE_BUFFER, 123, /* arbitrary size */
				  DE_NULL,				  /* data */
				  GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed");

	/* Create reference texture objects */
	const glw::GLint  reference_bo_id			  = m_bo_id;
	const glw::GLint  reference_to_depth		  = 2;
	const glw::GLenum reference_to_format		  = GL_RGBA;
	const glw::GLint  reference_to_height		  = 64;
	const glw::GLenum reference_to_internalformat = GL_RGBA32F;
	const glw::GLint  reference_n_cubemaps		  = 1;
	const glw::GLint  reference_n_levels		  = 1;
	const glw::GLenum reference_to_type			  = GL_FLOAT;
	const glw::GLint  reference_to_width		  = 64;

	gl.genTextures(1, &m_reference_immutable_to_1d_id);
	gl.genTextures(1, &m_reference_immutable_to_2d_id);
	gl.genTextures(1, &m_reference_immutable_to_2d_array_id);
	gl.genTextures(1, &m_reference_immutable_to_2d_array_32_by_33_id);
	gl.genTextures(1, &m_reference_immutable_to_2d_multisample_id);
	gl.genTextures(1, &m_reference_immutable_to_3d_id);
	gl.genTextures(1, &m_reference_immutable_to_cube_map_id);
	gl.genTextures(1, &m_reference_immutable_to_cube_map_array_id);
	gl.genTextures(1, &m_reference_immutable_to_rectangle_id);
	gl.genTextures(1, &m_reference_mutable_to_2d_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call(s) failed");

	/* Retrieve GL_SAMPLES value - we'll need it to initialize multisample storage */
	glw::GLint gl_max_samples_value = 0;

	gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, reference_to_internalformat, GL_SAMPLES,
						   1 /* bufSize - first result */, &gl_max_samples_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInternalformativ() failed for GL_SAMPLES pname");

	/* Set up texture storage for single-dimensional texture object */
	gl.bindTexture(GL_TEXTURE_1D, m_reference_immutable_to_1d_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	gl.texStorage1D(GL_TEXTURE_1D, reference_n_levels, reference_to_internalformat, reference_to_width);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage1D() call failed");

	/* Set up immutable texture storage for two-dimensional texture object */
	gl.bindTexture(GL_TEXTURE_2D, m_reference_immutable_to_2d_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	gl.texStorage2D(GL_TEXTURE_2D, reference_n_levels, reference_to_internalformat, reference_to_width,
					reference_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed");

	/* Set up immutable texture storage for two-dimensional array texture object */
	gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_reference_immutable_to_2d_array_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	gl.texStorage3D(GL_TEXTURE_2D_ARRAY, reference_n_levels, reference_to_internalformat, reference_to_width,
					reference_to_height, reference_to_depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D() call failed");

	/* Set up immutable texture storage for two-dimensional array texture object, base
	 * level of which uses a resolution of 32x33. We'll need it to check case r) */
	gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_reference_immutable_to_2d_array_32_by_33_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	gl.texStorage3D(GL_TEXTURE_2D_ARRAY, reference_n_levels, reference_to_internalformat, 32, /* width */
					33,																		  /* height */
					6); /* depth - 6 layers so that a cube-map/cube-map array view can be created from this texture */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D() call failed");

	/* Set up immutable texture storage for two-dimensional multisample texture object */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_reference_immutable_to_2d_multisample_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, gl_max_samples_value, reference_to_internalformat,
							   reference_to_width, reference_to_height, GL_TRUE); /* fixedsamplelocations */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample() call failed");

	/* Set up immutable texture storage for three-dimensional texture object */
	gl.bindTexture(GL_TEXTURE_3D, m_reference_immutable_to_3d_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	gl.texStorage3D(GL_TEXTURE_3D, reference_n_levels, reference_to_internalformat, reference_to_width,
					reference_to_height, reference_to_depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D() call failed");

	/* Set up immutable texture storage for cube-map texture object */
	gl.bindTexture(GL_TEXTURE_CUBE_MAP, m_reference_immutable_to_cube_map_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	gl.texStorage2D(GL_TEXTURE_CUBE_MAP, reference_n_levels, reference_to_internalformat, reference_to_width,
					reference_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed");

	/* Set up immutable texture storage for cube-map array texture object */
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_reference_immutable_to_cube_map_array_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, reference_n_levels, reference_to_internalformat, reference_to_width,
					reference_to_height, 6 /* layer-faces */ * reference_to_depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed");

	/* Set up immutable texture storage for rectangular texture object */
	gl.bindTexture(GL_TEXTURE_RECTANGLE, m_reference_immutable_to_rectangle_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	gl.texStorage2D(GL_TEXTURE_RECTANGLE, reference_n_levels, reference_to_internalformat, reference_to_width,
					reference_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed");

	/* Set up mutable texture storage for two-dimensional texture object */
	gl.bindTexture(GL_TEXTURE_2D, m_reference_mutable_to_2d_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	for (glw::GLint n_level = 0; n_level < reference_n_levels; ++n_level)
	{
		gl.texImage2D(GL_TEXTURE_2D, n_level, reference_to_internalformat, reference_to_width << n_level,
					  reference_to_height << n_level, 0,				/* border */
					  reference_to_format, reference_to_type, DE_NULL); /* pixels */

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D() call failed");
	}

	/* Create texture objects we'll be attempting to define as texture views */
	gl.genTextures(1, &m_view_bound_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed");

	gl.genTextures(1, &m_view_never_bound_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed");

	gl.bindTexture(GL_TEXTURE_2D, m_view_bound_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	/* a) GL_INVALID_VALUE should be generated if <texture> is 0. */
	glw::GLint error_code = GL_NO_ERROR;

	gl.textureView(0,																			  /* texture */
				   GL_TEXTURE_2D, m_reference_immutable_to_2d_id, reference_to_internalformat, 0, /* minlevel */
				   reference_n_levels, 0,														  /* minlayer */
				   1);																			  /* numlayers */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
						   << "]"
							  " error generated when passing <texture> argument of 0"
							  " to a glTextureView(), whereas GL_INVALID_VALUE was "
							  "expected."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("GL_INVALID_VALUE not generated when passing 0 as <texture> argument to a "
				 "glTextureView() call.");
	}

	/* b) GL_INVALID_OPERATION should be generated if <texture> is not
	 *    a valid name returned by glGenTextures().
	 */
	const glw::GLint invalid_to_id = 0xFFFFFFFF;

	gl.textureView(invalid_to_id, GL_TEXTURE_2D, m_reference_immutable_to_2d_id, reference_to_internalformat,
				   0,					  /* minlevel */
				   reference_n_levels, 0, /* minlayer */
				   1);					  /* numlayers */

	error_code = gl.getError();
	if (error_code != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
						   << "]"
							  " error generated when passing <texture> argument of"
							  " value that does not correspond to a valid texture "
							  "object ID, whereas GL_INVALID_OPERATION was expected."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("GL_INVALID_OPERATION not generated when passing 0xFFFFFFFF as <texture> "
				 "argument to a glTextureView() call.");
	}

	/* c) GL_INVALID_OPERATION should be generated if <texture> has
	 *    already been bound and given a target.
	 */
	gl.textureView(m_view_bound_to_id, GL_TEXTURE_2D, m_reference_immutable_to_2d_id, reference_to_internalformat,
				   0,					  /* minlevel */
				   reference_n_levels, 0, /* minlayer */
				   1);					  /* numlayers */

	error_code = gl.getError();
	if (error_code != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
						   << "]"
							  " error generated when passing <texture> argument "
							  " that refers to an ID of a texture object that has "
							  "already been bound to a texture target, whereas "
							  "GL_INVALID_OPERATION was expected."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("GL_INVALID_OPERATION not generated when passing <texture> set"
				 " to an ID of a texture object, that has already been bound to"
				 " a texture target, to a glTextureView() call.");
	}

	/* d) GL_INVALID_VALUE should be generated if <origtexture> is not
	 *    the name of a texture object.
	 */
	gl.textureView(m_view_never_bound_to_id, GL_TEXTURE_2D, invalid_to_id, reference_to_internalformat,
				   0,					  /* minlevel */
				   reference_n_levels, 0, /* minlayer */
				   1);					  /* numlayers */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
						   << "]"
							  " error generated when passing <origtexture> argument "
							  " of value 0xFFFFFFFF, whereas GL_INVALID_VALUE was "
							  "expected."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("GL_INVALID_VALUE not generated when passing an invalid ID of a texture "
				 "object to <origtexture> argument.");
	}

	/* e) GL_INVALID_OPERATION error should be generated if <origtexture>
	 *    is a mutable texture object.
	 */
	gl.textureView(m_view_never_bound_to_id, GL_TEXTURE_2D, m_reference_mutable_to_2d_id, reference_to_internalformat,
				   0,					  /* minlevel */
				   reference_n_levels, 0, /* minlayer */
				   1);					  /* numlayers */

	error_code = gl.getError();
	if (error_code != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
						   << "]"
							  " error generated when passing <origtexture> argument "
							  " set to refer to a mutable texture object, whereas "
							  "GL_INVALID_OPERATION was expected."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("GL_INVALID_OPERATION not generated when passing an ID of a mutable "
				 "texture object through <origtexture> argument.");
	}

	/* f) GL_INVALID_OPERATION error should be generated whenever the
	 *    application tries to generate a texture view for a target
	 *    that is incompatible with original texture's target. (as per
	 *    table 8.20 from OpenGL 4.3 specification)
	 *
	 *   NOTE: All invalid original+view texture target combinations
	 *         should be checked.
	 */
	TextureViewUtilities::_incompatible_texture_target_pairs incompatible_texture_target_pairs =
		TextureViewUtilities::getIllegalTextureAndViewTargetCombinations();

	for (TextureViewUtilities::_incompatible_texture_target_pairs_const_iterator pair_iterator =
			 incompatible_texture_target_pairs.begin();
		 pair_iterator != incompatible_texture_target_pairs.end(); pair_iterator++)
	{
		TextureViewUtilities::_internalformat_pair texture_target_pair	 = *pair_iterator;
		glw::GLenum								   original_texture_target = texture_target_pair.first;
		glw::GLenum								   view_texture_target	 = texture_target_pair.second;

		/* Generate texture IDs */
		gl.genTextures(1, &m_test_modified_to_id_1);
		gl.genTextures(1, &m_test_modified_to_id_2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call(s) failed");

		/* Configure reference texture object storage */
		gl.bindTexture(original_texture_target, m_test_modified_to_id_1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

		TextureViewUtilities::initTextureStorage(gl, true, /* create mutable parent texture */
												 original_texture_target, reference_to_depth, reference_to_height,
												 reference_to_width, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
												 reference_n_levels, reference_n_cubemaps, reference_bo_id);

		/* Attempt to create the invalid view */
		gl.textureView(m_test_modified_to_id_2,						 /* texture */
					   view_texture_target, m_test_modified_to_id_1, /* origtexture */
					   reference_to_internalformat, 0,				 /* minlevel */
					   reference_n_levels, 0,						 /* minlayer */
					   1);											 /* numlayers */

		error_code = gl.getError();
		if (error_code != GL_INVALID_OPERATION)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
							   << "]"
								  " error generated when passing <origtexture> argument "
								  " set to refer to a mutable texture object, whereas "
								  "GL_INVALID_OPERATION was expected."
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("GL_INVALID_OPERATION not generated when passing an ID of a mutable "
					 "texture object through <origtexture> argument.");
		}

		/* Release the texture IDs */
		gl.deleteTextures(1, &m_test_modified_to_id_1);
		m_test_modified_to_id_1 = 0;

		gl.deleteTextures(1, &m_test_modified_to_id_2);
		m_test_modified_to_id_2 = 0;

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() call(s) failed");
	} /* for (all incompatible texture target pairs) */

	/* g) GL_INVALID_OPERATION error should be generated whenever the
	 *    application tries to create a texture view, internal format
	 *    of which can be found in table 8.21 of OpenGL 4.4
	 *    specification, and the texture view's internal format is
	 *    incompatible with parent object's internal format. Both
	 *    textures and views should be used as parent objects for the
	 *    purpose of the test.
	 *
	 * NOTE: All invalid texture view internal formats should be
	 *       checked for all applicable original object's internal
	 *       formats
	 */
	glw::GLint context_major_version = 0;
	glw::GLint context_minor_version = 0;

	TextureViewUtilities::getMajorMinorVersionFromContextVersion(m_context.getRenderContext().getType(),
																 &context_major_version, &context_minor_version);

	TextureViewUtilities::_incompatible_internalformat_pairs internalformat_pairs =
		TextureViewUtilities::getIllegalTextureAndViewInternalformatCombinations();

	for (TextureViewUtilities::_incompatible_internalformat_pairs::const_iterator pair_iterator =
			 internalformat_pairs.begin();
		 pair_iterator != internalformat_pairs.end(); pair_iterator++)
	{
		glw::GLenum src_internalformat  = pair_iterator->first;
		glw::GLenum view_internalformat = pair_iterator->second;

		/* Only run the test for internalformats supported by the tested OpenGL implementation */
		if (!TextureViewUtilities::isInternalformatSupported(src_internalformat, context_major_version,
															 context_minor_version) ||
			!TextureViewUtilities::isInternalformatSupported(view_internalformat, context_major_version,
															 context_minor_version))
		{
			/* Next iteration, please */
			continue;
		}

		/* Generate texture IDs */
		gl.genTextures(1, &m_test_modified_to_id_1);
		gl.genTextures(1, &m_test_modified_to_id_2);
		gl.genTextures(1, &m_test_modified_to_id_3);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call(s) failed");

		/* Configure reference texture object storage */
		gl.bindTexture(GL_TEXTURE_2D, m_test_modified_to_id_1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

		TextureViewUtilities::initTextureStorage(
			gl, false,		  /* views require immutable parent texture objects */
			GL_TEXTURE_2D, 0, /* texture_depth */
			reference_to_height, reference_to_width, src_internalformat,
			GL_NONE,			   /* texture_format - not needed for immutable texture objects */
			GL_NONE,			   /* texture_type   - not needed for immutable texture objects */
			reference_n_levels, 0, /* n_cubemaps_needed */
			0);					   /* bo_id */

		/* Attempt to create an invalid view */
		gl.textureView(m_test_modified_to_id_2,				   /* texture */
					   GL_TEXTURE_2D, m_test_modified_to_id_1, /* origtexture */
					   view_internalformat, 0,				   /* minlevel */
					   reference_n_levels, 0,				   /* minlayer */
					   1);									   /* numlayers */

		error_code = gl.getError();
		if (error_code != GL_INVALID_OPERATION)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
							   << "]"
								  " error generated when requesting a view that uses "
								  " an internalformat that is incompatible with parent "
								  " texture object's, whereas GL_INVALID_OPERATION was "
								  "expected."
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("GL_INVALID_OPERATION not generated when requesting a texture view that "
					 "uses an internalformat which is incompatible with parent texture's.");
		}

		/* Create a valid view now */
		gl.textureView(m_test_modified_to_id_2,				   /* texture */
					   GL_TEXTURE_2D, m_test_modified_to_id_1, /* origtexture */
					   src_internalformat, 0,				   /* minlevel */
					   reference_n_levels, 0,				   /* minlayer */
					   1);									   /* numlayers */

		GLU_EXPECT_NO_ERROR(gl.getError(), "A valid glTextureView() call failed");

		/* Attempt to create an invalid view, using the view we've just created
		 * as a parent */
		gl.textureView(m_test_modified_to_id_3,				   /* texture */
					   GL_TEXTURE_2D, m_test_modified_to_id_2, /* origtexture */
					   view_internalformat, 0,				   /* minlevel */
					   reference_n_levels, 0,				   /* minlayer */
					   1);									   /* numlayers */

		error_code = gl.getError();
		if (error_code != GL_INVALID_OPERATION)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
							   << "]"
								  " error generated when requesting a view that uses "
								  " an internalformat that is incompatible with parent "
								  " view's, whereas GL_INVALID_OPERATION was expected."
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("GL_INVALID_OPERATION not generated when requesting a texture view that "
					 "uses an internalformat which is incompatible with parent view's.");
		}

		/* Release the texture IDs */
		gl.deleteTextures(1, &m_test_modified_to_id_1);
		m_test_modified_to_id_1 = 0;

		gl.deleteTextures(1, &m_test_modified_to_id_2);
		m_test_modified_to_id_2 = 0;

		gl.deleteTextures(1, &m_test_modified_to_id_3);
		m_test_modified_to_id_3 = 0;

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() call(s) failed");
	} /* for (all incompatible texture+view internalformat pairs) */

	/* h) GL_INVALID_OPERATION error should be generated whenever the
	 *    application tries to create a texture view using an internal
	 *    format that does not match the original texture's, and the
	 *    original texture's internalformat cannot be found in table
	 *    8.21 of OpenGL 4.3 specification.
	 *
	 *    NOTE: All required base, sized and compressed texture internal
	 *          formats (as described in section 8.5.1 and table 8.14
	 *          of OpenGL 4.3 specification) that cannot be found in
	 *          table 8.21 should be considered for the purpose of this
	 *          test.
	 */
	for (int n_gl_internalformat = 0; n_gl_internalformat < n_valid_gl_internalformats; ++n_gl_internalformat)
	{
		glw::GLenum parent_texture_internalformat = valid_gl_internalformats[n_gl_internalformat];

		/* Only run the test for internalformats supported by the tested OpenGL implementation */
		if (!TextureViewUtilities::isInternalformatSupported(parent_texture_internalformat, context_major_version,
															 context_minor_version))
		{
			/* Iterate the loop */
			continue;
		}

		/* For the purpose of the test, only consider internalformats that
		 * are not associated with any view class */
		if (TextureViewUtilities::getViewClassForInternalformat(parent_texture_internalformat) == VIEW_CLASS_UNDEFINED)
		{
			/* Initialize parent texture object */
			gl.genTextures(1, &m_test_modified_to_id_1);
			gl.genTextures(1, &m_test_modified_to_id_2);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call(s) failed");

			/* Configure reference texture object storage */
			gl.bindTexture(GL_TEXTURE_2D, m_test_modified_to_id_1);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

			TextureViewUtilities::initTextureStorage(
				gl, false,		  /* views require immutable parent texture objects */
				GL_TEXTURE_2D, 0, /* texture_depth */
				reference_to_height, reference_to_width, parent_texture_internalformat,
				GL_NONE,			   /* texture_format - not needed for immutable texture objects */
				GL_NONE,			   /* texture_type   - not needed for immutable texture objects */
				reference_n_levels, 0, /* n_cubemaps_needed */
				0);					   /* bo_id */

			/* Attempt to create the invalid view */
			gl.textureView(m_test_modified_to_id_2,													  /* texture */
						   GL_TEXTURE_2D, m_test_modified_to_id_1,									  /* origtexture */
						   (parent_texture_internalformat != GL_RGBA32F) ? GL_RGBA32F : GL_RGB32F, 0, /* minlevel */
						   reference_n_levels, 0,													  /* minlayer */
						   1);																		  /* numlayers */

			error_code = gl.getError();
			if (error_code != GL_INVALID_OPERATION)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "["
								   << TextureViewUtilities::getErrorCodeString(error_code)
								   << "]"
									  " error generated when requesting a view that uses "
									  " an internalformat different than the one used by "
									  "parent texture object: "
									  "["
								   << parent_texture_internalformat
								   << "] "
									  " and the parent texture's internalformat is not "
									  "associated with any view class; GL_INVALID_OPERATION "
									  "was expected"
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("GL_INVALID_OPERATION not generated when requesting a texture view for "
						 "a parent texture, internalformat of which is not associated with any "
						 "view class, when the view's internalformat is different than the one "
						 "used for parent texture.");
			}

			/* Release the texture IDs */
			gl.deleteTextures(1, &m_test_modified_to_id_1);
			m_test_modified_to_id_1 = 0;

			gl.deleteTextures(1, &m_test_modified_to_id_2);
			m_test_modified_to_id_2 = 0;

			GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() call(s) failed");
		} /* if (parent texture internalformat is not associated with a view class) */
	}	 /* for (all valid GL internalformats) */

	/* i) GL_INVALID_VALUE error should be generated if <minlevel> is
	 *    larger than the greatest level of <origtexture>.
	 */
	gl.textureView(m_view_never_bound_to_id, GL_TEXTURE_2D, m_reference_immutable_to_2d_id, reference_to_internalformat,
				   reference_n_levels, /* minlevel */
				   1,				   /* numlevels */
				   0,				   /* minlayer */
				   1);				   /* numlayers */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
						   << "]"
							  " error generated when passing <minlevel> argument "
							  " larger than the greatest level of <origtexture>, whereas "
							  "GL_INVALID_VALUE was expected."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("GL_INVALID_VALUE not generated when passing a value of <minlevel> "
				 "larger than the greatest level defined for <origtexture>");
	}

	/* j) GL_INVALID_VALUE error should be generated if <minlayer> is
	 *    larger than the greatest layer of <origtexture>.
	 */
	gl.textureView(m_view_never_bound_to_id, GL_TEXTURE_2D_ARRAY, m_reference_immutable_to_2d_array_id,
				   reference_to_internalformat, 0, /* minlevel */
				   reference_n_levels,			   /* numlevels */
				   reference_to_depth,			   /* minlayer */
				   1);							   /* numlayers */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
						   << "]"
							  " error generated when passing <minlayer> argument "
							  " larger than the greatest layer of <origtexture>, whereas "
							  "GL_INVALID_VALUE was expected."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("GL_INVALID_VALUE not generated when passing a value of <minlayer> "
				 "larger than the greatest layer defined for <origtexture>");
	}

	/* k) GL_INVALID_VALUE error should be generated if <target> is
	 *    GL_TEXTURE_CUBE_MAP and <numlayers> is not 6.
	 */
	gl.textureView(m_view_never_bound_to_id, GL_TEXTURE_CUBE_MAP, m_reference_immutable_to_cube_map_id,
				   reference_to_internalformat, 0, /* minlevel */
				   1,							   /* numlevels */
				   0,							   /* minlayer */
				   5);							   /* numlayers - invalid argument value */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
						   << "]"
							  " error generated when passing <numlayers> argument of value "
							  "5 instead of 6 for GL_TEXTURE_CUBE_MAP texture target, whereas "
							  "GL_INVALID_VALUE was expected."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("GL_INVALID_VALUE not generated when passing a value of 5 to <minlayer>"
				 "argument");
	}

	/* l) GL_INVALID_VALUE error should be generated if <target> is
	 *    GL_TEXTURE_CUBE_MAP_ARRAY and <numlayers> is not a multiple
	 *    of 6.
	 */
	gl.textureView(m_view_never_bound_to_id, GL_TEXTURE_CUBE_MAP_ARRAY, m_reference_immutable_to_cube_map_array_id,
				   reference_to_internalformat, 0, /* minlevel */
				   1,							   /* numlevels */
				   0,							   /* minlayer */
				   1);							   /* numlayers - invalid argument value */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
						   << "]"
							  " error generated when passing <numlayers> argument of value "
							  "1 instead of a multiple of 6 for GL_TEXTURE_CUBE_MAP_ARRAY "
							  "texture target, whereas GL_INVALID_VALUE was expected."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("GL_INVALID_VALUE not generated when passing a value of 1 to <minlayer>"
				 "argument for a GL_TEXTURE_CUBE_MAP_ARRAY texture target");
	}

	/* m) GL_INVALID_VALUE error should be generated if <target> is
	 *    GL_TEXTURE_1D and <numlayers> is not 1;
	 */
	gl.textureView(m_view_never_bound_to_id, GL_TEXTURE_1D, m_reference_immutable_to_1d_id, reference_to_internalformat,
				   0,  /* minlevel */
				   1,  /* numlevels */
				   0,  /* minlayer */
				   2); /* numlayers - invalid argument value */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
						   << "]"
							  " error generated when passing <numlayers> argument of value "
							  "2 instead of 1 for GL_TEXTURE_1D texture target, whereas "
							  "GL_INVALID_VALUE was expected."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("GL_INVALID_VALUE not generated when passing a value of 2 to <numlayers>"
				 "argument for a GL_TEXTURE_1D texture target");
	}

	/* n) GL_INVALID_VALUE error should be generated if <target> is
	 *    GL_TEXTURE_2D and <numlayers> is not 1;
	 */
	gl.textureView(m_view_never_bound_to_id, GL_TEXTURE_2D, m_reference_immutable_to_2d_id, reference_to_internalformat,
				   0,  /* minlevel */
				   1,  /* numlevels */
				   0,  /* minlayer */
				   2); /* numlayers - invalid argument value */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
						   << "]"
							  " error generated when passing <numlayers> argument of value "
							  "2 instead of 1 for GL_TEXTURE_2D texture target, whereas "
							  "GL_INVALID_VALUE was expected."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("GL_INVALID_VALUE not generated when passing a value of 2 to <numlayers>"
				 "argument for a GL_TEXTURE_2D texture target");
	}

	/* o) GL_INVALID_VALUE error should be generated if <target> is
	 *    GL_TEXTURE_3D and <numlayers> is not 1;
	 */
	gl.textureView(m_view_never_bound_to_id, GL_TEXTURE_3D, m_reference_immutable_to_3d_id, reference_to_internalformat,
				   0,  /* minlevel */
				   1,  /* numlevels */
				   0,  /* minlayer */
				   2); /* numlayers - invalid argument value */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
						   << "]"
							  " error generated when passing <numlayers> argument of value "
							  "2 instead of 1 for GL_TEXTURE_3D texture target, whereas "
							  "GL_INVALID_VALUE was expected."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("GL_INVALID_VALUE not generated when passing a value of 2 to <numlayers>"
				 "argument for a GL_TEXTURE_3D texture target");
	}

	/* p) GL_INVALID_VALUE error should be generated if <target> is
	 *    GL_TEXTURE_RECTANGLE and <numlayers> is not 1;
	 */
	gl.textureView(m_view_never_bound_to_id, GL_TEXTURE_RECTANGLE, m_reference_immutable_to_rectangle_id,
				   reference_to_internalformat, 0, /* minlevel */
				   1,							   /* numlevels */
				   0,							   /* minlayer */
				   2);							   /* numlayers - invalid argument value */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
						   << "]"
							  " error generated when passing <numlayers> argument of value "
							  "2 instead of 1 for GL_TEXTURE_RECTANGLE texture target, whereas "
							  "GL_INVALID_VALUE was expected."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("GL_INVALID_VALUE not generated when passing a value of 2 to <numlayers>"
				 "argument for a GL_TEXTURE_RECTANGLE texture target");
	}

	/* q) GL_INVALID_VALUE error should be generated if <target> is
	 *    GL_TEXTURE_2D_MULTISAMPLE and <numlayers> is not 1;
	 */
	gl.textureView(m_view_never_bound_to_id, GL_TEXTURE_2D_MULTISAMPLE, m_reference_immutable_to_2d_multisample_id,
				   reference_to_internalformat, 0, /* minlevel */
				   1,							   /* numlevels */
				   0,							   /* minlayer */
				   2);							   /* numlayers - invalid argument value */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
						   << "]"
							  " error generated when passing <numlayers> argument of value "
							  "2 instead of 1 for GL_TEXTURE_2D_MULTISAMPLE texture target, whereas "
							  "GL_INVALID_VALUE was expected."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("GL_INVALID_VALUE not generated when passing a value of 2 to <numlayers>"
				 "argument for a GL_TEXTURE_2D_MULTISAMPLE texture target");
	}

	/* r) GL_INVALID_OPERATION error should be generated if <target> is
	 *    GL_TEXTURE_CUBE_MAP and original texture's width does not
	 *    match original texture's height for all levels.
	 */
	gl.textureView(m_view_never_bound_to_id, GL_TEXTURE_CUBE_MAP, m_reference_immutable_to_2d_array_32_by_33_id,
				   reference_to_internalformat, 0, /* minlevel */
				   1,							   /* numlevels */
				   0,							   /* minlayer */
				   6);							   /* numlayers */

	error_code = gl.getError();
	if (error_code != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
						   << "]"
							  " error generated when using an immutable 2D array texture of 32x33x6 "
							  "resolution to generate a GL_TEXTURE_CUBE_MAP view, whereas "
							  "GL_INVALID_OPERATION was expected."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("GL_INVALID_OPERATION not generated when using an immutable 2D array texture of "
				 "32x33x6 resolution to generate a GL_TEXTURE_CUBE_MAP view");
	}

	/* s) GL_INVALID_OPERATION error should be generated if <target> is
	 *    GL_TEXTURE_CUBE_MAP_ARRAY and original texture's width does
	 *    not match original texture's height for all levels.
	 */
	gl.textureView(m_view_never_bound_to_id, GL_TEXTURE_CUBE_MAP_ARRAY, m_reference_immutable_to_2d_array_32_by_33_id,
				   reference_to_internalformat, 0, /* minlevel */
				   1,							   /* numlevels */
				   0,							   /* minlayer */
				   6);							   /* numlayers */

	error_code = gl.getError();
	if (error_code != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "[" << TextureViewUtilities::getErrorCodeString(error_code)
						   << "]"
							  " error generated when using an immutable 2D array texture of 32x33x6 "
							  "resolution to generate a GL_TEXTURE_CUBE_MAP_ARRAY view, whereas "
							  "GL_INVALID_OPERATION was expected."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("GL_INVALID_OPERATION not generated when using an immutable 2D array texture of "
				 "32x33x6 resolution to generate a GL_TEXTURE_CUBE_MAP_ARRAY view");
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 **/
TextureViewTestViewSampling::TextureViewTestViewSampling(deqp::Context& context)
	: TestCase(context, "view_sampling", "Verify that sampling data from texture views, that use internal "
										 "format which is compatible with the original texture's internal "
										 "format, works correctly.")
	, m_bo_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_po_lod_location(-1)
	, m_po_n_face_location(-1)
	, m_po_reference_colors_location(-1)
	, m_po_texture_location(-1)
	, m_po_z_float_location(-1)
	, m_po_z_int_location(-1)
	, m_tc_id(0)
	, m_te_id(0)
	, m_vs_id(0)
	, m_per_sample_filler_fs_id(0)
	, m_per_sample_filler_gs_id(0)
	, m_per_sample_filler_po_id(0)
	, m_per_sample_filler_po_layer_id_location(-1)
	, m_per_sample_filler_po_reference_colors_location(-1)
	, m_per_sample_filler_vs_id(0)
	, m_result_to_id(0)
	, m_to_id(0)
	, m_view_to_id(0)
	, m_fbo_id(0)
	, m_vao_id(0)
	, m_max_color_texture_samples_gl_value(0)
	, m_iteration_parent_texture_depth(0)
	, m_iteration_parent_texture_height(0)
	, m_iteration_parent_texture_n_levels(0)
	, m_iteration_parent_texture_n_samples(0)
	, m_iteration_parent_texture_target(GL_NONE)
	, m_iteration_parent_texture_width(0)
	, m_iteration_view_texture_minlayer(0)
	, m_iteration_view_texture_numlayers(0)
	, m_iteration_view_texture_minlevel(0)
	, m_iteration_view_texture_numlevels(0)
	, m_iteration_view_texture_target(GL_NONE)
	, m_reference_texture_depth(4)
	, m_reference_texture_height(4)
	, m_reference_texture_n_mipmaps(3)
	, m_reference_texture_width(4)
	, m_reference_color_storage(DE_NULL)
	, m_result_data(DE_NULL)
{
	/* Left blank on purpose */
}

/** De-initializes all GL objects created for the test. */
void TextureViewTestViewSampling::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	deinitIterationSpecificProgramAndShaderObjects();
	deinitPerSampleFillerProgramAndShaderObjects();
	deinitTextureObjects();

	/* Make sure any buffers we may have allocated during the execution do not leak */
	if (m_result_data != DE_NULL)
	{
		delete[] m_result_data;

		m_result_data = DE_NULL;
	}

	/* Deinitialize other objects that are not re-created every iteration */
	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_reference_color_storage != DE_NULL)
	{
		delete m_reference_color_storage;

		m_reference_color_storage = DE_NULL;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	/* Restore default GL state the test may have modified */
	gl.patchParameteri(GL_PATCH_VERTICES, 3);
	gl.pixelStorei(GL_PACK_ALIGNMENT, 4);
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

/** De-initializes program and shader objects created for each iteration. **/
void TextureViewTestViewSampling::deinitIterationSpecificProgramAndShaderObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_tc_id != 0)
	{
		gl.deleteShader(m_tc_id);

		m_tc_id = 0;
	}

	if (m_te_id != 0)
	{
		gl.deleteShader(m_te_id);

		m_te_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** De-initializes shader and program objects providing the 'per-sample filling'
 *  functionality.
 **/
void TextureViewTestViewSampling::deinitPerSampleFillerProgramAndShaderObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_per_sample_filler_fs_id != 0)
	{
		gl.deleteShader(m_per_sample_filler_fs_id);

		m_per_sample_filler_fs_id = 0;
	}

	if (m_per_sample_filler_gs_id != 0)
	{
		gl.deleteShader(m_per_sample_filler_gs_id);

		m_per_sample_filler_gs_id = 0;
	}

	if (m_per_sample_filler_po_id != 0)
	{
		gl.deleteProgram(m_per_sample_filler_po_id);

		m_per_sample_filler_po_id = 0;
	}

	if (m_per_sample_filler_vs_id != 0)
	{
		gl.deleteShader(m_per_sample_filler_vs_id);

		m_per_sample_filler_vs_id = 0;
	}
}

/** De-initializes texture objects used by the test */
void TextureViewTestViewSampling::deinitTextureObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_result_to_id != 0)
	{
		gl.deleteTextures(1, &m_result_to_id);

		m_result_to_id = 0;
	}

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);

		m_to_id = 0;
	}

	if (m_view_to_id != 0)
	{
		gl.deleteTextures(1, &m_view_to_id);

		m_view_to_id = 0;
	}
}

/** Executes a single test iteration.
 *
 *  @return true if the iteration executed successfully, false otherwise.
 **/
bool TextureViewTestViewSampling::executeTest()
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* Bind the view to zero texture unit */
	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture() call failed.");

	gl.bindTexture(m_iteration_view_texture_target, m_view_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	/* Bind the buffer object to zero TF binding point */
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed.");

	/* Activate the test program */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	/* Update draw framebuffer configuration so that the test's fragment shader draws
	 * to the result texture */
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_result_to_id, 0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");

	/* Allocate enough space to hold reference color data for all sample s*/
	float* reference_color_data = new float[m_iteration_parent_texture_n_samples * sizeof(float) * 4 /* rgba */];

	/* Iterate through the layer/face/mipmap hierarchy. For each iteration, we
	 * potentially need to update relevant uniforms controlling the sampling process
	 * the test program object performs.
	 */
	bool is_view_cm_cma = (m_iteration_view_texture_target == GL_TEXTURE_CUBE_MAP ||
						   m_iteration_view_texture_target == GL_TEXTURE_CUBE_MAP_ARRAY);

	for (unsigned int n_current_layer = m_iteration_view_texture_minlayer;
		 n_current_layer < (m_iteration_view_texture_minlayer + m_iteration_view_texture_numlayers) && result;
		 n_current_layer++)
	{
		unsigned int n_texture_face  = 0;
		unsigned int n_texture_layer = 0;
		unsigned int n_view_face	 = 0;
		unsigned int n_view_layer	= 0;

		if (is_view_cm_cma)
		{
			n_texture_face  = n_current_layer % 6;										 /* faces */
			n_texture_layer = n_current_layer / 6;										 /* faces */
			n_view_face		= (n_current_layer - m_iteration_view_texture_minlayer) % 6; /* faces */
			n_view_layer	= (n_current_layer - m_iteration_view_texture_minlayer) / 6; /* faces */
		}
		else
		{
			/* Only cube-map and cube-map array textures consist of faces. */
			n_texture_face  = 0;
			n_texture_layer = n_current_layer;
			n_view_face		= 0;
			n_view_layer	= n_current_layer;
		}

		if (m_po_z_float_location != -1)
		{
			float z = 0.0f;

			if (((false == is_view_cm_cma) && (m_iteration_view_texture_numlayers > 1)) ||
				((true == is_view_cm_cma) && (m_iteration_view_texture_numlayers > 6)))
			{
				if (is_view_cm_cma)
				{
					z = float(n_view_layer) / float(m_iteration_view_texture_numlayers / 6 - 1);
				}
				else
				{
					if (m_iteration_view_texture_numlayers > 1)
					{
						/* The program will be sampling a view so make sure that layer the shader accesses
						 * is relative to how our view was configured */
						z = float(n_view_layer - m_iteration_view_texture_minlayer) /
							float(m_iteration_view_texture_numlayers - 1);
					}
					else
					{
						/* z should stay at 0 */
					}
				}
			}
			else
			{
				/* z should stay at 0.0 */
			}

			gl.uniform1f(m_po_z_float_location, z);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1f() call failed.");
		}

		if (m_po_z_int_location != -1)
		{
			DE_ASSERT(!is_view_cm_cma);

			gl.uniform1i(m_po_z_int_location, n_current_layer - m_iteration_view_texture_minlayer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() call failed.");
		}

		if (m_po_n_face_location != -1)
		{
			gl.uniform1i(m_po_n_face_location, n_view_face);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() call failed.");
		}

		for (unsigned int n_mipmap = m_iteration_view_texture_minlevel;
			 n_mipmap < (m_iteration_view_texture_minlevel + m_iteration_view_texture_numlevels) && result; n_mipmap++)
		{
			if (m_po_lod_location != -1)
			{
				/* The program will be sampling a view so make sure that LOD the shader accesses
				 * is relative to how our view was configured.
				 */
				gl.uniform1f(m_po_lod_location, (float)(n_mipmap - m_iteration_view_texture_minlevel));
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() call failed.");
			}

			/* Update local reference color data storage */
			for (unsigned int n_sample = 0; n_sample < m_iteration_parent_texture_n_samples; ++n_sample)
			{
				tcu::Vec4 reference_color = getReferenceColor(n_texture_layer, n_texture_face, n_mipmap, n_sample);

				reference_color_data[4 /* rgba */ * n_sample + 0] = reference_color.x();
				reference_color_data[4 /* rgba */ * n_sample + 1] = reference_color.y();
				reference_color_data[4 /* rgba */ * n_sample + 2] = reference_color.z();
				reference_color_data[4 /* rgba */ * n_sample + 3] = reference_color.w();
			}

			/* Upload it to GPU */
			gl.uniform4fv(m_po_reference_colors_location, m_iteration_parent_texture_n_samples, reference_color_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform4fv() call failed.");

			/* Bind the texture view to sample from */
			gl.bindTexture(m_iteration_view_texture_target, m_view_to_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

			/* Draw a single patch. Given the rendering pipeline we've defined in the
			 * test program object, this should give us a nice full-screen quad, as well
			 * as 6*4 ints XFBed out, describing whether the view was sampled correctly.
			 */
			gl.beginTransformFeedback(GL_TRIANGLES);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed.");
			{
				gl.drawArrays(GL_PATCHES, 0 /* first */, 1 /* count */);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");
			}
			gl.endTransformFeedback();
			GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed.");

			/* In order to verify if the texel data was sampled correctly, we need to do two things:
			 *
			 * 1) Verify buffer object contents;
			 * 2) Make sure that all texels of current render-target are vec4(1).
			 *
			 */
			const int* bo_storage_ptr = (const int*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer() call failed.");
			if (bo_storage_ptr == NULL)
			{
				TCU_FAIL("glMapBuffer() call succeeded but the pointer returned is NULL");
			}

			/* The rendering pipeline should have written 6 vertices * 4 ints to the BO.
			 * The integers are set to 1 if the sampled texels were found valid, 0 otherwise,
			 * and are arranged in the following order:
			 *
			 * 1) Result of sampling in vertex shader stage;
			 * 2) Result of sampling in tessellation control shader stage;
			 * 3) Result of sampling in tessellation evaluation shader stage;
			 * 4) Result of sampling in geometry shader stage;
			 */
			for (unsigned int n_vertex = 0; n_vertex < 6 /* as per comment */ && result; ++n_vertex)
			{
				const int* vertex_data_ptr = bo_storage_ptr + n_vertex * 4 /* as per comment */;
				int		   vs_result	   = vertex_data_ptr[0];
				int		   tc_result	   = vertex_data_ptr[1];
				int		   te_result	   = vertex_data_ptr[2];
				int		   gs_result	   = vertex_data_ptr[3];

				if (vs_result != 1)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Invalid data was sampled in vertex shader stage."
									   << tcu::TestLog::EndMessage;

					result = false;
				}

				if (tc_result != 1)
				{
					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Invalid data was sampled in tessellation control shader stage."
									   << tcu::TestLog::EndMessage;

					result = false;
				}

				if (te_result != 1)
				{
					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Invalid data was sampled in tessellation evaluation shader stage."
									   << tcu::TestLog::EndMessage;

					result = false;
				}

				if (gs_result != 1)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Invalid data was sampled in geometry shader stage."
									   << tcu::TestLog::EndMessage;

					result = false;
				}
			} /* for (all vertices) */

			/* Unmap the BO */
			gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

			/* Read texels rendered by the fragment shader. The texture attached uses
			 * GL_RGBA8 internalformat.*/
			m_result_data = new unsigned char[m_reference_texture_width * m_reference_texture_height * 4 /* RGBA */];

			gl.bindTexture(GL_TEXTURE_2D, m_result_to_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed for GL_TEXTURE_2D texture target.");

			gl.getTexImage(GL_TEXTURE_2D, 0 /* level */, GL_RGBA, GL_UNSIGNED_BYTE, m_result_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexImage() call failed.");

			/* The test fails if any of the fragments is not equal to vec4(1) */
			bool fs_result = true;

			for (unsigned int y = 0; y < m_reference_texture_height && fs_result; ++y)
			{
				const unsigned char* row_ptr = m_result_data + m_reference_texture_width * y * 4 /* RGBA */;

				for (unsigned int x = 0; x < m_reference_texture_width && fs_result; ++x)
				{
					const unsigned char* pixel_ptr = row_ptr + x * 4 /* RGBA */;

					if (pixel_ptr[0] != 255 || pixel_ptr[1] != 255 || pixel_ptr[2] != 255 || pixel_ptr[3] != 255)
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Invalid data was sampled at (" << x << ", " << y
										   << ") "
											  "in fragment shader stage."
										   << tcu::TestLog::EndMessage;

						fs_result = false;
					}
				} /* for (all columns) */
			}	 /* for (all rows) */

			if (!fs_result)
			{
				result = false;
			}

			/* Done - we can release the buffer at this point */
			delete[] m_result_data;
			m_result_data = DE_NULL;
		} /* for (all mip-maps) */
	}	 /* for (all texture layers) */

	/* Release the reference color data buffer */
	delete[] reference_color_data;
	reference_color_data = DE_NULL;

	/* All done */
	return result;
}

/** Returns a different vec4 every time the function is called. Each component
 *  is assigned a normalized value within <0, 1> range.
 *
 *  @return As per description.
 **/
tcu::Vec4 TextureViewTestViewSampling::getRandomReferenceColor()
{
	static unsigned int seed = 195;
	tcu::Vec4			result;

	result = tcu::Vec4(float((seed) % 255) / 255.0f, float((seed << 3) % 255) / 255.0f,
					   float((seed << 4) % 255) / 255.0f, float((seed << 5) % 255) / 255.0f);

	seed += 17;

	return result;
}

/** Every test iteration is assigned a different set of so-called reference colors.
 *  Depending on the texture target, each reference color corresponds to an unique color
 *  used to build different layers/faces/mip-maps or even samples of tose.
 *
 *  Once the reference color storage is initialized, this function can be used to retrieve
 *  details of a color allocated a specific sample of a layer/face mip-map.
 *
 *  This function will cause an assertion failure if an invalid layer/face/mipmap/sample is
 *  requested, as well as if the reference color storage is not initialized at the time of the call.
 *
 *  @param n_layer  Layer index to use for the query. A value of 0 should be used for non-arrayed
 *                  texture targets.
 *  @param n_face   Face index to use for the query. A value of 0 should be used for non-CM texture
 *                  targets. Otherwise:
 *                  * 0 corresponds to +X;
 *                  * 1 corresponds to -X;
 *                  * 2 corresponds to +Y;
 *                  * 3 corresponds to -Y;
 *                  * 4 corresponds to +Z;
 *                  * 5 corresponds to -Z.
 *  @param n_mipmap Mip-map index to use for the query. A value of 0 should be used for non-mipmapped
 *                  texture targets.
 *  @param n_sample Sample index to use for the query. A value of 0 should be used for single-sampled
 *                  texture targets.
 *
 *  @return Requested color data.
 **/
tcu::Vec4 TextureViewTestViewSampling::getReferenceColor(unsigned int n_layer, unsigned int n_face,
														 unsigned int n_mipmap, unsigned int n_sample)
{
	tcu::Vec4 result;

	DE_ASSERT(m_reference_color_storage != DE_NULL);
	if (m_reference_color_storage != DE_NULL)
	{
		bool is_parent_texture_cm_cma = (m_iteration_parent_texture_target == GL_TEXTURE_CUBE_MAP ||
										 m_iteration_parent_texture_target == GL_TEXTURE_CUBE_MAP_ARRAY);
		bool is_view_texture_cm_cma = (m_iteration_view_texture_target == GL_TEXTURE_CUBE_MAP ||
									   m_iteration_view_texture_target == GL_TEXTURE_CUBE_MAP_ARRAY);

		if (is_view_texture_cm_cma && !is_parent_texture_cm_cma)
		{
			/* Parent texture is not using faces. Compute layer index, as
			 * if the texture was actually a CM or a CMA */
			unsigned int temp = n_layer * 6 /* layer-faces per layer */ + n_face;

			n_layer = temp;
			n_face  = 0;
		}
		else if (!is_view_texture_cm_cma && is_parent_texture_cm_cma)
		{
			/* The other way around - assume the texture is a CM or CMA */
			n_face  = n_layer % 6; /* faces per cube-map layer */
			n_layer = n_layer / 6; /* faces per cube-map layer */
		}

		DE_ASSERT(n_face < m_reference_color_storage->n_faces);
		DE_ASSERT(n_layer < m_reference_color_storage->n_layers);
		DE_ASSERT(n_mipmap < m_reference_color_storage->n_mipmaps);
		DE_ASSERT(n_sample < m_reference_color_storage->n_samples);

		/* Hierarchy is:
		 *
		 * layers -> faces -> mipmaps -> samples */
		const unsigned int index =
			n_layer * (m_reference_color_storage->n_faces * m_reference_color_storage->n_mipmaps *
					   m_reference_color_storage->n_samples) +
			n_face * (m_reference_color_storage->n_mipmaps * m_reference_color_storage->n_samples) +
			n_mipmap * (m_reference_color_storage->n_samples) + n_sample;

		result = m_reference_color_storage->data[index];
	}

	return result;
}

/* Retrieve max conformant sample count when GL_NV_internalformat_sample_query is supported */
glw::GLint TextureViewTestViewSampling::getMaxConformantSampleCount(glw::GLenum target, glw::GLenum internalFormat)
{
	(void)internalFormat;
	glw::GLint max_conformant_samples = 0;

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Return the max conformant sample count if extension is supported */
	if (m_context.getContextInfo().isExtensionSupported("GL_NV_internalformat_sample_query"))
	{
		glw::GLint gl_sample_counts = 0;
		gl.getInternalformativ(target, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, 1, &gl_sample_counts);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInternalformativ() failed for GL_NUM_SAMPLE_COUNTS pname");

		/* Check and return the first conformant sample count */
		glw::GLint* gl_supported_samples = new glw::GLint[gl_sample_counts];
		if (gl_supported_samples)
		{
			gl.getInternalformativ(target, GL_RGBA8, GL_SAMPLES, gl_sample_counts, gl_supported_samples);

			for (glw::GLint i = 0; i < gl_sample_counts; i++)
			{
				glw::GLint isConformant = 0;
				gl.getInternalformatSampleivNV(target, GL_RGBA8, gl_supported_samples[i], GL_CONFORMANT_NV, 1,
											   &isConformant);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInternalformatSampleivNV() call(s) failed");

				if (isConformant && gl_supported_samples[i] > max_conformant_samples)
				{
					max_conformant_samples = gl_supported_samples[i];
				}
			}
			delete[] gl_supported_samples;
		}
	}
	else
	{
		/* Otherwise return GL_MAX_COLOR_TEXTURE_SAMPLES */
		gl.getIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &max_conformant_samples);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed for GL_MAX_COLOR_TEXTURE_SAMPLES pname.");
	}

	return max_conformant_samples;
}

/** Initializes iteration-specific program object used to sample the texture data. */
void TextureViewTestViewSampling::initIterationSpecificProgramObject()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release shader/program objects that may have been initialized in previous
	 * iterations.
	 */
	deinitIterationSpecificProgramAndShaderObjects();

	/* Create program and shader objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_tc_id = gl.createShader(GL_TESS_CONTROL_SHADER);
	m_te_id = gl.createShader(GL_TESS_EVALUATION_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call(s) failed.");

	/* Prepare token replacement strings */
	std::stringstream n_samples_sstream;
	std::string		  sampler_declarations_string;
	std::string		  sample_fetch_string;
	std::string		  sample_fetch_fs_string;
	std::size_t		  token_location			 = std::string::npos;
	const char*		  token_n_samples			 = "N_SAMPLES";
	const char*		  token_sampler_declarations = "SAMPLER_DECLARATIONS";
	const char*		  token_sample_fetch		 = "SAMPLE_FETCH";

	n_samples_sstream << m_iteration_parent_texture_n_samples;

	switch (m_iteration_view_texture_target)
	{
	case GL_TEXTURE_1D:
	{
		sampler_declarations_string = "uniform sampler1D texture;";
		sample_fetch_string			= "vec4 current_sample = textureLod(texture, 0.5,        lod);\n";
		sample_fetch_fs_string		= "vec4 current_sample = textureLod(texture, gs_fs_uv.x, lod);\n";

		break;
	}

	case GL_TEXTURE_1D_ARRAY:
	{
		sampler_declarations_string = "uniform sampler1DArray texture;\n"
									  "uniform float          z_float;\n";

		sample_fetch_string	= "vec4 current_sample = textureLod(texture, vec2(0.5, z_float), lod);\n";
		sample_fetch_fs_string = "vec4 current_sample = textureLod(texture, vec2(gs_fs_uv.x, z_float), lod);\n";

		break;
	}

	case GL_TEXTURE_2D:
	{
		sampler_declarations_string = "uniform sampler2D texture;";
		sample_fetch_string			= "vec4 current_sample = textureLod(texture, vec2(0.5), lod);\n";
		sample_fetch_fs_string		= "vec4 current_sample = textureLod(texture, gs_fs_uv, lod);\n";

		break;
	}

	case GL_TEXTURE_2D_ARRAY:
	{
		sampler_declarations_string = "uniform float          z_float;\n"
									  "uniform sampler2DArray texture;";

		sample_fetch_string	= "vec4 current_sample = textureLod(texture, vec3(vec2(0.5), z_float), lod);\n";
		sample_fetch_fs_string = "vec4 current_sample = textureLod(texture, vec3(gs_fs_uv, z_float), lod);\n";

		break;
	}

	case GL_TEXTURE_2D_MULTISAMPLE:
	{
		sampler_declarations_string = "uniform sampler2DMS texture;";
		sample_fetch_string			= "ivec2 texture_size   = textureSize(texture);\n"
							  "vec4  current_sample = texelFetch (texture,\n"
							  "                                   ivec2(texture_size.xy / ivec2(2)),\n"
							  "                                   n_sample);\n";

		sample_fetch_fs_string = "ivec2 texture_size   = textureSize(texture);\n"
								 "vec4  current_sample = texelFetch (texture,\n"
								 "                                   ivec2(gs_fs_uv * vec2(texture_size)),\n"
								 "                                   n_sample);\n";

		break;
	}

	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
	{
		sampler_declarations_string = "uniform sampler2DMSArray texture;"
									  "uniform int              z_int;\n";

		sample_fetch_string = "ivec3 texture_size   = textureSize(texture);\n"
							  "vec4  current_sample = texelFetch (texture,\n"
							  "                                   ivec3(texture_size.xy / ivec2(2), z_int),\n"
							  "                                   n_sample);\n";

		sample_fetch_fs_string =
			"ivec3 texture_size = textureSize(texture);\n"
			"vec4  current_sample = texelFetch (texture,\n"
			"                                   ivec3(ivec2(gs_fs_uv * vec2(texture_size).xy), z_int),\n"
			"                                   n_sample);\n";

		break;
	}

	case GL_TEXTURE_3D:
	{
		sampler_declarations_string = "uniform sampler3D texture;"
									  "uniform float     z_float;";

		sample_fetch_string	= "vec4 current_sample = textureLod(texture, vec3(vec2(0.5), z_float), lod);\n";
		sample_fetch_fs_string = "vec4 current_sample = textureLod(texture, vec3(gs_fs_uv, z_float), lod);\n";

		break;
	}

	case GL_TEXTURE_CUBE_MAP:
	{
		sampler_declarations_string = "uniform samplerCube texture;\n"
									  "uniform int         n_face;";

		sample_fetch_string = "vec4 current_sample;\n"
							  "\n"
							  "switch (n_face)\n"
							  "{\n"
							  // GL_TEXTURE_CUBE_MAP_POSITIVE_X
							  "    case 0: current_sample = textureLod(texture, vec3( 1,  0,  0), lod); break;\n"
							  // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
							  "    case 1: current_sample = textureLod(texture, vec3(-1,  0,  0), lod); break;\n"
							  // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
							  "    case 2: current_sample = textureLod(texture, vec3( 0,  1,  0), lod); break;\n"
							  // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
							  "    case 3: current_sample = textureLod(texture, vec3( 0, -1,  0), lod); break;\n"
							  // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
							  "    case 4: current_sample = textureLod(texture, vec3( 0,  0,  1), lod); break;\n"
							  // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
							  "    case 5: current_sample = textureLod(texture, vec3( 0,  0, -1), lod); break;\n"
							  "}\n";

		sample_fetch_fs_string =
			"vec4 current_sample;\n"
			"\n"
			"switch (n_face)\n"
			"{\n"
			// GL_TEXTURE_CUBE_MAP_POSITIVE_X
			"    case 0: current_sample = textureLod(texture, normalize(vec3( 1, gs_fs_uv.xy)), lod); break;\n"
			// GL_TEXTURE_CUBE_MAP_NEGATIVE_X
			"    case 1: current_sample = textureLod(texture, normalize(vec3(-1, gs_fs_uv.xy)), lod); break;\n"
			// GL_TEXTURE_CUBE_MAP_POSITIVE_Y
			"    case 2: current_sample = textureLod(texture, normalize(vec3( gs_fs_uv.x, 1,  gs_fs_uv.y)), lod); "
			"break;\n"
			// GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
			"    case 3: current_sample = textureLod(texture, normalize(vec3( gs_fs_uv.x, -1, gs_fs_uv.y)), lod); "
			"break;\n"
			// GL_TEXTURE_CUBE_MAP_POSITIVE_Z
			"    case 4: current_sample = textureLod(texture, normalize(vec3( gs_fs_uv.xy,  1)), lod); break;\n"
			// GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
			"    case 5: current_sample = textureLod(texture, normalize(vec3( gs_fs_uv.xy, -1)), lod); break;\n"
			"}\n";
		break;
	}

	case GL_TEXTURE_CUBE_MAP_ARRAY:
	{
		sampler_declarations_string = "uniform samplerCubeArray texture;\n"
									  "uniform int              n_face;\n"
									  "uniform float            z_float;\n";

		sample_fetch_string =
			"vec4 current_sample;\n"
			"\n"
			"switch (n_face)\n"
			"{\n"
			// GL_TEXTURE_CUBE_MAP_POSITIVE_X
			"    case 0: current_sample = textureLod(texture, vec4( 1,  0,  0, z_float), lod); break;\n"
			// GL_TEXTURE_CUBE_MAP_NEGATIVE_X
			"    case 1: current_sample = textureLod(texture, vec4(-1,  0,  0, z_float), lod); break;\n"
			// GL_TEXTURE_CUBE_MAP_POSITIVE_Y
			"    case 2: current_sample = textureLod(texture, vec4( 0,  1,  0, z_float), lod); break;\n"
			// GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
			"    case 3: current_sample = textureLod(texture, vec4( 0, -1,  0, z_float), lod); break;\n"
			// GL_TEXTURE_CUBE_MAP_POSITIVE_Z
			"    case 4: current_sample = textureLod(texture, vec4( 0,  0,  1, z_float), lod); break;\n"
			// GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
			"    case 5: current_sample = textureLod(texture, vec4( 0,  0, -1, z_float), lod); break;\n"
			"}\n";

		sample_fetch_fs_string = "vec4 current_sample;\n"
								 "\n"
								 "switch (n_face)\n"
								 "{\n"
								 // GL_TEXTURE_CUBE_MAP_POSITIVE_X
								 "    case 0: current_sample = textureLod(texture, vec4(normalize(vec3( 1, "
								 "gs_fs_uv.xy)), z_float), lod); break;\n"
								 // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
								 "    case 1: current_sample = textureLod(texture, vec4(normalize(vec3(-1, "
								 "gs_fs_uv.xy)), z_float), lod); break;\n"
								 // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
								 "    case 2: current_sample = textureLod(texture, vec4(normalize(vec3( gs_fs_uv.x, 1, "
								 " gs_fs_uv.y)), z_float), lod); break;\n"
								 // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
								 "    case 3: current_sample = textureLod(texture, vec4(normalize(vec3( gs_fs_uv.x, "
								 "-1,  gs_fs_uv.y)), z_float), lod); break;\n"
								 // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
								 "    case 4: current_sample = textureLod(texture, vec4(normalize(vec3( gs_fs_uv.xy, "
								 "1)), z_float), lod); break;\n"
								 // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
								 "    case 5: current_sample = textureLod(texture, vec4(normalize(vec3( gs_fs_uv.xy, "
								 "-1)), z_float), lod); break;\n"
								 "}\n";

		break;
	}

	case GL_TEXTURE_RECTANGLE:
	{
		sampler_declarations_string = "uniform sampler2DRect texture;";
		sample_fetch_string			= "ivec2 texture_size   = textureSize(texture);\n"
							  "vec4  current_sample = texelFetch (texture, texture_size / ivec2(2));\n";

		sample_fetch_fs_string =
			"ivec2 texture_size   = textureSize(texture);\n"
			"vec4  current_sample = texelFetch (texture, ivec2(gs_fs_uv.xy * vec2(texture_size)));\n";

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized texture target");
	}
	} /* switch (m_iteration_view_texture_target) */

	/* Set vertex shader's body */
	const char* vs_body = "#version 400\n"
						  "\n"
						  "uniform float lod;\n"
						  "uniform vec4 reference_colors[N_SAMPLES];\n"
						  "SAMPLER_DECLARATIONS\n"
						  "\n"
						  "out int vs_tc_vs_sampling_result;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    const float epsilon = 1.0 / 255.0;\n"
						  "\n"
						  "    vs_tc_vs_sampling_result = 1;\n"
						  "\n"
						  "    for (int n_sample = 0; n_sample < N_SAMPLES; ++n_sample)\n"
						  "    {\n"
						  "        SAMPLE_FETCH;\n"
						  "\n"
						  "        if (abs(current_sample.x - reference_colors[n_sample].x) > epsilon ||\n"
						  "            abs(current_sample.y - reference_colors[n_sample].y) > epsilon ||\n"
						  "            abs(current_sample.z - reference_colors[n_sample].z) > epsilon ||\n"
						  "            abs(current_sample.w - reference_colors[n_sample].w) > epsilon)\n"
						  "        {\n"
						  "            vs_tc_vs_sampling_result = int(current_sample.x * 256.0);\n"
						  "\n"
						  "            break;\n"
						  "        }\n"
						  "    }\n"
						  "\n"
						  "    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
						  "}\n";
	std::string vs_string = vs_body;

	while ((token_location = vs_string.find(token_n_samples)) != std::string::npos)
	{
		vs_string.replace(token_location, strlen(token_n_samples), n_samples_sstream.str());
	}

	while ((token_location = vs_string.find(token_sampler_declarations)) != std::string::npos)
	{
		vs_string.replace(token_location, strlen(token_sampler_declarations), sampler_declarations_string);
	}

	while ((token_location = vs_string.find(token_sample_fetch)) != std::string::npos)
	{
		vs_string.replace(token_location, strlen(token_sample_fetch), sample_fetch_string);
	}

	/* Set tessellation control shader's body */
	const char* tc_body = "#version 400\n"
						  "\n"
						  "layout(vertices = 1) out;\n"
						  "\n"
						  "uniform float lod;\n"
						  "uniform vec4  reference_colors[N_SAMPLES];\n"
						  "SAMPLER_DECLARATIONS\n"
						  "\n"
						  "in  int vs_tc_vs_sampling_result[];\n"
						  "out int tc_te_vs_sampling_result[];\n"
						  "out int tc_te_tc_sampling_result[];\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    const float epsilon = 1.0 / 255.0;\n"
						  "\n"
						  "    tc_te_vs_sampling_result[gl_InvocationID] = vs_tc_vs_sampling_result[gl_InvocationID];\n"
						  "    tc_te_tc_sampling_result[gl_InvocationID] = 1;\n"
						  "\n"
						  "    for (int n_sample = 0; n_sample < N_SAMPLES; ++n_sample)\n"
						  "    {\n"
						  "        SAMPLE_FETCH\n"
						  "\n"
						  "        if (abs(current_sample.x - reference_colors[n_sample].x) > epsilon ||\n"
						  "            abs(current_sample.y - reference_colors[n_sample].y) > epsilon ||\n"
						  "            abs(current_sample.z - reference_colors[n_sample].z) > epsilon ||\n"
						  "            abs(current_sample.w - reference_colors[n_sample].w) > epsilon)\n"
						  "        {\n"
						  "            tc_te_tc_sampling_result[gl_InvocationID] = 0;\n"
						  "\n"
						  "            break;\n"
						  "        }\n"
						  "    }\n"
						  "\n"
						  "   gl_TessLevelInner[0] = 1.0;\n"
						  "   gl_TessLevelInner[1] = 1.0;\n"
						  "   gl_TessLevelOuter[0] = 1.0;\n"
						  "   gl_TessLevelOuter[1] = 1.0;\n"
						  "   gl_TessLevelOuter[2] = 1.0;\n"
						  "   gl_TessLevelOuter[3] = 1.0;\n"
						  "}\n";

	std::string tc_string = tc_body;

	while ((token_location = tc_string.find(token_n_samples)) != std::string::npos)
	{
		tc_string.replace(token_location, strlen(token_n_samples), n_samples_sstream.str());
	}

	while ((token_location = tc_string.find(token_sampler_declarations)) != std::string::npos)
	{
		tc_string.replace(token_location, strlen(token_sampler_declarations), sampler_declarations_string);
	}

	while ((token_location = tc_string.find(token_sample_fetch)) != std::string::npos)
	{
		tc_string.replace(token_location, strlen(token_sample_fetch), sample_fetch_string);
	}

	/* Set tessellation evaluation shader's body */
	const char* te_body = "#version 400\n"
						  "\n"
						  "layout(quads) in;\n"
						  "\n"
						  "in  int  tc_te_vs_sampling_result[];\n"
						  "in  int  tc_te_tc_sampling_result[];\n"
						  "out int  te_gs_vs_sampling_result;\n"
						  "out int  te_gs_tc_sampling_result;\n"
						  "out int  te_gs_te_sampling_result;\n"
						  "out vec2 te_gs_uv;\n"
						  "\n"
						  "uniform float lod;\n"
						  "uniform vec4  reference_colors[N_SAMPLES];\n"
						  "SAMPLER_DECLARATIONS\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    te_gs_vs_sampling_result = tc_te_vs_sampling_result[0];\n"
						  "    te_gs_tc_sampling_result = tc_te_tc_sampling_result[0];\n"
						  "    te_gs_te_sampling_result = 1;\n"
						  "\n"
						  /* gl_TessCoord spans from 0 to 1 for XY. To generate a screen-space quad,
		 * we need to project these components to <-1, 1>. */
						  "    gl_Position.xy = gl_TessCoord.xy * 2.0 - 1.0;\n"
						  "    gl_Position.zw = vec2(0, 1);\n"
						  "    te_gs_uv       = vec2(gl_TessCoord.x, 1.0 - gl_TessCoord.y);\n"
						  "\n"
						  "\n"
						  "    const float epsilon = 1.0 / 255.0;\n"
						  "\n"
						  "    for (int n_sample = 0; n_sample < N_SAMPLES; ++n_sample)\n"
						  "    {\n"
						  "        SAMPLE_FETCH\n"
						  "\n"
						  "        if (abs(current_sample.x - reference_colors[n_sample].x) > epsilon ||\n"
						  "            abs(current_sample.y - reference_colors[n_sample].y) > epsilon ||\n"
						  "            abs(current_sample.z - reference_colors[n_sample].z) > epsilon ||\n"
						  "            abs(current_sample.w - reference_colors[n_sample].w) > epsilon)\n"
						  "        {\n"
						  "            te_gs_te_sampling_result = 0;\n"
						  "\n"
						  "            break;\n"
						  "        }\n"
						  "    }\n"
						  "\n"
						  "}\n";

	std::string te_string = te_body;

	while ((token_location = te_string.find(token_n_samples)) != std::string::npos)
	{
		te_string.replace(token_location, strlen(token_n_samples), n_samples_sstream.str());
	}

	while ((token_location = te_string.find(token_sampler_declarations)) != std::string::npos)
	{
		te_string.replace(token_location, strlen(token_sampler_declarations), sampler_declarations_string);
	}

	while ((token_location = te_string.find(token_sample_fetch)) != std::string::npos)
	{
		te_string.replace(token_location, strlen(token_sample_fetch), sample_fetch_string);
	}

	/* Set geometry shader's body */
	const char* gs_body = "#version 400\n"
						  "\n"
						  "layout (triangles)                        in;\n"
						  "layout (triangle_strip, max_vertices = 3) out;\n"
						  "\n"
						  "in  int  te_gs_vs_sampling_result[];\n"
						  "in  int  te_gs_tc_sampling_result[];\n"
						  "in  int  te_gs_te_sampling_result[];\n"
						  "in  vec2 te_gs_uv                [];\n"
						  "out int  gs_fs_vs_sampling_result;\n"
						  "out int  gs_fs_tc_sampling_result;\n"
						  "out int  gs_fs_te_sampling_result;\n"
						  "out int  gs_fs_gs_sampling_result;\n"
						  "out vec2 gs_fs_uv;\n"
						  "\n"
						  "uniform float lod;\n"
						  "uniform vec4  reference_colors[N_SAMPLES];\n"
						  "SAMPLER_DECLARATIONS\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    const float epsilon            = 1.0 / 255.0;\n"
						  "    int         gs_sampling_result = 1;\n"
						  "    int         tc_sampling_result = te_gs_tc_sampling_result[0] & "
						  "te_gs_tc_sampling_result[1] & te_gs_tc_sampling_result[2];\n"
						  "    int         te_sampling_result = te_gs_te_sampling_result[0] & "
						  "te_gs_te_sampling_result[1] & te_gs_te_sampling_result[2];\n"
						  "    int         vs_sampling_result = te_gs_vs_sampling_result[0] & "
						  "te_gs_vs_sampling_result[1] & te_gs_vs_sampling_result[2];\n"
						  "\n"
						  "    for (int n_sample = 0; n_sample < N_SAMPLES; ++n_sample)\n"
						  "    {\n"
						  "        SAMPLE_FETCH;\n"
						  "\n"
						  "        if (abs(current_sample.x - reference_colors[n_sample].x) > epsilon ||\n"
						  "            abs(current_sample.y - reference_colors[n_sample].y) > epsilon ||\n"
						  "            abs(current_sample.z - reference_colors[n_sample].z) > epsilon ||\n"
						  "            abs(current_sample.w - reference_colors[n_sample].w) > epsilon)\n"
						  "        {\n"
						  "            gs_sampling_result = 0;\n"
						  "\n"
						  "            break;\n"
						  "        }\n"
						  "    }\n"
						  "\n"
						  "    gl_Position              = gl_in[0].gl_Position;\n"
						  "    gs_fs_uv                 = te_gs_uv[0];\n"
						  "    gs_fs_gs_sampling_result = gs_sampling_result;\n"
						  "    gs_fs_tc_sampling_result = tc_sampling_result;\n"
						  "    gs_fs_te_sampling_result = te_sampling_result;\n"
						  "    gs_fs_vs_sampling_result = vs_sampling_result;\n"
						  "    EmitVertex();\n"
						  "\n"
						  "    gl_Position              = gl_in[1].gl_Position;\n"
						  "    gs_fs_uv                 = te_gs_uv[1];\n"
						  "    gs_fs_gs_sampling_result = gs_sampling_result;\n"
						  "    gs_fs_tc_sampling_result = tc_sampling_result;\n"
						  "    gs_fs_te_sampling_result = te_sampling_result;\n"
						  "    gs_fs_vs_sampling_result = vs_sampling_result;\n"
						  "    EmitVertex();\n"
						  "\n"
						  "    gl_Position              = gl_in[2].gl_Position;\n"
						  "    gs_fs_uv                 = te_gs_uv[2];\n"
						  "    gs_fs_gs_sampling_result = gs_sampling_result;\n"
						  "    gs_fs_tc_sampling_result = tc_sampling_result;\n"
						  "    gs_fs_te_sampling_result = te_sampling_result;\n"
						  "    gs_fs_vs_sampling_result = vs_sampling_result;\n"
						  "    EmitVertex();\n"
						  "    EndPrimitive();\n"
						  "}\n";

	std::string gs_string = gs_body;

	while ((token_location = gs_string.find(token_n_samples)) != std::string::npos)
	{
		gs_string.replace(token_location, strlen(token_n_samples), n_samples_sstream.str());
	}

	while ((token_location = gs_string.find(token_sampler_declarations)) != std::string::npos)
	{
		gs_string.replace(token_location, strlen(token_sampler_declarations), sampler_declarations_string);
	}

	while ((token_location = gs_string.find(token_sample_fetch)) != std::string::npos)
	{
		gs_string.replace(token_location, strlen(token_sample_fetch), sample_fetch_string);
	}

	/* Set fragment shader's body */
	const char* fs_body = "#version 400\n"
						  "\n"
						  "in vec2 gs_fs_uv;\n"
						  "\n"
						  "uniform float lod;\n"
						  "uniform vec4  reference_colors[N_SAMPLES];\n"
						  "SAMPLER_DECLARATIONS\n"
						  "\n"
						  "out vec4 result;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    const float epsilon = 1.0 / 255.0;\n"
						  "\n"
						  "    result = vec4(1.0);\n"
						  "\n"
						  "    for (int n_sample = 0; n_sample < N_SAMPLES; ++n_sample)\n"
						  "    {\n"
						  "        SAMPLE_FETCH\n"
						  "\n"
						  "        if (abs(current_sample.x - reference_colors[n_sample].x) > epsilon ||\n"
						  "            abs(current_sample.y - reference_colors[n_sample].y) > epsilon ||\n"
						  "            abs(current_sample.z - reference_colors[n_sample].z) > epsilon ||\n"
						  "            abs(current_sample.w - reference_colors[n_sample].w) > epsilon)\n"
						  "        {\n"
						  "            result = vec4(0.0);\n"
						  "\n"
						  "            break;\n"
						  "        }\n"
						  "    }\n"
						  "}\n";

	std::string fs_string = fs_body;

	while ((token_location = fs_string.find(token_n_samples)) != std::string::npos)
	{
		fs_string.replace(token_location, strlen(token_n_samples), n_samples_sstream.str());
	}

	while ((token_location = fs_string.find(token_sampler_declarations)) != std::string::npos)
	{
		fs_string.replace(token_location, strlen(token_sampler_declarations), sampler_declarations_string);
	}

	while ((token_location = fs_string.find(token_sample_fetch)) != std::string::npos)
	{
		fs_string.replace(token_location, strlen(token_sample_fetch), sample_fetch_fs_string);
	}

	/* Configure shader bodies */
	const char* fs_body_raw_ptr = fs_string.c_str();
	const char* gs_body_raw_ptr = gs_string.c_str();
	const char* tc_body_raw_ptr = tc_string.c_str();
	const char* te_body_raw_ptr = te_string.c_str();
	const char* vs_body_raw_ptr = vs_string.c_str();

	gl.shaderSource(m_fs_id, 1 /* count */, &fs_body_raw_ptr, NULL /* length */);
	gl.shaderSource(m_gs_id, 1 /* count */, &gs_body_raw_ptr, NULL /* length */);
	gl.shaderSource(m_tc_id, 1 /* count */, &tc_body_raw_ptr, NULL /* length */);
	gl.shaderSource(m_te_id, 1 /* count */, &te_body_raw_ptr, NULL /* length */);
	gl.shaderSource(m_vs_id, 1 /* count */, &vs_body_raw_ptr, NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call(s) failed.");

	/* Compile the shaders */
	const glw::GLuint  so_ids[] = { m_fs_id, m_gs_id, m_tc_id, m_te_id, m_vs_id };
	const unsigned int n_so_ids = sizeof(so_ids) / sizeof(so_ids[0]);

	const glw::GLchar* shader_sources[] = { fs_body_raw_ptr, gs_body_raw_ptr, tc_body_raw_ptr, te_body_raw_ptr,
											vs_body_raw_ptr };

	for (unsigned int n_so_id = 0; n_so_id < n_so_ids; ++n_so_id)
	{
		glw::GLint compile_status = GL_FALSE;
		glw::GLint so_id		  = so_ids[n_so_id];

		gl.compileShader(so_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

		gl.getShaderiv(so_id, GL_COMPILE_STATUS, &compile_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

		if (compile_status != GL_TRUE)
		{
			char temp[1024];

			gl.getShaderInfoLog(so_id, 1024, NULL, temp);

			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Shader compilation error log:\n"
												<< temp << "\nShader source:\n"
												<< shader_sources[n_so_id] << tcu::TestLog::EndMessage;

			TCU_FAIL("Shader compilation failed");
		}

		/* Attach the shaders to the program object */
		gl.attachShader(m_po_id, so_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed");
	} /* for (all shader objects) */

	/* Set up XFB */
	const char* varying_names[] = { "gs_fs_vs_sampling_result", "gs_fs_tc_sampling_result", "gs_fs_te_sampling_result",
									"gs_fs_gs_sampling_result" };
	const unsigned int n_varying_names = sizeof(varying_names) / sizeof(varying_names[0]);

	gl.transformFeedbackVaryings(m_po_id, n_varying_names, varying_names, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed.");

	/* Link the program object */
	glw::GLint link_status = GL_FALSE;

	gl.linkProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed.");
	}

	/* Retrieve uniform locations. Depending on the iteration, a number of those will be
	 * inactive.
	 */
	m_po_lod_location			   = gl.getUniformLocation(m_po_id, "lod");
	m_po_n_face_location		   = gl.getUniformLocation(m_po_id, "n_face");
	m_po_reference_colors_location = gl.getUniformLocation(m_po_id, "reference_colors");
	m_po_texture_location		   = gl.getUniformLocation(m_po_id, "texture");
	m_po_z_float_location		   = gl.getUniformLocation(m_po_id, "z_float");
	m_po_z_int_location			   = gl.getUniformLocation(m_po_id, "z_int");

	if (m_po_reference_colors_location == -1)
	{
		TCU_FAIL("reference_colors is considered an inactive uniform which is invalid.");
	}
}

/** Initializes contents of a texture, from which the view texture will be created. **/
void TextureViewTestViewSampling::initParentTextureContents()
{
	static const glw::GLenum cm_texture_targets[] = {
		/* NOTE: This order must match the order used for sampling CM/CMA texture targets. */
		GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};
	static const unsigned int n_cm_texture_targets = sizeof(cm_texture_targets) / sizeof(cm_texture_targets[0]);
	const glw::Functions&	 gl				   = m_context.getRenderContext().getFunctions();

	/* Bind the parent texture */
	gl.bindTexture(m_iteration_parent_texture_target, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	/* If we're dealing with a single-sampled texture target, then we can clear the
	 * contents of each layer/face/slice using FBO. This will unfortunately not work
	 * for arrayed textures, layers or layer-faces of which cannot be attached to a draw
	 * framebuffer.
	 * If we need to update contents of a multisampled, potentially arrayed texture,
	 * we'll need to use the filler program.
	 **/
	bool is_arrayed_texture_target = (m_iteration_parent_texture_target == GL_TEXTURE_1D_ARRAY ||
									  m_iteration_parent_texture_target == GL_TEXTURE_2D_ARRAY ||
									  m_iteration_parent_texture_target == GL_TEXTURE_CUBE_MAP_ARRAY);
	bool is_multisampled_texture_target = (m_iteration_parent_texture_target == GL_TEXTURE_2D_MULTISAMPLE ||
										   m_iteration_parent_texture_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY);

	if (!is_arrayed_texture_target && !is_multisampled_texture_target)
	{
		/* Good, no need to work with samples! */
		DE_ASSERT(m_iteration_parent_texture_depth >= 1);
		DE_ASSERT(m_iteration_parent_texture_n_levels >= 1);

		/* Cube-map texture target cannot be directly used for a glFramebufferTexture2D() call. Instead,
		 * we need to split it into 6 cube-map texture targets. */
		unsigned int n_texture_targets					   = 1;
		glw::GLenum  texture_targets[n_cm_texture_targets] = {
			m_iteration_parent_texture_target, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE,
		};

		if (m_iteration_parent_texture_target == GL_TEXTURE_CUBE_MAP)
		{
			DE_STATIC_ASSERT(sizeof(texture_targets) == sizeof(cm_texture_targets));
			memcpy(texture_targets, cm_texture_targets, sizeof(cm_texture_targets));

			n_texture_targets = n_cm_texture_targets;
		}

		resetReferenceColorStorage(m_iteration_parent_texture_depth,	/* n_layers */
								   n_texture_targets,					/* n_faces */
								   m_iteration_parent_texture_n_levels, /* n_mipmaps */
								   1);									/* n_samples */

		/* Iterate through all texture targets we need to update */
		for (unsigned int n_texture_target = 0; n_texture_target < n_texture_targets; ++n_texture_target)
		{
			const glw::GLenum texture_target = texture_targets[n_texture_target];

			/* Iterate through all layers of the texture. */
			for (unsigned int n_layer = 0; n_layer < m_iteration_parent_texture_depth; ++n_layer)
			{
				/* ..and mip-maps, too. */
				const unsigned int n_mipmaps_for_layer = (texture_target == GL_TEXTURE_3D) ?
															 (m_iteration_parent_texture_n_levels - n_layer) :
															 (m_iteration_parent_texture_n_levels);

				for (unsigned int n_mipmap = 0; n_mipmap < n_mipmaps_for_layer; ++n_mipmap)
				{
					/* Use appropriate glFramebufferTexture*() API, depending on the texture target of the
					 * parent texture.
					 */
					switch (texture_target)
					{
					case GL_TEXTURE_1D:
					{
						gl.framebufferTexture1D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_1D, m_to_id,
												n_mipmap);

						GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture1D() call failed.");
						break;
					}

					case GL_TEXTURE_2D:
					case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
					case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
					case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
					case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
					case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
					case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
					case GL_TEXTURE_RECTANGLE:
					{
						gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_target, m_to_id,
												n_mipmap);

						GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");
						break;
					}

					case GL_TEXTURE_3D:
					{
						gl.framebufferTexture3D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, m_to_id,
												n_mipmap, n_layer);

						GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture3D() call failed.");
						break;
					}

					default:
					{
						TCU_FAIL("Unrecognized texture target");
					}
					} /* switch (m_iteration_parent_texture_target) */

					/* Each layer/mipmap needs to be assigned an unique vec4. */
					tcu::Vec4 reference_color = getRandomReferenceColor();

					setReferenceColor(n_layer, n_texture_target, /* n_face */
									  n_mipmap, 0,				 /* n_sample */
									  reference_color);

					/* We should be OK to clear the mip-map at this point */
					gl.clearColor(reference_color.x(), reference_color.y(), reference_color.z(), reference_color.w());
					GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor() call failed.");

					gl.clear(GL_COLOR_BUFFER_BIT);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glClear() call failed.");
				} /* for (all mip-maps) */
			}	 /* for (all layers) */
		}		  /* for (all texture targets) */
	}			  /* if (!is_arrayed_texture_target && !is_multisampled_texture_target) */
	else
	{
		/* We need to handle an either multisampled or arrayed texture target or
		 * a combination of the two.
		 */
		DE_ASSERT(m_iteration_parent_texture_target == GL_TEXTURE_1D_ARRAY ||
				  m_iteration_parent_texture_target == GL_TEXTURE_2D_ARRAY ||
				  m_iteration_parent_texture_target == GL_TEXTURE_2D_MULTISAMPLE ||
				  m_iteration_parent_texture_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY ||
				  m_iteration_parent_texture_target == GL_TEXTURE_CUBE_MAP_ARRAY);

		DE_ASSERT(m_iteration_parent_texture_depth >= 1);
		DE_ASSERT(m_iteration_parent_texture_n_levels >= 1);

		const unsigned int n_faces =
			(m_iteration_parent_texture_target == GL_TEXTURE_CUBE_MAP_ARRAY) ? 6 /* faces */ : 1;

		resetReferenceColorStorage(m_iteration_parent_texture_depth,			 /* n_layers */
								   n_faces, m_iteration_parent_texture_n_levels, /* n_mipmaps */
								   m_max_color_texture_samples_gl_value);		 /* n_samples */

		/* Set up storage for reference colors the fragment shader should use
		 * when rendering to multisampled texture target */
		float* reference_colors = new float[4 /* rgba */ * m_max_color_texture_samples_gl_value];

		/* Activate the filler program */
		gl.useProgram(m_per_sample_filler_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

		/* Iterate through all layers of the texture. */
		for (unsigned int n_layer = 0; n_layer < m_iteration_parent_texture_depth; ++n_layer)
		{
			/* ..faces.. */
			for (unsigned int n_face = 0; n_face < n_faces; ++n_face)
			{
				/* ..and mip-maps, too. */
				for (unsigned int n_mipmap = 0; n_mipmap < m_iteration_parent_texture_n_levels; ++n_mipmap)
				{
					/* For all texture targets considered excl. GL_TEXTURE_2D_MULTISAMPLE, we need
					 * to use glFramebufferTextur() to bind all layers to the color atatchment. For
					 * 2DMS textures, we can use plain glFramebufferTexture2D().
					 */
					if (m_iteration_parent_texture_target != GL_TEXTURE_2D_MULTISAMPLE)
					{
						gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_to_id, n_mipmap);

						GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTextureLayer() call failed.");
					}
					else
					{
						/* Sanity check */
						DE_ASSERT(m_iteration_parent_texture_depth == 1);

						gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
												m_iteration_parent_texture_target, m_to_id, n_mipmap);

						GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");
					}

					/* Generate reference colors for all samples */
					const unsigned int n_samples =
						(is_multisampled_texture_target) ? m_max_color_texture_samples_gl_value : 1;

					for (unsigned int n_sample = 0; n_sample < n_samples; ++n_sample)
					{
						tcu::Vec4 reference_color = getRandomReferenceColor();

						reference_colors[4 /* rgba */ * n_sample + 0] = reference_color.x();
						reference_colors[4 /* rgba */ * n_sample + 1] = reference_color.y();
						reference_colors[4 /* rgba */ * n_sample + 2] = reference_color.z();
						reference_colors[4 /* rgba */ * n_sample + 3] = reference_color.w();

						setReferenceColor(n_layer, n_face, n_mipmap, n_sample, reference_color);
					} /* for (all samples) */

					/* Upload the reference sample colors */
					gl.uniform4fv(m_per_sample_filler_po_reference_colors_location, n_samples, reference_colors);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform4fv() call failed.");

					/* Update the layer ID the program should render to */
					const unsigned int layer_id = n_face * 6 /* faces */ + n_layer;

					gl.uniform1i(m_per_sample_filler_po_layer_id_location, layer_id);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() call failed.");

					/* Draw the full-screen quad. Geometry shader will draw the quad for us,
					 * so all we need to do is to feed the rendering pipeline with a single
					 * point.
					 */
					gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");
				} /* for (all mip-maps) */
			}	 /* for (all faces) */
		}		  /* for (all layers) */

		delete[] reference_colors;
	}
}

/** Initializes the 'per sample filler' program object, used to fill a multi-sample texture
 *  with colors varying on a per-sample basis.
 */
void TextureViewTestViewSampling::initPerSampleFillerProgramObject()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Sanity checks: GL_MAX_COLOR_TEXTURE_SAMPLES is not 0 */
	DE_ASSERT(m_max_color_texture_samples_gl_value != 0);

	/* Generate program and shader objects */
	m_per_sample_filler_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_per_sample_filler_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_per_sample_filler_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	m_per_sample_filler_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Configure fragment shader's body */
	static const char* fs_body = "#version 400\n"
								 "\n"
								 "uniform vec4 reference_colors[N_MAX_SAMPLES];\n"
								 "\n"
								 "out vec4 result;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    result = reference_colors[gl_SampleID];\n"
								 "}\n";
	std::string		  fs_body_string		 = fs_body;
	const char*		  fs_body_string_raw_ptr = DE_NULL;
	std::stringstream n_max_samples_sstream;
	const char*		  n_max_samples_token = "N_MAX_SAMPLES";
	std::size_t		  token_location	  = std::string::npos;

	n_max_samples_sstream << m_max_color_texture_samples_gl_value;

	while ((token_location = fs_body_string.find(n_max_samples_token)) != std::string::npos)
	{
		fs_body_string.replace(token_location, strlen(n_max_samples_token), n_max_samples_sstream.str());
	}

	fs_body_string_raw_ptr = fs_body_string.c_str();

	gl.shaderSource(m_per_sample_filler_fs_id, 1 /* count */, &fs_body_string_raw_ptr, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	/* Configure geometry shader's body */
	static const char* gs_body = "#version 400\n"
								 "\n"
								 "layout(points)                           in;\n"
								 "layout(triangle_strip, max_vertices = 4) out;\n"
								 "\n"
								 "uniform int layer_id;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    gl_Layer    = layer_id;\n"
								 "    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
								 "    EmitVertex();\n"
								 "\n"
								 "    gl_Layer    = layer_id;\n"
								 "    gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
								 "    EmitVertex();\n"
								 "\n"
								 "    gl_Layer    = layer_id;\n"
								 "    gl_Position = vec4( 1.0, -1.0, 0.0, 1.0);\n"
								 "    EmitVertex();\n"
								 "\n"
								 "    gl_Layer    = layer_id;\n"
								 "    gl_Position = vec4( 1.0,  1.0, 0.0, 1.0);\n"
								 "    EmitVertex();\n"
								 "\n"
								 "    EndPrimitive();\n"
								 "\n"
								 "}\n";

	gl.shaderSource(m_per_sample_filler_gs_id, 1 /* count */, &gs_body, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	/* Configure vertex shader */
	static const char* vs_body = "#version 400\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
								 "}\n";

	gl.shaderSource(m_per_sample_filler_vs_id, 1 /* count */, &vs_body, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	/* Attach the shaders to the program object */
	gl.attachShader(m_per_sample_filler_po_id, m_per_sample_filler_fs_id);
	gl.attachShader(m_per_sample_filler_po_id, m_per_sample_filler_gs_id);
	gl.attachShader(m_per_sample_filler_po_id, m_per_sample_filler_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed.");

	/* Compile the shaders */
	const glw::GLuint  so_ids[] = { m_per_sample_filler_fs_id, m_per_sample_filler_gs_id, m_per_sample_filler_vs_id };
	const unsigned int n_so_ids = sizeof(so_ids) / sizeof(so_ids[0]);

	for (unsigned int n_so_id = 0; n_so_id < n_so_ids; ++n_so_id)
	{
		glw::GLint  compile_status = GL_FALSE;
		glw::GLuint so_id		   = so_ids[n_so_id];

		gl.compileShader(so_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

		gl.getShaderiv(so_id, GL_COMPILE_STATUS, &compile_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiV() call failed.");

		if (compile_status != GL_TRUE)
		{
			TCU_FAIL("Shader compilation failed.");
		}
	} /* for (all shader objects) */

	/* Link the program object */
	glw::GLint link_status = GL_FALSE;

	gl.linkProgram(m_per_sample_filler_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(m_per_sample_filler_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed.");
	}

	/* Retrieve uniform locations */
	m_per_sample_filler_po_layer_id_location = gl.getUniformLocation(m_per_sample_filler_po_id, "layer_id");
	m_per_sample_filler_po_reference_colors_location =
		gl.getUniformLocation(m_per_sample_filler_po_id, "reference_colors[0]");

	if (m_per_sample_filler_po_layer_id_location == -1)
	{
		TCU_FAIL("layer_id uniform is considered inactive which is invalid");
	}

	if (m_per_sample_filler_po_reference_colors_location == -1)
	{
		TCU_FAIL("reference_colors uniform is considered inactive which is invalid");
	}
}

/** Initializes GL objects needed to run the test (excluding iteration-specific objects) */
void TextureViewTestViewSampling::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate and configure BO storage to hold result XFB data of a single
	 * draw call.
	 *
	 * Each draw call outputs 6 vertices. For each vertex, 4 ints will be XFBed out. */
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 6 /* draw calls */ * (4 * sizeof(int)), /* as per comment */
				  DE_NULL, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	/* Generate a FBO and bind it to both binding targets */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	/* Generate and bind a VAO */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	/* Generate and configure a texture object we will use to verify view sampling works correctly
	 * from within a fragment shader.
	 */
	gl.genTextures(1, &m_result_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	gl.bindTexture(GL_TEXTURE_2D, m_result_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1 /* levels */, GL_RGBA8, m_reference_texture_width, m_reference_texture_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

	/* Determine implementation-specific GL_MAX_COLOR_TEXTURE_SAMPLES value */
	gl.getIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &m_max_color_texture_samples_gl_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed for GL_MAX_COLOR_TEXTURE_SAMPLES pname.");

	/* Modify pixel storage settings so that we don't rely on the default aligment setting. */
	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei() call(s) failed.");

	/* Modify GL_PATCH_VERTICES setting so that a single patch consists of only a single vertex
	 * (instead of the default 3) */
	gl.patchParameteri(GL_PATCH_VERTICES, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteri() call failed.");
}

/** Initializes and sets up a texture object storage, but does not fill it
 *  with actual content. Implements separate code paths for handling parent
 *  & view textures.
 *
 *  @param is_view_texture     true if a view texture should be initialized,
 *                             false if te caller needs a parent texture. Note
 *                             that a parent texture must be initialized prior
 *                             to configuring a view texture.
 *  @param texture_target      Texture target to use for the parent texture.
 *  @param view_texture_target Texture target to use for the view texture.
 **/
void TextureViewTestViewSampling::initTextureObject(bool is_view_texture, glw::GLenum texture_target,
													glw::GLenum view_texture_target)
{
	const glw::Functions& gl			= m_context.getRenderContext().getFunctions();
	unsigned int		  texture_depth = 0;
	glw::GLuint*		  to_id_ptr		= (is_view_texture) ? &m_view_to_id : &m_to_id;

	/* Sanity check: make sure GL_TEXTURE_BUFFER texture target is not requested. This
	 *               would be against the test specification.
	 **/
	DE_ASSERT(texture_target != GL_TEXTURE_BUFFER);
	DE_ASSERT(view_texture_target != GL_TEXTURE_BUFFER);

	/* If we're going to be creating a cube-map or cube-map array texture view in this iteration,
	 * make sure the parent or view texture's depth is valid */
	if (view_texture_target == GL_TEXTURE_CUBE_MAP_ARRAY)
	{
		texture_depth = 13; /* 1 + 2 * (6 faces) */
	}
	else if (view_texture_target == GL_TEXTURE_CUBE_MAP)
	{
		texture_depth = 7; /* 1 + (6 faces) */
	}
	else
	{
		texture_depth = m_reference_texture_depth;
	}

	if (texture_target == GL_TEXTURE_CUBE_MAP_ARRAY)
	{
		texture_depth = 6 /* faces */ * 3;
	}

	/* Release the texture object, as we're using immutable texture objects and would
	 * prefer the resources not to leak.
	 */
	if (*to_id_ptr != 0)
	{
		gl.deleteTextures(1, to_id_ptr);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() call failed.");
	}

	/* Generate a new texture object */
	gl.genTextures(1, to_id_ptr);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	if (is_view_texture)
	{
		/* Determine values of arguments we'll pass to glTextureView() call */
		unsigned int minlayer  = 0;
		unsigned int minlevel  = 0;
		unsigned int numlayers = 0;
		unsigned int numlevels = 2;

		const bool is_texture_arrayed_texture_target =
			(texture_target == GL_TEXTURE_1D_ARRAY || texture_target == GL_TEXTURE_2D_ARRAY ||
			 texture_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY || texture_target == GL_TEXTURE_CUBE_MAP_ARRAY);
		const bool is_texture_cube_map_texture_target = (texture_target == GL_TEXTURE_CUBE_MAP);
		const bool is_texture_multisample_texture_target =
			(texture_target == GL_TEXTURE_2D_MULTISAMPLE || texture_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY);
		const bool is_texture_rectangle_texture_target = (texture_target == GL_TEXTURE_RECTANGLE);
		const bool is_view_arrayed_texture_target =
			(view_texture_target == GL_TEXTURE_1D_ARRAY || view_texture_target == GL_TEXTURE_2D_ARRAY ||
			 view_texture_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY ||
			 view_texture_target == GL_TEXTURE_CUBE_MAP_ARRAY);
		const bool is_view_cube_map_texture_target		 = (view_texture_target == GL_TEXTURE_CUBE_MAP);
		const bool is_view_cube_map_array_texture_target = (view_texture_target == GL_TEXTURE_CUBE_MAP_ARRAY);

		if (is_texture_multisample_texture_target || is_texture_rectangle_texture_target)
		{
			minlevel  = 0;
			numlevels = 1;
		}
		else
		{
			minlevel = 1;
		}

		if ((true == is_texture_arrayed_texture_target) ||
			((false == is_texture_cube_map_texture_target) && (true == is_view_cube_map_texture_target)) ||
			((false == is_texture_cube_map_texture_target) && (true == is_view_cube_map_array_texture_target)))
		{
			minlayer = 1;
		}
		else
		{
			minlayer = 0;
		}

		if (!is_texture_cube_map_texture_target && is_view_cube_map_array_texture_target)
		{
			numlayers = 12;
		}
		else if (is_view_cube_map_texture_target || is_view_cube_map_array_texture_target)
		{
			numlayers = 6;
		}
		else if (is_view_arrayed_texture_target)
		{
			if (is_texture_arrayed_texture_target || is_texture_cube_map_texture_target)
			{
				numlayers = 2;
			}
			else
			{
				numlayers = 1;
			}
		}
		else
		{
			numlayers = 1;
		}

		/* Set up view texture */
		gl.textureView(*to_id_ptr, view_texture_target, m_to_id, GL_RGBA8, minlevel, numlevels, minlayer, numlayers);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureView() call failed.");

		/* Store the argument values */
		m_iteration_view_texture_minlayer  = minlayer;
		m_iteration_view_texture_minlevel  = minlevel;
		m_iteration_view_texture_numlayers = numlayers;
		m_iteration_view_texture_numlevels = numlevels;
		m_iteration_view_texture_target	= view_texture_target;

		m_testCtx.getLog() << tcu::TestLog::Message << "Created a view for texture target "
						   << "[" << TextureViewUtilities::getTextureTargetString(view_texture_target) << "] "
						   << "from a parent texture target "
						   << "[" << TextureViewUtilities::getTextureTargetString(texture_target) << "] "
						   << "using arguments: "
						   << "minlayer:[" << minlayer << "] "
						   << "minlevel:[" << minlevel << "] "
						   << "numlayers:[" << numlayers << "] "
						   << "numlevels:[" << numlevels << "]." << tcu::TestLog::EndMessage;

		gl.bindTexture(view_texture_target, *to_id_ptr);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");
	} /* if (is_view_texture) */
	else
	{
		/* Reset iteration-specific view settings */
		m_iteration_parent_texture_depth	 = 1;
		m_iteration_parent_texture_height	= 1;
		m_iteration_parent_texture_n_levels  = 1;
		m_iteration_parent_texture_n_samples = 1;
		m_iteration_parent_texture_width	 = 1;

		/* Initialize storage for the newly created texture object */
		gl.bindTexture(texture_target, *to_id_ptr);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

		/* Use max conformant sample count for multisample texture targets */
		if (texture_target == GL_TEXTURE_2D_MULTISAMPLE || texture_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
		{
			m_max_color_texture_samples_gl_value = getMaxConformantSampleCount(texture_target, GL_RGBA8);
		}
		else
		{
			/* Use GL_MAX_COLOR_TEXTURE_SAMPLES value for other targets */
			gl.getIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &m_max_color_texture_samples_gl_value);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed for GL_MAX_COLOR_TEXTURE_SAMPLES pname.");
		}

		switch (texture_target)
		{
		case GL_TEXTURE_1D:
		{
			gl.texStorage1D(texture_target, m_reference_texture_n_mipmaps, GL_RGBA8, m_reference_texture_width);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage1D() call failed.");

			m_iteration_parent_texture_n_levels = m_reference_texture_n_mipmaps;
			m_iteration_parent_texture_width	= m_reference_texture_width;

			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Created an immutable parent texture object for texture target "
							   << "[" << TextureViewUtilities::getTextureTargetString(texture_target) << "] "
							   << "of "
							   << "levels:[" << m_reference_texture_n_mipmaps << "] "
							   << "width:[" << m_reference_texture_width << "]." << tcu::TestLog::EndMessage;
			break;
		}

		case GL_TEXTURE_1D_ARRAY:
		{
			gl.texStorage2D(texture_target, m_reference_texture_n_mipmaps, GL_RGBA8, m_reference_texture_width,
							texture_depth);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

			m_iteration_parent_texture_depth	= texture_depth;
			m_iteration_parent_texture_n_levels = m_reference_texture_n_mipmaps;
			m_iteration_parent_texture_width	= m_reference_texture_width;

			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Created an immutable parent texture object for texture target "
							   << "[" << TextureViewUtilities::getTextureTargetString(texture_target) << "] "
							   << "of "
							   << "depth:[" << texture_depth << "] "
							   << "levels:[" << m_reference_texture_n_mipmaps << "] "
							   << "width:[" << m_reference_texture_width << "]." << tcu::TestLog::EndMessage;

			break;
		}

		case GL_TEXTURE_CUBE_MAP:
		case GL_TEXTURE_2D:
		{
			gl.texStorage2D(texture_target, m_reference_texture_n_mipmaps, GL_RGBA8, m_reference_texture_width,
							m_reference_texture_height);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

			m_iteration_parent_texture_height   = m_reference_texture_height;
			m_iteration_parent_texture_n_levels = m_reference_texture_n_mipmaps;
			m_iteration_parent_texture_width	= m_reference_texture_width;

			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Created an immutable parent texture object for texture target "
							   << "[" << TextureViewUtilities::getTextureTargetString(texture_target) << "] "
							   << "of "
							   << "levels:[" << m_reference_texture_n_mipmaps << "] "
							   << "width:[" << m_reference_texture_width << "] "
							   << "height:[" << m_reference_texture_height << "]." << tcu::TestLog::EndMessage;

			break;
		}

		case GL_TEXTURE_RECTANGLE:
		{
			gl.texStorage2D(texture_target, 1, /* rectangle textures do not use mip-maps */
							GL_RGBA8, m_reference_texture_width, m_reference_texture_height);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

			m_iteration_parent_texture_height = m_reference_texture_height;
			m_iteration_parent_texture_width  = m_reference_texture_width;

			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Created an immutable parent texture object for texture target "
							   << "[" << TextureViewUtilities::getTextureTargetString(texture_target) << "] "
							   << "of "
							   << "levels:1 "
							   << "width:[" << m_reference_texture_width << "] "
							   << "height:[" << m_reference_texture_height << "]." << tcu::TestLog::EndMessage;

			break;
		}

		case GL_TEXTURE_2D_ARRAY:
		{
			gl.texStorage3D(texture_target, m_reference_texture_n_mipmaps, GL_RGBA8, m_reference_texture_width,
							m_reference_texture_height, texture_depth);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D() call failed.");

			m_iteration_parent_texture_depth	= texture_depth;
			m_iteration_parent_texture_height   = m_reference_texture_height;
			m_iteration_parent_texture_n_levels = m_reference_texture_n_mipmaps;
			m_iteration_parent_texture_width	= m_reference_texture_width;

			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Created an immutable parent texture object for texture target "
							   << "[" << TextureViewUtilities::getTextureTargetString(texture_target) << "] "
							   << "of "
							   << "depth:[" << texture_depth << "] "
							   << "levels:[" << m_reference_texture_n_mipmaps << "] "
							   << "width:[" << m_reference_texture_width << "] "
							   << "height:[" << m_reference_texture_height << "]." << tcu::TestLog::EndMessage;

			break;
		}

		case GL_TEXTURE_2D_MULTISAMPLE:
		{
			gl.texStorage2DMultisample(texture_target, m_max_color_texture_samples_gl_value, GL_RGBA8,
									   m_reference_texture_width, m_reference_texture_height,
									   GL_TRUE); /* fixedsamplelocations */

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample() call failed.");

			m_iteration_parent_texture_height	= m_reference_texture_height;
			m_iteration_parent_texture_n_samples = m_max_color_texture_samples_gl_value;
			m_iteration_parent_texture_width	 = m_reference_texture_width;

			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Created an immutable parent texture object for texture target "
							   << "[" << TextureViewUtilities::getTextureTargetString(texture_target) << "] "
							   << "of "
							   << "samples:[" << m_max_color_texture_samples_gl_value << "] "
							   << "width:[" << m_reference_texture_width << "] "
							   << "height:[" << m_reference_texture_height << "]." << tcu::TestLog::EndMessage;

			break;
		}

		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		{
			gl.texStorage3DMultisample(texture_target, m_max_color_texture_samples_gl_value, GL_RGBA8,
									   m_reference_texture_width, m_reference_texture_height, texture_depth,
									   GL_TRUE); /* fixed sample locations */

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3DMultisample() call failed.");

			m_iteration_parent_texture_depth	 = texture_depth;
			m_iteration_parent_texture_height	= m_reference_texture_height;
			m_iteration_parent_texture_n_samples = m_max_color_texture_samples_gl_value;
			m_iteration_parent_texture_width	 = m_reference_texture_width;

			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Created an immutable parent texture object for texture target "
							   << "[" << TextureViewUtilities::getTextureTargetString(texture_target) << "] "
							   << "of "
							   << "samples:[" << m_max_color_texture_samples_gl_value << "] "
							   << "width:[" << m_reference_texture_width << "] "
							   << "height:[" << m_reference_texture_height << "] "
							   << "depth:[" << texture_depth << "]." << tcu::TestLog::EndMessage;

			break;
		}

		case GL_TEXTURE_3D:
		case GL_TEXTURE_CUBE_MAP_ARRAY:
		{
			gl.texStorage3D(texture_target, m_reference_texture_n_mipmaps, GL_RGBA8, m_reference_texture_width,
							m_reference_texture_height, texture_depth);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D() call failed.");

			m_iteration_parent_texture_depth	= texture_depth;
			m_iteration_parent_texture_height   = m_reference_texture_height;
			m_iteration_parent_texture_n_levels = m_reference_texture_n_mipmaps;
			m_iteration_parent_texture_width	= m_reference_texture_width;

			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Created an immutable parent texture object for texture target "
							   << "[" << TextureViewUtilities::getTextureTargetString(texture_target) << "] "
							   << "of "
							   << "levels:[" << m_reference_texture_n_mipmaps << "] "
							   << "width:[" << m_reference_texture_width << "] "
							   << "height:[" << m_reference_texture_height << "] "
							   << "depth:[" << texture_depth << "]." << tcu::TestLog::EndMessage;

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized texture target.");
		}
		} /* switch (texture_target) */

		m_iteration_parent_texture_target = texture_target;
	}

	/* Configure texture filtering */
	if (texture_target != GL_TEXTURE_2D_MULTISAMPLE && texture_target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY &&
		texture_target != GL_TEXTURE_RECTANGLE)
	{
		gl.texParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		gl.texParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri() call(s) failed.");
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult TextureViewTestViewSampling::iterate()
{
	bool has_failed = false;

	/* Make sure GL_ARB_texture_view is reported as supported before carrying on
	 * with actual execution */
	const std::vector<std::string>& extensions = m_context.getContextInfo().getExtensions();

	if (std::find(extensions.begin(), extensions.end(), "GL_ARB_texture_view") == extensions.end())
	{
		throw tcu::NotSupportedError("GL_ARB_texture_view is not supported");
	}

	/* Initialize all objects required to run the test */
	initTest();

	/* Initialize per-sample filler program */
	initPerSampleFillerProgramObject();

	/* Iterate through all texture/view texture target combinations */
	TextureViewUtilities::_compatible_texture_target_pairs_const_iterator texture_target_iterator;
	TextureViewUtilities::_compatible_texture_target_pairs				  texture_target_pairs =
		TextureViewUtilities::getLegalTextureAndViewTargetCombinations();

	for (texture_target_iterator = texture_target_pairs.begin(); texture_target_iterator != texture_target_pairs.end();
		 ++texture_target_iterator)
	{
		const glw::GLenum parent_texture_target = texture_target_iterator->first;
		const glw::GLenum view_texture_target   = texture_target_iterator->second;

		/* Initialize parent texture */
		initTextureObject(false /* is_view_texture */, parent_texture_target, view_texture_target);

		/* Initialize view */
		initTextureObject(true /* is_view_texture */, parent_texture_target, view_texture_target);

		/* Initialize contents of the parent texture */
		initParentTextureContents();

		/* Initialize iteration-specific test program object */
		initIterationSpecificProgramObject();

		/* Run the actual test */
		bool status = executeTest();

		if (!status)
		{
			has_failed = true;

			m_testCtx.getLog() << tcu::TestLog::Message << "Test case failed." << tcu::TestLog::EndMessage;
		}
		else
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Test case succeeded." << tcu::TestLog::EndMessage;
		}
	}

	if (!has_failed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** De-allocates existing reference color storage (if one already exists) and
 *  allocates a new one using user-provided properties.
 *
 *  @param n_layers  Amount of layers to consider. Use a value of 1 for non-arrayed
 *                   texture targets.
 *  @param n_faces   Amount of faces to consider. Use a value of 1 for non-CM
 *                   texture targets.
 *  @param n_mipmaps Amount of mip-maps to consider. Use a value of 1 for non-mipmapped
 *                   texture targets.
 *  @param n_samples Amount of samples to consider. Use a value of 1 for single-sampled
 *                   texture targets.
 **/
void TextureViewTestViewSampling::resetReferenceColorStorage(unsigned int n_layers, unsigned int n_faces,
															 unsigned int n_mipmaps, unsigned int n_samples)
{
	/* Sanity checks */
	DE_ASSERT(n_layers >= 1);
	DE_ASSERT(n_faces >= 1);
	DE_ASSERT(n_mipmaps >= 1);
	DE_ASSERT(n_samples >= 1);

	/* Allocate the descriptor if it's the first time the test will be
	 * attempting to access it */
	if (m_reference_color_storage == DE_NULL)
	{
		m_reference_color_storage = new _reference_color_storage(n_faces, n_layers, n_mipmaps, n_samples);
	}
	else
	{
		/* The descriptor's already there so we only need to update the properties */
		m_reference_color_storage->n_faces   = n_faces;
		m_reference_color_storage->n_layers  = n_layers;
		m_reference_color_storage->n_mipmaps = n_mipmaps;
		m_reference_color_storage->n_samples = n_samples;
	}

	/* If there's any data descriptor found allocated at this point,
	 * release it */
	if (m_reference_color_storage->data != DE_NULL)
	{
		delete[] m_reference_color_storage->data;

		m_reference_color_storage->data = DE_NULL;
	}

	m_reference_color_storage->data = new tcu::Vec4[n_layers * n_faces * n_mipmaps * n_samples];
}

/** Assigns user-specified reference color to a specific sample of a layer/face's mip-map.
 *
 *  This function throws an assertion failure if the requested layer/face/mip-map/sample index
 *  is invalid.
 *
 *  @param n_layer  Layer index to use for the association. Use a value of 0 for non-arrayed texture
 *                  targets.
 *  @param n_face   Face index to use for the association. Use a value of 0 for non-CM texture targets.
 *  @param n_mipmap Mip-map index to use for the association. Use a value of 0 for non-mipmapped texture
 *                  targets.
 *  @param n_sample Sample index to use for the association. Use a value of 0 for single-sampled texture
 *                  targets.
 *  @param color    Color to associate with the specified sample.
 **/
void TextureViewTestViewSampling::setReferenceColor(unsigned int n_layer, unsigned int n_face, unsigned int n_mipmap,
													unsigned int n_sample, tcu::Vec4 color)
{
	DE_ASSERT(m_reference_color_storage != DE_NULL);
	if (m_reference_color_storage != DE_NULL)
	{
		DE_ASSERT(n_face < m_reference_color_storage->n_faces);
		DE_ASSERT(n_layer < m_reference_color_storage->n_layers);
		DE_ASSERT(n_mipmap < m_reference_color_storage->n_mipmaps);
		DE_ASSERT(n_sample < m_reference_color_storage->n_samples);

		/* Hierarchy is:
		 *
		 * layers -> faces -> mipmaps -> samples */
		const unsigned int index =
			n_layer * (m_reference_color_storage->n_faces * m_reference_color_storage->n_mipmaps *
					   m_reference_color_storage->n_samples) +
			n_face * (m_reference_color_storage->n_mipmaps * m_reference_color_storage->n_samples) +
			n_mipmap * (m_reference_color_storage->n_samples) + n_sample;

		m_reference_color_storage->data[index] = color;
	}
}

/** Constructor.
 *
 *  @param context Rendering context.
 **/
TextureViewTestViewClasses::TextureViewTestViewClasses(deqp::Context& context)
	: TestCase(context, "view_classes", "Verifies view sampling works correctly. Tests all valid"
										" texture/view internalformat combinations.")
	, m_bo_id(0)
	, m_po_id(0)
	, m_to_id(0)
	, m_to_temp_id(0)
	, m_vao_id(0)
	, m_view_to_id(0)
	, m_vs_id(0)
	, m_decompressed_mipmap_data(DE_NULL)
	, m_mipmap_data(DE_NULL)
	, m_bo_size(0)
	, m_has_test_failed(false)
	, m_texture_height(4)
	, m_texture_unit_for_parent_texture(GL_TEXTURE0)
	, m_texture_unit_for_view_texture(GL_TEXTURE1)
	, m_texture_width(4)
	, m_view_data_offset(0)
{
	/* Left blank on purpose */
}

/** Deinitializes all buffers and GL objects that may have been created
 *  during test execution. Also restores GL state that may have been modified.
 **/
void TextureViewTestViewClasses::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	if (m_decompressed_mipmap_data != DE_NULL)
	{
		delete[] m_decompressed_mipmap_data;

		m_decompressed_mipmap_data = DE_NULL;
	}

	if (m_mipmap_data != DE_NULL)
	{
		delete[] m_mipmap_data;

		m_mipmap_data = DE_NULL;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);

		m_to_id = 0;
	}

	if (m_to_temp_id != 0)
	{
		gl.deleteTextures(1, &m_to_temp_id);

		m_to_temp_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_view_to_id != 0)
	{
		gl.deleteTextures(1, &m_view_to_id);

		m_view_to_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	/* Bring back the default pixel storage settings that the test may have modified. */
	gl.pixelStorei(GL_PACK_ALIGNMENT, 4);
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 4);

	/* Restore rasterization */
	gl.enable(GL_RASTERIZER_DISCARD);
}

/** Reads user-specified amount of components and stores it in user-provided location,
 *  according to user-defined format (snorm, unorm, etc.) and component size.
 *
 *  This function assumes component sizes are aligned on by boundary (that is: size % 8
 *  equals 0 for all components). An assertion failure will occur if this requirement is
 *  not met.
 *  This function throws TestError exception if any of the requested component sizes
 *  or format is not supported.
 *
 *  @param data            Raw data buffer to read the data from.
 *  @param n_components    Amount of components to read.
 *  @param component_sizes 4 ints subsequently defining component size for R/G/B/A channels.
 *                         Must not be NULL.
 *  @param format          Format to be used for data retrieval. This defines data format of
 *                         the underlying components (for instance: for UNORMs we need to
 *                         divide the read ubyte/ushort/uint data by maximum value allowed for
 *                         the type minus one)
 *  @param result          Location to store the read components. Must not be NULL. Must be
 *                         large enough to hold requested amount of components of user-specified
 *                         component size.
 *
 **/
void TextureViewTestViewClasses::getComponentDataForByteAlignedInternalformat(const unsigned char* data,
																			  const unsigned int   n_components,
																			  const unsigned int*  component_sizes,
																			  const _format format, void* result)
{
	float*		  result_float = (float*)result;
	signed int*   result_sint  = (signed int*)result;
	unsigned int* result_uint  = (unsigned int*)result;

	/* Sanity checks: we assume the components are aligned on byte boundary. */
	DE_ASSERT((component_sizes[0] % 8) == 0 && (component_sizes[1] % 8) == 0 && (component_sizes[2] % 8) == 0 &&
			  (component_sizes[3] % 8) == 0);

	for (unsigned int n_component = 0; n_component < n_components;
		 data += (component_sizes[n_component] >> 3 /* 8 bits/byte */), ++n_component)
	{
		switch (format)
		{
		case FORMAT_FLOAT:
		{
			switch (component_sizes[n_component])
			{
			case 16:
				result_float[n_component] = deFloat16To32(*(const deFloat16*)data);
				break;
			case 32:
				result_float[n_component] = *(float*)data;
				break;

			default:
				TCU_FAIL("Unsupported component size");
			}

			break;
		}

		case FORMAT_SIGNED_INTEGER:
		{
			switch (component_sizes[n_component])
			{
			case 8:
				result_sint[n_component] = *(signed char*)data;
				break;
			case 16:
				result_sint[n_component] = *(signed short*)data;
				break;
			case 32:
				result_sint[n_component] = *(signed int*)data;
				break;

			default:
				TCU_FAIL("Unsupported component size");
			}

			break;
		}

		case FORMAT_SNORM:
		{
			switch (component_sizes[n_component])
			{
			case 8:
				result_float[n_component] = float(*(signed char*)data) / 127.0f;
				break;
			case 16:
				result_float[n_component] = float(*(signed short*)data) / 32767.0f;
				break;

			default:
				TCU_FAIL("Unsupported component size");
			}

			if (result_float[n_component] < -1.0f)
			{
				result_float[n_component] = -1.0f;
			}

			break;
		}

		case FORMAT_UNORM:
		{
			switch (component_sizes[n_component])
			{
			case 8:
				result_float[n_component] = float(*((unsigned char*)data)) / 255.0f;
				break;
			case 16:
				result_float[n_component] = float(*((unsigned short*)data)) / 65535.0f;
				break;

			default:
				TCU_FAIL("Unsupported component size");
			}

			break;
		}

		case FORMAT_UNSIGNED_INTEGER:
		{
			switch (component_sizes[n_component])
			{
			case 8:
				result_uint[n_component] = *(unsigned char*)data;
				break;
			case 16:
				result_uint[n_component] = *(unsigned short*)data;
				break;
			case 32:
				result_uint[n_component] = *(unsigned int*)data;
				break;

			default:
				TCU_FAIL("Unsupported component size");
			}

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized mip-map format");
		}
		} /* switch (view_format) */
	}	 /* for (all components) */
}

/** Initializes buffer object storage of sufficient size to hold data that will be
 *  XFBed out by the test's vertex shader, given user-specified parent texture &
 *  view's internalformats.
 *
 *  Throws TestError exceptions if GL calls fail.
 *
 *  @param texture_internalformat Internalformat used by the parent texture object,
 *                                from which the view will be created.
 *  @param view_internalformat    Internalformat that will be used by the texture view.
 **/
void TextureViewTestViewClasses::initBufferObject(glw::GLenum texture_internalformat, glw::GLenum view_internalformat)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Calculate how much size we will need to read the XFBed data. Each sampled data
	 * will either end up stored in a vecX, ivecX or uvecX (where X stands for amount
	 * of components supported by considered internalformat), so we can assume it will
	 * take a sizeof(float) = sizeof(int) = sizeof(unsigned int)
	 */
	const unsigned int parent_texture_n_components =
		TextureViewUtilities::getAmountOfComponentsForInternalformat(texture_internalformat);
	const unsigned int view_texture_n_components =
		TextureViewUtilities::getAmountOfComponentsForInternalformat(view_internalformat);

	/* Configure buffer object storage.
	 *
	 * NOTE: We do not care about the data type of the stored data, since sizes of the
	 *       types we're interested in (floats, ints and uints) match.
	 */
	DE_ASSERT(sizeof(float) == sizeof(unsigned int) && sizeof(float) == sizeof(int));

	m_bo_size =
		static_cast<unsigned int>(parent_texture_n_components * sizeof(float) * m_texture_height * m_texture_width +
								  view_texture_n_components * sizeof(float) * m_texture_height * m_texture_width);

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_size, DE_NULL, /* data */
				  GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	/* For XFB, we'll be outputting data sampled from both the texture and the view.
	 * Sampled texture data will go to the first half of the buffer, and the corresponding
	 * view data will go to the one half.
	 *
	 * Store the offset, from which the view's data will start so that we can correctly
	 * configure buffer object bindings in initProgramObject()
	 **/
	m_view_data_offset =
		static_cast<unsigned int>(parent_texture_n_components * sizeof(float) * m_texture_height * m_texture_width);
}

/** Initializes a program object that should be used for the test, given
 *  user-specified texture and view internalformats.
 *
 *  @param texture_internalformat Internalformat used by the parent texture object,
 *                                from which the view will be created.
 *  @param view_internalformat    Internalformat that will be used by the texture view.
 **/
void TextureViewTestViewClasses::initProgramObject(glw::GLenum texture_internalformat, glw::GLenum view_internalformat)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Determine which samplers we should be using for sampling both textures */
	const unsigned int texture_n_components =
		TextureViewUtilities::getAmountOfComponentsForInternalformat(texture_internalformat);
	const _sampler_type texture_sampler_type =
		TextureViewUtilities::getSamplerTypeForInternalformat(texture_internalformat);
	const char* texture_sampler_data_type_glsl =
		TextureViewUtilities::getGLSLDataTypeForSamplerType(texture_sampler_type, texture_n_components);
	const char* texture_sampler_glsl = TextureViewUtilities::getGLSLTypeForSamplerType(texture_sampler_type);
	const char* texture_swizzle_glsl =
		(texture_n_components == 4) ? "xyzw" :
									  (texture_n_components == 3) ? "xyz" : (texture_n_components == 2) ? "xy" : "x";
	const unsigned int view_n_components =
		TextureViewUtilities::getAmountOfComponentsForInternalformat(view_internalformat);
	const _sampler_type view_sampler_type = TextureViewUtilities::getSamplerTypeForInternalformat(view_internalformat);
	const char*			view_sampler_data_type_glsl =
		TextureViewUtilities::getGLSLDataTypeForSamplerType(view_sampler_type, view_n_components);
	const char* view_sampler_glsl = TextureViewUtilities::getGLSLTypeForSamplerType(view_sampler_type);
	const char* view_swizzle_glsl =
		(view_n_components == 4) ? "xyzw" : (view_n_components == 3) ? "xyz" : (view_n_components == 2) ? "xy" : "x";

	/* Form vertex shader body */
	const char* token_texture_data_type = "TEXTURE_DATA_TYPE";
	const char* token_texture_sampler   = "TEXTURE_SAMPLER";
	const char* token_texture_swizzle   = "TEXTURE_SWIZZLE";
	const char* token_view_data_type	= "VIEW_DATA_TYPE";
	const char* token_view_sampler		= "VIEW_SAMPLER";
	const char* token_view_swizzle		= "VIEW_SWIZZLE";
	const char* vs_template_body		= "#version 400\n"
								   "\n"
								   "uniform TEXTURE_SAMPLER texture;\n"
								   "uniform VIEW_SAMPLER    view;\n"
								   "\n"
								   "out TEXTURE_DATA_TYPE out_texture_data;\n"
								   "out VIEW_DATA_TYPE    out_view_data;\n"
								   "\n"
								   "void main()\n"
								   "{\n"
								   "    ivec2 uv = ivec2(gl_VertexID % 4,\n"
								   "                     gl_VertexID / 4);\n"
								   "\n"
								   "    out_texture_data = texelFetch(texture, uv, 0).TEXTURE_SWIZZLE;\n"
								   "    out_view_data    = texelFetch(view,    uv, 0).VIEW_SWIZZLE;\n"
								   "}\n";

	std::size_t token_position = std::string::npos;
	std::string vs_body		   = vs_template_body;

	while ((token_position = vs_body.find(token_texture_data_type)) != std::string::npos)
	{
		vs_body.replace(token_position, strlen(token_texture_data_type), texture_sampler_data_type_glsl);
	}

	while ((token_position = vs_body.find(token_texture_sampler)) != std::string::npos)
	{
		vs_body.replace(token_position, strlen(token_texture_sampler), texture_sampler_glsl);
	}

	while ((token_position = vs_body.find(token_texture_swizzle)) != std::string::npos)
	{
		vs_body.replace(token_position, strlen(token_texture_swizzle), texture_swizzle_glsl);
	}

	while ((token_position = vs_body.find(token_view_data_type)) != std::string::npos)
	{
		vs_body.replace(token_position, strlen(token_view_data_type), view_sampler_data_type_glsl);
	}

	while ((token_position = vs_body.find(token_view_sampler)) != std::string::npos)
	{
		vs_body.replace(token_position, strlen(token_view_sampler), view_sampler_glsl);
	}

	while ((token_position = vs_body.find(token_view_swizzle)) != std::string::npos)
	{
		vs_body.replace(token_position, strlen(token_view_swizzle), view_swizzle_glsl);
	}

	/* Compile the shader */
	glw::GLint  compile_status  = GL_FALSE;
	const char* vs_body_raw_ptr = vs_body.c_str();

	gl.shaderSource(m_vs_id, 1 /* count */, &vs_body_raw_ptr, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	gl.compileShader(m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

	gl.getShaderiv(m_vs_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

	if (compile_status != GL_TRUE)
	{
		TCU_FAIL("Shader compilation failed");
	}

	/* Configure test program object for XFB */
	const char*		   varying_names[] = { "out_texture_data", "out_view_data" };
	const unsigned int n_varying_names = sizeof(varying_names) / sizeof(varying_names[0]);

	gl.transformFeedbackVaryings(m_po_id, n_varying_names, varying_names, GL_SEPARATE_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed.");

	/* Configure buffer object bindings for XFB */
	gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* index for 'out_texture_data' */
					   m_bo_id, 0,						/* offset */
					   m_view_data_offset);				/* size */
	gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 1, /* index for 'out_view_data' */
					   m_bo_id, m_view_data_offset,		/* offset */
					   m_bo_size - m_view_data_offset); /* size */

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferRange() call(s) failed.");

	/* Link the program object */
	glw::GLint link_status = GL_FALSE;

	gl.linkProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed.");
	}

	/* Configure sampler uniforms */
	glw::GLint texture_uniform_location = gl.getUniformLocation(m_po_id, "texture");
	glw::GLint view_uniform_location	= gl.getUniformLocation(m_po_id, "view");

	if (texture_uniform_location == -1)
	{
		TCU_FAIL("'texture' uniform is considered inactive which is invalid");
	}

	if (view_uniform_location == -1)
	{
		TCU_FAIL("'view' uniform is considered inactive which is invalid");
	}

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	gl.uniform1i(texture_uniform_location, m_texture_unit_for_parent_texture - GL_TEXTURE0);
	gl.uniform1i(view_uniform_location, m_texture_unit_for_view_texture - GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() call(s) failed.");
}

/** Creates GL objects required to run the test, as well as modifies GL
 *  configuration (pixel pack/unpack settings, enables 'rasterizer discard' mode)
 *  in order for the test to run properly.
 **/
void TextureViewTestViewClasses::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate objects that will be used by all test iterations.
	 *
	 * Note that we're not generating texture objects here. This is owing to the fact
	 * we'll be using immutable textures and it makes more sense to generate the objects
	 * in initTexture() instead.
	 **/
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

	/* Attach the vertex shader to the program object */
	gl.attachShader(m_po_id, m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");

	/* Configure general buffer object binding. Indiced bindings will be configured
	 * in initProgramObject()
	 */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed.");

	/* Bind the VAO */
	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	/* Modify pack & unpack alignment settings */
	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei() call(s) failed.");

	/* Disable rasterizatino */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) call failed.");
}

/** Generates and initializes storage of either the parent texture object or the
 *  texture view, using user-specified internalformat.
 *
 *  @param should_init_parent_texture true to initialize parent texture object storage,
 *                                    false to configure texture view.
 *  @param internalformat             Internalformat to use for the process.
 *  @param viewformat                 Internalformat that will be used by "view" texture.
 *
 **/
void TextureViewTestViewClasses::initTextureObject(bool should_init_parent_texture, glw::GLenum texture_internalformat,
												   glw::GLenum view_internalformat)
{
	glw::GLenum			  cached_view_internalformat = view_internalformat;
	const glw::Functions& gl						 = m_context.getRenderContext().getFunctions();
	glw::GLuint*		  to_id_ptr					 = (should_init_parent_texture) ? &m_to_id : &m_view_to_id;

	/* If the object we're about to initialize has already been created, delete it first. */
	if (*to_id_ptr != 0)
	{
		gl.deleteTextures(1, to_id_ptr);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() call failed.");

		*to_id_ptr = 0;
	}

	/* Generate a new texture object */
	gl.genTextures(1, to_id_ptr);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	/* Initialize the object */
	if (should_init_parent_texture)
	{
		gl.bindTexture(GL_TEXTURE_2D, *to_id_ptr);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

		gl.texStorage2D(GL_TEXTURE_2D, 1, /* levels */
						texture_internalformat, m_texture_width, m_texture_height);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");
	}
	else
	{
		gl.textureView(m_view_to_id, GL_TEXTURE_2D, m_to_id, view_internalformat, 0, /* minlevel */
					   1,															 /* numlevels */
					   0,															 /* minlayer */
					   1);															 /* numlayers */

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureView() call failed.");

		gl.bindTexture(GL_TEXTURE_2D, m_view_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");
	}

	if (should_init_parent_texture)
	{
		/* We need to fill the base mip-map with actual contents. Calculate how much
		 * data we will need to fill a 4x4 mip-map, given the requested internalformat.
		 */
		bool is_internalformat_compressed = TextureViewUtilities::isInternalformatCompressed(texture_internalformat);
		glw::GLenum internalformat_to_use = GL_NONE;

		if (is_internalformat_compressed)
		{
			/* In order to initialize texture objects defined with a compressed internalformat
			 * using raw decompressed data, we need to override caller-specified internalformat
			 * with an internalformat that will describe decompressed data. The data will then
			 * be converted by GL to the compressed internalformat that was used for the previous
			 * glTexStorage2D() call.
			 **/
			_format format = TextureViewUtilities::getFormatOfInternalformat(texture_internalformat);

			if (format == FORMAT_FLOAT)
			{
				internalformat_to_use = GL_RGBA32F;
			}
			else
			{
				DE_ASSERT(format == FORMAT_UNORM || format == FORMAT_SNORM);

				internalformat_to_use = GL_RGBA8;
			}

			view_internalformat = internalformat_to_use;
		}
		else
		{
			internalformat_to_use = texture_internalformat;
		}

		/* Allocate the buffer.
		 *
		 * NOTE: This buffer is used in verifyResults(). When no longer needed, it will either
		 *       be deallocated there or in deinit().
		 **/
		glw::GLenum format_to_use   = TextureViewUtilities::getGLFormatOfInternalformat(internalformat_to_use);
		glw::GLenum type_to_use		= TextureViewUtilities::getTypeCompatibleWithInternalformat(internalformat_to_use);
		const glw::GLenum view_type = TextureViewUtilities::getTypeCompatibleWithInternalformat(view_internalformat);

		/* For some internalformats, we need to use a special data type in order to avoid
		 * implicit data conversion during glTexSubImage2D() call.
		 */
		switch (texture_internalformat)
		{
		case GL_R11F_G11F_B10F:
			type_to_use = GL_UNSIGNED_INT_10F_11F_11F_REV;
			break;
		case GL_RGB9_E5:
			type_to_use = GL_UNSIGNED_INT_5_9_9_9_REV;
			break;
		case GL_RGB10_A2:
			type_to_use = GL_UNSIGNED_INT_2_10_10_10_REV;
			break;

			/* Fall-through for other internalformats! */
		} /* switch (texture_internalformat) */

		/* Carry on */
		const unsigned int mipmap_raw_size = TextureViewUtilities::getTextureDataSize(
			internalformat_to_use, type_to_use, m_texture_width, m_texture_height);

		if (m_mipmap_data != NULL)
		{
			delete[] m_mipmap_data;

			m_mipmap_data = NULL;
		}

		m_mipmap_data = new unsigned char[mipmap_raw_size];

		/* Prepare data for texture */
		memset(m_mipmap_data, 0, mipmap_raw_size);

		switch (view_type)
		{
		case GL_BYTE:
		{
			glw::GLbyte*	  buffer			 = (glw::GLbyte*)m_mipmap_data;
			const glw::GLuint n_total_components = mipmap_raw_size / 1;

			for (glw::GLuint i = 0; i < n_total_components; ++i)
			{
				buffer[i] = static_cast<glw::GLbyte>(i - 128);
			}

			break;
		}

		case GL_SHORT:
		{
			glw::GLshort*	 buffer			 = (glw::GLshort*)m_mipmap_data;
			const glw::GLuint n_total_components = mipmap_raw_size / 2;

			for (glw::GLuint i = 0; i < n_total_components; ++i)
			{
				buffer[i] = static_cast<glw::GLshort>(i - 0xC000); // 0xC000 = (fp16) -2 makes this non de-norm
			}

			break;
		}

		case GL_INT:
		{
			glw::GLint*		  buffer			 = (glw::GLint*)m_mipmap_data;
			const glw::GLuint n_total_components = mipmap_raw_size / 4;

			for (glw::GLuint i = 0; i < n_total_components; ++i)
			{
				buffer[i] = i - 128;
			}

			break;
		}

		case GL_UNSIGNED_BYTE:
		{
			glw::GLubyte*	 buffer			 = (glw::GLubyte*)m_mipmap_data;
			const glw::GLuint n_total_components = mipmap_raw_size / 1;

			for (glw::GLuint i = 0; i < n_total_components; ++i)
			{
				buffer[i] = static_cast<glw::GLubyte>(i);
			}

			break;
		}

		case GL_UNSIGNED_SHORT:
		{
			glw::GLushort*	buffer			 = (glw::GLushort*)m_mipmap_data;
			const glw::GLuint n_total_components = mipmap_raw_size / 2;

			for (glw::GLuint i = 0; i < n_total_components; ++i)
			{
				buffer[i] = static_cast<glw::GLushort>(i);
			}

			break;
		}

		case GL_UNSIGNED_INT:
		{
			glw::GLuint*	  buffer			 = (glw::GLuint*)m_mipmap_data;
			const glw::GLuint n_total_components = mipmap_raw_size / 4;

			for (glw::GLuint i = 0; i < n_total_components; ++i)
			{
				buffer[i] = i;
			}

			break;
		}

		case GL_UNSIGNED_INT_24_8:
		{
			glw::GLuint*	  buffer			 = (glw::GLuint*)m_mipmap_data;
			const glw::GLuint n_total_components = mipmap_raw_size / 4;

			for (glw::GLuint i = 0; i < n_total_components; ++i)
			{
				buffer[i] = (i << 8) | (0xaa);
			}

			break;
		}

		case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
		{
			glw::GLfloat*	 float_buffer		 = (glw::GLfloat*)m_mipmap_data;
			const glw::GLuint n_total_components = mipmap_raw_size / 8;
			glw::GLuint*	  uint_buffer		 = (glw::GLuint*)(m_mipmap_data + 4);

			for (glw::GLuint i = 0; i < n_total_components; ++i)
			{
				float_buffer[i * 2] = (float)i - 128;
				uint_buffer[i * 2]  = (i << 8) | (0xaa);
			}

			break;
		}

		case GL_HALF_FLOAT:
		{
			tcu::Float16*	 buffer			 = (tcu::Float16*)m_mipmap_data;
			const glw::GLuint n_total_components = mipmap_raw_size / 2;

			for (glw::GLuint i = 0; i < n_total_components; ++i)
			{
				float value = (float)i - 128;

				buffer[i] = (tcu::Float16)value;
			}

			break;
		}

		case GL_FLOAT:
		{
			glw::GLfloat*	 float_buffer		 = (glw::GLfloat*)m_mipmap_data;
			const glw::GLuint n_total_components = mipmap_raw_size / 4;

			float offset = (cached_view_internalformat == GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT) ? 0.0f : -128.0f;
			for (glw::GLuint i = 0; i < n_total_components; ++i)
			{
				float_buffer[i] = (float)i + offset;
			}

			break;
		}

		case GL_UNSIGNED_INT_2_10_10_10_REV:
		{
			glw::GLuint*	  buffer			 = (glw::GLuint*)m_mipmap_data;
			const glw::GLuint n_total_components = mipmap_raw_size / 4;

			for (glw::GLuint i = 0; i < n_total_components; ++i)
			{
				buffer[i] = i | (i << 8) | (i << 16);
			}

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized texture view type");
		}
		} /* switch (view_type) */

		/* BPTC_FLOAT view class is a special case that needs an extra step. Signed and
		 * unsigned internal formats use different encodings, so instead of passing
		 * "regular" 32-bit floating-point data, we need to convert the values we initialized
		 * above to an actual BPTC representation. Since the encodings differ, we should
		 * compress these values using the internalformat that we will be later using for the
		 * texture view.
		 */
		unsigned int imageSize_to_use					= 0;
		bool		 use_glCompressedTexSubImage2D_call = false;

		if ((cached_view_internalformat == GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT ||
			 cached_view_internalformat == GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT) &&
			cached_view_internalformat != texture_internalformat)
		{
			/* Create a temporary texture object we'll use to compress the floating-point data. */
			gl.genTextures(1, &m_to_temp_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

			gl.bindTexture(GL_TEXTURE_2D, m_to_temp_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

			/* Initialize compressed storage */
			gl.texStorage2D(GL_TEXTURE_2D, 1, /* levels */
							cached_view_internalformat, m_texture_width, m_texture_height);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

			/* Submit floating-point decompressed data */
			gl.texSubImage2D(GL_TEXTURE_2D, 0, /* level */
							 0,				   /* xoffset */
							 0,				   /* yoffset */
							 m_texture_width, m_texture_height, GL_RGB, GL_FLOAT, m_mipmap_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage2D() call failed.");

			/* Extract the compressed version */
			gl.getCompressedTexImage(GL_TEXTURE_2D, 0, /* level */
									 m_mipmap_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetCompressedTexImage() call failed.");

			/* Delete the temporary texture object */
			gl.deleteTextures(1, &m_to_temp_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() call failed.");

			m_to_temp_id = 0;

			/* Revert to previous 2D texture binding */
			gl.bindTexture(GL_TEXTURE_2D, m_to_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

			/* Make sure upcoming glCompressedTexSubImage2D() call is made with sensible arguments */
			imageSize_to_use = (unsigned int)ceil((float)m_texture_width / 4.0f) *
							   (unsigned int)ceil((float)m_texture_height / 4.0f) * 16; /* block size */
			use_glCompressedTexSubImage2D_call = true;
		}

		/* Fill the mip-map with data */
		if (use_glCompressedTexSubImage2D_call)
		{
			gl.compressedTexSubImage2D(GL_TEXTURE_2D, 0, /* level */
									   0,				 /* xoffset */
									   0,				 /* yoffset */
									   m_texture_width, m_texture_height, texture_internalformat, imageSize_to_use,
									   m_mipmap_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompressedTexSubImage2D() call failed.");
		}
		else
		{
			gl.texSubImage2D(GL_TEXTURE_2D, 0, /* level */
							 0,				   /* xoffset */
							 0,				   /* yoffset */
							 m_texture_width, m_texture_height, format_to_use, type_to_use, m_mipmap_data);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage2D() call failed.");
		}
	}

	/* Make sure the texture object is complete */
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
					 GL_NEAREST); /* we're using texelFetch() so no mipmaps needed */
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri() call(s) failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult TextureViewTestViewClasses::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Only execute the test if GL_ARB_texture_view is supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_texture_view"))
	{
		throw tcu::NotSupportedError("GL_ARB_texture_view is not supported, skipping.");
	}

	/* Create all GL objects required to run the test */
	initTest();

	/* Iterate through all valid "texture internalformat + view internalformat" combinations */
	TextureViewUtilities::_compatible_internalformat_pairs_const_iterator internalformat_combination_iterator;
	TextureViewUtilities::_compatible_internalformat_pairs				  internalformat_combinations =
		TextureViewUtilities::getLegalTextureAndViewInternalformatCombinations();

	for (internalformat_combination_iterator = internalformat_combinations.begin();
		 internalformat_combination_iterator != internalformat_combinations.end();
		 internalformat_combination_iterator++)
	{
		TextureViewUtilities::_internalformat_pair internalformat_pair	= *internalformat_combination_iterator;
		glw::GLenum								   texture_internalformat = internalformat_pair.first;
		glw::GLenum								   view_internalformat	= internalformat_pair.second;

		/* Initialize parent texture object storage */
		initTextureObject(true, /* should_init_parent_texture */
						  texture_internalformat, view_internalformat);

		/* Create the texture view */
		initTextureObject(false, /* should_init_parent_texture */
						  texture_internalformat, view_internalformat);

		/* Initialize buffer object storage so that it's large enough to
		 * hold the result data.
		 **/
		initBufferObject(texture_internalformat, view_internalformat);

		/* Create the program object we'll use for the test */
		initProgramObject(texture_internalformat, view_internalformat);

		/* Configure texture bindings */
		gl.activeTexture(m_texture_unit_for_parent_texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture() call failed.");

		gl.bindTexture(GL_TEXTURE_2D, m_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

		gl.activeTexture(m_texture_unit_for_view_texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture() call failed.");

		gl.bindTexture(GL_TEXTURE_2D, m_view_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

		/* Run the test program */
		gl.beginTransformFeedback(GL_POINTS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed.");
		{
			gl.drawArrays(GL_POINTS, 0 /* first */, m_texture_width * m_texture_height);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");
		}
		gl.endTransformFeedback();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed.");

		/* Retrieve the results */
		const unsigned char* result_bo_data_ptr =
			(const unsigned char*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
		const unsigned char* result_texture_data_ptr = result_bo_data_ptr;
		const unsigned char* result_view_data_ptr	= result_bo_data_ptr + m_view_data_offset;

		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer() call failed.");

		/* Verify the retrieved values are valid */
		verifyResultData(texture_internalformat, view_internalformat, result_texture_data_ptr, result_view_data_ptr);

		/* Unmap the buffer object */
		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");
	} /* for (all internalformat combinations) */

	if (m_has_test_failed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	return STOP;
}

/** Verifies the data XFBed out by the test's vertex shader is valid.
 *
 *  @param texture_internalformat Internalformat that was used to define storage
 *                                of the parent texture object.
 *  @param view_internalformat    Internalformat that was used to define texture
 *                                view.
 *  @param texture_data_ptr       Data, as XFBed out by the vertex shader, that was
 *                                built by texelFetch() calls operating on the parent
 *                                texture object.
 *  @param view_data_ptr          Data, as XFBed out by the vertex shader, that was
 *                                built by texelFetch() calls operating on the texture
 *                                view.
 *
 **/
void TextureViewTestViewClasses::verifyResultData(glw::GLenum texture_internalformat, glw::GLenum view_internalformat,
												  const unsigned char* texture_data_ptr,
												  const unsigned char* view_data_ptr)
{
	const char* texture_internalformat_string = TextureViewUtilities::getInternalformatString(texture_internalformat);
	const char* view_internalformat_string	= TextureViewUtilities::getInternalformatString(view_internalformat);

	/* For quite a number of cases, we can do a plain memcmp() applied to sampled texture/view data.
	 * If both buffers are a match, we're OK.
	 */
	bool				 has_failed  = false;
	const unsigned char* mipmap_data = DE_NULL;

	if (memcmp(texture_data_ptr, view_data_ptr, m_view_data_offset) != 0)
	{
		/* Iterate over all texel components.
		 *
		 * The approach we're taking here works as follows:
		 *
		 * 1) Calculate what values should be sampled for each component using input mipmap
		 *    data.
		 * 2) Compare the reference values against the values returned when sampling the view.
		 *
		 * Note that in step 2) we're dealing with data that is returned by float/int/uint samplers,
		 * so we need to additionally process the data that we obtain by "casting" input data to
		 * the view's internalformat before we can perform the comparison.
		 *
		 * Finally, if the reference values are calculated for compressed data, we decompress it
		 * to GL_R8/GL_RG8/GL_RGB8/GL_RGBA8 internalformat first, depending on how many components
		 * the compressed internalformat supports.
		 **/
		bool				  can_continue = true;
		const glw::Functions& gl		   = m_context.getRenderContext().getFunctions();

		/* Determine a few important properties first */
		const bool is_view_internalformat_compressed =
			TextureViewUtilities::isInternalformatCompressed(view_internalformat);
		unsigned int n_bits_per_view_texel = 0;

		const unsigned int n_view_components =
			TextureViewUtilities::getAmountOfComponentsForInternalformat(view_internalformat);
		_format texture_format = TextureViewUtilities::getFormatOfInternalformat(texture_internalformat);

		unsigned int view_component_sizes[4] = { 0 };
		_format		 view_format			 = TextureViewUtilities::getFormatOfInternalformat(view_internalformat);

		if (!is_view_internalformat_compressed)
		{
			TextureViewUtilities::getComponentSizeForInternalformat(view_internalformat, view_component_sizes);

			n_bits_per_view_texel =
				view_component_sizes[0] + view_component_sizes[1] + view_component_sizes[2] + view_component_sizes[3];
		}
		else
		{
			if (texture_internalformat == GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT ||
				texture_internalformat == GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT)
			{
				/* Each component of decompressed data will be retrieved as a 32-bit FP */
				for (unsigned int n_component = 0; n_component < n_view_components; ++n_component)
				{
					view_component_sizes[n_component] = 32 /* bits per byte */;
				}

				n_bits_per_view_texel = 32 /* bits per byte */ * n_view_components;
			}
			else
			{
				/* Each component of decompressed data is stored as either signed or unsigned
				 * byte. */
				for (unsigned int n_component = 0; n_component < n_view_components; ++n_component)
				{
					view_component_sizes[n_component] = 8 /* bits per byte */;
				}

				n_bits_per_view_texel = 8 /* bits per byte */ * n_view_components;
			}
		}

		/* If we need to use compressed data as reference, we need to ask GL to decompress
		 * the mipmap data using view-specific internalformat.
		 */
		mipmap_data = m_mipmap_data;

		if (is_view_internalformat_compressed)
		{
			/* Deallocate the buffer if necessary just in case */
			if (m_decompressed_mipmap_data != DE_NULL)
			{
				delete[] m_decompressed_mipmap_data;

				m_decompressed_mipmap_data = DE_NULL;
			}

			m_decompressed_mipmap_data =
				new unsigned char[m_texture_width * m_texture_height * (n_bits_per_view_texel >> 3)];

			glw::GLuint reference_tex_id = m_to_id;
			if (texture_internalformat == GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT ||
				texture_internalformat == GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT)
			{
				// Encodings of SIGNED and UNSIGNED BPTC compressed texture are not compatible
				// even though they are in the same view class. Since the "view" texture contains
				// the correct encoding for the results we use that as a reference instead of the
				// incompatible parent encoded.
				reference_tex_id = m_view_to_id;
			}
			gl.bindTexture(GL_TEXTURE_2D, reference_tex_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

			gl.getTexImage(GL_TEXTURE_2D, 0, /* level */
						   (n_view_components == 4) ?
							   GL_RGBA :
							   (n_view_components == 3) ? GL_RGB : (n_view_components == 2) ? GL_RG : GL_RED,
						   (texture_format == FORMAT_SNORM) ?
							   GL_BYTE :
							   (texture_format == FORMAT_FLOAT) ? GL_FLOAT : GL_UNSIGNED_BYTE,
						   m_decompressed_mipmap_data);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexImage() call failed.");

			mipmap_data = m_decompressed_mipmap_data;
		}

		for (unsigned int n_texel = 0; n_texel < m_texture_height * m_texture_width && can_continue; ++n_texel)
		{
			/* NOTE: Vertex shader stores the sampled contents of a view texture as a
			 *       vec4/ivec4/uvec4. This means that each comonent in view_data_ptr
			 *       always takes sizeof(float) = sizeof(int) = sizeof(uint) bytes.
			 *
			 * NOTE: We cast input mip-map's data to view's internalformat, which is
			 *       why we're assuming each components takes n_bits_per_view_texel
			 *       bits instead of n_bits_per_mipmap_texel.
			 */
			const unsigned char* mipmap_texel_data =
				mipmap_data + (n_bits_per_view_texel >> 3 /* 8 bits/byte */) * n_texel;
			float		  reference_components_float[4] = { 0 };
			signed int	reference_components_int[4]   = { 0 };
			unsigned int  reference_components_uint[4]  = { 0 };
			float		  view_components_float[4]		= { 0 };
			signed int	view_components_int[4]		= { 0 };
			unsigned int  view_components_uint[4]		= { 0 };
			_sampler_type view_sampler_type =
				TextureViewUtilities::getSamplerTypeForInternalformat(view_internalformat);
			const unsigned char* view_texel_data = view_data_ptr + sizeof(float) * n_view_components * n_texel;

			/* Retrieve data sampled from the view */
			for (unsigned int n_component = 0; n_component < n_view_components;
				 view_texel_data += sizeof(float), /* as per comment */
				 ++n_component)
			{
				switch (view_sampler_type)
				{
				case SAMPLER_TYPE_FLOAT:
				{
					view_components_float[n_component] = *((float*)view_texel_data);

					break;
				}

				case SAMPLER_TYPE_SIGNED_INTEGER:
				{
					view_components_int[n_component] = *((signed int*)view_texel_data);

					break;
				}

				case SAMPLER_TYPE_UNSIGNED_INTEGER:
				{
					view_components_uint[n_component] = *((unsigned int*)view_texel_data);

					break;
				}

				default:
				{
					TCU_FAIL("Unrecognized sampler type");
				}
				} /* switch (view_sampler_type) */
			}	 /* for (all components) */

			/* Compute reference data. Handle non-byte aligned internalformats manually. */
			if (view_internalformat == GL_R11F_G11F_B10F)
			{
				const unsigned int* reference_data  = (unsigned int*)mipmap_texel_data;
				const unsigned int  red_component   = (*reference_data) & ((1 << 11) - 1);
				const unsigned int  green_component = (*reference_data >> 11) & ((1 << 11) - 1);
				const unsigned int  blue_component  = (*reference_data >> 22) & ((1 << 10) - 1);

				if (view_sampler_type == SAMPLER_TYPE_FLOAT)
				{
					reference_components_float[0] = Float11(red_component).asFloat();
					reference_components_float[1] = Float11(green_component).asFloat();
					reference_components_float[2] = Float10(blue_component).asFloat();
				}
				else
				{
					TCU_FAIL("Internal error: invalid sampler type requested");
				}
			}
			else if (view_internalformat == GL_RGB9_E5)
			{
				/* Refactored version of tcuTexture.cpp::unpackRGB999E5() */
				const unsigned int* reference_data  = (unsigned int*)mipmap_texel_data;
				const unsigned int  exponent		= (*reference_data >> 27) & ((1 << 5) - 1);
				const unsigned int  red_component   = (*reference_data) & ((1 << 9) - 1);
				const unsigned int  green_component = (*reference_data >> 9) & ((1 << 9) - 1);
				const unsigned int  blue_component  = (*reference_data >> 18) & ((1 << 9) - 1);

				float shared_exponent =
					deFloatPow(2.0f, (float)((int)exponent - 15 /* exponent bias */ - 9 /* mantissa */));

				if (view_sampler_type == SAMPLER_TYPE_FLOAT)
				{
					reference_components_float[0] = float(red_component) * shared_exponent;
					reference_components_float[1] = float(green_component) * shared_exponent;
					reference_components_float[2] = float(blue_component) * shared_exponent;
				}
				else
				{
					TCU_FAIL("Internal error: invalid sampler type requested");
				}
			}
			else if (view_internalformat == GL_RGB10_A2)
			{
				unsigned int*	  reference_data = (unsigned int*)mipmap_texel_data;
				const unsigned int mask_rgb		  = (1 << 10) - 1;
				const unsigned int mask_a		  = (1 << 2) - 1;

				if (view_sampler_type == SAMPLER_TYPE_FLOAT)
				{
					reference_components_float[0] = float(((*reference_data)) & (mask_rgb)) / float(mask_rgb);
					reference_components_float[1] = float(((*reference_data) >> 10) & (mask_rgb)) / float(mask_rgb);
					reference_components_float[2] = float(((*reference_data) >> 20) & (mask_rgb)) / float(mask_rgb);
					reference_components_float[3] = float(((*reference_data) >> 30) & (mask_a)) / float(mask_a);
				}
				else
				{
					TCU_FAIL("Internal error: invalid sampler type requested");
				}
			}
			else if (view_internalformat == GL_RGB10_A2UI)
			{
				unsigned int*	  reference_data = (unsigned int*)mipmap_texel_data;
				const unsigned int mask_rgb		  = (1 << 10) - 1;
				const unsigned int mask_a		  = (1 << 2) - 1;

				if (view_sampler_type == SAMPLER_TYPE_UNSIGNED_INTEGER)
				{
					reference_components_uint[0] = ((*reference_data)) & (mask_rgb);
					reference_components_uint[1] = ((*reference_data) >> 10) & (mask_rgb);
					reference_components_uint[2] = ((*reference_data) >> 20) & (mask_rgb);
					reference_components_uint[3] = ((*reference_data) >> 30) & (mask_a);
				}
				else
				{
					TCU_FAIL("Internal error: invalid sampler type requested");
				}
			}
			else if (view_internalformat == GL_RG16F)
			{
				unsigned short* reference_data = (unsigned short*)mipmap_texel_data;

				if (view_sampler_type == SAMPLER_TYPE_FLOAT)
				{
					reference_components_float[0] = tcu::Float16(*(reference_data + 0)).asFloat();
					reference_components_float[1] = tcu::Float16(*(reference_data + 1)).asFloat();
				}
				else
				{
					TCU_FAIL("Internal error: invalid sampler type requested");
				}
			}
			else
			{
				void* result_data = NULL;

				switch (view_sampler_type)
				{
				case SAMPLER_TYPE_FLOAT:
					result_data = reference_components_float;
					break;
				case SAMPLER_TYPE_SIGNED_INTEGER:
					result_data = reference_components_int;
					break;
				case SAMPLER_TYPE_UNSIGNED_INTEGER:
					result_data = reference_components_uint;
					break;

				default:
					TCU_FAIL("Unrecognized sampler type");
				}

				getComponentDataForByteAlignedInternalformat(mipmap_texel_data, n_view_components, view_component_sizes,
															 view_format, result_data);
			}

			for (unsigned int n_component = 0; n_component < n_view_components; ++n_component)
			{
				/* If view texture operates on sRGB color space, we need to adjust our
				 * reference value so that it is moved back into linear space.
				 */
				if (TextureViewUtilities::isInternalformatSRGB(view_internalformat) &&
					!TextureViewUtilities::isInternalformatSRGB(texture_internalformat))
				{
					DE_ASSERT(view_sampler_type == SAMPLER_TYPE_FLOAT);

					/* Convert as per (8.14) from GL4.4 spec. Exclude alpha channel. */
					if (n_component != 3)
					{
						if (reference_components_float[n_component] <= 0.04045f)
						{
							reference_components_float[n_component] /= 12.92f;
						}
						else
						{
							reference_components_float[n_component] =
								deFloatPow((reference_components_float[n_component] + 0.055f) / 1.055f, 2.4f);
						}
					} /* if (n_component != 3) */
				}	 /* if (TextureViewUtilities::isInternalformatSRGB(view_internalformat) ) */

				/* Compare the reference and view texture values */
				const float		   epsilon_float = 1.0f / float((1 << (view_component_sizes[n_component] - 1)) - 1);
				const signed int   epsilon_int   = 1;
				const unsigned int epsilon_uint  = 1;

				switch (view_sampler_type)
				{
				case SAMPLER_TYPE_FLOAT:
				{
					if (de::abs(reference_components_float[n_component] - view_components_float[n_component]) >
						epsilon_float)
					{
						has_failed = true;
					}

					break;
				}

				case SAMPLER_TYPE_SIGNED_INTEGER:
				{
					signed int larger_value  = 0;
					signed int smaller_value = 0;

					if (reference_components_int[n_component] > view_components_int[n_component])
					{
						larger_value  = reference_components_int[n_component];
						smaller_value = view_components_int[n_component];
					}
					else
					{
						smaller_value = reference_components_int[n_component];
						larger_value  = view_components_int[n_component];
					}

					if ((larger_value - smaller_value) > epsilon_int)
					{
						has_failed = true;
					}

					break;
				}

				case SAMPLER_TYPE_UNSIGNED_INTEGER:
				{
					unsigned int larger_value  = 0;
					unsigned int smaller_value = 0;

					if (reference_components_uint[n_component] > view_components_uint[n_component])
					{
						larger_value  = reference_components_uint[n_component];
						smaller_value = view_components_uint[n_component];
					}
					else
					{
						smaller_value = reference_components_uint[n_component];
						larger_value  = view_components_uint[n_component];
					}

					if ((larger_value - smaller_value) > epsilon_uint)
					{
						has_failed = true;
					}

					break;
				}

				default:
					TCU_FAIL("Unrecognized sampler type");
				} /* switch (view_sampler_type) */

				if (has_failed)
				{
					can_continue = false;

					switch (view_sampler_type)
					{
					case SAMPLER_TYPE_FLOAT:
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Invalid data sampled from a texture view "
																	   "["
										   << view_internalformat_string << "]"
																			" created from a texture object"
																			"["
										   << texture_internalformat_string << "]"
																			   " at texel "
																			   "("
										   << (n_texel % m_texture_width) << ", " << (n_texel / m_texture_height)
										   << "): expected:(" << reference_components_float[0] << ", "
										   << reference_components_float[1] << ", " << reference_components_float[2]
										   << ", " << reference_components_float[3] << ") found:("
										   << view_components_float[0] << ", " << view_components_float[1] << ", "
										   << view_components_float[2] << ", " << view_components_float[3] << ")."
										   << tcu::TestLog::EndMessage;

						break;
					}

					case SAMPLER_TYPE_SIGNED_INTEGER:
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message << "Invalid data sampled from a signed integer texture view "
														"["
							<< view_internalformat_string << "]"
															 " created from a texture object"
															 "["
							<< texture_internalformat_string << "]"
																" at texel "
																"("
							<< (n_texel % m_texture_width) << ", " << (n_texel / m_texture_height) << "): expected:("
							<< reference_components_int[0] << ", " << reference_components_int[1] << ", "
							<< reference_components_int[2] << ", " << reference_components_int[3] << ") found:("
							<< view_components_int[0] << ", " << view_components_int[1] << ", "
							<< view_components_int[2] << ", " << view_components_int[3] << ")."
							<< tcu::TestLog::EndMessage;

						break;
					}

					case SAMPLER_TYPE_UNSIGNED_INTEGER:
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message << "Invalid data sampled from an unsigned integer texture view "
														"["
							<< view_internalformat_string << "]"
															 " created from a texture object"
															 "["
							<< texture_internalformat_string << "]"
																" at texel "
																"("
							<< (n_texel % m_texture_width) << ", " << (n_texel / m_texture_height) << "): expected:("
							<< reference_components_uint[0] << ", " << reference_components_uint[1] << ", "
							<< reference_components_uint[2] << ", " << reference_components_uint[3] << ") found:("
							<< view_components_uint[0] << ", " << view_components_uint[1] << ", "
							<< view_components_uint[2] << ", " << view_components_uint[3] << ")."
							<< tcu::TestLog::EndMessage;

						break;
					}

					default:
						TCU_FAIL("Unrecognized sampler type");
					} /* switch (view_sampler_type) */

					break;
				} /* if (has_failed) */
			}	 /* for (all components) */
		}		  /* for (all texels) */
	}			  /* if (memcmp(texture_data_ptr, view_data_ptr, m_view_data_offset) != 0) */

	if (has_failed)
	{
		/* Log detailed information about the failure */
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid data read from a view of internalformat "
						   << "[" << view_internalformat_string << "]"
						   << " created from a texture of internalformat "
						   << "[" << texture_internalformat_string << "]"
						   << ". Byte streams follow:" << tcu::TestLog::EndMessage;

		/* Form texture and view data strings */
		std::stringstream mipmap_data_sstream;
		std::stringstream sampled_view_data_sstream;

		mipmap_data_sstream.fill('0');
		sampled_view_data_sstream.fill('0');

		mipmap_data_sstream.width(2);
		sampled_view_data_sstream.width(2);

		mipmap_data_sstream << "Mip-map data: [";
		sampled_view_data_sstream << "Sampled view data: [";

		for (unsigned int n = 0; n < m_view_data_offset; ++n)
		{
			mipmap_data_sstream << "0x" << std::hex << (int)(mipmap_data[n]);
			sampled_view_data_sstream << "0x" << std::hex << (int)(view_data_ptr[n]);

			if (n != (m_view_data_offset - 1))
			{
				mipmap_data_sstream << "|";
				sampled_view_data_sstream << "|";
			}
			else
			{
				mipmap_data_sstream << "]";
				sampled_view_data_sstream << "]";
			}
		}

		sampled_view_data_sstream << "\n";
		mipmap_data_sstream << "\n";

		/* Log both strings */
		m_testCtx.getLog() << tcu::TestLog::Message << mipmap_data_sstream.str() << sampled_view_data_sstream.str()
						   << tcu::TestLog::EndMessage;

		/* Do not fail the test at this point. Instead, raise a failure flag that will
		 * cause the test to fail once all iterations execute */
		m_has_test_failed = true;
	}
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Correct data read from a view of internalformat "
						   << "[" << view_internalformat_string << "]"
						   << " created from a texture of internalformat "
						   << "[" << texture_internalformat_string << "]" << tcu::TestLog::EndMessage;
	}
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
TextureViewTestCoherency::TextureViewTestCoherency(deqp::Context& context)
	: TestCase(context, "coherency", "Verifies view/parent texture coherency")
	, m_are_images_supported(false)
	, m_bo_id(0)
	, m_draw_fbo_id(0)
	, m_gradient_verification_po_id(0)
	, m_gradient_verification_po_sample_exact_uv_location(-1)
	, m_gradient_verification_po_lod_location(-1)
	, m_gradient_verification_po_texture_location(-1)
	, m_gradient_verification_vs_id(0)
	, m_gradient_image_write_image_size_location(-1)
	, m_gradient_image_write_po_id(0)
	, m_gradient_image_write_vs_id(0)
	, m_gradient_write_po_id(0)
	, m_gradient_write_fs_id(0)
	, m_gradient_write_vs_id(0)
	, m_read_fbo_id(0)
	, m_static_to_id(0)
	, m_to_id(0)
	, m_vao_id(0)
	, m_view_to_id(0)
	, m_verification_po_expected_color_location(-1)
	, m_verification_po_lod_location(-1)
	, m_verification_po_id(0)
	, m_verification_vs_id(0)
	, m_static_texture_height(1)
	, m_static_texture_width(1)
	, m_texture_height(64)
	, m_texture_n_components(4)
	, m_texture_n_levels(7)
	, m_texture_width(64)
{
	/* Initialize static color that will be used for some of the cases */
	m_static_color_byte[0] = 100;
	m_static_color_byte[1] = 0;
	m_static_color_byte[2] = 255;
	m_static_color_byte[3] = 200;

	m_static_color_float[0] = float(m_static_color_byte[0]) / 255.0f;
	m_static_color_float[1] = float(m_static_color_byte[1]) / 255.0f;
	m_static_color_float[2] = float(m_static_color_byte[2]) / 255.0f;
	m_static_color_float[3] = float(m_static_color_byte[3]) / 255.0f;
}

/** Verifies that texture/view & view/texture coherency requirement is met
 *  when glTexSubImage2D() or glBlitFramebuffer() API calls are used to modify
 *  the contents of one of the mip-maps. The function does not use any memory
 *  barriers as these are not required for the objects to stay synchronised.
 *
 *  Throws TestError  exceptionif the GL implementation fails the check.
 *
 *  @param texture_type               Defines whether it should be parent texture or
 *                                    its view that the writing operation should be
 *                                    performed against. The reading operation will
 *                                    be issued against the sibling object.
 *  @param should_use_glTexSubImage2D true if glTexSubImage2D() should be used for the
 *                                    check, false to use glBlitFramebuffer().
 *
 **/
void TextureViewTestCoherency::checkAPICallCoherency(_texture_type texture_type, bool should_use_glTexSubImage2D)
{
	const glw::Functions& gl			  = m_context.getRenderContext().getFunctions();
	unsigned int		  write_to_height = 0;
	unsigned int		  write_to_width  = 0;
	glw::GLuint			  write_to_id	 = 0;

	getWritePropertiesForTextureType(texture_type, &write_to_id, &write_to_width, &write_to_height);

	if (should_use_glTexSubImage2D)
	{
		/* Update texture binding for texture unit 0, given the texture type the caller wants
		 * us to test. We'll need the binding set appropriately for the subsequent
		 * glTexSubImage2D() call.
		 */
		gl.activeTexture(GL_TEXTURE0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture() call failed.");

		gl.bindTexture(GL_TEXTURE_2D, write_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");
	}
	else
	{
		/* To perform a blit operation, we need to configure draw & read FBO, taking
		 * the tested texture type into account. */
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_draw_fbo_id);
		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_read_fbo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call(s) failed.");

		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, write_to_id, 1); /* level */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed for GL_DRAW_FRAMEBUFFER target.");

		gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_static_to_id,
								0); /* level */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed for GL_READ_FRAMEBUFFER target.");
	}

	/* Execute the API call */
	const unsigned int region_width  = (write_to_width >> 1);
	const unsigned int region_height = (write_to_height >> 1);
	const unsigned int region_x		 = region_width - (region_width >> 1);
	const unsigned int region_y		 = region_height - (region_height >> 1);

	if (should_use_glTexSubImage2D)
	{
		/* Call glTexSubImage2D() to replace a portion of the gradient with a static color */
		{
			unsigned char* static_color_data_ptr = getStaticColorTextureData(region_width, region_height);

			gl.texSubImage2D(GL_TEXTURE_2D, 1, /* level */
							 region_x, region_y, region_width, region_height, GL_RGBA, GL_UNSIGNED_BYTE,
							 static_color_data_ptr);

			/* Good to release static color data buffer at this point */
			delete[] static_color_data_ptr;

			static_color_data_ptr = DE_NULL;

			/* Make sure the API call was successful */
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage2D() call failed.");
		}
	}
	else
	{
		gl.blitFramebuffer(0,						/* srcX0 */
						   0,						/* srcY0 */
						   m_static_texture_width,  /* srcX1 */
						   m_static_texture_height, /* srcY1 */
						   region_x, region_y, region_x + region_width, region_y + region_height, GL_COLOR_BUFFER_BIT,
						   GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBlitFramebuffer() call failed.");
	}

	/* Bind the sibling object so that we can make sure the data read from the
	 * region can be correctly read from a shader without a memory barrier.
	 *
	 * While we're here, also determine which LOD we should be sampling from in
	 * the shader.
	 **/
	unsigned int read_lod   = 0;
	glw::GLuint  read_to_id = 0;

	getReadPropertiesForTextureType(texture_type, &read_to_id, &read_lod);

	gl.bindTexture(GL_TEXTURE_2D, read_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	/* Update the test program uniforms before we carry on with actual
	 * verification
	 */
	gl.useProgram(m_verification_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	DE_STATIC_ASSERT(sizeof(m_static_color_float) == sizeof(float) * 4);

	gl.uniform4fv(m_verification_po_expected_color_location, 1, /* count */
				  m_static_color_float);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform4fv() call failed.");

	gl.uniform1i(m_verification_po_lod_location, read_lod);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() call failed.");

	/* Make sure rasterization is disabled before we carry on */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable() call failed.");

	/* Go ahead with the rendering. Make sure to capture the varyings */
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed.");
	{
		gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");
	}
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed.");

	/* Map the buffer object so we can validate the sampling result */
	const glw::GLint* data_ptr = (const glw::GLint*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer() call failed.");
	DE_ASSERT(data_ptr != NULL);

	/* Verify the outcome of the sampling operation */
	if (*data_ptr != 1)
	{
		TCU_FAIL("Invalid data was sampled in vertex shader");
	}

	/* Unmap the buffer object */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

	data_ptr = DE_NULL;

	/* Disable GL_RASTERIZER_DISCARD mode */
	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable() call failed.");
}

/** Verifies texture/view & view/texture coherency is met when one of the objects
 *  is used as a render-target. The function writes to user-specified texture type,
 *  and then verifies the contents of the sibling object.
 *
 *  The function throws TestError exception if any of the checks fail.
 *
 *  @param texture_type      Tells which of the two objects should be written to.
 *  @param should_use_images true if images should be used for
 *  @param barrier_type      Type of the memory barrier that should be injected
 *                           after vertex shader stage with image writes is executed.
 *                           Must be BARRIER_TYPE_NONE if @param should_use_images
 *                           is set to false.
 *  @param verification_mean Determines whether the verification should be performed
 *                           using a program object, or by CPU with the data
 *                           extracted from the sibling object using a glGetTexImage()
 *                           call.
 *
 **/
void TextureViewTestCoherency::checkProgramWriteCoherency(_texture_type texture_type, bool should_use_images,
														  _barrier_type		 barrier_type,
														  _verification_mean verification_mean)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (!should_use_images)
	{
		/* Sanity check: no barrier should be requested if images are not used */
		DE_ASSERT(barrier_type == BARRIER_TYPE_NONE);

		/* Sanity check: glGetTexImage*() call should only be used for verification
		 *               when images are used */
		DE_ASSERT(verification_mean == VERIFICATION_MEAN_PROGRAM);
	}

	/* Determine GL id of an object we will be rendering the gradient to */
	glw::GLuint  write_to_id	 = 0;
	unsigned int write_to_width  = 0;
	unsigned int write_to_height = 0;

	getWritePropertiesForTextureType(texture_type, &write_to_id, &write_to_width, &write_to_height);

	/* Configure the render targets */
	if (should_use_images)
	{
		gl.bindImageTexture(0,				/* unit */
							write_to_id, 1, /* second level */
							GL_FALSE,		/* layered */
							0,				/* layer */
							GL_WRITE_ONLY, GL_RGBA8);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture() call failed.");
	}
	else
	{
		/* We first need to fill either the texture or its sibling view with
		 * gradient data. Set up draw framebuffer */
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_draw_fbo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed for GL_FRAMEBUFFER target");

		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, write_to_id, 1); /* level */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");

		/* Configure the viewport accordingly */
		gl.viewport(0, /* x */
					0, /* y */
					write_to_width, write_to_height);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() call failed.");
	}

	/* The gradient needs to be rendered differently, depending on whether
	 * we're asked to use images or not */
	if (should_use_images)
	{
		gl.useProgram(m_gradient_image_write_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

		gl.uniform2i(m_gradient_image_write_image_size_location, write_to_width, write_to_height);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform2i() call failed.");

		gl.enable(GL_RASTERIZER_DISCARD);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) call failed.");
		{
			gl.drawArrays(GL_POINTS, 0 /* first */, write_to_width * write_to_height);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");
		}
		gl.disable(GL_RASTERIZER_DISCARD);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable(GL_RASTERIZER_DISCARD) call failed.");

		/* If the caller requested any barriers, issue them at this point */
		switch (barrier_type)
		{
		case BARRIER_TYPE_TEXTURE_FETCH_BARRIER_BIT:
		{
			gl.memoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

			GLU_EXPECT_NO_ERROR(gl.getError(),
								"glMemoryBarrier() call failed for GL_TEXTURE_FETCH_BARRIER_BIT barrier");

			break;
		}

		case BARRIER_TYPE_TEXTURE_UPDATE_BUFFER_BIT:
		{
			gl.memoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

			GLU_EXPECT_NO_ERROR(gl.getError(),
								"glMemoryBarrier() call failed for GL_TEXTURE_UPDATE_BARRIER_BIT barrier");

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized barrier type");
		}
		} /* switch (barrier_type) */
	}	 /* if (should_use_images) */
	else
	{
		/* Render the gradient on a full-screen quad */
		gl.useProgram(m_gradient_write_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gluseProgram() call failed.");

		gl.drawArrays(GL_TRIANGLE_STRIP, 0 /* first */, 4 /* count */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");
	}

	/* Determine which texture and which mip-map level we will need to sample
	 * in order to verify whether the former operations have been completed
	 * successfully.
	 **/
	unsigned int read_lod   = 0;
	glw::GLuint  read_to_id = 0;

	getReadPropertiesForTextureType(texture_type, &read_to_id, &read_lod);

	/* Before we proceed with verification, update the texture binding so that
	 * the verification program can sample from the right texture */
	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture() call failed.");

	gl.bindTexture(GL_TEXTURE_2D, read_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	if (verification_mean == VERIFICATION_MEAN_PROGRAM)
	{
		/* Switch to a verification program. It uses a vertex shader to sample
		 * all texels of the texture so issue as many invocations as necessary. */
		unsigned int n_invocations = write_to_width * write_to_height;

		gl.useProgram(m_gradient_verification_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

		gl.uniform1i(m_gradient_verification_po_texture_location, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() call failed.");

		gl.uniform1i(m_gradient_verification_po_sample_exact_uv_location, should_use_images);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() call failed.");

		gl.uniform1f(m_gradient_verification_po_lod_location, (float)read_lod);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1f() call failed.");

		gl.enable(GL_RASTERIZER_DISCARD);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) call failed.");

		gl.beginTransformFeedback(GL_POINTS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed.");
		{
			gl.drawArrays(GL_POINTS, 0 /* first */, n_invocations);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");
		}
		gl.endTransformFeedback();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed.");

		gl.disable(GL_RASTERIZER_DISCARD);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable(GL_RASTERIZER_DISCARD) call failed.");

		/* Map the result buffer object storage into process space */
		const int* result_data_ptr = (const int*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer() call failed.");

		if (result_data_ptr == DE_NULL)
		{
			TCU_FAIL("glMapBuffer() did not generate an error but returned a NULL pointer");
		}

		/* Verify the XFBed data */
		for (unsigned int n_invocation = 0; n_invocation < n_invocations; ++n_invocation)
		{
			if (result_data_ptr[n_invocation] != 1)
			{
				unsigned int invocation_x = n_invocation % write_to_width;
				unsigned int invocation_y = n_invocation / write_to_width;

				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid data was sampled at "
								   << "(" << invocation_x << ", " << invocation_y << ") when sampling from "
								   << ((texture_type == TEXTURE_TYPE_PARENT_TEXTURE) ? "a texture" : "a view")
								   << tcu::TestLog::EndMessage;

				/* Make sure the buffer is unmapped before throwing the exception */
				gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);

				TCU_FAIL("Invalid data sampled");
			}
		} /* for (all invocations) */

		/* Unmap the buffer storage */
		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");
	} /* if (verification_mean == VERIFICATION_MEAN_PROGRAM) */
	else
	{
		DE_ASSERT(verification_mean == VERIFICATION_MEAN_GLGETTEXIMAGE);

		/* Allocate space for the data */
		unsigned char* data_ptr = new unsigned char[write_to_width * write_to_height * m_texture_n_components];

		/* Retrieve the rendered data */
		gl.getTexImage(GL_TEXTURE_2D, read_lod, GL_RGBA, GL_UNSIGNED_BYTE, data_ptr);

		if (gl.getError() != GL_NO_ERROR)
		{
			/* Release the buffer before we throw an exception */
			delete[] data_ptr;

			TCU_FAIL("glGetTexImage() call failed.");
		}

		/* Verify the data is correct */
		const int epsilon		  = 1;
		bool	  is_data_correct = true;

		for (unsigned int y = 0; y < write_to_height; ++y)
		{
			const unsigned char* row_ptr = data_ptr + y * m_texture_n_components * write_to_width;

			for (unsigned int x = 0; x < write_to_width; ++x)
			{
				const unsigned char* texel_ptr	= row_ptr + x * m_texture_n_components;
				const float			 end_rgba[]   = { 0.0f, 0.1f, 1.0f, 1.0f };
				const float			 lerp_factor  = float(x) / float(write_to_width);
				const float			 start_rgba[] = { 1.0f, 0.9f, 0.0f, 0.0f };
				const float expected_data_float[] = { start_rgba[0] * (1.0f - lerp_factor) + end_rgba[0] * lerp_factor,
													  start_rgba[1] * (1.0f - lerp_factor) + end_rgba[1] * lerp_factor,
													  start_rgba[2] * (1.0f - lerp_factor) + end_rgba[2] * lerp_factor,
													  start_rgba[3] * (1.0f - lerp_factor) +
														  end_rgba[3] * lerp_factor };
				const unsigned char expected_data_ubyte[] = { (unsigned char)(expected_data_float[0] * 255.0f),
															  (unsigned char)(expected_data_float[1] * 255.0f),
															  (unsigned char)(expected_data_float[2] * 255.0f),
															  (unsigned char)(expected_data_float[3] * 255.0f) };

				if (de::abs((int)texel_ptr[0] - (int)expected_data_ubyte[0]) > epsilon ||
					de::abs((int)texel_ptr[1] - (int)expected_data_ubyte[1]) > epsilon ||
					de::abs((int)texel_ptr[2] - (int)expected_data_ubyte[2]) > epsilon ||
					de::abs((int)texel_ptr[3] - (int)expected_data_ubyte[3]) > epsilon)
				{
					is_data_correct = false;

					break;
				}
			}
		} /* for (all rows) */

		/* Good to release the data buffer at this point */
		delete[] data_ptr;

		data_ptr = DE_NULL;

		/* Fail the test if any of the rendered texels were found invalid */
		if (!is_data_correct)
		{
			TCU_FAIL("Invalid data sampled");
		}
	}
}

/** Deinitializes all GL objects that may have been created during
 *  test execution.
 **/
void TextureViewTestCoherency::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release any GL objects the test may have created */
	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	if (m_draw_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_draw_fbo_id);

		m_draw_fbo_id = 0;
	}

	if (m_gradient_image_write_po_id != 0)
	{
		gl.deleteProgram(m_gradient_image_write_po_id);

		m_gradient_image_write_po_id = 0;
	}

	if (m_gradient_image_write_vs_id != 0)
	{
		gl.deleteShader(m_gradient_image_write_vs_id);

		m_gradient_image_write_vs_id = 0;
	}

	if (m_gradient_verification_po_id != 0)
	{
		gl.deleteProgram(m_gradient_verification_po_id);

		m_gradient_verification_po_id = 0;
	}

	if (m_gradient_verification_vs_id != 0)
	{
		gl.deleteShader(m_gradient_verification_vs_id);

		m_gradient_verification_vs_id = 0;
	}

	if (m_gradient_write_fs_id != 0)
	{
		gl.deleteShader(m_gradient_write_fs_id);

		m_gradient_write_fs_id = 0;
	}

	if (m_gradient_write_po_id != 0)
	{
		gl.deleteProgram(m_gradient_write_po_id);

		m_gradient_write_po_id = 0;
	}

	if (m_gradient_write_vs_id != 0)
	{
		gl.deleteShader(m_gradient_write_vs_id);

		m_gradient_write_vs_id = 0;
	}

	if (m_read_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_read_fbo_id);

		m_read_fbo_id = 0;
	}

	if (m_static_to_id != 0)
	{
		gl.deleteTextures(1, &m_static_to_id);

		m_static_to_id = 0;
	}

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);

		m_to_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_view_to_id != 0)
	{
		gl.deleteTextures(1, &m_view_to_id);

		m_view_to_id = 0;
	}

	if (m_verification_po_id != 0)
	{
		gl.deleteProgram(m_verification_po_id);

		m_verification_po_id = 0;
	}

	if (m_verification_vs_id != 0)
	{
		gl.deleteShader(m_verification_vs_id);

		m_verification_vs_id = 0;
	}

	/* Disable GL_RASTERIZER_DISCARD mode */
	gl.disable(GL_RASTERIZER_DISCARD);
}

/** Allocates a sufficiently large buffer for RGBA8 data and fills it with
 *  a horizontal gradient (as described in the test specification)
 *
 *  It is user's responsibility to release the buffer when no longer needed.
 *
 *  @return Pointer to the buffer.
 **/
unsigned char* TextureViewTestCoherency::getHorizontalGradientData() const
{
	const float		   end_rgba[]   = { 1.0f, 0.9f, 0.0f, 0.0f };
	unsigned char*	 result		= new unsigned char[m_texture_width * m_texture_height * m_texture_n_components];
	const float		   start_rgba[] = { 0.0f, 0.1f, 1.0f, 1.0f };
	const unsigned int texel_size   = m_texture_n_components;

	for (unsigned int y = 0; y < m_texture_height; ++y)
	{
		unsigned char* row_data_ptr = result + texel_size * m_texture_width * y;

		for (unsigned int x = 0; x < m_texture_width; ++x)
		{
			const float	lerp_factor	= float(x) / float(m_texture_width);
			unsigned char* pixel_data_ptr = row_data_ptr + texel_size * x;

			for (unsigned int n_component = 0; n_component < 4 /* rgba */; ++n_component)
			{
				pixel_data_ptr[n_component] = (unsigned char)((end_rgba[n_component] * lerp_factor +
															   start_rgba[n_component] * (1.0f - lerp_factor)) *
															  255.0f);
			} /* for (all components) */
		}	 /* for (all columns) */
	}		  /* for (all rows) */

	return result;
}

/** Retrieves properties of a sibling object that should be read from during
 *  some of the checks.
 *
 *  @param texture_type Type of the texture object that should be used for reading.
 *  @param out_to_id    Deref will be used to store texture object ID of the object.
 *  @param out_read_lod Deref will be used to store LOD to be used for reading from
 *                      the object.
 *
 **/
void TextureViewTestCoherency::getReadPropertiesForTextureType(_texture_type texture_type, glw::GLuint* out_to_id,
															   unsigned int* out_read_lod) const
{
	switch (texture_type)
	{
	case TEXTURE_TYPE_PARENT_TEXTURE:
	{
		*out_to_id = m_view_to_id;

		/* We've modified LOD1 of parent texture which corresponds
		 * to LOD 0 from the view's PoV
		 */
		*out_read_lod = 0;

		break;
	}

	case TEXTURE_TYPE_TEXTURE_VIEW:
	{
		*out_to_id = m_to_id;

		/* We've modified LOD1 of the view texture which corresponds
		 * to LOD2 from parent texture's PoV.
		 */
		*out_read_lod = 2;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized read source");
	}
	} /* switch (texture_type) */
}

/** Allocates a sufficiently large buffer to hold RGBA8 data of user-specified resolution
 *  and fills it with a static color (as described in the test specification)
 *
 *  It is caller's responsibility to release the returned buffer when it's no longer
 *  needed.
 *
 *  @param width  Width of the mip-map the buffer will be used as a data source for;
 *  @param height Height of the mip-map the buffer will be used as a data source for;
 *
 *  @return Pointer to the buffer.
 **/
unsigned char* TextureViewTestCoherency::getStaticColorTextureData(unsigned int width, unsigned int height) const
{
	/* Prepare the data buffer storing the data we want to replace the region of the
	 * data source with.
	 */
	unsigned char* result_ptr = new unsigned char[width * height * m_texture_n_components];

	for (unsigned int y = 0; y < height; ++y)
	{
		unsigned char* row_data_ptr = result_ptr + y * width * m_texture_n_components;

		for (unsigned int x = 0; x < width; ++x)
		{
			unsigned char* pixel_data_ptr = row_data_ptr + x * m_texture_n_components;

			memcpy(pixel_data_ptr, m_static_color_byte, sizeof(m_static_color_byte));
		} /* for (all columns) */
	}	 /* for (all rows) */

	return result_ptr;
}

/** Retrieves properties of a parent texture object that should be written to during
 *  some of the checks.
 *
 *  @param texture_type Type of the texture object that should be used for writing.
 *  @param out_to_id    Deref will be used to store texture object ID of the object. Must not be NULL.
 *  @param out_width    Deref will be used to store width of the mip-map the test will
 *                      be writing to; Must not be NULL.
 *  @param out_height   Deref will be used to store height of the mip-map the test will
 *                      be writing to. Must not be NULL.
 *
 **/
void TextureViewTestCoherency::getWritePropertiesForTextureType(_texture_type texture_type, glw::GLuint* out_to_id,
																unsigned int* out_width, unsigned int* out_height) const
{
	DE_ASSERT(out_to_id != DE_NULL);
	DE_ASSERT(out_width != DE_NULL);
	DE_ASSERT(out_height != DE_NULL);

	/* All tests will be attempting to modify layer 1 of either the texture
	 * or its sibling view. For views, the resolution is therefore going to
	 * be 16x16 (because the base resolution is 32x32, as the view uses a mipmap
	 * range of 1 to 2 inclusive); for parent texture, this will be 32x32 (as the base
	 * mip-map is 64x64)
	 */
	switch (texture_type)
	{
	case TEXTURE_TYPE_PARENT_TEXTURE:
	{
		*out_to_id  = m_to_id;
		*out_width  = m_texture_width >> 1;
		*out_height = m_texture_height >> 1;

		break;
	}

	case TEXTURE_TYPE_TEXTURE_VIEW:
	{
		*out_to_id  = m_view_to_id;
		*out_width  = m_texture_width >> 2;
		*out_height = m_texture_height >> 2;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized texture type");
	}
	} /* switch (texture_type) */
}

/** Initializes buffer objects that will be used during the test.
 *
 *  Throws exceptions if the initialization fails at any point.
 **/
void TextureViewTestCoherency::initBufferObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate and configure buffer object storage */
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed.");

	/* Case 1) needs the BO to hold just a single int.
	 * Case 3) needs one int per result texel.
	 *
	 * Allocate enough space to handle all the cases.
	 **/
	glw::GLint bo_size = static_cast<glw::GLint>((m_texture_height >> 1) * (m_texture_width >> 1) * sizeof(int));

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, bo_size, DE_NULL, /* data */
				  GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferdata() call failed.");
}

/** Initializes framebuffer objects that will be used during the test.
 *
 *  Throws exceptions if the initialization fails at any point.
 **/
void TextureViewTestCoherency::initFBO()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate framebuffer object(s) */
	gl.genFramebuffers(1, &m_draw_fbo_id);
	gl.genFramebuffers(1, &m_read_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed.");
}

/** Initializes program objects that will be used during the test.
 *
 *  This method will throw exceptions if either compilation or linking of
 *  any of the processed shaders/programs fails.
 *
 **/
void TextureViewTestCoherency::initPrograms()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* The test uses images in vertex shader stage. Make sure this is actually supported by
	 * the implementation */
	m_are_images_supported = m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_image_load_store");

	if (m_are_images_supported)
	{
		glw::GLint gl_max_vertex_image_uniforms_value = 0;

		gl.getIntegerv(GL_MAX_VERTEX_IMAGE_UNIFORMS, &gl_max_vertex_image_uniforms_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed for GL_MAX_VERTEX_IMAGE_UNIFORM pname");

		if (gl_max_vertex_image_uniforms_value < 1)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Image support will not be tested by view_parent_texture_coherency, as"
								  "the implementation does not support image uniforms in vertex shader stage."
							   << tcu::TestLog::EndMessage;

			/* We cannot execute the test on this platform */
			m_are_images_supported = false;
		}
	} /* if (m_are_images_supported) */

	/* Create program objects */
	if (m_are_images_supported)
	{
		m_gradient_image_write_po_id = gl.createProgram();
	}

	m_gradient_verification_po_id = gl.createProgram();
	m_gradient_write_po_id		  = gl.createProgram();
	m_verification_po_id		  = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call(s) failed.");

	/* Create fragment shader objects */
	m_gradient_write_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed for GL_FRAGMENT_SHADER type");

	/* Create vertex shader objects */
	if (m_are_images_supported)
	{
		m_gradient_image_write_vs_id = gl.createShader(GL_VERTEX_SHADER);
	}

	m_gradient_verification_vs_id = gl.createShader(GL_VERTEX_SHADER);
	m_gradient_write_vs_id		  = gl.createShader(GL_VERTEX_SHADER);
	m_verification_vs_id		  = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed for GL_VERTEX_SHADER type.");

	/* Set gradient verification program's fragment shader body */
	const char* gradient_verification_vs_body =
		"#version 400\n"
		"\n"
		"out int result;\n"
		"\n"
		"uniform float     lod;\n"
		"uniform bool      sample_exact_uv;\n"
		"uniform sampler2D texture;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    const float epsilon    = 1.0 / 255.0;\n"
		"    const vec4  end_rgba   = vec4(0.0, 0.1, 1.0, 1.0);\n"
		"    const vec4  start_rgba = vec4(1.0, 0.9, 0.0, 0.0);\n"
		"\n"
		"    ivec2 texture_size   = textureSize(texture, int(lod) );\n"
		"    vec2  uv             = vec2(      float(gl_VertexID % texture_size.x) / float(texture_size.x),\n"
		"                                1.0 - float(gl_VertexID / texture_size.x) / float(texture_size.y) );\n"
		"    vec4  expected_color;\n"
		"    vec4  texture_color  = textureLod(texture, uv, lod);\n"
		"\n"
		"    if (sample_exact_uv)\n"
		"    {\n"
		"        expected_color = mix(start_rgba, end_rgba, uv.x);\n"
		"    }\n"
		"    else\n"
		"    {\n"
		"        expected_color = mix(start_rgba, end_rgba, uv.x + 0.5/float(texture_size.x) );\n"
		"    }\n"
		"\n"
		"\n"
		"    if (abs(texture_color.x - expected_color.x) > epsilon ||\n"
		"        abs(texture_color.y - expected_color.y) > epsilon ||\n"
		"        abs(texture_color.z - expected_color.z) > epsilon ||\n"
		"        abs(texture_color.w - expected_color.w) > epsilon)\n"
		"    {\n"
		"        result = int( texture_color.y * 255.0);\n"
		"    }\n"
		"    else\n"
		"    {\n"
		"        result = 1;\n"
		"    }\n"
		"}\n";

	gl.shaderSource(m_gradient_verification_vs_id, 1 /* count */, &gradient_verification_vs_body, NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	/* Set gradient write (for images) program's vertex shader body */
	if (m_are_images_supported)
	{
		const char* gradient_write_image_vs_body =
			"#version 400\n"
			"\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"layout(rgba8) uniform image2D image;\n"
			"              uniform ivec2   image_size;\n"
			"\n"
			"void main()\n"
			"{\n"
			"    const  vec4 end_rgba   =  vec4(0.0, 0.1, 1.0, 1.0);\n"
			"    const  vec4 start_rgba =  vec4(1.0, 0.9, 0.0, 0.0);\n"
			"          ivec2 xy         = ivec2(gl_VertexID % image_size.x,              gl_VertexID / image_size.x);\n"
			"           vec2 uv         =  vec2(float(xy.x) / float(image_size.x), 1.0 - float(xy.y) / "
			"float(image_size.y) );\n"
			"          vec4  result     = mix  (start_rgba, end_rgba, uv.x);\n"
			"\n"
			"    imageStore(image, xy, result);\n"
			"}\n";

		gl.shaderSource(m_gradient_image_write_vs_id, 1 /* count */, &gradient_write_image_vs_body, NULL /* length */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");
	}

	/* Set gradient write program's fragment shader body */
	const char* gradient_write_fs_body = "#version 400\n"
										 "\n"
										 "in vec2 uv;\n"
										 "\n"
										 "layout(location = 0) out vec4 result;\n"
										 "\n"
										 "void main()\n"
										 "{\n"
										 "    const vec4 end_rgba   = vec4(0.0, 0.1, 1.0, 1.0);\n"
										 "    const vec4 start_rgba = vec4(1.0, 0.9, 0.0, 0.0);\n"
										 "\n"
										 "    result = mix(start_rgba, end_rgba, uv.x);\n"
										 "}\n";

	gl.shaderSource(m_gradient_write_fs_id, 1 /* count */, &gradient_write_fs_body, NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	/* Set gradient write program's vertex shader body */
	const char* gradient_write_vs_body =
		"#version 400\n"
		"\n"
		"out vec2 uv;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    switch (gl_VertexID)\n"
		"    {\n"
		"        case 0: gl_Position = vec4(-1.0, -1.0, 0.0, 1.0); uv = vec2(0.0, 1.0); break;\n"
		"        case 1: gl_Position = vec4(-1.0,  1.0, 0.0, 1.0); uv = vec2(0.0, 0.0); break;\n"
		"        case 2: gl_Position = vec4( 1.0, -1.0, 0.0, 1.0); uv = vec2(1.0, 1.0); break;\n"
		"        case 3: gl_Position = vec4( 1.0,  1.0, 0.0, 1.0); uv = vec2(1.0, 0.0); break;\n"
		"    }\n"
		"}\n";

	gl.shaderSource(m_gradient_write_vs_id, 1 /* count */, &gradient_write_vs_body, NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	/* Set verification program's vertex shader body */
	const char* verification_vs_body = "#version 400\n"
									   "\n"
									   "uniform vec4      expected_color;\n"
									   "uniform int       lod;\n"
									   "uniform sampler2D sampler;\n"
									   "\n"
									   "out int result;\n"
									   "\n"
									   "void main()\n"
									   "{\n"
									   "    const float epsilon = 1.0 / 256.0;\n"
									   "\n"
									   "    vec4 sampled_data = textureLod(sampler, vec2(0.5, 0.5), lod);\n"
									   "\n"
									   "    if (abs(sampled_data.x - expected_color.x) > epsilon ||\n"
									   "        abs(sampled_data.y - expected_color.y) > epsilon ||\n"
									   "        abs(sampled_data.z - expected_color.z) > epsilon ||\n"
									   "        abs(sampled_data.w - expected_color.w) > epsilon)\n"
									   "    {\n"
									   "        result = 0;\n"
									   "    }\n"
									   "    else\n"
									   "    {\n"
									   "        result = 1;\n"
									   "    }\n"
									   "}\n";

	gl.shaderSource(m_verification_vs_id, 1 /* count */, &verification_vs_body, NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	/* Compile the shaders */
	const glw::GLuint so_ids[] = { m_gradient_image_write_vs_id, m_gradient_verification_vs_id, m_gradient_write_fs_id,
								   m_gradient_write_vs_id, m_verification_vs_id };
	const unsigned int n_so_ids = sizeof(so_ids) / sizeof(so_ids[0]);

	for (unsigned int n_so_id = 0; n_so_id < n_so_ids; ++n_so_id)
	{
		glw::GLuint so_id = so_ids[n_so_id];

		if (so_id != 0)
		{
			gl.compileShader(so_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

			/* Verify the compilation ended successfully */
			glw::GLint compile_status = GL_FALSE;

			gl.getShaderiv(so_id, GL_COMPILE_STATUS, &compile_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

			if (compile_status != GL_TRUE)
			{
				TCU_FAIL("Shader compilation failed.");
			}
		}
	} /* for (all shader objects) */

	/* Attach the shaders to relevant programs */
	if (m_are_images_supported)
	{
		gl.attachShader(m_gradient_image_write_po_id, m_gradient_image_write_vs_id);
	}

	gl.attachShader(m_gradient_verification_po_id, m_gradient_verification_vs_id);
	gl.attachShader(m_gradient_write_po_id, m_gradient_write_fs_id);
	gl.attachShader(m_gradient_write_po_id, m_gradient_write_vs_id);
	gl.attachShader(m_verification_po_id, m_verification_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed.");

	/* Set up XFB */
	const char*		  verification_varying_name = "result";
	const glw::GLuint xfb_po_ids[]				= {
		m_gradient_verification_po_id, m_verification_po_id,
	};
	const unsigned int n_xfb_po_ids = sizeof(xfb_po_ids) / sizeof(xfb_po_ids[0]);

	for (unsigned int n_xfb_po_id = 0; n_xfb_po_id < n_xfb_po_ids; ++n_xfb_po_id)
	{
		glw::GLint po_id = xfb_po_ids[n_xfb_po_id];

		gl.transformFeedbackVaryings(po_id, 1 /* count */, &verification_varying_name, GL_INTERLEAVED_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed.");
	}

	/* Link the programs */
	const glw::GLuint po_ids[] = { m_gradient_image_write_po_id, m_gradient_verification_po_id, m_gradient_write_po_id,
								   m_verification_po_id };
	const unsigned int n_po_ids = sizeof(po_ids) / sizeof(po_ids[0]);

	for (unsigned int n_po_id = 0; n_po_id < n_po_ids; ++n_po_id)
	{
		glw::GLuint po_id = po_ids[n_po_id];

		if (po_id != 0)
		{
			gl.linkProgram(po_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

			/* Make sure the linking was successful. */
			glw::GLint link_status = GL_FALSE;

			gl.getProgramiv(po_id, GL_LINK_STATUS, &link_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

			if (link_status != GL_TRUE)
			{
				TCU_FAIL("Program linking failed.");
			}
		}
	} /* for (all program objects) */

	/* Retrieve uniform locations for gradient write program (image case) */
	if (m_are_images_supported)
	{
		m_gradient_image_write_image_size_location = gl.getUniformLocation(m_gradient_image_write_po_id, "image_size");

		if (m_gradient_image_write_image_size_location == -1)
		{
			TCU_FAIL("image_size is considered an inactive uniform which is invalid.");
		}
	}

	/* Retrieve uniform locations for gradient verification program */
	m_gradient_verification_po_sample_exact_uv_location =
		gl.getUniformLocation(m_gradient_verification_po_id, "sample_exact_uv");
	m_gradient_verification_po_lod_location		= gl.getUniformLocation(m_gradient_verification_po_id, "lod");
	m_gradient_verification_po_texture_location = gl.getUniformLocation(m_gradient_verification_po_id, "texture");

	if (m_gradient_verification_po_sample_exact_uv_location == -1)
	{
		TCU_FAIL("sample_exact_uv is considered an inactive uniform which is invalid");
	}

	if (m_gradient_verification_po_lod_location == -1)
	{
		TCU_FAIL("lod is considered an inactive uniform which is invalid.");
	}

	if (m_gradient_verification_po_texture_location == -1)
	{
		TCU_FAIL("texture is considered an inactive uniform which is invalid.");
	}

	/* Retrieve uniform locations for verification program */
	m_verification_po_expected_color_location = gl.getUniformLocation(m_verification_po_id, "expected_color");
	m_verification_po_lod_location			  = gl.getUniformLocation(m_verification_po_id, "lod");

	if (m_verification_po_expected_color_location == -1)
	{
		TCU_FAIL("expected_color is considered an inactive uniform which is invalid.");
	}

	if (m_verification_po_lod_location == -1)
	{
		TCU_FAIL("lod is considered an inactive uniform which is invalid.");
	}
}

/** Initializes texture objects required to run the test.
 *
 *  Throws exceptions if the initialization fails at any point.
 **/
void TextureViewTestCoherency::initTextures()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate the texture objects */
	gl.genTextures(1, &m_static_to_id);
	gl.genTextures(1, &m_to_id);
	gl.genTextures(1, &m_view_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call(s) failed.");

	/* Set up parent texture object */
	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage2D(GL_TEXTURE_2D, m_texture_n_levels, GL_RGBA8, m_texture_width, m_texture_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri() call(s) failed.");

	/* Set up the texture views we'll be using for the test */
	gl.textureView(m_view_to_id, GL_TEXTURE_2D, m_to_id, GL_RGBA8, 1, /* minlevel */
				   2,												  /* numlevels */
				   0,												  /* minlayer */
				   1);												  /* numlayers */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureView() call failed");

	gl.bindTexture(GL_TEXTURE_2D, m_view_to_id);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

	/* Set up storage for static color texture */
	gl.bindTexture(GL_TEXTURE_2D, m_static_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1, /* levels */
					GL_RGBA8, m_static_texture_width, m_static_texture_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

	/* Fill the texture objects with actual contents */
	initTextureContents();
}

/** Fills all relevant mip-maps of all previously initialized texture objects with
 *  contents.
 *
 *  Throws an exception if any of the issued GL API calls fail.
 **/
void TextureViewTestCoherency::initTextureContents()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Make sure parent texture object is bound before we start modifying
	 * the mip-maps */
	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	/* Set up parent texture mip-maps */
	unsigned char* base_mipmap_data_ptr = getHorizontalGradientData();

	gl.texSubImage2D(GL_TEXTURE_2D, 0, /* level */
					 0,				   /* xoffset */
					 0,				   /* yoffset */
					 m_texture_width, m_texture_height, GL_RGBA, GL_UNSIGNED_BYTE, base_mipmap_data_ptr);

	delete[] base_mipmap_data_ptr;
	base_mipmap_data_ptr = NULL;

	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage2D() call failed.");

	/* Generate all mip-maps */
	gl.generateMipmap(GL_TEXTURE_2D);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenerateMipmap() call failed.");

	/* Set up static color texture contents. We only need to fill the base mip-map
	 * since the texture's resolution is 1x1.
	 */
	DE_ASSERT(m_static_texture_height == 1 && m_static_texture_width == 1);

	unsigned char* static_texture_data_ptr = getStaticColorTextureData(m_static_texture_width, m_static_texture_height);

	gl.bindTexture(GL_TEXTURE_2D, m_static_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texSubImage2D(GL_TEXTURE_2D, 0, /* level */
					 0,				   /* xoffset */
					 0,				   /* yoffset */
					 m_static_texture_width, m_static_texture_height, GL_RGBA, GL_UNSIGNED_BYTE,
					 static_texture_data_ptr);

	/* Good to release the buffer at this point */
	delete[] static_texture_data_ptr;

	static_texture_data_ptr = DE_NULL;

	/* Was the API call successful? */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage2D() call failed.");
}

/** Initializes a vertex array object used for the draw calls issued during the test. */
void TextureViewTestCoherency::initVAO()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate and bind a vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult TextureViewTestCoherency::iterate()
{
	/* Do not execute the test if GL_ARB_texture_view is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_texture_view"))
	{
		throw tcu::NotSupportedError("GL_ARB_texture_view is not supported.");
	}

	/* Initialize all GL objects required to run the test */
	initBufferObjects();
	initPrograms();
	initTextures();
	initFBO();
	initVAO();

	/* Iterate over the set of texture types we are to test */
	const _texture_type texture_types[] = { TEXTURE_TYPE_PARENT_TEXTURE, TEXTURE_TYPE_TEXTURE_VIEW };
	const unsigned int  n_texture_types = sizeof(texture_types) / sizeof(texture_types[0]);

	for (unsigned int n_texture_type = 0; n_texture_type < n_texture_types; ++n_texture_type)
	{
		_texture_type texture_type = texture_types[n_texture_type];

		/* Verify parent texture/view coherency when using glTexSubImage2D() */
		checkAPICallCoherency(texture_type, true);

		/* Verify parent texture/view coherency when using glBlitFramebuffer() */
		checkAPICallCoherency(texture_type, false);

		/* Verify parent texture/view coherency when modifying contents of one
		 * of the objects in a program, and then reading the sibling from another
		 * program.
		 */
		checkProgramWriteCoherency(texture_type, false, /* should_use_images */
								   BARRIER_TYPE_NONE, VERIFICATION_MEAN_PROGRAM);

		if (m_are_images_supported)
		{
			/* Verify a view bound to an image unit and written to using image uniforms
			 * in vertex shader stage can later be sampled correctly, assuming
			 * a GL_TEXTURE_FETCH_BARRIER_BIT barrier is inserted between the write
			 * operations and sampling from another program object.
			 */
			checkProgramWriteCoherency(texture_type, true, /* should_use_images */
									   BARRIER_TYPE_TEXTURE_FETCH_BARRIER_BIT, VERIFICATION_MEAN_PROGRAM);

			/* Verify a view bound to an image unit and written to using image uniforms
			 * in vertex shader stage can later be correctly retrieved using a glGetTexImage()
			 * call, assuming a GL_TEXTURE_UPDATE_BARRIER_BIT barrier is inserted between the
			 * two operations.
			 **/
			checkProgramWriteCoherency(texture_type, true, /* should_use_images */
									   BARRIER_TYPE_TEXTURE_UPDATE_BUFFER_BIT, VERIFICATION_MEAN_GLGETTEXIMAGE);
		}

		/* Reinitialize all texture contents */
		initTextureContents();
	} /* for (all read sources) */

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
TextureViewTestBaseAndMaxLevels::TextureViewTestBaseAndMaxLevels(deqp::Context& context)
	: TestCase(context, "base_and_max_levels", "test_description")
	, m_texture_height(256)
	, m_texture_n_components(4)
	, m_texture_n_levels(6)
	, m_texture_width(256)
	, m_view_height(128)
	, m_view_width(128)
	, m_layer_data_lod0(DE_NULL)
	, m_layer_data_lod1(DE_NULL)
	, m_fbo_id(0)
	, m_fs_id(0)
	, m_po_id(0)
	, m_po_lod_index_uniform_location(-1)
	, m_po_to_sampler_uniform_location(-1)
	, m_result_to_id(0)
	, m_to_id(0)
	, m_vao_id(0)
	, m_view_to_id(0)
	, m_vs_id(0)
{
	/* Left blank on purpose */
}

/* Deinitializes all GL objects that may have been created during test execution. */
void TextureViewTestBaseAndMaxLevels::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_layer_data_lod0 != DE_NULL)
	{
		delete[] m_layer_data_lod0;

		m_layer_data_lod0 = DE_NULL;
	}

	if (m_layer_data_lod1 != DE_NULL)
	{
		delete[] m_layer_data_lod1;

		m_layer_data_lod1 = DE_NULL;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_result_to_id != 0)
	{
		gl.deleteTextures(1, &m_result_to_id);

		m_result_to_id = 0;
	}

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);

		m_to_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_view_to_id != 0)
	{
		gl.deleteTextures(1, &m_view_to_id);

		m_view_to_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/* Initializes and configures a program object used later by the test. */
void TextureViewTestBaseAndMaxLevels::initProgram()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate shader object IDs */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed");

	/* Generate program object ID */
	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed");

	/* Set up vertex shader body */
	static const char* vs_body =
		"#version 400\n"
		"\n"
		"out vec2 uv;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    switch (gl_VertexID)\n"
		"    {\n"
		"        case 0: gl_Position = vec4(-1.0,  1.0, 0.0, 1.0); uv = vec2(0.0, 1.0); break;\n"
		"        case 1: gl_Position = vec4(-1.0, -1.0, 0.0, 1.0); uv = vec2(0.0, 0.0); break;\n"
		"        case 2: gl_Position = vec4( 1.0,  1.0, 0.0, 1.0); uv = vec2(1.0, 1.0); break;\n"
		"        case 3: gl_Position = vec4( 1.0, -1.0, 0.0, 1.0); uv = vec2(1.0, 0.0); break;\n"
		"    };\n"
		"}\n";

	gl.shaderSource(m_vs_id, 1 /* count */, &vs_body, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed for vertex shader case");

	/* Set up fragment shader body */
	static const char* fs_body = "#version 400\n"
								 "\n"
								 "in vec2 uv;\n"
								 "\n"
								 "uniform int       lod_index;\n"
								 "uniform sampler2D to_sampler;\n"
								 "\n"
								 "out vec4 result;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    result = textureLod(to_sampler, uv, float(lod_index) );\n"
								 "}\n";

	gl.shaderSource(m_fs_id, 1 /* count */, &fs_body, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed for fragment shader case");

	/* Compile both shaders */
	const glw::GLuint  so_ids[] = { m_fs_id, m_vs_id };
	const unsigned int n_so_ids = sizeof(so_ids) / sizeof(so_ids[0]);

	for (unsigned int n_so_id = 0; n_so_id < n_so_ids; ++n_so_id)
	{
		glw::GLint so_id = so_ids[n_so_id];

		gl.compileShader(so_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

		/* Make sure the compilation has succeeded */
		glw::GLint compile_status = GL_FALSE;

		gl.getShaderiv(so_id, GL_COMPILE_STATUS, &compile_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

		if (compile_status != GL_TRUE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Shader compilation failed" << tcu::TestLog::EndMessage;
		}
	} /* for (all shader objects) */

	/* Attach the shaders to the program object */
	gl.attachShader(m_po_id, m_fs_id);
	gl.attachShader(m_po_id, m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed");

	/* Link the program object */
	gl.linkProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call faikled");

	/* Verify the linking has succeeded */
	glw::GLint link_status = GL_FALSE;

	gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed for GL_LINK_STATUS pname");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed");
	}

	/* Retrieve uniform locations */
	m_po_lod_index_uniform_location  = gl.getUniformLocation(m_po_id, "lod_index");
	m_po_to_sampler_uniform_location = gl.getUniformLocation(m_po_id, "to_sampler");

	if (m_po_lod_index_uniform_location == -1)
	{
		TCU_FAIL("lod_index is considered an inactive uniform");
	}

	if (m_po_to_sampler_uniform_location == -1)
	{
		TCU_FAIL("to_sampler is considered an inactive uniform");
	}
}

/* Initializes all GL objects used by the test. */
void TextureViewTestBaseAndMaxLevels::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initialize textures */
	initTextures();

	/* Initialize framebuffer and configure its attachments */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed");

	/* Build the program we'll need for the test */
	initProgram();

	/* Generate a vertex array object to execute the draw calls */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	/* Finally, allocate space for buffers that will be filled with rendered data */
	m_layer_data_lod0 = new unsigned char[m_texture_width * m_texture_height * m_texture_n_components];
	m_layer_data_lod1 = new unsigned char[(m_texture_width >> 1) * (m_texture_height >> 1) * m_texture_n_components];

	if (m_layer_data_lod0 == DE_NULL || m_layer_data_lod1 == DE_NULL)
	{
		TCU_FAIL("Out of memory");
	}
}

/** Initializes texture objects used by the test. */
void TextureViewTestBaseAndMaxLevels::initTextures()
{
	/* Generate IDs first */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_result_to_id);
	gl.genTextures(1, &m_to_id);
	gl.genTextures(1, &m_view_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call(s) failed");

	/* Set up parent texture object's storage */
	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	gl.texStorage2D(GL_TEXTURE_2D, m_texture_n_levels, GL_RGBA8, m_texture_width, m_texture_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed");

	/* Configure GL_TEXTURE_BASE_LEVEL parameter of the texture object as per test spec */
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri() call failed for GL_TEXTURE_BASE_LEVEL pname");

	/* Configure GL_TEXTURE_MAX_LEVEL parameter of the texture object as per test spec */
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri() call failed for GL_TEXTURE_MAX_LEVEL pname");

	/* Set up mip-maps */
	for (unsigned int n_mipmap = 0; n_mipmap < m_texture_n_levels; ++n_mipmap)
	{
		const float start_rgba[] = { /* As per test specification */
									 float(n_mipmap + 0) / 10.0f, float(n_mipmap + 1) / 10.0f,
									 float(n_mipmap + 2) / 10.0f, float(n_mipmap + 3) / 10.0f
		};
		const float end_rgba[] = { float(10 - (n_mipmap + 0)) / 10.0f, float(10 - (n_mipmap + 1)) / 10.0f,
								   float(10 - (n_mipmap + 2)) / 10.0f, float(10 - (n_mipmap + 3)) / 10.0f };

		/* Allocate space for the layer data */
		const unsigned int mipmap_height = m_texture_height >> n_mipmap;
		const unsigned int mipmap_width  = m_texture_width >> n_mipmap;
		unsigned char*	 data			 = new unsigned char[mipmap_width * mipmap_height * m_texture_n_components];

		if (data == NULL)
		{
			TCU_FAIL("Out of memory");
		}

		/* Fill the buffer with layer data */
		const unsigned int pixel_size = 4 /* components */;

		for (unsigned int y = 0; y < mipmap_height; ++y)
		{
			unsigned char* row_data_ptr = data + mipmap_width * y * pixel_size;

			for (unsigned int x = 0; x < mipmap_width; ++x)
			{
				const float	lerp_factor	= float(x) / float(mipmap_width);
				unsigned char* pixel_data_ptr = row_data_ptr + x * pixel_size;

				for (unsigned int n_component = 0; n_component < m_texture_n_components; n_component++)
				{
					pixel_data_ptr[n_component] = (unsigned char)((start_rgba[n_component] * lerp_factor +
																   end_rgba[n_component] * (1.0f - lerp_factor)) *
																  255.0f);
				}
			} /* for (all columns) */
		}	 /* for (all rows) */

		/* Upload the layer data */
		gl.texSubImage2D(GL_TEXTURE_2D, n_mipmap, 0, /* xoffset */
						 0,							 /* yoffset */
						 mipmap_width, mipmap_height, GL_RGBA, GL_UNSIGNED_BYTE, data);

		/* Release the data buffer */
		delete[] data;

		data = DE_NULL;

		/* Make sure the API call finished successfully */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage2D() call failed");
	} /* for (all mip-maps) */

	/* Configure the texture view storage as per spec. */
	gl.textureView(m_view_to_id, GL_TEXTURE_2D, m_to_id, GL_RGBA8, 0, /* minlevel */
				   5,												  /* numlevels */
				   0,												  /* minlayer */
				   1);												  /* numlayers */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureView() call failed");

	/* Configure the texture view's GL_TEXTURE_BASE_LEVEL parameter */
	gl.bindTexture(GL_TEXTURE_2D, m_view_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri() call failed for GL_TEXTURE_BASE_LEVEL pname");

	/* Configure the texture view's GL_TEXTURE_MAX_LEVEL parameter */
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri() call failed for GL_TEXTURE_MAX_LEVEL pname");

	/* Set up result texture storage */
	gl.bindTexture(GL_TEXTURE_2D, m_result_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	gl.texStorage2D(GL_TEXTURE_2D, 1, /* We will only attach the first level of the result texture to the FBO */
					GL_RGBA8, m_view_width, m_view_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult TextureViewTestBaseAndMaxLevels::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Only execute if GL_ARB_texture_view extension is supported */
	const std::vector<std::string>& extensions = m_context.getContextInfo().getExtensions();

	if (std::find(extensions.begin(), extensions.end(), "GL_ARB_texture_view") == extensions.end())
	{
		throw tcu::NotSupportedError("GL_ARB_texture_view is not supported");
	}

	/* Initialize all GL objects necessary to run the test */
	initTest();

	/* Activate test-wide program object */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	/* Bind the data texture */
	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture() call failed");

	gl.bindTexture(GL_TEXTURE_2D, m_view_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	/* We will now use the program to sample the view's LOD 0 and LOD 1 and store
	 * it in two separate textures.
	 **/
	for (unsigned int lod_level = 0; lod_level < 2; /* as per test spec */
		 ++lod_level)
	{
		/* Set up FBO attachments */
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed");

		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_result_to_id,
								0); /* level */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed");

		/* Update viewport configuration */
		gl.viewport(0, /* x */
					0, /* y */
					m_view_width >> lod_level, m_view_height >> lod_level);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() call failed");

		/* Configure program object uniforms before we continue */
		gl.uniform1i(m_po_lod_index_uniform_location, lod_level);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() call failed.");

		/* Render a triangle strip. The program we're using will output a full-screen
		 * quad with the sampled data */
		gl.drawArrays(GL_TRIANGLE_STRIP, 0 /* first */, 4 /* count */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");

		/* At the time of the draw call, we've modified the draw/read framebuffer binding
		 * so that everything we render ends up in result texture. It's time to read it */
		glw::GLvoid* result_data_ptr = (lod_level == 0) ? m_layer_data_lod0 : m_layer_data_lod1;

		gl.readPixels(0, /* x */
					  0, /* y */
					  m_view_width >> lod_level, m_view_height >> lod_level, GL_RGBA, GL_UNSIGNED_BYTE,
					  result_data_ptr);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed");
	} /* for (both LODs) */

	/* Now that we have both pieces of data, we can proceed with actual verification */
	for (unsigned int lod_level = 0; lod_level < 2; ++lod_level)
	{
		/* NOTE: This code is a modification of initialization routine
		 *       found in initTextures()
		 */
		const unsigned char epsilon			   = 1;
		const glw::GLvoid*  layer_data_ptr	 = (lod_level == 0) ? m_layer_data_lod0 : m_layer_data_lod1;
		const unsigned int  layer_height	   = m_view_height >> lod_level;
		const unsigned int  layer_width		   = m_view_width >> lod_level;
		const unsigned int  pixel_size		   = 4 /* components */;
		const unsigned int  view_minimum_level = 1; /* THS SHOULD BE 1 */
		const float			start_rgba[]	   = {
			/* As per test specification */
			float(lod_level + view_minimum_level + 0) / 10.0f, float(lod_level + view_minimum_level + 1) / 10.0f,
			float(lod_level + view_minimum_level + 2) / 10.0f, float(lod_level + view_minimum_level + 3) / 10.0f
		};
		const float end_rgba[] = { float(10 - (lod_level + view_minimum_level + 0)) / 10.0f,
								   float(10 - (lod_level + view_minimum_level + 1)) / 10.0f,
								   float(10 - (lod_level + view_minimum_level + 2)) / 10.0f,
								   float(10 - (lod_level + view_minimum_level + 3)) / 10.0f };

		for (unsigned int y = 0; y < layer_height; ++y)
		{
			const unsigned char* row_data_ptr = (const unsigned char*)layer_data_ptr + layer_width * y * pixel_size;

			for (unsigned int x = 0; x < layer_width; ++x)
			{
				const float			 lerp_factor	= float(x) / float(layer_width);
				const unsigned char* pixel_data_ptr = row_data_ptr + x * pixel_size;

				const unsigned char expected_data[] = {
					(unsigned char)((start_rgba[0] * lerp_factor + end_rgba[0] * (1.0f - lerp_factor)) * 255.0f),
					(unsigned char)((start_rgba[1] * lerp_factor + end_rgba[1] * (1.0f - lerp_factor)) * 255.0f),
					(unsigned char)((start_rgba[2] * lerp_factor + end_rgba[2] * (1.0f - lerp_factor)) * 255.0f),
					(unsigned char)((start_rgba[3] * lerp_factor + end_rgba[3] * (1.0f - lerp_factor)) * 255.0f)
				};

				if (de::abs(expected_data[0] - pixel_data_ptr[0]) > epsilon ||
					de::abs(expected_data[1] - pixel_data_ptr[1]) > epsilon ||
					de::abs(expected_data[2] - pixel_data_ptr[2]) > epsilon ||
					de::abs(expected_data[3] - pixel_data_ptr[3]) > epsilon)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Found an invalid texel at (" << x << ", " << y
									   << ");"
										  " expected value:"
										  "("
									   << expected_data[0] << ", " << expected_data[1] << ", " << expected_data[2]
									   << ", " << expected_data[3] << ")"
																	  ", found:"
																	  "("
									   << pixel_data_ptr[0] << ", " << pixel_data_ptr[1] << ", " << pixel_data_ptr[2]
									   << ", " << pixel_data_ptr[3] << ")" << tcu::TestLog::EndMessage;

					TCU_FAIL("Rendered data does not match expected pixel data");
				} /* if (pixel mismatch found) */
			}	 /* for (all columns) */
		}		  /* for (all rows) */
	}			  /* for (both LODs) */

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 **/
TextureViewTestReferenceCounting::TextureViewTestReferenceCounting(deqp::Context& context)
	: TestCase(context, "reference_counting",
			   "Makes sure that sampling from views, for which the parent texture object "
			   "has already been deleted, works correctly.")
	, m_bo_id(0)
	, m_parent_to_id(0)
	, m_po_id(0)
	, m_po_expected_texel_uniform_location(-1)
	, m_po_lod_uniform_location(-1)
	, m_vao_id(0)
	, m_view_to_id(0)
	, m_view_view_to_id(0)
	, m_vs_id(0)
	, m_texture_height(64)
	, m_texture_n_levels(7)
	, m_texture_width(64)
{
	/* Configure a vector storing unique colors that should be used
	 * for filling subsequent mip-maps of parent texture */
	m_mipmap_colors.push_back(_norm_vec4(123, 34, 56, 78));
	m_mipmap_colors.push_back(_norm_vec4(234, 45, 67, 89));
	m_mipmap_colors.push_back(_norm_vec4(34, 56, 78, 90));
	m_mipmap_colors.push_back(_norm_vec4(45, 67, 89, 1));
	m_mipmap_colors.push_back(_norm_vec4(56, 78, 90, 123));
	m_mipmap_colors.push_back(_norm_vec4(67, 89, 1, 234));
	m_mipmap_colors.push_back(_norm_vec4(78, 90, 12, 34));

	DE_ASSERT(m_mipmap_colors.size() == m_texture_n_levels);
}

/** Deinitializes all GL objects that may have been created during test
 *  execution.
 **/
void TextureViewTestReferenceCounting::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	if (m_parent_to_id != 0)
	{
		gl.deleteTextures(1, &m_parent_to_id);

		m_parent_to_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_view_to_id != 0)
	{
		gl.deleteTextures(1, &m_view_to_id);

		m_view_to_id = 0;
	}

	if (m_view_view_to_id != 0)
	{
		gl.deleteTextures(1, &m_view_view_to_id);

		m_view_view_to_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/* Initializes a program object to be used during the test. */
void TextureViewTestReferenceCounting::initProgram()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program & shader object IDs */
	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed");

	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed");

	/* Set vertex shader body */
	const char* vs_body = "#version 400\n"
						  "\n"
						  "uniform vec4      expected_texel;\n"
						  "uniform int       lod;\n"
						  "uniform sampler2D sampler;\n"
						  "\n"
						  "out int has_passed;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "          vec4  data    = textureLod(sampler, vec2(0.5, 0.5), lod);\n"
						  "    const float epsilon = 1.0 / 256.0;\n"
						  "\n"
						  "    if (abs(data.r - expected_texel.r) > epsilon ||\n"
						  "        abs(data.g - expected_texel.g) > epsilon ||\n"
						  "        abs(data.b - expected_texel.b) > epsilon ||\n"
						  "        abs(data.a - expected_texel.a) > epsilon)\n"
						  "    {\n"
						  "        has_passed = 0;\n"
						  "    }\n"
						  "    else\n"
						  "    {\n"
						  "        has_passed = 1;\n"
						  "    }\n"
						  "}\n";

	gl.shaderSource(m_vs_id, 1 /* count */, &vs_body, NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed");

	/* Configure XFB */
	const char* varying_name = "has_passed";

	gl.transformFeedbackVaryings(m_po_id, 1 /* count */, &varying_name, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed");

	/* Attach the shader object to the program */
	gl.attachShader(m_po_id, m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed");

	/* Compile the shader */
	gl.compileShader(m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

	/* Make sure the compilation has succeeded */
	glw::GLint compile_status = GL_FALSE;

	gl.getShaderiv(m_vs_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

	if (compile_status != GL_TRUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Shader compilation failed" << tcu::TestLog::EndMessage;
	}

	/* Link the program object */
	gl.linkProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed");

	/* Make sure the program object has linked successfully */
	glw::GLint link_status = GL_FALSE;

	gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed");
	}

	/* Retrieve uniform locations */
	m_po_expected_texel_uniform_location = gl.getUniformLocation(m_po_id, "expected_texel");
	m_po_lod_uniform_location			 = gl.getUniformLocation(m_po_id, "lod");

	if (m_po_expected_texel_uniform_location == -1)
	{
		TCU_FAIL("expected_texel is considered an inactive uniform which is invalid");
	}

	if (m_po_lod_uniform_location == -1)
	{
		TCU_FAIL("lod is considered an inactive uniform which is invalid");
	}
}

/** Initializes all texture objects and all views used by the test. */
void TextureViewTestReferenceCounting::initTextures()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate texture IDs */
	gl.genTextures(1, &m_parent_to_id);
	gl.genTextures(1, &m_view_to_id);
	gl.genTextures(1, &m_view_view_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call(s) failed");

	/* Set up parent texture object A */
	gl.bindTexture(GL_TEXTURE_2D, m_parent_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	gl.texStorage2D(GL_TEXTURE_2D, m_texture_n_levels, GL_RGBA8, m_texture_width, m_texture_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed");

	/* Set up view B */
	gl.textureView(m_view_to_id, GL_TEXTURE_2D, m_parent_to_id, GL_RGBA8, 0, /* minlevel */
				   m_texture_n_levels, 0,									 /* minlayer */
				   1);														 /* numlayers */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureView() call failed");

	/* Set up view C */
	gl.textureView(m_view_view_to_id, GL_TEXTURE_2D, m_view_to_id, GL_RGBA8, 0, /* minlevel */
				   m_texture_n_levels, 0,										/* minlayer */
				   1);															/* numlayers */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureView() call failed");

	/* Fill parent texture mip-maps with different static colors */
	unsigned char* texel_data = new unsigned char[m_texture_width * m_texture_height * 4 /* components */];

	for (unsigned int n_mipmap = 0; n_mipmap < m_mipmap_colors.size(); n_mipmap++)
	{
		const _norm_vec4&  mipmap_color  = m_mipmap_colors[n_mipmap];
		const unsigned int mipmap_height = m_texture_height >> n_mipmap;
		const unsigned int mipmap_width  = m_texture_width >> n_mipmap;

		for (unsigned int n_texel = 0; n_texel < mipmap_height * mipmap_width; ++n_texel)
		{
			unsigned char* texel_data_ptr = texel_data + n_texel * sizeof(mipmap_color.rgba);

			memcpy(texel_data_ptr, mipmap_color.rgba, sizeof(mipmap_color.rgba));
		} /* for (all relevant mip-map texels) */

		/* Upload new mip-map contents */
		gl.texSubImage2D(GL_TEXTURE_2D, n_mipmap, 0, /* xoffset */
						 0,							 /* yoffset */
						 m_texture_width >> n_mipmap, m_texture_height >> n_mipmap, GL_RGBA, GL_UNSIGNED_BYTE,
						 texel_data);

		if (gl.getError() != GL_NO_ERROR)
		{
			delete[] texel_data;
			texel_data = NULL;

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage2D() call failed");
		}
	} /* for (all mip-maps) */

	delete[] texel_data;
	texel_data = NULL;
}

/* Initialize all GL objects necessary to run the test */
void TextureViewTestReferenceCounting::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initialize all texture objects */
	initTextures();

	/* Initialize test program object */
	initProgram();

	/* Initialize XFB */
	initXFB();

	/* Generate and bind a vertex array object, since we'll be doing a number of
	 * draw calls later in the test */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");
}

/** Initializes a buffer object later used for Transform Feedback and binds
 *  it to both general and indexed Transform Feedback binding points.
 **/
void TextureViewTestReferenceCounting::initXFB()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Sanity checks */
	DE_ASSERT(m_po_id != 0);

	/* Generate a buffer object we'll use for Transform Feedback */
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	/* Set up buffer object storage. We need it to be large enough to hold
	 * sizeof(glw::GLint) per mipmap level */
	const glw::GLint bo_size = (glw::GLint)(sizeof(glw::GLint) * m_mipmap_colors.size());

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed.");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, bo_size, DE_NULL, /* data */
				  GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult TextureViewTestReferenceCounting::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Carry on only if GL_ARB_texture_view extension is supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_texture_view"))
	{
		throw tcu::NotSupportedError("GL_ARB_texture_view is not supported");
	}

	/* Initialize all GL objects used for the test */
	initTest();

	/* Make sure texture unit 0 is currently active */
	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture() call failed.");

	/* Activate the test program object */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	/* Run the test in three iterations:
	 *
	 * - Sample both the texture and all the views; once that's finished
	 *   successfully, delete the parent texture.
	 * - Sample both views; once that's finished successfully, delete
	 *   the first of the views;
	 * - Sample the only remaining view and make sure all mip-maps store
	 *   valid colors.
	 **/
	for (unsigned int n_iteration = 0; n_iteration < 3; /* iterations in total */
		 n_iteration++)
	{
		glw::GLuint to_ids_to_sample[3] = { 0, 0, 0 };

		/* Configure IDs of textures we need to validate for current iteration */
		switch (n_iteration)
		{
		case 0:
		{
			to_ids_to_sample[0] = m_parent_to_id;
			to_ids_to_sample[1] = m_view_to_id;
			to_ids_to_sample[2] = m_view_view_to_id;

			break;
		}

		case 1:
		{
			to_ids_to_sample[0] = m_view_to_id;
			to_ids_to_sample[1] = m_view_view_to_id;

			break;
		}

		case 2:
		{
			to_ids_to_sample[0] = m_view_view_to_id;

			break;
		}

		default:
			TCU_FAIL("Invalid iteration index");
		} /* switch (n_iteration) */

		/* Iterate through all texture objects of our concern */
		for (unsigned int n_texture = 0; n_texture < sizeof(to_ids_to_sample) / sizeof(to_ids_to_sample[0]);
			 n_texture++)
		{
			glw::GLint to_id = to_ids_to_sample[n_texture];

			if (to_id == 0)
			{
				/* No texture object to sample from. */
				continue;
			}

			/* Bind the texture object of our interest to GL_TEXTURE_2D */
			gl.bindTexture(GL_TEXTURE_2D, to_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

			/* Start XFB */
			gl.beginTransformFeedback(GL_POINTS);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed.");

			/* Iterate through all mip-maps of the texture we're currently sampling */
			for (unsigned int n_mipmap = 0; n_mipmap < m_mipmap_colors.size(); ++n_mipmap)
			{
				const _norm_vec4& expected_mipmap_color = m_mipmap_colors[n_mipmap];

				/* Update uniforms first */
				gl.uniform4f(m_po_expected_texel_uniform_location, (float)(expected_mipmap_color.rgba[0]) / 255.0f,
							 (float)(expected_mipmap_color.rgba[1]) / 255.0f,
							 (float)(expected_mipmap_color.rgba[2]) / 255.0f,
							 (float)(expected_mipmap_color.rgba[3]) / 255.0f);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform4f() call failed.");

				gl.uniform1i(m_po_lod_uniform_location, n_mipmap);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() call failed.");

				/* Draw a single point. That'll feed the XFB buffer object with a single bool
				 * indicating if the test passed for the mip-map, or not */
				gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");
			} /* for (all mip-maps) */

			/* We're done - close XFB */
			gl.endTransformFeedback();
			GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed.");

			/* How did the sampling go? Map the buffer object containing the run
			 * results into process space.
			 */
			const glw::GLint* run_results_ptr =
				(const glw::GLint*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer() call failed.");

			if (run_results_ptr == NULL)
			{
				TCU_FAIL("Pointer to mapped buffer object storage is NULL.");
			}

			/* Make sure all mip-maps were sampled successfully */
			for (unsigned int n_mipmap = 0; n_mipmap < m_mipmap_colors.size(); ++n_mipmap)
			{
				if (run_results_ptr[n_mipmap] != 1)
				{
					/* Make sure the TF BO is unmapped before we throw the exception */
					gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);

					m_testCtx.getLog() << tcu::TestLog::Message << "Invalid data was sampled for mip-map level ["
									   << n_mipmap << "] and iteration [" << n_iteration << "]"
									   << tcu::TestLog::EndMessage;

					TCU_FAIL("Mip-map sampling failed.");
				}
			} /* for (all mip-maps) */

			/* Good to unmap the buffer object at this point */
			gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");
		} /* for (all initialized texture objects) */

		/* Now that we're done with the iteration, we should delete iteration-specific texture
		 * object.
		 */
		switch (n_iteration)
		{
		case 0:
		{
			gl.deleteTextures(1, &m_parent_to_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() call failed.");

			m_parent_to_id = 0;
			break;
		}

		case 1:
		{
			gl.deleteTextures(1, &m_view_to_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() call failed.");

			m_view_to_id = 0;
			break;
		}

		case 2:
		{
			gl.deleteTextures(1, &m_view_view_to_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() call failed.");

			m_view_view_to_id = 0;
			break;
		}

		default:
			TCU_FAIL("Invalid iteration index");
		} /* switch (n_iteration) */
	}	 /* for (all iterations) */

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 **/
TextureViewTests::TextureViewTests(deqp::Context& context)
	: TestCaseGroup(context, "texture_view", "Verifies \"texture view\" functionality")
{
	/* Left blank on purpose */
}

/** Initializes a texture_storage_multisample test group.
 *
 **/
void TextureViewTests::init(void)
{
	addChild(new TextureViewTestGetTexParameter(m_context));
	addChild(new TextureViewTestErrors(m_context));
	addChild(new TextureViewTestViewSampling(m_context));
	addChild(new TextureViewTestViewClasses(m_context));
	addChild(new TextureViewTestCoherency(m_context));
	addChild(new TextureViewTestBaseAndMaxLevels(m_context));
	addChild(new TextureViewTestReferenceCounting(m_context));
}

} /* glcts namespace */
