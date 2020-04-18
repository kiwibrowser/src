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

template<typename Base = ID3D11DeviceChild>
struct GalliumD3D11DeviceChild : public GalliumPrivateDataComObject<Base, dual_refcnt_t>
{
	GalliumD3D11Screen* device; // must not be null


	// if this is called, the subclass constructor must set device itself
	GalliumD3D11DeviceChild()
	: device(0)
	{}

	GalliumD3D11DeviceChild(GalliumD3D11Screen* p_device)
	{
		// we store the reference count minus one in refcnt
		device = p_device;
		device->AddRef();
	}

	virtual ~GalliumD3D11DeviceChild()
	{
		if(device)
			device->Release();
	}

	/* The purpose of this is to avoid cyclic garbage, since this won't hold
	 * a pointer to the device if it is only held by a pipeline binding in the immediate context
	 *
	 * TODO: we could only manipulate the device refcnt when atomic_refcnt == 0 changes,
	 * but this requires more complex atomic ops
	 */
	inline ULONG add_ref()
	{
		return GalliumPrivateDataComObject<Base, dual_refcnt_t>::add_ref();
	}

	inline ULONG release()
	{
		return GalliumPrivateDataComObject<Base, dual_refcnt_t>::release();
	}

	virtual ULONG STDMETHODCALLTYPE AddRef()
	{
		return add_ref();
	}

	virtual ULONG STDMETHODCALLTYPE Release()
	{
		return release();
	}

	virtual void STDMETHODCALLTYPE GetDevice(
		ID3D11Device **out_device
	 )
	{
		device->AddRef();
		*out_device = device;
	}
};

template<typename Base = ID3D11DeviceChild, typename Object = void>
struct GalliumD3D11Object : public GalliumD3D11DeviceChild<Base>
{
	Object* object;
	GalliumD3D11Object(GalliumD3D11Screen* device, Object* object)
	: GalliumD3D11DeviceChild<Base>(device), object(object)
	{}

	virtual ~GalliumD3D11Object();
};

#define IMPLEMENT_OBJECT_DTOR(name, gallium) \
template<> \
GalliumD3D11Object<ID3D11##name, void>::~GalliumD3D11Object() \
{ \
	DX10_ONLY(device->Unbind##name(this)); \
	device->immediate_pipe->delete_##gallium##_state(device->immediate_pipe, object); \
}

#define IMPLEMENT_VIEW_DTOR(name, gallium) \
template<> \
GalliumD3D11Object<ID3D11##name, struct pipe_##gallium>::~GalliumD3D11Object() \
{ \
	DX10_ONLY(device->Unbind##name(this)); \
	pipe_##gallium##_reference(&object, 0); \
}

IMPLEMENT_OBJECT_DTOR(InputLayout, vertex_elements)
IMPLEMENT_OBJECT_DTOR(DepthStencilState, depth_stencil_alpha)
IMPLEMENT_OBJECT_DTOR(RasterizerState, rasterizer)
IMPLEMENT_OBJECT_DTOR(SamplerState, sampler)
IMPLEMENT_OBJECT_DTOR(BlendState, blend)
IMPLEMENT_OBJECT_DTOR(VertexShader, vs)
IMPLEMENT_OBJECT_DTOR(PixelShader, fs)
IMPLEMENT_OBJECT_DTOR(GeometryShader, gs)

IMPLEMENT_VIEW_DTOR(ShaderResourceView, sampler_view)
IMPLEMENT_VIEW_DTOR(RenderTargetView, surface)
IMPLEMENT_VIEW_DTOR(DepthStencilView, surface)

#if API >= 11
// IMPLEMENT_VIEW_DTOR(UnorderedAccessView, surface);
// IMPLEMENT_OBJECT_DTOR(HullShader, tcs);
// IMPLEMENT_OBJECT_DTOR(DomainShader, tes);
// IMPLEMENT_OBJECT_DTOR(ComputeShader, cs);
#else
IMPLEMENT_OBJECT_DTOR(BlendState1, blend)
IMPLEMENT_VIEW_DTOR(ShaderResourceView1, sampler_view)
#endif

template<typename Base, typename Desc, typename Object = void>
struct GalliumD3D11DescribedObject : public GalliumD3D11Object<Base, Object>
{
	Desc desc;
	GalliumD3D11DescribedObject(GalliumD3D11Screen* device, Object* object, const Desc& desc)
	: GalliumD3D11Object<Base, Object>(device, object), desc(desc)
	{}

	virtual void STDMETHODCALLTYPE GetDesc(Desc *out_desc)
	{
		memcpy(out_desc, &desc, sizeof(desc));
	}
};

typedef GalliumD3D11Object<ID3D11InputLayout> GalliumD3D11InputLayout;
typedef GalliumD3D11DescribedObject<ID3D11DepthStencilState, D3D11_DEPTH_STENCIL_DESC> GalliumD3D11DepthStencilState;
typedef GalliumD3D11DescribedObject<ID3D11RasterizerState, D3D11_RASTERIZER_DESC> GalliumD3D11RasterizerStateBase;
typedef GalliumD3D11DescribedObject<ID3D11SamplerState, D3D11_SAMPLER_DESC> GalliumD3D11SamplerState;

#if API >= 11
typedef GalliumD3D11DescribedObject<ID3D11BlendState, D3D11_BLEND_DESC> GalliumD3D11BlendState;
#else
typedef GalliumD3D10DescribedObject<ID3D10BlendState1, D3D10_BLEND_DESC> GalliumD3D10BlendStateBase;

struct GalliumD3D10BlendState : public GalliumD3D10BlendStateBase
{
	static D3D10_BLEND_DESC convert_to_d3d10(const D3D10_BLEND_DESC1& desc1)
	{
		D3D10_BLEND_DESC desc;
		desc.AlphaToCoverageEnable = desc1.AlphaToCoverageEnable;
		desc.SrcBlend = desc1.RenderTarget[0].SrcBlend;
		desc.DestBlend = desc1.RenderTarget[0].DestBlend;
		desc.BlendOp = desc1.RenderTarget[0].BlendOp;
		desc.SrcBlendAlpha = desc1.RenderTarget[0].SrcBlendAlpha;
		desc.DestBlendAlpha = desc1.RenderTarget[0].DestBlendAlpha;
		desc.BlendOpAlpha = desc1.RenderTarget[0].BlendOpAlpha;
		for(unsigned i = 0; i < 8; ++i)
		{
			desc.BlendEnable[i] = desc1.RenderTarget[i].BlendEnable;
			desc.RenderTargetWriteMask[i] = desc1.RenderTarget[i].RenderTargetWriteMask;
		}
		return desc;
	}

	D3D10_BLEND_DESC1 desc1;

	GalliumD3D10BlendState(GalliumD3D10Screen* device, void* object, const D3D10_BLEND_DESC& desc)
	: GalliumD3D10BlendStateBase(device, object, desc)
	{
		memset(&desc1, 0, sizeof(desc1));
		desc1.AlphaToCoverageEnable = desc.AlphaToCoverageEnable;
		desc1.RenderTarget[0].SrcBlend = desc.SrcBlend;
		desc1.RenderTarget[0].DestBlend = desc.DestBlend;
		desc1.RenderTarget[0].BlendOp = desc.BlendOp;
		desc1.RenderTarget[0].SrcBlendAlpha = desc.SrcBlendAlpha;
		desc1.RenderTarget[0].DestBlendAlpha = desc.DestBlendAlpha;
		desc1.RenderTarget[0].BlendOpAlpha = desc.BlendOpAlpha;
		for(unsigned i = 0; i < 8; ++i)
		{
			desc1.RenderTarget[i].BlendEnable = desc.BlendEnable[i];
			desc1.RenderTarget[i].RenderTargetWriteMask = desc.RenderTargetWriteMask[i];
		}
	}

	GalliumD3D10BlendState(GalliumD3D10Screen* device, void* object, const D3D10_BLEND_DESC1& desc)
	: GalliumD3D10BlendStateBase(device, object, convert_to_d3d10(desc)), desc1(desc1)
	{}

	virtual void STDMETHODCALLTYPE GetDesc1(D3D10_BLEND_DESC1 *out_desc)
	{
		memcpy(out_desc, &desc1, sizeof(desc1));
	}
};
#endif

struct GalliumD3D11RasterizerState : public GalliumD3D11RasterizerStateBase
{
	GalliumD3D11RasterizerState(GalliumD3D11Screen* device, void* object, const D3D11_RASTERIZER_DESC& desc)
	: GalliumD3D11RasterizerStateBase(device, object, desc)
	{}
};

template<typename Base = ID3D11DeviceChild>
struct GalliumD3D11Shader : public GalliumD3D11Object<Base>
{
	GalliumD3D11Shader(GalliumD3D11Screen* device, void* object)
	: GalliumD3D11Object<Base>(device, object)
	{}
};

typedef GalliumD3D11Shader<ID3D11VertexShader> GalliumD3D11VertexShader;
typedef GalliumD3D11Shader<ID3D11GeometryShader> GalliumD3D11GeometryShader;
typedef GalliumD3D11Shader<ID3D11PixelShader> GalliumD3D11PixelShader;

#if API >= 11
/*
typedef GalliumD3D11Shader<ID3D11HullShader> GalliumD3D11HullShader;
typedef GalliumD3D11Shader<ID3D11DomainShader> GalliumD3D11DomainShader;
typedef GalliumD3D11Shader<ID3D11ComputeShader> GalliumD3D11ComputeShader;
*/
#endif

template<typename Base = ID3D11Resource>
struct GalliumD3D11ResourceBase : public GalliumD3D11DeviceChild<Base>
{
	unsigned eviction_priority;

	virtual void STDMETHODCALLTYPE SetEvictionPriority(
		unsigned new_eviction_priority
	)
	{
		eviction_priority = new_eviction_priority;
	}

	virtual unsigned STDMETHODCALLTYPE GetEvictionPriority()
	{
		return eviction_priority;
	}
};

template<typename Real>
struct GalliumDXGIResource : public IDXGIResource
{
	virtual HRESULT STDMETHODCALLTYPE SetEvictionPriority(
		unsigned new_eviction_priority
	)
	{
		static_cast<Real*>(this)->eviction_priority = new_eviction_priority;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetEvictionPriority(unsigned* out_eviction_priority)
	{
	 	*out_eviction_priority = static_cast<Real*>(this)->eviction_priority;
	 	return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDevice(
		REFIID riid,
		void **out_parent)
	{
		if(!static_cast<Real*>(this)->device)
			return E_NOINTERFACE;
		return static_cast<Real*>(this)->device->QueryInterface(riid, out_parent);
	}

	virtual HRESULT STDMETHODCALLTYPE GetParent(
		REFIID riid,
		void **out_parent)
	{
		if(!static_cast<Real*>(this)->device)
			return E_NOINTERFACE;
		return static_cast<Real*>(this)->device->QueryInterface(riid, out_parent);
	}
};

template<typename T>
struct com_traits<GalliumDXGIResource<T> > : public com_traits<IDXGIResource>
{};

template<typename Base = ID3D11Resource>
struct GalliumD3D11Resource
	: public GalliumMultiComObject<
		GalliumMultiPrivateDataComObject<
			GalliumD3D11ResourceBase<Base>,
			GalliumDXGIResource<GalliumD3D11Resource<Base> >
		>,
		IGalliumResource
	>
{
	struct pipe_resource* resource;
	std::unordered_map<unsigned, pipe_transfer*> transfers;
	float min_lod;
	DXGI_USAGE dxgi_usage;

	GalliumD3D11Resource(GalliumD3D11Screen* device = 0, struct pipe_resource* resource = 0, unsigned dxgi_usage = 0)
	: resource(resource), min_lod(0), dxgi_usage(dxgi_usage)
	{
		this->device = device;
		if(device)
			device->AddRef();
		this->eviction_priority = 0;
	}

	~GalliumD3D11Resource()
	{
		pipe_resource_reference(&resource, 0);
	}

	virtual HRESULT STDMETHODCALLTYPE GetUsage(
		DXGI_USAGE *out_usage
	 )
	{
		*out_usage = this->dxgi_usage;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetSharedHandle(HANDLE *out_shared_handle)
	{
		return E_NOTIMPL;
	}

	virtual struct pipe_resource* STDMETHODCALLTYPE GetGalliumResource()
	{
		return resource;
	}
};

template<typename Base, typename Desc, D3D11_RESOURCE_DIMENSION Dim>
struct GalliumD3D11TypedResource : public GalliumD3D11Resource<Base>
{
	Desc desc;
	GalliumD3D11TypedResource() {}
	GalliumD3D11TypedResource(GalliumD3D11Screen* device, struct pipe_resource* resource, const Desc& desc, unsigned dxgi_usage)
	: GalliumD3D11Resource<Base>(device, resource, dxgi_usage), desc(desc)
	{}
	virtual void STDMETHODCALLTYPE GetType(
		D3D11_RESOURCE_DIMENSION *out_resource_dimension)
	{
		*out_resource_dimension = Dim;
	}
	virtual void STDMETHODCALLTYPE GetDesc(Desc *out_desc)
	{
		memcpy(out_desc, &desc, sizeof(desc));
	}
};

typedef GalliumD3D11TypedResource<ID3D11Texture1D, D3D11_TEXTURE1D_DESC, D3D11_RESOURCE_DIMENSION_TEXTURE1D> GalliumD3D11Texture1DBase;
typedef GalliumD3D11TypedResource<ID3D11Texture2D, D3D11_TEXTURE2D_DESC, D3D11_RESOURCE_DIMENSION_TEXTURE2D> GalliumD3D11Texture2DBase;
typedef GalliumD3D11TypedResource<ID3D11Texture3D, D3D11_TEXTURE3D_DESC, D3D11_RESOURCE_DIMENSION_TEXTURE3D> GalliumD3D11Texture3DBase;
typedef GalliumD3D11TypedResource<ID3D11Buffer, D3D11_BUFFER_DESC, D3D11_RESOURCE_DIMENSION_BUFFER> GalliumD3D11BufferBase;

#if API >= 11
typedef GalliumD3D11Texture1DBase GalliumD3D11Texture1D;
typedef GalliumD3D11Texture2DBase GalliumD3D11Texture2D;
typedef GalliumD3D11Texture3DBase GalliumD3D11Texture3D;

struct GalliumD3D11Buffer : public GalliumD3D11BufferBase
{
	struct pipe_stream_output_target* so_target;

	GalliumD3D11Buffer(GalliumD3D11Screen* device, struct pipe_resource* resource, const D3D11_BUFFER_DESC& desc, unsigned dxgi_usage)
	: GalliumD3D11BufferBase(device, resource, desc, dxgi_usage), so_target(0)
	{
	}

	~GalliumD3D11Buffer()
	{
		if(so_target)
			pipe_so_target_reference(&so_target, NULL);
	}
};
#else
struct GalliumD3D10Buffer : public GalliumD3D10BufferBase
{
	struct pipe_stream_output_target *so_target;

	GalliumD3D10Buffer(GalliumD3D10Screen* device, struct pipe_resource* resource, const D3D10_BUFFER_DESC& desc, unsigned dxgi_usage)
	: GalliumD3D10BufferBase(device, resource, desc, dxgi_usage)
	{
	}

	~GalliumD3D10Buffer()
	{
		if(so_target)
			pipe_so_target_reference(&so_target, NULL);

		device->UnbindBuffer(this);
	}

	virtual HRESULT STDMETHODCALLTYPE Map(
		D3D10_MAP map_type,
		unsigned map_flags,
		void **out_data)
	{
		D3D10_MAPPED_SUBRESOURCE msr;
		HRESULT hr = device->Map(this, 0, map_type, map_flags, &msr);
		if(!SUCCEEDED(hr))
			return hr;
		*out_data = msr.pData;
		return S_OK;
	}

	virtual void STDMETHODCALLTYPE Unmap()
	{
		device->Unmap(this, 0);
	}
};

struct GalliumD3D10Texture1D : public GalliumD3D10Texture1DBase
{
	GalliumD3D10Texture1D(GalliumD3D10Screen* device, struct pipe_resource* resource, const D3D10_TEXTURE1D_DESC& desc, unsigned dxgi_usage)
	: GalliumD3D10Texture1DBase(device, resource, desc, dxgi_usage)
	{}

	virtual HRESULT STDMETHODCALLTYPE Map(
		unsigned subresource,
		D3D10_MAP map_type,
		unsigned map_flags,
		void **out_data)
	{
		D3D10_MAPPED_SUBRESOURCE msr;
		HRESULT hr = device->Map(this, subresource, map_type, map_flags, &msr);
		if(!SUCCEEDED(hr))
			return hr;
		*out_data = msr.pData;
		return S_OK;
	}

	virtual void STDMETHODCALLTYPE Unmap(
		unsigned subresource
	)
	{
		device->Unmap(this, subresource);
	}
};

struct GalliumD3D10Texture2D : public GalliumD3D10Texture2DBase
{
	GalliumD3D10Texture2D() {}
	GalliumD3D10Texture2D(GalliumD3D10Screen* device, struct pipe_resource* resource, const D3D10_TEXTURE2D_DESC& desc, unsigned dxgi_usage)
	: GalliumD3D10Texture2DBase(device, resource, desc, dxgi_usage)
	{}

	virtual HRESULT STDMETHODCALLTYPE Map(
		unsigned subresource,
		D3D10_MAP map_type,
		unsigned map_flags,
		D3D10_MAPPED_TEXTURE2D *out_mapped_subresource)
	{
		D3D10_MAPPED_SUBRESOURCE msr;
		HRESULT hr = device->Map(this, subresource, map_type, map_flags, &msr);
		if(!SUCCEEDED(hr))
			return hr;
		out_mapped_subresource->pData = msr.pData;
		out_mapped_subresource->RowPitch = msr.RowPitch;
		return S_OK;
	}

	virtual void STDMETHODCALLTYPE Unmap(
		unsigned subresource
	)
	{
		device->Unmap(this, subresource);
	}
};


struct GalliumD3D10Texture3D : public GalliumD3D10Texture3DBase
{
	GalliumD3D10Texture3D(GalliumD3D10Screen* device, struct pipe_resource* resource, const D3D10_TEXTURE3D_DESC& desc, unsigned dxgi_usage)
	: GalliumD3D10Texture3DBase(device, resource, desc, dxgi_usage)
	{}

	virtual HRESULT STDMETHODCALLTYPE Map(
		unsigned subresource,
		D3D10_MAP map_type,
		unsigned map_flags,
		D3D10_MAPPED_TEXTURE3D *out_mapped_subresource)
	{
		D3D10_MAPPED_SUBRESOURCE msr;
		HRESULT hr = device->Map(this, subresource, map_type, map_flags, &msr);
		if(!SUCCEEDED(hr))
			return hr;
		out_mapped_subresource->pData = msr.pData;
		out_mapped_subresource->RowPitch = msr.RowPitch;
		out_mapped_subresource->DepthPitch = msr.DepthPitch;
		return S_OK;
	}

	virtual void STDMETHODCALLTYPE Unmap(
		unsigned subresource
	)
	{
		device->Unmap(this, subresource);
	}
};
#endif

struct GalliumD3D11Surface : public GalliumMultiPrivateDataComObject<GalliumD3D11Texture2D, IDXGISurface1>
{
	GalliumD3D11Surface(GalliumD3D11Screen* device, struct pipe_resource* resource, const D3D11_TEXTURE2D_DESC& desc, unsigned dxgi_usage)
	{
		this->device = device;
		this->device->AddRef();
		this->resource = resource;
		this->desc = desc;
		this->dxgi_usage = dxgi_usage;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDesc(
		DXGI_SURFACE_DESC *out_desc)
	{
		out_desc->Format = this->desc.Format;
		out_desc->Width = this->desc.Width;
		out_desc->Height = this->desc.Height;
		out_desc->SampleDesc = this->desc.SampleDesc;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetParent(
		REFIID riid,
		void **out_parent)
	{
		if(!device)
			return E_NOINTERFACE;
		return device->QueryInterface(riid, out_parent);
	}

	/* TODO: somehow implement these */
	virtual HRESULT STDMETHODCALLTYPE GetDC(
		BOOL discard,
		HDC *out_hdc)
	{
		*out_hdc = 0;
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE ReleaseDC(
		RECT *out_dirty_rect)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Map(
		DXGI_MAPPED_RECT *out_locked_rect,
		unsigned map_flags)
	{
		D3D11_MAP d3d_map;
		if(map_flags & DXGI_MAP_DISCARD)
			d3d_map = D3D11_MAP_WRITE_DISCARD;
		else
		{
			if(map_flags & DXGI_MAP_READ)
			{
				if(map_flags & DXGI_MAP_WRITE)
					d3d_map = D3D11_MAP_READ_WRITE;
				else
					d3d_map = D3D11_MAP_READ;
			}
			else
				d3d_map = D3D11_MAP_WRITE;
		}
		D3D11_MAPPED_SUBRESOURCE d3d_mapped;
		HRESULT hres = this->device->get_immediate_context()->Map(this, 0, d3d_map, 0, &d3d_mapped);
		out_locked_rect->pBits = (uint8_t*)d3d_mapped.pData;
		out_locked_rect->Pitch = d3d_mapped.RowPitch;
		return hres;
	}

	virtual HRESULT STDMETHODCALLTYPE Unmap(void)
	{
		this->device->get_immediate_context()->Unmap(this, 0);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDevice(
		REFIID riid,
		void **out_parent)
	{
		if(!device)
			return E_NOINTERFACE;
		return device->QueryInterface(riid, out_parent);
	}
};

template<typename Base, typename Desc, typename Object>
struct GalliumD3D11View : public GalliumD3D11DescribedObject<Base, Desc, Object>
{
	GalliumD3D11Resource<>* resource;
	GalliumD3D11View(GalliumD3D11Screen* device, GalliumD3D11Resource<>* resource, Object* object, const Desc& desc)
	: GalliumD3D11DescribedObject<Base, Desc, Object>(device, object, desc), resource(resource)
	{
		resource->AddRef();
	}

	~GalliumD3D11View()
	{
		resource->Release();
	}

	virtual void STDMETHODCALLTYPE GetResource(ID3D11Resource** out_resource)
	{
		resource->AddRef();
		*out_resource = resource;
	}
};

typedef GalliumD3D11View<ID3D11DepthStencilView, D3D11_DEPTH_STENCIL_VIEW_DESC, struct pipe_surface> GalliumD3D11DepthStencilView;
typedef GalliumD3D11View<ID3D11RenderTargetView, D3D11_RENDER_TARGET_VIEW_DESC, struct pipe_surface> GalliumD3D11RenderTargetView;

#if API >= 11
typedef GalliumD3D11View<ID3D11ShaderResourceView, D3D11_SHADER_RESOURCE_VIEW_DESC, struct pipe_sampler_view> GalliumD3D11ShaderResourceView;
#else
typedef GalliumD3D10View<ID3D10ShaderResourceView1, D3D10_SHADER_RESOURCE_VIEW_DESC1, struct pipe_sampler_view> GalliumD3D10ShaderResourceViewBase;

struct GalliumD3D10ShaderResourceView : public GalliumD3D10ShaderResourceViewBase
{
	GalliumD3D10ShaderResourceView(GalliumD3D10Screen* device, GalliumD3D10Resource<>* resource, struct pipe_sampler_view* view, const D3D10_SHADER_RESOURCE_VIEW_DESC1& desc)
	: GalliumD3D10ShaderResourceViewBase(device, resource, view, desc)
	{}

	virtual void STDMETHODCALLTYPE GetDesc1(D3D10_SHADER_RESOURCE_VIEW_DESC1 *out_desc)
	{
		memcpy(out_desc, &desc, sizeof(*out_desc));
	}

	virtual void STDMETHODCALLTYPE GetDesc(D3D10_SHADER_RESOURCE_VIEW_DESC *out_desc)
	{
		memcpy(out_desc, &desc, sizeof(*out_desc));
	}
};
#endif

template<typename Base = ID3D11Asynchronous>
struct GalliumD3D11Asynchronous : public GalliumD3D11DeviceChild<Base>
{
	struct pipe_query* query;
	unsigned data_size;

	GalliumD3D11Asynchronous(GalliumD3D11Screen* device, struct pipe_query* query, unsigned data_size)
	: GalliumD3D11DeviceChild<Base>(device), query(query), data_size(data_size)
	{}

	~GalliumD3D11Asynchronous()
	{
		this->device->immediate_pipe->destroy_query(this->device->immediate_pipe, query);
	}

	virtual unsigned STDMETHODCALLTYPE GetDataSize()
	{
		return data_size;
	}

#if API < 11
	virtual void STDMETHODCALLTYPE Begin()
	{
		this->device->Begin(this);
	}

	virtual void STDMETHODCALLTYPE End()
	{
		this->device->End(this);
	}

	virtual HRESULT STDMETHODCALLTYPE GetData(
		void * out_data,
		unsigned data_size,
		unsigned get_data_flags)
	{
		return this->device->GetData(this, out_data, data_size, get_data_flags);
	}
#endif
};

template<typename Base = ID3D11Asynchronous>
struct GalliumD3D11QueryOrPredicate : public GalliumD3D11Asynchronous<Base>
{
	D3D11_QUERY_DESC desc;
	GalliumD3D11QueryOrPredicate(GalliumD3D11Screen* device, struct pipe_query* query, unsigned data_size, const D3D11_QUERY_DESC& desc)
	: GalliumD3D11Asynchronous<Base>(device, query, data_size), desc(desc)
	{}

	virtual void STDMETHODCALLTYPE GetDesc(
		D3D11_QUERY_DESC *out_desc)
	{
		*out_desc = desc;
	}
};

struct GalliumD3D11Query : public GalliumD3D11QueryOrPredicate<ID3D11Query>
{
	GalliumD3D11Query(GalliumD3D11Screen* device, struct pipe_query* query, unsigned data_size, const D3D11_QUERY_DESC& desc)
	: GalliumD3D11QueryOrPredicate<ID3D11Query>(device, query, data_size, desc)
	{}
};

struct GalliumD3D11Predicate : public GalliumD3D11QueryOrPredicate<ID3D11Predicate>
{
	GalliumD3D11Predicate(GalliumD3D11Screen* device, struct pipe_query* query, unsigned data_size, const D3D11_QUERY_DESC& desc)
	: GalliumD3D11QueryOrPredicate<ID3D11Predicate>(device, query, data_size, desc)
	{}

	~GalliumD3D11Predicate()
	{
		DX10_ONLY(device->UnbindPredicate(this));
	}
};

struct GalliumD3D11Counter : public GalliumD3D11Asynchronous<ID3D11Counter>
{
	D3D11_COUNTER_DESC desc;
	GalliumD3D11Counter(GalliumD3D11Screen* device, struct pipe_query* query, unsigned data_size, const D3D11_COUNTER_DESC& desc)
	: GalliumD3D11Asynchronous<ID3D11Counter>(device, query, data_size), desc(desc)
	{}

	virtual void STDMETHODCALLTYPE GetDesc(
		D3D11_COUNTER_DESC *out_desc)
	{
		*out_desc = desc;
	}
};
