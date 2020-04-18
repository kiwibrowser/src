/*
* Copyright (C) 1999-2001 Brian Paul All Rights Reserved.
* Copyright (C) 2009-2010 Luca Barbieri All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*.
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
* AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
* This is a port of the infamous "glxgears" demo to straight EGL
* Port by Dane Rushton 10 July 2005
*
* This a rewrite of the 'eglgears' demo in straight Gallium
* Port by Luca Barbieri
*
* This a port of the 'galliumgears' demo to Direct3D 11
* Port by Luca Barbieri
*/

#define _USE_MATH_DEFINES
#include "d3d11app.h"
#include "d3d11u.h"
#include "d3d11gears.hlsl.ps.h"
#include "d3d11gears.hlsl.vs.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

struct gear
{
	struct mesh* mesh;
	float x;
	float y;
	float t0;
	float wmul;
	float4 color;
};

struct cbuf_t
{
	float4x4 projection;
	float4x4 modelview;
	float4 light;
	float4 diffuse;
	float4 specular;
	float specular_power;
	float padding[3];
};

struct gear gears[3];

struct vertex
{
	float position[3];
	float normal[3];

	vertex(float x, float y, float z, float nx, float ny, float nz)
	{
		position[0] = x;
		position[1] = y;
		position[2] = z;
		normal[0] = nx;
		normal[1] = ny;
		normal[2] = nz;
	}
};

#define VERT(x, y, z) vertices.push_back(vertex((x), (y), (z), (nx), (ny), (nz)))

static mesh* build_gear(ID3D11Device* dev, int triangle_budget, float inner_radius, float outer_radius, float width, int teeth, float tooth_depth)
{
	int i, j, k;
	float r0, r1, r2;
	float da;
	float nx, ny, nz;
	int face;
	int segs = 4;
	int base_triangles = teeth * segs * 2 * 2;
	int divs0 = (triangle_budget / base_triangles) - 1;
	int divs = (divs0 > 0) ? divs0 : 1;
	float* c = (float*)malloc(teeth * segs * sizeof(float));
	float* s = (float*)malloc(teeth * segs * sizeof(float));
	float* dc = (float*)malloc(teeth * segs * divs * sizeof(float));
	float* ds = (float*)malloc(teeth * segs * divs * sizeof(float));
	int num_vertices = teeth * segs * 2 * (3 + 2 * divs);
	int num_triangles = base_triangles * (1 + divs);
	printf("Creating gear with %i teeth using %i vertices used in %i triangles\n", teeth, num_vertices, num_triangles);
	triangle_list_indices<> indices;
	std::vector<vertex> vertices;

	r0 = inner_radius;
	r1 = outer_radius - tooth_depth / 2.0f;
	r2 = outer_radius + tooth_depth / 2.0f;

	da = (float)(2.0 * M_PI / (teeth * segs * divs));
	for(i = 0; i < teeth * segs * divs; ++i) {
		float angle = da * i;
		ds[i] = sin(angle);
		dc[i] = cos(angle);
	}

	for(i = 0; i < teeth * segs; ++i) {
		s[i] = ds[i * divs];
		c[i] = dc[i * divs];
	}

	/* faces */
	for(face = -1; face <= 1; face += 2) {
		float z = width * face * 0.5f;
		nx = 0.0f;
		ny = 0.0f;
		nz = (float)face;

		indices.flip = face > 0;

		assert(segs == 4);
		for(i = 0; i < teeth; ++i) {
			VERT(r1 * c[segs * i], r1 * s[segs * i], z);
			VERT(r2 * c[segs * i + 1], r2 * s[segs * i + 1], z);
			VERT(r2 * c[segs * i + 2], r2 * s[segs * i + 2], z);
			VERT(r1 * c[segs * i + 3], r1 * s[segs * i + 3], z);
		}

		for(i = 0; i < teeth * segs * divs; ++i) {
			VERT(r0 * dc[i], r0 * ds[i], z);
		}

		for(i = 0; i < teeth; ++i) {
			for(j = i * segs; j < (i + 1) * segs; ++j) {
				int nextj = j + 1;
				if(nextj == teeth * segs)
					nextj = 0;

				for(k = j * divs; k < (j + 1) * divs; ++k) {
					int nextk = k + 1;
					if(nextk == teeth * segs * divs)
						nextk = 0;
					indices.poly(teeth * segs + k, j, teeth * segs + nextk);
				}

				indices.poly(teeth * segs + nextj * divs, j, nextj);
			}
		}

		indices.base += teeth * segs * (1 + divs);
	}

	/* teeth faces */
	indices.flip = true;
	float z = width * 0.5f;

	float* coords = (float*)malloc((segs + 1) * 2 * sizeof(float));
	nz = 0;
	for(i = 0; i < teeth; i++) {
		int next = i + 1;
		if(next == teeth)
			next = 0;

		coords[0] = r1 * c[segs * i];
		coords[1] = r1 * s[segs * i];
		coords[2] = r2 * c[segs * i + 1];
		coords[3] = r2 * s[segs * i + 1];
		coords[4] = r2 * c[segs * i + 2];
		coords[5] = r2 * s[segs * i + 2];
		coords[6] = r1 * c[segs * i + 3];
		coords[7] = r1 * s[segs * i + 3];
		coords[8] = r1 * c[segs * next];
		coords[9] = r1 * s[segs * next];

		for(int j = 0; j < segs; ++j) {
			float dx = coords[j * 2] - coords[j * 2 + 2];
			float dy = coords[j * 2 + 1] - coords[j * 2 + 3];
			float len = hypotf(dx, dy);
			nx = -dy / len;
			ny = dx / len;
			VERT(coords[j * 2], coords[j * 2 + 1], z);
			VERT(coords[j * 2], coords[j * 2 + 1], -z);
			VERT(coords[j * 2 + 2], coords[j * 2 + 3], z);
			VERT(coords[j * 2 + 2], coords[j * 2 + 3], -z);

			indices.poly(0, 1, 3, 2);
			indices.base += 4;
		}
	}
	free(coords);

	/* inner part - simulate a cylinder */
	indices.flip = true;
	for(i = 0; i < teeth * segs * divs; i++) {
		int next = i + 1;
		if(next == teeth * segs * divs)
			next = 0;

		nx = -dc[i];
		ny = -ds[i];
		VERT(r0 * dc[i], r0 * ds[i], -width * 0.5f);
		VERT(r0 * dc[i], r0 * ds[i], width * 0.5f);

		indices.poly(i * 2, i * 2 + 1, next * 2 + 1, next * 2);
	}

	indices.base += teeth * segs * divs * 2;
	free(c);
	free(s);
	free(dc);
	free(ds);

	D3D11_INPUT_ELEMENT_DESC elements[2] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	return new mesh(dev, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		elements, 2,
		g_vs, sizeof(g_vs),
		&vertices[0], sizeof(vertices[0]), vertices.size(),
		&indices[0], sizeof(indices[0]), indices.size());
}

struct d3d11gears : public d3d11_application
{
	float view_rotx;
	float view_roty;
	float view_rotz;
	int wireframe;
	int triangles;
	float speed;
	float period;
	unsigned impressions;
	bool blue_only;

	float last_time;
	
	int cur_width;
	int cur_height;

	ID3D11DepthStencilView* zsv;
	ID3D11RenderTargetView* offscreen_rtv;
	ID3D11ShaderResourceView* offscreen_srv;
	
	ID3D11Device* dev;
	ID3D11BlendState* blend;
	ID3D11DepthStencilState* zsa;

	ID3D11PixelShader* ps;
	ID3D11VertexShader* vs;
	ID3D11Buffer* cb;

	d3d11_blitter* blitter;

	d3d11gears()
		: cur_width(-1), cur_height(-1), zsv(0), offscreen_rtv(0), offscreen_srv(0)
	{
		view_rotx = (float)(M_PI / 9.0);
		view_roty = (float)(M_PI / 6.0);
		view_rotz = 0.0f;
		wireframe = 0;
		triangles = 3200;
		speed = 1.0f;
		period = -1.0f;
		impressions = 1;
		blue_only = false;
	}

	void draw_one(ID3D11DeviceContext* ctx, cbuf_t& cbd, const float4x4& modelview, float angle)
	{
		for(unsigned i = blue_only ? 2 : 0; i < 3; ++i)
		{
			float4x4 m2 = modelview;
			m2 = mat_push_translate(m2, gears[i].x, gears[i].y, 0.0f);
			m2 = mat_push_rotate(m2, 2, angle * gears[i].wmul + gears[i].t0);

			cbd.modelview = m2;
			cbd.diffuse = gears[i].color;
			cbd.specular = gears[i].color;
			cbd.specular_power = 5.0f;

			ctx->UpdateSubresource(cb, 0, 0, &cbd, 0, 0);

			gears[i].mesh->bind_and_draw(ctx);
		}
	}

	float get_angle(double time)
	{
		// designed so that 1 = original glxgears speed
		float mod_speed = M_PI * 70.0f / 180.0f * speed;
		if(period < 0)
			return (float)(time * mod_speed);
		else
			return (float)(cos(time / period) * period * mod_speed);
	}

	void init_for_dimensions(unsigned width, unsigned height)
	{
		if(zsv)
			zsv->Release();
		ID3D11Texture2D* zsbuf;
		D3D11_TEXTURE2D_DESC zsbufd;
		memset(&zsbufd, 0, sizeof(zsbufd));
		zsbufd.Width = width;
		zsbufd.Height = height;
		zsbufd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		zsbufd.ArraySize = 1;
		zsbufd.MipLevels = 1;
		zsbufd.SampleDesc.Count = 1;
		zsbufd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		ensure(dev->CreateTexture2D(&zsbufd, 0, &zsbuf));
		ensure(dev->CreateDepthStencilView(zsbuf, 0, &zsv));
		zsbuf->Release();

		ID3D11Texture2D* offscreen;
		if(offscreen_rtv)
		{
			offscreen_rtv->Release();
			offscreen_srv->Release();
			offscreen_rtv = 0;
			offscreen_srv = 0;
		}

		if(impressions > 1)
		{
			DXGI_FORMAT formats[] = {
				DXGI_FORMAT_R32G32B32A32_FLOAT,
				DXGI_FORMAT_R16G16B16A16_UNORM,
				DXGI_FORMAT_R16G16B16A16_FLOAT,
				DXGI_FORMAT_R10G10B10A2_UNORM,
			};
			DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM; // this won't work well at all
			unsigned needed_support = D3D11_FORMAT_SUPPORT_RENDER_TARGET | D3D11_FORMAT_SUPPORT_BLENDABLE | D3D11_FORMAT_SUPPORT_SHADER_SAMPLE;
			for(unsigned i = 0; i < sizeof(formats); ++i)
			{	
				unsigned support;
				dev->CheckFormatSupport(DXGI_FORMAT_R32G32B32A32_FLOAT, &support);
				if((support & needed_support) == needed_support)
				{
					format = formats[i];
					break;
				}
			}
				

			D3D11_TEXTURE2D_DESC offscreend;
			memset(&offscreend, 0, sizeof(offscreend));
			offscreend.Width = width;
			offscreend.Height = height;
				
			offscreend.Format = format;
			offscreend.MipLevels = 1;
			offscreend.ArraySize = 1;
			offscreend.SampleDesc.Count = 1;
			offscreend.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
			ensure(dev->CreateTexture2D(&offscreend, 0, &offscreen));
			ensure(dev->CreateRenderTargetView(offscreen, 0, &offscreen_rtv));
			ensure(dev->CreateShaderResourceView(offscreen, 0, &offscreen_srv));
			offscreen->Release();
		}

		cur_width = width;
		cur_height = height;
	}

	void draw(ID3D11DeviceContext* ctx, ID3D11RenderTargetView* rtv, unsigned width, unsigned height, double time)
	{
		D3D11_VIEWPORT vp;
		memset(&vp, 0, sizeof(vp));
		vp.Width = (float)width;
		vp.Height = (float)height;
		vp.MaxDepth = 1.0f;

		if((int)width != cur_width || (int)height != cur_height)
			init_for_dimensions(width, height);

		float4 lightpos = vec(5.0f, 5.0f, 10.0f, 0.0f);
		float black[4] = {0.0, 0.0, 0.0, 0};

		float4x4 proj;
		float4x4 m;

		float xr = (float)width / (float)height;
		float yr = 1.0f;
		if(xr < 1.0f) {
			yr /= xr;
			xr = 1.0f;
		}
		proj = mat4x4_frustum(-xr, xr, -yr, yr, 5.0f, 60.0f);

		m = mat4x4_diag(1.0f);
		m = mat_push_translate(m, 0.0f, 0.0f, -40.0f);
		m = mat_push_rotate(m, 0, view_rotx);
		m = mat_push_rotate(m, 1, view_roty);
		m = mat_push_rotate(m, 2, view_rotz);

		cbuf_t cbd;

		cbd.projection = proj;
		cbd.light = lightpos;

		float blend_factor[4] = {1.0f / (float)impressions, 1.0f / (float)impressions, 1.0f / (float)impressions, 1.0f / (float)impressions};

		ID3D11RenderTargetView* render_rtv;
		if(impressions == 1)
			render_rtv = rtv;
		else
			render_rtv = offscreen_rtv;

		ctx->RSSetViewports(1, &vp);
		ctx->ClearRenderTargetView(render_rtv, black);
		
		ctx->PSSetShader(ps, 0, 0);
		ctx->VSSetShader(vs, 0, 0);

		ctx->PSSetConstantBuffers(0, 1, &cb);
		ctx->VSSetConstantBuffers(0, 1, &cb);

		if(impressions == 1)
		{
			ctx->OMSetBlendState(0, 0, ~0);
			ctx->OMSetDepthStencilState(0, 0);
			ctx->OMSetRenderTargets(1, &rtv, zsv);
			ctx->ClearDepthStencilView(zsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);
			draw_one(ctx, cbd, m, get_angle(time));
		}
		else
		{
			ctx->OMSetBlendState(blend, blend_factor, ~0);

			float time_delta = (float)time - last_time;
			float time_delta_per_impression = time_delta / impressions;
			float base_time = last_time + time_delta_per_impression / 2;
			for(unsigned impression = 0; impression < impressions; ++impression)
			{
				float impression_time = base_time + time_delta_per_impression * impression;

				ctx->ClearDepthStencilView(zsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);

				// do early z-pass since we must not write any pixel more than once due to blending
				for(unsigned pass = 0; pass < 2; ++pass)
				{
					if(pass == 0)
					{
						ctx->OMSetRenderTargets(0, 0, zsv);
						ctx->OMSetDepthStencilState(0, 0);
					}
					else
					{
						ctx->OMSetRenderTargets(1, &render_rtv, zsv);
						ctx->OMSetDepthStencilState(zsa, 0);
					}

					draw_one(ctx, cbd, m, get_angle(impression_time));
				}
			}

			blitter->bind_draw_and_unbind(ctx, offscreen_srv, rtv, 0, 0, (float)width, (float)height, false);
		}
		last_time = (float)time;
	}

	bool init(ID3D11Device* dev, int argc, char** argv)
	{
		this->dev = dev;

		for(char** p = argv + 1; *p; ++p) {
			if(!strcmp(*p, "-w"))
				wireframe = 1;
			else if(!strcmp(*p, "-b"))
				blue_only = true;
			else if(!strcmp(*p, "-t"))
				triangles = atoi(*++p);
			else if(!strcmp(*p, "-m"))
				impressions = (float)atof(*++p);
			else if(!strcmp(*p, "-p"))
				period = (float)atof(*++p);
			else if(!strcmp(*p, "-s"))
				speed = (float)atof(*++p);
			else {
				fprintf(stderr, "Usage: d3d11gears [-v|-w] [-t TRIANGLES]\n");
				fprintf(stderr, "d3d11gears is an enhanced port of glxgears to Direct3D 11\n");
				fprintf(stderr, "\n");
				//fprintf(stderr, "-v\t\tuse per-vertex diffuse-only lighting (classic glxgears look)\n");
				fprintf(stderr, "-w\t\twireframe mode\n");
				fprintf(stderr, "-t TRIANGLES\ttriangle budget (default is 3200)\n");
				fprintf(stderr, "-m IMPRESSIONS\tmotion blur impressions (default is 1)\n");
				fprintf(stderr, "-p PERIOD\tspeed reversal period (default is infinite)\n");
				fprintf(stderr, "-s SPEED\tgear speed (default is 1.0)\n");
				fprintf(stderr, "-b\tonly show blue gear (for faster motion blur)\n");
				return false;
			}
		}

		ensure(dev->CreatePixelShader(g_ps, sizeof(g_ps), NULL, &ps));
		ensure(dev->CreateVertexShader(g_vs, sizeof(g_vs), NULL, &vs));

		gears[0].color = vec(0.8f, 0.1f, 0.0f, 1.0f);
		gears[1].color = vec(0.0f, 0.8f, 0.2f, 1.0f);
		gears[2].color = vec(0.2f, 0.2f, 1.0f, 1.0f);

		gears[0].mesh = build_gear(dev, triangles / 2, 1.0f, 4.0f, 1.0f, 20, 0.7f);
		gears[1].mesh = build_gear(dev, triangles / 4, 0.5f, 2.0f, 2.0f, 10, 0.7f);
		gears[2].mesh = build_gear(dev, triangles / 4, 1.3f, 2.0f, 0.5f, 10, 0.7f);

		gears[0].x = -3.0f;
		gears[0].y = -2.0f;
		gears[0].wmul = 1.0f;
		gears[0].t0 = 0.0 * M_PI / 180.0f;

		gears[1].x = 3.1f;
		gears[1].y = -2.0f;
		gears[1].wmul = -2.0f;
		gears[1].t0 = -9.0f * (float)M_PI / 180.0f;

		gears[2].x = -3.1f;
		gears[2].y = 4.2f;
		gears[2].wmul = -2.0f;
		gears[2].t0 = -25.0f * (float)M_PI / 180.0f;

		D3D11_BUFFER_DESC bufferd;
		memset(&bufferd, 0, sizeof(bufferd));
		bufferd.ByteWidth = sizeof(cbuf_t);
		bufferd.Usage = D3D11_USAGE_DEFAULT;
		bufferd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		ensure(dev->CreateBuffer(&bufferd, 0, &cb));

		if(impressions > 1)
		{
			D3D11_BLEND_DESC blendd;
			memset(&blendd, 0, sizeof(blendd));
			blendd.RenderTarget[0].BlendEnable = TRUE;
			blendd.RenderTarget[0].BlendOp = blendd.RenderTarget[0].BlendOpAlpha
				= D3D11_BLEND_OP_ADD;
			blendd.RenderTarget[0].SrcBlend = blendd.RenderTarget[0].SrcBlendAlpha
				= D3D11_BLEND_BLEND_FACTOR;
			blendd.RenderTarget[0].DestBlend = blendd.RenderTarget[0].DestBlendAlpha
				= D3D11_BLEND_ONE;
			blendd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			ensure(dev->CreateBlendState(&blendd, &blend));

			D3D11_DEPTH_STENCIL_DESC zsad;
			memset(&zsad, 0, sizeof(zsad));
			zsad.DepthEnable = TRUE;
			zsad.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			zsad.DepthFunc = D3D11_COMPARISON_EQUAL;
			ensure(dev->CreateDepthStencilState(&zsad, &zsa));

			blitter = new d3d11_blitter(dev);
		}
			
		return true;
	}
};

d3d11_application* d3d11_application_create()
{
	return new d3d11gears();
}
