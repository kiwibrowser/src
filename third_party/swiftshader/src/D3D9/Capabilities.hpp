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

#ifndef D3D9_Capabilities_hpp
#define D3D9_Capabilities_hpp

#include "Config.hpp"

#include <d3d9.h>

namespace D3D9
{
	enum
	{
		D3DFMT_ATI1 = MAKEFOURCC('A', 'T', 'I', '1'),
		D3DFMT_ATI2 = MAKEFOURCC('A', 'T', 'I', '2'),
		D3DFMT_INST = MAKEFOURCC('I', 'N', 'S', 'T'),
		D3DFMT_DF24 = MAKEFOURCC('D', 'F', '2', '4'),
		D3DFMT_DF16 = MAKEFOURCC('D', 'F', '1', '6'),
		D3DFMT_NULL = MAKEFOURCC('N', 'U', 'L', 'L'),
		D3DFMT_GET4 = MAKEFOURCC('G', 'E', 'T', '4'),
		D3DFMT_GET1 = MAKEFOURCC('G', 'E', 'T', '1'),
		D3DFMT_NVDB = MAKEFOURCC('N', 'V', 'D', 'B'),
		D3DFMT_A2M1 = MAKEFOURCC('A', '2', 'M', '1'),
		D3DFMT_A2M0 = MAKEFOURCC('A', '2', 'M', '0'),
		D3DFMT_ATOC = MAKEFOURCC('A', 'T', 'O', 'C'),
		D3DFMT_INTZ = MAKEFOURCC('I', 'N', 'T', 'Z')
	};

	struct Capabilities
	{
		struct Surface
		{
			struct RenderTarget
			{
				static bool NULL_;
				static bool R8G8B8;
				static bool R5G6B5;
				static bool X1R5G5B5;
				static bool A1R5G5B5;
				static bool A4R4G4B4;
				static bool R3G3B2;
				static bool A8R3G3B2;
				static bool X4R4G4B4;
				static bool A8R8G8B8;
				static bool X8R8G8B8;
				static bool A8B8G8R8;
				static bool X8B8G8R8;
				// Integer HDR formats
				static bool G16R16;
				static bool A2B10G10R10;
				static bool A2R10G10B10;
				static bool A16B16G16R16;
				// Floating-point formats
				static bool R16F;
				static bool G16R16F;
				static bool A16B16G16R16F;
				static bool R32F;
				static bool G32R32F;
				static bool A32B32G32R32F;
			};

			struct DepthStencil
			{
				static bool D32;
				static bool D24S8;
				static bool D24X8;
				static bool D16;
				static bool D24FS8;
				static bool D32F_LOCKABLE;
				static bool DF24;
				static bool DF16;
				static bool INTZ;
			};

			static bool A8;
			static bool R5G6B5;
			static bool X1R5G5B5;
			static bool A1R5G5B5;
			static bool A4R4G4B4;
			static bool R3G3B2;
			static bool A8R3G3B2;
			static bool X4R4G4B4;
			static bool R8G8B8;
			static bool X8R8G8B8;
			static bool A8R8G8B8;
			static bool A8B8G8R8;
			static bool X8B8G8R8;
			// Paletted formats
			static bool P8;
			static bool A8P8;
			// Integer HDR formats
			static bool G16R16;
			static bool A2R10G10B10;
			static bool A2B10G10R10;
			static bool A16B16G16R16;
			// Compressed formats
			static bool DXT1;
			static bool DXT2;
			static bool DXT3;
			static bool DXT4;
			static bool DXT5;
			static bool ATI1;
			static bool ATI2;
			// Floating-point formats
			static bool R16F;
			static bool G16R16F;
			static bool A16B16G16R16F;
			static bool R32F;
			static bool G32R32F;
			static bool A32B32G32R32F;
			// Bump map formats
			static bool V8U8;
			static bool L6V5U5;
			static bool X8L8V8U8;
			static bool Q8W8V8U8;
			static bool V16U16;
			static bool A2W10V10U10;
			static bool Q16W16V16U16;
			// Luminance formats
			static bool L8;
			static bool A4L4;
			static bool L16;
			static bool A8L8;
			// Depth Bounds Test
			static bool NVDB;
			// Transparency anti-aliasing
			static bool ATOC;
		};

		struct Volume
		{
			static bool A8;
			static bool R5G6B5;
			static bool X1R5G5B5;
			static bool A1R5G5B5;
			static bool A4R4G4B4;
			static bool R3G3B2;
			static bool A8R3G3B2;
			static bool X4R4G4B4;
			static bool R8G8B8;
			static bool X8R8G8B8;
			static bool A8R8G8B8;
			static bool A8B8G8R8;
			static bool X8B8G8R8;
			// Paletted formats
			static bool P8;
			static bool A8P8;
			// Integer HDR formats
			static bool G16R16;
			static bool A2R10G10B10;
			static bool A2B10G10R10;
			static bool A16B16G16R16;
			// Compressed formats
			static bool DXT1;
			static bool DXT2;
			static bool DXT3;
			static bool DXT4;
			static bool DXT5;
			static bool ATI1;
			static bool ATI2;
			// Floating-point formats
			static bool R16F;
			static bool G16R16F;
			static bool A16B16G16R16F;
			static bool R32F;
			static bool G32R32F;
			static bool A32B32G32R32F;
			// Bump map formats
			static bool V8U8;
			static bool L6V5U5;
			static bool X8L8V8U8;
			static bool Q8W8V8U8;
			static bool V16U16;
			static bool A2W10V10U10;
			static bool Q16W16V16U16;
			// Luminance formats
			static bool L8;
			static bool A4L4;
			static bool L16;
			static bool A8L8;
		};

		struct CubeMap
		{
			struct RenderTarget
			{
				static bool NULL_;
				static bool R8G8B8;
				static bool R5G6B5;
				static bool X1R5G5B5;
				static bool A1R5G5B5;
				static bool A4R4G4B4;
				static bool R3G3B2;
				static bool A8R3G3B2;
				static bool X4R4G4B4;
				static bool A8R8G8B8;
				static bool X8R8G8B8;
				static bool A8B8G8R8;
				static bool X8B8G8R8;
				// Integer HDR formats
				static bool G16R16;
				static bool A2B10G10R10;
				static bool A2R10G10B10;
				static bool A16B16G16R16;
				// Floating-point formats
				static bool R16F;
				static bool G16R16F;
				static bool A16B16G16R16F;
				static bool R32F;
				static bool G32R32F;
				static bool A32B32G32R32F;
			};

			struct DepthStencil
			{
				static bool D32;
				static bool D24S8;
				static bool D24X8;
				static bool D16;
				static bool D24FS8;
				static bool D32F_LOCKABLE;
				static bool DF24;
				static bool DF16;
				static bool INTZ;
			};

			static bool A8;
			static bool R5G6B5;
			static bool X1R5G5B5;
			static bool A1R5G5B5;
			static bool A4R4G4B4;
			static bool R3G3B2;
			static bool A8R3G3B2;
			static bool X4R4G4B4;
			static bool R8G8B8;
			static bool X8R8G8B8;
			static bool A8R8G8B8;
			static bool A8B8G8R8;
			static bool X8B8G8R8;
			// Paletted formats
			static bool P8;
			static bool A8P8;
			// Integer HDR formats
			static bool G16R16;
			static bool A2R10G10B10;
			static bool A2B10G10R10;
			static bool A16B16G16R16;
			// Compressed formats
			static bool DXT1;
			static bool DXT2;
			static bool DXT3;
			static bool DXT4;
			static bool DXT5;
			static bool ATI1;
			static bool ATI2;
			// Floating-point formats
			static bool R16F;
			static bool G16R16F;
			static bool A16B16G16R16F;
			static bool R32F;
			static bool G32R32F;
			static bool A32B32G32R32F;
			// Bump map formats
			static bool V8U8;
			static bool L6V5U5;
			static bool X8L8V8U8;
			static bool Q8W8V8U8;
			static bool V16U16;
			static bool A2W10V10U10;
			static bool Q16W16V16U16;
			// Luminance formats
			static bool L8;
			static bool A4L4;
			static bool L16;
			static bool A8L8;
		};

		struct VolumeTexture
		{
			static bool A8;
			static bool R5G6B5;
			static bool X1R5G5B5;
			static bool A1R5G5B5;
			static bool A4R4G4B4;
			static bool R3G3B2;
			static bool A8R3G3B2;
			static bool X4R4G4B4;
			static bool R8G8B8;
			static bool X8R8G8B8;
			static bool A8R8G8B8;
			static bool A8B8G8R8;
			static bool X8B8G8R8;
			// Paletted formats
			static bool P8;
			static bool A8P8;
			// Integer HDR formats
			static bool G16R16;
			static bool A2R10G10B10;
			static bool A2B10G10R10;
			static bool A16B16G16R16;
			// Compressed formats
			static bool DXT1;
			static bool DXT2;
			static bool DXT3;
			static bool DXT4;
			static bool DXT5;
			static bool ATI1;
			static bool ATI2;
			// Floating-point formats
			static bool R16F;
			static bool G16R16F;
			static bool A16B16G16R16F;
			static bool R32F;
			static bool G32R32F;
			static bool A32B32G32R32F;
			// Bump map formats
			static bool V8U8;
			static bool L6V5U5;
			static bool X8L8V8U8;
			static bool Q8W8V8U8;
			static bool V16U16;
			static bool A2W10V10U10;
			static bool Q16W16V16U16;
			// Luminance formats
			static bool L8;
			static bool A4L4;
			static bool L16;
			static bool A8L8;
		};

		struct Texture
		{
			struct RenderTarget
			{
				static bool NULL_;
				static bool R8G8B8;
				static bool R5G6B5;
				static bool X1R5G5B5;
				static bool A1R5G5B5;
				static bool A4R4G4B4;
				static bool R3G3B2;
				static bool A8R3G3B2;
				static bool X4R4G4B4;
				static bool A8R8G8B8;
				static bool X8R8G8B8;
				static bool A8B8G8R8;
				static bool X8B8G8R8;
				// Integer HDR formats
				static bool G16R16;
				static bool A2B10G10R10;
				static bool A2R10G10B10;
				static bool A16B16G16R16;
				// Floating-point formats
				static bool R16F;
				static bool G16R16F;
				static bool A16B16G16R16F;
				static bool R32F;
				static bool G32R32F;
				static bool A32B32G32R32F;
			};

			struct DepthStencil
			{
				static bool D32;
				static bool D24S8;
				static bool D24X8;
				static bool D16;
				static bool D24FS8;
				static bool D32F_LOCKABLE;
				static bool DF24;
				static bool DF16;
				static bool INTZ;
			};

			static bool NULL_;
			static bool A8;
			static bool R5G6B5;
			static bool X1R5G5B5;
			static bool A1R5G5B5;
			static bool A4R4G4B4;
			static bool R3G3B2;
			static bool A8R3G3B2;
			static bool X4R4G4B4;
			static bool R8G8B8;
			static bool X8R8G8B8;
			static bool A8R8G8B8;
			static bool A8B8G8R8;
			static bool X8B8G8R8;
			// Paletted formats
			static bool P8;
			static bool A8P8;
			// Integer HDR formats
			static bool G16R16;
			static bool A2R10G10B10;
			static bool A2B10G10R10;
			static bool A16B16G16R16;
			// Compressed formats
			static bool DXT1;
			static bool DXT2;
			static bool DXT3;
			static bool DXT4;
			static bool DXT5;
			static bool ATI1;
			static bool ATI2;
			// Floating-point formats
			static bool R16F;
			static bool G16R16F;
			static bool A16B16G16R16F;
			static bool R32F;
			static bool G32R32F;
			static bool A32B32G32R32F;
			// Bump map formats
			static bool V8U8;
			static bool L6V5U5;
			static bool X8L8V8U8;
			static bool Q8W8V8U8;
			static bool V16U16;
			static bool A2W10V10U10;
			static bool Q16W16V16U16;
			// Luminance formats
			static bool L8;
			static bool A4L4;
			static bool L16;
			static bool A8L8;
			// Depth formats
			static bool D32;
			static bool D24S8;
			static bool D24X8;
			static bool D16;
			static bool D24FS8;
			static bool D32F_LOCKABLE;
			static bool DF24;
			static bool DF16;
			static bool INTZ;
		};

		static bool isSRGBreadable(D3DFORMAT format);
		static bool isSRGBwritable(D3DFORMAT format);
	};

	extern unsigned int pixelShaderVersionX;
	extern unsigned int vertexShaderVersionX;

	extern unsigned long pixelShaderArbitrarySwizzle;
	extern unsigned long pixelShaderGradientInstructions;
	extern unsigned long pixelShaderPredication;
	extern unsigned long pixelShaderNoDependentReadLimit;
	extern unsigned long pixelShaderNoTexInstructionLimit;

	extern unsigned long pixelShaderDynamicFlowControlDepth;
	extern unsigned long pixelShaderStaticFlowControlDepth;

	extern unsigned long vertexShaderPredication;
	extern unsigned long vertexShaderDynamicFlowControlDepth;

	extern unsigned int textureMemory;
	extern unsigned int maxAnisotropy;

	enum
	{
		MAX_VERTEX_SHADER_CONST = 256,
		MAX_PIXEL_SHADER_CONST = 224,
		MAX_VERTEX_INPUTS = 16,
		MAX_VERTEX_OUTPUTS = 12,
		MAX_PIXEL_INPUTS = 10,
	};

	// Shader Model 3.0 requirements
	static_assert(MAX_VERTEX_SHADER_CONST >= 256, "");
	static_assert(MAX_PIXEL_SHADER_CONST == 224, "");
	static_assert(MAX_VERTEX_INPUTS == 16, "");
	static_assert(MAX_VERTEX_OUTPUTS == 12, "");
	static_assert(MAX_PIXEL_INPUTS == 10, "");

	// Back-end minimum requirements
	static_assert(sw::VERTEX_UNIFORM_VECTORS >= MAX_VERTEX_SHADER_CONST, "");
	static_assert(sw::FRAGMENT_UNIFORM_VECTORS >= MAX_PIXEL_SHADER_CONST, "");
	static_assert(sw::MAX_VERTEX_INPUTS >= MAX_VERTEX_INPUTS, "");
	static_assert(sw::MAX_VERTEX_OUTPUTS >= MAX_VERTEX_OUTPUTS, "");
	static_assert(sw::MAX_FRAGMENT_INPUTS >= MAX_PIXEL_INPUTS, "");
}

#endif   // D3D9_Capabilities_hpp
