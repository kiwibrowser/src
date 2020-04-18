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

#include "FrameBufferGDI.hpp"

#include "Common/Debug.hpp"

namespace sw
{
	extern bool forceWindowed;

	FrameBufferGDI::FrameBufferGDI(HWND windowHandle, int width, int height, bool fullscreen, bool topLeftOrigin) : FrameBufferWin(windowHandle, width, height, fullscreen, topLeftOrigin)
	{
		if(!windowed)
		{
			SetWindowPos(windowHandle, HWND_TOPMOST, 0, 0, width, height, SWP_SHOWWINDOW);

			DEVMODE deviceMode;
			deviceMode.dmSize = sizeof(DEVMODE);
			deviceMode.dmPelsWidth= width;
			deviceMode.dmPelsHeight = height;
			deviceMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

			ChangeDisplaySettings(&deviceMode, CDS_FULLSCREEN);
		}

		init(this->windowHandle);

		format = FORMAT_X8R8G8B8;
	}

	FrameBufferGDI::~FrameBufferGDI()
	{
		release();

		if(!windowed)
		{
			ChangeDisplaySettings(0, 0);

			RECT clientRect;
			RECT windowRect;
			GetClientRect(windowHandle, &clientRect);
			GetWindowRect(windowHandle, &windowRect);
			int windowWidth = width + (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left);
			int windowHeight = height + (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top);
			int desktopWidth = GetSystemMetrics(SM_CXSCREEN);
			int desktopHeight = GetSystemMetrics(SM_CYSCREEN);
			SetWindowPos(windowHandle, HWND_TOP, desktopWidth / 2 - windowWidth / 2, desktopHeight / 2 - windowHeight / 2, windowWidth, windowHeight, SWP_SHOWWINDOW);
		}
	}

	void *FrameBufferGDI::lock()
	{
		stride = width * 4;

		return framebuffer;
	}

	void FrameBufferGDI::unlock()
	{
	}

	void FrameBufferGDI::flip(sw::Surface *source)
	{
		blit(source, nullptr, nullptr);
	}

	void FrameBufferGDI::blit(sw::Surface *source, const Rect *sourceRect, const Rect *destRect)
	{
		copy(source);

		int sourceLeft = sourceRect ? sourceRect->x0 : 0;
		int sourceTop = sourceRect ? sourceRect->y0 : 0;
		int sourceWidth = sourceRect ? sourceRect->x1 - sourceRect->x0 : width;
		int sourceHeight = sourceRect ? sourceRect->y1 - sourceRect->y0 : height;
		int destLeft = destRect ? destRect->x0 : 0;
		int destTop = destRect ? destRect->y0 : 0;
		int destWidth = destRect ? destRect->x1 - destRect->x0 : bounds.right - bounds.left;
		int destHeight = destRect ? destRect->y1 - destRect->y0 : bounds.bottom - bounds.top;

		StretchBlt(windowContext, destLeft, destTop, destWidth, destHeight, bitmapContext, sourceLeft, sourceTop, sourceWidth, sourceHeight, SRCCOPY);
	}

	void FrameBufferGDI::flip(HWND windowOverride, sw::Surface *source)
	{
		blit(windowOverride, source, nullptr, nullptr);
	}

	void FrameBufferGDI::blit(HWND windowOverride, sw::Surface *source, const Rect *sourceRect, const Rect *destRect)
	{
		if(windowed && windowOverride != 0 && windowOverride != bitmapWindow)
		{
			release();
			init(windowOverride);
		}

		blit(source, sourceRect, destRect);
	}

	void FrameBufferGDI::setGammaRamp(GammaRamp *gammaRamp, bool calibrate)
	{
		SetDeviceGammaRamp(windowContext, gammaRamp);
	}

	void FrameBufferGDI::getGammaRamp(GammaRamp *gammaRamp)
	{
		GetDeviceGammaRamp(windowContext, gammaRamp);
	}

	void FrameBufferGDI::screenshot(void *destBuffer)
	{
		UNIMPLEMENTED();
	}

	bool FrameBufferGDI::getScanline(bool &inVerticalBlank, unsigned int &scanline)
	{
		UNIMPLEMENTED();

		return false;
	}

	void FrameBufferGDI::init(HWND window)
	{
		bitmapWindow = window;

		windowContext = GetDC(window);
		bitmapContext = CreateCompatibleDC(windowContext);

		BITMAPINFO bitmapInfo;
		memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
		bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFO);
		bitmapInfo.bmiHeader.biBitCount = 32;
		bitmapInfo.bmiHeader.biPlanes = 1;
		bitmapInfo.bmiHeader.biHeight = -height;
		bitmapInfo.bmiHeader.biWidth = width;
		bitmapInfo.bmiHeader.biCompression = BI_RGB;

		bitmap = CreateDIBSection(bitmapContext, &bitmapInfo, DIB_RGB_COLORS, &framebuffer, 0, 0);
		SelectObject(bitmapContext, bitmap);

		updateBounds(window);
	}

	void FrameBufferGDI::release()
	{
		SelectObject(bitmapContext, 0);
		DeleteObject(bitmap);
		ReleaseDC(bitmapWindow, windowContext);
		DeleteDC(bitmapContext);
	}
}
