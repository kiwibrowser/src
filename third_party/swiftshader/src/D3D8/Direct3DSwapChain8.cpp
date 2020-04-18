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

#include "Direct3DSwapChain8.hpp"

#include "Direct3DDevice8.hpp"
#include "Config.hpp"
#include "Configurator.hpp"
#include "Debug.hpp"

#include "FrameBufferDD.hpp"
#include "FrameBufferGDI.hpp"

namespace D3D8
{
	Direct3DSwapChain8::Direct3DSwapChain8(Direct3DDevice8 *device, D3DPRESENT_PARAMETERS *presentParameters) : device(device), presentParameters(*presentParameters)
	{
		frameBuffer = 0;

		for(int i = 0; i < 3; i++)
		{
			backBuffer[i] = 0;
		}

		reset(presentParameters);
	}

	Direct3DSwapChain8::~Direct3DSwapChain8()
	{
		release();
	}

	long Direct3DSwapChain8::QueryInterface(const IID &iid, void **object)
	{
		TRACE("");

		if(iid == IID_IDirect3DSwapChain8 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DSwapChain8::AddRef()
	{
		TRACE("");

		return Unknown::AddRef();
	}

	unsigned long Direct3DSwapChain8::Release()
	{
		TRACE("");

		return Unknown::Release();
	}

	long Direct3DSwapChain8::Present(const RECT *sourceRect, const RECT *destRect, HWND destWindowOverride, const RGNDATA *dirtyRegion)
	{
		TRACE("");

		#if PERF_PROFILE
			profiler.nextFrame();
		#endif

		if(!sourceRect && !destRect)   // FIXME: More cases?
		{
			frameBuffer->flip(destWindowOverride, backBuffer[0]);
		}
		else   // TODO: Check for SWAPEFFECT_COPY
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

			frameBuffer->blit(destWindowOverride, backBuffer[0], sourceRect ? &sRect : nullptr, destRect ? &dRect : nullptr);
		}

		return D3D_OK;
	}

	long Direct3DSwapChain8::GetBackBuffer(unsigned int index, D3DBACKBUFFER_TYPE type, IDirect3DSurface8 **backBuffer)
	{
		TRACE("");

		if(!backBuffer/* || type != D3DBACKBUFFER_TYPE_MONO*/)
		{
			return INVALIDCALL();
		}

		if(index >= 3 || this->backBuffer[index] == 0)
		{
			return INVALIDCALL();
		}

		this->backBuffer[index]->AddRef();
		*backBuffer = this->backBuffer[index];

		return D3D_OK;
	}

	void Direct3DSwapChain8::reset(D3DPRESENT_PARAMETERS *presentParameters)
	{
		release();

		this->presentParameters = *presentParameters;

		ASSERT(presentParameters->BackBufferCount <= 3);   // Maximum of three back buffers

		if(presentParameters->BackBufferCount == 0)
		{
			presentParameters->BackBufferCount = 1;
		}

		D3DDEVICE_CREATION_PARAMETERS creationParameters;
		device->GetCreationParameters(&creationParameters);

		HWND windowHandle = presentParameters->hDeviceWindow ? presentParameters->hDeviceWindow : creationParameters.hFocusWindow;

		int width = 0;
		int height = 0;

		if(presentParameters->Windowed && (presentParameters->BackBufferHeight == 0 || presentParameters->BackBufferWidth == 0))
		{
			RECT rectangle;
			GetClientRect(windowHandle, &rectangle);

			width = rectangle.right - rectangle.left;
			height = rectangle.bottom - rectangle.top;
		}
		else
		{
			width = presentParameters->BackBufferWidth;
			height = presentParameters->BackBufferHeight;
		}

		frameBuffer = createFrameBufferWin(windowHandle, presentParameters->BackBufferWidth, presentParameters->BackBufferHeight, presentParameters->Windowed == FALSE, true);

		lockable = presentParameters->Flags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

		backBuffer[0] = 0;
		backBuffer[1] = 0;
		backBuffer[2] = 0;

		for(int i = 0; i < (int)presentParameters->BackBufferCount; i++)
		{
			backBuffer[i] = new Direct3DSurface8(device, this, width, height, presentParameters->BackBufferFormat, D3DPOOL_DEFAULT, presentParameters->MultiSampleType, lockable, D3DUSAGE_RENDERTARGET);
			backBuffer[i]->bind();
		}
	}

	void Direct3DSwapChain8::release()
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

	void Direct3DSwapChain8::screenshot(void *destBuffer)
	{
		frameBuffer->screenshot(destBuffer);
	}

	void Direct3DSwapChain8::setGammaRamp(sw::GammaRamp *gammaRamp, bool calibrate)
	{
		frameBuffer->setGammaRamp(gammaRamp, calibrate);
	}

	void Direct3DSwapChain8::getGammaRamp(sw::GammaRamp *gammaRamp)
	{
		frameBuffer->getGammaRamp(gammaRamp);
	}

	void *Direct3DSwapChain8::lockBackBuffer(int index)
	{
		return backBuffer[index]->lockInternal(0, 0, 0, sw::LOCK_READWRITE, sw::PUBLIC);   // FIXME: External
	}

	void Direct3DSwapChain8::unlockBackBuffer(int index)
	{
		backBuffer[index]->unlockInternal();   // FIXME: External
	}
}
