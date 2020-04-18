// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_LOCAL_DEVICE_DATA_PROVIDER_H_
#define COMPONENTS_CRYPTAUTH_LOCAL_DEVICE_DATA_PROVIDER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"

namespace cryptauth {

class BeaconSeed;
class CryptAuthService;

// Fetches CryptAuth data about the local device (i.e., the device on which this
// code is running) for the current user (i.e., the one which is logged-in).
class LocalDeviceDataProvider {
 public:
  explicit LocalDeviceDataProvider(CryptAuthService* cryptauth_service);
  virtual ~LocalDeviceDataProvider();

  // Fetches the public key and/or the beacon seeds for the local device.
  // Returns whether the operation succeeded. If |nullptr| is passed as a
  // parameter, the associated data will not be fetched.
  virtual bool GetLocalDeviceData(
      std::string* public_key_out,
      std::vector<BeaconSeed>* beacon_seeds_out) const;

 private:
  friend class LocalDeviceDataProviderTest;

  CryptAuthService* cryptauth_service_;

  DISALLOW_COPY_AND_ASSIGN(LocalDeviceDataProvider);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_LOCAL_DEVICE_DATA_PROVIDER_H_
