// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/testing/fuzzed_data_provider.h"

namespace blink {

FuzzedDataProvider::FuzzedDataProvider(const uint8_t* bytes, size_t num_bytes)
    : provider_(bytes, num_bytes) {}

CString FuzzedDataProvider::ConsumeBytesInRange(uint32_t min_bytes,
                                                uint32_t max_bytes) {
  size_t num_bytes =
      static_cast<size_t>(provider_.ConsumeUint32InRange(min_bytes, max_bytes));
  std::string bytes = provider_.ConsumeBytes(num_bytes);
  return CString(bytes.data(), bytes.size());
}

CString FuzzedDataProvider::ConsumeRemainingBytes() {
  std::string bytes = provider_.ConsumeRemainingBytes();
  return CString(bytes.data(), bytes.size());
}

bool FuzzedDataProvider::ConsumeBool() {
  return provider_.ConsumeBool();
}

int FuzzedDataProvider::ConsumeInt32InRange(int min, int max) {
  return provider_.ConsumeInt32InRange(min, max);
}

}  // namespace blink
