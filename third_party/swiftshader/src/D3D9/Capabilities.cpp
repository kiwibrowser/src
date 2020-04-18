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

#include "Capabilities.hpp"

#include "Main/Config.hpp"

namespace D3D9
{
	bool Capabilities::Surface::RenderTarget::NULL_ = true;
	bool Capabilities::Surface::RenderTarget::R8G8B8 = false;
	bool Capabilities::Surface::RenderTarget::R5G6B5 = true;
	bool Capabilities::Surface::RenderTarget::X1R5G5B5 = true;
	bool Capabilities::Surface::RenderTarget::A1R5G5B5 = true;
	bool Capabilities::Surface::RenderTarget::A4R4G4B4 = true;
	bool Capabilities::Surface::RenderTarget::R3G3B2 = false;
	bool Capabilities::Surface::RenderTarget::A8R3G3B2 = false;
	bool Capabilities::Surface::RenderTarget::X4R4G4B4 = true;
	bool Capabilities::Surface::RenderTarget::A8R8G8B8 = true;
	bool Capabilities::Surface::RenderTarget::X8R8G8B8 = true;
	bool Capabilities::Surface::RenderTarget::A8B8G8R8 = true;
	bool Capabilities::Surface::RenderTarget::X8B8G8R8 = true;
	bool Capabilities::Surface::RenderTarget::G16R16 = true;
	bool Capabilities::Surface::RenderTarget::A2B10G10R10 = true;
	bool Capabilities::Surface::RenderTarget::A2R10G10B10 = true;
	bool Capabilities::Surface::RenderTarget::A16B16G16R16 = true;
	bool Capabilities::Surface::RenderTarget::R16F = true;
	bool Capabilities::Surface::RenderTarget::G16R16F = true;
	bool Capabilities::Surface::RenderTarget::A16B16G16R16F = true;
	bool Capabilities::Surface::RenderTarget::R32F = true;
	bool Capabilities::Surface::RenderTarget::G32R32F = true;
	bool Capabilities::Surface::RenderTarget::A32B32G32R32F = true;

	bool Capabilities::Surface::DepthStencil::D32 = true;
	bool Capabilities::Surface::DepthStencil::D24S8 = true;
	bool Capabilities::Surface::DepthStencil::D24X8 = true;
	bool Capabilities::Surface::DepthStencil::D16 = true;
	bool Capabilities::Surface::DepthStencil::D24FS8 = true;
	bool Capabilities::Surface::DepthStencil::D32F_LOCKABLE = true;
	bool Capabilities::Surface::DepthStencil::DF24 = true;
	bool Capabilities::Surface::DepthStencil::DF16 = true;
	bool Capabilities::Surface::DepthStencil::INTZ = true;

	bool Capabilities::Surface::A8 = true;
	bool Capabilities::Surface::R5G6B5 = true;
	bool Capabilities::Surface::X1R5G5B5 = true;
	bool Capabilities::Surface::A1R5G5B5 = true;
	bool Capabilities::Surface::A4R4G4B4 = true;
	bool Capabilities::Surface::R3G3B2 = true;
	bool Capabilities::Surface::A8R3G3B2 = true;
	bool Capabilities::Surface::X4R4G4B4 = true;
	bool Capabilities::Surface::R8G8B8 = true;
	bool Capabilities::Surface::X8R8G8B8 = true;
	bool Capabilities::Surface::A8R8G8B8 = true;
	bool Capabilities::Surface::X8B8G8R8 = true;
	bool Capabilities::Surface::A8B8G8R8 = true;
	bool Capabilities::Surface::P8 = false;
	bool Capabilities::Surface::A8P8 = false;
	bool Capabilities::Surface::G16R16 = true;
	bool Capabilities::Surface::A2R10G10B10 = true;
	bool Capabilities::Surface::A2B10G10R10 = true;
	bool Capabilities::Surface::A16B16G16R16 = true;
	bool Capabilities::Surface::DXT1 = true;
	bool Capabilities::Surface::DXT2 = true;
	bool Capabilities::Surface::DXT3 = true;
	bool Capabilities::Surface::DXT4 = true;
	bool Capabilities::Surface::DXT5 = true;
	bool Capabilities::Surface::ATI1 = true;
	bool Capabilities::Surface::ATI2 = true;
	bool Capabilities::Surface::R16F = true;
	bool Capabilities::Surface::G16R16F = true;
	bool Capabilities::Surface::A16B16G16R16F = true;
	bool Capabilities::Surface::R32F = true;
	bool Capabilities::Surface::G32R32F = true;
	bool Capabilities::Surface::A32B32G32R32F = true;
	bool Capabilities::Surface::V8U8 = true;
	bool Capabilities::Surface::L6V5U5 = true;
	bool Capabilities::Surface::X8L8V8U8 = true;
	bool Capabilities::Surface::Q8W8V8U8 = true;
	bool Capabilities::Surface::V16U16 = true;
	bool Capabilities::Surface::A2W10V10U10 = true;
	bool Capabilities::Surface::Q16W16V16U16 = true;
	bool Capabilities::Surface::L8 = true;
	bool Capabilities::Surface::A4L4 = true;
	bool Capabilities::Surface::L16 = true;
	bool Capabilities::Surface::A8L8 = true;
	bool Capabilities::Surface::NVDB = false;
	bool Capabilities::Surface::ATOC = true;

	bool Capabilities::Volume::A8 = true;
	bool Capabilities::Volume::R5G6B5 = true;
	bool Capabilities::Volume::X1R5G5B5 = true;
	bool Capabilities::Volume::A1R5G5B5 = true;
	bool Capabilities::Volume::A4R4G4B4 = true;
	bool Capabilities::Volume::R3G3B2 = true;
	bool Capabilities::Volume::A8R3G3B2 = true;
	bool Capabilities::Volume::X4R4G4B4 = true;
	bool Capabilities::Volume::R8G8B8 = false;
	bool Capabilities::Volume::X8R8G8B8 = true;
	bool Capabilities::Volume::A8R8G8B8 = true;
	bool Capabilities::Volume::X8B8G8R8 = true;
	bool Capabilities::Volume::A8B8G8R8 = true;
	bool Capabilities::Volume::P8 = false;
	bool Capabilities::Volume::A8P8 = false;
	bool Capabilities::Volume::G16R16 = true;
	bool Capabilities::Volume::A2R10G10B10 = true;
	bool Capabilities::Volume::A2B10G10R10 = true;
	bool Capabilities::Volume::A16B16G16R16 = true;
	bool Capabilities::Volume::DXT1 = true;
	bool Capabilities::Volume::DXT2 = true;
	bool Capabilities::Volume::DXT3 = true;
	bool Capabilities::Volume::DXT4 = true;
	bool Capabilities::Volume::DXT5 = true;
	bool Capabilities::Volume::ATI1 = true;
	bool Capabilities::Volume::ATI2 = true;
	bool Capabilities::Volume::R16F = true;
	bool Capabilities::Volume::G16R16F = true;
	bool Capabilities::Volume::A16B16G16R16F = true;
	bool Capabilities::Volume::R32F = true;
	bool Capabilities::Volume::G32R32F = true;
	bool Capabilities::Volume::A32B32G32R32F = true;
	bool Capabilities::Volume::V8U8 = true;
	bool Capabilities::Volume::L6V5U5 = true;
	bool Capabilities::Volume::X8L8V8U8 = true;
	bool Capabilities::Volume::Q8W8V8U8 = true;
	bool Capabilities::Volume::V16U16 = true;
	bool Capabilities::Volume::A2W10V10U10 = true;
	bool Capabilities::Volume::Q16W16V16U16 = true;
	bool Capabilities::Volume::L8 = true;
	bool Capabilities::Volume::A4L4 = true;
	bool Capabilities::Volume::L16 = true;
	bool Capabilities::Volume::A8L8 = true;

	bool Capabilities::CubeMap::RenderTarget::NULL_ = true;
	bool Capabilities::CubeMap::RenderTarget::R8G8B8 = false;
	bool Capabilities::CubeMap::RenderTarget::R5G6B5 = true;
	bool Capabilities::CubeMap::RenderTarget::X1R5G5B5 = true;
	bool Capabilities::CubeMap::RenderTarget::A1R5G5B5 = true;
	bool Capabilities::CubeMap::RenderTarget::A4R4G4B4 = true;
	bool Capabilities::CubeMap::RenderTarget::R3G3B2 = false;
	bool Capabilities::CubeMap::RenderTarget::A8R3G3B2 = false;
	bool Capabilities::CubeMap::RenderTarget::X4R4G4B4 = true;
	bool Capabilities::CubeMap::RenderTarget::A8R8G8B8 = true;
	bool Capabilities::CubeMap::RenderTarget::X8R8G8B8 = true;
	bool Capabilities::CubeMap::RenderTarget::A8B8G8R8 = true;
	bool Capabilities::CubeMap::RenderTarget::X8B8G8R8 = true;
	bool Capabilities::CubeMap::RenderTarget::G16R16 = true;
	bool Capabilities::CubeMap::RenderTarget::A2B10G10R10 = true;
	bool Capabilities::CubeMap::RenderTarget::A2R10G10B10 = true;
	bool Capabilities::CubeMap::RenderTarget::A16B16G16R16 = true;
	bool Capabilities::CubeMap::RenderTarget::R16F = true;
	bool Capabilities::CubeMap::RenderTarget::G16R16F = true;
	bool Capabilities::CubeMap::RenderTarget::A16B16G16R16F = true;
	bool Capabilities::CubeMap::RenderTarget::R32F = true;
	bool Capabilities::CubeMap::RenderTarget::G32R32F = true;
	bool Capabilities::CubeMap::RenderTarget::A32B32G32R32F = true;

	bool Capabilities::CubeMap::DepthStencil::D32 = false;
	bool Capabilities::CubeMap::DepthStencil::D24S8 = false;
	bool Capabilities::CubeMap::DepthStencil::D24X8 = false;
	bool Capabilities::CubeMap::DepthStencil::D16 = false;
	bool Capabilities::CubeMap::DepthStencil::D24FS8 = false;
	bool Capabilities::CubeMap::DepthStencil::D32F_LOCKABLE = false;
	bool Capabilities::CubeMap::DepthStencil::DF24 = false;
	bool Capabilities::CubeMap::DepthStencil::DF16 = false;
	bool Capabilities::CubeMap::DepthStencil::INTZ = false;

	bool Capabilities::CubeMap::A8 = true;
	bool Capabilities::CubeMap::R5G6B5 = true;
	bool Capabilities::CubeMap::X1R5G5B5 = true;
	bool Capabilities::CubeMap::A1R5G5B5 = true;
	bool Capabilities::CubeMap::A4R4G4B4 = true;
	bool Capabilities::CubeMap::R3G3B2 = true;
	bool Capabilities::CubeMap::A8R3G3B2 = true;
	bool Capabilities::CubeMap::X4R4G4B4 = true;
	bool Capabilities::CubeMap::R8G8B8 = false;
	bool Capabilities::CubeMap::X8R8G8B8 = true;
	bool Capabilities::CubeMap::A8R8G8B8 = true;
	bool Capabilities::CubeMap::X8B8G8R8 = true;
	bool Capabilities::CubeMap::A8B8G8R8 = true;
	bool Capabilities::CubeMap::P8 = false;
	bool Capabilities::CubeMap::A8P8 = false;
	bool Capabilities::CubeMap::G16R16 = true;
	bool Capabilities::CubeMap::A2R10G10B10 = true;
	bool Capabilities::CubeMap::A2B10G10R10 = true;
	bool Capabilities::CubeMap::A16B16G16R16 = true;
	bool Capabilities::CubeMap::DXT1 = true;
	bool Capabilities::CubeMap::DXT2 = true;
	bool Capabilities::CubeMap::DXT3 = true;
	bool Capabilities::CubeMap::DXT4 = true;
	bool Capabilities::CubeMap::DXT5 = true;
	bool Capabilities::CubeMap::ATI1 = true;
	bool Capabilities::CubeMap::ATI2 = true;
	bool Capabilities::CubeMap::R16F = true;
	bool Capabilities::CubeMap::G16R16F = true;
	bool Capabilities::CubeMap::A16B16G16R16F = true;
	bool Capabilities::CubeMap::R32F = true;
	bool Capabilities::CubeMap::G32R32F = true;
	bool Capabilities::CubeMap::A32B32G32R32F = true;
	bool Capabilities::CubeMap::V8U8 = true;
	bool Capabilities::CubeMap::L6V5U5 = true;
	bool Capabilities::CubeMap::X8L8V8U8 = true;
	bool Capabilities::CubeMap::Q8W8V8U8 = true;
	bool Capabilities::CubeMap::V16U16 = true;
	bool Capabilities::CubeMap::A2W10V10U10 = true;
	bool Capabilities::CubeMap::Q16W16V16U16 = true;
	bool Capabilities::CubeMap::L8 = true;
	bool Capabilities::CubeMap::A4L4 = true;
	bool Capabilities::CubeMap::L16 = true;
	bool Capabilities::CubeMap::A8L8 = true;

	bool Capabilities::VolumeTexture::A8 = true;
	bool Capabilities::VolumeTexture::R5G6B5 = true;
	bool Capabilities::VolumeTexture::X1R5G5B5 = true;
	bool Capabilities::VolumeTexture::A1R5G5B5 = true;
	bool Capabilities::VolumeTexture::A4R4G4B4 = true;
	bool Capabilities::VolumeTexture::R3G3B2 = true;
	bool Capabilities::VolumeTexture::A8R3G3B2 = true;
	bool Capabilities::VolumeTexture::X4R4G4B4 = true;
	bool Capabilities::VolumeTexture::R8G8B8 = false;
	bool Capabilities::VolumeTexture::X8R8G8B8 = true;
	bool Capabilities::VolumeTexture::A8R8G8B8 = true;
	bool Capabilities::VolumeTexture::X8B8G8R8 = true;
	bool Capabilities::VolumeTexture::A8B8G8R8 = true;
	bool Capabilities::VolumeTexture::P8 = false;
	bool Capabilities::VolumeTexture::A8P8 = false;
	bool Capabilities::VolumeTexture::G16R16 = true;
	bool Capabilities::VolumeTexture::A2R10G10B10 = true;
	bool Capabilities::VolumeTexture::A2B10G10R10 = true;
	bool Capabilities::VolumeTexture::A16B16G16R16 = true;
	bool Capabilities::VolumeTexture::DXT1 = true;
	bool Capabilities::VolumeTexture::DXT2 = true;
	bool Capabilities::VolumeTexture::DXT3 = true;
	bool Capabilities::VolumeTexture::DXT4 = true;
	bool Capabilities::VolumeTexture::DXT5 = true;
	bool Capabilities::VolumeTexture::ATI1 = true;
	bool Capabilities::VolumeTexture::ATI2 = true;
	bool Capabilities::VolumeTexture::R16F = true;
	bool Capabilities::VolumeTexture::G16R16F = true;
	bool Capabilities::VolumeTexture::A16B16G16R16F = true;
	bool Capabilities::VolumeTexture::R32F = true;
	bool Capabilities::VolumeTexture::G32R32F = true;
	bool Capabilities::VolumeTexture::A32B32G32R32F = true;
	bool Capabilities::VolumeTexture::V8U8 = true;
	bool Capabilities::VolumeTexture::L6V5U5 = true;
	bool Capabilities::VolumeTexture::X8L8V8U8 = true;
	bool Capabilities::VolumeTexture::Q8W8V8U8 = true;
	bool Capabilities::VolumeTexture::V16U16 = true;
	bool Capabilities::VolumeTexture::A2W10V10U10 = true;
	bool Capabilities::VolumeTexture::Q16W16V16U16 = true;
	bool Capabilities::VolumeTexture::L8 = true;
	bool Capabilities::VolumeTexture::A4L4 = true;
	bool Capabilities::VolumeTexture::L16 = true;
	bool Capabilities::VolumeTexture::A8L8 = true;

	bool Capabilities::Texture::RenderTarget::NULL_ = true;
	bool Capabilities::Texture::RenderTarget::R8G8B8 = false;
	bool Capabilities::Texture::RenderTarget::R5G6B5 = true;
	bool Capabilities::Texture::RenderTarget::X1R5G5B5 = true;
	bool Capabilities::Texture::RenderTarget::A1R5G5B5 = true;
	bool Capabilities::Texture::RenderTarget::A4R4G4B4 = true;
	bool Capabilities::Texture::RenderTarget::R3G3B2 = false;
	bool Capabilities::Texture::RenderTarget::A8R3G3B2 = false;
	bool Capabilities::Texture::RenderTarget::X4R4G4B4 = true;
	bool Capabilities::Texture::RenderTarget::A8R8G8B8 = true;
	bool Capabilities::Texture::RenderTarget::X8R8G8B8 = true;
	bool Capabilities::Texture::RenderTarget::A8B8G8R8 = true;
	bool Capabilities::Texture::RenderTarget::X8B8G8R8 = true;
	bool Capabilities::Texture::RenderTarget::G16R16 = true;
	bool Capabilities::Texture::RenderTarget::A2B10G10R10 = true;
	bool Capabilities::Texture::RenderTarget::A2R10G10B10 = true;
	bool Capabilities::Texture::RenderTarget::A16B16G16R16 = true;
	bool Capabilities::Texture::RenderTarget::R16F = true;
	bool Capabilities::Texture::RenderTarget::G16R16F = true;
	bool Capabilities::Texture::RenderTarget::A16B16G16R16F = true;
	bool Capabilities::Texture::RenderTarget::R32F = true;
	bool Capabilities::Texture::RenderTarget::G32R32F = true;
	bool Capabilities::Texture::RenderTarget::A32B32G32R32F = true;

	bool Capabilities::Texture::DepthStencil::D32 = true;
	bool Capabilities::Texture::DepthStencil::D24S8 = true;
	bool Capabilities::Texture::DepthStencil::D24X8 = true;
	bool Capabilities::Texture::DepthStencil::D16 = true;
	bool Capabilities::Texture::DepthStencil::D24FS8 = true;
	bool Capabilities::Texture::DepthStencil::D32F_LOCKABLE = true;
	bool Capabilities::Texture::DepthStencil::DF24 = true;
	bool Capabilities::Texture::DepthStencil::DF16 = true;
	bool Capabilities::Texture::DepthStencil::INTZ = true;

	bool Capabilities::Texture::NULL_ = true;
	bool Capabilities::Texture::A8 = true;
	bool Capabilities::Texture::R5G6B5 = true;
	bool Capabilities::Texture::X1R5G5B5 = true;
	bool Capabilities::Texture::A1R5G5B5 = true;
	bool Capabilities::Texture::A4R4G4B4 = true;
	bool Capabilities::Texture::R3G3B2 = true;
	bool Capabilities::Texture::A8R3G3B2 = true;
	bool Capabilities::Texture::X4R4G4B4 = true;
	bool Capabilities::Texture::R8G8B8 = false;
	bool Capabilities::Texture::X8R8G8B8 = true;
	bool Capabilities::Texture::A8R8G8B8 = true;
	bool Capabilities::Texture::X8B8G8R8 = true;
	bool Capabilities::Texture::A8B8G8R8 = true;
	bool Capabilities::Texture::P8 = false;
	bool Capabilities::Texture::A8P8 = false;
	bool Capabilities::Texture::G16R16 = true;
	bool Capabilities::Texture::A2R10G10B10 = true;
	bool Capabilities::Texture::A2B10G10R10 = true;
	bool Capabilities::Texture::A16B16G16R16 = true;
	bool Capabilities::Texture::DXT1 = true;
	bool Capabilities::Texture::DXT2 = true;
	bool Capabilities::Texture::DXT3 = true;
	bool Capabilities::Texture::DXT4 = true;
	bool Capabilities::Texture::DXT5 = true;
	bool Capabilities::Texture::ATI1 = true;
	bool Capabilities::Texture::ATI2 = true;
	bool Capabilities::Texture::R16F = true;
	bool Capabilities::Texture::G16R16F = true;
	bool Capabilities::Texture::A16B16G16R16F = true;
	bool Capabilities::Texture::R32F = true;
	bool Capabilities::Texture::G32R32F = true;
	bool Capabilities::Texture::A32B32G32R32F = true;
	bool Capabilities::Texture::V8U8 = true;
	bool Capabilities::Texture::L6V5U5 = true;
	bool Capabilities::Texture::X8L8V8U8 = true;
	bool Capabilities::Texture::Q8W8V8U8 = true;
	bool Capabilities::Texture::V16U16 = true;
	bool Capabilities::Texture::A2W10V10U10 = true;
	bool Capabilities::Texture::Q16W16V16U16 = true;
	bool Capabilities::Texture::L8 = true;
	bool Capabilities::Texture::A4L4 = true;
	bool Capabilities::Texture::L16 = true;
	bool Capabilities::Texture::A8L8 = true;
	bool Capabilities::Texture::D32 = true;
	bool Capabilities::Texture::D24S8 = true;
	bool Capabilities::Texture::D24X8 = true;
	bool Capabilities::Texture::D16 = true;
	bool Capabilities::Texture::D24FS8 = true;
	bool Capabilities::Texture::D32F_LOCKABLE = true;
	bool Capabilities::Texture::DF24 = true;
	bool Capabilities::Texture::DF16 = true;
	bool Capabilities::Texture::INTZ = true;

	bool Capabilities::isSRGBreadable(D3DFORMAT format)
	{
		// Keep in sync with Surface::isSRGBreadable
		switch(format)
		{
		case D3DFMT_L8:
		case D3DFMT_A8L8:
		case D3DFMT_R8G8B8:
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
		case D3DFMT_A8B8G8R8:
		case D3DFMT_X8B8G8R8:
		case D3DFMT_R5G6B5:
		case D3DFMT_X1R5G5B5:
		case D3DFMT_A1R5G5B5:
		case D3DFMT_A4R4G4B4:
		case D3DFMT_DXT1:
		case D3DFMT_DXT2:
		case D3DFMT_DXT3:
		case D3DFMT_DXT4:
		case D3DFMT_DXT5:
		case D3DFMT_ATI1:
		case D3DFMT_ATI2:
			return true;
		default:
			return false;
		}

		return false;
	}

	bool Capabilities::isSRGBwritable(D3DFORMAT format)
	{
		// Keep in sync with Surface::isSRGBwritable
		switch(format)
		{
		case D3DFMT_NULL:
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
		case D3DFMT_A8B8G8R8:
		case D3DFMT_X8B8G8R8:
		case D3DFMT_R5G6B5:
			return true;
		default:
			return false;
		}
	}

	unsigned int pixelShaderVersionX = D3DPS_VERSION(3, 0);
	unsigned int vertexShaderVersionX = D3DVS_VERSION(3, 0);

	unsigned long pixelShaderArbitrarySwizzle = D3DPS20CAPS_ARBITRARYSWIZZLE;
	unsigned long pixelShaderGradientInstructions = D3DPS20CAPS_GRADIENTINSTRUCTIONS;
	unsigned long pixelShaderPredication = D3DPS20CAPS_PREDICATION;
	unsigned long pixelShaderNoDependentReadLimit = D3DPS20CAPS_NODEPENDENTREADLIMIT;
	unsigned long pixelShaderNoTexInstructionLimit = D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT;

	unsigned long pixelShaderDynamicFlowControlDepth = D3DPS20_MAX_DYNAMICFLOWCONTROLDEPTH;
	unsigned long pixelShaderStaticFlowControlDepth = D3DPS20_MAX_STATICFLOWCONTROLDEPTH;

	unsigned long vertexShaderPredication = D3DVS20CAPS_PREDICATION;
	unsigned long vertexShaderDynamicFlowControlDepth = D3DVS20_MAX_DYNAMICFLOWCONTROLDEPTH;

	unsigned int textureMemory = 256 * 1024 * 1024;
	unsigned int maxAnisotropy = 16;
}
