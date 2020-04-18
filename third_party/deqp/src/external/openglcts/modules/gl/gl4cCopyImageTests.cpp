/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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
 * \file gl4cCopyImageTests.cpp
 * \brief Implements CopyImageSubData functional tests.
 */ /*-------------------------------------------------------------------*/

#include "gl4cCopyImageTests.hpp"

#include "gluDefs.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuFloat.hpp"
#include "tcuTestLog.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "deMath.h"

/* There are far too much combinations specified for FunctionalTest.
 *
 * Following flags controls what is enabled. Set as 1 to enable
 * all test case from given category, 0 otherwise.
 *
 * By default everything is disabled - which still gives 14560 test cases.
 *
 * ALL_FORMAT  - selects all internal formats, 61 x 61
 * ALL_TARGETS - selects all valid targets, 10 x 10
 * ALL_IMG_DIM - selects all image dimmensions, 9 x 9
 * ALL_REG_DIM - selects all region dimmensions, 7 x 7
 * ALL_REG_POS - selects all region positions, like left-top corner, 8 x 8
 */
#define COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_FORMATS 0
#define COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_TARGETS 0
#define COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_IMG_DIM 0
#define COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_REG_DIM 0
#define COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_REG_POS 0

/* The following flags controls if workarounds are enabled */
#define COPY_IMAGE_WRKARD_FORMATS 0

using namespace glw;

namespace gl4cts
{
namespace CopyImage
{
/** Various utilities used by all tests
 *
 **/
class Utils
{
public:
	/* Routines */
	static bool areFormatsCompatible(glw::GLenum src, glw::GLenum dst);

	static bool comparePixels(glw::GLenum left_internal_format, const glw::GLdouble& left_red,
							  const glw::GLdouble& left_green, const glw::GLdouble& left_blue,
							  const glw::GLdouble& left_alpha, glw::GLenum right_internal_format,
							  const glw::GLdouble& right_red, const glw::GLdouble& right_green,
							  const glw::GLdouble& right_blue, const glw::GLdouble& right_alpha);

	static bool comparePixels(glw::GLuint left_pixel_size, const glw::GLubyte* left_pixel_data,
							  glw::GLuint right_pixel_size, const glw::GLubyte* right_pixel_data);

	static void deleteTexture(deqp::Context& context, glw::GLenum target, glw::GLuint name);

	static bool isTargetMultilayer(glw::GLenum target);
	static bool isTargetMultilevel(glw::GLenum target);
	static bool isTargetMultisampled(glw::GLenum target);

	static glw::GLuint generateTexture(deqp::Context& context, glw::GLenum target);

	static void maskPixelForFormat(glw::GLenum internal_format, glw::GLubyte* pixel);

	static glw::GLdouble getEpsilon(glw::GLenum internal_format);
	static glw::GLuint getPixelSizeForFormat(glw::GLenum internal_format);
	static glw::GLenum getFormat(glw::GLenum internal_format);
	static glw::GLuint getNumberOfChannels(glw::GLenum internal_format);

	static std::string getPixelString(glw::GLenum internal_format, const glw::GLubyte* pixel);

	static glw::GLenum getType(glw::GLenum internal_format);
	static void makeTextureComplete(deqp::Context& context, glw::GLenum target, glw::GLuint id, glw::GLint base_level,
									glw::GLint max_level);

	static glw::GLuint prepareCompressedTex(deqp::Context& context, glw::GLenum target, glw::GLenum internal_format);

	static glw::GLuint prepareMultisampleTex(deqp::Context& context, glw::GLenum target, glw::GLsizei n_samples);

	static glw::GLuint prepareRenderBuffer(deqp::Context& context, glw::GLenum internal_format);

	static glw::GLuint prepareTex16x16x6(deqp::Context& context, glw::GLenum target, glw::GLenum internal_format,
										 glw::GLenum format, glw::GLenum type, glw::GLuint& out_buf_id);

	static void prepareTexture(deqp::Context& context, glw::GLuint name, glw::GLenum target,
							   glw::GLenum internal_format, glw::GLenum format, glw::GLenum type, glw::GLuint level,
							   glw::GLuint width, glw::GLuint height, glw::GLuint depth, const glw::GLvoid* pixels,
							   glw::GLuint& out_buf_id);

	static glw::GLenum transProxyToRealTarget(glw::GLenum target);
	static glw::GLenum transRealToBindTarget(glw::GLenum target);

	static void readChannel(glw::GLenum type, glw::GLuint channel, const glw::GLubyte* pixel, glw::GLdouble& out_value);

	static void writeChannel(glw::GLenum type, glw::GLuint channel, glw::GLdouble value, glw::GLubyte* pixel);

	static void packPixel(glw::GLenum internal_format, glw::GLenum type, glw::GLdouble red, glw::GLdouble green,
						  glw::GLdouble blue, glw::GLdouble alpha, glw::GLubyte* out_pixel);

	static void unpackPixel(glw::GLenum format, glw::GLenum type, const glw::GLubyte* pixel, glw::GLdouble& out_red,
							glw::GLdouble& out_green, glw::GLdouble& out_blue, glw::GLdouble& out_alpha);

	static bool unpackAndComaprePixels(glw::GLenum left_format, glw::GLenum left_type, glw::GLenum left_internal_format,
									   const glw::GLubyte* left_pixel, glw::GLenum right_format, glw::GLenum right_type,
									   glw::GLenum right_internal_format, const glw::GLubyte* right_pixel);

	static inline bool roundComponent(glw::GLenum internal_format, glw::GLenum component, glw::GLdouble& value);
};

/* Global constants */
static const GLenum s_internal_formats[] = {
	/* R8 */
	GL_R8, GL_R8I, GL_R8UI, GL_R8_SNORM,

	/* R16 */
	GL_R16, GL_R16F, GL_R16I, GL_R16UI, GL_R16_SNORM,

	/* R32 */
	GL_R32F, GL_R32I, GL_R32UI,

	/* RG8 */
	GL_RG8, GL_RG8I, GL_RG8UI, GL_RG8_SNORM,

	/* RG16 */
	GL_RG16, GL_RG16F, GL_RG16I, GL_RG16UI, GL_RG16_SNORM,

	/* RG32 */
	GL_RG32F, GL_RG32I, GL_RG32UI,

	/* RGB8 */
	GL_RGB8, GL_RGB8I, GL_RGB8UI, GL_RGB8_SNORM,

	/* RGB16 */
	GL_RGB16, GL_RGB16F, GL_RGB16I, GL_RGB16UI, GL_RGB16_SNORM,

	/* RGB32 */
	GL_RGB32F, GL_RGB32I, GL_RGB32UI,

	/* RGBA8 */
	GL_RGBA8, GL_RGBA8I, GL_RGBA8UI, GL_RGBA8_SNORM,

	/* RGBA16 */
	GL_RGBA16, GL_RGBA16F, GL_RGBA16I, GL_RGBA16UI, GL_RGBA16_SNORM,

	/* RGBA32 */
	GL_RGBA32F, GL_RGBA32I, GL_RGBA32UI,

	/* 8 */
	GL_R3_G3_B2, GL_RGBA2,

	/* 12 */
	GL_RGB4,

	/* 15 */
	GL_RGB5,

	/* 16 */
	GL_RGBA4, GL_RGB5_A1,

	/* 30 */
	GL_RGB10,

	/* 32 */
	GL_RGB10_A2, GL_RGB10_A2UI, GL_R11F_G11F_B10F, GL_RGB9_E5,

	/* 36 */
	GL_RGB12,

	/* 48 */
	GL_RGBA12,
};

static const GLenum s_invalid_targets[] = {
	GL_TEXTURE_BUFFER,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_PROXY_TEXTURE_1D,
	GL_PROXY_TEXTURE_1D_ARRAY,
	GL_PROXY_TEXTURE_2D,
	GL_PROXY_TEXTURE_2D_ARRAY,
	GL_PROXY_TEXTURE_2D_MULTISAMPLE,
	GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY,
	GL_PROXY_TEXTURE_3D,
	GL_PROXY_TEXTURE_CUBE_MAP,
	GL_PROXY_TEXTURE_CUBE_MAP_ARRAY,
	GL_PROXY_TEXTURE_RECTANGLE,
};

static const GLenum s_valid_targets[] = {
	GL_RENDERBUFFER,
	GL_TEXTURE_1D,
	GL_TEXTURE_1D_ARRAY,
	GL_TEXTURE_2D,
	GL_TEXTURE_2D_ARRAY,
	GL_TEXTURE_2D_MULTISAMPLE,
	GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
	GL_TEXTURE_3D,
	GL_TEXTURE_CUBE_MAP,
	GL_TEXTURE_CUBE_MAP_ARRAY,
	GL_TEXTURE_RECTANGLE,
};

static const GLuint s_n_internal_formats = sizeof(s_internal_formats) / sizeof(s_internal_formats[0]);
static const GLuint s_n_invalid_targets  = sizeof(s_invalid_targets) / sizeof(s_invalid_targets[0]);
static const GLuint s_n_valid_targets	= sizeof(s_valid_targets) / sizeof(s_valid_targets[0]);

/**
 * Pixel compatibility depends on pixel size. However value returned by getPixelSizeForFormat
 * needs some refinements
 *
 * @param internal_format Internal format of image
 *
 * @return Size of pixel for compatibility checks
 **/
GLuint getPixelSizeForCompatibilityVerification(GLenum internal_format)
{
	GLuint size = Utils::getPixelSizeForFormat(internal_format);

	switch (internal_format)
	{
	case GL_RGBA2:
		size = 1;
		break;
	default:
		break;
	}

	return size;
}

#if COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_FORMATS == 0

/** Filters out formats that should not be tested by FunctionalTest
 *
 * @param format Internal format
 *
 * @return true if format should be tested, false otherwise
 **/
bool filterFormats(GLenum format)
{
	bool result = true;

	switch (format)
	{
	/* R8 */
	case GL_R8I:
	case GL_R8UI:
	case GL_R8_SNORM:

	/* R16 */
	case GL_R16:
	case GL_R16F:
	case GL_R16I:
	case GL_R16UI:
	case GL_R16_SNORM:

	/* R32 */
	case GL_R32F:
	case GL_R32I:
	case GL_R32UI:

	/* RG8 */
	case GL_RG8:
	case GL_RG8I:
	case GL_RG8UI:
	case GL_RG8_SNORM:

	/* RG16 */
	case GL_RG16:
	case GL_RG16F:
	case GL_RG16I:
	case GL_RG16UI:
	case GL_RG16_SNORM:

	/* RG32 */
	case GL_RG32F:
	case GL_RG32I:
	case GL_RG32UI:

	/* RGB8 */
	case GL_RGB8:
	case GL_RGB8I:
	case GL_RGB8UI:
	case GL_RGB8_SNORM:

	/* RGB16 */
	case GL_RGB16:
	case GL_RGB16F:
	case GL_RGB16I:
	case GL_RGB16UI:
	case GL_RGB16_SNORM:

	/* RGB32 */
	case GL_RGB32I:
	case GL_RGB32UI:

	/* RGBA8 */
	case GL_RGBA8:
	case GL_RGBA8I:
	case GL_RGBA8UI:
	case GL_RGBA8_SNORM:

	/* RGBA16 */
	case GL_RGBA16:
	case GL_RGBA16F:
	case GL_RGBA16I:
	case GL_RGBA16UI:
	case GL_RGBA16_SNORM:

	/* RGBA32 */
	case GL_RGBA32F:
	case GL_RGBA32I:
	case GL_RGBA32UI:
		result = false;
		break;

	default:
		result = true;
		break;
	}

	return result;
}

#endif /* COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_FORMATS */

/** Checks if two internal_formats are compatible
 *
 * @param src Internal format of source image
 * @param dst Internal format of destination image
 *
 * @return true for compatible formats, false otherwise
 **/
bool Utils::areFormatsCompatible(glw::GLenum src, glw::GLenum dst)
{
	const GLuint dst_size = getPixelSizeForCompatibilityVerification(dst);
	const GLuint src_size = getPixelSizeForCompatibilityVerification(src);

	if (dst_size != src_size)
	{
		return false;
	}

#if COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_FORMATS == 0

	if ((false == filterFormats(src)) || (false == filterFormats(dst)))
	{
		return false;
	}

#endif /* COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_FORMATS */

	if (src != dst)
	{
		if ((GL_R3_G3_B2 == dst) || (GL_R3_G3_B2 == src) || (GL_RGBA2 == dst) || (GL_RGBA2 == src) ||
			(GL_RGBA4 == dst) || (GL_RGBA4 == src) || (GL_RGB5_A1 == dst) || (GL_RGB5_A1 == src) || (GL_RGB10 == dst) ||
			(GL_RGB10 == src))
		{
			return false;
		}
	}

#if COPY_IMAGE_WRKARD_FORMATS

	if ((GL_RGB10_A2 == src) && (GL_R11F_G11F_B10F == dst) || (GL_RGB10_A2 == src) && (GL_RGB9_E5 == dst) ||
		(GL_RGB10_A2UI == src) && (GL_R11F_G11F_B10F == dst) || (GL_RGB10_A2UI == src) && (GL_RGB9_E5 == dst) ||
		(GL_RGB9_E5 == src) && (GL_RGB10_A2 == dst) || (GL_RGB9_E5 == src) && (GL_RGB10_A2UI == dst) ||
		(GL_R11F_G11F_B10F == src) && (GL_RGB10_A2 == dst) || (GL_R11F_G11F_B10F == src) && (GL_RGB10_A2UI == dst))
	{
		return false;
	}

#endif /* COPY_IMAGE_WRKARD_FORMATS */

	if (2 == dst_size)
	{
		if (src == dst)
		{
			return true;
		}

		if (((GL_RGB4 == src) && (GL_RGB4 != dst)) || ((GL_RGB4 != src) && (GL_RGB4 == dst)) ||
			((GL_RGB5 == src) && (GL_RGB5 != dst)) || ((GL_RGB5 != src) && (GL_RGB5 == dst)))
		{
			return false;
		}

		return true;
	}

	if (4 == dst_size)
	{
		if (src == dst)
		{
			return true;
		}

		return true;
	}

	return true;
}

/** Compare two pixels
 *
 * @param left_internal_format  Internal format of left image
 * @param left_red              Red channel of left image
 * @param left_green            Green channel of left image
 * @param left_blue             Blue channel of left image
 * @param left_alpha            Alpha channel of left image
 * @param right_internal_format Internal format of right image
 * @param right_red             Red channel of right image
 * @param right_green           Green channel of right image
 * @param right_blue            Blue channel of right image
 * @param right_alpha           Alpha channel of right image
 *
 * @return true if pixels match, false otherwise
 **/
bool Utils::comparePixels(GLenum left_internal_format, const GLdouble& left_red, const GLdouble& left_green,
						  const GLdouble& left_blue, const GLdouble& left_alpha, GLenum right_internal_format,
						  const GLdouble& right_red, const GLdouble& right_green, const GLdouble& right_blue,
						  const GLdouble& right_alpha)
{
	const GLuint left_n_channels  = getNumberOfChannels(left_internal_format);
	const GLuint right_n_channels = getNumberOfChannels(right_internal_format);
	const GLuint n_channels		  = (left_n_channels >= right_n_channels) ? right_n_channels : left_n_channels;

	const GLdouble left_channels[4] = { left_red, left_green, left_blue, left_alpha };

	const GLdouble right_channels[4] = { right_red, right_green, right_blue, right_alpha };

	for (GLuint i = 0; i < n_channels; ++i)
	{
		const GLdouble left		 = left_channels[i];
		const GLdouble right	 = right_channels[i];
		const GLdouble left_eps  = getEpsilon(left_internal_format);
		const GLdouble right_eps = getEpsilon(right_internal_format);
		const GLdouble eps		 = fabs(std::max(left_eps, right_eps));

		if (eps < fabs(left - right))
		{
			return false;
		}
	}

	return true;
}

/** Compare two pixels with memcmp
 *
 * @param left_pixel_size  Size of left pixel
 * @param left_pixel_data  Data of left pixel
 * @param right_pixel_size Size of right pixel
 * @param right_pixel_data Data of right pixel
 *
 * @return true if memory match, false otherwise
 **/
bool Utils::comparePixels(GLuint left_pixel_size, const GLubyte* left_pixel_data, GLuint right_pixel_size,
						  const GLubyte* right_pixel_data)
{
	const GLuint pixel_size = (left_pixel_size >= right_pixel_size) ? left_pixel_size : right_pixel_size;

	return 0 == memcmp(left_pixel_data, right_pixel_data, pixel_size);
}

/** Delete texture or renderbuffer
 *
 * @param context Test context
 * @param target  Image target
 * @param name    Name of image
 **/
void Utils::deleteTexture(deqp::Context& context, GLenum target, GLuint name)
{
	const Functions& gl = context.getRenderContext().getFunctions();

	if (GL_RENDERBUFFER == target)
	{
		gl.deleteRenderbuffers(1, &name);
	}
	else
	{
		gl.deleteTextures(1, &name);
	}
}

/** Get epsilon for given internal_format
 *
 * @param internal_format Internal format of image
 *
 * @return Epsilon value
 **/
GLdouble Utils::getEpsilon(GLenum internal_format)
{
	GLdouble epsilon;

	switch (internal_format)
	{
	case GL_R8:
	case GL_R8_SNORM:
	case GL_R16:
	case GL_R16F:
	case GL_R16_SNORM:
	case GL_R32F:
	case GL_R8I:
	case GL_R8UI:
	case GL_R16I:
	case GL_R16UI:
	case GL_R32I:
	case GL_R32UI:
	case GL_RG8:
	case GL_RG8_SNORM:
	case GL_RG16:
	case GL_RG16F:
	case GL_RG16_SNORM:
	case GL_RG32F:
	case GL_RG8I:
	case GL_RG8UI:
	case GL_RG16I:
	case GL_RG16UI:
	case GL_RG32I:
	case GL_RG32UI:
	case GL_R3_G3_B2:
	case GL_RGB4:
	case GL_RGB5:
	case GL_RGB8:
	case GL_RGB8_SNORM:
	case GL_R11F_G11F_B10F:
	case GL_RGB16:
	case GL_RGB16F:
	case GL_RGB16_SNORM:
	case GL_RGB32F:
	case GL_RGB8I:
	case GL_RGB8UI:
	case GL_RGB10:
	case GL_RGB16I:
	case GL_RGB16UI:
	case GL_RGB32I:
	case GL_RGB32UI:
	case GL_RGB9_E5:
	case GL_RGBA2:
	case GL_RGBA4:
	case GL_RGB5_A1:
	case GL_RGBA8:
	case GL_RGBA8_SNORM:
	case GL_RGB10_A2:
	case GL_RGBA16:
	case GL_RGBA16F:
	case GL_RGBA16_SNORM:
	case GL_RGBA32F:
	case GL_RGBA8I:
	case GL_RGBA8UI:
	case GL_RGB10_A2UI:
	case GL_RGBA16I:
	case GL_RGBA16UI:
	case GL_RGBA32I:
	case GL_RGBA32UI:
		epsilon = 0.0;
		break;
	case GL_RGB12:
	case GL_RGBA12:
		epsilon = 0.00390625;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	return epsilon;
}

/** Get format for given internal format
 *
 * @param internal_format Internal format
 *
 * @return Format
 **/
GLenum Utils::getFormat(GLenum internal_format)
{
	GLenum format = 0;

	switch (internal_format)
	{
	/* R */
	case GL_R8:
	case GL_R8_SNORM:
	case GL_R16:
	case GL_R16F:
	case GL_R16_SNORM:
	case GL_R32F:
		format = GL_RED;
		break;

	case GL_R8I:
	case GL_R8UI:
	case GL_R16I:
	case GL_R16UI:
	case GL_R32I:
	case GL_R32UI:
		format = GL_RED_INTEGER;
		break;

	/* RG */
	case GL_RG8:
	case GL_RG8_SNORM:
	case GL_RG16:
	case GL_RG16F:
	case GL_RG16_SNORM:
	case GL_RG32F:
		format = GL_RG;
		break;

	case GL_RG8I:
	case GL_RG8UI:
	case GL_RG16I:
	case GL_RG16UI:
	case GL_RG32I:
	case GL_RG32UI:
		format = GL_RG_INTEGER;
		break;

	/* RGB */
	case GL_R3_G3_B2:
	case GL_RGB4:
	case GL_RGB5:
	case GL_RGB8:
	case GL_RGB8_SNORM:
	case GL_R11F_G11F_B10F:
	case GL_RGB12:
	case GL_RGB16:
	case GL_RGB16F:
	case GL_RGB16_SNORM:
	case GL_RGB32F:
	case GL_RGB9_E5:
		format = GL_RGB;
		break;

	case GL_RGB8I:
	case GL_RGB8UI:
	case GL_RGB16I:
	case GL_RGB16UI:
	case GL_RGB32I:
	case GL_RGB32UI:
		format = GL_RGB_INTEGER;
		break;

	/* RGBA */
	case GL_RGB10:
	case GL_RGBA2:
	case GL_RGBA4:
	case GL_RGB5_A1:
	case GL_RGBA8:
	case GL_RGBA8_SNORM:
	case GL_RGB10_A2:
	case GL_RGBA12:
	case GL_RGBA16:
	case GL_RGBA16F:
	case GL_RGBA16_SNORM:
	case GL_RGBA32F:
		format = GL_RGBA;
		break;

	case GL_RGBA8I:
	case GL_RGBA8UI:
	case GL_RGB10_A2UI:
	case GL_RGBA16I:
	case GL_RGBA16UI:
	case GL_RGBA32I:
	case GL_RGBA32UI:
		format = GL_RGBA_INTEGER;
		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	return format;
}

/** Get number of channels for given internal_format
 *
 * @param internal_format Internal format
 *
 * @return Number of channels
 **/
GLuint Utils::getNumberOfChannels(GLenum internal_format)
{
	GLuint result = 0;

	switch (internal_format)
	{
	case GL_R8:
	case GL_R8_SNORM:
	case GL_R16:
	case GL_R16F:
	case GL_R16_SNORM:
	case GL_R32F:
	case GL_R8I:
	case GL_R8UI:
	case GL_R16I:
	case GL_R16UI:
	case GL_R32I:
	case GL_R32UI:
		result = 1;
		break;

	case GL_RG8:
	case GL_RG8_SNORM:
	case GL_RG16:
	case GL_RG16F:
	case GL_RG16_SNORM:
	case GL_RG32F:
	case GL_RG8I:
	case GL_RG8UI:
	case GL_RG16I:
	case GL_RG16UI:
	case GL_RG32I:
	case GL_RG32UI:
		result = 2;
		break;

	case GL_R3_G3_B2:
	case GL_RGB4:
	case GL_RGB5:
	case GL_RGB8:
	case GL_RGB8_SNORM:
	case GL_RGB10:
	case GL_R11F_G11F_B10F:
	case GL_RGB12:
	case GL_RGB16:
	case GL_RGB16F:
	case GL_RGB16_SNORM:
	case GL_RGB32F:
	case GL_RGB9_E5:
	case GL_RGB8I:
	case GL_RGB8UI:
	case GL_RGB16I:
	case GL_RGB16UI:
	case GL_RGB32I:
	case GL_RGB32UI:
		result = 3;
		break;

	case GL_RGBA2:
	case GL_RGBA4:
	case GL_RGB5_A1:
	case GL_RGBA8:
	case GL_RGBA8_SNORM:
	case GL_RGB10_A2:
	case GL_RGBA12:
	case GL_RGBA16:
	case GL_RGBA16F:
	case GL_RGBA16_SNORM:
	case GL_RGBA32F:
	case GL_RGBA8I:
	case GL_RGBA8UI:
	case GL_RGB10_A2UI:
	case GL_RGBA16I:
	case GL_RGBA16UI:
	case GL_RGBA32I:
	case GL_RGBA32UI:
		result = 4;
		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	return result;
}

/** Get type for given internal format
 *
 * @param internal_format Internal format
 *
 * @return Type
 **/
GLenum Utils::getType(GLenum internal_format)
{
	GLenum type = 0;

	switch (internal_format)
	{
	case GL_R8:
	case GL_R8UI:
	case GL_RG8:
	case GL_RG8UI:
	case GL_RGB8:
	case GL_RGB8UI:
	case GL_RGBA8:
	case GL_RGBA8UI:
		type = GL_UNSIGNED_BYTE;
		break;

	case GL_R8_SNORM:
	case GL_R8I:
	case GL_RG8_SNORM:
	case GL_RG8I:
	case GL_RGB8_SNORM:
	case GL_RGB8I:
	case GL_RGBA8_SNORM:
	case GL_RGBA8I:
		type = GL_BYTE;
		break;

	case GL_R3_G3_B2:
		type = GL_UNSIGNED_BYTE_3_3_2;
		break;

	case GL_RGB4:
	case GL_RGB5:
		type = GL_UNSIGNED_SHORT_5_6_5;
		break;

	case GL_RGBA2:
	case GL_RGBA4:
		type = GL_UNSIGNED_SHORT_4_4_4_4;
		break;

	case GL_RGB5_A1:
		type = GL_UNSIGNED_SHORT_5_5_5_1;
		break;

	case GL_RGB10:
	case GL_RGB10_A2:
	case GL_RGB10_A2UI:
		type = GL_UNSIGNED_INT_2_10_10_10_REV;
		break;

	case GL_R16F:
	case GL_RG16F:
	case GL_RGB16F:
	case GL_RGBA16F:
		type = GL_HALF_FLOAT;
		break;

	case GL_R16:
	case GL_R16UI:
	case GL_RG16:
	case GL_RG16UI:
	case GL_RGB12:
	case GL_RGB16:
	case GL_RGB16UI:
	case GL_RGBA12:
	case GL_RGBA16:
	case GL_RGBA16UI:
		type = GL_UNSIGNED_SHORT;
		break;

	case GL_R16_SNORM:
	case GL_R16I:
	case GL_RG16_SNORM:
	case GL_RG16I:
	case GL_RGB16_SNORM:
	case GL_RGB16I:
	case GL_RGBA16_SNORM:
	case GL_RGBA16I:
		type = GL_SHORT;
		break;

	case GL_R32UI:
	case GL_RG32UI:
	case GL_RGB32UI:
	case GL_RGBA32UI:
		type = GL_UNSIGNED_INT;
		break;

	case GL_RGB9_E5:
		type = GL_UNSIGNED_INT_5_9_9_9_REV;
		break;

	case GL_R32I:
	case GL_RG32I:
	case GL_RGB32I:
	case GL_RGBA32I:
		type = GL_INT;
		break;

	case GL_R32F:
	case GL_RG32F:
	case GL_RGB32F:
	case GL_RGBA32F:
		type = GL_FLOAT;
		break;

	case GL_R11F_G11F_B10F:
		type = GL_UNSIGNED_INT_10F_11F_11F_REV;
		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	return type;
}

/** Returns mask that should be applied to pixel value
 *
 * @param internal_format Internal format of texture
 * @param pixel           Pixel data
 *
 * @return Mask
 **/
void Utils::maskPixelForFormat(GLenum internal_format, GLubyte* pixel)
{
	switch (internal_format)
	{
	case GL_RGB10:
		/* UINT_10_10_10_2 - ALPHA will be set to 3*/
		pixel[0] |= 0x03;
		break;

	default:
		break;
	}
}

/** Get size of pixel for given internal format
 *
 * @param internal_format Internal format
 *
 * @return Number of bytes used by given format
 **/
GLuint Utils::getPixelSizeForFormat(GLenum internal_format)
{
	GLuint size = 0;

	switch (internal_format)
	{
	/* 8 */
	case GL_R8:
	case GL_R8I:
	case GL_R8UI:
	case GL_R8_SNORM:
	case GL_R3_G3_B2:
		size = 1;
		break;

	/* 8 */
	case GL_RGBA2:
		size = 2;
		break;

	/* 12 */
	case GL_RGB4:
		size = 2;
		break;

	/* 15 */
	case GL_RGB5:
		size = 2;
		break;

	/* 16 */
	case GL_RG8:
	case GL_RG8I:
	case GL_RG8UI:
	case GL_RG8_SNORM:
	case GL_R16:
	case GL_R16F:
	case GL_R16I:
	case GL_R16UI:
	case GL_R16_SNORM:
	case GL_RGBA4:
	case GL_RGB5_A1:
		size = 2;
		break;

	/* 24 */
	case GL_RGB8:
	case GL_RGB8I:
	case GL_RGB8UI:
	case GL_RGB8_SNORM:
		size = 3;
		break;

	/* 30 */
	case GL_RGB10:
		size = 4;
		break;

	/* 32 */
	case GL_RGBA8:
	case GL_RGBA8I:
	case GL_RGBA8UI:
	case GL_RGBA8_SNORM:
	case GL_RG16:
	case GL_RG16F:
	case GL_RG16I:
	case GL_RG16UI:
	case GL_RG16_SNORM:
	case GL_R32F:
	case GL_R32I:
	case GL_R32UI:
	case GL_RGB10_A2:
	case GL_RGB10_A2UI:
	case GL_R11F_G11F_B10F:
	case GL_RGB9_E5:
		size = 4;
		break;

	/* 36 */
	case GL_RGB12:
		size = 6;
		break;

	/* 48 */
	case GL_RGB16:
	case GL_RGB16F:
	case GL_RGB16I:
	case GL_RGB16UI:
	case GL_RGB16_SNORM:
		size = 6;
		break;

	/* 64 */
	case GL_RGBA12:
	case GL_RGBA16:
	case GL_RGBA16F:
	case GL_RGBA16I:
	case GL_RGBA16UI:
	case GL_RGBA16_SNORM:
	case GL_RG32F:
	case GL_RG32I:
	case GL_RG32UI:
		size = 8;
		break;

	/* 96 */
	case GL_RGB32F:
	case GL_RGB32I:
	case GL_RGB32UI:
		size = 12;
		break;

	/* 128 */
	case GL_RGBA32F:
	case GL_RGBA32I:
	case GL_RGBA32UI:
		size = 16;
		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	return size;
}

/** Prepare string that represents bytes of pixel
 *
 * @param internal_format Format
 * @param pixel           Pixel data
 *
 * @return String
 **/
std::string Utils::getPixelString(GLenum internal_format, const GLubyte* pixel)
{
	const GLuint	  pixel_size = Utils::getPixelSizeForFormat(internal_format);
	std::stringstream stream;

	stream << "0x";

	for (GLint i = pixel_size - 1; i >= 0; --i)
	{
		stream << std::setbase(16) << std::setw(2) << std::setfill('0') << (GLuint)pixel[i];
	}

	return stream.str();
}

/** Check if target supports multiple layers
 *
 * @param target Texture target
 *
 * @return true if target is multilayered
 **/
bool Utils::isTargetMultilayer(GLenum target)
{
	bool result = false;

	switch (target)
	{
	case GL_TEXTURE_1D_ARRAY:
	case GL_TEXTURE_2D_ARRAY:
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
	case GL_TEXTURE_3D:
	case GL_TEXTURE_CUBE_MAP_ARRAY:
		result = true;
		break;

	default:
		break;
	}

	return result;
}

/** Check if target supports multiple level
 *
 * @param target Texture target
 *
 * @return true if target supports mipmaps
 **/
bool Utils::isTargetMultilevel(GLenum target)
{
	bool result = true;

	switch (target)
	{
	case GL_TEXTURE_2D_MULTISAMPLE:
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
	case GL_TEXTURE_RECTANGLE:
	case GL_RENDERBUFFER:
		result = false;
		break;
	default:
		break;
	}

	return result;
}

/** Check if target is multisampled
 *
 * @param target Texture target
 *
 * @return true when for multisampled formats, false otherwise
 **/
bool Utils::isTargetMultisampled(GLenum target)
{
	bool result = false;

	switch (target)
	{
	case GL_TEXTURE_2D_MULTISAMPLE:
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		result = true;
		break;
	default:
		break;
	}

	return result;
}

/** Generate texture object
 *
 * @param context Test context
 * @param target  Target of texture
 *
 * @return Generated name
 **/
glw::GLuint Utils::generateTexture(deqp::Context& context, GLenum target)
{
	const Functions& gl   = context.getRenderContext().getFunctions();
	GLuint			 name = 0;

	switch (target)
	{
	case GL_RENDERBUFFER:
		gl.genRenderbuffers(1, &name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GenRenderbuffers");
		break;

	default:
		gl.genTextures(1, &name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");
		break;
	}

	return name;
}

/** Sets base and max level parameters of texture to make it complete
 *
 * @param context    Test context
 * @param target     GLenum representing target of texture that should be created
 * @param id         Id of texture
 * @param base_level Base level value, eg 0
 * @param max_level  Max level value, eg 0
 **/
void Utils::makeTextureComplete(deqp::Context& context, GLenum target, GLuint id, GLint base_level, GLint max_level)
{
	const Functions& gl = context.getRenderContext().getFunctions();

	if (GL_RENDERBUFFER == target)
	{
		return;
	}

	/* Translate proxies into real targets */
	target = transRealToBindTarget(transProxyToRealTarget(target));

	gl.bindTexture(target, id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	/* Set levels */
	if (GL_TEXTURE_BUFFER != target)
	{
		gl.texParameteri(target, GL_TEXTURE_BASE_LEVEL, base_level);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");

		gl.texParameteri(target, GL_TEXTURE_MAX_LEVEL, max_level);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");

		/* Integer textures won't be complete with the default min filter
		 * of GL_NEAREST_MIPMAP_LINEAR (or GL_LINEAR for rectangle textures)
		 * and default mag filter of GL_LINEAR, so switch to nearest.
		 */
		if (GL_TEXTURE_2D_MULTISAMPLE != target && GL_TEXTURE_2D_MULTISAMPLE_ARRAY != target)
		{
			gl.texParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			if (GL_TEXTURE_RECTANGLE != target)
			{
				gl.texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
				GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");
			}
			else
			{
				gl.texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");
			}
		}
	}

	/* Clean binding point */
	gl.bindTexture(target, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
}

/** Generate and initialize texture for given target
 *
 * @param context   Test context
 * @param target    GLenum representing target of texture that should be created
 * @param n_samples Number of samples
 *
 * @return "name" of texture
 **/
GLuint Utils::prepareMultisampleTex(deqp::Context& context, GLenum target, GLsizei n_samples)
{
	static const GLuint depth			= 6;
	const Functions&	gl				= context.getRenderContext().getFunctions();
	static const GLuint height			= 16;
	static const GLenum internal_format = GL_RGBA8;
	GLuint				name			= 0;
	static const GLuint width			= 16;

	gl.genTextures(1, &name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	/* Initialize */
	switch (target)
	{
	case GL_TEXTURE_2D_MULTISAMPLE:
		gl.bindTexture(target, name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		gl.texImage2DMultisample(target, n_samples, internal_format, width, height, GL_FALSE /* fixedsamplelocation */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexImage2DMultisample");

		break;

	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		gl.bindTexture(target, name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		gl.texImage3DMultisample(target, n_samples, internal_format, width, height, depth,
								 GL_FALSE /* fixedsamplelocation */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexImage3DMultisample");

		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	/* Clean binding point */
	gl.bindTexture(target, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	return name;
}

/** Generate and initialize texture for given target
 *
 * @param context         Test context
 * @param internal_format Internal format of render buffer
 *
 * @return "name" of texture
 **/
GLuint Utils::prepareRenderBuffer(deqp::Context& context, GLenum internal_format)
{
	const Functions&	gl	 = context.getRenderContext().getFunctions();
	static const GLuint height = 16;
	GLuint				name   = 0;
	static const GLuint width  = 16;

	gl.genRenderbuffers(1, &name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenRenderbuffers");

	/* Initialize */
	gl.bindRenderbuffer(GL_RENDERBUFFER, name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindRenderbuffer");

	gl.renderbufferStorage(GL_RENDERBUFFER, internal_format, width, height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "RenderbufferStorage");

	/* Clean binding point */
	gl.bindRenderbuffer(GL_RENDERBUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindRenderbuffer");

	return name;
}

/** Generate and initialize texture for given target
 *
 * @param context         Test context
 * @param target          GLenum representing target of texture that should be created
 * @param internal_format <internalformat>
 * @param format          <format>
 * @param type            <type>
 * @param out_buf_id      ID of buffer that will be used for TEXTURE_BUFFER
 *
 * @return "name" of texture
 **/
GLuint Utils::prepareTex16x16x6(deqp::Context& context, GLenum target, GLenum internal_format, GLenum format,
								GLenum type, GLuint& out_buf_id)
{
	static const GLuint  depth  = 6;
	static const GLuint  height = 16;
	static const GLuint  level  = 0;
	GLuint				 name   = 0;
	static const GLchar* pixels = 0;
	static const GLuint  width  = 16;

	name = generateTexture(context, target);

	prepareTexture(context, name, target, internal_format, format, type, level, width, height, depth, pixels,
				   out_buf_id);

	return name;
}

/** Initialize texture
 *
 * @param context         Test context
 * @param name            Name of texture object
 * @param target          GLenum representing target of texture that should be created
 * @param internal_format <internalformat>
 * @param format          <format>
 * @param type            <type>
 * @param level           <level>
 * @param width           <width>
 * @param height          <height>
 * @param depth           <depth>
 * @param pixels          <pixels>
 * @param out_buf_id      ID of buffer that will be used for TEXTURE_BUFFER
 *
 * @return "name" of texture
 **/
void Utils::prepareTexture(deqp::Context& context, GLuint name, GLenum target, GLenum internal_format, GLenum format,
						   GLenum type, GLuint level, GLuint width, GLuint height, GLuint depth, const GLvoid* pixels,
						   GLuint& out_buf_id)
{
	static const GLint   border		   = 0;
	GLenum				 error		   = 0;
	const GLchar*		 function_name = "unknown";
	const Functions&	 gl			   = context.getRenderContext().getFunctions();
	static const GLsizei samples	   = 1;

	/* Translate proxies into real targets */
	target = transProxyToRealTarget(target);

	/* Initialize */
	switch (target)
	{
	case GL_RENDERBUFFER:
		gl.bindRenderbuffer(target, name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindRenderbuffer");

		gl.renderbufferStorage(target, internal_format, width, height);
		GLU_EXPECT_NO_ERROR(gl.getError(), "RenderbufferStorage");

		gl.bindRenderbuffer(target, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindRenderbuffer");

		break;

	case GL_TEXTURE_1D:
		gl.bindTexture(target, name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		gl.texImage1D(target, level, internal_format, width, border, format, type, pixels);
		error		  = gl.getError();
		function_name = "TexImage1D";

		break;

	case GL_TEXTURE_1D_ARRAY:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_RECTANGLE:
		gl.bindTexture(target, name);

		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		gl.texImage2D(target, level, internal_format, width, height, border, format, type, pixels);
		error		  = gl.getError();
		function_name = "TexImage2D";

		break;

	case GL_TEXTURE_2D_MULTISAMPLE:
		gl.bindTexture(target, name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		gl.texImage2DMultisample(target, samples, internal_format, width, height, GL_FALSE /* fixedsamplelocation */);
		error		  = gl.getError();
		function_name = "TexImage2DMultisample";

		break;

	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		gl.bindTexture(target, name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		gl.texImage3DMultisample(target, samples, internal_format, width, height, depth,
								 GL_FALSE /* fixedsamplelocation */);
		error		  = gl.getError();
		function_name = "TexImage3DMultisample";

		break;

	case GL_TEXTURE_2D_ARRAY:
	case GL_TEXTURE_3D:
	case GL_TEXTURE_CUBE_MAP_ARRAY:
		gl.bindTexture(target, name);

		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		gl.texImage3D(target, level, internal_format, width, height, depth, border, format, type, pixels);
		error		  = gl.getError();
		function_name = "TexImage3D";

		break;

	case GL_TEXTURE_BUFFER:
		gl.genBuffers(1, &out_buf_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GenBuffers");

		gl.bindBuffer(GL_TEXTURE_BUFFER, out_buf_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

		{
			GLsizei		  size = 16;
			const GLvoid* data = 0;

			if (0 != pixels)
			{
				size = width;
				data = pixels;
			}

			gl.bufferData(GL_TEXTURE_BUFFER, size, data, GL_DYNAMIC_COPY);
			GLU_EXPECT_NO_ERROR(gl.getError(), "BufferData");
		}

		gl.bindTexture(GL_TEXTURE_BUFFER, name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		gl.texBuffer(GL_TEXTURE_BUFFER, internal_format, out_buf_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexBuffer");

		break;

	case GL_TEXTURE_CUBE_MAP:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
		/* Change target to CUBE_MAP, it will be used later to change base and max level */
		target = GL_TEXTURE_CUBE_MAP;
		gl.bindTexture(target, name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		gl.texImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, level, internal_format, width, height, border, format, type,
					  pixels);
		gl.texImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, level, internal_format, width, height, border, format, type,
					  pixels);
		gl.texImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, level, internal_format, width, height, border, format, type,
					  pixels);
		gl.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, level, internal_format, width, height, border, format, type,
					  pixels);
		gl.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, level, internal_format, width, height, border, format, type,
					  pixels);
		gl.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, level, internal_format, width, height, border, format, type,
					  pixels);
		error		  = gl.getError();
		function_name = "TexImage2D";

		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	if (GL_NO_ERROR != error)
	{
		context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Error: " << glu::getErrorStr(error) << ". Function: " << function_name
			<< ". Target: " << glu::getTextureTargetStr(target)
			<< ". Format: " << glu::getInternalFormatParameterStr(internal_format) << ", "
			<< glu::getTextureFormatName(format) << ", " << glu::getTypeStr(type) << tcu::TestLog::EndMessage;
		TCU_FAIL("Failed to create texture");
	}

	if (GL_RENDERBUFFER != target)
	{
		/* Clean binding point */
		gl.bindTexture(target, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
	}
}

/** Translate proxies into real targets
 *
 * @param target Target to be converted
 *
 * @return Converted target for proxies, <target> otherwise
 **/
GLenum Utils::transProxyToRealTarget(GLenum target)
{
	switch (target)
	{
	case GL_PROXY_TEXTURE_1D:
		target = GL_TEXTURE_1D;
		break;
	case GL_PROXY_TEXTURE_1D_ARRAY:
		target = GL_TEXTURE_1D_ARRAY;
		break;
	case GL_PROXY_TEXTURE_2D:
		target = GL_TEXTURE_2D;
		break;
	case GL_PROXY_TEXTURE_2D_ARRAY:
		target = GL_TEXTURE_2D_ARRAY;
		break;
	case GL_PROXY_TEXTURE_2D_MULTISAMPLE:
		target = GL_TEXTURE_2D_MULTISAMPLE;
		break;
	case GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
		target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
		break;
	case GL_PROXY_TEXTURE_3D:
		target = GL_TEXTURE_3D;
		break;
	case GL_PROXY_TEXTURE_CUBE_MAP:
		target = GL_TEXTURE_CUBE_MAP;
		break;
	case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
		target = GL_TEXTURE_CUBE_MAP_ARRAY;
		break;
	case GL_PROXY_TEXTURE_RECTANGLE:
		target = GL_TEXTURE_RECTANGLE;
		break;
	default:
		break;
	}

	return target;
}

/** Translate real targets into binding targets
 *
 * @param target Target to be converted
 *
 * @return Converted target for cube map faces, <target> otherwise
 **/
GLenum Utils::transRealToBindTarget(GLenum target)
{
	switch (target)
	{
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
		target = GL_TEXTURE_CUBE_MAP;
		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
		target = GL_TEXTURE_CUBE_MAP;
		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
		target = GL_TEXTURE_CUBE_MAP;
		break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
		target = GL_TEXTURE_CUBE_MAP;
		break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
		target = GL_TEXTURE_CUBE_MAP;
		break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
		target = GL_TEXTURE_CUBE_MAP;
		break;
	default:
		break;
	}

	return target;
}

/** Read value of channel
 *
 * @tparam T Type used to store channel value
 *
 * @param channel   Channel index
 * @param pixel     Pixel data
 * @param out_value Read value
 **/
template <typename T>
void readBaseTypeFromUnsignedChannel(GLuint channel, const GLubyte* pixel, GLdouble& out_value)
{
	static const T max = -1;

	const GLdouble d_max   = (GLdouble)max;
	const T*	   ptr	 = (T*)pixel;
	const T		   t_value = ptr[channel];
	const GLdouble d_value = (GLdouble)t_value;

	out_value = d_value / d_max;
}

/** Read value of channel
 *
 * @tparam T Type used to store channel value
 *
 * @param channel   Channel index
 * @param pixel     Pixel data
 * @param out_value Read value
 **/
template <typename T>
void readBaseTypeFromSignedChannel(GLuint channel, const GLubyte* pixel, GLdouble& out_value)
{
	static const GLuint n_bytes = sizeof(T);
	static const GLuint n_bits  = 8u * n_bytes;
	static const T		max		= (T)((1u << (n_bits - 1u)) - 1u);

	const GLdouble d_max   = (GLdouble)max;
	const T*	   ptr	 = (T*)pixel;
	const T		   t_value = ptr[channel];
	const GLdouble d_value = (GLdouble)t_value;

	out_value = d_value / d_max;
}

/** Read value of channel
 *
 * @tparam T Type used to store channel value
 *
 * @param channel   Channel index
 * @param pixel     Pixel data
 * @param out_value Read value
 **/
void readBaseTypeFromFloatChannel(GLuint channel, const GLubyte* pixel, GLdouble& out_value)
{
	const GLfloat* ptr	 = (const GLfloat*)pixel;
	const GLfloat  t_value = ptr[channel];
	const GLdouble d_value = (GLdouble)t_value;

	out_value = d_value;
}

/** Read value of channel
 *
 * @tparam T Type used to store channel value
 *
 * @param channel   Channel index
 * @param pixel     Pixel data
 * @param out_value Read value
 **/
void readBaseTypeFromHalfFloatChannel(GLuint channel, const GLubyte* pixel, GLdouble& out_value)
{
	const deUint16* ptr  = (const deUint16*)pixel;
	const deUint16  bits = ptr[channel];
	tcu::Float16	val(bits);
	const GLdouble  d_value = val.asDouble();

	out_value = d_value;
}

/** Read value of channel
 *
 * @tparam T      Type used to store pixel
 * @tparam size_1 Size of channel in bits
 * @tparam size_2 Size of channel in bits
 * @tparam size_3 Size of channel in bits
 * @tparam off_1  Offset of channel in bits
 * @tparam off_2  Offset of channel in bits
 * @tparam off_3  Offset of channel in bits
 *
 * @param channel   Channel index
 * @param pixel     Pixel data
 * @param out_value Read value
 **/
template <typename T, unsigned int size_1, unsigned int size_2, unsigned int size_3, unsigned int off_1,
		  unsigned int off_2, unsigned int off_3>
void read3Channel(GLuint channel, const GLubyte* pixel, GLdouble& out_value)
{
	T		 mask	= 0;
	T		 max	 = 0;
	T		 off	 = 0;
	const T* ptr	 = (T*)pixel;
	T		 result  = 0;
	const T  t_value = ptr[0];

	static const T max_1 = (1 << size_1) - 1;
	static const T max_2 = (1 << size_2) - 1;
	static const T max_3 = (1 << size_3) - 1;

	switch (channel)
	{
	case 0:
		mask = max_1;
		max  = max_1;
		off  = off_1;
		break;
	case 1:
		mask = max_2;
		max  = max_2;
		off  = off_2;
		break;
	case 2:
		mask = max_3;
		max  = max_3;
		off  = off_3;
		break;
	default:
		TCU_FAIL("Invalid channel");
		break;
	}

	result = (T)((t_value >> off) & mask);

	const GLdouble d_max   = (GLdouble)max;
	const GLdouble d_value = (GLdouble)result;

	out_value = d_value / d_max;
}

/** Read value of channel
 *
 * @tparam T      Type used to store pixel
 * @tparam size_1 Size of channel in bits
 * @tparam size_2 Size of channel in bits
 * @tparam size_3 Size of channel in bits
 * @tparam size_4 Size of channel in bits
 * @tparam off_1  Offset of channel in bits
 * @tparam off_2  Offset of channel in bits
 * @tparam off_3  Offset of channel in bits
 * @tparam off_4  Offset of channel in bits
 *
 * @param channel   Channel index
 * @param pixel     Pixel data
 * @param out_value Read value
 **/
template <typename T, unsigned int size_1, unsigned int size_2, unsigned int size_3, unsigned int size_4,
		  unsigned int off_1, unsigned int off_2, unsigned int off_3, unsigned int off_4>
void read4Channel(GLuint channel, const GLubyte* pixel, GLdouble& out_value)
{
	T		 mask	= 0;
	T		 max	 = 0;
	T		 off	 = 0;
	const T* ptr	 = (T*)pixel;
	T		 result  = 0;
	const T  t_value = ptr[0];

	static const T max_1 = (1 << size_1) - 1;
	static const T max_2 = (1 << size_2) - 1;
	static const T max_3 = (1 << size_3) - 1;
	static const T max_4 = (1 << size_4) - 1;

	switch (channel)
	{
	case 0:
		mask = max_1;
		max  = max_1;
		off  = off_1;
		break;
	case 1:
		mask = max_2;
		max  = max_2;
		off  = off_2;
		break;
	case 2:
		mask = max_3;
		max  = max_3;
		off  = off_3;
		break;
	case 3:
		mask = max_4;
		max  = max_4;
		off  = off_4;
		break;
	default:
		TCU_FAIL("Invalid channel");
		break;
	}

	result = (T)((t_value >> off) & mask);

	const GLdouble d_max   = (GLdouble)max;
	const GLdouble d_value = (GLdouble)result;

	out_value = d_value / d_max;
}

/** Read value of channel
 *
 * @param channel   Channel index
 * @param pixel     Pixel data
 * @param out_value Read value
 **/
void read11F_11F_10F_Channel(GLuint channel, const GLubyte* pixel, GLdouble& out_value)
{
	const deUint32* ptr = (deUint32*)pixel;
	deUint32		val = *ptr;

	switch (channel)
	{
	case 0:
	{
		deUint32 bits = (val & 0x000007ff);
		tcu::Float<deUint32, 5, 6, 15, tcu::FLOAT_SUPPORT_DENORM> temp_val(bits);

		out_value = temp_val.asDouble();
	}
	break;
	case 1:
	{
		deUint32 bits = ((val >> 11) & 0x000007ff);
		tcu::Float<deUint32, 5, 6, 15, tcu::FLOAT_SUPPORT_DENORM> temp_val(bits);

		out_value = temp_val.asDouble();
	}
	break;
	case 2:
	{
		deUint32 bits = ((val >> 22) & 0x000003ff);
		tcu::Float<deUint32, 5, 5, 15, tcu::FLOAT_SUPPORT_DENORM> temp_val(bits);

		out_value = temp_val.asDouble();
	}
	break;
	default:
		TCU_FAIL("Invalid channel");
		break;
	}
}

/** Write value of channel
 *
 * @tparam T Type used to store pixel
 *
 * @param channel Channel index
 * @param value   Value to write
 * @param pixel   Pixel data
 **/
template <typename T>
void writeBaseTypeToUnsignedChannel(GLuint channel, GLdouble value, GLubyte* pixel)
{
	static const T max = -1;

	const GLdouble d_max   = (GLdouble)max;
	const GLdouble d_value = value * d_max;
	const T		   t_value = (T)d_value;

	T* ptr = (T*)pixel;

	ptr[channel] = t_value;
}

/** Write value of channel
 *
 * @tparam T Type used to store pixel
 *
 * @param channel Channel index
 * @param value   Value to write
 * @param pixel   Pixel data
 **/
template <typename T>
void writeBaseTypeToSignedChannel(GLuint channel, GLdouble value, GLubyte* pixel)
{
	static const GLuint n_bytes = sizeof(T);
	static const GLuint n_bits  = 8u * n_bytes;
	static const T		max		= (T)((1u << (n_bits - 1u)) - 1u);

	const GLdouble d_max   = (GLdouble)max;
	const GLdouble d_value = value * d_max;
	const T		   t_value = (T)d_value;

	T* ptr = (T*)pixel;

	ptr[channel] = t_value;
}

/** Write value of channel
 *
 * @param channel Channel index
 * @param value   Value to write
 * @param pixel   Pixel data
 **/
void writeBaseTypeToFloatChannel(GLuint channel, GLdouble value, GLubyte* pixel)
{
	const GLfloat t_value = (GLfloat)value;

	GLfloat* ptr = (GLfloat*)pixel;

	ptr[channel] = t_value;
}

/** Write value of channel
 *
 * @param channel Channel index
 * @param value   Value to write
 * @param pixel   Pixel data
 **/
void writeBaseTypeToHalfFloatChannel(GLuint channel, GLdouble value, GLubyte* pixel)
{
	deUint16* ptr = (deUint16*)pixel;

	tcu::Float16 val(value);

	ptr[channel] = val.bits();
}

/** Write value of channel
 *
 * @tparam T      Type used to store pixel
 * @tparam size_1 Size of channel in bits
 * @tparam size_2 Size of channel in bits
 * @tparam size_3 Size of channel in bits
 * @tparam off_1  Offset of channel in bits
 * @tparam off_2  Offset of channel in bits
 * @tparam off_3  Offset of channel in bits
 *
 * @param channel Channel index
 * @param value   Value to write
 * @param pixel   Pixel data
 **/
template <typename T, unsigned int size_1, unsigned int size_2, unsigned int size_3, unsigned int off_1,
		  unsigned int off_2, unsigned int off_3>
void write3Channel(GLuint channel, GLdouble value, GLubyte* pixel)
{
	T  mask   = 0;
	T  max	= 0;
	T  off	= 0;
	T* ptr	= (T*)pixel;
	T  result = 0;

	static const T max_1 = (1 << size_1) - 1;
	static const T max_2 = (1 << size_2) - 1;
	static const T max_3 = (1 << size_3) - 1;

	switch (channel)
	{
	case 0:
		mask = max_1;
		max  = max_1;
		off  = off_1;
		break;
	case 1:
		mask = max_2;
		max  = max_2;
		off  = off_2;
		break;
	case 2:
		mask = max_3;
		max  = max_3;
		off  = off_3;
		break;
	default:
		TCU_FAIL("Invalid channel");
		break;
	}

	const GLdouble d_max   = (GLdouble)max;
	const GLdouble d_value = value * d_max;
	const T		   t_value = (T)d_value;

	result = (T)((t_value & mask) << off);

	*ptr |= result;
}

/** Write value of channel
 *
 * @tparam T      Type used to store pixel
 * @tparam size_1 Size of channel in bits
 * @tparam size_2 Size of channel in bits
 * @tparam size_3 Size of channel in bits
 * @tparam size_4 Size of channel in bits
 * @tparam off_1  Offset of channel in bits
 * @tparam off_2  Offset of channel in bits
 * @tparam off_3  Offset of channel in bits
 * @tparam off_4  Offset of channel in bits
 *
 * @param channel Channel index
 * @param value   Value to write
 * @param pixel   Pixel data
 **/
template <typename T, unsigned int size_1, unsigned int size_2, unsigned int size_3, unsigned int size_4,
		  unsigned int off_1, unsigned int off_2, unsigned int off_3, unsigned int off_4>
void write4Channel(GLuint channel, GLdouble value, GLubyte* pixel)
{
	T  mask   = 0;
	T  max	= 0;
	T  off	= 0;
	T* ptr	= (T*)pixel;
	T  result = 0;

	static const T max_1 = (1 << size_1) - 1;
	static const T max_2 = (1 << size_2) - 1;
	static const T max_3 = (1 << size_3) - 1;
	static const T max_4 = (1 << size_4) - 1;

	switch (channel)
	{
	case 0:
		mask = max_1;
		max  = max_1;
		off  = off_1;
		break;
	case 1:
		mask = max_2;
		max  = max_2;
		off  = off_2;
		break;
	case 2:
		mask = max_3;
		max  = max_3;
		off  = off_3;
		break;
	case 3:
		mask = max_4;
		max  = max_4;
		off  = off_4;
		break;
	default:
		TCU_FAIL("Invalid channel");
		break;
	}

	const GLdouble d_max   = (GLdouble)max;
	const GLdouble d_value = value * d_max;
	const T		   t_value = (T)d_value;

	result = (T)((t_value & mask) << off);

	*ptr |= result;
}

/** Write value of channel
 *
 * @param channel Channel index
 * @param value   Value to write
 * @param pixel   Pixel data
 **/
void write11F_11F_10F_Channel(GLuint channel, GLdouble value, GLubyte* pixel)
{
	deUint32* ptr = (deUint32*)pixel;

	switch (channel)
	{
	case 0:
	{
		tcu::Float<deUint32, 5, 6, 15, tcu::FLOAT_SUPPORT_DENORM> val(value);
		deUint32 bits = val.bits();

		*ptr |= bits;
	}
	break;
	case 1:
	{
		tcu::Float<deUint32, 5, 6, 15, tcu::FLOAT_SUPPORT_DENORM> val(value);
		deUint32 bits = val.bits();

		*ptr |= (bits << 11);
	}
	break;
	case 2:
	{
		tcu::Float<deUint32, 5, 5, 15, tcu::FLOAT_SUPPORT_DENORM> val(value);
		deUint32 bits = val.bits();

		*ptr |= (bits << 22);
	}
	break;
	default:
		TCU_FAIL("Invalid channel");
		break;
	}
}

/** Read value of channel
 *
 * @param type    Type used by pixel
 * @param channel Channel index
 * @param pixel   Pixel data
 * @param value   Read value
 **/
void Utils::readChannel(GLenum type, GLuint channel, const GLubyte* pixel, GLdouble& value)
{
	switch (type)
	{
	/* Base types */
	case GL_UNSIGNED_BYTE:
		readBaseTypeFromUnsignedChannel<GLubyte>(channel, pixel, value);
		break;
	case GL_UNSIGNED_SHORT:
		readBaseTypeFromUnsignedChannel<GLushort>(channel, pixel, value);
		break;
	case GL_UNSIGNED_INT:
		readBaseTypeFromUnsignedChannel<GLuint>(channel, pixel, value);
		break;
	case GL_BYTE:
		readBaseTypeFromSignedChannel<GLbyte>(channel, pixel, value);
		break;
	case GL_SHORT:
		readBaseTypeFromSignedChannel<GLshort>(channel, pixel, value);
		break;
	case GL_INT:
		readBaseTypeFromSignedChannel<GLint>(channel, pixel, value);
		break;
	case GL_HALF_FLOAT:
		readBaseTypeFromHalfFloatChannel(channel, pixel, value);
		break;
	case GL_FLOAT:
		readBaseTypeFromFloatChannel(channel, pixel, value);
		break;

	/* Complicated */
	/* 3 channles */
	case GL_UNSIGNED_BYTE_3_3_2:
		read3Channel<GLubyte, 3, 3, 2, 5, 2, 0>(channel, pixel, value);
		break;
	case GL_UNSIGNED_SHORT_5_6_5:
		read3Channel<GLushort, 5, 6, 5, 11, 5, 0>(channel, pixel, value);
		break;

	/* 4 channels */
	case GL_UNSIGNED_SHORT_4_4_4_4:
		read4Channel<GLushort, 4, 4, 4, 4, 12, 8, 4, 0>(channel, pixel, value);
		break;
	case GL_UNSIGNED_SHORT_5_5_5_1:
		read4Channel<GLushort, 5, 5, 5, 1, 11, 6, 1, 0>(channel, pixel, value);
		break;
	case GL_UNSIGNED_INT_2_10_10_10_REV:
		read4Channel<GLuint, 2, 10, 10, 10, 30, 20, 10, 0>(3 - channel, pixel, value);
		break;
	case GL_UNSIGNED_INT_5_9_9_9_REV:
		read4Channel<GLuint, 5, 9, 9, 9, 27, 18, 9, 0>(3 - channel, pixel, value);
		break;

	/* R11F_G11F_B10F - uber complicated */
	case GL_UNSIGNED_INT_10F_11F_11F_REV:
		read11F_11F_10F_Channel(channel, pixel, value);
		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}
}

/** Write value of channel
 *
 * @param type    Type used by pixel
 * @param channel Channel index
 * @param value   Value to write
 * @param pixel   Pixel data
 **/
void Utils::writeChannel(GLenum type, GLuint channel, GLdouble value, GLubyte* pixel)
{
	switch (type)
	{
	/* Base types */
	case GL_UNSIGNED_BYTE:
		writeBaseTypeToUnsignedChannel<GLubyte>(channel, value, pixel);
		break;
	case GL_UNSIGNED_SHORT:
		writeBaseTypeToUnsignedChannel<GLushort>(channel, value, pixel);
		break;
	case GL_UNSIGNED_INT:
		writeBaseTypeToUnsignedChannel<GLuint>(channel, value, pixel);
		break;
	case GL_BYTE:
		writeBaseTypeToSignedChannel<GLbyte>(channel, value, pixel);
		break;
	case GL_SHORT:
		writeBaseTypeToSignedChannel<GLshort>(channel, value, pixel);
		break;
	case GL_INT:
		writeBaseTypeToSignedChannel<GLint>(channel, value, pixel);
		break;
	case GL_HALF_FLOAT:
		writeBaseTypeToHalfFloatChannel(channel, value, pixel);
		break;
	case GL_FLOAT:
		writeBaseTypeToFloatChannel(channel, value, pixel);
		break;

	/* Complicated */

	/* 3 channles */
	case GL_UNSIGNED_BYTE_3_3_2:
		write3Channel<GLubyte, 3, 3, 2, 5, 2, 0>(channel, value, pixel);
		break;
	case GL_UNSIGNED_SHORT_5_6_5:
		write3Channel<GLushort, 5, 6, 5, 11, 5, 0>(channel, value, pixel);
		break;

	/* 4 channels */
	case GL_UNSIGNED_SHORT_4_4_4_4:
		write4Channel<GLushort, 4, 4, 4, 4, 12, 8, 4, 0>(channel, value, pixel);
		break;
	case GL_UNSIGNED_SHORT_5_5_5_1:
		write4Channel<GLushort, 5, 5, 5, 1, 11, 6, 1, 0>(channel, value, pixel);
		break;
	case GL_UNSIGNED_INT_2_10_10_10_REV:
		write4Channel<GLuint, 2, 10, 10, 10, 30, 20, 10, 0>(3 - channel, value, pixel);
		break;
	case GL_UNSIGNED_INT_5_9_9_9_REV:
		write4Channel<GLuint, 5, 9, 9, 9, 27, 18, 9, 0>(3 - channel, value, pixel);
		break;

	/* R11F_G11F_B10F - uber complicated */
	case GL_UNSIGNED_INT_10F_11F_11F_REV:
		write11F_11F_10F_Channel(channel, value, pixel);
		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}
}

/** Packs given channels to pixel
 *
 * @param internal_format Internal format of image
 * @param type            Type used by image
 * @param red             Red channel
 * @param green           Green channel
 * @param blue            Blue channel
 * @param alpha           Alpha channel
 * @param out_pixel       Pixel data
 **/
void Utils::packPixel(GLenum internal_format, GLenum type, GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha,
					  GLubyte* out_pixel)
{
	switch (internal_format)
	{
	case GL_R8:
	case GL_R8_SNORM:
	case GL_R16:
	case GL_R16F:
	case GL_R16_SNORM:
	case GL_R32F:
	case GL_R8I:
	case GL_R8UI:
	case GL_R16I:
	case GL_R16UI:
	case GL_R32I:
	case GL_R32UI:
		writeChannel(type, 0, red, out_pixel);
		break;

	case GL_RG8:
	case GL_RG8_SNORM:
	case GL_RG16:
	case GL_RG16F:
	case GL_RG16_SNORM:
	case GL_RG32F:
	case GL_RG8I:
	case GL_RG8UI:
	case GL_RG16I:
	case GL_RG16UI:
	case GL_RG32I:
	case GL_RG32UI:
		writeChannel(type, 0, red, out_pixel);
		writeChannel(type, 1, green, out_pixel);
		break;

	case GL_R3_G3_B2:
	case GL_RGB4:
	case GL_RGB5:
	case GL_RGB8:
	case GL_RGB8_SNORM:
	case GL_RGB10:
	case GL_R11F_G11F_B10F:
	case GL_RGB12:
	case GL_RGB16:
	case GL_RGB16F:
	case GL_RGB16_SNORM:
	case GL_RGB32F:
	case GL_RGB8I:
	case GL_RGB8UI:
	case GL_RGB16I:
	case GL_RGB16UI:
	case GL_RGB32I:
	case GL_RGB32UI:
		writeChannel(type, 0, red, out_pixel);
		writeChannel(type, 1, green, out_pixel);
		writeChannel(type, 2, blue, out_pixel);
		break;

	case GL_RGB9_E5:
	case GL_RGBA2:
	case GL_RGBA4:
	case GL_RGB5_A1:
	case GL_RGBA8:
	case GL_RGBA8_SNORM:
	case GL_RGB10_A2:
	case GL_RGBA12:
	case GL_RGBA16:
	case GL_RGBA16F:
	case GL_RGBA16_SNORM:
	case GL_RGBA32F:
	case GL_RGBA8I:
	case GL_RGBA8UI:
	case GL_RGB10_A2UI:
	case GL_RGBA16I:
	case GL_RGBA16UI:
	case GL_RGBA32I:
	case GL_RGBA32UI:
		writeChannel(type, 0, red, out_pixel);
		writeChannel(type, 1, green, out_pixel);
		writeChannel(type, 2, blue, out_pixel);
		writeChannel(type, 3, alpha, out_pixel);
		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}
}

/** Unpacks channels from pixel
 *
 * @param internal_format Internal format of image
 * @param type            Type used by image
 * @param pixel           Pixel data
 * @param red             Red channel
 * @param green           Green channel
 * @param blue            Blue channel
 * @param alpha           Alpha channel
 **/
void Utils::unpackPixel(GLenum format, GLenum type, const GLubyte* pixel, GLdouble& out_red, GLdouble& out_green,
						GLdouble& out_blue, GLdouble& out_alpha)
{
	switch (format)
	{
	case GL_RED:
	case GL_RED_INTEGER:
		readChannel(type, 0, pixel, out_red);
		out_green = 1.0;
		out_blue  = 1.0;
		out_alpha = 1.0;
		break;
	case GL_RG:
	case GL_RG_INTEGER:
		readChannel(type, 0, pixel, out_red);
		readChannel(type, 1, pixel, out_green);
		out_blue  = 1.0;
		out_alpha = 1.0;
		break;
	case GL_RGB:
	case GL_RGB_INTEGER:
		switch (type)
		{
		case GL_UNSIGNED_INT_5_9_9_9_REV:
			readChannel(type, 0, pixel, out_red);
			readChannel(type, 1, pixel, out_green);
			readChannel(type, 2, pixel, out_blue);
			readChannel(type, 3, pixel, out_alpha);
			break;
		default:
			readChannel(type, 0, pixel, out_red);
			readChannel(type, 1, pixel, out_green);
			readChannel(type, 2, pixel, out_blue);
			out_alpha = 1.0;
			break;
		}
		break;
	case GL_RGBA:
	case GL_RGBA_INTEGER:
		readChannel(type, 0, pixel, out_red);
		readChannel(type, 1, pixel, out_green);
		readChannel(type, 2, pixel, out_blue);
		readChannel(type, 3, pixel, out_alpha);
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}
}

inline bool Utils::roundComponent(GLenum internal_format, GLenum component, GLdouble& value)
{
	int exponent = (internal_format == GL_RGB4 ? 4 : (internal_format == GL_RGB5 ? 5 : 0));
	if (!exponent)
		return false; //Currently this only happens with GL_RGB4 and GL_RGB5 when stored as 565 type.

	int rounded_value = static_cast<int>(floor((pow(2, exponent) - 1) * value + 0.5));
	int multiplier	= (component == GL_GREEN ? 2 : 1);
	if (internal_format == GL_RGB4)
	{
		multiplier *= 2;
	}
	value = rounded_value * multiplier;
	return true;
}

/** Unpacks pixels and compars them
 *
 * @param left_format           Format of left image
 * @param left_type             Type of left image
 * @param left_internal_format  Internal format of left image
 * @param left_pixel            Data of left pixel
 * @param right_format          Format of right image
 * @param right_type            Type of right image
 * @param right_internal_format Internal format of right image
 * @param right_pixel           Data of right pixel
 *
 * @return true if pixels match, false otherwise
 **/
bool Utils::unpackAndComaprePixels(GLenum left_format, GLenum left_type, GLenum left_internal_format,
								   const GLubyte* left_pixel, GLenum right_format, GLenum right_type,
								   GLenum right_internal_format, const GLubyte* right_pixel)
{
	GLdouble left_red;
	GLdouble left_green;
	GLdouble left_blue;
	GLdouble left_alpha;
	GLdouble right_red;
	GLdouble right_green;
	GLdouble right_blue;
	GLdouble right_alpha;

	unpackPixel(left_format, left_type, left_pixel, left_red, left_green, left_blue, left_alpha);

	unpackPixel(right_format, right_type, right_pixel, right_red, right_green, right_blue, right_alpha);

	roundComponent(left_internal_format, GL_RED, left_red);
	roundComponent(left_internal_format, GL_GREEN, left_green);
	roundComponent(left_internal_format, GL_BLUE, left_blue);

	roundComponent(right_internal_format, GL_RED, right_red);
	roundComponent(right_internal_format, GL_GREEN, right_green);
	roundComponent(right_internal_format, GL_BLUE, right_blue);

	return comparePixels(left_internal_format, left_red, left_green, left_blue, left_alpha, right_internal_format,
						 right_red, right_green, right_blue, right_alpha);
}

/* FunctionalTest */
#define FUNCTIONAL_TEST_N_LAYERS 12
#define FUNCTIONAL_TEST_N_LEVELS 3

/** Constructor
 *
 * @param context Text context
 **/
FunctionalTest::FunctionalTest(deqp::Context& context)
	: TestCase(context, "functional", "Test verifies CopyImageSubData copy data as requested")
	, m_dst_buf_name(0)
	, m_dst_tex_name(0)
	, m_rb_name(0)
	, m_src_buf_name(0)
	, m_src_tex_name(0)
	, m_test_case_index(0)
{
	for (GLuint src_tgt_id = 0; src_tgt_id < s_n_valid_targets; ++src_tgt_id)
	{
		const GLenum src_target = s_valid_targets[src_tgt_id];

#if COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_TARGETS == 0
		if ((GL_TEXTURE_1D == src_target) || (GL_TEXTURE_1D_ARRAY == src_target) || (GL_TEXTURE_2D == src_target) ||
			(GL_TEXTURE_CUBE_MAP == src_target) || (GL_TEXTURE_CUBE_MAP_ARRAY == src_target))
		{
			continue;
		}
#endif /* COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_TARGETS == 0 */

		for (GLuint dst_tgt_id = 0; dst_tgt_id < s_n_valid_targets; ++dst_tgt_id)
		{
			const GLenum dst_target = s_valid_targets[dst_tgt_id];

#if COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_TARGETS == 0
			if ((GL_TEXTURE_1D == dst_target) || (GL_TEXTURE_1D_ARRAY == dst_target) || (GL_TEXTURE_2D == dst_target) ||
				(GL_TEXTURE_CUBE_MAP == dst_target) || (GL_TEXTURE_CUBE_MAP_ARRAY == dst_target))
			{
				continue;
			}
#endif /* COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_TARGETS == 0 */

			/* Skip render buffer as destination */
			if (GL_RENDERBUFFER == dst_target)
			{
				continue;
			}

			/* Skip multisampled */
			if ((true == Utils::isTargetMultisampled(src_target)) || (true == Utils::isTargetMultisampled(dst_target)))
			{
				continue;
			}

			for (GLuint src_frmt_id = 0; src_frmt_id < s_n_internal_formats; ++src_frmt_id)
			{
				const GLenum src_format = s_internal_formats[src_frmt_id];

				if (src_format == GL_RGB9_E5 && src_target == GL_RENDERBUFFER)
				{
					continue;
				}

				for (GLuint dst_frmt_id = 0; dst_frmt_id < s_n_internal_formats; ++dst_frmt_id)
				{
					const GLenum dst_format = s_internal_formats[dst_frmt_id];

					/* Skip not compatible formats */
					if (false == Utils::areFormatsCompatible(src_format, dst_format))
					{
						continue;
					}

					prepareTestCases(dst_format, dst_target, src_format, src_target);
				}
			}
		}
	}
}

/** Execute test
 *
 * @return CONTINUE as long there are more test case, STOP otherwise
 **/
tcu::TestNode::IterateResult FunctionalTest::iterate()
{
	GLubyte*					 dst_pixels[FUNCTIONAL_TEST_N_LEVELS] = { 0 };
	const Functions&			 gl									  = m_context.getRenderContext().getFunctions();
	tcu::TestNode::IterateResult it_result							  = tcu::TestNode::STOP;
	bool						 result								  = false;
	GLubyte*					 src_pixels[FUNCTIONAL_TEST_N_LEVELS] = { 0 };
	const testCase&				 test_case							  = m_test_cases[m_test_case_index];

	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "PixelStorei");

	try
	{
		/* Prepare pixels */
		prepareDstPxls(test_case.m_dst, dst_pixels);
		prepareSrcPxls(test_case.m_src, test_case.m_dst.m_internal_format, src_pixels);

		/* Prepare textures */
		m_dst_tex_name = prepareTexture(test_case.m_dst, (const GLubyte**)dst_pixels, m_dst_buf_name);

		if (GL_RENDERBUFFER == test_case.m_src.m_target)
		{
			targetDesc desc = test_case.m_src;
			desc.m_target   = GL_TEXTURE_2D;

			m_rb_name	  = prepareTexture(test_case.m_src, (const GLubyte**)src_pixels, m_src_buf_name);
			m_src_tex_name = prepareTexture(desc, (const GLubyte**)src_pixels, m_src_buf_name);
		}
		else
		{
			m_src_tex_name = prepareTexture(test_case.m_src, (const GLubyte**)src_pixels, m_src_buf_name);
		}

		/* Copy images and verify results */
		result = copyAndVerify(test_case, (const GLubyte**)dst_pixels, (const GLubyte**)src_pixels);
	}
	catch (tcu::Exception& exc)
	{
		clean();
		cleanPixels((GLubyte**)dst_pixels);
		cleanPixels((GLubyte**)src_pixels);
		throw exc;
	}

	/* Free resources */
	clean();
	cleanPixels((GLubyte**)dst_pixels);
	cleanPixels((GLubyte**)src_pixels);

	/* Set result */
	if (true == result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

		/* Increase index */
		m_test_case_index += 1;

		/* Are there any test cases left */
		if (m_test_cases.size() > m_test_case_index)
		{
			it_result = tcu::TestNode::CONTINUE;
		}
	}
	else
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Failure. " << tcu::TestLog::EndMessage;

		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return it_result;
}

/** Calculate dimmensions of all levels based on size of specific level
 *
 * @param target      Target of image
 * @param level       Level index
 * @param width       Width of image at <level>
 * @param height      Height of image at <level>
 * @param out_widths  Calcualted widths, array of FUNCTIONAL_TEST_N_LEVELS'th elements
 * @param out_heights Calculated heights, array of FUNCTIONAL_TEST_N_LEVELS'th elements
 * @param out_depths  Calculated dephts, array of FUNCTIONAL_TEST_N_LEVELS'th elements
 **/
void FunctionalTest::calculateDimmensions(GLenum target, GLuint level, GLuint width, GLuint height, GLuint* out_widths,
										  GLuint* out_heights, GLuint* out_depths) const
{
	GLuint		 divide = 100;
	GLuint		 factors[FUNCTIONAL_TEST_N_LEVELS];
	GLuint		 factor			= divide;
	const bool   is_multi_layer = Utils::isTargetMultilayer(target);
	const GLuint n_layers		= (true == is_multi_layer) ? FUNCTIONAL_TEST_N_LAYERS : 1;

	for (GLint i = (GLint)level; i >= 0; --i)
	{
		factors[i] = factor;
		factor *= 2;
	}

	factor = divide / 2;
	for (GLuint i = level + 1; i < FUNCTIONAL_TEST_N_LEVELS; ++i)
	{
		factors[i] = factor;
		factor /= 2;
	}

	for (GLuint i = 0; i < FUNCTIONAL_TEST_N_LEVELS; ++i)
	{
		out_widths[i]  = width * factors[i] / divide;
		out_heights[i] = height * factors[i] / divide;

		if (GL_TEXTURE_3D == target)
		{
			out_depths[i] = FUNCTIONAL_TEST_N_LAYERS * factors[i] / divide;
		}
		else
		{
			out_depths[i] = n_layers;
		}
	}
}

/** Execute copyImageSubData for given test case and verify results
 *
 * @param test_case  Test case
 * @param dst_pixels Data of destination image
 * @param src_pixels Data of source image
 *
 * @return true if there is no error and results match expectations, false otherwise
 **/
bool FunctionalTest::copyAndVerify(const testCase& test_case, const GLubyte** dst_pixels, const GLubyte** src_pixels)
{
	GLenum			 error				= GL_NO_ERROR;
	const Functions& gl					= m_context.getRenderContext().getFunctions();
	GLuint			 region_depth		= 1;
	GLuint			 dst_layer_step		= 0;
	const bool		 is_dst_multi_layer = Utils::isTargetMultilayer(test_case.m_dst.m_target);
	const bool		 is_src_multi_layer = Utils::isTargetMultilayer(test_case.m_src.m_target);
	bool			 result				= false;
	GLuint			 src_layer_step		= 0;
	GLuint			 n_layers			= 1;

	/* Configure layers */
	if ((true == is_dst_multi_layer) || (true == is_src_multi_layer))
	{
		if (is_src_multi_layer == is_dst_multi_layer)
		{
			/* Both objects are multilayered, copy all layers at once, verify at once */
			region_depth = FUNCTIONAL_TEST_N_LAYERS;
		}
		else if (true == is_dst_multi_layer)
		{
			/* Destination is multilayered, copy each layer separetly, verify at once */
			n_layers	   = FUNCTIONAL_TEST_N_LAYERS;
			dst_layer_step = 1;
		}
		else
		{
			/* Destination is multilayered, copy and verify each layer separetly */
			n_layers	   = FUNCTIONAL_TEST_N_LAYERS;
			src_layer_step = 1;
		}
	}

	/* Copy and verification */
	{
		GLuint dst_layer = 0;
		GLuint src_layer = 0;

		/* For each layer */
		for (GLuint layer = 0; layer < n_layers; ++layer)
		{
			if (0 == m_rb_name)
			{
				gl.copyImageSubData(m_src_tex_name, test_case.m_src.m_target, test_case.m_src.m_level,
									test_case.m_src_x, test_case.m_src_y, src_layer, m_dst_tex_name,
									test_case.m_dst.m_target, test_case.m_dst.m_level, test_case.m_dst_x,
									test_case.m_dst_y, dst_layer, test_case.m_width, test_case.m_height, region_depth);
			}
			else /* Copy from src to rb and from rb to dst */
			{
				/* Src and rb shares differs only on target */
				gl.copyImageSubData(m_src_tex_name, GL_TEXTURE_2D, test_case.m_src.m_level, test_case.m_src_x,
									test_case.m_src_y, src_layer, m_rb_name, test_case.m_src.m_target,
									test_case.m_src.m_level, test_case.m_src_x, test_case.m_src_y, src_layer,
									test_case.m_width, test_case.m_height, region_depth);

				gl.copyImageSubData(m_rb_name, test_case.m_src.m_target, test_case.m_src.m_level, test_case.m_src_x,
									test_case.m_src_y, src_layer, m_dst_tex_name, test_case.m_dst.m_target,
									test_case.m_dst.m_level, test_case.m_dst_x, test_case.m_dst_y, dst_layer,
									test_case.m_width, test_case.m_height, region_depth);
			}

			/* Verify generated error */
			error = gl.getError();

			if (GL_NO_ERROR == error)
			{
				/* Verify copy results */
				result = verify(test_case, dst_layer, dst_pixels, src_layer, src_pixels, region_depth);
			}

			if ((GL_NO_ERROR != error) || (false == result))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "Failure. Targets src: " << glu::getTextureTargetStr(test_case.m_src.m_target)
					<< ", dst: " << glu::getTextureTargetStr(test_case.m_dst.m_target)
					<< ". Levels src: " << test_case.m_src.m_level << ", dst: " << test_case.m_dst.m_level
					<< ". Dimmensions src [" << test_case.m_src.m_width << ", " << test_case.m_src.m_height
					<< "], dst [" << test_case.m_dst.m_width << ", " << test_case.m_dst.m_height << "]. Region ["
					<< test_case.m_width << " x " << test_case.m_height << " x " << region_depth << "] from ["
					<< test_case.m_src_x << ", " << test_case.m_src_y << ", " << src_layer << "] to ["
					<< test_case.m_dst_x << ", " << test_case.m_dst_y << ", " << dst_layer
					<< "]. Format src: " << glu::getInternalFormatParameterStr(test_case.m_src.m_internal_format)
					<< ", dst: " << glu::getInternalFormatParameterStr(test_case.m_dst.m_internal_format)
					<< tcu::TestLog::EndMessage;

				if (GL_NO_ERROR != error)
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message
														<< "Failed due to error: " << glu::getErrorStr(error)
														<< tcu::TestLog::EndMessage;

					TCU_FAIL("Copy operation failed");
				}

				return false;
			}

			/* Step one layer */
			dst_layer += dst_layer_step;
			src_layer += src_layer_step;
		}
	}

	return true;
}

/** Cleans resources
 *
 **/
void FunctionalTest::clean()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean textures and buffers. Errors ignored */
	gl.deleteTextures(1, &m_dst_tex_name);
	gl.deleteTextures(1, &m_src_tex_name);

	m_dst_tex_name = 0;
	m_src_tex_name = 0;

	if (0 != m_dst_buf_name)
	{
		gl.deleteBuffers(1, &m_dst_buf_name);
		m_dst_buf_name = 0;
	}

	if (0 != m_rb_name)
	{
		gl.deleteRenderbuffers(1, &m_rb_name);
		m_rb_name = 0;
	}

	if (0 != m_src_buf_name)
	{
		gl.deleteBuffers(1, &m_src_buf_name);
		m_src_buf_name = 0;
	}
}

/** Free memory allocated for images
 *
 * @param pixels Array of pointers to image data
 **/
void FunctionalTest::cleanPixels(GLubyte** pixels) const
{
	for (GLuint i = 0; i < FUNCTIONAL_TEST_N_LEVELS; ++i)
	{
		if (0 != pixels[i])
		{
			delete[] pixels[i];
			pixels[i] = 0;
		}
	}
}

/** Compare two images
 * @param left_desc     Descriptor of left image
 * @param left_data     Data of left image
 * @param left_x        X of left image
 * @param left_y        Y of left image
 * @param left_layer    Layer of left image
 * @param left_level    Level of left image
 * @param right_desc    Descriptor of right image
 * @param right_data    Data of right image
 * @param right_x       X of right image
 * @param right_y       Y of right image
 * @param right_layer   Layer of right image
 * @param right_level   Level of right image
 * @param region_width  Width of region to compare
 * @param region_height Height of region to compare
 *
 * @return true if images are considered idenctial, false otherwise
 **/
bool FunctionalTest::compareImages(const targetDesc& left_desc, const GLubyte* left_data, GLuint left_x, GLuint left_y,
								   GLuint left_layer, GLuint left_level, const targetDesc& right_desc,
								   const glw::GLubyte* right_data, GLuint right_x, GLuint right_y, GLuint right_layer,
								   GLuint right_level, GLuint region_width, GLuint region_height) const
{
	/* Get level dimmensions */
	GLuint left_heights[FUNCTIONAL_TEST_N_LEVELS];
	GLuint left_widths[FUNCTIONAL_TEST_N_LEVELS];
	GLuint left_depths[FUNCTIONAL_TEST_N_LEVELS];
	GLuint right_heights[FUNCTIONAL_TEST_N_LEVELS];
	GLuint right_widths[FUNCTIONAL_TEST_N_LEVELS];
	GLuint right_depths[FUNCTIONAL_TEST_N_LEVELS];

	calculateDimmensions(left_desc.m_target, left_desc.m_level, left_desc.m_width, left_desc.m_height, left_widths,
						 left_heights, left_depths);

	calculateDimmensions(right_desc.m_target, right_desc.m_level, right_desc.m_width, right_desc.m_height, right_widths,
						 right_heights, right_depths);

	/* Constants */
	/* Dimmensions */
	const GLuint left_height  = left_heights[left_level];
	const GLuint left_width   = left_widths[left_level];
	const GLuint right_height = right_heights[right_level];
	const GLuint right_width  = right_widths[right_level];
	/* Sizes */
	const GLuint left_pixel_size  = Utils::getPixelSizeForFormat(left_desc.m_internal_format);
	const GLuint left_line_size   = left_pixel_size * left_width;
	const GLuint left_layer_size  = left_line_size * left_height;
	const GLuint right_pixel_size = Utils::getPixelSizeForFormat(right_desc.m_internal_format);
	const GLuint right_line_size  = right_pixel_size * right_width;
	const GLuint right_layer_size = right_line_size * right_height;

	/* Offsets */
	const GLuint left_layer_offset	 = left_layer_size * left_layer;
	const GLuint left_reg_line_offset  = left_line_size * left_y;
	const GLuint left_reg_pix_offset   = left_pixel_size * left_x;
	const GLuint right_layer_offset	= right_layer_size * right_layer;
	const GLuint right_reg_line_offset = right_line_size * right_y;
	const GLuint right_reg_pix_offset  = right_pixel_size * right_x;

	/* Pointers */
	const GLubyte* left_layer_data  = left_data + left_layer_offset;
	const GLubyte* right_layer_data = right_data + right_layer_offset;

	/* For each line of region */
	for (GLuint y = 0; y < region_height; ++y)
	{
		/* Offsets */
		const GLuint left_line_offset  = left_reg_line_offset + y * left_line_size;
		const GLuint right_line_offset = right_reg_line_offset + y * right_line_size;

		/* Pointers */
		const GLubyte* left_line_data  = left_layer_data + left_line_offset;
		const GLubyte* right_line_data = right_layer_data + right_line_offset;

		/* For each pixel of region */
		for (GLuint x = 0; x < region_width; ++x)
		{
			/* Offsets */
			const GLuint left_pixel_offset  = left_reg_pix_offset + x * left_pixel_size;
			const GLuint right_pixel_offset = right_reg_pix_offset + x * right_pixel_size;

			/* Pointers */
			const GLubyte* left_pixel_data  = left_line_data + left_pixel_offset;
			const GLubyte* right_pixel_data = right_line_data + right_pixel_offset;

			/* Compare */
			if (false == Utils::comparePixels(left_pixel_size, left_pixel_data, right_pixel_size, right_pixel_data))
			{
				if (false == Utils::unpackAndComaprePixels(left_desc.m_format, left_desc.m_type,
														   left_desc.m_internal_format, left_pixel_data,
														   right_desc.m_format, right_desc.m_type,
														   right_desc.m_internal_format, right_pixel_data))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Not matching pixels found. Left: [" << x + left_x << ", "
						<< y + left_y << ", " << left_layer << "] lvl:" << left_level
						<< ", off: " << left_pixel_data - left_data
						<< ", data: " << Utils::getPixelString(left_desc.m_internal_format, left_pixel_data)
						<< ". Right: [" << x + right_x << ", " << y + right_y << ", " << right_layer
						<< "] lvl: " << right_level << ", off: " << right_pixel_data - right_data
						<< ", data: " << Utils::getPixelString(right_desc.m_internal_format, right_pixel_data)
						<< tcu::TestLog::EndMessage;

					return false;
				}
			}
		}
	}

	return true;
}

/** Prepare regions that should not be modified during test case
 *
 * @param test_case     Test case descriptor
 * @param dst_level     Level of destination image
 * @param out_regions   Number of regions
 * @param out_n_regions Regions
 **/
void FunctionalTest::getCleanRegions(const testCase& test_case, GLuint dst_level, GLuint out_regions[4][4],
									 GLuint& out_n_regions) const
{
	GLuint dst_heights[FUNCTIONAL_TEST_N_LEVELS];
	GLuint dst_widths[FUNCTIONAL_TEST_N_LEVELS];
	GLuint dst_depths[FUNCTIONAL_TEST_N_LEVELS];

	out_n_regions = 0;

	calculateDimmensions(test_case.m_dst.m_target, dst_level, test_case.m_dst.m_width, test_case.m_dst.m_height,
						 dst_widths, dst_heights, dst_depths);

	/* Constants */
	/* Copied region */
	const GLuint reg_x = test_case.m_dst_x;
	const GLuint reg_y = test_case.m_dst_y;
	const GLuint reg_w = test_case.m_width;
	const GLuint reg_h = test_case.m_height;
	const GLuint reg_r = reg_x + reg_w;
	const GLuint reg_t = reg_y + reg_h;

	/* Image */
	const GLuint img_w = dst_widths[dst_level];
	const GLuint img_h = dst_heights[dst_level];

	/* Bottom row */
	if (0 != reg_y)
	{
		out_regions[out_n_regions][0] = 0;
		out_regions[out_n_regions][1] = 0;
		out_regions[out_n_regions][2] = img_w;
		out_regions[out_n_regions][3] = reg_y;
		out_n_regions += 1;
	}

	/* Left edge */
	if (0 != reg_x)
	{
		out_regions[out_n_regions][0] = 0;
		out_regions[out_n_regions][1] = reg_y;
		out_regions[out_n_regions][2] = reg_x;
		out_regions[out_n_regions][3] = reg_h;
		out_n_regions += 1;
	}

	/* Right edge */
	if (img_w != reg_r)
	{
		out_regions[out_n_regions][0] = reg_r;
		out_regions[out_n_regions][1] = reg_y;
		out_regions[out_n_regions][2] = img_w - reg_r;
		out_regions[out_n_regions][3] = reg_h;
		out_n_regions += 1;
	}

	/* Top row */
	if (img_h != reg_t)
	{
		out_regions[out_n_regions][0] = 0;
		out_regions[out_n_regions][1] = reg_t;
		out_regions[out_n_regions][2] = img_w;
		out_regions[out_n_regions][3] = img_h - reg_t;
		out_n_regions += 1;
	}
}

/** Get pixel data for image
 *
 * @param name       Name of image
 * @param desc       Descriptor of image
 * @param level      Level to capture
 * @param out_pixels Pixels
 **/
void FunctionalTest::getPixels(GLuint name, const targetDesc& desc, GLuint level, GLubyte* out_pixels) const
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindTexture(desc.m_target, name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	gl.getTexImage(desc.m_target, level, desc.m_format, desc.m_type, out_pixels);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexImage");

	gl.bindTexture(desc.m_target, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
}

/** Prepare data for destination image
 *
 * @param desc       Descriptor
 * @param out_pixels Array of pointer to image data
 **/
void FunctionalTest::prepareDstPxls(const FunctionalTest::targetDesc& desc, GLubyte** out_pixels) const
{
	const GLenum internal_format = desc.m_internal_format;
	const bool   is_multi_level  = Utils::isTargetMultilevel(desc.m_target);
	GLuint		 n_levels		 = 1;
	const GLuint pixel_size		 = Utils::getPixelSizeForFormat(desc.m_internal_format);
	const GLenum type			 = desc.m_type;

	/* Configure levels */
	if (true == is_multi_level)
	{
		n_levels = FUNCTIONAL_TEST_N_LEVELS;
	}

	/* Calculate dimmensions */
	GLuint heights[FUNCTIONAL_TEST_N_LEVELS];
	GLuint widths[FUNCTIONAL_TEST_N_LEVELS];
	GLuint depths[FUNCTIONAL_TEST_N_LEVELS];

	calculateDimmensions(desc.m_target, desc.m_level, desc.m_width, desc.m_height, widths, heights, depths);

	/* Prepare storage */
	for (GLuint i = 0; i < n_levels; ++i)
	{
		const GLuint req_memory_per_layer = pixel_size * widths[i] * heights[i];
		const GLuint req_memory_for_level = req_memory_per_layer * depths[i];

		out_pixels[i] = new GLubyte[req_memory_for_level];

		if (0 == out_pixels[i])
		{
			TCU_FAIL("Memory allocation failed");
		}

		memset(out_pixels[i], 0, req_memory_for_level);
	}

	/* Fill pixels */
	for (GLuint i = 0; i < n_levels; ++i)
	{
		const GLuint n_layers = depths[i];
		const GLuint n_pixels = widths[i] * heights[i];
		GLubyte*	 ptr	  = (GLubyte*)out_pixels[i];

		for (GLuint j = 0; j < n_pixels * n_layers; ++j)
		{
			GLubyte* pixel_data = ptr + j * pixel_size;

			Utils::packPixel(internal_format, type, 1.0, 1.0, 1.0, 1.0, pixel_data);
		}
	}
}

/** Prepare data for source image
 *
 * @param desc                Descriptor
 * @param dst_internal_format Internal format of destination image
 * @param out_pixels          Array of pointer to image data
 **/
void FunctionalTest::prepareSrcPxls(const FunctionalTest::targetDesc& desc, GLenum /* dst_internal_format */,
									GLubyte**						  out_pixels) const
{
	const GLenum internal_format = desc.m_internal_format;
	const bool   is_multi_level  = Utils::isTargetMultilevel(desc.m_target);
	GLuint		 n_levels		 = 1;
	const GLuint pixel_size		 = Utils::getPixelSizeForFormat(desc.m_internal_format);
	const GLenum type			 = desc.m_type;

	/* Configure levels */
	if (true == is_multi_level)
	{
		n_levels = FUNCTIONAL_TEST_N_LEVELS;
	}

	/* Calculate dimmensions */
	GLuint heights[FUNCTIONAL_TEST_N_LEVELS];
	GLuint widths[FUNCTIONAL_TEST_N_LEVELS];
	GLuint depths[FUNCTIONAL_TEST_N_LEVELS];

	calculateDimmensions(desc.m_target, desc.m_level, desc.m_width, desc.m_height, widths, heights, depths);

	/* Prepare storage */
	for (GLuint i = 0; i < n_levels; ++i)
	{
		const GLuint req_memory_per_layer = pixel_size * widths[i] * heights[i];
		const GLuint req_memory_for_level = req_memory_per_layer * depths[i];

		out_pixels[i] = new GLubyte[req_memory_for_level];

		if (0 == out_pixels[i])
		{
			TCU_FAIL("Memory allocation failed");
		}

		memset(out_pixels[i], 0, req_memory_for_level);
	}

	for (GLuint lvl = 0; lvl < n_levels; ++lvl)
	{
		const GLuint n_layers			  = depths[lvl];
		const GLuint line_size			  = pixel_size * widths[lvl];
		const GLuint req_memory_per_layer = line_size * heights[lvl];
		GLubyte*	 level				  = (GLubyte*)out_pixels[lvl];

		for (GLuint lay = 0; lay < n_layers; ++lay)
		{
			const GLuint layer_offset = lay * req_memory_per_layer;

			GLubyte* layer = ((GLubyte*)level) + layer_offset;

			for (GLuint y = 0; y < heights[lvl]; ++y)
			{
				const GLuint line_offset = line_size * y;

				GLubyte* line = layer + line_offset;

				for (GLuint x = 0; x < widths[lvl]; ++x)
				{
					const GLuint pixel_offset = x * pixel_size;

					GLubyte* pixel = line + pixel_offset;

					/* 255 is max ubyte. 1/15.9375 = 16/255 */
					const GLdouble red   = ((GLdouble)x) / 255.0 + (((GLdouble)y) / 15.9375);
					const GLdouble green = ((GLdouble)lay) / 255.0 + (((GLdouble)lvl) / 15.9375);
					const GLdouble blue  = 0.125;
					const GLdouble alpha = 1.0; /* This value has special meaning for some internal_formats */

					Utils::packPixel(internal_format, type, red, green, blue, alpha, pixel);
				}
			}
		}
	}
}

/** Prepare test cases for given targets and internal formats
 *
 * @param dst_internal_format Internal format of destination image
 * @param dst_target          Target of destination image
 * @param src_internal_format Internal format of source image
 * @param src_target          Target of source image
 **/
void FunctionalTest::prepareTestCases(GLenum dst_internal_format, GLenum dst_target, GLenum src_internal_format,
									  GLenum src_target)
{
	static const GLuint image_dimmensions[] = {
		7,
#if COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_IMG_DIM
		8,
		9,
		10,
		11,
		12,
		13,
		14,
#endif /* COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_IMG_DIM */
		15
	};

	static const GLuint region_dimmensions[] = {
		1,
#if COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_REG_DIM
		2,
		3,
		4,
		5,
		6,
#endif /* COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_REG_DIM */
		7
	};

	static const GLuint n_image_dimmensions  = sizeof(image_dimmensions) / sizeof(image_dimmensions[0]);
	static const GLuint n_region_dimmensions = sizeof(region_dimmensions) / sizeof(region_dimmensions[0]);

	const bool   is_dst_multi_level = Utils::isTargetMultilevel(dst_target);
	const bool   is_src_multi_level = Utils::isTargetMultilevel(src_target);
	const GLenum dst_format			= Utils::getFormat(dst_internal_format);
	const GLuint dst_n_levels		= (true == is_dst_multi_level) ? FUNCTIONAL_TEST_N_LEVELS : 1;
	const GLenum dst_type			= Utils::getType(dst_internal_format);
	const GLenum src_format			= Utils::getFormat(src_internal_format);
	const GLuint src_n_levels		= (true == is_src_multi_level) ? FUNCTIONAL_TEST_N_LEVELS : 1;
	const GLenum src_type			= Utils::getType(src_internal_format);

	for (GLuint src_level = 0; src_level < src_n_levels; ++src_level)
	{
		for (GLuint dst_level = 0; dst_level < dst_n_levels; ++dst_level)
		{
			for (GLuint src_img_dim_id = 0; src_img_dim_id < n_image_dimmensions; ++src_img_dim_id)
			{
				const GLuint src_image_dimmension = image_dimmensions[src_img_dim_id];

				for (GLuint dst_img_dim_id = 0; dst_img_dim_id < n_image_dimmensions; ++dst_img_dim_id)
				{
					const GLuint dst_image_dimmension = image_dimmensions[dst_img_dim_id];

					for (GLuint reg_dim_id = 0; reg_dim_id < n_region_dimmensions; ++reg_dim_id)
					{
						const GLuint region_dimmension = region_dimmensions[reg_dim_id];
						GLuint		 dst_coord[3]	  = { 0, 0, 0 };
						const GLuint dst_dim_diff	  = dst_image_dimmension - region_dimmension;
						GLuint		 n_dst_coords	  = 1;
#if COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_REG_POS
						GLuint n_src_coords = 1;
#endif /* COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_REG_POS */
						GLuint		 src_coord[3] = { 0, 0, 0 };
						const GLuint src_dim_diff = src_image_dimmension - region_dimmension;

						/* Calculate coords */
						if (1 == dst_dim_diff)
						{
							dst_coord[1] = 1;
							n_dst_coords = 2;
						}
						else if (1 < dst_dim_diff)
						{
							dst_coord[1] = dst_dim_diff / 2;
							dst_coord[2] = dst_dim_diff;
							n_dst_coords = 3;
						}

						if (1 == src_dim_diff)
						{
							src_coord[1] = 1;
#if COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_REG_POS
							n_src_coords = 2;
#endif /* COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_REG_POS */
						}
						else if (1 < src_dim_diff)
						{
							src_coord[1] = src_dim_diff / 2;
							src_coord[2] = src_dim_diff;
#if COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_REG_POS
							n_src_coords = 3;
#endif /* COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_REG_POS */
						}

						testCase test_case = {
							{									/* m_dst */
							  dst_target, dst_image_dimmension, /* width */
							  dst_image_dimmension,				/* height */
							  dst_level, dst_internal_format, dst_format, dst_type },
							0,									/* dst_x */
							0,									/* dst_y */
							{									/* m_src */
							  src_target, src_image_dimmension, /* width */
							  src_image_dimmension,				/* height */
							  src_level, src_internal_format, src_format, src_type },
							0,				   /* src_x */
							0,				   /* src_y */
							region_dimmension, /* width */
							region_dimmension, /* height */
						};

#if COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_REG_POS
						for (GLuint src_x = 0; src_x < n_src_coords; ++src_x)
						{
							for (GLuint src_y = 0; src_y < n_src_coords; ++src_y)
							{
								for (GLuint dst_x = 0; dst_x < n_dst_coords; ++dst_x)
								{
									for (GLuint dst_y = 0; dst_y < n_dst_coords; ++dst_y)
									{
										test_case.m_dst_x = dst_coord[dst_x];
										test_case.m_dst_y = dst_coord[dst_y];
										test_case.m_src_x = src_coord[src_x];
										test_case.m_src_y = src_coord[src_y];

										m_test_cases.push_back(test_case);
									}
								}
							}
						}
#else  /* COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_REG_POS */
						test_case.m_dst_x = dst_coord[n_dst_coords - 1];
						test_case.m_dst_y = dst_coord[n_dst_coords - 1];
						test_case.m_src_x = src_coord[0];
						test_case.m_src_y = src_coord[0];

						m_test_cases.push_back(test_case);
#endif /* COPY_IMAGE_FUNCTIONAL_TEST_ENABLE_ALL_REG_POS */

						/* Whole image, for non 7x7 */
						if ((dst_image_dimmension == src_image_dimmension) &&
							(image_dimmensions[0] != dst_image_dimmension))
						{
							test_case.m_dst_x  = 0;
							test_case.m_dst_y  = 0;
							test_case.m_src_x  = 0;
							test_case.m_src_y  = 0;
							test_case.m_width  = dst_image_dimmension;
							test_case.m_height = dst_image_dimmension;

							m_test_cases.push_back(test_case);
						}
					}
				}
			}
		}
	}
}

/** Prepare texture
 *
 * @param desc       Descriptor
 * @param pixels     Image data
 * @param out_buf_id Id of buffer used by texture buffer
 *
 * @return Name of iamge
 **/
GLuint FunctionalTest::prepareTexture(const targetDesc& desc, const GLubyte** pixels, GLuint& out_buf_id)
{
	GLuint name = Utils::generateTexture(m_context, desc.m_target);

	if (false == Utils::isTargetMultilevel(desc.m_target))
	{
		Utils::prepareTexture(m_context, name, desc.m_target, desc.m_internal_format, desc.m_format, desc.m_type,
							  0 /* level */, desc.m_width, desc.m_height,
							  FUNCTIONAL_TEST_N_LAYERS /* depth - 12 for multilayered, 1D and 2D will ignore that */,
							  pixels[0], out_buf_id);

		Utils::makeTextureComplete(m_context, desc.m_target, name, 0 /* base */, 0 /* max */);
	}
	else
	{
		/* Calculate dimmensions */
		GLuint heights[FUNCTIONAL_TEST_N_LEVELS];
		GLuint widths[FUNCTIONAL_TEST_N_LEVELS];
		GLuint depths[FUNCTIONAL_TEST_N_LEVELS];

		calculateDimmensions(desc.m_target, desc.m_level, desc.m_width, desc.m_height, widths, heights, depths);

		for (GLuint level = 0; level < FUNCTIONAL_TEST_N_LEVELS; ++level)
		{
			Utils::prepareTexture(m_context, name, desc.m_target, desc.m_internal_format, desc.m_format, desc.m_type,
								  level, widths[level], heights[level], depths[level], pixels[level], out_buf_id);

			Utils::makeTextureComplete(m_context, desc.m_target, name, 0 /* base */, 2 /* max */);
		}
	}

	return name;
}

/** Verify copy operation
 *
 * @param test_case  Test case
 * @param dst_layer  First layer modified by copy
 * @param dst_pixels Origiranl data of destination image
 * @param src_layer  First layer read by copy
 * @param src_pixels Original data of source image
 * @param depth      Number of copied layers
 *
 * @return true if everything is as expected, false otherwise
 **/
bool FunctionalTest::verify(const testCase& test_case, GLuint dst_layer, const GLubyte** dst_pixels, GLuint src_layer,
							const GLubyte** src_pixels, GLuint depth)
{
	const bool			 is_dst_multi_level = Utils::isTargetMultilevel(test_case.m_dst.m_target);
	const bool			 is_src_multi_level = Utils::isTargetMultilevel(test_case.m_src.m_target);
	const GLuint		 dst_level			= test_case.m_dst.m_level;
	std::vector<GLubyte> dst_level_data;
	const GLuint		 dst_pixel_size = Utils::getPixelSizeForFormat(test_case.m_dst.m_internal_format);
	targetDesc			 src_desc		= test_case.m_src;
	const GLuint		 src_level		= src_desc.m_level;
	std::vector<GLubyte> src_level_data;
	const GLuint		 src_pixel_size = Utils::getPixelSizeForFormat(src_desc.m_internal_format);

	if (0 != m_rb_name)
	{
		src_desc.m_target = GL_TEXTURE_2D;
	}

	/* Calculate storage requirements */
	GLuint dst_req_mem_per_layer[FUNCTIONAL_TEST_N_LEVELS];
	GLuint dst_heights[FUNCTIONAL_TEST_N_LEVELS];
	GLuint dst_widths[FUNCTIONAL_TEST_N_LEVELS];
	GLuint dst_depths[FUNCTIONAL_TEST_N_LEVELS];
	GLuint src_req_mem_per_layer[FUNCTIONAL_TEST_N_LEVELS];
	GLuint src_heights[FUNCTIONAL_TEST_N_LEVELS];
	GLuint src_widths[FUNCTIONAL_TEST_N_LEVELS];
	GLuint src_depths[FUNCTIONAL_TEST_N_LEVELS];

	calculateDimmensions(test_case.m_dst.m_target, dst_level, test_case.m_dst.m_width, test_case.m_dst.m_height,
						 dst_widths, dst_heights, dst_depths);

	calculateDimmensions(src_desc.m_target, src_level, src_desc.m_width, src_desc.m_height, src_widths, src_heights,
						 src_depths);

	for (GLuint i = 0; i < FUNCTIONAL_TEST_N_LEVELS; ++i)
	{
		dst_req_mem_per_layer[i] = dst_widths[i] * dst_heights[i] * dst_pixel_size;
		src_req_mem_per_layer[i] = src_widths[i] * src_heights[i] * src_pixel_size;
	}

	/* Prepare storage, use 0 level as it is the biggest one */
	dst_level_data.resize(dst_req_mem_per_layer[0] * dst_depths[0]);
	src_level_data.resize(src_req_mem_per_layer[0] * src_depths[0]);

	/* Verification of contents
	 * - source image                                           - expect no modification
	 * - destination image, mipmap before and after dst_level   - expect no modification
	 * - destination image, mipmap at dst_level:
	 *   * layers after dst_layer + depth                       - expect no modification
	 *   * layers <0, dst_layer + depth>                        - expect that contents at selected region were copied
	 */

	/* Check if source image was not modified */
	{
		const GLuint n_levels = (true == is_src_multi_level) ? FUNCTIONAL_TEST_N_LEVELS : 1;

		for (GLuint level = 0; level < n_levels; ++level)
		{
			getPixels(m_src_tex_name, src_desc, level, &src_level_data[0]);

			for (GLuint layer = 0; layer < src_depths[level]; ++layer)
			{
				if (false == compareImages(src_desc, src_pixels[level], 0, 0, layer, level, src_desc,
										   &src_level_data[0], 0, 0, layer, level, src_widths[level],
										   src_heights[level]))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message
						<< "CopyImageSubData modified contents of source image. Original data: left."
						<< tcu::TestLog::EndMessage;
					return false;
				}
			}
		}
	}

	/* Check if contents of destination at levels != dst_level were not modified */
	{
		const GLuint n_levels = (true == is_dst_multi_level) ? FUNCTIONAL_TEST_N_LEVELS : 1;

		for (GLuint level = 0; level < n_levels; ++level)
		{
			if (dst_level == level)
			{
				continue;
			}

			getPixels(m_dst_tex_name, test_case.m_dst, level, &dst_level_data[0]);

			for (GLuint layer = 0; layer < dst_depths[level]; ++layer)
			{
				if (false == compareImages(test_case.m_dst, dst_pixels[level], 0, 0, layer, level, test_case.m_dst,
										   &dst_level_data[0], 0, 0, layer, level, dst_widths[level],
										   dst_heights[level]))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message
						<< "CopyImageSubData modified contents of wrong mipmap level. Original data: left."
						<< tcu::TestLog::EndMessage;

					return false;
				}
			}
		}
	}

	/* Check contents of modified level */
	{
		getPixels(m_dst_tex_name, test_case.m_dst, dst_level, &dst_level_data[0]);

		/* Check anything after dst_layer + depth */
		{
			for (GLuint layer = dst_layer + depth; layer < dst_depths[dst_level]; ++layer)
			{
				if (false == compareImages(test_case.m_dst, dst_pixels[dst_level], 0, 0, layer, dst_level,
										   test_case.m_dst, &dst_level_data[0], 0, 0, layer, dst_level,
										   dst_widths[dst_level], dst_heights[dst_level]))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message
						<< "CopyImageSubData modified contents of wrong layer. Original data: left."
						<< tcu::TestLog::EndMessage;

					return false;
				}
			}
		}

		/* Check modified layers */
		for (GLuint layer = 0; layer < depth; ++layer)
		{
			/* Check contents outside of copied region */
			{
				GLuint n_regions	 = 0;
				GLuint regions[4][4] = { { 0 } };

				getCleanRegions(test_case, dst_level, regions, n_regions);

				for (GLuint region = 0; region < n_regions; ++region)
				{
					const GLuint x = regions[region][0];
					const GLuint y = regions[region][1];
					const GLuint w = regions[region][2];
					const GLuint h = regions[region][3];

					if (false == compareImages(test_case.m_dst, dst_pixels[dst_level], x, y, layer + dst_layer,
											   dst_level, test_case.m_dst, &dst_level_data[0], x, y, layer + dst_layer,
											   dst_level, w, h))
					{
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message
							<< "CopyImageSubData modified contents outside of copied region. Original data: left."
							<< tcu::TestLog::EndMessage;
						return false;
					}
				}
			}

			/* Check contents of copied region */
			if (false == compareImages(test_case.m_dst, &dst_level_data[0], test_case.m_dst_x, test_case.m_dst_y,
									   layer + dst_layer, dst_level, src_desc, src_pixels[src_level], test_case.m_src_x,
									   test_case.m_src_y, layer + src_layer, src_level, test_case.m_width,
									   test_case.m_height))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "CopyImageSubData stored invalid data in copied region. Destination data: left."
					<< tcu::TestLog::EndMessage;
				return false;
			}
		}
	}

	return true;
}

/* SmokeTest */
/* Constants */
const GLuint SmokeTest::m_width  = 16;
const GLuint SmokeTest::m_height = 16;
const GLuint SmokeTest::m_depth  = 1;

/** Constructor
 *
 * @param context Text context
 **/
SmokeTest::SmokeTest(deqp::Context& context)
	: TestCase(context, "smoke_test", "Test tries all formats and targets")
	, m_dst_buf_name(0)
	, m_dst_tex_name(0)
	, m_rb_name(0)
	, m_src_buf_name(0)
	, m_src_tex_name(0)
	, m_test_case_index(0)
{
	/* Iterate over valid targets */
	for (GLuint tgt_id = 0; tgt_id < s_n_valid_targets; ++tgt_id)
	{
		const GLenum target = s_valid_targets[tgt_id];

		if (true == Utils::isTargetMultisampled(target))
		{
			continue;
		}

		const testCase test_case = { target, GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT };

		m_test_cases.push_back(test_case);
	}

	/* Iterate over internal formats */
	for (GLuint fmt_id = 0; fmt_id < s_n_internal_formats; ++fmt_id)
	{
		const GLenum internal_format = s_internal_formats[fmt_id];
		const GLenum format			 = Utils::getFormat(internal_format);
		const GLenum type			 = Utils::getType(internal_format);

		const testCase test_case = { GL_TEXTURE_2D, internal_format, format, type };

		m_test_cases.push_back(test_case);
	}
}

/** Cleans resources
 *
 **/
void SmokeTest::clean()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean textures and buffers. Errors ignored */
	gl.deleteTextures(1, &m_dst_tex_name);
	gl.deleteTextures(1, &m_src_tex_name);

	m_dst_tex_name = 0;
	m_src_tex_name = 0;

	if (0 != m_dst_buf_name)
	{
		gl.deleteBuffers(1, &m_dst_buf_name);
		m_dst_buf_name = 0;
	}

	if (0 != m_rb_name)
	{
		gl.deleteRenderbuffers(1, &m_rb_name);
		m_rb_name = 0;
	}

	if (0 != m_src_buf_name)
	{
		gl.deleteBuffers(1, &m_src_buf_name);
		m_src_buf_name = 0;
	}
}

/** Free memory allocated for images
 *
 * @param pixels Pointers to image data
 **/
void SmokeTest::cleanPixels(GLubyte*& pixels) const
{
	if (0 == pixels)
	{
		return;
	}

	delete[] pixels;
	pixels = 0;
}

/** Compare two images
 * @param test_case     Test case descriptor
 * @param left_data     Data of left image
 * @param right_data    Data of right image
 *
 * @return true if images are considered idenctial, false otherwise
 **/
bool SmokeTest::compareImages(const testCase& test_case, const GLubyte* left_data, const GLubyte* right_data) const
{
	/* Constants */
	/* Sizes */
	const GLuint pixel_size = Utils::getPixelSizeForFormat(test_case.m_internal_format);
	const GLuint line_size  = pixel_size * m_width;

	GLuint height = m_height;

	if ((GL_TEXTURE_1D == test_case.m_target) || (GL_TEXTURE_1D_ARRAY == test_case.m_target))
	{
		height = 1;
	}

	/* For each line */
	for (GLuint y = 0; y < height; ++y)
	{
		/* Offsets */
		const GLuint line_offset = y * line_size;

		/* Pointers */
		const GLubyte* left_line_data  = left_data + line_offset;
		const GLubyte* right_line_data = right_data + line_offset;

		/* For each pixel of region */
		for (GLuint x = 0; x < m_width; ++x)
		{
			/* Offsets */
			const GLuint pixel_offset = x * pixel_size;

			/* Pointers */
			const GLubyte* left_pixel_data  = left_line_data + pixel_offset;
			const GLubyte* right_pixel_data = right_line_data + pixel_offset;

			/* Compare */
			if (false == Utils::comparePixels(pixel_size, left_pixel_data, pixel_size, right_pixel_data))
			{
				if (false == Utils::unpackAndComaprePixels(
								 test_case.m_format, test_case.m_type, test_case.m_internal_format, left_pixel_data,
								 test_case.m_format, test_case.m_type, test_case.m_internal_format, right_pixel_data))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Not matching pixels found. "
						<< "[" << x << ", " << y << "], off: " << left_pixel_data - left_data
						<< ". Data left: " << Utils::getPixelString(test_case.m_internal_format, left_pixel_data)
						<< ", right: " << Utils::getPixelString(test_case.m_internal_format, right_pixel_data)
						<< tcu::TestLog::EndMessage;

					return false;
				}
			}
		}
	}

	return true;
}

/** Execute copyImageSubData for given test case and verify results
 *
 * @param test_case  Test case
 * @param src_pixels Data of source image
 *
 * @return true if there is no error and results match expectations, false otherwise
 **/
bool SmokeTest::copyAndVerify(const testCase& test_case, const GLubyte* src_pixels)
{
	GLenum			 error  = GL_NO_ERROR;
	const Functions& gl		= m_context.getRenderContext().getFunctions();
	bool			 result = false;

	/* Copy and verification */
	{
		if (0 == m_rb_name)
		{
			GLuint height = m_height;

			if ((GL_TEXTURE_1D == test_case.m_target) || (GL_TEXTURE_1D_ARRAY == test_case.m_target))
			{
				height = 1;
			}

			gl.copyImageSubData(m_src_tex_name, test_case.m_target, 0 /* srcLevel */, 0 /* srcX */, 0 /* srcY */,
								0 /* srcZ */, m_dst_tex_name, test_case.m_target, 0 /* dstLevel */, 0 /* dstX */,
								0 /* dstY */, 0 /* dstZ */, m_width, height, m_depth);
		}
		else /* Copy from src to rb and from rb to dst */
		{
			/* Src and rb shares differs only on target */
			gl.copyImageSubData(m_src_tex_name, GL_TEXTURE_2D, 0 /* srcLevel */, 0 /* srcX */, 0 /* srcY */,
								0 /* srcZ */, m_rb_name, test_case.m_target, 0 /* dstLevel */, 0 /* dstX */,
								0 /* dstY */, 0 /* dstZ */, m_width, m_height, m_depth);

			gl.copyImageSubData(m_rb_name, test_case.m_target, 0 /* dstLevel */, 0 /* dstX */, 0 /* dstY */,
								0 /* dstZ */, m_dst_tex_name, GL_TEXTURE_2D, 0 /* dstLevel */, 0 /* dstX */,
								0 /* dstY */, 0 /* dstZ */, m_width, m_height, m_depth);
		}

		/* Verify generated error */
		error = gl.getError();

		if (GL_NO_ERROR == error)
		{
			/* Verify copy results */
			result = verify(test_case, src_pixels);
		}

		if ((GL_NO_ERROR != error) || (false == result))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Failure. Target: " << glu::getTextureTargetStr(test_case.m_target)
				<< ". Format: " << glu::getInternalFormatParameterStr(test_case.m_internal_format)
				<< tcu::TestLog::EndMessage;

			if (GL_NO_ERROR != error)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message
													<< "Failed due to error: " << glu::getErrorStr(error)
													<< tcu::TestLog::EndMessage;

				TCU_FAIL("Copy operation failed");
			}

			return false;
		}
	}

	return true;
}

/** Get pixel data for image
 *
 * @param name       Name of image
 * @param test_case  Test case descriptor
 * @param out_pixels Pixels
 **/
void SmokeTest::getPixels(GLuint name, const SmokeTest::testCase& test_case, GLubyte* out_pixels) const
{
	const Functions& gl		  = m_context.getRenderContext().getFunctions();
	GLenum			 tgt_bind = test_case.m_target;
	GLenum			 tgt_get  = test_case.m_target;

	if (GL_RENDERBUFFER == test_case.m_target)
	{
		tgt_bind = GL_TEXTURE_2D;
		tgt_get  = GL_TEXTURE_2D;
	}
	else if (GL_TEXTURE_CUBE_MAP == test_case.m_target)
	{
		tgt_get = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
	}

	gl.bindTexture(tgt_bind, name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	gl.getTexImage(tgt_get, 0 /* level */, test_case.m_format, test_case.m_type, out_pixels);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexImage");

	gl.bindTexture(tgt_bind, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
}

/** Execute test
 *
 * @return CONTINUE as long there are more test case, STOP otherwise
 **/
tcu::TestNode::IterateResult SmokeTest::iterate()
{
	GLubyte*					 dst_pixels = 0;
	const Functions&			 gl			= m_context.getRenderContext().getFunctions();
	tcu::TestNode::IterateResult it_result  = tcu::TestNode::STOP;
	bool						 result		= false;
	GLubyte*					 src_pixels = 0;
	const testCase&				 test_case  = m_test_cases[m_test_case_index];

	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "PixelStorei");

	try
	{
		/* Prepare pixels */
		prepareDstPxls(test_case, dst_pixels);
		prepareSrcPxls(test_case, src_pixels);

		/* Prepare textures */
		if (GL_RENDERBUFFER == test_case.m_target)
		{
			testCase desc	= test_case;
			GLuint   ignored = 0;

			desc.m_target = GL_TEXTURE_2D;

			m_rb_name	  = prepareTexture(test_case, 0 /* pixels */, ignored /* buffer name */);
			m_dst_tex_name = prepareTexture(desc, dst_pixels, m_dst_buf_name);
			m_src_tex_name = prepareTexture(desc, src_pixels, m_src_buf_name);
		}
		else
		{
			m_dst_tex_name = prepareTexture(test_case, dst_pixels, m_dst_buf_name);
			m_src_tex_name = prepareTexture(test_case, src_pixels, m_src_buf_name);
		}

		/* Copy images and verify results */
		result = copyAndVerify(test_case, src_pixels);
	}
	catch (tcu::Exception& exc)
	{
		clean();
		cleanPixels(dst_pixels);
		cleanPixels(src_pixels);
		throw exc;
	}

	/* Free resources */
	clean();
	cleanPixels(dst_pixels);
	cleanPixels(src_pixels);

	/* Set result */
	if (true == result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

		/* Increase index */
		m_test_case_index += 1;

		/* Are there any test cases left */
		if (m_test_cases.size() > m_test_case_index)
		{
			it_result = tcu::TestNode::CONTINUE;
		}
	}
	else
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Failure. " << tcu::TestLog::EndMessage;

		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return it_result;
}

/** Prepare data for destination image
 *
 * @param test_case  Test case descriptor
 * @param out_pixels Pointer to image data
 **/
void SmokeTest::prepareDstPxls(const SmokeTest::testCase& test_case, GLubyte*& out_pixels) const
{
	static const GLuint n_pixels_per_layer = m_width * m_height;

	const GLenum internal_format = test_case.m_internal_format;
	const GLuint n_layers		 = (GL_TEXTURE_CUBE_MAP_ARRAY != test_case.m_target) ? m_depth : 6;
	const GLuint n_pixels		 = n_pixels_per_layer * n_layers;
	const GLuint pixel_size		 = Utils::getPixelSizeForFormat(internal_format);
	const GLuint req_memory		 = pixel_size * n_pixels;
	const GLenum type			 = test_case.m_type;

	/* Prepare storage */
	out_pixels = new GLubyte[req_memory];

	if (0 == out_pixels)
	{
		TCU_FAIL("Memory allocation failed");
	}

	memset(out_pixels, 0, req_memory);

	/* Fill pixels */
	for (GLuint j = 0; j < n_pixels; ++j)
	{
		GLubyte* pixel_data = out_pixels + j * pixel_size;

		Utils::packPixel(internal_format, type, 1.0, 1.0, 1.0, 1.0, pixel_data);
	}
}

/** Prepare data for source image
 *
 * @param test_case  Test case descriptor
 * @param out_pixels Pointer to image data
 **/
void SmokeTest::prepareSrcPxls(const SmokeTest::testCase& test_case, GLubyte*& out_pixels) const
{
	static const GLuint n_pixels_per_layer = m_width * m_height;

	const GLenum internal_format = test_case.m_internal_format;
	const GLuint n_layers		 = (GL_TEXTURE_CUBE_MAP_ARRAY != test_case.m_target) ? m_depth : 6;
	const GLuint n_pixels		 = n_pixels_per_layer * n_layers;
	const GLuint pixel_size		 = Utils::getPixelSizeForFormat(internal_format);
	const GLuint layer_size		 = pixel_size * n_pixels_per_layer;
	const GLuint line_size		 = pixel_size * m_width;
	const GLuint req_memory		 = pixel_size * n_pixels;
	const GLenum type			 = test_case.m_type;

	/* Prepare storage */
	out_pixels = new GLubyte[req_memory];

	if (0 == out_pixels)
	{
		TCU_FAIL("Memory allocation failed");
	}

	memset(out_pixels, 0, req_memory);

	/* Fill pixels */
	for (GLuint layer = 0; layer < n_layers; ++layer)
	{
		const GLuint layer_offset = layer * layer_size;

		GLubyte* layer_data = out_pixels + layer_offset;

		for (GLuint y = 0; y < m_height; ++y)
		{
			const GLuint line_offset = line_size * y;

			GLubyte* line_data = layer_data + line_offset;

			for (GLuint x = 0; x < m_width; ++x)
			{
				const GLuint pixel_offset = x * pixel_size;

				GLubyte* pixel_data = line_data + pixel_offset;

				/* 255 is max ubyte. 1/15.9375 = 16/255 */
				const GLdouble red   = ((GLdouble)x) / 255.0 + (((GLdouble)y) / 15.9375);
				const GLdouble green = ((GLdouble)layer) / 255.0 + (1.0 / 15.9375);
				const GLdouble blue  = 0.125;
				const GLdouble alpha = 1.0; /* This value has special meaning for some internal_formats */

				Utils::packPixel(internal_format, type, red, green, blue, alpha, pixel_data);
			}
		}
	}
}

/** Prepare texture
 *
 * @param desc       Descriptor
 * @param pixels     Image data
 * @param out_buf_id Id of buffer used by texture buffer
 *
 * @return Name of iamge
 **/
GLuint SmokeTest::prepareTexture(const testCase& test_case, const GLubyte* pixels, GLuint& out_buf_id)
{
	const GLuint n_layers = (GL_TEXTURE_CUBE_MAP_ARRAY != test_case.m_target) ? m_depth : 6;
	GLuint		 name	 = Utils::generateTexture(m_context, test_case.m_target);

	Utils::prepareTexture(m_context, name, test_case.m_target, test_case.m_internal_format, test_case.m_format,
						  test_case.m_type, 0 /* level */, m_width, m_height, n_layers, pixels, out_buf_id);

	Utils::makeTextureComplete(m_context, test_case.m_target, name, 0 /* base */, 0 /* max */);

	return name;
}

/** Verify copy operation
 *
 * @param test_case  Test case
 * @param src_pixels Original data of source image
 *
 * @return true if everything is as expected, false otherwise
 **/
bool SmokeTest::verify(const testCase& test_case, const GLubyte* src_pixels)
{
	std::vector<GLubyte> dst_data;
	const GLuint		 n_layers   = (GL_TEXTURE_CUBE_MAP_ARRAY != test_case.m_target) ? m_depth : 6;
	const GLuint		 pixel_size = Utils::getPixelSizeForFormat(test_case.m_internal_format);
	const GLuint		 line_size  = pixel_size * m_width;
	const GLuint		 req_memory = line_size * m_height * n_layers;
	std::vector<GLubyte> src_data;

	/* Prepare storage */
	dst_data.resize(req_memory);
	src_data.resize(req_memory);

	/* Check if source image was not modified */
	{
		getPixels(m_src_tex_name, test_case, &src_data[0]);

		if (false == compareImages(test_case, src_pixels, &src_data[0]))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "CopyImageSubData modified contents of source image. Original data: left."
				<< tcu::TestLog::EndMessage;
			return false;
		}
	}

	/* Check contents of destination image */
	{
		getPixels(m_dst_tex_name, test_case, &dst_data[0]);

		if (false == compareImages(test_case, src_pixels, &dst_data[0]))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "CopyImageSubData stored invalid contents in destination image. Source data: left."
				<< tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/* InvalidTargetTest */
/** Constructor
 *
 * @param context Text context
 **/
InvalidTargetTest::InvalidTargetTest(deqp::Context& context)
	: TestCase(context, "invalid_target",
			   "Test verifies if INVALID_ENUM is generated when invalid target is provided to CopyImageSubData")
	, m_dst_buf_name(0)
	, m_dst_tex_name(0)
	, m_src_buf_name(0)
	, m_src_tex_name(0)
	, m_test_case_index(0)
{

	/* Valid source, valid dst */
	for (GLuint src = 0; src < s_n_valid_targets; ++src)
	{
		for (GLuint dst = 0; dst < s_n_valid_targets; ++dst)
		{
			const GLenum src_target = s_valid_targets[src];
			const GLenum dst_target = s_valid_targets[dst];
			testCase	 test_case  = { src_target, dst_target, GL_NO_ERROR };

			if (Utils::isTargetMultisampled(dst_target) != Utils::isTargetMultisampled(src_target))
			{
				test_case.m_expected_result = GL_INVALID_OPERATION;
			}

			m_test_cases.push_back(test_case);
		}
	}

	/* Invalid source, invalid dst */
	for (GLuint src = 0; src < s_n_invalid_targets; ++src)
	{
		for (GLuint dst = 0; dst < s_n_invalid_targets; ++dst)
		{
			const GLenum   src_target = s_invalid_targets[src];
			const GLenum   dst_target = s_invalid_targets[dst];
			const testCase test_case  = { src_target, dst_target, GL_INVALID_ENUM };

			m_test_cases.push_back(test_case);
		}
	}

	/* Invalid source, valid dst */
	for (GLuint src = 0; src < s_n_invalid_targets; ++src)
	{
		for (GLuint dst = 0; dst < s_n_valid_targets; ++dst)
		{
			const GLenum   src_target = s_invalid_targets[src];
			const GLenum   dst_target = s_valid_targets[dst];
			const testCase test_case  = { src_target, dst_target, GL_INVALID_ENUM };

			m_test_cases.push_back(test_case);
		}
	}

	/* Valid source, invalid dst */
	for (GLuint src = 0; src < s_n_valid_targets; ++src)
	{
		for (GLuint dst = 0; dst < s_n_invalid_targets; ++dst)
		{
			const GLenum   src_target = s_valid_targets[src];
			const GLenum   dst_target = s_invalid_targets[dst];
			const testCase test_case  = { src_target, dst_target, GL_INVALID_ENUM };

			m_test_cases.push_back(test_case);
		}
	}
}

/** Execute test
 *
 * @return CONTINUE as long there are more test case, STOP otherwise
 **/
tcu::TestNode::IterateResult InvalidTargetTest::iterate()
{
	GLenum						 error	 = GL_NO_ERROR;
	const Functions&			 gl		   = m_context.getRenderContext().getFunctions();
	tcu::TestNode::IterateResult it_result = tcu::TestNode::STOP;
	bool						 result	= false;
	const testCase&				 test_case = m_test_cases[m_test_case_index];

	try
	{
		/* Prepare textures */
		m_dst_tex_name = Utils::prepareTex16x16x6(m_context, test_case.m_dst_target, GL_RGBA8, GL_RGBA,
												  GL_UNSIGNED_BYTE, m_dst_buf_name);
		m_src_tex_name = Utils::prepareTex16x16x6(m_context, test_case.m_src_target, GL_RGBA8, GL_RGBA,
												  GL_UNSIGNED_BYTE, m_src_buf_name);

		/* Make textures complete */
		Utils::makeTextureComplete(m_context, test_case.m_dst_target, m_dst_tex_name, 0 /* base */, 0 /* max */);
		Utils::makeTextureComplete(m_context, test_case.m_src_target, m_src_tex_name, 0 /* base */, 0 /* max */);
	}
	catch (tcu::Exception& exc)
	{
		clean();
		throw exc;
	}

	/* Execute CopyImageSubData */
	gl.copyImageSubData(m_src_tex_name, test_case.m_src_target, 0 /* srcLevel */, 0 /* srcX */, 0 /* srcY */,
						0 /* srcZ */, m_dst_tex_name, test_case.m_dst_target, 0 /* dstLevel */, 0 /* dstX */,
						0 /* dstY */, 0 /* dstZ */, 1 /* srcWidth */, 1 /* srcHeight */, 1 /* srcDepth */);

	/* Verify generated error */
	error  = gl.getError();
	result = (test_case.m_expected_result == error);

	/* Free resources */
	clean();

	/* Set result */
	if (true == result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

		/* Increase index */
		m_test_case_index += 1;

		/* Are there any test cases left */
		if (m_test_cases.size() > m_test_case_index)
		{
			it_result = tcu::TestNode::CONTINUE;
		}
	}
	else
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Failure. Expected result: " << glu::getErrorStr(test_case.m_expected_result)
			<< " got: " << glu::getErrorStr(error)
			<< ". Source target: " << glu::getTextureTargetStr(test_case.m_src_target)
			<< ", destination target: " << glu::getTextureTargetStr(test_case.m_dst_target) << tcu::TestLog::EndMessage;

		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return it_result;
}

/** Cleans resources
 *
 **/
void InvalidTargetTest::clean()
{
	const Functions& gl		   = m_context.getRenderContext().getFunctions();
	const testCase&  test_case = m_test_cases[m_test_case_index];

	/* Clean textures and buffers. Errors ignored */
	Utils::deleteTexture(m_context, test_case.m_dst_target, m_dst_tex_name);
	Utils::deleteTexture(m_context, test_case.m_src_target, m_src_tex_name);

	m_dst_tex_name = 0;
	m_src_tex_name = 0;

	if (0 != m_dst_buf_name)
	{
		gl.deleteBuffers(1, &m_dst_buf_name);
		m_dst_buf_name = 0;
	}

	if (0 != m_src_buf_name)
	{
		gl.deleteBuffers(1, &m_src_buf_name);
		m_src_buf_name = 0;
	}
}

/* TargetMismatchTest */
/** Constructor
 *
 * @param context Text context
 **/
TargetMismatchTest::TargetMismatchTest(deqp::Context& context)
	: TestCase(
		  context, "target_miss_match",
		  "Test verifies if INVALID_ENUM is generated when target provided to CopyImageSubData does not match texture")
	, m_dst_buf_name(0)
	, m_dst_tex_name(0)
	, m_src_buf_name(0)
	, m_src_tex_name(0)
	, m_test_case_index(0)
{
	/* Wrong dst target */
	for (GLuint target = 0; target < s_n_valid_targets; ++target)
	{
		for (GLuint dst = 0; dst < s_n_valid_targets; ++dst)
		{
			const GLenum dst_target = s_valid_targets[dst];
			const GLenum src_target = s_valid_targets[target];
			const GLenum tex_target = s_valid_targets[target];
			testCase	 test_case  = { tex_target, src_target, dst_target, GL_INVALID_ENUM };

			/* Skip renderbuffers */
			if ((GL_RENDERBUFFER == tex_target) || (GL_RENDERBUFFER == dst_target) || (GL_RENDERBUFFER == src_target))
			{
				continue;
			}

			/* Valid case */
			if (dst_target == tex_target)
			{
				test_case.m_expected_result = GL_NO_ERROR;
			}

			/* Skip cases with multisampling conflict */
			if (Utils::isTargetMultisampled(dst_target) != Utils::isTargetMultisampled(src_target))
			{
				continue;
			}

			m_test_cases.push_back(test_case);
		}
	}

	/* Wrong src target */
	for (GLuint target = 0; target < s_n_valid_targets; ++target)
	{
		for (GLuint src = 0; src < s_n_valid_targets; ++src)
		{
			const GLenum dst_target = s_valid_targets[target];
			const GLenum src_target = s_valid_targets[src];
			const GLenum tex_target = s_valid_targets[target];
			testCase	 test_case  = { tex_target, src_target, dst_target, GL_INVALID_ENUM };

			/* Skip renderbuffers */
			if ((GL_RENDERBUFFER == tex_target) || (GL_RENDERBUFFER == dst_target) || (GL_RENDERBUFFER == src_target))
			{
				continue;
			}

			/* Valid case */
			if (src_target == tex_target)
			{
				test_case.m_expected_result = GL_NO_ERROR;
			}

			/* Skip cases with multisampling conflict */
			if (Utils::isTargetMultisampled(dst_target) != Utils::isTargetMultisampled(src_target))
			{
				continue;
			}

			m_test_cases.push_back(test_case);
		}
	}
}

/** Execute test
 *
 * @return CONTINUE as long there are more test case, STOP otherwise
 **/
tcu::TestNode::IterateResult TargetMismatchTest::iterate()
{
	GLenum						 error	 = GL_NO_ERROR;
	const Functions&			 gl		   = m_context.getRenderContext().getFunctions();
	tcu::TestNode::IterateResult it_result = tcu::TestNode::STOP;
	bool						 result	= false;
	const testCase&				 test_case = m_test_cases[m_test_case_index];

	try
	{
		/* Prepare textures */
		m_dst_tex_name = Utils::prepareTex16x16x6(m_context, test_case.m_tex_target, GL_RGBA8, GL_RGBA,
												  GL_UNSIGNED_BYTE, m_dst_buf_name);
		m_src_tex_name = Utils::prepareTex16x16x6(m_context, test_case.m_tex_target, GL_RGBA8, GL_RGBA,
												  GL_UNSIGNED_BYTE, m_src_buf_name);

		/* Make textures complete */
		Utils::makeTextureComplete(m_context, test_case.m_tex_target, m_dst_tex_name, 0 /* base */, 0 /* max */);
		Utils::makeTextureComplete(m_context, test_case.m_tex_target, m_src_tex_name, 0 /* base */, 0 /* max */);
	}
	catch (tcu::Exception& exc)
	{
		clean();
		throw exc;
	}

	/* Execute CopyImageSubData */
	gl.copyImageSubData(m_src_tex_name, test_case.m_src_target, 0 /* srcLevel */, 0 /* srcX */, 0 /* srcY */,
						0 /* srcZ */, m_dst_tex_name, test_case.m_dst_target, 0 /* dstLevel */, 0 /* dstX */,
						0 /* dstY */, 0 /* dstZ */, 1 /* srcWidth */, 1 /* srcHeight */, 1 /* srcDepth */);

	/* Verify generated error */
	error  = gl.getError();
	result = (test_case.m_expected_result == error);

	/* Remove resources */
	clean();

	if (true == result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

		/* Increase index */
		m_test_case_index += 1;

		/* Are there any test cases left */
		if (m_test_cases.size() > m_test_case_index)
		{
			it_result = tcu::TestNode::CONTINUE;
		}
	}
	else
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Failure. Expected result: " << glu::getErrorStr(test_case.m_expected_result)
			<< " got: " << glu::getErrorStr(error)
			<< ". Texture target: " << glu::getTextureTargetStr(test_case.m_tex_target)
			<< ". Source target: " << glu::getTextureTargetStr(test_case.m_src_target)
			<< ", destination target: " << glu::getTextureTargetStr(test_case.m_dst_target) << tcu::TestLog::EndMessage;

		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return it_result;
}

/** Cleans resources
 *
 **/
void TargetMismatchTest::clean()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean textures and buffers. Errors ignored */
	gl.deleteTextures(1, &m_dst_tex_name);
	gl.deleteTextures(1, &m_src_tex_name);

	m_dst_tex_name = 0;
	m_src_tex_name = 0;

	if (0 != m_dst_buf_name)
	{
		gl.deleteBuffers(1, &m_dst_buf_name);
		m_dst_buf_name = 0;
	}

	if (0 != m_src_buf_name)
	{
		gl.deleteBuffers(1, &m_src_buf_name);
		m_src_buf_name = 0;
	}
}

/* TargetMismatchTest */
/** Constructor
 *
 * @param context Text context
 **/
IncompleteTexTest::IncompleteTexTest(deqp::Context& context)
	: TestCase(
		  context, "incomplete_tex",
		  "Test verifies if INVALID_OPERATION is generated when texture provided to CopyImageSubData is incomplete")
	, m_dst_buf_name(0)
	, m_dst_tex_name(0)
	, m_src_buf_name(0)
	, m_src_tex_name(0)
	, m_test_case_index(0)
{
	for (GLuint target = 0; target < s_n_valid_targets; ++target)
	{
		const GLenum tex_target = s_valid_targets[target];
		testCase	 test_case  = { tex_target, false, false, GL_INVALID_OPERATION };

		/* Skip targets that are not multi level */
		if (false == Utils::isTargetMultilevel(tex_target))
		{
			continue;
		}

		m_test_cases.push_back(test_case);

		test_case.m_is_dst_complete = true;
		test_case.m_is_src_complete = false;
		m_test_cases.push_back(test_case);

		test_case.m_is_dst_complete = false;
		test_case.m_is_src_complete = true;
		m_test_cases.push_back(test_case);

		test_case.m_is_dst_complete = true;
		test_case.m_is_src_complete = true;
		test_case.m_expected_result = GL_NO_ERROR;
		m_test_cases.push_back(test_case);
	}
}

/** Execute test
 *
 * @return CONTINUE as long there are more test case, STOP otherwise
 **/
tcu::TestNode::IterateResult IncompleteTexTest::iterate()
{
	GLenum						 error	 = GL_NO_ERROR;
	const Functions&			 gl		   = m_context.getRenderContext().getFunctions();
	tcu::TestNode::IterateResult it_result = tcu::TestNode::STOP;
	bool						 result	= false;
	const testCase&				 test_case = m_test_cases[m_test_case_index];

	try
	{
		/* Prepare textures */
		m_dst_tex_name = Utils::prepareTex16x16x6(m_context, test_case.m_tex_target, GL_RGBA8, GL_RGBA,
												  GL_UNSIGNED_BYTE, m_dst_buf_name);
		m_src_tex_name = Utils::prepareTex16x16x6(m_context, test_case.m_tex_target, GL_RGBA8, GL_RGBA,
												  GL_UNSIGNED_BYTE, m_src_buf_name);

		/* Make textures complete */
		if (true == test_case.m_is_dst_complete)
		{
			Utils::makeTextureComplete(m_context, test_case.m_tex_target, m_dst_tex_name, 0 /* base */, 0 /* max */);
		}

		if (true == test_case.m_is_src_complete)
		{
			Utils::makeTextureComplete(m_context, test_case.m_tex_target, m_src_tex_name, 0 /* base */, 0 /* max */);
		}
	}
	catch (tcu::Exception& exc)
	{
		clean();
		throw exc;
	}

	/* Execute CopyImageSubData */
	gl.copyImageSubData(m_src_tex_name, test_case.m_tex_target, 0 /* srcLevel */, 0 /* srcX */, 0 /* srcY */,
						0 /* srcZ */, m_dst_tex_name, test_case.m_tex_target, 0 /* dstLevel */, 0 /* dstX */,
						0 /* dstY */, 0 /* dstZ */, 1 /* srcWidth */, 1 /* srcHeight */, 1 /* srcDepth */);

	/* Verify generated error */
	error  = gl.getError();
	result = (test_case.m_expected_result == error);

	/* Remove resources */
	clean();

	if (true == result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

		/* Increase index */
		m_test_case_index += 1;

		/* Are there any test cases left */
		if (m_test_cases.size() > m_test_case_index)
		{
			it_result = tcu::TestNode::CONTINUE;
		}
	}
	else
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Failure. Expected result: " << glu::getErrorStr(test_case.m_expected_result)
			<< " got: " << glu::getErrorStr(error)
			<< ". Texture target: " << glu::getTextureTargetStr(test_case.m_tex_target)
			<< ". Is source complete: " << test_case.m_is_src_complete
			<< ", is destination complete: " << test_case.m_is_dst_complete << tcu::TestLog::EndMessage;

		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return it_result;
}

/** Cleans resources
 *
 **/
void IncompleteTexTest::clean()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean textures and buffers. Errors ignored */
	gl.deleteTextures(1, &m_dst_tex_name);
	gl.deleteTextures(1, &m_src_tex_name);

	m_dst_tex_name = 0;
	m_src_tex_name = 0;

	if (0 != m_dst_buf_name)
	{
		gl.deleteBuffers(1, &m_dst_buf_name);
		m_dst_buf_name = 0;
	}

	if (0 != m_src_buf_name)
	{
		gl.deleteBuffers(1, &m_src_buf_name);
		m_src_buf_name = 0;
	}
}

/* IncompatibleFormatsTest */
/** Constructor
 *
 * @param context Text context
 **/
IncompatibleFormatsTest::IncompatibleFormatsTest(deqp::Context& context)
	: TestCase(
		  context, "incompatible_formats",
		  "Test verifies if INVALID_OPERATION is generated when textures provided to CopyImageSubData are incompatible")
	, m_dst_buf_name(0)
	, m_dst_tex_name(0)
	, m_src_buf_name(0)
	, m_src_tex_name(0)
	, m_test_case_index(0)
{
	/* RGBA8UI vs RGBA16UI */
	for (GLuint target = 0; target < s_n_valid_targets; ++target)
	{
		const GLenum tex_target = s_valid_targets[target];

		testCase test_case = { tex_target,  GL_RGBA8UI,		 GL_RGBA_INTEGER,   GL_UNSIGNED_BYTE,
							   GL_RGBA16UI, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, GL_INVALID_OPERATION };

		/* Skip multisampled and rectangle targets */
		if (true == Utils::isTargetMultisampled(tex_target))
		{
			continue;
		}

		m_test_cases.push_back(test_case);
	}

	/* RGBA8UI vs RGBA32UI */
	for (GLuint target = 0; target < s_n_valid_targets; ++target)
	{
		const GLenum tex_target = s_valid_targets[target];

		testCase test_case = { tex_target,  GL_RGBA8UI,		 GL_RGBA_INTEGER, GL_UNSIGNED_BYTE,
							   GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, GL_INVALID_OPERATION };

		/* Skip multisampled and rectangle targets */
		if (true == Utils::isTargetMultisampled(tex_target))
		{
			continue;
		}

		m_test_cases.push_back(test_case);
	}

	/* RGBA16UI vs RG16UI */
	for (GLuint target = 0; target < s_n_valid_targets; ++target)
	{
		const GLenum tex_target = s_valid_targets[target];

		testCase test_case = { tex_target, GL_RGBA16UI,   GL_RGBA_INTEGER,   GL_UNSIGNED_SHORT,
							   GL_RG16UI,  GL_RG_INTEGER, GL_UNSIGNED_SHORT, GL_INVALID_OPERATION };

		/* Skip multisampled and rectangle targets */
		if (true == Utils::isTargetMultisampled(tex_target))
		{
			continue;
		}

		m_test_cases.push_back(test_case);
	}

	/* RGBA32UI vs RGBA32F */
	for (GLuint target = 0; target < s_n_valid_targets; ++target)
	{
		const GLenum tex_target = s_valid_targets[target];

		testCase test_case = { tex_target, GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT,
							   GL_RGBA32F, GL_RGBA,		GL_FLOAT,		 GL_NO_ERROR };

		/* Skip multisampled and rectangle targets */
		if (true == Utils::isTargetMultisampled(tex_target))
		{
			continue;
		}

		m_test_cases.push_back(test_case);
	}

	/* RGBA8 vs RGBA32F */
	for (GLuint target = 0; target < s_n_valid_targets; ++target)
	{
		const GLenum tex_target = s_valid_targets[target];

		testCase test_case = { tex_target, GL_RGBA8, GL_RGBA,  GL_UNSIGNED_BYTE,
							   GL_RGBA32F, GL_RGBA,  GL_FLOAT, GL_INVALID_OPERATION };

		/* Skip multisampled and rectangle targets */
		if (true == Utils::isTargetMultisampled(tex_target))
		{
			continue;
		}

		m_test_cases.push_back(test_case);
	}
}

/** Execute test
 *
 * @return CONTINUE as long there are more test case, STOP otherwise
 **/
tcu::TestNode::IterateResult IncompatibleFormatsTest::iterate()
{
	GLenum						 error	 = GL_NO_ERROR;
	const Functions&			 gl		   = m_context.getRenderContext().getFunctions();
	tcu::TestNode::IterateResult it_result = tcu::TestNode::STOP;
	bool						 result	= false;
	const testCase&				 test_case = m_test_cases[m_test_case_index];

	try
	{
		/* Prepare textures */
		m_dst_tex_name = Utils::prepareTex16x16x6(m_context, test_case.m_tex_target, test_case.m_dst_internal_format,
												  test_case.m_dst_format, test_case.m_dst_type, m_dst_buf_name);
		m_src_tex_name = Utils::prepareTex16x16x6(m_context, test_case.m_tex_target, test_case.m_src_internal_format,
												  test_case.m_src_format, test_case.m_src_type, m_src_buf_name);

		/* Make textures complete */
		Utils::makeTextureComplete(m_context, test_case.m_tex_target, m_dst_tex_name, 0 /* base */, 0 /* max */);
		Utils::makeTextureComplete(m_context, test_case.m_tex_target, m_src_tex_name, 0 /* base */, 0 /* max */);
	}
	catch (tcu::Exception& exc)
	{
		clean();
		throw exc;
	}

	/* Execute CopyImageSubData */
	gl.copyImageSubData(m_src_tex_name, test_case.m_tex_target, 0 /* srcLevel */, 0 /* srcX */, 0 /* srcY */,
						0 /* srcZ */, m_dst_tex_name, test_case.m_tex_target, 0 /* dstLevel */, 0 /* dstX */,
						0 /* dstY */, 0 /* dstZ */, 1 /* srcWidth */, 1 /* srcHeight */, 1 /* srcDepth */);

	/* Verify generated error */
	error  = gl.getError();
	result = (test_case.m_expected_result == error);

	/* Remove resources */
	clean();

	if (true == result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

		/* Increase index */
		m_test_case_index += 1;

		/* Are there any test cases left */
		if (m_test_cases.size() > m_test_case_index)
		{
			it_result = tcu::TestNode::CONTINUE;
		}
	}
	else
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Failure. Expected result: " << glu::getErrorStr(test_case.m_expected_result)
			<< " got: " << glu::getErrorStr(error)
			<< ". Texture target: " << glu::getTextureTargetStr(test_case.m_tex_target)
			<< ". Source format: " << glu::getInternalFormatParameterStr(test_case.m_src_internal_format)
			<< ". Destination format: " << glu::getInternalFormatParameterStr(test_case.m_dst_internal_format)
			<< tcu::TestLog::EndMessage;

		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return it_result;
}

/** Cleans resources
 *
 **/
void IncompatibleFormatsTest::clean()
{
	const Functions& gl		   = m_context.getRenderContext().getFunctions();
	const testCase&  test_case = m_test_cases[m_test_case_index];

	/* Clean textures and buffers. Errors ignored */
	Utils::deleteTexture(m_context, test_case.m_tex_target, m_dst_tex_name);
	Utils::deleteTexture(m_context, test_case.m_tex_target, m_src_tex_name);

	m_dst_tex_name = 0;
	m_src_tex_name = 0;

	if (0 != m_dst_buf_name)
	{
		gl.deleteBuffers(1, &m_dst_buf_name);
		m_dst_buf_name = 0;
	}

	if (0 != m_src_buf_name)
	{
		gl.deleteBuffers(1, &m_src_buf_name);
		m_src_buf_name = 0;
	}
}

/* InvalidTargetTest */
/** Constructor
 *
 * @param context Text context
 **/
SamplesMismatchTest::SamplesMismatchTest(deqp::Context& context)
	: TestCase(context, "samples_mismatch", "Test verifies if INVALID_OPERATION is generated when textures provided "
											"to CopyImageSubData have different number of samples")
	, m_dst_tex_name(0)
	, m_src_tex_name(0)
	, m_test_case_index(0)
{
	testCase test_case;

	static const GLsizei n_samples[2] = { 1, 4 };

	static const GLenum targets[2] = { GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_2D_MULTISAMPLE_ARRAY };

	for (GLuint src_sample = 0; src_sample < 2; ++src_sample)
	{
		for (GLuint dst_sample = 0; dst_sample < 2; ++dst_sample)
		{
			for (GLuint src_target = 0; src_target < 2; ++src_target)
			{
				for (GLuint dst_target = 0; dst_target < 2; ++dst_target)
				{
					test_case.m_src_target	= targets[src_target];
					test_case.m_src_n_samples = n_samples[src_sample];
					test_case.m_dst_target	= targets[dst_target];
					test_case.m_dst_n_samples = n_samples[dst_sample];

					if (test_case.m_src_n_samples == test_case.m_dst_n_samples)
					{
						test_case.m_expected_result = GL_NO_ERROR;
					}
					else
					{
						test_case.m_expected_result = GL_INVALID_OPERATION;
					}

					m_test_cases.push_back(test_case);
				}
			}
		}
	}
}

/** Execute test
 *
 * @return CONTINUE as long there are more test case, STOP otherwise
 **/
tcu::TestNode::IterateResult SamplesMismatchTest::iterate()
{
	GLenum						 error	 = GL_NO_ERROR;
	const Functions&			 gl		   = m_context.getRenderContext().getFunctions();
	tcu::TestNode::IterateResult it_result = tcu::TestNode::STOP;
	bool						 result	= false;
	const testCase&				 test_case = m_test_cases[m_test_case_index];

	try
	{
		/* Prepare textures */
		m_dst_tex_name = Utils::prepareMultisampleTex(m_context, test_case.m_dst_target, test_case.m_dst_n_samples);
		m_src_tex_name = Utils::prepareMultisampleTex(m_context, test_case.m_src_target, test_case.m_src_n_samples);
	}
	catch (tcu::Exception& exc)
	{
		clean();
		throw exc;
	}

	/* Execute CopyImageSubData */
	gl.copyImageSubData(m_src_tex_name, test_case.m_src_target, 0 /* srcLevel */, 0 /* srcX */, 0 /* srcY */,
						0 /* srcZ */, m_dst_tex_name, test_case.m_dst_target, 0 /* dstLevel */, 0 /* dstX */,
						0 /* dstY */, 0 /* dstZ */, 1 /* srcWidth */, 1 /* srcHeight */, 1 /* srcDepth */);

	/* Verify generated error */
	error  = gl.getError();
	result = (test_case.m_expected_result == error);

	/* Free resources */
	clean();

	/* Set result */
	if (true == result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

		/* Increase index */
		m_test_case_index += 1;

		/* Are there any test cases left */
		if (m_test_cases.size() > m_test_case_index)
		{
			it_result = tcu::TestNode::CONTINUE;
		}
	}
	else
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Failure. Expected result: " << glu::getErrorStr(test_case.m_expected_result)
			<< " got: " << glu::getErrorStr(error)
			<< ". Source target: " << glu::getTextureTargetStr(test_case.m_src_target)
			<< " samples: " << test_case.m_src_n_samples
			<< ", destination target: " << glu::getTextureTargetStr(test_case.m_dst_target)
			<< " samples: " << test_case.m_dst_n_samples << tcu::TestLog::EndMessage;

		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return it_result;
}

/** Cleans resources
 *
 **/
void SamplesMismatchTest::clean()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean textures . Errors ignored */
	gl.deleteTextures(1, &m_dst_tex_name);
	gl.deleteTextures(1, &m_src_tex_name);

	m_dst_tex_name = 0;
	m_src_tex_name = 0;
}

/* IncompatibleFormatsCompressionTest */
/** Constructor
 *
 * @param context Text context
 **/
IncompatibleFormatsCompressionTest::IncompatibleFormatsCompressionTest(deqp::Context& context)
	: TestCase(context, "incompatible_formats_compression", "Test verifies if INVALID_OPERATION is generated when "
															"textures provided to CopyImageSubData are incompatible, "
															"one of formats is compressed")
	, m_dst_tex_name(0)
	, m_src_tex_name(0)
	, m_test_case_index(0)
{
	for (GLuint target = 0; target < s_n_valid_targets; ++target)
	{
		const GLenum tex_target = s_valid_targets[target];

		/* Skip 1D targets, not supported */
		if ((GL_TEXTURE_1D == tex_target) || (GL_TEXTURE_1D_ARRAY == tex_target) || (GL_TEXTURE_3D == tex_target) ||
			(GL_TEXTURE_RECTANGLE == tex_target) || (GL_RENDERBUFFER == tex_target) || (GL_TEXTURE_CUBE_MAP_ARRAY == tex_target))
		{
			continue;
		}

		/* Skip multisampled and rectangle targets */
		if (true == Utils::isTargetMultisampled(tex_target))
		{
			continue;
		}

		/* Compressed 128bit vs RGBA32UI */
		{
			testCase test_case = {
				tex_target, GL_RGBA32UI,	  GL_RGBA_INTEGER, GL_UNSIGNED_INT, GL_COMPRESSED_RG_RGTC2,
				GL_RG,		GL_UNSIGNED_BYTE, GL_NO_ERROR
			};

			m_test_cases.push_back(test_case);
		}

		/* Compressed 128bit vs RGBA16UI */
		{
			testCase test_case = {
				tex_target, GL_RGBA16UI,	  GL_RGBA_INTEGER,	 GL_UNSIGNED_SHORT, GL_COMPRESSED_RG_RGTC2,
				GL_RG,		GL_UNSIGNED_BYTE, GL_INVALID_OPERATION
			};

			m_test_cases.push_back(test_case);
		}
	}
}

/** Execute test
 *
 * @return CONTINUE as long there are more test case, STOP otherwise
 **/
tcu::TestNode::IterateResult IncompatibleFormatsCompressionTest::iterate()
{
	GLenum						 error	 = GL_NO_ERROR;
	const Functions&			 gl		   = m_context.getRenderContext().getFunctions();
	tcu::TestNode::IterateResult it_result = tcu::TestNode::STOP;
	bool						 result	= false;
	GLuint						 temp	  = 0;
	const testCase&				 test_case = m_test_cases[m_test_case_index];

	try
	{
		/* Prepare textures */
		m_dst_tex_name = Utils::prepareTex16x16x6(m_context, test_case.m_tex_target, test_case.m_dst_internal_format,
												  test_case.m_dst_format, test_case.m_dst_type, temp);
		m_src_tex_name = Utils::prepareTex16x16x6(m_context, test_case.m_tex_target, test_case.m_src_internal_format,
												  test_case.m_src_format, test_case.m_src_type, temp);

		/* Make textures complete */
		Utils::makeTextureComplete(m_context, test_case.m_tex_target, m_dst_tex_name, 0 /* base */, 0 /* max */);
		Utils::makeTextureComplete(m_context, test_case.m_tex_target, m_src_tex_name, 0 /* base */, 0 /* max */);
	}
	catch (tcu::Exception& exc)
	{
		clean();
		throw exc;
	}

	/* Execute CopyImageSubData */
	gl.copyImageSubData(m_src_tex_name, test_case.m_tex_target, 0 /* srcLevel */, 0 /* srcX */, 0 /* srcY */,
						0 /* srcZ */, m_dst_tex_name, test_case.m_tex_target, 0 /* dstLevel */, 0 /* dstX */,
						0 /* dstY */, 0 /* dstZ */, 4 /* srcWidth */, 4 /* srcHeight */, 1 /* srcDepth */);

	/* Verify generated error */
	error  = gl.getError();
	result = (test_case.m_expected_result == error);

	/* Remove resources */
	clean();

	if (true == result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

		/* Increase index */
		m_test_case_index += 1;

		/* Are there any test cases left */
		if (m_test_cases.size() > m_test_case_index)
		{
			it_result = tcu::TestNode::CONTINUE;
		}
	}
	else
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Failure. Expected result: " << glu::getErrorStr(test_case.m_expected_result)
			<< " got: " << glu::getErrorStr(error)
			<< ". Texture target: " << glu::getTextureTargetStr(test_case.m_tex_target)
			<< ". Source format: " << glu::getInternalFormatParameterStr(test_case.m_src_internal_format)
			<< ". Destination format: " << glu::getInternalFormatParameterStr(test_case.m_dst_internal_format)
			<< tcu::TestLog::EndMessage;

		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return it_result;
}

/** Cleans resources
 *
 **/
void IncompatibleFormatsCompressionTest::clean()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean textures and buffers. Errors ignored */
	gl.deleteTextures(1, &m_dst_tex_name);
	gl.deleteTextures(1, &m_src_tex_name);

	m_dst_tex_name = 0;
	m_src_tex_name = 0;
}

/* InvalidObjectTest */
/** Constructor
 *
 * @param context Text context
 **/
InvalidObjectTest::InvalidObjectTest(deqp::Context& context)
	: TestCase(
		  context, "invalid_object",
		  "Test verifies if INVALID_VALUE is generated when object & target provided to CopyImageSubData do not match")
	, m_dst_name(0)
	, m_src_name(0)
	, m_test_case_index(0)
{
	static glw::GLenum  arg_types[] = { GL_TEXTURE_2D, GL_RENDERBUFFER };
	static const GLuint n_arg_types = sizeof(arg_types) / sizeof(arg_types[0]);

	for (GLuint dst = 0; dst < n_arg_types; dst++)
	{
		for (GLuint src = 0; src < n_arg_types; src++)
		{
			for (GLuint dst_valid = 0; dst_valid < 2; dst_valid++)
			{
				for (GLuint src_valid = 0; src_valid < 2; src_valid++)
				{
					glw::GLenum expected_error = GL_INVALID_VALUE;
					if (!!src_valid && !!dst_valid)
					{
						expected_error = GL_NO_ERROR;
					}
					const testCase test_case = { arg_types[dst], !!dst_valid, arg_types[src], !!src_valid,
												 expected_error };

					m_test_cases.push_back(test_case);
				}
			}
		}
	}
}

/** Execute test
 *
 * @return CONTINUE as long there are more test case, STOP otherwise
 **/
tcu::TestNode::IterateResult InvalidObjectTest::iterate()
{
	GLenum						 error	 = GL_NO_ERROR;
	const Functions&			 gl		   = m_context.getRenderContext().getFunctions();
	tcu::TestNode::IterateResult it_result = tcu::TestNode::STOP;
	bool						 result	= false;
	GLuint						 temp	  = 0;
	const testCase&				 test_case = m_test_cases[m_test_case_index];

	try
	{
		/* Prepare objects */
		if (GL_RENDERBUFFER == test_case.m_dst_target)
		{
			m_dst_name = Utils::prepareRenderBuffer(m_context, GL_RGBA8);
		}
		else
		{
			m_dst_name =
				Utils::prepareTex16x16x6(m_context, test_case.m_dst_target, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, temp);

			/* Make textures complete */
			Utils::makeTextureComplete(m_context, test_case.m_dst_target, m_dst_name, 0 /* base */, 0 /* max */);
		}

		if (GL_RENDERBUFFER == test_case.m_src_target)
		{
			m_src_name = Utils::prepareRenderBuffer(m_context, GL_RGBA8);
		}
		else
		{
			m_src_name =
				Utils::prepareTex16x16x6(m_context, test_case.m_src_target, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, temp);

			/* Make texture complete */
			Utils::makeTextureComplete(m_context, test_case.m_src_target, m_src_name, 0 /* base */, 0 /* max */);
		}

		/* If an object is invalid, free it before use to make it invalid */
		if (!test_case.m_dst_valid)
		{
			Utils::deleteTexture(m_context, test_case.m_dst_target, m_dst_name);
		}

		if (!test_case.m_src_valid)
		{
			Utils::deleteTexture(m_context, test_case.m_src_target, m_src_name);
		}
	}
	catch (tcu::Exception& exc)
	{
		clean();
		throw exc;
	}

	/* Execute CopyImageSubData */
	gl.copyImageSubData(m_src_name, test_case.m_src_target, 0 /* srcLevel */, 0 /* srcX */, 0 /* srcY */, 0 /* srcZ */,
						m_dst_name, test_case.m_dst_target, 0 /* dstLevel */, 0 /* dstX */, 0 /* dstY */, 0 /* dstZ */,
						1 /* srcWidth */, 1 /* srcHeight */, 1 /* srcDepth */);

	/* Verify generated error */
	error  = gl.getError();
	result = (test_case.m_expected_result == error);

	/* Remove resources */
	clean();

	if (true == result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

		/* Increase index */
		m_test_case_index += 1;

		/* Are there any test cases left */
		if (m_test_cases.size() > m_test_case_index)
		{
			it_result = tcu::TestNode::CONTINUE;
		}
	}
	else
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Failure. Expected result: " << glu::getErrorStr(test_case.m_expected_result)
			<< " got: " << glu::getErrorStr(error)
			<< ". Destination target: " << glu::getTextureTargetStr(test_case.m_dst_target)
			<< ". Destination valid: " << (test_case.m_src_valid ? "true" : "false")
			<< ". Source target: " << glu::getTextureTargetStr(test_case.m_dst_target)
			<< ". Source valid: " << (test_case.m_dst_valid ? "true" : "false") << "." << tcu::TestLog::EndMessage;

		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return it_result;
}

/** Cleans resources
 *
 **/
void InvalidObjectTest::clean()
{
	const Functions& gl		   = m_context.getRenderContext().getFunctions();
	const testCase&  test_case = m_test_cases[m_test_case_index];

	/* Clean textures or renderbuffers. Errors ignored */
	if (test_case.m_dst_valid)
	{
		if (GL_RENDERBUFFER == test_case.m_dst_target)
		{
			gl.deleteRenderbuffers(1, &m_dst_name);
		}
		else
		{
			gl.deleteTextures(1, &m_dst_name);
		}
	}
	if (test_case.m_src_valid)
	{
		if (GL_RENDERBUFFER == test_case.m_src_target)
		{
			gl.deleteRenderbuffers(1, &m_src_name);
		}
		else
		{
			gl.deleteTextures(1, &m_src_name);
		}
	}

	m_dst_name = 0;
	m_src_name = 0;
}

/* NonExistentMipMapTest */
/** Constructor
 *
 * @param context Text context
 **/
NonExistentMipMapTest::NonExistentMipMapTest(deqp::Context& context)
	: TestCase(context, "non_existent_mipmap", "Test verifies if INVALID_VALUE is generated when CopyImageSubData is "
											   "executed for mipmap that does not exist")
	, m_dst_tex_name(0)
	, m_src_tex_name(0)
	, m_test_case_index(0)
{
	for (GLuint target = 0; target < s_n_valid_targets; ++target)
	{
		const GLenum tex_target = s_valid_targets[target];
		testCase	 test_case  = { tex_target, 0, 0, GL_NO_ERROR };

		if (GL_RENDERBUFFER == tex_target)
		{
			continue;
		}

		m_test_cases.push_back(test_case);

		/* Rest of cases is invalid */
		test_case.m_expected_result = GL_INVALID_VALUE;

		test_case.m_dst_level = 1;
		test_case.m_src_level = 0;
		m_test_cases.push_back(test_case);

		test_case.m_dst_level = 0;
		test_case.m_src_level = 1;
		m_test_cases.push_back(test_case);

		test_case.m_dst_level = 1;
		test_case.m_src_level = 1;
		m_test_cases.push_back(test_case);
	}
}

/** Execute test
 *
 * @return CONTINUE as long there are more test case, STOP otherwise
 **/
tcu::TestNode::IterateResult NonExistentMipMapTest::iterate()
{
	GLenum						 error	 = GL_NO_ERROR;
	const Functions&			 gl		   = m_context.getRenderContext().getFunctions();
	tcu::TestNode::IterateResult it_result = tcu::TestNode::STOP;
	bool						 result	= false;
	GLuint						 temp	  = 0;
	const testCase&				 test_case = m_test_cases[m_test_case_index];

	try
	{
		/* Prepare textures */
		m_dst_tex_name =
			Utils::prepareTex16x16x6(m_context, test_case.m_tex_target, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, temp);
		m_src_tex_name =
			Utils::prepareTex16x16x6(m_context, test_case.m_tex_target, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, temp);

		/* Make textures complete */
		Utils::makeTextureComplete(m_context, test_case.m_tex_target, m_dst_tex_name, 0 /* base */, 0 /* max */);
		Utils::makeTextureComplete(m_context, test_case.m_tex_target, m_src_tex_name, 0 /* base */, 0 /* max */);
	}
	catch (tcu::Exception& exc)
	{
		clean();
		throw exc;
	}

	/* Execute CopyImageSubData */
	gl.copyImageSubData(m_src_tex_name, test_case.m_tex_target, test_case.m_src_level, 0 /* srcX */, 0 /* srcY */,
						0 /* srcZ */, m_dst_tex_name, test_case.m_tex_target, test_case.m_dst_level, 0 /* dstX */,
						0 /* dstY */, 0 /* dstZ */, 1 /* srcWidth */, 1 /* srcHeight */, 1 /* srcDepth */);

	/* Verify generated error */
	error  = gl.getError();
	result = (test_case.m_expected_result == error);

	/* Free resources */
	clean();

	/* Set result */
	if (true == result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

		/* Increase index */
		m_test_case_index += 1;

		/* Are there any test cases left */
		if (m_test_cases.size() > m_test_case_index)
		{
			it_result = tcu::TestNode::CONTINUE;
		}
	}
	else
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Failure. Expected result: " << glu::getErrorStr(test_case.m_expected_result)
			<< " got: " << glu::getErrorStr(error)
			<< ". Texture target: " << glu::getTextureTargetStr(test_case.m_tex_target)
			<< ", source level: " << test_case.m_src_level << ", destination level: " << test_case.m_dst_level
			<< tcu::TestLog::EndMessage;

		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return it_result;
}

/** Cleans resources
 *
 **/
void NonExistentMipMapTest::clean()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean textures and buffers. Errors ignored */
	gl.deleteTextures(1, &m_dst_tex_name);
	gl.deleteTextures(1, &m_src_tex_name);

	m_dst_tex_name = 0;
	m_src_tex_name = 0;
}

/* ExceedingBoundariesTest */
const glw::GLuint ExceedingBoundariesTest::m_region_depth  = 4;
const glw::GLuint ExceedingBoundariesTest::m_region_height = 4;
const glw::GLuint ExceedingBoundariesTest::m_region_width  = 4;

/** Constructor
 *
 * @param context Text context
 **/
ExceedingBoundariesTest::ExceedingBoundariesTest(deqp::Context& context)
	: TestCase(context, "exceeding_boundaries", "Test verifies if INVALID_VALUE is generated when CopyImageSubData is "
												"executed for regions exceeding image boundaries")
	, m_dst_tex_name(0)
	, m_src_tex_name(0)
	, m_test_case_index(0)
{
	/* 16x16x6 are values used by prepareTex16x16x6 */
	static const GLuint invalid_x = 16 - (m_region_width / 2);
	static const GLuint invalid_y = 16 - (m_region_height / 2);
	static const GLuint invalid_z = 6 - (m_region_depth / 2);

	static const GLuint x_vals[] = { 0, invalid_x };
	static const GLuint y_vals[] = { 0, invalid_y };
	static const GLuint z_vals[] = { 0, invalid_z };

	static const GLuint n_x_vals = sizeof(x_vals) / sizeof(x_vals[0]);
	static const GLuint n_y_vals = sizeof(y_vals) / sizeof(y_vals[0]);
	static const GLuint n_z_vals = sizeof(z_vals) / sizeof(z_vals[0]);

	for (GLuint target = 0; target < s_n_valid_targets; ++target)
	{
		const GLenum tex_target = s_valid_targets[target];
		GLuint		 height		= m_region_height;

		if (GL_TEXTURE_BUFFER == tex_target)
		{
			continue;
		}

		if ((GL_TEXTURE_1D == tex_target) || (GL_TEXTURE_1D_ARRAY == tex_target))
		{
			height = 1;
		}

		for (GLuint x = 0; x < n_x_vals; ++x)
		{
			for (GLuint y = 0; y < n_y_vals; ++y)
			{
				for (GLuint z = 0; z < n_z_vals; ++z)
				{
					const GLuint x_val = x_vals[x];
					const GLuint y_val = y_vals[y];
					const GLuint z_val = z_vals[z];
					const GLenum res = ((0 == x_val) && (0 == y_val) && (0 == z_val)) ? GL_NO_ERROR : GL_INVALID_VALUE;
					GLuint		 depth = 1;

					if (0 != z_val)
					{
						if ((GL_TEXTURE_2D_ARRAY != tex_target) || (GL_TEXTURE_2D_MULTISAMPLE_ARRAY != tex_target) ||
							(GL_TEXTURE_3D != tex_target) || (GL_TEXTURE_CUBE_MAP_ARRAY != tex_target))
						{
							/* Skip z != 0 for 2d textures */
							continue;
						}
						else
						{
							/* Set depth so as to exceed boundary */
							depth = m_region_depth;
						}
					}

					testCase src_test_case = { tex_target, depth, height, x_val, y_val, z_val, 0, 0, 0, res };

					testCase dst_test_case = { tex_target, depth, height, 0, 0, 0, x_val, y_val, z_val, res };

					m_test_cases.push_back(src_test_case);
					m_test_cases.push_back(dst_test_case);
				}
			}
		}
	}
}

/** Execute test
 *
 * @return CONTINUE as long there are more test case, STOP otherwise
 **/
tcu::TestNode::IterateResult ExceedingBoundariesTest::iterate()
{
	GLenum						 error	 = GL_NO_ERROR;
	const Functions&			 gl		   = m_context.getRenderContext().getFunctions();
	tcu::TestNode::IterateResult it_result = tcu::TestNode::STOP;
	bool						 result	= false;
	GLuint						 temp	  = 0;
	const testCase&				 test_case = m_test_cases[m_test_case_index];

	try
	{
		/* Prepare textures */
		m_dst_tex_name =
			Utils::prepareTex16x16x6(m_context, test_case.m_tex_target, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, temp);
		m_src_tex_name =
			Utils::prepareTex16x16x6(m_context, test_case.m_tex_target, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, temp);

		/* Make textures complete */
		Utils::makeTextureComplete(m_context, test_case.m_tex_target, m_dst_tex_name, 0 /* base */, 0 /* max */);
		Utils::makeTextureComplete(m_context, test_case.m_tex_target, m_src_tex_name, 0 /* base */, 0 /* max */);
	}
	catch (tcu::Exception& exc)
	{
		clean();
		throw exc;
	}

	/* Execute CopyImageSubData */
	gl.copyImageSubData(m_src_tex_name, test_case.m_tex_target, 0 /* level */, test_case.m_src_x /* srcX */,
						test_case.m_src_y /* srcY */, test_case.m_src_z /* srcZ */, m_dst_tex_name,
						test_case.m_tex_target, 0 /* level */, test_case.m_dst_x /* dstX */,
						test_case.m_dst_y /* dstY */, test_case.m_dst_z /* dstZ */, m_region_width /* srcWidth */,
						test_case.m_height /* srcHeight */, test_case.m_depth /* srcDepth */);

	/* Verify generated error */
	error  = gl.getError();
	result = (test_case.m_expected_result == error);

	/* Free resources */
	clean();

	/* Set result */
	if (true == result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

		/* Increase index */
		m_test_case_index += 1;

		/* Are there any test cases left */
		if (m_test_cases.size() > m_test_case_index)
		{
			it_result = tcu::TestNode::CONTINUE;
		}
	}
	else
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Failure. Expected result: " << glu::getErrorStr(test_case.m_expected_result)
			<< " got: " << glu::getErrorStr(error)
			<< ". Texture target: " << glu::getTextureTargetStr(test_case.m_tex_target) << ", source: ["
			<< test_case.m_src_x << ", " << test_case.m_src_y << ", " << test_case.m_src_z << "], destination: ["
			<< test_case.m_src_x << ", " << test_case.m_src_y << ", " << test_case.m_src_z
			<< "], depth: " << test_case.m_depth << tcu::TestLog::EndMessage;

		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return it_result;
}

/** Cleans resources
 *
 **/
void ExceedingBoundariesTest::clean()
{
	const testCase& test_case = m_test_cases[m_test_case_index];

	/* Clean textures and buffers. Errors ignored */
	Utils::deleteTexture(m_context, test_case.m_tex_target, m_dst_tex_name);
	Utils::deleteTexture(m_context, test_case.m_tex_target, m_src_tex_name);

	m_dst_tex_name = 0;
	m_src_tex_name = 0;
}

/* InvalidAlignmentTest */
/** Constructor
 *
 * @param context Text context
 **/
InvalidAlignmentTest::InvalidAlignmentTest(deqp::Context& context)
	: TestCase(context, "invalid_alignment", "Test verifies if INVALID_VALUE is generated when CopyImageSubData is "
											 "executed for regions with invalid alignment")
	, m_dst_tex_name(0)
	, m_src_tex_name(0)
	, m_test_case_index(0)
{
	/* 16x16x6 are values used by prepareTex16x16x6 */
	static const GLuint invalid_h = 2;
	static const GLuint invalid_w = 2;
	static const GLuint invalid_x = 2;
	static const GLuint invalid_y = 2;
	static const GLuint valid_h   = 4;
	static const GLuint valid_w   = 4;

	static const GLuint h_vals[] = { valid_h, invalid_h };
	static const GLuint w_vals[] = { valid_w, invalid_w };
	static const GLuint x_vals[] = { 0, invalid_x };
	static const GLuint y_vals[] = { 0, invalid_y };

	static const GLuint n_h_vals = sizeof(h_vals) / sizeof(h_vals[0]);
	static const GLuint n_w_vals = sizeof(w_vals) / sizeof(w_vals[0]);
	static const GLuint n_x_vals = sizeof(x_vals) / sizeof(x_vals[0]);
	static const GLuint n_y_vals = sizeof(y_vals) / sizeof(y_vals[0]);

	for (GLuint x = 0; x < n_x_vals; ++x)
	{
		for (GLuint y = 0; y < n_y_vals; ++y)
		{
			for (GLuint h = 0; h < n_h_vals; ++h)
			{
				for (GLuint w = 0; w < n_w_vals; ++w)
				{
					const GLuint h_val = h_vals[h];
					const GLuint w_val = w_vals[w];
					const GLuint x_val = x_vals[x];
					const GLuint y_val = y_vals[y];
					const GLenum res   = ((valid_h == h_val) && (valid_w == w_val) && (0 == x_val) && (0 == y_val)) ?
										   GL_NO_ERROR :
										   GL_INVALID_VALUE;

					testCase dst_test_case = { h_val, w_val, 0, 0, x_val, y_val, res };

					testCase src_test_case = { h_val, w_val, x_val, y_val, 0, 0, res };

					m_test_cases.push_back(dst_test_case);
					m_test_cases.push_back(src_test_case);
				}
			}
		}
	}
}

/** Execute test
 *
 * @return CONTINUE as long there are more test case, STOP otherwise
 **/
tcu::TestNode::IterateResult InvalidAlignmentTest::iterate()
{
	GLenum						 error	 = GL_NO_ERROR;
	const Functions&			 gl		   = m_context.getRenderContext().getFunctions();
	tcu::TestNode::IterateResult it_result = tcu::TestNode::STOP;
	bool						 result	= false;
	GLuint						 temp	  = 0;
	const testCase&				 test_case = m_test_cases[m_test_case_index];

	try
	{
		/* Prepare textures */
		m_dst_tex_name =
			Utils::prepareTex16x16x6(m_context, GL_TEXTURE_2D, GL_COMPRESSED_RG_RGTC2, GL_RG, GL_UNSIGNED_BYTE, temp);
		m_src_tex_name =
			Utils::prepareTex16x16x6(m_context, GL_TEXTURE_2D, GL_COMPRESSED_RG_RGTC2, GL_RG, GL_UNSIGNED_BYTE, temp);

		/* Make textures complete */
		Utils::makeTextureComplete(m_context, GL_TEXTURE_2D, m_dst_tex_name, 0 /* base */, 0 /* max */);
		Utils::makeTextureComplete(m_context, GL_TEXTURE_2D, m_src_tex_name, 0 /* base */, 0 /* max */);
	}
	catch (tcu::Exception& exc)
	{
		clean();
		throw exc;
	}

	/* Execute CopyImageSubData */
	gl.copyImageSubData(m_src_tex_name, GL_TEXTURE_2D, 0 /* level */, test_case.m_src_x /* srcX */,
						test_case.m_src_y /* srcY */, 0 /* srcZ */, m_dst_tex_name, GL_TEXTURE_2D, 0 /* level */,
						test_case.m_dst_x /* dstX */, test_case.m_dst_y /* dstY */, 0 /* dstZ */,
						test_case.m_width /* srcWidth */, test_case.m_height /* srcHeight */, 1 /* srcDepth */);

	/* Verify generated error */
	error  = gl.getError();
	result = (test_case.m_expected_result == error);

	/* Free resources */
	clean();

	/* Set result */
	if (true == result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

		/* Increase index */
		m_test_case_index += 1;

		/* Are there any test cases left */
		if (m_test_cases.size() > m_test_case_index)
		{
			it_result = tcu::TestNode::CONTINUE;
		}
	}
	else
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Failure. Expected result: " << glu::getErrorStr(test_case.m_expected_result)
			<< " got: " << glu::getErrorStr(error) << ". source: [" << test_case.m_src_x << ", " << test_case.m_src_y
			<< "], destination: [" << test_case.m_src_x << ", " << test_case.m_src_y << "], size: " << test_case.m_width
			<< " x " << test_case.m_height << tcu::TestLog::EndMessage;

		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return it_result;
}

/** Cleans resources
 *
 **/
void InvalidAlignmentTest::clean()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean textures and buffers. Errors ignored */
	gl.deleteTextures(1, &m_dst_tex_name);
	gl.deleteTextures(1, &m_src_tex_name);

	m_dst_tex_name = 0;
	m_src_tex_name = 0;
}

/** Constructor
 *
 * @param context Text context
 **/
IntegerTexTest::IntegerTexTest(deqp::Context& context)
	: TestCase(
		  context, "integer_tex",
		  "Test verifies if INVALID_OPERATION is generated when texture provided to CopySubImageData is incomplete")
	, m_dst_buf_name(0)
	, m_dst_tex_name(0)
	, m_src_buf_name(0)
	, m_src_tex_name(0)
	, m_test_case_index(0)
{
}

/** Execute test
 *
 * @return CONTINUE as long there are more test case, STOP otherwise
 **/
tcu::TestNode::IterateResult IntegerTexTest::iterate()
{
	testCase testCases[] = { { GL_R32I, GL_INT }, { GL_R32UI, GL_UNSIGNED_INT } };

	const unsigned int width  = 16;
	const unsigned int height = 16;

	const Functions&			 gl		   = m_context.getRenderContext().getFunctions();
	tcu::TestNode::IterateResult it_result = tcu::TestNode::STOP;
	const testCase&				 test_case = testCases[m_test_case_index];

	std::vector<int> data_buf(width * height, 1);
	m_dst_tex_name = createTexture(width, height, test_case.m_internal_format, test_case.m_type, &data_buf[0],
								   GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR);
	std::fill(data_buf.begin(), data_buf.end(), 2);
	m_src_tex_name = createTexture(width, height, test_case.m_internal_format, test_case.m_type, &data_buf[0],
								   GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR);

	/* Execute CopyImageSubData */
	gl.copyImageSubData(m_src_tex_name, GL_TEXTURE_2D, 0 /* srcLevel */, 0 /* srcX */, 0 /* srcY */, 0 /* srcZ */,
						m_dst_tex_name, GL_TEXTURE_2D, 0 /* dstLevel */, 0 /* dstX */, 0 /* dstY */, 0 /* dstZ */,
						1 /* srcWidth */, 1 /* srcHeight */, 1 /* srcDepth */);

	/* Check generated error */
	GLenum error = gl.getError();
	if (error == GL_NO_ERROR)
	{
		/* Verify result */
		std::fill(data_buf.begin(), data_buf.end(), 3);

		gl.bindTexture(GL_TEXTURE_2D, m_dst_tex_name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		gl.getTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &data_buf[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexImage");

		if ((data_buf[0] == 2) && (std::count(data_buf.begin(), data_buf.end(), 1) == (width * height - 1)))
		{
			m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

			/* Increase index */
			++m_test_case_index;

			/* Are there any test cases left */
			if (DE_LENGTH_OF_ARRAY(testCases) > m_test_case_index)
				it_result = tcu::TestNode::CONTINUE;
		}
		else
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Failure. Image data is not valid." << tcu::TestLog::EndMessage;
			m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}
	else
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Failure. Expected no error, got: " << glu::getErrorStr(error)
			<< ". Texture internal format: " << glu::getTextureFormatStr(test_case.m_internal_format)
			<< tcu::TestLog::EndMessage;
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Remove resources */
	clean();

	/* Done */
	return it_result;
}

/** Create texture
 *
 **/
unsigned int IntegerTexTest::createTexture(int width, int height, GLint internalFormat, GLuint type, const void* data,
										   int minFilter, int magFilter)
{
	const Functions& gl = m_context.getRenderContext().getFunctions();
	GLuint			 tex_name;

	gl.genTextures(1, &tex_name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");
	gl.bindTexture(GL_TEXTURE_2D, tex_name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
	gl.texImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RED_INTEGER, type, data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexImage2D");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");
	gl.bindTexture(GL_TEXTURE_2D, 0);

	return tex_name;
}

/** Cleans resources
 *
 **/
void IntegerTexTest::clean()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean textures and buffers. Errors ignored */
	gl.deleteTextures(1, &m_dst_tex_name);
	gl.deleteTextures(1, &m_src_tex_name);

	m_dst_tex_name = 0;
	m_src_tex_name = 0;

	if (0 != m_dst_buf_name)
	{
		gl.deleteBuffers(1, &m_dst_buf_name);
		m_dst_buf_name = 0;
	}

	if (0 != m_src_buf_name)
	{
		gl.deleteBuffers(1, &m_src_buf_name);
		m_src_buf_name = 0;
	}
}

} /* namespace CopyImage */

CopyImageTests::CopyImageTests(deqp::Context& context) : TestCaseGroup(context, "copy_image", "")
{
}

CopyImageTests::~CopyImageTests(void)
{
}

void CopyImageTests::init()
{
	addChild(new CopyImage::FunctionalTest(m_context));
	addChild(new CopyImage::IncompleteTexTest(m_context));
	addChild(new CopyImage::InvalidObjectTest(m_context));
	addChild(new CopyImage::SmokeTest(m_context));
	addChild(new CopyImage::InvalidTargetTest(m_context));
	addChild(new CopyImage::TargetMismatchTest(m_context));
	addChild(new CopyImage::IncompatibleFormatsTest(m_context));
	addChild(new CopyImage::SamplesMismatchTest(m_context));
	addChild(new CopyImage::IncompatibleFormatsCompressionTest(m_context));
	addChild(new CopyImage::NonExistentMipMapTest(m_context));
	addChild(new CopyImage::ExceedingBoundariesTest(m_context));
	addChild(new CopyImage::InvalidAlignmentTest(m_context));
	addChild(new CopyImage::IntegerTexTest(m_context));
}
} /* namespace gl4cts */
