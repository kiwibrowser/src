// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/mock_remote_beacon_seed_fetcher.h"

#include "components/cryptauth/cryptauth_device_manager.h"

namespace cryptauth {

MockRemoteBeaconSeedFetcher::MockRemoteBeaconSeedFetcher()
    : RemoteBeaconSeedFetcher(nullptr) {}

MockRemoteBeaconSeedFetcher::~MockRemoteBeaconSeedFetcher() {}

bool MockRemoteBeaconSeedFetcher::FetchSeedsForDeviceId(
    const std::string& device_id,
    std::vector<BeaconSeed>* beacon_seeds_out) const {
  const auto& seeds_iter = device_id_to_beacon_seeds_map_.find(device_id);

  if (seeds_iter == device_id_to_beacon_seeds_map_.end())
    return false;

  *beacon_seeds_out = seeds_iter->second;
  return true;
}

void MockRemoteBeaconSeedFetcher::SetSeedsForDeviceId(
    const std::string& device_id,
    const std::vector<BeaconSeed>* beacon_seeds) {
  if (!beacon_seeds) {
    const auto& it = device_id_to_beacon_seeds_map_.find(device_id);

    if (it != device_id_to_beacon_seeds_map_.end())
      device_id_to_beacon_seeds_map_.erase(it);

    return;
  }

  device_id_to_beacon_seeds_map_[device_id] = *beacon_seeds;
}

}  // namespace cryptauth
