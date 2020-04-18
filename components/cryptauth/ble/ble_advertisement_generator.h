// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_BLE_ADVERTISEMENT_GENERATOR_H_
#define COMPONENTS_CRYPTAUTH_BLE_ADVERTISEMENT_GENERATOR_H_

#include <memory>

#include "base/macros.h"
#include "components/cryptauth/foreground_eid_generator.h"

namespace chromeos {
namespace tether {
class BleAdvertiserImplTest;
class AdHocBleAdvertiserImplTest;
}  // namespace tether
}  // namespace chromeos

namespace cryptauth {

class LocalDeviceDataProvider;
class RemoteBeaconSeedFetcher;

// Generates advertisements for the ProximityAuth BLE advertisement scheme.
class BleAdvertisementGenerator {
 public:
  // Generates an advertisement from the current device to the device with ID
  // |device_id|. The generated advertisement should be used immediately since
  // it is based on the current timestamp.
  static std::unique_ptr<DataWithTimestamp> GenerateBleAdvertisement(
      const std::string& device_id,
      LocalDeviceDataProvider* local_device_data_provider,
      RemoteBeaconSeedFetcher* remote_beacon_seed_fetcher);

  virtual ~BleAdvertisementGenerator();

 protected:
  BleAdvertisementGenerator();

  virtual std::unique_ptr<DataWithTimestamp> GenerateBleAdvertisementInternal(
      const std::string& device_id,
      LocalDeviceDataProvider* local_device_data_provider,
      RemoteBeaconSeedFetcher* remote_beacon_seed_fetcher);

 private:
  friend class CryptAuthBleAdvertisementGeneratorTest;
  friend class chromeos::tether::BleAdvertiserImplTest;
  friend class chromeos::tether::AdHocBleAdvertiserImplTest;

  static BleAdvertisementGenerator* instance_;

  static void SetInstanceForTesting(BleAdvertisementGenerator* test_generator);

  void SetEidGeneratorForTesting(
      std::unique_ptr<ForegroundEidGenerator> test_eid_generator);

  std::unique_ptr<ForegroundEidGenerator> eid_generator_;

  DISALLOW_COPY_AND_ASSIGN(BleAdvertisementGenerator);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_BLE_ADVERTISEMENT_GENERATOR_H_
