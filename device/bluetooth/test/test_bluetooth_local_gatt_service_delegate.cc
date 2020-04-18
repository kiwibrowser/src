// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <device/bluetooth/test/test_bluetooth_local_gatt_service_delegate.h>
#include "base/callback.h"
#include "device/bluetooth/test/bluetooth_gatt_server_test.h"

namespace device {

TestBluetoothLocalGattServiceDelegate::TestBluetoothLocalGattServiceDelegate()
    : should_fail_(false),
      last_written_value_(0),
      value_to_write_(0),
      expected_service_(nullptr),
      expected_characteristic_(nullptr),
      expected_descriptor_(nullptr) {}

TestBluetoothLocalGattServiceDelegate::
    ~TestBluetoothLocalGattServiceDelegate() = default;

void TestBluetoothLocalGattServiceDelegate::OnCharacteristicReadRequest(
    const BluetoothDevice* device,
    const BluetoothLocalGattCharacteristic* characteristic,
    int offset,
    const ValueCallback& callback,
    const ErrorCallback& error_callback) {
  EXPECT_EQ(expected_characteristic_->GetIdentifier(),
            characteristic->GetIdentifier());
  if (should_fail_) {
    error_callback.Run();
    return;
  }
  last_seen_device_ = device->GetIdentifier();
  callback.Run(BluetoothGattServerTest::GetValue(value_to_write_));
}

void TestBluetoothLocalGattServiceDelegate::OnCharacteristicWriteRequest(
    const BluetoothDevice* device,
    const BluetoothLocalGattCharacteristic* characteristic,
    const std::vector<uint8_t>& value,
    int offset,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  EXPECT_EQ(expected_characteristic_->GetIdentifier(),
            characteristic->GetIdentifier());
  if (should_fail_) {
    error_callback.Run();
    return;
  }
  last_seen_device_ = device->GetIdentifier();
  last_written_value_ = BluetoothGattServerTest::GetInteger(value);
  callback.Run();
}

void TestBluetoothLocalGattServiceDelegate::OnDescriptorReadRequest(
    const BluetoothDevice* device,
    const BluetoothLocalGattDescriptor* descriptor,
    int offset,
    const ValueCallback& callback,
    const ErrorCallback& error_callback) {
  EXPECT_EQ(expected_descriptor_->GetIdentifier(), descriptor->GetIdentifier());
  if (should_fail_) {
    error_callback.Run();
    return;
  }
  last_seen_device_ = device->GetIdentifier();
  callback.Run(BluetoothGattServerTest::GetValue(value_to_write_));
}

void TestBluetoothLocalGattServiceDelegate::OnDescriptorWriteRequest(
    const BluetoothDevice* device,
    const BluetoothLocalGattDescriptor* descriptor,
    const std::vector<uint8_t>& value,
    int offset,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  EXPECT_EQ(expected_descriptor_->GetIdentifier(), descriptor->GetIdentifier());
  if (should_fail_) {
    error_callback.Run();
    return;
  }
  last_seen_device_ = device->GetIdentifier();
  last_written_value_ = BluetoothGattServerTest::GetInteger(value);
  callback.Run();
}

void TestBluetoothLocalGattServiceDelegate::OnNotificationsStart(
    const BluetoothDevice* device,
    const BluetoothLocalGattCharacteristic* characteristic) {
  EXPECT_EQ(expected_characteristic_->GetIdentifier(),
            characteristic->GetIdentifier());
  notifications_started_for_characteristic_[characteristic->GetIdentifier()] =
      true;
}

void TestBluetoothLocalGattServiceDelegate::OnNotificationsStop(
    const BluetoothDevice* device,
    const BluetoothLocalGattCharacteristic* characteristic) {
  EXPECT_EQ(expected_characteristic_->GetIdentifier(),
            characteristic->GetIdentifier());
  notifications_started_for_characteristic_[characteristic->GetIdentifier()] =
      false;
}

bool TestBluetoothLocalGattServiceDelegate::NotificationStatusForCharacteristic(
    BluetoothLocalGattCharacteristic* characteristic) {
  auto found = notifications_started_for_characteristic_.find(
      characteristic->GetIdentifier());
  if (found == notifications_started_for_characteristic_.end())
    return false;
  return found->second;
}

}  // namespace device
