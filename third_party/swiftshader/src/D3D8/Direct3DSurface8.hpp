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

#ifndef D3D8_Direct3DSurface8_hpp
#define D3D8_Direct3DSurface8_hpp

#include "Unknown.hpp"
#include "Surface.hpp"

#include <d3d8.h>

namespace D3D8
{
	class Direct3DDevice8;
	class Direct3DResource8;
	class Direct3DBaseTexture8;

	class Direct3DSurface8 : public IDirect3DSurface8, public Unknown, public sw::Surface
	{
	public:
		Direct3DSurface8(Direct3DDevice8 *device, Unknown *container, int width, int height, D3DFORMAT format, D3DPOOL pool, D3DMULTISAMPLE_TYPE multiSample, bool lockable, unsigned long usage);

		~Direct3DSurface8() override;

		// Surface methods
		void *lockInternal(int x, int y, int z, sw::Lock lock, sw::Accessor client) override;
		void unlockInternal() override;

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object) override;
		unsigned long __stdcall AddRef() override;
		unsigned long __stdcall Release() override;

		// IDirect3DSurface8 methods
		long __stdcall GetDevice(IDirect3DDevice8 **device) override;
		long __stdcall SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags) override;
		long __stdcall GetPrivateData(const GUID &guid, void *data, unsigned long *size) override;
		long __stdcall FreePrivateData(const GUID &guid) override;
		long __stdcall GetContainer(const IID &iid, void **container) override;
		long __stdcall GetDesc(D3DSURFACE_DESC *desc) override;
		long __stdcall LockRect(D3DLOCKED_RECT *lockedRect, const RECT *rect, unsigned long Flags) override;
		long __stdcall UnlockRect() override;

		// Internal methods
		static sw::Format translateFormat(D3DFORMAT format);
		static int bytes(D3DFORMAT format);

	private:
		static unsigned int memoryUsage(int width, int height, D3DMULTISAMPLE_TYPE multiSample, D3DFORMAT format);   // FIXME: Surface::size

		// Creation parameters
		Direct3DDevice8 *const device;
		Unknown *const container;
		const int width;
		const int height;
		const D3DFORMAT format;
		const D3DMULTISAMPLE_TYPE multiSample;
		const D3DPOOL pool;
		const bool lockable;
		const unsigned long usage;

		Direct3DBaseTexture8 *parentTexture;
		Direct3DResource8 *resource;
	};
}

#endif // D3D8_Direct3DSurface8_hpp
