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

#include "Direct3DDevice9Ex.hpp"

#include "Direct3D9Ex.hpp"
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
#include "Renderer.hpp"
#include "FrameBuffer.hpp"
#include "Clipper.hpp"
#include "Configurator.hpp"
#include "Timer.hpp"

#include <assert.h>

namespace D3D9
{
	inline unsigned long FtoDW(float f)
	{
		return (unsigned long&)f;
	}

	inline float DWtoF(unsigned long dw)
	{
		return (float&)dw;
	}

	Direct3DDevice9Ex::Direct3DDevice9Ex(const HINSTANCE instance, Direct3D9Ex *d3d9ex, unsigned int adapter, D3DDEVTYPE deviceType, HWND focusWindow, unsigned long behaviourFlags, D3DPRESENT_PARAMETERS *presentParameters) : Direct3DDevice9(instance, d3d9ex, adapter, deviceType, focusWindow, behaviourFlags, presentParameters), d3d9ex(d3d9ex)
	{
	}

	Direct3DDevice9Ex::~Direct3DDevice9Ex()
	{
	}

	long Direct3DDevice9Ex::QueryInterface(const IID &iid, void **object)
	{
		CriticalSection cs(this);

		TRACE("const IID &iid = 0x%0.8p, void **object = 0x%0.8p", iid, object);

		if(iid == IID_IDirect3DDevice9Ex ||
		   iid == IID_IDirect3DDevice9 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DDevice9Ex::AddRef()
	{
		TRACE("void");

		return Direct3DDevice9::AddRef();
	}

	unsigned long Direct3DDevice9Ex::Release()
	{
		TRACE("void");

		return Direct3DDevice9::Release();
	}

	long Direct3DDevice9Ex::BeginScene()
	{
		TRACE("void");

		return Direct3DDevice9::BeginScene();
	}

	long Direct3DDevice9Ex::BeginStateBlock()
	{
		TRACE("void");

		return Direct3DDevice9::BeginStateBlock();
	}

	long Direct3DDevice9Ex::Clear(unsigned long count, const D3DRECT *rects, unsigned long flags, unsigned long color, float z, unsigned long stencil)
	{
		TRACE("unsigned long count = %d, const D3DRECT *rects = 0x%0.8p, unsigned long flags = 0x%0.8X, unsigned long color = 0x%0.8X, float z = %f, unsigned long stencil = %d", count, rects, flags, color, z, stencil);

		return Direct3DDevice9::Clear(count, rects, flags, color, z, stencil);
	}

	long Direct3DDevice9Ex::ColorFill(IDirect3DSurface9 *surface, const RECT *rect, D3DCOLOR color)
	{
		TRACE("IDirect3DSurface9 *surface = 0x%0.8p, const RECT *rect = 0x%0.8p, D3DCOLOR color = 0x%0.8X", surface, rect, color);

		return Direct3DDevice9::ColorFill(surface, rect, color);
	}

	long Direct3DDevice9Ex::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS *presentParameters, IDirect3DSwapChain9 **swapChain)
	{
		TRACE("D3DPRESENT_PARAMETERS *presentParameters = 0x%0.8p, IDirect3DSwapChain9 **swapChain = 0x%0.8p", presentParameters, swapChain);

		return Direct3DDevice9::CreateAdditionalSwapChain(presentParameters, swapChain);
	}

	long Direct3DDevice9Ex::CreateCubeTexture(unsigned int edgeLength, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DCubeTexture9 **cubeTexture, void **sharedHandle)
	{
		TRACE("unsigned int edgeLength = %d, unsigned int levels = %d, unsigned long usage = %d, D3DFORMAT format = %d, D3DPOOL pool = %d, IDirect3DCubeTexture9 **cubeTexture = 0x%0.8p, void **sharedHandle = 0x%0.8p", edgeLength, levels, usage, format, pool, cubeTexture, sharedHandle);

		return Direct3DDevice9::CreateCubeTexture(edgeLength, levels, usage, format, pool, cubeTexture, sharedHandle);
	}

	long Direct3DDevice9Ex::CreateDepthStencilSurface(unsigned int width, unsigned int height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multiSample, unsigned long multiSampleQuality, int discard, IDirect3DSurface9 **surface, void **sharedHandle)
	{
		TRACE("unsigned int width = %d, unsigned int height = %d, D3DFORMAT format = %d, D3DMULTISAMPLE_TYPE multiSample = %d, unsigned long multiSampleQuality = %d, int discard = %d, IDirect3DSurface9 **surface = 0x%0.8p, void **sharedHandle = 0x%0.8p", width, height, format, multiSample, multiSampleQuality, discard, surface, sharedHandle);

		return Direct3DDevice9::CreateDepthStencilSurface(width, height, format, multiSample, multiSampleQuality, discard, surface, sharedHandle);
	}

	long Direct3DDevice9Ex::CreateIndexBuffer(unsigned int length, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DIndexBuffer9 **indexBuffer, void **sharedHandle)
	{
		TRACE("unsigned int length = %d, unsigned long usage = %d, D3DFORMAT format = %d, D3DPOOL pool = %d, IDirect3DIndexBuffer9 **indexBuffer = 0x%0.8p, void **sharedHandle = 0x%0.8p", length, usage, format, pool, indexBuffer, sharedHandle);

		return Direct3DDevice9::CreateIndexBuffer(length, usage, format, pool, indexBuffer, sharedHandle);
	}

	long Direct3DDevice9Ex::CreateOffscreenPlainSurface(unsigned int width, unsigned int height, D3DFORMAT format, D3DPOOL pool, IDirect3DSurface9 **surface, void **sharedHandle)
	{
		TRACE("unsigned int width = %d, unsigned int height = %d, D3DFORMAT format = %d, D3DPOOL pool = %d, IDirect3DSurface9 **surface = 0x%0.8p, void **sharedHandle = 0x%0.8p", width, height, format, pool, surface, sharedHandle);

		return Direct3DDevice9::CreateOffscreenPlainSurface(width, height, format, pool, surface, sharedHandle);
	}

	long Direct3DDevice9Ex::CreatePixelShader(const unsigned long *function, IDirect3DPixelShader9 **shader)
	{
		TRACE("const unsigned long *function = 0x%0.8p, IDirect3DPixelShader9 **shader = 0x%0.8p", function, shader);

		return Direct3DDevice9::CreatePixelShader(function, shader);
	}

	long Direct3DDevice9Ex::CreateQuery(D3DQUERYTYPE type, IDirect3DQuery9 **query)
	{
		TRACE("D3DQUERYTYPE type = %d, IDirect3DQuery9 **query = 0x%0.8p", type, query);

		return Direct3DDevice9::CreateQuery(type, query);
	}

	long Direct3DDevice9Ex::CreateRenderTarget(unsigned int width, unsigned int height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multiSample, unsigned long multiSampleQuality, int lockable, IDirect3DSurface9 **surface, void **sharedHandle)
	{
		TRACE("unsigned int width = %d, unsigned int height = %d, D3DFORMAT format = %d, D3DMULTISAMPLE_TYPE multiSample = %d, unsigned long multiSampleQuality = %d, int lockable = %d, IDirect3DSurface9 **surface = 0x%0.8p, void **sharedHandle = 0x%0.8p", width, height, format, multiSample, multiSampleQuality, lockable, surface, sharedHandle);

		return Direct3DDevice9::CreateRenderTarget(width, height, format, multiSample, multiSampleQuality, lockable, surface, sharedHandle);
	}

	long Direct3DDevice9Ex::CreateStateBlock(D3DSTATEBLOCKTYPE type, IDirect3DStateBlock9 **stateBlock)
	{
		TRACE("D3DSTATEBLOCKTYPE type = %d, IDirect3DStateBlock9 **stateBlock = 0x%0.8p", type, stateBlock);

		return Direct3DDevice9::CreateStateBlock(type, stateBlock);
	}

	long Direct3DDevice9Ex::CreateTexture(unsigned int width, unsigned int height, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture9 **texture, void **sharedHandle)
	{
		TRACE("unsigned int width = %d, unsigned int height = %d, unsigned int levels = %d, unsigned long usage = %d, D3DFORMAT format = %d, D3DPOOL pool = %d, IDirect3DTexture9 **texture = 0x%0.8p, void **sharedHandle = 0x%0.8p", width, height, levels, usage, format, pool, texture, sharedHandle);

		return Direct3DDevice9::CreateTexture(width, height, levels, usage, format, pool, texture, sharedHandle);
	}

	long Direct3DDevice9Ex::CreateVertexBuffer(unsigned int length, unsigned long usage, unsigned long FVF, D3DPOOL pool, IDirect3DVertexBuffer9 **vertexBuffer, void **sharedHandle)
	{
		TRACE("unsigned int length = %d, unsigned long usage = %d, unsigned long FVF = 0x%0.8X, D3DPOOL pool = %d, IDirect3DVertexBuffer9 **vertexBuffer = 0x%0.8p, void **sharedHandle = 0x%0.8p", length, usage, FVF, pool, vertexBuffer, sharedHandle);

		return Direct3DDevice9::CreateVertexBuffer(length, usage, FVF, pool, vertexBuffer, sharedHandle);
	}

	long Direct3DDevice9Ex::CreateVertexDeclaration(const D3DVERTEXELEMENT9 *vertexElements, IDirect3DVertexDeclaration9 **declaration)
	{
		TRACE("const D3DVERTEXELEMENT9 *vertexElements = 0x%0.8p, IDirect3DVertexDeclaration9 **declaration = 0x%0.8p", vertexElements, declaration);

		return Direct3DDevice9::CreateVertexDeclaration(vertexElements, declaration);
	}

	long Direct3DDevice9Ex::CreateVertexShader(const unsigned long *function, IDirect3DVertexShader9 **shader)
	{
		TRACE("const unsigned long *function = 0x%0.8p, IDirect3DVertexShader9 **shader = 0x%0.8p", function, shader);

		return Direct3DDevice9::CreateVertexShader(function, shader);
	}

	long Direct3DDevice9Ex::CreateVolumeTexture(unsigned int width, unsigned int height, unsigned int depth, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DVolumeTexture9 **volumeTexture, void **sharedHandle)
	{
		TRACE("unsigned int width = %d, unsigned int height = %d, unsigned int depth = %d, unsigned int levels = %d, unsigned long usage = %d, D3DFORMAT format = %d, D3DPOOL pool = %d, IDirect3DVolumeTexture9 **volumeTexture = 0x%0.8p, void **sharedHandle = 0x%0.8p", width, height, depth, levels, usage, format, pool, volumeTexture, sharedHandle);

		return Direct3DDevice9::CreateVolumeTexture(width, height, depth, levels, usage, format, pool, volumeTexture, sharedHandle);
	}

	long Direct3DDevice9Ex::DeletePatch(unsigned int handle)
	{
		TRACE("unsigned int handle = %d", handle);

		return Direct3DDevice9::DeletePatch(handle);
	}

	long Direct3DDevice9Ex::DrawIndexedPrimitive(D3DPRIMITIVETYPE type, int baseVertexIndex, unsigned int minIndex, unsigned int numVertices, unsigned int startIndex, unsigned int primitiveCount)
	{
		TRACE("D3DPRIMITIVETYPE type = %d, int baseVertexIndex = %d, unsigned int minIndex = %d, unsigned int numVertices = %d, unsigned int startIndex = %d, unsigned int primitiveCount = %d", type, baseVertexIndex, minIndex, numVertices, startIndex, primitiveCount);

		return Direct3DDevice9::DrawIndexedPrimitive(type, baseVertexIndex, minIndex, numVertices, startIndex, primitiveCount);
	}

	long Direct3DDevice9Ex::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE type, unsigned int minIndex, unsigned int numVertices, unsigned int primitiveCount, const void *indexData, D3DFORMAT indexDataFormat, const void *vertexStreamZeroData, unsigned int vertexStreamZeroStride)
	{
		TRACE("D3DPRIMITIVETYPE type = %d, unsigned int minIndex = %d, unsigned int numVertices = %d, unsigned int primitiveCount = %d, const void *indexData = 0x%0.8p, D3DFORMAT indexDataFormat = %d, const void *vertexStreamZeroData = 0x%0.8p, unsigned int vertexStreamZeroStride = %d", type, minIndex, numVertices, primitiveCount, indexData, indexDataFormat, vertexStreamZeroData, vertexStreamZeroStride);

		return Direct3DDevice9::DrawIndexedPrimitiveUP(type, minIndex, numVertices, primitiveCount, indexData, indexDataFormat, vertexStreamZeroData, vertexStreamZeroStride);
	}

	long Direct3DDevice9Ex::DrawPrimitive(D3DPRIMITIVETYPE primitiveType, unsigned int startVertex, unsigned int primitiveCount)
	{
		TRACE("D3DPRIMITIVETYPE primitiveType = %d, unsigned int startVertex = %d, unsigned int primitiveCount = %d", primitiveType, startVertex, primitiveCount);

		return Direct3DDevice9::DrawPrimitive(primitiveType, startVertex, primitiveCount);
	}

	long Direct3DDevice9Ex::DrawPrimitiveUP(D3DPRIMITIVETYPE primitiveType, unsigned int primitiveCount, const void *vertexStreamZeroData, unsigned int vertexStreamZeroStride)
	{
		TRACE("D3DPRIMITIVETYPE primitiveType = %d, unsigned int primitiveCount = %d, const void *vertexStreamZeroData = 0x%0.8p, unsigned int vertexStreamZeroStride = %d", primitiveType, primitiveCount, vertexStreamZeroData, vertexStreamZeroStride);

		return Direct3DDevice9::DrawPrimitiveUP(primitiveType, primitiveCount, vertexStreamZeroData, vertexStreamZeroStride);
	}

	long Direct3DDevice9Ex::DrawRectPatch(unsigned int handle, const float *numSegs, const D3DRECTPATCH_INFO *rectPatchInfo)
	{
		TRACE("unsigned int handle = %d, const float *numSegs = 0x%0.8p, const D3DRECTPATCH_INFO *rectPatchInfo = 0x%0.8p", handle, numSegs, rectPatchInfo);

		return Direct3DDevice9::DrawRectPatch(handle, numSegs, rectPatchInfo);
	}

	long Direct3DDevice9Ex::DrawTriPatch(unsigned int handle, const float *numSegs, const D3DTRIPATCH_INFO *triPatchInfo)
	{
		TRACE("unsigned int handle = %d, const float *numSegs = 0x%0.8p, const D3DTRIPATCH_INFO *triPatchInfo = 0x%0.8p", handle, numSegs, triPatchInfo);

		return Direct3DDevice9::DrawTriPatch(handle, numSegs, triPatchInfo);
	}

	long Direct3DDevice9Ex::EndScene()
	{
		TRACE("void");

		return Direct3DDevice9::EndScene();
	}

	long Direct3DDevice9Ex::EndStateBlock(IDirect3DStateBlock9 **stateBlock)
	{
		TRACE("IDirect3DStateBlock9 **stateBlock = 0x%0.8p", stateBlock);

		return Direct3DDevice9::EndStateBlock(stateBlock);
	}

	long Direct3DDevice9Ex::EvictManagedResources()
	{
		TRACE("void");

		return Direct3DDevice9::EvictManagedResources();
	}

	unsigned int Direct3DDevice9Ex::GetAvailableTextureMem()
	{
		TRACE("void");

		return Direct3DDevice9::GetAvailableTextureMem();
	}

	long Direct3DDevice9Ex::GetBackBuffer(unsigned int swapChainIndex, unsigned int backBufferIndex, D3DBACKBUFFER_TYPE type, IDirect3DSurface9 **backBuffer)
	{
		TRACE("unsigned int swapChainIndex = %d, unsigned int backBufferIndex = %d, D3DBACKBUFFER_TYPE type = %d, IDirect3DSurface9 **backBuffer = 0x%0.8p", swapChainIndex, backBufferIndex, type, backBuffer);

		return Direct3DDevice9::GetBackBuffer(swapChainIndex, backBufferIndex, type, backBuffer);
	}

	long Direct3DDevice9Ex::GetClipPlane(unsigned long index, float *plane)
	{
		TRACE("unsigned long index = %d, float *plane = 0x%0.8p", index, plane);

		return Direct3DDevice9::GetClipPlane(index, plane);
	}

	long Direct3DDevice9Ex::GetClipStatus(D3DCLIPSTATUS9 *clipStatus)
	{
		TRACE("D3DCLIPSTATUS9 *clipStatus = 0x%0.8p", clipStatus);

		return Direct3DDevice9::GetClipStatus(clipStatus);
	}

	long Direct3DDevice9Ex::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *parameters)
	{
		TRACE("D3DDEVICE_CREATION_PARAMETERS *parameters = 0x%0.8p", parameters);

		return Direct3DDevice9::GetCreationParameters(parameters);
	}

	long Direct3DDevice9Ex::GetCurrentTexturePalette(unsigned int *paletteNumber)
	{
		TRACE("unsigned int *paletteNumber = 0x%0.8p", paletteNumber);

		return Direct3DDevice9::GetCurrentTexturePalette(paletteNumber);
	}

	long Direct3DDevice9Ex::GetDepthStencilSurface(IDirect3DSurface9 **depthStencilSurface)
	{
		TRACE("IDirect3DSurface9 **depthStencilSurface = 0x%0.8p", depthStencilSurface);

		return Direct3DDevice9::GetDepthStencilSurface(depthStencilSurface);
	}

	long Direct3DDevice9Ex::GetDeviceCaps(D3DCAPS9 *caps)
	{
		TRACE("D3DCAPS9 *caps = 0x%0.8p", caps);

		return d3d9ex->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, caps);
	}

	long Direct3DDevice9Ex::GetDirect3D(IDirect3D9 **d3d9)
	{
		TRACE("IDirect3D9 **d3d9 = 0x%0.8p", d3d9);

		return Direct3DDevice9::GetDirect3D(d3d9);
	}

	long Direct3DDevice9Ex::GetDisplayMode(unsigned int index, D3DDISPLAYMODE *mode)
	{
		TRACE("unsigned int index = %d, D3DDISPLAYMODE *mode = 0x%0.8p", index, mode);

		return Direct3DDevice9::GetDisplayMode(index, mode);
	}

	long Direct3DDevice9Ex::GetFrontBufferData(unsigned int index, IDirect3DSurface9 *destSurface)
	{
		TRACE("unsigned int index = %d, IDirect3DSurface9 *destSurface = %p", index, destSurface);

		return Direct3DDevice9::GetFrontBufferData(index, destSurface);
	}

	long Direct3DDevice9Ex::GetFVF(unsigned long *FVF)
	{
		TRACE("unsigned long *FVF = 0x%0.8p", FVF);

		return Direct3DDevice9::GetFVF(FVF);
	}

	void Direct3DDevice9Ex::GetGammaRamp(unsigned int index, D3DGAMMARAMP *ramp)
	{
		TRACE("unsigned int index = %d, D3DGAMMARAMP *ramp = 0x%0.8p", index, ramp);

		return Direct3DDevice9::GetGammaRamp(index, ramp);
	}

	long Direct3DDevice9Ex::GetIndices(IDirect3DIndexBuffer9 **indexData)
	{
		TRACE("IDirect3DIndexBuffer9 **indexData = 0x%0.8p", indexData);

		return Direct3DDevice9::GetIndices(indexData);
	}

	long Direct3DDevice9Ex::GetLight(unsigned long index, D3DLIGHT9 *light)
	{
		TRACE("unsigned long index = %d, D3DLIGHT9 *light = 0x%0.8p", index, light);

		return Direct3DDevice9::GetLight(index, light);
	}

	long Direct3DDevice9Ex::GetLightEnable(unsigned long index, int *enable)
	{
		TRACE("unsigned long index = %d, int *enable = 0x%0.8p", index, enable);

		return Direct3DDevice9::GetLightEnable(index, enable);
	}

	long Direct3DDevice9Ex::GetMaterial(D3DMATERIAL9 *material)
	{
		TRACE("D3DMATERIAL9 *material = 0x%0.8p", material);

		return Direct3DDevice9::GetMaterial(material);
	}

	float Direct3DDevice9Ex::GetNPatchMode()
	{
		TRACE("void");

		return Direct3DDevice9::GetNPatchMode();
	}

	unsigned int Direct3DDevice9Ex::GetNumberOfSwapChains()
	{
		TRACE("void");

		return Direct3DDevice9::GetNumberOfSwapChains();
	}

	long Direct3DDevice9Ex::GetPaletteEntries(unsigned int paletteNumber, PALETTEENTRY *entries)
	{
		TRACE("unsigned int paletteNumber = %d, PALETTEENTRY *entries = 0x%0.8p", paletteNumber, entries);

		return Direct3DDevice9::GetPaletteEntries(paletteNumber, entries);
	}

	long Direct3DDevice9Ex::GetPixelShader(IDirect3DPixelShader9 **shader)
	{
		TRACE("IDirect3DPixelShader9 **shader = 0x%0.8p", shader);

		return Direct3DDevice9::GetPixelShader(shader);
	}

	long Direct3DDevice9Ex::GetPixelShaderConstantB(unsigned int startRegister, int *constantData, unsigned int count)
	{
		TRACE("unsigned int startRegister = %d, int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		return Direct3DDevice9::GetPixelShaderConstantB(startRegister, constantData, count);
	}

	long Direct3DDevice9Ex::GetPixelShaderConstantF(unsigned int startRegister, float *constantData, unsigned int count)
	{
		TRACE("unsigned int startRegister = %d, int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		return Direct3DDevice9::GetPixelShaderConstantF(startRegister, constantData, count);
	}

	long Direct3DDevice9Ex::GetPixelShaderConstantI(unsigned int startRegister, int *constantData, unsigned int count)
	{
		TRACE("unsigned int startRegister = %d, int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		return Direct3DDevice9::GetPixelShaderConstantI(startRegister, constantData, count);
	}

	long Direct3DDevice9Ex::GetRasterStatus(unsigned int swapChain, D3DRASTER_STATUS *rasterStatus)
	{
		TRACE("unsigned int swapChain = %d, D3DRASTER_STATUS *rasterStatus = 0x%0.8p", swapChain, rasterStatus);

		return Direct3DDevice9::GetRasterStatus(swapChain, rasterStatus);
	}

	long Direct3DDevice9Ex::GetRenderState(D3DRENDERSTATETYPE state, unsigned long *value)
	{
		TRACE("D3DRENDERSTATETYPE state = %d, unsigned long *value = 0x%0.8p", state, value);

		return Direct3DDevice9::GetRenderState(state, value);
	}

	long Direct3DDevice9Ex::GetRenderTarget(unsigned long index, IDirect3DSurface9 **renderTarget)
	{
		TRACE("unsigned long index = %d, IDirect3DSurface9 **renderTarget = 0x%0.8p", index, renderTarget);

		return Direct3DDevice9::GetRenderTarget(index, renderTarget);
	}

	long Direct3DDevice9Ex::GetRenderTargetData(IDirect3DSurface9 *renderTarget, IDirect3DSurface9 *destSurface)
	{
		TRACE("IDirect3DSurface9 *renderTarget = 0x%0.8p, IDirect3DSurface9 *destSurface = 0x%0.8p", renderTarget, destSurface);

		return Direct3DDevice9::GetRenderTargetData(renderTarget, destSurface);
	}

	long Direct3DDevice9Ex::GetSamplerState(unsigned long sampler, D3DSAMPLERSTATETYPE state, unsigned long *value)
	{
		TRACE("unsigned long sampler = %d, D3DSAMPLERSTATETYPE type = %d, unsigned long *value = 0x%0.8p", sampler, state, value);

		return Direct3DDevice9::GetSamplerState(sampler, state, value);
	}

	long Direct3DDevice9Ex::GetScissorRect(RECT *rect)
	{
		TRACE("RECT *rect = 0x%0.8p", rect);

		return Direct3DDevice9::GetScissorRect(rect);
	}

	int Direct3DDevice9Ex::GetSoftwareVertexProcessing()
	{
		TRACE("void");

		return Direct3DDevice9::GetSoftwareVertexProcessing();
	}

	long Direct3DDevice9Ex::GetStreamSource(unsigned int streamNumber, IDirect3DVertexBuffer9 **streamData, unsigned int *offset, unsigned int *stride)
	{
		TRACE("unsigned int streamNumber = %d, IDirect3DVertexBuffer9 **streamData = 0x%0.8p, unsigned int *offset = 0x%0.8p, unsigned int *stride = 0x%0.8p", streamNumber, streamData, offset, stride);

		return Direct3DDevice9::GetStreamSource(streamNumber, streamData, offset, stride);
	}

	long Direct3DDevice9Ex::GetStreamSourceFreq(unsigned int streamNumber, unsigned int *divider)
	{
		TRACE("unsigned int streamNumber = %d, unsigned int *divider = 0x%0.8p", streamNumber, divider);

		return Direct3DDevice9::GetStreamSourceFreq(streamNumber, divider);
	}

	long Direct3DDevice9Ex::GetSwapChain(unsigned int index, IDirect3DSwapChain9 **swapChain)
	{
		TRACE("unsigned int index = %d, IDirect3DSwapChain9 **swapChain = 0x%0.8p", index, swapChain);

		return Direct3DDevice9::GetSwapChain(index, swapChain);
	}

	long Direct3DDevice9Ex::GetTexture(unsigned long sampler, IDirect3DBaseTexture9 **texture)
	{
		TRACE("unsigned long sampler = %d, IDirect3DBaseTexture9 **texture = 0x%0.8p", sampler, texture);

		return Direct3DDevice9::GetTexture(sampler, texture);
	}

	long Direct3DDevice9Ex::GetTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type, unsigned long *value)
	{
		TRACE("unsigned long stage = %d, D3DTEXTURESTAGESTATETYPE type = %d, unsigned long *value = 0x%0.8p", stage, type, value);

		return Direct3DDevice9::GetTextureStageState(stage, type, value);
	}

	long Direct3DDevice9Ex::GetTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
	{
		TRACE("D3DTRANSFORMSTATETYPE state = %d, D3DMATRIX *matrix = 0x%0.8p", state, matrix);

		return Direct3DDevice9::GetTransform(state, matrix);
	}

	long Direct3DDevice9Ex::GetVertexDeclaration(IDirect3DVertexDeclaration9 **declaration)
	{
		TRACE("IDirect3DVertexDeclaration9 **declaration = 0x%0.8p", declaration);

		return Direct3DDevice9::GetVertexDeclaration(declaration);
	}

	long Direct3DDevice9Ex::GetVertexShader(IDirect3DVertexShader9 **shader)
	{
		TRACE("IDirect3DVertexShader9 **shader = 0x%0.8p", shader);

		return Direct3DDevice9::GetVertexShader(shader);
	}

	long Direct3DDevice9Ex::GetVertexShaderConstantB(unsigned int startRegister, int *constantData, unsigned int count)
	{
		TRACE("unsigned int startRegister = %d, int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		return Direct3DDevice9::GetVertexShaderConstantB(startRegister, constantData, count);
	}

	long Direct3DDevice9Ex::GetVertexShaderConstantF(unsigned int startRegister, float *constantData, unsigned int count)
	{
		TRACE("unsigned int startRegister = %d, int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		return Direct3DDevice9::GetVertexShaderConstantF(startRegister, constantData, count);
	}

	long Direct3DDevice9Ex::GetVertexShaderConstantI(unsigned int startRegister, int *constantData, unsigned int count)
	{
		TRACE("unsigned int startRegister = %d, int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		return Direct3DDevice9::GetVertexShaderConstantI(startRegister, constantData, count);
	}

	long Direct3DDevice9Ex::GetViewport(D3DVIEWPORT9 *viewport)
	{
		TRACE("D3DVIEWPORT9 *viewport = 0x%0.8p", viewport);

		return Direct3DDevice9::GetViewport(viewport);
	}

	long Direct3DDevice9Ex::LightEnable(unsigned long index, int enable)
	{
		TRACE("unsigned long index = %d, int enable = %d", index, enable);

		return Direct3DDevice9::LightEnable(index, enable);
	}

	long Direct3DDevice9Ex::MultiplyTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix)
	{
		TRACE("D3DTRANSFORMSTATETYPE state = %d, const D3DMATRIX *matrix = 0x%0.8p", state, matrix);

		return Direct3DDevice9::MultiplyTransform(state, matrix);
	}

	long Direct3DDevice9Ex::Present(const RECT *sourceRect, const RECT *destRect, HWND destWindowOverride, const RGNDATA *dirtyRegion)
	{
		TRACE("const RECT *sourceRect = 0x%0.8p, const RECT *destRect = 0x%0.8p, HWND destWindowOverride = %d, const RGNDATA *dirtyRegion = 0x%0.8p", sourceRect, destRect, destWindowOverride, dirtyRegion);

		return Direct3DDevice9::Present(sourceRect, destRect, destWindowOverride, dirtyRegion);
	}

	long Direct3DDevice9Ex::ProcessVertices(unsigned int srcStartIndex, unsigned int destIndex, unsigned int vertexCount, IDirect3DVertexBuffer9 *destBuffer, IDirect3DVertexDeclaration9 *vertexDeclaration, unsigned long flags)
	{
		TRACE("unsigned int srcStartIndex = %d, unsigned int destIndex = %d, unsigned int vertexCount = %d, IDirect3DVertexBuffer9 *destBuffer = 0x%0.8p, IDirect3DVertexDeclaration9 *vertexDeclaration = 0x%0.8p, unsigned long flags = %d", srcStartIndex, destIndex, vertexCount, destBuffer, vertexDeclaration, flags);

		return Direct3DDevice9::ProcessVertices(srcStartIndex, destIndex, vertexCount, destBuffer, vertexDeclaration, flags);
	}

	long Direct3DDevice9Ex::Reset(D3DPRESENT_PARAMETERS *presentParameters)
	{
		TRACE("D3DPRESENT_PARAMETERS *presentParameters = 0x%0.8p", presentParameters);

		return Direct3DDevice9::Reset(presentParameters);
	}

	long Direct3DDevice9Ex::SetClipPlane(unsigned long index, const float *plane)
	{
		TRACE("unsigned long index = %d, const float *plane = 0x%0.8p", index, plane);

		return Direct3DDevice9::SetClipPlane(index, plane);
	}

	long Direct3DDevice9Ex::SetClipStatus(const D3DCLIPSTATUS9 *clipStatus)
	{
		TRACE("const D3DCLIPSTATUS9 *clipStatus = 0x%0.8p", clipStatus);

		return Direct3DDevice9::SetClipStatus(clipStatus);
	}

	long Direct3DDevice9Ex::SetCurrentTexturePalette(unsigned int paletteNumber)
	{
		TRACE("unsigned int paletteNumber = %d", paletteNumber);

		return Direct3DDevice9::SetCurrentTexturePalette(paletteNumber);
	}

	void Direct3DDevice9Ex::SetCursorPosition(int x, int y, unsigned long flags)
	{
		TRACE("int x = %d, int y = %d, unsigned long flags = 0x%0.8X", x, y, flags);

		return Direct3DDevice9::SetCursorPosition(x, y, flags);
	}

	long Direct3DDevice9Ex::SetCursorProperties(unsigned int x, unsigned int y, IDirect3DSurface9 *cursorBitmap)
	{
		TRACE("unsigned int x = %d, unsigned int y = %d, IDirect3DSurface9 *cursorBitmap = 0x%0.8p", x, y, cursorBitmap);

		return Direct3DDevice9::SetCursorProperties(x, y, cursorBitmap);
	}

	long Direct3DDevice9Ex::SetDepthStencilSurface(IDirect3DSurface9 *iDepthStencil)
	{
		TRACE("IDirect3DSurface9 *newDepthStencil = 0x%0.8p", iDepthStencil);

		return Direct3DDevice9::SetDepthStencilSurface(iDepthStencil);
	}

	long Direct3DDevice9Ex::SetDialogBoxMode(int enableDialogs)
	{
		TRACE("int enableDialogs = %d", enableDialogs);

		return Direct3DDevice9::SetDialogBoxMode(enableDialogs);
	}

	long Direct3DDevice9Ex::SetFVF(unsigned long FVF)
	{
		TRACE("unsigned long FVF = 0x%0.8X", FVF);

		return Direct3DDevice9::SetFVF(FVF);
	}

	void Direct3DDevice9Ex::SetGammaRamp(unsigned int index, unsigned long flags, const D3DGAMMARAMP *ramp)
	{
		TRACE("unsigned int index = %d, unsigned long flags = 0x%0.8X, const D3DGAMMARAMP *ramp = 0x%0.8p", index, flags, ramp);

		return Direct3DDevice9::SetGammaRamp(index, flags, ramp);
	}

	long Direct3DDevice9Ex::SetIndices(IDirect3DIndexBuffer9* iIndexBuffer)
	{
		TRACE("IDirect3DIndexBuffer9* indexData = 0x%0.8p", iIndexBuffer);

		return Direct3DDevice9::SetIndices(iIndexBuffer);
	}

	long Direct3DDevice9Ex::SetLight(unsigned long index, const D3DLIGHT9 *light)
	{
		TRACE("unsigned long index = %d, const D3DLIGHT9 *light = 0x%0.8p", index, light);

		return Direct3DDevice9::SetLight(index, light);
	}

	long Direct3DDevice9Ex::SetMaterial(const D3DMATERIAL9 *material)
	{
		TRACE("const D3DMATERIAL9 *material = 0x%0.8p", material);

		return Direct3DDevice9::SetMaterial(material);
	}

	long Direct3DDevice9Ex::SetNPatchMode(float segments)
	{
		TRACE("float segments = %f", segments);

		return Direct3DDevice9::SetNPatchMode(segments);
	}

	long Direct3DDevice9Ex::SetPaletteEntries(unsigned int paletteNumber, const PALETTEENTRY *entries)
	{
		TRACE("unsigned int paletteNumber = %d, const PALETTEENTRY *entries = 0x%0.8p", paletteNumber, entries);

		return Direct3DDevice9::SetPaletteEntries(paletteNumber, entries);
	}

	long Direct3DDevice9Ex::SetPixelShader(IDirect3DPixelShader9 *iPixelShader)
	{
		TRACE("IDirect3DPixelShader9 *shader = 0x%0.8p", iPixelShader);

		return Direct3DDevice9::SetPixelShader(iPixelShader);
	}

	long Direct3DDevice9Ex::SetPixelShaderConstantB(unsigned int startRegister, const int *constantData, unsigned int count)
	{
		TRACE("unsigned int startRegister = %d, const int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		return Direct3DDevice9::SetPixelShaderConstantB(startRegister, constantData, count);
	}

	long Direct3DDevice9Ex::SetPixelShaderConstantF(unsigned int startRegister, const float *constantData, unsigned int count)
	{
		TRACE("unsigned int startRegister = %d, const int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		return Direct3DDevice9::SetPixelShaderConstantF(startRegister, constantData, count);
	}

	long Direct3DDevice9Ex::SetPixelShaderConstantI(unsigned int startRegister, const int *constantData, unsigned int count)
	{
		TRACE("unsigned int startRegister = %d, const int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		return Direct3DDevice9::SetPixelShaderConstantI(startRegister, constantData, count);
	}

	long Direct3DDevice9Ex::SetRenderState(D3DRENDERSTATETYPE state, unsigned long value)
	{
		TRACE("D3DRENDERSTATETYPE state = %d, unsigned long value = %d", state, value);

		return Direct3DDevice9::SetRenderState(state, value);
	}

	long Direct3DDevice9Ex::SetRenderTarget(unsigned long index, IDirect3DSurface9 *iRenderTarget)
	{
		TRACE("unsigned long index = %d, IDirect3DSurface9 *newRenderTarget = 0x%0.8p", index, iRenderTarget);

		return Direct3DDevice9::SetRenderTarget(index, iRenderTarget);
	}

	long Direct3DDevice9Ex::SetSamplerState(unsigned long sampler, D3DSAMPLERSTATETYPE state, unsigned long value)
	{
		TRACE("unsigned long sampler = %d, D3DSAMPLERSTATETYPE state = %d, unsigned long value = %d", sampler, state, value);

		return Direct3DDevice9::SetSamplerState(sampler, state, value);
	}

	long Direct3DDevice9Ex::SetScissorRect(const RECT *rect)
	{
		TRACE("const RECT *rect = 0x%0.8p", rect);

		return Direct3DDevice9::SetScissorRect(rect);
	}

	long Direct3DDevice9Ex::SetSoftwareVertexProcessing(int software)
	{
		TRACE("int software = %d", software);

		return Direct3DDevice9::SetSoftwareVertexProcessing(software);
	}

	long Direct3DDevice9Ex::SetStreamSource(unsigned int stream, IDirect3DVertexBuffer9 *iVertexBuffer, unsigned int offset, unsigned int stride)
	{
		TRACE("unsigned int stream = %d, IDirect3DVertexBuffer9 *data = 0x%0.8p, unsigned int offset = %d, unsigned int stride = %d", stream, iVertexBuffer, offset, stride);

		return Direct3DDevice9::SetStreamSource(stream, iVertexBuffer, offset, stride);
	}

	long Direct3DDevice9Ex::SetStreamSourceFreq(unsigned int streamNumber, unsigned int divider)
	{
		TRACE("unsigned int streamNumber = %d, unsigned int divider = %d", streamNumber, divider);

		return Direct3DDevice9::SetStreamSourceFreq(streamNumber, divider);
	}

	long Direct3DDevice9Ex::SetTexture(unsigned long sampler, IDirect3DBaseTexture9 *iBaseTexture)
	{
		TRACE("unsigned long sampler = %d, IDirect3DBaseTexture9 *texture = 0x%0.8p", sampler, iBaseTexture);

		return Direct3DDevice9::SetTexture(sampler, iBaseTexture);
	}

	long Direct3DDevice9Ex::SetTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type, unsigned long value)
	{
		TRACE("unsigned long stage = %d, D3DTEXTURESTAGESTATETYPE type = %d, unsigned long value = %d", stage, type, value);

		return Direct3DDevice9::SetTextureStageState(stage, type, value);
	}

	long Direct3DDevice9Ex::SetTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix)
	{
		TRACE("D3DTRANSFORMSTATETYPE state = %d, const D3DMATRIX *matrix = 0x%0.8p", state, matrix);

		return Direct3DDevice9::SetTransform(state, matrix);
	}

	long Direct3DDevice9Ex::SetVertexDeclaration(IDirect3DVertexDeclaration9 *iVertexDeclaration)
	{
		TRACE("IDirect3DVertexDeclaration9 *declaration = 0x%0.8p", iVertexDeclaration);

		return Direct3DDevice9::SetVertexDeclaration(iVertexDeclaration);
	}

	long Direct3DDevice9Ex::SetVertexShader(IDirect3DVertexShader9 *iVertexShader)
	{
		TRACE("IDirect3DVertexShader9 *shader = 0x%0.8p", iVertexShader);

		return Direct3DDevice9::SetVertexShader(iVertexShader);
	}

	long Direct3DDevice9Ex::SetVertexShaderConstantB(unsigned int startRegister, const int *constantData, unsigned int count)
	{
		TRACE("unsigned int startRegister = %d, const int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		return Direct3DDevice9::SetVertexShaderConstantB(startRegister, constantData, count);
	}

	long Direct3DDevice9Ex::SetVertexShaderConstantF(unsigned int startRegister, const float *constantData, unsigned int count)
	{
		TRACE("unsigned int startRegister = %d, const int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		return Direct3DDevice9::SetVertexShaderConstantF(startRegister, constantData, count);
	}

	long Direct3DDevice9Ex::SetVertexShaderConstantI(unsigned int startRegister, const int *constantData, unsigned int count)
	{
		TRACE("unsigned int startRegister = %d, const int *constantData = 0x%0.8p, unsigned int count = %d", startRegister, constantData, count);

		return Direct3DDevice9::SetVertexShaderConstantI(startRegister, constantData, count);
	}

	long Direct3DDevice9Ex::SetViewport(const D3DVIEWPORT9 *viewport)
	{
		TRACE("const D3DVIEWPORT9 *viewport = 0x%0.8p", viewport);

		return Direct3DDevice9::SetViewport(viewport);
	}

	int Direct3DDevice9Ex::ShowCursor(int show)
	{
		TRACE("int show = %d", show);

		return Direct3DDevice9::ShowCursor(show);
	}

	long Direct3DDevice9Ex::StretchRect(IDirect3DSurface9 *sourceSurface, const RECT *sourceRect, IDirect3DSurface9 *destSurface, const RECT *destRect, D3DTEXTUREFILTERTYPE filter)
	{
		TRACE("IDirect3DSurface9 *sourceSurface = 0x%0.8p, const RECT *sourceRect = 0x%0.8p, IDirect3DSurface9 *destSurface = 0x%0.8p, const RECT *destRect = 0x%0.8p, D3DTEXTUREFILTERTYPE filter = %d", sourceSurface, sourceRect, destSurface, destRect, filter);

		return Direct3DDevice9::StretchRect(sourceSurface, sourceRect, destSurface, destRect, filter);
	}

	long Direct3DDevice9Ex::TestCooperativeLevel()
	{
		TRACE("void");

		return Direct3DDevice9::TestCooperativeLevel();
	}

	long Direct3DDevice9Ex::UpdateSurface(IDirect3DSurface9 *sourceSurface, const RECT *sourceRect, IDirect3DSurface9 *destinationSurface, const POINT *destPoint)
	{
		TRACE("IDirect3DSurface9 *sourceSurface = 0x%0.8p, const RECT *sourceRect = 0x%0.8p, IDirect3DSurface9 *destinationSurface = 0x%0.8p, const POINT *destPoint = 0x%0.8p", sourceSurface, sourceRect, destinationSurface, destPoint);

		return Direct3DDevice9::UpdateSurface(sourceSurface, sourceRect, destinationSurface, destPoint);
	}

	long Direct3DDevice9Ex::UpdateTexture(IDirect3DBaseTexture9 *sourceTexture, IDirect3DBaseTexture9 *destinationTexture)
	{
		TRACE("IDirect3DBaseTexture9 *sourceTexture = 0x%0.8p, IDirect3DBaseTexture9 *destinationTexture = 0x%0.8p", sourceTexture, destinationTexture);

		return Direct3DDevice9::UpdateTexture(sourceTexture, destinationTexture);
	}

	long Direct3DDevice9Ex::ValidateDevice(unsigned long *numPasses)
	{
		TRACE("unsigned long *numPasses = 0x%0.8p", numPasses);

		return Direct3DDevice9::ValidateDevice(numPasses);
	}

	HRESULT Direct3DDevice9Ex::SetConvolutionMonoKernel(UINT,UINT,float *,float *)
	{
		CriticalSection cs(this);

		UNIMPLEMENTED();

		return D3D_OK;
	}

	HRESULT Direct3DDevice9Ex::ComposeRects(IDirect3DSurface9 *,IDirect3DSurface9 *,IDirect3DVertexBuffer9 *,UINT,IDirect3DVertexBuffer9 *,D3DCOMPOSERECTSOP,int,int)
	{
		CriticalSection cs(this);

		UNIMPLEMENTED();

		return D3D_OK;
	}

	HRESULT Direct3DDevice9Ex::PresentEx(const RECT *,const RECT *,HWND,const RGNDATA *,DWORD)
	{
		CriticalSection cs(this);

		UNIMPLEMENTED();

		return D3D_OK;
	}

	HRESULT Direct3DDevice9Ex::GetGPUThreadPriority(INT *)
	{
		CriticalSection cs(this);

		UNIMPLEMENTED();

		return D3D_OK;
	}

	HRESULT Direct3DDevice9Ex::SetGPUThreadPriority(INT)
	{
		CriticalSection cs(this);

		UNIMPLEMENTED();

		return D3D_OK;
	}

	HRESULT Direct3DDevice9Ex::WaitForVBlank(UINT)
	{
		CriticalSection cs(this);

		UNIMPLEMENTED();

		return D3D_OK;
	}

	HRESULT Direct3DDevice9Ex::CheckResourceResidency(IDirect3DResource9 **,UINT32)
	{
		CriticalSection cs(this);

		UNIMPLEMENTED();

		return D3D_OK;
	}

	HRESULT Direct3DDevice9Ex::SetMaximumFrameLatency(UINT)
	{
		CriticalSection cs(this);

		UNIMPLEMENTED();

		return D3D_OK;
	}

	HRESULT Direct3DDevice9Ex::GetMaximumFrameLatency(UINT *)
	{
		CriticalSection cs(this);

		UNIMPLEMENTED();

		return D3D_OK;
	}

	HRESULT Direct3DDevice9Ex::CheckDeviceState(HWND destinationWindow)
	{
		CriticalSection cs(this);

	//	UNIMPLEMENTED();

		return D3D_OK;
	}

	HRESULT Direct3DDevice9Ex::CreateRenderTargetEx(UINT,UINT,D3DFORMAT,D3DMULTISAMPLE_TYPE,DWORD,BOOL,IDirect3DSurface9 **,HANDLE *,DWORD)
	{
		CriticalSection cs(this);

		UNIMPLEMENTED();

		return D3D_OK;
	}

	HRESULT Direct3DDevice9Ex::CreateOffscreenPlainSurfaceEx(UINT,UINT,D3DFORMAT,D3DPOOL,IDirect3DSurface9 **,HANDLE *,DWORD)
	{
		CriticalSection cs(this);

		UNIMPLEMENTED();

		return D3D_OK;
	}

	HRESULT Direct3DDevice9Ex::CreateDepthStencilSurfaceEx(UINT,UINT,D3DFORMAT,D3DMULTISAMPLE_TYPE,DWORD,BOOL,IDirect3DSurface9 **,HANDLE *,DWORD)
	{
		CriticalSection cs(this);

		UNIMPLEMENTED();

		return D3D_OK;
	}

	HRESULT Direct3DDevice9Ex::ResetEx(D3DPRESENT_PARAMETERS *,D3DDISPLAYMODEEX *)
	{
		CriticalSection cs(this);

		UNIMPLEMENTED();

		return D3D_OK;
	}

	HRESULT Direct3DDevice9Ex::GetDisplayModeEx(UINT,D3DDISPLAYMODEEX *,D3DDISPLAYROTATION *)
	{
		CriticalSection cs(this);

		UNIMPLEMENTED();

		return D3D_OK;
	}
}
