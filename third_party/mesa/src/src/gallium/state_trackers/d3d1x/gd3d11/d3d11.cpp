/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "d3d1x_private.h"

extern "C"
{
#include "util/u_gen_mipmap.h"
#include "tgsi/tgsi_ureg.h"
#include "tgsi/tgsi_dump.h"
#include "cso_cache/cso_context.h"
}


// the perl script will change this to 10 for d3d10, and also do s/D3D11/D3D10 in the whole file
#define API 11

#if API >= 11
#define DX10_ONLY(x)
#else
#define DX10_ONLY(x) x
#endif

typedef D3D10_MAPPED_TEXTURE3D D3D10_MAPPED_SUBRESOURCE;

// used to make QueryInterface know the IIDs of the interface and its ancestors
COM_INTERFACE(ID3D11DeviceChild, IUnknown)
COM_INTERFACE(ID3D11InputLayout, ID3D11DeviceChild)
COM_INTERFACE(ID3D11DepthStencilState, ID3D11DeviceChild)
COM_INTERFACE(ID3D11BlendState, ID3D11DeviceChild)
COM_INTERFACE(ID3D11RasterizerState, ID3D11DeviceChild)
COM_INTERFACE(ID3D11SamplerState, ID3D11DeviceChild)
COM_INTERFACE(ID3D11Resource, ID3D11DeviceChild)
COM_INTERFACE(ID3D11Buffer, ID3D11Resource)
COM_INTERFACE(ID3D11Texture1D, ID3D11Resource)
COM_INTERFACE(ID3D11Texture2D, ID3D11Resource)
COM_INTERFACE(ID3D11Texture3D, ID3D11Resource)
COM_INTERFACE(ID3D11View, ID3D11DeviceChild)
COM_INTERFACE(ID3D11ShaderResourceView, ID3D11View)
COM_INTERFACE(ID3D11RenderTargetView, ID3D11View)
COM_INTERFACE(ID3D11DepthStencilView, ID3D11View)
COM_INTERFACE(ID3D11VertexShader, ID3D11DeviceChild)
COM_INTERFACE(ID3D11GeometryShader, ID3D11DeviceChild)
COM_INTERFACE(ID3D11PixelShader, ID3D11DeviceChild)
COM_INTERFACE(ID3D11Asynchronous, ID3D11DeviceChild)
COM_INTERFACE(ID3D11Query, ID3D11Asynchronous)
COM_INTERFACE(ID3D11Predicate, ID3D11Query)
COM_INTERFACE(ID3D11Counter, ID3D11Asynchronous)
COM_INTERFACE(ID3D11Device, IUnknown)

#if API >= 11
COM_INTERFACE(ID3D11UnorderedAccessView, ID3D11View)
COM_INTERFACE(ID3D11HullShader, ID3D11DeviceChild)
COM_INTERFACE(ID3D11DomainShader, ID3D11DeviceChild)
COM_INTERFACE(ID3D11ComputeShader, ID3D11DeviceChild)
COM_INTERFACE(ID3D11ClassInstance, ID3D11DeviceChild)
COM_INTERFACE(ID3D11ClassLinkage, ID3D11DeviceChild)
COM_INTERFACE(ID3D11CommandList, ID3D11DeviceChild)
COM_INTERFACE(ID3D11DeviceContext, ID3D11DeviceChild)
#else
COM_INTERFACE(ID3D10BlendState1, ID3D10BlendState)
COM_INTERFACE(ID3D10ShaderResourceView1, ID3D10ShaderResourceView)
COM_INTERFACE(ID3D10Device1, ID3D10Device)
#endif

struct GalliumD3D11Screen;

#if API >= 11
static ID3D11DeviceContext* GalliumD3D11ImmediateDeviceContext_Create(GalliumD3D11Screen* device, struct pipe_context* pipe, bool owns_pipe);
static void GalliumD3D11ImmediateDeviceContext_RestoreGalliumState(ID3D11DeviceContext* context);
static void GalliumD3D11ImmediateDeviceContext_RestoreGalliumStateBlitOnly(ID3D11DeviceContext* context);
static void GalliumD3D11ImmediateDeviceContext_Destroy(ID3D11DeviceContext* device);
#endif

static inline pipe_box d3d11_to_pipe_box(struct pipe_resource* resource, unsigned level, const D3D11_BOX* pBox)
{
	pipe_box box;
	if(pBox)
	{
		box.x = pBox->left;
		box.y = pBox->top;
		box.z = pBox->front;
		box.width = pBox->right - pBox->left;
		box.height = pBox->bottom - pBox->top;
		box.depth = pBox->back - pBox->front;
	}
	else
	{
		box.x = box.y = box.z = 0;
		box.width = u_minify(resource->width0, level);
		box.height = u_minify(resource->height0, level);
		box.depth = u_minify(resource->depth0, level);
	}
	return box;
}

struct GalliumD3D11Caps
{
	bool so;
	bool gs;
	bool queries;
	bool render_condition;
	unsigned constant_buffers[D3D11_STAGES];
	unsigned stages;
	unsigned stages_with_sampling;
};

typedef GalliumDXGIDevice<
	GalliumMultiComObject<
#if API >= 11
		GalliumPrivateDataComObject<ID3D11Device>,
#else
		GalliumPrivateDataComObject<ID3D10Device1>,
#endif
		IGalliumDevice
	>
> GalliumD3D11ScreenBase;

// used to avoid needing to have forward declarations of functions
// this is called "screen" because in the D3D10 case it's only part of the device
struct GalliumD3D11Screen : public GalliumD3D11ScreenBase
{

	pipe_screen* screen;
	pipe_context* immediate_pipe;
	GalliumD3D11Caps screen_caps;

#if API >= 11
	ID3D11DeviceContext* immediate_context;
	ID3D11DeviceContext* get_immediate_context()
	{
		return immediate_context;
	}
#else
	GalliumD3D11Screen* get_immediate_context()
	{
		return this;
	}
#endif


	GalliumD3D11Screen(pipe_screen* screen, struct pipe_context* immediate_pipe, IDXGIAdapter* adapter)
	: GalliumD3D11ScreenBase(adapter), screen(screen), immediate_pipe(immediate_pipe)
	{
	}

#if API < 11
	// we use a D3D11-like API internally
	virtual HRESULT STDMETHODCALLTYPE Map(
			ID3D11Resource *pResource,
			unsigned Subresource,
			D3D11_MAP MapType,
			unsigned MapFlags,
			D3D11_MAPPED_SUBRESOURCE *pMappedResource) = 0;
	virtual void STDMETHODCALLTYPE Unmap(
			ID3D11Resource *pResource,
			unsigned Subresource) = 0;
	virtual void STDMETHODCALLTYPE Begin(
		ID3D11Asynchronous *pAsync) = 0;
	virtual void STDMETHODCALLTYPE End(
		ID3D11Asynchronous *pAsync) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetData(
		ID3D11Asynchronous *pAsync,
		void *pData,
		unsigned DataSize,
		unsigned GetDataFlags) = 0;

	// TODO: maybe we should use function overloading, but that might risk silent errors,
	// and cannot be exported to a C interface
	virtual void UnbindBlendState(ID3D11BlendState* state) = 0;
	virtual void UnbindRasterizerState(ID3D11RasterizerState* state) = 0;
	virtual void UnbindDepthStencilState(ID3D11DepthStencilState* state) = 0;
	virtual void UnbindInputLayout(ID3D11InputLayout* state) = 0;
	virtual void UnbindPixelShader(ID3D11PixelShader* state) = 0;
	virtual void UnbindVertexShader(ID3D11VertexShader* state) = 0;
	virtual void UnbindGeometryShader(ID3D11GeometryShader* state) = 0;
	virtual void UnbindPredicate(ID3D11Predicate* predicate) = 0;
	virtual void UnbindSamplerState(ID3D11SamplerState* state) = 0;
	virtual void UnbindBuffer(ID3D11Buffer* buffer) = 0;
	virtual void UnbindDepthStencilView(ID3D11DepthStencilView* view) = 0;
	virtual void UnbindRenderTargetView(ID3D11RenderTargetView* view) = 0;
	virtual void UnbindShaderResourceView(ID3D11ShaderResourceView* view) = 0;

	void UnbindBlendState1(ID3D11BlendState1* state)
	{
		UnbindBlendState(state);
	}
	void UnbindShaderResourceView1(ID3D11ShaderResourceView1* view)
	{
		UnbindShaderResourceView(view);
	}
#endif
};

#include "d3d11_objects.h"
#include "d3d11_screen.h"
#include "d3d11_context.h"
#include "d3d11_misc.h"

#if API >= 11
HRESULT STDMETHODCALLTYPE GalliumD3D11DeviceCreate(struct pipe_screen* screen, struct pipe_context* context, BOOL owns_context, unsigned creation_flags, IDXGIAdapter* adapter, ID3D11Device** ppDevice)
{
	if(creation_flags & D3D11_CREATE_DEVICE_SINGLETHREADED)
		*ppDevice = new GalliumD3D11ScreenImpl<false>(screen, context, owns_context, creation_flags, adapter);
	else
		*ppDevice = new GalliumD3D11ScreenImpl<true>(screen, context, owns_context, creation_flags, adapter);
	return S_OK;
}
#else
HRESULT STDMETHODCALLTYPE GalliumD3D10DeviceCreate1(struct pipe_screen* screen, struct pipe_context* context, BOOL owns_context, unsigned creation_flags, IDXGIAdapter* adapter, ID3D10Device1** ppDevice)
{
	if(creation_flags & D3D10_CREATE_DEVICE_SINGLETHREADED)
		*ppDevice = new GalliumD3D10Device<false>(screen, context, owns_context, creation_flags, adapter);
	else
		*ppDevice = new GalliumD3D10Device<true>(screen, context, owns_context, creation_flags, adapter);
	return S_OK;
}
#endif
