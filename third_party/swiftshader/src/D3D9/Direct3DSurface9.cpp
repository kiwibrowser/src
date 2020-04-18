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

#include "Direct3DSurface9.hpp"

#include "Direct3DDevice9.hpp"
#include "Direct3DBaseTexture9.hpp"
#include "Capabilities.hpp"
#include "Resource.hpp"
#include "Debug.hpp"

#include <malloc.h>
#include <assert.h>

extern bool quadLayoutEnabled;

namespace D3D9
{
	sw::Resource *getParentResource(Unknown *container)
	{
		Direct3DBaseTexture9 *baseTexture = dynamic_cast<Direct3DBaseTexture9*>(container);

		if(baseTexture)
		{
			return baseTexture->getResource();
		}

		return 0;
	}

	int sampleCount(D3DMULTISAMPLE_TYPE multiSample, unsigned int quality)
	{
		if(multiSample == D3DMULTISAMPLE_NONMASKABLE)
		{
			switch(quality)
			{
			case 0: return 2;
			case 1: return 4;
			case 2: return 8;
			case 3: return 16;
			}
		}
		else if(multiSample == D3DMULTISAMPLE_2_SAMPLES)
		{
			return 2;
		}
		else if(multiSample == D3DMULTISAMPLE_4_SAMPLES)
		{
			return 4;
		}
		else if(multiSample == D3DMULTISAMPLE_8_SAMPLES)
		{
			return 8;
		}
		else if(multiSample == D3DMULTISAMPLE_16_SAMPLES)
		{
			return 16;
		}

		return 1;
	}

	bool isLockable(D3DPOOL pool, unsigned long usage, bool lockableOverride)
	{
		return (pool != D3DPOOL_DEFAULT) || (usage & D3DUSAGE_DYNAMIC) || lockableOverride;
	}

	Direct3DSurface9::Direct3DSurface9(Direct3DDevice9 *device, Unknown *container, int width, int height, D3DFORMAT format, D3DPOOL pool, D3DMULTISAMPLE_TYPE multiSample, unsigned int quality, bool lockableOverride, unsigned long usage)
		: Direct3DResource9(device, D3DRTYPE_SURFACE, pool, memoryUsage(width, height, multiSample, quality, format)), Surface(getParentResource(container), width, height, 1, 0, sampleCount(multiSample, quality), translateFormat(format), isLockable(pool, usage, lockableOverride), (usage & D3DUSAGE_RENDERTARGET) || (usage & D3DUSAGE_DEPTHSTENCIL)), container(container), width(width), height(height), format(format), pool(pool), multiSample(multiSample), quality(quality), lockable(isLockable(pool, usage, lockableOverride)), usage(usage)
	{
		parentTexture = dynamic_cast<Direct3DBaseTexture9*>(container);
	}

	Direct3DSurface9::~Direct3DSurface9()
	{
	}

	void *Direct3DSurface9::lockInternal(int x, int y, int z, sw::Lock lock, sw::Accessor client)
	{
		return Surface::lockInternal(x, y, z, lock, client);
	}

	void Direct3DSurface9::unlockInternal()
	{
		Surface::unlockInternal();
	}

	long Direct3DSurface9::QueryInterface(const IID &iid, void **object)
	{
		CriticalSection cs(device);

		TRACE("");

		if(iid == IID_IDirect3DSurface9 ||
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

	unsigned long Direct3DSurface9::AddRef()
	{
		TRACE("");

		if(parentTexture)
		{
			return parentTexture->AddRef();
		}

		return Direct3DResource9::AddRef();
	}

	unsigned long Direct3DSurface9::Release()
	{
		TRACE("");

		if(parentTexture)
		{
			return parentTexture->Release();
		}

		return Direct3DResource9::Release();
	}

	long Direct3DSurface9::FreePrivateData(const GUID &guid)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::FreePrivateData(guid);
	}

	long Direct3DSurface9::GetPrivateData(const GUID &guid, void *data, unsigned long *size)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::GetPrivateData(guid, data, size);
	}

	void Direct3DSurface9::PreLoad()
	{
		CriticalSection cs(device);

		TRACE("");

		Direct3DResource9::PreLoad();
	}

	long Direct3DSurface9::SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::SetPrivateData(guid, data, size, flags);
	}

	long Direct3DSurface9::GetDevice(IDirect3DDevice9 **device)
	{
		CriticalSection cs(this->device);

		TRACE("");

		return Direct3DResource9::GetDevice(device);
	}

	unsigned long Direct3DSurface9::SetPriority(unsigned long newPriority)
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::SetPriority(newPriority);
	}

	unsigned long Direct3DSurface9::GetPriority()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::GetPriority();
	}

	D3DRESOURCETYPE Direct3DSurface9::GetType()
	{
		CriticalSection cs(device);

		TRACE("");

		return Direct3DResource9::GetType();
	}

	long Direct3DSurface9::GetDC(HDC *deviceContext)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!deviceContext)
		{
			return INVALIDCALL();
		}

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DSurface9::ReleaseDC(HDC deviceContext)
	{
		CriticalSection cs(device);

		TRACE("");

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DSurface9::LockRect(D3DLOCKED_RECT *lockedRect, const RECT *rect, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("D3DLOCKED_RECT *lockedRect = 0x%0.8p, const RECT *rect = 0x%0.8p, unsigned long flags = %d", lockedRect, rect, flags);

		if(!lockedRect)
		{
			return INVALIDCALL();
		}

		lockedRect->Pitch = 0;
		lockedRect->pBits = 0;

		if(!lockable)
		{
			return INVALIDCALL();
		}

		lockedRect->Pitch = getExternalPitchB();

		sw::Lock lock = sw::LOCK_READWRITE;

		if(flags & D3DLOCK_DISCARD)
		{
			lock = sw::LOCK_DISCARD;
		}

		if(flags & D3DLOCK_READONLY)
		{
			lock = sw::LOCK_READONLY;
		}

		if(rect)
		{
			lockedRect->pBits = lockExternal(rect->left, rect->top, 0, lock, sw::PUBLIC);
		}
		else
		{
			lockedRect->pBits = lockExternal(0, 0, 0, lock, sw::PUBLIC);
		}

		return D3D_OK;
	}

	long Direct3DSurface9::UnlockRect()
	{
		CriticalSection cs(device);

		TRACE("");

		unlockExternal();

		return D3D_OK;
	}

	long Direct3DSurface9::GetContainer(const IID &iid, void **container)
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

	long Direct3DSurface9::GetDesc(D3DSURFACE_DESC *description)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!description)
		{
			return INVALIDCALL();
		}

		description->Format = format;
		description->Pool = pool;
		description->Type = D3DRTYPE_SURFACE;
		description->Height = height;
		description->Width = width;
		description->MultiSampleType = multiSample;
		description->MultiSampleQuality = quality;
		description->Usage = usage;

		return D3D_OK;
	}

	sw::Format Direct3DSurface9::translateFormat(D3DFORMAT format)
	{
		switch(format)
		{
		case D3DFMT_NULL:			return sw::FORMAT_NULL;
		case D3DFMT_DXT1:			return sw::FORMAT_DXT1;
		case D3DFMT_DXT2:			return sw::FORMAT_DXT3;
		case D3DFMT_DXT3:			return sw::FORMAT_DXT3;
		case D3DFMT_DXT4:			return sw::FORMAT_DXT5;
		case D3DFMT_DXT5:			return sw::FORMAT_DXT5;
		case D3DFMT_ATI1:			return sw::FORMAT_ATI1;
		case D3DFMT_ATI2:			return sw::FORMAT_ATI2;
		case D3DFMT_R3G3B2:			return sw::FORMAT_R3G3B2;
		case D3DFMT_A8R3G3B2:		return sw::FORMAT_A8R3G3B2;
		case D3DFMT_X4R4G4B4:		return sw::FORMAT_X4R4G4B4;
		case D3DFMT_A4R4G4B4:		return sw::FORMAT_A4R4G4B4;
		case D3DFMT_A8R8G8B8:		return sw::FORMAT_A8R8G8B8;
		case D3DFMT_A8B8G8R8:		return sw::FORMAT_A8B8G8R8;
		case D3DFMT_G16R16:			return sw::FORMAT_G16R16;
		case D3DFMT_A2R10G10B10:	return sw::FORMAT_A2R10G10B10;
		case D3DFMT_A2B10G10R10:	return sw::FORMAT_A2B10G10R10;
		case D3DFMT_A16B16G16R16:	return sw::FORMAT_A16B16G16R16;
		case D3DFMT_P8:				return sw::FORMAT_P8;
		case D3DFMT_A8P8:			return sw::FORMAT_A8P8;
		case D3DFMT_A8:				return sw::FORMAT_A8;
		case D3DFMT_R5G6B5:			return sw::FORMAT_R5G6B5;
		case D3DFMT_X1R5G5B5:		return sw::FORMAT_X1R5G5B5;
		case D3DFMT_A1R5G5B5:		return sw::FORMAT_A1R5G5B5;
		case D3DFMT_R8G8B8:			return sw::FORMAT_R8G8B8;
		case D3DFMT_X8R8G8B8:		return sw::FORMAT_X8R8G8B8;
		case D3DFMT_X8B8G8R8:		return sw::FORMAT_X8B8G8R8;
		case D3DFMT_V8U8:			return sw::FORMAT_V8U8;
		case D3DFMT_L6V5U5:			return sw::FORMAT_L6V5U5;
		case D3DFMT_Q8W8V8U8:		return sw::FORMAT_Q8W8V8U8;
		case D3DFMT_X8L8V8U8:		return sw::FORMAT_X8L8V8U8;
		case D3DFMT_A2W10V10U10:	return sw::FORMAT_A2W10V10U10;
		case D3DFMT_V16U16:			return sw::FORMAT_V16U16;
		case D3DFMT_Q16W16V16U16:	return sw::FORMAT_Q16W16V16U16;
		case D3DFMT_L8:				return sw::FORMAT_L8;
		case D3DFMT_A4L4:			return sw::FORMAT_A4L4;
		case D3DFMT_L16:			return sw::FORMAT_L16;
		case D3DFMT_A8L8:			return sw::FORMAT_A8L8;
		case D3DFMT_R16F:			return sw::FORMAT_R16F;
		case D3DFMT_G16R16F:		return sw::FORMAT_G16R16F;
		case D3DFMT_A16B16G16R16F:	return sw::FORMAT_A16B16G16R16F;
		case D3DFMT_R32F:			return sw::FORMAT_R32F;
		case D3DFMT_G32R32F:		return sw::FORMAT_G32R32F;
		case D3DFMT_A32B32G32R32F:	return sw::FORMAT_A32B32G32R32F;
		case D3DFMT_D16:			return sw::FORMAT_D16;
		case D3DFMT_D32:			return sw::FORMAT_D32;
		case D3DFMT_D24X8:			return sw::FORMAT_D24X8;
		case D3DFMT_D24S8:			return sw::FORMAT_D24S8;
		case D3DFMT_D24FS8:			return sw::FORMAT_D24FS8;
		case D3DFMT_D32F_LOCKABLE:	return sw::FORMAT_D32F_LOCKABLE;
		case D3DFMT_DF24:			return sw::FORMAT_DF24S8;
		case D3DFMT_DF16:			return sw::FORMAT_DF16S8;
		case D3DFMT_INTZ:			return sw::FORMAT_INTZ;
		default:
			ASSERT(false);
		}

		return sw::FORMAT_NULL;
	}

	int Direct3DSurface9::bytes(D3DFORMAT format)
	{
		return Surface::bytes(translateFormat(format));
	}

	unsigned int Direct3DSurface9::memoryUsage(int width, int height, D3DMULTISAMPLE_TYPE multiSample, unsigned int quality, D3DFORMAT format)
	{
		return Surface::size(width, height, 1, 0, sampleCount(multiSample, quality), translateFormat(format));
	}
}
