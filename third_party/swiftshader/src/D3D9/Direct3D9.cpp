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

#include "Direct3D9.hpp"

#include "Direct3DDevice9.hpp"
#include "Capabilities.hpp"
#include "Configurator.hpp"
#include "Debug.hpp"
#include "CPUID.hpp"
#include "Version.h"
#include "Config.hpp"

#define COMPILE_MULTIMON_STUBS
#include <multimon.h>
#include <assert.h>
#include <psapi.h>

namespace D3D9
{
	Direct3D9::Direct3D9(int version, const HINSTANCE instance) : version(version), instance(instance)
	{
		displayMode = 0;
		numDisplayModes = 0;

		DEVMODE devmode;
		devmode.dmSize = sizeof(DEVMODE);

		// Count number of display modes
		while(EnumDisplaySettings(0, numDisplayModes, &devmode))   // FIXME: Only enumarate internal modes!
		{
			numDisplayModes++;
		}

		displayMode = new DEVMODE[numDisplayModes];

		// Store display modes information
		for(int i = 0; i < numDisplayModes; i++)
		{
			displayMode[i].dmSize = sizeof(DEVMODE);
			EnumDisplaySettings(0, i, &displayMode[i]);
		}

		d3d9Lib = 0;
		d3d9 = 0;

		sw::Configurator ini("SwiftShader.ini");

		disableAlphaMode = ini.getBoolean("Testing", "DisableAlphaMode", false);
		disable10BitMode = ini.getBoolean("Testing", "Disable10BitMode", false);

		int ps = ini.getInteger("Capabilities", "PixelShaderVersion", 30);
		int vs = ini.getInteger("Capabilities", "VertexShaderVersion", 30);

		     if(ps ==  0) pixelShaderVersionX = D3DPS_VERSION(0, 0);
		else if(ps <= 11) pixelShaderVersionX = D3DPS_VERSION(1, 1);
		else if(ps <= 12) pixelShaderVersionX = D3DPS_VERSION(1, 2);
		else if(ps <= 13) pixelShaderVersionX = D3DPS_VERSION(1, 3);
		else if(ps <= 14) pixelShaderVersionX = D3DPS_VERSION(1, 4);
		else if(ps <= 20) pixelShaderVersionX = D3DPS_VERSION(2, 0);
		else if(ps <= 21) pixelShaderVersionX = D3DPS_VERSION(2, 1);
		else              pixelShaderVersionX = D3DPS_VERSION(3, 0);

		     if(vs ==  0) vertexShaderVersionX = D3DVS_VERSION(0, 0);
		else if(vs <= 11) vertexShaderVersionX = D3DVS_VERSION(1, 1);
		else if(vs <= 20) vertexShaderVersionX = D3DVS_VERSION(2, 0);
		else if(vs <= 21) vertexShaderVersionX = D3DVS_VERSION(2, 1);
		else              vertexShaderVersionX = D3DVS_VERSION(3, 0);

		pixelShaderArbitrarySwizzle = 0;
		pixelShaderGradientInstructions = 0;
		pixelShaderPredication = 0;
		pixelShaderNoDependentReadLimit = 0;
		pixelShaderNoTexInstructionLimit = 0;

		pixelShaderDynamicFlowControlDepth = D3DPS20_MIN_DYNAMICFLOWCONTROLDEPTH;
		pixelShaderStaticFlowControlDepth = D3DPS20_MIN_STATICFLOWCONTROLDEPTH;

		if(ps >= 21)
		{
			pixelShaderArbitrarySwizzle = D3DPS20CAPS_ARBITRARYSWIZZLE;
			pixelShaderGradientInstructions = D3DPS20CAPS_GRADIENTINSTRUCTIONS;
			pixelShaderPredication = D3DPS20CAPS_PREDICATION;
			pixelShaderNoDependentReadLimit = D3DPS20CAPS_NODEPENDENTREADLIMIT;
			pixelShaderNoTexInstructionLimit = D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT;

			pixelShaderDynamicFlowControlDepth = D3DPS20_MAX_DYNAMICFLOWCONTROLDEPTH;
			pixelShaderStaticFlowControlDepth = D3DPS20_MAX_STATICFLOWCONTROLDEPTH;
		}

		vertexShaderPredication = 0;
		vertexShaderDynamicFlowControlDepth = 0;

		if(vs >= 21)
		{
			vertexShaderPredication = D3DVS20CAPS_PREDICATION;
			vertexShaderDynamicFlowControlDepth = D3DVS20_MAX_DYNAMICFLOWCONTROLDEPTH;
		}

		textureMemory = 1024 * 1024 * ini.getInteger("Capabilities", "TextureMemory", 256);

		int shadowMapping = ini.getInteger("Testing", "ShadowMapping", 3);

		if(shadowMapping != 2 && shadowMapping != 3)   // No DST support
		{
			Capabilities::Texture::DepthStencil::D32 = false;
			Capabilities::Texture::DepthStencil::D24S8 = false;
			Capabilities::Texture::DepthStencil::D24X8 = false;
			Capabilities::Texture::DepthStencil::D16 = false;
			Capabilities::Texture::DepthStencil::D24FS8 = false;
			Capabilities::Texture::DepthStencil::D32F_LOCKABLE = false;

			Capabilities::Texture::D32 = false;
			Capabilities::Texture::D24S8 = false;
			Capabilities::Texture::D24X8 = false;
			Capabilities::Texture::D16 = false;
			Capabilities::Texture::D24FS8 = false;
			Capabilities::Texture::D32F_LOCKABLE = false;
		}

		if(shadowMapping != 1 && shadowMapping != 3)   // No Fetch4 support
		{
			Capabilities::Texture::DepthStencil::DF24 = false;
			Capabilities::Texture::DepthStencil::DF16 = false;

			Capabilities::Texture::DF24 = false;
			Capabilities::Texture::DF16 = false;
		}
	}

	Direct3D9::~Direct3D9()
	{
		delete[] displayMode;
		displayMode = 0;

		if(d3d9)
		{
			d3d9->Release();
			d3d9 = 0;
		}

		if(d3d9Lib)
		{
			FreeLibrary(d3d9Lib);
			d3d9Lib = 0;
		}
	}

	long Direct3D9::QueryInterface(const IID &iid, void **object)
	{
		TRACE("const IID &iid = 0x%0.8p, void **object = 0x%0.8p", iid, object);

		if(iid == IID_IDirect3D9 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Direct3D9::AddRef()
	{
		TRACE("void");

		return Unknown::AddRef();
	}

	unsigned long Direct3D9::Release()
	{
		TRACE("void");

		return Unknown::Release();
	}

	long Direct3D9::CheckDepthStencilMatch(unsigned int adapter, D3DDEVTYPE deviceType, D3DFORMAT adapterFormat, D3DFORMAT renderTargetFormat, D3DFORMAT depthStencilFormat)
	{
		TRACE("unsigned int adapter = %d, D3DDEVTYPE deviceType = %d, D3DFORMAT adapterFormat = %d, D3DFORMAT renderTargetFormat = %d, D3DFORMAT depthStencilFormat = %d", adapter, deviceType, adapterFormat, renderTargetFormat, depthStencilFormat);

		if(deviceType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D9();

			if(d3d9)
			{
				return d3d9->CheckDepthStencilMatch(adapter, deviceType, adapterFormat, renderTargetFormat, depthStencilFormat);
			}
			else
			{
				return CheckDepthStencilMatch(adapter, D3DDEVTYPE_HAL, adapterFormat, renderTargetFormat, depthStencilFormat);
			}
		}

		return D3D_OK;   // Any format combination is ok
	}

	long Direct3D9::CheckDeviceFormat(unsigned int adapter, D3DDEVTYPE deviceType, D3DFORMAT adapterFormat, unsigned long usage, D3DRESOURCETYPE resourceType, D3DFORMAT checkFormat)
	{
		TRACE("unsigned int adapter = %d, D3DDEVTYPE deviceType = %d, D3DFORMAT adapterFormat = %d, unsigned long usage = %d, D3DRESOURCETYPE resourceType = %d, D3DFORMAT checkFormat = %d", adapter, deviceType, adapterFormat, usage, resourceType, checkFormat);

		if(deviceType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D9();

			if(d3d9)
			{
				return d3d9->CheckDeviceFormat(adapter, deviceType, adapterFormat, usage, resourceType, checkFormat);
			}
			else
			{
				return CheckDeviceFormat(adapter, D3DDEVTYPE_HAL, adapterFormat, usage, resourceType, checkFormat);
			}
		}

		if(usage & D3DUSAGE_QUERY_SRGBREAD)
		{
			if(!Capabilities::isSRGBreadable(checkFormat))
			{
				return NOTAVAILABLE();
			}
		}

		if(usage & D3DUSAGE_QUERY_SRGBWRITE)
		{
			if(!Capabilities::isSRGBwritable(checkFormat))
			{
				return NOTAVAILABLE();
			}
		}

		// ATI hack to indicate instancing support on SM 2.0 hardware
		if(checkFormat == D3DFMT_INST && pixelShaderVersionX >= D3DPS_VERSION(2, 0) && pixelShaderVersionX < D3DPS_VERSION(3, 0))
		{
			return D3D_OK;
		}

		switch(resourceType)
		{
		case D3DRTYPE_SURFACE:
			if(usage & D3DUSAGE_RENDERTARGET)
			{
				switch(checkFormat)
				{
				case D3DFMT_NULL:			if(!Capabilities::Surface::RenderTarget::NULL_)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R8G8B8:			if(!Capabilities::Surface::RenderTarget::R8G8B8)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R5G6B5:			if(!Capabilities::Surface::RenderTarget::R5G6B5)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X1R5G5B5:		if(!Capabilities::Surface::RenderTarget::X1R5G5B5)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A1R5G5B5:		if(!Capabilities::Surface::RenderTarget::A1R5G5B5)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A4R4G4B4:		if(!Capabilities::Surface::RenderTarget::A4R4G4B4)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R3G3B2:			if(!Capabilities::Surface::RenderTarget::R3G3B2)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8R3G3B2:		if(!Capabilities::Surface::RenderTarget::A8R3G3B2)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X4R4G4B4:		if(!Capabilities::Surface::RenderTarget::X4R4G4B4)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8R8G8B8:		if(!Capabilities::Surface::RenderTarget::A8R8G8B8)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8R8G8B8:		if(!Capabilities::Surface::RenderTarget::X8R8G8B8)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8B8G8R8:		if(!Capabilities::Surface::RenderTarget::A8B8G8R8)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8B8G8R8:		if(!Capabilities::Surface::RenderTarget::X8B8G8R8)		return NOTAVAILABLE();	else return D3D_OK;
				// Integer HDR formats
				case D3DFMT_G16R16:			if(!Capabilities::Surface::RenderTarget::G16R16)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2B10G10R10:	if(!Capabilities::Surface::RenderTarget::A2B10G10R10)	return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2R10G10B10:	if(!Capabilities::Surface::RenderTarget::A2R10G10B10)	return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A16B16G16R16:	if(!Capabilities::Surface::RenderTarget::A16B16G16R16)	return NOTAVAILABLE();	else return D3D_OK;
				// Floating-point formats
				case D3DFMT_R16F:			if(!Capabilities::Surface::RenderTarget::R16F)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_G16R16F:		if(!Capabilities::Surface::RenderTarget::G16R16F)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A16B16G16R16F:	if(!Capabilities::Surface::RenderTarget::A16B16G16R16F)	return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R32F:			if(!Capabilities::Surface::RenderTarget::R32F)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_G32R32F:		if(!Capabilities::Surface::RenderTarget::G32R32F)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A32B32G32R32F:	if(!Capabilities::Surface::RenderTarget::A32B32G32R32F)	return NOTAVAILABLE();	else return D3D_OK;
				default:
					return NOTAVAILABLE();
				}
			}
			else if(usage & D3DUSAGE_DEPTHSTENCIL)
			{
				switch(checkFormat)
				{
				case D3DFMT_D32:			if(!Capabilities::Surface::DepthStencil::D32)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D24S8:			if(!Capabilities::Surface::DepthStencil::D24S8)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D24X8:			if(!Capabilities::Surface::DepthStencil::D24X8)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D16:			if(!Capabilities::Surface::DepthStencil::D16)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D24FS8:			if(!Capabilities::Surface::DepthStencil::D24FS8)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D32F_LOCKABLE:	if(!Capabilities::Surface::DepthStencil::D32F_LOCKABLE)	return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DF24:			if(!Capabilities::Surface::DepthStencil::DF24)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DF16:			if(!Capabilities::Surface::DepthStencil::DF16)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_INTZ:			if(!Capabilities::Surface::DepthStencil::INTZ)			return NOTAVAILABLE();	else return D3D_OK;
				default:
					return NOTAVAILABLE();
				}
			}
			else
			{
				switch(checkFormat)
				{
				case D3DFMT_A8:				if(!Capabilities::Surface::A8)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R5G6B5:			if(!Capabilities::Surface::R5G6B5)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X1R5G5B5:		if(!Capabilities::Surface::X1R5G5B5)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A1R5G5B5:		if(!Capabilities::Surface::A1R5G5B5)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A4R4G4B4:		if(!Capabilities::Surface::A4R4G4B4)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R3G3B2:			if(!Capabilities::Surface::R3G3B2)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8R3G3B2:		if(!Capabilities::Surface::A8R3G3B2)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X4R4G4B4:		if(!Capabilities::Surface::X4R4G4B4)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R8G8B8:			if(!Capabilities::Surface::R8G8B8)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8R8G8B8:		if(!Capabilities::Surface::X8R8G8B8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8R8G8B8:		if(!Capabilities::Surface::A8R8G8B8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8B8G8R8:		if(!Capabilities::Surface::X8B8G8R8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8B8G8R8:		if(!Capabilities::Surface::A8B8G8R8)					return NOTAVAILABLE();	else return D3D_OK;
				// Paletted formats
				case D3DFMT_P8:				if(!Capabilities::Surface::P8)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8P8:			if(!Capabilities::Surface::A8P8)						return NOTAVAILABLE();	else return D3D_OK;
				// Integer HDR formats
				case D3DFMT_G16R16:			if(!Capabilities::Surface::G16R16)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2R10G10B10:	if(!Capabilities::Surface::A2R10G10B10)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2B10G10R10:	if(!Capabilities::Surface::A2B10G10R10)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A16B16G16R16:	if(!Capabilities::Surface::A16B16G16R16)				return NOTAVAILABLE();	else return D3D_OK;
				// Compressed formats
				case D3DFMT_DXT1:			if(!Capabilities::Surface::DXT1)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT2:			if(!Capabilities::Surface::DXT2)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT3:			if(!Capabilities::Surface::DXT3)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT4:			if(!Capabilities::Surface::DXT4)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT5:			if(!Capabilities::Surface::DXT5)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_ATI1:			if(!Capabilities::Surface::ATI1)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_ATI2:			if(!Capabilities::Surface::ATI2)						return NOTAVAILABLE();	else return D3D_OK;
				// Floating-point formats
				case D3DFMT_R16F:			if(!Capabilities::Surface::R16F)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_G16R16F:		if(!Capabilities::Surface::G16R16F)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A16B16G16R16F:	if(!Capabilities::Surface::A16B16G16R16F)				return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R32F:			if(!Capabilities::Surface::R32F)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_G32R32F:		if(!Capabilities::Surface::G32R32F)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A32B32G32R32F:	if(!Capabilities::Surface::A32B32G32R32F)				return NOTAVAILABLE();	else return D3D_OK;
				// Bump map formats
				case D3DFMT_V8U8:			if(!Capabilities::Surface::V8U8)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_L6V5U5:			if(!Capabilities::Surface::L6V5U5)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8L8V8U8:		if(!Capabilities::Surface::X8L8V8U8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_Q8W8V8U8:		if(!Capabilities::Surface::Q8W8V8U8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_V16U16:			if(!Capabilities::Surface::V16U16)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2W10V10U10:	if(!Capabilities::Surface::A2W10V10U10)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_Q16W16V16U16:	if(!Capabilities::Surface::Q16W16V16U16)				return NOTAVAILABLE();	else return D3D_OK;
				// Luminance formats
				case D3DFMT_L8:				if(!Capabilities::Surface::L8)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A4L4:			if(!Capabilities::Surface::A4L4)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_L16:			if(!Capabilities::Surface::L16)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8L8:			if(!Capabilities::Surface::A8L8)						return NOTAVAILABLE();	else return D3D_OK;
				// Depth Bounds Test
				case D3DFMT_NVDB:			if(!Capabilities::Surface::NVDB)						return NOTAVAILABLE();	else return D3D_OK;
				// Transparency anti-aliasing
				case D3DFMT_ATOC:			if(!Capabilities::Surface::ATOC)						return NOTAVAILABLE();	else return D3D_OK;
				default:
					return NOTAVAILABLE();
				}
			}
		case D3DRTYPE_VOLUME:
			switch(checkFormat)
			{
			case D3DFMT_A8:					if(!Capabilities::Volume::A8)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_R5G6B5:				if(!Capabilities::Volume::R5G6B5)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_X1R5G5B5:			if(!Capabilities::Volume::X1R5G5B5)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A1R5G5B5:			if(!Capabilities::Volume::A1R5G5B5)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A4R4G4B4:			if(!Capabilities::Volume::A4R4G4B4)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_R3G3B2:				if(!Capabilities::Volume::R3G3B2)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A8R3G3B2:			if(!Capabilities::Volume::A8R3G3B2)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_X4R4G4B4:			if(!Capabilities::Volume::X4R4G4B4)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_R8G8B8:				if(!Capabilities::Volume::R8G8B8)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_X8R8G8B8:			if(!Capabilities::Volume::X8R8G8B8)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A8R8G8B8:			if(!Capabilities::Volume::A8R8G8B8)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_X8B8G8R8:			if(!Capabilities::Volume::X8B8G8R8)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A8B8G8R8:			if(!Capabilities::Volume::A8B8G8R8)						return NOTAVAILABLE();	else return D3D_OK;
			// Paletted formats
			case D3DFMT_P8:					if(!Capabilities::Volume::P8)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A8P8:				if(!Capabilities::Volume::A8P8)							return NOTAVAILABLE();	else return D3D_OK;
			// Integer HDR formats
			case D3DFMT_G16R16:				if(!Capabilities::Volume::G16R16)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A2R10G10B10:		if(!Capabilities::Volume::A2R10G10B10)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A2B10G10R10:		if(!Capabilities::Volume::A2B10G10R10)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A16B16G16R16:		if(!Capabilities::Volume::A16B16G16R16)					return NOTAVAILABLE();	else return D3D_OK;
			// Compressed formats
			case D3DFMT_DXT1:				if(!Capabilities::Volume::DXT1)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_DXT2:				if(!Capabilities::Volume::DXT2)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_DXT3:				if(!Capabilities::Volume::DXT3)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_DXT4:				if(!Capabilities::Volume::DXT4)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_DXT5:				if(!Capabilities::Volume::DXT5)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_ATI1:				if(!Capabilities::Volume::ATI1)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_ATI2:				if(!Capabilities::Volume::ATI2)							return NOTAVAILABLE();	else return D3D_OK;
			// Floating-point formats
			case D3DFMT_R16F:				if(!Capabilities::Volume::R16F)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_G16R16F:			if(!Capabilities::Volume::G16R16F)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A16B16G16R16F:		if(!Capabilities::Volume::A16B16G16R16F)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_R32F:				if(!Capabilities::Volume::R32F)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_G32R32F:			if(!Capabilities::Volume::G32R32F)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A32B32G32R32F:		if(!Capabilities::Volume::A32B32G32R32F)				return NOTAVAILABLE();	else return D3D_OK;
			// Bump map formats
			case D3DFMT_V8U8:				if(!Capabilities::Volume::V8U8)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_L6V5U5:				if(!Capabilities::Volume::L6V5U5)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_X8L8V8U8:			if(!Capabilities::Volume::X8L8V8U8)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_Q8W8V8U8:			if(!Capabilities::Volume::Q8W8V8U8)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_V16U16:				if(!Capabilities::Volume::V16U16)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A2W10V10U10:		if(!Capabilities::Volume::A2W10V10U10)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_Q16W16V16U16:		if(!Capabilities::Volume::Q16W16V16U16)					return NOTAVAILABLE();	else return D3D_OK;
			// Luminance formats
			case D3DFMT_L8:					if(!Capabilities::Volume::L8)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A4L4:				if(!Capabilities::Volume::A4L4)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_L16:				if(!Capabilities::Volume::L16)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A8L8:				if(!Capabilities::Volume::A8L8)							return NOTAVAILABLE();	else return D3D_OK;
			default:
				return NOTAVAILABLE();
			}
		case D3DRTYPE_CUBETEXTURE:
			if(usage & D3DUSAGE_RENDERTARGET)
			{
				switch(checkFormat)
				{
				case D3DFMT_NULL:			if(!Capabilities::CubeMap::RenderTarget::NULL_)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R8G8B8:			if(!Capabilities::CubeMap::RenderTarget::R8G8B8)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R5G6B5:			if(!Capabilities::CubeMap::RenderTarget::R5G6B5)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X1R5G5B5:		if(!Capabilities::CubeMap::RenderTarget::X1R5G5B5)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A1R5G5B5:		if(!Capabilities::CubeMap::RenderTarget::A1R5G5B5)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A4R4G4B4:		if(!Capabilities::CubeMap::RenderTarget::A4R4G4B4)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R3G3B2:			if(!Capabilities::CubeMap::RenderTarget::R3G3B2)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8R3G3B2:		if(!Capabilities::CubeMap::RenderTarget::A8R3G3B2)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X4R4G4B4:		if(!Capabilities::CubeMap::RenderTarget::X4R4G4B4)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8R8G8B8:		if(!Capabilities::CubeMap::RenderTarget::A8R8G8B8)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8R8G8B8:		if(!Capabilities::CubeMap::RenderTarget::X8R8G8B8)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8B8G8R8:		if(!Capabilities::CubeMap::RenderTarget::A8B8G8R8)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8B8G8R8:		if(!Capabilities::CubeMap::RenderTarget::X8B8G8R8)		return NOTAVAILABLE();	else return D3D_OK;
				// Integer HDR formats
				case D3DFMT_G16R16:			if(!Capabilities::CubeMap::RenderTarget::G16R16)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2B10G10R10:	if(!Capabilities::CubeMap::RenderTarget::A2B10G10R10)	return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2R10G10B10:	if(!Capabilities::CubeMap::RenderTarget::A2R10G10B10)	return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A16B16G16R16:	if(!Capabilities::CubeMap::RenderTarget::A16B16G16R16)	return NOTAVAILABLE();	else return D3D_OK;
				// Floating-point formats
				case D3DFMT_R16F:			if(!Capabilities::CubeMap::RenderTarget::R16F)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_G16R16F:		if(!Capabilities::CubeMap::RenderTarget::G16R16F)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A16B16G16R16F:	if(!Capabilities::CubeMap::RenderTarget::A16B16G16R16F)	return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R32F:			if(!Capabilities::CubeMap::RenderTarget::R32F)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_G32R32F:		if(!Capabilities::CubeMap::RenderTarget::G32R32F)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A32B32G32R32F:	if(!Capabilities::CubeMap::RenderTarget::A32B32G32R32F)	return NOTAVAILABLE();	else return D3D_OK;
				default:
					return NOTAVAILABLE();
				}
			}
			else if(usage & D3DUSAGE_DEPTHSTENCIL)
			{
				switch(checkFormat)
				{
				case D3DFMT_D32:			if(!Capabilities::CubeMap::DepthStencil::D32)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D24S8:			if(!Capabilities::CubeMap::DepthStencil::D24S8)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D24X8:			if(!Capabilities::CubeMap::DepthStencil::D24X8)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D16:			if(!Capabilities::CubeMap::DepthStencil::D16)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D24FS8:			if(!Capabilities::CubeMap::DepthStencil::D24FS8)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D32F_LOCKABLE:	if(!Capabilities::CubeMap::DepthStencil::D32F_LOCKABLE)	return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DF24:			if(!Capabilities::CubeMap::DepthStencil::DF24)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DF16:			if(!Capabilities::CubeMap::DepthStencil::DF16)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_INTZ:			if(!Capabilities::CubeMap::DepthStencil::INTZ)			return NOTAVAILABLE();	else return D3D_OK;
				default:
					return NOTAVAILABLE();
				}
			}
			else
			{
				switch(checkFormat)
				{
				case D3DFMT_A8:				if(!Capabilities::CubeMap::A8)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R5G6B5:			if(!Capabilities::CubeMap::R5G6B5)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X1R5G5B5:		if(!Capabilities::CubeMap::X1R5G5B5)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A1R5G5B5:		if(!Capabilities::CubeMap::A1R5G5B5)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A4R4G4B4:		if(!Capabilities::CubeMap::A4R4G4B4)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R3G3B2:			if(!Capabilities::CubeMap::R3G3B2)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8R3G3B2:		if(!Capabilities::CubeMap::A8R3G3B2)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X4R4G4B4:		if(!Capabilities::CubeMap::X4R4G4B4)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R8G8B8:			if(!Capabilities::CubeMap::R8G8B8)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8R8G8B8:		if(!Capabilities::CubeMap::X8R8G8B8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8R8G8B8:		if(!Capabilities::CubeMap::A8R8G8B8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8B8G8R8:		if(!Capabilities::CubeMap::X8B8G8R8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8B8G8R8:		if(!Capabilities::CubeMap::A8B8G8R8)					return NOTAVAILABLE();	else return D3D_OK;
				// Paletted formats
				case D3DFMT_P8:				if(!Capabilities::CubeMap::P8)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8P8:			if(!Capabilities::CubeMap::A8P8)						return NOTAVAILABLE();	else return D3D_OK;
				// Integer HDR formats
				case D3DFMT_G16R16:			if(!Capabilities::CubeMap::G16R16)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2R10G10B10:	if(!Capabilities::CubeMap::A2R10G10B10)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2B10G10R10:	if(!Capabilities::CubeMap::A2B10G10R10)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A16B16G16R16:	if(!Capabilities::CubeMap::A16B16G16R16)				return NOTAVAILABLE();	else return D3D_OK;
				// Compressed formats
				case D3DFMT_DXT1:			if(!Capabilities::CubeMap::DXT1)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT2:			if(!Capabilities::CubeMap::DXT2)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT3:			if(!Capabilities::CubeMap::DXT3)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT4:			if(!Capabilities::CubeMap::DXT4)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT5:			if(!Capabilities::CubeMap::DXT5)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_ATI1:			if(!Capabilities::CubeMap::ATI1)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_ATI2:			if(!Capabilities::CubeMap::ATI2)						return NOTAVAILABLE();	else return D3D_OK;
				// Floating-point formats
				case D3DFMT_R16F:			if(!Capabilities::CubeMap::R16F)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_G16R16F:		if(!Capabilities::CubeMap::G16R16F)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A16B16G16R16F:	if(!Capabilities::CubeMap::A16B16G16R16F)				return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R32F:			if(!Capabilities::CubeMap::R32F)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_G32R32F:		if(!Capabilities::CubeMap::G32R32F)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A32B32G32R32F:	if(!Capabilities::CubeMap::A32B32G32R32F)				return NOTAVAILABLE();	else return D3D_OK;
				// Bump map formats
				case D3DFMT_V8U8:			if(!Capabilities::CubeMap::V8U8)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_L6V5U5:			if(!Capabilities::CubeMap::L6V5U5)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8L8V8U8:		if(!Capabilities::CubeMap::X8L8V8U8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_Q8W8V8U8:		if(!Capabilities::CubeMap::Q8W8V8U8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_V16U16:			if(!Capabilities::CubeMap::V16U16)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2W10V10U10:	if(!Capabilities::CubeMap::A2W10V10U10)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_Q16W16V16U16:	if(!Capabilities::CubeMap::Q16W16V16U16)				return NOTAVAILABLE();	else return D3D_OK;
				// Luminance formats
				case D3DFMT_L8:				if(!Capabilities::CubeMap::L8)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A4L4:			if(!Capabilities::CubeMap::A4L4)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_L16:			if(!Capabilities::CubeMap::L16)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8L8:			if(!Capabilities::CubeMap::A8L8)						return NOTAVAILABLE();	else return D3D_OK;
				default:
					return NOTAVAILABLE();
				}
			}
		case D3DRTYPE_VOLUMETEXTURE:
			switch(checkFormat)
			{
			case D3DFMT_A8:					if(!Capabilities::VolumeTexture::A8)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_R5G6B5:				if(!Capabilities::VolumeTexture::R5G6B5)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_X1R5G5B5:			if(!Capabilities::VolumeTexture::X1R5G5B5)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A1R5G5B5:			if(!Capabilities::VolumeTexture::A1R5G5B5)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A4R4G4B4:			if(!Capabilities::VolumeTexture::A4R4G4B4)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_R3G3B2:				if(!Capabilities::VolumeTexture::R3G3B2)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A8R3G3B2:			if(!Capabilities::VolumeTexture::A8R3G3B2)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_X4R4G4B4:			if(!Capabilities::VolumeTexture::X4R4G4B4)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_R8G8B8:				if(!Capabilities::VolumeTexture::R8G8B8)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_X8R8G8B8:			if(!Capabilities::VolumeTexture::X8R8G8B8)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A8R8G8B8:			if(!Capabilities::VolumeTexture::A8R8G8B8)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_X8B8G8R8:			if(!Capabilities::VolumeTexture::X8B8G8R8)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A8B8G8R8:			if(!Capabilities::VolumeTexture::A8B8G8R8)				return NOTAVAILABLE();	else return D3D_OK;
			// Paletted formats
			case D3DFMT_P8:					if(!Capabilities::VolumeTexture::P8)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A8P8:				if(!Capabilities::VolumeTexture::A8P8)					return NOTAVAILABLE();	else return D3D_OK;
			// Integer HDR formats
			case D3DFMT_G16R16:				if(!Capabilities::VolumeTexture::G16R16)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A2R10G10B10:		if(!Capabilities::VolumeTexture::A2R10G10B10)			return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A2B10G10R10:		if(!Capabilities::VolumeTexture::A2B10G10R10)			return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A16B16G16R16:		if(!Capabilities::VolumeTexture::A16B16G16R16)			return NOTAVAILABLE();	else return D3D_OK;
			// Compressed formats
			case D3DFMT_DXT1:				if(!Capabilities::VolumeTexture::DXT1)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_DXT2:				if(!Capabilities::VolumeTexture::DXT2)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_DXT3:				if(!Capabilities::VolumeTexture::DXT3)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_DXT4:				if(!Capabilities::VolumeTexture::DXT4)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_DXT5:				if(!Capabilities::VolumeTexture::DXT5)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_ATI1:				if(!Capabilities::VolumeTexture::ATI1)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_ATI2:				if(!Capabilities::VolumeTexture::ATI2)					return NOTAVAILABLE();	else return D3D_OK;
			// Floating-point formats
			case D3DFMT_R16F:				if(!Capabilities::VolumeTexture::R16F)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_G16R16F:			if(!Capabilities::VolumeTexture::G16R16F)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A16B16G16R16F:		if(!Capabilities::VolumeTexture::A16B16G16R16F)			return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_R32F:				if(!Capabilities::VolumeTexture::R32F)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_G32R32F:			if(!Capabilities::VolumeTexture::G32R32F)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A32B32G32R32F:		if(!Capabilities::VolumeTexture::A32B32G32R32F)			return NOTAVAILABLE();	else return D3D_OK;
			// Bump map formats
			case D3DFMT_V8U8:				if(!Capabilities::VolumeTexture::V8U8)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_L6V5U5:				if(!Capabilities::VolumeTexture::L6V5U5)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_X8L8V8U8:			if(!Capabilities::VolumeTexture::X8L8V8U8)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_Q8W8V8U8:			if(!Capabilities::VolumeTexture::Q8W8V8U8)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_V16U16:				if(!Capabilities::VolumeTexture::V16U16)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A2W10V10U10:		if(!Capabilities::VolumeTexture::A2W10V10U10)			return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_Q16W16V16U16:		if(!Capabilities::VolumeTexture::Q16W16V16U16)			return NOTAVAILABLE();	else return D3D_OK;
			// Luminance formats
			case D3DFMT_L8:					if(!Capabilities::VolumeTexture::L8)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A4L4:				if(!Capabilities::VolumeTexture::A4L4)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_L16:				if(!Capabilities::VolumeTexture::L16)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A8L8:				if(!Capabilities::VolumeTexture::A8L8)					return NOTAVAILABLE();	else return D3D_OK;
			default:
				return NOTAVAILABLE();
			}
		case D3DRTYPE_TEXTURE:
			if(usage & D3DUSAGE_RENDERTARGET)
			{
				switch(checkFormat)
				{
				case D3DFMT_NULL:			if(!Capabilities::Texture::RenderTarget::NULL_)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R8G8B8:			if(!Capabilities::Texture::RenderTarget::R8G8B8)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R5G6B5:			if(!Capabilities::Texture::RenderTarget::R5G6B5)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X1R5G5B5:		if(!Capabilities::Texture::RenderTarget::X1R5G5B5)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A1R5G5B5:		if(!Capabilities::Texture::RenderTarget::A1R5G5B5)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A4R4G4B4:		if(!Capabilities::Texture::RenderTarget::A4R4G4B4)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R3G3B2:			if(!Capabilities::Texture::RenderTarget::R3G3B2)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8R3G3B2:		if(!Capabilities::Texture::RenderTarget::A8R3G3B2)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X4R4G4B4:		if(!Capabilities::Texture::RenderTarget::X4R4G4B4)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8R8G8B8:		if(!Capabilities::Texture::RenderTarget::A8R8G8B8)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8R8G8B8:		if(!Capabilities::Texture::RenderTarget::X8R8G8B8)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8B8G8R8:		if(!Capabilities::Texture::RenderTarget::A8B8G8R8)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8B8G8R8:		if(!Capabilities::Texture::RenderTarget::X8B8G8R8)		return NOTAVAILABLE();	else return D3D_OK;
				// Integer HDR formats
				case D3DFMT_G16R16:			if(!Capabilities::Texture::RenderTarget::G16R16)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2B10G10R10:	if(!Capabilities::Texture::RenderTarget::A2B10G10R10)	return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2R10G10B10:	if(!Capabilities::Texture::RenderTarget::A2R10G10B10)	return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A16B16G16R16:	if(!Capabilities::Texture::RenderTarget::A16B16G16R16)	return NOTAVAILABLE();	else return D3D_OK;
				// Floating-point formats
				case D3DFMT_R16F:			if(!Capabilities::Texture::RenderTarget::R16F)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_G16R16F:		if(!Capabilities::Texture::RenderTarget::G16R16F)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A16B16G16R16F:	if(!Capabilities::Texture::RenderTarget::A16B16G16R16F)	return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R32F:			if(!Capabilities::Texture::RenderTarget::R32F)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_G32R32F:		if(!Capabilities::Texture::RenderTarget::G32R32F)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A32B32G32R32F:	if(!Capabilities::Texture::RenderTarget::A32B32G32R32F)	return NOTAVAILABLE();	else return D3D_OK;
				default:
					return NOTAVAILABLE();
				}
			}
			else if(usage & D3DUSAGE_DEPTHSTENCIL)
			{
				switch(checkFormat)
				{
				case D3DFMT_D32:			if(!Capabilities::Texture::DepthStencil::D32)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D24S8:			if(!Capabilities::Texture::DepthStencil::D24S8)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D24X8:			if(!Capabilities::Texture::DepthStencil::D24X8)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D16:			if(!Capabilities::Texture::DepthStencil::D16)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D24FS8:			if(!Capabilities::Texture::DepthStencil::D24FS8)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D32F_LOCKABLE:	if(!Capabilities::Texture::DepthStencil::D32F_LOCKABLE)	return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DF24:			if(!Capabilities::Texture::DepthStencil::DF24)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DF16:			if(!Capabilities::Texture::DepthStencil::DF16)			return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_INTZ:			if(!Capabilities::Texture::DepthStencil::INTZ)			return NOTAVAILABLE();	else return D3D_OK;
				default:
					return NOTAVAILABLE();
				}
			}
			else
			{
				switch(checkFormat)
				{
				case D3DFMT_NULL:			if(!Capabilities::Texture::NULL_)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8:				if(!Capabilities::Texture::A8)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R5G6B5:			if(!Capabilities::Texture::R5G6B5)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X1R5G5B5:		if(!Capabilities::Texture::X1R5G5B5)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A1R5G5B5:		if(!Capabilities::Texture::A1R5G5B5)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A4R4G4B4:		if(!Capabilities::Texture::A4R4G4B4)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R3G3B2:			if(!Capabilities::Texture::R3G3B2)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8R3G3B2:		if(!Capabilities::Texture::A8R3G3B2)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X4R4G4B4:		if(!Capabilities::Texture::X4R4G4B4)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R8G8B8:			if(!Capabilities::Texture::R8G8B8)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8R8G8B8:		if(!Capabilities::Texture::X8R8G8B8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8R8G8B8:		if(!Capabilities::Texture::A8R8G8B8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8B8G8R8:		if(!Capabilities::Texture::X8B8G8R8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8B8G8R8:		if(!Capabilities::Texture::A8B8G8R8)					return NOTAVAILABLE();	else return D3D_OK;
				// Paletted formats
				case D3DFMT_P8:				if(!Capabilities::Texture::P8)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8P8:			if(!Capabilities::Texture::A8P8)						return NOTAVAILABLE();	else return D3D_OK;
				// Integer HDR formats
				case D3DFMT_G16R16:			if(!Capabilities::Texture::G16R16)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2R10G10B10:	if(!Capabilities::Texture::A2R10G10B10)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2B10G10R10:	if(!Capabilities::Texture::A2B10G10R10)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A16B16G16R16:	if(!Capabilities::Texture::A16B16G16R16)				return NOTAVAILABLE();	else return D3D_OK;
				// Compressed formats
				case D3DFMT_DXT1:			if(!Capabilities::Texture::DXT1)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT2:			if(!Capabilities::Texture::DXT2)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT3:			if(!Capabilities::Texture::DXT3)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT4:			if(!Capabilities::Texture::DXT4)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT5:			if(!Capabilities::Texture::DXT5)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_ATI1:			if(!Capabilities::Texture::ATI1)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_ATI2:			if(!Capabilities::Texture::ATI2)						return NOTAVAILABLE();	else return D3D_OK;
				// Floating-point formats
				case D3DFMT_R16F:			if(!Capabilities::Texture::R16F)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_G16R16F:		if(!Capabilities::Texture::G16R16F)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A16B16G16R16F:	if(!Capabilities::Texture::A16B16G16R16F)				return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_R32F:			if(!Capabilities::Texture::R32F)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_G32R32F:		if(!Capabilities::Texture::G32R32F)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A32B32G32R32F:	if(!Capabilities::Texture::A32B32G32R32F)				return NOTAVAILABLE();	else return D3D_OK;
				// Bump map formats
				case D3DFMT_V8U8:			if(!Capabilities::Texture::V8U8)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_L6V5U5:			if(!Capabilities::Texture::L6V5U5)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8L8V8U8:		if(!Capabilities::Texture::X8L8V8U8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_Q8W8V8U8:		if(!Capabilities::Texture::Q8W8V8U8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_V16U16:			if(!Capabilities::Texture::V16U16)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2W10V10U10:	if(!Capabilities::Texture::A2W10V10U10)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_Q16W16V16U16:	if(!Capabilities::Texture::Q16W16V16U16)				return NOTAVAILABLE();	else return D3D_OK;
				// Luminance formats
				case D3DFMT_L8:				if(!Capabilities::Texture::L8)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A4L4:			if(!Capabilities::Texture::A4L4)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_L16:			if(!Capabilities::Texture::L16)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8L8:			if(!Capabilities::Texture::A8L8)						return NOTAVAILABLE();	else return D3D_OK;
				// Depth formats
				case D3DFMT_D32:			if(!Capabilities::Texture::D32)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D24S8:			if(!Capabilities::Texture::D24S8)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D24X8:			if(!Capabilities::Texture::D24X8)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D16:			if(!Capabilities::Texture::D16)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D24FS8:			if(!Capabilities::Texture::D24FS8)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_D32F_LOCKABLE:	if(!Capabilities::Texture::D32F_LOCKABLE)				return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DF24:			if(!Capabilities::Texture::DF24)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DF16:			if(!Capabilities::Texture::DF16)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_INTZ:			if(!Capabilities::Texture::INTZ)						return NOTAVAILABLE();	else return D3D_OK;
				default:
					return NOTAVAILABLE();
				}
			}
		case D3DRTYPE_VERTEXBUFFER:
			if(checkFormat == D3DFMT_VERTEXDATA)
			{
				return D3D_OK;
			}
			else
			{
				return NOTAVAILABLE();
			}
		case D3DRTYPE_INDEXBUFFER:
			switch(checkFormat)
			{
			case D3DFMT_INDEX16:
			case D3DFMT_INDEX32:
				return D3D_OK;
			default:
				return NOTAVAILABLE();
			};
		default:
			return NOTAVAILABLE();
		}
	}

	long Direct3D9::CheckDeviceFormatConversion(unsigned int adapter, D3DDEVTYPE deviceType, D3DFORMAT sourceFormat, D3DFORMAT targetFormat)
	{
		TRACE("unsigned int adapter = %d, D3DDEVTYPE deviceType = %d, D3DFORMAT sourceFormat = %d, D3DFORMAT targetFormat = %d", adapter, deviceType, sourceFormat, targetFormat);

		if(deviceType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D9();

			if(d3d9)
			{
				return d3d9->CheckDeviceFormatConversion(adapter, deviceType, sourceFormat, targetFormat);
			}
			else
			{
				return CheckDeviceFormatConversion(adapter, D3DDEVTYPE_HAL, sourceFormat, targetFormat);
			}
		}

		return D3D_OK;
	}

	long Direct3D9::CheckDeviceMultiSampleType(unsigned int adapter, D3DDEVTYPE deviceType, D3DFORMAT surfaceFormat, int windowed, D3DMULTISAMPLE_TYPE multiSampleType, unsigned long *qualityLevels)
	{
		TRACE("unsigned int adapter = %d, D3DDEVTYPE deviceType = %d, D3DFORMAT surfaceFormat = %d, int windowed = %d, D3DMULTISAMPLE_TYPE multiSampleType = %d, unsigned long *qualityLevels = 0x%0.8p", adapter, deviceType, surfaceFormat, windowed, multiSampleType, qualityLevels);

		if(deviceType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D9();

			if(d3d9)
			{
				return d3d9->CheckDeviceMultiSampleType(adapter, deviceType, surfaceFormat, windowed, multiSampleType, qualityLevels);
			}
			else
			{
				return CheckDeviceMultiSampleType(adapter, D3DDEVTYPE_HAL, surfaceFormat, windowed, multiSampleType, qualityLevels);
			}
		}

		if(adapter >= GetAdapterCount())
		{
			return INVALIDCALL();
		}

		if(qualityLevels)
		{
			if(multiSampleType == D3DMULTISAMPLE_NONMASKABLE)
			{
				*qualityLevels = 4;   // 2, 4, 8, 16 samples
			}
			else
			{
				*qualityLevels = 1;
			}
		}

		if(multiSampleType == D3DMULTISAMPLE_NONE ||
		   multiSampleType == D3DMULTISAMPLE_NONMASKABLE ||
		   multiSampleType == D3DMULTISAMPLE_2_SAMPLES ||
		   multiSampleType == D3DMULTISAMPLE_4_SAMPLES ||
		   multiSampleType == D3DMULTISAMPLE_8_SAMPLES ||
		   multiSampleType == D3DMULTISAMPLE_16_SAMPLES)
		{
			if(CheckDeviceFormat(adapter, deviceType, D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, surfaceFormat) == D3D_OK ||
			   CheckDeviceFormat(adapter, deviceType, D3DFMT_X8R8G8B8, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, surfaceFormat) == D3D_OK)
			{
				if(surfaceFormat != D3DFMT_D32F_LOCKABLE && surfaceFormat != D3DFMT_D16_LOCKABLE)
				{
					return D3D_OK;
				}
			}
		}

		return NOTAVAILABLE();
	}

	long Direct3D9::CheckDeviceType(unsigned int adapter, D3DDEVTYPE checkType, D3DFORMAT displayFormat, D3DFORMAT backBufferFormat, int windowed)
	{
		TRACE("unsigned int adapter = %d, D3DDEVTYPE checkType = %d, D3DFORMAT displayFormat = %d, D3DFORMAT backBufferFormat = %d, int windowed = %d", adapter, checkType, displayFormat, backBufferFormat, windowed);

		if(checkType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D9();

			if(d3d9)
			{
				return d3d9->CheckDeviceType(adapter, checkType, displayFormat, backBufferFormat, windowed);
			}
			else
			{
				return CheckDeviceType(adapter, D3DDEVTYPE_HAL, displayFormat, backBufferFormat, windowed);
			}
		}

		if(adapter >= GetAdapterCount())
		{
			return INVALIDCALL();
		}

		switch(displayFormat)
		{
		case D3DFMT_UNKNOWN:
			if(windowed == FALSE)
			{
				return INVALIDCALL();
			}
			else
			{
				return NOTAVAILABLE();
			}
		case D3DFMT_A2R10G10B10:
		case D3DFMT_A2B10G10R10:
			if(disable10BitMode)
			{
				return NOTAVAILABLE();
			}
		case D3DFMT_X8R8G8B8:
		case D3DFMT_R5G6B5:
			break;
		default:
			return NOTAVAILABLE();
		}

		if(windowed != FALSE)
		{
			switch(backBufferFormat)
			{
			case D3DFMT_A2R10G10B10:
			case D3DFMT_A2B10G10R10:
				if(disable10BitMode)
				{
					return NOTAVAILABLE();
				}
			case D3DFMT_A8R8G8B8:
				if(disableAlphaMode)
				{
					return NOTAVAILABLE();
				}
			case D3DFMT_UNKNOWN:
			case D3DFMT_X8R8G8B8:
			case D3DFMT_R5G6B5:
		//	case D3DFMT_X1R5G5B5:      // FIXME: Supported by REF
		//	case D3DFMT_A1R5G5B5:      // FIXME: Supported by REF
				break;
			default:
				return NOTAVAILABLE();
			}
		}
		else
		{
			switch(backBufferFormat)
			{
			case D3DFMT_UNKNOWN:
				return INVALIDCALL();
			case D3DFMT_A2R10G10B10:
			case D3DFMT_A2B10G10R10:
				if(disable10BitMode)
				{
					return NOTAVAILABLE();
				}
			case D3DFMT_A8R8G8B8:
				if(disableAlphaMode)
				{
					return NOTAVAILABLE();
				}
			case D3DFMT_X8R8G8B8:
				break;
			default:
				return NOTAVAILABLE();
			}
		}

		return D3D_OK;
	}

	long Direct3D9::CreateDevice(unsigned int adapter, D3DDEVTYPE deviceType, HWND focusWindow, unsigned long behaviorFlags, D3DPRESENT_PARAMETERS *presentParameters, IDirect3DDevice9 **returnedDeviceInterface)
	{
		TRACE("unsigned int adapter = %d, D3DDEVTYPE deviceType = %d, HWND focusWindow = %d, unsigned long behaviorFlags = 0x%0.8X, D3DPRESENT_PARAMETERS *presentParameters = 0x%0.8p, IDirect3DDevice9 **returnedDeviceInterface = 0x%0.8p", adapter, deviceType, focusWindow, behaviorFlags, presentParameters, returnedDeviceInterface);

		if(deviceType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D9();

			if(d3d9)
			{
				return d3d9->CreateDevice(adapter, deviceType, focusWindow, behaviorFlags, presentParameters, returnedDeviceInterface);
			}
			else
			{
				return CreateDevice(adapter, D3DDEVTYPE_HAL, focusWindow, behaviorFlags, presentParameters, returnedDeviceInterface);
			}
		}

		if(!focusWindow || !presentParameters || !returnedDeviceInterface)
		{
			*returnedDeviceInterface = 0;

			return INVALIDCALL();
		}

		*returnedDeviceInterface = new Direct3DDevice9(instance, this, adapter, deviceType, focusWindow, behaviorFlags, presentParameters);

		if(*returnedDeviceInterface)
		{
			(*returnedDeviceInterface)->AddRef();
		}

		return D3D_OK;
	}

	long Direct3D9::EnumAdapterModes(unsigned int adapter, D3DFORMAT format, unsigned int index, D3DDISPLAYMODE *mode)
	{
		TRACE("unsigned int adapter = %d, D3DFORMAT format = %d, unsigned int index = %d, D3DDISPLAYMODE *mode = 0x%0.8p", adapter, format, index, mode);

		if(adapter != D3DADAPTER_DEFAULT || !mode)
		{
			return INVALIDCALL();
		}

		unsigned int bpp = 32;

		switch(format)
		{
		case D3DFMT_A1R5G5B5:		bpp = 16; break;
		case D3DFMT_A2R10G10B10:	bpp = 32; break;
		case D3DFMT_A8R8G8B8:		bpp = 32; break;
		case D3DFMT_R5G6B5:			bpp = 16; break;
		case D3DFMT_X1R5G5B5:		bpp = 16; break;
		case D3DFMT_X8R8G8B8:		bpp = 32; break;
		default: return INVALIDCALL();
		}

		for(int i = 0; i < numDisplayModes; i++)
		{
			if(displayMode[i].dmBitsPerPel == bpp)
			{
				if(index-- == 0)
				{
					mode->Width = displayMode[i].dmPelsWidth;
					mode->Height = displayMode[i].dmPelsHeight;
					mode->RefreshRate = displayMode[i].dmDisplayFrequency;
					mode->Format = format;

					return D3D_OK;
				}
			}
		}

		return INVALIDCALL();
	}

	unsigned int Direct3D9::GetAdapterCount()
	{
		TRACE("void");

		return 1;   // SwiftShader does not support multiple display adapters
	}

	long Direct3D9::GetAdapterDisplayMode(unsigned int adapter, D3DDISPLAYMODE *mode)
	{
		TRACE("unsigned int adapter = %d, D3DDISPLAYMODE *mode = 0x%0.8p", adapter, mode);

		if(adapter != D3DADAPTER_DEFAULT || !mode)
		{
			return INVALIDCALL();
		}

		HDC deviceContext = GetDC(0);

		mode->Width = ::GetDeviceCaps(deviceContext, HORZRES);
		mode->Height = ::GetDeviceCaps(deviceContext, VERTRES);
		mode->RefreshRate = ::GetDeviceCaps(deviceContext, VREFRESH);
		unsigned int bpp = ::GetDeviceCaps(deviceContext, BITSPIXEL);

		ReleaseDC(0, deviceContext);

		bpp = 32;   // FIXME

		switch(bpp)
		{
		case 32: mode->Format = D3DFMT_X8R8G8B8; break;
		case 24: mode->Format = D3DFMT_R8G8B8;   break;
		case 16: mode->Format = D3DFMT_R5G6B5;   break;
		default:
			ASSERT(false);   // Unexpected display mode color depth
		}

		return D3D_OK;
	}

	long Direct3D9::GetAdapterIdentifier(unsigned int adapter, unsigned long flags, D3DADAPTER_IDENTIFIER9 *identifier)
	{
		TRACE("unsigned int adapter = %d, unsigned long flags = 0x%0.8X, D3DADAPTER_IDENTIFIER9 *identifier = 0x%0.8p", adapter, flags, identifier);

		if(!identifier)
		{
			return INVALIDCALL();
		}

		sw::Configurator ini("SwiftShader.ini");
		int id = ini.getInteger("Capabilities", "Identifier", 0);

		if(id == 1)
		{
			unsigned short product = 6;
			unsigned short version = 14;
			unsigned short subVersion = 10;
			unsigned short revision = 8440;
			GUID guid = {0xD7B71E3E, 0x41D2, 0x11CF, {0x96, 0x53, 0x7A, 0x23, 0x00, 0xC2, 0xCB, 0x35}};

			identifier->DriverVersion.HighPart = product << 16 | version;
			identifier->DriverVersion.LowPart = subVersion << 16 | revision;
			strcpy(identifier->Driver, "nv4_disp.dll");
			strcpy(identifier->Description, "NVIDIA GeForce 7900 GS");
			strcpy(identifier->DeviceName, "\\\\.\\DISPLAY1");
			identifier->VendorId = 0x10DE;
			identifier->DeviceId = 0x0292;
			identifier->SubSysId = 0x037010DE;
			identifier->Revision = 0x00A1;
			identifier->DeviceIdentifier = guid;
			identifier->WHQLLevel = 1;

			return D3D_OK;
		}
		else if(id == 2)
		{
			unsigned short product = 7;
			unsigned short version = 14;
			unsigned short subVersion = 10;
			unsigned short revision = 464;
			GUID guid = {0xD7B71EE2, 0x3285, 0x11CF, {0x11, 0x72, 0x0D, 0xA2, 0xA1, 0xC2, 0xCA, 0x35}};

			identifier->DriverVersion.HighPart = product << 16 | version;
			identifier->DriverVersion.LowPart = subVersion << 16 | revision;
			strcpy(identifier->Driver, "atiumdag.dll");
			strcpy(identifier->Description, "ATI Mobility Radeon X1600");
			strcpy(identifier->DeviceName, "\\\\.\\DISPLAY1");
			identifier->VendorId = 0x1002;
			identifier->DeviceId = 0x71C5;
			identifier->SubSysId = 0x82071071;
			identifier->Revision = 0x0000;
			identifier->DeviceIdentifier = guid;
			identifier->WHQLLevel = 1;

			return D3D_OK;
		}
		else if(id == 3)
		{
			unsigned short product = 7;
			unsigned short version = 14;
			unsigned short subVersion = 10;
			unsigned short revision = 1437;
			GUID guid = {0xD7B78E66, 0x6942, 0x11CF, {0x05, 0x76, 0x03, 0x22, 0xAD, 0xC2, 0xCA, 0x35}};

			identifier->DriverVersion.HighPart = product << 16 | version;
			identifier->DriverVersion.LowPart = subVersion << 16 | revision;
			strcpy(identifier->Driver, "igdumd64.dll");
			strcpy(identifier->Description, "Intel GMA X3100");
			strcpy(identifier->DeviceName, "\\\\.\\DISPLAY1");
			identifier->VendorId = 0x8086;
			identifier->DeviceId = 0x2A02;
			identifier->SubSysId = 0x02091028;
			identifier->Revision = 0x000C;
			identifier->DeviceIdentifier = guid;
			identifier->WHQLLevel = 1;

			return D3D_OK;
		}
		else if(id == 4)
		{
			loadSystemD3D9();

			if(d3d9)
			{
				return d3d9->GetAdapterIdentifier(adapter, flags, identifier);
			}
		}

		unsigned short product = 'sw';
		unsigned short version = 3;
		unsigned short subVersion = 0;
		unsigned short revision = BUILD_REVISION;
		GUID guid = {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

		identifier->DriverVersion.HighPart = product << 16 | version;
		identifier->DriverVersion.LowPart = subVersion << 16 | revision;
		strcpy(identifier->Driver, "SwiftShader");
		strcpy(identifier->Description, "Google SwiftShader 3D Renderer");
		strcpy(identifier->DeviceName, "\\\\.\\DISPLAY1");
		identifier->VendorId = 0;
		identifier->DeviceId = 0;
		identifier->SubSysId = 0;
		identifier->Revision = 0;
		identifier->DeviceIdentifier = guid;
		identifier->WHQLLevel = 0;

		char exeName[MAX_PATH + 1];
		int n = GetModuleBaseName(GetCurrentProcess(), 0, exeName, MAX_PATH);
		exeName[n] = '\0';

		if(strncmp(exeName, "Fallout", 7) == 0)
		{
			strcpy(identifier->Driver, "nv4_disp.dll");
		}

		return D3D_OK;
	}

	unsigned int Direct3D9::GetAdapterModeCount(unsigned int adapter, D3DFORMAT format)
	{
		TRACE("unsigned int adapter = %d, D3DFORMAT format = %d", adapter, format);

		if(adapter != D3DADAPTER_DEFAULT)
		{
			return 0;
		}

		int modeCount = 0;

		for(int i = 0; i < numDisplayModes; i++)
		{
			switch(format)
			{
			case D3DFMT_A8R8G8B8:
				if(!disableAlphaMode)
				{
					if(displayMode[i].dmBitsPerPel == 32) modeCount++;
				}
				break;
			case D3DFMT_X8R8G8B8:    if(displayMode[i].dmBitsPerPel == 32) modeCount++; break;
			case D3DFMT_A2R10G10B10:
			case D3DFMT_A2B10G10R10:
				if(!disable10BitMode)
				{
					if(displayMode[i].dmBitsPerPel == 32) modeCount++;
				}
				break;
		//	case D3DFMT_R8G8B8:      if(displayMode[i].dmBitsPerPel == 24) modeCount++; break;   // NOTE: Deprecated by DirectX 9
			case D3DFMT_R5G6B5:      if(displayMode[i].dmBitsPerPel == 16) modeCount++; break;
		//	case D3DFMT_X1R5G5B5:    if(displayMode[i].dmBitsPerPel == 16) modeCount++; break;
		//	case D3DFMT_A1R5G5B5:    if(displayMode[i].dmBitsPerPel == 16) modeCount++; break;
		//	default:
		//		ASSERT(false);
			}
		}

		return modeCount;
	}

	HMONITOR Direct3D9::GetAdapterMonitor(unsigned int adapter)
	{
		TRACE("unsigned int adapter = %d", adapter);

		POINT point = {0, 0};

		return MonitorFromPoint(point, MONITOR_DEFAULTTOPRIMARY);   // FIXME: Ignores adapter parameter
	}

	long Direct3D9::GetDeviceCaps(unsigned int adapter, D3DDEVTYPE deviceType, D3DCAPS9 *capabilities)
	{
		TRACE("unsigned int adapter = %d, D3DDEVTYPE deviceType = %d, D3DCAPS9 *capabilities = 0x%0.8p", adapter, deviceType, capabilities);

		if(deviceType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D9();

			if(d3d9)
			{
				return d3d9->GetDeviceCaps(adapter, deviceType, capabilities);
			}
			else
			{
				return GetDeviceCaps(adapter, D3DDEVTYPE_HAL, capabilities);
			}
		}

		if(!capabilities)
		{
			return INVALIDCALL();
		}

		unsigned int pixelShaderVersion = pixelShaderVersionX;
		unsigned int vertexShaderVersion = vertexShaderVersionX;

		if(pixelShaderVersion == D3DPS_VERSION(2, 1)) pixelShaderVersion = D3DPS_VERSION(2, 0);
		if(vertexShaderVersion == D3DVS_VERSION(2, 1)) vertexShaderVersion = D3DVS_VERSION(2, 0);

		unsigned int maximumVertexShaderInstructionsExecuted = 0;
		unsigned int maximumPixelShaderInstructionsExecuted = 0;
		unsigned int maximumVertexShader30InstructionSlots = 0;
		unsigned int maximumPixelShader30InstructionSlots = 0;

		if(vertexShaderVersion >= D3DVS_VERSION(1, 0)) maximumVertexShaderInstructionsExecuted = 0xFFFFFFFF;
		if(pixelShaderVersion >= D3DPS_VERSION(1, 0)) maximumPixelShaderInstructionsExecuted = 0xFFFFFFFF;
		if(vertexShaderVersion >= D3DVS_VERSION(3, 0)) maximumVertexShader30InstructionSlots = 32768;
		if(pixelShaderVersion >= D3DPS_VERSION(3, 0)) maximumPixelShader30InstructionSlots = 32768;

		D3DCAPS9 caps;
		ZeroMemory(&caps, sizeof(D3DCAPS9));

		// Device info
		caps.DeviceType = D3DDEVTYPE_HAL;
		caps.AdapterOrdinal = D3DADAPTER_DEFAULT;

		// Caps from DX7
		caps.Caps =	D3DCAPS_READ_SCANLINE;

		caps.Caps2 =		D3DCAPS2_CANAUTOGENMIPMAP |		// The driver is capable of automatically generating mipmaps. For more information, see Automatic Generation of Mipmaps (Direct3D 9).
						//	D3DCAPS2_CANCALIBRATEGAMMA |	// The system has a calibrator installed that can automatically adjust the gamma ramp so that the result is identical on all systems that have a calibrator. To invoke the calibrator when setting new gamma levels, use the D3DSGR_CALIBRATE flag when calling SetGammaRamp. Calibrating gamma ramps incurs some processing overhead and should not be used frequently.
						//	D3DCAPS2_CANSHARERESOURCE |		// The device can create sharable resources. Methods that create resources can set non-NULL values for their pSharedHandle parameters. Differences between Direct3D 9 and Direct3D 9Ex:This flag is available in Direct3D 9Ex only.
						//	D3DCAPS2_CANMANAGERESOURCE |	// The driver is capable of managing resources. On such drivers, D3DPOOL_MANAGED resources will be managed by the driver. To have Direct3D override the driver so that Direct3D manages resources, use the D3DCREATE_DISABLE_DRIVER_MANAGEMENT flag when calling CreateDevice.
							D3DCAPS2_DYNAMICTEXTURES | 		// The driver supports dynamic textures.
							D3DCAPS2_FULLSCREENGAMMA;		// The driver supports dynamic gamma ramp adjustment in full-screen mode.

		caps.Caps3 =		D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD |	// Indicates that the device can respect the D3DRS_ALPHABLENDENABLE render state in full-screen mode while using the FLIP or DISCARD swap effect.
							D3DCAPS3_COPY_TO_VIDMEM |					// Device can accelerate a memory copy from system memory to local video memory.
							D3DCAPS3_COPY_TO_SYSTEMMEM;					// Device can accelerate a memory copy from local video memory to system memory.
						//	D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION;		// Indicates that the device can perform gamma correction from a windowed back buffer (containing linear content) to an sRGB desktop.

		caps.PresentationIntervals =		D3DPRESENT_INTERVAL_IMMEDIATE |
											D3DPRESENT_INTERVAL_ONE;
										//	D3DPRESENT_INTERVAL_TWO;
										//	D3DPRESENT_INTERVAL_THREE;
										//	D3DPRESENT_INTERVAL_FOUR;

		// Cursor caps
		caps.CursorCaps =	D3DCURSORCAPS_COLOR	|	// A full-color cursor is supported in hardware. Specifically, this flag indicates that the driver supports at least a hardware color cursor in high-resolution modes (with scan lines greater than or equal to 400).
							D3DCURSORCAPS_LOWRES;	// A full-color cursor is supported in hardware. Specifically, this flag indicates that the driver supports a hardware color cursor in both high-resolution and low-resolution modes (with scan lines less than 400).

		// 3D Device caps
		caps.DevCaps =		D3DDEVCAPS_CANBLTSYSTONONLOCAL |		// Device supports blits from system-memory textures to nonlocal video-memory textures.
							D3DDEVCAPS_CANRENDERAFTERFLIP |			// Device can queue rendering commands after a page flip. Applications do not change their behavior if this flag is set; this capability means that the device is relatively fast.
							D3DDEVCAPS_DRAWPRIMITIVES2 |			// Device can support DrawPrimitives2.
							D3DDEVCAPS_DRAWPRIMITIVES2EX |			// Device can support extended DrawPrimitives2; that is, this is a DirectX 7.0-compliant driver.
							D3DDEVCAPS_DRAWPRIMTLVERTEX |			// Device exports an IDirect3DDevice9::DrawPrimitive-aware hardware abstraction layer (HAL).
							D3DDEVCAPS_EXECUTESYSTEMMEMORY |		// Device can use execute buffers from system memory.
							D3DDEVCAPS_EXECUTEVIDEOMEMORY |			// Device can use execute buffers from video memory.
							D3DDEVCAPS_HWRASTERIZATION |			// Device has hardware acceleration for scene rasterization.
							D3DDEVCAPS_HWTRANSFORMANDLIGHT |		// Device can support transformation and lighting in hardware.
						//	D3DDEVCAPS_NPATCHES |					// Device supports N patches.
							D3DDEVCAPS_PUREDEVICE |					// Device can support rasterization, transform, lighting, and shading in hardware.
						//	D3DDEVCAPS_QUINTICRTPATCHES |			// Device supports quintic Bézier curves and B-splines.
						//	D3DDEVCAPS_RTPATCHES |					// Device supports rectangular and triangular patches.
							D3DDEVCAPS_RTPATCHHANDLEZERO |			// When this device capability is set, the hardware architecture does not require caching of any information and uncached patches (handle zero) will be drawn as efficiently as cached ones. Note that setting D3DDEVCAPS_RTPATCHHANDLEZERO does not mean that a patch with handle zero can be drawn. A handle-zero patch can always be drawn whether this cap is set or not.
						//	D3DDEVCAPS_SEPARATETEXTUREMEMORIES |	// Device is texturing from separate memory pools.
							D3DDEVCAPS_TEXTURENONLOCALVIDMEM |		// Device can retrieve textures from non-local video memory.
							D3DDEVCAPS_TEXTURESYSTEMMEMORY |		// Device can retrieve textures from system memory.
							D3DDEVCAPS_TEXTUREVIDEOMEMORY |			// Device can retrieve textures from device memory.
							D3DDEVCAPS_TLVERTEXSYSTEMMEMORY |		// Device can use buffers from system memory for transformed and lit vertices.
							D3DDEVCAPS_TLVERTEXVIDEOMEMORY;			// Device can use buffers from video memory for transformed and lit vertices.

		caps.PrimitiveMiscCaps = 		D3DPMISCCAPS_MASKZ |						// Device can enable and disable modification of the depth buffer on pixel operations.
										D3DPMISCCAPS_CULLNONE |						// The driver does not perform triangle culling. This corresponds to the D3DCULL_NONE member of the D3DCULL enumerated type.
										D3DPMISCCAPS_CULLCW |						// The driver supports clockwise triangle culling through the D3DRS_CULLMODE state. (This applies only to triangle primitives.) This flag corresponds to the D3DCULL_CW member of the D3DCULL enumerated type.
										D3DPMISCCAPS_CULLCCW |						// The driver supports counterclockwise culling through the D3DRS_CULLMODE state. (This applies only to triangle primitives.) This flag corresponds to the D3DCULL_CCW member of the D3DCULL enumerated type.
										D3DPMISCCAPS_COLORWRITEENABLE |				// Device supports per-channel writes for the render-target color buffer through the D3DRS_COLORWRITEENABLE state.
										D3DPMISCCAPS_CLIPPLANESCALEDPOINTS |		// Device correctly clips scaled points of size greater than 1.0 to user-defined clipping planes.
										D3DPMISCCAPS_CLIPTLVERTS |					// Device clips post-transformed vertex primitives. Specify D3DUSAGE_DONOTCLIP when the pipeline should not do any clipping. For this case, additional software clipping may need to be performed at draw time, requiring the vertex buffer to be in system memory.
										D3DPMISCCAPS_TSSARGTEMP |					// Device supports D3DTA for temporary register.
										D3DPMISCCAPS_BLENDOP |						// Device supports alpha-blending operations other than D3DBLENDOP_ADD.
									//	D3DPMISCCAPS_NULLREFERENCE |				// A reference device that does not render.
										D3DPMISCCAPS_INDEPENDENTWRITEMASKS |		// Device supports independent write masks for multiple element textures or multiple render targets.
										D3DPMISCCAPS_PERSTAGECONSTANT |				// Device supports per-stage constants. See D3DTSS_CONSTANT in D3DTEXTURESTAGESTATETYPE.
										D3DPMISCCAPS_FOGANDSPECULARALPHA |			// Device supports separate fog and specular alpha. Many devices use the specular alpha channel to store the fog factor.
										D3DPMISCCAPS_SEPARATEALPHABLEND |			// Device supports separate blend settings for the alpha channel.
										D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS |		// Device supports different bit depths for multiple render targets.
										D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING |	// Device supports post-pixel shader operations for multiple render targets.
										D3DPMISCCAPS_FOGVERTEXCLAMPED;				// Device clamps fog blend factor per vertex.

		caps.RasterCaps =		D3DPRASTERCAPS_ANISOTROPY |				// Device supports anisotropic filtering.
								D3DPRASTERCAPS_COLORPERSPECTIVE |		// Device iterates colors perspective correctly.
							//	D3DPRASTERCAPS_DITHER |					// Device can dither to improve color resolution.
								D3DPRASTERCAPS_DEPTHBIAS |				// Device supports legacy depth bias. For true depth bias, see D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS.
								D3DPRASTERCAPS_FOGRANGE |				// Device supports range-based fog. In range-based fog, the distance of an object from the viewer is used to compute fog effects, not the depth of the object (that is, the z-coordinate) in the scene.
								D3DPRASTERCAPS_FOGTABLE |				// Device calculates the fog value by referring to a lookup table containing fog values that are indexed to the depth of a given pixel.
								D3DPRASTERCAPS_FOGVERTEX |				// Device calculates the fog value during the lighting operation and interpolates the fog value during rasterization.
								D3DPRASTERCAPS_MIPMAPLODBIAS |			// Device supports level of detail (LOD) bias adjustments. These bias adjustments enable an application to make a mipmap appear crisper or less sharp than it normally would. For more information about LOD bias in mipmaps, see D3DSAMP_MIPMAPLODBIAS.
							//	D3DPRASTERCAPS_MULTISAMPLE_TOGGLE |		// Device supports toggling multisampling on and off between IDirect3DDevice9::BeginScene and IDirect3DDevice9::EndScene (using D3DRS_MULTISAMPLEANTIALIAS).
								D3DPRASTERCAPS_SCISSORTEST |			// Device supports scissor test. See Scissor Test.
								D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS |	// Device performs true slope-scale based depth bias. This is in contrast to the legacy style D3DPRASTERCAPS_DEPTHBIAS.
							//	D3DPRASTERCAPS_WBUFFER |				// Device supports depth buffering using w.
								D3DPRASTERCAPS_WFOG |					// Device supports w-based fog. W-based fog is used when a perspective projection matrix is specified, but affine projections still use z-based fog. The system considers a projection matrix that contains a nonzero value in the [3][4] element to be a perspective projection matrix.
							//	D3DPRASTERCAPS_ZBUFFERLESSHSR |			// Device can perform hidden-surface removal (HSR) without requiring the application to sort polygons and without requiring the allocation of a depth-buffer. This leaves more video memory for textures. The method used to perform HSR is hardware-dependent and is transparent to the application. Z-bufferless HSR is performed if no depth-buffer surface is associated with the rendering-target surface and the depth-buffer comparison test is enabled (that is, when the state value associated with the D3DRS_ZENABLE enumeration constant is set to TRUE).
								D3DPRASTERCAPS_ZFOG |					// Device supports z-based fog.
								D3DPRASTERCAPS_ZTEST;					// Device can perform z-test operations. This effectively renders a primitive and indicates whether any z pixels have been rendered.

		caps.ZCmpCaps =		D3DPCMPCAPS_ALWAYS |		// Always pass the z-test.
							D3DPCMPCAPS_EQUAL |			// Pass the z-test if the new z equals the current z.
							D3DPCMPCAPS_GREATER |		// Pass the z-test if the new z is greater than the current z.
							D3DPCMPCAPS_GREATEREQUAL |	// Pass the z-test if the new z is greater than or equal to the current z.
							D3DPCMPCAPS_LESS |			// Pass the z-test if the new z is less than the current z.
							D3DPCMPCAPS_LESSEQUAL |		// Pass the z-test if the new z is less than or equal to the current z.
							D3DPCMPCAPS_NEVER |			// Always fail the z-test.
							D3DPCMPCAPS_NOTEQUAL;		// Pass the z-test if the new z does not equal the current z.

		caps.SrcBlendCaps =		D3DPBLENDCAPS_BLENDFACTOR |		// The driver supports both D3DBLEND_BLENDFACTOR and D3DBLEND_INVBLENDFACTOR. See D3DBLEND.
								D3DPBLENDCAPS_BOTHINVSRCALPHA |	// Source blend factor is (1-As,1-As,1-As,1-As) and destination blend factor is (As,As,As,As); the destination blend selection is overridden.
								D3DPBLENDCAPS_BOTHSRCALPHA |	// The driver supports the D3DBLEND_BOTHSRCALPHA blend mode. (This blend mode is obsolete. For more information, see D3DBLEND.)
								D3DPBLENDCAPS_DESTALPHA |		// Blend factor is (Ad, Ad, Ad, Ad).
								D3DPBLENDCAPS_DESTCOLOR |		// Blend factor is (Rd, Gd, Bd, Ad).
								D3DPBLENDCAPS_INVDESTALPHA |	// Blend factor is (1-Ad, 1-Ad, 1-Ad, 1-Ad).
								D3DPBLENDCAPS_INVDESTCOLOR |	// Blend factor is (1-Rd, 1-Gd, 1-Bd, 1-Ad).
								D3DPBLENDCAPS_INVSRCALPHA |		// Blend factor is (1-As, 1-As, 1-As, 1-As).
								D3DPBLENDCAPS_INVSRCCOLOR |		// Blend factor is (1-Rs, 1-Gs, 1-Bs, 1-As).
								D3DPBLENDCAPS_ONE |				// Blend factor is (1, 1, 1, 1).
								D3DPBLENDCAPS_SRCALPHA |		// Blend factor is (As, As, As, As).
								D3DPBLENDCAPS_SRCALPHASAT |		// Blend factor is (f, f, f, 1); f = min(As, 1-Ad).
								D3DPBLENDCAPS_SRCCOLOR |		// Blend factor is (Rs, Gs, Bs, As).
								D3DPBLENDCAPS_ZERO;				// Blend factor is (0, 0, 0, 0).

		caps.DestBlendCaps = 	D3DPBLENDCAPS_BLENDFACTOR |		// The driver supports both D3DBLEND_BLENDFACTOR and D3DBLEND_INVBLENDFACTOR. See D3DBLEND.
								D3DPBLENDCAPS_BOTHINVSRCALPHA |	// Source blend factor is (1-As,1-As,1-As,1-As) and destination blend factor is (As,As,As,As); the destination blend selection is overridden.
								D3DPBLENDCAPS_BOTHSRCALPHA |	// The driver supports the D3DBLEND_BOTHSRCALPHA blend mode. (This blend mode is obsolete. For more information, see D3DBLEND.)
								D3DPBLENDCAPS_DESTALPHA |		// Blend factor is (Ad, Ad, Ad, Ad).
								D3DPBLENDCAPS_DESTCOLOR |		// Blend factor is (Rd, Gd, Bd, Ad).
								D3DPBLENDCAPS_INVDESTALPHA |	// Blend factor is (1-Ad, 1-Ad, 1-Ad, 1-Ad).
								D3DPBLENDCAPS_INVDESTCOLOR |	// Blend factor is (1-Rd, 1-Gd, 1-Bd, 1-Ad).
								D3DPBLENDCAPS_INVSRCALPHA |		// Blend factor is (1-As, 1-As, 1-As, 1-As).
								D3DPBLENDCAPS_INVSRCCOLOR |		// Blend factor is (1-Rs, 1-Gs, 1-Bs, 1-As).
								D3DPBLENDCAPS_ONE |				// Blend factor is (1, 1, 1, 1).
								D3DPBLENDCAPS_SRCALPHA |		// Blend factor is (As, As, As, As).
								D3DPBLENDCAPS_SRCALPHASAT |		// Blend factor is (f, f, f, 1); f = min(As, 1-Ad).
								D3DPBLENDCAPS_SRCCOLOR |		// Blend factor is (Rs, Gs, Bs, As).
								D3DPBLENDCAPS_ZERO;				// Blend factor is (0, 0, 0, 0).

		caps.AlphaCmpCaps = D3DPCMPCAPS_ALWAYS |		// Always pass the apha-test.
							D3DPCMPCAPS_EQUAL |			// Pass the apha-test if the new apha equals the current apha.
							D3DPCMPCAPS_GREATER |		// Pass the apha-test if the new apha is greater than the current apha.
							D3DPCMPCAPS_GREATEREQUAL |	// Pass the apha-test if the new apha is greater than or equal to the current apha.
							D3DPCMPCAPS_LESS |			// Pass the apha-test if the new apha is less than the current apha.
							D3DPCMPCAPS_LESSEQUAL |		// Pass the apha-test if the new apha is less than or equal to the current apha.
							D3DPCMPCAPS_NEVER |			// Always fail the apha-test.
							D3DPCMPCAPS_NOTEQUAL;		// Pass the apha-test if the new apha does not equal the current apha.

		caps.ShadeCaps =	D3DPSHADECAPS_ALPHAGOURAUDBLEND |	// Device can support an alpha component for Gouraud-blended transparency (the D3DSHADE_GOURAUD state for the D3DSHADEMODE enumerated type). In this mode, the alpha color component of a primitive is provided at vertices and interpolated across a face along with the other color components.
							D3DPSHADECAPS_COLORGOURAUDRGB |		// Device can support colored Gouraud shading in the RGB color model. In this mode, the color component for a primitive is provided at vertices and interpolated across a face along with the other color components. In the RGB lighting model, the red, green, and blue components are interpolated.
							D3DPSHADECAPS_FOGGOURAUD |			// Device can support fog in the Gouraud shading mode.
							D3DPSHADECAPS_SPECULARGOURAUDRGB;	// Device supports Gouraud shading of specular highlights.

		caps.TextureCaps =		D3DPTEXTURECAPS_ALPHA |						// Alpha in texture pixels is supported.
								D3DPTEXTURECAPS_ALPHAPALETTE |				// Device can draw alpha from texture palettes.
								D3DPTEXTURECAPS_CUBEMAP |					// Supports cube textures.
							//	D3DPTEXTURECAPS_CUBEMAP_POW2 |				// Device requires that cube texture maps have dimensions specified as powers of two.
								D3DPTEXTURECAPS_MIPCUBEMAP |				// Device supports mipmapped cube textures.
								D3DPTEXTURECAPS_MIPMAP |					// Device supports mipmapped textures.
								D3DPTEXTURECAPS_MIPVOLUMEMAP |				// Device supports mipmapped volume textures.
							//	D3DPTEXTURECAPS_NONPOW2CONDITIONAL |		// Conditionally supports the use of 2-D textures with dimensions that are not powers of two. A device that exposes this capability can use such a texture if all of the following requirements are met...
							//	D3DPTEXTURECAPS_NOPROJECTEDBUMPENV |		// Device does not support a projected bump-environment loopkup operation in programmable and fixed function shaders.
								D3DPTEXTURECAPS_PERSPECTIVE |				// Perspective correction texturing is supported.
							//	D3DPTEXTURECAPS_POW2 |						// All textures must have widths and heights specified as powers of two. This requirement does not apply to either cube textures or volume textures.
								D3DPTEXTURECAPS_PROJECTED |					// Supports the D3DTTFF_PROJECTED texture transformation flag. When applied, the device divides transformed texture coordinates by the last texture coordinate. If this capability is present, then the projective divide occurs per pixel. If this capability is not present, but the projective divide needs to occur anyway, then it is performed on a per-vertex basis by the Direct3D runtime.
							//	D3DPTEXTURECAPS_SQUAREONLY |				// All textures must be square.
								D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE |	// Texture indices are not scaled by the texture size prior to interpolation.
								D3DPTEXTURECAPS_VOLUMEMAP;					// Device supports volume textures.
							//	D3DPTEXTURECAPS_VOLUMEMAP_POW2;				// Device requires that volume texture maps have dimensions specified as powers of two.

		caps.TextureFilterCaps =		D3DPTFILTERCAPS_MAGFPOINT |			// Device supports per-stage point-sample filtering for magnifying textures. The point-sample magnification filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type.
										D3DPTFILTERCAPS_MAGFLINEAR |		// Device supports per-stage bilinear interpolation filtering for magnifying mipmaps. The bilinear interpolation mipmapping filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type.
									//	D3DPTFILTERCAPS_MAGFANISOTROPIC |	// Device supports per-stage anisotropic filtering for magnifying textures. The anisotropic magnification filter is represented by the D3DTEXF_ANISOTROPIC member of the D3DTEXTUREFILTERTYPE enumerated type.
									//	D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD |	// Device supports per-stage pyramidal sample filtering for magnifying textures. The pyramidal magnifying filter is represented by the D3DTEXF_PYRAMIDALQUAD member of the D3DTEXTUREFILTERTYPE enumerated type.
									//	D3DPTFILTERCAPS_MAGFGAUSSIANQUAD |	// Device supports per-stage Gaussian quad filtering for magnifying textures.
										D3DPTFILTERCAPS_MINFPOINT |			// Device supports per-stage point-sample filtering for minifying textures. The point-sample minification filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type.
										D3DPTFILTERCAPS_MINFLINEAR |		// Device supports per-stage linear filtering for minifying textures. The linear minification filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type.
										D3DPTFILTERCAPS_MINFANISOTROPIC |	// Device supports per-stage anisotropic filtering for minifying textures. The anisotropic minification filter is represented by the D3DTEXF_ANISOTROPIC member of the D3DTEXTUREFILTERTYPE enumerated type.
									//	D3DPTFILTERCAPS_MINFPYRAMIDALQUAD |	// Device supports per-stage pyramidal sample filtering for minifying textures.
									//	D3DPTFILTERCAPS_MINFGAUSSIANQUAD |	// Device supports per-stage Gaussian quad filtering for minifying textures.
										D3DPTFILTERCAPS_MIPFPOINT |			// Device supports per-stage point-sample filtering for mipmaps. The point-sample mipmapping filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type.
										D3DPTFILTERCAPS_MIPFLINEAR;			// Device supports per-stage bilinear interpolation filtering for mipmaps. The bilinear interpolation mipmapping filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type.

		caps.CubeTextureFilterCaps =		D3DPTFILTERCAPS_MAGFPOINT |			// Device supports per-stage point-sample filtering for magnifying textures. The point-sample magnification filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type.
											D3DPTFILTERCAPS_MAGFLINEAR |		// Device supports per-stage bilinear interpolation filtering for magnifying mipmaps. The bilinear interpolation mipmapping filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MAGFANISOTROPIC |	// Device supports per-stage anisotropic filtering for magnifying textures. The anisotropic magnification filter is represented by the D3DTEXF_ANISOTROPIC member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD |	// Device supports per-stage pyramidal sample filtering for magnifying textures. The pyramidal magnifying filter is represented by the D3DTEXF_PYRAMIDALQUAD member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MAGFGAUSSIANQUAD |	// Device supports per-stage Gaussian quad filtering for magnifying textures.
											D3DPTFILTERCAPS_MINFPOINT |			// Device supports per-stage point-sample filtering for minifying textures. The point-sample minification filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type.
											D3DPTFILTERCAPS_MINFLINEAR |		// Device supports per-stage linear filtering for minifying textures. The linear minification filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MINFANISOTROPIC |	// Device supports per-stage anisotropic filtering for minifying textures. The anisotropic minification filter is represented by the D3DTEXF_ANISOTROPIC member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MINFPYRAMIDALQUAD |	// Device supports per-stage pyramidal sample filtering for minifying textures.
										//	D3DPTFILTERCAPS_MINFGAUSSIANQUAD |	// Device supports per-stage Gaussian quad filtering for minifying textures.
											D3DPTFILTERCAPS_MIPFPOINT |			// Device supports per-stage point-sample filtering for mipmaps. The point-sample mipmapping filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type.
											D3DPTFILTERCAPS_MIPFLINEAR;			// Device supports per-stage bilinear interpolation filtering for mipmaps. The bilinear interpolation mipmapping filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type.

		caps.VolumeTextureFilterCaps = 		D3DPTFILTERCAPS_MAGFPOINT |			// Device supports per-stage point-sample filtering for magnifying textures. The point-sample magnification filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type.
											D3DPTFILTERCAPS_MAGFLINEAR |		// Device supports per-stage bilinear interpolation filtering for magnifying mipmaps. The bilinear interpolation mipmapping filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MAGFANISOTROPIC |	// Device supports per-stage anisotropic filtering for magnifying textures. The anisotropic magnification filter is represented by the D3DTEXF_ANISOTROPIC member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD |	// Device supports per-stage pyramidal sample filtering for magnifying textures. The pyramidal magnifying filter is represented by the D3DTEXF_PYRAMIDALQUAD member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MAGFGAUSSIANQUAD |	// Device supports per-stage Gaussian quad filtering for magnifying textures.
											D3DPTFILTERCAPS_MINFPOINT |			// Device supports per-stage point-sample filtering for minifying textures. The point-sample minification filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type.
											D3DPTFILTERCAPS_MINFLINEAR |		// Device supports per-stage linear filtering for minifying textures. The linear minification filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MINFANISOTROPIC |	// Device supports per-stage anisotropic filtering for minifying textures. The anisotropic minification filter is represented by the D3DTEXF_ANISOTROPIC member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MINFPYRAMIDALQUAD |	// Device supports per-stage pyramidal sample filtering for minifying textures.
										//	D3DPTFILTERCAPS_MINFGAUSSIANQUAD |	// Device supports per-stage Gaussian quad filtering for minifying textures.
											D3DPTFILTERCAPS_MIPFPOINT |			// Device supports per-stage point-sample filtering for mipmaps. The point-sample mipmapping filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type.
											D3DPTFILTERCAPS_MIPFLINEAR;			// Device supports per-stage bilinear interpolation filtering for mipmaps. The bilinear interpolation mipmapping filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type.

		caps.TextureAddressCaps =	D3DPTADDRESSCAPS_BORDER |			// Device supports setting coordinates outside the range [0.0, 1.0] to the border color, as specified by the D3DSAMP_BORDERCOLOR texture-stage state.
									D3DPTADDRESSCAPS_CLAMP |			// Device can clamp textures to addresses.
									D3DPTADDRESSCAPS_INDEPENDENTUV |	// Device can separate the texture-addressing modes of the u and v coordinates of the texture. This ability corresponds to the D3DSAMP_ADDRESSU and D3DSAMP_ADDRESSV render-state values.
									D3DPTADDRESSCAPS_MIRROR |			// Device can mirror textures to addresses.
									D3DPTADDRESSCAPS_MIRRORONCE |		// Device can take the absolute value of the texture coordinate (thus, mirroring around 0) and then clamp to the maximum value.
									D3DPTADDRESSCAPS_WRAP;				// Device can wrap textures to addresses.

		caps.VolumeTextureAddressCaps =	D3DPTADDRESSCAPS_BORDER |			// Device supports setting coordinates outside the range [0.0, 1.0] to the border color, as specified by the D3DSAMP_BORDERCOLOR texture-stage state.
										D3DPTADDRESSCAPS_CLAMP |			// Device can clamp textures to addresses.
										D3DPTADDRESSCAPS_INDEPENDENTUV |	// Device can separate the texture-addressing modes of the u and v coordinates of the texture. This ability corresponds to the D3DSAMP_ADDRESSU and D3DSAMP_ADDRESSV render-state values.
										D3DPTADDRESSCAPS_MIRROR |			// Device can mirror textures to addresses.
										D3DPTADDRESSCAPS_MIRRORONCE |		// Device can take the absolute value of the texture coordinate (thus, mirroring around 0) and then clamp to the maximum value.
										D3DPTADDRESSCAPS_WRAP;				// Device can wrap textures to addresses.

		caps.LineCaps =		D3DLINECAPS_ALPHACMP |	// Supports alpha-test comparisons.
						//	D3DLINECAPS_ANTIALIAS | // Antialiased lines are supported.
							D3DLINECAPS_BLEND |		// Supports source-blending.
							D3DLINECAPS_FOG |		// Supports fog.
							D3DLINECAPS_TEXTURE |	// Supports texture-mapping.
							D3DLINECAPS_ZTEST;		// Supports z-buffer comparisons.

		caps.MaxTextureWidth = 1 << (sw::MIPMAP_LEVELS - 1);
		caps.MaxTextureHeight = 1 << (sw::MIPMAP_LEVELS - 1);
		caps.MaxVolumeExtent = 1 << (sw::MIPMAP_LEVELS - 1);
		caps.MaxTextureRepeat = 8192;
		caps.MaxTextureAspectRatio = 1 << (sw::MIPMAP_LEVELS - 1);
		caps.MaxAnisotropy = maxAnisotropy;
		caps.MaxVertexW = 1e+010;

		caps.GuardBandLeft = -1e+008;
		caps.GuardBandTop = -1e+008;
		caps.GuardBandRight = 1e+008;
		caps.GuardBandBottom = 1e+008;

		caps.ExtentsAdjust = 0;

		caps.StencilCaps =		D3DSTENCILCAPS_KEEP |		// Do not update the entry in the stencil buffer. This is the default value.
								D3DSTENCILCAPS_ZERO |		// Set the stencil-buffer entry to 0.
								D3DSTENCILCAPS_REPLACE |	// Replace the stencil-buffer entry with reference value.
								D3DSTENCILCAPS_INCRSAT |	// Increment the stencil-buffer entry, clamping to the maximum value.
								D3DSTENCILCAPS_DECRSAT |	// Decrement the stencil-buffer entry, clamping to zero.
								D3DSTENCILCAPS_INVERT |		// Invert the bits in the stencil-buffer entry.
								D3DSTENCILCAPS_INCR |		// Increment the stencil-buffer entry, wrapping to zero if the new value exceeds the maximum value.
								D3DSTENCILCAPS_DECR |		// Decrement the stencil-buffer entry, wrapping to the maximum value if the new value is less than zero.
								D3DSTENCILCAPS_TWOSIDED;	// The device supports two-sided stencil.

		caps.FVFCaps =		D3DFVFCAPS_DONOTSTRIPELEMENTS |		// It is preferable that vertex elements not be stripped. That is, if the vertex format contains elements that are not used with the current render states, there is no need to regenerate the vertices. If this capability flag is not present, stripping extraneous elements from the vertex format provides better performance.
							D3DFVFCAPS_PSIZE |					// Point size is determined by either the render state or the vertex data.
						//	D3DFVFCAPS_TEXCOORDCOUNTMASK |		// Masks the low WORD of FVFCaps. These bits, cast to the WORD data type, describe the total number of texture coordinate sets that the device can simultaneously use for multiple texture blending. (You can use up to eight texture coordinate sets for any vertex, but the device can blend using only the specified number of texture coordinate sets.)
							8;

		caps.TextureOpCaps =	D3DTEXOPCAPS_ADD |							// The D3DTOP_ADD texture-blending operation is supported.
								D3DTEXOPCAPS_ADDSIGNED |					// The D3DTOP_ADDSIGNED texture-blending operation is supported.
								D3DTEXOPCAPS_ADDSIGNED2X |					// The D3DTOP_ADDSIGNED2X texture-blending operation is supported.
								D3DTEXOPCAPS_ADDSMOOTH |					// The D3DTOP_ADDSMOOTH texture-blending operation is supported.
								D3DTEXOPCAPS_BLENDCURRENTALPHA |			// The D3DTOP_BLENDCURRENTALPHA texture-blending operation is supported.
								D3DTEXOPCAPS_BLENDDIFFUSEALPHA |			// The D3DTOP_BLENDDIFFUSEALPHA texture-blending operation is supported.
								D3DTEXOPCAPS_BLENDFACTORALPHA |				// The D3DTOP_BLENDFACTORALPHA texture-blending operation is supported.
								D3DTEXOPCAPS_BLENDTEXTUREALPHA |			// The D3DTOP_BLENDTEXTUREALPHA texture-blending operation is supported.
								D3DTEXOPCAPS_BLENDTEXTUREALPHAPM |			// The D3DTOP_BLENDTEXTUREALPHAPM texture-blending operation is supported.
								D3DTEXOPCAPS_BUMPENVMAP |					// The D3DTOP_BUMPENVMAP texture-blending operation is supported.
								D3DTEXOPCAPS_BUMPENVMAPLUMINANCE |			// The D3DTOP_BUMPENVMAPLUMINANCE texture-blending operation is supported.
								D3DTEXOPCAPS_DISABLE |						// The D3DTOP_DISABLE texture-blending operation is supported.
								D3DTEXOPCAPS_DOTPRODUCT3 |					// The D3DTOP_DOTPRODUCT3 texture-blending operation is supported.
								D3DTEXOPCAPS_LERP |							// The D3DTOP_LERP texture-blending operation is supported.
								D3DTEXOPCAPS_MODULATE |						// The D3DTOP_MODULATE texture-blending operation is supported.
								D3DTEXOPCAPS_MODULATE2X |					// The D3DTOP_MODULATE2X texture-blending operation is supported.
								D3DTEXOPCAPS_MODULATE4X |					// The D3DTOP_MODULATE4X texture-blending operation is supported.
								D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR |		// The D3DTOP_MODULATEALPHA_ADDCOLOR texture-blending operation is supported.
								D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA |		// The D3DTOP_MODULATECOLOR_ADDALPHA texture-blending operation is supported.
								D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR |	// The D3DTOP_MODULATEINVALPHA_ADDCOLOR texture-blending operation is supported.
								D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA |	// The D3DTOP_MODULATEINVCOLOR_ADDALPHA texture-blending operation is supported.
								D3DTEXOPCAPS_MULTIPLYADD |					// The D3DTOP_MULTIPLYADD texture-blending operation is supported.
								D3DTEXOPCAPS_PREMODULATE |					// The D3DTOP_PREMODULATE texture-blending operation is supported.
								D3DTEXOPCAPS_SELECTARG1 |					// The D3DTOP_SELECTARG1 texture-blending operation is supported.
								D3DTEXOPCAPS_SELECTARG2 |					// The D3DTOP_SELECTARG2 texture-blending operation is supported.
								D3DTEXOPCAPS_SUBTRACT;						// The D3DTOP_SUBTRACT texture-blending operation is supported.

		caps.MaxTextureBlendStages = 8;
		caps.MaxSimultaneousTextures = 8;

		caps.VertexProcessingCaps =		D3DVTXPCAPS_DIRECTIONALLIGHTS |			// Device can do directional lights.
										D3DVTXPCAPS_LOCALVIEWER |				// Device can do local viewer.
										D3DVTXPCAPS_MATERIALSOURCE7	|			// Device can do Microsoft® DirectX® 7.0 colormaterialsource operations.
									//	D3DVTXPCAPS_NO_TEXGEN_NONLOCALVIEWER |	// Device does not support texture generation in non-local viewer mode.
										D3DVTXPCAPS_POSITIONALLIGHTS |			// Device can do positional lights (includes point and spot).
										D3DVTXPCAPS_TEXGEN |					// Device can do texgen.
										D3DVTXPCAPS_TEXGEN_SPHEREMAP;			// Device supports D3DTSS_TCI_SPHEREMAP.
									//	D3DVTXPCAPS_TWEENING;					// Device can do vertex tweening.

		caps.MaxActiveLights = 8;						// Maximum number of lights that can be active simultaneously. For a given physical device, this capability might vary across Direct3DDevice objects depending on the parameters supplied to IDirect3D9::CreateDevice.
		caps.MaxUserClipPlanes = 6;						// Maximum number of user-defined clipping planes supported. This member can range from 0 through D3DMAXUSERCLIPPLANES. For a given physical device, this capability may vary across Direct3DDevice objects depending on the parameters supplied to IDirect3D9::CreateDevice.
		caps.MaxVertexBlendMatrices = 4;				// Maximum number of matrices that this device can apply when performing multimatrix vertex blending. For a given physical device, this capability may vary across Direct3DDevice objects depending on the parameters supplied to IDirect3D9::CreateDevice.
		caps.MaxVertexBlendMatrixIndex = 11;			// DWORD value that specifies the maximum matrix index that can be indexed into using the per-vertex indices. The number of matrices is MaxVertexBlendMatrixIndex + 1, which is the size of the matrix palette. If normals are present in the vertex data that needs to be blended for lighting, then the number of matrices is half the number specified by this capability flag. If MaxVertexBlendMatrixIndex is set to zero, the driver does not support indexed vertex blending. If this value is not zero then the valid range of indices is zero through MaxVertexBlendMatrixIndex.
		caps.MaxPointSize = 8192.0f;					// Maximum size of a point primitive. If set to 1.0f then device does not support point size control. The range is greater than or equal to 1.0f.
		caps.MaxPrimitiveCount = 1 << 21;				// Maximum number of primitives for each IDirect3DDevice9::DrawPrimitive call. Note that when Direct3D is working with a DirectX 6.0 or DirectX 7.0 driver, this field is set to 0xFFFF. This means that not only the number of primitives but also the number of vertices is limited by this value.
		caps.MaxVertexIndex = 1 << 24;					// Maximum size of indices supported for hardware vertex processing. It is possible to create 32-bit index buffers by specifying D3DFMT_INDEX32; however, you will not be able to render with the index buffer unless this value is greater than 0x0000FFFF.
		caps.MaxStreams = 16;							// Maximum number of concurrent data streams for IDirect3DDevice9::SetStreamSource. The valid range is 1 to 16. Note that if this value is 0, then the driver is not a DirectX 9.0 driver.
		caps.MaxStreamStride = 65536;					// Maximum stride for IDirect3DDevice9::SetStreamSource.
		caps.VertexShaderVersion = vertexShaderVersion;	// Two numbers that represent the vertex shader main and sub versions. For more information about the instructions supported for each vertex shader version, see Version 1_x, Version 2_0, Version 2_0 Extended, or Version 3_0.
		caps.MaxVertexShaderConst = 256;				// The number of vertex shader Registers that are reserved for constants.
		caps.PixelShaderVersion = pixelShaderVersion;	// Two numbers that represent the pixel shader main and sub versions. For more information about the instructions supported for each pixel shader version, see Version 1_x, Version 2_0, Version 2_0 Extended, or Version 3_0.
		caps.PixelShader1xMaxValue = 8.0;				// Maximum value of pixel shader arithmetic component. This value indicates the internal range of values supported for pixel color blending operations. Within the range that they report to, implementations must allow data to pass through pixel processing unmodified (unclamped). Normally, the value of this member is an absolute value. For example, a 1.0 indicates that the range is -1.0 to 1, and an 8.0 indicates that the range is -8.0 to 8.0. The value must be >= 1.0 for any hardware that supports pixel shaders.

		caps.DevCaps2 = //	D3DDEVCAPS2_ADAPTIVETESSRTPATCH |				// Device supports adaptive tessellation of RT-patches
						//	D3DDEVCAPS2_ADAPTIVETESSNPATCH |				// Device supports adaptive tessellation of N-patches.
							D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES |		// Device supports IDirect3DDevice9::StretchRect using a texture as the source.
						//	D3DDEVCAPS2_DMAPNPATCH |						// Device supports displacement maps for N-patches.
						//	D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH |				// Device supports presampled displacement maps for N-patches. For more information about displacement mapping, see Displacement Mapping.
							D3DDEVCAPS2_STREAMOFFSET |						// Device supports stream offsets.
							D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET;	// Multiple vertex elements can share the same offset in a stream if D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET is set by the device and the vertex declaration does not have an element with D3DDECLUSAGE_POSITIONT0.

		caps.MaxNpatchTessellationLevel = 0;	// Maximum number of N-patch subdivision levels. The driver will clamp applications to this value, if they are using presampled displacement mapping. See Tessellation and Displacement Mapping.
		caps.MasterAdapterOrdinal = 0;			// This number indicates which device is the master for this subordinate. This number is taken from the same space as the adapter values passed to the IDirect3D9 methods.
		caps.AdapterOrdinalInGroup = 0;			// This number indicates the order in which heads are referenced by the application programming interface (API). The master adapter always has AdapterOrdinalInGroup = 0. These values do not correspond to the adapter ordinals passed to the IDirect3D9 methods. They apply only to heads within a group.
		caps.NumberOfAdaptersInGroup = 1;		// Number of adapters in this adapter group (only if master). This will be 1 for conventional adapters. The value will be greater than 1 for the master adapter of a multihead card. The value will be 0 for a subordinate adapter of a multihead card. Each card can have at most one master, but may have many subordinates.

		caps.DeclTypes =	D3DDTCAPS_UBYTE4 |		// 4-D unsigned byte.
							D3DDTCAPS_UBYTE4N |		// Normalized, 4-D unsigned byte. Each of the four bytes is normalized by dividing to 255.0.
							D3DDTCAPS_SHORT2N |		// Normalized, 2-D signed short, expanded to (first byte/32767.0, second byte/32767.0, 0, 1).
							D3DDTCAPS_SHORT4N |		// Normalized, 4-D signed short, expanded to (first byte/32767.0, second byte/32767.0, third byte/32767.0, fourth byte/32767.0).
							D3DDTCAPS_USHORT2N |	// Normalized, 2-D unsigned short, expanded to (first byte/65535.0, second byte/65535.0, 0, 1).
							D3DDTCAPS_USHORT4N |	// Normalized 4-D unsigned short, expanded to (first byte/65535.0, second byte/65535.0, third byte/65535.0, fourth byte/65535.0).
							D3DDTCAPS_UDEC3 |		// 3-D unsigned 10 10 10 format expanded to (value, value, value, 1).
							D3DDTCAPS_DEC3N |		// 3-D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1).
							D3DDTCAPS_FLOAT16_2 |	// 2-D 16-bit floating point numbers.
							D3DDTCAPS_FLOAT16_4;	// 4-D 16-bit floating point numbers.

		caps.NumSimultaneousRTs = 4;	// Number of simultaneous render targets. This number must be at least one.

		caps.StretchRectFilterCaps = 	D3DPTFILTERCAPS_MINFPOINT |		// Device supports point-sample filtering for minifying rectangles. This filter type is requested by calling IDirect3DDevice9::StretchRect using D3DTEXF_POINT.
										D3DPTFILTERCAPS_MAGFPOINT |		// Device supports point-sample filtering for magnifying rectangles. This filter type is requested by calling IDirect3DDevice9::StretchRect using D3DTEXF_POINT.
										D3DPTFILTERCAPS_MINFLINEAR |	// Device supports bilinear interpolation filtering for minifying rectangles. This filter type is requested by calling IDirect3DDevice9::StretchRect using D3DTEXF_LINEAR.
										D3DPTFILTERCAPS_MAGFLINEAR;		// Device supports bilinear interpolation filtering for magnifying rectangles. This filter type is requested by calling IDirect3DDevice9::StretchRect using D3DTEXF_LINEAR.

		caps.VS20Caps.Caps = vertexShaderPredication;									// Instruction predication is supported. See setp_comp.
		caps.VS20Caps.DynamicFlowControlDepth = vertexShaderDynamicFlowControlDepth;	// The maximum level of nesting of dynamic flow control instructions (break, breakc, ifc).
		caps.VS20Caps.NumTemps = D3DVS20_MAX_NUMTEMPS;									// The maximum number of temporary registers supported.
		caps.VS20Caps.StaticFlowControlDepth = D3DVS20_MAX_STATICFLOWCONTROLDEPTH;		// The maximum depth of nesting of the loop/rep and call/callnz bool instructions.

		caps.PS20Caps.Caps =	pixelShaderArbitrarySwizzle |						// Arbitrary swizzling is supported.
								pixelShaderGradientInstructions |					// Gradient instructions are supported.
								pixelShaderPredication |							// Instruction predication is supported. See setp_comp - ps.
								pixelShaderNoDependentReadLimit |					// There is no limit on the number of dependent reads per instruction.
								pixelShaderNoTexInstructionLimit;					// There is no limit on the number of tex instructions.

		caps.PS20Caps.DynamicFlowControlDepth = pixelShaderDynamicFlowControlDepth;		// The maximum level of nesting of dynamic flow control instructions (break, breakc, ifc).
		caps.PS20Caps.NumInstructionSlots = D3DPS20_MAX_NUMINSTRUCTIONSLOTS;			// The driver will support at most this many instructions.
		caps.PS20Caps.NumTemps = D3DPS20_MAX_NUMTEMPS;									// The driver will support at most this many temporary register.
		caps.PS20Caps.StaticFlowControlDepth = pixelShaderStaticFlowControlDepth;		// The maximum depth of nesting of the loop/rep and call/callnz bool instructions.

		caps.VertexTextureFilterCaps =		D3DPTFILTERCAPS_MAGFPOINT |			// Device supports per-stage point-sample filtering for magnifying textures. The point-sample magnification filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MAGFLINEAR |		// Device supports per-stage bilinear interpolation filtering for magnifying mipmaps. The bilinear interpolation mipmapping filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MAGFANISOTROPIC |	// Device supports per-stage anisotropic filtering for magnifying textures. The anisotropic magnification filter is represented by the D3DTEXF_ANISOTROPIC member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD |	// Device supports per-stage pyramidal sample filtering for magnifying textures. The pyramidal magnifying filter is represented by the D3DTEXF_PYRAMIDALQUAD member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MAGFGAUSSIANQUAD |	// Device supports per-stage Gaussian quad filtering for magnifying textures.
											D3DPTFILTERCAPS_MINFPOINT |			// Device supports per-stage point-sample filtering for minifying textures. The point-sample minification filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MINFLINEAR |		// Device supports per-stage linear filtering for minifying textures. The linear minification filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MINFANISOTROPIC |	// Device supports per-stage anisotropic filtering for minifying textures. The anisotropic minification filter is represented by the D3DTEXF_ANISOTROPIC member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MINFPYRAMIDALQUAD |	// Device supports per-stage pyramidal sample filtering for minifying textures.
										//	D3DPTFILTERCAPS_MINFGAUSSIANQUAD |	// Device supports per-stage Gaussian quad filtering for minifying textures.
											D3DPTFILTERCAPS_MIPFPOINT;			// Device supports per-stage point-sample filtering for mipmaps. The point-sample mipmapping filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type.
										//	D3DPTFILTERCAPS_MIPFLINEAR;			// Device supports per-stage bilinear interpolation filtering for mipmaps. The bilinear interpolation mipmapping filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type.

		caps.MaxVShaderInstructionsExecuted = maximumVertexShaderInstructionsExecuted;	// Maximum number of vertex shader instructions that can be run.
		caps.MaxPShaderInstructionsExecuted = maximumPixelShaderInstructionsExecuted;	// Maximum number of pixel shader instructions that can be run.
		caps.MaxVertexShader30InstructionSlots = maximumVertexShader30InstructionSlots;	// Maximum number of vertex shader instruction slots supported. The maximum value that can be set on this cap is 32768. Devices that support vs_3_0 are required to support at least 512 instruction slots.
		caps.MaxPixelShader30InstructionSlots = maximumPixelShader30InstructionSlots;	// Maximum number of pixel shader instruction slots supported. The maximum value that can be set on this cap is 32768. Devices that support ps_3_0 are required to support at least 512 instruction slots.

		*capabilities = caps;

		return D3D_OK;
	}

	long Direct3D9::RegisterSoftwareDevice(void *initializeFunction)
	{
		TRACE("void *initializeFunction = 0x%0.8p", initializeFunction);

		loadSystemD3D9();

		if(d3d9)
		{
			return d3d9->RegisterSoftwareDevice(initializeFunction);
		}
		else
		{
			return INVALIDCALL();
		}
	}

	void Direct3D9::loadSystemD3D9()
	{
		if(d3d9)
		{
			return;
		}

		char d3d9Path[MAX_PATH + 16];
		GetSystemDirectory(d3d9Path, MAX_PATH);
		strcat(d3d9Path, "\\d3d9.dll");
		d3d9Lib = LoadLibrary(d3d9Path);

		if(d3d9Lib)
		{
			typedef IDirect3D9* (__stdcall *DIRECT3DCREATE9)(unsigned int);
			DIRECT3DCREATE9 direct3DCreate9 = (DIRECT3DCREATE9)GetProcAddress(d3d9Lib, "Direct3DCreate9");
			d3d9 = direct3DCreate9(D3D_SDK_VERSION);
		}
	}
}
