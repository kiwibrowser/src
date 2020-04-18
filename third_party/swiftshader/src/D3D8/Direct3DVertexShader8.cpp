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

#include "Direct3DVertexShader8.hpp"

#include "Debug.hpp"

namespace D3D8
{
	Direct3DVertexShader8::Direct3DVertexShader8(Direct3DDevice8 *device, const unsigned long *declaration, const unsigned long *shaderToken) : device(device)
	{
		if(shaderToken)
		{
			vertexShader = new sw::VertexShader(shaderToken);

			const unsigned long *token = shaderToken;
			size = 0;

			while(shaderToken[size] != 0x0000FFFF)
			{
				size++;
			}

			size++;

			this->shaderToken = new unsigned long[size];
			memcpy(this->shaderToken, shaderToken, size * sizeof(unsigned long));
		}
		else
		{
			vertexShader = 0;
			this->shaderToken = 0;
		}

		this->declaration = new Direct3DVertexDeclaration8(device, declaration);
	}

	Direct3DVertexShader8::~Direct3DVertexShader8()
	{
		delete vertexShader;
		vertexShader = 0;

		delete[] shaderToken;
		shaderToken = 0;

		declaration->Release();
	}

	long Direct3DVertexShader8::QueryInterface(const IID &iid, void **object)
	{
		TRACE("");

		ASSERT(false);   // Internal object

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DVertexShader8::AddRef()
	{
		TRACE("");

		return Unknown::AddRef();
	}
	
	unsigned long Direct3DVertexShader8::Release()
	{
		TRACE("");

		return Unknown::Release();
	}

	const sw::VertexShader *Direct3DVertexShader8::getVertexShader() const
	{
		return vertexShader;
	}

	const unsigned long *Direct3DVertexShader8::getDeclaration()
	{
		return declaration->getDeclaration();
	}
}