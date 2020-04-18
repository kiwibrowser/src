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

#include "Direct3DSurface8.hpp"

#include "Direct3DBaseTexture8.hpp"
#include "Debug.hpp"

#include <malloc.h>
#include <assert.h>

extern bool quadLayoutEnabled;

namespace D3D8
{
	static sw::Resource *getParentResource(Unknown *container)
	{
		Direct3DBaseTexture8 *baseTexture = dynamic_cast<Direct3DBaseTexture8*>(container);

		if(baseTexture)
		{
			return baseTexture->getResource();
		}

		return 0;
	}

	int sampleCount(D3DMULTISAMPLE_TYPE multiSample)
	{
		if(multiSample == D3DMULTISAMPLE_2_SAMPLES)
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

	Direct3DSurface8::Direct3DSurface8(Direct3DDevice8 *device, Unknown *container, int width, int height, D3DFORMAT format, D3DPOOL pool, D3DMULTISAMPLE_TYPE multiSample, bool lockable, unsigned long usage)
		: Surface(getParentResource(container), width, height, 1, 0, sampleCount(multiSample), translateFormat(format), lockable, (usage & D3DUSAGE_RENDERTARGET) == D3DUSAGE_RENDERTARGET || (usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL), device(device), container(container), width(width), height(height), format(format), pool(pool), multiSample(multiSample), lockable(lockable), usage(usage)
	{
		parentTexture = dynamic_cast<Direct3DBaseTexture8*>(container);

		resource = new Direct3DResource8(device, D3DRTYPE_SURFACE, memoryUsage(width, height, multiSample, format));
	}

	Direct3DSurface8::~Direct3DSurface8()
	{
		resource->Release();
	}

	void *Direct3DSurface8::lockInternal(int x, int y, int z, sw::Lock lock, sw::Accessor client)
	{
		return Surface::lockInternal(x, y, z, lock, client);
	}

	void Direct3DSurface8::unlockInternal()
	{
		Surface::unlockInternal();
	}

	long Direct3DSurface8::QueryInterface(const IID &iid, void **object)
	{
		TRACE("");

		if(iid == IID_IDirect3DSurface8 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DSurface8::AddRef()
	{
		TRACE("");

		if(parentTexture)
		{
			return parentTexture->AddRef();
		}

		return Unknown::AddRef();
	}

	unsigned long Direct3DSurface8::Release()
	{
		TRACE("");

		if(parentTexture)
		{
			return parentTexture->Release();
		}

		return Unknown::Release();
	}

	long Direct3DSurface8::FreePrivateData(const GUID &guid)
	{
		TRACE("");

		return resource->FreePrivateData(guid);
	}

	long Direct3DSurface8::GetPrivateData(const GUID &guid, void *data, unsigned long *size)
	{
		TRACE("");

		return resource->GetPrivateData(guid, data, size);
	}

	long Direct3DSurface8::SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags)
	{
		TRACE("");

		return resource->SetPrivateData(guid, data, size, flags);
	}

	long Direct3DSurface8::GetDevice(IDirect3DDevice8 **device)
	{
		TRACE("");

		return resource->GetDevice(device);
	}

	long Direct3DSurface8::LockRect(D3DLOCKED_RECT *lockedRect, const RECT *rect, unsigned long flags)
	{
		TRACE("");

		if(!lockedRect)
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

		unlockExternal();

		return D3D_OK;
	}

	long Direct3DSurface8::UnlockRect()
	{
		TRACE("");

		return D3D_OK;
	}

	long Direct3DSurface8::GetContainer(const IID &iid, void **container)
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

	long Direct3DSurface8::GetDesc(D3DSURFACE_DESC *desc)
	{
		TRACE("");

		if(!desc)
		{
			return INVALIDCALL();
		}

		desc->Format = format;
		desc->Pool = pool;
		desc->Type = D3DRTYPE_SURFACE;
		desc->Height = height;
		desc->Width = width;
		desc->Size = memoryUsage(width, height, multiSample, format);
		desc->MultiSampleType = multiSample;
		desc->Usage = usage;

		return D3D_OK;
	}

	sw::Format Direct3DSurface8::translateFormat(D3DFORMAT format)
	{
		switch(format)
		{
		case D3DFMT_DXT1:			return sw::FORMAT_DXT1;
		case D3DFMT_DXT2:			return sw::FORMAT_DXT3;
		case D3DFMT_DXT3:			return sw::FORMAT_DXT3;
		case D3DFMT_DXT4:			return sw::FORMAT_DXT5;
		case D3DFMT_DXT5:			return sw::FORMAT_DXT5;
		case D3DFMT_R3G3B2:			return sw::FORMAT_R3G3B2;
		case D3DFMT_A8R3G3B2:		return sw::FORMAT_A8R3G3B2;
		case D3DFMT_X4R4G4B4:		return sw::FORMAT_X4R4G4B4;
		case D3DFMT_A4R4G4B4:		return sw::FORMAT_A4R4G4B4;
		case D3DFMT_A8R8G8B8:		return sw::FORMAT_A8R8G8B8;
		case D3DFMT_G16R16:			return sw::FORMAT_G16R16;
		case D3DFMT_A2B10G10R10:	return sw::FORMAT_A2B10G10R10;
		case D3DFMT_P8:				return sw::FORMAT_P8;
		case D3DFMT_A8P8:			return sw::FORMAT_A8P8;
		case D3DFMT_A8:				return sw::FORMAT_A8;
		case D3DFMT_R5G6B5:			return sw::FORMAT_R5G6B5;
		case D3DFMT_X1R5G5B5:		return sw::FORMAT_X1R5G5B5;
		case D3DFMT_A1R5G5B5:		return sw::FORMAT_A1R5G5B5;
		case D3DFMT_R8G8B8:			return sw::FORMAT_R8G8B8;
		case D3DFMT_X8R8G8B8:		return sw::FORMAT_X8R8G8B8;
		case D3DFMT_V8U8:			return sw::FORMAT_V8U8;
		case D3DFMT_L6V5U5:			return sw::FORMAT_L6V5U5;
		case D3DFMT_Q8W8V8U8:		return sw::FORMAT_Q8W8V8U8;
		case D3DFMT_X8L8V8U8:		return sw::FORMAT_X8L8V8U8;
		case D3DFMT_A2W10V10U10:	return sw::FORMAT_A2W10V10U10;
		case D3DFMT_V16U16:			return sw::FORMAT_V16U16;
		case D3DFMT_L8:				return sw::FORMAT_L8;
		case D3DFMT_A4L4:			return sw::FORMAT_A4L4;
		case D3DFMT_A8L8:			return sw::FORMAT_A8L8;
		case D3DFMT_D16:			return sw::FORMAT_D16;
		case D3DFMT_D32:			return sw::FORMAT_D32;
		case D3DFMT_D24X8:			return sw::FORMAT_D24X8;
		case D3DFMT_D24S8:			return sw::FORMAT_D24S8;
		default:
			ASSERT(false);
		}

		return sw::FORMAT_NULL;
	}

	int Direct3DSurface8::bytes(D3DFORMAT format)
	{
		return Surface::bytes(translateFormat(format));
	}

	unsigned int Direct3DSurface8::memoryUsage(int width, int height, D3DMULTISAMPLE_TYPE multiSample, D3DFORMAT format)
	{
		return Surface::size(width, height, 1, 0, sampleCount(multiSample), translateFormat(format));
	}
}
