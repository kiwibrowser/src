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

#include "FrameBufferDD.hpp"

#include "Common/Debug.hpp"

namespace sw
{
	extern bool forceWindowed;

	GUID secondaryDisplay = {0};

	int __stdcall enumDisplayCallback(GUID* guid, char *driverDescription, char *driverName, void *context, HMONITOR monitor)
	{
		if(strcmp(driverName, "\\\\.\\DISPLAY2") == 0)
		{
			secondaryDisplay = *guid;
		}

		return 1;
	}

	FrameBufferDD::FrameBufferDD(HWND windowHandle, int width, int height, bool fullscreen, bool topLeftOrigin) : FrameBufferWin(windowHandle, width, height, fullscreen, topLeftOrigin)
	{
		directDraw = 0;
		frontBuffer = 0;
		backBuffer = 0;

		framebuffer = nullptr;

		ddraw = LoadLibrary("ddraw.dll");
		DirectDrawCreate = (DIRECTDRAWCREATE)GetProcAddress(ddraw, "DirectDrawCreate");
		DirectDrawEnumerateExA = (DIRECTDRAWENUMERATEEXA)GetProcAddress(ddraw, "DirectDrawEnumerateExA");

		if(!windowed)
		{
			initFullscreen();
		}
		else
		{
			initWindowed();
		}
	}

	FrameBufferDD::~FrameBufferDD()
	{
		releaseAll();

		FreeLibrary(ddraw);
	}

	void FrameBufferDD::createSurfaces()
	{
		if(backBuffer)
		{
			backBuffer->Release();
			backBuffer = 0;
		}

		if(frontBuffer)
		{
			frontBuffer->Release();
			frontBuffer = 0;
		}

		if(!windowed)
		{
			DDSURFACEDESC surfaceDescription = {0};
			surfaceDescription.dwSize = sizeof(surfaceDescription);
			surfaceDescription.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
			surfaceDescription.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
			surfaceDescription.dwBackBufferCount = 1;
			directDraw->CreateSurface(&surfaceDescription, &frontBuffer, 0);

			if(frontBuffer)
			{
				DDSCAPS surfaceCapabilties = {0};
				surfaceCapabilties.dwCaps = DDSCAPS_BACKBUFFER;
				frontBuffer->GetAttachedSurface(&surfaceCapabilties, &backBuffer);
				backBuffer->AddRef();
			}
		}
		else
		{
			IDirectDrawClipper *clipper;

			DDSURFACEDESC ddsd = {0};
			ddsd.dwSize = sizeof(ddsd);
			ddsd.dwFlags = DDSD_CAPS;
			ddsd.ddsCaps.dwCaps	= DDSCAPS_PRIMARYSURFACE;

			long result = directDraw->CreateSurface(&ddsd, &frontBuffer, 0);
			directDraw->GetDisplayMode(&ddsd);

			switch(ddsd.ddpfPixelFormat.dwRGBBitCount)
			{
			case 32: format = FORMAT_X8R8G8B8; break;
			case 24: format = FORMAT_R8G8B8;   break;
			case 16: format = FORMAT_R5G6B5;   break;
			default: format = FORMAT_NULL;     break;
			}

			if((result != DD_OK && result != DDERR_PRIMARYSURFACEALREADYEXISTS) || (format == FORMAT_NULL))
			{
				assert(!"Failed to initialize graphics: Incompatible display mode.");
			}
			else
			{
				ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
				ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
				ddsd.dwWidth = width;
				ddsd.dwHeight = height;

				directDraw->CreateSurface(&ddsd, &backBuffer, 0);

				directDraw->CreateClipper(0, &clipper, 0);
				clipper->SetHWnd(0, windowHandle);
				frontBuffer->SetClipper(clipper);
				clipper->Release();
			}
		}
	}

	bool FrameBufferDD::readySurfaces()
	{
		if(!frontBuffer || !backBuffer)
		{
			createSurfaces();
		}

		if(frontBuffer && backBuffer)
		{
			if(frontBuffer->IsLost() || backBuffer->IsLost())
			{
				restoreSurfaces();
			}

			if(frontBuffer && backBuffer)
			{
				if(!frontBuffer->IsLost() && !backBuffer->IsLost())
				{
					return true;
				}
			}
		}

		return false;
	}

	void FrameBufferDD::updateClipper(HWND windowOverride)
	{
		if(windowed)
		{
			if(frontBuffer)
			{
				HWND window = windowOverride ? windowOverride : windowHandle;

				IDirectDrawClipper *clipper;
				frontBuffer->GetClipper(&clipper);
				clipper->SetHWnd(0, window);
				clipper->Release();
			}
		}
	}

	void FrameBufferDD::restoreSurfaces()
	{
		long result1 = frontBuffer->Restore();
		long result2 = backBuffer->Restore();

		if(result1 != DD_OK || result2 != DD_OK)   // Surfaces could not be restored; recreate them
		{
			createSurfaces();
		}
	}

	void FrameBufferDD::initFullscreen()
	{
		releaseAll();

		if(true)   // Render to primary display
		{
			DirectDrawCreate(0, &directDraw, 0);
		}
		else   // Render to secondary display
		{
			DirectDrawEnumerateEx(&enumDisplayCallback, 0, DDENUM_ATTACHEDSECONDARYDEVICES);
			DirectDrawCreate(&secondaryDisplay, &directDraw, 0);
		}

		directDraw->SetCooperativeLevel(windowHandle, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);

		long result;

		do
		{
			format = FORMAT_X8R8G8B8;
			result = directDraw->SetDisplayMode(width, height, 32);

			if(result == DDERR_INVALIDMODE)
			{
				format = FORMAT_R8G8B8;
				result = directDraw->SetDisplayMode(width, height, 24);

				if(result == DDERR_INVALIDMODE)
				{
					format = FORMAT_R5G6B5;
					result = directDraw->SetDisplayMode(width, height, 16);

					if(result == DDERR_INVALIDMODE)
					{
						assert(!"Failed to initialize graphics: Display mode not supported.");
					}
				}
			}

			if(result != DD_OK)
			{
				Sleep(1);
			}
		}
		while(result != DD_OK);

		createSurfaces();

		updateBounds(windowHandle);
	}

	void FrameBufferDD::initWindowed()
	{
		releaseAll();

		DirectDrawCreate(0, &directDraw, 0);
		directDraw->SetCooperativeLevel(windowHandle, DDSCL_NORMAL);

		createSurfaces();

		updateBounds(windowHandle);
	}

	void FrameBufferDD::flip(sw::Surface *source)
	{
		copy(source);

		if(!readySurfaces())
		{
			return;
		}

		while(true)
		{
			long result;

			if(windowed)
			{
				result = frontBuffer->Blt(&bounds, backBuffer, 0, DDBLT_WAIT, 0);
			}
			else
			{
				result = frontBuffer->Flip(0, DDFLIP_NOVSYNC);
			}

			if(result != DDERR_WASSTILLDRAWING)
			{
				break;
			}

			Sleep(0);
		}
	}

	void FrameBufferDD::blit(sw::Surface *source, const Rect *sourceRect, const Rect *destRect)
	{
		copy(source);

		if(!readySurfaces())
		{
			return;
		}

		RECT dRect;

		if(destRect)
		{
			dRect.bottom = bounds.top + destRect->y1;
			dRect.left = bounds.left + destRect->x0;
			dRect.right = bounds.left + destRect->x1;
			dRect.top = bounds.top + destRect->y0;
		}
		else
		{
			dRect.bottom = bounds.top + height;
			dRect.left = bounds.left + 0;
			dRect.right = bounds.left + width;
			dRect.top = bounds.top + 0;
		}

		while(true)
		{
			long result = frontBuffer->Blt(&dRect, backBuffer, (LPRECT)sourceRect, DDBLT_WAIT, 0);

			if(result != DDERR_WASSTILLDRAWING)
			{
				break;
			}

			Sleep(0);
		}
	}

	void FrameBufferDD::flip(HWND windowOverride, sw::Surface *source)
	{
		updateClipper(windowOverride);
		updateBounds(windowOverride);

		flip(source);
	}

	void FrameBufferDD::blit(HWND windowOverride, sw::Surface *source, const Rect *sourceRect, const Rect *destRect)
	{
		updateClipper(windowOverride);
		updateBounds(windowOverride);

		blit(source, sourceRect, destRect);
	}

	void FrameBufferDD::screenshot(void *destBuffer)
	{
		if(!readySurfaces())
		{
			return;
		}

		DDSURFACEDESC DDSD;
		DDSD.dwSize = sizeof(DDSD);

		long result = frontBuffer->Lock(0, &DDSD, DDLOCK_WAIT, 0);

		if(result == DD_OK)
		{
			int width = DDSD.dwWidth;
			int height = DDSD.dwHeight;
			int stride = DDSD.lPitch;

			void *sourceBuffer = DDSD.lpSurface;

			for(int y = 0; y < height; y++)
			{
				memcpy(destBuffer, sourceBuffer, width * 4);   // FIXME: Assumes 32-bit buffer

				(char*&)sourceBuffer += stride;
				(char*&)destBuffer += 4 * width;
			}

			frontBuffer->Unlock(0);
		}
	}

	void FrameBufferDD::setGammaRamp(GammaRamp *gammaRamp, bool calibrate)
	{
		IDirectDrawGammaControl *gammaControl = 0;

		if(frontBuffer)
		{
			frontBuffer->QueryInterface(IID_IDirectDrawGammaControl, (void**)&gammaControl);

			if(gammaControl)
			{
				gammaControl->SetGammaRamp(calibrate ? DDSGR_CALIBRATE : 0, (DDGAMMARAMP*)gammaRamp);

				gammaControl->Release();
			}
		}
	}

	void FrameBufferDD::getGammaRamp(GammaRamp *gammaRamp)
	{
		IDirectDrawGammaControl *gammaControl = 0;

		if(frontBuffer)
		{
			frontBuffer->QueryInterface(IID_IDirectDrawGammaControl, (void**)&gammaControl);

			if(gammaControl)
			{
				gammaControl->GetGammaRamp(0, (DDGAMMARAMP*)gammaRamp);

				gammaControl->Release();
			}
		}
	}

	void *FrameBufferDD::lock()
	{
		if(framebuffer)
		{
			return framebuffer;
		}

		if(!readySurfaces())
		{
			return nullptr;
		}

		DDSURFACEDESC DDSD;
		DDSD.dwSize = sizeof(DDSD);

		long result = backBuffer->Lock(0, &DDSD, DDLOCK_WAIT, 0);

		if(result == DD_OK)
		{
			width = DDSD.dwWidth;
			height = DDSD.dwHeight;
			stride = DDSD.lPitch;

			framebuffer = DDSD.lpSurface;

			return framebuffer;
		}

		return nullptr;
	}

	void FrameBufferDD::unlock()
	{
		if(!framebuffer || !backBuffer) return;

		backBuffer->Unlock(0);

		framebuffer = nullptr;
	}

	void FrameBufferDD::drawText(int x, int y, const char *string, ...)
	{
		char buffer[256];
		va_list arglist;

		va_start(arglist, string);
		vsprintf(buffer, string, arglist);
		va_end(arglist);

		HDC hdc;

		backBuffer->GetDC(&hdc);

		SetBkColor(hdc, RGB(0, 0, 255));
		SetTextColor(hdc, RGB(255, 255, 255));

		TextOut(hdc, x, y, buffer, lstrlen(buffer));

		backBuffer->ReleaseDC(hdc);
	}

	bool FrameBufferDD::getScanline(bool &inVerticalBlank, unsigned int &scanline)
	{
		HRESULT result = directDraw->GetScanLine((unsigned long*)&scanline);

		if(result == DD_OK)
		{
			inVerticalBlank = false;
		}
		else if(result == DDERR_VERTICALBLANKINPROGRESS)
		{
			inVerticalBlank = true;
		}
		else if(result == DDERR_UNSUPPORTED)
		{
			return false;
		}
		else ASSERT(false);

		return true;
	}

	void FrameBufferDD::releaseAll()
	{
		unlock();

		if(backBuffer)
		{
			backBuffer->Release();
			backBuffer = 0;
		}

		if(frontBuffer)
		{
			frontBuffer->Release();
			frontBuffer = 0;
		}

		if(directDraw)
		{
			directDraw->SetCooperativeLevel(0, DDSCL_NORMAL);
			directDraw->Release();
			directDraw = 0;
		}
	}
}
