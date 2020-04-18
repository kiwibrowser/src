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

#ifndef DebugAndroid_hpp
#define DebugAndroid_hpp

#include <cutils/log.h>
#include <cassert>

// On Android Virtual Devices we heavily depend on logging, even in
// production builds. We do this because AVDs are components of larger
// systems, and may be configured in ways that are difficult to
// reproduce locally. For example some system run tests against
// third-party code that we cannot access.  Aborting (cf. assert) on
// unimplemented functionality creates two problems. First, it produces
// a service failure where none is needed. Second, it puts the
// customer on the critical path for notifying us of a problem.
// The alternative, skipping unimplemented functionality silently, is
// arguably worse: neither the service provider nor the customer will
// learn that unimplemented functionality may have compromised the test
// results.
// Logging invocations of unimplemented functionality is useful to both
// service provider and the customer. The service provider can learn
// that the functionality is needed. The customer learns that the test
// results may be compromised.

/**
 * Enter the debugger with a memory fault iff debuggerd is set to capture this
 * process. Otherwise return.
 */
void AndroidEnterDebugger();

#define ASSERT(E) do { \
		if (!(E)) { \
			ALOGE("badness: assertion_failed %s in %s at %s:%d", #E,	\
				  __FUNCTION__, __FILE__, __LINE__);					\
			AndroidEnterDebugger();										\
		}																\
	} while(0)

#undef assert
#define assert(E) ASSERT(E)

#define ERR(format, ...)												\
	do {																\
		ALOGE("badness: err %s %s:%d (" format ")", __FUNCTION__, __FILE__, \
			  __LINE__, ##__VA_ARGS__);									\
		AndroidEnterDebugger();											\
	} while(0)

#define FIXME(format, ...)												\
	do {																\
		ALOGE("badness: fixme %s %s:%d (" format ")", __FUNCTION__, __FILE__, \
			  __LINE__, ##__VA_ARGS__);									\
		AndroidEnterDebugger();											\
	} while(0)

#define UNIMPLEMENTED() do {						\
		ALOGE("badness: unimplemented: %s %s:%d",	\
			  __FUNCTION__, __FILE__, __LINE__);	\
		AndroidEnterDebugger();						\
	} while(0)

#define UNREACHABLE(value) do {                                         \
		ALOGE("badness: unreachable case reached: %s %s:%d. %s: %d", \
			  __FUNCTION__, __FILE__, __LINE__, #value, value);			\
		AndroidEnterDebugger();                                         \
	} while(0)

#ifndef NDEBUG
	#define TRACE(format, ...)								   \
		ALOGV("%s %s:%d (" format ")", __FUNCTION__, __FILE__, \
			  __LINE__, ##__VA_ARGS__)
#else
	#define TRACE(...) ((void)0)
#endif

void trace(const char *format, ...);

#endif   // DebugAndroid_hpp
