// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/remote_device_ref.h"

#include "base/base64.h"
#include "base/stl_util.h"

namespace cryptauth {

// static
std::string RemoteDeviceRef::GenerateDeviceId(const std::string& public_key) {
  return RemoteDevice::GenerateDeviceId(public_key);
}

// static
std::string RemoteDeviceRef::DerivePublicKey(const std::string& device_id) {
  std::string public_key;
  if (base::Base64Decode(device_id, &public_key))
    return public_key;
  return std::string();
}

// static
std::string RemoteDeviceRef::TruncateDeviceIdForLogs(
    const std::string& full_id) {
  if (full_id.length() <= 10) {
    return full_id;
  }

  return full_id.substr(0, 5) + "..." +
         full_id.substr(full_id.length() - 5, full_id.length());
}

RemoteDeviceRef::RemoteDeviceRef(std::shared_ptr<RemoteDevice> remote_device)
    : remote_device_(std::move(remote_device)) {}

RemoteDeviceRef::RemoteDeviceRef(const RemoteDeviceRef& other) = default;

RemoteDeviceRef::~RemoteDeviceRef() = default;

SoftwareFeatureState RemoteDeviceRef::GetSoftwareFeatureState(
    const SoftwareFeature& software_feature) const {
  if (!base::ContainsKey(remote_device_->software_features, software_feature))
    return SoftwareFeatureState::kNotSupported;

  return remote_device_->software_features.at(software_feature);
}

std::string RemoteDeviceRef::GetDeviceId() const {
  return remote_device_->GetDeviceId();
}

std::string RemoteDeviceRef::GetTruncatedDeviceIdForLogs() const {
  return RemoteDeviceRef::TruncateDeviceIdForLogs(GetDeviceId());
}

bool RemoteDeviceRef::operator==(const RemoteDeviceRef& other) const {
  return *remote_device_ == *other.remote_device_;
}

bool RemoteDeviceRef::operator<(const RemoteDeviceRef& other) const {
  return *remote_device_ < *other.remote_device_;
}

}  // namespace cryptauth