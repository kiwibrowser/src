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

#include "Direct3DIndexBuffer9.hpp"

#include "Direct3DDevice9.hpp"
#include "Resource.hpp"
#include "Debug.hpp"

#include <assert.h>

namespace D3D9
{
	Direct3DIndexBuffer9::Direct3DIndexBuffer9(Direct3DDevice9 *device, unsigned int length, unsigned long usage, D3DFORMAT format, D3DPOOL pool) : Direct3DResource9(device, D3DRTYPE_INDEXBUFFER, pool, length), length(length), usage(usage), format(format)
	{
		indexBuffer = new sw::Resource(length + 16);
		lockCount = 0;
	}

	Direct3DIndexBuffer9::~Direct3DIndexBuffer9()
	{
		indexBuffer->destruct();
	}

	long Direct3DIndexBuffer9::QueryInterface(const IID &iid, void **object)
	{
		CriticalSection cs(device);

		TRACE("");

		if(iid == IID_IDirect3DIndexBuffer9 ||
		   iid == IID_IDirect3DResource9 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DIndexBuffer9::AddRef()
	{
		TRACE("");

		return Direct3DResource9::AddRef();
	}

	unsigned long Direct3DIndexBuffer9::Release()
	{
		TRACE("");

		return Direct3DResource9::Release();
	}

	long Direct3DIndexBuffer9::FreePrivateData(const GUID &guid)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::FreePrivateData(guid);
	}

	long Direct3DIndexBuffer9::GetPrivateData(const GUID &guid, void *data, unsigned long *size)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::GetPrivateData(guid, data, size);
	}

	void Direct3DIndexBuffer9::PreLoad()
	{
		CriticalSection cs(device);

		TRACE("");

		Direct3DResource9::PreLoad();
	}

	long Direct3DIndexBuffer9::SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::SetPrivateData(guid, data, size, flags);
	}

	long Direct3DIndexBuffer9::GetDevice(IDirect3DDevice9 **device)
	{
		CriticalSection cs(this->device);

		TRACE("");

		return Direct3DResource9::GetDevice(device);
	}

	unsigned long Direct3DIndexBuffer9::SetPriority(unsigned long newPriority)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::SetPriority(newPriority);
	}

	unsigned long Direct3DIndexBuffer9::GetPriority()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::GetPriority();
	}

	D3DRESOURCETYPE Direct3DIndexBuffer9::GetType()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::GetType();
	}

	long Direct3DIndexBuffer9::GetDesc(D3DINDEXBUFFER_DESC *description)
	{
		CriticalSection cs(device);

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

	long Direct3DIndexBuffer9::Lock(unsigned int offset, unsigned int size, void **data, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("");

		if(offset == 0 && size == 0)   // Lock whole buffer
		{
			size = length;
		}

		if(!data || offset > length || offset + size > length)
		{
			return INVALIDCALL();
		}

		void *buffer;

		if(flags & D3DLOCK_DISCARD/* && usage & D3DUSAGE_DYNAMIC*/)
		{
			indexBuffer->destruct();
			indexBuffer = new sw::Resource(length + 16);

			buffer = (void*)indexBuffer->data();
		}
		else if(flags & D3DLOCK_NOOVERWRITE/* && usage & D3DUSAGE_DYNAMIC*/)
		{
			buffer = (void*)indexBuffer->data();
		}
		else
		{
			buffer = indexBuffer->lock(sw::PUBLIC);
			lockCount++;
		}

		*data = (unsigned char*)buffer + offset;

		return D3D_OK;
	}

	long Direct3DIndexBuffer9::Unlock()
	{
		CriticalSection cs(device);

		TRACE("");

		if(lockCount > 0)
		{
			indexBuffer->unlock();
			lockCount--;
		}

		return D3D_OK;
	}

	sw::Resource *Direct3DIndexBuffer9::getResource() const
	{
		return indexBuffer;
	}

	bool Direct3DIndexBuffer9::is32Bit() const
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
