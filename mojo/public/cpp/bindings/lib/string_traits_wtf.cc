// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/string_traits_wtf.h"

#include <string.h>

#include "base/logging.h"
#include "mojo/public/cpp/bindings/lib/array_internal.h"
#include "mojo/public/cpp/bindings/string_data_view.h"
#include "third_party/blink/renderer/platform/wtf/text/string_utf8_adaptor.h"

namespace mojo {
namespace {

struct UTF8AdaptorInfo {
  explicit UTF8AdaptorInfo(const WTF::String& input) : utf8_adaptor(input) {
#if DCHECK_IS_ON()
    original_size_in_bytes = input.CharactersSizeInBytes();
#endif
  }

  ~UTF8AdaptorInfo() {}

  WTF::StringUTF8Adaptor utf8_adaptor;

#if DCHECK_IS_ON()
  // For sanity check only.
  size_t original_size_in_bytes;
#endif
};

UTF8AdaptorInfo* ToAdaptor(const WTF::String& input, void* context) {
  UTF8AdaptorInfo* adaptor = static_cast<UTF8AdaptorInfo*>(context);

#if DCHECK_IS_ON()
  DCHECK_EQ(adaptor->original_size_in_bytes, input.CharactersSizeInBytes());
#endif
  return adaptor;
}

}  // namespace

// static
void StringTraits<WTF::String>::SetToNull(WTF::String* output) {
  if (output->IsNull())
    return;

  WTF::String result;
  output->swap(result);
}

// static
void* StringTraits<WTF::String>::SetUpContext(const WTF::String& input) {
  return new UTF8AdaptorInfo(input);
}

// static
void StringTraits<WTF::String>::TearDownContext(const WTF::String& input,
                                                void* context) {
  delete ToAdaptor(input, context);
}

// static
size_t StringTraits<WTF::String>::GetSize(const WTF::String& input,
                                          void* context) {
  return ToAdaptor(input, context)->utf8_adaptor.length();
}

// static
const char* StringTraits<WTF::String>::GetData(const WTF::String& input,
                                               void* context) {
  return ToAdaptor(input, context)->utf8_adaptor.Data();
}

// static
bool StringTraits<WTF::String>::Read(StringDataView input,
                                     WTF::String* output) {
  WTF::String result = WTF::String::FromUTF8(input.storage(), input.size());
  output->swap(result);
  return true;
}

}  // namespace mojo
