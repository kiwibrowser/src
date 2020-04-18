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

// debug.h: Debugging utilities.

#ifndef COMMON_DEBUG_H_
#define COMMON_DEBUG_H_

#ifdef __ANDROID__
#include "../../Common/DebugAndroid.hpp"
#else
#include <stdio.h>
#include <assert.h>

#if !defined(TRACE_OUTPUT_FILE)
#define TRACE_OUTPUT_FILE "debug.txt"
#endif

namespace es
{
	// Outputs text to the debugging log
	void trace(const char *format, ...);
}

// A macro to output a trace of a function call and its arguments to the debugging log
#if defined(ANGLE_DISABLE_TRACE)
#define TRACE(message, ...) (void(0))
#else
#define TRACE(message, ...) es::trace("trace: %s(%d): " message "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

// A macro to output a function call and its arguments to the debugging log, to denote an item in need of fixing.
#if defined(ANGLE_DISABLE_TRACE)
#define FIXME(message, ...) (void(0))
#else
#define FIXME(message, ...) do {es::trace("fixme: %s(%d): " message "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); assert(false);} while(false)
#endif

// A macro to output a function call and its arguments to the debugging log, in case of error.
#if defined(ANGLE_DISABLE_TRACE)
#define ERR(message, ...) (void(0))
#else
#define ERR(message, ...) do {es::trace("err: %s(%d): " message "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); assert(false);} while(false)
#endif

// A macro asserting a condition and outputting failures to the debug log
#undef ASSERT
#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)
#define ASSERT(expression) do { \
	if(!(expression)) { \
		ERR("\t! Assert failed in %s(%d): "#expression"\n", __FUNCTION__, __LINE__); \
		assert(expression); \
	} } while(0)
#else
#define ASSERT(expression) (void(0))
#endif

// A macro to indicate unimplemented functionality
#undef UNIMPLEMENTED
#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)
#define UNIMPLEMENTED() do { \
	FIXME("\t! Unimplemented: %s(%d)\n", __FUNCTION__, __LINE__); \
	assert(false); \
	} while(0)
#else
	#define UNIMPLEMENTED() FIXME("\t! Unimplemented: %s(%d)\n", __FUNCTION__, __LINE__)
#endif

// A macro for code which is not expected to be reached under valid assumptions
#undef UNREACHABLE
#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)
#define UNREACHABLE(value) do { \
	ERR("\t! Unreachable case reached: %s(%d). %s: %d\n", __FUNCTION__, __LINE__, #value, value); \
	assert(false); \
	} while(0)
#else
	#define UNREACHABLE(value) ERR("\t! Unreachable reached: %s(%d). %s: %d\n", __FUNCTION__, __LINE__, #value, value)
#endif

#endif   // !__ANDROID__

// A macro asserting a condition and outputting failures to the debug log, or return when in release mode.
#undef ASSERT_OR_RETURN
#define ASSERT_OR_RETURN(expression) do { \
	if(!(expression)) { \
		ASSERT(expression); \
		return; \
	} } while(0)

#endif   // COMMON_DEBUG_H_
