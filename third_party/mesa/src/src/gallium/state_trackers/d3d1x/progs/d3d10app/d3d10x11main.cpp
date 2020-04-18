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

#include "d3d10app.h"
#include <X11/Xlib.h>
#include <galliumdxgi.h>
#include <sys/time.h>

static d3d10_application* app;
static IDXGISwapChain* swap_chain;
unsigned width, height;
DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
static ID3D10Device* dev;
static ID3D10Device* ctx;

double get_time()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (double)tv.tv_sec + (double)tv.tv_usec * 0.000001;
}

int main(int argc, char** argv)
{
	Display* dpy = XOpenDisplay(0);
	Visual* visual = DefaultVisual(dpy, DefaultScreen(dpy));
	Colormap cmap = XCreateColormap(dpy, RootWindow(dpy, DefaultScreen(dpy)), visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap = cmap;
	swa.border_pixel = 0;
	swa.event_mask = StructureNotifyMask;
	width = 512;
	height = 512;
	Window win = XCreateWindow(dpy, RootWindow(dpy, DefaultScreen(dpy)), 0, 0, width, height, 0, CopyFromParent, InputOutput, visual, CWBorderPixel | CWColormap| CWEventMask, &swa);
	XMapWindow(dpy, win);

	GalliumDXGIUseX11Display(dpy, 0);

	DXGI_SWAP_CHAIN_DESC swap_chain_desc;
	memset(&swap_chain_desc, 0, sizeof(swap_chain_desc));
	swap_chain_desc.BufferDesc.Width = width;
	swap_chain_desc.BufferDesc.Height = height;
	swap_chain_desc.BufferDesc.Format = format;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.OutputWindow = (HWND)win;
	swap_chain_desc.Windowed = TRUE;
	swap_chain_desc.BufferCount = 3;
	swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	D3D10_FEATURE_LEVEL1 feature_level = D3D10_FEATURE_LEVEL_10_0;

	HRESULT hr;
	if(0)
	{
		hr = D3D10CreateDeviceAndSwapChain(
			NULL,
			D3D10_DRIVER_TYPE_HARDWARE,
			NULL,
			D3D10_CREATE_DEVICE_SINGLETHREADED,
			D3D10_SDK_VERSION,
			&swap_chain_desc,
			&swap_chain,
			&dev);
	}
	else
	{
		hr = D3D10CreateDeviceAndSwapChain1(
			NULL,
			D3D10_DRIVER_TYPE_HARDWARE,
			NULL,
			D3D10_CREATE_DEVICE_SINGLETHREADED,
			feature_level,
			D3D10_SDK_VERSION,
			&swap_chain_desc,
			&swap_chain,
			(ID3D10Device1**)&dev);
	}
	if(!SUCCEEDED(hr))
	{
		fprintf(stderr, "Failed to create D3D10 device (hresult %08x)\n", hr);
		return 1;
	}
	ctx = dev;

	app = d3d10_application_create();
	if(!app->init(dev, argc, argv))
		return 1;

	double start_time = get_time();

	MSG msg;
	for(;;)
	{
		XEvent event;
		if(XPending(dpy))
		{
			XNextEvent(dpy, &event);
			if(event.type == DestroyNotify)
				break;
			switch(event.type)
			{
			case ConfigureNotify:
				width = event.xconfigure.width;
				height = event.xconfigure.height;
				swap_chain->ResizeBuffers(3, width, height, format, 0);
				break;
			}
		}
		else if(width && height)
		{
			ID3D10Texture2D* tex;
			ID3D10RenderTargetView* rtv;
			ensure(swap_chain->GetBuffer(0, IID_ID3D10Texture2D, (void**)&tex));
			ensure(dev->CreateRenderTargetView(tex, NULL, &rtv));

			double ctime = get_time() - start_time;

			app->draw(ctx, rtv, width, height, ctime);
			ctx->OMSetRenderTargets(0, 0, 0);

			tex->Release();
			rtv->Release();
			swap_chain->Present(0, 0);
		}
		else
			XPeekEvent(dpy, &event);
	}
	return (int) msg.wParam;
}
