// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_TEST_MOCK_BLUETOOTH_GATT_CHARACTERISTIC_H_
#define DEVICE_BLUETOOTH_TEST_MOCK_BLUETOOTH_GATT_CHARACTERISTIC_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_remote_gatt_descriptor.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace device {

class BluetoothRemoteGattDescriptor;
class BluetoothRemoteGattService;
class MockBluetoothGattDescriptor;
class MockBluetoothGattService;

class MockBluetoothGattCharacteristic
    : public BluetoothRemoteGattCharacteristic {
 public:
  MockBluetoothGattCharacteristic(MockBluetoothGattService* service,
                                  const std::string& identifier,
                                  const BluetoothUUID& uuid,
                                  bool is_local,
                                  Properties properties,
                                  Permissions permissions);
  ~MockBluetoothGattCharacteristic() override;

  MOCK_CONST_METHOD0(GetIdentifier, std::string());
  MOCK_CONST_METHOD0(GetUUID, BluetoothUUID());
  MOCK_CONST_METHOD0(IsLocal, bool());
  MOCK_CONST_METHOD0(GetValue, const std::vector<uint8_t>&());
  MOCK_CONST_METHOD0(GetService, BluetoothRemoteGattService*());
  MOCK_CONST_METHOD0(GetProperties, Properties());
  MOCK_CONST_METHOD0(GetPermissions, Permissions());
  MOCK_CONST_METHOD0(IsNotifying, bool());
  MOCK_CONST_METHOD0(GetDescriptors,
                     std::vector<BluetoothRemoteGattDescriptor*>());
  MOCK_CONST_METHOD1(GetDescriptor,
                     BluetoothRemoteGattDescriptor*(const std::string&));
  MOCK_METHOD1(AddDescriptor, bool(BluetoothRemoteGattDescriptor*));
  MOCK_METHOD1(UpdateValue, bool(const std::vector<uint8_t>&));
  MOCK_METHOD2(StartNotifySession,
               void(const NotifySessionCallback&, const ErrorCallback&));
  MOCK_METHOD2(StopNotifySession,
               void(BluetoothGattNotifySession*, const base::Closure&));
  MOCK_METHOD2(ReadRemoteCharacteristic,
               void(const ValueCallback&, const ErrorCallback&));
  MOCK_METHOD3(WriteRemoteCharacteristic,
               void(const std::vector<uint8_t>&,
                    const base::Closure&,
                    const ErrorCallback&));

  void AddMockDescriptor(
      std::unique_ptr<MockBluetoothGattDescriptor> mock_descriptor);
  std::vector<BluetoothRemoteGattDescriptor*> GetMockDescriptors() const;
  BluetoothRemoteGattDescriptor* GetMockDescriptor(
      const std::string& identifier) const;

 protected:
  MOCK_METHOD3(SubscribeToNotifications,
               void(BluetoothRemoteGattDescriptor*,
                    const base::Closure&,
                    const ErrorCallback&));
  MOCK_METHOD3(UnsubscribeFromNotifications,
               void(BluetoothRemoteGattDescriptor*,
                    const base::Closure&,
                    const ErrorCallback&));

 private:
  std::vector<std::unique_ptr<MockBluetoothGattDescriptor>> mock_descriptors_;

  DISALLOW_COPY_AND_ASSIGN(MockBluetoothGattCharacteristic);
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_TEST_MOCK_BLUETOOTH_GATT_CHARACTERISTIC_H_
