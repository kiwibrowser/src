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

#include <windows.h>
#include <winnt.h>
#include <X11/Xlib.h>
#include <galliumdxgi.h>

#define DLL_WINE_PREATTACH 8

#define X11DRV_ESCAPE 6789
#define X11DRV_GET_DISPLAY 0
#define X11DRV_GET_DRAWABLE 1

/* Wine works in this way: wineserver stores the all window positions
 * in (somewhat fictitious) "screen coordinates", and does not itself
 * interact with X11.
 *
 * Instead, it is the responsibliity of the owner of the X window to
 * handle ConfigureNotify and inform wineserver that the window
 * moved.
 *
 * This means that we can freely look at window positions non-atomically,
 * since they won't get updated until we return and the application
 * processes the Win32 message queue.
 *
 * Of course, if this thread doesn't own the window, we are screwed.
 *
 * It might be a good idea to integrate this code in winex11.drv.
 */

struct WineDXGIBackend
{
		const IGalliumDXGIBackendVtbl *vtbl_IGalliumDXGIBackend;
		LONG ref;
};

static HRESULT STDMETHODCALLTYPE WineDXGIBackend_BeginPresent(
	IGalliumDXGIBackend* This,
	HWND hwnd,
	void** ppresent_cookie,
	void** pwindow,
	RECT* prect,
	RGNDATA** prgndata,
	BOOL* ppreserve_aspect_ratio)
{
	/* this is the parent HWND which actually has an X11 window associated */
	HWND x11_hwnd;
	HDC hdc;
	RECT client_rect;
	POINT x11_hwnd_origin_from_screen;
	Drawable drawable;
	POINT hwnd_origin_from_screen;
	HRGN hrgn;
	unsigned code = X11DRV_GET_DRAWABLE;
	unsigned rgndata_size;
	RGNDATA* rgndata;
	RECT rgn_box;
	int rgn_box_type;

	hdc = GetDC(hwnd);
	GetDCOrgEx(hdc, &hwnd_origin_from_screen);
	hrgn = CreateRectRgn(0, 0, 0, 0);
	GetRandomRgn(hdc, hrgn, SYSRGN);
	rgn_box_type = GetRgnBox(hrgn, &rgn_box);

	/* the coordinate system differs depending on whether Wine is
	 * pretending to be Win9x or WinNT, so match that behavior.
	 */
	if (!(GetVersion() & 0x80000000))
		OffsetRgn(hrgn, -hwnd_origin_from_screen.x, -hwnd_origin_from_screen.y);
	ReleaseDC(hwnd, hdc);

	if(rgn_box_type == NULLREGION)
	{
		DeleteObject(hrgn);
		return DXGI_STATUS_OCCLUDED;
	}

	rgndata_size = GetRegionData(hrgn, 0, NULL);
	rgndata = HeapAlloc(GetProcessHeap(), 0, rgndata_size);
	GetRegionData(hrgn, rgndata_size, rgndata);
	DeleteObject(hrgn);
	*prgndata = rgndata;

	x11_hwnd = GetAncestor(hwnd, GA_ROOT);
	hdc = GetDC(x11_hwnd);
	ExtEscape(hdc, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, sizeof(drawable), (LPTSTR)&drawable);

	GetDCOrgEx(hdc, &x11_hwnd_origin_from_screen);
	ReleaseDC(x11_hwnd, hdc);

	*pwindow = (void*)drawable;
	GetClientRect(hwnd, &client_rect);

	prect->left = hwnd_origin_from_screen.x - x11_hwnd_origin_from_screen.x;
	prect->top = hwnd_origin_from_screen.y - x11_hwnd_origin_from_screen.y;

	prect->right = prect->left + client_rect.right;
	prect->bottom = prect->top + client_rect.bottom;

	// Windows doesn't preserve the aspect ratio
	// TODO: maybe let the user turn this on somehow
	*ppreserve_aspect_ratio = FALSE;

	*ppresent_cookie = rgndata;

	// TODO: check for errors and return them
	return S_OK;
}

static void STDMETHODCALLTYPE WineDXGIBackend_EndPresent(
	IGalliumDXGIBackend* This,
	HWND hwnd,
	void *present_cookie)
{
	HeapFree(GetProcessHeap(), 0, present_cookie);
}

static HRESULT STDMETHODCALLTYPE WineDXGIBackend_TestPresent(
	IGalliumDXGIBackend* This,
	HWND hwnd)
{
	HDC hdc;
	HRGN hrgn;
	RECT rgn_box;
	int rgn_box_type;

	// TODO: is there a simpler way to check this?
	hdc = GetDC(hwnd);
	hrgn = CreateRectRgn(0, 0, 0, 0);
	GetRandomRgn(hdc, hrgn, SYSRGN);
	rgn_box_type = GetRgnBox(hrgn, &rgn_box);
	DeleteObject(hrgn);
	ReleaseDC(hwnd, hdc);

	return rgn_box_type == NULLREGION ? DXGI_STATUS_OCCLUDED : S_OK;
}

static HRESULT STDMETHODCALLTYPE WineDXGIBackend_GetPresentSize(
	IGalliumDXGIBackend* This,
	HWND hwnd,
	unsigned* width,
	unsigned* height)
{
	RECT client_rect;
	GetClientRect(hwnd, &client_rect);
	*width = client_rect.right - client_rect.left;
	*height = client_rect.bottom - client_rect.top;

	// TODO: check for errors and return them
	return S_OK;
}

/* Wine should switch to C++ at least to be able to implement COM interfaces in a sensible way,
 * instead of this ridiculous amount of clumsy duplicated code everywhere
 * C++ exists exactly to avoid having to write the following code */
static ULONG STDMETHODCALLTYPE WineDXGIBackend_AddRef(IGalliumDXGIBackend* This)
{
	return InterlockedIncrement(&((struct WineDXGIBackend*)&This)->ref);
}

static ULONG STDMETHODCALLTYPE WineDXGIBackend_Release(IGalliumDXGIBackend* This)
{
	ULONG v = InterlockedDecrement(&((struct WineDXGIBackend*)&This)->ref);
	if(!v)
		HeapFree(GetProcessHeap(), 0, This);
	return v;
}

static HRESULT WINAPI WineDXGIBackend_QueryInterface(
	IGalliumDXGIBackend* iface,
	REFIID riid,
	void** ppvObject)
{
	if (IsEqualGUID(riid, &IID_IUnknown)
		|| IsEqualGUID(riid, &IID_IGalliumDXGIBackend))
	{
		WineDXGIBackend_AddRef(iface);
		*ppvObject = iface;
		return S_OK;
	}

	return E_NOINTERFACE;
}

static IGalliumDXGIBackendVtbl WineDXGIBackend_vtbl =
{
	WineDXGIBackend_QueryInterface,
	WineDXGIBackend_AddRef,
	WineDXGIBackend_Release,
	WineDXGIBackend_BeginPresent,
	WineDXGIBackend_EndPresent,
	WineDXGIBackend_TestPresent,
	WineDXGIBackend_GetPresentSize
};

IGalliumDXGIBackend* new_WineDXGIBackend()
{
	struct WineDXGIBackend* backend = HeapAlloc(GetProcessHeap(), 0, sizeof(struct WineDXGIBackend));
	backend->ref = 1;
	backend->vtbl_IGalliumDXGIBackend = &WineDXGIBackend_vtbl;
	return (IGalliumDXGIBackend*)backend;
}

static void install_wine_dxgi_backend()
{
	IGalliumDXGIBackend* backend = new_WineDXGIBackend();
	HWND root = GetDesktopWindow();
	unsigned code = X11DRV_GET_DISPLAY;
	Display* dpy;
	HDC hdc;

	hdc = GetDC(root);
	ExtEscape(hdc, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, sizeof(dpy), (LPTSTR)&dpy);
	ReleaseDC(root, hdc);

	GalliumDXGIUseX11Display(dpy, backend);
	GalliumDXGIMakeDefault();
	GalliumDXGIUseNothing();
	backend->lpVtbl->Release(backend);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_WINE_PREATTACH:
		return TRUE;
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);
		install_wine_dxgi_backend();
		break;
        case DLL_PROCESS_DETACH:
        	break;
	default:
        	break;
	}

	return TRUE;
}
