// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/ble_advertiser.h"

namespace chromeos {

namespace tether {

BleAdvertiser::BleAdvertiser() = default;

BleAdvertiser::~BleAdvertiser() = default;

void BleAdvertiser::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void BleAdvertiser::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void BleAdvertiser::NotifyAllAdvertisementsUnregistered() {
  for (auto& observer : observer_list_)
    observer.OnAllAdvertisementsUnregistered();
}

}  // namespace tether

}  // namespace chromeos
