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

#include "Direct3DVolumeTexture9.hpp"

#include "Direct3DVolume9.hpp"
#include "Direct3DDevice9.hpp"
#include "Resource.hpp"
#include "Debug.hpp"

#include <assert.h>

namespace D3D9
{
	Direct3DVolumeTexture9::Direct3DVolumeTexture9(Direct3DDevice9 *device, unsigned int width, unsigned int height, unsigned int depth, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool) : Direct3DBaseTexture9(device, D3DRTYPE_VOLUMETEXTURE, format, pool, levels, usage), width(width), height(height), depth(depth)
	{
		if(levels == 0)
		{
			this->levels = sw::log2(sw::max((int)width, (int)height, (int)depth, 1)) + 1;
		}

		for(unsigned int level = 0; level < sw::MIPMAP_LEVELS; level++)
		{
			if(level < this->levels)
			{
				volumeLevel[level] = new Direct3DVolume9(device, this, width, height, depth, format, pool, usage);
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

	Direct3DVolumeTexture9::~Direct3DVolumeTexture9()
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

	long Direct3DVolumeTexture9::QueryInterface(const IID &iid, void **object)
	{
		CriticalSection cs(device);

		TRACE("");

		if(iid == IID_IDirect3DVolumeTexture9 ||
		   iid == IID_IDirect3DBaseTexture9 ||
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

	unsigned long Direct3DVolumeTexture9::AddRef()
	{
		TRACE("");

		return Direct3DBaseTexture9::AddRef();
	}

	unsigned long Direct3DVolumeTexture9::Release()
	{
		TRACE("");

		return Direct3DBaseTexture9::Release();
	}

	long Direct3DVolumeTexture9::FreePrivateData(const GUID &guid)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::FreePrivateData(guid);
	}

	long Direct3DVolumeTexture9::GetPrivateData(const GUID &guid, void *data, unsigned long *size)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetPrivateData(guid, data, size);
	}

	void Direct3DVolumeTexture9::PreLoad()
	{
		CriticalSection cs(device);

		TRACE("");

		Direct3DBaseTexture9::PreLoad();
	}

	long Direct3DVolumeTexture9::SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::SetPrivateData(guid, data, size, flags);
	}

	long Direct3DVolumeTexture9::GetDevice(IDirect3DDevice9 **device)
	{
		CriticalSection(this->device);

		TRACE("");

		return Direct3DBaseTexture9::GetDevice(device);
	}

	unsigned long Direct3DVolumeTexture9::SetPriority(unsigned long newPriority)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::SetPriority(newPriority);
	}

	unsigned long Direct3DVolumeTexture9::GetPriority()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetPriority();
	}

	D3DRESOURCETYPE Direct3DVolumeTexture9::GetType()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetType();
	}

	void Direct3DVolumeTexture9::GenerateMipSubLevels()
	{
		CriticalSection cs(device);

		TRACE("");

		if(!(usage & D3DUSAGE_AUTOGENMIPMAP) || !volumeLevel[0]->hasDirtyContents())
		{
			return;
		}

		resource->lock(sw::PUBLIC);

		for(unsigned int i = 0; i < levels - 1; i++)
		{
			Direct3DVolume9 *source = volumeLevel[i];
			Direct3DVolume9 *dest = volumeLevel[i + 1];

			source->lockInternal(0, 0, 0, sw::LOCK_READONLY, sw::PUBLIC);
			dest->lockInternal(0, 0, 0, sw::LOCK_DISCARD, sw::PUBLIC);

			int sWidth = source->getWidth();
			int sHeight = source->getHeight();
			int sDepth = source->getDepth();

			int dWidth = dest->getWidth();
			int dHeight = dest->getHeight();
			int dDepth = dest->getDepth();

			D3DTEXTUREFILTERTYPE filter = GetAutoGenFilterType();

			float w = (float)sWidth / (float)dWidth;
			float h = (float)sHeight / (float)dHeight;
			float d = (float)sDepth / (float)dDepth;

			float z = 0.5f * d;

			for(int k = 0; k < dDepth; k++)
			{
				float y = 0.5f * h;

				for(int j = 0; j < dHeight; j++)
				{
					float x = 0.5f * w;

					for(int i = 0; i < dWidth; i++)
					{
						dest->copyInternal(source, i, j, k, x, y, z, filter > D3DTEXF_POINT);

						x += w;
					}

					y += h;
				}

				z += d;
			}

			source->unlockInternal();
			dest->unlockInternal();
		}

		volumeLevel[0]->markContentsClean();

		resource->unlock();
	}

	D3DTEXTUREFILTERTYPE Direct3DVolumeTexture9::GetAutoGenFilterType()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetAutoGenFilterType();
	}

	unsigned long Direct3DVolumeTexture9::GetLevelCount()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetLevelCount();
	}

	unsigned long Direct3DVolumeTexture9::GetLOD()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetLOD();
	}

	long Direct3DVolumeTexture9::SetAutoGenFilterType(D3DTEXTUREFILTERTYPE filterType)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::SetAutoGenFilterType(filterType);
	}

	unsigned long Direct3DVolumeTexture9::SetLOD(unsigned long newLOD)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::SetLOD(newLOD);
	}

	long Direct3DVolumeTexture9::GetVolumeLevel(unsigned int level, IDirect3DVolume9 **volume)
	{
		CriticalSection cs(device);

		TRACE("");

		*volume = 0;

		if(level >= GetLevelCount() || !volumeLevel[level])
		{
			return INVALIDCALL();
		}

		volumeLevel[level]->AddRef();
		*volume = volumeLevel[level];

		return D3D_OK;
	}

	long Direct3DVolumeTexture9::LockBox(unsigned int level, D3DLOCKED_BOX *lockedVolume, const D3DBOX *box, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!lockedVolume || level >= GetLevelCount() || !volumeLevel[level])
		{
			return INVALIDCALL();
		}

		return volumeLevel[level]->LockBox(lockedVolume, box, flags);
	}

	long Direct3DVolumeTexture9::UnlockBox(unsigned int level)
	{
		CriticalSection cs(device);

		TRACE("");

		if(level >= GetLevelCount() || !volumeLevel[level])
		{
			return INVALIDCALL();
		}

		return volumeLevel[level]->UnlockBox();
	}

	long Direct3DVolumeTexture9::AddDirtyBox(const D3DBOX *dirtyBox)
	{
		CriticalSection cs(device);

		TRACE("");

	//	UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DVolumeTexture9::GetLevelDesc(unsigned int level, D3DVOLUME_DESC *description)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!description || level >= GetLevelCount() || !volumeLevel[level])
		{
			return INVALIDCALL();
		}

		return volumeLevel[level]->GetDesc(description);
	}

	Direct3DVolume9 *Direct3DVolumeTexture9::getInternalVolumeLevel(unsigned int level)
	{
		return volumeLevel[level];
	}
}
