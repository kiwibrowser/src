// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/i18n/icu_util.h"
#include "content/browser/web_package/signed_exchange_header_parser.h"  // nogncheck

namespace content {

struct IcuEnvironment {
  IcuEnvironment() { CHECK(base::i18n::InitializeICU()); }
  // used by ICU integration.
  base::AtExitManager at_exit_manager;
};

IcuEnvironment* env = new IcuEnvironment();

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  base::StringPiece input(reinterpret_cast<const char*>(data), size);
  SignedExchangeHeaderParser::ParseSignature(input,
                                             nullptr /* devtools_proxy */);
  return 0;
}

}  // namespace content
