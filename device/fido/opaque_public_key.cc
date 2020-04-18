// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/opaque_public_key.h"

namespace device {

OpaquePublicKey::OpaquePublicKey(
    base::span<const uint8_t> cose_encoded_public_key)
    : PublicKey(),
      cose_encoding_(std::vector<uint8_t>(cose_encoded_public_key.cbegin(),
                                          cose_encoded_public_key.cend())) {}

OpaquePublicKey::~OpaquePublicKey() = default;

std::vector<uint8_t> OpaquePublicKey::EncodeAsCOSEKey() const {
  return cose_encoding_;
}

}  // namespace device
