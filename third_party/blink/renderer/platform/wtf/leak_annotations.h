/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_LEAK_ANNOTATIONS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_LEAK_ANNOTATIONS_H_

// This file defines macros for working with LeakSanitizer, allowing memory
// and allocations to be registered as exempted from LSan consideration.

#include "base/macros.h"
#if defined(LEAK_SANITIZER)
#include "third_party/blink/renderer/platform/wtf/address_sanitizer.h"
#include "third_party/blink/renderer/platform/wtf/type_traits.h"
#endif

namespace WTF {

#if defined(LEAK_SANITIZER)
class LeakSanitizerDisabler {
 public:
  LeakSanitizerDisabler() { __lsan_disable(); }

  ~LeakSanitizerDisabler() { __lsan_enable(); }

  DISALLOW_COPY_AND_ASSIGN(LeakSanitizerDisabler);
};

// WTF_INTERNAL_LEAK_SANITIZER_DISABLED_SCOPE: all allocations made in the
// current scope will be exempted from LSan consideration. Only to be
// used internal to wtf/, Blink should use LEAK_SANITIZER_DISABLED_SCOPE
// elsewhere.
//
// TODO(sof): once layering rules allow wtf/ to make use of the Oilpan
// infrastructure, remove this macro.
#define WTF_INTERNAL_LEAK_SANITIZER_DISABLED_SCOPE  \
  WTF::LeakSanitizerDisabler leakSanitizerDisabler; \
  static_cast<void>(0)

// LEAK_SANITIZER_IGNORE_OBJECT(X): the heap object referenced by pointer X
// will be ignored by LSan.
//
// "Ignorance" means that LSan's reachability traversal is stopped short
// upon encountering an ignored memory chunk. Consequently, LSan will not
// scan an ignored memory chunk for live, reachable pointers. However, should
// those embedded pointers be reachable by some other path, they will be
// reported as leaking.
#define LEAK_SANITIZER_IGNORE_OBJECT(X) __lsan_ignore_object(X)

// If the object pointed to by the static local is on the Oilpan heap, a strong
// Persistent<> is created to keep the pointed-to heap object alive. This makes
// both the Persistent<> and the heap object _reachable_ by LeakSanitizer's leak
// detection pass. We do not want these intentional leaks to be reported by
// LSan, hence the static local is registered with Oilpan
// (see RegisterStaticLocalReference<> below.)
//
// Upon Blink shutdown, all the registered statics are released and a final
// round of GCs are performed to sweep out their now-unreachable object graphs.
// The end result being a tidied heap that the LeakSanitizer can then scan to
// report real leaks.
//
// The CanRegisterStaticLocalReference<> and RegisterStaticLocalReference<>
// templates arrange for this -- for a class type T, a registerStatic()
// implementation is provided if "T* T::registerAsStaticReference(T*)" is a
// method on T (inherited or otherwise.)
//
// An empty, trivial registerStatic() method is provided for all other class
// types T.
template <typename T>
class CanRegisterStaticLocalReference {
  typedef char YesType;
  typedef struct NoType { char padding[8]; } NoType;

  // Check if class T has public method "T* registerAsStaticReference()".
  template <typename V>
  static YesType checkHasRegisterAsStaticReferenceMethod(
      V* p,
      typename std::enable_if<IsSubclass<
          V,
          typename std::remove_pointer<decltype(
              p->registerAsStaticReference())>::type>::value>::type* = 0);
  template <typename V>
  static NoType checkHasRegisterAsStaticReferenceMethod(...);

 public:
  static const bool value =
      sizeof(YesType) + sizeof(T) ==
      sizeof(checkHasRegisterAsStaticReferenceMethod<T>(nullptr)) + sizeof(T);
};

template <typename T, bool = CanRegisterStaticLocalReference<T>::value>
class RegisterStaticLocalReference {
 public:
  static T* registerStatic(T* ptr) { return ptr; }
};

template <typename T>
class RegisterStaticLocalReference<T, true> {
 public:
  static T* registerStatic(T* ptr) {
    return static_cast<T*>(ptr->registerAsStaticReference());
  }
};

#define LEAK_SANITIZER_REGISTER_STATIC_LOCAL(Type, Object) \
  WTF::RegisterStaticLocalReference<Type>::registerStatic(Object)
#else
#define WTF_INTERNAL_LEAK_SANITIZER_DISABLED_SCOPE
#define LEAK_SANITIZER_IGNORE_OBJECT(X) ((void)0)
#define LEAK_SANITIZER_REGISTER_STATIC_LOCAL(Type, Object) Object
#endif  // defined(LEAK_SANITIZER)

}  // namespace WTF

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_LEAK_ANNOTATIONS_H_
