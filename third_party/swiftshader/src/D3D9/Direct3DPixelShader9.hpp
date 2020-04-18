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

#ifndef D3D9_Direct3DPixelShader9_hpp
#define D3D9_Direct3DPixelShader9_hpp

#include "Unknown.hpp"

#include "PixelShader.hpp"

#include <d3d9.h>

namespace D3D9
{
	class Direct3DDevice9;

	class Direct3DPixelShader9 : public IDirect3DPixelShader9, public Unknown
	{
	public:
		Direct3DPixelShader9(Direct3DDevice9 *device, const unsigned long *shaderToken);

		~Direct3DPixelShader9() override;

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object) override;
		unsigned long __stdcall AddRef() override;
		unsigned long __stdcall Release() override;

		// IDirect3DPixelShader9 methods
		long __stdcall GetDevice(IDirect3DDevice9 **device) override;
		long __stdcall GetFunction(void *data, unsigned int *size) override;

		// Internal methods
		const sw::PixelShader *getPixelShader() const;

	private:
		// Creation parameters
		Direct3DDevice9 *const device;

		unsigned long *shaderToken;
		int tokenCount;

		sw::PixelShader pixelShader;
	};
}

#endif   // D3D9_Direct3DPixelShader9_hpp
