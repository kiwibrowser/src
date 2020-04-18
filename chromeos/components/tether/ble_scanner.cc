// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/ble_scanner.h"

namespace chromeos {

namespace tether {

BleScanner::BleScanner() = default;

BleScanner::~BleScanner() = default;

void BleScanner::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void BleScanner::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void BleScanner::NotifyReceivedAdvertisementFromDevice(
    cryptauth::RemoteDeviceRef remote_device,
    device::BluetoothDevice* bluetooth_device,
    bool is_background_advertisement) {
  for (auto& observer : observer_list_)
    observer.OnReceivedAdvertisementFromDevice(remote_device, bluetooth_device,
                                               is_background_advertisement);
}

void BleScanner::NotifyDiscoverySessionStateChanged(
    bool discovery_session_active) {
  for (auto& observer : observer_list_)
    observer.OnDiscoverySessionStateChanged(discovery_session_active);
}

}  // namespace tether

}  // namespace chromeos
