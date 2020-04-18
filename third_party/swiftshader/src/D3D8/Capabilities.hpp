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

#ifndef D3D8_Capabilities_hpp
#define D3D8_Capabilities_hpp

#include <d3d8.h>

namespace D3D8
{
	struct Capabilities
	{
		struct Surface
		{
			struct RenderTarget
			{
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
			};

			struct DepthStencil
			{
				static bool D32;
				static bool D24S8;
				static bool D24X8;
				static bool D16;
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
			static bool A2B10G10R10;
			// Compressed formats
			static bool DXT1;
			static bool DXT2;
			static bool DXT3;
			static bool DXT4;
			static bool DXT5;
			// Bump map formats
			static bool V8U8;
			static bool L6V5U5;
			static bool X8L8V8U8;
			static bool Q8W8V8U8;
			static bool V16U16;
			static bool A2W10V10U10;
			// Luminance formats
			static bool L8;
			static bool A4L4;
			static bool A8L8;
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
			static bool A2B10G10R10;
			// Compressed formats
			static bool DXT1;
			static bool DXT2;
			static bool DXT3;
			static bool DXT4;
			static bool DXT5;
			// Bump map formats
			static bool V8U8;
			static bool L6V5U5;
			static bool X8L8V8U8;
			static bool Q8W8V8U8;
			static bool V16U16;
			static bool A2W10V10U10;
			// Luminance formats
			static bool L8;
			static bool A4L4;
			static bool A8L8;
		};

		struct CubeMap
		{
			struct RenderTarget
			{
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
			};

			struct DepthStencil
			{
				static bool D32;
				static bool D24S8;
				static bool D24X8;
				static bool D16;
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
			static bool A2B10G10R10;
			// Compressed formats
			static bool DXT1;
			static bool DXT2;
			static bool DXT3;
			static bool DXT4;
			static bool DXT5;
			// Bump map formats
			static bool V8U8;
			static bool L6V5U5;
			static bool X8L8V8U8;
			static bool Q8W8V8U8;
			static bool V16U16;
			static bool A2W10V10U10;
			// Luminance formats
			static bool L8;
			static bool A4L4;
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
			static bool A2B10G10R10;
			// Compressed formats
			static bool DXT1;
			static bool DXT2;
			static bool DXT3;
			static bool DXT4;
			static bool DXT5;
			// Bump map formats
			static bool V8U8;
			static bool L6V5U5;
			static bool X8L8V8U8;
			static bool Q8W8V8U8;
			static bool V16U16;
			static bool A2W10V10U10;
			// Luminance formats
			static bool L8;
			static bool A4L4;
			static bool A8L8;
		};

		struct Texture
		{
			struct RenderTarget
			{
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
			};

			struct DepthStencil
			{
				static bool D32;
				static bool D24S8;
				static bool D24X8;
				static bool D16;
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
			static bool A2B10G10R10;
			// Compressed formats
			static bool DXT1;
			static bool DXT2;
			static bool DXT3;
			static bool DXT4;
			static bool DXT5;
			// Bump map formats
			static bool V8U8;
			static bool L6V5U5;
			static bool X8L8V8U8;
			static bool Q8W8V8U8;
			static bool V16U16;
			static bool A2W10V10U10;
			// Luminance formats
			static bool L8;
			static bool A4L4;
			static bool A8L8;
		};
	};

	extern unsigned int pixelShaderVersion;
	extern unsigned int vertexShaderVersion;

	extern unsigned int textureMemory;
	extern unsigned int maxAnisotropy;
}

#endif   // D3D8_Capabilities_hpp
