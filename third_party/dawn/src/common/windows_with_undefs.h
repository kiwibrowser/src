// Copyright 2018 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef COMMON_WINDOWS_WITH_UNDEFS_H_
#define COMMON_WINDOWS_WITH_UNDEFS_H_

#include "common/Platform.h"

#if !defined(DAWN_PLATFORM_WINDOWS)
#    error "windows_with_undefs.h included on non-Windows"
#endif

// This header includes <windows.h> but removes all the extra defines that conflict with identifiers
// in internal code. It should never be included in something that is part of the public interface.
#include <windows.h>

// Macros defined for ANSI / Unicode support
#undef CreateWindow
#undef GetMessage

// Macros defined to produce compiler intrinsics
#undef MemoryBarrier

#endif  // COMMON_WINDOWS_WITH_UNDEFS_H_
