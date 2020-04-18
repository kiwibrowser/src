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

#include "Direct3DBaseTexture9.hpp"

#include "Direct3DDevice9.hpp"
#include "Resource.hpp"
#include "Debug.hpp"

#include <assert.h>

namespace D3D9
{
	Direct3DBaseTexture9::Direct3DBaseTexture9(Direct3DDevice9 *device, D3DRESOURCETYPE type, D3DFORMAT format, D3DPOOL pool, unsigned long levels, unsigned long usage) : Direct3DResource9(device, type, pool, 0), format(format), levels(levels), usage(usage)
	{
		filterType = D3DTEXF_LINEAR;
		LOD = 0;

		resource = new sw::Resource(0);
	}

	Direct3DBaseTexture9::~Direct3DBaseTexture9()
	{
		resource->destruct();
	}

	long Direct3DBaseTexture9::QueryInterface(const IID &iid, void **object)
	{
		CriticalSection cs(device);

		TRACE("");

		if(iid == IID_IDirect3DBaseTexture9 ||
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

	unsigned long Direct3DBaseTexture9::AddRef()
	{
		TRACE("");

		return Direct3DResource9::AddRef();
	}

	unsigned long Direct3DBaseTexture9::Release()
	{
		TRACE("");

		return Direct3DResource9::Release();
	}

	long Direct3DBaseTexture9::FreePrivateData(const GUID &guid)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::FreePrivateData(guid);
	}

	long Direct3DBaseTexture9::GetPrivateData(const GUID &guid, void *data, unsigned long *size)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::GetPrivateData(guid, data, size);
	}

	void Direct3DBaseTexture9::PreLoad()
	{
		CriticalSection cs(device);

		TRACE("");

		Direct3DResource9::PreLoad();
	}

	long Direct3DBaseTexture9::SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::SetPrivateData(guid, data, size, flags);
	}

	long Direct3DBaseTexture9::GetDevice(IDirect3DDevice9 **device)
	{
		CriticalSection cs(this->device);

		TRACE("");

		return Direct3DResource9::GetDevice(device);
	}

	unsigned long Direct3DBaseTexture9::SetPriority(unsigned long newPriority)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::SetPriority(newPriority);
	}

	unsigned long Direct3DBaseTexture9::GetPriority()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::GetPriority();
	}

	D3DRESOURCETYPE Direct3DBaseTexture9::GetType()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::GetType();
	}

	D3DTEXTUREFILTERTYPE Direct3DBaseTexture9::GetAutoGenFilterType()
	{
		CriticalSection cs(device);

		TRACE("");

		if(usage & D3DUSAGE_AUTOGENMIPMAP)
		{
			return filterType;
		}
		else
		{
			return D3DTEXF_NONE;
		}
	}

	unsigned long Direct3DBaseTexture9::GetLevelCount()
	{
		CriticalSection cs(device);

		TRACE("");

		if(usage & D3DUSAGE_AUTOGENMIPMAP)
		{
			return 1;
		}

		return levels;
	}

	unsigned long Direct3DBaseTexture9::GetLOD()
	{
		CriticalSection cs(device);

		TRACE("");

		if(pool & D3DPOOL_MANAGED)
		{
			return LOD;
		}
		else
		{
			return 0;
		}
	}

	long Direct3DBaseTexture9::SetAutoGenFilterType(D3DTEXTUREFILTERTYPE filterType)
	{
		CriticalSection cs(device);

		TRACE("");

		if(usage & D3DUSAGE_AUTOGENMIPMAP)
		{
			this->filterType = filterType;   // FIXME: Check if valid
			// FIXME: Dirty the mipmap chain

			return D3D_OK;
		}
		else
		{
			return D3DTEXF_NONE;
		}
	}

	unsigned long Direct3DBaseTexture9::SetLOD(unsigned long newLOD)
	{
		CriticalSection cs(device);

		TRACE("");

		unsigned long oldLOD = LOD;
		LOD = newLOD < levels ? newLOD : levels - 1;

		if(pool & D3DPOOL_MANAGED)
		{
			return oldLOD;
		}
		else
		{
			return 0;
		}
	}

	void Direct3DBaseTexture9::GenerateMipSubLevels()
	{
		CriticalSection cs(device);

		TRACE("");
	}

	sw::Resource *Direct3DBaseTexture9::getResource() const
	{
		return resource;
	}

	unsigned long Direct3DBaseTexture9::getInternalLevelCount() const
	{
		return levels;
	}

	unsigned long Direct3DBaseTexture9::getUsage() const
	{
		return usage;
	}

	D3DFORMAT Direct3DBaseTexture9::getFormat() const
	{
		return format;
	}
}
