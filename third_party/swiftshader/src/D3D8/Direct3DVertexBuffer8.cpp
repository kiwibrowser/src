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

#include "Direct3DVertexBuffer8.hpp"

#include "Direct3DDevice8.hpp"
#include "Resource.hpp"
#include "Debug.hpp"

#include <assert.h>

namespace D3D8
{
	Direct3DVertexBuffer8::Direct3DVertexBuffer8(Direct3DDevice8 *device, unsigned int length, unsigned long usage, long FVF, D3DPOOL pool) : Direct3DResource8(device, D3DRTYPE_VERTEXBUFFER, length), length(length), usage(usage), FVF(FVF), pool(pool)
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
			ASSERT(length % stride == 0);   // D3D vertex size calculated incorrectly   // FIXME
		}

		vertexBuffer = new sw::Resource(length + 192 + 1024);   // NOTE: Applications can 'overshoot' while writing vertices
	}

	Direct3DVertexBuffer8::~Direct3DVertexBuffer8()
	{
		vertexBuffer->destruct();
	}

	long Direct3DVertexBuffer8::QueryInterface(const IID &iid, void **object)
	{
		TRACE("");

		if(iid == IID_IDirect3DVertexBuffer8 ||
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

	unsigned long Direct3DVertexBuffer8::AddRef()
	{
		TRACE("");

		return Direct3DResource8::AddRef();
	}

	unsigned long Direct3DVertexBuffer8::Release()
	{
		TRACE("");

		return Direct3DResource8::Release();
	}

	long Direct3DVertexBuffer8::FreePrivateData(const GUID &guid)
	{
		TRACE("");

		return Direct3DResource8::FreePrivateData(guid);
	}

	long Direct3DVertexBuffer8::GetPrivateData(const GUID &guid, void *data, unsigned long *size)
	{
		TRACE("");

		return Direct3DResource8::GetPrivateData(guid, data, size);
	}

	void Direct3DVertexBuffer8::PreLoad()
	{
		TRACE("");

		Direct3DResource8::PreLoad();
	}

	long Direct3DVertexBuffer8::SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags)
	{
		TRACE("");

		return Direct3DResource8::SetPrivateData(guid, data, size, flags);
	}

	long Direct3DVertexBuffer8::GetDevice(IDirect3DDevice8 **device)
	{
		TRACE("");

		return Direct3DResource8::GetDevice(device);
	}

	unsigned long Direct3DVertexBuffer8::SetPriority(unsigned long newPriority)
	{
		TRACE("");

		return Direct3DResource8::SetPriority(newPriority);
	}

	unsigned long Direct3DVertexBuffer8::GetPriority()
	{
		TRACE("");

		return Direct3DResource8::GetPriority();
	}

	D3DRESOURCETYPE Direct3DVertexBuffer8::GetType()
	{
		TRACE("");

		return Direct3DResource8::GetType();
	}

	long Direct3DVertexBuffer8::Lock(unsigned int offset, unsigned int size, unsigned char **data, unsigned long flags)
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

		*data = (unsigned char*)vertexBuffer->lock(sw::PUBLIC) + offset;
		vertexBuffer->unlock();

		return D3D_OK;
	}

	long Direct3DVertexBuffer8::Unlock()
	{
		TRACE("");

		return D3D_OK;
	}

	long Direct3DVertexBuffer8::GetDesc(D3DVERTEXBUFFER_DESC *description)
	{
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

	int Direct3DVertexBuffer8::getLength() const
	{
		return length;
	}

	sw::Resource *Direct3DVertexBuffer8::getResource() const
	{
		return vertexBuffer;
	}
}
