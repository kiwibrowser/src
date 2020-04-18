// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/mock_local_device_data_provider.h"

#include "components/cryptauth/cryptauth_device_manager.h"
#include "components/cryptauth/cryptauth_enrollment_manager.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"

namespace cryptauth {

MockLocalDeviceDataProvider::MockLocalDeviceDataProvider()
    : LocalDeviceDataProvider(nullptr /* cryptauth_service */) {}

MockLocalDeviceDataProvider::~MockLocalDeviceDataProvider() {}

void MockLocalDeviceDataProvider::SetPublicKey(
    std::unique_ptr<std::string> public_key) {
  if (public_key) {
    public_key_ = std::move(public_key);
  } else {
    public_key_.reset();
  }
}

void MockLocalDeviceDataProvider::SetBeaconSeeds(
    std::unique_ptr<std::vector<BeaconSeed>> beacon_seeds) {
  if (beacon_seeds) {
    beacon_seeds_ = std::move(beacon_seeds);
  } else {
    beacon_seeds_.reset();
  }
}

bool MockLocalDeviceDataProvider::GetLocalDeviceData(
    std::string* public_key_out,
    std::vector<BeaconSeed>* beacon_seeds_out) const {
  bool success = false;

  if (public_key_ && public_key_out) {
    *public_key_out = *public_key_;
    success = true;
  }

  if (beacon_seeds_ && beacon_seeds_out) {
    *beacon_seeds_out = *beacon_seeds_;
    success = true;
  }

  return success;
}

}  // namespace cryptauth
