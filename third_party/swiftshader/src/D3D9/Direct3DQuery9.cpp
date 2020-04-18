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

#include "Direct3DQuery9.hpp"

#include "Direct3DDevice9.hpp"
#include "Timer.hpp"
#include "Renderer.hpp"
#include "Debug.hpp"

#include <assert.h>

namespace D3D9
{
	Direct3DQuery9::Direct3DQuery9(Direct3DDevice9 *device, D3DQUERYTYPE type) : device(device), type(type)
	{
		if(type == D3DQUERYTYPE_OCCLUSION)
		{
			query = new sw::Query(sw::Query::FRAGMENTS_PASSED);
		}
		else
		{
			query = 0;
		}
	}

	Direct3DQuery9::~Direct3DQuery9()
	{
		if(query)
		{
			device->removeQuery(query);

			delete query;
		}
	}

	long Direct3DQuery9::QueryInterface(const IID &iid, void **object)
	{
		CriticalSection cs(device);

		TRACE("");

		if(iid == IID_IDirect3DQuery9 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DQuery9::AddRef()
	{
		TRACE("");

		return Unknown::AddRef();
	}

	unsigned long Direct3DQuery9::Release()
	{
		TRACE("");

		return Unknown::Release();
	}

	long __stdcall Direct3DQuery9::GetDevice(IDirect3DDevice9 **device)
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

	D3DQUERYTYPE Direct3DQuery9::GetType()
	{
		CriticalSection cs(device);

		return type;
	}

	unsigned long Direct3DQuery9::GetDataSize()
	{
		CriticalSection cs(device);

		TRACE("");

		switch(type)
		{
		case D3DQUERYTYPE_VCACHE:				return sizeof(D3DDEVINFO_VCACHE);
		case D3DQUERYTYPE_RESOURCEMANAGER:		return sizeof(D3DDEVINFO_RESOURCEMANAGER);
		case D3DQUERYTYPE_VERTEXSTATS:			return sizeof(D3DDEVINFO_D3DVERTEXSTATS);
		case D3DQUERYTYPE_EVENT:				return sizeof(BOOL);
		case D3DQUERYTYPE_OCCLUSION:			return sizeof(DWORD);
		case D3DQUERYTYPE_TIMESTAMP:			return sizeof(UINT64);
		case D3DQUERYTYPE_TIMESTAMPDISJOINT:	return sizeof(BOOL);
		case D3DQUERYTYPE_TIMESTAMPFREQ:		return sizeof(UINT64);
		case D3DQUERYTYPE_PIPELINETIMINGS:		return sizeof(D3DDEVINFO_D3D9PIPELINETIMINGS);
		case D3DQUERYTYPE_INTERFACETIMINGS:		return sizeof(D3DDEVINFO_D3D9INTERFACETIMINGS);
		case D3DQUERYTYPE_VERTEXTIMINGS:		return sizeof(D3DDEVINFO_D3D9STAGETIMINGS);
		case D3DQUERYTYPE_PIXELTIMINGS:			return sizeof(D3DDEVINFO_D3D9PIPELINETIMINGS);
		case D3DQUERYTYPE_BANDWIDTHTIMINGS:		return sizeof(D3DDEVINFO_D3D9BANDWIDTHTIMINGS);
		case D3DQUERYTYPE_CACHEUTILIZATION:		return sizeof(D3DDEVINFO_D3D9CACHEUTILIZATION);
		default:
			ASSERT(false);
		}

		return D3D_OK;
	}

	long Direct3DQuery9::Issue(unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("");

		if(flags != D3DISSUE_BEGIN && flags != D3DISSUE_END)
		{
			return INVALIDCALL();
		}

		switch(type)
		{
		case D3DQUERYTYPE_VCACHE:				if(flags != D3DISSUE_END) return INVALIDCALL(); break;
		case D3DQUERYTYPE_RESOURCEMANAGER:		if(flags != D3DISSUE_END) return INVALIDCALL(); break;
		case D3DQUERYTYPE_VERTEXSTATS:			if(flags != D3DISSUE_END) return INVALIDCALL(); break;
		case D3DQUERYTYPE_EVENT:
			if(flags == D3DISSUE_END)
			{
			//	device->renderer->synchronize();   // FIXME
			}
			else return INVALIDCALL();
			break;
		case D3DQUERYTYPE_OCCLUSION:
			if(flags == D3DISSUE_BEGIN)
			{
				query->begin();
				device->addQuery(query);
				device->setOcclusionEnabled(true);
			}
			else   // flags == D3DISSUE_END
			{
				query->end();
				device->removeQuery(query);
				device->setOcclusionEnabled(false);
			}
			break;
		case D3DQUERYTYPE_TIMESTAMP:
			if(flags == D3DISSUE_END)
			{
				timestamp = sw::Timer::counter();
			}
			else return INVALIDCALL();
			break;
		case D3DQUERYTYPE_TIMESTAMPDISJOINT:	if(flags != D3DISSUE_BEGIN && flags != D3DISSUE_END) return INVALIDCALL(); break;
		case D3DQUERYTYPE_TIMESTAMPFREQ:		if(flags != D3DISSUE_END) return INVALIDCALL(); break;
		case D3DQUERYTYPE_PIPELINETIMINGS:		if(flags != D3DISSUE_BEGIN && flags != D3DISSUE_END) return INVALIDCALL(); break;
		case D3DQUERYTYPE_INTERFACETIMINGS:		if(flags != D3DISSUE_BEGIN && flags != D3DISSUE_END) return INVALIDCALL(); break;
		case D3DQUERYTYPE_VERTEXTIMINGS:		if(flags != D3DISSUE_BEGIN && flags != D3DISSUE_END) return INVALIDCALL(); break;
		case D3DQUERYTYPE_PIXELTIMINGS:			if(flags != D3DISSUE_BEGIN && flags != D3DISSUE_END) return INVALIDCALL(); break;
		case D3DQUERYTYPE_BANDWIDTHTIMINGS:		if(flags != D3DISSUE_BEGIN && flags != D3DISSUE_END) return INVALIDCALL(); break;
		case D3DQUERYTYPE_CACHEUTILIZATION:		if(flags != D3DISSUE_BEGIN && flags != D3DISSUE_END) return INVALIDCALL(); break;
		default:
			ASSERT(false);
		}

		return D3D_OK;
	}

	long Direct3DQuery9::GetData(void *data, unsigned long size, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("void *data = %p, unsigned long size = %d, unsigned long flags = %d", data, size, flags);

		if(query && query->building)
		{
			return INVALIDCALL();
		}

		bool signaled = !query || query->reference == 0;

		if(size && signaled)
		{
			if(!data)
			{
				return INVALIDCALL();
			}

			// FIXME: Check size

			switch(type)
			{
			case D3DQUERYTYPE_VCACHE:
				{
					D3DDEVINFO_VCACHE vcache;

					vcache.Pattern = 'CACH';
					vcache.OptMethod = 1;   // Vertex-cache based optimization
					vcache.CacheSize = 16;
					vcache.MagicNumber = 8;

					*(D3DDEVINFO_VCACHE*)data = vcache;
				}
				break;
			case D3DQUERYTYPE_RESOURCEMANAGER:		UNIMPLEMENTED(); break;
			case D3DQUERYTYPE_VERTEXSTATS:			UNIMPLEMENTED(); break;
			case D3DQUERYTYPE_EVENT:				*(BOOL*)data = TRUE; break;                       // FIXME
			case D3DQUERYTYPE_OCCLUSION:
				*(DWORD*)data = query->data;
				break;
			case D3DQUERYTYPE_TIMESTAMP:			*(UINT64*)data = timestamp; break;                // FIXME: Verify behaviour
			case D3DQUERYTYPE_TIMESTAMPDISJOINT:	*(BOOL*)data = FALSE; break;                      // FIXME: Verify behaviour
			case D3DQUERYTYPE_TIMESTAMPFREQ:		*(UINT64*)data = sw::Timer::frequency(); break;   // FIXME: Verify behaviour
			case D3DQUERYTYPE_PIPELINETIMINGS:		UNIMPLEMENTED(); break;
			case D3DQUERYTYPE_INTERFACETIMINGS:		UNIMPLEMENTED(); break;
			case D3DQUERYTYPE_VERTEXTIMINGS:		UNIMPLEMENTED(); break;
			case D3DQUERYTYPE_PIXELTIMINGS:			UNIMPLEMENTED(); break;
			case D3DQUERYTYPE_BANDWIDTHTIMINGS:		UNIMPLEMENTED(); break;
			case D3DQUERYTYPE_CACHEUTILIZATION:		UNIMPLEMENTED(); break;
			default:
				ASSERT(false);
			}
		}

		return signaled ? S_OK : S_FALSE;
	}
}
