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

#include "Direct3D9Ex.hpp"

#include "Direct3DDevice9Ex.hpp"
#include "Debug.hpp"

namespace sw
{
	extern bool postBlendSRGB;
}

namespace D3D9
{
	Direct3D9Ex::Direct3D9Ex(int version, const HINSTANCE instance) : Direct3D9(version, instance)
	{
		d3d9ex = 0;
	}

	Direct3D9Ex::~Direct3D9Ex()
	{
	}

	long Direct3D9Ex::QueryInterface(const IID &iid, void **object)
	{
		TRACE("const IID &iid = 0x%0.8p, void **object = 0x%0.8p", iid, object);

		if(iid == IID_IDirect3D9Ex ||
		   iid == IID_IDirect3D9 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Direct3D9Ex::AddRef()
	{
		TRACE("void");

		return Direct3D9::AddRef();
	}

	unsigned long Direct3D9Ex::Release()
	{
		TRACE("void");

		return Direct3D9::Release();
	}

	long Direct3D9Ex::RegisterSoftwareDevice(void *initializeFunction)
	{
		TRACE("void *initializeFunction = 0x%0.8p", initializeFunction);

		loadSystemD3D9ex();

		if(d3d9ex)
		{
			return d3d9ex->RegisterSoftwareDevice(initializeFunction);
		}
		else
		{
			return INVALIDCALL();
		}
	}

	unsigned int Direct3D9Ex::GetAdapterCount()
	{
		TRACE("void");

		return Direct3D9::GetAdapterCount();
	}

	long Direct3D9Ex::GetAdapterIdentifier(unsigned int adapter, unsigned long flags, D3DADAPTER_IDENTIFIER9 *identifier)
	{
		TRACE("unsigned int adapter = %d, unsigned long flags = 0x%0.8X, D3DADAPTER_IDENTIFIER9 *identifier = 0x%0.8p", adapter, flags, identifier);

		return Direct3D9::GetAdapterIdentifier(adapter, flags, identifier);
	}

	unsigned int Direct3D9Ex::GetAdapterModeCount(unsigned int adapter, D3DFORMAT format)
	{
		TRACE("unsigned int adapter = %d, D3DFORMAT format = %d", adapter, format);

		return Direct3D9::GetAdapterModeCount(adapter, format);
	}

	long Direct3D9Ex::EnumAdapterModes(unsigned int adapter, D3DFORMAT format, unsigned int index, D3DDISPLAYMODE *mode)
	{
		TRACE("unsigned int adapter = %d, D3DFORMAT format = %d, unsigned int index = %d, D3DDISPLAYMODE *mode = 0x%0.8p", adapter, format, index, mode);

		return Direct3D9::EnumAdapterModes(adapter, format, index, mode);
	}

	long Direct3D9Ex::GetAdapterDisplayMode(unsigned int adapter, D3DDISPLAYMODE *mode)
	{
		TRACE("unsigned int adapter = %d, D3DDISPLAYMODE *mode = 0x%0.8p", adapter, mode);

		return Direct3D9::GetAdapterDisplayMode(adapter, mode);
	}

	long Direct3D9Ex::CheckDeviceType(unsigned int adapter, D3DDEVTYPE checkType, D3DFORMAT displayFormat, D3DFORMAT backBufferFormat, int windowed)
	{
		TRACE("unsigned int adapter = %d, D3DDEVTYPE checkType = %d, D3DFORMAT displayFormat = %d, D3DFORMAT backBufferFormat = %d, int windowed = %d", adapter, checkType, displayFormat, backBufferFormat, windowed);

		if(checkType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D9ex();

			if(d3d9ex)
			{
				return d3d9ex->CheckDeviceType(adapter, checkType, displayFormat, backBufferFormat, windowed);
			}
			else
			{
				return CheckDeviceType(adapter, D3DDEVTYPE_HAL, displayFormat, backBufferFormat, windowed);
			}
		}

		return Direct3D9::CheckDeviceType(adapter, checkType, displayFormat, backBufferFormat, windowed);
	}

	long Direct3D9Ex::CheckDeviceFormat(unsigned int adapter, D3DDEVTYPE deviceType, D3DFORMAT adapterFormat, unsigned long usage, D3DRESOURCETYPE resourceType, D3DFORMAT checkFormat)
	{
		TRACE("unsigned int adapter = %d, D3DDEVTYPE deviceType = %d, D3DFORMAT adapterFormat = %d, unsigned long usage = %d, D3DRESOURCETYPE resourceType = %d, D3DFORMAT checkFormat = %d", adapter, deviceType, adapterFormat, usage, resourceType, checkFormat);

		if(deviceType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D9ex();

			if(d3d9ex)
			{
				return d3d9ex->CheckDeviceFormat(adapter, deviceType, adapterFormat, usage, resourceType, checkFormat);
			}
			else
			{
				return CheckDeviceFormat(adapter, D3DDEVTYPE_HAL, adapterFormat, usage, resourceType, checkFormat);
			}
		}

		return Direct3D9::CheckDeviceFormat(adapter, deviceType, adapterFormat, usage, resourceType, checkFormat);
	}

	long Direct3D9Ex::CheckDeviceMultiSampleType(unsigned int adapter, D3DDEVTYPE deviceType, D3DFORMAT surfaceFormat, int windowed, D3DMULTISAMPLE_TYPE multiSampleType, unsigned long *qualityLevels)
	{
		TRACE("unsigned int adapter = %d, D3DDEVTYPE deviceType = %d, D3DFORMAT surfaceFormat = %d, int windowed = %d, D3DMULTISAMPLE_TYPE multiSampleType = %d, unsigned long *qualityLevels = 0x%0.8p", adapter, deviceType, surfaceFormat, windowed, multiSampleType, qualityLevels);

		if(deviceType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D9ex();

			if(d3d9ex)
			{
				return d3d9ex->CheckDeviceMultiSampleType(adapter, deviceType, surfaceFormat, windowed, multiSampleType, qualityLevels);
			}
			else
			{
				return CheckDeviceMultiSampleType(adapter, D3DDEVTYPE_HAL, surfaceFormat, windowed, multiSampleType, qualityLevels);
			}
		}

		return Direct3D9::CheckDeviceMultiSampleType(adapter, deviceType, surfaceFormat, windowed, multiSampleType, qualityLevels);
	}

	long Direct3D9Ex::CheckDepthStencilMatch(unsigned int adapter, D3DDEVTYPE deviceType, D3DFORMAT adapterFormat, D3DFORMAT renderTargetFormat, D3DFORMAT depthStencilFormat)
	{
		TRACE("unsigned int adapter = %d, D3DDEVTYPE deviceType = %d, D3DFORMAT adapterFormat = %d, D3DFORMAT renderTargetFormat = %d, D3DFORMAT depthStencilFormat = %d", adapter, deviceType, adapterFormat, renderTargetFormat, depthStencilFormat);

		return Direct3D9::CheckDepthStencilMatch(adapter, deviceType, adapterFormat, renderTargetFormat, depthStencilFormat);
	}

	long Direct3D9Ex::CheckDeviceFormatConversion(unsigned int adapter, D3DDEVTYPE deviceType, D3DFORMAT sourceFormat, D3DFORMAT targetFormat)
	{
		TRACE("unsigned int adapter = %d, D3DDEVTYPE deviceType = %d, D3DFORMAT sourceFormat = %d, D3DFORMAT targetFormat = %d", adapter, deviceType, sourceFormat, targetFormat);

		if(deviceType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D9ex();

			if(d3d9ex)
			{
				return d3d9ex->CheckDeviceFormatConversion(adapter, deviceType, sourceFormat, targetFormat);
			}
			else
			{
				return CheckDeviceFormatConversion(adapter, D3DDEVTYPE_HAL, sourceFormat, targetFormat);
			}
		}

		return D3D_OK;
	}

	long Direct3D9Ex::GetDeviceCaps(unsigned int adapter, D3DDEVTYPE deviceType, D3DCAPS9 *capabilities)
	{
		TRACE("unsigned int adapter = %d, D3DDEVTYPE deviceType = %d, D3DCAPS9 *capabilities = 0x%0.8p", adapter, deviceType, capabilities);

		if(deviceType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D9ex();

			if(d3d9ex)
			{
				return d3d9ex->GetDeviceCaps(adapter, deviceType, capabilities);
			}
			else
			{
				return GetDeviceCaps(adapter, D3DDEVTYPE_HAL, capabilities);
			}
		}

		long result = Direct3D9::GetDeviceCaps(adapter, deviceType, capabilities);

		if(sw::postBlendSRGB)
		{
			capabilities->PrimitiveMiscCaps |= D3DPMISCCAPS_POSTBLENDSRGBCONVERT;   // Indicates device can perform conversion to sRGB after blending.
		}

		return result;
	}

	HMONITOR Direct3D9Ex::GetAdapterMonitor(unsigned int adapter)
	{
		TRACE("unsigned int adapter = %d", adapter);

		return Direct3D9::GetAdapterMonitor(adapter);
	}

	long Direct3D9Ex::CreateDevice(unsigned int adapter, D3DDEVTYPE deviceType, HWND focusWindow, unsigned long behaviorFlags, D3DPRESENT_PARAMETERS *presentParameters, IDirect3DDevice9 **returnedDeviceInterface)
	{
		TRACE("unsigned int adapter = %d, D3DDEVTYPE deviceType = %d, HWND focusWindow = %d, unsigned long behaviorFlags = 0x%0.8X, D3DPRESENT_PARAMETERS *presentParameters = 0x%0.8p, IDirect3DDevice9 **returnedDeviceInterface = 0x%0.8p", adapter, deviceType, focusWindow, behaviorFlags, presentParameters, returnedDeviceInterface);

		if(!focusWindow || !presentParameters || !returnedDeviceInterface)
		{
			*returnedDeviceInterface = 0;

			return INVALIDCALL();
		}

		D3DDISPLAYMODEEX fullscreenDisplayMode = {0};
		fullscreenDisplayMode.Size = sizeof(D3DDISPLAYMODEEX);
		fullscreenDisplayMode.Format = presentParameters->BackBufferFormat;
		fullscreenDisplayMode.Width = presentParameters->BackBufferWidth;
		fullscreenDisplayMode.Height = presentParameters->BackBufferHeight;
		fullscreenDisplayMode.RefreshRate = presentParameters->FullScreen_RefreshRateInHz;
		fullscreenDisplayMode.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;

		return CreateDeviceEx(adapter, deviceType, focusWindow, behaviorFlags, presentParameters, presentParameters->Windowed ? 0 : &fullscreenDisplayMode, (IDirect3DDevice9Ex**)returnedDeviceInterface);
	}

	unsigned int __stdcall Direct3D9Ex::GetAdapterModeCountEx(unsigned int adapter, const D3DDISPLAYMODEFILTER *filter)
	{
		TRACE("unsigned int adapter = %d, const D3DDISPLAYMODEFILTER *filter = 0x%0.8p", adapter, filter);

		return Direct3D9::GetAdapterModeCount(adapter, filter->Format);   // FIXME
	}

	long __stdcall Direct3D9Ex::EnumAdapterModesEx(unsigned int adapter, const D3DDISPLAYMODEFILTER *filter, unsigned int index, D3DDISPLAYMODEEX *modeEx)
	{
		TRACE("unsigned int adapter = %d, const D3DDISPLAYMODEFILTER *filter = 0x%0.8p, unsigned int index = %d, D3DDISPLAYMODEEX *modeEx = 0x%0.8p", adapter, filter, index, modeEx);

		D3DDISPLAYMODE mode;

		mode.Format = modeEx->Format;
		mode.Width = modeEx->Width;
		mode.Height = modeEx->Height;
		mode.RefreshRate = modeEx->RefreshRate;

		return Direct3D9::EnumAdapterModes(adapter, filter->Format, index, &mode);   // FIXME
	}

	long __stdcall Direct3D9Ex::GetAdapterDisplayModeEx(unsigned int adapter, D3DDISPLAYMODEEX *modeEx, D3DDISPLAYROTATION *rotation)
	{
		TRACE("unsigned int adapter = %d, D3DDISPLAYMODEEX *modeEx = 0x%0.8p, D3DDISPLAYROTATION *rotation = 0x%0.8p", adapter, modeEx, rotation);

		D3DDISPLAYMODE mode;

		mode.Format = modeEx->Format;
		mode.Width = modeEx->Width;
		mode.Height = modeEx->Height;
		mode.RefreshRate = modeEx->RefreshRate;

		return GetAdapterDisplayMode(adapter, &mode);   // FIXME
	}

	long __stdcall Direct3D9Ex::CreateDeviceEx(unsigned int adapter, D3DDEVTYPE deviceType, HWND focusWindow, DWORD behaviorFlags, D3DPRESENT_PARAMETERS *presentParameters, D3DDISPLAYMODEEX *fullscreenDisplayMode, IDirect3DDevice9Ex **returnedDeviceInterface)
	{
		TRACE("unsigned int adapter = %d, D3DDEVTYPE deviceType = %d, HWND focusWindow = %d, DWORD behaviorFlags = 0x%0.8X, D3DPRESENT_PARAMETERS *presentParameters = 0x%0.8p, D3DDISPLAYMODEEX *fullscreenDisplayMode = 0x%0.8p, IDirect3DDevice9Ex **returnedDeviceInterface = 0x%0.8p", adapter, deviceType, focusWindow, behaviorFlags, presentParameters, fullscreenDisplayMode, returnedDeviceInterface);

		if(deviceType != D3DDEVTYPE_HAL)
		{
			loadSystemD3D9ex();

			if(d3d9ex)
			{
				return d3d9ex->CreateDeviceEx(adapter, deviceType, focusWindow, behaviorFlags, presentParameters, fullscreenDisplayMode, returnedDeviceInterface);
			}
			else
			{
				CreateDeviceEx(adapter, deviceType, focusWindow, behaviorFlags, presentParameters, fullscreenDisplayMode, returnedDeviceInterface);
			}
		}

		if(!focusWindow || !presentParameters || !returnedDeviceInterface)
		{
			*returnedDeviceInterface = 0;

			return INVALIDCALL();
		}

		*returnedDeviceInterface = new Direct3DDevice9Ex(instance, this, adapter, deviceType, focusWindow, behaviorFlags, presentParameters);

		if(*returnedDeviceInterface)
		{
			(*returnedDeviceInterface)->AddRef();
		}

		return D3D_OK;
	}

	long __stdcall Direct3D9Ex::GetAdapterLUID(unsigned int adapter, LUID *luid)
	{
		TRACE("unsigned int adapter = %d, LUID *luid = 0x%0.8p", adapter, luid);

		if(adapter != D3DADAPTER_DEFAULT)
		{
			UNIMPLEMENTED();
		}

		// FIXME: Should return a presistent id using AllocateLocallyUniqueId()
		luid->LowPart = 0x0000001;
		luid->HighPart = 0x0000000;

		return D3D_OK;
	}

	void Direct3D9Ex::loadSystemD3D9ex()
	{
		if(d3d9ex)
		{
			return;
		}

		char d3d9Path[MAX_PATH + 16];
		GetSystemDirectory(d3d9Path, MAX_PATH);
		strcat(d3d9Path, "\\d3d9.dll");
		d3d9Lib = LoadLibrary(d3d9Path);

		if(d3d9Lib)
		{
			typedef IDirect3D9Ex* (__stdcall *DIRECT3DCREATE9EX)(unsigned int, IDirect3D9Ex**);
			DIRECT3DCREATE9EX direct3DCreate9Ex = (DIRECT3DCREATE9EX)GetProcAddress(d3d9Lib, "Direct3DCreate9Ex");
			direct3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
		}
	}
}
