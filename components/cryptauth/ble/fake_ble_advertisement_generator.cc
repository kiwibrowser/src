// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/ble/fake_ble_advertisement_generator.h"

namespace cryptauth {

FakeBleAdvertisementGenerator::FakeBleAdvertisementGenerator() {}

FakeBleAdvertisementGenerator::~FakeBleAdvertisementGenerator() {}

std::unique_ptr<DataWithTimestamp>
FakeBleAdvertisementGenerator::GenerateBleAdvertisementInternal(
    const std::string& device_id,
    LocalDeviceDataProvider* local_device_data_provider,
    RemoteBeaconSeedFetcher* remote_beacon_seed_fetcher) {
  return std::move(advertisement_);
}

}  // namespace cryptauth
