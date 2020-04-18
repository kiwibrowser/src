// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_STRING_TRAITS_H_
#define MOJO_PUBLIC_CPP_BINDINGS_STRING_TRAITS_H_

#include "mojo/public/cpp/bindings/lib/template_util.h"

namespace mojo {

// This must be specialized for any type |T| to be serialized/deserialized as
// a mojom string.
//
// Imagine you want to specialize it for CustomString, usually you need to
// implement:
//
//   template <T>
//   struct StringTraits<CustomString> {
//     // These two methods are optional. Please see comments in struct_traits.h
//     static bool IsNull(const CustomString& input);
//     static void SetToNull(CustomString* output);
//
//     static size_t GetSize(const CustomString& input);
//     static const char* GetData(const CustomString& input);
//
//     // The caller guarantees that |!input.is_null()|.
//     static bool Read(StringDataView input, CustomString* output);
//   };
//
// In some cases, you may need to do conversion before you can return the size
// and data as 8-bit characters for serialization. (For example, CustomString is
// UTF-16 string). In that case, you can add two optional methods:
//
//   static void* SetUpContext(const CustomString& input);
//   static void TearDownContext(const CustomString& input, void* context);
//
// And then you append a second parameter, void* context, to GetSize() and
// GetData():
//
//   static size_t GetSize(const CustomString& input, void* context);
//   static const char* GetData(const CustomString& input, void* context);
//
// If a CustomString instance is not null, the serialization code will call
// SetUpContext() at the beginning, and pass the resulting context pointer to
// GetSize()/GetData(). After serialization is done, it calls TearDownContext()
// so that you can do any necessary cleanup.
//
template <typename T>
struct StringTraits {
  static_assert(internal::AlwaysFalse<T>::value,
                "Cannot find the mojo::StringTraits specialization. Did you "
                "forget to include the corresponding header file?");
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_STRING_TRAITS_H_
