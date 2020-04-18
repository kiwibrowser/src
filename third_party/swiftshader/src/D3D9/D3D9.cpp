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

#include "Direct3D9.hpp"
#include "Direct3D9Ex.hpp"

#include "Debug.hpp"

#include "resource.h"

#include <stdio.h>
#include <assert.h>

namespace D3D9
{
	class Direct3DShaderValidator9
	{
	public:
		Direct3DShaderValidator9();

		virtual ~Direct3DShaderValidator9();

		virtual int __stdcall ValidateShader(long *shader, long *shader1, long *shader2, long *shader3);
		virtual int __stdcall ValidateShader2(long *shader);
		virtual int __stdcall ValidateShader3(long *shader, long *shader1, long *shader2, long *shader3);
		virtual int __stdcall ValidateShader4(long *shader, long *shader1, long *shader2, long *shader3);
		virtual int __stdcall ValidateShader5(long *shader, long *shader1, long *shader2);
		virtual int __stdcall ValidateShader6(long *shader, long *shader1, long *shader2, long *shader3);
	};

	Direct3DShaderValidator9::Direct3DShaderValidator9()
	{
	}

	Direct3DShaderValidator9::~Direct3DShaderValidator9()
	{
	}

	int __stdcall Direct3DShaderValidator9::ValidateShader(long *shader, long *shader1, long *shader2, long *shader3)   // FIXME
	{
		TRACE("");

		UNIMPLEMENTED();

		return true;   // FIXME
	}

	int __stdcall Direct3DShaderValidator9::ValidateShader2(long *shader)   // FIXME
	{
		TRACE("");

		UNIMPLEMENTED();

		return true;   // FIXME
	}

	int __stdcall Direct3DShaderValidator9::ValidateShader3(long *shader, long *shader1, long *shader2, long *shader3)
	{
		TRACE("");

		UNIMPLEMENTED();

		return true;   // FIXME
	}

	int __stdcall Direct3DShaderValidator9::ValidateShader4(long *shader, long *shader1, long *shader2, long *shader3)
	{
		TRACE("");

		UNIMPLEMENTED();

		return true;   // FIXME
	}

	int __stdcall Direct3DShaderValidator9::ValidateShader5(long *shader, long *shader1, long *shader2)   // FIXME
	{
		TRACE("");

		UNIMPLEMENTED();

		return true;   // FIXME
	}

	int __stdcall Direct3DShaderValidator9::ValidateShader6(long *shader, long *shader1, long *shader2, long *shader3)   // FIXME
	{
		TRACE("");

		UNIMPLEMENTED();

		return true;   // FIXME
	}
}

static INT_PTR CALLBACK DebuggerWaitDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT rect;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		GetWindowRect(GetDesktopWindow(), &rect);
		SetWindowPos(hwnd, HWND_TOP, rect.right / 2, rect.bottom / 2, 0, 0, SWP_NOSIZE);
		SetTimer(hwnd, 1, 100, NULL);
		return TRUE;
	case WM_COMMAND:
		if(LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hwnd, 0);
		}
		break;
	case WM_TIMER:
		if(IsDebuggerPresent())
		{
			EndDialog(hwnd, 0);
		}
	}

	return FALSE;
}

static void WaitForDebugger(HINSTANCE instance)
{
	if(!IsDebuggerPresent())
	{
		HRSRC dialog = FindResource(instance, MAKEINTRESOURCE(IDD_DIALOG1), RT_DIALOG);
		DLGTEMPLATE *dialogTemplate = (DLGTEMPLATE*)LoadResource(instance, dialog);
		DialogBoxIndirect(instance, dialogTemplate, NULL, DebuggerWaitDialogProc);
	}
}

using namespace D3D9;

extern "C"
{
	HINSTANCE dllInstance = 0;

	int	__stdcall DllMain(HINSTANCE instance, unsigned long reason, void *reserved)
	{
		#ifndef NDEBUG
			if(dllInstance == 0)
			{
				FILE *file = fopen("debug.txt", "w");   // Clear debug log
				if(file) fclose(file);
			}
		#endif

		GTRACE("HINSTANCE instance = 0x%0.8p, unsigned long reason = %d, void *reserved = 0x%0.8p", instance, reason, reserved);

		dllInstance	= instance;

		switch(reason)
		{
		case DLL_PROCESS_DETACH:
			break;
		case DLL_PROCESS_ATTACH:
			#ifndef NDEBUG
				WaitForDebugger(instance);
			#endif
			DisableThreadLibraryCalls(instance);
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		default:
			SetLastError(ERROR_INVALID_PARAMETER);
			return FALSE;
		}

		return TRUE;
	}

	IDirect3D9 *__stdcall Direct3DCreate9(unsigned int version)
	{
		GTRACE("");

		// D3D_SDK_VERSION check
		if(version != (31 | 0x80000000) && // 9.0a/b DEBUG_INFO
		   version != 31 &&                // 9.0a/b
		   version != (32 | 0x80000000) && // 9.0c DEBUG_INFO
		   version != 32)                  // 9.0c
		{
			return 0;
		}

		IDirect3D9 *device = new D3D9::Direct3D9(version, dllInstance);

		if(device)
		{
			device->AddRef();
		}

		return device;
	}

	HRESULT __stdcall Direct3DCreate9Ex(unsigned int version, IDirect3D9Ex **device)
	{
		// D3D_SDK_VERSION check
		if(version != (31 | 0x80000000) && // 9.0a/b DEBUG_INFO
		   version != 31 &&                // 9.0a/b
		   version != (32 | 0x80000000) && // 9.0c DEBUG_INFO
		   version != 32)                  // 9.0c
		{
			return NOTAVAILABLE();
		}

		*device = new D3D9::Direct3D9Ex(version, dllInstance);

		if(device)
		{
			(*device)->AddRef();
		}
		else
		{
			return OUTOFMEMORY();
		}

		return D3D_OK;
	}

	int __stdcall CheckFullscreen()
	{
		GTRACE("");

		UNIMPLEMENTED();

		return FALSE;
	}

	int __stdcall D3DPERF_BeginEvent(D3DCOLOR color, const wchar_t *name)
	{
		GTRACE("");

	//	UNIMPLEMENTED();   // PIX unsupported

		return -1;
	}

	int __stdcall D3DPERF_EndEvent()
	{
		GTRACE("");

	//	UNIMPLEMENTED();   // PIX unsupported

		return -1;
	}

	unsigned long __stdcall D3DPERF_GetStatus()
	{
		GTRACE("");

	//	UNIMPLEMENTED();   // PIX unsupported

		return 0;
	}

	int __stdcall D3DPERF_QueryRepeatFrame()
	{
		GTRACE("");

	//	UNIMPLEMENTED();   // PIX unsupported

		return FALSE;
	}

	void __stdcall D3DPERF_SetMarker(D3DCOLOR color, const wchar_t *name)
	{
		GTRACE("");

	//	UNIMPLEMENTED();   // PIX unsupported
	}

	void __stdcall D3DPERF_SetOptions(unsigned long options)
	{
		GTRACE("");

	//	UNIMPLEMENTED();   // PIX unsupported
	}

	void __stdcall D3DPERF_SetRegion(D3DCOLOR color, const wchar_t *name)
	{
		GTRACE("");

	//	UNIMPLEMENTED();   // PIX unsupported
	}

	void __cdecl DebugSetLevel(long level)
	{
		GTRACE("long level = %d", level);

	//	UNIMPLEMENTED();   // Debug output unsupported
	}

	void __cdecl DebugSetMute(long mute)
	{
		GTRACE("long mute = %d", mute);

	//	UNIMPLEMENTED();   // Debug output unsupported
	}

	void *__stdcall Direct3DShaderValidatorCreate9()
	{
		GTRACE("");

	//	UNIMPLEMENTED();

		return 0;

	//	return new D3D9::Direct3DShaderValidator9();
	}

	void __stdcall PSGPError()
	{
		GTRACE("");

		UNIMPLEMENTED();
	}

	void __stdcall PSGPSampleTexture()
	{
		GTRACE("");

		UNIMPLEMENTED();
	}
}
