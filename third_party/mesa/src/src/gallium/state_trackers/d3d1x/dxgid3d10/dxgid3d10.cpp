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
#include "galliumd3d10_1.h"
#include <dxgi.h>
#include "pipe/p_screen.h"
#include "pipe/p_context.h"

HRESULT D3D10CreateDevice1(
	IDXGIAdapter *adapter,
	D3D10_DRIVER_TYPE driver_type,
	HMODULE software,
	unsigned flags,
	D3D10_FEATURE_LEVEL1 hardware_level,
	unsigned sdk_version,
	ID3D10Device1 **out_device
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
	if(driver_type == D3D10_DRIVER_TYPE_REFERENCE)
		screen = gallium_adapter->GetGalliumReferenceSoftwareScreen();
	else if(driver_type == D3D10_DRIVER_TYPE_SOFTWARE || driver_type == D3D10_DRIVER_TYPE_WARP)
		screen = gallium_adapter->GetGalliumFastSoftwareScreen();
	else
		screen = gallium_adapter->GetGalliumScreen();
	if(!screen)
		return E_FAIL;
	struct pipe_context* context = screen->context_create(screen, 0);
	if(!context)
		return E_FAIL;
	ComPtr<ID3D10Device1> device;
	hr = GalliumD3D10DeviceCreate1(screen, context, TRUE, flags, adapter, &device);
	if(!SUCCEEDED(hr))
	{
		context->destroy(context);
		return hr;
	}
	if(out_device)
		*out_device = device.steal();
	return S_OK;
}

HRESULT WINAPI D3D10CreateDeviceAndSwapChain1(
	IDXGIAdapter* adapter,
	D3D10_DRIVER_TYPE driver_type,
	HMODULE software,
	unsigned flags,
	D3D10_FEATURE_LEVEL1 hardware_level,
	unsigned sdk_version,
	DXGI_SWAP_CHAIN_DESC* swap_chain_desc,
	IDXGISwapChain** out_swap_chain,
	ID3D10Device1** out_device
)
{
	ComPtr<ID3D10Device1> dev;
	HRESULT hr;
	hr = D3D10CreateDevice1(adapter, driver_type, software, flags, hardware_level, sdk_version, &dev);
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
		hr = factory->CreateSwapChain(dev.p, (DXGI_SWAP_CHAIN_DESC*)swap_chain_desc, out_swap_chain);
		if(!SUCCEEDED(hr))
			return hr;
	}
	if(out_device)
		*out_device = dev.steal();
	return hr;
}

HRESULT D3D10CreateDevice(
	IDXGIAdapter *adapter,
	D3D10_DRIVER_TYPE driver_type,
	HMODULE software,
	unsigned flags,
	unsigned sdk_version,
	ID3D10Device **out_device
)
{
	return D3D10CreateDevice1(adapter, driver_type, software, flags, D3D10_FEATURE_LEVEL_10_0, sdk_version, (ID3D10Device1**)out_device);
}

HRESULT WINAPI D3D10CreateDeviceAndSwapChain(
	IDXGIAdapter* adapter,
	D3D10_DRIVER_TYPE driver_type,
	HMODULE software,
	unsigned flags,
	unsigned sdk_version,
	DXGI_SWAP_CHAIN_DESC* swap_chain_desc,
	IDXGISwapChain** out_swap_chain,
	ID3D10Device** out_device
)
{
	return D3D10CreateDeviceAndSwapChain1(adapter, driver_type, software, flags, D3D10_FEATURE_LEVEL_10_0, sdk_version, swap_chain_desc, out_swap_chain, (ID3D10Device1**)out_device);
}
