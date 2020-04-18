// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/authenticator_data.h"

#include <utility>

#include "device/fido/attested_credential_data.h"
#include "device/fido/fido_parsing_utils.h"

namespace device {

namespace {

constexpr size_t kApplicationParameterLength = 32;
constexpr size_t kAuthDataCounterLength = 4;
constexpr size_t kAaguidOffset =
    32 /* RP ID hash */ + 1 /* flags */ + 4 /* signature counter */;

}  // namespace

// static
base::Optional<AuthenticatorData> AuthenticatorData::DecodeAuthenticatorData(
    base::span<const uint8_t> auth_data) {
  if (auth_data.size() < kAaguidOffset)
    return base::nullopt;
  std::vector<uint8_t> application_parameter(
      auth_data.data(), auth_data.data() + kApplicationParameterLength);
  uint8_t flag_byte = auth_data[kApplicationParameterLength];
  std::vector<uint8_t> counter(
      auth_data.data() + kApplicationParameterLength + 1,
      auth_data.data() + kApplicationParameterLength + 1 +
          kAuthDataCounterLength);
  auto attested_credential_data =
      AttestedCredentialData::DecodeFromCtapResponse(
          auth_data.subspan(kAaguidOffset));

  return AuthenticatorData(std::move(application_parameter), flag_byte,
                           std::move(counter),
                           std::move(attested_credential_data));
}

AuthenticatorData::AuthenticatorData(
    std::vector<uint8_t> application_parameter,
    uint8_t flags,
    std::vector<uint8_t> counter,
    base::Optional<AttestedCredentialData> data)
    : application_parameter_(std::move(application_parameter)),
      flags_(flags),
      counter_(std::move(counter)),
      attested_data_(std::move(data)) {
  // TODO(kpaulhamus): use std::array for these small, fixed-sized vectors.
  CHECK_EQ(counter_.size(), 4u);
}

AuthenticatorData::AuthenticatorData(AuthenticatorData&& other) = default;
AuthenticatorData& AuthenticatorData::operator=(AuthenticatorData&& other) =
    default;

AuthenticatorData::~AuthenticatorData() = default;

void AuthenticatorData::DeleteDeviceAaguid() {
  if (!attested_data_)
    return;

  attested_data_->DeleteAaguid();
}

std::vector<uint8_t> AuthenticatorData::SerializeToByteArray() const {
  std::vector<uint8_t> authenticator_data;
  fido_parsing_utils::Append(&authenticator_data, application_parameter_);
  authenticator_data.insert(authenticator_data.end(), flags_);
  fido_parsing_utils::Append(&authenticator_data, counter_);
  if (attested_data_) {
    // Attestations are returned in registration responses but not in assertion
    // responses.
    fido_parsing_utils::Append(&authenticator_data,
                               attested_data_->SerializeAsBytes());
  }
  return authenticator_data;
}

std::vector<uint8_t> AuthenticatorData::GetCredentialId() const {
  if (!attested_data_)
    return std::vector<uint8_t>();

  return attested_data_->credential_id();
}

}  // namespace device
