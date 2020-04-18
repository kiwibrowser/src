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

#ifndef D3D9_Direct3DDevice9Ex_hpp
#define D3D9_Direct3DDevice9Ex_hpp

#include "Direct3DDevice9.hpp"

#include "Direct3D9Ex.hpp"
#include "Direct3DSwapChain9.hpp"

#include "Stream.hpp"

#include <d3d9.h>
#include <map>
#include <list>
#include <vector>

namespace sw
{
	class Renderer;
	class Context;
}

namespace D3D9
{
	class Direct3DVertexDeclaration9;
	class Direct3DStateBlock9;
	class Direct3DSurface9;
	class Direct3DPixelShader9;
	class Direct3DVertexShader9;
	class irect3DVertexDeclaration9;
	class Direct3DVertexBuffer9;
	class Direct3DIndexBuffer9;

	class Direct3DDevice9Ex : public IDirect3DDevice9Ex, public Direct3DDevice9
	{
	public:
		Direct3DDevice9Ex(const HINSTANCE instance, Direct3D9Ex *d3d9ex, unsigned int adapter, D3DDEVTYPE deviceType, HWND focusWindow, unsigned long behaviourFlags, D3DPRESENT_PARAMETERS *presentParameters);

		~Direct3DDevice9Ex() override;

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object) override;
		unsigned long __stdcall AddRef() override;
		unsigned long __stdcall Release() override;

		// IDirect3DDevice9 methods
		long __stdcall TestCooperativeLevel() override;
		unsigned int __stdcall GetAvailableTextureMem() override;
		long __stdcall EvictManagedResources() override;
		long __stdcall GetDirect3D(IDirect3D9 **D3D) override;
		long __stdcall GetDeviceCaps(D3DCAPS9 *caps) override;
		long __stdcall GetDisplayMode(unsigned int swapChain ,D3DDISPLAYMODE *mode) override;
		long __stdcall GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *parameters) override;
		long __stdcall SetCursorProperties(unsigned int x, unsigned int y, IDirect3DSurface9 *cursorBitmap) override;
		void __stdcall SetCursorPosition(int x, int y, unsigned long flags) override;
		int __stdcall ShowCursor(int show) override;
		long __stdcall CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS *presentParameters, IDirect3DSwapChain9 **swapChain) override;
		long __stdcall GetSwapChain(unsigned int index, IDirect3DSwapChain9 **swapChain) override;
		unsigned int __stdcall GetNumberOfSwapChains() override;
		long __stdcall Reset(D3DPRESENT_PARAMETERS *presentParameters) override;
		long __stdcall Present(const RECT *sourceRect, const RECT *destRect, HWND destWindowOverride, const RGNDATA *dirtyRegion) override;
		long __stdcall GetBackBuffer(unsigned int swapChain, unsigned int index, D3DBACKBUFFER_TYPE type, IDirect3DSurface9 **backBuffer) override;
		long __stdcall GetRasterStatus(unsigned int swapChain, D3DRASTER_STATUS *rasterStatus) override;
		long __stdcall SetDialogBoxMode(int enableDialogs) override;
		void __stdcall SetGammaRamp(unsigned int swapChain, unsigned long flags, const D3DGAMMARAMP *ramp) override;
		void __stdcall GetGammaRamp(unsigned int swapChain, D3DGAMMARAMP *ramp) override;
		long __stdcall CreateTexture(unsigned int width, unsigned int height, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture9 **texture, void **sharedHandle) override;
		long __stdcall CreateVolumeTexture(unsigned int width, unsigned int height, unsigned int depth, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DVolumeTexture9 **volumeTexture, void **sharedHandle) override;
		long __stdcall CreateCubeTexture(unsigned int edgeLength, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DCubeTexture9 **cubeTexture, void **sharedHandle) override;
		long __stdcall CreateVertexBuffer(unsigned int length, unsigned long usage, unsigned long FVF, D3DPOOL, IDirect3DVertexBuffer9 **vertexBuffer, void **sharedHandle) override;
		long __stdcall CreateIndexBuffer(unsigned int length, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DIndexBuffer9 **indexBuffer, void **sharedHandle) override;
		long __stdcall CreateRenderTarget(unsigned int width, unsigned int height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multiSample, unsigned long multiSampleQuality, int lockable, IDirect3DSurface9 **surface, void **sharedHandle) override;
		long __stdcall CreateDepthStencilSurface(unsigned int width, unsigned int height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multiSample, unsigned long multiSampleQuality, int discard, IDirect3DSurface9 **surface, void **sharedHandle) override;
		long __stdcall UpdateSurface(IDirect3DSurface9 *sourceSurface, const RECT *sourceRect, IDirect3DSurface9 *destinationSurface, const POINT *destPoint) override;
		long __stdcall UpdateTexture(IDirect3DBaseTexture9 *sourceTexture, IDirect3DBaseTexture9 *destinationTexture) override;
		long __stdcall GetRenderTargetData(IDirect3DSurface9 *renderTarget, IDirect3DSurface9 *destSurface) override;
		long __stdcall GetFrontBufferData(unsigned int swapChain, IDirect3DSurface9 *destSurface) override;
		long __stdcall StretchRect(IDirect3DSurface9 *sourceSurface, const RECT *sourceRect, IDirect3DSurface9 *destSurface, const RECT *destRect, D3DTEXTUREFILTERTYPE filter) override;
		long __stdcall ColorFill(IDirect3DSurface9 *surface, const RECT *rect, D3DCOLOR color) override;
		long __stdcall CreateOffscreenPlainSurface(unsigned int width, unsigned int height, D3DFORMAT format, D3DPOOL pool, IDirect3DSurface9 **surface, void **sharedHandle) override;
		long __stdcall SetRenderTarget(unsigned long index, IDirect3DSurface9 *renderTarget) override;
		long __stdcall GetRenderTarget(unsigned long index, IDirect3DSurface9 **renderTarget) override;
		long __stdcall SetDepthStencilSurface(IDirect3DSurface9 *newDepthStencil) override;
		long __stdcall GetDepthStencilSurface(IDirect3DSurface9 **depthStencilSurface) override;
		long __stdcall BeginScene() override;
		long __stdcall EndScene() override;
		long __stdcall Clear(unsigned long Count, const D3DRECT *rects, unsigned long Flags, unsigned long Color, float Z, unsigned long Stencil) override;
		long __stdcall SetTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix) override;
		long __stdcall GetTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix) override;
		long __stdcall MultiplyTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix) override;
		long __stdcall SetViewport(const D3DVIEWPORT9 *viewport) override;
		long __stdcall GetViewport(D3DVIEWPORT9 *viewport) override;
		long __stdcall SetMaterial(const D3DMATERIAL9 *material) override;
		long __stdcall GetMaterial(D3DMATERIAL9 *material) override;
		long __stdcall SetLight(unsigned long index, const D3DLIGHT9 *light) override;
		long __stdcall GetLight(unsigned long index, D3DLIGHT9 *light) override;
		long __stdcall LightEnable(unsigned long index, int enable) override;
		long __stdcall GetLightEnable(unsigned long index , int *enable) override;
		long __stdcall SetClipPlane(unsigned long index, const float *plane) override;
		long __stdcall GetClipPlane(unsigned long index, float *plane) override;
		long __stdcall SetRenderState(D3DRENDERSTATETYPE state, unsigned long value) override;
		long __stdcall GetRenderState(D3DRENDERSTATETYPE State, unsigned long *value) override;
		long __stdcall CreateStateBlock(D3DSTATEBLOCKTYPE type, IDirect3DStateBlock9 **stateBlock) override;
		long __stdcall BeginStateBlock() override;
		long __stdcall EndStateBlock(IDirect3DStateBlock9 **stateBlock) override;
		long __stdcall SetClipStatus(const D3DCLIPSTATUS9 *clipStatus) override;
		long __stdcall GetClipStatus(D3DCLIPSTATUS9 *clipStatus) override;
		long __stdcall GetTexture(unsigned long sampler, IDirect3DBaseTexture9 **texture) override;
		long __stdcall SetTexture(unsigned long sampler, IDirect3DBaseTexture9 *texture) override;
		long __stdcall GetTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type, unsigned long *value) override;
		long __stdcall SetTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type, unsigned long value) override;
		long __stdcall GetSamplerState(unsigned long sampler, D3DSAMPLERSTATETYPE state, unsigned long *value) override;
		long __stdcall SetSamplerState(unsigned long sampler, D3DSAMPLERSTATETYPE state, unsigned long value) override;
		long __stdcall ValidateDevice(unsigned long *numPasses) override;
		long __stdcall SetPaletteEntries(unsigned int paletteNumber, const PALETTEENTRY *entries) override;
		long __stdcall GetPaletteEntries(unsigned int paletteNumber, PALETTEENTRY *entries) override;
		long __stdcall SetCurrentTexturePalette(unsigned int paletteNumber) override;
		long __stdcall GetCurrentTexturePalette(unsigned int *paletteNumber) override;
		long __stdcall SetScissorRect(const RECT *rect) override;
		long __stdcall GetScissorRect(RECT *rect) override;
		long __stdcall SetSoftwareVertexProcessing(int software) override;
		int __stdcall GetSoftwareVertexProcessing() override;
		long __stdcall SetNPatchMode(float segments) override;
		float __stdcall GetNPatchMode() override;
		long __stdcall DrawPrimitive(D3DPRIMITIVETYPE primitiveType, unsigned int startVertex, unsigned int primiveCount) override;
		long __stdcall DrawIndexedPrimitive(D3DPRIMITIVETYPE type, int baseVertexIndex, unsigned int minIndex, unsigned int numVertices, unsigned int startIndex, unsigned int primitiveCount) override;
		long __stdcall DrawPrimitiveUP(D3DPRIMITIVETYPE primitiveType, unsigned int primitiveCount, const void *vertexStreamZeroData, unsigned int vertexStreamZeroStride) override;
		long __stdcall DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE type, unsigned int minVertexIndex, unsigned int numVertexIndices, unsigned int PrimitiveCount, const void *indexData, D3DFORMAT indexDataFormat, const void *vertexStreamZeroData, unsigned int VertexStreamZeroStride) override;
		long __stdcall ProcessVertices(unsigned int srcStartIndex, unsigned int destIndex, unsigned int vertexCount, IDirect3DVertexBuffer9 *destBuffer, IDirect3DVertexDeclaration9 *vertexDeclaration, unsigned long flags) override;
		long __stdcall CreateVertexDeclaration(const D3DVERTEXELEMENT9 *vertexElements, IDirect3DVertexDeclaration9 **declaration) override;
		long __stdcall SetVertexDeclaration(IDirect3DVertexDeclaration9 *declaration) override;
		long __stdcall GetVertexDeclaration(IDirect3DVertexDeclaration9 **declaration) override;
		long __stdcall SetFVF(unsigned long FVF) override;
		long __stdcall GetFVF(unsigned long *FVF) override;
		long __stdcall CreateVertexShader(const unsigned long *function, IDirect3DVertexShader9 **shader) override;
		long __stdcall SetVertexShader(IDirect3DVertexShader9 *shader) override;
		long __stdcall GetVertexShader(IDirect3DVertexShader9 **shader) override;
		long __stdcall SetVertexShaderConstantF(unsigned int startRegister, const float *constantData, unsigned int count) override;
		long __stdcall GetVertexShaderConstantF(unsigned int startRegister, float *constantData, unsigned int count) override;
		long __stdcall SetVertexShaderConstantI(unsigned int startRegister, const int *constantData, unsigned int count) override;
		long __stdcall GetVertexShaderConstantI(unsigned int startRegister, int *constantData, unsigned int count) override;
		long __stdcall SetVertexShaderConstantB(unsigned int startRegister, const int *constantData, unsigned int count) override;
		long __stdcall GetVertexShaderConstantB(unsigned int startRegister, int *constantData, unsigned int count) override;
		long __stdcall SetStreamSource(unsigned int stream, IDirect3DVertexBuffer9 *data, unsigned int offset, unsigned int stride) override;
		long __stdcall GetStreamSource(unsigned int streamNumber, IDirect3DVertexBuffer9 **streamData, unsigned int *offset, unsigned int *stride) override;
		long __stdcall SetStreamSourceFreq(unsigned int streamNumber, unsigned int divider) override;
		long __stdcall GetStreamSourceFreq(unsigned int streamNumber, unsigned int *divider) override;
		long __stdcall SetIndices(IDirect3DIndexBuffer9 *indexData) override;
		long __stdcall GetIndices(IDirect3DIndexBuffer9 **indexData) override;
		long __stdcall CreatePixelShader(const unsigned long *function, IDirect3DPixelShader9 **shader) override;
		long __stdcall SetPixelShader(IDirect3DPixelShader9 *shader) override;
		long __stdcall GetPixelShader(IDirect3DPixelShader9 **shader) override;
		long __stdcall SetPixelShaderConstantI(unsigned int startRegister, const int *constantData, unsigned int count) override;
		long __stdcall GetPixelShaderConstantI(unsigned int startRegister, int *constantData, unsigned int count) override;
		long __stdcall SetPixelShaderConstantF(unsigned int startRegister, const float *constantData, unsigned int count) override;
		long __stdcall GetPixelShaderConstantF(unsigned int startRegister, float *constantData, unsigned int count) override;
		long __stdcall SetPixelShaderConstantB(unsigned int startRegister, const int *constantData, unsigned int count) override;
		long __stdcall GetPixelShaderConstantB(unsigned int startRegister, int *constantData, unsigned int count) override;
		long __stdcall DrawRectPatch(unsigned int handle, const float *numSegs, const D3DRECTPATCH_INFO *rectPatchInfo) override;
		long __stdcall DrawTriPatch(unsigned int handle, const float *numSegs, const D3DTRIPATCH_INFO *triPatchInfo) override;
		long __stdcall DeletePatch(unsigned int handle) override;
		long __stdcall CreateQuery(D3DQUERYTYPE type, IDirect3DQuery9 **query) override;

		// IDirect3DDevice9Ex methods
		long __stdcall SetConvolutionMonoKernel(UINT,UINT,float *,float *) override;
		long __stdcall ComposeRects(IDirect3DSurface9 *,IDirect3DSurface9 *,IDirect3DVertexBuffer9 *,UINT,IDirect3DVertexBuffer9 *,D3DCOMPOSERECTSOP,int,int) override;
		long __stdcall PresentEx(const RECT *,const RECT *,HWND,const RGNDATA *,DWORD) override;
		long __stdcall GetGPUThreadPriority(int *priority) override;
		long __stdcall SetGPUThreadPriority(int priority) override;
		long __stdcall WaitForVBlank(unsigned int swapChain) override;
		long __stdcall CheckResourceResidency(IDirect3DResource9 **resourceArray, unsigned int numResources) override;
		long __stdcall SetMaximumFrameLatency(unsigned int maxLatency) override;
		long __stdcall GetMaximumFrameLatency(unsigned int *maxLatency) override;
		long __stdcall CheckDeviceState(HWND destinationWindow) override;
		long __stdcall CreateRenderTargetEx(unsigned int width, unsigned int height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multiSampleType, unsigned long multiSampleQuality, int lockable, IDirect3DSurface9 **surface, void **sharedHandle, unsigned long usage) override;
		long __stdcall CreateOffscreenPlainSurfaceEx(unsigned int width, unsigned int height, D3DFORMAT format, D3DPOOL pool, IDirect3DSurface9 **surface, void **sharedHandle, unsigned long usage) override;
		long __stdcall CreateDepthStencilSurfaceEx(unsigned int width, unsigned int height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multiSampleType, unsigned long multiSampleQuality, int discard, IDirect3DSurface9 **surface, void **sharedHandle, unsigned long usage) override;
		long __stdcall ResetEx(D3DPRESENT_PARAMETERS *presentParameters, D3DDISPLAYMODEEX *fullscreenDisplayMode) override;
		long __stdcall GetDisplayModeEx(unsigned int swapChain, D3DDISPLAYMODEEX *mode, D3DDISPLAYROTATION *rotation) override;

	private:
		Direct3D9Ex *const d3d9ex;
	};
}

#endif // D3D9_Direct3DDevice9Ex_hpp
