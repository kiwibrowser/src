/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 *//*!
 * \file
 * \brief Negative Texture API tests.
 *//*--------------------------------------------------------------------*/

#include "es31fNegativeTextureApiTests.hpp"
#include "es31fNegativeTestShared.hpp"

#include "gluCallLogWrapper.hpp"
#include "gluContextInfo.hpp"
#include "gluRenderContext.hpp"

#include "glwDefs.hpp"
#include "glwEnums.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace NegativeTestShared
{

using tcu::TestLog;
using glu::CallLogWrapper;
using namespace glw;

static inline int divRoundUp (int a, int b)
{
	return a/b + (a%b != 0 ? 1 : 0);
}

static inline int etc2DataSize (int width, int height)
{
	return (int)(divRoundUp(width, 4) * divRoundUp(height, 4) * sizeof(deUint64));
}

static inline int etc2EacDataSize (int width, int height)
{
	return 2 * etc2DataSize(width, height);
}

static deUint32 cubeFaceToGLFace (tcu::CubeFace face)
{
	switch (face)
	{
		case tcu::CUBEFACE_NEGATIVE_X: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
		case tcu::CUBEFACE_POSITIVE_X: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		case tcu::CUBEFACE_NEGATIVE_Y: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
		case tcu::CUBEFACE_POSITIVE_Y: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
		case tcu::CUBEFACE_NEGATIVE_Z: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
		case tcu::CUBEFACE_POSITIVE_Z: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
		default:
			DE_ASSERT(DE_FALSE);
			return GL_NONE;
	}
}

#define FOR_CUBE_FACES(FACE_GL_VAR, BODY)												\
	do																					\
	{																					\
		for (int faceIterTcu = 0; faceIterTcu < tcu::CUBEFACE_LAST; faceIterTcu++)		\
		{																				\
			const GLenum FACE_GL_VAR = cubeFaceToGLFace((tcu::CubeFace)faceIterTcu);	\
			BODY																		\
		}																				\
	} while (false)


// glActiveTexture

void activetexture (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if texture is not one of GL_TEXTUREi, where i ranges from 0 to (GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS - 1).");
	ctx.glActiveTexture(-1);
	ctx.expectError(GL_INVALID_ENUM);
	int numMaxTextureUnits = ctx.getInteger(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
	ctx.glActiveTexture(GL_TEXTURE0 + numMaxTextureUnits);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

// glBindTexture

void bindtexture (NegativeTestContext& ctx)
{
	GLuint texture[5];
	ctx.glGenTextures(5, texture);

	ctx.beginSection("GL_INVALID_ENUM is generated if target is not one of the allowable values.");
	ctx.glBindTexture(0, 1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glBindTexture(GL_FRAMEBUFFER, 1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if texture was previously created with a target that doesn't match that of target.");
	ctx.glBindTexture(GL_TEXTURE_2D, texture[0]);
	ctx.expectError(GL_NO_ERROR);
	ctx.glBindTexture(GL_TEXTURE_CUBE_MAP, texture[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glBindTexture(GL_TEXTURE_3D, texture[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glBindTexture(GL_TEXTURE_2D_ARRAY, texture[0]);
	ctx.expectError(GL_INVALID_OPERATION);

	ctx.glBindTexture(GL_TEXTURE_CUBE_MAP, texture[1]);
	ctx.expectError(GL_NO_ERROR);
	ctx.glBindTexture(GL_TEXTURE_2D, texture[1]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glBindTexture(GL_TEXTURE_3D, texture[1]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glBindTexture(GL_TEXTURE_2D_ARRAY, texture[1]);
	ctx.expectError(GL_INVALID_OPERATION);

	ctx.glBindTexture(GL_TEXTURE_3D, texture[2]);
	ctx.expectError(GL_NO_ERROR);
	ctx.glBindTexture(GL_TEXTURE_2D, texture[2]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glBindTexture(GL_TEXTURE_CUBE_MAP, texture[2]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glBindTexture(GL_TEXTURE_2D_ARRAY, texture[2]);
	ctx.expectError(GL_INVALID_OPERATION);

	ctx.glBindTexture(GL_TEXTURE_2D_ARRAY, texture[3]);
	ctx.expectError(GL_NO_ERROR);
	ctx.glBindTexture(GL_TEXTURE_2D, texture[3]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glBindTexture(GL_TEXTURE_CUBE_MAP, texture[3]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glBindTexture(GL_TEXTURE_3D, texture[3]);
	ctx.expectError(GL_INVALID_OPERATION);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_cube_map_array"))
	{
		ctx.glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, texture[0]);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, texture[1]);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, texture[2]);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, texture[3]);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, texture[4]);
		ctx.expectError(GL_NO_ERROR);
		ctx.glBindTexture(GL_TEXTURE_2D, texture[4]);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.glBindTexture(GL_TEXTURE_CUBE_MAP, texture[4]);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.glBindTexture(GL_TEXTURE_3D, texture[4]);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.glBindTexture(GL_TEXTURE_2D_ARRAY, texture[4]);
		ctx.expectError(GL_INVALID_OPERATION);
	}
	ctx.endSection();

	ctx.glDeleteTextures(5, texture);
}

// glCompressedTexImage2D

void compressedteximage2d_invalid_target (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if target is invalid.");
	ctx.glCompressedTexImage2D(0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void compressedteximage2d_invalid_format (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if internalformat is not a supported format returned in GL_COMPRESSED_TEXTURE_FORMATS.");
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void compressedteximage2d_neg_level (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if level is less than 0.");
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, -1, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, -1, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, -1, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, -1, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, -1, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, -1, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, -1, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void compressedteximage2d_max_level (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_TEXTURE_SIZE) for a 2d texture target.");
	deUint32 log2MaxTextureSize = deLog2Floor32(ctx.getInteger(GL_MAX_TEXTURE_SIZE)) + 1;
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, log2MaxTextureSize, GL_COMPRESSED_RGB8_ETC2, 16, 16, 0, etc2DataSize(16, 16), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_CUBE_MAP_TEXTURE_SIZE) for a cubemap target.");
	deUint32 log2MaxCubemapSize = deLog2Floor32(ctx.getInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE)) + 1;
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, log2MaxCubemapSize, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 0, etc2EacDataSize(16, 16), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, log2MaxCubemapSize, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 0, etc2EacDataSize(16, 16), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, log2MaxCubemapSize, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 0, etc2EacDataSize(16, 16), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, log2MaxCubemapSize, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 0, etc2EacDataSize(16, 16), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, log2MaxCubemapSize, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 0, etc2EacDataSize(16, 16), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, log2MaxCubemapSize, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 0, etc2EacDataSize(16, 16), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void compressedteximage2d_neg_width_height (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if width or height is less than 0.");

	ctx.beginSection("GL_TEXTURE_2D target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, -1, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, -1, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_X target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, -1, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, -1, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Y target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, -1, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, -1, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Z target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, -1, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, -1, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_X target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, -1, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, -1, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Y target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, -1, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, -1, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Z target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, -1, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, -1, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.endSection();
}

void compressedteximage2d_max_width_height (NegativeTestContext& ctx)
{
	int maxTextureSize = ctx.getInteger(GL_MAX_TEXTURE_SIZE) + 1;
	int maxCubemapSize = ctx.getInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE) + 1;
	ctx.beginSection("GL_INVALID_VALUE is generated if width or height is greater than GL_MAX_TEXTURE_SIZE.");

	ctx.beginSection("GL_TEXTURE_2D target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, maxTextureSize, 1, 0, etc2EacDataSize(maxTextureSize, 1), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 1, maxTextureSize, 0, etc2EacDataSize(1, maxTextureSize), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, maxTextureSize, maxTextureSize, 0, etc2EacDataSize(maxTextureSize, maxTextureSize), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_X target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, maxCubemapSize, 1, 0, etc2EacDataSize(maxCubemapSize, 1), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 1, maxCubemapSize, 0, etc2EacDataSize(1, maxCubemapSize), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, maxCubemapSize, maxCubemapSize, 0, etc2EacDataSize(maxCubemapSize, maxCubemapSize), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Y target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, maxCubemapSize, 1, 0, etc2EacDataSize(maxCubemapSize, 1), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 1, maxCubemapSize, 0, etc2EacDataSize(1, maxCubemapSize), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, maxCubemapSize, maxCubemapSize, 0, etc2EacDataSize(maxCubemapSize, maxCubemapSize), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Z target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, maxCubemapSize, 1, 0, etc2EacDataSize(maxCubemapSize, 1), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 1, maxCubemapSize, 0, etc2EacDataSize(1, maxCubemapSize), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, maxCubemapSize, maxCubemapSize, 0, etc2EacDataSize(maxCubemapSize, maxCubemapSize), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_X target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, maxCubemapSize, 1, 0, etc2EacDataSize(maxCubemapSize, 1), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 1, maxCubemapSize, 0, etc2EacDataSize(1, maxCubemapSize), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, maxCubemapSize, maxCubemapSize, 0, etc2EacDataSize(maxCubemapSize, maxCubemapSize), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Y target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, maxCubemapSize, 1, 0, etc2EacDataSize(maxCubemapSize, 1), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 1, maxCubemapSize, 0, etc2EacDataSize(1, maxCubemapSize), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, maxCubemapSize, maxCubemapSize, 0, etc2EacDataSize(maxCubemapSize, maxCubemapSize), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Z target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, maxCubemapSize, 1, 0, etc2EacDataSize(maxCubemapSize, 1), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 1, maxCubemapSize, 0, etc2EacDataSize(1, maxCubemapSize), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, maxCubemapSize, maxCubemapSize, 0, etc2EacDataSize(maxCubemapSize, maxCubemapSize), 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.endSection();
}

void compressedteximage2d_invalid_border (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if border is not 0.");

	ctx.beginSection("GL_TEXTURE_2D target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 1, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, -1, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_X target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 1, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, -1, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Y target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 1, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, -1, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Z target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 1, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, -1, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_X target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 1, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, -1, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Y target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 1, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, -1, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Z target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 1, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, -1, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.endSection();
}

void compressedteximage2d_invalid_size (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if imageSize is not consistent with the format, dimensions, and contents of the specified compressed image data.");
	// Subtracting 1 to the imageSize field to deviate from the expected size. Removing the -1 would cause the imageSize to be correct.
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_R11_EAC, 1, 1, 0, divRoundUp(1, 4) * divRoundUp(1, 4) * 8 - 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SIGNED_R11_EAC, 1, 1, 0, divRoundUp(1, 4) * divRoundUp(1, 4) * 8 - 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RG11_EAC, 1, 1, 0, divRoundUp(1, 4) * divRoundUp(1, 4) * 16 - 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SIGNED_RG11_EAC, 1, 1, 0, divRoundUp(1, 4) * divRoundUp(1, 4) * 16 - 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB8_ETC2, 1, 1, 0, divRoundUp(1, 4) * divRoundUp(1, 4) * 8 - 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_ETC2, 1, 1, 0, divRoundUp(1, 4) * divRoundUp(1, 4) * 8 - 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, 1, 1, 0, divRoundUp(1, 4) * divRoundUp(1, 4) * 8 - 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, 1, 1, 0, divRoundUp(1, 4) * divRoundUp(1, 4) * 8 - 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 1, 1, 0, divRoundUp(1, 4) * divRoundUp(1, 4) * 16 - 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC, 1, 1, 0, divRoundUp(1, 4) * divRoundUp(1, 4) * 16 - 1, 0);
	ctx.expectError(GL_INVALID_VALUE);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)))
	{
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ASTC_4x4, 1, 1, 0, divRoundUp(1, 4) * divRoundUp(1, 4) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ASTC_5x4, 1, 1, 0, divRoundUp(1, 5) * divRoundUp(1, 4) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ASTC_5x5, 1, 1, 0, divRoundUp(1, 5) * divRoundUp(1, 5) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ASTC_6x5, 1, 1, 0, divRoundUp(1, 6) * divRoundUp(1, 5) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ASTC_6x6, 1, 1, 0, divRoundUp(1, 6) * divRoundUp(1, 6) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ASTC_8x5, 1, 1, 0, divRoundUp(1, 8) * divRoundUp(1, 5) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ASTC_8x6, 1, 1, 0, divRoundUp(1, 8) * divRoundUp(1, 6) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ASTC_8x8, 1, 1, 0, divRoundUp(1, 8) * divRoundUp(1, 8) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ASTC_10x5, 1, 1, 0, divRoundUp(1, 10) * divRoundUp(1, 5) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ASTC_10x6, 1, 1, 0, divRoundUp(1, 10) * divRoundUp(1, 6) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ASTC_10x8, 1, 1, 0, divRoundUp(1, 10) * divRoundUp(1, 8) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ASTC_10x10, 1, 1, 0, divRoundUp(1, 10) * divRoundUp(1, 10) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ASTC_12x10, 1, 1, 0, divRoundUp(1, 12) * divRoundUp(1, 10) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ASTC_12x12, 1, 1, 0, divRoundUp(1, 12) * divRoundUp(1, 12) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4, 1, 1, 0, divRoundUp(1, 4) * divRoundUp(1, 4) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4, 1, 1, 0, divRoundUp(1, 5) * divRoundUp(1, 4) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5, 1, 1, 0, divRoundUp(1, 5) * divRoundUp(1, 5) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5, 1, 1, 0, divRoundUp(1, 6) * divRoundUp(1, 5) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6, 1, 1, 0, divRoundUp(1, 6) * divRoundUp(1, 6) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5, 1, 1, 0, divRoundUp(1, 8) * divRoundUp(1, 5) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6, 1, 1, 0, divRoundUp(1, 8) * divRoundUp(1, 6) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8, 1, 1, 0, divRoundUp(1, 8) * divRoundUp(1, 8) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5, 1, 1, 0, divRoundUp(1, 10) * divRoundUp(1, 5) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6, 1, 1, 0, divRoundUp(1, 10) * divRoundUp(1, 6) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8, 1, 1, 0, divRoundUp(1, 10) * divRoundUp(1, 8) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10, 1, 1, 0, divRoundUp(1, 10) * divRoundUp(1, 10) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10, 1, 1, 0, divRoundUp(1, 12) * divRoundUp(1, 10) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	    ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12, 1, 1, 0, divRoundUp(1, 12) * divRoundUp(1, 12) * 16 - 1, 0);
	    ctx.expectError(GL_INVALID_VALUE);
	}
	ctx.endSection();
}

void compressedteximage2d_neg_size (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if imageSize is negative.");
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_R11_EAC, 0, 0, 0, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void compressedteximage2d_invalid_width_height (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if target is a cube map face and width and height are not equal.");

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_X target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_COMPRESSED_R11_EAC, 1, 2, 0, divRoundUp(1, 4) * divRoundUp(2, 4) * 8, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Y target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_COMPRESSED_R11_EAC, 1, 2, 0, divRoundUp(1, 4) * divRoundUp(2, 4) * 8, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Z target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_COMPRESSED_R11_EAC, 1, 2, 0, divRoundUp(1, 4) * divRoundUp(2, 4) * 8, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_X target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_COMPRESSED_R11_EAC, 1, 2, 0, divRoundUp(1, 4) * divRoundUp(2, 4) * 8, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Y target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_COMPRESSED_R11_EAC, 1, 2, 0, divRoundUp(1, 4) * divRoundUp(2, 4) * 8, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Z target");
	ctx.glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_COMPRESSED_R11_EAC, 1, 2, 0, divRoundUp(1, 4) * divRoundUp(2, 4) * 8, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.endSection();
}

void compressedteximage2d_invalid_buffer_target (NegativeTestContext& ctx)
{
	deUint32				buf = 1234;
	std::vector<GLubyte>	data(64);

	ctx.glGenBuffers			(1, &buf);
	ctx.glBindBuffer			(GL_PIXEL_UNPACK_BUFFER, buf);
	ctx.glBufferData			(GL_PIXEL_UNPACK_BUFFER, 64, &data[0], GL_DYNAMIC_COPY);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_OPERATION is generated if a non-zero buffer object name is bound to the GL_PIXEL_UNPACK_BUFFER target and the buffer object's data store is currently mapped.");
	ctx.glMapBufferRange		(GL_PIXEL_UNPACK_BUFFER, 0, 32, GL_MAP_WRITE_BIT);
	ctx.glCompressedTexImage2D	(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB8_ETC2, 4, 4, 0, etc2DataSize(4, 4), 0);
	ctx.expectError				(GL_INVALID_OPERATION);
	ctx.glUnmapBuffer			(GL_PIXEL_UNPACK_BUFFER);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if a non-zero buffer object name is bound to the GL_PIXEL_UNPACK_BUFFER target and the data would be unpacked from the buffer object such that the memory reads required would exceed the data store size.");
	ctx.glCompressedTexImage2D	(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB8_ETC2, 16, 16, 0, etc2DataSize(16, 16), 0);
	ctx.expectError				(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteBuffers			(1, &buf);
}

// glCopyTexImage2D

void copyteximage2d_invalid_target (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if target is invalid.");
	ctx.glCopyTexImage2D(0, 0, GL_RGB, 0, 0, 64, 64, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void copyteximage2d_invalid_format (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM or GL_INVALID_VALUE is generated if internalformat is not an accepted format.");
	ctx.glCopyTexImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 64, 64, 0);
	ctx.expectError(GL_INVALID_ENUM, GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, 0, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_ENUM, GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, 0, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_ENUM, GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, 0, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_ENUM, GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, 0, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_ENUM, GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, 0, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_ENUM, GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, 0, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_ENUM, GL_INVALID_VALUE);
	ctx.endSection();
}

void copyteximage2d_inequal_width_height_cube (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if target is one of the six cube map 2D image targets and the width and height parameters are not equal.");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 0, 0, 16, 17, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 0, 0, 16, 17, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 0, 0, 16, 17, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 0, 0, 16, 17, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 0, 0, 16, 17, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 0, 0, 16, 17, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void copyteximage2d_neg_level (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if level is less than 0.");
	ctx.glCopyTexImage2D(GL_TEXTURE_2D, -1, GL_RGB, 0, 0, 64, 64, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, -1, GL_RGB, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, -1, GL_RGB, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, -1, GL_RGB, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, -1, GL_RGB, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, -1, GL_RGB, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, -1, GL_RGB, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void copyteximage2d_max_level (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_TEXTURE_SIZE).");
	deUint32 log2MaxTextureSize = deLog2Floor32(ctx.getInteger(GL_MAX_TEXTURE_SIZE)) + 1;
	ctx.glCopyTexImage2D(GL_TEXTURE_2D, log2MaxTextureSize, GL_RGB, 0, 0, 64, 64, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_CUBE_MAP_TEXTURE_SIZE).");
	deUint32 log2MaxCubemapSize = deLog2Floor32(ctx.getInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE)) + 1;
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, log2MaxCubemapSize, GL_RGB, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, log2MaxCubemapSize, GL_RGB, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, log2MaxCubemapSize, GL_RGB, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, log2MaxCubemapSize, GL_RGB, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, log2MaxCubemapSize, GL_RGB, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, log2MaxCubemapSize, GL_RGB, 0, 0, 16, 16, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void copyteximage2d_neg_width_height (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if width or height is less than 0.");

	ctx.beginSection("GL_TEXTURE_2D target");
	ctx.glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, -1, 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, 1, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, -1, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_X target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 0, 0, -1, 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 0, 0, 1, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 0, 0, -1, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Y target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 0, 0, -1, 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 0, 0, 1, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 0, 0, -1, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Z target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 0, 0, -1, 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 0, 0, 1, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 0, 0, -1, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_X target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 0, 0, -1, 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 0, 0, 1, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 0, 0, -1, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Y target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 0, 0, -1, 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 0, 0, 1, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 0, 0, -1, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Z target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 0, 0, -1, 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 0, 0, 1, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 0, 0, -1, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.endSection();
}

void copyteximage2d_max_width_height (NegativeTestContext& ctx)
{
	int maxTextureSize = ctx.getInteger(GL_MAX_TEXTURE_SIZE) + 1;
	int maxCubemapSize = ctx.getInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE) + 1;

	ctx.beginSection("GL_INVALID_VALUE is generated if width or height is greater than GL_MAX_TEXTURE_SIZE.");

	ctx.beginSection("GL_TEXTURE_2D target");
	ctx.glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, maxTextureSize, 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, 1, maxTextureSize, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, maxTextureSize, maxTextureSize, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_X target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 0, 0, 1, maxCubemapSize, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 0, 0, maxCubemapSize, 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 0, 0, maxCubemapSize, maxCubemapSize, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Y target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 0, 0, 1, maxCubemapSize, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 0, 0, maxCubemapSize, 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 0, 0, maxCubemapSize, maxCubemapSize, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Z target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 0, 0, 1, maxCubemapSize, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 0, 0, maxCubemapSize, 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 0, 0, maxCubemapSize, maxCubemapSize, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_X target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 0, 0, 1, maxCubemapSize, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 0, 0, maxCubemapSize, 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 0, 0, maxCubemapSize, maxCubemapSize, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Y target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 0, 0, 1, maxCubemapSize, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 0, 0, maxCubemapSize, 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 0, 0, maxCubemapSize, maxCubemapSize, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Z target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 0, 0, 1, maxCubemapSize, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 0, 0, maxCubemapSize, 1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 0, 0, maxCubemapSize, maxCubemapSize, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.endSection();
}

void copyteximage2d_invalid_border (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if border is not 0.");

	ctx.beginSection("GL_TEXTURE_2D target");
	ctx.glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, 0, 0, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, 0, 0, 1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_X target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 0, 0, 0, 0, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 0, 0, 0, 0, 1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Y target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 0, 0, 0, 0, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 0, 0, 0, 0, 1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_2D target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 0, 0, 0, 0, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 0, 0, 0, 0, 1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_X target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 0, 0, 0, 0, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 0, 0, 0, 0, 1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Y target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 0, 0, 0, 0, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 0, 0, 0, 0, 1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Z target");
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 0, 0, 0, 0, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 0, 0, 0, 0, 1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.endSection();
}

void copyteximage2d_incomplete_framebuffer (NegativeTestContext& ctx)
{
	GLuint fbo = 0x1234;
	ctx.glGenFramebuffers		(1, &fbo);
	ctx.glBindFramebuffer		(GL_FRAMEBUFFER, fbo);
	ctx.glCheckFramebufferStatus(GL_FRAMEBUFFER);

	ctx.beginSection("GL_INVALID_FRAMEBUFFER_OPERATION is generated if the currently bound framebuffer is not framebuffer complete.");
	ctx.glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_FRAMEBUFFER_OPERATION);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA8, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_FRAMEBUFFER_OPERATION);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA8, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_FRAMEBUFFER_OPERATION);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA8, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_FRAMEBUFFER_OPERATION);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA8, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_FRAMEBUFFER_OPERATION);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA8, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_FRAMEBUFFER_OPERATION);
	ctx.glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA8, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_FRAMEBUFFER_OPERATION);
	ctx.endSection();

	ctx.glBindFramebuffer	(GL_FRAMEBUFFER, 0);
	ctx.glDeleteFramebuffers(1, &fbo);
}

void copytexsubimage2d_invalid_target (NegativeTestContext& ctx)
{
	GLuint texture = 0x1234;
	ctx.glGenTextures	(1, &texture);
	ctx.glBindTexture	(GL_TEXTURE_2D, texture);
	ctx.glTexImage2D	(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	ctx.beginSection("GL_INVALID_ENUM is generated if target is invalid.");
	ctx.glCopyTexSubImage2D(0, 0, 0, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}
void copytexsubimage2d_read_buffer_is_none (NegativeTestContext& ctx)
{
	GLuint texture = 0x1234;
	ctx.glGenTextures	(1, &texture);
	ctx.glBindTexture	(GL_TEXTURE_2D, texture);
	ctx.glTexImage2D	(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	ctx.beginSection("GL_INVALID_OPERATION is generated if the read buffer is NONE");
	ctx.glReadBuffer(GL_NONE);
	ctx.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

void copytexsubimage2d_texture_internalformat (NegativeTestContext& ctx)
{
	GLuint texture = 0x1234;
	ctx.glGenTextures	(1, &texture);
	ctx.glBindTexture	(GL_TEXTURE_2D, texture);
	ctx.glTexImage2D	(GL_TEXTURE_2D, 0, GL_RGB9_E5, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	ctx.beginSection("GL_INVALID_OPERATION is generated if internal format of the texture is GL_RGB9_E5");
	ctx.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

void copytexsubimage2d_neg_level (NegativeTestContext& ctx)
{
	GLuint textures[2];
	ctx.glGenTextures	(2, &textures[0]);
	ctx.glBindTexture	(GL_TEXTURE_2D, textures[0]);
	ctx.glTexImage2D	(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glBindTexture	(GL_TEXTURE_CUBE_MAP, textures[1]);
	FOR_CUBE_FACES(faceGL, ctx.glTexImage2D(faceGL, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0););

	ctx.beginSection("GL_INVALID_VALUE is generated if level is less than 0.");
	ctx.glCopyTexSubImage2D(GL_TEXTURE_2D, -1, 0, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	FOR_CUBE_FACES(faceGL,
	{
		ctx.glCopyTexSubImage2D(faceGL, -1, 0, 0, 0, 0, 4, 4);
		ctx.expectError(GL_INVALID_VALUE);
	});
	ctx.endSection();

	ctx.glDeleteTextures(2, &textures[0]);
}

void copytexsubimage2d_max_level (NegativeTestContext& ctx)
{
	GLuint textures[2];
	ctx.glGenTextures	(2, &textures[0]);
	ctx.glBindTexture	(GL_TEXTURE_2D, textures[0]);
	ctx.glTexImage2D	(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glBindTexture	(GL_TEXTURE_CUBE_MAP, textures[1]);
	FOR_CUBE_FACES(faceGL, ctx.glTexImage2D(faceGL, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0););

	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_TEXTURE_SIZE) for 2D texture targets.");
	deUint32 log2MaxTextureSize = deLog2Floor32(ctx.getInteger(GL_MAX_TEXTURE_SIZE)) + 1;
	ctx.glCopyTexSubImage2D(GL_TEXTURE_2D, log2MaxTextureSize, 0, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_CUBE_MAP_SIZE) for cubemap targets.");
	deUint32 log2MaxCubemapSize = deLog2Floor32(ctx.getInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE)) + 1;
	FOR_CUBE_FACES(faceGL,
	{
		ctx.glCopyTexSubImage2D(faceGL, log2MaxCubemapSize, 0, 0, 0, 0, 4, 4);
		ctx.expectError(GL_INVALID_VALUE);
	});
	ctx.endSection();

	ctx.glDeleteTextures(2, &textures[0]);
}

void copytexsubimage2d_neg_offset (NegativeTestContext& ctx)
{
	GLuint texture = 0x1234;
	ctx.glGenTextures	(1, &texture);
	ctx.glBindTexture	(GL_TEXTURE_2D, texture);
	ctx.glTexImage2D	(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	ctx.beginSection("GL_INVALID_VALUE is generated if xoffset < 0 or yoffset < 0.");
	ctx.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, -1, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, -1, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, -1, -1, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

void copytexsubimage2d_invalid_offset (NegativeTestContext& ctx)
{
	GLuint texture = 0x1234;
	ctx.glGenTextures	(1, &texture);
	ctx.glBindTexture	(GL_TEXTURE_2D, texture);
	ctx.glTexImage2D	(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	ctx.beginSection("GL_INVALID_VALUE is generated if xoffset + width > texture_width or yoffset + height > texture_height.");
	ctx.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 14, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 14, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 14, 14, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

void copytexsubimage2d_neg_width_height (NegativeTestContext& ctx)
{
	GLuint texture = 0x1234;
	ctx.glGenTextures	(1, &texture);
	ctx.glBindTexture	(GL_TEXTURE_2D, texture);
	ctx.glTexImage2D	(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	ctx.beginSection("GL_INVALID_VALUE is generated if width or height is less than 0.");
	ctx.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 0, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, -1, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

void copytexsubimage2d_incomplete_framebuffer (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_FRAMEBUFFER_OPERATION is generated if the currently bound framebuffer is not framebuffer complete.");

	GLuint texture[2];
	GLuint fbo = 0x1234;

	ctx.glGenTextures			(2, texture);
	ctx.glBindTexture			(GL_TEXTURE_2D, texture[0]);
	ctx.glTexImage2D			(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glBindTexture			(GL_TEXTURE_CUBE_MAP, texture[1]);
	ctx.glTexImage2D			(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glTexImage2D			(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glTexImage2D			(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glTexImage2D			(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glTexImage2D			(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glTexImage2D			(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_NO_ERROR);

	ctx.glGenFramebuffers(1, &fbo);
	ctx.glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	ctx.glCheckFramebufferStatus(GL_FRAMEBUFFER);
	ctx.expectError(GL_NO_ERROR);

	ctx.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_FRAMEBUFFER_OPERATION);
	ctx.glCopyTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_FRAMEBUFFER_OPERATION);
	ctx.glCopyTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_FRAMEBUFFER_OPERATION);
	ctx.glCopyTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_FRAMEBUFFER_OPERATION);
	ctx.glCopyTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_FRAMEBUFFER_OPERATION);
	ctx.glCopyTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_FRAMEBUFFER_OPERATION);
	ctx.glCopyTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_FRAMEBUFFER_OPERATION);

	ctx.glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ctx.glDeleteFramebuffers(1, &fbo);
	ctx.glDeleteTextures(2, texture);

	ctx.endSection();
}

// glDeleteTextures

void deletetextures (NegativeTestContext& ctx)
{
	GLuint texture = 0x1234;
	ctx.glGenTextures(1, &texture);

	ctx.beginSection("GL_INVALID_VALUE is generated if n is negative.");
	ctx.glDeleteTextures(-1, 0);
	ctx.expectError(GL_INVALID_VALUE);

	ctx.glBindTexture(GL_TEXTURE_2D, texture);
	ctx.glDeleteTextures(-1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

// glGenerateMipmap

void generatemipmap (NegativeTestContext& ctx)
{
	GLuint texture[2];
	ctx.glGenTextures(2, texture);

	ctx.beginSection("GL_INVALID_ENUM is generated if target is not GL_TEXTURE_2D or GL_TEXTURE_CUBE_MAP.");
	ctx.glGenerateMipmap(0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("INVALID_OPERATION is generated if the texture bound to target is not cube complete.");
	ctx.glBindTexture(GL_TEXTURE_CUBE_MAP, texture[0]);
	ctx.glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 0, 0, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	ctx.expectError(GL_INVALID_OPERATION);

	ctx.glBindTexture(GL_TEXTURE_CUBE_MAP, texture[0]);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 16, 16, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 16, 16, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 16, 16, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 16, 16, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 16, 16, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if the zero level array is stored in a compressed internal format.");
	ctx.glBindTexture(GL_TEXTURE_2D, texture[1]);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, 0, 0);
	ctx.glGenerateMipmap(GL_TEXTURE_2D);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if the level base array was not specified with an unsized internal format or a sized internal format that is both color-renderable and texture-filterable.");
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8_SNORM, 0, 0, 0, GL_RGB, GL_BYTE, 0);
	ctx.glGenerateMipmap(GL_TEXTURE_2D);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_R8I, 0, 0, 0, GL_RED_INTEGER, GL_BYTE, 0);
	ctx.glGenerateMipmap(GL_TEXTURE_2D);
	ctx.expectError(GL_INVALID_OPERATION);

	if (!(ctx.getContextInfo().isExtensionSupported("GL_EXT_color_buffer_float") && ctx.getContextInfo().isExtensionSupported("GL_OES_texture_float_linear")))
	{
		ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 0, 0, 0, GL_RGBA, GL_FLOAT, 0);
		ctx.glGenerateMipmap(GL_TEXTURE_2D);
		ctx.expectError(GL_INVALID_OPERATION);
	}

	ctx.endSection();

	ctx.glDeleteTextures(2, texture);
}

// glGenTextures

void gentextures (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if n is negative.");
	ctx.glGenTextures(-1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

// glPixelStorei

void pixelstorei (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not an accepted value.");
	ctx.glPixelStorei(0,1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if a negative row length, pixel skip, or row skip value is specified, or if alignment is specified as other than 1, 2, 4, or 8.");
	ctx.glPixelStorei(GL_PACK_ROW_LENGTH, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glPixelStorei(GL_PACK_SKIP_ROWS, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glPixelStorei(GL_PACK_SKIP_PIXELS, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glPixelStorei(GL_UNPACK_ROW_LENGTH, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glPixelStorei(GL_UNPACK_SKIP_ROWS, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glPixelStorei(GL_UNPACK_SKIP_PIXELS, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glPixelStorei(GL_UNPACK_SKIP_IMAGES, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glPixelStorei(GL_PACK_ALIGNMENT, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glPixelStorei(GL_UNPACK_ALIGNMENT, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glPixelStorei(GL_PACK_ALIGNMENT, 16);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glPixelStorei(GL_UNPACK_ALIGNMENT, 16);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

// glTexImage2D

void teximage2d (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if target is invalid.");
	ctx.glTexImage2D(0, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if type is not a type constant.");
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if the combination of internalFormat, format and type is invalid.");
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGB, GL_UNSIGNED_SHORT_4_4_4_4, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, 1, 1, 0, GL_RGB, GL_UNSIGNED_SHORT_5_5_5_1, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB10_A2, 1, 1, 0, GL_RGB, GL_UNSIGNED_INT_2_10_10_10_REV, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, 1, 1, 0, GL_RGBA_INTEGER, GL_INT, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();
}

void teximage2d_inequal_width_height_cube (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if target is one of the six cube map 2D image targets and the width and height parameters are not equal.");
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 1, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 1, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 1, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 1, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 1, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 1, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void teximage2d_neg_level (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if level is less than 0.");
	ctx.glTexImage2D(GL_TEXTURE_2D, -1, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if level is less than 0.");
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, -1, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, -1, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, -1, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, -1, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, -1, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, -1, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void teximage2d_max_level (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_TEXTURE_SIZE).");
	deUint32 log2MaxTextureSize = deLog2Floor32(ctx.getInteger(GL_MAX_TEXTURE_SIZE)) + 1;
	ctx.glTexImage2D(GL_TEXTURE_2D, log2MaxTextureSize, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_CUBE_MAP_TEXTURE_SIZE).");
	deUint32 log2MaxCubemapSize = deLog2Floor32(ctx.getInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE)) + 1;
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, log2MaxCubemapSize, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, log2MaxCubemapSize, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, log2MaxCubemapSize, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, log2MaxCubemapSize, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, log2MaxCubemapSize, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, log2MaxCubemapSize, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void teximage2d_neg_width_height (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if width or height is less than 0.");

	ctx.beginSection("GL_TEXTURE_2D target");
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, -1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, -1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, -1, -1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_X target");
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, -1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 1, -1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, -1, -1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Y target");
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, -1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 1, -1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, -1, -1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Z target");
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, -1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 1, -1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, -1, -1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_X target");
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, -1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 1, -1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, -1, -1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Y target");
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, -1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 1, -1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, -1, -1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Z target");
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, -1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 1, -1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, -1, -1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.endSection();
}

void teximage2d_max_width_height (NegativeTestContext& ctx)
{
	int maxTextureSize = ctx.getInteger(GL_MAX_TEXTURE_SIZE) + 1;
	int maxCubemapSize = ctx.getInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE) + 1;

	ctx.beginSection("GL_INVALID_VALUE is generated if width or height is greater than GL_MAX_TEXTURE_SIZE.");
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, maxTextureSize, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, maxTextureSize, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, maxTextureSize, maxTextureSize, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if width or height is greater than GL_MAX_CUBE_MAP_TEXTURE_SIZE.");

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_X target");
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, maxCubemapSize, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 1, maxCubemapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, maxCubemapSize, maxCubemapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Y target");
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, maxCubemapSize, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 1, maxCubemapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, maxCubemapSize, maxCubemapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_POSITIVE_Z target");
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, maxCubemapSize, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 1, maxCubemapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, maxCubemapSize, maxCubemapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_X target");
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, maxCubemapSize, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 1, maxCubemapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, maxCubemapSize, maxCubemapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Y target");
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, maxCubemapSize, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 1, maxCubemapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, maxCubemapSize, maxCubemapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_TEXTURE_CUBE_MAP_NEGATIVE_Z target");
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, maxCubemapSize, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 1, maxCubemapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, maxCubemapSize, maxCubemapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.endSection();
}

void teximage2d_invalid_border (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if border is not 0.");
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, -1, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void teximage2d_invalid_buffer_target (NegativeTestContext& ctx)
{
	deUint32				buf = 0x1234;
	deUint32				texture = 0x1234;
	std::vector<GLubyte>	data(64);

	ctx.glGenBuffers			(1, &buf);
	ctx.glBindBuffer			(GL_PIXEL_UNPACK_BUFFER, buf);
	ctx.glBufferData			(GL_PIXEL_UNPACK_BUFFER, 64, &data[0], GL_DYNAMIC_COPY);
	ctx.glGenTextures			(1, &texture);
	ctx.glBindTexture			(GL_TEXTURE_2D, texture);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_OPERATION is generated if a non-zero buffer object name is bound to the GL_PIXEL_UNPACK_BUFFER target and...");
	ctx.beginSection("...the buffer object's data store is currently mapped.");
	ctx.glMapBufferRange		(GL_PIXEL_UNPACK_BUFFER, 0, 32, GL_MAP_WRITE_BIT);
	ctx.glTexImage2D			(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError				(GL_INVALID_OPERATION);
	ctx.glUnmapBuffer			(GL_PIXEL_UNPACK_BUFFER);
	ctx.endSection();

	ctx.beginSection("...the data would be unpacked from the buffer object such that the memory reads required would exceed the data store size.");
	ctx.glTexImage2D			(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError				(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("...data is not evenly divisible into the number of bytes needed to store in memory a datum indicated by type.");
	ctx.getLog() << TestLog::Message << "// Set byte offset = 3" << TestLog::EndMessage;
	ctx.glTexImage2D			(GL_TEXTURE_2D, 0, GL_RGB5_A1, 4, 4, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, (const GLvoid*)3);
	ctx.expectError				(GL_INVALID_OPERATION);
	ctx.endSection();
	ctx.endSection();

	ctx.glDeleteBuffers			(1, &buf);
	ctx.glDeleteTextures		(1, &texture);
}

// glTexSubImage2D

void texsubimage2d (NegativeTestContext& ctx)
{
	deUint32			texture = 0x1234;
	ctx.glGenTextures		(1, &texture);
	ctx.glBindTexture		(GL_TEXTURE_2D, texture);
	ctx.glTexImage2D		(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError			(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_ENUM is generated if target is invalid.");
	ctx.glTexSubImage2D(0, 0, 0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if format is not an accepted format constant.");
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 4, 4, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if type is not a type constant.");
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RGB, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if the combination of internalFormat of the previously specified texture array, format and type is not valid.");
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_SHORT_5_6_5, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RGB, GL_UNSIGNED_SHORT_4_4_4_4, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RGB, GL_UNSIGNED_SHORT_5_5_5_1, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RGB, GL_UNSIGNED_SHORT_5_5_5_1, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RGBA_INTEGER, GL_UNSIGNED_INT, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RGB, GL_FLOAT, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteTextures	(1, &texture);
}

void texsubimage2d_neg_level (NegativeTestContext& ctx)
{
	deUint32			textures[2];
	ctx.glGenTextures		(2, &textures[0]);
	ctx.glBindTexture		(GL_TEXTURE_2D, textures[0]);
	ctx.glTexImage2D		(GL_TEXTURE_2D, 0, GL_RGB, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.glBindTexture		(GL_TEXTURE_2D, textures[1]);
	FOR_CUBE_FACES(faceGL, ctx.glTexImage2D(faceGL, 0, GL_RGB, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, 0););
	ctx.expectError			(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if level is less than 0.");
	ctx.glTexSubImage2D(GL_TEXTURE_2D, -1, 0, 0, 0, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if level is less than 0.");
	FOR_CUBE_FACES(faceGL,
	{
		ctx.glTexSubImage2D(faceGL, -1, 0, 0, 0, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
	});
	ctx.endSection();

	ctx.glDeleteTextures(2, &textures[0]);
}

void texsubimage2d_max_level (NegativeTestContext& ctx)
{
	deUint32			textures[2];
	ctx.glGenTextures		(2, &textures[0]);
	ctx.glBindTexture		(GL_TEXTURE_2D, textures[0]);
	ctx.glTexImage2D		(GL_TEXTURE_2D, 0, GL_RGB, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.glBindTexture		(GL_TEXTURE_CUBE_MAP, textures[1]);
	FOR_CUBE_FACES(faceGL, ctx.glTexImage2D(faceGL, 0, GL_RGB, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, 0););
	ctx.expectError			(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_TEXTURE_SIZE).");
	deUint32 log2MaxTextureSize = deLog2Floor32(ctx.getInteger(GL_MAX_TEXTURE_SIZE)) + 1;
	ctx.glTexSubImage2D(GL_TEXTURE_2D, log2MaxTextureSize, 0, 0, 0, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_CUBE_MAP_TEXTURE_SIZE).");
	deUint32 log2MaxCubemapSize = deLog2Floor32(ctx.getInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE)) + 1;
	FOR_CUBE_FACES(faceGL,
	{
		ctx.glTexSubImage2D(faceGL, log2MaxCubemapSize, 0, 0, 0, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
	});
	ctx.endSection();

	ctx.glDeleteTextures(2, &textures[0]);
}

void texsubimage2d_neg_offset (NegativeTestContext& ctx)
{
	deUint32 texture = 0x1234;
	ctx.glGenTextures(1, &texture);
	ctx.glBindTexture(GL_TEXTURE_2D, texture);
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if xoffset or yoffset are negative.");
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, -1, 0, 0, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, -1, 0, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, -1, -1, 0, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

void texsubimage2d_invalid_offset (NegativeTestContext& ctx)
{
	deUint32			texture = 0x1234;
	ctx.glGenTextures		(1, &texture);
	ctx.glBindTexture		(GL_TEXTURE_2D, texture);
	ctx.glTexImage2D		(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError			(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if xoffset + width > texture_width or yoffset + height > texture_height.");
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 30, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 30, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 30, 30, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteTextures	(1, &texture);
}

void texsubimage2d_neg_width_height (NegativeTestContext& ctx)
{
	deUint32			texture = 0x1234;
	ctx.glGenTextures		(1, &texture);
	ctx.glBindTexture		(GL_TEXTURE_2D, texture);
	ctx.glTexImage2D		(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError			(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if width or height is less than 0.");
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, -1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, -1, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, -1, -1, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteTextures	(1, &texture);
}

void texsubimage2d_invalid_buffer_target (NegativeTestContext& ctx)
{
	deUint32				buf = 0x1234;
	deUint32				texture = 0x1234;
	std::vector<GLubyte>	data(64);

	ctx.glGenTextures			(1, &texture);
	ctx.glBindTexture			(GL_TEXTURE_2D, texture);
	ctx.glTexImage2D			(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glGenBuffers			(1, &buf);
	ctx.glBindBuffer			(GL_PIXEL_UNPACK_BUFFER, buf);
	ctx.glBufferData			(GL_PIXEL_UNPACK_BUFFER, 64, &data[0], GL_DYNAMIC_COPY);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_OPERATION is generated if a non-zero buffer object name is bound to the GL_PIXEL_UNPACK_BUFFER target and...");
	ctx.beginSection("...the buffer object's data store is currently mapped.");
	ctx.glMapBufferRange		(GL_PIXEL_UNPACK_BUFFER, 0, 32, GL_MAP_WRITE_BIT);
	ctx.glTexSubImage2D			(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError				(GL_INVALID_OPERATION);
	ctx.glUnmapBuffer			(GL_PIXEL_UNPACK_BUFFER);
	ctx.endSection();

	ctx.beginSection("...the data would be unpacked from the buffer object such that the memory reads required would exceed the data store size.");
	ctx.glTexSubImage2D			(GL_TEXTURE_2D, 0, 0, 0, 32, 32, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError				(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("...data is not evenly divisible into the number of bytes needed to store in memory a datum indicated by type.");
	ctx.getLog() << TestLog::Message << "// Set byte offset = 3" << TestLog::EndMessage;
	ctx.glBindBuffer			(GL_PIXEL_UNPACK_BUFFER, 0);
	ctx.glTexImage2D			(GL_TEXTURE_2D, 0, GL_RGBA4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, 0);
	ctx.glBindBuffer			(GL_PIXEL_UNPACK_BUFFER, buf);
	ctx.expectError				(GL_NO_ERROR);
	ctx.glTexSubImage2D			(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, (const GLvoid*)3);
	ctx.expectError				(GL_INVALID_OPERATION);
	ctx.endSection();
	ctx.endSection();

	ctx.glDeleteBuffers			(1, &buf);
	ctx.glDeleteTextures		(1, &texture);
}

// glTexParameteri

void texparameteri (NegativeTestContext& ctx)
{
	GLuint texture = 0x1234;
	GLint textureMode = -1;

	ctx.glGenTextures(1, &texture);
	ctx.glBindTexture(GL_TEXTURE_2D, texture);

	ctx.beginSection("GL_INVALID_ENUM is generated if target or pname is not one of the accepted defined values.");
	ctx.glTexParameteri(0, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D, 0, GL_LINEAR);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(0, 0, GL_LINEAR);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is enum and the value of param(s) is not a valid value.");
	ctx.glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if pname is GL_TEXTURE_BASE_LEVEL or GL_TEXTURE_MAX_LEVEL and param(s) is negative.");
	ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)))
	{
		ctx.beginSection("GL_INVALID_ENUM is generated if pname is a non-scalar parameter.");
		ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, 0);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.endSection();
	}

	ctx.beginSection("GL_INVALID_ENUM error is generated if target is GL_TEXTURE_2D_MULTISAMPLE or GL_TEXTURE_2D_MULTISAMPLE_ARRAY and pname is not valid.");
	ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BORDER_COLOR, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_R, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_LOD, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LOD, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_MODE, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_FUNC, textureMode);
	ctx.expectError(GL_INVALID_ENUM);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array"))
	{
		ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BORDER_COLOR, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MIN_FILTER, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MAG_FILTER, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_S, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_T, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_R, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MIN_LOD, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MAX_LOD, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_COMPARE_MODE, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_COMPARE_FUNC, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
	}
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if target is GL_TEXTURE_2D_MULTISAMPLE or GL_TEXTURE_2D_MULTISAMPLE_ARRAY and pname GL_TEXTURE_BASE_LEVEL is not 0.");
	ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BASE_LEVEL, 1);
	ctx.expectError(GL_INVALID_OPERATION);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array"))
	{
		ctx.glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BASE_LEVEL, 1);
		ctx.expectError(GL_INVALID_OPERATION);
	}
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

// glTexParameterf

void texparameterf (NegativeTestContext& ctx)
{
	GLuint texture = 0x1234;
	GLfloat textureMode = -1.0f;
	ctx.glGenTextures(1, &texture);
	ctx.glBindTexture(GL_TEXTURE_2D, texture);

	ctx.beginSection("GL_INVALID_ENUM is generated if target or pname is not one of the accepted defined values.");
	ctx.glTexParameterf(0, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D, 0, GL_LINEAR);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(0, 0, GL_LINEAR);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is enum and the value of param(s) is not a valid value.");
	ctx.glTexParameterf(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if pname is GL_TEXTURE_BASE_LEVEL or GL_TEXTURE_MAX_LEVEL and param(s) is negative.");
	ctx.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, -1.0f);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, -1.0f);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)))
	{
		ctx.beginSection("GL_INVALID_ENUM is generated if pname is a non-scalar parameter.");
		ctx.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, 0.0f);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.endSection();
	}

	ctx.beginSection("GL_INVALID_ENUM error is generated if target is GL_TEXTURE_2D_MULTISAMPLE or GL_TEXTURE_2D_MULTISAMPLE_ARRAY and pname is not valid.");
	ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BORDER_COLOR, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_R, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_LOD, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LOD, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_MODE, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_FUNC, textureMode);
	ctx.expectError(GL_INVALID_ENUM);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array"))
	{
		ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BORDER_COLOR, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MIN_FILTER, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MAG_FILTER, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_S, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_T, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_R, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MIN_LOD, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MAX_LOD, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_COMPARE_MODE, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_COMPARE_FUNC, textureMode);
		ctx.expectError(GL_INVALID_ENUM);
	}
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if target is GL_TEXTURE_2D_MULTISAMPLE or GL_TEXTURE_2D_MULTISAMPLE_ARRAY and pname GL_TEXTURE_BASE_LEVEL is not 0.");
	ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BASE_LEVEL, 1.0f);
	ctx.expectError(GL_INVALID_OPERATION);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array"))
	{
		ctx.glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BASE_LEVEL, 1.0f);
		ctx.expectError(GL_INVALID_OPERATION);
	}
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

// glTexParameteriv

void texparameteriv (NegativeTestContext& ctx)
{
	GLint params[1] = {GL_LINEAR};

	GLuint texture = 0x1234;
	ctx.glGenTextures(1, &texture);
	ctx.glBindTexture(GL_TEXTURE_2D, texture);

	ctx.beginSection("GL_INVALID_ENUM is generated if target or pname is not one of the accepted defined values.");
	ctx.glTexParameteriv(0, GL_TEXTURE_MIN_FILTER, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D, 0, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(0, 0, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is enum and the value of param(s) is not a valid value.");
	params[0] = -1;
	ctx.glTexParameteriv(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if pname is GL_TEXTURE_BASE_LEVEL or GL_TEXTURE_MAX_LEVEL and param(s) is negative.");
	ctx.glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, &params[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &params[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM error is generated if target is GL_TEXTURE_2D_MULTISAMPLE or GL_TEXTURE_2D_MULTISAMPLE_ARRAY and pname is not valid.");
	ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BORDER_COLOR, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_R, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_LOD, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LOD, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_MODE, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_FUNC, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array"))
	{
		ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BORDER_COLOR, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MIN_FILTER, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MAG_FILTER, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_S, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_T, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_R, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MIN_LOD, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MAX_LOD, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_COMPARE_MODE, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_COMPARE_FUNC, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
	}
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if target is GL_TEXTURE_2D_MULTISAMPLE or GL_TEXTURE_2D_MULTISAMPLE_ARRAY and pname GL_TEXTURE_BASE_LEVEL is not 0.");
	params[0] = 1;
	ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BASE_LEVEL, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array"))
	{
		params[0] = 1;
		ctx.glTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BASE_LEVEL, &params[0]);
		ctx.expectError(GL_INVALID_OPERATION);
	}
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

// glTexParameterfv

void texparameterfv (NegativeTestContext& ctx)
{
	GLfloat params[1] = {GL_LINEAR};
	GLuint texture = 0x1234;
	ctx.glGenTextures(1, &texture);
	ctx.glBindTexture(GL_TEXTURE_2D, texture);

	ctx.beginSection("GL_INVALID_ENUM is generated if target or pname is not one of the accepted defined values.");
	params[0] = GL_LINEAR;
	ctx.glTexParameterfv(0, GL_TEXTURE_MIN_FILTER, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D, 0, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(0, 0, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is enum and the value of param(s) is not a valid value.");
	params[0] = -1.0f;
	ctx.glTexParameterfv(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if pname is GL_TEXTURE_BASE_LEVEL or GL_TEXTURE_MAX_LEVEL and param(s) is negative.");
	ctx.glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, &params[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &params[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM error is generated if target is GL_TEXTURE_2D_MULTISAMPLE or GL_TEXTURE_2D_MULTISAMPLE_ARRAY and pname is not valid.");
	ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BORDER_COLOR, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_R, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_LOD, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LOD, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_MODE, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_FUNC, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array"))
	{
		ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BORDER_COLOR, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MIN_FILTER, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MAG_FILTER, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_S, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_T, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_R, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MIN_LOD, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MAX_LOD, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_COMPARE_MODE, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
		ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_COMPARE_FUNC, &params[0]);
		ctx.expectError(GL_INVALID_ENUM);
	}
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if target is GL_TEXTURE_2D_MULTISAMPLE or GL_TEXTURE_2D_MULTISAMPLE_ARRAY and pname GL_TEXTURE_BASE_LEVEL is not 0.");
	params[0] = 1.0f;
	ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BASE_LEVEL, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array"))
	{
		params[0] = 1.0f;
		ctx.glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BASE_LEVEL, &params[0]);
		ctx.expectError(GL_INVALID_OPERATION);
	}
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

// glTexParameterIiv

void texparameterIiv (NegativeTestContext& ctx)
{
	if (!contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		throw tcu::NotSupportedError("glTexParameterIiv is not supported.", DE_NULL, __FILE__, __LINE__);

	GLint textureMode[] = { GL_DEPTH_COMPONENT, GL_STENCIL_INDEX };
	ctx.beginSection("GL_INVALID_ENUM is generated if target is not a valid target.");
	ctx.glTexParameterIiv(0, GL_DEPTH_STENCIL_TEXTURE_MODE, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not a valid parameter.");
	ctx.glTexParameterIiv(GL_TEXTURE_2D, 0, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is enum and the value of param(s) is not a valid value.");
	textureMode[0] = -1;
	textureMode[1] = -1;
	ctx.glTexParameterIiv(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if pname is GL_TEXTURE_BASE_LEVEL or GL_TEXTURE_MAX_LEVEL and param(s) is negative.");
	ctx.glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, textureMode);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, textureMode);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM error is generated if target is GL_TEXTURE_2D_MULTISAMPLE or GL_TEXTURE_2D_MULTISAMPLE_ARRAY and pname is not valid.");
	textureMode[0] = 0;
	textureMode[1] = 0;
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BORDER_COLOR, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_R, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_LOD, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LOD, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_MODE, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_FUNC, textureMode);
	ctx.expectError(GL_INVALID_ENUM);

	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BORDER_COLOR, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MIN_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MAG_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_S, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_T, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_R, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MIN_LOD, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MAX_LOD, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_COMPARE_MODE, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_COMPARE_FUNC, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if target is GL_TEXTURE_2D_MULTISAMPLE or GL_TEXTURE_2D_MULTISAMPLE_ARRAY and pname GL_TEXTURE_BASE_LEVEL is not 0.");
	textureMode[0] = 1;
	textureMode[1] = 1;
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BASE_LEVEL, textureMode);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BASE_LEVEL, textureMode);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();
}

// glTexParameterIuiv

void texparameterIuiv (NegativeTestContext& ctx)
{
	if (!contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		throw tcu::NotSupportedError("glTexParameterIuiv is not supported.", DE_NULL, __FILE__, __LINE__);

	GLuint textureMode[] = { GL_DEPTH_COMPONENT, GL_STENCIL_INDEX };
	ctx.beginSection("GL_INVALID_ENUM is generated if target is not a valid target.");
	ctx.glTexParameterIuiv(0, GL_DEPTH_STENCIL_TEXTURE_MODE, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not a valid parameter.");
	ctx.glTexParameterIuiv(GL_TEXTURE_2D, 0, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is enum and the value of param(s) is not a valid value.");
	textureMode[0] = GL_DONT_CARE;
	ctx.glTexParameterIuiv(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM error is generated if target is GL_TEXTURE_2D_MULTISAMPLE or GL_TEXTURE_2D_MULTISAMPLE_ARRAY and pname is not valid.");
	textureMode[0] = 0;
	textureMode[1] = 0;
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BORDER_COLOR, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_R, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_LOD, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LOD, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_MODE, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_FUNC, textureMode);
	ctx.expectError(GL_INVALID_ENUM);

	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BORDER_COLOR, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MIN_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MAG_FILTER, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_S, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_T, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_WRAP_R, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MIN_LOD, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_MAX_LOD, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_COMPARE_MODE, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_COMPARE_FUNC, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if target is GL_TEXTURE_2D_MULTISAMPLE or GL_TEXTURE_2D_MULTISAMPLE_ARRAY and pname GL_TEXTURE_BASE_LEVEL is not 0.");
	textureMode[0] = 1;
	textureMode[1] = 1;
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BASE_LEVEL, textureMode);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BASE_LEVEL, textureMode);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();
}

// glCompressedTexSubImage2D

void compressedtexsubimage2d (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if target is invalid.");
	ctx.glCompressedTexSubImage2D(0, 0, 0, 0, 0, 0, GL_COMPRESSED_RGB8_ETC2, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	deUint32				texture = 0x1234;
	ctx.glGenTextures			(1, &texture);
	ctx.glBindTexture			(GL_TEXTURE_2D, texture);
	ctx.glCompressedTexImage2D	(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 18, 18, 0, etc2EacDataSize(18, 18), 0);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_OPERATION is generated if format does not match the internal format of the texture image being modified.");
	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, GL_COMPRESSED_RGB8_ETC2, 0, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("For ETC2/EAC images GL_INVALID_OPERATION is generated if width is not a multiple of four, and width + xoffset is not equal to the width of the texture level.");
	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 4, 0, 10, 4, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(10, 4), 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("For ETC2/EAC images GL_INVALID_OPERATION is generated if height is not a multiple of four, and height + yoffset is not equal to the height of the texture level.");
	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 4, 4, 10, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(4, 10), 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("For ETC2/EAC images GL_INVALID_OPERATION is generated if xoffset or yoffset is not a multiple of four.");
	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 1, 4, 4, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(4, 4), 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 1, 0, 4, 4, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(4, 4), 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 1, 1, 4, 4, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(4, 4), 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteTextures		(1, &texture);
}

void compressedtexsubimage2d_neg_level (NegativeTestContext& ctx)
{
	deUint32				textures[2];
	ctx.glGenTextures			(2, &textures[0]);
	ctx.glBindTexture			(GL_TEXTURE_2D, textures[0]);
	ctx.glCompressedTexImage2D	(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 18, 18, 0, etc2EacDataSize(18, 18), 0);
	ctx.glBindTexture			(GL_TEXTURE_CUBE_MAP, textures[1]);
	FOR_CUBE_FACES(faceGL, ctx.glCompressedTexImage2D(faceGL, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 18, 18, 0, etc2EacDataSize(18, 18), 0););
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if level is less than 0.");
	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, -1, 0, 0, 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if level is less than 0.");
	FOR_CUBE_FACES(faceGL,
	{
		ctx.glCompressedTexSubImage2D(faceGL, -1, 0, 0, 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
		ctx.expectError(GL_INVALID_VALUE);
	});
	ctx.endSection();

	ctx.glDeleteTextures(2, &textures[0]);
}

void compressedtexsubimage2d_max_level (NegativeTestContext& ctx)
{
	deUint32				textures[2];
	ctx.glGenTextures			(2, &textures[0]);
	ctx.glBindTexture			(GL_TEXTURE_2D, textures[0]);
	ctx.glCompressedTexImage2D	(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 18, 18, 0, etc2EacDataSize(18, 18), 0);
	ctx.glBindTexture			(GL_TEXTURE_CUBE_MAP, textures[1]);
	FOR_CUBE_FACES(faceGL, ctx.glCompressedTexImage2D(faceGL, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 18, 18, 0, etc2EacDataSize(18, 18), 0););
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_TEXTURE_SIZE).");
	deUint32 log2MaxTextureSize = deLog2Floor32(ctx.getInteger(GL_MAX_TEXTURE_SIZE)) + 1;
	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, log2MaxTextureSize, 0, 0, 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_CUBE_MAP_TEXTURE_SIZE).");
	deUint32 log2MaxCubemapSize = deLog2Floor32(ctx.getInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE)) + 1;
	FOR_CUBE_FACES(faceGL,
	{
		ctx.glCompressedTexSubImage2D(faceGL, log2MaxCubemapSize, 0, 0, 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
		ctx.expectError(GL_INVALID_VALUE);
	});
	ctx.endSection();

	ctx.glDeleteTextures(2, &textures[0]);
}

void compressedtexsubimage2d_neg_offset (NegativeTestContext& ctx)
{
	GLuint texture = 0x1234;
	ctx.glGenTextures(1, &texture);
	ctx.glBindTexture(GL_TEXTURE_2D, texture);
	ctx.glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 8, 8, 0, etc2EacDataSize(8, 8), 0);

	// \note Both GL_INVALID_VALUE and GL_INVALID_OPERATION are valid here since implementation may
	//		 first check if offsets are valid for certain format and only after that check that they
	//		 are not negative.
	ctx.beginSection("GL_INVALID_VALUE or GL_INVALID_OPERATION is generated if xoffset or yoffset are negative.");

	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, -4, 0, 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, -4, 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, -4, -4, 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);

	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

void compressedtexsubimage2d_invalid_offset (NegativeTestContext& ctx)
{
	deUint32				texture = 0x1234;
	ctx.glGenTextures			(1, &texture);
	ctx.glBindTexture			(GL_TEXTURE_2D, texture);
	ctx.glCompressedTexImage2D	(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 0, etc2EacDataSize(16, 16), 0);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE or GL_INVALID_OPERATION is generated if xoffset + width > texture_width or yoffset + height > texture_height.");

	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 12, 0, 8, 4, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(8, 4), 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 12, 4, 8, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(4, 8), 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 12, 12, 8, 8, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(8, 8), 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteTextures		(1, &texture);
}

void compressedtexsubimage2d_neg_width_height (NegativeTestContext& ctx)
{
	deUint32				texture = 0x1234;
	ctx.glGenTextures			(1, &texture);
	ctx.glBindTexture			(GL_TEXTURE_2D, texture);
	ctx.glCompressedTexImage2D	(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 0, etc2EacDataSize(16, 16), 0);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE or GL_INVALID_OPERATION is generated if width or height is less than 0.");
	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, -4, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, -4, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, -4, -4, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteTextures(1,		&texture);
}

void compressedtexsubimage2d_invalid_size (NegativeTestContext& ctx)
{
	deUint32				texture = 0x1234;
	ctx.glGenTextures			(1, &texture);
	ctx.glBindTexture			(GL_TEXTURE_2D, texture);
	ctx.glCompressedTexImage2D	(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 0, etc2EacDataSize(16, 16), 0);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if imageSize is not consistent with the format, dimensions, and contents of the specified compressed image data.");
	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);

	ctx.glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 16, 16, GL_COMPRESSED_RGBA8_ETC2_EAC, 4*4*16-1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteTextures		(1, &texture);
}

void compressedtexsubimage2d_invalid_buffer_target (NegativeTestContext& ctx)
{
	deUint32					buf = 0x1234;
	deUint32					texture = 0x1234;
	std::vector<GLubyte>		data(128);

	ctx.glGenTextures				(1, &texture);
	ctx.glBindTexture				(GL_TEXTURE_2D, texture);
	ctx.glCompressedTexImage2D		(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 0, etc2EacDataSize(16, 16), 0);
	ctx.glGenBuffers				(1, &buf);
	ctx.glBindBuffer				(GL_PIXEL_UNPACK_BUFFER, buf);
	ctx.glBufferData				(GL_PIXEL_UNPACK_BUFFER, 128, &data[0], GL_DYNAMIC_COPY);
	ctx.expectError					(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_OPERATION is generated if a non-zero buffer object name is bound to the GL_PIXEL_UNPACK_BUFFER target and...");
	ctx.beginSection("...the buffer object's data store is currently mapped.");
	ctx.glMapBufferRange			(GL_PIXEL_UNPACK_BUFFER, 0, 128, GL_MAP_WRITE_BIT);
	ctx.glCompressedTexSubImage2D	(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(4, 4), 0);
	ctx.expectError					(GL_INVALID_OPERATION);
	ctx.glUnmapBuffer				(GL_PIXEL_UNPACK_BUFFER);
	ctx.endSection();

	ctx.beginSection("...the data would be unpacked from the buffer object such that the memory reads required would exceed the data store size.");
	ctx.glCompressedTexSubImage2D	(GL_TEXTURE_2D, 0, 0, 0, 16, 16, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(16, 16), 0);
	ctx.expectError					(GL_INVALID_OPERATION);
	ctx.endSection();
	ctx.endSection();

	ctx.glDeleteBuffers			(1, &buf);
	ctx.glDeleteTextures		(1, &texture);
}

// glTexImage3D

void teximage3d (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if target is invalid.");
	ctx.glTexImage3D(0, 0, GL_RGBA, 1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexImage3D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if type is not a type constant.");
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 1, 1, 1, 0, GL_RGBA, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if format is not an accepted format constant.");
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 1, 1, 1, 0, 0, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if internalFormat is not one of the accepted resolution and format symbolic constants.");
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, 0, 1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if target is GL_TEXTURE_3D and format is GL_DEPTH_COMPONENT, or GL_DEPTH_STENCIL.");
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_DEPTH_STENCIL, 1, 1, 1, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_DEPTH_COMPONENT, 1, 1, 1, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if the combination of internalFormat, format and type is invalid.");
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, 1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 1, 1, 1, 0, GL_RGB, GL_UNSIGNED_SHORT_4_4_4_4, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB5_A1, 1, 1, 1, 0, GL_RGB, GL_UNSIGNED_SHORT_5_5_5_1, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB10_A2, 1, 1, 1, 0, GL_RGB, GL_UNSIGNED_INT_2_10_10_10_REV, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32UI, 1, 1, 1, 0, GL_RGBA_INTEGER, GL_INT, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_cube_map_array"))
	{
		ctx.beginSection("GL_INVALID_VALUE is generated if target is GL_TEXTURE_CUBE_MAP_ARRAY and width and height are not equal.");
		ctx.glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA, 2, 1, 6, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.endSection();

		ctx.beginSection("GL_INVALID_VALUE is generated if target is GL_TEXTURE_CUBE_MAP_ARRAY and depth is not a multiple of six.");
		ctx.glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA, 1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.endSection();
	}
}

void teximage3d_neg_level (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if level is less than 0.");
	ctx.glTexImage3D(GL_TEXTURE_3D, -1, GL_RGB, 1, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage3D(GL_TEXTURE_2D_ARRAY, -1, GL_RGB, 1, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_cube_map_array"))
	{
		ctx.glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, -1, GL_RGBA, 1, 1, 6, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
	}

	ctx.endSection();

}

void teximage3d_max_level (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_3D_TEXTURE_SIZE).");
	deUint32 log2Max3DTextureSize = deLog2Floor32(ctx.getInteger(GL_MAX_3D_TEXTURE_SIZE)) + 1;
	ctx.glTexImage3D(GL_TEXTURE_3D, log2Max3DTextureSize, GL_RGB, 1, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_TEXTURE_SIZE).");
	deUint32 log2MaxTextureSize = deLog2Floor32(ctx.getInteger(GL_MAX_TEXTURE_SIZE)) + 1;
	ctx.glTexImage3D(GL_TEXTURE_2D_ARRAY, log2MaxTextureSize, GL_RGB, 1, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void teximage3d_neg_width_height_depth (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if width or height is less than 0.");
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, -1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 1, -1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 1, 1, -1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, -1, -1, -1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);

	ctx.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, -1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 1, -1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 1, 1, -1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, -1, -1, -1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_cube_map_array"))
	{
		ctx.glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA, -1, 1, 6, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA, 1, -1, 6, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA, 1, 1, -6, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA, -1, -1, -6, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
	}
	ctx.endSection();
}

void teximage3d_max_width_height_depth (NegativeTestContext& ctx)
{
	int max3DTextureSize	= ctx.getInteger(GL_MAX_3D_TEXTURE_SIZE) + 1;
	int maxTextureSize		= ctx.getInteger(GL_MAX_TEXTURE_SIZE) + 1;

	ctx.beginSection("GL_INVALID_VALUE is generated if width, height or depth is greater than GL_MAX_3D_TEXTURE_SIZE.");
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, max3DTextureSize, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 1, max3DTextureSize, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 1, 1, max3DTextureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, max3DTextureSize, max3DTextureSize, max3DTextureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if width, height or depth is greater than GL_MAX_TEXTURE_SIZE.");
	ctx.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, maxTextureSize, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 1, maxTextureSize, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 1, 1, maxTextureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, maxTextureSize, maxTextureSize, maxTextureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void teximage3d_invalid_border (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if border is not 0 or 1.");
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, 1, 1, 1, -1, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, 1, 1, 1, 2, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, 1, 1, 1, -1, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, 1, 1, 1, 2, GL_RGB, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_cube_map_array"))
	{
		ctx.glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_RGBA, 1, 1, 6, -1, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_RGBA, 1, 1, 6, 1, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
	}

	ctx.endSection();
}

void teximage3d_invalid_buffer_target (NegativeTestContext& ctx)
{
	deUint32				buf = 0x1234;
	deUint32				texture = 0x1234;
	std::vector<GLubyte>	data(512);

	ctx.glGenBuffers			(1, &buf);
	ctx.glBindBuffer			(GL_PIXEL_UNPACK_BUFFER, buf);
	ctx.glBufferData			(GL_PIXEL_UNPACK_BUFFER, 512, &data[0], GL_DYNAMIC_COPY);
	ctx.glGenTextures			(1, &texture);
	ctx.glBindTexture			(GL_TEXTURE_3D, texture);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_OPERATION is generated if a non-zero buffer object name is bound to the GL_PIXEL_UNPACK_BUFFER target and...");

	ctx.beginSection("...the buffer object's data store is currently mapped.");
	ctx.glMapBufferRange		(GL_PIXEL_UNPACK_BUFFER, 0, 128, GL_MAP_WRITE_BIT);
	ctx.glTexImage3D			(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError				(GL_INVALID_OPERATION);
	ctx.glUnmapBuffer			(GL_PIXEL_UNPACK_BUFFER);
	ctx.endSection();

	ctx.beginSection("...the data would be unpacked from the buffer object such that the memory reads required would exceed the data store size.");
	ctx.glTexImage3D			(GL_TEXTURE_3D, 0, GL_RGBA, 64, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError				(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("...data is not evenly divisible into the number of bytes needed to store in memory a datum indicated by type.");
	ctx.getLog() << TestLog::Message << "// Set byte offset = 3" << TestLog::EndMessage;
	ctx.glTexImage3D			(GL_TEXTURE_3D, 0, GL_RGB5_A1, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, (const GLvoid*)3);
	ctx.expectError				(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.endSection();

	ctx.glDeleteBuffers			(1, &buf);
	ctx.glDeleteTextures		(1, &texture);
}

// glTexSubImage3D

void texsubimage3d (NegativeTestContext& ctx)
{
	deUint32			texture = 0x1234;
	ctx.glGenTextures		(1, &texture);
	ctx.glBindTexture		(GL_TEXTURE_3D, texture);
	ctx.glTexImage3D		(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError			(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_ENUM is generated if target is invalid.");
	ctx.glTexSubImage3D(0, 0, 0, 0, 0, 4, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexSubImage3D(GL_TEXTURE_2D, 0, 0, 0, 0, 4, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if format is not an accepted format constant.");
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 0, 4, 4, 4, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if type is not a type constant.");
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 4, 4, 4, GL_RGB, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if the combination of internalFormat of the previously specified texture array, format and type is not valid.");
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 4, 4, 4, GL_RGB, GL_UNSIGNED_SHORT_4_4_4_4, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 4, 4, 4, GL_RGB, GL_UNSIGNED_SHORT_5_5_5_1, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 4, 4, 4, GL_RGB, GL_UNSIGNED_SHORT_5_5_5_1, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 4, 4, 4, GL_RGBA_INTEGER, GL_UNSIGNED_INT, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 4, 4, 4, GL_RGB, GL_FLOAT, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteTextures	(1, &texture);
}

void texsubimage3d_neg_level (NegativeTestContext& ctx)
{
	deUint32			textures[3];
	ctx.glGenTextures		(3, &textures[0]);
	ctx.glBindTexture		(GL_TEXTURE_3D, textures[0]);
	ctx.glTexImage3D		(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glBindTexture		(GL_TEXTURE_2D_ARRAY, textures[1]);
	ctx.glTexImage3D		(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError			(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if level is less than 0.");
	ctx.glTexSubImage3D(GL_TEXTURE_3D, -1, 0, 0, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage3D(GL_TEXTURE_2D_ARRAY, -1, 0, 0, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_cube_map_array"))
	{
		ctx.glBindTexture		(GL_TEXTURE_CUBE_MAP_ARRAY, textures[2]);
		ctx.glTexImage3D		(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA, 4, 4, 6, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError			(GL_NO_ERROR);

		ctx.glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, -1, 0, 0, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
	}

	ctx.endSection();

	ctx.glDeleteTextures	(3, &textures[0]);
}

void texsubimage3d_max_level (NegativeTestContext& ctx)
{
	deUint32			textures[2];
	ctx.glGenTextures		(2, &textures[0]);
	ctx.glBindTexture		(GL_TEXTURE_3D, textures[0]);
	ctx.glTexImage3D		(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glBindTexture		(GL_TEXTURE_2D_ARRAY, textures[1]);
	ctx.glTexImage3D		(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError			(GL_NO_ERROR);

	deUint32 log2Max3DTextureSize	= deLog2Floor32(ctx.getInteger(GL_MAX_3D_TEXTURE_SIZE)) + 1;
	deUint32 log2MaxTextureSize		= deLog2Floor32(ctx.getInteger(GL_MAX_TEXTURE_SIZE)) + 1;

	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_3D_TEXTURE_SIZE).");
	ctx.glTexSubImage3D(GL_TEXTURE_3D, log2Max3DTextureSize, 0, 0, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_TEXTURE_SIZE).");
	ctx.glTexSubImage3D(GL_TEXTURE_2D_ARRAY, log2MaxTextureSize, 0, 0, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteTextures	(2, &textures[0]);
}

void texsubimage3d_neg_offset (NegativeTestContext& ctx)
{
	deUint32			textures[3];
	ctx.glGenTextures		(3, &textures[0]);
	ctx.glBindTexture		(GL_TEXTURE_3D, textures[0]);
	ctx.glTexImage3D		(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glBindTexture		(GL_TEXTURE_2D_ARRAY, textures[1]);
	ctx.glTexImage3D		(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError			(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if xoffset, yoffset or zoffset are negative.");
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, -1, 0, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, 0, -1, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, -1, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, -1, -1, -1, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, -1, 0, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, -1, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, -1, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, -1, -1, -1, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_cube_map_array"))
	{
		ctx.glBindTexture		(GL_TEXTURE_CUBE_MAP_ARRAY, textures[2]);
		ctx.glTexImage3D		(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA, 4, 4, 6, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError			(GL_NO_ERROR);

		ctx.glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, -1, 0, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, -1, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, -1, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, -1, -1, -1, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
	}

	ctx.endSection();

	ctx.glDeleteTextures	(3, &textures[0]);
}

void texsubimage3d_invalid_offset (NegativeTestContext& ctx)
{
	deUint32			texture = 0x1234;
	ctx.glGenTextures		(1, &texture);
	ctx.glBindTexture		(GL_TEXTURE_3D, texture);
	ctx.glTexImage3D		(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError			(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if xoffset + width > texture_width.");
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, 2, 0, 0, 4, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if yoffset + height > texture_height.");
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 2, 0, 4, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if zoffset + depth > texture_depth.");
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 2, 4, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteTextures	(1, &texture);
}

void texsubimage3d_neg_width_height (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if width, height or depth is less than 0.");
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, -1, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 0, -1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 0, 0, -1, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, -1, -1, -1, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_INVALID_VALUE);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_cube_map_array"))
	{
		ctx.glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, 0, -1, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, 0, 0, -1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, 0, 0, 0, -1, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, 0, -1, -1, -1, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_INVALID_VALUE);
	}

	ctx.endSection();
}

void texsubimage3d_invalid_buffer_target (NegativeTestContext& ctx)
{
	deUint32				buf = 0x1234;
	deUint32				texture = 0x1234;
	std::vector<GLubyte>	data(512);

	ctx.glGenTextures			(1, &texture);
	ctx.glBindTexture			(GL_TEXTURE_3D, texture);
	ctx.glTexImage3D			(GL_TEXTURE_3D, 0, GL_RGBA, 16, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glGenBuffers			(1, &buf);
	ctx.glBindBuffer			(GL_PIXEL_UNPACK_BUFFER, buf);
	ctx.glBufferData			(GL_PIXEL_UNPACK_BUFFER, 512, &data[0], GL_DYNAMIC_COPY);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_OPERATION is generated if a non-zero buffer object name is bound to the GL_PIXEL_UNPACK_BUFFER target and...");

	ctx.beginSection("...the buffer object's data store is currently mapped.");
	ctx.glMapBufferRange		(GL_PIXEL_UNPACK_BUFFER, 0, 512, GL_MAP_WRITE_BIT);
	ctx.glTexSubImage3D			(GL_TEXTURE_3D, 0, 0, 0, 0, 4, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError				(GL_INVALID_OPERATION);
	ctx.glUnmapBuffer			(GL_PIXEL_UNPACK_BUFFER);
	ctx.endSection();

	ctx.beginSection("...the data would be unpacked from the buffer object such that the memory reads required would exceed the data store size.");
	ctx.glTexSubImage3D			(GL_TEXTURE_3D, 0, 0, 0, 0, 16, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError				(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("...data is not evenly divisible into the number of bytes needed to store in memory a datum indicated by type.");
	ctx.getLog() << TestLog::Message << "// Set byte offset = 3" << TestLog::EndMessage;
	ctx.glBindBuffer			(GL_PIXEL_UNPACK_BUFFER, 0);
	ctx.glTexImage3D			(GL_TEXTURE_3D, 0, GL_RGBA4, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, 0);
	ctx.glBindBuffer			(GL_PIXEL_UNPACK_BUFFER, buf);
	ctx.expectError				(GL_NO_ERROR);
	ctx.glTexSubImage3D			(GL_TEXTURE_3D, 0, 0, 0, 0, 4, 4, 4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, (const GLvoid*)3);
	ctx.expectError				(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.endSection();

	ctx.glDeleteBuffers			(1, &buf);
	ctx.glDeleteTextures		(1, &texture);
}

// glCopyTexSubImage3D

void copytexsubimage3d (NegativeTestContext& ctx)
{
	GLuint texture = 0x1234;
	ctx.glGenTextures	(1, &texture);
	ctx.glBindTexture	(GL_TEXTURE_3D, texture);
	ctx.glTexImage3D	(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	ctx.beginSection("GL_INVALID_ENUM is generated if target is invalid.");
	ctx.glCopyTexSubImage3D(0, 0, 0, 0, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

void copytexsubimage3d_neg_level (NegativeTestContext& ctx)
{
	deUint32	textures[3];
	ctx.glGenTextures(3, &textures[0]);
	ctx.glBindTexture(GL_TEXTURE_3D, textures[0]);
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glBindTexture(GL_TEXTURE_2D_ARRAY, textures[1]);
	ctx.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if level is less than 0.");
	ctx.glCopyTexSubImage3D(GL_TEXTURE_3D, -1, 0, 0, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexSubImage3D(GL_TEXTURE_2D_ARRAY, -1, 0, 0, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_cube_map_array"))
	{
		ctx.glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textures[2]);
		ctx.glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA, 4, 4, 6, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_NO_ERROR);
		ctx.glCopyTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, -1, 0, 0, 0, 0, 0, 4, 4);
		ctx.expectError(GL_INVALID_VALUE);
	}

	ctx.endSection();

	ctx.glDeleteTextures(3, &textures[0]);
}

void copytexsubimage3d_max_level (NegativeTestContext& ctx)
{
	deUint32	log2Max3DTextureSize		= deLog2Floor32(ctx.getInteger(GL_MAX_3D_TEXTURE_SIZE)) + 1;
	deUint32	log2MaxTextureSize			= deLog2Floor32(ctx.getInteger(GL_MAX_TEXTURE_SIZE)) + 1;
	deUint32	log2MaxCubeMapTextureSize	= deLog2Floor32(ctx.getInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE)) + 1;

	deUint32	textures[3];
	ctx.glGenTextures(3, &textures[0]);
	ctx.glBindTexture(GL_TEXTURE_3D, textures[0]);
	ctx.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glBindTexture(GL_TEXTURE_2D_ARRAY, textures[1]);
	ctx.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_3D_TEXTURE_SIZE).");
	ctx.glCopyTexSubImage3D(GL_TEXTURE_3D, log2Max3DTextureSize, 0, 0, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_TEXTURE_SIZE).");
	ctx.glCopyTexSubImage3D(GL_TEXTURE_2D_ARRAY, log2MaxTextureSize, 0, 0, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_cube_map_array"))
	{
		ctx.glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textures[2]);
		ctx.glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA, 4, 4, 6, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		ctx.expectError(GL_NO_ERROR);
		ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_CUBE_MAP_TEXTURE_SIZE).");
		ctx.glCopyTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, log2MaxCubeMapTextureSize, 0, 0, 0, 0, 0, 4, 4);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.endSection();
	}

	ctx.glDeleteTextures(3, &textures[0]);
}

void copytexsubimage3d_neg_offset (NegativeTestContext& ctx)
{
	GLuint texture = 0x1234;
	ctx.glGenTextures	(1, &texture);
	ctx.glBindTexture	(GL_TEXTURE_3D, texture);
	ctx.glTexImage3D	(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	ctx.beginSection("GL_INVALID_VALUE is generated if xoffset, yoffset or zoffset is negative.");
	ctx.glCopyTexSubImage3D(GL_TEXTURE_3D, 0, -1, 0,  0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexSubImage3D(GL_TEXTURE_3D, 0, 0, -1, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, -1, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCopyTexSubImage3D(GL_TEXTURE_3D, 0, -1, -1, -1, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

void copytexsubimage3d_invalid_offset (NegativeTestContext& ctx)
{
	GLuint texture = 0x1234;
	ctx.glGenTextures	(1, &texture);
	ctx.glBindTexture	(GL_TEXTURE_3D, texture);
	ctx.glTexImage3D	(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	ctx.beginSection("GL_INVALID_VALUE is generated if xoffset + width > texture_width.");
	ctx.glCopyTexSubImage3D(GL_TEXTURE_3D, 0, 1, 0, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if yoffset + height > texture_height.");
	ctx.glCopyTexSubImage3D(GL_TEXTURE_3D, 0, 0, 1, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if zoffset + 1 > texture_depth.");
	ctx.glCopyTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 4, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

void copytexsubimage3d_neg_width_height (NegativeTestContext& ctx)
{
	GLuint texture = 0x1234;
	ctx.glGenTextures	(1, &texture);
	ctx.glBindTexture	(GL_TEXTURE_3D, texture);
	ctx.glTexImage3D	(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	ctx.beginSection("GL_INVALID_VALUE is generated if width < 0.");
	ctx.glCopyTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 0, 0, -4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if height < 0.");
	ctx.glCopyTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 0, 0, 4, -4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

void copytexsubimage3d_incomplete_framebuffer (NegativeTestContext& ctx)
{
	GLuint fbo = 0x1234;
	GLuint texture[2];

	ctx.glGenTextures			(2, texture);
	ctx.glBindTexture			(GL_TEXTURE_3D, texture[0]);
	ctx.glTexImage3D			(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glBindTexture			(GL_TEXTURE_2D_ARRAY, texture[1]);
	ctx.glTexImage3D			(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	ctx.glGenFramebuffers		(1, &fbo);
	ctx.glBindFramebuffer		(GL_READ_FRAMEBUFFER, fbo);
	ctx.glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);

	ctx.beginSection("GL_INVALID_FRAMEBUFFER_OPERATION is generated if the currently bound framebuffer is not framebuffer complete.");
	ctx.glCopyTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_FRAMEBUFFER_OPERATION);
	ctx.glCopyTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 0, 0, 4, 4);
	ctx.expectError(GL_INVALID_FRAMEBUFFER_OPERATION);
	ctx.endSection();

	ctx.glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ctx.glDeleteFramebuffers(1, &fbo);
	ctx.glDeleteTextures(2, texture);
}

// glCompressedTexImage3D

void compressedteximage3d (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if target is invalid.");
	ctx.glCompressedTexImage3D(0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glCompressedTexImage3D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if internalformat is not one of the specific compressed internal formats.");
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void compressedteximage3d_neg_level (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if level is less than 0.");
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, -1, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void compressedteximage3d_max_level (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_TEXTURE_SIZE).");
	deUint32 log2MaxTextureSize = deLog2Floor32(ctx.getInteger(GL_MAX_TEXTURE_SIZE)) + 1;
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, log2MaxTextureSize, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void compressedteximage3d_neg_width_height_depth (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if width, height or depth is less than 0.");
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, -1, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, -1, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, -1, -1, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void compressedteximage3d_max_width_height_depth (NegativeTestContext& ctx)
{
	int maxTextureSize = ctx.getInteger(GL_MAX_TEXTURE_SIZE) + 1;

	ctx.beginSection("GL_INVALID_VALUE is generated if width, height or depth is greater than GL_MAX_TEXTURE_SIZE.");
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, maxTextureSize, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, maxTextureSize, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, maxTextureSize, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, maxTextureSize, maxTextureSize, maxTextureSize, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void compressedteximage3d_invalid_border (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if border is not 0.");
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, -1, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, 1, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void compressedteximage3d_invalid_size (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_VALUE is generated if imageSize is not consistent with the format, dimensions, and contents of the specified compressed image data.");
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0, 0, 0, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 1, 0, 4*4*8, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGB8_ETC2, 16, 16, 1, 0, 4*4*16, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_SIGNED_R11_EAC, 16, 16, 1, 0, 4*4*16, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void compressedteximage3d_invalid_width_height (NegativeTestContext& ctx)
{
	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_cube_map_array"))
	{
		const int				width		= 4;
		const int				height		= 6;
		const int				depth		= 6;
		const int				blockSize	= 16;
		const int				imageSize	= divRoundUp(width, 4) * divRoundUp(height, 4) * depth * blockSize;
		std::vector<GLubyte>	data		(imageSize);
		ctx.beginSection("GL_INVALID_VALUE is generated if target is GL_TEXTURE_CUBE_MAP_ARRAY and width and height are not equal.");
		ctx.glCompressedTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_COMPRESSED_RGB8_ETC2, width, height, depth, 0, imageSize, &data[0]);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.endSection();
	}
}

void compressedteximage3d_invalid_buffer_target (NegativeTestContext& ctx)
{
	deUint32				buf = 0x1234;
	std::vector<GLubyte>	data(512);

	ctx.glGenBuffers			(1, &buf);
	ctx.glBindBuffer			(GL_PIXEL_UNPACK_BUFFER, buf);
	ctx.glBufferData			(GL_PIXEL_UNPACK_BUFFER, 64, &data[0], GL_DYNAMIC_COPY);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_OPERATION is generated if a non-zero buffer object name is bound to the GL_PIXEL_UNPACK_BUFFER target and the buffer object's data store is currently mapped.");
	ctx.glMapBufferRange		(GL_PIXEL_UNPACK_BUFFER, 0, 64, GL_MAP_WRITE_BIT);
	ctx.glCompressedTexImage3D	(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGB8_ETC2, 4, 4, 1, 0, etc2DataSize(4, 4), 0);
	ctx.expectError				(GL_INVALID_OPERATION);
	ctx.glUnmapBuffer			(GL_PIXEL_UNPACK_BUFFER);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if a non-zero buffer object name is bound to the GL_PIXEL_UNPACK_BUFFER target and the data would be unpacked from the buffer object such that the memory reads required would exceed the data store size.");
	ctx.glCompressedTexImage3D	(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGB8_ETC2, 16, 16, 1, 0, etc2DataSize(16, 16), 0);
	ctx.expectError				(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteBuffers			(1, &buf);
}

// glCompressedTexSubImage3D

void compressedtexsubimage3d (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if target is invalid.");
	ctx.glCompressedTexSubImage3D(0, 0, 0, 0, 0, 0, 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	deUint32				texture = 0x1234;
	ctx.glGenTextures			(1, &texture);
	ctx.glBindTexture			(GL_TEXTURE_2D_ARRAY, texture);
	ctx.glCompressedTexImage3D	(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 18, 18, 1, 0, etc2EacDataSize(18, 18), 0);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_OPERATION is generated if format does not match the internal format of the texture image being modified.");
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 0, 0, 0, GL_COMPRESSED_RGB8_ETC2, 0, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if internalformat is an ETC2/EAC format and target is not GL_TEXTURE_2D_ARRAY.");
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 18, 18, 1, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(18, 18), 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("For ETC2/EAC images GL_INVALID_OPERATION is generated if width is not a multiple of four, and width + xoffset is not equal to the width of the texture level.");
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 4, 0, 0, 10, 4, 1, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(10, 4), 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("For ETC2/EAC images GL_INVALID_OPERATION is generated if height is not a multiple of four, and height + yoffset is not equal to the height of the texture level.");
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 4, 0, 4, 10, 1, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(4, 10), 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("For ETC2/EAC images GL_INVALID_OPERATION is generated if xoffset or yoffset is not a multiple of four.");
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 1, 0, 0, 4, 4, 1, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(4, 4), 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 1, 0, 4, 4, 1, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(4, 4), 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 1, 1, 0, 4, 4, 1, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(4, 4), 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteTextures		(1, &texture);
}

void compressedtexsubimage3d_neg_level (NegativeTestContext& ctx)
{
	deUint32				texture = 0x1234;
	ctx.glGenTextures			(1, &texture);
	ctx.glBindTexture			(GL_TEXTURE_2D_ARRAY, texture);
	ctx.glCompressedTexImage3D	(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 1, 0, etc2EacDataSize(16, 16), 0);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if level is less than 0.");
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, -1, 0, 0, 0, 0, 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteTextures		(1, &texture);
}

void compressedtexsubimage3d_max_level (NegativeTestContext& ctx)
{
	deUint32				texture = 0x1234;
	ctx.glGenTextures			(1, &texture);
	ctx.glBindTexture			(GL_TEXTURE_2D_ARRAY, texture);
	ctx.glCompressedTexImage3D	(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 1, 0, etc2EacDataSize(16, 16), 0);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_TEXTURE_SIZE).");
	deUint32 log2MaxTextureSize = deLog2Floor32(ctx.getInteger(GL_MAX_TEXTURE_SIZE)) + 1;
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, log2MaxTextureSize, 0, 0, 0, 0, 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteTextures		(1, &texture);
}

void compressedtexsubimage3d_neg_offset (NegativeTestContext& ctx)
{
	deUint32				texture = 0x1234;
	ctx.glGenTextures			(1, &texture);
	ctx.glBindTexture			(GL_TEXTURE_2D_ARRAY, texture);
	ctx.glCompressedTexImage3D	(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 1, 0, etc2EacDataSize(16, 16), 0);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE or GL_INVALID_OPERATION is generated if xoffset, yoffset or zoffset are negative.");
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, -4, 0, 0, 0, 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, -4, 0, 0, 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, -4, 0, 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, -4, -4, -4, 0, 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteTextures		(1, &texture);
}

void compressedtexsubimage3d_invalid_offset (NegativeTestContext& ctx)
{
	deUint32				texture = 0x1234;
	ctx.glGenTextures			(1, &texture);
	ctx.glBindTexture			(GL_TEXTURE_2D_ARRAY, texture);
	ctx.glCompressedTexImage3D	(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 4, 4, 1, 0, etc2EacDataSize(4, 4), 0);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE or GL_INVALID_OPERATION is generated if xoffset + width > texture_width or yoffset + height > texture_height.");

	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 12, 0, 0, 8, 4, 1, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(8, 4), 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 12, 0, 4, 8, 1, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(4, 8), 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 12, 4, 4, 1, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(4, 4), 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 12, 12, 12, 8, 8, 1, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(8, 8), 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteTextures		(1, &texture);
}

void compressedtexsubimage3d_neg_width_height_depth (NegativeTestContext& ctx)
{
	deUint32				texture = 0x1234;
	ctx.glGenTextures			(1, &texture);
	ctx.glBindTexture			(GL_TEXTURE_2D_ARRAY, texture);
	ctx.glCompressedTexImage3D	(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 1, 0, etc2EacDataSize(16, 16), 0);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE or GL_INVALID_OPERATION is generated if width, height or depth are negative.");
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, -4, 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 0, -4, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 0, 0, -4, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, -4, -4, -4, GL_COMPRESSED_RGBA8_ETC2_EAC, 0, 0);
	ctx.expectError(GL_INVALID_VALUE, GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteTextures		(1, &texture);
}

void compressedtexsubimage3d_invalid_size (NegativeTestContext& ctx)
{
	deUint32				texture = 0x1234;
	ctx.glGenTextures			(1, &texture);
	ctx.glBindTexture			(GL_TEXTURE_2D_ARRAY, texture);
	ctx.glCompressedTexImage3D	(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 1, 0, 4*4*16, 0);
	ctx.expectError				(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if imageSize is not consistent with the format, dimensions, and contents of the specified compressed image data.");
	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 16, 16, 1, GL_COMPRESSED_RGBA8_ETC2_EAC, -1, 0);
	ctx.expectError(GL_INVALID_VALUE);

	ctx.glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 16, 16, 1, GL_COMPRESSED_RGBA8_ETC2_EAC, 4*4*16-1, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteTextures		(1, &texture);
}

void compressedtexsubimage3d_invalid_buffer_target (NegativeTestContext& ctx)
{
	deUint32						buf = 0x1234;
	deUint32						texture = 0x1234;
	GLsizei							bufferSize =  etc2EacDataSize(4, 4);
	std::vector<GLubyte>			data(bufferSize);

	ctx.glGenTextures				(1, &texture);
	ctx.glBindTexture				(GL_TEXTURE_2D_ARRAY, texture);
	ctx.glCompressedTexImage3D		(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, 16, 16, 1, 0, etc2EacDataSize(16, 16), 0);
	ctx.glGenBuffers				(1, &buf);
	ctx.glBindBuffer				(GL_PIXEL_UNPACK_BUFFER, buf);
	ctx.glBufferData				(GL_PIXEL_UNPACK_BUFFER, bufferSize, &data[0], GL_DYNAMIC_COPY);
	ctx.expectError					(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_OPERATION is generated if a non-zero buffer object name is bound to the GL_PIXEL_UNPACK_BUFFER target and...");
	ctx.beginSection("...the buffer object's data store is currently mapped.");
	ctx.glMapBufferRange			(GL_PIXEL_UNPACK_BUFFER, 0, bufferSize, GL_MAP_WRITE_BIT);
	ctx.glCompressedTexSubImage3D	(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 4, 4, 1, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(4, 4), 0);
	ctx.expectError					(GL_INVALID_OPERATION);
	ctx.glUnmapBuffer				(GL_PIXEL_UNPACK_BUFFER);
	ctx.endSection();

	ctx.beginSection("...the data would be unpacked from the buffer object such that the memory reads required would exceed the data store size.");
	ctx.glCompressedTexSubImage3D	(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 16, 16, 1, GL_COMPRESSED_RGBA8_ETC2_EAC, etc2EacDataSize(16, 16), 0);
	ctx.expectError					(GL_INVALID_OPERATION);
	ctx.endSection();
	ctx.endSection();

	ctx.glDeleteBuffers			(1, &buf);
	ctx.glDeleteTextures		(1, &texture);
}

// glTexStorage2D

void texstorage2d (NegativeTestContext& ctx)
{
	deUint32  textures[] = {0x1234, 0x1234};

	ctx.glGenTextures(2, textures);
	ctx.glBindTexture(GL_TEXTURE_2D, textures[0]);
	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_ENUM or GL_INVALID_VALUE is generated if internalformat is not a valid sized internal format.");
	ctx.glTexStorage2D(GL_TEXTURE_2D, 1, 0, 16, 16);
	ctx.expectError(GL_INVALID_ENUM, GL_INVALID_VALUE);
	ctx.glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA_INTEGER, 16, 16);
	ctx.expectError(GL_INVALID_ENUM, GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if target is not one of the accepted target enumerants.");
	ctx.glTexStorage2D(0, 1, GL_RGBA8, 16, 16);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexStorage2D(GL_TEXTURE_3D, 1, GL_RGBA8, 16, 16);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexStorage2D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 16, 16);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if width or height are less than 1.");
	ctx.glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 0, 16);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 16, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)))
	{
		ctx.glBindTexture(GL_TEXTURE_CUBE_MAP, textures[1]);
		ctx.expectError(GL_NO_ERROR);
		ctx.glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, 0, 16);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, 16, 0);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, 0, 0);
		ctx.expectError(GL_INVALID_VALUE);
	}
	ctx.endSection();

	ctx.glDeleteTextures(2, textures);
}

void texstorage2d_invalid_binding (NegativeTestContext& ctx)
{
	deUint32	textures[]		= {0x1234, 0x1234};
	deInt32		immutable		= 0x1234;
	const bool	supportsES32	= contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));

	ctx.beginSection("GL_INVALID_OPERATION is generated if the default texture object is curently bound to target.");
	ctx.glBindTexture(GL_TEXTURE_2D, 0);
	ctx.glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 16, 16);
	ctx.expectError(GL_INVALID_OPERATION);

	if (supportsES32)
	{
		ctx.glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		ctx.glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, 16, 16);
		ctx.expectError(GL_INVALID_OPERATION);
	}
	ctx.endSection();

	ctx.glGenTextures(2, textures);

	ctx.beginSection("GL_INVALID_OPERATION is generated if the texture object currently bound to target already has GL_TEXTURE_IMMUTABLE_FORMAT set to GL_TRUE.");
	ctx.glBindTexture(GL_TEXTURE_2D, textures[0]);
	ctx.glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);
	ctx.getLog() << TestLog::Message << "// GL_TEXTURE_IMMUTABLE_FORMAT = " << ((immutable != 0) ? "GL_TRUE" : "GL_FALSE") << TestLog::EndMessage;
	ctx.glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 16, 16);
	ctx.expectError(GL_NO_ERROR);
	ctx.glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);
	ctx.getLog() << TestLog::Message << "// GL_TEXTURE_IMMUTABLE_FORMAT = " << ((immutable != 0) ? "GL_TRUE" : "GL_FALSE") << TestLog::EndMessage;
	ctx.glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 16, 16);
	ctx.expectError(GL_INVALID_OPERATION);

	if (supportsES32)
	{
		ctx.glBindTexture(GL_TEXTURE_CUBE_MAP, textures[1]);
		ctx.glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);
		ctx.getLog() << TestLog::Message << "// GL_TEXTURE_IMMUTABLE_FORMAT = " << ((immutable != 0) ? "GL_TRUE" : "GL_FALSE") << TestLog::EndMessage;
		ctx.glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, 16, 16);
		ctx.expectError(GL_NO_ERROR);
		ctx.glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);
		ctx.getLog() << TestLog::Message << "// GL_TEXTURE_IMMUTABLE_FORMAT = " << ((immutable != 0) ? "GL_TRUE" : "GL_FALSE") << TestLog::EndMessage;
		ctx.glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, 16, 16);
		ctx.expectError(GL_INVALID_OPERATION);
	}
	ctx.endSection();

	ctx.glDeleteTextures(2, textures);
}

void texstorage2d_invalid_levels (NegativeTestContext& ctx)
{
	deUint32	textures[]		= {0x1234, 0x1234};
	deUint32	log2MaxSize		= deLog2Floor32(deMax32(16, 16)) + 1 + 1;
	const bool	supportsES32	= contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));

	ctx.glGenTextures(2, textures);
	ctx.glBindTexture(GL_TEXTURE_2D, textures[0]);

	if (supportsES32)
		ctx.glBindTexture(GL_TEXTURE_CUBE_MAP, textures[1]);

	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if levels is less than 1.");
	ctx.glTexStorage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 16, 16);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexStorage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);

	if (supportsES32)
	{
		ctx.glTexStorage2D(GL_TEXTURE_CUBE_MAP, 0, GL_RGBA8, 16, 16);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexStorage2D(GL_TEXTURE_CUBE_MAP, 0, GL_RGBA8, 0, 0);
		ctx.expectError(GL_INVALID_VALUE);
	}
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if levels is greater than floor(log_2(max(width, height))) + 1");
	ctx.glTexStorage2D(GL_TEXTURE_2D, log2MaxSize, GL_RGBA8, 16, 16);
	ctx.expectError(GL_INVALID_OPERATION);

	if (supportsES32)
	{
		ctx.glTexStorage2D(GL_TEXTURE_CUBE_MAP, log2MaxSize, GL_RGBA8, 16, 16);
		ctx.expectError(GL_INVALID_OPERATION);
	}
	ctx.endSection();

	ctx.glDeleteTextures(2, textures);
}

// glTexStorage3D

void texstorage3d (NegativeTestContext& ctx)
{
	deUint32 textures[] = {0x1234, 0x1234};

	ctx.glGenTextures(2, textures);
	ctx.glBindTexture(GL_TEXTURE_3D, textures[0]);
	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_ENUM or GL_INVALID_VALUE is generated if internalformat is not a valid sized internal format.");
	ctx.glTexStorage3D(GL_TEXTURE_3D, 1, 0, 4, 4, 4);
	ctx.expectError(GL_INVALID_ENUM, GL_INVALID_VALUE);
	ctx.glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA_INTEGER, 4, 4, 4);
	ctx.expectError(GL_INVALID_ENUM, GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if target is not one of the accepted target enumerants.");
	ctx.glTexStorage3D(0, 1, GL_RGBA8, 4, 4, 4);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexStorage3D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, 4, 4, 4);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glTexStorage3D(GL_TEXTURE_2D, 1, GL_RGBA8, 4, 4, 4);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if width, height or depth are less than 1.");
	ctx.glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, 0, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, 4, 0, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, 4, 4, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_cube_map_array"))
	{
		ctx.glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textures[1]);
		ctx.expectError(GL_NO_ERROR);
		ctx.glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_RGBA8, 0, 4, 4);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_RGBA8, 4, 0, 4);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_RGBA8, 4, 4, 0);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_RGBA8, 0, 0, 0);
		ctx.expectError(GL_INVALID_VALUE);
	}
	ctx.endSection();

	ctx.glDeleteTextures(2, textures);
}

void texstorage3d_invalid_binding (NegativeTestContext& ctx)
{
	deUint32	textures[]	= {0x1234, 0x1234};
	deInt32		immutable	= 0x1234;

	ctx.beginSection("GL_INVALID_OPERATION is generated if the default texture object is curently bound to target.");
	ctx.glBindTexture	(GL_TEXTURE_3D, 0);
	ctx.glTexStorage3D	(GL_TEXTURE_3D, 1, GL_RGBA8, 4, 4, 4);
	ctx.expectError		(GL_INVALID_OPERATION);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_cube_map_array"))
	{
		ctx.glBindTexture	(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
		ctx.glTexStorage3D	(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_RGBA8, 4, 4, 6);
		ctx.expectError		(GL_INVALID_OPERATION);
	}
	ctx.endSection();

	ctx.glGenTextures	(2, textures);

	ctx.beginSection("GL_INVALID_OPERATION is generated if the texture object currently bound to target already has GL_TEXTURE_IMMUTABLE_FORMAT set to GL_TRUE.");
	ctx.glBindTexture	(GL_TEXTURE_3D, textures[0]);
	ctx.glGetTexParameteriv(GL_TEXTURE_3D, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);
	ctx.getLog() << TestLog::Message << "// GL_TEXTURE_IMMUTABLE_FORMAT = " << ((immutable != 0) ? "GL_TRUE" : "GL_FALSE") << TestLog::EndMessage;
	ctx.glTexStorage3D	(GL_TEXTURE_3D, 1, GL_RGBA8, 4, 4, 4);
	ctx.expectError		(GL_NO_ERROR);
	ctx.glGetTexParameteriv(GL_TEXTURE_3D, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);
	ctx.getLog() << TestLog::Message << "// GL_TEXTURE_IMMUTABLE_FORMAT = " << ((immutable != 0) ? "GL_TRUE" : "GL_FALSE") << TestLog::EndMessage;
	ctx.glTexStorage3D	(GL_TEXTURE_3D, 1, GL_RGBA8, 4, 4, 4);
	ctx.expectError		(GL_INVALID_OPERATION);

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_cube_map_array"))
	{
		ctx.glBindTexture	(GL_TEXTURE_CUBE_MAP_ARRAY, textures[1]);
		ctx.glGetTexParameteriv(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);
		ctx.getLog() << TestLog::Message << "// GL_TEXTURE_IMMUTABLE_FORMAT = " << ((immutable != 0) ? "GL_TRUE" : "GL_FALSE") << TestLog::EndMessage;
		ctx.glTexStorage3D	(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_RGBA8, 4, 4, 6);
		ctx.expectError		(GL_NO_ERROR);
		ctx.glGetTexParameteriv(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);
		ctx.getLog() << TestLog::Message << "// GL_TEXTURE_IMMUTABLE_FORMAT = " << ((immutable != 0) ? "GL_TRUE" : "GL_FALSE") << TestLog::EndMessage;
		ctx.glTexStorage3D	(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_RGBA8, 4, 4, 6);
		ctx.expectError		(GL_INVALID_OPERATION);
	}
	ctx.endSection();

	ctx.glDeleteTextures(2, textures);
}

void texstorage3d_invalid_levels (NegativeTestContext& ctx)
{
	deUint32	textures[]		= {0x1234, 0x1234};
	deUint32	log2MaxSize		= deLog2Floor32(8) + 1 + 1;
	const bool	supportsES32	= contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	ctx.glGenTextures(2, textures);
	ctx.glBindTexture(GL_TEXTURE_3D, textures[0]);

	ctx.beginSection("GL_INVALID_VALUE is generated if levels is less than 1.");
	ctx.glTexStorage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 4, 4, 4);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glTexStorage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);

	if (supportsES32 || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_cube_map_array"))
	{
		ctx.glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textures[1]);
		ctx.glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA8, 4, 4, 6);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA8, 0, 0, 6);
		ctx.expectError(GL_INVALID_VALUE);
	}
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if levels is greater than floor(log_2(max(width, height, depth))) + 1");
	ctx.glTexStorage3D	(GL_TEXTURE_3D, log2MaxSize, GL_RGBA8, 8, 2, 2);
	ctx.expectError		(GL_INVALID_OPERATION);
	ctx.glTexStorage3D	(GL_TEXTURE_3D, log2MaxSize, GL_RGBA8, 2, 8, 2);
	ctx.expectError		(GL_INVALID_OPERATION);
	ctx.glTexStorage3D	(GL_TEXTURE_3D, log2MaxSize, GL_RGBA8, 2, 2, 8);
	ctx.expectError		(GL_INVALID_OPERATION);
	ctx.glTexStorage3D	(GL_TEXTURE_3D, log2MaxSize, GL_RGBA8, 8, 8, 8);
	ctx.expectError		(GL_INVALID_OPERATION);

	if (supportsES32 || ctx.getContextInfo().isExtensionSupported("GL_OES_texture_cube_map_array"))
	{
		ctx.glTexStorage3D	(GL_TEXTURE_CUBE_MAP_ARRAY, log2MaxSize, GL_RGBA8, 2, 2, 6);
		ctx.expectError		(GL_INVALID_OPERATION);
		ctx.glTexStorage3D	(GL_TEXTURE_CUBE_MAP_ARRAY, log2MaxSize, GL_RGBA8, 8, 8, 6);
		ctx.expectError		(GL_INVALID_OPERATION);
	}
	ctx.endSection();

	ctx.glDeleteTextures(2, textures);
}

void srgb_decode_texparameteri (NegativeTestContext& ctx)
{
	if (!ctx.isExtensionSupported("GL_EXT_texture_sRGB_decode"))
		TCU_THROW(NotSupportedError, "GL_EXT_texture_sRGB_decode is not supported.");

	GLuint	texture		= 0x1234;
	GLint	textureMode	= -1;

	ctx.glGenTextures(1, &texture);
	ctx.glBindTexture(GL_TEXTURE_2D, texture);

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is GL_TEXTURE_SRGB_DECODE_EXT and the value of param(s) is not a valid value (one of DECODE_EXT, or SKIP_DECODE_EXT).");
	ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

void srgb_decode_texparameterf (NegativeTestContext& ctx)
{
	if (!ctx.isExtensionSupported("GL_EXT_texture_sRGB_decode"))
		TCU_THROW(NotSupportedError, "GL_EXT_texture_sRGB_decode is not supported.");

	GLuint	texture		= 0x1234;
	GLfloat	textureMode	= -1.0f;

	ctx.glGenTextures(1, &texture);
	ctx.glBindTexture(GL_TEXTURE_2D, texture);

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is GL_TEXTURE_SRGB_DECODE_EXT and the value of param(s) is not a valid value (one of DECODE_EXT, or SKIP_DECODE_EXT).");
	ctx.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

void srgb_decode_texparameteriv (NegativeTestContext& ctx)
{
	if (!ctx.isExtensionSupported("GL_EXT_texture_sRGB_decode"))
		TCU_THROW(NotSupportedError, "GL_EXT_texture_sRGB_decode is not supported.");

	GLint	params[1]	= {GL_LINEAR};
	GLuint	texture		= 0x1234;

	ctx.glGenTextures(1, &texture);
	ctx.glBindTexture(GL_TEXTURE_2D, texture);

	params[0] = -1;
	ctx.beginSection("GL_INVALID_ENUM is generated if pname is GL_TEXTURE_SRGB_DECODE_EXT and the value of param(s) is not a valid value (one of DECODE_EXT, or SKIP_DECODE_EXT).");
	ctx.glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

void srgb_decode_texparameterfv (NegativeTestContext& ctx)
{
	if (!ctx.isExtensionSupported("GL_EXT_texture_sRGB_decode"))
		TCU_THROW(NotSupportedError, "GL_EXT_texture_sRGB_decode is not supported.");

	GLfloat	params[1]	= {GL_LINEAR};
	GLuint	texture		= 0x1234;

	ctx.glGenTextures(1, &texture);
	ctx.glBindTexture(GL_TEXTURE_2D, texture);

	params[0] = -1.0f;
	ctx.beginSection("GL_INVALID_ENUM is generated if pname is GL_TEXTURE_SRGB_DECODE_EXT and the value of param(s) is not a valid value (one of DECODE_EXT, or SKIP_DECODE_EXT).");
	ctx.glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

void srgb_decode_texparameterIiv (NegativeTestContext& ctx)
{
	if (!contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		TCU_THROW(NotSupportedError,"glTexParameterIiv is not supported.");

	if (!ctx.isExtensionSupported("GL_EXT_texture_sRGB_decode"))
		TCU_THROW(NotSupportedError, "GL_EXT_texture_sRGB_decode is not supported.");

	GLint	textureMode[]	= {GL_DEPTH_COMPONENT, GL_STENCIL_INDEX};
	GLuint	texture			= 0x1234;

	ctx.glGenTextures(1, &texture);
	ctx.glBindTexture(GL_TEXTURE_2D, texture);

	textureMode[0] = -1;
	textureMode[1] = -1;
	ctx.beginSection("GL_INVALID_ENUM is generated if pname is GL_TEXTURE_SRGB_DECODE_EXT and the value of param(s) is not a valid value (one of DECODE_EXT, or SKIP_DECODE_EXT).");
	ctx.glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

void srgb_decode_texparameterIuiv (NegativeTestContext& ctx)
{
	if (!contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		TCU_THROW(NotSupportedError,"glTexParameterIuiv is not supported.");

	if (!ctx.isExtensionSupported("GL_EXT_texture_sRGB_decode"))
		TCU_THROW(NotSupportedError, "GL_EXT_texture_sRGB_decode is not supported.");

	GLuint	textureMode[]	= {GL_DEPTH_COMPONENT, GL_STENCIL_INDEX};
	GLuint	texture			= 0x1234;

	ctx.glGenTextures(1, &texture);
	ctx.glBindTexture(GL_TEXTURE_2D, texture);

	textureMode[0] = GL_DONT_CARE;
	textureMode[1] = GL_DONT_CARE;
	ctx.beginSection("GL_INVALID_ENUM is generated if pname is GL_TEXTURE_SRGB_DECODE_EXT and the value of param(s) is not a valid value (one of DECODE_EXT, or SKIP_DECODE_EXT).");
	ctx.glTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, textureMode);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.glDeleteTextures(1, &texture);
}

std::vector<FunctionContainer> getNegativeTextureApiTestFunctions()
{
	FunctionContainer funcs[] =
	{
		{activetexture,									"activetexture",									"Invalid glActiveTexture() usage"		   },
		{bindtexture,									"bindtexture",										"Invalid glBindTexture() usage"			   },
		{compressedteximage2d_invalid_target,			"compressedteximage2d_invalid_target",				"Invalid glCompressedTexImage2D() usage"   },
		{compressedteximage2d_invalid_format,			"compressedteximage2d_invalid_format",				"Invalid glCompressedTexImage2D() usage"   },
		{compressedteximage2d_neg_level,				"compressedteximage2d_neg_level",					"Invalid glCompressedTexImage2D() usage"   },
		{compressedteximage2d_max_level,				"compressedteximage2d_max_level",					"Invalid glCompressedTexImage2D() usage"   },
		{compressedteximage2d_neg_width_height,			"compressedteximage2d_neg_width_height",			"Invalid glCompressedTexImage2D() usage"   },
		{compressedteximage2d_max_width_height,			"compressedteximage2d_max_width_height",			"Invalid glCompressedTexImage2D() usage"   },
		{compressedteximage2d_invalid_border,			"compressedteximage2d_invalid_border",				"Invalid glCompressedTexImage2D() usage"   },
		{compressedteximage2d_invalid_size,				"compressedteximage2d_invalid_size",				"Invalid glCompressedTexImage2D() usage"   },
		{compressedteximage2d_neg_size,					"compressedteximage2d_neg_size",					"Invalid glCompressedTexImage2D() usage"   },
		{compressedteximage2d_invalid_width_height,		"compressedteximage2d_invalid_width_height",		"Invalid glCompressedTexImage2D() usage"   },
		{compressedteximage2d_invalid_buffer_target,	"compressedteximage2d_invalid_buffer_target",		"Invalid glCompressedTexImage2D() usage"   },
		{copyteximage2d_invalid_target,					"copyteximage2d_invalid_target",					"Invalid glCopyTexImage2D() usage"		   },
		{copyteximage2d_invalid_format,					"copyteximage2d_invalid_format",					"Invalid glCopyTexImage2D() usage"		   },
		{copyteximage2d_inequal_width_height_cube,		"copyteximage2d_inequal_width_height_cube",			"Invalid glCopyTexImage2D() usage"		   },
		{copyteximage2d_neg_level,						"copyteximage2d_neg_level",							"Invalid glCopyTexImage2D() usage"		   },
		{copyteximage2d_max_level,						"copyteximage2d_max_level",							"Invalid glCopyTexImage2D() usage"		   },
		{copyteximage2d_neg_width_height,				"copyteximage2d_neg_width_height",					"Invalid glCopyTexImage2D() usage"		   },
		{copyteximage2d_max_width_height,				"copyteximage2d_max_width_height",					"Invalid glCopyTexImage2D() usage"		   },
		{copyteximage2d_invalid_border,					"copyteximage2d_invalid_border",					"Invalid glCopyTexImage2D() usage"		   },
		{copyteximage2d_incomplete_framebuffer,			"copyteximage2d_incomplete_framebuffer",			"Invalid glCopyTexImage2D() usage"		   },
		{copytexsubimage2d_invalid_target,				"copytexsubimage2d_invalid_target",					"Invalid glCopyTexSubImage2D() usage"	   },
		{copytexsubimage2d_read_buffer_is_none,			"copytexsubimage2d_read_buffer_is_none",			"Invalid glCopyTexSubImage2D() usage"	   },
		{copytexsubimage2d_texture_internalformat,		"copytexsubimage2d_texture_internalformat",			"Invalid glCopyTexSubImage2D() usage"	   },
		{copytexsubimage2d_neg_level,					"copytexsubimage2d_neg_level",						"Invalid glCopyTexSubImage2D() usage"	   },
		{copytexsubimage2d_max_level,					"copytexsubimage2d_max_level",						"Invalid glCopyTexSubImage2D() usage"	   },
		{copytexsubimage2d_neg_offset,					"copytexsubimage2d_neg_offset",						"Invalid glCopyTexSubImage2D() usage"	   },
		{copytexsubimage2d_invalid_offset,				"copytexsubimage2d_invalid_offset",					"Invalid glCopyTexSubImage2D() usage"	   },
		{copytexsubimage2d_neg_width_height,			"copytexsubimage2d_neg_width_height",				"Invalid glCopyTexSubImage2D() usage"	   },
		{copytexsubimage2d_incomplete_framebuffer,		"copytexsubimage2d_incomplete_framebuffer",			"Invalid glCopyTexSubImage2D() usage"	   },
		{deletetextures,								"deletetextures",									"Invalid glDeleteTextures() usage"		   },
		{generatemipmap,								"generatemipmap",									"Invalid glGenerateMipmap() usage"		   },
		{gentextures,									"gentextures",										"Invalid glGenTextures() usage"			   },
		{pixelstorei,									"pixelstorei",										"Invalid glPixelStorei() usage"			   },
		{teximage2d,									"teximage2d",										"Invalid glTexImage2D() usage"			   },
		{teximage2d_inequal_width_height_cube,			"teximage2d_inequal_width_height_cube",				"Invalid glTexImage2D() usage"			   },
		{teximage2d_neg_level,							"teximage2d_neg_level",								"Invalid glTexImage2D() usage"			   },
		{teximage2d_max_level,							"teximage2d_max_level",								"Invalid glTexImage2D() usage"			   },
		{teximage2d_neg_width_height,					"teximage2d_neg_width_height",						"Invalid glTexImage2D() usage"			   },
		{teximage2d_max_width_height,					"teximage2d_max_width_height",						"Invalid glTexImage2D() usage"			   },
		{teximage2d_invalid_border,						"teximage2d_invalid_border",						"Invalid glTexImage2D() usage"			   },
		{teximage2d_invalid_buffer_target,				"teximage2d_invalid_buffer_target",					"Invalid glTexImage2D() usage"			   },
		{texsubimage2d,									"texsubimage2d",									"Invalid glTexSubImage2D() usage"		   },
		{texsubimage2d_neg_level,						"texsubimage2d_neg_level",							"Invalid glTexSubImage2D() usage"		   },
		{texsubimage2d_max_level,						"texsubimage2d_max_level",							"Invalid glTexSubImage2D() usage"		   },
		{texsubimage2d_neg_offset,						"texsubimage2d_neg_offset",							"Invalid glTexSubImage2D() usage"		   },
		{texsubimage2d_invalid_offset,					"texsubimage2d_invalid_offset",						"Invalid glTexSubImage2D() usage"		   },
		{texsubimage2d_neg_width_height,				"texsubimage2d_neg_width_height",					"Invalid glTexSubImage2D() usage"		   },
		{texsubimage2d_invalid_buffer_target,			"texsubimage2d_invalid_buffer_target",				"Invalid glTexSubImage2D() usage"		   },
		{texparameteri,									"texparameteri",									"Invalid glTexParameteri() usage"		   },
		{texparameterf,									"texparameterf",									"Invalid glTexParameterf() usage"		   },
		{texparameteriv,								"texparameteriv",									"Invalid glTexParameteriv() usage"		   },
		{texparameterfv,								"texparameterfv",									"Invalid glTexParameterfv() usage"		   },
		{texparameterIiv,								"texparameterIiv",									"Invalid glTexParameterIiv() usage"		   },
		{texparameterIuiv,								"texparameterIuiv",									"Invalid glTexParameterIuiv() usage"	   },
		{compressedtexsubimage2d,						"compressedtexsubimage2d",							"Invalid glCompressedTexSubImage2D() usage"},
		{compressedtexsubimage2d_neg_level,				"compressedtexsubimage2d_neg_level",				"Invalid glCompressedTexSubImage2D() usage"},
		{compressedtexsubimage2d_max_level,				"compressedtexsubimage2d_max_level",				"Invalid glCompressedTexSubImage2D() usage"},
		{compressedtexsubimage2d_neg_offset,			"compressedtexsubimage2d_neg_offset",				"Invalid glCompressedTexSubImage2D() usage"},
		{compressedtexsubimage2d_invalid_offset,		"compressedtexsubimage2d_invalid_offset",			"Invalid glCompressedTexSubImage2D() usage"},
		{compressedtexsubimage2d_neg_width_height,		"compressedtexsubimage2d_neg_width_height",			"Invalid glCompressedTexSubImage2D() usage"},
		{compressedtexsubimage2d_invalid_size,			"compressedtexsubimage2d_invalid_size",				"Invalid glCompressedTexImage2D() usage"   },
		{compressedtexsubimage2d_invalid_buffer_target,	"compressedtexsubimage2d_invalid_buffer_target",	"Invalid glCompressedTexSubImage2D() usage"},
		{teximage3d,									"teximage3d",										"Invalid glTexImage3D() usage"			   },
		{teximage3d_neg_level,							"teximage3d_neg_level",								"Invalid glTexImage3D() usage"			   },
		{teximage3d_max_level,							"teximage3d_max_level",								"Invalid glTexImage3D() usage"			   },
		{teximage3d_neg_width_height_depth,				"teximage3d_neg_width_height_depth",				"Invalid glTexImage3D() usage"			   },
		{teximage3d_max_width_height_depth,				"teximage3d_max_width_height_depth",				"Invalid glTexImage3D() usage"			   },
		{teximage3d_invalid_border,						"teximage3d_invalid_border",						"Invalid glTexImage3D() usage"			   },
		{teximage3d_invalid_buffer_target,				"teximage3d_invalid_buffer_target",					"Invalid glTexImage3D() usage"			   },
		{texsubimage3d,									"texsubimage3d",									"Invalid glTexSubImage3D() usage"		   },
		{texsubimage3d_neg_level,						"texsubimage3d_neg_level",							"Invalid glTexSubImage3D() usage"		   },
		{texsubimage3d_max_level,						"texsubimage3d_max_level",							"Invalid glTexSubImage3D() usage"		   },
		{texsubimage3d_neg_offset,						"texsubimage3d_neg_offset",							"Invalid glTexSubImage3D() usage"		   },
		{texsubimage3d_invalid_offset,					"texsubimage3d_invalid_offset",						"Invalid glTexSubImage3D() usage"		   },
		{texsubimage3d_neg_width_height,				"texsubimage3d_neg_width_height",					"Invalid glTexSubImage3D() usage"		   },
		{texsubimage3d_invalid_buffer_target,			"texsubimage3d_invalid_buffer_target",				"Invalid glTexSubImage3D() usage"		   },
		{copytexsubimage3d,								"copytexsubimage3d",								"Invalid glCopyTexSubImage3D() usage"	   },
		{copytexsubimage3d_neg_level,					"copytexsubimage3d_neg_level",						"Invalid glCopyTexSubImage3D() usage"	   },
		{copytexsubimage3d_max_level,					"copytexsubimage3d_max_level",						"Invalid glCopyTexSubImage3D() usage"	   },
		{copytexsubimage3d_neg_offset,					"copytexsubimage3d_neg_offset",						"Invalid glCopyTexSubImage3D() usage"	   },
		{copytexsubimage3d_invalid_offset,				"copytexsubimage3d_invalid_offset",					"Invalid glCopyTexSubImage3D() usage"	   },
		{copytexsubimage3d_neg_width_height,			"copytexsubimage3d_neg_width_height",				"Invalid glCopyTexSubImage3D() usage"	   },
		{copytexsubimage3d_incomplete_framebuffer,		"copytexsubimage3d_incomplete_framebuffer",			"Invalid glCopyTexSubImage3D() usage"	   },
		{compressedteximage3d,							"compressedteximage3d",								"Invalid glCompressedTexImage3D() usage"   },
		{compressedteximage3d_neg_level,				"compressedteximage3d_neg_level",					"Invalid glCompressedTexImage3D() usage"   },
		{compressedteximage3d_max_level,				"compressedteximage3d_max_level",					"Invalid glCompressedTexImage3D() usage"   },
		{compressedteximage3d_neg_width_height_depth,	"compressedteximage3d_neg_width_height_depth",		"Invalid glCompressedTexImage3D() usage"   },
		{compressedteximage3d_max_width_height_depth,	"compressedteximage3d_max_width_height_depth",		"Invalid glCompressedTexImage3D() usage"   },
		{compressedteximage3d_invalid_border,			"compressedteximage3d_invalid_border",				"Invalid glCompressedTexImage3D() usage"   },
		{compressedteximage3d_invalid_size,				"compressedteximage3d_invalid_size",				"Invalid glCompressedTexImage3D() usage"   },
		{compressedteximage3d_invalid_width_height,		"compressedteximage3d_invalid_width_height",		"Invalid glCompressedTexImage3D() usage"   },
		{compressedteximage3d_invalid_buffer_target,	"compressedteximage3d_invalid_buffer_target",		"Invalid glCompressedTexImage3D() usage"   },
		{compressedtexsubimage3d,						"compressedtexsubimage3d",							"Invalid glCompressedTexSubImage3D() usage"},
		{compressedtexsubimage3d_neg_level,				"compressedtexsubimage3d_neg_level",				"Invalid glCompressedTexSubImage3D() usage"},
		{compressedtexsubimage3d_max_level,				"compressedtexsubimage3d_max_level",				"Invalid glCompressedTexSubImage3D() usage"},
		{compressedtexsubimage3d_neg_offset,			"compressedtexsubimage3d_neg_offset",				"Invalid glCompressedTexSubImage3D() usage"},
		{compressedtexsubimage3d_invalid_offset,		"compressedtexsubimage3d_invalid_offset",			"Invalid glCompressedTexSubImage3D() usage"},
		{compressedtexsubimage3d_neg_width_height_depth,"compressedtexsubimage3d_neg_width_height_depth",	"Invalid glCompressedTexSubImage3D() usage"},
		{compressedtexsubimage3d_invalid_size,			"compressedtexsubimage3d_invalid_size",				"Invalid glCompressedTexSubImage3D() usage"},
		{compressedtexsubimage3d_invalid_buffer_target,	"compressedtexsubimage3d_invalid_buffer_target",	"Invalid glCompressedTexSubImage3D() usage"},
		{texstorage2d,									"texstorage2d",										"Invalid glTexStorage2D() usage"		   },
		{texstorage2d_invalid_binding,					"texstorage2d_invalid_binding",						"Invalid glTexStorage2D() usage"		   },
		{texstorage2d_invalid_levels,					"texstorage2d_invalid_levels",						"Invalid glTexStorage2D() usage"		   },
		{texstorage3d,									"texstorage3d",										"Invalid glTexStorage3D() usage"		   },
		{texstorage3d_invalid_binding,					"texstorage3d_invalid_binding",						"Invalid glTexStorage3D() usage"		   },
		{texstorage3d_invalid_levels,					"texstorage3d_invalid_levels",						"Invalid glTexStorage3D() usage"		   },
		{srgb_decode_texparameteri,						"srgb_decode_texparameteri",						"Invalid texparameteri() usage srgb"	   },
		{srgb_decode_texparameterf,						"srgb_decode_texparameterf",						"Invalid texparameterf() usage srgb"	   },
		{srgb_decode_texparameteriv,					"srgb_decode_texparameteriv",						"Invalid texparameteriv() usage srgb"	   },
		{srgb_decode_texparameterfv,					"srgb_decode_texparameterfv",						"Invalid texparameterfv() usage srgb"	   },
		{srgb_decode_texparameterIiv,					"srgb_decode_texparameterIiv",						"Invalid texparameterIiv() usage srgb"	   },
		{srgb_decode_texparameterIuiv,					"srgb_decode_texparameterIuiv",						"Invalid texparameterIuiv() usage srgb"	   },
	};

	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

} // NegativeTestShared
} // Functional
} // gles31
} // deqp
