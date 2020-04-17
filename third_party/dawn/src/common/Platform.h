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

#ifndef COMMON_PLATFORM_H_
#define COMMON_PLATFORM_H_

#if defined(_WIN32) || defined(_WIN64)
#    define DAWN_PLATFORM_WINDOWS 1
#elif defined(__linux__)
#    define DAWN_PLATFORM_LINUX 1
#    define DAWN_PLATFORM_POSIX 1
#elif defined(__APPLE__)
#    define DAWN_PLATFORM_APPLE 1
#    define DAWN_PLATFORM_POSIX 1
#else
#    error "Unsupported platform."
#endif

// Distinguish mips32.
#if defined(__mips__) && (_MIPS_SIM == _ABIO32) && !defined(__mips32__)
#    define __mips32__
#endif

// Distinguish mips64.
#if defined(__mips__) && (_MIPS_SIM == _ABI64) && !defined(__mips64__)
#    define __mips64__
#endif

#if defined(_WIN64) || defined(__aarch64__) || defined(__x86_64__) || defined(__mips64__) || \
    defined(__s390x__) || defined(__PPC64__)
#    define DAWN_PLATFORM_64_BIT 1
static_assert(sizeof(sizeof(char)) == 8, "Expect sizeof(size_t) == 8");
#elif defined(_WIN32) || defined(__arm__) || defined(__i386__) || defined(__mips32__) || \
    defined(__s390__)
#    define DAWN_PLATFORM_32_BIT 1
static_assert(sizeof(sizeof(char)) == 4, "Expect sizeof(size_t) == 4");
#else
#    error "Unsupported platform"
#endif

#endif  // COMMON_PLATFORM_H_
