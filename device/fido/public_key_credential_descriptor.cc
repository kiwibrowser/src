// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "device/fido/public_key_credential_descriptor.h"

namespace device {

namespace {

// Keys for storing credential descriptor information in CBOR map.
constexpr char kCredentialIdKey[] = "id";
constexpr char kCredentialTypeKey[] = "type";

}  // namespace

// static
base::Optional<PublicKeyCredentialDescriptor>
PublicKeyCredentialDescriptor::CreateFromCBORValue(
    const cbor::CBORValue& cbor) {
  if (!cbor.is_map()) {
    return base::nullopt;
  }

  const cbor::CBORValue::MapValue& map = cbor.GetMap();
  auto type = map.find(cbor::CBORValue(kCredentialTypeKey));
  if (type == map.end() || !type->second.is_string() ||
      type->second.GetString() != kPublicKey)
    return base::nullopt;

  auto id = map.find(cbor::CBORValue(kCredentialIdKey));
  if (id == map.end() || !id->second.is_bytestring())
    return base::nullopt;

  return PublicKeyCredentialDescriptor(CredentialType::kPublicKey,
                                       id->second.GetBytestring());
}

PublicKeyCredentialDescriptor::PublicKeyCredentialDescriptor(
    CredentialType credential_type,
    std::vector<uint8_t> id)
    : credential_type_(credential_type), id_(std::move(id)) {}

PublicKeyCredentialDescriptor::PublicKeyCredentialDescriptor(
    const PublicKeyCredentialDescriptor& other) = default;

PublicKeyCredentialDescriptor::PublicKeyCredentialDescriptor(
    PublicKeyCredentialDescriptor&& other) = default;

PublicKeyCredentialDescriptor& PublicKeyCredentialDescriptor::operator=(
    const PublicKeyCredentialDescriptor& other) = default;

PublicKeyCredentialDescriptor& PublicKeyCredentialDescriptor::operator=(
    PublicKeyCredentialDescriptor&& other) = default;

PublicKeyCredentialDescriptor::~PublicKeyCredentialDescriptor() = default;

cbor::CBORValue PublicKeyCredentialDescriptor::ConvertToCBOR() const {
  cbor::CBORValue::MapValue cbor_descriptor_map;
  cbor_descriptor_map[cbor::CBORValue(kCredentialIdKey)] = cbor::CBORValue(id_);
  cbor_descriptor_map[cbor::CBORValue(kCredentialTypeKey)] =
      cbor::CBORValue(CredentialTypeToString(credential_type_));
  return cbor::CBORValue(std::move(cbor_descriptor_map));
}

}  // namespace device
