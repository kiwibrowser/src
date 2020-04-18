// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_remote_gatt_service.h"

#include "device/bluetooth/bluetooth_remote_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_uuid.h"

namespace device {

BluetoothRemoteGattService::BluetoothRemoteGattService() = default;

BluetoothRemoteGattService::~BluetoothRemoteGattService() = default;

std::vector<BluetoothRemoteGattCharacteristic*>
BluetoothRemoteGattService::GetCharacteristicsByUUID(
    const BluetoothUUID& characteristic_uuid) {
  std::vector<BluetoothRemoteGattCharacteristic*> result;
  std::vector<BluetoothRemoteGattCharacteristic*> characteristics =
      GetCharacteristics();
  for (auto* characteristic : characteristics) {
    if (characteristic->GetUUID() == characteristic_uuid) {
      result.push_back(characteristic);
    }
  }
  return result;
}

bool BluetoothRemoteGattService::IsDiscoveryComplete() const {
  return discovery_complete_;
}

void BluetoothRemoteGattService::SetDiscoveryComplete(bool complete) {
  discovery_complete_ = complete;
}

}  // namespace device
