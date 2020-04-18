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

#include "Direct3DDevice8.hpp"

#include "Direct3D8.hpp"
#include "Direct3DSurface8.hpp"
#include "Direct3DIndexBuffer8.hpp"
#include "Direct3DVertexBuffer8.hpp"
#include "Direct3DTexture8.hpp"
#include "Direct3DVolumeTexture8.hpp"
#include "Direct3DCubeTexture8.hpp"
#include "Direct3DSwapChain8.hpp"
#include "Direct3DPixelShader8.hpp"
#include "Direct3DVertexShader8.hpp"
#include "Direct3DVolume8.hpp"

#include "Debug.hpp"
#include "Capabilities.hpp"
#include "Renderer.hpp"
#include "Config.hpp"
#include "FrameBuffer.hpp"
#include "Clipper.hpp"
#include "Configurator.hpp"
#include "Timer.hpp"
#include "Resource.hpp"

#include <assert.h>

bool localShaderConstants = false;

namespace D3D8
{
	inline unsigned long FtoDW(float f)   // FIXME: Deprecate
	{
		return (unsigned long&)f;
	}

	Direct3DDevice8::Direct3DDevice8(const HINSTANCE instance, Direct3D8 *d3d8, unsigned int adapter, D3DDEVTYPE deviceType, HWND focusWindow, unsigned long behaviourFlags, D3DPRESENT_PARAMETERS *presentParameters) : instance(instance), d3d8(d3d8), adapter(adapter), deviceType(deviceType), focusWindow(focusWindow), behaviourFlags(behaviourFlags), presentParameters(*presentParameters)
	{
		init = true;
		recordState = false;

		d3d8->AddRef();

		context = new sw::Context();
		renderer = new sw::Renderer(context, sw::Direct3D, false);

		swapChain.push_back(0);
		depthStencil = 0;
		renderTarget = 0;

		for(int i = 0; i < 8; i++)
		{
			texture[i] = 0;
		}

		cursor = 0;
		unsigned char one[32 * 32 / sizeof(unsigned char)];
		memset(one, 0xFFFFFFFF, sizeof(one));
		unsigned char zero[32 * 32 / sizeof(unsigned char)] = {0};
		nullCursor = CreateCursor(instance, 0, 0, 32, 32, one, zero);
		win32Cursor = GetCursor();

		Reset(presentParameters);

		pixelShader.push_back(0);   // pixelShader[0] = 0
		vertexShader.push_back(0);   // vertexShader[0] = 0
		vertexShaderHandle = 0;
		pixelShaderHandle = 0;

		lightsDirty = true;

		for(int i = 0; i < 16; i++)
		{
			dataStream[i] = 0;
			streamStride[i] = 0;
		}

		indexData = 0;
		baseVertexIndex = 0;
		declaration = 0;
		FVF = 0;

		D3DMATERIAL8 material;

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

		for(int i = 0; i < 8; i++)
		{
			float zero[4] = {0, 0, 0, 0};

			SetPixelShaderConstant(i, zero, 1);
		}

		for(int i = 0; i < 256; i++)
		{
			float zero[4] = {0, 0, 0, 0};

			SetVertexShaderConstant(i, zero, 1);
		}

		init = false;

		if(!(behaviourFlags & D3DCREATE_FPU_PRESERVE))
		{
			configureFPU();
		}
	}

	Direct3DDevice8::~Direct3DDevice8()
	{
		delete renderer;
		renderer = 0;
		delete context;
		context = 0;

		d3d8->Release();
		d3d8 = 0;

		for(unsigned int i = 0; i < swapChain.size(); i++)
		{
			if(swapChain[i])
			{
				swapChain[i]->unbind();
				swapChain[i] = 0;
			}
		}

		if(depthStencil)
		{
			depthStencil->unbind();
			depthStencil = 0;
		}

		if(renderTarget)
		{
			renderTarget->unbind();
			renderTarget = 0;
		}

		for(int i = 0; i < 8; i++)
		{
			if(texture[i])
			{
				texture[i]->unbind();
				texture[i] = 0;
			}
		}

		for(int i = 0; i < 16; i++)
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

		for(unsigned int i = 0; i < pixelShader.size(); i++)
		{
			if(pixelShader[i])
			{
				pixelShader[i]->unbind();
				pixelShader[i] = 0;
			}
		}

		for(unsigned int i = 0; i < vertexShader.size(); i++)
		{
			if(vertexShader[i])
			{
				vertexShader[i]->unbind();
				vertexShader[i] = 0;
			}
		}

		for(unsigned int i = 0; i < stateRecorder.size(); i++)
		{
			if(stateRecorder[i])
			{
				stateRecorder[i]->unbind();
				stateRecorder[i] = 0;
			}
		}

		palette.clear();

		delete cursor;
		DestroyCursor(nullCursor);
	}

	long Direct3DDevice8::QueryInterface(const IID &iid, void **object)
	{
		TRACE("");

		if(iid == IID_IDirect3DDevice8 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DDevice8::AddRef()
	{
		TRACE("");

		return Unknown::AddRef();
	}

	unsigned long Direct3DDevice8::Release()
	{
		TRACE("");

		return Unknown::Release();
	}

	long Direct3DDevice8::ApplyStateBlock(unsigned long token)
	{
		TRACE("");

		stateRecorder[token]->Apply();

		return D3D_OK;
	}

	long Direct3DDevice8::BeginScene()
	{
		TRACE("");

		return D3D_OK;
	}

	long Direct3DDevice8::BeginStateBlock()
	{
		TRACE("");

		recordState = true;
		Direct3DStateBlock8 *stateBlock = new Direct3DStateBlock8(this, (D3DSTATEBLOCKTYPE)0);
		stateBlock->bind();
		stateRecorder.push_back(stateBlock);

		return D3D_OK;
	}

	long Direct3DDevice8::CaptureStateBlock(unsigned long token)
	{
		TRACE("");

		stateRecorder[token]->Capture();

		return D3D_OK;
	}

	long Direct3DDevice8::Clear(unsigned long count, const D3DRECT *rects, unsigned long flags, unsigned long color, float z, unsigned long stencil)
	{
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
				break;
			case D3DFMT_D16_LOCKABLE:
			case D3DFMT_D32:
			case D3DFMT_D16:
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

			if(flags & D3DCLEAR_TARGET)
			{
				if(renderTarget)
				{
					D3DSURFACE_DESC description;
					renderTarget->GetDesc(&description);

					float rgba[4];
					rgba[0] = (float)(color & 0x00FF0000) / 0x00FF0000;
					rgba[1] = (float)(color & 0x0000FF00) / 0x0000FF00;
					rgba[2] = (float)(color & 0x000000FF) / 0x000000FF;
					rgba[3] = (float)(color & 0xFF000000) / 0xFF000000;

					renderer->clear(rgba, sw::FORMAT_A32B32G32R32F, renderTarget, clearRect, 0xF);
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

	long Direct3DDevice8::CopyRects(IDirect3DSurface8 *sourceSurface, const RECT *sourceRectsArray, unsigned int rects, IDirect3DSurface8 *destinationSurface, const POINT *destPointsArray)
	{
		TRACE("");

		if(!sourceSurface || !destinationSurface)
		{
			return INVALIDCALL();
		}

		if(sourceRectsArray && rects == 0 || !sourceRectsArray && rects > 0)
		{
			return INVALIDCALL();   // FIXME: Verify REF behaviour
		}

		D3DSURFACE_DESC sourceDescription;
		D3DSURFACE_DESC destDescription;

		sourceSurface->GetDesc(&sourceDescription);
		destinationSurface->GetDesc(&destDescription);

		if(sourceDescription.Format != destDescription.Format)
		{
			return INVALIDCALL();
		}

		int sWidth = sourceDescription.Width;
		int sHeight = sourceDescription.Height;
		int dWidth = destDescription.Width;
		int dHeight = destDescription.Height;

		RECT sRect = {0, 0, sWidth, sHeight};
		POINT dPoint = {0, 0};

		if(!sourceRectsArray || !destPointsArray)
		{
			sourceRectsArray = &sRect;
			destPointsArray = &dPoint;

			rects = 1;
		}

		int bpp = 8 * Direct3DSurface8::bytes(sourceDescription.Format);

		for(unsigned int i = 0; i < rects; i++)
		{
			const RECT &sRect = sourceRectsArray[i];
			const POINT &dPoint = destPointsArray[i];

			int rWidth = sRect.right - sRect.left;
			int rHeight = sRect.bottom - sRect.top;

			RECT dRect;

			dRect.top = dPoint.y;
			dRect.left = dPoint.x;
			dRect.bottom = dPoint.y + rHeight;
			dRect.right = dPoint.x + rWidth;

			D3DLOCKED_RECT sourceLock;
			D3DLOCKED_RECT destLock;

			sourceSurface->LockRect(&sourceLock, &sRect, D3DLOCK_READONLY);
			destinationSurface->LockRect(&destLock, &dRect, D3DLOCK_DISCARD);

			for(int y = 0; y < rHeight; y++)
			{
				switch(sourceDescription.Format)
				{
				case D3DFMT_DXT1:
				case D3DFMT_DXT2:
				case D3DFMT_DXT3:
				case D3DFMT_DXT4:
				case D3DFMT_DXT5:
					memcpy(destLock.pBits, sourceLock.pBits, rWidth * bpp / 8);
					y += 3;   // Advance four lines at once
					break;
				default:
					memcpy(destLock.pBits, sourceLock.pBits, rWidth * bpp / 8);
				}

				(char*&)sourceLock.pBits += sourceLock.Pitch;
				(char*&)destLock.pBits += destLock.Pitch;
			}

			sourceSurface->UnlockRect();
			destinationSurface->UnlockRect();
		}

		return D3D_OK;
	}

	long Direct3DDevice8::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS *presentParameters, IDirect3DSwapChain8 **swapChain)
	{
		TRACE("");

		*swapChain = 0;

		if(!presentParameters || !swapChain)
		{
			return INVALIDCALL();
		}

		if(presentParameters->BackBufferCount > 3)
		{
			return INVALIDCALL();   // Maximum of three back buffers
		}

		if(presentParameters->BackBufferCount == 0)
		{
			presentParameters->BackBufferCount = 1;
		}

		D3DPRESENT_PARAMETERS present = *presentParameters;

		*swapChain = new Direct3DSwapChain8(this, &present);

		if(!*swapChain)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *swapChain;

			return OUTOFVIDEOMEMORY();
		}

		this->swapChain.push_back(static_cast<Direct3DSwapChain8*>(*swapChain));

		(*swapChain)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice8::CreateCubeTexture(unsigned int edgeLength, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DCubeTexture8 **cubeTexture)
	{
		TRACE("");

		*cubeTexture = 0;

		if(edgeLength == 0 || d3d8->CheckDeviceFormat(adapter, deviceType, D3DFMT_X8R8G8B8, usage, D3DRTYPE_CUBETEXTURE, format) != D3D_OK)
		{
			return INVALIDCALL();
		}

		*cubeTexture = new Direct3DCubeTexture8(this, edgeLength, levels, usage, format, pool);

		if(!*cubeTexture)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *cubeTexture;

			return OUTOFVIDEOMEMORY();
		}

		(*cubeTexture)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice8::CreateDepthStencilSurface(unsigned int width, unsigned int height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multiSample, IDirect3DSurface8 **surface)
	{
		TRACE("");

		*surface = 0;

		if(width == 0 || height == 0 || d3d8->CheckDeviceFormat(adapter, deviceType, D3DFMT_X8R8G8B8, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, format) != D3D_OK || height > sw::OUTLINE_RESOLUTION)
		{
			return INVALIDCALL();
		}

		*surface = new Direct3DSurface8(this, this, width, height, format, D3DPOOL_DEFAULT, multiSample, format == D3DFMT_D16_LOCKABLE, D3DUSAGE_DEPTHSTENCIL);

		if(!*surface)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *surface;

			return OUTOFVIDEOMEMORY();
		}

		(*surface)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice8::CreateImageSurface(unsigned int width, unsigned int height, D3DFORMAT format, IDirect3DSurface8 **surface)
	{
		TRACE("");

		*surface = 0;

		if(width == 0 || height == 0 || d3d8->CheckDeviceFormat(adapter, deviceType, D3DFMT_X8R8G8B8, 0, D3DRTYPE_SURFACE, format) != D3D_OK)
		{
			return INVALIDCALL();
		}

		*surface = new Direct3DSurface8(this, this, width, height, format, D3DPOOL_SYSTEMMEM, D3DMULTISAMPLE_NONE, true, 0);

		if(!*surface)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *surface;

			return OUTOFVIDEOMEMORY();
		}

		(*surface)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice8::CreateIndexBuffer(unsigned int length, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DIndexBuffer8 **indexBuffer)
	{
		TRACE("");

		*indexBuffer = new Direct3DIndexBuffer8(this, length, usage, format, pool);

		if(!*indexBuffer)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *indexBuffer;

			return OUTOFVIDEOMEMORY();
		}

		(*indexBuffer)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice8::CreatePixelShader(const unsigned long *function, unsigned long *handle)
	{
		TRACE("");

		if(!function || !handle || function[0] > pixelShaderVersion)
		{
			return INVALIDCALL();
		}

		unsigned int index;

		for(index = 1; index < pixelShader.size(); index++)   // Skip NULL handle
		{
			if(pixelShader[index] == 0)
			{
				pixelShader[index] = new Direct3DPixelShader8(this, function);   // FIXME: Check for null

				break;
			}
		}

		if(index == pixelShader.size())
		{
			pixelShader.push_back(new Direct3DPixelShader8(this, function));
		}

		pixelShader[index]->AddRef();

		*handle = index;

		return D3D_OK;
	}

	long Direct3DDevice8::CreateRenderTarget(unsigned int width, unsigned int height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multiSample, int lockable, IDirect3DSurface8 **surface)
	{
		TRACE("");

		*surface = 0;

		if(width == 0 || height == 0 || d3d8->CheckDeviceFormat(adapter, deviceType, D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, format) != D3D_OK || height > sw::OUTLINE_RESOLUTION)
		{
			return INVALIDCALL();
		}

		*surface = new Direct3DSurface8(this, this, width, height, format, D3DPOOL_DEFAULT, multiSample, lockable != FALSE, D3DUSAGE_RENDERTARGET);

		if(!*surface)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *surface;

			return OUTOFVIDEOMEMORY();
		}

		(*surface)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice8::CreateStateBlock(D3DSTATEBLOCKTYPE type, unsigned long *token)
	{
		TRACE("");

		if(!token)
		{
			return INVALIDCALL();
		}

		Direct3DStateBlock8 *stateBlock = new Direct3DStateBlock8(this, type);
		stateBlock->bind();
		stateRecorder.push_back(stateBlock);
		*token = (unsigned long)(stateRecorder.size() - 1);

		return D3D_OK;
	}

	long Direct3DDevice8::CreateTexture(unsigned int width, unsigned int height, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture8 **texture)
	{
		TRACE("");

		*texture = 0;

		if(width == 0 || height == 0 || d3d8->CheckDeviceFormat(adapter, deviceType, D3DFMT_X8R8G8B8, usage, D3DRTYPE_TEXTURE, format) != D3D_OK)
		{
			return INVALIDCALL();
		}

		*texture = new Direct3DTexture8(this, width, height, levels, usage, format, pool);

		if(!*texture)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *texture;

			return OUTOFVIDEOMEMORY();
		}

		(*texture)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice8::CreateVertexBuffer(unsigned int length, unsigned long usage, unsigned long FVF, D3DPOOL pool, IDirect3DVertexBuffer8 **vertexBuffer)
	{
		TRACE("");

		*vertexBuffer = new Direct3DVertexBuffer8(this, length, usage, FVF, pool);

		if(!*vertexBuffer)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *vertexBuffer;

			return OUTOFVIDEOMEMORY();
		}

		(*vertexBuffer)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice8::CreateVertexShader(const unsigned long *declaration, const unsigned long *function, unsigned long *handle, unsigned long usage)
	{
		TRACE("const unsigned long *declaration = 0x%0.8p, const unsigned long *function = 0x%0.8p, unsigned long *handle = 0x%0.8p, unsigned long usage = %d", declaration, function, handle, usage);

		if(!declaration || !handle || (function && function[0] > vertexShaderVersion))
		{
			return INVALIDCALL();
		}

		unsigned int index;

		for(index = 1; index < vertexShader.size(); index++)   // NOTE: skip NULL handle
		{
			if(vertexShader[index] == 0)
			{
				vertexShader[index] = new Direct3DVertexShader8(this, declaration, function);   // FIXME: Check for null

				break;
			}
		}

		if(index == vertexShader.size())
		{
			vertexShader.push_back(new Direct3DVertexShader8(this, declaration, function));
		}

		vertexShader[index]->AddRef();

		*handle = (index << 16) + 1;

		return D3D_OK;
	}

	long Direct3DDevice8::CreateVolumeTexture(unsigned int width, unsigned int height, unsigned int depth, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DVolumeTexture8 **volumeTexture)
	{
		TRACE("");

		*volumeTexture = 0;

		if(width == 0 || height == 0 || depth == 0 || d3d8->CheckDeviceFormat(adapter, deviceType, D3DFMT_X8R8G8B8, usage, D3DRTYPE_VOLUMETEXTURE, format) != D3D_OK)
		{
			return INVALIDCALL();
		}

		*volumeTexture = new Direct3DVolumeTexture8(this, width, height, depth, levels, usage, format, pool);

		if(!*volumeTexture)
		{
			return OUTOFMEMORY();
		}

		if(GetAvailableTextureMem() == 0)
		{
			delete *volumeTexture;

			return OUTOFVIDEOMEMORY();
		}

		(*volumeTexture)->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice8::DeletePatch(unsigned int handle)
	{
		TRACE("");

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DDevice8::DeleteStateBlock(unsigned long token)
	{
		TRACE("");

		if(token >= stateRecorder.size() || !stateRecorder[token])
		{
			return INVALIDCALL();
		}

		stateRecorder[token]->unbind();
		stateRecorder[token] = 0;

		return D3D_OK;
	}

	long Direct3DDevice8::DeleteVertexShader(unsigned long handle)
	{
		TRACE("");

		unsigned int index = handle >> 16;

		if(index >= vertexShader.size() || !vertexShader[index])
		{
			return INVALIDCALL();
		}

		vertexShader[index]->Release();
		vertexShader[index] = 0;

		return D3D_OK;
	}

	long Direct3DDevice8::DrawIndexedPrimitive(D3DPRIMITIVETYPE type, unsigned int minIndex, unsigned int numVertices, unsigned int startIndex, unsigned int primitiveCount)
	{
		TRACE("");

		if(!indexData)
		{
			return INVALIDCALL();
		}

		if(!bindData(indexData, baseVertexIndex) || !primitiveCount)
		{
			return D3D_OK;
		}

		unsigned int indexOffset = startIndex * (indexData->is32Bit() ? 4 : 2);   // FIXME: Doesn't take stream frequencies into account

		sw::DrawType drawType;

		if(indexData->is32Bit())
		{
			switch(type)
			{
			case D3DPT_POINTLIST:		drawType = sw::DRAW_INDEXEDPOINTLIST32;		break;
			case D3DPT_LINELIST:		drawType = sw::DRAW_INDEXEDLINELIST32;			break;
			case D3DPT_LINESTRIP:		drawType = sw::DRAW_INDEXEDLINESTRIP32;		break;
			case D3DPT_TRIANGLELIST:	drawType = sw::DRAW_INDEXEDTRIANGLELIST32;		break;
			case D3DPT_TRIANGLESTRIP:	drawType = sw::DRAW_INDEXEDTRIANGLESTRIP32;	break;
			case D3DPT_TRIANGLEFAN:		drawType = sw::DRAW_INDEXEDTRIANGLEFAN32;		break;
			default:
				ASSERT(false);
			}
		}
		else
		{
			switch(type)
			{
			case D3DPT_POINTLIST:		drawType = sw::DRAW_INDEXEDPOINTLIST16;		break;
			case D3DPT_LINELIST:		drawType = sw::DRAW_INDEXEDLINELIST16;			break;
			case D3DPT_LINESTRIP:		drawType = sw::DRAW_INDEXEDLINESTRIP16;		break;
			case D3DPT_TRIANGLELIST:	drawType = sw::DRAW_INDEXEDTRIANGLELIST16;		break;
			case D3DPT_TRIANGLESTRIP:	drawType = sw::DRAW_INDEXEDTRIANGLESTRIP16;	break;
			case D3DPT_TRIANGLEFAN:		drawType = sw::DRAW_INDEXEDTRIANGLEFAN16;		break;
			default:
				ASSERT(false);
			}
		}

		bindData(indexData, baseVertexIndex);

		renderer->draw(drawType, indexOffset, primitiveCount);

		return D3D_OK;
	}

	long Direct3DDevice8::DeletePixelShader(unsigned long handle)
	{
		TRACE("");

		if(handle >= pixelShader.size() || !pixelShader[handle])
		{
			return INVALIDCALL();
		}

		pixelShader[handle]->Release();
		pixelShader[handle] = 0;

		return D3D_OK;
	}

	long Direct3DDevice8::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE type, unsigned int minIndex, unsigned int numVertices, unsigned int primitiveCount, const void *indexData, D3DFORMAT indexDataFormat, const void *vertexStreamZeroData, unsigned int vertexStreamZeroStride)
	{
		TRACE("");

		if(!vertexStreamZeroData || !indexData)
		{
			return INVALIDCALL();
		}

		int length = (minIndex + numVertices) * vertexStreamZeroStride;

		Direct3DVertexBuffer8 *vertexBuffer = new Direct3DVertexBuffer8(this, length, 0, 0, D3DPOOL_DEFAULT);

		unsigned char *data;
		vertexBuffer->Lock(0, 0, &data, 0);
		memcpy(data, vertexStreamZeroData, length);
		vertexBuffer->Unlock();

		SetStreamSource(0, vertexBuffer, vertexStreamZeroStride);

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

		Direct3DIndexBuffer8 *indexBuffer = new Direct3DIndexBuffer8(this, length, 0, indexDataFormat, D3DPOOL_DEFAULT);

		indexBuffer->Lock(0, 0, &data, 0);
		memcpy(data, indexData, length);
		indexBuffer->Unlock();

		SetIndices(indexBuffer, 0);

		if(!bindData(indexBuffer, 0) || !primitiveCount)
		{
			vertexBuffer->Release();

			return D3D_OK;
		}

		sw::DrawType drawType;

		if(indexDataFormat == D3DFMT_INDEX32)
		{
			switch(type)
			{
			case D3DPT_POINTLIST:		drawType = sw::DRAW_INDEXEDPOINTLIST32;		break;
			case D3DPT_LINELIST:		drawType = sw::DRAW_INDEXEDLINELIST32;			break;
			case D3DPT_LINESTRIP:		drawType = sw::DRAW_INDEXEDLINESTRIP32;		break;
			case D3DPT_TRIANGLELIST:	drawType = sw::DRAW_INDEXEDTRIANGLELIST32;		break;
			case D3DPT_TRIANGLESTRIP:	drawType = sw::DRAW_INDEXEDTRIANGLESTRIP32;	break;
			case D3DPT_TRIANGLEFAN:		drawType = sw::DRAW_INDEXEDTRIANGLEFAN32;		break;
			default:
				ASSERT(false);
			}
		}
		else
		{
			switch(type)
			{
			case D3DPT_POINTLIST:		drawType = sw::DRAW_INDEXEDPOINTLIST16;		break;
			case D3DPT_LINELIST:		drawType = sw::DRAW_INDEXEDLINELIST16;			break;
			case D3DPT_LINESTRIP:		drawType = sw::DRAW_INDEXEDLINESTRIP16;		break;
			case D3DPT_TRIANGLELIST:	drawType = sw::DRAW_INDEXEDTRIANGLELIST16;		break;
			case D3DPT_TRIANGLESTRIP:	drawType = sw::DRAW_INDEXEDTRIANGLESTRIP16;	break;
			case D3DPT_TRIANGLEFAN:		drawType = sw::DRAW_INDEXEDTRIANGLEFAN16;		break;
			default:
				ASSERT(false);
			}
		}

		renderer->draw(drawType, 0, primitiveCount);

		SetStreamSource(0, 0, 0);
		SetIndices(0, 0);

		return D3D_OK;
	}

	long Direct3DDevice8::DrawPrimitive(D3DPRIMITIVETYPE primitiveType, unsigned int startVertex, unsigned int primitiveCount)
	{
		TRACE("");

		if(!bindData(0, startVertex) || !primitiveCount)
		{
			return D3D_OK;
		}

		sw::DrawType drawType;

		switch(primitiveType)
		{
		case D3DPT_POINTLIST:		drawType = sw::DRAW_POINTLIST;		break;
		case D3DPT_LINELIST:		drawType = sw::DRAW_LINELIST;		break;
		case D3DPT_LINESTRIP:		drawType = sw::DRAW_LINESTRIP;		break;
		case D3DPT_TRIANGLELIST:	drawType = sw::DRAW_TRIANGLELIST;	break;
		case D3DPT_TRIANGLESTRIP:	drawType = sw::DRAW_TRIANGLESTRIP;	break;
		case D3DPT_TRIANGLEFAN:		drawType = sw::DRAW_TRIANGLEFAN;	break;
		default:
			ASSERT(false);
		}

		renderer->draw(drawType, 0, primitiveCount);

		return D3D_OK;
	}

	long Direct3DDevice8::DrawPrimitiveUP(D3DPRIMITIVETYPE primitiveType, unsigned int primitiveCount, const void *vertexStreamZeroData, unsigned int vertexStreamZeroStride)
	{
		TRACE("");

		if(!vertexStreamZeroData)
		{
			return INVALIDCALL();
		}

		IDirect3DVertexBuffer8 *vertexBuffer = 0;
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

		CreateVertexBuffer(length, 0, 0, D3DPOOL_DEFAULT, &vertexBuffer);

		unsigned char *data;
		vertexBuffer->Lock(0, 0, &data, 0);
		memcpy(data, vertexStreamZeroData, length);
		vertexBuffer->Unlock();

		SetStreamSource(0, vertexBuffer, vertexStreamZeroStride);

		if(!bindData(0, 0) || !primitiveCount)
		{
			vertexBuffer->Release();

			return D3D_OK;
		}

		sw::DrawType drawType;

		switch(primitiveType)
		{
		case D3DPT_POINTLIST:		drawType = sw::DRAW_POINTLIST;		break;
		case D3DPT_LINELIST:		drawType = sw::DRAW_LINELIST;		break;
		case D3DPT_LINESTRIP:		drawType = sw::DRAW_LINESTRIP;		break;
		case D3DPT_TRIANGLELIST:	drawType = sw::DRAW_TRIANGLELIST;	break;
		case D3DPT_TRIANGLESTRIP:	drawType = sw::DRAW_TRIANGLESTRIP;	break;
		case D3DPT_TRIANGLEFAN:		drawType = sw::DRAW_TRIANGLEFAN;	break;
		default:
			ASSERT(false);
		}

		renderer->draw(drawType, 0, primitiveCount);

		SetStreamSource(0, 0, 0);
		vertexBuffer->Release();

		return D3D_OK;
	}

	long Direct3DDevice8::DrawRectPatch(unsigned int handle, const float *numSegs, const D3DRECTPATCH_INFO *rectPatchInfo)
	{
		TRACE("");

		if(!numSegs || !rectPatchInfo)
		{
			return INVALIDCALL();
		}

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DDevice8::DrawTriPatch(unsigned int handle, const float *numSegs, const D3DTRIPATCH_INFO *triPatchInfo)
	{
		TRACE("");

		if(!numSegs || !triPatchInfo)
		{
			return INVALIDCALL();
		}

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DDevice8::EndScene()
	{
		TRACE("");

		return D3D_OK;
	}

	long Direct3DDevice8::EndStateBlock(unsigned long *token)
	{
		TRACE("");

		if(!token)
		{
			return INVALIDCALL();
		}

		recordState = false;
		*token = (unsigned long)(stateRecorder.size() - 1);

		return D3D_OK;
	}

	unsigned int Direct3DDevice8::GetAvailableTextureMem()
	{
		TRACE("");

		int availableMemory = textureMemory - Direct3DResource8::getMemoryUsage();
		if(availableMemory < 0) availableMemory = 0;

		// Round to nearest MB
		return (availableMemory + 0x80000) & 0xFFF00000;
	}

	long Direct3DDevice8::GetBackBuffer(unsigned int index, D3DBACKBUFFER_TYPE type, IDirect3DSurface8 **backBuffer)
	{
		TRACE("");

		if(!backBuffer/* || type != D3DBACKBUFFER_TYPE_MONO*/)
		{
			return INVALIDCALL();
		}

		swapChain[index]->GetBackBuffer(index, type, backBuffer);

		return D3D_OK;
	}

	long Direct3DDevice8::GetClipPlane(unsigned long index, float *plane)
	{
		TRACE("");

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

	long Direct3DDevice8::GetClipStatus(D3DCLIPSTATUS8 *clipStatus)
	{
		TRACE("");

		if(!clipStatus)
		{
			return INVALIDCALL();
		}

		*clipStatus = this->clipStatus;

		return D3D_OK;
	}

	long Direct3DDevice8::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *parameters)
	{
		TRACE("");

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

	long Direct3DDevice8::GetCurrentTexturePalette(unsigned int *paletteNumber)
	{
		TRACE("");

		if(!paletteNumber)
		{
			return INVALIDCALL();
		}

		*paletteNumber = currentPalette;

		return D3D_OK;
	}

	long Direct3DDevice8::GetDepthStencilSurface(IDirect3DSurface8 **depthStencilSurface)
	{
		TRACE("");

		if(!depthStencilSurface)
		{
			return INVALIDCALL();
		}

		*depthStencilSurface = depthStencil;

		if(depthStencil)
		{
			depthStencil->AddRef();
		}

		return D3D_OK;   // FIXME: Return NOTFOUND() when no depthStencil?
	}

	long Direct3DDevice8::GetDeviceCaps(D3DCAPS8 *caps)
	{
		TRACE("");

		return d3d8->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, caps);
	}

	long Direct3DDevice8::GetDirect3D(IDirect3D8 **d3d8)
	{
		TRACE("");

		if(!d3d8)
		{
			return INVALIDCALL();
		}

		ASSERT(this->d3d8);

		*d3d8 = this->d3d8;
		this->d3d8->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice8::GetDisplayMode(D3DDISPLAYMODE *mode)
	{
		TRACE("");

		if(!mode)
		{
			return INVALIDCALL();
		}

		d3d8->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, mode);

		return D3D_OK;
	}

	long Direct3DDevice8::GetFrontBuffer(IDirect3DSurface8 *destSurface)
	{
		TRACE("");

		if(!destSurface)
		{
			return INVALIDCALL();
		}

		D3DLOCKED_RECT description;
		destSurface->LockRect(&description, 0, 0);

		swapChain[0]->screenshot(description.pBits);

		destSurface->UnlockRect();

		return D3D_OK;
	}

	void Direct3DDevice8::GetGammaRamp(D3DGAMMARAMP *ramp)
	{
		TRACE("");

		if(!ramp)
		{
			return;
		}

		swapChain[0]->getGammaRamp((sw::GammaRamp*)ramp);
	}

	long Direct3DDevice8::GetIndices(IDirect3DIndexBuffer8 **indexData, unsigned int *baseVertexIndex)
	{
		TRACE("");

		if(!indexData || !baseVertexIndex)
		{
			return INVALIDCALL();
		}

		*indexData = this->indexData;

		if(this->indexData)
		{
			this->indexData->AddRef();
		}

		*baseVertexIndex = this->baseVertexIndex;

		return D3D_OK;
	}

	long Direct3DDevice8::GetInfo(unsigned long devInfoID, void *devInfoStruct, unsigned long devInfoStructSize)
	{
		TRACE("");

		if(!devInfoStruct || devInfoStructSize == 0)
		{
			return INVALIDCALL();
		}

		switch(devInfoID)
		{
		case 0: return E_FAIL;
		case 1: return E_FAIL;
		case 2: return E_FAIL;
		case 3: return E_FAIL;
		case 4: return S_FALSE;
		case 5: UNIMPLEMENTED();   // FIXME: D3DDEVINFOID_RESOURCEMANAGER
		case 6: UNIMPLEMENTED();   // FIXME: D3DDEVINFOID_D3DVERTEXSTATS
		case 7: return E_FAIL;
		}

		return D3D_OK;
	}

	long Direct3DDevice8::GetLight(unsigned long index, D3DLIGHT8 *light)
	{
		TRACE("");

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

	long Direct3DDevice8::GetLightEnable(unsigned long index , int *enable)
	{
		TRACE("");

		if(!enable)
		{
			return INVALIDCALL();
		}

		if(!light.exists(index))
		{
			return INVALIDCALL();
		}

		*enable = light[index].enable;

		return D3D_OK;
	}

	long Direct3DDevice8::GetMaterial(D3DMATERIAL8 *material)
	{
		TRACE("");

		if(!material)
		{
			return INVALIDCALL();
		}

		*material = this->material;

		return D3D_OK;
	}

	long Direct3DDevice8::GetPaletteEntries(unsigned int paletteNumber, PALETTEENTRY *entries)
	{
		TRACE("");

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

	long Direct3DDevice8::GetPixelShader(unsigned long *handle)
	{
		TRACE("");

		if(!handle)
		{
			return INVALIDCALL();
		}

		*handle = pixelShaderHandle;

		return D3D_OK;
	}

	long Direct3DDevice8::GetPixelShaderFunction(unsigned long handle, void *data, unsigned long *size)
	{
		TRACE("");

		if(!data)
		{
			return INVALIDCALL();
		}

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DDevice8::GetPixelShaderConstant(unsigned long startRegister, void *constantData, unsigned long count)
	{
		TRACE("");

		if(!constantData)
		{
			return INVALIDCALL();
		}

		for(unsigned int i = 0; i < count; i++)
		{
			((float*)constantData)[i * 4 + 0] = pixelShaderConstant[startRegister + i][0];
			((float*)constantData)[i * 4 + 1] = pixelShaderConstant[startRegister + i][1];
			((float*)constantData)[i * 4 + 2] = pixelShaderConstant[startRegister + i][2];
			((float*)constantData)[i * 4 + 3] = pixelShaderConstant[startRegister + i][3];
		}

		return D3D_OK;
	}

	long Direct3DDevice8::GetRasterStatus(D3DRASTER_STATUS *rasterStatus)
	{
		TRACE("");

		if(!rasterStatus)
		{
			return INVALIDCALL();
		}

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DDevice8::GetRenderState(D3DRENDERSTATETYPE state, unsigned long *value)
	{
		TRACE("");

		if(!value)
		{
			return INVALIDCALL();
		}

		*value = renderState[state];

		return D3D_OK;
	}

	long Direct3DDevice8::GetRenderTarget(IDirect3DSurface8 **renderTarget)
	{
		TRACE("");

		if(!renderTarget)
		{
			return INVALIDCALL();
		}

		*renderTarget = this->renderTarget;
		this->renderTarget->AddRef();

		return D3D_OK;
	}

	long Direct3DDevice8::GetStreamSource(unsigned int streamNumber, IDirect3DVertexBuffer8 **streamData, unsigned int *stride)
	{
		TRACE("");

		if(streamNumber >= 16 || !streamData || !stride)
		{
			return INVALIDCALL();
		}

		*streamData = dataStream[streamNumber];

		if(dataStream[streamNumber])
		{
			dataStream[streamNumber]->AddRef();
		}

		*stride = 0;   // NOTE: Unimplemented

		return D3D_OK;
	}

	long Direct3DDevice8::GetTexture(unsigned long stage, IDirect3DBaseTexture8 **texture)
	{
		TRACE("");

		if(!texture || stage >= 8)
		{
			return INVALIDCALL();
		}

		*texture = this->texture[stage];

		if(this->texture[stage])
		{
			this->texture[stage]->AddRef();
		}

		return D3D_OK;
	}

	long Direct3DDevice8::GetTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE state, unsigned long *value)
	{
		TRACE("");

		if(!value  || stage < 0 || stage >= 8 || state < 0 || state > D3DTSS_RESULTARG)   // FIXME: Set *value to 0?
		{
			return INVALIDCALL();
		}

		*value = textureStageState[stage][state];

		return D3D_OK;
	}

	long Direct3DDevice8::GetTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
	{
		TRACE("");

		if(!matrix || state < 0 || state > 511)
		{
			return INVALIDCALL();
		}

		*matrix = this->matrix[state];

		return D3D_OK;
	}

	long Direct3DDevice8::GetVertexShader(unsigned long *handle)
	{
		TRACE("");

		if(!handle)
		{
			return INVALIDCALL();
		}

		*handle = vertexShaderHandle;

		return D3D_OK;
	}

	long Direct3DDevice8::GetVertexShaderConstant(unsigned long startRegister, void *constantData, unsigned long count)
	{
		TRACE("");

		if(!constantData)
		{
			return INVALIDCALL();
		}

		for(unsigned int i = 0; i < count; i++)
		{
			((float*)constantData)[i * 4 + 0] = vertexShaderConstant[startRegister + i][0];
			((float*)constantData)[i * 4 + 1] = vertexShaderConstant[startRegister + i][1];
			((float*)constantData)[i * 4 + 2] = vertexShaderConstant[startRegister + i][2];
			((float*)constantData)[i * 4 + 3] = vertexShaderConstant[startRegister + i][3];
		}

		return D3D_OK;
	}

	long Direct3DDevice8::GetVertexShaderDeclaration(unsigned long handle, void *data, unsigned long *size)
	{
		TRACE("");

		if(!data || !size)
		{
			return INVALIDCALL();
		}

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DDevice8::GetVertexShaderFunction(unsigned long handle, void *data, unsigned long *size)
	{
		TRACE("");

		if(!data || !size)
		{
			return INVALIDCALL();
		}

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DDevice8::GetViewport(D3DVIEWPORT8 *viewport)
	{
		TRACE("");

		if(!viewport)
		{
			return INVALIDCALL();
		}

		*viewport = this->viewport;

		return D3D_OK;
	}

	long Direct3DDevice8::LightEnable(unsigned long index, int enable)
	{
		TRACE("");

		if(!recordState)
		{
			if(!light.exists(index))   // Insert default light
			{
				D3DLIGHT8 light;

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

				SetLight(index, &light);
			}

			light[index].enable = (enable != FALSE);

			lightsDirty = true;
		}
		else
		{
			stateRecorder.back()->lightEnable(index, enable);
		}

		return D3D_OK;
	}

	long Direct3DDevice8::MultiplyTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix)
	{
		TRACE("");

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

	long Direct3DDevice8::Present(const RECT *sourceRect, const RECT *destRect, HWND destWindowOverride, const RGNDATA *dirtyRegion)
	{
		TRACE("");

		// NOTE: sourceRect and destRect can be null, dirtyRegion has to be null

		HWND windowHandle = presentParameters.hDeviceWindow ? presentParameters.hDeviceWindow : focusWindow;

		if(destWindowOverride && destWindowOverride != windowHandle)
		{
			UNIMPLEMENTED();
		}

		if(dirtyRegion)
		{
			return INVALIDCALL();
		}

		swapChain[0]->Present(sourceRect, destRect, destWindowOverride, dirtyRegion);

		return D3D_OK;
	}

	long Direct3DDevice8::ProcessVertices(unsigned int srcStartIndex, unsigned int destIndex, unsigned int vertexCount, IDirect3DVertexBuffer8 *destBuffer, unsigned long flags)
	{
		TRACE("");

		if(!destBuffer)
		{
			return INVALIDCALL();
		}

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DDevice8::Reset(D3DPRESENT_PARAMETERS *presentParameters)
	{
		TRACE("");

		if(!presentParameters)
		{
			return INVALIDCALL();
		}

		if(swapChain[0])
		{
			swapChain[0]->unbind();
			swapChain[0] = 0;
		}

		if(depthStencil)
		{
			depthStencil->unbind();
			depthStencil = 0;
		}

		if(renderTarget)
		{
			renderTarget->unbind();
			renderTarget = 0;
		}

		D3DPRESENT_PARAMETERS present = *presentParameters;

		if(!swapChain[0])
		{
			swapChain[0] = new Direct3DSwapChain8(this, &present);
			swapChain[0]->bind();
		}
		else
		{
			swapChain[0]->reset(&present);
		}

		HWND windowHandle = presentParameters->hDeviceWindow ? presentParameters->hDeviceWindow : focusWindow;

		int width = 0;
		int height = 0;

		if(presentParameters->Windowed && (presentParameters->BackBufferHeight == 0 || presentParameters->BackBufferWidth == 0))
		{
			RECT rectangle;
			GetClientRect(windowHandle, &rectangle);

			width = rectangle.right - rectangle.left;
			height = rectangle.bottom - rectangle.top;
		}
		else
		{
			width = presentParameters->BackBufferWidth;
			height = presentParameters->BackBufferHeight;
		}

		if(presentParameters->EnableAutoDepthStencil != FALSE)
		{
			depthStencil = new Direct3DSurface8(this, this, width, height, presentParameters->AutoDepthStencilFormat, D3DPOOL_DEFAULT, presentParameters->MultiSampleType, presentParameters->AutoDepthStencilFormat == D3DFMT_D16_LOCKABLE, D3DUSAGE_DEPTHSTENCIL);
			depthStencil->bind();
		}

		IDirect3DSurface8 *renderTarget;
		swapChain[0]->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &renderTarget);
		SetRenderTarget(renderTarget, depthStencil);
		renderTarget->Release();

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
		SetRenderState(D3DRS_EDGEANTIALIAS, FALSE);
		SetRenderState(D3DRS_RANGEFOGENABLE, FALSE);
		SetRenderState(D3DRS_ZBIAS, 0);
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
		SetRenderState(D3DRS_SOFTWAREVERTEXPROCESSING, FALSE);
		SetRenderState(D3DRS_POINTSIZE, FtoDW(1.0f));
		SetRenderState(D3DRS_POINTSIZE_MIN, FtoDW(0.0f));
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
		SetRenderState(D3DRS_POSITIONORDER, D3DORDER_CUBIC);
		SetRenderState(D3DRS_NORMALORDER, D3DORDER_LINEAR);

		for(int i = 0; i < 8; i++)
		{
			SetTexture(i, 0);

			SetTextureStageState(i, D3DTSS_COLOROP, i == 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE);
			SetTextureStageState(i, D3DTSS_COLORARG1, D3DTA_TEXTURE);   // TODO: D3DTA_DIFFUSE when no texture assigned
			SetTextureStageState(i, D3DTSS_COLORARG2, D3DTA_CURRENT);
			SetTextureStageState(i, D3DTSS_ALPHAOP, i == 0 ? D3DTOP_SELECTARG1 : D3DTOP_DISABLE);
			SetTextureStageState(i, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);   // TODO: D3DTA_DIFFUSE when no texture assigned
			SetTextureStageState(i, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
			SetTextureStageState(i, D3DTSS_BUMPENVMAT00, FtoDW(0.0f));
			SetTextureStageState(i, D3DTSS_BUMPENVMAT01, FtoDW(0.0f));
			SetTextureStageState(i, D3DTSS_BUMPENVMAT10, FtoDW(0.0f));
			SetTextureStageState(i, D3DTSS_BUMPENVMAT11, FtoDW(0.0f));
			SetTextureStageState(i, D3DTSS_TEXCOORDINDEX, i);
			SetTextureStageState(i, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
			SetTextureStageState(i, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
			SetTextureStageState(i, D3DTSS_ADDRESSW, D3DTADDRESS_WRAP);
			SetTextureStageState(i, D3DTSS_BORDERCOLOR, 0x00000000);
			SetTextureStageState(i, D3DTSS_MAGFILTER, D3DTEXF_POINT);
			SetTextureStageState(i, D3DTSS_MINFILTER, D3DTEXF_POINT);
			SetTextureStageState(i, D3DTSS_MIPFILTER, D3DTEXF_NONE);
			SetTextureStageState(i, D3DTSS_MIPMAPLODBIAS, 0);
			SetTextureStageState(i, D3DTSS_MAXMIPLEVEL, 0);
			SetTextureStageState(i, D3DTSS_MAXANISOTROPY, 1);
			SetTextureStageState(i, D3DTSS_BUMPENVLSCALE, FtoDW(0.0f));
			SetTextureStageState(i, D3DTSS_BUMPENVLOFFSET, FtoDW(0.0f));
			SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
			SetTextureStageState(i, D3DTSS_COLORARG0, D3DTA_CURRENT);
			SetTextureStageState(i, D3DTSS_ALPHAARG0, D3DTA_CURRENT);
			SetTextureStageState(i, D3DTSS_RESULTARG, D3DTA_CURRENT);
		}

		currentPalette = 0xFFFF;

		delete cursor;
		showCursor = false;

		return D3D_OK;
	}

	long Direct3DDevice8::ResourceManagerDiscardBytes(unsigned long bytes)
	{
		TRACE("");

		return D3D_OK;
	}

	long Direct3DDevice8::SetClipPlane(unsigned long index, const float *plane)
	{
		TRACE("");

		if(!plane || index > 6)
		{
			return INVALIDCALL();
		}

		if(!recordState)
		{
			this->plane[index][0] = plane[0];
			this->plane[index][1] = plane[1];
			this->plane[index][2] = plane[2];
			this->plane[index][3] = plane[3];

			renderer->setClipPlane(index, plane);
		}
		else
		{
			stateRecorder.back()->setClipPlane(index, plane);
		}

		return D3D_OK;
	}

	long Direct3DDevice8::SetClipStatus(const D3DCLIPSTATUS8 *clipStatus)
	{
		TRACE("");

		if(!clipStatus)
		{
			return INVALIDCALL();
		}

		this->clipStatus = *clipStatus;

		UNIMPLEMENTED();

		return D3D_OK;
	}

	long Direct3DDevice8::SetCurrentTexturePalette(unsigned int paletteNumber)
	{
		TRACE("");

		if(paletteNumber > 0xFFFF || palette.find(paletteNumber) == palette.end())
		{
			return INVALIDCALL();
		}

		if(!recordState)
		{
			currentPalette = paletteNumber;

			sw::Surface::setTexturePalette((unsigned int*)&palette[currentPalette]);
		}
		else
		{
			stateRecorder.back()->setCurrentTexturePalette(paletteNumber);
		}

		return D3D_OK;
	}

	void Direct3DDevice8::SetCursorPosition(int x, int y, unsigned long flags)
	{
		TRACE("");

		POINT point = {x, y};
		HWND window = focusWindow ? focusWindow : presentParameters.hDeviceWindow;
		ScreenToClient(window, &point);

		sw::FrameBuffer::setCursorPosition(point.x, point.y);
	}

	long Direct3DDevice8::SetCursorProperties(unsigned int x0, unsigned int y0, IDirect3DSurface8 *cursorBitmap)
	{
		TRACE("");

		if(!cursorBitmap)
		{
			return INVALIDCALL();
		}

		D3DSURFACE_DESC desc;
		D3DLOCKED_RECT lock;

		cursorBitmap->GetDesc(&desc);
		cursorBitmap->LockRect(&lock, 0, 0);

		delete cursor;
		cursor = sw::Surface::create(0, desc.Width, desc.Height, 1, 0, 1, sw::FORMAT_A8R8G8B8, false, false);

		void *buffer = cursor->lockExternal(0, 0, 0, sw::LOCK_DISCARD, sw::PUBLIC);
		memcpy(buffer, lock.pBits, desc.Width * desc.Height * sizeof(unsigned int));
		cursor->unlockExternal();

		cursorBitmap->UnlockRect();

		sw::FrameBuffer::setCursorOrigin(x0, y0);

		bindCursor();

		return D3D_OK;
	}

	void Direct3DDevice8::SetGammaRamp(unsigned long flags, const D3DGAMMARAMP *ramp)
	{
		TRACE("");

		if(!ramp)
		{
			return;
		}

		swapChain[0]->setGammaRamp((sw::GammaRamp*)ramp, flags & D3DSGR_CALIBRATE);

		return;
	}

	long Direct3DDevice8::SetLight(unsigned long index, const D3DLIGHT8 *light)
	{
		TRACE("");

		if(!light)
		{
			return INVALIDCALL();
		}

		if(!recordState)
		{
			this->light[index] = *light;

			lightsDirty = true;
		}
		else
		{
			stateRecorder.back()->setLight(index, light);
		}

		return D3D_OK;
	}

	long Direct3DDevice8::SetMaterial(const D3DMATERIAL8 *material)
	{
		TRACE("");

		if(!material)
		{
			return INVALIDCALL();   // FIXME: Correct behaviour?
		}

		if(!recordState)
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
			stateRecorder.back()->setMaterial(material);
		}

		return D3D_OK;
	}

	long Direct3DDevice8::SetPaletteEntries(unsigned int paletteNumber, const PALETTEENTRY *entries)
	{
		TRACE("");

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

	long Direct3DDevice8::SetPixelShader(unsigned long handle)
	{
		TRACE("");

		if(!recordState)
		{
			if(pixelShader[handle])
			{
				pixelShader[handle]->bind();
			}

			if(pixelShader[pixelShaderHandle])
			{
				pixelShader[pixelShaderHandle]->unbind();
			}

			pixelShaderHandle = handle;

			if(handle != 0)
			{
				renderer->setPixelShader(pixelShader[handle]->getPixelShader());
			}
			else
			{
				renderer->setPixelShader(0);
			}
		}
		else
		{
			stateRecorder.back()->setPixelShader(handle);
		}

		return D3D_OK;
	}

	long Direct3DDevice8::SetPixelShaderConstant(unsigned long startRegister, const void *constantData, unsigned long count)
	{
		TRACE("");

		if(!recordState)
		{
			for(unsigned int i = 0; i < count; i++)
			{
				pixelShaderConstant[startRegister + i][0] = ((float*)constantData)[i * 4 + 0];
				pixelShaderConstant[startRegister + i][1] = ((float*)constantData)[i * 4 + 1];
				pixelShaderConstant[startRegister + i][2] = ((float*)constantData)[i * 4 + 2];
				pixelShaderConstant[startRegister + i][3] = ((float*)constantData)[i * 4 + 3];
			}

			renderer->setPixelShaderConstantF(startRegister, (const float*)constantData, count);
		}
		else
		{
			stateRecorder.back()->setPixelShaderConstant(startRegister, constantData, count);
		}

		return D3D_OK;
	}

	long Direct3DDevice8::SetRenderState(D3DRENDERSTATETYPE state, unsigned long value)
	{
		TRACE("D3DRENDERSTATETYPE state = %d, unsigned long value = %d", state, value);

		if(state < D3DRS_ZENABLE || state > D3DRS_NORMALORDER)
		{
			return D3D_OK;   // FIXME: Warning
		}

		if(!recordState)
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
					// FIXME: Unimplemented (should set gouraud)?
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DRS_LINEPATTERN:
				if(!init) UNIMPLEMENTED();
				break;
			case D3DRS_ZWRITEENABLE:
				renderer->setDepthWriteEnable(value != FALSE);
				break;
			case D3DRS_ALPHATESTENABLE:
				renderer->setAlphaTestEnable(value != FALSE);
				break;
			case D3DRS_LASTPIXEL:
				if(!init) UNIMPLEMENTED();
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
			//	if(!init && value == 1) UNIMPLEMENTED();   // FIXME: Unimplemented
				break;
			case D3DRS_ALPHABLENDENABLE:
				renderer->setAlphaBlendEnable(value != FALSE);
				break;
			case D3DRS_FOGENABLE:
				renderer->setFogEnable(value != FALSE);
				break;
			case D3DRS_ZVISIBLE:
				break;   // Not supported
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
			case D3DRS_EDGEANTIALIAS:
				if(!init) if(value != FALSE) UNIMPLEMENTED();
				break;
			case D3DRS_ZBIAS:
				renderer->setDepthBias(-2.0e-6f * value);
				renderer->setSlopeDepthBias(0.0f);
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
			case D3DRS_SOFTWAREVERTEXPROCESSING:
				break;
			case D3DRS_POINTSIZE:
				renderer->setPointSize((float&)value);
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
				SetRenderTarget(renderTarget, depthStencil);   // Sets the multi-sample mask, if maskable
				break;
			case D3DRS_PATCHEDGESTYLE:
			//	if(!init) UNIMPLEMENTED();
				break;
			case D3DRS_PATCHSEGMENTS:
			//	UNIMPLEMENTED();   // FIXME
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
				renderer->setColorWriteMask(0, value);
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
			case D3DRS_POSITIONORDER:
				if(!init) UNIMPLEMENTED();
				break;
			case D3DRS_NORMALORDER:
				if(!init) UNIMPLEMENTED();
				break;
			default:
				ASSERT(false);
			}
		}
		else   // stateRecorder
		{
			stateRecorder.back()->setRenderState(state, value);
		}

		return D3D_OK;
	}

	long Direct3DDevice8::SetRenderTarget(IDirect3DSurface8 *newRenderTarget, IDirect3DSurface8 *newDepthStencil)
	{
		TRACE("");

		Direct3DSurface8 *renderTarget = static_cast<Direct3DSurface8*>(newRenderTarget);

		if(renderTarget)   // FIXME: Check for D3DUSAGE_RENDERTARGET
		{
			renderTarget->bind();
		}

		if(this->renderTarget)
		{
			this->renderTarget->unbind();
		}

		this->renderTarget = renderTarget;

		Direct3DSurface8 *depthStencil = static_cast<Direct3DSurface8*>(newDepthStencil);

		if(depthStencil)   // FIXME: Check for D3DUSAGE_DEPTHSTENCIL and D3DPOOL_DEFAULT
		{
			depthStencil->bind();
		}

		if(this->depthStencil)
		{
			this->depthStencil->unbind();
		}

		this->depthStencil = depthStencil;

		// Reset viewport to size of current render target
		D3DSURFACE_DESC renderTargetDesc;
		renderTarget->GetDesc(&renderTargetDesc);

		D3DVIEWPORT8 viewport;
		viewport.X = 0;
		viewport.Y = 0;
		viewport.Width = renderTargetDesc.Width;
		viewport.Height = renderTargetDesc.Height;
		viewport.MinZ = 0;
		viewport.MaxZ = 1;

		SetViewport(&viewport);

		// Set the multi-sample mask, if maskable
		if(renderTargetDesc.MultiSampleType != D3DMULTISAMPLE_NONE)
		{
			renderer->setMultiSampleMask(renderState[D3DRS_MULTISAMPLEMASK]);
		}
		else
		{
			renderer->setMultiSampleMask(0xFFFFFFFF);
		}

		renderer->setRenderTarget(0, renderTarget);
		renderer->setDepthBuffer(depthStencil);
		renderer->setStencilBuffer(depthStencil);

		return D3D_OK;
	}

	long Direct3DDevice8::SetStreamSource(unsigned int stream, IDirect3DVertexBuffer8 *iVertexBuffer, unsigned int stride)
	{
		TRACE("");

		Direct3DVertexBuffer8 *vertexBuffer = static_cast<Direct3DVertexBuffer8*>(iVertexBuffer);

		if(!recordState)
		{
			if(vertexBuffer)
			{
				vertexBuffer->bind();
			}

			if(dataStream[stream])
			{
				dataStream[stream]->unbind();
				streamStride[stream] = 0;
			}

			dataStream[stream] = vertexBuffer;
			streamStride[stream] = stride;
		}
		else
		{
			stateRecorder.back()->setStreamSource(stream, vertexBuffer, stride);
		}

		return D3D_OK;
	}

	long Direct3DDevice8::SetTexture(unsigned long stage, IDirect3DBaseTexture8 *iBaseTexture)
	{
		TRACE("");

		if(stage >= 8)
		{
			return INVALIDCALL();
		}

		Direct3DBaseTexture8 *baseTexture = dynamic_cast<Direct3DBaseTexture8*>(iBaseTexture);

		if(!recordState)
		{
			if(texture[stage] == baseTexture)
			{
				return D3D_OK;
			}

			if(baseTexture)
			{
				baseTexture->bind();
			}

			if(texture[stage])
			{
				texture[stage]->unbind();
			}

			texture[stage] = baseTexture;
		}
		else
		{
			stateRecorder.back()->setTexture(stage, baseTexture);
		}

		return D3D_OK;
	}

	long Direct3DDevice8::SetTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type, unsigned long value)
	{
		TRACE("unsigned long stage = %d, D3DTEXTURESTAGESTATETYPE type = %d, unsigned long value = %d", stage, type, value);

		if(stage >= 8 || type < 0 || type > D3DTSS_RESULTARG)
		{
			return INVALIDCALL();
		}

		if(!recordState)
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
				renderer->setTexCoordIndex(stage, value & 0xFFFF);

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
				default:
					ASSERT(false);
				}
				break;
			case D3DTSS_ADDRESSU:
				switch(value)
				{
				case D3DTADDRESS_WRAP:
					renderer->setAddressingModeU(sw::SAMPLER_PIXEL, stage, sw::ADDRESSING_WRAP);
					break;
				case D3DTADDRESS_MIRROR:
					renderer->setAddressingModeU(sw::SAMPLER_PIXEL, stage, sw::ADDRESSING_MIRROR);
					break;
				case D3DTADDRESS_CLAMP:
					renderer->setAddressingModeU(sw::SAMPLER_PIXEL, stage, sw::ADDRESSING_CLAMP);
					break;
				case D3DTADDRESS_BORDER:
					renderer->setAddressingModeU(sw::SAMPLER_PIXEL, stage, sw::ADDRESSING_BORDER);
					break;
				case D3DTADDRESS_MIRRORONCE:
					renderer->setAddressingModeU(sw::SAMPLER_PIXEL, stage, sw::ADDRESSING_MIRRORONCE);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DTSS_ADDRESSV:
				switch(value)
				{
				case D3DTADDRESS_WRAP:
					renderer->setAddressingModeV(sw::SAMPLER_PIXEL, stage, sw::ADDRESSING_WRAP);
					break;
				case D3DTADDRESS_MIRROR:
					renderer->setAddressingModeV(sw::SAMPLER_PIXEL, stage, sw::ADDRESSING_MIRROR);
					break;
				case D3DTADDRESS_CLAMP:
					renderer->setAddressingModeV(sw::SAMPLER_PIXEL, stage, sw::ADDRESSING_CLAMP);
					break;
				case D3DTADDRESS_BORDER:
					renderer->setAddressingModeV(sw::SAMPLER_PIXEL, stage, sw::ADDRESSING_BORDER);
					break;
				case D3DTADDRESS_MIRRORONCE:
					renderer->setAddressingModeV(sw::SAMPLER_PIXEL, stage, sw::ADDRESSING_MIRRORONCE);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DTSS_BORDERCOLOR:
				renderer->setBorderColor(sw::SAMPLER_PIXEL, stage, value);
				break;
			case D3DTSS_MAGFILTER:
				// NOTE: SwiftShader does not differentiate between minification and magnification filter
				switch(value)
				{
				case D3DTEXF_NONE:
					renderer->setTextureFilter(sw::SAMPLER_PIXEL, stage, sw::FILTER_POINT);
					break;
				case D3DTEXF_POINT:
					renderer->setTextureFilter(sw::SAMPLER_PIXEL, stage, sw::FILTER_POINT);
					break;
				case D3DTEXF_LINEAR:
					renderer->setTextureFilter(sw::SAMPLER_PIXEL, stage, sw::FILTER_LINEAR);
					break;
				case D3DTEXF_ANISOTROPIC:
					renderer->setTextureFilter(sw::SAMPLER_PIXEL, stage, sw::FILTER_ANISOTROPIC);
					break;
				case D3DTEXF_FLATCUBIC:
					renderer->setTextureFilter(sw::SAMPLER_PIXEL, stage, sw::FILTER_LINEAR);   // NOTE: Unimplemented, fail silently
					break;
				case D3DTEXF_GAUSSIANCUBIC:
					renderer->setTextureFilter(sw::SAMPLER_PIXEL, stage, sw::FILTER_LINEAR);   // NOTE: Unimplemented, fail silently
					break;
				default:
					return INVALIDCALL();
				};
				break;
			case D3DTSS_MINFILTER:
				// NOTE: SwiftShader does not differentiate between minification and magnification filter
				switch(value)
				{
				case D3DTEXF_NONE:
					renderer->setTextureFilter(sw::SAMPLER_PIXEL, stage, sw::FILTER_POINT);
					break;
				case D3DTEXF_POINT:
					renderer->setTextureFilter(sw::SAMPLER_PIXEL, stage, sw::FILTER_POINT);
					break;
				case D3DTEXF_LINEAR:
					renderer->setTextureFilter(sw::SAMPLER_PIXEL, stage, sw::FILTER_LINEAR);
					break;
				case D3DTEXF_ANISOTROPIC:
					renderer->setTextureFilter(sw::SAMPLER_PIXEL, stage, sw::FILTER_ANISOTROPIC);
					break;
				case D3DTEXF_FLATCUBIC:
					renderer->setTextureFilter(sw::SAMPLER_PIXEL, stage, sw::FILTER_LINEAR);   // NOTE: Unimplemented, fail silently
					break;
				case D3DTEXF_GAUSSIANCUBIC:
					renderer->setTextureFilter(sw::SAMPLER_PIXEL, stage, sw::FILTER_LINEAR);   // NOTE: Unimplemented, fail silently
					break;
				default:
					return INVALIDCALL();
				};
				break;
			case D3DTSS_MIPFILTER:
				switch(value)
				{
				case D3DTEXF_NONE:
					renderer->setMipmapFilter(sw::SAMPLER_PIXEL, stage, sw::MIPMAP_NONE);
					break;
				case D3DTEXF_POINT:
					renderer->setMipmapFilter(sw::SAMPLER_PIXEL, stage, sw::MIPMAP_POINT);
					break;
				case D3DTEXF_LINEAR:
					renderer->setMipmapFilter(sw::SAMPLER_PIXEL, stage, sw::MIPMAP_LINEAR);
					break;
				case D3DTEXF_ANISOTROPIC:
					renderer->setMipmapFilter(sw::SAMPLER_PIXEL, stage, sw::MIPMAP_LINEAR);   // NOTE: Unimplemented, fail silently
					break;
				case D3DTEXF_FLATCUBIC:
					renderer->setMipmapFilter(sw::SAMPLER_PIXEL, stage, sw::MIPMAP_LINEAR);   // NOTE: Unimplemented, fail silently
					break;
				case D3DTEXF_GAUSSIANCUBIC:
					renderer->setMipmapFilter(sw::SAMPLER_PIXEL, stage, sw::MIPMAP_LINEAR);   // NOTE: Unimplemented, fail silently
					break;
				default:
					return INVALIDCALL();
				};
				break;
			case D3DTSS_MIPMAPLODBIAS:
				{
					float LOD = (float&)value - sw::log2((float)context->renderTarget[0]->getSuperSampleCount());   // FIXME: Update when render target changes
					renderer->setMipmapLOD(sw::SAMPLER_PIXEL, stage, LOD);
				}
				break;
			case D3DTSS_MAXMIPLEVEL:
				break;
			case D3DTSS_MAXANISOTROPY:
				renderer->setMaxAnisotropy(sw::SAMPLER_PIXEL, stage, sw::clamp((unsigned int)value, (unsigned int)1, maxAnisotropy));
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
					renderer->setTextureTransform(stage, 0, (value &  D3DTTFF_PROJECTED) == D3DTTFF_PROJECTED);
					break;
				case D3DTTFF_COUNT1:
					renderer->setTextureTransform(stage, 1, (value &  D3DTTFF_PROJECTED) == D3DTTFF_PROJECTED);
					break;
				case D3DTTFF_COUNT2:
					renderer->setTextureTransform(stage, 2, (value &  D3DTTFF_PROJECTED) == D3DTTFF_PROJECTED);
					break;
				case D3DTTFF_COUNT3:
					renderer->setTextureTransform(stage, 3, (value &  D3DTTFF_PROJECTED) == D3DTTFF_PROJECTED);
					break;
				case D3DTTFF_COUNT4:
					renderer->setTextureTransform(stage, 4, (value &  D3DTTFF_PROJECTED) == D3DTTFF_PROJECTED);
					break;
				default:
					ASSERT(false);
				}
				break;
			case D3DTSS_ADDRESSW:
				switch(value)
				{
				case D3DTADDRESS_WRAP:
					renderer->setAddressingModeW(sw::SAMPLER_PIXEL, stage, sw::ADDRESSING_WRAP);
					break;
				case D3DTADDRESS_MIRROR:
					renderer->setAddressingModeW(sw::SAMPLER_PIXEL, stage, sw::ADDRESSING_MIRROR);
					break;
				case D3DTADDRESS_CLAMP:
					renderer->setAddressingModeW(sw::SAMPLER_PIXEL, stage, sw::ADDRESSING_CLAMP);
					break;
				case D3DTADDRESS_BORDER:
					renderer->setAddressingModeW(sw::SAMPLER_PIXEL, stage, sw::ADDRESSING_BORDER);
					break;
				case D3DTADDRESS_MIRRORONCE:
					renderer->setAddressingModeW(sw::SAMPLER_PIXEL, stage, sw::ADDRESSING_MIRRORONCE);
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
			default:
				ASSERT(false);
			}
		}
		else   // stateRecorder
		{
			stateRecorder.back()->setTextureStageState(stage, type, value);
		}

		return D3D_OK;
	}

	long Direct3DDevice8::SetTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix)
	{
		TRACE("");

		if(!matrix || state < 0 || state > 511)
		{
			return INVALIDCALL();
		}

		if(!recordState)
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
			stateRecorder.back()->setTransform(state, matrix);
		}

		return D3D_OK;
	}

	long Direct3DDevice8::SetVertexShader(unsigned long handle)
	{
		TRACE("");

		if(!recordState)
		{
			if(handle & 0x00000001)
			{
				unsigned int index = handle >> 16;

				if(vertexShader[index])
				{
					vertexShader[index]->bind();
				}

				if(vertexShader[vertexShaderHandle >> 16])
				{
					vertexShader[vertexShaderHandle >> 16]->unbind();
				}

				vertexShaderHandle = handle;

				Direct3DVertexShader8 *shader = vertexShader[index];
				renderer->setVertexShader(shader->getVertexShader());
				declaration = shader->getDeclaration();

				FVF = 0;
			}
			else
			{
				renderer->setVertexShader(0);
				declaration = 0;

				FVF = handle;
			}
		}
		else
		{
			stateRecorder.back()->setVertexShader(handle);
		}

		return D3D_OK;
	}

	long Direct3DDevice8::SetVertexShaderConstant(unsigned long startRegister, const void *constantData, unsigned long count)
	{
		TRACE("");

		if(!constantData)
		{
			return INVALIDCALL();
		}

		if(!recordState)
		{
			for(unsigned int i = 0; i < count; i++)
			{
				vertexShaderConstant[startRegister + i][0] = ((float*)constantData)[i * 4 + 0];
				vertexShaderConstant[startRegister + i][1] = ((float*)constantData)[i * 4 + 1];
				vertexShaderConstant[startRegister + i][2] = ((float*)constantData)[i * 4 + 2];
				vertexShaderConstant[startRegister + i][3] = ((float*)constantData)[i * 4 + 3];
			}

			renderer->setVertexShaderConstantF(startRegister, (const float*)constantData, count);
		}
		else
		{
			stateRecorder.back()->setVertexShaderConstant(startRegister, constantData, count);
		}

		return D3D_OK;
	}

	long Direct3DDevice8::SetViewport(const D3DVIEWPORT8 *viewport)
	{
		TRACE("");

		if(!viewport)
		{
			return INVALIDCALL();
		}

		if(!recordState)
		{
			this->viewport = *viewport;
		}
		else
		{
			stateRecorder.back()->setViewport(viewport);
		}

		return D3D_OK;
	}

	int Direct3DDevice8::ShowCursor(int show)
	{
		TRACE("");

		int oldValue = showCursor ? TRUE : FALSE;

		showCursor = show != FALSE && cursor;

		bindCursor();

		return oldValue;
	}

	long Direct3DDevice8::TestCooperativeLevel()
	{
		TRACE("");

		return D3D_OK;
	}

	long Direct3DDevice8::UpdateTexture(IDirect3DBaseTexture8 *sourceTexture, IDirect3DBaseTexture8 *destinationTexture)
	{
		TRACE("");

		if(!sourceTexture || !destinationTexture)
		{
			return INVALIDCALL();
		}

		D3DRESOURCETYPE type = sourceTexture->GetType();

		if(type != destinationTexture->GetType())
		{
			return INVALIDCALL();
		}

		switch(type)
		{
		case D3DRTYPE_TEXTURE:
			{
				IDirect3DTexture8 *source;
				IDirect3DTexture8 *dest;

				sourceTexture->QueryInterface(IID_IDirect3DTexture8, (void**)&source);
				destinationTexture->QueryInterface(IID_IDirect3DTexture8, (void**)&dest);

				ASSERT(source && dest);

				for(unsigned int level = 0; level < source->GetLevelCount() && level < dest->GetLevelCount(); level++)
				{
					IDirect3DSurface8 *sourceSurface;
					IDirect3DSurface8 *destinationSurface;

					source->GetSurfaceLevel(level, &sourceSurface);
					dest->GetSurfaceLevel(level, &destinationSurface);

					updateSurface(sourceSurface, 0, destinationSurface, 0);

					sourceSurface->Release();
					destinationSurface->Release();
				}

				source->Release();
				dest->Release();
			}
			break;
		case D3DRTYPE_VOLUMETEXTURE:
			{
				IDirect3DVolumeTexture8 *source;
				IDirect3DVolumeTexture8 *dest;

				sourceTexture->QueryInterface(IID_IDirect3DVolumeTexture8, (void**)&source);
				destinationTexture->QueryInterface(IID_IDirect3DVolumeTexture8, (void**)&dest);

				ASSERT(source && dest);

				for(unsigned int level = 0; level < source->GetLevelCount() && level < dest->GetLevelCount(); level++)   // FIXME: Fail when source texture has fewer levels than the destination
				{
					IDirect3DVolume8 *sourceVolume;
					IDirect3DVolume8 *destinationVolume;

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
				IDirect3DCubeTexture8 *source;
				IDirect3DCubeTexture8 *dest;

				sourceTexture->QueryInterface(IID_IDirect3DCubeTexture8, (void**)&source);
				destinationTexture->QueryInterface(IID_IDirect3DCubeTexture8, (void**)&dest);

				ASSERT(source && dest);

				for(int face = 0; face < 6; face++)
				{
					for(unsigned int level = 0; level < source->GetLevelCount() && level < dest->GetLevelCount(); level++)
					{
						IDirect3DSurface8 *sourceSurface;
						IDirect3DSurface8 *destinationSurface;

						source->GetCubeMapSurface((D3DCUBEMAP_FACES)face, level, &sourceSurface);
						dest->GetCubeMapSurface((D3DCUBEMAP_FACES)face, level, &destinationSurface);

						updateSurface(sourceSurface, 0, destinationSurface, 0);

						sourceSurface->Release();
						destinationSurface->Release();
					}
				}

				source->Release();
				dest->Release();
			}
			break;
		default:
			ASSERT(false);
		}

		return D3D_OK;
	}

	long Direct3DDevice8::ValidateDevice(unsigned long *numPasses)
	{
		TRACE("");

		if(!numPasses)
		{
			return INVALIDCALL();
		}

		*numPasses = 1;

		return D3D_OK;
	}

	long Direct3DDevice8::updateSurface(IDirect3DSurface8 *sourceSurface, const RECT *sourceRect, IDirect3DSurface8 *destinationSurface, const POINT *destPoint)
	{
		TRACE("IDirect3DSurface8 *sourceSurface = 0x%0.8p, const RECT *sourceRect = 0x%0.8p, IDirect3DSurface8 *destinationSurface = 0x%0.8p, const POINT *destPoint = 0x%0.8p", sourceSurface, sourceRect, destinationSurface, destPoint);

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

		if(sourceRect && destPoint)
		{
			sRect.left = sourceRect->left;
			sRect.top = sourceRect->top;
			sRect.right = sourceRect->right;
			sRect.bottom = sourceRect->bottom;

			dRect.left = destPoint->x;
			dRect.top = destPoint->y;
			dRect.right = destPoint->x + sourceRect->right - sourceRect->left;
			dRect.bottom = destPoint->y + sourceRect->bottom - sourceRect->top;
		}
		else
		{
			sRect.left = 0;
			sRect.top = 0;
			sRect.right = sourceDescription.Width;
			sRect.bottom = sourceDescription.Height;

			dRect.left = 0;
			dRect.top = 0;
			dRect.right = destinationDescription.Width;
			dRect.bottom = destinationDescription.Height;
		}

		int sWidth = sRect.right - sRect.left;
		int sHeight = sRect.bottom - sRect.top;

		int dWidth = dRect.right - dRect.left;
		int dHeight = dRect.bottom - dRect.top;

		if(sourceDescription.MultiSampleType      != D3DMULTISAMPLE_NONE ||
		   destinationDescription.MultiSampleType != D3DMULTISAMPLE_NONE ||
		// sourceDescription.Pool      != D3DPOOL_SYSTEMMEM ||   // FIXME: Check back buffer and depth buffer memory pool flags
		// destinationDescription.Pool != D3DPOOL_DEFAULT ||
		   sourceDescription.Format != destinationDescription.Format ||
		   sWidth  != dWidth ||
		   sHeight != dHeight)
		{
			return INVALIDCALL();
		}

		D3DLOCKED_RECT sourceLock;
		D3DLOCKED_RECT destinationLock;

		sourceSurface->LockRect(&sourceLock, &sRect, D3DLOCK_READONLY);
		destinationSurface->LockRect(&destinationLock, &dRect, 0);

		unsigned int width;
		unsigned int height;
		unsigned int bytes;

		switch(sourceDescription.Format)
		{
		case D3DFMT_DXT1:
			width = (dWidth + 3) / 4;
			height = (dHeight + 3) / 4;
			bytes = width * 8;   // 64 bit per 4x4 block
			break;
		case D3DFMT_DXT2:
		case D3DFMT_DXT3:
		case D3DFMT_DXT4:
		case D3DFMT_DXT5:
			width = (dWidth + 3) / 4;
			height = (dHeight + 3) / 4;
			bytes = width * 16;   // 128 bit per 4x4 block
			break;
		default:
			width = dWidth;
			height = dHeight;
			bytes = width * Direct3DSurface8::bytes(sourceDescription.Format);
		}

		for(unsigned int y = 0; y < height; y++)
		{
			memcpy(destinationLock.pBits, sourceLock.pBits, bytes);

			(byte*&)sourceLock.pBits += sourceLock.Pitch;
			(byte*&)destinationLock.pBits += destinationLock.Pitch;
		}

		sourceSurface->UnlockRect();
		destinationSurface->UnlockRect();

		return D3D_OK;
	}

	long Direct3DDevice8::SetIndices(IDirect3DIndexBuffer8 *iIndexBuffer, unsigned int baseVertexIndex)
	{
		TRACE("");

		Direct3DIndexBuffer8 *indexBuffer = static_cast<Direct3DIndexBuffer8*>(iIndexBuffer);

		if(!recordState)
		{
			if(indexBuffer)
			{
				indexBuffer->bind();
			}

			if(this->indexData)
			{
				this->indexData->unbind();
			}

			this->indexData = indexBuffer;
			this->baseVertexIndex = baseVertexIndex;
		}
		else
		{
			stateRecorder.back()->setIndices(indexBuffer, baseVertexIndex);
		}

		return D3D_OK;
	}

	int Direct3DDevice8::FVFStride(unsigned long FVF)
	{
		int stride = 0;

		switch(FVF & D3DFVF_POSITION_MASK)
		{
		case D3DFVF_XYZ:	stride += 12;	break;
		case D3DFVF_XYZRHW:	stride += 16;	break;
		case D3DFVF_XYZB1:	stride += 16;	break;
		case D3DFVF_XYZB2:	stride += 20;	break;
		case D3DFVF_XYZB3:	stride += 24;	break;
		case D3DFVF_XYZB4:	stride += 28;	break;
		case D3DFVF_XYZB5:	stride += 32;	break;
		}

		if(FVF & D3DFVF_NORMAL)		stride += 12;
		if(FVF & D3DFVF_PSIZE)		stride += 4;
		if(FVF & D3DFVF_DIFFUSE)	stride += 4;
		if(FVF & D3DFVF_SPECULAR)	stride += 4;

		switch((FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT)
		{
		case 8: stride += 4 + 4 * ((1 + (FVF >> 30)) % 4);
		case 7: stride += 4 + 4 * ((1 + (FVF >> 28)) % 4);
		case 6: stride += 4 + 4 * ((1 + (FVF >> 26)) % 4);
		case 5: stride += 4 + 4 * ((1 + (FVF >> 24)) % 4);
		case 4: stride += 4 + 4 * ((1 + (FVF >> 22)) % 4);
		case 3: stride += 4 + 4 * ((1 + (FVF >> 20)) % 4);
		case 2: stride += 4 + 4 * ((1 + (FVF >> 18)) % 4);
		case 1: stride += 4 + 4 * ((1 + (FVF >> 16)) % 4);
		case 0: break;
		default:
			ASSERT(false);
		}

		return stride;
	}

	int Direct3DDevice8::typeStride(unsigned char type)
	{
		static const int LUT[] =
		{
			4,	// D3DDECLTYPE_FLOAT1    =  0,  // 1D float expanded to (value, 0., 0., 1.)
			8,	// D3DDECLTYPE_FLOAT2    =  1,  // 2D float expanded to (value, value, 0., 1.)
			12,	// D3DDECLTYPE_FLOAT3    =  2,  // 3D float expanded to (value, value, value, 1.)
			16,	// D3DDECLTYPE_FLOAT4    =  3,  // 4D float
			4,	// D3DDECLTYPE_D3DCOLOR  =  4,  // 4D packed unsigned bytes mapped to 0. to 1. range. Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
			4,	// D3DDECLTYPE_UBYTE4    =  5,  // 4D unsigned byte
			4,	// D3DDECLTYPE_SHORT2    =  6,  // 2D signed short expanded to (value, value, 0., 1.)
			8 	// D3DDECLTYPE_SHORT4    =  7,  // 4D signed short
		};

		if(type <= 7)
		{
			return LUT[type];
		}
		else ASSERT(false);

		return 0;
	}

	bool Direct3DDevice8::bindData(Direct3DIndexBuffer8 *indexBuffer, int base)
	{
		if(!bindViewport())
		{
			return false;   // Zero-area target region
		}

		bindTextures();
		bindStreams(base);
		bindIndexBuffer(indexBuffer);
		bindLights();

		return true;
	}

	void Direct3DDevice8::bindStreams(int base)
	{
		renderer->resetInputStreams((FVF & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW);

		int stride;

		if(!declaration)   // Fixed-function vertex pipeline
		{
			const void *buffer = 0;

			ASSERT(dataStream[0]);

			Direct3DVertexBuffer8 *stream = dataStream[0];
			sw::Resource *resource = stream->getResource();
			buffer = (char*)resource->data();
			stride = FVFStride(FVF);

			ASSERT(stride == streamStride[0]);   // FIXME
			ASSERT(buffer && stride);

			(char*&)buffer += stride * base;

			sw::Stream attribute(resource, buffer, stride);

			switch(FVF & D3DFVF_POSITION_MASK)
			{
			case D3DFVF_XYZ:
				renderer->setInputStream(sw::Position, attribute.define(buffer, sw::STREAMTYPE_FLOAT, 3));
				(char*&)buffer += 12;
				break;
			case D3DFVF_XYZRHW:
				renderer->setInputStream(sw::PositionT, attribute.define(buffer, sw::STREAMTYPE_FLOAT, 4));
				(char*&)buffer += 16;
				break;
			case D3DFVF_XYZB1:
				renderer->setInputStream(sw::Position, attribute.define(buffer, sw::STREAMTYPE_FLOAT, 3));
				(char*&)buffer += 12;

				renderer->setInputStream(sw::BlendWeight, attribute.define(buffer, sw::STREAMTYPE_FLOAT, 1));   // FIXME: Stream type depends on indexed blending active?
				(char*&)buffer += 4;
				break;
			case D3DFVF_XYZB2:
				renderer->setInputStream(sw::Position, attribute.define(buffer, sw::STREAMTYPE_FLOAT, 3));
				(char*&)buffer += 12;

				renderer->setInputStream(sw::BlendWeight, attribute.define(buffer, sw::STREAMTYPE_FLOAT, 1));   // FIXME: Stream type depends on indexed blending active?
				(char*&)buffer += 8;
				break;
			case D3DFVF_XYZB3:
				renderer->setInputStream(sw::Position, attribute.define(buffer, sw::STREAMTYPE_FLOAT, 3));
				(char*&)buffer += 12;

				renderer->setInputStream(sw::BlendWeight, attribute.define(buffer, sw::STREAMTYPE_FLOAT, 2));   // FIXME: Stream type depends on indexed blending active?
				(char*&)buffer += 12;
				break;
			case D3DFVF_XYZB4:
				renderer->setInputStream(sw::Position, attribute.define(buffer, sw::STREAMTYPE_FLOAT, 3));
				(char*&)buffer += 12;

				renderer->setInputStream(sw::BlendWeight, attribute.define(buffer, sw::STREAMTYPE_FLOAT, 3));   // FIXME: Stream type depends on indexed blending active?
				(char*&)buffer += 16;
				break;
			case D3DFVF_XYZB5:
				renderer->setInputStream(sw::Position, attribute.define(buffer, sw::STREAMTYPE_FLOAT, 3));
				(char*&)buffer += 12;

				renderer->setInputStream(sw::BlendWeight, attribute.define(buffer, sw::STREAMTYPE_FLOAT, 4));   // FIXME: Stream type depends on indexed blending active?
				(char*&)buffer += 20;
				break;
			}

			if(FVF & D3DFVF_LASTBETA_UBYTE4)
			{
				renderer->setInputStream(sw::BlendIndices, attribute.define((char*&)buffer - 4, sw::STREAMTYPE_INDICES, 1));
			}

			if(FVF & D3DFVF_NORMAL)
			{
				renderer->setInputStream(sw::Normal, attribute.define(buffer, sw::STREAMTYPE_FLOAT, 3));
				(char*&)buffer += 12;
			}

			if(FVF & D3DFVF_PSIZE)
			{
				renderer->setInputStream(sw::PointSize, attribute.define(buffer, sw::STREAMTYPE_FLOAT, 1));
				(char*&)buffer += 4;
			}

			if(FVF & D3DFVF_DIFFUSE)
			{
				renderer->setInputStream(sw::Color0, attribute.define(buffer, sw::STREAMTYPE_COLOR, 4));
				(char*&)buffer += 4;
			}

			if(FVF & D3DFVF_SPECULAR)
			{
				renderer->setInputStream(sw::Color1, attribute.define(buffer, sw::STREAMTYPE_COLOR, 4));
				(char*&)buffer += 4;
			}

			for(unsigned int i = 0; i < 8; i++)
			{
				if((FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT >= i + 1)
				{
					renderer->setInputStream(sw::TexCoord0 + i, attribute.define(buffer, sw::STREAMTYPE_FLOAT, 1 + (1 + (FVF >> (16 + i * 2))) % 4));
					(char*&)buffer += 4 + 4 * ((1 + (FVF >> (16 + i * 2))) % 4);
				}
			}
		}
		else
		{
			const unsigned long *element = declaration;
			int stream = 0;
			sw::Resource *resource;
			const void *buffer = 0;

			while(*element != 0xFFFFFFFF)
			{
				switch((*element & 0xE0000000) >> 29)
				{
				case 0:   // NOP
					if(*element != 0x00000000)
					{
						ASSERT(false);
					}
					break;
				case 1:   // Stream selector
					stream = *element & 0x0000000F;
					{
						ASSERT(dataStream[stream]);   // Expected a stream

						Direct3DVertexBuffer8 *streamBuffer = (Direct3DVertexBuffer8*)dataStream[stream];
						resource = streamBuffer->getResource();
						buffer = (char*)resource->data();

						const unsigned long *streamElement = element + 1;
						stride = 0;

						while((*streamElement & 0xE0000000) >> 29 == 2)   // Data definition
						{
							if(*streamElement & 0x10000000)   // Data skip
							{
								int skip = (*streamElement & 0x000F0000) >> 16;

								stride += 4 * skip;
							}
							else
							{
								stride += typeStride((unsigned char)((*streamElement & 0x000F0000) >> 16));
							}

							streamElement++;
						}

					//	ASSERT(stride == streamStride[stream]);   // FIXME: Probably just ignore

						(char*&)buffer += stride * base;
					}
					break;
				case 2:   // Data definition
					if(*element & 0x10000000)   // Data skip
					{
						int skip = (*element & 0x000F0000) >> 16;

						(char*&)buffer += 4 * skip;
					}
					else
					{
						int type = (*element & 0x000F0000) >> 16;
						int index = (*element & 0x0000000F) >> 0;

						sw::Stream attribute(resource, buffer, stride);

						switch(type)
						{
						case D3DVSDT_FLOAT1:   attribute.define(sw::STREAMTYPE_FLOAT, 1); break;
						case D3DVSDT_FLOAT2:   attribute.define(sw::STREAMTYPE_FLOAT, 2); break;
						case D3DVSDT_FLOAT3:   attribute.define(sw::STREAMTYPE_FLOAT, 3); break;
						case D3DVSDT_FLOAT4:   attribute.define(sw::STREAMTYPE_FLOAT, 4); break;
						case D3DVSDT_D3DCOLOR: attribute.define(sw::STREAMTYPE_COLOR, 4); break;
						case D3DVSDT_UBYTE4:   attribute.define(sw::STREAMTYPE_BYTE, 4);  break;
						case D3DVSDT_SHORT2:   attribute.define(sw::STREAMTYPE_SHORT, 2); break;
						case D3DVSDT_SHORT4:   attribute.define(sw::STREAMTYPE_SHORT, 4); break;
						default:               attribute.define(sw::STREAMTYPE_FLOAT, 0); ASSERT(false);
						}

						switch(index)
						{
						case D3DVSDE_POSITION:     renderer->setInputStream(sw::Position, attribute);     break;
						case D3DVSDE_BLENDWEIGHT:  renderer->setInputStream(sw::BlendWeight, attribute);  break;
						case D3DVSDE_BLENDINDICES: renderer->setInputStream(sw::BlendIndices, attribute); break;
						case D3DVSDE_NORMAL:       renderer->setInputStream(sw::Normal, attribute);       break;
						case D3DVSDE_PSIZE:        renderer->setInputStream(sw::PointSize, attribute);    break;
						case D3DVSDE_DIFFUSE:      renderer->setInputStream(sw::Color0, attribute);       break;
						case D3DVSDE_SPECULAR:     renderer->setInputStream(sw::Color1, attribute);       break;
						case D3DVSDE_TEXCOORD0:    renderer->setInputStream(sw::TexCoord0, attribute);    break;
						case D3DVSDE_TEXCOORD1:    renderer->setInputStream(sw::TexCoord1, attribute);    break;
						case D3DVSDE_TEXCOORD2:    renderer->setInputStream(sw::TexCoord2, attribute);    break;
						case D3DVSDE_TEXCOORD3:    renderer->setInputStream(sw::TexCoord3, attribute);    break;
						case D3DVSDE_TEXCOORD4:    renderer->setInputStream(sw::TexCoord4, attribute);    break;
						case D3DVSDE_TEXCOORD5:    renderer->setInputStream(sw::TexCoord5, attribute);    break;
						case D3DVSDE_TEXCOORD6:    renderer->setInputStream(sw::TexCoord6, attribute);    break;
						case D3DVSDE_TEXCOORD7:    renderer->setInputStream(sw::TexCoord7, attribute);    break;
					//	case D3DVSDE_POSITION2:    renderer->setInputStream(sw::Position1, attribute);    break;
					//	case D3DVSDE_NORMAL2:      renderer->setInputStream(sw::Normal1, attribute);      break;
						default:
							ASSERT(false);
						}

						(char*&)buffer += typeStride(type);
					}
					break;
				case 3:   // Tesselator data
					UNIMPLEMENTED();
					break;
				case 4:   // Constant data
					{
						int count = (*element & 0x1E000000) >> 25;
						int index = (*element & 0x0000007F) >> 0;

						SetVertexShaderConstant(index, element + 1, count);

						element += 4 * count;
					}
					break;
				case 5:   // Extension
					UNIMPLEMENTED();
					break;
				default:
					ASSERT(false);
				}

				element++;
			}
		}
	}

	void Direct3DDevice8::bindIndexBuffer(Direct3DIndexBuffer8 *indexBuffer)
	{
		sw::Resource *resource = 0;

		if(indexBuffer)
		{
			resource = indexBuffer->getResource();
		}

		renderer->setIndexBuffer(resource);
	}

	void Direct3DDevice8::bindLights()
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
			//	goto next;   // FIXME

				// FIXME: Unsupported, make it a positional light far away without falloff
				renderer->setLightPosition(active, -1000 * direction);
				renderer->setLightRange(active, l.Range);
				renderer->setLightAttenuation(active, 1, 0, 0);
			}
			else if(l.Type == D3DLIGHT_SPOT)
			{
			//	goto next;   // FIXME

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

	//	next:   // FIXME
			i++;
		}

		// Remaining lights are disabled
		while(active < 8)
		{
			renderer->setLightEnable(active, false);

			active++;
		}

		lightsDirty= false;
	}

	bool Direct3DDevice8::bindViewport()
	{
		if(viewport.Width == 0 || viewport.Height == 0)
		{
			return false;
		}

		sw::Viewport view;
		view.x0 = (float)viewport.X;
		view.y0 = (float)viewport.Y + viewport.Height;
		view.width = (float)viewport.Width;
		view.height = -(float)viewport.Height;
		view.minZ = viewport.MinZ;
		view.maxZ = viewport.MaxZ;

		renderer->setViewport(view);

		sw::Rect scissor;
		scissor.x0 = viewport.X;
		scissor.x1 = viewport.X + viewport.Width;
		scissor.y0 = viewport.Y;
		scissor.y1 = viewport.Y + viewport.Height;

		renderer->setScissor(scissor);

		return true;
	}

	void Direct3DDevice8::bindTextures()
	{
		for(int stage = 0; stage < 8; stage++)
		{
			Direct3DBaseTexture8 *baseTexture = texture[stage];
			sw::Resource *resource = 0;

			bool textureUsed = false;

			if(pixelShader[pixelShaderHandle])
			{
				textureUsed = pixelShader[pixelShaderHandle]->getPixelShader()->usesSampler(stage);
			}
			else
			{
				textureUsed = true;   // FIXME: Check fixed-function use?
			}

			if(baseTexture && textureUsed)
			{
				resource = baseTexture->getResource();
			}

			renderer->setTextureResource(stage, resource);

			if(baseTexture && textureUsed)
			{
				int levelCount = baseTexture->getInternalLevelCount();

				int textureLOD = baseTexture->GetLOD();
				int stageLOD = textureStageState[stage][D3DTSS_MAXMIPLEVEL];
				int LOD = textureLOD > stageLOD ? textureLOD : stageLOD;

				if(textureStageState[stage][D3DTSS_MIPFILTER] == D3DTEXF_NONE)
				{
					LOD = 0;
				}

				switch(baseTexture->GetType())
				{
				case D3DRTYPE_TEXTURE:
					{
						Direct3DTexture8 *texture = dynamic_cast<Direct3DTexture8*>(baseTexture);
						Direct3DSurface8 *surface;

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
							renderer->setTextureLevel(stage, 0, mipmapLevel, surface, sw::TEXTURE_2D);
						}
					}
					break;
				case D3DRTYPE_CUBETEXTURE:
					for(int face = 0; face < 6; face++)
					{
						Direct3DCubeTexture8 *cubeTexture = dynamic_cast<Direct3DCubeTexture8*>(baseTexture);
						Direct3DSurface8 *surface;

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
							renderer->setTextureLevel(stage, face, mipmapLevel, surface, sw::TEXTURE_CUBE);
						}
					}
					break;
				case D3DRTYPE_VOLUMETEXTURE:
					{
						Direct3DVolumeTexture8 *volumeTexture = dynamic_cast<Direct3DVolumeTexture8*>(baseTexture);
						Direct3DVolume8 *volume;

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
							renderer->setTextureLevel(stage, 0, mipmapLevel, volume, sw::TEXTURE_3D);
						}
					}
					break;
				default:
					UNIMPLEMENTED();
				}
			}
			else
			{
				renderer->setTextureLevel(stage, 0, 0, 0, sw::TEXTURE_NULL);
			}
		}
	}

	void Direct3DDevice8::bindCursor()
	{
		if(showCursor)
		{
			sw::FrameBuffer::setCursorImage(cursor);

			HCURSOR oldCursor = SetCursor(nullCursor);

			if(oldCursor != nullCursor)
			{
				win32Cursor = oldCursor;
			}
		}
		else
		{
			sw::FrameBuffer::setCursorImage(0);

			if(GetCursor() == nullCursor)
			{
				SetCursor(win32Cursor);
			}
		}
	}

	long Direct3DDevice8::updateVolume(IDirect3DVolume8 *sourceVolume, IDirect3DVolume8 *destinationVolume)
	{
		TRACE("IDirect3DVolume8 *sourceVolume = 0x%0.8p, IDirect3DVolume8 *destinationVolume = 0x%0.8p", sourceVolume, destinationVolume);

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
		   sourceDescription.Height != destinationDescription.Height)
		{
			return INVALIDCALL();
		}

		D3DLOCKED_BOX sourceLock;
		D3DLOCKED_BOX destinationLock;

		sourceVolume->LockBox(&sourceLock, 0, 0);
		destinationVolume->LockBox(&destinationLock, 0, 0);

		if(sourceLock.RowPitch != destinationLock.RowPitch ||
		   sourceLock.SlicePitch != destinationLock.SlicePitch)
		{
			UNIMPLEMENTED();
		}

		memcpy(destinationLock.pBits, sourceLock.pBits, sourceLock.SlicePitch * sourceDescription.Depth);

		sourceVolume->UnlockBox();
		destinationVolume->UnlockBox();

		return D3D_OK;
	}

	void Direct3DDevice8::configureFPU()
	{
		unsigned short cw;

		__asm
		{
			fstcw cw
			and cw, 0xFCFC   // Single-precision
			or cw, 0x003F    // Mask all exceptions
			and cw, 0xF3FF   // Round to nearest
			fldcw cw
		}
	}
}
