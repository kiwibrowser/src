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

#ifndef D3D8_Direct3DCubeTexture8_hpp
#define D3D8_Direct3DCubeTexture8_hpp

#include "Direct3DBaseTexture8.hpp"

#include "Config.hpp"

#include <d3d8.h>

namespace D3D8
{
	class Direct3DSurface8;

	class Direct3DCubeTexture8 : public IDirect3DCubeTexture8, public Direct3DBaseTexture8
	{
	public:
		Direct3DCubeTexture8(Direct3DDevice8 *device, unsigned int edgeLength, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool);

		~Direct3DCubeTexture8() override;

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object) override;
		unsigned long __stdcall AddRef() override;
		unsigned long __stdcall Release() override;

		// IDirect3DResource8 methods
		long __stdcall FreePrivateData(const GUID &guid) override;
		long __stdcall GetPrivateData(const GUID &guid, void *data, unsigned long *size) override;
		void __stdcall PreLoad() override;
		long __stdcall SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags) override;
		long __stdcall GetDevice(IDirect3DDevice8 **device) override;
		unsigned long __stdcall SetPriority(unsigned long newPriority) override;
		unsigned long __stdcall GetPriority() override;
		D3DRESOURCETYPE __stdcall GetType() override;

		// IDirect3DBaseTexture methods
		unsigned long __stdcall GetLevelCount() override;
		unsigned long __stdcall GetLOD() override;
		unsigned long __stdcall SetLOD(unsigned long newLOD) override;

		// IDirect3DCubeTexture8 methods
		long __stdcall AddDirtyRect(D3DCUBEMAP_FACES face, const RECT *dirtyRect) override;
		long __stdcall GetCubeMapSurface(D3DCUBEMAP_FACES face, unsigned int level , IDirect3DSurface8 **cubeMapSurface) override;
		long __stdcall GetLevelDesc(unsigned int level, D3DSURFACE_DESC *description) override;
		long __stdcall LockRect(D3DCUBEMAP_FACES face, unsigned int level, D3DLOCKED_RECT *lockedRect, const RECT *rect, unsigned long flags) override;
		long __stdcall UnlockRect(D3DCUBEMAP_FACES face, unsigned int level) override;

		// Internal methods
		Direct3DSurface8 *getInternalCubeMapSurface(D3DCUBEMAP_FACES face, unsigned int level);

	private:
		// Creation parameters
		const unsigned int edgeLength;
		const D3DFORMAT format;
		const D3DPOOL pool;

		Direct3DSurface8 *surfaceLevel[6][sw::MIPMAP_LEVELS];
	};
}

#endif // D3D8_Direct3D8_hpp
