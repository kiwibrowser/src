// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/ad_hoc_ble_advertiser.h"

namespace chromeos {

namespace tether {

AdHocBleAdvertiser::AdHocBleAdvertiser() {}

AdHocBleAdvertiser::~AdHocBleAdvertiser() {}

void AdHocBleAdvertiser::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void AdHocBleAdvertiser::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void AdHocBleAdvertiser::NotifyAsynchronousShutdownComplete() {
  for (auto& observer : observer_list_)
    observer.OnAsynchronousShutdownComplete();
}

}  // namespace tether

}  // namespace chromeos
