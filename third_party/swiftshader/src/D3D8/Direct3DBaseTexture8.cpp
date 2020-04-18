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

#include "Direct3DBaseTexture8.hpp"

#include "Resource.hpp"
#include "Debug.hpp"

namespace D3D8
{
	Direct3DBaseTexture8::Direct3DBaseTexture8(Direct3DDevice8 *device, D3DRESOURCETYPE type, unsigned long levels, unsigned long usage) : Direct3DResource8(device, type, 0), levels(levels), usage(usage)
	{
		filterType = D3DTEXF_LINEAR;
		LOD = 0;

		resource = new sw::Resource(0);
	}

	Direct3DBaseTexture8::~Direct3DBaseTexture8()
	{
		resource->destruct();
	}
	
	long Direct3DBaseTexture8::QueryInterface(const IID &iid, void **object)
	{
		TRACE("");

		if(iid == IID_IDirect3DBaseTexture8 ||
		   iid == IID_IDirect3DResource8 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DBaseTexture8::AddRef()
	{
		TRACE("");

		return Direct3DResource8::AddRef();
	}

	unsigned long Direct3DBaseTexture8::Release()
	{
		TRACE("");

		return Direct3DResource8::Release();
	}

	long Direct3DBaseTexture8::FreePrivateData(const GUID &guid)
	{
		TRACE("");

		return Direct3DResource8::FreePrivateData(guid);
	}

	long Direct3DBaseTexture8::GetPrivateData(const GUID &guid, void *data, unsigned long *size)
	{
		TRACE("");

		return Direct3DResource8::GetPrivateData(guid, data, size);
	}

	void Direct3DBaseTexture8::PreLoad()
	{
		TRACE("");

		Direct3DResource8::PreLoad();
	}

	long Direct3DBaseTexture8::SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags)
	{
		TRACE("");

		return Direct3DResource8::SetPrivateData(guid, data, size, flags);
	}

	long Direct3DBaseTexture8::GetDevice(IDirect3DDevice8 **device)
	{
		TRACE("");

		return Direct3DResource8::GetDevice(device);
	}

	unsigned long Direct3DBaseTexture8::SetPriority(unsigned long newPriority)
	{
		TRACE("");

		return Direct3DResource8::SetPriority(newPriority);
	}

	unsigned long Direct3DBaseTexture8::GetPriority()
	{
		TRACE("");

		return Direct3DResource8::GetPriority();
	}

	D3DRESOURCETYPE Direct3DBaseTexture8::GetType()
	{
		TRACE("");

		return Direct3DResource8::GetType();
	}

	unsigned long Direct3DBaseTexture8::GetLevelCount()
	{
		TRACE("");

		return levels;
	}

	unsigned long Direct3DBaseTexture8::GetLOD()
	{
		TRACE("");

		return LOD;
	}

	unsigned long Direct3DBaseTexture8::SetLOD(unsigned long newLOD)
	{
		TRACE("");

		LOD = newLOD;

		return 0;   // TODO
	}

	sw::Resource *Direct3DBaseTexture8::getResource() const
	{
		return resource;
	}

	unsigned long Direct3DBaseTexture8::getInternalLevelCount()
	{
		return levels;
	}
}
