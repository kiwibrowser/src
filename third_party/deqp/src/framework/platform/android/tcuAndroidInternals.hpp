#ifndef _TCUANDROIDINTERNALS_HPP
#define _TCUANDROIDINTERNALS_HPP
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

#include "tcuDefs.hpp"

#include "deDynamicLibrary.hpp"

#include <vector>
#include <errno.h>

struct ANativeWindowBuffer;

namespace android
{
class GraphicBuffer;
class android_native_base_t;
}

namespace tcu
{

namespace Android
{

// These classes and enums reflect internal android definitions
namespace internal
{

// utils/Errors.h
enum
{
	OK					= 0,
	UNKNOWN_ERROR		= (-2147483647-1),
	NO_MEMORY			= -ENOMEM,
	INVALID_OPERATION	= -ENOSYS,
	BAD_VALUE			= -EINVAL,
	BAD_TYPE			= (UNKNOWN_ERROR + 1),
	NAME_NOT_FOUND		= -ENOENT,
	PERMISSION_DENIED	= -EPERM,
	NO_INIT				= -ENODEV,
	ALREADY_EXISTS		= -EEXIST,
	DEAD_OBJECT			= -EPIPE,
	FAILED_TRANSACTION	= (UNKNOWN_ERROR + 2),
	BAD_INDEX			= -E2BIG,
	NOT_ENOUGH_DATA		= (UNKNOWN_ERROR + 3),
	WOULD_BLOCK			= (UNKNOWN_ERROR + 4),
	TIMED_OUT			= (UNKNOWN_ERROR + 5),
	UNKNOWN_TRANSACTION = (UNKNOWN_ERROR + 6),
	FDS_NOT_ALLOWED		= (UNKNOWN_ERROR + 7),
};

typedef deInt32 status_t;

// ui/PixelFormat.h, system/graphics.h
enum
{
	PIXEL_FORMAT_UNKNOWN				= 0,
	PIXEL_FORMAT_NONE					= 0,
	PIXEL_FORMAT_CUSTOM					= -4,
	PIXEL_FORMAT_TRANSLUCENT			= -3,
	PIXEL_FORMAT_TRANSPARENT			= -2,
	PIXEL_FORMAT_OPAQUE					= -1,
	PIXEL_FORMAT_RGBA_8888				= 1,
	PIXEL_FORMAT_RGBX_8888				= 2,
	PIXEL_FORMAT_RGB_888				= 3,
	PIXEL_FORMAT_RGB_565				= 4,
	PIXEL_FORMAT_BGRA_8888				= 5,
	PIXEL_FORMAT_RGBA_5551				= 6,
	PIXEL_FORMAT_RGBA_4444				= 7,
};

typedef deInt32 PixelFormat;

// ui/GraphicBuffer.h
struct GraphicBufferFunctions
{
	typedef void					(*genericFunc)			();
	typedef status_t				(*initCheckFunc)		(android::GraphicBuffer* buffer);
	typedef status_t				(*lockFunc)				(android::GraphicBuffer* buffer, deUint32 usage, void** vaddr);
	typedef status_t				(*unlockFunc)			(android::GraphicBuffer* buffer);
	typedef ANativeWindowBuffer*	(*getNativeBufferFunc)	(const android::GraphicBuffer* buffer);

	genericFunc						constructor;
	genericFunc						destructor;
	lockFunc						lock;
	unlockFunc						unlock;
	getNativeBufferFunc				getNativeBuffer;
	initCheckFunc					initCheck;
};

// system/window.h
struct NativeBaseFunctions
{
	typedef void	(*incRefFunc)			(android::android_native_base_t* base);
	typedef void	(*decRefFunc)			(android::android_native_base_t* base);

	incRefFunc		incRef;
	decRefFunc		decRef;
};

struct LibUIFunctions
{
	GraphicBufferFunctions graphicBuffer;
};

class LibUI
{
public:
	struct Functions
	{
		GraphicBufferFunctions graphicBuffer;
	};

							LibUI			(void);
	const Functions&		getFunctions	(void) const { return m_functions; }

private:
	Functions				m_functions;
	de::DynamicLibrary		m_library;
};

class GraphicBuffer
{
public:
	// ui/GraphicBuffer.h, hardware/gralloc.h
	enum {
		USAGE_SW_READ_NEVER		= 0x00000000,
		USAGE_SW_READ_RARELY	= 0x00000002,
		USAGE_SW_READ_OFTEN		= 0x00000003,
		USAGE_SW_READ_MASK		= 0x0000000f,

		USAGE_SW_WRITE_NEVER	= 0x00000000,
		USAGE_SW_WRITE_RARELY	= 0x00000020,
		USAGE_SW_WRITE_OFTEN	= 0x00000030,
		USAGE_SW_WRITE_MASK		= 0x000000f0,

		USAGE_SOFTWARE_MASK		= USAGE_SW_READ_MASK | USAGE_SW_WRITE_MASK,

		USAGE_PROTECTED			= 0x00004000,

		USAGE_HW_TEXTURE		= 0x00000100,
		USAGE_HW_RENDER			= 0x00000200,
		USAGE_HW_2D				= 0x00000400,
		USAGE_HW_COMPOSER		= 0x00000800,
		USAGE_HW_VIDEO_ENCODER	= 0x00010000,
		USAGE_HW_MASK			= 0x00071F00,
	};

									GraphicBuffer			(const LibUI& lib, deUint32 width, deUint32 height, PixelFormat format, deUint32 usage);
									~GraphicBuffer			();

	status_t						lock					(deUint32 usage, void** vaddr);
	status_t						unlock					(void);
	ANativeWindowBuffer*			getNativeBuffer			(void) const;

private:
	const GraphicBufferFunctions&	m_functions;
	NativeBaseFunctions				m_baseFunctions;
	android::GraphicBuffer*			m_impl;
};

} // internal
} // Android
} // tcu

#endif // _TCUANDROIDINTERNALS_HPP
