// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_FAKE_BACKGROUND_EID_GENERATOR_H_
#define COMPONENTS_CRYPTAUTH_FAKE_BACKGROUND_EID_GENERATOR_H_

#include <memory>
#include <string>
#include <vector>

#include "components/cryptauth/background_eid_generator.h"

namespace cryptauth {

class BeaconSeed;

// Test double class for BackgroundEidGenerator.
class FakeBackgroundEidGenerator : public BackgroundEidGenerator {
 public:
  FakeBackgroundEidGenerator();
  ~FakeBackgroundEidGenerator() override;

  // BackgroundEidGenerator:
  std::vector<DataWithTimestamp> GenerateNearestEids(
      const std::vector<BeaconSeed>& beacon_seed) const override;
  std::string IdentifyRemoteDeviceByAdvertisement(
      cryptauth::RemoteBeaconSeedFetcher* remote_beacon_seed_fetcher,
      const std::string& advertisement_service_data,
      const std::vector<std::string>& device_ids) const override;

  void set_nearest_eids_(
      std::unique_ptr<std::vector<DataWithTimestamp>> nearest_eids) {
    nearest_eids_ = std::move(nearest_eids);
  }

  void set_identified_device_id(const std::string& identified_device_id) {
    identified_device_id_ = identified_device_id;
  }

  int num_identify_calls() { return num_identify_calls_; }

 private:
  std::unique_ptr<std::vector<DataWithTimestamp>> nearest_eids_;
  std::string identified_device_id_;

  int num_identify_calls_ = 0;
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_FAKE_BACKGROUND_EID_GENERATOR_H_
