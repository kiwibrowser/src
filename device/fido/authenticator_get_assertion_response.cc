// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/authenticator_get_assertion_response.h"

#include <utility>

#include "base/optional.h"
#include "device/fido/authenticator_data.h"
#include "device/fido/fido_parsing_utils.h"

namespace device {

namespace {

constexpr size_t kFlagIndex = 0;
constexpr size_t kFlagLength = 1;
constexpr size_t kCounterIndex = 1;
constexpr size_t kCounterLength = 4;
constexpr size_t kSignatureIndex = 5;

}  // namespace

// static
base::Optional<AuthenticatorGetAssertionResponse>
AuthenticatorGetAssertionResponse::CreateFromU2fSignResponse(
    const std::vector<uint8_t>& relying_party_id_hash,
    const std::vector<uint8_t>& u2f_data,
    const std::vector<uint8_t>& key_handle) {
  if (key_handle.empty())
    return base::nullopt;

  auto flags = fido_parsing_utils::Extract(u2f_data, kFlagIndex, kFlagLength);
  if (flags.empty())
    return base::nullopt;

  auto counter =
      fido_parsing_utils::Extract(u2f_data, kCounterIndex, kCounterLength);
  if (counter.empty())
    return base::nullopt;

  AuthenticatorData authenticator_data(relying_party_id_hash, flags[0],
                                       std::move(counter), base::nullopt);

  auto signature = fido_parsing_utils::Extract(
      u2f_data, kSignatureIndex, u2f_data.size() - kSignatureIndex);
  if (signature.empty())
    return base::nullopt;

  AuthenticatorGetAssertionResponse response(std::move(authenticator_data),
                                             std::move(signature));
  response.SetCredential(
      PublicKeyCredentialDescriptor(CredentialType::kPublicKey, key_handle));

  return std::move(response);
}

AuthenticatorGetAssertionResponse::AuthenticatorGetAssertionResponse(
    AuthenticatorData authenticator_data,
    std::vector<uint8_t> signature)
    : authenticator_data_(std::move(authenticator_data)),
      signature_(std::move(signature)) {}

AuthenticatorGetAssertionResponse::AuthenticatorGetAssertionResponse(
    AuthenticatorGetAssertionResponse&& that) = default;

AuthenticatorGetAssertionResponse& AuthenticatorGetAssertionResponse::operator=(
    AuthenticatorGetAssertionResponse&& other) = default;

AuthenticatorGetAssertionResponse::~AuthenticatorGetAssertionResponse() =
    default;

const std::vector<uint8_t>& AuthenticatorGetAssertionResponse::GetRpIdHash()
    const {
  return authenticator_data_.application_parameter();
}

AuthenticatorGetAssertionResponse&
AuthenticatorGetAssertionResponse::SetCredential(
    PublicKeyCredentialDescriptor credential) {
  credential_ = std::move(credential);
  raw_credential_id_ = credential_->id();
  return *this;
}

AuthenticatorGetAssertionResponse&
AuthenticatorGetAssertionResponse::SetUserEntity(
    PublicKeyCredentialUserEntity user_entity) {
  user_entity_ = std::move(user_entity);
  return *this;
}

AuthenticatorGetAssertionResponse&
AuthenticatorGetAssertionResponse::SetNumCredentials(uint8_t num_credentials) {
  num_credentials_ = num_credentials;
  return *this;
}

}  // namespace device
