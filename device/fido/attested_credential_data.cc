// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/attested_credential_data.h"

#include <algorithm>
#include <utility>

#include "base/numerics/safe_math.h"
#include "device/fido/fido_parsing_utils.h"
#include "device/fido/opaque_public_key.h"
#include "device/fido/public_key.h"

namespace device {

// static
base::Optional<AttestedCredentialData>
AttestedCredentialData::DecodeFromCtapResponse(
    base::span<const uint8_t> buffer) {
  if (buffer.size() < kAaguidLength + kCredentialIdLengthLength)
    return base::nullopt;

  std::array<uint8_t, kAaguidLength> aaguid;
  if (!fido_parsing_utils::ExtractArray(buffer, 0, &aaguid))
    return base::nullopt;

  std::array<uint8_t, kCredentialIdLengthLength> credential_id_length_array;
  if (!fido_parsing_utils::ExtractArray(buffer, kAaguidLength,
                                        &credential_id_length_array)) {
    return base::nullopt;
  }

  static_assert(kCredentialIdLengthLength == 2u, "L must be 2 bytes");
  const size_t credential_id_length =
      (base::strict_cast<size_t>(credential_id_length_array[0]) << 8) |
      base::strict_cast<size_t>(credential_id_length_array[1]);

  auto credential_id = fido_parsing_utils::Extract(
      buffer, kAaguidLength + kCredentialIdLengthLength, credential_id_length);
  if (credential_id.empty())
    return base::nullopt;

  DCHECK_LE(kAaguidLength + kCredentialIdLengthLength + credential_id_length,
            buffer.size());
  auto credential_public_key_data =
      std::make_unique<OpaquePublicKey>(buffer.subspan(
          kAaguidLength + kCredentialIdLengthLength + credential_id_length));

  return AttestedCredentialData(
      std::move(aaguid), std::move(credential_id_length_array),
      std::move(credential_id), std::move(credential_public_key_data));
}

// static
base::Optional<AttestedCredentialData>
AttestedCredentialData::CreateFromU2fRegisterResponse(
    base::span<const uint8_t> u2f_data,
    std::unique_ptr<PublicKey> public_key) {
  // TODO(crbug/799075): Introduce a CredentialID class to do this extraction.
  // Extract the length of the credential (i.e. of the U2FResponse key
  // handle). Length is big endian.
  std::vector<uint8_t> extracted_length = fido_parsing_utils::Extract(
      u2f_data, fido_parsing_utils::kU2fResponseKeyHandleLengthPos, 1);

  if (extracted_length.empty()) {
    return base::nullopt;
  }

  // For U2F register request, device AAGUID is set to zeros.
  std::array<uint8_t, kAaguidLength> aaguid;
  aaguid.fill(0);

  // Note that U2F responses only use one byte for length.
  std::array<uint8_t, kCredentialIdLengthLength> credential_id_length = {
      0, extracted_length[0]};

  // Extract the credential id (i.e. key handle).
  std::vector<uint8_t> credential_id = fido_parsing_utils::Extract(
      u2f_data, fido_parsing_utils::kU2fResponseKeyHandleStartPos,
      base::strict_cast<size_t>(credential_id_length[1]));

  if (credential_id.empty()) {
    return base::nullopt;
  }

  return AttestedCredentialData(
      std::move(aaguid), std::move(credential_id_length),
      std::move(credential_id), std::move(public_key));
}

AttestedCredentialData::AttestedCredentialData(AttestedCredentialData&& other) =
    default;

AttestedCredentialData& AttestedCredentialData::operator=(
    AttestedCredentialData&& other) = default;

AttestedCredentialData::~AttestedCredentialData() = default;

void AttestedCredentialData::DeleteAaguid() {
  std::fill(aaguid_.begin(), aaguid_.end(), 0);
}

std::vector<uint8_t> AttestedCredentialData::SerializeAsBytes() const {
  std::vector<uint8_t> attestation_data;
  fido_parsing_utils::Append(&attestation_data,
                             base::make_span(aaguid_.data(), kAaguidLength));
  fido_parsing_utils::Append(
      &attestation_data,
      base::make_span(credential_id_length_.data(), kCredentialIdLengthLength));
  fido_parsing_utils::Append(&attestation_data, credential_id_);
  fido_parsing_utils::Append(&attestation_data, public_key_->EncodeAsCOSEKey());
  return attestation_data;
}

AttestedCredentialData::AttestedCredentialData(
    std::array<uint8_t, kAaguidLength> aaguid,
    std::array<uint8_t, kCredentialIdLengthLength> credential_id_length,
    std::vector<uint8_t> credential_id,
    std::unique_ptr<PublicKey> public_key)
    : aaguid_(std::move(aaguid)),
      credential_id_length_(std::move(credential_id_length)),
      credential_id_(std::move(credential_id)),
      public_key_(std::move(public_key)) {}

}  // namespace device
