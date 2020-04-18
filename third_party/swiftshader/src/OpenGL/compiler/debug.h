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

#ifndef COMPILER_DEBUG_H_
#define COMPILER_DEBUG_H_

#ifdef __ANDROID__
#include "../../Common/DebugAndroid.hpp"

#define Trace(...) ((void)0)
#else

#include <assert.h>

#ifdef _DEBUG
#define TRACE_ENABLED  // define to enable debug message tracing
#endif  // _DEBUG

// Outputs text to the debug log
#ifdef TRACE_ENABLED

#ifdef  __cplusplus
extern "C" {
#endif  // __cplusplus
void Trace(const char* format, ...);
#ifdef  __cplusplus
}
#endif  // __cplusplus

#else   // TRACE_ENABLED

#define Trace(...) ((void)0)

#endif  // TRACE_ENABLED

// A macro asserting a condition and outputting failures to the debug log
#undef ASSERT
#define ASSERT(expression) do { \
	if(!(expression)) \
		Trace("Assert failed: %s(%d): "#expression"\n", __FUNCTION__, __LINE__); \
	assert(expression); \
} while(0)

#undef UNIMPLEMENTED
#define UNIMPLEMENTED() do { \
	Trace("Unimplemented invoked: %s(%d)\n", __FUNCTION__, __LINE__); \
	assert(false); \
} while(0)

#undef UNREACHABLE
#define UNREACHABLE(value) do { \
	Trace("Unreachable reached: %s(%d). %s: %d\n", __FUNCTION__, __LINE__, #value, value); \
	assert(false); \
} while(0)

#endif   // __ANDROID__
#endif   // COMPILER_DEBUG_H_

