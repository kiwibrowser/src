// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/public_key_credential_params.h"

#include <utility>

namespace device {

PublicKeyCredentialParams::PublicKeyCredentialParams(
    std::vector<CredentialInfo> credential_params)
    : public_key_credential_params_(std::move(credential_params)) {}

PublicKeyCredentialParams::PublicKeyCredentialParams(
    const PublicKeyCredentialParams& other) = default;

PublicKeyCredentialParams::PublicKeyCredentialParams(
    PublicKeyCredentialParams&& other) = default;

PublicKeyCredentialParams& PublicKeyCredentialParams::operator=(
    const PublicKeyCredentialParams& other) = default;

PublicKeyCredentialParams& PublicKeyCredentialParams::operator=(
    PublicKeyCredentialParams&& other) = default;

PublicKeyCredentialParams::~PublicKeyCredentialParams() = default;

cbor::CBORValue PublicKeyCredentialParams::ConvertToCBOR() const {
  cbor::CBORValue::ArrayValue credential_param_array;
  credential_param_array.reserve(public_key_credential_params_.size());

  for (const auto& credential : public_key_credential_params_) {
    cbor::CBORValue::MapValue cbor_credential_map;
    cbor_credential_map[cbor::CBORValue("type")] =
        cbor::CBORValue(CredentialTypeToString(credential.type));
    cbor_credential_map[cbor::CBORValue("alg")] =
        cbor::CBORValue(credential.algorithm);
    credential_param_array.emplace_back(std::move(cbor_credential_map));
  }
  return cbor::CBORValue(std::move(credential_param_array));
}

}  // namespace device
