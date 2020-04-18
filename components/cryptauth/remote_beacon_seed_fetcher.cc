// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/remote_beacon_seed_fetcher.h"

#include "components/cryptauth/cryptauth_device_manager.h"
#include "components/cryptauth/remote_device_ref.h"

namespace cryptauth {

RemoteBeaconSeedFetcher::RemoteBeaconSeedFetcher(
    const CryptAuthDeviceManager* device_manager)
    : device_manager_(device_manager) {}

RemoteBeaconSeedFetcher::~RemoteBeaconSeedFetcher() {}

bool RemoteBeaconSeedFetcher::FetchSeedsForDeviceId(
    const std::string& device_id,
    std::vector<BeaconSeed>* beacon_seeds_out) const {
  for(const auto& device_info : device_manager_->GetSyncedDevices()) {
    if (RemoteDeviceRef::GenerateDeviceId(device_info.public_key()) ==
        device_id) {
      if (device_info.beacon_seeds_size() == 0) {
        return false;
      }

      beacon_seeds_out->clear();
      for (int i = 0; i < device_info.beacon_seeds_size(); i++) {
        beacon_seeds_out->push_back(device_info.beacon_seeds(i));
      }
      return true;
    }
  }

  return false;
}

}  // namespace cryptauth
