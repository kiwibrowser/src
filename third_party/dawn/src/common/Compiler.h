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

#ifndef COMMON_COMPILER_H_
#define COMMON_COMPILER_H_

// Defines macros for compiler-specific functionality
//  - DAWN_COMPILER_[CLANG|GCC|MSVC]: Compiler detection
//  - DAWN_BREAKPOINT(): Raises an exception and breaks in the debugger
//  - DAWN_BUILTIN_UNREACHABLE(): Hints the compiler that a code path is unreachable
//  - DAWN_NO_DISCARD: An attribute that is C++17 [[nodiscard]] where available
//  - DAWN_(UN)?LIKELY(EXPR): Where available, hints the compiler that the expression will be true
//      (resp. false) to help it generate code that leads to better branch prediction.
//  - DAWN_UNUSED(EXPR): Prevents unused variable/expression warnings on EXPR.
//  - DAWN_UNUSED_FUNC(FUNC): Prevents unused function warnings on FUNC.
//  - DAWN_DECLARE_UNUSED:    Prevents unused function warnings a subsequent declaration.
//  Both DAWN_UNUSED_FUNC and DAWN_DECLARE_UNUSED may be necessary, e.g. to suppress clang's
//  unneeded-internal-declaration warning.

// Clang and GCC, check for __clang__ too to catch clang-cl masquarading as MSVC
#if defined(__GNUC__) || defined(__clang__)
#    if defined(__clang__)
#        define DAWN_COMPILER_CLANG
#    else
#        define DAWN_COMPILER_GCC
#    endif

#    if defined(__i386__) || defined(__x86_64__)
#        define DAWN_BREAKPOINT() __asm__ __volatile__("int $3\n\t")
#    else
// TODO(cwallez@chromium.org): Implement breakpoint on all supported architectures
#        define DAWN_BREAKPOINT()
#    endif

#    define DAWN_BUILTIN_UNREACHABLE() __builtin_unreachable()
#    define DAWN_LIKELY(x) __builtin_expect(!!(x), 1)
#    define DAWN_UNLIKELY(x) __builtin_expect(!!(x), 0)

#    if !defined(__has_cpp_attribute)
#        define __has_cpp_attribute(name) 0
#    endif

// Use warn_unused_result on clang otherwise we can a c++1z extension warning in C++14 mode
// Also avoid warn_unused_result with GCC because it is only a function attribute and not a type
// attribute.
#    if __has_cpp_attribute(warn_unused_result) && defined(__clang__)
#        define DAWN_NO_DISCARD __attribute__((warn_unused_result))
#    elif DAWN_CPP_VERSION >= 17 && __has_cpp_attribute(nodiscard)
#        define DAWN_NO_DISCARD [[nodiscard]]
#    endif

#    define DAWN_DECLARE_UNUSED __attribute__((unused))
#    if defined(NDEBUG)
#        define DAWN_FORCE_INLINE inline __attribute__((always_inline))
#    endif

// MSVC
#elif defined(_MSC_VER)
#    define DAWN_COMPILER_MSVC

extern void __cdecl __debugbreak(void);
#    define DAWN_BREAKPOINT() __debugbreak()

#    define DAWN_BUILTIN_UNREACHABLE() __assume(false)

// Visual Studio 2017 15.3 adds support for [[nodiscard]]
#    if _MSC_VER >= 1911 && DAWN_CPP_VERSION >= 17
#        define DAWN_NO_DISCARD [[nodiscard]]
#    endif

#    define DAWN_DECLARE_UNUSED
#    if defined(NDEBUG)
#        define DAWN_FORCE_INLINE __forceinline
#    endif

#else
#    error "Unsupported compiler"
#endif

// It seems that (void) EXPR works on all compilers to silence the unused variable warning.
#define DAWN_UNUSED(EXPR) (void)EXPR
// Likewise using static asserting on sizeof(&FUNC) seems to make it tagged as used
#define DAWN_UNUSED_FUNC(FUNC) static_assert(sizeof(&FUNC) == sizeof(void (*)()), "")

// Add noop replacements for macros for features that aren't supported by the compiler.
#if !defined(DAWN_LIKELY)
#    define DAWN_LIKELY(X) X
#endif
#if !defined(DAWN_UNLIKELY)
#    define DAWN_UNLIKELY(X) X
#endif
#if !defined(DAWN_NO_DISCARD)
#    define DAWN_NO_DISCARD
#endif
#if !defined(DAWN_FORCE_INLINE)
#    define DAWN_FORCE_INLINE inline
#endif

#if defined(__clang__)
#    define DAWN_FALLTHROUGH [[clang::fallthrough]]
#else
#    define DAWN_FALLTHROUGH
#endif

#endif  // COMMON_COMPILER_H_
