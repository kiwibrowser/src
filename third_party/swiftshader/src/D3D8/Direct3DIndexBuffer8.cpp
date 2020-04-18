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

#include "Direct3DIndexBuffer8.hpp"

#include "Direct3DDevice8.hpp"
#include "Resource.hpp"
#include "Debug.hpp"

#include <assert.h>

namespace D3D8
{
	Direct3DIndexBuffer8::Direct3DIndexBuffer8(Direct3DDevice8 *device, unsigned int length, unsigned long usage, D3DFORMAT format, D3DPOOL pool) : Direct3DResource8(device, D3DRTYPE_INDEXBUFFER, length), length(length), usage(usage), format(format), pool(pool)
	{
		indexBuffer = new sw::Resource(length + 16);
	}

	Direct3DIndexBuffer8::~Direct3DIndexBuffer8()
	{
		indexBuffer->destruct();
	}

	long Direct3DIndexBuffer8::QueryInterface(const IID &iid, void **object)
	{
		TRACE("");

		if(iid == IID_IDirect3DIndexBuffer8 ||
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

	unsigned long Direct3DIndexBuffer8::AddRef()
	{
		TRACE("");

		return Direct3DResource8::AddRef();
	}

	unsigned long Direct3DIndexBuffer8::Release()
	{
		TRACE("");

		return Direct3DResource8::Release();
	}

	long Direct3DIndexBuffer8::FreePrivateData(const GUID &guid)
	{
		TRACE("");

		return Direct3DResource8::FreePrivateData(guid);
	}

	long Direct3DIndexBuffer8::GetPrivateData(const GUID &guid, void *data, unsigned long *size)
	{
		TRACE("");

		return Direct3DResource8::GetPrivateData(guid, data, size);
	}

	void Direct3DIndexBuffer8::PreLoad()
	{
		TRACE("");

		Direct3DResource8::PreLoad();
	}

	long Direct3DIndexBuffer8::SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags)
	{
		TRACE("");

		return Direct3DResource8::SetPrivateData(guid, data, size, flags);
	}

	long Direct3DIndexBuffer8::GetDevice(IDirect3DDevice8 **device)
	{
		TRACE("");

		return Direct3DResource8::GetDevice(device);
	}

	unsigned long Direct3DIndexBuffer8::SetPriority(unsigned long newPriority)
	{
		TRACE("");

		return Direct3DResource8::SetPriority(newPriority);
	}

	unsigned long Direct3DIndexBuffer8::GetPriority()
	{
		TRACE("");

		return Direct3DResource8::GetPriority();
	}

	D3DRESOURCETYPE Direct3DIndexBuffer8::GetType()
	{
		TRACE("");

		return Direct3DResource8::GetType();
	}

	long Direct3DIndexBuffer8::GetDesc(D3DINDEXBUFFER_DESC *description)
	{
		TRACE("");

		if(!description)
		{
			return INVALIDCALL();
		}

		description->Format = format;
		description->Pool = pool;
		description->Size = length;
		description->Type = GetType();
		description->Usage = usage;

		return 0;
	}

	long Direct3DIndexBuffer8::Lock(unsigned int offset, unsigned int size, unsigned char **data, unsigned long flags)
	{
		TRACE("");

		if(offset == 0 && size == 0)   // Lock whole buffer
		{
			size = length;
		}

		if(!data || offset + size > length)
		{
			return INVALIDCALL();
		}

		lockOffset = offset;
		lockSize = size;

		*data = (unsigned char*)indexBuffer->lock(sw::PUBLIC) + offset;
		indexBuffer->unlock();

		return D3D_OK;
	}

	long Direct3DIndexBuffer8::Unlock()
	{
		TRACE("");

		return D3D_OK;
	}

	sw::Resource *Direct3DIndexBuffer8::getResource() const
	{	
		return indexBuffer;
	}

	bool Direct3DIndexBuffer8::is32Bit() const
	{
		switch(format)
		{
		case D3DFMT_INDEX16:
			return false;
		case D3DFMT_INDEX32:
			return true;
		default:
			ASSERT(false);
		}

		return false;
	}
}