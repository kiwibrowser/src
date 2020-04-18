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

#include "Direct3DCubeTexture9.hpp"

#include "Direct3DDevice9.hpp"
#include "Direct3DSurface9.hpp"
#include "Resource.hpp"
#include "Debug.hpp"

#include <assert.h>

namespace D3D9
{
	Direct3DCubeTexture9::Direct3DCubeTexture9(Direct3DDevice9 *device, unsigned int edgeLength, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool) : Direct3DBaseTexture9(device, D3DRTYPE_CUBETEXTURE, format, pool, levels, usage), edgeLength(edgeLength)
	{
		if(levels == 0)
		{
			this->levels = sw::log2(sw::max((int)edgeLength, 1)) + 1;
		}

		for(unsigned int face = 0; face < 6; face++)
		{
			int width = edgeLength;
			int height = edgeLength;

			for(unsigned int level = 0; level < sw::MIPMAP_LEVELS; level++)
			{
				if(level < this->levels)
				{
					surfaceLevel[face][level] = new Direct3DSurface9(device, this, width, height, format, pool, D3DMULTISAMPLE_NONE, 0, false, usage);
					surfaceLevel[face][level]->bind();
				}
				else
				{
					surfaceLevel[face][level] = 0;
				}

				width = sw::max(1, width / 2);
				height = sw::max(1, height / 2);
			}
		}
	}

	Direct3DCubeTexture9::~Direct3DCubeTexture9()
	{
		for(unsigned int face = 0; face < 6; face++)
		{
			for(int level = 0; level < sw::MIPMAP_LEVELS; level++)
			{
				if(surfaceLevel[face][level])
				{
					surfaceLevel[face][level]->unbind();
					surfaceLevel[face][level] = 0;
				}
			}
		}
	}

	long Direct3DCubeTexture9::QueryInterface(const IID &iid, void **object)
	{
		CriticalSection cs(device);

		TRACE("");

		if(iid == IID_IDirect3DCubeTexture9 ||
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

	unsigned long Direct3DCubeTexture9::AddRef()
	{
		TRACE("");

		return Direct3DBaseTexture9::AddRef();
	}

	unsigned long Direct3DCubeTexture9::Release()
	{
		TRACE("");

		return Direct3DBaseTexture9::Release();
	}

	long Direct3DCubeTexture9::FreePrivateData(const GUID &guid)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::FreePrivateData(guid);
	}

	long Direct3DCubeTexture9::GetPrivateData(const GUID &guid, void *data, unsigned long *size)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetPrivateData(guid, data, size);
	}

	void Direct3DCubeTexture9::PreLoad()
	{
		CriticalSection cs(device);

		TRACE("");

		Direct3DBaseTexture9::PreLoad();
	}

	long Direct3DCubeTexture9::SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::SetPrivateData(guid, data, size, flags);
	}

	long Direct3DCubeTexture9::GetDevice(IDirect3DDevice9 **device)
	{
		CriticalSection cs(this->device);

		TRACE("");

		return Direct3DBaseTexture9::GetDevice(device);
	}

	unsigned long Direct3DCubeTexture9::SetPriority(unsigned long newPriority)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::SetPriority(newPriority);
	}

	unsigned long Direct3DCubeTexture9::GetPriority()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetPriority();
	}

	D3DRESOURCETYPE Direct3DCubeTexture9::GetType()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetType();
	}

	void Direct3DCubeTexture9::GenerateMipSubLevels()
	{
		CriticalSection cs(device);

		TRACE("");

		if(!(usage & D3DUSAGE_AUTOGENMIPMAP))
		{
			return;
		}

		resource->lock(sw::PUBLIC);

		for(unsigned int face = 0; face < 6; face++)
		{
			if(!surfaceLevel[face][0]->hasDirtyContents())
			{
				continue;
			}

			for(unsigned int i = 0; i < levels - 1; i++)
			{
				device->stretchRect(surfaceLevel[face][i], 0, surfaceLevel[face][i + 1], 0, GetAutoGenFilterType());
			}

			surfaceLevel[face][0]->markContentsClean();
		}

		resource->unlock();
	}

	D3DTEXTUREFILTERTYPE Direct3DCubeTexture9::GetAutoGenFilterType()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetAutoGenFilterType();
	}

	unsigned long Direct3DCubeTexture9::GetLevelCount()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetLevelCount();
	}

	unsigned long Direct3DCubeTexture9::GetLOD()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::GetLOD();
	}

	long Direct3DCubeTexture9::SetAutoGenFilterType(D3DTEXTUREFILTERTYPE filterType)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::SetAutoGenFilterType(filterType);
	}

	unsigned long Direct3DCubeTexture9::SetLOD(unsigned long newLOD)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DBaseTexture9::SetLOD(newLOD);
	}

	long Direct3DCubeTexture9::AddDirtyRect(D3DCUBEMAP_FACES face, const RECT *dirtyRect)
	{
		CriticalSection cs(device);

		TRACE("");

	//	UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DCubeTexture9::GetCubeMapSurface(D3DCUBEMAP_FACES face, unsigned int level, IDirect3DSurface9 **cubeMapSurface)
	{
		CriticalSection cs(device);

		TRACE("");

		*cubeMapSurface = 0;

		if(face >= 6 || level >= GetLevelCount() || !surfaceLevel[face][level])
		{
			return INVALIDCALL();
		}

		surfaceLevel[face][level]->AddRef();
		*cubeMapSurface = surfaceLevel[face][level];

		return D3D_OK;
	}

	long Direct3DCubeTexture9::GetLevelDesc(unsigned int level, D3DSURFACE_DESC *description)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!description || level >= GetLevelCount() || !surfaceLevel[0][level])
		{
			return INVALIDCALL();
		}

		return surfaceLevel[0][level]->GetDesc(description);
	}

	long Direct3DCubeTexture9::LockRect(D3DCUBEMAP_FACES face, unsigned int level, D3DLOCKED_RECT *lockedRect, const RECT *rect, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!lockedRect || face >= 6 || level >= GetLevelCount() || !surfaceLevel[face][level])
		{
			return INVALIDCALL();
		}

		return surfaceLevel[face][level]->LockRect(lockedRect, rect, flags);
	}

	long Direct3DCubeTexture9::UnlockRect(D3DCUBEMAP_FACES face, unsigned int level)
	{
		CriticalSection cs(device);

		TRACE("");

		if(face >= 6 || level >= GetLevelCount() || !surfaceLevel[face][level])
		{
			return INVALIDCALL();
		}

		return surfaceLevel[face][level]->UnlockRect();
	}

	Direct3DSurface9 *Direct3DCubeTexture9::getInternalCubeMapSurface(D3DCUBEMAP_FACES face, unsigned int level)
	{
		return surfaceLevel[face][level];
	}
}