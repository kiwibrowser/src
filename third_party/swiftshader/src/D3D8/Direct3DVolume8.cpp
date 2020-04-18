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

#include "Direct3DVolume8.hpp"

#include "Direct3DResource8.hpp"
#include "Direct3DVolumeTexture8.hpp"
#include "Direct3DSurface8.hpp"
#include "Debug.hpp"

#include <assert.h>

namespace D3D8
{
	Direct3DVolume8::Direct3DVolume8(Direct3DDevice8 *device, Direct3DVolumeTexture8 *container, int width, int height, int depth, D3DFORMAT format, D3DPOOL pool, bool lockable, unsigned long usage)
		: Surface(container->getResource(), width, height, depth, 0, 1, translateFormat(format), lockable, false), container(container), width(width), height(height), depth(depth), format(format), pool(pool), lockable(lockable), usage(usage)
	{
		resource = new Direct3DResource8(device, D3DRTYPE_VOLUME, memoryUsage(width, height, depth, format));
	}

	Direct3DVolume8::~Direct3DVolume8()
	{
		resource->Release();
	}

	void *Direct3DVolume8::lockInternal(int x, int y, int z, sw::Lock lock, sw::Accessor client)
	{
		return Surface::lockInternal(x, y, z, lock, client);
	}

	void Direct3DVolume8::unlockInternal()
	{
		Surface::unlockInternal();
	}

	long __stdcall Direct3DVolume8::QueryInterface(const IID &iid, void **object)
	{
		TRACE("");

		if(iid == IID_IDirect3DVolume8 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long __stdcall Direct3DVolume8::AddRef()
	{
		TRACE("");

		return container->AddRef();
	}

	unsigned long __stdcall Direct3DVolume8::Release()
	{
		TRACE("");

		return container->Release();
	}

	long Direct3DVolume8::FreePrivateData(const GUID &guid)
	{
		TRACE("");

		return resource->FreePrivateData(guid);
	}

	long Direct3DVolume8::GetContainer(const IID &iid, void **container)
	{
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

	long Direct3DVolume8::GetDesc(D3DVOLUME_DESC *description)
	{
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

	long Direct3DVolume8::GetDevice(IDirect3DDevice8 **device)
	{
		TRACE("");

		return resource->GetDevice(device);
	}

	long Direct3DVolume8::GetPrivateData(const GUID &guid, void *data, unsigned long *size)
	{
		TRACE("");

		return resource->GetPrivateData(guid, data, size);
	}

	long Direct3DVolume8::LockBox(D3DLOCKED_BOX *lockedVolume, const D3DBOX *box, unsigned long flags)
	{
		TRACE("");

		if(!lockedVolume)
		{
			return INVALIDCALL();
		}

		lockedVolume->RowPitch = pitchB(getWidth(), 0, getExternalFormat(), false);
		lockedVolume->SlicePitch = sliceB(getWidth(), getHeight(), 0, getExternalFormat(), false);

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

		unlockExternal();
		
		return D3D_OK;
	}

	long Direct3DVolume8::SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags)
	{
		TRACE("");
		
		return SetPrivateData(guid, data, size, flags);
	}

	long Direct3DVolume8::UnlockBox()
	{
		TRACE("");

		return D3D_OK;
	}

	sw::Format Direct3DVolume8::translateFormat(D3DFORMAT format)
	{
		return Direct3DSurface8::translateFormat(format);
	}

	unsigned int Direct3DVolume8::memoryUsage(int width, int height, int depth, D3DFORMAT format)
	{
		return Surface::size(width, height, depth, 0, 1, translateFormat(format));
	}
}
