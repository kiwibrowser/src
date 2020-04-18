// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_STRING16_MOJOM_TRAITS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_STRING16_MOJOM_TRAITS_H_

#include "base/containers/span.h"
#include "base/strings/string16.h"
#include "mojo/public/cpp/bindings/struct_traits.h"
#include "mojo/public/mojom/base/string16.mojom-blink.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace mojo_base {
class BigBuffer;
}

namespace mojo {

template <>
struct PLATFORM_EXPORT
    StructTraits<mojo_base::mojom::String16DataView, WTF::String> {
  static bool IsNull(const WTF::String& input) { return input.IsNull(); }
  static void SetToNull(WTF::String* output) { *output = WTF::String(); }

  static void* SetUpContext(const WTF::String& input);
  static void TearDownContext(const WTF::String& input, void* context);

  static base::span<const uint16_t> data(const WTF::String& input,
                                         void* context);
  static bool Read(mojo_base::mojom::String16DataView, WTF::String* out);
};

template <>
struct PLATFORM_EXPORT
    StructTraits<mojo_base::mojom::BigString16DataView, WTF::String> {
  static bool IsNull(const WTF::String& input) { return input.IsNull(); }
  static void SetToNull(WTF::String* output) { *output = WTF::String(); }

  static mojo_base::BigBuffer data(const WTF::String& input);
  static bool Read(mojo_base::mojom::BigString16DataView, WTF::String* out);
};

}  // namespace mojo

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_STRING16_MOJOM_TRAITS_H_
