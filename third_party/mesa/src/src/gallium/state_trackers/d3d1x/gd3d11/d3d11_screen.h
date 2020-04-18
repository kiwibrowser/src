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

DEBUG_GET_ONCE_BOOL_OPTION(dump_shaders, "D3D1X_DUMP_SHADERS", FALSE);

/* These cap sets are much more correct than the ones in u_caps.c */
/* TODO: it seems cube levels should be the same as 2D levels */

/* DX 9_1 */
static unsigned caps_dx_9_1[] = {
	UTIL_CHECK_INT(MAX_RENDER_TARGETS, 1),
	UTIL_CHECK_INT(MAX_TEXTURE_2D_LEVELS, 12),	/* 2048 */
	UTIL_CHECK_INT(MAX_TEXTURE_3D_LEVELS, 8),	 /* 256 */
	UTIL_CHECK_INT(MAX_TEXTURE_CUBE_LEVELS, 10), /* 512 */
	UTIL_CHECK_TERMINATE
};

/* DX 9_2 */
static unsigned caps_dx_9_2[] = {
	UTIL_CHECK_CAP(OCCLUSION_QUERY),
	UTIL_CHECK_CAP(TWO_SIDED_STENCIL),
	UTIL_CHECK_CAP(TEXTURE_MIRROR_CLAMP),
	UTIL_CHECK_CAP(BLEND_EQUATION_SEPARATE),
	UTIL_CHECK_INT(MAX_RENDER_TARGETS, 1),
	UTIL_CHECK_INT(MAX_TEXTURE_2D_LEVELS, 12),	/* 2048 */
	UTIL_CHECK_INT(MAX_TEXTURE_3D_LEVELS, 9),	 /* 256 */
	UTIL_CHECK_INT(MAX_TEXTURE_CUBE_LEVELS, 10), /* 512 */
	UTIL_CHECK_TERMINATE
};

/* DX 9_3 */
static unsigned caps_dx_9_3[] = {
	UTIL_CHECK_CAP(OCCLUSION_QUERY),
	UTIL_CHECK_CAP(TWO_SIDED_STENCIL),
	UTIL_CHECK_CAP(TEXTURE_MIRROR_CLAMP),
	UTIL_CHECK_CAP(BLEND_EQUATION_SEPARATE),
	UTIL_CHECK_CAP(SM3),
	UTIL_CHECK_CAP(VERTEX_ELEMENT_INSTANCE_DIVISOR),
	UTIL_CHECK_CAP(OCCLUSION_QUERY),
	UTIL_CHECK_INT(MAX_RENDER_TARGETS, 4),
	UTIL_CHECK_INT(MAX_TEXTURE_2D_LEVELS, 13),	/* 4096 */
	UTIL_CHECK_INT(MAX_TEXTURE_3D_LEVELS, 9),	 /* 256 */
	UTIL_CHECK_INT(MAX_TEXTURE_CUBE_LEVELS, 10), /* 512 */
	UTIL_CHECK_TERMINATE
};

static unsigned caps_dx_10_0[] = {
	UTIL_CHECK_CAP(INDEP_BLEND_ENABLE),
	UTIL_CHECK_CAP(ANISOTROPIC_FILTER),
	UTIL_CHECK_CAP(MIXED_COLORBUFFER_FORMATS),
	UTIL_CHECK_CAP(FRAGMENT_COLOR_CLAMP_CONTROL),
	UTIL_CHECK_CAP(CONDITIONAL_RENDER),
	UTIL_CHECK_CAP(PRIMITIVE_RESTART),
	UTIL_CHECK_CAP(TGSI_INSTANCEID),
	UTIL_CHECK_INT(MAX_RENDER_TARGETS, 8),
	UTIL_CHECK_INT(MAX_TEXTURE_2D_LEVELS, 13),
	UTIL_CHECK_INT(MAX_TEXTURE_ARRAY_LAYERS, 512),
	UTIL_CHECK_INT(MAX_STREAM_OUTPUT_BUFFERS, 4),
	UTIL_CHECK_SHADER(VERTEX, MAX_INPUTS, 16),
	UTIL_CHECK_SHADER(GEOMETRY, MAX_CONST_BUFFERS, 14),
	UTIL_CHECK_SHADER(GEOMETRY, MAX_TEXTURE_SAMPLERS, 16),
	UTIL_CHECK_SHADER(GEOMETRY, SUBROUTINES, 1),
	UTIL_CHECK_SHADER(FRAGMENT, INTEGERS, 1),
	UTIL_CHECK_TERMINATE
};


// this is called "screen" because in the D3D10 case it's only part of the device
template<bool threadsafe>
struct GalliumD3D11ScreenImpl : public GalliumD3D11Screen
{
	D3D_FEATURE_LEVEL feature_level;
	int format_support[PIPE_FORMAT_COUNT];
	unsigned creation_flags;
	unsigned exception_mode;
	maybe_mutex_t<threadsafe> mutex;

/* TODO: Direct3D 11 specifies that fine-grained locking should be used if the driver supports it.
 * Right now, I don't trust Gallium drivers to get this right.
 */
#define SYNCHRONIZED lock_t<maybe_mutex_t<threadsafe> > lock_(mutex)

	GalliumD3D11ScreenImpl(struct pipe_screen* screen, struct pipe_context* immediate_pipe, BOOL owns_immediate_pipe,unsigned creation_flags, IDXGIAdapter* adapter)
	: GalliumD3D11Screen(screen, immediate_pipe, adapter), creation_flags(creation_flags)
	{
		memset(&screen_caps, 0, sizeof(screen_caps));
		screen_caps.gs = screen->get_shader_param(screen, PIPE_SHADER_GEOMETRY, PIPE_SHADER_CAP_MAX_INSTRUCTIONS) > 0;
		screen_caps.so = screen->get_param(screen, PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS) > 0;
		screen_caps.queries = screen->get_param(screen, PIPE_CAP_OCCLUSION_QUERY);
		screen_caps.render_condition = screen->get_param(screen, PIPE_CAP_CONDITIONAL_RENDER);
		for(unsigned i = 0; i < PIPE_SHADER_TYPES; ++i)
			screen_caps.constant_buffers[i] = screen->get_shader_param(screen, i, PIPE_SHADER_CAP_MAX_CONST_BUFFERS);
		screen_caps.stages = 0;
		for(unsigned i = 0; i < PIPE_SHADER_TYPES; ++i)
		{
			if(!screen->get_shader_param(screen, i, PIPE_SHADER_CAP_MAX_INSTRUCTIONS))
				break;
			screen_caps.stages = i + 1;
		}

		screen_caps.stages_with_sampling = (1 << screen_caps.stages) - 1;
		if(!screen->get_shader_param(screen, PIPE_SHADER_VERTEX, PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS))
			screen_caps.stages_with_sampling &=~ (1 << PIPE_SHADER_VERTEX);

		memset(format_support, 0xff, sizeof(format_support));

		float default_level = 9.1f;
		if(!util_check_caps(screen, caps_dx_9_1))
			_debug_printf("Warning: driver does not even meet D3D_FEATURE_LEVEL_9_1 features, advertising it anyway!\n");
		else if(!util_check_caps(screen, caps_dx_9_2))
			default_level = 9.1f;
		else if(!util_check_caps(screen, caps_dx_9_3))
			default_level = 9.2f;
		else if(!util_check_caps(screen, caps_dx_10_0))
			default_level = 9.3f;
		else
			default_level = 10.0f;

		char default_level_name[64];
		sprintf(default_level_name, "%.1f", default_level);
		float feature_level_number = atof(debug_get_option("D3D11_FEATURE_LEVEL", default_level_name));
		if(!feature_level_number)
			feature_level_number = default_level;

#if API >= 11
		if(feature_level_number >= 11.0f)
			feature_level = D3D_FEATURE_LEVEL_11_0;
		else
#endif
		if(feature_level_number >= 10.1f)
			feature_level = D3D_FEATURE_LEVEL_10_1;
		else if(feature_level_number >= 10.0f)
			feature_level = D3D_FEATURE_LEVEL_10_0;
		else if(feature_level_number >= 9.3f)
			feature_level = D3D_FEATURE_LEVEL_9_3;
		else if(feature_level_number >= 9.2f)
			feature_level = D3D_FEATURE_LEVEL_9_2;
		else
			feature_level = D3D_FEATURE_LEVEL_9_1;

#if API >= 11
		immediate_context = GalliumD3D11ImmediateDeviceContext_Create(this, immediate_pipe, owns_immediate_pipe);
		// release to the reference to ourselves that the immediate context took, to avoid a garbage cycle
		immediate_context->Release();
#endif
	}

	~GalliumD3D11ScreenImpl()
	{
#if API >= 11
		GalliumD3D11ImmediateDeviceContext_Destroy(immediate_context);
#endif
	}

	virtual D3D_FEATURE_LEVEL STDMETHODCALLTYPE GetFeatureLevel(void)
	{
		return feature_level;
	}

	virtual unsigned STDMETHODCALLTYPE GetCreationFlags(void)
	{
		return creation_flags;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason(void)
	{
		return S_OK;
	}

#if API >= 11
	virtual void STDMETHODCALLTYPE GetImmediateContext(
		ID3D11DeviceContext **out_immediate_context)
	{
		immediate_context->AddRef();
		*out_immediate_context = immediate_context;
	}
#endif

	virtual HRESULT STDMETHODCALLTYPE SetExceptionMode(unsigned RaiseFlags)
	{
		exception_mode = RaiseFlags;
		return S_OK;
	}

	virtual unsigned STDMETHODCALLTYPE GetExceptionMode(void)
	{
		return exception_mode;
	}

	virtual HRESULT STDMETHODCALLTYPE CheckCounter(
		const D3D11_COUNTER_DESC *desc,
		D3D11_COUNTER_TYPE *type,
		unsigned *active_counters,
		LPSTR sz_name,
		unsigned *name_length,
		LPSTR sz_units,
		unsigned *units_length,
		LPSTR sz_description,
		unsigned *description_length)
	{
		return E_NOTIMPL;
	}

	virtual void STDMETHODCALLTYPE CheckCounterInfo(
		D3D11_COUNTER_INFO *counter_info)
	{
		/* none supported at the moment */
		counter_info->LastDeviceDependentCounter = (D3D11_COUNTER)0;
		counter_info->NumDetectableParallelUnits = 1;
		counter_info->NumSimultaneousCounters = 0;
	}

#if API >= 11
	virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport(
		D3D11_FEATURE feature,
		void *out_feature_support_data,
		unsigned feature_support_data_size)
	{
		SYNCHRONIZED;

		switch(feature)
		{
			case D3D11_FEATURE_THREADING:
			{
				D3D11_FEATURE_DATA_THREADING* data = (D3D11_FEATURE_DATA_THREADING*)out_feature_support_data;
				if(feature_support_data_size != sizeof(*data))
					return E_INVALIDARG;

				data->DriverCommandLists = FALSE;
				data->DriverConcurrentCreates = FALSE;
				return S_OK;
			}
			case D3D11_FEATURE_DOUBLES:
			{
				D3D11_FEATURE_DATA_DOUBLES* data = (D3D11_FEATURE_DATA_DOUBLES*)out_feature_support_data;
				if(feature_support_data_size != sizeof(*data))
					return E_INVALIDARG;

				data->DoublePrecisionFloatShaderOps = FALSE;
				return S_OK;
			}
			case D3D11_FEATURE_FORMAT_SUPPORT:
			{
				D3D11_FEATURE_DATA_FORMAT_SUPPORT* data = (D3D11_FEATURE_DATA_FORMAT_SUPPORT*)out_feature_support_data;
				if(feature_support_data_size != sizeof(*data))
					return E_INVALIDARG;

				return this->CheckFormatSupport(data->InFormat, &data->OutFormatSupport);
			}
			case D3D11_FEATURE_FORMAT_SUPPORT2:
			{
				D3D11_FEATURE_DATA_FORMAT_SUPPORT* data = (D3D11_FEATURE_DATA_FORMAT_SUPPORT*)out_feature_support_data;
				if(feature_support_data_size != sizeof(*data))
					return E_INVALIDARG;

				data->OutFormatSupport = 0;
				/* TODO: should this be S_OK? */
				return E_INVALIDARG;
			}
			case D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS:
			{
				D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS* data = (D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS*)out_feature_support_data;
				if(feature_support_data_size != sizeof(*data))
					return E_INVALIDARG;

				data->ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x = FALSE;
				return S_OK;
			}
			default:
				return E_INVALIDARG;
		}
	}
#endif

	virtual HRESULT STDMETHODCALLTYPE CheckFormatSupport(
		DXGI_FORMAT dxgi_format,
		unsigned *out_format_support
	)
	{
		SYNCHRONIZED;

		/* TODO: MSAA, advanced features */

		pipe_format format = dxgi_to_pipe_format[dxgi_format];
		if(!format)
			return E_INVALIDARG;

		int support = format_support[format];
		if(support < 0)
		{
			support = 0;

			if(dxgi_format == DXGI_FORMAT_R8_UINT ||
			   dxgi_format == DXGI_FORMAT_R16_UINT ||
			   dxgi_format == DXGI_FORMAT_R32_UINT)
				support |= D3D11_FORMAT_SUPPORT_IA_INDEX_BUFFER;

			if(screen->is_format_supported(screen, format, PIPE_BUFFER, 0, PIPE_BIND_VERTEX_BUFFER))
				support |= D3D11_FORMAT_SUPPORT_IA_VERTEX_BUFFER;

                        if(screen->is_format_supported(screen, format, PIPE_BUFFER, 0, PIPE_BIND_STREAM_OUTPUT))
				support |= D3D11_FORMAT_SUPPORT_SO_BUFFER;

                        if(screen->is_format_supported(screen, format, PIPE_TEXTURE_1D, 0, PIPE_BIND_SAMPLER_VIEW))
				support |= D3D11_FORMAT_SUPPORT_TEXTURE1D;
                        if(screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 0, PIPE_BIND_SAMPLER_VIEW))
				support |= D3D11_FORMAT_SUPPORT_TEXTURE2D;
                        if(screen->is_format_supported(screen, format, PIPE_TEXTURE_CUBE, 0, PIPE_BIND_SAMPLER_VIEW))
				support |= D3D11_FORMAT_SUPPORT_TEXTURECUBE;
                        if(screen->is_format_supported(screen, format, PIPE_TEXTURE_3D, 0, PIPE_BIND_SAMPLER_VIEW))
				support |= D3D11_FORMAT_SUPPORT_TEXTURE3D;

			if(support & (D3D11_FORMAT_SUPPORT_TEXTURE1D | D3D11_FORMAT_SUPPORT_TEXTURE2D |
				      D3D11_FORMAT_SUPPORT_TEXTURE3D | D3D11_FORMAT_SUPPORT_TEXTURECUBE))
			{
				support |=
					D3D11_FORMAT_SUPPORT_SHADER_LOAD |
					D3D11_FORMAT_SUPPORT_SHADER_SAMPLE |
					D3D11_FORMAT_SUPPORT_MIP |
					D3D11_FORMAT_SUPPORT_MIP_AUTOGEN;
				if(util_format_is_depth_or_stencil(format))
					support |= D3D11_FORMAT_SUPPORT_SHADER_SAMPLE_COMPARISON;
			}

			if(screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 0, PIPE_BIND_RENDER_TARGET | PIPE_BIND_BLENDABLE))
				support |= D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_RENDER_TARGET | D3D11_FORMAT_SUPPORT_BLENDABLE;
			else
			if(screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 0, PIPE_BIND_RENDER_TARGET))
				support |= D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_RENDER_TARGET;
			if(screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 0, PIPE_BIND_DEPTH_STENCIL))
				support |= D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_DEPTH_STENCIL;
			if(screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 0, PIPE_BIND_DISPLAY_TARGET))
				support |= D3D11_FORMAT_SUPPORT_DISPLAY;

			unsigned ms;
			for(ms = 2; ms <= 8; ++ms)
			{
				if(screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, ms, PIPE_BIND_RENDER_TARGET))
				{
					support |= D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET | D3D11_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE;
					break;
				}
			}
			if(ms <= 8 && screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, ms, PIPE_BIND_SAMPLER_VIEW))
				support |= D3D11_FORMAT_SUPPORT_MULTISAMPLE_LOAD;

			format_support[format] = support;
		}
		*out_format_support = support;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels(
		DXGI_FORMAT format,
		unsigned sample_count,
		unsigned *pcount
	)
	{
		SYNCHRONIZED;

		if(sample_count == 1)
			*pcount = 1;
		else
			*pcount = 0;
		return S_OK;
	}

	template<typename T, typename U>
	bool convert_blend_state(T& to, const U& from, unsigned BlendEnable, unsigned RenderTargetWriteMask)
	{
		if(unlikely(BlendEnable &&
			    (from.SrcBlend >= D3D11_BLEND_COUNT ||
			     from.SrcBlendAlpha >= D3D11_BLEND_COUNT ||
			     from.DestBlend >= D3D11_BLEND_COUNT ||
			     from.DestBlendAlpha >= D3D11_BLEND_COUNT ||
			     from.BlendOp >= 6 ||
			     from.BlendOp == 0 ||
			     from.BlendOpAlpha >= 6 ||
			     from.BlendOpAlpha == 0)))
			return false;

		to.blend_enable = BlendEnable;

		if(BlendEnable)
		{
			to.rgb_func = from.BlendOp - 1;
			to.alpha_func = from.BlendOpAlpha - 1;

			to.rgb_src_factor = d3d11_to_pipe_blend[from.SrcBlend];
			to.alpha_src_factor = d3d11_to_pipe_blend[from.SrcBlendAlpha];
			to.rgb_dst_factor = d3d11_to_pipe_blend[from.DestBlend];
			to.alpha_dst_factor = d3d11_to_pipe_blend[from.DestBlendAlpha];
		}

		to.colormask = RenderTargetWriteMask & 0xf;
		return true;
	}

#if API >= 11
	virtual HRESULT STDMETHODCALLTYPE CreateBlendState(
		const D3D11_BLEND_DESC *blend_state_desc,
		ID3D11BlendState **out_blend_state
	)
#else
	virtual HRESULT STDMETHODCALLTYPE CreateBlendState1(
		const D3D10_BLEND_DESC1 *blend_state_desc,
		ID3D10BlendState1 **out_blend_state
	)
#endif
	{
		SYNCHRONIZED;

		pipe_blend_state state;
		memset(&state, 0, sizeof(state));
		state.alpha_to_coverage = !!blend_state_desc->AlphaToCoverageEnable;
		state.independent_blend_enable = !!blend_state_desc->IndependentBlendEnable;

		assert(PIPE_MAX_COLOR_BUFS >= 8);
		const unsigned n = blend_state_desc->IndependentBlendEnable ? 8 : 1;
		for(unsigned i = 0; i < n; ++i)
		{
			 if(!convert_blend_state(
					 state.rt[i],
					 blend_state_desc->RenderTarget[i],
					 blend_state_desc->RenderTarget[i].BlendEnable,
					 blend_state_desc->RenderTarget[i].RenderTargetWriteMask))
				 return E_INVALIDARG;
		}

		if(!out_blend_state)
			return S_FALSE;

		void* object = immediate_pipe->create_blend_state(immediate_pipe, &state);
		if(!object)
			return E_FAIL;

		*out_blend_state = new GalliumD3D11BlendState(this, object, *blend_state_desc);
		return S_OK;
	}

#if API < 11
	virtual HRESULT STDMETHODCALLTYPE CreateBlendState(
		const D3D10_BLEND_DESC *blend_state_desc,
		ID3D10BlendState **out_blend_state
	)
	{
		SYNCHRONIZED;

		pipe_blend_state state;
		memset(&state, 0, sizeof(state));
		state.alpha_to_coverage = !!blend_state_desc->AlphaToCoverageEnable;
		assert(PIPE_MAX_COLOR_BUFS >= 8);
		for(unsigned i = 0; i < 8; ++i)
		{
			if(!convert_blend_state(
				state.rt[i],
				*blend_state_desc,
				blend_state_desc->BlendEnable[i],
				blend_state_desc->RenderTargetWriteMask[i]))
				return E_INVALIDARG;
		}

		for(unsigned i = 1; i < 8; ++i)
		{
			if(memcmp(&state.rt[0], &state.rt[i], sizeof(state.rt[0])))
			{
				state.independent_blend_enable = TRUE;
				break;
			}
		}

		void* object = immediate_pipe->create_blend_state(immediate_pipe, &state);
		if(!object)
			return E_FAIL;

		*out_blend_state = new GalliumD3D11BlendState(this, object, *blend_state_desc);
		return S_OK;
	}
#endif

	virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilState(
		const D3D11_DEPTH_STENCIL_DESC *depth_stencil_state_desc,
		ID3D11DepthStencilState **depth_stencil_state
	)
	{
		SYNCHRONIZED;

		pipe_depth_stencil_alpha_state state;
		memset(&state, 0, sizeof(state));

		state.depth.enabled = !!depth_stencil_state_desc->DepthEnable;
		if(depth_stencil_state_desc->DepthEnable)
		{
			if(depth_stencil_state_desc->DepthFunc == 0 ||
			   depth_stencil_state_desc->DepthFunc >= 9)
				return E_INVALIDARG;
			state.depth.writemask = depth_stencil_state_desc->DepthWriteMask;
			state.depth.func = depth_stencil_state_desc->DepthFunc - 1;
		}

		state.stencil[0].enabled = !!depth_stencil_state_desc->StencilEnable;
		if(depth_stencil_state_desc->StencilEnable)
		{
			if(depth_stencil_state_desc->FrontFace.StencilPassOp >= D3D11_STENCIL_OP_COUNT ||
			   depth_stencil_state_desc->FrontFace.StencilFailOp >= D3D11_STENCIL_OP_COUNT ||
			   depth_stencil_state_desc->FrontFace.StencilDepthFailOp >= D3D11_STENCIL_OP_COUNT ||
			   depth_stencil_state_desc->BackFace.StencilPassOp >= D3D11_STENCIL_OP_COUNT ||
			   depth_stencil_state_desc->BackFace.StencilFailOp >= D3D11_STENCIL_OP_COUNT ||
			   depth_stencil_state_desc->BackFace.StencilDepthFailOp >= D3D11_STENCIL_OP_COUNT)
				return E_INVALIDARG;
			state.stencil[0].writemask = depth_stencil_state_desc->StencilWriteMask;
			state.stencil[0].valuemask = depth_stencil_state_desc->StencilReadMask;
			state.stencil[0].zpass_op = d3d11_to_pipe_stencil_op[depth_stencil_state_desc->FrontFace.StencilPassOp];
			state.stencil[0].fail_op = d3d11_to_pipe_stencil_op[depth_stencil_state_desc->FrontFace.StencilFailOp];
			state.stencil[0].zfail_op = d3d11_to_pipe_stencil_op[depth_stencil_state_desc->FrontFace.StencilDepthFailOp];
			state.stencil[0].func = depth_stencil_state_desc->FrontFace.StencilFunc - 1;
			state.stencil[1].enabled = !!depth_stencil_state_desc->StencilEnable;
			state.stencil[1].writemask = depth_stencil_state_desc->StencilWriteMask;
			state.stencil[1].valuemask = depth_stencil_state_desc->StencilReadMask;
			state.stencil[1].zpass_op = d3d11_to_pipe_stencil_op[depth_stencil_state_desc->BackFace.StencilPassOp];
			state.stencil[1].fail_op = d3d11_to_pipe_stencil_op[depth_stencil_state_desc->BackFace.StencilFailOp];
			state.stencil[1].zfail_op = d3d11_to_pipe_stencil_op[depth_stencil_state_desc->BackFace.StencilDepthFailOp];
			state.stencil[1].func = depth_stencil_state_desc->BackFace.StencilFunc - 1;
		}

		if(!depth_stencil_state)
			return S_FALSE;

		void* object = immediate_pipe->create_depth_stencil_alpha_state(immediate_pipe, &state);
		if(!object)
			return E_FAIL;

		*depth_stencil_state = new GalliumD3D11DepthStencilState(this, object, *depth_stencil_state_desc);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState(
		const D3D11_RASTERIZER_DESC *rasterizer_desc,
		ID3D11RasterizerState **out_rasterizer_state)
	{
		SYNCHRONIZED;

		pipe_rasterizer_state state;
		memset(&state, 0, sizeof(state));
		state.gl_rasterization_rules = 1; /* D3D10/11 use GL rules */
		state.fill_front = state.fill_back = (rasterizer_desc->FillMode == D3D11_FILL_WIREFRAME) ? PIPE_POLYGON_MODE_LINE : PIPE_POLYGON_MODE_FILL;
		if(rasterizer_desc->CullMode == D3D11_CULL_FRONT)
			state.cull_face = PIPE_FACE_FRONT;
		else if(rasterizer_desc->CullMode == D3D11_CULL_BACK)
			state.cull_face = PIPE_FACE_BACK;
		else
			state.cull_face = PIPE_FACE_NONE;
		state.front_ccw = !!rasterizer_desc->FrontCounterClockwise;
		state.offset_tri = state.offset_line = state.offset_point = rasterizer_desc->SlopeScaledDepthBias || rasterizer_desc->DepthBias;
		state.offset_scale = rasterizer_desc->SlopeScaledDepthBias;
		state.offset_units = rasterizer_desc->DepthBias;
		state.offset_clamp = rasterizer_desc->DepthBiasClamp;
		state.depth_clip = rasterizer_desc->DepthClipEnable;
		state.scissor = !!rasterizer_desc->ScissorEnable;
		state.multisample = !!rasterizer_desc->MultisampleEnable;
		state.line_smooth = !!rasterizer_desc->AntialiasedLineEnable;
		state.flatshade_first = 1;
		state.line_width = 1.0f;
		state.point_size = 1.0f;

		/* TODO: is this correct? */
		state.point_quad_rasterization = 1;

		if(!out_rasterizer_state)
			return S_FALSE;

		void* object = immediate_pipe->create_rasterizer_state(immediate_pipe, &state);
		if(!object)
			return E_FAIL;

		*out_rasterizer_state = new GalliumD3D11RasterizerState(this, object, *rasterizer_desc);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateSamplerState(
		const D3D11_SAMPLER_DESC *sampler_desc,
		ID3D11SamplerState **out_sampler_state)
	{
		SYNCHRONIZED;

		pipe_sampler_state state;
		memset(&state, 0, sizeof(state));
		state.normalized_coords = 1;
		state.min_mip_filter = (sampler_desc->Filter & 1);
		state.mag_img_filter = ((sampler_desc->Filter >> 2) & 1);
		state.min_img_filter = ((sampler_desc->Filter >> 4) & 1);
		if(sampler_desc->Filter & 0x40)
			state.max_anisotropy = sampler_desc->MaxAnisotropy;
		if(sampler_desc->Filter & 0x80)
		{
			state.compare_mode = PIPE_TEX_COMPARE_R_TO_TEXTURE;
			state.compare_func = sampler_desc->ComparisonFunc - 1;
		}
		state.wrap_s = d3d11_to_pipe_wrap[sampler_desc->AddressU];
		state.wrap_t = d3d11_to_pipe_wrap[sampler_desc->AddressV];
		state.wrap_r = d3d11_to_pipe_wrap[sampler_desc->AddressW];
		state.lod_bias = sampler_desc->MipLODBias;
		memcpy(state.border_color.f, sampler_desc->BorderColor, sizeof(state.border_color));
		state.min_lod = sampler_desc->MinLOD;
		state.max_lod = sampler_desc->MaxLOD;

		if(!out_sampler_state)
			return S_FALSE;

		void* object = immediate_pipe->create_sampler_state(immediate_pipe, &state);
		if(!object)
			return E_FAIL;

		*out_sampler_state = new GalliumD3D11SamplerState(this, object, *sampler_desc);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateInputLayout(
		const D3D11_INPUT_ELEMENT_DESC *input_element_descs,
		unsigned count,
		const void *shader_bytecode_with_input_signature,
		SIZE_T bytecode_length,
		ID3D11InputLayout **out_input_layout)
	{
		SYNCHRONIZED;

		if(count > D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT)
			return E_INVALIDARG;
		assert(D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT <= PIPE_MAX_ATTRIBS);

		// putting semantics matching in the core API seems to be a (minor) design mistake

		struct dxbc_chunk_signature* sig = dxbc_find_signature(shader_bytecode_with_input_signature, bytecode_length, DXBC_FIND_INPUT_SIGNATURE);
		D3D11_SIGNATURE_PARAMETER_DESC* params;
		unsigned num_params = dxbc_parse_signature(sig, &params);

		typedef std::unordered_map<std::pair<c_string, unsigned>, unsigned> semantic_to_idx_map_t;
		semantic_to_idx_map_t semantic_to_idx_map;
		for(unsigned i = 0; i < count; ++i)
			semantic_to_idx_map[std::make_pair(c_string(input_element_descs[i].SemanticName), input_element_descs[i].SemanticIndex)] = i;

		struct pipe_vertex_element elements[D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT];

		enum pipe_format formats[D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT];
		unsigned offsets[D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT];

		offsets[0] = 0;
		for(unsigned i = 0; i < count; ++i)
		{
			formats[i] = dxgi_to_pipe_format[input_element_descs[i].Format];

			if(likely(input_element_descs[i].AlignedByteOffset != D3D11_APPEND_ALIGNED_ELEMENT))
			{
				offsets[i] = input_element_descs[i].AlignedByteOffset;
			}
			else if(i > 0)
			{
				unsigned align_mask = util_format_description(formats[i])->channel[0].size;
				if(align_mask & 7) // e.g. R10G10B10A2
					align_mask = 32;
				align_mask = (align_mask / 8) - 1;

				offsets[i] = (offsets[i - 1] + util_format_get_blocksize(formats[i - 1]) + align_mask) & ~align_mask;
			}
		}

		// TODO: check for & report errors (e.g. ambiguous layouts, unmatched semantics)

		unsigned num_params_to_use = 0;
		for(unsigned i = 0; i < num_params && num_params_to_use < D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT; ++i)
		{
			if(!strcasecmp(params[i].SemanticName, "SV_INSTANCEID") ||
			   !strcasecmp(params[i].SemanticName, "SV_VERTEXID"))
				continue;
			const unsigned n = num_params_to_use++;

			semantic_to_idx_map_t::iterator iter = semantic_to_idx_map.find(std::make_pair(c_string(params[i].SemanticName), params[i].SemanticIndex));

			if(iter != semantic_to_idx_map.end())
			{
				unsigned idx = iter->second;

				elements[n].src_format = formats[idx];
				elements[n].src_offset = offsets[idx];
				elements[n].vertex_buffer_index = input_element_descs[idx].InputSlot;
				elements[n].instance_divisor = input_element_descs[idx].InstanceDataStepRate;
				if (input_element_descs[idx].InputSlotClass == D3D11_INPUT_PER_INSTANCE_DATA)
					if (elements[n].instance_divisor == 0)
						elements[n].instance_divisor = ~0; // XXX: can't specify 'never' to gallium
			}
			else
			{
				// XXX: undefined input, is this valid or should we return an error ?
				elements[n].src_format = PIPE_FORMAT_NONE;
				elements[n].src_offset = 0;
				elements[n].vertex_buffer_index = 0;
				elements[n].instance_divisor = 0;
			}
		}

		free(params);

		if(!out_input_layout)
			return S_FALSE;

		void* object = immediate_pipe->create_vertex_elements_state(immediate_pipe, num_params_to_use, elements);
		if(!object)
			return E_FAIL;

		*out_input_layout = new GalliumD3D11InputLayout(this, object);
		return S_OK;
	}

	static unsigned d3d11_to_pipe_bind_flags(unsigned bind_flags)
	{
		unsigned bind = 0;
		if(bind_flags & D3D11_BIND_VERTEX_BUFFER)
			bind |= PIPE_BIND_VERTEX_BUFFER;
		if(bind_flags & D3D11_BIND_INDEX_BUFFER)
			bind |= PIPE_BIND_INDEX_BUFFER;
		if(bind_flags & D3D11_BIND_CONSTANT_BUFFER)
			bind |= PIPE_BIND_CONSTANT_BUFFER;
		if(bind_flags & D3D11_BIND_SHADER_RESOURCE)
			bind |= PIPE_BIND_SAMPLER_VIEW;
		if(bind_flags & D3D11_BIND_STREAM_OUTPUT)
			bind |= PIPE_BIND_STREAM_OUTPUT;
		if(bind_flags & D3D11_BIND_RENDER_TARGET)
			bind |= PIPE_BIND_RENDER_TARGET;
		if(bind_flags & D3D11_BIND_DEPTH_STENCIL)
			bind |= PIPE_BIND_DEPTH_STENCIL;
		return bind;
	}

	inline HRESULT create_resource(
		pipe_texture_target target,
		unsigned width,
		unsigned height,
		unsigned depth,
		unsigned mip_levels,
		unsigned array_size,
		DXGI_FORMAT format,
		const DXGI_SAMPLE_DESC* SampleDesc,
		D3D11_USAGE usage,
		unsigned bind_flags,
		unsigned c_p_u_access_flags,
		unsigned misc_flags,
		const D3D11_SUBRESOURCE_DATA *initial_data,
		DXGI_USAGE dxgi_usage,
		struct pipe_resource** ppresource
	)
	{
		if(invalid(format >= DXGI_FORMAT_COUNT))
			return E_INVALIDARG;
		if(misc_flags & D3D11_RESOURCE_MISC_TEXTURECUBE)
		{
			if(target != PIPE_TEXTURE_2D)
				return E_INVALIDARG;
			target = PIPE_TEXTURE_CUBE;
			if(array_size % 6)
				return E_INVALIDARG;
		}
		else if(array_size > 1)
		{
			switch (target) {
			case PIPE_TEXTURE_1D: target = PIPE_TEXTURE_1D_ARRAY; break;
			case PIPE_TEXTURE_2D: target = PIPE_TEXTURE_2D_ARRAY; break;
			default:
				return E_INVALIDARG;
			}
		}
		/* TODO: msaa */
		struct pipe_resource templat;
		memset(&templat, 0, sizeof(templat));
		templat.target = target;
		templat.width0 = width;
		templat.height0 = height;
		templat.depth0 = depth;
		templat.array_size = array_size;
		if(mip_levels)
			templat.last_level = mip_levels - 1;
		else
			templat.last_level = MAX2(MAX2(util_logbase2(templat.width0), util_logbase2(templat.height0)), util_logbase2(templat.depth0));
		templat.format = dxgi_to_pipe_format[format];
		if(bind_flags & D3D11_BIND_DEPTH_STENCIL) {
			// colour formats are not depth-renderable, but depth/stencil-formats may be colour-renderable
			switch(format)
			{
			case DXGI_FORMAT_R32_TYPELESS: templat.format = PIPE_FORMAT_Z32_FLOAT; break;
			case DXGI_FORMAT_R16_TYPELESS: templat.format = PIPE_FORMAT_Z16_UNORM; break;
			default:
				break;
			}
		}
		templat.bind = d3d11_to_pipe_bind_flags(bind_flags);
		if(c_p_u_access_flags & D3D11_CPU_ACCESS_READ)
			templat.bind |= PIPE_BIND_TRANSFER_READ;
		if(c_p_u_access_flags & D3D11_CPU_ACCESS_WRITE)
			templat.bind |= PIPE_BIND_TRANSFER_WRITE;
		if(misc_flags & D3D11_RESOURCE_MISC_SHARED)
			templat.bind |= PIPE_BIND_SHARED;
		if(misc_flags & D3D11_RESOURCE_MISC_GDI_COMPATIBLE)
			templat.bind |= PIPE_BIND_TRANSFER_READ | PIPE_BIND_TRANSFER_WRITE;
		if(dxgi_usage & DXGI_USAGE_BACK_BUFFER)
			templat.bind |= PIPE_BIND_DISPLAY_TARGET;
		templat.usage = d3d11_to_pipe_usage[usage];
		if(invalid(!templat.format))
			return E_NOTIMPL;

		if(!ppresource)
			return S_FALSE;

		struct pipe_resource* resource = screen->resource_create(screen, &templat);
		if(!resource)
			return E_FAIL;
		if(initial_data)
		{
			for(unsigned slice = 0; slice < array_size; ++slice)
			{
				for(unsigned level = 0; level <= templat.last_level; ++level)
				{
					struct pipe_box box;
					box.x = box.y = 0;
					box.z = slice;
					box.width = u_minify(width, level);
					box.height = u_minify(height, level);
					box.depth = u_minify(depth, level);
					immediate_pipe->transfer_inline_write(immediate_pipe, resource, level, PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD | PIPE_TRANSFER_UNSYNCHRONIZED, &box, initial_data->pSysMem, initial_data->SysMemPitch, initial_data->SysMemSlicePitch);
					++initial_data;
				}
			}
		}
		*ppresource = resource;
		return S_OK;
	}

	static unsigned d3d_to_dxgi_usage(unsigned bind, unsigned misc)
	{
		unsigned dxgi_usage = 0;
		if(bind |= D3D11_BIND_RENDER_TARGET)
			dxgi_usage |= DXGI_USAGE_RENDER_TARGET_OUTPUT;
		if(bind & D3D11_BIND_SHADER_RESOURCE)
			dxgi_usage |= DXGI_USAGE_SHADER_INPUT;
#if API >= 11
		if(bind & D3D11_BIND_UNORDERED_ACCESS)
			dxgi_usage |= DXGI_USAGE_UNORDERED_ACCESS;
#endif
		if(misc & D3D11_RESOURCE_MISC_SHARED)
			dxgi_usage |= DXGI_USAGE_SHARED;
		return dxgi_usage;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateTexture1D(
		const D3D11_TEXTURE1D_DESC *desc,
		const D3D11_SUBRESOURCE_DATA *initial_data,
		ID3D11Texture1D **out_texture1d)
	{
		SYNCHRONIZED;

		struct pipe_resource* resource;
		DXGI_USAGE dxgi_usage = d3d_to_dxgi_usage(desc->BindFlags, desc->MiscFlags);
		HRESULT hr = create_resource(PIPE_TEXTURE_1D, desc->Width, 1, 1, desc->MipLevels, desc->ArraySize, desc->Format, 0, desc->Usage, desc->BindFlags, desc->CPUAccessFlags, desc->MiscFlags, initial_data, dxgi_usage, out_texture1d ? &resource : 0);
		if(hr != S_OK)
			return hr;
		D3D11_TEXTURE1D_DESC cdesc = *desc;
		cdesc.MipLevels = resource->last_level + 1;
		*out_texture1d = new GalliumD3D11Texture1D(this, resource, cdesc, dxgi_usage);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateTexture2D(
		const D3D11_TEXTURE2D_DESC *desc,
		const D3D11_SUBRESOURCE_DATA *initial_data,
		ID3D11Texture2D **out_texture2d)
	{
		SYNCHRONIZED;

		struct pipe_resource* resource;
		DXGI_USAGE dxgi_usage = d3d_to_dxgi_usage(desc->BindFlags, desc->MiscFlags);
		HRESULT hr = create_resource(PIPE_TEXTURE_2D, desc->Width, desc->Height, 1, desc->MipLevels, desc->ArraySize, desc->Format, &desc->SampleDesc, desc->Usage, desc->BindFlags, desc->CPUAccessFlags, desc->MiscFlags, initial_data, dxgi_usage, out_texture2d ? &resource : 0);
		if(hr != S_OK)
			return hr;
		D3D11_TEXTURE2D_DESC cdesc = *desc;
		cdesc.MipLevels = resource->last_level + 1;
		if(cdesc.MipLevels == 1 && cdesc.ArraySize == 1)
			*out_texture2d = new GalliumD3D11Surface(this, resource, cdesc, dxgi_usage);
		else
			*out_texture2d = new GalliumD3D11Texture2D(this, resource, cdesc, dxgi_usage);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateTexture3D(
		const D3D11_TEXTURE3D_DESC *desc,
		const D3D11_SUBRESOURCE_DATA *initial_data,
		ID3D11Texture3D **out_texture3d)
	{
		SYNCHRONIZED;

		struct pipe_resource* resource;
		DXGI_USAGE dxgi_usage = d3d_to_dxgi_usage(desc->BindFlags, desc->MiscFlags);
		HRESULT hr = create_resource(PIPE_TEXTURE_3D, desc->Width, desc->Height, desc->Depth, desc->MipLevels, 1, desc->Format, 0, desc->Usage, desc->BindFlags, desc->CPUAccessFlags, desc->MiscFlags, initial_data, dxgi_usage, out_texture3d ? &resource : 0);
		if(hr != S_OK)
			return hr;
		D3D11_TEXTURE3D_DESC cdesc = *desc;
		cdesc.MipLevels = resource->last_level + 1;
		*out_texture3d = new GalliumD3D11Texture3D(this, resource, cdesc, dxgi_usage);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateBuffer(
		const D3D11_BUFFER_DESC *desc,
		const D3D11_SUBRESOURCE_DATA *initial_data,
		ID3D11Buffer **out_buffer)
	{
		SYNCHRONIZED;

		struct pipe_resource* resource;
		DXGI_USAGE dxgi_usage = d3d_to_dxgi_usage(desc->BindFlags, desc->MiscFlags);
		HRESULT hr = create_resource(PIPE_BUFFER, desc->ByteWidth, 1, 1, 1, 1, DXGI_FORMAT_R8_UNORM, 0, desc->Usage, desc->BindFlags, desc->CPUAccessFlags, desc->MiscFlags, initial_data, dxgi_usage, out_buffer ? &resource : 0);
		if(hr != S_OK)
			return hr;
		*out_buffer = new GalliumD3D11Buffer(this, resource, *desc, dxgi_usage);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OpenGalliumResource(
		struct pipe_resource* resource,
		IUnknown** dxgi_resource)
	{
		SYNCHRONIZED;

		/* TODO: maybe support others */
		assert(resource->target == PIPE_TEXTURE_2D);
		*dxgi_resource = 0;
		D3D11_TEXTURE2D_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.Width = resource->width0;
		desc.Height = resource->height0;
		init_pipe_to_dxgi_format();
		desc.Format = pipe_to_dxgi_format[resource->format];
		desc.SampleDesc.Count = resource->nr_samples;
		desc.SampleDesc.Quality = 0;
		desc.ArraySize = 1;
		desc.MipLevels = resource->last_level + 1;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		if(resource->bind & PIPE_BIND_RENDER_TARGET)
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		if(resource->bind & PIPE_BIND_DEPTH_STENCIL)
			desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
		if(resource->bind & PIPE_BIND_SAMPLER_VIEW)
			desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
		if(resource->bind & PIPE_BIND_SHARED)
			desc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED;
		DXGI_USAGE dxgi_usage = d3d_to_dxgi_usage(desc.BindFlags, desc.MiscFlags);
		if(desc.MipLevels == 1 && desc.ArraySize == 1)
			*dxgi_resource = (ID3D11Texture2D*)new GalliumD3D11Surface(this, resource, desc, dxgi_usage);
		else
			*dxgi_resource = (ID3D11Texture2D*)new GalliumD3D11Texture2D(this, resource, desc, dxgi_usage);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateSurface(
		const DXGI_SURFACE_DESC *dxgi_desc,
		unsigned count,
		DXGI_USAGE usage,
		const DXGI_SHARED_RESOURCE *shared_resource,
		IDXGISurface **out_surface)
	{
		SYNCHRONIZED;

		D3D11_TEXTURE2D_DESC desc;
		memset(&desc, 0, sizeof(desc));

		struct pipe_resource* resource;
		desc.Width = dxgi_desc->Width;
		desc.Height = dxgi_desc->Height;
		desc.Format = dxgi_desc->Format;
		desc.SampleDesc = dxgi_desc->SampleDesc;
		desc.ArraySize = count;
		desc.MipLevels = 1;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		if(usage & DXGI_USAGE_RENDER_TARGET_OUTPUT)
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		if(usage & DXGI_USAGE_SHADER_INPUT)
			desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
#if API >= 11
		if(usage & DXGI_USAGE_UNORDERED_ACCESS)
			desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
#endif
		if(usage & DXGI_USAGE_SHARED)
			desc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED;
		HRESULT hr = create_resource(PIPE_TEXTURE_2D, dxgi_desc->Width, dxgi_desc->Height, 1, 1, count, dxgi_desc->Format, &dxgi_desc->SampleDesc, D3D11_USAGE_DEFAULT, desc.BindFlags, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE, desc.MiscFlags, 0, usage, &resource);
		if(hr != S_OK)
			return hr;
		*out_surface = new GalliumD3D11Surface(this, resource, desc, usage);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView(
		ID3D11Resource *iresource,
		const D3D11_SHADER_RESOURCE_VIEW_DESC *desc,
		ID3D11ShaderResourceView **out_srv)
	{
#if API >= 11
		D3D11_SHADER_RESOURCE_VIEW_DESC def_desc;
#else
		if(desc->ViewDimension == D3D10_1_SRV_DIMENSION_TEXTURECUBEARRAY)
			return E_INVALIDARG;
		D3D10_SHADER_RESOURCE_VIEW_DESC1 desc1;
		memset(&desc1, 0, sizeof(desc1));
		memcpy(&desc1, desc, sizeof(*desc));
		return CreateShaderResourceView1(iresource, &desc1, (ID3D10ShaderResourceView1**)out_srv);
	}

	virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView1(
			ID3D11Resource *iresource,
			const D3D10_SHADER_RESOURCE_VIEW_DESC1 *desc,
			ID3D10ShaderResourceView1 **out_srv)
	{
		D3D10_SHADER_RESOURCE_VIEW_DESC1 def_desc;
#endif
		SYNCHRONIZED;

		const struct pipe_resource* resource = ((GalliumD3D11Resource<>*)iresource)->resource;

		if(!desc)
		{
			init_pipe_to_dxgi_format();
			memset(&def_desc, 0, sizeof(def_desc));
			def_desc.Format = pipe_to_dxgi_format[resource->format];
			switch(resource->target)
			{
			case PIPE_BUFFER:
				def_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
				def_desc.Buffer.ElementWidth = resource->width0;
				break;
			case PIPE_TEXTURE_1D:
				def_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
				def_desc.Texture1D.MipLevels = resource->last_level + 1;
				break;
			case PIPE_TEXTURE_1D_ARRAY:
				def_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
				def_desc.Texture1DArray.MipLevels = resource->last_level + 1;
				def_desc.Texture1DArray.ArraySize = resource->array_size;
				break;
			case PIPE_TEXTURE_2D:
			case PIPE_TEXTURE_RECT:
				def_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				def_desc.Texture2D.MipLevels = resource->last_level + 1;
				break;
			case PIPE_TEXTURE_2D_ARRAY:
				def_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				def_desc.Texture2DArray.MipLevels = resource->last_level + 1;
				def_desc.Texture2DArray.ArraySize = resource->array_size;
				break;
			case PIPE_TEXTURE_3D:
				def_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
				def_desc.Texture3D.MipLevels = resource->last_level + 1;
				break;
			case PIPE_TEXTURE_CUBE:
				if(resource->array_size > 6)
				{
					def_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
					def_desc.TextureCubeArray.NumCubes = resource->array_size / 6;
				}
				else
				{
					def_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				}
				def_desc.TextureCube.MipLevels = resource->last_level + 1;
				break;
			default:
				return E_INVALIDARG;
			}
			desc = &def_desc;
		}

		struct pipe_sampler_view templat;
		memset(&templat, 0, sizeof(templat));
		if(invalid(format >= DXGI_FORMAT_COUNT))
			return E_INVALIDARG;
		templat.format = (desc->Format == DXGI_FORMAT_UNKNOWN) ? resource->format : dxgi_to_pipe_format[desc->Format];
		if(!templat.format)
			return E_NOTIMPL;
		templat.swizzle_r = PIPE_SWIZZLE_RED;
		templat.swizzle_g = PIPE_SWIZZLE_GREEN;
		templat.swizzle_b = PIPE_SWIZZLE_BLUE;
		templat.swizzle_a = PIPE_SWIZZLE_ALPHA;

		templat.texture = ((GalliumD3D11Resource<>*)iresource)->resource;
		switch(desc->ViewDimension)
		{
		case D3D11_SRV_DIMENSION_TEXTURE1DARRAY:
		case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
		case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY:
			templat.u.tex.first_layer = desc->Texture1DArray.FirstArraySlice;
			templat.u.tex.last_layer = desc->Texture1DArray.FirstArraySlice + desc->Texture1DArray.ArraySize - 1;
			if (desc->ViewDimension == D3D11_SRV_DIMENSION_TEXTURECUBEARRAY) {
				templat.u.tex.first_layer *= 6;
				templat.u.tex.last_layer *= 6;
			}
			// fall through
		case D3D11_SRV_DIMENSION_TEXTURE1D:
		case D3D11_SRV_DIMENSION_TEXTURE2D:
		case D3D11_SRV_DIMENSION_TEXTURE3D:
		case D3D11_SRV_DIMENSION_TEXTURECUBE:
			// yes, this works for all of these types
			templat.u.tex.first_level = desc->Texture1D.MostDetailedMip;
			if(desc->Texture1D.MipLevels == (unsigned)-1)
				templat.u.tex.last_level = templat.texture->last_level;
			else
				templat.u.tex.last_level = templat.u.tex.first_level + desc->Texture1D.MipLevels - 1;
			assert(templat.u.tex.last_level >= templat.u.tex.first_level);
			break;
		case D3D11_SRV_DIMENSION_BUFFER:
			templat.u.buf.first_element = desc->Buffer.ElementOffset;
			templat.u.buf.last_element = desc->Buffer.ElementOffset + desc->Buffer.ElementWidth - 1;
			break;
		case D3D11_SRV_DIMENSION_TEXTURE2DMS:
		case D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY:
			return E_NOTIMPL;
		default:
			return E_INVALIDARG;
		}

		if(!out_srv)
			return S_FALSE;

		struct pipe_sampler_view* view = immediate_pipe->create_sampler_view(immediate_pipe, templat.texture, &templat);
		if(!view)
			return E_FAIL;
		*out_srv = new GalliumD3D11ShaderResourceView(this, (GalliumD3D11Resource<>*)iresource, view, *desc);
		return S_OK;
	}

#if API >= 11
	virtual HRESULT STDMETHODCALLTYPE CreateUnorderedAccessView(
		ID3D11Resource *resource,
		const D3D11_UNORDERED_ACCESS_VIEW_DESC *desc,
		ID3D11UnorderedAccessView **out_uav)
	{
		SYNCHRONIZED;

		return E_NOTIMPL;

		// remember to return S_FALSE and not crash if out_u_a_view == 0 and parameters are valid
	}
#endif

	virtual HRESULT STDMETHODCALLTYPE CreateRenderTargetView(
		ID3D11Resource *iresource,
		const D3D11_RENDER_TARGET_VIEW_DESC *desc,
		ID3D11RenderTargetView **out_rtv)
	{
		SYNCHRONIZED;

		const struct pipe_resource* resource = ((GalliumD3D11Resource<>*)iresource)->resource;

		D3D11_RENDER_TARGET_VIEW_DESC def_desc;
		if(!desc)
		{
			init_pipe_to_dxgi_format();
			memset(&def_desc, 0, sizeof(def_desc));
			def_desc.Format = pipe_to_dxgi_format[resource->format];
			switch(resource->target)
			{
			case PIPE_BUFFER:
				def_desc.ViewDimension = D3D11_RTV_DIMENSION_BUFFER;
				def_desc.Buffer.ElementWidth = resource->width0;
				break;
			case PIPE_TEXTURE_1D:
				def_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
				break;
			case PIPE_TEXTURE_1D_ARRAY:
				def_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
				def_desc.Texture1DArray.ArraySize = resource->array_size;
				break;
			case PIPE_TEXTURE_2D:
			case PIPE_TEXTURE_RECT:
				def_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				break;
			case PIPE_TEXTURE_2D_ARRAY:
				def_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
				def_desc.Texture2DArray.ArraySize = resource->array_size;
				break;
			case PIPE_TEXTURE_3D:
				def_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
				def_desc.Texture3D.WSize = resource->depth0;
				break;
			case PIPE_TEXTURE_CUBE:
				def_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
				def_desc.Texture2DArray.ArraySize = 6;
				break;
			default:
				return E_INVALIDARG;
			}
			desc = &def_desc;
		}

		struct pipe_surface templat;
		memset(&templat, 0, sizeof(templat));
		if(invalid(desc->format >= DXGI_FORMAT_COUNT))
			return E_INVALIDARG;
		templat.format = (desc->Format == DXGI_FORMAT_UNKNOWN) ? resource->format : dxgi_to_pipe_format[desc->Format];
		if(!templat.format)
			return E_NOTIMPL;
		templat.usage = PIPE_BIND_RENDER_TARGET;
		templat.texture = ((GalliumD3D11Resource<>*)iresource)->resource;

		switch(desc->ViewDimension)
		{
		case D3D11_RTV_DIMENSION_TEXTURE1D:
		case D3D11_RTV_DIMENSION_TEXTURE2D:
			templat.u.tex.level = desc->Texture1D.MipSlice;
			break;
		case D3D11_RTV_DIMENSION_TEXTURE3D:
			templat.u.tex.level = desc->Texture3D.MipSlice;
			templat.u.tex.first_layer = desc->Texture3D.FirstWSlice;
			templat.u.tex.last_layer = desc->Texture3D.FirstWSlice + desc->Texture3D.WSize - 1;
			break;
		case D3D11_RTV_DIMENSION_TEXTURE1DARRAY:
		case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
			templat.u.tex.level = desc->Texture1DArray.MipSlice;
			templat.u.tex.first_layer = desc->Texture1DArray.FirstArraySlice;
			templat.u.tex.last_layer = desc->Texture1DArray.FirstArraySlice + desc->Texture1DArray.ArraySize - 1;
			break;
		case D3D11_RTV_DIMENSION_BUFFER:
			templat.u.buf.first_element = desc->Buffer.ElementOffset;
			templat.u.buf.last_element = desc->Buffer.ElementOffset + desc->Buffer.ElementWidth - 1;
			break;
		case D3D11_RTV_DIMENSION_TEXTURE2DMS:
		case D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:
			return E_NOTIMPL;
		default:
			return E_INVALIDARG;
		}

		if(!out_rtv)
			return S_FALSE;

		struct pipe_surface* surface = immediate_pipe->create_surface(immediate_pipe, templat.texture, &templat);
		if(!surface)
			return E_FAIL;
		*out_rtv = new GalliumD3D11RenderTargetView(this, (GalliumD3D11Resource<>*)iresource, surface, *desc);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilView(
		ID3D11Resource *iresource,
		const D3D11_DEPTH_STENCIL_VIEW_DESC *desc,
		ID3D11DepthStencilView **out_depth_stencil_view)
	{
		SYNCHRONIZED;

		const struct pipe_resource* resource = ((GalliumD3D11Resource<>*)iresource)->resource;

		D3D11_DEPTH_STENCIL_VIEW_DESC def_desc;
		if(!desc)
		{
			init_pipe_to_dxgi_format();
			memset(&def_desc, 0, sizeof(def_desc));
			def_desc.Format = pipe_to_dxgi_format[resource->format];
			switch(resource->target)
			{
			case PIPE_TEXTURE_1D:
				def_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
				break;
			case PIPE_TEXTURE_1D_ARRAY:
				def_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
				def_desc.Texture1DArray.ArraySize = resource->array_size;
				break;
			case PIPE_TEXTURE_2D:
			case PIPE_TEXTURE_RECT:
				def_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				break;
			case PIPE_TEXTURE_2D_ARRAY:
				def_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				def_desc.Texture2DArray.ArraySize = resource->array_size;
				break;
			case PIPE_TEXTURE_CUBE:
				def_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				def_desc.Texture2DArray.ArraySize = 6;
				break;
			default:
				return E_INVALIDARG;
			}
			desc = &def_desc;
		}

		struct pipe_surface templat;
		memset(&templat, 0, sizeof(templat));
		if(invalid(desc->format >= DXGI_FORMAT_COUNT))
			return E_INVALIDARG;
		templat.format = (desc->Format == DXGI_FORMAT_UNKNOWN) ? resource->format : dxgi_to_pipe_format[desc->Format];
		if(!templat.format)
			return E_NOTIMPL;
		templat.usage = PIPE_BIND_DEPTH_STENCIL;
		templat.texture = ((GalliumD3D11Resource<>*)iresource)->resource;

		switch(desc->ViewDimension)
		{
		case D3D11_DSV_DIMENSION_TEXTURE1D:
		case D3D11_DSV_DIMENSION_TEXTURE2D:
			templat.u.tex.level = desc->Texture1D.MipSlice;
			break;
		case D3D11_DSV_DIMENSION_TEXTURE1DARRAY:
		case D3D11_DSV_DIMENSION_TEXTURE2DARRAY:
			templat.u.tex.level = desc->Texture1DArray.MipSlice;
			templat.u.tex.first_layer = desc->Texture1DArray.FirstArraySlice;
			templat.u.tex.last_layer = desc->Texture1DArray.FirstArraySlice + desc->Texture1DArray.ArraySize - 1;
			break;
		case D3D11_DSV_DIMENSION_TEXTURE2DMS:
		case D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY:
			return E_NOTIMPL;
		default:
			return E_INVALIDARG;
		}

		if(!out_depth_stencil_view)
			return S_FALSE;

		struct pipe_surface* surface = immediate_pipe->create_surface(immediate_pipe, templat.texture, &templat);
		if(!surface)
			return E_FAIL;
		*out_depth_stencil_view = new GalliumD3D11DepthStencilView(this, (GalliumD3D11Resource<>*)iresource, surface, *desc);
		return S_OK;
	}

#define D3D1X_SHVER_GEOMETRY_SHADER 2 /* D3D11_SHVER_GEOMETRY_SHADER */

	GalliumD3D11Shader<>* create_stage_shader(unsigned type, const void* shader_bytecode, SIZE_T bytecode_length
#if API >= 11
			, ID3D11ClassLinkage *class_linkage
#endif
			, struct pipe_stream_output_info* so_info)
	{
		bool dump = debug_get_option_dump_shaders();

		std::auto_ptr<sm4_program> sm4(0);

		dxbc_chunk_header* sm4_chunk = dxbc_find_shader_bytecode(shader_bytecode, bytecode_length);
		if(!sm4_chunk)
		{
			if(so_info)
				sm4.reset(new sm4_program());
		}
		else
		{
			sm4.reset(sm4_parse(sm4_chunk + 1, bswap_le32(sm4_chunk->size)));
			// check if this is a dummy GS, in which case we only need a place to store the signature
			if(sm4.get() && so_info && sm4->version.type != D3D1X_SHVER_GEOMETRY_SHADER)
				sm4.reset(new sm4_program());
		}
		if(!sm4.get())
			return 0;

		if(dump)
			sm4->dump();

		struct dxbc_chunk_signature* sig;

		sig = dxbc_find_signature(shader_bytecode, bytecode_length, DXBC_FIND_INPUT_SIGNATURE);
		if(sig)
			sm4->num_params_in = dxbc_parse_signature(sig, &sm4->params_in);

		sig = dxbc_find_signature(shader_bytecode, bytecode_length, DXBC_FIND_OUTPUT_SIGNATURE);
		if(sig)
			sm4->num_params_out = dxbc_parse_signature(sig, &sm4->params_out);

		sig = dxbc_find_signature(shader_bytecode, bytecode_length, DXBC_FIND_PATCH_SIGNATURE);
		if(sig)
			sm4->num_params_patch = dxbc_parse_signature(sig, &sm4->params_patch);

		struct pipe_shader_state tgsi_shader;
		memset(&tgsi_shader, 0, sizeof(tgsi_shader));
		if(so_info)
			memcpy(&tgsi_shader.stream_output, so_info, sizeof(tgsi_shader.stream_output));

		if(so_info && sm4->version.type != D3D1X_SHVER_GEOMETRY_SHADER)
			tgsi_shader.tokens = (const tgsi_token*)sm4_to_tgsi_linkage_only(*sm4);
		else
			tgsi_shader.tokens = (const tgsi_token*)sm4_to_tgsi(*sm4);
		if(!tgsi_shader.tokens)
			return 0;

		if(dump)
			tgsi_dump(tgsi_shader.tokens, 0);

		void* shader_cso;
		GalliumD3D11Shader<>* shader;

		switch(type)
		{
		case PIPE_SHADER_VERTEX:
			shader_cso = immediate_pipe->create_vs_state(immediate_pipe, &tgsi_shader);
			shader = (GalliumD3D11Shader<>*)new GalliumD3D11VertexShader(this, shader_cso);
			break;
		case PIPE_SHADER_FRAGMENT:
			shader_cso = immediate_pipe->create_fs_state(immediate_pipe, &tgsi_shader);
			shader = (GalliumD3D11Shader<>*)new GalliumD3D11PixelShader(this, shader_cso);
			break;
		case PIPE_SHADER_GEOMETRY:
			shader_cso = immediate_pipe->create_gs_state(immediate_pipe, &tgsi_shader);
			shader = (GalliumD3D11Shader<>*)new GalliumD3D11GeometryShader(this, shader_cso);
			break;
		default:
			shader_cso = 0;
			shader = 0;
			break;
		}

		free((void*)tgsi_shader.tokens);
		return shader;
	}

#if API >= 11
#define CREATE_SHADER_ARGS \
	const void *shader_bytecode, \
	SIZE_T bytecode_length, \
	ID3D11ClassLinkage *class_linkage
#define PASS_SHADER_ARGS shader_bytecode, bytecode_length, class_linkage
#else
#define CREATE_SHADER_ARGS \
	const void *shader_bytecode, \
	SIZE_T bytecode_length
#define PASS_SHADER_ARGS shader_bytecode, bytecode_length
#endif

#define IMPLEMENT_CREATE_SHADER(Stage, GALLIUM) \
	virtual HRESULT STDMETHODCALLTYPE Create##Stage##Shader( \
		CREATE_SHADER_ARGS, \
		ID3D11##Stage##Shader **out_shader) \
	{ \
		SYNCHRONIZED; \
		GalliumD3D11##Stage##Shader* shader = (GalliumD3D11##Stage##Shader*)create_stage_shader(PIPE_SHADER_##GALLIUM, PASS_SHADER_ARGS, NULL); \
		if(!shader) \
			return E_FAIL; \
		if(out_shader) \
		{ \
			*out_shader = shader; \
			return S_OK; \
		} \
		else \
		{ \
			shader->Release(); \
			return S_FALSE; \
		} \
	}

#define IMPLEMENT_NOTIMPL_CREATE_SHADER(Stage) \
	virtual HRESULT STDMETHODCALLTYPE Create##Stage##Shader( \
		CREATE_SHADER_ARGS, \
		ID3D11##Stage##Shader **out_shader) \
	{ \
		return E_NOTIMPL; \
	}

	IMPLEMENT_CREATE_SHADER(Vertex, VERTEX)
	IMPLEMENT_CREATE_SHADER(Pixel, FRAGMENT)
	IMPLEMENT_CREATE_SHADER(Geometry, GEOMETRY)
#if API >= 11
	IMPLEMENT_NOTIMPL_CREATE_SHADER(Hull)
	IMPLEMENT_NOTIMPL_CREATE_SHADER(Domain)
	IMPLEMENT_NOTIMPL_CREATE_SHADER(Compute)
#endif

	virtual HRESULT STDMETHODCALLTYPE CreateGeometryShaderWithStreamOutput(
		const void *shader_bytecode,
		SIZE_T bytecode_length,
		const D3D11_SO_DECLARATION_ENTRY *so_declaration,
		unsigned num_entries,
#if API >= 11
		const unsigned *buffer_strides,
		unsigned num_strides,
		unsigned rasterized_stream,
		ID3D11ClassLinkage *class_linkage,
#else
		UINT output_stream_stride,
#endif
		ID3D11GeometryShader **out_geometry_shader)
	{
		SYNCHRONIZED;
		GalliumD3D11GeometryShader* gs;

#if API >= 11
		if(rasterized_stream != 0)
			return E_NOTIMPL; // not yet supported by gallium
#endif
		struct dxbc_chunk_signature* sig = dxbc_find_signature(shader_bytecode, bytecode_length, DXBC_FIND_OUTPUT_SIGNATURE);
		if(!sig)
			return E_INVALIDARG;
		D3D11_SIGNATURE_PARAMETER_DESC* out;
		unsigned num_outputs = dxbc_parse_signature(sig, &out);

		struct pipe_stream_output_info so;
		memset(&so, 0, sizeof(so));

#if API >= 11
		if(num_strides)
			so.stride = buffer_strides[0];
		if(num_strides > 1)
			debug_printf("Warning: multiple user-specified strides not implemented !\n");
#else
		so.stride = output_stream_stride;
#endif

		for(unsigned i = 0; i < num_entries; ++i)
		{
			unsigned j;
			for(j = 0; j < num_outputs; ++j)
				if(out[j].SemanticIndex == so_declaration[i].SemanticIndex && !strcasecmp(out[j].SemanticName, so_declaration[i].SemanticName))
					break;
			if(j >= num_outputs)
				continue;
			const int first_comp = ffs(out[j].Mask) - 1 + so_declaration[i].StartComponent;
			so.output[i].output_buffer = so_declaration[i].OutputSlot;
			so.output[i].register_index = out[j].Register;
			so.output[i].register_mask = ((1 << so_declaration[i].ComponentCount) - 1) << first_comp;
			++so.num_outputs;
		}
		if(out)
			free(out);

		gs = reinterpret_cast<GalliumD3D11GeometryShader*>(create_stage_shader(PIPE_SHADER_GEOMETRY, PASS_SHADER_ARGS, &so));
		if(!gs)
			return E_FAIL;

		if(!out_geometry_shader) {
			gs->Release();
			return S_FALSE;
		}
		*out_geometry_shader = gs;

		return S_OK;
	}

#if API >= 11
	virtual HRESULT STDMETHODCALLTYPE CreateClassLinkage(
		ID3D11ClassLinkage **out_linkage)
	{
		SYNCHRONIZED;

		return E_NOTIMPL;
	}
#endif

	virtual HRESULT STDMETHODCALLTYPE CreateQuery(
		const D3D11_QUERY_DESC *query_desc,
		ID3D11Query **out_query)
	{
		SYNCHRONIZED;

		if(invalid(query_desc->Query >= D3D11_QUERY_COUNT))
			return E_INVALIDARG;
		unsigned query_type = d3d11_to_pipe_query[query_desc->Query];
		if(query_type >= PIPE_QUERY_TYPES)
			return E_NOTIMPL;

		if(!out_query)
			return S_FALSE;

		struct pipe_query* query = immediate_pipe->create_query(immediate_pipe, query_type);
		if(!query)
			return E_FAIL;

		*out_query = new GalliumD3D11Query(this, query, d3d11_query_size[query_desc->Query], *query_desc);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreatePredicate(
		const D3D11_QUERY_DESC *predicate_desc,
		ID3D11Predicate **out_predicate)
	{
		SYNCHRONIZED;

		unsigned query_type;
		switch(predicate_desc->Query)
		{
		case D3D11_QUERY_SO_OVERFLOW_PREDICATE:
			query_type = PIPE_QUERY_SO_OVERFLOW_PREDICATE;
			break;
		case D3D11_QUERY_OCCLUSION_PREDICATE:
			query_type = PIPE_QUERY_OCCLUSION_PREDICATE;
			break;
		default:
			return E_INVALIDARG;
		}

		if(out_predicate)
			return S_FALSE;

		struct pipe_query* query = immediate_pipe->create_query(immediate_pipe, query_type);
		if(!query)
			return E_FAIL;

		*out_predicate = new GalliumD3D11Predicate(this, query, sizeof(BOOL), *predicate_desc);
		return S_OK;
	}


	virtual HRESULT STDMETHODCALLTYPE CreateCounter(
		const D3D11_COUNTER_DESC *counter_desc,
		ID3D11Counter **out_counter)
	{
		SYNCHRONIZED;

		return E_NOTIMPL;

		// remember to return S_FALSE if out_counter == NULL and everything is OK
	}

#if API >= 11
	virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext(
		unsigned context_flags,
		ID3D11DeviceContext **out_deferred_context)
	{
		SYNCHRONIZED;

		// TODO: this will have to be implemented using a new Gallium util module
		return E_NOTIMPL;

		// remember to return S_FALSE if out_counter == NULL and everything is OK
	}
#endif

	virtual HRESULT STDMETHODCALLTYPE OpenSharedResource(
			HANDLE resource,
			REFIID iid,
			void **out_resource)
	{
		SYNCHRONIZED;

		// TODO: the problem here is that we need to communicate dimensions somehow
		return E_NOTIMPL;

		// remember to return S_FALSE if out_counter == NULL and everything is OK
#if 0
		struct pipe_resou	rce templat;
		struct winsys_handle handle;
		handle.stride = 0;
		handle.handle = resource;
		handle.type = DRM_API_HANDLE_TYPE_SHARED;
		screen->resource_from_handle(screen, &templat, &handle);
#endif
	}

#if API < 11
	/* these are documented as "Not implemented".
	 * According to the UMDDI documentation, they apparently turn on a
	 * (width + 1) x (height + 1) convolution filter for 1-bit textures.
	 * Probably nothing uses these, assuming it has ever been implemented anywhere.
	 */
	void STDMETHODCALLTYPE SetTextFilterSize(
		UINT width,
		UINT height
	)
	{}

	virtual void STDMETHODCALLTYPE GetTextFilterSize(
		UINT *width,
		UINT *height
	)
	{}
#endif

#if API >= 11
	virtual void STDMETHODCALLTYPE RestoreGalliumState()
	{
		GalliumD3D11ImmediateDeviceContext_RestoreGalliumState(immediate_context);
	}

	virtual void STDMETHODCALLTYPE RestoreGalliumStateBlitOnly()
	{
		GalliumD3D11ImmediateDeviceContext_RestoreGalliumStateBlitOnly(immediate_context);
	}
#endif

	virtual struct pipe_context* STDMETHODCALLTYPE GetGalliumContext(void)
	{
		return immediate_pipe;
	}

#undef SYNCHRONIZED
};
