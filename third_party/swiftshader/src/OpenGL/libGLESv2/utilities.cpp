// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// utilities.cpp: Conversion functions and other utility routines.

#include "utilities.h"

#include "Framebuffer.h"
#include "main.h"
#include "mathutil.h"
#include "Context.h"
#include "Shader.h"
#include "common/debug.h"

#include <limits>
#include <stdio.h>
#include <stdlib.h>

namespace es2
{

	unsigned int UniformComponentCount(GLenum type)
	{
		switch(type)
		{
		case GL_BOOL:
		case GL_FLOAT:
		case GL_INT:
		case GL_UNSIGNED_INT:
		case GL_SAMPLER_2D:
		case GL_SAMPLER_CUBE:
		case GL_SAMPLER_2D_RECT_ARB:
		case GL_SAMPLER_EXTERNAL_OES:
		case GL_SAMPLER_3D_OES:
		case GL_SAMPLER_2D_ARRAY:
		case GL_SAMPLER_2D_SHADOW:
		case GL_SAMPLER_CUBE_SHADOW:
		case GL_SAMPLER_2D_ARRAY_SHADOW:
		case GL_INT_SAMPLER_2D:
		case GL_UNSIGNED_INT_SAMPLER_2D:
		case GL_INT_SAMPLER_CUBE:
		case GL_UNSIGNED_INT_SAMPLER_CUBE:
		case GL_INT_SAMPLER_3D:
		case GL_UNSIGNED_INT_SAMPLER_3D:
		case GL_INT_SAMPLER_2D_ARRAY:
		case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
			return 1;
		case GL_BOOL_VEC2:
		case GL_FLOAT_VEC2:
		case GL_INT_VEC2:
		case GL_UNSIGNED_INT_VEC2:
			return 2;
		case GL_INT_VEC3:
		case GL_UNSIGNED_INT_VEC3:
		case GL_FLOAT_VEC3:
		case GL_BOOL_VEC3:
			return 3;
		case GL_BOOL_VEC4:
		case GL_FLOAT_VEC4:
		case GL_INT_VEC4:
		case GL_UNSIGNED_INT_VEC4:
		case GL_FLOAT_MAT2:
			return 4;
		case GL_FLOAT_MAT2x3:
		case GL_FLOAT_MAT3x2:
			return 6;
		case GL_FLOAT_MAT2x4:
		case GL_FLOAT_MAT4x2:
			return 8;
		case GL_FLOAT_MAT3:
			return 9;
		case GL_FLOAT_MAT3x4:
		case GL_FLOAT_MAT4x3:
			return 12;
		case GL_FLOAT_MAT4:
			return 16;
		default:
			UNREACHABLE(type);
		}

		return 0;
	}

	GLenum UniformComponentType(GLenum type)
	{
		switch(type)
		{
		case GL_BOOL:
		case GL_BOOL_VEC2:
		case GL_BOOL_VEC3:
		case GL_BOOL_VEC4:
			return GL_BOOL;
		case GL_FLOAT:
		case GL_FLOAT_VEC2:
		case GL_FLOAT_VEC3:
		case GL_FLOAT_VEC4:
		case GL_FLOAT_MAT2:
		case GL_FLOAT_MAT2x3:
		case GL_FLOAT_MAT2x4:
		case GL_FLOAT_MAT3:
		case GL_FLOAT_MAT3x2:
		case GL_FLOAT_MAT3x4:
		case GL_FLOAT_MAT4:
		case GL_FLOAT_MAT4x2:
		case GL_FLOAT_MAT4x3:
			return GL_FLOAT;
		case GL_INT:
		case GL_SAMPLER_2D:
		case GL_SAMPLER_CUBE:
		case GL_SAMPLER_2D_RECT_ARB:
		case GL_SAMPLER_EXTERNAL_OES:
		case GL_SAMPLER_3D_OES:
		case GL_SAMPLER_2D_ARRAY:
		case GL_SAMPLER_2D_SHADOW:
		case GL_SAMPLER_CUBE_SHADOW:
		case GL_SAMPLER_2D_ARRAY_SHADOW:
		case GL_INT_SAMPLER_2D:
		case GL_UNSIGNED_INT_SAMPLER_2D:
		case GL_INT_SAMPLER_CUBE:
		case GL_UNSIGNED_INT_SAMPLER_CUBE:
		case GL_INT_SAMPLER_3D:
		case GL_UNSIGNED_INT_SAMPLER_3D:
		case GL_INT_SAMPLER_2D_ARRAY:
		case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
		case GL_INT_VEC2:
		case GL_INT_VEC3:
		case GL_INT_VEC4:
			return GL_INT;
		case GL_UNSIGNED_INT:
		case GL_UNSIGNED_INT_VEC2:
		case GL_UNSIGNED_INT_VEC3:
		case GL_UNSIGNED_INT_VEC4:
			return GL_UNSIGNED_INT;
		default:
			UNREACHABLE(type);
		}

		return GL_NONE;
	}

	size_t UniformTypeSize(GLenum type)
	{
		switch(type)
		{
		case GL_BOOL:  return sizeof(GLboolean);
		case GL_FLOAT: return sizeof(GLfloat);
		case GL_INT:   return sizeof(GLint);
		case GL_UNSIGNED_INT: return sizeof(GLuint);
		}

		return UniformTypeSize(UniformComponentType(type)) * UniformComponentCount(type);
	}

	bool IsSamplerUniform(GLenum type)
	{
		switch(type)
		{
		case GL_SAMPLER_2D:
		case GL_SAMPLER_CUBE:
		case GL_SAMPLER_2D_RECT_ARB:
		case GL_SAMPLER_EXTERNAL_OES:
		case GL_SAMPLER_3D_OES:
		case GL_SAMPLER_2D_ARRAY:
		case GL_SAMPLER_2D_SHADOW:
		case GL_SAMPLER_CUBE_SHADOW:
		case GL_SAMPLER_2D_ARRAY_SHADOW:
		case GL_INT_SAMPLER_2D:
		case GL_UNSIGNED_INT_SAMPLER_2D:
		case GL_INT_SAMPLER_CUBE:
		case GL_UNSIGNED_INT_SAMPLER_CUBE:
		case GL_INT_SAMPLER_3D:
		case GL_UNSIGNED_INT_SAMPLER_3D:
		case GL_INT_SAMPLER_2D_ARRAY:
		case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
			return true;
		default:
			return false;
		}
	}

	int VariableRowCount(GLenum type)
	{
		switch(type)
		{
		case GL_NONE:
			return 0;
		case GL_BOOL:
		case GL_FLOAT:
		case GL_INT:
		case GL_UNSIGNED_INT:
		case GL_BOOL_VEC2:
		case GL_FLOAT_VEC2:
		case GL_INT_VEC2:
		case GL_UNSIGNED_INT_VEC2:
		case GL_INT_VEC3:
		case GL_UNSIGNED_INT_VEC3:
		case GL_FLOAT_VEC3:
		case GL_BOOL_VEC3:
		case GL_BOOL_VEC4:
		case GL_FLOAT_VEC4:
		case GL_INT_VEC4:
		case GL_UNSIGNED_INT_VEC4:
		case GL_SAMPLER_2D:
		case GL_SAMPLER_CUBE:
		case GL_SAMPLER_2D_RECT_ARB:
		case GL_SAMPLER_EXTERNAL_OES:
		case GL_SAMPLER_3D_OES:
		case GL_SAMPLER_2D_ARRAY:
		case GL_SAMPLER_2D_SHADOW:
		case GL_SAMPLER_CUBE_SHADOW:
		case GL_SAMPLER_2D_ARRAY_SHADOW:
		case GL_INT_SAMPLER_2D:
		case GL_UNSIGNED_INT_SAMPLER_2D:
		case GL_INT_SAMPLER_CUBE:
		case GL_UNSIGNED_INT_SAMPLER_CUBE:
		case GL_INT_SAMPLER_3D:
		case GL_UNSIGNED_INT_SAMPLER_3D:
		case GL_INT_SAMPLER_2D_ARRAY:
		case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
			return 1;
		case GL_FLOAT_MAT2:
		case GL_FLOAT_MAT3x2:
		case GL_FLOAT_MAT4x2:
			return 2;
		case GL_FLOAT_MAT3:
		case GL_FLOAT_MAT2x3:
		case GL_FLOAT_MAT4x3:
			return 3;
		case GL_FLOAT_MAT4:
		case GL_FLOAT_MAT2x4:
		case GL_FLOAT_MAT3x4:
			return 4;
		default:
			UNREACHABLE(type);
		}

		return 0;
	}

	int VariableColumnCount(GLenum type)
	{
		switch(type)
		{
		case GL_NONE:
			return 0;
		case GL_BOOL:
		case GL_FLOAT:
		case GL_INT:
		case GL_UNSIGNED_INT:
			return 1;
		case GL_BOOL_VEC2:
		case GL_FLOAT_VEC2:
		case GL_INT_VEC2:
		case GL_UNSIGNED_INT_VEC2:
		case GL_FLOAT_MAT2:
		case GL_FLOAT_MAT2x3:
		case GL_FLOAT_MAT2x4:
			return 2;
		case GL_INT_VEC3:
		case GL_UNSIGNED_INT_VEC3:
		case GL_FLOAT_VEC3:
		case GL_BOOL_VEC3:
		case GL_FLOAT_MAT3:
		case GL_FLOAT_MAT3x2:
		case GL_FLOAT_MAT3x4:
			return 3;
		case GL_BOOL_VEC4:
		case GL_FLOAT_VEC4:
		case GL_INT_VEC4:
		case GL_UNSIGNED_INT_VEC4:
		case GL_FLOAT_MAT4:
		case GL_FLOAT_MAT4x2:
		case GL_FLOAT_MAT4x3:
			return 4;
		default:
			UNREACHABLE(type);
		}

		return 0;
	}

	int VariableRegisterCount(GLenum type)
	{
		// Number of registers used is the number of columns for matrices or 1 for scalars and vectors
		return (VariableRowCount(type) > 1) ? VariableColumnCount(type) : 1;
	}

	int VariableRegisterSize(GLenum type)
	{
		// Number of components per register is the number of rows for matrices or columns for scalars and vectors
		int nbRows = VariableRowCount(type);
		return (nbRows > 1) ? nbRows : VariableColumnCount(type);
	}

	int AllocateFirstFreeBits(unsigned int *bits, unsigned int allocationSize, unsigned int bitsSize)
	{
		ASSERT(allocationSize <= bitsSize);

		unsigned int mask = std::numeric_limits<unsigned int>::max() >> (std::numeric_limits<unsigned int>::digits - allocationSize);

		for(unsigned int i = 0; i < bitsSize - allocationSize + 1; i++)
		{
			if((*bits & mask) == 0)
			{
				*bits |= mask;
				return i;
			}

			mask <<= 1;
		}

		return -1;
	}

	bool IsCompressed(GLint internalformat, GLint clientVersion)
	{
		switch(internalformat)
		{
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
		case GL_ETC1_RGB8_OES:
			return true;
		case GL_COMPRESSED_R11_EAC:
		case GL_COMPRESSED_SIGNED_R11_EAC:
		case GL_COMPRESSED_RG11_EAC:
		case GL_COMPRESSED_SIGNED_RG11_EAC:
		case GL_COMPRESSED_RGB8_ETC2:
		case GL_COMPRESSED_SRGB8_ETC2:
		case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
		case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
		case GL_COMPRESSED_RGBA8_ETC2_EAC:
		case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
			return (clientVersion >= 3);
		case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
		case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
		case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
		case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
		case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
		case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
		case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
		case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
		case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:
		case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:
		case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:
		case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
		case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
		case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:
			return ASTC_SUPPORT && (clientVersion >= 3);
		default:
			return false;
		}
	}

	bool IsSizedInternalFormat(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_ALPHA8_EXT:
		case GL_LUMINANCE8_EXT:
		case GL_LUMINANCE8_ALPHA8_EXT:
		case GL_ALPHA32F_EXT:
		case GL_LUMINANCE32F_EXT:
		case GL_LUMINANCE_ALPHA32F_EXT:
		case GL_ALPHA16F_EXT:
		case GL_LUMINANCE16F_EXT:
		case GL_LUMINANCE_ALPHA16F_EXT:
		case GL_R8:
		case GL_R8UI:
		case GL_R8I:
		case GL_R16UI:
		case GL_R16I:
		case GL_R32UI:
		case GL_R32I:
		case GL_RG8:
		case GL_RG8UI:
		case GL_RG8I:
		case GL_RG16UI:
		case GL_RG16I:
		case GL_RG32UI:
		case GL_RG32I:
		case GL_SRGB8_ALPHA8:
		case GL_RGB8UI:
		case GL_RGB8I:
		case GL_RGB16UI:
		case GL_RGB16I:
		case GL_RGB32UI:
		case GL_RGB32I:
		case GL_RG8_SNORM:
		case GL_R8_SNORM:
		case GL_RGB10_A2:
		case GL_RGBA8UI:
		case GL_RGBA8I:
		case GL_RGB10_A2UI:
		case GL_RGBA16UI:
		case GL_RGBA16I:
		case GL_RGBA32I:
		case GL_RGBA32UI:
		case GL_RGBA4:
		case GL_RGB5_A1:
		case GL_RGB565:
		case GL_RGB8:
		case GL_RGBA8:
		case GL_BGRA8_EXT:   // GL_APPLE_texture_format_BGRA8888
		case GL_R16F:
		case GL_RG16F:
		case GL_R11F_G11F_B10F:
		case GL_RGB16F:
		case GL_RGBA16F:
		case GL_R32F:
		case GL_RG32F:
		case GL_RGB32F:
		case GL_RGBA32F:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32_OES:
		case GL_DEPTH_COMPONENT32F:
		case GL_DEPTH32F_STENCIL8:
		case GL_DEPTH_COMPONENT16:
		case GL_STENCIL_INDEX8:
		case GL_DEPTH24_STENCIL8_OES:
		case GL_RGBA8_SNORM:
		case GL_SRGB8:
		case GL_RGB8_SNORM:
		case GL_RGB9_E5:
			return true;
		default:
			return false;
		}
	}

	GLenum ValidateSubImageParams(bool compressed, bool copy, GLenum target, GLint level, GLint xoffset, GLint yoffset,
	                              GLsizei width, GLsizei height, GLenum format, GLenum type, Texture *texture, GLint clientVersion)
	{
		if(!texture)
		{
			return GL_INVALID_OPERATION;
		}

		GLenum sizedInternalFormat = texture->getFormat(target, level);

		if(compressed)
		{
			if(format != sizedInternalFormat)
			{
				return GL_INVALID_OPERATION;
			}
		}
		else if(!copy)   // CopyTexSubImage doesn't have format/type parameters.
		{
			GLenum validationError = ValidateTextureFormatType(format, type, sizedInternalFormat, target, clientVersion);
			if(validationError != GL_NO_ERROR)
			{
				return validationError;
			}
		}

		if(compressed)
		{
			if((width % 4 != 0 && width != texture->getWidth(target, 0)) ||
			   (height % 4 != 0 && height != texture->getHeight(target, 0)))
			{
				return GL_INVALID_OPERATION;
			}
		}

		if(xoffset + width > texture->getWidth(target, level) ||
		   yoffset + height > texture->getHeight(target, level))
		{
			return GL_INVALID_VALUE;
		}

		return GL_NO_ERROR;
	}

	GLenum ValidateSubImageParams(bool compressed, bool copy, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
	                              GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, Texture *texture, GLint clientVersion)
	{
		if(!texture)
		{
			return GL_INVALID_OPERATION;
		}

		if(compressed != texture->isCompressed(target, level))
		{
			return GL_INVALID_OPERATION;
		}

		if(!copy)
		{
			GLenum sizedInternalFormat = texture->getFormat(target, level);

			GLenum validationError = ValidateTextureFormatType(format, type, sizedInternalFormat, target, clientVersion);
			if(validationError != GL_NO_ERROR)
			{
				return validationError;
			}
		}

		if(compressed)
		{
			if((width % 4 != 0 && width != texture->getWidth(target, 0)) ||
			   (height % 4 != 0 && height != texture->getHeight(target, 0)) ||
			   (depth % 4 != 0 && depth != texture->getDepth(target, 0)))
			{
				return GL_INVALID_OPERATION;
			}
		}

		if(xoffset + width > texture->getWidth(target, level) ||
		   yoffset + height > texture->getHeight(target, level) ||
		   zoffset + depth > texture->getDepth(target, level))
		{
			return GL_INVALID_VALUE;
		}

		return GL_NO_ERROR;
	}

	bool ValidateCopyFormats(GLenum textureFormat, GLenum colorbufferFormat)
	{
		ASSERT(!gl::IsUnsizedInternalFormat(textureFormat));
		ASSERT(!gl::IsUnsizedInternalFormat(colorbufferFormat));

		if(GetColorComponentType(textureFormat) == GL_NONE)
		{
			return error(GL_INVALID_ENUM, false);
		}

		if(GetColorComponentType(colorbufferFormat) != GetColorComponentType(textureFormat))
		{
			return error(GL_INVALID_OPERATION, false);
		}

		if(GetColorEncoding(colorbufferFormat) != GetColorEncoding(textureFormat))
		{
			return error(GL_INVALID_OPERATION, false);
		}

		GLenum baseTexureFormat = gl::GetBaseInternalFormat(textureFormat);
		GLenum baseColorbufferFormat = gl::GetBaseInternalFormat(colorbufferFormat);

		// [OpenGL ES 2.0.24] table 3.9
		// [OpenGL ES 3.0.5] table 3.16
		switch(baseTexureFormat)
		{
		case GL_ALPHA:
			if(baseColorbufferFormat != GL_ALPHA &&
			   baseColorbufferFormat != GL_RGBA &&
			   baseColorbufferFormat != GL_BGRA_EXT)   // GL_EXT_texture_format_BGRA8888 / GL_APPLE_texture_format_BGRA8888
			{
				return error(GL_INVALID_OPERATION, false);
			}
			break;
		case GL_LUMINANCE_ALPHA:
		case GL_RGBA:
			if(baseColorbufferFormat != GL_RGBA &&
			   baseColorbufferFormat != GL_BGRA_EXT)   // GL_EXT_texture_format_BGRA8888 / GL_APPLE_texture_format_BGRA8888
			{
				return error(GL_INVALID_OPERATION, false);
			}
			break;
		case GL_LUMINANCE:
		case GL_RED:
			if(baseColorbufferFormat != GL_RED &&
			   baseColorbufferFormat != GL_RG &&
			   baseColorbufferFormat != GL_RGB &&
			   baseColorbufferFormat != GL_RGBA &&
			   baseColorbufferFormat != GL_BGRA_EXT)   // GL_EXT_texture_format_BGRA8888 / GL_APPLE_texture_format_BGRA8888
			{
				return error(GL_INVALID_OPERATION, false);
			}
			break;
		case GL_RG:
			if(baseColorbufferFormat != GL_RG &&
			   baseColorbufferFormat != GL_RGB &&
			   baseColorbufferFormat != GL_RGBA &&
			   baseColorbufferFormat != GL_BGRA_EXT)   // GL_EXT_texture_format_BGRA8888 / GL_APPLE_texture_format_BGRA8888
			{
				return error(GL_INVALID_OPERATION, false);
			}
			break;
		case GL_RGB:
			if(baseColorbufferFormat != GL_RGB &&
			   baseColorbufferFormat != GL_RGBA &&
			   baseColorbufferFormat != GL_BGRA_EXT)   // GL_EXT_texture_format_BGRA8888 / GL_APPLE_texture_format_BGRA8888
			{
				return error(GL_INVALID_OPERATION, false);
			}
			break;
		case GL_DEPTH_COMPONENT:
		case GL_DEPTH_STENCIL_OES:
			return error(GL_INVALID_OPERATION, false);
		case GL_BGRA_EXT:   // GL_EXT_texture_format_BGRA8888 nor GL_APPLE_texture_format_BGRA8888 mention the format to be accepted by glCopyTexImage2D.
		default:
			return error(GL_INVALID_ENUM, false);
		}

		return true;
	}

	bool IsValidReadPixelsFormatType(const Framebuffer *framebuffer, GLenum format, GLenum type, GLint clientVersion)
	{
		// GL_NV_read_depth
		if(format == GL_DEPTH_COMPONENT)
		{
			Renderbuffer *depthbuffer = framebuffer->getDepthbuffer();

			if(!depthbuffer)
			{
				return false;
			}

			switch(type)
			{
			case GL_UNSIGNED_SHORT:
			case GL_FLOAT:
				return true;
			default:
				UNIMPLEMENTED();
				return false;
			}
		}

		Renderbuffer *colorbuffer = framebuffer->getReadColorbuffer();

		if(!colorbuffer)
		{
			return false;
		}

		GLint internalformat = colorbuffer->getFormat();

		if(IsNormalizedInteger(internalformat))
		{
			// Combination always supported by normalized fixed-point rendering surfaces.
			if(format == GL_RGBA && type == GL_UNSIGNED_BYTE)
			{
				return true;
			}

			// GL_EXT_read_format_bgra combinations.
			if(format == GL_BGRA_EXT)
			{
				if(type == GL_UNSIGNED_BYTE ||
				   type == GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT ||
				   type == GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT)
				{
					return true;
				}
			}
		}
		else if(IsFloatFormat(internalformat))
		{
			// Combination always supported by floating-point rendering surfaces.
			// Supported in OpenGL ES 2.0 due to GL_EXT_color_buffer_half_float.
			if(format == GL_RGBA && type == GL_FLOAT)
			{
				return true;
			}
		}
		else if(IsSignedNonNormalizedInteger(internalformat))
		{
			ASSERT(clientVersion >= 3);

			if(format == GL_RGBA_INTEGER && type == GL_INT)
			{
				return true;
			}
		}
		else if(IsUnsignedNonNormalizedInteger(internalformat))
		{
			ASSERT(clientVersion >= 3);

			if(format == GL_RGBA_INTEGER && type == GL_UNSIGNED_INT)
			{
				return true;
			}
		}
		else UNREACHABLE(internalformat);

		// GL_IMPLEMENTATION_COLOR_READ_FORMAT / GL_IMPLEMENTATION_COLOR_READ_TYPE
		GLenum implementationReadFormat = GL_NONE;
		GLenum implementationReadType = GL_NONE;
		switch(format)
		{
		default:
			implementationReadFormat = framebuffer->getImplementationColorReadFormat();
			implementationReadType = framebuffer->getImplementationColorReadType();
			break;
		case GL_DEPTH_COMPONENT:
			implementationReadFormat = framebuffer->getDepthReadFormat();
			implementationReadType = framebuffer->getDepthReadType();
			break;
		}

		if(format == implementationReadFormat && type == implementationReadType)
		{
			return true;
		}

		// Additional third combination accepted by OpenGL ES 3.0.
		if(internalformat == GL_RGB10_A2)
		{
			ASSERT(clientVersion >= 3);

			if(format == GL_RGBA && type == GL_UNSIGNED_INT_2_10_10_10_REV)
			{
				return true;
			}
		}

		return false;
	}

	bool IsDepthTexture(GLenum format)
	{
		return format == GL_DEPTH_COMPONENT ||
		       format == GL_DEPTH_STENCIL_OES ||
		       format == GL_DEPTH_COMPONENT16 ||
		       format == GL_DEPTH_COMPONENT24 ||
		       format == GL_DEPTH_COMPONENT32_OES ||
		       format == GL_DEPTH_COMPONENT32F ||
		       format == GL_DEPTH24_STENCIL8 ||
		       format == GL_DEPTH32F_STENCIL8;
	}

	bool IsStencilTexture(GLenum format)
	{
		return format == GL_STENCIL_INDEX_OES ||
		       format == GL_DEPTH_STENCIL_OES ||
		       format == GL_DEPTH24_STENCIL8 ||
		       format == GL_DEPTH32F_STENCIL8;
	}

	bool IsCubemapTextureTarget(GLenum target)
	{
		return (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
	}

	int CubeFaceIndex(GLenum cubeFace)
	{
		switch(cubeFace)
		{
		case GL_TEXTURE_CUBE_MAP:
		case GL_TEXTURE_CUBE_MAP_POSITIVE_X: return 0;
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_X: return 1;
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Y: return 2;
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: return 3;
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Z: return 4;
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: return 5;
		default: UNREACHABLE(cubeFace); return 0;
		}
	}

	bool IsTextureTarget(GLenum target)
	{
		return target == GL_TEXTURE_2D || IsCubemapTextureTarget(target) || target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_RECTANGLE_ARB;
	}

	GLenum ValidateTextureFormatType(GLenum format, GLenum type, GLint internalformat, GLenum target, GLint clientVersion)
	{
		switch(type)
		{
		case GL_UNSIGNED_BYTE:
		case GL_UNSIGNED_SHORT_4_4_4_4:
		case GL_UNSIGNED_SHORT_5_5_5_1:
		case GL_UNSIGNED_SHORT_5_6_5:
		case GL_FLOAT:               // GL_OES_texture_float
		case GL_HALF_FLOAT_OES:      // GL_OES_texture_half_float
		case GL_HALF_FLOAT:
		case GL_UNSIGNED_INT_24_8:   // GL_OES_packed_depth_stencil (GL_UNSIGNED_INT_24_8_EXT)
		case GL_UNSIGNED_SHORT:      // GL_OES_depth_texture
		case GL_UNSIGNED_INT:        // GL_OES_depth_texture
			break;
		case GL_BYTE:
		case GL_SHORT:
		case GL_INT:
		case GL_UNSIGNED_INT_2_10_10_10_REV:
		case GL_UNSIGNED_INT_10F_11F_11F_REV:
		case GL_UNSIGNED_INT_5_9_9_9_REV:
		case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
			if(clientVersion < 3)
			{
				return GL_INVALID_ENUM;
			}
			break;
		default:
			return GL_INVALID_ENUM;
		}

		switch(format)
		{
		case GL_ALPHA:
		case GL_RGB:
		case GL_RGBA:
		case GL_LUMINANCE:
		case GL_LUMINANCE_ALPHA:
		case GL_BGRA_EXT:          // GL_EXT_texture_format_BGRA8888
		case GL_RED_EXT:           // GL_EXT_texture_rg
		case GL_RG_EXT:            // GL_EXT_texture_rg
			break;
		case GL_DEPTH_STENCIL:     // GL_OES_packed_depth_stencil (GL_DEPTH_STENCIL_OES)
		case GL_DEPTH_COMPONENT:   // GL_OES_depth_texture
			switch(target)
			{
			case GL_TEXTURE_2D:
			case GL_TEXTURE_2D_ARRAY:
			case GL_TEXTURE_CUBE_MAP_POSITIVE_X:   // GL_OES_depth_texture_cube_map
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
			case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
			case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
				break;
			default:
				return GL_INVALID_OPERATION;
			}
			break;
		case GL_RED_INTEGER:
		case GL_RG_INTEGER:
		case GL_RGB_INTEGER:
		case GL_RGBA_INTEGER:
			if(clientVersion < 3)
			{
				return GL_INVALID_ENUM;
			}
			break;
		default:
			return GL_INVALID_ENUM;
		}

		if((GLenum)internalformat != format)
		{
			if(gl::IsUnsizedInternalFormat(internalformat))
			{
				return GL_INVALID_OPERATION;
			}

			if(!IsSizedInternalFormat(internalformat))
			{
				return GL_INVALID_VALUE;
			}
		}

		if((GLenum)internalformat == format)
		{
			// Validate format, type, and unsized internalformat combinations [OpenGL ES 3.0 Table 3.3]
			switch(format)
			{
			case GL_RGBA:
				switch(type)
				{
				case GL_UNSIGNED_BYTE:
				case GL_UNSIGNED_SHORT_4_4_4_4:
				case GL_UNSIGNED_SHORT_5_5_5_1:
				case GL_FLOAT:            // GL_OES_texture_float
				case GL_HALF_FLOAT_OES:   // GL_OES_texture_half_float
					break;
				default:
					return GL_INVALID_OPERATION;
				}
				break;
			case GL_RGB:
				switch(type)
				{
				case GL_UNSIGNED_BYTE:
				case GL_UNSIGNED_SHORT_5_6_5:
				case GL_FLOAT:            // GL_OES_texture_float
				case GL_HALF_FLOAT_OES:   // GL_OES_texture_half_float
					break;
				default:
					return GL_INVALID_OPERATION;
				}
				break;
			case GL_LUMINANCE_ALPHA:
			case GL_LUMINANCE:
			case GL_ALPHA:
				switch(type)
				{
				case GL_UNSIGNED_BYTE:
				case GL_FLOAT:            // GL_OES_texture_float
				case GL_HALF_FLOAT_OES:   // GL_OES_texture_half_float
					break;
				default:
					return GL_INVALID_OPERATION;
				}
				break;
			case GL_DEPTH_COMPONENT:
				switch(type)
				{
				case GL_UNSIGNED_SHORT:   // GL_OES_depth_texture
				case GL_UNSIGNED_INT:     // GL_OES_depth_texture
					break;
				default:
					return GL_INVALID_OPERATION;
				}
				break;
			case GL_DEPTH_STENCIL_OES:
				switch(type)
				{
				case GL_UNSIGNED_INT_24_8_OES:   // GL_OES_packed_depth_stencil
					break;
				default:
					return GL_INVALID_OPERATION;
				}
				break;
			case GL_RED_EXT:
			case GL_RG_EXT:
				switch(type)
				{
				case GL_UNSIGNED_BYTE:    // GL_EXT_texture_rg
				case GL_FLOAT:            // GL_EXT_texture_rg + GL_OES_texture_float
				case GL_HALF_FLOAT_OES:   // GL_EXT_texture_rg + GL_OES_texture_half_float
					break;
				default:
					return GL_INVALID_OPERATION;
				}
				break;
			case GL_BGRA_EXT:
				if(type != GL_UNSIGNED_BYTE)   // GL_APPLE_texture_format_BGRA8888 / GL_EXT_texture_format_BGRA8888
				{
					return GL_INVALID_OPERATION;
				}
				break;
			default:
				UNREACHABLE(format);
				return GL_INVALID_ENUM;
			}

			return GL_NO_ERROR;
		}

		// Validate format, type, and sized internalformat combinations [OpenGL ES 3.0 Table 3.2]
		bool validSizedInternalformat = false;
		#define VALIDATE_INTERNALFORMAT(...) { GLint validInternalformats[] = {__VA_ARGS__}; for(GLint v : validInternalformats) {if(internalformat == v) validSizedInternalformat = true;} } break;

		switch(format)
		{
		case GL_RGBA:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:               VALIDATE_INTERNALFORMAT(GL_RGBA8, GL_RGB5_A1, GL_RGBA4, GL_SRGB8_ALPHA8)
			case GL_BYTE:                        VALIDATE_INTERNALFORMAT(GL_RGBA8_SNORM)
			case GL_UNSIGNED_SHORT_4_4_4_4:      VALIDATE_INTERNALFORMAT(GL_RGBA4)
			case GL_UNSIGNED_SHORT_5_5_5_1:      VALIDATE_INTERNALFORMAT(GL_RGB5_A1)
			case GL_UNSIGNED_INT_2_10_10_10_REV: VALIDATE_INTERNALFORMAT(GL_RGB10_A2, GL_RGB5_A1)
			case GL_HALF_FLOAT_OES:
			case GL_HALF_FLOAT:                  VALIDATE_INTERNALFORMAT(GL_RGBA16F)
			case GL_FLOAT:                       VALIDATE_INTERNALFORMAT(GL_RGBA32F, GL_RGBA16F)
			default:                             return GL_INVALID_OPERATION;
			}
			break;
		case GL_RGBA_INTEGER:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:               VALIDATE_INTERNALFORMAT(GL_RGBA8UI)
			case GL_BYTE:                        VALIDATE_INTERNALFORMAT(GL_RGBA8I)
			case GL_UNSIGNED_SHORT:              VALIDATE_INTERNALFORMAT(GL_RGBA16UI)
			case GL_SHORT:                       VALIDATE_INTERNALFORMAT(GL_RGBA16I)
			case GL_UNSIGNED_INT:                VALIDATE_INTERNALFORMAT(GL_RGBA32UI)
			case GL_INT:                         VALIDATE_INTERNALFORMAT(GL_RGBA32I)
			case GL_UNSIGNED_INT_2_10_10_10_REV: VALIDATE_INTERNALFORMAT(GL_RGB10_A2UI)
			default:                             return GL_INVALID_OPERATION;
			}
			break;
		case GL_RGB:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:                VALIDATE_INTERNALFORMAT(GL_RGB8, GL_RGB565, GL_SRGB8)
			case GL_BYTE:                         VALIDATE_INTERNALFORMAT(GL_RGB8_SNORM)
			case GL_UNSIGNED_SHORT_5_6_5:         VALIDATE_INTERNALFORMAT(GL_RGB565)
			case GL_UNSIGNED_INT_10F_11F_11F_REV: VALIDATE_INTERNALFORMAT(GL_R11F_G11F_B10F)
			case GL_UNSIGNED_INT_5_9_9_9_REV:     VALIDATE_INTERNALFORMAT(GL_RGB9_E5)
			case GL_HALF_FLOAT_OES:
			case GL_HALF_FLOAT:                   VALIDATE_INTERNALFORMAT(GL_RGB16F, GL_R11F_G11F_B10F, GL_RGB9_E5)
			case GL_FLOAT:                        VALIDATE_INTERNALFORMAT(GL_RGB32F, GL_RGB16F, GL_R11F_G11F_B10F, GL_RGB9_E5)
			default:                              return GL_INVALID_OPERATION;
			}
			break;
		case GL_RGB_INTEGER:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  VALIDATE_INTERNALFORMAT(GL_RGB8UI)
			case GL_BYTE:           VALIDATE_INTERNALFORMAT(GL_RGB8I)
			case GL_UNSIGNED_SHORT: VALIDATE_INTERNALFORMAT(GL_RGB16UI)
			case GL_SHORT:          VALIDATE_INTERNALFORMAT(GL_RGB16I)
			case GL_UNSIGNED_INT:   VALIDATE_INTERNALFORMAT(GL_RGB32UI)
			case GL_INT:            VALIDATE_INTERNALFORMAT(GL_RGB32I)
			default:                return GL_INVALID_OPERATION;
			}
			break;
		case GL_RG:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  VALIDATE_INTERNALFORMAT(GL_RG8)
			case GL_BYTE:           VALIDATE_INTERNALFORMAT(GL_RG8_SNORM)
			case GL_HALF_FLOAT_OES:
			case GL_HALF_FLOAT:     VALIDATE_INTERNALFORMAT(GL_RG16F)
			case GL_FLOAT:          VALIDATE_INTERNALFORMAT(GL_RG32F, GL_RG16F)
			default:                return GL_INVALID_OPERATION;
			}
			break;
		case GL_RG_INTEGER:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  VALIDATE_INTERNALFORMAT(GL_RG8UI)
			case GL_BYTE:           VALIDATE_INTERNALFORMAT(GL_RG8I)
			case GL_UNSIGNED_SHORT: VALIDATE_INTERNALFORMAT(GL_RG16UI)
			case GL_SHORT:          VALIDATE_INTERNALFORMAT(GL_RG16I)
			case GL_UNSIGNED_INT:   VALIDATE_INTERNALFORMAT(GL_RG32UI)
			case GL_INT:            VALIDATE_INTERNALFORMAT(GL_RG32I)
			default:                return GL_INVALID_OPERATION;
			}
			break;
		case GL_RED:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  VALIDATE_INTERNALFORMAT(GL_R8)
			case GL_BYTE:           VALIDATE_INTERNALFORMAT(GL_R8_SNORM)
			case GL_HALF_FLOAT_OES:
			case GL_HALF_FLOAT:     VALIDATE_INTERNALFORMAT(GL_R16F)
			case GL_FLOAT:          VALIDATE_INTERNALFORMAT(GL_R32F, GL_R16F)
			default:                return GL_INVALID_OPERATION;
			}
			break;
		case GL_RED_INTEGER:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  VALIDATE_INTERNALFORMAT(GL_R8UI)
			case GL_BYTE:           VALIDATE_INTERNALFORMAT(GL_R8I)
			case GL_UNSIGNED_SHORT: VALIDATE_INTERNALFORMAT(GL_R16UI)
			case GL_SHORT:          VALIDATE_INTERNALFORMAT(GL_R16I)
			case GL_UNSIGNED_INT:   VALIDATE_INTERNALFORMAT(GL_R32UI)
			case GL_INT:            VALIDATE_INTERNALFORMAT(GL_R32I)
			default:                return GL_INVALID_OPERATION;
			}
			break;
		case GL_DEPTH_COMPONENT:
			switch(type)
			{
			case GL_UNSIGNED_SHORT: VALIDATE_INTERNALFORMAT(GL_DEPTH_COMPONENT16)
			case GL_UNSIGNED_INT:   VALIDATE_INTERNALFORMAT(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT16)
			case GL_FLOAT:          VALIDATE_INTERNALFORMAT(GL_DEPTH_COMPONENT32F)
			default:                return GL_INVALID_OPERATION;
			}
			break;
		case GL_DEPTH_STENCIL:
			switch(type)
			{
			case GL_UNSIGNED_INT_24_8:              VALIDATE_INTERNALFORMAT(GL_DEPTH24_STENCIL8)
			case GL_FLOAT_32_UNSIGNED_INT_24_8_REV: VALIDATE_INTERNALFORMAT(GL_DEPTH32F_STENCIL8)
			default:                                return GL_INVALID_OPERATION;
			}
			break;
		case GL_LUMINANCE_ALPHA:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  VALIDATE_INTERNALFORMAT(GL_LUMINANCE8_ALPHA8_EXT)
			case GL_HALF_FLOAT_OES:
			case GL_HALF_FLOAT:     VALIDATE_INTERNALFORMAT(GL_LUMINANCE_ALPHA16F_EXT)
			case GL_FLOAT:          VALIDATE_INTERNALFORMAT(GL_LUMINANCE_ALPHA32F_EXT, GL_LUMINANCE_ALPHA16F_EXT)
			default:
				return GL_INVALID_OPERATION;
			}
			break;
		case GL_LUMINANCE:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  VALIDATE_INTERNALFORMAT(GL_LUMINANCE8_EXT)
			case GL_HALF_FLOAT_OES:
			case GL_HALF_FLOAT:     VALIDATE_INTERNALFORMAT(GL_LUMINANCE16F_EXT)
			case GL_FLOAT:          VALIDATE_INTERNALFORMAT(GL_LUMINANCE32F_EXT, GL_LUMINANCE16F_EXT)
			default:
				return GL_INVALID_OPERATION;
			}
			break;
		case GL_ALPHA:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  VALIDATE_INTERNALFORMAT(GL_ALPHA8_EXT)
			case GL_HALF_FLOAT_OES:
			case GL_HALF_FLOAT:     VALIDATE_INTERNALFORMAT(GL_ALPHA16F_EXT)
			case GL_FLOAT:          VALIDATE_INTERNALFORMAT(GL_ALPHA32F_EXT, GL_ALPHA16F_EXT)
			default:
				return GL_INVALID_OPERATION;
			}
			break;
		case GL_BGRA_EXT:   // GL_APPLE_texture_format_BGRA8888
			switch(type)
			{
			case GL_UNSIGNED_BYTE: VALIDATE_INTERNALFORMAT(GL_BGRA8_EXT)
			default:               return GL_INVALID_OPERATION;
			}
			break;
		default:
			UNREACHABLE(format);
			return GL_INVALID_ENUM;
		}

		#undef VALIDATE_INTERNALFORMAT

		if(!validSizedInternalformat)
		{
			return GL_INVALID_OPERATION;
		}

		return GL_NO_ERROR;
	}

	size_t GetTypeSize(GLenum type)
	{
		switch(type)
		{
		case GL_BYTE:
		case GL_UNSIGNED_BYTE:
			return 1;
		case GL_UNSIGNED_SHORT_4_4_4_4:
		case GL_UNSIGNED_SHORT_5_5_5_1:
		case GL_UNSIGNED_SHORT_5_6_5:
		case GL_UNSIGNED_SHORT:
		case GL_SHORT:
		case GL_HALF_FLOAT:
		case GL_HALF_FLOAT_OES:
			return 2;
		case GL_FLOAT:
		case GL_UNSIGNED_INT_24_8:
		case GL_UNSIGNED_INT:
		case GL_INT:
		case GL_UNSIGNED_INT_2_10_10_10_REV:
		case GL_UNSIGNED_INT_10F_11F_11F_REV:
		case GL_UNSIGNED_INT_5_9_9_9_REV:
		case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
			return 4;
		default:
			UNREACHABLE(type);
			break;
		}

		return 1;
	}

	bool IsColorRenderable(GLint internalformat, GLint clientVersion)
	{
		if(IsCompressed(internalformat, clientVersion))
		{
			return false;
		}

		switch(internalformat)
		{
		case GL_RGBA4:
		case GL_RGB5_A1:
		case GL_RGB565:
		case GL_R8:
		case GL_RG8:
		case GL_RGB8:
		case GL_RGBA8:
		case GL_R16F:
		case GL_RG16F:
		case GL_RGB16F:
		case GL_RGBA16F:
		case GL_R32F:
		case GL_RG32F:
		case GL_RGB32F:
		case GL_RGBA32F:
		case GL_BGRA8_EXT:   // GL_EXT_texture_format_BGRA8888
			return true;
		case GL_R8UI:
		case GL_R8I:
		case GL_R16UI:
		case GL_R16I:
		case GL_R32UI:
		case GL_R32I:
		case GL_RG8UI:
		case GL_RG8I:
		case GL_RG16UI:
		case GL_RG16I:
		case GL_RG32UI:
		case GL_RG32I:
		case GL_SRGB8_ALPHA8:
		case GL_RGB10_A2:
		case GL_RGBA8UI:
		case GL_RGBA8I:
		case GL_RGB10_A2UI:
		case GL_RGBA16UI:
		case GL_RGBA16I:
		case GL_RGBA32I:
		case GL_RGBA32UI:
		case GL_R11F_G11F_B10F:
			return clientVersion >= 3;
		case GL_R8_SNORM:
		case GL_RG8_SNORM:
		case GL_RGB8_SNORM:
		case GL_RGBA8_SNORM:
		case GL_ALPHA8_EXT:
		case GL_LUMINANCE8_EXT:
		case GL_LUMINANCE8_ALPHA8_EXT:
		case GL_ALPHA32F_EXT:
		case GL_LUMINANCE32F_EXT:
		case GL_LUMINANCE_ALPHA32F_EXT:
		case GL_ALPHA16F_EXT:
		case GL_LUMINANCE16F_EXT:
		case GL_LUMINANCE_ALPHA16F_EXT:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32_OES:
		case GL_DEPTH_COMPONENT32F:
		case GL_DEPTH32F_STENCIL8:
		case GL_DEPTH_COMPONENT16:
		case GL_STENCIL_INDEX8:
		case GL_DEPTH24_STENCIL8_OES:
			return false;
		default:
			UNIMPLEMENTED();
		}

		return false;
	}

	bool IsDepthRenderable(GLint internalformat, GLint clientVersion)
	{
		if(IsCompressed(internalformat, clientVersion))
		{
			return false;
		}

		switch(internalformat)
		{
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH24_STENCIL8_OES:    // GL_OES_packed_depth_stencil
		case GL_DEPTH_COMPONENT32_OES:   // GL_OES_depth32
			return true;
		case GL_DEPTH32F_STENCIL8:
		case GL_DEPTH_COMPONENT32F:
			return clientVersion >= 3;
		case GL_STENCIL_INDEX8:
		case GL_R8:
		case GL_R8UI:
		case GL_R8I:
		case GL_R16UI:
		case GL_R16I:
		case GL_R32UI:
		case GL_R32I:
		case GL_RG8:
		case GL_RG8UI:
		case GL_RG8I:
		case GL_RG16UI:
		case GL_RG16I:
		case GL_RG32UI:
		case GL_RG32I:
		case GL_SRGB8_ALPHA8:
		case GL_RGB10_A2:
		case GL_RGBA8UI:
		case GL_RGBA8I:
		case GL_RGB10_A2UI:
		case GL_RGBA16UI:
		case GL_RGBA16I:
		case GL_RGBA32I:
		case GL_RGBA32UI:
		case GL_RGBA4:
		case GL_RGB5_A1:
		case GL_RGB565:
		case GL_RGB8:
		case GL_RGBA8:
		case GL_RED:
		case GL_RG:
		case GL_RGB:
		case GL_RGBA:
		case GL_R16F:
		case GL_RG16F:
		case GL_R11F_G11F_B10F:
		case GL_RGB16F:
		case GL_RGBA16F:
		case GL_R32F:
		case GL_RG32F:
		case GL_RGB32F:
		case GL_RGBA32F:
		case GL_R8_SNORM:
		case GL_RG8_SNORM:
		case GL_RGB8_SNORM:
		case GL_RGBA8_SNORM:
			return false;
		default:
			UNIMPLEMENTED();
		}

		return false;
	}

	bool IsStencilRenderable(GLint internalformat, GLint clientVersion)
	{
		if(IsCompressed(internalformat, clientVersion))
		{
			return false;
		}

		switch(internalformat)
		{
		case GL_STENCIL_INDEX8:
		case GL_DEPTH24_STENCIL8_OES:
			return true;
		case GL_DEPTH32F_STENCIL8:
			return clientVersion >= 3;
		case GL_R8:
		case GL_R8UI:
		case GL_R8I:
		case GL_R16UI:
		case GL_R16I:
		case GL_R32UI:
		case GL_R32I:
		case GL_RG8:
		case GL_RG8UI:
		case GL_RG8I:
		case GL_RG16UI:
		case GL_RG16I:
		case GL_RG32UI:
		case GL_RG32I:
		case GL_SRGB8_ALPHA8:
		case GL_RGB10_A2:
		case GL_RGBA8UI:
		case GL_RGBA8I:
		case GL_RGB10_A2UI:
		case GL_RGBA16UI:
		case GL_RGBA16I:
		case GL_RGBA32I:
		case GL_RGBA32UI:
		case GL_RGBA4:
		case GL_RGB5_A1:
		case GL_RGB565:
		case GL_RGB8:
		case GL_RGBA8:
		case GL_RED:
		case GL_RG:
		case GL_RGB:
		case GL_RGBA:
		case GL_R16F:
		case GL_RG16F:
		case GL_R11F_G11F_B10F:
		case GL_RGB16F:
		case GL_RGBA16F:
		case GL_R32F:
		case GL_RG32F:
		case GL_RGB32F:
		case GL_RGBA32F:
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32_OES:
		case GL_DEPTH_COMPONENT32F:
		case GL_R8_SNORM:
		case GL_RG8_SNORM:
		case GL_RGB8_SNORM:
		case GL_RGBA8_SNORM:
			return false;
		default:
			UNIMPLEMENTED();
		}

		return false;
	}

	bool IsMipmappable(GLint internalformat, GLint clientVersion)
	{
		if(internalformat == GL_NONE)
		{
			return true;   // Image unspecified. Not an error.
		}

		if(IsNonNormalizedInteger(internalformat))
		{
			return false;
		}

		switch(internalformat)
		{
		case GL_ALPHA8_EXT:
		case GL_LUMINANCE8_EXT:
		case GL_LUMINANCE8_ALPHA8_EXT:
		case GL_ALPHA32F_EXT:
		case GL_LUMINANCE32F_EXT:
		case GL_LUMINANCE_ALPHA32F_EXT:
		case GL_ALPHA16F_EXT:
		case GL_LUMINANCE16F_EXT:
		case GL_LUMINANCE_ALPHA16F_EXT:
			return true;
		default:
			return IsColorRenderable(internalformat, clientVersion);
		}
	}

	GLuint GetAlphaSize(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_NONE:           return 0;
		case GL_RGBA4:          return 4;
		case GL_RGB5_A1:        return 1;
		case GL_RGB565:         return 0;
		case GL_R8:             return 0;
		case GL_RG8:            return 0;
		case GL_RGB8:           return 0;
		case GL_RGBA8:          return 8;
		case GL_R16F:           return 0;
		case GL_RG16F:          return 0;
		case GL_RGB16F:         return 0;
		case GL_RGBA16F:        return 16;
		case GL_R32F:           return 0;
		case GL_RG32F:          return 0;
		case GL_RGB32F:         return 0;
		case GL_RGBA32F:        return 32;
		case GL_BGRA8_EXT:      return 8;
		case GL_R8UI:           return 0;
		case GL_R8I:            return 0;
		case GL_R16UI:          return 0;
		case GL_R16I:           return 0;
		case GL_R32UI:          return 0;
		case GL_R32I:           return 0;
		case GL_RG8UI:          return 0;
		case GL_RG8I:           return 0;
		case GL_RG16UI:         return 0;
		case GL_RG16I:          return 0;
		case GL_RG32UI:         return 0;
		case GL_RG32I:          return 0;
		case GL_SRGB8_ALPHA8:   return 8;
		case GL_RGB10_A2:       return 2;
		case GL_RGBA8UI:        return 8;
		case GL_RGBA8I:         return 8;
		case GL_RGB10_A2UI:     return 2;
		case GL_RGBA16UI:       return 16;
		case GL_RGBA16I:        return 16;
		case GL_RGBA32I:        return 32;
		case GL_RGBA32UI:       return 32;
		case GL_R11F_G11F_B10F: return 0;
		default:
		//	UNREACHABLE(internalformat);
			return 0;
		}
	}

	GLuint GetRedSize(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_NONE:           return 0;
		case GL_RGBA4:          return 4;
		case GL_RGB5_A1:        return 5;
		case GL_RGB565:         return 5;
		case GL_R8:             return 8;
		case GL_RG8:            return 8;
		case GL_RGB8:           return 8;
		case GL_RGBA8:          return 8;
		case GL_R16F:           return 16;
		case GL_RG16F:          return 16;
		case GL_RGB16F:         return 16;
		case GL_RGBA16F:        return 16;
		case GL_R32F:           return 32;
		case GL_RG32F:          return 32;
		case GL_RGB32F:         return 32;
		case GL_RGBA32F:        return 32;
		case GL_BGRA8_EXT:      return 8;
		case GL_R8UI:           return 8;
		case GL_R8I:            return 8;
		case GL_R16UI:          return 16;
		case GL_R16I:           return 16;
		case GL_R32UI:          return 32;
		case GL_R32I:           return 32;
		case GL_RG8UI:          return 8;
		case GL_RG8I:           return 8;
		case GL_RG16UI:         return 16;
		case GL_RG16I:          return 16;
		case GL_RG32UI:         return 32;
		case GL_RG32I:          return 32;
		case GL_SRGB8_ALPHA8:   return 8;
		case GL_RGB10_A2:       return 10;
		case GL_RGBA8UI:        return 8;
		case GL_RGBA8I:         return 8;
		case GL_RGB10_A2UI:     return 10;
		case GL_RGBA16UI:       return 16;
		case GL_RGBA16I:        return 16;
		case GL_RGBA32I:        return 32;
		case GL_RGBA32UI:       return 32;
		case GL_R11F_G11F_B10F: return 11;
		default:
		//	UNREACHABLE(internalformat);
			return 0;
		}
	}

	GLuint GetGreenSize(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_NONE:           return 0;
		case GL_RGBA4:          return 4;
		case GL_RGB5_A1:        return 5;
		case GL_RGB565:         return 6;
		case GL_R8:             return 0;
		case GL_RG8:            return 8;
		case GL_RGB8:           return 8;
		case GL_RGBA8:          return 8;
		case GL_R16F:           return 0;
		case GL_RG16F:          return 16;
		case GL_RGB16F:         return 16;
		case GL_RGBA16F:        return 16;
		case GL_R32F:           return 0;
		case GL_RG32F:          return 32;
		case GL_RGB32F:         return 32;
		case GL_RGBA32F:        return 32;
		case GL_BGRA8_EXT:      return 8;
		case GL_R8UI:           return 0;
		case GL_R8I:            return 0;
		case GL_R16UI:          return 0;
		case GL_R16I:           return 0;
		case GL_R32UI:          return 0;
		case GL_R32I:           return 0;
		case GL_RG8UI:          return 8;
		case GL_RG8I:           return 8;
		case GL_RG16UI:         return 16;
		case GL_RG16I:          return 16;
		case GL_RG32UI:         return 32;
		case GL_RG32I:          return 32;
		case GL_SRGB8_ALPHA8:   return 8;
		case GL_RGB10_A2:       return 10;
		case GL_RGBA8UI:        return 8;
		case GL_RGBA8I:         return 8;
		case GL_RGB10_A2UI:     return 10;
		case GL_RGBA16UI:       return 16;
		case GL_RGBA16I:        return 16;
		case GL_RGBA32I:        return 32;
		case GL_RGBA32UI:       return 32;
		case GL_R11F_G11F_B10F: return 11;
		default:
		//	UNREACHABLE(internalformat);
			return 0;
		}
	}

	GLuint GetBlueSize(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_NONE:           return 0;
		case GL_RGBA4:          return 4;
		case GL_RGB5_A1:        return 5;
		case GL_RGB565:         return 5;
		case GL_R8:             return 0;
		case GL_RG8:            return 0;
		case GL_RGB8:           return 8;
		case GL_RGBA8:          return 8;
		case GL_R16F:           return 0;
		case GL_RG16F:          return 0;
		case GL_RGB16F:         return 16;
		case GL_RGBA16F:        return 16;
		case GL_R32F:           return 0;
		case GL_RG32F:          return 0;
		case GL_RGB32F:         return 32;
		case GL_RGBA32F:        return 32;
		case GL_BGRA8_EXT:      return 8;
		case GL_R8UI:           return 0;
		case GL_R8I:            return 0;
		case GL_R16UI:          return 0;
		case GL_R16I:           return 0;
		case GL_R32UI:          return 0;
		case GL_R32I:           return 0;
		case GL_RG8UI:          return 0;
		case GL_RG8I:           return 0;
		case GL_RG16UI:         return 0;
		case GL_RG16I:          return 0;
		case GL_RG32UI:         return 0;
		case GL_RG32I:          return 0;
		case GL_SRGB8_ALPHA8:   return 8;
		case GL_RGB10_A2:       return 10;
		case GL_RGBA8UI:        return 8;
		case GL_RGBA8I:         return 8;
		case GL_RGB10_A2UI:     return 10;
		case GL_RGBA16UI:       return 16;
		case GL_RGBA16I:        return 16;
		case GL_RGBA32I:        return 32;
		case GL_RGBA32UI:       return 32;
		case GL_R11F_G11F_B10F: return 10;
		default:
		//	UNREACHABLE(internalformat);
			return 0;
		}
	}

	GLuint GetDepthSize(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_STENCIL_INDEX8:        return 0;
		case GL_DEPTH_COMPONENT16:     return 16;
		case GL_DEPTH_COMPONENT24:     return 24;
		case GL_DEPTH_COMPONENT32_OES: return 32;
		case GL_DEPTH_COMPONENT32F:    return 32;
		case GL_DEPTH24_STENCIL8:      return 24;
		case GL_DEPTH32F_STENCIL8:     return 32;
		default:
		//	UNREACHABLE(internalformat);
			return 0;
		}
	}

	GLuint GetStencilSize(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_STENCIL_INDEX8:        return 8;
		case GL_DEPTH_COMPONENT16:     return 0;
		case GL_DEPTH_COMPONENT24:     return 0;
		case GL_DEPTH_COMPONENT32_OES: return 0;
		case GL_DEPTH_COMPONENT32F:    return 0;
		case GL_DEPTH24_STENCIL8:      return 8;
		case GL_DEPTH32F_STENCIL8:     return 8;
		default:
		//	UNREACHABLE(internalformat);
			return 0;
		}
	}

	GLenum GetColorComponentType(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_ALPHA8_EXT:
		case GL_LUMINANCE8_ALPHA8_EXT:
		case GL_LUMINANCE8_EXT:
		case GL_R8:
		case GL_RG8:
		case GL_SRGB8_ALPHA8:
		case GL_RGB10_A2:
		case GL_RGBA4:
		case GL_RGB5_A1:
		case GL_RGB565:
		case GL_RGB8:
		case GL_RGBA8:
		case GL_SRGB8:
		case GL_BGRA8_EXT:
			return GL_UNSIGNED_NORMALIZED;
		case GL_R8_SNORM:
		case GL_RG8_SNORM:
		case GL_RGB8_SNORM:
		case GL_RGBA8_SNORM:
			return GL_SIGNED_NORMALIZED;
		case GL_R8UI:
		case GL_R16UI:
		case GL_R32UI:
		case GL_RG8UI:
		case GL_RG16UI:
		case GL_RG32UI:
		case GL_RGB8UI:
		case GL_RGB16UI:
		case GL_RGB32UI:
		case GL_RGB10_A2UI:
		case GL_RGBA16UI:
		case GL_RGBA32UI:
		case GL_RGBA8UI:
			return GL_UNSIGNED_INT;
		case GL_R8I:
		case GL_R16I:
		case GL_R32I:
		case GL_RG8I:
		case GL_RG16I:
		case GL_RG32I:
		case GL_RGB8I:
		case GL_RGB16I:
		case GL_RGB32I:
		case GL_RGBA8I:
		case GL_RGBA16I:
		case GL_RGBA32I:
			return GL_INT;
		case GL_ALPHA32F_EXT:
		case GL_LUMINANCE32F_EXT:
		case GL_LUMINANCE_ALPHA32F_EXT:
		case GL_ALPHA16F_EXT:
		case GL_LUMINANCE16F_EXT:
		case GL_LUMINANCE_ALPHA16F_EXT:
		case GL_R16F:
		case GL_RG16F:
		case GL_R11F_G11F_B10F:
		case GL_RGB16F:
		case GL_RGBA16F:
		case GL_R32F:
		case GL_RG32F:
		case GL_RGB32F:
		case GL_RGBA32F:
		case GL_RGB9_E5:
			return GL_FLOAT;
		default:
		//	UNREACHABLE(internalformat);
			return GL_NONE;
		}
	}

	GLenum GetComponentType(GLint internalformat, GLenum attachment)
	{
		// Can be one of GL_FLOAT, GL_INT, GL_UNSIGNED_INT, GL_SIGNED_NORMALIZED, or GL_UNSIGNED_NORMALIZED
		switch(attachment)
		{
		case GL_COLOR_ATTACHMENT0:
		case GL_COLOR_ATTACHMENT1:
		case GL_COLOR_ATTACHMENT2:
		case GL_COLOR_ATTACHMENT3:
		case GL_COLOR_ATTACHMENT4:
		case GL_COLOR_ATTACHMENT5:
		case GL_COLOR_ATTACHMENT6:
		case GL_COLOR_ATTACHMENT7:
		case GL_COLOR_ATTACHMENT8:
		case GL_COLOR_ATTACHMENT9:
		case GL_COLOR_ATTACHMENT10:
		case GL_COLOR_ATTACHMENT11:
		case GL_COLOR_ATTACHMENT12:
		case GL_COLOR_ATTACHMENT13:
		case GL_COLOR_ATTACHMENT14:
		case GL_COLOR_ATTACHMENT15:
		case GL_COLOR_ATTACHMENT16:
		case GL_COLOR_ATTACHMENT17:
		case GL_COLOR_ATTACHMENT18:
		case GL_COLOR_ATTACHMENT19:
		case GL_COLOR_ATTACHMENT20:
		case GL_COLOR_ATTACHMENT21:
		case GL_COLOR_ATTACHMENT22:
		case GL_COLOR_ATTACHMENT23:
		case GL_COLOR_ATTACHMENT24:
		case GL_COLOR_ATTACHMENT25:
		case GL_COLOR_ATTACHMENT26:
		case GL_COLOR_ATTACHMENT27:
		case GL_COLOR_ATTACHMENT28:
		case GL_COLOR_ATTACHMENT29:
		case GL_COLOR_ATTACHMENT30:
		case GL_COLOR_ATTACHMENT31:
			return GetColorComponentType(internalformat);
		case GL_DEPTH_ATTACHMENT:
		case GL_STENCIL_ATTACHMENT:
			// Only color buffers may have integer components.
			return GL_FLOAT;
		default:
			UNREACHABLE(attachment);
			return GL_NONE;
		}
	}

	bool IsNormalizedInteger(GLint internalformat)
	{
		GLenum type = GetColorComponentType(internalformat);

		return type == GL_UNSIGNED_NORMALIZED || type == GL_SIGNED_NORMALIZED;
	}

	bool IsNonNormalizedInteger(GLint internalformat)
	{
		GLenum type = GetColorComponentType(internalformat);

		return type == GL_UNSIGNED_INT || type == GL_INT;
	}

	bool IsFloatFormat(GLint internalformat)
	{
		return GetColorComponentType(internalformat) == GL_FLOAT;
	}

	bool IsSignedNonNormalizedInteger(GLint internalformat)
	{
		return GetColorComponentType(internalformat) == GL_INT;
	}

	bool IsUnsignedNonNormalizedInteger(GLint internalformat)
	{
		return GetColorComponentType(internalformat) == GL_UNSIGNED_INT;
	}

	GLenum GetColorEncoding(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_SRGB8:
		case GL_SRGB8_ALPHA8:
			return GL_SRGB;
		default:
			// [OpenGL ES 3.0.5] section 6.1.13 page 242:
			// If attachment is not a color attachment, or no data storage or texture image
			// has been specified for the attachment, params will contain the value LINEAR.
			return GL_LINEAR;
		}
	}

	std::string ParseUniformName(const std::string &name, unsigned int *outSubscript)
	{
		// Strip any trailing array operator and retrieve the subscript
		size_t open = name.find_last_of('[');
		size_t close = name.find_last_of(']');
		bool hasIndex = (open != std::string::npos) && (close == name.length() - 1);
		if(!hasIndex)
		{
			if(outSubscript)
			{
				*outSubscript = GL_INVALID_INDEX;
			}
			return name;
		}

		if(outSubscript)
		{
			int index = atoi(name.substr(open + 1).c_str());
			if(index >= 0)
			{
				*outSubscript = index;
			}
			else
			{
				*outSubscript = GL_INVALID_INDEX;
			}
		}

		return name.substr(0, open);
	}
}

namespace es2sw
{
	sw::DepthCompareMode ConvertDepthComparison(GLenum comparison)
	{
		switch(comparison)
		{
		case GL_NEVER:    return sw::DEPTH_NEVER;
		case GL_ALWAYS:   return sw::DEPTH_ALWAYS;
		case GL_LESS:     return sw::DEPTH_LESS;
		case GL_LEQUAL:   return sw::DEPTH_LESSEQUAL;
		case GL_EQUAL:    return sw::DEPTH_EQUAL;
		case GL_GREATER:  return sw::DEPTH_GREATER;
		case GL_GEQUAL:   return sw::DEPTH_GREATEREQUAL;
		case GL_NOTEQUAL: return sw::DEPTH_NOTEQUAL;
		default: UNREACHABLE(comparison);
		}

		return sw::DEPTH_ALWAYS;
	}

	sw::StencilCompareMode ConvertStencilComparison(GLenum comparison)
	{
		switch(comparison)
		{
		case GL_NEVER:    return sw::STENCIL_NEVER;
		case GL_ALWAYS:   return sw::STENCIL_ALWAYS;
		case GL_LESS:     return sw::STENCIL_LESS;
		case GL_LEQUAL:   return sw::STENCIL_LESSEQUAL;
		case GL_EQUAL:    return sw::STENCIL_EQUAL;
		case GL_GREATER:  return sw::STENCIL_GREATER;
		case GL_GEQUAL:   return sw::STENCIL_GREATEREQUAL;
		case GL_NOTEQUAL: return sw::STENCIL_NOTEQUAL;
		default: UNREACHABLE(comparison);
		}

		return sw::STENCIL_ALWAYS;
	}

	sw::Color<float> ConvertColor(es2::Color color)
	{
		return sw::Color<float>(color.red, color.green, color.blue, color.alpha);
	}

	sw::BlendFactor ConvertBlendFunc(GLenum blend)
	{
		switch(blend)
		{
		case GL_ZERO:                     return sw::BLEND_ZERO;
		case GL_ONE:                      return sw::BLEND_ONE;
		case GL_SRC_COLOR:                return sw::BLEND_SOURCE;
		case GL_ONE_MINUS_SRC_COLOR:      return sw::BLEND_INVSOURCE;
		case GL_DST_COLOR:                return sw::BLEND_DEST;
		case GL_ONE_MINUS_DST_COLOR:      return sw::BLEND_INVDEST;
		case GL_SRC_ALPHA:                return sw::BLEND_SOURCEALPHA;
		case GL_ONE_MINUS_SRC_ALPHA:      return sw::BLEND_INVSOURCEALPHA;
		case GL_DST_ALPHA:                return sw::BLEND_DESTALPHA;
		case GL_ONE_MINUS_DST_ALPHA:      return sw::BLEND_INVDESTALPHA;
		case GL_CONSTANT_COLOR:           return sw::BLEND_CONSTANT;
		case GL_ONE_MINUS_CONSTANT_COLOR: return sw::BLEND_INVCONSTANT;
		case GL_CONSTANT_ALPHA:           return sw::BLEND_CONSTANTALPHA;
		case GL_ONE_MINUS_CONSTANT_ALPHA: return sw::BLEND_INVCONSTANTALPHA;
		case GL_SRC_ALPHA_SATURATE:       return sw::BLEND_SRCALPHASAT;
		default: UNREACHABLE(blend);
		}

		return sw::BLEND_ZERO;
	}

	sw::BlendOperation ConvertBlendOp(GLenum blendOp)
	{
		switch(blendOp)
		{
		case GL_FUNC_ADD:              return sw::BLENDOP_ADD;
		case GL_FUNC_SUBTRACT:         return sw::BLENDOP_SUB;
		case GL_FUNC_REVERSE_SUBTRACT: return sw::BLENDOP_INVSUB;
		case GL_MIN_EXT:               return sw::BLENDOP_MIN;
		case GL_MAX_EXT:               return sw::BLENDOP_MAX;
		default: UNREACHABLE(blendOp);
		}

		return sw::BLENDOP_ADD;
	}

	sw::StencilOperation ConvertStencilOp(GLenum stencilOp)
	{
		switch(stencilOp)
		{
		case GL_ZERO:      return sw::OPERATION_ZERO;
		case GL_KEEP:      return sw::OPERATION_KEEP;
		case GL_REPLACE:   return sw::OPERATION_REPLACE;
		case GL_INCR:      return sw::OPERATION_INCRSAT;
		case GL_DECR:      return sw::OPERATION_DECRSAT;
		case GL_INVERT:    return sw::OPERATION_INVERT;
		case GL_INCR_WRAP: return sw::OPERATION_INCR;
		case GL_DECR_WRAP: return sw::OPERATION_DECR;
		default: UNREACHABLE(stencilOp);
		}

		return sw::OPERATION_KEEP;
	}

	sw::AddressingMode ConvertTextureWrap(GLenum wrap)
	{
		switch(wrap)
		{
		case GL_REPEAT:            return sw::ADDRESSING_WRAP;
		case GL_CLAMP_TO_EDGE:     return sw::ADDRESSING_CLAMP;
		case GL_MIRRORED_REPEAT:   return sw::ADDRESSING_MIRROR;
		default: UNREACHABLE(wrap);
		}

		return sw::ADDRESSING_WRAP;
	}

	sw::CompareFunc ConvertCompareFunc(GLenum compareFunc, GLenum compareMode)
	{
		if(compareMode == GL_COMPARE_REF_TO_TEXTURE)
		{
			switch(compareFunc)
			{
			case GL_LEQUAL:   return sw::COMPARE_LESSEQUAL;
			case GL_GEQUAL:   return sw::COMPARE_GREATEREQUAL;
			case GL_LESS:     return sw::COMPARE_LESS;
			case GL_GREATER:  return sw::COMPARE_GREATER;
			case GL_EQUAL:    return sw::COMPARE_EQUAL;
			case GL_NOTEQUAL: return sw::COMPARE_NOTEQUAL;
			case GL_ALWAYS:   return sw::COMPARE_ALWAYS;
			case GL_NEVER:    return sw::COMPARE_NEVER;
			default: UNREACHABLE(compareFunc);
			}
		}
		else if(compareMode == GL_NONE)
		{
			return sw::COMPARE_BYPASS;
		}
		else UNREACHABLE(compareMode);

		return sw::COMPARE_BYPASS;
	};

	sw::SwizzleType ConvertSwizzleType(GLenum swizzleType)
	{
		switch(swizzleType)
		{
		case GL_RED:   return sw::SWIZZLE_RED;
		case GL_GREEN: return sw::SWIZZLE_GREEN;
		case GL_BLUE:  return sw::SWIZZLE_BLUE;
		case GL_ALPHA: return sw::SWIZZLE_ALPHA;
		case GL_ZERO:  return sw::SWIZZLE_ZERO;
		case GL_ONE:   return sw::SWIZZLE_ONE;
		default: UNREACHABLE(swizzleType);
		}

		return sw::SWIZZLE_RED;
	};

	sw::CullMode ConvertCullMode(GLenum cullFace, GLenum frontFace)
	{
		switch(cullFace)
		{
		case GL_FRONT:
			return (frontFace == GL_CCW ? sw::CULL_CLOCKWISE : sw::CULL_COUNTERCLOCKWISE);
		case GL_BACK:
			return (frontFace == GL_CCW ? sw::CULL_COUNTERCLOCKWISE : sw::CULL_CLOCKWISE);
		case GL_FRONT_AND_BACK:
			return sw::CULL_NONE;   // culling will be handled during draw
		default: UNREACHABLE(cullFace);
		}

		return sw::CULL_COUNTERCLOCKWISE;
	}

	unsigned int ConvertColorMask(bool red, bool green, bool blue, bool alpha)
	{
		return (red   ? 0x00000001 : 0) |
			   (green ? 0x00000002 : 0) |
			   (blue  ? 0x00000004 : 0) |
			   (alpha ? 0x00000008 : 0);
	}

	sw::MipmapType ConvertMipMapFilter(GLenum minFilter)
	{
		switch(minFilter)
		{
		case GL_NEAREST:
		case GL_LINEAR:
			return sw::MIPMAP_NONE;
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_NEAREST:
			return sw::MIPMAP_POINT;
		case GL_NEAREST_MIPMAP_LINEAR:
		case GL_LINEAR_MIPMAP_LINEAR:
			return sw::MIPMAP_LINEAR;
		default:
			UNREACHABLE(minFilter);
			return sw::MIPMAP_NONE;
		}
	}

	sw::FilterType ConvertTextureFilter(GLenum minFilter, GLenum magFilter, float maxAnisotropy)
	{
		if(maxAnisotropy > 1.0f)
		{
			return sw::FILTER_ANISOTROPIC;
		}

		switch(magFilter)
		{
		case GL_NEAREST:
		case GL_LINEAR:
			break;
		default:
			UNREACHABLE(magFilter);
		}

		switch(minFilter)
		{
		case GL_NEAREST:
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_NEAREST_MIPMAP_LINEAR:
			return (magFilter == GL_NEAREST) ? sw::FILTER_POINT : sw::FILTER_MIN_POINT_MAG_LINEAR;
		case GL_LINEAR:
		case GL_LINEAR_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_LINEAR:
			return (magFilter == GL_NEAREST) ? sw::FILTER_MIN_LINEAR_MAG_POINT : sw::FILTER_LINEAR;
		default:
			UNREACHABLE(minFilter);
			return sw::FILTER_POINT;
		}
	}

	bool ConvertPrimitiveType(GLenum primitiveType, GLsizei elementCount, GLenum elementType, sw::DrawType &drawType, int &primitiveCount, int &verticesPerPrimitive)
	{
		switch(primitiveType)
		{
		case GL_POINTS:
			drawType = sw::DRAW_POINTLIST;
			primitiveCount = elementCount;
			verticesPerPrimitive = 1;
			break;
		case GL_LINES:
			drawType = sw::DRAW_LINELIST;
			primitiveCount = elementCount / 2;
			verticesPerPrimitive = 2;
			break;
		case GL_LINE_LOOP:
			drawType = sw::DRAW_LINELOOP;
			primitiveCount = elementCount;
			verticesPerPrimitive = 2;
			break;
		case GL_LINE_STRIP:
			drawType = sw::DRAW_LINESTRIP;
			primitiveCount = elementCount - 1;
			verticesPerPrimitive = 2;
			break;
		case GL_TRIANGLES:
			drawType = sw::DRAW_TRIANGLELIST;
			primitiveCount = elementCount / 3;
			verticesPerPrimitive = 3;
			break;
		case GL_TRIANGLE_STRIP:
			drawType = sw::DRAW_TRIANGLESTRIP;
			primitiveCount = elementCount - 2;
			verticesPerPrimitive = 3;
			break;
		case GL_TRIANGLE_FAN:
			drawType = sw::DRAW_TRIANGLEFAN;
			primitiveCount = elementCount - 2;
			verticesPerPrimitive = 3;
			break;
		default:
			return false;
		}

		sw::DrawType elementSize;
		switch(elementType)
		{
		case GL_NONE:           elementSize = sw::DRAW_NONINDEXED; break;
		case GL_UNSIGNED_BYTE:  elementSize = sw::DRAW_INDEXED8;   break;
		case GL_UNSIGNED_SHORT: elementSize = sw::DRAW_INDEXED16;  break;
		case GL_UNSIGNED_INT:   elementSize = sw::DRAW_INDEXED32;  break;
		default: return false;
		}

		drawType = sw::DrawType(drawType | elementSize);

		return true;
	}
}

namespace sw2es
{
	GLenum ConvertBackBufferFormat(sw::Format format)
	{
		switch(format)
		{
		case sw::FORMAT_A4R4G4B4: return GL_RGBA4;
		case sw::FORMAT_A8R8G8B8: return GL_RGBA8;
		case sw::FORMAT_A8B8G8R8: return GL_RGBA8;
		case sw::FORMAT_A1R5G5B5: return GL_RGB5_A1;
		case sw::FORMAT_R5G6B5:   return GL_RGB565;
		case sw::FORMAT_X8R8G8B8: return GL_RGB8;
		case sw::FORMAT_X8B8G8R8: return GL_RGB8;
		case sw::FORMAT_SRGB8_A8: return GL_RGBA8;
		case sw::FORMAT_SRGB8_X8: return GL_RGB8;
		default:
			UNREACHABLE(format);
		}

		return GL_RGBA4;
	}

	GLenum ConvertDepthStencilFormat(sw::Format format)
	{
		switch(format)
		{
		case sw::FORMAT_D16:    return GL_DEPTH_COMPONENT16;
		case sw::FORMAT_D24X8:  return GL_DEPTH_COMPONENT24;
		case sw::FORMAT_D32:    return GL_DEPTH_COMPONENT32_OES;
		case sw::FORMAT_D24S8:  return GL_DEPTH24_STENCIL8_OES;
		case sw::FORMAT_D32F:   return GL_DEPTH_COMPONENT32F;
		case sw::FORMAT_D32FS8: return GL_DEPTH32F_STENCIL8;
		case sw::FORMAT_S8:     return GL_STENCIL_INDEX8;
		default:
			UNREACHABLE(format);
		}

		return GL_DEPTH24_STENCIL8_OES;
	}
}