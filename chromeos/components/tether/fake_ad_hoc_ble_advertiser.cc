// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/fake_ad_hoc_ble_advertiser.h"

namespace chromeos {

namespace tether {

FakeAdHocBleAdvertiser::FakeAdHocBleAdvertiser() {}

FakeAdHocBleAdvertiser::~FakeAdHocBleAdvertiser() {}

void FakeAdHocBleAdvertiser::NotifyAsynchronousShutdownComplete() {
  AdHocBleAdvertiser::NotifyAsynchronousShutdownComplete();
}

void FakeAdHocBleAdvertiser::RequestGattServicesForDevice(
    const std::string& device_id) {
  requested_device_ids_.push_back(device_id);
}

bool FakeAdHocBleAdvertiser::HasPendingRequests() {
  return has_pending_requests_;
}

}  // namespace tether

}  // namespace chromeos
