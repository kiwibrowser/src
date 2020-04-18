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

#include "Direct3D8.hpp"

#include "Direct3DDevice8.hpp"
#include "Capabilities.hpp"
#include "Configurator.hpp"
#include "Debug.hpp"
#include "CPUID.hpp"
#include "Version.h"
#include "Config.hpp"

#define COMPILE_MULTIMON_STUBS
#include <multimon.h>
#include <assert.h>

namespace D3D8
{
	Direct3D8::Direct3D8(int version, const HINSTANCE instance) : version(version), instance(instance)
	{
		displayMode = 0;
		numDisplayModes = 0;

		DEVMODE devmode;
		devmode.dmSize = sizeof(DEVMODE);

		// Count number of display modes
		while(EnumDisplaySettings(0, numDisplayModes, &devmode))
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

		d3d8Lib = 0;
		d3d8 = 0;

		sw::Configurator ini("SwiftShader.ini");

		int ps = ini.getInteger("Capabilities", "PixelShaderVersion", 14);
		int vs = ini.getInteger("Capabilities", "VertexShaderVersion", 11);

		     if(ps ==  0) pixelShaderVersion = D3DPS_VERSION(0, 0);
		else if(ps <= 11) pixelShaderVersion = D3DPS_VERSION(1, 1);
		else if(ps <= 12) pixelShaderVersion = D3DPS_VERSION(1, 2);
		else if(ps <= 13) pixelShaderVersion = D3DPS_VERSION(1, 3);
		else              pixelShaderVersion = D3DPS_VERSION(1, 4);

		     if(vs ==  0) vertexShaderVersion = D3DVS_VERSION(0, 0);
		else              vertexShaderVersion = D3DVS_VERSION(1, 1);

		textureMemory = 1024 * 1024 * ini.getInteger("Capabilities", "TextureMemory", 256);
	}

	Direct3D8::~Direct3D8()
	{
		delete[] displayMode;
		displayMode = 0;

		if(d3d8)
		{
			d3d8->Release();
			d3d8 = 0;
		}

		if(d3d8Lib)
		{
			FreeLibrary(d3d8Lib);
			d3d8Lib = 0;
		}
	}

	long Direct3D8::QueryInterface(const IID &iid, void **object)
	{
		TRACE("");

		if(iid == IID_IDirect3D8 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Direct3D8::AddRef()
	{
		TRACE("");

		return Unknown::AddRef();
	}

	unsigned long Direct3D8::Release()
	{
		TRACE("");

		return Unknown::Release();
	}

	long Direct3D8::CheckDepthStencilMatch(unsigned int adapter, D3DDEVTYPE deviceType, D3DFORMAT adapterFormat, D3DFORMAT renderTargetFormat, D3DFORMAT depthStencilFormat)
	{
		TRACE("");

		if(deviceType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D8();

			if(d3d8)
			{
				return d3d8->CheckDepthStencilMatch(adapter, deviceType, adapterFormat, renderTargetFormat, depthStencilFormat);
			}
			else
			{
				return CheckDepthStencilMatch(adapter, D3DDEVTYPE_HAL, adapterFormat, renderTargetFormat, depthStencilFormat);
			}
		}

		return D3D_OK;   // Any format combination is ok
	}

	long Direct3D8::CheckDeviceFormat(unsigned int adapter, D3DDEVTYPE deviceType, D3DFORMAT adapterFormat, unsigned long usage, D3DRESOURCETYPE resourceType, D3DFORMAT checkFormat)
	{
		TRACE("unsigned int adapter = %d, D3DDEVTYPE deviceType = %d, D3DFORMAT adapterFormat = %d, unsigned long usage = %d, D3DRESOURCETYPE resourceType = %d, D3DFORMAT checkFormat = %d", adapter, deviceType, adapterFormat, usage, resourceType, checkFormat);

		if(deviceType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D8();

			if(d3d8)
			{
				return d3d8->CheckDeviceFormat(adapter, deviceType, adapterFormat, usage, resourceType, checkFormat);
			}
			else
			{
				return CheckDeviceFormat(adapter, D3DDEVTYPE_HAL, adapterFormat, usage, resourceType, checkFormat);
			}
		}

		switch(resourceType)
		{
		case D3DRTYPE_SURFACE:
			if(usage & D3DUSAGE_RENDERTARGET)
			{
				switch(checkFormat)
				{
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
				// Integer HDR formats
				case D3DFMT_G16R16:			if(!Capabilities::Surface::RenderTarget::G16R16)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2B10G10R10:	if(!Capabilities::Surface::RenderTarget::A2B10G10R10)	return NOTAVAILABLE();	else return D3D_OK;
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
				// Paletted formats
				case D3DFMT_P8:				if(!Capabilities::Surface::P8)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8P8:			if(!Capabilities::Surface::A8P8)						return NOTAVAILABLE();	else return D3D_OK;
				// Integer HDR formats
				case D3DFMT_G16R16:			if(!Capabilities::Surface::G16R16)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2B10G10R10:	if(!Capabilities::Surface::A2B10G10R10)					return NOTAVAILABLE();	else return D3D_OK;
				// Compressed formats
				case D3DFMT_DXT1:			if(!Capabilities::Surface::DXT1)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT2:			if(!Capabilities::Surface::DXT2)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT3:			if(!Capabilities::Surface::DXT3)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT4:			if(!Capabilities::Surface::DXT4)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT5:			if(!Capabilities::Surface::DXT5)						return NOTAVAILABLE();	else return D3D_OK;
				// Bump map formats
				case D3DFMT_V8U8:			if(!Capabilities::Surface::V8U8)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_L6V5U5:			if(!Capabilities::Surface::L6V5U5)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8L8V8U8:		if(!Capabilities::Surface::X8L8V8U8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_Q8W8V8U8:		if(!Capabilities::Surface::Q8W8V8U8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_V16U16:			if(!Capabilities::Surface::V16U16)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2W10V10U10:	if(!Capabilities::Surface::A2W10V10U10)					return NOTAVAILABLE();	else return D3D_OK;
				// Luminance formats
				case D3DFMT_L8:				if(!Capabilities::Surface::L8)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A4L4:			if(!Capabilities::Surface::A4L4)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8L8:			if(!Capabilities::Surface::A8L8)						return NOTAVAILABLE();	else return D3D_OK;
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
			// Paletted formats
			case D3DFMT_P8:					if(!Capabilities::Volume::P8)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A8P8:				if(!Capabilities::Volume::A8P8)							return NOTAVAILABLE();	else return D3D_OK;
			// Integer HDR formats
			case D3DFMT_G16R16:				if(!Capabilities::Volume::G16R16)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A2B10G10R10:		if(!Capabilities::Volume::A2B10G10R10)					return NOTAVAILABLE();	else return D3D_OK;
			// Compressed formats
			case D3DFMT_DXT1:				if(!Capabilities::Volume::DXT1)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_DXT2:				if(!Capabilities::Volume::DXT2)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_DXT3:				if(!Capabilities::Volume::DXT3)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_DXT4:				if(!Capabilities::Volume::DXT4)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_DXT5:				if(!Capabilities::Volume::DXT5)							return NOTAVAILABLE();	else return D3D_OK;
			// Bump map formats
			case D3DFMT_V8U8:				if(!Capabilities::Volume::V8U8)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_L6V5U5:				if(!Capabilities::Volume::L6V5U5)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_X8L8V8U8:			if(!Capabilities::Volume::X8L8V8U8)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_Q8W8V8U8:			if(!Capabilities::Volume::Q8W8V8U8)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_V16U16:				if(!Capabilities::Volume::V16U16)						return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A2W10V10U10:		if(!Capabilities::Volume::A2W10V10U10)					return NOTAVAILABLE();	else return D3D_OK;
			// Luminance formats
			case D3DFMT_L8:					if(!Capabilities::Volume::L8)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A4L4:				if(!Capabilities::Volume::A4L4)							return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A8L8:				if(!Capabilities::Volume::A8L8)							return NOTAVAILABLE();	else return D3D_OK;
			default:
				return NOTAVAILABLE();
			}
		case D3DRTYPE_CUBETEXTURE:
			if(usage & D3DUSAGE_RENDERTARGET)
			{
				switch(checkFormat)
				{
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
				// Integer HDR formats
				case D3DFMT_G16R16:			if(!Capabilities::CubeMap::RenderTarget::G16R16)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2B10G10R10:	if(!Capabilities::CubeMap::RenderTarget::A2B10G10R10)	return NOTAVAILABLE();	else return D3D_OK;
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
				// Paletted formats
				case D3DFMT_P8:				if(!Capabilities::CubeMap::P8)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8P8:			if(!Capabilities::CubeMap::A8P8)						return NOTAVAILABLE();	else return D3D_OK;
				// Integer HDR formats
				case D3DFMT_G16R16:			if(!Capabilities::CubeMap::G16R16)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2B10G10R10:	if(!Capabilities::CubeMap::A2B10G10R10)					return NOTAVAILABLE();	else return D3D_OK;
				// Compressed formats
				case D3DFMT_DXT1:			if(!Capabilities::CubeMap::DXT1)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT2:			if(!Capabilities::CubeMap::DXT2)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT3:			if(!Capabilities::CubeMap::DXT3)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT4:			if(!Capabilities::CubeMap::DXT4)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT5:			if(!Capabilities::CubeMap::DXT5)						return NOTAVAILABLE();	else return D3D_OK;
				// Bump map formats
				case D3DFMT_V8U8:			if(!Capabilities::CubeMap::V8U8)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_L6V5U5:			if(!Capabilities::CubeMap::L6V5U5)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8L8V8U8:		if(!Capabilities::CubeMap::X8L8V8U8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_Q8W8V8U8:		if(!Capabilities::CubeMap::Q8W8V8U8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_V16U16:			if(!Capabilities::CubeMap::V16U16)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2W10V10U10:	if(!Capabilities::CubeMap::A2W10V10U10)					return NOTAVAILABLE();	else return D3D_OK;
				// Luminance formats
				case D3DFMT_L8:				if(!Capabilities::CubeMap::L8)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A4L4:			if(!Capabilities::CubeMap::A4L4)						return NOTAVAILABLE();	else return D3D_OK;
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
			// Paletted formats
			case D3DFMT_P8:					if(!Capabilities::VolumeTexture::P8)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A8P8:				if(!Capabilities::VolumeTexture::A8P8)					return NOTAVAILABLE();	else return D3D_OK;
			// Integer HDR formats
			case D3DFMT_G16R16:				if(!Capabilities::VolumeTexture::G16R16)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A2B10G10R10:		if(!Capabilities::VolumeTexture::A2B10G10R10)			return NOTAVAILABLE();	else return D3D_OK;
			// Compressed formats
			case D3DFMT_DXT1:				if(!Capabilities::VolumeTexture::DXT1)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_DXT2:				if(!Capabilities::VolumeTexture::DXT2)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_DXT3:				if(!Capabilities::VolumeTexture::DXT3)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_DXT4:				if(!Capabilities::VolumeTexture::DXT4)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_DXT5:				if(!Capabilities::VolumeTexture::DXT5)					return NOTAVAILABLE();	else return D3D_OK;
			// Bump map formats
			case D3DFMT_V8U8:				if(!Capabilities::VolumeTexture::V8U8)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_L6V5U5:				if(!Capabilities::VolumeTexture::L6V5U5)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_X8L8V8U8:			if(!Capabilities::VolumeTexture::X8L8V8U8)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_Q8W8V8U8:			if(!Capabilities::VolumeTexture::Q8W8V8U8)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_V16U16:				if(!Capabilities::VolumeTexture::V16U16)				return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A2W10V10U10:		if(!Capabilities::VolumeTexture::A2W10V10U10)			return NOTAVAILABLE();	else return D3D_OK;
			// Luminance formats
			case D3DFMT_L8:					if(!Capabilities::VolumeTexture::L8)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A4L4:				if(!Capabilities::VolumeTexture::A4L4)					return NOTAVAILABLE();	else return D3D_OK;
			case D3DFMT_A8L8:				if(!Capabilities::VolumeTexture::A8L8)					return NOTAVAILABLE();	else return D3D_OK;
			default:
				return NOTAVAILABLE();
			}
		case D3DRTYPE_TEXTURE:
			if(usage & D3DUSAGE_RENDERTARGET)
			{
				switch(checkFormat)
				{
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
				// Integer HDR formats
				case D3DFMT_G16R16:			if(!Capabilities::Texture::RenderTarget::G16R16)		return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2B10G10R10:	if(!Capabilities::Texture::RenderTarget::A2B10G10R10)	return NOTAVAILABLE();	else return D3D_OK;
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
				default:
					return NOTAVAILABLE();
				}
			}
			else
			{
				switch(checkFormat)
				{
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
				// Paletted formats
				case D3DFMT_P8:				if(!Capabilities::Texture::P8)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8P8:			if(!Capabilities::Texture::A8P8)						return NOTAVAILABLE();	else return D3D_OK;
				// Integer HDR formats
				case D3DFMT_G16R16:			if(!Capabilities::Texture::G16R16)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2B10G10R10:	if(!Capabilities::Texture::A2B10G10R10)					return NOTAVAILABLE();	else return D3D_OK;
				// Compressed formats
				case D3DFMT_DXT1:			if(!Capabilities::Texture::DXT1)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT2:			if(!Capabilities::Texture::DXT2)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT3:			if(!Capabilities::Texture::DXT3)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT4:			if(!Capabilities::Texture::DXT4)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_DXT5:			if(!Capabilities::Texture::DXT5)						return NOTAVAILABLE();	else return D3D_OK;
				// Bump map formats
				case D3DFMT_V8U8:			if(!Capabilities::Texture::V8U8)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_L6V5U5:			if(!Capabilities::Texture::L6V5U5)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_X8L8V8U8:		if(!Capabilities::Texture::X8L8V8U8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_Q8W8V8U8:		if(!Capabilities::Texture::Q8W8V8U8)					return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_V16U16:			if(!Capabilities::Texture::V16U16)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A2W10V10U10:	if(!Capabilities::Texture::A2W10V10U10)					return NOTAVAILABLE();	else return D3D_OK;
				// Luminance formats
				case D3DFMT_L8:				if(!Capabilities::Texture::L8)							return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A4L4:			if(!Capabilities::Texture::A4L4)						return NOTAVAILABLE();	else return D3D_OK;
				case D3DFMT_A8L8:			if(!Capabilities::Texture::A8L8)						return NOTAVAILABLE();	else return D3D_OK;
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

	long Direct3D8::CheckDeviceMultiSampleType(unsigned int adapter, D3DDEVTYPE deviceType, D3DFORMAT surfaceFormat, int windowed, D3DMULTISAMPLE_TYPE multiSampleType)
	{
		TRACE("");

		if(deviceType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D8();

			if(d3d8)
			{
				return d3d8->CheckDeviceMultiSampleType(adapter, deviceType, surfaceFormat, windowed, multiSampleType);
			}
			else
			{
				return CheckDeviceMultiSampleType(adapter, D3DDEVTYPE_HAL, surfaceFormat, windowed, multiSampleType);
			}
		}

		if(adapter >= GetAdapterCount())
		{
			return INVALIDCALL();
		}

		if(multiSampleType == D3DMULTISAMPLE_NONE ||
		   multiSampleType == D3DMULTISAMPLE_2_SAMPLES ||
		   multiSampleType == D3DMULTISAMPLE_4_SAMPLES ||
		   multiSampleType == D3DMULTISAMPLE_8_SAMPLES ||
		   multiSampleType == D3DMULTISAMPLE_16_SAMPLES)
		{
			if(surfaceFormat != D3DFMT_UNKNOWN)
			{
				return D3D_OK;	
			}
		}

		return NOTAVAILABLE();
	}

	long Direct3D8::CheckDeviceType(unsigned int adapter, D3DDEVTYPE checkType, D3DFORMAT displayFormat, D3DFORMAT backBufferFormat, int windowed)
	{
		TRACE("");

		if(checkType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D8();

			if(d3d8)
			{
				return d3d8->CheckDeviceType(adapter, checkType, displayFormat, backBufferFormat, windowed);
			}
			else
			{
				return CheckDeviceType(adapter, D3DDEVTYPE_HAL, displayFormat, backBufferFormat, windowed);
			}
		}

		return D3D_OK;   // TODO
	}

	long Direct3D8::CreateDevice(unsigned int adapter, D3DDEVTYPE deviceType, HWND focusWindow, unsigned long behaviorFlags, D3DPRESENT_PARAMETERS *presentParameters, IDirect3DDevice8 **returnedDeviceInterface)
	{
		TRACE("");

		if(deviceType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D8();

			if(d3d8)
			{
				return d3d8->CreateDevice(adapter, deviceType, focusWindow, behaviorFlags, presentParameters, returnedDeviceInterface);
			}
			else
			{
				return CreateDevice(adapter, D3DDEVTYPE_HAL, focusWindow, behaviorFlags, presentParameters, returnedDeviceInterface);
			}
		}

		if(!focusWindow || !presentParameters || !returnedDeviceInterface)
		{
			*returnedDeviceInterface = NULL;
			return INVALIDCALL();
		}

		if(!sw::CPUID::supportsSSE())
		{
			return NOTAVAILABLE();
		}

		*returnedDeviceInterface = new Direct3DDevice8(instance, this, adapter, deviceType, focusWindow, behaviorFlags, presentParameters);

		if(*returnedDeviceInterface)
		{
			(*returnedDeviceInterface)->AddRef();
		}

		return D3D_OK;
	}

	long Direct3D8::EnumAdapterModes(unsigned int adapter, unsigned int index, D3DDISPLAYMODE *mode)
	{
		TRACE("");

		if(adapter != D3DADAPTER_DEFAULT || !mode)
		{
			return INVALIDCALL();
		}

		for(int i = 0; i < numDisplayModes; i++)
		{
			if(index-- == 0)
			{
				mode->Width = displayMode[i].dmPelsWidth;
				mode->Height = displayMode[i].dmPelsHeight;
				mode->RefreshRate = displayMode[i].dmDisplayFrequency;

				displayMode[i].dmBitsPerPel = 32;   // FIXME

				switch(displayMode[i].dmBitsPerPel)
				{
				case 4:
					mode->Format = D3DFMT_A4L4;
					break;
				case 8:
					mode->Format = D3DFMT_P8;
					break;
				case 16:
					mode->Format = D3DFMT_R5G6B5;
					break;
				case 24:
					mode->Format = D3DFMT_R8G8B8;
					break;
				case 32:
					mode->Format = D3DFMT_X8R8G8B8;
					break;
				default:
					ASSERT(false);
				}

				return D3D_OK;
			}
		}

		return INVALIDCALL();
	}

	unsigned int Direct3D8::GetAdapterCount()
	{
		TRACE("");

		return 1;   // SwiftShader does not support multiple display adapters
	}

	long Direct3D8::GetAdapterDisplayMode(unsigned int adapter, D3DDISPLAYMODE *mode)
	{
		TRACE("");

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
		case 24: mode->Format = D3DFMT_R8G8B8; break;
		case 16: mode->Format = D3DFMT_R5G6B5; break;
		default:
			ASSERT(false);   // Unexpected display mode color depth
		}

		return D3D_OK;
	}

	long Direct3D8::GetAdapterIdentifier(unsigned int adapter, unsigned long flags, D3DADAPTER_IDENTIFIER8 *identifier)
	{
		TRACE("");

		if(!identifier)
		{
			return INVALIDCALL();
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
		identifier->VendorId = 0;
		identifier->DeviceId = 0;
		identifier->SubSysId = 0;
		identifier->Revision = 0;
		identifier->DeviceIdentifier = guid;
		identifier->WHQLLevel = 0;

		return D3D_OK;
	}

	unsigned int Direct3D8::GetAdapterModeCount(unsigned int adapter)
	{
		TRACE("");

		if(adapter != D3DADAPTER_DEFAULT)
		{
			return 0;
		}

		return numDisplayModes;
	}

	HMONITOR Direct3D8::GetAdapterMonitor(unsigned int adapter)
	{
		TRACE("");

		POINT point = {0, 0};

		return MonitorFromPoint(point, MONITOR_DEFAULTTOPRIMARY);   // FIXME: Ignores adapter parameter
	}

	long Direct3D8::GetDeviceCaps(unsigned int adapter, D3DDEVTYPE deviceType, D3DCAPS8 *capabilities)
	{
		TRACE("");

		if(deviceType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D8();

			if(d3d8)
			{
				return d3d8->GetDeviceCaps(adapter, deviceType, capabilities);
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

		D3DCAPS8 caps;
		ZeroMemory(&caps, sizeof(D3DCAPS8));

		// Device info
		caps.DeviceType = D3DDEVTYPE_HAL;
		caps.AdapterOrdinal = D3DADAPTER_DEFAULT;

		// Caps from DX7
		caps.Caps =	0;	//	D3DCAPS_READ_SCANLINE

		caps.Caps2 =	//	D3DCAPS2_CANCALIBRATEGAMMA |	// The system has a calibrator installed that can automatically adjust the gamma ramp so that the result is identical on all systems that have a calibrator. To invoke the calibrator when setting new gamma levels, use the D3DSGR_CALIBRATE flag when calling IDirect3DDevice8::SetGammaRamp. Calibrating gamma ramps incurs some processing overhead and should not be used frequently.  
							D3DCAPS2_CANRENDERWINDOWED |	// The driver is capable of rendering in windowed mode. 
							D3DCAPS2_CANMANAGERESOURCE |	// The driver is capable of managing resources. On such drivers, D3DPOOL_MANAGED resources will be managed by the driver. To have Microsoft® Direct3D® override the driver so that Direct3D manages resources, use the D3DCREATE_DISABLE_DRIVER_MANAGEMENT flag when calling IDirect3D8::CreateDevice. 
							D3DCAPS2_DYNAMICTEXTURES |		// The driver supports dynamic textures. 
							D3DCAPS2_FULLSCREENGAMMA;		// The driver supports dynamic gamma ramp adjustment in full-screen mode.
						//	D3DCAPS2_NO2DDURING3DSCENE;		// When the D3DCAPS2_NO2DDURING3DSCENE capability is set by the driver, it means that 2-D operations cannot be performed between calls to IDirect3DDevice8::BeginScene and IDirect3DDevice8::EndScene. 

		caps.Caps3 = D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD;   //The device will work as expected with the D3DRS_ALPHABLENDENABLE render state when a full-screen application uses D3DSWAPEFFECT_FLIP or D3DRS_SWAPEFFECT_DISCARD. D3DRS_ALPHABLENDENABLE works as expected when using D3DSWAPEFFECT_COPY and D3DSWAPEFFECT_COPYSYNC

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
							D3DDEVCAPS_DRAWPRIMTLVERTEX |			// Device exports an IDirect3DDevice8::DrawPrimitive-aware hardware abstraction layer (HAL). 
						//	D3DDEVCAPS_EXECUTESYSTEMMEMORY |		// Device can use execute buffers from system memory.
						//	D3DDEVCAPS_EXECUTEVIDEOMEMORY |			// Device can use execute buffers from video memory. 
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

		caps.PrimitiveMiscCaps = 		D3DPMISCCAPS_BLENDOP | 					// Device supports the alpha-blending operations defined in the D3DBLENDOP enumerated type. 
										D3DPMISCCAPS_CLIPPLANESCALEDPOINTS |	// Device correctly clips scaled points of size greater than 1.0 to user-defined clipping planes. 
										D3DPMISCCAPS_CLIPTLVERTS |				// Device clips post-transformed vertex primitives. 
										D3DPMISCCAPS_COLORWRITEENABLE |			// Device supports per-channel writes for the render target color buffer through the D3DRS_COLORWRITEENABLE state. 
										D3DPMISCCAPS_CULLCCW |					// The driver supports counterclockwise culling through the D3DRS_CULLMODE state. (This applies only to triangle primitives.) This flag corresponds to the D3DCULL_CCW member of the D3DCULL enumerated type. 
										D3DPMISCCAPS_CULLCW |					// The driver supports clockwise triangle culling through the D3DRS_CULLMODE state. (This applies only to triangle primitives.) This flag corresponds to the D3DCULL_CW member of the D3DCULL enumerated type. 
										D3DPMISCCAPS_CULLNONE |					// The driver does not perform triangle culling. This corresponds to the D3DCULL_NONE member of the D3DCULL enumerated type. 
									//	D3DPMISCCAPS_LINEPATTERNREP	|			// The driver can handle values other than 1 in the wRepeatFactor member of the D3DLINEPATTERN structure. (This applies only to line-drawing primitives.) 
										D3DPMISCCAPS_MASKZ |					// Device can enable and disable modification of the depth buffer on pixel operations. 
										D3DPMISCCAPS_TSSARGTEMP;				// Device supports D3DTA_TEMP for temporary register.

		caps.RasterCaps =		D3DPRASTERCAPS_ANISOTROPY |				// Device supports anisotropic filtering. 
							//	D3DPRASTERCAPS_ANTIALIASEDGES |			// Device can antialias lines forming the convex outline of objects. For more information, see D3DRS_EDGEANTIALIAS.
								D3DPRASTERCAPS_COLORPERSPECTIVE |		// Device iterates colors perspective correctly. 
							//	D3DPRASTERCAPS_DITHER |					// Device can dither to improve color resolution. 
								D3DPRASTERCAPS_ZBIAS |					// Device supports legacy depth bias. For true depth bias, see D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS.
								D3DPRASTERCAPS_FOGRANGE |				// Device supports range-based fog. In range-based fog, the distance of an object from the viewer is used to compute fog effects, not the depth of the object (that is, the z-coordinate) in the scene. 
								D3DPRASTERCAPS_FOGTABLE |				// Device calculates the fog value by referring to a lookup table containing fog values that are indexed to the depth of a given pixel. 
								D3DPRASTERCAPS_FOGVERTEX |				// Device calculates the fog value during the lighting operation and interpolates the fog value during rasterization. 
								D3DPRASTERCAPS_MIPMAPLODBIAS |			// Device supports level of detail (LOD) bias adjustments. These bias adjustments enable an application to make a mipmap appear crisper or less sharp than it normally would. For more information about LOD bias in mipmaps, see D3DSAMP_MIPMAPLODBIAS.
							//	D3DPRASTERCAPS_PAT |					// The driver can perform patterned drawing lines or fills with D3DRS_LINEPATTERN for the primitive being queried. 
							//	D3DPRASTERCAPS_STRETCHBLTMULTISAMPLE |	// Device provides limited multisample support through a stretch-blt implementation. When this capability is set, D3DRS_MULTISAMPLEANTIALIAS cannot be turned on and off in the middle of a scene. Multisample masking cannot be performed if this flag is set. 
							//	D3DPRASTERCAPS_WBUFFER |				// Device supports depth buffering using w. 
								D3DPRASTERCAPS_WFOG |					// Device supports w-based fog. W-based fog is used when a perspective projection matrix is specified, but affine projections still use z-based fog. The system considers a projection matrix that contains a nonzero value in the [3][4] element to be a perspective projection matrix. 
								D3DPRASTERCAPS_ZBIAS |					// Device supports z-bias values. These are integer values assigned to polygons that allow physically coplanar polygons to appear separate. For more information, see D3DRS_ZBIAS. 
							//	D3DPRASTERCAPS_ZBUFFERLESSHSR |			// Device can perform hidden-surface removal (HSR) without requiring the application to sort polygons and without requiring the allocation of a depth-buffer. This leaves more video memory for textures. The method used to perform HSR is hardware-dependent and is transparent to the application. Z-bufferless HSR is performed if no depth-buffer surface is associated with the rendering-target surface and the depth-buffer comparison test is enabled (that is, when the state value associated with the D3DRS_ZENABLE enumeration constant is set to TRUE).
								D3DPRASTERCAPS_ZFOG;					// Device supports z-based fog. 
							//	D3DPRASTERCAPS_ZTEST;					// Device can perform z-test operations. This effectively renders a primitive and indicates whether any z pixels have been rendered.

		caps.ZCmpCaps =		D3DPCMPCAPS_ALWAYS |		// Always pass the z-test. 
							D3DPCMPCAPS_EQUAL |			// Pass the z-test if the new z equals the current z. 
							D3DPCMPCAPS_GREATER |		// Pass the z-test if the new z is greater than the current z. 
							D3DPCMPCAPS_GREATEREQUAL |	// Pass the z-test if the new z is greater than or equal to the current z. 
							D3DPCMPCAPS_LESS |			// Pass the z-test if the new z is less than the current z. 
							D3DPCMPCAPS_LESSEQUAL |		// Pass the z-test if the new z is less than or equal to the current z. 
							D3DPCMPCAPS_NEVER |			// Always fail the z-test. 
							D3DPCMPCAPS_NOTEQUAL;		// Pass the z-test if the new z does not equal the current z.

		caps.SrcBlendCaps =		D3DPBLENDCAPS_BOTHINVSRCALPHA |	// Source blend factor is (1-As,1-As,1-As,1-As) and destination blend factor is (As,As,As,As); the destination blend selection is overridden. 
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

		caps.DestBlendCaps = 	D3DPBLENDCAPS_BOTHINVSRCALPHA |	// Source blend factor is (1-As,1-As,1-As,1-As) and destination blend factor is (As,As,As,As); the destination blend selection is overridden. 
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
								D3DPTEXTURECAPS_PERSPECTIVE |				// Perspective correction texturing is supported.
							//	D3DPTEXTURECAPS_POW2 |						// All textures must have widths and heights specified as powers of two. This requirement does not apply to either cube textures or volume textures.
								D3DPTEXTURECAPS_PROJECTED |					// Supports the D3DTTFF_PROJECTED texture transformation flag. When applied, the device divides transformed texture coordinates by the last texture coordinate. If this capability is present, then the projective divide occurs per pixel. If this capability is not present, but the projective divide needs to occur anyway, then it is performed on a per-vertex basis by the Direct3D runtime. 
							//	D3DPTEXTURECAPS_SQUAREONLY |				// All textures must be square. 
								D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE |	// Texture indices are not scaled by the texture size prior to interpolation.
								D3DPTEXTURECAPS_VOLUMEMAP;					// Device supports volume textures.
							//	D3DPTEXTURECAPS_VOLUMEMAP_POW2;				// Device requires that volume texture maps have dimensions specified as powers of two. 

		caps.TextureFilterCaps =	//	D3DPTFILTERCAPS_MAGFAFLATCUBIC |	// Device supports per-stage flat cubic filtering for magnifying textures. The flat cubic magnification filter is represented by the D3DTEXF_FLATCUBIC member of the D3DTEXTUREFILTERTYPE enumerated type. 
									//	D3DPTFILTERCAPS_MAGFANISOTROPIC |	// Device supports per-stage anisotropic filtering for magnifying textures. The anisotropic magnification filter is represented by the D3DTEXF_ANISOTROPIC member of the D3DTEXTUREFILTERTYPE enumerated type. 
									//	D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC |	// Device supports the per-stage Gaussian cubic filtering for magnifying textures. The Gaussian cubic magnification filter is represented by the D3DTEXF_GAUSSIANCUBIC member of the D3DTEXTUREFILTERTYPE enumerated type.
										D3DPTFILTERCAPS_MAGFLINEAR |		// Device supports per-stage bilinear interpolation filtering for magnifying mipmaps. The bilinear interpolation mipmapping filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type. 
										D3DPTFILTERCAPS_MAGFPOINT |			// Device supports per-stage point-sample filtering for magnifying textures. The point-sample magnification filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type. 
										D3DPTFILTERCAPS_MINFANISOTROPIC |	// Device supports per-stage anisotropic filtering for minifying textures. The anisotropic minification filter is represented by the D3DTEXF_ANISOTROPIC member of the D3DTEXTUREFILTERTYPE enumerated type. 
										D3DPTFILTERCAPS_MINFLINEAR |		// Device supports per-stage linear filtering for minifying textures. The linear minification filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type. 
										D3DPTFILTERCAPS_MINFPOINT |			// Device supports per-stage point-sample filtering for minifying textures. The point-sample minification filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type. 
										D3DPTFILTERCAPS_MIPFLINEAR |		// Device supports per-stage bilinear interpolation filtering for mipmaps. The bilinear interpolation mipmapping filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type. 
										D3DPTFILTERCAPS_MIPFPOINT;			// Device supports per-stage point-sample filtering for mipmaps. The point-sample mipmapping filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type. 
									
		caps.CubeTextureFilterCaps =	//	D3DPTFILTERCAPS_MAGFAFLATCUBIC |	// Device supports per-stage flat cubic filtering for magnifying textures. The flat cubic magnification filter is represented by the D3DTEXF_FLATCUBIC member of the D3DTEXTUREFILTERTYPE enumerated type. 
										//	D3DPTFILTERCAPS_MAGFANISOTROPIC |	// Device supports per-stage anisotropic filtering for magnifying textures. The anisotropic magnification filter is represented by the D3DTEXF_ANISOTROPIC member of the D3DTEXTUREFILTERTYPE enumerated type. 
										//	D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC |	// Device supports the per-stage Gaussian cubic filtering for magnifying textures. The Gaussian cubic magnification filter is represented by the D3DTEXF_GAUSSIANCUBIC member of the D3DTEXTUREFILTERTYPE enumerated type.
											D3DPTFILTERCAPS_MAGFLINEAR |		// Device supports per-stage bilinear interpolation filtering for magnifying mipmaps. The bilinear interpolation mipmapping filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type. 
											D3DPTFILTERCAPS_MAGFPOINT |			// Device supports per-stage point-sample filtering for magnifying textures. The point-sample magnification filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type. 
										//	D3DPTFILTERCAPS_MINFANISOTROPIC |	// Device supports per-stage anisotropic filtering for minifying textures. The anisotropic minification filter is represented by the D3DTEXF_ANISOTROPIC member of the D3DTEXTUREFILTERTYPE enumerated type. 
											D3DPTFILTERCAPS_MINFLINEAR |		// Device supports per-stage linear filtering for minifying textures. The linear minification filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type. 
											D3DPTFILTERCAPS_MINFPOINT |			// Device supports per-stage point-sample filtering for minifying textures. The point-sample minification filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type. 
											D3DPTFILTERCAPS_MIPFLINEAR |		// Device supports per-stage bilinear interpolation filtering for mipmaps. The bilinear interpolation mipmapping filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type. 
											D3DPTFILTERCAPS_MIPFPOINT;			// Device supports per-stage point-sample filtering for mipmaps. The point-sample mipmapping filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type. 
									
		caps.VolumeTextureFilterCaps = 	//	D3DPTFILTERCAPS_MAGFAFLATCUBIC |	// Device supports per-stage flat cubic filtering for magnifying textures. The flat cubic magnification filter is represented by the D3DTEXF_FLATCUBIC member of the D3DTEXTUREFILTERTYPE enumerated type. 
										//	D3DPTFILTERCAPS_MAGFANISOTROPIC |	// Device supports per-stage anisotropic filtering for magnifying textures. The anisotropic magnification filter is represented by the D3DTEXF_ANISOTROPIC member of the D3DTEXTUREFILTERTYPE enumerated type. 
										//	D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC |	// Device supports the per-stage Gaussian cubic filtering for magnifying textures. The Gaussian cubic magnification filter is represented by the D3DTEXF_GAUSSIANCUBIC member of the D3DTEXTUREFILTERTYPE enumerated type.
											D3DPTFILTERCAPS_MAGFLINEAR |		// Device supports per-stage bilinear interpolation filtering for magnifying mipmaps. The bilinear interpolation mipmapping filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type. 
											D3DPTFILTERCAPS_MAGFPOINT |			// Device supports per-stage point-sample filtering for magnifying textures. The point-sample magnification filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type. 
										//	D3DPTFILTERCAPS_MINFANISOTROPIC |	// Device supports per-stage anisotropic filtering for minifying textures. The anisotropic minification filter is represented by the D3DTEXF_ANISOTROPIC member of the D3DTEXTUREFILTERTYPE enumerated type. 
											D3DPTFILTERCAPS_MINFLINEAR |		// Device supports per-stage linear filtering for minifying textures. The linear minification filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type. 
											D3DPTFILTERCAPS_MINFPOINT |			// Device supports per-stage point-sample filtering for minifying textures. The point-sample minification filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type. 
											D3DPTFILTERCAPS_MIPFLINEAR |		// Device supports per-stage bilinear interpolation filtering for mipmaps. The bilinear interpolation mipmapping filter is represented by the D3DTEXF_LINEAR member of the D3DTEXTUREFILTERTYPE enumerated type. 
											D3DPTFILTERCAPS_MIPFPOINT;			// Device supports per-stage point-sample filtering for mipmaps. The point-sample mipmapping filter is represented by the D3DTEXF_POINT member of the D3DTEXTUREFILTERTYPE enumerated type. 
									
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

		caps.LineCaps = D3DLINECAPS_ALPHACMP |	// Supports alpha-test comparisons. 
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
								D3DSTENCILCAPS_DECR;		// Decrement the stencil-buffer entry, wrapping to the maximum value if the new value is less than zero. 
		
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
										D3DVTXPCAPS_POSITIONALLIGHTS |			// Device can do positional lights (includes point and spot). 
										D3DVTXPCAPS_TEXGEN |					// Device can do texgen. 
										D3DVTXPCAPS_TWEENING;					// Device can do vertex tweening. 
									//	D3DVTXPCAPS_NO_VSDT_UBYTE4;				// Device does not support the D3DVSDT_UBYTE4 vertex declaration type.  

		caps.MaxActiveLights = 8;						// Maximum number of lights that can be active simultaneously. For a given physical device, this capability might vary across Direct3DDevice objects depending on the parameters supplied to IDirect3D8::CreateDevice.
		caps.MaxUserClipPlanes = 6;						// Maximum number of user-defined clipping planes supported. This member can range from 0 through D3DMAXUSERCLIPPLANES. For a given physical device, this capability may vary across Direct3DDevice objects depending on the parameters supplied to IDirect3D8::CreateDevice. 
		caps.MaxVertexBlendMatrices = 4;				// Maximum number of matrices that this device can apply when performing multimatrix vertex blending. For a given physical device, this capability may vary across Direct3DDevice objects depending on the parameters supplied to IDirect3D8::CreateDevice. 
		caps.MaxVertexBlendMatrixIndex = 11;			// DWORD value that specifies the maximum matrix index that can be indexed into using the per-vertex indices. The number of matrices is MaxVertexBlendMatrixIndex + 1, which is the size of the matrix palette. If normals are present in the vertex data that needs to be blended for lighting, then the number of matrices is half the number specified by this capability flag. If MaxVertexBlendMatrixIndex is set to zero, the driver does not support indexed vertex blending. If this value is not zero then the valid range of indices is zero through MaxVertexBlendMatrixIndex.
		caps.MaxPointSize = 8192.0f;					// Maximum size of a point primitive. If set to 1.0f then device does not support point size control. The range is greater than or equal to 1.0f.
		caps.MaxPrimitiveCount = 1 << 21;				// Maximum number of primitives for each IDirect3DDevice8::DrawPrimitive call. Note that when Direct3D is working with a DirectX 6.0 or DirectX 7.0 driver, this field is set to 0xFFFF. This means that not only the number of primitives but also the number of vertices is limited by this value.
		caps.MaxVertexIndex = 1 << 24;					// Maximum size of indices supported for hardware vertex processing. It is possible to create 32-bit index buffers by specifying D3DFMT_INDEX32; however, you will not be able to render with the index buffer unless this value is greater than 0x0000FFFF.
		caps.MaxStreams = 16;							// Maximum number of concurrent data streams for IDirect3DDevice8::SetStreamSource. The valid range is 1 to 16. Note that if this value is 0, then the driver is not a DirectX 9.0 driver.
		caps.MaxStreamStride = 65536;					// Maximum stride for IDirect3DDevice8::SetStreamSource.
		caps.VertexShaderVersion = vertexShaderVersion;	// Two numbers that represent the vertex shader main and sub versions. For more information about the instructions supported for each vertex shader version, see Version 1_x, Version 2_0, Version 2_0 Extended, or Version 3_0.
		caps.MaxVertexShaderConst = 256;				// The number of vertex shader Registers that are reserved for constants.
		caps.PixelShaderVersion = pixelShaderVersion;	// Two numbers that represent the pixel shader main and sub versions. For more information about the instructions supported for each pixel shader version, see Version 1_x, Version 2_0, Version 2_0 Extended, or Version 3_0.
		caps.MaxPixelShaderValue = 8.0;
		
		*capabilities = caps;

		return D3D_OK;
	}

	long Direct3D8::RegisterSoftwareDevice(void *initializeFunction)
	{
		TRACE("");

		loadSystemD3D8();

		if(d3d8)
		{
			return d3d8->RegisterSoftwareDevice(initializeFunction);
		}
		else
		{
			return INVALIDCALL();
		}
	}

	void Direct3D8::loadSystemD3D8()
	{
		if(d3d8)
		{
			return;
		}

		char d3d8Path[MAX_PATH + 16];
		GetSystemDirectory(d3d8Path, MAX_PATH);
		strcat(d3d8Path, "\\d3d8.dll");
		d3d8Lib = LoadLibrary(d3d8Path);

		if(d3d8Lib)
		{
			typedef IDirect3D8* (__stdcall *DIRECT3DCREATE8)(unsigned int);
			DIRECT3DCREATE8 direct3DCreate8 = (DIRECT3DCREATE8)GetProcAddress(d3d8Lib, "Direct3DCreate8");
			d3d8 = direct3DCreate8(D3D_SDK_VERSION);
		}
	}
}
