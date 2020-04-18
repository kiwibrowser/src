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

#include "Direct3DTexture8.hpp"

#include "Direct3DSurface8.hpp"
#include "Resource.hpp"
#include "Debug.hpp"

#include <assert.h>

namespace D3D8
{
	Direct3DTexture8::Direct3DTexture8(Direct3DDevice8 *device, unsigned int width, unsigned int height, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool) : Direct3DBaseTexture8(device, D3DRTYPE_TEXTURE, levels, usage), width(width), height(height), format(format), pool(pool)
	{
		if(levels == 0)
		{
			this->levels = sw::log2(sw::max((int)width, (int)height, 1)) + 1;
		}

		for(unsigned int level = 0; level < sw::MIPMAP_LEVELS; level++)
		{
			if(level < this->levels)
			{
				surfaceLevel[level] = new Direct3DSurface8(device, this, width, height, format, pool, D3DMULTISAMPLE_NONE, true, usage);
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

	Direct3DTexture8::~Direct3DTexture8()
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

	long Direct3DTexture8::QueryInterface(const IID &iid, void **object)
	{
		TRACE("");

		if(iid == IID_IDirect3DTexture8 ||
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

	unsigned long Direct3DTexture8::AddRef()
	{
		TRACE("");

		return Direct3DBaseTexture8::AddRef();
	}

	unsigned long Direct3DTexture8::Release()
	{
		TRACE("");

		return Direct3DBaseTexture8::Release();
	}

	long Direct3DTexture8::FreePrivateData(const GUID &guid)
	{
		TRACE("");

		return Direct3DBaseTexture8::FreePrivateData(guid);
	}

	long Direct3DTexture8::GetPrivateData(const GUID &guid, void *data, unsigned long *size)
	{
		TRACE("");

		return Direct3DBaseTexture8::GetPrivateData(guid, data, size);
	}

	void Direct3DTexture8::PreLoad()
	{
		TRACE("");

		Direct3DBaseTexture8::PreLoad();
	}

	long Direct3DTexture8::SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags)
	{
		TRACE("");

		return Direct3DBaseTexture8::SetPrivateData(guid, data, size, flags);
	}

	long Direct3DTexture8::GetDevice(IDirect3DDevice8 **device)
	{
		TRACE("");

		return Direct3DBaseTexture8::GetDevice(device);
	}

	unsigned long Direct3DTexture8::SetPriority(unsigned long newPriority)
	{
		TRACE("");

		return Direct3DBaseTexture8::SetPriority(newPriority);
	}

	unsigned long Direct3DTexture8::GetPriority()
	{
		TRACE("");

		return Direct3DBaseTexture8::GetPriority();
	}

	D3DRESOURCETYPE Direct3DTexture8::GetType()
	{
		TRACE("");

		return Direct3DBaseTexture8::GetType();
	}

	unsigned long Direct3DTexture8::GetLevelCount()
	{
		TRACE("");

		return Direct3DBaseTexture8::GetLevelCount();
	}

	unsigned long Direct3DTexture8::GetLOD()
	{
		TRACE("");

		return Direct3DBaseTexture8::GetLOD();
	}

	unsigned long Direct3DTexture8::SetLOD(unsigned long newLOD)
	{
		TRACE("");

		return Direct3DBaseTexture8::SetLOD(newLOD);
	}

	long Direct3DTexture8::GetLevelDesc(unsigned int level, D3DSURFACE_DESC *description)
	{	
		TRACE("");

		if(level >= GetLevelCount() || !surfaceLevel[level])
		{
			return INVALIDCALL();
		}

		return surfaceLevel[level]->GetDesc(description);
	}

	long Direct3DTexture8::LockRect(unsigned int level, D3DLOCKED_RECT *lockedRect, const RECT *rect, unsigned long flags)
	{
		TRACE("");

		if(!lockedRect || level >= GetLevelCount() || !surfaceLevel[level])
		{
			return INVALIDCALL();
		}

		return surfaceLevel[level]->LockRect(lockedRect, rect, flags);
	}

	long Direct3DTexture8::GetSurfaceLevel(unsigned int level, IDirect3DSurface8 **surface)
	{
		TRACE("");

		*surface = 0;   // FIXME: Verify

		if(level >= GetLevelCount() || !surfaceLevel[level])
		{
			return INVALIDCALL();
		}

		surfaceLevel[level]->AddRef();
		*surface = surfaceLevel[level];

		return D3D_OK;
	}		

	long Direct3DTexture8::UnlockRect(unsigned int level)
	{
		TRACE("");

		if(level >= GetLevelCount() || !surfaceLevel[level])
		{
			return INVALIDCALL();
		}

		return surfaceLevel[level]->UnlockRect();
	}

	long Direct3DTexture8::AddDirtyRect(const RECT *dirtyRect)
	{
		TRACE("");

	//	UNIMPLEMENTED();

		return D3D_OK;
	}

	Direct3DSurface8 *Direct3DTexture8::getInternalSurfaceLevel(unsigned int level)
	{
		return surfaceLevel[level];
	}
}
