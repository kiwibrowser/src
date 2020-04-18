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

#include "FrameBufferWin.hpp"

namespace sw
{
	FrameBufferWin::FrameBufferWin(HWND windowHandle, int width, int height, bool fullscreen, bool topLeftOrigin) : FrameBuffer(width, height, fullscreen, topLeftOrigin), windowHandle(windowHandle)
	{
		if(!windowed)
		{
			// Force fullscreen window style (no borders)
			originalWindowStyle = GetWindowLong(windowHandle, GWL_STYLE);
			SetWindowLong(windowHandle, GWL_STYLE, WS_POPUP);
		}
	}

	FrameBufferWin::~FrameBufferWin()
	{
		if(!windowed && GetWindowLong(windowHandle, GWL_STYLE) == WS_POPUP)
		{
			SetWindowLong(windowHandle, GWL_STYLE, originalWindowStyle);
		}
	}

	void FrameBufferWin::updateBounds(HWND windowOverride)
	{
		HWND window = windowOverride ? windowOverride : windowHandle;

		if(windowed)
		{
			GetClientRect(window, &bounds);
			ClientToScreen(window, (POINT*)&bounds);
			ClientToScreen(window, (POINT*)&bounds + 1);
		}
		else
		{
			SetRect(&bounds, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		}
	}
}

#include "FrameBufferDD.hpp"
#include "FrameBufferGDI.hpp"
#include "Common/Configurator.hpp"

sw::FrameBufferWin *createFrameBufferWin(HWND windowHandle, int width, int height, bool fullscreen, bool topLeftOrigin)
{
	sw::Configurator ini("SwiftShader.ini");
	int api = ini.getInteger("Testing", "FrameBufferAPI", 0);

	if(api == 0 && topLeftOrigin)
	{
		return new sw::FrameBufferDD(windowHandle, width, height, fullscreen, topLeftOrigin);
	}
	else
	{
		return new sw::FrameBufferGDI(windowHandle, width, height, fullscreen, topLeftOrigin);
	}

	return 0;
}

sw::FrameBuffer *createFrameBuffer(void *display, HWND window, int width, int height)
{
	return createFrameBufferWin(window, width, height, false, false);
}
