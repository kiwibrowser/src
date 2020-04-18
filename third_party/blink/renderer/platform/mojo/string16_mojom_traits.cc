// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/mojo/string16_mojom_traits.h"

#include <cstring>

#include "base/containers/span.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/base/big_buffer_mojom_traits.h"

namespace mojo {

// static
void* StructTraits<mojo_base::mojom::String16DataView,
                   WTF::String>::SetUpContext(const WTF::String& input) {
  // If it is null (i.e., StructTraits<>::IsNull() returns true), this method is
  // guaranteed not to be called.
  DCHECK(!input.IsNull());

  if (!input.Is8Bit())
    return nullptr;

  return new base::string16(input.Characters8(),
                            input.Characters8() + input.length());
}

// static
void StructTraits<mojo_base::mojom::String16DataView,
                  WTF::String>::TearDownContext(const WTF::String& input,
                                                void* context) {
  delete static_cast<base::string16*>(context);
}

// static
base::span<const uint16_t>
StructTraits<mojo_base::mojom::String16DataView, WTF::String>::data(
    const WTF::String& input,
    void* context) {
  auto* contextObject = static_cast<base::string16*>(context);
  DCHECK_EQ(input.Is8Bit(), !!contextObject);

  if (contextObject) {
    return base::make_span(
        reinterpret_cast<const uint16_t*>(contextObject->data()),
        contextObject->size());
  }

  return base::make_span(
      reinterpret_cast<const uint16_t*>(input.Characters16()), input.length());
}

// static
bool StructTraits<mojo_base::mojom::String16DataView, WTF::String>::Read(
    mojo_base::mojom::String16DataView data,
    WTF::String* out) {
  ArrayDataView<uint16_t> view;
  data.GetDataDataView(&view);
  *out = WTF::String(reinterpret_cast<const UChar*>(view.data()), view.size());
  return true;
}

// static
mojo_base::BigBuffer StructTraits<mojo_base::mojom::BigString16DataView,
                                  WTF::String>::data(const WTF::String& input) {
  if (input.Is8Bit()) {
    base::string16 input16(input.Characters8(),
                           input.Characters8() + input.length());
    return mojo_base::BigBuffer(
        base::make_span(reinterpret_cast<const uint8_t*>(input16.data()),
                        input16.size() * sizeof(UChar)));
  }

  return mojo_base::BigBuffer(
      base::make_span(reinterpret_cast<const uint8_t*>(input.Characters16()),
                      input.length() * sizeof(UChar)));
}

// static
bool StructTraits<mojo_base::mojom::BigString16DataView, WTF::String>::Read(
    mojo_base::mojom::BigString16DataView data,
    WTF::String* out) {
  mojo_base::BigBuffer buffer;
  if (!data.ReadData(&buffer))
    return false;
  size_t size = buffer.size();
  if (size % sizeof(UChar))
    return false;

  // An empty |mojo_base::BigBuffer| may have a null |data()| if empty.
  if (!size) {
    *out = g_empty_string;
  } else {
    *out = WTF::String(reinterpret_cast<const UChar*>(buffer.data()),
                       size / sizeof(UChar));
  }

  return true;
}

}  // namespace mojo
