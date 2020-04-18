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
#include "d3d11u.h"
#include "d3d11tex.hlsl.ps.h"
#include "d3d11tex.hlsl.vs.h"
#include "../data/cornell_box_image.h"
#include "../data/tux_image.h"

struct d3d11tex : public d3d11_application
{
	ID3D11PixelShader* ps;
	ID3D11VertexShader* vs;
	mesh* quad;
	ID3D11ShaderResourceView* srv[2];
	ID3D11SamplerState* samp[2];

	virtual bool init(ID3D11Device* dev, int argc, char** argv)
	{
		ensure(dev->CreatePixelShader(g_ps, sizeof(g_ps), NULL, &ps));
		ensure(dev->CreateVertexShader(g_vs, sizeof(g_vs), NULL, &vs));

		quad = create_tex_quad(dev, g_vs, sizeof(g_vs));

		D3D11_TEXTURE2D_DESC texd;
		memset(&texd, 0, sizeof(texd));
		texd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texd.Usage = D3D11_USAGE_IMMUTABLE;
		texd.SampleDesc.Count = 1;
		texd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texd.Width = 32;
		texd.Height = 32;
		texd.ArraySize = 1;
		texd.MipLevels = 1;

		D3D11_SUBRESOURCE_DATA texsd;
		texsd.SysMemPitch = 32 * 4;
		texsd.SysMemSlicePitch = 32 * 32 * 4;

		ID3D11Texture2D* tex;

		texsd.pSysMem = g_cornell_box_image;
		ensure(dev->CreateTexture2D(&texd, &texsd, &tex));
		ensure(dev->CreateShaderResourceView(tex, 0, &srv[0]));
		tex->Release();

		texsd.pSysMem = g_tux_image;
		ensure(dev->CreateTexture2D(&texd, &texsd, &tex));
		ensure(dev->CreateShaderResourceView(tex, 0, &srv[1]));
		tex->Release();

		D3D11_SAMPLER_DESC sampd;
		memset(&sampd, 0, sizeof(sampd));
		sampd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampd.MinLOD = -FLT_MAX;
		sampd.MaxLOD = FLT_MAX;

		sampd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		dev->CreateSamplerState(&sampd, &samp[0]);

		sampd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		dev->CreateSamplerState(&sampd, &samp[1]);
		return true;
	}

	virtual void draw(ID3D11DeviceContext* ctx, ID3D11RenderTargetView* rtv, unsigned width, unsigned height, double time)
	{
		D3D11_VIEWPORT vp;
		memset(&vp, 0, sizeof(vp));
		vp.Width = (float)width;
		vp.Height = (float)height;
		vp.MaxDepth = 1.0f;

		ctx->OMSetRenderTargets(1, &rtv, 0);
		ctx->RSSetViewports(1, &vp);

		ctx->VSSetShader(vs, NULL, 0);
		ctx->PSSetShader(ps, NULL, 0);	

		ctx->PSSetShaderResources(0, 2, srv);
		ctx->PSSetSamplers(0, 2, samp);

		quad->bind_and_draw(ctx);
	}
};

d3d11_application* d3d11_application_create()
{
	return new d3d11tex();
}
