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

#include "Direct3DTexture9.hpp"

#include "Direct3DDevice9.hpp"
#include "Direct3DSurface9.hpp"
#include "Resource.hpp"
#include "Debug.hpp"

#include <assert.h>

namespace D3D9
{
	Direct3DTexture9::Direct3DTexture9(Direct3DDevice9 *device, unsigned int width, unsigned int height, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool) : Direct3DBaseTexture9(device, D3DRTYPE_TEXTURE, format, pool, levels, usage), width(width), height(height)
	{
		if(levels == 0)
		{
			this->levels = sw::log2(sw::max((int)width, (int)height, 1)) + 1;
		}

		for(unsigned int level = 0; level < sw::MIPMAP_LEVELS; level++)
		{
			if(level < this->levels)
			{
				surfaceLevel[level] = new Direct3DSurface9(device, this, width, height, format, pool, D3DMULTISAMPLE_NONE, 0, false, usage);
				surfaceLevel[level]->bind();
			}
			else
			{
				surfaceLevel[level] = 0;
			}

			width = sw::max(1, (int)width / 2);
			height = sw::max(1, (int)height / 2);
		}
	}

	Direct3DTexture9::~Direct3DTexture9()
	{
		for(int level = 0; level < sw::MIPMAP_LEVELS; level++)
		{
			if(surfaceLevel[level])
			{
				surfaceLevel[level]->unbind();
				surfaceLevel[level] = 0;
			}
		}
	}

	long Direct3DTexture9::QueryInterface(const IID &iid, void **object)
	{
		CriticalSection cs(device);

		TRACE("");

		if(iid == IID_IDirect3DTexture9 ||
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

	unsigned long Direct3DTexture9::AddRef()
	{
		TRACE("");

		return Direct3DBaseTexture9::AddRef();
	}

	unsigned long Direct3DTexture9::Release()
	{
		TRACE("");

		return Direct3DBaseTexture9::Release();
	}

	long Direct3DTexture9::FreePrivateData(const GUID &guid)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::FreePrivateData(guid);
	}

	long Direct3DTexture9::GetPrivateData(const GUID &guid, void *data, unsigned long *size)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetPrivateData(guid, data, size);
	}

	void Direct3DTexture9::PreLoad()
	{
		CriticalSection cs(device);

		TRACE("");

		Direct3DBaseTexture9::PreLoad();
	}

	long Direct3DTexture9::SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::SetPrivateData(guid, data, size, flags);
	}

	long Direct3DTexture9::GetDevice(IDirect3DDevice9 **device)
	{
		CriticalSection cs(this->device);

		TRACE("IDirect3DDevice9 **device = 0x%0.8p", device);

		return Direct3DBaseTexture9::GetDevice(device);
	}

	unsigned long Direct3DTexture9::SetPriority(unsigned long newPriority)
	{
		CriticalSection cs(device);

		TRACE("unsigned long newPriority = %d", newPriority);

		return Direct3DBaseTexture9::SetPriority(newPriority);
	}

	unsigned long Direct3DTexture9::GetPriority()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetPriority();
	}

	D3DRESOURCETYPE Direct3DTexture9::GetType()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetType();
	}

	void Direct3DTexture9::GenerateMipSubLevels()
	{
		CriticalSection cs(device);

		TRACE("");

		if(!(usage & D3DUSAGE_AUTOGENMIPMAP) || !surfaceLevel[0]->hasDirtyContents())
		{
			return;
		}

		resource->lock(sw::PUBLIC);

		for(unsigned int i = 0; i < levels - 1; i++)
		{
			device->stretchRect(surfaceLevel[i], 0, surfaceLevel[i + 1], 0, GetAutoGenFilterType());
		}

		surfaceLevel[0]->markContentsClean();

		resource->unlock();
	}

	D3DTEXTUREFILTERTYPE Direct3DTexture9::GetAutoGenFilterType()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetAutoGenFilterType();
	}

	unsigned long Direct3DTexture9::GetLevelCount()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetLevelCount();
	}

	unsigned long Direct3DTexture9::GetLOD()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetLOD();
	}

	long Direct3DTexture9::SetAutoGenFilterType(D3DTEXTUREFILTERTYPE filterType)
	{
		CriticalSection cs(device);

		TRACE("D3DTEXTUREFILTERTYPE filterType = %d", filterType);

		return Direct3DBaseTexture9::SetAutoGenFilterType(filterType);
	}

	unsigned long Direct3DTexture9::SetLOD(unsigned long newLOD)
	{
		CriticalSection cs(device);

		TRACE("unsigned long newLOD = %d", newLOD);

		return Direct3DBaseTexture9::SetLOD(newLOD);
	}

	long Direct3DTexture9::GetLevelDesc(unsigned int level, D3DSURFACE_DESC *description)
	{
		CriticalSection cs(device);

		TRACE("unsigned int level = %d, D3DSURFACE_DESC *description = 0x%0.8p", level, description);

		if(!description || level >= GetLevelCount() || !surfaceLevel[level])
		{
			return INVALIDCALL();
		}

		return surfaceLevel[level]->GetDesc(description);
	}

	long Direct3DTexture9::LockRect(unsigned int level, D3DLOCKED_RECT *lockedRect, const RECT *rect, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("unsigned int level = %d, D3DLOCKED_RECT *lockedRect = 0x%0.8p, const RECT *rect = 0x%0.8p, unsigned long flags = %d", level, lockedRect, rect, flags);

		if(!lockedRect || level >= GetLevelCount() || !surfaceLevel[level])
		{
			return INVALIDCALL();
		}

		return surfaceLevel[level]->LockRect(lockedRect, rect, flags);
	}

	long Direct3DTexture9::GetSurfaceLevel(unsigned int level, IDirect3DSurface9 **surface)
	{
		CriticalSection cs(device);

		TRACE("unsigned int level = %d, IDirect3DSurface9 **surface = 0x%0.8p", level, surface);

		*surface = 0;

		if(level >= GetLevelCount() || !surfaceLevel[level])
		{
			return INVALIDCALL();
		}

		surfaceLevel[level]->AddRef();
		*surface = surfaceLevel[level];

		return D3D_OK;
	}

	long Direct3DTexture9::UnlockRect(unsigned int level)
	{
		CriticalSection cs(device);

		TRACE("unsigned int level = %d", level);

		if(level >= GetLevelCount() || !surfaceLevel[level])
		{
			return INVALIDCALL();
		}

		return surfaceLevel[level]->UnlockRect();
	}

	long Direct3DTexture9::AddDirtyRect(const RECT *dirtyRect)
	{
		CriticalSection cs(device);

		TRACE("");

	//	UNIMPLEMENTED();

		return D3D_OK;
	}

	Direct3DSurface9 *Direct3DTexture9::getInternalSurfaceLevel(unsigned int level)
	{
		return surfaceLevel[level];
	}
}
