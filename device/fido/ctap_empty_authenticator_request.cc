// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/ctap_empty_authenticator_request.h"

#include "base/numerics/safe_conversions.h"

namespace device {

namespace internal {

std::vector<uint8_t> CtapEmptyAuthenticatorRequest::Serialize() const {
  return std::vector<uint8_t>{base::strict_cast<uint8_t>(cmd_)};
}

}  // namespace internal

}  // namespace device
