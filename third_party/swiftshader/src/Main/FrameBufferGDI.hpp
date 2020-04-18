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

#ifndef	sw_FrameBufferGDI_hpp
#define	sw_FrameBufferGDI_hpp

#include "FrameBufferWin.hpp"

namespace sw
{
	class FrameBufferGDI : public FrameBufferWin
	{
	public:
		FrameBufferGDI(HWND windowHandle, int width, int height, bool fullscreen, bool topLeftOrigin);

		~FrameBufferGDI() override;

		void flip(sw::Surface *source) override;
		void blit(sw::Surface *source, const Rect *sourceRect, const Rect *destRect) override;

		void flip(HWND windowOverride, sw::Surface *source) override;
		void blit(HWND windowOverride, sw::Surface *source, const Rect *sourceRect, const Rect *destRect) override;

		void *lock() override;
		void unlock() override;

		void setGammaRamp(GammaRamp *gammaRamp, bool calibrate) override;
		void getGammaRamp(GammaRamp *gammaRamp) override;

		void screenshot(void *destBuffer) override;
		bool getScanline(bool &inVerticalBlank, unsigned int &scanline) override;

	private:
		void init(HWND bitmapWindow);
		void release();

		HDC windowContext;
		HDC bitmapContext;
		HWND bitmapWindow;

		HBITMAP bitmap;
	};
}

#endif	 //	sw_FrameBufferGDI_hpp
