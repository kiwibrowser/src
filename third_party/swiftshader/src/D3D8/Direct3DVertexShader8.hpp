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

#ifndef D3D8_Direct3DVertexShader8_hpp
#define D3D8_Direct3DVertexShader8_hpp

#include "VertexShader.hpp"
#include "Direct3DVertexDeclaration8.hpp"

#include "Unknown.hpp"

namespace D3D8
{
	class Direct3DDevice8;

	class Direct3DVertexShader8 : public Unknown
	{
	public:
		Direct3DVertexShader8(Direct3DDevice8 *device, const unsigned long *declaration, const unsigned long *shaderToken);

		~Direct3DVertexShader8() override;

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object) override;
		unsigned long __stdcall AddRef() override;
		unsigned long __stdcall Release() override;

		// Internal methods
		const sw::VertexShader *getVertexShader() const;
		const unsigned long *getDeclaration();

	private:
		// Creation parameters
		Direct3DDevice8 *const device;
		Direct3DVertexDeclaration8 *declaration;
		unsigned long *shaderToken;
		unsigned int size;

		sw::VertexShader *vertexShader;
	};
}

#endif   // D3D8_Direct3DVertexShader8_hpp