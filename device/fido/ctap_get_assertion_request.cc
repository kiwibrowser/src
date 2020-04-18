// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/ctap_get_assertion_request.h"

#include <utility>

#include "base/numerics/safe_conversions.h"
#include "components/cbor/cbor_writer.h"
#include "device/fido/fido_constants.h"

namespace device {

CtapGetAssertionRequest::CtapGetAssertionRequest(
    std::string rp_id,
    std::vector<uint8_t> client_data_hash)
    : rp_id_(std::move(rp_id)),
      client_data_hash_(std::move(client_data_hash)) {}

CtapGetAssertionRequest::CtapGetAssertionRequest(
    const CtapGetAssertionRequest& that) = default;

CtapGetAssertionRequest::CtapGetAssertionRequest(
    CtapGetAssertionRequest&& that) = default;

CtapGetAssertionRequest& CtapGetAssertionRequest::operator=(
    const CtapGetAssertionRequest& other) = default;

CtapGetAssertionRequest& CtapGetAssertionRequest::operator=(
    CtapGetAssertionRequest&& other) = default;

CtapGetAssertionRequest::~CtapGetAssertionRequest() = default;

std::vector<uint8_t> CtapGetAssertionRequest::EncodeAsCBOR() const {
  cbor::CBORValue::MapValue cbor_map;
  cbor_map[cbor::CBORValue(1)] = cbor::CBORValue(rp_id_);
  cbor_map[cbor::CBORValue(2)] = cbor::CBORValue(client_data_hash_);

  if (allow_list_) {
    cbor::CBORValue::ArrayValue allow_list_array;
    for (const auto& descriptor : *allow_list_) {
      allow_list_array.push_back(descriptor.ConvertToCBOR());
    }
    cbor_map[cbor::CBORValue(3)] = cbor::CBORValue(std::move(allow_list_array));
  }

  if (pin_auth_) {
    cbor_map[cbor::CBORValue(6)] = cbor::CBORValue(*pin_auth_);
  }

  if (pin_protocol_) {
    cbor_map[cbor::CBORValue(7)] = cbor::CBORValue(*pin_protocol_);
  }

  cbor::CBORValue::MapValue option_map;

  // User presence is required by default.
  if (!user_presence_required_) {
    option_map[cbor::CBORValue(kUserPresenceMapKey)] =
        cbor::CBORValue(user_presence_required_);
  }

  // User verification is not required by default.
  if (user_verification_ == UserVerificationRequirement::kRequired) {
    option_map[cbor::CBORValue(kUserVerificationMapKey)] =
        cbor::CBORValue(true);
  }

  if (!option_map.empty()) {
    cbor_map[cbor::CBORValue(5)] = cbor::CBORValue(std::move(option_map));
  }

  auto serialized_param =
      cbor::CBORWriter::Write(cbor::CBORValue(std::move(cbor_map)));
  DCHECK(serialized_param);

  std::vector<uint8_t> cbor_request({base::strict_cast<uint8_t>(
      CtapRequestCommand::kAuthenticatorGetAssertion)});
  cbor_request.insert(cbor_request.end(), serialized_param->begin(),
                      serialized_param->end());
  return cbor_request;
}

CtapGetAssertionRequest& CtapGetAssertionRequest::SetUserVerification(
    UserVerificationRequirement user_verification) {
  user_verification_ = user_verification;
  return *this;
}

CtapGetAssertionRequest& CtapGetAssertionRequest::SetUserPresenceRequired(
    bool user_presence_required) {
  user_presence_required_ = user_presence_required;
  return *this;
}

CtapGetAssertionRequest& CtapGetAssertionRequest::SetAllowList(
    std::vector<PublicKeyCredentialDescriptor> allow_list) {
  allow_list_ = std::move(allow_list);
  return *this;
}

CtapGetAssertionRequest& CtapGetAssertionRequest::SetPinAuth(
    std::vector<uint8_t> pin_auth) {
  pin_auth_ = std::move(pin_auth);
  return *this;
}

CtapGetAssertionRequest& CtapGetAssertionRequest::SetPinProtocol(
    uint8_t pin_protocol) {
  pin_protocol_ = pin_protocol;
  return *this;
}

CtapGetAssertionRequest& CtapGetAssertionRequest::SetCableExtension(
    std::vector<FidoCableDiscovery::CableDiscoveryData> cable_extension) {
  cable_extension_ = std::move(cable_extension);
  return *this;
}

}  // namespace device
