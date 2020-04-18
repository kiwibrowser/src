// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_FAKE_AD_HOC_BLE_ADVERTISER_H_
#define CHROMEOS_COMPONENTS_TETHER_FAKE_AD_HOC_BLE_ADVERTISER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "chromeos/components/tether/ad_hoc_ble_advertiser.h"

namespace chromeos {

namespace tether {

// Test doublue for AdHocBleAdvertiser.
class FakeAdHocBleAdvertiser : public AdHocBleAdvertiser {
 public:
  FakeAdHocBleAdvertiser();
  ~FakeAdHocBleAdvertiser() override;

  void set_has_pending_requests(bool has_pending_requests) {
    has_pending_requests_ = has_pending_requests;
  }

  const std::vector<std::string>& requested_device_ids() {
    return requested_device_ids_;
  }

  void NotifyAsynchronousShutdownComplete();

  // AdHocBleAdvertiser:
  void RequestGattServicesForDevice(const std::string& device_id) override;
  bool HasPendingRequests() override;

 private:
  bool has_pending_requests_ = false;
  std::vector<std::string> requested_device_ids_;

  DISALLOW_COPY_AND_ASSIGN(FakeAdHocBleAdvertiser);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_FAKE_AD_HOC_BLE_ADVERTISER_H_
