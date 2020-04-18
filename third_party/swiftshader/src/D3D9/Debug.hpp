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

#ifndef Debug_hpp
#define Debug_hpp

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d9.h>
#include <stdio.h>
#include <guiddef.h>
#include <assert.h>

#define APPEND(x, y) x ## y
#define MACRO_APPEND(x, y) APPEND(x, y)
#define UNIQUE_IDENTIFIER(prefix) MACRO_APPEND(prefix, __COUNTER__)

struct Trace
{
	Trace(const char *format, ...)
	{
		if(false)
		{
			FILE *file = fopen("debug.txt", "a");

			if(file)
			{
				for(int i = 0; i < indent; i++) fprintf(file, " ");

				va_list vararg;
				va_start(vararg, format);
				vfprintf(file, format, vararg);
				va_end(vararg);

				fclose(file);
			}
		}

		indent++;
	}

	~Trace()
	{
		indent--;
	}

	static int indent;
};

#ifndef NDEBUG
	#define TRACE(format, ...) Trace UNIQUE_IDENTIFIER(_tracer_)("[0x%0.8X]%s("format")\n", this, __FUNCTION__, __VA_ARGS__)
	#define GTRACE(format, ...) Trace("%s("format")\n", __FUNCTION__, __VA_ARGS__)
#else
	#define TRACE(...) ((void)0)
	#define GTRACE(...) ((void)0)
#endif

#ifndef NDEBUG
	#define ASSERT(expression) {if(!(expression)) Trace("\t! Assert failed in %s(%d): "#expression"\n", __FUNCTION__, __LINE__); assert(expression);}
#else
	#define ASSERT assert
#endif

#ifndef NDEBUG
	#define UNIMPLEMENTED() {Trace("\t! Unimplemented: %s(%d)\n", __FUNCTION__, __LINE__); ASSERT(false);}
#else
	#define UNIMPLEMENTED() ((void)0)
#endif

#ifndef NDEBUG
	#define NOINTERFACE(iid) _NOINTERFACE(__FUNCTION__, iid)

	inline long _NOINTERFACE(const char *function, const IID &iid)
	{
		Trace("\t! No interface {0x%0.8X, 0x%0.4X, 0x%0.4X, 0x%0.2X, 0x%0.2X, 0x%0.2X, 0x%0.2X, 0x%0.2X, 0x%0.2X, 0x%0.2X, 0x%0.2X} for %s\n", iid.Data1, iid.Data2, iid.Data3, iid.Data4[0], iid.Data4[1], iid.Data4[2], iid.Data4[3], iid.Data4[4], iid.Data4[5], iid.Data4[6], iid.Data4[7], function);

		return E_NOINTERFACE;
	}
#else
	#define NOINTERFACE(iid) E_NOINTERFACE
#endif

#ifndef NDEBUG
	inline long INVALIDCALL()
	{
		Trace("\t! D3DERR_INVALIDCALL\n");

		return D3DERR_INVALIDCALL;
	}
#else
	#define INVALIDCALL() D3DERR_INVALIDCALL
#endif

#ifndef NDEBUG
	inline long OUTOFMEMORY()
	{
		Trace("\t! E_OUTOFMEMORY\n");

		return E_OUTOFMEMORY;
	}
#else
	#define OUTOFMEMORY() E_OUTOFMEMORY
#endif

#ifndef NDEBUG
	inline long OUTOFVIDEOMEMORY()
	{
		Trace("\t! D3DERR_OUTOFVIDEOMEMORY\n");

		return D3DERR_OUTOFVIDEOMEMORY;
	}
#else
	#define OUTOFVIDEOMEMORY() D3DERR_OUTOFVIDEOMEMORY
#endif

#ifndef NDEBUG
	inline long NOTAVAILABLE()
	{
		Trace("\t! D3DERR_NOTAVAILABLE\n");

		return D3DERR_NOTAVAILABLE;
	}
#else
	#define NOTAVAILABLE() D3DERR_NOTAVAILABLE
#endif

#ifndef NDEBUG
	inline long NOTFOUND()
	{
		Trace("\t! D3DERR_NOTFOUND\n");

		return D3DERR_NOTFOUND;
	}
#else
	#define NOTFOUND() D3DERR_NOTFOUND
#endif

#ifndef NDEBUG
	inline long MOREDATA()
	{
		Trace("\t! D3DERR_MOREDATA\n");

		return D3DERR_MOREDATA;
	}
#else
	#define MOREDATA() D3DERR_MOREDATA
#endif

#ifndef NDEBUG
	inline long FAIL()
	{
		Trace("\t! E_FAIL\n");

		return E_FAIL;
	}
#else
	#define FAIL() E_FAIL
#endif

#endif   // Debug_hpp
