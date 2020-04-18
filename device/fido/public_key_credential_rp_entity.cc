// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/public_key_credential_rp_entity.h"

#include <utility>

namespace device {

PublicKeyCredentialRpEntity::PublicKeyCredentialRpEntity(std::string rp_id)
    : rp_id_(std::move(rp_id)) {}

PublicKeyCredentialRpEntity::PublicKeyCredentialRpEntity(
    const PublicKeyCredentialRpEntity& other) = default;

PublicKeyCredentialRpEntity::PublicKeyCredentialRpEntity(
    PublicKeyCredentialRpEntity&& other) = default;

PublicKeyCredentialRpEntity& PublicKeyCredentialRpEntity::operator=(
    const PublicKeyCredentialRpEntity& other) = default;

PublicKeyCredentialRpEntity& PublicKeyCredentialRpEntity::operator=(
    PublicKeyCredentialRpEntity&& other) = default;

PublicKeyCredentialRpEntity::~PublicKeyCredentialRpEntity() = default;

PublicKeyCredentialRpEntity& PublicKeyCredentialRpEntity::SetRpName(
    std::string rp_name) {
  rp_name_ = std::move(rp_name);
  return *this;
}

PublicKeyCredentialRpEntity& PublicKeyCredentialRpEntity::SetRpIconUrl(
    GURL icon_url) {
  rp_icon_url_ = std::move(icon_url);
  return *this;
}

cbor::CBORValue PublicKeyCredentialRpEntity::ConvertToCBOR() const {
  cbor::CBORValue::MapValue rp_map;
  rp_map[cbor::CBORValue("id")] = cbor::CBORValue(rp_id_);
  if (rp_name_)
    rp_map[cbor::CBORValue("name")] = cbor::CBORValue(*rp_name_);
  if (rp_icon_url_)
    rp_map[cbor::CBORValue("icon")] = cbor::CBORValue(rp_icon_url_->spec());
  return cbor::CBORValue(std::move(rp_map));
}

}  // namespace device
