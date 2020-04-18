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

#ifndef LIBGL_UTILITIES_H
#define LIBGL_UTILITIES_H

#include "Device.hpp"
#include "Image.hpp"
#include "Texture.h"

#define _GDI32_
#include <windows.h>
#include <GL/GL.h>
#include <GL/glext.h>

#include <string>

namespace gl
{
	struct Color;

	unsigned int UniformComponentCount(GLenum type);
	GLenum UniformComponentType(GLenum type);
	size_t UniformTypeSize(GLenum type);
	int VariableRowCount(GLenum type);
	int VariableColumnCount(GLenum type);

	int AllocateFirstFreeBits(unsigned int *bits, unsigned int allocationSize, unsigned int bitsSize);

	int ComputePixelSize(GLenum format, GLenum type);
	GLsizei ComputePitch(GLsizei width, GLenum format, GLenum type, GLint alignment);
	GLsizei ComputeCompressedPitch(GLsizei width, GLenum format);
	GLsizei ComputeCompressedSize(GLsizei width, GLsizei height, GLenum format);
	bool IsCompressed(GLenum format);
	bool IsDepthTexture(GLenum format);
	bool IsStencilTexture(GLenum format);
	bool IsCubemapTextureTarget(GLenum target);
	int CubeFaceIndex(GLenum cubeTarget);
	bool IsTextureTarget(GLenum target);
	bool CheckTextureFormatType(GLenum format, GLenum type);

	bool IsColorRenderable(GLenum internalformat);
	bool IsDepthRenderable(GLenum internalformat);
	bool IsStencilRenderable(GLenum internalformat);
}

namespace es2sw
{
	sw::DepthCompareMode ConvertDepthComparison(GLenum comparison);
	sw::StencilCompareMode ConvertStencilComparison(GLenum comparison);
	sw::Color<float> ConvertColor(gl::Color color);
	sw::BlendFactor ConvertBlendFunc(GLenum blend);
	sw::BlendOperation ConvertBlendOp(GLenum blendOp);
	sw::LogicalOperation ConvertLogicalOperation(GLenum logicalOperation);
	sw::StencilOperation ConvertStencilOp(GLenum stencilOp);
	sw::AddressingMode ConvertTextureWrap(GLenum wrap);
	sw::CullMode ConvertCullMode(GLenum cullFace, GLenum frontFace);
	unsigned int ConvertColorMask(bool red, bool green, bool blue, bool alpha);
	sw::MipmapType ConvertMipMapFilter(GLenum minFilter);
	sw::FilterType ConvertTextureFilter(GLenum minFilter, GLenum magFilter, float maxAnisotropy);
	bool ConvertPrimitiveType(GLenum primitiveType, GLsizei elementCount,  gl::PrimitiveType &swPrimitiveType, int &primitiveCount);
	sw::Format ConvertRenderbufferFormat(GLenum format);
}

namespace sw2es
{
	GLuint GetAlphaSize(sw::Format colorFormat);
	GLuint GetRedSize(sw::Format colorFormat);
	GLuint GetGreenSize(sw::Format colorFormat);
	GLuint GetBlueSize(sw::Format colorFormat);
	GLuint GetDepthSize(sw::Format depthFormat);
	GLuint GetStencilSize(sw::Format stencilFormat);

	GLenum ConvertBackBufferFormat(sw::Format format);
	GLenum ConvertDepthStencilFormat(sw::Format format);
}

#endif  // LIBGL_UTILITIES_H
