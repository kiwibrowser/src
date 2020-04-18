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

#include "mathutil.h"
#include "Context.h"
#include "common/debug.h"

#include <limits>
#include <stdio.h>

namespace es1
{
	bool IsCompressed(GLenum format)
	{
		return format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
		       format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
               format == GL_ETC1_RGB8_OES;
	}

	bool IsSizedInternalFormat(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_ALPHA8_EXT:
		case GL_LUMINANCE8_ALPHA8_EXT:
		case GL_LUMINANCE8_EXT:
		case GL_RGBA4_OES:
		case GL_RGB5_A1_OES:
		case GL_RGB565_OES:
		case GL_RGB8_OES:
		case GL_RGBA8_OES:
		case GL_BGRA8_EXT:   // GL_APPLE_texture_format_BGRA8888
		case GL_DEPTH_COMPONENT16_OES:
		case GL_STENCIL_INDEX8_OES:
		case GL_DEPTH24_STENCIL8_OES:
			return true;
		default:
			return false;
		}
	}

	GLenum ValidateSubImageParams(bool compressed, bool copy, GLenum target, GLint level, GLint xoffset, GLint yoffset,
	                              GLsizei width, GLsizei height, GLenum format, GLenum type, Texture *texture)
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
			GLenum validationError = ValidateTextureFormatType(format, type, sizedInternalFormat, target);
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

	bool IsDepthTexture(GLenum format)
	{
		return format == GL_DEPTH_STENCIL_OES;
	}

	bool IsStencilTexture(GLenum format)
	{
		return format == GL_DEPTH_STENCIL_OES;
	}

	bool IsCubemapTextureTarget(GLenum target)
	{
		return (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_OES && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_OES);
	}

	int CubeFaceIndex(GLenum cubeFace)
	{
		switch(cubeFace)
		{
		case GL_TEXTURE_CUBE_MAP_OES:
		case GL_TEXTURE_CUBE_MAP_POSITIVE_X_OES: return 0;
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_OES: return 1;
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_OES: return 2;
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_OES: return 3;
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_OES: return 4;
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_OES: return 5;
		default: UNREACHABLE(cubeFace); return 0;
		}
	}

	bool IsTextureTarget(GLenum target)
	{
		return target == GL_TEXTURE_2D;
	}

	// Verify that format/type are one of the combinations from table 3.4.
	GLenum ValidateTextureFormatType(GLenum format, GLenum type, GLint internalformat, GLenum target)
	{
		switch(type)
		{
		case GL_UNSIGNED_BYTE:
		case GL_UNSIGNED_SHORT_4_4_4_4:
		case GL_UNSIGNED_SHORT_5_5_5_1:
		case GL_UNSIGNED_SHORT_5_6_5:
		case GL_UNSIGNED_INT_24_8_OES:   // GL_OES_packed_depth_stencil
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
		case GL_BGRA_EXT:            // GL_EXT_texture_format_BGRA8888
			break;
		case GL_DEPTH_STENCIL_OES:   // GL_OES_packed_depth_stencil (GL_DEPTH_STENCIL_OES)
			switch(target)
			{
			case GL_TEXTURE_2D:
				break;
			default:
				return GL_INVALID_OPERATION;
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
			// Validate format, type, and unsized internalformat combinations [OpenGL ES 1.1 Table 3.3]
			switch(format)
			{
			case GL_RGBA:
				switch(type)
				{
				case GL_UNSIGNED_BYTE:
				case GL_UNSIGNED_SHORT_4_4_4_4:
				case GL_UNSIGNED_SHORT_5_5_5_1:
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
			case GL_UNSIGNED_BYTE:               VALIDATE_INTERNALFORMAT(GL_RGBA8_OES, GL_RGB5_A1_OES, GL_RGBA4_OES)
			case GL_UNSIGNED_SHORT_4_4_4_4:      VALIDATE_INTERNALFORMAT(GL_RGBA4_OES)
			case GL_UNSIGNED_SHORT_5_5_5_1:      VALIDATE_INTERNALFORMAT(GL_RGB5_A1_OES)
			default:                             return GL_INVALID_OPERATION;
			}
			break;
		case GL_RGB:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:                VALIDATE_INTERNALFORMAT(GL_RGB8_OES, GL_RGB565_OES)
			case GL_UNSIGNED_SHORT_5_6_5:         VALIDATE_INTERNALFORMAT(GL_RGB565_OES)
			default:                              return GL_INVALID_OPERATION;
			}
			break;
		case GL_DEPTH_STENCIL_OES:
			switch(type)
			{
			case GL_UNSIGNED_INT_24_8_OES: VALIDATE_INTERNALFORMAT(GL_DEPTH24_STENCIL8_OES)
			default:                       return GL_INVALID_OPERATION;
			}
			break;
		case GL_LUMINANCE_ALPHA:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  VALIDATE_INTERNALFORMAT(GL_LUMINANCE8_ALPHA8_EXT)
			default:
				return GL_INVALID_OPERATION;
			}
			break;
		case GL_LUMINANCE:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  VALIDATE_INTERNALFORMAT(GL_LUMINANCE8_EXT)
			default:
				return GL_INVALID_OPERATION;
			}
			break;
		case GL_ALPHA:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  VALIDATE_INTERNALFORMAT(GL_ALPHA8_EXT)
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

	bool IsColorRenderable(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_RGBA4_OES:
		case GL_RGB5_A1_OES:
		case GL_RGB565_OES:
		case GL_RGB8_OES:
		case GL_RGBA8_OES:
			return true;
		case GL_DEPTH_COMPONENT16_OES:
		case GL_STENCIL_INDEX8_OES:
		case GL_DEPTH24_STENCIL8_OES:
			return false;
		default:
			UNIMPLEMENTED();
		}

		return false;
	}

	bool IsDepthRenderable(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_DEPTH_COMPONENT16_OES:
		case GL_DEPTH24_STENCIL8_OES:
			return true;
		case GL_STENCIL_INDEX8_OES:
		case GL_RGBA4_OES:
		case GL_RGB5_A1_OES:
		case GL_RGB565_OES:
		case GL_RGB8_OES:
		case GL_RGBA8_OES:
			return false;
		default:
			UNIMPLEMENTED();
		}

		return false;
	}

	bool IsStencilRenderable(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_STENCIL_INDEX8_OES:
		case GL_DEPTH24_STENCIL8_OES:
			return true;
		case GL_RGBA4_OES:
		case GL_RGB5_A1_OES:
		case GL_RGB565_OES:
		case GL_RGB8_OES:
		case GL_RGBA8_OES:
		case GL_DEPTH_COMPONENT16_OES:
			return false;
		default:
			UNIMPLEMENTED();
		}

		return false;
	}

	GLuint GetAlphaSize(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_NONE_OES:    return 0;
		case GL_RGBA4_OES:   return 4;
		case GL_RGB5_A1_OES: return 1;
		case GL_RGB565_OES:  return 0;
		case GL_RGB8_OES:    return 0;
		case GL_RGBA8_OES:   return 8;
		case GL_BGRA8_EXT:   return 8;
		default:
		//	UNREACHABLE(internalformat);
			return 0;
		}
	}

	GLuint GetRedSize(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_NONE_OES:    return 0;
		case GL_RGBA4_OES:   return 4;
		case GL_RGB5_A1_OES: return 5;
		case GL_RGB565_OES:  return 5;
		case GL_RGB8_OES:    return 8;
		case GL_RGBA8_OES:   return 8;
		case GL_BGRA8_EXT:   return 8;
		default:
		//	UNREACHABLE(internalformat);
			return 0;
		}
	}

	GLuint GetGreenSize(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_NONE_OES:    return 0;
		case GL_RGBA4_OES:   return 4;
		case GL_RGB5_A1_OES: return 5;
		case GL_RGB565_OES:  return 6;
		case GL_RGB8_OES:    return 8;
		case GL_RGBA8_OES:   return 8;
		case GL_BGRA8_EXT:   return 8;
		default:
		//	UNREACHABLE(internalformat);
			return 0;
		}
	}

	GLuint GetBlueSize(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_NONE_OES:    return 0;
		case GL_RGBA4_OES:   return 4;
		case GL_RGB5_A1_OES: return 5;
		case GL_RGB565_OES:  return 5;
		case GL_RGB8_OES:    return 8;
		case GL_RGBA8_OES:   return 8;
		case GL_BGRA8_EXT:   return 8;
		default:
		//	UNREACHABLE(internalformat);
			return 0;
		}
	}

	GLuint GetDepthSize(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_STENCIL_INDEX8_OES:    return 0;
		case GL_DEPTH_COMPONENT16_OES: return 16;
		case GL_DEPTH24_STENCIL8_OES:  return 24;
		default:
		//	UNREACHABLE(internalformat);
			return 0;
		}
	}

	GLuint GetStencilSize(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_STENCIL_INDEX8_OES:    return 8;
		case GL_DEPTH_COMPONENT16_OES: return 0;
		case GL_DEPTH24_STENCIL8_OES:  return 8;
		default:
		//	UNREACHABLE(internalformat);
			return 0;
		}
	}

	bool IsAlpha(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_ALPHA8_EXT:
			return true;
		default:
			return false;
		}
	}

	bool IsRGB(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_LUMINANCE8_EXT:
		case GL_RGB565_OES:
		case GL_RGB8_OES:
		case SW_YV12_BT601:
		case SW_YV12_BT709:
		case SW_YV12_JFIF:
			return true;
		default:
			return false;
		}
	}

	bool IsRGBA(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_LUMINANCE8_ALPHA8_EXT:
		case GL_RGBA:
		case GL_BGRA8_EXT:     // GL_EXT_texture_format_BGRA8888
		case GL_RGBA4_OES:
		case GL_RGB5_A1_OES:
		case GL_RGBA8_OES:
			return true;
		default:
			return false;
		}
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

	sw::AlphaCompareMode ConvertAlphaComparison(GLenum comparison)
	{
		switch(comparison)
		{
		case GL_NEVER:    return sw::ALPHA_NEVER;
		case GL_ALWAYS:   return sw::ALPHA_ALWAYS;
		case GL_LESS:     return sw::ALPHA_LESS;
		case GL_LEQUAL:   return sw::ALPHA_LESSEQUAL;
		case GL_EQUAL:    return sw::ALPHA_EQUAL;
		case GL_GREATER:  return sw::ALPHA_GREATER;
		case GL_GEQUAL:   return sw::ALPHA_GREATEREQUAL;
		case GL_NOTEQUAL: return sw::ALPHA_NOTEQUAL;
		default: UNREACHABLE(comparison);
		}

		return sw::ALPHA_ALWAYS;
	}

	sw::Color<float> ConvertColor(es1::Color color)
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
		case GL_SRC_ALPHA_SATURATE:       return sw::BLEND_SRCALPHASAT;
		default: UNREACHABLE(blend);
		}

		return sw::BLEND_ZERO;
	}

	sw::BlendOperation ConvertBlendOp(GLenum blendOp)
	{
		switch(blendOp)
		{
		case GL_FUNC_ADD_OES:              return sw::BLENDOP_ADD;
		case GL_FUNC_SUBTRACT_OES:         return sw::BLENDOP_SUB;
		case GL_FUNC_REVERSE_SUBTRACT_OES: return sw::BLENDOP_INVSUB;
		case GL_MIN_EXT:                   return sw::BLENDOP_MIN;
		case GL_MAX_EXT:                   return sw::BLENDOP_MAX;
		default: UNREACHABLE(blendOp);
		}

		return sw::BLENDOP_ADD;
	}

	sw::LogicalOperation ConvertLogicalOperation(GLenum logicalOperation)
	{
		switch(logicalOperation)
		{
		case GL_CLEAR:         return sw::LOGICALOP_CLEAR;
		case GL_SET:           return sw::LOGICALOP_SET;
		case GL_COPY:          return sw::LOGICALOP_COPY;
		case GL_COPY_INVERTED: return sw::LOGICALOP_COPY_INVERTED;
		case GL_NOOP:          return sw::LOGICALOP_NOOP;
		case GL_INVERT:        return sw::LOGICALOP_INVERT;
		case GL_AND:           return sw::LOGICALOP_AND;
		case GL_NAND:          return sw::LOGICALOP_NAND;
		case GL_OR:            return sw::LOGICALOP_OR;
		case GL_NOR:           return sw::LOGICALOP_NOR;
		case GL_XOR:           return sw::LOGICALOP_XOR;
		case GL_EQUIV:         return sw::LOGICALOP_EQUIV;
		case GL_AND_REVERSE:   return sw::LOGICALOP_AND_REVERSE;
		case GL_AND_INVERTED:  return sw::LOGICALOP_AND_INVERTED;
		case GL_OR_REVERSE:    return sw::LOGICALOP_OR_REVERSE;
		case GL_OR_INVERTED:   return sw::LOGICALOP_OR_INVERTED;
		default: UNREACHABLE(logicalOperation);
		}

		return sw::LOGICALOP_COPY;
	}

	sw::StencilOperation ConvertStencilOp(GLenum stencilOp)
	{
		switch(stencilOp)
		{
		case GL_ZERO:          return sw::OPERATION_ZERO;
		case GL_KEEP:          return sw::OPERATION_KEEP;
		case GL_REPLACE:       return sw::OPERATION_REPLACE;
		case GL_INCR:          return sw::OPERATION_INCRSAT;
		case GL_DECR:          return sw::OPERATION_DECRSAT;
		case GL_INVERT:        return sw::OPERATION_INVERT;
		case GL_INCR_WRAP_OES: return sw::OPERATION_INCR;
		case GL_DECR_WRAP_OES: return sw::OPERATION_DECR;
		default: UNREACHABLE(stencilOp);
		}

		return sw::OPERATION_KEEP;
	}

	sw::AddressingMode ConvertTextureWrap(GLenum wrap)
	{
		switch(wrap)
		{
		case GL_REPEAT:              return sw::ADDRESSING_WRAP;
		case GL_CLAMP_TO_EDGE:       return sw::ADDRESSING_CLAMP;
		case GL_MIRRORED_REPEAT_OES: return sw::ADDRESSING_MIRROR;
		default: UNREACHABLE(wrap);
		}

		return sw::ADDRESSING_WRAP;
	}

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
			break;
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_NEAREST:
			return sw::MIPMAP_POINT;
			break;
		case GL_NEAREST_MIPMAP_LINEAR:
		case GL_LINEAR_MIPMAP_LINEAR:
			return sw::MIPMAP_LINEAR;
			break;
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

		sw::FilterType magFilterType = sw::FILTER_POINT;
		switch(magFilter)
		{
		case GL_NEAREST: magFilterType = sw::FILTER_POINT;  break;
		case GL_LINEAR:  magFilterType = sw::FILTER_LINEAR; break;
		default: UNREACHABLE(magFilter);
		}

		switch(minFilter)
		{
		case GL_NEAREST:
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_NEAREST_MIPMAP_LINEAR:
			return (magFilterType == sw::FILTER_POINT) ? sw::FILTER_POINT : sw::FILTER_MIN_POINT_MAG_LINEAR;
		case GL_LINEAR:
		case GL_LINEAR_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_LINEAR:
			return (magFilterType == sw::FILTER_POINT) ? sw::FILTER_MIN_LINEAR_MAG_POINT : sw::FILTER_LINEAR;
		default:
			UNREACHABLE(minFilter);
			return (magFilterType == sw::FILTER_POINT) ? sw::FILTER_POINT : sw::FILTER_MIN_POINT_MAG_LINEAR;
		}
	}

	bool ConvertPrimitiveType(GLenum primitiveType, GLsizei elementCount, GLenum elementType,  sw::DrawType &drawType, int &primitiveCount)
	{
		switch(primitiveType)
		{
		case GL_POINTS:
			drawType = sw::DRAW_POINTLIST;
			primitiveCount = elementCount;
			break;
		case GL_LINES:
			drawType = sw::DRAW_LINELIST;
			primitiveCount = elementCount / 2;
			break;
		case GL_LINE_LOOP:
			drawType = sw::DRAW_LINELOOP;
			primitiveCount = elementCount;
			break;
		case GL_LINE_STRIP:
			drawType = sw::DRAW_LINESTRIP;
			primitiveCount = elementCount - 1;
			break;
		case GL_TRIANGLES:
			drawType = sw::DRAW_TRIANGLELIST;
			primitiveCount = elementCount / 3;
			break;
		case GL_TRIANGLE_STRIP:
			drawType = sw::DRAW_TRIANGLESTRIP;
			primitiveCount = elementCount - 2;
			break;
		case GL_TRIANGLE_FAN:
			drawType = sw::DRAW_TRIANGLEFAN;
			primitiveCount = elementCount - 2;
			break;
		default:
			return false;
		}

		sw::DrawType elementSize;
		switch(elementType)
		{
		case GL_NONE_OES:       elementSize = sw::DRAW_NONINDEXED; break;
		case GL_UNSIGNED_BYTE:  elementSize = sw::DRAW_INDEXED8;   break;
		case GL_UNSIGNED_SHORT: elementSize = sw::DRAW_INDEXED16;  break;
		case GL_UNSIGNED_INT:   elementSize = sw::DRAW_INDEXED32;  break;
		default: return false;
		}

		drawType = sw::DrawType(drawType | elementSize);

		return true;
	}

	sw::TextureStage::StageOperation ConvertCombineOperation(GLenum operation)
	{
		switch(operation)
		{
		case GL_REPLACE:        return sw::TextureStage::STAGE_SELECTARG1;
		case GL_MODULATE:       return sw::TextureStage::STAGE_MODULATE;
		case GL_ADD:            return sw::TextureStage::STAGE_ADD;
		case GL_ADD_SIGNED:     return sw::TextureStage::STAGE_ADDSIGNED;
		case GL_INTERPOLATE:    return sw::TextureStage::STAGE_LERP;
		case GL_SUBTRACT:       return sw::TextureStage::STAGE_SUBTRACT;
		case GL_DOT3_RGB:       return sw::TextureStage::STAGE_DOT3;
		case GL_DOT3_RGBA:      return sw::TextureStage::STAGE_DOT3;
		default: UNREACHABLE(operation); return sw::TextureStage::STAGE_SELECTARG1;
		}
	}

	sw::TextureStage::SourceArgument ConvertSourceArgument(GLenum argument)
	{
		switch(argument)
		{
		case GL_TEXTURE:        return sw::TextureStage::SOURCE_TEXTURE;
		case GL_CONSTANT:       return sw::TextureStage::SOURCE_CONSTANT;
		case GL_PRIMARY_COLOR:  return sw::TextureStage::SOURCE_DIFFUSE;
		case GL_PREVIOUS:       return sw::TextureStage::SOURCE_CURRENT;
		default: UNREACHABLE(argument); return sw::TextureStage::SOURCE_CURRENT;
		}
	}

	sw::TextureStage::ArgumentModifier ConvertSourceOperand(GLenum operand)
	{
		switch(operand)
		{
		case GL_SRC_COLOR:           return sw::TextureStage::MODIFIER_COLOR;
		case GL_ONE_MINUS_SRC_COLOR: return sw::TextureStage::MODIFIER_INVCOLOR;
		case GL_SRC_ALPHA:           return sw::TextureStage::MODIFIER_ALPHA;
		case GL_ONE_MINUS_SRC_ALPHA: return sw::TextureStage::MODIFIER_INVALPHA;
		default: UNREACHABLE(operand);      return sw::TextureStage::MODIFIER_COLOR;
		}
	}
}

namespace sw2es
{
	GLenum ConvertBackBufferFormat(sw::Format format)
	{
		switch(format)
		{
		case sw::FORMAT_A4R4G4B4: return GL_RGBA4_OES;
		case sw::FORMAT_A8R8G8B8: return GL_RGBA8_OES;
		case sw::FORMAT_A8B8G8R8: return GL_RGBA8_OES;
		case sw::FORMAT_A1R5G5B5: return GL_RGB5_A1_OES;
		case sw::FORMAT_R5G6B5:   return GL_RGB565_OES;
		case sw::FORMAT_X8R8G8B8: return GL_RGB8_OES;
		case sw::FORMAT_X8B8G8R8: return GL_RGB8_OES;
		default:
			UNREACHABLE(format);
		}

		return GL_RGBA4_OES;
	}

	GLenum ConvertDepthStencilFormat(sw::Format format)
	{
		switch(format)
		{
		case sw::FORMAT_D16:
		case sw::FORMAT_D24X8:
		case sw::FORMAT_D32:
			return GL_DEPTH_COMPONENT16_OES;
		case sw::FORMAT_D24S8:
			return GL_DEPTH24_STENCIL8_OES;
		default:
			UNREACHABLE(format);
		}

		return GL_DEPTH24_STENCIL8_OES;
	}
}
