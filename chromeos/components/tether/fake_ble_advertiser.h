// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_FAKE_BLE_ADVERTISER_H_
#define CHROMEOS_COMPONENTS_TETHER_FAKE_BLE_ADVERTISER_H_

#include <vector>

#include "base/macros.h"
#include "chromeos/components/tether/ble_advertiser.h"
#include "components/cryptauth/remote_device_ref.h"

namespace chromeos {

namespace tether {

// Test double for BleAdvertiser.
class FakeBleAdvertiser : public BleAdvertiser {
 public:
  // If |automatically_update_active_advertisements| is true,
  // AreAdvertisementsRegistered() will simply return whether at least one
  // device should be advertising; otherwise, that value must be determined
  // manually via set_is_discovery_session_active().
  FakeBleAdvertiser(bool automatically_update_active_advertisements);
  ~FakeBleAdvertiser() override;

  const std::vector<std::string>& registered_device_ids() {
    return registered_device_ids_;
  }

  void set_should_fail_to_start_advertising(
      bool should_fail_to_start_advertising) {
    should_fail_to_start_advertising_ = should_fail_to_start_advertising;
  }

  void set_are_advertisements_registered(bool are_advertisements_registered) {
    are_advertisements_registered_ = are_advertisements_registered;
  }

  void NotifyAllAdvertisementsUnregistered();

  // BleAdvertiser:
  bool StartAdvertisingToDevice(const std::string& device_id) override;
  bool StopAdvertisingToDevice(const std::string& device_id) override;
  bool AreAdvertisementsRegistered() override;

 private:
  const bool automatically_update_active_advertisements_;

  bool should_fail_to_start_advertising_ = false;
  bool are_advertisements_registered_ = false;
  std::vector<std::string> registered_device_ids_;

  DISALLOW_COPY_AND_ASSIGN(FakeBleAdvertiser);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_FAKE_BLE_ADVERTISER_H_
