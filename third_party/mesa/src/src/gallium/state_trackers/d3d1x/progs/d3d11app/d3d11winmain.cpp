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

#define INITGUID
#include "d3d11app.h"
#include "stdio.h"

static d3d11_application* app;
static IDXGISwapChain* swap_chain;
static unsigned width, height;
static DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
static ID3D11Device* dev;
static ID3D11DeviceContext* ctx;
static int frames = 0;
static int buffer_count = 1;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		width = lParam & 0xffff;
		height = lParam >> 16;
		
		swap_chain->ResizeBuffers(buffer_count, width, height, format, 0);
		frames = 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

int main(int argc, char** argv)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	WNDCLASSEXA wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style		= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon		= 0;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= "d3d11";
	wcex.hIconSm		= 0;

	RegisterClassExA(&wcex);

	HWND hwnd = CreateWindowA("d3d11", "d3d11", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if(!hwnd)
		return FALSE;

	RECT rc;
	GetClientRect(hwnd, &rc );
	width = rc.right - rc.left;
	height = rc.bottom - rc.top;

	DXGI_SWAP_CHAIN_DESC swap_chain_desc;
	memset(&swap_chain_desc, 0, sizeof(swap_chain_desc));
	swap_chain_desc.BufferDesc.Width = width;
	swap_chain_desc.BufferDesc.Height = height;
	swap_chain_desc.BufferDesc.Format = format;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.OutputWindow = hwnd;
	swap_chain_desc.Windowed = TRUE;
	swap_chain_desc.BufferCount = buffer_count;
	swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_10_0;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D11_CREATE_DEVICE_SINGLETHREADED, // | D3D11_CREATE_DEVICE_DEBUG,
		NULL,
		0,
		D3D11_SDK_VERSION,
		&swap_chain_desc,
		&swap_chain,
		&dev,
		&feature_level,
		&ctx);
	if(!SUCCEEDED(hr))
	{
		fprintf(stderr, "Failed to create D3D11 device (hresult %08x)\n", hr);
		return 1;
	}

	app = d3d11_application_create();
	if(!app->init(dev, argc, argv))
		return 1;

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	double period = 1.0 / (double)freq.QuadPart;
	LARGE_INTEGER ctime_li;
	QueryPerformanceCounter(&ctime_li);
	double start_time = ctime_li.QuadPart * period;

	MSG msg;
	for(;;)
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else if(width && height)
		{
			ID3D11Texture2D* tex;
			static ID3D11RenderTargetView* rtv;
			ensure(swap_chain->GetBuffer(0, __uuidof(tex), (void**)&tex));
			ensure(dev->CreateRenderTargetView(tex, NULL, &rtv));

			QueryPerformanceCounter(&ctime_li);
			double ctime = (double)ctime_li.QuadPart * period - start_time;

			app->draw(ctx, rtv, width, height, ctime);
			ctx->OMSetRenderTargets(0, 0, 0);

			swap_chain->Present(0, 0);
			rtv->Release();
			tex->Release();
		}
		else
			WaitMessage();
	}
	return (int) msg.wParam;
}
