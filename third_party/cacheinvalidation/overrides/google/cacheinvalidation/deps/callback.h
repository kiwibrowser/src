// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_CACHEINVALIDATION_DEPS_CALLBACK_H_
#define GOOGLE_CACHEINVALIDATION_DEPS_CALLBACK_H_

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"

#define INVALIDATION_CALLBACK1_TYPE(Arg1) ::base::Callback<void(Arg1)>

// Below are a collection of types and functions that adapt base::Callback's
// pass-by-value semantics to the pointer-based callback system that
// cacheinvalidation needs.

namespace invalidation {

typedef ::base::Closure Closure;

template <class T>
bool IsCallbackRepeatable(const T* callback) {
  // The default cacheinvalidation Callbacks may be self-deleting. We don't
  // support this behave, so we already return true to indicate that the
  // cacheinvalidation implementation should delete our Callbacks.
  return true;
}

namespace internal {

// Identity<T>::type is a typedef of T. Useful for preventing the
// compiler from inferring the type of an argument in templates.
template <typename T>
struct Identity {
  typedef T type;
};

}  // namespace internal

// The cacheinvalidation callback system expects to take the callback by
// pointer and handle the ownership semantics itself.  Adapting the
// Chromium Callback system requires returning a dynamically allocated
// copy of the result of Bind().

inline Closure* NewPermanentCallback(void (*fn)()) {
  return new ::base::Closure(::base::Bind(fn));
}

template <class T1, class T2>
Closure* NewPermanentCallback(
    T1* object, void (T2::*method)()) {
  return new ::base::Closure(::base::Bind(method, base::Unretained(object)));
}

template <class T1, class T2, typename Arg1>
::base::Callback<void(Arg1)>* NewPermanentCallback(
    T1* object, void (T2::*method)(Arg1)) {
  return new ::base::Callback<void(Arg1)>(
      ::base::Bind(method, base::Unretained(object)));
}

template <class T1, class T2, typename Arg1>
Closure* NewPermanentCallback(
    T1* object,
    void (T2::*method)(Arg1),
    typename internal::Identity<Arg1>::type arg1) {
  return new ::base::Closure(::base::Bind(method, base::Unretained(object),
                                          arg1));
}

template <typename Arg1, typename Arg2>
Closure* NewPermanentCallback(
    void (*fn)(Arg1, Arg2),
    typename internal::Identity<Arg1>::type arg1,
    typename internal::Identity<Arg2>::type arg2) {
  return new ::base::Closure(::base::Bind(fn, arg1, arg2));
}

template <class T1, class T2, typename Arg1, typename Arg2>
Closure* NewPermanentCallback(
    T1* object,
    void (T2::*method)(Arg1, Arg2),
    typename internal::Identity<Arg1>::type arg1,
    typename internal::Identity<Arg2>::type arg2) {
  return new ::base::Closure(::base::Bind(method, base::Unretained(object),
                                          arg1, arg2));
}

template <class T1, class T2, typename Arg1, typename Arg2>
::base::Callback<void(Arg2)>* NewPermanentCallback(
    T1* object,
    void (T2::*method)(Arg1, Arg2),
    typename internal::Identity<Arg1>::type arg1) {
  return new ::base::Callback<void(Arg2)>(
      ::base::Bind(method, base::Unretained(object), arg1));
}

template <class T1, class T2, typename Arg1, typename Arg2, typename Arg3>
Closure* NewPermanentCallback(
    T1* object,
    void (T2::*method)(Arg1, Arg2, Arg3),
    typename internal::Identity<Arg1>::type arg1,
    typename internal::Identity<Arg2>::type arg2,
    typename internal::Identity<Arg3>::type arg3) {
  return new ::base::Closure(::base::Bind(method, base::Unretained(object),
                                          arg1, arg2, arg3));
}

template <class T1, class T2, typename Arg1, typename Arg2, typename Arg3,
          typename Arg4>
Closure* NewPermanentCallback(
    T1* object,
    void (T2::*method)(Arg1, Arg2, Arg3, Arg4),
    typename internal::Identity<Arg1>::type arg1,
    typename internal::Identity<Arg2>::type arg2,
    typename internal::Identity<Arg3>::type arg3,
    typename internal::Identity<Arg4>::type arg4) {
  return new ::base::Closure(::base::Bind(method, base::Unretained(object),
                                          arg1, arg2, arg3, arg4));
}

// Creates a Closure that runs |callback| on |arg|. The returned Closure owns
// |callback|.
template <typename ArgType>
Closure* NewPermanentCallback(
    INVALIDATION_CALLBACK1_TYPE(ArgType)* callback,
    typename internal::Identity<ArgType>::type arg) {
  std::unique_ptr<::base::Callback<void(ArgType)>> deleter(callback);
  return new ::base::Closure(::base::Bind(*callback, arg));
}

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_DEPS_CALLBACK_H_
