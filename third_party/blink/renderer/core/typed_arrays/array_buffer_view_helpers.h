// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_ARRAY_BUFFER_VIEW_HELPERS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_ARRAY_BUFFER_VIEW_HELPERS_H_

#include <type_traits>
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer_view.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/type_traits.h"

namespace blink {

// A wrapper template type that is used to ensure that a TypedArray is not
// backed by a SharedArrayBuffer.
//
// Typically this is used as an annotation on C++ functions that are called by
// the bindings layer, e.g.:
//
//   void Foo(NotShared<DOMUint32Array> param) {
//     DOMUint32Array* array = param.View();
//     ...
//   }
template <typename T>
class NotShared {
  static_assert(WTF::IsSubclass<typename std::remove_const<T>::type,
                                DOMArrayBufferView>::value,
                "NotShared<T> must have T as subclass of DOMArrayBufferView");
  STACK_ALLOCATED();

 public:
  using TypedArrayType = T;

  NotShared() = default;

  explicit NotShared(T* typedArray) : typed_array_(typedArray) {
    DCHECK(!(typedArray && typedArray->View()->IsShared()));
  }
  NotShared(const NotShared& other) = default;
  template <typename U>
  NotShared(const NotShared<U>& other) : typed_array_(other.View()) {}
  template <typename U>
  NotShared(const Member<U>& other) {
    DCHECK(!other->View()->IsShared());
    typed_array_ = other.Get();
  }

  NotShared& operator=(const NotShared& other) = default;
  template <typename U>
  NotShared& operator=(const NotShared<U>& other) {
    typed_array_ = other.View();
    return *this;
  }

  T* View() const { return typed_array_.Get(); }

  bool operator!() const { return !typed_array_; }
  explicit operator bool() const { return !!typed_array_; }

 private:
  // Must use an untraced member here since this object may be constructed on a
  // thread without a ThreadState (e.g. an Audio worklet). It is safe in that
  // case because the pointed-to ArrayBuffer is being kept alive another way
  // (e.g. CrossThreadPersistent).
  //
  // TODO(binji): update to using Member, see crbug.com/710295.
  UntracedMember<T> typed_array_;
};

// A wrapper template type that specifies that a TypedArray may be backed by a
// SharedArrayBuffer.
//
// Typically this is used as an annotation on C++ functions that are called by
// the bindings layer, e.g.:
//
//   void Foo(MaybeShared<DOMUint32Array> param) {
//     DOMUint32Array* array = param.View();
//     ...
//   }
template <typename T>
class MaybeShared {
  static_assert(WTF::IsSubclass<typename std::remove_const<T>::type,
                                DOMArrayBufferView>::value,
                "MaybeShared<T> must have T as subclass of DOMArrayBufferView");
  STACK_ALLOCATED();

 public:
  using TypedArrayType = T;

  MaybeShared() = default;

  explicit MaybeShared(T* typedArray) : typed_array_(typedArray) {}
  MaybeShared(const MaybeShared& other) = default;
  template <typename U>
  MaybeShared(const MaybeShared<U>& other) : typed_array_(other.View()) {}
  template <typename U>
  MaybeShared(const Member<U>& other) {
    typed_array_ = other.Get();
  }

  MaybeShared& operator=(const MaybeShared& other) = default;
  template <typename U>
  MaybeShared& operator=(const MaybeShared<U>& other) {
    typed_array_ = other.View();
    return *this;
  }

  T* View() const { return typed_array_.Get(); }

  bool operator!() const { return !typed_array_; }
  explicit operator bool() const { return !!typed_array_; }

 private:
  Member<T> typed_array_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_ARRAY_BUFFER_VIEW_HELPERS_H_
