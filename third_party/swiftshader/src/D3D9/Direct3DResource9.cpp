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

#include "Direct3DResource9.hpp"

#include "Direct3DDevice9.hpp"
#include "Debug.hpp"

namespace D3D9
{
	unsigned int Direct3DResource9::memoryUsage = 0;

	Direct3DResource9::PrivateData::PrivateData()
	{
		data = 0;
	}

	Direct3DResource9::PrivateData::PrivateData(const void *data, int size, bool managed)
	{
		this->size = size;
		this->managed = managed;

		this->data = (void*)new unsigned char[size];
		memcpy(this->data, data, size);

		if(managed)
		{
			((IUnknown*)data)->AddRef();
		}
	}

	Direct3DResource9::PrivateData &Direct3DResource9::PrivateData::operator=(const PrivateData &privateData)
	{
		size = privateData.size;
		managed = privateData.managed;

		if(data)
		{
			if(managed)
			{
				((IUnknown*)data)->Release();
			}

			delete[] data;
		}

		data = (void*)new unsigned char[size];
		memcpy(data, privateData.data, size);

		return *this;
	}

	Direct3DResource9::PrivateData::~PrivateData()
	{
		if(data && managed)
		{
			((IUnknown*)data)->Release();
		}

		delete[] data;
		data = 0;
	}

	Direct3DResource9::Direct3DResource9(Direct3DDevice9 *device, D3DRESOURCETYPE type, D3DPOOL pool, unsigned int size) : device(device), type(type), pool(pool), size(size)
	{
		priority = 0;

		if(pool == D3DPOOL_DEFAULT)
		{
			memoryUsage += size;
		}
	}

	Direct3DResource9::~Direct3DResource9()
	{
		if(pool == D3DPOOL_DEFAULT)
		{
			memoryUsage -= size;
		}
	}

	long Direct3DResource9::QueryInterface(const IID &iid, void **object)
	{
		CriticalSection cs(device);

		TRACE("");

		if(iid == IID_IDirect3DResource9 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DResource9::AddRef()
	{
		TRACE("");

		return Unknown::AddRef();
	}

	unsigned long Direct3DResource9::Release()
	{
		TRACE("");

		return Unknown::Release();
	}

	long Direct3DResource9::GetDevice(IDirect3DDevice9 **device)
	{
		CriticalSection cs(this->device);

		TRACE("");

		if(!device)
		{
			return INVALIDCALL();
		}

		this->device->AddRef();
		*device = this->device;

		return D3D_OK;
	}

	long Direct3DResource9::SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("");

		privateData[guid] = PrivateData(data, size, flags == D3DSPD_IUNKNOWN);

		return D3D_OK;
	}

	long Direct3DResource9::GetPrivateData(const GUID &guid, void *data, unsigned long *size)
	{
		CriticalSection cs(device);

		TRACE("");

		Iterator result = privateData.find(guid);

		if(result == privateData.end())
		{
			return NOTFOUND();
		}

		if(result->second.size > *size)
		{
			return MOREDATA();
		}

		memcpy(data, result->second.data, result->second.size);

		return D3D_OK;
	}

	long Direct3DResource9::FreePrivateData(const GUID &guid)
	{
		CriticalSection cs(device);

		TRACE("");

		Iterator result = privateData.find(guid);

		if(result == privateData.end())
		{
			return D3DERR_NOTFOUND;
		}

		privateData.erase(guid);

		return D3D_OK;
	}

	unsigned long Direct3DResource9::SetPriority(unsigned long newPriority)
	{
		CriticalSection cs(device);

		TRACE("");

		unsigned long oldPriority = priority;
		priority = newPriority;

		return oldPriority;
	}

	unsigned long Direct3DResource9::GetPriority()
	{
		CriticalSection cs(device);

		TRACE("");

		return priority;
	}

	void Direct3DResource9::PreLoad()
	{
		CriticalSection cs(device);

		TRACE("");
	}

	D3DRESOURCETYPE Direct3DResource9::GetType()
	{
		CriticalSection cs(device);

		TRACE("");

		return type;
	}

	unsigned int Direct3DResource9::getMemoryUsage()
	{
		return memoryUsage;
	}

	D3DPOOL Direct3DResource9::getPool() const
	{
		return pool;
	}
}