// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_STRING_TRAITS_WTF_H_
#define MOJO_PUBLIC_CPP_BINDINGS_STRING_TRAITS_WTF_H_

#include "mojo/public/cpp/bindings/lib/bindings_internal.h"
#include "mojo/public/cpp/bindings/string_traits.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace mojo {

template <>
struct StringTraits<WTF::String> {
  static bool IsNull(const WTF::String& input) { return input.IsNull(); }
  static void SetToNull(WTF::String* output);

  static void* SetUpContext(const WTF::String& input);
  static void TearDownContext(const WTF::String& input, void* context);

  static size_t GetSize(const WTF::String& input, void* context);

  static const char* GetData(const WTF::String& input, void* context);

  static bool Read(StringDataView input, WTF::String* output);
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_STRING_TRAITS_WTF_H_
