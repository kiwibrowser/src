// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_ATTESTED_CREDENTIAL_DATA_H_
#define DEVICE_FIDO_ATTESTED_CREDENTIAL_DATA_H_

#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <vector>

#include "base/component_export.h"
#include "base/containers/span.h"
#include "base/macros.h"
#include "base/optional.h"

namespace device {

class PublicKey;

// https://www.w3.org/TR/2017/WD-webauthn-20170505/#sec-attestation-data
class COMPONENT_EXPORT(DEVICE_FIDO) AttestedCredentialData {
 public:
  static base::Optional<AttestedCredentialData> DecodeFromCtapResponse(
      base::span<const uint8_t> buffer);

  static base::Optional<AttestedCredentialData> CreateFromU2fRegisterResponse(
      base::span<const uint8_t> u2f_data,
      std::unique_ptr<PublicKey> public_key);

  // Moveable.
  AttestedCredentialData(AttestedCredentialData&& other);
  AttestedCredentialData& operator=(AttestedCredentialData&& other);

  ~AttestedCredentialData();

  const std::vector<uint8_t>& credential_id() const { return credential_id_; }

  // Invoked when sending "none" attestation statement to the relying party.
  // Replaces AAGUID with zero bytes.
  void DeleteAaguid();

  // Produces a byte array consisting of:
  // * AAGUID (16 bytes)
  // * Len (2 bytes)
  // * Credential Id (Len bytes)
  // * Credential Public Key.
  std::vector<uint8_t> SerializeAsBytes() const;

  static constexpr size_t kAaguidLength = 16;
  // Number of bytes used to represent length of credential ID.
  static constexpr size_t kCredentialIdLengthLength = 2;

  AttestedCredentialData(
      std::array<uint8_t, kAaguidLength> aaguid,
      std::array<uint8_t, kCredentialIdLengthLength> credential_id_length,
      std::vector<uint8_t> credential_id,
      std::unique_ptr<PublicKey> public_key);

 private:
  // The 16-byte AAGUID of the authenticator.
  std::array<uint8_t, kAaguidLength> aaguid_;

  // Big-endian length of the credential (i.e. key handle).
  std::array<uint8_t, kCredentialIdLengthLength> credential_id_length_;

  std::vector<uint8_t> credential_id_;
  std::unique_ptr<PublicKey> public_key_;

  DISALLOW_COPY_AND_ASSIGN(AttestedCredentialData);
};

}  // namespace device

#endif  // DEVICE_FIDO_ATTESTED_CREDENTIAL_DATA_H_
