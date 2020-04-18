// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_MOCK_BEACON_SEED_FETCHER_H_
#define COMPONENTS_CRYPTAUTH_MOCK_BEACON_SEED_FETCHER_H_

#include <map>
#include <vector>

#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_beacon_seed_fetcher.h"

namespace cryptauth {

class MockRemoteBeaconSeedFetcher : public RemoteBeaconSeedFetcher {
 public:
  MockRemoteBeaconSeedFetcher();
  ~MockRemoteBeaconSeedFetcher() override;

  // If |beacon_seeds| is null, FetchSeedsForDevice() will fail for the same
  // |remote_device|.
  void SetSeedsForDeviceId(const std::string& device_id,
                           const std::vector<BeaconSeed>* beacon_seeds);

  // RemoteBeaconSeedFetcher:
  bool FetchSeedsForDeviceId(
      const std::string& device_id,
      std::vector<BeaconSeed>* beacon_seeds_out) const override;

 private:
  std::map<std::string, std::vector<BeaconSeed>> device_id_to_beacon_seeds_map_;

  DISALLOW_COPY_AND_ASSIGN(MockRemoteBeaconSeedFetcher);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_MOCK_BEACON_SEED_FETCHER_H_
