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

// utilities.h: Conversion functions and other utility routines.

#ifndef LIBGLES_CM_UTILITIES_H
#define LIBGLES_CM_UTILITIES_H

#include "Device.hpp"
#include "common/Image.hpp"
#include "Texture.h"

#include <GLES/gl.h>
#include <GLES/glext.h>

#include <string>

namespace es1
{
	struct Color;

	bool IsCompressed(GLenum format);
	GLenum ValidateSubImageParams(bool compressed, bool copy, GLenum target, GLint level, GLint xoffset, GLint yoffset,
	                              GLsizei width, GLsizei height, GLenum format, GLenum type, Texture *texture);
	bool IsDepthTexture(GLenum format);
	bool IsStencilTexture(GLenum format);
	bool IsCubemapTextureTarget(GLenum target);
	int CubeFaceIndex(GLenum cubeTarget);
	bool IsTextureTarget(GLenum target);
	GLenum ValidateTextureFormatType(GLenum format, GLenum type, GLint internalformat, GLenum target);

	bool IsColorRenderable(GLint internalformat);
	bool IsDepthRenderable(GLint internalformat);
	bool IsStencilRenderable(GLint internalformat);

	GLuint GetAlphaSize(GLint internalformat);
	GLuint GetRedSize(GLint internalformat);
	GLuint GetGreenSize(GLint internalformat);
	GLuint GetBlueSize(GLint internalformat);
	GLuint GetDepthSize(GLint internalformat);
	GLuint GetStencilSize(GLint internalformat);

	bool IsAlpha(GLint texFormat);
	bool IsRGB(GLint texFormat);
	bool IsRGBA(GLint texFormat);
}

namespace es2sw
{
	sw::DepthCompareMode ConvertDepthComparison(GLenum comparison);
	sw::StencilCompareMode ConvertStencilComparison(GLenum comparison);
	sw::AlphaCompareMode ConvertAlphaComparison(GLenum comparison);
	sw::Color<float> ConvertColor(es1::Color color);
	sw::BlendFactor ConvertBlendFunc(GLenum blend);
	sw::BlendOperation ConvertBlendOp(GLenum blendOp);
	sw::LogicalOperation ConvertLogicalOperation(GLenum logicalOperation);
	sw::StencilOperation ConvertStencilOp(GLenum stencilOp);
	sw::AddressingMode ConvertTextureWrap(GLenum wrap);
	sw::CullMode ConvertCullMode(GLenum cullFace, GLenum frontFace);
	unsigned int ConvertColorMask(bool red, bool green, bool blue, bool alpha);
	sw::MipmapType ConvertMipMapFilter(GLenum minFilter);
	sw::FilterType ConvertTextureFilter(GLenum minFilter, GLenum magFilter, float maxAnisotropy);
	bool ConvertPrimitiveType(GLenum primitiveType, GLsizei elementCount,  GLenum elementType, sw::DrawType &swPrimitiveType, int &primitiveCount);
	sw::TextureStage::StageOperation ConvertCombineOperation(GLenum operation);
	sw::TextureStage::SourceArgument ConvertSourceArgument(GLenum argument);
	sw::TextureStage::ArgumentModifier ConvertSourceOperand(GLenum operand);
}

namespace sw2es
{
	GLenum ConvertBackBufferFormat(sw::Format format);
	GLenum ConvertDepthStencilFormat(sw::Format format);
}

#endif  // LIBGLES_CM_UTILITIES_H
