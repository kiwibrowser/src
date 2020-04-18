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

/* used to unbind things, we need 128 due to resources */
static const void* zero_data[128];

#define UPDATE_VIEWS_SHIFT (D3D11_STAGES * 0)
#define UPDATE_SAMPLERS_SHIFT (D3D11_STAGES * 1)
#define UPDATE_VERTEX_BUFFERS (1 << (D3D11_STAGES * 2))

#if API >= 11
template<typename PtrTraits>
struct GalliumD3D11DeviceContext :
	public GalliumD3D11DeviceChild<ID3D11DeviceContext>
{
#else
template<bool threadsafe>
struct GalliumD3D10Device : public GalliumD3D10ScreenImpl<threadsafe>
{
	typedef simple_ptr_traits PtrTraits;
	typedef GalliumD3D10Device GalliumD3D10DeviceContext;
#endif

	refcnt_ptr<GalliumD3D11Shader<>, PtrTraits> shaders[D3D11_STAGES];
	refcnt_ptr<GalliumD3D11InputLayout, PtrTraits> input_layout;
	refcnt_ptr<GalliumD3D11Buffer, PtrTraits> index_buffer;
	refcnt_ptr<GalliumD3D11RasterizerState, PtrTraits> rasterizer_state;
	refcnt_ptr<GalliumD3D11DepthStencilState, PtrTraits> depth_stencil_state;
	refcnt_ptr<GalliumD3D11BlendState, PtrTraits> blend_state;
	refcnt_ptr<GalliumD3D11DepthStencilView, PtrTraits> depth_stencil_view;
	refcnt_ptr<GalliumD3D11Predicate, PtrTraits> render_predicate;

	refcnt_ptr<GalliumD3D11Buffer, PtrTraits> constant_buffers[D3D11_STAGES][D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	refcnt_ptr<GalliumD3D11ShaderResourceView, PtrTraits> shader_resource_views[D3D11_STAGES][D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	refcnt_ptr<GalliumD3D11SamplerState, PtrTraits> samplers[D3D11_STAGES][D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	refcnt_ptr<GalliumD3D11Buffer, PtrTraits> input_buffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
	refcnt_ptr<GalliumD3D11RenderTargetView, PtrTraits> render_target_views[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
	refcnt_ptr<GalliumD3D11Buffer, PtrTraits> so_buffers[D3D11_SO_BUFFER_SLOT_COUNT];

#if API >= 11
	refcnt_ptr<ID3D11UnorderedAccessView, PtrTraits> cs_unordered_access_views[D3D11_PS_CS_UAV_REGISTER_COUNT];
	refcnt_ptr<ID3D11UnorderedAccessView, PtrTraits> om_unordered_access_views[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
#endif

	D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	D3D11_RECT scissor_rects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	D3D11_PRIMITIVE_TOPOLOGY primitive_topology;
	DXGI_FORMAT index_format;
	unsigned index_offset;
	uint32_t strip_cut_index;
	BOOL render_predicate_value;
	float blend_color[4];
	unsigned sample_mask;
	unsigned stencil_ref;

	void* default_input_layout;
	void* default_rasterizer;
	void* default_depth_stencil;
	void* default_blend;
	void* default_sampler;
	void* default_shaders[D3D11_STAGES];

	// derived state
	int primitive_mode;
	struct pipe_vertex_buffer vertex_buffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
	struct pipe_stream_output_target* so_targets[D3D11_SO_BUFFER_SLOT_COUNT];
	struct pipe_sampler_view* sampler_views[D3D11_STAGES][D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	void* sampler_csos[D3D11_STAGES][D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	unsigned num_shader_resource_views[D3D11_STAGES];
	unsigned num_samplers[D3D11_STAGES];
	unsigned num_vertex_buffers;
	unsigned num_render_target_views;
	unsigned num_viewports;
	unsigned num_scissor_rects;
	unsigned num_so_targets;

	struct pipe_context* pipe;
	unsigned update_flags;

	bool owns_pipe;
	unsigned context_flags;

	GalliumD3D11Caps caps;

	cso_context* cso_ctx;
	gen_mipmap_state* gen_mipmap;

#if API >= 11
#define SYNCHRONIZED do {} while(0)

	GalliumD3D11DeviceContext(GalliumD3D11Screen* device, pipe_context* pipe, bool owns_pipe, unsigned context_flags = 0)
	: GalliumD3D11DeviceChild<ID3D11DeviceContext>(device), pipe(pipe), owns_pipe(owns_pipe), context_flags(context_flags)
	{
		caps = device->screen_caps;
		init_context();
	}

	~GalliumD3D11DeviceContext()
	{
		destroy_context();
	}
#else
#define SYNCHRONIZED lock_t<maybe_mutex_t<threadsafe> > lock_(this->mutex)

	GalliumD3D10Device(pipe_screen* screen, pipe_context* pipe, bool owns_pipe, unsigned creation_flags, IDXGIAdapter* adapter)
	: GalliumD3D10ScreenImpl<threadsafe>(screen, pipe, owns_pipe, creation_flags, adapter), pipe(pipe), owns_pipe(owns_pipe), context_flags(0)
	{
		caps = this->screen_caps;
		init_context();
	}

	~GalliumD3D10Device()
	{
		destroy_context();
	}
#endif

	void init_context()
	{
		if(!pipe->begin_query)
			caps.queries = false;
		if(!pipe->bind_gs_state)
		{
			caps.gs = false;
			caps.stages = 2;
		}
		assert(!caps.so || pipe->set_stream_output_targets);
		if(!pipe->set_geometry_sampler_views)
			caps.stages_with_sampling &=~ (1 << PIPE_SHADER_GEOMETRY);
		if(!pipe->set_fragment_sampler_views)
			caps.stages_with_sampling &=~ (1 << PIPE_SHADER_FRAGMENT);
		if(!pipe->set_vertex_sampler_views)
			caps.stages_with_sampling &=~ (1 << PIPE_SHADER_VERTEX);

		update_flags = 0;

		// pipeline state
		memset(viewports, 0, sizeof(viewports));
		memset(scissor_rects, 0, sizeof(scissor_rects));
		primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
		index_format = DXGI_FORMAT_UNKNOWN;
		index_offset = 0;
		strip_cut_index = 0xffffffff;
		render_predicate_value = 0;
		memset(blend_color, 0, sizeof(blend_color));
		sample_mask = ~0;
		stencil_ref = 0;

		// derived state
		primitive_mode = 0;
		memset(vertex_buffers, 0, sizeof(vertex_buffers));
		memset(so_targets, 0, sizeof(so_buffers));
		memset(sampler_views, 0, sizeof(sampler_views));
		memset(sampler_csos, 0, sizeof(sampler_csos));
		memset(num_shader_resource_views, 0, sizeof(num_shader_resource_views));
		memset(num_samplers, 0, sizeof(num_samplers));
		num_vertex_buffers = 0;
		num_render_target_views = 0;
		num_viewports = 0;
		num_scissor_rects = 0;
		num_so_targets = 0;

		default_input_layout = pipe->create_vertex_elements_state(pipe, 0, 0);

		struct pipe_rasterizer_state rasterizerd;
		memset(&rasterizerd, 0, sizeof(rasterizerd));
		rasterizerd.gl_rasterization_rules = 1;
		rasterizerd.cull_face = PIPE_FACE_BACK;
		rasterizerd.flatshade_first = 1;
		rasterizerd.line_width = 1.0f;
		rasterizerd.point_size = 1.0f;
		rasterizerd.depth_clip = TRUE;
		default_rasterizer = pipe->create_rasterizer_state(pipe, &rasterizerd);

		struct pipe_depth_stencil_alpha_state depth_stencild;
		memset(&depth_stencild, 0, sizeof(depth_stencild));
		depth_stencild.depth.enabled = TRUE;
		depth_stencild.depth.writemask = 1;
		depth_stencild.depth.func = PIPE_FUNC_LESS;
		default_depth_stencil = pipe->create_depth_stencil_alpha_state(pipe, &depth_stencild);

		struct pipe_blend_state blendd;
		memset(&blendd, 0, sizeof(blendd));
		blendd.rt[0].colormask = 0xf;
		default_blend = pipe->create_blend_state(pipe, &blendd);

		struct pipe_sampler_state samplerd;
		memset(&samplerd, 0, sizeof(samplerd));
		samplerd.normalized_coords = 1;
		samplerd.min_img_filter = PIPE_TEX_FILTER_LINEAR;
		samplerd.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
		samplerd.min_mip_filter = PIPE_TEX_MIPFILTER_LINEAR;
		samplerd.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
		samplerd.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
		samplerd.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
		samplerd.border_color.f[0] = 1.0f;
		samplerd.border_color.f[1] = 1.0f;
		samplerd.border_color.f[2] = 1.0f;
		samplerd.border_color.f[3] = 1.0f;
		samplerd.min_lod = -FLT_MAX;
		samplerd.max_lod = FLT_MAX;
		samplerd.max_anisotropy = 1;
		default_sampler = pipe->create_sampler_state(pipe, &samplerd);

		memset(&samplerd, 0, sizeof(samplerd));
		samplerd.normalized_coords = 0;
		samplerd.min_img_filter = PIPE_TEX_FILTER_NEAREST;
		samplerd.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
		samplerd.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
		samplerd.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
		samplerd.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
		samplerd.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
		samplerd.min_lod = -FLT_MAX;
		samplerd.max_lod = FLT_MAX;
		samplerd.max_anisotropy = 1;

		for(unsigned s = 0; s < D3D11_STAGES; ++s)
			for(unsigned i = 0; i < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
				sampler_csos[s][i] = default_sampler;

		// TODO: should this really be empty shaders, or should they be all-passthrough?
		memset(default_shaders, 0, sizeof(default_shaders));
		struct ureg_program *ureg;
		ureg = ureg_create(TGSI_PROCESSOR_FRAGMENT);
		ureg_END(ureg);
		default_shaders[PIPE_SHADER_FRAGMENT] = ureg_create_shader_and_destroy(ureg, pipe);

		ureg = ureg_create(TGSI_PROCESSOR_VERTEX);
		ureg_END(ureg);
		default_shaders[PIPE_SHADER_VERTEX] = ureg_create_shader_and_destroy(ureg, pipe);

		cso_ctx = cso_create_context(pipe);
		gen_mipmap = util_create_gen_mipmap(pipe, cso_ctx);

		RestoreGalliumState();
	}

	void destroy_context()
	{
		util_destroy_gen_mipmap(gen_mipmap);
		cso_destroy_context(cso_ctx);

		pipe->bind_vertex_elements_state(pipe, 0);
		pipe->delete_vertex_elements_state(pipe, default_input_layout);

		pipe->bind_rasterizer_state(pipe, 0);
		pipe->delete_rasterizer_state(pipe, default_rasterizer);

		pipe->bind_depth_stencil_alpha_state(pipe, 0);
		pipe->delete_depth_stencil_alpha_state(pipe, default_depth_stencil);

		pipe->bind_blend_state(pipe, 0);
		pipe->delete_blend_state(pipe, default_blend);

		pipe->bind_fragment_sampler_states(pipe, 0, 0);
		pipe->bind_vertex_sampler_states(pipe, 0, 0);
		if(pipe->bind_geometry_sampler_states)
			pipe->bind_geometry_sampler_states(pipe, 0, 0);
		pipe->delete_sampler_state(pipe, default_sampler);

		pipe->bind_fs_state(pipe, 0);
		pipe->delete_fs_state(pipe, default_shaders[PIPE_SHADER_FRAGMENT]);

		pipe->bind_vs_state(pipe, 0);
		pipe->delete_vs_state(pipe, default_shaders[PIPE_SHADER_VERTEX]);

		if(owns_pipe)
			pipe->destroy(pipe);
	}

	virtual unsigned STDMETHODCALLTYPE GetContextFlags(void)
	{
		return context_flags;
	}
#if API >= 11
#define SET_SHADER_EXTRA_ARGS , \
	ID3D11ClassInstance *const *ppClassInstances, \
	unsigned count
#define GET_SHADER_EXTRA_ARGS , \
		ID3D11ClassInstance **ppClassInstances, \
		unsigned *out_count
#else
#define SET_SHADER_EXTRA_ARGS
#define GET_SHADER_EXTRA_ARGS
#endif

/* On Windows D3D11, SetConstantBuffers and SetShaderResources crash if passed a null pointer.
 * Instead, you have to pass a pointer to nulls to unbind things.
 * We do the same.
 * TODO: is D3D10 the same?
 */
	template<unsigned s>
	void xs_set_shader(GalliumD3D11Shader<>* shader)
	{
		if(shader != shaders[s].p)
		{
			shaders[s] = shader;
			void* shader_cso = shader ? shader->object : default_shaders[s];
			switch(s)
			{
			case PIPE_SHADER_VERTEX:
				pipe->bind_vs_state(pipe, shader_cso);
				break;
			case PIPE_SHADER_FRAGMENT:
				pipe->bind_fs_state(pipe, shader_cso);
				break;
			case PIPE_SHADER_GEOMETRY:
				pipe->bind_gs_state(pipe, shader_cso);
				break;
			}
			update_flags |= (1 << (UPDATE_SAMPLERS_SHIFT + s)) | (1 << (UPDATE_VIEWS_SHIFT + s));
		}
	}

	template<unsigned s>
	void xs_set_constant_buffers(unsigned start, unsigned count, GalliumD3D11Buffer *const *constbufs)
	{
		for(unsigned i = 0; i < count; ++i)
		{
			if(constbufs[i] != constant_buffers[s][start + i].p)
			{
				constant_buffers[s][start + i] = constbufs[i];
				if(s < caps.stages && start + i < caps.constant_buffers[s])
					pipe_set_constant_buffer(pipe, s, start + i, constbufs[i] ? constbufs[i]->resource : NULL);
			}
		}
	}

	template<unsigned s>
	void xs_set_shader_resources(unsigned start, unsigned count, GalliumD3D11ShaderResourceView *const *srvs)
	{
		int last_different = -1;
		for(unsigned i = 0; i < count; ++i)
		{
			if(shader_resource_views[s][start + i].p != srvs[i])
			{
				shader_resource_views[s][start + i] = srvs[i];
				sampler_views[s][start + i] = srvs[i] ? srvs[i]->object : 0;
				last_different = i;
			}
		}
		if(last_different >= 0)
		{
			num_shader_resource_views[s] = std::max(num_shader_resource_views[s], start + last_different + 1);
			update_flags |= 1 << (UPDATE_VIEWS_SHIFT + s);
		}
	}

	template<unsigned s>
	void xs_set_samplers(unsigned start, unsigned count, GalliumD3D11SamplerState *const *samps)
	{
		int last_different = -1;
		for(unsigned i = 0; i < count; ++i)
		{
			if(samplers[s][start + i].p != samps[i])
			{
				samplers[s][start + i] = samps[i];
				sampler_csos[s][start + i] = samps[i] ? samps[i]->object : default_sampler;
				last_different = i;
			}
		}
		if(last_different >= 0)
		{
			num_samplers[s] = std::max(num_samplers[s], start + last_different + 1);
			update_flags |= 1 << (UPDATE_SAMPLERS_SHIFT + s);
		}
	}

#define IMPLEMENT_SHADER_STAGE(XS, Stage) \
	virtual void STDMETHODCALLTYPE XS##SetShader( \
		ID3D11##Stage##Shader *pShader \
		SET_SHADER_EXTRA_ARGS) \
	{ \
		SYNCHRONIZED; \
		xs_set_shader<D3D11_STAGE_##XS>((GalliumD3D11Shader<>*)pShader); \
	} \
	virtual void STDMETHODCALLTYPE XS##GetShader(\
		ID3D11##Stage##Shader **ppShader \
		GET_SHADER_EXTRA_ARGS) \
	{ \
		SYNCHRONIZED; \
		*ppShader = (ID3D11##Stage##Shader*)shaders[D3D11_STAGE_##XS].ref(); \
	} \
	virtual void STDMETHODCALLTYPE XS##SetConstantBuffers(\
		unsigned start, \
		unsigned count, \
		ID3D11Buffer *const* constant_buffers) \
	{ \
		SYNCHRONIZED; \
		xs_set_constant_buffers<D3D11_STAGE_##XS>(start, count, (GalliumD3D11Buffer *const *)constant_buffers); \
	} \
	virtual void STDMETHODCALLTYPE XS##GetConstantBuffers(\
		unsigned start, \
		unsigned count, \
		ID3D11Buffer **out_constant_buffers) \
	{ \
		SYNCHRONIZED; \
		for(unsigned i = 0; i < count; ++i) \
			out_constant_buffers[i] = constant_buffers[D3D11_STAGE_##XS][start + i].ref(); \
	} \
	virtual void STDMETHODCALLTYPE XS##SetShaderResources(\
		unsigned start, \
		unsigned count, \
		ID3D11ShaderResourceView *const *new_shader_resource_views) \
	{ \
		SYNCHRONIZED; \
		xs_set_shader_resources<D3D11_STAGE_##XS>(start, count, (GalliumD3D11ShaderResourceView *const *)new_shader_resource_views); \
	} \
	virtual void STDMETHODCALLTYPE XS##GetShaderResources(\
		unsigned start, \
		unsigned count, \
		ID3D11ShaderResourceView **out_shader_resource_views) \
	{ \
		SYNCHRONIZED; \
		for(unsigned i = 0; i < count; ++i) \
			out_shader_resource_views[i] = shader_resource_views[D3D11_STAGE_##XS][start + i].ref(); \
	} \
	virtual void STDMETHODCALLTYPE XS##SetSamplers(\
		unsigned start, \
		unsigned count, \
		ID3D11SamplerState *const *new_samplers) \
	{ \
		SYNCHRONIZED; \
		xs_set_samplers<D3D11_STAGE_##XS>(start, count, (GalliumD3D11SamplerState *const *)new_samplers); \
	} \
	virtual void STDMETHODCALLTYPE XS##GetSamplers( \
		unsigned start, \
		unsigned count, \
		ID3D11SamplerState **out_samplers) \
	{ \
		SYNCHRONIZED; \
		for(unsigned i = 0; i < count; ++i) \
			out_samplers[i] = samplers[D3D11_STAGE_##XS][start + i].ref(); \
	}

#define DO_VS(x) x
#define DO_GS(x) do {if(caps.gs) {x;}} while(0)
#define DO_PS(x) x
#define DO_HS(x)
#define DO_DS(x)
#define DO_CS(x)
	IMPLEMENT_SHADER_STAGE(VS, Vertex)
	IMPLEMENT_SHADER_STAGE(GS, Geometry)
	IMPLEMENT_SHADER_STAGE(PS, Pixel)

#if API >= 11
	IMPLEMENT_SHADER_STAGE(HS, Hull)
	IMPLEMENT_SHADER_STAGE(DS, Domain)
	IMPLEMENT_SHADER_STAGE(CS, Compute)

	virtual void STDMETHODCALLTYPE CSSetUnorderedAccessViews(
		unsigned start,
		unsigned count,
		ID3D11UnorderedAccessView *const *new_unordered_access_views,
		const unsigned *new_uav_initial_counts)
	{
		SYNCHRONIZED;
		for(unsigned i = 0; i < count; ++i)
			cs_unordered_access_views[start + i] = new_unordered_access_views[i];
	}

	virtual void STDMETHODCALLTYPE CSGetUnorderedAccessViews(
		unsigned start,
		unsigned count,
		ID3D11UnorderedAccessView **out_unordered_access_views)
	{
		SYNCHRONIZED;
		for(unsigned i = 0; i < count; ++i)
			out_unordered_access_views[i] = cs_unordered_access_views[start + i].ref();
	}
#endif

	template<unsigned s>
	void update_stage()
	{
		if(update_flags & (1 << (UPDATE_VIEWS_SHIFT + s)))
		{
			while(num_shader_resource_views[s] && !sampler_views[s][num_shader_resource_views[s] - 1]) \
				--num_shader_resource_views[s];
			if((1 << s) & caps.stages_with_sampling)
			{
				const unsigned num_views_to_bind = num_shader_resource_views[s];
				switch(s)
				{
				case PIPE_SHADER_VERTEX:
					pipe->set_vertex_sampler_views(pipe, num_views_to_bind, sampler_views[s]);
					break;
				case PIPE_SHADER_FRAGMENT:
					pipe->set_fragment_sampler_views(pipe, num_views_to_bind, sampler_views[s]);
					break;
				case PIPE_SHADER_GEOMETRY:
					pipe->set_geometry_sampler_views(pipe, num_views_to_bind, sampler_views[s]);
					break;
				}
			}
		}

		if(update_flags & (1 << (UPDATE_SAMPLERS_SHIFT + s)))
		{
			while(num_samplers[s] && !sampler_csos[s][num_samplers[s] - 1])
				--num_samplers[s];
			if((1 << s) & caps.stages_with_sampling)
			{
				const unsigned num_samplers_to_bind = num_samplers[s];
				switch(s)
				{
				case PIPE_SHADER_VERTEX:
					pipe->bind_vertex_sampler_states(pipe, num_samplers_to_bind, sampler_csos[s]);
					break;
				case PIPE_SHADER_FRAGMENT:
					pipe->bind_fragment_sampler_states(pipe, num_samplers_to_bind, sampler_csos[s]);
					break;
				case PIPE_SHADER_GEOMETRY:
					pipe->bind_geometry_sampler_states(pipe, num_samplers_to_bind, sampler_csos[s]);
					break;
				}
			}
		}
	}

	void update_state()
	{
		update_stage<D3D11_STAGE_PS>();
		update_stage<D3D11_STAGE_VS>();
		update_stage<D3D11_STAGE_GS>();
#if API >= 11
		update_stage<D3D11_STAGE_HS>();
		update_stage<D3D11_STAGE_DS>();
		update_stage<D3D11_STAGE_CS>();
#endif

		if(update_flags & UPDATE_VERTEX_BUFFERS)
		{
			while(num_vertex_buffers && !vertex_buffers[num_vertex_buffers - 1].buffer)
				--num_vertex_buffers;
			pipe->set_vertex_buffers(pipe, num_vertex_buffers, vertex_buffers);
		}

		update_flags = 0;
	}

	virtual void STDMETHODCALLTYPE IASetInputLayout(
		ID3D11InputLayout *new_input_layout)
	{
		SYNCHRONIZED;
		if(new_input_layout != input_layout.p)
		{
			input_layout = new_input_layout;
			pipe->bind_vertex_elements_state(pipe, new_input_layout ? ((GalliumD3D11InputLayout*)new_input_layout)->object : default_input_layout);
		}
	}

	virtual void STDMETHODCALLTYPE IAGetInputLayout(
		ID3D11InputLayout **out_input_layout)
	{
		SYNCHRONIZED;
		*out_input_layout = input_layout.ref();
	}

	virtual void STDMETHODCALLTYPE IASetVertexBuffers(
		unsigned start,
		unsigned count,
		ID3D11Buffer *const *new_vertex_buffers,
		const unsigned *new_strides,
		const unsigned *new_offsets)
	{
		SYNCHRONIZED;
		int last_different = -1;
		for(unsigned i = 0; i < count; ++i)
		{
			ID3D11Buffer* buffer = new_vertex_buffers[i];
			if(buffer != input_buffers[start + i].p
				|| vertex_buffers[start + i].buffer_offset != new_offsets[i]
				|| vertex_buffers[start + i].stride != new_strides[i]
			)
			{
				input_buffers[start + i] = buffer;
				vertex_buffers[start + i].buffer = buffer ? ((GalliumD3D11Buffer*)buffer)->resource : 0;
				vertex_buffers[start + i].buffer_offset = new_offsets[i];
				vertex_buffers[start + i].stride = new_strides[i];
				last_different = i;
			}
		}
		if(last_different >= 0)
		{
			num_vertex_buffers = std::max(num_vertex_buffers, start + count);
			update_flags |= UPDATE_VERTEX_BUFFERS;
		}
	}

	virtual void STDMETHODCALLTYPE IAGetVertexBuffers(
		unsigned start,
		unsigned count,
		ID3D11Buffer **out_vertex_buffers,
		unsigned *out_strides,
		unsigned *out_offsets)
	{
		SYNCHRONIZED;
		if(out_vertex_buffers)
		{
			for(unsigned i = 0; i < count; ++i)
				out_vertex_buffers[i] = input_buffers[start + i].ref();
		}

		if(out_offsets)
		{
			for(unsigned i = 0; i < count; ++i)
				out_offsets[i] = vertex_buffers[start + i].buffer_offset;
		}

		if(out_strides)
		{
			for(unsigned i = 0; i < count; ++i)
				out_strides[i] = vertex_buffers[start + i].stride;
		}
	}

	void set_index_buffer()
	{
		pipe_index_buffer ib;
		if(!index_buffer)
		{
			memset(&ib, 0, sizeof(ib));
		}
		else
		{
			switch(index_format) {
			case DXGI_FORMAT_R32_UINT:
				ib.index_size = 4;
				strip_cut_index = 0xffffffff;
				break;
			case DXGI_FORMAT_R16_UINT:
				ib.index_size = 2;
				strip_cut_index = 0xffff;
				break;
			default:
				ib.index_size = 1;
				strip_cut_index = 0xff;
				break;
			}
			ib.offset = index_offset;
			ib.buffer = index_buffer ? ((GalliumD3D11Buffer*)index_buffer.p)->resource : 0;
		}
		pipe->set_index_buffer(pipe, &ib);
	}

	virtual void STDMETHODCALLTYPE IASetIndexBuffer(
		ID3D11Buffer *new_index_buffer,
		DXGI_FORMAT new_index_format,
		unsigned new_index_offset)
	{
		SYNCHRONIZED;
		if(index_buffer.p != new_index_buffer || index_format != new_index_format || index_offset != new_index_offset)
		{
			index_buffer = new_index_buffer;
			index_format = new_index_format;
			index_offset = new_index_offset;

			set_index_buffer();
		}
	}

	virtual void STDMETHODCALLTYPE IAGetIndexBuffer(
		ID3D11Buffer **out_index_buffer,
		DXGI_FORMAT *out_index_format,
		unsigned *out_index_offset)
	{
		SYNCHRONIZED;
		if(out_index_buffer)
			*out_index_buffer = index_buffer.ref();
		if(out_index_format)
			*out_index_format = index_format;
		if(out_index_offset)
			*out_index_offset = index_offset;
	}

	virtual void STDMETHODCALLTYPE IASetPrimitiveTopology(
		D3D11_PRIMITIVE_TOPOLOGY new_primitive_topology)
	{
		SYNCHRONIZED;
		if(primitive_topology != new_primitive_topology)
		{
			if(new_primitive_topology < D3D_PRIMITIVE_TOPOLOGY_COUNT)
				primitive_mode = d3d_to_pipe_prim[new_primitive_topology];
			else
				primitive_mode = 0;
			primitive_topology = new_primitive_topology;
		}
	}

	virtual void STDMETHODCALLTYPE IAGetPrimitiveTopology(
		D3D11_PRIMITIVE_TOPOLOGY *out_primitive_topology)
	{
		SYNCHRONIZED;
		*out_primitive_topology = primitive_topology;
	}

	virtual void STDMETHODCALLTYPE DrawIndexed(
		unsigned index_count,
		unsigned start_index_location,
		int base_vertex_location)
	{
		SYNCHRONIZED;
		if(update_flags)
			update_state();

		pipe_draw_info info;
		info.mode = primitive_mode;
		info.indexed = TRUE;
		info.count = index_count;
		info.start = start_index_location;
		info.index_bias = base_vertex_location;
		info.min_index = 0;
		info.max_index = ~0;
		info.start_instance = 0;
		info.instance_count = 1;
		info.primitive_restart = TRUE;
		info.restart_index = strip_cut_index;
		info.count_from_stream_output = NULL;

		pipe->draw_vbo(pipe, &info);
	}

	virtual void STDMETHODCALLTYPE Draw(
		unsigned vertex_count,
		unsigned start_vertex_location)
	{
		SYNCHRONIZED;
		if(update_flags)
			update_state();

		pipe_draw_info info;
		info.mode = primitive_mode;
		info.indexed = FALSE;
		info.count = vertex_count;
		info.start = start_vertex_location;
		info.index_bias = 0;
		info.min_index = 0;
		info.max_index = ~0;
		info.start_instance = 0;
		info.instance_count = 1;
		info.primitive_restart = FALSE;
		info.count_from_stream_output = NULL;

		pipe->draw_vbo(pipe, &info);
	}

	virtual void STDMETHODCALLTYPE DrawIndexedInstanced(
		unsigned index_countPerInstance,
		unsigned instance_count,
		unsigned start_index_location,
		int base_vertex_location,
		unsigned start_instance_location)
	{
		SYNCHRONIZED;
		if(update_flags)
			update_state();

		pipe_draw_info info;
		info.mode = primitive_mode;
		info.indexed = TRUE;
		info.count = index_countPerInstance;
		info.start = start_index_location;
		info.index_bias = base_vertex_location;
		info.min_index = 0;
		info.max_index = ~0;
		info.start_instance = start_instance_location;
		info.instance_count = instance_count;
		info.primitive_restart = TRUE;
		info.restart_index = strip_cut_index;
		info.count_from_stream_output = NULL;

		pipe->draw_vbo(pipe, &info);
	}

	virtual void STDMETHODCALLTYPE DrawInstanced(
		unsigned vertex_countPerInstance,
		unsigned instance_count,
		unsigned start_vertex_location,
		unsigned start_instance_location)
	{
		SYNCHRONIZED;
		if(update_flags)
			update_state();

		pipe_draw_info info;
		info.mode = primitive_mode;
		info.indexed = FALSE;
		info.count = vertex_countPerInstance;
		info.start = start_vertex_location;
		info.index_bias = 0;
		info.min_index = 0;
		info.max_index = ~0;
		info.start_instance = start_instance_location;
		info.instance_count = instance_count;
		info.primitive_restart = FALSE;
		info.count_from_stream_output = NULL;

		pipe->draw_vbo(pipe, &info);
	}

	virtual void STDMETHODCALLTYPE DrawAuto(void)
	{
		if(!caps.so)
			return;

		SYNCHRONIZED;
		if(update_flags)
			update_state();

		pipe_draw_info info;
		info.mode = primitive_mode;
		info.indexed = FALSE;
		info.count = 0;
		info.start = 0;
		info.index_bias = 0;
		info.min_index = 0;
		info.max_index = ~0;
		info.start_instance = 0;
		info.instance_count = 1;
		info.primitive_restart = FALSE;
		info.restart_index = 0;
		info.count_from_stream_output = input_buffers[0].p->so_target;

		pipe->draw_vbo(pipe, &info);
	}

	virtual void STDMETHODCALLTYPE DrawIndexedInstancedIndirect(
		ID3D11Buffer *buffer,
		unsigned aligned_byte_offset)
	{
		SYNCHRONIZED;
		if(update_flags)
			update_state();

		struct {
			unsigned count;
			unsigned instance_count;
			unsigned start;
			unsigned index_bias;
		} data;

		pipe_buffer_read(pipe, ((GalliumD3D11Buffer*)buffer)->resource, aligned_byte_offset, sizeof(data), &data);

		pipe_draw_info info;
		info.mode = primitive_mode;
		info.indexed = TRUE;
		info.start = data.start;
		info.count = data.count;
		info.index_bias = data.index_bias;
		info.min_index = 0;
		info.max_index = ~0;
		info.start_instance = 0;
		info.instance_count = data.instance_count;
		info.primitive_restart = TRUE;
		info.restart_index = strip_cut_index;
		info.count_from_stream_output = NULL;

		pipe->draw_vbo(pipe, &info);
	}

	virtual void STDMETHODCALLTYPE DrawInstancedIndirect(
		ID3D11Buffer *buffer,
		unsigned aligned_byte_offset)
	{
		SYNCHRONIZED;
		if(update_flags)
			update_state();

		struct {
			unsigned count;
			unsigned instance_count;
			unsigned start;
		} data;

		pipe_buffer_read(pipe, ((GalliumD3D11Buffer*)buffer)->resource, aligned_byte_offset, sizeof(data), &data);

		pipe_draw_info info;
		info.mode = primitive_mode;
		info.indexed = FALSE;
		info.start = data.start;
		info.count = data.count;
		info.index_bias = 0;
		info.min_index = 0;
		info.max_index = ~0;
		info.start_instance = 0;
		info.instance_count = data.instance_count;
		info.primitive_restart = FALSE;
		info.count_from_stream_output = NULL;

		pipe->draw_vbo(pipe, &info);
	}

#if API >= 11
	virtual void STDMETHODCALLTYPE Dispatch(
		unsigned thread_group_count_x,
		unsigned thread_group_count_y,
		unsigned thread_group_count_z)
	{
// uncomment this when this is implemented
//		SYNCHRONIZED;
//		if(update_flags)
//			update_state();
	}

	virtual void STDMETHODCALLTYPE DispatchIndirect(
		ID3D11Buffer *buffer,
		unsigned aligned_byte_offset)
	{
// uncomment this when this is implemented
//		SYNCHRONIZED;
//		if(update_flags)
//			update_state();
	}
#endif

	virtual void STDMETHODCALLTYPE RSSetState(
		ID3D11RasterizerState *new_rasterizer_state)
	{
		SYNCHRONIZED;
		if(new_rasterizer_state != rasterizer_state.p)
		{
			rasterizer_state = new_rasterizer_state;
			pipe->bind_rasterizer_state(pipe, new_rasterizer_state ? ((GalliumD3D11RasterizerState*)new_rasterizer_state)->object : default_rasterizer);
		}
	}

	virtual void STDMETHODCALLTYPE RSGetState(
		ID3D11RasterizerState **out_rasterizer_state)
	{
		SYNCHRONIZED;
		*out_rasterizer_state = rasterizer_state.ref();
	}

	void set_viewport()
	{
		// TODO: is depth correct? it seems D3D10/11 uses a [-1,1]x[-1,1]x[0,1] cube
		pipe_viewport_state viewport;
		float half_width = viewports[0].Width * 0.5f;
		float half_height = viewports[0].Height * 0.5f;

		viewport.scale[0] = half_width;
		viewport.scale[1] = -half_height;
		viewport.scale[2] = (viewports[0].MaxDepth - viewports[0].MinDepth);
		viewport.scale[3] = 1.0f;
		viewport.translate[0] = half_width + viewports[0].TopLeftX;
		viewport.translate[1] = half_height + viewports[0].TopLeftY;
		viewport.translate[2] = viewports[0].MinDepth;
		viewport.translate[3] = 1.0f;
		pipe->set_viewport_state(pipe, &viewport);
	}

	virtual void STDMETHODCALLTYPE RSSetViewports(
		unsigned count,
		const D3D11_VIEWPORT *new_viewports)
	{
		SYNCHRONIZED;
		if(count)
		{
			if(memcmp(&viewports[0], &new_viewports[0], sizeof(viewports[0])))
			{
				viewports[0] = new_viewports[0];
				set_viewport();
			}
			for(unsigned i = 1; i < count; ++i)
				viewports[i] = new_viewports[i];
		}
		else if(num_viewports)
		{
			// TODO: what should we do here?
			memset(&viewports[0], 0, sizeof(viewports[0]));
			set_viewport();
		}
		num_viewports = count;
	}

	virtual void STDMETHODCALLTYPE RSGetViewports(
		unsigned *out_count,
		D3D11_VIEWPORT *out_viewports)
	{
		SYNCHRONIZED;
		if(out_viewports)
		{
			unsigned i;
			for(i = 0; i < std::min(*out_count, num_viewports); ++i)
				out_viewports[i] = viewports[i];

			memset(out_viewports + i, 0, (*out_count - i) * sizeof(D3D11_VIEWPORT));
		}

		*out_count = num_viewports;
	}

	void set_scissor()
	{
		pipe_scissor_state scissor;
		scissor.minx = scissor_rects[0].left;
		scissor.miny = scissor_rects[0].top;
		scissor.maxx = scissor_rects[0].right;
		scissor.maxy = scissor_rects[0].bottom;
		pipe->set_scissor_state(pipe, &scissor);
	}

	virtual void STDMETHODCALLTYPE RSSetScissorRects(
		unsigned count,
		const D3D11_RECT *new_rects)
	{
		SYNCHRONIZED;
		if(count)
		{
			if(memcmp(&scissor_rects[0], &new_rects[0], sizeof(scissor_rects[0])))
			{
				scissor_rects[0] = new_rects[0];
				set_scissor();
			}
			for(unsigned i = 1; i < count; ++i)
				scissor_rects[i] = new_rects[i];
		}
		else if(num_scissor_rects)
		{
			// TODO: what should we do here?
			memset(&scissor_rects[0], 0, sizeof(scissor_rects[0]));
			set_scissor();
		}

		num_scissor_rects = count;
	}

	virtual void STDMETHODCALLTYPE RSGetScissorRects(
		unsigned *out_count,
		D3D11_RECT *out_rects)
	{
		SYNCHRONIZED;
		if(out_rects)
		{
			unsigned i;
			for(i = 0; i < std::min(*out_count, num_scissor_rects); ++i)
				out_rects[i] = scissor_rects[i];

			memset(out_rects + i, 0, (*out_count - i) * sizeof(D3D11_RECT));
		}

		*out_count = num_scissor_rects;
	}

	virtual void STDMETHODCALLTYPE OMSetBlendState(
		ID3D11BlendState *new_blend_state,
		const float new_blend_factor[4],
		unsigned new_sample_mask)
	{
		SYNCHRONIZED;
		float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};

		if(blend_state.p != new_blend_state)
		{
			pipe->bind_blend_state(pipe, new_blend_state ? ((GalliumD3D11BlendState*)new_blend_state)->object : default_blend);
			blend_state = new_blend_state;
		}

		// Windows D3D11 does this, even though it's apparently undocumented
		if(!new_blend_factor)
			new_blend_factor = white;

		if(memcmp(blend_color, new_blend_factor, sizeof(blend_color)))
		{
			pipe->set_blend_color(pipe, (struct pipe_blend_color*)new_blend_factor);
			memcpy(blend_color, new_blend_factor, sizeof(blend_color));
		}

		if(sample_mask != new_sample_mask)
		{
			pipe->set_sample_mask(pipe, new_sample_mask);
			sample_mask = new_sample_mask;
		}
	}

	virtual void STDMETHODCALLTYPE OMGetBlendState(
		ID3D11BlendState **out_blend_state,
		float out_blend_factor[4],
		unsigned *out_sample_mask)
	{
		SYNCHRONIZED;
		if(out_blend_state)
			*out_blend_state = blend_state.ref();
		if(out_blend_factor)
			memcpy(out_blend_factor, blend_color, sizeof(blend_color));
		if(out_sample_mask)
			*out_sample_mask = sample_mask;
	}

	void set_stencil_ref()
	{
		struct pipe_stencil_ref sref;
		sref.ref_value[0] = stencil_ref;
		sref.ref_value[1] = stencil_ref;
		pipe->set_stencil_ref(pipe, &sref);
	}

	virtual void STDMETHODCALLTYPE OMSetDepthStencilState(
		ID3D11DepthStencilState *new_depth_stencil_state,
		unsigned new_stencil_ref)
	{
		SYNCHRONIZED;
		if(new_depth_stencil_state != depth_stencil_state.p)
		{
			pipe->bind_depth_stencil_alpha_state(pipe, new_depth_stencil_state ? ((GalliumD3D11DepthStencilState*)new_depth_stencil_state)->object : default_depth_stencil);
			depth_stencil_state = new_depth_stencil_state;
		}

		if(new_stencil_ref != stencil_ref)
		{
			stencil_ref = new_stencil_ref;
			set_stencil_ref();
		}
	}

	virtual void STDMETHODCALLTYPE OMGetDepthStencilState(
		ID3D11DepthStencilState **out_depth_stencil_state,
		unsigned *out_stencil_ref)
	{
		SYNCHRONIZED;
		if(*out_depth_stencil_state)
			*out_depth_stencil_state = depth_stencil_state.ref();
		if(out_stencil_ref)
			*out_stencil_ref = stencil_ref;
	}

	void set_framebuffer()
	{
		struct pipe_framebuffer_state fb;
		memset(&fb, 0, sizeof(fb));
		if(depth_stencil_view)
		{
			struct pipe_surface* surf = ((GalliumD3D11DepthStencilView*)depth_stencil_view.p)->object;
			fb.zsbuf = surf;
			if(surf->width > fb.width)
				fb.width = surf->width;
			if(surf->height > fb.height)
				fb.height = surf->height;
		}
		fb.nr_cbufs = num_render_target_views;
		unsigned i;
		for(i = 0; i < num_render_target_views; ++i)
		{
			if(render_target_views[i])
			{
				struct pipe_surface* surf = ((GalliumD3D11RenderTargetView*)render_target_views[i].p)->object;
				fb.cbufs[i] = surf;
				if(surf->width > fb.width)
					fb.width = surf->width;
				if(surf->height > fb.height)
					fb.height = surf->height;
			}
		}

		pipe->set_framebuffer_state(pipe, &fb);
	}

	/* TODO: the docs say that we should unbind conflicting resources (e.g. those bound for read while we are binding them for write too), but we aren't.
	 * Hopefully nobody relies on this happening
	 */

	virtual void STDMETHODCALLTYPE OMSetRenderTargets(
		unsigned count,
		ID3D11RenderTargetView *const *new_render_target_views,
		ID3D11DepthStencilView  *new_depth_stencil_view)
	{
		SYNCHRONIZED;

		bool update = false;
		unsigned i, num;

		if(depth_stencil_view.p != new_depth_stencil_view) {
			update = true;
			depth_stencil_view = new_depth_stencil_view;
		}

		if(!new_render_target_views)
			count = 0;

		for(num = 0, i = 0; i < count; ++i) {
#if API >= 11
			// XXX: is unbinding the UAVs here correct ?
			om_unordered_access_views[i] = (ID3D11UnorderedAccessView*)NULL;
#endif
			if(new_render_target_views[i] != render_target_views[i].p) {
				update = true;
				render_target_views[i] = new_render_target_views[i];
			}
			if(new_render_target_views[i])
				num = i + 1;
		}
		if(num != num_render_target_views) {
			update = true;
			for(; i < num_render_target_views; ++i)
				render_target_views[i] = (ID3D11RenderTargetView*)NULL;
		}
		num_render_target_views = num;
		if(update)
			set_framebuffer();
	}

	virtual void STDMETHODCALLTYPE OMGetRenderTargets(
		unsigned count,
		ID3D11RenderTargetView **out_render_target_views,
		ID3D11DepthStencilView  **out_depth_stencil_view)
	{
		SYNCHRONIZED;
		if(out_render_target_views)
		{
			unsigned i;
			for(i = 0; i < std::min(num_render_target_views, count); ++i)
				out_render_target_views[i] = render_target_views[i].ref();

			for(; i < count; ++i)
				out_render_target_views[i] = 0;
		}

		if(out_depth_stencil_view)
			*out_depth_stencil_view = depth_stencil_view.ref();
	}

#if API >= 11
	/* TODO: what is this supposed to do _exactly_? are we doing the right thing? */
	virtual void STDMETHODCALLTYPE OMSetRenderTargetsAndUnorderedAccessViews(
		unsigned rtv_count,
		ID3D11RenderTargetView *const *new_render_target_views,
		ID3D11DepthStencilView  *new_depth_stencil_view,
		unsigned uav_start,
		unsigned uav_count,
		ID3D11UnorderedAccessView *const *new_unordered_access_views,
		const unsigned *new_uav_initial_counts)
	{
		SYNCHRONIZED;
		if(rtv_count != D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL)
			OMSetRenderTargets(rtv_count, new_render_target_views, new_depth_stencil_view);

		if(uav_count != D3D11_KEEP_UNORDERED_ACCESS_VIEWS)
		{
			for(unsigned i = 0; i < uav_count; ++i)
			{
				om_unordered_access_views[uav_start + i] = new_unordered_access_views[i];
				render_target_views[uav_start + i] = (ID3D11RenderTargetView*)0;
			}
		}
	}

	virtual void STDMETHODCALLTYPE OMGetRenderTargetsAndUnorderedAccessViews(
		unsigned rtv_count,
		ID3D11RenderTargetView **out_render_target_views,
		ID3D11DepthStencilView  **out_depth_stencil_view,
		unsigned uav_start,
		unsigned uav_count,
		ID3D11UnorderedAccessView **out_unordered_access_views)
	{
		SYNCHRONIZED;
		if(out_render_target_views)
			OMGetRenderTargets(rtv_count, out_render_target_views, out_depth_stencil_view);

		if(out_unordered_access_views)
		{
			for(unsigned i = 0; i < uav_count; ++i)
				out_unordered_access_views[i] = om_unordered_access_views[uav_start + i].ref();
		}
	}
#endif

	virtual void STDMETHODCALLTYPE SOSetTargets(
		unsigned count,
		ID3D11Buffer *const *new_so_targets,
		const unsigned *new_offsets)
	{
		SYNCHRONIZED;

		unsigned new_count, i;
		bool changed = false;

		uint32_t append_mask = 0xffffffff;

		if(!new_so_targets)
			count = 0;
		for(new_count = 0, i = 0; i < count; ++i)
		{
			GalliumD3D11Buffer* buffer = static_cast<GalliumD3D11Buffer*>(new_so_targets[i]);

			if(buffer != so_buffers[i].p)
			{
				changed = true;
				so_buffers[i] = buffer;
				so_targets[i] = buffer ? buffer->so_target : 0;
			}
			if(!buffer)
				continue;
			new_count = i + 1;

			if(new_offsets[i] == (unsigned)-1)
			{
				assert(so_targets[i]);
				continue;
			}
			append_mask &= ~(1 << i);

			if(!so_targets[i] || new_offsets[i] != so_targets[i]->buffer_offset)
			{
				pipe_so_target_reference(&buffer->so_target, NULL);
				buffer->so_target = pipe->create_stream_output_target(
					pipe, buffer->resource, new_offsets[i], buffer->resource->width0 - new_offsets[i]);
				so_targets[i] = buffer->so_target;
				changed = true;
			}
		}
		if(i < num_so_targets) {
			changed = true;
			for(; i < num_so_targets; ++i)
				so_buffers[i] = (GalliumD3D11Buffer*)0;
		}
		num_so_targets = new_count;

		if(likely(caps.so) && (changed || append_mask != 0xffffffff))
			pipe->set_stream_output_targets(pipe, num_so_targets, so_targets, append_mask);
	}

	virtual void STDMETHODCALLTYPE SOGetTargets(
		unsigned count,
		ID3D11Buffer **out_so_targets
#if API < 11
		, UINT *out_offsets
#endif
		)
	{
		SYNCHRONIZED;
		for(unsigned i = 0; i < count; ++i)
		{
			out_so_targets[i] = so_buffers[i].ref();
#if API < 11
			out_offsets[i] = so_targets[i]->buffer_offset;
#endif
		}
	}

	virtual void STDMETHODCALLTYPE Begin(
		ID3D11Asynchronous *async)
	{
		SYNCHRONIZED;
		if(caps.queries)
			pipe->begin_query(pipe, ((GalliumD3D11Asynchronous<>*)async)->query);
	}

	virtual void STDMETHODCALLTYPE End(
		ID3D11Asynchronous *async)
	{
		SYNCHRONIZED;
		if(caps.queries)
			pipe->end_query(pipe, ((GalliumD3D11Asynchronous<>*)async)->query);
	}

	virtual HRESULT STDMETHODCALLTYPE GetData(
		ID3D11Asynchronous *iasync,
		void *out_data,
		unsigned data_size,
		unsigned get_data_flags)
	{
		SYNCHRONIZED;
		if(!caps.queries)
			return E_NOTIMPL;

		GalliumD3D11Asynchronous<>* async = (GalliumD3D11Asynchronous<>*)iasync;
		void* tmp_data = alloca(async->data_size);
		memset(tmp_data, 0, async->data_size); // sizeof(BOOL) is 4, sizeof(boolean) is 1
		boolean ret = pipe->get_query_result(pipe, async->query, !(get_data_flags & D3D11_ASYNC_GETDATA_DONOTFLUSH), tmp_data);
		if(out_data)
      {
			memcpy(out_data, tmp_data, std::min(async->data_size, data_size));
      }
		return ret ? S_OK : S_FALSE;
	}

	void set_render_condition()
	{
		if(caps.render_condition)
		{
			if(!render_predicate)
				pipe->render_condition(pipe, 0, 0);
			else
			{
				GalliumD3D11Predicate* predicate = (GalliumD3D11Predicate*)render_predicate.p;
				if(!render_predicate_value && predicate->desc.Query == D3D11_QUERY_OCCLUSION_PREDICATE)
				{
					unsigned mode = (predicate->desc.MiscFlags & D3D11_QUERY_MISC_PREDICATEHINT) ? PIPE_RENDER_COND_NO_WAIT : PIPE_RENDER_COND_WAIT;
					pipe->render_condition(pipe, predicate->query, mode);
				}
				else
				{
					/* TODO: add inverted predication to Gallium*/
					pipe->render_condition(pipe, 0, 0);
				}
			}
		}
	}

	virtual void STDMETHODCALLTYPE SetPredication(
		ID3D11Predicate *new_predicate,
		BOOL new_predicate_value)
	{
		SYNCHRONIZED;
		if(render_predicate.p != new_predicate || render_predicate_value != new_predicate_value)
		{
			render_predicate = new_predicate;
			render_predicate_value = new_predicate_value;
			set_render_condition();
		}
	}

	virtual void STDMETHODCALLTYPE GetPredication(
		ID3D11Predicate **out_predicate,
		BOOL *out_predicate_value)
	{
		SYNCHRONIZED;
		if(out_predicate)
			*out_predicate = render_predicate.ref();
		if(out_predicate_value)
			*out_predicate_value = render_predicate_value;
	}

	static unsigned d3d11_subresource_to_level(struct pipe_resource* resource, unsigned subresource)
	{
		if(subresource <= resource->last_level)
		{
			return subresource;
		}
		else
		{
			unsigned levels = resource->last_level + 1;
			return subresource % levels;
		}
	}

	static unsigned d3d11_subresource_to_layer(struct pipe_resource* resource, unsigned subresource)
	{
		if(subresource <= resource->last_level)
		{
			return 0;
		}
		else
		{
			unsigned levels = resource->last_level + 1;
			return subresource / levels;
		}
	}
		
	
	/* TODO: deferred contexts will need a different implementation of this,
	 * because we can't put the transfer info into the resource itself.
	 * Also, there are very different restrictions, for obvious reasons.
	 */
	virtual HRESULT STDMETHODCALLTYPE Map(
		ID3D11Resource *iresource,
		unsigned subresource,
		D3D11_MAP map_type,
		unsigned map_flags,
		D3D11_MAPPED_SUBRESOURCE *mapped_resource)
	{
		SYNCHRONIZED;
		GalliumD3D11Resource<>* resource = (GalliumD3D11Resource<>*)iresource;
		if(resource->transfers.count(subresource))
			return E_FAIL;
		unsigned level = d3d11_subresource_to_level(resource->resource, subresource);
		unsigned layer = d3d11_subresource_to_layer(resource->resource, subresource);
		pipe_box box = d3d11_to_pipe_box(resource->resource, level, 0);
		box.z += layer;
		unsigned usage = 0;
		if(map_type == D3D11_MAP_READ)
			usage = PIPE_TRANSFER_READ;
		else if(map_type == D3D11_MAP_WRITE)
			usage = PIPE_TRANSFER_WRITE;
		else if(map_type == D3D11_MAP_READ_WRITE)
			usage = PIPE_TRANSFER_READ_WRITE;
		else if(map_type == D3D11_MAP_WRITE_DISCARD)
			usage = PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD;
		else if(map_type == D3D11_MAP_WRITE_NO_OVERWRITE)
			usage = PIPE_TRANSFER_WRITE | PIPE_TRANSFER_UNSYNCHRONIZED;
		else
			return E_INVALIDARG;
		if(map_type & D3D10_MAP_FLAG_DO_NOT_WAIT)
			usage |= PIPE_TRANSFER_DONTBLOCK;
		struct pipe_transfer* transfer = pipe->get_transfer(pipe, resource->resource, level, usage, &box);
		if(!transfer) {
			if(map_type & D3D10_MAP_FLAG_DO_NOT_WAIT)
				return DXGI_ERROR_WAS_STILL_DRAWING;
			else
				return E_FAIL;
		}
		resource->transfers[subresource] = transfer;
		mapped_resource->pData = pipe->transfer_map(pipe, transfer);
		mapped_resource->RowPitch = transfer->stride;
		mapped_resource->DepthPitch = transfer->layer_stride;
		return S_OK;
	}

	virtual void STDMETHODCALLTYPE Unmap(
		ID3D11Resource *iresource,
		unsigned subresource)
	{
		SYNCHRONIZED;
		GalliumD3D11Resource<>* resource = (GalliumD3D11Resource<>*)iresource;
		std::unordered_map<unsigned, pipe_transfer*>::iterator i = resource->transfers.find(subresource);
		if(i != resource->transfers.end())
		{
			pipe->transfer_unmap(pipe, i->second);
			pipe->transfer_destroy(pipe, i->second);
			resource->transfers.erase(i);
		}
	}

	virtual void STDMETHODCALLTYPE CopySubresourceRegion(
		ID3D11Resource *dst_resource,
		unsigned dst_subresource,
		unsigned dst_x,
		unsigned dst_y,
		unsigned dst_z,
		ID3D11Resource *src_resource,
		unsigned src_subresource,
		const D3D11_BOX *src_box)
	{
		SYNCHRONIZED;
		GalliumD3D11Resource<>* dst = (GalliumD3D11Resource<>*)dst_resource;
		GalliumD3D11Resource<>* src = (GalliumD3D11Resource<>*)src_resource;
		unsigned dst_level = d3d11_subresource_to_level(dst->resource, dst_subresource);
		unsigned dst_layer = d3d11_subresource_to_layer(dst->resource, dst_subresource);
		unsigned src_level = d3d11_subresource_to_level(src->resource, src_subresource);
		unsigned src_layer = d3d11_subresource_to_layer(src->resource, src_subresource);
		pipe_box box = d3d11_to_pipe_box(src->resource, src_level, src_box);
		dst_z += dst_layer;
		box.z += src_layer;
		{
			pipe->resource_copy_region(pipe,
				dst->resource, dst_level, dst_x, dst_y, dst_z,
				src->resource, src_level, &box);
		}
	}

	virtual void STDMETHODCALLTYPE CopyResource(
		ID3D11Resource *dst_resource,
		ID3D11Resource *src_resource)
	{
		SYNCHRONIZED;
		GalliumD3D11Resource<>* dst = (GalliumD3D11Resource<>*)dst_resource;
		GalliumD3D11Resource<>* src = (GalliumD3D11Resource<>*)src_resource;
		unsigned level;
		for(level = 0; level <= dst->resource->last_level; ++level)
		{
			pipe_box box;
			box.x = box.y = box.z = 0;
			box.width = u_minify(dst->resource->width0, level);
			box.height = u_minify(dst->resource->height0, level);
			if(dst->resource->target == PIPE_TEXTURE_3D)
				box.depth = u_minify(dst->resource->depth0, level);
			else
				box.depth = dst->resource->array_size;
			pipe->resource_copy_region(pipe,
						   dst->resource, level, 0, 0, 0,
						   src->resource, level, &box);
		}
	}

	virtual void STDMETHODCALLTYPE UpdateSubresource(
		ID3D11Resource *dst_resource,
		unsigned dst_subresource,
		const D3D11_BOX *pDstBox,
		const void *pSrcData,
		unsigned src_row_pitch,
		unsigned src_depth_pitch)
	{
		SYNCHRONIZED;
		GalliumD3D11Resource<>* dst = (GalliumD3D11Resource<>*)dst_resource;
		unsigned dst_level = d3d11_subresource_to_level(dst->resource, dst_subresource);
		unsigned dst_layer = d3d11_subresource_to_layer(dst->resource, dst_subresource);
		pipe_box box = d3d11_to_pipe_box(dst->resource, dst_level, pDstBox);
		box.z += dst_layer;
		pipe->transfer_inline_write(pipe, dst->resource, dst_level, PIPE_TRANSFER_WRITE, &box, pSrcData, src_row_pitch, src_depth_pitch);
	}

#if API >= 11
	virtual void STDMETHODCALLTYPE CopyStructureCount(
		ID3D11Buffer *dst_buffer,
		unsigned dst_aligned_byte_offset,
		ID3D11UnorderedAccessView *src_view)
	{
		SYNCHRONIZED;
	}
#endif

	virtual void STDMETHODCALLTYPE ClearRenderTargetView(
		ID3D11RenderTargetView *render_target_view,
		const float color[4])
	{
		SYNCHRONIZED;
		GalliumD3D11RenderTargetView* view = ((GalliumD3D11RenderTargetView*)render_target_view);
		union pipe_color_union cc;
		cc.f[0] = color[0];
		cc.f[1] = color[1];
		cc.f[2] = color[2];
		cc.f[3] = color[3];
		pipe->clear_render_target(pipe, view->object, &cc, 0, 0, view->object->width, view->object->height);
	}

	virtual void STDMETHODCALLTYPE ClearDepthStencilView(
		ID3D11DepthStencilView  *depth_stencil_view,
		unsigned clear_flags,
		float depth,
		UINT8 stencil)
	{
		SYNCHRONIZED;
		GalliumD3D11DepthStencilView* view = ((GalliumD3D11DepthStencilView*)depth_stencil_view);
		unsigned flags = 0;
		if(clear_flags & D3D11_CLEAR_DEPTH)
			flags |= PIPE_CLEAR_DEPTH;
		if(clear_flags & D3D11_CLEAR_STENCIL)
			flags |= PIPE_CLEAR_STENCIL;
		pipe->clear_depth_stencil(pipe, view->object, flags, depth, stencil, 0, 0, view->object->width, view->object->height);
	}

#if API >= 11
	virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewUint(
		ID3D11UnorderedAccessView *unordered_access_view,
		const unsigned values[4])
	{
		SYNCHRONIZED;
	}

	virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat(
			ID3D11UnorderedAccessView *unordered_access_view,
			const float values[4])
	{
		SYNCHRONIZED;
	}
#endif

	void restore_gallium_state_blit_only()
	{
		pipe->bind_blend_state(pipe, blend_state.p ? blend_state.p->object : default_blend);
		pipe->bind_depth_stencil_alpha_state(pipe, depth_stencil_state.p ? depth_stencil_state.p->object : default_depth_stencil);
		pipe->bind_rasterizer_state(pipe, rasterizer_state.p ? rasterizer_state.p->object : default_rasterizer);
		pipe->bind_vertex_elements_state(pipe, input_layout.p ? input_layout.p->object : default_input_layout);
		pipe->bind_fs_state(pipe, shaders[D3D11_STAGE_PS].p ? shaders[D3D11_STAGE_PS].p->object : default_shaders[PIPE_SHADER_FRAGMENT]);
		pipe->bind_vs_state(pipe, shaders[D3D11_STAGE_VS].p ? shaders[D3D11_STAGE_VS].p->object : default_shaders[PIPE_SHADER_VERTEX]);
		if(caps.gs)
			pipe->bind_gs_state(pipe, shaders[D3D11_STAGE_GS].p ? shaders[D3D11_STAGE_GS].p->object : default_shaders[PIPE_SHADER_GEOMETRY]);
		if(caps.so && num_so_targets)
			pipe->set_stream_output_targets(pipe, num_so_targets, so_targets, ~0);
		set_framebuffer();
		set_viewport();
		set_render_condition();

		update_flags |= UPDATE_VERTEX_BUFFERS | (1 << (UPDATE_SAMPLERS_SHIFT + D3D11_STAGE_PS)) | (1 << (UPDATE_VIEWS_SHIFT + D3D11_STAGE_PS));
	}

	virtual void STDMETHODCALLTYPE RestoreGalliumStateBlitOnly()
	{
		SYNCHRONIZED;
		restore_gallium_state_blit_only();
	}

	virtual void STDMETHODCALLTYPE GenerateMips(
		ID3D11ShaderResourceView *shader_resource_view)
	{
		SYNCHRONIZED;

		GalliumD3D11ShaderResourceView* view = (GalliumD3D11ShaderResourceView*)shader_resource_view;
		if(caps.gs)
			pipe->bind_gs_state(pipe, 0);
		if(caps.so && num_so_targets)
			pipe->set_stream_output_targets(pipe, 0, NULL, 0);
		if(pipe->render_condition)
			pipe->render_condition(pipe, 0, 0);
		for(unsigned layer = view->object->u.tex.first_layer; layer <= view->object->u.tex.last_layer; ++layer)
			util_gen_mipmap(gen_mipmap, view->object, layer, view->object->u.tex.first_level, view->object->u.tex.last_level, PIPE_TEX_FILTER_LINEAR);
		restore_gallium_state_blit_only();
	}

	virtual void STDMETHODCALLTYPE RestoreGalliumState()
	{
		SYNCHRONIZED;
		restore_gallium_state_blit_only();

		set_index_buffer();
		set_stencil_ref();
		pipe->set_blend_color(pipe, (struct pipe_blend_color*)blend_color);
		pipe->set_sample_mask(pipe, sample_mask);

		for(unsigned s = 0; s < 3; ++s)
		{
			unsigned num = std::min(caps.constant_buffers[s], (unsigned)D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);
			for(unsigned i = 0; i < num; ++i)
				pipe_set_constant_buffer(pipe, s, i, constant_buffers[s][i].p ? constant_buffers[s][i].p->resource : 0);
		}

		update_flags |= (1 << (UPDATE_SAMPLERS_SHIFT + D3D11_STAGE_VS)) | (1 << (UPDATE_VIEWS_SHIFT + D3D11_STAGE_VS));
		update_flags |= (1 << (UPDATE_SAMPLERS_SHIFT + D3D11_STAGE_GS)) | (1 << (UPDATE_VIEWS_SHIFT + D3D11_STAGE_GS));

		set_scissor();
	}

#if API >= 11
	/* TODO: hack SRVs or sampler states to handle this, or add to Gallium */
	virtual void STDMETHODCALLTYPE SetResourceMinLOD(
		ID3D11Resource *iresource,
		float min_lod)
	{
		SYNCHRONIZED;
		GalliumD3D11Resource<>* resource = (GalliumD3D11Resource<>*)iresource;
		if(resource->min_lod != min_lod)
		{
			// TODO: actually do anything?
			resource->min_lod = min_lod;
		}
	}

	virtual float STDMETHODCALLTYPE GetResourceMinLOD(
		ID3D11Resource *iresource)
	{
		SYNCHRONIZED;
		GalliumD3D11Resource<>* resource = (GalliumD3D11Resource<>*)iresource;
		return resource->min_lod;
	}
#endif

	virtual void STDMETHODCALLTYPE ResolveSubresource(
		ID3D11Resource *dst_resource,
		unsigned dst_subresource,
		ID3D11Resource *src_resource,
		unsigned src_subresource,
		DXGI_FORMAT format)
	{
		SYNCHRONIZED;
		GalliumD3D11Resource<>* dst = (GalliumD3D11Resource<>*)dst_resource;
		GalliumD3D11Resource<>* src = (GalliumD3D11Resource<>*)src_resource;
		struct pipe_resolve_info info;

		info.dst.res = dst->resource;
		info.src.res = src->resource;
		info.dst.level = 0;
		info.dst.layer = d3d11_subresource_to_layer(dst->resource, dst_subresource);
		info.src.layer = d3d11_subresource_to_layer(src->resource, src_subresource);

		info.src.x0 = 0;
		info.src.x1 = info.src.res->width0;
		info.src.y0 = 0;
		info.src.y1 = info.src.res->height0;
		info.dst.x0 = 0;
		info.dst.x1 = info.dst.res->width0;
		info.dst.y0 = 0;
		info.dst.y1 = info.dst.res->height0;

		info.mask = PIPE_MASK_RGBA | PIPE_MASK_ZS;

		pipe->resource_resolve(pipe, &info);
	}

#if API >= 11
	virtual void STDMETHODCALLTYPE ExecuteCommandList(
		ID3D11CommandList *command_list,
		BOOL restore_context_state)
	{
		SYNCHRONIZED;
	}

	virtual HRESULT STDMETHODCALLTYPE FinishCommandList(
		BOOL restore_deferred_context_state,
		ID3D11CommandList **out_command_list)
	{
		SYNCHRONIZED;
		return E_NOTIMPL;
	}
#endif

	virtual void STDMETHODCALLTYPE ClearState(void)
	{
		/* we don't take a lock here because we would deadlock otherwise
		 * TODO: this is probably incorrect, because ClearState should likely be atomic.
		 * However, I can't think of any correct usage that would be affected by this
		 * being non-atomic, and making this atomic is quite expensive and complicates
		 * the code
		 */

		// we qualify all calls so that we avoid virtual dispatch and might get them inlined
		// TODO: make sure all this gets inlined, which might require more compiler flags
		// TODO: optimize this
#if API >= 11
		GalliumD3D11DeviceContext::PSSetShader(0, 0, 0);
		GalliumD3D11DeviceContext::GSSetShader(0, 0, 0);
		GalliumD3D11DeviceContext::VSSetShader(0, 0, 0);
		GalliumD3D11DeviceContext::HSSetShader(0, 0, 0);
		GalliumD3D11DeviceContext::DSSetShader(0, 0, 0);
		GalliumD3D11DeviceContext::CSSetShader(0, 0, 0);
#else
		GalliumD3D11DeviceContext::PSSetShader(0);
		GalliumD3D11DeviceContext::GSSetShader(0);
		GalliumD3D11DeviceContext::VSSetShader(0);
#endif

		GalliumD3D11DeviceContext::IASetInputLayout(0);
		GalliumD3D11DeviceContext::IASetIndexBuffer(0, DXGI_FORMAT_UNKNOWN, 0);
		GalliumD3D11DeviceContext::RSSetState(0);
		GalliumD3D11DeviceContext::OMSetDepthStencilState(0, 0);
		GalliumD3D11DeviceContext::OMSetBlendState(0, (float*)zero_data, ~0);
		GalliumD3D11DeviceContext::SetPredication(0, 0);
		GalliumD3D11DeviceContext::IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);

		GalliumD3D11DeviceContext::PSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, (ID3D11Buffer**)zero_data);
		GalliumD3D11DeviceContext::GSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, (ID3D11Buffer**)zero_data);
		GalliumD3D11DeviceContext::VSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, (ID3D11Buffer**)zero_data);
#if API >= 11
		GalliumD3D11DeviceContext::HSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, (ID3D11Buffer**)zero_data);
		GalliumD3D11DeviceContext::DSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, (ID3D11Buffer**)zero_data);
		GalliumD3D11DeviceContext::CSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, (ID3D11Buffer**)zero_data);
#endif

		GalliumD3D11DeviceContext::IASetVertexBuffers(0, num_vertex_buffers, (ID3D11Buffer**)zero_data, (unsigned*)zero_data, (unsigned*)zero_data);
#if API >= 11
		GalliumD3D11DeviceContext::OMSetRenderTargetsAndUnorderedAccessViews(0, 0, 0 , 0, 0, 0, 0);
#else
		GalliumD3D11DeviceContext::OMSetRenderTargets(0, 0, 0 );
#endif
		GalliumD3D11DeviceContext::SOSetTargets(0, 0, 0);

		GalliumD3D11DeviceContext::PSSetShaderResources(0, num_shader_resource_views[D3D11_STAGE_PS], (ID3D11ShaderResourceView**)zero_data);
		GalliumD3D11DeviceContext::GSSetShaderResources(0, num_shader_resource_views[D3D11_STAGE_GS], (ID3D11ShaderResourceView**)zero_data);
		GalliumD3D11DeviceContext::VSSetShaderResources(0, num_shader_resource_views[D3D11_STAGE_VS], (ID3D11ShaderResourceView**)zero_data);
#if API >= 11
		GalliumD3D11DeviceContext::HSSetShaderResources(0, num_shader_resource_views[D3D11_STAGE_HS], (ID3D11ShaderResourceView**)zero_data);
		GalliumD3D11DeviceContext::DSSetShaderResources(0, num_shader_resource_views[D3D11_STAGE_DS], (ID3D11ShaderResourceView**)zero_data);
		GalliumD3D11DeviceContext::CSSetShaderResources(0, num_shader_resource_views[D3D11_STAGE_CS], (ID3D11ShaderResourceView**)zero_data);
#endif

		GalliumD3D11DeviceContext::PSSetSamplers(0, num_shader_resource_views[D3D11_STAGE_PS], (ID3D11SamplerState**)zero_data);
		GalliumD3D11DeviceContext::GSSetSamplers(0, num_shader_resource_views[D3D11_STAGE_GS], (ID3D11SamplerState**)zero_data);
		GalliumD3D11DeviceContext::VSSetSamplers(0, num_shader_resource_views[D3D11_STAGE_VS], (ID3D11SamplerState**)zero_data);
#if API >= 11
		GalliumD3D11DeviceContext::HSSetSamplers(0, num_shader_resource_views[D3D11_STAGE_HS], (ID3D11SamplerState**)zero_data);
		GalliumD3D11DeviceContext::DSSetSamplers(0, num_shader_resource_views[D3D11_STAGE_DS], (ID3D11SamplerState**)zero_data);
		GalliumD3D11DeviceContext::CSSetSamplers(0, num_shader_resource_views[D3D11_STAGE_CS], (ID3D11SamplerState**)zero_data);
#endif

		GalliumD3D11DeviceContext::RSSetViewports(0, 0);
		GalliumD3D11DeviceContext::RSSetScissorRects(0, 0);
	}

	virtual void STDMETHODCALLTYPE Flush(void)
	{
		SYNCHRONIZED;
                pipe->flush(pipe, 0);
	}

	/* In Direct3D 10, if the reference count of an object drops to 0, it is automatically
	 * cleanly unbound from the pipeline.
	 * In Direct3D 11, the pipeline holds a reference.
	 *
	 * Note that instead of always scanning the pipeline on destruction, we could
	 * maintain the internal reference count on DirectX 10 and use it to check if an
	 * object is still bound.
	 * Presumably, on average, scanning is faster if the application is well written.
	 */
#if API < 11
#define IMPLEMENT_SIMPLE_UNBIND(name, member, gallium, def) \
	void Unbind##name(ID3D11##name* state) \
	{ \
		SYNCHRONIZED; \
		if((void*)state == (void*)member.p) \
		{ \
			member.p = 0; \
			pipe->bind_##gallium##_state(pipe, default_##def); \
		} \
	}
	IMPLEMENT_SIMPLE_UNBIND(BlendState, blend_state, blend, blend)
	IMPLEMENT_SIMPLE_UNBIND(RasterizerState, rasterizer_state, rasterizer, rasterizer)
	IMPLEMENT_SIMPLE_UNBIND(DepthStencilState, depth_stencil_state, depth_stencil_alpha, depth_stencil)
	IMPLEMENT_SIMPLE_UNBIND(InputLayout, input_layout, vertex_elements, input_layout)
	IMPLEMENT_SIMPLE_UNBIND(PixelShader, shaders[D3D11_STAGE_PS], fs, shaders[D3D11_STAGE_PS])
	IMPLEMENT_SIMPLE_UNBIND(VertexShader, shaders[D3D11_STAGE_VS], vs, shaders[D3D11_STAGE_VS])
	IMPLEMENT_SIMPLE_UNBIND(GeometryShader, shaders[D3D11_STAGE_GS], gs, shaders[D3D11_STAGE_GS])

	void UnbindPredicate(ID3D11Predicate* predicate)
	{
		SYNCHRONIZED;
		if(predicate == render_predicate)
		{
			render_predicate.p = NULL;
			render_predicate_value = 0;
			pipe->render_condition(pipe, 0, 0);
		}
	}

	void UnbindSamplerState(ID3D11SamplerState* state)
	{
		SYNCHRONIZED;
		for(unsigned s = 0; s < D3D11_STAGES; ++s)
		{
			for(unsigned i = 0; i < num_samplers[s]; ++i)
			{
				if(samplers[s][i] == state)
				{
					samplers[s][i].p = NULL;
					sampler_csos[s][i] = NULL;
					update_flags |= (1 << (UPDATE_SAMPLERS_SHIFT + s));
				}
			}
		}
	}

	void UnbindBuffer(ID3D11Buffer* buffer)
	{
		SYNCHRONIZED;
		if(buffer == index_buffer)
		{
			index_buffer.p = 0;
			index_format = DXGI_FORMAT_UNKNOWN;
			index_offset = 0;
			struct pipe_index_buffer ib;
			memset(&ib, 0, sizeof(ib));
			pipe->set_index_buffer(pipe, &ib);
		}

		for(unsigned i = 0; i < num_vertex_buffers; ++i)
		{
			if(buffer == input_buffers[i])
			{
				input_buffers[i].p = 0;
				memset(&vertex_buffers[num_vertex_buffers], 0, sizeof(vertex_buffers[num_vertex_buffers]));
				update_flags |= UPDATE_VERTEX_BUFFERS;
			}
		}

		for(unsigned s = 0; s < D3D11_STAGES; ++s)
		{
			for(unsigned i = 0; i < sizeof(constant_buffers) / sizeof(constant_buffers[0]); ++i)
			{
				if(constant_buffers[s][i] == buffer)
				{
					constant_buffers[s][i] = (ID3D10Buffer*)NULL;
					pipe_set_constant_buffer(pipe, s, i, NULL);
				}
			}
		}
	}

	void UnbindDepthStencilView(ID3D11DepthStencilView * view)
	{
		SYNCHRONIZED;
		if(view == depth_stencil_view)
		{
			depth_stencil_view.p = NULL;
			set_framebuffer();
		}
	}

	void UnbindRenderTargetView(ID3D11RenderTargetView* view)
	{
		SYNCHRONIZED;
		bool any_bound = false;
		for(unsigned i = 0; i < num_render_target_views; ++i)
		{
			if(render_target_views[i] == view)
			{
				render_target_views[i].p = NULL;
				any_bound = true;
			}
		}
		if(any_bound)
			set_framebuffer();
	}

	void UnbindShaderResourceView(ID3D11ShaderResourceView* view)
	{
		SYNCHRONIZED;
		for(unsigned s = 0; s < D3D11_STAGES; ++s)
		{
			for(unsigned i = 0; i < num_shader_resource_views[s]; ++i)
			{
				if(shader_resource_views[s][i] == view)
				{
					shader_resource_views[s][i].p = NULL;
					sampler_views[s][i] = NULL;
					update_flags |= (1 << (UPDATE_VIEWS_SHIFT + s));
				}
			}
		}
	}
#endif

#undef SYNCHRONIZED
};

#if API >= 11
/* This approach serves two purposes.
 * First, we don't want to do an atomic operation to manipulate the reference
 * count every time something is bound/unbound to the pipeline, since they are
 * expensive.
 * Fortunately, the immediate context can only be used by a single thread, so
 * we don't have to use them, as long as a separate reference count is used
 * (see dual_refcnt_t).
 *
 * Second, we want to avoid the Device -> DeviceContext -> bound DeviceChild -> Device
 * garbage cycle.
 * To avoid it, DeviceChild doesn't hold a reference to Device as usual, but adds
 * one for each external reference count, while internal nonatomic_add_ref doesn't
 * add any.
 *
 * Note that ideally we would to eliminate the non-atomic op too, but this is more
 * complicated, since we would either need to use garbage collection and give up
 * deterministic destruction (especially bad for large textures), or scan the whole
 * pipeline state every time the reference count of object drops to 0, which risks
 * pathological slowdowns.
 *
 * Since this microoptimization should matter relatively little, let's avoid it for now.
 *
 * Note that deferred contexts don't use this, since as a whole, they must thread-safe.
 * Eliminating the atomic ops for deferred contexts seems substantially harder.
 * This might be a problem if they are used in a one-shot multithreaded rendering
 * fashion, where SMP cacheline bouncing on the reference count may be visible.
 *
 * The idea would be to attach a structure of reference counts indexed by deferred
 * context id to each object. Ideally, this should be organized like ext2 block pointers.
 *
 * Every deferred context would get a reference count in its own cacheline.
 * The external count is protected by a lock bit, and there is also a "lock bit" in each
 * internal count.
 *
 * When the external count has to be dropped to 0, the lock bit is taken and all internal
 * reference counts are scanned, taking a count of them. A flag would also be set on them.
 * Deferred context manipulation would notice the flag, and update the count.
 * Once the count goes to zero, the object is freed.
 *
 * The problem of this is that if the external reference count ping-pongs between
 * zero and non-zero, the scans will take a lot of time.
 *
 * The idea to solve this is to compute the scans in a binary-tree like fashion, where
 * each binary tree node would have a "determined bit", which would be invalidated
 * by manipulations.
 *
 * However, all this complexity might actually be a loss in most cases, so let's just
 * stick to a single atomic refcnt for now.
 *
 * Also, we don't even support deferred contexts yet, so this can wait.
 */
struct nonatomic_device_child_ptr_traits
{
	static void add_ref(void* p)
	{
		if(p)
			((GalliumD3D11DeviceChild<>*)p)->nonatomic_add_ref();
	}

	static void release(void* p)
	{
		if(p)
			((GalliumD3D11DeviceChild<>*)p)->nonatomic_release();
	}
};

struct GalliumD3D11ImmediateDeviceContext
	: public GalliumD3D11DeviceContext<nonatomic_device_child_ptr_traits>
{
	GalliumD3D11ImmediateDeviceContext(GalliumD3D11Screen* device, pipe_context* pipe, unsigned context_flags = 0)
	: GalliumD3D11DeviceContext<nonatomic_device_child_ptr_traits>(device, pipe, context_flags)
	{
		// not necessary, but tests that the API at least basically works
		ClearState();
	}

	/* we do this since otherwise we would have a garbage cycle between this and the device */
	virtual ULONG STDMETHODCALLTYPE AddRef()
	{
		return this->device->AddRef();
	}

	virtual ULONG STDMETHODCALLTYPE Release()
	{
		return this->device->Release();
	}

	virtual D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType()
	{
		return D3D11_DEVICE_CONTEXT_IMMEDIATE;
	}
};

static ID3D11DeviceContext* GalliumD3D11ImmediateDeviceContext_Create(GalliumD3D11Screen* device, struct pipe_context* pipe, bool owns_pipe)
{
	return new GalliumD3D11ImmediateDeviceContext(device, pipe, owns_pipe);
}

static void GalliumD3D11ImmediateDeviceContext_RestoreGalliumState(ID3D11DeviceContext* context)
{
	((GalliumD3D11ImmediateDeviceContext*)context)->RestoreGalliumState();
}

static void GalliumD3D11ImmediateDeviceContext_RestoreGalliumStateBlitOnly(ID3D11DeviceContext* context)
{
	((GalliumD3D11ImmediateDeviceContext*)context)->RestoreGalliumStateBlitOnly();
}

static void GalliumD3D11ImmediateDeviceContext_Destroy(ID3D11DeviceContext* context)
{
	delete (GalliumD3D11ImmediateDeviceContext*)context;
}
#endif
