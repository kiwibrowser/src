// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/chromeos/bluetooth_utils.h"

namespace device {

namespace {

// https://www.bluetooth.com/specifications/gatt/services.
const char kHIDServiceUUID[] = "1812";

// https://www.bluetooth.com/specifications/assigned-numbers/16-bit-uuids-for-sdos.
const char kSecurityKeyServiceUUID[] = "FFFD";

// Get limited number of devices from |devices| and
// prioritize paired/connecting devices over other devices.
BluetoothAdapter::DeviceList GetLimitedNumDevices(
    size_t max_device_num,
    const BluetoothAdapter::DeviceList& devices) {
  // If |max_device_num| is 0, it means there's no limit.
  if (max_device_num == 0)
    return devices;

  BluetoothAdapter::DeviceList result;
  for (BluetoothDevice* device : devices) {
    if (result.size() == max_device_num)
      break;

    if (device->IsPaired() || device->IsConnecting())
      result.push_back(device);
  }

  for (BluetoothDevice* device : devices) {
    if (result.size() == max_device_num)
      break;

    if (!device->IsPaired() && !device->IsConnecting())
      result.push_back(device);
  }

  return result;
}

// Filter out unknown devices from the list.
BluetoothAdapter::DeviceList FilterUnknownDevices(
    const BluetoothAdapter::DeviceList& devices) {
  BluetoothAdapter::DeviceList result;
  for (BluetoothDevice* device : devices) {
    switch (device->GetType()) {
      // Device with invalid bluetooth transport is filtered out.
      case BLUETOOTH_TRANSPORT_INVALID:
        break;
      // For LE devices, check the service UUID to determine if it supports HID
      // or second factor authenticator (security key).
      case BLUETOOTH_TRANSPORT_LE:
        if (base::ContainsKey(device->GetUUIDs(),
                              device::BluetoothUUID(kHIDServiceUUID)) ||
            base::ContainsKey(device->GetUUIDs(),
                              device::BluetoothUUID(kSecurityKeyServiceUUID))) {
          result.push_back(device);
        }
        break;
      // For classic and dual mode devices, only filter out if the name is empty
      // because the device could have an unknown or even known type and still
      // also provide audio/HID functionality.
      case BLUETOOTH_TRANSPORT_CLASSIC:
      case BLUETOOTH_TRANSPORT_DUAL:
        if (device->GetName())
          result.push_back(device);
        break;
    }
  }
  return result;
}

}  // namespace

device::BluetoothAdapter::DeviceList FilterBluetoothDeviceList(
    const BluetoothAdapter::DeviceList& devices,
    BluetoothFilterType filter_type,
    int max_devices) {
  BluetoothAdapter::DeviceList filtered_devices =
      filter_type == BluetoothFilterType::KNOWN ? FilterUnknownDevices(devices)
                                                : devices;
  return GetLimitedNumDevices(max_devices, filtered_devices);
}

}  // namespace device
