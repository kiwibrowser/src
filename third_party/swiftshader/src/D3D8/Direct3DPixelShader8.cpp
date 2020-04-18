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

#include "Direct3DPixelShader8.hpp"

#include "Debug.hpp"

namespace D3D8
{
	Direct3DPixelShader8::Direct3DPixelShader8(Direct3DDevice8 *device, const unsigned long *shaderToken) : device(device), pixelShader(shaderToken)
	{
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

	Direct3DPixelShader8::~Direct3DPixelShader8()
	{
		delete[] shaderToken;
		shaderToken = 0;
	}

	long Direct3DPixelShader8::QueryInterface(const IID &iid, void **object)
	{
		TRACE("");

		ASSERT(false);   // Internal object

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DPixelShader8::AddRef()
	{
		TRACE("");

		return Unknown::AddRef();
	}
	
	unsigned long Direct3DPixelShader8::Release()
	{
		TRACE("");

		return Unknown::Release();
	}

	void Direct3DPixelShader8::GetFunction(void *data, unsigned int *size)
	{
		TRACE("");

		if(data)
		{
			memcpy(data, shaderToken, this->size * 4);
		}

		*size = this->size * 4;
	}

	const sw::PixelShader *Direct3DPixelShader8::getPixelShader() const
	{
		return &pixelShader;
	}
}