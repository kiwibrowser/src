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

#ifndef D3D9_Direct3DBaseTexture9_hpp
#define D3D9_Direct3DBaseTexture9_hpp

#include "Direct3DResource9.hpp"

#include <d3d9.h>

namespace sw
{
	class Resource;
}

namespace D3D9
{
	class Direct3DBaseTexture9 : public IDirect3DBaseTexture9, public Direct3DResource9
	{
	public:
		Direct3DBaseTexture9(Direct3DDevice9 *device, D3DRESOURCETYPE type, D3DFORMAT format, D3DPOOL pool, unsigned long levels, unsigned long usage);

		~Direct3DBaseTexture9() override;

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object) override;
		unsigned long __stdcall AddRef() override;
		unsigned long __stdcall Release() override;

		// IDirect3DResource9 methods
		long __stdcall GetDevice(IDirect3DDevice9 **device) override;
		long __stdcall SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags) override;
		long __stdcall GetPrivateData(const GUID &guid, void *data, unsigned long *size) override;
		long __stdcall FreePrivateData(const GUID &guid) override;
		unsigned long __stdcall SetPriority(unsigned long newPriority) override;
		unsigned long __stdcall GetPriority() override;
		void __stdcall PreLoad() override;
		D3DRESOURCETYPE __stdcall GetType() override;

		// IDirect3DBaseTexture9 methods
		unsigned long __stdcall SetLOD(unsigned long newLOD) override;
		unsigned long __stdcall GetLOD() override;
		unsigned long __stdcall GetLevelCount() override;
		long __stdcall SetAutoGenFilterType(D3DTEXTUREFILTERTYPE filterType) override;
		D3DTEXTUREFILTERTYPE __stdcall GetAutoGenFilterType() override;
		void __stdcall GenerateMipSubLevels() override;

		// Intenal methods
		sw::Resource *getResource() const;
		unsigned long getInternalLevelCount() const;
		unsigned long getUsage() const;
		D3DFORMAT getFormat() const;

	protected:
		// Creation parameters
		unsigned long levels;   // Recalculated when 0
		const unsigned long usage;
		const D3DFORMAT format;

		sw::Resource *resource;

	private:
		D3DTEXTUREFILTERTYPE filterType;
		unsigned long LOD;
	};
}

#endif // D3D9_Direct3DBaseTexture9_hpp
