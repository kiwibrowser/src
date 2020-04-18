// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_MOCK_LOCAL_DEVICE_DATA_PROVIDER_H_
#define COMPONENTS_CRYPTAUTH_MOCK_LOCAL_DEVICE_DATA_PROVIDER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "components/cryptauth/local_device_data_provider.h"

namespace cryptauth {

class BeaconSeed;

// Test double for LocalDeviceDataProvider.
class MockLocalDeviceDataProvider : public LocalDeviceDataProvider {
 public:
  MockLocalDeviceDataProvider();
  ~MockLocalDeviceDataProvider() override;

  void SetPublicKey(std::unique_ptr<std::string> public_key);
  void SetBeaconSeeds(std::unique_ptr<std::vector<BeaconSeed>> beacon_seeds);

  // LocalDeviceDataProvider:
  bool GetLocalDeviceData(
      std::string* public_key_out,
      std::vector<BeaconSeed>* beacon_seeds_out) const override;

 private:
  std::unique_ptr<std::string> public_key_;
  std::unique_ptr<std::vector<BeaconSeed>> beacon_seeds_;

  DISALLOW_COPY_AND_ASSIGN(MockLocalDeviceDataProvider);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_MOCK_LOCAL_DEVICE_DATA_PROVIDER_H_
