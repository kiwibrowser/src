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

#include "Direct3DDevice9.hpp"

#include "Direct3D9.hpp"
#include "Direct3DSurface9.hpp"
#include "Direct3DIndexBuffer9.hpp"
#include "Direct3DVertexBuffer9.hpp"
#include "Direct3DTexture9.hpp"
#include "Direct3DVolumeTexture9.hpp"
#include "Direct3DCubeTexture9.hpp"
#include "Direct3DVertexDeclaration9.hpp"
#include "Direct3DSwapChain9.hpp"
#include "Direct3DPixelShader9.hpp"
#include "Direct3DVertexShader9.hpp"
#include "Direct3DStateBlock9.hpp"
#include "Direct3DQuery9.hpp"
#include "Direct3DVolume9.hpp"

#include "Debug.hpp"
#include "Capabilities.hpp"
#include "Math.hpp"
#include "Renderer.hpp"
#include "Config.hpp"
#include "FrameBuffer.hpp"
#include "Clipper.hpp"
#include "Configurator.hpp"
#include "Timer.hpp"
#include "Resource.hpp"

#include <assert.h>

bool localShaderConstants = true;

namespace D3D9
{
	inline unsigned long FtoDW(float f)
	{
		return (unsigned long&)f;
	}

	Direct3DDevice9::Direct3DDevice9(const HINSTANCE instance, Direct3D9 *d3d9, unsigned int adapter, D3DDEVTYPE deviceType, HWND focusWindow, unsigned long behaviourFlags, D3DPRESENT_PARAMETERS *presentParameters) : instance(instance), adapter(adapter), d3d9(d3d9), deviceType(deviceType), focusWindow(focusWindow), behaviourFlags(behaviourFlags)
	{
		InitializeCriticalSection(&criticalSection);

		init = true;
		stateRecorder = 0;

		d3d9->AddRef();

		context = new sw::Context();
		renderer = new sw::Renderer(context, sw::Direct3D, false);

		swapChain = 0;
		depthStencil = 0;
		autoDepthStencil = 0;
		renderTarget[0] = 0;
		renderTarget[1] = 0;
		renderTarget[2] = 0;
		renderTarget[3] = 0;

		for(int i = 0; i < 16 + 4; i++)
		{
			texture[i] = 0;
		}

		cursor = 0;

		Reset(presentParameters);

		pixelShader = 0;
		vertexShader = 0;

		lightsDirty = true;
		pixelShaderDirty = true;
		pixelShaderConstantsBDirty = 0;
		pixelShaderConstantsFDirty = 0;
		pixelShaderConstantsIDirty = 0;
		vertexShaderDirty = true;
		vertexShaderConstantsBDirty = 0;
		vertexShaderConstantsFDirty = 0;
		vertexShaderConstantsIDirty = 0;

		for(int i = 0; i < MAX_VERTEX_INPUTS; i++)
		{
			dataStream[i] = 0;
			streamStride[i] = 0;
			streamOffset[i] = 0;

			streamSourceFreq[i] = 1;
		}

		indexData = 0;
		vertexDeclaration = 0;

		D3DMATERIAL9 material;

		material.Diffuse.r = 1.0f;
		material.Diffuse.g = 1.0f;
		material.Diffuse.b = 1.0f;
		material.Diffuse.a = 0.0f;
		material.Ambient.r = 0.0f;
		material.Ambient.g = 0.0f;
		material.Ambient.b = 0.0f;
		material.Ambient.a = 0.0f;
		material.Emissive.r = 0.0f;
		material.Emissive.g = 0.0f;
		material.Emissive.b = 0.0f;
		material.Emissive.a = 0.0f;
		material.Specular.r = 0.0f;
		material.Specular.g = 0.0f;
		material.Specular.b = 0.0f;
		material.Specular.a = 0.0f;
		material.Power = 0.0f;

		SetMaterial(&material);

		D3DMATRIX identity = {1, 0, 0, 0,
		                      0, 1, 0, 0,
		                      0, 0, 1, 0,
		                      0, 0, 0, 1};

		SetTransform(D3DTS_VIEW, &identity);
		SetTransform(D3DTS_PROJECTION, &identity);
		SetTransform(D3DTS_TEXTURE0, &identity);
		SetTransform(D3DTS_TEXTURE1, &identity);
		SetTransform(D3DTS_TEXTURE2, &identity);
		SetTransform(D3DTS_TEXTURE3, &identity);
		SetTransform(D3DTS_TEXTURE4, &identity);
		SetTransform(D3DTS_TEXTURE5, &identity);
		SetTransform(D3DTS_TEXTURE6, &identity);
		SetTransform(D3DTS_TEXTURE7, &identity);

		for(int i = 0; i < 12; i++)
		{
			SetTransform(D3DTS_WORLDMATRIX(i), &identity);
		}

		for(int i = 0; i < MAX_PIXEL_SHADER_CONST; i++)
		{
			float zero[4] = {0, 0, 0, 0};

			SetPixelShaderConstantF(i, zero, 1);
		}

		for(int i = 0; i < MAX_VERTEX_SHADER_CONST; i++)
		{
			float zero[4] = {0, 0, 0, 0};

			SetVertexShaderConstantF(i, zero, 1);
		}

		for(int i = 0; i < 16; i++)
		{
			int zero[4] = {0, 0, 0, 0};

			SetPixelShaderConstantI(i, zero, 1);
			SetVertexShaderConstantI(i, zero, 1);
			SetPixelShaderConstantB(i, &zero[0], 1);
			SetVertexShaderConstantB(i, &zero[0], 1);
		}

		init = false;

		if(!(behaviourFlags & D3DCREATE_FPU_PRESERVE))
		{
			configureFPU();
		}

		instancingEnabled = pixelShaderVersionX >= D3DPS_VERSION(3, 0);
	}

	Direct3DDevice9::~Direct3DDevice9()
	{
		delete renderer;
		renderer = 0;
		delete context;
		context = 0;

		d3d9->Release();
		d3d9 = 0;

		swapChain->unbind();
		swapChain = 0;

		if(depthStencil)
		{
			depthStencil->unbind();
			depthStencil = 0;
		}

		if(autoDepthStencil)
		{
			autoDepthStencil->unbind();
			autoDepthStencil = 0;
		}

		for(int index = 0; index < 4; index++)
		{
			if(renderTarget[index])
			{
				renderTarget[index]->unbind();
				renderTarget[index] = 0;
			}
		}

		if(vertexDeclaration)
		{
			vertexDeclaration->unbind();
			vertexDeclaration = 0;
		}

		for(int i = 0; i < 16 + 4; i++)
		{
			if(texture[i])
			{
				texture[i]->unbind();
				texture[i] = 0;
			}
		}

		for(int i = 0; i < MAX_VERTEX_INPUTS; i++)
		{
			if(dataStream[i])
			{
				dataStream[i]->unbind();
				dataStream[i] = 0;
			}
		}

		if(indexData)
		{
			indexData->unbind();
			indexData = 0;
		}

		if(pixelShader)
		{
			pixelShader->unbind();
			pixelShader = 0;
		}

		if(vertexShader)
		{
			vertexShader->unbind();
			vertexShader = 0;
		}

		if(stateRecorder)
		{
			stateRecorder->unbind();
			stateRecorder = 0;
		}

		palette.clear();

		delete cursor;

		DeleteCriticalSection(&criticalSection);
	}

	long Direct3DDevice9::QueryInterface(const IID &iid, void **object)
	{
		CriticalSection cs(this);

		TRACE("const IID &iid = 0x%0.8p, void **object = 0x%0.8p", iid, object);

		if(iid == IID_IDirect3DDevice9 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DDevice9::AddRef()
	{
		TRACE("void");

		return Unknown::AddRef();
	}

	unsigned long Direct3DDevice9::Release()
	{
		TRACE("void");

		return Unknown::Release();
	}

	long Direct3DDevice9::BeginScene()
	{
		CriticalSection cs(this);

		TRACE("void");

		return D3D_OK;
	}

	long Direct3DDevice9::BeginStateBlock()
	{
		CriticalSection cs(this);

		TRACE("void");

		if(stateRecorder)
		{
			return INVALIDCALL();
		}

		stateRecorder = new Direct3DStateBlock9(this, (D3DSTATEBLOCKTYPE)0);

		if(!stateRecorder)
		{
			return OUTOFMEMORY();
		}

		stateRecorder->bind();

		return D3D_OK;
	}

	long Direct3DDevice9::Clear(unsigned long count, const D3DRECT *rects, unsigned long flags, unsigned long color, float z, unsigned long stencil)
	{
		CriticalSection cs(this);

		TRACE("unsigned long count = %d, const D3DRECT *rects = 0x%0.8p, unsigned long flags = 0x%0.8X, unsigned long color = 0x%0.8X, float z = %f, unsigned long stencil = %d", count, rects, flags, color, z, stencil);

		if(!rects && count != 0)
		{
			return INVALIDCALL();
		}

		if(flags & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL) && !depthStencil)
		{
			return INVALIDCALL();
		}

		if(flags & D3DCLEAR_STENCIL)   // Check for stencil component
		{
			D3DSURFACE_DESC description;
			depthStencil->GetDesc(&description);

			switch(description.Format)
			{
			case D3DFMT_D15S1:
			case D3DFMT_D24S8:
			case D3DFMT_D24X8:
			case D3DFMT_D24X4S4:
			case D3DFMT_D24FS8:
			case D3DFMT_S8_LOCKABLE:   // FIXME: INVALIDCALL when trying to clear depth?
			case D3DFMT_DF24:
			case D3DFMT_DF16:
			case D3DFMT_INTZ:
				break;
			case D3DFMT_D16_LOCKABLE:
			case D3DFMT_D32:
			case D3DFMT_D16:
			case D3DFMT_D32F_LOCKABLE:
			case D3DFMT_D32_LOCKABLE:
				return INVALIDCALL();
			default:
				ASSERT(false);
			}
		}

		if(!rects)
		{
			count = 1;

			D3DRECT rect;
			rect.x1 = viewport.X;
			rect.x2 = viewport.X + viewport.Width;
			rect.y1 = viewport.Y;
			rect.y2 = viewport.Y + viewport.Height;

			rects = &rect;
		}

		for(unsigned int i = 0; i < count; i++)
		{
			sw::Rect clearRect(rects[i].x1, rects[i].y1, rects[i].x2, rects[i].y2);

			clearRect.clip(viewport.X, viewport.Y, viewport.X + viewport.Width, viewport.Y + viewport.Height);

			if(scissorEnable)
			{
				clearRect.clip(scissorRect.left, scissorRect.top, scissorRect.right, scissorRect.bottom);
			}

			if(flags & D3DCLEAR_TARGET)
			{
				for(int index = 0; index < 4; index++)
				{
					if(renderTarget[index])
					{
						D3DSURFACE_DESC description;
						renderTarget[index]->GetDesc(&description);

						float rgba[4];
						rgba[0] = (float)(color & 0x00FF0000) / 0x00FF0000;
						rgba[1] = (float)(color & 0x0000FF00) / 0x0000FF00;
						rgba[2] = (float)(color & 0x000000FF) / 0x000000FF;
						rgba[3] = (float)(color & 0xFF000000) / 0xFF000000;

						if(renderState[D3DRS_SRGBWRITEENABLE] != FALSE && index == 0 && Capabilities::isSRGBwritable(description.Format))
						{
							rgba[0] = sw::linearToSRGB(rgba[0]);
							rgba[1] = sw::linearToSRGB(rgba[1]);
							rgba[2] = sw::linearToSRGB(rgba[2]);
						}

						renderer->clear(rgba, sw::FORMAT_A32B32G32R32F, renderTarget[index], clearRect, 0xF);
					}
				}
			}

			if(flags & D3DCLEAR_ZBUFFER)
			{
				z = sw::clamp01(z);
				depthStencil->clearDepth(z, clearRect.x0, clearRect.y0, clearRect.width(), clearRect.height());
			}

			if(flags & D3DCLEAR_STENCIL)
			{
				depthStencil->clearStencil(stencil, 0xFF, clearRect.x0, clearRect.y0, clearRect.width(), clearRect.height());
			}
		}

		return D3D_OK;
	}

	long Direct3DDevice9::ColorFill(IDirect3DSurface9 *surface, const RECT *rect, D3DCOLOR color)
	{
		CriticalSection cs(this);

		TRACE("IDirect3DSurface9 *surface = 0x%0.8p, const RECT *rect = 0x%0.8p, D3DCOLOR color = 0x%0.8X", surface, rect, color);

		if(!surface)
		{
			return INVALIDCALL();
		}

		D3DSURFACE_DESC description;

		surface->GetDesc(&description);

		if(description.Pool != D3DPOOL_DEFAULT)
		{
			return INVALIDCALL();
		}

		if(!rect)
		{
			RECT lock;

			lock.left = 0;
			lock.top = 0;
			lock.right = description.Width;
			lock.bottom = description.Height;

			rect = &lock;
		}

		static_cast<Direct3DSurface9*>(surface)->fill(color, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top);

		return D3D_OK;
	}

	long Direct3DDevice9::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS *presentParameters, IDirect3DSwapChain9 **swapChain)
	{
		CriticalSection cs(this);

		TRACE("D3DPRESENT_PARAMETERS *presentParameters = 0x%0.8p, IDirect3DSwapChain9 **swapChain = 0x%0.8p", presentParameters, swapChain);

		if(!swapChain)
		{
			return INVALIDCALL();
		}

		*swapChain = 0;

		if(!presentParameters)
		{
			return INVALIDCALL();
		}

		if(presentParameters->BackBufferCount > 3)
		{
			return INVALIDCALL();   // Maximum of three back buffers
		}

		*swapChain = new Direct3DSwapChain9(this, presentParameters);

		if(!*swapChain)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *swapChain;
			*swapChain = 0;

			return OUTOFVIDEOMEMORY();
		}

		(*swapChain)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice9::CreateCubeTexture(unsigned int edgeLength, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DCubeTexture9 **cubeTexture, void **sharedHandle)
	{
		CriticalSection cs(this);

		TRACE("unsigned int edgeLength = %d, unsigned int levels = %d, unsigned long usage = %d, D3DFORMAT format = %d, D3DPOOL pool = %d, IDirect3DCubeTexture9 **cubeTexture = 0x%0.8p, void **sharedHandle = 0x%0.8p", edgeLength, levels, usage, format, pool, cubeTexture, sharedHandle);

		*cubeTexture = 0;

		if(edgeLength == 0 || (usage & D3DUSAGE_AUTOGENMIPMAP && levels > 1) || d3d9->CheckDeviceFormat(adapter, deviceType, D3DFMT_X8R8G8B8, usage, D3DRTYPE_CUBETEXTURE, format) != D3D_OK)
		{
			return INVALIDCALL();
		}

		*cubeTexture = new Direct3DCubeTexture9(this, edgeLength, levels, usage, format, pool);

		if(!*cubeTexture)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *cubeTexture;
			*cubeTexture = 0;

			return OUTOFVIDEOMEMORY();
		}

		(*cubeTexture)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice9::CreateDepthStencilSurface(unsigned int width, unsigned int height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multiSample, unsigned long multiSampleQuality, int discard, IDirect3DSurface9 **surface, void **sharedHandle)
	{
		CriticalSection cs(this);

		TRACE("unsigned int width = %d, unsigned int height = %d, D3DFORMAT format = %d, D3DMULTISAMPLE_TYPE multiSample = %d, unsigned long multiSampleQuality = %d, int discard = %d, IDirect3DSurface9 **surface = 0x%0.8p, void **sharedHandle = 0x%0.8p", width, height, format, multiSample, multiSampleQuality, discard, surface, sharedHandle);

		*surface = 0;

		if(width == 0 || height == 0 || d3d9->CheckDeviceFormat(adapter, deviceType, D3DFMT_X8R8G8B8, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, format) != D3D_OK || height > sw::OUTLINE_RESOLUTION)
		{
			return INVALIDCALL();
		}

		bool lockable = false;

		switch(format)
		{
		case D3DFMT_D15S1:
		case D3DFMT_D24S8:
		case D3DFMT_D24X8:
		case D3DFMT_D24X4S4:
		case D3DFMT_D24FS8:
		case D3DFMT_D32:
		case D3DFMT_D16:
		case D3DFMT_DF24:
		case D3DFMT_DF16:
		case D3DFMT_INTZ:
			lockable = false;
			break;
		case D3DFMT_S8_LOCKABLE:
		case D3DFMT_D16_LOCKABLE:
		case D3DFMT_D32F_LOCKABLE:
		case D3DFMT_D32_LOCKABLE:
			lockable = true;
			break;
		default:
			ASSERT(false);
		}

		*surface = new Direct3DSurface9(this, this, width, height, format, D3DPOOL_DEFAULT, multiSample, multiSampleQuality, lockable, D3DUSAGE_DEPTHSTENCIL);

		if(!*surface)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *surface;
			*surface = 0;

			return OUTOFVIDEOMEMORY();
		}

		(*surface)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice9::CreateIndexBuffer(unsigned int length, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DIndexBuffer9 **indexBuffer, void **sharedHandle)
	{
		CriticalSection cs(this);

		TRACE("unsigned int length = %d, unsigned long usage = %d, D3DFORMAT format = %d, D3DPOOL pool = %d, IDirect3DIndexBuffer9 **indexBuffer = 0x%0.8p, void **sharedHandle = 0x%0.8p", length, usage, format, pool, indexBuffer, sharedHandle);

		*indexBuffer = new Direct3DIndexBuffer9(this, length, usage, format, pool);

		if(!*indexBuffer)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *indexBuffer;
			*indexBuffer = 0;

			return OUTOFVIDEOMEMORY();
		}

		(*indexBuffer)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice9::CreateOffscreenPlainSurface(unsigned int width, unsigned int height, D3DFORMAT format, D3DPOOL pool, IDirect3DSurface9 **surface, void **sharedHandle)
	{
		CriticalSection cs(this);

		TRACE("unsigned int width = %d, unsigned int height = %d, D3DFORMAT format = %d, D3DPOOL pool = %d, IDirect3DSurface9 **surface = 0x%0.8p, void **sharedHandle = 0x%0.8p", width, height, format, pool, surface, sharedHandle);

		*surface = 0;

		if(width == 0 || height == 0 || d3d9->CheckDeviceFormat(adapter, deviceType, D3DFMT_X8R8G8B8, 0, D3DRTYPE_SURFACE, format) != D3D_OK)   // FIXME: Allow all formats supported by runtime/REF
		{
			return INVALIDCALL();
		}

		if(pool == D3DPOOL_MANAGED)
		{
			return INVALIDCALL();
		}

		*surface = new Direct3DSurface9(this, this, width, height, format, pool, D3DMULTISAMPLE_NONE, 0, true, 0);

		if(!*surface)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *surface;
			*surface = 0;

			return OUTOFVIDEOMEMORY();
		}

		(*surface)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice9::CreatePixelShader(const unsigned long *function, IDirect3DPixelShader9 **shader)
	{
		CriticalSection cs(this);

		TRACE("const unsigned long *function = 0x%0.8p, IDirect3DPixelShader9 **shader = 0x%0.8p", function, shader);

		if(!shader)
		{
			return INVALIDCALL();
		}

		*shader = 0;

		if(!sw::PixelShader::validate(function) || function[0] > pixelShaderVersionX)
		{
			return INVALIDCALL();   // Shader contains unsupported operations
		}

		*shader = new Direct3DPixelShader9(this, function);

		if(!*shader)
		{
			return OUTOFMEMORY();
		}

		(*shader)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice9::CreateQuery(D3DQUERYTYPE type, IDirect3DQuery9 **query)
	{
		CriticalSection cs(this);

		TRACE("D3DQUERYTYPE type = %d, IDirect3DQuery9 **query = 0x%0.8p", type, query);

		if(query == 0)   // Support checked
		{
			switch(type)
			{
			case D3DQUERYTYPE_VCACHE:				return D3D_OK;
			case D3DQUERYTYPE_RESOURCEMANAGER:		return NOTAVAILABLE();
			case D3DQUERYTYPE_VERTEXSTATS:			return NOTAVAILABLE();
			case D3DQUERYTYPE_EVENT:				return D3D_OK;
			case D3DQUERYTYPE_OCCLUSION:			return D3D_OK;
			case D3DQUERYTYPE_TIMESTAMP:			return D3D_OK;
			case D3DQUERYTYPE_TIMESTAMPDISJOINT:	return D3D_OK;
			case D3DQUERYTYPE_TIMESTAMPFREQ:		return D3D_OK;
			case D3DQUERYTYPE_PIPELINETIMINGS:		return NOTAVAILABLE();
			case D3DQUERYTYPE_INTERFACETIMINGS:		return NOTAVAILABLE();
			case D3DQUERYTYPE_VERTEXTIMINGS:		return NOTAVAILABLE();
			case D3DQUERYTYPE_PIXELTIMINGS:			return NOTAVAILABLE();
			case D3DQUERYTYPE_BANDWIDTHTIMINGS:		return NOTAVAILABLE();
			case D3DQUERYTYPE_CACHEUTILIZATION:		return NOTAVAILABLE();
			default:								ASSERT(false);   return NOTAVAILABLE();
			}
		}
		else
		{
			switch(type)
			{
			case D3DQUERYTYPE_VCACHE:				break;
			case D3DQUERYTYPE_RESOURCEMANAGER:		return NOTAVAILABLE();
			case D3DQUERYTYPE_VERTEXSTATS:			return NOTAVAILABLE();
			case D3DQUERYTYPE_EVENT:				break;
			case D3DQUERYTYPE_OCCLUSION:			break;
			case D3DQUERYTYPE_TIMESTAMP:			break;
			case D3DQUERYTYPE_TIMESTAMPDISJOINT:	break;
			case D3DQUERYTYPE_TIMESTAMPFREQ:		break;
			case D3DQUERYTYPE_PIPELINETIMINGS:		return NOTAVAILABLE();
			case D3DQUERYTYPE_INTERFACETIMINGS:		return NOTAVAILABLE();
			case D3DQUERYTYPE_VERTEXTIMINGS:		return NOTAVAILABLE();
			case D3DQUERYTYPE_PIXELTIMINGS:			return NOTAVAILABLE();
			case D3DQUERYTYPE_BANDWIDTHTIMINGS:		return NOTAVAILABLE();
			case D3DQUERYTYPE_CACHEUTILIZATION:		return NOTAVAILABLE();
			default:								ASSERT(false);   return NOTAVAILABLE();
			}

			*query = new Direct3DQuery9(this, type);

			if(!*query)
			{
				return OUTOFMEMORY();
			}

			(*query)->AddRef();

			return D3D_OK;
		}
	}

	long Direct3DDevice9::CreateRenderTarget(unsigned int width, unsigned int height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multiSample, unsigned long multiSampleQuality, int lockable, IDirect3DSurface9 **surface, void **sharedHandle)
	{
		CriticalSection cs(this);

		TRACE("unsigned int width = %d, unsigned int height = %d, D3DFORMAT format = %d, D3DMULTISAMPLE_TYPE multiSample = %d, unsigned long multiSampleQuality = %d, int lockable = %d, IDirect3DSurface9 **surface = 0x%0.8p, void **sharedHandle = 0x%0.8p", width, height, format, multiSample, multiSampleQuality, lockable, surface, sharedHandle);

		*surface = 0;

		if(width == 0 || height == 0 || d3d9->CheckDeviceFormat(adapter, deviceType, D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, format) != D3D_OK || height > sw::OUTLINE_RESOLUTION)
		{
			return INVALIDCALL();
		}

		*surface = new Direct3DSurface9(this, this, width, height, format, D3DPOOL_DEFAULT, multiSample, multiSampleQuality, lockable != FALSE, D3DUSAGE_RENDERTARGET);

		if(!*surface)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *surface;
			*surface = 0;

			return OUTOFVIDEOMEMORY();
		}

		(*surface)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice9::CreateStateBlock(D3DSTATEBLOCKTYPE type, IDirect3DStateBlock9 **stateBlock)
	{
		CriticalSection cs(this);

		TRACE("D3DSTATEBLOCKTYPE type = %d, IDirect3DStateBlock9 **stateBlock = 0x%0.8p", type, stateBlock);

		*stateBlock = new Direct3DStateBlock9(this, type);

		if(!*stateBlock)
		{
			return OUTOFMEMORY();
		}

		(*stateBlock)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice9::CreateTexture(unsigned int width, unsigned int height, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture9 **texture, void **sharedHandle)
	{
		CriticalSection cs(this);

		TRACE("unsigned int width = %d, unsigned int height = %d, unsigned int levels = %d, unsigned long usage = %d, D3DFORMAT format = %d, D3DPOOL pool = %d, IDirect3DTexture9 **texture = 0x%0.8p, void **sharedHandle = 0x%0.8p", width, height, levels, usage, format, pool, texture, sharedHandle);

		*texture = 0;

		if(width == 0 || height == 0 || (usage & D3DUSAGE_AUTOGENMIPMAP && levels > 1) || d3d9->CheckDeviceFormat(adapter, deviceType, D3DFMT_X8R8G8B8, usage, D3DRTYPE_TEXTURE, format) != D3D_OK)
		{
			return INVALIDCALL();
		}

		*texture = new Direct3DTexture9(this, width, height, levels, usage, format, pool);

		if(!*texture)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *texture;
			*texture = 0;

			return OUTOFVIDEOMEMORY();
		}

		(*texture)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice9::CreateVertexBuffer(unsigned int length, unsigned long usage, unsigned long FVF, D3DPOOL pool, IDirect3DVertexBuffer9 **vertexBuffer, void **sharedHandle)
	{
		CriticalSection cs(this);

		TRACE("unsigned int length = %d, unsigned long usage = %d, unsigned long FVF = 0x%0.8X, D3DPOOL pool = %d, IDirect3DVertexBuffer9 **vertexBuffer = 0x%0.8p, void **sharedHandle = 0x%0.8p", length, usage, FVF, pool, vertexBuffer, sharedHandle);

		*vertexBuffer = new Direct3DVertexBuffer9(this, length, usage, FVF, pool);

		if(!*vertexBuffer)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *vertexBuffer;
			*vertexBuffer = 0;

			return OUTOFVIDEOMEMORY();
		}

		(*vertexBuffer)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice9::CreateVertexDeclaration(const D3DVERTEXELEMENT9 *vertexElements, IDirect3DVertexDeclaration9 **declaration)
	{
		CriticalSection cs(this);

		TRACE("const D3DVERTEXELEMENT9 *vertexElements = 0x%0.8p, IDirect3DVertexDeclaration9 **declaration = 0x%0.8p", vertexElements, declaration);

		if(!declaration)
		{
			return INVALIDCALL();
		}

		const D3DVERTEXELEMENT9 *element = vertexElements;

		while(element->Stream != 0xFF)
		{
			if(element->Type > D3DDECLTYPE_UNUSED)   // FIXME: Check other fields too
			{
				return FAIL();
			}

			element++;
		}

		*declaration = new Direct3DVertexDeclaration9(this, vertexElements);

		if(!*declaration)
		{
			return OUTOFMEMORY();
		}

		(*declaration)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice9::CreateVertexShader(const unsigned long *function, IDirect3DVertexShader9 **shader)
	{
		CriticalSection cs(this);

		TRACE("const unsigned long *function = 0x%0.8p, IDirect3DVertexShader9 **shader = 0x%0.8p", function, shader);

		if(!shader)
		{
			return INVALIDCALL();
		}

		*shader = 0;

		if(!sw::VertexShader::validate(function) || function[0] > vertexShaderVersionX)
		{
			return INVALIDCALL();   // Shader contains unsupported operations
		}

		*shader = new Direct3DVertexShader9(this, function);

		if(!*shader)
		{
			return OUTOFMEMORY();
		}

		(*shader)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice9::CreateVolumeTexture(unsigned int width, unsigned int height, unsigned int depth, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DVolumeTexture9 **volumeTexture, void **sharedHandle)
	{
		CriticalSection cs(this);

		TRACE("unsigned int width = %d, unsigned int height = %d, unsigned int depth = %d, unsigned int levels = %d, unsigned long usage = %d, D3DFORMAT format = %d, D3DPOOL pool = %d, IDirect3DVolumeTexture9 **volumeTexture = 0x%0.8p, void **sharedHandle = 0x%0.8p", width, height, depth, levels, usage, format, pool, volumeTexture, sharedHandle);

		*volumeTexture = 0;

		if(width == 0 || height == 0 || depth == 0 || (usage & D3DUSAGE_AUTOGENMIPMAP && levels > 1) || d3d9->CheckDeviceFormat(adapter, deviceType, D3DFMT_X8R8G8B8, usage, D3DRTYPE_VOLUMETEXTURE, format) != D3D_OK)
		{
			return INVALIDCALL();
		}

		*volumeTexture = new Direct3DVolumeTexture9(this, width, height, depth, levels, usage, format, pool);

		if(!*volumeTexture)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *volumeTexture;
			*volumeTexture = 0;

			return OUTOFVIDEOMEMORY();
		}

		(*volumeTexture)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice9::DeletePatch(unsigned int handle)
	{
		CriticalSection cs(this);

		TRACE("unsigned int handle = %d", handle);

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DDevice9::DrawIndexedPrimitive(D3DPRIMITIVETYPE type, int baseVertexIndex, unsigned int minIndex, unsigned int numVertices, unsigned int startIndex, unsigned int primitiveCount)
	{
		CriticalSection cs(this);

		TRACE("D3DPRIMITIVETYPE type = %d, int baseVertexIndex = %d, unsigned int minIndex = %d, unsigned int numVertices = %d, unsigned int startIndex = %d, unsigned int primitiveCount = %d", type, baseVertexIndex, minIndex, numVertices, startIndex, primitiveCount);

		if(!indexData)
		{
			return INVALIDCALL();
		}

		if(!bindResources(indexData) || !primitiveCount)
		{
			return D3D_OK;
		}

		unsigned int indexOffset = startIndex * (indexData->is32Bit() ? 4 : 2);   // FIXME: Doesn't take stream frequencies into account

		sw::DrawType drawType;

		if(indexData->is32Bit())
		{
			switch(type)
			{
			case D3DPT_POINTLIST:     drawType = sw::DRAW_INDEXEDPOINTLIST32;     break;
			case D3DPT_LINELIST:      drawType = sw::DRAW_INDEXEDLINELIST32;      break;
			case D3DPT_LINESTRIP:     drawType = sw::DRAW_INDEXEDLINESTRIP32;     break;
			case D3DPT_TRIANGLELIST:  drawType = sw::DRAW_INDEXEDTRIANGLELIST32;  break;
			case D3DPT_TRIANGLESTRIP: drawType = sw::DRAW_INDEXEDTRIANGLESTRIP32; break;
			case D3DPT_TRIANGLEFAN:   drawType = sw::DRAW_INDEXEDTRIANGLEFAN32;   break;
			default:
				ASSERT(false);
			}
		}
		else
		{
			switch(type)
			{
			case D3DPT_POINTLIST:     drawType = sw::DRAW_INDEXEDPOINTLIST16;     break;
			case D3DPT_LINELIST:      drawType = sw::DRAW_INDEXEDLINELIST16;      break;
			case D3DPT_LINESTRIP:     drawType = sw::DRAW_INDEXEDLINESTRIP16;     break;
			case D3DPT_TRIANGLELIST:  drawType = sw::DRAW_INDEXEDTRIANGLELIST16;  break;
			case D3DPT_TRIANGLESTRIP: drawType = sw::DRAW_INDEXEDTRIANGLESTRIP16; break;
			case D3DPT_TRIANGLEFAN:   drawType = sw::DRAW_INDEXEDTRIANGLEFAN16;   break;
			default:
				ASSERT(false);
			}
		}

		if((streamSourceFreq[0] & D3DSTREAMSOURCE_INDEXEDDATA) && instanceData())
		{
			int instanceCount = (streamSourceFreq[0] & ~D3DSTREAMSOURCE_INDEXEDDATA);

			for(int instance = 0; instance < instanceCount; instance++)
			{
				bindVertexStreams(baseVertexIndex, true, instance);
				renderer->draw(drawType, indexOffset, primitiveCount, instance == 0);
			}
		}
		else
		{
			bindVertexStreams(baseVertexIndex, false, 0);
			renderer->draw(drawType, indexOffset, primitiveCount);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE type, unsigned int minIndex, unsigned int numVertices, unsigned int primitiveCount, const void *indexData, D3DFORMAT indexDataFormat, const void *vertexStreamZeroData, unsigned int vertexStreamZeroStride)
	{
		CriticalSection cs(this);

		TRACE("D3DPRIMITIVETYPE type = %d, unsigned int minIndex = %d, unsigned int numVertices = %d, unsigned int primitiveCount = %d, const void *indexData = 0x%0.8p, D3DFORMAT indexDataFormat = %d, const void *vertexStreamZeroData = 0x%0.8p, unsigned int vertexStreamZeroStride = %d", type, minIndex, numVertices, primitiveCount, indexData, indexDataFormat, vertexStreamZeroData, vertexStreamZeroStride);

		if(!vertexStreamZeroData || !indexData)
		{
			return INVALIDCALL();
		}

		int length = (minIndex + numVertices) * vertexStreamZeroStride;

		Direct3DVertexBuffer9 *vertexBuffer = new Direct3DVertexBuffer9(this, length, 0, 0, D3DPOOL_DEFAULT);

		void *data;
		vertexBuffer->Lock(0, 0, &data, 0);
		memcpy(data, vertexStreamZeroData, length);
		vertexBuffer->Unlock();

		SetStreamSource(0, vertexBuffer, 0, vertexStreamZeroStride);

		switch(type)
		{
		case D3DPT_POINTLIST:		length = primitiveCount;		break;
		case D3DPT_LINELIST:		length = primitiveCount * 2;	break;
		case D3DPT_LINESTRIP:		length = primitiveCount + 1;	break;
		case D3DPT_TRIANGLELIST:	length = primitiveCount * 3;	break;
		case D3DPT_TRIANGLESTRIP:	length = primitiveCount + 2;	break;
		case D3DPT_TRIANGLEFAN:		length = primitiveCount + 2;	break;
		default:
			ASSERT(false);
		}

		length *= indexDataFormat == D3DFMT_INDEX32 ? 4 : 2;

		Direct3DIndexBuffer9 *indexBuffer = new Direct3DIndexBuffer9(this, length, 0, indexDataFormat, D3DPOOL_DEFAULT);

		indexBuffer->Lock(0, 0, &data, 0);
		memcpy(data, indexData, length);
		indexBuffer->Unlock();

		SetIndices(indexBuffer);

		if(!bindResources(indexBuffer) || !primitiveCount)
		{
			vertexBuffer->Release();

			return D3D_OK;
		}

		sw::DrawType drawType;

		if(indexDataFormat == D3DFMT_INDEX32)
		{
			switch(type)
			{
			case D3DPT_POINTLIST:     drawType = sw::DRAW_INDEXEDPOINTLIST32;     break;
			case D3DPT_LINELIST:      drawType = sw::DRAW_INDEXEDLINELIST32;      break;
			case D3DPT_LINESTRIP:     drawType = sw::DRAW_INDEXEDLINESTRIP32;     break;
			case D3DPT_TRIANGLELIST:  drawType = sw::DRAW_INDEXEDTRIANGLELIST32;  break;
			case D3DPT_TRIANGLESTRIP: drawType = sw::DRAW_INDEXEDTRIANGLESTRIP32; break;
			case D3DPT_TRIANGLEFAN:   drawType = sw::DRAW_INDEXEDTRIANGLEFAN32;   break;
			default:
				ASSERT(false);
			}
		}
		else
		{
			switch(type)
			{
			case D3DPT_POINTLIST:     drawType = sw::DRAW_INDEXEDPOINTLIST16;     break;
			case D3DPT_LINELIST:      drawType = sw::DRAW_INDEXEDLINELIST16;      break;
			case D3DPT_LINESTRIP:     drawType = sw::DRAW_INDEXEDLINESTRIP16;     break;
			case D3DPT_TRIANGLELIST:  drawType = sw::DRAW_INDEXEDTRIANGLELIST16;  break;
			case D3DPT_TRIANGLESTRIP: drawType = sw::DRAW_INDEXEDTRIANGLESTRIP16; break;
			case D3DPT_TRIANGLEFAN:   drawType = sw::DRAW_INDEXEDTRIANGLEFAN16;   break;
			default:
				ASSERT(false);
			}
		}

		bindVertexStreams(0, false, 0);
		renderer->draw(drawType, 0, primitiveCount);

		SetStreamSource(0, 0, 0, 0);
		SetIndices(0);

		return D3D_OK;
	}

	long Direct3DDevice9::DrawPrimitive(D3DPRIMITIVETYPE primitiveType, unsigned int startVertex, unsigned int primitiveCount)
	{
		CriticalSection cs(this);

		TRACE("D3DPRIMITIVETYPE primitiveType = %d, unsigned int startVertex = %d, unsigned int primitiveCount = %d", primitiveType, startVertex, primitiveCount);

		if(!bindResources(0) || !primitiveCount)
		{
			return D3D_OK;
		}

		sw::DrawType drawType;

		switch(primitiveType)
		{
		case D3DPT_POINTLIST:     drawType = sw::DRAW_POINTLIST;     break;
		case D3DPT_LINELIST:      drawType = sw::DRAW_LINELIST;      break;
		case D3DPT_LINESTRIP:     drawType = sw::DRAW_LINESTRIP;     break;
		case D3DPT_TRIANGLELIST:  drawType = sw::DRAW_TRIANGLELIST;  break;
		case D3DPT_TRIANGLESTRIP: drawType = sw::DRAW_TRIANGLESTRIP; break;
		case D3DPT_TRIANGLEFAN:   drawType = sw::DRAW_TRIANGLEFAN;   break;
		default:
			ASSERT(false);
		}

		bindVertexStreams(startVertex, false, 0);
		renderer->draw(drawType, 0, primitiveCount);

		return D3D_OK;
	}

	long Direct3DDevice9::DrawPrimitiveUP(D3DPRIMITIVETYPE primitiveType, unsigned int primitiveCount, const void *vertexStreamZeroData, unsigned int vertexStreamZeroStride)
	{
		CriticalSection cs(this);

		TRACE("D3DPRIMITIVETYPE primitiveType = %d, unsigned int primitiveCount = %d, const void *vertexStreamZeroData = 0x%0.8p, unsigned int vertexStreamZeroStride = %d", primitiveType, primitiveCount, vertexStreamZeroData, vertexStreamZeroStride);

		if(!vertexStreamZeroData)
		{
			return INVALIDCALL();
		}

		IDirect3DVertexBuffer9 *vertexBuffer = 0;
		int length = 0;

		switch(primitiveType)
		{
		case D3DPT_POINTLIST:		length = primitiveCount;		break;
		case D3DPT_LINELIST:		length = primitiveCount * 2;	break;
		case D3DPT_LINESTRIP:		length = primitiveCount + 1;	break;
		case D3DPT_TRIANGLELIST:	length = primitiveCount * 3;	break;
		case D3DPT_TRIANGLESTRIP:	length = primitiveCount + 2;	break;
		case D3DPT_TRIANGLEFAN:		length = primitiveCount + 2;	break;
		default:
			ASSERT(false);
		}

		length *= vertexStreamZeroStride;

		CreateVertexBuffer(length, 0, 0, D3DPOOL_DEFAULT, &vertexBuffer, 0);

		void *data;
		vertexBuffer->Lock(0, 0, &data, 0);
		memcpy(data, vertexStreamZeroData, length);
		vertexBuffer->Unlock();

		SetStreamSource(0, vertexBuffer, 0, vertexStreamZeroStride);

		if(!bindResources(0) || !primitiveCount)
		{
			vertexBuffer->Release();

			return D3D_OK;
		}

		sw::DrawType drawType;

		switch(primitiveType)
		{
		case D3DPT_POINTLIST:     drawType = sw::DRAW_POINTLIST;     break;
		case D3DPT_LINELIST:      drawType = sw::DRAW_LINELIST;      break;
		case D3DPT_LINESTRIP:     drawType = sw::DRAW_LINESTRIP;     break;
		case D3DPT_TRIANGLELIST:  drawType = sw::DRAW_TRIANGLELIST;  break;
		case D3DPT_TRIANGLESTRIP: drawType = sw::DRAW_TRIANGLESTRIP; break;
		case D3DPT_TRIANGLEFAN:   drawType = sw::DRAW_TRIANGLEFAN;   break;
		default:
			ASSERT(false);
		}

		bindVertexStreams(0, false, 0);
		renderer->draw(drawType, 0, primitiveCount);

		SetStreamSource(0, 0, 0, 0);
		vertexBuffer->Release();

		return D3D_OK;
	}

	long Direct3DDevice9::DrawRectPatch(unsigned int handle, const float *numSegs, const D3DRECTPATCH_INFO *rectPatchInfo)
	{
		CriticalSection cs(this);

		TRACE("unsigned int handle = %d, const float *numSegs = 0x%0.8p, const D3DRECTPATCH_INFO *rectPatchInfo = 0x%0.8p", handle, numSegs, rectPatchInfo);

		if(!numSegs || !rectPatchInfo)
		{
			return INVALIDCALL();
		}

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DDevice9::DrawTriPatch(unsigned int handle, const float *numSegs, const D3DTRIPATCH_INFO *triPatchInfo)
	{
		CriticalSection cs(this);

		TRACE("unsigned int handle = %d, const float *numSegs = 0x%0.8p, const D3DTRIPATCH_INFO *triPatchInfo = 0x%0.8p", handle, numSegs, triPatchInfo);

		if(!numSegs || !triPatchInfo)
		{
			return INVALIDCALL();
		}

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DDevice9::EndScene()
	{
		CriticalSection cs(this);

		TRACE("void");

		return D3D_OK;
	}

	long Direct3DDevice9::EndStateBlock(IDirect3DStateBlock9 **stateBlock)
	{
		CriticalSection cs(this);

		TRACE("IDirect3DStateBlock9 **stateBlock = 0x%0.8p", stateBlock);

		if(!stateBlock)
		{
			return INVALIDCALL();
		}

		*stateBlock = 0;

		if(!stateRecorder)
		{
			return INVALIDCALL();
		}

		*stateBlock = stateRecorder;
		stateRecorder->AddRef();
		stateRecorder->unbind();
		stateRecorder = 0;   // Stop recording

		return D3D_OK;
	}

	long Direct3DDevice9::EvictManagedResources()
	{
		CriticalSection cs(this);

		TRACE("void");

		//	UNIMPLEMENTED();   // FIXME

		return D3D_OK;
	}

	unsigned int Direct3DDevice9::GetAvailableTextureMem()
	{
		CriticalSection cs(this);

		TRACE("void");

		int availableMemory = textureMemory - Direct3DResource9::getMemoryUsage();
		if(availableMemory < 0) availableMemory = 0;

		// Round to nearest MB
		return (availableMemory + 0x80000) & 0xFFF00000;
	}

	long Direct3DDevice9::GetBackBuffer(unsigned int swapChainIndex, unsigned int backBufferIndex, D3DBACKBUFFER_TYPE type, IDirect3DSurface9 **backBuffer)
	{
		CriticalSection cs(this);

		TRACE("unsigned int swapChainIndex = %d, unsigned int backBufferIndex = %d, D3DBACKBUFFER_TYPE type = %d, IDirect3DSurface9 **backBuffer = 0x%0.8p", swapChainIndex, backBufferIndex, type, backBuffer);

		if(swapChainIndex >= GetNumberOfSwapChains())
		{
			return INVALIDCALL();
		}

		return swapChain->GetBackBuffer(backBufferIndex, type, backBuffer);
	}

	long Direct3DDevice9::GetClipPlane(unsigned long index, float *plane)
	{
		CriticalSection cs(this);

		TRACE("unsigned long index = %d, float *plane = 0x%0.8p", index, plane);

		if(!plane || index >= 6)
		{
			return INVALIDCALL();
		}

		plane[0] = this->plane[index][0];
		plane[1] = this->plane[index][1];
		plane[2] = this->plane[index][2];
		plane[3] = this->plane[index][3];

		return D3D_OK;
	}

	long Direct3DDevice9::GetClipStatus(D3DCLIPSTATUS9 *clipStatus)
	{
		CriticalSection cs(this);

		TRACE("D3DCLIPSTATUS9 *clipStatus = 0x%0.8p", clipStatus);

		if(!clipStatus)
		{
			return INVALIDCALL();
		}

		*clipStatus = this->clipStatus;

		return D3D_OK;
	}

	long Direct3DDevice9::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *parameters)
	{
		CriticalSection cs(this);

		TRACE("D3DDEVICE_CREATION_PARAMETERS *parameters = 0x%0.8p", parameters);

		if(!parameters)
		{
			return INVALIDCALL();
		}

		parameters->AdapterOrdinal = adapter;
		parameters->BehaviorFlags = behaviourFlags;
		parameters->DeviceType = deviceType;
		parameters->hFocusWindow = focusWindow;

		return D3D_OK;
	}

	long Direct3DDevice9::GetCurrentTexturePalette(unsigned int *paletteNumber)
	{
		CriticalSection cs(this);

		TRACE("unsigned int *paletteNumber = 0x%0.8p", paletteNumber);

		if(!paletteNumber)
		{
			return INVALIDCALL();
		}

		*paletteNumber = currentPalette;

		return D3D_OK;
	}

	long Direct3DDevice9::GetDepthStencilSurface(IDirect3DSurface9 **depthStencilSurface)
	{
		CriticalSection cs(this);

		TRACE("IDirect3DSurface9 **depthStencilSurface = 0x%0.8p", depthStencilSurface);

		if(!depthStencilSurface)
		{
			return INVALIDCALL();
		}

		*depthStencilSurface = depthStencil;

		if(depthStencil)
		{
			depthStencil->AddRef();
		}
		else
		{
			return NOTFOUND();
		}

		return D3D_OK;
	}

	long Direct3DDevice9::GetDeviceCaps(D3DCAPS9 *caps)
	{
		CriticalSection cs(this);

		TRACE("D3DCAPS9 *caps = 0x%0.8p", caps);

		return d3d9->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, caps);
	}

	long Direct3DDevice9::GetDirect3D(IDirect3D9 **d3d9)
	{
		CriticalSection cs(this);

		TRACE("IDirect3D9 **d3d9 = 0x%0.8p", d3d9);

		if(!d3d9)
		{
			return INVALIDCALL();
		}

		*d3d9 = this->d3d9;
		this->d3d9->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice9::GetDisplayMode(unsigned int index, D3DDISPLAYMODE *mode)
	{
		CriticalSection cs(this);

		TRACE("unsigned int index = %d, D3DDISPLAYMODE *mode = 0x%0.8p", index, mode);

		if(index >= GetNumberOfSwapChains())
		{
			return INVALIDCALL();
		}

		return swapChain->GetDisplayMode(mode);
	}

	long Direct3DDevice9::GetFrontBufferData(unsigned int index, IDirect3DSurface9 *destSurface)
	{
		CriticalSection cs(this);

		TRACE("unsigned int index = %d, IDirect3DSurface9 *destSurface = %p", index, destSurface);

		if(index >= GetNumberOfSwapChains())
		{
			return INVALIDCALL();
		}

		return swapChain->GetFrontBufferData(destSurface);
	}

	long Direct3DDevice9::GetFVF(unsigned long *FVF)
	{
		CriticalSection cs(this);

		TRACE("unsigned long *FVF = 0x%0.8p", FVF);

		if(!FVF)
		{
			return INVALIDCALL();
		}

		if(vertexDeclaration)
		{
			*FVF = vertexDeclaration->getFVF();
		}
		else
		{
			*FVF = 0;
		}

		return D3D_OK;
	}

	void Direct3DDevice9::GetGammaRamp(unsigned int index, D3DGAMMARAMP *ramp)
	{
		CriticalSection cs(this);

		TRACE("unsigned int index = %d, D3DGAMMARAMP *ramp = 0x%0.8p", index, ramp);

		if(!ramp || index >= GetNumberOfSwapChains())
		{
			return;
		}

		swapChain->getGammaRamp((sw::GammaRamp*)ramp);
	}

	long Direct3DDevice9::GetIndices(IDirect3DIndexBuffer9 **indexData)
	{
		CriticalSection cs(this);

		TRACE("IDirect3DIndexBuffer9 **indexData = 0x%0.8p", indexData);

		if(!indexData)
		{
			return INVALIDCALL();
		}

		*indexData = this->indexData;

		if(this->indexData)
		{
			this->indexData->AddRef();
		}

		return D3D_OK;
	}

	long Direct3DDevice9::GetLight(unsigned long index, D3DLIGHT9 *light)
	{
		CriticalSection cs(this);

		TRACE("unsigned long index = %d, D3DLIGHT9 *light = 0x%0.8p", index, light);

		if(!light)
		{
			return INVALIDCALL();
		}

		if(!this->light.exists(index))
		{
			return INVALIDCALL();
		}

		*light = this->light[index];

		return D3D_OK;
	}

	long Direct3DDevice9::GetLightEnable(unsigned long index, int *enable)
	{
		CriticalSection cs(this);

		TRACE("unsigned long index = %d, int *enable = 0x%0.8p", index, enable);

		if(!enable)
		{
			return INVALIDCALL();
		}

		if(!light.exists(index))
		{
			return INVALIDCALL();
		}

		*enable = light[index].enable ? 128 : 0;

		return D3D_OK;
	}

	long Direct3DDevice9::GetMaterial(D3DMATERIAL9 *material)
	{
		CriticalSection cs(this);

		TRACE("D3DMATERIAL9 *material = 0x%0.8p", material);

		if(!material)
		{
			return INVALIDCALL();
		}

		*material = this->material;

		return D3D_OK;
	}

	float Direct3DDevice9::GetNPatchMode()
	{
		CriticalSection cs(this);

		TRACE("void");

		return 0.0f;   // FIXME: Unimplemented
	}

	unsigned int Direct3DDevice9::GetNumberOfSwapChains()
	{
		CriticalSection cs(this);

		TRACE("void");

		return 1;
	}

	long Direct3DDevice9::GetPaletteEntries(unsigned int paletteNumber, PALETTEENTRY *entries)
	{
		CriticalSection cs(this);

		TRACE("unsigned int paletteNumber = %d, PALETTEENTRY *entries = 0x%0.8p", paletteNumber, entries);

		if(paletteNumber > 0xFFFF || !entries)
		{
			return INVALIDCALL();
		}

		for(int i = 0; i < 256; i++)
		{
			entries[i] = palette[paletteNumber].entry[i];
		}

		return D3D_OK;
	}

	long Direct3DDevice9::GetPixelShader(IDirect3DPixelShader9 **shader)
	{
		CriticalSection cs(this);

		TRACE("IDirect3DPixelShader9 **shader = 0x%0.8p", shader);

		if(!shader)
		{
			return INVALIDCALL();
		}

		if(pixelShader)
		{
			pixelShader->AddRef();
		}

		*shader = pixelShader;

		return D3D_OK;
	}

	long Direct3DDevice9::GetPixelShaderConstantB(unsigned int startRegister, int *constantData, unsigned int count)
	{
		CriticalSection cs(this);

		TRACE("unsigned int startRegister = %d, int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		if(!constantData)
		{
			return INVALIDCALL();
		}

		for(unsigned int i = 0; i < count; i++)
		{
			constantData[i] = pixelShaderConstantB[startRegister + i];
		}

		return D3D_OK;
	}

	long Direct3DDevice9::GetPixelShaderConstantF(unsigned int startRegister, float *constantData, unsigned int count)
	{
		CriticalSection cs(this);

		TRACE("unsigned int startRegister = %d, int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		if(!constantData)
		{
			return INVALIDCALL();
		}

		for(unsigned int i = 0; i < count; i++)
		{
			constantData[i * 4 + 0] = pixelShaderConstantF[startRegister + i][0];
			constantData[i * 4 + 1] = pixelShaderConstantF[startRegister + i][1];
			constantData[i * 4 + 2] = pixelShaderConstantF[startRegister + i][2];
			constantData[i * 4 + 3] = pixelShaderConstantF[startRegister + i][3];
		}

		return D3D_OK;
	}

	long Direct3DDevice9::GetPixelShaderConstantI(unsigned int startRegister, int *constantData, unsigned int count)
	{
		CriticalSection cs(this);

		TRACE("unsigned int startRegister = %d, int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		if(!constantData)
		{
			return INVALIDCALL();
		}

		for(unsigned int i = 0; i < count; i++)
		{
			constantData[i * 4 + 0] = pixelShaderConstantI[startRegister + i][0];
			constantData[i * 4 + 1] = pixelShaderConstantI[startRegister + i][1];
			constantData[i * 4 + 2] = pixelShaderConstantI[startRegister + i][2];
			constantData[i * 4 + 3] = pixelShaderConstantI[startRegister + i][3];
		}

		return D3D_OK;
	}

	long Direct3DDevice9::GetRasterStatus(unsigned int index, D3DRASTER_STATUS *rasterStatus)
	{
		CriticalSection cs(this);

		TRACE("unsigned int swapChain = %d, D3DRASTER_STATUS *rasterStatus = 0x%0.8p", index, rasterStatus);

		if(index >= GetNumberOfSwapChains())
		{
			return INVALIDCALL();
		}

		return swapChain->GetRasterStatus(rasterStatus);
	}

	long Direct3DDevice9::GetRenderState(D3DRENDERSTATETYPE state, unsigned long *value)
	{
		CriticalSection cs(this);

		TRACE("D3DRENDERSTATETYPE state = %d, unsigned long *value = 0x%0.8p", state, value);

		if(!value)
		{
			return INVALIDCALL();
		}

		*value = renderState[state];

		return D3D_OK;
	}

	long Direct3DDevice9::GetRenderTarget(unsigned long index, IDirect3DSurface9 **renderTarget)
	{
		CriticalSection cs(this);

		TRACE("unsigned long index = %d, IDirect3DSurface9 **renderTarget = 0x%0.8p", index, renderTarget);

		if(index >= 4 || !renderTarget)
		{
			return INVALIDCALL();
		}

		*renderTarget = 0;

		if(!this->renderTarget[index])
		{
			return NOTFOUND();
		}

		*renderTarget = this->renderTarget[index];
		this->renderTarget[index]->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice9::GetRenderTargetData(IDirect3DSurface9 *renderTarget, IDirect3DSurface9 *destSurface)
	{
		CriticalSection cs(this);

		TRACE("IDirect3DSurface9 *renderTarget = 0x%0.8p, IDirect3DSurface9 *destSurface = 0x%0.8p", renderTarget, destSurface);

		if(!renderTarget || !destSurface)
		{
			return INVALIDCALL();
		}

		D3DSURFACE_DESC sourceDescription;
		D3DSURFACE_DESC destinationDescription;

		renderTarget->GetDesc(&sourceDescription);
		destSurface->GetDesc(&destinationDescription);

		if(sourceDescription.Width  != destinationDescription.Width ||
		   sourceDescription.Height != destinationDescription.Height ||
		   sourceDescription.Format != destinationDescription.Format ||
		   sourceDescription.MultiSampleType != D3DMULTISAMPLE_NONE)
		{
			return INVALIDCALL();
		}

		if(sourceDescription.Format == D3DFMT_A8R8G8B8 ||
		   sourceDescription.Format == D3DFMT_X8R8G8B8)
		{
			sw::Surface *source = static_cast<Direct3DSurface9*>(renderTarget);
			sw::Surface *dest = static_cast<Direct3DSurface9*>(destSurface);

			void *sourceBuffer = source->lockExternal(0, 0, 0, sw::LOCK_READONLY, sw::PUBLIC);
			void *destBuffer = dest->lockExternal(0, 0, 0, sw::LOCK_WRITEONLY, sw::PUBLIC);

			static void (__cdecl *blitFunction)(void *dst, void *src);
			static sw::Routine *blitRoutine;
			static sw::BlitState blitState = {};

			sw::BlitState update;
			update.width = sourceDescription.Width;
			update.height = sourceDescription.Height;
			update.sourceFormat = sw::FORMAT_A8R8G8B8;
			update.sourceStride = source->getExternalPitchB();
			update.destFormat = sw::FORMAT_A8R8G8B8;
			update.destStride = dest->getExternalPitchB();
			update.cursorHeight = 0;
			update.cursorWidth = 0;

			if(memcmp(&blitState, &update, sizeof(sw::BlitState)) != 0)
			{
				blitState = update;
				delete blitRoutine;

				blitRoutine = sw::FrameBuffer::copyRoutine(blitState);
				blitFunction = (void(__cdecl*)(void*, void*))blitRoutine->getEntry();
			}

			blitFunction(destBuffer, sourceBuffer);

			dest->unlockExternal();
			source->unlockExternal();
		}
		else
		{
			return UpdateSurface(renderTarget, 0, destSurface, 0);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::GetSamplerState(unsigned long sampler, D3DSAMPLERSTATETYPE state, unsigned long *value)
	{
		CriticalSection cs(this);

		TRACE("unsigned long sampler = %d, D3DSAMPLERSTATETYPE type = %d, unsigned long *value = 0x%0.8p", sampler, state, value);

		if(!value || state < D3DSAMP_ADDRESSU || state > D3DSAMP_DMAPOFFSET)   // FIXME: Set *value to 0?
		{
			return INVALIDCALL();
		}

		if((sampler >= 16 && sampler <= D3DDMAPSAMPLER) || sampler > D3DVERTEXTEXTURESAMPLER3)
		{
			return INVALIDCALL();
		}

		if(sampler >= D3DVERTEXTEXTURESAMPLER0)
		{
			sampler = 16 + (sampler - D3DVERTEXTEXTURESAMPLER0);
		}

		*value = samplerState[sampler][state];

		return D3D_OK;
	}

	long Direct3DDevice9::GetScissorRect(RECT *rect)
	{
		CriticalSection cs(this);

		TRACE("RECT *rect = 0x%0.8p", rect);

		if(!rect)
		{
			return INVALIDCALL();
		}

		*rect = scissorRect;

		return D3D_OK;
	}

	int Direct3DDevice9::GetSoftwareVertexProcessing()
	{
		CriticalSection cs(this);

		TRACE("void");

		return softwareVertexProcessing ? TRUE : FALSE;
	}

	long Direct3DDevice9::GetStreamSource(unsigned int streamNumber, IDirect3DVertexBuffer9 **streamData, unsigned int *offset, unsigned int *stride)
	{
		CriticalSection cs(this);

		TRACE("unsigned int streamNumber = %d, IDirect3DVertexBuffer9 **streamData = 0x%0.8p, unsigned int *offset = 0x%0.8p, unsigned int *stride = 0x%0.8p", streamNumber, streamData, offset, stride);

		if(streamNumber >= 16 || !streamData || !offset || !stride)
		{
			return INVALIDCALL();
		}

		*streamData = dataStream[streamNumber];

		if(dataStream[streamNumber])
		{
			dataStream[streamNumber]->AddRef();
		}

		*offset = streamOffset[streamNumber];
		*stride = streamStride[streamNumber];

		return D3D_OK;
	}

	long Direct3DDevice9::GetStreamSourceFreq(unsigned int streamNumber, unsigned int *divider)
	{
		CriticalSection cs(this);

		TRACE("unsigned int streamNumber = %d, unsigned int *divider = 0x%0.8p", streamNumber, divider);

		if(streamNumber >= 16 || !divider)
		{
			return INVALIDCALL();
		}

		*divider = streamSourceFreq[streamNumber];

		return D3D_OK;
	}

	long Direct3DDevice9::GetSwapChain(unsigned int index, IDirect3DSwapChain9 **swapChain)
	{
		CriticalSection cs(this);

		TRACE("unsigned int index = %d, IDirect3DSwapChain9 **swapChain = 0x%0.8p", index, swapChain);

		if(!swapChain || index >= GetNumberOfSwapChains())
		{
			return INVALIDCALL();
		}

		*swapChain = this->swapChain;

		if(*swapChain)
		{
			(*swapChain)->AddRef();
		}

		return D3D_OK;
	}

	long Direct3DDevice9::GetTexture(unsigned long sampler, IDirect3DBaseTexture9 **texture)
	{
		CriticalSection cs(this);

		TRACE("unsigned long sampler = %d, IDirect3DBaseTexture9 **texture = 0x%0.8p", sampler, texture);

		if(!texture)
		{
			return INVALIDCALL();
		}

		*texture = 0;

		if((sampler >= 16 && sampler <= D3DDMAPSAMPLER) || sampler > D3DVERTEXTEXTURESAMPLER3)
		{
			return INVALIDCALL();
		}

		*texture = this->texture[sampler];

		if(this->texture[sampler])
		{
			this->texture[sampler]->AddRef();
		}

		return D3D_OK;
	}

	long Direct3DDevice9::GetTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type, unsigned long *value)
	{
		CriticalSection cs(this);

		TRACE("unsigned long stage = %d, D3DTEXTURESTAGESTATETYPE type = %d, unsigned long *value = 0x%0.8p", stage, type, value);

		if(!value)
		{
			return INVALIDCALL();
		}

		*value = textureStageState[stage][type];

		return D3D_OK;
	}

	long Direct3DDevice9::GetTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
	{
		CriticalSection cs(this);

		TRACE("D3DTRANSFORMSTATETYPE state = %d, D3DMATRIX *matrix = 0x%0.8p", state, matrix);

		if(!matrix || state < 0 || state > 511)
		{
			return INVALIDCALL();
		}

		*matrix = this->matrix[state];

		return D3D_OK;
	}

	long Direct3DDevice9::GetVertexDeclaration(IDirect3DVertexDeclaration9 **declaration)
	{
		CriticalSection cs(this);

		TRACE("IDirect3DVertexDeclaration9 **declaration = 0x%0.8p", declaration);

		if(!declaration)
		{
			return INVALIDCALL();
		}

		*declaration = vertexDeclaration;

		if(vertexDeclaration)
		{
			vertexDeclaration->AddRef();
		}

		return D3D_OK;
	}

	long Direct3DDevice9::GetVertexShader(IDirect3DVertexShader9 **shader)
	{
		CriticalSection cs(this);

		TRACE("IDirect3DVertexShader9 **shader = 0x%0.8p", shader);

		if(!shader)
		{
			return INVALIDCALL();
		}

		*shader = vertexShader;

		if(vertexShader)
		{
			vertexShader->AddRef();
		}

		return D3D_OK;
	}

	long Direct3DDevice9::GetVertexShaderConstantB(unsigned int startRegister, int *constantData, unsigned int count)
	{
		CriticalSection cs(this);

		TRACE("unsigned int startRegister = %d, int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		if(!constantData)
		{
			return INVALIDCALL();
		}

		for(unsigned int i = 0; i < count; i++)
		{
			constantData[i] = vertexShaderConstantB[startRegister + i];
		}

		return D3D_OK;
	}

	long Direct3DDevice9::GetVertexShaderConstantF(unsigned int startRegister, float *constantData, unsigned int count)
	{
		CriticalSection cs(this);

		TRACE("unsigned int startRegister = %d, int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		if(!constantData)
		{
			return INVALIDCALL();
		}

		for(unsigned int i = 0; i < count; i++)
		{
			constantData[i * 4 + 0] = vertexShaderConstantF[startRegister + i][0];
			constantData[i * 4 + 1] = vertexShaderConstantF[startRegister + i][1];
			constantData[i * 4 + 2] = vertexShaderConstantF[startRegister + i][2];
			constantData[i * 4 + 3] = vertexShaderConstantF[startRegister + i][3];
		}

		return D3D_OK;
	}

	long Direct3DDevice9::GetVertexShaderConstantI(unsigned int startRegister, int *constantData, unsigned int count)
	{
		CriticalSection cs(this);

		TRACE("unsigned int startRegister = %d, int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		if(!constantData)
		{
			return INVALIDCALL();
		}

		for(unsigned int i = 0; i < count; i++)
		{
			constantData[i * 4 + 0] = vertexShaderConstantI[startRegister + i][0];
			constantData[i * 4 + 1] = vertexShaderConstantI[startRegister + i][1];
			constantData[i * 4 + 2] = vertexShaderConstantI[startRegister + i][2];
			constantData[i * 4 + 3] = vertexShaderConstantI[startRegister + i][3];
		}

		return D3D_OK;
	}

	long Direct3DDevice9::GetViewport(D3DVIEWPORT9 *viewport)
	{
		CriticalSection cs(this);

		TRACE("D3DVIEWPORT9 *viewport = 0x%0.8p", viewport);

		if(!viewport)
		{
			return INVALIDCALL();
		}

		*viewport = this->viewport;

		return D3D_OK;
	}

	long Direct3DDevice9::LightEnable(unsigned long index, int enable)
	{
		CriticalSection cs(this);

		TRACE("unsigned long index = %d, int enable = %d", index, enable);

		if(!light.exists(index))   // Insert default light
		{
			D3DLIGHT9 light;

			light.Type = D3DLIGHT_DIRECTIONAL;
			light.Diffuse.r = 1;
			light.Diffuse.g = 1;
			light.Diffuse.b = 1;
			light.Diffuse.a = 0;
			light.Specular.r = 0;
			light.Specular.g = 0;
			light.Specular.b = 0;
			light.Specular.a = 0;
			light.Ambient.r = 0;
			light.Ambient.g = 0;
			light.Ambient.b = 0;
			light.Ambient.a = 0;
			light.Position.x = 0;
			light.Position.y = 0;
			light.Position.z = 0;
			light.Direction.x = 0;
			light.Direction.y = 0;
			light.Direction.z = 1;
			light.Range = 0;
			light.Falloff = 0;
			light.Attenuation0 = 0;
			light.Attenuation1 = 0;
			light.Attenuation2 = 0;
			light.Theta = 0;
			light.Phi = 0;

			this->light[index] = light;
			this->light[index].enable = false;
		}

		if(!stateRecorder)
		{
			light[index].enable = (enable != FALSE);

			lightsDirty = true;
		}
		else
		{
			stateRecorder->lightEnable(index, enable);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::MultiplyTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix)
	{
		CriticalSection cs(this);

		TRACE("D3DTRANSFORMSTATETYPE state = %d, const D3DMATRIX *matrix = 0x%0.8p", state, matrix);

		if(!matrix)
		{
			return INVALIDCALL();
		}

		D3DMATRIX *current = &this->matrix[state];

		sw::Matrix C(current->_11, current->_21, current->_31, current->_41,
		             current->_12, current->_22, current->_32, current->_42,
		             current->_13, current->_23, current->_33, current->_43,
		             current->_14, current->_24, current->_34, current->_44);

		sw::Matrix M(matrix->_11, matrix->_21, matrix->_31, matrix->_41,
		             matrix->_12, matrix->_22, matrix->_32, matrix->_42,
		             matrix->_13, matrix->_23, matrix->_33, matrix->_43,
		             matrix->_14, matrix->_24, matrix->_34, matrix->_44);

		switch(state)
		{
		case D3DTS_WORLD:
			renderer->setModelMatrix(C * M);
			break;
		case D3DTS_VIEW:
			renderer->setViewMatrix(C * M);
			break;
		case D3DTS_PROJECTION:
			renderer->setProjectionMatrix(C * M);
			break;
		case D3DTS_TEXTURE0:
			renderer->setTextureMatrix(0, C * M);
			break;
		case D3DTS_TEXTURE1:
			renderer->setTextureMatrix(1, C * M);
			break;
		case D3DTS_TEXTURE2:
			renderer->setTextureMatrix(2, C * M);
			break;
		case D3DTS_TEXTURE3:
			renderer->setTextureMatrix(3, C * M);
			break;
		case D3DTS_TEXTURE4:
			renderer->setTextureMatrix(4, C * M);
			break;
		case D3DTS_TEXTURE5:
			renderer->setTextureMatrix(5, C * M);
			break;
		case D3DTS_TEXTURE6:
			renderer->setTextureMatrix(6, C * M);
			break;
		case D3DTS_TEXTURE7:
			renderer->setTextureMatrix(7, C * M);
			break;
		default:
			if(state > 256 && state < 512)
			{
				renderer->setModelMatrix(C * M, state - 256);
			}
			else ASSERT(false);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::Present(const RECT *sourceRect, const RECT *destRect, HWND destWindowOverride, const RGNDATA *dirtyRegion)
	{
		CriticalSection cs(this);

		TRACE("const RECT *sourceRect = 0x%0.8p, const RECT *destRect = 0x%0.8p, HWND destWindowOverride = %d, const RGNDATA *dirtyRegion = 0x%0.8p", sourceRect, destRect, destWindowOverride, dirtyRegion);

		return swapChain->Present(sourceRect, destRect, destWindowOverride, dirtyRegion, 0);
	}

	long Direct3DDevice9::ProcessVertices(unsigned int srcStartIndex, unsigned int destIndex, unsigned int vertexCount, IDirect3DVertexBuffer9 *destBuffer, IDirect3DVertexDeclaration9 *vertexDeclaration, unsigned long flags)
	{
		CriticalSection cs(this);

		TRACE("unsigned int srcStartIndex = %d, unsigned int destIndex = %d, unsigned int vertexCount = %d, IDirect3DVertexBuffer9 *destBuffer = 0x%0.8p, IDirect3DVertexDeclaration9 *vertexDeclaration = 0x%0.8p, unsigned long flags = %d", srcStartIndex, destIndex, vertexCount, destBuffer, vertexDeclaration, flags);

		if(!destBuffer)
		{
			return INVALIDCALL();
		}

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DDevice9::Reset(D3DPRESENT_PARAMETERS *presentParameters)
	{
		CriticalSection cs(this);

		TRACE("D3DPRESENT_PARAMETERS *presentParameters = 0x%0.8p", presentParameters);

		if(!presentParameters)
		{
			return INVALIDCALL();
		}

		deviceWindow = presentParameters->hDeviceWindow;

		if(depthStencil)
		{
			depthStencil->unbind();
			depthStencil = 0;
		}

		if(autoDepthStencil)
		{
			autoDepthStencil->unbind();
			autoDepthStencil = 0;
		}

		for(int index = 0; index < 4; index++)
		{
			if(renderTarget[index])
			{
				renderTarget[index]->unbind();
				renderTarget[index] = 0;
			}
		}

		if(!swapChain)
		{
			swapChain = new Direct3DSwapChain9(this, presentParameters);
			swapChain->bind();
		}
		else
		{
			swapChain->reset(presentParameters);
		}

		if(presentParameters->EnableAutoDepthStencil != FALSE)
		{
			bool lockable = false;

			switch(presentParameters->AutoDepthStencilFormat)
			{
			case D3DFMT_D15S1:
			case D3DFMT_D24S8:
			case D3DFMT_D24X8:
			case D3DFMT_D24X4S4:
			case D3DFMT_D24FS8:
			case D3DFMT_D32:
			case D3DFMT_D16:
			case D3DFMT_DF24:
			case D3DFMT_DF16:
			case D3DFMT_INTZ:
				lockable = false;
				break;
			case D3DFMT_S8_LOCKABLE:
			case D3DFMT_D16_LOCKABLE:
			case D3DFMT_D32F_LOCKABLE:
			case D3DFMT_D32_LOCKABLE:
				lockable = true;
				break;
			default:
				ASSERT(false);
			}

			autoDepthStencil = new Direct3DSurface9(this, this, presentParameters->BackBufferWidth, presentParameters->BackBufferHeight, presentParameters->AutoDepthStencilFormat, D3DPOOL_DEFAULT, presentParameters->MultiSampleType, presentParameters->MultiSampleQuality, lockable, D3DUSAGE_DEPTHSTENCIL);
			autoDepthStencil->bind();

			SetDepthStencilSurface(autoDepthStencil);
		}

		IDirect3DSurface9 *renderTarget;
		swapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &renderTarget);
		SetRenderTarget(0, renderTarget);
		renderTarget->Release();

		SetRenderTarget(1, 0);
		SetRenderTarget(2, 0);
		SetRenderTarget(3, 0);

		softwareVertexProcessing = (behaviourFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING) == D3DCREATE_SOFTWARE_VERTEXPROCESSING;

		SetRenderState(D3DRS_ZENABLE, presentParameters->EnableAutoDepthStencil != FALSE ? D3DZB_TRUE : D3DZB_FALSE);
		SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
		SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		SetRenderState(D3DRS_LASTPIXEL, TRUE);
		SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
		SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
		SetRenderState(D3DRS_ALPHAREF, 0);
		SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
		SetRenderState(D3DRS_DITHERENABLE, FALSE);
		SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		SetRenderState(D3DRS_FOGENABLE, FALSE);
		SetRenderState(D3DRS_SPECULARENABLE, FALSE);
	//	SetRenderState(D3DRS_ZVISIBLE, 0);
		SetRenderState(D3DRS_FOGCOLOR, 0);
		SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
		SetRenderState(D3DRS_FOGSTART, FtoDW(0.0f));
		SetRenderState(D3DRS_FOGEND, FtoDW(1.0f));
		SetRenderState(D3DRS_FOGDENSITY, FtoDW(1.0f));
		SetRenderState(D3DRS_RANGEFOGENABLE, FALSE);
		SetRenderState(D3DRS_STENCILENABLE, FALSE);
		SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
		SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
		SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
		SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
		SetRenderState(D3DRS_STENCILREF, 0);
		SetRenderState(D3DRS_STENCILMASK, 0xFFFFFFFF);
		SetRenderState(D3DRS_STENCILWRITEMASK, 0xFFFFFFFF);
		SetRenderState(D3DRS_TEXTUREFACTOR, 0xFFFFFFFF);
		SetRenderState(D3DRS_WRAP0, 0);
		SetRenderState(D3DRS_WRAP1, 0);
		SetRenderState(D3DRS_WRAP2, 0);
		SetRenderState(D3DRS_WRAP3, 0);
		SetRenderState(D3DRS_WRAP4, 0);
		SetRenderState(D3DRS_WRAP5, 0);
		SetRenderState(D3DRS_WRAP6, 0);
		SetRenderState(D3DRS_WRAP7, 0);
		SetRenderState(D3DRS_CLIPPING, TRUE);
		SetRenderState(D3DRS_LIGHTING, TRUE);
		SetRenderState(D3DRS_AMBIENT, 0);
		SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
		SetRenderState(D3DRS_COLORVERTEX, TRUE);
		SetRenderState(D3DRS_LOCALVIEWER, TRUE);
		SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
		SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
		SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_COLOR2);
		SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
		SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
		SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
		SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
		SetRenderState(D3DRS_POINTSIZE, FtoDW(1.0f));
		SetRenderState(D3DRS_POINTSIZE_MIN, FtoDW(1.0f));
		SetRenderState(D3DRS_POINTSPRITEENABLE, FALSE);
		SetRenderState(D3DRS_POINTSCALEENABLE, FALSE);
		SetRenderState(D3DRS_POINTSCALE_A, FtoDW(1.0f));
		SetRenderState(D3DRS_POINTSCALE_B, FtoDW(0.0f));
		SetRenderState(D3DRS_POINTSCALE_C, FtoDW(0.0f));
		SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
		SetRenderState(D3DRS_MULTISAMPLEMASK, 0xFFFFFFFF);
		SetRenderState(D3DRS_PATCHEDGESTYLE, D3DPATCHEDGE_DISCRETE);
		SetRenderState(D3DRS_DEBUGMONITORTOKEN, D3DDMT_ENABLE);
		SetRenderState(D3DRS_POINTSIZE_MAX, FtoDW(64.0f));
		SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);
		SetRenderState(D3DRS_COLORWRITEENABLE, 0x0000000F);
		SetRenderState(D3DRS_TWEENFACTOR, FtoDW(0.0f));
		SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		SetRenderState(D3DRS_POSITIONDEGREE, D3DDEGREE_CUBIC);
		SetRenderState(D3DRS_NORMALDEGREE, D3DDEGREE_LINEAR);
		SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, FtoDW(0.0f));
		SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
		SetRenderState(D3DRS_MINTESSELLATIONLEVEL, FtoDW(1.0f));
		SetRenderState(D3DRS_MAXTESSELLATIONLEVEL, FtoDW(1.0f));
		SetRenderState(D3DRS_ADAPTIVETESS_X, FtoDW(0.0f));
		SetRenderState(D3DRS_ADAPTIVETESS_Y, FtoDW(0.0f));
		SetRenderState(D3DRS_ADAPTIVETESS_Z, FtoDW(1.0f));
		SetRenderState(D3DRS_ADAPTIVETESS_W, FtoDW(0.0f));
		SetRenderState(D3DRS_ENABLEADAPTIVETESSELLATION, FALSE);
		SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, FALSE);
		SetRenderState(D3DRS_CCW_STENCILFAIL, D3DSTENCILOP_KEEP);
		SetRenderState(D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_KEEP);
		SetRenderState(D3DRS_CCW_STENCILPASS, D3DSTENCILOP_KEEP);
		SetRenderState(D3DRS_CCW_STENCILFUNC, D3DCMP_ALWAYS);
		SetRenderState(D3DRS_COLORWRITEENABLE1, 0x0000000F);
		SetRenderState(D3DRS_COLORWRITEENABLE2, 0x0000000F);
		SetRenderState(D3DRS_COLORWRITEENABLE3, 0x0000000F);
		SetRenderState(D3DRS_BLENDFACTOR, 0xFFFFFFFF);
		SetRenderState(D3DRS_SRGBWRITEENABLE, 0);
		SetRenderState(D3DRS_DEPTHBIAS, FtoDW(0.0f));
		SetRenderState(D3DRS_WRAP8, 0);
		SetRenderState(D3DRS_WRAP9, 0);
		SetRenderState(D3DRS_WRAP10, 0);
		SetRenderState(D3DRS_WRAP11, 0);
		SetRenderState(D3DRS_WRAP12, 0);
		SetRenderState(D3DRS_WRAP13, 0);
		SetRenderState(D3DRS_WRAP14, 0);
		SetRenderState(D3DRS_WRAP15, 0);
		SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
		SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
		SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
		SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);

		for(int i = 0; i < 8; i++)
		{
			SetTextureStageState(i, D3DTSS_COLOROP, i == 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE);
			SetTextureStageState(i, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			SetTextureStageState(i, D3DTSS_COLORARG2, D3DTA_CURRENT);
			SetTextureStageState(i, D3DTSS_ALPHAOP, i == 0 ? D3DTOP_SELECTARG1 : D3DTOP_DISABLE);
			SetTextureStageState(i, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			SetTextureStageState(i, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
			SetTextureStageState(i, D3DTSS_BUMPENVMAT00, FtoDW(0.0f));
			SetTextureStageState(i, D3DTSS_BUMPENVMAT01, FtoDW(0.0f));
			SetTextureStageState(i, D3DTSS_BUMPENVMAT10, FtoDW(0.0f));
			SetTextureStageState(i, D3DTSS_BUMPENVMAT11, FtoDW(0.0f));
			SetTextureStageState(i, D3DTSS_TEXCOORDINDEX, i);
			SetTextureStageState(i, D3DTSS_BUMPENVLSCALE, FtoDW(0.0f));
			SetTextureStageState(i, D3DTSS_BUMPENVLOFFSET, FtoDW(0.0f));
			SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
			SetTextureStageState(i, D3DTSS_COLORARG0, D3DTA_CURRENT);
			SetTextureStageState(i, D3DTSS_ALPHAARG0, D3DTA_CURRENT);
			SetTextureStageState(i, D3DTSS_RESULTARG, D3DTA_CURRENT);
			SetTextureStageState(i, D3DTSS_CONSTANT, 0x00000000);
		}

		for(int i = 0; i <= D3DVERTEXTEXTURESAMPLER3; i = (i != 15) ? (i + 1) : D3DVERTEXTEXTURESAMPLER0)
		{
			SetTexture(i, 0);

			SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
			SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
			SetSamplerState(i, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
			SetSamplerState(i, D3DSAMP_BORDERCOLOR, 0x00000000);
			SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
			SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, 0);
			SetSamplerState(i, D3DSAMP_MAXMIPLEVEL, 0);
			SetSamplerState(i, D3DSAMP_MAXANISOTROPY, 1);
			SetSamplerState(i, D3DSAMP_SRGBTEXTURE, 0);
			SetSamplerState(i, D3DSAMP_ELEMENTINDEX, 0);
			SetSamplerState(i, D3DSAMP_DMAPOFFSET, 0);
		}

		for(int i = 0; i < 6; i++)
		{
			float plane[4] = {0, 0, 0, 0};

			SetClipPlane(i, plane);
		}

		currentPalette = 0xFFFF;

		ShowCursor(FALSE);
		delete cursor;
		cursor = 0;

		return D3D_OK;
	}

	long Direct3DDevice9::SetClipPlane(unsigned long index, const float *plane)
	{
		CriticalSection cs(this);

		TRACE("unsigned long index = %d, const float *plane = 0x%0.8p", index, plane);

		if(!plane || index >= 6)
		{
			return INVALIDCALL();
		}

		if(!stateRecorder)
		{
			this->plane[index][0] = plane[0];
			this->plane[index][1] = plane[1];
			this->plane[index][2] = plane[2];
			this->plane[index][3] = plane[3];

			renderer->setClipPlane(index, plane);
		}
		else
		{
			stateRecorder->setClipPlane(index, plane);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetClipStatus(const D3DCLIPSTATUS9 *clipStatus)
	{
		CriticalSection cs(this);

		TRACE("const D3DCLIPSTATUS9 *clipStatus = 0x%0.8p", clipStatus);

		if(!clipStatus)
		{
			return INVALIDCALL();
		}

		this->clipStatus = *clipStatus;

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DDevice9::SetCurrentTexturePalette(unsigned int paletteNumber)
	{
		CriticalSection cs(this);

		TRACE("unsigned int paletteNumber = %d", paletteNumber);

		if(paletteNumber > 0xFFFF || palette.find(paletteNumber) == palette.end())
		{
			return INVALIDCALL();
		}

		if(!stateRecorder)
		{
			currentPalette = paletteNumber;

			sw::Surface::setTexturePalette((unsigned int*)&palette[currentPalette]);
		}
		else
		{
			stateRecorder->setCurrentTexturePalette(paletteNumber);
		}

		return D3D_OK;
	}

	void Direct3DDevice9::SetCursorPosition(int x, int y, unsigned long flags)
	{
		CriticalSection cs(this);

		TRACE("int x = %d, int y = %d, unsigned long flags = 0x%0.8X", x, y, flags);

		POINT point = {x, y};
		HWND window = deviceWindow ? deviceWindow : focusWindow;
		ScreenToClient(window, &point);

		sw::FrameBuffer::setCursorPosition(point.x, point.y);
	}

	long Direct3DDevice9::SetCursorProperties(unsigned int x0, unsigned int y0, IDirect3DSurface9 *cursorBitmap)
	{
		CriticalSection cs(this);

		TRACE("unsigned int x0 = %d, unsigned int y0 = %d, IDirect3DSurface9 *cursorBitmap = 0x%0.8p", x0, y0, cursorBitmap);

		if(!cursorBitmap)
		{
			return INVALIDCALL();
		}

		sw::Surface *cursorSurface = static_cast<Direct3DSurface9*>(cursorBitmap);

		int width = cursorSurface->getWidth();
		int height = cursorSurface->getHeight();
		void *bitmap = cursorSurface->lockExternal(0, 0, 0, sw::LOCK_READONLY, sw::PUBLIC);

		delete cursor;
		cursor = sw::Surface::create(nullptr, width, height, 1, 0, 1, sw::FORMAT_A8R8G8B8, false, false);

		void *buffer = cursor->lockExternal(0, 0, 0, sw::LOCK_DISCARD, sw::PUBLIC);
		memcpy(buffer, bitmap, width * height * sizeof(unsigned int));
		cursor->unlockExternal();

		cursorSurface->unlockExternal();

		if(showCursor)
		{
			sw::FrameBuffer::setCursorImage(cursor);
		}
		else
		{
			sw::FrameBuffer::setCursorImage(nullptr);
		}

		sw::FrameBuffer::setCursorOrigin(x0, y0);

		return D3D_OK;
	}

	long Direct3DDevice9::SetDepthStencilSurface(IDirect3DSurface9 *iDepthStencil)
	{
		CriticalSection cs(this);

		TRACE("IDirect3DSurface9 *newDepthStencil = 0x%0.8p", iDepthStencil);

		Direct3DSurface9 *depthStencil = static_cast<Direct3DSurface9*>(iDepthStencil);

		if(this->depthStencil == depthStencil)
		{
			return D3D_OK;
		}

		if(depthStencil)
		{
			depthStencil->bind();
		}

		if(this->depthStencil)
		{
			this->depthStencil->unbind();
		}

		this->depthStencil = depthStencil;

		renderer->setDepthBuffer(depthStencil);
		renderer->setStencilBuffer(depthStencil);

		return D3D_OK;
	}

	long Direct3DDevice9::SetDialogBoxMode(int enableDialogs)
	{
		CriticalSection cs(this);

		TRACE("int enableDialogs = %d", enableDialogs);

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DDevice9::SetFVF(unsigned long FVF)
	{
		CriticalSection cs(this);

		TRACE("unsigned long FVF = 0x%0.8X", FVF);

		if(!stateRecorder)
		{
			if(FVF != 0 || !this->vertexDeclaration)
			{
				Direct3DVertexDeclaration9 *vertexDeclaration = new Direct3DVertexDeclaration9(this, FVF);
				vertexDeclaration->bind();

				if(this->vertexDeclaration)
				{
					this->vertexDeclaration->unbind();
				}

				this->vertexDeclaration = vertexDeclaration;
			}
		}
		else
		{
			stateRecorder->setFVF(FVF);
		}

		return D3D_OK;
	}

	void Direct3DDevice9::SetGammaRamp(unsigned int index, unsigned long flags, const D3DGAMMARAMP *ramp)
	{
		CriticalSection cs(this);

		TRACE("unsigned int index = %d, unsigned long flags = 0x%0.8X, const D3DGAMMARAMP *ramp = 0x%0.8p", index, flags, ramp);

		if(!ramp || index >= GetNumberOfSwapChains())
		{
			return;
		}

		swapChain->setGammaRamp((sw::GammaRamp*)ramp, flags & D3DSGR_CALIBRATE);
	}

	long Direct3DDevice9::SetIndices(IDirect3DIndexBuffer9* iIndexBuffer)
	{
		CriticalSection cs(this);

		TRACE("IDirect3DIndexBuffer9* indexData = 0x%0.8p", iIndexBuffer);

		Direct3DIndexBuffer9 *indexBuffer = static_cast<Direct3DIndexBuffer9*>(iIndexBuffer);

		if(!stateRecorder)
		{
			if(this->indexData == indexBuffer)
			{
				return D3D_OK;
			}

			if(indexBuffer)
			{
				indexBuffer->bind();
			}

			if(this->indexData)
			{
				this->indexData->unbind();
			}

			this->indexData = indexBuffer;
		}
		else
		{
			stateRecorder->setIndices(indexBuffer);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetLight(unsigned long index, const D3DLIGHT9 *light)
	{
		CriticalSection cs(this);

		TRACE("unsigned long index = %d, const D3DLIGHT9 *light = 0x%0.8p", index, light);

		if(!light)
		{
			return INVALIDCALL();
		}

		if(!stateRecorder)
		{
			this->light[index] = *light;

			lightsDirty = true;
		}
		else
		{
			stateRecorder->setLight(index, light);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetMaterial(const D3DMATERIAL9 *material)
	{
		CriticalSection cs(this);

		TRACE("const D3DMATERIAL9 *material = 0x%0.8p", material);

		if(!material)
		{
			return INVALIDCALL();   // FIXME: Correct behaviour?
		}

		if(!stateRecorder)
		{
			this->material = *material;

			renderer->setMaterialAmbient(sw::Color<float>(material->Ambient.r, material->Ambient.g, material->Ambient.b, material->Ambient.a));
			renderer->setMaterialDiffuse(sw::Color<float>(material->Diffuse.r, material->Diffuse.g, material->Diffuse.b, material->Diffuse.a));
			renderer->setMaterialEmission(sw::Color<float>(material->Emissive.r, material->Emissive.g, material->Emissive.b, material->Emissive.a));
			renderer->setMaterialShininess(material->Power);
			renderer->setMaterialSpecular(sw::Color<float>(material->Specular.r, material->Specular.g, material->Specular.b, material->Specular.a));
		}
		else
		{
			stateRecorder->setMaterial(material);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetNPatchMode(float segments)
	{
		CriticalSection cs(this);

		TRACE("float segments = %f", segments);

		if(!stateRecorder)
		{
			if(segments < 1)
			{
				// NOTE: Disable
			}
			else
			{
				UNIMPLEMENTED();
			}
		}
		else
		{
			stateRecorder->setNPatchMode(segments);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetPaletteEntries(unsigned int paletteNumber, const PALETTEENTRY *entries)
	{
		CriticalSection cs(this);

		TRACE("unsigned int paletteNumber = %d, const PALETTEENTRY *entries = 0x%0.8p", paletteNumber, entries);

		if(paletteNumber > 0xFFFF || !entries)
		{
			return INVALIDCALL();
		}

		for(int i = 0; i < 256; i++)
		{
			palette[paletteNumber].entry[i] = entries[i];
		}

		if(paletteNumber == currentPalette)
		{
			sw::Surface::setTexturePalette((unsigned int*)&palette[currentPalette]);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetPixelShader(IDirect3DPixelShader9 *iPixelShader)
	{
		CriticalSection cs(this);

		TRACE("IDirect3DPixelShader9 *shader = 0x%0.8p", iPixelShader);

		Direct3DPixelShader9 *pixelShader = static_cast<Direct3DPixelShader9*>(iPixelShader);

		if(!stateRecorder)
		{
			if(this->pixelShader == pixelShader)
			{
				return D3D_OK;
			}

			if(pixelShader)
			{
				pixelShader->bind();
			}

			if(this->pixelShader)
			{
				this->pixelShader->unbind();
			}

			this->pixelShader = pixelShader;
			pixelShaderDirty = true;
		}
		else
		{
			stateRecorder->setPixelShader(pixelShader);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetPixelShaderConstantB(unsigned int startRegister, const int *constantData, unsigned int count)
	{
		CriticalSection cs(this);

		TRACE("unsigned int startRegister = %d, const int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		if(!stateRecorder)
		{
			for(unsigned int i = 0; i < count && startRegister + i < 16; i++)
			{
				pixelShaderConstantB[startRegister + i] = constantData[i];
			}

			pixelShaderConstantsBDirty = sw::max(startRegister + count, pixelShaderConstantsBDirty);
			pixelShaderDirty = true;   // Reload DEF constants
		}
		else
		{
			stateRecorder->setPixelShaderConstantB(startRegister, constantData, count);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetPixelShaderConstantF(unsigned int startRegister, const float *constantData, unsigned int count)
	{
		CriticalSection cs(this);

		TRACE("unsigned int startRegister = %d, const int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		if(!stateRecorder)
		{
			for(unsigned int i = 0; i < count && startRegister + i < MAX_PIXEL_SHADER_CONST; i++)
			{
				pixelShaderConstantF[startRegister + i][0] = constantData[i * 4 + 0];
				pixelShaderConstantF[startRegister + i][1] = constantData[i * 4 + 1];
				pixelShaderConstantF[startRegister + i][2] = constantData[i * 4 + 2];
				pixelShaderConstantF[startRegister + i][3] = constantData[i * 4 + 3];
			}

			pixelShaderConstantsFDirty = sw::max(startRegister + count, pixelShaderConstantsFDirty);
			pixelShaderDirty = true;   // Reload DEF constants
		}
		else
		{
			stateRecorder->setPixelShaderConstantF(startRegister, constantData, count);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetPixelShaderConstantI(unsigned int startRegister, const int *constantData, unsigned int count)
	{
		CriticalSection cs(this);

		TRACE("unsigned int startRegister = %d, const int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		if(!stateRecorder)
		{
			for(unsigned int i = 0; i < count && startRegister + i < 16; i++)
			{
				pixelShaderConstantI[startRegister + i][0] = constantData[i * 4 + 0];
				pixelShaderConstantI[startRegister + i][1] = constantData[i * 4 + 1];
				pixelShaderConstantI[startRegister + i][2] = constantData[i * 4 + 2];
				pixelShaderConstantI[startRegister + i][3] = constantData[i * 4 + 3];
			}

			pixelShaderConstantsIDirty = sw::max(startRegister + count, pixelShaderConstantsIDirty);
			pixelShaderDirty = true;   // Reload DEF constants
		}
		else
		{
			stateRecorder->setPixelShaderConstantI(startRegister, constantData, count);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetRenderState(D3DRENDERSTATETYPE state, unsigned long value)
	{
		CriticalSection cs(this);

		TRACE("D3DRENDERSTATETYPE state = %d, unsigned long value = %d", state, value);

		if(state < D3DRS_ZENABLE || state > D3DRS_BLENDOPALPHA)
		{
			return D3D_OK;   // FIXME: Warning
		}

		if(!stateRecorder)
		{
			if(!init && renderState[state] == value)
			{
				return D3D_OK;
			}

			renderState[state] = value;

			switch(state)
			{
			case D3DRS_ZENABLE:
				switch(value)
				{
				case D3DZB_TRUE:
				case D3DZB_USEW:
					renderer->setDepthBufferEnable(true);
					break;
				case D3DZB_FALSE:
					renderer->setDepthBufferEnable(false);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_FILLMODE:
				switch(value)
				{
				case D3DFILL_POINT:
					renderer->setFillMode(sw::FILL_VERTEX);
					break;
				case D3DFILL_WIREFRAME:
					renderer->setFillMode(sw::FILL_WIREFRAME);
					break;
				case D3DFILL_SOLID:
					renderer->setFillMode(sw::FILL_SOLID);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_SHADEMODE:
				switch(value)
				{
				case D3DSHADE_FLAT:
					renderer->setShadingMode(sw::SHADING_FLAT);
					break;
				case D3DSHADE_GOURAUD:
					renderer->setShadingMode(sw::SHADING_GOURAUD);
					break;
				case D3DSHADE_PHONG:
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_ZWRITEENABLE:
				renderer->setDepthWriteEnable(value != FALSE);
				break;
			case D3DRS_ALPHATESTENABLE:
				renderer->setAlphaTestEnable(value != FALSE);
				break;
			case D3DRS_LASTPIXEL:
			//	if(!init) UNIMPLEMENTED();   // FIXME
				break;
			case D3DRS_SRCBLEND:
				switch(value)
				{
				case D3DBLEND_ZERO:
					renderer->setSourceBlendFactor(sw::BLEND_ZERO);
					break;
				case D3DBLEND_ONE:
					renderer->setSourceBlendFactor(sw::BLEND_ONE);
					break;
				case D3DBLEND_SRCCOLOR:
					renderer->setSourceBlendFactor(sw::BLEND_SOURCE);
					break;
				case D3DBLEND_INVSRCCOLOR:
					renderer->setSourceBlendFactor(sw::BLEND_INVSOURCE);
					break;
				case D3DBLEND_SRCALPHA:
					renderer->setSourceBlendFactor(sw::BLEND_SOURCEALPHA);
					break;
				case D3DBLEND_INVSRCALPHA:
					renderer->setSourceBlendFactor(sw::BLEND_INVSOURCEALPHA);
					break;
				case D3DBLEND_DESTALPHA:
					renderer->setSourceBlendFactor(sw::BLEND_DESTALPHA);
					break;
				case D3DBLEND_INVDESTALPHA:
					renderer->setSourceBlendFactor(sw::BLEND_INVDESTALPHA);
					break;
				case D3DBLEND_DESTCOLOR:
					renderer->setSourceBlendFactor(sw::BLEND_DEST);
					break;
				case D3DBLEND_INVDESTCOLOR:
					renderer->setSourceBlendFactor(sw::BLEND_INVDEST);
					break;
				case D3DBLEND_SRCALPHASAT:
					renderer->setSourceBlendFactor(sw::BLEND_SRCALPHASAT);
					break;
				case D3DBLEND_BOTHSRCALPHA:
					renderer->setSourceBlendFactor(sw::BLEND_SOURCEALPHA);
					renderer->setDestBlendFactor(sw::BLEND_INVSOURCEALPHA);
					break;
				case D3DBLEND_BOTHINVSRCALPHA:
					renderer->setSourceBlendFactor(sw::BLEND_INVSOURCEALPHA);
					renderer->setDestBlendFactor(sw::BLEND_SOURCEALPHA);
					break;
				case D3DBLEND_BLENDFACTOR:
					renderer->setSourceBlendFactor(sw::BLEND_CONSTANT);
					break;
				case D3DBLEND_INVBLENDFACTOR:
					renderer->setSourceBlendFactor(sw::BLEND_INVCONSTANT);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_DESTBLEND:
				switch(value)
				{
				case D3DBLEND_ZERO:
					renderer->setDestBlendFactor(sw::BLEND_ZERO);
					break;
				case D3DBLEND_ONE:
					renderer->setDestBlendFactor(sw::BLEND_ONE);
					break;
				case D3DBLEND_SRCCOLOR:
					renderer->setDestBlendFactor(sw::BLEND_SOURCE);
					break;
				case D3DBLEND_INVSRCCOLOR:
					renderer->setDestBlendFactor(sw::BLEND_INVSOURCE);
					break;
				case D3DBLEND_SRCALPHA:
					renderer->setDestBlendFactor(sw::BLEND_SOURCEALPHA);
					break;
				case D3DBLEND_INVSRCALPHA:
					renderer->setDestBlendFactor(sw::BLEND_INVSOURCEALPHA);
					break;
				case D3DBLEND_DESTALPHA:
					renderer->setDestBlendFactor(sw::BLEND_DESTALPHA);
					break;
				case D3DBLEND_INVDESTALPHA:
					renderer->setDestBlendFactor(sw::BLEND_INVDESTALPHA);
					break;
				case D3DBLEND_DESTCOLOR:
					renderer->setDestBlendFactor(sw::BLEND_DEST);
					break;
				case D3DBLEND_INVDESTCOLOR:
					renderer->setDestBlendFactor(sw::BLEND_INVDEST);
					break;
				case D3DBLEND_SRCALPHASAT:
					renderer->setDestBlendFactor(sw::BLEND_SRCALPHASAT);
					break;
				case D3DBLEND_BOTHSRCALPHA:
					renderer->setSourceBlendFactor(sw::BLEND_SOURCEALPHA);
					renderer->setDestBlendFactor(sw::BLEND_INVSOURCEALPHA);
					break;
				case D3DBLEND_BOTHINVSRCALPHA:
					renderer->setSourceBlendFactor(sw::BLEND_INVSOURCEALPHA);
					renderer->setDestBlendFactor(sw::BLEND_SOURCEALPHA);
					break;
				case D3DBLEND_BLENDFACTOR:
					renderer->setDestBlendFactor(sw::BLEND_CONSTANT);
					break;
				case D3DBLEND_INVBLENDFACTOR:
					renderer->setDestBlendFactor(sw::BLEND_INVCONSTANT);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_CULLMODE:
				switch(value)
				{
				case D3DCULL_NONE:
					renderer->setCullMode(sw::CULL_NONE);
					break;
				case D3DCULL_CCW:
					renderer->setCullMode(sw::CULL_COUNTERCLOCKWISE);
					break;
				case D3DCULL_CW:
					renderer->setCullMode(sw::CULL_CLOCKWISE);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_ZFUNC:
				switch(value)
				{
				case D3DCMP_NEVER:
					renderer->setDepthCompare(sw::DEPTH_NEVER);
					break;
				case D3DCMP_LESS:
					renderer->setDepthCompare(sw::DEPTH_LESS);
					break;
				case D3DCMP_EQUAL:
					renderer->setDepthCompare(sw::DEPTH_EQUAL);
					break;
				case D3DCMP_LESSEQUAL:
					renderer->setDepthCompare(sw::DEPTH_LESSEQUAL);
					break;
				case D3DCMP_GREATER:
					renderer->setDepthCompare(sw::DEPTH_GREATER);
					break;
				case D3DCMP_NOTEQUAL:
					renderer->setDepthCompare(sw::DEPTH_NOTEQUAL);
					break;
				case D3DCMP_GREATEREQUAL:
					renderer->setDepthCompare(sw::DEPTH_GREATEREQUAL);
					break;
				case D3DCMP_ALWAYS:
					renderer->setDepthCompare(sw::DEPTH_ALWAYS);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_ALPHAREF:
				renderer->setAlphaReference(value & 0x000000FF);
				break;
			case D3DRS_ALPHAFUNC:
				switch(value)
				{
				case D3DCMP_NEVER:
					renderer->setAlphaCompare(sw::ALPHA_NEVER);
					break;
				case D3DCMP_LESS:
					renderer->setAlphaCompare(sw::ALPHA_LESS);
					break;
				case D3DCMP_EQUAL:
					renderer->setAlphaCompare(sw::ALPHA_EQUAL);
					break;
				case D3DCMP_LESSEQUAL:
					renderer->setAlphaCompare(sw::ALPHA_LESSEQUAL);
					break;
				case D3DCMP_GREATER:
					renderer->setAlphaCompare(sw::ALPHA_GREATER);
					break;
				case D3DCMP_NOTEQUAL:
					renderer->setAlphaCompare(sw::ALPHA_NOTEQUAL);
					break;
				case D3DCMP_GREATEREQUAL:
					renderer->setAlphaCompare(sw::ALPHA_GREATEREQUAL);
					break;
				case D3DCMP_ALWAYS:
					renderer->setAlphaCompare(sw::ALPHA_ALWAYS);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_DITHERENABLE:
			//	if(!init) UNIMPLEMENTED();
				break;
			case D3DRS_ALPHABLENDENABLE:
				renderer->setAlphaBlendEnable(value != FALSE);
				break;
			case D3DRS_FOGENABLE:
				renderer->setFogEnable(value != FALSE);
				break;
			case D3DRS_FOGCOLOR:
				renderer->setFogColor(value);
				break;
			case D3DRS_FOGTABLEMODE:
				switch(value)
				{
				case D3DFOG_NONE:
					renderer->setPixelFogMode(sw::FOG_NONE);
					break;
				case D3DFOG_LINEAR:
					renderer->setPixelFogMode(sw::FOG_LINEAR);
					break;
				case D3DFOG_EXP:
					renderer->setPixelFogMode(sw::FOG_EXP);
					break;
				case D3DFOG_EXP2:
					renderer->setPixelFogMode(sw::FOG_EXP2);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_FOGSTART:
				renderer->setFogStart((float&)value);
				break;
			case D3DRS_FOGEND:
				renderer->setFogEnd((float&)value);
				break;
			case D3DRS_FOGDENSITY:
				renderer->setFogDensity((float&)value);
				break;
			case D3DRS_RANGEFOGENABLE:
				renderer->setRangeFogEnable(value != FALSE);
				break;
			case D3DRS_SPECULARENABLE:
				renderer->setSpecularEnable(value != FALSE);
				break;
			case D3DRS_STENCILENABLE:
				renderer->setStencilEnable(value != FALSE);
				break;
			case D3DRS_STENCILFAIL:
				switch(value)
				{
				case D3DSTENCILOP_KEEP:
					renderer->setStencilFailOperation(sw::OPERATION_KEEP);
					break;
				case D3DSTENCILOP_ZERO:
					renderer->setStencilFailOperation(sw::OPERATION_ZERO);
					break;
				case D3DSTENCILOP_REPLACE:
					renderer->setStencilFailOperation(sw::OPERATION_REPLACE);
					break;
				case D3DSTENCILOP_INCRSAT:
					renderer->setStencilFailOperation(sw::OPERATION_INCRSAT);
					break;
				case D3DSTENCILOP_DECRSAT:
					renderer->setStencilFailOperation(sw::OPERATION_DECRSAT);
					break;
				case D3DSTENCILOP_INVERT:
					renderer->setStencilFailOperation(sw::OPERATION_INVERT);
					break;
				case D3DSTENCILOP_INCR:
					renderer->setStencilFailOperation(sw::OPERATION_INCR);
					break;
				case D3DSTENCILOP_DECR:
					renderer->setStencilFailOperation(sw::OPERATION_DECR);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_STENCILZFAIL:
				switch(value)
				{
				case D3DSTENCILOP_KEEP:
					renderer->setStencilZFailOperation(sw::OPERATION_KEEP);
					break;
				case D3DSTENCILOP_ZERO:
					renderer->setStencilZFailOperation(sw::OPERATION_ZERO);
					break;
				case D3DSTENCILOP_REPLACE:
					renderer->setStencilZFailOperation(sw::OPERATION_REPLACE);
					break;
				case D3DSTENCILOP_INCRSAT:
					renderer->setStencilZFailOperation(sw::OPERATION_INCRSAT);
					break;
				case D3DSTENCILOP_DECRSAT:
					renderer->setStencilZFailOperation(sw::OPERATION_DECRSAT);
					break;
				case D3DSTENCILOP_INVERT:
					renderer->setStencilZFailOperation(sw::OPERATION_INVERT);
					break;
				case D3DSTENCILOP_INCR:
					renderer->setStencilZFailOperation(sw::OPERATION_INCR);
					break;
				case D3DSTENCILOP_DECR:
					renderer->setStencilZFailOperation(sw::OPERATION_DECR);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_STENCILPASS:
				switch(value)
				{
				case D3DSTENCILOP_KEEP:
					renderer->setStencilPassOperation(sw::OPERATION_KEEP);
					break;
				case D3DSTENCILOP_ZERO:
					renderer->setStencilPassOperation(sw::OPERATION_ZERO);
					break;
				case D3DSTENCILOP_REPLACE:
					renderer->setStencilPassOperation(sw::OPERATION_REPLACE);
					break;
				case D3DSTENCILOP_INCRSAT:
					renderer->setStencilPassOperation(sw::OPERATION_INCRSAT);
					break;
				case D3DSTENCILOP_DECRSAT:
					renderer->setStencilPassOperation(sw::OPERATION_DECRSAT);
					break;
				case D3DSTENCILOP_INVERT:
					renderer->setStencilPassOperation(sw::OPERATION_INVERT);
					break;
				case D3DSTENCILOP_INCR:
					renderer->setStencilPassOperation(sw::OPERATION_INCR);
					break;
				case D3DSTENCILOP_DECR:
					renderer->setStencilPassOperation(sw::OPERATION_DECR);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_STENCILFUNC:
				switch(value)
				{
				case D3DCMP_NEVER:
					renderer->setStencilCompare(sw::STENCIL_NEVER);
					break;
				case D3DCMP_LESS:
					renderer->setStencilCompare(sw::STENCIL_LESS);
					break;
				case D3DCMP_EQUAL:
					renderer->setStencilCompare(sw::STENCIL_EQUAL);
					break;
				case D3DCMP_LESSEQUAL:
					renderer->setStencilCompare(sw::STENCIL_LESSEQUAL);
					break;
				case D3DCMP_GREATER:
					renderer->setStencilCompare(sw::STENCIL_GREATER);
					break;
				case D3DCMP_NOTEQUAL:
					renderer->setStencilCompare(sw::STENCIL_NOTEQUAL);
					break;
				case D3DCMP_GREATEREQUAL:
					renderer->setStencilCompare(sw::STENCIL_GREATEREQUAL);
					break;
				case D3DCMP_ALWAYS:
					renderer->setStencilCompare(sw::STENCIL_ALWAYS);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_STENCILREF:
				renderer->setStencilReference(value);
				renderer->setStencilReferenceCCW(value);
				break;
			case D3DRS_STENCILMASK:
				renderer->setStencilMask(value);
				renderer->setStencilMaskCCW(value);
				break;
			case D3DRS_STENCILWRITEMASK:
				renderer->setStencilWriteMask(value);
				renderer->setStencilWriteMaskCCW(value);
				break;
			case D3DRS_TEXTUREFACTOR:
				renderer->setTextureFactor(value);
				break;
			case D3DRS_WRAP0:
				renderer->setTextureWrap(0, value);
				break;
			case D3DRS_WRAP1:
				renderer->setTextureWrap(1, value);
				break;
			case D3DRS_WRAP2:
				renderer->setTextureWrap(2, value);
				break;
			case D3DRS_WRAP3:
				renderer->setTextureWrap(3, value);
				break;
			case D3DRS_WRAP4:
				renderer->setTextureWrap(4, value);
				break;
			case D3DRS_WRAP5:
				renderer->setTextureWrap(5, value);
				break;
			case D3DRS_WRAP6:
				renderer->setTextureWrap(6, value);
				break;
			case D3DRS_WRAP7:
				renderer->setTextureWrap(7, value);
				break;
			case D3DRS_CLIPPING:
				// Ignored, clipping is always performed
				break;
			case D3DRS_LIGHTING:
				renderer->setLightingEnable(value != FALSE);
				break;
			case D3DRS_AMBIENT:
				renderer->setGlobalAmbient(value);
				break;
			case D3DRS_FOGVERTEXMODE:
				switch(value)
				{
				case D3DFOG_NONE:
					renderer->setVertexFogMode(sw::FOG_NONE);
					break;
				case D3DFOG_LINEAR:
					renderer->setVertexFogMode(sw::FOG_LINEAR);
					break;
				case D3DFOG_EXP:
					renderer->setVertexFogMode(sw::FOG_EXP);
					break;
				case D3DFOG_EXP2:
					renderer->setVertexFogMode(sw::FOG_EXP2);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_COLORVERTEX:
				renderer->setColorVertexEnable(value != FALSE);
				break;
			case D3DRS_LOCALVIEWER:
				renderer->setLocalViewer(value != FALSE);
				break;
			case D3DRS_NORMALIZENORMALS:
				renderer->setNormalizeNormals(value != FALSE);
				break;
			case D3DRS_DIFFUSEMATERIALSOURCE:
				switch(value)
				{
				case D3DMCS_MATERIAL:
					renderer->setDiffuseMaterialSource(sw::MATERIAL_MATERIAL);
					break;
				case D3DMCS_COLOR1:
					renderer->setDiffuseMaterialSource(sw::MATERIAL_COLOR1);
					break;
				case D3DMCS_COLOR2:
					renderer->setDiffuseMaterialSource(sw::MATERIAL_COLOR2);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_SPECULARMATERIALSOURCE:
				switch(value)
				{
				case D3DMCS_MATERIAL:
					renderer->setSpecularMaterialSource(sw::MATERIAL_MATERIAL);
					break;
				case D3DMCS_COLOR1:
					renderer->setSpecularMaterialSource(sw::MATERIAL_COLOR1);
					break;
				case D3DMCS_COLOR2:
					renderer->setSpecularMaterialSource(sw::MATERIAL_COLOR2);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_AMBIENTMATERIALSOURCE:
				switch(value)
				{
				case D3DMCS_MATERIAL:
					renderer->setAmbientMaterialSource(sw::MATERIAL_MATERIAL);
					break;
				case D3DMCS_COLOR1:
					renderer->setAmbientMaterialSource(sw::MATERIAL_COLOR1);
					break;
				case D3DMCS_COLOR2:
					renderer->setAmbientMaterialSource(sw::MATERIAL_COLOR2);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_EMISSIVEMATERIALSOURCE:
				switch(value)
				{
				case D3DMCS_MATERIAL:
					renderer->setEmissiveMaterialSource(sw::MATERIAL_MATERIAL);
					break;
				case D3DMCS_COLOR1:
					renderer->setEmissiveMaterialSource(sw::MATERIAL_COLOR1);
					break;
				case D3DMCS_COLOR2:
					renderer->setEmissiveMaterialSource(sw::MATERIAL_COLOR2);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_VERTEXBLEND:
				switch(value)
				{
				case D3DVBF_DISABLE:
					renderer->setVertexBlendMatrixCount(0);
					break;
				case D3DVBF_1WEIGHTS:
					renderer->setVertexBlendMatrixCount(2);
					break;
				case D3DVBF_2WEIGHTS:
					renderer->setVertexBlendMatrixCount(3);
					break;
				case D3DVBF_3WEIGHTS:
					renderer->setVertexBlendMatrixCount(4);
					break;
				case D3DVBF_TWEENING:
					UNIMPLEMENTED();
					break;
				case D3DVBF_0WEIGHTS:
					renderer->setVertexBlendMatrixCount(1);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_CLIPPLANEENABLE:
				renderer->setClipFlags(value);
				break;
			case D3DRS_POINTSIZE:
				if(value == D3DFMT_INST && pixelShaderVersionX >= D3DPS_VERSION(2, 0))   // ATI hack to enable instancing on SM 2.0 hardware
				{
					instancingEnabled = true;
				}
				else if(value == D3DFMT_A2M1)   // ATI hack to enable transparency anti-aliasing
				{
					renderer->setTransparencyAntialiasing(sw::TRANSPARENCY_ALPHA_TO_COVERAGE);
					renderer->setAlphaTestEnable(true);
				}
				else if(value == D3DFMT_A2M0)   // ATI hack to disable transparency anti-aliasing
				{
					renderer->setTransparencyAntialiasing(sw::TRANSPARENCY_NONE);
					renderer->setAlphaTestEnable(false);
				}
				else
				{
					renderer->setPointSize((float&)value);
				}
				break;
			case D3DRS_POINTSIZE_MIN:
				renderer->setPointSizeMin((float&)value);
				break;
			case D3DRS_POINTSPRITEENABLE:
				renderer->setPointSpriteEnable(value != FALSE);
				break;
			case D3DRS_POINTSCALEENABLE:
				renderer->setPointScaleEnable(value != FALSE);
				break;
			case D3DRS_POINTSCALE_A:
				renderer->setPointScaleA((float&)value);
				break;
			case D3DRS_POINTSCALE_B:
				renderer->setPointScaleB((float&)value);
				break;
			case D3DRS_POINTSCALE_C:
				renderer->setPointScaleC((float&)value);
				break;
			case D3DRS_MULTISAMPLEANTIALIAS:
			//	if(!init) UNIMPLEMENTED();
				break;
			case D3DRS_MULTISAMPLEMASK:
				SetRenderTarget(0, renderTarget[0]);   // Sets the multi-sample mask, if maskable
				break;
			case D3DRS_PATCHEDGESTYLE:
				if(!init) if(value != D3DPATCHEDGE_DISCRETE) UNIMPLEMENTED();
				break;
			case D3DRS_DEBUGMONITORTOKEN:
				if(!init) UNIMPLEMENTED();
				break;
			case D3DRS_POINTSIZE_MAX:
				renderer->setPointSizeMax((float&)value);
				break;
			case D3DRS_INDEXEDVERTEXBLENDENABLE:
				renderer->setIndexedVertexBlendEnable(value != FALSE);
				break;
			case D3DRS_COLORWRITEENABLE:
				renderer->setColorWriteMask(0, value & 0x0000000F);
				break;
			case D3DRS_TWEENFACTOR:
				if(!init) UNIMPLEMENTED();
				break;
			case D3DRS_BLENDOP:
				switch(value)
				{
				case D3DBLENDOP_ADD:
					renderer->setBlendOperation(sw::BLENDOP_ADD);
					break;
				case D3DBLENDOP_SUBTRACT:
					renderer->setBlendOperation(sw::BLENDOP_SUB);
					break;
				case D3DBLENDOP_REVSUBTRACT:
					renderer->setBlendOperation(sw::BLENDOP_INVSUB);
					break;
				case D3DBLENDOP_MIN:
					renderer->setBlendOperation(sw::BLENDOP_MIN);
					break;
				case D3DBLENDOP_MAX:
					renderer->setBlendOperation(sw::BLENDOP_MAX);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_POSITIONDEGREE:
				if(!init) UNIMPLEMENTED();
				break;
			case D3DRS_NORMALDEGREE:
				if(!init) UNIMPLEMENTED();
				break;
			case D3DRS_SCISSORTESTENABLE:
				scissorEnable = (value != FALSE);
				break;
			case D3DRS_SLOPESCALEDEPTHBIAS:
				renderer->setSlopeDepthBias((float&)value);
				break;
			case D3DRS_ANTIALIASEDLINEENABLE:
				if(!init) if(value != FALSE) UNIMPLEMENTED();
				break;
			case D3DRS_MINTESSELLATIONLEVEL:
				if(!init) UNIMPLEMENTED();
				break;
			case D3DRS_MAXTESSELLATIONLEVEL:
				if(!init) UNIMPLEMENTED();
				break;
			case D3DRS_ADAPTIVETESS_X:
				if(!init) if((float&)value != 0.0f) UNIMPLEMENTED();
				break;
			case D3DRS_ADAPTIVETESS_Y:
				if(value == D3DFMT_ATOC)   // NVIDIA hack to enable transparency anti-aliasing
				{
					renderer->setTransparencyAntialiasing(sw::TRANSPARENCY_ALPHA_TO_COVERAGE);
				}
				else if(value == D3DFMT_UNKNOWN)   // NVIDIA hack to disable transparency anti-aliasing
				{
					renderer->setTransparencyAntialiasing(sw::TRANSPARENCY_NONE);
				}
				else
				{
					if(!init) if((float&)value != 0.0f) UNIMPLEMENTED();
				}
				break;
			case D3DRS_ADAPTIVETESS_Z:
				if(!init) if((float&)value != 1.0f) UNIMPLEMENTED();
				break;
			case D3DRS_ADAPTIVETESS_W:
				if(!init) if((float&)value != 0.0f) UNIMPLEMENTED();
				break;
			case D3DRS_ENABLEADAPTIVETESSELLATION:
				if(!init) UNIMPLEMENTED();
				break;
			case D3DRS_TWOSIDEDSTENCILMODE:
				renderer->setTwoSidedStencil(value != FALSE);
				break;
			case D3DRS_CCW_STENCILFAIL:
				switch(value)
				{
				case D3DSTENCILOP_KEEP:
					renderer->setStencilFailOperationCCW(sw::OPERATION_KEEP);
					break;
				case D3DSTENCILOP_ZERO:
					renderer->setStencilFailOperationCCW(sw::OPERATION_ZERO);
					break;
				case D3DSTENCILOP_REPLACE:
					renderer->setStencilFailOperationCCW(sw::OPERATION_REPLACE);
					break;
				case D3DSTENCILOP_INCRSAT:
					renderer->setStencilFailOperationCCW(sw::OPERATION_INCRSAT);
					break;
				case D3DSTENCILOP_DECRSAT:
					renderer->setStencilFailOperationCCW(sw::OPERATION_DECRSAT);
					break;
				case D3DSTENCILOP_INVERT:
					renderer->setStencilFailOperationCCW(sw::OPERATION_INVERT);
					break;
				case D3DSTENCILOP_INCR:
					renderer->setStencilFailOperationCCW(sw::OPERATION_INCR);
					break;
				case D3DSTENCILOP_DECR:
					renderer->setStencilFailOperationCCW(sw::OPERATION_DECR);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_CCW_STENCILZFAIL:
				switch(value)
				{
				case D3DSTENCILOP_KEEP:
					renderer->setStencilZFailOperationCCW(sw::OPERATION_KEEP);
					break;
				case D3DSTENCILOP_ZERO:
					renderer->setStencilZFailOperationCCW(sw::OPERATION_ZERO);
					break;
				case D3DSTENCILOP_REPLACE:
					renderer->setStencilZFailOperationCCW(sw::OPERATION_REPLACE);
					break;
				case D3DSTENCILOP_INCRSAT:
					renderer->setStencilZFailOperationCCW(sw::OPERATION_INCRSAT);
					break;
				case D3DSTENCILOP_DECRSAT:
					renderer->setStencilZFailOperationCCW(sw::OPERATION_DECRSAT);
					break;
				case D3DSTENCILOP_INVERT:
					renderer->setStencilZFailOperationCCW(sw::OPERATION_INVERT);
					break;
				case D3DSTENCILOP_INCR:
					renderer->setStencilZFailOperationCCW(sw::OPERATION_INCR);
					break;
				case D3DSTENCILOP_DECR:
					renderer->setStencilZFailOperationCCW(sw::OPERATION_DECR);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_CCW_STENCILPASS:
				switch(value)
				{
				case D3DSTENCILOP_KEEP:
					renderer->setStencilPassOperationCCW(sw::OPERATION_KEEP);
					break;
				case D3DSTENCILOP_ZERO:
					renderer->setStencilPassOperationCCW(sw::OPERATION_ZERO);
					break;
				case D3DSTENCILOP_REPLACE:
					renderer->setStencilPassOperationCCW(sw::OPERATION_REPLACE);
					break;
				case D3DSTENCILOP_INCRSAT:
					renderer->setStencilPassOperationCCW(sw::OPERATION_INCRSAT);
					break;
				case D3DSTENCILOP_DECRSAT:
					renderer->setStencilPassOperationCCW(sw::OPERATION_DECRSAT);
					break;
				case D3DSTENCILOP_INVERT:
					renderer->setStencilPassOperationCCW(sw::OPERATION_INVERT);
					break;
				case D3DSTENCILOP_INCR:
					renderer->setStencilPassOperationCCW(sw::OPERATION_INCR);
					break;
				case D3DSTENCILOP_DECR:
					renderer->setStencilPassOperationCCW(sw::OPERATION_DECR);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_CCW_STENCILFUNC:
				switch(value)
				{
				case D3DCMP_NEVER:
					renderer->setStencilCompareCCW(sw::STENCIL_NEVER);
					break;
				case D3DCMP_LESS:
					renderer->setStencilCompareCCW(sw::STENCIL_LESS);
					break;
				case D3DCMP_EQUAL:
					renderer->setStencilCompareCCW(sw::STENCIL_EQUAL);
					break;
				case D3DCMP_LESSEQUAL:
					renderer->setStencilCompareCCW(sw::STENCIL_LESSEQUAL);
					break;
				case D3DCMP_GREATER:
					renderer->setStencilCompareCCW(sw::STENCIL_GREATER);
					break;
				case D3DCMP_NOTEQUAL:
					renderer->setStencilCompareCCW(sw::STENCIL_NOTEQUAL);
					break;
				case D3DCMP_GREATEREQUAL:
					renderer->setStencilCompareCCW(sw::STENCIL_GREATEREQUAL);
					break;
				case D3DCMP_ALWAYS:
					renderer->setStencilCompareCCW(sw::STENCIL_ALWAYS);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_COLORWRITEENABLE1:
				renderer->setColorWriteMask(1, value);
				break;
			case D3DRS_COLORWRITEENABLE2:
				renderer->setColorWriteMask(2, value);
				break;
			case D3DRS_COLORWRITEENABLE3:
				renderer->setColorWriteMask(3, value);
				break;
			case D3DRS_BLENDFACTOR:
				renderer->setBlendConstant(sw::Color<float>(value));
				break;
			case D3DRS_SRGBWRITEENABLE:
				renderer->setWriteSRGB(value != FALSE);
				break;
			case D3DRS_DEPTHBIAS:
				renderer->setDepthBias((float&)value);
				break;
			case D3DRS_WRAP8:
				renderer->setTextureWrap(8, value);
				break;
			case D3DRS_WRAP9:
				renderer->setTextureWrap(9, value);
				break;
			case D3DRS_WRAP10:
				renderer->setTextureWrap(10, value);
				break;
			case D3DRS_WRAP11:
				renderer->setTextureWrap(11, value);
				break;
			case D3DRS_WRAP12:
				renderer->setTextureWrap(12, value);
				break;
			case D3DRS_WRAP13:
				renderer->setTextureWrap(13, value);
				break;
			case D3DRS_WRAP14:
				renderer->setTextureWrap(14, value);
				break;
			case D3DRS_WRAP15:
				renderer->setTextureWrap(15, value);
				break;
			case D3DRS_SEPARATEALPHABLENDENABLE:
				renderer->setSeparateAlphaBlendEnable(value != FALSE);
				break;
			case D3DRS_SRCBLENDALPHA:
				switch(value)
				{
				case D3DBLEND_ZERO:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_ZERO);
					break;
				case D3DBLEND_ONE:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_ONE);
					break;
				case D3DBLEND_SRCCOLOR:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_SOURCE);
					break;
				case D3DBLEND_INVSRCCOLOR:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_INVSOURCE);
					break;
				case D3DBLEND_SRCALPHA:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_SOURCEALPHA);
					break;
				case D3DBLEND_INVSRCALPHA:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_INVSOURCEALPHA);
					break;
				case D3DBLEND_DESTALPHA:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_DESTALPHA);
					break;
				case D3DBLEND_INVDESTALPHA:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_INVDESTALPHA);
					break;
				case D3DBLEND_DESTCOLOR:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_DEST);
					break;
				case D3DBLEND_INVDESTCOLOR:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_INVDEST);
					break;
				case D3DBLEND_SRCALPHASAT:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_SRCALPHASAT);
					break;
				case D3DBLEND_BOTHSRCALPHA:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_SOURCEALPHA);
					renderer->setDestBlendFactorAlpha(sw::BLEND_INVSOURCEALPHA);
					break;
				case D3DBLEND_BOTHINVSRCALPHA:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_INVSOURCEALPHA);
					renderer->setDestBlendFactorAlpha(sw::BLEND_SOURCEALPHA);
					break;
				case D3DBLEND_BLENDFACTOR:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_CONSTANT);
					break;
				case D3DBLEND_INVBLENDFACTOR:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_INVCONSTANT);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_DESTBLENDALPHA:
				switch(value)
				{
				case D3DBLEND_ZERO:
					renderer->setDestBlendFactorAlpha(sw::BLEND_ZERO);
					break;
				case D3DBLEND_ONE:
					renderer->setDestBlendFactorAlpha(sw::BLEND_ONE);
					break;
				case D3DBLEND_SRCCOLOR:
					renderer->setDestBlendFactorAlpha(sw::BLEND_SOURCE);
					break;
				case D3DBLEND_INVSRCCOLOR:
					renderer->setDestBlendFactorAlpha(sw::BLEND_INVSOURCE);
					break;
				case D3DBLEND_SRCALPHA:
					renderer->setDestBlendFactorAlpha(sw::BLEND_SOURCEALPHA);
					break;
				case D3DBLEND_INVSRCALPHA:
					renderer->setDestBlendFactorAlpha(sw::BLEND_INVSOURCEALPHA);
					break;
				case D3DBLEND_DESTALPHA:
					renderer->setDestBlendFactorAlpha(sw::BLEND_DESTALPHA);
					break;
				case D3DBLEND_INVDESTALPHA:
					renderer->setDestBlendFactorAlpha(sw::BLEND_INVDESTALPHA);
					break;
				case D3DBLEND_DESTCOLOR:
					renderer->setDestBlendFactorAlpha(sw::BLEND_DEST);
					break;
				case D3DBLEND_INVDESTCOLOR:
					renderer->setDestBlendFactorAlpha(sw::BLEND_INVDEST);
					break;
				case D3DBLEND_SRCALPHASAT:
					renderer->setDestBlendFactorAlpha(sw::BLEND_SRCALPHASAT);
					break;
				case D3DBLEND_BOTHSRCALPHA:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_SOURCEALPHA);
					renderer->setDestBlendFactorAlpha(sw::BLEND_INVSOURCEALPHA);
					break;
				case D3DBLEND_BOTHINVSRCALPHA:
					renderer->setSourceBlendFactorAlpha(sw::BLEND_INVSOURCEALPHA);
					renderer->setDestBlendFactorAlpha(sw::BLEND_SOURCEALPHA);
					break;
				case D3DBLEND_BLENDFACTOR:
					renderer->setDestBlendFactorAlpha(sw::BLEND_CONSTANT);
					break;
				case D3DBLEND_INVBLENDFACTOR:
					renderer->setDestBlendFactorAlpha(sw::BLEND_INVCONSTANT);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_BLENDOPALPHA:
				switch(value)
				{
				case D3DBLENDOP_ADD:
					renderer->setBlendOperationAlpha(sw::BLENDOP_ADD);
					break;
				case D3DBLENDOP_SUBTRACT:
					renderer->setBlendOperationAlpha(sw::BLENDOP_SUB);
					break;
				case D3DBLENDOP_REVSUBTRACT:
					renderer->setBlendOperationAlpha(sw::BLENDOP_INVSUB);
					break;
				case D3DBLENDOP_MIN:
					renderer->setBlendOperationAlpha(sw::BLENDOP_MIN);
					break;
				case D3DBLENDOP_MAX:
					renderer->setBlendOperationAlpha(sw::BLENDOP_MAX);
					break;
				default:
					ASSERT(false);
				}
				break;
			default:
				ASSERT(false);
			}
		}
		else   // stateRecorder
		{
			stateRecorder->setRenderState(state, value);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetRenderTarget(unsigned long index, IDirect3DSurface9 *iRenderTarget)
	{
		CriticalSection cs(this);

		TRACE("unsigned long index = %d, IDirect3DSurface9 *newRenderTarget = 0x%0.8p", index, iRenderTarget);

		// FIXME: Check for D3DUSAGE_RENDERTARGET

		if(index >= 4 || (index == 0 && !iRenderTarget))
		{
			return INVALIDCALL();
		}

		Direct3DSurface9 *renderTarget = static_cast<Direct3DSurface9*>(iRenderTarget);

		if(renderTarget)
		{
			renderTarget->bind();
		}

		if(this->renderTarget[index])
		{
			this->renderTarget[index]->unbind();
		}

		this->renderTarget[index] = renderTarget;

		if(renderTarget && index == 0)
		{
			D3DSURFACE_DESC renderTargetDesc;
			renderTarget->GetDesc(&renderTargetDesc);

			// Reset viewport to size of current render target
			viewport.X = 0;
			viewport.Y = 0;
			viewport.Width = renderTargetDesc.Width;
			viewport.Height = renderTargetDesc.Height;
			viewport.MinZ = 0;
			viewport.MaxZ = 1;

			// Reset scissor rectangle to size of current render target
			scissorRect.left = 0;
			scissorRect.top = 0;
			scissorRect.right = renderTargetDesc.Width;
			scissorRect.bottom = renderTargetDesc.Height;

			// Set the multi-sample mask, if maskable
			if(renderTargetDesc.MultiSampleType != D3DMULTISAMPLE_NONE &&
			   renderTargetDesc.MultiSampleType != D3DMULTISAMPLE_NONMASKABLE)
			{
				renderer->setMultiSampleMask(renderState[D3DRS_MULTISAMPLEMASK]);
			}
			else
			{
				renderer->setMultiSampleMask(0xFFFFFFFF);
			}
		}

		renderer->setRenderTarget(index, renderTarget);

		return D3D_OK;
	}

	long Direct3DDevice9::SetSamplerState(unsigned long sampler, D3DSAMPLERSTATETYPE state, unsigned long value)
	{
		CriticalSection cs(this);

		TRACE("unsigned long sampler = %d, D3DSAMPLERSTATETYPE state = %d, unsigned long value = %d", sampler, state, value);

		if(state < D3DSAMP_ADDRESSU || state > D3DSAMP_DMAPOFFSET)
		{
			return INVALIDCALL();
		}

		if((sampler >= 16 && sampler <= D3DDMAPSAMPLER) || sampler > D3DVERTEXTEXTURESAMPLER3)
		{
			return INVALIDCALL();
		}

		if(sampler >= D3DVERTEXTEXTURESAMPLER0)
		{
			sampler = 16 + (sampler - D3DVERTEXTEXTURESAMPLER0);
		}

		if(!stateRecorder)
		{
			if(!init && samplerState[sampler][state] == value)
			{
				return D3D_OK;
			}

			samplerState[sampler][state] = value;

			sw::SamplerType type = sampler < 16 ? sw::SAMPLER_PIXEL : sw::SAMPLER_VERTEX;
			int index = sampler < 16 ? sampler : sampler - 16;   // Sampler index within type group

			switch(state)
			{
			case D3DSAMP_ADDRESSU:
				switch(value)
				{
				case D3DTADDRESS_WRAP:
					renderer->setAddressingModeU(type, index, sw::ADDRESSING_WRAP);
					break;
				case D3DTADDRESS_MIRROR:
					renderer->setAddressingModeU(type, index, sw::ADDRESSING_MIRROR);
					break;
				case D3DTADDRESS_CLAMP:
					renderer->setAddressingModeU(type, index, sw::ADDRESSING_CLAMP);
					break;
				case D3DTADDRESS_BORDER:
					renderer->setAddressingModeU(type, index, sw::ADDRESSING_BORDER);
					break;
				case D3DTADDRESS_MIRRORONCE:
					renderer->setAddressingModeU(type, index, sw::ADDRESSING_MIRRORONCE);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DSAMP_ADDRESSV:
				switch(value)
				{
				case D3DTADDRESS_WRAP:
					renderer->setAddressingModeV(type, index, sw::ADDRESSING_WRAP);
					break;
				case D3DTADDRESS_MIRROR:
					renderer->setAddressingModeV(type, index, sw::ADDRESSING_MIRROR);
					break;
				case D3DTADDRESS_CLAMP:
					renderer->setAddressingModeV(type, index, sw::ADDRESSING_CLAMP);
					break;
				case D3DTADDRESS_BORDER:
					renderer->setAddressingModeV(type, index, sw::ADDRESSING_BORDER);
					break;
				case D3DTADDRESS_MIRRORONCE:
					renderer->setAddressingModeV(type, index, sw::ADDRESSING_MIRRORONCE);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DSAMP_ADDRESSW:
				switch(value)
				{
				case D3DTADDRESS_WRAP:
					renderer->setAddressingModeW(type, index, sw::ADDRESSING_WRAP);
					break;
				case D3DTADDRESS_MIRROR:
					renderer->setAddressingModeW(type, index, sw::ADDRESSING_MIRROR);
					break;
				case D3DTADDRESS_CLAMP:
					renderer->setAddressingModeW(type, index, sw::ADDRESSING_CLAMP);
					break;
				case D3DTADDRESS_BORDER:
					renderer->setAddressingModeW(type, index, sw::ADDRESSING_BORDER);
					break;
				case D3DTADDRESS_MIRRORONCE:
					renderer->setAddressingModeW(type, index, sw::ADDRESSING_MIRRORONCE);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DSAMP_BORDERCOLOR:
				renderer->setBorderColor(type, index, value);
				break;
			case D3DSAMP_MAGFILTER:
				// NOTE: SwiftShader does not differentiate between minification and magnification filter
				switch(value)
				{
				case D3DTEXF_NONE:
					renderer->setTextureFilter(type, index, sw::FILTER_POINT);   // FIXME: Only for mipmap filter
					break;
				case D3DTEXF_POINT:
					renderer->setTextureFilter(type, index, sw::FILTER_POINT);
					break;
				case D3DTEXF_LINEAR:
					renderer->setTextureFilter(type, index, sw::FILTER_LINEAR);
					break;
				case D3DTEXF_ANISOTROPIC:
					renderer->setTextureFilter(type, index, sw::FILTER_ANISOTROPIC);
					break;
				case D3DTEXF_PYRAMIDALQUAD:
					renderer->setTextureFilter(type, index, sw::FILTER_LINEAR);   // FIXME: Unimplemented, fail silently
					break;
				case D3DTEXF_GAUSSIANQUAD:
					renderer->setTextureFilter(type, index, sw::FILTER_LINEAR);   // FIXME: Unimplemented, fail silently
					break;
				default:
					return INVALIDCALL();
				};
				break;
			case D3DSAMP_MINFILTER:
				// NOTE: SwiftShader does not differentiate between minification and magnification filter
				switch(value)
				{
				case D3DTEXF_NONE:
					renderer->setTextureFilter(type, index, sw::FILTER_POINT);   // FIXME: Only for mipmap filter
					break;
				case D3DTEXF_POINT:
					renderer->setTextureFilter(type, index, sw::FILTER_POINT);
					break;
				case D3DTEXF_LINEAR:
					renderer->setTextureFilter(type, index, sw::FILTER_LINEAR);
					break;
				case D3DTEXF_ANISOTROPIC:
					renderer->setTextureFilter(type, index, sw::FILTER_ANISOTROPIC);
					break;
				case D3DTEXF_PYRAMIDALQUAD:
					renderer->setTextureFilter(type, index, sw::FILTER_LINEAR);   // FIXME: Unimplemented, fail silently
					break;
				case D3DTEXF_GAUSSIANQUAD:
					renderer->setTextureFilter(type, index, sw::FILTER_LINEAR);   // FIXME: Unimplemented, fail silently
					break;
				default:
					return INVALIDCALL();
				};
				break;
			case D3DSAMP_MIPFILTER:
				switch(value)
				{
				case D3DTEXF_NONE:
					renderer->setMipmapFilter(type, index, sw::MIPMAP_NONE);
					break;
				case D3DTEXF_POINT:
					renderer->setMipmapFilter(type, index, sw::MIPMAP_POINT);
					break;
				case D3DTEXF_LINEAR:
					renderer->setMipmapFilter(type, index, sw::MIPMAP_LINEAR);
					break;
				case D3DTEXF_ANISOTROPIC:
					renderer->setMipmapFilter(type, index, sw::MIPMAP_LINEAR);   // FIXME: Only for texture filter
					break;
				case D3DTEXF_PYRAMIDALQUAD:
					renderer->setMipmapFilter(type, index, sw::MIPMAP_LINEAR);   // FIXME: Only for texture filter
					break;
				case D3DTEXF_GAUSSIANQUAD:
					renderer->setMipmapFilter(type, index, sw::MIPMAP_LINEAR);   // FIXME: Only for texture filter
					break;
				default:
					return INVALIDCALL();
				};
				break;
			case D3DSAMP_MIPMAPLODBIAS:
				if(value == D3DFMT_GET4)   // ATI hack to enable Fetch4
				{
					renderer->setGatherEnable(type, index, true);
				}
				else if(value == D3DFMT_GET1)   // ATI hack to disable Fetch4
				{
					renderer->setGatherEnable(type, index, false);
				}
				else
				{
					float LOD = (float&)value - sw::log2((float)context->renderTarget[0]->getSuperSampleCount());   // FIXME: Update when render target changes
					renderer->setMipmapLOD(type, index, LOD);
				}
				break;
			case D3DSAMP_MAXMIPLEVEL:
				break;
			case D3DSAMP_MAXANISOTROPY:
				renderer->setMaxAnisotropy(type, index, sw::clamp((unsigned int)value, (unsigned int)1, maxAnisotropy));
				break;
			case D3DSAMP_SRGBTEXTURE:
				renderer->setReadSRGB(type, index, value != FALSE);
				break;
			case D3DSAMP_ELEMENTINDEX:
				if(!init) UNIMPLEMENTED();   // Multi-element textures deprecated in favor of multiple render targets
				break;
			case D3DSAMP_DMAPOFFSET:
			//	if(!init) UNIMPLEMENTED();
				break;
			default:
				ASSERT(false);
			}
		}
		else   // stateRecorder
		{
			stateRecorder->setSamplerState(sampler, state, value);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetScissorRect(const RECT *rect)
	{
		CriticalSection cs(this);

		TRACE("const RECT *rect = 0x%0.8p", rect);

		if(!rect)
		{
			return INVALIDCALL();
		}

		if(!stateRecorder)
		{
			scissorRect = *rect;
		}
		else
		{
			stateRecorder->setScissorRect(rect);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetSoftwareVertexProcessing(int software)
	{
		CriticalSection cs(this);

		TRACE("int software = %d", software);

		if(behaviourFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING && software == FALSE)
		{
			return INVALIDCALL();
		}

		if(behaviourFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING && software == TRUE)
		{
			return INVALIDCALL();
		}

		softwareVertexProcessing = (software != FALSE);

		return D3D_OK;
	}

	long Direct3DDevice9::SetStreamSource(unsigned int stream, IDirect3DVertexBuffer9 *iVertexBuffer, unsigned int offset, unsigned int stride)
	{
		CriticalSection cs(this);

		TRACE("unsigned int stream = %d, IDirect3DVertexBuffer9 *data = 0x%0.8p, unsigned int offset = %d, unsigned int stride = %d", stream, iVertexBuffer, offset, stride);

		Direct3DVertexBuffer9 *vertexBuffer = static_cast<Direct3DVertexBuffer9*>(iVertexBuffer);

		if(!stateRecorder)
		{
			if(dataStream[stream] == vertexBuffer && streamOffset[stream] == offset && streamStride[stream] == stride)
			{
				return D3D_OK;
			}

			if(vertexBuffer)
			{
				vertexBuffer->bind();
			}

			if(dataStream[stream])
			{
				dataStream[stream]->unbind();
			}

			dataStream[stream] = vertexBuffer;
			streamOffset[stream] = offset;
			streamStride[stream] = stride;
		}
		else
		{
			stateRecorder->setStreamSource(stream, vertexBuffer, offset, stride);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetStreamSourceFreq(unsigned int streamNumber, unsigned int divider)
	{
		CriticalSection cs(this);

		TRACE("unsigned int streamNumber = %d, unsigned int divider = %d", streamNumber, divider);

		if(!instancingEnabled)
		{
			return INVALIDCALL();
		}

		if(!stateRecorder)
		{
			streamSourceFreq[streamNumber] = divider;
		}
		else
		{
			stateRecorder->setStreamSourceFreq(streamNumber, divider);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetTexture(unsigned long sampler, IDirect3DBaseTexture9 *iBaseTexture)
	{
		CriticalSection cs(this);

		TRACE("unsigned long sampler = %d, IDirect3DBaseTexture9 *texture = 0x%0.8p", sampler, iBaseTexture);

		if((sampler >= 16 && sampler <= D3DDMAPSAMPLER) || sampler > D3DVERTEXTEXTURESAMPLER3)
		{
			return INVALIDCALL();
		}

		if(sampler >= D3DVERTEXTEXTURESAMPLER0)
		{
			sampler = 16 + (sampler - D3DVERTEXTEXTURESAMPLER0);
		}

		Direct3DBaseTexture9 *baseTexture = dynamic_cast<Direct3DBaseTexture9*>(iBaseTexture);

		if(!stateRecorder)
		{
			if(texture[sampler] == baseTexture)
			{
				return D3D_OK;
			}

			if(baseTexture)
			{
				baseTexture->bind();   // FIXME: Bind individual sub-surfaces?
			}

			if(texture[sampler])
			{
				texture[sampler]->unbind();
			}

			texture[sampler] = baseTexture;
		}
		else
		{
			stateRecorder->setTexture(sampler, baseTexture);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type, unsigned long value)
	{
		CriticalSection cs(this);

		TRACE("unsigned long stage = %d, D3DTEXTURESTAGESTATETYPE type = %d, unsigned long value = %d", stage, type, value);

		if(stage < 0 || stage >= 8 || type < D3DTSS_COLOROP || type > D3DTSS_CONSTANT)
		{
			return INVALIDCALL();
		}

		if(!stateRecorder)
		{
			if(!init && textureStageState[stage][type] == value)
			{
				return D3D_OK;
			}

			textureStageState[stage][type] = value;

			switch(type)
			{
			case D3DTSS_COLOROP:
				switch(value)
				{
				case D3DTOP_DISABLE:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_DISABLE);
					break;
				case D3DTOP_SELECTARG1:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_SELECTARG1);
					break;
				case D3DTOP_SELECTARG2:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_SELECTARG2);
					break;
				case D3DTOP_MODULATE:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_MODULATE);
					break;
				case D3DTOP_MODULATE2X:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_MODULATE2X);
					break;
				case D3DTOP_MODULATE4X:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_MODULATE4X);
					break;
				case D3DTOP_ADD:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_ADD);
					break;
				case D3DTOP_ADDSIGNED:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_ADDSIGNED);
					break;
				case D3DTOP_ADDSIGNED2X:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_ADDSIGNED2X);
					break;
				case D3DTOP_SUBTRACT:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_SUBTRACT);
					break;
				case D3DTOP_ADDSMOOTH:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_ADDSMOOTH);
					break;
				case D3DTOP_BLENDDIFFUSEALPHA:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_BLENDDIFFUSEALPHA);
					break;
				case D3DTOP_BLENDTEXTUREALPHA:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_BLENDTEXTUREALPHA);
					break;
				case D3DTOP_BLENDFACTORALPHA:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_BLENDFACTORALPHA);
					break;
				case D3DTOP_BLENDTEXTUREALPHAPM:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_BLENDTEXTUREALPHAPM);
					break;
				case D3DTOP_BLENDCURRENTALPHA:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_BLENDCURRENTALPHA);
					break;
				case D3DTOP_PREMODULATE:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_PREMODULATE);
					break;
				case D3DTOP_MODULATEALPHA_ADDCOLOR:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_MODULATEALPHA_ADDCOLOR);
					break;
				case D3DTOP_MODULATECOLOR_ADDALPHA:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_MODULATECOLOR_ADDALPHA);
					break;
				case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_MODULATEINVALPHA_ADDCOLOR);
					break;
				case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_MODULATEINVCOLOR_ADDALPHA);
					break;
				case D3DTOP_BUMPENVMAP:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_BUMPENVMAP);
					break;
				case D3DTOP_BUMPENVMAPLUMINANCE:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_BUMPENVMAPLUMINANCE);
					break;
				case D3DTOP_DOTPRODUCT3:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_DOT3);
					break;
				case D3DTOP_MULTIPLYADD:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_MULTIPLYADD);
					break;
				case D3DTOP_LERP:
					renderer->setStageOperation(stage, sw::TextureStage::STAGE_LERP);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DTSS_COLORARG1:
				switch(value & D3DTA_SELECTMASK)
				{
				case D3DTA_DIFFUSE:
					renderer->setFirstArgument(stage, sw::TextureStage::SOURCE_DIFFUSE);
					break;
				case D3DTA_CURRENT:
					renderer->setFirstArgument(stage, sw::TextureStage::SOURCE_CURRENT);
					break;
				case D3DTA_TEXTURE:
					renderer->setFirstArgument(stage, sw::TextureStage::SOURCE_TEXTURE);
					break;
				case D3DTA_TFACTOR:
					renderer->setFirstArgument(stage, sw::TextureStage::SOURCE_TFACTOR);
					break;
				case D3DTA_SPECULAR:
					renderer->setFirstArgument(stage, sw::TextureStage::SOURCE_SPECULAR);
					break;
				case D3DTA_TEMP:
					renderer->setFirstArgument(stage, sw::TextureStage::SOURCE_TEMP);
					break;
				case D3DTA_CONSTANT:
					renderer->setFirstArgument(stage, sw::TextureStage::SOURCE_CONSTANT);
					break;
				default:
					ASSERT(false);
				}

				switch(value & ~D3DTA_SELECTMASK)
				{
				case 0:
					renderer->setFirstModifier(stage, sw::TextureStage::MODIFIER_COLOR);
					break;
				case D3DTA_COMPLEMENT:
					renderer->setFirstModifier(stage, sw::TextureStage::MODIFIER_INVCOLOR);
					break;
				case D3DTA_ALPHAREPLICATE:
					renderer->setFirstModifier(stage, sw::TextureStage::MODIFIER_ALPHA);
					break;
				case D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE:
					renderer->setFirstModifier(stage, sw::TextureStage::MODIFIER_INVALPHA);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DTSS_COLORARG2:
				switch(value & D3DTA_SELECTMASK)
				{
				case D3DTA_DIFFUSE:
					renderer->setSecondArgument(stage, sw::TextureStage::SOURCE_DIFFUSE);
					break;
				case D3DTA_CURRENT:
					renderer->setSecondArgument(stage, sw::TextureStage::SOURCE_CURRENT);
					break;
				case D3DTA_TEXTURE:
					renderer->setSecondArgument(stage, sw::TextureStage::SOURCE_TEXTURE);
					break;
				case D3DTA_TFACTOR:
					renderer->setSecondArgument(stage, sw::TextureStage::SOURCE_TFACTOR);
					break;
				case D3DTA_SPECULAR:
					renderer->setSecondArgument(stage, sw::TextureStage::SOURCE_SPECULAR);
					break;
				case D3DTA_TEMP:
					renderer->setSecondArgument(stage, sw::TextureStage::SOURCE_TEMP);
					break;
				case D3DTA_CONSTANT:
					renderer->setSecondArgument(stage, sw::TextureStage::SOURCE_CONSTANT);
					break;
				default:
					ASSERT(false);
				}

				switch(value & ~D3DTA_SELECTMASK)
				{
				case 0:
					renderer->setSecondModifier(stage, sw::TextureStage::MODIFIER_COLOR);
					break;
				case D3DTA_COMPLEMENT:
					renderer->setSecondModifier(stage, sw::TextureStage::MODIFIER_INVCOLOR);
					break;
				case D3DTA_ALPHAREPLICATE:
					renderer->setSecondModifier(stage, sw::TextureStage::MODIFIER_ALPHA);
					break;
				case D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE:
					renderer->setSecondModifier(stage, sw::TextureStage::MODIFIER_INVALPHA);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DTSS_ALPHAOP:
				switch(value)
				{
				case D3DTOP_DISABLE:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_DISABLE);
					break;
				case D3DTOP_SELECTARG1:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_SELECTARG1);
					break;
				case D3DTOP_SELECTARG2:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_SELECTARG2);
					break;
				case D3DTOP_MODULATE:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_MODULATE);
					break;
				case D3DTOP_MODULATE2X:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_MODULATE2X);
					break;
				case D3DTOP_MODULATE4X:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_MODULATE4X);
					break;
				case D3DTOP_ADD:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_ADD);
					break;
				case D3DTOP_ADDSIGNED:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_ADDSIGNED);
					break;
				case D3DTOP_ADDSIGNED2X:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_ADDSIGNED2X);
					break;
				case D3DTOP_SUBTRACT:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_SUBTRACT);
					break;
				case D3DTOP_ADDSMOOTH:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_ADDSMOOTH);
					break;
				case D3DTOP_BLENDDIFFUSEALPHA:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_BLENDDIFFUSEALPHA);
					break;
				case D3DTOP_BLENDTEXTUREALPHA:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_BLENDTEXTUREALPHA);
					break;
				case D3DTOP_BLENDFACTORALPHA:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_BLENDFACTORALPHA);
					break;
				case D3DTOP_BLENDTEXTUREALPHAPM:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_BLENDTEXTUREALPHAPM);
					break;
				case D3DTOP_BLENDCURRENTALPHA:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_BLENDCURRENTALPHA);
					break;
				case D3DTOP_PREMODULATE:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_PREMODULATE);
					break;
				case D3DTOP_MODULATEALPHA_ADDCOLOR:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_MODULATEALPHA_ADDCOLOR);
					break;
				case D3DTOP_MODULATECOLOR_ADDALPHA:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_MODULATECOLOR_ADDALPHA);
					break;
				case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_MODULATEINVALPHA_ADDCOLOR);
					break;
				case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_MODULATEINVCOLOR_ADDALPHA);
					break;
				case D3DTOP_BUMPENVMAP:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_BUMPENVMAP);
					break;
				case D3DTOP_BUMPENVMAPLUMINANCE:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_BUMPENVMAPLUMINANCE);
					break;
				case D3DTOP_DOTPRODUCT3:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_DOT3);
					break;
				case D3DTOP_MULTIPLYADD:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_MULTIPLYADD);
					break;
				case D3DTOP_LERP:
					renderer->setStageOperationAlpha(stage, sw::TextureStage::STAGE_LERP);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DTSS_ALPHAARG1:
				switch(value & D3DTA_SELECTMASK)
				{
				case D3DTA_DIFFUSE:
					renderer->setFirstArgumentAlpha(stage, sw::TextureStage::SOURCE_DIFFUSE);
					break;
				case D3DTA_CURRENT:
					renderer->setFirstArgumentAlpha(stage, sw::TextureStage::SOURCE_CURRENT);
					break;
				case D3DTA_TEXTURE:
					renderer->setFirstArgumentAlpha(stage, sw::TextureStage::SOURCE_TEXTURE);
					break;
				case D3DTA_TFACTOR:
					renderer->setFirstArgumentAlpha(stage, sw::TextureStage::SOURCE_TFACTOR);
					break;
				case D3DTA_SPECULAR:
					renderer->setFirstArgumentAlpha(stage, sw::TextureStage::SOURCE_SPECULAR);
					break;
				case D3DTA_TEMP:
					renderer->setFirstArgumentAlpha(stage, sw::TextureStage::SOURCE_TEMP);
					break;
				case D3DTA_CONSTANT:
					renderer->setFirstArgumentAlpha(stage, sw::TextureStage::SOURCE_CONSTANT);
					break;
				default:
					ASSERT(false);
				}

				switch(value & ~D3DTA_SELECTMASK)
				{
				case 0:
					renderer->setFirstModifierAlpha(stage, sw::TextureStage::MODIFIER_COLOR);
					break;
				case D3DTA_COMPLEMENT:
					renderer->setFirstModifierAlpha(stage, sw::TextureStage::MODIFIER_INVCOLOR);
					break;
				case D3DTA_ALPHAREPLICATE:
					renderer->setFirstModifierAlpha(stage, sw::TextureStage::MODIFIER_ALPHA);
					break;
				case D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE:
					renderer->setSecondModifier(stage, sw::TextureStage::MODIFIER_INVALPHA);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DTSS_ALPHAARG2:
				switch(value & D3DTA_SELECTMASK)
				{
				case D3DTA_DIFFUSE:
					renderer->setSecondArgumentAlpha(stage, sw::TextureStage::SOURCE_DIFFUSE);
					break;
				case D3DTA_CURRENT:
					renderer->setSecondArgumentAlpha(stage, sw::TextureStage::SOURCE_CURRENT);
					break;
				case D3DTA_TEXTURE:
					renderer->setSecondArgumentAlpha(stage, sw::TextureStage::SOURCE_TEXTURE);
					break;
				case D3DTA_TFACTOR:
					renderer->setSecondArgumentAlpha(stage, sw::TextureStage::SOURCE_TFACTOR);
					break;
				case D3DTA_SPECULAR:
					renderer->setSecondArgumentAlpha(stage, sw::TextureStage::SOURCE_SPECULAR);
					break;
				case D3DTA_TEMP:
					renderer->setSecondArgumentAlpha(stage, sw::TextureStage::SOURCE_TEMP);
					break;
				case D3DTA_CONSTANT:
					renderer->setSecondArgumentAlpha(stage, sw::TextureStage::SOURCE_CONSTANT);
					break;
				default:
					ASSERT(false);
				}

				switch(value & ~D3DTA_SELECTMASK)
				{
				case 0:
					renderer->setSecondModifierAlpha(stage, sw::TextureStage::MODIFIER_COLOR);
					break;
				case D3DTA_COMPLEMENT:
					renderer->setSecondModifierAlpha(stage, sw::TextureStage::MODIFIER_INVCOLOR);
					break;
				case D3DTA_ALPHAREPLICATE:
					renderer->setSecondModifierAlpha(stage, sw::TextureStage::MODIFIER_ALPHA);
					break;
				case D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE:
					renderer->setSecondModifierAlpha(stage, sw::TextureStage::MODIFIER_INVALPHA);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DTSS_BUMPENVMAT00:
				renderer->setBumpmapMatrix(stage, 0, (float&)value);
				break;
			case D3DTSS_BUMPENVMAT01:
				renderer->setBumpmapMatrix(stage, 1, (float&)value);
				break;
			case D3DTSS_BUMPENVMAT10:
				renderer->setBumpmapMatrix(stage, 2, (float&)value);
				break;
			case D3DTSS_BUMPENVMAT11:
				renderer->setBumpmapMatrix(stage, 3, (float&)value);
				break;
			case D3DTSS_TEXCOORDINDEX:
				renderer->setTexCoordIndex(stage, value & 0x0000FFFF);

				switch(value & 0xFFFF0000)
				{
				case D3DTSS_TCI_PASSTHRU:
					renderer->setTexGen(stage, sw::TEXGEN_PASSTHRU);
					break;
				case D3DTSS_TCI_CAMERASPACENORMAL:
					renderer->setTexCoordIndex(stage, stage);
					renderer->setTexGen(stage, sw::TEXGEN_NORMAL);
					break;
				case D3DTSS_TCI_CAMERASPACEPOSITION:
					renderer->setTexCoordIndex(stage, stage);
					renderer->setTexGen(stage, sw::TEXGEN_POSITION);
					break;
				case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
					renderer->setTexCoordIndex(stage, stage);
					renderer->setTexGen(stage, sw::TEXGEN_REFLECTION);
					break;
				case D3DTSS_TCI_SPHEREMAP:
					renderer->setTexCoordIndex(stage, stage);
					renderer->setTexGen(stage, sw::TEXGEN_SPHEREMAP);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DTSS_BUMPENVLSCALE:
				renderer->setLuminanceScale(stage, (float&)value);
				break;
			case D3DTSS_BUMPENVLOFFSET:
				renderer->setLuminanceOffset(stage, (float&)value);
				break;
			case D3DTSS_TEXTURETRANSFORMFLAGS:
				switch(value & ~D3DTTFF_PROJECTED)
				{
				case D3DTTFF_DISABLE:
					renderer->setTextureTransform(stage, 0, (value & D3DTTFF_PROJECTED) == D3DTTFF_PROJECTED);
					break;
				case D3DTTFF_COUNT1:
					renderer->setTextureTransform(stage, 1, (value & D3DTTFF_PROJECTED) == D3DTTFF_PROJECTED);
					break;
				case D3DTTFF_COUNT2:
					renderer->setTextureTransform(stage, 2, (value & D3DTTFF_PROJECTED) == D3DTTFF_PROJECTED);
					break;
				case D3DTTFF_COUNT3:
					renderer->setTextureTransform(stage, 3, (value & D3DTTFF_PROJECTED) == D3DTTFF_PROJECTED);
					break;
				case D3DTTFF_COUNT4:
					renderer->setTextureTransform(stage, 4, (value & D3DTTFF_PROJECTED) == D3DTTFF_PROJECTED);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DTSS_COLORARG0:
				switch(value & D3DTA_SELECTMASK)
				{
				case D3DTA_CURRENT:
					renderer->setThirdArgument(stage, sw::TextureStage::SOURCE_CURRENT);
					break;
				case D3DTA_DIFFUSE:
					renderer->setThirdArgument(stage, sw::TextureStage::SOURCE_DIFFUSE);
					break;
				case D3DTA_SPECULAR:
					renderer->setThirdArgument(stage, sw::TextureStage::SOURCE_SPECULAR);
					break;
				case D3DTA_TEMP:
					renderer->setThirdArgument(stage, sw::TextureStage::SOURCE_TEMP);
					break;
				case D3DTA_TEXTURE:
					renderer->setThirdArgument(stage, sw::TextureStage::SOURCE_TEXTURE);
					break;
				case D3DTA_TFACTOR:
					renderer->setThirdArgument(stage, sw::TextureStage::SOURCE_TFACTOR);
					break;
				default:
					ASSERT(false);
				}

				switch(value & ~D3DTA_SELECTMASK)
				{
				case 0:
					renderer->setThirdModifier(stage, sw::TextureStage::MODIFIER_COLOR);
					break;
				case D3DTA_COMPLEMENT:
					renderer->setThirdModifier(stage, sw::TextureStage::MODIFIER_INVCOLOR);
					break;
				case D3DTA_ALPHAREPLICATE:
					renderer->setThirdModifier(stage, sw::TextureStage::MODIFIER_ALPHA);
					break;
				case D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE:
					renderer->setThirdModifier(stage, sw::TextureStage::MODIFIER_INVALPHA);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DTSS_ALPHAARG0:
				switch(value & D3DTA_SELECTMASK)
				{
				case D3DTA_DIFFUSE:
					renderer->setThirdArgumentAlpha(stage, sw::TextureStage::SOURCE_DIFFUSE);
					break;
				case D3DTA_CURRENT:
					renderer->setThirdArgumentAlpha(stage, sw::TextureStage::SOURCE_CURRENT);
					break;
				case D3DTA_TEXTURE:
					renderer->setThirdArgumentAlpha(stage, sw::TextureStage::SOURCE_TEXTURE);
					break;
				case D3DTA_TFACTOR:
					renderer->setThirdArgumentAlpha(stage, sw::TextureStage::SOURCE_TFACTOR);
					break;
				case D3DTA_SPECULAR:
					renderer->setThirdArgumentAlpha(stage, sw::TextureStage::SOURCE_SPECULAR);
					break;
				case D3DTA_TEMP:
					renderer->setThirdArgumentAlpha(stage, sw::TextureStage::SOURCE_TEMP);
					break;
				case D3DTA_CONSTANT:
					renderer->setThirdArgumentAlpha(stage, sw::TextureStage::SOURCE_CONSTANT);
					break;
				default:
					ASSERT(false);
				}

				switch(value & ~D3DTA_SELECTMASK)
				{
				case 0:
					renderer->setThirdModifierAlpha(stage, sw::TextureStage::MODIFIER_COLOR);
					break;
				case D3DTA_COMPLEMENT:
					renderer->setThirdModifierAlpha(stage, sw::TextureStage::MODIFIER_INVCOLOR);
					break;
				case D3DTA_ALPHAREPLICATE:
					renderer->setThirdModifierAlpha(stage, sw::TextureStage::MODIFIER_ALPHA);
					break;
				case D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE:
					renderer->setThirdModifierAlpha(stage, sw::TextureStage::MODIFIER_INVALPHA);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DTSS_RESULTARG:
				switch(value & D3DTA_SELECTMASK)
				{
				case D3DTA_CURRENT:
					renderer->setDestinationArgument(stage, sw::TextureStage::DESTINATION_CURRENT);
					break;
				case D3DTA_TEMP:
					renderer->setDestinationArgument(stage, sw::TextureStage::DESTINATION_TEMP);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DTSS_CONSTANT:
				renderer->setConstantColor(stage, value);
				break;
			default:
				ASSERT(false);
			}
		}
		else   // stateRecorder
		{
			stateRecorder->setTextureStageState(stage, type, value);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix)
	{
		CriticalSection cs(this);

		TRACE("D3DTRANSFORMSTATETYPE state = %d, const D3DMATRIX *matrix = 0x%0.8p", state, matrix);

		if(!matrix)
		{
			return INVALIDCALL();
		}

		if(!stateRecorder)
		{
			this->matrix[state] = *matrix;

			sw::Matrix M(matrix->_11, matrix->_21, matrix->_31, matrix->_41,
			             matrix->_12, matrix->_22, matrix->_32, matrix->_42,
			             matrix->_13, matrix->_23, matrix->_33, matrix->_43,
			             matrix->_14, matrix->_24, matrix->_34, matrix->_44);

			switch(state)
			{
			case D3DTS_WORLD:
				renderer->setModelMatrix(M);
				break;
			case D3DTS_VIEW:
				renderer->setViewMatrix(M);
				break;
			case D3DTS_PROJECTION:
				renderer->setProjectionMatrix(M);
				break;
			case D3DTS_TEXTURE0:
				renderer->setTextureMatrix(0, M);
				break;
			case D3DTS_TEXTURE1:
				renderer->setTextureMatrix(1, M);
				break;
			case D3DTS_TEXTURE2:
				renderer->setTextureMatrix(2, M);
				break;
			case D3DTS_TEXTURE3:
				renderer->setTextureMatrix(3, M);
				break;
			case D3DTS_TEXTURE4:
				renderer->setTextureMatrix(4, M);
				break;
			case D3DTS_TEXTURE5:
				renderer->setTextureMatrix(5, M);
				break;
			case D3DTS_TEXTURE6:
				renderer->setTextureMatrix(6, M);
				break;
			case D3DTS_TEXTURE7:
				renderer->setTextureMatrix(7, M);
				break;
			default:
				if(state > 256 && state < 512)
				{
					renderer->setModelMatrix(M, state - 256);
				}
				else ASSERT(false);
			}
		}
		else   // stateRecorder
		{
			stateRecorder->setTransform(state, matrix);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetVertexDeclaration(IDirect3DVertexDeclaration9 *iVertexDeclaration)
	{
		CriticalSection cs(this);

		TRACE("IDirect3DVertexDeclaration9 *declaration = 0x%0.8p", iVertexDeclaration);

		Direct3DVertexDeclaration9 *vertexDeclaration = static_cast<Direct3DVertexDeclaration9*>(iVertexDeclaration);

		if(!stateRecorder)
		{
			if(this->vertexDeclaration == vertexDeclaration)
			{
				return D3D_OK;
			}

			if(vertexDeclaration)
			{
				vertexDeclaration->bind();
			}

			if(this->vertexDeclaration)
			{
				this->vertexDeclaration->unbind();
			}

			this->vertexDeclaration = vertexDeclaration;
		}
		else
		{
			stateRecorder->setVertexDeclaration(vertexDeclaration);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetVertexShader(IDirect3DVertexShader9 *iVertexShader)
	{
		CriticalSection cs(this);

		TRACE("IDirect3DVertexShader9 *shader = 0x%0.8p", iVertexShader);

		Direct3DVertexShader9 *vertexShader = static_cast<Direct3DVertexShader9*>(iVertexShader);

		if(!stateRecorder)
		{
			if(this->vertexShader == vertexShader)
			{
				return D3D_OK;
			}

			if(vertexShader)
			{
				vertexShader->bind();
			}

			if(this->vertexShader)
			{
				this->vertexShader->unbind();
			}

			this->vertexShader = vertexShader;
			vertexShaderDirty = true;
		}
		else
		{
			stateRecorder->setVertexShader(vertexShader);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetVertexShaderConstantB(unsigned int startRegister, const int *constantData, unsigned int count)
	{
		CriticalSection cs(this);

		TRACE("unsigned int startRegister = %d, const int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		if(!constantData)
		{
			return INVALIDCALL();
		}

		if(!stateRecorder)
		{
			for(unsigned int i = 0; i < count && startRegister + i < 16; i++)
			{
				vertexShaderConstantB[startRegister + i] = constantData[i];
			}

			vertexShaderConstantsBDirty = sw::max(startRegister + count, vertexShaderConstantsBDirty);
			vertexShaderDirty = true;   // Reload DEF constants
		}
		else
		{
			stateRecorder->setVertexShaderConstantB(startRegister, constantData, count);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetVertexShaderConstantF(unsigned int startRegister, const float *constantData, unsigned int count)
	{
		CriticalSection cs(this);

		TRACE("unsigned int startRegister = %d, const int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		if(!constantData)
		{
			return INVALIDCALL();
		}

		if(!stateRecorder)
		{
			for(unsigned int i = 0; i < count && startRegister + i < MAX_VERTEX_SHADER_CONST; i++)
			{
				vertexShaderConstantF[startRegister + i][0] = constantData[i * 4 + 0];
				vertexShaderConstantF[startRegister + i][1] = constantData[i * 4 + 1];
				vertexShaderConstantF[startRegister + i][2] = constantData[i * 4 + 2];
				vertexShaderConstantF[startRegister + i][3] = constantData[i * 4 + 3];
			}

			vertexShaderConstantsFDirty = sw::max(startRegister + count, vertexShaderConstantsFDirty);
			vertexShaderDirty = true;   // Reload DEF constants
		}
		else
		{
			stateRecorder->setVertexShaderConstantF(startRegister, constantData, count);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetVertexShaderConstantI(unsigned int startRegister, const int *constantData, unsigned int count)
	{
		CriticalSection cs(this);

		TRACE("unsigned int startRegister = %d, const int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		if(!constantData)
		{
			return INVALIDCALL();
		}

		if(!stateRecorder)
		{
			for(unsigned int i = 0; i < count && startRegister + i < 16; i++)
			{
				vertexShaderConstantI[startRegister + i][0] = constantData[i * 4 + 0];
				vertexShaderConstantI[startRegister + i][1] = constantData[i * 4 + 1];
				vertexShaderConstantI[startRegister + i][2] = constantData[i * 4 + 2];
				vertexShaderConstantI[startRegister + i][3] = constantData[i * 4 + 3];
			}

			vertexShaderConstantsIDirty = sw::max(startRegister + count, vertexShaderConstantsIDirty);
			vertexShaderDirty = true;   // Reload DEF constants
		}
		else
		{
			stateRecorder->setVertexShaderConstantI(startRegister, constantData, count);
		}

		return D3D_OK;
	}

	long Direct3DDevice9::SetViewport(const D3DVIEWPORT9 *viewport)
	{
		CriticalSection cs(this);

		TRACE("const D3DVIEWPORT9 *viewport = 0x%0.8p", viewport);

		if(!viewport)   // FIXME: Check if valid
		{
			return INVALIDCALL();
		}

		if(!stateRecorder)
		{
			this->viewport = *viewport;
		}
		else
		{
			stateRecorder->setViewport(viewport);
		}

		return D3D_OK;
	}

	int Direct3DDevice9::ShowCursor(int show)
	{
		CriticalSection cs(this);

		TRACE("int show = %d", show);

		int oldValue = showCursor ? TRUE : FALSE;
		showCursor = show != FALSE;

		if(showCursor)
		{
			sw::FrameBuffer::setCursorImage(cursor);
		}
		else
		{
			sw::FrameBuffer::setCursorImage(0);
		}

		return oldValue;
	}

	long Direct3DDevice9::StretchRect(IDirect3DSurface9 *sourceSurface, const RECT *sourceRect, IDirect3DSurface9 *destSurface, const RECT *destRect, D3DTEXTUREFILTERTYPE filter)
	{
		CriticalSection cs(this);

		TRACE("IDirect3DSurface9 *sourceSurface = 0x%0.8p, const RECT *sourceRect = 0x%0.8p, IDirect3DSurface9 *destSurface = 0x%0.8p, const RECT *destRect = 0x%0.8p, D3DTEXTUREFILTERTYPE filter = %d", sourceSurface, sourceRect, destSurface, destRect, filter);

		if(!sourceSurface || !destSurface || !validRectangle(sourceRect, sourceSurface) || !validRectangle(destRect, destSurface))
		{
			return INVALIDCALL();
		}

		D3DSURFACE_DESC sourceDescription;
		D3DSURFACE_DESC destDescription;

		sourceSurface->GetDesc(&sourceDescription);
		destSurface->GetDesc(&destDescription);

		if(sourceDescription.Pool != D3DPOOL_DEFAULT || destDescription.Pool != D3DPOOL_DEFAULT)
		{
			return INVALIDCALL();
		}

		Direct3DSurface9 *source = static_cast<Direct3DSurface9*>(sourceSurface);
		Direct3DSurface9 *dest = static_cast<Direct3DSurface9*>(destSurface);

		stretchRect(source, sourceRect, dest, destRect, filter);

		return D3D_OK;
	}

	long Direct3DDevice9::TestCooperativeLevel()
	{
		CriticalSection cs(this);

		TRACE("void");

		return D3D_OK;
	}

	long Direct3DDevice9::UpdateSurface(IDirect3DSurface9 *sourceSurface, const RECT *sourceRect, IDirect3DSurface9 *destinationSurface, const POINT *destPoint)
	{
		CriticalSection cs(this);

		TRACE("IDirect3DSurface9 *sourceSurface = 0x%0.8p, const RECT *sourceRect = 0x%0.8p, IDirect3DSurface9 *destinationSurface = 0x%0.8p, const POINT *destPoint = 0x%0.8p", sourceSurface, sourceRect, destinationSurface, destPoint);

		if(!sourceSurface || !destinationSurface)
		{
			return INVALIDCALL();
		}

		D3DSURFACE_DESC sourceDescription;
		D3DSURFACE_DESC destinationDescription;

		sourceSurface->GetDesc(&sourceDescription);
		destinationSurface->GetDesc(&destinationDescription);

		RECT sRect;
		RECT dRect;

		if(sourceRect)
		{
			sRect.left = sourceRect->left;
			sRect.top = sourceRect->top;
			sRect.right = sourceRect->right;
			sRect.bottom = sourceRect->bottom;
		}
		else
		{
			sRect.left = 0;
			sRect.top = 0;
			sRect.right = sourceDescription.Width;
			sRect.bottom = sourceDescription.Height;
		}

		if(destPoint)
		{
			dRect.left = destPoint->x;
			dRect.top = destPoint->y;
			dRect.right = destPoint->x + sRect.right - sRect.left;
			dRect.bottom = destPoint->y + sRect.bottom - sRect.top;
		}
		else
		{
			dRect.left = 0;
			dRect.top = 0;
			dRect.right = sRect.right - sRect.left;
			dRect.bottom = sRect.bottom - sRect.top;
		}

		if(!validRectangle(&sRect, sourceSurface) || !validRectangle(&dRect, destinationSurface))
		{
			return INVALIDCALL();
		}

		int sWidth = sRect.right - sRect.left;
		int sHeight = sRect.bottom - sRect.top;

		int dWidth = dRect.right - dRect.left;
		int dHeight = dRect.bottom - dRect.top;

		if(sourceDescription.MultiSampleType      != D3DMULTISAMPLE_NONE ||
		   destinationDescription.MultiSampleType != D3DMULTISAMPLE_NONE ||
		// sourceDescription.Pool      != D3DPOOL_SYSTEMMEM ||   // FIXME: Check back buffer and depth buffer memory pool flags
		// destinationDescription.Pool != D3DPOOL_DEFAULT ||
		   sourceDescription.Format != destinationDescription.Format)
		{
			return INVALIDCALL();
		}

		sw::Surface *source = static_cast<Direct3DSurface9*>(sourceSurface);
		sw::Surface *dest = static_cast<Direct3DSurface9*>(destinationSurface);

		unsigned char *sBuffer = (unsigned char*)source->lockExternal(sRect.left, sRect.top, 0, sw::LOCK_READONLY, sw::PUBLIC);
		unsigned char *dBuffer = (unsigned char*)dest->lockExternal(dRect.left, dRect.top, 0, sw::LOCK_WRITEONLY, sw::PUBLIC);
		int sPitch = source->getExternalPitchB();
		int dPitch = dest->getExternalPitchB();

		unsigned int width;
		unsigned int height;
		unsigned int bytes;

		switch(sourceDescription.Format)
		{
		case D3DFMT_DXT1:
		case D3DFMT_ATI1:
			width = (dWidth + 3) / 4;
			height = (dHeight + 3) / 4;
			bytes = width * 8;   // 64 bit per 4x4 block
			break;
		case D3DFMT_DXT2:
		case D3DFMT_DXT3:
		case D3DFMT_DXT4:
		case D3DFMT_DXT5:
		case D3DFMT_ATI2:
			width = (dWidth + 3) / 4;
			height = (dHeight + 3) / 4;
			bytes = width * 16;   // 128 bit per 4x4 block
			break;
		default:
			width = dWidth;
			height = dHeight;
			bytes = width * Direct3DSurface9::bytes(sourceDescription.Format);
		}

		if(sourceDescription.Format == D3DFMT_ATI1 || sourceDescription.Format == D3DFMT_ATI2)
		{
			// Make the pitch correspond to 4 rows
			sPitch *= 4;
			dPitch *= 4;
		}

		for(unsigned int y = 0; y < height; y++)
		{
			memcpy(dBuffer, sBuffer, bytes);

			sBuffer += sPitch;
			dBuffer += dPitch;
		}

		source->unlockExternal();
		dest->unlockExternal();

		return D3D_OK;
	}

	long Direct3DDevice9::UpdateTexture(IDirect3DBaseTexture9 *sourceTexture, IDirect3DBaseTexture9 *destinationTexture)
	{
		CriticalSection cs(this);

		TRACE("IDirect3DBaseTexture9 *sourceTexture = 0x%0.8p, IDirect3DBaseTexture9 *destinationTexture = 0x%0.8p", sourceTexture, destinationTexture);

		if(!sourceTexture || !destinationTexture)
		{
			return INVALIDCALL();
		}

		// FIXME: Check memory pools

		D3DRESOURCETYPE type = sourceTexture->GetType();

		if(type != destinationTexture->GetType())
		{
			return INVALIDCALL();
		}

		switch(type)
		{
		case D3DRTYPE_TEXTURE:
			{
				IDirect3DTexture9 *source;
				IDirect3DTexture9 *dest;

				sourceTexture->QueryInterface(IID_IDirect3DTexture9, (void**)&source);
				destinationTexture->QueryInterface(IID_IDirect3DTexture9, (void**)&dest);

				ASSERT(source && dest);

				for(unsigned int level = 0; level < source->GetLevelCount() && level < dest->GetLevelCount(); level++)   // FIXME: Fail when source texture has fewer levels than the destination
				{
					IDirect3DSurface9 *sourceSurface;
					IDirect3DSurface9 *destinationSurface;

					source->GetSurfaceLevel(level, &sourceSurface);
					dest->GetSurfaceLevel(level, &destinationSurface);

					UpdateSurface(sourceSurface, 0, destinationSurface, 0);

					sourceSurface->Release();
					destinationSurface->Release();
				}

				source->Release();
				dest->Release();
			}
			break;
		case D3DRTYPE_VOLUMETEXTURE:
			{
				IDirect3DVolumeTexture9 *source;
				IDirect3DVolumeTexture9 *dest;

				sourceTexture->QueryInterface(IID_IDirect3DVolumeTexture9, (void**)&source);
				destinationTexture->QueryInterface(IID_IDirect3DVolumeTexture9, (void**)&dest);

				ASSERT(source && dest);

				for(unsigned int level = 0; level < source->GetLevelCount() && level < dest->GetLevelCount(); level++)   // FIXME: Fail when source texture has fewer levels than the destination
				{
					IDirect3DVolume9 *sourceVolume;
					IDirect3DVolume9 *destinationVolume;

					source->GetVolumeLevel(level, &sourceVolume);
					dest->GetVolumeLevel(level, &destinationVolume);

					updateVolume(sourceVolume, destinationVolume);

					sourceVolume->Release();
					destinationVolume->Release();
				}

				source->Release();
				dest->Release();
			}
			break;
		case D3DRTYPE_CUBETEXTURE:
			{
				IDirect3DCubeTexture9 *source;
				IDirect3DCubeTexture9 *dest;

				sourceTexture->QueryInterface(IID_IDirect3DCubeTexture9, (void**)&source);
				destinationTexture->QueryInterface(IID_IDirect3DCubeTexture9, (void**)&dest);

				ASSERT(source && dest);

				for(int face = 0; face < 6; face++)
				{
					for(unsigned int level = 0; level < source->GetLevelCount() && level < dest->GetLevelCount(); level++)   // FIXME: Fail when source texture has fewer levels than the destination
					{
						IDirect3DSurface9 *sourceSurface;
						IDirect3DSurface9 *destinationSurface;

						source->GetCubeMapSurface((D3DCUBEMAP_FACES)face, level, &sourceSurface);
						dest->GetCubeMapSurface((D3DCUBEMAP_FACES)face, level, &destinationSurface);

						UpdateSurface(sourceSurface, 0, destinationSurface, 0);

						sourceSurface->Release();
						destinationSurface->Release();
					}
				}

				source->Release();
				dest->Release();
			}
			break;
		default:
			UNIMPLEMENTED();
		}

		return D3D_OK;
	}

	long Direct3DDevice9::ValidateDevice(unsigned long *numPasses)
	{
		CriticalSection cs(this);

		TRACE("unsigned long *numPasses = 0x%0.8p", numPasses);

		if(!numPasses)
		{
			return INVALIDCALL();
		}

		*numPasses = 1;

		return D3D_OK;
	}

	long Direct3DDevice9::getAdapterDisplayMode(unsigned int adapter, D3DDISPLAYMODE *mode)
	{
		return d3d9->GetAdapterDisplayMode(adapter, mode);
	}

	int Direct3DDevice9::typeStride(unsigned char streamType)
	{
		static int LUT[] =
		{
			4,	// D3DDECLTYPE_FLOAT1    =  0,  // 1D float expanded to (value, 0., 0., 1.)
			8,	// D3DDECLTYPE_FLOAT2    =  1,  // 2D float expanded to (value, value, 0., 1.)
			12,	// D3DDECLTYPE_FLOAT3    =  2,  // 3D float expanded to (value, value, value, 1.)
			16,	// D3DDECLTYPE_FLOAT4    =  3,  // 4D float
			4,	// D3DDECLTYPE_D3DCOLOR  =  4,  // 4D packed unsigned bytes mapped to 0. to 1. range. Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
			4,	// D3DDECLTYPE_UBYTE4    =  5,  // 4D unsigned byte
			4,	// D3DDECLTYPE_SHORT2    =  6,  // 2D signed short expanded to (value, value, 0., 1.)
			8,	// D3DDECLTYPE_SHORT4    =  7,  // 4D signed short
			4,	// D3DDECLTYPE_UBYTE4N   =  8,  // Each of 4 bytes is normalized by dividing to 255.0
			4,	// D3DDECLTYPE_SHORT2N   =  9,  // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
			8,	// D3DDECLTYPE_SHORT4N   = 10,  // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
			4,	// D3DDECLTYPE_USHORT2N  = 11,  // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
			8,	// D3DDECLTYPE_USHORT4N  = 12,  // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
			4,	// D3DDECLTYPE_UDEC3     = 13,  // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
			4,	// D3DDECLTYPE_DEC3N     = 14,  // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
			4,	// D3DDECLTYPE_FLOAT16_2 = 15,  // Two 16-bit floating point values, expanded to (value, value, 0, 1)
			8,	// D3DDECLTYPE_FLOAT16_4 = 16,  // Four 16-bit floating point values
			0,	// D3DDECLTYPE_UNUSED    = 17,  // When the type field in a decl is unused.
		};

		return LUT[streamType];
	}

	bool Direct3DDevice9::instanceData()
	{
		ASSERT(vertexDeclaration);

		D3DVERTEXELEMENT9 vertexElement[MAXD3DDECLLENGTH + 1];
		unsigned int numElements;
		vertexDeclaration->GetDeclaration(vertexElement, &numElements);

		bool instanceData = false;

		for(unsigned int i = 0; i < numElements - 1; i++)
		{
			unsigned short stream = vertexElement[i].Stream;

			if(stream != 0)
			{
				instanceData = instanceData || (streamSourceFreq[stream] & D3DSTREAMSOURCE_INSTANCEDATA) != 0;
			}
		}

		return instanceData;
	}

	bool Direct3DDevice9::bindResources(Direct3DIndexBuffer9 *indexBuffer)
	{
		if(!bindViewport())
		{
			return false;   // Zero-area target region
		}

		bindTextures();
		bindIndexBuffer(indexBuffer);
		bindShaderConstants();
		bindLights();

		return true;
	}

	void Direct3DDevice9::bindVertexStreams(int base, bool instancing, int instance)
	{
		ASSERT(vertexDeclaration);

		renderer->resetInputStreams(vertexDeclaration->isPreTransformed());

		D3DVERTEXELEMENT9 vertexElement[MAXD3DDECLLENGTH + 1];
		unsigned int numElements;
		vertexDeclaration->GetDeclaration(vertexElement, &numElements);

		// Bind vertex data streams
		for(unsigned int i = 0; i < numElements - 1; i++)
		{
			unsigned short stream = vertexElement[i].Stream;
			unsigned short offset = vertexElement[i].Offset;
			unsigned char type = vertexElement[i].Type;
			unsigned char method = vertexElement[i].Method;
			unsigned char usage = vertexElement[i].Usage;
			unsigned char index = vertexElement[i].UsageIndex;

			ASSERT(method == D3DDECLMETHOD_DEFAULT);	// FIXME: Unimplemented

			if(!dataStream[stream])
			{
				continue;
			}

			Direct3DVertexBuffer9 *streamBuffer = dataStream[stream];
			sw::Resource *resource = streamBuffer->getResource();
			const void *buffer = ((char*)resource->data() + streamOffset[stream]) + offset;

			int stride = streamStride[stream];

			if(instancing && streamSourceFreq[stream] & D3DSTREAMSOURCE_INSTANCEDATA)
			{
				int instanceFrequency = streamSourceFreq[stream] & ~D3DSTREAMSOURCE_INSTANCEDATA;
				buffer = (char*)buffer + stride * (instance / instanceFrequency);

				stride = 0;
			}
			else
			{
				buffer = (char*)buffer + stride * base;
			}

			sw::Stream attribute(resource, buffer, stride);

			switch(type)
			{
			case D3DDECLTYPE_FLOAT1:    attribute.define(sw::STREAMTYPE_FLOAT, 1, false);  break;
			case D3DDECLTYPE_FLOAT2:    attribute.define(sw::STREAMTYPE_FLOAT, 2, false);  break;
			case D3DDECLTYPE_FLOAT3:    attribute.define(sw::STREAMTYPE_FLOAT, 3, false);  break;
			case D3DDECLTYPE_FLOAT4:    attribute.define(sw::STREAMTYPE_FLOAT, 4, false);  break;
			case D3DDECLTYPE_D3DCOLOR:  attribute.define(sw::STREAMTYPE_COLOR, 4, false);  break;
			case D3DDECLTYPE_UBYTE4:    attribute.define(sw::STREAMTYPE_BYTE, 4, false);   break;
			case D3DDECLTYPE_SHORT2:    attribute.define(sw::STREAMTYPE_SHORT, 2, false);  break;
			case D3DDECLTYPE_SHORT4:    attribute.define(sw::STREAMTYPE_SHORT, 4, false);  break;
			case D3DDECLTYPE_UBYTE4N:   attribute.define(sw::STREAMTYPE_BYTE, 4, true);    break;
			case D3DDECLTYPE_SHORT2N:   attribute.define(sw::STREAMTYPE_SHORT, 2, true);   break;
			case D3DDECLTYPE_SHORT4N:   attribute.define(sw::STREAMTYPE_SHORT, 4, true);   break;
			case D3DDECLTYPE_USHORT2N:  attribute.define(sw::STREAMTYPE_USHORT, 2, true);  break;
			case D3DDECLTYPE_USHORT4N:  attribute.define(sw::STREAMTYPE_USHORT, 4, true);  break;
			case D3DDECLTYPE_UDEC3:     attribute.define(sw::STREAMTYPE_UDEC3, 3, false);  break;
			case D3DDECLTYPE_DEC3N:     attribute.define(sw::STREAMTYPE_DEC3N, 3, true);   break;
			case D3DDECLTYPE_FLOAT16_2: attribute.define(sw::STREAMTYPE_HALF, 2, false);   break;
			case D3DDECLTYPE_FLOAT16_4: attribute.define(sw::STREAMTYPE_HALF, 4, false);   break;
			case D3DDECLTYPE_UNUSED:    attribute.defaults();                              break;
			default:
				ASSERT(false);
			}

			if(vertexShader)
			{
				const sw::VertexShader *shader = vertexShader->getVertexShader();

				if(!vertexDeclaration->isPreTransformed())
				{
					for(int i = 0; i < MAX_VERTEX_INPUTS; i++)
					{
						const sw::Shader::Semantic& input = shader->getInput(i);
						if((usage == input.usage) && (index == input.index))
						{
							renderer->setInputStream(i, attribute);

							break;
						}
					}
				}
				else   // Bind directly to the output
				{
					for(int i = 0; i < MAX_VERTEX_OUTPUTS; i++)
					{
						const sw::Shader::Semantic& output = shader->getOutput(i, 0);
						if(((usage == output.usage) || (usage == D3DDECLUSAGE_POSITIONT && output.usage == D3DDECLUSAGE_POSITION)) &&
						   (index == output.index))
						{
							renderer->setInputStream(i, attribute);

							break;
						}
					}
				}
			}
			else
			{
				switch(usage)
				{
				case D3DDECLUSAGE_POSITION:     renderer->setInputStream(sw::Position, attribute);                                       break;
				case D3DDECLUSAGE_BLENDWEIGHT:  renderer->setInputStream(sw::BlendWeight, attribute);                                    break;
				case D3DDECLUSAGE_BLENDINDICES: renderer->setInputStream(sw::BlendIndices, attribute.define(sw::STREAMTYPE_INDICES, 1)); break;
				case D3DDECLUSAGE_NORMAL:       renderer->setInputStream(sw::Normal, attribute.define(sw::STREAMTYPE_FLOAT, 3));         break;
				case D3DDECLUSAGE_PSIZE:        renderer->setInputStream(sw::PointSize, attribute.define(sw::STREAMTYPE_FLOAT, 1));      break;
				case D3DDECLUSAGE_TEXCOORD:     renderer->setInputStream(sw::TexCoord0 + index, attribute);                              break;
				case D3DDECLUSAGE_TANGENT:      /* Ignored */                                                                            break;
				case D3DDECLUSAGE_BINORMAL:     /* Ignored */                                                                            break;
				case D3DDECLUSAGE_TESSFACTOR:   UNIMPLEMENTED();                                                                         break;
				case D3DDECLUSAGE_POSITIONT:    renderer->setInputStream(sw::PositionT, attribute.define(sw::STREAMTYPE_FLOAT, 4));      break;
				case D3DDECLUSAGE_COLOR:        renderer->setInputStream(sw::Color0 + index, attribute.define(sw::STREAMTYPE_COLOR, 4)); break;
				case D3DDECLUSAGE_FOG:          /* Ignored */                                                                            break;
				case D3DDECLUSAGE_DEPTH:        /* Ignored */                                                                            break;
				case D3DDECLUSAGE_SAMPLE:       UNIMPLEMENTED();                                                                         break;
				default:
					ASSERT(false);
				}
			}
		}
	}

	void Direct3DDevice9::bindIndexBuffer(Direct3DIndexBuffer9 *indexBuffer)
	{
		sw::Resource *resource = 0;

		if(indexBuffer)
		{
			resource = indexBuffer->getResource();
		}

		renderer->setIndexBuffer(resource);
	}

	void Direct3DDevice9::bindShaderConstants()
	{
		if(pixelShaderDirty)
		{
			if(pixelShader)
			{
				if(pixelShaderConstantsBDirty)
				{
					renderer->setPixelShaderConstantB(0, pixelShaderConstantB, pixelShaderConstantsBDirty);
				}

				if(pixelShaderConstantsFDirty)
				{
					renderer->setPixelShaderConstantF(0, pixelShaderConstantF[0], pixelShaderConstantsFDirty);
				}

				if(pixelShaderConstantsIDirty)
				{
					renderer->setPixelShaderConstantI(0, pixelShaderConstantI[0], pixelShaderConstantsIDirty);
				}

				renderer->setPixelShader(pixelShader->getPixelShader());   // Loads shader constants set with DEF
				pixelShaderConstantsBDirty = pixelShader->getPixelShader()->dirtyConstantsB;   // Shader DEF'ed constants are dirty
				pixelShaderConstantsFDirty = pixelShader->getPixelShader()->dirtyConstantsF;   // Shader DEF'ed constants are dirty
				pixelShaderConstantsIDirty = pixelShader->getPixelShader()->dirtyConstantsI;   // Shader DEF'ed constants are dirty
			}
			else
			{
				renderer->setPixelShader(0);
			}

			pixelShaderDirty = false;
		}

		if(vertexShaderDirty)
		{
			if(vertexShader)
			{
				if(vertexShaderConstantsBDirty)
				{
					renderer->setVertexShaderConstantB(0, vertexShaderConstantB, vertexShaderConstantsBDirty);
				}

				if(vertexShaderConstantsFDirty)
				{
					renderer->setVertexShaderConstantF(0, vertexShaderConstantF[0], vertexShaderConstantsFDirty);
				}

				if(vertexShaderConstantsIDirty)
				{
					renderer->setVertexShaderConstantI(0, vertexShaderConstantI[0], vertexShaderConstantsIDirty);
				}

				renderer->setVertexShader(vertexShader->getVertexShader());   // Loads shader constants set with DEF
				vertexShaderConstantsBDirty = vertexShader->getVertexShader()->dirtyConstantsB;   // Shader DEF'ed constants are dirty
				vertexShaderConstantsFDirty = vertexShader->getVertexShader()->dirtyConstantsF;   // Shader DEF'ed constants are dirty
				vertexShaderConstantsIDirty = vertexShader->getVertexShader()->dirtyConstantsI;   // Shader DEF'ed constants are dirty
			}
			else
			{
				renderer->setVertexShader(0);
			}

			vertexShaderDirty = false;
		}
	}

	void Direct3DDevice9::bindLights()
	{
		if(!lightsDirty) return;

		Lights::iterator i = light.begin();
		int active = 0;

		// Set and enable renderer lights
		while(active < 8)
		{
			while(i != light.end() && !i->second.enable)
			{
				i++;
			}

			if(i == light.end())
			{
				break;
			}

			const Light &l = i->second;

			sw::Point position(l.Position.x, l.Position.y, l.Position.z);
			sw::Color<float> diffuse(l.Diffuse.r, l.Diffuse.g, l.Diffuse.b, l.Diffuse.a);
			sw::Color<float> specular(l.Specular.r, l.Specular.g, l.Specular.b, l.Specular.a);
			sw::Color<float> ambient(l.Ambient.r, l.Ambient.g, l.Ambient.b, l.Ambient.a);
			sw::Vector direction(l.Direction.x, l.Direction.y, l.Direction.z);

			renderer->setLightDiffuse(active, diffuse);
			renderer->setLightSpecular(active, specular);
			renderer->setLightAmbient(active, ambient);

			if(l.Type == D3DLIGHT_DIRECTIONAL)
			{
				// FIXME: Unsupported, make it a positional light far away without falloff
				renderer->setLightPosition(active, -1e10f * direction);
				renderer->setLightRange(active, l.Range);
				renderer->setLightAttenuation(active, 1, 0, 0);
			}
			else if(l.Type == D3DLIGHT_SPOT)
			{
				// FIXME: Unsupported, make it a positional light
				renderer->setLightPosition(active, position);
				renderer->setLightRange(active, l.Range);
				renderer->setLightAttenuation(active, l.Attenuation0, l.Attenuation1, l.Attenuation2);
			}
			else
			{
				renderer->setLightPosition(active, position);
				renderer->setLightRange(active, l.Range);
				renderer->setLightAttenuation(active, l.Attenuation0, l.Attenuation1, l.Attenuation2);
			}

			renderer->setLightEnable(active, true);

			active++;
			i++;
		}

		// Remaining lights are disabled
		while(active < 8)
		{
			renderer->setLightEnable(active, false);

			active++;
		}

		lightsDirty = false;
	}

	bool Direct3DDevice9::bindViewport()
	{
		if(viewport.Width <= 0 || viewport.Height <= 0)
		{
			return false;
		}

		if(scissorEnable)
		{
			if(scissorRect.left >= scissorRect.right || scissorRect.top >= scissorRect.bottom)
			{
				return false;
			}

			sw::Rect scissor;
			scissor.x0 = scissorRect.left;
			scissor.x1 = scissorRect.right;
			scissor.y0 = scissorRect.top;
			scissor.y1 = scissorRect.bottom;

			renderer->setScissor(scissor);
		}
		else
		{
			sw::Rect scissor;
			scissor.x0 = viewport.X;
			scissor.x1 = viewport.X + viewport.Width;
			scissor.y0 = viewport.Y;
			scissor.y1 = viewport.Y + viewport.Height;

			renderer->setScissor(scissor);
		}

		sw::Viewport view;
		view.x0 = (float)viewport.X;
		view.y0 = (float)viewport.Y + viewport.Height;
		view.width = (float)viewport.Width;
		view.height = -(float)viewport.Height;
		view.minZ = viewport.MinZ;
		view.maxZ = viewport.MaxZ;

		renderer->setViewport(view);

		return true;
	}

	void Direct3DDevice9::bindTextures()
	{
		for(int sampler = 0; sampler < 16 + 4; sampler++)
		{
			Direct3DBaseTexture9 *baseTexture = texture[sampler];

			sw::SamplerType type = sampler < 16 ? sw::SAMPLER_PIXEL : sw::SAMPLER_VERTEX;
			int index = sampler < 16 ? sampler : sampler - 16;   // Sampler index within type group

			bool textureUsed = false;

			if(type == sw::SAMPLER_PIXEL && pixelShader)
			{
				textureUsed = pixelShader->getPixelShader()->usesSampler(index);
			}
			else if(type == sw::SAMPLER_VERTEX && vertexShader)
			{
				textureUsed = vertexShader->getVertexShader()->usesSampler(index);
			}
			else
			{
				textureUsed = true;   // FIXME: Check fixed-function use?
			}

			sw::Resource *resource = 0;

			if(baseTexture && textureUsed)
			{
				resource = baseTexture->getResource();
			}

			renderer->setTextureResource(sampler, resource);

			if(baseTexture && textureUsed)
			{
				baseTexture->GenerateMipSubLevels();
			}

			if(baseTexture && textureUsed)
			{
				int levelCount = baseTexture->getInternalLevelCount();

				int textureLOD = baseTexture->GetLOD();
				int samplerLOD = samplerState[sampler][D3DSAMP_MAXMIPLEVEL];
				int LOD = textureLOD > samplerLOD ? textureLOD : samplerLOD;

				if(samplerState[sampler][D3DSAMP_MIPFILTER] == D3DTEXF_NONE)
				{
					LOD = 0;
				}

				switch(baseTexture->GetType())
				{
				case D3DRTYPE_TEXTURE:
					{
						Direct3DTexture9 *texture = dynamic_cast<Direct3DTexture9*>(baseTexture);
						Direct3DSurface9 *surface;

						for(int mipmapLevel = 0; mipmapLevel < sw::MIPMAP_LEVELS; mipmapLevel++)
						{
							int surfaceLevel = mipmapLevel;

							if(surfaceLevel < LOD)
							{
								surfaceLevel = LOD;
							}

							if(surfaceLevel < 0)
							{
								surfaceLevel = 0;
							}
							else if(surfaceLevel >= levelCount)
							{
								surfaceLevel = levelCount - 1;
							}

							surface = texture->getInternalSurfaceLevel(surfaceLevel);
							renderer->setTextureLevel(sampler, 0, mipmapLevel, surface, sw::TEXTURE_2D);
						}
					}
					break;
				case D3DRTYPE_CUBETEXTURE:
					for(int face = 0; face < 6; face++)
					{
						Direct3DCubeTexture9 *cubeTexture = dynamic_cast<Direct3DCubeTexture9*>(baseTexture);
						Direct3DSurface9 *surface;

						for(int mipmapLevel = 0; mipmapLevel < sw::MIPMAP_LEVELS; mipmapLevel++)
						{
							int surfaceLevel = mipmapLevel;

							if(surfaceLevel < LOD)
							{
								surfaceLevel = LOD;
							}

							if(surfaceLevel < 0)
							{
								surfaceLevel = 0;
							}
							else if(surfaceLevel >= levelCount)
							{
								surfaceLevel = levelCount - 1;
							}

							surface = cubeTexture->getInternalCubeMapSurface((D3DCUBEMAP_FACES)face, surfaceLevel);
							renderer->setTextureLevel(sampler, face, mipmapLevel, surface, sw::TEXTURE_CUBE);
						}
					}
					break;
				case D3DRTYPE_VOLUMETEXTURE:
					{
						Direct3DVolumeTexture9 *volumeTexture = dynamic_cast<Direct3DVolumeTexture9*>(baseTexture);
						Direct3DVolume9 *volume;

						for(int mipmapLevel = 0; mipmapLevel < sw::MIPMAP_LEVELS; mipmapLevel++)
						{
							int surfaceLevel = mipmapLevel;

							if(surfaceLevel < LOD)
							{
								surfaceLevel = LOD;
							}

							if(surfaceLevel < 0)
							{
								surfaceLevel = 0;
							}
							else if(surfaceLevel >= levelCount)
							{
								surfaceLevel = levelCount - 1;
							}

							volume = volumeTexture->getInternalVolumeLevel(surfaceLevel);
							renderer->setTextureLevel(sampler, 0, mipmapLevel, volume, sw::TEXTURE_3D);
						}
					}
					break;
				default:
					UNIMPLEMENTED();
				}
			}
			else
			{
				renderer->setTextureLevel(sampler, 0, 0, 0, sw::TEXTURE_NULL);
			}
		}
	}

	bool Direct3DDevice9::isRecording() const
	{
		return stateRecorder != 0;
	}

	void Direct3DDevice9::setOcclusionEnabled(bool enable)
	{
		renderer->setOcclusionEnabled(enable);
	}

	void Direct3DDevice9::removeQuery(sw::Query *query)
	{
		renderer->removeQuery(query);
	}

	void Direct3DDevice9::addQuery(sw::Query *query)
	{
		renderer->addQuery(query);
	}

	void Direct3DDevice9::stretchRect(Direct3DSurface9 *source, const RECT *sourceRect, Direct3DSurface9 *dest, const RECT *destRect, D3DTEXTUREFILTERTYPE filter)
	{
		D3DSURFACE_DESC sourceDescription;
		D3DSURFACE_DESC destDescription;

		source->GetDesc(&sourceDescription);
		dest->GetDesc(&destDescription);

		int sWidth = source->getWidth();
		int sHeight = source->getHeight();
		int dWidth = dest->getWidth();
		int dHeight = dest->getHeight();

		sw::Rect sRect(0, 0, sWidth, sHeight);
		sw::Rect dRect(0, 0, dWidth, dHeight);

		if(sourceRect)
		{
			sRect.x0 = sourceRect->left;
			sRect.y0 = sourceRect->top;
			sRect.x1 = sourceRect->right;
			sRect.y1 = sourceRect->bottom;
		}

		if(destRect)
		{
			dRect.x0 = destRect->left;
			dRect.y0 = destRect->top;
			dRect.x1 = destRect->right;
			dRect.y1 = destRect->bottom;
		}

		bool scaling = (sRect.x1 - sRect.x0 != dRect.x1 - dRect.x0) || (sRect.y1 - sRect.y0 != dRect.y1 - dRect.y0);
		bool equalFormats = source->getInternalFormat() == dest->getInternalFormat();
		bool depthStencil = (sourceDescription.Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL;
		bool alpha0xFF = false;

		if((sourceDescription.Format == D3DFMT_A8R8G8B8 && destDescription.Format == D3DFMT_X8R8G8B8) ||
		   (sourceDescription.Format == D3DFMT_X8R8G8B8 && destDescription.Format == D3DFMT_A8R8G8B8))
		{
			equalFormats = true;
			alpha0xFF = true;
		}

		if(depthStencil)   // Copy entirely, internally   // FIXME: Check
		{
			if(source->hasDepth())
			{
				byte *sourceBuffer = (byte*)source->lockInternal(0, 0, 0, sw::LOCK_READONLY, sw::PUBLIC);
				byte *destBuffer = (byte*)dest->lockInternal(0, 0, 0, sw::LOCK_DISCARD, sw::PUBLIC);

				unsigned int width = source->getWidth();
				unsigned int height = source->getHeight();
				unsigned int pitch = source->getInternalPitchB();

				for(unsigned int y = 0; y < height; y++)
				{
					memcpy(destBuffer, sourceBuffer, pitch);   // FIXME: Only copy width * bytes

					sourceBuffer += pitch;
					destBuffer += pitch;
				}

				source->unlockInternal();
				dest->unlockInternal();
			}

			if(source->hasStencil())
			{
				byte *sourceBuffer = (byte*)source->lockStencil(0, 0, 0, sw::PUBLIC);
				byte *destBuffer = (byte*)dest->lockStencil(0, 0, 0, sw::PUBLIC);

				unsigned int width = source->getWidth();
				unsigned int height = source->getHeight();
				unsigned int pitch = source->getStencilPitchB();

				for(unsigned int y = 0; y < height; y++)
				{
					memcpy(destBuffer, sourceBuffer, pitch);   // FIXME: Only copy width * bytes

					sourceBuffer += pitch;
					destBuffer += pitch;
				}

				source->unlockStencil();
				dest->unlockStencil();
			}
		}
		else if(!scaling && equalFormats)
		{
			unsigned char *sourceBytes = (unsigned char*)source->lockInternal(sRect.x0, sRect.y0, 0, sw::LOCK_READONLY, sw::PUBLIC);
			unsigned char *destBytes = (unsigned char*)dest->lockInternal(dRect.x0, dRect.y0, 0, sw::LOCK_READWRITE, sw::PUBLIC);
			unsigned int sourcePitch = source->getInternalPitchB();
			unsigned int destPitch = dest->getInternalPitchB();

			unsigned int width = dRect.x1 - dRect.x0;
			unsigned int height = dRect.y1 - dRect.y0;
			unsigned int bytes = width * sw::Surface::bytes(source->getInternalFormat());

			for(unsigned int y = 0; y < height; y++)
			{
				memcpy(destBytes, sourceBytes, bytes);

				if(alpha0xFF)
				{
					for(unsigned int x = 0; x < width; x++)
					{
						destBytes[4 * x + 3] = 0xFF;
					}
				}

				sourceBytes += sourcePitch;
				destBytes += destPitch;
			}

			source->unlockInternal();
			dest->unlockInternal();
		}
		else
		{
			sw::SliceRectF sRectF((float)sRect.x0, (float)sRect.y0, (float)sRect.x1, (float)sRect.y1, 0);
			renderer->blit(source, sRectF, dest, dRect, filter >= D3DTEXF_LINEAR);
		}
	}

	long Direct3DDevice9::updateVolume(IDirect3DVolume9 *sourceVolume, IDirect3DVolume9 *destinationVolume)
	{
		TRACE("IDirect3DVolume9 *sourceVolume = 0x%0.8p, IDirect3DVolume9 *destinationVolume = 0x%0.8p", sourceVolume, destinationVolume);

		if(!sourceVolume || !destinationVolume)
		{
			return INVALIDCALL();
		}

		D3DVOLUME_DESC sourceDescription;
		D3DVOLUME_DESC destinationDescription;

		sourceVolume->GetDesc(&sourceDescription);
		destinationVolume->GetDesc(&destinationDescription);

		if(sourceDescription.Pool      != D3DPOOL_SYSTEMMEM ||
		   destinationDescription.Pool != D3DPOOL_DEFAULT ||
		   sourceDescription.Format != destinationDescription.Format ||
		   sourceDescription.Width  != destinationDescription.Width ||
		   sourceDescription.Height != destinationDescription.Height ||
		   sourceDescription.Depth  != destinationDescription.Depth)
		{
			return INVALIDCALL();
		}

		sw::Surface *source = static_cast<Direct3DVolume9*>(sourceVolume);
		sw::Surface *dest = static_cast<Direct3DVolume9*>(destinationVolume);

		if(source->getExternalPitchB() != dest->getExternalPitchB() ||
		   source->getExternalSliceB() != dest->getExternalSliceB())
		{
			UNIMPLEMENTED();
		}

		void *sBuffer = source->lockExternal(0, 0, 0, sw::LOCK_READONLY, sw::PUBLIC);
		void *dBuffer = dest->lockExternal(0, 0, 0, sw::LOCK_WRITEONLY, sw::PUBLIC);

		memcpy(dBuffer, sBuffer, source->getExternalSliceB() * sourceDescription.Depth);

		source->unlockExternal();
		dest->unlockExternal();

		return D3D_OK;
	}

	bool Direct3DDevice9::validRectangle(const RECT *rect, IDirect3DSurface9 *surface)
	{
		if(!rect)
		{
			return true;
		}

		if(rect->right <= rect->left || rect->bottom <= rect->top)
		{
			return false;
		}

		if(rect->left < 0 || rect->top < 0)
		{
			return false;
		}

		D3DSURFACE_DESC description;
		surface->GetDesc(&description);

		if(rect->right > (int)description.Width || rect->bottom > (int)description.Height)
		{
			return false;
		}

		return true;
	}

	void Direct3DDevice9::configureFPU()
	{
	//	_controlfp(_PC_24, _MCW_PC);     // Single-precision
		_controlfp(_MCW_EM, _MCW_EM);    // Mask all exceptions
		_controlfp(_RC_NEAR, _MCW_RC);   // Round to nearest
	}
}
