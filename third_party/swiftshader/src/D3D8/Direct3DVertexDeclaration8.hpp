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

#ifndef D3D8_Direct3DVertexDeclaration8_hpp
#define D3D8_Direct3DVertexDeclaration8_hpp

#include "Unknown.hpp"

#include <d3d8.h>

namespace D3D8
{
	class Direct3DDevice8;

	class Direct3DVertexDeclaration8 : protected Unknown
	{
	public:
		Direct3DVertexDeclaration8(Direct3DDevice8 *device, const unsigned long *vertexElements);

		~Direct3DVertexDeclaration8() override;

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object) override;
		unsigned long __stdcall AddRef() override;
		unsigned long __stdcall Release() override;

		// Internal methods
		const unsigned long *getDeclaration() const;

	private:
		// Creation parameters
		Direct3DDevice8 *const device;
		unsigned long *declaration;
	};
}

#endif   // D3D8_Direct3DVertexDeclaration8_hpp