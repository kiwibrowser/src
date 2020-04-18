// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Direct3DSwapChain9.hpp"

#include "Direct3DDevice9.hpp"
#include "Renderer.hpp"
#include "Timer.hpp"
#include "Resource.hpp"
#include "Configurator.hpp"
#include "Debug.hpp"

#include "FrameBufferDD.hpp"
#include "FrameBufferGDI.hpp"

namespace D3D9
{
	Direct3DSwapChain9::Direct3DSwapChain9(Direct3DDevice9 *device, D3DPRESENT_PARAMETERS *presentParameters) : device(device), presentParameters(*presentParameters)
	{
		frameBuffer = 0;

		for(int i = 0; i < 3; i++)
		{
			backBuffer[i] = 0;
		}

		reset(presentParameters);
	}

	Direct3DSwapChain9::~Direct3DSwapChain9()
	{
		release();
	}

	long Direct3DSwapChain9::QueryInterface(const IID &iid, void **object)
	{
		CriticalSection cs(device);

		TRACE("");

		if(iid == IID_IDirect3DSwapChain9 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DSwapChain9::AddRef()
	{
		TRACE("");

		return Unknown::AddRef();
	}

	unsigned long Direct3DSwapChain9::Release()
	{
		TRACE("");

		return Unknown::Release();
	}

	long Direct3DSwapChain9::Present(const RECT *sourceRect, const RECT *destRect, HWND destWindowOverride, const RGNDATA *dirtyRegion, unsigned long flags)
	{
		CriticalSection cs(device);

		TRACE("");

		#if PERF_PROFILE
			profiler.nextFrame();
		#endif

		#if PERF_HUD
			sw::Renderer *renderer = device->renderer;

			static int64_t frame = sw::Timer::ticks();

			int64_t frameTime = sw::Timer::ticks() - frame;
			frame = sw::Timer::ticks();

			if(frameTime > 0)
			{
				unsigned int *frameBuffer = (unsigned int*)lockBackBuffer(0);   // FIXME: Don't assume A8R8G8B8 mode
				unsigned int stride = backBuffer[0]->getInternalPitchP();

				int thread;
				for(thread = 0; thread < renderer->getThreadCount(); thread++)
				{
					int64_t drawTime = renderer->getVertexTime(thread) + renderer->getSetupTime(thread) + renderer->getPixelTime(thread);

					int vertexPercentage = sw::clamp((int)(100 * renderer->getVertexTime(thread) / frameTime), 0, 100);
					int setupPercentage = sw::clamp((int)(100 * renderer->getSetupTime(thread) / frameTime), 0, 100);
					int pixelPercentage = sw::clamp((int)(100 * renderer->getPixelTime(thread) / frameTime), 0, 100);

					for(int i = 0; i < 100; i++)
					{
						frameBuffer[thread * stride + i] = 0x00000000;
					}

					unsigned int *buffer = frameBuffer;

					for(int i = 0; i < vertexPercentage; i++)
					{
						buffer[thread * stride] = 0x000000FF;
						buffer++;
					}

					for(int i = 0; i < setupPercentage; i++)
					{
						buffer[thread * stride] = 0x0000FF00;
						buffer++;
					}

					for(int i = 0; i < pixelPercentage; i++)
					{
						buffer[thread * stride] = 0x00FF0000;
						buffer++;
					}

					frameBuffer[thread * stride + 100] = 0x00FFFFFF;
				}

				for(int i = 0; i <= 100; i++)
				{
					frameBuffer[thread * stride + i] = 0x00FFFFFF;
				}

				unlockBackBuffer(0);
			}

			renderer->resetTimers();
		#endif

		HWND window = destWindowOverride ? destWindowOverride : presentParameters.hDeviceWindow;

		POINT point;
		GetCursorPos(&point);
		ScreenToClient(window, &point);

		frameBuffer->setCursorPosition(point.x, point.y);

		if(!sourceRect && !destRect)   // FIXME: More cases?
		{
			frameBuffer->flip(window, backBuffer[0]);
		}
		else   // FIXME: Check for SWAPEFFECT_COPY
		{
			sw::Rect sRect(0, 0, 0, 0);
			sw::Rect dRect(0, 0, 0, 0);

			if(sourceRect)
			{
				sRect.x0 = sourceRect->left;
				sRect.y0 = sourceRect->top;
				sRect.x1 = sourceRect->right;
				sRect.y1 = sourceRect->bottom;
			}

			if(destRect)
			{
				dRect.x0 = destRect->left;
				dRect.y0 = destRect->top;
				dRect.x1 = destRect->right;
				dRect.y1 = destRect->bottom;
			}

			frameBuffer->blit(window, backBuffer[0], sourceRect ? &sRect : nullptr, destRect ? &dRect : nullptr);
		}

		return D3D_OK;
	}

	long Direct3DSwapChain9::GetFrontBufferData(IDirect3DSurface9 *destSurface)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!destSurface)
		{
			return INVALIDCALL();
		}

		sw::Surface *dest = static_cast<Direct3DSurface9*>(destSurface);
		void *buffer = dest->lockExternal(0, 0, 0, sw::LOCK_WRITEONLY, sw::PRIVATE);

		frameBuffer->screenshot(buffer);

		dest->unlockExternal();

		return D3D_OK;
	}

	long Direct3DSwapChain9::GetBackBuffer(unsigned int index, D3DBACKBUFFER_TYPE type, IDirect3DSurface9 **backBuffer)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!backBuffer/* || type != D3DBACKBUFFER_TYPE_MONO*/)
		{
			return INVALIDCALL();
		}

		*backBuffer = 0;

		if(index >= 3 || this->backBuffer[index] == 0)
		{
			return INVALIDCALL();
		}

		*backBuffer = this->backBuffer[index];
		this->backBuffer[index]->AddRef();

		return D3D_OK;
	}

	long Direct3DSwapChain9::GetRasterStatus(D3DRASTER_STATUS *rasterStatus)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!rasterStatus)
		{
			return INVALIDCALL();
		}

		bool inVerticalBlank;
		unsigned int scanline;
		bool supported = frameBuffer->getScanline(inVerticalBlank, scanline);

		if(supported)
		{
			rasterStatus->InVBlank = inVerticalBlank;
			rasterStatus->ScanLine = scanline;
		}
		else
		{
			return INVALIDCALL();
		}

		return D3D_OK;
	}

	long Direct3DSwapChain9::GetDisplayMode(D3DDISPLAYMODE *displayMode)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!displayMode)
		{
			return INVALIDCALL();
		}

		device->getAdapterDisplayMode(D3DADAPTER_DEFAULT, displayMode);

		return D3D_OK;
	}

	long Direct3DSwapChain9::GetDevice(IDirect3DDevice9 **device)
	{
		CriticalSection cs(this->device);

		TRACE("");

		if(!device)
		{
			return INVALIDCALL();
		}

		this->device->AddRef();
		*device = this->device;

		return D3D_OK;
	}

	long Direct3DSwapChain9::GetPresentParameters(D3DPRESENT_PARAMETERS *presentParameters)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!presentParameters)
		{
			return INVALIDCALL();
		}

		*presentParameters = this->presentParameters;

		return D3D_OK;
	}

	void Direct3DSwapChain9::reset(D3DPRESENT_PARAMETERS *presentParameters)
	{
		release();

		ASSERT(presentParameters->BackBufferCount <= 3);   // Maximum of three back buffers

		if(presentParameters->BackBufferCount == 0)
		{
			presentParameters->BackBufferCount = 1;
		}

		if(presentParameters->BackBufferFormat == D3DFMT_UNKNOWN)
		{
			D3DDISPLAYMODE displayMode;
			GetDisplayMode(&displayMode);

			presentParameters->BackBufferFormat = displayMode.Format;
		}

		D3DDEVICE_CREATION_PARAMETERS creationParameters;
		device->GetCreationParameters(&creationParameters);

		HWND windowHandle = presentParameters->hDeviceWindow ? presentParameters->hDeviceWindow : creationParameters.hFocusWindow;

		if(presentParameters->Windowed && (presentParameters->BackBufferHeight == 0 || presentParameters->BackBufferWidth == 0))
		{
			RECT rectangle;
			GetClientRect(windowHandle, &rectangle);

			presentParameters->BackBufferWidth = rectangle.right - rectangle.left;
			presentParameters->BackBufferHeight = rectangle.bottom - rectangle.top;
		}

		frameBuffer = createFrameBufferWin(windowHandle, presentParameters->BackBufferWidth, presentParameters->BackBufferHeight, presentParameters->Windowed == FALSE, true);

		lockable = presentParameters->Flags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

		backBuffer[0] = 0;
		backBuffer[1] = 0;
		backBuffer[2] = 0;

		for(int i = 0; i < (int)presentParameters->BackBufferCount; i++)
		{
			backBuffer[i] = new Direct3DSurface9(device, this, presentParameters->BackBufferWidth, presentParameters->BackBufferHeight, presentParameters->BackBufferFormat, D3DPOOL_DEFAULT, presentParameters->MultiSampleType, presentParameters->MultiSampleQuality, lockable, D3DUSAGE_RENDERTARGET);
			backBuffer[i]->bind();
		}

		this->presentParameters = *presentParameters;
	}

	void Direct3DSwapChain9::release()
	{
		delete frameBuffer;
		frameBuffer = 0;

		for(int i = 0; i < 3; i++)
		{
			if(backBuffer[i])
			{
				backBuffer[i]->unbind();
				backBuffer[i] = 0;
			}
		}
	}

	void Direct3DSwapChain9::setGammaRamp(sw::GammaRamp *gammaRamp, bool calibrate)
	{
		frameBuffer->setGammaRamp(gammaRamp, calibrate);
	}

	void Direct3DSwapChain9::getGammaRamp(sw::GammaRamp *gammaRamp)
	{
		frameBuffer->getGammaRamp(gammaRamp);
	}

	void *Direct3DSwapChain9::lockBackBuffer(int index)
	{
		return backBuffer[index]->lockInternal(0, 0, 0, sw::LOCK_READWRITE, sw::PUBLIC);   // FIXME: External
	}

	void Direct3DSwapChain9::unlockBackBuffer(int index)
	{
		backBuffer[index]->unlockInternal();   // FIXME: External
	}
}
