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

#ifndef D3D9_Direct3DVertexDeclaration9_hpp
#define D3D9_Direct3DVertexDeclaration9_hpp

#include "Unknown.hpp"

#include <d3d9.h>

namespace D3D9
{
	class Direct3DDevice9;

	class Direct3DVertexDeclaration9 : public IDirect3DVertexDeclaration9, public Unknown
	{
	public:
		Direct3DVertexDeclaration9(Direct3DDevice9 *device, const D3DVERTEXELEMENT9 *vertexElements);
		Direct3DVertexDeclaration9(Direct3DDevice9 *device, unsigned long FVF);

		~Direct3DVertexDeclaration9() override;

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object) override;
		unsigned long __stdcall AddRef() override;
		unsigned long __stdcall Release() override;

		// IDirect3DVertexDeclaration9 methods
		long __stdcall GetDevice(IDirect3DDevice9 **device) override;
		long __stdcall GetDeclaration(D3DVERTEXELEMENT9 *declaration, unsigned int *numElements) override;

		// Internal methods
		unsigned long getFVF() const;
		bool isPreTransformed() const;

	private:
		unsigned long computeFVF();   // If equivalent to FVF, else 0

		// Creation parameters
		Direct3DDevice9 *const device;
		D3DVERTEXELEMENT9 *vertexElement;

		int numElements;
		unsigned long FVF;
		bool preTransformed;
	};
}

#endif   // D3D9_Direct3DVertexDeclaration9_hpp
