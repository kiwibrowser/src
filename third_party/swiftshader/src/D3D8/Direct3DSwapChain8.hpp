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

#ifndef D3D8_Direct3DSwapChain8_hpp
#define D3D8_Direct3DSwapChain8_hpp

#include "Unknown.hpp"

#include "Direct3DSurface8.hpp"

#include "FrameBufferWin.hpp"

#include <d3d8.h>

namespace D3D8
{
	class Direct3DSwapChain8 : public IDirect3DSwapChain8, public Unknown
	{
	public:
		Direct3DSwapChain8(Direct3DDevice8 *device, D3DPRESENT_PARAMETERS *presentParameters);

		~Direct3DSwapChain8() override;

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object) override;
		unsigned long __stdcall AddRef() override;
		unsigned long __stdcall Release() override;

		// IDirect3DSwapChain8 methods
	    long __stdcall Present(const RECT *sourceRect, const RECT *destRect, HWND destWindowOverride, const RGNDATA *dirtyRegion) override;
	    long __stdcall GetBackBuffer(unsigned int index, D3DBACKBUFFER_TYPE type, IDirect3DSurface8 **backBuffer) override;

		// Internal methods
		void reset(D3DPRESENT_PARAMETERS *presentParameters);

		void screenshot(void *destBuffer);
		void setGammaRamp(sw::GammaRamp *gammaRamp, bool calibrate);
		void getGammaRamp(sw::GammaRamp *gammaRamp);

		void *lockBackBuffer(int index);
		void unlockBackBuffer(int index);

	private:
		void release();

		// Creation parameters
		Direct3DDevice8 *const device;
		D3DPRESENT_PARAMETERS presentParameters;

		bool lockable;

		sw::FrameBufferWin *frameBuffer;

	public:   // FIXME
		Direct3DSurface8 *backBuffer[3];   // NOTE: Up to three
	};
}

#endif // D3D8_Direct3DSwapChain8_hpp
