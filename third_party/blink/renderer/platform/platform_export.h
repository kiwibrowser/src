/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// This header defines the following macros to export component's symbols.
//
// - PLATFORM_EXPORT
//   Exports non-template symbols.
//
// - PLATFORM_TEMPLATE_CLASS_EXPORT
//   Exports an entire definition of class template.
//
// - PLATFORM_EXTERN_TEMPLATE_EXPORT
//   Applicable to template declarations (except for definitions). The
//   corresponding definition must come along with PLATFORM_TEMPLATE_EXPORT.
//   Template specialization uses this macro to declare that such a
//   specialization exists without providing an actual definition.
//
// - PLATFORM_TEMPLATE_EXPORT
//   Applicable to template definitions whose declarations are annotated
//   with PLATFORM_EXTERN_TEMPLATE_EXPORT. Template specialization uses this
//   macro to provide an actual definition.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_PLATFORM_EXPORT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_PLATFORM_EXPORT_H_

#include "build/build_config.h"

//
// BLINK_PLATFORM_IMPLEMENTATION
//
#if !defined(BLINK_PLATFORM_IMPLEMENTATION)
#define BLINK_PLATFORM_IMPLEMENTATION 0
#endif

//
// PLATFORM_EXPORT
//
#if !defined(COMPONENT_BUILD)
#define PLATFORM_EXPORT  // No need of export
#else

#if defined(COMPILER_MSVC)
#if BLINK_PLATFORM_IMPLEMENTATION
#define PLATFORM_EXPORT __declspec(dllexport)
#else
#define PLATFORM_EXPORT __declspec(dllimport)
#endif
#endif  // defined(COMPILER_MSVC)

#if defined(COMPILER_GCC)
#if BLINK_PLATFORM_IMPLEMENTATION
#define PLATFORM_EXPORT __attribute__((visibility("default")))
#else
#define PLATFORM_EXPORT
#endif
#endif  // defined(COMPILER_GCC)

#endif  // !defined(COMPONENT_BUILD)

//
// PLATFORM_TEMPLATE_CLASS_EXPORT
// PLATFORM_EXTERN_TEMPLATE_EXPORT
// PLATFORM_TEMPLATE_EXPORT
//
#if BLINK_PLATFORM_IMPLEMENTATION

#if defined(COMPILER_MSVC)
#define PLATFORM_TEMPLATE_CLASS_EXPORT
#define PLATFORM_EXTERN_TEMPLATE_EXPORT PLATFORM_EXPORT
#define PLATFORM_TEMPLATE_EXPORT PLATFORM_EXPORT
#endif

#if defined(COMPILER_GCC)
#define PLATFORM_TEMPLATE_CLASS_EXPORT PLATFORM_EXPORT
#define PLATFORM_EXTERN_TEMPLATE_EXPORT PLATFORM_EXPORT
#define PLATFORM_TEMPLATE_EXPORT
#endif

#else  // BLINK_PLATFORM_IMPLEMENTATION

#define PLATFORM_TEMPLATE_CLASS_EXPORT
#define PLATFORM_EXTERN_TEMPLATE_EXPORT PLATFORM_EXPORT
#define PLATFORM_TEMPLATE_EXPORT

#endif  // BLINK_PLATFORM_IMPLEMENTATION

#if defined(COMPILER_MSVC)
// MSVC Compiler warning C4275:
// non dll-interface class 'Bar' used as base for dll-interface class 'Foo'.
// Note that this is intended to be used only when no access to the base class'
// static data is done through derived classes or inline methods. For more info,
// see http://msdn.microsoft.com/en-us/library/3tdb471s(VS.80).aspx
//
// This pragma will allow exporting a class that inherits from a non-exported
// base class, anywhere in the Blink platform component. This is only
// a problem when using the MSVC compiler on Windows.
#pragma warning(suppress : 4275)
#endif

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_PLATFORM_EXPORT_H_
