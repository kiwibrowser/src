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

#include "Direct3DVertexDeclaration8.hpp"

#include "Debug.hpp"

#include <d3d8types.h>

namespace D3D8
{
	Direct3DVertexDeclaration8::Direct3DVertexDeclaration8(Direct3DDevice8 *device, const unsigned long *vertexElement) : device(device)
	{
		int size = sizeof(unsigned long);
		const unsigned long *element = vertexElement;

		while(*element != 0xFFFFFFFF)
		{
			size += sizeof(unsigned long);
			element++;
		}

		declaration = new unsigned long[size  / sizeof(unsigned long)];
		memcpy(declaration, vertexElement, size);
	}

	Direct3DVertexDeclaration8::~Direct3DVertexDeclaration8()
	{
		delete[] declaration;
		declaration = 0;
	}

	long Direct3DVertexDeclaration8::QueryInterface(const IID &iid, void **object)
	{
		TRACE("");

		ASSERT(false);   // Internal object

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DVertexDeclaration8::AddRef()
	{
		TRACE("");

		return Unknown::AddRef();
	}
	
	unsigned long Direct3DVertexDeclaration8::Release()
	{
		TRACE("");

		return Unknown::Release();
	}

	const unsigned long *Direct3DVertexDeclaration8::getDeclaration() const
	{
		return declaration;
	}
}