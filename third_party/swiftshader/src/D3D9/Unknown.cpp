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

#include "Unknown.hpp"

#include "Debug.hpp"

namespace D3D9
{
	Unknown::Unknown()
	{
		referenceCount = 0;
		bindCount = 0;
	}

	Unknown::~Unknown()
	{
		ASSERT(referenceCount == 0);
		ASSERT(bindCount == 0);
	}

	long Unknown::QueryInterface(const IID &iid, void **object)
	{
		if(iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Unknown::AddRef()
	{
		return InterlockedIncrement(&referenceCount);
	}

	unsigned long Unknown::Release()
	{
		int current = referenceCount;

		if(referenceCount > 0)
		{
			current = InterlockedDecrement(&referenceCount);
		}

		if(referenceCount == 0 && bindCount == 0)
		{
			delete this;
		}

		return current;
	}

	void Unknown::bind()
	{
		InterlockedIncrement(&bindCount);
	}

	void Unknown::unbind()
	{
		ASSERT(bindCount > 0);

		InterlockedDecrement(&bindCount);

		if(referenceCount == 0 && bindCount == 0)
		{
			delete this;
		}
	}
}