// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_EC_PUBLIC_KEY_H_
#define DEVICE_FIDO_EC_PUBLIC_KEY_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/containers/span.h"
#include "base/macros.h"
#include "device/fido/public_key.h"

namespace device {

// An uncompressed ECPublicKey consisting of 64 bytes:
// - the 32-byte x coordinate
// - the 32-byte y coordinate.
class COMPONENT_EXPORT(DEVICE_FIDO) ECPublicKey : public PublicKey {
 public:
  static std::unique_ptr<ECPublicKey> ExtractFromU2fRegistrationResponse(
      std::string algorithm,
      base::span<const uint8_t> u2f_data);

  // Parse a public key encoded in ANSI X9.62 uncompressed format.
  static std::unique_ptr<ECPublicKey> ParseX962Uncompressed(
      std::string algorithm,
      base::span<const uint8_t> input);

  ECPublicKey(std::string algorithm,
              std::vector<uint8_t> x,
              std::vector<uint8_t> y);

  ~ECPublicKey() override;

  // Produces a key in COSE_key format, which is an integer-keyed CBOR map:
  // { 1 ("kty") : 2 (the EC2 key id),
  //   3 ("alg") : -7 (the ES256 COSEAlgorithmIdentifier),
  //  -1 ("crv"): 1 (the P-256 EC identifier),
  //  -2: x-coordinate,
  //  -3: y-coordinate }
  std::vector<uint8_t> EncodeAsCOSEKey() const override;

 private:
  // Note that these values might not be minimal and might not be on the curve.
  const std::vector<uint8_t> x_coordinate_;
  const std::vector<uint8_t> y_coordinate_;

  DISALLOW_COPY_AND_ASSIGN(ECPublicKey);
};

}  // namespace device

#endif  // DEVICE_FIDO_EC_PUBLIC_KEY_H_
