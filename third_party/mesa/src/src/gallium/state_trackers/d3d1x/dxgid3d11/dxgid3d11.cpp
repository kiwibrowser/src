/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "software"), to deal in the software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the software, and to
 * permit persons to whom the software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the software.
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

#include "d3d1xstutil.h"
#include "galliumd3d11.h"
#include <dxgi.h>
#include "pipe/p_screen.h"
#include "pipe/p_context.h"

HRESULT D3D11CreateDevice(
	IDXGIAdapter *adapter,
	D3D_DRIVER_TYPE driver_type,
	HMODULE software,
	unsigned flags,
	const D3D_FEATURE_LEVEL *feature_levels,
	unsigned num_feature_levels,
	unsigned sdk_version,
	ID3D11Device **out_device,
	D3D_FEATURE_LEVEL *feature_level,
	ID3D11DeviceContext **out_immediate_context
)
{
	HRESULT hr;
	ComPtr<IDXGIAdapter1> adapter_to_release;
	if(!adapter)
	{
		ComPtr<IDXGIFactory1> factory;
		hr = CreateDXGIFactory1(IID_IDXGIFactory1, (void**)&factory);
		if(!SUCCEEDED(hr))
			return hr;
		hr = factory->EnumAdapters1(0, &adapter_to_release);
		if(!SUCCEEDED(hr))
			return hr;
		adapter = adapter_to_release.p;
	}
	ComPtr<IGalliumAdapter> gallium_adapter;
	hr = adapter->QueryInterface(IID_IGalliumAdapter, (void**)&gallium_adapter);
	if(!SUCCEEDED(hr))
		return hr;
	struct pipe_screen* screen;
	// TODO: what should D3D_DRIVER_TYPE_SOFTWARE return? fast or reference?
	if(driver_type == D3D_DRIVER_TYPE_REFERENCE)
		screen = gallium_adapter->GetGalliumReferenceSoftwareScreen();
	else if(driver_type == D3D_DRIVER_TYPE_SOFTWARE || driver_type == D3D_DRIVER_TYPE_WARP)
		screen = gallium_adapter->GetGalliumFastSoftwareScreen();
	else
		screen = gallium_adapter->GetGalliumScreen();
	if(!screen)
		return E_FAIL;
	struct pipe_context* context = screen->context_create(screen, 0);
	if(!context)
		return E_FAIL;
	ComPtr<ID3D11Device> device;
	hr = GalliumD3D11DeviceCreate(screen, context, TRUE, flags, adapter, &device);
	if(!SUCCEEDED(hr))
	{
		context->destroy(context);
		return hr;
	}
	if(out_immediate_context)
		device->GetImmediateContext(out_immediate_context);
	if(feature_level)
		*feature_level = device->GetFeatureLevel();
	if(out_device)
		*out_device = device.steal();
	return S_OK;
}

HRESULT WINAPI D3D11CreateDeviceAndSwapChain(
	IDXGIAdapter* adapter,
	D3D_DRIVER_TYPE driver_type,
	HMODULE software,
	unsigned flags,
	CONST D3D_FEATURE_LEVEL* feature_levels,
	unsigned num_feature_levels,
	unsigned sdk_version,
	CONST DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
	IDXGISwapChain** out_swap_chain,
	ID3D11Device** out_device,
	D3D_FEATURE_LEVEL* feature_level,
	ID3D11DeviceContext** out_immediate_context )
{
	ComPtr<ID3D11Device> dev;
	ComPtr<ID3D11DeviceContext> ctx;
	HRESULT hr;
	hr = D3D11CreateDevice(adapter, driver_type, software, flags, feature_levels, num_feature_levels, sdk_version, (ID3D11Device**)&dev, feature_level, (ID3D11DeviceContext**)&ctx);
	if(!SUCCEEDED(hr))
		return hr;
	if(out_swap_chain)
	{
		ComPtr<IDXGIFactory> factory;
		ComPtr<IDXGIDevice> dxgi_device;
		ComPtr<IDXGIAdapter> adapter;
		hr = dev->QueryInterface(IID_IDXGIDevice, (void**)&dxgi_device);
		if(!SUCCEEDED(hr))
			return hr;

		hr = dxgi_device->GetAdapter(&adapter);
		if(!SUCCEEDED(hr))
			return hr;

		adapter->GetParent(IID_IDXGIFactory, (void**)&factory);
		hr = factory->CreateSwapChain(dev.p, (DXGI_SWAP_CHAIN_DESC*)pSwapChainDesc, out_swap_chain);
		if(!SUCCEEDED(hr))
			return hr;
	}
	if(out_device)
		*out_device = dev.steal();
	if(out_immediate_context)
		*out_immediate_context = ctx.steal();
	return hr;
}
