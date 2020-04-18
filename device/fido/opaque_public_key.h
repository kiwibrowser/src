// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_OPAQUE_PUBLIC_KEY_H_
#define DEVICE_FIDO_OPAQUE_PUBLIC_KEY_H_

#include <stdint.h>
#include <vector>

#include "base/component_export.h"
#include "base/containers/span.h"
#include "base/macros.h"
#include "device/fido/public_key.h"

namespace device {

// An COSE key format encoded public key representation received from CTAP2
// device as defined in section 7 of the RFC 8152.
// https://tools.ietf.org/html/rfc8152#section-7
class COMPONENT_EXPORT(DEVICE_FIDO) OpaquePublicKey : public PublicKey {
 public:
  // TODO(hongjunchoi): Add static factory method that checks COSE encoding
  // format of the received byte array.
  // See https://www.crbug.com/824636
  OpaquePublicKey(base::span<const uint8_t> cose_encoded_public_key);
  ~OpaquePublicKey() override;

  std::vector<uint8_t> EncodeAsCOSEKey() const override;

 private:
  const std::vector<uint8_t> cose_encoding_;

  DISALLOW_COPY_AND_ASSIGN(OpaquePublicKey);
};

}  // namespace device

#endif  // DEVICE_FIDO_OPAQUE_PUBLIC_KEY_H_
