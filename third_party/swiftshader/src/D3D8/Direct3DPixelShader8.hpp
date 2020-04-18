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

#ifndef D3D8_Direct3DPixelShader8_hpp
#define D3D8_Direct3DPixelShader8_hpp

#include "PixelShader.hpp"

#include "Unknown.hpp"

namespace D3D8
{
	class Direct3DDevice8;

	class Direct3DPixelShader8 : public Unknown
	{
	public:
		Direct3DPixelShader8(Direct3DDevice8 *device, const unsigned long *shaderToken);

		~Direct3DPixelShader8() override;

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object) override;
		unsigned long __stdcall AddRef() override;
		unsigned long __stdcall Release() override;

		void __stdcall GetFunction(void *data, unsigned int *size);

		// Internal methods
		const sw::PixelShader *getPixelShader() const;

	private:
		// Creation parameters
		Direct3DDevice8 *const device;
		unsigned long *shaderToken;
		unsigned int size;

		sw::PixelShader pixelShader;
	};
}

#endif   // D3D8_Direct3DPixelShader8_hpp