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

#include "Direct3D8.hpp"

#include "resource.h"

#include <stdio.h>
#include <assert.h>

extern "C"
{
	HINSTANCE dllInstance;

	int	__stdcall DllMain(HINSTANCE	instance, unsigned long	reason,	void *reserved)
	{
		dllInstance	= instance;

		switch(reason)
		{
		case DLL_PROCESS_DETACH:
			break;
		case DLL_PROCESS_ATTACH:
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

	IDirect3D8 *__stdcall Direct3DCreate8(unsigned int version)
	{
		// D3D_SDK_VERSION check
		if(version != 120 &&   // 8.0
		   version != 220)     // 8.1
		{
			return 0;
		}
		
		#ifndef NDEBUG
			FILE *file = fopen("debug.txt", "w");   // Clear debug log
			fclose(file);
		#endif

		IDirect3D8 *device = new D3D8::Direct3D8(version, dllInstance);

		if(device)
		{
			device->AddRef();
		}

		return device;
	}

	int __stdcall CheckFullscreen()   // FIXME: __cdecl or __stdcall?
	{
		#ifndef NDEBUG
		//	ASSERT(false);   // FIXME
		#endif

		return FALSE;
	}

    void __cdecl DebugSetMute(long mute)   // FIXME: Return type
	{
	//	ASSERT(false);   // FIXME
	}

    int __stdcall ValidatePixelShader(long *shader, int x, int y, int z)   // FIXME: __cdecl or __stdcall?   // FIXME: Argument meanings
	{
	//	ASSERT(false);   // FIXME

		return TRUE;
	}

    int __stdcall ValidateVertexShader(long *shader, int x, int y, int z)   // FIXME: __cdecl or __stdcall?   // FIXME: Argument meanings
	{
	//	ASSERT(false);   // FIXME

		return TRUE;
	}
}
