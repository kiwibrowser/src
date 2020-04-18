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

#include "dxgi_private.h"
extern "C" {
#include "native.h"
#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_simple_shaders.h"
#include "pipe/p_shader_tokens.h"
}
#include <iostream>
#include <memory>

struct GalliumDXGIOutput;
struct GalliumDXGIAdapter;
struct GalliumDXGISwapChain;
struct GalliumDXGIFactory;

static HRESULT GalliumDXGISwapChainCreate(GalliumDXGIFactory* factory, IUnknown* device, const DXGI_SWAP_CHAIN_DESC& desc, IDXGISwapChain** out_swap_chain);
static HRESULT GalliumDXGIAdapterCreate(GalliumDXGIFactory* adapter, const struct native_platform* platform, void* dpy, IDXGIAdapter1** out_adapter);
static HRESULT GalliumDXGIOutputCreate(GalliumDXGIAdapter* adapter, const std::string& name, const struct native_connector* connector, IDXGIOutput** out_output);
static void GalliumDXGISwapChainRevalidate(IDXGISwapChain* swap_chain);

template<typename Base = IDXGIObject, typename Parent = IDXGIObject>
struct GalliumDXGIObject : public GalliumPrivateDataComObject<Base>
{
	ComPtr<Parent> parent;

	GalliumDXGIObject(Parent* p_parent = 0)
	{
		this->parent = p_parent;
	}

	virtual HRESULT STDMETHODCALLTYPE GetParent(
		REFIID riid,
		void **out_parent)
	{
		return parent->QueryInterface(riid, out_parent);
	}
};

COM_INTERFACE(IGalliumDXGIBackend, IUnknown)

// TODO: somehow check whether the window is fully obscured or not
struct GalliumDXGIIdentityBackend : public GalliumComObject<IGalliumDXGIBackend>
{
	virtual HRESULT STDMETHODCALLTYPE BeginPresent(
		HWND hwnd,
		void** present_cookie,
		void** window,
		RECT *rect,
		RGNDATA **rgndata,
		BOOL* preserve_aspect_ratio
	)
	{
		*window = (void*)hwnd;
		rect->left = 0;
		rect->top = 0;
		rect->right = INT_MAX;
		rect->bottom = INT_MAX;
		*rgndata = 0;

		// yes, because we like things looking good
		*preserve_aspect_ratio = TRUE;
		*present_cookie = 0;
		return S_OK;
	}

	virtual void STDMETHODCALLTYPE EndPresent(
		HWND hwnd,
		void* present_cookie
	)
	{}

	virtual HRESULT STDMETHODCALLTYPE TestPresent(HWND hwnd)
	{
		return S_OK;
	}

        virtual HRESULT STDMETHODCALLTYPE GetPresentSize(
                HWND hwnd,
                unsigned* width,
                unsigned* height
        )
        {
                *width = 0;
                *height = 0;
                return S_OK;
        }
};

// TODO: maybe install an X11 error hook, so we can return errors properly
struct GalliumDXGIX11IdentityBackend : public GalliumDXGIIdentityBackend
{
	Display* dpy;

	GalliumDXGIX11IdentityBackend(Display* dpy)
	: dpy(dpy)
	{}

	virtual HRESULT STDMETHODCALLTYPE GetPresentSize(
		HWND hwnd,
		unsigned* width,
		unsigned* height
	)
        {
		XWindowAttributes xwa;
		XGetWindowAttributes(dpy, (Window)hwnd, &xwa);
		*width = xwa.width;
		*height = xwa.height;
		return S_OK;
        }
};

struct GalliumDXGIFactory : public GalliumDXGIObject<IDXGIFactory1, IUnknown>
{
	HWND associated_window;
	const struct native_platform* platform;
	void* display;
	ComPtr<IGalliumDXGIBackend> backend;
	void* resolver_cookie;

	GalliumDXGIFactory(const struct native_platform* platform, void* display, IGalliumDXGIBackend* p_backend)
	: GalliumDXGIObject<IDXGIFactory1, IUnknown>((IUnknown*)NULL), platform(platform), display(display)
	 {
		if(p_backend)
			backend = p_backend;
		else if(!strcmp(platform->name, "X11"))
			backend.reset(new GalliumDXGIX11IdentityBackend((Display*)display));
		else
			backend.reset(new GalliumDXGIIdentityBackend());
	}

	virtual HRESULT STDMETHODCALLTYPE EnumAdapters(
		UINT adapter,
		IDXGIAdapter **out_adapter)
	{
		return EnumAdapters1(adapter, (IDXGIAdapter1**)out_adapter);
	}

	virtual HRESULT STDMETHODCALLTYPE EnumAdapters1(
		UINT adapter,
		IDXGIAdapter1 **out_adapter)
	{
		*out_adapter = 0;
		if(adapter == 0)
		{
			return GalliumDXGIAdapterCreate(this, platform, display, out_adapter);
		}
#if 0
		// TODO: enable this
		if(platform == native_get_x11_platform())
		{
			unsigned nscreens = ScreenCount((Display*)display);
			if(adapter < nscreens)
			{
				unsigned def_screen = DefaultScreen(display);
				if(adapter <= def_screen)
					--adapter;
				*out_adapter = GalliumDXGIAdapterCreate(this, platform, display, adapter);
				return S_OK;
			}
		}
#endif
		return DXGI_ERROR_NOT_FOUND;
	}

	/* TODO: this is a mysterious underdocumented magic API
	 * Can we have multiple windows associated?
	 * Can we have multiple windows associated if we use multiple factories?
	 * If so, what should GetWindowAssociation return?
	 * If not, does a new swapchain steal the association?
	 * Does this act for existing swapchains? For new swapchains?
	 */
	virtual HRESULT STDMETHODCALLTYPE MakeWindowAssociation(
		HWND window_handle,
		UINT flags)
	{
		/* TODO: actually implement, for Wine, X11 and KMS*/
		associated_window = window_handle;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetWindowAssociation(
		HWND *pwindow_handle)
	{
		*pwindow_handle = associated_window;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateSwapChain(
		IUnknown *device,
		DXGI_SWAP_CHAIN_DESC *desc,
		IDXGISwapChain **out_swap_chain)
	{
		return GalliumDXGISwapChainCreate(this, device, *desc, out_swap_chain);
	}

	virtual HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(
		HMODULE module,
		IDXGIAdapter **out_adapter)
	{
		/* TODO: ignore the module, and just create a Gallium software screen */
		*out_adapter = 0;
		return E_NOTIMPL;
	}

	/* TODO: support hotplug */
	virtual BOOL STDMETHODCALLTYPE IsCurrent( void)
	{
		return TRUE;
	}
};

struct GalliumDXGIAdapter
	: public GalliumMultiComObject<
		 GalliumDXGIObject<IDXGIAdapter1, GalliumDXGIFactory>,
		 IGalliumAdapter>
{
	struct native_display* display;
	const struct native_config** configs;
	std::unordered_multimap<unsigned, unsigned> configs_by_pipe_format;
	std::unordered_map<unsigned, unsigned> configs_by_native_visual_id;
	const struct native_connector** connectors;
	unsigned num_configs;
	DXGI_ADAPTER_DESC1 desc;
	std::vector<ComPtr<IDXGIOutput> > outputs;
	int num_outputs;

	GalliumDXGIAdapter(GalliumDXGIFactory* factory, const struct native_platform* platform, void* dpy)
	{
		this->parent = factory;

		display = platform->create_display(dpy, FALSE);
		if(!display)
                   display = platform->create_display(dpy, TRUE);
                if (display) {
                   display->user_data = this;
                   if (!display->init_screen(display)) {
                      display->destroy(display);
                      display = NULL;
                   }
                }
                if(!display)
			throw E_FAIL;
		memset(&desc, 0, sizeof(desc));
		std::string s = std::string("GalliumD3D on ") + display->screen->get_name(display->screen) + " by " + display->screen->get_vendor(display->screen);

		/* hopefully no one will decide to use UTF-8 in Gallium name/vendor strings */
		for(int i = 0; i < std::min((int)s.size(), 127); ++i)
			desc.Description[i] = (WCHAR)s[i];

		// TODO: add an interface to get these; for now, return mid/low values
		desc.DedicatedVideoMemory = 256 << 20;
		desc.DedicatedSystemMemory = 256 << 20;
		desc.SharedSystemMemory = 1024 << 20;

		// TODO: we should actually use an unique ID instead
		*(void**)&desc.AdapterLuid = dpy;

		configs = display->get_configs(display, (int*)&num_configs);
		for(unsigned i = 0; i < num_configs; ++i)
		{
			if(configs[i]->window_bit)
			{
				configs_by_pipe_format.insert(std::make_pair(configs[i]->color_format, i));
				configs_by_native_visual_id[configs[i]->native_visual_id] = i;
			}
		}

		connectors = 0;
		num_outputs = 0;

		if(display->modeset)
		{
			int num_crtcs;

			connectors = display->modeset->get_connectors(display, &num_outputs, &num_crtcs);
			if(!connectors)
				num_outputs = 0;
			else if(!num_outputs)
			{
				free(connectors);
				connectors = 0;
			}
		}
		if(!num_outputs)
			num_outputs = 1;
	}

	static void handle_invalid_surface(struct native_display *ndpy, struct native_surface *nsurf, unsigned int seq_num)
	{
		GalliumDXGISwapChainRevalidate((IDXGISwapChain*)nsurf->user_data);
	}

	~GalliumDXGIAdapter()
	{
		display->destroy(display);
		free(configs);
		free(connectors);
	}

	virtual HRESULT STDMETHODCALLTYPE EnumOutputs(
		UINT output,
		IDXGIOutput **out_output)
	{
		if(output >= (unsigned)num_outputs)
			return DXGI_ERROR_NOT_FOUND;

		if(connectors)
		{
			std::ostringstream ss;
			ss << "output #" << output;
			return GalliumDXGIOutputCreate(this, ss.str(), connectors[output], out_output);
		}
		else
			return GalliumDXGIOutputCreate(this, "Unique output", NULL, out_output);
	}

	virtual HRESULT STDMETHODCALLTYPE GetDesc(
		DXGI_ADAPTER_DESC *desc)
	{
		memcpy(desc, &desc, sizeof(*desc));
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDesc1(
		DXGI_ADAPTER_DESC1 *desc)
	{
		memcpy(desc, &desc, sizeof(*desc));
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CheckInterfaceSupport(
		REFGUID interface_name,
		LARGE_INTEGER *u_m_d_version)
	{
		// these number was taken from Windows 7 with Catalyst 10.8: its meaning is unclear
		if(interface_name == IID_ID3D11Device || interface_name == IID_ID3D10Device1 || interface_name == IID_ID3D10Device)
		{
			u_m_d_version->QuadPart = 0x00080011000a0411ULL;
			return S_OK;
		}
		return DXGI_ERROR_UNSUPPORTED;
	}

	pipe_screen* STDMETHODCALLTYPE GetGalliumScreen()
	{
		return display->screen;
	}

	pipe_screen* STDMETHODCALLTYPE GetGalliumReferenceSoftwareScreen()
	{
		// TODO: give a softpipe screen
		return display->screen;
	}

	pipe_screen* STDMETHODCALLTYPE GetGalliumFastSoftwareScreen()
	{
		// TODO: give an llvmpipe screen
		return display->screen;
	}
};


struct GalliumDXGIOutput : public GalliumDXGIObject<IDXGIOutput, GalliumDXGIAdapter>
{
	DXGI_OUTPUT_DESC desc;
	const struct native_mode** modes;
	DXGI_MODE_DESC* dxgi_modes;
	unsigned num_modes;
	const struct native_connector* connector;
	DXGI_GAMMA_CONTROL* gamma;

	GalliumDXGIOutput(GalliumDXGIAdapter* adapter, std::string name, const struct native_connector* connector = 0)
	: GalliumDXGIObject<IDXGIOutput, GalliumDXGIAdapter>(adapter), connector(connector)
	{
		memset(&desc, 0, sizeof(desc));
		for(unsigned i = 0; i < std::min(name.size(), sizeof(desc.DeviceName) - 1); ++i)
			desc.DeviceName[i] = name[i];
		desc.AttachedToDesktop = TRUE;
		/* TODO: should put an HMONITOR in desc.Monitor */

		gamma = 0;
		num_modes = 0;
		modes = 0;
		if(connector)
		{
			modes = parent->display->modeset->get_modes(parent->display, connector, (int*)&num_modes);
			if(modes && num_modes)
			{
				dxgi_modes = new DXGI_MODE_DESC[num_modes];
				for(unsigned i = 0; i < num_modes; ++i)
				{
					dxgi_modes[i].Width = modes[i]->width;
					dxgi_modes[i].Height = modes[i]->height;
					dxgi_modes[i].RefreshRate.Numerator = modes[i]->refresh_rate;
					dxgi_modes[i].RefreshRate.Denominator = 1;
					dxgi_modes[i].Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
					dxgi_modes[i].ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
				}
			}
			else
			{
				if(modes)
				{
					free(modes);
					modes = 0;
				}
				goto use_fake_mode;
			}
		}
		else
		{
use_fake_mode:
			dxgi_modes = new DXGI_MODE_DESC[1];
			dxgi_modes[0].Width = 1920;
			dxgi_modes[0].Height = 1200;
			dxgi_modes[0].RefreshRate.Numerator = 60;
			dxgi_modes[0].RefreshRate.Denominator = 1;
			dxgi_modes[0].Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
			dxgi_modes[0].ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		}
	}

	~GalliumDXGIOutput()
	{
		delete [] dxgi_modes;
		free(modes);
		if(gamma)
			delete gamma;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDesc(
		DXGI_OUTPUT_DESC *out_desc)
	{
		*out_desc = desc;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDisplayModeList(
		DXGI_FORMAT enum_format,
		UINT flags,
		UINT *pcount,
		DXGI_MODE_DESC *desc)
	{
		/* TODO: should we return DXGI_ERROR_NOT_CURRENTLY_AVAILABLE when we don't
		 * support modesetting instead of fake modes?
		 */
		pipe_format format = dxgi_to_pipe_format[enum_format];
		if(parent->configs_by_pipe_format.count(format))
		{
			if(!desc)
			{
				*pcount = num_modes;
				return S_OK;
			}

			unsigned copy_modes = std::min(num_modes, *pcount);
			for(unsigned i = 0; i < copy_modes; ++i)
			{
				desc[i] = dxgi_modes[i];
				desc[i].Format = enum_format;
			}
			*pcount = num_modes;

			if(copy_modes < num_modes)
				return DXGI_ERROR_MORE_DATA;
			else
				return S_OK;
		}
		else
		{
			*pcount = 0;
			return S_OK;
		}
	}

	virtual HRESULT STDMETHODCALLTYPE FindClosestMatchingMode(
		const DXGI_MODE_DESC *pModeToMatch,
		DXGI_MODE_DESC *closest_match,
		IUnknown *concerned_device)
	{
		/* TODO: actually implement this */
		DXGI_FORMAT dxgi_format = pModeToMatch->Format;
		enum pipe_format format = dxgi_to_pipe_format[dxgi_format];
		init_pipe_to_dxgi_format();
		if(!parent->configs_by_pipe_format.count(format))
		{
			if(!concerned_device)
				return E_FAIL;
			else
			{
				format = parent->configs[0]->color_format;
				dxgi_format = pipe_to_dxgi_format[format];
			}
		}

		*closest_match = dxgi_modes[0];
		closest_match->Format = dxgi_format;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE WaitForVBlank( void)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE TakeOwnership(
		IUnknown *device,
		BOOL exclusive)
	{
		return S_OK;
	}

	virtual void STDMETHODCALLTYPE ReleaseOwnership( void)
	{
	}

	virtual HRESULT STDMETHODCALLTYPE GetGammaControlCapabilities(
		DXGI_GAMMA_CONTROL_CAPABILITIES *gamma_caps)
	{
		memset(gamma_caps, 0, sizeof(*gamma_caps));
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetGammaControl(
			const DXGI_GAMMA_CONTROL *pArray)
	{
		if(!gamma)
			gamma = new DXGI_GAMMA_CONTROL;
		*gamma = *pArray;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetGammaControl(
			DXGI_GAMMA_CONTROL *pArray)
	{
		if(gamma)
			*pArray = *gamma;
		else
		{
			pArray->Scale.Red = 1;
			pArray->Scale.Green = 1;
			pArray->Scale.Blue = 1;
			pArray->Offset.Red = 0;
			pArray->Offset.Green = 0;
			pArray->Offset.Blue = 0;
			for(unsigned i = 0; i <= 1024; ++i)
				pArray->GammaCurve[i].Red = pArray->GammaCurve[i].Green = pArray->GammaCurve[i].Blue = (float)i / 1024.0;
		}
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetDisplaySurface(
		IDXGISurface *scanout_surface)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDisplaySurfaceData(
		IDXGISurface *destination)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(
		DXGI_FRAME_STATISTICS *stats)
	{
		memset(stats, 0, sizeof(*stats));
#ifdef _WIN32
		QueryPerformanceCounter(&stats->SyncQPCTime);
#endif
		return E_NOTIMPL;
	}
};

/* Swap chain are rather complex, and Microsoft's documentation is rather
 * lacking. As far as I know, this is the most thorough publicly available
 * description of how swap chains work, based on multiple sources and
 * experimentation.
 *
 * There are two modes (called "swap effects") that a swap chain can operate in:
 * discard and sequential.
 *
 * In discard mode, things always look as if there is a single buffer, which
 * you can get with GetBuffers(0).
 * The 2D texture returned by GetBuffers(0) and can only be
 * used as a render target view and for resource copies, since no CPU access
 * flags are set and only the D3D11_BIND_RENDER_TARGET bind flag is set.
 * On Present, it is copied to the actual display
 * surface and the contents become undefined.
 * D3D may internally use multiple buffers, but you can't observe this, except
 * by looking at the buffer contents after Present (but those are undefined).
 * If it uses multiple buffers internally, then it will normally use buffer_count buffers
 * (this has latency implications).
 * Discard mode seems to internally use a single buffer in windowed mode,
 * even if DWM is enabled, and buffer_count buffers in fullscreen mode.
 *
 * In sequential mode, the runtime alllocates buffer_count buffers.
 * You can get each with GetBuffers(n).
 * GetBuffers(0) ALWAYS points to the backbuffer to be presented and has the
 * same usage constraints as the discard mode.
 * GetBuffer(n) with n > 0 points to resources that are identical to buffer 0, but
 * are classified as "read-only resources" (due to DXGI_USAGE_READ_ONLY),
 * meaning that you can't create render target views on them, or use them as
 * a CopyResource/CopySubresourceRegion destination.
 * It appears the only valid operation is to use them as a source for CopyResource
 * and CopySubresourceRegion as well as just waiting for them to become
 * buffer 0 again.
 * Buffer n - 1 is always displayed on screen.
 * When you call Present(), the contents of the buffers are rotated, so that buffer 0
 * goes to buffer n - 1, and is thus displayed, and buffer 1 goes to buffer 0, becomes
 * the accessible back buffer.
 * The resources themselves are NOT rotated, so that you can still render on the
 * same ID3D11Texture2D*, and views based on it, that you got before Present().
 *
 * Present seems to happen by either copying the relevant buffer into the window,
 * or alternatively making it the current one, either by programming the CRTC or
 * by sending the resource name to the DWM compositor.
 *
 * Hence, you can call GetBuffer(0) once and keep using the same ID3D11Texture2D*
 * and ID3D11RenderTargetView* (and other views if needed) you got from it.
 *
 * If the window gets resized, DXGI will then "emulate" all successive presentations,
 * by using a stretched blit automatically.
 * Thus, you should handle WM_SIZE and call ResizeBuffers to update the DXGI
 * swapchain buffers size to the new window size.
 * Doing so requires you to release all GetBuffers() results and anything referencing
 * them, including views and Direct3D11 deferred context command lists (this is
 * documented).
 *
 * How does Microsoft implement the rotation behavior?
 * It turns out that it does it by calling RotateResourceIdentitiesDXGI in the user-mode
 * DDI driver.
 * This will rotate the kernel buffer handle, or possibly rotate the GPU virtual memory
 * mappings.
 *
 * The reason this is done by driver instead of by the runtime appears to be that
 * this is necessary to support driver-provided command list support, since otherwise
 * the command list would not always target the current backbuffer, since it would
 * be done at the driver level, while only the runtime knows about the rotation.
 *
 * OK, so how do we implement this in Gallium?
 *
 * There are three strategies:
 * 1. Use a single buffer, and always copy it to a window system provided buffer, or
 *	just give the buffer to the window system if it supports that
 * 2. Rotate the buffers in the D3D1x implementation, and recreate and rebind the views.
 *	 Don't support driver-provided command lists
 * 3. Add this rotation functionality to the Gallium driver, with the idea that it would rotate
 *	remap GPU virtual memory, so that virtual address are unchanged, but the physical
 *	ones are rotated (so that pushbuffers remain valid).
 *	If the driver does not support this, either fall back to (1), or have a layer doing this,
 *	putting a deferred context layer over this intermediate layer.
 *
 * (2) is not acceptable since it prevents an optimal implementation.
 * (3) is the ideal solution, but it is complicated.
 *
 * Hence, we implement (1) for now, and will switch to (3) later.
 *
 * Note that (1) doesn't really work for DXGI_SWAP_EFFECT_SEQUENTIAL with more
 * than one buffer, so we just pretend we got asked for a single buffer in that case
 * Fortunately, no one seems to rely on that, so we'll just not implement it at first, and
 * later perform the rotation with blits.
 * Once we switch to (3), we'll just use real rotation to do it..
 *
 * DXGI_SWAP_EFFECT_SEQUENTIAL with more than one buffer is of dubious use
 * anyway, since you can only render or write to buffer 0, and other buffers can apparently
 * be used only as sources for copies.
 * I was unable to find any code using it either in DirectX SDK examples, or on the web.
 *
 * It seems the only reason you would use it is to not have to redraw from scratch, while
 * also possibly avoid a copy compared to buffer_count == 1, assuming that your
 * application is OK with having to redraw starting not from the last frame, but from
 * one/two/more frames behind it.
 *
 * A better design would forbid the user specifying buffer_count explicitly, and
 * would instead let the application give an upper bound on how old the buffer can
 * become after presentation, with "infinite" being equivalent to discard.
 * The runtime would then tell the application with frame number the buffer switched to
 * after present.
 * In addition, in a better design, the application would be allowed to specify the
 * number of buffers available, having all them usable for rendering, so that things
 * like video players could efficiently decode frames in parallel.
 * Present would in such a better design gain a way to specify the number of buffers
 * to present.
 *
 * Other miscellaneous info:
 * DXGI_PRESENT_DO_NOT_SEQUENCE causes DXGI to hold the frame for another
 * vblank interval without rotating the resource data.
 *
 * References:
 * "DXGI Overview" in MSDN
 * IDXGISwapChain documentation on MSDN
 * "RotateResourceIdentitiesDXGI" on MSDN
 * http://forums.xna.com/forums/p/42362/266016.aspx
 */

static float quad_data[] = {
	-1, -1, 0, 0,
	-1, 1, 0, 1,
	1, 1, 1, 1,
	1, -1, 1, 0,
};

struct dxgi_blitter
{
	pipe_context* pipe;
	bool normalized;
	void* fs;
	void* vs;
	void* sampler[2];
	void* elements;
	void* blend;
	void* rasterizer;
	void* zsa;
	struct pipe_clip_state clip;
	struct pipe_vertex_buffer vbuf;
	struct pipe_draw_info draw;

	dxgi_blitter(pipe_context* pipe)
	: pipe(pipe)
	{
		//normalized = !!pipe->screen->get_param(pipe, PIPE_CAP_NPOT_TEXTURES);
		// TODO: need to update buffer in unnormalized case
		normalized = true;

		struct pipe_rasterizer_state rs_state;
		memset(&rs_state, 0, sizeof(rs_state));
		rs_state.cull_face = PIPE_FACE_NONE;
		rs_state.gl_rasterization_rules = 1;
		rs_state.depth_clip = 1;
		rs_state.flatshade = 1;
		rasterizer = pipe->create_rasterizer_state(pipe, &rs_state);

		struct pipe_blend_state blendd;
		memset(&blendd, 0, sizeof(blendd));
		blendd.rt[0].colormask = PIPE_MASK_RGBA;
		blend = pipe->create_blend_state(pipe, &blendd);

		struct pipe_depth_stencil_alpha_state zsad;
		memset(&zsad, 0, sizeof(zsad));
		zsa = pipe->create_depth_stencil_alpha_state(pipe, &zsad);

		struct pipe_vertex_element velem[2];
		memset(&velem[0], 0, sizeof(velem[0]) * 2);
		velem[0].src_offset = 0;
		velem[0].src_format = PIPE_FORMAT_R32G32_FLOAT;
		velem[1].src_offset = 8;
		velem[1].src_format = PIPE_FORMAT_R32G32_FLOAT;
		elements = pipe->create_vertex_elements_state(pipe, 2, &velem[0]);

		for(unsigned stretch = 0; stretch < 2; ++stretch)
		{
			struct pipe_sampler_state sampler_state;
			memset(&sampler_state, 0, sizeof(sampler_state));
			sampler_state.min_img_filter = stretch ? PIPE_TEX_FILTER_LINEAR : PIPE_TEX_FILTER_NEAREST;
			sampler_state.mag_img_filter = stretch ? PIPE_TEX_FILTER_LINEAR : PIPE_TEX_FILTER_NEAREST;
			sampler_state.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
			sampler_state.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
			sampler_state.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
			sampler_state.normalized_coords = normalized;

			sampler[stretch] = pipe->create_sampler_state(pipe, &sampler_state);
		}

		fs = util_make_fragment_tex_shader(pipe, normalized ? TGSI_TEXTURE_2D : TGSI_TEXTURE_RECT, TGSI_INTERPOLATE_LINEAR);

		const unsigned semantic_names[] = { TGSI_SEMANTIC_POSITION, TGSI_SEMANTIC_GENERIC };
		const unsigned semantic_indices[] = { 0, 0 };
		vs = util_make_vertex_passthrough_shader(pipe, 2, semantic_names, semantic_indices);

		vbuf.buffer = pipe_buffer_create(pipe->screen, PIPE_BIND_VERTEX_BUFFER,
						 PIPE_USAGE_STREAM, sizeof(quad_data));
		vbuf.buffer_offset = 0;
		vbuf.stride = 4 * sizeof(float);
		pipe_buffer_write(pipe, vbuf.buffer, 0, sizeof(quad_data), quad_data);

		memset(&clip, 0, sizeof(clip));

		memset(&draw, 0, sizeof(draw));
		draw.mode = PIPE_PRIM_QUADS;
		draw.count = 4;
		draw.instance_count = 1;
		draw.max_index = ~0;
	}

	void blit(struct pipe_surface* surf, struct pipe_sampler_view* view, unsigned x, unsigned y, unsigned w, unsigned h)
	{
		struct pipe_framebuffer_state fb;
		memset(&fb, 0, sizeof(fb));
		fb.nr_cbufs = 1;
		fb.cbufs[0] = surf;
		fb.width = surf->width;
		fb.height = surf->height;

		struct pipe_viewport_state viewport;
		float half_width = w * 0.5f;
		float half_height = h * 0.5f;
		viewport.scale[0] = half_width;
		viewport.scale[1] = half_height;
		viewport.scale[2] = 1.0f;
		viewport.scale[3] = 1.0f;
		viewport.translate[0] = x + half_width;
		viewport.translate[1] = y + half_height;
		viewport.translate[2] = 0.0f;
		viewport.translate[3] = 1.0f;

		bool stretch = view->texture->width0 != w || view->texture->height0 != h;
		if(pipe->render_condition)
			pipe->render_condition(pipe, 0, 0);
		pipe->set_framebuffer_state(pipe, &fb);
		pipe->bind_fragment_sampler_states(pipe, 1, &sampler[stretch]);
		pipe->set_viewport_state(pipe, &viewport);
		pipe->set_clip_state(pipe, &clip);
		pipe->bind_rasterizer_state(pipe, rasterizer);
		pipe->bind_depth_stencil_alpha_state(pipe, zsa);
		pipe->bind_blend_state(pipe, blend);
		pipe->bind_vertex_elements_state(pipe, elements);
		pipe->set_vertex_buffers(pipe, 1, &vbuf);
		pipe->bind_fs_state(pipe, fs);
		pipe->bind_vs_state(pipe, vs);
		if(pipe->bind_gs_state)
			pipe->bind_gs_state(pipe, 0);
		if(pipe->set_stream_output_targets)
			pipe->set_stream_output_targets(pipe, 0, NULL, 0);
		pipe->set_fragment_sampler_views(pipe, 1, &view);

		pipe->draw_vbo(pipe, &draw);
	}

	~dxgi_blitter()
	{
		pipe->delete_blend_state(pipe, blend);
		pipe->delete_rasterizer_state(pipe, rasterizer);
		pipe->delete_depth_stencil_alpha_state(pipe, zsa);
		pipe->delete_sampler_state(pipe, sampler[0]);
		pipe->delete_sampler_state(pipe, sampler[1]);
		pipe->delete_vertex_elements_state(pipe, elements);
		pipe->delete_vs_state(pipe, vs);
		pipe->delete_fs_state(pipe, fs);
		pipe->screen->resource_destroy(pipe->screen, vbuf.buffer);
	}
};

struct GalliumDXGISwapChain : public GalliumDXGIObject<IDXGISwapChain, GalliumDXGIFactory>
{
	ComPtr<IDXGIDevice>dxgi_device;
	ComPtr<IGalliumDevice>gallium_device;
	ComPtr<GalliumDXGIAdapter> adapter;
	ComPtr<IDXGIOutput> target;

	DXGI_SWAP_CHAIN_DESC desc;

	struct native_surface* surface;
	const struct native_config* config;

	void* window;
	struct pipe_resource* resources[NUM_NATIVE_ATTACHMENTS];
	int width;
	int height;
	unsigned seq_num;
	bool ever_validated;
	bool needs_validation;
	unsigned present_count;

	ComPtr<IDXGISurface> buffer0;
	struct pipe_resource* gallium_buffer0;
	struct pipe_sampler_view* gallium_buffer0_view;

	struct pipe_context* pipe;
	bool owns_pipe;

	BOOL fullscreen;

	std::auto_ptr<dxgi_blitter> blitter;
	bool formats_compatible;

	GalliumDXGISwapChain(GalliumDXGIFactory* factory, IUnknown* p_device, const DXGI_SWAP_CHAIN_DESC& p_desc)
	: GalliumDXGIObject<IDXGISwapChain, GalliumDXGIFactory>(factory), desc(p_desc), surface(0)
	{
		HRESULT hr;

		hr = p_device->QueryInterface(IID_IGalliumDevice, (void**)&gallium_device);
		if(!SUCCEEDED(hr))
			throw hr;

		hr = p_device->QueryInterface(IID_IDXGIDevice, (void**)&dxgi_device);
		if(!SUCCEEDED(hr))
			throw hr;

		hr = dxgi_device->GetAdapter((IDXGIAdapter**)&adapter);
		if(!SUCCEEDED(hr))
			throw hr;

		memset(resources, 0, sizeof(resources));

		if(desc.SwapEffect == DXGI_SWAP_EFFECT_SEQUENTIAL && desc.BufferCount != 1)
		{
			std::cerr << "Gallium DXGI: if DXGI_SWAP_EFFECT_SEQUENTIAL is specified, only buffer_count == 1 is implemented, but " << desc.BufferCount << " was specified: ignoring this" << std::endl;
			// change the returned desc, so that the application might perhaps notice what we did and react well
			desc.BufferCount = 1;
		}

		pipe = gallium_device->GetGalliumContext();
		owns_pipe = false;
		if(!pipe)
		{
			pipe = adapter->display->screen->context_create(adapter->display->screen, 0);
			owns_pipe = true;
		}

		blitter.reset(new dxgi_blitter(pipe));
		window = 0;

		hr = resolve_zero_width_height(true);
		if(!SUCCEEDED(hr))
			throw hr;
	}

	void init_for_window()
	{
		if(surface)
		{
			surface->destroy(surface);
			surface = 0;
		}

		unsigned config_num;
		if(!strcmp(parent->platform->name, "X11"))
		{
			XWindowAttributes xwa;
			XGetWindowAttributes((Display*)parent->display, (Window)window, &xwa);
			assert(adapter->configs_by_native_visual_id.count(xwa.visual->visualid));
			config_num = adapter->configs_by_native_visual_id[xwa.visual->visualid];
		}
		else
		{
			enum pipe_format format = dxgi_to_pipe_format[desc.BufferDesc.Format];
			if(!adapter->configs_by_pipe_format.count(format))
			{
				if(adapter->configs_by_pipe_format.empty())
					throw E_FAIL;
				// TODO: choose the best match
				format = (pipe_format)adapter->configs_by_pipe_format.begin()->first;
			}
			// TODO: choose the best config
			config_num = adapter->configs_by_pipe_format.find(format)->second;
		}

		config = adapter->configs[config_num];
		surface = adapter->display->create_window_surface(adapter->display, (EGLNativeWindowType)window, config);
		surface->user_data = this;

		width = 0;
		height = 0;
		seq_num = 0;
		present_count = 0;
		needs_validation = true;
		ever_validated = false;

		formats_compatible = util_is_format_compatible(
				util_format_description(dxgi_to_pipe_format[desc.BufferDesc.Format]),
				util_format_description(config->color_format));
	}

	~GalliumDXGISwapChain()
	{
		if(owns_pipe)
			pipe->destroy(pipe);
	}

	virtual HRESULT STDMETHODCALLTYPE GetDevice(
		REFIID riid,
		void **pdevice)
	{
		return dxgi_device->QueryInterface(riid, pdevice);
	}

	HRESULT create_buffer0()
	{
		HRESULT hr;
		ComPtr<IDXGISurface> new_buffer0;
		DXGI_USAGE usage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
		if(desc.SwapEffect == DXGI_SWAP_EFFECT_DISCARD)
			usage |= DXGI_USAGE_DISCARD_ON_PRESENT;
		// for our blitter
		usage |= DXGI_USAGE_SHADER_INPUT;

		DXGI_SURFACE_DESC surface_desc;
		surface_desc.Format = desc.BufferDesc.Format;
		surface_desc.Width = desc.BufferDesc.Width;
		surface_desc.Height = desc.BufferDesc.Height;
		surface_desc.SampleDesc = desc.SampleDesc;
		hr = dxgi_device->CreateSurface(&surface_desc, 1, usage, 0, &new_buffer0);
		if(!SUCCEEDED(hr))
			return hr;

		ComPtr<IGalliumResource> gallium_resource;
		hr = new_buffer0->QueryInterface(IID_IGalliumResource, (void**)&gallium_resource);
		if(!SUCCEEDED(hr))
			return hr;

		struct pipe_resource* new_gallium_buffer0 = gallium_resource->GetGalliumResource();
		if(!new_gallium_buffer0)
			return E_FAIL;

		buffer0.reset(new_buffer0.steal());
		gallium_buffer0 = new_gallium_buffer0;
		struct pipe_sampler_view templat;
		memset(&templat, 0, sizeof(templat));
		templat.texture = gallium_buffer0;
		templat.swizzle_r = 0;
		templat.swizzle_g = 1;
		templat.swizzle_b = 2;
		templat.swizzle_a = 3;
		templat.format = gallium_buffer0->format;
		gallium_buffer0_view = pipe->create_sampler_view(pipe, gallium_buffer0, &templat);
		return S_OK;
	}

	bool validate()
	{
		unsigned new_seq_num;
		needs_validation = false;

		if(!surface->validate(surface, (1 << NATIVE_ATTACHMENT_BACK_LEFT) | (1 << NATIVE_ATTACHMENT_FRONT_LEFT), &new_seq_num, resources, &width, &height))
			return false;

		if(!ever_validated || seq_num != new_seq_num)
		{
			seq_num = new_seq_num;
			ever_validated = true;
		}
		return true;
	}

	HRESULT resolve_zero_width_height(bool force = false)
	{
		if(!force && desc.BufferDesc.Width && desc.BufferDesc.Height)
			return S_OK;

		unsigned width, height;
		HRESULT hr = parent->backend->GetPresentSize(desc.OutputWindow, &width, &height);
		if(!SUCCEEDED(hr))
			return hr;

		// On Windows, 8 is used, and a debug message saying so gets printed
		if(!width)
			width = 8;
		if(!height)
			height = 8;

		if(!desc.BufferDesc.Width)
			desc.BufferDesc.Width = width;
		if(!desc.BufferDesc.Height)
			desc.BufferDesc.Height = height;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Present(
		UINT sync_interval,
		UINT flags)
	{
		HRESULT hr;
		if(flags & DXGI_PRESENT_TEST)
			return parent->backend->TestPresent(desc.OutputWindow);

		if(!buffer0)
		{
			HRESULT hr = create_buffer0();
			if(!SUCCEEDED(hr))
				return hr;
		}

		void* cur_window = 0;
		RECT rect;
		RGNDATA* rgndata;
		BOOL preserve_aspect_ratio;
		unsigned dst_w, dst_h;
		bool db;
		struct pipe_resource* dst;
		struct pipe_resource* src;
		struct pipe_surface* dst_surface;
		struct native_present_control ctrl;

		void* present_cookie;
		hr = parent->backend->BeginPresent(desc.OutputWindow, &present_cookie, &cur_window, &rect, &rgndata, &preserve_aspect_ratio);
		if(hr != S_OK)
			return hr;

		if(!cur_window || rect.left >= rect.right || rect.top >= rect.bottom)
			goto end_present;

		if(cur_window != window)
		{
			window = cur_window;
			init_for_window();
		}

		if(needs_validation)
		{
			if(!validate())
				return DXGI_ERROR_DEVICE_REMOVED;
		}

		db = !!(config->buffer_mask & (1 << NATIVE_ATTACHMENT_BACK_LEFT));
		dst = resources[db ? NATIVE_ATTACHMENT_BACK_LEFT : NATIVE_ATTACHMENT_FRONT_LEFT];
		src = gallium_buffer0;
		dst_surface = 0;

		assert(src);
		assert(dst);

		/* TODO: sharing the context for blitting won't work correctly if queries are active
		 * Hopefully no one is crazy enough to keep queries active while presenting, expecting
		 * sensible results.
		 * We could alternatively force using another context, but that might cause inefficiency issues
		 */

		if((unsigned)rect.right > dst->width0)
			rect.right = dst->width0;
		if((unsigned)rect.bottom > dst->height0)
			rect.bottom = dst->height0;
		if(rect.left > rect.right)
			rect.left = rect.right;
		if(rect.top > rect.bottom)
			rect.top = rect.bottom;

		if(rect.left >= rect.right && rect.top >= rect.bottom)
			goto end_present;

		dst_w = rect.right - rect.left;
		dst_h = rect.bottom - rect.top;

		// TODO: add support for rgndata
//		if(preserve_aspect_ratio || !rgndata)
		if(1)
		{
			unsigned blit_x, blit_y, blit_w, blit_h;
			static const union pipe_color_union black = { { 0, 0, 0, 0 } };

			if(!formats_compatible || src->width0 != dst_w || src->height0 != dst_h) {
				struct pipe_surface templat;
				templat.usage = PIPE_BIND_RENDER_TARGET;
				templat.format = dst->format;
				templat.u.tex.level = 0;
				templat.u.tex.first_layer = 0;
				templat.u.tex.last_layer = 0;
				dst_surface = pipe->create_surface(pipe, dst, &templat);
			}

			if(preserve_aspect_ratio)
			{
				int delta = src->width0 * dst_h - dst_w * src->height0;
				if(delta > 0)
				{
					blit_w = dst_w;
					blit_h = dst_w * src->height0 / src->width0;
				}
				else if(delta < 0)
				{
					blit_w = dst_h * src->width0 / src->height0;
					blit_h = dst_h;
				}
				else
				{
					blit_w = dst_w;
					blit_h = dst_h;
				}

				blit_x = (dst_w - blit_w) >> 1;
				blit_y = (dst_h - blit_h) >> 1;
			}
			else
			{
				blit_x = 0;
				blit_y = 0;
				blit_w = dst_w;
				blit_h = dst_h;
			}

			if(blit_x)
				pipe->clear_render_target(pipe, dst_surface, &black, rect.left, rect.top, blit_x, dst_h);
			if(blit_y)
				pipe->clear_render_target(pipe, dst_surface, &black, rect.left, rect.top, dst_w, blit_y);

			if(formats_compatible && blit_w == src->width0 && blit_h == src->height0)
			{
				pipe_box box;
				box.x = box.y = box.z = 0;
				box.width = blit_w;
				box.height = blit_h;
				box.depth = 1;
				pipe->resource_copy_region(pipe, dst, 0, rect.left, rect.top, 0, src, 0, &box);
			}
			else
			{
				blitter->blit(dst_surface, gallium_buffer0_view, rect.left + blit_x, rect.top + blit_y, blit_w, blit_h);
				if(!owns_pipe)
					gallium_device->RestoreGalliumState();
			}

			if(blit_w != dst_w)
				pipe->clear_render_target(pipe, dst_surface, &black, rect.left + blit_x + blit_w, rect.top, dst_w - blit_x - blit_w, dst_h);
			if(blit_h != dst_h)
				pipe->clear_render_target(pipe, dst_surface, &black, rect.left, rect.top + blit_y + blit_h, dst_w, dst_h - blit_y - blit_h);
		}

		if(dst_surface)
			pipe->surface_destroy(pipe, dst_surface);

                pipe->flush(pipe, 0);

		memset(&ctrl, 0, sizeof(ctrl));
		ctrl.natt = (db) ? NATIVE_ATTACHMENT_BACK_LEFT : NATIVE_ATTACHMENT_FRONT_LEFT;
		if(!surface->present(surface, &ctrl))
			return DXGI_ERROR_DEVICE_REMOVED;

end_present:
		parent->backend->EndPresent(desc.OutputWindow, present_cookie);

		++present_count;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetBuffer(
			UINT Buffer,
			REFIID riid,
			void **ppSurface)
	{
		if(Buffer > 0)
		{
			if(desc.SwapEffect == DXGI_SWAP_EFFECT_SEQUENTIAL)
				std::cerr << "DXGI unimplemented: GetBuffer(n) with n > 0 not supported, returning buffer 0 instead!" << std::endl;
			else
				std::cerr << "DXGI error: in GetBuffer(n), n must be 0 for DXGI_SWAP_EFFECT_DISCARD\n" << std::endl;
		}

		if(!buffer0)
		{
			HRESULT hr = create_buffer0();
			if(!SUCCEEDED(hr))
				return hr;
		}
		return buffer0->QueryInterface(riid, ppSurface);
	}

	/* TODO: implement somehow */
	virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(
		BOOL fullscreen,
		IDXGIOutput *target)
	{
		fullscreen = fullscreen;
		target = target;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(
		BOOL *out_fullscreen,
		IDXGIOutput **out_target)
	{
		if(out_fullscreen)
			*out_fullscreen = fullscreen;
		if(out_target)
			*out_target = target.ref();
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDesc(
		DXGI_SWAP_CHAIN_DESC *out_desc)
	{
		*out_desc = desc;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(
		UINT buffer_count,
		UINT width,
		UINT height,
		DXGI_FORMAT new_format,
		UINT swap_chain_flags)
	{
		if(buffer0)
		{
			buffer0.p->AddRef();
			ULONG v = buffer0.p->Release();
			// we must fail if there are any references to buffer0 other than ours
			if(v > 1)
				return E_FAIL;
			pipe_sampler_view_reference(&gallium_buffer0_view, 0);
			buffer0 = (IUnknown*)NULL;
			gallium_buffer0 = 0;
		}

		if(desc.SwapEffect != DXGI_SWAP_EFFECT_SEQUENTIAL)
			desc.BufferCount = buffer_count;
		desc.BufferDesc.Format = new_format;
		desc.BufferDesc.Width = width;
		desc.BufferDesc.Height = height;
		desc.Flags = swap_chain_flags;
		return resolve_zero_width_height();
	}

	virtual HRESULT STDMETHODCALLTYPE ResizeTarget(
		const DXGI_MODE_DESC *out_new_target_parameters)
	{
		/* TODO: implement */
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(
		IDXGIOutput **out_output)
	{
		*out_output = adapter->outputs[0].ref();
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(
		DXGI_FRAME_STATISTICS *out_stats)
	{
		memset(out_stats, 0, sizeof(*out_stats));
#ifdef _WIN32
		QueryPerformanceCounter(&out_stats->SyncQPCTime);
#endif
		out_stats->PresentCount = present_count;
		out_stats->PresentRefreshCount = present_count;
		out_stats->SyncRefreshCount = present_count;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(
		UINT *last_present_count)
	{
		*last_present_count = present_count;
		return S_OK;
	}
};

static void GalliumDXGISwapChainRevalidate(IDXGISwapChain* swap_chain)
{
	((GalliumDXGISwapChain*)swap_chain)->needs_validation = true;
}

static HRESULT GalliumDXGIAdapterCreate(GalliumDXGIFactory* factory, const struct native_platform* platform, void* dpy, IDXGIAdapter1** out_adapter)
{
	try
	{
		*out_adapter = new GalliumDXGIAdapter(factory, platform, dpy);
		return S_OK;
	}
	catch(HRESULT hr)
	{
		return hr;
	}
}

static HRESULT GalliumDXGIOutputCreate(GalliumDXGIAdapter* adapter, const std::string& name, const struct native_connector* connector, IDXGIOutput** out_output)
{
	try
	{
		*out_output = new GalliumDXGIOutput(adapter, name, connector);
		return S_OK;
	}
	catch(HRESULT hr)
	{
		return hr;
	}
}

static HRESULT GalliumDXGISwapChainCreate(GalliumDXGIFactory* factory, IUnknown* device, const DXGI_SWAP_CHAIN_DESC& desc, IDXGISwapChain** out_swap_chain)
{
	try
	{
		*out_swap_chain = new GalliumDXGISwapChain(factory, device, desc);
		return S_OK;
	}
	catch(HRESULT hr)
	{
		return hr;
	}
}

struct dxgi_binding
{
	const struct native_platform* platform;
	void* display;
	IGalliumDXGIBackend* backend;
};

static dxgi_binding dxgi_default_binding;
static __thread dxgi_binding dxgi_thread_binding;
static const struct native_event_handler dxgi_event_handler = {
   GalliumDXGIAdapter::handle_invalid_surface,
   dxgi_loader_create_drm_screen,
   dxgi_loader_create_sw_screen
};

void STDMETHODCALLTYPE GalliumDXGIUseNothing()
{
	dxgi_thread_binding.platform = 0;
	dxgi_thread_binding.display = 0;
	if(dxgi_thread_binding.backend)
		dxgi_thread_binding.backend->Release();
	dxgi_thread_binding.backend = 0;
}

#ifdef GALLIUM_DXGI_USE_X11
void STDMETHODCALLTYPE GalliumDXGIUseX11Display(Display* dpy, IGalliumDXGIBackend* backend)
{
	GalliumDXGIUseNothing();
	dxgi_thread_binding.platform = native_get_x11_platform(&dxgi_event_handler);
	dxgi_thread_binding.display = dpy;

	if(backend)
	{
		dxgi_thread_binding.backend = backend;
		backend->AddRef();
	}
}
#endif

/*
#ifdef GALLIUM_DXGI_USE_DRM
void STDMETHODCALLTYPE GalliumDXGIUseDRMCard(int fd)
{
	GalliumDXGIUseNothing();
	dxgi_thread_binding.platform = native_get_drm_platform(&dxgi_event_handler);
	dxgi_thread_binding.display = (void*)fd;
	dxgi_thread_binding.backend = 0;
}
#endif

#ifdef GALLIUM_DXGI_USE_FBDEV
void STDMETHODCALLTYPE GalliumDXGIUseFBDev(int fd)
{
	GalliumDXGIUseNothing();
	dxgi_thread_binding.platform = native_get_fbdev_platform(&dxgi_event_handler);
	dxgi_thread_binding.display = (void*)fd;
	dxgi_thread_binding.backend = 0;
}
#endif

#ifdef GALLIUM_DXGI_USE_GDI
void STDMETHODCALLTYPE GalliumDXGIUseHDC(HDC hdc, PFNHWNDRESOLVER resolver, void* resolver_cookie)
{
	GalliumDXGIUseNothing();
	dxgi_thread_binding.platform = native_get_gdi_platform(&dxgi_event_handler);
	dxgi_thread_binding.display = (void*)hdc;
	dxgi_thread_binding.backend = 0;
}
#endif
*/
void STDMETHODCALLTYPE GalliumDXGIMakeDefault()
{
	if(dxgi_default_binding.backend)
		dxgi_default_binding.backend->Release();
	dxgi_default_binding = dxgi_thread_binding;
	if(dxgi_default_binding.backend)
		dxgi_default_binding.backend->AddRef();
}

 /* TODO: why did Microsoft add this? should we do something different for DXGI 1.0 and 1.1?
 * Or perhaps what they actually mean is "only create a single factory in your application"?
 * TODO: should we use a singleton here, so we never have multiple DXGI objects for the same thing? */
 HRESULT STDMETHODCALLTYPE CreateDXGIFactory1(
		REFIID riid,
		void **out_factory
)
 {
	 GalliumDXGIFactory* factory;
	 *out_factory = 0;
	 if(dxgi_thread_binding.platform)
		 factory = new GalliumDXGIFactory(dxgi_thread_binding.platform, dxgi_thread_binding.display, dxgi_thread_binding.backend);
	 else if(dxgi_default_binding.platform)
		 factory = new GalliumDXGIFactory(dxgi_default_binding.platform, dxgi_default_binding.display, dxgi_default_binding.backend);
	 else
		 factory = new GalliumDXGIFactory(native_get_x11_platform(&dxgi_event_handler), NULL, NULL);
	 HRESULT hres = factory->QueryInterface(riid, out_factory);
	 factory->Release();
	 return hres;
 }

 HRESULT STDMETHODCALLTYPE CreateDXGIFactory(
		 REFIID riid,
		 void **out_factor
)
 {
	 return CreateDXGIFactory1(riid, out_factor);
 }
