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

#ifndef COMMON_ASSERT_H_
#define COMMON_ASSERT_H_

#include "common/Compiler.h"

// Dawn asserts to be used instead of the regular C stdlib assert function (if you don't use assert
// yet, you should start now!). In debug ASSERT(condition) will trigger an error, otherwise in
// release it does nothing at runtime.
//
// In case of name clashes (with for example a testing library), you can define the
// DAWN_SKIP_ASSERT_SHORTHANDS to only define the DAWN_ prefixed macros.
//
// These asserts feature:
//     - Logging of the error with file, line and function information.
//     - Breaking in the debugger when an assert is triggered and a debugger is attached.
//     - Use the assert information to help the compiler optimizer in release builds.

// MSVC triggers a warning in /W4 for do {} while(0). SDL worked around this by using (0,0) and
// points out that it looks like an owl face.
#if defined(DAWN_COMPILER_MSVC)
#    define DAWN_ASSERT_LOOP_CONDITION (0, 0)
#else
#    define DAWN_ASSERT_LOOP_CONDITION (0)
#endif

// DAWN_ASSERT_CALLSITE_HELPER generates the actual assert code. In Debug it does what you would
// expect of an assert and in release it tries to give hints to make the compiler generate better
// code.
#if defined(DAWN_ENABLE_ASSERTS)
#    define DAWN_ASSERT_CALLSITE_HELPER(file, func, line, condition)  \
        do {                                                          \
            if (!(condition)) {                                       \
                HandleAssertionFailure(file, func, line, #condition); \
            }                                                         \
        } while (DAWN_ASSERT_LOOP_CONDITION)
#else
#    if defined(DAWN_COMPILER_MSVC)
#        define DAWN_ASSERT_CALLSITE_HELPER(file, func, line, condition) __assume(condition)
#    elif defined(DAWN_COMPILER_CLANG) && defined(__builtin_assume)
#        define DAWN_ASSERT_CALLSITE_HELPER(file, func, line, condition) __builtin_assume(condition)
#    else
#        define DAWN_ASSERT_CALLSITE_HELPER(file, func, line, condition) \
            do {                                                         \
                DAWN_UNUSED(sizeof(condition));                          \
            } while (DAWN_ASSERT_LOOP_CONDITION)
#    endif
#endif

#define DAWN_ASSERT(condition) DAWN_ASSERT_CALLSITE_HELPER(__FILE__, __func__, __LINE__, condition)
#define DAWN_UNREACHABLE()                                                 \
    do {                                                                   \
        DAWN_ASSERT(DAWN_ASSERT_LOOP_CONDITION && "Unreachable code hit"); \
        DAWN_BUILTIN_UNREACHABLE();                                        \
    } while (DAWN_ASSERT_LOOP_CONDITION)

#if !defined(DAWN_SKIP_ASSERT_SHORTHANDS)
#    define ASSERT DAWN_ASSERT
#    define UNREACHABLE DAWN_UNREACHABLE
#endif

void HandleAssertionFailure(const char* file,
                            const char* function,
                            int line,
                            const char* condition);

#endif  // COMMON_ASSERT_H_
