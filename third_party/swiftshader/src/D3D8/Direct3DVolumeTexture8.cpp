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

#include "Direct3DVolumeTexture8.hpp"

#include "Direct3DVolume8.hpp"
#include "Resource.hpp"
#include "Debug.hpp"

#include <assert.h>

namespace D3D8
{
	Direct3DVolumeTexture8::Direct3DVolumeTexture8(Direct3DDevice8 *device, unsigned int width, unsigned int height, unsigned int depth, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool) : Direct3DBaseTexture8(device, D3DRTYPE_VOLUMETEXTURE, levels, usage), width(width), height(height), depth(depth), format(format), pool(pool)
	{
		if(levels == 0)
		{
			this->levels = sw::log2(sw::max((int)width, (int)height, (int)depth, 1)) + 1;
		}

		for(unsigned int level = 0; level < sw::MIPMAP_LEVELS; level++)
		{
			if(level < this->levels)
			{
				volumeLevel[level] = new Direct3DVolume8(device, this, width, height, depth, format, pool, true, usage);
				volumeLevel[level]->bind();
			}
			else
			{
				volumeLevel[level] = 0;
			}

			width = sw::max(1, (int)width / 2);
			height = sw::max(1, (int)height / 2);
			depth = sw::max(1, (int)depth / 2);
		}
	}

	Direct3DVolumeTexture8::~Direct3DVolumeTexture8()
	{
		for(int level = 0; level < sw::MIPMAP_LEVELS; level++)
		{
			if(volumeLevel[level])
			{
				volumeLevel[level]->unbind();
				volumeLevel[level] = 0;
			}
		}
	}

	long Direct3DVolumeTexture8::QueryInterface(const IID &iid, void **object)
	{
		TRACE("");

		if(iid == IID_IDirect3DVolumeTexture8 ||
		   iid == IID_IDirect3DBaseTexture8 ||
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

	unsigned long Direct3DVolumeTexture8::AddRef()
	{
		TRACE("");

		return Direct3DBaseTexture8::AddRef();
	}

	unsigned long Direct3DVolumeTexture8::Release()
	{
		TRACE("");

		return Direct3DBaseTexture8::Release();
	}

	long Direct3DVolumeTexture8::FreePrivateData(const GUID &guid)
	{
		TRACE("");

		return Direct3DBaseTexture8::FreePrivateData(guid);
	}

	long Direct3DVolumeTexture8::GetPrivateData(const GUID &guid, void *data, unsigned long *size)
	{
		TRACE("");

		return Direct3DBaseTexture8::GetPrivateData(guid, data, size);
	}

	void Direct3DVolumeTexture8::PreLoad()
	{
		TRACE("");

		Direct3DBaseTexture8::PreLoad();
	}

	long Direct3DVolumeTexture8::SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags)
	{
		TRACE("");

		return Direct3DBaseTexture8::SetPrivateData(guid, data, size, flags);
	}

	long Direct3DVolumeTexture8::GetDevice(IDirect3DDevice8 **device)
	{
		TRACE("");

		return Direct3DBaseTexture8::GetDevice(device);
	}

	unsigned long Direct3DVolumeTexture8::SetPriority(unsigned long newPriority)
	{
		TRACE("");

		return Direct3DBaseTexture8::SetPriority(newPriority);
	}

	unsigned long Direct3DVolumeTexture8::GetPriority()
	{
		TRACE("");

		return Direct3DBaseTexture8::GetPriority();
	}

	D3DRESOURCETYPE Direct3DVolumeTexture8::GetType()
	{
		TRACE("");

		return Direct3DBaseTexture8::GetType();
	}

	unsigned long Direct3DVolumeTexture8::GetLevelCount()
	{
		TRACE("");

		return Direct3DBaseTexture8::GetLevelCount();
	}

	unsigned long Direct3DVolumeTexture8::GetLOD()
	{
		TRACE("");

		return Direct3DBaseTexture8::GetLOD();
	}

	unsigned long Direct3DVolumeTexture8::SetLOD(unsigned long newLOD)
	{
		TRACE("");

		return Direct3DBaseTexture8::SetLOD(newLOD);
	}

	long Direct3DVolumeTexture8::GetVolumeLevel(unsigned int level, IDirect3DVolume8 **volume)
	{
		TRACE("");

		*volume = 0;   // FIXME: Verify

		if(level >= GetLevelCount() || !volumeLevel[level])
		{
			return INVALIDCALL();
		}

		volumeLevel[level]->AddRef();
		*volume = volumeLevel[level];

		return D3D_OK;
	}

	long Direct3DVolumeTexture8::LockBox(unsigned int level, D3DLOCKED_BOX *lockedVolume, const D3DBOX *box, unsigned long flags)
	{
		TRACE("");

		if(!lockedVolume || level >= GetLevelCount() || !volumeLevel[level])
		{
			return INVALIDCALL();
		}

		return volumeLevel[level]->LockBox(lockedVolume, box, flags);
	}

	long Direct3DVolumeTexture8::UnlockBox(unsigned int level)
	{
		TRACE("");

		if(level >= GetLevelCount() || !volumeLevel[level])
		{
			return INVALIDCALL();
		}

		return volumeLevel[level]->UnlockBox();
	}

	long Direct3DVolumeTexture8::AddDirtyBox(const D3DBOX *dirtyBox)
	{
		TRACE("");

		if(!dirtyBox)
		{
			return INVALIDCALL();
		}

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DVolumeTexture8::GetLevelDesc(unsigned int level, D3DVOLUME_DESC *description)
	{
		TRACE("");

		if(!description || level >= GetLevelCount() || !volumeLevel[level])
		{
			return INVALIDCALL();
		}

		volumeLevel[level]->GetDesc(description);

		return D3D_OK;
	}

	Direct3DVolume8 *Direct3DVolumeTexture8::getInternalVolumeLevel(unsigned int level)
	{
		return volumeLevel[level];
	}
}
