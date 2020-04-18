// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_CHROMEOS_BLUETOOTH_UTILS_H_
#define DEVICE_BLUETOOTH_CHROMEOS_BLUETOOTH_UTILS_H_

#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_export.h"

// This file contains common utilities for filtering the bluetooth devices
// based on the filter criteria.
namespace device {

enum class BluetoothFilterType {
  // No filtering, all bluetooth devices will be returned.
  ALL = 0,
  // Return bluetooth devices that are known to the UI.
  // I.e. bluetooth device type != UNKNOWN
  KNOWN,
};

// Return filtered devices based on the filter type and max number of devices.
device::BluetoothAdapter::DeviceList DEVICE_BLUETOOTH_EXPORT
FilterBluetoothDeviceList(const BluetoothAdapter::DeviceList& devices,
                          BluetoothFilterType filter_type,
                          int max_devices);

}  // namespace device

#endif  // DEVICE_BLUETOOTH_CHROMEOS_BLUETOOTH_UTILS_H_
