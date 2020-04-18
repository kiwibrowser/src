// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/fake_ble_scanner.h"

#include <algorithm>

#include "base/stl_util.h"

namespace chromeos {

namespace tether {

FakeBleScanner::FakeBleScanner(bool automatically_update_discovery_session)
    : automatically_update_discovery_session_(
          automatically_update_discovery_session) {}

FakeBleScanner::~FakeBleScanner() = default;

void FakeBleScanner::NotifyReceivedAdvertisementFromDevice(
    cryptauth::RemoteDeviceRef remote_device,
    device::BluetoothDevice* bluetooth_device,
    bool is_background_advertisement) {
  BleScanner::NotifyReceivedAdvertisementFromDevice(
      remote_device, bluetooth_device, is_background_advertisement);
}

void FakeBleScanner::NotifyDiscoverySessionStateChanged(
    bool discovery_session_active) {
  BleScanner::NotifyDiscoverySessionStateChanged(discovery_session_active);
}

bool FakeBleScanner::RegisterScanFilterForDevice(const std::string& device_id) {
  if (should_fail_to_register_)
    return false;

  if (std::find(registered_device_ids_.begin(), registered_device_ids_.end(),
                device_id) != registered_device_ids_.end()) {
    return false;
  }

  bool was_empty = registered_device_ids_.empty();
  registered_device_ids_.push_back(device_id);

  if (was_empty && automatically_update_discovery_session_) {
    is_discovery_session_active_ = true;
    NotifyDiscoverySessionStateChanged(true);
  }

  return true;
}

bool FakeBleScanner::UnregisterScanFilterForDevice(
    const std::string& device_id) {
  if (std::find(registered_device_ids_.begin(), registered_device_ids_.end(),
                device_id) == registered_device_ids_.end()) {
    return false;
  }

  base::Erase(registered_device_ids_, device_id);

  if (automatically_update_discovery_session_ &&
      registered_device_ids_.empty()) {
    is_discovery_session_active_ = false;
    NotifyDiscoverySessionStateChanged(false);
  }

  return true;
}

bool FakeBleScanner::ShouldDiscoverySessionBeActive() {
  return !registered_device_ids_.empty();
}

bool FakeBleScanner::IsDiscoverySessionActive() {
  return is_discovery_session_active_;
}

}  // namespace tether

}  // namespace chromeos
