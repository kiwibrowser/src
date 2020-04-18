// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/mojo/big_string_mojom_traits.h"

#include <cstring>

#include "base/containers/span.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/base/big_buffer_mojom_traits.h"
#include "third_party/blink/renderer/platform/wtf/text/string_utf8_adaptor.h"

namespace mojo {

// static
mojo_base::BigBuffer StructTraits<mojo_base::mojom::BigStringDataView,
                                  WTF::String>::data(const WTF::String& input) {
  WTF::StringUTF8Adaptor adaptor(input);
  return mojo_base::BigBuffer(
      base::make_span(reinterpret_cast<const uint8_t*>(adaptor.Data()),
                      adaptor.length() * sizeof(char)));
}

// static
bool StructTraits<mojo_base::mojom::BigStringDataView, WTF::String>::Read(
    mojo_base::mojom::BigStringDataView data,
    WTF::String* out) {
  mojo_base::BigBuffer buffer;
  if (!data.ReadData(&buffer))
    return false;
  size_t size = buffer.size();
  if (size % sizeof(char))
    return false;
  // An empty |mojo_base::BigBuffer| may have a null |data()| if empty.
  if (!size) {
    *out = g_empty_string;
  } else {
    *out = WTF::String::FromUTF8(reinterpret_cast<const char*>(buffer.data()),
                                 size / sizeof(char));
  }
  return true;
}

}  // namespace mojo
