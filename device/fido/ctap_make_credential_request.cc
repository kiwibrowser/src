// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/ctap_make_credential_request.h"

#include <utility>

#include "base/numerics/safe_conversions.h"
#include "components/cbor/cbor_writer.h"
#include "device/fido/fido_constants.h"

namespace device {

CtapMakeCredentialRequest::CtapMakeCredentialRequest(
    std::vector<uint8_t> client_data_hash,
    PublicKeyCredentialRpEntity rp,
    PublicKeyCredentialUserEntity user,
    PublicKeyCredentialParams public_key_credential_params)
    : client_data_hash_(std::move(client_data_hash)),
      rp_(std::move(rp)),
      user_(std::move(user)),
      public_key_credential_params_(std::move(public_key_credential_params)) {}

CtapMakeCredentialRequest::CtapMakeCredentialRequest(
    const CtapMakeCredentialRequest& that) = default;

CtapMakeCredentialRequest::CtapMakeCredentialRequest(
    CtapMakeCredentialRequest&& that) = default;

CtapMakeCredentialRequest& CtapMakeCredentialRequest::operator=(
    const CtapMakeCredentialRequest& that) = default;

CtapMakeCredentialRequest& CtapMakeCredentialRequest::operator=(
    CtapMakeCredentialRequest&& that) = default;

CtapMakeCredentialRequest::~CtapMakeCredentialRequest() = default;

std::vector<uint8_t> CtapMakeCredentialRequest::EncodeAsCBOR() const {
  cbor::CBORValue::MapValue cbor_map;
  cbor_map[cbor::CBORValue(1)] = cbor::CBORValue(client_data_hash_);
  cbor_map[cbor::CBORValue(2)] = rp_.ConvertToCBOR();
  cbor_map[cbor::CBORValue(3)] = user_.ConvertToCBOR();
  cbor_map[cbor::CBORValue(4)] = public_key_credential_params_.ConvertToCBOR();
  if (exclude_list_) {
    cbor::CBORValue::ArrayValue exclude_list_array;
    for (const auto& descriptor : *exclude_list_) {
      exclude_list_array.push_back(descriptor.ConvertToCBOR());
    }
    cbor_map[cbor::CBORValue(5)] =
        cbor::CBORValue(std::move(exclude_list_array));
  }
  if (pin_auth_) {
    cbor_map[cbor::CBORValue(8)] = cbor::CBORValue(*pin_auth_);
  }

  if (pin_protocol_) {
    cbor_map[cbor::CBORValue(9)] = cbor::CBORValue(*pin_protocol_);
  }

  cbor::CBORValue::MapValue option_map;

  // Resident keys are not supported by default.
  if (resident_key_supported_) {
    option_map[cbor::CBORValue(kResidentKeyMapKey)] =
        cbor::CBORValue(resident_key_supported_);
  }

  // User verification is not required by default.
  if (user_verification_required_) {
    option_map[cbor::CBORValue(kUserVerificationMapKey)] =
        cbor::CBORValue(user_verification_required_);
  }

  if (!option_map.empty()) {
    cbor_map[cbor::CBORValue(7)] = cbor::CBORValue(std::move(option_map));
  }

  auto serialized_param =
      cbor::CBORWriter::Write(cbor::CBORValue(std::move(cbor_map)));
  DCHECK(serialized_param);

  std::vector<uint8_t> cbor_request({base::strict_cast<uint8_t>(
      CtapRequestCommand::kAuthenticatorMakeCredential)});
  cbor_request.insert(cbor_request.end(), serialized_param->begin(),
                      serialized_param->end());
  return cbor_request;
}

CtapMakeCredentialRequest&
CtapMakeCredentialRequest::SetUserVerificationRequired(
    bool user_verification_required) {
  user_verification_required_ = user_verification_required;
  return *this;
}

CtapMakeCredentialRequest& CtapMakeCredentialRequest::SetResidentKeySupported(
    bool resident_key_supported) {
  resident_key_supported_ = resident_key_supported;
  return *this;
}

CtapMakeCredentialRequest& CtapMakeCredentialRequest::SetExcludeList(
    std::vector<PublicKeyCredentialDescriptor> exclude_list) {
  exclude_list_ = std::move(exclude_list);
  return *this;
}

CtapMakeCredentialRequest& CtapMakeCredentialRequest::SetPinAuth(
    std::vector<uint8_t> pin_auth) {
  pin_auth_ = std::move(pin_auth);
  return *this;
}

CtapMakeCredentialRequest& CtapMakeCredentialRequest::SetPinProtocol(
    uint8_t pin_protocol) {
  pin_protocol_ = pin_protocol;
  return *this;
}

}  // namespace device
