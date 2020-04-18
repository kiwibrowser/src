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

#define _USE_MATH_DEFINES
#include "d3d11app.h"
#include "d3d11spikysphere.hlsl.vs.h"
#include "d3d11spikysphere.hlsl.hs.h"
#include "d3d11spikysphere.hlsl.ds.h"
#include "d3d11spikysphere.hlsl.ps.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <D3DX10math.h>

struct cb_frame_t
{
	D3DXMATRIX model;
	D3DXMATRIX view_proj;
	float disp_scale;
	float disp_freq;
	float tess_factor;
};

static float vertex_data[] =
{
	1.0, 0.0, 0.0,
	0.0, 1.0, 0.0,
	0.0, 0.0, 1.0,

	0.0, 1.0, 0.0,
	-1.0, 0.0, 0.0,
	0.0, 0.0, 1.0,
			
	0.0, -1.0, 0.0,
	1.0, 0.0, 0.0,
	0.0, 0.0, 1.0,
			
	-1.0, 0.0, 0.0,
	0.0, -1.0, 0.0,
	0.0, 0.0, 1.0,
			
	0.0, 1.0, 0.0,
	1.0, 0.0, 0.0,
	0.0, 0.0, -1.0,

	-1.0, 0.0, 0.0,
	0.0, 1.0, 0.0,
	0.0, 0.0, -1.0,
			
	1.0, 0.0, 0.0,
	0.0, -1.0, 0.0,
	0.0, 0.0, -1.0,

	0.0, -1.0, 0.0,
	-1.0, 0.0, 0.0,
	0.0, 0.0, -1.0,
};

struct d3d11spikysphere : public d3d11_application
{
	ID3D11Device* dev;
	ID3D11PixelShader* ps;
	ID3D11DomainShader* ds;
	ID3D11HullShader* hs;
	ID3D11VertexShader* vs;
	ID3D11InputLayout* layout;
	ID3D11Buffer* vb;
	ID3D11RenderTargetView* rtv;
	ID3D11DepthStencilView* zsv;
	ID3D11Buffer* cb_frame;

	int cur_width;
	int cur_height;

	d3d11spikysphere()
	: cur_width(-1), cur_height(-1), zsv(0)
	{}

	bool init(ID3D11Device* dev, int argc, char** argv)
	{
		this->dev = dev;
		ensure(dev->CreateVertexShader(g_vs, sizeof(g_vs), NULL, &vs));
		ensure(dev->CreateHullShader(g_hs, sizeof(g_hs), NULL, &hs));
		ensure(dev->CreateDomainShader(g_ds, sizeof(g_ds), NULL, &ds));
		ensure(dev->CreatePixelShader(g_ps, sizeof(g_ps), NULL, &ps));
		
		D3D11_INPUT_ELEMENT_DESC elements[1] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		ensure(dev->CreateInputLayout(elements, 1, g_vs, sizeof(g_vs), &layout));

		D3D11_BUFFER_DESC bufferd;
		bufferd.ByteWidth = sizeof(vertex_data);
		bufferd.Usage = D3D11_USAGE_IMMUTABLE;
		bufferd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferd.CPUAccessFlags = 0;
		bufferd.MiscFlags = 0;
		bufferd.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA buffersd;
		buffersd.pSysMem = vertex_data;

		ensure(dev->CreateBuffer(&bufferd, &buffersd, &vb));

		D3D11_BUFFER_DESC cbd;
		cbd.ByteWidth = (sizeof(cb_frame_t) + 15) & ~15;
		cbd.Usage = D3D11_USAGE_DYNAMIC;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbd.MiscFlags = 0;
		cbd.StructureByteStride = 0;

		ensure(dev->CreateBuffer(&cbd, NULL, &cb_frame));
		return true;
	}

	void draw(ID3D11DeviceContext* ctx, ID3D11RenderTargetView* rtv, unsigned width, unsigned height, double time)
	{
		D3D11_VIEWPORT vp;
		memset(&vp, 0, sizeof(vp));
		vp.Width = (float)width;
		vp.Height = (float)height;
		vp.MaxDepth = 1.0f;

		if(width != cur_width || height != cur_height)
		{
			if(zsv)
				zsv->Release();
			ID3D11Texture2D* zsbuf;
			D3D11_TEXTURE2D_DESC zsbufd;
			memset(&zsbufd, 0, sizeof(zsbufd));
			zsbufd.Width = width;
			zsbufd.Height = height;
			zsbufd.Format = DXGI_FORMAT_D32_FLOAT;
			zsbufd.ArraySize = 1;
			zsbufd.MipLevels = 1;
			zsbufd.SampleDesc.Count = 1;
			zsbufd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			ensure(dev->CreateTexture2D(&zsbufd, 0, &zsbuf));
			ensure(dev->CreateDepthStencilView(zsbuf, 0, &zsv));
			zsbuf->Release();
		}

		float black[4] = {0, 0, 0, 0};

		D3D11_MAPPED_SUBRESOURCE map;
		ensure(ctx->Map(cb_frame, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
		cb_frame_t* cb_frame_data = (cb_frame_t*)map.pData;
		D3DXMatrixIdentity(&cb_frame_data->model);

		D3DXMATRIX view;
		D3DXVECTOR3 eye(2.0f * (float)sin(time), 0.0f, 2.0f * (float)cos(time));
		D3DXVECTOR3 at(0, 0, 0);
		D3DXVECTOR3 up(0, 1, 0);
		D3DXMatrixLookAtLH(&view, &eye, &at, &up);
		D3DXMATRIX proj;
		D3DXMatrixPerspectiveLH(&proj, 1.1f, 1.1f, 1.0f, 3.0f);

		cb_frame_data->view_proj = view * proj;
		float min_tess_factor = 1.0f;
		cb_frame_data->tess_factor = (1.0f - (float)cos(time)) * ((64.0f - min_tess_factor) / 2.0f) + min_tess_factor;
		cb_frame_data->disp_scale = 0.9f;
		//cb_frame_data->disp_scale = (sin(time) + 1.0) / 2.0;
		cb_frame_data->disp_freq = 5.0f * (float)M_PI;
		//cb_frame_data->disp_freq = (4.0 + 4.0 * cos(time / 5.0)) * PI;
		ctx->Unmap(cb_frame, 0);

		ctx->HSSetConstantBuffers(0, 1, &cb_frame);
		ctx->DSSetConstantBuffers(0, 1, &cb_frame);
		
		//ctx->OMSetBlendState(bs, black, ~0);
		//ctx->OMSetDepthStencilState(dss, 0);
		ctx->OMSetRenderTargets(1, &rtv, zsv);
		//ctx->RSSetState(rs);
		ctx->RSSetViewports(1, &vp);

		ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		ctx->IASetInputLayout(layout);
		unsigned stride = 3 * 4;
		unsigned offset = 0;
		ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

		ctx->VSSetShader(vs, NULL, 0);
		ctx->HSSetShader(hs, NULL, 0);
		ctx->DSSetShader(ds, NULL, 0);
		ctx->GSSetShader(NULL, NULL, 0);
		ctx->PSSetShader(ps, NULL, 0);	

		ctx->ClearRenderTargetView(rtv, black);
		ctx->ClearDepthStencilView(zsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

		ctx->Draw(3 * 8, 0);
	}
};

d3d11_application* d3d11_application_create()
{
	return new d3d11spikysphere();
}
