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

#include "Direct3DVertexBuffer9.hpp"

#include "Direct3DDevice9.hpp"
#include "Resource.hpp"
#include "Debug.hpp"

#include <assert.h>

namespace D3D9
{
	Direct3DVertexBuffer9::Direct3DVertexBuffer9(Direct3DDevice9 *device, unsigned int length, unsigned long usage, long FVF, D3DPOOL pool) : Direct3DResource9(device, D3DRTYPE_VERTEXBUFFER, pool, length), length(length), usage(usage), FVF(FVF)
	{
		if(FVF)
		{
			unsigned int stride = 0;

			switch(FVF & D3DFVF_POSITION_MASK)
			{
			case D3DFVF_XYZ:	stride += 12;	break;
			case D3DFVF_XYZRHW:	stride += 16;	break;
			case D3DFVF_XYZB1:	stride += 16;	break;
			case D3DFVF_XYZB2:	stride += 20;	break;
			case D3DFVF_XYZB3:	stride += 24;	break;
			case D3DFVF_XYZB4:	stride += 28;	break;
			case D3DFVF_XYZB5:	stride += 32;	break;
			case D3DFVF_XYZW:   stride += 16;   break;
			}

			if(FVF & D3DFVF_NORMAL)   stride += 12;
			if(FVF & D3DFVF_PSIZE)    stride += 4;
			if(FVF & D3DFVF_DIFFUSE)  stride += 4;
			if(FVF & D3DFVF_SPECULAR) stride += 4;

			switch((FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT)
			{
			case 8: stride += 4 + 4 * ((1 + (FVF >> 30)) % 4);
			case 7: stride += 4 + 4 * ((1 + (FVF >> 28)) % 4);
			case 6: stride += 4 + 4 * ((1 + (FVF >> 26)) % 4);
			case 5: stride += 4 + 4 * ((1 + (FVF >> 24)) % 4);
			case 4: stride += 4 + 4 * ((1 + (FVF >> 22)) % 4);
			case 3: stride += 4 + 4 * ((1 + (FVF >> 20)) % 4);
			case 2: stride += 4 + 4 * ((1 + (FVF >> 18)) % 4);
			case 1: stride += 4 + 4 * ((1 + (FVF >> 16)) % 4);
			case 0: break;
			default:
				ASSERT(false);
			}

			ASSERT(length >= stride);       // FIXME
		}

		vertexBuffer = new sw::Resource(length + 192 + 1024);   // NOTE: Applications can 'overshoot' while writing vertices
		lockCount = 0;
	}

	Direct3DVertexBuffer9::~Direct3DVertexBuffer9()
	{
		vertexBuffer->destruct();
	}

	long Direct3DVertexBuffer9::QueryInterface(const IID &iid, void **object)
	{
		CriticalSection cs(device);

		TRACE("");

		if(iid == IID_IDirect3DVertexBuffer9 ||
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

	unsigned long Direct3DVertexBuffer9::AddRef()
	{
		TRACE("");

		return Direct3DResource9::AddRef();
	}

	unsigned long Direct3DVertexBuffer9::Release()
	{
		TRACE("");

		return Direct3DResource9::Release();
	}

	long Direct3DVertexBuffer9::FreePrivateData(const GUID &guid)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::FreePrivateData(guid);
	}

	long Direct3DVertexBuffer9::GetPrivateData(const GUID &guid, void *data, unsigned long *size)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::GetPrivateData(guid, data, size);
	}

	void Direct3DVertexBuffer9::PreLoad()
	{
		CriticalSection cs(device);

		TRACE("");

		Direct3DResource9::PreLoad();
	}

	long Direct3DVertexBuffer9::SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::SetPrivateData(guid, data, size, flags);
	}

	long Direct3DVertexBuffer9::GetDevice(IDirect3DDevice9 **device)
	{
		CriticalSection cs(this->device);

		TRACE("");

		return Direct3DResource9::GetDevice(device);
	}

	unsigned long Direct3DVertexBuffer9::SetPriority(unsigned long newPriority)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::SetPriority(newPriority);
	}

	unsigned long Direct3DVertexBuffer9::GetPriority()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::GetPriority();
	}

	D3DRESOURCETYPE Direct3DVertexBuffer9::GetType()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::GetType();
	}

	long Direct3DVertexBuffer9::Lock(unsigned int offset, unsigned int size, void **data, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("");

		if(offset == 0 && size == 0)   // Lock whole buffer
		{
			size = length;
		}

		if(!data || offset + size > length)
		{
			return INVALIDCALL();
		}

		void *buffer;

		if(flags & D3DLOCK_DISCARD/* && usage & D3DUSAGE_DYNAMIC*/)
		{
			vertexBuffer->destruct();
			vertexBuffer = new sw::Resource(length + 192 + 1024);   // NOTE: Applications can 'overshoot' while writing vertices

			buffer = (void*)vertexBuffer->data();
		}
		else if(flags & D3DLOCK_NOOVERWRITE/* && usage & D3DUSAGE_DYNAMIC*/)
		{
			buffer = (void*)vertexBuffer->data();
		}
		else
		{
			buffer = vertexBuffer->lock(sw::PUBLIC);
			lockCount++;
		}

		*data = (unsigned char*)buffer + offset;

		return D3D_OK;
	}

	long Direct3DVertexBuffer9::Unlock()
	{
		CriticalSection cs(device);

		TRACE("");

		if(lockCount > 0)
		{
			vertexBuffer->unlock();
			lockCount--;
		}

		return D3D_OK;
	}

	long Direct3DVertexBuffer9::GetDesc(D3DVERTEXBUFFER_DESC *description)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!description)
		{
			return INVALIDCALL();
		}

		description->FVF = FVF;
		description->Format = D3DFMT_VERTEXDATA;
		description->Pool = pool;
		description->Size = length;
		description->Type = D3DRTYPE_VERTEXBUFFER;
		description->Usage = usage;

		return D3D_OK;
	}

	int Direct3DVertexBuffer9::getLength() const
	{
		return length;
	}

	sw::Resource *Direct3DVertexBuffer9::getResource() const
	{
		return vertexBuffer;
	}
}
