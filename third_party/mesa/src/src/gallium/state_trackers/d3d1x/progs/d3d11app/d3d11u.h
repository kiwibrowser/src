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

#include <vector>

#include "d3d11blit.hlsl.ps.h"
#include "d3d11blit.hlsl.vs.h"

template<typename index_type = unsigned>
struct triangle_list_indices : public std::vector<index_type>
{
	unsigned base;
	bool flip;

	triangle_list_indices()
	: base(0), flip(false)
	{}

	void poly(unsigned a, unsigned b, unsigned c)
	{
		this->push_back(base + a);
		this->push_back(base + (flip ? c : b));
		this->push_back(base + (flip ? b : c));
	}

	void poly(unsigned a, unsigned b, unsigned c, unsigned d)
	{
		poly(a, b, c);
		poly(a, c, d);
	}

	void poly(unsigned a, unsigned b, unsigned c, unsigned d, unsigned e)
	{
		poly(a, b, c, d);
		poly(a, d, e);
	}

	void poly(unsigned a, unsigned b, unsigned c, unsigned d, unsigned e, unsigned f)
	{
		poly(a, b, c, d, e);
		poly(a, e, f);
	}

	void poly(unsigned a, unsigned b, unsigned c, unsigned d, unsigned e, unsigned f, unsigned g)
	{
		poly(a, b, c, d, e, f);
		poly(a, f, g);
	}

	void poly(unsigned a, unsigned b, unsigned c, unsigned d, unsigned e, unsigned f, unsigned g, unsigned h)
	{
		poly(a, b, c, d, e, f, g);
		poly(a, g, h);
	}
};

struct mesh
{
	ID3D11InputLayout* layout;
	ID3D11Buffer* buffer;
	D3D11_PRIMITIVE_TOPOLOGY topology;
	unsigned vertex_size;
	unsigned draw_count;
	DXGI_FORMAT index_format;
	unsigned index_offset;

	mesh(ID3D11Device* dev, D3D11_PRIMITIVE_TOPOLOGY topology,
		const D3D11_INPUT_ELEMENT_DESC *elements, unsigned num_elements,
		const void* vs, unsigned vs_size,
		const void* vertices, unsigned vertex_size, unsigned num_vertices,
		const void* indices = 0, unsigned index_size = 0, unsigned num_indices = 0)
		: topology(topology), vertex_size(vertex_size), draw_count(index_size ? num_indices : num_vertices)
	{
		dev->CreateInputLayout(elements, num_elements, vs, vs_size, &layout);
		if(index_size == 2)
			index_format = DXGI_FORMAT_R16_UINT;
		else if(index_size == 4)
			index_format = DXGI_FORMAT_R32_UINT;
		else
			index_format = DXGI_FORMAT_UNKNOWN;
		this->vertex_size = vertex_size;
		index_offset = vertex_size * num_vertices;

	 	D3D11_BUFFER_DESC bufferd;
		memset(&bufferd, 0, sizeof(bufferd));
		bufferd.Usage = D3D11_USAGE_IMMUTABLE;
		bufferd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		if(index_format)
			bufferd.BindFlags |= D3D11_BIND_INDEX_BUFFER;
		bufferd.ByteWidth = index_offset + index_format * num_indices;

		char* data = (char*)malloc(bufferd.ByteWidth);
		memcpy(data, vertices, vertex_size * num_vertices);
		memcpy(data + index_offset, indices, index_size * num_indices);

		D3D11_SUBRESOURCE_DATA buffersd;
		buffersd.pSysMem = data;

		ensure(dev->CreateBuffer(&bufferd, &buffersd, &buffer));
		free(data);
	}

	~mesh()
	{
		layout->Release();
		buffer->Release();
	}

	void bind(ID3D11DeviceContext* ctx)
	{
		unsigned offset = 0;
		ctx->IASetPrimitiveTopology(topology);
		ctx->IASetInputLayout(layout);
		if(index_format)
			ctx->IASetIndexBuffer(buffer, index_format, index_offset);
		ctx->IASetVertexBuffers(0, 1, &buffer, &vertex_size, &offset);
	}

	void draw_bound(ID3D11DeviceContext* ctx)
	{
		if(index_format)
			ctx->DrawIndexed(draw_count, 0, 0);
		else
			ctx->Draw(draw_count, 0);
	}

	void bind_and_draw(ID3D11DeviceContext* ctx)
	{
		bind(ctx);
		draw_bound(ctx);
	}
};

mesh* create_tex_quad(ID3D11Device* dev, const BYTE* vs, unsigned vs_size)
{
	float quad_data[] = {
		-1, -1, 0, 1,
		-1, 1, 0, 0,
		1, -1, 1, 1,
		1, 1, 1, 0,
	};

	D3D11_INPUT_ELEMENT_DESC elements[2] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	return new mesh(dev, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
		elements, 2,
		vs, vs_size,
		quad_data, 4 * sizeof(float), 4,
		0, 0, 0);
}

struct d3d11_blitter
{
	mesh* quad;
	ID3D11VertexShader* vs;
	ID3D11PixelShader* ps;
	ID3D11SamplerState* sampler[2];

	d3d11_blitter(ID3D11Device* dev)
	{
		quad = create_tex_quad(dev, g_vs_blit, sizeof(g_vs_blit));

		dev->CreateVertexShader(g_vs_blit, sizeof(g_vs_blit), 0, &vs);
		dev->CreatePixelShader(g_ps_blit, sizeof(g_ps_blit), 0, &ps);

		for(unsigned i = 0; i < 2; ++i)
		{
			D3D11_SAMPLER_DESC samplerd;
			memset(&samplerd, 0, sizeof(samplerd));
			samplerd.Filter = i ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			samplerd.AddressU = samplerd.AddressV = samplerd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			dev->CreateSamplerState(&samplerd, &sampler[i]);
		}
	}

	void bind(ID3D11DeviceContext* ctx, ID3D11ShaderResourceView* srv, ID3D11RenderTargetView* rtv, float x, float y, float width, float height, bool linear)
	{
		D3D11_VIEWPORT vp;
		vp.TopLeftX = x;
		vp.TopLeftY = y;
		vp.Width = width;
		vp.Height = height;
		vp.MinDepth = 0;
		vp.MaxDepth = 1;
		ctx->RSSetViewports(1, &vp);
		ctx->RSSetState(0);
		ctx->OMSetBlendState(0, 0, ~0);
		ctx->OMSetDepthStencilState(0, 0);
		ctx->OMSetRenderTargets(1, &rtv, 0);
		ctx->VSSetShader(vs, 0, 0);
		ctx->PSSetShader(ps, 0, 0);
		ctx->PSSetShaderResources(0, 1, &srv);
		ctx->PSSetSamplers(0, 1, &sampler[!!linear]);
		quad->bind(ctx);
	}

	void draw_bound(ID3D11DeviceContext* ctx)
	{
		quad->draw_bound(ctx);
	}

	void bind_draw_and_unbind(ID3D11DeviceContext* ctx, ID3D11ShaderResourceView* srv, ID3D11RenderTargetView* rtv, float x, float y, float width, float height, bool linear)
	{
		bind(ctx, srv, rtv, x, y, width, height, linear);
		draw_bound(ctx);
		unbind(ctx);
	}

	void unbind(ID3D11DeviceContext* ctx)
	{
		void* null = 0;
		ctx->PSSetShaderResources(0, 1, (ID3D11ShaderResourceView**)&null);
		ctx->PSSetSamplers(0, 1, (ID3D11SamplerState**)&null);
	}
};

template<typename T, unsigned n>
struct vec_t
{
	T v[n];

	T& operator [](unsigned i)
	{
		return v[i];
	}

	const T& operator [](unsigned i) const
	{
		return v[i];
	}
};

template<typename T, unsigned n>
vec_t<T, n> operator -(const vec_t<T, n> a)
{
	vec_t<T, n> r;
	for(unsigned i = 0; i < n; ++i)
		r[i] = -a[i];
	return r;
}

template<typename T, unsigned n>
vec_t<T, n> operator +(const vec_t<T, n>& a, const vec_t<T, n>& b)
{
	vec_t<T, n> r;
	for(unsigned i = 0; i < n; ++i)
		r[i] = a[i] + b[i];
	return r;
}

template<typename T, unsigned n>
vec_t<T, n>& operator +=(vec_t<T, n>& a, const vec_t<T, n>& b)
{
	for(unsigned i = 0; i < n; ++i)
		a[i] += b[i];
	return a;
}

template<typename T, unsigned r, unsigned c>
struct mat_t : public vec_t<vec_t<T, r>, c>
{};

template<typename T, unsigned n>
vec_t<T, n> operator *(const vec_t<T, n>& a, const T& b)
{
	vec_t<T, n> r;
	for(unsigned i = 0; i < n; ++i)
		r[i] = a[i] * b;
	return r;
}

template<typename T, unsigned n>
vec_t<T, n> operator *(const T& b, const vec_t<T, n>& a)
{
	vec_t<T, n> r;
	for(unsigned i = 0; i < n; ++i)
		r[i] = a[i] * b;
	return r;
}

template<typename T, unsigned d, unsigned e>
vec_t<T, e> operator *(const mat_t<T, e, d>& m, const vec_t<T, d>& b)
{
	vec_t<T, e> r;
	r = m[0] * b[0];
	for(unsigned i = 1; i < d; ++i)
		r += m[i] * b[i];
	return r;
}

template<typename T, unsigned d, unsigned e, unsigned f>
mat_t<T, e, f> operator *(const mat_t<T, e, d>& m, const mat_t<T, d, f>& b)
{
	mat_t<T, e, f> r;
	for(unsigned i = 0; i < d; ++i)
		r[i] = m * b[i];
	return r;
}

template<typename T>
vec_t<T, 3> vec(T a, T b, T c)
{
	vec_t<T, 4> v;
	v[0] = a;
	v[1] = b;
	v[2] = c;
	return v;
}

template<typename T>
vec_t<T, 4> vec(T a, T b, T c, T d)
{
	vec_t<T, 4> v;
	v[0] = a;
	v[1] = b;
	v[2] = c;
	v[3] = d;
	return v;
}

typedef mat_t<float, 4, 4> float4x4;
typedef mat_t<float, 4, 3> float4x3;
typedef mat_t<float, 3, 4> float3x4;
typedef mat_t<float, 3, 3> float3x3;

typedef vec_t<float, 3> float3;
typedef vec_t<float, 4> float4;

template<typename T>
mat_t<T, 4, 4> mat4x4_frustum(T left, T right, T bottom, T top, T nearval, T farval)
{
	T x = (2.0f * nearval) / (right - left);
	T y = (2.0f * nearval) / (top - bottom);
	T a = (right + left) / (right - left);
	T b = (top + bottom) / (top - bottom);
	T c = -(farval + nearval) / (farval - nearval);
	T d = -(2.0f * farval * nearval) / (farval - nearval);
	T _0 = (T)0;

	mat_t<T, 4, 4> m;	
	m[0] = vec(x, _0, _0, _0);
	m[1] = vec(_0, y, _0, _0);
	m[2] = vec(a, b, c, (T)-1);
	m[3] = vec(_0, _0, d, _0);
	return m;
}

template<typename T>
mat_t<T, 3, 3> mat3x3_diag(T v)
{
	mat_t<T, 3, 3> m;
	T _0 = (T)0;
	m[0] = vec(v, _0, _0);
	m[1] = vec(_0, v, _0);
	m[2] = vec(_0, _0, v);
	return m;
}

template<typename T>
mat_t<T, 4, 4> mat4x4_diag(T v)
{
	mat_t<T, 4, 4> m;
	T _0 = (T)0;
	m[0] = vec(v, _0, _0, _0);
	m[1] = vec(_0, v, _0, _0);
	m[2] = vec(_0, _0, v, _0);
	m[3] = vec(_0, _0, _0, v);
	return m;
}

template<typename T, unsigned n>
mat_t<T, n, n> mat_push_rotate(const mat_t<T, n, n>& m, unsigned axis, T angle)
{
	T s = (T)sin(angle);
	T c = (T)cos(angle);

	mat_t<T, n, n> r = m;
	unsigned a = (axis + 1) % 3;
	unsigned b = (axis + 2) % 3;
	r[a] = (m[a] * c) + (m[b] * s);
	r[b] = -(m[a] * s) + (m[b] * c);
	return r;
}

template<typename T, unsigned n>
mat_t<T, n, n> mat_push_translate(const mat_t<T, n, n>& m, float x, float y, float z)
{
	mat_t<T, n, n> r = m;
	vec_t<T, n> v;
	v[0] = x;
	v[1] = y;
	v[2] = z;
	if(n >= 4)
		v[3] = (T)0;
	r[3] += m * v;
	return r;
}
