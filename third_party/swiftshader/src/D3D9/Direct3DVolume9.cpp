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

#include "Direct3DVolume9.hpp"

#include "Direct3DDevice9.hpp"
#include "Direct3DResource9.hpp"
#include "Direct3DVolumeTexture9.hpp"
#include "Direct3DSurface9.hpp"
#include "Resource.hpp"
#include "Debug.hpp"

#include <assert.h>

namespace D3D9
{
	bool isLockable(D3DPOOL pool, unsigned long usage)
	{
		return (pool != D3DPOOL_DEFAULT) || (usage & D3DUSAGE_DYNAMIC);
	}

	Direct3DVolume9::Direct3DVolume9(Direct3DDevice9 *device, Direct3DVolumeTexture9 *container, int width, int height, int depth, D3DFORMAT format, D3DPOOL pool, unsigned long usage)
		: device(device), Surface(container->getResource(), width, height, depth, 0, 1, translateFormat(format), isLockable(pool, usage), false), container(container), width(width), height(height), depth(depth), format(format), pool(pool), lockable(isLockable(pool, usage)), usage(usage)
	{
		resource = new Direct3DResource9(device, D3DRTYPE_VOLUME, pool, memoryUsage(width, height, depth, format));
		resource->bind();
	}

	Direct3DVolume9::~Direct3DVolume9()
	{
		resource->unbind();
	}

	void *Direct3DVolume9::lockInternal(int x, int y, int z, sw::Lock lock, sw::Accessor client)
	{
		return Surface::lockInternal(x, y, z, lock, client);
	}

	void Direct3DVolume9::unlockInternal()
	{
		Surface::unlockInternal();
	}

	long __stdcall Direct3DVolume9::QueryInterface(const IID &iid, void **object)
	{
		CriticalSection cs(device);

		TRACE("");

		if(iid == IID_IDirect3DVolume9 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long __stdcall Direct3DVolume9::AddRef()
	{
		TRACE("");

		return container->AddRef();
	}

	unsigned long __stdcall Direct3DVolume9::Release()
	{
		TRACE("");

		return container->Release();
	}

	long Direct3DVolume9::FreePrivateData(const GUID &guid)
	{
		CriticalSection cs(device);

		TRACE("");

		return resource->FreePrivateData(guid);
	}

	long Direct3DVolume9::GetContainer(const IID &iid, void **container)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!container)
		{
			return INVALIDCALL();
		}

		long result = this->container->QueryInterface(iid, container);

		if(result == S_OK)
		{
			return D3D_OK;
		}

		return INVALIDCALL();
	}

	long Direct3DVolume9::GetDesc(D3DVOLUME_DESC *description)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!description)
		{
			return INVALIDCALL();
		}

		description->Format = format;
		description->Type = D3DRTYPE_VOLUME;
		description->Usage = usage;
		description->Pool = pool;
		description->Width = width;
		description->Height = height;
		description->Depth = depth;

		return D3D_OK;
	}

	long Direct3DVolume9::GetDevice(IDirect3DDevice9 **device)
	{
		CriticalSection cs(this->device);

		TRACE("");

		return resource->GetDevice(device);
	}

	long Direct3DVolume9::GetPrivateData(const GUID &guid, void *data, unsigned long *size)
	{
		CriticalSection cs(device);

		TRACE("");

		return resource->GetPrivateData(guid, data, size);
	}

	long Direct3DVolume9::LockBox(D3DLOCKED_BOX *lockedVolume, const D3DBOX *box, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!lockedVolume)
		{
			return INVALIDCALL();
		}

		lockedVolume->RowPitch = 0;
		lockedVolume->SlicePitch = 0;
		lockedVolume->pBits = 0;

		if(!lockable)
		{
			return INVALIDCALL();
		}

		lockedVolume->RowPitch = getExternalPitchB();
		lockedVolume->SlicePitch = getExternalSliceB();

		sw::Lock lock = sw::LOCK_READWRITE;

		if(flags & D3DLOCK_DISCARD)
		{
			lock = sw::LOCK_DISCARD;
		}

		if(flags & D3DLOCK_READONLY)
		{
			lock = sw::LOCK_READONLY;
		}

		if(box)
		{
			lockedVolume->pBits = lockExternal(box->Left, box->Top, box->Front, lock, sw::PUBLIC);
		}
		else
		{
			lockedVolume->pBits = lockExternal(0, 0, 0, lock, sw::PUBLIC);
		}

		return D3D_OK;
	}

	long Direct3DVolume9::SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("");

		return resource->SetPrivateData(guid, data, size, flags);
	}

	long Direct3DVolume9::UnlockBox()
	{
		CriticalSection cs(device);

		TRACE("");

		unlockExternal();

		return D3D_OK;
	}

	sw::Format Direct3DVolume9::translateFormat(D3DFORMAT format)
	{
		return Direct3DSurface9::translateFormat(format);
	}

	unsigned int Direct3DVolume9::memoryUsage(int width, int height, int depth, D3DFORMAT format)
	{
		return Surface::size(width, height, depth, 0, 1, translateFormat(format));
	}
}
