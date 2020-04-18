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

#include "Image.hpp"

#include "../libEGL/Texture.hpp"
#include "../common/debug.h"
#include "Common/Math.hpp"
#include "Common/Thread.hpp"

#include <GLES3/gl3.h>

#include <string.h>
#include <algorithm>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <IOSurface/IOSurface.h>
#endif

namespace gl
{
	sw::Format ConvertReadFormatType(GLenum format, GLenum type)
	{
		switch(format)
		{
		case GL_LUMINANCE:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  return sw::FORMAT_L8;
			case GL_HALF_FLOAT:     return sw::FORMAT_L16F;
			case GL_HALF_FLOAT_OES: return sw::FORMAT_L16F;
			case GL_FLOAT:          return sw::FORMAT_L32F;
			default: UNREACHABLE(type);
			}
			break;
		case GL_LUMINANCE_ALPHA:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  return sw::FORMAT_A8L8;
			case GL_HALF_FLOAT:     return sw::FORMAT_A16L16F;
			case GL_HALF_FLOAT_OES: return sw::FORMAT_A16L16F;
			case GL_FLOAT:          return sw::FORMAT_A32L32F;
			default: UNREACHABLE(type);
			}
			break;
		case GL_RGBA:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:          return sw::FORMAT_A8B8G8R8;
			case GL_UNSIGNED_SHORT_4_4_4_4: return sw::FORMAT_R4G4B4A4;
			case GL_UNSIGNED_SHORT_5_5_5_1: return sw::FORMAT_R5G5B5A1;
			case GL_HALF_FLOAT:             return sw::FORMAT_A16B16G16R16F;
			case GL_HALF_FLOAT_OES:         return sw::FORMAT_A16B16G16R16F;
			case GL_FLOAT:                  return sw::FORMAT_A32B32G32R32F;
			case GL_UNSIGNED_INT_2_10_10_10_REV_EXT: return sw::FORMAT_A2B10G10R10;
			default: UNREACHABLE(type);
			}
			break;
		case GL_BGRA_EXT:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:                  return sw::FORMAT_A8R8G8B8;
			case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT: return sw::FORMAT_A4R4G4B4;
			case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT: return sw::FORMAT_A1R5G5B5;
			default: UNREACHABLE(type);
			}
			break;
		case GL_RGB:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:          return sw::FORMAT_B8G8R8;
			case GL_UNSIGNED_SHORT_5_6_5:   return sw::FORMAT_R5G6B5;
			case GL_HALF_FLOAT:             return sw::FORMAT_B16G16R16F;
			case GL_HALF_FLOAT_OES:         return sw::FORMAT_B16G16R16F;
			case GL_FLOAT:                  return sw::FORMAT_B32G32R32F;
			default: UNREACHABLE(type);
			}
			break;
		case GL_RG:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:          return sw::FORMAT_G8R8;
			case GL_HALF_FLOAT:             return sw::FORMAT_G16R16F;
			case GL_HALF_FLOAT_OES:         return sw::FORMAT_G16R16F;
			case GL_FLOAT:                  return sw::FORMAT_G32R32F;
			default: UNREACHABLE(type);
			}
			break;
		case GL_RED:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:          return sw::FORMAT_R8;
			case GL_HALF_FLOAT:             return sw::FORMAT_R16F;
			case GL_HALF_FLOAT_OES:         return sw::FORMAT_R16F;
			case GL_FLOAT:                  return sw::FORMAT_R32F;
			default: UNREACHABLE(type);
			}
			break;
		case GL_ALPHA:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:          return sw::FORMAT_A8;
			case GL_HALF_FLOAT:             return sw::FORMAT_A16F;
			case GL_HALF_FLOAT_OES:         return sw::FORMAT_A16F;
			case GL_FLOAT:                  return sw::FORMAT_A32F;
			default: UNREACHABLE(type);
			}
			break;
		case GL_RED_INTEGER:
			switch(type)
			{
			case GL_INT:          return sw::FORMAT_R32I;
			case GL_UNSIGNED_INT: return sw::FORMAT_R32UI;
			default: UNREACHABLE(type);
			}
			break;
		case GL_RG_INTEGER:
			switch(type)
			{
			case GL_INT:          return sw::FORMAT_G32R32I;
			case GL_UNSIGNED_INT: return sw::FORMAT_G32R32UI;
			default: UNREACHABLE(type);
			}
			break;
		case GL_RGB_INTEGER:
			switch(type)
			{
			case GL_INT:          return sw::FORMAT_X32B32G32R32I;
			case GL_UNSIGNED_INT: return sw::FORMAT_X32B32G32R32UI;
			default: UNREACHABLE(type);
			}
			break;
		case GL_RGBA_INTEGER:
			switch(type)
			{
			case GL_INT:          return sw::FORMAT_A32B32G32R32I;
			case GL_UNSIGNED_INT: return sw::FORMAT_A32B32G32R32UI;
			case GL_UNSIGNED_INT_2_10_10_10_REV: return sw::FORMAT_A2B10G10R10UI;
			default: UNREACHABLE(type);
			}
			break;
		case GL_DEPTH_COMPONENT:
			switch(type)
			{
			case GL_UNSIGNED_SHORT:        return sw::FORMAT_D16;
			case GL_UNSIGNED_INT_24_8_OES: return sw::FORMAT_D24S8;
			case GL_UNSIGNED_INT:          return sw::FORMAT_D32;
			case GL_FLOAT:                 return sw::FORMAT_D32F_LOCKABLE;
			default: UNREACHABLE(type);
			}
			break;
		default:
			UNREACHABLE(format);
			break;
		}

		return sw::FORMAT_NULL;
	}

	bool IsUnsizedInternalFormat(GLint internalformat)
	{
		switch(internalformat)
		{
		case GL_ALPHA:
		case GL_LUMINANCE:
		case GL_LUMINANCE_ALPHA:
		case GL_RED:
		case GL_RG:
		case GL_RGB:
		case GL_RGBA:
		case GL_RED_INTEGER:
		case GL_RG_INTEGER:
		case GL_RGB_INTEGER:
		case GL_RGBA_INTEGER:
		case GL_BGRA_EXT:
		case GL_DEPTH_COMPONENT:
		case GL_DEPTH_STENCIL:
		// GL_EXT_sRGB
	//	case GL_SRGB_EXT:
	//	case GL_SRGB_ALPHA_EXT:
			return true;
		default:
			return false;
		}
	}

	GLenum GetBaseInternalFormat(GLint internalformat)
	{
		switch(internalformat)
		{
		// [OpenGL ES 3.0 Table 3.13]
		case GL_R8:       return GL_RED;
		case GL_R8_SNORM: return GL_RED;
		case GL_RG8:       return GL_RG;
		case GL_RG8_SNORM: return GL_RG;
		case GL_RGB8:       return GL_RGB;
		case GL_RGB8_SNORM: return GL_RGB;
		case GL_RGB565:     return GL_RGB;
		case GL_RGBA4:        return GL_RGBA;
		case GL_RGB5_A1:      return GL_RGBA;
		case GL_RGBA8:        return GL_RGBA;
		case GL_RGBA8_SNORM:  return GL_RGBA;
		case GL_RGB10_A2:     return GL_RGBA;
		case GL_RGB10_A2UI:   return GL_RGBA;
		case GL_SRGB8:        return GL_RGB;
		case GL_SRGB8_ALPHA8: return GL_RGBA;
		case GL_R16F:    return GL_RED;
		case GL_RG16F:   return GL_RG;
		case GL_RGB16F:  return GL_RGB;
		case GL_RGBA16F: return GL_RGBA;
		case GL_R32F:    return GL_RED;
		case GL_RG32F:   return GL_RG;
		case GL_RGB32F:  return GL_RGB;
		case GL_RGBA32F: return GL_RGBA;
		case GL_R11F_G11F_B10F: return GL_RGB;
		case GL_RGB9_E5:        return GL_RGB;
		case GL_R8I:      return GL_RED;
		case GL_R8UI:     return GL_RED;
		case GL_R16I:     return GL_RED;
		case GL_R16UI:    return GL_RED;
		case GL_R32I:     return GL_RED;
		case GL_R32UI:    return GL_RED;
		case GL_RG8I:     return GL_RG;
		case GL_RG8UI:    return GL_RG;
		case GL_RG16I:    return GL_RG;
		case GL_RG16UI:   return GL_RG;
		case GL_RG32I:    return GL_RG;
		case GL_RG32UI:   return GL_RG;
		case GL_RGB8I:    return GL_RGB;
		case GL_RGB8UI:   return GL_RGB;
		case GL_RGB16I:   return GL_RGB;
		case GL_RGB16UI:  return GL_RGB;
		case GL_RGB32I:   return GL_RGB;
		case GL_RGB32UI:  return GL_RGB;
		case GL_RGBA8I:   return GL_RGBA;
		case GL_RGBA8UI:  return GL_RGBA;
		case GL_RGBA16I:  return GL_RGBA;
		case GL_RGBA16UI: return GL_RGBA;
		case GL_RGBA32I:  return GL_RGBA;
		case GL_RGBA32UI: return GL_RGBA;

		// GL_EXT_texture_storage
		case GL_ALPHA8_EXT:             return GL_ALPHA;
		case GL_LUMINANCE8_EXT:         return GL_LUMINANCE;
		case GL_LUMINANCE8_ALPHA8_EXT:  return GL_LUMINANCE_ALPHA;
		case GL_ALPHA32F_EXT:           return GL_ALPHA;
		case GL_LUMINANCE32F_EXT:       return GL_LUMINANCE;
		case GL_LUMINANCE_ALPHA32F_EXT: return GL_LUMINANCE_ALPHA;
		case GL_ALPHA16F_EXT:           return GL_ALPHA;
		case GL_LUMINANCE16F_EXT:       return GL_LUMINANCE;
		case GL_LUMINANCE_ALPHA16F_EXT: return GL_LUMINANCE_ALPHA;

		case GL_BGRA8_EXT: return GL_BGRA_EXT;   // GL_APPLE_texture_format_BGRA8888

		case GL_DEPTH_COMPONENT24:     return GL_DEPTH_COMPONENT;
		case GL_DEPTH_COMPONENT32_OES: return GL_DEPTH_COMPONENT;
		case GL_DEPTH_COMPONENT32F:    return GL_DEPTH_COMPONENT;
		case GL_DEPTH_COMPONENT16:     return GL_DEPTH_COMPONENT;
		case GL_DEPTH32F_STENCIL8:     return GL_DEPTH_STENCIL;
		case GL_DEPTH24_STENCIL8:      return GL_DEPTH_STENCIL;
		case GL_STENCIL_INDEX8:        return GL_STENCIL_INDEX_OES;
		default:
			UNREACHABLE(internalformat);
			break;
		}

		return GL_NONE;
	}

	GLint GetSizedInternalFormat(GLint internalformat, GLenum type)
	{
		if(!IsUnsizedInternalFormat(internalformat))
		{
			return internalformat;
		}

		switch(internalformat)
		{
		case GL_RGBA:
			switch(type)
			{
			case GL_UNSIGNED_BYTE: return GL_RGBA8;
			case GL_BYTE:          return GL_RGBA8_SNORM;
			case GL_UNSIGNED_SHORT_4_4_4_4:      return GL_RGBA4;
			case GL_UNSIGNED_SHORT_5_5_5_1:      return GL_RGB5_A1;
			case GL_UNSIGNED_INT_2_10_10_10_REV: return GL_RGB10_A2;
			case GL_FLOAT:          return GL_RGBA32F;
			case GL_HALF_FLOAT:     return GL_RGBA16F;
			case GL_HALF_FLOAT_OES: return GL_RGBA16F;
			default: UNREACHABLE(type); return GL_NONE;
			}
		case GL_RGBA_INTEGER:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  return GL_RGBA8UI;
			case GL_BYTE:           return GL_RGBA8I;
			case GL_UNSIGNED_SHORT: return GL_RGBA16UI;
			case GL_SHORT:          return GL_RGBA16I;
			case GL_UNSIGNED_INT:   return GL_RGBA32UI;
			case GL_INT:            return GL_RGBA32I;
			case GL_UNSIGNED_INT_2_10_10_10_REV: return GL_RGB10_A2UI;
			default: UNREACHABLE(type); return GL_NONE;
			}
		case GL_RGB:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  return GL_RGB8;
			case GL_BYTE:           return GL_RGB8_SNORM;
			case GL_UNSIGNED_SHORT_5_6_5:         return GL_RGB565;
			case GL_UNSIGNED_INT_10F_11F_11F_REV: return GL_R11F_G11F_B10F;
			case GL_UNSIGNED_INT_5_9_9_9_REV:     return GL_RGB9_E5;
			case GL_FLOAT:          return GL_RGB32F;
			case GL_HALF_FLOAT:     return GL_RGB16F;
			case GL_HALF_FLOAT_OES: return GL_RGB16F;
			default: UNREACHABLE(type); return GL_NONE;
			}
		case GL_RGB_INTEGER:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  return GL_RGB8UI;
			case GL_BYTE:           return GL_RGB8I;
			case GL_UNSIGNED_SHORT: return GL_RGB16UI;
			case GL_SHORT:          return GL_RGB16I;
			case GL_UNSIGNED_INT:   return GL_RGB32UI;
			case GL_INT:            return GL_RGB32I;
			default: UNREACHABLE(type); return GL_NONE;
			}
		case GL_RG:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  return GL_RG8;
			case GL_BYTE:           return GL_RG8_SNORM;
			case GL_FLOAT:          return GL_RG32F;
			case GL_HALF_FLOAT:     return GL_RG16F;
			case GL_HALF_FLOAT_OES: return GL_RG16F;
			default: UNREACHABLE(type); return GL_NONE;
			}
		case GL_RG_INTEGER:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  return GL_RG8UI;
			case GL_BYTE:           return GL_RG8I;
			case GL_UNSIGNED_SHORT: return GL_RG16UI;
			case GL_SHORT:          return GL_RG16I;
			case GL_UNSIGNED_INT:   return GL_RG32UI;
			case GL_INT:            return GL_RG32I;
			default: UNREACHABLE(type); return GL_NONE;
			}
		case GL_RED:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  return GL_R8;
			case GL_BYTE:           return GL_R8_SNORM;
			case GL_FLOAT:          return GL_R32F;
			case GL_HALF_FLOAT:     return GL_R16F;
			case GL_HALF_FLOAT_OES: return GL_R16F;
			default: UNREACHABLE(type); return GL_NONE;
			}
		case GL_RED_INTEGER:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  return GL_R8UI;
			case GL_BYTE:           return GL_R8I;
			case GL_UNSIGNED_SHORT: return GL_R16UI;
			case GL_SHORT:          return GL_R16I;
			case GL_UNSIGNED_INT:   return GL_R32UI;
			case GL_INT:            return GL_R32I;
			default: UNREACHABLE(type); return GL_NONE;
			}
		case GL_LUMINANCE_ALPHA:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  return GL_LUMINANCE8_ALPHA8_EXT;
			case GL_FLOAT:          return GL_LUMINANCE_ALPHA32F_EXT;
			case GL_HALF_FLOAT:     return GL_LUMINANCE_ALPHA16F_EXT;
			case GL_HALF_FLOAT_OES: return GL_LUMINANCE_ALPHA16F_EXT;
			default: UNREACHABLE(type); return GL_NONE;
			}
		case GL_LUMINANCE:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  return GL_LUMINANCE8_EXT;
			case GL_FLOAT:          return GL_LUMINANCE32F_EXT;
			case GL_HALF_FLOAT:     return GL_LUMINANCE16F_EXT;
			case GL_HALF_FLOAT_OES: return GL_LUMINANCE16F_EXT;
			default: UNREACHABLE(type); return GL_NONE;
			}
		case GL_ALPHA:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:  return GL_ALPHA8_EXT;
			case GL_FLOAT:          return GL_ALPHA32F_EXT;
			case GL_HALF_FLOAT:     return GL_ALPHA16F_EXT;
			case GL_HALF_FLOAT_OES: return GL_ALPHA16F_EXT;
			default: UNREACHABLE(type); return GL_NONE;
			}
		case GL_BGRA_EXT:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:                  return GL_BGRA8_EXT;
			case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT: // Only valid for glReadPixels calls.
			case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT: // Only valid for glReadPixels calls.
			default: UNREACHABLE(type); return GL_NONE;
			}
		case GL_DEPTH_COMPONENT:
			switch(type)
			{
			case GL_UNSIGNED_SHORT: return GL_DEPTH_COMPONENT16;
			case GL_UNSIGNED_INT:   return GL_DEPTH_COMPONENT32_OES;
			case GL_FLOAT:          return GL_DEPTH_COMPONENT32F;
			default: UNREACHABLE(type); return GL_NONE;
			}
		case GL_DEPTH_STENCIL:
			switch(type)
			{
			case GL_UNSIGNED_INT_24_8:              return GL_DEPTH24_STENCIL8;
			case GL_FLOAT_32_UNSIGNED_INT_24_8_REV: return GL_DEPTH32F_STENCIL8;
			default: UNREACHABLE(type); return GL_NONE;
			}

		// GL_OES_texture_stencil8
	//	case GL_STENCIL_INDEX_OES / GL_UNSIGNED_BYTE: return GL_STENCIL_INDEX8;

		// GL_EXT_sRGB
	//	case GL_SRGB_EXT / GL_UNSIGNED_BYTE: return GL_SRGB8;
	//	case GL_SRGB_ALPHA_EXT / GL_UNSIGNED_BYTE: return GL_SRGB8_ALPHA8;

		default:
			UNREACHABLE(internalformat);
		}

		return GL_NONE;
	}

	sw::Format SelectInternalFormat(GLint format)
	{
		switch(format)
		{
		case GL_RGBA4:   return sw::FORMAT_A8B8G8R8;
		case GL_RGB5_A1: return sw::FORMAT_A8B8G8R8;
		case GL_RGBA8:   return sw::FORMAT_A8B8G8R8;
		case GL_RGB565:  return sw::FORMAT_R5G6B5;
		case GL_RGB8:    return sw::FORMAT_X8B8G8R8;

		case GL_DEPTH_COMPONENT32F:    return sw::FORMAT_D32F_LOCKABLE;
		case GL_DEPTH_COMPONENT16:     return sw::FORMAT_D32F_LOCKABLE;
		case GL_DEPTH_COMPONENT24:     return sw::FORMAT_D32F_LOCKABLE;
		case GL_DEPTH_COMPONENT32_OES: return sw::FORMAT_D32F_LOCKABLE;
		case GL_DEPTH24_STENCIL8:      return sw::FORMAT_D32FS8_TEXTURE;
		case GL_DEPTH32F_STENCIL8:     return sw::FORMAT_D32FS8_TEXTURE;
		case GL_STENCIL_INDEX8:        return sw::FORMAT_S8;

		case GL_R8:             return sw::FORMAT_R8;
		case GL_RG8:            return sw::FORMAT_G8R8;
		case GL_R8I:            return sw::FORMAT_R8I;
		case GL_RG8I:           return sw::FORMAT_G8R8I;
		case GL_RGB8I:          return sw::FORMAT_X8B8G8R8I;
		case GL_RGBA8I:         return sw::FORMAT_A8B8G8R8I;
		case GL_R8UI:           return sw::FORMAT_R8UI;
		case GL_RG8UI:          return sw::FORMAT_G8R8UI;
		case GL_RGB8UI:         return sw::FORMAT_X8B8G8R8UI;
		case GL_RGBA8UI:        return sw::FORMAT_A8B8G8R8UI;
		case GL_R16I:           return sw::FORMAT_R16I;
		case GL_RG16I:          return sw::FORMAT_G16R16I;
		case GL_RGB16I:         return sw::FORMAT_X16B16G16R16I;
		case GL_RGBA16I:        return sw::FORMAT_A16B16G16R16I;
		case GL_R16UI:          return sw::FORMAT_R16UI;
		case GL_RG16UI:         return sw::FORMAT_G16R16UI;
		case GL_RGB16UI:        return sw::FORMAT_X16B16G16R16UI;
		case GL_RGBA16UI:       return sw::FORMAT_A16B16G16R16UI;
		case GL_R32I:           return sw::FORMAT_R32I;
		case GL_RG32I:          return sw::FORMAT_G32R32I;
		case GL_RGB32I:         return sw::FORMAT_X32B32G32R32I;
		case GL_RGBA32I:        return sw::FORMAT_A32B32G32R32I;
		case GL_R32UI:          return sw::FORMAT_R32UI;
		case GL_RG32UI:         return sw::FORMAT_G32R32UI;
		case GL_RGB32UI:        return sw::FORMAT_X32B32G32R32UI;
		case GL_RGBA32UI:       return sw::FORMAT_A32B32G32R32UI;
		case GL_R16F:           return sw::FORMAT_R16F;
		case GL_RG16F:          return sw::FORMAT_G16R16F;
		case GL_R11F_G11F_B10F: return sw::FORMAT_X16B16G16R16F_UNSIGNED;
		case GL_RGB16F:         return sw::FORMAT_X16B16G16R16F;
		case GL_RGBA16F:        return sw::FORMAT_A16B16G16R16F;
		case GL_R32F:           return sw::FORMAT_R32F;
		case GL_RG32F:          return sw::FORMAT_G32R32F;
		case GL_RGB32F:         return sw::FORMAT_X32B32G32R32F;
		case GL_RGBA32F:        return sw::FORMAT_A32B32G32R32F;
		case GL_RGB10_A2:       return sw::FORMAT_A2B10G10R10;
		case GL_RGB10_A2UI:     return sw::FORMAT_A2B10G10R10UI;
		case GL_SRGB8:          return sw::FORMAT_SRGB8_X8;
		case GL_SRGB8_ALPHA8:   return sw::FORMAT_SRGB8_A8;

		case GL_ETC1_RGB8_OES:              return sw::FORMAT_ETC1;
		case GL_COMPRESSED_R11_EAC:         return sw::FORMAT_R11_EAC;
		case GL_COMPRESSED_SIGNED_R11_EAC:  return sw::FORMAT_SIGNED_R11_EAC;
		case GL_COMPRESSED_RG11_EAC:        return sw::FORMAT_RG11_EAC;
		case GL_COMPRESSED_SIGNED_RG11_EAC: return sw::FORMAT_SIGNED_RG11_EAC;
		case GL_COMPRESSED_RGB8_ETC2:       return sw::FORMAT_RGB8_ETC2;
		case GL_COMPRESSED_SRGB8_ETC2:      return sw::FORMAT_SRGB8_ETC2;
		case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:  return sw::FORMAT_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
		case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2: return sw::FORMAT_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2;
		case GL_COMPRESSED_RGBA8_ETC2_EAC:        return sw::FORMAT_RGBA8_ETC2_EAC;
		case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC: return sw::FORMAT_SRGB8_ALPHA8_ETC2_EAC;
		case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:   return sw::FORMAT_RGBA_ASTC_4x4_KHR;
		case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:   return sw::FORMAT_RGBA_ASTC_5x4_KHR;
		case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:   return sw::FORMAT_RGBA_ASTC_5x5_KHR;
		case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:   return sw::FORMAT_RGBA_ASTC_6x5_KHR;
		case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:   return sw::FORMAT_RGBA_ASTC_6x6_KHR;
		case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:   return sw::FORMAT_RGBA_ASTC_8x5_KHR;
		case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:   return sw::FORMAT_RGBA_ASTC_8x6_KHR;
		case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:   return sw::FORMAT_RGBA_ASTC_8x8_KHR;
		case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:  return sw::FORMAT_RGBA_ASTC_10x5_KHR;
		case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:  return sw::FORMAT_RGBA_ASTC_10x6_KHR;
		case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:  return sw::FORMAT_RGBA_ASTC_10x8_KHR;
		case GL_COMPRESSED_RGBA_ASTC_10x10_KHR: return sw::FORMAT_RGBA_ASTC_10x10_KHR;
		case GL_COMPRESSED_RGBA_ASTC_12x10_KHR: return sw::FORMAT_RGBA_ASTC_12x10_KHR;
		case GL_COMPRESSED_RGBA_ASTC_12x12_KHR: return sw::FORMAT_RGBA_ASTC_12x12_KHR;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:   return sw::FORMAT_SRGB8_ALPHA8_ASTC_4x4_KHR;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:   return sw::FORMAT_SRGB8_ALPHA8_ASTC_5x4_KHR;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:   return sw::FORMAT_SRGB8_ALPHA8_ASTC_5x5_KHR;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:   return sw::FORMAT_SRGB8_ALPHA8_ASTC_6x5_KHR;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:   return sw::FORMAT_SRGB8_ALPHA8_ASTC_6x6_KHR;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:   return sw::FORMAT_SRGB8_ALPHA8_ASTC_8x5_KHR;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:   return sw::FORMAT_SRGB8_ALPHA8_ASTC_8x6_KHR;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:   return sw::FORMAT_SRGB8_ALPHA8_ASTC_8x8_KHR;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:  return sw::FORMAT_SRGB8_ALPHA8_ASTC_10x5_KHR;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:  return sw::FORMAT_SRGB8_ALPHA8_ASTC_10x6_KHR;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:  return sw::FORMAT_SRGB8_ALPHA8_ASTC_10x8_KHR;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR: return sw::FORMAT_SRGB8_ALPHA8_ASTC_10x10_KHR;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR: return sw::FORMAT_SRGB8_ALPHA8_ASTC_12x10_KHR;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR: return sw::FORMAT_SRGB8_ALPHA8_ASTC_12x12_KHR;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:    return sw::FORMAT_DXT1;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:   return sw::FORMAT_DXT1;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE: return sw::FORMAT_DXT3;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE: return sw::FORMAT_DXT5;

		case GL_ALPHA32F_EXT:           return sw::FORMAT_A32F;
		case GL_LUMINANCE32F_EXT:       return sw::FORMAT_L32F;
		case GL_LUMINANCE_ALPHA32F_EXT: return sw::FORMAT_A32L32F;
		case GL_RGB9_E5:                return sw::FORMAT_X16B16G16R16F_UNSIGNED;
		case GL_ALPHA16F_EXT:           return sw::FORMAT_A16F;
		case GL_LUMINANCE16F_EXT:       return sw::FORMAT_L16F;
		case GL_LUMINANCE_ALPHA16F_EXT: return sw::FORMAT_A16L16F;
		case GL_R8_SNORM:    return sw::FORMAT_R8_SNORM;
		case GL_RG8_SNORM:   return sw::FORMAT_G8R8_SNORM;
		case GL_RGB8_SNORM:  return sw::FORMAT_X8B8G8R8_SNORM;
		case GL_RGBA8_SNORM: return sw::FORMAT_A8B8G8R8_SNORM;
		case GL_LUMINANCE8_EXT:        return sw::FORMAT_L8;
		case GL_LUMINANCE8_ALPHA8_EXT: return sw::FORMAT_A8L8;
		case GL_BGRA8_EXT:  return sw::FORMAT_A8R8G8B8;
		case GL_ALPHA8_EXT: return sw::FORMAT_A8;

		case SW_YV12_BT601: return sw::FORMAT_YV12_BT601;
		case SW_YV12_BT709: return sw::FORMAT_YV12_BT709;
		case SW_YV12_JFIF:  return sw::FORMAT_YV12_JFIF;

		default:
			UNREACHABLE(format);   // Not a sized internal format.
			return sw::FORMAT_NULL;
		}
	}

	// Returns the size, in bytes, of a single client-side pixel.
    // OpenGL ES 3.0.5 table 3.2.
	static int ComputePixelSize(GLenum format, GLenum type)
	{
		switch(format)
		{
		case GL_RED:
		case GL_RED_INTEGER:
		case GL_ALPHA:
		case GL_LUMINANCE:
			switch(type)
			{
			case GL_BYTE:           return 1;
			case GL_UNSIGNED_BYTE:  return 1;
			case GL_FLOAT:          return 4;
			case GL_HALF_FLOAT:     return 2;
			case GL_HALF_FLOAT_OES: return 2;
			case GL_SHORT:          return 2;
			case GL_UNSIGNED_SHORT: return 2;
			case GL_INT:            return 4;
			case GL_UNSIGNED_INT:   return 4;
			default: UNREACHABLE(type);
			}
			break;
		case GL_RG:
		case GL_RG_INTEGER:
		case GL_LUMINANCE_ALPHA:
			switch(type)
			{
			case GL_BYTE:           return 2;
			case GL_UNSIGNED_BYTE:  return 2;
			case GL_FLOAT:          return 8;
			case GL_HALF_FLOAT:     return 4;
			case GL_HALF_FLOAT_OES: return 4;
			case GL_SHORT:          return 4;
			case GL_UNSIGNED_SHORT: return 4;
			case GL_INT:            return 8;
			case GL_UNSIGNED_INT:   return 8;
			default: UNREACHABLE(type);
			}
			break;
		case GL_RGB:
		case GL_RGB_INTEGER:
			switch(type)
			{
			case GL_BYTE:                         return 3;
			case GL_UNSIGNED_BYTE:                return 3;
			case GL_UNSIGNED_SHORT_5_6_5:         return 2;
			case GL_UNSIGNED_INT_10F_11F_11F_REV: return 4;
			case GL_UNSIGNED_INT_5_9_9_9_REV:     return 4;
			case GL_FLOAT:                        return 12;
			case GL_HALF_FLOAT:                   return 6;
			case GL_HALF_FLOAT_OES:               return 6;
			case GL_SHORT:                        return 6;
			case GL_UNSIGNED_SHORT:               return 6;
			case GL_INT:                          return 12;
			case GL_UNSIGNED_INT:                 return 12;
			default: UNREACHABLE(type);
			}
			break;
		case GL_RGBA:
		case GL_RGBA_INTEGER:
		case GL_BGRA_EXT:
			switch(type)
			{
			case GL_BYTE:                           return 4;
			case GL_UNSIGNED_BYTE:                  return 4;
			case GL_UNSIGNED_SHORT_4_4_4_4:         return 2;
			case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT: return 2;
			case GL_UNSIGNED_SHORT_5_5_5_1:         return 2;
			case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT: return 2;
			case GL_UNSIGNED_INT_2_10_10_10_REV:    return 4;
			case GL_FLOAT:                          return 16;
			case GL_HALF_FLOAT:                     return 8;
			case GL_HALF_FLOAT_OES:                 return 8;
			case GL_SHORT:                          return 8;
			case GL_UNSIGNED_SHORT:                 return 8;
			case GL_INT:                            return 16;
			case GL_UNSIGNED_INT:                   return 16;
			default: UNREACHABLE(type);
			}
			break;
		case GL_DEPTH_COMPONENT:
			switch(type)
			{
			case GL_FLOAT:          return 4;
			case GL_UNSIGNED_SHORT: return 2;
			case GL_UNSIGNED_INT:   return 4;
			default: UNREACHABLE(type);
			}
			break;
		case GL_DEPTH_STENCIL:
			switch(type)
			{
			case GL_UNSIGNED_INT_24_8:              return 4;
			case GL_FLOAT_32_UNSIGNED_INT_24_8_REV: return 8;
			default: UNREACHABLE(type);
			}
			break;
		default:
			UNREACHABLE(format);
		}

		return 0;
	}

	GLsizei ComputePitch(GLsizei width, GLenum format, GLenum type, GLint alignment)
	{
		ASSERT(alignment > 0 && sw::isPow2(alignment));

		GLsizei rawPitch = ComputePixelSize(format, type) * width;
		return (rawPitch + alignment - 1) & ~(alignment - 1);
	}

	size_t ComputePackingOffset(GLenum format, GLenum type, GLsizei width, GLsizei height, const gl::PixelStorageModes &storageModes)
	{
		GLsizei pitchB = ComputePitch(width, format, type, storageModes.alignment);
		return (storageModes.skipImages * height + storageModes.skipRows) * pitchB + storageModes.skipPixels * ComputePixelSize(format, type);
	}

	inline GLsizei ComputeCompressedPitch(GLsizei width, GLenum format)
	{
		return ComputeCompressedSize(width, 1, format);
	}

	inline int GetNumCompressedBlocks(int w, int h, int blockSizeX, int blockSizeY)
	{
		return ((w + blockSizeX - 1) / blockSizeX) * ((h + blockSizeY - 1) / blockSizeY);
	}

	GLsizei ComputeCompressedSize(GLsizei width, GLsizei height, GLenum format)
	{
		switch(format)
		{
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_ETC1_RGB8_OES:
		case GL_COMPRESSED_R11_EAC:
		case GL_COMPRESSED_SIGNED_R11_EAC:
		case GL_COMPRESSED_RGB8_ETC2:
		case GL_COMPRESSED_SRGB8_ETC2:
		case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
		case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
			return 8 * GetNumCompressedBlocks(width, height, 4, 4);
		case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
		case GL_COMPRESSED_RG11_EAC:
		case GL_COMPRESSED_SIGNED_RG11_EAC:
		case GL_COMPRESSED_RGBA8_ETC2_EAC:
		case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
		case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
			return 16 * GetNumCompressedBlocks(width, height, 4, 4);
		case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:
			return 16 * GetNumCompressedBlocks(width, height, 5, 4);
		case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:
			return 16 * GetNumCompressedBlocks(width, height, 5, 5);
		case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:
			return 16 * GetNumCompressedBlocks(width, height, 6, 5);
		case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
			return 16 * GetNumCompressedBlocks(width, height, 6, 6);
		case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:
			return 16 * GetNumCompressedBlocks(width, height, 8, 5);
		case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:
			return 16 * GetNumCompressedBlocks(width, height, 8, 6);
		case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
			return 16 * GetNumCompressedBlocks(width, height, 8, 8);
		case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:
			return 16 * GetNumCompressedBlocks(width, height, 10, 5);
		case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:
			return 16 * GetNumCompressedBlocks(width, height, 10, 6);
		case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:
			return 16 * GetNumCompressedBlocks(width, height, 10, 8);
		case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:
			return 16 * GetNumCompressedBlocks(width, height, 10, 10);
		case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:
			return 16 * GetNumCompressedBlocks(width, height, 12, 10);
		case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:
			return 16 * GetNumCompressedBlocks(width, height, 12, 12);
		default:
			UNREACHABLE(format);
			return 0;
		}
	}
}

namespace egl
{
	// We assume the data can be indexed with a signed 32-bit offset, including any padding,
	// so we must keep the image size reasonable. 1 GiB ought to be enough for anybody.
	enum { IMPLEMENTATION_MAX_IMAGE_SIZE_BYTES = 0x40000000 };

	enum TransferType
	{
		Bytes,
		RGB8toRGBX8,
		RGB16toRGBX16,
		RGB32toRGBX32,
		RGB32FtoRGBX32F,
		RGB16FtoRGBX16F,
		RGBA4toRGBA8,
		RGBA5_A1toRGBA8,
		R11G11B10FtoRGBX16F,
		RGB9_E5FtoRGBX16F,
		D16toD32F,
		D24X8toD32F,
		D32toD32F,
		D32FtoD32F_CLAMPED,
		D32FX32toD32F,
		X24S8toS8,
		X56S8toS8,
		RGBA1010102toRGBA8,
		RGB8toRGB565,
		R32FtoR16F,
		RG32FtoRG16F,
		RGB32FtoRGB16F,
		RGB32FtoRGB16F_UNSIGNED,
		RGBA32FtoRGBA16F
	};

	template<TransferType transferType>
	void TransferRow(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes);

	template<>
	void TransferRow<Bytes>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		memcpy(dest, source, width * bytes);
	}

	template<>
	void TransferRow<RGB8toRGBX8>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		unsigned char *destB = dest;

		for(int x = 0; x < width; x++)
		{
			destB[4 * x + 0] = source[x * 3 + 0];
			destB[4 * x + 1] = source[x * 3 + 1];
			destB[4 * x + 2] = source[x * 3 + 2];
			destB[4 * x + 3] = 0xFF;
		}
	}

	template<>
	void TransferRow<RGB16toRGBX16>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const unsigned short *sourceS = reinterpret_cast<const unsigned short*>(source);
		unsigned short *destS = reinterpret_cast<unsigned short*>(dest);

		for(int x = 0; x < width; x++)
		{
			destS[4 * x + 0] = sourceS[x * 3 + 0];
			destS[4 * x + 1] = sourceS[x * 3 + 1];
			destS[4 * x + 2] = sourceS[x * 3 + 2];
			destS[4 * x + 3] = 0xFFFF;
		}
	}

	template<>
	void TransferRow<RGB32toRGBX32>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const unsigned int *sourceI = reinterpret_cast<const unsigned int*>(source);
		unsigned int *destI = reinterpret_cast<unsigned int*>(dest);

		for(int x = 0; x < width; x++)
		{
			destI[4 * x + 0] = sourceI[x * 3 + 0];
			destI[4 * x + 1] = sourceI[x * 3 + 1];
			destI[4 * x + 2] = sourceI[x * 3 + 2];
			destI[4 * x + 3] = 0xFFFFFFFF;
		}
	}

	template<>
	void TransferRow<RGB32FtoRGBX32F>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const float *sourceF = reinterpret_cast<const float*>(source);
		float *destF = reinterpret_cast<float*>(dest);

		for(int x = 0; x < width; x++)
		{
			destF[4 * x + 0] = sourceF[x * 3 + 0];
			destF[4 * x + 1] = sourceF[x * 3 + 1];
			destF[4 * x + 2] = sourceF[x * 3 + 2];
			destF[4 * x + 3] = 1.0f;
		}
	}

	template<>
	void TransferRow<RGB16FtoRGBX16F>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const unsigned short *sourceH = reinterpret_cast<const unsigned short*>(source);
		unsigned short *destH = reinterpret_cast<unsigned short*>(dest);

		for(int x = 0; x < width; x++)
		{
			destH[4 * x + 0] = sourceH[x * 3 + 0];
			destH[4 * x + 1] = sourceH[x * 3 + 1];
			destH[4 * x + 2] = sourceH[x * 3 + 2];
			destH[4 * x + 3] = 0x3C00;   // SEEEEEMMMMMMMMMM, S = 0, E = 15, M = 0: 16-bit floating-point representation of 1.0
		}
	}

	template<>
	void TransferRow<RGBA4toRGBA8>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const unsigned short *source4444 = reinterpret_cast<const unsigned short*>(source);
		unsigned char *dest4444 = dest;

		for(int x = 0; x < width; x++)
		{
			unsigned short rgba = source4444[x];
			dest4444[4 * x + 0] = ((rgba & 0xF000) >> 8) | ((rgba & 0xF000) >> 12);
			dest4444[4 * x + 1] = ((rgba & 0x0F00) >> 4) | ((rgba & 0x0F00) >> 8);
			dest4444[4 * x + 2] = ((rgba & 0x00F0) << 0) | ((rgba & 0x00F0) >> 4);
			dest4444[4 * x + 3] = ((rgba & 0x000F) << 4) | ((rgba & 0x000F) >> 0);
		}
	}

	template<>
	void TransferRow<RGBA5_A1toRGBA8>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const unsigned short *source5551 = reinterpret_cast<const unsigned short*>(source);
		unsigned char *dest8888 = dest;

		for(int x = 0; x < width; x++)
		{
			unsigned short rgba = source5551[x];
			dest8888[4 * x + 0] = ((rgba & 0xF800) >> 8) | ((rgba & 0xF800) >> 13);
			dest8888[4 * x + 1] = ((rgba & 0x07C0) >> 3) | ((rgba & 0x07C0) >> 8);
			dest8888[4 * x + 2] = ((rgba & 0x003E) << 2) | ((rgba & 0x003E) >> 3);
			dest8888[4 * x + 3] = (rgba & 0x0001) ? 0xFF : 0;
		}
	}

	template<>
	void TransferRow<RGBA1010102toRGBA8>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const unsigned int *source1010102 = reinterpret_cast<const unsigned int*>(source);
		unsigned char *dest8888 = dest;

		for(int x = 0; x < width; x++)
		{
			unsigned int rgba = source1010102[x];
			dest8888[4 * x + 0] = sw::unorm<8>((rgba & 0x000003FF) * (1.0f / 0x000003FF));
			dest8888[4 * x + 1] = sw::unorm<8>((rgba & 0x000FFC00) * (1.0f / 0x000FFC00));
			dest8888[4 * x + 2] = sw::unorm<8>((rgba & 0x3FF00000) * (1.0f / 0x3FF00000));
			dest8888[4 * x + 3] = sw::unorm<8>((rgba & 0xC0000000) * (1.0f / 0xC0000000));
		}
	}

	template<>
	void TransferRow<RGB8toRGB565>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		unsigned short *dest565 = reinterpret_cast<unsigned short*>(dest);

		for(int x = 0; x < width; x++)
		{
			float r = source[3 * x + 0] * (1.0f / 0xFF);
			float g = source[3 * x + 1] * (1.0f / 0xFF);
			float b = source[3 * x + 2] * (1.0f / 0xFF);
			dest565[x] = (sw::unorm<5>(r) << 11) | (sw::unorm<6>(g) << 5) | (sw::unorm<5>(b) << 0);
		}
	}

	template<>
	void TransferRow<R11G11B10FtoRGBX16F>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const sw::R11G11B10F *sourceRGB = reinterpret_cast<const sw::R11G11B10F*>(source);
		sw::half *destF = reinterpret_cast<sw::half*>(dest);

		for(int x = 0; x < width; x++, sourceRGB++, destF += 4)
		{
			sourceRGB->toRGB16F(destF);
			destF[3] = 1.0f;
		}
	}

	template<>
	void TransferRow<RGB9_E5FtoRGBX16F>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const sw::RGB9E5 *sourceRGB = reinterpret_cast<const sw::RGB9E5*>(source);
		sw::half *destF = reinterpret_cast<sw::half*>(dest);

		for(int x = 0; x < width; x++, sourceRGB++, destF += 4)
		{
			sourceRGB->toRGB16F(destF);
			destF[3] = 1.0f;
		}
	}

	template<>
	void TransferRow<R32FtoR16F>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const float *source32F = reinterpret_cast<const float*>(source);
		sw::half *dest16F = reinterpret_cast<sw::half*>(dest);

		for(int x = 0; x < width; x++)
		{
			dest16F[x] = source32F[x];
		}
	}

	template<>
	void TransferRow<RG32FtoRG16F>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const float *source32F = reinterpret_cast<const float*>(source);
		sw::half *dest16F = reinterpret_cast<sw::half*>(dest);

		for(int x = 0; x < width; x++)
		{
			dest16F[2 * x + 0] = source32F[2 * x + 0];
			dest16F[2 * x + 1] = source32F[2 * x + 1];
		}
	}

	template<>
	void TransferRow<RGB32FtoRGB16F>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const float *source32F = reinterpret_cast<const float*>(source);
		sw::half *dest16F = reinterpret_cast<sw::half*>(dest);

		for(int x = 0; x < width; x++)
		{
			dest16F[4 * x + 0] = source32F[3 * x + 0];
			dest16F[4 * x + 1] = source32F[3 * x + 1];
			dest16F[4 * x + 2] = source32F[3 * x + 2];
			dest16F[4 * x + 3] = 1.0f;
		}
	}

	template<>
	void TransferRow<RGB32FtoRGB16F_UNSIGNED>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const float *source32F = reinterpret_cast<const float*>(source);
		sw::half *dest16F = reinterpret_cast<sw::half*>(dest);

		for(int x = 0; x < width; x++)
		{
			dest16F[4 * x + 0] = std::max(source32F[3 * x + 0], 0.0f);
			dest16F[4 * x + 1] = std::max(source32F[3 * x + 1], 0.0f);
			dest16F[4 * x + 2] = std::max(source32F[3 * x + 2], 0.0f);
			dest16F[4 * x + 3] = 1.0f;
		}
	}

	template<>
	void TransferRow<RGBA32FtoRGBA16F>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const float *source32F = reinterpret_cast<const float*>(source);
		sw::half *dest16F = reinterpret_cast<sw::half*>(dest);

		for(int x = 0; x < width; x++)
		{
			dest16F[4 * x + 0] = source32F[4 * x + 0];
			dest16F[4 * x + 1] = source32F[4 * x + 1];
			dest16F[4 * x + 2] = source32F[4 * x + 2];
			dest16F[4 * x + 3] = source32F[4 * x + 3];
		}
	}

	template<>
	void TransferRow<D16toD32F>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const unsigned short *sourceD16 = reinterpret_cast<const unsigned short*>(source);
		float *destF = reinterpret_cast<float*>(dest);

		for(int x = 0; x < width; x++)
		{
			destF[x] = (float)sourceD16[x] / 0xFFFF;
		}
	}

	template<>
	void TransferRow<D24X8toD32F>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const unsigned int *sourceD24 = reinterpret_cast<const unsigned int*>(source);
		float *destF = reinterpret_cast<float*>(dest);

		for(int x = 0; x < width; x++)
		{
			destF[x] = (float)(sourceD24[x] & 0xFFFFFF00) / 0xFFFFFF00;
		}
	}

	template<>
	void TransferRow<D32toD32F>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const unsigned int *sourceD32 = reinterpret_cast<const unsigned int*>(source);
		float *destF = reinterpret_cast<float*>(dest);

		for(int x = 0; x < width; x++)
		{
			destF[x] = (float)sourceD32[x] / 0xFFFFFFFF;
		}
	}

	template<>
	void TransferRow<D32FtoD32F_CLAMPED>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const float *sourceF = reinterpret_cast<const float*>(source);
		float *destF = reinterpret_cast<float*>(dest);

		for(int x = 0; x < width; x++)
		{
			destF[x] = sw::clamp(sourceF[x], 0.0f, 1.0f);
		}
	}

	template<>
	void TransferRow<D32FX32toD32F>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		struct D32FS8 { float depth32f; unsigned int stencil24_8; };
		const D32FS8 *sourceD32FS8 = reinterpret_cast<const D32FS8*>(source);
		float *destF = reinterpret_cast<float*>(dest);

		for(int x = 0; x < width; x++)
		{
			destF[x] = sw::clamp(sourceD32FS8[x].depth32f, 0.0f, 1.0f);
		}
	}

	template<>
	void TransferRow<X24S8toS8>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		const unsigned int *sourceI = reinterpret_cast<const unsigned int*>(source);
		unsigned char *destI = dest;

		for(int x = 0; x < width; x++)
		{
			destI[x] = static_cast<unsigned char>(sourceI[x] & 0x000000FF);   // FIXME: Quad layout
		}
	}

	template<>
	void TransferRow<X56S8toS8>(unsigned char *dest, const unsigned char *source, GLsizei width, GLsizei bytes)
	{
		struct D32FS8 { float depth32f; unsigned int stencil24_8; };
		const D32FS8 *sourceD32FS8 = reinterpret_cast<const D32FS8*>(source);
		unsigned char *destI = dest;

		for(int x = 0; x < width; x++)
		{
			destI[x] = static_cast<unsigned char>(sourceD32FS8[x].stencil24_8 & 0x000000FF);   // FIXME: Quad layout
		}
	}

	struct Rectangle
	{
		GLsizei bytes;
		GLsizei width;
		GLsizei height;
		GLsizei depth;
		int inputPitch;
		int inputHeight;
		int destPitch;
		GLsizei destSlice;
	};

	template<TransferType transferType>
	void Transfer(void *buffer, const void *input, const Rectangle &rect)
	{
		for(int z = 0; z < rect.depth; z++)
		{
			const unsigned char *inputStart = static_cast<const unsigned char*>(input) + (z * rect.inputPitch * rect.inputHeight);
			unsigned char *destStart = static_cast<unsigned char*>(buffer) + (z * rect.destSlice);
			for(int y = 0; y < rect.height; y++)
			{
				const unsigned char *source = inputStart + y * rect.inputPitch;
				unsigned char *dest = destStart + y * rect.destPitch;

				TransferRow<transferType>(dest, source, rect.width, rect.bytes);
			}
		}
	}

	class ImageImplementation : public Image
	{
	public:
		ImageImplementation(Texture *parentTexture, GLsizei width, GLsizei height, GLint internalformat)
			: Image(parentTexture, width, height, internalformat) {}
		ImageImplementation(Texture *parentTexture, GLsizei width, GLsizei height, GLsizei depth, int border, GLint internalformat)
			: Image(parentTexture, width, height, depth, border, internalformat) {}
		ImageImplementation(GLsizei width, GLsizei height, GLint internalformat, int pitchP)
			: Image(width, height, internalformat, pitchP) {}
		ImageImplementation(GLsizei width, GLsizei height, GLint internalformat, int multiSampleDepth, bool lockable)
			: Image(width, height, internalformat, multiSampleDepth, lockable) {}

		~ImageImplementation() override
		{
			sync();   // Wait for any threads that use this image to finish.
		}

		void *lockInternal(int x, int y, int z, sw::Lock lock, sw::Accessor client) override
		{
			return Image::lockInternal(x, y, z, lock, client);
		}

		void unlockInternal() override
		{
			return Image::unlockInternal();
		}

		void release() override
		{
			return Image::release();
		}
	};

	Image *Image::create(Texture *parentTexture, GLsizei width, GLsizei height, GLint internalformat)
	{
		if(size(width, height, 1, 0, 1, internalformat) > IMPLEMENTATION_MAX_IMAGE_SIZE_BYTES)
		{
			return nullptr;
		}

		return new ImageImplementation(parentTexture, width, height, internalformat);
	}

	Image *Image::create(Texture *parentTexture, GLsizei width, GLsizei height, GLsizei depth, int border, GLint internalformat)
	{
		if(size(width, height, depth, border, 1, internalformat) > IMPLEMENTATION_MAX_IMAGE_SIZE_BYTES)
		{
			return nullptr;
		}

		return new ImageImplementation(parentTexture, width, height, depth, border, internalformat);
	}

	Image *Image::create(GLsizei width, GLsizei height, GLint internalformat, int pitchP)
	{
		if(size(pitchP, height, 1, 0, 1, internalformat) > IMPLEMENTATION_MAX_IMAGE_SIZE_BYTES)
		{
			return nullptr;
		}

		return new ImageImplementation(width, height, internalformat, pitchP);
	}

	Image *Image::create(GLsizei width, GLsizei height, GLint internalformat, int multiSampleDepth, bool lockable)
	{
		if(size(width, height, 1, 0, multiSampleDepth, internalformat) > IMPLEMENTATION_MAX_IMAGE_SIZE_BYTES)
		{
			return nullptr;
		}

		return new ImageImplementation(width, height, internalformat, multiSampleDepth, lockable);
	}

	size_t Image::size(int width, int height, int depth, int border, int samples, GLint internalformat)
	{
		return sw::Surface::size(width, height, depth, border, samples, gl::SelectInternalFormat(internalformat));
	}

	int ClientBuffer::getWidth() const
	{
		return width;
	}

	int ClientBuffer::getHeight() const
	{
		return height;
	}

	sw::Format ClientBuffer::getFormat() const
	{
		return format;
	}

	size_t ClientBuffer::getPlane() const
	{
		return plane;
	}

	int ClientBuffer::pitchP() const
	{
#if defined(__APPLE__)
		if(buffer)
		{
			IOSurfaceRef ioSurface = reinterpret_cast<IOSurfaceRef>(buffer);
			int pitchB = static_cast<int>(IOSurfaceGetBytesPerRowOfPlane(ioSurface, plane));
			int bytesPerPixel = sw::Surface::bytes(format);
			ASSERT((pitchB % bytesPerPixel) == 0);
			return pitchB / bytesPerPixel;
		}

		return 0;
#else
		return sw::Surface::pitchP(width, 0, format, false);
#endif
	}

	void ClientBuffer::retain()
	{
#if defined(__APPLE__)
		if(buffer)
		{
			CFRetain(reinterpret_cast<IOSurfaceRef>(buffer));
		}
#endif
	}

	void ClientBuffer::release()
	{
#if defined(__APPLE__)
		if(buffer)
		{
			CFRelease(reinterpret_cast<IOSurfaceRef>(buffer));
			buffer = nullptr;
		}
#endif
	}

	void* ClientBuffer::lock(int x, int y, int z)
	{
#if defined(__APPLE__)
		if(buffer)
		{
			IOSurfaceRef ioSurface = reinterpret_cast<IOSurfaceRef>(buffer);
			IOSurfaceLock(ioSurface, 0, nullptr);
			void* pixels = IOSurfaceGetBaseAddressOfPlane(ioSurface, plane);
			int bytes = sw::Surface::bytes(format);
			int pitchB = static_cast<int>(IOSurfaceGetBytesPerRowOfPlane(ioSurface, plane));
			int sliceB = static_cast<int>(IOSurfaceGetHeightOfPlane(ioSurface, plane)) * pitchB;
			return (unsigned char*)pixels + x * bytes + y * pitchB + z * sliceB;
		}

		return nullptr;
#else
		int bytes = sw::Surface::bytes(format);
		int pitchB = sw::Surface::pitchB(width, 0, format, false);
		int sliceB = height * pitchB;
		return (unsigned char*)buffer + x * bytes + y * pitchB + z * sliceB;
#endif
	}

	void ClientBuffer::unlock()
	{
#if defined(__APPLE__)
		if(buffer)
		{
			IOSurfaceRef ioSurface = reinterpret_cast<IOSurfaceRef>(buffer);
			IOSurfaceUnlock(ioSurface, 0, nullptr);
		}
#endif
	}

	class ClientBufferImage : public egl::Image
	{
	public:
		explicit ClientBufferImage(const ClientBuffer& clientBuffer) :
			egl::Image(clientBuffer.getWidth(),
				clientBuffer.getHeight(),
				getClientBufferInternalFormat(clientBuffer.getFormat()),
				clientBuffer.pitchP()),
			clientBuffer(clientBuffer)
		{
			shared = false;
			this->clientBuffer.retain();
		}

	private:
		ClientBuffer clientBuffer;

		~ClientBufferImage() override
		{
			sync();   // Wait for any threads that use this image to finish.

			clientBuffer.release();
		}

		static GLint getClientBufferInternalFormat(sw::Format format)
		{
			switch(format)
			{
			case sw::FORMAT_R8:            return GL_R8;
			case sw::FORMAT_G8R8:          return GL_RG8;
			case sw::FORMAT_A8R8G8B8:      return GL_BGRA8_EXT;
			case sw::FORMAT_R16UI:         return GL_R16UI;
			case sw::FORMAT_A16B16G16R16F: return GL_RGBA16F;
			default:                       return GL_NONE;
			}
		}

		void *lockInternal(int x, int y, int z, sw::Lock lock, sw::Accessor client) override
		{
			LOGLOCK("image=%p op=%s.swsurface lock=%d", this, __FUNCTION__, lock);

			// Always do this for reference counting.
			void *data = sw::Surface::lockInternal(x, y, z, lock, client);

			if(x != 0 || y != 0 || z != 0)
			{
				LOGLOCK("badness: %s called with unsupported parms: image=%p x=%d y=%d z=%d", __FUNCTION__, this, x, y, z);
			}

			LOGLOCK("image=%p op=%s.ani lock=%d", this, __FUNCTION__, lock);

			// Lock the ClientBuffer and use its address.
			data = clientBuffer.lock(x, y, z);

			if(lock == sw::LOCK_UNLOCKED)
			{
				// We're never going to get a corresponding unlock, so unlock
				// immediately. This keeps the reference counts sane.
				clientBuffer.unlock();
			}

			return data;
		}

		void unlockInternal() override
		{
			LOGLOCK("image=%p op=%s.ani", this, __FUNCTION__);
			clientBuffer.unlock();

			LOGLOCK("image=%p op=%s.swsurface", this, __FUNCTION__);
			sw::Surface::unlockInternal();
		}

		void *lock(int x, int y, int z, sw::Lock lock) override
		{
			LOGLOCK("image=%p op=%s lock=%d", this, __FUNCTION__, lock);
			(void)sw::Surface::lockExternal(x, y, z, lock, sw::PUBLIC);

			return clientBuffer.lock(x, y, z);
		}

		void unlock() override
		{
			LOGLOCK("image=%p op=%s.ani", this, __FUNCTION__);
			clientBuffer.unlock();

			LOGLOCK("image=%p op=%s.swsurface", this, __FUNCTION__);
			sw::Surface::unlockExternal();
		}

		void release() override
		{
			Image::release();
		}
	};

	Image *Image::create(const egl::ClientBuffer& clientBuffer)
	{
		return new ClientBufferImage(clientBuffer);
	}

	Image::~Image()
	{
		// sync() must be called in the destructor of the most derived class to ensure their vtable isn't destroyed
		// before all threads are done using this image. Image itself is abstract so it can't be the most derived.
		ASSERT(isUnlocked());

		if(parentTexture)
		{
			parentTexture->release();
		}

		ASSERT(!shared);
	}

	void *Image::lockInternal(int x, int y, int z, sw::Lock lock, sw::Accessor client)
	{
		return Surface::lockInternal(x, y, z, lock, client);
	}

	void Image::unlockInternal()
	{
		Surface::unlockInternal();
	}

	void Image::release()
	{
		int refs = dereference();

		if(refs > 0)
		{
			if(parentTexture)
			{
				parentTexture->sweep();
			}
		}
		else
		{
			delete this;
		}
	}

	void Image::unbind(const egl::Texture *parent)
	{
		if(parentTexture == parent)
		{
			parentTexture = nullptr;
		}

		release();
	}

	bool Image::isChildOf(const egl::Texture *parent) const
	{
		return parentTexture == parent;
	}

	void Image::loadImageData(GLsizei width, GLsizei height, GLsizei depth, int inputPitch, int inputHeight, GLenum format, GLenum type, const void *input, void *buffer)
	{
		Rectangle rect;
		rect.bytes = gl::ComputePixelSize(format, type);
		rect.width = width;
		rect.height = height;
		rect.depth = depth;
		rect.inputPitch = inputPitch;
		rect.inputHeight = inputHeight;
		rect.destPitch = getPitch();
		rect.destSlice = getSlice();

		// [OpenGL ES 3.0.5] table 3.2 and 3.3.
		switch(format)
		{
		case GL_RGBA:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:
				switch(internalformat)
				{
				case GL_RGBA8:
				case GL_SRGB8_ALPHA8:
					return Transfer<Bytes>(buffer, input, rect);
				case GL_RGB5_A1:
				case GL_RGBA4:
					ASSERT_OR_RETURN(getExternalFormat() == sw::FORMAT_A8B8G8R8);
					return Transfer<Bytes>(buffer, input, rect);
				default:
					UNREACHABLE(internalformat);
				}
			case GL_BYTE:
				ASSERT_OR_RETURN(internalformat == GL_RGBA8_SNORM && getExternalFormat() == sw::FORMAT_A8B8G8R8_SNORM);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_UNSIGNED_SHORT_4_4_4_4:
				ASSERT_OR_RETURN(internalformat == GL_RGBA4 && getExternalFormat() == sw::FORMAT_A8B8G8R8);
				return Transfer<RGBA4toRGBA8>(buffer, input, rect);
			case GL_UNSIGNED_SHORT_5_5_5_1:
				ASSERT_OR_RETURN(internalformat == GL_RGB5_A1 && getExternalFormat() == sw::FORMAT_A8B8G8R8);
				return Transfer<RGBA5_A1toRGBA8>(buffer, input, rect);
			case GL_UNSIGNED_INT_2_10_10_10_REV:
				switch(internalformat)
				{
				case GL_RGB10_A2:
					ASSERT_OR_RETURN(getExternalFormat() == sw::FORMAT_A2B10G10R10);
					return Transfer<Bytes>(buffer, input, rect);
				case GL_RGB5_A1:
					ASSERT_OR_RETURN(getExternalFormat() == sw::FORMAT_A8B8G8R8);
					return Transfer<RGBA1010102toRGBA8>(buffer, input, rect);
				default:
					UNREACHABLE(internalformat);
				}
			case GL_HALF_FLOAT:
			case GL_HALF_FLOAT_OES:
				ASSERT_OR_RETURN(internalformat == GL_RGBA16F && getExternalFormat() == sw::FORMAT_A16B16G16R16F);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_FLOAT:
				switch(internalformat)
				{
				case GL_RGBA32F: return Transfer<Bytes>(buffer, input, rect);
				case GL_RGBA16F: return Transfer<RGBA32FtoRGBA16F>(buffer, input, rect);
				default: UNREACHABLE(internalformat);
				}
			default:
				UNREACHABLE(type);
			}
		case GL_RGBA_INTEGER:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:
				ASSERT_OR_RETURN(internalformat == GL_RGBA8UI && getExternalFormat() == sw::FORMAT_A8B8G8R8UI);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_BYTE:
				ASSERT_OR_RETURN(internalformat == GL_RGBA8I && getExternalFormat() == sw::FORMAT_A8B8G8R8I);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_UNSIGNED_SHORT:
				ASSERT_OR_RETURN(internalformat == GL_RGBA16UI && getExternalFormat() == sw::FORMAT_A16B16G16R16UI);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_SHORT:
				ASSERT_OR_RETURN(internalformat == GL_RGBA16I && getExternalFormat() == sw::FORMAT_A16B16G16R16I);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_UNSIGNED_INT:
				ASSERT_OR_RETURN(internalformat == GL_RGBA32UI && getExternalFormat() == sw::FORMAT_A32B32G32R32UI);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_INT:
				ASSERT_OR_RETURN(internalformat == GL_RGBA32I && getExternalFormat() == sw::FORMAT_A32B32G32R32I);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_UNSIGNED_INT_2_10_10_10_REV:
				ASSERT_OR_RETURN(internalformat == GL_RGB10_A2UI && getExternalFormat() == sw::FORMAT_A2B10G10R10UI);
				return Transfer<Bytes>(buffer, input, rect);
			default:
				UNREACHABLE(type);
			}
		case GL_BGRA_EXT:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:
				ASSERT_OR_RETURN(internalformat == GL_BGRA8_EXT && getExternalFormat() == sw::FORMAT_A8R8G8B8);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT:   // Only valid for glReadPixels calls.
			case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT:   // Only valid for glReadPixels calls.
			default:
				UNREACHABLE(type);
			}
		case GL_RGB:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:
				switch(internalformat)
				{
				case GL_RGB8:   return Transfer<RGB8toRGBX8>(buffer, input, rect);
				case GL_SRGB8:  return Transfer<RGB8toRGBX8>(buffer, input, rect);
				case GL_RGB565: return Transfer<RGB8toRGB565>(buffer, input, rect);
				default: UNREACHABLE(internalformat);
				}
			case GL_BYTE:
				ASSERT_OR_RETURN(internalformat == GL_RGB8_SNORM && getExternalFormat() == sw::FORMAT_X8B8G8R8_SNORM);
				return Transfer<RGB8toRGBX8>(buffer, input, rect);
			case GL_UNSIGNED_SHORT_5_6_5:
				ASSERT_OR_RETURN(internalformat == GL_RGB565 && getExternalFormat() == sw::FORMAT_R5G6B5);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_UNSIGNED_INT_10F_11F_11F_REV:
				ASSERT_OR_RETURN(internalformat == GL_R11F_G11F_B10F && getExternalFormat() == sw::FORMAT_X16B16G16R16F_UNSIGNED);
				return Transfer<R11G11B10FtoRGBX16F>(buffer, input, rect);
			case GL_UNSIGNED_INT_5_9_9_9_REV:
				ASSERT_OR_RETURN(internalformat == GL_RGB9_E5 && getExternalFormat() == sw::FORMAT_X16B16G16R16F_UNSIGNED);
				return Transfer<RGB9_E5FtoRGBX16F>(buffer, input, rect);
			case GL_HALF_FLOAT:
			case GL_HALF_FLOAT_OES:
				switch(internalformat)
				{
				case GL_RGB16F:
					ASSERT_OR_RETURN(getExternalFormat() == sw::FORMAT_X16B16G16R16F);
					return Transfer<RGB16FtoRGBX16F>(buffer, input, rect);
				case GL_R11F_G11F_B10F:
				case GL_RGB9_E5:
					ASSERT_OR_RETURN(getExternalFormat() == sw::FORMAT_X16B16G16R16F_UNSIGNED);
					return Transfer<RGB16FtoRGBX16F>(buffer, input, rect);
				default:
					UNREACHABLE(internalformat);
				}
			case GL_FLOAT:
				switch(internalformat)
				{
				case GL_RGB32F:
					ASSERT_OR_RETURN(getExternalFormat() == sw::FORMAT_X32B32G32R32F);
					return Transfer<RGB32FtoRGBX32F>(buffer, input, rect);
				case GL_RGB16F:
					ASSERT_OR_RETURN(getExternalFormat() == sw::FORMAT_X16B16G16R16F);
					return Transfer<RGB32FtoRGB16F>(buffer, input, rect);
				case GL_R11F_G11F_B10F:
				case GL_RGB9_E5:
					ASSERT_OR_RETURN(getExternalFormat() == sw::FORMAT_X16B16G16R16F_UNSIGNED);
					return Transfer<RGB32FtoRGB16F_UNSIGNED>(buffer, input, rect);
				default:
					UNREACHABLE(internalformat);
				}
			default:
				UNREACHABLE(type);
			}
		case GL_RGB_INTEGER:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:
				ASSERT_OR_RETURN(internalformat == GL_RGB8UI && getExternalFormat() == sw::FORMAT_X8B8G8R8UI);
				return Transfer<RGB8toRGBX8>(buffer, input, rect);
			case GL_BYTE:
				ASSERT_OR_RETURN(internalformat == GL_RGB8I && getExternalFormat() == sw::FORMAT_X8B8G8R8I);
				return Transfer<RGB8toRGBX8>(buffer, input, rect);
			case GL_UNSIGNED_SHORT:
				ASSERT_OR_RETURN(internalformat == GL_RGB16UI && getExternalFormat() == sw::FORMAT_X16B16G16R16UI);
				return Transfer<RGB16toRGBX16>(buffer, input, rect);
			case GL_SHORT:
				ASSERT_OR_RETURN(internalformat == GL_RGB16I && getExternalFormat() == sw::FORMAT_X16B16G16R16I);
				return Transfer<RGB16toRGBX16>(buffer, input, rect);
			case GL_UNSIGNED_INT:
				ASSERT_OR_RETURN(internalformat == GL_RGB32UI && getExternalFormat() == sw::FORMAT_X32B32G32R32UI);
				return Transfer<RGB32toRGBX32>(buffer, input, rect);
			case GL_INT:
				ASSERT_OR_RETURN(internalformat == GL_RGB32I && getExternalFormat() == sw::FORMAT_X32B32G32R32I);
				return Transfer<RGB32toRGBX32>(buffer, input, rect);
			default:
				UNREACHABLE(type);
			}
		case GL_RG:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:
			case GL_BYTE:
			case GL_HALF_FLOAT:
			case GL_HALF_FLOAT_OES:
				return Transfer<Bytes>(buffer, input, rect);
			case GL_FLOAT:
				switch(internalformat)
				{
				case GL_RG32F: return Transfer<Bytes>(buffer, input, rect);
				case GL_RG16F: return Transfer<RG32FtoRG16F>(buffer, input, rect);
				default: UNREACHABLE(internalformat);
				}
			default:
				UNREACHABLE(type);
			}
		case GL_RG_INTEGER:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:
				ASSERT_OR_RETURN(internalformat == GL_RG8UI && getExternalFormat() == sw::FORMAT_G8R8UI);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_BYTE:
				ASSERT_OR_RETURN(internalformat == GL_RG8I && getExternalFormat() == sw::FORMAT_G8R8I);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_UNSIGNED_SHORT:
				ASSERT_OR_RETURN(internalformat == GL_RG16UI && getExternalFormat() == sw::FORMAT_G16R16UI);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_SHORT:
				ASSERT_OR_RETURN(internalformat == GL_RG16I && getExternalFormat() == sw::FORMAT_G16R16I);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_UNSIGNED_INT:
				ASSERT_OR_RETURN(internalformat == GL_RG32UI && getExternalFormat() == sw::FORMAT_G32R32UI);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_INT:
				ASSERT_OR_RETURN(internalformat == GL_RG32I && getExternalFormat() == sw::FORMAT_G32R32I);
				return Transfer<Bytes>(buffer, input, rect);
			default:
				UNREACHABLE(type);
			}
		case GL_RED:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:
			case GL_BYTE:
			case GL_HALF_FLOAT:
			case GL_HALF_FLOAT_OES:
				return Transfer<Bytes>(buffer, input, rect);
			case GL_FLOAT:
				switch(internalformat)
				{
				case GL_R32F: return Transfer<Bytes>(buffer, input, rect);
				case GL_R16F: return Transfer<R32FtoR16F>(buffer, input, rect);
				default: UNREACHABLE(internalformat);
				}
			default:
				UNREACHABLE(type);
			}
		case GL_RED_INTEGER:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:
				ASSERT_OR_RETURN(internalformat == GL_R8UI && getExternalFormat() == sw::FORMAT_R8UI);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_BYTE:
				ASSERT_OR_RETURN(internalformat == GL_R8I && getExternalFormat() == sw::FORMAT_R8I);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_UNSIGNED_SHORT:
				ASSERT_OR_RETURN(internalformat == GL_R16UI && getExternalFormat() == sw::FORMAT_R16UI);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_SHORT:
				ASSERT_OR_RETURN(internalformat == GL_R16I && getExternalFormat() == sw::FORMAT_R16I);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_UNSIGNED_INT:
				ASSERT_OR_RETURN(internalformat == GL_R32UI && getExternalFormat() == sw::FORMAT_R32UI);
				return Transfer<Bytes>(buffer, input, rect);
			case GL_INT:
				ASSERT_OR_RETURN(internalformat == GL_R32I && getExternalFormat() == sw::FORMAT_R32I);
				return Transfer<Bytes>(buffer, input, rect);
			default:
				UNREACHABLE(type);
			}
		case GL_DEPTH_COMPONENT:
			switch(type)
			{
			case GL_UNSIGNED_SHORT: return Transfer<D16toD32F>(buffer, input, rect);
			case GL_UNSIGNED_INT:   return Transfer<D32toD32F>(buffer, input, rect);
			case GL_FLOAT:          return Transfer<D32FtoD32F_CLAMPED>(buffer, input, rect);
			case GL_DEPTH_COMPONENT24:       // Only valid for glRenderbufferStorage calls.
			case GL_DEPTH_COMPONENT32_OES:   // Only valid for glRenderbufferStorage calls.
			default: UNREACHABLE(type);
			}
		case GL_DEPTH_STENCIL:
			switch(type)
			{
			case GL_UNSIGNED_INT_24_8:              return Transfer<D24X8toD32F>(buffer, input, rect);
			case GL_FLOAT_32_UNSIGNED_INT_24_8_REV: return Transfer<D32FX32toD32F>(buffer, input, rect);
			default: UNREACHABLE(type);
			}
		case GL_LUMINANCE_ALPHA:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:
				return Transfer<Bytes>(buffer, input, rect);
			case GL_FLOAT:
				switch(internalformat)
				{
				case GL_LUMINANCE_ALPHA32F_EXT: return Transfer<Bytes>(buffer, input, rect);
				case GL_LUMINANCE_ALPHA16F_EXT: return Transfer<RG32FtoRG16F>(buffer, input, rect);
				default: UNREACHABLE(internalformat);
				}
			case GL_HALF_FLOAT:
			case GL_HALF_FLOAT_OES:
				ASSERT_OR_RETURN(internalformat == GL_LUMINANCE_ALPHA16F_EXT);
				return Transfer<Bytes>(buffer, input, rect);
			default:
				UNREACHABLE(type);
			}
		case GL_LUMINANCE:
		case GL_ALPHA:
			switch(type)
			{
			case GL_UNSIGNED_BYTE:
				return Transfer<Bytes>(buffer, input, rect);
			case GL_FLOAT:
				switch(internalformat)
				{
				case GL_LUMINANCE32F_EXT: return Transfer<Bytes>(buffer, input, rect);
				case GL_LUMINANCE16F_EXT: return Transfer<R32FtoR16F>(buffer, input, rect);
				case GL_ALPHA32F_EXT:     return Transfer<Bytes>(buffer, input, rect);
				case GL_ALPHA16F_EXT:     return Transfer<R32FtoR16F>(buffer, input, rect);
				default: UNREACHABLE(internalformat);
				}
			case GL_HALF_FLOAT:
			case GL_HALF_FLOAT_OES:
				ASSERT_OR_RETURN(internalformat == GL_LUMINANCE16F_EXT || internalformat == GL_ALPHA16F_EXT);
				return Transfer<Bytes>(buffer, input, rect);
			default:
				UNREACHABLE(type);
			}
		default:
			UNREACHABLE(format);
		}
	}

	void Image::loadStencilData(GLsizei width, GLsizei height, GLsizei depth, int inputPitch, int inputHeight, GLenum format, GLenum type, const void *input, void *buffer)
	{
		Rectangle rect;
		rect.bytes = gl::ComputePixelSize(format, type);
		rect.width = width;
		rect.height = height;
		rect.depth = depth;
		rect.inputPitch = inputPitch;
		rect.inputHeight = inputHeight;
		rect.destPitch = getStencilPitchB();
		rect.destSlice = getStencilSliceB();

		switch(type)
		{
		case GL_UNSIGNED_INT_24_8:              return Transfer<X24S8toS8>(buffer, input, rect);
		case GL_FLOAT_32_UNSIGNED_INT_24_8_REV: return Transfer<X56S8toS8>(buffer, input, rect);
		default: UNREACHABLE(format);
		}
	}

	void Image::loadImageData(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels)
	{
		GLsizei inputWidth = (unpackParameters.rowLength == 0) ? width : unpackParameters.rowLength;
		GLsizei inputPitch = gl::ComputePitch(inputWidth, format, type, unpackParameters.alignment);
		GLsizei inputHeight = (unpackParameters.imageHeight == 0) ? height : unpackParameters.imageHeight;
		char *input = ((char*)pixels) + gl::ComputePackingOffset(format, type, inputWidth, inputHeight, unpackParameters);

		void *buffer = lock(xoffset, yoffset, zoffset, sw::LOCK_WRITEONLY);

		if(buffer)
		{
			loadImageData(width, height, depth, inputPitch, inputHeight, format, type, input, buffer);
		}

		unlock();

		if(hasStencil())
		{
			unsigned char *stencil = reinterpret_cast<unsigned char*>(lockStencil(xoffset, yoffset, zoffset, sw::PUBLIC));

			if(stencil)
			{
				loadStencilData(width, height, depth, inputPitch, inputHeight, format, type, input, stencil);
			}

			unlockStencil();
		}
	}

	void Image::loadCompressedData(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels)
	{
		int inputPitch = gl::ComputeCompressedPitch(width, internalformat);
		int inputSlice = imageSize / depth;
		int rows = inputSlice / inputPitch;

		void *buffer = lock(xoffset, yoffset, zoffset, sw::LOCK_WRITEONLY);

		if(buffer)
		{
			for(int z = 0; z < depth; z++)
			{
				for(int y = 0; y < rows; y++)
				{
					GLbyte *dest = (GLbyte*)buffer + y * getPitch() + z * getSlice();
					GLbyte *source = (GLbyte*)pixels + y * inputPitch + z * inputSlice;
					memcpy(dest, source, inputPitch);
				}
			}
		}

		unlock();
	}
}
