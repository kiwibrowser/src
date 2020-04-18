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

#include "d3d11app.h"
#include "d3d11tri.hlsl.ps.h"
#include "d3d11tri.hlsl.vs.h"

struct vertex {
 float position[4];
 float color[4];
};

static struct vertex vertices[3] =
{
 {
	 { 0.0f, 0.9f, 0.5f, 1.0f },
	 { 1.0f, 0.0f, 0.0f, 1.0f }
 },
 {
	 { 0.9f, -0.9f, 0.5f, 1.0f },
	 { 0.0f, 0.0f, 1.0f, 1.0f }
 },
 {
	 { -0.9f, -0.9f, 0.5f, 1.0f },
	 { 0.0f, 1.0f, 0.0f, 1.0f }
 },
};

struct d3d11tri : public d3d11_application
{
	ID3D11PixelShader* ps;
	ID3D11VertexShader* vs;
	ID3D11InputLayout* layout;
	ID3D11Buffer* vb;

	virtual bool init(ID3D11Device* dev, int argc, char** argv)
	{
		ensure(dev->CreatePixelShader(g_ps, sizeof(g_ps), NULL, &ps));
		ensure(dev->CreateVertexShader(g_vs, sizeof(g_vs), NULL, &vs));

		D3D11_INPUT_ELEMENT_DESC elements[] =
		{
			// inverse order to make sure the implementation can properly parse the vertex shader signature
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		ensure(dev->CreateInputLayout(elements, sizeof(elements) / sizeof(elements[0]), g_vs, sizeof(g_vs), &layout));
		D3D11_BUFFER_DESC bufferd;
		bufferd.ByteWidth = sizeof(vertices);
		bufferd.Usage = D3D11_USAGE_IMMUTABLE;
		bufferd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferd.CPUAccessFlags = 0;
		bufferd.MiscFlags = 0;
		bufferd.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA buffersd;
		buffersd.pSysMem = vertices;
		buffersd.SysMemPitch = sizeof(vertices);
		buffersd.SysMemSlicePitch = sizeof(vertices);

		ensure(dev->CreateBuffer(&bufferd, &buffersd, &vb));

		return true;
	}

	virtual void draw(ID3D11DeviceContext* ctx, ID3D11RenderTargetView* rtv, unsigned width, unsigned height, double time)
	{
		float clear_color[4] = {1, 0, 1, 1};
		D3D11_VIEWPORT vp;
		memset(&vp, 0, sizeof(vp));
		vp.Width = (float)width;
		vp.Height = (float)height;
		vp.MaxDepth = 1.0f;

		ctx->OMSetRenderTargets(1, &rtv, 0);
		ctx->RSSetViewports(1, &vp);

		ctx->ClearRenderTargetView(rtv, clear_color);

		ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ctx->IASetInputLayout(layout);
		unsigned stride = 2 * 4 * 4;
		unsigned offset = 0;
		ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

		ctx->VSSetShader(vs, NULL, 0);
		ctx->PSSetShader(ps, NULL, 0);	

		ctx->Draw(3, 0);
	}
};

d3d11_application* d3d11_application_create()
{
	return new d3d11tri();
}
