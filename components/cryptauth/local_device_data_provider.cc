// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/local_device_data_provider.h"

#include "components/cryptauth/cryptauth_device_manager.h"
#include "components/cryptauth/cryptauth_enrollment_manager.h"
#include "components/cryptauth/cryptauth_service.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"

namespace cryptauth {

LocalDeviceDataProvider::LocalDeviceDataProvider(
    CryptAuthService* cryptauth_service)
    : cryptauth_service_(cryptauth_service) {}

LocalDeviceDataProvider::~LocalDeviceDataProvider() {}

bool LocalDeviceDataProvider::GetLocalDeviceData(
    std::string* public_key_out,
    std::vector<BeaconSeed>* beacon_seeds_out) const {
  DCHECK(public_key_out || beacon_seeds_out);

  std::string public_key =
      cryptauth_service_->GetCryptAuthEnrollmentManager()->GetUserPublicKey();
  if (public_key.empty()) {
    return false;
  }

  std::vector<ExternalDeviceInfo> synced_devices =
      cryptauth_service_->GetCryptAuthDeviceManager()->GetSyncedDevices();
  for (const auto& device : synced_devices) {
    if (device.has_public_key() && device.public_key() == public_key &&
        device.beacon_seeds_size() > 0) {
      if (public_key_out) {
        public_key_out->assign(public_key);
      }

      if (beacon_seeds_out) {
        beacon_seeds_out->clear();
        for (int i = 0; i < device.beacon_seeds_size(); i++) {
          beacon_seeds_out->push_back(device.beacon_seeds(i));
        }
      }

      return true;
    }
  }

  return false;
}

}  // namespace cryptauth
