#include "d3d11app.h"
#include <X11/Xlib.h>
#include <galliumdxgi.h>
#include <sys/time.h>

static d3d11_application* app;
static IDXGISwapChain* swap_chain;
unsigned width, height;
DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
static ID3D11Device* dev;
static ID3D11DeviceContext* ctx;

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

	D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_10_0;

	HRESULT hr =D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D11_CREATE_DEVICE_SINGLETHREADED,
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
			ID3D11Texture2D* tex;
			ID3D11RenderTargetView* rtv;
			ensure(swap_chain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&tex));
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
