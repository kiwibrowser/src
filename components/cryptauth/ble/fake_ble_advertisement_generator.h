// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_BLE_FAKE_ADVERTISEMENT_GENERATOR_H_
#define COMPONENTS_CRYPTAUTH_BLE_FAKE_ADVERTISEMENT_GENERATOR_H_

#include "base/macros.h"
#include "components/cryptauth/ble/ble_advertisement_generator.h"
#include "components/cryptauth/data_with_timestamp.h"

namespace cryptauth {

// Test double for BleAdvertisementGenerator.
class FakeBleAdvertisementGenerator : public BleAdvertisementGenerator {
 public:
  FakeBleAdvertisementGenerator();
  ~FakeBleAdvertisementGenerator() override;

  // Sets the advertisement to be returned by the next call to
  // GenerateBleAdvertisementInternal(). Note that, because |advertisement| is
  // a std::unique_ptr, set_advertisement() must be called each time an
  // advertisement is expected to be returned.
  void set_advertisement(std::unique_ptr<DataWithTimestamp> advertisement) {
    advertisement_ = std::move(advertisement);
  }

 protected:
  std::unique_ptr<DataWithTimestamp> GenerateBleAdvertisementInternal(
      const std::string& device_id,
      LocalDeviceDataProvider* local_device_data_provider,
      RemoteBeaconSeedFetcher* remote_beacon_seed_fetcher) override;

 private:
  std::unique_ptr<DataWithTimestamp> advertisement_;

  DISALLOW_COPY_AND_ASSIGN(FakeBleAdvertisementGenerator);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_BLE_FAKE_ADVERTISEMENT_GENERATOR_H_
