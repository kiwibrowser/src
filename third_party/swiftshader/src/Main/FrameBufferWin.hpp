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

#ifndef	sw_FrameBufferWin_hpp
#define	sw_FrameBufferWin_hpp

#include "FrameBuffer.hpp"

namespace sw
{
	struct GammaRamp
	{
		short red[256];
		short green[256];
		short blue[256];
	};

	class FrameBufferWin : public FrameBuffer
	{
	public:
		FrameBufferWin(HWND windowHandle, int width, int height, bool fullscreen, bool topLeftOrigin);

		~FrameBufferWin() override;

		void flip(sw::Surface *source) override = 0;
		void blit(sw::Surface *source, const Rect *sourceRect, const Rect *destRect) override = 0;

		virtual void flip(HWND windowOverride, sw::Surface *source) = 0;
		virtual void blit(HWND windowOverride, sw::Surface *source, const Rect *sourceRect, const Rect *destRect) = 0;

		virtual void setGammaRamp(GammaRamp *gammaRamp, bool calibrate) = 0;
		virtual void getGammaRamp(GammaRamp *gammaRamp) = 0;

		virtual void screenshot(void *destBuffer) = 0;
		virtual bool getScanline(bool &inVerticalBlank, unsigned int &scanline) = 0;

	protected:
		void updateBounds(HWND windowOverride);

		HWND windowHandle;
		DWORD originalWindowStyle;
		RECT bounds;
	};
}

sw::FrameBufferWin *createFrameBufferWin(HWND windowHandle, int width, int height, bool fullscreen, bool topLeftOrigin);

#endif	 //	sw_FrameBufferWin_hpp
