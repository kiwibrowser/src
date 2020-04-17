// Copyright 2017 The Dawn Authors
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

#include "common/Platform.h"

#if defined(DAWN_PLATFORM_WINDOWS)
#    include <Windows.h>
#elif defined(DAWN_PLATFORM_POSIX)
#    include <unistd.h>
#else
#    error "Unsupported platform."
#endif

namespace utils {

#if defined(DAWN_PLATFORM_WINDOWS)
    void USleep(unsigned int usecs) {
        Sleep(static_cast<DWORD>(usecs / 1000));
    }
#elif defined(DAWN_PLATFORM_POSIX)
    void USleep(unsigned int usecs) {
        usleep(usecs);
    }
#else
#    error "Implement USleep for your platform."
#endif

}  // namespace utils
