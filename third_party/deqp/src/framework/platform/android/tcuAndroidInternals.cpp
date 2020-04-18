/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Access to Android internals that are not a part of the NDK.
 *//*--------------------------------------------------------------------*/

#include "tcuAndroidInternals.hpp"
#include "deMemory.h"
#include "deStringUtil.hpp"

namespace tcu
{
namespace Android
{
namespace internal
{

using std::string;
using de::DynamicLibrary;

template<typename Func>
void setFuncPtr (Func*& funcPtr, DynamicLibrary& lib, const string& symname)
{
	funcPtr = reinterpret_cast<Func*>(lib.getFunction(symname.c_str()));
	if (!funcPtr)
		TCU_THROW(NotSupportedError, ("Unable to look up symbol from shared object: " + symname).c_str());
}

LibUI::LibUI (void)
	: m_library	("libui.so")
{
	GraphicBufferFunctions& gb = m_functions.graphicBuffer;

	setFuncPtr(gb.constructor,		m_library,	"_ZN7android13GraphicBufferC1Ejjij");
	setFuncPtr(gb.destructor,		m_library,	"_ZN7android13GraphicBufferD1Ev");
	setFuncPtr(gb.getNativeBuffer,	m_library,	"_ZNK7android13GraphicBuffer15getNativeBufferEv");
	setFuncPtr(gb.lock,				m_library,	"_ZN7android13GraphicBuffer4lockEjPPv");
	setFuncPtr(gb.unlock,			m_library,	"_ZN7android13GraphicBuffer6unlockEv");
	setFuncPtr(gb.initCheck,		m_library,	"_ZNK7android13GraphicBuffer9initCheckEv");
}

#define GRAPHICBUFFER_SIZE 1024 // Hopefully enough

typedef void (*GenericFptr)();

//! call constructor with 4 arguments
template <typename RT, typename T1, typename T2, typename T3, typename T4>
RT* callConstructor4 (GenericFptr fptr, void* memory, size_t memorySize, T1 param1, T2 param2, T3 param3, T4 param4)
{
	DE_UNREF(memorySize);

#if (DE_CPU == DE_CPU_ARM)
	// C1 constructors return pointer
	typedef RT* (*ABIFptr)(void*, T1, T2, T3, T4);
	(void)((ABIFptr)fptr)(memory, param1, param2, param3, param4);
	return reinterpret_cast<RT*>(memory);
#elif (DE_CPU == DE_CPU_ARM_64)
	// C1 constructors return void
	typedef void (*ABIFptr)(void*, T1, T2, T3, T4);
	((ABIFptr)fptr)(memory, param1, param2, param3, param4);
	return reinterpret_cast<RT*>(memory);
#elif (DE_CPU == DE_CPU_X86)
	// ctor returns void
	typedef void (*ABIFptr)(void*, T1, T2, T3, T4);
	((ABIFptr)fptr)(memory, param1, param2, param3, param4);
	return reinterpret_cast<RT*>(memory);
#elif (DE_CPU == DE_CPU_X86_64)
	// ctor returns void
	typedef void (*ABIFptr)(void*, T1, T2, T3, T4);
	((ABIFptr)fptr)(memory, param1, param2, param3, param4);
	return reinterpret_cast<RT*>(memory);
#else
	DE_UNREF(fptr);
	DE_UNREF(memory);
	DE_UNREF(param1);
	DE_UNREF(param2);
	DE_UNREF(param3);
	DE_UNREF(param4);
	TCU_THROW(NotSupportedError, "ABI not supported");
	return DE_NULL;
#endif
}

template <typename T>
void callDestructor (GenericFptr fptr, T* obj)
{
#if (DE_CPU == DE_CPU_ARM)
	// D1 destructor returns ptr
	typedef void* (*ABIFptr)(T* obj);
	(void)((ABIFptr)fptr)(obj);
#elif (DE_CPU == DE_CPU_ARM_64)
	// D1 destructor returns void
	typedef void (*ABIFptr)(T* obj);
	((ABIFptr)fptr)(obj);
#elif (DE_CPU == DE_CPU_X86)
	// dtor returns void
	typedef void (*ABIFptr)(T* obj);
	((ABIFptr)fptr)(obj);
#elif (DE_CPU == DE_CPU_X86_64)
	// dtor returns void
	typedef void (*ABIFptr)(T* obj);
	((ABIFptr)fptr)(obj);
#else
	DE_UNREF(fptr);
	DE_UNREF(obj);
	TCU_THROW(NotSupportedError, "ABI not supported");
#endif
}

template<typename T1, typename T2>
T1* pointerToOffset (T2* ptr, size_t bytes)
{
	return reinterpret_cast<T1*>((deUint8*)ptr + bytes);
}

static android::android_native_base_t* getAndroidNativeBase (android::GraphicBuffer* gb)
{
	// \note: assuming Itanium ABI
	return pointerToOffset<android::android_native_base_t>(gb, 2 * DE_PTR_SIZE);
}

//! android_native_base_t::magic for ANativeWindowBuffer
static deInt32 getExpectedNativeBufferVersion (void)
{
#if (DE_PTR_SIZE == 4)
	return 96;
#elif (DE_PTR_SIZE == 8)
	return 168;
#else
#	error Invalid DE_PTR_SIZE
#endif
}

//! access android_native_base_t::magic
static deUint32 getNativeBaseMagic (android::android_native_base_t* base)
{
	return *pointerToOffset<deUint32>(base, 0);
}

//! access android_native_base_t::version
static deUint32 getNativeBaseVersion (android::android_native_base_t* base)
{
	return *pointerToOffset<deInt32>(base, 4);
}

//! access android_native_base_t::incRef
static NativeBaseFunctions::incRefFunc getNativeBaseIncRefFunc (android::android_native_base_t* base)
{
	return *pointerToOffset<NativeBaseFunctions::incRefFunc>(base, 8 + DE_PTR_SIZE*4);
}

//! access android_native_base_t::decRef
static NativeBaseFunctions::decRefFunc getNativeBaseDecRefFunc (android::android_native_base_t* base)
{
	return *pointerToOffset<NativeBaseFunctions::decRefFunc>(base, 8 + DE_PTR_SIZE*5);
}

static android::GraphicBuffer* createGraphicBuffer (const GraphicBufferFunctions& functions, NativeBaseFunctions& baseFunctions, deUint32 w, deUint32 h, PixelFormat format, deUint32 usage)
{
	// \note: Hopefully uses the same allocator as libui
	void* const memory = deMalloc(GRAPHICBUFFER_SIZE);
	if (memory == DE_NULL)
		TCU_THROW(ResourceError, "Could not alloc for GraphicBuffer");
	else
	{
		try
		{
			android::GraphicBuffer* const			gb			= callConstructor4<android::GraphicBuffer, deUint32, deUint32, PixelFormat, deUint32>(functions.constructor,
																																					  memory,
																																					  GRAPHICBUFFER_SIZE,
																																					  w,
																																					  h,
																																					  format,
																																					  usage);
			android::android_native_base_t* const	base		= getAndroidNativeBase(gb);
			status_t								ctorStatus	= functions.initCheck(gb);

			if (ctorStatus)
			{
				// ctor failed
				callDestructor<android::GraphicBuffer>(functions.destructor, gb);
				TCU_THROW(NotSupportedError, ("GraphicBuffer ctor failed, initCheck returned " + de::toString(ctorStatus)).c_str());
			}

			// check object layout
			{
				const deUint32 magic		= getNativeBaseMagic(base);
				const deUint32 bufferMagic	= 0x5f626672u; // "_bfr"

				if (magic != bufferMagic)
					TCU_THROW(NotSupportedError, "GraphicBuffer layout unexpected");
			}

			// check object version
			{
				const deInt32 version			= getNativeBaseVersion(base);
				const deInt32 expectedVersion	= getExpectedNativeBufferVersion();

				if (version != expectedVersion)
					TCU_THROW(NotSupportedError, "GraphicBuffer version unexpected");
			}

			// locate refcounting functions

			if (!baseFunctions.incRef || !baseFunctions.decRef)
			{
				baseFunctions.incRef = getNativeBaseIncRefFunc(base);
				baseFunctions.decRef = getNativeBaseDecRefFunc(base);
			}

			// take the initial reference and return
			baseFunctions.incRef(base);
			return gb;
		}
		catch (...)
		{
			deFree(memory);
			throw;
		}
	}
}

GraphicBuffer::GraphicBuffer (const LibUI& lib, deUint32 width, deUint32 height, PixelFormat format, deUint32 usage)
	: m_functions	(lib.getFunctions().graphicBuffer)
	, m_impl		(DE_NULL)
{
	m_baseFunctions.incRef = DE_NULL;
	m_baseFunctions.decRef = DE_NULL;

	// \note createGraphicBuffer updates m_baseFunctions
	m_impl = createGraphicBuffer(m_functions, m_baseFunctions, width, height, format, usage);
}

GraphicBuffer::~GraphicBuffer (void)
{
	if (m_impl && m_baseFunctions.decRef)
	{
		m_baseFunctions.decRef(getAndroidNativeBase(m_impl));
		m_impl = DE_NULL;
	}
}

status_t GraphicBuffer::lock (deUint32 usage, void** vaddr)
{
	return m_functions.lock(m_impl, usage, vaddr);
}

status_t GraphicBuffer::unlock (void)
{
	return m_functions.unlock(m_impl);
}

ANativeWindowBuffer* GraphicBuffer::getNativeBuffer (void) const
{
	return m_functions.getNativeBuffer(m_impl);
}

} // internal
} // Android
} // tcu
