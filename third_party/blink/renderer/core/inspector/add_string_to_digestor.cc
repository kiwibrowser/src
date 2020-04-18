// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/inspector/add_string_to_digestor.h"

#include "third_party/blink/public/platform/web_crypto.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

void AddStringToDigestor(WebCryptoDigestor* digestor, const String& string) {
  const CString c_string = string.Utf8();
  digestor->Consume(reinterpret_cast<const unsigned char*>(c_string.data()),
                    c_string.length());
}

}  // namespace blink
