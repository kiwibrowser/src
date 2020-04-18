// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/test/mock_bluetooth_gatt_service.h"

#include <memory>
#include <utility>

#include "device/bluetooth/test/mock_bluetooth_device.h"

using testing::Return;
using testing::_;

namespace device {

MockBluetoothGattService::MockBluetoothGattService(
    MockBluetoothDevice* device,
    const std::string& identifier,
    const BluetoothUUID& uuid,
    bool is_primary,
    bool is_local) {
  ON_CALL(*this, GetIdentifier()).WillByDefault(Return(identifier));
  ON_CALL(*this, GetUUID()).WillByDefault(Return(uuid));
  ON_CALL(*this, IsLocal()).WillByDefault(Return(is_local));
  ON_CALL(*this, IsPrimary()).WillByDefault(Return(is_primary));
  ON_CALL(*this, GetDevice()).WillByDefault(Return(device));
  ON_CALL(*this, GetCharacteristics())
      .WillByDefault(Return(std::vector<BluetoothRemoteGattCharacteristic*>()));
  ON_CALL(*this, GetIncludedServices())
      .WillByDefault(Return(std::vector<BluetoothRemoteGattService*>()));
  ON_CALL(*this, GetCharacteristic(_))
      .WillByDefault(
          Return(static_cast<BluetoothRemoteGattCharacteristic*>(NULL)));
}

MockBluetoothGattService::~MockBluetoothGattService() = default;

void MockBluetoothGattService::AddMockCharacteristic(
    std::unique_ptr<MockBluetoothGattCharacteristic> mock_characteristic) {
  mock_characteristics_.push_back(std::move(mock_characteristic));
}

std::vector<BluetoothRemoteGattCharacteristic*>
MockBluetoothGattService::GetMockCharacteristics() const {
  std::vector<BluetoothRemoteGattCharacteristic*> characteristics;
  for (const auto& characteristic : mock_characteristics_)
    characteristics.push_back(characteristic.get());

  return characteristics;
}

BluetoothRemoteGattCharacteristic*
MockBluetoothGattService::GetMockCharacteristic(
    const std::string& identifier) const {
  for (const auto& characteristic : mock_characteristics_)
    if (characteristic->GetIdentifier() == identifier)
      return characteristic.get();

  return nullptr;
}

}  // namespace device
