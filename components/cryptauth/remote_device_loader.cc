// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/remote_device_loader.h"

#include <algorithm>
#include <utility>

#include "base/base64url.h"
#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "components/cryptauth/remote_device.h"
#include "components/cryptauth/remote_device_ref.h"
#include "components/cryptauth/secure_message_delegate.h"

namespace {

std::map<cryptauth::SoftwareFeature, cryptauth::SoftwareFeatureState>
GetSoftwareFeatureToStateMap(const cryptauth::ExternalDeviceInfo& device) {
  std::map<cryptauth::SoftwareFeature, cryptauth::SoftwareFeatureState>
      software_feature_to_state_map;

  for (int i = 0; i < device.supported_software_features_size(); ++i) {
    software_feature_to_state_map[device.supported_software_features(i)] =
        cryptauth::SoftwareFeatureState::kSupported;
  }

  for (int i = 0; i < device.enabled_software_features_size(); ++i) {
    software_feature_to_state_map[device.enabled_software_features(i)] =
        cryptauth::SoftwareFeatureState::kEnabled;
  }

  return software_feature_to_state_map;
}

}  // namespace

namespace cryptauth {

// static
RemoteDeviceLoader::Factory* RemoteDeviceLoader::Factory::factory_instance_ =
    nullptr;

// static
std::unique_ptr<RemoteDeviceLoader> RemoteDeviceLoader::Factory::NewInstance(
    const std::vector<cryptauth::ExternalDeviceInfo>& device_info_list,
    const std::string& user_id,
    const std::string& user_private_key,
    std::unique_ptr<cryptauth::SecureMessageDelegate> secure_message_delegate) {
  if (!factory_instance_) {
    factory_instance_ = new Factory();
  }
  return factory_instance_->BuildInstance(device_info_list,
                                          user_id,
                                          user_private_key,
                                          std::move(secure_message_delegate));
}

// static
void RemoteDeviceLoader::Factory::SetInstanceForTesting(Factory* factory) {
  factory_instance_ = factory;
}

std::unique_ptr<RemoteDeviceLoader> RemoteDeviceLoader::Factory::BuildInstance(
    const std::vector<cryptauth::ExternalDeviceInfo>& device_info_list,
    const std::string& user_id,
    const std::string& user_private_key,
    std::unique_ptr<cryptauth::SecureMessageDelegate> secure_message_delegate) {
  return base::WrapUnique(
      new RemoteDeviceLoader(device_info_list,
                             user_id,
                             user_private_key,
                             std::move(secure_message_delegate)));
}

RemoteDeviceLoader::RemoteDeviceLoader(
    const std::vector<cryptauth::ExternalDeviceInfo>& device_info_list,
    const std::string& user_id,
    const std::string& user_private_key,
    std::unique_ptr<cryptauth::SecureMessageDelegate> secure_message_delegate)
    : should_load_beacon_seeds_(false),
      remaining_devices_(device_info_list),
      user_id_(user_id),
      user_private_key_(user_private_key),
      secure_message_delegate_(std::move(secure_message_delegate)),
      weak_ptr_factory_(this) {}

RemoteDeviceLoader::~RemoteDeviceLoader() {}

void RemoteDeviceLoader::Load(bool should_load_beacon_seeds,
                              const RemoteDeviceCallback& callback) {
  DCHECK(callback_.is_null());
  should_load_beacon_seeds_ = should_load_beacon_seeds;
  callback_ = callback;
  PA_LOG(INFO) << "Loading " << remaining_devices_.size()
               << " remote devices";

  if (remaining_devices_.empty()) {
    callback_.Run(remote_devices_);
    return;
  }

  std::vector<cryptauth::ExternalDeviceInfo> all_devices_to_convert =
      remaining_devices_;

  for (const auto& device : all_devices_to_convert) {
    secure_message_delegate_->DeriveKey(
        user_private_key_, device.public_key(),
        base::Bind(&RemoteDeviceLoader::OnPSKDerived,
                   weak_ptr_factory_.GetWeakPtr(), device));
  }
}

void RemoteDeviceLoader::OnPSKDerived(
    const cryptauth::ExternalDeviceInfo& device,
    const std::string& psk) {
  std::string public_key = device.public_key();
  auto iterator = std::find_if(
      remaining_devices_.begin(), remaining_devices_.end(),
      [&public_key](const cryptauth::ExternalDeviceInfo& device) {
        return device.public_key() == public_key;
      });

  DCHECK(iterator != remaining_devices_.end());
  remaining_devices_.erase(iterator);

  RemoteDevice remote_device(
      user_id_, device.friendly_device_name(), device.public_key(), psk,
      device.unlock_key(), device.mobile_hotspot_supported(),
      device.last_update_time_millis(), GetSoftwareFeatureToStateMap(device));

  if (should_load_beacon_seeds_) {
    std::vector<BeaconSeed> beacon_seeds;
    for (const BeaconSeed& beacon_seed : device.beacon_seeds()) {
      beacon_seeds.push_back(beacon_seed);
    }
    remote_device.LoadBeaconSeeds(beacon_seeds);
  }

  remote_devices_.push_back(remote_device);

  if (remaining_devices_.empty()) {
    PA_LOG(INFO) << "Derived keys for " << remote_devices_.size()
                 << " devices.";
    callback_.Run(remote_devices_);
  }
}

}  // namespace cryptauth
