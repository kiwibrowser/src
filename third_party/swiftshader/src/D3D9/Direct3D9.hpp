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

#ifndef D3D9_Direct3D9_hpp
#define D3D9_Direct3D9_hpp

#include "Unknown.hpp"

#include <stdio.h>
#include <d3d9.h>

namespace D3D9
{
	class Direct3DDevice9;

	class Direct3D9 : public IDirect3D9, public Unknown
	{
	public:
		Direct3D9(int version, const HINSTANCE instance);

		~Direct3D9() override;

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object) override;
		unsigned long __stdcall AddRef() override;
		unsigned long __stdcall Release() override;

		// IDirect3D9 methods
		long __stdcall RegisterSoftwareDevice(void *initializeFunction) override;
		unsigned int __stdcall GetAdapterCount() override;
		long __stdcall GetAdapterIdentifier(unsigned int adapter, unsigned long flags, D3DADAPTER_IDENTIFIER9 *identifier) override;
		unsigned int __stdcall GetAdapterModeCount(unsigned int adapter, D3DFORMAT format) override;
		long __stdcall EnumAdapterModes(unsigned int adapter, D3DFORMAT format, unsigned int index, D3DDISPLAYMODE *mode) override;
		long __stdcall GetAdapterDisplayMode(unsigned int adapter, D3DDISPLAYMODE *mode) override;
		long __stdcall CheckDeviceType(unsigned int adapter, D3DDEVTYPE checkType, D3DFORMAT displayFormat, D3DFORMAT backBufferFormat, int windowed) override;
		long __stdcall CheckDeviceFormat(unsigned int adapter, D3DDEVTYPE deviceType, D3DFORMAT adapterFormat, unsigned long usage, D3DRESOURCETYPE type, D3DFORMAT checkFormat) override;
		long __stdcall CheckDeviceMultiSampleType(unsigned int adapter, D3DDEVTYPE deviceType, D3DFORMAT surfaceFormat, int windowed, D3DMULTISAMPLE_TYPE multiSampleType, unsigned long *qualityLevels) override;
		long __stdcall CheckDepthStencilMatch(unsigned int adapter, D3DDEVTYPE deviceType, D3DFORMAT adapterFormat, D3DFORMAT renderTargetFormat, D3DFORMAT depthStencilFormat) override;
		long __stdcall CheckDeviceFormatConversion(unsigned int adapter, D3DDEVTYPE deviceType, D3DFORMAT sourceFormat, D3DFORMAT targetFormat) override;
		long __stdcall GetDeviceCaps(unsigned int adapter, D3DDEVTYPE deviceType, D3DCAPS9 *caps) override;
		HMONITOR __stdcall GetAdapterMonitor(unsigned int adapter) override;
		long __stdcall CreateDevice(unsigned int adapter, D3DDEVTYPE deviceType, HWND focusWindow, unsigned long behaviorFlags, D3DPRESENT_PARAMETERS *presentParameters, IDirect3DDevice9 **returnedDeviceInterface) override;

	protected:
		// Creation parameters
		const int version;
		const HINSTANCE instance;

		// Real D3D9 library
		HMODULE d3d9Lib;

	private:
		void loadSystemD3D9();

		DEVMODE *displayMode;
		int numDisplayModes;
		bool disableAlphaMode;   // Disable A8R8G8B8 display mode support
		bool disable10BitMode;   // Disable A2R10G10B10 display mode support

		//Real IDirect3D9 object
		IDirect3D9 *d3d9;
	};
}

#endif   // D3D9_Direct3D9_hpp
