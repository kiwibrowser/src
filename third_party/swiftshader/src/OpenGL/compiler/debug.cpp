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

// debug.cpp: Debugging utilities.

#include "debug.h"

#include <stdarg.h>
#include <stdio.h>

#include "InitializeParseContext.h"
#include "ParseHelper.h"

#ifdef TRACE_ENABLED
extern "C" {
void Trace(const char *format, ...) {
	if (!format) return;

	TParseContext* parseContext = GetGlobalParseContext();
	if (parseContext) {
		const int kTraceBufferLen = 1024;
		char buf[kTraceBufferLen];
		va_list args;
		va_start(args, format);
		vsnprintf(buf, kTraceBufferLen, format, args);
		va_end(args);

		parseContext->trace(buf);
	}
}
}  // extern "C"
#endif  // TRACE_ENABLED

